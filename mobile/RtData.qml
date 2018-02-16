/*
    Copyright 2017 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Extras 1.4

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property int gaugeSize: height / 4

    width: 400
    height: 620

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 5

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true

                CircularGauge {
                    id: currentGauge
                    Layout.fillWidth: true
                    minimumValue: 0
                    value: 0
                    Layout.preferredWidth: gaugeSize
                    Layout.preferredHeight: gaugeSize

                    Behavior on value {
                        NumberAnimation {
                            easing.type: Easing.OutCirc
                            duration: 100
                        }
                    }
                }

                Text {
                    id: currentText
                    text: qsTr("Motor 0 A")
                    horizontalAlignment: Text.AlignHCenter
                    Layout.preferredWidth: 27
                    font.pixelSize: 12
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    Layout.preferredHeight: 15
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true

                CircularGauge {
                    id: dutyGauge
                    Layout.fillWidth: true
                    minimumValue: -100
                    value: 0
                    Layout.preferredWidth: gaugeSize
                    Layout.preferredHeight: gaugeSize

                    Behavior on value {
                        NumberAnimation {
                            easing.type: Easing.OutCirc
                            duration: 100
                        }
                    }
                }

                Text {
                    id: dutyText
                    text: qsTr("Duty 0 %")
                    horizontalAlignment: Text.AlignHCenter
                    Layout.preferredWidth: 27
                    font.pixelSize: 12
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    Layout.preferredHeight: 15
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle {
            color: "#aaaaaa"
            Layout.fillWidth: true
            height: 2
        }

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true

                CircularGauge {
                    id: rpmGauge
                    Layout.fillWidth: true
                    minimumValue: 0
                    value: 0
                    Layout.preferredWidth: gaugeSize
                    Layout.preferredHeight: gaugeSize

                    Behavior on value {
                        NumberAnimation {
                            easing.type: Easing.OutCirc
                            duration: 100
                        }
                    }
                }

                Text {
                    id: rpmText
                    text: qsTr("0 ERPM")
                    horizontalAlignment: Text.AlignHCenter
                    Layout.preferredWidth: 27
                    font.pixelSize: 12
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    Layout.preferredHeight: 15
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true

                CircularGauge {
                    id: powerGauge
                    Layout.fillWidth: true
                    minimumValue: -100
                    value: 0
                    Layout.preferredWidth: gaugeSize
                    Layout.preferredHeight: gaugeSize

                    Behavior on value {
                        NumberAnimation {
                            easing.type: Easing.OutCirc
                            duration: 100
                        }
                    }
                }

                Text {
                    id: powerText
                    text: qsTr("Power 0 W")
                    horizontalAlignment: Text.AlignHCenter
                    Layout.preferredWidth: 27
                    font.pixelSize: 12
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    Layout.preferredHeight: 15
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle {
            id: textRect
            gradient: Gradient {
                GradientStop {
                    position: 0.00;
                    color: "#008fbe78"
                }
                GradientStop {
                    position: 0.146
                    color: "#e4eaff"
                }
                GradientStop {
                    position: 0.905
                    color: "#d8e4d2"
                }
                GradientStop {
                    position: 1
                    color: "#a4c096";
                }
            }

            Layout.fillWidth: true
            Layout.fillHeight: true

            Text {
                id: valText
                color: "black"
                text: VescIf.getConnectedPortName()
                font.family: "DejaVu Sans Mono"
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
                anchors.margins: 10
            }
        }
    }

    Connections {
        target: mMcConf

        onUpdated: {
            currentGauge.minimumValue = -mMcConf.getParamDouble("l_current_max")
            currentGauge.maximumValue = mMcConf.getParamDouble("l_current_max")
        }
    }

    Connections {
        target: mCommands

        onValuesReceived: {
            currentGauge.value = values.current_motor
            currentText.text = "Motor " + parseFloat(values.current_motor).toFixed(2) + " A"
            dutyGauge.value = values.duty_now * 100.0
            dutyText.text = "Duty " + parseFloat(values.duty_now * 100.0).toFixed(2) + " %"

            var fl = mMcConf.getParamDouble("foc_motor_flux_linkage")
            var rpmMax = (values.v_in * 60.0) / (Math.sqrt(3.0) * 2.0 * Math.PI * fl)
            var rpmMaxRound = (Math.ceil(rpmMax / 5000.0) * 5000.0) / 1000.0

            if (Math.abs(rpmGauge.maximumValue - rpmMaxRound) > 6) {
                rpmGauge.maximumValue = rpmMaxRound
                rpmGauge.minimumValue = -rpmMaxRound
            }

            rpmGauge.value = values.rpm / 1000.0
            rpmText.text = parseFloat(values.rpm) + " ERPM"

            var powerMax = Math.min(values.v_in * Math.min(mMcConf.getParamDouble("l_in_current_max"),
                                                           mMcConf.getParamDouble("l_current_max")),
                                    mMcConf.getParamDouble("l_watt_max"))
            var powerMaxRound = (Math.ceil(powerMax / 100.0) * 100.0) / 100

            if (Math.abs(powerGauge.maximumValue - powerMaxRound) > 1.2) {
                powerGauge.maximumValue = powerMaxRound
                powerGauge.minimumValue = -powerMaxRound
            }

            powerGauge.value = (values.current_in * values.v_in) / 100
            powerText.text = "Power " + parseFloat(values.current_in * values.v_in).toFixed(1) + " W"

            valText.text =
                    "Battery    : " + parseFloat(values.v_in).toFixed(2) + " V\n" +
                    "I Battery  : " + parseFloat(values.current_in).toFixed(2) + " A\n" +
                    "Temp MOS   : " + parseFloat(values.temp_mos).toFixed(2) + " \u00B0C\n" +
                    "Temp Motor : " + parseFloat(values.temp_motor).toFixed(2) + " \u00B0C\n" +
                    "Ah Draw    : " + parseFloat(values.amp_hours * 1000.0).toFixed(1) + " mAh\n" +
                    "Ah Charge  : " + parseFloat(values.amp_hours_charged * 1000.0).toFixed(1) + " mAh\n" +
                    "Wh Draw    : " + parseFloat(values.watt_hours).toFixed(2) + " Wh\n" +
                    "Wh Charge  : " + parseFloat(values.watt_hours_charged).toFixed(2) + " Wh\n" +
                    "ABS Tacho  : " + values.tachometer_abs + " Counts\n" +
                    "Fault      : " + values.fault_str
        }
    }
}
