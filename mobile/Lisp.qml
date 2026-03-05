import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.3 as Dl
import Qt.labs.folderlistmodel 2.1
import Qt.labs.settings 1.0 as QSettings

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0
import Vedder.vesc.codeloader 1.0

Item {
    id: lispPageItem

    property var dialogParent: ApplicationWindow.overlay
    property Commands mCommands: VescIf.commands()
    property string currentFilePath: ""
    property string consoleText: ""
    property real lispCpuUse: 0
    property real lispHeapUse: 0
    property real lispMemUse: 0
    property int statsPollHz: 2
    property int editorFontSize: 12

    function fileUrlToPath(url) {
        var path = decodeURIComponent(url.toString())
        if (path.startsWith("file://")) {
            path = path.substring(7)
        }

        if (Qt.platform.os === "windows" && path.length > 2 && path[0] === "/" && path[2] === ":") {
            path = path.substring(1)
        }

        return path
    }

    function pathDir(path) {
        var slash = path.lastIndexOf("/")
        var backslash = path.lastIndexOf("\\")
        var ind = Math.max(slash, backslash)
        if (ind > 0) {
            return path.substring(0, ind)
        }

        return ""
    }

    function addRecent(path) {
        if (path.length === 0) {
            return
        }

        var arr = []
        try {
            arr = JSON.parse(recentSettings.recentFiles)
        } catch (e) {
            arr = []
        }

        var filtered = []
        for (var i = 0; i < arr.length; i++) {
            if (arr[i] !== path) {
                filtered.push(arr[i])
            }
        }

        filtered.unshift(path)
        if (filtered.length > 40) {
            filtered = filtered.slice(0, 40)
        }

        recentSettings.recentFiles = JSON.stringify(filtered)
        updateRecentModel("")
    }

    function updateRecentModel(filter) {
        recentModel.clear()

        var arr = []
        try {
            arr = JSON.parse(recentSettings.recentFiles)
        } catch (e) {
            arr = []
        }

        var flt = filter.toLowerCase()

        for (var i = 0; i < arr.length; i++) {
            var path = arr[i]
            if (flt.length === 0 || path.toLowerCase().indexOf(flt) >= 0) {
                recentModel.append({"path": path})
            }
        }
    }

    function openFilePath(path) {
        var data = Utility.readAllFromFile(path)
        editorText.text = Utility.arr2str(data)
        currentFilePath = path
        addRecent(path)
    }

    function appendConsole(text) {
        if (consoleText.length > 0) {
            consoleText += "\n"
        }
        consoleText += text

        var lines = consoleText.split("\n")
        if (lines.length > 5000) {
            lines = lines.slice(lines.length - 5000)
            consoleText = lines.join("\n")
        }
    }

    function increaseEditorFont() {
        editorFontSize = Math.min(36, editorFontSize + 1)
    }

    function decreaseEditorFont() {
        editorFontSize = Math.max(8, editorFontSize - 1)
    }

    Timer {
        id: workaroundTimerUpload
        interval: 0
        repeat: false
        running: false
        onTriggered: {
            var editorPath = currentFilePath.length > 0 ? pathDir(currentFilePath) : ""

            disableDialog()
            dlDialog.title = "Uploading..."
            var ok = mLoader.lispUploadString(editorText.text, editorPath, reduceLispBox.checked)
            enableDialog()

            if (ok && autoRunBox.checked) {
                mCommands.lispSetRunning(1)
            }
        }
    }

    Timer {
        id: workaroundTimerStream
        interval: 0
        repeat: false
        running: false
        onTriggered: {
            var editorPath = currentFilePath.length > 0 ? pathDir(currentFilePath) : ""

            disableDialog()
            dlDialog.title = "Streaming..."
            dlProgBar.indeterminate = true
            mLoader.lispStreamString(editorText.text, 0)
            enableDialog()
        }
    }

    Timer {
        id: workaroundTimerRead
        interval: 0
        repeat: false
        running: false
        onTriggered: {
            var editorPath = currentFilePath.length > 0 ? pathDir(currentFilePath) : ""

            disableDialog()
            dlDialog.title = "Reading..."
            var res = mLoader.lispReadWithPath()
            if (res.code && res.code.length > 0) {
                editorText.text = res.code
                currentFilePath = res.path
            }
            enableDialog()
        }
    }

    QSettings.Settings {
        id: recentSettings
        property string recentFiles: "[]"
    }

    ListModel {
        id: recentModel
    }

    ListModel {
        id: bindingModel
    }

    CodeLoader {
        id: mLoader
        Component.onCompleted: {
            mLoader.setVesc(VescIf)
            updateRecentModel("")
        }
    }

    Connections {
        target: mLoader

        function onDownloadProgress (bytesReceived, bytesTotal) {
            dlProgBar.indeterminate = false
            dlProgBar.to = bytesTotal
            dlProgBar.value = bytesReceived
        }

        function onLispUploadProgress (bytes, bytesTotal) {
            dlProgBar.indeterminate = false
            dlProgBar.to = bytesTotal
            dlProgBar.value = bytes
        }
    }

    Dialog {
        id: dlDialog
        title: "Processing..."
        closePolicy: Popup.NoAutoClose
        standardButtons: Dialog.Cancel
        modal: true
        focus: true
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 20
        x: 10
        y: parent.height / 2 - height / 2
        parent: dialogParent

        ProgressBar {
            id: dlProgBar
            anchors.fill: parent
            indeterminate: false
            from: 0
            to: 1000
            value: 100
        }

        onRejected: {
            mLoader.abortDownloadUpload()
        }
    }

    function disableDialog() {
        dlDialog.open()
        column.enabled = false
    }

    function enableDialog() {
        dlDialog.close()
        column.enabled = true
    }

    Timer {
        id: statsPollTimer
        interval: Math.max(50, Math.round(1000 / Math.max(1, statsPollHz)))
        repeat: true
        running: true
        onTriggered: {
            if (VescIf.isPortConnected() &&
                    pollStatsBox.checked &&
                    statsTabItem.visible) {
                mCommands.lispGetStats(true)
            }
        }
    }

    ColumnLayout {
        id: column

        anchors.fill: parent
        anchors.leftMargin: 10 + notchLeft
        anchors.rightMargin: 10 + notchRight
        spacing: 0

        TabBar {
            id: lispTabBar
            Layout.fillWidth: true

            TabButton {
                text: "Editor"
            }

            TabButton {
                text: "REPL"
            }

            TabButton {
                text: "Stats"
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: lispTabBar.currentIndex

            // === Editor tab ===
            ColumnLayout {
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true

                    CheckBox {
                        id: autoRunBox
                        text: "Auto Run"
                        checked: true
                    }

                    CheckBox {
                        id: reduceLispBox
                        text: "Reduce"
                        checked: false
                    }

                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                        text: currentFilePath.length > 0 ? currentFilePath : "Unnamed"
                        elide: Text.ElideLeft
                    }
                }

                Label {
                    text: "Code"
                    Layout.fillWidth: true
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TextArea {
                        id: editorText
                        wrapMode: TextArea.NoWrap
                        font.family: "DejaVu Sans Mono"
                        font.pointSize: editorFontSize
                        selectByMouse: (Qt.platform.os === "ios" || Qt.platform.os === "android") ? false : true
                        persistentSelection: true
                        color: Utility.getAppHexColor("lightText")
                        inputMethodHints: Qt.ImhNoPredictiveText
                        readOnly: false
                        activeFocusOnPress: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 8

                    Button {
                        text: "Upload"
                        Layout.fillWidth: true
                        onClicked: workaroundTimerUpload.start()
                    }

                    Button {
                        text: "Read"
                        Layout.fillWidth: true
                        onClicked: workaroundTimerRead.start()
                    }

                    Button {
                        text: "Erase"
                        Layout.fillWidth: true
                        onClicked: mLoader.lispErase(16)
                    }

                    Button {
                        text: "Stream"
                        Layout.fillWidth: true
                        onClicked: workaroundTimerStream.start()
                    }

                    Button {
                        text: "..."
                        Layout.preferredWidth: 52
                        onClicked: pageMenu.open()

                        Menu {
                            id: pageMenu
                            leftPadding: notchLeft
                            rightPadding: notchRight
                            parent: lispPageItem
                            y: parent.height - implicitHeight
                            width: parent.width

                            MenuItem {
                                text: "Run"
                                onTriggered: mCommands.lispSetRunning(1)
                            }

                            MenuItem {
                                text: "Stop"
                                onTriggered: mCommands.lispSetRunning(0)
                            }

                            MenuItem {
                                text: "Open File..."
                                onTriggered: {
                                    if (Utility.requestFilePermission()) {
                                        fileDialogLoad.close()
                                        fileDialogLoad.open()
                                    } else {
                                        VescIf.emitMessageDialog(
                                                    "File Permissions",
                                                    "Unable to request file system permission.",
                                                    false, false)
                                    }
                                }
                            }

                            MenuItem {
                                text: "Open Recent..."
                                onTriggered: {
                                    updateRecentModel(recentFilter.text)
                                    recentDialog.open()
                                }
                            }

                            MenuItem {
                                text: "Open Example..."
                                onTriggered: {
                                    exampleDialog.open()
                                }
                            }

                            MenuItem {
                                text: "Close File"
                                enabled: currentFilePath.length > 0 || editorText.text.length > 0
                                onTriggered: {
                                    editorText.text = ""
                                    currentFilePath = ""
                                }
                            }

                            MenuItem {
                                text: "Text Size +"
                                onTriggered: increaseEditorFont()
                            }

                            MenuItem {
                                text: "Text Size -"
                                onTriggered: decreaseEditorFont()
                            }

                            MenuItem {
                                text: "Help"
                                onTriggered: {
                                    VescIf.emitMessageDialog(
                                                "VESC LispBM Editor",
                                                "For documentation, see:<br>" +
                                                "<a href=\"https://github.com/vedderb/bldc/blob/master/lispBM/README.md\">" +
                                                "https://github.com/vedderb/bldc/blob/master/lispBM/README.md</a>",
                                                true,
                                                true)
                                }
                            }
                        }
                    }
                }
            }

            // === REPL tab ===
            ColumnLayout {
                RowLayout {
                    Layout.fillWidth: true

                    Item {
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "REPL Console"
                        color: Utility.getAppHexColor("lightText")
                    }
                }

                ScrollView {
                    id: replScroll
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TextArea {
                        text: consoleText
                        readOnly: true
                        wrapMode: TextArea.WrapAnywhere
                        font.family: "DejaVu Sans Mono"
                        color: Utility.getAppHexColor("lightText")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        id: replInput
                        Layout.fillWidth: true
                        placeholderText: "REPL command"
                        onAccepted: {
                            if (text.length > 0) {
                                mCommands.lispSendReplCmd(text)
                                text = ""
                            }
                        }
                    }

                    Button {
                        text: "Send"
                        onClicked: {
                            if (replInput.text.length > 0) {
                                mCommands.lispSendReplCmd(replInput.text)
                                replInput.text = ""
                            }
                        }
                    }

                    Button {
                        text: "..."
                        Layout.preferredWidth: 52
                        onClicked: replMenu.open()

                        Menu {
                            id: replMenu
                            leftPadding: notchLeft
                            rightPadding: notchRight
                            parent: replScroll
                            y: parent.height - implicitHeight
                            width: parent.width

                            MenuItem {
                                text: ":help"
                                onTriggered: mCommands.lispSendReplCmd(":help")
                            }

                            MenuItem {
                                text: "Clear"
                                onTriggered: consoleText = ""
                            }
                        }
                    }
                }
            }

            // === Stats tab ===
            ColumnLayout {
                id: statsTabItem

                RowLayout {
                    Layout.fillWidth: true

                    CheckBox {
                        id: pollStatsBox
                        text: "Auto Poll"
                        checked: true
                    }

                    Label {
                        text: "Hz"
                    }

                    ComboBox {
                        id: pollHzBox
                        model: [1, 2, 5, 10]
                        currentIndex: 1
                        onCurrentTextChanged: {
                            statsPollHz = parseInt(currentText)
                        }
                    }

                    Button {
                        text: "Refresh"
                        onClicked: mCommands.lispGetStats(true)
                    }
                }

                Label {
                    text: "CPU: " + lispCpuUse.toFixed(1) + "%"
                }

                ProgressBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 10
                    from: 0
                    to: 100
                    value: lispCpuUse
                }

                Label {
                    text: "Heap: " + lispHeapUse.toFixed(1) + "%"
                }

                ProgressBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 10
                    from: 0
                    to: 100
                    value: lispHeapUse
                }

                Label {
                    text: "Memory: " + lispMemUse.toFixed(1) + "%"
                }

                ProgressBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 10
                    from: 0
                    to: 100
                    value: lispMemUse
                }

                Label {
                    text: "Global Variables"
                    Layout.fillWidth: true
                }

                ListView {
                    id: bindingListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 1
                    model: bindingModel

                    delegate: Rectangle {
                        width: bindingListView.width
                        height: 26
                        color: "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            anchors.rightMargin: 4

                            Label {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1500
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignRight
                                text: name + ": "
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1500
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignLeft
                                text: parseFloat(value).toFixed(6)
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: mCommands

        function onLispPrintReceived(str) {
            appendConsole(str)
        }

        function onLispRunningResRx(ok) {
            if (!ok) {
                VescIf.emitMessageDialog("Start/Stop LispBM",
                                         "Start/Stop LispBM failed. Did you forget to upload the code?",
                                         false, false)
            }
        }

        function onLispStatsRxMap(stats) {
            if (stats.cpu_use !== undefined) {
                lispCpuUse = stats.cpu_use
            }

            if (stats.heap_use !== undefined) {
                lispHeapUse = stats.heap_use
            }

            if (stats.mem_use !== undefined) {
                lispMemUse = stats.mem_use
            }

            bindingModel.clear()

            if (stats.globals === undefined || stats.globals === null || !Array.isArray(stats.globals)) {
                return
            }

            for (var i = 0; i < stats.globals.length; i++) {
                var g = stats.globals[i]
                if (g.name !== undefined && g.value !== undefined) {
                    bindingModel.append({"name": g.name, "value": g.value})
                }
            }
        }
    }

    Dl.FileDialog {
        id: fileDialogLoad
        title: "Open LispBM File"
        nameFilters: ["LispBM files (*.lbm *.lisp)", "All files (*)"]
        selectExisting: true
        selectMultiple: false

        onAccepted: {
            var path = fileUrlToPath(fileUrl)
            openFilePath(path)
            close()
            parent.forceActiveFocus()
        }

        onRejected: {
            close()
            parent.forceActiveFocus()
        }
    }

    Dialog {
        id: recentDialog
        modal: true
        focus: true
        title: "Recent Files"
        closePolicy: Popup.CloseOnEscape
        standardButtons: Dialog.Cancel
        Overlay.modal: Rectangle { color: "#AA000000" }
        parent: lispPageItem

        width: parent.width - 20 - notchLeft - notchRight
        height: Math.min(parent.height - 40, 450)
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        ColumnLayout {
            anchors.fill: parent

            TextField {
                id: recentFilter
                Layout.fillWidth: true
                placeholderText: "Filter"
                onTextChanged: updateRecentModel(text)
            }

            ListView {
                id: recentListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: recentModel

                delegate: ItemDelegate {
                    width: recentListView.width
                    text: path
                    onClicked: {
                        openFilePath(path)
                        recentDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: exampleDialog
        modal: true
        focus: true
        title: "Examples"
        closePolicy: Popup.CloseOnEscape
        standardButtons: Dialog.Cancel
        Overlay.modal: Rectangle { color: "#AA000000" }
        parent: lispPageItem

        width: parent.width - 20 - notchLeft - notchRight
        height: Math.min(parent.height - 40, 500)
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        ColumnLayout {
            anchors.fill: parent

            TextField {
                id: exampleFilter
                Layout.fillWidth: true
                placeholderText: "Filter"
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: FolderListModel {
                    id: exampleModel
                    folder: "qrc:/res/LispBM/Examples"
                    nameFilters: ["*.lbm", "*.lisp"]
                    showDirs: false
                    sortField: FolderListModel.Name
                }

                delegate: ItemDelegate {
                    visible: exampleFilter.text.length === 0 ||
                             fileName.toLowerCase().indexOf(exampleFilter.text.toLowerCase()) >= 0
                    height: visible ? implicitHeight : 0
                    text: fileName
                    onClicked: {
                        editorText.text = Utility.arr2str(Utility.readAllFromFile(filePath))
                        currentFilePath = ""
                        exampleDialog.close()
                    }
                }
            }
        }
    }
}
