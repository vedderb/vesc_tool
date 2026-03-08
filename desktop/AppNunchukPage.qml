/*
    Desktop AppNunchukPage — exact parity with PageAppNunchuk widget.
    Layout: VBoxLayout
      TabWidget (General | Throttle Curve)
        General:  ParamTable of "VESC Remote" / "general" + DisplayPercentage (h=30)
        Throttle: ParamTable (fixed h~100) + throttle curve plot
      NrfPair GroupBox at bottom
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

    // NRF pairing state
    property double _pairCnt: 0.0
    property bool _pairRunning: false

    ParamEditors { id: editors }

    function updateThrottleCurve() {
        var mode = mAppConf.getParamEnum("app_chuk_conf.throttle_exp_mode")
        var valAcc = mAppConf.getParamDouble("app_chuk_conf.throttle_exp")
        var valBrk = mAppConf.getParamDouble("app_chuk_conf.throttle_exp_brake")

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
            if (name === "app_chuk_conf.throttle_exp" || name === "app_chuk_conf.throttle_exp_brake")
                updateThrottleCurve()
        }
        function onParamChangedEnum(src, name, newParam) {
            if (name === "app_chuk_conf.throttle_exp_mode")
                updateThrottleCurve()
        }
    }

    Connections {
        target: mCommands
        function onDecodedChukReceived(value) {
            var p = value * 100.0
            chukBar.value = p / 100.0
            chukLabel.text = p.toFixed(1) + " %"
        }

        function onNrfPairingRes(res) {
            if (!_pairRunning) return
            if (res === 0) { // NRF_PAIR_STARTED
                _pairCnt = nrfTimeBox.value
                nrfStartBtn.enabled = false
            } else if (res === 1) { // NRF_PAIR_OK
                nrfStartBtn.enabled = true
                _pairCnt = 0
                VescIf.emitStatusMessage("Pairing NRF Successful", true)
                _pairRunning = false
            } else if (res === 2) { // NRF_PAIR_FAIL
                nrfStartBtn.enabled = true
                _pairCnt = 0
                VescIf.emitStatusMessage("Pairing NRF Timed Out", false)
                _pairRunning = false
            }
        }
    }

    Timer {
        interval: 100; running: true; repeat: true
        onTriggered: {
            if (_pairCnt > 0.01) {
                _pairCnt -= 0.1
                if (_pairCnt <= 0.01) {
                    nrfStartBtn.enabled = true
                    _pairCnt = 0
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            TabButton { text: "General" }
            TabButton { text: "Throttle Curve" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Tab 0: General + DisplayPercentage
            Flickable {
                clip: true; contentWidth: width; contentHeight: genLayout.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: genLayout
                    width: parent.width
                    spacing: 4

                    Item { Layout.preferredHeight: 1 }

                    ColumnLayout {
                        id: genCol
                        Layout.fillWidth: true
                        spacing: 4
                    }

                    // DisplayPercentage equivalent (dual, min height 30)
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        color: "transparent"
                        border.width: 1
                        border.color: palette.mid
                        radius: 2

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 2

                            ProgressBar {
                                id: chukBar
                                Layout.fillWidth: true
                                from: -1; to: 1; value: 0
                            }

                            Label {
                                id: chukLabel
                                text: "-- %"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 80
                            }
                        }
                    }
                }
            }

            // Tab 1: Throttle Curve
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

        // NRF Pairing GroupBox (below TabWidget, matching original layout)
        GroupBox {
            title: "NRF Pairing"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                spacing: 4

                Button {
                    id: nrfStartBtn
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                    ToolTip.text: "Start Pairing"; ToolTip.visible: hovered
                    onClicked: {
                        mCommands.pairNrf(Math.round(nrfTimeBox.value * 1000))
                        _pairRunning = true
                    }
                }

                SpinBox {
                    id: nrfTimeBox
                    from: 10; to: 1000; value: 100; stepSize: 10
                    editable: true
                    property double realValue: value / 10.0
                    textFromValue: function(v) { return "Time: " + (v / 10.0).toFixed(1) + " s" }
                    valueFromText: function(t) { return Math.round(parseFloat(t.replace("Time: ", "").replace(" s", "")) * 10) || 100 }
                    ToolTip.text: "Stay in pairing mode for this amount of time"
                    ToolTip.visible: nrfTimeHover.hovered; ToolTip.delay: 500
                    HoverHandler { id: nrfTimeHover }
                }

                Label {
                    text: _pairCnt.toFixed(1)
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    Layout.preferredWidth: 40
                    background: Rectangle { border.width: 1; border.color: palette.mid; radius: 2; color: "transparent" }
                    padding: 4
                }
            }
        }
    }

    function loadSubgroup(parentCol, subgroup) {
        var params = mAppConf.getParamsFromSubgroup("VESC Remote", subgroup)
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
        loadSubgroup(tcCol, "throttle curve")
    }
}
