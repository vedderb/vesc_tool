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

#include "ridesession.h"
#include "../vescinterface.h"
#include "ridebridge.h"
#include "../utility.h"

#include <QDateTime>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <cmath>

namespace {
const qint64 FreshnessMs = 5000;
const int DisconnectGraceMs = 15000;

double epochSeconds()
{
    return QDateTime::currentMSecsSinceEpoch() / 1000.0;
}

double estimateRange(const SETUP_VALUES &v, unsigned int mask)
{
    const unsigned int fields = (1u << 11) | (1u << 12) | (1u << 14) | (1u << 19);
    if ((mask & fields) != fields || !std::isfinite(v.battery_wh) || v.battery_wh <= 0.0 ||
            !std::isfinite(v.battery_level) || v.battery_level < 0.0 || v.battery_level > 1.0 ||
            !std::isfinite(v.tachometer_abs) || v.tachometer_abs < 100.0 ||
            !std::isfinite(v.watt_hours) || v.watt_hours < 0.0 ||
            !std::isfinite(v.watt_hours_charged) || v.watt_hours_charged < 0.0) {
        return -1.0;
    }
    const double netWh = v.watt_hours - v.watt_hours_charged;
    if (!std::isfinite(netWh) || netWh < 1.0) {
        return -1.0;
    }
    // Same trip-average consumption as RtDataSetup, with startup/invalid estimates hidden.
    const double range = v.battery_wh * (v.tachometer_abs / netWh);
    return std::isfinite(range) && range > 0.0 && range < 999000.0 ? range : -1.0;
}
}

RideSession::RideSession(VescInterface *vesc, QObject *parent)
    : QObject(parent), mVesc(vesc)
{
    mClock.start();
    mAutoStart = QSettings().value("rideDisplayOnConnect", true).toBool();
    mTimer.setInterval(1000);
    connect(&mTimer, &QTimer::timeout, this, [this]() { refresh(true); });
    // Short link drops and auto reconnects keep the ride, and an ended ride stays ended
    mEndTimer.setSingleShot(true);
    mEndTimer.setInterval(DisconnectGraceMs);
    connect(&mEndTimer, &QTimer::timeout, this, [this]() {
        mEndedByUser = false;
        mCanMotor = -1;
        finish();
    });
    connect(mVesc->commands(), &Commands::valuesSetupReceived, this, &RideSession::receiveValues);
    connect(mVesc, &VescInterface::portConnectedChanged, this, [this]() {
        if (mVesc->isPortConnected()) {
            mEndTimer.stop();
        } else {
            mEndTimer.start();
        }
        refresh();
    });
    connect(mVesc, &VescInterface::fwRxChanged, this, [this](bool rx) {
        if (rx) {
            if (mActive) {
                checkController();
            }
            startOnConnect();
        }
    });
    connect(mVesc, &VescInterface::useImperialUnitsChanged, this, [this]() {
        publish();
        emit changed();
    });
    connect(mVesc, &QObject::destroyed, this, [this]() { finish(); });
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive) {
            refreshAvailability();
            // Starting can fail while the app is in the background. Not retried while
            // disabled, as returning from the permission prompt also activates the app.
            if (mEnabled) {
                startOnConnect();
            }
        }
    });
    refreshAvailability();
}

RideSession::~RideSession()
{
    if (mActive) {
        vesc_live_activity_end();
    }
}

bool RideSession::supported() const
{
    return vesc_live_activity_supported();
}

bool RideSession::useImperial() const
{
    return mVesc && mVesc->useImperialUnits();
}

bool RideSession::fresh() const
{
    return mActive && mConnected && mHasBaseline && mClock.elapsed() - mSampledMs < FreshnessMs;
}

int RideSession::elapsedSeconds() const
{
    return mActive ? int((mClock.elapsed() - mStartedMs) / 1000) : mElapsedAtEnd;
}

void RideSession::setAutoStart(bool autoStart)
{
    if (autoStart != mAutoStart) {
        mAutoStart = autoStart;
        QSettings().setValue("rideDisplayOnConnect", autoStart);
        emit autoStartChanged();
        startOnConnect();
    }
}

void RideSession::startOnConnect()
{
    if (mAutoStart && !mActive && !mEndedByUser && mVesc->isPortConnected() && mVesc->fwRx() &&
            (mCanMotor >= 0 || mVesc->getLastFwRxParams().hwType == HW_TYPE_VESC)) {
        start();
    }
}

// Without a motor controller on the port, ride data comes from one on the CAN bus
void RideSession::canMotorFound(int canId)
{
    if (mVesc->fwRx() && mVesc->getLastFwRxParams().hwType != HW_TYPE_VESC) {
        mCanMotor = canId;
        startOnConnect();
    }
}

void RideSession::requestValues()
{
    if (mCanMotor >= 0) {
        mVesc->canTmpOverride(true, mCanMotor);
        mVesc->commands()->getValuesSetup();
        mVesc->canTmpOverrideEnd();
    } else {
        mVesc->commands()->getValuesSetup();
    }
}

void RideSession::refreshAvailability()
{
    const bool enabled = vesc_live_activity_enabled();
    if (enabled != mEnabled) {
        mEnabled = enabled;
        emit availabilityChanged();
    }
    refresh(true);
}

bool RideSession::start()
{
    if (mActive) {
        return true;
    }
#ifdef Q_OS_ANDROID
    Utility::requestNotificationPermission();
#endif
    refreshAvailability();
    mError.clear();
    if (!supported()) {
        mError = tr("The ride display requires iOS 18 or later.");
    } else if (!mEnabled) {
#ifdef Q_OS_ANDROID
        mError = tr("Allow notifications for VESC Tool in Android Settings.");
#else
        mError = tr("Enable Live Activities for VESC Tool in iPhone Settings.");
#endif
    } else if (!mVesc || !mVesc->isPortConnected() || !mVesc->fwRx()) {
        mError = tr("Connect to a VESC before starting a ride.");
    }
    if (!mError.isEmpty()) {
        emit changed();
        return false;
    }

    mControllerUuid = mVesc->getConnectedUuid();
    mPortName = mVesc->getConnectedPortName();
    mSendCan = mVesc->commands()->getSendCan();
    mCanId = mVesc->commands()->getCanSendId();
    mActive = mConnected = true;
    mHasBaseline = mDistanceIncomplete = false;
    mSpeed = mDistance = mSampledAt = 0.0;
    mBattery = mRange = -1.0;
    mStartedMs = mClock.elapsed();
    mStartedAt = epochSeconds();
    const QByteArray json = payload();
    if (!vesc_live_activity_start(json.constData())) {
        finish(QString::fromUtf8(vesc_live_activity_last_error()), false);
        return false;
    }
    mLastPayload = json;
    mTimer.start();
    // Keep sampling independently of the selected QML page or logging state.
    requestValues();
    emit changed();
    return true;
}

void RideSession::end()
{
    mEndedByUser = true;
    finish();
}

void RideSession::finish(const QString &error, bool endNative)
{
    if (mActive) {
        mElapsedAtEnd = elapsedSeconds();
        mActive = mConnected = mHasBaseline = false;
        mTimer.stop();
        if (endNative) {
            vesc_live_activity_end();
        }
    }
    mError = error;
    emit changed();
}

bool RideSession::checkController()
{
    const QString uuid = mVesc->getConnectedUuid();
    if (mSendCan != mVesc->commands()->getSendCan() ||
            (mSendCan && mCanId != mVesc->commands()->getCanSendId()) ||
            (!mControllerUuid.isEmpty() && !uuid.isEmpty() && uuid != mControllerUuid) ||
            mPortName != mVesc->getConnectedPortName()) {
        finish(tr("Ride ended because the selected controller changed."));
        return false;
    }
    return true;
}

void RideSession::receiveValues(const SETUP_VALUES &v, unsigned int mask)
{
    if (!mActive || !mVesc || !mVesc->isPortConnected() || !mVesc->fwRx()) {
        return;
    }
    // Selective replies zero-initialize absent fields; don't mistake them for live data.
    const unsigned int required = (1u << 6) | (1u << 8) | (1u << 14);
    if ((mask & required) != required || !checkController() || !std::isfinite(v.speed) ||
            !std::isfinite(v.tachometer_abs) || v.tachometer_abs < 0.0) {
        return;
    }
    const qint64 now = mClock.elapsed();
    if (mSampledAt == 0.0 && now - mStartedMs >= FreshnessMs) {
        mDistanceIncomplete = true;
    }
    if (mHasBaseline) {
        const double delta = v.tachometer_abs - mLastTrip;
        if (now - mSampledMs < FreshnessMs && delta >= 0.0) {
            mDistance += delta;
        } else {
            mDistanceIncomplete = true;
        }
    }
    mSpeed = std::abs(v.speed);
    mBattery = std::isfinite(v.battery_level) && v.battery_level >= 0.0 ? qMin(v.battery_level, 1.0) : -1.0;
    mRange = estimateRange(v, mask);
    mSampledAt = epochSeconds();
    mSampledMs = now;
    mLastTrip = v.tachometer_abs;
    mConnected = mHasBaseline = true;
    // The timer coalesces dashboard/logging samples to at most one update per second.
}

void RideSession::refresh(bool poll)
{
    if (!mActive) {
        return;
    }
    if (!vesc_live_activity_is_active()) {
        mEndedByUser = true;
        finish(tr("The ride display was ended or dismissed."), false);
        return;
    }
    mConnected = mVesc && mVesc->isPortConnected();
    if (!mConnected) {
        mDistanceIncomplete = mDistanceIncomplete || mHasBaseline;
        mHasBaseline = false;
    } else if (mVesc->fwRx()) {
        if (!checkController()) {
            return;
        }
        if (poll) {
            requestValues();
        }
    }
    publish();
    emit changed();
}

QByteArray RideSession::payload() const
{
    QJsonObject object {
        {"speedMps", mSpeed}, {"distanceMeters", mDistance},
        {"distanceIncomplete", mDistanceIncomplete}, {"batteryLevel", mBattery},
        {"estimatedRangeMeters", estimatedRangeMeters()}, {"startedAt", mStartedAt},
        {"sampledAt", mSampledAt}, {"connected", mConnected},
        {"fresh", fresh()}, {"useImperial", useImperial()}
    };
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

void RideSession::publish()
{
    if (mActive) {
        const QByteArray json = payload();
        if (json != mLastPayload) {
            vesc_live_activity_update(json.constData());
            mLastPayload = json;
        }
    }
}
