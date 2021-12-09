import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0

import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    id: mainItem
    anchors.fill: parent
    anchors.margins: 5

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()

    ColumnLayout {
        id: gaugeColumn
        anchors.fill: parent
        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            CustomGauge {
                id: adc1Gauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: width
                maximumValue: 120
                minimumValue: -120
                tickmarkScale: 1
                labelStep: 20
                value: slider1.value
                unitText: "VAL"
                typeText: "SLIDER 1"
            }

            CustomGauge {
                id: adc2Gauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: width
                maximumValue: 12
                minimumValue: -12
                maxAngle: -230
                minAngle: 50
                tickmarkScale: 1
                labelStep:2
                value: slider2.value/10.0
                precision: 1
                unitText: "VAL"
                typeText: "SLIDER 2"
                property color posColor:  Utility.getAppHexColor("tertiary3")
                property color negColor:  Utility.getAppHexColor("tertiary1")
                nibColor: value >= 0 ? posColor: negColor
            }
        }
        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Slider {
                id:slider1
                Layout.fillWidth: true
                from: -120
                to: 120
                value: 0
            }
            Slider {
                id:slider2
                Layout.fillWidth: true
                from: -200
                to: 200
                value: 0
            }
        }
        RowLayout {
            Button{
                Layout.fillWidth: true
                text: "change lims"
                onClicked: {
                    if(adc2Gauge.maximumValue === 12) {
                        adc2Gauge.labelStep = 3
                        adc2Gauge.maximumValue = 21
                        adc2Gauge.minimumValue = -18
                    } else {
                        adc2Gauge.labelStep = 3
                        adc2Gauge.maximumValue = 12
                        adc2Gauge.minimumValue = -12
                    }
                }
            }
            Button{
                Layout.fillWidth: true
                text: "change angle"
                onClicked: {
                    if(adc2Gauge.maxAngle === -230) {
                        adc2Gauge.maxAngle = 230
                        adc2Gauge.minAngle = -50
                    } else {
                        adc2Gauge.maxAngle = -230
                        adc2Gauge.minAngle = 50
                    }

                }
            }
        }
    }
}
