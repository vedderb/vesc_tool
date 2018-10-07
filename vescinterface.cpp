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

#include "vescinterface.h"
#include <QDebug>
#include <QHostInfo>
#include <QFileInfo>
#include <QThread>
#include <QEventLoop>
#include <utility.h>

#ifdef HAS_SERIALPORT
#include <QSerialPortInfo>
#endif

VescInterface::VescInterface(QObject *parent) : QObject(parent)
{
    mMcConfig = new ConfigParams(this);
    mAppConfig = new ConfigParams(this);
    mInfoConfig = new ConfigParams(this);
    mPacket = new Packet(this);
    mCommands = new Commands(this);

    // Compatible firmwares
    mFwVersionReceived = false;
    mFwRetries = 0;
    mFwPollCnt = 0;
    mFwTxt = "x.x";
    mIsUploadingFw = false;

    mTimer = new QTimer(this);
    mTimer->setInterval(20);
    mTimer->start();

    mLastConnType = CONN_NONE;
    mSendCanBefore = false;
    mCanIdBefore = 0;
    mWasConnected = false;
    mAutoconnectOngoing = false;
    mAutoconnectProgress = 0.0;

    // Serial
#ifdef HAS_SERIALPORT
    mSerialPort = new QSerialPort(this);
    mLastSerialPort = "";
    mLastSerialBaud = 0;

    connect(mSerialPort, SIGNAL(readyRead()),
            this, SLOT(serialDataAvailable()));
    connect(mSerialPort, SIGNAL(error(QSerialPort::SerialPortError)),
            this, SLOT(serialPortError(QSerialPort::SerialPortError)));
#endif

    // TCP
    mTcpSocket = new QTcpSocket(this);
    mTcpConnected = false;
    mLastTcpServer = "";
    mLastTcpPort = 0;

    connect(mTcpSocket, SIGNAL(readyRead()), this, SLOT(tcpInputDataAvailable()));
    connect(mTcpSocket, SIGNAL(connected()), this, SLOT(tcpInputConnected()));
    connect(mTcpSocket, SIGNAL(disconnected()),
            this, SLOT(tcpInputDisconnected()));
    connect(mTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(tcpInputError(QAbstractSocket::SocketError)));

    // BLE
#ifdef HAS_BLUETOOTH
    mBleUart = new BleUart(this);

    int size = mSettings.beginReadArray("bleNames");
    for (int i = 0; i < size; ++i) {
        mSettings.setArrayIndex(i);
        QString address = mSettings.value("address").toString();
        QString name = mSettings.value("name").toString();
        mBleNames.insert(address, name);
    }
    mSettings.endArray();

    connect(mBleUart, SIGNAL(dataRx(QByteArray)), this, SLOT(bleDataRx(QByteArray)));
#endif

    mCommands->setAppConfig(mAppConfig);
    mCommands->setMcConfig(mMcConfig);

    // Other signals/slots
    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    connect(mPacket, SIGNAL(dataToSend(QByteArray&)),
            this, SLOT(packetDataToSend(QByteArray&)));
    connect(mPacket, SIGNAL(packetReceived(QByteArray&)),
            this, SLOT(packetReceived(QByteArray&)));
    connect(mCommands, SIGNAL(dataToSend(QByteArray&)),
            this, SLOT(cmdDataToSend(QByteArray&)));
    connect(mCommands, SIGNAL(fwVersionReceived(int,int,QString,QByteArray)),
            this, SLOT(fwVersionReceived(int,int,QString,QByteArray)));
    connect(mCommands, SIGNAL(ackReceived(QString)), this, SLOT(ackReceived(QString)));
    connect(mMcConfig, SIGNAL(updated()), this, SLOT(mcconfUpdated()));
    connect(mAppConfig, SIGNAL(updated()), this, SLOT(appconfUpdated()));
}

VescInterface::~VescInterface()
{
    mSettings.beginWriteArray("bleNames");

    QHashIterator<QString, QString> i(mBleNames);
    int ind = 0;
    while (i.hasNext()) {
        i.next();
        mSettings.setArrayIndex(ind);
        mSettings.setValue("address", i.key());
        mSettings.setValue("name", i.value());
        ind++;
    }

    mSettings.endArray();
}

Commands *VescInterface::commands() const
{
    return mCommands;
}

ConfigParams *VescInterface::mcConfig()
{
    return mMcConfig;
}

ConfigParams *VescInterface::appConfig()
{
    return mAppConfig;
}

ConfigParams *VescInterface::infoConfig()
{
    return mInfoConfig;
}

QStringList VescInterface::getSupportedFirmwares()
{
    QList<QPair<int, int> > fwPairs = getSupportedFirmwarePairs();
    QStringList fws;

    for (int i = 0;i < fwPairs.size();i++) {
        QString tmp;
        tmp.sprintf("%d.%d", fwPairs.at(i).first, fwPairs.at(i).second);
        fws.append(tmp);
    }
    return fws;
}

QList<QPair<int, int> > VescInterface::getSupportedFirmwarePairs()
{
    QList<QPair<int, int> > fws;

    ConfigParam *p = mInfoConfig->getParam("fw_version");

    if (p) {
        QStringList strs = p->enumNames;

        for (int i = 0;i < strs.size();i++) {
            QStringList mami = strs.at(i).split(".");
            QPair<int, int> fw = qMakePair(mami.at(0).toInt(), mami.at(1).toInt());
            fws.append(fw);
        }
    }

    return fws;
}

QString VescInterface::getFirmwareNow()
{
    return mFwTxt;
}

void VescInterface::emitStatusMessage(const QString &msg, bool isGood)
{
    emit statusMessage(msg, isGood);
}

void VescInterface::emitMessageDialog(const QString &title, const QString &msg, bool isGood, bool richText)
{
    emit messageDialog(title, msg, isGood, richText);
}

bool VescInterface::fwRx()
{
    return mFwVersionReceived;
}

#ifdef HAS_BLUETOOTH
BleUart *VescInterface::bleDevice()
{
    return mBleUart;
}

void VescInterface::storeBleName(QString address, QString name)
{
    mBleNames.insert(address, name);
}

QString VescInterface::getBleName(QString address)
{
    QString res;
    if(mBleNames.contains(address)) {
        res = mBleNames[address];
    }
    return res;
}
#endif

bool VescInterface::isPortConnected()
{
    bool res = false;

#ifdef HAS_SERIALPORT
    if (mSerialPort->isOpen()) {
        res = true;
    }
#endif

    if (mTcpConnected) {
        res = true;
    }

#ifdef HAS_BLUETOOTH
    if (mBleUart->isConnected()) {
        res = true;
    }
#endif

    return res;
}

void VescInterface::disconnectPort()
{
#ifdef HAS_SERIALPORT
    if(mSerialPort->isOpen()) {
        mSerialPort->close();
        updateFwRx(false);
    }
#endif

    if (mTcpConnected) {
        mTcpSocket->close();
        updateFwRx(false);
    }

#ifdef HAS_BLUETOOTH
    if (mBleUart->isConnected()) {
        mBleUart->disconnectBle();
        updateFwRx(false);
    }
#endif

    mFwRetries = 0;
}

bool VescInterface::reconnectLastPort()
{
    if (mLastConnType == CONN_SERIAL) {
#ifdef HAS_SERIALPORT
        return connectSerial(mLastSerialPort, mLastSerialBaud);
#else
        return false;
#endif
    } else if (mLastConnType == CONN_TCP) {
        connectTcp(mLastTcpServer, mLastTcpPort);
        return true;
    } else if (mLastConnType == CONN_BLE) {
#ifdef HAS_BLUETOOTH
        mBleUart->startConnect(mLastBleAddr);
#endif
        return true;
    } else {
#ifdef HAS_SERIALPORT
        QList<VSerialInfo_t> ports = listSerialPorts();
        if (!ports.isEmpty()) {
            return connectSerial(ports.first().systemPath);
        } else  {
            emit messageDialog(tr("Reconnect"), tr("No ports found"), false, false);
            return false;
        }
#else
        emit messageDialog(tr("Reconnect"),
                           tr("Please specify the connection manually "
                              "the first time you are connecting."),
                           false, false);
        return false;
#endif
    }
}

bool VescInterface::autoconnect()
{
    bool res = false;

#ifdef HAS_SERIALPORT
    QList<VSerialInfo_t> ports = listSerialPorts();
    mAutoconnectOngoing = true;
    mAutoconnectProgress = 0.0;

    disconnectPort();
    disconnect(mCommands, SIGNAL(fwVersionReceived(int,int,QString,QByteArray)),
               this, SLOT(fwVersionReceived(int,int,QString,QByteArray)));

    for (int i = 0;i < ports.size();i++) {
        VSerialInfo_t serial = ports[i];

        if (!connectSerial(serial.systemPath)) {
            continue;
        }

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.start(500);
        connect(mCommands, SIGNAL(fwVersionReceived(int,int,QString,QByteArray)), &loop, SLOT(quit()));
        connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        loop.exec();

        if (timeoutTimer.isActive()) {
            // If the timer is still running a firmware version was received.
            res = true;
            break;
        } else {
            mAutoconnectProgress = (double)i / (double)ports.size();
            emit autoConnectProgressUpdated(mAutoconnectProgress, false);
            disconnectPort();
        }
    }

    connect(mCommands, SIGNAL(fwVersionReceived(int,int,QString,QByteArray)),
            this, SLOT(fwVersionReceived(int,int,QString,QByteArray)));
#endif

    emit autoConnectProgressUpdated(1.0, true);
    emit autoConnectFinished();
    mAutoconnectOngoing = false;
    return res;
}

QString VescInterface::getConnectedPortName()
{
    QString res = tr("Not connected");
    bool connected = false;

#ifdef HAS_SERIALPORT
    if (mSerialPort->isOpen()) {
        res = tr("Connected (serial) to %1").arg(mSerialPort->portName());
        connected = true;
    }
#endif

    if (mTcpConnected) {
        res = tr("Connected (TCP) to %1:%2").arg(mLastTcpServer).arg(mLastTcpPort);
        connected = true;
    }

#ifdef HAS_BLUETOOTH
    if (mBleUart->isConnected()) {
        res = tr("Connected (BLE) to %1").arg(mLastBleAddr);
        connected = true;
    }
#endif

    if (connected && mCommands->isLimitedMode()) {
        res += tr(", limited mode");
    }

    return res;
}

bool VescInterface::connectSerial(QString port, int baudrate)
{
#ifdef HAS_SERIALPORT
    mLastSerialPort = port;
    mLastSerialBaud = baudrate;
    mLastConnType = CONN_SERIAL;

    bool found = false;
    for (VSerialInfo_t ser: listSerialPorts()) {
        if (ser.systemPath == port) {
            found = true;
            break;
        }
    }

    if (!found) {
        emit statusMessage(tr("Invalid serial port: %1").arg(port), false);
        return false;
    }

    if(mSerialPort->isOpen()) {
        return true;
    }

    // TODO: Maybe this test works on other OSes as well
#ifdef Q_OS_UNIX
    QFileInfo fi(port);
    if (fi.exists()) {
        if (!fi.isWritable()) {
            emit statusMessage(tr("Serial port is not writable"), false);
            emit serialPortNotWritable(port);
            return false;
        }
    }
#endif

    mSerialPort->setPortName(port);
    mSerialPort->open(QIODevice::ReadWrite);

    if(!mSerialPort->isOpen()) {
        return false;
    }

    mSerialPort->setBaudRate(baudrate);
    mSerialPort->setDataBits(QSerialPort::Data8);
    mSerialPort->setParity(QSerialPort::NoParity);
    mSerialPort->setStopBits(QSerialPort::OneStop);
    mSerialPort->setFlowControl(QSerialPort::NoFlowControl);

    // For nrf
    mSerialPort->setRequestToSend(true);
    mSerialPort->setDataTerminalReady(true);
    QThread::msleep(5);
    mSerialPort->setDataTerminalReady(false);
    QThread::msleep(100);

    return true;
#else
    (void)port;
    (void)baudrate;
    emit messageDialog(tr("Connect serial"),
                       tr("Serial port support is not enabled in this build "
                          "of VESC Tool."),
                       false, false);
    return false;
#endif
}

QList<VSerialInfo_t> VescInterface::listSerialPorts()
{
    QList<VSerialInfo_t> res;

#ifdef HAS_SERIALPORT
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    foreach(const QSerialPortInfo &port, ports) {
        VSerialInfo_t info;
        info.name = port.portName();
        info.systemPath = port.systemLocation();
        int index = res.size();

        if(port.manufacturer().startsWith("STMicroelectronics")) {
            info.name.insert(0, "VESC - ");
            info.isVesc = true;
            index = 0;
        } else {
            info.isVesc = false;
        }

        res.insert(index, info);
    }
#endif

    return res;
}

void VescInterface::connectTcp(QString server, int port)
{
    mLastTcpServer = server;
    mLastTcpPort = port;
    mLastConnType = CONN_TCP;

    QHostAddress host;
    host.setAddress(server);

    // Try DNS lookup
    if (host.isNull()) {
        QList<QHostAddress> addresses = QHostInfo::fromName(server).addresses();

        if (!addresses.isEmpty()) {
            host.setAddress(addresses.first().toString());
        }
    }

    mTcpSocket->abort();
    mTcpSocket->connectToHost(host, port);
}

void VescInterface::connectBle(QString address)
{
#ifdef HAS_BLUETOOTH
    mBleUart->startConnect(address);
    mLastConnType = CONN_BLE;
    mLastBleAddr = address;
#else
    (void)address;
#endif
}

bool VescInterface::isAutoconnectOngoing() const
{
    return mAutoconnectOngoing;
}

double VescInterface::getAutoconnectProgress() const
{
    return mAutoconnectProgress;
}

#ifdef HAS_SERIALPORT
void VescInterface::serialDataAvailable()
{
    while (mSerialPort->bytesAvailable() > 0) {
        mPacket->processData(mSerialPort->readAll());
    }
}

void VescInterface::serialPortError(QSerialPort::SerialPortError error)
{
    QString message;
    switch (error) {
    case QSerialPort::NoError:
        break;

    default:
        message = "Serial port error: " + mSerialPort->errorString();
        break;
    }

    if(!message.isEmpty()) {
        emit statusMessage(message, false);

        if (mSerialPort->isOpen()) {
            mSerialPort->close();
        }

       updateFwRx(false);
    }
}
#endif

void VescInterface::tcpInputConnected()
{
    mTcpConnected = true;
    updateFwRx(false);
}

void VescInterface::tcpInputDisconnected()
{
    mTcpConnected = false;
    updateFwRx(false);
}

void VescInterface::tcpInputDataAvailable()
{
    while (mTcpSocket->bytesAvailable() > 0) {
        mPacket->processData(mTcpSocket->readAll());
    }
}

void VescInterface::tcpInputError(QAbstractSocket::SocketError socketError)
{
    (void)socketError;

    QString errorStr = mTcpSocket->errorString();
    emit statusMessage(tr("TCP Error") + errorStr, false);
    mTcpSocket->close();
    updateFwRx(false);
}

#ifdef HAS_BLUETOOTH
void VescInterface::bleDataRx(QByteArray data)
{
    mPacket->processData(data);
}
#endif

void VescInterface::timerSlot()
{
    // Poll the serial port as well since readyRead is not emitted recursively. This
    // can be a problem when waiting for input with an additional event loop, such as
    // when using QMessageBox.
#ifdef HAS_SERIALPORT
    serialDataAvailable();
#endif

    if (isPortConnected()) {
        if (mSendCanBefore != mCommands->getSendCan() ||
                mCanIdBefore != mCommands->getCanSendId()) {
            updateFwRx(false);
            mFwRetries = 0;
        }

        mFwPollCnt++;
        if (mFwPollCnt >= 4) {
            mFwPollCnt = 0;
            if (!mFwVersionReceived) {
                mCommands->getFwVersion();
                mFwRetries++;

                // Timeout if the firmware cannot be read
                if (mFwRetries >= 25) {
                    emit statusMessage(tr("No firmware read response"), false);
                    emit messageDialog(tr("Read Firmware Version"),
                                       tr("Could not read firmware version. Make sure "
                                          "that selected port really belongs to the VESC. "),
                                       false, false);
                    disconnectPort();
                }
            }
        }
    } else {
        updateFwRx(false);
        mFwRetries = 0;
    }
    mSendCanBefore = mCommands->getSendCan();
    mCanIdBefore = mCommands->getCanSendId();

    // Update fw upload bar and label
    double fwProg = mCommands->getFirmwareUploadProgress();
    QString fwStatus = mCommands->getFirmwareUploadStatus();
    if (fwProg > -0.1) {
        mIsUploadingFw = true;
        emit fwUploadStatus(fwStatus, fwProg, true);
    } else {
        // If the firmware upload just finished or failed
        if (mIsUploadingFw) {
            updateFwRx(false);
            mFwRetries = 0;
            if (fwStatus.compare("FW Upload Done") == 0) {
                emit fwUploadStatus(fwStatus, 1.0, false);
            } else {
                emit fwUploadStatus(fwStatus, 0.0, false);
            }
        }
        mIsUploadingFw = false;
    }

    if (mWasConnected != isPortConnected()) {
        mWasConnected = isPortConnected();
        emit portConnectedChanged();
    }
}

void VescInterface::packetDataToSend(QByteArray &data)
{
#ifdef HAS_SERIALPORT
    if (mSerialPort->isOpen()) {
        mSerialPort->write(data);
    }
#endif

    if (mTcpConnected && mTcpSocket->isOpen()) {
        mTcpSocket->write(data);
    }

#ifdef HAS_BLUETOOTH
    if (mBleUart->isConnected()) {
        mBleUart->writeData(data);
    }
#endif
}

void VescInterface::packetReceived(QByteArray &data)
{
    mCommands->processPacket(data);
}

void VescInterface::cmdDataToSend(QByteArray &data)
{
    mPacket->sendPacket(data);
}

void VescInterface::fwVersionReceived(int major, int minor, QString hw, QByteArray uuid)
{
    QList<QPair<int, int> > fwPairs = getSupportedFirmwarePairs();

    QString strUuid = Utility::uuid2Str(uuid, true);

    if (fwPairs.isEmpty()) {
        emit messageDialog(tr("No Supported Firmwares"),
                           tr("This version of VESC Tool does not seem to have any supported "
                              "firmwares. Something is probably wrong with the motor configuration "
                              "file."),
                           false, false);
        updateFwRx(false);
        mFwRetries = 0;
        disconnectPort();
        return;
    }

    QPair<int, int> highest_supported = *std::max_element(fwPairs.begin(), fwPairs.end());
    QPair<int, int> fw_connected = qMakePair(major, minor);

    bool wasReceived = mFwVersionReceived;
    mCommands->setLimitedMode(false);

    if (major < 0) {
        updateFwRx(false);
        mFwRetries = 0;
        disconnectPort();
        emit messageDialog(tr("Error"), tr("The firmware on the connected VESC is too old. Please"
                                           " update it using a programmer."), false, false);
    } else if (fw_connected > highest_supported) {
        mCommands->setLimitedMode(true);
        updateFwRx(true);
        if (!wasReceived) {
            emit messageDialog(tr("Warning"), tr("The connected VESC has newer firmware than this version of"
                                                " VESC Tool supports. It is recommended that you update VESC "
                                                " Tool to the latest version. Alternatively, the firmware on"
                                                " the connected VESC can be downgraded in the firmware page."
                                                " Until then, limited communication mode will be used where"
                                                " only the firmware can be changed."), false, false);
        }
    } else if (!fwPairs.contains(fw_connected)) {
        if (fw_connected >= qMakePair(1, 1)) {
            mCommands->setLimitedMode(true);
            updateFwRx(true);
            if (!wasReceived) {
                emit messageDialog(tr("Warning"), tr("The connected VESC has too old firmware. Since the"
                                                    " connected VESC has firmware with bootloader support, it can be"
                                                    " updated from the Firmware page."
                                                    " Until then, limited communication mode will be used where only the"
                                                    " firmware can be changed."), false, false);
            }
        } else {
            updateFwRx(false);
            mFwRetries = 0;
            disconnectPort();
            if (!wasReceived) {
                emit messageDialog(tr("Error"), tr("The firmware on the connected VESC is too old. Please"
                                                   " update it using a programmer."), false, false);
            }
        }
    } else {
        updateFwRx(true);
        if (fw_connected < highest_supported) {
            if (!wasReceived) {
                emit messageDialog(tr("Warning"), tr("The connected VESC has compatible, but old"
                                                    " firmware. It is recommended that you update it."), false, false);
            }
        }

        QString fwStr;
        fwStr.sprintf("VESC Firmware Version %d.%d", major, minor);
        if (!hw.isEmpty()) {
            fwStr += ", Hardware: " + hw;
        }

        if (!strUuid.isEmpty()) {
            fwStr += ", UUID: " + strUuid;
        }

        emit statusMessage(fwStr, true);
    }

    if (major >= 0) {
        mFwTxt.sprintf("Fw: %d.%d", major, minor);
        mHwTxt = hw;
        if (!hw.isEmpty()) {
            mFwTxt += ", Hw: " + hw;
        }

        if (!strUuid.isEmpty()) {
            mFwTxt += "\n" + strUuid;
        }
    }
}

void VescInterface::appconfUpdated()
{
    emit statusMessage(tr("App configuration updated"), true);
}

void VescInterface::mcconfUpdated()
{
    emit statusMessage(tr("MC configuration updated"), true);
}

void VescInterface::ackReceived(QString ackType)
{
    emit statusMessage(ackType, true);
}

void VescInterface::updateFwRx(bool fwRx)
{
    bool change = mFwVersionReceived != fwRx;
    mFwVersionReceived = fwRx;
    if (change) {
        emit fwRxChanged(mFwVersionReceived, mCommands->isLimitedMode());
    }
}
