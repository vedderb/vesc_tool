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

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

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
        rows: 3
        columns: 2

        CheckBox {
            id: tcpServerEnBox
            text: "Activate Bridge"
            Layout.fillWidth: true
            Layout.columnSpan: 2

            onClicked: {
                if (checked) {
                    VescIf.tcpServerStart(tcpServerPortBox.value)
                } else {
                    VescIf.tcpServerStop()
                }
            }
        }

        Text {
            text: "TCP Port"
            color: Utility.getAppHexColor("lightText")
            Layout.fillWidth: true
            Layout.preferredWidth: 5000
        }

        SpinBox {
            id: tcpServerPortBox
            from: 0
            to: 65535
            value: 65102
            enabled: !tcpServerEnBox.checked
            Layout.fillWidth: true
            Layout.preferredWidth: 5000
            editable: true
        }

        Timer {
            repeat: true
            running: true
            interval: 500

            onTriggered: {
                tcpServerEnBox.checked = VescIf.tcpServerIsRunning()

                if (VescIf.tcpServerIsRunning()) {
                    var ipTxt = "IP(s)\n"
                    var addresses = Utility.getNetworkAddresses()
                    for (var i = 0;i < addresses.length;i++) {
                        ipTxt += addresses[i]
                        if (i < (addresses.length - 1)) {
                            ipTxt += "\n"
                        }
                    }
                    tcpLocalAddress.text = ipTxt
                } else {
                    tcpLocalAddress.text = "IP(s)"
                }

                tcpRemoteAddress.text = "Connected Client"

                if (VescIf.tcpServerIsClientConnected()) {
                    tcpRemoteAddress.text += "\n" + VescIf.tcpServerClientIp()
                }
            }
        }

        Text {
            id: tcpLocalAddress
            Layout.fillWidth: true
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            color: Utility.getAppHexColor("lightText")
        }

        Text {
            id: tcpRemoteAddress
            Layout.fillWidth: true
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            color: Utility.getAppHexColor("lightText")
        }
    }
}
