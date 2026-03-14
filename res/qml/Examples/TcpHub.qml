import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import Vedder.vesc


Item {
    id: mainItem
    anchors.fill: parent
    anchors.margins: 5

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    
    TcpHub {
        id: hub
    }
    
    Component.onCompleted: {
        console.log(hub.ping("veschub.vedder.se", 65101, "2093051450"))
    }
}
