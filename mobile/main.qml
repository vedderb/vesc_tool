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

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
import QtQuick.Window
import QtCore as Core

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0
import Vedder.vesc.vesc3ditem 1.0

ApplicationWindow {
    id: appWindow
    Material.background: Utility.getAppHexColor("darkBackground")
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mAppConf: VescIf.appConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()
    property bool connected: false
    property bool fwReadCorrectly: false

    visible: true
    width: 500
    height: 850
    // Instead of the full safe area, see updateNotch()
    leftPadding: notchLeft
    rightPadding: notchRight
    title: qsTr("VESC Tool")

    // Full screen iPhone X workaround:
    property int notchLeft: 0
    property int notchRight: 0
    property int notchBot: 0
    property int notchTop: 0
    property bool mainIsHorizontal: appWindow.width > appWindow.height
    property bool mainIsTablet: appWindow.width >= 700 && Math.min(Screen.width, Screen.height) >= 600
    readonly property bool phoneLandscape: mainIsHorizontal && !mainIsTablet
    readonly property real canDockWidth: canDrawerLoader.item && canDrawerLoader.item.docked && canDrawerLoader.item.visible ? canDrawerLoader.item.width : 0

    // Connection status shown in the footer, the drawer and on the start page
    property string connName: ""
    property string connLink: ""
    property string hwName: ""
    property string hwType: ""
    property int fwCanId: -1
    // CAN ID of the connected device itself, as it reports it in values replies, or in its custom config
    // for the Express and BMS. The loaded configs can be from the device CAN forwarding selects.
    property int localCanId: -1
    // First motor controller on CAN when the connected device is not one, from the CAN scan
    property int canMotorId: -1
    property string fwVersion: ""
    property string fwLatest: ""
    readonly property bool fwUpdateAvailable: Number(fwLatest) > Number(fwVersion)
    property real rtVin: 0
    property real rtBattery: -1
    // Last BMS values with cell voltages, null while no BMS reports them
    property var bmsValues: null
    property real rtTempMos: 0
    property real rtTempMotor: 0
    property string faultText: ""
    property string statusFlash: ""
    property var warnings: []
    property int canCount: 0
    readonly property ConfigParams customConf: fwReadCorrectly && confCustomPage.visible && confCustomLoader.item ?
                                                   confCustomLoader.item.conf : null
    readonly property int customChangedCount: customConf ? customConf.changedParams.length : 0
    readonly property var writableConfigs: [fwReadCorrectly && confTimer.mcConfRx ? mMcConf : null,
                                            fwReadCorrectly && confTimer.appConfRx ? mAppConf : null, customConf]
    readonly property int changedCount: dirtyConfigs().reduce((count, conf) => count + conf.changedParams.length, 0)
    property alias lastDeviceName: lastDevice.name
    property alias lastDeviceTime: lastDevice.time
    property alias lastDeviceIcon: lastDevice.icon
    property alias lastDeviceAddr: lastDevice.addr
    property var lastPage: null

    // https://github.com/ekke/c2gQtWS_x/blob/master/qml/main.qml
    flags: Qt.platform.os === "ios" ? (Qt.Window | Qt.MaximizeUsingFullscreenGeometryHint) : Qt.Window

    function updateNotch() {
        // The iOS safe area is taller than the status bar and the home indicator need. In landscape it
        // is as wide on both sides, while only the side with the camera needs it
        var ios = Qt.platform.os === "ios"
        var cameraLeft = Screen.orientation === Qt.InvertedLandscapeOrientation
        notchTop = SafeArea.margins.top * (ios ? 0.85 : 1)
        notchLeft = SafeArea.margins.left * (ios && !cameraLeft ? 0.3 : 1)
        notchRight = SafeArea.margins.right * (ios && cameraLeft ? 0.3 : 1)
        notchBot = SafeArea.margins.bottom * (ios ? 0.6 : 1)
    }

    // Prefers the user's BLE name, then the hardware name, over the port address
    function updateConnectionInfo() {
        var m = VescIf.getConnectedPortName().match(/^Connected (?:\((.*)\) )?to (.*?)(, limited mode)?$/)
        var bleName = m && m[1] === "BLE" ? VescIf.getBleName(VescIf.getLastBleAddr()) : ""
        connLink = m ? [m[1], mCommands.getSendCan() ? "CAN " + mCommands.getCanSendId() : "",
                        m[3] ? qsTr("limited mode") : ""].filter(Boolean).join(" · ") : ""
        connName = bleName !== "" ? bleName : m && m[1] && hwName !== "" ? hwName.replace(/_/g, " ") : m ? m[2] : VescIf.getConnectedPortName()
    }

    // Routine messages flash in the status pill, problems keep it in a warning state until viewed
    function addMessage(msg, isGood) {
        messageModel.insert(0, {"msg": msg, "good": isGood, "time": new Date().toLocaleTimeString(Qt.locale(), Locale.ShortFormat)})
        if (messageModel.count > 50) {
            messageModel.remove(50)
        }

        if (isGood) {
            statusFlash = msg
            statusFlashTimer.restart()
        } else {
            warnings = warnings.concat([msg])
        }
    }

    function showToast(text, detail, kind, actionText, action) {
        toastText.text = text
        toastDetail.text = detail
        toast.kind = kind
        toastButton.text = actionText ? actionText : ""
        toast.action = action ? action : null
        toast.open()
        toastTimer.restart()
    }

    // Description of a fault from its name, like FAULT_CODE_DRV
    function faultDescription(name) {
        for (var code = 1; mCommands.faultToStr(code) !== "Unknown fault"; code++) {
            if (mCommands.faultToStr(code) === name) return Utility.faultToDescription(code)
        }
        return ""
    }

    function fwString(params) {
        return params.major + "." + ("0" + params.minor).slice(-2)
    }

    function dirtyConfigs() {
        return writableConfigs.filter(conf => conf && conf.changedParams.length > 0)
    }

    // Writes go to the device the config was read from, not to a CAN device that a scan is probing
    function writeReady(conf) {
        var ready = writableConfigs.includes(conf) && fwCanId === (mCommands.getSendCan() ? mCommands.getCanSendId() : -1)
        if (!ready) {
            showToast(qsTr("Not ready to write"), qsTr("Wait until the configuration is read from the VESC."), "warn")
        }
        return ready
    }

    function requestDisconnect() {
        if (disconnectTimer.running) return
        if (dirtyConfigs().length > 0) {
            disconnectSheet.open()
        } else {
            VescIf.disconnectPort()
        }
    }

    function reviewChanges(page) {
        var loader = page === confPageMotor ? confMotorLoader : page === confPageApp ? confAppLoader : confCustomLoader
        mainSwipeView.setCurrentIndex(page.SwipeView.index)
        loader.item.review()
    }

    // The app folder is shown as Vesc Tool in the Files app on iOS
    function displayPath(path) {
        var ind = path.indexOf("/Documents/")
        if (Qt.platform.os !== "ios" || ind < 0) {
            return path
        }
        return "Files › Vesc Tool › " + path.slice(ind + 11).split("/").join(" › ")
    }

    function timeAgo(time) {
        var minutes = Math.round((Date.now() - time) / 60000)
        return minutes < 2 ? qsTr("just now") :
               minutes < 60 ? qsTr("%1 min ago").arg(minutes) :
               minutes < 2880 ? qsTr("%1 h ago").arg(Math.round(minutes / 60)) :
                                qsTr("%1 days ago").arg(Math.round(minutes / 1440))
    }

    function tabIcon(title) {
        var icons = {
            "Start": "icons/Home-96.png",
            "RT Data": "icons/rt_off.png",
            "BMS": "icons/bms_off.png",
            "Profiles": "icons/Speed-96.png",
            "Motor Cfg": "icons/motor.png",
            "App Cfg": "icons/Outgoing Data-96.png",
            "Terminal": "icons/Console-96.png",
            "LispBM": "icons_textedit/Outdent-96.png",
            "LogView": "icons/Waypoint Map-96.png"
        }
        return "qrc" + Utility.getThemePath() + (icons[title] ? icons[title] : "icons/Electronics-96.png")
    }

    // Keys.onReleased: function(event) {
    //     if (event.key === Qt.Key_Back) {
    //         Qt.quit()
    //     }
    // }

    Timer {
        id: oriTimer
        interval: 100
        running: true
        repeat: true

        onTriggered: {
            updateNotch()
        }
    }

    Component.onCompleted: {
        if (!VescIf.isIntroDone()) {
            introWizard.openDialog()
        }
        updateNotch()
        Utility.keepScreenOn(VescIf.keepScreenOn())
        Utility.allowScreenRotation(VescIf.getAllowScreenRotation())
        Utility.stopGnssForegroundService()
    }

    SetupWizardIntro {
        id: introWizard
        dialogParent: mainSwipeView
    }

    // Feeds the Live Activity (iOS) or the ride notification (Android)
    readonly property var rideSession: Qt.platform.os === "ios" || Qt.platform.os === "android" ? RideSession : null

    onCanMotorIdChanged: {
        if (rideSession && canMotorId >= 0) {
            rideSession.canMotorFound(canMotorId)
        }
    }

    Controls {
        id: controls
        dialogParent: mainSwipeView
    }

    MultiSettings {
        id: multiSettings
        dialogParent: mainSwipeView
    }

    Loader {
        id: settingsLoader
        anchors.fill: parent
        asynchronous: true
        visible: status == Loader.Ready
        sourceComponent: Settings {
            id: settings
            dialogParent: mainSwipeView
        }
    }

    Loader {
        id: canDrawerLoader
        anchors.fill: parent
        asynchronous: true
        visible: status == Loader.Ready
        sourceComponent: Drawer {
            id: canDrawer
            property bool allowDrag: false
            // Stays open beside the page on landscape tablets
            readonly property bool docked: mainIsTablet && mainIsHorizontal
            edge: Qt.RightEdge
            width: docked ? 280 : Math.min(300, 0.8 * appWindow.width)
            height: appWindow.height > appWindow.width || docked ?  appWindow.height - footer.height - headerBar.height : appWindow.height
            y: appWindow.height > appWindow.width || docked ?  headerBar.height : 0
            dragMargin: 20
            modal: !docked
            closePolicy: docked ? Popup.NoAutoClose : Popup.CloseOnEscape | Popup.CloseOnReleaseOutside
            interactive: allowDrag && !docked
            onDockedChanged: if (!docked) close()

            Overlay.modal: Rectangle {
                color: Utility.getAppHexColor("overlay")
            }

            background: Rectangle {
                color: Utility.getAppHexColor("cardBackground")
                topLeftRadius: 20
                bottomLeftRadius: 20
            }

            CanScreen {
                id: canScreen
                anchors.fill: parent
                dialogParent: mainSwipeView
            }

            onVisibleChanged: {
                if (visible) {
                    canScreen.scanIfEmpty()
                }
            }
        }
    }

    Drawer {
        id: drawer
        width: Math.min(296, 0.8 * appWindow.width)
        height: appWindow.height > appWindow.width ?  appWindow.height - footer.height - headerBar.height : appWindow.height
        y: appWindow.height > appWindow.width ?  headerBar.height : 0
        dragMargin: 20
        interactive: false

        Overlay.modal: Rectangle {
            color: Utility.getAppHexColor("overlay")
        }

        background: Rectangle {
            color: Utility.getAppHexColor("cardBackground")
            topRightRadius: 20
            bottomRightRadius: 20
        }

        // Scrolls on landscape phones, where the menu is taller than the screen
        Flickable {
            id: drawerFlick
            anchors.fill: parent
            contentHeight: drawerColumn.implicitHeight + 28
            boundsBehavior: Flickable.StopAtBounds
            clip: true
        }

        ColumnLayout {
            id: drawerColumn
            parent: drawerFlick.contentItem
            x: 14
            y: 14
            width: drawerFlick.width - 28
            height: Math.max(implicitHeight, drawerFlick.height - 28)
            spacing: 0

            Image {
                id: image
                Layout.preferredWidth: parent.width * 0.6
                Layout.preferredHeight: (sourceSize.height * Layout.preferredWidth) / sourceSize.width
                Layout.margins: 6
                Layout.bottomMargin: 18
                source: "qrc" + Utility.getThemePath() + "/logo.png"
                antialiasing: true
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: connColumn.implicitHeight + 24
                radius: 12
                color: Utility.getAppHexColor("darkBackground")

                ColumnLayout {
                    id: connColumn
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 2

                    RowLayout {
                        spacing: 8

                        Rectangle {
                            width: 9
                            height: 9
                            radius: 4.5
                            color: connected ? Utility.getAppHexColor("positive") : Utility.getAppHexColor("disabledText")
                        }

                        Text {
                            Layout.fillWidth: true
                            text: connected ? connName : qsTr("Not connected")
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                            color: Utility.getAppHexColor("titleText")
                        }
                    }

                    Text {
                        Layout.leftMargin: 17
                        visible: connected
                        text: [connLink, "FW " + fwVersion].filter(Boolean).join(" · ")
                        font.pointSize: 9
                        color: Utility.getAppHexColor("normalText")
                    }

                    Button {
                        id: reconnectButton
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        text: connected ? "Disconnect" : "Connect"
                        highlighted: !connected
                        onClicked: {
                            if (connected) {
                                requestDisconnect()
                            } else {
                                connScreen.opened = true
                            }

                            drawer.close()
                        }
                    }
                }
            }

            Item {
                // Spacer
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            ItemDelegate {
                Layout.fillWidth: true
                text: qsTr("Messages")
                icon.source: "qrc" + Utility.getThemePath() + "icons_textedit/Bulleted List-96.png"
                onClicked: {
                    drawer.close()
                    messageLog.open()
                }
            }

            ItemDelegate {
                Layout.fillWidth: true
                text: "Settings"
                icon.source: "qrc" + Utility.getThemePath() + "icons/Settings-96.png"

                onClicked: {
                    drawer.close()
                    settingsLoader.item.openDialog()
                }
            }

            ItemDelegate {
                Layout.fillWidth: true
                text: "About"
                icon.source: "qrc" + Utility.getThemePath() + "icons/About-96.png"
                onClicked: {
                    VescIf.emitMessageDialog(
                                "About",
                                Utility.aboutText(),
                                true, true)
                }
            }

            ItemDelegate {
                Layout.fillWidth: true
                text: "Changelog"
                icon.source: "qrc" + Utility.getThemePath() + "icons_textedit/Numbered List-96.png"

                onClicked: {
                    VescIf.emitMessageDialog(
                                "VESC Tool Changelog",
                                Utility.vescToolChangeLog(),
                                true, false)
                }
            }

            ItemDelegate {
                Layout.fillWidth: true
                text: "License"
                icon.source: "qrc" + Utility.getThemePath() + "icons_textedit/Paste-96.png"

                onClicked: {
                    if (Qt.platform.os == "ios"){
                        VescIf.emitMessageDialog(
                                    mInfoConf.getLongName("ios_license_text"),
                                    mInfoConf.getDescription("ios_license_text"),
                                    true, true)
                    } else {
                        VescIf.emitMessageDialog(
                                    mInfoConf.getLongName("gpl_text"),
                                    mInfoConf.getDescription("gpl_text"),
                                    true, true)
                    }
                }
            }

            ItemDelegate {
                Layout.fillWidth: true
                text: "Privacy Policy"
                icon.source: "qrc" + Utility.getThemePath() + "icons/Globe-96.png"

                onClicked: {
                    Qt.openUrlExternally("https://vesc-project.com/privacy_policies")
                }
            }
        }
    }

    // Wheel and touchpad scrolling across the pages, as with a mouse on the desktop
    WheelPager {
        id: wheelPager
    }

    SwipeView {
        id: mainSwipeView
        currentIndex: tabBar.currentIndex
        anchors.fill: parent
        // The window already pads its content by the safe area
        anchors.rightMargin: controls.dockWidth + canDockWidth
        clip: true
        contentItem: ListView {
            model: mainSwipeView.contentModel
            interactive: mainSwipeView.interactive
            currentIndex: mainSwipeView.currentIndex

            spacing: mainSwipeView.spacing
            orientation: mainSwipeView.orientation
            snapMode: ListView.SnapOneItem
            boundsBehavior: Flickable.StopAtBounds

            // Only a swipe picks the page from the scroll position. Rotation resizes the pages
            // in several steps, and the old offset would otherwise show or select another page.
            highlightRangeMode: moving ? ListView.StrictlyEnforceRange : ListView.ApplyRange
            preferredHighlightBegin: 0
            preferredHighlightEnd: 0
            highlightMoveDuration: 250

            maximumFlickVelocity: 8 * (mainSwipeView.orientation ===
                                       Qt.Horizontal ? width : height)

            onContentWidthChanged: positionViewAtIndex(currentIndex, ListView.Beginning)
        }

        Component.onCompleted: wheelPager.attach(mainSwipeView)

        Page {
            Loader {
                anchors.fill: parent
                asynchronous: true
                visible: status == Loader.Ready
                sourceComponent: StartPage {
                    id: connBle
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    isHorizontal: mainIsHorizontal

                    onRequestOpenControls: {
                        controls.openDialog()
                    }

                    onRequestConnect: {
                        connScreen.opened = true
                    }

                    onRequestOpenMultiSettings: {
                        multiSettings.openDialog()
                    }
                }
            }
        }

        Page {
            id: rtDataPage

            header: Item {
                implicitHeight: phoneLandscape ? 40 : 44

                SegmentedBar {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.topMargin: phoneLandscape ? 4 : 8
                    model: [qsTr("Gauges"), qsTr("Dashboard"), qsTr("IMU"), qsTr("Statistics")]
                    currentIndex: rtSwipeView.currentIndex
                    onActivated: function(index) { rtSwipeView.setCurrentIndex(index) }
                }
            }

            SwipeView {
                id: rtSwipeView
                enabled: true
                clip: true
                currentIndex: 1
                anchors.fill:parent
                orientation: Qt.Vertical

                contentItem: ListView {
                    model: rtSwipeView.contentModel
                    interactive: rtSwipeView.interactive
                    currentIndex: rtSwipeView.currentIndex

                    spacing: rtSwipeView.spacing
                    orientation: rtSwipeView.orientation
                    snapMode: ListView.SnapOneItem
                    boundsBehavior: Flickable.StopAtBounds

                    // Same as for mainSwipeView, this one is vertical
                    highlightRangeMode: moving ? ListView.StrictlyEnforceRange : ListView.ApplyRange
                    preferredHighlightBegin: 0
                    preferredHighlightEnd: 0
                    highlightMoveDuration: 250

                    maximumFlickVelocity: 8 * (rtSwipeView.orientation ===
                                               Qt.Horizontal ? width : height)

                    onContentHeightChanged: positionViewAtIndex(currentIndex, ListView.Beginning)
                }

                Page {
                    Loader {
                        anchors.fill: parent
                        asynchronous: true
                        visible: status == Loader.Ready
                        sourceComponent: RtData {
                            anchors.fill: parent
                            updateData: tabBar.currentIndex === (1 + indexOffset()) && rtSwipeView.currentIndex === 0
                            isHorizontal: mainIsHorizontal
                        }
                    }
                }

                Page {
                    Loader {
                        anchors.fill: parent
                        asynchronous: true
                        visible: status == Loader.Ready
                        sourceComponent: RtDataSetup {
                            anchors.fill: parent
                            dialogParent: mainSwipeView
                            updateData: tabBar.currentIndex === (1 + indexOffset()) && rtSwipeView.currentIndex === 1
                            isHorizontal: mainIsHorizontal
                        }
                    }
                }

                // The model is beside the tiles on landscape phones and below them otherwise
                Page {
                    RtDataIMU {
                        id: rtIMU
                        anchors.top: parent.top
                        width: phoneLandscape ? parent.width * 0.6 : parent.width
                        Layout.fillWidth: true
                        z:2
                    }
                    CheckBox {
                        anchors.top: rtIMU.bottom
                        Layout.fillWidth: true
                        id: useYawBox
                        text: "Use Yaw (will drift)"
                        checked: false
                        z:2
                    }
                    Loader {
                        id: vesc3dLoader
                        asynchronous: true
                        visible: status == Loader.Ready
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: phoneLandscape ? parent.width * 0.3 : 0
                        anchors.verticalCenterOffset: phoneLandscape ? 0 : rtIMU.height/2
                        width: (phoneLandscape ? Math.min(parent.width * 0.4, parent.height) :
                                                 Math.min(parent.width, parent.height - rtIMU.height)) * Screen.devicePixelRatio
                        antialiasing: true
                        height: width
                        onLoaded: {
                            item.setRotation(0.1, 0.75, 0.4)
                        }
                        z:1
                        sourceComponent:
                            Vesc3dItem {
                            id: vesc3d
                            anchors.fill: parent
                            scale: 1.0 / Screen.devicePixelRatio
                            z:1
                        }
                    }
                }

                Page {
                    id: statPage
                    Loader {
                        anchors.fill: parent
                        anchors.margins: 10
                        asynchronous: true

                        sourceComponent: StatPage {
                            anchors.fill: parent
                            isHorizontal: mainIsHorizontal
                        }
                    }
                }
            }
        }

        Page {
            id: profilesPage
            Loader {
                anchors.fill: parent
                asynchronous: true
                visible: status == Loader.Ready
                sourceComponent: Profiles {
                    anchors.fill: parent
                    anchors.topMargin: 5
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    dialogParent: mainSwipeView
                }
            }
        }

        Page {
            id: logViewPage
            Loader {
                id: logViewLoader
                anchors.fill: parent
                asynchronous: true
                visible: status == Loader.Ready
                sourceComponent: LogAnalysis {
                    anchors.fill: parent
                }
            }
        }

        Page {
            id: terminalPage
            Loader {
                id: terminalLoader
                anchors.fill: parent
                asynchronous: true
                visible: status == Loader.Ready
                sourceComponent: Terminal {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.topMargin: 10
                }
            }
        }

        Page {
            Loader {
                anchors.fill: parent
                asynchronous: true
                visible: status == Loader.Ready
                sourceComponent: Lisp {
                    anchors.fill: parent
                }
            }
        }
    }

    header: Rectangle {
        id: headerBar
        color: Utility.getAppHexColor("darkBackground")
        height: tabBar.implicitHeight + notchTop // iPhone X Workaround

        // Fade out the tabs at an edge when more tabs are available beyond it
        Instantiator {
            model: 2

            Rectangle {
                parent: tabBar
                x: index === 0 ? 0 : parent.width - width
                width: 40
                height: parent.height - 3
                z: 1
                visible: index === 0 ? tabBar.contentItem.contentX > 1 :
                                       tabBar.contentItem.contentX + tabBar.width < tabBar.contentWidth - 1
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: index === 0 ? headerBar.color : "transparent" }
                    GradientStop { position: 1.0; color: index === 0 ? "transparent" : headerBar.color }
                }
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Utility.getAppHexColor("raisedBackground")
        }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: notchLeft
            anchors.rightMargin: notchRight
            spacing: 0

            TabBar {
                id: tabBar
                currentIndex: mainSwipeView.currentIndex
                Layout.fillWidth: true
                implicitWidth: 0
                clip: true
                // The stock snap scrolls a narrow first tab partly out of view
                Component.onCompleted: contentItem.snapMode = ListView.NoSnap

                background: Rectangle {
                    opacity: 1
                    color: "transparent"
                }

                Repeater {
                    id: rep
                    model: ["Start", "RT Data", "Profiles", "LogView", "Terminal", "LispBM"]

                    MainTabButton {
                        text: modelData
                    }
                }
            }

            ToolButton {
                Layout.fillHeight: true
                Layout.preferredWidth: 48
                icon.source: "qrc" + Utility.getThemePath() + "icons/Grid-96.png"
                Accessible.name: qsTr("All pages")
                onClicked: allPagesPopup.open()

                background: Rectangle {
                    color: allPagesPopup.visible ? Qt.alpha(Material.accent, 0.2) : "transparent"
                }
            }
        }
    }

    component MainTabButton: TabButton {
        id: mainTabButton
        property bool dot: false

        width: Math.max(implicitWidth, tabBar.width / tabBar.count)
        leftPadding: 14
        rightPadding: 14

        // A tab removed while checked would still look selected when added back
        onParentChanged: if (!parent) checked = false

        contentItem: Item {
            implicitWidth: tabRow.implicitWidth
            implicitHeight: tabRow.implicitHeight

            Row {
                id: tabRow
                anchors.centerIn: parent
                spacing: 5

                Text {
                    text: mainTabButton.text
                    font: mainTabButton.font
                    color: mainTabButton.checked ? Utility.getAppHexColor("accentText") : Utility.getAppHexColor("lightText")
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 7
                    height: 7
                    radius: 3.5
                    visible: mainTabButton.dot
                    color: Material.accent
                }
            }
        }
    }

    MainTabButton {
        id: uiHwButton
        visible: uiHwPage.visible
        text: "HwUi"
    }

    Page {
        id: uiHwPage
        visible: false

        Item {
            id: uiHw
            anchors.fill: parent
            property var tabBarItem: tabBar
            property var swipeViewItem: mainSwipeView
        }
    }

    MainTabButton {
        id: uiAppButton
        visible: uiAppPage.visible
        text: "AppUi"
    }

    Page {
        id: uiAppPage
        visible: false

        Item {
            id: uiApp
            anchors.fill: parent
            property var tabBarItem: tabBar
            property var swipeViewItem: mainSwipeView
        }
    }

    MainTabButton {
        id: bmsButton
        visible: bmsPage.visible
        text: "BMS"
    }

    Page {
        id: bmsPage
        visible: false

        Loader {
            anchors.fill: parent
            asynchronous: true
            visible: status == Loader.Ready
            sourceComponent: BMS {
                anchors.fill: parent
            }
        }
    }

    MainTabButton {
        id: confMotorButton
        visible: confPageMotor.visible
        text: "Motor Cfg"
        dot: mMcConf.changedParams.length > 0
    }

    MainTabButton {
        id: confAppButton
        visible: confPageApp.visible
        text: "App Cfg"
        dot: mAppConf.changedParams.length > 0
    }

    MainTabButton {
        id: confCustomButton
        dot: customChangedCount > 0
        visible: confCustomPage.visible
        text: "Custom Cfg"
    }

    Page {
        id: confPageMotor
        visible: false

        Loader {
            id: confMotorLoader
            anchors.fill: parent
            asynchronous: true
            sourceComponent: ConfigPageMotor {
                anchors.fill: parent
                dialogParent: mainSwipeView
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                isHorizontal: mainIsHorizontal
            }
        }
    }

    Page {
        id: confPageApp
        visible: false

        Loader {
            id: confAppLoader
            anchors.fill: parent
            asynchronous: true
            sourceComponent: ConfigPageApp {
                dialogParent: mainSwipeView
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                isHorizontal: mainIsHorizontal
                active: confPageApp.SwipeView.isCurrentItem
            }
        }
    }

    Page {
        id: confCustomPage
        visible: false

        Loader {
            id: confCustomLoader
            anchors.fill: parent
            asynchronous: true
            sourceComponent: ConfigPageCustom {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                isHorizontal: mainIsHorizontal
            }
        }
    }

    footer: Rectangle {
        id: connectedRect
        color: Utility.getAppHexColor("darkBackground")
        width: parent.width
        height: footerRow.height + notchBot

        Rectangle {
            width: parent.width
            height: 1
            color: Utility.getAppHexColor("raisedBackground")
        }

        RowLayout {
            id: footerRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: notchLeft
            anchors.rightMargin: notchRight
            height: phoneLandscape ? 44 : 52
            spacing: 0

            ToolButton {
                id: settingsButton
                Layout.fillHeight: true
                Layout.preferredWidth: 58
                icon.source: "qrc" + Utility.getThemePath() + "icons/Settings-96.png"
                icon.width: 26
                icon.height: 26
                Accessible.name: qsTr("Menu")
                onClicked: {
                    if (drawer.visible) {
                        drawer.close()
                    } else {
                        drawer.open()
                    }
                }
            }

            // Status pill: connection state, device and live values
            Rectangle {
                id: statusPill
                readonly property bool linkLost: bleDisconnectTimer.running
                readonly property bool warning: warnings.length > 0 && !linkLost && !(connected && faultText !== "")
                readonly property color dotColor: linkLost ? Utility.getAppHexColor("warning") :
                                                  connected && faultText !== "" ? Utility.getAppHexColor("danger") :
                                                  warning ? Utility.getAppHexColor("warning") :
                                                  !connected ? Utility.getAppHexColor("disabledText") :
                                                  !fwReadCorrectly ? Utility.getAppHexColor("lightAccent") :
                                                  changedCount > 0 ? Material.accent : Utility.getAppHexColor("positive")

                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: height / 2
                color: Utility.getAppHexColor("lightBackground")
                border.color: warning ? Qt.alpha(Utility.getAppHexColor("warning"), 0.6) : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Rectangle {
                        width: 9
                        height: 9
                        radius: 4.5
                        color: statusPill.dotColor
                    }

                    Text {
                        Layout.maximumWidth: statusPill.width * 0.5
                        elide: Text.ElideRight
                        font.weight: Font.Medium
                        color: faultText !== "" && connected ? Utility.getAppHexColor("dangerText") :
                               statusPill.warning ? Utility.getAppHexColor("warningText") : Utility.getAppHexColor("lightText")
                        text: statusPill.linkLost ? qsTr("Link lost") :
                              connected && faultText !== "" ? qsTr("FAULT") :
                              statusPill.warning ? (warnings.length > 1 ? qsTr("%1 warnings").arg(warnings.length) : qsTr("Warning")) :
                              !connected ? qsTr("Not connected") :
                              !fwReadCorrectly ? qsTr("Connecting…") : connName
                    }

                    Text {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font.pointSize: 10
                        color: statusPill.warning ? Utility.getAppHexColor("lightText") :
                               statusFlash !== "" && connected ? Utility.getAppHexColor("positiveText") :
                               changedCount > 0 && connected && faultText === "" ? Utility.getAppHexColor("accentText") :
                                                                                     Utility.getAppHexColor("normalText")
                        text: statusPill.linkLost ? qsTr("Reconnecting %1 / 5").arg(6 - bleDisconnectTimer.trysLeft) :
                              connected && faultText !== "" ? faultText :
                              statusPill.warning ? warnings[warnings.length - 1] :
                              !connected ? qsTr("Tap to connect") :
                              statusFlash !== "" ? statusFlash :
                              !fwReadCorrectly ? connName :
                              changedCount > 0 && !mainIsTablet ? qsTr("%1 unwritten").arg(changedCount) :
                              [changedCount > 0 ? qsTr("%1 unwritten").arg(changedCount) : "", connLink,
                               rtVin > 0 ? rtVin.toFixed(1) + " V" : "",
                               mainIsTablet && rtVin > 0 && rtBattery >= 0 ? Math.round(rtBattery * 100) + " %" : "",
                               rtVin > 0 ? (mainIsTablet ? qsTr("FET %1 °C") : "%1 °C").arg(rtTempMos.toFixed(0)) : "",
                               mainIsTablet && rtVin > 0 ? qsTr("Motor %1 °C").arg(rtTempMotor.toFixed(0)) : ""].filter(Boolean).join(" · ")
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (statusPill.warning) {
                            messageLog.open()
                        } else if (connected) {
                            connectionSheet.open()
                        } else {
                            connScreen.opened = true
                        }
                    }
                    onPressAndHold: messageLog.open()
                }
            }

            ToolButton {
                Layout.fillHeight: true
                Layout.preferredWidth: 58
                icon.source: "qrc" + Utility.getThemePath() + "icons/can_off.png"
                icon.width: 26
                icon.height: 26
                Accessible.name: qsTr("CAN devices")
                onClicked: {
                    if (canDrawerLoader.item.visible) {
                        canDrawerLoader.item.close()
                    } else {
                        canDrawerLoader.item.open()
                    }
                }

                Rectangle {
                    x: parent.width / 2 + 6
                    y: 9
                    width: Math.max(height, canCountText.implicitWidth + 8)
                    height: 16
                    radius: 8
                    visible: connected && canCount > 0
                    color: Material.accent

                    Text {
                        id: canCountText
                        anchors.centerIn: parent
                        text: canCount
                        font.pointSize: 9
                        font.weight: Font.Bold
                        color: Utility.getAppHexColor("white")
                    }
                }
            }
        }
    }

    Rectangle {
        parent: Overlay.overlay
        anchors.fill: parent
        color: "black"

        ConnectScreen {
            id: connScreen
            x: 0
            y: 0
            height: parent.height
            width: parent.width
            opened: true
            fullLogo: true

            dialogParent: mainSwipeView

            Component.onCompleted: {
                startBleScan()
            }

            onYChanged: {
                parent.color.a = Math.min(1, Math.max(1 - y / height, 0))

                if (opened) {
                    drawer.interactive = false
                    canDrawerLoader.item.allowDrag = false
                    drawer.close()
                    canDrawerLoader.item.close()
                } else {
                    drawer.interactive = true
                    canDrawerLoader.item.allowDrag = true
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
            updateConnectionInfo()
            canCount = VescIf.getCanDevsLast().length
        }
    }

    Timer {
        id: confTimer
        interval: 1000
        running: true
        repeat: true

        // Null while waiting for a read, false if it could not be decoded.
        property var mcConfRx: null
        property var appConfRx: null

        onTriggered: {
            if (VescIf.isPortConnected() && VescIf.getLastFwRxParams().hwTypeStr() === "VESC") {
                if (mcConfRx === null) {
                    mCommands.getMcconf()
                }

                if (appConfRx === null) {
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

                    if (mainSwipeView.currentItem == rtDataPage && rtSwipeView.currentIndex === 3) {
                        mCommands.getStats(0xFFFFFFFF)
                    }

                    if (mainSwipeView.currentItem == bmsPage) {
                        mCommands.bmsGetValues()
                    }
                } else {
                    if (mainSwipeView.currentItem == rtDataPage && rtSwipeView.currentIndex === 0) {
                        interval = 50
                        mCommands.getValues()
                    }

                    if (mainSwipeView.currentItem == rtDataPage && rtSwipeView.currentIndex === 1) {
                        interval = 50
                        mCommands.getValuesSetup()
                        mCommands.getImuData(0x2)
                    }

                    if (mainSwipeView.currentItem == rtDataPage && rtSwipeView.currentIndex === 2) {
                        interval = 20
                        mCommands.getImuData(0x1FF)
                    }

                    if (mainSwipeView.currentItem == rtDataPage && rtSwipeView.currentIndex === 3) {
                        interval = 100
                        mCommands.getValuesSetupSelective(0x7E00)
                        mCommands.getStats(0xFFFFFFFF)
                    }

                    if (mainSwipeView.currentItem == bmsPage) {
                        interval = 100
                        mCommands.bmsGetValues()
                    }

                    if (mainSwipeView.currentItem != rtDataPage && mainSwipeView.currentItem != bmsPage) {
                        interval = 1000
                    }
                }
            }
        }
    }

    // Slow update of the values shown in the footer and on the start page, where the
    // RT data polling does not include them
    Timer {
        interval: 1000
        running: true
        repeat: true

        onTriggered: {
            var polled = VescIf.isRtLogOpen() ||
                    (mainSwipeView.currentItem == rtDataPage && rtSwipeView.currentIndex < 2)

            if (VescIf.isPortConnected() && !polled &&
                    (hwType === "VESC" || canMotorId >= 0) && VescIf.getFwUploadProgress() < 0) {
                if (hwType === "VESC") {
                    // With vesc_id for localCanId
                    mCommands.getValuesSetupSelective(0x30183)
                } else {
                    VescIf.canTmpOverride(true, canMotorId)
                    mCommands.getValuesSetupSelective(0x10183)
                    VescIf.canTmpOverrideEnd()
                }
            }
        }
    }

    // Polls the BMS at 1 Hz. Its page hides after 4 polls without cell voltages.
    Timer {
        id: bmsTimer
        interval: 1000
        running: fwReadCorrectly
        repeat: true
        property int misses: 0

        onTriggered: {
            if (++misses > 4) {
                setBmsPageVisible(false)
            }

            if (VescIf.getFwUploadProgress() < 0) {
                mCommands.bmsGetValues()
            }
        }
    }

    Timer {
        id: bleDisconnectTimer
        interval: 1000
        running: false
        repeat: true
        property int trysLeft: 0

        onTriggered: {
            if(trysLeft < 1 || fwReadCorrectly) {
                bleDisconnectTimer.stop()
                connScreen.opened = VescIf.isPortConnected() ? false : true
                return
            }
            if(VescIf.getLastBleAddr().length > 0) {
                VescIf.connectBle(VescIf.getLastBleAddr())
            }
            trysLeft = trysLeft - 1
        }
    }

    SheetDialog {
        id: vescDialog
        acceptText: qsTr("OK")
        rejectText: ""

        ScrollView {
            id: vescDialogScroll
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth

            Text {
                id: vescDialogLabel
                color: {color = Utility.getAppHexColor("lightText")}
                linkColor: {linkColor = Utility.getAppHexColor("lightAccent")}
                verticalAlignment: Text.AlignVCenter
                width: vescDialogScroll.availableWidth
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                textFormat: Text.RichText
                onLinkActivated: {
                    Qt.openUrlExternally(link)
                }
            }
        }
    }

    property var hwUiObj: 0
    property var appUiObj: 0

    function updateHwAppUi () {
        if (hwUiObj != 0) {
            hwUiObj.destroy()
            hwUiObj = 0
        }

        if (appUiObj != 0) {
            appUiObj.destroy()
            appUiObj = 0
        }

        mainSwipeView.interactive = true
        headerBar.visible = true
        tabBar.enabled = true

        if (VescIf.isPortConnected() && VescIf.qmlHwLoaded()) {
            if (VescIf.getLastFwRxParams().qmlHwFullscreen) {
                mainSwipeView.interactive = false
                headerBar.visible = false
                tabBar.enabled = false
            }

            hwUiObj = Qt.createQmlObject(VescIf.qmlHw(), uiHw, "HwUi")
            setPageShown(uiHwPage, uiHwButton, true, 1)
            uiHwPage.visible = true

            uiHwButton.text = "HwUi"
            if (hwUiObj.tabTitle) {
                uiHwButton.text = hwUiObj.tabTitle
            }

            if (VescIf.getLastFwRxParams().qmlHwFullscreen) {
                mainSwipeView.setCurrentIndex(0)
                mainSwipeView.setCurrentIndex(1)
            }
        } else {
            uiHwPage.visible = false
            setPageShown(uiHwPage, uiHwButton, false)
        }

        if (VescIf.isPortConnected() && VescIf.qmlAppLoaded()) {
            if (VescIf.getLastFwRxParams().qmlAppFullscreen) {
                mainSwipeView.interactive = false
                headerBar.visible = false
                tabBar.enabled = false
            }

            appUiObj = Qt.createQmlObject(VescIf.qmlApp(), uiApp, "AppUi")
            setPageShown(uiAppPage, uiAppButton, true, 1)
            uiAppPage.visible = true

            uiAppButton.text = "AppUi"
            if (appUiObj.tabTitle) {
                uiAppButton.text = appUiObj.tabTitle
            }

            if (VescIf.getLastFwRxParams().qmlAppFullscreen) {
                mainSwipeView.setCurrentIndex(0)
                mainSwipeView.setCurrentIndex(1)
            }
        } else {
            uiAppPage.visible = false
            setPageShown(uiAppPage, uiAppButton, false)
        }
    }

    Timer {
        id: confCustomTimer
        running: false
        triggeredOnStart: true
        interval: 500
        repeat: true
        onTriggered: {
            if (confCustomLoader.status == Loader.Ready) {
                stop()

                if (VescIf.isPortConnected() && VescIf.customConfig(0) !== null) {
                    setPageShown(confCustomPage, confCustomButton, true, profilesPage.SwipeView.index + 1)
                    confCustomPage.visible = true
                    confCustomLoader.item.reloadConfig()
                    confCustomButton.text = VescIf.customConfig(0).getLongName("hw_name")
                } else {
                    confCustomPage.visible = false
                    setPageShown(confCustomPage, confCustomButton, false)
                }
            }
        }
    }

    function updateConfCustom () {
        confCustomTimer.start()
    }

    // Adds or removes a page and its tab, keeping the shown page, or lastPage once it is back.
    // The reminder is paused so that lastPage stays the page the user picked.
    function setPageShown(page, button, show, index) {
        var shown = mainSwipeView.currentItem
        pageReminder.enabled = false
        if (show) {
            mainSwipeView.insertItem(index, page)
            tabBar.insertItem(index, button)
            wheelPager.attach(page)
        } else {
            page.parent = null
            button.parent = null
        }

        if (shown !== null && shown.SwipeView.index >= 0) {
            mainSwipeView.setCurrentIndex(shown.SwipeView.index)
        }
        if (lastPage !== null && lastPage.SwipeView.index >= 0) {
            mainSwipeView.setCurrentIndex(lastPage.SwipeView.index)
        }
        pageReminder.enabled = true
    }

    function setBmsPageVisible(show) {
        if (show === bmsPage.visible) {
            return
        }

        bmsPage.visible = show
        if (show) {
            setPageShown(bmsPage, bmsButton, true, rtDataPage.SwipeView.index + 1)
        } else {
            bmsValues = null
            setPageShown(bmsPage, bmsButton, false)
        }
    }

    function indexOffset() {
        var res = 0
        if (uiHwButton.visible) {
            res++
        }
        if (uiAppButton.visible) {
            res++
        }
        return res
    }

    Connections {
        target: VescIf
        function onPortConnectedChanged() {
            updateConnectionInfo()
            if (!VescIf.isPortConnected()) {
                confTimer.mcConfRx = null
                confTimer.appConfRx = null
                connected = false
                fwReadCorrectly = false
                hwName = ""
                hwType = ""
                canMotorId = -1
                localCanId = -1
                rtVin = 0
                rtBattery = -1
                faultText = ""
                setBmsPageVisible(false)
            } else {
                connected = true
            }

            if (VescIf.useWakeLock()) {
                VescIf.setWakeLock(VescIf.isPortConnected())
            }
            if(!bleDisconnectTimer.running) {
                connScreen.opened = VescIf.isPortConnected() ? false : true
            }
        }

        function onUnintentionalBleDisconnect() {
            bleDisconnectTimer.trysLeft = 5
            bleDisconnectTimer.start()
        }

        function onStatusMessage(msg, isGood) {
            addMessage(msg, isGood)
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
            if (rx) {
                fwCanId = mCommands.getSendCan() ? mCommands.getCanSendId() : -1
                if (VescIf.getFwSupportsConfiguration()) {
                    confPageMotor.visible = true
                    confPageApp.visible = true

                    var ind = profilesPage.SwipeView.index + 1
                    setPageShown(confPageApp, confAppButton, true, ind)
                    setPageShown(confPageMotor, confMotorButton, true, ind)
                } else {
                    confPageMotor.visible = false
                    confPageApp.visible = false
                    setPageShown(confPageMotor, confMotorButton, false)
                    setPageShown(confPageApp, confAppButton, false)
                }

                if (VescIf.getFwSupportsConfiguration()) {
                    confTimer.restart()
                    confTimer.mcConfRx = null
                    confTimer.appConfRx = null

                    mCommands.getMcconf()
                    mCommands.getAppConf()
                }

                fwReadCorrectly = true
                bleDisconnectTimer.stop()

                var p = VescIf.getLastFwRxParams()
                var fws = VescIf.getSupportedFirmwares()
                hwName = p.hw
                hwType = p.hwTypeStr()
                fwVersion = fwString(p)
                fwLatest = fws.reduce(function(a, b) { return Number(b) > Number(a) ? b : a }, fwVersion)
                updateConnectionInfo()
                lastDevice.name = connName
                lastDevice.time = Date.now()
                // connectTcp() clears the hub ID, so it tells a hub link from plain TCP
                var tcp = connLink.startsWith("TCP") && VescIf.getLastTcpHubVescID() === ""
                lastDevice.icon = connLink.startsWith("BLE") ? "icons/bluetooth.png" :
                                  connLink.startsWith("serial") ? "icons/USB-96.png" :
                                  tcp ? "icons/LAN-96.png" :
                                  connLink.startsWith("TCP") ? "icons/Globe-96.png" : "icons/Connected-96.png"
                // Address in the scan list, for the devices that a scan can find
                lastDevice.addr = connLink.startsWith("BLE") ? VescIf.getLastBleAddr() :
                                  tcp ? VescIf.getLastTcpServer() : ""
            } else {
                disconnectTimer.stop()
                disconnectSheet.close()
                updateConfCustom()
            }

            updateHwAppUi()
        }

        function onQmlLoadDone() {
            // The QML on the Virtual ESC is from VESC Tool or uploaded by the user
            if (VescIf.askQmlLoad() && !VescIf.isVirtualConnected()) {
                qmlLoadDialog.open()
            } else {
                updateHwAppUi()
            }
        }

        function onCustomConfigLoadDone() {
            updateConfCustom()
        }
    }

    Connections {
        target: mMcConf
        function onWriteConfirmed() { disconnectTimer.confirm(mMcConf) }

        function onReferenceStored() {
            confTimer.mcConfRx = true
        }
    }

    Connections {
        target: mAppConf
        function onWriteConfirmed() { disconnectTimer.confirm(mAppConf) }

        function onReferenceStored() {
            confTimer.appConfRx = true
        }
    }

    Connections {
        target: customConf
        function onWriteConfirmed() { disconnectTimer.confirm(customConf) }

        // VESC Express and BMS keep their CAN ID in the custom config
        function onReferenceStored() {
            if (!mCommands.getSendCan() && !VescIf.getFwSupportsConfiguration() && customConf.hasParam("controller_id")) {
                localCanId = customConf.getReference().getParamInt("controller_id")
            }
        }
    }

    Connections {
        target: mCommands

        function onValuesReceived(values, mask) {
            if (mask & (1 << 0)) rtTempMos = values.temp_mos
            if (mask & (1 << 1)) rtTempMotor = values.temp_motor
            if (mask & (1 << 8)) rtVin = values.v_in
            if (mask & (1 << 15)) faultText = values.fault_code !== 0 ? values.fault_str : ""
            if ((mask & (1 << 17)) && values.vesc_id !== 255 && hwType === "VESC" && !mCommands.getSendCan()) localCanId = values.vesc_id
        }

        function onValuesSetupReceived(values, mask) {
            if (mask & (1 << 0)) rtTempMos = values.temp_mos
            if (mask & (1 << 1)) rtTempMotor = values.temp_motor
            if (mask & (1 << 7)) rtVin = values.v_in
            if (mask & (1 << 8)) rtBattery = values.battery_level
            if (mask & (1 << 16)) faultText = values.fault_code !== 0 ? values.fault_str : ""
            if ((mask & (1 << 17)) && values.vesc_id !== 255 && hwType === "VESC" && !mCommands.getSendCan()) localCanId = values.vesc_id
        }

        function onBmsValuesRx(val) {
            if (val.v_cells.length > 0) {
                bmsTimer.misses = 0
                bmsValues = val
                setBmsPageVisible(true)
            }
        }

        function onValuesImuReceived(values, mask) {
            if (tabBar.currentIndex === (1 + indexOffset()) && rtSwipeView.currentIndex === 2) {
                vesc3dLoader.item.setRotation(values.roll, values.pitch,
                                              useYawBox.checked ? values.yaw : 0)
                rtIMU.updateText(values)
            }
        }

        function onDeserializeConfigFailed(isMc, isApp) {
            if (isMc) {
                confTimer.mcConfRx = false
            }

            if (isApp) {
                confTimer.appConfRx = false
            }
        }
    }

    // Remind about unwritten changes when leaving a configuration page
    Connections {
        id: pageReminder
        target: mainSwipeView

        function onCurrentItemChanged() {
            var page = lastPage
            var count = page === confPageMotor ? mMcConf.changedParams.length :
                        page === confPageApp ? mAppConf.changedParams.length :
                        page === confCustomPage ? customChangedCount : 0

            if (count > 0 && page.SwipeView.index >= 0) {
                var name = page === confPageMotor ? confMotorButton.text :
                           page === confPageApp ? confAppButton.text : confCustomButton.text
                showToast(qsTr("%1 unwritten in %2").arg(count).arg(name),
                          "", "warn", qsTr("Review"), function() {
                              reviewChanges(page)
                          })
            }

            lastPage = mainSwipeView.currentItem
        }
    }

    Core.Settings {
        id: lastDevice
        category: "lastDevice"
        property string name: ""
        property real time: 0
        property string icon: "icons/Connected-96.png"
        property string addr: ""
    }

    ListModel {
        id: messageModel
    }

    TextEdit {
        id: clipboardHelper
        visible: false
    }

    Popup {
        id: toast
        property string kind: "ok"
        property var action: null

        parent: Overlay.overlay
        x: 12 + notchLeft
        y: parent.height - footer.height - height - 10
        width: parent.width - 24 - notchLeft - notchRight
        padding: 0
        closePolicy: Popup.NoAutoClose

        background: Rectangle {
            radius: 12
            color: Utility.getAppHexColor("raisedBackground")
            border.color: Utility.getAppHexColor("lightestBackground")

            Rectangle {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 4
                height: parent.height - 16
                radius: 2
                color: toast.kind === "warn" ? Utility.getAppHexColor("warning") :
                       toast.kind === "info" ? Utility.getAppHexColor("lightAccent") : Utility.getAppHexColor("positive")
            }
        }

        contentItem: RowLayout {
            spacing: 10

            Image {
                Layout.leftMargin: 22
                Layout.preferredWidth: 22
                Layout.preferredHeight: 22
                source: "qrc" + Utility.getThemePath() + (toast.kind === "warn" ? "icons/Brake Warning-96.png" :
                                                          toast.kind === "info" ? "icons/About-96.png" : "icons/Ok-96.png")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 10
                Layout.bottomMargin: 10
                spacing: 1

                Text {
                    id: toastText
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    font.weight: Font.Medium
                    color: Utility.getAppHexColor("lightText")
                }

                Text {
                    id: toastDetail
                    Layout.fillWidth: true
                    visible: text !== ""
                    elide: Text.ElideRight
                    font.pointSize: 9
                    color: Utility.getAppHexColor("normalText")
                }
            }

            Button {
                id: toastButton
                visible: text !== ""
                flat: true
                Material.foreground: Utility.getAppHexColor("accentText")
                onClicked: {
                    toast.close()
                    toast.action()
                }
            }
        }

        TapHandler {
            onTapped: toast.close()
        }

        Timer {
            id: toastTimer
            interval: 3000
            onTriggered: toast.close()
        }
    }

    Timer {
        id: statusFlashTimer
        interval: 3000
        onTriggered: statusFlash = ""
    }

    SheetDialog {
        id: messageLog
        title: qsTr("Messages")
        onOpened: warnings = []

        ListView {
            anchors.fill: parent
            implicitHeight: Math.max(contentHeight, 60)
            clip: true
            model: messageModel

            delegate: RowLayout {
                width: ListView.view.width
                height: Math.max(40, implicitHeight + 12)
                spacing: 10

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: good ? Utility.getAppHexColor("positiveText") : Utility.getAppHexColor("dangerText")
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    Text {
                        Layout.fillWidth: true
                        text: msg
                        elide: Text.ElideRight
                        color: Utility.getAppHexColor("lightText")
                    }

                    // Faults are logged by name, with what they mean below
                    Text {
                        Layout.fillWidth: true
                        visible: text !== ""
                        text: faultDescription(msg)
                        wrapMode: Text.Wrap
                        font.pointSize: 9
                        color: Utility.getAppHexColor("normalText")
                    }
                }

                Text {
                    text: time
                    font.pointSize: 9
                    color: Utility.getAppHexColor("normalText")
                }
            }
        }
    }

    SheetDialog {
        id: connectionSheet

        title: connName
        iconSource: "qrc" + Utility.getThemePath() + "icons/v_icon-96.png"
        acceptText: qsTr("Disconnect")
        rejectText: qsTr("Close")
        onAccepted: requestDisconnect()

        function diagnostics() {
            return [qsTr("Connection: %1").arg(VescIf.getConnectedPortName()),
                    qsTr("Hardware: %1").arg(hwName),
                    qsTr("Firmware: %1").arg(fwVersion),
                    qsTr("UUID: %1").arg(VescIf.getConnectedUuid()),
                    qsTr("Battery: %1 V").arg(rtVin.toFixed(1)),
                    qsTr("Fault: %1").arg(faultText !== "" ? faultText : "FAULT_CODE_NONE")].join("\n")
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: connectionSheet.visible ? connectionSheet.diagnostics() : ""
                wrapMode: Text.WrapAnywhere
                lineHeight: 1.3
                font.family: "DejaVu Sans Mono"
                font.pointSize: 10
                color: Utility.getAppHexColor("lightText")
            }

            Text {
                Layout.fillWidth: true
                visible: text !== ""
                text: connectionSheet.visible ? faultDescription(faultText) : ""
                wrapMode: Text.Wrap
                color: Utility.getAppHexColor("dangerText")
            }

            Text {
                Layout.fillWidth: true
                visible: canCount > 0
                text: qsTr("CAN devices: %1").arg(VescIf.getCanDevsLast().join(", "))
                color: Utility.getAppHexColor("normalText")
            }

            RowLayout {
                Button {
                    flat: true
                    text: qsTr("Copy diagnostics")
                    icon.source: "qrc" + Utility.getThemePath() + "icons_textedit/Copy-96.png"
                    onClicked: {
                        clipboardHelper.text = connectionSheet.diagnostics()
                        clipboardHelper.selectAll()
                        clipboardHelper.copy()
                        showToast(qsTr("Diagnostics copied"), "", "info")
                    }
                }

                Button {
                    flat: true
                    visible: tabBar.enabled
                    text: qsTr("Fault log")
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Console-96.png"
                    onClicked: {
                        connectionSheet.close()
                        mainSwipeView.setCurrentIndex(terminalPage.SwipeView.index)
                        terminalLoader.item.send("faults")
                    }
                }
            }
        }
    }

    SheetDialog {
        id: disconnectSheet
        title: qsTr("Write changes before disconnecting?")
        acceptText: qsTr("Write")
        rejectText: qsTr("Disconnect anyway")
        closePolicy: Popup.NoAutoClose

        onAccepted: writeChanges()

        function writeChanges() {
            var groups = dirtyConfigs()
            if (!groups.length) {
                VescIf.disconnectPort()
                return
            }
            if (!groups.every(writeReady)) return
            disconnectTimer.pending = groups
            disconnectTimer.restart()
            if (groups.includes(mMcConf)) mCommands.setMcconf(false)
            if (groups.includes(mAppConf)) mCommands.setAppConf()
            if (groups.includes(customConf)) mCommands.customConfigSet(0, customConf)
        }

        onRejected: VescIf.disconnectPort()

        Text {
            anchors.fill: parent
            text: qsTr("The connected device has unwritten configuration changes.")
            wrapMode: Text.WordWrap
            color: Utility.getAppHexColor("lightText")
        }
    }

    Timer {
        id: disconnectTimer
        interval: 5000
        property var pending: []
        onRunningChanged: if (running) showToast(qsTr("Writing changes..."), qsTr("Disconnecting when the VESC confirms."), "ok")

        function confirm(conf) {
            if (!running || !pending.includes(conf)) return
            pending = pending.filter(p => p !== conf)
            if (!pending.length) {
                Qt.callLater(function() {
                    if (disconnectTimer.running) {
                        disconnectTimer.stop()
                        requestDisconnect()
                    }
                })
            }
        }

        onTriggered: {
            var uuid = VescIf.getConnectedUuid()
            showToast(qsTr("Write not confirmed"), qsTr("You are still connected."), "warn", qsTr("Retry"), function() {
                if (connected && fwReadCorrectly && VescIf.getConnectedUuid() === uuid) disconnectSheet.writeChanges()
            })
        }
    }

    Popup {
        id: allPagesPopup
        parent: Overlay.overlay
        Overlay.modal: Rectangle {
            color: Utility.getAppHexColor("overlay")
        }
        // Landscape phones: a full height column under the button, in reach of the right thumb
        x: phoneLandscape ? parent.width - width - 10 - notchRight : 10 + notchLeft
        y: headerBar.height + 6
        width: phoneLandscape ? 380 : parent.width - 20 - notchLeft - notchRight
        height: phoneLandscape ? parent.height - y - 10 - notchBot : implicitHeight
        modal: true
        padding: 12

        background: Rectangle {
            radius: 20
            color: Utility.getAppHexColor("cardBackground")
        }

        contentItem: ColumnLayout {
            spacing: 10

            Text {
                text: qsTr("All pages")
                font.weight: Font.Medium
                color: Utility.getAppHexColor("lightText")
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: 3
                rowSpacing: 8
                columnSpacing: 8

                Repeater {
                    model: allPagesPopup.visible ? tabBar.count : 0

                    AbstractButton {
                        id: pageTile
                        readonly property Item tab: tabBar.itemAt(index)
                        readonly property bool current: index === tabBar.currentIndex

                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: phoneLandscape ? 56 : 76
                        Layout.fillHeight: phoneLandscape
                        onClicked: {
                            mainSwipeView.setCurrentIndex(index)
                            allPagesPopup.close()
                        }

                        background: Rectangle {
                            radius: 12
                            color: pageTile.current ? Qt.alpha(Material.accent, 0.12) : Utility.getAppHexColor("darkBackground")
                            border.width: pageTile.current ? 1.5 : 0
                            border.color: Material.accent

                            Rectangle {
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 8
                                width: 8
                                height: 8
                                radius: 4
                                visible: pageTile.tab.dot
                                color: Material.accent
                            }
                        }

                        contentItem: ColumnLayout {
                            spacing: 6

                            Image {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: 30
                                Layout.preferredHeight: 30
                                source: tabIcon(pageTile.tab.text)
                            }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: pageTile.tab.text
                                color: pageTile.current ? Utility.getAppHexColor("accentText") : Utility.getAppHexColor("lightText")
                            }
                        }
                    }
                }
            }
        }
    }

    ParamSearch {
        id: paramSearch

        sources: [
            {title: confMotorButton.text, params: mMcConf,
                pageIndex: confPageMotor.SwipeView.index, editor: confMotorLoader.item},
            {title: confAppButton.text, params: mAppConf,
                pageIndex: confPageApp.SwipeView.index, editor: confAppLoader.item},
            {title: confCustomButton.text, params: customConf,
                pageIndex: confCustomPage.SwipeView.index, editor: confCustomLoader.item, groups: ["General"]}
        ]

        onParamSelected: function(sourceIndex, group, subgroup, name) {
            var target = sources[sourceIndex]
            if (target.editor) {
                mainSwipeView.setCurrentIndex(target.pageIndex)
                target.editor.jumpTo(group, subgroup, name)
            }
        }

        onPageSelected: function(title) {
            if (title === "Controls") {
                controls.openDialog()
            } else if (title === "Multi Settings") {
                multiSettings.openDialog()
            } else if (title === "Settings") {
                settingsLoader.item.openDialog()
            } else {
                for (var i = 0;i < tabBar.count;i++) {
                    if (tabBar.itemAt(i).text === title) {
                        mainSwipeView.setCurrentIndex(i)
                    }
                }
            }
        }
    }

    SheetDialog {
        id: qmlLoadDialog
        acceptText: "Load"
        closePolicy: Popup.CloseOnEscape
        title: "Load Custom User Interface"

        ColumnLayout {
            anchors.fill: parent

            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Utility.getAppHexColor("lightText")
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text:
                    "The hardware you are connecting to contains code that will alter the " +
                    "user interface of VESC Tool. This code has not been verified by the " +
                    "authors of VESC Tool and could contain bugs and security problems. \n\n" +
                    "Do you want to load this custom user interface?"
            }

            CheckBox {
                Layout.fillWidth: true
                id: qmlDoNotAskAgainBox
                text: "Load without asking"
            }
        }

        onAccepted: {
            VescIf.setAskQmlLoad(!qmlDoNotAskAgainBox.checked)
            VescIf.storeSettings()
            updateHwAppUi()
        }

        onRejected: {
            VescIf.disconnectPort()
        }
    }
}
