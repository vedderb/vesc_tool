/*
    Copyright 2021 Radinn AB.

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
#include "hexfile.h"

#include <QFile>
#include <QDebug>

enum {
    RecordType_Data = 0,
    RecordType_EndOfFile,
    RecordType_ExtendedSegmentAddress,
    RecordType_StartSegmentAddress,
    RecordType_ExtenededLinearAddress,
    RecordType_StartLinearAddress,
};

bool HexFile::parseFile(QString fileName, QMap<quint32, QByteArray> &output)
{
    bool endOfFileSeen = false;
    quint32 currentOffset = 0;
    quint32 currentAddress = 0;
    QByteArray currentBytes;
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open" << fileName;
        return false;
    }

    while (!file.atEnd()) {
        QByteArray line = file.readLine();

        if (line.size() == 0) {
            continue;
        }
        if (endOfFileSeen) {
            qDebug() << "End-of-file record found in the middle of the file";
            return false;
        }
        if (line[0] != ':') {
            qDebug() << "Invalid line start:" << line;
            return false;
        }

        // The starting colon will be ignored by fromHex()
        QByteArray binaryLine = QByteArray::fromHex(line);

        if (binaryLine.size() < 5) {
            qDebug() << "Line too short:" << line;
            return false;
        }

        quint8 dataLength = quint8(binaryLine[0]);
        quint32 address = (quint32(binaryLine[1] & 0xFF) << 8) | quint32(binaryLine[2] & 0xFF);
        quint8 recordType = quint8(binaryLine[3]);

        if (binaryLine.size() < 5 + dataLength) {
            qDebug() << "Missing data in line:" << line;
            return false;
        }

        quint8 checksum = 0;
        for (int i = 0; i < binaryLine.size() - 1; i++) {
            checksum += binaryLine[i];
        }
        quint8 embeddedChecksum = quint8(binaryLine[4 + dataLength]);
        if (quint8(checksum + embeddedChecksum) != 0) {
            qDebug() << "Invalid checksum in line:" << line;
            return false;
        };

        QByteArray data = binaryLine.mid(4, dataLength);

        switch (recordType) {
        case RecordType_Data:
            address |= currentOffset;
            if (address == currentAddress + quint32(currentBytes.size())) {
                currentBytes.append(data);
            } else {
                if (currentBytes.size() > 0) {
                    output.insert(currentAddress, currentBytes);
                }
                currentBytes = data;
                currentAddress = address;
            }
            break;
        case RecordType_EndOfFile:
            endOfFileSeen = true;
            break;
        case RecordType_ExtendedSegmentAddress:
            if (dataLength != 2) {
                qDebug() << "Invalid data length for extended segment address:" << dataLength;
                return false;
            }
            currentOffset = ((quint32(data[0] & 0xFF) << 8) | quint32(data[1] & 0xFF)) << 4;
            break;
        case RecordType_StartSegmentAddress:
            // Start address is not used
            break;
        case RecordType_ExtenededLinearAddress:
            if (dataLength != 2) {
                qDebug() << "Invalid data length for extended linear address:" << dataLength;
                return false;
            }
            currentOffset = ((quint32(data[0] & 0xFF) << 8) | quint32(data[1] & 0xFF)) << 16;
            break;
        case RecordType_StartLinearAddress:
            // Start address is not used
            break;
        default:
            qDebug() << "Unknown record type:" << line;
            return false;
        }
    }

    if (!endOfFileSeen) {
        qDebug() << "File truncated";
        return false;
    }

    if (currentBytes.size() > 0) {
        output.insert(currentAddress, currentBytes);
    }

    return true;
}
