import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.folderlistmodel 2.1
import QtCore

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0
import Vedder.vesc.codeloader 1.0
import Vedder.vesc.logwriter 1.0

Item {
    id: lispPageItem

    property var dialogParent: Overlay.overlay
    property Commands mCommands: VescIf.commands()
    property string currentFilePath: ""
    property string consoleText: ""
    property real lispCpuUse: 0
    property real lispHeapUse: 0
    property real lispMemUse: 0
    property int statsPollHz: 2
    property int editorFontSize: 10

    // Same box as the Terminal output
    component TextBoxBackground: Rectangle {
        radius: 12
        color: Utility.getAppHexColor("darkBackground")
        border.color: Utility.getAppHexColor("raisedBackground")
    }

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

    Settings {
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

    BusyDialog {
        id: dlDialog
        title: "Processing..."
        spinner: false
        cancelText: qsTr("Cancel")

        ProgressBar {
            id: dlProgBar
            Layout.fillWidth: true
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
        anchors.leftMargin: 10
        anchors.rightMargin: 10
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

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Screen, so that the keyboard does not change it
            columns: Screen.width > Screen.height ? 2 : 1
            columnSpacing: 10

            // === Editor tab ===
            GridLayout {
                id: editorTab
                // On phones in landscape the options and the buttons share the top row
                readonly property bool oneRow: phoneLandscape
                visible: mainIsTablet || lispTabBar.currentIndex === 0
                Layout.preferredWidth: parent.width * 0.6
                Layout.preferredHeight: parent.height * 0.6
                flow: GridLayout.TopToBottom
                rows: oneRow ? 2 : 3
                rowSpacing: 4

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
                        // In one row a long path must not squeeze the buttons
                        Layout.preferredWidth: editorTab.oneRow ? 0 : -1
                        horizontalAlignment: Text.AlignRight
                        text: currentFilePath.length > 0 ? currentFilePath : "Unnamed"
                        elide: Text.ElideLeft
                    }
                }

                // At least as wide as the view, so short text leaves sideways swipes to the page swiper
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.columnSpan: editorTab.oneRow ? 2 : 1
                    Layout.preferredWidth: editorTab.oneRow ? 0 : -1
                    contentWidth: Math.max(editorText.implicitWidth, availableWidth)
                    clip: true
                    padding: 4
                    background: TextBoxBackground {}

                    TextArea {
                        id: editorText
                        background: null
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
                    Layout.topMargin: editorTab.oneRow ? 0 : 8

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

                        BottomMenu {
                            id: pageMenu

                            MenuEntry {
                                text: "Run"
                                iconFile: "Circled Play-96.png"
                                onTriggered: mCommands.lispSetRunning(1)
                            }

                            MenuEntry {
                                text: "Stop"
                                iconFile: "Stop-96.png"
                                onTriggered: mCommands.lispSetRunning(0)
                            }

                            MenuEntry {
                                text: "Open File..."
                                iconFile: "Open Folder-96.png"
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

                            MenuEntry {
                                text: "Save File as..."
                                iconFile: "Save as-96.png"
                                onTriggered: {
                                    if (Utility.requestFilePermission()) {
                                        fileDialogSave.close()
                                        fileDialogSave.open()
                                    } else {
                                        VescIf.emitMessageDialog(
                                                    "File Permissions",
                                                    "Unable to request file system permission.",
                                                    false, false)
                                    }
                                }

                                LogWriter {
                                    id: mLogWriter
                                }

                                FileDialog {
                                    id: fileDialogSave
                                    title: "Please choose a file"
                                    fileMode: FileDialog.SaveFile
                                    nameFilters: ["LBM files (*.lbm *.lisp)"]

                                    onAccepted: {
                                        var path = selectedFile.toString()

                                        var okOpen = mLogWriter.openLogFileFullPath(path)
                                        var okWrite = mLogWriter.writeToLogFile(editorText.text)
                                        mLogWriter.closeLogFile()

                                        if (okOpen && okWrite) {
                                            VescIf.emitStatusMessage("LBM-file Saved", true)
                                        } else {
                                            VescIf.emitStatusMessage("LBM-file Save Failed", false)
                                        }

                                        close()
                                        parent.forceActiveFocus()
                                    }

                                    onRejected: {
                                        close()
                                        parent.forceActiveFocus()
                                    }
                                }
                            }

                            MenuEntry {
                                text: "Open Recent..."
                                iconFile: "History-96.png"
                                onTriggered: {
                                    updateRecentModel(recentFilter.displayText)
                                    recentDialog.open()
                                }
                            }

                            MenuEntry {
                                text: "Open Example..."
                                iconFile: "Console-96.png"
                                onTriggered: {
                                    exampleDialog.open()
                                }
                            }

                            MenuEntry {
                                text: "Close File"
                                iconFile: "Cancel-96.png"
                                enabled: currentFilePath.length > 0 || editorText.text.length > 0
                                onTriggered: {
                                    editorText.text = ""
                                    currentFilePath = ""
                                }
                            }

                            MenuEntry {
                                text: "Text Size +"
                                iconFile: "Text Larger-96.png"
                                onTriggered: increaseEditorFont()
                            }

                            MenuEntry {
                                text: "Text Size -"
                                iconFile: "Text Smaller-96.png"
                                onTriggered: decreaseEditorFont()
                            }

                            MenuEntry {
                                text: "Help"
                                iconFile: "Help-96.png"
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
                visible: lispTabBar.currentIndex === 1
                Layout.preferredWidth: parent.width * 0.4
                Layout.preferredHeight: parent.height * 0.4
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: Math.max(replText.implicitWidth, availableWidth)
                    clip: true
                    padding: 4
                    background: TextBoxBackground {}

                    TextArea {
                        id: replText
                        background: null
                        text: consoleText
                        readOnly: true
                        wrapMode: TextArea.NoWrap
                        // Keeps the newest output in view
                        onTextChanged: cursorPosition = length
                        selectByMouse: Qt.platform.os !== "ios" && Qt.platform.os !== "android"
                        font.family: "DejaVu Sans Mono"
                        color: Utility.getAppHexColor("lightText")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    InputField {
                        id: replInput
                        Layout.fillWidth: true
                        Layout.topMargin: 5
                        Layout.bottomMargin: 5
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
                        Layout.preferredWidth: 60
                        onClicked: replMenu.open()

                        BottomMenu {
                            id: replMenu

                            MenuEntry {
                                text: ":help"
                                iconFile: "Help-96.png"
                                onTriggered: mCommands.lispSendReplCmd(":help")
                            }

                            MenuEntry {
                                text: "Clear"
                                iconFile: "Cancel-96.png"
                                onTriggered: consoleText = ""
                            }
                        }
                    }
                }
            }

            // === Stats tab ===
            ColumnLayout {
                id: statsTabItem
                visible: lispTabBar.currentIndex === 2
                Layout.preferredWidth: parent.width * 0.4
                Layout.preferredHeight: parent.height * 0.4

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
                        background: InputBackground {}
                        Layout.fillWidth: true
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

        function syncBindings(newData) {
            // 1. Build a lookup map of incoming data by name
            var incomingMap = {};
            for (var i = 0; i < newData.length; i++) {
                incomingMap[newData[i].name] = newData[i].value;
            }

            // 2. Update existing items & remove deleted ones (iterate backwards when removing)
            for (var j = bindingModel.count - 1; j >= 0; j--) {
                var item = bindingModel.get(j);
                if (incomingMap.hasOwnProperty(item.name)) {
                    if (item.value !== incomingMap[item.name]) {
                        bindingModel.setProperty(j, "value", incomingMap[item.name]);
                    }
                    delete incomingMap[item.name]; // Mark as handled
                } else {
                    bindingModel.remove(j, 1);
                }
            }

            // 3. Append any brand-new items left in the map
            for (var k = 0; k < newData.length; k++) {
                if (incomingMap.hasOwnProperty(newData[k].name)) {
                    bindingModel.append({ "name": newData[k].name, "value": newData[k].value });
                }
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

            if (stats.globals === undefined || stats.globals === null) {
                return
            }

            syncBindings(stats.globals)
        }
    }

    FileDialog {
        id: fileDialogLoad
        title: "Open LispBM File"
        fileMode: FileDialog.OpenFile
        nameFilters: ["LispBM files (*.lbm *.lisp)", "All files (*)"]

        onAccepted: {
            var path = fileUrlToPath(selectedFile)
            openFilePath(path)
            close()
            parent.forceActiveFocus()
        }

        onRejected: {
            close()
            parent.forceActiveFocus()
        }
    }

    SheetDialog {
        id: recentDialog
        title: "Recent Files"

        fullHeight: true

        ColumnLayout {
            anchors.fill: parent

            InputField {
                id: recentFilter
                Layout.fillWidth: true
                placeholderText: "Filter"
                onDisplayTextChanged: updateRecentModel(displayText)
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

    SheetDialog {
        id: exampleDialog
        title: "Examples"
        fullHeight: true

        ColumnLayout {
            anchors.fill: parent

            InputField {
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
                    visible: exampleFilter.displayText.length === 0 ||
                             fileName.toLowerCase().indexOf(exampleFilter.displayText.toLowerCase()) >= 0
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
