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

#include "packet.h"
#include <cstring>
#include <QDebug>

namespace {
// CRC Table
const unsigned short crc16_tab[] = { 0x0000, 0x1021, 0x2042, 0x3063, 0x4084,
        0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad,
        0xe1ce, 0xf1ef, 0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7,
        0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a,
        0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672,
        0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719,
        0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7,
        0x0840, 0x1861, 0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948,
        0x9969, 0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50,
        0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b,
        0xab1a, 0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
        0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97,
        0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe,
        0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca,
        0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3,
        0x5004, 0x4025, 0x7046, 0x6067, 0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d,
        0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214,
        0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c,
        0xc50d, 0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3,
        0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d,
        0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806,
        0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e,
        0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1,
        0x1ad0, 0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b,
        0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0,
        0x0cc1, 0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
        0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0 };
}

Packet::Packet(QObject *parent) : QObject(parent)
{
    mRxTimer = 0;
    mByteTimeout = 50;
    mMaxPacketLen = 512;
    mRxReadPtr = 0;
    mRxWritePtr = 0;
    mBytesLeft = 0;
    mBufferLen = mMaxPacketLen + 8;
    mRxBuffer = new unsigned char[mBufferLen];

    mTimer = new QTimer(this);
    mTimer->setInterval(10);
    mTimer->start();

    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
}

Packet::~Packet()
{
    delete[] mRxBuffer;
}

void Packet::sendPacket(const QByteArray &data)
{
    if (data.size() == 0 || data.size() > (int)mMaxPacketLen) {
        return;
    }

    QByteArray to_send;
    unsigned int len_tot = data.size();

    if (len_tot <= 255) {
        to_send.append((char)2);
        to_send.append((char)len_tot);
    } else if (len_tot <= 65535) {
        to_send.append((char)3);
        to_send.append((char)(len_tot >> 8));
        to_send.append((char)(len_tot & 0xFF));
    } else {
        to_send.append((char)4);
        to_send.append((char)((len_tot >> 16) & 0xFF));
        to_send.append((char)((len_tot >> 8) & 0xFF));
        to_send.append((char)(len_tot & 0xFF));
    }

    unsigned short crc = crc16((const unsigned char*)data.data(), len_tot);

    to_send.append(data);
    to_send.append((char)(crc >> 8));
    to_send.append((char)(crc & 0xFF));
    to_send.append((char)3);

    emit dataToSend(to_send);
}

unsigned short Packet::crc16(const unsigned char *buf, unsigned int len)
{
    unsigned short cksum = 0;
    for (unsigned int i = 0; i < len; i++) {
        cksum = crc16_tab[(((cksum >> 8) ^ *buf++) & 0xFF)] ^ (cksum << 8);
    }
    return cksum;
}

void Packet::processData(QByteArray data)
{
    QVector<QByteArray> decodedPackets;

    for(unsigned char rx_data: data) {
        mRxTimer = mByteTimeout;

        unsigned int data_len = mRxWritePtr - mRxReadPtr;

        // Out of space (should not happen)
        if (data_len >= mBufferLen) {
            mRxWritePtr = 0;
            mRxReadPtr = 0;
            mBytesLeft = 0;
            mRxBuffer[mRxWritePtr++] = rx_data;
            continue;
        }

        // Everything has to be aligned, so shift buffer if we are out of space.
        // (as opposed to using a circular buffer)
        if (mRxWritePtr >= mBufferLen) {
            memmove(mRxBuffer, mRxBuffer + mRxReadPtr, data_len);
            mRxReadPtr = 0;
            mRxWritePtr = data_len;
        }

        mRxBuffer[mRxWritePtr++] = rx_data;
        data_len++;

        if (mBytesLeft > 1) {
            mBytesLeft--;
            continue;
        }

        // Try decoding the packet at various offsets until it succeeds, or
        // until we run out of data.
        for (;;) {
            int res = try_decode_packet(mRxBuffer + mRxReadPtr, data_len,
                                        &mBytesLeft, decodedPackets);

            // More data is needed
            if (res == -2) {
                break;
            }

            if (res > 0) {
                data_len -= res;
                mRxReadPtr += res;
            } else if (res == -1) {
                // Something went wrong. Move pointer forward and try again.
                mRxReadPtr++;
                data_len--;
            }
        }

        // Nothing left, move pointers to avoid memmove
        if (data_len == 0) {
            mRxReadPtr = 0;
            mRxWritePtr = 0;
        }
    }

    for (QByteArray b: decodedPackets) {
        emit packetReceived(b);
    }
}

void Packet::timerSlot()
{
    if (mRxTimer) {
        mRxTimer--;
    } else {
        mRxReadPtr = 0;
        mRxWritePtr = 0;
        mBytesLeft = 0;
    }
}

int Packet::try_decode_packet(unsigned char *buffer, unsigned int in_len,
                              int *bytes_left, QVector<QByteArray> &decodedPackets)
{
    *bytes_left = 0;

    if (in_len == 0) {
        *bytes_left = 1;
        return -2;
    }

    unsigned int data_start = buffer[0];
    bool is_len_8b = buffer[0] == 2;
    bool is_len_16b = buffer[0] == 3;
    bool is_len_24b = buffer[0] == 4;

    // No valid start byte
    if (!is_len_8b && !is_len_16b && !is_len_24b) {
        return -1;
    }

    // Not enough data to determine length
    if (in_len < data_start) {
        *bytes_left = data_start - in_len;
        return -2;
    }

    unsigned int len = 0;

    if (is_len_8b) {
        len = (unsigned int)buffer[1];

        // No support for zero length packets
        if (len < 1) {
            return -1;
        }
    } else if (is_len_16b) {
        len = (unsigned int)buffer[1] << 8 | (unsigned int)buffer[2];

        // A shorter packet should use less length bytes
        if (len < 255) {
            return -1;
        }
    } else if (is_len_24b) {
        len = (unsigned int)buffer[1] << 16 |
              (unsigned int)buffer[2] << 8 |
              (unsigned int)buffer[3];

        // A shorter packet should use less length bytes
        if (len < 65535) {
            return -1;
        }
    }

    // Too long packet
    if (len > mMaxPacketLen) {
        return -1;
    }

    // Need more data to determine rest of packet
    if (in_len < (len + data_start + 3)) {
        *bytes_left = (len + data_start + 3) - in_len;
        return -2;
    }

    // Invalid stop byte
    if (buffer[data_start + len + 2] != 3) {
        return -1;
    }

    unsigned short crc_calc = crc16(buffer + data_start, len);
    unsigned short crc_rx = (unsigned short)buffer[data_start + len] << 8
                          | (unsigned short)buffer[data_start + len + 1];

    if (crc_calc == crc_rx) {
        QByteArray res((const char*)(buffer + data_start), (int)len);
        decodedPackets.append(res);
        return len + data_start + 3;
    } else {
        return -1;
    }
}
