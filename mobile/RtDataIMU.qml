import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: root
    height: rect.height
    function updateText(values) {
        valText1.text =
                "Roll   : " + (values.roll * 57.2958).toFixed(3) + "\n" +
                "Acc X  : " + values.accX.toFixed(3) + "\n" +
                "Gyro X : " + values.gyroX.toFixed(3)

        valText2.text =
                "Pitch  : " + (values.pitch * 57.2958).toFixed(3) + "\n" +
                "Acc Y  : " + values.accY.toFixed(3) + "\n" +
                "Gyro Y : " + values.gyroY.toFixed(3)

        valText3.text =
                "yaw    : " + (values.yaw * 57.2958).toFixed(3) + "\n" +
                "Acc Z  : " + values.accZ.toFixed(3) + "\n" +
                "Gyro Z : " + values.gyroZ.toFixed(3)
    }

    Rectangle {
        id: rect
        anchors.left: parent.left
        anchors.right: parent.right
        height: valMetrics.height * 3 + 12
        color: Utility.getAppHexColor("darkBackground")

        GridLayout {
            anchors.fill: parent
            Layout.alignment: Qt.AlignHCenter
            columns: 3

            Text {
                id: valText1
                color: Utility.getAppHexColor("lightText")
                font.family: "DejaVu Sans Mono"
                Layout.margins: 0
                Layout.leftMargin: 5
                Layout.preferredWidth: parent.width/3
                text: "Roll   : 0.00\n" +
                      "Acc X  : 0.00\n" +
                      "Gyro X : 0.00"
            }

            Text {
                id: valText2
                color: Utility.getAppHexColor("lightText")
                font.family: "DejaVu Sans Mono"
                Layout.margins: 0
                Layout.preferredWidth: parent.width/3
                text: "Pitch  : 0.00\n" +
                      "Acc Y  : 0.00\n" +
                      "Gyro Y : 0.00"
            }

            Text {
                id: valText3
                color: Utility.getAppHexColor("lightText")
                font.family: "DejaVu Sans Mono"
                Layout.margins: 0
                Layout.preferredWidth: parent.width/3
                text: "Yaw    : 0.00\n" +
                      "Acc Z  : 0.00\n" +
                      "Gyro Z : 0.00"
            }

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: Utility.getAppHexColor("lightAccent")
                Layout.columnSpan: 3
                Layout.margins: 0
            }
        }
    }

    TextMetrics {
        id: valMetrics
        font: valText1.font
        text: valText1.text
    }
}
