/*
    Copyright 2017 - 2019 Benjamin Vedder	benjamin@vedder.se

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
import Vedder.vesc.bleuart 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0

Item {
    id: topItem

    property Commands mCommands: VescIf.commands()
    property bool isHorizontal: width > height
    signal requestOpenControls()
    signal requestConnect()

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        clip: true

        GridLayout {
            id: grid
            anchors.fill: parent
            columns: isHorizontal ? 2 : 1
            columnSpacing: 5
            rowSpacing: 10

            Image {
                id: image
                Layout.columnSpan: isHorizontal ? 2 : 1
                Layout.preferredWidth: Math.min(topItem.width, topItem.height) * 0.8
                Layout.preferredHeight: (sourceSize.height * Layout.preferredWidth) / sourceSize.width
                Layout.margins: Math.min(topItem.width, topItem.height) * 0.1
                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                source: "qrc" + Utility.getThemePath() + "/logo_white.png"
                antialiasing: true
                Layout.bottomMargin: 0
                Layout.topMargin: Math.min(topItem.width, topItem.height) * 0.025
            }

            GroupBox {
                id: wizardBox
                title: qsTr("Configuration")
                Layout.fillWidth: true

                GridLayout {
                    anchors.topMargin: -5
                    anchors.bottomMargin: -5
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 5
                    rowSpacing: isHorizontal ? 5 : 0

                    ImageButton {
                        id: connectButton

                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Connect"
                        imageSrc: "qrc" + Utility.getThemePath() + (VescIf.isPortConnected() ? "icons/Disconnected-96.png" : "icons/Connected-96.png")
                        onClicked: {
                            if (VescIf.isPortConnected()) {
                                VescIf.disconnectPort()
                            } else {
                                topItem.requestConnect()
                            }
                        }
                    }

                    ImageButton {
                        id: nrfPairButton

                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "VESC\nRemote\nQuick Pair"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/icons8-fantasy-96.png"

                        onClicked: {
                            if (!VescIf.isPortConnected()) {
                                VescIf.emitMessageDialog("Quick Pair",
                                                         "You are not connected to the VESC. Please connect in order " +
                                                         "to quick pair an NRF-based remote.", false, false)
                            } else {
                                nrfPairStartDialog.open()
                            }
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Setup\nMotors"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/motor.png"

                        onClicked: {
                            if (!VescIf.isPortConnected()) {
                                VescIf.emitMessageDialog("FOC Setup Wizard",
                                                         "You are not connected to the VESC. Please connect in order " +
                                                         "to run this wizard.", false, false)
                            } else {
                                wizardFoc.openDialog()
                            }
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Setup\nInput"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Wizard-96.png"

                        onClicked: {
                            if (!VescIf.isPortConnected()) {
                                VescIf.emitMessageDialog("Input Setup Wizard",
                                                         "You are not connected to the VESC. Please connect in order " +
                                                         "to run this wizard.", false, false)
                            } else {
                                // Something in the opendialog function causes a weird glitch, probably
                                // caused by the eventloop in the can scan function. Disabling the button
                                // seems to help. TODO: figure out what the actual problem is.
                                enabled = false
                                wizardInput.openDialog()
                                enabled = true
                            }
                        }
                    }

                    NrfPair {
                        id: nrfPair
                        Layout.fillWidth: true
                        Layout.columnSpan: 2
                        visible: false
                        hideAfterPair: true
                    }

                    Item {
                        visible: isHorizontal
                        Layout.fillHeight: true
                    }
                }
            }

            GroupBox {
                id: canFwdBox
                Layout.preferredHeight: isHorizontal ? wizardBox.height : -1
                title: qsTr("CAN Forwarding")
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.topMargin: -5
                    anchors.bottomMargin: -5
                    anchors.fill: parent
                    spacing: -10

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.bottomMargin: isHorizontal ? 5 : 0

                        ComboBox {
                            id: canIdBox
                            Layout.fillWidth: true

                            textRole: "key"
                            model: ListModel {
                                id: canItems
                            }

                            onCurrentIndexChanged: {
                                if (fwdCanBox.checked && canItems.rowCount() > 0) {
                                    mCommands.setCanSendId(canItems.get(canIdBox.currentIndex).value)
                                }
                            }
                        }

                        CheckBox {
                            id: fwdCanBox
                            text: qsTr("Activate")
                            enabled: canIdBox.currentIndex >= 0 && canIdBox.count > 0

                            onClicked: {
                                mCommands.setSendCan(fwdCanBox.checked, canItems.get(canIdBox.currentIndex).value)
                                canScanButton.enabled = !checked
                                canAllButton.enabled = !checked
                            }
                        }
                    }

                    ProgressBar {
                        id: canScanBar
                        visible: false
                        Layout.fillWidth: true
                        indeterminate: true
                        Layout.preferredHeight: canAllButton.height
                    }

                    RowLayout {
                        id: canButtonLayout
                        Layout.fillWidth: true

                        Button {
                            id: canAllButton
                            text: "List All (no Scan)"
                            Layout.fillWidth: true
                            Layout.preferredWidth: 500

                            onClicked: {
                                canItems.clear()
                                for (var i = 0;i < 255;i++) {
                                    var name = "VESC " + i
                                    canItems.append({ key: name, value: i })
                                }
                                canIdBox.currentIndex = 0
                            }
                        }

                        Button {
                            id: canScanButton
                            text: "Scan CAN Bus"
                            Layout.fillWidth: true
                            Layout.preferredWidth: 500

                            onClicked: {
                                canScanBar.indeterminate = true
                                canButtonLayout.visible = false
                                canScanBar.visible = true
                                canItems.clear()
                                enabled = false
                                canAllButton.enabled = false
                                mCommands.pingCan()
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                        visible: isHorizontal
                    }
                }
            }

            GroupBox {
                id: toolsBox
                title: qsTr("Tools")
                Layout.fillWidth: true

                GridLayout {
                    anchors.topMargin: -5
                    anchors.bottomMargin: -5
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 5
                    rowSpacing: 0

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Controls"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Controller-96.png"

                        onClicked: {
                            requestOpenControls()
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Invert\nMotor\nDirections"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Process-96.png"

                        onClicked: {
                            if (!VescIf.isPortConnected()) {
                                VescIf.emitMessageDialog("Directions",
                                                         "You are not connected to the VESC. Please connect in order " +
                                                         "to map directions.", false, false)
                            } else {
                                enabled = false
                                directionSetupDialog.open()
                                directionSetup.scanCan()
                                enabled = true
                            }
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Backup\nConfigs"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Save-96.png"

                        onClicked: {
                            backupConfigDialog.open()
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Restore\nConfigs"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"

                        onClicked: {
                            restoreConfigDialog.open()
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Pair\nBLE"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/bluetooth.png"

                        onClicked: {
                            pairDialog.openDialog()
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Realtime Data Logging")
                Layout.fillWidth: true

                LogBox {
                    anchors.fill: parent
                    dialogParent: topItem
                }
            }

            GroupBox {
                title: qsTr("Wireless Bridge to Computer (TCP)")
                Layout.fillWidth: true

                TcpBox {
                    anchors.fill: parent
                }
            }
        }
    }

    SetupWizardFoc {
        id: wizardFoc
    }

    SetupWizardInput {
        id: wizardInput
    }

    PairingDialog {
        id: pairDialog
    }

    Timer {
        repeat: true
        interval: 1000
        running: true

        onTriggered: {
            if (!VescIf.isPortConnected()) {
                canItems.clear()
                mCommands.setSendCan(false, -1)
                fwdCanBox.checked = false
                canScanButton.enabled = true
                canAllButton.enabled = true
            }
        }
    }

    Connections {
        target: mCommands

        onPingCanRx: {
            if (canItems.count == 0) {
                for (var i = 0;i < devs.length;i++) {
                    var params = Utility.getFwVersionBlockingCan(VescIf, devs[i])
                    var name = params.hwTypeStr() + " " + devs[i]
                    canItems.append({ key: name, value: devs[i] })
                }
                canScanButton.enabled = true
                canAllButton.enabled = true
                canIdBox.currentIndex = 0
                canButtonLayout.visible = true
                canScanBar.visible = false
                canScanBar.indeterminate = false
            }
        }

        onNrfPairingRes: {
            if (res != 0) {
                nrfPairButton.visible = true
            }
        }
    }

    Connections {
        target: VescIf
        onPortConnectedChanged: {
            if (VescIf.isPortConnected()) {
                connectButton.buttonText = "Disconnect"
                connectButton.imageSrc = "qrc" + Utility.getThemePath() + "icons/Disconnected-96.png"
            } else {
                connectButton.buttonText = "Connect"
                connectButton.imageSrc = "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
            }
        }
    }

    Dialog {
        id: directionSetupDialog
        title: "Direction Setup"
        standardButtons: Dialog.Close
        modal: true
        focus: true
        padding: 10

        width: parent.width - 10
        closePolicy: Popup.CloseOnEscape
        x: 5
        y: parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        DirectionSetup {
            id: directionSetup
            anchors.fill: parent
        }
    }

    Dialog {
        id: nrfPairStartDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "NRF Pairing"

        parent: ApplicationWindow.overlay
        x: 10
        y: topItem.y + topItem.height / 2 - height / 2

        Text {
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text:
                "After clicking OK the VESC will be put in pairing mode for 10 seconds. Switch" +
                "on your remote during this time to complete the pairing process."
        }

        onAccepted: {
            nrfPair.visible = true
            nrfPairButton.visible = false
            nrfPair.startPairing()
        }
    }

    Dialog {
        id: backupConfigDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Backup configuration(s)"

        parent: ApplicationWindow.overlay
        x: 10
        y: topItem.y + topItem.height / 2 - height / 2

        Text {
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text:
                "This will backup the configuration of the connected VESC, as well as for the VESCs " +
                "connected over CAN-bus. The configurations are stored by VESC UUID. If a backup for a " +
                "VESC UUID already exists it will be overwritten. Continue?"
        }

        onAccepted: {
            progDialog.open()
            VescIf.confStoreBackup(true, "")
            progDialog.close()
        }
    }

    Dialog {
        id: restoreConfigDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Restore configuration backup(s)"

        parent: ApplicationWindow.overlay
        x: 10
        y: topItem.y + topItem.height / 2 - height / 2

        Text {
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text:
                "This will restore the configuration of the connected VESC, as well as the VESCs connected over CAN bus " +
                "if a backup exists for their UUID in this instance of VESC Tool. If no backup is found for the UUID of " +
                "the VESCs nothing will be changed. Continue?"
        }

        onAccepted: {
            progDialog.open()
            VescIf.confRestoreBackup(true)
            progDialog.close()
        }
    }

    Dialog {
        id: progDialog
        title: "Processing..."
        closePolicy: Popup.NoAutoClose
        modal: true
        focus: true

        width: parent.width - 20
        x: 10
        y: parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        ProgressBar {
            anchors.fill: parent
            indeterminate: visible
        }
    }
}
