// This example attempts to reconnect when the connection is lost and upload the motor
// configuration afterwards. It is useful during firmware development to keep VESC Tool
// connected and the configuration updated.

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

    Material.theme: Utility.isDarkMode() ? "Dark" : "Light"
    Material.accent: Utility.getAppHexColor("lightAccent")
    
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
