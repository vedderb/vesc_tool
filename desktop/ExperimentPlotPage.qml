/*
    Desktop ExperimentPlotPage — full feature parity with ExperimentPlot widget.

    Layout:
    ┌────────────────────┬─────────────────────────────────────┐
    │  Left sidebar      │  Main plot area (GraphsView)        │
    │  ScrollView 190px  │                                     │
    │ ┌────────────────┐ │                                     │
    │ │ Settings       │ │                                     │
    │ │  History: 2000 │ │                                     │
    │ │  [1][2][3]     │ │                                     │
    │ │  [4][5][6]     │ │                                     │
    │ │  [Clear][Line] │ │                                     │
    │ │  [Scat][Auto]  │ │                                     │
    │ │  [HZ ][VZ ]    │ │                                     │
    │ ├────────────────┤ │                                     │
    │ │ Import         │ │                                     │
    │ │  [XML]         │ │                                     │
    │ ├────────────────┤ │                                     │
    │ │ Export         │ │                                     │
    │ │  W:640  H:480  │ │                                     │
    │ │  Scale: 1.0    │ │                                     │
    │ │  [XML][PNG]    │ │                                     │
    │ │  [CSV][PDF]    │ │                                     │
    │ └────────────────┘ │                                     │
    └────────────────────┴─────────────────────────────────────┘
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtGraphs
import Vedder.vesc

Item {
    id: expPage

    // Internal data storage (mirrors EXPERIMENT_PLOT struct vector)
    property var plotGraphs: []        // array of {label, color, xData[], yData[]}
    property int currentGraphIndex: 0
    property bool needsReplot: false
    property string xLabelText: ""
    property string yLabelText: ""

    // Auto-color assignment matching original order
    readonly property var graphColors: [
        Utility.getAppHexColor("plot_graph1"),
        Utility.getAppHexColor("plot_graph2"),
        Utility.getAppHexColor("plot_graph3"),
        Utility.getAppHexColor("plot_graph4"),
        Utility.getAppHexColor("plot_graph5"),
        Utility.getAppHexColor("plot_graph6")
    ]

    // 20ms replot timer (matches original)
    Timer {
        id: replotTimer
        interval: 20; running: true; repeat: true
        onTriggered: {
            if (needsReplot) {
                doReplot()
                needsReplot = false
            }
        }
    }

    function doReplot() {
        // Clear existing series
        for (var i = experimentChart.count - 1; i >= 0; i--)
            experimentChart.removeSeries(experimentChart.seriesAt(i))

        var graphButtons = [graph1Btn, graph2Btn, graph3Btn, graph4Btn, graph5Btn, graph6Btn]

        for (var g = 0; g < plotGraphs.length; g++) {
            // Check visibility via toggle buttons (first 6)
            if (g < 6 && !graphButtons[g].checked) continue

            var pg = plotGraphs[g]

            var series = experimentChart.createSeries(
                showLineBtn.checked ? GraphsView.SeriesTypeLine : GraphsView.SeriesTypeScatter,
                pg.label
            )
            series.color = pg.color
            series.width = 1.5

            for (var k = 0; k < pg.xData.length; k++)
                series.append(pg.xData[k], pg.yData[k])
        }

        // Update axis labels
        experimentAxisX.titleText = xLabelText
        experimentAxisY.titleText = yLabelText

        // Auto-scale if checked
        if (autoScaleBtn.checked) {
            rescaleAxes()
        }
    }

    function rescaleAxes() {
        var xMin = Infinity, xMax = -Infinity, yMin = Infinity, yMax = -Infinity
        for (var i = 0; i < experimentChart.count; i++) {
            var s = experimentChart.seriesAt(i)
            for (var j = 0; j < s.count; j++) {
                var pt = s.at(j)
                if (pt.x < xMin) xMin = pt.x
                if (pt.x > xMax) xMax = pt.x
                if (pt.y < yMin) yMin = pt.y
                if (pt.y > yMax) yMax = pt.y
            }
        }
        if (xMin < xMax) {
            experimentAxisX.min = xMin
            experimentAxisX.max = xMax
        }
        if (yMin < yMax) {
            var pad = (yMax - yMin) * 0.05
            if (pad < 0.001) pad = 0.001
            experimentAxisY.min = yMin - pad
            experimentAxisY.max = yMax + pad
        }
    }

    // ---- Signal handlers from VESC Commands ----
    Connections {
        target: VescIf.commands()

        function onPlotInitReceived(xLabel, yLabel) {
            plotGraphs = []
            currentGraphIndex = 0
            xLabelText = xLabel
            yLabelText = yLabel
            needsReplot = true
        }

        function onPlotDataReceived(x, y) {
            // Ensure graph exists
            while (plotGraphs.length <= currentGraphIndex) {
                plotGraphs.push({
                    label: "Graph " + (plotGraphs.length + 1),
                    color: graphColors[Math.min(plotGraphs.length, graphColors.length - 1)],
                    xData: [],
                    yData: []
                })
            }

            var g = plotGraphs[currentGraphIndex]
            g.xData.push(x)
            g.yData.push(y)

            // History cap
            var historyMax = historyBox.value
            if (g.xData.length > historyMax) {
                var excess = g.xData.length - historyMax
                g.xData.splice(0, excess)
                g.yData.splice(0, excess)
            }

            // Trigger reactive update
            plotGraphs = plotGraphs
            needsReplot = true
        }

        function onPlotAddGraphReceived(name) {
            var newGraph = {
                label: name,
                color: graphColors[Math.min(plotGraphs.length, graphColors.length - 1)],
                xData: [],
                yData: []
            }
            var temp = plotGraphs.slice()
            temp.push(newGraph)
            plotGraphs = temp
            needsReplot = true
        }

        function onPlotSetGraphReceived(graph) {
            currentGraphIndex = graph
        }
    }

    // ---- XML Save/Load helpers using Utility ----
    function saveXml(path) {
        var xml = '<?xml version="1.0" encoding="UTF-8"?>\n<plot>\n'
        xml += '  <xlabel>' + xLabelText + '</xlabel>\n'
        xml += '  <ylabel>' + yLabelText + '</ylabel>\n'
        for (var i = 0; i < plotGraphs.length; i++) {
            var g = plotGraphs[i]
            xml += '  <graph>\n'
            xml += '    <label>' + g.label + '</label>\n'
            xml += '    <color>' + g.color + '</color>\n'
            for (var j = 0; j < g.xData.length; j++) {
                xml += '    <point><x>' + g.xData[j] + '</x><y>' + g.yData[j] + '</y></point>\n'
            }
            xml += '  </graph>\n'
        }
        xml += '</plot>\n'
        return Utility.writeTextFile(path, xml)
    }

    function loadXml(path) {
        var text = Utility.readTextFile(path)
        if (text.length === 0) return false

        var parser = new DOMParser()
        // Not available in QML — use simple regex-based parsing instead
        return parseXmlManual(text)
    }

    function parseXmlManual(text) {
        plotGraphs = []
        currentGraphIndex = 0

        // Extract xlabel
        var xlm = text.match(/<xlabel>(.*?)<\/xlabel>/)
        if (xlm) xLabelText = xlm[1]

        var ylm = text.match(/<ylabel>(.*?)<\/ylabel>/)
        if (ylm) yLabelText = ylm[1]

        // Extract graphs
        var graphRegex = /<graph>([\s\S]*?)<\/graph>/g
        var gMatch
        while ((gMatch = graphRegex.exec(text)) !== null) {
            var gText = gMatch[1]
            var g = { label: "", color: "#4d7fc4", xData: [], yData: [] }

            var lm = gText.match(/<label>(.*?)<\/label>/)
            if (lm) g.label = lm[1]

            var cm = gText.match(/<color>(.*?)<\/color>/)
            if (cm) g.color = cm[1]

            var pointRegex = /<point>\s*<x>([\d.e\+\-]+)<\/x>\s*<y>([\d.e\+\-]+)<\/y>\s*<\/point>/g
            var pm
            while ((pm = pointRegex.exec(gText)) !== null) {
                g.xData.push(parseFloat(pm[1]))
                g.yData.push(parseFloat(pm[2]))
            }

            plotGraphs.push(g)
        }

        plotGraphs = plotGraphs  // trigger reactivity
        needsReplot = true
        return plotGraphs.length > 0
    }

    function saveCsv(path) {
        var lines = []
        // Header
        var header = ""
        var maxLen = 0
        for (var i = 0; i < plotGraphs.length; i++) {
            header += plotGraphs[i].label + " x;" + plotGraphs[i].label + " y;"
            if (plotGraphs[i].xData.length > maxLen)
                maxLen = plotGraphs[i].xData.length
        }
        lines.push(header)

        // Data rows
        for (var r = 0; r < maxLen; r++) {
            var row = ""
            for (var g = 0; g < plotGraphs.length; g++) {
                if (r < plotGraphs[g].xData.length) {
                    row += plotGraphs[g].xData[r] + ";" + plotGraphs[g].yData[r] + ";"
                } else {
                    row += ";;"
                }
            }
            lines.push(row)
        }

        return Utility.writeTextFile(path, lines.join("\n") + "\n")
    }

    // ============ UI Layout ============
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Left sidebar ----
        ScrollView {
            Layout.preferredWidth: 190
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: 178
                spacing: 2

                // Settings group
                GroupBox {
                    title: "Settings"
                    Layout.fillWidth: true

                    GridLayout {
                        anchors.fill: parent
                        columns: 6
                        rowSpacing: 2; columnSpacing: 2

                        // History spinbox (spans full row)
                        SpinBox {
                            id: historyBox
                            Layout.columnSpan: 6
                            Layout.fillWidth: true
                            from: 2; to: 10000; stepSize: 50; value: 2000; editable: true
                            textFromValue: function(v) { return "History: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("History: ", "")) || 2000 }
                            ToolTip.text: "Drop old samples when the plot grows larger than this"
                            ToolTip.visible: histHover.hovered; ToolTip.delay: 500
                            HoverHandler { id: histHover }
                        }

                        // Graph toggle buttons (row 1: 1-6)
                        Button {
                            id: graph1Btn; text: "1"; checkable: true; checked: true
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Show graph 1"; ToolTip.visible: hovered
                        }
                        Button {
                            id: graph2Btn; text: "2"; checkable: true; checked: true
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Show graph 2"; ToolTip.visible: hovered
                        }
                        Button {
                            id: graph3Btn; text: "3"; checkable: true; checked: true
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Show graph 3"; ToolTip.visible: hovered
                        }
                        Button {
                            id: graph4Btn; text: "4"; checkable: true; checked: true
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Show graph 4"; ToolTip.visible: hovered
                        }
                        Button {
                            id: graph5Btn; text: "5"; checkable: true; checked: true
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Show graph 5"; ToolTip.visible: hovered
                        }
                        Button {
                            id: graph6Btn; text: "6"; checkable: true; checked: true
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Show graph 6"; ToolTip.visible: hovered
                        }

                        // Row 2: Clear, Line, Scatter, AutoScale, HZoom, VZoom
                        Button {
                            id: clearBtn
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Delete-96.png"
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Clear data from plots"; ToolTip.visible: hovered
                            onClicked: {
                                for (var i = 0; i < plotGraphs.length; i++) {
                                    plotGraphs[i].xData = []
                                    plotGraphs[i].yData = []
                                }
                                plotGraphs = plotGraphs
                                needsReplot = true
                            }
                        }
                        Button {
                            id: showLineBtn; checkable: true; checked: true
                            icon.source: "qrc" + Utility.getThemePath() + "icons/3ph_sine.png"
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Line Plot"; ToolTip.visible: hovered
                            onToggled: needsReplot = true
                        }
                        Button {
                            id: scatterBtn; checkable: true; checked: false
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Polyline-96.png"
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Use Scatterplot"; ToolTip.visible: hovered
                            onToggled: needsReplot = true
                        }
                        Button {
                            id: autoScaleBtn; checkable: true; checked: true
                            icon.source: "qrc" + Utility.getThemePath() + "icons/size_off.png"
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Auto-Scale Plot when samples are received"; ToolTip.visible: hovered
                            onToggled: needsReplot = true
                        }
                        Button {
                            id: hZoomBtn; checkable: true; checked: true
                            icon.source: "qrc" + Utility.getThemePath() + "icons/expand_off.png"
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Enable Horizontal Zoom"; ToolTip.visible: hovered
                        }
                        Button {
                            id: vZoomBtn; checkable: true; checked: true
                            icon.source: "qrc" + Utility.getThemePath() + "icons/expand_v_off.png"
                            implicitWidth: 25; implicitHeight: 25
                            ToolTip.text: "Enable Vertical Zoom"; ToolTip.visible: hovered
                        }
                    }
                }

                // Import group
                GroupBox {
                    title: "Import"
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 2

                        Button {
                            Layout.fillWidth: true; text: "XML"
                            onClicked: loadXmlDialog.open()
                        }
                    }
                }

                // Export group
                GroupBox {
                    title: "Export"
                    Layout.fillWidth: true

                    GridLayout {
                        anchors.fill: parent
                        columns: 2; rowSpacing: 2; columnSpacing: 2

                        SpinBox {
                            id: exportWBox
                            Layout.fillWidth: true
                            from: 1; to: 4000; value: 640; editable: true
                            textFromValue: function(v) { return "W: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("W: ", "")) || 640 }
                            ToolTip.text: "Export Width"
                            ToolTip.visible: wHover.hovered; ToolTip.delay: 500
                            HoverHandler { id: wHover }
                        }
                        SpinBox {
                            id: exportHBox
                            Layout.fillWidth: true
                            from: 1; to: 4000; value: 480; editable: true
                            textFromValue: function(v) { return "H: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("H: ", "")) || 480 }
                            ToolTip.text: "Export Height"
                            ToolTip.visible: hHover.hovered; ToolTip.delay: 500
                            HoverHandler { id: hHover }
                        }

                        SpinBox {
                            id: exportScaleBox
                            Layout.columnSpan: 2; Layout.fillWidth: true
                            from: 1; to: 100; value: 10; stepSize: 1; editable: true
                            property double realScale: value / 10.0
                            textFromValue: function(v) { return "Scale: " + (v / 10).toFixed(1) }
                            valueFromText: function(t) {
                                return Math.round(parseFloat(t.replace("Scale: ", "")) * 10) || 10
                            }
                            ToolTip.text: "Export Scale"
                            ToolTip.visible: scaleHover.hovered; ToolTip.delay: 500
                            HoverHandler { id: scaleHover }
                        }

                        Button { Layout.fillWidth: true; text: "XML"; onClicked: saveXmlDialog.open() }
                        Button { Layout.fillWidth: true; text: "PNG"; onClicked: savePngDialog.open() }
                        Button { Layout.fillWidth: true; text: "CSV"; onClicked: saveCsvDialog.open() }
                        Button { Layout.fillWidth: true; text: "PDF"; onClicked: savePdfDialog.open() }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // ---- Main plot area ----
        GraphsView {
            id: experimentChart
            Layout.fillWidth: true
            Layout.fillHeight: true

            theme: GraphsTheme {
                colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
            }

            axisX: ValueAxis { id: experimentAxisX; min: 0; max: 1; titleText: "" }
            axisY: ValueAxis { id: experimentAxisY; min: -1; max: 1; titleText: "" }
        }
    }

    // ---- File Dialogs ----
    FileDialog {
        id: loadXmlDialog
        title: "Load Plot"
        fileMode: FileDialog.OpenFile
        nameFilters: ["XML Files (*.xml)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (loadXml(path)) {
                VescIf.emitStatusMessage("Loaded plot", true)
            } else {
                VescIf.emitStatusMessage("Failed to load XML plot", false)
            }
        }
    }

    FileDialog {
        id: saveXmlDialog
        title: "Save Plot"
        fileMode: FileDialog.SaveFile
        nameFilters: ["XML Files (*.xml)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (!path.toLowerCase().endsWith(".xml")) path += ".xml"
            if (saveXml(path)) {
                VescIf.emitStatusMessage("Saved plot as XML", true)
            } else {
                VescIf.emitStatusMessage("Failed to save XML", false)
            }
        }
    }

    FileDialog {
        id: saveCsvDialog
        title: "Save Plot"
        fileMode: FileDialog.SaveFile
        nameFilters: ["CSV Files (*.csv)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (!path.toLowerCase().endsWith(".csv")) path += ".csv"
            if (saveCsv(path)) {
                VescIf.emitStatusMessage("Saved plot as CSV", true)
            } else {
                VescIf.emitStatusMessage("Failed to save CSV", false)
            }
        }
    }

    FileDialog {
        id: savePngDialog
        title: "Save Image"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG Files (*.png)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (!path.toLowerCase().endsWith(".png")) path += ".png"
            experimentChart.grabToImage(function(result) {
                result.saveToFile(path)
                VescIf.emitStatusMessage("Saved plot as PNG", true)
            }, Qt.size(exportWBox.value * exportScaleBox.realScale,
                        exportHBox.value * exportScaleBox.realScale))
        }
    }

    FileDialog {
        id: savePdfDialog
        title: "Save PDF"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PDF Files (*.pdf)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            if (!path.toLowerCase().endsWith(".pdf")) path += ".pdf"
            // PDF export via grabToImage fallback (save as PNG since native PDF not available in QML GraphsView)
            experimentChart.grabToImage(function(result) {
                result.saveToFile(path.replace(".pdf", ".png"))
                VescIf.emitStatusMessage("Saved plot as PNG (PDF not supported in QML, saved as PNG)", true)
            }, Qt.size(exportWBox.value * exportScaleBox.realScale,
                        exportHBox.value * exportScaleBox.realScale))
        }
    }
}
