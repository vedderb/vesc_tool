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

#include "vbytearray.h"
#include <cmath>
#include <stdint.h>

namespace {
inline double roundDouble(double x) {
    return x < 0.0 ? ceil(x - 0.5) : floor(x + 0.5);
}
}

VByteArray::VByteArray()
{

}

VByteArray::VByteArray(const QByteArray &data) : QByteArray(data)
{

}

void VByteArray::vbAppendInt32(qint32 number)
{
    QByteArray data;
    data.append((char)((number >> 24) & 0xFF));
    data.append((char)((number >> 16) & 0xFF));
    data.append((char)((number >> 8) & 0xFF));
    data.append((char)(number & 0xFF));
    append(data);
}

void VByteArray::vbAppendUint32(quint32 number)
{
    QByteArray data;
    data.append((char)((number >> 24) & 0xFF));
    data.append((char)((number >> 16) & 0xFF));
    data.append((char)((number >> 8) & 0xFF));
    data.append((char)(number & 0xFF));
    append(data);
}

void VByteArray::vbAppendInt16(qint16 number)
{
    QByteArray data;
    data.append((char)((number >> 8) & 0xFF));
    data.append((char)(number & 0xFF));
    append(data);
}

void VByteArray::vbAppendUint16(quint16 number)
{
    QByteArray data;
    data.append((char)((number >> 8) & 0xFF));
    data.append((char)(number & 0xFF));
    append(data);
}

void VByteArray::vbAppendInt8(qint8 number)
{
    append((char)number);
}

void VByteArray::vbAppendUint8(quint8 number)
{
    append((char)number);
}

void VByteArray::vbAppendDouble32(double number, double scale)
{
    vbAppendInt32((qint32)roundDouble(number * scale));
}

void VByteArray::vbAppendDouble16(double number, double scale)
{
    vbAppendInt16((qint16)roundDouble(number * scale));
}

void VByteArray::vbAppendDouble32Auto(double number)
{
    int e = 0;
    float fr = frexpf(number, &e);
    float fr_abs = fabsf(fr);
    uint32_t fr_s = 0;

    if (fr_abs >= 0.5) {
        fr_s = (uint32_t)((fr_abs - 0.5f) * 2.0f * 8388608.0f);
        e += 126;
    }

    uint32_t res = ((e & 0xFF) << 23) | (fr_s & 0x7FFFFF);
    if (fr < 0) {
        res |= 1 << 31;
    }

    vbAppendUint32(res);
}

void VByteArray::vbAppendString(QString str)
{
    append(str.toLocal8Bit());
    append((char)0);
}

qint32 VByteArray::vbPopFrontInt32()
{
    if (size() < 4) {
        return 0;
    }

    qint32 res =	((quint8) at(0)) << 24 |
                    ((quint8) at(1)) << 16 |
                    ((quint8) at(2)) << 8 |
                    ((quint8) at(3));

    remove(0, 4);
    return res;
}

quint32 VByteArray::vbPopFrontUint32()
{
    if (size() < 4) {
        return 0;
    }

    quint32 res =	((quint8) at(0)) << 24 |
                    ((quint8) at(1)) << 16 |
                    ((quint8) at(2)) << 8 |
                    ((quint8) at(3));

    remove(0, 4);
    return res;
}

qint16 VByteArray::vbPopFrontInt16()
{
    if (size() < 2) {
        return 0;
    }

    qint16 res =	((quint8) at(0)) << 8 |
                    ((quint8) at(1));

    remove(0, 2);
    return res;
}

quint16 VByteArray::vbPopFrontUint16()
{
    if (size() < 2) {
        return 0;
    }

    quint16 res =	((quint8) at(0)) << 8 |
                    ((quint8) at(1));

    remove(0, 2);
    return res;
}

qint8 VByteArray::vbPopFrontInt8()
{
    if (size() < 1) {
        return 0;
    }

    qint8 res = (quint8)at(0);

    remove(0, 1);
    return res;
}

quint8 VByteArray::vbPopFrontUint8()
{
    if (size() < 1) {
        return 0;
    }

    quint8 res = (quint8)at(0);

    remove(0, 1);
    return res;
}

double VByteArray::vbPopFrontDouble32(double scale)
{
    return (double)vbPopFrontInt32() / scale;
}

double VByteArray::vbPopFrontDouble16(double scale)
{
    return (double)vbPopFrontInt16() / scale;
}

double VByteArray::vbPopFrontDouble32Auto()
{
    uint32_t res = vbPopFrontUint32();

    int e = (res >> 23) & 0xFF;
    int fr = res & 0x7FFFFF;
    bool negative = res & (1 << 31);

    float f = 0.0;
    if (e != 0 || fr != 0) {
        f = (float)fr / (8388608.0 * 2.0) + 0.5;
        e -= 126;
    }

    if (negative) {
        f = -f;
    }

    return ldexpf(f, e);
}

QString VByteArray::vbPopFrontString()
{
    if (size() < 1) {
        return QString();
    }

    QString str(data());
    remove(0, str.size() + 1);
    return str;
}
