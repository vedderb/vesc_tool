import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import Vedder.vesc.utility 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    id: topItem
    anchors.fill: parent
    anchors.margins: 10
    
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mAppConf: VescIf.appConfig()
    
    property var vecRoll: []
    property var vecPitch: []
    property var vecYaw: []
    
    ColumnLayout {
        anchors.fill: parent
        
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            ParamList {
                id: params
                anchors.fill: parent
                
                Component.onCompleted: {
                    addEditorApp("imu_conf.mode")
                    addEditorApp("imu_conf.rot_roll")
                    addEditorApp("imu_conf.rot_pitch")
                    addEditorApp("imu_conf.rot_yaw")
                    
                    // Editors for custom config, e.g. the balance app
//                    addEditorCustom("kp", 0)
//                    addEditorCustom("ki", 0)
//                    addEditorCustom("kd", 0)
                }
            }
        }
        
        Canvas {
            id: plot
            
            Layout.fillWidth: true
            Layout.preferredHeight: topItem.height * 0.3
            
            function drawVec(ctx, vec, yMax) {
                ctx.beginPath()
                var mid = height / 2
                
                if (vec.length > 0) {
                    ctx.moveTo(0, mid - vec[0] / yMax * mid)
                }
                
                for (var i = 0;i < vec.length;i++) {
                    ctx.lineTo((i + 1) * width / vec.length, mid - vec[i] / yMax * mid)
                }
                ctx.stroke()
            }
            
            onPaint: {
                var ctx = getContext("2d");
                
                ctx.fillStyle = Qt.rgba(0.1, 0.1, 0.1, 1)
                ctx.fillRect(0, 0, width, height)
                
                // Draw x-axis
                ctx.lineWidth = 2;
                ctx.strokeStyle = Qt.rgba(1, 1, 1, 1)
                ctx.beginPath()
                ctx.moveTo(0, height / 2)
                ctx.lineTo(width, height / 2)
                ctx.stroke()
                
                // Draw IMU-vectors
                ctx.lineWidth = 1.5;
                ctx.strokeStyle = Qt.rgba(1, 0, 0, 1)
                drawVec(ctx, vecRoll, Math.PI)
                ctx.strokeStyle = Qt.rgba(0, 1, 0, 1)
                drawVec(ctx, vecPitch, Math.PI)
                ctx.strokeStyle = Qt.rgba(0, 0, 1, 1)
                drawVec(ctx, vecYaw, Math.PI)
            }
        }
        
        Timer {
            running: true
            repeat: true
            interval: 100
            
            onTriggered: {
                mCommands.getImuData(0xFFFFFFFF)
            }
        }
        
        Connections {
            target: mCommands
            
            function addToVec(vec, sample) {
                var maxLen = 100
                
                vec.push(sample)
                if (vec.length > maxLen) {
                    vec.shift()
                }
            }
            
            function onValuesImuReceived(val, mask) {
                addToVec(vecRoll, val.roll)
                addToVec(vecPitch, val.pitch)
                addToVec(vecYaw, val.yaw)
                
                plot.requestPaint()
            }
        }
    }
}
