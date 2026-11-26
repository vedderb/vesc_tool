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

#include <QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "mcpdeviceoperations.h"
#include "mcpdatahelpers.h"
#include "vbytearray.h"
#include "datatypes.h"

class FakeMcpDevice : public McpDevice
{
    Q_OBJECT
  public:
    bool online = true, failErase = false, badAck = false, badReadback = false;
    bool inputActive = false, rejectChanges = false;
    bool flashBusy = false;
    int flashDelayMs = 0;
    int backupDelayMs = 0;
    QString identity = "fake-device";
    QString mode = "adc";
    QPointer<QObject> owner;
    QJsonObject motor{{"limit", 10.25}, {"enabled", true}};
    QJsonObject app{{"controller_id", 2}};
    QJsonObject storedMotor{{"limit", 5.0}, {"enabled", true}};
    QJsonObject storedApp = app;
    QByteArray flash;
    QByteArray lisp = QByteArray("(print 1)") + QByteArray(3, '\0');
    QList<int> packets;
    int configWrites = 0, codeWrites = 0, runningRequests = 0;
    bool ready() const override
    {
        return online;
    }
    QString target() const override
    {
        return identity;
    }
    bool acquire(QObject *value) override
    {
        if (owner) {
            return false;
        }
        owner = value;
        return true;
    }
    void release(QObject *value) override
    {
        if (owner == value) {
            owner.clear();
        }
    }
    QJsonObject configValues(const QString &kind) const override
    {
        return kind == "motor" ? motor : app;
    }
    QJsonObject canonicalValues(const QString &kind, QString *) const override
    {
        return configValues(kind);
    }
    QByteArray configWire(const QString &kind) const override
    {
        return "SIG0" + QJsonDocument(configValues(kind)).toJson(QJsonDocument::Compact);
    }
    QByteArray configXml(const QString &) const override
    {
        if (backupDelayMs) {
            QThread::msleep(backupDelayMs); // Model blocking backup I/O and aged measurements.
        }
        return "<Configuration/>";
    }
    bool validateChanges(const QString &, const QJsonObject &, QString *error) const override
    {
        if (rejectChanges) {
            *error = "invalid change";
        }
        return !rejectChanges;
    }
    QString inputMode() const override
    {
        return mode;
    }

    bool send(QObject *value, const QByteArray &packet) override
    {
        if (!online || owner != value || packet.isEmpty()) {
            return false;
        }
        VByteArray payload(packet);
        const int command = payload.vbPopFrontUint8();
        packets.append(command);
        QTimer::singleShot(0, this, [this, command, payload]() mutable {
            switch (command) {
            case COMM_FW_VERSION:
                emit firmware();
                break;
            case COMM_GET_MCCONF:
                motor = storedMotor;
                emit configuration("motor");
                break;
            case COMM_GET_APPCONF:
                app = storedApp;
                emit configuration("app");
                break;
            case COMM_GET_VALUES:
                if (flashBusy) {
                    break;
                }
                emit telemetry(
                    {{"erpm", 0.0}, {"duty", 0.0}, {"current_motor_a", 0.0}, {"voltage_v", 40.0}});
                break;
            case COMM_GET_DECODED_ADC:
                if (flashBusy) {
                    break;
                }
                emit input({{"type", "adc"},
                            {"value", inputActive ? 0.3 : 0.0},
                            {"value2", 0.0},
                            {"voltage_v", 1.1},
                            {"voltage2_v", 0.2}});
                break;
            case COMM_SET_MCCONF:
            case COMM_SET_APPCONF: {
                configWrites++;
                const auto values = QJsonDocument::fromJson(payload.mid(4)).object();
                if (command == COMM_SET_MCCONF) {
                    storedMotor = badReadback ? storedMotor : values;
                } else {
                    storedApp = values;
                }
                const auto text = badAck                       ? "unexpected"
                                  : command == COMM_SET_MCCONF ? "Motor config write OK"
                                                               : "App config write OK";
                if (flashDelayMs) {
                    flashBusy = true;
                    QTimer::singleShot(flashDelayMs, this, [this, text]() {
                        flashBusy = false;
                        emit acknowledgment(text);
                    });
                } else {
                    emit acknowledgment(text);
                }
            } break;
            case COMM_LISP_READ_CODE: {
                const int len = payload.vbPopFrontInt32(), offset = payload.vbPopFrontInt32();
                emit lispCode(lisp.size(), offset, lisp.mid(offset, len));
            } break;
            case COMM_LISP_SET_RUNNING:
                runningRequests += payload.vbPopFrontInt8() ? 100 : 1;
                break;
            case COMM_LISP_ERASE_CODE:
                if (!failErase) {
                    flash.clear();
                    lisp.clear();
                }
                emit lispErased(!failErase);
                break;
            case COMM_LISP_WRITE_CODE: {
                const auto offset = payload.vbPopFrontUint32();
                codeWrites++;
                const bool ok = int(offset) == flash.size();
                if (ok) {
                    flash += payload;
                    lisp = flash.mid(8);
                }
                emit lispWritten(ok, offset);
            } break;
            default:
                break;
            }
        });
        return true;
    }
};

class TestMcpDeviceOperations : public QObject
{
    Q_OBJECT
  private:
    QString id(const QJsonObject &value)
    {
        return value.value("id").toString();
    }
    void approveWithFreshMeasurements(FakeMcpDevice &device, McpDeviceOperations &ops,
                                      const QString &job)
    {
        // Immediate fake GUI approval must not assume disk I/O finished within
        // the 250 ms freshness window. Publish current sensor input explicitly.
        emit device.input(
            {{"type", "adc"}, {"value", device.inputActive ? 0.3 : 0.0}, {"value2", 0.0}});
        emit device.telemetry({{"erpm", 0.0}, {"duty", 0.0}, {"current_motor_a", 0.0}});
        ops.approve(job, true);
    }
  private slots:
    void controllerDownload()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        McpDeviceOperations ops(&device, dir.path());
        const auto job = id(ops.start("download", {{"config", "motor"}}));
        QVERIFY(!job.isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 3000);
        QCOMPARE(ops.job(job).value("state").toString(), QString("succeeded"));
        QCOMPARE(device.motor, device.storedMotor);
        QVERIFY(QFile::exists(ops.job(job).value("appBackup").toString()));
        QCOMPARE(device.configWrites, 0);
    }
    void controllerUpload()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        McpDeviceOperations ops(&device, dir.path());
        const auto desired = device.motor;
        QSignalSpy review(&ops, &McpDeviceOperations::approvalRequested);
        const auto job = id(ops.start("upload", {{"config", "motor"}}));
        QTRY_COMPARE_WITH_TIMEOUT(review.count(), 1, 3000);
        QCOMPARE(device.configWrites, 0);
        approveWithFreshMeasurements(device, ops, job);
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 3000);
        QCOMPARE(ops.job(job).value("state").toString(), QString("succeeded"));
        QCOMPARE(device.configWrites, 1);
        QCOMPARE(device.storedMotor, desired);
        QVERIFY(ops.job(job).value("result").toObject().value("verified").toBool());
        QVERIFY(!device.owner);
    }
    void adcUartUploadNeedsReviewAndNeutralInput()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        device.mode = "adc_uart";
        McpDeviceOperations ops(&device, dir.path());
        QSignalSpy review(&ops, &McpDeviceOperations::approvalRequested);
        const auto job = id(ops.start("upload", {{"config", "motor"}}));
        QTRY_COMPARE_WITH_TIMEOUT(review.count(), 1, 3000);
        QVERIFY(ops.job(job).value("unmonitoredUart").toBool());
        QVERIFY(ops.job(job).value("review").toObject().value("unmonitoredUart").toBool());
        QCOMPARE(device.configWrites, 0);
        device.inputActive = true;
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 3000);
        QCOMPARE(device.configWrites, 0);
        QCOMPARE(ops.job(job).value("state").toString(), QString("failed"));
    }
    void adcUartApprovedUpload()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        device.mode = "adc_uart";
        McpDeviceOperations ops(&device, dir.path());
        QSignalSpy review(&ops, &McpDeviceOperations::approvalRequested);
        const auto job = id(ops.start("upload", {{"config", "motor"}}));
        QTRY_COMPARE_WITH_TIMEOUT(review.count(), 1, 3000);
        QCOMPARE(device.configWrites, 0);
        approveWithFreshMeasurements(device, ops, job);
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 3000);
        QCOMPARE(device.configWrites, 1);
        QCOMPARE(ops.job(job).value("state").toString(), QString("succeeded"));
    }
    void deniedUploadNeverWrites()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        McpDeviceOperations ops(&device, dir.path());
        QSignalSpy review(&ops, &McpDeviceOperations::approvalRequested);
        const auto job = id(ops.start("upload", {{"config", "motor"}}));
        QTRY_COMPARE_WITH_TIMEOUT(review.count(), 1, 3000);
        ops.approve(job, false);
        QVERIFY(!ops.busy());
        QCOMPARE(device.configWrites, 0);
        QCOMPARE(ops.job(job).value("state").toString(), QString("failed"));
    }
    void delayedFlashAckWaitsForFreshTelemetry()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        device.flashDelayMs = 400;
        McpDeviceOperations ops(&device, dir.path());
        connect(&ops, &McpDeviceOperations::approvalRequested, &ops,
                [this, &device, &ops](const QString &id, const QString &, const QString &) {
                    approveWithFreshMeasurements(device, ops, id);
                });
        const auto job = id(ops.start("upload", {{"config", "motor"}}));
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 4000);
        QCOMPARE(ops.job(job).value("state").toString(), QString("succeeded"));
        QCOMPARE(device.configWrites, 1);
    }
    void readbackMismatch()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        device.badReadback = true;
        McpDeviceOperations ops(&device, dir.path());
        connect(&ops, &McpDeviceOperations::approvalRequested, &ops,
                [this, &device, &ops](const QString &id, const QString &, const QString &) {
                    approveWithFreshMeasurements(device, ops, id);
                });
        const auto job = id(ops.start("upload", {{"config", "motor"}}));
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 3000);
        QVERIFY(ops.job(job).value("outcomeUnknown").toBool());
        QCOMPARE(device.configWrites, 1);
        QCOMPARE(ops.job(job).value("state").toString(), QString("failed"));
    }
    void wrongAck()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        device.badAck = true;
        device.backupDelayMs = 300;
        McpDeviceOperations ops(&device, dir.path());
        connect(&ops, &McpDeviceOperations::approvalRequested, &ops,
                [this, &device, &ops](const QString &id, const QString &, const QString &) {
                    approveWithFreshMeasurements(device, ops, id);
                });
        const auto job = id(ops.start("upload", {{"config", "app"}}));
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 3000);
        QVERIFY2(ops.job(job).value("outcomeUnknown").toBool(),
                 qPrintable(QString::fromUtf8(QJsonDocument(ops.job(job)).toJson())));
        QCOMPARE(device.configWrites, 1);
    }
    void changedDevice()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        McpDeviceOperations ops(&device, dir.path());
        const auto job = id(ops.start("download", {{"config", "motor"}}));
        device.identity = "another-device";
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 1000);
        QCOMPARE(ops.job(job).value("state").toString(), QString("failed"));
        QVERIFY(!device.owner);
    }
    void concurrentJobRejected()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        McpDeviceOperations ops(&device, dir.path());
        const auto first = id(ops.start("download", {{"config", "motor"}}));
        QVERIFY(!first.isEmpty());
        QVERIFY(ops.start("download", {{"config", "app"}}).contains("error"));
        ops.cancel(first);
        QVERIFY(!device.owner);
    }
    void rtRecording()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        device.mode = "adc_uart";
        McpDeviceOperations ops(&device, dir.path());
        const auto job = id(ops.start("capture", {{"durationMs", 250}, {"intervalMs", 50}}));
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 3000);
        const auto data = ops.job(job, true);
        QCOMPARE(data.value("state").toString(), QString("succeeded"));
        QVERIFY(data.value("samples").toArray().size() >= 2);
        QVERIFY(data.value("samples").toArray().last().toObject().contains("input_adc"));
        const auto last = data.value("samples").toArray().last().toObject();
        QCOMPARE(last.value("input_adc_voltage_v").toDouble(), 1.1);
        QCOMPARE(last.value("input_adc2_voltage_v").toDouble(), 0.2);
        QVERIFY(last.value("input_age_ms").toDouble() <= 250);
        QCOMPARE(data.value("result")
                     .toObject()
                     .value("fields")
                     .toObject()
                     .value("voltage_v")
                     .toObject()
                     .value("mean")
                     .toDouble(),
                 40.0);
        QVERIFY(!ops.job(job).contains("samples"));
        QVERIFY(QFile::exists(data.value("result").toObject().value("recordingPath").toString()));
        QCOMPARE(device.configWrites, 0);
    }
    void analyzeNumbers()
    {
        const auto result =
            McpDataHelpers::analyze({QJsonObject{{"x", 3.0}}, QJsonObject{{"x", 4.0}}});
        const auto x = result.value("fields").toObject().value("x").toObject();
        QCOMPARE(x.value("mean").toDouble(), 3.5);
        QCOMPARE(x.value("stddev").toDouble(), 0.5);
        QVERIFY(qAbs(x.value("rms").toDouble() - std::sqrt(12.5)) < 0.000001);
        QCOMPARE(McpDataHelpers::analyze({}).value("samples").toInt(), 0);
    }
    void lispRoundtrip()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        McpDeviceOperations ops(&device, dir.path());
        connect(&ops, &McpDeviceOperations::approvalRequested, &ops,
                [this, &device, &ops](const QString &id, const QString &, const QString &) {
                    approveWithFreshMeasurements(device, ops, id);
                });
        const QString source = "(print \"test\")\n" + QString(1500, ' ');
        const auto job = id(ops.start("lisp_upload", {{"source", source}}));
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 4000);
        QCOMPARE(ops.job(job).value("state").toString(), QString("succeeded"));
        QVERIFY(device.codeWrites > 1);
        QCOMPARE(device.runningRequests, 0); // erase stops the VM; no run request
        QCOMPARE(McpDataHelpers::lispSource(device.lisp), source);
        QVERIFY(ops.job(job).value("result").toObject().value("verified").toBool());
    }
    void lispEraseFailure()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        device.failErase = true;
        McpDeviceOperations ops(&device, dir.path());
        connect(&ops, &McpDeviceOperations::approvalRequested, &ops,
                [this, &device, &ops](const QString &id, const QString &, const QString &) {
                    approveWithFreshMeasurements(device, ops, id);
                });
        const auto job = id(ops.start("lisp_upload", {{"source", "(print 1)"}}));
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 4000);
        QCOMPARE(device.codeWrites, 0);
        QVERIFY(ops.job(job).value("outcomeUnknown").toBool());
    }
    void lispDownload()
    {
        QTemporaryDir dir;
        FakeMcpDevice device;
        McpDeviceOperations ops(&device, dir.path());
        const auto job = id(ops.start("lisp_download", {}));
        QTRY_VERIFY_WITH_TIMEOUT(!ops.busy(), 3000);
        QCOMPARE(ops.job(job).value("result").toObject().value("source").toString(),
                 QString("(print 1)"));
        QCOMPARE(device.runningRequests, 0);
    }
    void lispValidation()
    {
        QString error;
        QVERIFY(McpDataHelpers::lispImage("(print 1)", &error).size() > 8);
        QCOMPARE(McpDataHelpers::lispImage("(print 1)", &error).toHex(),
                 QByteArray("0000000cf4e80000287072696e74203129000000"));
        QVERIFY(McpDataHelpers::lispImage("", &error).isEmpty());
        QVERIFY(McpDataHelpers::lispImage("(import \"secret\" 'x)", &error).isEmpty());
        QVERIFY(McpDataHelpers::lispImage(QString(32769, 'x'), &error).isEmpty());
        QVERIFY(McpDataHelpers::lispImage(QString(QChar(0)), &error).isEmpty());
    }
};
QTEST_GUILESS_MAIN(TestMcpDeviceOperations)
#include "tst_mcpdeviceoperations.moc"
