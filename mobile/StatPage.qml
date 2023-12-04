/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0

Item {
    property Commands mCommands: VescIf.commands()
    property bool isHorizontal: width > height

    Component.onCompleted: {
        mCommands.emitEmptyStats()
        mCommands.emitEmptySetupValues()
    }

    GridLayout {
        anchors.fill: parent
        columns: isHorizontal ? 2 : 1

        GroupBox {
            id: statBox
            title: "Since Start"
            Layout.fillWidth: true
            Layout.preferredHeight: isHorizontal ? tripBox.height : implicitHeight

            Text {
                id: statText
                anchors.fill: parent
                color: Utility.getAppHexColor("lightText")
                text: ""
                font.family: "DejaVu Sans Mono"
            }
        }

        GroupBox {
            id: tripBox
            title: "Trip"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent

                Text {
                    id: tripText
                    color: Utility.getAppHexColor("lightText")
                    text: ""
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.family: "DejaVu Sans Mono"
                }

                Button {
                    text: "Reset"
                    Layout.fillWidth: true
                    onClicked: {
                        mCommands.resetStats(false)
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.columnSpan: isHorizontal ? 2 : 1
        }
    }

    Connections {
        target: mCommands

        function onValuesSetupReceived(values, mask) {
            statText.text =
                    "Ah Drawn   : " + parseFloat(values.amp_hours * 1000).toFixed(1) + " mAh\n" +
                    "Ah Charged : " + parseFloat(values.amp_hours_charged * 1000).toFixed(1) + " mAh\n" +
                    "Wh Drawn   : " + parseFloat(values.watt_hours).toFixed(3) + " Wh\n" +
                    "Wh Charged : " + parseFloat(values.watt_hours_charged).toFixed(3) + " Wh\n" +
                    "Dist       : " + parseFloat(values.tachometer).toFixed(2) + " m\n" +
                    "Dist Abs   : " + parseFloat(values.tachometer_abs).toFixed(2) + " m\n"
        }

        function onStatsRx(val, mask) {
            var useImperial = VescIf.useImperialUnits()
            var impFact = useImperial ? 0.621371192 : 1.0
            var speedUnit = useImperial ? "mph" : "km/h"
            var efficiencyUnit = useImperial ? "Wh/mi" : "Wh/km"

            tripText.text =
                    "Time       : " + new Date(val.count_time * 1000).toISOString().substr(11, 8) + "\n" +
                    "Distance   : " + parseFloat(val.distance()).toFixed(2) + " m\n" +
                    "Efficiency : " + parseFloat(val.efficiency() / impFact).toFixed(3) + " " + efficiencyUnit + "\n" +
                    "Energy     : " + parseFloat(val.energy()).toFixed(3) + " Wh\n" +
                    "Ah         : " + parseFloat(val.ah()).toFixed(3) + " Ah\n" +
                    "Speed Avg  : " + parseFloat(val.speed_avg * 3.6 * impFact).toFixed(1) + " " + speedUnit + "\n" +
                    "Speed Max  : " + parseFloat(val.speed_max * 3.6 * impFact).toFixed(1) + " " + speedUnit + "\n" +
                    "Power Avg  : " + parseFloat(val.power_avg).toFixed(1) + " W\n" +
                    "Power Max  : " + parseFloat(val.power_max).toFixed(1) + " W\n" +
                    "T Mot Avg  : " + parseFloat(val.temp_motor_avg).toFixed(1) + " \u00B0C\n" +
                    "T Mot Max  : " + parseFloat(val.temp_motor_max).toFixed(1) + " \u00B0C\n" +
                    "T FET Avg  : " + parseFloat(val.temp_mos_avg).toFixed(1) + " \u00B0C\n" +
                    "T FET Max  : " + parseFloat(val.temp_mos_max).toFixed(1) + " \u00B0C\n" +
                    "I Avg      : " + parseFloat(val.current_avg).toFixed(1) + " A\n" +
                    "I Max      : " + parseFloat(val.current_max).toFixed(1) + " A\n"
        }
    }
}
