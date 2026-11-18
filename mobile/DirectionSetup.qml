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
    property var dialogParent: Overlay.overlay
    // Wizard step title. When set, it and the hint go beside the list on landscape phones
    property string title: ""

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

    GridLayout {
        id: column
        anchors.fill: parent
        columns: phoneLandscape && title !== "" ? 2 : 1
        columnSpacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            spacing: 2

            Text {
                Layout.fillWidth: true
                visible: column.columns > 1
                text: title
                color: Utility.getAppHexColor("lightText")
                font.bold: true
                wrapMode: Text.WordWrap
            }

            Text {
                id: text
                Layout.fillWidth: true
                color: Utility.getAppHexColor("normalText")
                text: qsTr("Tap Forward to spin a motor briefly. If it turns the wrong way, flip the Inverted switch.")
                font.pointSize: 11
                wrapMode: Text.WordWrap
            }
        }

        ListModel {
            id: canIdModel
        }

        ListView {
            id: canIdList
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            focus: true
            clip: true
            spacing: 5

            Component {
                id: canIdDelegate

                Rectangle {
                    property variant modelData: model

                    width: canIdList.width
                    height: cardColumn.implicitHeight + 24
                    color: Utility.getAppHexColor("cardBackground")
                    border.color: Utility.getAppHexColor("raisedBackground")
                    radius: 12

                    ColumnLayout {
                        id: cardColumn
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Image {
                                id: image
                                fillMode: Image.PreserveAspectFit
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                source: isCan ? "qrc" + Utility.getThemePath() + "icons/can_off.png" : "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    Layout.fillWidth: true
                                    text: name
                                    color: Utility.getAppHexColor("lightText")
                                    font.weight: Font.Medium
                                    wrapMode: Text.WordWrap
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("ID %1").arg(canId)
                                    color: Utility.getAppHexColor("normalText")
                                    font.pointSize: 11
                                }
                            }

                            Text {
                                color: Utility.getAppHexColor("normalText")
                                font.pointSize: 11
                                text: "Inverted"
                            }

                            Switch {
                                id: invertSwitch
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

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                text: qsTr("Forward")

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
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                text: qsTr("Reverse")

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

    BusyDialog {
        id: commDialog
        title: "Communicating..."
    }
}
