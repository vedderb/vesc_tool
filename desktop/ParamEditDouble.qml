/*
    Desktop ParamEditDouble — compact single-row editor matching
    the original widget QDoubleSpinBox + [Read][Default][Help] layout.
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

    SpinBox {
        id: valSpin
        Layout.fillWidth: true
        Layout.minimumWidth: 100
        editable: true
        font.pixelSize: 12

        property int decimals: 2
        property double realFrom: 0
        property double realTo: 100
        property double realStep: 1
        property double realValue: 0
        property double factor: Math.pow(10, decimals)
        property string unit: ""

        from: Math.round(realFrom * factor)
        to: Math.round(realTo * factor)
        stepSize: Math.max(1, Math.round(realStep * factor))
        value: Math.round(realValue * factor)

        textFromValue: function(value, locale) {
            return (value / factor).toFixed(decimals) + (unit.length > 0 ? " " + unit : "")
        }
        valueFromText: function(text, locale) {
            var s = text.replace(unit, "").trim()
            return Math.round(parseFloat(s) * factor)
        }

        onValueModified: {
            if (createReady) {
                var scaled = value / factor
                params.updateParamDouble(paramName, scaled / params.getParamEditorScale(paramName), editor)
            }
        }
    }

    // Percentage spin (hidden unless editAsPercentage)
    SpinBox {
        id: pctSpin
        visible: false
        Layout.fillWidth: true
        Layout.minimumWidth: 80
        editable: true
        font.pixelSize: 12
        from: 0
        to: 100
        stepSize: 1
        textFromValue: function(v) { return v + " %" }
        valueFromText: function(t) { return parseInt(t) }

        onValueModified: {
            if (createReady) {
                var raw = pctSpin.value / 100.0 * (params.getParamMaxDouble(paramName) - params.getParamMinDouble(paramName)) + params.getParamMinDouble(paramName)
                params.updateParamDouble(paramName, raw, editor)
            }
        }
    }

    ToolButton {
        id: readBtn
        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_up.png"
        icon.width: 16; icon.height: 16
        implicitWidth: 28; implicitHeight: 28
        ToolTip.text: "Read Current"; ToolTip.visible: hovered
        visible: params ? params.getParamTransmittable(paramName) : false
        onClicked: { params.setUpdateOnly(paramName); params.requestUpdate() }
    }

    ToolButton {
        id: defBtn
        icon.source: "qrc" + Utility.getThemePath() + "icons/motor_default.png"
        icon.width: 16; icon.height: 16
        implicitWidth: 28; implicitHeight: 28
        ToolTip.text: "Read Default"; ToolTip.visible: hovered
        visible: params ? params.getParamTransmittable(paramName) : false
        onClicked: { params.setUpdateOnly(paramName); params.requestUpdateDefault() }
    }

    ToolButton {
        id: helpBtn
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

        var scale = params.getParamEditorScale(paramName)
        nameLabel.text = params.getLongName(paramName)

        if (params.getParamEditAsPercentage(paramName)) {
            valSpin.visible = false
            pctSpin.visible = true
            var range = params.getParamMaxDouble(paramName) - params.getParamMinDouble(paramName)
            var pct = (params.getParamDouble(paramName) - params.getParamMinDouble(paramName)) / range * 100
            pctSpin.value = Math.round(pct)
        } else {
            valSpin.decimals = params.getParamDecimalsDouble(paramName)
            valSpin.factor = Math.pow(10, valSpin.decimals)
            valSpin.unit = params.getParamSuffix(paramName)
            valSpin.realFrom = params.getParamMinDouble(paramName) * scale
            valSpin.realTo = params.getParamMaxDouble(paramName) * scale
            valSpin.realStep = params.getParamStepDouble(paramName) * scale
            valSpin.realValue = params.getParamDouble(paramName) * scale
            // Force re-calculation of int ranges after factor changed
            valSpin.from = Math.round(valSpin.realFrom * valSpin.factor)
            valSpin.to = Math.round(valSpin.realTo * valSpin.factor)
            valSpin.stepSize = Math.max(1, Math.round(valSpin.realStep * valSpin.factor))
            valSpin.value = Math.round(valSpin.realValue * valSpin.factor)
        }

        createReady = true
    }

    Connections {
        target: params
        function onParamChangedDouble(src, name, newParam) {
            if (src !== editor && name === paramName && createReady) {
                var scale = params.getParamEditorScale(paramName)
                if (params.getParamEditAsPercentage(paramName)) {
                    var range = params.getParamMaxDouble(paramName) - params.getParamMinDouble(paramName)
                    pctSpin.value = Math.round((newParam - params.getParamMinDouble(paramName)) / range * 100)
                } else {
                    valSpin.value = Math.round(newParam * scale * valSpin.factor)
                }
            }
        }
    }
}
