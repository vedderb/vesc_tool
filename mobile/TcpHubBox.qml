/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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
import Vedder.vesc.utility 1.0

Item {
    implicitHeight: grid.implicitHeight

    GridLayout {
        id: grid
        anchors.topMargin: -5
        anchors.bottomMargin: -5
        anchors.fill: parent

        clip: false
        visible: true
        rowSpacing: -10
        columnSpacing: 5
        columns: 2

        CheckBox {
            id: enableBox
            text: "Connect as Server"
            Layout.fillWidth: true
            Layout.columnSpan: 2

            onClicked: {
                if (checked) {
                    VescIf.tcpServerConnectToHub(
                                serverText.text,
                                portBox.value,
                                idText.text,
                                passText.text)
                } else {
                    VescIf.tcpServerStop()
                }
            }
        }

        Text {
            text: "Server"
            color: Utility.getAppHexColor("lightText")
            Layout.fillWidth: false
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 6
            Layout.bottomMargin: 6
            height: serverText.implicitHeight + 14
            border.width: 2
            border.color: Utility.getAppHexColor("disabledText")
            color: Utility.getAppHexColor("normalBackground")
            radius: 3
            enabled: !enableBox.checked

            TextInput {
                color: Utility.getAppHexColor("lightText")
                id: serverText
                anchors.fill: parent
                anchors.margins: 7
                font.pointSize: 12
                text: "veschub.vedder.se"
                clip: true
                QSettings.Settings {
                    property alias tcp_hub_server: serverText.text
                }
            }
        }

        Text {
            text: "TCP Port"
            color: Utility.getAppHexColor("lightText")
            Layout.fillWidth: false
        }

        SpinBox {
            id: portBox
            from: 0
            to: 65535
            value: 65101
            enabled: !enableBox.checked
            Layout.fillWidth: true
            editable: true
        }

        Text {
            text: "VESC ID"
            color: Utility.getAppHexColor("lightText")
            Layout.fillWidth: false
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 6
            Layout.bottomMargin: 6
            height: idText.implicitHeight + 14
            border.width: 2
            border.color: Utility.getAppHexColor("disabledText")
            color: Utility.getAppHexColor("normalBackground")
            radius: 3
            enabled: !enableBox.checked

            TextInput {
                color: Utility.getAppHexColor("lightText")
                id: idText
                anchors.fill: parent
                anchors.margins: 7
                font.pointSize: 12
                text: ""
                clip: true
                QSettings.Settings {
                    property alias tcp_hub_vesc_id: idText.text
                }
            }
        }

        Text {
            text: "Password"
            color: Utility.getAppHexColor("lightText")
            Layout.fillWidth: false
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 6
            Layout.bottomMargin: 6
            height: passText.implicitHeight + 14
            border.width: 2
            border.color: Utility.getAppHexColor("disabledText")
            color: Utility.getAppHexColor("normalBackground")
            radius: 3
            enabled: !enableBox.checked

            TextInput {
                color: Utility.getAppHexColor("lightText")
                id: passText
                anchors.fill: parent
                anchors.margins: 7
                font.pointSize: 12
                text: ""
                clip: true
                QSettings.Settings {
                    property alias tcp_hub_vesc_pass: passText.text
                }
            }
        }

        RowLayout {
            Layout.columnSpan: 2
            Layout.fillWidth: true

            Button {
                Layout.topMargin: 5
                Layout.fillWidth: true
                text: "Reset Defaults"
                enabled: !enableBox.checked

                onClicked: {
                    serverText.text = "veschub.vedder.se"
                    portBox.value = 65101
                    idText.text = Utility.strCrc32(VescIf.getConnectedUuid())
                }
            }

            Button {
                Layout.topMargin: 5
                Layout.fillWidth: true
                text: "Connect as Client"
                enabled: !enableBox.checked

                onClicked: {
                    VescIf.connectTcpHub(serverText.text, portBox.value, idText.text, passText.text)
                }
            }
        }

        Timer {
            repeat: true
            running: true
            interval: 500

            onTriggered: {
                enableBox.checked = VescIf.tcpServerIsClientConnected()
            }
        }
    }
}
