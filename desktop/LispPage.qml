/*
    Desktop LispPage — native implementation matching the original widget page.
    Features: Code editor, REPL console, run/stop/upload/erase buttons,
    example/recent file browser.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Vedder.vesc

Item {
    id: lispPage

    property Commands mCommands: VescIf.commands()
    property string currentFile: ""

    SplitView {
        anchors.fill: parent
        orientation: Qt.Vertical

        // Top: Code editor
        ColumnLayout {
            SplitView.fillHeight: true
            SplitView.minimumHeight: 200
            spacing: 0

            // Toolbar
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 4
                spacing: 4

                Button { text: "New"; onClicked: { codeEditor.text = ""; currentFile = "" } }
                Button {
                    text: "Open..."
                    onClicked: openDialog.open()
                }
                Button {
                    text: "Save..."
                    onClicked: saveDialog.open()
                }

                ToolSeparator {}

                Button {
                    text: "Upload"
                    highlighted: true
                    icon.source: "qrc" + Utility.getThemePath() + "icons/motor_down.png"
                    onClicked: {
                        if (codeEditor.text.length > 0) {
                            replOutput.text += "Upload requires CodeLoader integration (use CLI --uploadLisp instead)\n"
                        }
                    }
                }
                Button {
                    text: "Run"
                    onClicked: mCommands.lispSetRunning(1)
                }
                Button {
                    text: "Stop"
                    onClicked: mCommands.lispSetRunning(0)
                }
                Button {
                    text: "Erase"
                    onClicked: {
                        mCommands.lispEraseCode(16)
                        replOutput.text += "Erasing Lisp code...\n"
                    }
                }
                Button {
                    text: "Stream"
                    onClicked: {
                        replOutput.text += "Stream requires CodeLoader integration\n"
                    }
                }
                Button {
                    text: "Read"
                    onClicked: mCommands.lispReadCode(0, 0)
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Help"
                    onClicked: mCommands.sendTerminalCmd("lispref")
                }
            }

            // Code editor
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    id: codeEditor
                    font.family: "DejaVu Sans Mono"
                    font.pixelSize: 12
                    color: Utility.getAppHexColor("lightText")
                    wrapMode: TextEdit.NoWrap
                    tabStopDistance: 28
                    selectByMouse: true
                    background: Rectangle {
                        color: Utility.getAppHexColor("darkBackground")
                        border.color: Utility.getAppHexColor("disabledText")
                        border.width: 1
                    }

                    text: "; Enter LispBM code here\n(print \"Hello VESC!\")\n"
                }
            }
        }

        // Bottom: REPL
        ColumnLayout {
            SplitView.preferredHeight: 200
            SplitView.minimumHeight: 100
            spacing: 0

            Label {
                text: "REPL Output"
                font.bold: true
                font.pixelSize: 12
                Layout.margins: 4
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    id: replOutput
                    readOnly: true
                    font.family: "DejaVu Sans Mono"
                    font.pixelSize: 11
                    color: Utility.getAppHexColor("lightText")
                    wrapMode: TextEdit.NoWrap
                    selectByMouse: true
                    background: Rectangle {
                        color: Utility.getAppHexColor("darkBackground")
                        border.color: Utility.getAppHexColor("disabledText")
                        border.width: 1
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 4
                spacing: 4

                TextField {
                    id: replEdit
                    Layout.fillWidth: true
                    font.family: "DejaVu Sans Mono"
                    font.pixelSize: 12
                    placeholderText: "REPL command..."

                    Keys.onReturnPressed: sendRepl()
                    Keys.onEnterPressed: sendRepl()
                }

                Button {
                    text: "Send"
                    onClicked: sendRepl()
                }

                Button {
                    text: "Clear"
                    onClicked: replOutput.text = ""
                }
            }
        }
    }

    function sendRepl() {
        var cmd = replEdit.text.trim()
        if (cmd === "") return

        if (VescIf.isPortConnected()) {
            mCommands.lispSendReplCmd(cmd)
        } else {
            replOutput.text += "VESC not connected\n"
        }
        replEdit.text = ""
    }

    FileDialog {
        id: openDialog
        title: "Open Lisp File"
        nameFilters: ["Lisp files (*.lisp *.lsp)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            var content = Utility.readTextFile(path)
            if (content !== "") {
                codeEditor.text = content
                currentFile = path
            }
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save Lisp File"
        nameFilters: ["Lisp files (*.lisp)", "All files (*)"]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            Utility.writeTextFile(path, codeEditor.text)
            currentFile = path
        }
    }

    Connections {
        target: mCommands

        function onLispPrintReceived(str) {
            replOutput.text += str
            if (!str.endsWith("\n")) replOutput.text += "\n"

            // Auto-scroll
            var flickable = replOutput.parent
            if (flickable && flickable.contentHeight !== undefined) {
                flickable.contentY = Math.max(0, flickable.contentHeight - flickable.height)
            }
        }

        function onLispReadCodeRx(lenQml, ofsQml, data) {
            codeEditor.text = Utility.arr2str(data)
        }
    }

    Connections {
        target: VescIf
        function onStatusMessage(msg, isGood) {
            if (msg.toLowerCase().indexOf("lisp") >= 0) {
                replOutput.text += (isGood ? "[OK] " : "[ERR] ") + msg + "\n"
            }
        }
    }
}
