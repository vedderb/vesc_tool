/*
    Copyright 2026 Jeffrey M. Friesen

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

// The Virtual ESC serves this page as the QML UI of its hardware. It uses the
// sim terminal command, so everything here can also be done from the terminal.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0
import Vedder.vesc.virtualdisplay 1.0
import "qrc:/mobile"

Item {
    id: root
    anchors.fill: parent

    property string tabTitle: "Sim"
    property Commands mCommands: VescIf.commands()
    property var values: ({}) // Lines of sim get by name: name, value, unit, min, max, decimals

    function sim(cmd) {
        mCommands.sendTerminalCmd("sim " + cmd)
    }

    function value(name, fallback) {
        return values[name] ? values[name][1] : fallback
    }

    Component.onCompleted: sim("get")
    onVisibleChanged: if (visible) sim("get")

    // Values change after every sim command, also from the terminal
    Timer {
        id: refreshTimer
        interval: 100
        onTriggered: sim("get")
    }

    Connections {
        target: mCommands

        function onPrintReceived(str) {
            if (str.startsWith("-> sim ") && !str.startsWith("-> sim get")) {
                refreshTimer.restart()
            }

            var t = str.trim().split(/\s+/)
            if ((t.length === 6 && !isNaN(t[1])) ||
                    (t.length === 2 && ["scenario", "throttle", "fault", "can", "imu", "dash"].includes(t[0]))) {
                var v = Object.assign({}, values)
                v[t[0]] = t
                values = v
            }
        }
    }

    // Rows for the values of sim get, model: [name, label]. Not an inline
    // component, as the page is created from a string.
    Component {
        id: valueRow

        RowLayout {
            id: row
            property string name: modelData[0]
            property var line: root.values[name]
            property bool updating: false

            Layout.fillWidth: true
            visible: line !== undefined

            // The range is set first, the spin box clamps the value to it
            onLineChanged: {
                if (line) {
                    updating = true
                    box.decimals = Number(line[5])
                    box.realStepSize = Math.pow(10, -box.decimals)
                    box.realFrom = Number(line[3])
                    box.realTo = Number(line[4])
                    box.suffix = " " + line[2]
                    box.realValue = Number(line[1])
                    updating = false
                }
            }

            Text {
                Layout.fillWidth: true
                text: modelData[1]
                color: Utility.getAppHexColor("lightText")
                elide: Text.ElideRight
            }

            DoubleSpinBox {
                id: box
                Layout.preferredWidth: 190
                decimals: 0 // The text only updates with the value, so it is right for values that start at 0

                onRealValueChanged: {
                    if (row.line && !row.updating && realValue.toFixed(decimals) !== row.line[1]) {
                        root.sim("set " + row.name + " " + realValue.toFixed(decimals))
                    }
                }
            }
        }
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        contentHeight: grid.height + 20
        clip: true

        GridLayout {
            id: grid
            x: 10
            width: scroll.availableWidth - 20
            columns: width > 700 ? 2 : 1
            columnSpacing: 16

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.preferredWidth: 1 // Equal columns

                SectionHeader { text: "Motor" }

                Text {
                    Layout.fillWidth: true
                    color: Utility.getAppHexColor("disabledText")
                    wrapMode: Text.WordWrap
                    text: "Detection measures these values, the motor configuration does not change."
                }

                Repeater {
                    model: [["motor_r", "Resistance"], ["motor_l", "Inductance"], ["motor_ld_lq", "Ld - Lq"],
                        ["motor_flux", "Flux linkage"], ["hall_offset", "Hall sensor offset"],
                        ["enc_offset", "Encoder offset"]]
                    delegate: valueRow
                }

                SectionHeader { text: "Ride" }

                RowLayout {
                    Text {
                        Layout.fillWidth: true
                        color: Utility.getAppHexColor("lightText")
                        text: "Scenario"
                    }

                    ComboBox {
                        Layout.preferredWidth: 190
                        property var names: ["cruise", "hill", "stopgo", "idle"]
                        model: ["Cruise", "Hill", "Stop and go", "Idle"]
                        property var line: root.values.scenario
                        onLineChanged: currentIndex = names.indexOf(line[1])
                        onActivated: root.sim("scenario " + names[currentIndex])
                    }
                }

                RowLayout {
                    Text {
                        color: Utility.getAppHexColor("lightText")
                        text: "Throttle"
                    }

                    Slider {
                        id: throttleSlider
                        Layout.fillWidth: true
                        enabled: !autoBox.checked
                        from: -1.0
                        to: 1.0
                        property var line: root.values.throttle
                        onLineChanged: value = Number(line[1]) || 0 // 0 when auto
                        onMoved: mCommands.sendTerminalCmd("sim throttle " + value.toFixed(2))
                    }

                    Switch {
                        id: autoBox
                        text: "Rider"
                        checked: true
                        property var line: root.values.throttle
                        onLineChanged: checked = line[1] === "auto"
                        onToggled: root.sim("throttle " + (checked ? "auto" : throttleSlider.value.toFixed(2)))
                    }
                }

                Repeater {
                    model: [["mass", "Mass"]]
                    delegate: valueRow
                }

                RowLayout {
                    Text {
                        Layout.fillWidth: true
                        color: Utility.getAppHexColor("lightText")
                        text: "Fault"
                    }

                    ComboBox {
                        Layout.preferredWidth: 190
                        // In the order of mc_fault_code
                        model: ["None", "Over voltage", "Under voltage", "DRV", "ABS over current",
                            "FET over temp", "Motor over temp"]
                        property var line: root.values.fault
                        onLineChanged: currentIndex = Number(line[1]) < count ? Number(line[1]) : -1
                        displayText: currentIndex < 0 ? "Code " + root.value("fault", 0) : currentText
                        onActivated: root.sim("fault " + currentIndex)
                    }
                }

                Switch {
                    text: "CAN devices"
                    checked: true
                    property var line: root.values.can
                    onLineChanged: checked = line[1] === "on"
                    onToggled: root.sim("can " + (checked ? "on" : "off"))
                }

                Switch {
                    text: "Use phone IMU"
                    visible: root.value("imu", "none") !== "none"
                    property var line: root.values.imu
                    onLineChanged: checked = line[1] === "phone"
                    onToggled: root.sim("imu " + (checked ? "phone" : "model"))
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.preferredWidth: 1 // Equal columns

                SectionHeader { text: "Battery and BMS" }

                Repeater {
                    model: [["cells", "Cells in series"], ["cell_res", "Cell resistance"],
                        ["soc", "State of charge"], ["soh", "State of health"], ["pack_temp", "Temperature"]]
                    delegate: valueRow
                }

                RowLayout {
                    Text {
                        Layout.fillWidth: true
                        color: Utility.getAppHexColor("lightText")
                        text: "Override cell"
                    }

                    DoubleSpinBox {
                        id: cellBox
                        Layout.preferredWidth: 190
                        decimals: 0
                        realFrom: 1
                        realTo: Number(root.value("cells", 1))
                        realValue: 1
                    }
                }

                RowLayout {
                    DoubleSpinBox {
                        id: cellVoltageBox
                        Layout.fillWidth: true
                        decimals: 2
                        realFrom: 2.5
                        realTo: 4.5
                        realStepSize: 0.05
                        realValue: 3.3
                        suffix: " V"
                    }

                    Button {
                        text: "Set"
                        onClicked: root.sim("cell " + cellBox.realValue + " " + cellVoltageBox.realValue.toFixed(2))
                    }

                    Button {
                        text: "Clear"
                        onClicked: root.sim("cell " + cellBox.realValue + " off")
                    }
                }

                // Only in builds with LispBM
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.values.dash !== undefined

                    SectionHeader { text: "Dash 35B" }

                    Text {
                        Layout.fillWidth: true
                        color: Utility.getAppHexColor("disabledText")
                        wrapMode: Text.WordWrap
                        text: "On CAN ID 4. The display shows what its Lisp code draws, e.g. the Dash 35B package."
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: width * 2 / 3
                        color: "black"

                        VirtualDisplay {
                            anchors.fill: parent
                            canId: 4
                        }
                    }

                    RowLayout {
                        Repeater {
                            model: 4
                            Button {
                                Layout.fillWidth: true
                                text: "Button " + index
                                onPressed: root.sim("button " + index + " on")
                                onReleased: root.sim("button " + index + " off")
                                onCanceled: root.sim("button " + index + " off")
                            }
                        }
                    }
                }

                Button {
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    text: "Restore All Values"
                    onClicked: root.sim("reset")
                }
            }
        }
    }
}
