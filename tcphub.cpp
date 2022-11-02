/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se
    Copyright 2022 Joel Svensson    svenssonjoel@yahoo.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include "tcphub.h"
#include "utility.h"
#include <QDateTime>
#include <QEventLoop>
#include <QtDebug>
#include <QHostInfo>

TcpHub::TcpHub(QObject *parent)
    : QObject{parent}
{
    mTcpHubServer = new QTcpServer(this);
    connect(mTcpHubServer, SIGNAL(newConnection()), this, SLOT(newTcpHubConnection()));
}

TcpHub::~TcpHub()
{
    QMapIterator<QString, TcpConnectedVesc*> i(mConnectedVescs);
    while (i.hasNext()) {
        i.next();
        delete i.value();
    }
}

bool TcpHub::start(int port, QHostAddress addr)
{
    return mTcpHubServer->listen(addr,  port);
}

bool TcpHub::ping(QString server, int port, QString uuid)
{
    QHostAddress host;
    host.setAddress(server);

    // Try DNS lookup
    if (host.isNull()) {
        QList<QHostAddress> addresses = QHostInfo::fromName(server).addresses();

        if (!addresses.isEmpty()) {
            host.setAddress(addresses.first().toString());
        }
    }

    if (host.isNull()) {
        return false;
    }

    QTcpSocket socket;
    socket.connectToHost(host, port);
    if (!Utility::waitSignal(&socket, SIGNAL(connected()), 1000)) {
        return false;
    }

    socket.write(QString("PING:%1:0\n").arg(uuid).toLocal8Bit());
    socket.flush();

    auto res = Utility::waitForLine(&socket, 1000);

    return res == "PONG";
}

void TcpHub::newTcpHubConnection()
{
    QTcpSocket *socket = mTcpHubServer->nextPendingConnection();

    socket->setSocketOption(QAbstractSocket::LowDelayOption, true);
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, true);

    QString connStr = Utility::waitForLine(socket, 5000);

    if (connStr.isEmpty()) {
        socket->close();
        socket->deleteLater();
        qWarning() << "Waiting for connect string timed out";
        return;
    }

    auto tokens = connStr.split(":");
    if (tokens.size() == 3) {
        auto type = tokens.at(0).toUpper().replace(" ", "");
        auto uuid = tokens.at(1).toUpper().replace(" ", "");
        auto pass = tokens.at(2);

        if (uuid.length() < 3) {
            qWarning() << "Too short UUID";
            socket->close();
            socket->deleteLater();
            return;
        }

        if (type == "VESC") {
            if (mConnectedVescs.contains(uuid)) {
                mConnectedVescs[uuid]->vescSocket->close();
            }

            TcpConnectedVesc *v = new TcpConnectedVesc;
            v->vescSocket = socket;
            v->pass = pass;
            mConnectedVescs.insert(uuid, v);

            connect(v->vescSocket, &QTcpSocket::disconnected, [v, uuid, this]() {
                qDebug() << tr("VESC with UUID %1 disconnected").arg(uuid);
                mConnectedVescs.remove(uuid);
                QTimer::singleShot(0, [v]() {
                    delete v;
                });
            });

            qDebug() << tr("VESC with UUID %1 connected").arg(uuid);
            return;
        } else if (type == "VESCTOOL") {
            if (mConnectedVescs.contains(uuid)) {
                TcpConnectedVesc *v = mConnectedVescs[uuid];
                if (v->pass == pass) {
                    if (v->vescToolSocket != nullptr) {
                        v->vescToolSocket->close();
                        v->vescToolSocket->deleteLater();
                    }
                    v->vescToolSocket = socket;

                    connect(v->vescToolSocket, &QTcpSocket::readyRead, [v]() {
                        if (v->vescSocket != nullptr && v->vescSocket->isOpen()) {
                            v->vescSocket->write(v->vescToolSocket->readAll());
                        }
                    });

                    connect(v->vescSocket, &QTcpSocket::readyRead, [v]() {
                        if (v->vescToolSocket != nullptr && v->vescToolSocket->isOpen()) {
                            v->vescToolSocket->write(v->vescSocket->readAll());
                        }
                    });

                    connect(v->vescToolSocket, &QTcpSocket::disconnected, [uuid]() {
                        qDebug() << "VESC Tool disconnected from" << uuid;
                    });

                    qDebug() << "VESC Tool connected to" << uuid;

                    return;
                } else {
                    qWarning() << "Invalid password" << pass << v->pass;
                }
            } else {
                qWarning() << tr("No VESC with UUID %1 found").arg(uuid);
            }
        } else if (type == "PING") {
            if (mConnectedVescs.contains(uuid)) {
                socket->write("PONG\n");
                socket->flush();
            } else {
                socket->write("NULL\n");
                socket->flush();
            }
        } else {
            qWarning() << "Invalid connect string";
        }
    } else {
        qWarning() << "Invalid connect string";
    }

    socket->close();
    socket->deleteLater();
}
