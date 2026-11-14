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
        pendingDeleteName = fileName
        pendingDeletePath = base + fileName
        confirmDeleteDialog.open()
    }

    function executeDeleteDeviceFile() {
        if (pendingDeletePath.length === 0) return
        if (mCommands.fileBlockRemove(pendingDeletePath)) {
            statusText = "Deleted " + pendingDeleteName + " from device."
            refreshDeviceDir(deviceCurrentDir)
        } else {
            statusText = "Failed to delete " + pendingDeleteName + "."
        }
        pendingDeletePath = ""
        pendingDeleteName = ""
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

    Dialog {
        id: confirmDeleteDialog
        title: "Delete Device Log?"
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.85, 360)
        standardButtons: Dialog.Yes | Dialog.No
        closePolicy: Popup.NoAutoClose

        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "Delete '" + root.pendingDeleteName + "' permanently from the connected device?"
        }

        onAccepted: root.executeDeleteDeviceFile()
    }

    Dialog {
        id: deviceFileDialog
        title: "Device Logs (" + root.deviceCurrentDir + ")"
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.92, 460)
        height: Math.min(parent.height * 0.80, 520)
        closePolicy: Popup.NoAutoClose

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                TextField {
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

        standardButtons: Dialog.Close
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
            if (Utility.writeTextFile(selectedFile.toString(), root.loadedCsvText)) {
                root.statusText = "Saved backup: " + decodeURIComponent(selectedFile.toString()).split("/").pop()
            } else {
                root.statusText = "Failed to save local backup."
            }
        }
    }

    function saveLoadedLogLocally() {
        if (!loadedCsvText || loadedCsvText.length === 0) {
            statusText = "No log loaded to save."
            return
        }
        saveCsvDialog.currentFile = loadedFileName
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
    property string statusText: "Click 'Open CSV Log' to load a VESC log file."

    property var slotEnabled: [true, true, true, true]
    property var slotColIdx: [-1, -1, -1, -1]
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
        if (rawRows.length === 0) return
        var s = Math.max(0, Math.min(viewStart, rawRows.length - 1))
        var e = Math.max(s, Math.min(viewEnd, rawRows.length - 1))

        var distM = 0.0
        var prevCoord = null
        var firstTime = null
        var lastTime = null

        for (var i = s; i <= e; i++) {
            var row = rawRows[i]
            if (row.coord !== null) {
                if (prevCoord !== null) {
                    var stepM = prevCoord.distanceTo(row.coord)
                    if (stepM < 100) distM += stepM
                }
                prevCoord = row.coord
            }
            if (timeColIdx !== -1 && row.vals[timeColIdx] !== null) {
                if (firstTime === null) firstTime = row.vals[timeColIdx]
                lastTime = row.vals[timeColIdx]
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
            mapView.map.center = visibleCoords.slice(0, 1).pop()
            mapView.map.zoomLevel = 16
            return
        }
        mapView.map.fitViewportToGeoShape(QtPositioning.rectangle(visibleCoords), Qt.size(40, 40))
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
        if (colIdx < 0 || rawRows.length === 0) return { min: 0, max: 1, rawMin: null, rawMax: null }
        var minV = Infinity, maxV = -Infinity
        var s = Math.max(0, viewStart), e = Math.min(rawRows.length - 1, viewEnd)
        for (var i = s; i <= e; i++) {
            var v = rawRows[i].vals[colIdx]
            if (v !== null && !isNaN(v)) {
                if (v < minV) minV = v
                if (v > maxV) maxV = v
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

    function parseCsvText(text) {
        if (!text || text.length === 0) { statusText = "Failed to read file."; return }
        loadedCsvText = text

        var lines = text.trim().split(/\r?\n/)
        if (lines.length < 2) { statusText = "CSV file has no data rows."; return }

        var meta = parseHeaderMeta(lines.shift())
        var latIdx = findColIdx(meta.cols, ["gnss_lat", "lat", "latitude"])
        var lonIdx = findColIdx(meta.cols, ["gnss_lon", "lon", "lng", "longitude"])
        timeColIdx = findColIdx(meta.cols, ["t_day", "esc uptime", "jet uptime", "time"])

        var coords = [], rows = []
        for (var i = 0; i < lines.length; i++) {
            if (lines[i].trim().length === 0) continue
            var parts = lines[i].split(meta.delim)
            var vals = new Array(meta.cols.length)
            for (var c = 0; c < meta.cols.length; c++) {
                if (c < parts.length && parts[c].trim().length > 0) {
                    var n = parseFloat(parts[c])
                    vals[c] = isNaN(n) ? null : n
                } else {
                    vals[c] = null
                }
            }

            var coord = null
            if (latIdx !== -1 && lonIdx !== -1 && vals[latIdx] !== null && vals[lonIdx] !== null) {
                if (vals[latIdx] !== 0 || vals[lonIdx] !== 0) {
                    coord = QtPositioning.coordinate(vals[latIdx], vals[lonIdx])
                    coords.push(coord)
                }
            }
            rows.push({ coord: coord, vals: vals })
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
    onCurrentIndexChanged: chartCursor.requestPaint()
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

                Menu {
                    id: fileMenu
                    y: fileMenuBtn.height

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
                }
            }

            Button {
                text: "Fit Route"
                enabled: root.hasGps
                onClicked: root.fitRouteBounds()
            }

            Label {
                Layout.fillWidth: true
                text: root.statusText
                elide: Text.ElideMiddle
                color: palette.text
            }
        }

        MapView {
            id: mapView
            Layout.fillWidth: true
            Layout.fillHeight: true
            map.plugin: mapPlugin
            map.center: QtPositioning.coordinate(57.49, 13.11)
            map.zoomLevel: 13

            Connections {
                target: mapView.map
                function onSupportedMapTypesChanged() {
                    var types = mapView.map.supportedMapTypes
                    if (types && types.length > 0) {
                        mapView.map.activeMapType = types[types.length - 1]
                    }
                }
            }

            Component.onCompleted: {
                var types = mapView.map.supportedMapTypes
                if (types && types.length > 0) {
                    mapView.map.activeMapType = types[types.length - 1]
                }
                mapView.map.addMapItem(routeLine)
                mapView.map.addMapItem(startMarker)
                mapView.map.addMapItem(endMarker)
                mapView.map.addMapItem(currentMarker)
            }
            TapHandler {
                enabled: root.hasGps
                onTapped: (eventPoint) => {
                              var geo = mapView.map.toCoordinate(eventPoint.position)
                              root.jumpToNearestCoord(geo)
                          }
            }
            PinchHandler {
                id: mapPinch
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                property real startZoom: 13
                property var startGeo: QtPositioning.coordinate(0, 0)

                onActiveChanged: {
                    if (active) {
                        startZoom = mapView.map.zoomLevel
                        startGeo = mapView.map.toCoordinate(centroid.position, false)
                    }
                }

                onScaleChanged: (delta) => {
                                    if (!active || scale <= 0) return
                                    var targetZoom = startZoom + Math.log2(scale)
                                    targetZoom = Math.max(mapView.map.minimumZoomLevel,
                                                          Math.min(mapView.map.maximumZoomLevel, targetZoom))
                                    mapView.map.zoomLevel = targetZoom
                                    // Keep the geographic point under the two fingers anchored while zooming
                                    mapView.map.alignCoordinateToPoint(startGeo, centroid.position)
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
                        enabled: root.rawRows.length > 1
                        preventStealing: true

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
                        enabled: root.rawRows.length > 1
                        grabPermissions: PointerHandler.CanTakeOverFromAnything
                        onCentroidChanged: {
                            if (active) {
                                var span = Math.max(1, root.viewEnd - root.viewStart)
                                var ratio = Math.max(0, Math.min(1, centroid.position.x / parent.width))
                                root.currentIndex = root.viewStart + Math.round(ratio * span)
                            }
                        }
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label {
                color: "#9ca3af"
                font.pixelSize: 11
                text: "Zoom:"
            }
            RangeSlider {
                id: zoomRange
                Layout.fillWidth: true
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