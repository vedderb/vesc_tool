/*
    Copyright 2018 - 2021 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick 2.5
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.utility 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    id: rtData
    property var dialogParent: ApplicationWindow.overlay
    anchors.fill: parent

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property int odometerValue: 0
    property double efficiency_lpf: 0
    property bool isHorizontal: rtData.width > rtData.height

    property int gaugeSize: (isHorizontal ? Math.min((height - valMetrics.height * 4)/1.2 - 30, width / 2.6 - 20) :
    Math.min(width / 1.3, (height - valMetrics.height * 4) / 2.1 - 10))
    property int gaugeSize2: gaugeSize * 0.55

    Component.onCompleted: {
        mCommands.emitEmptySetupValues()
    }

    // Make background slightly darker
     RadialGradient {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#10ffffff" }
            GradientStop { position: 0.5; color: Utility.getAppHexColor("darkBackground") }
        }
    }

    GridLayout {
        anchors.fill: parent
        columns: isHorizontal ? 2 : 1
        columnSpacing: 0
        rowSpacing: 0
        //anchors.bottomMargin: gaugeSize2*2
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: gaugeSize2*1.1
            color: "transparent"
            CustomGauge {
                id: currentGauge
                width:gaugeSize2
                height:gaugeSize2
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: -0.675*gaugeSize2
                anchors.verticalCenterOffset: 0.1*gaugeSize2
                minimumValue: -60
                maximumValue: 60
                labelStep: maximumValue > 60 ? 20 : 10
                nibColor: Utility.getAppHexColor("tertiary1")
                unitText: "A"
                typeText: "Current"
                minAngle: -210
                maxAngle: 15
                CustomGauge {
                    id: dutyGauge
                    width: gaugeSize2
                    height: gaugeSize2
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: gaugeSize2*1.35
                    maximumValue: 100
                    minimumValue: -100
                    minAngle: 210
                    maxAngle: -15
                    isInverted: -1
                    labelStep: 25
                    value: 0
                    unitText: "%"
                    typeText: "Duty"
                    nibColor: Utility.getAppHexColor("tertiary3")
                    CustomGauge {
                        id: powerGauge
                        width: gaugeSize2*1.05
                        height: gaugeSize2*1.05
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: -0.675*gaugeSize2
                        anchors.verticalCenterOffset: -0.1*gaugeSize2
                        maximumValue: 10000
                        minimumValue: -10000
                        tickmarkScale: 0.001
                        tickmarkSuffix: "k"
                        labelStep: 1000
                        value: 1000
                        unitText: "W"
                        typeText: "Power"
                        nibColor: Utility.getAppHexColor("tertiary2")
                    }
                }
            }
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: gaugeSize
            color: "transparent"
            Layout.rowSpan: isHorizontal ? 2:1

            CustomGauge {
                id: speedGauge
                width:parent.height
                height:parent.height
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: (width/4 - gaugeSize2)/2
                //anchors.verticalCenterOffset: gaugeSize2*0.1
                minimumValue: 0
                maximumValue: 60
                minAngle: -225
                maxAngle: 45
                labelStep: maximumValue > 60 ? 20 : 10
                value: 20
                unitText: VescIf.useImperialUnits() ? "mph" : "km/h"
                typeText: "Speed"
                CustomGauge {
                    id: batteryGauge
                    width: gaugeSize2
                    height: gaugeSize2
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: parent.width/4 + width/2
                    minAngle: -225
                    maxAngle: 45
                    minimumValue: 0
                    maximumValue: 100
                    value: 95
                    unitText: "%"
                    typeText: "Battery"
                    centerTextVisible: false
                    nibColor: value > 50 ? "green" : (value > 20 ? Utility.getAppHexColor("orange") : Utility.getAppHexColor("red"))
                    Text {
                        id: batteryLabel
                        color: Utility.getAppHexColor("lightText")
                        text: "BATTERY"
                        font.pixelSize: gaugeSize2/18.0
                        verticalAlignment: Text.AlignVCenter
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: - gaugeSize2*0.175
                        anchors.margins: 10
                        font.family:  "Roboto mono"
                        Text {
                            id: rangeValLabel
                            color: Utility.getAppHexColor("lightText")
                            text: "20.0"
                            font.pixelSize: gaugeSize2/8.0
                            verticalAlignment: Text.AlignVCenter
                            anchors.centerIn: parent
                            anchors.verticalCenterOffset: gaugeSize2*0.115
                            //anchors.horizontalCenterOffset: (width -parent.width)/2
                            anchors.margins: 10
                            font.family:  "Roboto mono"
                            Text {
                                id: rangeLabel
                                color: Utility.getAppHexColor("lightText")
                                text: "KM RANGE"
                                font.pixelSize: gaugeSize2/20.0
                                verticalAlignment: Text.AlignVCenter
                                anchors.centerIn: parent
                                // anchors.verticalCenterOffset: //(-height +parent.height)/2
                                anchors.horizontalCenterOffset: gaugeSize2*0.3
                                anchors.margins: 10
                                font.family:  "Roboto mono"
                            }
                        }
                        Text {
                            id: battValLabel
                            color: Utility.getAppHexColor("lightText")
                            text: "50"
                            font.pixelSize: gaugeSize2/8.0
                            verticalAlignment: Text.AlignVCenter
                            anchors.centerIn: parent
                            anchors.verticalCenterOffset: gaugeSize2*0.235
                            //anchors.horizontalCenterOffset: (width -parent.width)/2
                            anchors.margins: 10
                            font.family:  "Roboto mono"
                            Text {
                                id: battLabel
                                color: Utility.getAppHexColor("lightText")
                                width:rangeLabel.width
                                //height: parent.height
                                text: "%"
                                font.pixelSize: gaugeSize2/15.0
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment:Text.AlignLeft
                                anchors.centerIn: parent
                                anchors.verticalCenterOffset: 0.1*height
                                anchors.horizontalCenterOffset: gaugeSize2*0.3
                                anchors.margins: 10
                                font.family:  "Roboto mono"
                            }
                        }
                    }

                    Rectangle {
                        id:clockRect
                        width:rideTime.implicitWidth + gaugeSize2*0.1
                        height: rideTime.implicitHeight + gaugeSize2*0.05
                        anchors.centerIn: parent
                        color: "transparent"
                        anchors.verticalCenterOffset: -gaugeSize2*0.72
                        anchors.horizontalCenterOffset: gaugeSize2*0.1
                        border.color: "#333333"
                        border.width: 2
                        radius: gaugeSize2*0.03


                        Text{
                            id: rideTime
                            color: "white"
                            text: "00:00:00"
                            font.pixelSize: gaugeSize2/10.0
                            verticalAlignment: Text.AlignVCenter
                            font.letterSpacing: gaugeSize2*0.001
                            anchors.centerIn: parent
                            anchors.margins: 10
                            font.family:  "Exan"
                        }
                        Glow{
                            anchors.fill: rideTime
                            radius: 4
                            samples: 9
                            color: "#55ffffff"
                            source: rideTime
                        }
                    }
                    Behavior on nibColor {
                        ColorAnimation {
                            duration: 1000;
                            easing.type: Easing.InOutSine
                            easing.overshoot: 3
                        }
                    }
                }
            }

            Rectangle {
                id: inclineCanvas
                property double incline:0
                anchors.centerIn: parent
                visible:true
                color: "transparent"
                anchors.horizontalCenterOffset: 3.45*gaugeSize/8
                anchors.verticalCenterOffset: gaugeSize2*0.75
                z:-1
                Rectangle {
                    id: incline1
                    height:2
                    width: 1.75*inclineText.height
                    radius:2
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: 1.25*width
                    color: Utility.getAppHexColor("disabledText")
                    transform: Rotation {
                        id:inclineTransform
                        origin.x: -1.25*incline1.width/2
                        origin.y: 0
                        angle: 180/3.14*Math.atan(inclineCanvas.incline/100.0)
                    }
                    Rectangle {
                        id: incline2
                        height:2
                        width: parent.width
                        radius:2
                        color: parent.color
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: -2.5*width
                    }
                }
                Text {
                    id: inclineText
                    anchors.centerIn: parent
                    color: "white"
                    text: parseFloat(inclineCanvas.incline).toFixed(0) + "%"
                    font.pixelSize: gaugeSize2/12.0
                    verticalAlignment: Text.AlignVCenter
                    //font.letterSpacing: gaugeSize2*0.001
                    anchors.margins: 10
                    font.family:  "Roboto mono"
                }
                width: gaugeSize2*0.7
                height: gaugeSize2*0.7
                Behavior on incline {
                    NumberAnimation {
                        easing.type: Easing.Linear
                        duration: 1000
                    }
                }
                Glow{
                    width: inclineCanvas.width
                    height: inclineCanvas.height
                    anchors.fill: inclineText
                    anchors.horizontalCenterOffset: 3.45*gaugeSize/8
                    anchors.verticalCenterOffset: gaugeSize2*0.75
                    radius: 3
                    samples: 7
                    color: "#55ffffff"
                    source: inclineText
                }
            }

        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: gaugeSize2*1.1
            color: "transparent"
            CustomGauge {
                id: escTempGauge
                width:gaugeSize2
                height:gaugeSize2
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: -0.675*gaugeSize2
                anchors.verticalCenterOffset: -0.1*gaugeSize2
                minimumValue: 0
                maximumValue:150
                labelStep: 15
                nibColor: Utility.getAppHexColor("tertiary2")
                unitText: "°C"
                typeText: "MAX \n ESC TEMP"
                minAngle: -195
                maxAngle: 30
                CustomGauge {
                    id: motTempGauge
                    width: gaugeSize2
                    height: gaugeSize2
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: gaugeSize2*1.35
                    maximumValue: 150
                    minimumValue: 0
                    minAngle: 195
                    maxAngle: -30
                    isInverted: -1
                    labelStep: 15
                    value: 0
                    unitText: "°C"
                    typeText: "MAX \n MOT TEMP"
                    nibColor: Utility.getAppHexColor("tertiary2")
                    CustomGauge {
                        id: efficiencyGauge
                        width: gaugeSize2*1.05
                        height: gaugeSize2*1.05
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: -0.675*gaugeSize2
                        anchors.verticalCenterOffset: 0.1*gaugeSize2
                        minimumValue: -50
                        maximumValue:  50
                        labelStep: maximumValue > 60 ? 20 : 10
                        value: 20
                        unitText: VescIf.useImperialUnits() ? "Wh/mi" : "Wh/km"
                        typeText: "Consump"
                        nibColor: value < 15 ? "green" : (value < 30 ? Utility.getAppHexColor("orange") : Utility.getAppHexColor("red"))
                        Behavior on nibColor {
                            ColorAnimation {
                                duration: 1000;
                                easing.type: Easing.InOutSine
                                easing.overshoot: 3
                            }
                        }
                        nibColor: Utility.getAppHexColor("tertiary2")
                    }
                }
            }
        }

        Rectangle {
            id: textRect
            color: Utility.getAppHexColor("darkBackground")
            Layout.fillWidth: true
            Layout.preferredHeight: 0
            Layout.alignment: Qt.AlignBottom
            Layout.columnSpan: isHorizontal ? 2 : 1
            visible: false
            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 2
                color: Utility.getAppHexColor("lightAccent")
            }

            Text {
                id: valText
                color: Utility.getAppHexColor("lightText")
                text: VescIf.getConnectedPortName()
                font.family: "DejaVu Sans Mono"
                verticalAlignment: Text.AlignVCenter
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 10
            }
            Text {
                id: valText2
                color: Utility.getAppHexColor("lightText")
                text: VescIf.getConnectedPortName()
                font.family: "DejaVu Sans Mono"
                verticalAlignment: Text.AlignVCenter
                anchors.left: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 10
            }

            TextMetrics {
                id: valMetrics
                font: valText.font
                text: valText.text
            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    var impFact = VescIf.useImperialUnits() ? 0.621371192 : 1.0
                    odometerBox.realValue = odometerValue*impFact/1000.0
                    tripDialog.open()
                }

                Dialog {
                    id: tripDialog
                    modal: true
                    focus: true
                    width: parent.width - 20
                    height: Math.min(implicitHeight, parent.height - 60)
                    closePolicy: Popup.CloseOnEscape

                    Overlay.modal: Rectangle {
                        color: "#AA000000"
                    }

                    x: 10
                    y: Math.max((parent.height - height) / 2, 10)
                    parent: dialogParent
                    standardButtons: Dialog.Ok | Dialog.Cancel
                    onAccepted: {
                        var impFact = VescIf.useImperialUnits() ? 0.621371192 : 1.0
                        mCommands.setOdometer(Math.round(odometerBox.realValue*1000/impFact))
                    }

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
                                    color: Utility.getAppHexColor("lightText")
                                    text: "Odometer"
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    Layout.fillWidth: true
                                    font.pointSize: 12
                                }

                                DoubleSpinBox {
                                    id: odometerBox
                                    decimals: 2
                                    realFrom: 0.0
                                    realTo: 20000000
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: mCommands

        onValuesImuReceived: {
            inclineCanvas.incline = Math.tan(values.pitch) * 100
            inclineCanvas.requestPaint()
        }

        onValuesSetupReceived: {
            currentGauge.maximumValue = Math.ceil(mMcConf.getParamDouble("l_current_max") / 5) * 5 * values.num_vescs
            currentGauge.minimumValue = -currentGauge.maximumValue
            currentGauge.labelStep = Math.ceil(currentGauge.maximumValue / 20) * 5


            currentGauge.value = values.current_motor
            dutyGauge.value = values.duty_now * 100.0
            batteryGauge.value = values.battery_level * 100.0

            var useImperial = VescIf.useImperialUnits()
            var fl = mMcConf.getParamDouble("foc_motor_flux_linkage")
            var rpmMax = (values.v_in * 60.0) / (Math.sqrt(3.0) * 2.0 * Math.PI * fl)
            var speedFact = ((mMcConf.getParamInt("si_motor_poles") / 2.0) * 60.0 *
            mMcConf.getParamDouble("si_gear_ratio")) /
            (mMcConf.getParamDouble("si_wheel_diameter") * Math.PI)
            var speedMax = 3.6 * rpmMax / speedFact
            var impFact = useImperial ? 0.621371192 : 1.0
            var speedMaxRound = (Math.ceil((speedMax * impFact) / 10.0) * 10.0)

            var dist = values.tachometer_abs / 1000.0
            var wh_consume = values.watt_hours - values.watt_hours_charged
            var wh_km_total = wh_consume / dist

            if (Math.abs(speedGauge.maximumValue - speedMaxRound) > 6.0) {
                speedGauge.maximumValue = speedMaxRound
                speedGauge.minimumValue = -speedMaxRound
            }

            speedGauge.value = values.speed * 3.6 * impFact
            speedGauge.unitText = useImperial ? "mph" : "km/h"

            var powerMax = Math.min(values.v_in * Math.min(mMcConf.getParamDouble("l_in_current_max"),
            mMcConf.getParamDouble("l_current_max")),
            mMcConf.getParamDouble("l_watt_max")) * values.num_vescs
            var powerMaxRound = (Math.ceil(powerMax / 1000.0) * 1000.0)

            if (Math.abs(powerGauge.maximumValue - powerMaxRound) > 1.2) {
                powerGauge.maximumValue = powerMaxRound
                powerGauge.minimumValue = -powerMaxRound
            }

            powerGauge.value = (values.current_in * values.v_in)

            var alpha = 0.05
            var efficiencyNow = Math.max( Math.min(values.current_in * values.v_in/Math.max(values.speed * 3.6 * impFact, 1e-6) , 60) , -60)
            efficiency_lpf = (1.0 - alpha) * efficiency_lpf + alpha *  efficiencyNow
            efficiencyGauge.value = efficiency_lpf

            valText.text =
            "Temp MOS : " + parseFloat(values.temp_mos).toFixed(2) + " \u00B0C\n" +
            "Temp Mot : " + parseFloat(values.temp_motor).toFixed(2) + " \u00B0C\n" +
            "mAh Out  : " + parseFloat(values.amp_hours * 1000.0).toFixed(1) + "\n" +
            "mAh In   : " + parseFloat(values.amp_hours_charged * 1000.0).toFixed(1)

            odometerValue = values.odometer
            batteryGauge.unitText = parseFloat(wh_km_total / impFact).toFixed(1) +
            useImperial ? " mi range " : "km range: "

            var l1Txt = useImperial ? "mi Trip : " : "km Trip : "
            var l2Txt = useImperial ? "mi ODO  : " : "km ODO  : "
            var l3Txt = useImperial ? "Wh/mi   : " : "Wh/km   : "
            var l4Txt = useImperial ? "mi Range: " : "km Range: "

            valText2.text =
            l1Txt + parseFloat((values.tachometer_abs * impFact) / 1000.0).toFixed(3) + "\n" +
            l2Txt + parseFloat((values.odometer * impFact) / 1000.0).toFixed(odometerBox.decimals) + "\n" +
            l3Txt + parseFloat(wh_km_total / impFact).toFixed(1) + "\n" +
            l4Txt + parseFloat(values.battery_wh / (wh_km_total / impFact)).toFixed(2)
        }
    }
}
