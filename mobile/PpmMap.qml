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

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    property int parentWidth: 10
    property real msMin: 0.0
    property real msMax: 0.0
    property real msCenter: 0.0
    property real msNow: 0.0
    property real valueNow: 0.5
    property bool resetDone: true

    property Commands mCommands: VescIf.commands()
    property ConfigParams mAppConf: VescIf.appConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()

    function openDialog() {
        dialog.open()
    }

    function updateDisplay() {
        resultArea.text =
                "Value  : " + parseFloat(valueNow).toFixed(2) + "\n\n" +
                "Now    : " + parseFloat(msNow).toFixed(4) + " ms\n" +
                "Min    : " + parseFloat(msMin).toFixed(4) + " ms\n" +
                "Max    : " + parseFloat(msMax).toFixed(4) + " ms\n" +
                "Center : " + parseFloat(msCenter).toFixed(4) + " ms"

        valueBar.value = valueNow
    }

    Component.onCompleted: {
        updateDisplay()
    }

    Dialog {
        id: dialog
        standardButtons: Dialog.Close
        modal: true
        focus: true
        width: parentWidth - 20
        height: Math.min(implicitHeight, column.height - 40)
        closePolicy: Popup.CloseOnEscape
        x: 10
        y: 10

        ScrollView {
            anchors.fill: parent
            clip: true
            contentWidth: parent.width
            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                TextArea {
                    id: resultArea
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    readOnly: true
                    wrapMode: TextEdit.WordWrap
                    font.family: "DejaVu Sans Mono"
                }

                ProgressBar {
                    id: valueBar
                    Layout.fillWidth: true
                    from: -1.0
                    to: 1.0
                    value: 0.0
                }

                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: "Help"
                        Layout.preferredWidth: 50
                        Layout.fillWidth: true
                        onClicked: {
                            VescIf.emitMessageDialog(
                                        mInfoConf.getLongName("app_ppm_mapping_help"),
                                        mInfoConf.getDescription("app_ppm_mapping_help"),
                                        true, true)
                        }
                    }

                    Button {
                        text: "Reset"
                        Layout.preferredWidth: 50
                        Layout.fillWidth: true
                        onClicked: {
                            msMin = 0.0
                            msMax = 0.0
                            msCenter = 0.0
                            resetDone = true
                            updateDisplay()
                        }
                    }
                }

                Button {
                    text: "Apply & Write"
                    Layout.fillWidth: true
                    onClicked: {
                        mAppConf.updateParamDouble("app_ppm_conf.pulse_start", msMin)
                        mAppConf.updateParamDouble("app_ppm_conf.pulse_end", msMax)
                        mAppConf.updateParamDouble("app_ppm_conf.pulse_center", msCenter)
                        VescIf.emitStatusMessage("Start, End and Center Pulselengths Applied", true)
                        mCommands.setAppConf()
                    }
                }
            }
        }
    }

    Timer {
        id: rtTimer
        interval: 50
        running: true
        repeat: true

        onTriggered: {
            if (VescIf.isPortConnected() && dialog.visible) {
                mCommands.getDecodedPpm()
            }
        }
    }

    Connections {
        target: mCommands

        onDecodedPpmReceived: {
            valueNow = value
            msNow = last_len

            if (resetDone) {
                resetDone = false
                msMin = msNow
                msMax = msNow
            }

            if (msNow < msMin) {
                msMin = msNow
            }

            if (msNow > msMax) {
                msMax = msNow
            }

            var range = msMax - msMin
            var pos = msNow - msMin

            if (pos > (range / 4.0) && pos < ((3.0 * range) / 4.0)) {
                msCenter = msNow
            } else {
                msCenter = range / 2.0 + msMin
            }

            updateDisplay()
        }
    }
}
