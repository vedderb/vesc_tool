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
#include <QProgressDialog>
#include "mainwindow.h"
#include "commands.h"
#include "vescinterface.h"
#include "utility.h"
#include "mcp/vescmcpdevice.h"
#include "mcp/mcpdatahelpers.h"

class TestMcpApplication : public QObject
{
    Q_OBJECT
  private slots:
    void rtPacketPresenceAndSelectiveDqVoltage()
    {
        Commands commands;
        QList<QJsonObject> samples;
        QList<quint32> masks;
        connect(&commands, &Commands::valuesReceived, &commands,
                [&](MC_VALUES values, unsigned int mask) {
                    masks.append(mask);
                    samples.append(McpDataHelpers::rtValues(values, mask));
                });
        VByteArray base;
        base.vbAppendUint8(COMM_GET_VALUES);
        base.append(QByteArray(53, '\0'));
        commands.processPacket(base);
        QCOMPARE(samples.size(), 1);
        QCOMPARE(masks.last(), quint32(0xffff));
        QVERIFY(samples.last().contains("current_d_a"));
        QVERIFY(samples.last().contains("watt_hours_wh"));
        QVERIFY(!samples.last().contains("pid_position_deg"));
        QVERIFY(!samples.last().contains("controller_id"));
        QVERIFY(!samples.last().contains("temp_mos_1_c"));
        QVERIFY(!samples.last().contains("voltage_d_v"));
        QVERIFY(!samples.last().contains("has_timeout"));
        commands.processPacket(base.left(base.size() - 1));
        QCOMPARE(samples.size(), 1); // Truncated mandatory data must not become zeros.
        for (int bit : {19, 20}) {
            VByteArray packet;
            packet.vbAppendUint8(COMM_GET_VALUES_SELECTIVE);
            packet.vbAppendUint32(quint32(1) << bit);
            packet.vbAppendDouble32(12.345, 1e3);
            commands.processPacket(packet);
            QCOMPARE(masks.last(), quint32(1) << bit);
            QCOMPARE(samples.last().size(), 1);
            QCOMPARE(samples.last().value(bit == 19 ? "voltage_d_v" : "voltage_q_v").toDouble(),
                     12.345);
        }
        VByteArray full = base;
        full.vbAppendDouble32(123.0, 1e6);
        full.vbAppendUint8(7);
        full.vbAppendDouble16(21.0, 1e1);
        full.vbAppendDouble16(22.0, 1e1);
        full.vbAppendDouble16(23.0, 1e1);
        full.vbAppendDouble32(-1.234, 1e3);
        full.vbAppendDouble32(2.345, 1e3);
        full.vbAppendUint8(3);
        commands.processPacket(full);
        QCOMPARE(masks.last(), (quint32(1) << 22) - 1);
        QCOMPARE(samples.last().size(), 26);
        QCOMPARE(samples.last().value("pid_position_deg").toDouble(), 123.0);
        QCOMPARE(samples.last().value("controller_id").toInt(), 7);
        QCOMPARE(samples.last().value("temp_mos_3_c").toDouble(), 23.0);
        QVERIFY(samples.last().value("has_timeout").toBool());
        QVERIFY(samples.last().value("kill_switch_active").toBool());
    }
    void idleMcpServerDoesNotShowProgressDialog()
    {
        MainWindow window;
        // Isolate MCP from unrelated startup wizards and network/version checks.
        // MCP's own timers are created by startMcpServer below and remain active.
        for (auto timer : window.findChildren<QTimer *>()) {
            timer->stop();
        }
        for (auto device : window.findChildren<VescInterface *>()) {
            device->setIgnoreTestVersion(true);
        }
        QVERIFY(window.startMcpServer(0, QByteArray(64, 't'), true, true));
        window.show();
        QTest::qWait(4500); // Includes QProgressDialog's initial four-second force-show timer.
        int dialogs = 0;
        for (auto dialog : window.findChildren<QProgressDialog *>()) {
            if (dialog->labelText().contains("MCP")) {
                dialogs++;
                QVERIFY(!dialog->isVisible());
            }
        }
        QCOMPARE(dialogs, 1);
    }
    void exclusiveCommandsBlockOtherCallers()
    {
        Commands commands;
        QObject owner, other;
        int packets = 0;
        QByteArray last;
        connect(&commands, &Commands::dataToSend, &commands, [&](QByteArray &data) {
            packets++;
            last = data;
        });
        commands.setDutyCycle(0.0); // Observed locally only; no Packet/physical connection exists.
        QCOMPARE(packets, 1);
        QVERIFY(commands.acquireExclusive(&owner));
        QVERIFY(!commands.acquireExclusive(&other));
        commands.setDutyCycle(0.0);
        QCOMPARE(packets, 1);
        QVERIFY(!commands.setSendCan(true, 2));
        QCOMPARE(commands.getCanSendId(), -1);
        commands.setCanSendId(3);
        QCOMPARE(commands.getCanSendId(), -1);
        QVERIFY(!commands.sendExclusive(&other, QByteArray(1, char(COMM_GET_VALUES))));
        QVERIFY(commands.sendExclusive(&owner, QByteArray(1, char(COMM_GET_VALUES))));
        QCOMPARE(packets, 2);
        QCOMPARE(int(static_cast<unsigned char>(last.at(0))), int(COMM_GET_VALUES));
        commands.releaseExclusive(&other);
        QVERIFY(commands.hasExclusiveOwner());
        commands.releaseExclusive(&owner);
        QVERIFY(!commands.hasExclusiveOwner());
        commands.setDutyCycle(0.0);
        QCOMPARE(packets, 3);
    }
    void reentrantCommandsCannotBypassOwner()
    {
        Commands commands;
        QObject owner;
        int packets = 0;
        QVERIFY(commands.acquireExclusive(&owner));
        connect(&commands, &Commands::dataToSend, &commands, [&](QByteArray &) {
            packets++;
            commands.setDutyCycle(0.0);
        });
        QVERIFY(commands.sendExclusive(&owner, QByteArray(1, char(COMM_GET_VALUES))));
        QCOMPARE(packets, 1);
    }
    void ownerDestructionReleasesChannel()
    {
        Commands commands;
        auto owner = new QObject;
        QVERIFY(commands.acquireExclusive(owner));
        delete owner;
        QVERIFY(!commands.hasExclusiveOwner());
        QVERIFY(!commands.sendExclusive(nullptr, QByteArray(1, char(COMM_GET_VALUES))));
    }
    void limitedModeRefusesExclusiveWrites()
    {
        Commands commands;
        QObject owner;
        QVERIFY(commands.acquireExclusive(&owner));
        commands.setLimitedMode(true);
        QVERIFY(!commands.sendExclusive(&owner, QByteArray(1, char(COMM_SET_APPCONF))));
    }
    void limitedModeHonorsCompatibilityAllowlist()
    {
        Commands commands;
        QObject owner, other;
        int packets = 0;
        connect(&commands, &Commands::dataToSend, &commands, [&](QByteArray &) { packets++; });
        QVERIFY(commands.acquireExclusive(&owner));
        commands.setLimitedMode(true);
        commands.setLimitedCompatibilityCommands(
            {COMM_GET_VALUES, COMM_GET_APPCONF, COMM_SET_APPCONF, COMM_LISP_READ_CODE});
        for (int command : {int(COMM_FW_VERSION), int(COMM_GET_VALUES), int(COMM_GET_APPCONF),
                            int(COMM_SET_APPCONF), int(COMM_LISP_READ_CODE)}) {
            QVERIFY(commands.sendExclusive(&owner, QByteArray(1, char(command))));
        }
        QCOMPARE(packets, 5);
        QVERIFY(commands.isLimitedMode());
        QVERIFY(!commands.sendExclusive(&other, QByteArray(1, char(COMM_GET_VALUES))));
        QVERIFY(!commands.sendExclusive(&owner, QByteArray(1, char(COMM_SET_MCCONF))));
        QVERIFY(!commands.sendExclusive(&owner, QByteArray(1, char(COMM_LISP_ERASE_CODE))));
        QCOMPARE(packets, 5);
        commands.setLimitedCompatibilityCommands({});
        QVERIFY(!commands.sendExclusive(&owner, QByteArray(1, char(COMM_GET_VALUES))));
    }
    void emergencyStopCanInterruptOwnership()
    {
        Commands commands;
        QObject owner;
        int packets = 0, interruptions = 0;
        QVERIFY(commands.acquireExclusive(&owner));
        connect(&commands, &Commands::exclusiveInterrupted, &commands, [&]() {
            interruptions++;
            commands.releaseExclusive(&owner);
        });
        connect(&commands, &Commands::dataToSend, &commands, [&](QByteArray &) { packets++; });
        commands.setCurrent(0.0);
        QCOMPARE(interruptions, 1);
        QCOMPARE(packets, 1);
        QVERIFY(!commands.hasExclusiveOwner());
        QVERIFY(commands.acquireExclusive(&owner));
        commands.motorEstop(5000);
        QCOMPARE(interruptions, 2);
        QCOMPARE(packets, 2);
    }
    void realParameterCodecNormalizesWithoutHardware()
    {
        VescInterface vesc;
        QVERIFY(!vesc.isPortConnected());
        QVERIFY(Utility::configLoadLatest(&vesc));
        VescMcpDevice device(&vesc);
        int packets = 0;
        connect(vesc.commands(), &Commands::dataToSend, &vesc, [&](QByteArray &) { packets++; });
        for (const auto &kind : QStringList{"motor", "app"}) {
            QString error;
            const auto canonical = device.canonicalValues(kind, &error);
            QVERIFY2(!canonical.isEmpty(), qPrintable(error));
            VByteArray wire(device.configWire(kind));
            auto params = kind == "motor" ? vesc.mcConfig() : vesc.appConfig();
            QVERIFY(params->deSerialize(wire));
            QCOMPARE(device.configValues(kind), canonical);
            QVERIFY(device.validateChanges(kind, canonical, &error));
        }
        auto invalid = device.configValues("app");
        invalid.insert("controller_id", -1);
        QString error;
        QVERIFY(!device.validateChanges("app", invalid, &error));
        QVERIFY(!device.ready());
        QCOMPARE(packets, 0);
    }
    void firmware606SchemaIsAvailable()
    {
        VescInterface vesc;
        QVERIFY(Utility::configLoad(&vesc, 6, 6));
        QVERIFY(vesc.getSupportedFirmwarePairs().contains(qMakePair(6, 6)));
        VescMcpDevice device(&vesc);
        vesc.appConfig()->updateParamEnum("app_to_use", 5);
        QCOMPARE(device.inputMode(), QString("adc_uart"));
        for (const auto &kind : QStringList{"motor", "app"}) {
            QString error;
            QVERIFY2(!device.canonicalValues(kind, &error).isEmpty(), qPrintable(error));
        }
        QVERIFY(!device.ready()); // Loading a schema cannot fabricate a connection.
    }
};
QTEST_MAIN(TestMcpApplication)
#include "tst_mcpapplication.moc"
