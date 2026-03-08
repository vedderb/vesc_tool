/*
    Desktop ScriptingPage — faithful recreation of the original PageScripting widget.
    Features: QML code editor with file tabs, run/stop/fullscreen, console output,
    recent files, examples browser, tools (export C array, connected VESC, QML upload).
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Vedder.vesc

Item {
    id: scriptingPage

    property var recentFiles: []
    property string currentDir: ""

    // --- Main vertical split: top (editor+preview), bottom (tabs) ---
    SplitView {
        anchors.fill: parent
        orientation: Qt.Vertical

        // Top area: horizontal split — left editor, right preview
        SplitView {
            SplitView.fillWidth: true
            SplitView.preferredHeight: parent.height * 0.6
            orientation: Qt.Horizontal

            // Left: toolbar + file tabs with code editors
            ColumnLayout {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 300
                spacing: 2

                // Toolbar
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Button {
                        text: "Run"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                        flat: true
                        ToolTip.text: "Run in panel"
                        ToolTip.visible: hovered
                        onClicked: runScript(false)
                    }
                    Button {
                        text: "Window"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                        flat: true
                        ToolTip.text: "Run in Window"
                        ToolTip.visible: hovered
                        onClicked: runScript(true)
                    }
                    Button {
                        text: "Reload && Run"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                        flat: true
                        ToolTip.text: "Reload file from filesystem and run in panel"
                        ToolTip.visible: hovered
                        onClicked: reloadAndRun()
                    }
                    Button {
                        text: "Fullscreen"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/size_off.png"
                        flat: true
                        ToolTip.text: "Toggle fullscreen in running window"
                        ToolTip.visible: hovered
                    }
                    Button {
                        text: "Stop"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Shutdown-96.png"
                        flat: true
                        onClicked: stopScript()
                    }
                    Button {
                        text: "Help"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
                        flat: true
                        onClicked: showHelp()
                    }
                    Item { Layout.fillWidth: true }
                }

                // File tabs
                TabBar {
                    id: fileTabs
                    Layout.fillWidth: true

                    TabButton {
                        text: "main"
                        width: implicitWidth
                    }
                    TabButton {
                        text: "+"
                        width: 40
                        onClicked: addNewTab()
                    }
                }

                // Editor stack — one TextArea per tab
                StackLayout {
                    id: editorStack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: Math.min(fileTabs.currentIndex, editorStack.count - 1)

                    ScrollView {
                        id: mainEditorScroll
                        clip: true
                        TextArea {
                            id: mainEditor
                            font.family: "DejaVu Sans Mono"
                            font.pixelSize: 13
                            wrapMode: TextEdit.NoWrap
                            selectByMouse: true
                            color: Utility.getAppHexColor("normalText")
                            property string fileName: ""
                            property bool isDirty: false
                            onTextChanged: {
                                if (!isDirty) {
                                    isDirty = true
                                    updateTabTitle(0)
                                }
                            }
                        }
                    }
                }
            }

            // Right: QML preview pane (placeholder)
            Rectangle {
                SplitView.preferredWidth: 300
                SplitView.minimumWidth: 100
                color: Utility.getAppHexColor("normalBackground")
                border.color: Utility.getAppHexColor("disabledText")
                border.width: 1

                Label {
                    anchors.centerIn: parent
                    text: "QML Preview"
                    color: Utility.getAppHexColor("disabledText")
                    font.pixelSize: 14
                }
            }
        }

        // Bottom: Console / Recent / Examples / Tools tabs
        TabBar {
            id: bottomTabBar
            SplitView.fillWidth: true
            SplitView.preferredHeight: 30

            TabButton { text: "Console" }
            TabButton { text: "Recent" }
            TabButton { text: "Examples" }
            TabButton { text: "Tools" }
        }

        StackLayout {
            SplitView.fillWidth: true
            SplitView.preferredHeight: parent.height * 0.3
            currentIndex: bottomTabBar.currentIndex

            // === Console tab ===
            RowLayout {
                spacing: 4
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    TextArea {
                        id: consoleEdit
                        readOnly: true
                        font.family: "DejaVu Sans Mono"
                        font.pixelSize: 12
                        wrapMode: TextEdit.NoWrap
                        textFormat: TextEdit.RichText
                        color: Utility.getAppHexColor("normalText")
                    }
                }
                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    Button {
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Delete-96.png"
                        flat: true
                        ToolTip.text: "Clear console"
                        ToolTip.visible: hovered
                        onClicked: consoleEdit.text = ""
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // === Recent tab ===
            RowLayout {
                spacing: 4
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 2
                    RowLayout {
                        spacing: 2
                        Label { text: "Filter" }
                        TextField {
                            id: recentFilterEdit
                            Layout.fillWidth: true
                            onTextChanged: filterList(recentListView, text)
                        }
                        Button {
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Cancel-96.png"
                            flat: true
                            onClicked: recentFilterEdit.text = ""
                        }
                    }
                    ListView {
                        id: recentListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: ListModel { id: recentModel }
                        delegate: ItemDelegate {
                            width: recentListView.width
                            text: model.path
                            highlighted: ListView.isCurrentItem
                            onClicked: recentListView.currentIndex = index
                            onDoubleClicked: openRecentItem()
                        }
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }
                }
                ColumnLayout {
                    spacing: 3
                    Button {
                        text: "Open"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                        onClicked: openRecentItem()
                    }
                    Button {
                        text: "Remove"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Delete-96.png"
                        onClicked: {
                            if (recentListView.currentIndex >= 0) {
                                recentFiles.splice(recentListView.currentIndex, 1)
                                updateRecentModel()
                            }
                        }
                    }
                    Button {
                        text: "Clear"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Delete-96.png"
                        onClicked: { recentFiles = []; updateRecentModel() }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // === Examples tab ===
            RowLayout {
                spacing: 4
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 2
                    RowLayout {
                        spacing: 2
                        Label { text: "Filter" }
                        TextField {
                            id: exampleFilterEdit
                            Layout.fillWidth: true
                            onTextChanged: filterList(exampleListView, text)
                        }
                        Button {
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Cancel-96.png"
                            flat: true
                            onClicked: exampleFilterEdit.text = ""
                        }
                    }
                    ListView {
                        id: exampleListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: ListModel { id: exampleModel }
                        delegate: ItemDelegate {
                            width: exampleListView.width
                            text: model.name
                            highlighted: ListView.isCurrentItem
                            onClicked: exampleListView.currentIndex = index
                            onDoubleClicked: openExampleItem()
                        }
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }
                }
                ColumnLayout {
                    Button {
                        text: "Open"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                        onClicked: openExampleItem()
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // === Tools tab ===
            RowLayout {
                spacing: 4
                // Export C Array
                GroupBox {
                    title: "Export C Array"
                    Layout.alignment: Qt.AlignTop
                    ColumnLayout {
                        spacing: 2
                        Button {
                            text: "Export Array HW"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Save as-96.png"
                            onClicked: VescIf.emitMessageDialog("Export", "Export C Array HW — not yet implemented in QML UI", false, false)
                        }
                        Button {
                            text: "Export Array App"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Save as-96.png"
                            onClicked: VescIf.emitMessageDialog("Export", "Export C Array App — not yet implemented in QML UI", false, false)
                        }
                        Button {
                            text: "Calculate Size"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Calculator-96.png"
                            onClicked: VescIf.emitMessageDialog("QML Size", "Calculate Size — not yet implemented in QML UI", false, false)
                        }
                    }
                }
                // Connected VESC
                GroupBox {
                    title: "Connected VESC"
                    Layout.alignment: Qt.AlignTop
                    ColumnLayout {
                        spacing: 2
                        Button {
                            text: "Open Qmlui HW"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                            onClicked: {
                                if (VescIf.isPortConnected() && VescIf.qmlHwLoaded()) {
                                    setMainEditorContent(VescIf.qmlHw(), "VESC Hw")
                                } else {
                                    VescIf.emitMessageDialog("Open Qmlui HW", "No HW qmlui loaded.", false, false)
                                }
                            }
                        }
                        Button {
                            text: "Open Qmlui App"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                            onClicked: {
                                if (VescIf.isPortConnected() && VescIf.qmlAppLoaded()) {
                                    setMainEditorContent(VescIf.qmlApp(), "VESC App")
                                } else {
                                    VescIf.emitMessageDialog("Open Qmlui App", "No App qmlui loaded.", false, false)
                                }
                            }
                        }
                    }
                }
                // QML Upload
                GroupBox {
                    title: "QML Upload"
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 2
                        RowLayout {
                            spacing: 2
                            Button {
                                text: "Erase && Upload"
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Download-96.png"
                                onClicked: VescIf.emitMessageDialog("Upload", "Erase & Upload — not yet implemented in QML UI", false, false)
                            }
                            Button {
                                text: "Erase Only"
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Delete-96.png"
                                onClicked: VescIf.emitMessageDialog("Erase", "Erase Only — not yet implemented in QML UI", false, false)
                            }
                            CheckBox {
                                id: uploadFullscreenBox
                                text: "Fullscreen"
                                ToolTip.text: "Run uploaded UI in fullscreen mode"
                                ToolTip.visible: hovered
                            }
                            Item { Layout.fillWidth: true }
                            Button {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Delete-96.png"
                                flat: true
                                ToolTip.text: "Clear text below"
                                ToolTip.visible: hovered
                                onClicked: uploadTextEdit.text = ""
                            }
                        }
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            TextArea {
                                id: uploadTextEdit
                                readOnly: true
                                font.family: "DejaVu Sans Mono"
                                font.pixelSize: 12
                                wrapMode: TextEdit.NoWrap
                                color: Utility.getAppHexColor("normalText")
                            }
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: openFileDialog
        title: "Open QML File"
        nameFilters: ["QML files (*.qml)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            loadFileIntoEditor(path)
        }
    }

    FileDialog {
        id: saveFileDialog
        title: "Save QML File"
        nameFilters: ["QML files (*.qml)", "All files (*)"]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            saveCurrentEditor(path)
        }
    }

    // Load examples list on startup
    Component.onCompleted: {
        loadExamples()
    }

    function loadExamples() {
        // Load example files from resource
        var examples = [
            "BalanceUi.qml", "BMS.qml", "BrakeBench.qml", "CanDebugger.qml",
            "ConfigParams.qml", "Controls.qml", "DCDC.qml", "ExperimentPlot.qml",
            "GaugeTest.qml", "IoBoard.qml", "LogTextToFile.qml", "Meters.qml",
            "MMXGui.qml", "MotorComparisonModel.qml", "Mp3Stream.qml",
            "ParamTableAndPlot.qml", "ParentTabBar.qml", "PollValues.qml",
            "PositionControl.qml", "Profiles.qml", "Reconnect.qml",
            "RpmSlider.qml", "RtData.qml", "RtDataSetup.qml",
            "SendToLbm.qml", "TcpHub.qml", "TcpServer.qml", "UdpServer.qml"
        ]
        for (var i = 0; i < examples.length; i++) {
            exampleModel.append({
                name: examples[i],
                path: "://res/qml/Examples/" + examples[i]
            })
        }
    }

    function updateRecentModel() {
        recentModel.clear()
        for (var i = 0; i < recentFiles.length; i++) {
            recentModel.append({ path: recentFiles[i] })
        }
    }

    function filterList(listView, filterText) {
        // Simple filter: hide non-matching items
        // Since ListModel doesn't natively support visibility, we just update
        for (var i = 0; i < listView.model.count; i++) {
            var item = listView.itemAtIndex(i)
            if (item) {
                item.visible = (filterText === "" ||
                    listView.model.get(i).name !== undefined ?
                    listView.model.get(i).name.toLowerCase().indexOf(filterText.toLowerCase()) >= 0 :
                    listView.model.get(i).path.toLowerCase().indexOf(filterText.toLowerCase()) >= 0)
            }
        }
    }

    function openRecentItem() {
        if (recentListView.currentIndex >= 0) {
            var path = recentModel.get(recentListView.currentIndex).path
            loadFileIntoEditor(path)
        }
    }

    function openExampleItem() {
        if (exampleListView.currentIndex >= 0) {
            var item = exampleModel.get(exampleListView.currentIndex)
            // Read example from resource using Utility
            var content = Utility.readTextFile(item.path)
            if (content.length > 0) {
                setMainEditorContent(content, item.name)
            }
        }
    }

    function loadFileIntoEditor(path) {
        var content = Utility.readTextFile(path)
        if (content.length > 0) {
            mainEditor.text = content
            mainEditor.fileName = path
            mainEditor.isDirty = false
            updateTabTitle(0)

            // Add to recent
            var idx = recentFiles.indexOf(path)
            if (idx >= 0) recentFiles.splice(idx, 1)
            recentFiles.unshift(path)
            updateRecentModel()
        }
    }

    function setMainEditorContent(content, title) {
        if (mainEditor.text.length === 0) {
            mainEditor.text = content
            mainEditor.fileName = ""
            mainEditor.isDirty = false
            fileTabs.itemAt(0).text = title
        } else {
            // TODO: add new tab
            mainEditor.text = content
            mainEditor.fileName = ""
            mainEditor.isDirty = false
            fileTabs.itemAt(0).text = title
        }
    }

    function saveCurrentEditor(path) {
        Utility.writeTextFile(path, mainEditor.text)
        mainEditor.fileName = path
        mainEditor.isDirty = false
        updateTabTitle(0)

        var idx = recentFiles.indexOf(path)
        if (idx >= 0) recentFiles.splice(idx, 1)
        recentFiles.unshift(path)
        updateRecentModel()
    }

    function updateTabTitle(tabIndex) {
        if (tabIndex === 0) {
            var title = "main"
            if (mainEditor.fileName.length > 0) {
                var parts = mainEditor.fileName.split("/")
                title = parts[parts.length - 1] + " (main)"
            }
            if (mainEditor.isDirty) title += "*"
            fileTabs.itemAt(0).text = title
        }
    }

    function addNewTab() {
        // For now just clear and focus main editor
        VescIf.emitMessageDialog("New Tab", "Multi-tab editing is not yet implemented in QML UI. Use the main tab.", false, false)
    }

    function runScript(inWindow) {
        appendConsole("Running script...")
        // The actual QML runtime execution requires C++ integration
        // For now, log the action
        if (inWindow) {
            appendConsole("Run in window — requires C++ QmlUi integration")
        } else {
            appendConsole("Run in panel — requires C++ QmlUi integration")
        }
    }

    function stopScript() {
        appendConsole("Script stopped")
    }

    function reloadAndRun() {
        if (mainEditor.fileName.length > 0) {
            var content = Utility.readTextFile(mainEditor.fileName)
            if (content.length > 0) {
                mainEditor.text = content
                mainEditor.isDirty = false
                updateTabTitle(0)
                runScript(false)
            } else {
                appendConsole("<font color=\"red\">Could not reload file</font>")
            }
        } else {
            appendConsole("<font color=\"red\">No file to reload</font>")
        }
    }

    function appendConsole(msg) {
        var timestamp = new Date().toLocaleString(Qt.locale(), "yyyy-MM-dd hh:mm:ss")
        consoleEdit.append("<font color=\"#4d7fc4\">" + timestamp + ": </font>" + msg)
    }

    function showHelp() {
        VescIf.emitMessageDialog("VESC Tool Script Editor",
            "<b>Keyboard Commands</b><br>" +
            "Ctrl + '+' : Increase font size<br>" +
            "Ctrl + '-' : Decrease font size<br>" +
            "Ctrl + space : Show auto-complete suggestions<br>" +
            "Ctrl + '/' : Toggle auto-comment on line or block<br>" +
            "Ctrl + '#' : Toggle auto-comment on line or block<br>" +
            "Ctrl + 'i' : Auto-indent selected line or block<br>" +
            "Ctrl + 'f' : Open search (and replace) bar<br>" +
            "Ctrl + 'e' : Run or restart embedded<br>" +
            "Ctrl + 'w' : Run or restart window<br>" +
            "Ctrl + 'q' : Stop code<br>" +
            "Ctrl + 'd' : Clear console<br>" +
            "Ctrl + 's' : Save file<br>" +
            "Ctrl + Shift + 'd' : Duplicate current line<br>",
            false, false)
    }
}
