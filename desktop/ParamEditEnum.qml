/*
    Desktop ParamEditEnum — compact single-row enum editor with ComboBox
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
        text: params ? params.getLongName(paramName) : ""
        ToolTip.text: nameLabel.text
        ToolTip.visible: nameMA.containsMouse && nameLabel.truncated
        MouseArea { id: nameMA; anchors.fill: parent; hoverEnabled: true }
    }

    ComboBox {
        id: enumCombo
        Layout.fillWidth: true
        Layout.minimumWidth: 120
        font.pointSize: 12
        implicitHeight: 34
        editable: false
        textRole: "display"
        model: Utility.stringListModel(params ? params.getParamEnumNames(paramName) : [])
        currentIndex: params ? params.getParamEnum(paramName) : 0

        onCurrentIndexChanged: {
            if (createReady) {
                params.updateParamEnum(paramName, currentIndex, editor)
            }
        }

        Component.onCompleted: createReady = true
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

    Connections {
        target: params
        function onParamChangedEnum(src, name, newParam) {
            if (src !== editor && name === paramName && createReady) {
                enumCombo.currentIndex = newParam
            }
        }
    }
}
