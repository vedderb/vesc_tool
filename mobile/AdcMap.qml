/*
    Copyright 2018 - 2019 Benjamin Vedder	benjamin@vedder.se

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

Item {
    implicitHeight: column.implicitHeight

    property real vMin: 0.0
    property real vMax: 0.0
    property real vCenter: 0.0
    property real vNow: 0.0
    property real valueNow: 0.5
    property real vMin2: 0.0
    property real vMax2: 0.0
    property real vNow2: 0.0
    property real valueNow2: 0.5
    property bool resetDone: true

    property Commands mCommands: VescIf.commands()
    property ConfigParams mAppConf: VescIf.appConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()

    function openDialog() {
        dialog.open()
    }

    function updateDisplay() {
        resultArea.text =
                "Val: " + parseFloat(valueNow).toFixed(2) + "    Val2: " + parseFloat(valueNow2).toFixed(2) + "\n" +
                "Now: " + parseFloat(vNow).toFixed(2) +     " V  Now2: " + parseFloat(vNow2).toFixed(2) +     " V\n" +
                "Min: " + parseFloat(vMin).toFixed(2) +     " V  Min2: " + parseFloat(vMin2).toFixed(2) +     " V\n" +
                "Max: " + parseFloat(vMax).toFixed(2) +     " V  Max2: " + parseFloat(vMax2).toFixed(2) +     " V\n" +
                "Ctr: " + parseFloat(vCenter).toFixed(2) +  " V"
        valueBar.value = valueNow
        valueBar2.value = valueNow2
    }

    function reset() {
        vMin = 0.0
        vMax = 0.0
        vCenter = 0.0
        vMin2 = 0.0
        vMax2 = 0.0
        resetDone = true
        updateDisplay()
    }

    Component.onCompleted: {
        updateDisplay()
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 0

        TextArea {
            id: resultArea
            Layout.fillWidth: true
            readOnly: true
            wrapMode: TextEdit.WordWrap
            font.family: "DejaVu Sans Mono"
        }

        ProgressBar {
            id: valueBar
            Layout.fillWidth: true
            Layout.bottomMargin: 5
            from: 0.0
            to: 1.0
            value: 0.0
        }

        ProgressBar {
            id: valueBar2
            Layout.fillWidth: true
            from: 0.0
            to: 1.0
            value: 0.0
        }

        RowLayout {
            Layout.fillWidth: true
            Button {
                text: "Help"
                Layout.preferredWidth: 50
                Layout.fillWidth: true
                flat: true
                onClicked: {
                    VescIf.emitMessageDialog(
                                mInfoConf.getLongName("app_adc_mapping_help"),
                                mInfoConf.getDescription("app_adc_mapping_help"),
                                true, true)
                }
            }

            Button {
                text: "Reset"
                Layout.preferredWidth: 50
                Layout.fillWidth: true
                flat: true
                onClicked: {
                    reset()
                }
            }
        }

        Button {
            text: "Apply and Write"
            Layout.fillWidth: true
            flat: true
            onClicked: {
                mAppConf.updateParamDouble("app_adc_conf.voltage_start", vMin)
                mAppConf.updateParamDouble("app_adc_conf.voltage_end", vMax)
                mAppConf.updateParamDouble("app_adc_conf.voltage_center", vCenter)
                mAppConf.updateParamDouble("app_adc_conf.voltage2_start", vMin2)
                mAppConf.updateParamDouble("app_adc_conf.voltage2_end", vMax2)
                VescIf.emitStatusMessage("Start, End and Center ADC Voltages Applied", true)
                mCommands.setAppConf()
            }
        }
    }

    Timer {
        id: rtTimer
        interval: 50
        running: true
        repeat: true

        onTriggered: {
            if (VescIf.isPortConnected() && visible) {
                mCommands.getDecodedAdc()
            }
        }
    }

    Connections {
        target: mCommands

        onDecodedAdcReceived: {
            valueNow = value
            vNow = voltage
            valueNow2 = value2
            vNow2 = voltage2

            if (resetDone) {
                resetDone = false
                vMin = vNow
                vMax = vNow
                vMin2 = vNow2
                vMax2 = vNow2
            }

            if (vNow < vMin) {
                vMin = vNow
            }

            if (vNow > vMax) {
                vMax = vNow
            }

            var range = vMax - vMin
            var pos = vNow - vMin

            if (pos > (range / 4.0) && pos < ((3.0 * range) / 4.0)) {
                vCenter = vNow
            } else {
                vCenter = range / 2.0 + vMin
            }

            if (vNow2 < vMin2) {
                vMin2 = vNow2
            }

            if (vNow2 > vMax2) {
                vMax2 = vNow2
            }

            updateDisplay()
        }
    }
}
