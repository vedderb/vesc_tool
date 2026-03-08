// This example attempts to reconnect when the connection is lost and upload the motor
// configuration afterwards. It is useful during firmware development to keep VESC Tool
// connected and the configuration updated.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import Vedder.vesc
import "qrc:/mobile"

Item {
    anchors.fill: parent
    anchors.margins: 10
    
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mAppConf: VescIf.appConfig()
    
    Timer {
        running: true
        repeat: true
        interval: 2000
        
        property bool wasConnected: true
                
        onTriggered: {
            if (!VescIf.isPortConnected()) {
                VescIf.reconnectLastPort()
                wasConnected = false
            } else {
                if (!wasConnected) {
                    mCommands.setMcconf(false)
                    wasConnected = true
                }
            }
        }
    }
}
