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

#ifndef ESP32FLASH_H
#define ESP32FLASH_H

#include <QObject>
#include "serial_io.h"

#ifdef HAS_SERIALPORT
#include <QSerialPort>
#include <QSerialPortInfo>
#endif

class Esp32Flash : public QObject
{
    Q_OBJECT
public:
    explicit Esp32Flash(QObject *parent = nullptr);
    ~Esp32Flash();

    Q_INVOKABLE bool connectEsp(QString port);
    Q_INVOKABLE bool disconnectEsp();
    Q_INVOKABLE bool flashFirmware(QByteArray data, quint64 address);
    Q_INVOKABLE bool eraseFlash(quint64 size, quint64 address);
    Q_INVOKABLE QString espPort();
    Q_INVOKABLE bool isBuiltinUsb();
    Q_INVOKABLE bool isEspConnected();
    Q_INVOKABLE target_chip_t getTarget();

signals:
    void flashProgress(double prog);
    void stateUpdate(QString text);

private slots:
#ifdef HAS_SERIALPORT
    void serialPortError(QSerialPort::SerialPortError error);
#endif

};

#endif // ESP32FLASH_H
