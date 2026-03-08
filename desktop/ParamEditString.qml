/*
    Desktop ParamEditString — compact single-row string editor
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

RowLayout {
    id: editor
    property string paramName: ""
    property ConfigParams params: null
    property bool createReady: false

    spacing: 4
    Layout.fillWidth: true
    implicitHeight: 28

    Label {
        id: nameLabel
        Layout.preferredWidth: 200
        Layout.minimumWidth: 120
        elide: Text.ElideRight
        font.pixelSize: 12
        color: Utility.getAppHexColor("lightText")
        ToolTip.text: nameLabel.text
        ToolTip.visible: nameMA.containsMouse && nameLabel.truncated
        MouseArea { id: nameMA; anchors.fill: parent; hoverEnabled: true }
    }

    TextField {
        id: valField
        Layout.fillWidth: true
        Layout.minimumWidth: 100
        font.pixelSize: 12

        onTextEdited: {
            if (createReady) {
                params.updateParamString(paramName, text, editor)
            }
        }
    }

    ToolButton {
        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_up.png"
        icon.width: 16; icon.height: 16
        implicitWidth: 28; implicitHeight: 28
        ToolTip.text: "Read Current"; ToolTip.visible: hovered
        visible: params ? params.getParamTransmittable(paramName) : false
        onClicked: { params.setUpdateOnly(paramName); params.requestUpdate() }
    }

    ToolButton {
        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_default.png"
        icon.width: 16; icon.height: 16
        implicitWidth: 28; implicitHeight: 28
        ToolTip.text: "Read Default"; ToolTip.visible: hovered
        visible: params ? params.getParamTransmittable(paramName) : false
        onClicked: { params.setUpdateOnly(paramName); params.requestUpdateDefault() }
    }

    ToolButton {
        icon.source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
        icon.width: 16; icon.height: 16
        implicitWidth: 28; implicitHeight: 28
        ToolTip.text: "Help"; ToolTip.visible: hovered
        onClicked: {
            VescIf.emitMessageDialog(params.getLongName(paramName),
                                     params.getDescription(paramName), true, true)
        }
    }

    Component.onCompleted: {
        if (!params || paramName === "") return

        nameLabel.text = params.getLongName(paramName)
        var maxLen = params.getParamMaxLen(paramName)
        if (maxLen > 0) valField.maximumLength = maxLen
        valField.text = params.getParamQString(paramName)

        createReady = true
    }

    Connections {
        target: params
        function onParamChangedQString(src, name, newParam) {
            if (src !== editor && name === paramName && createReady) {
                valField.text = newParam
            }
        }
    }
}
