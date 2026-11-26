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
#include <QSignalSpy>
#include <limits>
#include "mcpwritetransaction.h"

class TestMcpWriteTransaction : public QObject
{
    Q_OBJECT
  private:
    typedef McpWriteTransaction Tx;
    const QJsonObject mBefore{{"limit", 5.0}, {"enabled", true}};
    const QJsonObject mDesired{{"limit", 10.25}, {"enabled", true}};

    Tx::Guard guard() const
    {
        Tx::Guard value;
        value.target = "fake-device/firmware/route";
        value.connected = true;
        value.firmwareCompatible = true;
        value.exclusiveAccess = true;
        value.inputNeutral = true;
        value.valuesAgeMs = 10;
        value.inputAgeMs = 10;
        value.erpm = 0.0;
        value.duty = 0.0;
        value.motorCurrent = 0.0;
        return value;
    }

    bool prepare(Tx &tx, const QString &config = "motor")
    {
        const auto context = guard();
        return tx.begin(config, mDesired, context) &&
               tx.configReceived(tx.id(), context.target, mBefore, context) &&
               tx.backupFinished(tx.id(), true, "fake-durable-backup");
    }

    bool approve(Tx &tx)
    {
        return tx.approve(tx.id(), Tx::fingerprint(mBefore), Tx::fingerprint(mDesired), guard());
    }

  private slots:
    void successfulMotorWrite()
    {
        Tx tx;
        QSignalSpy reads(&tx, &Tx::readRequested);
        QSignalSpy backups(&tx, &Tx::backupRequested);
        QSignalSpy writes(&tx, &Tx::writeRequested);
        QVERIFY(prepare(tx));
        QCOMPARE(tx.state(), Tx::AwaitingApproval);
        QCOMPARE(reads.count(), 1);
        QCOMPARE(backups.count(), 1);
        QCOMPARE(writes.count(), 0);
        QVERIFY(approve(tx));
        QCOMPARE(writes.count(), 1);
        QVERIFY(!approve(tx));
        QVERIFY(tx.ackReceived(tx.id(), guard().target, "Motor config write OK", guard()));
        QCOMPARE(reads.count(), 2);
        QVERIFY(reads.at(1).at(1).toBool());
        QVERIFY(tx.configReceived(tx.id(), guard().target, mDesired, guard()));
        QCOMPARE(tx.state(), Tx::Succeeded);
        QVERIFY(tx.status().value("verified").toBool());
        QVERIFY(!tx.status().value("outcomeUnknown").toBool());
    }

    void successfulAppWrite()
    {
        Tx tx;
        QVERIFY(prepare(tx, "app"));
        QVERIFY(approve(tx));
        QVERIFY(tx.ackReceived(tx.id(), guard().target, "App config write OK", guard()));
        QVERIFY(tx.configReceived(tx.id(), guard().target, mDesired, guard()));
        QCOMPARE(tx.state(), Tx::Succeeded);
    }

    void missingBackup()
    {
        Tx tx;
        QSignalSpy writes(&tx, &Tx::writeRequested);
        QVERIFY(tx.begin("motor", mDesired, guard()));
        QVERIFY(tx.configReceived(tx.id(), guard().target, mBefore, guard()));
        QVERIFY(!approve(tx));
        QVERIFY(!tx.backupFinished(tx.id(), false, ""));
        QCOMPARE(tx.state(), Tx::Failed);
        QCOMPARE(writes.count(), 0);
        QVERIFY(!tx.status().value("writeIssued").toBool());
    }

    void emptyBackupReference()
    {
        Tx tx;
        QVERIFY(tx.begin("motor", mDesired, guard()));
        QVERIFY(tx.configReceived(tx.id(), guard().target, mBefore, guard()));
        QVERIFY(!tx.backupFinished(tx.id(), true, ""));
        QCOMPARE(tx.state(), Tx::Failed);
    }

    void staleApproval()
    {
        Tx tx;
        QSignalSpy writes(&tx, &Tx::writeRequested);
        QVERIFY(prepare(tx));
        QVERIFY(!tx.approve(tx.id(), Tx::fingerprint(mBefore), "changed-app-values", guard()));
        QCOMPARE(tx.state(), Tx::Failed);
        QCOMPARE(writes.count(), 0);
    }

    void guardFailures_data()
    {
        QTest::addColumn<int>("field");
        const char *names[] = {"connection",
                               "firmware",
                               "exclusive-access",
                               "active-input",
                               "old-values",
                               "old-input",
                               "unknown-values-age",
                               "unknown-input-age",
                               "erpm",
                               "duty",
                               "current",
                               "nonfinite",
                               "target"};
        for (int i = 0; i < 13; i++) {
            QTest::newRow(names[i]) << i;
        }
    }

    void guardFailures()
    {
        QFETCH(int, field);
        Tx tx;
        QSignalSpy writes(&tx, &Tx::writeRequested);
        QVERIFY(prepare(tx));
        auto unsafe = guard();
        switch (field) {
        case 0:
            unsafe.connected = false;
            break;
        case 1:
            unsafe.firmwareCompatible = false;
            break;
        case 2:
            unsafe.exclusiveAccess = false;
            break;
        case 3:
            unsafe.inputNeutral = false;
            break;
        case 4:
            unsafe.valuesAgeMs = 251;
            break;
        case 5:
            unsafe.inputAgeMs = 251;
            break;
        case 6:
            unsafe.valuesAgeMs = -1;
            break;
        case 7:
            unsafe.inputAgeMs = -1;
            break;
        case 8:
            unsafe.erpm = 2.0;
            break;
        case 9:
            unsafe.duty = 0.002;
            break;
        case 10:
            unsafe.motorCurrent = 0.6;
            break;
        case 11:
            unsafe.erpm = std::numeric_limits<double>::quiet_NaN();
            break;
        case 12:
            unsafe.target = "other-device";
            break;
        }
        QVERIFY(!tx.approve(tx.id(), Tx::fingerprint(mBefore), Tx::fingerprint(mDesired), unsafe));
        QCOMPARE(tx.state(), Tx::Failed);
        QCOMPARE(writes.count(), 0);
    }

    void unsafeStart()
    {
        Tx tx;
        auto unsafe = guard();
        unsafe.exclusiveAccess = false;
        QVERIFY(!tx.begin("motor", mDesired, unsafe));
        QCOMPARE(tx.state(), Tx::Idle);
        QVERIFY(!tx.begin("unknown", mDesired, guard()));
        QVERIFY(!tx.begin("motor", {}, guard()));
        QVERIFY(!tx.begin("motor", QJsonObject{{"nested", QJsonObject()}}, guard()));
    }

    void incompatibleConfiguration()
    {
        Tx tx;
        QVERIFY(tx.begin("motor", mDesired, guard()));
        QVERIFY(!tx.configReceived(tx.id(), guard().target, QJsonObject{{"limit", 5.0}}, guard()));
        QCOMPARE(tx.state(), Tx::Failed);
    }

    void wrongAckIsUncertain()
    {
        Tx tx;
        QVERIFY(prepare(tx));
        QVERIFY(approve(tx));
        QVERIFY(!tx.ackReceived(tx.id(), guard().target, "App config write OK", guard()));
        QCOMPARE(tx.state(), Tx::Failed);
        QVERIFY(tx.status().value("outcomeUnknown").toBool());
        QVERIFY(!tx.status().value("verified").toBool());
    }

    void readbackMismatchIsUncertain()
    {
        Tx tx;
        QVERIFY(prepare(tx));
        QVERIFY(approve(tx));
        QVERIFY(tx.ackReceived(tx.id(), guard().target, "Motor config write OK", guard()));
        QVERIFY(!tx.configReceived(tx.id(), guard().target, mBefore, guard()));
        QCOMPARE(tx.state(), Tx::Failed);
        QVERIFY(tx.status().value("outcomeUnknown").toBool());
    }

    void disconnectAfterWrite()
    {
        Tx tx;
        QVERIFY(prepare(tx));
        QVERIFY(approve(tx));
        auto disconnected = guard();
        disconnected.connected = false;
        tx.observe(disconnected);
        QCOMPARE(tx.state(), Tx::Failed);
        QVERIFY(tx.status().value("outcomeUnknown").toBool());
        QVERIFY(!tx.ackReceived(tx.id(), guard().target, "Motor config write OK", guard()));
    }

    void staleRepliesAndDuplicateApproval()
    {
        Tx tx;
        QSignalSpy writes(&tx, &Tx::writeRequested);
        QVERIFY(prepare(tx));
        QVERIFY(!tx.begin("app", mDesired, guard()));
        QVERIFY(!tx.backupFinished("old-request", true, "backup"));
        QVERIFY(!tx.ackReceived("old-request", guard().target, "Motor config write OK", guard()));
        QCOMPARE(tx.state(), Tx::AwaitingApproval);
        QVERIFY(approve(tx));
        QVERIFY(!approve(tx));
        QCOMPARE(writes.count(), 1);
        QVERIFY(!tx.ackReceived("old-request", guard().target, "Motor config write OK", guard()));
        QCOMPARE(tx.state(), Tx::AwaitingAck);
    }

    void cancelBeforeAndAfter()
    {
        Tx tx;
        QVERIFY(prepare(tx));
        tx.cancel(tx.id());
        QCOMPARE(tx.state(), Tx::Cancelled);
        QVERIFY(!tx.status().value("outcomeUnknown").toBool());
        QVERIFY(prepare(tx));
        QVERIFY(approve(tx));
        tx.cancel(tx.id());
        QCOMPARE(tx.state(), Tx::Cancelled);
        QVERIFY(tx.status().value("outcomeUnknown").toBool());
    }

    void timeoutBeforeWrite()
    {
        Tx tx(nullptr, 20, 20);
        QVERIFY(tx.begin("motor", mDesired, guard()));
        QTRY_COMPARE(tx.state(), Tx::Failed);
        QVERIFY(!tx.status().value("outcomeUnknown").toBool());
    }

    void timeoutAfterWrite()
    {
        Tx tx(nullptr, 20, 20);
        QVERIFY(prepare(tx));
        QVERIFY(approve(tx));
        QTRY_COMPARE(tx.state(), Tx::Failed);
        QVERIFY(tx.status().value("outcomeUnknown").toBool());
    }

    void timeoutAwaitingReadback()
    {
        Tx tx(nullptr, 20, 20);
        QVERIFY(prepare(tx));
        QVERIFY(approve(tx));
        QVERIFY(tx.ackReceived(tx.id(), guard().target, "Motor config write OK", guard()));
        QTRY_COMPARE(tx.state(), Tx::Failed);
        QVERIFY(tx.status().value("outcomeUnknown").toBool());
        QVERIFY(!tx.status().value("verified").toBool());
    }

    void timeoutAwaitingApproval()
    {
        Tx tx(nullptr, 20, 20);
        QSignalSpy writes(&tx, &Tx::writeRequested);
        QVERIFY(prepare(tx));
        QTRY_COMPARE(tx.state(), Tx::Failed);
        QCOMPARE(writes.count(), 0);
        QVERIFY(!approve(tx));
    }

    void wrongTargetReply()
    {
        Tx tx;
        QVERIFY(tx.begin("motor", mDesired, guard()));
        QVERIFY(!tx.configReceived(tx.id(), "other-device", mBefore, guard()));
        QCOMPARE(tx.state(), Tx::Failed);
    }

    void wrongTargetAck()
    {
        Tx tx;
        QVERIFY(prepare(tx));
        QVERIFY(approve(tx));
        QVERIFY(!tx.ackReceived(tx.id(), "other-device", "Motor config write OK", guard()));
        QVERIFY(tx.status().value("outcomeUnknown").toBool());
    }

    void staleReadbackContext()
    {
        Tx tx;
        QVERIFY(prepare(tx));
        QVERIFY(approve(tx));
        QVERIFY(tx.ackReceived(tx.id(), guard().target, "Motor config write OK", guard()));
        auto stale = guard();
        stale.inputAgeMs = 251;
        QVERIFY(!tx.configReceived(tx.id(), guard().target, mDesired, stale));
        QVERIFY(tx.status().value("outcomeUnknown").toBool());
        QVERIFY(!tx.status().value("verified").toBool());
    }

    void missingNumericTelemetry()
    {
        Tx tx;
        auto context = guard();
        context.duty = Tx::Guard().duty;
        QVERIFY(!tx.begin("motor", mDesired, context));
    }

    void restartDoesNotAcceptOldReplies()
    {
        Tx tx;
        QVERIFY(prepare(tx));
        const auto oldId = tx.id();
        tx.cancel(oldId);
        auto other = guard();
        other.target = "another-fake-device";
        QVERIFY(tx.begin("app", mDesired, other));
        QVERIFY(tx.id() != oldId);
        QVERIFY(!tx.configReceived(oldId, guard().target, mBefore, guard()));
        QCOMPARE(tx.state(), Tx::ReadingBefore);
        QCOMPARE(tx.status().value("target").toString(), other.target);
    }

    void reentrantCancellationNeverWrites()
    {
        Tx tx;
        QVERIFY(prepare(tx));
        QSignalSpy writes(&tx, &Tx::writeRequested);
        connect(&tx, &Tx::changed, &tx, [&]() {
            if (tx.state() == Tx::AwaitingAck) {
                tx.cancel(tx.id());
            }
        });
        QVERIFY(!approve(tx));
        QCOMPARE(tx.state(), Tx::Cancelled);
        QCOMPARE(writes.count(), 0);
        QVERIFY(!tx.status().value("writeIssued").toBool());
    }

    void synchronousFakeAdapter()
    {
        Tx tx;
        connect(&tx, &Tx::readRequested, &tx, [&](const QString &id, bool verification) {
            tx.configReceived(id, guard().target, verification ? mDesired : mBefore, guard());
        });
        connect(&tx, &Tx::backupRequested, &tx, [&](const QString &id, const QJsonObject &) {
            tx.backupFinished(id, true, "fake-backup");
        });
        connect(&tx, &Tx::writeRequested, &tx, [&](const QString &id, const QJsonObject &) {
            tx.ackReceived(id, guard().target, "Motor config write OK", guard());
        });
        QVERIFY(tx.begin("motor", mDesired, guard()));
        QVERIFY(approve(tx));
        QCOMPARE(tx.state(), Tx::Succeeded);
    }

    void failedRestartPreservesPreviousResult()
    {
        Tx tx;
        QVERIFY(prepare(tx));
        tx.cancel(tx.id());
        const auto previous = tx.status();
        auto unsafe = guard();
        unsafe.target = "new-device";
        unsafe.connected = false;
        QVERIFY(!tx.begin("app", mDesired, unsafe));
        QCOMPARE(tx.status(), previous);
    }
};

QTEST_GUILESS_MAIN(TestMcpWriteTransaction)
#include "tst_mcpwritetransaction.moc"
