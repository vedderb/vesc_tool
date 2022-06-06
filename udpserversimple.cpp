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

#include "udpserversimple.h"
#include <QDebug>
#include <QNetworkDatagram>

UdpServerSimple::UdpServerSimple(QObject *parent) : QObject(parent)
{
    mUdpSocket = new QUdpSocket(this);
    mPacket = new Packet(this);
    mUsePacket = false;

    connect(mPacket, SIGNAL(dataToSend(QByteArray&)),
            this, SLOT(dataToSend(QByteArray&)));
    connect(mUdpSocket, &QUdpSocket::readyRead,
                this, &UdpServerSimple::udpInputDataAvailable);
}

bool UdpServerSimple::startServer(int port, QHostAddress addr)
{
    mUdpSocket->close();
    if (!mUdpSocket->bind(addr,  port)) {
        return false;
    }

    return true;
}

bool UdpServerSimple::startServerBroadcast(int port)
{
    mUdpSocket->close();
    return mUdpSocket->bind(QHostAddress::Any, port, QUdpSocket::ShareAddress);
}

void UdpServerSimple::stopServer()
{
    emit connectionChanged(false, mUdpSocket->peerAddress().toString());
    mUdpSocket->close();
    clientAddr.clear();
}

bool UdpServerSimple::sendData(const QByteArray &data)
{
    if (!clientAddr.isNull())
        return mUdpSocket->writeDatagram(data, clientAddr, clientPort) == data.length();

    return false;
}

QString UdpServerSimple::errorString()
{
    return mUdpSocket->errorString();
}

Packet *UdpServerSimple::packet()
{
    return mPacket;
}

void UdpServerSimple::udpInputDataAvailable()
{
    while (mUdpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = mUdpSocket->receiveDatagram();
        clientAddr = datagram.senderAddress();
        clientPort = datagram.senderPort();
        emit dataRx(datagram.data());
        if (mUsePacket) {
            mPacket->processData(datagram.data());
        }
    }
}

void UdpServerSimple::dataToSend(QByteArray &data)
{
    sendData(data);
}

bool UdpServerSimple::usePacket() const
{
    return mUsePacket;
}

void UdpServerSimple::setUsePacket(bool usePacket)
{
    mUsePacket = usePacket;
}

bool UdpServerSimple::isClientConnected()
{
    return !clientAddr.isNull();
}

QString UdpServerSimple::getConnectedClientIp()
{
    QString res;

    if (!clientAddr.isNull())
        res = clientAddr.toString() + ":" + QString::number(clientPort);

    return res;
}

bool UdpServerSimple::isServerRunning()
{
    return mUdpSocket->state() == QAbstractSocket::BoundState;
}
