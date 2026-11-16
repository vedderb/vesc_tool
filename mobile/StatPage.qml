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
    property var startValues: []
    property var tripValues: []
    property string tripTime: ""

    Component.onCompleted: {
        mCommands.emitEmptyStats()
        mCommands.emitEmptySetupValues()
    }

    component TileGrid: GridLayout {
        id: tileGrid
        property var titles: []
        property var values: []

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        columns: 2
        rowSpacing: 6
        columnSpacing: 6

        Repeater {
            model: tileGrid.titles

            ValueTile {
                Layout.fillWidth: true
                implicitHeight: 48
                title: modelData
                value: tileGrid.values[index] ? tileGrid.values[index][0] : ""
                unit: tileGrid.values[index] ? tileGrid.values[index][1] : ""
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: grid.height + 20
        clip: true

        GridLayout {
            id: grid
            x: 10
            width: parent.width - 20
            columns: isHorizontal ? 2 : 1
            columnSpacing: 16

            ColumnLayout {
                Layout.alignment: Qt.AlignTop

                SectionHeader {
                    text: "Since Start"
                }

                TileGrid {
                    titles: ["Ah Drawn", "Ah Charged", "Wh Drawn", "Wh Charged", "Dist", "Dist Abs"]
                    values: startValues
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignTop

                SectionHeader {
                    text: "Trip"

                    Text {
                        text: tripTime
                        color: Utility.getAppHexColor("lightText")
                        font.features: { "tnum": 1 }
                    }
                }

                TileGrid {
                    titles: ["Distance", "Efficiency", "Energy", "Ah", "Speed Avg / Max",
                        "Power Avg / Max", "T Mot Avg / Max", "T FET Avg / Max", "I Avg / Max"]
                    values: tripValues
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
    }

    Connections {
        target: mCommands

        function onValuesSetupReceived(values, mask) {
            if ((mask & 0x7E00) === 0) {
                return
            }

            var useImperial = VescIf.useImperialUnits()
            var impFact = useImperial ? 0.621371192 : 1.0
            var distUnit = useImperial ? "mi" : "km"

            startValues = [
                        [parseFloat(values.amp_hours * 1000).toFixed(1), "mAh"],
                        [parseFloat(values.amp_hours_charged * 1000).toFixed(1), "mAh"],
                        [parseFloat(values.watt_hours).toFixed(3), "Wh"],
                        [parseFloat(values.watt_hours_charged).toFixed(3), "Wh"],
                        [parseFloat(values.tachometer / 1000 * impFact).toFixed(2), distUnit],
                        [parseFloat(values.tachometer_abs / 1000 * impFact).toFixed(2), distUnit]
                    ]
        }

        function onStatsRx(val, mask) {
            var useImperial = VescIf.useImperialUnits()
            var impFact = useImperial ? 0.621371192 : 1.0
            var speedUnit = useImperial ? "mph" : "km/h"
            var efficiencyUnit = useImperial ? "Wh/mi" : "Wh/km"
            var distUnit = useImperial ? "mi" : "km"

            tripTime = new Date(val.count_time * 1000).toISOString().substr(11, 8)
            tripValues = [
                        [parseFloat(val.distance() / 1000 * impFact).toFixed(2), distUnit],
                        [parseFloat(val.efficiency() / impFact).toFixed(1), efficiencyUnit],
                        [parseFloat(val.energy()).toFixed(1), "Wh"],
                        [parseFloat(val.ah()).toFixed(2), "Ah"],
                        [parseFloat(val.speed_avg * 3.6 * impFact).toFixed(1) + " / " +
                         parseFloat(val.speed_max * 3.6 * impFact).toFixed(1), speedUnit],
                        [parseFloat(val.power_avg).toFixed(0) + " / " + parseFloat(val.power_max).toFixed(0), "W"],
                        [parseFloat(val.temp_motor_avg).toFixed(1) + " / " + parseFloat(val.temp_motor_max).toFixed(1), "\u00B0C"],
                        [parseFloat(val.temp_mos_avg).toFixed(1) + " / " + parseFloat(val.temp_mos_max).toFixed(1), "\u00B0C"],
                        [parseFloat(val.current_avg).toFixed(1) + " / " + parseFloat(val.current_max).toFixed(1), "A"]
                    ]
        }
    }
}
