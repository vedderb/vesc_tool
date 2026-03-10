/*
    Desktop SampledDataPage — full feature parity with PageSampledData widget.

    Layout (top to bottom):
    ┌──────────────────────────────────────────────────────────┐
    │ TabBar: "Current" | "BEMF"                               │
    ├──────────────────────────────────────────────────────────┤
    │  Current tab:                                            │
    │   StackLayout controlled by plotModeBox:                 │
    │     page 0/1: Current chart + trace checkboxes           │
    │     page 2:   Filter response (left) | Filter coeff (rt)│
    │   Right sidebar: plotModeBox, Filter Data groupbox, Fs   │
    │                                                          │
    │  BEMF tab:                                               │
    │   Voltage chart + trace checkboxes + Truncate            │
    ├──────────────────────────────────────────────────────────┤
    │ Button bar: SampleNow, Start, TrigStart, TrigFault,     │
    │   TrigStartNosend, TrigFaultNosend, Last, Stop, Raw     │
    │   [spacer] Load, Save                                    │
    ├──────────────────────────────────────────────────────────┤
    │ Samp: __, Dec: __, [progress bar], ZoomH, ZoomV, Rescale│
    └──────────────────────────────────────────────────────────┘
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtGraphs
import Vedder.vesc

Item {
    id: sampledPage

    SampledDataHelper {
        id: helper
        Component.onCompleted: setVescInterface(VescIf)
    }

    function replotCurrent() {
        if (!helper.hasData) return

        var plotMode = plotModeBox.currentIndex  // 0=time, 1=fft, 2=filter
        var mode = plotMode === 1 ? 1 : 0       // getCurrentPlotData mode: 0=time, 1=fft

        var data = helper.getCurrentPlotData(
            mode,
            showI1.checked, showI2.checked, showI3.checked, showMcTotal.checked,
            showPosI.checked, showPhaseI.checked,
            filterBox.currentIndex, filterFreqBox.realValue, filterTapsBox.value,
            compDelayBox.checked, hammingBox.checked,
            fsBox.value, decBox.value
        )

        // Clear existing series
        for (var i = currentChart.count - 1; i >= 0; i--)
            currentChart.removeSeries(currentChart.seriesAt(i))

        // Add new series
        for (var j = 0; j < data.length; j++) {
            var s = data[j]
            var series = currentChart.createSeries(GraphsView.SeriesTypeLine, s.name)
            series.color = s.color
            series.width = 1.5
            for (var k = 0; k < s.xData.length; k++)
                series.append(s.xData[k], s.yData[k])
        }

        // Set axis labels
        if (plotMode === 1) {
            currentAxisX.titleText = "Frequency (Hz)"
            currentAxisY.titleText = "Amplitude"
        } else {
            currentAxisX.titleText = "Seconds (s)"
            currentAxisY.titleText = "Amperes (A)"
        }

        rescaleCurrentAxes()
    }

    function replotVoltage() {
        if (!helper.hasData) return

        var data = helper.getVoltagePlotData(
            showPh1.checked, showPh2.checked, showPh3.checked,
            showVGnd.checked, showPosV.checked, showPhaseV.checked,
            truncateBox.checked
        )

        for (var i = voltageChart.count - 1; i >= 0; i--)
            voltageChart.removeSeries(voltageChart.seriesAt(i))

        for (var j = 0; j < data.length; j++) {
            var s = data[j]
            var series = voltageChart.createSeries(GraphsView.SeriesTypeLine, s.name)
            series.color = s.color
            series.width = 1.5
            for (var k = 0; k < s.xData.length; k++)
                series.append(s.xData[k], s.yData[k])
        }

        voltageAxisX.titleText = "Seconds (s)"
        voltageAxisY.titleText = "Volts (V)"

        rescaleVoltageAxes()
    }

    function replotFilter() {
        if (plotModeBox.currentIndex !== 2) return

        var data = helper.getFilterPlotData(
            filterBox.currentIndex, filterFreqBox.realValue, filterTapsBox.value,
            hammingBox.checked, filterScatterBox.checked, fsBox.value
        )

        // Filter response chart (left)
        for (var i2 = filterResponseChart.count - 1; i2 >= 0; i2--)
            filterResponseChart.removeSeries(filterResponseChart.seriesAt(i2))
        if (data.length >= 2) {
            var fr = data[1]
            var frs = filterResponseChart.createSeries(GraphsView.SeriesTypeLine, fr.name)
            frs.color = fr.color
            frs.width = 1.5
            for (var k2 = 0; k2 < fr.xData.length; k2++)
                frs.append(fr.xData[k2], fr.yData[k2])
        }

        // Filter coeff chart (right)
        for (var i1 = filterCoeffChart.count - 1; i1 >= 0; i1--)
            filterCoeffChart.removeSeries(filterCoeffChart.seriesAt(i1))
        if (data.length >= 1) {
            var fc = data[0]
            var fcs = filterCoeffChart.createSeries(GraphsView.SeriesTypeLine, fc.name)
            fcs.color = fc.color
            fcs.width = 1.5
            for (var k1 = 0; k1 < fc.xData.length; k1++)
                fcs.append(fc.xData[k1], fc.yData[k1])
        }
    }

    function replotAll() {
        replotCurrent()
        replotVoltage()
        replotFilter()
    }

    function rescaleCurrentAxes() {
        var xMin = Infinity, xMax = -Infinity, yMin = Infinity, yMax = -Infinity
        for (var i = 0; i < currentChart.count; i++) {
            var s = currentChart.seriesAt(i)
            for (var j = 0; j < s.count; j++) {
                var pt = s.at(j)
                if (pt.x < xMin) xMin = pt.x
                if (pt.x > xMax) xMax = pt.x
                if (pt.y < yMin) yMin = pt.y
                if (pt.y > yMax) yMax = pt.y
            }
        }
        if (xMin < xMax) { currentAxisX.min = xMin; currentAxisX.max = xMax }
        if (yMin < yMax) {
            var pad = (yMax - yMin) * 0.05
            currentAxisY.min = yMin - pad
            currentAxisY.max = yMax + pad
        }
    }

    function rescaleVoltageAxes() {
        var xMin = Infinity, xMax = -Infinity, yMin = Infinity, yMax = -Infinity
        for (var i = 0; i < voltageChart.count; i++) {
            var s = voltageChart.seriesAt(i)
            for (var j = 0; j < s.count; j++) {
                var pt = s.at(j)
                if (pt.x < xMin) xMin = pt.x
                if (pt.x > xMax) xMax = pt.x
                if (pt.y < yMin) yMin = pt.y
                if (pt.y > yMax) yMax = pt.y
            }
        }
        if (xMin < xMax) { voltageAxisX.min = xMin; voltageAxisX.max = xMax }
        if (yMin < yMax) {
            var pad = (yMax - yMin) * 0.05
            voltageAxisY.min = yMin - pad
            voltageAxisY.max = yMax + pad
        }
    }

    Connections {
        target: helper
        function onDataReady() { replotAll() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==== Main tabbed area ====
        TabBar {
            id: mainTabBar
            Layout.fillWidth: true
            TabButton { text: "Current"; topPadding: 9; bottomPadding: 9 }
            TabButton { text: "BEMF"; topPadding: 9; bottomPadding: 9 }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: mainTabBar.currentIndex

            // ---- Tab 0: Current ----
            RowLayout {
                spacing: 0

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: plotModeBox.currentIndex === 2 ? 1 : 0

                    // Page 0: Time/FFT current plot
                    ColumnLayout {
                        spacing: 0

                        GraphsView {
                            id: currentChart
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            theme: GraphsTheme {
                                colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                                plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                                grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                            }

                            axisX: ValueAxis { id: currentAxisX; min: 0; max: 1; titleText: "Seconds (s)" }
                            axisY: ValueAxis { id: currentAxisY; min: -100; max: 100; titleText: "Amperes (A)" }
                        }

                        // Trace checkboxes row
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 4; Layout.rightMargin: 4
                            spacing: 8

                            CheckBox { id: showI1; text: "PH 1"; checked: true; onToggled: replotCurrent() }
                            CheckBox { id: showI2; text: "PH 2"; checked: true; onToggled: replotCurrent() }
                            CheckBox { id: showI3; text: "PH 3"; checked: true; onToggled: replotCurrent() }
                            CheckBox { id: showMcTotal; text: "MC total"; checked: true; onToggled: replotCurrent() }
                            CheckBox { id: showPosI; text: "Position"; checked: true; onToggled: replotCurrent() }
                            CheckBox {
                                id: showPhaseI; text: "Phase"; checked: true
                                ToolTip.text: "Show FOC motor phase"
                                ToolTip.visible: phaseIHover.hovered; ToolTip.delay: 500
                                HoverHandler { id: phaseIHover }
                                onToggled: replotCurrent()
                            }
                            Item { Layout.fillWidth: true }
                        }
                    }

                    // Page 1: Filter plots (when plotModeBox == 2)
                    SplitView {
                        orientation: Qt.Horizontal

                        // Filter response (left)
                        ColumnLayout {
                            SplitView.fillWidth: true
                            spacing: 0

                            GraphsView {
                                id: filterResponseChart
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                theme: GraphsTheme {
                                    colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                                    plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                                    grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                                }

                                axisX: ValueAxis { min: 0; max: 20000; titleText: "Frequency (Hz)" }
                                axisY: ValueAxis { min: 0; max: 1.2; titleText: "Gain" }
                            }

                            CheckBox {
                                id: filterLogScaleBox
                                text: "Logscale"
                            }
                        }

                        // Filter coefficients (right)
                        ColumnLayout {
                            SplitView.fillWidth: true
                            spacing: 0

                            GraphsView {
                                id: filterCoeffChart
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                theme: GraphsTheme {
                                    colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                                    plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                                    grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                                }

                                axisX: ValueAxis { min: 0; max: 64; titleText: "Index" }
                                axisY: ValueAxis { min: -0.1; max: 0.2; titleText: "Value" }
                            }

                            CheckBox {
                                id: filterScatterBox
                                text: "Scatterplot"
                                onToggled: replotFilter()
                            }
                        }
                    }
                }

                // Right sidebar: Plot mode + Filter settings
                ScrollView {
                    Layout.preferredWidth: 160
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: 150
                        spacing: 3

                        ComboBox {
                            id: plotModeBox
                            Layout.fillWidth: true
                            textRole: "display"
                            model: Utility.stringListModel(["Time Plot", "FFT Plot", "Filter Plot"])
                            currentIndex: 0
                            ToolTip.text: "Select what to plot"
                            ToolTip.visible: plotModeHover.hovered; ToolTip.delay: 500
                            HoverHandler { id: plotModeHover }
                            onCurrentIndexChanged: {
                                replotCurrent()
                                replotFilter()
                            }
                        }

                        GroupBox {
                            title: "Filter Data"
                            Layout.fillWidth: true

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 3

                                ComboBox {
                                    id: filterBox
                                    Layout.fillWidth: true
                                    textRole: "display"
                                    model: Utility.stringListModel(["No Filter", "FIR Filter", "Mean Filter"])
                                    currentIndex: 0
                                    onCurrentIndexChanged: { replotCurrent(); replotFilter() }
                                }

                                // F_St: 0.000 - 0.500 (double spinbox emulated)
                                SpinBox {
                                    id: filterFreqBox
                                    Layout.fillWidth: true
                                    from: 0; to: 500; value: 50; stepSize: 5; editable: true
                                    property double realValue: value / 1000.0
                                    textFromValue: function(v) { return "F_St: " + (v / 1000).toFixed(3) }
                                    valueFromText: function(t) {
                                        var s = t.replace("F_St: ", "")
                                        return Math.round(parseFloat(s) * 1000) || 50
                                    }
                                    ToolTip.text: "FIR filter stop frequency"
                                    ToolTip.visible: filterFreqHover.hovered; ToolTip.delay: 500
                                    HoverHandler { id: filterFreqHover }
                                    onValueChanged: { replotCurrent(); replotFilter() }
                                }

                                SpinBox {
                                    id: filterTapsBox
                                    Layout.fillWidth: true
                                    from: 1; to: 10; value: 6; editable: true
                                    textFromValue: function(v) { return "Taps: " + v }
                                    valueFromText: function(t) { return parseInt(t.replace("Taps: ", "")) || 6 }
                                    ToolTip.text: "FIR filter taps (2^n)"
                                    ToolTip.visible: filterTapsHover.hovered; ToolTip.delay: 500
                                    HoverHandler { id: filterTapsHover }
                                    onValueChanged: { replotCurrent(); replotFilter() }
                                }

                                CheckBox {
                                    id: compDelayBox
                                    text: "Delay Comp"
                                    checked: true
                                    ToolTip.text: "Compensate for filter delay"
                                    ToolTip.visible: compDelayHover.hovered; ToolTip.delay: 500
                                    HoverHandler { id: compDelayHover }
                                    onToggled: { replotCurrent(); replotFilter() }
                                }

                                CheckBox {
                                    id: hammingBox
                                    text: "Hamming"
                                    checked: true
                                    ToolTip.text: "Use Hamming window"
                                    ToolTip.visible: hammingHover.hovered; ToolTip.delay: 500
                                    HoverHandler { id: hammingHover }
                                    onToggled: { replotCurrent(); replotFilter() }
                                }
                            }
                        }

                        SpinBox {
                            id: fsBox
                            Layout.fillWidth: true
                            from: 1; to: 200000; stepSize: 1000; value: 40000; editable: true
                            textFromValue: function(v) { return "Fs: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("Fs: ", "")) || 40000 }
                            ToolTip.text: "Sampling frequency for FFT time axis"
                            ToolTip.visible: fsHover.hovered; ToolTip.delay: 500
                            HoverHandler { id: fsHover }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // ---- Tab 1: BEMF ----
            ColumnLayout {
                spacing: 0

                GraphsView {
                    id: voltageChart
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    theme: GraphsTheme {
                        colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                        plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                        grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                    }

                    axisX: ValueAxis { id: voltageAxisX; min: 0; max: 1; titleText: "Seconds (s)" }
                    axisY: ValueAxis { id: voltageAxisY; min: -100; max: 100; titleText: "Volts (V)" }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 4; Layout.rightMargin: 4
                    spacing: 8

                    CheckBox { id: showPh1; text: "PH 1"; checked: true; onToggled: replotVoltage() }
                    CheckBox { id: showPh2; text: "PH 2"; checked: true; onToggled: replotVoltage() }
                    CheckBox { id: showPh3; text: "PH 3"; checked: true; onToggled: replotVoltage() }
                    CheckBox { id: showVGnd; text: "Virtual Ground"; checked: true; onToggled: replotVoltage() }
                    CheckBox { id: showPosV; text: "Position"; checked: true; onToggled: replotVoltage() }
                    CheckBox {
                        id: showPhaseV; text: "Phase"; checked: true
                        ToolTip.text: "Show FOC motor phase"
                        ToolTip.visible: phaseVHover.hovered; ToolTip.delay: 500
                        HoverHandler { id: phaseVHover }
                        onToggled: replotVoltage()
                    }
                    Item { Layout.fillWidth: true }
                    CheckBox {
                        id: truncateBox; text: "Truncate"
                        ToolTip.text: "Truncate conducting phases"
                        ToolTip.visible: truncHover.hovered; ToolTip.delay: 500
                        HoverHandler { id: truncHover }
                        onToggled: replotVoltage()
                    }
                }
            }
        }

        // ==== Button bar (sampling commands) ====
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 3
            spacing: 3

            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/3ph_sine.png"
                ToolTip.text: "Sample now"; ToolTip.visible: hovered
                onClicked: helper.sampleNow(samplesBox.value, decBox.value, rawBox.checked)
            }
            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/motor.png"
                ToolTip.text: "Sample when the motor starts"; ToolTip.visible: hovered
                onClicked: helper.sampleStart(samplesBox.value, decBox.value, rawBox.checked)
            }
            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/sampl_trigger_start.png"
                ToolTip.text: "Triggered sampling when the motor starts"; ToolTip.visible: hovered
                onClicked: helper.sampleTriggerStart(samplesBox.value, decBox.value, rawBox.checked)
            }
            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/sample_trigger_fault.png"
                ToolTip.text: "Triggered sampling at fault codes"; ToolTip.visible: hovered
                onClicked: helper.sampleTriggerFault(samplesBox.value, decBox.value, rawBox.checked)
            }
            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/sample_trigger_start_nosend.png"
                ToolTip.text: "Triggered sampling when motor starts (no auto-send)"; ToolTip.visible: hovered
                onClicked: helper.sampleTriggerStartNosend(samplesBox.value, decBox.value, rawBox.checked)
            }
            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/sample_trigger_fault_nosend.png"
                ToolTip.text: "Triggered sampling at fault codes (no auto-send)"; ToolTip.visible: hovered
                onClicked: helper.sampleTriggerFaultNosend(samplesBox.value, decBox.value, rawBox.checked)
            }
            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/Upload-96.png"
                ToolTip.text: "Read last samples"; ToolTip.visible: hovered
                onClicked: helper.sampleLast(samplesBox.value, decBox.value, rawBox.checked)
            }
            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/Cancel-96.png"
                ToolTip.text: "Stop sampling"; ToolTip.visible: hovered
                onClicked: helper.sampleStop(samplesBox.value, decBox.value, rawBox.checked)
            }

            CheckBox {
                id: rawBox
                text: "Raw"
                ToolTip.text: "Use raw ADC values instead of scaled voltages and currents"
                ToolTip.visible: rawHover.hovered; ToolTip.delay: 500
                HoverHandler { id: rawHover }
            }

            Item { Layout.fillWidth: true }

            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                ToolTip.text: "Load samples from CSV file"; ToolTip.visible: hovered
                onClicked: loadDialog.open()
            }
            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/Save as-96.png"
                ToolTip.text: "Save samples to CSV file"; ToolTip.visible: hovered
                onClicked: saveDialog.open()
            }
        }

        // ==== Bottom bar (samples, decimation, progress, zoom) ====
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 3
            spacing: 3

            SpinBox {
                id: samplesBox
                from: 1; to: 1000; stepSize: 100; value: 600; editable: true
                textFromValue: function(v) { return "Samp: " + v }
                valueFromText: function(t) { return parseInt(t.replace("Samp: ", "")) || 600 }
                ToolTip.text: "Number of samples (or before trigger in triggered modes)"
                ToolTip.visible: sampHover.hovered; ToolTip.delay: 500
                HoverHandler { id: sampHover }
            }

            SpinBox {
                id: decBox
                from: 1; to: 99; value: 1; editable: true
                textFromValue: function(v) { return "Dec: " + v }
                valueFromText: function(t) { return parseInt(t.replace("Dec: ", "")) || 1 }
                ToolTip.text: "Decimation (skip samples to fit more data)"
                ToolTip.visible: decHover.hovered; ToolTip.delay: 500
                HoverHandler { id: decHover }
            }

            ProgressBar {
                Layout.fillWidth: true
                value: helper.progress
            }

            Button {
                id: zoomHBtn
                text: "⇔"; checkable: true; checked: true
                implicitWidth: 34; implicitHeight: 32
                ToolTip.text: "Enable horizontal zoom"; ToolTip.visible: hovered
            }
            Button {
                id: zoomVBtn
                text: "⇕"; checkable: true; checked: true
                implicitWidth: 34; implicitHeight: 32
                ToolTip.text: "Enable vertical zoom"; ToolTip.visible: hovered
            }
            Button {
                icon.source: "qrc" + Utility.getThemePath() + "icons/expand_off.png"
                ToolTip.text: "Rescale plots to fit"; ToolTip.visible: hovered
                onClicked: {
                    rescaleCurrentAxes()
                    rescaleVoltageAxes()
                }
            }
        }
    }

    // File dialogs
    FileDialog {
        id: saveDialog
        title: "Save CSV"
        fileMode: FileDialog.SaveFile
        nameFilters: ["CSV Files (*.csv)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (!path.toLowerCase().endsWith(".csv")) path += ".csv"
            if (helper.saveCsv(path)) {
                VescIf.emitStatusMessage("Samples saved to " + path, true)
            } else {
                VescIf.emitStatusMessage("Failed to save CSV", false)
            }
        }
    }

    FileDialog {
        id: loadDialog
        title: "Load CSV"
        fileMode: FileDialog.OpenFile
        nameFilters: ["CSV Files (*.csv)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (helper.loadCsv(path)) {
                VescIf.emitStatusMessage("Samples loaded from " + path, true)
            } else {
                VescIf.emitStatusMessage("Failed to load CSV", false)
            }
        }
    }
}
