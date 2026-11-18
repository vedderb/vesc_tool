/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

// Based on https://stackoverflow.com/questions/43406830/how-to-use-float-in-a-qml-spinbox

import QtQuick 2.15
import QtQuick.Controls 2.15
import Vedder.vesc.utility 1.0

Item {
    height: spinbox.implicitHeight

    property int decimals: 2
    property real realValue: 0.0
    property real realFrom: 0.0
    property real realTo: 100.0
    property real realStepSize: 1.0
    property string suffix: ""
    property string prefix: ""

    SpinBox {
        id: spinbox
        background: InputBackground {}
        anchors.fill: parent
        editable: true
        // The iOS number pads have no return key, so there would be no way to finish the edit
        inputMethodHints: Qt.platform.os === "ios" ? Qt.ImhPreferNumbers | Qt.ImhNoPredictiveText : Qt.ImhFormattedNumbersOnly
//        wheelEnabled: true

        property real factor: Math.pow(10, decimals)
        stepSize: realStepSize * factor
        value: Math.round(realValue * factor)
        to : realTo * factor
        from : realFrom * factor

        validator: DoubleValidator {
            bottom: Math.min(spinbox.from, spinbox.to) * spinbox.factor
            top:  Math.max(spinbox.from, spinbox.to) * spinbox.factor
        }

        textFromValue: function(value, locale) {
            return parseFloat(value * 1.0 / factor).toFixed(decimals);
        }

        valueFromText: function(text, locale) {
            // For some reason android does not hide the keyboard
            // when finishing the edit. Hiding it from here seems
            // to behave as expected. TODO: Test on ios and different
            // phones in general.
            Qt.inputMethod.hide()

            return Math.round(parseFloat(text.replace(",", ".")) * factor)
        }

        // Prefix and suffix are labels around the number, so only the number is edited
        Binding { target: spinbox.contentItem; property: "leftPadding"; value: prefixText.width }
        Binding { target: spinbox.contentItem; property: "rightPadding"; value: suffixText.width }

        Row {
            parent: spinbox.contentItem
            anchors.centerIn: parent

            Text {
                id: prefixText
                text: prefix
                font: spinbox.font
                color: Utility.getAppHexColor("disabledText")
            }

            Item {
                width: spinbox.contentItem.contentWidth
                height: 1
            }

            Text {
                id: suffixText
                text: suffix
                font: spinbox.font
                color: Utility.getAppHexColor("disabledText")
            }
        }

        onValueChanged: {
            if (Math.round(realValue * factor) !== value) {
                realValue = value * 1.0 / factor
            }
        }
    }
}
