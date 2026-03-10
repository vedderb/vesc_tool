/*
    Desktop RtDataPage — fully native implementation matching the original widget page.
    Tabs: Charts, Values, Setup, IMU, Statistics
    All charts use QtGraphs. No mobile component dependencies.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs
import QtQuick3D
import Vedder.vesc

Item {
    id: rtDataPage

    property int maxSamples: 500
    property int sampleIndex: 0
    property Commands mCommands: VescIf.commands()
    property int autoRangeInterval: 20  // recalc Y range every N samples
    property int autoRangeCounter: 0

    // Scan all visible points in the given series list and return { yMin, yMax }
    function calcSeriesRange(seriesList) {
        var yMin = Number.MAX_VALUE
        var yMax = -Number.MAX_VALUE
        var xMin = seriesList[0] ? (sampleIndex > maxSamples ? sampleIndex - maxSamples : 0) : 0
        for (var s = 0; s < seriesList.length; s++) {
            var series = seriesList[s]
            for (var i = 0; i < series.count; i++) {
                var pt = series.at(i)
                if (pt.x >= xMin) {
                    if (pt.y < yMin) yMin = pt.y
                    if (pt.y > yMax) yMax = pt.y
                }
            }
        }
        if (yMin === Number.MAX_VALUE) { yMin = -1; yMax = 1 }
        return { yMin: yMin, yMax: yMax }
    }

    // Apply autorange to a ValueAxis given the range from calcSeriesRange
    function autoRangeAxis(axis, range, symmetric, minSpan) {
        var lo = range.yMin
        var hi = range.yMax
        var span = hi - lo
        if (span < minSpan) {
            var mid = (hi + lo) / 2
            lo = mid - minSpan / 2
            hi = mid + minSpan / 2
            span = minSpan
        }
        var margin = span * 0.15
        if (symmetric) {
            var absMax = Math.max(Math.abs(lo), Math.abs(hi)) + margin
            if (absMax < minSpan / 2) absMax = minSpan / 2
            axis.min = -absMax
            axis.max = absMax
        } else {
            axis.min = lo - margin
            axis.max = hi + margin
        }
    }

    TabBar {
        id: rtTabBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        TabButton { text: "Charts"; topPadding: 9; bottomPadding: 9 }
        TabButton { text: "Values"; topPadding: 9; bottomPadding: 9 }
        TabButton { text: "IMU"; topPadding: 9; bottomPadding: 9 }
        TabButton { text: "Statistics"; topPadding: 9; bottomPadding: 9 }
    }

    StackLayout {
        anchors.top: rtTabBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 4
        currentIndex: rtTabBar.currentIndex

        // ---- Tab 0: Charts ----
        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: 4

                // Current chart
                GraphsView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 100

                    theme: GraphsTheme {
                        colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                        plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                        grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                    }

                    axisX: ValueAxis { id: currentAxisX; min: 0; max: maxSamples; titleText: "Samples" }
                    axisY: ValueAxis { id: currentAxisY; min: -100; max: 100; titleText: "Current (A) / Duty" }

                    LineSeries { id: motorCurrentSeries; color: Utility.getAppHexColor("blue"); width: 2; name: "Motor I" }
                    LineSeries { id: batteryCurrentSeries; color: Utility.getAppHexColor("green"); width: 2; name: "Battery I" }
                    LineSeries { id: dutySeries; color: Utility.getAppHexColor("orange"); width: 1; name: "Duty×100" }
                }

                // RPM + Speed chart
                GraphsView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 100

                    theme: GraphsTheme {
                        colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                        plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                        grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                    }

                    axisX: ValueAxis { id: rpmAxisX; min: 0; max: maxSamples; titleText: "Samples" }
                    axisY: ValueAxis { id: rpmAxisY; min: -10000; max: 10000; titleText: "RPM" }

                    LineSeries { id: rpmSeries; color: Utility.getAppHexColor("cyan"); width: 2; name: "RPM" }
                }

                // Temperature chart
                GraphsView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 100

                    theme: GraphsTheme {
                        colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                        plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                        grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                    }

                    axisX: ValueAxis { id: tempAxisXX; min: 0; max: maxSamples; titleText: "Samples" }
                    axisY: ValueAxis { id: tempAxisY; min: 0; max: 100; titleText: "Temp (°C)" }

                    LineSeries { id: tempMosfetSeries; color: Utility.getAppHexColor("red"); width: 2; name: "MOSFET" }
                    LineSeries { id: tempMotorSeries; color: Utility.getAppHexColor("magenta"); width: 2; name: "Motor" }
                }
            }
        }

        // ---- Tab 1: Values ----
        Item {
            SplitView {
                anchors.fill: parent
                orientation: Qt.Horizontal

                ScrollView {
                    SplitView.fillWidth: true
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 2

                        Label {
                            text: "Realtime Values"
                            font.bold: true
                            font.pointSize: 14
                            Layout.bottomMargin: 4
                        }

                        Repeater {
                            id: valRepeater
                            model: ListModel {
                                id: valModel
                                ListElement { label: "MOSFET Temp"; val: "--" }
                                ListElement { label: "Motor Temp"; val: "--" }
                                ListElement { label: "Motor Current"; val: "--" }
                                ListElement { label: "Battery Current"; val: "--" }
                                ListElement { label: "Duty Cycle"; val: "--" }
                                ListElement { label: "ERPM"; val: "--" }
                                ListElement { label: "Input Voltage"; val: "--" }
                                ListElement { label: "Ah Used"; val: "--" }
                                ListElement { label: "Ah Charged"; val: "--" }
                                ListElement { label: "Wh Used"; val: "--" }
                                ListElement { label: "Wh Charged"; val: "--" }
                                ListElement { label: "Tacho"; val: "--" }
                                ListElement { label: "Tacho ABS"; val: "--" }
                                ListElement { label: "Position"; val: "--" }
                                ListElement { label: "Fault Code"; val: "--" }
                                ListElement { label: "VQ"; val: "--" }
                                ListElement { label: "VD"; val: "--" }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Label {
                                    text: model.label + ":"
                                    font.pointSize: 12
                                    Layout.preferredWidth: 130
                                    color: Utility.getAppHexColor("lightText")
                                }
                                Label {
                                    text: model.val
                                    font.pointSize: 12
                                    font.family: "DejaVu Sans Mono"
                                    Layout.fillWidth: true
                                    color: Utility.getAppHexColor("lightText")
                                }
                            }
                        }

                        Label {
                            text: "Setup Values"
                            font.bold: true
                            font.pointSize: 14
                            Layout.topMargin: 12
                            Layout.bottomMargin: 4
                        }

                        Repeater {
                            id: setupRepeater
                            model: ListModel {
                                id: setupModel
                                ListElement { label: "Battery Voltage"; val: "--" }
                                ListElement { label: "Battery Current"; val: "--" }
                                ListElement { label: "Motor Current"; val: "--" }
                                ListElement { label: "Duty"; val: "--" }
                                ListElement { label: "ERPM"; val: "--" }
                                ListElement { label: "Speed"; val: "--" }
                                ListElement { label: "Distance"; val: "--" }
                                ListElement { label: "Ah Used"; val: "--" }
                                ListElement { label: "Ah Charged"; val: "--" }
                                ListElement { label: "Wh Used"; val: "--" }
                                ListElement { label: "Wh Charged"; val: "--" }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Label {
                                    text: model.label + ":"
                                    font.pointSize: 12
                                    Layout.preferredWidth: 130
                                    color: Utility.getAppHexColor("lightText")
                                }
                                Label {
                                    text: model.val
                                    font.pointSize: 12
                                    font.family: "DejaVu Sans Mono"
                                    Layout.fillWidth: true
                                    color: Utility.getAppHexColor("lightText")
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- Tab 2: IMU ----
        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: 4

                GridLayout {
                    Layout.fillWidth: true
                    columns: 6
                    rowSpacing: 4
                    columnSpacing: 12

                    Label { text: "Roll:"; font.bold: true }
                    Label { id: rollLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
                    Label { text: "Pitch:"; font.bold: true }
                    Label { id: pitchLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
                    Label { text: "Yaw:"; font.bold: true }
                    Label { id: yawLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }

                    Label { text: "Acc X:"; font.bold: true }
                    Label { id: accXLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
                    Label { text: "Acc Y:"; font.bold: true }
                    Label { id: accYLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
                    Label { text: "Acc Z:"; font.bold: true }
                    Label { id: accZLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }

                    Label { text: "Gyro X:"; font.bold: true }
                    Label { id: gyroXLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
                    Label { text: "Gyro Y:"; font.bold: true }
                    Label { id: gyroYLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
                    Label { text: "Gyro Z:"; font.bold: true }
                    Label { id: gyroZLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
                }

                CheckBox {
                    id: useYawBox
                    text: "Use Yaw (will drift)"
                    checked: false
                }

                // 3D View
                View3D {
                    id: imu3dView
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    property real roll: 0
                    property real pitch: 0
                    property real yaw: 0

                    environment: SceneEnvironment {
                        clearColor: Utility.getAppHexColor("plotBackground")
                        backgroundMode: SceneEnvironment.Color
                        antialiasingMode: SceneEnvironment.MSAA
                        antialiasingQuality: SceneEnvironment.High
                    }

                    PerspectiveCamera {
                        id: imuCamera
                        position: Qt.vector3d(0, 200, 300)
                        eulerRotation.x: -30
                    }

                    DirectionalLight {
                        eulerRotation.x: -30
                        eulerRotation.y: -70
                        color: "#ffffff"
                        brightness: 1.0
                    }

                    DirectionalLight {
                        eulerRotation.x: 30
                        eulerRotation.y: 70
                        color: "#888888"
                        brightness: 0.5
                    }

                    Model {
                        id: imuModel
                        source: "#Cube"
                        scale: Qt.vector3d(2.0, 0.3, 1.2)

                        materials: [
                            PrincipledMaterial {
                                baseColor: Utility.getAppHexColor("midAccent")
                                roughness: 0.4
                                metalness: 0.1
                            }
                        ]

                        eulerRotation: Qt.vector3d(
                            imu3dView.pitch * 180 / Math.PI,
                            imu3dView.yaw * 180 / Math.PI,
                            imu3dView.roll * 180 / Math.PI
                        )
                    }

                    function setRotation(r, p, y) {
                        roll = r
                        pitch = p
                        yaw = y
                    }
                }
            }
        }

        // ---- Tab 3: Statistics ----
        Item {
            ScrollView {
                anchors.fill: parent
                clip: true

                ColumnLayout {
                    width: parent.width
                    spacing: 2

                    Label {
                        text: "Statistics"
                        font.bold: true
                        font.pointSize: 14
                        Layout.bottomMargin: 4
                    }

                    Repeater {
                        model: ListModel {
                            id: statsModel
                            ListElement { label: "Speed Avg"; val: "--" }
                            ListElement { label: "Speed Max"; val: "--" }
                            ListElement { label: "Power Avg"; val: "--" }
                            ListElement { label: "Power Max"; val: "--" }
                            ListElement { label: "Current Avg"; val: "--" }
                            ListElement { label: "Current Max"; val: "--" }
                            ListElement { label: "Temp MOSFET Avg"; val: "--" }
                            ListElement { label: "Temp MOSFET Max"; val: "--" }
                            ListElement { label: "Temp Motor Avg"; val: "--" }
                            ListElement { label: "Temp Motor Max"; val: "--" }
                            ListElement { label: "Distance"; val: "--" }
                            ListElement { label: "Efficiency"; val: "--" }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Label {
                                text: model.label + ":"
                                font.pointSize: 12
                                Layout.preferredWidth: 140
                                color: Utility.getAppHexColor("lightText")
                            }
                            Label {
                                text: model.val
                                font.pointSize: 12
                                font.family: "DejaVu Sans Mono"
                                Layout.fillWidth: true
                                color: Utility.getAppHexColor("lightText")
                            }
                        }
                    }

                    RowLayout {
                        Layout.topMargin: 8
                        Button {
                            text: "Reset Stats"
                            onClicked: mCommands.resetStats(0xFFFFFFFF)
                        }
                    }
                }
            }
        }
    }

    // ---------------------------------------------------------------
    // Data connections
    // ---------------------------------------------------------------
    Connections {
        target: mCommands

        function onValuesReceived(values, mask) {
            // Update charts (Tab 0)
            if (rtTabBar.currentIndex === 0) {
                motorCurrentSeries.append(sampleIndex, values.current_motor)
                batteryCurrentSeries.append(sampleIndex, values.current_in)
                dutySeries.append(sampleIndex, values.duty_now * 100)
                rpmSeries.append(sampleIndex, values.rpm)
                tempMosfetSeries.append(sampleIndex, values.temp_mos)
                tempMotorSeries.append(sampleIndex, values.temp_motor)

                sampleIndex++

                if (sampleIndex > maxSamples) {
                    currentAxisX.min = sampleIndex - maxSamples
                    currentAxisX.max = sampleIndex
                    rpmAxisX.min = sampleIndex - maxSamples
                    rpmAxisX.max = sampleIndex
                    tempAxisXX.min = sampleIndex - maxSamples
                    tempAxisXX.max = sampleIndex
                }

                // Auto-range Y axes
                autoRangeCounter++
                if (autoRangeCounter >= autoRangeInterval) {
                    autoRangeCounter = 0

                    var currRange = calcSeriesRange([motorCurrentSeries, batteryCurrentSeries, dutySeries])
                    autoRangeAxis(currentAxisY, currRange, true, 20)

                    var rpmRange = calcSeriesRange([rpmSeries])
                    autoRangeAxis(rpmAxisY, rpmRange, true, 1000)

                    var tempRange = calcSeriesRange([tempMosfetSeries, tempMotorSeries])
                    autoRangeAxis(tempAxisY, tempRange, false, 10)
                }

                // Trim old data
                while (motorCurrentSeries.count > maxSamples + 100) {
                    motorCurrentSeries.remove(0)
                    batteryCurrentSeries.remove(0)
                    dutySeries.remove(0)
                    rpmSeries.remove(0)
                    tempMosfetSeries.remove(0)
                    tempMotorSeries.remove(0)
                }
            }

            // Update values table (Tab 1)
            if (rtTabBar.currentIndex === 1) {
                var idx = 0
                valModel.setProperty(idx++, "val", values.temp_mos.toFixed(1) + " °C")
                valModel.setProperty(idx++, "val", values.temp_motor.toFixed(1) + " °C")
                valModel.setProperty(idx++, "val", values.current_motor.toFixed(2) + " A")
                valModel.setProperty(idx++, "val", values.current_in.toFixed(2) + " A")
                valModel.setProperty(idx++, "val", (values.duty_now * 100).toFixed(1) + " %")
                valModel.setProperty(idx++, "val", values.rpm.toFixed(0))
                valModel.setProperty(idx++, "val", values.v_in.toFixed(2) + " V")
                valModel.setProperty(idx++, "val", values.amp_hours.toFixed(4) + " Ah")
                valModel.setProperty(idx++, "val", values.amp_hours_charged.toFixed(4) + " Ah")
                valModel.setProperty(idx++, "val", values.watt_hours.toFixed(4) + " Wh")
                valModel.setProperty(idx++, "val", values.watt_hours_charged.toFixed(4) + " Wh")
                valModel.setProperty(idx++, "val", values.tachometer.toFixed(0))
                valModel.setProperty(idx++, "val", values.tachometer_abs.toFixed(0))
                valModel.setProperty(idx++, "val", values.position.toFixed(2) + " °")
                valModel.setProperty(idx++, "val", values.fault_str)
                valModel.setProperty(idx++, "val", values.vq.toFixed(2) + " V")
                valModel.setProperty(idx++, "val", values.vd.toFixed(2) + " V")
            }
        }

        function onValuesSetupReceived(values, mask) {
            if (rtTabBar.currentIndex === 1) {
                var idx = 0
                setupModel.setProperty(idx++, "val", values.battery_wh.toFixed(2) + " V")
                setupModel.setProperty(idx++, "val", values.current_in.toFixed(2) + " A")
                setupModel.setProperty(idx++, "val", values.current_motor.toFixed(2) + " A")
                setupModel.setProperty(idx++, "val", (values.duty_now * 100).toFixed(1) + " %")
                setupModel.setProperty(idx++, "val", values.rpm.toFixed(0))
                setupModel.setProperty(idx++, "val", values.speed.toFixed(2) + " m/s")
                setupModel.setProperty(idx++, "val", values.odometer.toFixed(2) + " m")
                setupModel.setProperty(idx++, "val", values.amp_hours.toFixed(4) + " Ah")
                setupModel.setProperty(idx++, "val", values.amp_hours_charged.toFixed(4) + " Ah")
                setupModel.setProperty(idx++, "val", values.watt_hours.toFixed(4) + " Wh")
                setupModel.setProperty(idx++, "val", values.watt_hours_charged.toFixed(4) + " Wh")
            }
        }

        function onValuesImuReceived(values, mask) {
            if (rtTabBar.currentIndex === 2) {
                rollLabel.text = (values.roll * 180 / Math.PI).toFixed(1) + " °"
                pitchLabel.text = (values.pitch * 180 / Math.PI).toFixed(1) + " °"
                yawLabel.text = (values.yaw * 180 / Math.PI).toFixed(1) + " °"
                accXLabel.text = values.accX.toFixed(3) + " G"
                accYLabel.text = values.accY.toFixed(3) + " G"
                accZLabel.text = values.accZ.toFixed(3) + " G"
                gyroXLabel.text = values.gyroX.toFixed(1) + " °/s"
                gyroYLabel.text = values.gyroY.toFixed(1) + " °/s"
                gyroZLabel.text = values.gyroZ.toFixed(1) + " °/s"

                imu3dView.setRotation(values.roll, values.pitch,
                                      useYawBox.checked ? values.yaw : 0)
            }
        }

        function onStatsRx(values, mask) {
            if (rtTabBar.currentIndex === 3 && values !== undefined && values.speed_avg !== undefined) {
                var idx = 0
                statsModel.setProperty(idx++, "val", values.speed_avg.toFixed(2) + " km/h")
                statsModel.setProperty(idx++, "val", values.speed_max.toFixed(2) + " km/h")
                statsModel.setProperty(idx++, "val", values.power_avg.toFixed(1) + " W")
                statsModel.setProperty(idx++, "val", values.power_max.toFixed(1) + " W")
                statsModel.setProperty(idx++, "val", values.current_avg.toFixed(2) + " A")
                statsModel.setProperty(idx++, "val", values.current_max.toFixed(2) + " A")
                statsModel.setProperty(idx++, "val", values.temp_mos_avg.toFixed(1) + " °C")
                statsModel.setProperty(idx++, "val", values.temp_mos_max.toFixed(1) + " °C")
                statsModel.setProperty(idx++, "val", values.temp_motor_avg.toFixed(1) + " °C")
                statsModel.setProperty(idx++, "val", values.temp_motor_max.toFixed(1) + " °C")
                var dist = values.distance()
                var eff = values.efficiency()
                statsModel.setProperty(idx++, "val", (dist !== undefined ? dist.toFixed(2) : "0.00") + " m")
                statsModel.setProperty(idx++, "val", (eff !== undefined && isFinite(eff) ? eff.toFixed(2) : "0.00") + " Wh/km")
            }
        }
    }
}
