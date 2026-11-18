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
import Vedder.vesc.utility 1.0

Item {
    implicitHeight: column.implicitHeight
    property bool hideAfterPair: false
    property bool pairOngoing: false

    function startPairing() {
        mCommands.pairNrf(timeBox.realValue * 1000.0)
        pairCnt = timeBox.realValue
        pairOngoing = true
        afterPairTimer.stop()
    }

    property real pairCnt: 0.0
    property Commands mCommands: VescIf.commands()
    property ConfigParams mInfoConf: VescIf.infoConfig()

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 12

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 10
            uniformCellWidths: true

            LabeledSpinBox {
                id: timeBox
                Layout.fillWidth: true
                enabled: pairCnt <= 0
                label: qsTr("Pairing time")
                decimals: 1
                realFrom: 1.0
                realTo: 30.0
                realValue: 10.0
                suffix: " s"
            }

            Button {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignBottom
                text: qsTr("Start pairing")
                enabled: pairCnt <= 0
                highlighted: true
                onClicked: startPairing()
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            value: pairCnt / timeBox.realValue
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Utility.getAppHexColor("lightText")
                font.pointSize: 11
                text: pairCnt > 0 ? qsTr("Switch on the remote now, %1 s left").arg(Math.ceil(pairCnt)) :
                                    qsTr("Switch on the remote while the VESC is in pairing mode")
            }

            ToolButton {
                icon.source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
                Accessible.name: qsTr("Help")
                onClicked: {
                    VescIf.emitMessageDialog(
                                mInfoConf.getLongName("help_nrf_pair"),
                                mInfoConf.getDescription("help_nrf_pair"),
                                true, true)
                }
            }
        }
    }

    Timer {
        id: cntTimer
        interval: 100
        running: true
        repeat: true

        onTriggered: {
            if (pairCnt > 0.01) {
                pairCnt -= 0.1

                if (pairCnt <= 0.01) {
                    pairCnt = 0.0
                }
            } else {
                if (visible) {
                    afterPairTimer.start()
                }
            }
        }
    }

    Timer {
        id: afterPairTimer
        interval: 2000
        running: false
        repeat: false

        onTriggered: {
            if (pairOngoing) {
                pairOngoing = false
                if (hideAfterPair) {
                    visible = false
                }

                VescIf.emitMessageDialog(
                            "VESC Remote Pairing",
                            "No response received. This most likely means that the " +
                            "hardware you are using is not capable of VESC Remote pairing " +
                            "or that something is not plugged in properly.",
                            true, false)
            }
        }
    }

    Connections {
        target: mCommands

        function onNrfPairingRes(res) {
            if (!visible) {
                return
            }

            switch (res) {
            case 1:
                pairCnt = 0.0
                pairOngoing = false
                VescIf.emitStatusMessage("Pairing Sucessful", true)
                VescIf.emitMessageDialog(
                            "VESC Remote Pairing",
                            "Pairing was successful.",
                            true, false)
                break;

            case 2:
                pairCnt = 0.0
                pairOngoing = false
                VescIf.emitStatusMessage("Pairing NRF Timed Out", false)
                VescIf.emitMessageDialog(
                            "VESC Remote Pairing",
                            "Pairing timed out. Make sure to put your device (e.g. VESC Wand) " +
                            "in pairing mode before the time runs out." +
                            "<br><br>" +
                            "To put a VESC-compatible Remote in " +
                            "pairing mode, just switch it on using any of the buttons. Then it " +
                            "will enter pairing mode if it was switched off previously.",
                            false, false)
                break;

            default:
                break;
            }

            if (hideAfterPair && res > 0) {
                visible = false
            }
        }
    }
}
