import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0
import Vedder.vesc.commands 1.0

Item {
    id: mainItem
    anchors.fill: parent
    anchors.margins: 5
    
    property Commands mCommands: VescIf.commands()
    property bool isRunning: false
    
    ColumnLayout {
        anchors.fill: parent
        
        GroupBox {
            title: qsTr("Driving Motor (Local)")
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                
                DoubleSpinBox {
                    id: rpmBox
                    Layout.fillWidth: true
                    decimals: 1
                    prefix: "ERPM: "
                    suffix: ""
                    realFrom: 0.0
                    realTo: 50000.0
                    realValue: 15000.0
                    realStepSize: 1000.0
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        Layout.fillWidth: true
                        text: "Start"
                        
                        onClicked: {
                            mCommands.setRpm(rpmBox.realValue)
                            isRunning = true
                        }
                    }
                    
                    Button {
                        Layout.fillWidth: true
                        text: "Stop"
                        
                        onClicked: {
                            // Disable brake first so that we don't feed anything back to the supply
                            VescIf.canTmpOverride(true, canBox.realValue)
                            mCommands.setCurrentBrake(0)
                            VescIf.canTmpOverrideEnd()
                            
                            mCommands.setCurrent(0)
                            
                            isRunning = false
                        }
                    }
                }
            }
        }
        
        GroupBox {
            title: qsTr("Braking Motor (CAN)")
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                
                DoubleSpinBox {
                    id: canBox
                    Layout.fillWidth: true
                    decimals: 0
                    prefix: "CAN ID: "
                    suffix: ""
                    realFrom: 0
                    realTo: 254
                    realValue: 1
                    realStepSize: 1
                }
                
                DoubleSpinBox {
                    id: currentBox
                    Layout.fillWidth: true
                    decimals: 1
                    prefix: "Current: "
                    suffix: " A"
                    realFrom: 0.0
                    realTo: 500.0
                    realValue: 25.0
                    realStepSize: 1.0
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        Layout.fillWidth: true
                        text: "Brake ON"
                        
                        onClicked: {
                            VescIf.canTmpOverride(true, canBox.realValue)
                            mCommands.setCurrentBrake(currentBox.realValue)
                            VescIf.canTmpOverrideEnd()
                            isRunning = true
                        }
                    }
                    
                    Button {
                        Layout.fillWidth: true
                        text: "Brake Off"
                        
                        onClicked: {
                            VescIf.canTmpOverride(true, canBox.realValue)
                            mCommands.setCurrentBrake(0)
                            VescIf.canTmpOverrideEnd()
                        }
                    }
                }
            }
        }
        
        Text {
            id: valText
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "white"
            font.family: "DejaVu Sans Mono"
        }
    }
    
    Timer {
        id: aliveTimer
        interval: 100
        running: true
        repeat: true
        
        property bool other: false
        
        onTriggered: {
            if (VescIf.isPortConnected()) {
                if (other) {
                    VescIf.canTmpOverride(true, canBox.realValue)
                    if (isRunning) {
                        mCommands.sendAlive()
                    }
                    mCommands.getValues()
                    VescIf.canTmpOverrideEnd()
                } else {
                    if (isRunning) {
                        mCommands.sendAlive()
                    }
                    mCommands.getValues()
                }
                
                other = !other
            }
        }
    }
    
    Connections {
        target: mCommands
        
        property var valOther: []
        
        onValuesReceived: { // values, mask
            var friction = 10.0
                    
            if (values.vesc_id == canBox.realValue) {
                valOther = values
            } else {
                var powerDrive = values.v_in * values.current_in
                var powerBrake = valOther.v_in * valOther.current_in
                var powerFriction = friction * values.vq
                var losses = powerDrive + powerBrake - powerFriction
                var efficiency = 1.0 - (losses / 2) / powerDrive
                
                valText.text = ""
                valText.text += "Power Drive    : " + parseFloat(powerDrive).toFixed(2) + " W\n"
                valText.text += "Power Friction : " + parseFloat(powerFriction).toFixed(2) + " W\n"
                valText.text += "Power Brake    : " + parseFloat(powerBrake).toFixed(2) + " W\n"
                valText.text += "Efficiency     : " + parseFloat(efficiency * 100.0).toFixed(2) + " %\n"
                valText.text += "T Drive        : " + parseFloat(values.temp_mos).toFixed(2) + "\n"
                valText.text += "T Brake        : " + parseFloat(valOther.temp_mos).toFixed(2) + "\n"
            }
        }
    }
}
