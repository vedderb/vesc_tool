/*
    Copyright 2026 Marvin Holzwarth

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
    */

#include "mcpwritetransaction.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QUuid>
#include <cmath>

McpWriteTransaction::McpWriteTransaction(QObject *parent, int operationTimeoutMs,
                                         int approvalTimeoutMs)
    : QObject(parent), mOperationTimeoutMs(qBound(1, operationTimeoutMs, 300000)),
      mApprovalTimeoutMs(qBound(1, approvalTimeoutMs, 300000))
{
    mDeadline.setParent(this);
    mDeadline.setSingleShot(true);
    connect(&mDeadline, &QTimer::timeout, this, [this]() {
        if (active()) {
            fail("Transaction deadline expired in " + stateName(mState) + ".");
        }
    });
}

bool McpWriteTransaction::validValues(const QJsonObject &values)
{
    if (values.isEmpty() || values.size() > 1024) {
        return false;
    }
    for (auto it = values.begin(); it != values.end(); ++it) {
        const auto value = it.value();
        if (it.key().isEmpty() || (!value.isBool() && !value.isString() && !value.isDouble()) ||
            (value.isDouble() && !std::isfinite(value.toDouble()))) {
            return false;
        }
    }
    return true;
}

QString McpWriteTransaction::fingerprint(const QJsonObject &values)
{
    if (!validValues(values)) {
        return {};
    }
    return QString::fromLatin1(
        QCryptographicHash::hash(QJsonDocument(values).toJson(QJsonDocument::Compact),
                                 QCryptographicHash::Sha256)
            .toHex());
}

bool McpWriteTransaction::active() const
{
    return mState >= ReadingBefore && mState <= ReadingBack;
}

bool McpWriteTransaction::checkGuard(const Guard &guard, QString *reason) const
{
    if (guard.target.isEmpty() || (active() && guard.target != mTarget) || !guard.connected ||
        !guard.firmwareCompatible || !guard.exclusiveAccess) {
        *reason = "Connection, device/route, firmware or exclusive access is not verified.";
        return false;
    }
    if (!guard.inputNeutral || guard.valuesAgeMs < 0 || guard.valuesAgeMs > 250 ||
        guard.inputAgeMs < 0 || guard.inputAgeMs > 250 || !std::isfinite(guard.erpm) ||
        !std::isfinite(guard.duty) || !std::isfinite(guard.motorCurrent) ||
        std::abs(guard.erpm) > 1.0 || std::abs(guard.duty) > 0.001 ||
        std::abs(guard.motorCurrent) > 0.5) {
        *reason = "Fresh standstill and neutral active-input data are required.";
        return false;
    }
    return true;
}

bool McpWriteTransaction::begin(const QString &config, const QJsonObject &desired,
                                const Guard &guard)
{
    if (active()) {
        return false;
    }
    // Do not replace the previous result with a malformed new request.
    if ((config != "motor" && config != "app") || !validValues(desired)) {
        return false;
    }
    QString reason;
    if (!checkGuard(guard, &reason)) {
        return false;
    }
    mId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    mConfig = config;
    mTarget = guard.target;
    mDesired = desired;
    mBefore = {};
    mBackup.clear();
    mError.clear();
    mWriteIssued = false;
    const auto id = mId;
    enter(ReadingBefore);
    if (mId != id || mState != ReadingBefore) {
        return false;
    }
    emit readRequested(id, false);
    return true;
}

bool McpWriteTransaction::configReceived(const QString &id, const QString &target,
                                         const QJsonObject &values, const Guard &guard)
{
    if (id != mId || (mState != ReadingBefore && mState != ReadingBack)) {
        return false;
    }
    QString reason;
    if (target != mTarget || !checkGuard(guard, &reason)) {
        fail(reason.isEmpty() ? "Configuration reply came from a different target." : reason);
        return false;
    }
    if (!validValues(values) || values.keys() != mDesired.keys()) {
        fail("Configuration reply is incomplete or has incompatible parameter keys.");
        return false;
    }
    if (mState == ReadingBefore) {
        mBefore = values;
        const auto before = mBefore;
        enter(SavingBackup);
        if (mId != id || mState != SavingBackup) {
            return false;
        }
        emit backupRequested(id, before);
        return true;
    }
    // Expected values must already be canonicalized by the future wire adapter.
    // Exact comparison fails closed if firmware quantization was not accounted for.
    if (values != mDesired) {
        fail("Controller readback differs from the proposed configuration.");
        return false;
    }
    enter(Succeeded);
    return true;
}

bool McpWriteTransaction::backupFinished(const QString &id, bool saved, const QString &reference)
{
    if (id != mId || mState != SavingBackup) {
        return false;
    }
    if (!saved || reference.isEmpty()) {
        fail("A confirmed durable controller backup is required.");
        return false;
    }
    mBackup = reference;
    enter(AwaitingApproval);
    return true;
}

bool McpWriteTransaction::approve(const QString &id, const QString &beforeFingerprint,
                                  const QString &currentDesiredFingerprint, const Guard &guard)
{
    if (id != mId || mState != AwaitingApproval) {
        return false;
    }
    if (beforeFingerprint != fingerprint(mBefore) ||
        currentDesiredFingerprint != fingerprint(mDesired)) {
        fail("The reviewed/current configuration fingerprints no longer match.");
        return false;
    }
    QString reason;
    if (!checkGuard(guard, &reason)) {
        fail(reason);
        return false;
    }
    const auto desired = mDesired;
    enter(AwaitingAck);
    if (mId != id || mState != AwaitingAck) {
        return false;
    }
    mWriteIssued = true;
    // Set state before emission so a synchronous fake adapter is supported.
    emit writeRequested(id, desired);
    return true;
}

bool McpWriteTransaction::ackReceived(const QString &id, const QString &target, const QString &ack,
                                      const Guard &guard)
{
    if (id != mId || mState != AwaitingAck || !mWriteIssued) {
        return false;
    }
    QString reason;
    const auto expected = mConfig == "motor" ? "Motor config write OK" : "App config write OK";
    if (target != mTarget || !checkGuard(guard, &reason)) {
        fail(reason.isEmpty() ? "Write acknowledgment came from a different target." : reason);
        return false;
    }
    if (ack != expected) {
        fail("Unexpected or ambiguous write acknowledgment.");
        return false;
    }
    enter(ReadingBack);
    if (mId != id || mState != ReadingBack) {
        return false;
    }
    emit readRequested(id, true);
    return true;
}

void McpWriteTransaction::observe(const Guard &guard)
{
    if (!active()) {
        return;
    }
    QString reason;
    if (!checkGuard(guard, &reason)) {
        fail(reason);
    }
}

void McpWriteTransaction::cancel(const QString &id)
{
    if (id != mId || !active()) {
        return;
    }
    mError = "Cancelled; an issued write cannot be undone or presumed cancelled.";
    enter(Cancelled);
}

McpWriteTransaction::State McpWriteTransaction::state() const
{
    return mState;
}

QString McpWriteTransaction::id() const
{
    return mId;
}

QString McpWriteTransaction::stateName(State state)
{
    switch (state) {
    case Idle:
        return "idle";
    case ReadingBefore:
        return "reading_before";
    case SavingBackup:
        return "saving_backup";
    case AwaitingApproval:
        return "awaiting_approval";
    case AwaitingAck:
        return "awaiting_ack";
    case ReadingBack:
        return "reading_back";
    case Succeeded:
        return "succeeded";
    case Failed:
        return "failed";
    case Cancelled:
        return "cancelled";
    }
    return "invalid";
}

QJsonObject McpWriteTransaction::status() const
{
    return {{"id", mId},
            {"config", mConfig},
            {"target", mTarget},
            {"state", stateName(mState)},
            {"beforeFingerprint", fingerprint(mBefore)},
            {"desiredFingerprint", fingerprint(mDesired)},
            {"backupReference", mBackup},
            {"writeIssued", mWriteIssued},
            {"verified", mState == Succeeded},
            {"outcomeUnknown", mWriteIssued && mState != Succeeded},
            {"error", mError}};
}

void McpWriteTransaction::enter(State state)
{
    mState = state;
    if (active()) {
        mDeadline.start(state == AwaitingApproval ? mApprovalTimeoutMs : mOperationTimeoutMs);
    } else {
        mDeadline.stop();
    }
    emit changed();
}

void McpWriteTransaction::fail(const QString &reason)
{
    mError = reason;
    enter(Failed);
}
