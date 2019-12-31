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

#ifndef PACKET_H
#define PACKET_H

#include <QObject>
#include <QTimer>

class Packet : public QObject
{
    Q_OBJECT
public:
    explicit Packet(QObject *parent = 0);
    ~Packet();
    void sendPacket(const QByteArray &data);
    static unsigned short crc16(const unsigned char *buf, unsigned int len);

signals:
    void dataToSend(QByteArray &data);
    void packetReceived(QByteArray &packet);

public slots:
    void processData(QByteArray data);

private slots:
    void timerSlot();

private:
    QTimer *mTimer;
    int mRxTimer;
    int mByteTimeout;
    unsigned int mRxReadPtr;
    unsigned int mRxWritePtr;
    int mBytesLeft;
    unsigned int mMaxPacketLen;
    unsigned int mBufferLen;
    unsigned char *mRxBuffer;

    int try_decode_packet(unsigned char *buffer, unsigned int in_len,
                          int *bytes_left, QVector<QByteArray> &decodedPackets);

};

#endif // PACKET_H
