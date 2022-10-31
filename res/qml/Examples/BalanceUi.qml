// Remote control for balance robot

import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: container
    anchors.fill: parent
    anchors.margins: 10
    
    property Commands mCommands: VescIf.commands()
    property var btnWidth: container.width / 3
    property var btnHeight: 70
    property var mx: 0
    property var my: 0
    
    Component.onCompleted: {
        
    }
    
    ColumnLayout {
        anchors.fill: parent
        
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: btnHeight * 3
            
            Button {
                id: btnFwd
                width: btnWidth
                height: btnHeight
                text: "Forward"
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Button {
                id: btnLeft
                width: btnWidth
                height: btnHeight
                text: "Left"
                anchors.top: btnFwd.bottom
                anchors.right: btnFwd.left
            }
            
            Button {
                id: btnRight
                width: btnWidth
                height: btnHeight
                text: "Right"
                anchors.top: btnFwd.bottom
                anchors.left: btnFwd.right
            }
            
            Button {
                id: btnRev
                width: btnWidth
                height: btnHeight
                text: "Reverse"
                anchors.top: btnLeft.bottom
                anchors.left: btnLeft.right
            }
        }
        
        RowLayout {
            Layout.fillWidth: true
            
            CheckBox {
                id: posBox
                Layout.fillWidth: true
                text: "PosControl"
                checked: true
            }
            
            CheckBox {
                id: yawBox
                Layout.fillWidth: true
                text: "YawControl"
                checked: true
            }
        }
        
        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "red"
            
            MouseArea {
                anchors.fill: parent
                id: mouseArea
                preventStealing: true
                
                onPositionChanged: {
                    if (pressed) {
                        var x = 2 * (mouseX / width - 0.5)
                        var y = -2 * (mouseY / height - 0.5)
                        if (x < -1) {x = -1}
                        if (x > 1) {x = 1}
                        if (y < -1) {y = -1}
                        if (y > 1) {y = 1}
                        mx = x
                        my = y
                    } else {
                        mx = 0
                        my = 0
                    }
                }
            }
        }
    }
    
    Timer {
        running: true
        repeat: true
        interval: 100
        
        onTriggered: {
            var step = 30
            
            if (!mouseArea.pressed) {
                mx = 0
                my = 0
            }
            
            if (my < 0) {
                mx = -mx
            }
            
            var buffer = new ArrayBuffer(6);
            var dv = new DataView(buffer);
            dv.setUint8(0, (btnFwd.pressed ? step : 0) + 2 * step * (my > 0 ? my : 0))
            dv.setUint8(1, (btnRev.pressed ? step : 0) + 2 * step * (my < 0 ? -my : 0))
            dv.setUint8(2, (btnLeft.pressed ? step : 0) + 2 * step * (mx < 0 ? -mx : 0))
            dv.setUint8(3, (btnRight.pressed ? step : 0) + 2 * step * (mx > 0 ? mx : 0))
            dv.setUint8(4, posBox.checked ? 1 : 0)
            dv.setUint8(5, yawBox.checked ? 1 : 0)
            mCommands.sendCustomAppData(buffer)
        }
    }
    
    Connections {
        target: mCommands
        
        function onCustomAppDataReceived(data) {
            
        }
    }
}
