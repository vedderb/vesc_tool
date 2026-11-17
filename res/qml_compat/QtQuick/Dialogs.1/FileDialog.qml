import QtQuick
import QtQuick.Dialogs as D

// The Qt 5 QtQuick.Dialogs 1.x FileDialog API on top of the Qt 6 FileDialog
D.FileDialog {
    id: root

    property bool selectExisting: true
    property bool selectMultiple: false
    property alias folder: root.currentFolder
    readonly property url fileUrl: selectedFile
    readonly property var fileUrls: selectedFiles

    fileMode: !selectExisting ? D.FileDialog.SaveFile
            : (selectMultiple ? D.FileDialog.OpenFiles : D.FileDialog.OpenFile)
}
