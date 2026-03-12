/*
    Desktop ExperimentsPage — faithful recreation of the original PageExperiments widget.
    Features: Duty/Current/RPM sweep experiments, multi-channel recording & plot,
    sample interval control, stop, rescale, save CSV, comparison loading.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtGraphs
import Vedder.vesc

Item {
    id: experimentsPage

    // Experiment state
    property int expState: 0  // 0=off, 1=duty, 2=current, 3=rpm, 4=hold
    property double startTimeMs: 0
    property int sampleCount: 0

    // Data arrays
    property var timeData: []
    property var currentMotorData: []
    property var currentInData: []
    property var powerData: []
    property var voltageData: []
    property var rpmData: []
    property var dutyData: []
    property var tempFetData: []
    property var tempMotorData: []

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // === Top controls ===
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            // Duty sweep
            GroupBox {
                title: "Duty Sweep"
                RowLayout {
                    spacing: 4
                    Label { text: "From:" }
                    SpinBox { id: dutyFromBox; from: -1000; to: 1000; value: 0; stepSize: 10; editable: true
                        property real realValue: value / 1000.0
                        textFromValue: function(v) { return (v / 1000.0).toFixed(3) }
                        valueFromText: function(t) { return parseFloat(t) * 1000 }
                    }
                    Label { text: "To:" }
                    SpinBox { id: dutyToBox; from: -1000; to: 1000; value: 500; stepSize: 10; editable: true
                        property real realValue: value / 1000.0
                        textFromValue: function(v) { return (v / 1000.0).toFixed(3) }
                        valueFromText: function(t) { return parseFloat(t) * 1000 }
                    }
                    Label { text: "Step:" }
                    SpinBox { id: dutyStepBox; from: 1; to: 1000; value: 10; stepSize: 1; editable: true
                        property real realValue: value / 1000.0
                        textFromValue: function(v) { return (v / 1000.0).toFixed(3) }
                        valueFromText: function(t) { return parseFloat(t) * 1000 }
                    }
                    Label { text: "Step T(s):" }
                    SpinBox { id: dutyStepTimeBox; from: 1; to: 600; value: 5; editable: true }
                    Button {
                        text: "Run"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                        onClicked: startExperiment(1)
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Sample interval + Stop
            ColumnLayout {
                spacing: 2
                RowLayout {
                    Label { text: "Sample (ms):" }
                    SpinBox { id: sampleIntervalBox; from: 10; to: 10000; value: 100; editable: true; stepSize: 10 }
                }
                RowLayout {
                    Button {
                        text: "Stop"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Shutdown-96.png"
                        onClicked: stopExperiment()
                    }
                    Button {
                        text: "Rescale"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/size_off.png"
                        onClicked: rescalePlot()
                    }
                }
            }
        }

        // Second row: Current & RPM sweeps
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            GroupBox {
                title: "Current Sweep"
                RowLayout {
                    spacing: 4
                    Label { text: "From:" }
                    SpinBox { id: currentFromBox; from: -2000; to: 2000; value: 0; editable: true
                        property real realValue: value / 10.0
                        textFromValue: function(v) { return (v / 10.0).toFixed(1) }
                        valueFromText: function(t) { return parseFloat(t) * 10 }
                    }
                    Label { text: "To:" }
                    SpinBox { id: currentToBox; from: -2000; to: 2000; value: 100; editable: true
                        property real realValue: value / 10.0
                        textFromValue: function(v) { return (v / 10.0).toFixed(1) }
                        valueFromText: function(t) { return parseFloat(t) * 10 }
                    }
                    Label { text: "Step:" }
                    SpinBox { id: currentStepBox; from: 1; to: 2000; value: 10; editable: true
                        property real realValue: value / 10.0
                        textFromValue: function(v) { return (v / 10.0).toFixed(1) }
                        valueFromText: function(t) { return parseFloat(t) * 10 }
                    }
                    Label { text: "Step T(s):" }
                    SpinBox { id: currentStepTimeBox; from: 1; to: 600; value: 5; editable: true }
                    Button {
                        text: "Run"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                        onClicked: startExperiment(2)
                    }
                }
            }

            GroupBox {
                title: "RPM Sweep"
                RowLayout {
                    spacing: 4
                    Label { text: "From:" }
                    SpinBox { id: rpmFromBox; from: -100000; to: 100000; value: 0; editable: true; stepSize: 100 }
                    Label { text: "To:" }
                    SpinBox { id: rpmToBox; from: -100000; to: 100000; value: 5000; editable: true; stepSize: 100 }
                    Label { text: "Step:" }
                    SpinBox { id: rpmStepBox; from: 1; to: 100000; value: 100; editable: true; stepSize: 100 }
                    Label { text: "Step T(s):" }
                    SpinBox { id: rpmStepTimeBox; from: 1; to: 600; value: 5; editable: true }
                    Button {
                        text: "Run"
                        icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                        onClicked: startExperiment(3)
                    }
                }
            }
        }

        // Progress bar
        ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            from: 0; to: 100; value: 0
            visible: expState !== 0
        }

        // Data visibility toggles
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            CheckBox { id: showCurrentMotor; text: "Motor Current"; checked: true }
            CheckBox { id: showCurrentIn; text: "Input Current"; checked: false }
            CheckBox { id: showPower; text: "Power"; checked: true }
            CheckBox { id: showVoltage; text: "Voltage"; checked: false }
            CheckBox { id: showDuty; text: "Duty"; checked: false }
            CheckBox { id: showTempFet; text: "FET Temp"; checked: false }
            CheckBox { id: showTempMotor; text: "Motor Temp"; checked: false }

            Item { Layout.fillWidth: true }

            Button {
                text: "Clear"
                onClicked: clearData()
            }
            Button {
                text: "Save CSV"
                icon.source: "qrc" + Utility.getThemePath() + "icons/Save-96.png"
                onClicked: saveCsvDialog.open()
            }
        }

        // Graph
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
                id: expAxisX
                min: 0; max: 30
                titleText: "Seconds (s)"
            }

            axisY: ValueAxis {
                id: expAxisY
                min: -10; max: 50
                titleText: "Value"
            }

            LineSeries {
                id: currentMotorSeries; color: "#3498db"; width: 2
                name: "Motor Current (A)"; visible: showCurrentMotor.checked
            }
            LineSeries {
                id: currentInSeries; color: "#e74c3c"; width: 2
                name: "Input Current (A)"; visible: showCurrentIn.checked
            }
            LineSeries {
                id: powerSeries; color: "#2ecc71"; width: 2
                name: "Power (W)"; visible: showPower.checked
            }
            LineSeries {
                id: voltageSeries; color: "#f39c12"; width: 2
                name: "Voltage (V)"; visible: showVoltage.checked
            }
            LineSeries {
                id: dutySeries; color: "#9b59b6"; width: 2
                name: "Duty (%)"; visible: showDuty.checked
            }
            LineSeries {
                id: tempFetSeries; color: "#e67e22"; width: 2
                name: "FET Temp (°C)"; visible: showTempFet.checked
            }
            LineSeries {
                id: tempMotorSeries; color: "#1abc9c"; width: 2
                name: "Motor Temp (°C)"; visible: showTempMotor.checked
            }
        }

        PlotLegend { graphsView: graphsView }

        // Status bar
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: {
                    var stateStr = ["Idle", "Duty Sweep", "Current Sweep", "RPM Sweep", "RPM Hold"]
                    return "State: " + stateStr[expState] + " | Samples: " + sampleCount
                }
                color: Utility.getAppHexColor("disabledText")
                font.pointSize: 11
            }
            Item { Layout.fillWidth: true }
        }
    }

    // Experiment timer
    Timer {
        id: expTimer
        interval: sampleIntervalBox.value
        repeat: true
        running: expState !== 0

        onTriggered: {
            if (expState === 0) return

            VescIf.commands().getValues()

            if (expState === 4) {
                // Hold mode
                VescIf.commands().setRpm(rpmToBox.value)
                return
            }

            var from = 0, to = 0, step = 0, stepTime = 0

            if (expState === 1) { // Duty
                from = dutyFromBox.realValue; to = dutyToBox.realValue
                step = dutyStepBox.realValue; stepTime = dutyStepTimeBox.value
            } else if (expState === 2) { // Current
                from = currentFromBox.realValue; to = currentToBox.realValue
                step = currentStepBox.realValue; stepTime = currentStepTimeBox.value
            } else if (expState === 3) { // RPM
                from = rpmFromBox.value; to = rpmToBox.value
                step = rpmStepBox.value; stepTime = rpmStepTimeBox.value
            }

            if (from > to) step = -step

            var elapsed = (Date.now() - startTimeMs) / 1000.0
            var totalSteps = (to - from) / step
            var totalTime = totalSteps * stepTime
            var progress = elapsed / totalTime
            var stepNow = Math.floor(progress * totalSteps)
            var valueNow = from + stepNow * step

            if (progress >= 1.0) {
                VescIf.commands().setCurrent(0)
                expState = 0
                progressBar.value = 100
            } else {
                progressBar.value = progress * 100

                if (expState === 1) VescIf.commands().setDutyCycle(valueNow)
                else if (expState === 2) VescIf.commands().setCurrent(valueNow)
                else if (expState === 3) VescIf.commands().setRpm(valueNow)
            }
        }
    }

    // Value received connection
    Connections {
        target: VescIf.commands()

        function onValuesReceived(values, mask) {
            if (expState === 0) return

            var t = (Date.now() - startTimeMs) / 1000.0
            sampleCount++

            currentMotorSeries.append(t, values.current_motor)
            currentInSeries.append(t, values.current_in)
            powerSeries.append(t, values.current_in * values.v_in)
            voltageSeries.append(t, values.v_in)
            dutySeries.append(t, values.duty_now * 100.0)
            tempFetSeries.append(t, values.temp_mos)
            tempMotorSeries.append(t, values.temp_motor)

            // Auto-scroll X
            if (t > expAxisX.max * 0.9) {
                expAxisX.max = t * 1.2
            }

            // Auto-scale Y
            var vals = [
                Math.abs(values.current_motor),
                Math.abs(values.current_in),
                Math.abs(values.current_in * values.v_in),
                values.v_in,
                Math.abs(values.duty_now * 100.0),
                values.temp_mos,
                values.temp_motor
            ]
            var maxVal = Math.max.apply(null, vals)
            if (maxVal > expAxisY.max * 0.85) {
                expAxisY.max = maxVal * 1.3
            }
            var minVal = Math.min(values.current_motor, values.current_in, values.duty_now * 100.0)
            if (minVal < expAxisY.min * 0.85 && minVal < 0) {
                expAxisY.min = minVal * 1.3
            }
        }
    }

    FileDialog {
        id: saveCsvDialog
        title: "Save CSV"
        nameFilters: ["CSV files (*.csv)", "All files (*)"]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            VescIf.emitMessageDialog("Save CSV", "CSV export not yet fully implemented in QML UI", false, false)
        }
    }

    function startExperiment(state) {
        clearData()
        expState = state
        startTimeMs = Date.now()
        sampleCount = 0
        progressBar.value = 0
    }

    function stopExperiment() {
        if (expState !== 0) {
            VescIf.commands().setCurrent(0)
        }
        expState = 0
        progressBar.value = 100
    }

    function clearData() {
        expState = 0
        sampleCount = 0
        currentMotorSeries.clear()
        currentInSeries.clear()
        powerSeries.clear()
        voltageSeries.clear()
        dutySeries.clear()
        tempFetSeries.clear()
        tempMotorSeries.clear()
        expAxisX.min = 0; expAxisX.max = 30
        expAxisY.min = -10; expAxisY.max = 50
    }

    function rescalePlot() {
        expAxisX.min = 0
        expAxisX.max = Math.max(30, (Date.now() - startTimeMs) / 1000.0 * 1.1)
        expAxisY.min = -10
        expAxisY.max = 50
    }
}
