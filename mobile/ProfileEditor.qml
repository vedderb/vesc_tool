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
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.configparams 1.0

Item {
    id: editor

    property alias profileName: nameInput.text
    property double current: currentBox.realValue / 100
    property double currentBrake: currentBrakeBox.realValue / 100
    property double speedKmh: 0.0
    property double speedKmhRev: 0.0
    property alias powerMax: powerMaxBox.realValue
    property alias powerMin: powerMinBox.realValue
    signal closed(bool ok)

    property ConfigParams mMcConf: VescIf.mcConfig()

    function getMcConfTemp() {
        var conf = VescIf.createMcconfTemp()
        conf.current_max_scale = current
        conf.current_min_scale = currentBrake
        conf.erpm_or_speed_max = speedKmh / 3.6
        conf.erpm_or_speed_min = -speedKmhRev / 3.6
        conf.duty_min = mMcConf.getParamDouble("l_min_duty")
        conf.duty_max = mMcConf.getParamDouble("l_max_duty")
        conf.watt_max = powerMax
        conf.watt_min = powerMin
        conf.name = profileName
        return conf
    }

    function updateFromMcConfTemp(conf) {
        currentBox.realValue = conf.current_max_scale * 100
        currentBrakeBox.realValue = conf.current_min_scale * 100
        speedKmh = conf.erpm_or_speed_max * 3.6
        speedKmhRev = -conf.erpm_or_speed_min * 3.6
        powerMax = conf.watt_max
        powerMin = conf.watt_min
        profileName = conf.name
        updatePowerChecked()
        updateSpeedBoxes()
    }

    function openDialog() {
        dialog.open()
    }

    function readCurrentConfig() {
        currentBox.realValue = mMcConf.getParamDouble("l_current_max_scale") * 100
        currentBrakeBox.realValue = mMcConf.getParamDouble("l_current_min_scale") * 100
        powerMaxBox.realValue = mMcConf.getParamDouble("l_watt_max")
        powerMinBox.realValue = mMcConf.getParamDouble("l_watt_min")

        var speedFact = ((mMcConf.getParamInt("si_motor_poles") / 2.0) * 60.0 *
                mMcConf.getParamDouble("si_gear_ratio")) /
                (mMcConf.getParamDouble("si_wheel_diameter") * Math.PI)

        speedKmh = 3.6 * mMcConf.getParamDouble("l_max_erpm") / speedFact
        speedKmhRev = 3.6 * -mMcConf.getParamDouble("l_min_erpm") / speedFact
        updatePowerChecked()
        updateSpeedBoxes()
    }

    function testConnected() {
        if (VescIf.isPortConnected()) {
            return true
        } else {
            VescIf.emitMessageDialog(
                        "Connection Error",
                        "The VESC is not connected. Please connect it to run detection.",
                        false, false)
            return false
        }
    }

    function setupSpinboxFromConfig(box, config, paramName) {
        box.decimals = config.getParamDecimalsDouble(paramName)
        box.realFrom = config.getParamMinDouble(paramName) * config.getParamEditorScale(paramName)
        box.realTo = config.getParamMaxDouble(paramName) * config.getParamEditorScale(paramName)
        box.realStepSize = config.getParamStepDouble(paramName)
        box.suffix = config.getParamSuffix(paramName)
    }

    function updatePowerChecked() {
        powerBox.checked = powerMax < 1400000 || powerMin > -1400000
    }

    function updateSpeedBoxes() {
        var useImperial = VescIf.useImperialUnits()
        var impFact = useImperial ? 0.621371192 : 1.0
        var speedUnit = useImperial ? " mph" : " km/h"

        speedKmhBox.realValue = speedKmh * impFact
        speedKmhRevBox.realValue = speedKmhRev * impFact
        speedKmhBox.suffix = speedUnit
        speedKmhRevBox.suffix = speedUnit
    }

    Component.onCompleted: {
        setupSpinboxFromConfig(powerMaxBox, mMcConf, "l_watt_max")
        setupSpinboxFromConfig(powerMinBox, mMcConf, "l_watt_min")
        updateSpeedBoxes()
    }

    Connections {
        target: VescIf

        onUseImperialUnitsChanged: {
            updateSpeedBoxes()
        }
    }

    Dialog {
        id: dialog
        modal: true
        focus: true
        width: parent.width - 20
        height: Math.min(implicitHeight, parent.height - 60)
        closePolicy: Popup.CloseOnEscape
        x: 10
        y: 50
        parent: ApplicationWindow.overlay

        ColumnLayout {
            id: scrollColumn
            anchors.fill: parent

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: parent.width
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Text {
                        color: "white"
                        text: "Profile Name"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                        font.pointSize: 12
                        Layout.bottomMargin: 3
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.bottomMargin: 20
                        height: nameInput.implicitHeight + 14
                        border.width: 2
                        border.color: "#8d8d8d"
                        color: "#33a8a8a8"
                        radius: 3
                        TextInput {
                            id: nameInput
                            text: ""
                            color: "#ffffff"
                            anchors.fill: parent
                            anchors.margins: 7
                            font.pointSize: 12
                            focus: true
                        }
                    }

                    Text {
                        color: "white"
                        text: "Speed Limit"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                        font.pointSize: 12
                    }

                    DoubleSpinBox {
                        id: speedKmhBox
                        prefix: "Forward: "
                        realFrom: 0.0
                        realTo: 400.0
                        Layout.fillWidth: true

                        onRealValueChanged: {
                            speedKmh = realValue / (VescIf.useImperialUnits() ? 0.621371192 : 1.0)
                        }
                    }

                    DoubleSpinBox {
                        id: speedKmhRevBox
                        prefix: "Reverse: "
                        realFrom: 0.0
                        realTo: 400.0
                        Layout.fillWidth: true
                        Layout.bottomMargin: 20

                        onRealValueChanged: {
                            speedKmhRev = realValue / (VescIf.useImperialUnits() ? 0.621371192 : 1.0)
                        }
                    }

                    Text {
                        color: "white"
                        text: "Motor Current Scale"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                        font.pointSize: 12
                    }

                    DoubleSpinBox {
                        id: currentBox
                        Layout.fillWidth: true
                        prefix: "Accel: "
                        suffix: " %"
                        realFrom: 0
                        realTo: 100
                        realStepSize: 5
                    }

                    DoubleSpinBox {
                        id: currentBrakeBox
                        Layout.fillWidth: true
                        Layout.bottomMargin: 20
                        prefix: "Brake: "
                        suffix: " %"
                        realFrom: 0
                        realTo: 100
                        realStepSize: 5
                    }

                    CheckBox {
                        id: powerBox
                        text: "Enable Power Limit"
                        checked: false
                        Layout.fillWidth: true

                        onToggled: {
                            if (!checked) {
                                powerMax = 1500000
                                powerMin = -1500000
                            } else {
                                powerMax = 5000
                                powerMin = -5000
                            }
                        }
                    }

                    DoubleSpinBox {
                        id: powerMaxBox
                        prefix: "Output: "
                        Layout.fillWidth: true
                        visible: powerBox.checked
                    }

                    DoubleSpinBox {
                        id: powerMinBox
                        prefix: "Regen: "
                        Layout.fillWidth: true
                        visible: powerBox.checked
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Button {
                    Layout.preferredWidth: 50
                    Layout.fillWidth: true
                    text: "..."
                    onClicked: menu.open()

                    Menu {
                        id: menu
                        width: 500

                        MenuItem {
                            text: "Read current configuration"
                            onTriggered: {
                                readCurrentConfig()
                            }
                        }
                    }
                }

                Button {
                    text: "Cancel"
                    Layout.fillWidth: true
                    onClicked: {
                        dialog.close()
                        closed(false)
                    }
                }

                Button {
                    id: okButton
                    text: "Ok"
                    Layout.fillWidth: true
                    onClicked: {
                        dialog.close()
                        closed(true)
                    }
                }
            }
        }
    }
}
