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

Item {
    id: rootItem

    property BleUart mBle: VescIf.bleDevice()
    property Commands mCommands: VescIf.commands()
    property int notchTop: 0
    property int animationSpeed: 500
    Behavior on y {
        NumberAnimation {
            duration: animationSpeed;
            easing.type: Easing.InOutSine
        }
    }

    Rectangle {
        color: Utility.getAppHexColor("darkBackground")
        anchors.fill: parent
    }

    Component.onCompleted: {
        mBle.startScan()
        scanDotTimer.running = true
    }

    onYChanged: {
        if (y > 1) {
            enableDialog()
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: 10
        Rectangle{
            Layout.preferredHeight: notchTop
            Layout.fillWidth: true
            opacity: 0
        }

        Image {
            id: image
            Layout.preferredWidth: Math.min(column.width, column.height) * 0.8
            Layout.preferredHeight: (sourceSize.height * Layout.preferredWidth) / sourceSize.width
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            Layout.topMargin: Math.min(rootItem.width, rootItem.height) * 0.025
            Layout.bottomMargin: 0
            source: "qrc" + Utility.getThemePath() + "/logo.png"

            DragHandler {
                id: handler
                target:rootItem
                margin: 0
                xAxis.enabled: false
                yAxis.maximum: rootItem.height
                yAxis.minimum: 0

                onActiveChanged: {
                    if (handler.active) {
                        animationSpeed = 3
                    } else {
                        animationSpeed = 500
                        if (rootItem.y > rootItem.height / 4) {
                            rootItem.y = rootItem.height
                        } else {
                            rootItem.y = 0
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
                    rootItem.y = rootItem.height
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
                text: qsTr("Scan")
                enabled: false
                flat: true
                Layout.preferredWidth: 120

                onClicked: {
                    scanButton.enabled = false
                    scanDotTimer.running = true
                    bleModel.clear()
                    mBle.startScan()
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
            // transitions for insertion/deletation of elements
            add: Transition {
                NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 200 }
                NumberAnimation { property: "scale"; easing.type: Easing.OutExpo; from: 0; to: 1.0; duration: 300 }
            }

            addDisplaced: Transition {
                NumberAnimation { properties: "y"; duration: 300; easing.type: Easing.InOutBack }
            }

            remove: Transition {
                NumberAnimation { property: "scale"; from: 1.0; to: 0; duration: 100 }
                NumberAnimation { property: "opacity"; from: 1.0; to: 0; duration: 200 }
            }

            removeDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 300; easing.type: Easing.InExpo }
            }

            id: bleList
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true
            clip: true
            spacing: 5

            Component {
                id: bleDelegate

                Rectangle {
                    //Component.onCompleted: showAnim.start();
                    //transform: Rotation { id:rt; origin.x: 0; origin.y: height; axis { x: 0.3; y: 1; z: 0 } angle: 0}//     <--- I like this one more!
                    //SequentialAnimation {
                    //    id: showAnim
                    //    running: false
                    //    RotationAnimation { target: rt; from: 180; to: 0; duration: 800; easing.type: Easing.OutExpo; property: "angle" }
                    //}

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
                                    source: "qrc" + Utility.getThemePath() + (isSerial ? "icons/Connected-96.png" : "icons/bluetooth.png")
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
                            visible: !isSerial

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
                                    mBle.emitScanDone()
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.rightMargin: 10
                            spacing: -5

                            Button {
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                                Layout.preferredHeight: 55
                                text: isSerial ? "Autoconnect" : "Connect"

                                onClicked: {
                                    if (isSerial) {
                                        VescIf.autoconnect()
                                    } else {
                                        if (!VescIf.getBlePreferred(bleAddr)) {
                                            preferredDialog.bleAddr = bleAddr
                                            preferredDialog.open()
                                        } else {
                                            disableDialog()
                                            VescIf.connectBle(bleAddr)
                                        }
                                    }
                                }
                            }

                            Button {
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                                Layout.preferredHeight: 55
                                text: "Set Name"
                                visible: !isSerial

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
                running: !scanButton.enabled
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

    Connections {
        target: mBle
        onScanDone: {
            if (done) {
                scanDotTimer.running = false
                scanButton.enabled = true
                scanButton.text = qsTr("Scan")
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
                                            "isSerial": false})
                    } else {
                        bleModel.append({"name": setName,
                                            "setName": setNameShort,
                                            "preferred": preferred,
                                            "bleAddr": addr,
                                            "isSerial": false})
                    }

                }
            }
            addToList = true
            for(j=0; j < bleModel.count; j++) {
                if(bleModel.get(j).name === ("Serial Port")){
                    addToList  = false
                }
            }
            if (Utility.hasSerialport() && addToList) {
                bleModel.insert(0, {"name": "Serial Port",
                                    "setName": "",
                                    "preferred": true,
                                    "bleAddr": "",
                                    "isSerial": true})
            }
        }

        onBleError: {
            VescIf.emitMessageDialog("BLE Error", info, false, false)
            enableDialog()
        }
    }

    function disableDialog() {
        commDialog.open()
        column.enabled = false
    }

    function enableDialog() {
        commDialog.close()
        column.enabled = true
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

        width: parent.width - 20
        x: 10
        y: parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        ProgressBar {
            anchors.fill: parent
            indeterminate: visible
        }
    }

    Dialog {
        property var addr: ""

        id: bleNameDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        title: "Set BLE Device Name"

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 20
        height: 200
        closePolicy: Popup.CloseOnEscape
        x: 10
        y: Math.max(parent.height / 4 - height / 2, 20)
        parent: ApplicationWindow.overlay

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
            mBle.emitScanDone()
        }
    }

    Dialog {
        id: preferredDialog
        property var bleAddr: ""
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        focus: true
        rightMargin: 10
        leftMargin: 10
        closePolicy: Popup.CloseOnEscape
        title: "Preferred BLE Devices"
        y: 10 + parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay      
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        Text {
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "Do you want to make this a preferred device? That will make it show " +
                  "up a bit faster in BLE scans and it will be shown at the top of the device list."
        }

        onAccepted: {
            VescIf.storeBlePreferred(bleAddr, true)
            mBle.emitScanDone()
            disableDialog()
            VescIf.connectBle(bleAddr)
        }

        onRejected: {
            disableDialog()
            VescIf.connectBle(bleAddr)
        }
    }
}
