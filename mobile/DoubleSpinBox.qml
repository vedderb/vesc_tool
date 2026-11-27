/*
    Copyright 2018 - 2026 Benjamin Vedder	benjamin@vedder.se
*/

import QtQuick
import QtQuick.Controls as Controls
import Vedder.vesc.utility 1.0

Controls.SpinBox {
    id: root
    locale: Qt.locale("C")

    property int decimals: 2
    property real realValue: 0.0
    property real realFrom: 0.0
    property real realTo: 100.0
    property real realStepSize: 1.0
    property string suffix: ""
    property string prefix: ""

    readonly property int factor: Math.pow(10, decimals)

    from: Math.round(realFrom * factor)
    to: Math.round(realTo * factor)
    stepSize: Math.max(1, Math.round(realStepSize * factor))

    onRealValueChanged: {
        var target = Math.round(realValue * factor)
        if (value !== target) {
            value = target
        }
    }

    onValueChanged: {
        var nextReal = value / factor
        if (Math.abs(realValue - nextReal) > (0.5 / factor)) {
            realValue = nextReal
        }
    }

    validator: DoubleValidator {
        bottom: Math.min(root.realFrom, root.realTo)
        top: Math.max(root.realFrom, root.realTo)
        decimals: root.decimals
        notation: DoubleValidator.StandardNotation
    }

    textFromValue: function(value, locale) {
        return Number(value / factor).toFixed(decimals)
    }

    valueFromText: function(text, locale) {
        var num = parseFloat(text.replace(",", "."))
        return isNaN(num) ? 0 : Math.round(num * factor)
    }

    background: InputBackground {}
    editable: true

    inputMethodHints: Qt.platform.os === "ios"
                      ? (Qt.ImhPreferNumbers | Qt.ImhNoPredictiveText)
                      : Qt.ImhFormattedNumbersOnly

    Binding {
        target: root.contentItem
        property: "leftPadding"
        value: prefixText.width
    }
    Binding {
        target: root.contentItem
        property: "rightPadding"
        value: suffixText.width
    }

    Row {
        parent: root.contentItem
        anchors.centerIn: parent

        Text {
            id: prefixText
            text: root.prefix
            font: root.font
            color: Utility.getAppHexColor("disabledText")
        }

        Item {
            width: root.contentItem.contentWidth
            height: 1
        }

        Text {
            id: suffixText
            text: root.suffix
            font: root.font
            color: Utility.getAppHexColor("disabledText")
        }
    }
}
