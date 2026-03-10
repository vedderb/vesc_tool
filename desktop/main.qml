/*
    Copyright 2016 - 2022 Benjamin Vedderbenjamin@vedder.se

    VESC Tool desktop QML UI - faithful recreation of the original
    MainWindow widget layout.

    Layout (from mainwindow.ui):
      MenuBar: File | Edit | ConfBackup | Wizards | Terminal | Developer | Help
      Sidebar (220px)  |  Content (StackView)  |  Toolbar (right, vertical)
      Bottom motor control bar (2-row grid)
      StatusBar
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Vedder.vesc

ApplicationWindow {
    id: appWindow
    visible: true
    width: 1175
    height: 652
    title: "VESC Tool"

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mAppConf: VescIf.appConfig()

    property bool connected: false
    property bool mcConfRead: false
    property bool appConfRead: false
    property bool fwReadCorrectly: false

    // DEV: Set to true to show all pages without a VESC connected
    readonly property bool devShowAllPages: true

    // ---------------------------------------------------------------
    // MenuBar: File | Edit | ConfBackup | Wizards | Terminal | Developer | Help
    // ---------------------------------------------------------------
    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            Action { text: qsTr("Save Motor Configuration XML..."); onTriggered: motorConfSaveDialog.open() }
            Action { text: qsTr("Load Motor Configuration XML"); onTriggered: motorConfLoadDialog.open() }
            MenuSeparator {}
            Action { text: qsTr("Save App Configuration XML..."); onTriggered: appConfSaveDialog.open() }
            Action { text: qsTr("Load App Configuration XML"); onTriggered: appConfLoadDialog.open() }
            MenuSeparator {}
            Action { text: qsTr("Exit"); onTriggered: Qt.quit() }
        }
        Menu {
            title: qsTr("Edit")
            Action { text: qsTr("Preferences"); onTriggered: preferencesDialog.open() }
        }
        Menu {
            title: qsTr("ConfBackup")
            Action { text: qsTr("Backup Configuration"); onTriggered: VescIf.confStoreBackup(false) }
            Action { text: qsTr("Backup Configurations (CAN)"); onTriggered: VescIf.confStoreBackup(true) }
            Action { text: qsTr("Restore Configuration"); onTriggered: VescIf.confRestoreBackup(false) }
            Action { text: qsTr("Restore Configurations (CAN)"); onTriggered: VescIf.confRestoreBackup(true) }
            Action { text: qsTr("Clear Configuration Backups"); onTriggered: VescIf.confClearBackups() }
        }
        Menu {
            title: qsTr("Wizards")
            Action { text: qsTr("Setup Motors FOC"); onTriggered: {} }
            Action { text: qsTr("Setup Motors FOC (Quick)"); onTriggered: {} }
            Action { text: qsTr("Setup Input"); onTriggered: {} }
            Action { text: qsTr("Setup Other Motors"); onTriggered: {} }
        }
        Menu {
            title: qsTr("Terminal")
            Action { text: qsTr("Show Help"); onTriggered: mCommands.sendTerminalCmd("help") }
            Action { text: qsTr("Print Faults"); onTriggered: mCommands.sendTerminalCmd("faults") }
            Action { text: qsTr("Print Threads"); onTriggered: mCommands.sendTerminalCmd("threads") }
            MenuSeparator {}
            Action { text: qsTr("DRV Reset Latched Faults"); onTriggered: mCommands.sendTerminalCmd("drv8301_reset_latched_faults") }
            MenuSeparator {}
            Action { text: qsTr("Clear Terminal"); onTriggered: {} }
            MenuSeparator {}
            Action { text: qsTr("Reboot"); onTriggered: mCommands.reboot() }
            Action { text: qsTr("Shutdown"); onTriggered: mCommands.shutdown() }
            Action { text: qsTr("Restart LispBM"); onTriggered: mCommands.sendTerminalCmd("lisp_restart") }
        }
        Menu {
            title: qsTr("Developer")
            Action { text: qsTr("Parameter Editor Mcconf"); onTriggered: {} }
            Action { text: qsTr("Parameter Editor Appconf"); onTriggered: {} }
            Action { text: qsTr("Parameter Editor Info"); onTriggered: {} }
            Action { text: qsTr("Parameter Editor FW"); onTriggered: {} }
            Action { text: qsTr("Parameter Editor CustomConf0"); onTriggered: {} }
            Action { text: qsTr("Export Configuration Parser"); onTriggered: {} }
        }
        Menu {
            title: qsTr("Help")
            MenuSeparator {}
            Action { text: qsTr("VESC Project Forums"); onTriggered: Qt.openUrlExternally("https://vesc-project.com/forum") }
            Action { text: qsTr("VESC Discord"); onTriggered: Qt.openUrlExternally("https://discord.gg/JgvV5NwYts") }
            MenuSeparator {}
            Action { text: qsTr("VESC Tool Changelog"); onTriggered: showTextDialog("VESC Tool Changelog", Utility.vescToolChangeLog()) }
            Action { text: qsTr("Firmware Changelog"); onTriggered: showTextDialog("Firmware Changelog", Utility.fwChangeLog()) }
            MenuSeparator {}
            Action { text: qsTr("About VESC Tool"); onTriggered: showTextDialog("About VESC Tool", Utility.aboutText()) }
            Action {
                text: qsTr("Libraries Used")
                onTriggered: showTextDialog("Libraries Used",
                    "<b>Icons</b><br><a href='https://icons8.com/'>https://icons8.com/</a><br><br>" +
                    "<b>Plotting</b><br><a href='http://qcustomplot.com/'>http://qcustomplot.com/</a>")
            }
            Action { text: qsTr("About Qt"); onTriggered: {} }
        }
    }

    // ---------------------------------------------------------------
    // Main layout: [sidebar + content] | right toolbar
    //              [bottom control bar]
    // ---------------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 1

            // =====================================================
            // LEFT: Sidebar (logo + page list + CAN devices)
            // =====================================================
            Rectangle {
                Layout.preferredWidth: 220
                Layout.minimumWidth: 180
                Layout.fillHeight: true
                color: Utility.getAppHexColor("darkBackground")

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 2
                    spacing: 2

                    // Logo
                    Image {
                        id: pageLabel
                        Layout.alignment: Qt.AlignHCenter
                        Layout.maximumWidth: 220
                        Layout.maximumHeight: (460 * 220) / 1550
                        Layout.topMargin: 6
                        Layout.bottomMargin: 6
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: (sourceSize.height * Layout.preferredWidth) / sourceSize.width
                        source: "qrc" + Utility.getThemePath() + "logo.png"
                        sourceSize.width: 220
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                        antialiasing: true
                    }

                    // Splitter: page list + CAN devices
                    SplitView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        orientation: Qt.Vertical

                        // Page list
                        ListView {
                            id: pageListView
                            SplitView.fillWidth: true
                            SplitView.fillHeight: true
                            SplitView.minimumHeight: 200
                            clip: true
                            currentIndex: 0
                            focus: true
                            boundsBehavior: Flickable.StopAtBounds

                            highlight: Rectangle {
                                color: Utility.getAppHexColor("brightHighlightActive")
                            }
                            highlightMoveDuration: 0
                            highlightFollowsCurrentItem: true

                            model: ListModel {
                                id: pageModel
                            }

                            delegate: Item {
                                width: pageListView.width
                                height: model.isVisible ? (model.bold ? 30 : 26) : 0
                                visible: model.isVisible
                                clip: true

                                Rectangle {
                                    anchors.fill: parent
                                    color: "transparent"

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: model.indent ? 15 : 6
                                        anchors.rightMargin: 4
                                        spacing: 4

                                        // Item icon (square, fontHeight * 1.1 matching original)
                                        Image {
                                            property int iconSz: Math.round(nameLabel.implicitHeight * 1.1)
                                            Layout.preferredWidth: iconSz
                                            Layout.preferredHeight: iconSz
                                            source: model.iconFile !== "" ? ("qrc" + Utility.getThemePath() + model.iconFile) : ""
                                            sourceSize: Qt.size(iconSz, iconSz)
                                            fillMode: Image.PreserveAspectFit
                                            smooth: true
                                            mipmap: true
                                            visible: model.iconFile !== ""
                                        }

                                        // Name
                                        Label {
                                            id: nameLabel
                                            text: model.name
                                            font.bold: model.bold
                                            font.pointSize: 12
                                            color: Utility.getAppHexColor("lightText")
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }

                                        // Group badge (mcconf.png / appconf.png) — aspect-ratio aware
                                        // Original: height = fontHeight, width = height * (imgW / imgH)
                                        Image {
                                            property int badgeH: nameLabel.implicitHeight
                                            Layout.preferredHeight: badgeH
                                            Layout.preferredWidth: sourceSize.width > 0
                                                ? Math.round(badgeH * sourceSize.width / sourceSize.height)
                                                : badgeH
                                            source: model.groupIcon !== "" ? ("qrc" + Utility.getThemePath() + model.groupIcon) : ""
                                            fillMode: Image.PreserveAspectFit
                                            smooth: true
                                            mipmap: true
                                            visible: model.groupIcon !== ""
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            if (model.pageSource !== "") {
                                                pageListView.currentIndex = index
                                                pageStack.loadPage(model.pageSource)
                                            }
                                        }
                                    }
                                }
                            }

                            ScrollBar.vertical: ScrollBar { }
                        }

                        // CAN Devices section
                        Rectangle {
                            SplitView.preferredHeight: 100
                            SplitView.minimumHeight: 60
                            color: Utility.getAppHexColor("normalBackground")

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 2

                                Label {
                                    text: qsTr("CAN-Devices")
                                    font.bold: true
                                    font.family: "Roboto"
                                    Layout.alignment: Qt.AlignHCenter
                                    color: Utility.getAppHexColor("lightText")
                                }

                                ListView {
                                    id: canList
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    model: ListModel { id: canModel }
                                    boundsBehavior: Flickable.StopAtBounds

                                    highlight: Rectangle {
                                        color: Utility.getAppHexColor("brightHighlightActive")
                                    }
                                    highlightFollowsCurrentItem: true
                                    highlightMoveDuration: 0

                                    delegate: Item {
                                        width: canList.width
                                        height: 22

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 6
                                            spacing: 4

                                            Label {
                                                text: model.canId < 0 ? "Local VESC" : ("ID: " + model.canId)
                                                font.pointSize: 12
                                                font.family: "Roboto"
                                                color: Utility.getAppHexColor("lightText")
                                                Layout.fillWidth: true
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                canList.currentIndex = index
                                                if (model.canId < 0) {
                                                    mCommands.setSendCan(false)
                                                } else {
                                                    mCommands.setSendCan(true, model.canId)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Scan CAN button
                    Button {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 200
                        Layout.alignment: Qt.AlignHCenter
                        Layout.bottomMargin: 2
                        text: qsTr("Scan CAN")
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Refresh-96.png"
                        onClicked: mCommands.pingCan()
                    }
                }
            }

            // =====================================================
            // CENTER: Content / StackView
            // =====================================================
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Utility.getAppHexColor("normalBackground")

                StackView {
                    id: pageStack
                    anchors.fill: parent
                    anchors.margins: 8

                    // Disable all page transition animations
                    pushEnter: null
                    pushExit: null
                    popEnter: null
                    popExit: null
                    replaceEnter: null
                    replaceExit: null

                    function loadPage(source) {
                        if (currentItem && currentItem.objectName === source) {
                            return
                        }
                        replace(null, Qt.resolvedUrl(source), { objectName: source })
                    }

                    initialItem: WelcomePage {}
                }
            }

            // =====================================================
            // RIGHT: Vertical Toolbar (RightToolBarArea equivalent)
            // =====================================================
            ToolBar {
                id: rightToolBar
                Layout.fillHeight: true
                Layout.preferredWidth: 42

                contentItem: ColumnLayout {
                    spacing: 1

                    ToolButton {
                        id: reconnectButton
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Reconnect last connection")
                        enabled: !connected
                        implicitWidth: 38; implicitHeight: 38
                        onClicked: VescIf.reconnectLastPort()
                    }
                    ToolButton {
                        id: disconnectButton
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Disconnected-96.png"
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Disconnect")
                        enabled: connected
                        implicitWidth: 38; implicitHeight: 38
                        onClicked: VescIf.disconnectPort()
                    }

                    ToolSeparator { Layout.fillWidth: true; orientation: Qt.Horizontal }

                    ToolButton {
                        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_up.png"
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Read motor configuration")
                        implicitWidth: 38; implicitHeight: 38
                        onClicked: mCommands.getMcconf()
                    }
                    ToolButton {
                        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_default.png"
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Read default motor configuration")
                        implicitWidth: 38; implicitHeight: 38
                        onClicked: mCommands.getMcconfDefault()
                    }
                    ToolButton {
                        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_down.png"
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Write motor configuration")
                        implicitWidth: 38; implicitHeight: 38
                        onClicked: mCommands.setMcconf(true)
                    }

                    ToolSeparator { Layout.fillWidth: true; orientation: Qt.Horizontal }

                    ToolButton {
                        icon.source: "qrc" + Utility.getThemePath() + "icons/app_up.png"
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Read app configuration")
                        implicitWidth: 38; implicitHeight: 38
                        onClicked: mCommands.getAppConf()
                    }
                    ToolButton {
                        icon.source: "qrc" + Utility.getThemePath() + "icons/app_default.png"
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Read default app configuration")
                        implicitWidth: 38; implicitHeight: 38
                        onClicked: mCommands.getAppConfDefault()
                    }
                    ToolButton {
                        icon.source: "qrc" + Utility.getThemePath() + "icons/app_down.png"
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Write app configuration")
                        implicitWidth: 38; implicitHeight: 38
                        onClicked: mCommands.setAppConf()
                    }

                    ToolSeparator { Layout.fillWidth: true; orientation: Qt.Horizontal }

                    ToolButton {
                        id: keyboardButton
                        checkable: true
                        icon.source: checked
                            ? ("qrc" + Utility.getThemePath() + "icons/keys_on.png")
                            : ("qrc" + Utility.getThemePath() + "icons/keys_off.png")
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Keyboard control")
                        implicitWidth: 38; implicitHeight: 38
                    }
                    ToolButton {
                        id: gamepadButton
                        checkable: true
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Controller-96.png"
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Gamepad control")
                        implicitWidth: 38; implicitHeight: 38
                    }
                    ToolButton {
                        id: rtDataButton
                        checkable: true
                        icon.source: checked
                            ? ("qrc" + Utility.getThemePath() + "icons/rt_on.png")
                            : ("qrc" + Utility.getThemePath() + "icons/rt_off.png")
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Stream realtime data")
                        implicitWidth: 38; implicitHeight: 38
                    }
                    ToolButton {
                        id: rtDataAppButton
                        checkable: true
                        icon.source: checked
                            ? ("qrc" + Utility.getThemePath() + "icons/rt_app_on.png")
                            : ("qrc" + Utility.getThemePath() + "icons/rt_app_off.png")
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Stream app data")
                        implicitWidth: 38; implicitHeight: 38
                    }
                    ToolButton {
                        id: imuButton
                        checkable: true
                        icon.source: checked
                            ? ("qrc" + Utility.getThemePath() + "icons/imu_on.png")
                            : ("qrc" + Utility.getThemePath() + "icons/imu_off.png")
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Stream IMU data")
                        implicitWidth: 38; implicitHeight: 38
                    }
                    ToolButton {
                        id: bmsButton
                        checkable: true
                        icon.source: checked
                            ? ("qrc" + Utility.getThemePath() + "icons/bms_on.png")
                            : ("qrc" + Utility.getThemePath() + "icons/bms_off.png")
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Stream BMS data")
                        implicitWidth: 38; implicitHeight: 38
                    }
                    ToolButton {
                        id: sendAliveButton
                        checkable: true
                        icon.source: checked
                            ? ("qrc" + Utility.getThemePath() + "icons/alive_on.png")
                            : ("qrc" + Utility.getThemePath() + "icons/alive_off.png")
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Send alive command")
                        implicitWidth: 38; implicitHeight: 38
                    }
                    ToolButton {
                        id: canFwdButton
                        checkable: true
                        icon.source: checked
                            ? ("qrc" + Utility.getThemePath() + "icons/can_on.png")
                            : ("qrc" + Utility.getThemePath() + "icons/can_off.png")
                        icon.width: 24; icon.height: 24
                        ToolTip.visible: hovered; ToolTip.text: qsTr("Forward CAN")
                        implicitWidth: 38; implicitHeight: 38
                        onToggled: {
                            if (checked !== mCommands.getSendCan()) {
                                mCommands.setSendCan(checked)
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // =====================================================
        // BOTTOM: Motor control bar (2-row grid)
        // =====================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: motorControlGrid.implicitHeight + 12
            color: Utility.getAppHexColor("lightBackground")

            GridLayout {
                id: motorControlGrid
                anchors.fill: parent
                anchors.margins: 4
                columns: 9
                rowSpacing: 2
                columnSpacing: 4

                // ---- Row 0 ----
                // Col 0: dutyBox
                SpinBox {
                    id: dutyBox
                    Layout.fillWidth: true
                    Layout.row: 0; Layout.column: 0
                    from: -1000; to: 1000; stepSize: 10
                    value: 200
                    editable: true
                    property real realValue: value / 1000.0
                    textFromValue: function(value, locale) { return "D " + (value / 1000.0).toFixed(3) }
                    valueFromText: function(text, locale) {
                        var v = parseFloat(text.replace("D ", ""))
                        return Math.round(v * 1000)
                    }
                    font.family: "DejaVu Sans Mono"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Duty cycle")
                }
                // Col 1: dutyButton
                Button {
                    Layout.row: 0; Layout.column: 1
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Duty cycle")
                    onClicked: {
                        mCommands.setDutyCycle(dutyBox.realValue)
                        sendAliveButton.checked = true
                    }
                }
                // Col 2: speedBox
                SpinBox {
                    id: speedBox
                    Layout.fillWidth: true
                    Layout.row: 0; Layout.column: 2
                    from: -400000; to: 400000; stepSize: 100
                    value: 5000
                    editable: true
                    textFromValue: function(value, locale) { return "\u03C9 " + value + " RPM" }
                    valueFromText: function(text, locale) {
                        var v = parseInt(text.replace("\u03C9 ", "").replace(" RPM", ""))
                        return isNaN(v) ? 0 : v
                    }
                    font.family: "DejaVu Sans Mono"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Speed (RPM)")
                }
                // Col 3: speedButton
                Button {
                    Layout.row: 0; Layout.column: 3
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Speed")
                    onClicked: {
                        mCommands.setRpm(speedBox.value)
                        sendAliveButton.checked = true
                    }
                }
                // Col 4: brakeCurrentBox
                SpinBox {
                    id: brakeCurrentBox
                    Layout.fillWidth: true
                    Layout.row: 0; Layout.column: 4
                    from: -500000; to: 500000; stepSize: 500
                    value: 3000
                    editable: true
                    property real realValue: value / 1000.0
                    textFromValue: function(value, locale) { return "IB " + (value / 1000.0).toFixed(1) + " A" }
                    valueFromText: function(text, locale) {
                        var v = parseFloat(text.replace("IB ", "").replace(" A", ""))
                        return Math.round(v * 1000)
                    }
                    font.family: "DejaVu Sans Mono"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Brake current")
                }
                // Col 5: brakeCurrentButton
                Button {
                    Layout.row: 0; Layout.column: 5
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Brake Warning-96.png"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Brake current")
                    onClicked: {
                        mCommands.setCurrentBrake(brakeCurrentBox.realValue)
                        sendAliveButton.checked = true
                    }
                }
                // Col 6: fullBrakeButton (rowspan 2)
                Button {
                    id: fullBrakeButton
                    Layout.row: 0; Layout.column: 6; Layout.rowSpan: 2
                    Layout.fillHeight: true
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Anchor-96.png"
                    icon.width: 40; icon.height: 40
                    implicitWidth: 48
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Full brake")
                    onClicked: {
                        mCommands.setDutyCycle(0)
                        sendAliveButton.checked = true
                    }
                }
                // Col 7: stopButton (rowspan 2)
                Button {
                    id: stopButton
                    Layout.row: 0; Layout.column: 7; Layout.rowSpan: 2
                    Layout.fillHeight: true
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Stop Sign-96.png"
                    icon.width: 40; icon.height: 40
                    implicitWidth: 48
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Switch off")
                    onClicked: {
                        mCommands.setCurrent(0)
                        sendAliveButton.checked = false
                    }
                }
                // Col 8: dispDuty (row 0) — faithful DisplayBar replica
                Rectangle {
                    id: dispDutyBar
                    Layout.row: 0; Layout.column: 8
                    Layout.fillWidth: true
                    Layout.minimumWidth: 150
                    Layout.fillHeight: true
                    color: "black"
                    radius: 5
                    clip: true

                    property real dutyVal: 0.0
                    property real range: 100.0

                    // Top 30%: progress bar area
                    Item {
                        id: dutyBarArea
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.top: parent.top
                        height: parent.height * 0.3

                        // Positive bar (right of center)
                        Rectangle {
                            visible: dispDutyBar.dutyVal > 0
                            x: parent.width / 2 + 1
                            y: 1
                            width: Math.min(dispDutyBar.dutyVal / dispDutyBar.range, 1.0) * (parent.width / 2 - 2)
                            height: parent.height - 2
                            color: Utility.getAppHexColor("green")
                        }
                        // Negative bar (left of center)
                        Rectangle {
                            visible: dispDutyBar.dutyVal < 0
                            x: parent.width / 2 - 1 - width
                            y: 1
                            width: Math.min(Math.abs(dispDutyBar.dutyVal) / dispDutyBar.range, 1.0) * (parent.width / 2 - 2)
                            height: parent.height - 2
                            color: Utility.getAppHexColor("red")
                        }
                        // Center divider
                        Rectangle {
                            x: parent.width / 2 - 1; y: 0
                            width: 2; height: parent.height
                            color: Qt.rgba(0.59, 0.15, 0.15, 1.0)
                        }
                    }
                    // Horizontal divider
                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right
                        y: parent.height * 0.3 - 1
                        height: 2
                        color: Utility.getAppHexColor("darkAccent")
                    }
                    // Bottom 70%: name (left) | value (right)
                    Item {
                        anchors.left: parent.left; anchors.right: parent.right
                        y: parent.height * 0.3 + 1
                        height: parent.height * 0.7 - 1

                        Label {
                            anchors.left: parent.left
                            anchors.top: parent.top; anchors.bottom: parent.bottom
                            width: parent.width / 2 - 2
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: "Duty"
                            font.family: "DejaVu Sans Mono"
                            font.bold: true
                            font.pixelSize: Math.max(parent.height - 4, 10)
                            color: "white"
                        }
                        Label {
                            anchors.right: parent.right
                            anchors.top: parent.top; anchors.bottom: parent.bottom
                            width: parent.width / 2 - 2
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: dispDutyBar.dutyVal.toFixed(1) + " %"
                            font.family: "DejaVu Sans Mono"
                            font.bold: true
                            font.pixelSize: Math.max(parent.height - 4, 10)
                            color: "white"
                        }
                        // Center divider
                        Rectangle {
                            x: parent.width / 2 - 1; y: 0
                            width: 2; height: parent.height
                            color: Qt.rgba(0.59, 0.15, 0.15, 1.0)
                        }
                    }
                }

                // ---- Row 1 ----
                // Col 0: currentBox
                SpinBox {
                    id: currentBox
                    Layout.fillWidth: true
                    Layout.row: 1; Layout.column: 0
                    from: -5000000; to: 5000000; stepSize: 500
                    value: 3000
                    editable: true
                    property real realValue: value / 1000.0
                    textFromValue: function(value, locale) { return "I " + (value / 1000.0).toFixed(1) + " A" }
                    valueFromText: function(text, locale) {
                        var v = parseFloat(text.replace("I ", "").replace(" A", ""))
                        return Math.round(v * 1000)
                    }
                    font.family: "DejaVu Sans Mono"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Current")
                }
                // Col 1: currentButton
                Button {
                    Layout.row: 1; Layout.column: 1
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Current")
                    onClicked: {
                        mCommands.setCurrent(currentBox.realValue)
                        sendAliveButton.checked = true
                    }
                }
                // Col 2: posBox
                SpinBox {
                    id: posBox
                    Layout.fillWidth: true
                    Layout.row: 1; Layout.column: 2
                    from: 0; to: 360000; stepSize: 100
                    value: 0
                    editable: true
                    property real realValue: value / 1000.0
                    textFromValue: function(value, locale) { return "P " + (value / 1000.0).toFixed(1) + " \u00B0" }
                    valueFromText: function(text, locale) {
                        var v = parseFloat(text.replace("P ", "").replace(" \u00B0", ""))
                        return Math.round(v * 1000)
                    }
                    font.family: "DejaVu Sans Mono"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Position")
                }
                // Col 3: posButton
                Button {
                    Layout.row: 1; Layout.column: 3
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Position")
                    onClicked: {
                        mCommands.setPos(posBox.realValue)
                        sendAliveButton.checked = true
                    }
                }
                // Col 4: handbrakeBox
                SpinBox {
                    id: handbrakeBox
                    Layout.fillWidth: true
                    Layout.row: 1; Layout.column: 4
                    from: -500000; to: 500000; stepSize: 500
                    value: 3000
                    editable: true
                    property real realValue: value / 1000.0
                    textFromValue: function(value, locale) { return "HB " + (value / 1000.0).toFixed(1) + " A" }
                    valueFromText: function(text, locale) {
                        var v = parseFloat(text.replace("HB ", "").replace(" A", ""))
                        return Math.round(v * 1000)
                    }
                    font.family: "DejaVu Sans Mono"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Handbrake current")
                }
                // Col 5: handbrakeButton
                Button {
                    Layout.row: 1; Layout.column: 5
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Brake Warning-96.png"
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Handbrake current")
                    onClicked: {
                        mCommands.setHandbrake(handbrakeBox.realValue)
                        sendAliveButton.checked = true
                    }
                }
                // Col 6,7: fullBrake + stop already span from row 0

                // Col 8: dispCurrent (row 1) — faithful DisplayBar replica
                Rectangle {
                    id: dispCurrentBar
                    Layout.row: 1; Layout.column: 8
                    Layout.fillWidth: true
                    Layout.minimumWidth: 150
                    Layout.fillHeight: true
                    color: "black"
                    radius: 5
                    clip: true

                    property real currentVal: 0.0
                    property real currentRange: 60.0

                    // Top 30%: progress bar area
                    Item {
                        id: currentBarArea
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.top: parent.top
                        height: parent.height * 0.3

                        Rectangle {
                            visible: dispCurrentBar.currentVal > 0
                            x: parent.width / 2 + 1
                            y: 1
                            width: dispCurrentBar.currentRange > 0 ? Math.min(dispCurrentBar.currentVal / dispCurrentBar.currentRange, 1.0) * (parent.width / 2 - 2) : 0
                            height: parent.height - 2
                            color: Utility.getAppHexColor("green")
                        }
                        Rectangle {
                            visible: dispCurrentBar.currentVal < 0
                            x: parent.width / 2 - 1 - width
                            y: 1
                            width: dispCurrentBar.currentRange > 0 ? Math.min(Math.abs(dispCurrentBar.currentVal) / dispCurrentBar.currentRange, 1.0) * (parent.width / 2 - 2) : 0
                            height: parent.height - 2
                            color: Utility.getAppHexColor("red")
                        }
                        Rectangle {
                            x: parent.width / 2 - 1; y: 0
                            width: 2; height: parent.height
                            color: Qt.rgba(0.59, 0.15, 0.15, 1.0)
                        }
                    }
                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right
                        y: parent.height * 0.3 - 1
                        height: 2
                        color: Utility.getAppHexColor("darkAccent")
                    }
                    // Bottom 70%: name (left) | value (right)
                    Item {
                        anchors.left: parent.left; anchors.right: parent.right
                        y: parent.height * 0.3 + 1
                        height: parent.height * 0.7 - 1

                        Label {
                            anchors.left: parent.left
                            anchors.top: parent.top; anchors.bottom: parent.bottom
                            width: parent.width / 2 - 2
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: "Current"
                            font.family: "DejaVu Sans Mono"
                            font.bold: true
                            font.pixelSize: Math.max(parent.height - 4, 10)
                            color: "white"
                        }
                        Label {
                            anchors.right: parent.right
                            anchors.top: parent.top; anchors.bottom: parent.bottom
                            width: parent.width / 2 - 2
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: dispCurrentBar.currentVal.toFixed(2) + " A"
                            font.family: "DejaVu Sans Mono"
                            font.bold: true
                            font.pixelSize: Math.max(parent.height - 4, 10)
                            color: "white"
                        }
                        Rectangle {
                            x: parent.width / 2 - 1; y: 0
                            width: 2; height: parent.height
                            color: Qt.rgba(0.59, 0.15, 0.15, 1.0)
                        }
                    }
                }
            }
        }
    }

    // ---------------------------------------------------------------
    // Status bar (footer)
    // ---------------------------------------------------------------
    footer: Rectangle {
        height: 28
        color: Utility.getAppHexColor("lightBackground")

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 16

            Label {
                id: statusInfo
                text: ""
                color: Utility.getAppHexColor("lightText")
                font.pointSize: 12
                Layout.fillWidth: true
            }

            Label {
                id: connectedLabel
                text: VescIf.getConnectedPortName()
                color: Utility.getAppHexColor("lightText")
                font.pointSize: 12
            }
        }
    }

    // ---------------------------------------------------------------
    // Populate the page list on start (matches reloadPages() exactly)
    // ---------------------------------------------------------------
    Component.onCompleted: {
        function addPage(name, icon, groupIcon, bold, indent, pageSource, pageId, visible) {
            pageModel.append({
                name: name,
                iconFile: icon,
                groupIcon: groupIcon,
                bold: bold,
                indent: indent,
                pageSource: pageSource,
                pageId: pageId,
                isVisible: visible
            })
        }

        // --- Top-level (always visible) ---
        addPage("Welcome & Wizards", "icons/Home-96.png", "", true, false, "WelcomePage.qml", "welcome", true)
        addPage("Connection", "icons/Connected-96.png", "", true, false, "ConnectionPage.qml", "connection", true)
        addPage("Firmware", "icons/Electronics-96.png", "", true, false, "FirmwarePage.qml", "firmware", true)
        addPage("VESC Packages", "icons/Package-96.png", "", true, false, "PackagesPage.qml", "packages", true)

        // --- Motor Settings ---
        addPage("Motor Settings", "icons/motor.png", "", true, false, "", "motor", devShowAllPages)
        addPage("General", "icons/Horizontal Settings Mixer-96.png", "icons/mcconf.png", false, true, "MotorGeneralPage.qml", "motor_general", devShowAllPages)
        addPage("BLDC", "icons/bldc.png", "icons/mcconf.png", false, true, "", "motor_bldc", devShowAllPages)
        addPage("DC", "icons/Car Battery-96.png", "icons/mcconf.png", false, true, "", "motor_dc", devShowAllPages)
        addPage("FOC", "icons/3ph_sine.png", "icons/mcconf.png", false, true, "FocPage.qml", "motor_foc", devShowAllPages)
        addPage("GPDrive", "icons/3ph_sine.png", "icons/mcconf.png", false, true, "", "motor_gpdrive", devShowAllPages)
        addPage("PID Controllers", "icons/Speed-96.png", "icons/mcconf.png", false, true, "ControllersPage.qml", "motor_pid", devShowAllPages)
        addPage("Additional Info", "icons/About-96.png", "icons/mcconf.png", false, true, "MotorInfoPage.qml", "motor_additional_info", devShowAllPages)
        addPage("Experiments", "icons/Calculator-96.png", "icons/mcconf.png", false, true, "ExperimentsPage.qml", "motor_experiments", devShowAllPages)

        // --- App Settings ---
        addPage("App Settings", "icons/Outgoing Data-96.png", "", true, false, "", "app", devShowAllPages)
        addPage("General", "icons/Horizontal Settings Mixer-96.png", "icons/appconf.png", false, true, "AppGeneralPage.qml", "app_general", devShowAllPages)
        addPage("PPM", "icons/Controller-96.png", "icons/appconf.png", false, true, "AppPpmPage.qml", "app_ppm", devShowAllPages)
        addPage("ADC", "icons/Potentiometer-96.png", "icons/appconf.png", false, true, "AppAdcPage.qml", "app_adc", devShowAllPages)
        addPage("UART", "icons/Rs 232 Male-96.png", "icons/appconf.png", false, true, "AppUartPage.qml", "app_uart", devShowAllPages)
        addPage("VESC Remote", "icons/icons8-fantasy-96.png", "icons/appconf.png", false, true, "AppNunchukPage.qml", "app_vescremote", devShowAllPages)
        addPage("Nrf", "icons/Online-96.png", "icons/appconf.png", false, true, "AppNrfPage.qml", "app_nrf", devShowAllPages)
        addPage("PAS", "icons/icons8-fantasy-96.png", "icons/appconf.png", false, true, "AppPasPage.qml", "app_pas", devShowAllPages)
        addPage("IMU", "icons/Gyroscope-96.png", "icons/appconf.png", false, true, "AppImuPage.qml", "app_imu", devShowAllPages)

        // --- Custom Configs (hidden until package loaded) ---
        addPage("Config0", "icons/Electronics-96.png", "", true, false, "", "app_custom_config_0", false)
        addPage("Config1", "icons/Electronics-96.png", "", true, false, "", "app_custom_config_1", false)
        addPage("Config2", "icons/Electronics-96.png", "", true, false, "", "app_custom_config_2", false)

        // --- Data Analysis ---
        addPage("Data Analysis", "icons/Line Chart-96.png", "", true, false, "", "data_analysis", true)
        addPage("Realtime Data", "icons/rt_off.png", "", false, true, "RtDataPage.qml", "data_rt", devShowAllPages)
        addPage("Sampled Data", "icons/Line Chart-96.png", "", false, true, "SampledDataPage.qml", "data_sampled", devShowAllPages)
        addPage("Experiment Plot", "icons/rt_off.png", "", false, true, "ExperimentPlotPage.qml", "data_experiment", devShowAllPages)
        addPage("IMU Data", "icons/Gyroscope-96.png", "", false, true, "ImuPage.qml", "data_imu", false)
        addPage("BMS Data", "icons/icons8-battery-100.png", "", false, true, "BmsPage.qml", "data_bms", false)
        addPage("Log Analysis", "icons/Waypoint Map-96.png", "", false, true, "LogAnalysisPage.qml", "data_log", true)
        addPage("Motor Analysis", "icons/motor.png", "", false, true, "MotorComparisonPage.qml", "motor_comparison", true)

        // --- VESC Dev Tools ---
        addPage("VESC Dev Tools", "icons/v_icon-96.png", "", true, false, "", "dev_tools", true)
        addPage("Terminal", "icons/Console-96.png", "", false, true, "TerminalPage.qml", "terminal", true)
        addPage("QML Scripting", "icons_textedit/Outdent-96.png", "", false, true, "ScriptingPage.qml", "scripting", true)
        addPage("LispBM Scripting", "icons_textedit/Outdent-96.png", "", false, true, "LispPage.qml", "lisp", true)
        addPage("CAN Tools", "icons/can_off.png", "", false, true, "CanAnalyzerPage.qml", "can_tools", true)
        addPage("Display Tool", "icons/Calculator-96.png", "", false, true, "DisplayToolPage.qml", "display_tool", true)
        addPage("Debug Console", "icons/Bug-96.png", "", false, true, "", "debug_console", true)

        // --- Programmer pages ---
        addPage("SWD Programmer", "icons/Electronics-96.png", "", true, false, "", "swd_prog", true)
        addPage("ESP Programmer", "icons/Electronics-96.png", "", true, false, "", "esp_prog", true)

        // Initialize CAN list with local entry
        canModel.clear()
        canModel.append({ canId: -1 })

        pageStack.loadPage("WelcomePage.qml")
    }

    // ---------------------------------------------------------------
    // Helper: find page index by pageId
    // ---------------------------------------------------------------
    function findPageIndex(pageId) {
        for (var i = 0; i < pageModel.count; i++) {
            if (pageModel.get(i).pageId === pageId) {
                return i
            }
        }
        return -1
    }

    function setPageVisible(pageId, vis) {
        var idx = findPageIndex(pageId)
        if (idx >= 0) {
            pageModel.setProperty(idx, "isVisible", vis)
        }
    }

    // ---------------------------------------------------------------
    // Functions
    // ---------------------------------------------------------------
    function showStatusInfo(msg, isGood) {
        statusInfo.text = msg
        statusInfo.color = isGood ? Utility.getAppHexColor("lightAccent")
                                  : Utility.getAppHexColor("red")
        statusResetTimer.restart()
    }

    function showTextDialog(titleStr, body) {
        vescDialog.title = titleStr
        if (body.trim().startsWith("#")) {
            vescDialogLabel.textFormat = Text.MarkdownText
        } else {
            vescDialogLabel.textFormat = Text.RichText
        }
        vescDialogLabel.text = body
        vescDialogScroll.ScrollBar.vertical.position = 0
        vescDialog.open()
    }

    // ---------------------------------------------------------------
    // Timers
    // ---------------------------------------------------------------
    Timer {
        id: statusResetTimer
        interval: 3000
        onTriggered: {
            statusInfo.text = ""
            statusInfo.color = Utility.getAppHexColor("lightText")
        }
    }

    Timer {
        id: rtPollTimer
        interval: 50
        running: true
        repeat: true
        onTriggered: {
            if (VescIf.isPortConnected()) {
                if (rtDataButton.checked) {
                    mCommands.getValues()
                    mCommands.getValuesSetup()
                    mCommands.getStats(0xFFFFFFFF)
                }
                if (rtDataAppButton.checked) {
                    mCommands.getDecodedAdc()
                    mCommands.getDecodedChuk()
                    mCommands.getDecodedPpm()
                }
                if (imuButton.checked) {
                    mCommands.getImuData(0xFFFF)
                }
                if (bmsButton.checked) {
                    mCommands.bmsGetValues()
                }
            }
        }
    }

    Timer {
        id: confTimer
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            if (VescIf.isPortConnected() && VescIf.getLastFwRxParams().hwTypeStr() === "VESC") {
                if (!mcConfRead) {
                    mCommands.getMcconf()
                }
                if (!appConfRead) {
                    mCommands.getAppConf()
                }
            }
        }
    }

    Timer {
        id: uiTimer
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            connectedLabel.text = VescIf.getConnectedPortName()

            // Highlight reconnect when last port available
            if (!VescIf.isPortConnected() && VescIf.lastPortAvailable()) {
                reconnectButton.icon.source = "qrc" + Utility.getThemePath() + "icons/Connected-hl-96.png"
            } else {
                reconnectButton.icon.source = "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
            }
            reconnectButton.enabled = !VescIf.isPortConnected()
            disconnectButton.enabled = VescIf.isPortConnected()

            // Sync CAN fwd state
            if (canFwdButton.checked !== mCommands.getSendCan()) {
                canFwdButton.checked = mCommands.getSendCan()
            }
        }
    }

    // Send alive every ~200ms
    Timer {
        id: aliveTimer
        interval: 200
        running: true
        repeat: true
        onTriggered: {
            if (sendAliveButton.checked && VescIf.isPortConnected()) {
                mCommands.sendAlive()
            }
        }
    }

    // Auto scan CAN on connect
    Timer {
        id: canScanTimer
        interval: 500
        running: true
        repeat: true
        onTriggered: {
            if (VescIf.isPortConnected() && canModel.count <= 1 && VescIf.fwRx()) {
                mCommands.pingCan()
            }
        }
    }

    // ---------------------------------------------------------------
    // File Dialogs
    // ---------------------------------------------------------------
    FileDialog {
        id: motorConfSaveDialog
        title: qsTr("Save Motor Configuration XML")
        nameFilters: ["XML files (*.xml)"]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (!path.toLowerCase().endsWith(".xml")) path += ".xml"
            if (mMcConf.saveXml(path, "MCConfiguration")) {
                showStatusInfo("Saved motor configuration", true)
            } else {
                showStatusInfo("Could not save motor configuration", false)
            }
        }
    }
    FileDialog {
        id: motorConfLoadDialog
        title: qsTr("Load Motor Configuration XML")
        nameFilters: ["XML files (*.xml)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (mMcConf.loadXml(path, "MCConfiguration")) {
                showStatusInfo("Loaded motor configuration", true)
            } else {
                showStatusInfo("Could not load motor configuration", false)
            }
        }
    }
    FileDialog {
        id: appConfSaveDialog
        title: qsTr("Save App Configuration XML")
        nameFilters: ["XML files (*.xml)"]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (!path.toLowerCase().endsWith(".xml")) path += ".xml"
            if (mAppConf.saveXml(path, "APPConfiguration")) {
                showStatusInfo("Saved app configuration", true)
            } else {
                showStatusInfo("Could not save app configuration", false)
            }
        }
    }
    FileDialog {
        id: appConfLoadDialog
        title: qsTr("Load App Configuration XML")
        nameFilters: ["XML files (*.xml)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (mAppConf.loadXml(path, "APPConfiguration")) {
                showStatusInfo("Loaded app configuration", true)
            } else {
                showStatusInfo("Could not load app configuration", false)
            }
        }
    }

    // ---------------------------------------------------------------
    // Dialogs
    // ---------------------------------------------------------------
    Dialog {
        id: preferencesDialog
        title: qsTr("Preferences")
        modal: true
        standardButtons: Dialog.Close
        width: Math.min(600, appWindow.width - 40)
        height: Math.min(700, appWindow.height - 80)
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            GroupBox {
                title: "Dark Mode"
                Layout.fillWidth: true
                RowLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: "Use dark theme"
                        checked: Utility.isDarkMode()
                        onToggled: Utility.setDarkMode(checked)
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    Dialog {
        id: vescDialog
        standardButtons: Dialog.Ok
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: Math.min(600, appWindow.width - 40)
        height: Math.min(implicitHeight + 100, appWindow.height - 80)
        anchors.centerIn: parent

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        ScrollView {
            id: vescDialogScroll
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth

            Text {
                id: vescDialogLabel
                color: Utility.getAppHexColor("lightText")
                linkColor: Utility.getAppHexColor("lightAccent")
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                textFormat: Text.RichText
                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }
            }
        }
    }

    // ---------------------------------------------------------------
    // Connections
    // ---------------------------------------------------------------
    Connections {
        target: VescIf

        function onPortConnectedChanged() {
            connectedLabel.text = VescIf.getConnectedPortName()
            if (!VescIf.isPortConnected()) {
                mcConfRead = false
                appConfRead = false
                connected = false
                fwReadCorrectly = false

                // Hide motor/app/data pages (unless dev mode)
                if (!devShowAllPages) {
                    var hideIds = ["motor", "motor_general", "motor_bldc", "motor_dc",
                        "motor_foc", "motor_gpdrive", "motor_pid", "motor_additional_info",
                        "motor_experiments", "app", "app_general", "app_ppm", "app_adc",
                        "app_uart", "app_vescremote", "app_nrf", "app_pas", "app_imu",
                        "data_rt", "data_sampled"]
                    for (var i = 0; i < hideIds.length; i++) {
                        setPageVisible(hideIds[i], false)
                    }
                }

                // Clear CAN devices
                canModel.clear()
                canModel.append({ canId: -1 })
            } else {
                connected = true
            }
        }

        function onStatusMessage(msg, isGood) {
            showStatusInfo(msg, isGood)
        }

        function onMessageDialog(title, msg, isGood, richText) {
            if (!richText && msg.trim().startsWith("#")) {
                vescDialogLabel.textFormat = Text.MarkdownText
            } else {
                vescDialogLabel.textFormat = richText ? Text.RichText : Text.AutoText
            }
            vescDialog.title = title
            vescDialogLabel.text = (richText ? "<style>a:link { color: lightblue; }</style>" : "") + msg
            vescDialogScroll.ScrollBar.vertical.position = 0
            vescDialog.open()
        }

        function onFwRxChanged(rx, limited) {
            if (!rx) return
            fwReadCorrectly = true
            var params = VescIf.getLastFwRxParams()

            if (!devShowAllPages) {
                var vescIds = ["motor", "motor_general", "motor_bldc", "motor_dc",
                    "motor_foc", "motor_gpdrive", "motor_pid", "motor_additional_info",
                    "motor_experiments", "app", "app_general", "app_ppm", "app_adc",
                    "app_uart", "app_vescremote", "app_nrf", "app_pas", "app_imu",
                    "data_rt", "data_sampled"]
                var isVesc = (params.hwTypeStr() === "VESC")
                for (var i = 0; i < vescIds.length; i++) {
                    setPageVisible(vescIds[i], isVesc)
                }
            }

            if (VescIf.getFwSupportsConfiguration()) {
                confTimer.restart()
                mcConfRead = false
                appConfRead = false
                mCommands.getMcconf()
                mCommands.getAppConf()
            }
        }
    }

    Connections {
        target: mMcConf
        function onUpdated() {
            mcConfRead = true
            // Update motor type visibility
            if (!devShowAllPages && VescIf.getLastFwRxParams().hwTypeStr() === "VESC") {
                var motorType = mMcConf.getParamEnum("motor_type")
                setPageVisible("motor_bldc", motorType === 0)
                setPageVisible("motor_dc", motorType === 1)
                setPageVisible("motor_foc", motorType === 2)
                setPageVisible("motor_gpdrive", motorType === 3)
            }
            // Update current display range
            var maxCurrent = mMcConf.getParamDouble("l_current_max")
            if (maxCurrent > 0) {
                dispCurrentBar.currentRange = maxCurrent
            }
        }
    }

    Connections {
        target: mAppConf
        function onUpdated() {
            appConfRead = true
            // Update app type visibility
            if (!devShowAllPages && VescIf.getLastFwRxParams().hwTypeStr() === "VESC") {
                var appType = mAppConf.getParamEnum("app_to_use")
                setPageVisible("app_ppm", (appType === 1 || appType === 4 || appType === 8))
                setPageVisible("app_adc", (appType === 2 || appType === 5 || appType === 8 || appType === 10))
                setPageVisible("app_uart", (appType === 3 || appType === 4 || appType === 5 || appType === 8))
                setPageVisible("app_vescremote", (appType === 0 || appType === 3 || appType === 6 || appType === 7 || appType === 8))
                setPageVisible("app_pas", (appType === 9 || appType === 10))
            }
        }
    }

    Connections {
        target: mCommands

        function onPingCanRx(devs, isTimeout) {
            canModel.clear()
            canModel.append({ canId: -1 })
            for (var i = 0; i < devs.length; i++) {
                canModel.append({ canId: devs[i] })
            }
        }

        function onValuesReceived(values, mask) {
            dispDutyBar.dutyVal = values.duty_now * 100.0
            dispCurrentBar.currentVal = values.current_motor
        }
    }

    // ---------------------------------------------------------------
    // Keyboard handling
    // ---------------------------------------------------------------
    Shortcut {
        sequence: "Escape"
        onActivated: {
            mCommands.setCurrent(0)
            sendAliveButton.checked = false
        }
    }
    Shortcut {
        sequence: "Ctrl+R"
        onActivated: mCommands.getMcconf()
    }
    Shortcut {
        sequence: "Ctrl+W"
        onActivated: mCommands.setMcconf(true)
    }

    // Keyboard motor control via Shortcuts
    // Note: Shortcuts fire on press. We use a timer to send alive/release.
    Shortcut {
        sequence: "Up"
        enabled: keyboardButton.checked && VescIf.isPortConnected()
        onActivated: {
            mCommands.setCurrent(currentBox.realValue)
            sendAliveButton.checked = true
            keyReleaseTimer.restart()
        }
    }
    Shortcut {
        sequence: "Down"
        enabled: keyboardButton.checked && VescIf.isPortConnected()
        onActivated: {
            mCommands.setCurrent(-currentBox.realValue)
            sendAliveButton.checked = true
            keyReleaseTimer.restart()
        }
    }
    Shortcut {
        sequence: "PgDown"
        enabled: keyboardButton.checked && VescIf.isPortConnected()
        onActivated: {
            mCommands.setCurrentBrake(-currentBox.realValue)
            sendAliveButton.checked = true
            keyReleaseTimer.restart()
        }
    }
    Timer {
        id: keyReleaseTimer
        interval: 300
        repeat: false
        onTriggered: {
            mCommands.setCurrent(0.0)
            sendAliveButton.checked = false
        }
    }
}
