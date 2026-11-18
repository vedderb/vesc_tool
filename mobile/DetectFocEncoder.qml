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
    property real mOffset: 0.0
    property real mRatio: 0.0
    property bool mInverted: false
    property bool mReceived: false
    property var dialogParent: Overlay.overlay

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()

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
        title: qsTr("Encoder Detection")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Encoder-96.png"
        acceptText: qsTr("Apply")
        acceptEnabled: mReceived

        onAccepted: {
            mMcConf.updateParamDouble("foc_encoder_offset", mOffset)
            mMcConf.updateParamDouble("foc_encoder_ratio", mRatio)
            mMcConf.updateParamBool("foc_encoder_inverted", mInverted)

            VescIf.emitStatusMessage("Encoder Parameters Applied", true)
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
                                    mInfoConf.getLongName("help_foc_encoder_detect"),
                                    mInfoConf.getDescription("help_foc_encoder_detect"),
                                    true, true)
                    }
                }

                SectionHeader {
                    text: qsTr("Detect")
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Run detection")
                    highlighted: !mReceived
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
                    columns: 3
                    columnSpacing: 8
                    rowSpacing: 8
                    uniformCellWidths: true

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("Offset")
                        value: mReceived ? mOffset.toFixed(1) : "\u2013"
                        unit: "°"
                    }

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("Ratio")
                        value: mReceived ? mRatio.toFixed(1) : "\u2013"
                    }

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("Inverted")
                        value: mReceived ? (mInverted ? qsTr("Yes") : qsTr("No")) : "\u2013"
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
        title: qsTr("Detect Encoder")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Encoder-96.png"

        WarningBanner {
            width: parent.width
            text: qsTr("This turns the motor slowly. Make sure that nothing is in the way.")
        }

        onAccepted: {
            mCommands.measureEncoder(currentBox.realValue)
        }
    }

    Connections {
        target: mCommands

        function onEncoderParamReceived(res) {
            if (res.offset > 1000.0) {
                VescIf.emitStatusMessage("Encoder not enabled in firmware", false)
                VescIf.emitMessageDialog("Error",
                                         "Encoder support is not enabled. Enable it in the general settings.",
                                         false, false)
            } else {
                VescIf.emitStatusMessage("Encoder Result Received", true)
                mOffset = res.offset
                mRatio = res.ratio
                mInverted = res.inverted
                mReceived = true
            }
        }
    }
}
