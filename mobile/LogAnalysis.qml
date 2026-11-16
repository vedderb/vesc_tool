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
import QtQuick.Layouts
import QtQuick.Dialogs
import QtLocation
import QtPositioning
import QtCore

import Vedder.vesc.utility
import Vedder.vesc.vescinterface

Item {
    id: root
    anchors.fill: parent

    property var mCommands: VescIf.commands()
    property string deviceCurrentDir: "/"
    property var deviceFileEntries: []
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
        var rawList = mCommands.fileBlockList(dirPath)
        var items = []

        if (dirPath !== "/" && dirPath.length > 0) {
            items.push({ name: "..", isDir: true, size: 0 })
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
                items.push({ name: name, isDir: isDir, size: size })
            }
        }

        // Sort folders first, then newest/alphabetical filenames
        items.sort(function(a, b) {
            if (a.name === "..") return -1
            if (b.name === "..") return 1
            if (a.isDir !== b.isDir) return a.isDir ? -1 : 1
            return b.name.localeCompare(a.name)
        })

        deviceFileEntries = items
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

    SheetDialog {
        id: confirmDeleteDialog
        acceptText: qsTr("Delete")
        title: "Delete Device Log?"
        closePolicy: Popup.NoAutoClose

        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "Delete '" + root.pendingDeleteName + "' permanently from the connected device?"
        }

        onAccepted: root.executeDeleteFile()
    }

    SheetDialog {
        id: deviceFileDialog
        title: "Device Logs (" + root.deviceCurrentDir + ")"
        fullHeight: true
        closePolicy: Popup.NoAutoClose

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
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

            ListView {
                id: deviceListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                enabled: !root.downloadingFromDevice
                model: root.deviceFileEntries

                delegate: Item {
                    width: deviceListView.width
                    height: rowDelegate.implicitHeight

                    RowLayout {
                        id: rowDelegate
                        anchors.fill: parent
                        spacing: 4

                        ItemDelegate {
                            Layout.fillWidth: true
                            text: (modelData.isDir ? "[DIR] " : "") + modelData.name +
                                  (!modelData.isDir && modelData.size > 0 ? " (" + (modelData.size / 1024).toFixed(1) + " KB)" : "")
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
                        }

                        ToolButton {
                            visible: !modelData.isDir
                            text: "Save"
                            font.pixelSize: 11
                            onClicked: root.backupDeviceCsv(modelData.name)
                        }

                        ToolButton {
                            visible: !modelData.isDir
                            text: "Delete"
                            font.pixelSize: 11
                            palette.buttonText: "#ff5555"
                            onClicked: root.promptDeleteDeviceFile(modelData.name)
                        }
                    }
                }

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
                    font.family: "Monospace"
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
                rtLogEntries = Utility.listFilesInDir(root.currentRtLogDir)
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
        title: "RT Logs (" + root.currentRtLogDir + ")"
        closePolicy: Popup.NoAutoClose
        fullHeight: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 6

            Label {
                visible: root.rtLogEntries.length === 0
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: "No .csv logs found in:\n" + Utility.getSetting("path_rt_log", "").toString()
                color: "#9ca3af"
            }

            ListView {
                id: rtLogListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.rtLogEntries

                delegate: Item {
                    width: rtLogListView.width
                    height: rtRow.implicitHeight

                    RowLayout {
                        id: rtRow
                        anchors.fill: parent
                        spacing: 4

                        ItemDelegate {
                            Layout.fillWidth: true
                            text: modelData.name + "  (" + (modelData.size / 1024).toFixed(1) + " KB · " + modelData.modified + ")"
                            onClicked: {
                                var text = Utility.readTextFile(modelData.path)
                                if (text && text.length > 0) {
                                    root.loadedFileName = modelData.name
                                    root.parseCsvText(text)
                                    rtLogListDialog.close()
                                } else {
                                    root.statusText = "Failed to read: " + modelData.name
                                }
                            }
                        }

                        ToolButton {
                            text: "Delete"
                            font.pixelSize: 11
                            palette.buttonText: "#ff5555"
                            onClicked: root.promptDeleteLocalFile(modelData.name, modelData.path)
                        }
                    }
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

    property var trackCoords: []
    property var rawRows: []
    property var columnMeta: []
    property var columnNames: []
    property int timeColIdx: -1
    property bool hasGps: false
    property int currentIndex: 0
    property int viewStart: 0
    property int viewEnd: 0
    property string statusText: "Open a log from the File menu."
    property bool comboOpen: false
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
    readonly property var slotColors: ["#00d4ff", "#ff9900", "#ff4d6d", "#4ade80"]

    property var currentMapCoord: {
        if (!hasGps || rawRows.length === 0) return QtPositioning.coordinate(0, 0)
        for (var i = currentIndex; i >= 0; i--) {
            if (rawRows[i].coord !== null) return rawRows[i].coord
        }
        return trackCoords.length > 0 ? trackCoords.slice(0, 1).pop() : QtPositioning.coordinate(0, 0)
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

        var distStr = (distM / 1000.0).toFixed(2) + " km"
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

    function parseHeaderMeta(hLine) {
        var delim = hLine.indexOf(";") !== -1 ? ";" : (hLine.indexOf(",") !== -1 ? "," : "\t")
        var rawCols = hLine.split(delim)
        var cols = [], names = []
        rawCols.forEach(function(raw, i) {
            var p = raw.trim().split(":")
            var k = (p.shift() || "").trim()
            var lbl = (p.length > 0 ? p.shift() : k).trim()
            var u = (p.length > 0 ? p.shift() : "").trim()
            cols.push({ index: i, key: k.toLowerCase(), label: lbl, unit: u })
            names.push(u.length > 0 ? (lbl + " (" + u + ")") : lbl)
        })
        return { delim: delim, cols: cols, names: names }
    }

    function findColIdx(cols, candidates) {
        for (var c = 0; c < candidates.length; c++) {
            var t = candidates[c].toLowerCase()
            for (var i = 0; i < cols.length; i++) {
                if (cols[i].key === t || cols[i].label.toLowerCase() === t) return i
            }
        }
        return -1
    }

    function loadCsv(fileUrl) {
        var text = Utility.readTextFile(fileUrl.toString())
        if (!text || text.length === 0) { statusText = "Failed to read file."; return }
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
        loadedDataLines = cleanLines

        var meta = parseHeaderMeta(loadedHeaderLine)
        var latIdx = findColIdx(meta.cols, ["gnss_lat", "lat", "latitude"])
        var lonIdx = findColIdx(meta.cols, ["gnss_lon", "lon", "lng", "longitude"])

        // Pass 1: Parse values and collect unique GPS fixes (collapsing repeated coordinates)
        var rawParsed = []
        var uniqueGps = []
        var lastRawLat = null, lastRawLon = null

        for (var i = 0; i < cleanLines.length; i++) {
            var parts = cleanLines[i].split(meta.delim)
            var vals = new Array(meta.cols.length)
            for (var c = 0; c < meta.cols.length; c++) {
                if (c < parts.length && parts[c].trim().length > 0) {
                    var n = parseFloat(parts[c])
                    vals[c] = isNaN(n) ? null : n
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
            rows.push({ coord: coord, vals: rawParsed[r].vals, distance: distance })
        }

        if (rows.length === 0) { statusText = "No valid rows found."; return }

        columnMeta = meta.cols
        columnNames = meta.names
        rawRows = rows
        trackCoords = coords
        hasGps = coords.length > 0
        viewStart = 0
        viewEnd = rows.length - 1
        currentIndex = 0
        zoomRange.first.value = 0
        zoomRange.second.value = rows.length - 1
        updateVisibleRoute()
        var d0 = findColIdx(meta.cols, ["gnss_h_vel", "speed_meters_per_sec", "impeller rpm", "m1 erpm"])
        var d1 = findColIdx(meta.cols, ["bms current in", "m1 current in", "current_in", "impeller power"])
        var d2 = findColIdx(meta.cols, ["m1 current", "current_motor", "jet temp gearbox"])
        var d3 = findColIdx(meta.cols, ["voltage in", "input_voltage", "bms soc"])
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
    }
    function latestSlotVal(slot) {
        var colIdx = slotColIdx[slot]
        if (colIdx < 0 || rawRows.length === 0 || colIdx >= columnMeta.length) return ""
        var unit = columnMeta[colIdx].unit
        for (var i = currentIndex; i >= 0; i--) {
            var v = rawRows[i].vals[colIdx]
            if (v !== null && !isNaN(v)) return v.toFixed(1) + (unit ? " " + unit : "")
        }
        return "N/A"
    }
    function slotMinMaxText(slot) {
        var r = slotRanges[slot]
        if (!r || r.rawMin === null || r.rawMax === null) return "Min: -  Max: -"
        return "Min: " + r.rawMin.toFixed(1) + "  Max: " + r.rawMax.toFixed(1)
    }

    onRawRowsChanged: rangeCache = ({})

    onCurrentIndexChanged: {
        chartCursor.requestPaint()
        if (followMap && hasGps && rawRows.length > 0) {
            var c = currentMapCoord
            if (c.isValid && (c.latitude !== 0 || c.longitude !== 0)) {
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                id: fileMenuBtn
                text: "File ▾"
                highlighted: true
                onClicked: fileMenu.open()

                PopupMenu {
                    id: fileMenu
                    y: fileMenuBtn.height

                    MenuItem {
                        text: "Open RT Log..."
                        onTriggered: root.openRtLogBrowser()
                    }
                    MenuItem {
                        text: "Open Local CSV..."
                        onTriggered: csvDialog.open()
                    }
                    MenuItem {
                        text: "Open Device CSV..."
                        onTriggered: root.openDeviceBrowser()
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: "Save Local Backup..."
                        enabled: root.loadedCsvText.length > 0
                        onTriggered: root.saveLoadedLogLocally()
                    }
                    MenuItem {
                        text: "Save Zoomed CSV..."
                        enabled: root.loadedDataLines.length > 0 &&
                                 (root.viewStart > 0 || root.viewEnd < root.loadedDataLines.length - 1)
                        onTriggered: root.saveZoomedLogLocally()
                    }
                }
            }

            Button {
                text: "Fit Route"
                enabled: root.hasGps
                onClicked: root.fitRouteBounds()
            }

            Button {
                text: "Follow"
                checkable: true
                checked: root.followMap
                enabled: root.hasGps
                onToggled: {
                    root.followMap = checked
                    if (checked && root.currentMapCoord.isValid) {
                        map.center = root.currentMapCoord
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: root.statusText
                elide: Text.ElideMiddle
                color: palette.text
            }
        }

        // Compact GPS Outlier Filter Row (between File bar and Map)
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            CheckBox {
                id: filterCheck
                text: "Filter outliers"
                checked: root.filterOutliers
                implicitHeight: 26
                padding: 0
                font.pixelSize: 12
                onToggled: {
                    root.filterOutliers = checked
                    root.reparseWithFilter()
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "Threshold:"
                font.pixelSize: 12
                color: "#9ca3af"
                enabled: root.filterOutliers
            }

            SpinBox {
                id: threshSpin
                background: InputBackground {}
                from: 10
                to: 2000
                stepSize: 10
                value: Math.round(root.outlierThresholdM)
                implicitHeight: 26
                implicitWidth: 118
                font.pixelSize: 12
                textFromValue: function(val) { return val + " m" }
                valueFromText: function(txt) { return parseInt(txt) || 100 }
                onValueModified: {
                    root.outlierThresholdM = value

                    if (filterCheck.checked) {
                        root.reparseWithFilter()
                    }
                }
            }
        }

        Map {
            id: map
            Layout.fillWidth: true
            Layout.fillHeight: true
            plugin: mapPlugin
            center: QtPositioning.coordinate(57.49, 13.11)
            zoomLevel: 13
            enabled: !root.comboOpen

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

            WheelHandler {
                onWheel: (event) => {
                             var pt = Qt.point(event.x, event.y)
                             var geo = map.toCoordinate(pt, false)
                             map.zoomLevel += (event.angleDelta.y > 0 ? 0.5 : -0.5)
                             if (geo.isValid) map.alignCoordinateToPoint(geo, pt)
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
            visible: root.hasGps
            coordinate: root.currentMapCoord
            anchorPoint: Qt.point(8, 8)
            sourceItem: Rectangle { width: 16; height: 16; radius: 8; color: "#ff3333"; border.color: "white"; border.width: 2 }
        }
        Rectangle {
            id: chartBox
            readonly property bool compactMode: root.width < 750
            Layout.fillWidth: true
            Layout.preferredHeight: compactMode ? 275 : 230
            color: "#1a1d24"; radius: 6; border.color: "#333a45"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 6; spacing: 4
                GridLayout {
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
                                    // Color dot overlaid neatly or tinting the checkbox
                                    Rectangle {
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        width: 7; height: 7; radius: 3.5
                                        color: root.slotColors[index]
                                        border.color: "#1a1d24"; border.width: 1
                                    }
                                }
                                ComboBox {
                                    id: colCombo
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 40
                                    implicitHeight: 28
                                    leftPadding: 4
                                    rightPadding: 18
                                    enabled: root.columnNames.length > 0
                                    model: root.columnNames
                                    currentIndex: root.slotColIdx[index]
                                    onActivated: (idx) => root.setSlotColumn(index, idx)

                                    popup.width: Math.max(width, 240)
                                    popup.modal: true
                                    popup.dim: false
                                    popup.onOpened: root.comboOpen = true
                                    popup.onClosed: root.comboOpen = false


                                    indicator: Text {
                                        anchors.right: parent.right
                                        anchors.rightMargin: 5
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "▾"
                                        color: "#9ca3af"
                                        font.pixelSize: 11
                                    }

                                    contentItem: Text {
                                        leftPadding: 4
                                        rightPadding: 2
                                        text: colCombo.displayText
                                        font.pixelSize: 12
                                        color: root.slotColors[index]
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                }
                                Label {
                                    color: root.slotColors[index]
                                    font.family: "Monospace"
                                    font.bold: true
                                    font.pixelSize: 11
                                    text: root.rawRows.length > 0 ? root.latestSlotVal(index) : ""
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                leftPadding: 28
                                color: "#9ca3af"
                                font.family: "Monospace"
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
                        anchors.fill: parent
                        onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
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
                            for (var i = s; i <= e; i += step) {
                                var v = data[i].vals[colIdx]
                                if (v === null || isNaN(v)) continue
                                var x = ((i - s) / (n - 1)) * width
                                var y = height - (((v - range.min) / span) * (height - 10) + 5)
                                if (!started) { ctx.moveTo(x, y); started = true } else ctx.lineTo(x, y)
                            }
                            ctx.stroke()
                        }
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = "#2a303c"; ctx.lineWidth = 1
                            for (var g = 1; g <= 3; g++) {
                                var gy = (height / 4) * g
                                ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(width, gy); ctx.stroke()
                            }
                            if (root.rawRows.length < 2) return
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
                            ctx.strokeStyle = "#ffffff"; ctx.lineWidth = 1.5
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
                        enabled: root.rawRows.length > 1 && !root.comboOpen && !chartPinch.active

                        function scrub(mx) {
                            var span = Math.max(1, root.viewEnd - root.viewStart)
                            var idx = root.viewStart + Math.round(Math.max(0, Math.min(1, mx / width)) * span)
                            root.currentIndex = idx
                        }
                        onPressed: (m) => scrub(m.x)
                        onPositionChanged: (m) => { if (pressed) scrub(m.x) }
                        onWheel: (w) => {
                                     if (w.angleDelta.y > 0) root.zoomAroundCursor(0.75)
                                     else if (w.angleDelta.y < 0) root.zoomAroundCursor(1.33)
                                 }
                    }
                    // Extra touch-drag claim so parent SwipeView never steals horizontal drags
                    DragHandler {
                        target: null
                        enabled: root.rawRows.length > 1 && !root.comboOpen && !chartPinch.active
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
                        enabled: root.rawRows.length > 2 && !root.comboOpen
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
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 22
            spacing: 6

            Label {
                color: "#9ca3af"
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
                color: "#9ca3af"
                font.family: "Monospace"
                font.pixelSize: 11
                text: root.rawRows.length > 0 ? (root.viewStart + 1) + "-" + (root.viewEnd + 1) : ""
            }
        }
    }
}
