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

import QtQuick
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0 as QSettings

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.utility 1.0

Item {
    property bool showControls: true

    function connectToHub() {
        VescIf.connectTcpHub(serverText.text, portBox.value, idText.text, passText.text)
    }

    implicitHeight: grid.implicitHeight

    GridLayout {
        id: grid
        anchors.fill: parent

        clip: false
        visible: true
        columnSpacing: 5
        columns: 2

        CheckBox {
            id: enableBox
            focusPolicy: Qt.NoFocus
            text: "Connect as Server"
            Layout.fillWidth: true
            Layout.columnSpan: 2
            visible: showControls

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

        TextField {
            id: serverText
            Layout.fillWidth: true
            placeholderText: "Server"
            text: "veschub.vedder.se"
            clip: true
            QSettings.Settings {
                property alias tcp_hub_server: serverText.text
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

        TextField {
            id: idText
            Layout.fillWidth: true
            placeholderText: "VESC ID"
            text: ""
            clip: true
            QSettings.Settings {
                property alias tcp_hub_vesc_id: idText.text
            }
        }

        Text {
            text: "Password"
            color: Utility.getAppHexColor("lightText")
            Layout.fillWidth: false
        }

        TextField {
            id: passText
            Layout.fillWidth: true
            placeholderText: "Password"
            text: ""
            clip: true
                QSettings.Settings {
                    property alias tcp_hub_vesc_pass: passText.text
                }
        }

        RowLayout {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            visible: showControls

            Button {
                Layout.topMargin: 5
                Layout.fillWidth: true
                text: "Reset Defaults"
                enabled: !enableBox.checked
                focusPolicy: Qt.NoFocus

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
                focusPolicy: Qt.NoFocus

                onClicked: {
                    connectToHub()
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
