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
import QtQuick.Controls 2.10
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
    signal requestOpenMultiSettings()

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: grid.height + 30
        property int gridItemPreferredWidth: isHorizontal ? parent.width/2.0 - 15 : parent.width - 10
        clip: true

        GridLayout {
            id: grid
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            columns: isHorizontal ? 2 : 1
            anchors.topMargin: 15
            anchors.bottomMargin: 15
            columnSpacing: 10
            rowSpacing: 5
            Item {
                Layout.columnSpan: 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                implicitHeight: image.height
                Image {
                    id: image
                    anchors.centerIn: parent
                    width: Math.min(parent.width * 0.8, 0.4 * sourceSize.width * wizardBox.height/sourceSize.height)
                    height: (sourceSize.height * width) / sourceSize.width
                    source: "qrc" + Utility.getThemePath() + "/logo.png"
                    antialiasing: true
                }
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
                    rowSpacing: 5

                    ImageButton {
                        id: connectButton
                        Layout.fillWidth: true
                        Layout.fillHeight: true
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
                        visible: !nrfPair.visible
                        Layout.fillWidth: true
                        Layout.fillHeight: true
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
                        Layout.fillHeight: true
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
                        Layout.fillHeight: true
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

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Setup\nIMU"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/imu_off.png"

                        onClicked: {
                            if (!VescIf.isPortConnected()) {
                                VescIf.emitMessageDialog("IMU Setup Wizard",
                                                         "You are not connected to the VESC. Please connect in order " +
                                                         "to run this wizard.", false, false)
                            } else {
                                wizardIMU.openDialog()
                            }
                        }
                    }

                    NrfPair {
                        id: nrfPair
                        Layout.fillWidth: true
                        Layout.fillHeight: true
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
                id: toolsBox
                title: qsTr("Tools")
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.rowSpan: isHorizontal ? 2 : 1

                GridLayout {
                    anchors.topMargin: -5
                    anchors.bottomMargin: -5
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 5
                    rowSpacing: 5

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
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
                        Layout.fillHeight: true
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
                        Layout.fillHeight: true
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
                        Layout.fillHeight: true
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
                        Layout.fillHeight: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Setup\nBluetooth\nModule"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/bluetooth.png"

                        onClicked: {
                            if (VescIf.getLastFwRxParams().nrfNameSupported &&
                                    VescIf.getLastFwRxParams().nrfPinSupported) {
                                bleSetupDialog.openDialog()
                            } else {
                                pairDialog.openDialog()
                            }
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 80

                        buttonText: "Multi\nSettings"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Settings-96.png"

                        onClicked: {
                            requestOpenMultiSettings()
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

            GroupBox {
                title: qsTr("TCP Hub (Internet Bridge)")
                Layout.bottomMargin: isHorizontal ? 0 : 10
                Layout.fillWidth: true

                TcpHubBox {
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

    BleSetupDialog {
        id: bleSetupDialog
    }

    SetupWizardIMU {
        id: wizardIMU
    }

    Connections {
        target: VescIf
        function onPortConnectedChanged() {
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

        width: parent.width - 10 - notchLeft - notchRight
        closePolicy: Popup.CloseOnEscape
        x: parent.width/2 - width/2
        y: parent.height / 2 - height / 2 + notchTop
        parent: ApplicationWindow.overlay

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

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
        width: parent.width - 20 - notchLeft - notchRight
        closePolicy: Popup.CloseOnEscape
        title: "NRF Pairing"

        parent: ApplicationWindow.overlay
        x: parent.width/2 - width/2
        y: topItem.y + topItem.height / 2 - height / 2 + notchTop

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

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
            nrfPair.startPairing()
        }
    }

    Dialog {
        id: backupConfigDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20 - notchLeft - notchRight
        closePolicy: Popup.CloseOnEscape
        title: "Backup configuration(s)"

        parent: ApplicationWindow.overlay
        x: parent.width/2 - width/2
        y: topItem.y + topItem.height / 2 - height / 2 + notchTop

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

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
            workaroundTimerBackup.start()
        }
        Timer {
            id: workaroundTimerBackup
            interval: 0
            repeat: false
            running: false
            onTriggered: {
                VescIf.confStoreBackup(true, "")
                progDialog.close()
            }
        }
    }

    Dialog {
        id: restoreConfigDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20 - notchLeft - notchRight
        closePolicy: Popup.CloseOnEscape
        title: "Restore configuration backup(s)"

        parent: ApplicationWindow.overlay
        x: parent.width/2 - width/2
        y: topItem.y + topItem.height / 2 - height / 2 + notchTop

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

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
            workaroundTimerRestore.start()
        }
        Timer {
            id: workaroundTimerRestore
            interval: 0
            repeat: false
            running: false
            onTriggered: {
                VescIf.confRestoreBackup(true)
                progDialog.close()
            }
        }
    }

    Dialog {
        id: progDialog
        title: "Processing..."
        closePolicy: Popup.NoAutoClose
        modal: true
        focus: true

        width: parent.width - 20 - notchLeft - notchRight
        x: parent.width/2 - width/2
        y: parent.height / 2 - height / 2 + notchTop
        parent: ApplicationWindow.overlay

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        ProgressBar {
            anchors.fill: parent
            indeterminate: visible
        }
    }
}
