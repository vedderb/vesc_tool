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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    id: root
    implicitHeight: column.implicitHeight

    property real vMin: 0.0
    property real vMax: 0.0
    property real vCenter: 0.0
    property real vNow: 0.0
    property real valueNow: 0.0
    property real vMin2: 0.0
    property real vMax2: 0.0
    property real vNow2: 0.0
    property real valueNow2: 0.0
    property bool resetDone: true
    // Off where the host applies the mapping, like the setup wizard
    property bool showApply: true

    property Commands mCommands: VescIf.commands()
    property ConfigParams mAppConf: VescIf.appConfig()

    function openDialog() {
        dialog.open()
    }

    function isValid() {
        return (vMax - vMin) > 0.3
    }

    function applyMapping() {
        mAppConf.updateParamDouble("app_adc_conf.voltage_start", vMin)
        mAppConf.updateParamDouble("app_adc_conf.voltage_end", vMax)
        mAppConf.updateParamDouble("app_adc_conf.voltage_center", vCenter)
        mAppConf.updateParamDouble("app_adc_conf.voltage2_start", vMin2)
        mAppConf.updateParamDouble("app_adc_conf.voltage2_end", vMax2)
        VescIf.emitStatusMessage("Start, End and Center ADC Voltages Applied", true)
        mCommands.setAppConf()
    }

    function reset() {
        vMin = 0.0
        vMax = 0.0
        vCenter = 0.0
        vMin2 = 0.0
        vMax2 = 0.0
        resetDone = true
    }

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 12

        PpmMap.MapChannel {
            title: qsTr("ADC 1")
            unit: "V"
            // Grows when the input goes above 3.3 V
            to: Math.max(3.3, vMax)
            now: vNow
            start: vMin
            end: vMax
            center: vCenter
            output: valueNow
        }

        PpmMap.MapChannel {
            title: qsTr("ADC 2")
            unit: "V"
            to: Math.max(3.3, vMax2)
            now: vNow2
            start: vMin2
            end: vMax2
            output: valueNow2
        }

        PpmMap.MapStatus {
            valid: isValid()
            text: !isValid() ? qsTr("Move the throttle and brake through their full travel") :
                               showApply ? qsTr("Release the throttle, then apply") :
                                           qsTr("Release the throttle")
        }

        PpmMap.MapButtons {
            helpName: "app_adc_mapping_help"
            showApply: root.showApply
            canApply: isValid()
            onReset: root.reset()
            onApply: applyMapping()
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

        function onDecodedAdcReceived(value, voltage, value2, voltage2) {
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
        }
    }
}
