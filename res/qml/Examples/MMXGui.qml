import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    id: topComponent
    
    anchors.fill: parent
    anchors.margins: 10
    
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mAppConf: VescIf.appConfig()
    property bool isHorizontal: width > height
    
    property bool motor_running: false
    
    property var bpm_tab: [
        0,
        50,
        100,
        110,
        120,
        160,
        200,
        220
    ]
    
    property var bpm_ind: 0
    property var motor_poles: 14
    
    function bpm_to_erpm(bpm) {
        var motor_poles = 14
        var gear_ratio = 1 / 7
        var bpm_per_crank_rpm = 1 / 2
        return bpm * (motor_poles / 2) * (1 / gear_ratio) * bpm_per_crank_rpm
    }
    
    function erpm_to_bpm(erpm) {
        return erpm / bpm_to_erpm(1)
    }
    
    Component.onCompleted: {
        bpmSelGauge.value = bpm_tab[bpm_ind]
        bpmSetGauge.value = bpm_tab[bpm_ind]
    }
    
    ColumnLayout {
        anchors.fill: parent
        id: gaugeColumn
        
        Text {
            Layout.fillWidth: true
            color: "White"
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 20
            text: "MMX VESC UI"
        }
        
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            columns: isHorizontal ? 1 : 2
            
            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                
                columns: isHorizontal ? 3 : 1
                
                property var gaugeSize: (isHorizontal ? Math.min(gaugeColumn.width / 3.5, gaugeColumn.height / 1.7) :
                                        Math.min(gaugeColumn.height / 3.5, gaugeColumn.width / 2))
                
                CustomGaugeV2 {
                    id: bpmSelGauge
                    Layout.fillWidth: false
                    Layout.preferredWidth: parent.gaugeSize
                    Layout.preferredHeight: parent.gaugeSize
                    maximumValue: 220
                    minimumValue: 0
                    tickmarkScale: 1
                    tickmarkSuffix: ""
                    labelStep: 20
                    value: 0
                    unitText: ""
                    typeText: "BPM\nSELECTED"
                }
                
                CustomGaugeV2 {
                    id: bpmSetGauge
                    Layout.fillWidth: false
                    Layout.preferredWidth: parent.gaugeSize
                    Layout.preferredHeight: parent.gaugeSize
                    maximumValue: 220
                    minimumValue: 0
                    tickmarkScale: 1
                    tickmarkSuffix: ""
                    labelStep: 20
                    value: 0
                    unitText: ""
                    typeText: "BPM\nSET"
                }
                
                CustomGaugeV2 {
                    id: bpmNowGauge
                    Layout.fillWidth: false
                    Layout.preferredWidth: parent.gaugeSize
                    Layout.preferredHeight: parent.gaugeSize
                    maximumValue: 220
                    minimumValue: 0
                    tickmarkScale: 1
                    tickmarkSuffix: ""
                    labelStep: 20
                    value: 0
                    unitText: ""
                    typeText: "BPM\nNOW"
                }
            }
            
            GridLayout {
                id: buttonRow
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter
                layoutDirection: Qt.RightToLeft
                
                rowSpacing: 10
                columnSpacing: 10
                columns: isHorizontal ? 4 : 1
                
                property var buttonHeight: isHorizontal ? (topComponent.height / 4) : (topComponent.height / 5)
                
                ImageButton {
                    buttonText: "Set\nBPM"
                    imageSrc: "qrc:/res/icons/Ok-96.png"
                    Layout.preferredHeight: buttonRow.buttonHeight
                    Layout.fillWidth: true
                    
                    onClicked: {
                        bpmSetGauge.value = bpmSelGauge.value
                    }
                }
                
                ImageButton {
                    buttonText: "Next\nBPM"
                    imageSrc: "qrc:/res/icons/Up-96.png"
                    Layout.preferredHeight: buttonRow.buttonHeight
                    Layout.fillWidth: true
                    
                    onClicked: {
                        if (bpm_ind < (bpm_tab.length - 1)) {
                            bpm_ind++
                        }
                        
                        bpmSelGauge.value = bpm_tab[bpm_ind]
                    }
                }
                
                ImageButton {
                    buttonText: "Prev\nBPM"
                    imageSrc: "qrc:/res/icons/Down-96.png"
                    Layout.preferredHeight: buttonRow.buttonHeight
                    Layout.fillWidth: true
                    
                    onClicked: {
                        if (bpm_ind > 0) {
                            bpm_ind--
                        }
                        
                        bpmSelGauge.value = bpm_tab[bpm_ind]
                    }
                }
                
                ImageButton {
                    buttonText: "Start\nMotor"
                    imageSrc: "qrc:/res/icons/Restart-96.png"
                    Layout.preferredHeight: buttonRow.buttonHeight
                    Layout.fillWidth: true
                    
                    onClicked: {
                        motor_running = !motor_running
                        
                        if (motor_running) {
                            buttonText = "Stop\nMotor"
                            imageSrc = "qrc:/res/icons/Shutdown-96.png"
                        } else {
                            buttonText = "Start\nMotor"
                            imageSrc = "qrc:/res/icons/Restart-96.png"
                        }
                    }
                }
            }
        }
    }
    
    Timer {
        running: true
        repeat: true
        interval: 50
                
        onTriggered: {
            mCommands.getValues()
            
            var erpm = bpm_to_erpm(bpmSetGauge.value)
            
            if (motor_running && Math.abs(erpm) > 200) {
                mCommands.setRpm(erpm)
            } else {
                mCommands.setCurrent(0)
            }
        }
    }
    
    Connections {
        target: mCommands
        
        function onValuesReceived(values, mask) {
            bpmNowGauge.value = erpm_to_bpm(Math.abs(values.rpm))
        }
    }
}
