/*
    Desktop BmsPage — native implementation matching the original widget page.
    Features: Cell voltage bar chart, temperature bar chart, value table,
    control buttons (balance on/off, charge enable/disable, reset counters).
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs
import Vedder.vesc

Item {
    id: bmsPage

    property Commands mCommands: VescIf.commands()

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // Left: Charts
        ColumnLayout {
            SplitView.fillWidth: true
            SplitView.minimumWidth: 400
            spacing: 4

            TabBar {
                id: bmsTabBar
                Layout.fillWidth: true
                TabButton { text: "Cell Voltages"; topPadding: 9; bottomPadding: 9 }
                TabButton { text: "Temperatures"; topPadding: 9; bottomPadding: 9 }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: bmsTabBar.currentIndex

                // Cell voltage chart
                GraphsView {
                    id: cellChart
                    theme: GraphsTheme {
                        colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                        plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                        grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                    }

                    axisX: ValueAxis { id: cellAxisX; min: 0; max: 13; titleText: "Cell" }
                    axisY: ValueAxis { id: cellAxisY; min: 2.5; max: 4.4; titleText: "Voltage (V)" }

                    BarSeries {
                        id: cellBarSeries
                        barWidth: 0.6
                    }
                }

                // Temperature chart
                GraphsView {
                    id: tempChart
                    theme: GraphsTheme {
                        colorScheme: Utility.isDarkMode() ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                        plotAreaBackgroundColor: Utility.getAppHexColor("plotBackground")
                        grid.mainColor: Utility.isDarkMode() ? "#444444" : "#cccccc"
                    }

                    axisX: ValueAxis { id: tempAxisX; min: 0; max: 6; titleText: "Sensor" }
                    axisY: ValueAxis { id: tempAxisY; min: -20; max: 90; titleText: "Temperature (°C)" }

                    BarSeries {
                        id: tempBarSeries
                        barWidth: 0.6
                    }
                }
            }

            // Control buttons
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 4
                spacing: 4

                Button { text: "Balance On"; onClicked: mCommands.bmsForceBalance(true) }
                Button { text: "Balance Off"; onClicked: mCommands.bmsForceBalance(false) }
                Button { text: "Charge Enable"; onClicked: mCommands.bmsSetChargeAllowed(true) }
                Button { text: "Charge Disable"; onClicked: mCommands.bmsSetChargeAllowed(false) }
                Button { text: "Zero Current"; onClicked: mCommands.bmsZeroCurrentOffset() }
                Button { text: "Reset Ah"; onClicked: mCommands.bmsResetCounters(true, false) }
                Button { text: "Reset Wh"; onClicked: mCommands.bmsResetCounters(false, true) }
            }
        }

        // Right: Value table
        ScrollView {
            SplitView.preferredWidth: 280
            SplitView.minimumWidth: 200
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 2

                Label {
                    text: "BMS Values"
                    font.bold: true
                    font.pointSize: 14
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    Layout.bottomMargin: 4
                }

                Repeater {
                    id: valRepeater
                    model: ListModel {
                        id: valModel
                        ListElement { label: "V Total"; val: "--" }
                        ListElement { label: "V Cell Min"; val: "--" }
                        ListElement { label: "V Cell Max"; val: "--" }
                        ListElement { label: "V Cell Diff"; val: "--" }
                        ListElement { label: "V Charge"; val: "--" }
                        ListElement { label: "Current"; val: "--" }
                        ListElement { label: "Current IC"; val: "--" }
                        ListElement { label: "Ah Counter"; val: "--" }
                        ListElement { label: "Wh Counter"; val: "--" }
                        ListElement { label: "Power"; val: "--" }
                        ListElement { label: "SoC"; val: "--" }
                        ListElement { label: "SoH"; val: "--" }
                        ListElement { label: "T Cell Max"; val: "--" }
                        ListElement { label: "T PCB Max"; val: "--" }
                        ListElement { label: "Humidity"; val: "--" }
                        ListElement { label: "Pressure"; val: "--" }
                        ListElement { label: "Ah Chg Total"; val: "--" }
                        ListElement { label: "Wh Chg Total"; val: "--" }
                        ListElement { label: "Ah Dis Total"; val: "--" }
                        ListElement { label: "Wh Dis Total"; val: "--" }
                        ListElement { label: "Status"; val: "--" }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Label {
                            text: model.label + ":"
                            font.pointSize: 11
                            Layout.preferredWidth: 100
                            color: Utility.getAppHexColor("lightText")
                        }
                        Label {
                            text: model.val
                            font.pointSize: 11
                            font.family: "DejaVu Sans Mono"
                            Layout.fillWidth: true
                            color: Utility.getAppHexColor("lightText")
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: mCommands
        function onBmsValuesRx(val) {
            // Update value table
            var vcMin = 0, vcMax = 0
            if (val.v_cells.length > 0) {
                vcMin = val.v_cells[0]
                vcMax = vcMin
                for (var i = 0; i < val.v_cells.length; i++) {
                    if (val.v_cells[i] < vcMin) vcMin = val.v_cells[i]
                    if (val.v_cells[i] > vcMax) vcMax = val.v_cells[i]
                }
            }

            var idx = 0
            valModel.setProperty(idx++, "val", val.v_tot.toFixed(2) + " V")
            valModel.setProperty(idx++, "val", vcMin.toFixed(3) + " V")
            valModel.setProperty(idx++, "val", vcMax.toFixed(3) + " V")
            valModel.setProperty(idx++, "val", (vcMax - vcMin).toFixed(3) + " V")
            valModel.setProperty(idx++, "val", val.v_charge.toFixed(2) + " V")
            valModel.setProperty(idx++, "val", val.i_in.toFixed(2) + " A")
            valModel.setProperty(idx++, "val", val.i_in_ic.toFixed(2) + " A")
            valModel.setProperty(idx++, "val", val.ah_cnt.toFixed(3) + " Ah")
            valModel.setProperty(idx++, "val", val.wh_cnt.toFixed(3) + " Wh")
            valModel.setProperty(idx++, "val", (val.i_in_ic * val.v_tot).toFixed(3) + " W")
            valModel.setProperty(idx++, "val", (val.soc * 100).toFixed(0) + " %")
            valModel.setProperty(idx++, "val", (val.soh * 100).toFixed(0) + " %")
            valModel.setProperty(idx++, "val", val.temp_cells_highest.toFixed(2) + " °C")
            valModel.setProperty(idx++, "val", val.temp_hum_sensor.toFixed(2) + " °C")
            valModel.setProperty(idx++, "val", val.humidity.toFixed(2) + " % (" + val.temp_hum_sensor.toFixed(2) + " °C)")
            valModel.setProperty(idx++, "val", val.pressure.toFixed(0) + " Pa")
            valModel.setProperty(idx++, "val", val.ah_cnt_chg_total.toFixed(3) + " Ah")
            valModel.setProperty(idx++, "val", val.wh_cnt_chg_total.toFixed(3) + " Wh")
            valModel.setProperty(idx++, "val", val.ah_cnt_dis_total.toFixed(3) + " Ah")
            valModel.setProperty(idx++, "val", val.wh_cnt_dis_total.toFixed(3) + " Wh")
            valModel.setProperty(idx++, "val", val.status)

            // Update cell chart axis
            cellAxisX.max = val.v_cells.length + 1
        }
    }
}
