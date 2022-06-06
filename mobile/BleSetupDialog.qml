/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    property var dialogParent: ApplicationWindow.overlay
    property Commands mCommands: VescIf.commands()

    function openDialog() {
        dialog.open()
    }

    Dialog {
        title: "BLE Module Setup"

        id: dialog
        modal: true
        focus: true
        width: parent.width - 20 - notchLeft - notchRight
        closePolicy: Popup.CloseOnEscape
        x: (parent.width - width)/2
        y: parent.height / 2 - height / 2 + notchTop
        parent: dialogParent
        padding: 10

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        ColumnLayout {
            anchors.fill: parent

            RowLayout {
                Layout.fillWidth: true

                Rectangle {
                    enabled: useNameBox.checked

                    Layout.fillWidth: true
                    height: nameInput.implicitHeight + 14
                    border.width: 2
                    border.color: enabled ? Utility.getAppHexColor("midAccent") : Utility.getAppHexColor("disabledText")
                    color: "#33a8a8a8"
                    radius: 3
                    TextInput {
                        id: nameInput
                        color: Utility.getAppHexColor("lightText")
                        anchors.fill: parent
                        anchors.margins: 7
                        font.pointSize: 12
                        maximumLength: 25
                        focus: true
                    }
                }

                CheckBox {
                    id: useNameBox
                    checked: true
                    text: "Update Name"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Rectangle {
                    enabled: usePinBox.checked
                    Layout.fillWidth: true
                    height: pinInput.implicitHeight + 14
                    border.width: 2
                    border.color: enabled ? Utility.getAppHexColor("midAccent") : Utility.getAppHexColor("disabledText")
                    color: "#33a8a8a8"
                    radius: 3
                    TextInput {
                        id: pinInput
                        color: Utility.getAppHexColor("lightText")
                        inputMethodHints: Qt.ImhDigitsOnly
                        anchors.fill: parent
                        anchors.margins: 7
                        font.pointSize: 12
                        validator: IntValidator {bottom: 0; top: 999999}
                        maximumLength: 6
                        focus: true
                    }
                }

                CheckBox {
                    id: usePinBox
                    checked: true
                    Layout.preferredWidth: useNameBox.width
                    text: "Update Pin\nUse 6 digits"
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Apply && Reset NRF"
                visible: useNameBox.checked || usePinBox.checked
                onClicked: {
                    if (!VescIf.isPortConnected()) {
                        VescIf.emitMessageDialog("BLE Setup", "Not Connected", false, false)
                        dialog.close()
                        return;
                    }

                    var msg = ""

                    var nameTxt = ""
                    if (useNameBox.checked) {
                        nameTxt = nameInput.text

                        if (nameTxt.length >= 2) {
                            msg += "New Name: " + nameTxt + "\n"
                        } else {
                            msg += "Name reset to default value.\n"
                        }

                        mCommands.setBleName(nameTxt)
                    }

                    var pinTxt = ""
                    if (usePinBox.checked) {
                        pinTxt = pinInput.text
                        if (pinTxt.length > 0) {
                            while(pinTxt.length < 6) {
                                pinTxt = "0" + pinTxt
                            }
                        }

                        if (pinTxt.length > 0) {
                            msg += "New Pin : " + pinTxt + "\n"
                        } else {
                            msg += "Pin code disabled.\n"
                        }

                        mCommands.setBlePin(pinTxt)
                    }

                    if (msg.length > 0) {
                        VescIf.emitMessageDialog("BLE Setup", msg, true, false)
                    }

                    dialog.close()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Cancel"

                onClicked: {
                    dialog.close()
                }
            }
        }
    }
}
