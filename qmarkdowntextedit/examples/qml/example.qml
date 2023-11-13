import QtQuick 2.0
import QtQuick.Window 2.0
import MarkdownHighlighter 1.0

Window {
    id: mainwindow
    width: 640
    height: 400
    visible: true
    title: qsTr("QtQuick Project")

    TextEdit {
        id: editor
        text: "# Hello world!"
        focus: true
    }

    MarkdownHighlighter {
        id: syntaxHighlighter
        textDocument: editor.textDocument
    }
}
