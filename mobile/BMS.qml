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
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0

Item {
    id: bmsPageItem
    property Commands mCommands: VescIf.commands()
    property var mVal
    property bool mValSet: false
    property bool isHorizontal: width > height

    GridLayout {
        anchors.fill: parent
        columns: isHorizontal ? 2 : 1
        rows: isHorizontal ? 1 : 2
        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.column: isHorizontal ? 1 : 0
            Layout.row: 0
            Layout.preferredWidth: isHorizontal ? parent.width/2 : parent.width
            Layout.preferredHeight: parent.height*2/3
            spacing: 0

            Rectangle {
                color: Utility.getAppHexColor("lightestBackground")
                width: 16
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter |  Qt.AlignVCenter

                PageIndicator {
                    id: indicator
                    count: swipeView.count
                    currentIndex: swipeView.currentIndex
                    anchors.centerIn: parent
                    rotation: 90
                }
            }

            SwipeView {
                id: swipeView
                enabled: true
                clip: true

                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Vertical

                Page {
                    Canvas {
                        id: cellCanvas
                        anchors.fill: parent

                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.reset()

                            var xOfsLeft = valMetrics.width * 4.5
                            var xOfsRight = valMetrics.width * 8
                            var heightBars = (height - valMetrics.height * 1.2)
                            var cellWidth = width - xOfsLeft - xOfsRight

                            ctx.font = '%1pt %2'.arg(valText.font.pointSize).arg(valText.font.family)

                            var lineDivs = 6
                            var lineStep = cellWidth / lineDivs

                            ctx.beginPath()

                            for (var i = 0; i <= lineDivs;i++) {
                                ctx.moveTo(xOfsLeft + lineStep * i, 0)
                                ctx.lineTo(xOfsLeft + lineStep * i, heightBars)
                            }

                            ctx.strokeStyle = Utility.getAppHexColor("lightAccent")
                            ctx.lineWidth = 1.2
                            ctx.stroke()

                            ctx.fillStyle = Utility.getAppHexColor("lightAccent")
                            ctx.textBaseline = "top"

                            ctx.textAlign = "start";
                            ctx.fillText("V", valMetrics.width / 2, heightBars)
                            ctx.textAlign = "center"

                            for (i = 0; i <= lineDivs;i++) {
                                ctx.fillText(parseFloat(3 + i * (1.2 / lineDivs)).toFixed(1), xOfsLeft + lineStep * i, heightBars)
                            }

                            if (!mValSet) {
                                return
                            }

                            var vCells = mVal.v_cells
                            var isBal = mVal.is_balancing

                            //                            var vCells = [3.1, 2.7, 3.3, 3.4, 3.5, 3.4, 3.3, 4.4, 4.1, 4.2]
                            //                            var isBal = [false, false, false, false, true, true, false, false, false, true]

                            var cellNum = vCells.length
                            var cellHeight = (heightBars / cellNum) * 0.6
                            var cellDist = (heightBars / cellNum) * 0.4

                            ctx.textAlign = "start"
                            ctx.textBaseline = "middle"

                            for (i = 0;i < cellNum;i++) {
                                ctx.fillStyle = Utility.getAppHexColor("lightAccent")

                                var cellW = ((vCells[i] - 3.0) / 1.2) * cellWidth
                                if (cellW > cellWidth) {
                                    cellW = cellWidth
                                }
                                if (cellW < 1.2) {
                                    cellW = 1.2
                                }

                                var txtY = i * (cellHeight + cellDist) + cellDist / 2 + cellHeight / 2

                                ctx.fillText("C" + parseFloat(i + 1).toFixed(0),
                                             valMetrics.width / 2, txtY)
                                ctx.fillText(parseFloat(vCells[i]).toFixed(3) + " V",
                                             xOfsLeft + cellWidth + valMetrics.width / 2, txtY)

                                if (isBal[i]) {
                                    ctx.fillStyle = Utility.getAppHexColor("orange")
                                } else {
                                    ctx.fillStyle = Qt.rgba(0, 0.9, 0, 1)
                                }

                                ctx.fillRect(xOfsLeft, i * (cellHeight + cellDist) + cellDist / 2,
                                             cellW, cellHeight)
                            }
                        }
                    }
                }

                Page {
                    Canvas {
                        id: tempCanvas
                        anchors.fill: parent

                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.reset()

                            if (!mValSet) {
                                return
                            }

                            var temps =  [mVal.temp_ic, mVal.temp_hum_sensor].concat(mVal.temps)
                            var tempNum = temps.length

                            var xOfsLeft = valMetrics.width * 4.5
                            var xOfsRight = valMetrics.width * 8

                            var cellHeight = (height / tempNum) * 0.7
                            var cellDist = (height / tempNum) * 0.3
                            var cellWidth = width - xOfsLeft - xOfsRight

                            for (var i = 0;i < tempNum;i++) {
                                ctx.fillStyle = Utility.getAppHexColor("lightAccent")
                                ctx.textAlign = "start";
                                ctx.textBaseline = "middle";
                                ctx.font = '%1pt %2'.arg(valText.font.pointSize).arg(valText.font.family)

                                var cellW = ((temps[i] + 10) / 70) * cellWidth
                                if (cellW > cellWidth) {
                                    cellW = cellWidth
                                }
                                if (cellW < 0) {
                                    cellW = 0
                                }

                                var txtY = i * (cellHeight + cellDist) + cellDist / 2 + cellHeight / 2

                                ctx.fillText("T" + parseFloat(i + 1).toFixed(0),
                                             valMetrics.width / 2, txtY)
                                ctx.fillText(parseFloat(temps[i]).toFixed(2) + " °C",
                                             xOfsLeft + cellW + valMetrics.width / 2, txtY)

                                ctx.fillStyle = Qt.rgba(0, 0, 1, 1)
                                ctx.fillRect(xOfsLeft, i * (cellHeight + cellDist) + cellDist / 2,
                                             cellW, cellHeight)
                            }
                        }
                    }
                }

                Page {

                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: isHorizontal ? parent.width/2 : parent.width
            Layout.column:0
            Layout.row: isHorizontal ? 0 : 1
            Layout.preferredHeight: valMetrics.height * 12 + 20 + menuButton.implicitHeight
            spacing: 0
            Rectangle {
                id: textRect
                color: Utility.getAppHexColor("darkBackground")

                Rectangle {
                    anchors.top: textRect.top
                    width: parent.width
                    height: 2
                    color: Utility.getAppHexColor("lightAccent")
                }

                Layout.fillWidth: true
                Layout.preferredHeight: valMetrics.height * 12 + 20
                Layout.alignment: Qt.AlignBottom

                Text {
                    id: valText
                    color: Utility.getAppHexColor("lightText")
                    text: "No Data"
                    font.family: "DejaVu Sans Mono"
                    verticalAlignment: Text.AlignVCenter
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.topMargin: 5
                }

                TextMetrics {
                    id: valMetrics
                    font: valText.font
                    text: "A"
                }
            }

            RowLayout {
                Layout.leftMargin: 10 + notchLeft
                Layout.rightMargin: 10 + notchRight
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 150
                }

                Button {
                    id: menuButton
                    Layout.preferredWidth: 50
                    Layout.fillWidth: true
                    text: "..."
                    onClicked: menu.open()

                    Menu {
                        id: menu
                        bottomPadding: notchBot
                        leftPadding: notchLeft
                        rightPadding: notchRight
                        parent: bmsPageItem
                        y: parent.height - implicitHeight
                        width: parent.width

                        MenuItem {
                            text: "Balance On"
                            onTriggered: {
                                mCommands.bmsForceBalance(true)
                            }
                        }
                        MenuItem {
                            text: "Balance Off"
                            onTriggered: {
                                mCommands.bmsForceBalance(false)
                            }
                        }
                        MenuItem {
                            text: "Reset Ah Counter"
                            onTriggered: {
                                mCommands.bmsResetCounters(true, false)
                            }
                        }
                        MenuItem {
                            text: "Reset Wh Counter"
                            onTriggered: {
                                mCommands.bmsResetCounters(false, true)
                            }
                        }
                        MenuItem {
                            text: "Allow Charging"
                            onTriggered: {
                                mCommands.bmsSetChargeAllowed(true)
                            }
                        }
                        MenuItem {
                            text: "Disable Charging"
                            onTriggered: {
                                mCommands.bmsSetChargeAllowed(false)
                            }
                        }
                        MenuItem {
                            text: "Zero Current Offset"
                            onTriggered: {
                                mCommands.bmsZeroCurrentOffset()
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: mCommands

        function onBmsValuesRx(val) {
            mVal = val
            mValSet = true

            valText.text =
                    "V Tot      : " + parseFloat(val.v_tot).toFixed(2) + " V\n" +
                    "V Charge   : " + parseFloat(val.v_charge).toFixed(2) + " V\n" +
                    "I In       : " + parseFloat(val.i_in_ic).toFixed(2) + " A\n" +
                    "Ah Cnt     : " + parseFloat(val.ah_cnt).toFixed(3) + " Ah\n" +
                    "Wh Cnt     : " + parseFloat(val.wh_cnt).toFixed(2) + " Wh\n" +
                    "Power      : " + parseFloat(val.v_tot * val.i_in_ic).toFixed(2) + " W\n" +
                    "Humidity   : " + parseFloat(val.humidity).toFixed(1) + " %\n" +
                    "Pressure   : " + parseFloat(val.pressure).toFixed(0) + " Pa\n" +
                    "Temp Hum   : " + parseFloat(val.temp_hum_sensor).toFixed(1) + " \u00B0C\n" +
                    "Temp Max   : " + parseFloat(val.temp_cells_highest).toFixed(1) + " \u00B0C\n" +
                    "SoC        : " + parseFloat(val.soc * 100).toFixed(1) + " %\n" +
                    "Ah Chg Tot : " + parseFloat(val.ah_cnt_chg_total).toFixed(3) + " Ah"

            cellCanvas.requestPaint()
            tempCanvas.requestPaint()
        }
    }
}
