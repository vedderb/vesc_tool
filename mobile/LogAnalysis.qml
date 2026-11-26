/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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

// Note: This file is heavily AI-assisted

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Dialogs
import QtLocation
import QtPositioning
import QtCore

import Vedder.vesc.utility
import Vedder.vesc.vescinterface

Item {
    id: root
    // Map and graphs side by side in landscape. On phones the header only spans the map, so the
    // graphs get the full height.
    readonly property bool wide: mainIsHorizontal
    readonly property bool headerOverMap: phoneLandscape
    // Tablets and large windows in landscape show the map, the values and the graphs together
    readonly property bool showAll: wide && !headerOverMap
    anchors.fill: parent

    property var mCommands: VescIf.commands()
    property string deviceCurrentDir: "/"
    property var deviceFileEntries: []
    property bool deviceLoading: false
    property bool downloadingFromDevice: false
    property real downloadProgress: 0.0
    property real downloadSpeedKBps: 0.0
    property string pendingDeletePath: ""
    property string pendingDeleteName: ""
    property string loadedCsvText: ""
    property string loadedFileName: "vesc_log.csv"
    property string pendingBackupName: ""

    function promptDeleteDeviceFile(fileName) {
        var base = deviceCurrentDir.endsWith("/") ? deviceCurrentDir : (deviceCurrentDir + "/")
        pendingDeleteIsLocal = false
        pendingDeleteName = fileName
        pendingDeletePath = base + fileName
        confirmDeleteDialog.open()
    }

    // Live progress timer while fileBlockRead() runs its internal event loop
    Timer {
        interval: 100
        repeat: true
        running: root.downloadingFromDevice
        onTriggered: {
            root.downloadProgress = root.mCommands.getFilePercentage()
            root.downloadSpeedKBps = root.mCommands.getFileSpeed() / 1024.0
        }
    }

    function openDeviceBrowser() {
        if (!VescIf.isPortConnected()) {
            statusText = "Connect to a VESC device first."
            return
        }
        refreshDeviceDir(deviceCurrentDir)
        deviceFileDialog.open()
    }

    function refreshDeviceDir(dirPath) {
        deviceCurrentDir = dirPath
        deviceFileEntries = []
        deviceLoading = true
        // Later, so that the sheet shows that it is loading while the listing blocks
        Qt.callLater(listDeviceDir, dirPath)
    }

    function listDeviceDir(dirPath) {
        var rawList = mCommands.fileBlockList(dirPath)
        var items = []

        if (dirPath !== "/" && dirPath.length > 0) {
            items.push({ name: "..", isDir: true, size: 0, timestamp: 0 })
        }

        for (var i = 0; i < rawList.length; i++) {
            var entry = rawList[i]
            var name = ""
            var isDir = false
            var size = 0

            if (typeof entry === "string") {
                name = entry
                isDir = name.endsWith("/")
                if (isDir) name = name.slice(0, -1)
            } else if (entry && entry.name !== undefined) {
                name = entry.name
                isDir = Boolean(entry.isDir)
                size = entry.size || 0
            }

            // Show directories and .csv files
            if (isDir || name.toLowerCase().endsWith(".csv")) {
                items.push({ name: name, isDir: isDir, size: size, timestamp: nameTime(name) })
            }
        }

        deviceFileEntries = items
        deviceLoading = false
    }

    // Time in the name of a log, like 2026-11-16_16-05-55.csv, or 0
    function nameTime(name) {
        var m = name.match(/(\d{4})-(\d\d)-(\d\d)_(\d\d)-(\d\d)-(\d\d)/)
        return m ? new Date(m[1], m[2] - 1, m[3], m[4], m[5], m[6]).getTime() : 0
    }

    function loadDeviceCsv(fileName) {
        var base = deviceCurrentDir.endsWith("/") ? deviceCurrentDir : (deviceCurrentDir + "/")
        var fullPath = base + fileName

        downloadingFromDevice = true
        downloadProgress = 0
        downloadSpeedKBps = 0
        statusText = "Downloading " + fullPath + "..."

        var byteData = mCommands.fileBlockRead(fullPath)
        downloadingFromDevice = false

        if (mCommands.fileBlockDidCancel()) {
            statusText = "Download cancelled."
            return
        }

        var text = byteData ? byteData.toString() : ""
        if (!text || text.length === 0) {
            statusText = "Failed to read file from device: " + fullPath
            return
        }

        loadedFileName = fileName
        parseCsvText(text)
    }

    function sizeText(bytes) {
        return bytes < 1048576 ? (bytes / 1024).toFixed(1) + " KB" : (bytes / 1048576).toFixed(1) + " MB"
    }

    // Like Nov 17, 2026 · 4:05 PM, with the time in the format of the locale
    function dateText(ms) {
        return Qt.formatDateTime(new Date(ms), "MMM d, yyyy · " + Qt.locale().timeFormat(Locale.ShortFormat))
    }

    // Like 2 of 6 logs · 2.0 MB · /log
    function logCountText(shown, logs, path) {
        return qsTr("%1 logs · %2 · %3").arg((shown < logs.length ? shown + " of " : "") + logs.length).
                arg(sizeText(logs.reduce((sum, e) => sum + e.size, 0))).arg(path)
    }

    // Distance of a log file in meters, kept as reading the file takes a while
    property var logDistances: ({})
    function logMeters(path, size) {
        var key = path + size
        if (logDistances[key] === undefined) {
            logDistances[key] = Utility.logDistance(path)
        }
        return logDistances[key]
    }

    function logDistanceText(path, size) {
        var m = logMeters(path, size)
        return m < 0 ? "" : distanceText(m)
    }

    function distanceText(m) {
        return VescIf.useImperialUnits() ? (m / 1609.344).toFixed(2) + " mi" :
                                           m < 1000 ? Math.round(m) + " m" : (m / 1000).toFixed(2) + " km"
    }

    // Row of the log lists: icon, name, details, a value and actions above a divider
    component LogRow: Item {
        id: logRow
        property string iconSource: ""
        property string name: ""
        property string details: ""
        property string value: ""
        property bool canSave: false
        property bool canDelete: false
        signal clicked()
        signal saveClicked()
        signal deleteClicked()

        implicitHeight: rowLayout.implicitHeight

        RowLayout {
            id: rowLayout
            anchors.fill: parent
            spacing: 0

            ItemDelegate {
                Layout.fillWidth: true
                onClicked: logRow.clicked()

                contentItem: RowLayout {
                    spacing: 10

                    Image {
                        visible: logRow.iconSource !== ""
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        source: logRow.iconSource
                        mipmap: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Text {
                            Layout.fillWidth: true
                            text: logRow.name
                            elide: Text.ElideMiddle
                            color: Utility.getAppHexColor("lightText")
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: text !== ""
                            text: logRow.details
                            elide: Text.ElideRight
                            font.pointSize: 9
                            color: Utility.getAppHexColor("normalText")
                        }
                    }

                    Text {
                        visible: text !== ""
                        text: logRow.value
                        font.weight: Font.Medium
                        color: Utility.getAppHexColor("lightText")
                    }
                }
            }

            ToolButton {
                visible: logRow.canSave
                icon.source: "qrc" + Utility.getThemePath() + "icons/Save-96.png"
                Accessible.name: qsTr("Save")
                onClicked: logRow.saveClicked()
            }

            ToolButton {
                visible: logRow.canDelete
                icon.source: "qrc" + Utility.getThemePath() + "icons/Delete-96.png"
                icon.width: 20
                icon.height: 20
                icon.color: Utility.getAppHexColor("danger")
                Accessible.name: qsTr("Delete")
                onClicked: logRow.deleteClicked()
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Utility.getAppHexColor("lightestBackground")
        }
    }

    // Shown in place of an empty log list
    component EmptyLogs: ColumnLayout {
        property alias text: emptyLabel.text
        spacing: 12

        Item { Layout.fillHeight: true }

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            source: "qrc" + Utility.getThemePath() + "icons/Line Chart-96.png"
            opacity: 0.35
        }

        Label {
            id: emptyLabel
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: Utility.getAppHexColor("normalText")
        }

        Item { Layout.fillHeight: true }
    }

    // Search field and sort chips above the count of logs. The chips go next to the field when it
    // gets 240 pt, as in landscape and on tablets.
    component LogSearch: ColumnLayout {
        id: logSearch
        property alias text: searchField.displayText
        property alias sorts: sortRepeater.model
        property string sortBy: "Date"
        property bool ascending: false
        property alias count: countLabel.text
        spacing: 8

        // If s has all searched words, ignoring case
        function matches(s) {
            var t = s.toLowerCase()
            return searchField.displayText.toLowerCase().split(" ").every(w => t.indexOf(w) >= 0)
        }

        // Ascending comparison d in the picked direction
        function order(d) {
            return ascending ? d : -d
        }

        function clear() {
            searchField.text = ""
            // Or the field takes the focus and the keyboard on the next open
            searchField.focus = false
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > sortChips.implicitWidth + 240 ? 2 : 1
            rowSpacing: 0
            columnSpacing: 8

            InputField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search logs")
                inputMethodHints: Qt.ImhNoPredictiveText
            }

            Row {
                id: sortChips
                spacing: 6

                Repeater {
                    id: sortRepeater

                    // The picked sort shows its direction, tapping it again flips it. Name starts
                    // A-Z, the others newest, longest or largest first.
                    Chip {
                        id: sortChip
                        text: modelData + (checked ? (logSearch.ascending ? " ↑" : " ↓") : "")
                        // Room for the arrow on every chip, so picking one does not move the others
                        implicitWidth: Math.ceil(arrowMetrics.advanceWidth) + 24
                        checked: logSearch.sortBy === modelData
                        onClicked: {
                            Qt.inputMethod.hide()
                            logSearch.ascending = checked ? !logSearch.ascending : modelData === "Name"
                            logSearch.sortBy = modelData
                        }

                        TextMetrics {
                            id: arrowMetrics
                            font.family: sortChip.font.family
                            font.weight: Font.Medium
                            text: modelData + " ↓"
                        }
                    }
                }
            }
        }

        Label {
            id: countLabel
            Layout.fillWidth: true
            elide: Text.ElideMiddle
            font.pointSize: 10
            color: Utility.getAppHexColor("normalText")
        }
    }

    SheetDialog {
        id: confirmDeleteDialog
        acceptText: qsTr("Delete")
        title: "Delete Device Log?"
        closePolicy: Popup.NoAutoClose

        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "Delete '" + root.pendingDeleteName + "' permanently?"
        }

        onAccepted: root.executeDeleteFile()
    }

    SheetDialog {
        id: deviceFileDialog
        // Up, the folders and then the logs with all searched words in their name, in the picked order
        readonly property var shownFiles: root.deviceFileEntries.filter(e => e.name === ".." || deviceSearch.matches(e.name))
            .sort((a, b) => (b.name === "..") - (a.name === "..") || b.isDir - a.isDir ||
                            deviceSearch.order((deviceSearch.sortBy === "Date" ? a.timestamp - b.timestamp :
                                                deviceSearch.sortBy === "Size" ? a.size - b.size : 0) || a.name.localeCompare(b.name)))
        readonly property var logs: root.deviceFileEntries.filter(e => !e.isDir)

        // Landscape phones have the height for a few rows only, the path is in the count line there
        title: phoneLandscape ? "" : "Device Logs"
        fullHeight: true
        closePolicy: Popup.NoAutoClose
        // Cleared on open, as clearing on Close mid-download rebuilds the row that is downloading
        onAboutToShow: deviceSearch.clear()

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                visible: !phoneLandscape
                enabled: !root.deviceLoading && !root.downloadingFromDevice
                InputField {
                    id: dirInput
                    Layout.fillWidth: true
                    text: root.deviceCurrentDir
                    placeholderText: "/log"
                    onAccepted: root.refreshDeviceDir(text)
                }
                Button {
                    text: "Go"
                    onClicked: root.refreshDeviceDir(dirInput.text)
                }
            }

            LogSearch {
                id: deviceSearch
                Layout.fillWidth: true
                enabled: !root.downloadingFromDevice
                sorts: ["Date", "Name", "Size"]
                count: root.logCountText(deviceFileDialog.shownFiles.filter(e => !e.isDir).length,
                                         deviceFileDialog.logs, root.deviceCurrentDir)
            }

            ListView {
                id: deviceListView
                Layout.fillWidth: true
                // Only as tall as the Up row above the empty text
                Layout.fillHeight: !deviceEmpty.visible
                Layout.preferredHeight: deviceEmpty.visible ? contentHeight : 0
                clip: true
                enabled: !root.downloadingFromDevice
                model: deviceFileDialog.shownFiles

                delegate: LogRow {
                    width: deviceListView.width
                    iconSource: modelData.isDir ? "qrc" + Utility.getThemePath() + "icons/" +
                                                  (modelData.name === ".." ? "Up-96.png" : "Open Folder-96.png") : ""
                    name: modelData.name === ".." ? qsTr("Up") : modelData.name
                    details: modelData.name === ".." ? "" : modelData.isDir ? qsTr("Folder") :
                             root.sizeText(modelData.size) + (modelData.timestamp ? " · " + root.dateText(modelData.timestamp) : "")
                    canSave: !modelData.isDir
                    canDelete: !modelData.isDir
                    onClicked: {
                        if (modelData.isDir) {
                            if (modelData.name === "..") {
                                var parts = root.deviceCurrentDir.replace(/\/+$/, "").split("/")
                                parts.pop()
                                var up = parts.join("/")
                                root.refreshDeviceDir(up.length === 0 ? "/" : up)
                            } else {
                                var next = (root.deviceCurrentDir === "/" ? "/" : root.deviceCurrentDir + "/") + modelData.name
                                root.refreshDeviceDir(next)
                            }
                        } else {
                            root.loadDeviceCsv(modelData.name)
                            if (!root.mCommands.fileBlockDidCancel()) {
                                deviceFileDialog.close()
                            }
                        }
                    }
                    onSaveClicked: root.backupDeviceCsv(modelData.name)
                    onDeleteClicked: root.promptDeleteDeviceFile(modelData.name)
                }

            }

            EmptyLogs {
                id: deviceEmpty
                visible: deviceFileDialog.shownFiles.every(e => e.name === "..")
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: root.deviceLoading ? qsTr("Loading…") :
                      deviceFileDialog.shownFiles.length < root.deviceFileEntries.length ?
                          "No logs match \"" + deviceSearch.text + "\"" : "No .csv logs found in:\n" + root.deviceCurrentDir
            }

            // Download progress bar & cancel button
            RowLayout {
                Layout.fillWidth: true
                visible: root.downloadingFromDevice
                spacing: 8

                ProgressBar {
                    Layout.fillWidth: true
                    from: 0; to: 100
                    value: root.downloadProgress
                }
                Label {
                    text: root.downloadProgress.toFixed(0) + "% (" + root.downloadSpeedKBps.toFixed(1) + " KB/s)"
                }
                Button {
                    text: "Cancel"
                    onClicked: root.mCommands.fileBlockCancel()
                }
            }
        }
    }

    function rtLogFolderUrl(dir) {
        if (!dir || dir.length === 0) return ""
        if (dir.startsWith("file://") || dir.startsWith("content://")) return dir
        return dir.startsWith("/") ? ("file://" + dir) : ("file:///" + dir)
    }

    property var rtLogEntries: []
    property bool pendingDeleteIsLocal: false
    property string currentRtLogDir: ""

    function promptDeleteLocalFile(fileName, fullPath) {
        pendingDeleteIsLocal = true
        pendingDeleteName = fileName
        pendingDeletePath = fullPath
        confirmDeleteDialog.open()
    }

    function executeDeleteFile() {
        if (pendingDeletePath.length === 0) return
        if (pendingDeleteIsLocal) {
            if (Utility.removeFile(pendingDeletePath)) {
                statusText = "Deleted " + pendingDeleteName + "."
                if (rtLogListDialog.visible) {
                    rtLogEntries = Utility.listFilesInDir(root.currentRtLogDir)
                }
                if (recentLogsDialog.visible) {
                    openRecentLogsDialog()
                }
            } else {
                statusText = "Failed to delete " + pendingDeleteName + "."
            }
        } else {
            if (mCommands.fileBlockRemove(pendingDeletePath)) {
                statusText = "Deleted " + pendingDeleteName + " from device."
                refreshDeviceDir(deviceCurrentDir)
            } else {
                statusText = "Failed to delete " + pendingDeleteName + "."
            }
        }
        pendingDeletePath = ""
        pendingDeleteName = ""
    }

    function openRtLogBrowser() {
        var dir = Utility.getSetting("path_rt_log", "").toString()
        if (!dir || dir.length === 0) {
            statusText = "RT log directory (path_rt_log) is not set."
            return
        }

        currentRtLogDir = dir

        if (dir.startsWith("content://")) {
            rtLogDialog.currentFolder = dir
            rtLogDialog.open()
        } else {
            rtLogEntries = Utility.listFilesInDir(dir)
            rtLogListDialog.open()
        }
    }

    SheetDialog {
        id: rtLogListDialog
        // Logs with all searched words in their name or date, in the picked order
        readonly property var shownLogs: root.rtLogEntries.filter(e => rtLogSearch.matches(e.name + " " + e.modified + " " + root.dateText(e.timestamp)))
            .sort((a, b) => rtLogSearch.order(rtLogSearch.sortBy === "Distance" ? root.logMeters(a.path, a.size) - root.logMeters(b.path, b.size) :
                                              rtLogSearch.sortBy === "Size" ? a.size - b.size : a.timestamp - b.timestamp))

        title: "RT Logs"
        acceptText: qsTr("Close")
        rejectText: ""
        fullHeight: true
        onOpened: distanceTimer.next = 0
        onClosed: rtLogSearch.clear()

        // Reads the distances one log at a time while the sheet is open, so that sorting by them is quick
        Timer {
            id: distanceTimer
            property int next: 0
            interval: 50
            repeat: true
            running: rtLogListDialog.opened && next < root.rtLogEntries.length
            onTriggered: {
                var e = root.rtLogEntries[next++]
                root.logMeters(e.path, e.size)
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            LogSearch {
                id: rtLogSearch
                Layout.fillWidth: true
                visible: root.rtLogEntries.length > 0
                sorts: ["Date", "Distance", "Size"]
                count: root.logCountText(rtLogListDialog.shownLogs.length, root.rtLogEntries, displayPath(root.currentRtLogDir))
            }

            EmptyLogs {
                visible: rtLogListDialog.shownLogs.length === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: root.rtLogEntries.length === 0 ? "No .csv logs found in:\n" + displayPath(root.currentRtLogDir) :
                                                       "No logs match \"" + rtLogSearch.text + "\""
            }

            ListView {
                id: rtLogListView
                visible: rtLogListDialog.shownLogs.length > 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: rtLogListDialog.shownLogs

                delegate: LogRow {
                    width: rtLogListView.width
                    name: modelData.name
                    details: root.sizeText(modelData.size) + " · " + root.dateText(modelData.timestamp)
                    value: root.logDistanceText(modelData.path, modelData.size)
                    canDelete: true
                    onClicked: {
                        var text = Utility.readTextFile(modelData.path)
                        if (text && text.length > 0) {
                            root.loadedFileName = modelData.name
                            root.parseCsvText(text)
                            recordRecentLocalLog(modelData.path)
                            rtLogListDialog.close()
                        } else {
                            root.statusText = "Failed to read: " + modelData.name
                        }
                    }
                    onDeleteClicked: root.promptDeleteLocalFile(modelData.name, modelData.path)
                }
            }
        }
    }

    FileDialog {
        id: saveCsvDialog
        title: "Save Local CSV Backup"
        fileMode: FileDialog.SaveFile
        currentFile: root.loadedFileName
        nameFilters: Qt.platform.os === "android"
                     ? ["All files (*)"]
                     : ["CSV files (*.csv)", "All files (*)"]
        onAccepted: {
            if (Utility.writeTextFile(selectedFile.toString(), root.pendingSaveText)) {
                root.statusText = "Saved: " + decodeURIComponent(selectedFile.toString()).split("/").pop()
                recordRecentLocalLog(selectedFile)
            } else {
                root.statusText = "Failed to save CSV."
            }
        }
    }

    FileDialog {
        id: rtLogDialog
        title: "Select VESC RT Log"
        currentFolder: root.rtLogFolderUrl()
        nameFilters: Qt.platform.os === "android"
                     ? ["All files (*)"]
                     : ["CSV files (*.csv)", "All files (*)"]
        onAccepted: root.loadCsv(selectedFile)
    }

    function saveLoadedLogLocally() {
        if (!loadedCsvText || loadedCsvText.length === 0) {
            statusText = "No log loaded to save."
            return
        }
        pendingSaveText = loadedCsvText
        saveCsvDialog.currentFile = loadedFileName
        saveCsvDialog.open()
    }

    function saveZoomedLogLocally() {
        if (loadedDataLines.length === 0) {
            statusText = "No log loaded to save."
            return
        }
        var s = Math.max(0, Math.min(viewStart, loadedDataLines.length - 1))
        var e = Math.max(s, Math.min(viewEnd, loadedDataLines.length - 1))
        var sliced = loadedDataLines.slice(s, e + 1)
        pendingSaveText = loadedHeaderLine + "\n" + sliced.join("\n") + "\n"

        var baseName = loadedFileName.replace(/\.csv$/i, "")
        saveCsvDialog.currentFile = baseName + "_zoomed.csv"
        saveCsvDialog.open()
    }


    function backupDeviceCsv(fileName) {
        var base = deviceCurrentDir.endsWith("/") ? deviceCurrentDir : (deviceCurrentDir + "/")
        var fullPath = base + fileName

        downloadingFromDevice = true
        downloadProgress = 0
        downloadSpeedKBps = 0
        statusText = "Downloading " + fileName + " for backup..."

        var byteData = mCommands.fileBlockRead(fullPath)
        downloadingFromDevice = false

        if (mCommands.fileBlockDidCancel()) {
            statusText = "Backup download cancelled."
            return
        }

        var text = byteData ? byteData.toString() : ""
        if (!text || text.length === 0) {
            statusText = "Failed to download " + fileName
            return
        }

        loadedCsvText = text
        loadedFileName = fileName
        saveCsvDialog.currentFile = fileName
        saveCsvDialog.open()
    }

    property var recentLogEntries: []

    function toJsArray(seq) {
        var out = []
        if (!seq || typeof seq === "string" || seq.length === undefined) return out
        for (var i = 0; i < seq.length; i++) {
            out.push(seq[i])
        }
        return out
    }

    function recordRecentLocalLog(filePathOrUrl) {
        var cleanPath = filePathOrUrl.toString()
        if (!cleanPath.startsWith("content://") && cleanPath.startsWith("file://")) {
            cleanPath = decodeURIComponent(cleanPath.replace(/^file:\/\//, ""))
        }

        var list = toJsArray(Utility.getSetting("recent_local_logs", []))
        list = list.filter(function(p) { return p !== cleanPath })
        list.unshift(cleanPath)
        if (list.length > 15) list = list.slice(0, 15)

        Utility.setSetting("recent_local_logs", list)
    }

    function openRecentLogsDialog() {
        var combined = []
        var seenPaths = {}

        // Build lookup map of RT logs in path_rt_log so we know which files can be deleted
        var rtMap = {}
        var rtList = []
        var rtDir = Utility.getSetting("path_rt_log", "").toString()
        if (rtDir.length > 0 && !rtDir.startsWith("content://")) {
            var rtFiles = Utility.listFilesInDir(rtDir)
            for (var i = 0; i < rtFiles.length; i++) {
                var item = rtFiles[i]
                item.source = "RT Log"
                item.canDelete = true
                rtMap[item.path] = item
                rtList.push(item)
            }
        }

        // 1. First add recently opened logs in exact "most recently opened" order
        var recentPaths = toJsArray(Utility.getSetting("recent_local_logs", []))
        for (var j = 0; j < recentPaths.length; j++) {
            var p = recentPaths[j]
            if (!p || seenPaths[p]) continue

            if (rtMap[p]) {
                combined.push(rtMap[p])
                seenPaths[p] = true
            } else if (p.startsWith("content://")) {
                var fname = decodeURIComponent(p).split("/").pop()
                combined.push({
                                  name: fname,
                                  path: p,
                                  size: 0,
                                  source: "Local",
                                  canDelete: false
                              })
                seenPaths[p] = true
            } else {
                var info = Utility.getFileInfo(p)
                if (info && info.path) {
                    info.source = "Local"
                    info.canDelete = false
                    combined.push(info)
                    seenPaths[info.path] = true
                }
            }
        }

        // 2. Then append remaining unopened RT logs (already sorted newest-first by QDir::Time)
        for (var k = 0; k < rtList.length; k++) {
            var rtItem = rtList[k]
            if (!seenPaths[rtItem.path]) {
                combined.push(rtItem)
                seenPaths[rtItem.path] = true
            }
        }

        recentLogEntries = combined
        recentLogsDialog.open()
    }

    function clearRecentLogsHistory() {
        Utility.setSetting("recent_local_logs", [])
        openRecentLogsDialog()
        statusText = "Cleared recent log history."
    }

    SheetDialog {
        id: recentLogsDialog
        title: "Recent Logs (Local & RT)"
        acceptText: qsTr("Close")
        rejectText: ""
        fullHeight: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            EmptyLogs {
                visible: root.recentLogEntries.length === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "No recent local or RT logs found."
            }

            ListView {
                id: recentLogsListView
                visible: root.recentLogEntries.length > 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.recentLogEntries

                delegate: LogRow {
                    width: recentLogsListView.width
                    name: modelData.name
                    details: modelData.source + (modelData.size > 0 ? " · " + root.sizeText(modelData.size) : "") +
                             (modelData.timestamp ? " · " + root.dateText(modelData.timestamp) : "")
                    value: modelData.size > 0 ? root.logDistanceText(modelData.path, modelData.size) : ""
                    canDelete: modelData.canDelete
                    onClicked: {
                        var text = Utility.readTextFile(modelData.path)
                        if (text && text.length > 0) {
                            root.loadedFileName = modelData.name
                            root.parseCsvText(text)
                            root.recordRecentLocalLog(modelData.path)
                            recentLogsDialog.close()
                        } else {
                            root.statusText = "Failed to read: " + modelData.name
                        }
                    }
                    onDeleteClicked: root.promptDeleteLocalFile(modelData.name, modelData.path)
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Clear History"
                flat: true
                enabled: root.toJsArray(Utility.getSetting("recent_local_logs", [])).length > 0
                onClicked: root.clearRecentLogsHistory()
            }
        }
    }

    // Faults in the log over the map or the values, so that the graphs stay in view. A tap moves the cursor to one
    Dialog {
        id: faultsDialog
        parent: map.visible ? map : valuesView
        width: parent.width
        height: parent.height
        padding: 8
        topPadding: 0
        modal: false
        closePolicy: Popup.CloseOnEscape

        background: Rectangle {
            color: Utility.getAppHexColor("cardBackground")
            radius: 12
            border.color: Utility.getAppHexColor("raisedBackground")
        }

        header: RowLayout {
            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                text: qsTr("Faults")
                font.pointSize: 13
                font.weight: Font.Medium
                color: Utility.getAppHexColor("titleText")
            }

            ToolButton {
                text: "×"
                font.pixelSize: 18
                Accessible.name: qsTr("Close")
                onClicked: faultsDialog.close()
            }
        }

        ListView {
            anchors.fill: parent
            clip: true
            model: root.faultRuns

            delegate: ItemDelegate {
                readonly property var time: root.faultTimeText(modelData)
                width: ListView.view.width
                highlighted: root.currentIndex >= modelData.start && root.currentIndex <= modelData.end
                onClicked: {
                    if (modelData.start < root.viewStart || modelData.start > root.viewEnd) {
                        root.applyZoomWindow(0, root.rawRows.length - 1, false)
                    }
                    root.currentIndex = modelData.start
                }

                contentItem: GridLayout {
                    columns: 2
                    rowSpacing: 2

                    Text {
                        Layout.fillWidth: true
                        text: root.mCommands.faultToStr(modelData.code)
                        wrapMode: Text.WrapAnywhere
                        font.weight: Font.Medium
                        color: Utility.getAppHexColor("danger")
                    }

                    Text {
                        text: time.at
                        font.weight: Font.Medium
                        color: Utility.getAppHexColor("lightText")
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.columnSpan: 2
                        visible: text !== ""
                        text: Utility.faultToDescription(modelData.code)
                        wrapMode: Text.Wrap
                        color: Utility.getAppHexColor("lightText")
                    }

                    Text {
                        Layout.columnSpan: 2
                        text: root.columnMeta[modelData.col].label + " · " + time.duration
                        font.pointSize: 9
                        color: Utility.getAppHexColor("normalText")
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Utility.getAppHexColor("lightestBackground")
                }
            }
        }
    }

    // Column of a graph line. Logs with many columns are grouped by device or kind.
    SheetDialog {
        id: columnSheet
        property int slot: 0
        property string group: ""
        // Columns without data or that never change are left out
        readonly property var shown: root.columnMeta.filter(c => c.max > c.min)
        readonly property bool grouped: shown.length > 16
        // Device groups first, then the kinds, of the shown columns
        readonly property var chips: {
            var kinds = root.columnKinds.map(kd => kd[0])
            return root.columnGroups.filter(g => kinds.indexOf(g) < 0 && g !== "Other").concat(kinds, "Other")
                .filter(t => shown.some(c => c.tags.indexOf(t) >= 0))
        }
        // Columns in the picked group or kind that have all searched words, in group order when grouped
        readonly property var rows: {
            var terms = columnField.displayText.toLowerCase().split(" ")
            return shown.filter(c => (group === "" || c.tags.indexOf(group) >= 0) &&
                                terms.every(t => [c.label, c.key, c.unit].concat(c.tags).join(" ").toLowerCase().indexOf(t) >= 0))
                .sort((a, b) => (grouped ? root.columnGroups.indexOf(a.group) - root.columnGroups.indexOf(b.group) : 0) || a.index - b.index)
        }

        title: qsTr("Select Value")
        rejectText: ""
        fullHeight: true
        onClosed: {
            group = ""
            columnField.text = ""
            // Or the field takes the focus and the keyboard on the next open
            columnField.focus = false
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            InputField {
                id: columnField
                Layout.fillWidth: true
                placeholderText: qsTr("Search %1 values").arg(columnSheet.shown.length)
                inputMethodHints: Qt.ImhNoPredictiveText
            }

            Flow {
                Layout.fillWidth: true
                visible: columnSheet.grouped
                spacing: 6

                Repeater {
                    model: columnSheet.chips

                    Chip {
                        topInset: 0
                        text: modelData
                        checked: columnSheet.group === modelData
                        onClicked: {
                            Qt.inputMethod.hide()
                            columnSheet.group = checked ? "" : modelData
                        }
                    }
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: columnSheet.rows
                section.property: columnSheet.grouped ? "group" : ""
                section.delegate: SectionHeader {
                    width: ListView.view.width
                    text: section
                }

                delegate: LogRow {
                    width: ListView.view.width
                    // Without the device's word in front, as the section header shows it
                    name: columnSheet.grouped && modelData.group === modelData.word &&
                          root.columnKinds.every(kd => kd[0] !== modelData.group) ?
                              modelData.label.slice(modelData.word.length + 1) || modelData.label : modelData.label
                    readonly property int decimals: root.colDecimals(modelData)
                    details: modelData.min.toFixed(decimals) + " – " + modelData.max.toFixed(decimals) + " " + modelData.unit
                    // A dot in the colour of each line that shows this column
                    value: root.slotColIdx.map((c, s) => c === modelData.index ? "<font color='" + root.slotColors[s] + "'>●</font>" : "").join("")
                    onClicked: {
                        root.setSlotColumn(columnSheet.slot, modelData.index)
                        columnSheet.close()
                    }
                }
            }
        }
    }

    property var trackCoords: []
    property var rawRows: []
    property var columnMeta: []
    property var columnGroups: []
    property int timeColIdx: -1
    property bool hasGps: false
    property int currentIndex: 0
    property int viewStart: 0
    property int viewEnd: 0
    property string statusText: "Open a log from the File menu."
    property bool followMap: true

    property string loadedHeaderLine: ""
    property var loadedDataLines: []
    property string pendingSaveText: ""

    property bool filterOutliers: true
    property real outlierThresholdM: 20.0

    property var slotEnabled: [true, true, true, true]
    property var slotColIdx: [-1, -1, -1, -1]
    property var rangeCache: ({})
    property var slotRanges: [
        { min: 0, max: 1, rawMin: null, rawMax: null },
        { min: 0, max: 1, rawMin: null, rawMax: null },
        { min: 0, max: 1, rawMin: null, rawMax: null },
        { min: 0, max: 1, rawMin: null, rawMax: null }
    ]
    // Darker series colours on the light card
    readonly property var slotColors: Utility.isDarkMode() ? ["#00d4ff", "#ff9900", "#ff4d6d", "#4ade80"] :
                                                             ["#0079a8", "#b85f00", "#d42a4c", "#1c8a4a"]

    // The marker eases after the cursor through the rows in between, so that it stays on the track
    property real markerIndex: currentIndex
    Behavior on markerIndex {
        NumberAnimation { duration: 80; easing.type: Easing.OutQuad }
    }

    // Position on the track at a fractional row, between the position logged before and after it
    function trackCoordAt(index) {
        var n = rawRows.length
        var i = Math.min(Math.floor(index), n - 1)
        while (i >= 0 && rawRows[i].coord === null) i--
        if (i < 0) return trackCoords.length > 0 ? trackCoords[0] : QtPositioning.coordinate(0, 0)

        var a = rawRows[i].coord
        var same = (c) => c === null || (c.latitude === a.latitude && c.longitude === a.longitude)
        var start = i
        while (start > 0 && same(rawRows[start - 1].coord)) start--
        var next = i + 1
        while (next < n && same(rawRows[next].coord)) next++
        if (next >= n) return a

        var b = rawRows[next].coord
        var t = Math.min(1, (index - start) / (next - start))
        return QtPositioning.coordinate(a.latitude + (b.latitude - a.latitude) * t,
                                        a.longitude + (b.longitude - a.longitude) * t)
    }

    property var currentMapCoord: hasGps && rawRows.length > 0 ? trackCoordAt(markerIndex) :
                                                                 QtPositioning.coordinate(0, 0)

    // Direction of travel at the marker in degrees from north, towards the next logged position.
    // Positions closer than 5 m are skipped as their direction is mostly GPS noise.
    property real currentHeading: {
        if (!hasGps || rawRows.length === 0) return 0
        var cur = currentMapCoord
        var row = Math.min(Math.floor(markerIndex), rawRows.length - 1)
        for (var i = row + 1; i < rawRows.length; i++) {
            var next = rawRows[i].coord
            if (next !== null && cur.distanceTo(next) > 5) return cur.azimuthTo(next)
        }
        for (var j = row; j >= 0; j--) {
            var prev = rawRows[j].coord
            if (prev !== null && prev.distanceTo(cur) > 5) return prev.azimuthTo(cur)
        }
        return 0
    }

    function updateStatusSummary() {
        var data = rawRows
        var timeIdx = timeColIdx
        if (data.length === 0) return
        var s = Math.max(0, Math.min(viewStart, data.length - 1))
        var e = Math.max(s, Math.min(viewEnd, data.length - 1))

        var distM = 0.0
        var firstTime = null
        var lastTime = null

        if (hasGps) {
            for (var i = s; i <= e; i++) {
                if (data[i].coord !== null) {
                    distM = data[e].distance - data[i].distance
                    break
                }
            }
        }
        if (timeIdx !== -1) {
            for (var i = s; i <= e; i++) {
                if (data[i].vals[timeIdx] !== null) {
                    firstTime = data[i].vals[timeIdx]
                    break
                }
            }
            for (var i = e; i >= s; i--) {
                if (data[i].vals[timeIdx] !== null) {
                    lastTime = data[i].vals[timeIdx]
                    break
                }
            }
        }

        var distStr = distanceText(distM)
        var durStr = ""
        if (firstTime !== null && lastTime !== null && lastTime >= firstTime) {
            var secs = Math.round(lastTime - firstTime)
            var mins = Math.floor(secs / 60)
            var remS = secs % 60
            durStr = " | " + mins + "m " + (remS < 10 ? "0" : "") + remS + "s"
        }

        var countStr = " (" + (e - s + 1) + " pts)"
        statusText = hasGps ? (distStr + durStr + countStr) : ((e - s + 1) + " pts" + durStr + " (no GPS)")
    }

    function updateVisibleRoute() {
        routeLine.path = []
        if (!hasGps || rawRows.length === 0) return
        var s = Math.max(0, Math.min(viewStart, rawRows.length - 1))
        var e = Math.max(s, Math.min(viewEnd, rawRows.length - 1))
        for (var i = s; i <= e; i++) {
            if (rawRows[i].coord !== null) routeLine.addCoordinate(rawRows[i].coord)
        }
    }

    function fitRouteBounds() {
        if (!hasGps || rawRows.length === 0) return
        if (!map.visible) {
            routeFitPending = true
            return
        }
        var visibleCoords = []
        var s = Math.max(0, Math.min(viewStart, rawRows.length - 1))
        var e = Math.max(s, Math.min(viewEnd, rawRows.length - 1))
        for (var i = s; i <= e; i++) {
            if (rawRows[i].coord !== null) visibleCoords.push(rawRows[i].coord)
        }
        if (visibleCoords.length === 0) visibleCoords = trackCoords
        if (visibleCoords.length === 0) return
        if (visibleCoords.length === 1) {
            map.center = visibleCoords.slice(0, 1).pop()
            map.zoomLevel = 16
            return
        }
        map.fitViewportToGeoShape(QtPositioning.rectangle(visibleCoords), Qt.size(40, 40))
    }

    // Eases the map to where moveMap() puts it. restart() sets the map back to the start before it is drawn.
    function easeMap(moveMap) {
        centerAnim.from = map.center
        zoomAnim.from = map.zoomLevel
        moveMap()
        centerAnim.to = map.center
        zoomAnim.to = map.zoomLevel
        mapAnim.restart()
    }

    // Zoom before center, as the map limits the center latitude by the zoom level
    ParallelAnimation {
        id: mapAnim
        NumberAnimation { id: zoomAnim; target: map; property: "zoomLevel"; duration: 300; easing.type: Easing.OutCubic }
        CoordinateAnimation { id: centerAnim; target: map; property: "center"; duration: 300; easing.type: Easing.OutCubic }
    }

    function applyZoomWindow(startIdx, endIdx, fitMap) {
        if (rawRows.length === 0) return
        var maxIdx = rawRows.length - 1
        var s = Math.max(0, Math.min(Math.round(startIdx), maxIdx))
        var e = Math.max(s, Math.min(Math.round(endIdx), maxIdx))
        viewStart = s
        viewEnd = e
        zoomRange.first.value = s
        zoomRange.second.value = e

        if (currentIndex < s) currentIndex = s
        if (currentIndex > e) currentIndex = e

        updateVisibleRoute()
        refreshRanges()
        updateStatusSummary()
        if (fitMap && hasGps) fitRouteBounds()
        computeZoomedStats()
    }

    function zoomAroundCursor(factor) {
        if (rawRows.length < 2) return
        var maxIdx = rawRows.length - 1
        var curSpan = Math.max(10, viewEnd - viewStart)
        var newSpan = Math.max(10, Math.min(maxIdx, Math.round(curSpan * factor)))
        var ratio = curSpan > 0 ? (currentIndex - viewStart) / curSpan : 0.5
        var newStart = Math.round(currentIndex - ratio * newSpan)
        var newEnd = newStart + newSpan
        if (newStart < 0) { newEnd -= newStart; newStart = 0 }
        if (newEnd > maxIdx) { newStart = Math.max(0, newStart - (newEnd - maxIdx)); newEnd = maxIdx }
        applyZoomWindow(newStart, newEnd, false)
    }

    function jumpToNearestCoord(targetCoord) {
        if (!hasGps || rawRows.length === 0) return
        var bestIdx = -1, bestDist = Infinity
        for (var i = viewStart; i <= viewEnd; i++) {
            var c = rawRows[i].coord
            if (c !== null) {
                var d = c.distanceTo(targetCoord)
                if (d < bestDist) { bestDist = d; bestIdx = i }
            }
        }
        if (bestIdx !== -1) {
            root.currentIndex = bestIdx
        }
    }

    function computeColRange(colIdx) {
        var data = rawRows
        if (colIdx < 0 || data.length === 0) return { min: 0, max: 1, rawMin: null, rawMax: null }
        var blocks = rangeCache[colIdx]
        if (blocks === undefined) { blocks = []; rangeCache[colIdx] = blocks }
        var blockSize = 256
        var minV = Infinity, maxV = -Infinity
        var s = Math.max(0, viewStart), e = Math.min(data.length - 1, viewEnd)
        for (var i = s; i <= e;) {
            if (i % blockSize === 0 && i + blockSize - 1 <= e) {
                var b = i / blockSize
                var block = blocks[b]
                if (block === undefined) {
                    block = { min: Infinity, max: -Infinity }
                    for (var j = i; j < i + blockSize; j++) {
                        var v = data[j].vals[colIdx]
                        if (v !== null && !isNaN(v)) {
                            if (v < block.min) block.min = v
                            if (v > block.max) block.max = v
                        }
                    }
                    blocks[b] = block
                }
                if (block.min < minV) minV = block.min
                if (block.max > maxV) maxV = block.max
                i += blockSize
            } else {
                var v = data[i].vals[colIdx]
                if (v !== null && !isNaN(v)) {
                    if (v < minV) minV = v
                    if (v > maxV) maxV = v
                }
                i++
            }
        }
        if (minV === Infinity) return { min: 0, max: 1, rawMin: null, rawMax: null }
        var span = maxV - minV
        var pad = span === 0 ? 1 : span * 0.08
        return { min: minV - pad, max: maxV + pad, rawMin: minV, rawMax: maxV }
    }

    function refreshRanges() {
        var r = []
        for (var s = 0; s < 4; s++) r.push(computeColRange(slotColIdx[s]))
        slotRanges = r
        chartLines.requestPaint()
        chartCursor.requestPaint()
    }

    function setSlotColumn(slot, colIdx) {
        var idxs = slotColIdx.slice()
        idxs[slot] = colIdx
        slotColIdx = idxs
        refreshRanges()
    }

    function toggleSlot(slot, enabled) {
        var en = slotEnabled.slice()
        en[slot] = enabled
        slotEnabled = en
        chartLines.requestPaint()
        chartCursor.requestPaint()
    }

    // Names and units for the VESC Tool RT log, which has only keys in its header, as in the desktop log analysis
    readonly property var rtLogColumns: ({
        "ms_today": ["Time", "ms"], "input_voltage": ["Input Voltage", "V"],
        "temp_mos_max": ["Temp MOSFET", "°C"], "temp_mos_1": ["Temp MOSFET 1", "°C"],
        "temp_mos_2": ["Temp MOSFET 2", "°C"], "temp_mos_3": ["Temp MOSFET 3", "°C"],
        "temp_motor": ["Temp Motor", "°C"], "current_motor": ["Current Motor", "A"],
        "current_in": ["Current Battery", "A"], "d_axis_current": ["id", "A"], "q_axis_current": ["iq", "A"],
        "erpm": ["ERPM", ""], "duty_cycle": ["Duty", "%"], "amp_hours_used": ["Ah Used", "Ah"],
        "amp_hours_charged": ["Ah Charged", "Ah"], "watt_hours_used": ["Wh Used", "Wh"],
        "watt_hours_charged": ["Wh Charged", "Wh"], "tachometer": ["Tachometer", ""],
        "tachometer_abs": ["Tachometer ABS", ""], "encoder_position": ["Motor Pos", "°"],
        "fault_code": ["Fault Code", ""], "vesc_id": ["VESC ID", ""], "d_axis_voltage": ["vd", "V"],
        "q_axis_voltage": ["vq", "V"], "ms_today_setup": ["Time Setup", "ms"],
        "amp_hours_setup": ["Ah Used Setup", "Ah"], "amp_hours_charged_setup": ["Ah Charged Setup", "Ah"],
        "watt_hours_setup": ["Wh Used Setup", "Wh"], "watt_hours_charged_setup": ["Wh Charged Setup", "Wh"],
        "battery_level": ["Battery Level", "%"], "battery_wh_tot": ["Battery Wh", "Wh"],
        "current_in_setup": ["Current Battery Setup", "A"], "current_motor_setup": ["Current Motors Setup", "A"],
        "speed_meters_per_sec": ["Speed VESC", "m/s"], "tacho_meters": ["Trip VESC", "m"],
        "tacho_abs_meters": ["Trip VESC ABS", "m"], "num_vescs": ["VESC num", ""], "ms_today_imu": ["Time IMU", "ms"],
        "roll": ["Roll", "rad"], "pitch": ["Pitch", "rad"], "yaw": ["Yaw", "rad"],
        "accx": ["Accel X", "G"], "accy": ["Accel Y", "G"], "accz": ["Accel Z", "G"],
        "gyrox": ["Gyro X", "°/s"], "gyroy": ["Gyro Y", "°/s"], "gyroz": ["Gyro Z", "°/s"],
        "gnss_postime": ["Time GNSS", "ms"], "gnss_lat": ["Latitude", "°"], "gnss_lon": ["Longitude", "°"],
        "gnss_alt": ["Altitude", "m"], "gnss_gvel": ["Speed GNSS", "m/s"], "gnss_vvel": ["V. Speed GNSS", "m/s"],
        "gnss_hacc": ["H. Accuracy GNSS", "m"], "gnss_vacc": ["V. Accuracy GNSS", "m"]
    })

    // What a column measures, all that match its words and [unit]
    readonly property var columnKinds: [
        ["GPS", /gnss|gps|\b(lat|lon|alt)(itude|gitude)?\b/],
        ["Time", /\b(time|ms|t day)\b|\[m?s\]/],
        ["IMU", /\b(imu|roll|pitch|yaw|gyro|accel)/],
        ["Status", /fault|\bok\b|\[(bool|cnt)\]/],
        ["Temperature", /\btemp|\[°c\]/],
        ["Energy", /hours|\b[aw]h\b/],
        ["Power", /power|\[w\]/],
        ["Current", /current|\[a\]/],
        ["Voltage", /volt|\bv in\b|\[v\]/],
        ["Speed", /speed|rpm|kmh|\bvel|\[(km\/h|mph)\]/],
        ["Distance", /\b(dist|trip|tacho|odo)/],
        ["Battery", /batt|\bsoc\b/]
    ]

    function parseHeaderMeta(hLine, extraCol) {
        var delim = hLine.indexOf(";") !== -1 ? ";" : (hLine.indexOf(",") !== -1 ? "," : "\t")
        var rawCols = hLine.split(delim)
        var imperial = VescIf.useImperialUnits()
        // RT logs end the header with the delimiter
        if (rawCols[rawCols.length - 1].trim() === "") rawCols.pop()
        if (extraCol) rawCols.push(extraCol)
        var cols = []
        rawCols.forEach(function(raw, i) {
            var p = raw.trim().split(":")
            var k = (p.shift() || "").trim()
            var lbl = (p.length > 0 ? p.shift() : k).trim()
            var u = (p.length > 0 ? p.shift() : "").trim()
            var scale = 1
            var rt = raw.indexOf(":") === -1 ? rtLogColumns[k.toLowerCase()] : undefined
            if (rt) {
                lbl = rt[0]
                u = rt[1]
                // The RT log has the duty cycle and battery level as fractions
                if (u === "%") scale = 100
            }
            u = ({ "degC": "°C", "deg": "°" })[u] || u
            // Speeds are shown in km/h or mph
            var toKmh = ({ "m/s": 3.6, "km/h": 1, "kmh": 1, "kph": 1, "mph": 1.609344 })[u.toLowerCase()]
            if (toKmh) {
                scale = imperial ? toKmh / 1.609344 : toKmh
                u = imperial ? "mph" : "km/h"
            }
            // Words of the label and key and the unit, like "h. speed gnss gnss h vel [km/h]"
            var text = (lbl.replace(/([a-z])([A-Z])/g, "$1 $2") + " " + k + " [" + u + "]").replace(/_/g, " ").toLowerCase()
            var kinds = columnKinds.filter(kd => kd[1].test(text)).map(kd => kd[0])
            // Distances are shown in km or mi
            var toKm = kinds.indexOf("Distance") >= 0 ? ({ "m": 0.001, "km": 1, "mi": 1.609344 })[u] : 0
            if (toKm) {
                scale = imperial ? toKm / 1.609344 : toKm
                u = imperial ? "mi" : "km"
            }
            cols.push({ index: i, key: k.toLowerCase(), label: lbl, unit: u, scale: scale, min: Infinity, max: -Infinity,
                        kind: kinds.length > 0 ? kinds[0] : "Other", kinds: kinds, word: lbl.split(/[ _]/)[0] })
        })

        // LogUI names motor controllers on CAN V<CAN ID>. They become M1, M2... in CAN ID order,
        // after the motors the log already numbers and the logger's own motor.
        var motors = cols.reduce((n, c) => Math.max(n, /^M\d+$/.test(c.word) ? parseInt(c.word.slice(1)) : 0),
                                 cols.some(c => c.key === "current in") ? 1 : 0)
        var canIds = cols.map(c => /^V(\d+)$/.exec(c.word)).filter(v => v).map(v => parseInt(v[1]))
            .filter((id, i, ids) => ids.indexOf(id) === i).sort((a, b) => a - b)
        cols.forEach(function(c) {
            var v = /^V(\d+)$/.exec(c.word)
            if (v) {
                c.word = "M" + (motors + canIds.indexOf(parseInt(v[1])) + 1)
                c.label = c.word + c.label.slice(v[0].length)
            }
        })

        // A first word that three or more labels share groups them, like BMS or M1, unless they
        // all measure the same known kind. The other columns are grouped by kind, Other last.
        // Filtering finds a column by its group and by every kind it measures.
        var groups = []
        cols.forEach(function(c) {
            var kinds = cols.filter(o => o.word === c.word).map(o => o.kind)
            c.group = kinds.length >= 3 && !kinds.every(x => x === kinds[0] && x !== "Other") ? c.word : c.kind
            c.tags = [c.group].concat(c.kinds)
            if (groups.indexOf(c.group) < 0) groups.push(c.group)
        })
        groups = groups.filter(g => g !== "Other").concat(groups.filter(g => g === "Other"))
        return { delim: delim, cols: cols, groups: groups }
    }

    function findColKeyIdx(colsOrCandidates, maybeCandidates) {
        var cols = maybeCandidates ? colsOrCandidates : columnMeta
        var candidates = maybeCandidates ? maybeCandidates : colsOrCandidates
        if (!cols || !candidates) return -1

        for (var c = 0; c < candidates.length; c++) {
            var target = String(candidates[c]).toLowerCase()
            for (var i = 0; i < cols.length; i++) {
                var item = cols[i]
                if (!item) continue
                if (typeof item === "string") {
                    if (item.toLowerCase() === target) return i
                } else {
                    var k = (typeof item.key === "string") ? item.key.toLowerCase() : ""
                    var lbl = (typeof item.label === "string") ? item.label.toLowerCase() : ""
                    if (k === target || lbl === target) return i
                }
            }
        }
        return -1
    }

    function loadCsv(fileUrl) {
        var text = Utility.readTextFile(fileUrl.toString())
        if (!text || text.length === 0) { statusText = "Failed to read file."; return }
        recordRecentLocalLog(fileUrl)
        loadedFileName = decodeURIComponent(fileUrl.toString()).split("/").pop() || "vesc_log.csv"
        parseCsvText(text)
    }

    function reparseWithFilter() {
        if (loadedCsvText.length === 0) return
        var prevViewStart = viewStart
        var prevViewEnd = viewEnd
        var prevCur = currentIndex
        var prevSlots = slotColIdx.slice()

        parseCsvText(loadedCsvText)

        // Preserve active columns & zoom range when toggling/tuning the filter
        slotColIdx = prevSlots
        refreshRanges()
        applyZoomWindow(prevViewStart, prevViewEnd, false)
        currentIndex = Math.max(viewStart, Math.min(viewEnd, prevCur))
    }

    function parseCsvText(text) {
        if (!text || text.length === 0) { statusText = "Failed to read file."; return }
        loadedCsvText = text

        var allLines = text.trim().split(/\r?\n/)
        if (allLines.length < 2) { statusText = "CSV file has no data rows."; return }

        loadedHeaderLine = allLines.shift()
        var cleanLines = []
        for (var j = 0; j < allLines.length; j++) {
            if (allLines[j].trim().length > 0) cleanLines.push(allLines[j])
        }

        var meta = parseHeaderMeta(loadedHeaderLine)
        // A log cut off by a power loss ends in a partial line
        if (cleanLines.length > 1 && cleanLines[cleanLines.length - 1].split(meta.delim).length <
                loadedHeaderLine.split(meta.delim).length) {
            cleanLines.pop()
        }
        loadedDataLines = cleanLines
        var latIdx = findColKeyIdx(meta.cols, ["gnss_lat", "lat", "latitude"])
        var lonIdx = findColKeyIdx(meta.cols, ["gnss_lon", "lon", "lng", "longitude"])
        // Distance along the GPS track, to compare with the distances in the log
        var tripCol = null
        if (latIdx !== -1 && lonIdx !== -1 && findColKeyIdx(meta.cols, ["trip_gnss"]) === -1) {
            meta = parseHeaderMeta(loadedHeaderLine, "trip_gnss:Trip GNSS:m")
            tripCol = meta.cols[meta.cols.length - 1]
        }

        // Pass 1: Parse values and collect unique GPS fixes (collapsing repeated coordinates)
        var rawParsed = []
        var uniqueGps = []
        var lastRawLat = null, lastRawLon = null

        for (var i = 0; i < cleanLines.length; i++) {
            var parts = cleanLines[i].split(meta.delim)
            var vals = new Array(meta.cols.length)
            for (var c = 0; c < meta.cols.length; c++) {
                if (c < parts.length && parts[c].trim().length > 0) {
                    var n = parseFloat(parts[c]) * meta.cols[c].scale
                    vals[c] = isNaN(n) ? null : n
                    if (n < meta.cols[c].min) meta.cols[c].min = n
                    if (n > meta.cols[c].max) meta.cols[c].max = n
                } else {
                    vals[c] = null
                }
            }

            var uniqueIdx = -1
            if (latIdx !== -1 && lonIdx !== -1 && vals[latIdx] !== null && vals[lonIdx] !== null) {
                var lat = vals[latIdx], lon = vals[lonIdx]
                if ((lat !== 0 || lon !== 0) && Math.abs(lat) <= 90 && Math.abs(lon) <= 180) {
                    if (lat !== lastRawLat || lon !== lastRawLon) {
                        uniqueGps.push(QtPositioning.coordinate(lat, lon))
                        lastRawLat = lat
                        lastRawLon = lon
                    }
                    uniqueIdx = uniqueGps.length - 1
                }
            }
            rawParsed.push({ uniqueIdx: uniqueIdx, vals: vals })
        }

        // Pass 2: Optional distance-only outlier filter across unique GPS fixes
        var uniqueValid = new Array(uniqueGps.length)
        var anchorCoord = null
        var thresh = outlierThresholdM

        for (var u = 0; u < uniqueGps.length; u++) {
            var cand = uniqueGps[u]
            if (!filterOutliers) {
                uniqueValid[u] = cand
            } else if (anchorCoord === null) {
                var closeCount = 0
                for (var k = u + 1; k < Math.min(uniqueGps.length, u + 5); k++) {
                    if (cand.distanceTo(uniqueGps[k]) < thresh) closeCount++
                }
                if (closeCount >= 2 || uniqueGps.length <= 3) {
                    uniqueValid[u] = cand
                    anchorCoord = cand
                } else {
                    uniqueValid[u] = null
                }
            } else {
                var step = anchorCoord.distanceTo(cand)
                if (step < thresh) {
                    uniqueValid[u] = cand
                    anchorCoord = cand
                } else {
                    var ok1 = (u + 1 < uniqueGps.length) && (cand.distanceTo(uniqueGps[u + 1]) < thresh)
                    var ok2 = (u + 2 < uniqueGps.length) && (cand.distanceTo(uniqueGps[u + 2]) < thresh)
                    if (ok1 && ok2) {
                        uniqueValid[u] = cand
                        anchorCoord = cand
                    } else {
                        uniqueValid[u] = null
                    }
                }
            }
        }

        // Pass 3: Map filtered coordinates back to all rows & accumulate distance
        var coords = [], rows = []
        var distance = 0.0, prevCoord = null
        for (var r = 0; r < rawParsed.length; r++) {
            var uIdx = rawParsed[r].uniqueIdx
            var coord = (uIdx >= 0) ? uniqueValid[uIdx] : null
            if (coord !== null) {
                coords.push(coord)
                if (prevCoord !== null) {
                    var d = prevCoord.distanceTo(coord)
                    if (d < 100) distance += d
                }
                prevCoord = coord
            }
            if (tripCol) rawParsed[r].vals[tripCol.index] = distance * tripCol.scale
            rows.push({ coord: coord, vals: rawParsed[r].vals, distance: distance })
        }
        if (tripCol) {
            tripCol.min = 0
            tripCol.max = distance * tripCol.scale
        }

        // Runs of rows with the same fault, from each column that logs one. Empty cells keep the
        // last value. LogUI logs the same fault as both fault and Fault, so a label is used once.
        var faults = [], faultLabels = []
        meta.cols.forEach(function(col) {
            var label = col.label.toLowerCase()
            if (!/fault/.test(col.key + " " + label) || faultLabels.indexOf(label) >= 0) return
            faultLabels.push(label)
            var code = 0, run = null
            for (var r = 0; r < rows.length; r++) {
                if (rows[r].vals[col.index] !== null) code = Math.round(rows[r].vals[col.index])
                if (run && code !== run.code) { faults.push(run); run = null }
                if (!run && code !== 0) run = { col: col.index, code: code, start: r }
                if (run) run.end = r
            }
            if (run) faults.push(run)
        })

        if (rows.length === 0) { statusText = "No valid rows found."; return }

        columnMeta = meta.cols
        columnGroups = meta.groups
        rawRows = rows
        trackCoords = coords
        hasGps = coords.length > 0
        faults.forEach(f => f.coord = hasGps ? trackCoordAt(f.start) : null)
        faultRuns = faults.sort((a, b) => a.start - b.start)
        viewStart = 0
        viewEnd = rows.length - 1
        currentIndex = 0
        zoomRange.first.value = 0
        zoomRange.second.value = rows.length - 1
        updateVisibleRoute()
        var d0 = findColKeyIdx(meta.cols, ["gnss_h_vel", "speed_meters_per_sec", "impeller rpm", "m1 erpm", "kmh_vesc", "m1 rpm"])
        var d1 = findColKeyIdx(meta.cols, ["bms current in", "current in", "m1 current in", "current_in", "impeller power"])
        var d2 = findColKeyIdx(meta.cols, ["current", "m1 current", "current_motor", "jet temp gearbox"])
        var d3 = findColKeyIdx(meta.cols, ["voltage in", "input_voltage", "bms soc", "input voltage", "m1 input voltage"])
        var maxIdx = Math.max(0, meta.cols.length - 1)
        slotColIdx = [
                    d0 !== -1 ? d0 : 0,
                    d1 !== -1 ? d1 : Math.min(1, maxIdx),
                    d2 !== -1 ? d2 : Math.min(2, maxIdx),
                    d3 !== -1 ? d3 : Math.min(3, maxIdx)
                ]
        refreshRanges()
        updateStatusSummary()
        if (hasGps) fitRouteBounds()
        computeZoomedStats()
    }

    // Value of a column at the cursor, from the last row up to it that has one
    function cursorValue(colIdx) {
        // The tiles update while a log loads, before rawRows and currentIndex do
        for (var i = Math.min(currentIndex, rawRows.length - 1); i >= 0; i--) {
            var v = rawRows[i].vals[colIdx]
            if (v !== null && !isNaN(v)) return v
        }
        return null
    }

    // Enough decimals to tell min from max, like 57.4886 – 57.4926 °
    function colDecimals(col) {
        return col.max > col.min ? Math.min(7, Math.max(1, 1 - Math.floor(Math.log10(col.max - col.min)))) :
                                   Number.isInteger(col.max) ? 0 : 1
    }

    function latestSlotVal(slot) {
        var colIdx = slotColIdx[slot]
        if (colIdx < 0 || rawRows.length === 0 || colIdx >= columnMeta.length) return ""
        var unit = columnMeta[colIdx].unit
        var v = cursorValue(colIdx)
        return v === null ? "N/A" : v.toFixed(1) + (unit ? " " + unit : "")
    }

    function slotMinMaxText(slot) {
        var r = slotRanges[slot]
        if (!r || r.rawMin === null || r.rawMax === null) return "Min: -  Max: -"
        return "Min: " + r.rawMin.toFixed(1) + "  Max: " + r.rawMax.toFixed(1)
    }

    property var zoomedStats: []
    property bool showMapStats: true
    // The values at the cursor in place of the map
    property bool showValues: false
    // The hidden map keeps the size it had, so the route is fitted when it is shown again
    property bool routeFitPending: false

    // Column, fault code, first and last row and position of each run of rows with a fault
    property var faultRuns: []
    property bool showFaults: true
    onShowFaultsChanged: chartLines.requestPaint()

    // When a fault began from the start of the log and how long it lasted, in rows without a time column
    function faultTimeText(run) {
        var t = findColKeyIdx(["ms_today", "t_day", "esc uptime", "jet uptime", "time"])
        var next = Math.min(run.end + 1, rawRows.length - 1)
        if (t === -1 || [0, run.start, next].some(i => rawRows[i].vals[t] === null)) {
            return { at: "#" + (run.start + 1), duration: (run.end - run.start + 1) + " rows" }
        }
        var k = columnMeta[t].key.indexOf("ms_") === 0 ? 1000 : 1
        var sec = (a, b) => ((rawRows[b].vals[t] - rawRows[a].vals[t]) / k + 86400) % 86400
        var at = sec(0, run.start)
        return { at: Math.floor(at / 60) + ":" + ("0" + Math.floor(at % 60)).slice(-2),
                 duration: sec(run.start, next).toFixed(1) + " s" }
    }

    function computeZoomedStats() {
        if (rawRows.length === 0) {
            zoomedStats = []
            return
        }

        var s = Math.max(0, Math.min(viewStart, rawRows.length - 1))
        var e = Math.max(s, Math.min(viewEnd, rawRows.length - 1))
        // Logs leave a cell empty when its value was not updated, so each end takes the nearest value
        function edgeRow(from, step) {
            var vals = rawRows[from].vals.slice()
            for (var c = 0; c < vals.length; c++) {
                for (var i = from + step; vals[c] === null && i >= s && i <= e && Math.abs(i - from) < 100; i += step) {
                    vals[c] = rawRows[i].vals[c]
                }
            }
            return { distance: rawRows[from].distance, vals: vals }
        }
        var rStart = edgeRow(s, 1)
        var rEnd = edgeRow(e, -1)
        var items = []

        // 1. Samples
        items.push({ label: "Samples", value: (e - s + 1) + " / " + rawRows.length })

        // 2. Distance (GPS cumulative distance, tacho_meters, or trip_vesc)
        var gpsDistM = (rEnd.distance || 0) - (rStart.distance || 0)
        var tachoIdx = findColKeyIdx(["tacho_meters", "tacho_abs_meters", "trip_vesc", "trip_vesc_abs"])
        var distM = gpsDistM
        if (distM <= 0 && tachoIdx !== -1 && rStart.vals[tachoIdx] !== null && rEnd.vals[tachoIdx] !== null) {
            distM = Math.max(0, rEnd.vals[tachoIdx] - rStart.vals[tachoIdx]) / columnMeta[tachoIdx].scale
        }
        if (distM > 0) {
            items.push({
                           label: "Distance",
                           value: distanceText(distM)
                       })
        }

        // 3. Duration (supports ms_today in ms, or t_day / uptime in seconds)
        var tIdx = findColKeyIdx(["ms_today", "t_day", "esc uptime", "jet uptime", "time"])
        var dtSec = 0
        if (tIdx !== -1 && tIdx < columnMeta.length && rStart.vals[tIdx] !== null && rEnd.vals[tIdx] !== null) {
            var tMeta = columnMeta[tIdx]
            var tKey = (typeof tMeta.key === "string") ? tMeta.key.toLowerCase() : ""
            var rawDt = rEnd.vals[tIdx] - rStart.vals[tIdx]
            if (rawDt < 0 && tKey.indexOf("ms_today") !== -1) {
                rawDt += 86400000 // Midnight rollover
            }
            dtSec = (tKey.indexOf("ms_") === 0) ? (rawDt / 1000.0) : rawDt
            if (dtSec > 0) {
                var mins = Math.floor(dtSec / 60)
                var secs = Math.floor(dtSec % 60)
                items.push({
                               label: "Duration",
                               value: mins > 0 ? (mins + "m " + secs + "s") : (dtSec.toFixed(1) + "s")
                           })
                // Rows between the ones the time was taken from
                var r0 = s, r1 = e
                while (rawRows[r0].vals[tIdx] === null) r0++
                while (rawRows[r1].vals[tIdx] === null) r1--
                items.push({ label: "Sample Rate", value: ((r1 - r0) / dtSec).toFixed(1) + " Hz" })
            }
        }

        // 4. Average & Max Speed, in the unit the speed columns were converted to while parsing
        var imperial = VescIf.useImperialUnits()
        var spdIdx = findColKeyIdx(["speed_meters_per_sec", "gnss_gvel", "gnss_h_vel", "kmh_vesc", "speed vesc", "speed"])
        var maxSpd = 0, sumSpd = 0, spdCount = 0
        if (spdIdx !== -1) {
            for (var i = s; i <= e; i++) {
                var sv = rawRows[i].vals[spdIdx]
                if (sv !== null && !isNaN(sv)) {
                    var spd = Math.abs(sv)
                    sumSpd += spd
                    spdCount++
                    if (spd > maxSpd) maxSpd = spd
                }
            }
        }
        var avgSpd = spdCount > 0 ? (sumSpd / spdCount)
                                  : ((distM > 0 && dtSec > 0) ? (distM / dtSec * (imperial ? 2.236936 : 3.6)) : 0)
        if (avgSpd > 0) {
            items.push({ label: "Avg Speed", value: avgSpd.toFixed(1) + (imperial ? " mph" : " km/h") })
        }
        if (maxSpd > 0) {
            items.push({ label: "Max Speed", value: maxSpd.toFixed(1) + (imperial ? " mph" : " km/h") })
        }

        // 5. Energy Consumed (Wh net on one row, Regen on its own row when present)
        var whUsedIdx = findColKeyIdx(["watt_hours_used", "watt_hours_setup", "cnt_wh", "watt hours", "wh_used"])
        var whChgIdx  = findColKeyIdx(["watt_hours_charged", "watt_hours_charged_setup", "cnt_wh_chg", "wh chg", "wh_charged"])
        if (whUsedIdx !== -1 && rStart.vals[whUsedIdx] !== null && rEnd.vals[whUsedIdx] !== null) {
            var whUsed = Math.max(0, rEnd.vals[whUsedIdx] - rStart.vals[whUsedIdx])
            var whChg = (whChgIdx !== -1 && rStart.vals[whChgIdx] !== null && rEnd.vals[whChgIdx] !== null)
                    ? Math.max(0, rEnd.vals[whChgIdx] - rStart.vals[whChgIdx]) : 0
            var netWh = whUsed - whChg
            items.push({
                           label: "Energy",
                           value: netWh.toFixed(1) + " Wh"
                       })
            if (whChg > 0.05) {
                items.push({
                               label: "Regen",
                               value: whChg.toFixed(1) + " Wh"
                           })
            }
            if (distM >= 50) {
                items.push({
                               label: "Efficiency",
                               value: (netWh / (distM / (imperial ? 1609.344 : 1000.0))).toFixed(1) + (imperial ? " Wh/mi" : " Wh/km")
                           })
            }
        }

        // 6. Capacity Consumed (Ah)
        var ahUsedIdx = findColKeyIdx(["amp_hours_used", "amp_hours_setup", "cnt_ah", "amp hours", "ah_used"])
        var ahChgIdx  = findColKeyIdx(["amp_hours_charged", "amp_hours_charged_setup", "cnt_ah_chg", "ah chg", "ah_charged"])
        if (ahUsedIdx !== -1 && rStart.vals[ahUsedIdx] !== null && rEnd.vals[ahUsedIdx] !== null) {
            var ahUsed = Math.max(0, rEnd.vals[ahUsedIdx] - rStart.vals[ahUsedIdx])
            var ahChg = (ahChgIdx !== -1 && rStart.vals[ahChgIdx] !== null && rEnd.vals[ahChgIdx] !== null)
                    ? Math.max(0, rEnd.vals[ahChgIdx] - rStart.vals[ahChgIdx]) : 0
            items.push({ label: "Capacity", value: (ahUsed - ahChg).toFixed(2) + " Ah" })
        }

        // 7. Peak Electrical Power (V * I_in)
        var vIdx = findColKeyIdx(["input_voltage", "input voltage", "bms_v_tot", "v_in"])
        var iIdx = findColKeyIdx(["current_in", "current in", "current_in_setup", "bms_i_in_ic", "i_in"])
        if (vIdx !== -1 && iIdx !== -1) {
            var maxW = 0
            for (var p = s; p <= e; p++) {
                var vv = rawRows[p].vals[vIdx]
                var ii = rawRows[p].vals[iIdx]
                if (vv !== null && ii !== null) {
                    var w = vv * ii
                    if (w > maxW) maxW = w
                }
            }
            if (maxW > 0) {
                items.push({ label: "Peak Power", value: Math.round(maxW) + " W" })
            }
        }

        // 8. Faults in view, the row opens the list
        var faultCount = faultRuns.filter(f => f.end >= s && f.start <= e).length
        if (faultCount > 0) {
            items.push({ label: "Faults", value: faultCount + " →" })
        }

        zoomedStats = items
    }


    onRawRowsChanged: rangeCache = ({})

    onCurrentIndexChanged: chartCursor.requestPaint()

    Connections {
        target: VescIf

        function onUseImperialUnitsChanged() {
            root.reparseWithFilter()
        }
    }

    onCurrentMapCoordChanged: {
        if (followMap && hasGps && rawRows.length > 0) {
            var c = currentMapCoord
            if (c.isValid && (c.latitude !== 0 || c.longitude !== 0)) {
                mapAnim.stop()
                map.center = c
            }
        }
    }

    FileDialog {
        id: csvDialog
        title: "Select VESC CSV Log"
        nameFilters: Qt.platform.os === "android"
                     ? ["All files (*)"]
                     : ["CSV files (*.csv)", "All files (*)"]
        onAccepted: root.loadCsv(selectedFile)
    }
    Plugin {
        id: mapPlugin
        name: "osm"
        PluginParameter {
            name: "osm.mapping.custom.host"
            value: "http://tiles.vedder.se/"
        }
        PluginParameter {
            name: "osm.useragent"
            value: "VescToolCustomLogViewer/1.0"
        }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: root.wide ? 2 : 1
        rows: 2
        flow: root.headerOverMap ? GridLayout.TopToBottom : GridLayout.LeftToRight
        rowSpacing: 8
        columnSpacing: 8

        // Unified Top Header Card (matches chartBox styling)
        Rectangle {
            Layout.fillWidth: true
            Layout.columnSpan: root.headerOverMap ? 1 : parent.columns
            implicitHeight: headerCol.implicitHeight + 12
            color: Utility.getAppHexColor("cardBackground")
            radius: 12
            border.color: Utility.getAppHexColor("raisedBackground")

            ColumnLayout {
                id: headerCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 6
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 0

                // Evenly spaced action buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Chip {
                        id: fileMenuBtn
                        Layout.fillWidth: true
                        text: "File ▾"
                        onClicked: fileMenu.open()

                        PopupMenu {
                            id: fileMenu
                            y: fileMenuBtn.height

                            MenuEntry {
                                text: "Open RT Log..."
                                iconFile: "Line Chart-96.png"
                                onTriggered: root.openRtLogBrowser()
                            }
                            MenuEntry {
                                text: "Open Local CSV..."
                                iconFile: "Open Folder-96.png"
                                onTriggered: csvDialog.open()
                            }
                            MenuEntry {
                                text: "Open Device CSV..."
                                iconFile: "Download-96.png"
                                onTriggered: root.openDeviceBrowser()
                            }
                            MenuEntry {
                                text: "Recent Logs..."
                                iconFile: "History-96.png"
                                onTriggered: root.openRecentLogsDialog()
                            }
                            MenuSeparator {}
                            MenuEntry {
                                text: "Save Local Backup..."
                                iconFile: "Save-96.png"
                                enabled: root.loadedCsvText.length > 0
                                onTriggered: root.saveLoadedLogLocally()
                            }
                            MenuEntry {
                                text: "Save Zoomed CSV..."
                                iconFile: "Fit to Screen-96.png"
                                enabled: root.loadedDataLines.length > 0 &&
                                         (root.viewStart > 0 || root.viewEnd < root.loadedDataLines.length - 1)
                                onTriggered: root.saveZoomedLogLocally()
                            }
                        }
                    }

                    Chip {
                        Layout.fillWidth: true
                        text: "Stats"
                        checkable: true
                        checked: root.showMapStats
                        // The stats are on the map
                        enabled: root.rawRows.length > 0 && map.visible
                        opacity: enabled ? 1 : 0.4
                        onToggled: root.showMapStats = checked
                    }

                    Chip {
                        id: filterBtn
                        Layout.fillWidth: true
                        text: "Filter"
                        checkable: true
                        checked: root.filterOutliers
                        onToggled: {
                            root.filterOutliers = checked
                            root.reparseWithFilter()
                        }
                    }

                    SpinBox {
                        id: threshSpin
                        background: InputBackground {}
                        from: 10
                        to: 2000
                        stepSize: 10
                        value: Math.round(root.outlierThresholdM)
                        implicitHeight: 32
                        implicitWidth: 112
                        topInset: 0
                        bottomInset: 0
                        font.pixelSize: 12
                        textFromValue: function(val) { return val + " m" }
                        valueFromText: function(txt) { return parseInt(txt) || 100 }
                        onValueModified: {
                            root.outlierThresholdM = value
                            if (root.filterOutliers) {
                                root.reparseWithFilter()
                            }
                        }
                    }
                }
            }
        }

        Map {
            id: map
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Half the page on phones, so that the header fits above it
            Layout.preferredWidth: root.headerOverMap ? 3 : 2
            // Beside the values and the graphs when all show
            Layout.rowSpan: root.showAll ? 2 : 1
            visible: root.showAll || !root.showValues
            onVisibleChanged: {
                if (visible && root.routeFitPending) {
                    root.routeFitPending = false
                    Qt.callLater(root.fitRouteBounds)
                }
            }
            plugin: mapPlugin
            center: QtPositioning.coordinate(57.49, 13.11)
            zoomLevel: 13
            enabled: !columnSheet.visible

            // Keeps the stats card inside when the map is resized
            onWidthChanged: statsCard.x = Math.min(statsCard.x, width - statsCard.width - 4)
            onHeightChanged: statsCard.y = Math.min(statsCard.y, height - statsCard.height - 4)

            onSupportedMapTypesChanged: {
                if (supportedMapTypes.length > 0) {
                    activeMapType = supportedMapTypes[supportedMapTypes.length - 1]
                }
            }

            Component.onCompleted: {
                if (supportedMapTypes.length > 0) {
                    activeMapType = supportedMapTypes[supportedMapTypes.length - 1]
                }
                addMapItem(routeLine)
                addMapItem(startMarker)
                addMapItem(endMarker)
                addMapItem(currentMarker)
            }

            TapHandler {
                enabled: root.hasGps
                onTapped: (eventPoint) => {
                              root.jumpToNearestCoord(map.toCoordinate(eventPoint.position))
                          }
            }

            DragHandler {
                id: mapDrag
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                property var lastPos: Qt.point(0, 0)

                onActiveChanged: {
                    if (active) {
                        mapAnim.stop()
                        lastPos = centroid.position
                        root.followMap = false
                    }
                }
                onCentroidChanged: {
                    if (active && !mapPinch.active) {
                        var dx = centroid.position.x - lastPos.x
                        var dy = centroid.position.y - lastPos.y
                        lastPos = centroid.position
                        map.pan(-dx, -dy)
                    }
                }
            }

            PinchHandler {
                id: mapPinch
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                property var anchorGeo: QtPositioning.coordinate(0, 0)

                onActiveChanged: {
                    if (active) {
                        mapAnim.stop()
                        root.followMap = false
                        anchorGeo = map.toCoordinate(centroid.position, false)
                    } else if (mapDrag.active) {
                        mapDrag.lastPos = mapDrag.centroid.position
                    }
                }

                onScaleChanged: (delta) => {
                                    if (!active || delta <= 0) return
                                    map.zoomLevel = Math.max(map.minimumZoomLevel,
                                                             Math.min(map.maximumZoomLevel, map.zoomLevel + Math.log2(delta)))
                                    if (anchorGeo.isValid) {
                                        map.alignCoordinateToPoint(anchorGeo, centroid.position)
                                    }
                                }

                onCentroidChanged: {
                    if (active && anchorGeo.isValid) {
                        map.alignCoordinateToPoint(anchorGeo, centroid.position)
                    }
                }
            }

            // Touchpads pan and pinch, wheels zoom and leave sideways scrolling to the pages. Ctrl+wheel, also a
            // Windows touchpad pinch, zooms.
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                cursorShape: undefined
                onWheel: (event) => {
                             var pixels = event.phase !== Qt.NoScrollPhase &&
                                     (event.pixelDelta.x !== 0 || event.pixelDelta.y !== 0)
                             var ctrl = (event.modifiers & Qt.ControlModifier) && event.angleDelta.y !== 0
                             if (!ctrl && !pixels && Math.abs(event.angleDelta.y) <= Math.abs(event.angleDelta.x)) {
                                 event.accepted = false
                                 return
                             }
                             mapAnim.stop()
                             if (pixels && !ctrl) {
                                 map.pan(-event.pixelDelta.x, -event.pixelDelta.y)
                                 return
                             }
                             var pt = Qt.point(event.x, event.y)
                             var geo = map.toCoordinate(pt, false)
                             map.zoomLevel += event.angleDelta.y / 240
                             if (geo.isValid) map.alignCoordinateToPoint(geo, pt)
                         }
            }

            // Rounds the corners of the map by covering them with the page colour
            Rectangle {
                anchors.fill: parent
                anchors.margins: -6
                z: 5
                radius: 18
                color: "transparent"
                border.width: 6
                border.color: Utility.getAppHexColor("darkBackground")
            }

            // Floating, movable stats card (translucent with outlined text for readability)
            Rectangle {
                id: statsCard
                visible: root.showMapStats && root.zoomedStats.length > 0
                z: 10
                x: 10
                y: 10
                width: Math.min(map.width - 20, Math.max(120, statsCol.implicitWidth + 16))
                height: statsCol.implicitHeight + 16
                radius: 12
                color: Qt.alpha(Utility.getAppHexColor("cardBackground"), 0.55)
                border.color: Qt.alpha(Utility.getAppHexColor("raisedBackground"), 0.4)
                border.width: 1

                onXChanged: {
                    var maxX = Math.max(4, map.width - width - 4)
                    if (x < 4) x = 4
                    else if (x > maxX) x = maxX
                }
                onYChanged: {
                    var maxY = Math.max(4, map.height - height - 4)
                    if (y < 4) y = 4
                    else if (y > maxY) y = maxY
                }

                DragHandler {
                    id: statsDrag
                    target: statsCard
                    grabPermissions: PointerHandler.CanTakeOverFromAnything
                    xAxis.minimum: 4
                    xAxis.maximum: Math.max(4, map.width - statsCard.width - 4)
                    yAxis.minimum: 4
                    yAxis.maximum: Math.max(4, map.height - statsCard.height - 4)
                }

                ColumnLayout {
                    id: statsCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 2

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: "⋮⋮ Stats"
                            font.pixelSize: 11
                            font.bold: true
                            color: Utility.getAppHexColor("titleText")
                            style: Text.Outline
                            styleColor: Qt.alpha(Utility.getAppHexColor("darkBackground"), 0.8)
                        }

                        Item { Layout.fillWidth: true }

                        ToolButton {
                            text: "×"
                            implicitWidth: 20
                            implicitHeight: 20
                            padding: 0
                            font.pixelSize: 13
                            Material.foreground: Utility.getAppHexColor("lightText")
                            onClicked: root.showMapStats = false
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Qt.alpha(Utility.getAppHexColor("lightText"), 0.33)
                    }

                    Repeater {
                        model: root.zoomedStats
                        RowLayout {
                            id: statRow
                            // The fault count on a red badge, as the row opens the list
                            readonly property bool isFaults: modelData.label === "Faults"
                            Layout.fillWidth: true
                            spacing: 8

                            TapHandler {
                                enabled: parent.isFaults
                                margin: 8
                                onTapped: faultsDialog.open()
                            }

                            Label {
                                text: modelData.label + ":"
                                font.pixelSize: 11
                                color: Utility.getAppHexColor("lightText")
                                style: Text.Outline
                                styleColor: Qt.alpha(Utility.getAppHexColor("darkBackground"), 0.8)
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: modelData.value
                                font.pixelSize: 11
                                font.bold: true
                                color: parent.isFaults ? "white" : Utility.getAppHexColor("titleText")
                                style: parent.isFaults ? Text.Normal : Text.Outline
                                styleColor: Qt.alpha(Utility.getAppHexColor("darkBackground"), 0.8)
                                leftPadding: parent.isFaults ? 8 : 0
                                rightPadding: leftPadding
                                topPadding: parent.isFaults ? 2 : 0
                                bottomPadding: topPadding
                                background: Rectangle {
                                    visible: statRow.isFaults
                                    radius: height / 2
                                    color: Utility.getAppHexColor("danger")
                                }
                            }
                        }
                    }
                }
            }

            Column {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 4
                z: 10

                // Top right on both the map and the values, where the values one shows the map icon
                RoundButton {
                    visible: !root.showAll
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Value Tiles-96.png"
                    Accessible.name: qsTr("Values")
                    onClicked: root.showValues = true
                }

                RoundButton {
                    visible: root.hasGps
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Fit to Screen-96.png"
                    Accessible.name: qsTr("Fit Route")
                    onClicked: {
                        root.followMap = false
                        root.easeMap(root.fitRouteBounds)
                    }
                }

                RoundButton {
                    visible: root.hasGps
                    icon.source: "qrc" + Utility.getThemePath() + "icons/icons8-target-96.png"
                    Accessible.name: qsTr("Follow")
                    checkable: true
                    checked: root.followMap
                    highlighted: checked
                    onToggled: {
                        root.followMap = checked
                        if (checked && root.currentMapCoord.isValid) {
                            root.easeMap(() => map.center = root.currentMapCoord)
                        }
                    }
                }

                // Shows the faults on the map and graphs, holding it opens the list
                RoundButton {
                    visible: root.faultRuns.length > 0
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Brake Warning-96.png"
                    Accessible.name: qsTr("Faults")
                    checkable: true
                    checked: root.showFaults
                    highlighted: checked
                    onToggled: root.showFaults = checked
                    onPressAndHold: faultsDialog.open()
                }
            }

            MapItemView {
                model: root.showFaults ? root.faultRuns.filter(f => f.coord && f.end >= root.viewStart && f.start <= root.viewEnd) : []
                // Over the route, under the marker
                delegate: MapQuickItem {
                    z: 1
                    coordinate: modelData.coord
                    anchorPoint: Qt.point(8, 8)
                    sourceItem: Rectangle {
                        width: 16; height: 16; radius: 8
                        color: Utility.getAppHexColor("danger"); border.color: "white"; border.width: 2

                        Text {
                            anchors.centerIn: parent
                            text: "!"
                            color: "white"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }
            }

            Label {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 8
                z: 10
                width: Math.min(implicitWidth, map.width * 0.6)
                horizontalAlignment: Text.AlignRight
                text: root.loadedFileName.length > 0
                      ? (root.loadedFileName + (root.statusText.length > 0 ? (" · " + root.statusText) : ""))
                      : root.statusText
                font.pixelSize: 11
                color: Utility.getAppHexColor("lightText")
                style: Text.Outline
                styleColor: Qt.alpha(Utility.getAppHexColor("darkBackground"), 0.8)
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        // Every value at the cursor, in the place of the map
        ColumnLayout {
            id: valuesView
            Layout.fillWidth: true
            // Above the graphs when all show, as tall as its tiles up to a bit under half the page, the
            // graphs get the rest. Else in the place of the map
            Layout.fillHeight: !root.showAll
            Layout.preferredHeight: root.showAll ? Math.min(valuesHeader.implicitHeight + spacing + valuesGrid.rowsHeight,
                                                            root.height * 0.45) : -1
            Layout.preferredWidth: root.showAll ? 3 : map.Layout.preferredWidth
            visible: root.showAll || root.showValues
            spacing: 4
            onVisibleChanged: swapMenu.close()

            RowLayout {
                id: valuesHeader

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 4
                    text: root.rawRows.length > 0 ? "#" + (root.currentIndex + 1) + " / " + root.rawRows.length : ""
                    color: Utility.getAppHexColor("normalText")
                }

                Chip {
                    topInset: 0
                    visible: root.faultRuns.length > 0
                    text: qsTr("Faults")
                    tinted: true
                    tint: Utility.getAppHexColor("danger")
                    tintText: Utility.getAppHexColor("dangerText")
                    onClicked: faultsDialog.open()
                }

                // Where the Values button is on the map
                RoundButton {
                    visible: !root.showAll
                    Layout.topMargin: 4
                    Layout.rightMargin: 4
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Waypoint Map-96.png"
                    Accessible.name: qsTr("Map")
                    onClicked: root.showValues = false
                }
            }

            GridView {
                id: valuesGrid
                // Height of all the rows of tiles
                readonly property real rowsHeight: Math.ceil(count / Math.max(1, Math.floor(width / 120))) * cellHeight
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                // As many tiles as fit at 120 wide or more
                cellWidth: width / Math.max(1, Math.floor(width / 120))
                cellHeight: 46
                // The menu of a tile is placed at it, so it closes when the tiles move
                onWidthChanged: swapMenu.close()
                // Columns with data, in the groups of the value picker. They keep their places when the
                // plotted values change, the plotted ones are marked in the colours of their lines.
                model: root.columnMeta.filter(c => c.max >= c.min)
                       .sort((a, b) => root.columnGroups.indexOf(a.group) - root.columnGroups.indexOf(b.group) || a.index - b.index)

                delegate: Item {
                    width: valuesGrid.cellWidth
                    height: valuesGrid.cellHeight

                    ValueTile {
                        id: tile
                        readonly property var val: root.cursorValue(modelData.index)
                        // Lines that plot it
                        readonly property var slots: [0, 1, 2, 3].filter(s => root.slotColIdx[s] === modelData.index)
                        // Marked like a checked chip while its menu is open
                        readonly property bool picked: swapMenu.visible && swapMenu.colIdx === modelData.index
                        anchors.fill: parent
                        anchors.margins: 3
                        color: picked ? Qt.alpha(Material.accent, 0.12) : Utility.getAppHexColor("cardBackground")
                        border.width: picked ? 1.5 : 1
                        border.color: picked ? Material.accent : Utility.getAppHexColor("raisedBackground")
                        title: modelData.label
                        value: val === null ? "–" : val.toFixed(root.colDecimals(modelData))
                        unit: modelData.unit
                        // Smaller than elsewhere, the title like the min and max under the graphs
                        titleFont.pixelSize: 10
                        valueFont.pointSize: 12

                        // A bar in the colour of each line that plots it, in the margin at the left edge
                        Column {
                            x: 3
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2

                            Repeater {
                                model: tile.slots

                                Rectangle {
                                    width: 3
                                    height: (tile.height - 16 - (tile.slots.length - 1) * 2) / tile.slots.length
                                    radius: 1.5
                                    color: root.slotColors[modelData]
                                }
                            }
                        }

                        // A tap plots the value in place of one of the lines, if it is not plotted yet
                        TapHandler {
                            enabled: tile.slots.length === 0
                            onTapped: swapMenu.show(modelData.index, tile)
                        }
                    }
                }
            }

            // The lines to plot a tapped value in place of, at its tile. One for all the tiles, as the grid
            // makes them again while it scrolls.
            PopupMenu {
                id: swapMenu
                property int colIdx: -1
                width: Math.min(280, root.width - 32)
                // A press outside only closes it, without scrubbing the graphs under it. The page dims a little
                // and the menu is raised above the tiles, so that it stands out from them.
                modal: true

                Overlay.modal: Rectangle {
                    color: Qt.alpha("black", Utility.isDarkMode() ? 0.4 : 0.2)
                }

                background: Rectangle {
                    color: Utility.getAppHexColor(Utility.isDarkMode() ? "raisedBackground" : "darkBackground")
                    border.color: Utility.getAppHexColor("lightestBackground")
                    radius: 16
                }

                function show(col, tile) {
                    colIdx = col
                    // Under the tile, or over it where the page has no room
                    var p = tile.mapToItem(valuesView, 0, 0)
                    x = p.x
                    y = tile.mapToItem(root, 0, tile.height).y + implicitHeight <= root.height ? p.y + tile.height :
                                                                                               p.y - implicitHeight
                    open()
                }

                function swap(slot) {
                    root.setSlotColumn(slot, colIdx)
                    if (!root.slotEnabled[slot]) root.toggleSlot(slot, true)
                }

                function label(slot) {
                    return root.columnMeta[root.slotColIdx[slot]]?.label ?? ""
                }

                // A dot and the name in the colour of the line, like its picker under the graphs. The name is
                // from the log, so it is escaped for the rich text.
                function entryText(slot) {
                    return "<font color='" + root.slotColors[slot] + "'>● " +
                            label(slot).replace(/&/g, "&amp;").replace(/</g, "&lt;") + "</font>"
                }

                Label {
                    leftPadding: 10
                    topPadding: 6
                    bottomPadding: 4
                    text: qsTr("Plot %1 in place of").arg(root.columnMeta[swapMenu.colIdx]?.label ?? "")
                    elide: Text.ElideRight
                    font.pointSize: 10
                    color: Utility.getAppHexColor("normalText")
                }

                // Faded while the line is hidden, the swap shows it again
                MenuEntry {
                    text: swapMenu.entryText(0)
                    Accessible.name: swapMenu.label(0)
                    opacity: root.slotEnabled[0] ? 1 : 0.5
                    onTriggered: swapMenu.swap(0)
                }
                MenuEntry {
                    text: swapMenu.entryText(1)
                    Accessible.name: swapMenu.label(1)
                    opacity: root.slotEnabled[1] ? 1 : 0.5
                    onTriggered: swapMenu.swap(1)
                }
                MenuEntry {
                    text: swapMenu.entryText(2)
                    Accessible.name: swapMenu.label(2)
                    opacity: root.slotEnabled[2] ? 1 : 0.5
                    onTriggered: swapMenu.swap(2)
                }
                MenuEntry {
                    text: swapMenu.entryText(3)
                    Accessible.name: swapMenu.label(3)
                    opacity: root.slotEnabled[3] ? 1 : 0.5
                    onTriggered: swapMenu.swap(3)
                }
            }
        }

        MapPolyline { id: routeLine; visible: root.hasGps; line.width: 4; line.color: "#00aaff" }
        MapQuickItem {
            id: startMarker
            visible: root.hasGps && root.trackCoords.length > 0
            coordinate: (root.hasGps && root.trackCoords.length > 0) ? root.trackCoords.slice(0, 1).pop() : QtPositioning.coordinate(0, 0)
            anchorPoint: Qt.point(6, 6)
            sourceItem: Rectangle { width: 12; height: 12; radius: 6; color: "#22cc66"; border.color: "white"; border.width: 2 }
        }
        MapQuickItem {
            id: endMarker
            visible: root.hasGps && root.trackCoords.length > 1
            coordinate: (root.hasGps && root.trackCoords.length > 1) ? root.trackCoords[root.trackCoords.length - 1] : QtPositioning.coordinate(0, 0)
            anchorPoint: Qt.point(6, 6)
            sourceItem: Rectangle { width: 12; height: 12; radius: 6; color: "#2255cc"; border.color: "white"; border.width: 2 }
        }
        MapQuickItem {
            id: currentMarker
            z: 2
            visible: root.hasGps
            coordinate: root.currentMapCoord
            anchorPoint: Qt.point(12, 12)
            sourceItem: Item {
                width: 24
                height: 24

                Shape {
                    anchors.fill: parent
                    // Unwrapped so that it turns the short way. A smoothed animation keeps
                    // turning while the direction changes, other animations restart on each change.
                    property real heading: 0
                    readonly property real target: root.currentHeading - map.bearing
                    onTargetChanged: heading += ((target - heading) % 360 + 540) % 360 - 180
                    rotation: heading
                    preferredRendererType: Shape.CurveRenderer

                    Behavior on heading {
                        SmoothedAnimation { velocity: 1800 }
                    }

                    ShapePath {
                        fillColor: "#ff3333"
                        strokeColor: "white"
                        strokeWidth: 2
                        joinStyle: ShapePath.RoundJoin
                        startX: 12; startY: 2
                        PathLine { x: 21; y: 21 }
                        PathLine { x: 12; y: 16 }
                        PathLine { x: 3; y: 21 }
                        PathLine { x: 12; y: 2 }
                    }
                }
            }
        }
        Rectangle {
            id: chartBox
            readonly property bool compactMode: root.wide || root.width < 750
            property bool collapsed: false
            property real expansion: collapsed && !root.wide ? 0 : 1
            Behavior on expansion {
                NumberAnimation { id: expandAnim; duration: 200; easing.type: Easing.OutCubic }
            }
            Layout.fillWidth: true
            Layout.fillHeight: root.wide
            Layout.rowSpan: root.headerOverMap ? 2 : 1
            Layout.preferredWidth: 3
            // Collapsed it keeps the series and a strip of the graphs. Expanded it takes half the page on tall
            // screens, as it collapses with one tap, and leaves room for the map on short ones
            readonly property real collapsedHeight: 116 + seriesGrid.implicitHeight
            Layout.preferredHeight: Math.round(collapsedHeight + expansion * Math.max(0, Math.min(Math.max(compactMode ? 305 : 260, root.height / 2),
                                                                                                root.height - 140) - collapsedHeight))
            color: Utility.getAppHexColor("cardBackground"); radius: 12; border.color: Utility.getAppHexColor("raisedBackground")
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 8; anchors.topMargin: root.wide ? 8 : 0; spacing: 6

                // Tap or drag the grabber to shrink the graphs to a strip that still scrubs, below the series
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 16
                    visible: !root.wide

                    Rectangle {
                        anchors.centerIn: parent
                        width: 36; height: 4; radius: 2
                        color: Utility.getAppHexColor("disabledText")
                    }

                    TapHandler {
                        onTapped: chartBox.collapsed = !chartBox.collapsed
                    }

                    DragHandler {
                        target: null
                        margin: 6
                        enabled: !columnSheet.visible
                        xAxis.enabled: false
                        onActiveTranslationChanged: chartBox.collapsed = activeTranslation.y > 0
                    }
                }

                GridLayout {
                    id: seriesGrid
                    Layout.fillWidth: true
                    columns: chartBox.compactMode ? 2 : 4
                    columnSpacing: 8
                    rowSpacing: 4
                    Repeater {
                        model: 4
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            spacing: 1

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                spacing: 3

                                CheckBox {
                                    checked: root.slotEnabled[index]
                                    onToggled: root.toggleSlot(index, checked)
                                    padding: 0
                                    implicitWidth: 28
                                    implicitHeight: 32
                                    Material.accent: root.slotColors[index]
                                }
                                AbstractButton {
                                    id: columnButton
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 40
                                    implicitHeight: 32
                                    background: InputBackground {}
                                    leftPadding: 4
                                    rightPadding: 18
                                    focusPolicy: Qt.NoFocus
                                    enabled: root.columnMeta.length > 0
                                    // The value next to it shows the unit
                                    text: root.columnMeta[root.slotColIdx[index]]?.label ?? ""
                                    onClicked: {
                                        columnSheet.slot = index
                                        columnSheet.open()
                                    }

                                    indicator: Text {
                                        anchors.right: parent.right
                                        anchors.rightMargin: 5
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "▾"
                                        color: Utility.getAppHexColor("normalText")
                                        font.pixelSize: 11
                                    }

                                    contentItem: Text {
                                        leftPadding: 4
                                        rightPadding: 2
                                        text: columnButton.text
                                        font.pixelSize: 12
                                        color: root.slotColors[index]
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                }
                                Label {
                                    color: root.slotColors[index]
                                    font.bold: true
                                    font.pixelSize: 11
                                    text: root.rawRows.length > 0 ? root.latestSlotVal(index) : ""
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                leftPadding: 31
                                color: Utility.getAppHexColor("normalText")
                                font.pixelSize: 10
                                elide: Text.ElideRight
                                text: root.rawRows.length > 0 ? root.slotMinMaxText(index) : ""
                            }
                        }
                    }

                }
                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Canvas {
                        id: chartLines
                        // Keeps its size and is stretched while the graphs grow or shrink, as large logs take
                        // too long to paint every frame. It paints at the new size when they stop
                        width: parent.width
                        height: expandAnim.running ? paintedHeight : parent.height
                        property real paintedHeight
                        transform: Scale { yScale: chartLines.parent.height / Math.max(1, chartLines.height) }
                        onWidthChanged: requestPaint(); onHeightChanged: { paintedHeight = height; requestPaint() }
                        function drawColSeries(ctx, colIdx, color, range) {
                            var data = root.rawRows
                            var s = Math.max(0, root.viewStart)
                            var e = Math.min(data.length - 1, root.viewEnd)
                            var n = e - s + 1
                            if (n < 2 || colIdx < 0) return
                            var span = Math.max(0.001, range.max - range.min)
                            var step = Math.max(1, Math.floor(n / Math.max(1, width * 2)))
                            ctx.strokeStyle = color; ctx.lineWidth = 2; ctx.beginPath()
                            var started = false
                            var px = 0, py = 0
                            for (var i = s; i <= e; i += step) {
                                var v = data[i].vals[colIdx]
                                if (v === null || isNaN(v)) continue
                                var x = ((i - s) / (n - 1)) * width
                                var y = height - (((v - range.min) / span) * (height - 10) + 5)
                                if (!started) { ctx.moveTo(x, y); px = x; py = y; started = true } else ctx.lineTo(x, y)
                                // Short paths stroke much faster than one long path that zigzags over itself. Each
                                // starts a point back, so that the lines still join
                                if ((i - s) % (16 * step) === 0) { ctx.stroke(); ctx.beginPath(); ctx.moveTo(px, py); ctx.lineTo(x, y) }
                                px = x; py = y
                            }
                            ctx.stroke()
                        }
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = Utility.getAppHexColor("raisedBackground"); ctx.lineWidth = 1
                            for (var g = 1; g <= 3; g++) {
                                var gy = (height / 4) * g
                                ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(width, gy); ctx.stroke()
                            }
                            if (root.rawRows.length < 2) return
                            // A band over the rows with a fault, dashed where it began and with a tab on top,
                            // so that it is not taken for a spike in a red line
                            if (root.showFaults) {
                                var span = Math.max(1, root.viewEnd - root.viewStart)
                                var red = Utility.getAppHexColor("danger")
                                ctx.strokeStyle = red; ctx.lineWidth = 1; ctx.setLineDash([3, 3])
                                root.faultRuns.forEach(function(f) {
                                    var x1 = (f.start - root.viewStart) / span * width
                                    var x2 = (f.end - root.viewStart) / span * width
                                    if (x2 < 0 || x1 > width) return
                                    ctx.fillStyle = Qt.alpha(red, 0.15)
                                    ctx.fillRect(x1, 0, Math.max(2, x2 - x1), height)
                                    ctx.beginPath(); ctx.moveTo(x1, 0); ctx.lineTo(x1, height); ctx.stroke()
                                    ctx.fillStyle = red
                                    ctx.beginPath(); ctx.moveTo(x1 - 5, 0); ctx.lineTo(x1 + 5, 0); ctx.lineTo(x1, 7); ctx.closePath(); ctx.fill()
                                })
                                ctx.setLineDash([])
                            }
                            for (var s = 3; s >= 0; s--) {
                                if (root.slotEnabled[s]) {
                                    drawColSeries(ctx, root.slotColIdx[s], root.slotColors[s], root.slotRanges[s])
                                }
                            }
                        }
                    }
                    Canvas {
                        id: chartCursor
                        anchors.fill: parent
                        onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            var sIdx = Math.max(0, root.viewStart)
                            var eIdx = Math.min(root.rawRows.length - 1, root.viewEnd)
                            var n = eIdx - sIdx + 1
                            if (n < 2) return
                            var clampedCur = Math.max(sIdx, Math.min(eIdx, root.currentIndex))
                            var x = ((clampedCur - sIdx) / (n - 1)) * width
                            ctx.strokeStyle = Utility.getAppHexColor("titleText"); ctx.lineWidth = 1.5
                            ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke()
                            var pt = root.rawRows[clampedCur]
                            for (var s = 0; s < 4; s++) {
                                if (!root.slotEnabled[s]) continue
                                var cIdx = root.slotColIdx[s]
                                if (cIdx < 0) continue
                                var v = pt.vals[cIdx]
                                if (v === null || isNaN(v)) continue
                                var r = root.slotRanges[s]
                                var span = Math.max(0.001, r.max - r.min)
                                var y = height - (((v - r.min) / span) * (height - 10) + 5)
                                ctx.fillStyle = root.slotColors[s]
                                ctx.beginPath(); ctx.arc(x, y, 4.5, 0, Math.PI * 2); ctx.fill()
                            }
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: root.rawRows.length > 1 && !columnSheet.visible && !chartPinch.active
                        // Zoom not yet applied
                        property real zoom: 1

                        function scrub(mx) {
                            var span = Math.max(1, root.viewEnd - root.viewStart)
                            var idx = root.viewStart + Math.round(Math.max(0, Math.min(1, mx / width)) * span)
                            root.currentIndex = idx
                        }
                        onPressed: (m) => scrub(m.x)
                        onPositionChanged: (m) => { if (pressed) scrub(m.x) }
                        onWheel: (w) => {
                                     // Sideways scrolling is left to the pages. Small touchpad steps add up.
                                     if (Math.abs(w.angleDelta.x) > Math.abs(w.angleDelta.y)) {
                                         w.accepted = false
                                     } else if (w.angleDelta.y !== 0) {
                                         zoom *= Math.pow(0.75, w.angleDelta.y / 120)
                                         var span = Math.max(10, root.viewEnd - root.viewStart)
                                         if (Math.round(span * zoom) !== span) {
                                             root.zoomAroundCursor(zoom)
                                             zoom = 1
                                         }
                                     }
                                 }
                    }
                    // Extra touch-drag claim so parent SwipeView never steals horizontal drags
                    DragHandler {
                        target: null
                        enabled: root.rawRows.length > 1 && !columnSheet.visible && !chartPinch.active
                        grabPermissions: PointerHandler.CanTakeOverFromAnything
                        onCentroidChanged: {
                            if (active) {
                                var span = Math.max(1, root.viewEnd - root.viewStart)
                                var ratio = Math.max(0, Math.min(1, centroid.position.x / parent.width))
                                root.currentIndex = root.viewStart + Math.round(ratio * span)
                            }
                        }
                    }

                    PinchHandler {
                        id: chartPinch
                        target: null
                        enabled: root.rawRows.length > 2 && !columnSheet.visible
                        grabPermissions: PointerHandler.CanTakeOverFromAnything

                        property real lastRatio: 0.5

                        onActiveChanged: {
                            if (active) {
                                lastRatio = Math.max(0, Math.min(1, centroid.position.x / parent.width))
                            }
                        }

                        onScaleChanged: (delta) => {
                                            if (!active || delta <= 0) return
                                            var maxIdx = root.rawRows.length - 1
                                            var curSpan = Math.max(10, root.viewEnd - root.viewStart)
                                            var newSpan = Math.max(10, Math.min(maxIdx, Math.round(curSpan / delta)))

                                            var curRatio = Math.max(0, Math.min(1, centroid.position.x / parent.width))
                                            var anchorIdx = root.viewStart + lastRatio * curSpan
                                            var newStart = Math.round(anchorIdx - curRatio * newSpan)
                                            var newEnd = newStart + newSpan

                                            if (newStart < 0) { newEnd = Math.min(maxIdx, newEnd - newStart); newStart = 0 }
                                            if (newEnd > maxIdx) { newStart = Math.max(0, newStart - (newEnd - maxIdx)); newEnd = maxIdx }

                                            lastRatio = curRatio
                                            root.applyZoomWindow(newStart, newEnd, false)
                                        }

                        onCentroidChanged: {
                            if (!active) return
                            var maxIdx = root.rawRows.length - 1
                            var curSpan = root.viewEnd - root.viewStart
                            if (curSpan >= maxIdx) return

                            var curRatio = Math.max(0, Math.min(1, centroid.position.x / parent.width))
                            var shift = Math.round((lastRatio - curRatio) * curSpan)
                            if (shift !== 0) {
                                var newStart = Math.max(0, Math.min(maxIdx - curSpan, root.viewStart + shift))
                                lastRatio = curRatio
                                root.applyZoomWindow(newStart, newStart + curSpan, false)
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 22
                    spacing: 6

                    Label {
                        color: Utility.getAppHexColor("normalText")
                        font.pixelSize: 11
                        text: "Zoom:"
                    }

                    RangeSlider {
                        id: zoomRange
                        Layout.fillWidth: true
                        implicitHeight: 22
                        padding: 0
                        enabled: root.rawRows.length > 1
                        from: 0; to: Math.max(0, root.rawRows.length - 1); stepSize: 1
                        first.onMoved: root.applyZoomWindow(first.value, second.value, false)
                        second.onMoved: root.applyZoomWindow(first.value, second.value, false)
                    }

                    Label {
                        color: Utility.getAppHexColor("normalText")
                        font.pixelSize: 11
                        text: root.rawRows.length > 0 ? (root.viewStart + 1) + "-" + (root.viewEnd + 1) : ""
                    }
                }
            }
        }
    }
}
