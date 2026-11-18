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
    property real intLim: 0.0
    property real coupling: 0.0
    property var hallTable: []
    property int hallRes: -4
    property bool resultReceived: false
    property var dialogParent: Overlay.overlay

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()

    function openDialog() {
        dialog.open()
    }

    function hallText() {
        switch (hallRes) {
        case 0: return hallTable.join(", ")
        case -1: return qsTr("Failed: %1").arg(hallTable.join(", "))
        case -2: return qsTr("WS2811 enabled, cannot use hall sensors")
        case -3: return qsTr("Encoder enabled, cannot use hall sensors")
        case -4: return "\u2013"
        default: return qsTr("Unknown error: %1").arg(hallRes)
        }
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
        title: qsTr("BLDC Detection")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Detect BLDC-96.png"
        acceptText: qsTr("Apply")
        acceptEnabled: resultReceived

        onAccepted: {
            mMcConf.updateParamDouble("sl_bemf_coupling_k", coupling)
            mMcConf.updateParamDouble("sl_cycle_int_limit", intLim)

            if (hallRes == 0) {
                for(var i = 0;i < 7;i++) {
                    mMcConf.updateParamInt("hall_table__" + i, hallTable[i])
                }
            }
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

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 10
                    uniformCellWidths: true

                    LabeledSpinBox {
                        id: currentBox
                        Layout.fillWidth: true
                        label: qsTr("Current")
                        decimals: 2
                        realValue: 5.0
                        realFrom: 0.0
                        realTo: 200.0
                        suffix: " A"
                    }

                    LabeledSpinBox {
                        id: dutyBox
                        Layout.fillWidth: true
                        label: qsTr("Duty cycle")
                        decimals: 2
                        realValue: 0.05
                        realFrom: 0.0
                        realTo: 1.0
                        realStepSize: 0.01
                    }

                    LabeledSpinBox {
                        id: erpmBox
                        Layout.fillWidth: true
                        label: qsTr("Min ERPM")
                        decimals: 1
                        realValue: 450.0
                        realFrom: 0.0
                        realTo: 20000.0
                        realStepSize: 10.0
                    }
                }

                Button {
                    Layout.alignment: Qt.AlignLeft
                    text: qsTr("How detection works")
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
                    flat: true
                    onClicked: {
                        VescIf.emitMessageDialog(
                                    mInfoConf.getLongName("help_bldc_detect"),
                                    mInfoConf.getDescription("help_bldc_detect"),
                                    true, true)
                    }
                }

                SectionHeader {
                    text: qsTr("Detect")
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Run detection")
                    highlighted: !resultReceived
                    onClicked: {
                        if (testConnected()) {
                            detectDialog.open()
                        }
                    }
                }

                SectionHeader {
                    text: qsTr("Results")
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 8
                    rowSpacing: 8
                    uniformCellWidths: true

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("Integrator limit")
                        value: resultReceived ? intLim.toFixed(2) : "\u2013"
                    }

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("BEMF coupling")
                        value: resultReceived ? coupling.toFixed(2) : "\u2013"
                    }

                    ValueTile {
                        Layout.fillWidth: true
                        Layout.columnSpan: 2
                        title: qsTr("Hall sensor table")
                        value: hallText()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Utility.getAppHexColor("normalText")
                    font.pointSize: 11
                    text: qsTr("Apply puts the results in the motor configuration. Write it to save them on the VESC.")
                }
            }
        }
    }

    SheetDialog {
        id: detectDialog
        acceptText: qsTr("Start")
        title: qsTr("Detect BLDC Parameters")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Detect BLDC-96.png"

        WarningBanner {
            width: parent.width
            text: qsTr("This spins up the motor. Make sure that nothing is in the way.")
        }

        onAccepted: {
            mCommands.detectMotorParam(currentBox.realValue, erpmBox.realValue, dutyBox.realValue)
        }
    }

    Connections {
        target: mCommands

        function onBldcDetectReceived(param) {
            if (param.cycle_int_limit < 0.01 && param.bemf_coupling_k < 0.01) {
                VescIf.emitStatusMessage("Bad Detection Result Received", false)
                VescIf.emitMessageDialog("BLDC Detection",
                                         "Bad Detection Result Received",
                                         false, false)
            } else {
                VescIf.emitStatusMessage("Detection Result Received", true)
                intLim = param.cycle_int_limit
                coupling = param.bemf_coupling_k
                hallTable = param.hall_table
                hallRes = param.hall_res
                resultReceived = true
            }
        }
    }
}
