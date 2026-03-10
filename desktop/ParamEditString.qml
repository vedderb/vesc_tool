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

    Label {
        id: nameLabel
        Layout.preferredWidth: 240
        Layout.minimumWidth: 140
        elide: Text.ElideRight
        font.pointSize: 12
        color: Utility.getAppHexColor("lightText")
        ToolTip.text: nameLabel.text
        ToolTip.visible: nameMA.containsMouse && nameLabel.truncated
        MouseArea { id: nameMA; anchors.fill: parent; hoverEnabled: true }
    }

    TextField {
        id: valField
        Layout.fillWidth: true
        Layout.minimumWidth: 120
        font.pointSize: 12
        implicitHeight: 34

        onTextEdited: {
            if (createReady) {
                params.updateParamString(paramName, text, editor)
            }
        }
    }

    ToolButton {
        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_up.png"
        icon.width: 20; icon.height: 20
        implicitWidth: 34; implicitHeight: 34
        ToolTip.text: "Read Current"; ToolTip.visible: hovered
        visible: params ? params.getParamTransmittable(paramName) : false
        onClicked: { params.setUpdateOnly(paramName); params.requestUpdate() }
    }

    ToolButton {
        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_default.png"
        icon.width: 20; icon.height: 20
        implicitWidth: 34; implicitHeight: 34
        ToolTip.text: "Read Default"; ToolTip.visible: hovered
        visible: params ? params.getParamTransmittable(paramName) : false
        onClicked: { params.setUpdateOnly(paramName); params.requestUpdateDefault() }
    }

    ToolButton {
        icon.source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
        icon.width: 20; icon.height: 20
        implicitWidth: 34; implicitHeight: 34
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
