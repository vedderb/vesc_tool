import QtQuick 2.12
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
        GridLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            
            columns: 2
            
            CustomGauge {
                id: adc1Gauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.width * 0.45
                maximumValue: 120
                minimumValue: 0
                tickmarkScale: 1
                labelStep: 10
                precision: 1
                value: 0
                unitText: "V"
                typeText: "V In"
            }
            
            CustomGauge {
                id: adc2Gauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.width * 0.45
                maximumValue: 30
                minimumValue: 0
                tickmarkScale: 1
                labelStep: 5
                precision: 1
                value: 0
                unitText: "V"
                typeText: "V Out"
            }
            
            CustomGauge {
                id: adc3Gauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.width * 0.45
                maximumValue: 120
                minimumValue: -20
                tickmarkScale: 1
                labelStep: 10
                precision: 1
                value: 0
                unitText: "degC"
                typeText: "Temp"
            }
            
            CustomGauge {
                id: adc4Gauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.width * 0.45
                maximumValue: 20
                minimumValue: 0
                tickmarkScale: 1
                labelStep: 10
                precision: 1
                value: 0
                unitText: "A"
                typeText: "Current"
            }
        }
        
        Slider {
            Layout.fillWidth: true
            
            from: 0
            to: 150
            value: 0
            
            onValueChanged: {
                mCommands.ioBoardSetPwm(255, 0, value / 1000)
            }
        }
    }
    
    Timer {
        running: true
        repeat: true
        interval: 100
        
        onTriggered: {
            mCommands.ioBoardGetAll(104)
        }
    }
    
    Connections {
        target: mCommands
        
        function onIoBoardValRx(val) {
            adc1Gauge.value = val.adc_1_4[0]
            adc2Gauge.value = val.adc_1_4[1]
            adc3Gauge.value = val.adc_1_4[2]
            adc4Gauge.value = val.adc_1_4[3]
        }
    }
}
