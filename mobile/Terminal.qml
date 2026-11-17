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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id:terminalPageItem
    property Commands mCommands: VescIf.commands()
    property var history: []
    property int historyIndex: -1

    // Dims the key of "key : value" lines and shows firmware fault codes in red
    function formatLine(html) {
        var kv = html.match(/^(\s*[A-Za-z][\w .()\/-]{0,40}?\s*:)(\s.*)?$/)
        if (kv) {
            html = "<font color='" + Utility.getAppHexColor("normalText") + "'>" + kv[1] + "</font>" + (kv[2] || "")
        }

        return html.replace(/FAULT_CODE_(?!NONE\b)\w+/g, function(code) {
            return "<font color='" + Utility.getAppHexColor("dangerText") + "'>" + code + "</font>"
        })
    }

    function appendOutput(str, color) {
        for (var line of str.split("\n")) {
            var html = line.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
            terminalText.append("<div style='white-space:pre'>" + (color ? "<font color='" + color + "'>" + html + "</font>" :
                                                                            formatLine(html)) + "</div>")
        }
    }

    function send(cmd) {
        if (cmd === "") {
            return
        }

        appendOutput("\u203A " + cmd, Utility.getAppHexColor("infoText"))
        mCommands.sendTerminalCmd(cmd)
        history = [cmd].concat(history.filter(h => h !== cmd))
        historyIndex = -1
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 8

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: Utility.getAppHexColor("darkBackground")
            border.color: Utility.getAppHexColor("raisedBackground")

            ScrollView {
                id: scroll
                anchors.fill: parent
                anchors.margins: 4
                // At least as wide as the view, so short output leaves sideways swipes to the page swiper
                contentWidth: Math.max(terminalText.implicitWidth, availableWidth)
                clip: true

                TextArea {
                    id: terminalText
                    width: parent.width
                    readOnly: true
                    selectByMouse: Qt.platform.os !== "ios" && Qt.platform.os !== "android"
                    rightPadding: 48
                    textFormat: TextEdit.RichText
                    font.family: "DejaVu Sans Mono"
                    font.pointSize: 10
                    background: null
                }
            }

            RoundButton {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 4
                icon.source: "qrc" + Utility.getThemePath() + "icons_textedit/Copy-96.png"
                Accessible.name: qsTr("Copy")
                visible: terminalText.length > 0
                onClicked: {
                    terminalText.selectAll()
                    terminalText.copy()
                    terminalText.deselect()
                    showToast(qsTr("Terminal output copied"), "", "info")
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.topMargin: -6
            Layout.bottomMargin: -6
            orientation: ListView.Horizontal
            spacing: 6
            clip: true
            model: ["faults", "threads", "help", "hw_status"]

            delegate: Chip {
                text: modelData
                font.family: "DejaVu Sans Mono"
                onClicked: send(modelData)
            }
        }

        RowLayout {
            Layout.fillWidth: true

            InputField {
                id: stringInput
                Layout.fillWidth: true
                placeholderText: "Terminal Command"
                font.family: "DejaVu Sans Mono"
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                onAccepted: {
                    send(text)
                    text = ""
                }
                Keys.onUpPressed: {
                    if (historyButton.enabled) {
                        historyButton.clicked()
                    }
                }
            }

            ToolButton {
                id: historyButton
                text: "\u2191"
                font.pointSize: 18
                enabled: history.length > 0
                Accessible.name: qsTr("Previous command")
                onClicked: {
                    historyIndex = (historyIndex + 1) % history.length
                    stringInput.text = history[historyIndex]
                }
            }

            Button {
                text: "Send"
                highlighted: true
                onClicked: {
                    send(stringInput.text)
                    stringInput.clear()
                }
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
                text: "Reboot"

                onClicked: {
                    rebootDialog.open()
                }

                SheetDialog {
                    id: rebootDialog
                    title: "Reboot"
                    acceptText: qsTr("Reboot")
                    onAccepted: mCommands.reboot()

                    Text {
                        anchors.fill: parent
                        color: Utility.getAppHexColor("lightText")
                        wrapMode: Text.WordWrap
                        text: "This is going to reboot the VESC. Continue?"
                    }
                }
            }

            Button {
                Layout.preferredWidth: 56
                text: "..."
                onClicked: menu.open()

                BottomMenu {
                    id: menu
                    parent: terminalPageItem

                    MenuItem {
                        text: "Shutdown"
                        onTriggered: {
                            mCommands.shutdown()
                        }
                    }
                    MenuItem {
                        text: "Restart LispBM"
                        onTriggered: {
                            mCommands.lispSetRunning(1)
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: mCommands

        function onPrintReceived(str) {
            appendOutput(str)
        }
    }
}
