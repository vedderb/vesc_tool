/*
    Desktop ParamEditSeparator — styled section header
    matching the original widget's gradient separator rows
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Rectangle {
    id: sep
    property string sepName: ""

    Layout.fillWidth: true
    implicitHeight: 25
    color: Utility.getAppHexColor("darkAccent")
    radius: 2

    Label {
        anchors.centerIn: parent
        text: sepName
        font.bold: true
        font.pixelSize: 12
        color: Utility.getAppHexColor("white")
    }
}
