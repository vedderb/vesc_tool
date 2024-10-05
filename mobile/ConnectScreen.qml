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
            scanDotTimer.running = true
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
        anchors.leftMargin: notchLeft
        anchors.rightMargin: notchRight

        Rectangle {
            Layout.preferredHeight: notchTop
            Layout.fillWidth: true
            opacity: 0
        }

        Image {
            id: image
            Layout.preferredWidth: Math.min(column.width, column.height*0.8) * 0.8
            Layout.preferredHeight: (sourceSize.height * Layout.preferredWidth) / sourceSize.width
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            Layout.topMargin: Math.min(rootItem.width, rootItem.height) * 0.025
            Layout.bottomMargin: 0
            source: "qrc" + Utility.getThemePath() + "/logo.png"
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
                Layout.preferredWidth: 120
                flat: true
                onClicked: {
                    rootItem.opened = false
                }
            }

            Text {
                id: text
                Layout.fillWidth: true
                color: Utility.getAppHexColor("lightText")
                text: qsTr("Devices Found")
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Button {
                id: scanButton
                text: qsTr("Scan...")
                enabled: true
                flat: true
                Layout.preferredWidth: 120

                onClicked: {
                    scanMenu.open()
                }

                Menu {
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

                        Dialog {
                            id: tcpHubConnDialog
                            standardButtons: Dialog.Ok | Dialog.Cancel
                            modal: true
                            focus: true
                            rightMargin: 10
                            leftMargin: 10
                            closePolicy: Popup.CloseOnEscape
                            y: 10 + parent.height / 2 - height / 2
                            x: parent.width/2 - width/2
                            width: parent.width - 20 - notchLeft - notchRight
                            parent: rootItem.parent
                            Overlay.modal: Rectangle {
                                color: "#AA000000"
                            }

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

                        Dialog {
                            id: tcpHubClearDialog
                            standardButtons: Dialog.Ok | Dialog.Cancel
                            modal: true
                            focus: true
                            rightMargin: 10
                            leftMargin: 10
                            closePolicy: Popup.CloseOnEscape
                            y: 10 + parent.height / 2 - height / 2
                            x: parent.width/2 - width/2
                            width: parent.width - 20 - notchLeft - notchRight
                            parent: rootItem.parent
                            Overlay.modal: Rectangle {
                                color: "#AA000000"
                            }

                            onAccepted: {
                                VescIf.clearTcpHubDevs()
                            }

                            title: "Clear TCP Hub Devices"

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
                }
            }

            Timer {
                id: scanDotTimer
                interval: 500
                running: false
                repeat: true

                property int dots: 0
                onTriggered: {
                    var text = "Scanning"
                    for (var i = 0;i < dots;i++) {
                        text = "-" + text + "-"
                    }

                    dots++;
                    if (dots > 3) {
                        dots = 0;
                    }

                    scanButton.text = text
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

            Component {
                id: bleDelegate

                Rectangle {
                    width: bleList.width
                    height: 120
                    color: Utility.getAppHexColor("normalBackground")
                    radius: 10

                    RowLayout {
                        anchors.fill: parent
                        spacing: 10

                        Rectangle {
                            Layout.leftMargin: 10
                            Layout.fillWidth: true
                            opacity: 1.0
                            color: preferred ? ("#55" + Utility.getAppHexColor("lightAccent").slice(1)) : (Utility.getAppHexColor("lightestBackground") )
                            height: column.height + 10
                            radius: height / 2

                            ColumnLayout {
                                id: column
                                anchors.centerIn: parent

                                Image {
                                    id: image
                                    fillMode: Image.PreserveAspectFit
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                    source: "qrc" + Utility.getThemePath() + ((isSerial === 0) ? "icons/bluetooth.png" : "icons/Connected-96.png")
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                    text: name
                                    horizontalAlignment: Text.AlignHCenter
                                    color: Utility.getAppHexColor("lightText")
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        ColumnLayout {
                            visible: isSerial === 0

                            Text {
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                                Layout.rightMargin: 5
                                color: Utility.getAppHexColor("lightText")
                                text: "Preferred"
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Switch {
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                                enabled: true
                                checked: preferred
                                onToggled: {
                                    VescIf.storeBlePreferred(bleAddr, checked)
                                    bleModel.clear()
                                    mBle.emitScanDone()
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.rightMargin: 10
                            spacing: -5

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

                            Button {
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                                Layout.preferredHeight: 55
                                Layout.preferredWidth: {
                                    if (isSerial === 0) {
                                        nameButton.width
                                    } else if (isSerial === 3) {
                                        passButton.width
                                    } else {
                                        nameButton.width
                                    }
                                }

                                text: "Connect"

                                onClicked: {
                                    if (isSerial === 1) {
                                        if (bleAddr === "") {
                                            VescIf.autoconnect()
                                        } else {
                                            VescIf.connectSerial(bleAddr, 115200)
                                        }
                                    } else if (isSerial === 2) {
                                        VescIf.connectTcp(bleAddr, tcpPort)
                                    } else if (isSerial === 3) {
                                        VescIf.connectTcpHubUuid(hubUuid)
                                    } else {
                                        disableDialog()
                                        workaroundTimerConnect.bleAddr = bleAddr
                                        workaroundTimerConnect.start()
                                    }
                                }
                            }

                            Button {
                                id: passButton
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                                Layout.preferredHeight: 55
                                text: "Update Password"
                                visible: isSerial === 3

                                onClicked: {
                                    hubPassDialog.uuid = hubUuid
                                    hubPassDialog.open()
                                }
                            }

                            Button {
                                id: nameButton
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                                Layout.preferredHeight: 55
                                text: "Set Name"
                                visible: isSerial === 0

                                onClicked: {
                                    bleNameDialog.addr = bleAddr
                                    stringInput.text = setName
                                    bleNameDialog.open()
                                }
                            }
                        }
                    }
                }
            }

            model: bleModel
            delegate: bleDelegate

            footer: BusyIndicator {
                Layout.alignment: Qt.AlignCenter
                Layout.fillHeight: true
                running: scanning
                width: parent.width
                height: BusyIndicator.implicitHeight*1.5

                Behavior on y {
                    NumberAnimation {
                        duration: 300;
                        easing.type: Easing.InOutSine
                    }
                }

            }
        }
    }

    property var vescsUdp: []

    UdpServerSimple {
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

    Timer {
        running: true
        repeat: true
        interval: 500

        onTriggered: {
            var removed = false
            for (var i = 0; i < vescsUdp.length;i++) {
                var age = new Date().getTime() - vescsUdp[i].updateTime
                if (age > 4000) {
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
        function onScanDone(devs, done) {
            if (done) {
                scanDotTimer.running = false
                scanning = false
                scanButton.text = qsTr("Scan...")
            }

            for (var addr in devs) {
                var name = devs[addr]
                if(Qt.platform.os == "ios" || Qt.platform.os == "mac") {
                    var ids = addr.split('-')
                    var shortAddr = ids[0].slice(1,3) + '-' + ids[1].slice(0,2) + '-' +
                            ids[2].slice(0,2) + '-' + ids[3].slice(0,2) + '-' + ids[4].slice(0,2)
                } else {
                    shortAddr = addr;
                }
                var setName = VescIf.getBleName(addr)
                var setNameShort = setName
                var preferred = VescIf.getBlePreferred(addr)
                if (setName.length > 0) {
                    setName += "\n[" + shortAddr + "]"
                } else {
                    setName = name + "\n[" + shortAddr + "]"
                }
                var addToList = true
                var j = 0
                var k = 0
                for(j=0; j < bleModel.count; j++) {
                    if(bleModel.get(j).bleAddr === addr){
                        addToList  = false
                    }
                }

                if (addToList) {
                    if (preferred) {
                        bleModel.insert(0, {"name": setName,
                                            "setName": setNameShort,
                                            "preferred": preferred,
                                            "bleAddr": addr,
                                            "tcpPort": 0,
                                            "hubUuid": "",
                                            "isSerial": 0})
                    } else {
                        bleModel.append({"name": setName,
                                            "setName": setNameShort,
                                            "preferred": preferred,
                                            "bleAddr": addr,
                                            "tcpPort": 0,
                                            "hubUuid": "",
                                            "isSerial": 0})
                    }

                }
            }

            for (k = 0; k < vescsUdp.length;k++) {
                addToList  = true
                for (j = 0; j < bleModel.count; j++) {
                    if (bleModel.get(j).bleAddr === (vescsUdp[k].ip)) {
                        addToList  = false
                        break
                    }
                }

                if (addToList) {
                    bleModel.insert(0, {"name": vescsUdp[k].name + " (TCP)\n" +
                                        vescsUdp[k].ip + ":" + vescsUdp[k].port,
                                        "setName": "",
                                        "preferred": true,
                                        "bleAddr": vescsUdp[k].ip,
                                        "tcpPort": vescsUdp[k].port,
                                        "hubUuid": "",
                                        "isSerial": 2})
                }
            }

            if (Utility.hasSerialport()) {
                var ports = VescIf.listSerialPorts()
                for (j = 0; j < ports.length; j++) {
                    if (ports[j].isEsp) {
                        var nameNow = "Serial ESP32\n" + ports[j].systemPath
                        addToList = true
                        for (k = 0; k < bleModel.count; k++) {
                            if(bleModel.get(k).name === nameNow){
                                addToList  = false
                            }
                        }

                        if (addToList) {
                            bleModel.insert(0, {"name": nameNow,
                                                "setName": "",
                                                "preferred": true,
                                                "bleAddr": ports[j].systemPath,
                                                "tcpPort": 0,
                                                "hubUuid": "",
                                                "isSerial": 1})
                        }
                    }

                    if (ports[j].isVesc) {
                        nameNow = "Serial STM32\n" + ports[j].systemPath
                        addToList = true
                        for (k = 0; k < bleModel.count; k++) {
                            if (bleModel.get(k).name === nameNow){
                                addToList  = false
                            }
                        }

                        if (addToList) {
                            bleModel.insert(0, {"name": nameNow,
                                                "setName": "",
                                                "preferred": true,
                                                "bleAddr": ports[j].systemPath,
                                                "tcpPort": 0,
                                                "hubUuid": "",
                                                "isSerial": 1})
                        }
                    }
                }

                // Remove ports that no longer are there
                var removed = true
                while (removed) {
                    removed = false

                    for (k = 0; k < bleModel.count; k++) {
                        var found = false
                        for (j = 0; j < ports.length; j++) {
                            if (ports[j].systemPath === bleModel.get(k).bleAddr) {
                                found = true
                                break
                            }
                        }

                        if (bleModel.get(k).isSerial === 1 && !found) {
                            bleModel.remove(k)
                            removed = true
                            break
                        }
                    }
                }
            }

            if (pingTcpHub) {
                pingTcpHub = false
                disableDialog()
                var hubDevs = VescIf.getTcpHubDevs()
                for (j = 0; j < hubDevs.length; j++) {
                    if (hubDevs[j].ping()) {
                        bleModel.insert(0, {"name": hubDevs[j].id + " (TCP Hub)\n" + hubDevs[j].server,
                                            "setName": "",
                                            "preferred": true,
                                            "bleAddr": "",
                                            "tcpPort": 0,
                                            "hubUuid": hubDevs[j].uuid(),
                                            "isSerial": 3})
                    }
                }

                enableDialog()
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

    Dialog {
        id: commDialog
        title: "Connecting..."
        closePolicy: Popup.NoAutoClose
        modal: true
        focus: true

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 20 - notchLeft - notchRight
        x: parent.width/2 - width/2
        y: parent.height / 2 - height / 2
        parent: rootItem.parent
        ProgressBar {
            anchors.fill: parent
            indeterminate: visible
        }
    }

    Dialog {
        property string addr: ""

        id: bleNameDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        title: "Set BLE Device Name"

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 20 - notchLeft - notchRight
        height: 200
        closePolicy: Popup.CloseOnEscape
        x: parent.width/2 - width/2
        y: Math.max(parent.height / 4 - height / 2, 20)
        parent: rootItem.parent

        Rectangle {
            anchors.fill: parent
            height: stringInput.implicitHeight + 14
            border.width: 2
            border.color: Utility.getAppHexColor("lightestBackground")
            color: Utility.getAppHexColor("normalBackground")
            radius: 3
            TextInput {
                id: stringInput
                color: Utility.getAppHexColor("lightText")
                anchors.fill: parent
                anchors.margins: 7
                font.pointSize: 12
                focus: true
            }
        }

        onAccepted: {
            VescIf.storeBleName(addr, stringInput.text)
            VescIf.storeSettings()
            bleModel.clear()
            mBle.emitScanDone()
        }
    }

    Dialog {
        property string uuid: ""

        id: hubPassDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        title: "Set TCP Hub Password"

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 20 - notchLeft - notchRight
        height: 200
        closePolicy: Popup.CloseOnEscape
        x: parent.width/2 - width/2
        y: Math.max(parent.height / 4 - height / 2, 20)
        parent: rootItem.parent

        Rectangle {
            anchors.fill: parent
            height: hubPassInput.implicitHeight + 14
            border.width: 2
            border.color: Utility.getAppHexColor("lightestBackground")
            color: Utility.getAppHexColor("normalBackground")
            radius: 3
            TextInput {
                id: hubPassInput
                color: Utility.getAppHexColor("lightText")
                anchors.fill: parent
                anchors.margins: 7
                font.pointSize: 12
                focus: true
            }
        }

        onAccepted: {
            VescIf.updateTcpHubPassword(uuid, hubPassInput.text)
        }
    }

    Dialog {
        id: bleEn
        standardButtons: Dialog.Ok
        modal: true
        focus: true
        rightMargin: 10
        leftMargin: 10
        closePolicy: Popup.CloseOnEscape
        y: 10 + parent.height / 2 - height / 2
        x: parent.width/2 - width/2
        width: parent.width - 20 - notchLeft - notchRight
        parent: rootItem.parent
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

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

    Dialog {
        id: bleScanStart
        standardButtons: Dialog.Ok | Dialog.No
        modal: true
        focus: true
        rightMargin: 10
        leftMargin: 10
        closePolicy: Popup.CloseOnEscape
        y: 10 + parent.height / 2 - height / 2
        x: parent.width/2 - width/2
        width: parent.width - 20 - notchLeft - notchRight
        parent: rootItem.parent
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        onAccepted: {
            scanning = true
            scanDotTimer.running = true
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
