/*
    Log Analysis page – faithful QML port of PageLogAnalysis.
    Uses QtLocation (OSM) for the map and QtGraphs for the plot.
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtGraphs
import QtLocation
import QtPositioning

import Vedder.vesc

Item {
    id: root

    // ── Data model ─────────────────────────────────────────────────
    property var logHeader: []      // [{key, name, unit, precision, isTimeStamp, isRelativeToFirst}]
    property var logData: []        // [[double,...], ...]  (each row same length as logHeader)
    property var logTruncated: []   // subset of logData after span slider filtering
    property string currentLogName: "No log opened"

    // Well-known column indices (mirrors mInd_* in C++)
    property int ind_t_day: -1
    property int ind_t_day_pos: -1
    property int ind_gnss_lat: -1
    property int ind_gnss_lon: -1
    property int ind_gnss_alt: -1
    property int ind_gnss_h_acc: -1
    property int ind_trip_vesc: -1
    property int ind_trip_vesc_abs: -1
    property int ind_trip_gnss: -1
    property int ind_cnt_wh: -1
    property int ind_cnt_wh_chg: -1
    property int ind_cnt_ah: -1
    property int ind_cnt_ah_chg: -1
    property int ind_roll: -1
    property int ind_pitch: -1
    property int ind_yaw: -1

    // Playback state
    property double playPosNow: 0.0
    property bool   playing: false

    // Dynamic LineSeries objects
    property var dynamicSeries: []
    property int _legendRevision: 0

    // ── Helpers ────────────────────────────────────────────────────
    function resetInds() {
        ind_t_day = -1; ind_t_day_pos = -1
        ind_gnss_lat = -1; ind_gnss_lon = -1; ind_gnss_alt = -1; ind_gnss_h_acc = -1
        ind_trip_vesc = -1; ind_trip_vesc_abs = -1; ind_trip_gnss = -1
        ind_cnt_wh = -1; ind_cnt_wh_chg = -1; ind_cnt_ah = -1; ind_cnt_ah_chg = -1
        ind_roll = -1; ind_pitch = -1; ind_yaw = -1
    }

    function updateInds() {
        resetInds()
        for (var i = 0; i < logHeader.length; i++) {
            switch (logHeader[i].key) {
            case "t_day":          ind_t_day = i;          break
            case "t_day_pos":      ind_t_day_pos = i;      break
            case "gnss_lat":       ind_gnss_lat = i;       break
            case "gnss_lon":       ind_gnss_lon = i;       break
            case "gnss_alt":       ind_gnss_alt = i;       break
            case "gnss_h_acc":     ind_gnss_h_acc = i;     break
            case "trip_vesc":      ind_trip_vesc = i;       break
            case "trip_vesc_abs":  ind_trip_vesc_abs = i;   break
            case "trip_gnss":      ind_trip_gnss = i;       break
            case "cnt_wh":         ind_cnt_wh = i;          break
            case "cnt_wh_chg":     ind_cnt_wh_chg = i;     break
            case "cnt_ah":         ind_cnt_ah = i;          break
            case "cnt_ah_chg":     ind_cnt_ah_chg = i;     break
            case "roll":           ind_roll = i;            break
            case "pitch":          ind_pitch = i;           break
            case "yaw":            ind_yaw = i;             break
            }
        }
    }

    function getLogSample(time) {
        if (logTruncated.length === 0) return []
        var d = logTruncated[0]
        if (ind_t_day >= 0) {
            var t0 = logTruncated[0][ind_t_day]
            for (var i = 0; i < logTruncated.length; i++) {
                var dt = logTruncated[i][ind_t_day] - t0
                if (dt < 0) dt += 86400
                if (dt >= time) { d = logTruncated[i]; break }
            }
        }
        return d
    }

    function fmtTime(ms) {
        var s = Math.floor(ms / 1000)
        var h = Math.floor(s / 3600); s -= h * 3600
        var m = Math.floor(s / 60);   s -= m * 60
        return (h < 10 ? "0" : "") + h + ":" +
               (m < 10 ? "0" : "") + m + ":" +
               (s < 10 ? "0" : "") + s
    }

    function fmtSec(val) {
        var totalSec = Math.floor(val)
        var hh = Math.floor(totalSec / 3600); totalSec -= hh * 3600
        var mm = Math.floor(totalSec / 60);   totalSec -= mm * 60
        return (hh < 10 ? "0" : "") + hh + ":" +
               (mm < 10 ? "0" : "") + mm + ":" +
               (totalSec < 10 ? "0" : "") + totalSec
    }

    // ── Centre map to best GNSS point ──────────────────────────────
    function centreMapOnData(data) {
        if (ind_gnss_lat < 0 || ind_gnss_lon < 0) return
        var bestAcc = 1e9, bLat = 0, bLon = 0
        for (var i = 0; i < data.length; i++) {
            var ha = ind_gnss_h_acc >= 0 ? data[i][ind_gnss_h_acc] : 0
            if (ha > 0 && ha < bestAcc) {
                bestAcc = ha; bLat = data[i][ind_gnss_lat]; bLon = data[i][ind_gnss_lon]
            }
        }
        if (bestAcc < 1e9) {
            theMap.center = QtPositioning.coordinate(bLat, bLon)
            theMap.zoomLevel = 15
        }
    }

    function fitMapToTrack() {
        if (trackCoords.length === 0) return
        var mnLa = 90, mxLa = -90, mnLo = 180, mxLo = -180
        for (var i = 0; i < trackCoords.length; i++) {
            var c = trackCoords[i]
            var la = c.latitude, lo = c.longitude
            if (la < mnLa) mnLa = la;  if (la > mxLa) mxLa = la
            if (lo < mnLo) mnLo = lo;  if (lo > mxLo) mxLo = lo
        }
        theMap.fitViewportToGeoShape(
            QtPositioning.rectangle(
                QtPositioning.coordinate(mxLa, mnLo),
                QtPositioning.coordinate(mnLa, mxLo)),
            40)
    }

    // ── Load VESC RT log  ──────────────────────────────────────────
    function loadVescRtLog() {
        var rtLog = VescIf.getRtLogData()
        if (rtLog.length === 0) { statusLabel.text = "No RT log data"; return }

        currentLogName = "Realtime"
        logData = [];  logTruncated = [];  logHeader = []

        logHeader = [
            {key:"kmh_vesc",         name:"Speed VESC",       unit:"km/h",precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"kmh_gnss",         name:"Speed GNSS",       unit:"km/h",precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"t_day",            name:"Time",             unit:"s",   precision:0, isTimeStamp:true,  isRelativeToFirst:false},
            {key:"t_day_pos",        name:"Time GNSS",        unit:"s",   precision:0, isTimeStamp:true,  isRelativeToFirst:false},
            {key:"t_trip",           name:"Time of trip",     unit:"s",   precision:0, isTimeStamp:true,  isRelativeToFirst:true},
            {key:"trip_vesc",        name:"Trip VESC",        unit:"m",   precision:3, isTimeStamp:false, isRelativeToFirst:true},
            {key:"trip_vesc_abs",    name:"Trip VESC ABS",    unit:"m",   precision:3, isTimeStamp:false, isRelativeToFirst:true},
            {key:"trip_gnss",        name:"Trip GNSS",        unit:"m",   precision:3, isTimeStamp:false, isRelativeToFirst:true},
            {key:"setup_curr_motor", name:"Current Motors",   unit:"A",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"setup_curr_battery",name:"Current Battery", unit:"A",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"setup_power",      name:"Power",            unit:"W",   precision:0, isTimeStamp:false, isRelativeToFirst:false},
            {key:"erpm",             name:"ERPM",             unit:"",    precision:0, isTimeStamp:false, isRelativeToFirst:false},
            {key:"duty",             name:"Duty",             unit:"%",   precision:1, isTimeStamp:false, isRelativeToFirst:false},
            {key:"fault",            name:"Fault Code",       unit:"",    precision:0, isTimeStamp:false, isRelativeToFirst:false},
            {key:"v_in",             name:"Input Voltage",    unit:"V",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"soc",              name:"Battery Level",    unit:"%",   precision:1, isTimeStamp:false, isRelativeToFirst:false},
            {key:"t_mosfet",         name:"Temp MOSFET",      unit:"°C",  precision:1, isTimeStamp:false, isRelativeToFirst:false},
            {key:"t_motor",          name:"Temp Motor",       unit:"°C",  precision:1, isTimeStamp:false, isRelativeToFirst:false},
            {key:"cnt_ah",           name:"Ah Used",          unit:"Ah",  precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"cnt_ah_chg",       name:"Ah Charged",       unit:"Ah",  precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"cnt_wh",           name:"Wh Used",          unit:"Wh",  precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"cnt_wh_chg",       name:"Wh Charged",       unit:"Wh",  precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"id",               name:"Id",               unit:"A",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"iq",               name:"Iq",               unit:"A",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"vd",               name:"Vd",               unit:"V",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"vq",               name:"Vq",               unit:"V",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"t_mosfet_1",       name:"Temp MOSFET 1",    unit:"°C",  precision:1, isTimeStamp:false, isRelativeToFirst:false},
            {key:"t_mosfet_2",       name:"Temp MOSFET 2",    unit:"°C",  precision:1, isTimeStamp:false, isRelativeToFirst:false},
            {key:"t_mosfet_3",       name:"Temp MOSFET 3",    unit:"°C",  precision:1, isTimeStamp:false, isRelativeToFirst:false},
            {key:"position",         name:"Position",         unit:"°",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"roll",             name:"Roll",             unit:"°",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"pitch",            name:"Pitch",            unit:"°",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"yaw",              name:"Yaw",              unit:"°",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"acc_x",            name:"Acc X",            unit:"G",   precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"acc_y",            name:"Acc Y",            unit:"G",   precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"acc_z",            name:"Acc Z",            unit:"G",   precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"gyro_x",           name:"Gyro X",           unit:"°/s", precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"gyro_y",           name:"Gyro Y",           unit:"°/s", precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"gyro_z",           name:"Gyro Z",           unit:"°/s", precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"v1_curr_motor",    name:"V1 Curr Motor",    unit:"A",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"v1_curr_battery",  name:"V1 Curr Battery",  unit:"A",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"v1_cnt_ah",        name:"V1 Ah Used",       unit:"Ah",  precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"v1_cnt_ah_chg",    name:"V1 Ah Charged",    unit:"Ah",  precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"v1_cnt_wh",        name:"V1 Wh Used",       unit:"Wh",  precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"v1_cnt_wh_chg",    name:"V1 Wh Charged",    unit:"Wh",  precision:3, isTimeStamp:false, isRelativeToFirst:false},
            {key:"gnss_lat",         name:"Latitude",         unit:"°",   precision:7, isTimeStamp:false, isRelativeToFirst:false},
            {key:"gnss_lon",         name:"Longitude",        unit:"°",   precision:7, isTimeStamp:false, isRelativeToFirst:false},
            {key:"gnss_alt",         name:"Altitude",         unit:"m",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"gnss_v_vel",       name:"V. Speed GNSS",    unit:"m/s", precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"gnss_h_acc",       name:"H. Accuracy GNSS", unit:"m",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"gnss_v_acc",       name:"V. Accuracy GNSS", unit:"m",   precision:2, isTimeStamp:false, isRelativeToFirst:false},
            {key:"num_vesc",         name:"VESC num",         unit:"",    precision:0, isTimeStamp:false, isRelativeToFirst:false}
        ]

        var rows = []
        for (var i = 0; i < rtLog.length; i++) {
            var d = rtLog[i]
            var v = d.values
            var s = d.setupValues
            var im = d.imuValues
            rows.push([
                s.speed * 3.6,                  // kmh_vesc
                d.gVel * 3.6,                   // kmh_gnss
                d.valTime / 1000.0,             // t_day
                d.posTime / 1000.0,             // t_day_pos
                d.valTime / 1000.0,             // t_trip
                s.tachometer,                   // trip_vesc
                s.tachometer,                   // trip_vesc_abs (abs not available per-sample)
                0,                              // trip_gnss (computed later)
                s.current_motor,                // setup_curr_motor
                s.current_in,                   // setup_curr_battery
                s.current_in * v.v_in,          // setup_power
                v.rpm,                          // erpm
                v.duty_now * 100,               // duty
                v.fault_code,                   // fault
                v.v_in,                         // v_in
                s.battery_level * 100.0,        // soc
                v.temp_mos,                     // t_mosfet
                v.temp_motor,                   // t_motor
                s.amp_hours,                    // cnt_ah
                s.amp_hours_charged,            // cnt_ah_chg
                s.watt_hours,                   // cnt_wh
                s.watt_hours_charged,           // cnt_wh_chg
                v.id,                           // id
                v.iq,                           // iq
                v.vd,                           // vd
                v.vq,                           // vq
                v.temp_mos_1,                   // t_mosfet_1
                v.temp_mos_2,                   // t_mosfet_2
                v.temp_mos_3,                   // t_mosfet_3
                v.position,                     // position
                im.roll,                        // roll  (radians)
                im.pitch,                       // pitch
                im.yaw,                         // yaw
                im.accX,                        // acc_x
                im.accY,                        // acc_y
                im.accZ,                        // acc_z
                im.gyroX,                       // gyro_x
                im.gyroY,                       // gyro_y
                im.gyroZ,                       // gyro_z
                0, 0, 0, 0, 0, 0,              // v1 placeholders
                d.lat,                          // gnss_lat
                d.lon,                          // gnss_lon
                d.alt,                          // gnss_alt
                d.vVel,                         // gnss_v_vel
                d.hAcc,                         // gnss_h_acc
                d.vAcc,                         // gnss_v_acc
                s.num_vescs                     // num_vesc
            ])
        }

        logData = rows
        updateInds()
        rebuildDataTable()
        truncateAndPlot(true)
        centreMapOnData(logData)
        statusLabel.text = "Loaded " + rtLog.length + " RT log entries"
    }

    // ── Open CSV (semicolon-separated) ─────────────────────────────
    function openCsvData(name, text) {
        currentLogName = name
        var lines = text.split("\n")
        if (lines.length < 2) { statusLabel.text = "Invalid log file"; return }

        var hdrTokens = lines[0].split(";")
        if (hdrTokens.length < 1) { statusLabel.text = "Invalid header"; return }

        // Check for old-format RT log (no colons in first field)
        if (hdrTokens[0].indexOf(":") < 0) {
            if (VescIf.loadRtLogFile(text)) loadVescRtLog()
            return
        }

        logData = [];  logTruncated = [];  logHeader = []
        var lastEntry = []
        for (var h = 0; h < hdrTokens.length; h++) {
            var p = hdrTokens[h].split(":")
            logHeader.push({
                key:               p[0] || "",
                name:              p.length > 1 ? p[1] : p[0],
                unit:              p.length > 2 ? p[2] : "",
                precision:         p.length > 3 ? parseInt(p[3]) : 2,
                isRelativeToFirst: p.length > 4 ? parseInt(p[4]) !== 0 : false,
                isTimeStamp:       p.length > 5 ? parseInt(p[5]) !== 0 : false
            })
            lastEntry.push(0.0)
        }

        for (var li = 1; li < lines.length; li++) {
            var line = lines[li].trim()
            if (line.length === 0) continue
            var tokens = line.split(";")
            if (tokens.length !== lastEntry.length) continue
            var entry = []
            for (var t = 0; t < tokens.length; t++) {
                if (tokens[t].length > 0) lastEntry[t] = parseFloat(tokens[t])
                entry.push(lastEntry[t])
            }
            logData.push(entry)
        }

        updateInds()

        // Generate t_day if missing
        if (ind_t_day < 0) {
            logHeader.push({key:"t_day", name:"Sample", unit:"", precision:0, isTimeStamp:false, isRelativeToFirst:false})
            for (var si = 0; si < logData.length; si++) logData[si].push(si)
            updateInds()
        }

        rebuildDataTable()
        truncateAndPlot(true)
        centreMapOnData(logData)
        statusLabel.text = name + "  (" + logData.length + " samples)"
    }

    // ── Rebuild the column-selection ListModel ─────────────────────
    function rebuildDataTable() {
        dataListModel.clear()
        for (var i = 0; i < logHeader.length; i++) {
            dataListModel.append({
                name: logHeader[i].name,
                unit: logHeader[i].unit,
                y1:   false,
                y2:   false,
                scale: 1.0,
                isTS:  logHeader[i].isTimeStamp,
                value: ""
            })
        }
    }

    // ── Truncate and plot ──────────────────────────────────────────
    property var trackCoords: []

    function truncateAndPlot(zoomMap) {
        var start = spanSlider.first.value / 10000.0
        var end   = spanSlider.second.value / 10000.0

        logTruncated = []
        var coords = []

        for (var i = 0; i < logData.length; i++) {
            var prop = logData.length > 1 ? (i / (logData.length - 1)) : 0
            if (prop < start || prop > end) continue
            logTruncated.push(logData[i])

            if (ind_gnss_lat >= 0 && ind_gnss_lon >= 0) {
                var lat = logData[i][ind_gnss_lat]
                var lon = logData[i][ind_gnss_lon]
                var skip = false
                if (ind_gnss_h_acc >= 0) {
                    var ha = logData[i][ind_gnss_h_acc]
                    if (ha <= 0) skip = true
                    if (filterBox.checked && ha > filterHAcc.value) skip = true
                }
                if (!skip && lat !== 0 && lon !== 0)
                    coords.push(QtPositioning.coordinate(lat, lon))
            }
        }

        trackCoords = coords
        trackLine.path = coords

        if (zoomMap) fitMapToTrack()
        updateGraphs()
        updateStats()
    }

    // ── Plot update (dynamic LineSeries) ───────────────────────────
    function updateGraphs() {
        // Remove old dynamic series
        for (var r = 0; r < dynamicSeries.length; r++)
            graphsView.removeSeries(dynamicSeries[r])
        dynamicSeries = []

        if (logTruncated.length === 0) return

        var plotColors = ["#4080ff", "#FF00FF", "#40C040", "#FF4040", "#00FFFF", "#FFA000",
                          "#FF80FF", "#80FF80", "#80FFFF", "#FFFF40", "#C0C0C0", "#8080FF"]

        var colIdx = 0
        var startTime = ind_t_day >= 0 ? logTruncated[0][ind_t_day] : 0
        var yMin = 1e15, yMax = -1e15, xMax = 0

        for (var c = 0; c < dataListModel.count; c++) {
            var item = dataListModel.get(c)
            if (!item.y1 && !item.y2) continue
            var scale = item.scale

            var ser = lineSeriesComp.createObject(graphsView, {
                color: plotColors[colIdx % plotColors.length],
                width: 1.5,
                name:  item.name + " (" + item.unit + " ×" + scale.toFixed(1) + ")"
            })

            for (var s = 0; s < logTruncated.length; s++) {
                var x
                if (ind_t_day >= 0) {
                    x = logTruncated[s][ind_t_day] - startTime
                    if (x < 0) x += 86400
                } else {
                    x = s
                }
                var y = logTruncated[s][c] * scale
                ser.append(x, y)
                if (y < yMin) yMin = y
                if (y > yMax) yMax = y
                if (x > xMax) xMax = x
            }

            graphsView.addSeries(ser)
            dynamicSeries.push(ser)
            colIdx++
        }

        plotAxisX.min = 0
        plotAxisX.max = xMax > 0 ? xMax : 1
        var margin = (yMax - yMin) * 0.05
        if (margin === 0) margin = 1
        if (yMin < 1e14) { plotAxisY.min = yMin - margin; plotAxisY.max = yMax + margin }

        _legendRevision++
    }

    // ── Stats ──────────────────────────────────────────────────────
    function updateStats() {
        statsModel.clear()
        if (logTruncated.length < 2) return
        var first = logTruncated[0], last = logTruncated[logTruncated.length - 1]
        var n = logTruncated.length

        var tMs = 0
        if (ind_t_day >= 0) { tMs = (last[ind_t_day] - first[ind_t_day]) * 1000; if (tMs < 0) tMs += 86400000 }
        var tSec = tMs / 1000.0

        var dist    = ind_trip_vesc >= 0     ? last[ind_trip_vesc]     - first[ind_trip_vesc]     : 0
        var distAbs = ind_trip_vesc_abs >= 0 ? last[ind_trip_vesc_abs] - first[ind_trip_vesc_abs] : 0
        var distG   = ind_trip_gnss >= 0     ? last[ind_trip_gnss]     - first[ind_trip_gnss]     : 0
        var wh      = ind_cnt_wh >= 0        ? last[ind_cnt_wh]        - first[ind_cnt_wh]        : 0
        var whC     = ind_cnt_wh_chg >= 0    ? last[ind_cnt_wh_chg]    - first[ind_cnt_wh_chg]    : 0
        var ah      = ind_cnt_ah >= 0        ? last[ind_cnt_ah]        - first[ind_cnt_ah]        : 0
        var ahC     = ind_cnt_ah_chg >= 0    ? last[ind_cnt_ah_chg]    - first[ind_cnt_ah_chg]    : 0

        var avgSpd  = tSec > 0 ? 3.6 * distAbs / tSec : 0
        var avgSpdG = tSec > 0 ? 3.6 * distG   / tSec : 0
        var effV    = distAbs > 0 ? (wh - whC) / (distAbs / 1000.0) : 0
        var effG    = distG   > 0 ? (wh - whC) / (distG   / 1000.0) : 0
        var rate    = tSec > 0 ? n / tSec : 0

        statsModel.append({stat:"Samples",          val:"" + n})
        statsModel.append({stat:"Total Time",        val:fmtTime(tMs)})
        statsModel.append({stat:"Distance",          val:dist.toFixed(2) + " m"})
        statsModel.append({stat:"Distance ABS",      val:distAbs.toFixed(2) + " m"})
        statsModel.append({stat:"Distance GNSS",     val:distG.toFixed(2) + " m"})
        statsModel.append({stat:"Wh Used",           val:wh.toFixed(2) + " Wh"})
        statsModel.append({stat:"Wh Charged",        val:whC.toFixed(2) + " Wh"})
        statsModel.append({stat:"Ah Used",           val:ah.toFixed(2) + " Ah"})
        statsModel.append({stat:"Ah Charged",        val:ahC.toFixed(2) + " Ah"})
        statsModel.append({stat:"Avg Speed",         val:avgSpd.toFixed(2) + " km/h"})
        statsModel.append({stat:"Avg Speed GNSS",    val:avgSpdG.toFixed(2) + " km/h"})
        statsModel.append({stat:"Efficiency",        val:effV.toFixed(2) + " Wh/km"})
        statsModel.append({stat:"Efficiency GNSS",   val:effG.toFixed(2) + " Wh/km"})
        statsModel.append({stat:"Avg Sample Rate",   val:rate.toFixed(2) + " Hz"})
    }

    // ── Update current-position readout ────────────────────────────
    function updateDataAndPlot(time) {
        if (logTruncated.length === 0) return
        playPosNow = time
        var sample = getLogSample(time)
        var first  = logTruncated[0]

        for (var i = 0; i < sample.length && i < dataListModel.count; i++) {
            var val = sample[i]
            var hdr = logHeader[i]

            if (hdr.isRelativeToFirst) {
                val -= first[i]
                if (hdr.isTimeStamp && val < 0) val += 86400
            }

            var txt = hdr.isTimeStamp ? fmtSec(val) : val.toFixed(hdr.precision) + " " + hdr.unit
            dataListModel.setProperty(i, "value", txt)
        }

        // Map marker
        if (ind_gnss_lat >= 0 && ind_gnss_lon >= 0) {
            var lat = sample[ind_gnss_lat]
            var lon = sample[ind_gnss_lon]
            var ok  = lat !== 0 && lon !== 0
            if (ind_gnss_h_acc >= 0 && sample[ind_gnss_h_acc] <= 0) ok = false
            if (ok) {
                posMarker.coordinate = QtPositioning.coordinate(lat, lon)
                posMarker.visible = true
                if (followBox.checked) theMap.center = posMarker.coordinate
            }
        }

        // 3D IMU view
        if (ind_roll >= 0 && ind_pitch >= 0 && ind_yaw >= 0 &&
                vesc3dLoader.status === Loader.Ready) {
            vesc3dLoader.item.setRotation(
                sample[ind_roll]  * 180.0 / Math.PI,
                sample[ind_pitch] * 180.0 / Math.PI,
                useYawBox.checked ? sample[ind_yaw] * 180.0 / Math.PI : 0)
        }
    }

    // URL → local-path helper
    function urlToPath(url) {
        var s = url.toString()
        if (Qt.platform.os === "windows") return s.replace(/^file:\/\/\//, "")
        return s.replace(/^file:\/\//, "")
    }

    // ── File dialogs ───────────────────────────────────────────────
    FileDialog {
        id: openCsvDialog
        title: "Open CSV Log"
        nameFilters: ["CSV files (*.csv)", "All files (*)"]
        onAccepted: {
            var path = urlToPath(selectedFile)
            var data = Utility.readAllFromFile(path)
            if (data.length > 0) openCsvData(path, Utility.arr2str(data))
        }
    }

    // ── Models ─────────────────────────────────────────────────────
    ListModel { id: dataListModel }
    ListModel { id: statsModel }

    // ── Component for dynamic LineSeries ───────────────────────────
    Component {
        id: lineSeriesComp
        LineSeries { }
    }

    // ── Timers ─────────────────────────────────────────────────────
    Timer {
        id: playTimer; interval: 100; repeat: true; running: playing
        onTriggered: {
            if (logTruncated.length > 0 && ind_t_day >= 0) {
                playPosNow += interval / 1000.0
                var total = logTruncated[logTruncated.length-1][ind_t_day] - logTruncated[0][ind_t_day]
                if (total < 0) total += 86400
                if (playPosNow <= total) updateDataAndPlot(playPosNow)
                else playing = false
            }
        }
    }

    Timer {
        id: gnssTimer; interval: 100; repeat: true; running: pollGnssBox.checked
        onTriggered: {
            if (VescIf.isPortConnected()) VescIf.commands().getGnss(0xFFFF)
        }
    }

    // ═══════════════════════════════════════════════════════════════
    //                            UI
    // ═══════════════════════════════════════════════════════════════

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Toolbar row ────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 4
            spacing: 4

            Label {
                id: statusLabel
                text: currentLogName
                elide: Text.ElideMiddle
                Layout.fillWidth: true
                color: Utility.getAppHexColor("lightText")
            }

            Button {
                text: "Open CSV"
                onClicked: openCsvDialog.open()
            }

            Button {
                text: "Load Current"
                onClicked: loadVescRtLog()
            }
            Button {
                text: playing ? "⏸ Pause" : "▶ Play"
                onClicked: {
                    if (!playing) { playPosNow = 0; playing = true }
                    else playing = false
                }
            }
        }

        // ── Main split: upper (map | 3D)  lower (plot | tables) ───
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical

            // ─── Upper half ────────────────────────────────────────
            SplitView {
                SplitView.fillWidth: true
                SplitView.preferredHeight: parent.height * 0.50
                orientation: Qt.Horizontal

                // ── Map column ─────────────────────────────────────
                ColumnLayout {
                    SplitView.fillWidth: true
                    SplitView.minimumWidth: 300
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 4; spacing: 4

                        CheckBox { id: followBox;   text: "Follow"   }
                        CheckBox { id: autoZoomBox;  text: "AutoZoom" }
                        CheckBox { id: pollGnssBox;  text: "Poll GNSS" }
                        CheckBox { id: filterBox;    text: "Filter"   }
                        SpinBox  { id: filterHAcc; from: 1; to: 100; value: 10; visible: filterBox.checked }
                        Item { Layout.fillWidth: true }
                        Button { text: "Center"; onClicked: fitMapToTrack() }
                    }

                    Map {
                        id: theMap
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        plugin: Plugin {
                            name: "osm"
                            PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: "true" }
                        }
                        center: QtPositioning.coordinate(57.715, 12.891)
                        zoomLevel: 5

                        // Pan + zoom handlers
                        PinchHandler {
                            id: pinch; target: null
                            property geoCoordinate startCentroid
                            onActiveChanged: if (active) startCentroid = theMap.toCoordinate(pinch.centroid.position, false)
                            onScaleChanged: function(delta) {
                                theMap.zoomLevel += Math.log2(delta)
                                theMap.alignCoordinateToPoint(startCentroid, pinch.centroid.position)
                            }
                        }
                        WheelHandler {
                            id: wheel
                            acceptedDevices: PointerDevice.Mouse
                            rotationScale: 1/120
                            property: "zoomLevel"
                        }
                        DragHandler {
                            id: drag; target: null
                            onTranslationChanged: function(delta) { theMap.pan(-delta.x, -delta.y) }
                        }

                        // Track polyline
                        MapPolyline {
                            id: trackLine
                            line.width: 3
                            line.color: "#4080ff"
                        }

                        // Current-position marker
                        MapQuickItem {
                            id: posMarker
                            visible: false
                            anchorPoint.x: 8; anchorPoint.y: 8
                            sourceItem: Rectangle {
                                width: 16; height: 16; radius: 8
                                color: "#FF4040"
                                border.width: 2; border.color: "white"
                            }
                        }

                        // Live GNSS marker
                        MapQuickItem {
                            id: gnssMarker
                            visible: false
                            anchorPoint.x: 6; anchorPoint.y: 6
                            sourceItem: Rectangle {
                                width: 12; height: 12; radius: 6
                                color: "#00FF00"
                                border.width: 2; border.color: "white"
                            }
                        }
                    }
                }

                // ── 3D IMU panel ───────────────────────────────────
                ColumnLayout {
                    SplitView.preferredWidth: 250
                    SplitView.minimumWidth: 150
                    spacing: 4

                    CheckBox { id: useYawBox; text: "Use Yaw (will drift)"; checked: true }

                    Loader {
                        id: vesc3dLoader
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        asynchronous: true
                        visible: status === Loader.Ready
                        sourceComponent: Vesc3DView { anchors.fill: parent }
                        onLoaded: item.setRotation(20, 20, 0)
                    }
                }
            }

            // ─── Lower half ────────────────────────────────────────
            ColumnLayout {
                SplitView.fillWidth: true
                SplitView.preferredHeight: parent.height * 0.50
                spacing: 0

                // Span slider
                RangeSlider {
                    id: spanSlider
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    from: 0; to: 10000; stepSize: 1
                    first.value: 0; second.value: 10000
                    snapMode: RangeSlider.SnapAlways
                    first.onMoved:  truncateAndPlot(autoZoomBox.checked)
                    second.onMoved: truncateAndPlot(autoZoomBox.checked)
                }

                SplitView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: Qt.Horizontal

                    // ── Plot ───────────────────────────────────────
                    ColumnLayout {
                        SplitView.fillWidth: true
                        SplitView.minimumWidth: 300
                        spacing: 2

                    GraphsView {
                        id: graphsView
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        theme: GraphsTheme {
                            colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark
                                                              : GraphsTheme.ColorScheme.Light
                            plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                            grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                            grid.subColor:  Utility.isDarkMode() ? "#333333" : "#eeeeee"
                        }

                        axisX: ValueAxis { id: plotAxisX; min: 0; max: 100; titleText: "Seconds (s)" }
                        axisY: ValueAxis { id: plotAxisY; min: -100; max: 100; titleText: "Value" }

                        // Scrub by clicking on plot
                        TapHandler {
                            onTapped: function(eventPoint) {
                                var pa = graphsView.plotArea
                                var lx = eventPoint.position.x - pa.x
                                if (lx >= 0 && lx <= pa.width) {
                                    var frac = lx / pa.width
                                    var t = plotAxisX.min + frac * (plotAxisX.max - plotAxisX.min)
                                    updateDataAndPlot(t)
                                }
                            }
                        }
                    }
                    PlotLegend { graphsView: graphsView; revision: _legendRevision }
                    }

                    // ── Data / Stats tables ────────────────────────
                    ColumnLayout {
                        SplitView.preferredWidth: 380
                        SplitView.minimumWidth: 260
                        spacing: 0

                        TabBar {
                            id: tableTab
                            Layout.fillWidth: true
                            TabButton { text: "Data"; topPadding: 9; bottomPadding: 9 }
                            TabButton { text: "Stats"; topPadding: 9; bottomPadding: 9 }
                        }

                        StackLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            currentIndex: tableTab.currentIndex

                            // ── Data table ─────────────────────────
                            ListView {
                                id: dataTable
                                clip: true
                                model: dataListModel
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                header: Rectangle {
                                    width: dataTable.width; height: 26
                                    color: Utility.getAppHexColor("darkAccent")
                                    z: 2

                                    RowLayout {
                                        anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4
                                        Label { text:"Name";  Layout.preferredWidth:110; font.bold:true; color:Utility.getAppHexColor("lightText") }
                                        Label { text:"Value"; Layout.fillWidth:true;     font.bold:true; color:Utility.getAppHexColor("lightText") }
                                        Label { text:"Y1"; Layout.preferredWidth:30;     font.bold:true; color:Utility.getAppHexColor("lightText") }
                                        Label { text:"Y2"; Layout.preferredWidth:30;     font.bold:true; color:Utility.getAppHexColor("lightText") }
                                        Label { text:"Scale"; Layout.preferredWidth:70;  font.bold:true; color:Utility.getAppHexColor("lightText") }
                                    }
                                }

                                delegate: Rectangle {
                                    width: dataTable.width; height: 24
                                    color: index % 2 === 0 ? "transparent" : Qt.rgba(0.5,0.5,0.5,0.08)

                                    RowLayout {
                                        anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4; spacing: 2

                                        Label {
                                            text: model.name; Layout.preferredWidth: 110; elide: Text.ElideRight
                                            font.pointSize: 11; color: Utility.getAppHexColor("lightText")
                                        }
                                        Label {
                                            text: model.value; Layout.fillWidth: true; elide: Text.ElideRight
                                            font.pointSize: 11; color: Utility.getAppHexColor("normalText")
                                        }
                                        CheckBox {
                                            Layout.preferredWidth: 30; checked: model.y1; visible: !model.isTS
                                            onToggled: {
                                                dataListModel.setProperty(index, "y1", checked)
                                                if (checked) dataListModel.setProperty(index, "y2", false)
                                                updateGraphs()
                                            }
                                        }
                                        CheckBox {
                                            Layout.preferredWidth: 30; checked: model.y2; visible: !model.isTS
                                            onToggled: {
                                                dataListModel.setProperty(index, "y2", checked)
                                                if (checked) dataListModel.setProperty(index, "y1", false)
                                                updateGraphs()
                                            }
                                        }
                                        SpinBox {
                                            Layout.preferredWidth: 80; visible: !model.isTS
                                            from: 1; to: 9999; value: model.scale * 100; stepSize: 10; editable: true
                                            onValueModified: {
                                                dataListModel.setProperty(index, "scale", value / 100.0)
                                                updateGraphs()
                                            }
                                            textFromValue: function(v) { return (v / 100.0).toFixed(2) }
                                            valueFromText: function(t) { return Math.round(parseFloat(t) * 100) }
                                        }
                                    }
                                }
                            }

                            // ── Stats table ────────────────────────
                            ListView {
                                clip: true
                                model: statsModel
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                header: Rectangle {
                                    width: parent ? parent.width : 0; height: 26
                                    color: Utility.getAppHexColor("darkAccent")
                                    z: 2
                                    RowLayout {
                                        anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4
                                        Label { text:"Statistic"; Layout.fillWidth:true; font.bold:true; color:Utility.getAppHexColor("lightText") }
                                        Label { text:"Value";     Layout.fillWidth:true; font.bold:true; color:Utility.getAppHexColor("lightText") }
                                    }
                                }

                                delegate: Rectangle {
                                    width: parent ? parent.width : 0; height: 22
                                    color: index % 2 === 0 ? "transparent" : Qt.rgba(0.5,0.5,0.5,0.08)
                                    RowLayout {
                                        anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4
                                        Label { text: model.stat; Layout.fillWidth:true; font.pointSize:12; color:Utility.getAppHexColor("lightText") }
                                        Label { text: model.val;  Layout.fillWidth:true; font.pointSize:12; color:Utility.getAppHexColor("normalText") }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ── RT log streaming ───────────────────────────────────────────
    property var rtHeader: []
    property var rtSamples: []
    property var rtData: []
    property bool rtAppendTime: false

    Connections {
        target: VescIf.commands()

        function onLogStart(fieldNum, rateHz, appendTime, appendGnss, appendGnssTime) {
            rtAppendTime = appendTime
            var num = fieldNum + (appendTime ? 1 : 0)
            rtHeader = new Array(num)
            if (appendTime) {
                rtHeader[0] = {key:"t_day", name:"Time", unit:"s", precision:0, isTimeStamp:true, isRelativeToFirst:false}
            }
            rtSamples = new Array(num).fill(0)
            rtData = []
        }

        function onLogStop() {
            logHeader = rtHeader
            logData = rtData
            updateInds()
            rebuildDataTable()
            truncateAndPlot(true)
            centreMapOnData(logData)
        }

        function onLogConfigField(fieldInd, header) {
            var idx = fieldInd + (rtAppendTime ? 1 : 0)
            while (rtHeader.length <= idx) rtHeader.push(null)
            rtHeader[idx] = {
                key: header.key, name: header.name, unit: header.unit,
                precision: header.precision, isTimeStamp: header.isTimeStamp,
                isRelativeToFirst: header.isRelativeToFirst
            }
        }

        function onLogSamples(fieldStart, samples) {
            var start = fieldStart + (rtAppendTime ? 1 : 0)
            for (var i = 0; i < samples.length; i++) {
                var idx = i + start
                if (idx < rtSamples.length) rtSamples[idx] = samples[i]
            }
            // When we receive data for field 0 we know a full row arrived
            if (fieldStart === 0) {
                rtData.push(rtSamples.slice())
            }
        }

        function onGnssRx(val, mask) {
            if (!pollGnssBox.checked) return
            var lat = val.lat, lon = val.lon
            if (lat === 0 && lon === 0) return
            if (filterBox.checked && val.hdop * 5.0 > filterHAcc.value) return
            gnssMarker.coordinate = QtPositioning.coordinate(lat, lon)
            gnssMarker.visible = true
            if (followBox.checked) theMap.center = gnssMarker.coordinate
        }
    }
}
