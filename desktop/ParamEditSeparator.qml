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
    implicitHeight: 30
    radius: 2

    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: Utility.getAppHexColor("lightAccent") }
        GradientStop { position: 0.2; color: Utility.getAppHexColor("darkAccent") }
        GradientStop { position: 0.8; color: Utility.getAppHexColor("darkAccent") }
        GradientStop { position: 1.0; color: Utility.getAppHexColor("lightAccent") }
    }

    Label {
        anchors.centerIn: parent
        text: sepName
        font.bold: true
        font.pointSize: 12
        color: Utility.getAppHexColor("white")
    }
}
