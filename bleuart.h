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

#ifndef BLEUART_H
#define BLEUART_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QVariantMap>

class BleUart : public QObject
{
    Q_OBJECT
public:
    explicit BleUart(QObject *parent = nullptr);

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void startConnect(QString addr);
    Q_INVOKABLE void disconnectBle();
    Q_INVOKABLE bool isConnected();
    Q_INVOKABLE bool isConnecting();

signals:
    void dataRx(QByteArray data);
    void scanDone(QVariantMap devs, bool done);
    void bleError(QString info);
    void connected();

public slots:
    void writeData(QByteArray data);

private slots:
    void addDevice(const QBluetoothDeviceInfo&dev);
    void scanFinished();
    void deviceScanError(QBluetoothDeviceDiscoveryAgent::Error e);

    void serviceDiscovered(const QBluetoothUuid &);
    void serviceScanDone();
    void controllerError(QLowEnergyController::Error);
    void deviceConnected();
    void deviceDisconnected();

    void serviceStateChanged(QLowEnergyService::ServiceState s);
    void updateData(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value);

    void controlStateChanged(QLowEnergyController::ControllerState state);
    void connectionUpdated(const QLowEnergyConnectionParameters &newParameters);

private:
    QBluetoothDeviceDiscoveryAgent *mDeviceDiscoveryAgent;
    QLowEnergyController *mControl;
    QLowEnergyService *mService;
    QLowEnergyDescriptor mNotificationDescTx;
    QVariantMap mDevs;
    bool mUartServiceFound;
    bool mConnectDone;
    QString mServiceUuid;
    QString mRxUuid;
    QString mTxUuid;

};

#endif // BLEUART_H
