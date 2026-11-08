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
#include "commands.h"
#include "vescinterface.h"
#include "utility.h"
#include "mcp/vescmcpdevice.h"

class TestMcpApplication : public QObject
{
    Q_OBJECT
  private slots:
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
};
QTEST_GUILESS_MAIN(TestMcpApplication)
#include "tst_mcpapplication.moc"
