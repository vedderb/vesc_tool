/* SPDX-License-Identifier: GPL-3.0-or-later */
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

    Q_INVOKABLE bool start();
    Q_INVOKABLE void end();
    Q_INVOKABLE void refreshAvailability();

signals:
    void changed();
    void availabilityChanged();

private:
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
