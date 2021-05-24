/*
    Copyright 2016 - 2017 Benjamin Vedder	benjamin@vedder.se

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

#ifndef VBYTEARRAY_H
#define VBYTEARRAY_H

#include <QByteArray>
#include <QString>

class VByteArray : public QByteArray
{
public:
    VByteArray();
    VByteArray(const QByteArray &data);

    void vbAppendInt32(qint32 number);
    void vbAppendUint32(quint32 number);
    void vbAppendInt16(qint16 number);
    void vbAppendUint16(quint16 number);
    void vbAppendInt8(qint8 number);
    void vbAppendUint8(quint8 number);
    void vbAppendDouble32(double number, double scale);
    void vbAppendDouble16(double number, double scale);
    void vbAppendDouble32Auto(double number);
    void vbAppendString(QString str);

    qint32 vbPopFrontInt32();
    quint32 vbPopFrontUint32();
    qint16 vbPopFrontInt16();
    quint16 vbPopFrontUint16();
    qint8 vbPopFrontInt8();
    quint8 vbPopFrontUint8();
    double vbPopFrontDouble32(double scale);
    double vbPopFrontDouble16(double scale);
    double vbPopFrontDouble32Auto();
    QString vbPopFrontString();

};

#endif // VBYTEARRAY_H
