import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: root
    property var tileValues: []
    height: grid.implicitHeight + 16

    function updateText(values) {
        tileValues = [(values.roll * 57.2958).toFixed(3), (values.pitch * 57.2958).toFixed(3),
                      (values.yaw * 57.2958).toFixed(3), values.accX.toFixed(3), values.accY.toFixed(3),
                      values.accZ.toFixed(3), values.gyroX.toFixed(3), values.gyroY.toFixed(3), values.gyroZ.toFixed(3)]
    }

    GridLayout {
        id: grid
        anchors.fill: parent
        anchors.margins: 8
        columns: 3
        rowSpacing: 6
        columnSpacing: 6

        Repeater {
            model: ["Roll", "Pitch", "Yaw", "Acc X", "Acc Y", "Acc Z", "Gyro X", "Gyro Y", "Gyro Z"]

            ValueTile {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                implicitHeight: 46
                title: modelData
                value: tileValues[index] || "0.000"
                unit: index < 3 ? "\u00B0" : index < 6 ? "g" : "\u00B0/s"
            }
        }
    }
}
