/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

ApplicationWindow {
    visible: true
    width: 800
    height: 480
    title: qsTr("My Application")
    
    property Commands mCommands: VescIf.commands()

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        Page {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                
                Button {
                    Layout.fillWidth: true
                    text: "Connect"

                    onClicked: {
                        VescIf.autoconnect()
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: "Disconnect"

                    onClicked: {
                        VescIf.disconnectPort()
                    }
                }

                Text {
                    id: connText
                    text: VescIf.getConnectedPortName()
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillHeight: true
                }
            }
        }

        Page {
            ColumnLayout {
                anchors.fill: parent
            
                Text {
                    id: valText
                    text: VescIf.getConnectedPortName()
                    verticalAlignment: Text.AlignVCenter
                    font.family: "DejaVu Sans Mono"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 10
                }
            }
        }
    }

    footer: TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex
        TabButton {
            text: qsTr("Connection")
        }
        TabButton {
            text: qsTr("Data")
        }
    }
    
    Timer {
        id: rtTimer
        interval: 50
        running: true
        repeat: true

        onTriggered: {
            connText.text = VescIf.getConnectedPortName()

            if (VescIf.isPortConnected() && tabBar.currentIndex == 1) {
                // Sample RT data when the RT page is selected
                mCommands.getValues()
            }
        }
    }
    
    Connections {
        target: mCommands

        onValuesReceived: {
            valText.text =
                    "Battery    : " + parseFloat(values.v_in).toFixed(2) + " V\n" +
                    "I Battery  : " + parseFloat(values.current_in).toFixed(2) + " A\n" +
                    "Temp MOS   : " + parseFloat(values.temp_mos).toFixed(2) + " \u00B0C\n" +
                    "Temp Motor : " + parseFloat(values.temp_motor).toFixed(2) + " \u00B0C\n" +
                    "Ah Draw    : " + parseFloat(values.amp_hours * 1000.0).toFixed(1) + " mAh\n" +
                    "Ah Charge  : " + parseFloat(values.amp_hours_charged * 1000.0).toFixed(1) + " mAh\n" +
                    "Wh Draw    : " + parseFloat(values.watt_hours).toFixed(2) + " Wh\n" +
                    "Wh Charge  : " + parseFloat(values.watt_hours_charged).toFixed(2) + " Wh\n" +
                    "ABS Tacho  : " + values.tachometer_abs + " Counts\n" +
                    "Fault      : " + values.fault_str
        }
    }
}
