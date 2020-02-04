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
import Qt.labs.settings 1.0 as QSettings

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

ApplicationWindow {
    id: appWindow
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mAppConf: VescIf.appConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()

    visible: true
    width: 500
    height: 850
    title: qsTr("VESC Tool")

    Component.onCompleted: {
        //        Utility.checkVersion(VescIf)
        //        swipeView.setCurrentIndex(1)
        //        rtSwipeView.setCurrentIndex(1)

        if (!VescIf.isIntroDone()) {
            introWizard.openDialog()
        }

        Utility.keepScreenOn(VescIf.keepScreenOn())
        Utility.stopGnssForegroundService()
    }

    SetupWizardIntro {
        id: introWizard
    }

    Controls {
        id: controls
        parentWidth: appWindow.width
        parentHeight: appWindow.height - footer.height - tabBar.height
    }

    Settings {
        id: settings
    }

    Drawer {
        id: drawer
        width: 0.5 * appWindow.width
        height: appWindow.height - footer.height - tabBar.height
        y: tabBar.height
        dragMargin: 20

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 0

            Image {
                id: image
                Layout.preferredWidth: Math.min(parent.width, parent.height)
                Layout.preferredHeight: (394 * Layout.preferredWidth) / 1549
                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                source: "qrc:/res/logo_white.png"
            }

            Button {
                id: reconnectButton

                Layout.fillWidth: true
                text: "Reconnect"
                enabled: false
                flat: true

                onClicked: {
                    VescIf.reconnectLastPort()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Disconnect"
                enabled: connBle.disconnectButton.enabled
                flat: true

                onClicked: {
                    VescIf.disconnectPort()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Controls"
                flat: true

                onClicked: {
                    drawer.close()
                    controls.openDialog()
                }
            }

            Item {
                // Spacer
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            Button {
                Layout.fillWidth: true
                text: "Settings"
                flat: true

                onClicked: {
                    drawer.close()
                    settings.openDialog()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "About"
                flat: true

                onClicked: {
                    VescIf.emitMessageDialog(
                                "About",
                                Utility.aboutText(),
                                true, true)
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Changelog"
                flat: true

                onClicked: {
                    VescIf.emitMessageDialog(
                                "VESC Tool Changelog",
                                Utility.vescToolChangeLog(),
                                true, false)
                }
            }

            Button {
                Layout.fillWidth: true
                text: "License"
                flat: true

                onClicked: {
                    VescIf.emitMessageDialog(
                                mInfoConf.getLongName("gpl_text"),
                                mInfoConf.getDescription("gpl_text"),
                                true, true)
                }
            }
        }
    }

    SwipeView {
        id: swipeView
        currentIndex: tabBar.currentIndex
        anchors.fill: parent

        Page {
            ConnectBle {
                id: connBle
                anchors.fill: parent
                anchors.margins: 10

                onRequestOpenControls: {
                    controls.openDialog()
                }
            }
        }

        Page {
            RowLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    color: "#4f4f4f"
                    width: 16
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignHCenter |  Qt.AlignVCenter

                    PageIndicator {
                        count: rtSwipeView.count
                        currentIndex: rtSwipeView.currentIndex
                        anchors.centerIn: parent
                        rotation: 90
                    }
                }

                SwipeView {
                    id: rtSwipeView
                    enabled: true
                    clip: true

                    property var vesc3dViewNow: 0

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: Qt.Vertical

                    Page {
                        RtData {
                            anchors.fill: parent
                        }
                    }

                    Page {
                        RtDataSetup {
                            anchors.fill: parent
                        }
                    }

                    Page {
                        ColumnLayout {
                            anchors.fill: parent

                            CheckBox {
                                Layout.fillWidth: true
                                id: useYawBox
                                text: "Use Yaw (will drift)"
                                checked: false
                            }

                            Item {
                                id: item3d
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }
                        }
                    }

                    // Create 3d view on demand, due to high CPU usage even when hidden
                    onCurrentIndexChanged: {
                        if (currentIndex == 2) {
                            var component = Qt.createComponent("Vesc3DView.qml");
                            vesc3dViewNow = component.createObject(item3d, {"anchors.fill": item3d})
                            vesc3dViewNow.setRotation(0.1, 0.1, 0.1)
                        } else {
                            if (vesc3dViewNow != 0) {
                                vesc3dViewNow.destroy()
                                vesc3dViewNow = 0
                            }
                        }
                    }
                }
            }
        }

        Page {
            Profiles {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
            }
        }

        Page {
            ConfigPageMotor {
                id: confPageMotor
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
            }
        }

        Page {
            ConfigPageApp {
                id: confPageApp
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
            }
        }

        Page {
            FwUpdate {
                anchors.fill: parent
            }
        }

        Page {
            Terminal {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.topMargin: 10
            }
        }

        Page {
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.topMargin: 10

                GroupBox {
                    id: bleConnBox
                    title: qsTr("Realtime Data Logging")
                    Layout.fillWidth: true
                    Layout.columnSpan: 1

                    GridLayout {
                        anchors.topMargin: -5
                        anchors.bottomMargin: -5
                        anchors.fill: parent

                        clip: false
                        visible: true
                        rowSpacing: -10
                        columnSpacing: 5
                        rows: 3
                        columns: 2

                        Button {
                            text: "Help"
                            Layout.fillWidth: true

                            onClicked: {
                                VescIf.emitMessageDialog(
                                            mInfoConf.getLongName("help_rt_logging"),
                                            mInfoConf.getDescription("help_rt_logging"),
                                            true, true)
                            }
                        }

                        Button {
                            text: "Choose Log Directory..."
                            Layout.fillWidth: true

                            onClicked: {
                                if (Utility.requestFilePermission()) {
                                    logFilePicker.enabled = true
                                    logFilePicker.visible = true
                                } else {
                                    VescIf.emitMessageDialog(
                                                "File Permissions",
                                                "Unable to request file system permission.",
                                                false, false)
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.columnSpan: 2
                            Layout.topMargin: 6
                            Layout.bottomMargin: 6
                            height: rtLogFileText.implicitHeight + 14
                            border.width: 2
                            border.color: "#8d8d8d"
                            color: "#33a8a8a8"
                            radius: 3

                            TextInput {
                                color: "white"
                                id: rtLogFileText
                                anchors.fill: parent
                                anchors.margins: 7
                                font.pointSize: 12
                                text: "./log"

                                QSettings.Settings {
                                    property alias rtLog: rtLogFileText.text
                                }
                            }
                        }

                        CheckBox {
                            id: rtLogEnBox
                            text: "Enable RT Data Logging"
                            Layout.fillWidth: true
                            Layout.columnSpan: 2

                            onClicked: {
                                if (checked) {
                                    if (VescIf.openRtLogFile(rtLogFileText.text)) {
                                        Utility.startGnssForegroundService()
                                        VescIf.setWakeLock(true)
                                    }
                                } else {
                                    VescIf.closeRtLogFile()
                                    Utility.stopGnssForegroundService()

                                    if (!VescIf.useWakeLock()) {
                                        VescIf.setWakeLock(false)
                                    }
                                }
                            }

                            Timer {
                                repeat: true
                                running: true
                                interval: 500

                                onTriggered: {
                                    if (rtLogEnBox.checked && !VescIf.isRtLogOpen()) {
                                        Utility.stopGnssForegroundService()

                                        if (!VescIf.useWakeLock()) {
                                            VescIf.setWakeLock(false)
                                        }
                                    }

                                    rtLogEnBox.checked = VescIf.isRtLogOpen()
                                }
                            }
                        }
                    }
                }

                GroupBox {
                    id: tcpServerBox
                    title: qsTr("TCP Server")
                    Layout.fillWidth: true
                    Layout.columnSpan: 1

                    GridLayout {
                        anchors.topMargin: -5
                        anchors.bottomMargin: -5
                        anchors.fill: parent

                        clip: false
                        visible: true
                        rowSpacing: -10
                        columnSpacing: 5
                        rows: 3
                        columns: 2

                        CheckBox {
                            id: tcpServerEnBox
                            text: "Run TCP Server"
                            Layout.fillWidth: true
                            Layout.columnSpan: 2

                            onClicked: {
                                if (checked) {
                                    VescIf.tcpServerStart(tcpServerPortBox.value)
                                } else {
                                    VescIf.tcpServerStop()
                                }
                            }
                        }

                        Text {
                            text: "TCP Port"
                            color: "white"
                            Layout.fillWidth: true
                            Layout.preferredWidth: 5000
                        }

                        SpinBox {
                            id: tcpServerPortBox
                            from: 0
                            to: 65535
                            value: 65102
                            enabled: !tcpServerEnBox.checked
                            Layout.fillWidth: true
                            Layout.preferredWidth: 5000
                            editable: true
                        }

                        Timer {
                            repeat: true
                            running: true
                            interval: 500

                            onTriggered: {
                                tcpServerEnBox.checked = VescIf.tcpServerIsRunning()

                                if (VescIf.tcpServerIsRunning()) {
                                    var ipTxt = "IP(s)\n"
                                    var addresses = Utility.getNetworkAddresses()
                                    for (var i = 0;i < addresses.length;i++) {
                                        ipTxt += addresses[i]
                                        if (i < (addresses.length - 1)) {
                                            ipTxt += "\n"
                                        }
                                    }
                                    tcpLocalAddress.text = ipTxt
                                } else {
                                    tcpLocalAddress.text = "IP(s)"
                                }

                                tcpRemoteAddress.text = "Connected Client"

                                if (VescIf.tcpServerIsClientConnected()) {
                                    tcpRemoteAddress.text += "\n" + VescIf.tcpServerClientIp()
                                }
                            }
                        }

                        Text {
                            id: tcpLocalAddress
                            Layout.fillWidth: true
                            Layout.topMargin: 10
                            Layout.bottomMargin: 10
                            color: "white"
                        }

                        Text {
                            id: tcpRemoteAddress
                            Layout.fillWidth: true
                            Layout.topMargin: 10
                            Layout.bottomMargin: 10
                            color: "white"
                        }
                    }
                }

                Item {
                    // Spacer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            DirectoryPicker {
                id: logFilePicker
                anchors.fill: parent
                showDotAndDotDot: true
                visible: false
                enabled: false

                onDirSelected: {
                    rtLogFileText.text = fileName
                }
            }
        }
    }

    header: Rectangle {
        color: "#5f5f5f"
        height: tabBar.height

        RowLayout {
            anchors.fill: parent
            spacing: 0

            ToolButton {
                Layout.preferredHeight: tabBar.height
                Layout.preferredWidth: tabBar.height - 10

                Image {
                    id: manuButton
                    anchors.centerIn: parent
                    width: tabBar.height * 0.5
                    height: tabBar.height * 0.5
                    opacity: 0.5
                    source: "qrc:/res/icons/Settings-96.png"
                }

                onClicked: {
                    if (drawer.visible) {
                        drawer.close()
                    } else {
                        drawer.open()
                    }
                }
            }

            TabBar {
                id: tabBar
                currentIndex: swipeView.currentIndex
                Layout.fillWidth: true
                implicitWidth: 0
                clip: true

                background: Rectangle {
                    opacity: 1
                    color: "#4f4f4f"
                }

                property int buttons: 8
                property int buttonWidth: 120

                TabButton {
                    text: qsTr("Start")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("RT Data")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("Profiles")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("Motor Cfg")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("App Cfg")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("Firmware")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("Terminal")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("Developer")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
            }
        }
    }

    footer: Rectangle {
        id: connectedRect
        color: "#4f4f4f"

        Text {
            id: connectedText
            color: "white"
            text: VescIf.getConnectedPortName()
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            anchors.fill: parent
        }

        width: parent.width
        height: 20
    }

    Timer {
        id: statusTimer
        interval: 1600
        running: false
        repeat: false
        onTriggered: {
            connectedText.text = VescIf.getConnectedPortName()
            connectedRect.color = "#4f4f4f"
        }
    }

    Timer {
        id: uiTimer
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            if (!statusTimer.running && connectedText.text !== VescIf.getConnectedPortName()) {
                connectedText.text = VescIf.getConnectedPortName()
            }
        }
    }

    Timer {
        id: confTimer
        interval: 1000
        running: true
        repeat: true

        property bool mcConfRx: false
        property bool appConfRx: false

        onTriggered: {
            if (VescIf.isPortConnected()) {
                if (!mcConfRx) {
                    mCommands.getMcconf()
                }

                if (!appConfRx) {
                    mCommands.getAppConf()
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
            if (VescIf.isPortConnected()) {
                // Sample RT data when the corresponding page is selected, or when
                // RT logging is active.

                if (VescIf.isRtLogOpen()) {
                    interval = 50
                    mCommands.getValues()
                    mCommands.getValuesSetup()
                    mCommands.getImuData(0xFFFF)
                } else {
                    if ((tabBar.currentIndex == 1 && rtSwipeView.currentIndex == 0)) {
                        interval = 50
                        mCommands.getValues()
                    }

                    if (tabBar.currentIndex == 1 && rtSwipeView.currentIndex == 1) {
                        interval = 50
                        mCommands.getValuesSetup()
                    }

                    if (tabBar.currentIndex == 1 && rtSwipeView.currentIndex == 2) {
                        interval = 20
                        mCommands.getImuData(0x7)
                    }
                }
            }
        }
    }

    Dialog {
        id: vescDialog
        standardButtons: Dialog.Ok
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape

        width: parent.width - 20
        height: Math.min(implicitHeight, parent.height - 40)
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        parent: ApplicationWindow.overlay

        ScrollView {
            anchors.fill: parent
            clip: true
            contentWidth: parent.width - 20

            Text {
                id: vescDialogLabel
                color: "#ffffff"
                linkColor: "lightblue"
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
                wrapMode: Text.WordWrap
                textFormat: Text.RichText
                onLinkActivated: {
                    Qt.openUrlExternally(link)
                }
            }
        }
    }

    Connections {
        target: VescIf
        onPortConnectedChanged: {
            connectedText.text = VescIf.getConnectedPortName()
            if (VescIf.isPortConnected()) {
                reconnectButton.enabled = true
            } else {
                confTimer.mcConfRx = false
                confTimer.appConfRx = false
            }

            if (VescIf.useWakeLock()) {
                VescIf.setWakeLock(VescIf.isPortConnected())
            }
        }

        onStatusMessage: {
            connectedText.text = msg
            connectedRect.color = isGood ? "green" : "red"
            statusTimer.restart()
        }

        onMessageDialog: {
            vescDialog.title = title
            vescDialogLabel.text = (richText ? "<style>a:link { color: lightblue; }</style>" : "") + msg
            vescDialogLabel.textFormat = richText ? Text.RichText : Text.AutoText
            vescDialog.open()
        }

        onFwRxChanged: {
            if (rx) {
                if (limited && !VescIf.getFwSupportsConfiguration()) {
                    confPageMotor.enabled = false
                    confPageApp.enabled = false
                    swipeView.setCurrentIndex(5)
                } else {
                    confPageMotor.enabled = true
                    confPageApp.enabled = true
                    mCommands.getMcconf()
                    mCommands.getAppConf()
                }
            }
        }
    }

    Connections {
        target: mMcConf

        onUpdated: {
            confTimer.mcConfRx = true
        }
    }

    Connections {
        target: mAppConf

        onUpdated: {
            confTimer.appConfRx = true
        }
    }

    Connections {
        target: mCommands

        onValuesImuReceived: {
            if (rtSwipeView.vesc3dViewNow != 0) {
                rtSwipeView.vesc3dViewNow.setRotation(
                            values.roll, values.pitch,
                            useYawBox.checked ? values.yaw : -Math.PI / 2.0)
            }
        }
    }
}
