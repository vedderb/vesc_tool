/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

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
import Vedder.vesc.utility 1.0

Item {
    id: hallDetect
    property var table: []

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()
    property var dialogParent: Overlay.overlay

    function openDialog() {
        dialog.open()
    }

    function testConnected() {
        if (VescIf.isPortConnected()) {
            return true
        } else {
            VescIf.emitMessageDialog(
                        "Connection Error",
                        "The VESC is not connected. Please connect it to run detection.",
                        false, false)
            return false
        }
    }

    SheetDialog {
        id: dialog
        fullHeight: true
        title: qsTr("Hall Sensor Detection")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Hall Sensors-96.png"
        acceptText: qsTr("Apply")
        acceptEnabled: table.length == 8

        onAccepted: {
            for(var i = 0;i < 7;i++) {
                mMcConf.updateParamInt("foc_hall_table__" + i, table[i])
            }

            VescIf.emitStatusMessage("Hall Sensor Parameters Applied", true)
        }

        ScrollView {
            id: scroll
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 12

                SectionHeader {
                    Layout.topMargin: 0
                    text: qsTr("Measurement")
                }

                LabeledSpinBox {
                    id: currentBox
                    // As wide as a cell of the two column grids in the other detection sheets
                    Layout.preferredWidth: (scroll.availableWidth - 10) / 2
                    label: qsTr("Current")
                    decimals: 2
                    realValue: 10.0
                    realFrom: 0.0
                    realTo: 200.0
                    suffix: " A"
                }

                Button {
                    Layout.alignment: Qt.AlignLeft
                    text: qsTr("How detection works")
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
                    flat: true
                    onClicked: {
                        VescIf.emitMessageDialog(
                                    mInfoConf.getLongName("help_foc_hall_detect"),
                                    mInfoConf.getDescription("help_foc_hall_detect"),
                                    true, true)
                    }
                }

                SectionHeader {
                    text: qsTr("Detect")
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Run detection")
                    highlighted: table.length != 8
                    onClicked: {
                        if (testConnected()) {
                            detectDialog.open()
                        }
                    }
                }

                SectionHeader {
                    text: qsTr("Results")
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    HallPlot {
                        Layout.minimumWidth: 170
                        Layout.preferredWidth: 170
                        Layout.preferredHeight: 170
                        table: hallDetect.table
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8
                        rowSpacing: 8
                        uniformCellWidths: true

                        Repeater {
                            model: 8

                            ValueTile {
                                Layout.fillWidth: true
                                title: qsTr("Hall %1").arg(index)
                                value: table.length == 8 ? "" + table[index] : "\u2013"
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Utility.getAppHexColor("normalText")
                    font.pointSize: 11
                    text: qsTr("Apply puts the table in the motor configuration. Write it to save it on the VESC.")
                }
            }
        }
    }

    SheetDialog {
        id: detectDialog
        acceptText: qsTr("Start")
        title: qsTr("Detect Hall Sensors")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Hall Sensors-96.png"

        WarningBanner {
            width: parent.width
            text: qsTr("This turns the motor slowly. Make sure that nothing is in the way.")
        }

        onAccepted: {
            mCommands.measureHallFoc(currentBox.realValue)
        }
    }

    Connections {
        target: mCommands

        function onFocHallTableReceived(hall_table, res) {
            if (res !== 0) {
                VescIf.emitStatusMessage("Bad FOC Hall Detection Result Received", false)
                VescIf.emitMessageDialog("Bad FOC Hall Detection Result Received",
                                         "Could not detect hall sensors. Make sure that everything " +
                                         "is connected properly.",
                                         false, false)
            } else {
                table = hall_table
            }
        }
    }
}
