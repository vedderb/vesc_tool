/*
    Desktop ParamEditBitfield — compact bitfield editor
    (8 checkboxes in a 4-column grid, like the original widget)
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
        Layout.preferredWidth: 200
        Layout.minimumWidth: 120
        Layout.alignment: Qt.AlignTop
        elide: Text.ElideRight
        font.pixelSize: 12
        color: Utility.getAppHexColor("lightText")
        ToolTip.text: nameLabel.text
        ToolTip.visible: nameMA.containsMouse && nameLabel.truncated
        MouseArea { id: nameMA; anchors.fill: parent; hoverEnabled: true }
    }

    GridLayout {
        id: bitGrid
        Layout.fillWidth: true
        columns: 4
        rowSpacing: 2
        columnSpacing: 8

        Repeater {
            id: bitRepeater
            model: 8
            CheckBox {
                id: bitCb
                property int bitIndex: index
                property string bitName: ""
                visible: bitName !== "" && bitName.toLowerCase() !== "unused"
                text: bitName
                font.pixelSize: 11

                onToggled: {
                    if (createReady) {
                        var res = 0
                        for (var i = 0; i < bitRepeater.count; i++) {
                            var item = bitRepeater.itemAt(i)
                            if (item && item.checked) {
                                res |= (1 << i)
                            }
                        }
                        params.updateParamInt(paramName, res, editor)
                    }
                }
            }
        }
    }

    ToolButton {
        Layout.alignment: Qt.AlignTop
        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_up.png"
        icon.width: 16; icon.height: 16
        implicitWidth: 28; implicitHeight: 28
        ToolTip.text: "Read Current"; ToolTip.visible: hovered
        visible: params ? params.getParamTransmittable(paramName) : false
        onClicked: { params.setUpdateOnly(paramName); params.requestUpdate() }
    }

    ToolButton {
        Layout.alignment: Qt.AlignTop
        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_default.png"
        icon.width: 16; icon.height: 16
        implicitWidth: 28; implicitHeight: 28
        ToolTip.text: "Read Default"; ToolTip.visible: hovered
        visible: params ? params.getParamTransmittable(paramName) : false
        onClicked: { params.setUpdateOnly(paramName); params.requestUpdateDefault() }
    }

    ToolButton {
        Layout.alignment: Qt.AlignTop
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
        var names = params.getParamEnumNames(paramName)
        var val = params.getParamInt(paramName)

        for (var i = 0; i < 8; i++) {
            var item = bitRepeater.itemAt(i)
            if (item) {
                item.bitName = (i < names.length) ? names[i] : ""
                item.checked = (val & (1 << i)) !== 0
            }
        }

        createReady = true
    }

    Connections {
        target: params
        function onParamChangedInt(src, name, newParam) {
            if (src !== editor && name === paramName && createReady) {
                for (var i = 0; i < bitRepeater.count; i++) {
                    var item = bitRepeater.itemAt(i)
                    if (item) {
                        item.checked = (newParam & (1 << i)) !== 0
                    }
                }
            }
        }
    }
}
