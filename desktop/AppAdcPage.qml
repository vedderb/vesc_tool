/*
    Desktop AppAdcPage — exact parity with PageAppAdc widget.
    Layout: TabWidget (General | Mapping | Throttle Curve)
      General:  ParamTable of "ADC" / "general"
      Mapping:  ParamTable of "ADC" / "mapping" + ADC Voltage Mapping GroupBox
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
    property double _minCh1: 0.0
    property double _maxCh1: 0.0
    property double _centerCh1: 0.0
    property double _minCh2: 0.0
    property double _maxCh2: 0.0

    ParamEditors { id: editors }

    function updateThrottleCurve() {
        var mode = mAppConf.getParamEnum("app_adc_conf.throttle_exp_mode")
        var valAcc = mAppConf.getParamDouble("app_adc_conf.throttle_exp")
        var valBrk = mAppConf.getParamDouble("app_adc_conf.throttle_exp_brake")

        for (var si = throttlePlot.count - 1; si >= 0; si--)
            throttlePlot.removeSeries(throttlePlot.seriesAt(si))

        var series = throttlePlot.createSeries(GraphsView.SeriesTypeLine, "Throttle Curve")
        series.color = Utility.getAppHexColor("plot_graph1")
        series.width = 1.5

        for (var i = -1.0; i <= 1.0001; i += 0.002)
            series.append(i, Utility.throttle_curve(i, valAcc, valBrk, mode))

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
            if (name === "app_adc_conf.throttle_exp" || name === "app_adc_conf.throttle_exp_brake")
                updateThrottleCurve()
        }
        function onParamChangedEnum(src, name, newParam) {
            if (name === "app_adc_conf.throttle_exp_mode")
                updateThrottleCurve()
        }
    }

    Connections {
        target: mCommands
        function onDecodedAdcReceived(value, voltage, value2, voltage2) {
            // CH1 display
            var p1 = dualBox.checked ? (value - 0.5) * 200.0 : value * 100.0
            adc1Bar.value = Math.max(-100, Math.min(100, p1)) / 100.0
            adc1Label.text = voltage.toFixed(dualBox.checked ? 2 : 4) + " V (" + p1.toFixed(1) + " %)"

            // CH2 display
            var p2 = dualBox.checked ? (value2 - 0.5) * 200.0 : value2 * 100.0
            adc2Bar.value = Math.max(-100, Math.min(100, p2)) / 100.0
            adc2Label.text = voltage2.toFixed(4) + " V (" + p2.toFixed(1) + " %)"

            // Auto min/max
            if (_mapResetDone) {
                _mapResetDone = false
                _minCh1 = voltage; _maxCh1 = voltage
                _minCh2 = voltage2; _maxCh2 = voltage2
            } else {
                if (voltage < _minCh1) _minCh1 = voltage
                if (voltage > _maxCh1) _maxCh1 = voltage
                if (voltage2 < _minCh2) _minCh2 = voltage2
                if (voltage2 > _maxCh2) _maxCh2 = voltage2
            }

            var range = _maxCh1 - _minCh1
            var pos = voltage - _minCh1
            if (pos > range / 4.0 && pos < (3.0 * range) / 4.0)
                _centerCh1 = voltage
            else
                _centerCh1 = range / 2.0 + _minCh1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            TabButton { text: "General"; topPadding: 9; bottomPadding: 9 }
            TabButton { text: "Mapping"; topPadding: 9; bottomPadding: 9 }
            TabButton { text: "Throttle Curve"; topPadding: 9; bottomPadding: 9 }
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

                    ColumnLayout {
                        id: mapCol
                        Layout.fillWidth: true
                        spacing: 4
                    }

                    // ADC Voltage Mapping GroupBox
                    GroupBox {
                        title: "ADC Voltage Mapping"
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
                                    onClicked: { _mapResetDone = true; _minCh1 = 0; _maxCh1 = 0; _minCh2 = 0; _maxCh2 = 0 }
                                }
                                Button {
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/Ok-96.png"
                                    text: "Apply"
                                    ToolTip.text: "Apply min and max to configuration"; ToolTip.visible: hovered
                                    onClicked: {
                                        if (_maxCh1 > 1e-10) {
                                            mAppConf.updateParamDouble("app_adc_conf.voltage_start", _minCh1)
                                            mAppConf.updateParamDouble("app_adc_conf.voltage_end", _maxCh1)
                                            mAppConf.updateParamDouble("app_adc_conf.voltage_center", _centerCh1)
                                            mAppConf.updateParamDouble("app_adc_conf.voltage2_start", _minCh2)
                                            mAppConf.updateParamDouble("app_adc_conf.voltage2_end", _maxCh2)
                                            VescIf.emitStatusMessage("Start, End and Center ADC Voltages Applied", true)
                                        } else {
                                            VescIf.emitStatusMessage("Applying Voltages Failed", false)
                                        }
                                    }
                                }
                            }

                            GridLayout {
                                columns: 5
                                Layout.fillWidth: true
                                columnSpacing: 2; rowSpacing: 2

                                // CH1 row
                                Label { text: "CH1" }
                                Label { text: "Min: " + _minCh1.toFixed(2); padding: 4
                                    background: Rectangle { border.width: 1; border.color: palette.mid; radius: 2; color: "transparent" } }
                                Label { text: "Max: " + _maxCh1.toFixed(2); padding: 4
                                    background: Rectangle { border.width: 1; border.color: palette.mid; radius: 2; color: "transparent" } }
                                Label { text: "Center: " + _centerCh1.toFixed(2); padding: 4
                                    background: Rectangle { border.width: 1; border.color: palette.mid; radius: 2; color: "transparent" } }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    ProgressBar { id: adc1Bar; Layout.fillWidth: true; from: -1; to: 1; value: 0; Layout.preferredHeight: 20 }
                                    Label { id: adc1Label; text: "-- V (-- %)"; font.pointSize: 11; color: Utility.getAppHexColor("lightText") }
                                }

                                // CH2 row
                                Label { text: "CH2" }
                                Label { text: "Min: " + _minCh2.toFixed(2); padding: 4
                                    background: Rectangle { border.width: 1; border.color: palette.mid; radius: 2; color: "transparent" } }
                                Label { text: "Max: " + _maxCh2.toFixed(2); padding: 4
                                    background: Rectangle { border.width: 1; border.color: palette.mid; radius: 2; color: "transparent" } }
                                Item { } // no center for CH2
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    ProgressBar { id: adc2Bar; Layout.fillWidth: true; from: -1; to: 1; value: 0; Layout.preferredHeight: 20 }
                                    Label { id: adc2Label; text: "-- V (-- %)"; font.pointSize: 11; color: Utility.getAppHexColor("lightText") }
                                }
                            }
                        }
                    }
                }
            }

            // Tab 2: Throttle Curve
            ColumnLayout {
                spacing: 0

                Flickable {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100; Layout.maximumHeight: 100
                    clip: true; contentWidth: width; contentHeight: tcCol.height + 16
                    flickableDirection: Flickable.VerticalFlick
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    ColumnLayout { id: tcCol; width: parent.width; spacing: 4; Item { Layout.preferredHeight: 1 } }
                }

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
        var params = mAppConf.getParamsFromSubgroup("ADC", subgroup)
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
