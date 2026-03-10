/*
    Desktop PackagesPage — faithful replica of PageVescPackage (widgets).
    Tabs: "Package Store", "Load Custom", "Create Package"
    Below tabs: DisplayPercentage-style download bar + Update Archive + Uninstall Current
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Vedder.vesc

Item {
    id: root

    // ---- CodeLoader (QML_ELEMENT) ----
    CodeLoader {
        id: mLoader
        Component.onCompleted: {
            mLoader.setVesc(VescIf)
        }
    }

    // ---- State properties ----
    property var applicationModel: []     // [{name, pkg}]
    property var libraryModel: []         // [{name, pkg}]
    property int appCurrentIndex: -1
    property int libCurrentIndex: -1
    property var currentPkg: null         // currently selected VescPackage
    property bool downloadInProgress: false

    // DisplayPercentage-style bar state
    property string dlText: ""
    property real dlValue: 0.0

    // Create Package tab state
    property bool descriptionUpdated: true

    // ---- Settings persistence ----
    property string settLastPkgLoad: ""
    property string settLastLisp: ""
    property string settLastQml: ""
    property string settLastOutput: ""

    // ---- Connections to VescIf ----
    Connections {
        target: VescIf

        function onPortConnectedChanged() {
            if (!VescIf.isPortConnected()) {
                reloadArchive()
            }
        }

        function onCustomConfigLoadDone() {
            reloadArchive()
        }
    }

    // ---- Connections to CodeLoader download progress ----
    Connections {
        target: mLoader

        function onDownloadProgress(bytesReceived, bytesTotal) {
            dlText = "Downloading..."
            dlValue = 100.0 * bytesReceived / bytesTotal
        }
    }

    // ---- Markdown preview timer (Create Package tab) ----
    Timer {
        id: previewTimer
        interval: 500
        repeat: true
        running: true
        onTriggered: {
            if (descriptionUpdated) {
                descriptionUpdated = false
                var flick = descriptionScrollView.contentItem
                var scrollPos = flick ? flick.contentY : 0
                descriptionBrowser.text = Utility.md2html(descriptionEdit.text)
                if (flick) flick.contentY = scrollPos
            }
        }
    }

    // ---- File Dialogs ----
    FileDialog {
        id: loadFileDialog
        title: "Choose Package File"
        nameFilters: ["VESC Package Files (*.vescpkg)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            settLastPkgLoad = path
            loadEdit.text = path
            refreshLoad()
        }
    }

    FileDialog {
        id: lispFileDialog
        title: "Choose LispBM File"
        nameFilters: ["LispBM files (*.lbm *.lisp)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            settLastLisp = path
            lispEdit.text = path
        }
    }

    FileDialog {
        id: qmlFileDialog
        title: "Choose Qml File"
        nameFilters: ["Qml files (*.qml)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            settLastQml = path
            qmlEdit.text = path
        }
    }

    FileDialog {
        id: outputFileDialog
        title: "Choose Package Output File"
        nameFilters: ["VESC Package Files (*.vescpkg)"]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (!path.toLowerCase().endsWith(".vescpkg")) {
                path += ".vescpkg"
            }

            if (Utility.fileExists(path)) {
                if (descriptionEdit.text.length > 10) {
                    replaceContentDialog.pendingPath = path
                    replaceContentDialog.open()
                } else {
                    settLastOutput = path
                    outputEdit.text = path
                    refreshOutput()
                }
            } else {
                settLastOutput = path
                outputEdit.text = path
                savePackage()
            }
        }
    }

    // Replace content confirmation dialog
    Dialog {
        id: replaceContentDialog
        title: "Replace Content"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        property string pendingPath: ""

        Label {
            text: "Opening an existing package will replace the content " +
                  "in the editor. Do you want to continue?"
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            settLastOutput = pendingPath
            outputEdit.text = pendingPath
            refreshOutput()
        }
    }

    // Incompatible package warning dialog (Load Custom tab)
    Dialog {
        id: incompatibleDialog
        title: "Incompatible Package"
        modal: true
        standardButtons: Dialog.Yes | Dialog.Cancel
        anchors.centerIn: parent
        property var pendingPkg: null

        Label {
            text: "The selected package reports that it is not compatible with " +
                  "the connected device. Do you want to install it anyway?"
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            if (pendingPkg) {
                Qt.callLater(function() {
                    mLoader.installVescPackage(pendingPkg.compressedData)
                })
            }
        }
    }

    // ---- Archive reload ----
    function reloadArchive() {
        var pList = mLoader.reloadPackageArchive()
        var apps = []
        var libs = []

        for (var i = 0; i < pList.length; i++) {
            var p = pList[i]
            if (p.isLibrary) {
                libs.push({name: p.name, pkg: p})
            } else {
                if (mLoader.shouldShowPackage(p)) {
                    apps.push({name: p.name, pkg: p})
                }
            }
        }

        applicationModel = apps
        libraryModel = libs
        appCurrentIndex = -1
        libCurrentIndex = -1
        currentPkg = null
        storeBrowser.text = ""
    }

    // ---- Package selected ----
    function packageSelected(pkg) {
        currentPkg = pkg

        var desc = pkg.description
        var line1 = desc.substring(0, desc.indexOf("\n"))
        if (line1.toUpperCase().indexOf("<!DOCTYPE HTML PUBLIC") >= 0) {
            storeBrowser.text = desc
        } else {
            storeBrowser.text = Utility.md2html(desc)
        }

        installButton.enabled = !pkg.isLibrary
        if (installButton.enabled) {
            installButton.ToolTip.text = ""
        } else {
            installButton.ToolTip.text = "This is a library, so it is not supposed to be installed. You can use " +
                                          "it from your own LispBM scripts without installing it."
        }
    }

    // ---- Load package from file (Load Custom tab) ----
    function refreshLoad() {
        if (loadEdit.text === "") return
        if (!Utility.fileExists(loadEdit.text)) return

        var pkg = mLoader.unpackVescPackageFromPath(loadEdit.text)
        if (!pkg.loadOk) {
            VescIf.emitMessageDialog("Open Package", "Package is not valid.", false, false)
            return
        }

        var desc = pkg.description
        var line1 = desc.substring(0, desc.indexOf("\n"))
        if (line1.toUpperCase().indexOf("<!DOCTYPE HTML PUBLIC") >= 0) {
            loadBrowser.text = desc
        } else {
            loadBrowser.text = Utility.md2html(desc)
        }
    }

    // ---- Refresh output (Create Package tab) ----
    function refreshOutput() {
        if (outputEdit.text === "") return
        if (!Utility.fileExists(outputEdit.text)) return

        var pkg = mLoader.unpackVescPackageFromPath(outputEdit.text)
        if (!pkg.loadOk) {
            VescIf.emitMessageDialog("Open Package", "Package is not valid.", false, false)
            return
        }

        if (pkg.description_md.length > 0) {
            descriptionEdit.text = pkg.description_md
        } else {
            var desc = pkg.description
            var line1 = desc.substring(0, desc.indexOf("\n"))
            if (line1.toUpperCase().indexOf("<!DOCTYPE HTML PUBLIC") >= 0) {
                descriptionEdit.text = desc
            } else {
                descriptionEdit.text = desc
            }
        }

        descriptionUpdated = true
        nameEdit.text = pkg.name
    }

    // ---- Save package (Create Package tab) ----
    function savePackage() {
        if (outputEdit.text === "") {
            outputFileDialog.open()
            return
        }

        // Note: packVescPackage and lispPackImports are not Q_INVOKABLE,
        // so full Create Package save requires C++ side support.
        // For now we report this limitation.
        VescIf.emitMessageDialog("Save Package",
                                 "Package creation/saving requires the C++ backend. " +
                                 "Use the packVescPackage API from a C++ context.",
                                 false, false)
    }

    // ---- Main layout ----
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton { text: "Package Store"; topPadding: 9; bottomPadding: 9 }
            TabButton { text: "Load Custom"; topPadding: 9; bottomPadding: 9 }
            TabButton { text: "Create Package"; topPadding: 9; bottomPadding: 9 }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // ==========================================
            // Tab 0: Package Store
            // ==========================================
            Item {
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 0

                    // Left side: Applications and Libraries lists
                    ColumnLayout {
                        Layout.preferredWidth: parent.width * 0.35
                        Layout.fillHeight: true
                        spacing: 2

                        // Applications section
                        Label {
                            text: "Applications"
                            font.bold: true
                        }

                        ListView {
                            id: applicationList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: applicationModel
                            currentIndex: appCurrentIndex
                            highlight: Rectangle {
                                color: Utility.getAppHexColor("lightAccent")
                                opacity: 0.3
                            }
                            highlightFollowsCurrentItem: true

                            delegate: ItemDelegate {
                                width: applicationList.width
                                text: modelData.name
                                highlighted: applicationList.currentIndex === index
                                onClicked: {
                                    appCurrentIndex = index
                                    libCurrentIndex = -1
                                    libraryList.currentIndex = -1
                                    packageSelected(modelData.pkg)
                                }
                            }

                            ScrollBar.vertical: ScrollBar {}
                        }

                        // Libraries section
                        Label {
                            text: "Libraries"
                            font.bold: true
                        }

                        ListView {
                            id: libraryList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: libraryModel
                            currentIndex: libCurrentIndex
                            highlight: Rectangle {
                                color: Utility.getAppHexColor("lightAccent")
                                opacity: 0.3
                            }
                            highlightFollowsCurrentItem: true

                            delegate: ItemDelegate {
                                width: libraryList.width
                                text: modelData.name
                                highlighted: libraryList.currentIndex === index
                                onClicked: {
                                    libCurrentIndex = index
                                    appCurrentIndex = -1
                                    applicationList.currentIndex = -1
                                    packageSelected(modelData.pkg)
                                }
                            }

                            ScrollBar.vertical: ScrollBar {}
                        }
                    }

                    // Right side: description browser + Install button
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        // Store browser (read-only HTML display)
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                id: storeBrowser
                                readOnly: true
                                textFormat: TextArea.RichText
                                wrapMode: TextArea.Wrap
                                onLinkActivated: function(link) {
                                    Qt.openUrlExternally(link)
                                }
                            }
                        }

                        Button {
                            id: installButton
                            text: "Install"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Download-96.png"
                            Layout.fillWidth: true
                            ToolTip.visible: hovered && ToolTip.text !== ""
                            ToolTip.text: "Uninstall current package and install the selected package to the connected VESC"

                            onClicked: {
                                if (!VescIf.isPortConnected()) {
                                    VescIf.emitMessageDialog("Install Package", "Not Connected", false, false)
                                    return
                                }

                                if (currentPkg && currentPkg.loadOk) {
                                    Qt.callLater(function() {
                                        mLoader.installVescPackage(currentPkg.compressedData)
                                        VescIf.emitMessageDialog("Install Package",
                                                                 "Installation Done!",
                                                                 true, false)
                                    })
                                } else {
                                    VescIf.emitMessageDialog("Install Package",
                                                             "No package selected.",
                                                             false, false)
                                }
                            }
                        }
                    }
                }
            }

            // ==========================================
            // Tab 1: Load Custom
            // ==========================================
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    // Load Package group box
                    GroupBox {
                        title: "Load Package"
                        Layout.fillWidth: true

                        RowLayout {
                            anchors.fill: parent
                            spacing: 2

                            TextField {
                                id: loadEdit
                                Layout.fillWidth: true
                                text: settLastPkgLoad
                            }

                            Button {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                                ToolTip.visible: hovered
                                ToolTip.text: "Choose file..."
                                implicitWidth: implicitHeight
                                onClicked: loadFileDialog.open()
                            }

                            Button {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Refresh-96.png"
                                implicitWidth: implicitHeight
                                onClicked: refreshLoad()
                            }

                            Button {
                                text: "Install"
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Download-96.png"
                                ToolTip.visible: hovered
                                ToolTip.text: "Uninstall current package and install the selected package to the connected VESC"
                                onClicked: {
                                    if (!VescIf.isPortConnected()) {
                                        VescIf.emitMessageDialog("Write Package", "Not Connected", false, false)
                                        return
                                    }

                                    var pkg = mLoader.unpackVescPackageFromPath(loadEdit.text)
                                    if (!pkg.loadOk) {
                                        return
                                    }

                                    if (mLoader.shouldShowPackage(pkg)) {
                                        Qt.callLater(function() {
                                            mLoader.installVescPackage(pkg.compressedData)
                                        })
                                    } else {
                                        incompatibleDialog.pendingPkg = pkg
                                        incompatibleDialog.open()
                                    }
                                }
                            }
                        }
                    }

                    // Load browser (read-only HTML display)
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        TextArea {
                            id: loadBrowser
                            readOnly: true
                            textFormat: TextArea.RichText
                            wrapMode: TextArea.Wrap
                            onLinkActivated: function(link) {
                                Qt.openUrlExternally(link)
                            }
                        }
                    }
                }
            }

            // ==========================================
            // Tab 2: Create Package
            // ==========================================
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    // LispBM and Qml group boxes side by side
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        // LispBM group (checkable)
                        GroupBox {
                            id: lispBox
                            property bool checked: true
                            Layout.fillWidth: true

                            label: CheckBox {
                                id: lispBoxCheck
                                checked: lispBox.checked
                                text: "LispBM"
                                onCheckedChanged: lispBox.checked = checked
                            }

                            RowLayout {
                                anchors.fill: parent
                                spacing: 2
                                enabled: lispBox.checked
                                opacity: lispBox.checked ? 1.0 : 0.5

                                TextField {
                                    id: lispEdit
                                    Layout.fillWidth: true
                                    text: settLastLisp
                                }

                                Button {
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Choose file..."
                                    implicitWidth: implicitHeight
                                    onClicked: lispFileDialog.open()
                                }
                            }
                        }

                        // Qml group (checkable)
                        GroupBox {
                            id: qmlBox
                            property bool checked: true
                            Layout.fillWidth: true

                            label: CheckBox {
                                id: qmlBoxCheck
                                checked: qmlBox.checked
                                text: "Qml"
                                onCheckedChanged: qmlBox.checked = checked
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 2
                                enabled: qmlBox.checked
                                opacity: qmlBox.checked ? 1.0 : 0.5

                                RowLayout {
                                    spacing: 2

                                    TextField {
                                        id: qmlEdit
                                        Layout.fillWidth: true
                                        text: settLastQml
                                    }

                                    Button {
                                        icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                                        ToolTip.visible: hovered
                                        ToolTip.text: "Choose file..."
                                        implicitWidth: implicitHeight
                                        onClicked: qmlFileDialog.open()
                                    }
                                }

                                CheckBox {
                                    id: qmlFullscreenBox
                                    text: "Fullscreen"
                                }
                            }
                        }
                    }

                    // Package Name
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: "Package Name"
                        }

                        TextField {
                            id: nameEdit
                            Layout.fillWidth: true
                        }
                    }

                    // Description editor + preview (horizontal split)
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 4

                        // Markdown editor
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                id: descriptionEdit
                                text: "Package Description"
                                wrapMode: TextArea.Wrap
                                font.family: "monospace"
                                onTextChanged: {
                                    descriptionUpdated = true
                                }
                            }
                        }

                        // HTML preview
                        ScrollView {
                            id: descriptionScrollView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                id: descriptionBrowser
                                readOnly: true
                                textFormat: TextArea.RichText
                                wrapMode: TextArea.Wrap
                                onLinkActivated: function(link) {
                                    Qt.openUrlExternally(link)
                                }
                            }
                        }
                    }

                    // Output group box
                    GroupBox {
                        title: "Output"
                        Layout.fillWidth: true

                        RowLayout {
                            anchors.fill: parent
                            spacing: 2

                            TextField {
                                id: outputEdit
                                Layout.fillWidth: true
                                text: settLastOutput
                            }

                            Button {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Refresh-96.png"
                                implicitWidth: implicitHeight
                                onClicked: refreshOutput()
                            }

                            Button {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                                ToolTip.visible: hovered
                                ToolTip.text: "Choose file..."
                                implicitWidth: implicitHeight
                                onClicked: outputFileDialog.open()
                            }

                            Button {
                                text: "Save Package"
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Save-96.png"
                                onClicked: savePackage()
                            }
                        }
                    }
                }
            }
        }

        // ==========================================
        // Bottom bar: DisplayPercentage + Update Archive + Uninstall Current
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 3
            Layout.rightMargin: 3
            Layout.topMargin: 3
            Layout.bottomMargin: 3

            // DisplayPercentage-style progress bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                color: Utility.getAppHexColor("darkBackground")
                border.color: Utility.getAppHexColor("darkAccent")
                border.width: 1
                radius: 5
                clip: true

                // Progress fill (top third)
                Rectangle {
                    width: dlValue > 0 ? (parent.width * dlValue / 100.0) : 0
                    height: parent.height / 3 - 2
                    y: 1
                    color: Utility.getAppHexColor("lightAccent")
                }

                // Divider line
                Rectangle {
                    width: parent.width
                    height: 2
                    y: parent.height / 3 - 1
                    color: Utility.getAppHexColor("darkAccent")
                }

                // Text (bottom two-thirds)
                Label {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: parent.height * 2 / 3
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: dlText
                    font.bold: true
                    font.family: "DejaVu Sans Mono"
                    font.pixelSize: height - 6
                    color: Utility.getAppHexColor("lightText")
                    elide: Text.ElideRight
                }
            }

            Button {
                text: "Update Archive"
                icon.source: "qrc" + Utility.getThemePath() + "icons/Refresh-96.png"
                enabled: !downloadInProgress
                onClicked: {
                    downloadInProgress = true
                    dlText = "Preparing download..."
                    dlValue = 0

                    Qt.callLater(function() {
                        var ok = mLoader.downloadPackageArchive()

                        if (ok) {
                            dlText = "Download Finished"
                            VescIf.emitStatusMessage("Downloads OK", true)
                        } else {
                            dlText = "Download Failed"
                            VescIf.emitStatusMessage("Downloads Failed", false)
                        }

                        downloadInProgress = false
                        reloadArchive()
                    })
                }
            }

            Button {
                text: "Uninstall Current"
                icon.source: "qrc" + Utility.getThemePath() + "icons/Delete-96.png"
                onClicked: {
                    if (!VescIf.isPortConnected()) {
                        VescIf.emitMessageDialog("Uninstall Package", "Not Connected", false, false)
                        return
                    }

                    Qt.callLater(function() {
                        mLoader.qmlErase(16)
                        mLoader.lispErase(16)
                        Utility.sleepWithEventLoop(500)
                        VescIf.reloadFirmware()
                        VescIf.emitMessageDialog("Uninstall Package",
                                                 "Uninstallation Done!",
                                                 true, false)
                    })
                }
            }
        }
    }

    Component.onCompleted: {
        refreshLoad()
        reloadArchive()
    }
}
