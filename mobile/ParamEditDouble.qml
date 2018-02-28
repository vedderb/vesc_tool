import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.configparams 1.0

Item {
    property String paramName: ""
    property ConfigParams params: 0

    Layout.fillWidth: true
    Layout.fillHeight: false
    
    ColumnLayout {
        id: colLayout
        anchors.fill: parent

        Component.onCompleted: {
            if (params !== 0) {

            }
        }
        
        Text {
            id: name
            text: paramName
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        ProgressBar {
            id: bar
            Layout.fillWidth: true
        }
    }
}
