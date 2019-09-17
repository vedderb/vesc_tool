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

TcpServerSimple::TcpServerSimple(QObject *parent) : QObject(parent)
{
    mTcpServer = new QTcpServer(this);
    mPacket = new Packet(this);
    mTcpSocket = nullptr;
    mUsePacket = false;

    connect(mTcpServer, SIGNAL(newConnection()), this, SLOT(newTcpConnection()));
    connect(mPacket, SIGNAL(dataToSend(QByteArray&)),
            this, SLOT(dataToSend(QByteArray&)));
}

bool TcpServerSimple::startServer(int port, QHostAddress addr)
{
    if (!mTcpServer->listen(addr,  port)) {
        return false;
    }

    return true;
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
    mTcpSocket = 0;
}

void TcpServerSimple::tcpInputDataAvailable()
{
    QByteArray data = mTcpSocket->readAll();
    emit dataRx(data);

    if (mUsePacket) {
        mPacket->processData(data);
    }
}

void TcpServerSimple::tcpInputError(QAbstractSocket::SocketError socketError)
{
    (void)socketError;
    mTcpSocket->abort();
//    qDebug() << socketError;
}

void TcpServerSimple::dataToSend(QByteArray &data)
{
    sendData(data);
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
