import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc


Item {
    id: mainItem
    anchors.fill: parent
    anchors.margins: 5

    property Commands mCommands: VescIf.commands()
    
    function sendStr(str) {
        // Append null to ensure that the string is null-terminated
        mCommands.sendCustomAppData(str + "\0")
    }
    
    ColumnLayout {
        id: gaugeColumn
        anchors.fill: parent
        
        Button{
            Layout.fillWidth: true
            text: "Send Hello"
            onClicked: {
                sendStr("Hello")
            }
        }
    }
}
