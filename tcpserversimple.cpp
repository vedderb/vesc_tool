/*
    Copyright 2016 - 2019 Benjamin Vedder	benjamin@vedder.se

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

#include "tcpserversimple.h"
#include <QDebug>
#include <QHostInfo>
#include <QEventLoop>
#include <QTimer>

TcpServerSimple::TcpServerSimple(QObject *parent) : QObject(parent)
{
    mTcpServer = new QTcpServer(this);
    mPacket = new Packet(this);
    mTcpSocket = nullptr;
    mUsePacket = false;
    mLastPort = -1;

    connect(mTcpServer, SIGNAL(newConnection()), this, SLOT(newTcpConnection()));
    connect(mPacket, SIGNAL(dataToSend(QByteArray&)),
            this, SLOT(dataToSend(QByteArray&)));
}

bool TcpServerSimple::startServer(int port, QHostAddress addr)
{
    mLastPort = port;
    return mTcpServer->listen(addr,  port);
}

bool TcpServerSimple::connectToHub(QString server, int port, QString id, QString pass)
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

    stopServer();

    mTcpSocket = new QTcpSocket(this);
    mTcpSocket->connectToHost(host, port);

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(3000);
    auto conn = QObject::connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));

    connect(mTcpSocket, &QTcpSocket::connected, [&id, &pass, this, &loop]() {
        QString login = QString("VESC:%1:%2\n").arg(id).arg(pass);
        mTcpSocket->write(login.toLocal8Bit());
        loop.quit();
    });

    loop.exec();
    disconnect(conn);

    if (timeoutTimer.isActive()) {
        connect(mTcpSocket, SIGNAL(readyRead()), this, SLOT(tcpInputDataAvailable()));
        connect(mTcpSocket, SIGNAL(disconnected()),
                this, SLOT(tcpInputDisconnected()));
        connect(mTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(tcpInputError(QAbstractSocket::SocketError)));
        emit connectionChanged(true, mTcpSocket->peerAddress().toString());
        return true;
    } else {
        qWarning() << "Connection timed out";
    }

    stopServer();
    return false;
}

void TcpServerSimple::stopServer()
{
    mTcpServer->close();

    if (mTcpSocket) {
        emit connectionChanged(false, mTcpSocket->peerAddress().toString());
        mTcpSocket->close();
        delete mTcpSocket;
        mTcpSocket = 0;
    }
}

bool TcpServerSimple::sendData(const QByteArray &data)
{
    bool res = false;

    if (mTcpSocket) {
        mTcpSocket->write(data);
        res = true;
    }

    return res;
}

QString TcpServerSimple::errorString()
{
    return mTcpServer->errorString();
}

Packet *TcpServerSimple::packet()
{
    return mPacket;
}

void TcpServerSimple::newTcpConnection()
{
    QTcpSocket *socket = mTcpServer->nextPendingConnection();
    socket->setSocketOption(QAbstractSocket::LowDelayOption, true);

    if (mTcpSocket) {
        socket->close();
        delete socket;
    } else {
        mTcpSocket = socket;

        if (mTcpSocket) {
            connect(mTcpSocket, SIGNAL(readyRead()), this, SLOT(tcpInputDataAvailable()));
            connect(mTcpSocket, SIGNAL(disconnected()),
                    this, SLOT(tcpInputDisconnected()));
            connect(mTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                    this, SLOT(tcpInputError(QAbstractSocket::SocketError)));
            emit connectionChanged(true, mTcpSocket->peerAddress().toString());
        }
    }
}

void TcpServerSimple::tcpInputDisconnected()
{
    emit connectionChanged(false, mTcpSocket->peerAddress().toString());
    mTcpSocket->deleteLater();
    mTcpSocket = nullptr;
}

void TcpServerSimple::tcpInputDataAvailable()
{
    while (mTcpSocket->bytesAvailable() > 0) {
        QByteArray data = mTcpSocket->readAll();
        emit dataRx(data);

        if (mUsePacket) {
            mPacket->processData(data);
        }
    }
}

void TcpServerSimple::tcpInputError(QAbstractSocket::SocketError socketError)
{
    (void)socketError;
    mTcpSocket->abort();
    qDebug() << socketError;
}

void TcpServerSimple::dataToSend(QByteArray &data)
{
    sendData(data);
}

int TcpServerSimple::lastPort() const
{
    return mLastPort;
}

bool TcpServerSimple::usePacket() const
{
    return mUsePacket;
}

void TcpServerSimple::setUsePacket(bool usePacket)
{
    mUsePacket = usePacket;
}

bool TcpServerSimple::isClientConnected()
{
    return mTcpSocket != nullptr;
}

QString TcpServerSimple::getConnectedClientIp()
{
    QString res;

    if (mTcpSocket != nullptr && !mTcpSocket->peerAddress().isNull()) {
        res = mTcpSocket->peerAddress().toString();
    }

    return res;
}

bool TcpServerSimple::isServerRunning()
{
    return mTcpServer->isListening();
}
