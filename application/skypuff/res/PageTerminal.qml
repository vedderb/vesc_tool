/*
    Copyright 2018 - 2019 Benjamin Vedder	benjamin@vedder.se

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
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

import SkyPuff.vesc.winch 1.0

Page {
    property Commands mCommands: VescIf.commands()

    function sendCommand() {
        mCommands.sendTerminalCmd(stringInput.text)
        stringInput.clear()
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 0

        /*Chart {
            id: chart
            Layout.fillWidth: true
            height: 100

            property int v1: 65
            onPaint: {
                line({
                         labels : ["January","March","April","May","June","July"],
                         datasets : [
                             {
                                 fillColor : "rgba(220,220,220,0.5)",
                                 strokeColor : "rgba(220,220,220,1)",
                                 pointColor : "rgba(220,220,220,1)",
                                 pointStrokeColor : "#fff",
                                 data : [v1,59,90,81,56,55,40]
                             },
                             {
                                 fillColor : "rgba(151,187,205,0.5)",
                                 strokeColor : "rgba(151,187,205,1)",
                                 pointColor : "rgba(151,187,205,1)",
                                 pointStrokeColor : "#fff",
                                 data : [28,48,40,19,96,27,100]
                             }
                         ]
                     },
                     {
                         bezierCurve: false
                     }
                );
            }
        }
        Button {
            Layout.fillWidth: true

            text: 'pop'

            onClicked: {
                chart.v1++
                chart.requestPaint()
            }
        }*/

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 10
            contentWidth: terminalText.width
            clip: true

            TextArea {
                id: terminalText
                readOnly: true
                font.family: "DejaVu Sans Mono"
                font.pointSize: 10
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10

            TextField {
                id: stringInput
                Layout.fillWidth: true
                focus: true
                cursorVisible: true

                onAccepted: {sendCommand()}
            }

            Button {
                text: qsTr("Send")
                enabled: Skypuff.state != "DISCONNECTED"

                onClicked: {sendCommand()}
            }
            Button {
                text: qsTr("Clear")

                onClicked: {
                    terminalText.clear()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10

            Label {
                text: qsTr('Avg response %1ms').arg(Skypuff.avgResponseMillis.toFixed(1))
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                text: qsTr('Min / max: %1 / %2ms').arg(Skypuff.minResponseMillis).arg(Skypuff.maxResponseMillis)
            }
        }
    }

    Connections {
        target: mCommands

        onPrintReceived: {
            // Do not become too fat
            if(terminalText.text.length > 1024*16)
                terminalText.clear()

            terminalText.text = terminalText.text.concat("\n", str);
            scroll.contentItem.contentY = terminalText.height - scroll.height
        }
    }
}
