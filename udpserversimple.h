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

#ifndef UDPSERVERSIMPLE_H
#define UDPSERVERSIMPLE_H

#include <QObject>
#include <QUdpSocket>
#include "packet.h"

class UdpServerSimple : public QObject
{
    Q_OBJECT
public:
    explicit UdpServerSimple(QObject *parent = nullptr);
    Q_INVOKABLE bool startServer(int port, QHostAddress addr = QHostAddress::Any);
    Q_INVOKABLE bool startServerBroadcast(int port);
    Q_INVOKABLE void stopServer();
    Q_INVOKABLE bool sendData(const QByteArray &data);
    QString errorString();
    Packet *packet();
    bool usePacket() const;
    void setUsePacket(bool usePacket);
    Q_INVOKABLE bool isClientConnected();
    Q_INVOKABLE QString getConnectedClientIp();
    Q_INVOKABLE bool isServerRunning();

signals:
    void dataRx(const QByteArray &data);
    void connectionChanged(bool connected, QString address);

public slots:
    void udpInputDataAvailable();
    void dataToSend(QByteArray &data);

private:
    QUdpSocket *mUdpSocket;
    QHostAddress clientAddr;    // Only one client possible
    quint16 clientPort;
    Packet *mPacket;
    bool mUsePacket;

};

#endif // UDPSERVERSIMPLE_H
