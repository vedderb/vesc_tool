/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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
import Vedder.vesc.utility 1.0

// Summary of the connected device for the start page
Rectangle {
    id: card
    signal disconnectClicked()
    signal canClicked()
    signal updateClicked()
    signal changesClicked()
    readonly property bool hasMotorData: hwType === "VESC" || canMotorId >= 0
    // Battery values from a BMS, shown when there is no motor controller
    readonly property var bms: hasMotorData ? null : bmsValues

    implicitHeight: column.implicitHeight + 28
    radius: 12
    color: Utility.getAppHexColor("cardBackground")
    border.color: Utility.getAppHexColor("raisedBackground")

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            // Battery ring in the style of the gauges
            Canvas {
                id: ring
                property real level: card.bms ? card.bms.soc : rtBattery

                visible: card.hasMotorData || card.bms !== null
                Layout.preferredWidth: 88
                Layout.preferredHeight: 88
                onLevelChanged: requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    var c = width / 2
                    ctx.reset()
                    ctx.lineWidth = 7
                    ctx.lineCap = "round"
                    ctx.strokeStyle = Utility.getAppHexColor("lightBackground")
                    ctx.beginPath()
                    ctx.arc(c, c, c - 5, 0.75 * Math.PI, 2.25 * Math.PI)
                    ctx.stroke()

                    if (level >= 0) {
                        ctx.strokeStyle = level < 0.15 ? Utility.getAppHexColor("danger") :
                                          level < 0.3 ? Utility.getAppHexColor("warning") : Utility.getAppHexColor("positiveText")
                        ctx.beginPath()
                        ctx.arc(c, c, c - 5, 0.75 * Math.PI, (0.75 + 1.5 * level) * Math.PI)
                        ctx.stroke()
                    }
                }

                Column {
                    anchors.centerIn: parent

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: ring.level >= 0 ? Math.round(ring.level * 100) + "%" : "–"
                        font.pointSize: 18
                        font.weight: Font.Medium
                        color: Utility.getAppHexColor("lightText")
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: (card.bms ? card.bms.v_tot : rtVin).toFixed(1) + " V"
                        font.pointSize: 9
                        color: Utility.getAppHexColor("normalText")
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Text {
                    Layout.fillWidth: true
                    text: connName
                    elide: Text.ElideRight
                    font.pointSize: 15
                    font.weight: Font.Medium
                    color: Utility.getAppHexColor("titleText")
                }

                Text {
                    Layout.fillWidth: true
                    text: [hwName.replace(/_/g, " ") !== connName ? "HW " + hwName : "", "FW " + fwVersion, connLink,
                           hwType !== "VESC" && canMotorId >= 0 ? qsTr("motor on CAN %1").arg(canMotorId) : ""].filter(Boolean).join(" · ")
                    elide: Text.ElideRight
                    font.pointSize: 10
                    color: Utility.getAppHexColor("normalText")
                }

                Button {
                    Layout.topMargin: 4
                    text: qsTr("Disconnect")
                    onClicked: card.disconnectClicked()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: hwType !== "" && !card.hasMotorData && card.bms === null
            spacing: 10

            Text {
                Layout.fillWidth: true
                text: qsTr("This device has no motor data. Select a motor controller on the CAN bus to see live values.")
                wrapMode: Text.WordWrap
                font.pointSize: 10
                color: Utility.getAppHexColor("normalText")
            }

            Button {
                text: qsTr("CAN devices")
                onClicked: card.canClicked()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: card.hasMotorData
            spacing: 8

            ValueTile {
                Layout.fillWidth: true
                color: Utility.getAppHexColor("darkBackground")
                border.width: 0
                title: qsTr("MOSFET")
                value: rtTempMos.toFixed(0)
                unit: "°C"
            }

            ValueTile {
                Layout.fillWidth: true
                color: Utility.getAppHexColor("darkBackground")
                border.width: 0
                title: qsTr("Motor")
                value: rtTempMotor.toFixed(0)
                unit: "°C"
            }

            ValueTile {
                Layout.fillWidth: true
                color: Utility.getAppHexColor("darkBackground")
                border.width: 0
                title: qsTr("Faults")
                value: faultText !== "" ? qsTr("Fault") : qsTr("None")
                valueColor: faultText !== "" ? Utility.getAppHexColor("dangerText") : Utility.getAppHexColor("positiveText")
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: card.bms !== null
            spacing: 8

            Repeater {
                model: card.bms ? [[qsTr("Current"), card.bms.i_in_ic.toFixed(1), "A"],
                                   [qsTr("Min-Max"), ((Math.max(...card.bms.v_cells) - Math.min(...card.bms.v_cells)) * 1000).toFixed(0), "mV"],
                                   [qsTr("Cell Temp"), card.bms.temp_cells_highest.toFixed(0), "°C"]] : []

                ValueTile {
                    Layout.fillWidth: true
                    color: Utility.getAppHexColor("darkBackground")
                    border.width: 0
                    title: modelData[0]
                    value: modelData[1]
                    unit: modelData[2]
                }
            }
        }

        Flow {
            Layout.fillWidth: true
            Layout.topMargin: -6
            Layout.bottomMargin: -6
            spacing: 6
            visible: canCount > 0 || fwUpdateAvailable || changedCount > 0

            Chip {
                visible: canCount > 0
                tinted: true
                tint: Utility.getAppHexColor("lightAccent")
                tintText: Utility.getAppHexColor("infoText")
                text: qsTr("%1 on CAN").arg(canCount)
                iconSource: "qrc" + Utility.getThemePath() + "icons/can_off.png"
                onClicked: card.canClicked()
            }

            Chip {
                visible: fwUpdateAvailable
                tinted: true
                text: qsTr("FW %1 available").arg(fwLatest)
                onClicked: card.updateClicked()
            }

            Chip {
                visible: changedCount > 0
                tinted: true
                text: qsTr("%1 unwritten").arg(changedCount)
                onClicked: card.changesClicked()
            }
        }
    }
}
