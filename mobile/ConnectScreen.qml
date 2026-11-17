/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material
import QtQuick.Layouts 1.3
import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.bleuart 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0
import Vedder.vesc.udpserversimple 1.0

Item {
    id: rootItem
    property int animationDuration: 500
    property BleUart mBle: VescIf.bleDevice()
    property Commands mCommands: VescIf.commands()
    property bool opened: true
    property bool pingTcpHub: false
    property bool scanning: false
    property bool fullLogo: true
    property var dialogParent: rootItem.parent
    property int filterType: -1
    property var typeCounts: [0, 0, 0, 0]
    property var foundAddrs: []
    readonly property int deviceCount: typeCounts.reduce((a, b) => a + b)

    onOpenedChanged: {
        if(opened){
            animationDuration = 500
            y = 0
        } else {
            y = Qt.binding(function() {return parent.height})
        }
    }

    Behavior on y {
        NumberAnimation {
            duration: animationDuration
            easing.type: Easing.InOutSine
        }
    }

    Rectangle {
        color: Utility.getAppHexColor("darkBackground")
        anchors.fill: parent
    }

    // Prevents events from passing to components behind
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onWheel: {wheel.accepted=true}
        hoverEnabled: true
    }

    function startBleScan() {
        if (Utility.hasLocationPermission()) {
            scanning = true
            bleModel.clear()
            vescsUdp = []
            mBle.startScan()
        } else {
            bleScanStart.open()
        }
    }

    onYChanged: {
        if (y > 1) {
            enableDialog()
        }
        if(!opened & y == height){
            animationDuration = 0
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: 10
        anchors.leftMargin: 14 + notchLeft
        anchors.rightMargin: 14 + notchRight

        Rectangle {
            Layout.preferredHeight: notchTop
            Layout.fillWidth: true
            opacity: 0
        }

        Image {
            id: image
            Layout.preferredWidth: Math.min(column.width, column.height*0.8) * 0.5
            Layout.preferredHeight: (sourceSize.height * Layout.preferredWidth) / sourceSize.width
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            Layout.topMargin: Math.min(rootItem.width, rootItem.height) * 0.025
            Layout.bottomMargin: 0
            source: fullLogo
                    ? "qrc" + Utility.getThemePath() + "/logo.png"
                    : "qrc" + Utility.getThemePath() + "/symbol_v_wide.png"
            DragHandler {
                id: handler
                target: rootItem
                margin: 0
                xAxis.enabled: false
                yAxis.maximum: rootItem.height
                yAxis.minimum: 0

                onActiveChanged: {
                    if (handler.active) {
                        animationDuration = 3
                    } else {
                        animationDuration = 500
                        if(opened) {
                            if (rootItem.y > (rootItem.height / 4)) {
                                rootItem.opened = false
                            } else {
                                rootItem.y = 0
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: qsTr("Hide")
                flat: true
                onClicked: {
                    rootItem.opened = false
                }
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                icon.source: "qrc" + Utility.getThemePath() + "icons/Refresh-96.png"
                enabled: !scanning
                Accessible.name: qsTr("Scan BLE")
                onClicked: startBleScan()
            }

            ToolButton {
                icon.source: "qrc" + Utility.getThemePath() + "icons_textedit/Menu 2-96.png"
                Accessible.name: qsTr("Scan options")
                onClicked: {
                    scanMenu.open()
                }

                PopupMenu {
                    id: scanMenu

                    MenuItem {
                        text: "Scan BLE"
                        enabled: !scanning
                        onTriggered: {
                            startBleScan()
                        }
                    }

                    MenuItem {
                        text: "Ping TCP hub"
                        onTriggered: {
                            bleModel.clear()
                            pingTcpHub = true
                            mBle.emitScanDone()
                        }
                    }

                    MenuItem {
                        text: "Connect TCP Hub..."
                        onTriggered: {
                            tcpHubConnDialog.open()
                        }

                        SheetDialog {
                            id: tcpHubConnDialog
                            acceptText: qsTr("Connect")

                            onAccepted: {
                                hubBox.connectToHub()
                            }

                            title: "Connect TCP Hub"

                            TcpHubBox {
                                id: hubBox
                                anchors.fill: parent
                                showControls: false
                            }
                        }
                    }

                    MenuItem {
                        text: "Clear TCP hub devs"
                        onTriggered: {
                            tcpHubClearDialog.open()
                        }

                        SheetDialog {
                            id: tcpHubClearDialog
                            title: "Clear TCP Hub Devices"
                            acceptText: qsTr("Clear")
                            onAccepted: VescIf.clearTcpHubDevs()

                            Text {
                                color: Utility.getAppHexColor("lightText")
                                verticalAlignment: Text.AlignVCenter
                                anchors.fill: parent
                                wrapMode: Text.WordWrap
                                text:
                                    "This is going to clear all previous TCP hub connections. Continue?"
                            }
                        }
                    }

                    MenuItem {
                        text: "Connect Virtual ESC"
                        onTriggered: {
                            VescIf.connectVirtual()
                        }
                    }
                }
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            Material.accent: Utility.getAppHexColor("lightAccent")
            indeterminate: scanning
            opacity: scanning ? 1 : 0
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.rightMargin: 4

            Text {
                Layout.fillWidth: true
                color: Utility.getAppHexColor("normalText")
                text: scanning ? qsTr("Scanning Bluetooth, USB and Wi-Fi…") : qsTr("Devices")
                font.pointSize: 10
            }

            Text {
                color: Utility.getAppHexColor("normalText")
                text: qsTr("%1 found").arg(deviceCount)
                font.pointSize: 10
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.topMargin: -6
            Layout.bottomMargin: -4
            orientation: ListView.Horizontal
            spacing: 6
            clip: true
            model: [[qsTr("All"), -1, "", deviceCount],
                    ["BLE", 0, "icons/bluetooth.png", typeCounts[0]],
                    ["USB", 1, "icons/USB-96.png", typeCounts[1]],
                    ["Wi-Fi", 2, "icons/LAN-96.png", typeCounts[2]],
                    ["Hub", 3, "icons/Globe-96.png", typeCounts[3]]]

            delegate: Chip {
                text: modelData[0] + " " + modelData[3]
                iconSource: modelData[2] !== "" ? "qrc" + Utility.getThemePath() + modelData[2] : ""
                checked: filterType === modelData[1]
                onClicked: {
                    filterType = modelData[1]
                    mBle.emitScanDone()
                }
            }
        }

        ListModel {
            id: bleModel
        }

        ListView {
            id: bleList
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true
            clip: true
            spacing: 5
            z:2

            model: bleModel
            section.property: "group"
            section.delegate: SectionHeader {
                width: bleList.width
                height: implicitHeight + 8
                text: section === "1" ? qsTr("TCP Hub") : qsTr("Nearby")
            }

            header: ColumnLayout {
                width: bleList.width
                // Only when the last device is in range
                visible: lastDeviceAddr !== "" && foundAddrs.indexOf(lastDeviceAddr) >= 0
                height: visible ? implicitHeight + 4 : 0

                SectionHeader {
                    text: qsTr("Last connected")
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 70
                    radius: 12
                    color: Utility.getAppHexColor("cardBackground")
                    border.color: Utility.getAppHexColor("raisedBackground")

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 12

                        Rectangle {
                            width: 46
                            height: 46
                            radius: 23
                            color: Utility.isDarkMode() ? Utility.getAppHexColor("vescGreenMedium") :
                                                          Qt.alpha(Utility.getAppHexColor("positive"), 0.15)

                            Image {
                                anchors.centerIn: parent
                                width: 26
                                height: 26
                                source: "qrc" + Utility.getThemePath() + lastDeviceIcon
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                Layout.fillWidth: true
                                text: lastDeviceName
                                elide: Text.ElideRight
                                font.pointSize: 15
                                font.weight: Font.Medium
                                color: Utility.getAppHexColor("titleText")
                            }

                            Text {
                                text: timeAgo(lastDeviceTime)
                                font.pointSize: 9
                                color: Utility.getAppHexColor("normalText")
                            }
                        }

                        Button {
                            text: qsTr("Reconnect")
                            highlighted: true
                            onClicked: VescIf.reconnectLastPort()
                        }
                    }
                }
            }

            delegate: Rectangle {
                readonly property var nameLines: name.split("\n")
                readonly property int rssi: connectionType === 0 ? mBle.getRssi(bleAddr) : 0
                readonly property color transportColor: Utility.getAppHexColor(
                                                            ["positive", "warning", "lightAccent", "magenta"][connectionType])

                width: bleList.width
                height: 62
                radius: 12
                color: Utility.getAppHexColor("cardBackground")
                border.color: Utility.getAppHexColor("raisedBackground")

                TapHandler {
                    onLongPressed: {
                        if (connectionType === 0) {
                            bleNameDialog.addr = bleAddr
                            stringInput.text = setName
                            bleNameDialog.open()
                        } else if (connectionType === 3) {
                            hubPassDialog.uuid = hubUuid
                            hubPassDialog.open()
                        }
                    }
                }

                Timer {
                    id: workaroundTimerConnect
                    property string bleAddr: ""
                    interval: 0
                    repeat: false
                    running: false
                    onTriggered: {
                        VescIf.connectBle(workaroundTimerConnect.bleAddr)
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 10

                    Rectangle {
                        width: 38
                        height: 38
                        radius: 19
                        color: Qt.alpha(transportColor, 0.12)

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            source: "qrc" + Utility.getThemePath() + ({
                                0: "icons/bluetooth.png",
                                1: "icons/USB-96.png",
                                2: "icons/LAN-96.png",
                                3: "icons/Globe-96.png"
                            }[connectionType])
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Text {
                            Layout.fillWidth: true
                            text: nameLines[0]
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                            color: Utility.getAppHexColor("titleText")
                        }

                        Text {
                            Layout.fillWidth: true
                            text: nameLines.length > 1 ? nameLines[1].replace("[", "").replace("]", "") : ""
                            elide: Text.ElideRight
                            font.family: "DejaVu Sans Mono"
                            font.pointSize: 9
                            color: Utility.getAppHexColor("disabledText")
                        }
                    }

                    // Signal strength from the BLE scan
                    Row {
                        spacing: 2
                        visible: rssi < 0

                        Repeater {
                            model: 4

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: 3
                                height: 5 + index * 3
                                radius: 1
                                color: rssi > -100 + (index + 1) * 12 ? Utility.getAppHexColor("positiveText") :
                                                                       Utility.getAppHexColor("lightestBackground")
                            }
                        }
                    }

                    ToolButton {
                        icon.source: "qrc" + Utility.getThemePath() + (preferred ? "icons/Star-96.png" : "icons/Star Outline-96.png")
                        icon.color: "transparent"
                        Accessible.name: qsTr("Preferred")
                        onClicked: {
                            VescIf.storeBlePreferred(connectionType === 3 ? hubUuid : bleAddr, !preferred)
                            VescIf.storeSettings()
                            if (connectionType === 3) {
                                // Rebuilding the list would drop the TCP hub devices
                                bleModel.setProperty(index, "preferred", !preferred)
                            } else {
                                bleModel.clear()
                                mBle.emitScanDone()
                            }
                        }
                    }

                    Button {
                        text: "Connect"

                        onClicked: {
                            if (connectionType === 1) {
                                if (bleAddr === "") {
                                    VescIf.autoconnect()
                                } else {
                                    VescIf.connectSerial(bleAddr, 115200)
                                }
                            } else if (connectionType === 2) {
                                VescIf.connectTcp(bleAddr, tcpPort)
                            } else if (connectionType === 3) {
                                VescIf.connectTcpHubUuid(hubUuid)
                            } else {
                                disableDialog()
                                workaroundTimerConnect.bleAddr = bleAddr
                                workaroundTimerConnect.start()
                            }
                        }
                    }
                }
            }

            footer: ColumnLayout {
                width: bleList.width

                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                    visible: bleList.count > 0
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    font.pointSize: 9
                    color: Utility.getAppHexColor("normalText")
                    text: qsTr("Long-press a device to Set Name (Bluetooth) or Update Password (TCP hub)")
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    implicitHeight: helpRow.implicitHeight + 20
                    radius: 12
                    color: "transparent"
                    border.color: Qt.alpha(Utility.getAppHexColor("lightAccent"), 0.33)

                    RowLayout {
                        id: helpRow
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Image {
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
                        }

                        Text {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            font.pointSize: 10
                            color: Utility.getAppHexColor("normalText")
                            text: qsTr("Can't find your VESC? Check that Bluetooth and location access are enabled and that the VESC device is powered on.")
                        }

                        Button {
                            visible: !Utility.hasLocationPermission()
                            flat: true
                            text: qsTr("Allow")
                            onClicked: startBleScan()
                        }
                    }
                }
            }
        }
    }

    property var vescsUdp: []

    UdpServerSimple {
        id: udpDiscovery
        Component.onCompleted: {
            startServerBroadcast(65109)
        }

        onDataRx: {
            var tokens = Utility.arr2str(data).split("::")
            if (tokens.length === 3) {
                var found = false
                
                for (var i = 0; i < vescsUdp.length;i++) {
                    if (vescsUdp[i].ip === tokens[1]) {
                        vescsUdp[i].updateTime = new Date().getTime()
                        found = true
                        break
                    }
                }

                if (!found) {
                    vescsUdp[vescsUdp.length] = {
                        "name" : tokens[0],
                        "ip" : tokens[1],
                        "port" : parseInt(tokens[2]),
                        "updateTime" : new Date().getTime()
                    }
                    mBle.emitScanDone()
                }
            }
        }
    }

    Connections {
        target: Qt.application

        function onStateChanged() {
            if (Qt.platform.os == "ios") {
                if (Qt.application.state == Qt.ApplicationActive) {
                    udpDiscovery.startServerBroadcast(65109)
                } else {
                    udpDiscovery.stopServer()
                }
            }
        }
    }

    Timer {
        running: true
        repeat: true
        interval: 500

        onTriggered: {
            var removed = false
            for (var i = 0; i < vescsUdp.length;i++) {
                var age = new Date().getTime() - vescsUdp[i].updateTime
                if (age > 5000) {
                    vescsUdp.splice(i, 1)
                    removed = true
                    i--;
                    if (i < 0) {
                        break
                    }
                }
            }

            if (removed) {
                bleModel.clear()
                mBle.emitScanDone()
            }

            // Re-scan serial regularly
            if (Utility.hasSerialport()) {
                mBle.emitScanDone()
            }
        }
    }

    Connections {
        target: mBle
        function formatBleDevice(name, addr){
            var shortAddr = addr
            if(Qt.platform.os == "ios" || Qt.platform.os == "mac") {
                var ids = addr.split('-')
                shortAddr = 
                    ids[0].slice(1,3) + '-' +
                    ids[1].slice(0,2) + '-' +
                    ids[2].slice(0,2) + '-' +
                    ids[3].slice(0,2) + '-' + 
                    ids[4].slice(0,2)
            }
            
            var bleName = VescIf.getBleName(addr)
            var displayName
            if (bleName.length > 0) {
                displayName = bleName
            } else {
                displayName = name
            }
            displayName += "\n[" + shortAddr + "]"
            
            return {
                "name": displayName,
                "setName": bleName,
                "preferred": VescIf.getBlePreferred(addr),
                "bleAddr": addr,
                "tcpPort": 0,
                "hubUuid": "",
                "connectionType": 0,
                "group": 0
            }        
        }
        
        function formatUDPDevice(device){
           return {
                "name": device.name + "\n" + device.ip + ":" + device.port,
                "setName": "",
                "preferred": VescIf.getBlePreferred(device.ip),
                "bleAddr": device.ip,
                "tcpPort": device.port,
                "hubUuid": "",
                "connectionType": 2,
                "group": 0
            }
        }
        
        function formatSerialPort(port){
            return {
                "name": "Serial " + 
                    (port.isVesc ? "STM32" : port.isEsp ? "ESP32" : "") + 
                    "\n" + port.systemPath,
                "setName": "",
                "preferred": VescIf.getBlePreferred(port.systemPath),
                "bleAddr": port.systemPath,
                "tcpPort": 0,
                "hubUuid": "",
                "connectionType": 1,
                "group": 0
            }
        }
        
        function formatTcpHubDevice(tcpHubDevice){
           return {                         
               "name": tcpHubDevice.id + "\n" + tcpHubDevice.server,
               "setName": "",
               "preferred": VescIf.getBlePreferred(tcpHubDevice.uuid()),
               "bleAddr": "",
               "tcpPort": 0,
               "hubUuid": tcpHubDevice.uuid(),
               "connectionType": 3,
               "group": 1
           }
        }
        function isModelUpdated(array, model) {
            if (array.length !== model.count) return true
            for (var i = 0; i < array.length; ++i) {
                if (JSON.stringify(model.get(i)) !== JSON.stringify(array[i])) {
                    return true
                }
            }
            return false
        }
        function onScanDone(bleDevices, done) {
            if (done) {
                scanning = false
            }
            var devices = [];
            
            for (var addr in bleDevices){
                devices.push(formatBleDevice(bleDevices[addr], addr))
            }
            
            for(var udpDevice of vescsUdp){
                devices.push(formatUDPDevice(udpDevice))
            }
            
            if (Utility.hasSerialport()) {
                for (var serialPort of VescIf.listSerialPorts()){
                    if (serialPort.isVesc || serialPort.isEsp){
                        devices.push(formatSerialPort(serialPort))
                    }
                }
            }
           
            if (pingTcpHub){
                pingTcpHub = false
                disableDialog()
                var hubDevs = VescIf.getTcpHubDevs()
                for (tcpHubDevice of hubDevs) {
                    if (tcpHubDevice.ping()) {
                        devices.push(formatTcpHubDevice(tcpHubDevice))
                    }
                }
                enableDialog()
            }

            devices.sort((a, b) => {
                             if (a.group !== b.group) {
                                 return a.group - b.group
                             }

                             if (a.preferred === b.preferred) {
                                 if (a.connectionType === b.connectionType) {
                                     return a.name.localeCompare(b.name)
                                 }

                                 // Bluetooth devices after the others
                                 return (a.connectionType === 0) - (b.connectionType === 0) || a.connectionType - b.connectionType
                             } else {
                                return a.preferred ? -1 : 1
                             }
                         })
            
            typeCounts = [0, 1, 2, 3].map(t => devices.filter(d => d.connectionType === t).length)
            foundAddrs = devices.map(d => d.bleAddr)
            devices = devices.filter(d => filterType < 0 || d.connectionType === filterType)

           if (isModelUpdated(devices, bleModel)){
               bleModel.clear()
               for (var device of devices){
                   bleModel.append(device)
               }
           }
        }

        function onBleError(info) {
            VescIf.emitMessageDialog("BLE Error", info, false, false)
            enableDialog()
        }
    }

    function disableDialog() {
        commDialog.open()
        column.enabled = false
        disableTimeoutTimer.stop()
        disableTimeoutTimer.start()
    }

    function enableDialog() {
        commDialog.close()
        column.enabled = true
        disableTimeoutTimer.stop()
    }

    Timer {
        id: disableTimeoutTimer
        running: false
        repeat: false
        interval: 15000

        onTriggered: {
            enableDialog()
            VescIf.emitMessageDialog("Connect",
                                     "Connection timed out",
                                     false, false)
        }
    }

    BusyDialog {
        id: commDialog
        title: "Connecting..."
    }

    SheetDialog {
        property string addr: ""

        id: bleNameDialog
        acceptText: qsTr("Save")
        title: "Set BLE Device Name"

        InputField {
            id: stringInput
            width: parent.width
            placeholderText: qsTr("Name")
            onAccepted: bleNameDialog.accept()
        }

        // Qt moves the window up by the height of the keyboard on iOS when it covers the field
        onOpened: stringInput.forceActiveFocus()

        onAccepted: {
            VescIf.storeBleName(addr, stringInput.text)
            VescIf.storeSettings()
            bleModel.clear()
            mBle.emitScanDone()
        }
    }

    SheetDialog {
        property string uuid: ""

        id: hubPassDialog
        acceptText: qsTr("Save")
        title: "Set TCP Hub Password"

        InputField {
            id: hubPassInput
            width: parent.width
            placeholderText: qsTr("Password")
            onAccepted: hubPassDialog.accept()
        }

        onOpened: hubPassInput.forceActiveFocus()

        onAccepted: {
            VescIf.updateTcpHubPassword(uuid, hubPassInput.text)
        }
    }

    SheetDialog {
        id: bleEn
        acceptText: qsTr("OK")
        rejectText: ""

        title: "BLE scan"

        Text {
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "BLE scan does not seem to be possible. Make sure that the " +
                  "location service is enabled on your device."
        }
    }

    SheetDialog {
        id: bleScanStart
        acceptText: qsTr("Continue")
        rejectText: qsTr("Not now")

        onAccepted: {
            scanning = true
            bleModel.clear()
            vescsUdp = []
            mBle.startScan()

            if (!Utility.isBleScanEnabled()) {
                bleEn.open()
            }
        }

        onRejected: {
            VescIf.emitMessageDialog(
                        "Location Permission",
                        "VESC Tool cannot scan for bluetoot devices or log data with location information without the " +
                        "the location permission. Please accept the request in order to use these features.",
                        false, false)
        }

        title: "BLE scan"

        Text {
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text:
                "VESC Tool needs to access the location of your device to scan for " +
                "Bluetooth devices as well as for recording your location when doing " +
                "realtime data logging.\n\n" +

                "In order to keep logging when VESC Tool is in the background and/or when the " +
                "screen is off, the permission to log data in the background is also required. " +
                "Otherwise the logs will only get location information together with the motor " +
                "data when the screen is on and VESC Tool is in the foreground."
        }
    }
}
