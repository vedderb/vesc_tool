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

import QtQuick 2.10
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.bleuart 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0

Item {
    id: rootItem

    property BleUart mBle: VescIf.bleDevice()
    property Commands mCommands: VescIf.commands()
    property int animationSpeed: 500

    Component.onCompleted: {
        scanButton.enabled = VescIf.isPortConnected()
    }

    function scanIfEmpty() {
        if (canList.count == 0 &&
                VescIf.isPortConnected() &&
                scanButton.enabled) {
            scanButton.clicked()
        }
    }

    function selectDeviceInList() {
        if (mCommands.getSendCan()) {
            for (var i = 0; i < canModel.count;i++) {
                var id = parseInt(canModel.get(i).ID)
                if (id === mCommands.getCanSendId() && canList.currentIndex != i) {
                    canList.currentIndex = i
                    break
                }
            }
        } else {
            if (canList.currentIndex != 0) {
                canList.currentIndex = 0
            }
        }
    }

    Timer {
        repeat: true
        interval: 1000
        running: true

        onTriggered: {
            if (!VescIf.isPortConnected()) {
                mCommands.setSendCan(false, -1)
                canList.currentIndex = 0;
                canModel.clear()
                scanButton.enabled = false
            } else {
                selectDeviceInList()
            }

            if (VescIf.scanCanOnConnect() &&
                    scanButton.enabled && canList.count == 0 &&
                    VescIf.isPortConnected() &&
                    VescIf.fwRx() && VescIf.customConfigRxDone()) {
                scanButton.clicked()
            }
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: 10

        Text {
            id: text
            Layout.fillWidth: true
            color: Utility.getAppHexColor("lightText")
            text: qsTr("CAN Devices")
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        ListView {
            ListModel {
                id: canModel
            }

            // transitions for insertion/deletation of elements
            add: Transition {
                NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 200 }
                NumberAnimation { property: "scale"; easing.type: Easing.OutBack; from: 0; to: 1.0; duration: 800 }
            }

            addDisplaced: Transition {
                NumberAnimation { properties: "y"; duration: 600; easing.type: Easing.InBack }
            }

            remove: Transition {
                NumberAnimation { property: "scale"; from: 1.0; to: 0; duration: 200 }
                NumberAnimation { property: "opacity"; from: 1.0; to: 0; duration: 200 }
            }

            removeDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 500; easing.type: Easing.OutExpo }
            }

            id: canList
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true
            clip: true
            spacing: 5

            Component {
                id: canDelegate
                Rectangle {
                    Component.onCompleted: showAnim.start();
                    transform: Rotation { id:rt; origin.x: 0; origin.y: height; axis { x: 0.3; y: 1; z: 0 } angle: 0}//     <--- I like this one more!
                    SequentialAnimation {
                        id: showAnim
                        running: false
                        RotationAnimation { target: rt; from: 180; to: 0; duration: 800; easing.type: Easing.OutExpo; property: "angle" }
                    }
                    width: canList.width
                    height: 40
                    color: canList.currentIndex === index ? Utility.getAppHexColor("darkAccent") : Utility.getAppHexColor("normalBackground")
                    radius: 10
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            canList.currentIndex = index
                            if(index === 0){
                                mCommands.setSendCan(false,0)
                            }else{
                                mCommands.setSendCan(true,ID)
                            }
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 10
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10

                        Image {
                            id: image
                            fillMode: Image.PreserveAspectFit
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                            source: deviceIconPath
                        }
                        Text {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                            text: name
                            horizontalAlignment: Text.AlignHCenter
                            color: Utility.getAppHexColor("lightText")
                            wrapMode: Text.WordWrap
                        }
                        Rectangle{
                            color: "#00000000"
                            Layout.preferredWidth: t_metrics.boundingRect.width + 10
                            Layout.preferredHeight: idText.implicitHeight * 1.5
                            border.color: Utility.getAppHexColor("lightText")
                            border.width: 2
                            radius: 2

                            TextMetrics {
                                id:     t_metrics
                                font:   idText.font
                                text:   "LOCAL"
                            }

                            Text {
                                id:idText
                                textFormat: Text.RichText
                                anchors.centerIn: parent
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                text: ID
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                color: Utility.getAppHexColor("lightText")
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
            model: canModel
            delegate: canDelegate
        }

        Button {
            id: scanButton
            text: qsTr("CAN Scan")
            enabled: false
            Layout.fillWidth: true
            onClicked: {
                scanButton.enabled = false
                canModel.clear()

                VescIf.canTmpOverride(false, -1)
                mCommands.pingCan()
                VescIf.canTmpOverrideEnd()
            }
        }
    }

    Connections {
        target: mBle

        function onBleError(info) {
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

    Connections {
        target: VescIf

        function onPortConnectedChanged() {
            scanButton.enabled = VescIf.isPortConnected()
        }
    }

    Connections {
        target: mCommands

        function onPingCanRx(devs, isTimeout) {
            if (scanButton.enabled) {
                return
            }

            scanButton.enabled = true
            scanButton.text = qsTr("Scan")

            if (VescIf.isPortConnected()) {
                canModel.clear()
                var params = Utility.getFwVersionBlockingCan(VescIf, -1)
                var name = params.hw
                var theme ="qrc"  + Utility.getThemePath()
                var devicePath
                var logoPath = " "
                if (params.major === -1){
                    devicePath = theme + "icons/Help-96.png"
                    name = "Unknown"
                } else if (params.hwTypeStr() === "VESC") { //is a motor
                    devicePath = theme + "icons/motor_side.png"
                    name = params.hw
                } else if (params.hwTypeStr() === "VESC BMS") { //is a bms
                    devicePath = theme + "icons/icons8-battery-100.png"
                    name = "BMS (" + params.hw + ")"
                } else {
                    devicePath = theme + "icons/Electronics-96.png"
                    name = "Device (" + params.hw + ")"
                }
                name = name.replace("_", " ")

                canModel.append({"name": name,
                                    "ID": "LOCAL",
                                    "deviceIconPath": devicePath,
                                    "logoIconPath": logoPath})
                for (var i = 0;i < devs.length;i++) {
                    params = Utility.getFwVersionBlockingCan(VescIf, devs[i])
                    name = params.hw
                    if (params.hwTypeStr() === "VESC") { //is a motor
                        devicePath = theme + "icons/motor_side.png"
                        name = params.hw;
                    } else if (params.hwTypeStr() === "VESC BMS") { //is a bms
                        devicePath = theme + "icons/icons8-battery-100.png"
                        name = "BMS (" + params.hw + ")";
                    } else {
                        devicePath = theme + "icons/Electronics-96.png"
                        name = "Device (" + params.hw + ")"
                    }
                    name = name.replace("_", " ")
                    canModel.append({"name": name,
                                        "ID": devs[i].toString(),
                                        "deviceIconPath": devicePath,
                                        "logoIconPath": logoPath})
                }

                canList.currentIndex = 0

                if (!isTimeout) {
                    VescIf.emitStatusMessage("CAN Scan Finished", true)
                } else {
                    VescIf.emitStatusMessage("CAN Scan Timed Out", false)
                }

                selectDeviceInList()
            } else {
                VescIf.emitStatusMessage("Device not connected", false)
            }
        }
    }
}
