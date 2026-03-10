/*
    Desktop TerminalPage — native implementation matching the original widget page.
    Features: read-only output, command input with history, Help/Send/Clear buttons,
    line-count trimming (max 5000 lines), auto-scroll, connection check.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    id: terminalPage

    property Commands mCommands: VescIf.commands()
    property var commandHistory: []
    property int historyIndex: -1
    property int maxLines: 5000
    property int trimLines: 1000

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // Terminal output
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TextArea {
                id: terminalBrowser
                readOnly: true
                font.family: "DejaVu Sans Mono"
                font.pointSize: 12
                wrapMode: TextEdit.NoWrap
                color: Utility.getAppHexColor("lightText")
                background: Rectangle {
                    color: Utility.getAppHexColor("darkBackground")
                    border.color: Utility.getAppHexColor("disabledText")
                    border.width: 1
                }
                selectByMouse: true
            }
        }

        // Command input row
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Button {
                text: "Help"
                implicitWidth: 60
                onClicked: {
                    terminalEdit.text = "help"
                    sendCommand()
                }
            }

            TextField {
                id: terminalEdit
                Layout.fillWidth: true
                placeholderText: "Enter command..."
                font.family: "DejaVu Sans Mono"
                font.pointSize: 12

                Keys.onReturnPressed: sendCommand()
                Keys.onEnterPressed: sendCommand()

                Keys.onUpPressed: {
                    if (commandHistory.length > 0) {
                        if (historyIndex < commandHistory.length - 1) {
                            historyIndex++
                        }
                        terminalEdit.text = commandHistory[commandHistory.length - 1 - historyIndex]
                    }
                }

                Keys.onDownPressed: {
                    if (historyIndex > 0) {
                        historyIndex--
                        terminalEdit.text = commandHistory[commandHistory.length - 1 - historyIndex]
                    } else {
                        historyIndex = -1
                        terminalEdit.text = ""
                    }
                }
            }

            Button {
                text: "Send"
                implicitWidth: 60
                onClicked: sendCommand()
            }

            Button {
                text: "Clear"
                implicitWidth: 60
                onClicked: terminalBrowser.text = ""
            }
        }

        // Extra commands row
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Button {
                text: "Print Faults"
                onClicked: {
                    if (VescIf.isPortConnected()) {
                        mCommands.sendTerminalCmd("faults")
                    }
                }
            }
            Button {
                text: "Print Threads"
                onClicked: {
                    if (VescIf.isPortConnected()) {
                        mCommands.sendTerminalCmd("threads")
                    }
                }
            }
            Button {
                text: "DRV Reset"
                onClicked: {
                    if (VescIf.isPortConnected()) {
                        mCommands.sendTerminalCmd("drv8301_reset_latched_faults")
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Reboot"
                onClicked: mCommands.reboot()
            }
            Button {
                text: "Shutdown"
                onClicked: mCommands.shutdown()
            }
        }
    }

    function sendCommand() {
        var cmd = terminalEdit.text.trim()
        if (cmd === "") return

        if (!VescIf.isPortConnected()) {
            appendOutput("VESC not connected\n")
            return
        }

        mCommands.sendTerminalCmd(cmd)

        // Add to history (avoid duplicates of the last entry)
        if (commandHistory.length === 0 || commandHistory[commandHistory.length - 1] !== cmd) {
            commandHistory.push(cmd)
        }
        historyIndex = -1

        terminalEdit.text = ""
    }

    function appendOutput(text) {
        terminalBrowser.text += text
        if (!text.endsWith("\n")) {
            terminalBrowser.text += "\n"
        }

        // Trim old lines
        var lines = terminalBrowser.text.split("\n")
        if (lines.length > maxLines) {
            lines.splice(0, trimLines)
            terminalBrowser.text = lines.join("\n")
        }

        // Auto-scroll to bottom
        var flickable = terminalBrowser.parent
        if (flickable && flickable.contentHeight !== undefined) {
            flickable.contentY = Math.max(0, flickable.contentHeight - flickable.height)
        }
    }

    Connections {
        target: mCommands
        function onPrintReceived(str) {
            appendOutput(str)
        }
    }

    // Auto-focus the input when the page becomes visible
    onVisibleChanged: {
        if (visible) {
            terminalEdit.forceActiveFocus()
        }
    }
}
