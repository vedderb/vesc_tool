/*
    Copyright 2017 Benjamin Vedder	benjamin@vedder.se

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

#include "bleuart.h"
#include "utility.h"

#include <QDebug>
#include <QLowEnergyConnectionParameters>
#include <QSettings>

BleUart::BleUart(QObject *parent) : QObject(parent)
{
    mControl = nullptr;
    mService = nullptr;
    mUartServiceFound = false;
    mConnectDone = false;
    mScanFinished = true;
    mInitDone = false;

    mServiceUuid = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
    mRxUuid = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
    mTxUuid = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
}

BleUart::~BleUart() {
    if (mService) {
        delete mService;
        mService = nullptr;
    }

    if (mControl) {
        delete mControl;
        mControl = nullptr;
    }
}

void BleUart::startScan()
{
    init();

    mDevs.clear();
    mDeviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    mScanFinished = false;
}

void BleUart::startConnect(QString addr)
{
    init();

    disconnectBle();

    mUartServiceFound = false;
    mConnectDone = false;


//#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#if 0
    // Create BT Controller from unique device UUID stored as addr. Creating
    // a controller using a devices address is not supported on macOS or iOS.
    QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo();
    deviceInfo.setDeviceUuid(QBluetoothUuid(addr));
    mControl = QLowEnergyController::createPeripheral(&deviceInfo);

    //mControl = new QLowEnergyController(const_cast<QBluetoothDeviceInfo*>(&deviceInfo));

#else
    //mControl = new QLowEnergyController(QBluetoothAddress(addr));
    mControl = QLowEnergyController::createPeripheral(QBluetoothAddress(addr));

#endif

    mControl->setRemoteAddressType(QLowEnergyController::RandomAddress);

    connect(mControl, SIGNAL(serviceDiscovered(QBluetoothUuid)),
            this, SLOT(serviceDiscovered(QBluetoothUuid)));
    connect(mControl, SIGNAL(discoveryFinished()),
            this, SLOT(serviceScanDone()));
    connect(mControl, SIGNAL(error(QLowEnergyController::Error)),
            this, SLOT(controllerError(QLowEnergyController::Error)));
    connect(mControl, SIGNAL(connected()),
            this, SLOT(deviceConnected()));
    connect(mControl, SIGNAL(disconnected()),
            this, SLOT(deviceDisconnected()));
    connect(mControl, SIGNAL(stateChanged(QLowEnergyController::ControllerState)),
            this, SLOT(controlStateChanged(QLowEnergyController::ControllerState)));
    connect(mControl, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)),
            this, SLOT(connectionUpdated(QLowEnergyConnectionParameters)));

    mControl->connectToDevice();
    mConnectTimeoutTimer.start(10000);
}

void BleUart::disconnectBle()
{
    init();
    if (mService) {
        mService->deleteLater();
        mService = nullptr;
    }

    if (mControl) {
        mControl->deleteLater();
        mControl = nullptr;
    }
}

bool BleUart::isConnected()
{
    return mControl != nullptr && mConnectDone;
}

bool BleUart::isConnecting()
{
    return mControl && !mConnectDone;
}

void BleUart::emitScanDone()
{
    mConnectTimeoutTimer.stop();
    emit scanDone(mDevs, mScanFinished);
}

void BleUart::writeData(QByteArray data)
{
    if (isConnected()) {
        const QLowEnergyCharacteristic  rxChar = mService->characteristic(QBluetoothUuid(QUuid(mRxUuid)));
        if (rxChar.isValid()) {
            int chunk = 20;
            while(data.size() > chunk) {
                mService->writeCharacteristic(rxChar, data.mid(0, chunk),
                                              QLowEnergyService::WriteWithoutResponse);
                data.remove(0, chunk);
            }

            mService->writeCharacteristic(rxChar, data, QLowEnergyService::WriteWithoutResponse);
        }
    }
}

void BleUart::addDevice(const QBluetoothDeviceInfo &dev)
{
    if ((dev.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)) {
        qDebug() << "BLE scan found device:" << dev.name() <<
                    "Valid:" << dev.isValid() <<
                    "Cached:" << dev.isCached() <<
                    "rssi:" << dev.rssi();

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        // macOS and iOS do not expose the hardware address of BLTE devices, must use
        // the OS generated UUID.
        QString addr = dev.deviceUuid().toString();
#else
        QString addr = dev.address().toString();
#endif

        bool preferred = false;

        {
            QSettings set;
            int size = set.beginReadArray("blePreferred");
            for (int i = 0; i < size; ++i) {
                set.setArrayIndex(i);
                QString address = set.value("address").toString();
                bool pref = set.value("preferred").toBool();

                if (addr == address && pref) {
                    preferred = true;
                }
            }
            set.endArray();
        }

#if defined(Q_OS_WIN)
        mDevs.insert(addr, dev.name());
#else
        if(preferred || dev.serviceUuids().contains(QBluetoothUuid(QUuid("6e400001-b5a3-f393-e0a9-e50e24dcca9e")))) {
            mDevs.insert(addr, dev.name());
        }
#endif

        mConnectTimeoutTimer.stop();
        emit scanDone(mDevs, false);
    }
}

void BleUart::scanFinished()
{
    qDebug() << "BLE scan finished";
    mScanFinished = true;
    mConnectTimeoutTimer.stop();
    emit scanDone(mDevs, true);
}

void BleUart::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error e)
{
    qWarning() << "BLE Scan error: " << e;
    mDevs.clear();
    mScanFinished = true;
    emit scanDone(mDevs, true);
    QString errorStr = tr("BLE Scan error: ") + Utility::QEnumToQString(e);

#ifdef Q_OS_ANDROID
    errorStr += ". If you are on Android 10, make sure that both bluetooth and the "
                "location service are activated.";
#endif

    mConnectTimeoutTimer.stop();
    emit bleError(errorStr);
}

void BleUart::serviceDiscovered(const QBluetoothUuid &gatt)
{
    if (gatt==QBluetoothUuid(QUuid(mServiceUuid))){
        qDebug() << "BLE UART service found!";
        mUartServiceFound = true;
    }
}

void BleUart::serviceScanDone()
{
    if (mService) {
        mService->deleteLater();
        mService = nullptr;
    }

    if (mUartServiceFound) {
        qDebug() << "Connecting to BLE UART service";
        mService = mControl->createServiceObject(QBluetoothUuid(QUuid(mServiceUuid)), this);

        connect(mService, SIGNAL(stateChanged(QLowEnergyService::ServiceState)),
                this, SLOT(serviceStateChanged(QLowEnergyService::ServiceState)));
        connect(mService, SIGNAL(error(QLowEnergyService::ServiceError)),
                this, SLOT(serviceError(QLowEnergyService::ServiceError)));
        connect(mService, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)),
                this, SLOT(updateData(QLowEnergyCharacteristic,QByteArray)));
        connect(mService, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)),
                this, SLOT(confirmedDescriptorWrite(QLowEnergyDescriptor,QByteArray)));

        mService->discoverDetails();
    } else {
        qWarning() << "BLE UART service not found";
        disconnectBle();
    }
}

void BleUart::controllerError(QLowEnergyController::Error e)
{
    qWarning() << "BLE error:" << e;
    disconnectBle();
    mConnectTimeoutTimer.stop();
    emit bleError(tr("BLE error: ") + Utility::QEnumToQString(e));
}

void BleUart::deviceConnected()
{
    qDebug() << "BLE device connected";
    mControl->discoverServices();
}

void BleUart::deviceDisconnected()
{
    qDebug() << "BLE service disconnected";
    disconnectBle();
}

void BleUart::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    // A descriptor can only be written if the service is in the ServiceDiscovered state
    switch (s) {
    case QLowEnergyService::InvalidService: {
        //this gets triggered when a connected device is shutoff or goes out of BLE range
        qDebug() << "device disconnected unintentionally";
        emit unintentionalDisconnect();
        break;
    }
    case QLowEnergyService::ServiceDiscovered: {
        //looking for the TX characteristic
        const QLowEnergyCharacteristic txChar = mService->characteristic(
                    QBluetoothUuid(QUuid(mTxUuid)));

        if (!txChar.isValid()){
            qDebug() << "BLE Tx characteristic not found";
            break;
        }

        //looking for the RX characteristic
        const QLowEnergyCharacteristic  rxChar = mService->characteristic(QBluetoothUuid(QUuid(mRxUuid)));

        if (!rxChar.isValid()) {
            qDebug() << "BLE Rx characteristic not found";
            break;
        }

        // Bluetooth LE spec Where a characteristic can be notified, a Client Characteristic Configuration descriptor
        // shall be included in that characteristic as required by the Bluetooth Core Specification
        // Tx notify is enabled
        mNotificationDescTx = txChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        //mNotificationDescTx = txChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);

        if (mNotificationDescTx.isValid()) {
            // enable notification
            mService->writeDescriptor(mNotificationDescTx, QByteArray::fromHex("0100"));
        }

        break;
    }

    default:
        break;
    }
}

void BleUart::serviceError(QLowEnergyService::ServiceError e){
    qDebug() << e;
}

void BleUart::updateData(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() == QBluetoothUuid(QUuid(mTxUuid))) {
        emit dataRx(value);
    }
}

void BleUart::confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value)
{
    if (d.isValid() && d == mNotificationDescTx && value == QByteArray("0000")) {
        //disabled notifications -> assume disconnect intent
        disconnectBle();
    } else {
        mConnectDone = true;
        mConnectTimeoutTimer.stop();
        emit connected();
    }
}

void BleUart::controlStateChanged(QLowEnergyController::ControllerState state)
{
    (void)state;

//    qDebug() << state;

//    if (state == QLowEnergyController::ConnectedState) {
//        QLowEnergyConnectionParameters param;
//        param.setIntervalRange(7.5, 7.5);
//        param.setSupervisionTimeout(10000);
//        param.setLatency(0);
//        mControl->requestConnectionUpdate(param);
//    }
}

void BleUart::connectionUpdated(const QLowEnergyConnectionParameters &newParameters)
{
    (void)newParameters;
    qDebug() << "BLE connection parameters updated";
}

void BleUart::init()
{
    if (mInitDone) {
        return;
    }

    Utility::requestBleScanPermission();
    Utility::requestBleConnectPermission();

    mDeviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    connect(mDeviceDiscoveryAgent, SIGNAL(deviceDiscovered(const QBluetoothDeviceInfo&)),
            this, SLOT(addDevice(const QBluetoothDeviceInfo&)));
    connect(mDeviceDiscoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)),
            this, SLOT(deviceScanError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(mDeviceDiscoveryAgent, SIGNAL(finished()), this, SLOT(scanFinished()));

    mInitDone = true;

    mConnectTimeoutTimer.setSingleShot(true);
    connect(&mConnectTimeoutTimer, &QTimer::timeout, [this]() {
        disconnectBle();
        qDebug() << "BLE connect timeout";
        emit bleError(tr("BLE connect timed out."));
    });
}
