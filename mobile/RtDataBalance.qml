
import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: rtData
    property Commands mCommands: VescIf.commands()
    property ConfigParams mAppConf: VescIf.appConfig()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property bool isHorizontal: rtData.width > rtData.height

    property int largeGaugeSize: isHorizontal ? Math.min((root.width - 25) / 4, ((root.height - 5) - (textRect.height))) :
                                           Math.min((root.width-5) / 2, ((root.height - 15) - (textRect.height)) * (3/7))


    property int smallGaugeSize: isHorizontal ? Math.min((root.width - 25) / 6, ((root.height - 10) - (textRect.height)) / 2) :
                                            Math.min((root.width - 10) / 3, ((root.height - 15) - (textRect.height)) * (2/7))

    GridLayout {
        id: root
        anchors.fill: parent
        anchors.margins: 5
        columns: isHorizontal ? 2 : 1

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            columns: 3
            columnSpacing: 5
            rowSpacing: 5

            CustomGauge {
                id: pitchGauge
                Layout.preferredWidth: smallGaugeSize
                Layout.preferredHeight: smallGaugeSize
                maximumValue: 75
                minimumValue: -75
                minAngle: -150
                maxAngle: 150
                tickmarkSuffix: "°"
                labelStep: 15
                value: 0
                unitText: "Degrees"
                typeText: "Pitch"
                precision: 1
            }
            CustomGauge {
                id: rollGauge
                Layout.preferredWidth: smallGaugeSize
                Layout.preferredHeight: smallGaugeSize
                maximumValue: 75
                minimumValue: -75
                minAngle: -150
                maxAngle: 150
                tickmarkSuffix: "°"
                labelStep: 15
                value: 0
                unitText: "Degrees"
                typeText: "Roll"
                precision: 1
            }
            CustomGauge {
                id: erpmGauge
                Layout.preferredWidth: smallGaugeSize
                Layout.preferredHeight: smallGaugeSize
                maximumValue: 100
                minimumValue: -100
                labelStep: 20
                value: 0
                unitText: "x100"
                typeText: "ERPM"
            }
            CustomGauge {
                id: motorCurrentGauge
                Layout.preferredWidth: smallGaugeSize
                Layout.preferredHeight: smallGaugeSize
                maximumValue: 100
                minimumValue: -100
                labelStep: maximumValue > 60 ? 20 : 10
                value: 0
                unitText: "A"
                typeText: "Current"
            }
            CustomGauge {
                id: batteryCurrentGauge
                Layout.preferredWidth: smallGaugeSize
                Layout.preferredHeight: smallGaugeSize
                maximumValue: 100
                minimumValue: -100
                labelStep: maximumValue > 60 ? 20 : 10
                value: 0
                unitText: "A"
                typeText: "In Current"
            }
            CustomGauge {
                id: voltageGauge
                Layout.preferredWidth: smallGaugeSize
                Layout.preferredHeight: smallGaugeSize
                maximumValue: 200
                minimumValue: 0
                precision: 1
                labelStep: Math.round((maximumValue - minimumValue) / 10)
                value: 0
                unitText: "V"
                typeText: "Voltage"
            }

        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 5
            CustomGauge {
                id: speedGauge
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: largeGaugeSize
                Layout.preferredHeight: largeGaugeSize
                minimumValue: 0
                maximumValue: 60
                minAngle: -250
                maxAngle: 70
                labelStep: maximumValue > 60 ? 20 : 10
                value: 20
                unitText: VescIf.useImperialUnits() ? "mph" : "km/h"
                typeText: "Speed"
                precision: 1
            }
            CustomGauge {
                id: dutyGauge
                Layout.preferredWidth: largeGaugeSize
                Layout.preferredHeight: largeGaugeSize
                maximumValue: 100
                minimumValue: -100
                labelStep: 20
                value: 0
                unitText: "%"
                typeText: "Duty"
            }
        }
        Rectangle {
            id: textRect
            color: Utility.getAppHexColor("darkBackground")
            Layout.columnSpan: isHorizontal? 2 : 1
            Layout.fillWidth: true
            Layout.minimumHeight: valMetrics.height * 6 + 12
            Layout.preferredHeight: valMetrics.height * 6 + 12
            Layout.alignment: Qt.AlignBottom

            Rectangle {
                width: parent.width
                height: 2
                color: Utility.getAppHexColor("lightAccent")
            }

            Column{
                anchors.fill: parent
                anchors.topMargin: 7
                Layout.alignment: Qt.AlignVCenter
                spacing: 0

                Text {
                    id: valText
                    color: Utility.getAppHexColor("lightText")
                    text: VescIf.getConnectedPortName()
                    font.family: "DejaVu Sans Mono"
                    Layout.margins: 0
                    anchors.margins: 0
                }

                Text {
                    id: valText2
                    color: Utility.getAppHexColor("lightText")
                    text: ""
                    font.family: "DejaVu Sans Mono"
                    Layout.margins: 0
                    anchors.margins: 0
                }
            }

            TextMetrics {
                id: valMetrics
                font: valText.font
                text: valText.text
            }
        }
    }

    Connections {
        target: mCommands

        function onDecodedBalanceReceived(values) {
            pitchGauge.value = values.pitch_angle
            rollGauge.value = values.roll_angle

            var appState
            if(values.state === 0){
                appState = "Calibrating";
            }else if(values.state === 1){
                appState = "Running";
            }else if(values.state === 2){
                appState = "Running (Tiltback Duty)";
            }else if(values.state === 3){
                appState = "Running (Tiltback High Voltage)";
            }else if(values.state === 4){
                appState = "Running (Tiltback Low Voltage)";
            }else if(values.state === 5){
                appState = "Running (Tiltback Constant)";
            }else if(values.state === 6){
                appState = "Fault (Pitch Angle)";
            }else if(values.state === 7){
                appState = "Fault (Roll Angle)";
            }else if(values.state === 8){
                appState = "Fault (Switch Half)";
            }else if(values.state === 9){
                appState = "Fault (Switch FULL)";
            }else if(values.state === 10){
                appState = "Fault (Duty)";
            }else if(values.state === 11){
                appState = "Initial";
            }else{
                appState = "Unknown";
            }

            var switchState
            if(values.switch_value === 0){
                switchState = "Off";
            }else if(values.switch_value === 1){
                switchState = "Half";
            }else if(values.switch_value === 2){
                switchState = "On";
            }else{
                switchState = "Unknown";
            }

            valText.text =
                    "Balance State: " + appState + "\n" +
                    "ADC1         : " + values.adc1.toFixed(2) + " V\n" +
                    "ADC2         : " + values.adc2.toFixed(2) + " V\n" +
                    "Switch State : " + switchState + "\n" +
                    "Loop Time    : " + values.diff_time + " uS"
        }

        onValuesSetupReceived: {
            motorCurrentGauge.maximumValue = Math.ceil(mMcConf.getParamDouble("l_current_max") / 5) * 5 * values.num_vescs
            motorCurrentGauge.minimumValue = -motorCurrentGauge.maximumValue
            motorCurrentGauge.value = values.current_motor

            batteryCurrentGauge.maximumValue = Math.ceil(mMcConf.getParamDouble("l_in_current_max") / 5) * 5 * values.num_vescs
            batteryCurrentGauge.minimumValue = -batteryCurrentGauge.maximumValue
            batteryCurrentGauge.value = values.current_in

            dutyGauge.value = values.duty_now * 100.0

            erpmGauge.value = values.rpm / 100

            voltageGauge.maximumValue = Math.ceil(mAppConf.getParamDouble("app_balance_conf.tiltback_hv"))
            voltageGauge.minimumValue = Math.floor(mAppConf.getParamDouble("app_balance_conf.tiltback_lv"))
            voltageGauge.value = values.v_in

            var useImperial = VescIf.useImperialUnits()
            var fl = mMcConf.getParamDouble("foc_motor_flux_linkage")
            var rpmMax = (values.v_in * 60.0) / (Math.sqrt(3.0) * 2.0 * Math.PI * fl)
            var speedFact = ((mMcConf.getParamInt("si_motor_poles") / 2.0) * 60.0 *
                    mMcConf.getParamDouble("si_gear_ratio")) /
                    (mMcConf.getParamDouble("si_wheel_diameter") * Math.PI)
            var speedMax = 3.6 * rpmMax / speedFact
            var impFact = useImperial ? 0.621371192 : 1.0
            var speedMaxRound = (Math.ceil((speedMax * impFact) / 10.0) * 10.0)

            if (Math.abs(speedGauge.maximumValue - speedMaxRound) > 6.0) {
                speedGauge.maximumValue = speedMaxRound
                speedGauge.minimumValue = -speedMaxRound
            }

            speedGauge.value = values.speed * 3.6 * impFact
            speedGauge.unitText = useImperial ? "mph" : "km/h"

            valText2.text =
                    "Fault        : " + values.fault_str
        }
    }
}
