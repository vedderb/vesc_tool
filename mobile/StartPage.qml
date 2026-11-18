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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QtCore as Core

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.bleuart 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0

Item {
    id: topItem
    property Commands mCommands: VescIf.commands()
    property bool isHorizontal: width > height
    readonly property int cols: mainIsTablet && width >= 1040 ? 3 : isHorizontal || mainIsTablet ? 2 : 1
    signal requestOpenControls()
    signal requestConnect()
    signal requestOpenMultiSettings()

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: grid.height + 30
        clip: true

        GridLayout {
            id: grid
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            columns: cols
            uniformCellWidths: cols > 1
            anchors.topMargin: 15
            anchors.bottomMargin: 15
            columnSpacing: 10
            rowSpacing: 5

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 10

                Image {
                    // No room for the logo next to the device card on a phone in landscape
                    visible: !connected || !phoneLandscape
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Math.min(parent.width * (connected ? 0.45 : 0.7), 400)
                    Layout.preferredHeight: (sourceSize.height * Layout.preferredWidth) / sourceSize.width
                    Layout.bottomMargin: connected ? 0 : 8
                    source: "qrc" + Utility.getThemePath() + "/logo.png"
                    antialiasing: true
                }

                DeviceCard {
                    Layout.fillWidth: true
                    visible: connected
                    onDisconnectClicked: requestDisconnect()
                    onCanClicked: canDrawerLoader.item.open()
                    onUpdateClicked: firmwareDialog.open()
                    onChangesClicked: reviewChanges(mMcConf.changedParams.length > 0 ? confPageMotor : confPageApp)
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    visible: !connected
                    text: qsTr("Connect")
                    highlighted: true
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                    onClicked: topItem.requestConnect()
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 52
                    radius: 12
                    visible: !connected && lastDeviceName !== ""
                    color: Utility.getAppHexColor("cardBackground")
                    border.color: Utility.getAppHexColor("raisedBackground")

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                Layout.fillWidth: true
                                text: lastDeviceName
                                elide: Text.ElideRight
                                font.weight: Font.Medium
                                color: Utility.getAppHexColor("titleText")
                            }

                            Text {
                                text: qsTr("Last connected · %1").arg(timeAgo(lastDeviceTime))
                                font.pointSize: 9
                                color: Utility.getAppHexColor("normalText")
                            }
                        }

                        Button {
                            Layout.rightMargin: 10
                            text: qsTr("Reconnect")
                            onClicked: VescIf.reconnectLastPort()
                        }
                    }
                }
            }

            ColumnLayout {
                id: wizardBox
                Layout.fillWidth: true
                Layout.alignment: cols > 2 ? Qt.AlignTop : 0

                SectionHeader {
                    text: qsTr("Configuration")
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 8
                    rowSpacing: 8

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Setup Motors"
                        hintText: qsTr("FOC wizard")
                        locked: !connected
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/motor.png"

                        onClicked: {
                            wizardFoc.openDialog()
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Setup Input"
                        hintText: qsTr("PPM · ADC · Remote")
                        locked: !connected
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Wizard-96.png"

                        onClicked: {
                            // Something in the opendialog function causes a weird glitch, probably
                            // caused by the eventloop in the can scan function. Disabling the button
                            // seems to help. TODO: figure out what the actual problem is.
                            enabled = false
                            wizardInput.openDialog()
                            enabled = true
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Setup IMU"
                        hintText: qsTr("Orientation · calibration")
                        locked: !connected
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Gyroscope-96.png"

                        onClicked: {
                            wizardIMU.openDialog()
                        }
                    }

                    ImageButton {
                        id: nrfPairButton
                        visible: !nrfPair.visible
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "VESC Remote Quick Pair"
                        hintText: qsTr("NRF remotes")
                        locked: !connected
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/icons8-fantasy-96.png"

                        onClicked: {
                            nrfPairStartDialog.open()
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
                }
            }

            ColumnLayout {
                id: toolsBox
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.rowSpan: cols === 2 && !phoneLandscape ? 2 : 1
                Layout.columnSpan: phoneLandscape ? 2 : 1

                SectionHeader {
                    text: qsTr("Tools and Operations")
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: phoneLandscape ? 4 : 2
                    columnSpacing: 8
                    rowSpacing: 8

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Controls"
                        hintText: qsTr("Manual drive")
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Controller-96.png"

                        onClicked: {
                            requestOpenControls()
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Invert Motor Directions"
                        hintText: qsTr("All CAN devices")
                        locked: !connected
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Process-96.png"

                        onClicked: {
                            enabled = false
                            directionSetupDialog.open()
                            directionSetup.scanCan()
                            enabled = true
                        }
                    }

                    ImageButton {
                        id: backupConfButton
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Backup Configs"
                        hintText: qsTr("Local and CAN")
                        locked: !connected
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Save-96.png"

                        onClicked: {
                            backupConfigDialog.open()
                        }
                    }

                    ImageButton {
                        id: restoreConfButton
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Restore Configs"
                        hintText: qsTr("By UUID")
                        locked: !connected
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"

                        onClicked: {
                            restoreConfigDialog.open()
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Setup Bluetooth Module"
                        hintText: qsTr("Name · PIN")
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
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Multi Settings"
                        hintText: qsTr("Write to all")
                        locked: !connected
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Settings-96.png"

                        onClicked: {
                            requestOpenMultiSettings()
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Update Firmware"
                        hintText: badge ? "%1 → %2".arg(fwVersion).arg(fwLatest) : qsTr("Firmware and bootloader")
                        badge: connected && fwUpdateAvailable
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Electronics-96.png"

                        onClicked: {
                            firmwareDialog.open()
                        }
                    }

                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        Layout.preferredHeight: 62

                        buttonText: "Package Store"
                        hintText: qsTr("Packages and apps")
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Package-96.png"

                        onClicked: {
                            packageDialog.open()
                        }
                    }
                }

                Timer {
                    id: toolsTimer
                    interval: 100
                    repeat: true
                    running: true
                    onTriggered: {
                        restoreConfButton.enabled = VescIf.customConfigRxDone()
                        backupConfButton.enabled = VescIf.customConfigRxDone()
                        logSection.status = VescIf.isRtLogOpen() ? qsTr("On") : qsTr("Off")
                        tcpSection.status = !VescIf.tcpServerIsRunning() ? qsTr("Off") :
                                            VescIf.tcpServerIsClientConnected() ? VescIf.tcpServerClientIp() : qsTr("On")
                    }
                }
            }

            CollapsibleSection {
                id: logSection
                Layout.alignment: cols > 2 || phoneLandscape ? Qt.AlignTop : 0
                title: qsTr("Realtime Data Logging")
                iconSource: "qrc" + Utility.getThemePath() + "icons/Line Chart-96.png"

                LogBox {
                    Layout.fillWidth: true
                    dialogParent: topItem
                }
            }

            CollapsibleSection {
                id: tcpSection
                Layout.alignment: cols > 2 || phoneLandscape ? Qt.AlignTop : 0
                title: qsTr("Wireless Bridge to Computer (TCP)")
                iconSource: "qrc" + Utility.getThemePath() + "icons/LAN-96.png"

                TcpBox {
                    Layout.fillWidth: true
                }
            }

            CollapsibleSection {
                id: hubSection
                Layout.alignment: cols > 2 ? Qt.AlignTop : 0
                title: qsTr("TCP Hub (Internet Bridge)")
                iconSource: "qrc" + Utility.getThemePath() + "icons/Globe-96.png"
                Layout.bottomMargin: isHorizontal ? 0 : 10

                TcpHubBox {
                    Layout.fillWidth: true
                }
            }
        }
    }

    Core.Settings {
        category: "startPage"
        property alias logOpen: logSection.open
        property alias tcpOpen: tcpSection.open
        property alias hubOpen: hubSection.open
    }

    // Card that shows a status line and expands to show its content
    component CollapsibleSection: Rectangle {
        id: section
        property string title: ""
        property string status: ""
        property string iconSource: ""
        property bool open: false
        default property alias content: body.data

        Layout.fillWidth: true
        Layout.topMargin: 3
        implicitHeight: sectionColumn.implicitHeight
        radius: 12
        color: Utility.getAppHexColor("cardBackground")
        border.color: Utility.getAppHexColor("raisedBackground")

        ColumnLayout {
            id: sectionColumn
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 0

            ItemDelegate {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                icon.source: section.iconSource
                text: section.title
                onClicked: section.open = !section.open

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    text: section.status + (section.open ? "  ▴" : "  ▾")
                    color: Utility.getAppHexColor("normalText")
                }
            }

            ColumnLayout {
                id: body
                Layout.fillWidth: true
                Layout.margins: 12
                Layout.topMargin: 0
                visible: section.open
            }
        }
    }

    SetupWizardFoc {
        id: wizardFoc
        dialogParent: mainSwipeView
        isHorizontal: mainIsHorizontal
    }

    SetupWizardInput {
        id: wizardInput
        dialogParent: mainSwipeView
        isHorizontal: mainIsHorizontal
    }

    PairingDialog {
        id: pairDialog
        dialogParent: mainSwipeView
    }

    BleSetupDialog {
        id: bleSetupDialog
        dialogParent: mainSwipeView
    }

    SetupWizardIMU {
        id: wizardIMU
        dialogParent: mainSwipeView
    }

    SheetDialog {
        id: directionSetupDialog
        title: "Invert Motor Directions"

        DirectionSetup {
            id: directionSetup
            anchors.fill: parent
            dialogParent: topItem
        }
    }

    SheetDialog {
        id: nrfPairStartDialog
        title: "NRF Pairing"
        iconSource: "qrc" + Utility.getThemePath() + "icons/icons8-fantasy-96.png"
        acceptText: qsTr("Start pairing")

        Text {
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text:
                "The VESC will be put in pairing mode for 10 seconds. Switch " +
                "on your remote during this time to complete the pairing process."
        }

        onAccepted: {
            nrfPair.visible = true
            nrfPair.startPairing()
        }
    }

    SheetDialog {
        id: backupConfigDialog
        property var canDevs: []

        title: "Back up configurations"
        iconSource: "qrc" + Utility.getThemePath() + "icons/Save-96.png"
        acceptText: canDevs.length > 0 ? qsTr("Back up %1 VESCs").arg(canDevs.length + 1) : qsTr("Back up")
        onAboutToShow: canDevs = VescIf.getCanDevsLast()

        ColumnLayout {
            anchors.fill: parent
            spacing: 6

            Repeater {
                model: [qsTr("This VESC · ID %1").arg(VescIf.appConfig().getParamInt("controller_id"))].concat(
                           backupConfigDialog.canDevs.map(function(id) { return qsTr("CAN bus · ID %1").arg(id) }))

                Text {
                    text: "✓  " + modelData
                    color: Utility.getAppHexColor("lightText")
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: 6
                text: "Stored per VESC UUID. Replaces the previous backup."
                wrapMode: Text.WordWrap
                font.pointSize: 10
                color: Utility.getAppHexColor("normalText")
            }
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

    SheetDialog {
        id: restoreConfigDialog
        title: "Restore configurations"
        iconSource: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
        acceptText: qsTr("Restore")

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

    BusyDialog {
        id: progDialog
        title: "Processing..."
    }

    Dialog {
        id: firmwareDialog
        closePolicy: Popup.CloseOnEscape
        modal: true
        focus: true
        title: qsTr("Update Firmware")

        // Landscape phones: over the app bars too, only inside the safe area
        readonly property bool fullWindow: phoneLandscape
        parent: fullWindow ? Overlay.overlay : mainSwipeView
        x: fullWindow ? notchLeft : 0
        y: fullWindow ? notchTop : 0
        width: parent.width - (fullWindow ? notchLeft + notchRight : 0)
        height: parent.height - (fullWindow ? notchTop + notchBot : 0)
        leftInset: fullWindow ? -notchLeft : 0
        rightInset: fullWindow ? -notchRight : 0
        topInset: fullWindow ? -notchTop : 0
        bottomInset: fullWindow ? -notchBot : 0

        Overlay.modal: Rectangle {
            color: Utility.getAppHexColor("overlay")
        }

        background: Rectangle {
            color: Utility.getAppHexColor("darkBackground")
        }

        contentItem: FwUpdate {
            id: fwUpdate
            anchors.fill: parent
            anchors.topMargin: firmwareDialog.header.height
        }

        header: PageHeader {
            title: firmwareDialog.title
            onCloseClicked: firmwareDialog.close()
        }
    }

    SheetDialog {
        id: packageDialog
        title: phoneLandscape ? "" : qsTr("Package Store")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Package-96.png"
        fullHeight: true
        sheetWidth: Math.min(parent.width, mainIsTablet ? 1000 : 640)

        Packages {
            anchors.fill: parent
            dialogParent: mainSwipeView
        }
    }
}
