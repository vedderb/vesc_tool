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
#include <QDateTime>
#include <QEventLoop>
#include <QtDebug>

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

void TcpHub::newTcpHubConnection()
{
    QTcpSocket *socket = mTcpHubServer->nextPendingConnection();
    socket->setSocketOption(QAbstractSocket::LowDelayOption, true);

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(5000);
    auto conn = QObject::connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));

    QByteArray rxLine;
    auto conn2 = connect(socket, &QTcpSocket::readyRead, [&rxLine,socket,&loop]() {
        while (socket->bytesAvailable() > 0) {
            QByteArray rxb = socket->read(1);
            if (rxb.size() == 1) {
                if (rxb[0] != '\n') {
                    rxLine.append(rxb[0]);
                } else {
                    loop.quit();
                }
            } else {
                break;
            }
        }
    });

    loop.exec();

    disconnect(conn);
    disconnect(conn2);

    if (!timeoutTimer.isActive()) {
        socket->close();
        socket->deleteLater();
        qWarning() << "Waiting for connect string timed out";
        return;
    }

    QString connStr(rxLine);
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
                delete mConnectedVescs[uuid];
                mConnectedVescs.remove(uuid);
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
        } else {
            qWarning() << "Invalid connect string";
        }
    } else {
        qWarning() << "Invalid connect string";
    }

    socket->close();
    socket->deleteLater();
}
