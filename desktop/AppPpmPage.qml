/*
    Desktop AppPpmPage — exact parity with PageAppPpm widget.
    Layout: TabWidget (General | Mapping | Throttle Curve)
      General:  ParamTable of "PPM" / "general"
      Mapping:  ParamTable of "PPM" / "mapping" + PPM Mapping GroupBox
      Throttle: ParamTable (fixed height ~100) + throttle curve plot
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs
import Vedder.vesc

Item {
    id: root

    property ConfigParams mAppConf: VescIf.appConfig()
    property Commands mCommands: VescIf.commands()
    property var _dynamicItems: []
    property bool _mapResetDone: true
    property double _mapMinVal: 0.0
    property double _mapMaxVal: 0.0
    property double _mapCenterVal: 0.0

    ParamEditors { id: editors }

    function updateThrottleCurve() {
        var mode = mAppConf.getParamEnum("app_ppm_conf.throttle_exp_mode")
        var valAcc = mAppConf.getParamDouble("app_ppm_conf.throttle_exp")
        var valBrk = mAppConf.getParamDouble("app_ppm_conf.throttle_exp_brake")

        for (var si = throttlePlot.count - 1; si >= 0; si--)
            throttlePlot.removeSeries(throttlePlot.seriesAt(si))

        var series = throttlePlot.createSeries(GraphsView.SeriesTypeLine, "Throttle Curve")
        series.color = Utility.getAppHexColor("plot_graph1")
        series.width = 1.5

        for (var i = -1.0; i <= 1.0001; i += 0.002)
            series.append(i, Utility.throttle_curve(i, valAcc, valBrk, mode))

        // Auto-rescale
        throttleAxisX.min = -1.0; throttleAxisX.max = 1.0
        var yMin = -1.0, yMax = 1.0
        for (var j = 0; j < series.count; j++) {
            var pt = series.at(j)
            if (pt.y < yMin) yMin = pt.y
            if (pt.y > yMax) yMax = pt.y
        }
        var pad = (yMax - yMin) * 0.05
        throttleAxisY.min = yMin - pad
        throttleAxisY.max = yMax + pad
    }

    Connections {
        target: mAppConf
        function onParamChangedDouble(src, name, newParam) {
            if (name === "app_ppm_conf.throttle_exp" || name === "app_ppm_conf.throttle_exp_brake")
                updateThrottleCurve()
        }
        function onParamChangedEnum(src, name, newParam) {
            if (name === "app_ppm_conf.throttle_exp_mode")
                updateThrottleCurve()
        }
    }

    Connections {
        target: mCommands
        function onDecodedPpmReceived(value, lastLen) {
            // PPM mapping display update
            var minNow = mAppConf.getParamDouble("app_ppm_conf.pulse_start")
            var maxNow = mAppConf.getParamDouble("app_ppm_conf.pulse_end")
            var centerNow = mAppConf.getParamDouble("app_ppm_conf.pulse_center")

            // VESC Tool preview (using current config mapping)
            var p
            if (dualBox.checked) {
                if (lastLen < centerNow)
                    p = Utility.map(lastLen, minNow, centerNow, -100.0, 0.0)
                else
                    p = Utility.map(lastLen, centerNow, maxNow, 0.0, 100.0)
            } else {
                p = Utility.map(lastLen, minNow, maxNow, 0.0, 100.0)
            }
            ppmToolBar.value = Math.max(-100, Math.min(100, p)) / 100.0
            ppmToolLabel.text = lastLen.toFixed(4) + " ms (" + p.toFixed(1) + " %)"

            // VESC Firmware preview
            var p2 = dualBox.checked ? value * 100.0 : (value + 1.0) * 50.0
            ppmFwBar.value = Math.max(-100, Math.min(100, p2)) / 100.0
            ppmFwLabel.text = lastLen.toFixed(4) + " ms (" + p2.toFixed(1) + " %)"

            // Auto min/max tracking
            if (_mapResetDone) {
                _mapResetDone = false
                _mapMinVal = lastLen
                _mapMaxVal = lastLen
            } else {
                if (lastLen < _mapMinVal && lastLen > 1e-3) _mapMinVal = lastLen
                if (lastLen > _mapMaxVal) _mapMaxVal = lastLen
            }

            var range = _mapMaxVal - _mapMinVal
            var pos = lastLen - _mapMinVal
            if (pos > range / 4.0 && pos < (3.0 * range) / 4.0)
                _mapCenterVal = lastLen
            else
                _mapCenterVal = range / 2.0 + _mapMinVal
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            TabButton { text: "General" }
            TabButton { text: "Mapping" }
            TabButton { text: "Throttle Curve" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Tab 0: General
            Flickable {
                clip: true; contentWidth: width; contentHeight: genCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout { id: genCol; width: parent.width; spacing: 4; Item { Layout.preferredHeight: 1 } }
            }

            // Tab 1: Mapping
            Flickable {
                clip: true; contentWidth: width; contentHeight: mapLayout.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: mapLayout
                    width: parent.width
                    spacing: 4

                    Item { Layout.preferredHeight: 1 }

                    // Dynamic params inserted here via mapCol
                    ColumnLayout {
                        id: mapCol
                        Layout.fillWidth: true
                        spacing: 4
                    }

                    // PPM Pulselength Mapping GroupBox
                    GroupBox {
                        title: "PPM Pulselength Mapping"
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 4

                            RowLayout {
                                spacing: 4
                                CheckBox {
                                    id: dualBox
                                    text: "Use Centered Control"
                                    ToolTip.text: "Show centered graph, which is how the centered control modes interpret the throttle."
                                    ToolTip.visible: dualHover.hovered; ToolTip.delay: 500
                                    HoverHandler { id: dualHover }
                                }
                                Button {
                                    text: "Reset"
                                    ToolTip.text: "Reset min and max"; ToolTip.visible: hovered
                                    onClicked: { _mapResetDone = true; _mapMinVal = 0; _mapMaxVal = 0 }
                                }
                                Button {
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/Ok-96.png"
                                    text: "Apply"
                                    ToolTip.text: "Apply min, max and center to VESC Tool configuration"; ToolTip.visible: hovered
                                    onClicked: {
                                        if (_mapMaxVal > 1e-10) {
                                            mAppConf.updateParamDouble("app_ppm_conf.pulse_start", _mapMinVal)
                                            mAppConf.updateParamDouble("app_ppm_conf.pulse_end", _mapMaxVal)
                                            mAppConf.updateParamDouble("app_ppm_conf.pulse_center", _mapCenterVal)
                                            VescIf.emitStatusMessage("Start, End and Center Pulselengths Applied", true)
                                        } else {
                                            VescIf.emitStatusMessage("Applying Pulselengths Failed", false)
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                spacing: 2
                                Label { text: "Min: " + _mapMinVal.toFixed(4) + " ms"; Layout.fillWidth: true
                                    background: Rectangle { border.width: 1; border.color: palette.mid; radius: 2; color: "transparent" }
                                    padding: 4
                                }
                                Label { text: "Max: " + _mapMaxVal.toFixed(4) + " ms"; Layout.fillWidth: true
                                    background: Rectangle { border.width: 1; border.color: palette.mid; radius: 2; color: "transparent" }
                                    padding: 4
                                }
                                Label { text: "Center: " + _mapCenterVal.toFixed(4) + " ms"; Layout.fillWidth: true
                                    background: Rectangle { border.width: 1; border.color: palette.mid; radius: 2; color: "transparent" }
                                    padding: 4
                                }
                            }

                            GridLayout {
                                columns: 2
                                Layout.fillWidth: true
                                columnSpacing: 4; rowSpacing: 2

                                Label { text: "VESC Tool" }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    ProgressBar { id: ppmToolBar; Layout.fillWidth: true; from: -1; to: 1; value: 0; Layout.preferredHeight: 25 }
                                    Label { id: ppmToolLabel; text: "-- ms (-- %)"; font.pixelSize: 11; color: Utility.getAppHexColor("lightText") }
                                }

                                Label { text: "VESC Firmware" }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    ProgressBar { id: ppmFwBar; Layout.fillWidth: true; from: -1; to: 1; value: 0; Layout.preferredHeight: 25 }
                                    Label { id: ppmFwLabel; text: "-- ms (-- %)"; font.pixelSize: 11; color: Utility.getAppHexColor("lightText") }
                                }
                            }
                        }
                    }
                }
            }

            // Tab 2: Throttle Curve
            ColumnLayout {
                spacing: 0

                // Params (fixed height ~100, matching original)
                Flickable {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    Layout.maximumHeight: 100
                    clip: true; contentWidth: width; contentHeight: tcCol.height + 16
                    flickableDirection: Flickable.VerticalFlick
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    ColumnLayout { id: tcCol; width: parent.width; spacing: 4; Item { Layout.preferredHeight: 1 } }
                }

                // Throttle curve plot
                GraphsView {
                    id: throttlePlot
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    theme: GraphsTheme {
                        colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                        plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                        grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                    }

                    axisX: ValueAxis { id: throttleAxisX; min: -1; max: 1; titleText: "Throttle Value" }
                    axisY: ValueAxis { id: throttleAxisY; min: -1; max: 1; titleText: "Output Value" }
                }
            }
        }
    }

    function loadSubgroup(parentCol, subgroup) {
        var params = mAppConf.getParamsFromSubgroup("PPM", subgroup)
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
        loadSubgroup(genCol, "general")
        loadSubgroup(mapCol, "mapping")
        loadSubgroup(tcCol, "throttle curve")
    }
}
