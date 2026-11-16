/*
    Copyright 2017 - 2019 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: rtData
    property alias updateData: commandsUpdate.enabled
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property bool isHorizontal: rtData.width > rtData.height
    // Space left above the value tiles, including their margins and the layout spacing
    property int gaugeSize: Math.min(width / 2 - 15,
                                     (height - valueGrid.implicitHeight - 50) /
                                     (isHorizontal ? 1 : 2) - 5)
    property var tileValues: []

    Component.onCompleted: {
        currentGauge.minimumValue = -mMcConf.getParamDouble("l_current_max")
        currentGauge.maximumValue = mMcConf.getParamDouble("l_current_max")
        mCommands.emitEmptyValues()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 5

        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            columns: isHorizontal ? 4 : 2
            Layout.leftMargin: 5
            Layout.rightMargin: 5
            Layout.topMargin: 5
            Layout.bottomMargin: 5
            CustomGauge {
                id: currentGauge
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                maximumValue: 100
                minimumValue: -100
                labelStep: maximumValue > 60 ? 20 : 10
                value: 0
                unitText: "A"
                typeText: "Current"
                Layout.preferredWidth: gaugeSize
                Layout.preferredHeight: gaugeSize
                Layout.maximumWidth: gaugeSize
            }

            CustomGauge {
                id: dutyGauge
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                maximumValue: 100
                minimumValue: -100
                labelStep: 20
                value: 0
                unitText: "%"
                typeText: "Duty"
                Layout.preferredWidth: gaugeSize
                Layout.preferredHeight: gaugeSize
                Layout.maximumWidth: gaugeSize
            }

            CustomGauge {
                id: rpmGauge
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                maximumValue: 100
                minimumValue: -100
                labelStep: 20
                value: 0
                unitText: "x1000"
                typeText: "ERPM"
                Layout.preferredWidth: gaugeSize
                Layout.preferredHeight: gaugeSize
                Layout.maximumWidth: gaugeSize
            }

            CustomGauge {
                id: powerGauge
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                maximumValue: 10000
                minimumValue: -10000
                tickmarkScale: 0.001
                tickmarkSuffix: "k"
                labelStep: 2000
                value: 0
                unitText: "W"
                typeText: "Power"
                Layout.preferredWidth: gaugeSize
                Layout.preferredHeight: gaugeSize
                Layout.maximumWidth: gaugeSize
            }
        }

        GridLayout {
            id: valueGrid
            Layout.fillWidth: true
            Layout.margins: 10
            columns: isHorizontal ? 5 : 2
            rowSpacing: 6
            columnSpacing: 6

            Repeater {
                model: ["Battery", "I Battery", "Temp MOS", "Temp Motor", "Ah Draw",
                    "Ah Charge", "Wh Draw", "Wh Charge", "ABS Tacho", "Fault"]

                ValueTile {
                    readonly property var entry: tileValues[index] || ["", "", -1]
                    Layout.fillWidth: true
                    implicitHeight: 44
                    title: modelData
                    value: entry[0]
                    unit: entry[1]
                    level: entry[2]
                    levelColor: Utility.getAppHexColor(index === 0 ? "positiveText" : level < 0.6 ? "infoText" :
                                                                                          level < 0.85 ? "warning" : "danger")
                    valueColor: Utility.getAppHexColor(index !== 9 ? "titleText" : value === "None" ? "positiveText" : "dangerText")
                }
            }
        }
    }

    Connections {
        target: mMcConf

        function onUpdated() {
            var currentMaxRound = Math.ceil(mMcConf.getParamDouble("l_current_max") / 5) * 5

            if (currentMaxRound > currentGauge.maximumValue || currentMaxRound < (currentGauge.maximumValue * 0.7)) {
                currentGauge.maximumValue = currentMaxRound
                currentGauge.minimumValue = -currentMaxRound
            }

            currentGauge.labelStep = Math.ceil(currentMaxRound / 20) * 5
        }
    }

    Connections {
        id: commandsUpdate
        target: mCommands

        function onValuesReceived(values, mask) {
            currentGauge.value = values.current_motor
            dutyGauge.value = values.duty_now * 100.0

            var fl = mMcConf.getParamDouble("foc_motor_flux_linkage")
            var rpmMax = (values.v_in * 60.0) / (Math.sqrt(3.0) * 2.0 * Math.PI * fl)
            var rpmMaxRound = (Math.ceil(rpmMax / 5000.0) * 5000.0) / 1000

            if (rpmMaxRound > rpmGauge.maximumValue || rpmMaxRound < (rpmGauge.maximumValue * 0.6)) {
                rpmGauge.maximumValue = rpmMaxRound
                rpmGauge.minimumValue = -rpmMaxRound
            }

            rpmGauge.value = values.rpm / 1000

            var powerMax = Math.min(values.v_in * Math.min(mMcConf.getParamDouble("l_in_current_max"),
                                                           mMcConf.getParamDouble("l_current_max")),
                                    mMcConf.getParamDouble("l_watt_max"))
            var powerMaxRound = (Math.ceil(powerMax / 1000.0) * 1000.0)

            if (powerMaxRound > powerGauge.maximumValue || powerMaxRound < (powerGauge.maximumValue * 0.6)) {
                powerGauge.maximumValue = powerMaxRound
                powerGauge.minimumValue = -powerMaxRound
            }

            powerGauge.value = (values.current_in * values.v_in)

            tileValues = [
                        [parseFloat(values.v_in).toFixed(2), "V", -1],
                        [parseFloat(values.current_in).toFixed(2), "A", -1],
                        [parseFloat(values.temp_mos).toFixed(1), "\u00B0C",
                         values.temp_mos / mMcConf.getParamDouble("l_temp_fet_end")],
                        [parseFloat(values.temp_motor).toFixed(1), "\u00B0C",
                         values.temp_motor / mMcConf.getParamDouble("l_temp_motor_end")],
                        [parseFloat(values.amp_hours * 1000.0).toFixed(1), "mAh", -1],
                        [parseFloat(values.amp_hours_charged * 1000.0).toFixed(1), "mAh", -1],
                        [parseFloat(values.watt_hours).toFixed(2), "Wh", -1],
                        [parseFloat(values.watt_hours_charged).toFixed(2), "Wh", -1],
                        [values.tachometer_abs, "Counts", -1],
                        [values.fault_code === 0 ? "None" : values.fault_str.replace("FAULT_CODE_", ""), "", -1]
                    ]
        }
    }
}
