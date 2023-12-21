/*
    Copyright 2020 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0

import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import "qrc:/mobile"

Item {
    id: mainItem
    anchors.fill: parent

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()

    ColumnLayout {
        id: gaugeColumn
        anchors.fill: parent
        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            CustomGaugeV2 {
                id: rpmGauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 2000
                minimumValue: 0
                tickmarkScale: 0.001
                tickmarkSuffix: "k"
                labelStep: 500
                value: 0
                unitText: "RPM"
                typeText: "Speed"
            }

            CustomGaugeV2 {
                id: powerGauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 30000
                minimumValue: -30000
                tickmarkScale: 0.001
                tickmarkSuffix: "k"
                labelStep: 5000
                value: 1000
                unitText: "W"
                typeText: "Power"
            }
        }
        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            CustomGaugeV2 {
                id: currentGauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 200
                minimumValue: -200
                tickmarkScale: 1
                tickmarkSuffix: ""
                labelStep: 100
                value: 0
                unitText: "A"
                typeText: "Current"
            }

            CustomGaugeV2 {
                id: voltageGauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 300
                minimumValue: 0
                tickmarkScale: 1
                tickmarkSuffix: ""
                labelStep: 20
                value: 0
                unitText: "V"
                typeText: "Battery\nVoltage"
            }
        }

        Connections {
            target: mCommands

            function onValuesReceived(values, mask) {
                rpmGauge.value = Math.abs(values.rpm) / 32
                powerGauge.value = values.current_in * values.v_in
                currentGauge.value = values.current_motor
                voltageGauge.value = values.v_in
            }
        }
    }
}
