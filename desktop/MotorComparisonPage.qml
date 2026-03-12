import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtGraphs
import Vedder.vesc

Item {
    id: root

    property var plotColors: [
        Utility.getAppHexColor("plot_graph1"),
        "#FF00FF",
        Utility.getAppHexColor("plot_graph2"),
        Utility.getAppHexColor("plot_graph3"),
        "#00FFFF",
        Utility.getAppHexColor("plot_graph4")
    ]

    // ---- Data item definitions (matches original exactly) ----
    readonly property var dataItems: [
        { name: "Efficiency",       unit: "%",    decimals: 1, key: "efficiency",        plottable: true  },
        { name: "Mot Loss Tot",     unit: "W",    decimals: 1, key: "loss_motor_tot",    plottable: true  },
        { name: "Mot Loss Res",     unit: "W",    decimals: 1, key: "loss_motor_res",    plottable: true  },
        { name: "Mot Loss Other",   unit: "W",    decimals: 1, key: "loss_motor_other",  plottable: true  },
        { name: "Gearing Loss",     unit: "W",    decimals: 1, key: "loss_gearing",      plottable: true  },
        { name: "Total Losses",     unit: "W",    decimals: 1, key: "loss_tot",          plottable: true  },
        { name: "iq (per motor)",   unit: "A",    decimals: 1, key: "iq",                plottable: true  },
        { name: "id (per motor)",   unit: "A",    decimals: 1, key: "id",                plottable: true  },
        { name: "i_abs (per motor)",unit: "A",    decimals: 1, key: "i_mag",             plottable: true  },
        { name: "Power In",         unit: "W",    decimals: 1, key: "p_in",              plottable: true  },
        { name: "Power Out",        unit: "W",    decimals: 1, key: "p_out",             plottable: true  },
        { name: "Vq",               unit: "V",    decimals: 1, key: "vq",                plottable: true  },
        { name: "Vd",               unit: "V",    decimals: 1, key: "vd",                plottable: true  },
        { name: "VBus Min",         unit: "V",    decimals: 1, key: "vbus_min",          plottable: true  },
        { name: "Torque Out",       unit: "Nm",   decimals: 3, key: "torque_out",        plottable: true  },
        { name: "Torque Shaft",     unit: "Nm",   decimals: 3, key: "torque_motor_shaft",plottable: true  },
        { name: "RPM Out",          unit: "",     decimals: 1, key: "rpm_out",           plottable: true  },
        { name: "RPM Shaft",        unit: "",     decimals: 1, key: "rpm_motor_shaft",   plottable: true  },
        { name: "ExtraVal",         unit: "",     decimals: 1, key: "extraVal",          plottable: true  },
        { name: "ExtraVal2",        unit: "",     decimals: 1, key: "extraVal2",         plottable: true  },
        { name: "ExtraVal3",        unit: "",     decimals: 1, key: "extraVal3",         plottable: true  },
        { name: "ExtraVal4",        unit: "",     decimals: 1, key: "extraVal4",         plottable: true  },
        { name: "ERPM",             unit: "",     decimals: 1, key: "erpm",              plottable: true  },
        { name: "km/h",             unit: "km/h", decimals: 1, key: "km_h",             plottable: true  },
        { name: "mph",              unit: "mph",  decimals: 1, key: "mph",              plottable: false },
        { name: "wh/km",            unit: "wh/km",decimals: 1, key: "wh_km",            plottable: false },
        { name: "wh/mi",            unit: "wh/mi",decimals: 1, key: "wh_mi",            plottable: false },
        { name: "KV (BLDC)",        unit: "RPM/V",decimals: 1, key: "kv_bldc",          plottable: false },
        { name: "KV Noload (BLDC)", unit: "RPM/V",decimals: 1, key: "kv_bldc_noload",   plottable: false }
    ]

    // ---- State ----
    property int testMode: 0  // 0=Torque, 1=RPM, 2=RPM/Power, 3=Exp, 4=VBus, 5=VB+FW, 6=VB+RPM
    property bool liveUpdate: true
    property bool runDone: false
    property var lastResult: null
    property double verticalLineX: -1

    // Motor A/B scale arrays (mutable copy for scale spinboxes)
    property var m1Scales: []
    property var m2Scales: []

    MotorComparisonHelper { id: compHelper }

    Component.onCompleted: {
        compHelper.setVescInterface(VescIf)
        // Initialize scale arrays
        var s = []
        for (var i = 0; i < dataItems.length; i++) s.push(1.0)
        m1Scales = s.slice()
        m2Scales = s.slice()
        // Initialize data models
        for (var j = 0; j < dataItems.length; j++) {
            m1DataModel.append({ itemName: dataItems[j].name, itemValue: "" })
            m2DataModel.append({ itemName: dataItems[j].name, itemValue: "" })
        }
    }

    // ---- File dialogs ----
    FileDialog {
        id: m1ConfDialog
        title: "McConf XML File"
        nameFilters: ["XML files (*.xml)"]
        onAccepted: {
            m1ConfFileEdit.text = selectedFile.toString().replace("file://", "")
        }
    }
    FileDialog {
        id: m2ConfDialog
        title: "McConf XML File"
        nameFilters: ["XML files (*.xml)"]
        onAccepted: {
            m2ConfFileEdit.text = selectedFile.toString().replace("file://", "")
        }
    }
    FileDialog {
        id: qmlFileDialog
        title: "QML File"
        nameFilters: ["QML files (*.qml)"]
        onAccepted: {
            qmlFileEdit.text = selectedFile.toString().replace("file://", "")
        }
    }

    // ---- Helper functions ----
    function getM1Params() {
        return {
            gearing: m1GearingBox.realValue,
            maxRpm: m1MaxRpmBox.value,
            gearingEfficiency: m1GearEffBox.realValue / 100.0,
            fwCurrent: m1FwBox.realValue,
            motorNum: m1MotorNumBox.value,
            tempInc: m1TempIncBox.realValue,
            mtpa: m1MtpaBox.checked
        }
    }

    function getM2Params() {
        return {
            gearing: m2GearingBox.realValue,
            maxRpm: m2MaxRpmBox.value,
            gearingEfficiency: m2GearEffBox.realValue / 100.0,
            fwCurrent: m2FwBox.realValue,
            motorNum: m2MotorNumBox.value,
            tempInc: m2TempIncBox.realValue,
            mtpa: m2MtpaBox.checked
        }
    }

    function getSelectedItems(tableView, scaleArray) {
        var items = []
        for (var i = 0; i < dataItems.length; i++) {
            if (tableView.isSelected(i)) {
                var di = dataItems[i]
                if (di.plottable) {
                    items.push({index: i, name: di.name, scale: scaleArray[i]})
                }
            }
        }
        return items
    }

    function loadConfigs() {
        if (m1ConfLocalButton.checked) {
            compHelper.loadM1ConfigLocal()
        } else {
            if (!compHelper.loadM1ConfigFile(m1ConfFileEdit.text)) {
                VescIf.emitMessageDialog("Load M1 Config",
                    "Could not load motor 1 configuration. Make sure that the file path is valid.", false)
                return false
            }
        }
        if (m2ConfLocalButton.checked) {
            compHelper.loadM2ConfigLocal()
        } else {
            if (!compHelper.loadM2ConfigFile(m2ConfFileEdit.text)) {
                VescIf.emitMessageDialog("Load M2 Config",
                    "Could not load motor 2 configuration. Make sure that the file path is valid.", false)
                return false
            }
        }
        return true
    }

    function runTest() {
        if (!loadConfigs()) return

        var points = Math.round(pointsBox.value)
        var m1P = getM1Params()
        var m2P = getM2Params()
        var m1Sel = getSelectedItems(m1TableView, m1Scales)
        var m2Sel = getSelectedItems(m2TableView, m2Scales)

        var result
        switch (testMode) {
        case 0:
            result = compHelper.runTorqueSweep(testRpmBox.value, Math.abs(testTorqueBox.realValue),
                                                points, testNegativeBox.checked, m1P, m2P, m1Sel, m2Sel)
            break
        case 1:
            result = compHelper.runRpmSweep(testTorqueBox.realValue, testRpmBox.value,
                                             points, testNegativeBox.checked, m1P, m2P, m1Sel, m2Sel)
            break
        case 2:
            result = compHelper.runRpmPowerSweep(testPowerBox.realValue, testRpmStartBox.value,
                                                  testRpmBox.value, points, m1P, m2P, m1Sel, m2Sel)
            break
        case 3:
            result = compHelper.runExpSweep(testPowerBox.realValue, testRpmStartBox.value,
                                             testRpmBox.value, testExpBox.realValue, testExpBaseTorqueBox.realValue,
                                             points, m1P, m2P, m1Sel, m2Sel)
            break
        case 4:
            result = compHelper.runVbusSweep(Math.abs(testTorqueBox.realValue), testVbusBox.realValue,
                                              points, testNegativeBox.checked, m1P, m2P, m1Sel, m2Sel)
            break
        case 5:
            result = compHelper.runVbFwSweep(Math.abs(testTorqueBox.realValue), testRpmBox.value,
                                              testVbusBox.realValue, points, testNegativeBox.checked,
                                              m1P, m2P, m1Sel, m2Sel)
            break
        case 6:
            result = compHelper.runVbRpmSweep(testTorqueBox.realValue, testRpmBox.value,
                                               testVbusBox.realValue, points, testNegativeBox.checked,
                                               m1P, m2P, m1Sel, m2Sel)
            break
        }

        if (result) {
            lastResult = result
            updatePlot(result)
            runDone = true
        }
    }

    property int _legendRevision: 0

    function updatePlot(result) {
        // Clear all existing series
        while (graphsView.seriesList.length > 0) {
            graphsView.removeSeries(graphsView.seriesList[0])
        }

        var xData = result.xAxis
        var seriesList = result.series
        if (!xData || xData.length === 0) return

        var xMin = xData[0], xMax = xData[xData.length - 1]
        var yMin = Infinity, yMax = -Infinity

        for (var s = 0; s < seriesList.length; s++) {
            var d = seriesList[s].data
            for (var j = 0; j < d.length; j++) {
                if (d[j] < yMin) yMin = d[j]
                if (d[j] > yMax) yMax = d[j]
            }
        }

        if (yMin === Infinity) { yMin = 0; yMax = 100 }
        var yPad = (yMax - yMin) * 0.05
        if (yPad < 0.1) yPad = 1

        plotAxisX.min = xMin
        plotAxisX.max = xMax
        plotAxisX.titleText = result.xLabel || ""
        plotAxisY.min = yMin - yPad
        plotAxisY.max = yMax + yPad

        for (var i = 0; i < seriesList.length; i++) {
            var entry = seriesList[i]
            var color = plotColors[i % plotColors.length]

            var ls = lineSeriesComp.createObject(graphsView, {
                color: color,
                width: 2,
                name: entry.name
            })

            for (var k = 0; k < xData.length && k < entry.data.length; k++) {
                ls.append(xData[k], entry.data[k])
            }

            graphsView.addSeries(ls)
        }

        _legendRevision++
    }

    function updateTableAtX(x) {
        if (!loadConfigs()) return

        var qr = compHelper.queryDataAtX(x, testMode,
            testTorqueBox.realValue, testRpmBox.value, testPowerBox.realValue,
            testRpmStartBox.value, testExpBox.realValue, testExpBaseTorqueBox.realValue,
            testVbusBox.realValue, getM1Params(), getM2Params())

        if (qr) {
            var m1 = qr.m1
            var m2 = qr.m2
            for (var i = 0; i < dataItems.length; i++) {
                var di = dataItems[i]
                var v1 = m1[di.key] !== undefined ? m1[di.key] : 0
                var v2 = m2[di.key] !== undefined ? m2[di.key] : 0
                m1DataModel.set(i, { itemValue: v1.toFixed(di.decimals) + (di.unit ? " " + di.unit : "") })
                m2DataModel.set(i, { itemValue: v2.toFixed(di.decimals) + (di.unit ? " " + di.unit : "") })
            }
        }
    }

    function settingChanged() {
        if (liveUpdate && runDone) {
            runTest()
        }
    }

    // Component for dynamic LineSeries creation
    Component {
        id: lineSeriesComp
        LineSeries { }
    }

    // ---- Data models for tables ----
    ListModel { id: m1DataModel }
    ListModel { id: m2DataModel }

    // ---- LAYOUT: Vertical splitter top/bottom ----
    SplitView {
        anchors.fill: parent
        orientation: Qt.Vertical

        // ====== TOP: Horizontal splitter: settings (left) | plot (right) ======
        SplitView {
            SplitView.preferredHeight: parent.height * 0.6
            SplitView.minimumHeight: 200
            orientation: Qt.Horizontal

            // ---- Left: Tab settings ----
            Item {
                SplitView.preferredWidth: 260
                SplitView.minimumWidth: 200

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    TabBar {
                        id: settingsTabBar
                        Layout.fillWidth: true
                        TabButton { text: "Custom"; topPadding: 9; bottomPadding: 9 }
                        TabButton { text: "Custom"; topPadding: 9; bottomPadding: 9 }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: settingsTabBar.currentIndex

                        // ---- Tab 0: Custom test settings ----
                        Item {
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 4

                                // Test mode radio buttons (3x3 grid)
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 3
                                    columnSpacing: 4
                                    rowSpacing: 2

                                    RadioButton {
                                        text: "Torque"; checked: testMode === 0
                                        ToolTip.visible: hovered; ToolTip.text: "Sweep from 0 to the set torque at the set RPM."
                                        onToggled: if (checked) { testMode = 0; settingChanged() }
                                    }
                                    RadioButton {
                                        text: "RPM"; checked: testMode === 1
                                        ToolTip.visible: hovered; ToolTip.text: "Sweep from 0 to the end RPM at the set torque."
                                        onToggled: if (checked) { testMode = 1; settingChanged() }
                                    }
                                    RadioButton {
                                        text: "VBus"; checked: testMode === 4
                                        ToolTip.visible: hovered; ToolTip.text: "Torque sweep while keeping the bus voltage constant."
                                        onToggled: if (checked) { testMode = 4; settingChanged() }
                                    }
                                    RadioButton {
                                        text: "RPM/Power"; checked: testMode === 2
                                        ToolTip.visible: hovered; ToolTip.text: "Sweep from the start RPM to the end RPM at the set Power."
                                        onToggled: if (checked) { testMode = 2; settingChanged() }
                                    }
                                    RadioButton {
                                        text: "Exp"; checked: testMode === 3
                                        ToolTip.visible: hovered; ToolTip.text: "Torque + RPM Exponential mode."
                                        onToggled: if (checked) { testMode = 3; settingChanged() }
                                    }
                                    RadioButton {
                                        text: "VB+FW"; checked: testMode === 5
                                        ToolTip.visible: hovered; ToolTip.text: "Torque-sweep at fixed RPM and limited bus voltage. Uses field weakening."
                                        onToggled: if (checked) { testMode = 5; settingChanged() }
                                    }
                                    RadioButton {
                                        text: "VB+RPM"; checked: testMode === 6
                                        ToolTip.visible: hovered; ToolTip.text: "RPM sweep while limiting the torque by the bus voltage."
                                        onToggled: if (checked) { testMode = 6; settingChanged() }
                                    }
                                }

                                // Test parameter spin boxes
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    columnSpacing: 2
                                    rowSpacing: 2

                                    SpinBox {
                                        id: testPowerBox
                                        Layout.fillWidth: true
                                        from: 100; to: 999999000; stepSize: 100000
                                        value: 2000000; editable: true
                                        enabled: testMode === 2 || testMode === 3
                                        property real realValue: value / 1000.0
                                        textFromValue: function(v) { return "Pwr: " + (v / 1000.0).toFixed(1) + " W" }
                                        valueFromText: function(t) { return Math.round(parseFloat(t.replace("Pwr: ", "").replace(" W", "")) * 1000) }
                                        onValueModified: settingChanged()
                                    }
                                    SpinBox {
                                        id: testTorqueBox
                                        Layout.fillWidth: true
                                        from: -9999000; to: 9999000; stepSize: 500
                                        value: 3000; editable: true
                                        enabled: testMode === 0 || testMode === 1 || testMode === 4 || testMode === 5 || testMode === 6
                                        property real realValue: value / 1000.0
                                        textFromValue: function(v) { return (v / 1000.0).toFixed(3) + " Nm" }
                                        valueFromText: function(t) { return Math.round(parseFloat(t.replace(" Nm", "")) * 1000) }
                                        onValueModified: settingChanged()
                                    }

                                    SpinBox {
                                        id: testRpmStartBox
                                        Layout.fillWidth: true
                                        from: 0; to: 999999; stepSize: 100
                                        value: 500; editable: true
                                        enabled: testMode === 2 || testMode === 3
                                        ToolTip.visible: hovered; ToolTip.text: "Start RPM"
                                        textFromValue: function(v) { return v + " RPM" }
                                        valueFromText: function(t) { return parseInt(t.replace(" RPM", "")) || 0 }
                                        onValueModified: settingChanged()
                                    }
                                    SpinBox {
                                        id: testRpmBox
                                        Layout.fillWidth: true
                                        from: 0; to: 999999; stepSize: 100
                                        value: 8000; editable: true
                                        enabled: testMode !== 4
                                        ToolTip.visible: hovered; ToolTip.text: "End RPM"
                                        textFromValue: function(v) { return v + " RPM" }
                                        valueFromText: function(t) { return parseInt(t.replace(" RPM", "")) || 0 }
                                        onValueModified: settingChanged()
                                    }

                                    SpinBox {
                                        id: testExpBaseTorqueBox
                                        Layout.fillWidth: true
                                        from: 0; to: 999000; stepSize: 200
                                        value: 0; editable: true
                                        enabled: testMode === 3
                                        property real realValue: value / 1000.0
                                        textFromValue: function(v) { return "B: " + (v / 1000.0).toFixed(3) + " Nm" }
                                        valueFromText: function(t) { return Math.round(parseFloat(t.replace("B: ", "").replace(" Nm", "")) * 1000) }
                                        onValueModified: settingChanged()
                                    }
                                    SpinBox {
                                        id: testExpBox
                                        Layout.fillWidth: true
                                        from: 0; to: 4000; stepSize: 100
                                        value: 2700; editable: true
                                        enabled: testMode === 3
                                        property real realValue: value / 1000.0
                                        ToolTip.visible: hovered; ToolTip.text: "RPM exponent"
                                        textFromValue: function(v) { return "Exp: " + (v / 1000.0).toFixed(1) }
                                        valueFromText: function(t) { return Math.round(parseFloat(t.replace("Exp: ", "")) * 1000) }
                                        onValueModified: settingChanged()
                                    }

                                    CheckBox {
                                        id: testNegativeBox
                                        text: "Negative"
                                        ToolTip.visible: hovered; ToolTip.text: "Include negative RPM and Torque in tests."
                                        onToggled: settingChanged()
                                    }
                                    SpinBox {
                                        id: testVbusBox
                                        Layout.fillWidth: true
                                        from: 0; to: 9999000; stepSize: 1000
                                        value: 48000; editable: true
                                        enabled: testMode === 4 || testMode === 5 || testMode === 6
                                        property real realValue: value / 1000.0
                                        textFromValue: function(v) { return (v / 1000.0).toFixed(1) + " V" }
                                        valueFromText: function(t) { return Math.round(parseFloat(t.replace(" V", "")) * 1000) }
                                        onValueModified: settingChanged()
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Button {
                                        text: "Run"
                                        icon.source: "qrc" + Utility.getThemePath() + "icons/Process-96.png"
                                        onClicked: runTest()
                                    }
                                    CheckBox {
                                        text: "Live Update"; checked: true
                                        ToolTip.visible: hovered; ToolTip.text: "Re-run tests any time settings are changed."
                                        onToggled: { liveUpdate = checked; settingChanged() }
                                    }
                                }

                                Item { Layout.fillHeight: true }
                            }
                        }

                        // ---- Tab 1: QML custom test ----
                        Item {
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    TextField {
                                        id: qmlFileEdit
                                        Layout.fillWidth: true
                                    }
                                    Button {
                                        icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"
                                        onClicked: qmlFileDialog.open()
                                    }
                                    Button {
                                        icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                                        ToolTip.visible: hovered; ToolTip.text: "Run QML test"
                                    }
                                    Button {
                                        icon.source: "qrc" + Utility.getThemePath() + "icons/Stop Sign-96.png"
                                        ToolTip.visible: hovered; ToolTip.text: "Stop QML test"
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    color: Utility.getAppHexColor("normalBackground")
                                    border.color: Utility.getAppHexColor("disabledText")
                                    border.width: 1; radius: 2
                                    Label {
                                        anchors.centerIn: parent
                                        text: "QML Test Widget Area"
                                        color: Utility.getAppHexColor("disabledText")
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ---- Right: Plot area ----
            Item {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 300

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2

                    // Title row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        TextField {
                            id: titleEdit
                            Layout.fillWidth: true
                            text: "Graph Title"
                            onTextChanged: settingChanged()
                        }
                        TextField {
                            id: compAEdit
                            Layout.preferredWidth: 100
                            text: "Motor A"
                            onTextChanged: settingChanged()
                        }
                        TextField {
                            id: compBEdit
                            Layout.preferredWidth: 100
                            text: "Motor B"
                            onTextChanged: settingChanged()
                        }
                        SpinBox {
                            id: pointsBox
                            from: 5; to: 9999; stepSize: 1
                            value: 1000; editable: true
                            textFromValue: function(v) { return "Plot Points: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("Plot Points: ", "")) || 1000 }
                            onValueModified: settingChanged()
                        }
                        CheckBox {
                            id: scatterPlotBox
                            text: "ScatterPlot"
                            onToggled: settingChanged()
                        }
                    }

                    // Export row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Item { Layout.fillWidth: true }
                        Label { text: "Export" }
                        SpinBox {
                            id: saveWidthBox
                            from: 100; to: 9999; value: 1280; editable: true
                            textFromValue: function(v) { return "W: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("W: ", "")) || 1280 }
                        }
                        SpinBox {
                            id: saveHeightBox
                            from: 100; to: 9999; value: 720; editable: true
                            textFromValue: function(v) { return "H: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("H: ", "")) || 720 }
                        }
                        Button {
                            text: "PDF"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Line Chart-96.png"
                            ToolTip.visible: hovered; ToolTip.text: "Save plot as PDF image"
                        }
                        Button {
                            text: "PNG"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Line Chart-96.png"
                            ToolTip.visible: hovered; ToolTip.text: "Save plot as PNG image"
                        }
                    }

                    // Plot + button strip
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 2

                            GraphsView {
                                id: graphsView
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                theme: GraphsTheme {
                                    colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                                    plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                                    grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                                }

                                axisX: ValueAxis {
                                    id: plotAxisX
                                    min: 0; max: 100
                                    titleText: "Torque (Nm)"
                                }
                                axisY: ValueAxis {
                                    id: plotAxisY
                                    min: 0; max: 100
                                }
                            }
                            PlotLegend { graphsView: graphsView; revision: _legendRevision }
                        }

                        // Vertical button strip (right of plot)
                        ColumnLayout {
                            Layout.alignment: Qt.AlignTop
                            spacing: 3

                            ToolButton {
                                checkable: true; checked: true
                                icon.source: checked
                                    ? ("qrc" + Utility.getThemePath() + "icons/size_on.png")
                                    : ("qrc" + Utility.getThemePath() + "icons/size_off.png")
                                icon.width: 20; icon.height: 20
                                ToolTip.visible: hovered; ToolTip.text: "Autoscale plots after updates."
                            }
                            ToolButton {
                                checkable: true; checked: true
                                icon.source: checked
                                    ? ("qrc" + Utility.getThemePath() + "icons/expand_on.png")
                                    : ("qrc" + Utility.getThemePath() + "icons/expand_off.png")
                                icon.width: 20; icon.height: 20
                                ToolTip.visible: hovered; ToolTip.text: "Enable horizontal zoom"
                            }
                            ToolButton {
                                checkable: true; checked: true
                                icon.source: checked
                                    ? ("qrc" + Utility.getThemePath() + "icons/expand_v_on.png")
                                    : ("qrc" + Utility.getThemePath() + "icons/expand_v_off.png")
                                icon.width: 20; icon.height: 20
                                ToolTip.visible: hovered; ToolTip.text: "Enable vertical zoom"
                            }
                            ToolButton {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/size_off.png"
                                icon.width: 20; icon.height: 20
                                ToolTip.visible: hovered; ToolTip.text: "Rescale plots to fit"
                                onClicked: runTest()
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }
        }

        // ====== BOTTOM: Motor A + Motor B side by side ======
        SplitView {
            SplitView.preferredHeight: parent.height * 0.4
            SplitView.minimumHeight: 150
            orientation: Qt.Horizontal

            // ---- Motor A ----
            GroupBox {
                title: "Motor A"
                SplitView.fillWidth: true
                SplitView.minimumWidth: 200

                RowLayout {
                    anchors.fill: parent
                    spacing: 4

                    ColumnLayout {
                        Layout.preferredWidth: 200
                        Layout.fillHeight: true
                        spacing: 2

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 4
                            columnSpacing: 2; rowSpacing: 2

                            SpinBox {
                                id: m1GearingBox
                                Layout.columnSpan: 2; Layout.fillWidth: true
                                from: 10; to: 999000; stepSize: 50
                                value: 1000; editable: true
                                property real realValue: value / 1000.0
                                ToolTip.visible: hovered; ToolTip.text: "Gear Ratio"
                                textFromValue: function(v) { return "Gearing: " + (v / 1000.0).toFixed(3) }
                                valueFromText: function(t) { return Math.round(parseFloat(t.replace("Gearing: ", "")) * 1000) }
                                onValueModified: settingChanged()
                            }
                            CheckBox { id: m1MtpaBox; text: "MTPA"; onToggled: settingChanged() }
                            Button {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/motor_up.png"
                                ToolTip.visible: hovered; ToolTip.text: "Get gearing from config"
                                onClicked: { if (loadConfigs()) m1GearingBox.value = Math.round(VescIf.mcConfig().getParamDouble("si_gear_ratio") * 1000) }
                            }

                            SpinBox {
                                id: m1GearEffBox; Layout.fillWidth: true
                                from: 100; to: 100000; stepSize: 200; value: 100000; editable: true
                                property real realValue: value / 1000.0
                                textFromValue: function(v) { return "Gear Eff: " + (v / 1000.0).toFixed(1) + " %" }
                                valueFromText: function(t) { return Math.round(parseFloat(t.replace("Gear Eff: ", "").replace(" %", "")) * 1000) }
                                onValueModified: settingChanged()
                            }
                            SpinBox {
                                id: m1MaxRpmBox; Layout.columnSpan: 3; Layout.fillWidth: true
                                from: 0; to: 500000; stepSize: 1000; value: 50000; editable: true
                                ToolTip.visible: hovered; ToolTip.text: "Maximum motor shaft RPM"
                                textFromValue: function(v) { return "Max RPM: " + v }
                                valueFromText: function(t) { return parseInt(t.replace("Max RPM: ", "")) || 50000 }
                                onValueModified: settingChanged()
                            }

                            SpinBox {
                                id: m1MotorNumBox; Layout.fillWidth: true
                                from: 1; to: 99; value: 1; editable: true
                                ToolTip.visible: hovered; ToolTip.text: "Number of motors in parallel"
                                textFromValue: function(v) { return "Motors: " + v }
                                valueFromText: function(t) { return parseInt(t.replace("Motors: ", "")) || 1 }
                                onValueModified: settingChanged()
                            }
                            SpinBox {
                                id: m1FwBox; Layout.columnSpan: 3; Layout.fillWidth: true
                                from: 0; to: 999000; stepSize: 1000; value: 0; editable: true
                                property real realValue: value / 1000.0
                                ToolTip.visible: hovered; ToolTip.text: "Field Weakening Current"
                                textFromValue: function(v) { return "FW: " + (v / 1000.0).toFixed(1) + " A" }
                                valueFromText: function(t) { return Math.round(parseFloat(t.replace("FW: ", "").replace(" A", "")) * 1000) }
                                onValueModified: settingChanged()
                            }

                            SpinBox {
                                id: m1TempIncBox; Layout.columnSpan: 4; Layout.fillWidth: true
                                from: -200000; to: 200000; stepSize: 1000; value: 0; editable: true
                                property real realValue: value / 1000.0
                                textFromValue: function(v) { return "Temp Increase: " + (v / 1000.0).toFixed(1) + " °C" }
                                valueFromText: function(t) { return Math.round(parseFloat(t.replace("Temp Increase: ", "").replace(" °C", "")) * 1000) }
                                onValueModified: settingChanged()
                            }
                        }

                        GroupBox {
                            title: "Motor Configuration"
                            Layout.fillWidth: true
                            GridLayout {
                                anchors.fill: parent
                                columns: 4; columnSpacing: 2; rowSpacing: 2

                                RadioButton { id: m1ConfLocalButton; text: "Local"; checked: true
                                    ToolTip.visible: hovered; ToolTip.text: "Use mcconf from connected VESC"
                                    onToggled: settingChanged()
                                }
                                Item { Layout.columnSpan: 3 }

                                RadioButton { id: m1ConfFileButton; text: "File"; onToggled: settingChanged() }
                                TextField { id: m1ConfFileEdit; Layout.fillWidth: true; onTextChanged: settingChanged() }
                                Button { icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"; onClicked: m1ConfDialog.open() }
                                Button {
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/motor_up.png"
                                    ToolTip.visible: hovered; ToolTip.text: "Load selected motor configuration in VESC Tool"
                                    onClicked: {
                                        if (VescIf.mcConfig().loadXml(m1ConfFileEdit.text, "MCConfiguration"))
                                            VescIf.emitStatusMessage("Loaded motor configuration", true)
                                        else VescIf.emitMessageDialog("Load motor configuration", "Could not load motor configuration.", false, false)
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }

                    // Data table
                    ListView {
                        id: m1TableView
                        Layout.fillWidth: true; Layout.fillHeight: true
                        clip: true; model: m1DataModel
                        headerPositioning: ListView.OverlayHeader

                        property var selectedIndices: ({})
                        function isSelected(idx) { return selectedIndices[idx] === true }
                        function toggleSelection(idx) {
                            var s = selectedIndices; s[idx] = !s[idx]; selectedIndices = s
                            settingChanged()
                        }

                        header: Rectangle {
                            z: 2; width: m1TableView.width; height: 28
                            color: Utility.getAppHexColor("normalBackground")
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4; spacing: 4
                                Label { text: "Name"; Layout.preferredWidth: 100; font.bold: true; font.pointSize: 11 }
                                Label { text: "Value"; Layout.fillWidth: true; font.bold: true; font.pointSize: 11 }
                            }
                        }

                        delegate: Rectangle {
                            width: m1TableView.width; height: 26
                            color: m1TableView.isSelected(index) ? Utility.getAppHexColor("lightAccent")
                                : (index % 2 === 0 ? "transparent" : Qt.rgba(0.5, 0.5, 0.5, 0.1))
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4; spacing: 4
                                Label { text: model.itemName; Layout.preferredWidth: 100; font.pointSize: 11; elide: Text.ElideRight; color: Utility.getAppHexColor("lightText") }
                                Label { text: model.itemValue; Layout.fillWidth: true; font.pointSize: 11; color: Utility.getAppHexColor("lightText") }
                            }
                            MouseArea { anchors.fill: parent; onClicked: m1TableView.toggleSelection(index) }
                        }
                    }
                }
            }

            // ---- Motor B ----
            GroupBox {
                title: "Motor B"
                SplitView.fillWidth: true
                SplitView.minimumWidth: 200

                RowLayout {
                    anchors.fill: parent
                    spacing: 4

                    ColumnLayout {
                        Layout.preferredWidth: 200
                        Layout.fillHeight: true
                        spacing: 2

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 4; columnSpacing: 2; rowSpacing: 2

                            SpinBox {
                                id: m2GearingBox
                                Layout.columnSpan: 2; Layout.fillWidth: true
                                from: 10; to: 999000; stepSize: 50; value: 1000; editable: true
                                property real realValue: value / 1000.0
                                ToolTip.visible: hovered; ToolTip.text: "Gear Ratio"
                                textFromValue: function(v) { return "Gearing: " + (v / 1000.0).toFixed(3) }
                                valueFromText: function(t) { return Math.round(parseFloat(t.replace("Gearing: ", "")) * 1000) }
                                onValueModified: settingChanged()
                            }
                            CheckBox { id: m2MtpaBox; text: "MTPA"; onToggled: settingChanged() }
                            Button {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/motor_up.png"
                                ToolTip.visible: hovered; ToolTip.text: "Get gearing from config"
                                onClicked: { if (loadConfigs()) m2GearingBox.value = Math.round(VescIf.mcConfig().getParamDouble("si_gear_ratio") * 1000) }
                            }

                            SpinBox {
                                id: m2GearEffBox; Layout.fillWidth: true
                                from: 100; to: 100000; stepSize: 200; value: 100000; editable: true
                                property real realValue: value / 1000.0
                                textFromValue: function(v) { return "Gear Eff: " + (v / 1000.0).toFixed(1) + " %" }
                                valueFromText: function(t) { return Math.round(parseFloat(t.replace("Gear Eff: ", "").replace(" %", "")) * 1000) }
                                onValueModified: settingChanged()
                            }
                            SpinBox {
                                id: m2MaxRpmBox; Layout.columnSpan: 3; Layout.fillWidth: true
                                from: 0; to: 500000; stepSize: 1000; value: 50000; editable: true
                                ToolTip.visible: hovered; ToolTip.text: "Maximum motor shaft RPM"
                                textFromValue: function(v) { return "Max RPM: " + v }
                                valueFromText: function(t) { return parseInt(t.replace("Max RPM: ", "")) || 50000 }
                                onValueModified: settingChanged()
                            }

                            SpinBox {
                                id: m2MotorNumBox; Layout.fillWidth: true
                                from: 1; to: 99; value: 1; editable: true
                                ToolTip.visible: hovered; ToolTip.text: "Number of motors in parallel"
                                textFromValue: function(v) { return "Motors: " + v }
                                valueFromText: function(t) { return parseInt(t.replace("Motors: ", "")) || 1 }
                                onValueModified: settingChanged()
                            }
                            SpinBox {
                                id: m2FwBox; Layout.columnSpan: 3; Layout.fillWidth: true
                                from: 0; to: 999000; stepSize: 1000; value: 0; editable: true
                                property real realValue: value / 1000.0
                                ToolTip.visible: hovered; ToolTip.text: "Field Weakening Current"
                                textFromValue: function(v) { return "FW: " + (v / 1000.0).toFixed(1) + " A" }
                                valueFromText: function(t) { return Math.round(parseFloat(t.replace("FW: ", "").replace(" A", "")) * 1000) }
                                onValueModified: settingChanged()
                            }

                            SpinBox {
                                id: m2TempIncBox; Layout.columnSpan: 4; Layout.fillWidth: true
                                from: -200000; to: 200000; stepSize: 1000; value: 0; editable: true
                                property real realValue: value / 1000.0
                                textFromValue: function(v) { return "Temp Increase: " + (v / 1000.0).toFixed(1) + " °C" }
                                valueFromText: function(t) { return Math.round(parseFloat(t.replace("Temp Increase: ", "").replace(" °C", "")) * 1000) }
                                onValueModified: settingChanged()
                            }
                        }

                        GroupBox {
                            title: "Motor Configuration"
                            Layout.fillWidth: true
                            GridLayout {
                                anchors.fill: parent
                                columns: 4; columnSpacing: 2; rowSpacing: 2

                                RadioButton { id: m2ConfLocalButton; text: "Local"; checked: true
                                    ToolTip.visible: hovered; ToolTip.text: "Use mcconf from connected VESC"
                                    onToggled: settingChanged()
                                }
                                Item { Layout.columnSpan: 3 }

                                RadioButton { id: m2ConfFileButton; text: "File"; onToggled: settingChanged() }
                                TextField { id: m2ConfFileEdit; Layout.fillWidth: true; onTextChanged: settingChanged() }
                                Button { icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"; onClicked: m2ConfDialog.open() }
                                Button {
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/motor_up.png"
                                    ToolTip.visible: hovered; ToolTip.text: "Load selected motor configuration in VESC Tool"
                                    onClicked: {
                                        if (VescIf.mcConfig().loadXml(m2ConfFileEdit.text, "MCConfiguration"))
                                            VescIf.emitStatusMessage("Loaded motor configuration", true)
                                        else VescIf.emitMessageDialog("Load motor configuration", "Could not load motor configuration.", false, false)
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }

                    ListView {
                        id: m2TableView
                        Layout.fillWidth: true; Layout.fillHeight: true
                        clip: true; model: m2DataModel
                        headerPositioning: ListView.OverlayHeader

                        property var selectedIndices: ({})
                        function isSelected(idx) { return selectedIndices[idx] === true }
                        function toggleSelection(idx) {
                            var s = selectedIndices; s[idx] = !s[idx]; selectedIndices = s
                            settingChanged()
                        }

                        header: Rectangle {
                            z: 2; width: m2TableView.width; height: 28
                            color: Utility.getAppHexColor("normalBackground")
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4; spacing: 4
                                Label { text: "Name"; Layout.preferredWidth: 100; font.bold: true; font.pointSize: 11 }
                                Label { text: "Value"; Layout.fillWidth: true; font.bold: true; font.pointSize: 11 }
                            }
                        }

                        delegate: Rectangle {
                            width: m2TableView.width; height: 26
                            color: m2TableView.isSelected(index) ? Utility.getAppHexColor("lightAccent")
                                : (index % 2 === 0 ? "transparent" : Qt.rgba(0.5, 0.5, 0.5, 0.1))
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4; spacing: 4
                                Label { text: model.itemName; Layout.preferredWidth: 100; font.pointSize: 11; elide: Text.ElideRight; color: Utility.getAppHexColor("lightText") }
                                Label { text: model.itemValue; Layout.fillWidth: true; font.pointSize: 11; color: Utility.getAppHexColor("lightText") }
                            }
                            MouseArea { anchors.fill: parent; onClicked: m2TableView.toggleSelection(index) }
                        }
                    }
                }
            }
        }
    }
}
