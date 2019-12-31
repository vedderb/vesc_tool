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

#ifndef TCPSERVERSIMPLE_H
#define TCPSERVERSIMPLE_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include "packet.h"

class TcpServerSimple : public QObject
{
    Q_OBJECT
public:
    explicit TcpServerSimple(QObject *parent = nullptr);
    bool startServer(int port, QHostAddress addr = QHostAddress::Any);
    void stopServer();
    bool sendData(const QByteArray &data);
    QString errorString();
    Packet *packet();
    bool usePacket() const;
    void setUsePacket(bool usePacket);
    bool isClientConnected();
    QString getConnectedClientIp();
    bool isServerRunning();

signals:
    void dataRx(const QByteArray &data);
    void connectionChanged(bool connected, QString address);

public slots:
    void newTcpConnection();
    void tcpInputDisconnected();
    void tcpInputDataAvailable();
    void tcpInputError(QAbstractSocket::SocketError socketError);
    void dataToSend(QByteArray &data);

private:
    QTcpServer *mTcpServer;
    QTcpSocket *mTcpSocket;
    Packet *mPacket;
    bool mUsePacket;

};

#endif // TCPSERVERSIMPLE_H
