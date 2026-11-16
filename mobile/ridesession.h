/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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

#ifndef RIDESESSION_H
#define RIDESESSION_H

#include <QObject>
#include <QByteArray>
#include <QElapsedTimer>
#include <QPointer>
#include <QTimer>

class VescInterface;
struct SETUP_VALUES;

class RideSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool supported READ supported CONSTANT)
    Q_PROPERTY(bool enabled READ enabled NOTIFY availabilityChanged)
    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(bool fresh READ fresh NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(double speedMps READ speedMps NOTIFY changed)
    Q_PROPERTY(double distanceMeters READ distanceMeters NOTIFY changed)
    Q_PROPERTY(bool distanceIncomplete READ distanceIncomplete NOTIFY changed)
    Q_PROPERTY(double batteryLevel READ batteryLevel NOTIFY changed)
    Q_PROPERTY(double estimatedRangeMeters READ estimatedRangeMeters NOTIFY changed)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY changed)
    Q_PROPERTY(bool useImperial READ useImperial NOTIFY changed)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)

public:
    explicit RideSession(VescInterface *vesc, QObject *parent = nullptr);
    ~RideSession() override;

    bool supported() const;
    bool enabled() const { return mEnabled; }
    bool active() const { return mActive; }
    bool fresh() const;
    QString error() const { return mError; }
    double speedMps() const { return mSpeed; }
    double distanceMeters() const { return mDistance; }
    bool distanceIncomplete() const { return mDistanceIncomplete; }
    double batteryLevel() const { return mBattery; }
    double estimatedRangeMeters() const { return fresh() ? mRange : -1.0; }
    int elapsedSeconds() const;
    bool useImperial() const;

    bool autoStart() const { return mAutoStart; }
    void setAutoStart(bool autoStart);

    Q_INVOKABLE bool start();
    Q_INVOKABLE void canMotorFound(int canId);
    Q_INVOKABLE void end();
    Q_INVOKABLE void refreshAvailability();

signals:
    void changed();
    void availabilityChanged();
    void autoStartChanged();

private:
    void startOnConnect();
    void requestValues();
    void refresh(bool poll = false);
    void receiveValues(const SETUP_VALUES &values, unsigned int mask);
    void publish();
    void finish(const QString &error = QString(), bool endNative = true);
    bool checkController();
    QByteArray payload() const;

    QPointer<VescInterface> mVesc;
    QElapsedTimer mClock;
    QTimer mTimer;
    QString mError, mControllerUuid, mPortName;
    QByteArray mLastPayload;
    bool mEnabled = false;
    bool mAutoStart = true;
    bool mEndedByUser = false;
    int mCanMotor = -1;
    QTimer mEndTimer;
    bool mActive = false;
    bool mConnected = false;
    bool mHasBaseline = false;
    bool mDistanceIncomplete = false;
    bool mSendCan = false;
    int mCanId = -1;
    int mElapsedAtEnd = 0;
    qint64 mStartedMs = 0;
    qint64 mSampledMs = 0;
    double mSpeed = 0.0;
    double mDistance = 0.0;
    double mBattery = -1.0;
    double mRange = -1.0;
    double mStartedAt = 0.0;
    double mSampledAt = 0.0;
    double mLastTrip = 0.0;
};

#endif // RIDESESSION_H
