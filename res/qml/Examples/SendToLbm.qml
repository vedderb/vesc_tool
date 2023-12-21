import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import Vedder.vesc.utility 1.0

import Vedder.vesc.commands 1.0

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
