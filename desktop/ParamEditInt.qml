/*
    Desktop ParamEditInt — compact single-row int editor
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

    SpinBox {
        id: valSpin
        Layout.fillWidth: true
        Layout.minimumWidth: 120
        editable: true
        font.pointSize: 12
        topPadding: 6; bottomPadding: 6

        property string unit: ""
        property double editorScale: 1.0

        from: -2000000000
        to: 2000000000

        validator: IntValidator {
            bottom: valSpin.from
            top: valSpin.to
        }

        textFromValue: function(value, locale) {
            var v = Math.round(value * editorScale)
            return v.toString() + (unit.length > 0 ? " " + unit : "")
        }
        valueFromText: function(text, locale) {
            var s = text.replace(unit, "").trim()
            return Math.round(parseInt(s) / editorScale)
        }

        onValueModified: {
            if (createReady) {
                params.updateParamInt(paramName, value, editor)
            }
        }
    }

    // Percentage spin
    SpinBox {
        id: pctSpin
        visible: false
        Layout.fillWidth: true
        Layout.minimumWidth: 100
        editable: true
        font.pointSize: 12
        topPadding: 6; bottomPadding: 6
        from: 0; to: 100; stepSize: 1

        validator: IntValidator {
            bottom: 0
            top: 100
        }

        textFromValue: function(v) { return v + " %" }
        valueFromText: function(t) { return parseInt(t) }

        onValueModified: {
            if (createReady) {
                var range = params.getParamMaxInt(paramName) - params.getParamMinInt(paramName)
                var raw = Math.round(pctSpin.value / 100.0 * range + params.getParamMinInt(paramName))
                params.updateParamInt(paramName, raw, editor)
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

        if (params.getParamEditAsPercentage(paramName)) {
            valSpin.visible = false
            pctSpin.visible = true
            var range = params.getParamMaxInt(paramName) - params.getParamMinInt(paramName)
            var pct = (params.getParamInt(paramName) - params.getParamMinInt(paramName)) / range * 100
            pctSpin.value = Math.round(pct)
        } else {
            var sc = params.getParamEditorScale(paramName)
            valSpin.editorScale = sc
            valSpin.unit = params.getParamSuffix(paramName)
            valSpin.from = params.getParamMinInt(paramName)
            valSpin.to = params.getParamMaxInt(paramName)
            var step = params.getParamStepInt(paramName)
            valSpin.stepSize = step > 0 ? step : 1
            valSpin.value = params.getParamInt(paramName)
        }

        createReady = true
    }

    Connections {
        target: params
        function onParamChangedInt(src, name, newParam) {
            if (src !== editor && name === paramName && createReady) {
                if (params.getParamEditAsPercentage(paramName)) {
                    var range = params.getParamMaxInt(paramName) - params.getParamMinInt(paramName)
                    pctSpin.value = Math.round((newParam - params.getParamMinInt(paramName)) / range * 100)
                } else {
                    valSpin.value = newParam
                }
            }
        }
    }
}
