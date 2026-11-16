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
    readonly property bool hasCells: mValSet && mVal.v_cells.length > 0
    readonly property real cvMin: hasCells ? Math.min(...mVal.v_cells) : 0
    readonly property real cvMax: hasCells ? Math.max(...mVal.v_cells) : 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            contentHeight: grid.height + 20
            clip: true

            GridLayout {
                id: grid
                x: 10 + notchLeft
                y: 10
                width: parent.width - 20 - notchLeft - notchRight
                columns: isHorizontal ? 2 : 1
                columnSpacing: 16
                rowSpacing: 10

                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 1
                    spacing: 10

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 4
                        columnSpacing: 6

                        Repeater {
                            model: [["Pack", mValSet ? parseFloat(mVal.v_tot).toFixed(2) : "-", "V"],
                                ["Current", mValSet ? parseFloat(mVal.i_in_ic).toFixed(1) : "-", "A"],
                                ["SoC", mValSet ? parseFloat(mVal.soc * 100).toFixed(0) : "-", "%"],
                                ["Min-Max", hasCells ? parseFloat((cvMax - cvMin) * 1000).toFixed(0) : "-", "mV"]]

                            ValueTile {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                title: modelData[0]
                                value: modelData[1]
                                unit: modelData[2]
                            }
                        }
                    }

                    Text {
                        id: summaryText
                        Layout.fillWidth: true
                        color: Utility.getAppHexColor("normalText")
                        font.pointSize: 10
                        wrapMode: Text.WordWrap
                        text: hasCells ? qsTr("%1 cells · min %2 V (C%3) · max %4 V (C%5)").
                                         arg(mVal.v_cells.length).
                                         arg(parseFloat(cvMin).toFixed(3)).arg(mVal.v_cells.indexOf(cvMin) + 1).
                                         arg(parseFloat(cvMax).toFixed(3)).arg(mVal.v_cells.indexOf(cvMax) + 1) :
                                       "No Data"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: cellCanvas.height + 16
                        visible: hasCells
                        radius: 12
                        color: Utility.getAppHexColor("normalBackground")
                        border.color: Utility.getAppHexColor("lightBackground")

                        Canvas {
                            id: cellCanvas
                            readonly property int rowHeight: 24
                            x: 10
                            y: 8
                            width: parent.width - 20
                            height: (hasCells ? mVal.v_cells.length : 0) * rowHeight + 18

                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.reset()

                                if (!hasCells) {
                                    return
                                }

                                var vCells = mVal.v_cells
                                var isBal = mVal.is_balancing
                                var xOfsLeft = 34
                                var cellWidth = width - xOfsLeft - 96
                                var heightBars = vCells.length * rowHeight
                                var lineDivs = 6
                                var lineStep = cellWidth / lineDivs

                                ctx.font = '%1pt \"%2\"'.arg(summaryText.font.pointSize).arg(summaryText.font.family)
                                ctx.textBaseline = "middle"

                                ctx.beginPath()

                                for (var i = 0; i <= lineDivs;i++) {
                                    ctx.moveTo(xOfsLeft + lineStep * i, 0)
                                    ctx.lineTo(xOfsLeft + lineStep * i, heightBars)
                                }

                                ctx.strokeStyle = Utility.getAppHexColor("lightBackground")
                                ctx.lineWidth = 1
                                ctx.stroke()

                                ctx.fillStyle = Utility.getAppHexColor("normalText")
                                ctx.textAlign = "center"

                                for (i = 0; i <= lineDivs;i++) {
                                    ctx.fillText(parseFloat(3 + i * (1.2 / lineDivs)).toFixed(1), xOfsLeft + lineStep * i, heightBars + 9)
                                }

                                for (i = 0;i < vCells.length;i++) {
                                    var y = i * rowHeight + rowHeight / 2
                                    var isMin = vCells[i] === cvMin && cvMax > cvMin
                                    var tagColor = Utility.getAppHexColor(isBal[i] ? "infoText" : "warningText")
                                    var cellW = Math.max(Math.min(((vCells[i] - 3.0) / 1.2) * cellWidth, cellWidth), 1.2)

                                    ctx.fillStyle = Utility.getAppHexColor("normalText")
                                    ctx.textAlign = "start"
                                    ctx.fillText("C" + (i + 1), 0, y)

                                    ctx.beginPath()
                                    ctx.roundedRect(xOfsLeft, y - 6, cellW, 12, 3, 3)
                                    ctx.fillStyle = isBal[i] ? Utility.getAppHexColor(Utility.isDarkMode() ? "infoText" : "vescBlue") :
                                                               Utility.getAppHexColor(isMin ? "warning" : "cellBar")
                                    ctx.fill()

                                    // Stripes on balancing cells
                                    if (isBal[i]) {
                                        ctx.save()
                                        ctx.clip()
                                        ctx.beginPath()

                                        for (var x = xOfsLeft - 12; x < xOfsLeft + cellW; x += 8) {
                                            ctx.moveTo(x, y + 6)
                                            ctx.lineTo(x + 12, y - 6)
                                        }

                                        ctx.strokeStyle = Utility.isDarkMode() ? Qt.darker(tagColor, 1.6) : Qt.rgba(1, 1, 1, 0.35)
                                        ctx.lineWidth = 3
                                        ctx.stroke()
                                        ctx.restore()
                                    }

                                    ctx.textAlign = "end"
                                    ctx.fillStyle = isMin ? tagColor : Utility.getAppHexColor("lightText")
                                    ctx.fillText(parseFloat(vCells[i]).toFixed(3) + " V", width - 30, y)

                                    if (isBal[i] || isMin) {
                                        ctx.fillStyle = tagColor
                                        ctx.fillText(isBal[i] ? "BAL" : "MIN", width, y)
                                    }
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 1
                    spacing: 10
                    visible: mValSet

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: rows.implicitHeight + 8
                        radius: 14
                        color: Utility.getAppHexColor("rowBackground")
                        border.color: Utility.getAppHexColor("lightBackground")

                        Column {
                            id: rows
                            y: 4
                            width: parent.width

                            Repeater {
                                model: mValSet ? [["Status", mVal.status],
                                    ["Counters", parseFloat(mVal.ah_cnt).toFixed(3) + " Ah · " + parseFloat(mVal.wh_cnt).toFixed(2) + " Wh"],
                                    ["Power", parseFloat(mVal.v_tot * mVal.i_in_ic).toFixed(2) + " W"],
                                    ["V Charge", parseFloat(mVal.v_charge).toFixed(2) + " V"],
                                    ["Ah Chg / Dis Tot", parseFloat(mVal.ah_cnt_chg_total).toFixed(3) + " / " +
                                     parseFloat(mVal.ah_cnt_dis_total).toFixed(3) + " Ah"],
                                    ["Pressure", parseFloat(mVal.pressure).toFixed(0) + " Pa"]] : []

                                RowLayout {
                                    width: rows.width
                                    height: 36

                                    Text {
                                        Layout.leftMargin: 14
                                        Layout.fillWidth: true
                                        text: modelData[0]
                                        color: Utility.getAppHexColor("normalText")
                                    }

                                    Text {
                                        Layout.rightMargin: 14
                                        text: modelData[1]
                                        color: Utility.getAppHexColor("lightText")
                                        font.features: { "tnum": 1 }
                                    }
                                }
                            }
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 6

                        Repeater {
                            model: mValSet ? mVal.temps : []

                            Chip {
                                readonly property var labels: mVal.data_version === 1 ?
                                                                  ["IC", "Cell Min", "Cell Max", "Mosfet", "Ambient"] : []
                                enabled: false
                                text: (index < labels.length ? labels[index] : "T" + (index + 1 - labels.length)) + " " +
                                      (modelData <= -280.0 ? "N/A" : parseFloat(modelData).toFixed(1) + " \u00B0C")
                            }
                        }

                        Chip {
                            enabled: false
                            text: mValSet ? "Hum " + parseFloat(mVal.humidity).toFixed(1) + " % (" +
                                            parseFloat(mVal.temp_hum_sensor).toFixed(1) + " \u00B0C)" : ""
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.margins: 10
            Layout.leftMargin: 10 + notchLeft
            Layout.rightMargin: 10 + notchRight
            Layout.fillWidth: true

            Button {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                text: "Balance \u25BE"
                onClicked: balanceMenu.open()

                BottomMenu {
                    id: balanceMenu
                    parent: bmsPageItem

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
                }
            }

            Button {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                text: "Charging \u25BE"
                onClicked: chargeMenu.open()

                BottomMenu {
                    id: chargeMenu
                    parent: bmsPageItem

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
                }
            }

            Button {
                id: menuButton
                Layout.preferredWidth: 56
                text: "..."
                onClicked: menu.open()

                BottomMenu {
                    id: menu
                    parent: bmsPageItem

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
                        text: "Zero Current Offset"
                        onTriggered: {
                            mCommands.bmsZeroCurrentOffset()
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
            cellCanvas.requestPaint()
        }
    }
}
