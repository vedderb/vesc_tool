/*
    Desktop FirmwarePage — faithful replica of PageFirmware (widgets).
    Tabs: "Included Files", "Custom File", "Bootloader", "Archive"
    Below tabs: upload progress + Cancel / Upload / Upload All
    Bottom: VESC Firmware Info + Supported Firmwares
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Vedder.vesc

Item {
    id: root

    // ---- Models ----
    property var hwModel: []         // [{name, path}]
    property var fwModel: []         // [{name, path}]
    property var blModel: []         // [{name, path}]
    property var archVersionModel: [] // [{name, path}]
    property var archFwModel: []     // [{name, path}]

    property int hwCurrentIndex: -1
    property int fwCurrentIndex: -1
    property int blCurrentIndex: -1
    property int archVersionCurrentIndex: -1
    property int archFwCurrentIndex: -1

    property bool showNonDefault: false
    property bool showNonDefaultArch: false
    property bool uploadOngoing: false
    property bool downloadInProgress: false

    // Custom file paths
    property string customFw1: ""
    property string customFw2: ""
    property string customFw3: ""
    property string customFw4: ""
    property int customFwSelected: 0  // 0-3
    property bool customUploadBootloader: false

    // VESC info string
    property string vescInfoStr: "Not connected"
    property string supportedFwStr: ""

    // Download progress (Included Files tab)
    property string dlFwText: ""
    property double dlFwValue: 0.0

    // Download progress (Archive tab)
    property string dlArchText: ""
    property double dlArchValue: 0.0

    // Upload progress (below tabs)
    property string uploadText: ""
    property double uploadValue: 0.0

    Component.onCompleted: {
        supportedFwStr = VescIf.getSupportedFirmwares().join(", ")
        reloadLatest()
        reloadArchive()
    }

    // ---- Resource scanning functions ----
    function reloadLatest() {
        if (VescIf.isPortConnected()) {
            var params = VescIf.getLastFwRxParams()
            updateHwList(params)
            updateBlList(params)
        } else {
            updateHwList(null)
            updateBlList(null)
        }
    }

    function reloadArchive() {
        var fwDir = "://fw_archive"
        var entries = Utility.listDirEntries(fwDir, true)
        var model = []
        for (var i = 0; i < entries.length; i++) {
            model.push({name: entries[i], path: fwDir + "/" + entries[i]})
        }
        archVersionModel = model
        if (model.length > 0) {
            archVersionCurrentIndex = 0
        }
    }

    function updateHwList(params) {
        var model = []
        var hwType = params ? params.hwType : 0  // HW_TYPE_VESC = 0
        var hwName = params ? params.hw : ""

        if (hwType === 2) { // HW_TYPE_CUSTOM_MODULE
            var path = "://res/firmwares_esp/esp32c3/" + hwName
            if (!Utility.fileExists(path)) {
                path = "://res/firmwares_esp/esp32s3/" + hwName
            }
            if (Utility.fileExists(path)) {
                model.push({name: hwName, path: path})
            }
        } else {
            var fwDir = "://res/firmwares"
            if (hwType === 1) { // HW_TYPE_VESC_BMS
                fwDir = "://res/firmwares_bms"
            }

            var entries = Utility.listDirEntries(fwDir, true)
            for (var i = 0; i < entries.length; i++) {
                var entry = entries[i]
                var names = entry.split("_o_")
                var hwMatch = (hwName === "") || names.some(function(n) {
                    return n.toLowerCase() === hwName.toLowerCase()
                })

                if (hwMatch) {
                    var displayName = names[0]
                    for (var j = 1; j < names.length; j++) {
                        displayName += " & " + names[j]
                    }
                    model.push({name: displayName, path: fwDir + "/" + entry})
                }
            }
        }

        hwModel = model
        if (model.length > 0) {
            hwCurrentIndex = 0
        } else {
            hwCurrentIndex = -1
        }
        updateFwList()
    }

    function updateFwList() {
        var model = []
        if (hwCurrentIndex >= 0 && hwCurrentIndex < hwModel.length) {
            var hwPath = hwModel[hwCurrentIndex].path
            var entries = Utility.listDirEntries(hwPath, false)
            for (var i = 0; i < entries.length; i++) {
                var fn = entries[i]
                if (showNonDefault ||
                        fn.toLowerCase() === "vesc_default.bin" ||
                        fn.toLowerCase() === "vesc_express.bin") {
                    model.push({name: fn, path: hwPath + "/" + fn})
                }
            }
        }
        fwModel = model
        if (model.length > 0) {
            fwCurrentIndex = 0
        } else {
            fwCurrentIndex = -1
        }
    }

    function updateBlList(params) {
        var model = []
        var hwType = params ? params.hwType : 0
        var hwName = params ? params.hw : ""

        var blDir = ""
        if (hwType === 0) { // HW_TYPE_VESC
            blDir = "://res/bootloaders"
        } else if (hwType === 1) { // HW_TYPE_VESC_BMS
            blDir = "://res/bootloaders_bms"
        } else if (hwType === 2) { // HW_TYPE_CUSTOM_MODULE
            if (hwName === "hm1") {
                blDir = "://res/bootloaders_bms"
            } else {
                blDir = "://res/bootloaders_custom_module/stm32g431"
            }
        }

        if (blDir === "") return

        var entries = Utility.listDirEntries(blDir, false)
        for (var i = 0; i < entries.length; i++) {
            var fn = entries[i]
            var baseName = fn.replace(".bin", "")
            var names = baseName.split("_o_")
            var hwMatch = (hwName === "") || names.some(function(n) {
                return n.toLowerCase() === hwName.toLowerCase()
            })

            if (hwMatch) {
                var displayName = names[0]
                for (var j = 1; j < names.length; j++) {
                    displayName += " & " + names[j]
                }
                model.push({name: displayName, path: blDir + "/" + fn})
            }
        }

        // Fallback to generic/stm32g431
        if (model.length === 0) {
            if (Utility.fileExists(blDir + "/generic.bin")) {
                model.push({name: "generic", path: blDir + "/generic.bin"})
            }
            if (Utility.fileExists(blDir + "/stm32g431.bin")) {
                model.push({name: "stm32g431", path: blDir + "/stm32g431.bin"})
            }
        }

        blModel = model
        if (model.length > 0) {
            blCurrentIndex = 0
        } else {
            blCurrentIndex = -1
        }
    }

    function updateArchFwList() {
        var model = []
        if (archVersionCurrentIndex >= 0 && archVersionCurrentIndex < archVersionModel.length) {
            var versionPath = archVersionModel[archVersionCurrentIndex].path
            var hwDirs = Utility.listDirEntries(versionPath, true)

            for (var i = 0; i < hwDirs.length; i++) {
                var hwDir = hwDirs[i]
                var hwPath = versionPath + "/" + hwDir
                var files = Utility.listDirEntries(hwPath, false)
                var names = hwDir.split("_o_")

                var params = VescIf.isPortConnected() ? VescIf.getLastFwRxParams() : null
                var hwName = params ? params.hw : ""

                var hwMatch = (!VescIf.isPortConnected() || hwName === "" ||
                               names.some(function(n) {
                                   return n.toLowerCase() === hwName.toLowerCase()
                               }))

                if (hwMatch) {
                    for (var j = 0; j < files.length; j++) {
                        var fn = files[j]
                        if (showNonDefaultArch || fn.toLowerCase() === "vesc_default.bin") {
                            var displayName = names[0]
                            for (var k = 1; k < names.length; k++) {
                                displayName += " & " + names[k]
                            }

                            model.push({
                                name: "HW " + displayName + ": " + fn,
                                path: hwPath + "/" + fn
                            })
                        }
                    }
                }
            }
        }
        archFwModel = model
        if (model.length > 0) {
            archFwCurrentIndex = 0
        } else {
            archFwCurrentIndex = -1
        }
    }

    function uploadFirmware(allOverCan) {
        if (!VescIf.isPortConnected()) {
            dlgTitle = "Connection Error"
            dlgText = "Not connected to device. Please connect first."
            msgDialog.open()
            return
        }

        var tabIdx = fwTabBar.currentIndex
        var fwPath = ""
        var blPath = ""

        if (tabIdx === 0 || tabIdx === 3) {
            // Included Files or Archive
            var fwItem = (tabIdx === 0) ?
                         (fwCurrentIndex >= 0 ? fwModel[fwCurrentIndex] : null) :
                         (archFwCurrentIndex >= 0 ? archFwModel[archFwCurrentIndex] : null)

            if (fwItem) {
                fwPath = fwItem.path

                if (VescIf.commands().getLimitedSupportsEraseBootloader()) {
                    if (blCurrentIndex >= 0 && blCurrentIndex < blModel.length) {
                        blPath = blModel[blCurrentIndex].path
                    }
                }
            } else {
                if (hwModel.length === 0) {
                    dlgTitle = "Upload Error"
                    dlgText = "This version of VESC Tool does not include any firmware " +
                              "for your hardware version. You can either upload a custom file " +
                              "or look for a later version of VESC Tool that might support your hardware."
                } else {
                    dlgTitle = "Upload Error"
                    dlgText = "No firmware is selected."
                }
                msgDialog.open()
                return
            }
        } else if (tabIdx === 1) {
            // Custom File
            var paths = [customFw1, customFw2, customFw3, customFw4]
            fwPath = paths[customFwSelected]

            if (!fwPath.toLowerCase().endsWith(".bin") && !fwPath.toLowerCase().endsWith(".hex")) {
                dlgTitle = "Upload Error"
                dlgText = "The selected file name seems to be invalid."
                msgDialog.open()
                return
            }

            if (customUploadBootloader && VescIf.commands().getLimitedSupportsEraseBootloader()) {
                if (blCurrentIndex >= 0 && blCurrentIndex < blModel.length) {
                    blPath = blModel[blCurrentIndex].path
                }
            }
        } else if (tabIdx === 2) {
            // Bootloader tab
            if (blCurrentIndex >= 0 && blCurrentIndex < blModel.length) {
                blPath = blModel[blCurrentIndex].path
            } else {
                if (blModel.length === 0) {
                    dlgTitle = "Upload Error"
                    dlgText = "This version of VESC Tool does not include any bootloader " +
                              "for your hardware version."
                } else {
                    dlgTitle = "Upload Error"
                    dlgText = "No bootloader is selected."
                }
                msgDialog.open()
                return
            }
        }

        // Show confirmation dialog
        confirmAllOverCan = allOverCan
        confirmFwPath = fwPath
        confirmBlPath = blPath

        if ((tabIdx === 0 && hwModel.length === 1) || tabIdx === 3) {
            confirmTitle = "Warning"
            confirmText = "Uploading new firmware will clear all settings in the VESC firmware " +
                          "and you have to do the configuration again. Do you want to continue?"
        } else if ((tabIdx === 0 && hwModel.length > 1) || tabIdx === 1) {
            confirmTitle = "Warning"
            confirmText = "Uploading firmware for the wrong hardware version " +
                          "WILL damage the hardware. Are you sure that you have " +
                          "chosen the correct hardware version?"
        } else if (tabIdx === 2) {
            if (VescIf.commands().getLimitedSupportsEraseBootloader()) {
                confirmTitle = "Warning"
                confirmText = "This will attempt to upload a bootloader to the connected device. " +
                              "Do you want to continue?"
            } else {
                confirmTitle = "Warning"
                confirmText = "This will attempt to upload a bootloader to the connected device. " +
                              "If the connected device already has a bootloader this will destroy " +
                              "the bootloader and firmware updates cannot be done anymore. Do " +
                              "you want to continue?"
            }
        }

        confirmDialog.open()
    }

    property string dlgTitle: ""
    property string dlgText: ""
    property string confirmTitle: ""
    property string confirmText: ""
    property string confirmFwPath: ""
    property string confirmBlPath: ""
    property bool confirmAllOverCan: false

    function doUpload() {
        if (confirmBlPath !== "") {
            VescIf.fwUploadFromFile(confirmBlPath, true, confirmAllOverCan)
        }
        if (confirmFwPath !== "") {
            VescIf.fwUploadFromFile(confirmFwPath, false, confirmAllOverCan)
        }
    }

    // ---- Connections ----
    Connections {
        target: VescIf
        function onFwUploadStatus(msg, progress, isOngoing) {
            uploadOngoing = isOngoing
            if (isOngoing) {
                uploadText = msg + " (" + (progress * 100).toFixed(1) + " %)"
            } else {
                uploadText = msg
            }
            uploadValue = progress * 100
        }
        function onFwRxChanged(rx, limited) {
            if (!rx) return

            var params = VescIf.getLastFwRxParams()
            var fwStr = ""
            var strUuid = Utility.uuid2Str(params.uuid, true)

            if (params.major >= 0) {
                fwStr = "Fw: v" + params.major + "." + String(params.minor).padStart(2, '0')
                if (params.fwName.length > 0) fwStr += " (" + params.fwName + ")"
                if (params.hw.length > 0) fwStr += ", Hw: " + params.hw
                if (strUuid.length > 0) fwStr += "\n" + strUuid
            } else {
                if (strUuid.length > 0) fwStr += ", UUID: " + strUuid
            }

            fwStr += "\nPaired: " + (params.isPaired ? "true" : "false") + ", Status: "
            if (params.isTestFw > 0) {
                fwStr += "BETA " + params.isTestFw
            } else {
                fwStr += "STABLE"
            }

            fwStr += "\nHW Type: " + params.hwTypeStr()

            if (params.hwType === 0) { // HW_TYPE_VESC
                fwStr += ", Phase Filters: " + (params.hasPhaseFilters ? "Yes" : "No")
            }

            fwStr += "\nNRF Name: " + (params.nrfNameSupported ? "Yes" : "No")
            fwStr += ", Pin: " + (params.nrfPinSupported ? "Yes" : "No")

            vescInfoStr = fwStr
            updateHwList(params)
            updateBlList(params)
            updateArchFwList()
        }
        function onFwArchiveDlProgress(msg, prog) {
            dlFwText = msg
            dlFwValue = prog * 100
            dlArchText = msg
            dlArchValue = prog * 100
        }
    }

    Timer {
        interval: 500
        running: true
        repeat: true
        onTriggered: {
            if (!VescIf.isPortConnected()) {
                vescInfoStr = "Not connected"
            }
        }
    }

    // ---- Dialogs ----
    Dialog {
        id: msgDialog
        title: dlgTitle
        modal: true
        standardButtons: Dialog.Ok
        anchors.centerIn: parent
        width: Math.min(400, root.width - 40)
        Label {
            text: dlgText
            wrapMode: Text.WordWrap
            width: parent.width
        }
    }

    Dialog {
        id: confirmDialog
        title: confirmTitle
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        anchors.centerIn: parent
        width: Math.min(450, root.width - 40)
        Label {
            text: confirmText
            wrapMode: Text.WordWrap
            width: parent.width
        }
        onAccepted: doUpload()
    }

    Dialog {
        id: changelogDialog
        title: "Firmware Changelog"
        modal: true
        standardButtons: Dialog.Ok
        anchors.centerIn: parent
        width: Math.min(root.width - 60, 700)
        height: Math.min(root.height - 60, 500)
        ScrollView {
            anchors.fill: parent
            clip: true
            TextArea {
                readOnly: true
                text: Utility.fwChangeLog()
                font.pointSize: 12
                wrapMode: Text.WordWrap
            }
        }
    }

    FileDialog {
        id: customFileDialog1
        title: "Choose Firmware File"
        nameFilters: ["Firmware files (*.bin *.hex)", "All files (*)"]
        onAccepted: customFw1 = selectedFile.toString().replace("file://", "")
    }
    FileDialog {
        id: customFileDialog2
        title: "Choose Firmware File"
        nameFilters: ["Firmware files (*.bin *.hex)", "All files (*)"]
        onAccepted: customFw2 = selectedFile.toString().replace("file://", "")
    }
    FileDialog {
        id: customFileDialog3
        title: "Choose Firmware File"
        nameFilters: ["Firmware files (*.bin *.hex)", "All files (*)"]
        onAccepted: customFw3 = selectedFile.toString().replace("file://", "")
    }
    FileDialog {
        id: customFileDialog4
        title: "Choose Firmware File"
        nameFilters: ["Firmware files (*.bin *.hex)", "All files (*)"]
        onAccepted: customFw4 = selectedFile.toString().replace("file://", "")
    }

    // ---- Main Layout ----
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // GroupBox: Install New Firmware
        GroupBox {
            title: "Install New Firmware"
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 4

                TabBar {
                    id: fwTabBar
                    Layout.fillWidth: true
                    TabButton { text: "Included Files"; topPadding: 9; bottomPadding: 9 }
                    TabButton { text: "Custom File"; topPadding: 9; bottomPadding: 9 }
                    TabButton { text: "Bootloader"; topPadding: 9; bottomPadding: 9 }
                    TabButton { text: "Archive"; topPadding: 9; bottomPadding: 9 }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: fwTabBar.currentIndex

                    // ======== Tab 0: Included Files ========
                    ColumnLayout {
                        spacing: 4

                        // Download Latest row
                        RowLayout {
                            spacing: 4
                            Layout.fillWidth: true

                            // DisplayPercentage-style bar
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
                                    width: dlFwValue > 0 ? (parent.width * dlFwValue / 100.0) : 0
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
                                    text: dlFwText
                                    font.bold: true
                                    font.family: "DejaVu Sans Mono"
                                    font.pixelSize: height - 6
                                    color: Utility.getAppHexColor("lightText")
                                    elide: Text.ElideRight
                                }
                            }

                            Button {
                                id: dlFwButton
                                text: "Download Latest"
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Download-96.png"
                                enabled: !downloadInProgress
                                onClicked: {
                                    downloadInProgress = true
                                    dlFwText = "Preparing download..."
                                    Qt.callLater(function() {
                                        VescIf.downloadFwLatest()
                                        VescIf.downloadConfigs()
                                        reloadLatest()
                                        downloadInProgress = false
                                    })
                                }
                            }
                        }

                        // HW List + FW List side by side
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 8

                            // Hardware Version
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 1

                                Label {
                                    text: "Hardware Version"
                                    font.bold: true
                                }

                                ListView {
                                    id: hwListView
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    model: hwModel
                                    currentIndex: hwCurrentIndex
                                    onCurrentIndexChanged: {
                                        if (currentIndex !== hwCurrentIndex) {
                                            hwCurrentIndex = currentIndex
                                            updateFwList()
                                        }
                                    }

                                    delegate: ItemDelegate {
                                        width: hwListView.width
                                        text: modelData.name
                                        highlighted: ListView.isCurrentItem
                                        onClicked: hwListView.currentIndex = index
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        color: "transparent"
                                        border.color: Utility.getAppHexColor("disabledText")
                                        border.width: 1
                                        z: -1
                                    }
                                }
                            }

                            // Firmware
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 1

                                Label {
                                    text: "Firmware"
                                    font.bold: true
                                }

                                ListView {
                                    id: fwListView
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    model: fwModel
                                    currentIndex: fwCurrentIndex
                                    onCurrentIndexChanged: {
                                        if (currentIndex !== fwCurrentIndex) {
                                            fwCurrentIndex = currentIndex
                                        }
                                    }

                                    delegate: ItemDelegate {
                                        width: fwListView.width
                                        text: modelData.name
                                        highlighted: ListView.isCurrentItem
                                        onClicked: fwListView.currentIndex = index
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        color: "transparent"
                                        border.color: Utility.getAppHexColor("disabledText")
                                        border.width: 1
                                        z: -1
                                    }
                                }
                            }
                        }

                        // Bottom row: checkbox + changelog button
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CheckBox {
                                text: "Show non-default firmwares"
                                checked: showNonDefault
                                onCheckedChanged: {
                                    showNonDefault = checked
                                    updateFwList()
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Button {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/About-96.png"
                                ToolTip.text: "Firmware Changelog"
                                ToolTip.visible: hovered
                                onClicked: changelogDialog.open()
                            }
                        }
                    }

                    // ======== Tab 1: Custom File ========
                    ColumnLayout {
                        spacing: 8

                        Repeater {
                            model: [
                                {idx: 0, label: "Custom file 1"},
                                {idx: 1, label: "Custom file 2"},
                                {idx: 2, label: "Custom file 3"},
                                {idx: 3, label: "Custom file 4"}
                            ]

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                RadioButton {
                                    text: "Use"
                                    checked: customFwSelected === modelData.idx
                                    onClicked: customFwSelected = modelData.idx
                                }

                                TextField {
                                    Layout.fillWidth: true
                                    text: modelData.idx === 0 ? customFw1 :
                                          modelData.idx === 1 ? customFw2 :
                                          modelData.idx === 2 ? customFw3 : customFw4
                                    onTextChanged: {
                                        if (modelData.idx === 0) customFw1 = text
                                        else if (modelData.idx === 1) customFw2 = text
                                        else if (modelData.idx === 2) customFw3 = text
                                        else customFw4 = text
                                    }
                                    placeholderText: modelData.label + "..."
                                }

                                Button {
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                                    ToolTip.text: "Choose firmware file"
                                    ToolTip.visible: hovered
                                    onClicked: {
                                        if (modelData.idx === 0) customFileDialog1.open()
                                        else if (modelData.idx === 1) customFileDialog2.open()
                                        else if (modelData.idx === 2) customFileDialog3.open()
                                        else customFileDialog4.open()
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        CheckBox {
                            text: "Upload Bootloader"
                            checked: customUploadBootloader
                            onCheckedChanged: customUploadBootloader = checked
                        }
                    }

                    // ======== Tab 2: Bootloader ========
                    ColumnLayout {
                        spacing: 4

                        ListView {
                            id: blListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: blModel
                            currentIndex: blCurrentIndex
                            onCurrentIndexChanged: {
                                if (currentIndex !== blCurrentIndex) {
                                    blCurrentIndex = currentIndex
                                }
                            }

                            delegate: ItemDelegate {
                                width: blListView.width
                                text: modelData.name
                                highlighted: ListView.isCurrentItem
                                onClicked: blListView.currentIndex = index
                            }

                            Rectangle {
                                anchors.fill: parent
                                color: "transparent"
                                border.color: Utility.getAppHexColor("disabledText")
                                border.width: 1
                                z: -1
                            }
                        }
                    }

                    // ======== Tab 3: Archive ========
                    ColumnLayout {
                        spacing: 4

                        // Download Archive row
                        RowLayout {
                            spacing: 4
                            Layout.fillWidth: true

                            // DisplayPercentage-style bar
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
                                    width: dlArchValue > 0 ? (parent.width * dlArchValue / 100.0) : 0
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
                                    text: dlArchText
                                    font.bold: true
                                    font.family: "DejaVu Sans Mono"
                                    font.pixelSize: height - 6
                                    color: Utility.getAppHexColor("lightText")
                                    elide: Text.ElideRight
                                }
                            }

                            Button {
                                id: dlArchiveButton
                                text: "Download Archive"
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Download-96.png"
                                enabled: !downloadInProgress
                                onClicked: {
                                    downloadInProgress = true
                                    dlArchText = "Preparing download..."
                                    Qt.callLater(function() {
                                        VescIf.downloadFwArchive()
                                        reloadArchive()
                                        downloadInProgress = false
                                    })
                                }
                            }
                        }

                        // Version List + File List side by side
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 8

                            // Firmware Version
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 1

                                Label {
                                    text: "Firmware Version"
                                    font.bold: true
                                }

                                ListView {
                                    id: archVersionListView
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    model: archVersionModel
                                    currentIndex: archVersionCurrentIndex
                                    onCurrentIndexChanged: {
                                        if (currentIndex !== archVersionCurrentIndex) {
                                            archVersionCurrentIndex = currentIndex
                                            updateArchFwList()
                                        }
                                    }

                                    delegate: ItemDelegate {
                                        width: archVersionListView.width
                                        text: modelData.name
                                        highlighted: ListView.isCurrentItem
                                        onClicked: archVersionListView.currentIndex = index
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        color: "transparent"
                                        border.color: Utility.getAppHexColor("disabledText")
                                        border.width: 1
                                        z: -1
                                    }
                                }
                            }

                            // File
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 1

                                Label {
                                    text: "File"
                                    font.bold: true
                                }

                                ListView {
                                    id: archFwListView
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    model: archFwModel
                                    currentIndex: archFwCurrentIndex
                                    onCurrentIndexChanged: {
                                        if (currentIndex !== archFwCurrentIndex) {
                                            archFwCurrentIndex = currentIndex
                                        }
                                    }

                                    delegate: ItemDelegate {
                                        width: archFwListView.width
                                        text: modelData.name
                                        highlighted: ListView.isCurrentItem
                                        onClicked: archFwListView.currentIndex = index
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        color: "transparent"
                                        border.color: Utility.getAppHexColor("disabledText")
                                        border.width: 1
                                        z: -1
                                    }
                                }
                            }
                        }

                        // Bottom row: checkbox
                        CheckBox {
                            text: "Show non-default firmwares"
                            checked: showNonDefaultArch
                            onCheckedChanged: {
                                showNonDefaultArch = checked
                                updateArchFwList()
                            }
                        }
                    }
                }

                // ---- Upload progress + buttons (below tabs) ----
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    // DisplayPercentage-style upload bar
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
                            width: uploadValue > 0 ? (parent.width * uploadValue / 100.0) : 0
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
                            text: uploadText
                            font.bold: true
                            font.family: "DejaVu Sans Mono"
                            font.pixelSize: height - 6
                            color: Utility.getAppHexColor("lightText")
                            elide: Text.ElideRight
                        }
                    }

                    Button {
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Cancel-96.png"
                        enabled: uploadOngoing
                        ToolTip.text: "Cancel upload"
                        ToolTip.visible: hovered
                        onClicked: VescIf.fwUploadCancel()
                    }

                    Button {
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Download-96.png"
                        enabled: !uploadOngoing
                        ToolTip.text: "Upload firmware"
                        ToolTip.visible: hovered
                        onClicked: uploadFirmware(false)
                    }

                    Button {
                        text: "All"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Download-96.png"
                        enabled: !uploadOngoing && VescIf.commands().getLimitedSupportsFwdAllCan() &&
                                 !VescIf.commands().getSendCan()
                        ToolTip.text: "Upload firmware to the connected VESC, as well as to the VESCs connected over CAN-bus."
                        ToolTip.visible: hovered
                        onClicked: uploadFirmware(true)
                    }
                }
            }
        }

        // ---- Bottom: Info GroupBoxes side-by-side ----
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            GroupBox {
                title: "VESC Firmware, Hardware and UUID"
                Layout.fillWidth: true
                Layout.preferredWidth: 2

                RowLayout {
                    anchors.fill: parent
                    spacing: 4

                    Label {
                        id: vescInfoLabel
                        text: vescInfoStr
                        color: Utility.getAppHexColor("lightText")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Button {
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Upload-96.png"
                        ToolTip.text: "Read firmware version"
                        ToolTip.visible: hovered
                        onClicked: {
                            if (VescIf.isPortConnected()) {
                                VescIf.commands().getFwVersion()
                            }
                        }
                    }
                }
            }

            GroupBox {
                title: "Supported Firmwares Loaded"
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                Label {
                    text: supportedFwStr
                    color: Utility.getAppHexColor("lightText")
                    wrapMode: Text.WordWrap
                    anchors.fill: parent
                }
            }
        }
    }
}
