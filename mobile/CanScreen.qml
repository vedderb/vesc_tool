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
import QtQuick.Controls.Material
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.bleuart 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0

Item {
    id: rootItem

    property var dialogParent: Overlay.overlay
    property BleUart mBle: VescIf.bleDevice()
    property Commands mCommands: VescIf.commands()
    property int animationSpeed: 500
    property real lastScan: 0
    property string lastScanText: ""

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
                lastScan = 0
                lastScanText = ""
            } else {
                selectDeviceInList()
            }

            if (lastScan > 0) {
                lastScanText = qsTr("Last scan %1").arg(timeAgo(lastScan))
            }

            // Once per connection, as a scan that is still filling the list also leaves it empty
            if (VescIf.scanCanOnConnect() && lastScan === 0 &&
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

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 4
            Layout.bottomMargin: 8

            Text {
                Layout.fillWidth: true
                color: Utility.getAppHexColor("titleText")
                text: qsTr("CAN Devices")
                font.pointSize: 15
                font.weight: Font.Medium
            }

            Text {
                color: Utility.getAppHexColor("normalText")
                text: qsTr("%1 found").arg(canModel.count)
                font.pointSize: 10
            }
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
            spacing: 6

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
                    height: 56
                    color: canList.currentIndex === index ? Qt.tint(Utility.getAppHexColor("darkBackground"), Qt.alpha(Material.accent, 0.1)) :
                                                            Utility.getAppHexColor("cardBackground")
                    border.width: canList.currentIndex === index ? 1.5 : 1
                    border.color: canList.currentIndex === index ? Material.accent : Utility.getAppHexColor("raisedBackground")
                    radius: 12
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
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                Layout.fillWidth: true
                                text: name
                                elide: Text.ElideRight
                                font.weight: Font.Medium
                                color: Utility.getAppHexColor("titleText")
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "ID " + (ID === "LOCAL" ? VescIf.appConfig().getParamInt("controller_id") : ID) + " · FW " + fw
                                elide: Text.ElideRight
                                font.pointSize: 9
                                color: Utility.getAppHexColor("normalText")
                            }
                        }

                        Chip {
                            enabled: false
                            tinted: ID === "LOCAL"
                            text: ID === "LOCAL" ? "LOCAL" : "CAN"
                        }
                    }
                }
            }
            model: canModel
            delegate: canDelegate
        }

        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            visible: lastScanText !== ""
            text: lastScanText
            font.pointSize: 9
            color: Utility.getAppHexColor("normalText")
        }

        Button {
            id: scanButton
            text: qsTr("CAN Scan")
            enabled: false
            Layout.fillWidth: true
            icon.source: "qrc" + Utility.getThemePath() + "icons/Refresh-96.png"
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

    BusyDialog {
        id: commDialog
        title: "Connecting..."
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
            lastScan = Date.now()

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
                    if (params.fwName.length !== 0) {
                        name = params.hw + "-" + params.fwName
                    } else {
                        name = params.hw
                    }
                } else if (params.hwTypeStr() === "VESC BMS") { //is a bms
                    devicePath = theme + "icons/icons8-battery-100.png"
                    if (params.fwName.length !== 0) {
                        name = params.hw + "-" + params.fwName
                    } else {
                        name = params.hw
                    }
                } else {
                    devicePath = theme + "icons/Electronics-96.png"
                    if (params.fwName.length !== 0) {
                        name = params.hw + "-" + params.fwName
                    } else {
                        name = params.hw
                    }
                }
                name = name.replace("_", " ")

                canModel.append({"name": name,
                                    "ID": "LOCAL",
                                    "fw": fwString(params),
                                    "deviceIconPath": devicePath,
                                    "logoIconPath": logoPath})
                var canMotor = -1
                for (var i = 0;i < devs.length;i++) {
                    params = Utility.getFwVersionBlockingCan(VescIf, devs[i])
                    name = params.hw
                    if (params.hwTypeStr() === "VESC") { //is a motor
                        canMotor = canMotor < 0 ? devs[i] : canMotor
                        devicePath = theme + "icons/motor_side.png"
                        if (params.fwName.length !== 0) {
                            name = params.hw + "-" + params.fwName
                        } else {
                            name = params.hw
                        }
                    } else if (params.hwTypeStr() === "VESC BMS") { //is a bms
                        devicePath = theme + "icons/icons8-battery-100.png"
                        if (params.fwName.length !== 0) {
                            name = params.hw + "-" + params.fwName
                        } else {
                            name = params.hw
                        }
                    } else {
                        devicePath = theme + "icons/Electronics-96.png"
                        if (params.fwName.length !== 0) {
                            name = params.hw + "-" + params.fwName
                        } else {
                            name = params.hw
                        }
                    }
                    name = name.replace("_", " ")
                    canModel.append({"name": name,
                                        "ID": devs[i].toString(),
                                        "fw": fwString(params),
                                        "deviceIconPath": devicePath,
                                        "logoIconPath": logoPath})
                }

                canList.currentIndex = 0

                canMotorId = canMotor

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
