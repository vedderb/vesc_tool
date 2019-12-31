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

import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    id: rtData
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property bool isHorizontal: rtData.width > rtData.height
    property int gaugeSize: Math.min(width / 2 - 10,
                                     (height - valMetrics.height * 10) /
                                     (isHorizontal ? 1 : 2) - (isHorizontal ? 30 : 20))

    Component.onCompleted: {
        currentGauge.minimumValue = -mMcConf.getParamDouble("l_current_max")
        currentGauge.maximumValue = mMcConf.getParamDouble("l_current_max")
        mCommands.emitEmptyValues()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 5

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: isHorizontal ? 4 : 2

            CustomGauge {
                id: currentGauge
                Layout.fillWidth: true
                maximumValue: 100
                minimumValue: -100
                labelStep: maximumValue > 60 ? 20 : 10
                value: 0
                unitText: "A"
                typeText: "Current"
                Layout.preferredWidth: gaugeSize
                Layout.preferredHeight: gaugeSize
            }

            CustomGauge {
                id: dutyGauge
                Layout.fillWidth: true
                maximumValue: 100
                minimumValue: -100
                labelStep: 20
                value: 0
                unitText: "%"
                typeText: "Duty"
                Layout.preferredWidth: gaugeSize
                Layout.preferredHeight: gaugeSize
            }

            CustomGauge {
                id: rpmGauge
                Layout.fillWidth: true
                maximumValue: 100
                minimumValue: -100
                labelStep: 20
                value: -10
                unitText: "x1000"
                typeText: "ERPM"
                Layout.preferredWidth: gaugeSize
                Layout.preferredHeight: gaugeSize
            }

            CustomGauge {
                id: powerGauge
                Layout.fillWidth: true
                maximumValue: 10000
                minimumValue: -10000
                tickmarkScale: 0.001
                tickmarkSuffix: "k"
                labelStep: 1000
                value: 0
                unitText: "W"
                typeText: "Power"
                Layout.preferredWidth: gaugeSize
                Layout.preferredHeight: gaugeSize
            }
        }

        Rectangle {
            id: textRect
            color: "#272727"

            Rectangle {
                anchors.bottom: valText.top
                width: parent.width
                height: 2
                color: "#81D4FA"
            }

            Layout.fillWidth: true
            Layout.preferredHeight: valMetrics.height * 10 + 20
            Layout.alignment: Qt.AlignBottom

            Text {
                id: valText
                color: "white"
                text: VescIf.getConnectedPortName()
                font.family: "DejaVu Sans Mono"
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.topMargin: 5
            }

            TextMetrics {
                id: valMetrics
                font: valText.font
                text: valText.text
            }
        }
    }

    Connections {
        target: mMcConf

        onUpdated: {
            currentGauge.maximumValue = Math.ceil(mMcConf.getParamDouble("l_current_max") / 5) * 5
            currentGauge.minimumValue = -currentGauge.maximumValue
        }
    }

    Connections {
        target: mCommands

        onValuesReceived: {
            currentGauge.value = values.current_motor
            dutyGauge.value = values.duty_now * 100.0

            var fl = mMcConf.getParamDouble("foc_motor_flux_linkage")
            var rpmMax = (values.v_in * 60.0) / (Math.sqrt(3.0) * 2.0 * Math.PI * fl)
            var rpmMaxRound = (Math.ceil(rpmMax / 5000.0) * 5000.0) / 1000

            if (Math.abs(rpmGauge.maximumValue - rpmMaxRound) > 6) {
                rpmGauge.maximumValue = rpmMaxRound
                rpmGauge.minimumValue = -rpmMaxRound
            }

            rpmGauge.value = values.rpm / 1000

            var powerMax = Math.min(values.v_in * Math.min(mMcConf.getParamDouble("l_in_current_max"),
                                                           mMcConf.getParamDouble("l_current_max")),
                                    mMcConf.getParamDouble("l_watt_max"))
            var powerMaxRound = (Math.ceil(powerMax / 1000.0) * 1000.0)

            if (Math.abs(powerGauge.maximumValue - powerMaxRound) > 1.2) {
                powerGauge.maximumValue = powerMaxRound
                powerGauge.minimumValue = -powerMaxRound
            }

            powerGauge.value = (values.current_in * values.v_in)

            valText.text =
                    "Battery    : " + parseFloat(values.v_in).toFixed(2) + " V\n" +
                    "I Battery  : " + parseFloat(values.current_in).toFixed(2) + " A\n" +
                    "Temp MOS   : " + parseFloat(values.temp_mos).toFixed(2) + " \u00B0C\n" +
                    "Temp Motor : " + parseFloat(values.temp_motor).toFixed(2) + " \u00B0C\n" +
                    "Ah Draw    : " + parseFloat(values.amp_hours * 1000.0).toFixed(1) + " mAh\n" +
                    "Ah Charge  : " + parseFloat(values.amp_hours_charged * 1000.0).toFixed(1) + " mAh\n" +
                    "Wh Draw    : " + parseFloat(values.watt_hours).toFixed(2) + " Wh\n" +
                    "Wh Charge  : " + parseFloat(values.watt_hours_charged).toFixed(2) + " Wh\n" +
                    "ABS Tacho  : " + values.tachometer_abs + " Counts\n" +
                    "Fault      : " + values.fault_str
        }
    }
}
