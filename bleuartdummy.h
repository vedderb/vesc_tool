/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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

#ifndef BLEUARTDUMMY_H
#define BLEUARTDUMMY_H

#include <QObject>
#include <QVariantMap>

class BleUartDummy : public QObject
{
    Q_OBJECT
public:
    explicit BleUartDummy(QObject *parent = nullptr);

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void emitScanDone();

signals:
    void scanDone(QVariantMap devs, bool done);
    void bleError(QString info);

};

#endif // BLEUARTDUMMY_H
