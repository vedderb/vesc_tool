/*
    Desktop AppImuPage — exact parity with PageAppImu widget.
    Layout: Vertical SplitView
      Top:    ParamTable for "imu"/"general"
      Bottom: Horizontal SplitView
        Left:  TabBar (bottom) with RPY / Accel / Gyro — each with rolling 500-sample GraphsView
        Right: "Use Yaw" checkbox + Vesc3DView
    Timer-driven replot at 20 ms.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs
import Vedder.vesc

Item {
    id: root

    property Commands mCommands: VescIf.commands()
    property ConfigParams mAppConf: VescIf.appConfig()
    property var _dynamicItems: []

    // Rolling data buffers (500 samples max)
    property var _seconds: []
    property var _rollVec: []; property var _pitchVec: []; property var _yawVec: []
    property var _accXVec: []; property var _accYVec: []; property var _accZVec: []
    property var _gyroXVec: []; property var _gyroYVec: []; property var _gyroZVec: []

    property double _secondCounter: 0.0
    property bool _updatePlots: false

    // Latest values for axis labels
    property double _curRoll: 0; property double _curPitch: 0; property double _curYaw: 0
    property double _curAccX: 0; property double _curAccY: 0; property double _curAccZ: 0
    property double _curGyroX: 0; property double _curGyroY: 0; property double _curGyroZ: 0

    readonly property int _maxS: 500

    ParamEditors { id: editors }

    function appendAndTrunc(arr, val) {
        arr.push(val)
        if (arr.length > _maxS) arr.splice(0, arr.length - _maxS)
    }

    Connections {
        target: RtDataStore
        function onImuDataAppended(time, roll, pitch, yaw, accX, accY, accZ, gyroX, gyroY, gyroZ) {
            appendAndTrunc(_seconds, time)
            appendAndTrunc(_rollVec, roll)
            appendAndTrunc(_pitchVec, pitch)
            appendAndTrunc(_yawVec, yaw)
            appendAndTrunc(_accXVec, accX)
            appendAndTrunc(_accYVec, accY)
            appendAndTrunc(_accZVec, accZ)
            appendAndTrunc(_gyroXVec, gyroX)
            appendAndTrunc(_gyroYVec, gyroY)
            appendAndTrunc(_gyroZVec, gyroZ)

            _secondCounter = time

            // Update 3D view (convert degrees back to radians)
            var rollRad  = roll  * Math.PI / 180.0
            var pitchRad = pitch * Math.PI / 180.0
            var yawRad   = yaw   * Math.PI / 180.0
            view3d.setRotation(rollRad, pitchRad,
                               useYawBox.checked ? yawRad : 0.0)

            _curRoll = roll; _curPitch = pitch; _curYaw = yaw
            _curAccX = accX; _curAccY = accY; _curAccZ = accZ
            _curGyroX = gyroX; _curGyroY = gyroY; _curGyroZ = gyroZ

            _updatePlots = true
        }
    }

    Timer {
        interval: 20; running: true; repeat: true
        onTriggered: {
            if (!_updatePlots) return
            _updatePlots = false
            rebuildRpy()
            rebuildAccel()
            rebuildGyro()
        }
    }

    // ── Rebuild helpers ─────────────────────────────────────────
    function updateSeries(series, xVec, yVec) {
        series.clear()
        for (var i = 0; i < xVec.length; i++)
            series.append(xVec[i], yVec[i])
    }

    function updateAxes(axX, axY, xVec, vecs, xLabel, yLabel) {
        var xMin = 1e18, xMax = -1e18
        var yMin = 1e18, yMax = -1e18
        for (var i = 0; i < xVec.length; i++) {
            if (xVec[i] < xMin) xMin = xVec[i]
            if (xVec[i] > xMax) xMax = xVec[i]
            for (var k = 0; k < vecs.length; k++) {
                if (vecs[k][i] < yMin) yMin = vecs[k][i]
                if (vecs[k][i] > yMax) yMax = vecs[k][i]
            }
        }
        if (xMin >= xMax) { xMin = 0; xMax = 1 }
        axX.min = xMin; axX.max = xMax
        axX.titleText = xLabel
        if (yMin >= yMax) { yMin = -1; yMax = 1 }
        var pad = (yMax - yMin) * 0.05
        axY.min = yMin - pad; axY.max = yMax + pad
        axY.titleText = yLabel
    }

    function rebuildRpy() {
        updateSeries(rpySeries0, _seconds, _rollVec)
        updateSeries(rpySeries1, _seconds, _pitchVec)
        updateSeries(rpySeries2, _seconds, _yawVec)
        updateAxes(rpyAxisX, rpyAxisY, _seconds, [_rollVec, _pitchVec, _yawVec],
            "Seconds (s)\nRoll: " + _curRoll.toFixed(3) + " Pitch: " + _curPitch.toFixed(3) + " Yaw: " + _curYaw.toFixed(3),
            "Angle (Deg)")
    }

    function rebuildAccel() {
        updateSeries(accelSeries0, _seconds, _accXVec)
        updateSeries(accelSeries1, _seconds, _accYVec)
        updateSeries(accelSeries2, _seconds, _accZVec)
        updateAxes(accelAxisX, accelAxisY, _seconds, [_accXVec, _accYVec, _accZVec],
            "Seconds (s)\nX: " + _curAccX.toFixed(3) + " Y: " + _curAccY.toFixed(3) + " Z: " + _curAccZ.toFixed(3),
            "Acceleration (G)")
    }

    function rebuildGyro() {
        updateSeries(gyroSeries0, _seconds, _gyroXVec)
        updateSeries(gyroSeries1, _seconds, _gyroYVec)
        updateSeries(gyroSeries2, _seconds, _gyroZVec)
        updateAxes(gyroAxisX, gyroAxisY, _seconds, [_gyroXVec, _gyroYVec, _gyroZVec],
            "Seconds (s)\nX: " + _curGyroX.toFixed(3) + " Y: " + _curGyroY.toFixed(3) + " Z: " + _curGyroZ.toFixed(3),
            "Angular Velocity (Deg/s)")
    }

    // ── Shared plot theme ───────────────────────────────────────
    property GraphsTheme _plotTheme: GraphsTheme {
        colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
        plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
        grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
    }

    // ─────────────────────────────────────────────────────────────
    //  MAIN LAYOUT
    // ─────────────────────────────────────────────────────────────
    SplitView {
        anchors.fill: parent
        orientation: Qt.Vertical

        // ── Top: param table ────────────────────────────────────
        Flickable {
            SplitView.preferredHeight: parent.height * 0.35
            SplitView.minimumHeight: 100
            clip: true; contentWidth: width; contentHeight: paramCol.height + 16
            flickableDirection: Flickable.VerticalFlick
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            ColumnLayout {
                id: paramCol
                width: parent.width
                spacing: 4
                Item { Layout.preferredHeight: 1 }
            }
        }

        // ── Bottom: plots + 3D view ────────────────────────────
        SplitView {
            orientation: Qt.Horizontal
            SplitView.fillHeight: true

            // Left: tab bar at bottom with 3 plot tabs
            ColumnLayout {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 300
                spacing: 0

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: plotTabBar.currentIndex

                    // RPY Tab
                    Item {
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 2
                            GraphsView {
                                id: rpyPlot
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                theme: root._plotTheme
                                axisX: ValueAxis { id: rpyAxisX; min: 0; max: 1; titleText: "Seconds (s)" }
                                axisY: ValueAxis { id: rpyAxisY; min: -180; max: 180; titleText: "Angle (Deg)" }
                                LineSeries { id: rpySeries0; color: Utility.getAppHexColor("blue"); width: 2; name: "Roll" }
                                LineSeries { id: rpySeries1; color: Utility.getAppHexColor("green"); width: 2; name: "Pitch" }
                                LineSeries { id: rpySeries2; color: Utility.getAppHexColor("red"); width: 2; name: "Yaw" }
                            }
                            PlotLegend { graphsView: rpyPlot }
                        }
                    }

                    // Accel Tab
                    Item {
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 2
                            GraphsView {
                                id: accelPlot
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                theme: root._plotTheme
                                axisX: ValueAxis { id: accelAxisX; min: 0; max: 1; titleText: "Seconds (s)" }
                                axisY: ValueAxis { id: accelAxisY; min: -2; max: 2; titleText: "Acceleration (G)" }
                                LineSeries { id: accelSeries0; color: Utility.getAppHexColor("blue"); width: 2; name: "X" }
                                LineSeries { id: accelSeries1; color: Utility.getAppHexColor("green"); width: 2; name: "Y" }
                                LineSeries { id: accelSeries2; color: Utility.getAppHexColor("red"); width: 2; name: "Z" }
                            }
                            PlotLegend { graphsView: accelPlot }
                        }
                    }

                    // Gyro Tab
                    Item {
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 2
                            GraphsView {
                                id: gyroPlot
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                theme: root._plotTheme
                                axisX: ValueAxis { id: gyroAxisX; min: 0; max: 1; titleText: "Seconds (s)" }
                                axisY: ValueAxis { id: gyroAxisY; min: -500; max: 500; titleText: "Angular Velocity (Deg/s)" }
                                LineSeries { id: gyroSeries0; color: Utility.getAppHexColor("blue"); width: 2; name: "X" }
                                LineSeries { id: gyroSeries1; color: Utility.getAppHexColor("green"); width: 2; name: "Y" }
                                LineSeries { id: gyroSeries2; color: Utility.getAppHexColor("red"); width: 2; name: "Z" }
                            }
                            PlotLegend { graphsView: gyroPlot }
                        }
                    }
                }

                TabBar {
                    id: plotTabBar
                    Layout.fillWidth: true
                    TabButton { text: "RPY"; topPadding: 9; bottomPadding: 9 }
                    TabButton { text: "Accel"; topPadding: 9; bottomPadding: 9 }
                    TabButton { text: "Gyro"; topPadding: 9; bottomPadding: 9 }
                }
            }

            // Right: Use Yaw checkbox + 3D view
            ColumnLayout {
                SplitView.preferredWidth: 250
                SplitView.minimumWidth: 180
                spacing: 4

                CheckBox {
                    id: useYawBox
                    text: "Use Yaw (will drift)"
                    Layout.fillWidth: true
                }

                Vesc3DView {
                    id: view3d
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 200
                }
            }
        }
    }

    function loadSubgroup(parentCol, subgroup) {
        var params = mAppConf.getParamsFromSubgroup("imu", subgroup)
        for (var p = 0; p < params.length; p++) {
            var paramName = params[p]
            if (paramName.indexOf("::sep::") === 0) {
                var sep = editors.createSeparator(parentCol, paramName.substring(7))
                if (sep) _dynamicItems.push(sep)
                continue
            }
            var e = editors.createEditorApp(parentCol, paramName)
            if (e) { e.Layout.fillWidth = true; _dynamicItems.push(e) }
        }
    }

    Component.onCompleted: {
        loadSubgroup(paramCol, "general")
        view3d.setRotation(20 * Math.PI / 180, 20 * Math.PI / 180, 0)

        // Restore IMU data from persistent C++ store
        var imuNames = ["roll", "pitch", "yaw", "accX", "accY", "accZ", "gyroX", "gyroY", "gyroZ"]
        var imuVecs  = [_rollVec, _pitchVec, _yawVec, _accXVec, _accYVec, _accZVec,
                        _gyroXVec, _gyroYVec, _gyroZVec]
        var rollPts  = RtDataStore.imuSeriesPoints("roll")
        for (var i = 0; i < rollPts.length; i++)
            _seconds.push(rollPts[i].x)

        for (var n = 0; n < imuNames.length; n++) {
            var pts = (n === 0) ? rollPts : RtDataStore.imuSeriesPoints(imuNames[n])
            for (var j = 0; j < pts.length; j++)
                imuVecs[n].push(pts[j].y)
        }
        _secondCounter = RtDataStore.imuSecondCounter
        if (_seconds.length > 0) {
            _updatePlots = true
        }
    }
}
