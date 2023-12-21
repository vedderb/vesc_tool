/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    property var dialogParent: ApplicationWindow.overlay

    // TODO: This is not pretty...
    implicitHeight: text.implicitHeight +
                    canIdList.contentHeight + 10

    property ConfigParams mAppConf: VescIf.appConfig()
    property Commands mCommands: VescIf.commands()

    function scanCan() {
        disableDialog()
        workaroundTimerCanScan.start()
    }
    Timer {
        id: workaroundTimerCanScan
        interval: 0
        repeat: false
        running: false
        onTriggered: {
            setCanDevs(Utility.scanCanVescOnly(VescIf))
        }
    }

    function setCanDevs(canDevs) {
        canIdModel.clear()
        if (!mCommands.getSendCan() && Utility.isConnectedToHwVesc(VescIf)) {
            canIdModel.append({"name": "This VESC",
                                  "canId": mAppConf.getParamInt("controller_id"),
                                  "isCan": false,
                                  "isInv": Utility.getInvertDirection(VescIf, -1)})
        }

        for (var i = 0;i < canDevs.length;i++) {
            canIdModel.append({"name": "VESC on CAN-bus",
                                  "canId": canDevs[i],
                                  "isCan": true,
                                  "isInv": Utility.getInvertDirection(VescIf, canDevs[i])})
        }
        enableDialog()
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.leftMargin: notchLeft
        anchors.rightMargin: notchRight

        Text {
            id: text
            Layout.fillWidth: true
            color: {color = Utility.getAppHexColor("lightText")}
            text: qsTr("Select which VESCs have inverted motor direction. Press the FWD or REV button to try.")
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        ListModel {
            id: canIdModel
        }

        ListView {
            id: canIdList
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true
            clip: true
            spacing: 5

            Component {
                id: canIdDelegate

                Rectangle {
                    property variant modelData: model

                    width: canIdList.width
                    height: 130
                    color: Utility.getAppHexColor("darkBackground")
                    radius: 5

                    RowLayout {
                        anchors.fill: parent
                        spacing: 10

                        Rectangle {
                            Layout.leftMargin: 5
                            Layout.fillWidth: true

                            color: Utility.getAppHexColor("lightAccent")
                            height: column.height
                            radius: height / 2

                            ColumnLayout {
                                id: column
                                anchors.centerIn: parent

                                Image {
                                    id: image
                                    fillMode: Image.PreserveAspectFit
                                    Layout.preferredWidth: 60
                                    Layout.preferredHeight: 60
                                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                    source: isCan ? "qrc" + Utility.getThemePath() + "icons/can_off.png" : "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                    text: name + "\nID: " + canId
                                    horizontalAlignment: Text.AlignHCenter
                                    color: Utility.getAppHexColor("lightText")
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        ColumnLayout {
                            Text {
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                                Layout.rightMargin: 5
                                color: Utility.getAppHexColor("lightText")
                                text: "Inverted"
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Switch {
                                id: invertSwitch
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                                enabled: true
                                checked: isInv
                                onToggled: {
                                    disableDialog()
                                    workaroundTimerInvertSwitch.start()
                                }
                                Timer {
                                    id: workaroundTimerInvertSwitch
                                    interval: 0
                                    repeat: false
                                    running: false
                                    onTriggered: {
                                        Utility.setInvertDirection(VescIf, isCan ? canId : -1, invertSwitch.checked)
                                        enableDialog()
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.rightMargin: 10
                            spacing: -5

                            Button {
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                                Layout.preferredWidth: 70
                                Layout.preferredHeight: 55
                                text: "FWD"

                                onClicked: {
                                    disableDialog()
                                    workaroundTimerFWD.start()
                                }
                                Timer {
                                    id: workaroundTimerFWD
                                    interval: 0
                                    repeat: false
                                    running: false
                                    onTriggered: {
                                        Utility.testDirection(VescIf, isCan ? canId : -1, 0.1, 2000)
                                        enableDialog()
                                    }
                                }
                            }

                            Button {
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                                Layout.preferredWidth: 70
                                Layout.preferredHeight: 55
                                text: "REV"

                                onClicked: {
                                    disableDialog()
                                    workaroundTimerBWD.start()
                                }
                                Timer {
                                    id: workaroundTimerBWD
                                    interval: 0
                                    repeat: false
                                    running: false
                                    onTriggered: {
                                        Utility.testDirection(VescIf, isCan ? canId : -1, -0.1, 2000)
                                        enableDialog()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            model: canIdModel
            delegate: canIdDelegate
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
        title: "Communicating..."
        closePolicy: Popup.NoAutoClose
        modal: true
        focus: true        
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 20
        x: 10
        y: parent.height / 2 - height / 2
        parent: dialogParent

        ProgressBar {
            anchors.fill: parent
            indeterminate: visible
        }
    }
}
