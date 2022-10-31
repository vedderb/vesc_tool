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
            CustomGaugeV2 {
                id: adc1Gauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 120
                minimumValue: 0
                tickmarkScale: 1
                labelStep: 10
                value: 0
                unitText: "V"
                typeText: "ADC 1"
            }
            
            CustomGaugeV2 {
                id: adc2Gauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 120
                minimumValue: 0
                tickmarkScale: 1
                labelStep: 10
                value: 0
                unitText: "V"
                typeText: "ADC 2"
            }
        }
        
        Repeater {
            id: rep
            model: [0, 1, 2, 3, 4]
            
            Slider {
                Layout.fillWidth: true
                
                from: 0
                to: 1000
                value: 0
                
                onValueChanged: {
                    mCommands.ioBoardSetPwm(255, modelData, value / 1000)
                }
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
            // Members of val
//            val.id
//            val.adc_1_4[ch]
//            val.adc_5_8[ch]
//            val.digital[ch]
//            val.adc_1_4_age
//            val.adc_5_8_age
//            val.digital_age
            
            adc1Gauge.value = val.adc_1_4[0]
            adc2Gauge.value = val.adc_1_4[1]
        }
    }
}
