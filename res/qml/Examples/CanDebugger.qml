// This is an example of using the VESC as a CAN-debugger. It demonstrates:
// * Changing the app configuration to forward CAN-frames
// * Receiving and decoding CAN-frames
// * Encoding and transmitting CAN-frames
// * Presenting the data in Text-elements

import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0

import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import "qrc:/mobile"

Item {
    anchors.fill: parent
    anchors.margins: 10
    
    property Commands mCommands: VescIf.commands()
    property ConfigParams mAppConf: VescIf.appConfig()
    
    property var cellv: []
    property var cellt: []
    property double relh: 0
    property double chargeh: 0
    property double chargeCurrent: 0
    property double dischargeCurrent: 0
    property var line: ""
    
    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 5
        
        RowLayout {
            Layout.fillWidth: true
            
            Button {
                Layout.fillWidth: true
                
                text: "CAN Bridge Enable"
                onClicked: {
                    mAppConf.updateParamEnum("can_mode", 2, this)
                    mCommands.setAppConf()
                }
            }
            
            Button {
                Layout.fillWidth: true
                
                text: "CAN Bridge Disable"
                onClicked: {
                    mAppConf.updateParamEnum("can_mode", 0, this)
                    mCommands.setAppConf()
                }
            }
        }
        
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                id: mText
                color: "white"
                font.family: "DejaVu Sans Mono"
                
                text: "Test Text"
            }
            
            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                id: mText2
                color: "white"
                font.family: "DejaVu Sans Mono"
                
                text: "Test Text"
            }
        }
        
        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            id: mText3
            color: "white"
            font.family: "DejaVu Sans Mono"
            
            text: "Test Text"
        }
    }
    
    Timer {
        running: true
        repeat: true
        interval: 50
        
        property var vals: [
                                [2, 0],
                                [2, 1],
                                [2, 2],
                                [2, 3],
                                [2, 4],
                                [3, 0],
                                [3, 1],
                                [5, 1],
                                [5, 2],
                                [6, 1],
                                [6, 2],
                                [9, 0],
                                [9, 1],
                                [9, 2],
                                [9, 3],
                               ]
                                
        property int ind: 0
        
        onTriggered: {
            var buffer = new ArrayBuffer(2);
            var dv = new DataView(buffer);
            
            dv.setUint8(0, vals[ind][0])
            dv.setUint8(1, vals[ind][1])
            mCommands.forwardCanFrame(buffer, 0x640, false)
            
            ind++
            if (ind == vals.length) {
                ind = 0
            }
        }
    }
    
    Connections {
        target: mCommands
        
        function onCanFrameRx(data, id, isExtended) {
            var dv = new DataView(data);
            
            var param = dv.getUInt8(0)
            var sub = dv.getUInt8(1)
            
            if (!isExtended && id === 0x660) {
                if (param === 2) {
                    for (var i = 0;i < 3;i++) {
                        var volt = dv.getUint16(2, true)
                        if (volt !== 65535) {
                            cellv[sub * 3 + i] = volt / 1000
                        }
                    }
                } else if (param === 3) {
                    for (var i = 0;i < 3;i++) {
                        var temp = dv.getInt16(2, true)
                        if (temp !== 32767) {
                            cellt[sub * 3 + i] = temp
                        }
                    }
                } else if (param === 5) {
                    if (sub === 1) {
                        var secs = dv.getUint32(2, true)
                        relh = secs / 60 / 60
                    } else if (sub === 2) {
                        var secs = dv.getUint32(2, true)
                        chargeh = secs / 60 / 60
                    }
                } else if (param === 6) {
                    if (sub === 0) {
                        var curr = dv.getUint32(2, true)
                        chargeCurrent = curr / 1000
                    } else if (sub === 1) {
                        var curr = dv.getUint32(2, true)
                        dischargeCurrent = curr / 1000
                    }
                } else if (param === 9) {
                    var str = ""
                    for (var i = 0;i < 6;i++) {
                        str += String.fromCharCode(dv.getUint8(2 + i));
                    }
                    
                    if (sub === 0) {
                        line = str
                    } else {
                        line += str
                    }
                }
            }
            
            mText.text = ""
            for (var i = 0;i < cellv.length;i++) {
                mText.text += "V" + i + ": " + parseFloat(cellv[i]).toFixed(2) + " V\n"
            }
            
            mText2.text = ""
            for (var i = 0;i < cellt.length;i++) {
                mText2.text += "T" + i + " : " + parseFloat(cellt[i]).toFixed(0) + " degC\n"
            }
            
            mText2.text += "Rel: " + parseFloat(relh).toFixed(2) + " h\n"
            mText2.text += "Ch : " + parseFloat(chargeh).toFixed(2) + " h\n"
            mText2.text += "Ch : " + parseFloat(chargeCurrent).toFixed(2) + " Ah\n"
            mText2.text += "Dsc: " + parseFloat(dischargeCurrent).toFixed(2) + " Ah\n"
            
            mText3.text = ""
            mText3.text += line + "\n"
        }
    }
}
