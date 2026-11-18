/*
    Copyright 2018 - 2019 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Controls.Material
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    property var dialogParent: Overlay.overlay
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()
    readonly property real dockWidth: dialog.visible ? dialog.dockWidth : 0

    property var modes: [
        {label: "Motor current", suffix: " A", decimals: 1, to: 500.0, step: 1.0, brake: "Brake",
            set: v => mCommands.setCurrent(v), stop: v => mCommands.setCurrentBrake(v)},
        {label: "Duty cycle", suffix: "", decimals: 3, to: 1.0, step: 0.01, brake: "0 Duty (brake)",
            set: v => mCommands.setDutyCycle(v), stop: v => mCommands.setDutyCycle(0.0)},
        {label: "Speed", suffix: " ERPM", decimals: 1, to: 500000, step: 500, brake: "0 ERPM (brake)",
            set: v => mCommands.setRpm(v), stop: v => mCommands.setRpm(0.0)},
        {label: "Position", suffix: " \u00B0", decimals: 3, to: 360, step: 0.1, brake: "",
            set: v => mCommands.setPos(v)}
    ]
    property var values: [3.0, 0.1, 3000, 20]
    property var mode: modes[modeBar.currentIndex]
    property real value: values[modeBar.currentIndex]
    property var readouts: []

    function openDialog() {
        dialog.open()
        opacitySlider.value = 1
    }

    function setValue(v) {
        if (!isNaN(v)) {
            // Assigning the same array does not update the bindings
            var newValues = values.slice()
            newValues[modeBar.currentIndex] = Math.min(Math.max(v, 0.0), mode.to)
            values = newValues
        }
    }

    // Runs the action while pressed and releases the motor afterwards unless told to continue
    function hold(pressed, action) {
        if (pressed) {
            action()
        } else if (!maintainBox.checked) {
            mCommands.setCurrent(0.0)
        }
    }

    SheetDialog {
        id: dialog
        parent: dialogParent
        title: "Controls"
        modal: false
        closePolicy: Popup.CloseOnEscape
        dockWidth: mainIsTablet && mainIsHorizontal && appWindow.width - canDockWidth >= 1080 ? 380 : 0
        background.opacity: dockWidth > 0 ? 1 : opacitySlider.value

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 10

                SegmentedBar {
                    id: modeBar
                    Layout.fillWidth: true
                    model: ["Current", "Duty", "RPM", "Position"]
                    onActivated: function(index) { currentIndex = index }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: setpoint.implicitHeight + 28
                    radius: 12
                    color: Utility.getAppHexColor("cardBackground")
                    border.color: Utility.getAppHexColor("raisedBackground")

                    ColumnLayout {
                        id: setpoint
                        anchors.centerIn: parent
                        width: parent.width - 28
                        spacing: 8

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: mode.label
                            font.pointSize: 10
                            color: Utility.getAppHexColor("normalText")
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 12

                            RoundButton {
                                text: "\u2212"
                                font.pointSize: 18
                                autoRepeat: true
                                onClicked: setValue(value - mode.step)
                            }

                            TextInput {
                                Layout.preferredWidth: 170
                                horizontalAlignment: TextInput.AlignHCenter
                                // Editing replaces the text binding
                                property string valueText: parseFloat(value).toFixed(mode.decimals) + mode.suffix
                                text: valueText
                                onValueTextChanged: text = valueText
                                onActiveFocusChanged: if (activeFocus) selectAll()
                                font.pointSize: 26
                                font.weight: Font.Medium
                                font.features: { "tnum": 1 }
                                color: Utility.getAppHexColor("titleText")
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                onEditingFinished: {
                                    Qt.inputMethod.hide()
                                    setValue(parseFloat(text.replace(",", ".")))
                                    text = valueText
                                    focus = false
                                }
                            }

                            RoundButton {
                                text: "+"
                                font.pointSize: 18
                                autoRepeat: true
                                onClicked: setValue(value + mode.step)
                            }
                        }

                        Flow {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 6

                            Repeater {
                                model: readouts

                                Chip {
                                    enabled: false
                                    topInset: 0
                                    text: modelData
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: "Continue after release"
                            color: Utility.getAppHexColor("lightText")
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Off: the motor stops when you lift your finger")
                            wrapMode: Text.WordWrap
                            font.pointSize: 9
                            color: Utility.getAppHexColor("normalText")
                        }
                    }

                    Switch {
                        id: maintainBox
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: mode.brake !== ""

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 56
                        text: "Set REV"
                        onPressedChanged: hold(pressed, () => mode.set(-value))
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 56
                        text: "Set FWD"
                        onPressedChanged: hold(pressed, () => mode.set(value))
                    }
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    visible: mode.brake === ""
                    text: "Set"
                    onPressedChanged: hold(pressed, () => mode.set(value))
                }

                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 56
                        visible: mode.brake !== ""
                        text: mode.brake
                        onPressedChanged: hold(pressed, () => mode.stop(value))
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 56
                        text: "Release"
                        Material.background: Qt.darker(Utility.getAppHexColor("danger"), 1.2)
                        Material.foreground: Utility.getAppHexColor("white")

                        onClicked: {
                            mCommands.setCurrent(0.0)
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: dialog.dockWidth === 0

                    Text {
                        color: Utility.getAppHexColor("lightText")
                        text: qsTr("Opacity")
                    }

                    Slider {
                        id: opacitySlider
                        Layout.fillWidth: true
                        from: 0.1
                        to: 1.0
                        value: 1.0
                    }
                }
            }
        }
    }

    Connections {
        target: mCommands
        enabled: dialog.visible

        function onValuesReceived(val, mask) {
            readouts = ["Motor " + parseFloat(val.current_motor).toFixed(1) + " A",
                        parseFloat(val.rpm).toFixed(0) + " ERPM",
                        parseFloat(val.v_in).toFixed(1) + " V"]
        }
    }

    Timer {
        id: aliveTimer
        interval: 200
        running: true
        repeat: true
        onTriggered: {
            if (VescIf.isPortConnected() && dialog.visible) {
                mCommands.sendAlive()
                mCommands.getValues()
            }
        }
    }
}
