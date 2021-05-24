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

Item {
    property Commands mCommands: VescIf.commands()

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 0

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: terminalText.width
            clip: true

            TextArea {
                id: terminalText
                anchors.fill: parent
                readOnly: true
                font.family: "DejaVu Sans Mono"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Button {
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                text: "Clear"

                onClicked: {
                    terminalText.clear()
                }
            }

            Button {
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                text: "Send"

                onClicked: {
                    mCommands.sendTerminalCmd(stringInput.text)
                    stringInput.clear()
                }
            }

            Button {
                Layout.preferredWidth: 50
                Layout.fillWidth: true
                text: "..."
                onClicked: menu.open()

                Menu {
                    id: menu
                    width: 500

                    MenuItem {
                        text: "Print Faults"
                        onTriggered: {
                            mCommands.sendTerminalCmd("faults")
                        }
                    }
                    MenuItem {
                        text: "Print Threads"
                        onTriggered: {
                            mCommands.sendTerminalCmd("threads")
                        }
                    }
                    MenuItem {
                        text: "Show Help"
                        onTriggered: {
                            mCommands.sendTerminalCmd("help")
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.bottomMargin: 10
            height: stringInput.implicitHeight + 14
            border.width: 2
            border.color: "#8d8d8d"
            color: "#33a8a8a8"
            radius: 3
            TextInput {
                id: stringInput
                color: "white"
                anchors.fill: parent
                anchors.margins: 7
                font.pointSize: 12
                focus: true
            }
        }
    }

    Connections {
        target: mCommands

        onPrintReceived: {
            terminalText.text += "\n" + str
        }
    }
}
