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

#include "mcpdeviceoperations.h"
#include "mcpdatahelpers.h"
#include "vbytearray.h"
#include "datatypes.h"
#include <QDateTime>
#include <QCryptographicHash>
#include <QDir>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUuid>
#include <cmath>

McpDeviceOperations::McpDeviceOperations(McpDevice *device, const QString &directory,
                                         QObject *parent)
    : QObject(parent), mDevice(device), mDirectory(directory), mTransaction(this)
{
    mPoll.setParent(this);
    mDeadline.setParent(this);
    mDeadline.setSingleShot(true);
    mPoll.setInterval(50);
    connect(&mPoll, &QTimer::timeout, this, &McpDeviceOperations::tick);
    connect(&mDeadline, &QTimer::timeout, this,
            [this]() { finish(false, "Controller operation timed out."); });
    connect(device, &McpDevice::fault, this, [this](const QString &error) {
        if (busy()) {
            finish(false, error);
        }
    });
    connect(device, &McpDevice::firmware, this, [this]() {
        if (mPhase != "identifying") {
            return;
        }
        if (mDevice->target() != mTarget) {
            finish(false, "Device identity changed.");
            return;
        }
        if (mOperation == "download") {
            phase("downloading");
            send(mKind == "motor" ? COMM_GET_MCCONF : COMM_GET_APPCONF);
        } else if (mOperation == "capture") {
            phase("capturing", mDuration + 3000);
            mClock.restart();
            mLastPoll = -1000;
        } else if (mOperation == "lisp_download") {
            readLisp("lisp_download");
        } else {
            phase("reading_input_config");
            send(COMM_GET_APPCONF);
        }
    });
    connect(device, &McpDevice::configuration, this, &McpDeviceOperations::onConfig);
    connect(device, &McpDevice::telemetry, this, [this](const QJsonObject &values) {
        if (!busy()) {
            return;
        }
        mValues = values;
        mValuesAge.start();
        if (mPhase == "capturing" && mSamples.size() < 2000) {
            auto sample = values;
            sample.insert("timeMs", double(mClock.elapsed()));
            if (values.value("voltage_v").isDouble() && values.value("current_in_a").isDouble()) {
                sample.insert("power_in_w", values.value("voltage_v").toDouble() *
                                                values.value("current_in_a").toDouble());
            }
            if (mInputAge.isValid() && mInputAge.elapsed() <= 250) {
                const auto type = mInput.value("type").toString();
                if (mInput.value("value").isDouble()) {
                    sample.insert("input_" + type, mInput.value("value"));
                }
                if (type == "adc" && mInput.value("value2").isDouble()) {
                    sample.insert("input_adc2", mInput.value("value2"));
                }
            }
            mSamples.append(sample);
        }
    });
    connect(device, &McpDevice::input, this, [this](const QJsonObject &values) {
        if (!busy()) {
            return;
        }
        mInput = values;
        mInputAge.start();
    });
    connect(device, &McpDevice::acknowledgment, this, [this](const QString &text) {
        if (mPhase == "config_tx" && mTransaction.state() == McpWriteTransaction::AwaitingAck) {
            const auto expected =
                mKind == "motor" ? "Motor config write OK" : "App config write OK";
            if (text != expected) {
                finish(false, "Unexpected or ambiguous write acknowledgment.");
                return;
            }
            mPendingAck = text;
            processPending();
        }
    });
    connect(device, &McpDevice::lispCode, this, &McpDeviceOperations::onLispCode);
    connect(device, &McpDevice::lispErased, this, [this](bool ok) {
        if (mPhase != "erasing_lisp") {
            return;
        }
        if (!ok) {
            finish(false, "LispBM erase failed; inspect the controller before retrying.");
            return;
        }
        mValuesAge.invalidate();
        mInputAge.invalidate();
        phase("priming_lisp_chunks");
    });
    connect(device, &McpDevice::lispWritten, this, [this](bool ok, quint32 offset) {
        if (mPhase != "writing_lisp") {
            return;
        }
        if (int(offset) < mWriteOffset) {
            return; // Duplicate prior ACK; never resend automatically.
        }
        if (!ok || int(offset) != mWriteOffset) {
            finish(false, "LispBM write acknowledgment mismatch.");
            return;
        }
        mWriteOffset += mChunkSize;
        if (mWriteOffset >= mLispImage.size()) {
            readLisp("lisp_verify");
        } else {
            writeLispChunk();
        }
    });

    connect(&mTransaction, &McpWriteTransaction::readRequested, this,
            [this](const QString &, bool) {
                send(mKind == "motor" ? COMM_GET_MCCONF : COMM_GET_APPCONF);
            });
    connect(&mTransaction, &McpWriteTransaction::backupRequested, this,
            [this](const QString &id, const QJsonObject &) {
                const auto path = backupConfig(mKind, "controller");
                mJob.insert("controllerBackup", path);
                mTransaction.backupFinished(id, !path.isEmpty(), path);
            });
    connect(&mTransaction, &McpWriteTransaction::writeRequested, this,
            [this](const QString &, const QJsonObject &) {
                mWriteIssued = true;
                recordJob();
                VByteArray p;
                p.vbAppendUint8(mKind == "motor" ? COMM_SET_MCCONF : COMM_SET_APPCONF);
                p.append(mDesiredWire);
                sendPacket(p); // Once; success requires ACK plus canonical readback.
            });
    connect(&mTransaction, &McpWriteTransaction::changed, this, [this]() {
        if (mPhase != "config_tx") {
            return;
        }
        mJob.insert("transaction", mTransaction.status());
        if (mTransaction.state() == McpWriteTransaction::Succeeded) {
            mJob.insert("result", QJsonObject{{"config", mKind},
                                              {"verified", true},
                                              {"values", mDevice->configValues(mKind)}});
            finish(true);
        } else if (mTransaction.state() == McpWriteTransaction::Failed ||
                   mTransaction.state() == McpWriteTransaction::Cancelled) {
            finish(false, mTransaction.status().value("error").toString());
        } else if (mTransaction.state() == McpWriteTransaction::AwaitingApproval &&
                   !mApprovalSent) {
            mApprovalSent = true;
            QJsonArray changes;
            const auto before = mDevice->configValues(mKind);
            for (auto it = mDesired.begin(); it != mDesired.end(); ++it) {
                if (before.value(it.key()) != it.value()) {
                    changes.append(QJsonObject{{"parameter", it.key()},
                                               {"before", before.value(it.key())},
                                               {"after", it.value()}});
                }
            }
            mJob.insert("review", QJsonObject{{"changes", changes}, {"target", mTarget}});
            emit approvalRequested(mId, tr("Write controller configuration"),
                                   QString::fromUtf8(QJsonDocument(mJob.value("review").toObject())
                                                         .toJson(QJsonDocument::Indented)));
        }
        recordJob();
    });
}

McpDeviceOperations::~McpDeviceOperations()
{
    mPoll.stop();
    mDeadline.stop();
    mDevice->release(this);
}
bool McpDeviceOperations::busy() const
{
    return !mId.isEmpty();
}
void McpDeviceOperations::recordJob()
{
    if (!mId.isEmpty()) {
        mJob.insert("state", mPhase);
        mJob.insert("writeIssued", mWriteIssued);
        mJob.insert("outcomeUnknown", mWriteIssued);
        mJobs.insert(mId, mJob);
    }
}
void McpDeviceOperations::phase(const QString &name, int timeoutMs)
{
    if (!busy()) {
        return;
    }
    mPhase = name;
    if (timeoutMs) {
        mDeadline.start(timeoutMs);
    } else {
        mDeadline.stop();
    }
    recordJob();
}
bool McpDeviceOperations::sendPacket(const QByteArray &packet)
{
    if (!busy() || mDevice->target() != mTarget || !mDevice->send(this, packet)) {
        finish(false, "Exclusive command channel or device context was lost.");
        return false;
    }
    return true;
}
bool McpDeviceOperations::send(int command)
{
    return sendPacket(QByteArray(1, char(command)));
}
QString McpDeviceOperations::save(const QString &suffix, const QByteArray &bytes)
{
    if (mDirectory.isEmpty() || !QDir().mkpath(mDirectory)) {
        return {};
    }
    const auto path = QDir(mDirectory).filePath(mId + "-" + suffix);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        return {};
    }
    return path;
}
QString McpDeviceOperations::backupConfig(const QString &kind, const QString &source)
{
    const auto xml = mDevice->configXml(kind);
    if (xml.isEmpty()) {
        return {};
    }
    const auto prefix = source + "-" + kind;
    if (save(prefix + ".xml", xml).isEmpty()) {
        return {};
    }
    const QJsonObject data{
        {"source", source},
        {"target", mTarget},
        {"config", kind},
        {"values", mDevice->configValues(kind)},
        {"wireBase64", QString::fromLatin1(mDevice->configWire(kind).toBase64())}};
    return save(prefix + ".json", QJsonDocument(data).toJson(QJsonDocument::Indented));
}
QJsonObject McpDeviceOperations::start(const QString &operation, const QJsonObject &arguments)
{
    if (busy()) {
        return {{"error", "Another controller operation owns the app connection."}};
    }
    if (!mDevice->ready()) {
        return {{"error", "Connect a compatible controller and stop TCP/UDP forwarding first."}};
    }
    if (operation != "download" && operation != "upload" && operation != "capture" &&
        operation != "lisp_download" && operation != "lisp_upload") {
        return {{"error", "Unknown operation."}};
    }
    const auto kind = arguments.value("config").toString();
    if ((operation == "upload" || operation == "download") && kind != "motor" && kind != "app") {
        return {{"error", "config must be motor or app."}};
    }
    QJsonObject desired;
    QByteArray wire, image;
    if (operation == "upload") {
        QString error;
        desired = mDevice->canonicalValues(kind, &error);
        if (desired.isEmpty()) {
            return {{"error", error}};
        }
        wire = mDevice->configWire(kind);
    }
    if (operation == "lisp_upload") {
        QString error;
        image = McpDataHelpers::lispImage(arguments.value("source").toString(), &error);
        if (image.isEmpty()) {
            return {{"error", error}};
        }
    }
    if (!mDevice->acquire(this)) {
        return {{"error", "Could not acquire exclusive app command access."}};
    }
    mId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    mTarget = mDevice->target();
    mKind = kind;
    mOperation = operation;
    mDesired = desired;
    mDesiredWire = wire;
    mLispImage = image;
    mSamples = {};
    mValues = {};
    mInput = {};
    mValuesAge.invalidate();
    mInputAge.invalidate();
    mWriteIssued = false;
    mApprovalSent = false;
    mWriteOffset = 0;
    mPendingAck.clear();
    mPendingConfigKind.clear();
    mPendingConfigValues = {};
    mDuration = arguments.value("durationMs").toInt(1000);
    mInterval = arguments.value("intervalMs").toInt(50);
    mJob = {{"id", mId}, {"operation", operation}, {"target", mTarget}, {"writeIssued", false}};
    if (operation == "lisp_upload") {
        mJob.insert("sourceSha256",
                    QString::fromLatin1(
                        QCryptographicHash::hash(arguments.value("source").toString().toUtf8(),
                                                 QCryptographicHash::Sha256)
                            .toHex()));
    }
    if (mDuration < 100 || mDuration > 60000 || mInterval < 50 || mInterval > 1000) {
        finish(false, "Capture duration/interval is outside its limits.");
        return mJob;
    }
    const auto id = mId;
    if (operation == "upload" || operation == "download") {
        const auto backup = backupConfig(kind, "app");
        if (backup.isEmpty()) {
            finish(false, "Could not save the original app-memory configuration.");
            return mJob;
        }
        mJob.insert("appBackup", backup);
    }
    phase("draining");
    mClock.start();
    mLastPoll = -1000;
    mPoll.start();
    emit busyChanged(true);
    return {{"id", id}, {"state", mPhase}};
}
void McpDeviceOperations::finish(bool ok, const QString &error)
{
    if (!busy()) {
        return;
    }
    mPoll.stop();
    mDeadline.stop();
    mJob.insert("state", ok ? "succeeded" : "failed");
    mJob.insert("error", error);
    mJob.insert("writeIssued", mWriteIssued);
    mJob.insert("outcomeUnknown", mWriteIssued && !ok);
    mJobs.insert(mId, mJob);
    mHistory.append(mId);
    while (mHistory.size() > 8) {
        mJobs.remove(mHistory.takeFirst());
    }
    mDevice->release(this);
    const auto transaction = mTransaction.id();
    mId.clear();
    mPhase.clear();
    mTransaction.cancel(transaction);
    emit busyChanged(false);
}
QJsonObject McpDeviceOperations::job(const QString &id, bool includeSamples) const
{
    QJsonObject result = mJobs.value(id);
    if (busy() && id == mId) {
        result = mJob;
        result.insert("state", mPhase);
        result.insert("writeIssued", mWriteIssued);
        result.insert("outcomeUnknown", mWriteIssued);
        if (mPhase == "config_tx") {
            result.insert("transaction", mTransaction.status());
        }
        if (mPhase == "capturing") {
            result.insert("result", McpDataHelpers::analyze(mSamples));
            if (includeSamples) {
                result.insert("samples", mSamples);
            }
        }
    }
    if (result.isEmpty()) {
        return {{"error", "Unknown job id."}};
    }
    if (!includeSamples) {
        result.remove("samples");
    }
    return result;
}
void McpDeviceOperations::cancel(const QString &id)
{
    if (id == mId && busy()) {
        finish(false, "Cancelled; issued writes are not automatically rolled back.");
    }
}
McpWriteTransaction::Guard McpDeviceOperations::guard() const
{
    McpWriteTransaction::Guard g;
    g.target = mDevice->target();
    g.connected = mDevice->ready();
    g.firmwareCompatible = g.connected;
    g.exclusiveAccess = busy();
    const auto mode = mDevice->inputMode();
    g.inputNeutral = mode == "none";
    if (mode != "none" && mInput.value("type") == mode && mInput.value("value").isDouble()) {
        g.inputNeutral = std::isfinite(mInput.value("value").toDouble()) &&
                         std::abs(mInput.value("value").toDouble()) <= 0.01;
        if (mode == "adc") {
            g.inputNeutral &= mInput.value("value2").isDouble() &&
                              std::isfinite(mInput.value("value2").toDouble()) &&
                              std::abs(mInput.value("value2").toDouble()) <= 0.01;
        }
    }
    g.valuesAgeMs = mValuesAge.isValid() ? mValuesAge.elapsed() : -1;
    g.inputAgeMs =
        mode == "none" ? g.valuesAgeMs : (mInputAge.isValid() ? mInputAge.elapsed() : -1);
    if (mValues.value("erpm").isDouble()) {
        g.erpm = mValues.value("erpm").toDouble();
    }
    if (mValues.value("duty").isDouble()) {
        g.duty = mValues.value("duty").toDouble();
    }
    if (mValues.value("current_motor_a").isDouble()) {
        g.motorCurrent = mValues.value("current_motor_a").toDouble();
    }
    return g;
}
void McpDeviceOperations::tick()
{
    if (!busy()) {
        return;
    }
    if (!mDevice->ready() || mDevice->target() != mTarget) {
        finish(false, "Connection, identity or route changed.");
        return;
    }
    if (mPhase == "draining") {
        if (mClock.elapsed() >= 350) {
            phase("identifying");
            send(COMM_FW_VERSION);
        }
        return;
    }
    if (mPhase == "identifying" || mPhase == "downloading" || mPhase.startsWith("lisp_") ||
        mPhase == "erasing_lisp" || mPhase == "writing_lisp") {
        return;
    }
    if (mPhase == "capturing" && mClock.elapsed() >= mDuration) {
        mJob.insert("samples", mSamples);
        auto analysis = McpDataHelpers::analyze(mSamples);
        analysis.insert("captureDurationMs", double(mClock.elapsed()));
        analysis.insert("sampleLimitReached", mSamples.size() >= 2000);
        const auto path = save("rt.json", QJsonDocument(QJsonObject{{"target", mTarget},
                                                                    {"samples", mSamples},
                                                                    {"analysis", analysis}})
                                              .toJson());
        analysis.insert("recordingPath", path);
        if (path.isEmpty()) {
            analysis.insert(
                "warning", "Local recording save failed; samples remain in the bounded job cache.");
        }
        mJob.insert("result", analysis);
        finish(!mSamples.isEmpty(), mSamples.isEmpty() ? "No RT samples received." : QString());
        return;
    }
    if (mClock.elapsed() - mLastPoll >= (mPhase == "capturing" ? mInterval : 50)) {
        mLastPoll = mClock.elapsed();
        const auto mode = mDevice->inputMode();
        if (mode == "adc" && !send(COMM_GET_DECODED_ADC)) {
            return;
        }
        if (mode == "ppm" && !send(COMM_GET_DECODED_PPM)) {
            return;
        }
        if (!send(COMM_GET_VALUES)) {
            return;
        }
    }
    if (mPhase == "config_tx") {
        const auto g = guard();
        if (mTransaction.state() == McpWriteTransaction::AwaitingApproval ||
            (g.valuesAgeMs >= 0 && g.valuesAgeMs <= 250 && g.inputAgeMs >= 0 &&
             g.inputAgeMs <= 250)) {
            mTransaction.observe(g);
        }
        processPending();
        return;
    }
    if (mPhase == "priming" || mPhase == "priming_lisp_chunks") {
        if (mDevice->inputMode() == "unsupported") {
            finish(false, "Uploads require No App, ADC or PPM mode; other active-input modes "
                          "cannot be verified.");
            return;
        }
        auto g = guard();
        if (!g.inputNeutral || g.valuesAgeMs < 0 || g.valuesAgeMs > 250 || g.inputAgeMs < 0 ||
            g.inputAgeMs > 250 || !std::isfinite(g.erpm) || !std::isfinite(g.duty) ||
            !std::isfinite(g.motorCurrent)) {
            return;
        }
        if (std::abs(g.erpm) > 1 || std::abs(g.duty) > 0.001 || std::abs(g.motorCurrent) > 0.5) {
            finish(false, "The motor is not at standstill.");
            return;
        }
        if (mPhase == "priming_lisp_chunks") {
            writeLispChunk();
            return;
        }
        if (mOperation == "upload") {
            phase("config_tx", 0);
            if (!mTransaction.begin(mKind, mDesired, g)) {
                finish(false, "Write guards were not accepted.");
            }
        } else {
            readLisp("lisp_backup");
        }
    }
}
void McpDeviceOperations::onConfig(const QString &kind)
{
    if (!busy()) {
        return;
    }
    if (mPhase == "reading_input_config" && kind == "app") {
        phase("priming");
        return;
    }
    if (mPhase == "downloading" && kind == mKind) {
        const auto path = backupConfig(kind, "controller");
        if (path.isEmpty()) {
            finish(false, "Could not save controller download.");
            return;
        }
        mJob.insert("result", QJsonObject{{"config", kind},
                                          {"source", "controller"},
                                          {"values", mDevice->configValues(kind)},
                                          {"backup", path}});
        finish(true);
    } else if (mPhase == "config_tx" && kind == mKind) {
        if (mTransaction.state() == McpWriteTransaction::ReadingBefore) {
            QString error;
            if (mDevice->configWire(kind).left(4) != mDesiredWire.left(4) ||
                !mDevice->validateChanges(kind, mDesired, &error)) {
                finish(false, error.isEmpty() ? "Configuration schema changed." : error);
                return;
            }
        }
        if (mTransaction.state() == McpWriteTransaction::ReadingBefore ||
            mTransaction.state() == McpWriteTransaction::ReadingBack) {
            mPendingConfigKind = kind;
            mPendingConfigValues = mDevice->configValues(kind);
            processPending();
        } else if (mTransaction.state() == McpWriteTransaction::AwaitingApproval &&
                   McpWriteTransaction::fingerprint(mDevice->configValues(kind)) !=
                       mTransaction.status().value("beforeFingerprint").toString()) {
            finish(false, "Configuration changed during review.");
        }
    }
}

void McpDeviceOperations::processPending()
{
    if (mPhase != "config_tx") {
        return;
    }
    const auto g = guard();
    if (g.valuesAgeMs < 0 || g.valuesAgeMs > 250 || g.inputAgeMs < 0 || g.inputAgeMs > 250) {
        return;
    }
    mTransaction.observe(g);
    if (mPhase != "config_tx") {
        return;
    }
    if (!mPendingAck.isEmpty() && mTransaction.state() == McpWriteTransaction::AwaitingAck) {
        const auto ack = mPendingAck;
        mPendingAck.clear();
        mTransaction.ackReceived(mTransaction.id(), mDevice->target(), ack, g);
    }
    if (mPhase != "config_tx") {
        return;
    }
    if (!mPendingConfigKind.isEmpty() &&
        (mTransaction.state() == McpWriteTransaction::ReadingBefore ||
         mTransaction.state() == McpWriteTransaction::ReadingBack)) {
        const auto values = mPendingConfigValues;
        const auto kind = mPendingConfigKind;
        mPendingConfigKind.clear();
        mPendingConfigValues = {};
        if (values != mDevice->configValues(kind)) {
            finish(false,
                   "App configuration changed before the controller reply could be verified.");
            return;
        }
        mTransaction.configReceived(mTransaction.id(), mDevice->target(), values, g);
    }
}
void McpDeviceOperations::approve(const QString &id, bool accepted)
{
    if (!busy() || id != mId) {
        return;
    }
    if (!accepted) {
        finish(false, "The app user declined the operation.");
        return;
    }
    if (mPhase == "config_tx" && mTransaction.state() == McpWriteTransaction::AwaitingApproval) {
        const auto before = mTransaction.status().value("beforeFingerprint").toString();
        if (McpWriteTransaction::fingerprint(mDevice->configValues(mKind)) != before) {
            finish(false, "App configuration changed during review.");
            return;
        }
        mTransaction.approve(mTransaction.id(), before, McpWriteTransaction::fingerprint(mDesired),
                             guard());
    } else if (mPhase == "awaiting_lisp_approval") {
        auto g = guard();
        if (!g.connected || !g.inputNeutral || g.valuesAgeMs < 0 || g.valuesAgeMs > 250 ||
            g.inputAgeMs < 0 || g.inputAgeMs > 250 || !std::isfinite(g.erpm) ||
            std::abs(g.erpm) > 1 || !std::isfinite(g.duty) || std::abs(g.duty) > 0.001 ||
            !std::isfinite(g.motorCurrent) || std::abs(g.motorCurrent) > 0.5) {
            finish(false, "Fresh standstill/neutral input is required before upload.");
            return;
        }
        mWriteIssued = true;
        // The firmware's Lisp erase path stops the VM before erasing. A
        // separate pause command fails if no Lisp VM is running yet.
        phase("erasing_lisp");
        VByteArray p;
        p.vbAppendUint8(COMM_LISP_ERASE_CODE);
        p.vbAppendInt32(mLispImage.size() + 100);
        sendPacket(p);
    }
}
void McpDeviceOperations::readLisp(const QString &nextPhase)
{
    mReadData.clear();
    mReadTotal = -1;
    mReadRequested = 0;
    phase(nextPhase);
    VByteArray p;
    p.vbAppendUint8(COMM_LISP_READ_CODE);
    p.vbAppendInt32(0);
    p.vbAppendInt32(0);
    sendPacket(p);
}
void McpDeviceOperations::onLispCode(int total, int offset, const QByteArray &data)
{
    if (!busy() ||
        (mPhase != "lisp_download" && mPhase != "lisp_backup" && mPhase != "lisp_verify")) {
        return;
    }
    if (offset < mReadData.size()) {
        return;
    }
    if (total < 0 || total > 131072 || offset != mReadData.size() ||
        data.size() != mReadRequested || offset + data.size() > total ||
        (mReadTotal >= 0 && total != mReadTotal)) {
        finish(false, "Invalid LispBM read chunk.");
        return;
    }
    if (mReadTotal >= 0 && total > offset && data.isEmpty()) {
        finish(false, "Empty LispBM data chunk.");
        return;
    }
    mReadTotal = total;
    mReadData.append(data);
    mDeadline.start(8000);
    if (mReadData.size() < total) {
        VByteArray p;
        p.vbAppendUint8(COMM_LISP_READ_CODE);
        mReadRequested = qMin(384, total - mReadData.size());
        p.vbAppendInt32(mReadRequested);
        p.vbAppendInt32(mReadData.size());
        sendPacket(p);
        return;
    }
    if (mPhase == "lisp_verify") {
        if (mReadData != McpDataHelpers::lispReadback(mLispImage)) {
            finish(false, "LispBM readback differs from the uploaded image.");
            return;
        }
        mJob.insert(
            "result",
            QJsonObject{{"verified", true}, {"running", false}, {"bytes", mReadData.size()}});
        finish(true);
    } else {
        const auto path = save("lisp-controller.bin", mReadData);
        if (path.isEmpty()) {
            finish(false, "Could not save LispBM backup.");
            return;
        }
        if (mPhase == "lisp_download") {
            mJob.insert("result", QJsonObject{{"source", McpDataHelpers::lispSource(mReadData)},
                                              {"backup", path},
                                              {"bytes", mReadData.size()}});
            finish(true);
        } else {
            mJob.insert("controllerBackup", path);
            phase("awaiting_lisp_approval", 60000);
            emit approvalRequested(
                mId, tr("Upload LispBM script"),
                tr("The existing program will be replaced. The VM will be stopped and not "
                   "restarted. "
                   "The uploaded program may run after a later controller restart.\nDevice: "
                   "%1\nBackup: %2\n\n%3")
                    .arg(mTarget, path,
                         McpDataHelpers::lispSource(McpDataHelpers::lispReadback(mLispImage))));
        }
    }
}
void McpDeviceOperations::writeLispChunk()
{
    const auto g = guard();
    if (!g.inputNeutral || g.valuesAgeMs < 0 || g.valuesAgeMs > 250 || g.inputAgeMs < 0 ||
        g.inputAgeMs > 250 || !std::isfinite(g.erpm) || std::abs(g.erpm) > 1 ||
        !std::isfinite(g.duty) || std::abs(g.duty) > 0.001 || !std::isfinite(g.motorCurrent) ||
        std::abs(g.motorCurrent) > 0.5) {
        mValuesAge.invalidate();
        mInputAge.invalidate();
        phase("priming_lisp_chunks");
        return;
    }
    phase("writing_lisp");
    mChunkSize = qMin(384, mLispImage.size() - mWriteOffset);
    VByteArray p;
    p.vbAppendUint8(COMM_LISP_WRITE_CODE);
    p.vbAppendUint32(mWriteOffset);
    p.append(mLispImage.mid(mWriteOffset, mChunkSize));
    sendPacket(p);
}
