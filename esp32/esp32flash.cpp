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

#include <QDebug>
#include <QTime>
#include "esp32flash.h"
#include "utility.h"

#ifdef HAS_SERIALPORT
static QSerialPort *sPort = nullptr;
#endif
static int64_t sTimeEnd;

Esp32Flash::Esp32Flash(QObject *parent) : QObject(parent)
{
#ifdef HAS_SERIALPORT
    if (sPort) {
        sPort->deleteLater();
        sPort = nullptr;
    }

    sPort = new QSerialPort();

    connect(sPort, SIGNAL(error(QSerialPort::SerialPortError)),
            this, SLOT(serialPortError(QSerialPort::SerialPortError)));
#endif
}

Esp32Flash::~Esp32Flash()
{
#ifdef HAS_SERIALPORT
    if (sPort) {
        sPort->deleteLater();
        sPort = nullptr;
    }
#endif
}

bool Esp32Flash::connectEsp(QString port)
{
#ifdef HAS_SERIALPORT
    sPort->setPortName(port);
    sPort->open(QIODevice::ReadWrite);

    if(!sPort->isOpen()) {
        emit stateUpdate("Cannot open port");
        return false;
    }

    sPort->setBaudRate(115200);
    sPort->setDataBits(QSerialPort::Data8);
    sPort->setParity(QSerialPort::NoParity);
    sPort->setStopBits(QSerialPort::OneStop);
    sPort->setFlowControl(QSerialPort::NoFlowControl);

    esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();
    esp_loader_error_t err = esp_loader_connect(&connect_config);
    if (err != ESP_LOADER_SUCCESS) {
        emit stateUpdate(QString("Cannot connect to target. Error: %1").arg(err));
        sPort->close();
        return false;
    }

    QString targetName = "Unknown";
    switch (esp_loader_get_target()) {
        case ESP8266_CHIP: targetName = "ESP8266"; break;
        case ESP32_CHIP: targetName = "ESP32"; break;
        case ESP32S2_CHIP: targetName = "ESP32S2"; break;
        case ESP32C3_CHIP: targetName = "ESP32C3"; break;
        case ESP32S3_CHIP: targetName = "ESP32S3"; break;
        case ESP32C2_CHIP: targetName = "ESP32C2"; break;
        case ESP32H2_CHIP: targetName = "ESP32H2"; break;
        case ESP_MAX_CHIP: targetName = "ESP_MAX"; break;
    }

    emit stateUpdate(QString("Connected to %1").arg(targetName));

    qDebug() << "Changing baudrate to 460800";
    if (esp_loader_change_baudrate(460800) == ESP_LOADER_SUCCESS) {
        loader_port_change_baudrate(460800);
        qDebug() << "Baudrate changed!";
    }

    return true;
#else
    (void)port;
    return false;
#endif
}

bool Esp32Flash::disconnectEsp()
{
#ifdef HAS_SERIALPORT
    esp_loader_reset_target();
    sPort->close();
    emit stateUpdate("Disconnected");
#endif
    return true;
}

bool Esp32Flash::flashFirmware(QByteArray data, quint64 address)
{
#ifdef HAS_SERIALPORT
    if (!sPort->isOpen()) {
        emit stateUpdate("Not connected");
        return false;
    }

    size_t size = data.size();

    esp_loader_error_t err;
    static uint8_t payload[1024];
    const uint8_t *bin_addr = (uint8_t*)data.data();

    emit stateUpdate("Erasing flash (this may take a while)...");
    Utility::sleepWithEventLoop(500);

    err = esp_loader_flash_start(address, size, sizeof(payload));
    if (err != ESP_LOADER_SUCCESS) {
        QString errStr = QString("Erasing flash failed with error: %1").arg(err);
        emit stateUpdate(errStr);
        qWarning() << errStr;
        return false;
    }

    emit stateUpdate("Programming...");
    Utility::sleepWithEventLoop(500);

    size_t binary_size = size;
    size_t written = 0;

    while (size > 0) {
        size_t to_read = std::min(size, sizeof(payload));
        memcpy(payload, bin_addr, to_read);

        err = esp_loader_flash_write(payload, to_read);
        if (err != ESP_LOADER_SUCCESS) {
            QString errStr = QString("Packet could not be written! Code: %1").arg(err);
            emit stateUpdate(errStr);
            qWarning() << errStr;
            return false;
        }

        size -= to_read;
        bin_addr += to_read;
        written += to_read;

        emit flashProgress(double(written) / double(binary_size));
    };

    emit stateUpdate("Done, verifying flash...");
    Utility::sleepWithEventLoop(500);

    err = esp_loader_flash_verify();
    if (err != ESP_LOADER_SUCCESS) {
        QString errStr = QString("MD5 error. Code: %1").arg(err);

        if (err == ESP_LOADER_ERROR_TIMEOUT) {
            errStr = "MD5 Timeout, but probably OK";
        }

        emit stateUpdate(errStr);
        qWarning() << errStr;
        return false;
    }
    emit stateUpdate("Flash verified");

    return true;
#else
    return false;
#endif
}

bool Esp32Flash::eraseFlash(quint64 size, quint64 address)
{
#ifdef HAS_SERIALPORT
    esp_loader_error_t err = esp_loader_flash_start(address, size, 1024);
    if (err != ESP_LOADER_SUCCESS) {
        emit stateUpdate(QString("Erasing flash failed with error: %1").arg(err));
        return false;
    }

    return true;
#else
    (void)size; (void)address;
    return false;
#endif
}

QString Esp32Flash::espPort()
{
#ifdef HAS_SERIALPORT
    return sPort->portName();
#else
    return "";
#endif
}

bool Esp32Flash::isBuiltinUsb()
{
#ifdef HAS_SERIALPORT
    auto port = QSerialPortInfo(sPort->portName());
    return port.productIdentifier() == 0x1001;
#else
    return false;
#endif
}

bool Esp32Flash::isEspConnected()
{
#ifdef HAS_SERIALPORT
    return sPort->isOpen();
#else
    return false;
#endif
}

target_chip_t Esp32Flash::getTarget()
{
#ifdef HAS_SERIALPORT
    if (sPort->isOpen()) {
        return esp_loader_get_target();
    } else {
        return ESP_UNKNOWN_CHIP;
    }
#else
    return ESP_UNKNOWN_CHIP;
#endif
}

#ifdef HAS_SERIALPORT
void Esp32Flash::serialPortError(QSerialPort::SerialPortError error)
{
    QString message;
    switch (error) {
    case QSerialPort::NoError:
        break;

    default:
        message = "Serial port error: " + sPort->errorString();
        break;
    }

    if (!message.isEmpty()) {
        if (sPort->isOpen()) {
            sPort->close();
        }

        emit stateUpdate(message);
    }
}
#endif

esp_loader_error_t loader_port_serial_write(const uint8_t *data, uint16_t size, uint32_t timeout)
{
#ifdef HAS_SERIALPORT
    if (!sPort->isOpen()) {
        return ESP_LOADER_ERROR_FAIL;
    }

    sPort->write((char*)data, size);

    qint64 written = 0;
    auto conn = QObject::connect(sPort, &QSerialPort::bytesWritten, [&written](qint64 bytes) {
        written += bytes;
    });

    bool ok = true;
    while (written < size && ok) {
        ok = Utility::waitSignal(sPort, SIGNAL(bytesWritten(qint64)), timeout);
    }

    QObject::disconnect(conn);

    return ok ? ESP_LOADER_SUCCESS : ESP_LOADER_ERROR_TIMEOUT;
#else
    (void)data;(void)size;(void)timeout;
    return ESP_LOADER_ERROR_FAIL;
#endif
}


esp_loader_error_t loader_port_serial_read(uint8_t *data, uint16_t size, uint32_t timeout)
{
#ifdef HAS_SERIALPORT
    if (!sPort->isOpen()) {
        return ESP_LOADER_ERROR_FAIL;
    }

    QByteArray res;
    while (size > 0 && timeout > 0) {
        auto chunk = sPort->read(size);
        size -= chunk.size();
        res.append(chunk);
        if (size > 0) {
            Utility::sleepWithEventLoop(1);
            timeout--;
        }
    }

    for (int i = 0; i < res.size();i++) {
        data[i] = uint8_t(res.at(i));
    }

    return timeout > 0 ? ESP_LOADER_SUCCESS : ESP_LOADER_ERROR_TIMEOUT;
#else
    (void)data;(void)size;(void)timeout;
    return ESP_LOADER_ERROR_FAIL;
#endif
}

#ifdef HAS_SERIALPORT
static void setDtr(bool state) {
    sPort->setDataTerminalReady(state);
}

static void setRts(bool state) {
    sPort->setRequestToSend(state);
    sPort->setDataTerminalReady(sPort->isDataTerminalReady());
}
#endif

void loader_port_enter_bootloader(void)
{
#ifdef HAS_SERIALPORT
    if (!sPort->isOpen()) {
        return;
    }

    auto port = QSerialPortInfo(sPort->portName());

    bool isBuiltin = false;
    if (port.hasProductIdentifier()) {
        isBuiltin = port.productIdentifier() == 0x1001;
    } else {
        isBuiltin = port.manufacturer().startsWith("Espressif", Qt::CaseInsensitive);
    }

    if (isBuiltin) {
        // The built in USB requires a different sequence
        qDebug() << "Using builtin USB-serial";
        setRts(false);
        setDtr(false);
        Utility::sleepWithEventLoop(100);
        setDtr(true);
        setRts(false);
        Utility::sleepWithEventLoop(100);
        setRts(true);
        setDtr(false);
        setRts(true);
        Utility::sleepWithEventLoop(100);
        setDtr(false);
        setRts(false);
    } else {
        qDebug() << "Using external USB-serial";
        setDtr(false);
        setRts(true);
        Utility::sleepWithEventLoop(100);
        setDtr(true);
        setRts(false);
        Utility::sleepWithEventLoop(100);
        setDtr(false);
    }
#endif
}

void loader_port_reset_target(void)
{
#ifdef HAS_SERIALPORT
    if (!sPort->isOpen()) {
        return;
    }

    setDtr(false);
    setRts(false);
    setRts(true);
    Utility::sleepWithEventLoop(100);
    setRts(false);
#endif
}


void loader_port_delay_ms(uint32_t ms)
{
    Utility::sleepWithEventLoop(ms);
}


void loader_port_start_timer(uint32_t ms)
{
    sTimeEnd = QTime::currentTime().msecsSinceStartOfDay() + ms;
}


uint32_t loader_port_remaining_time(void)
{
    int64_t remaining = sTimeEnd - QTime::currentTime().msecsSinceStartOfDay();
    return (remaining > 0) ? uint32_t(remaining) : 0;
}


void loader_port_debug_print(const char *str)
{
    qDebug() << str;
}

esp_loader_error_t loader_port_change_baudrate(uint32_t baudrate)
{
#ifdef HAS_SERIALPORT
    if (!sPort->isOpen()) {
        return ESP_LOADER_ERROR_FAIL;
    }
    sPort->setBaudRate(baudrate);
    return ESP_LOADER_SUCCESS;
#else
    (void)baudrate;
    return ESP_LOADER_ERROR_FAIL;
#endif
}
