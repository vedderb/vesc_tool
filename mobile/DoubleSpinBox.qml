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

import QtQuick 2.0
import QtQuick.Controls 2.2

Item {
    height: spinbox.implicitHeight

    property int decimals: 2
    property real realValue: 0.0
    property real realFrom: 0.0
    property real realTo: 100.0
    property real realStepSize: 1.0
    property string suffix: ""
    property string prefix: ""

    onPrefixChanged: forceUpdate()
    onSuffixChanged: forceUpdate()

    function forceUpdate() {
        var old = realValue
        realValue += realStepSize
        realValue = old
    }

    SpinBox {
        id: spinbox
        anchors.fill: parent
        editable: true
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
            return prefix + parseFloat(value * 1.0 / factor).toFixed(decimals) + suffix;
        }

        valueFromText: function(text, locale) {
            return Math.round(parseFloat(text.replace(",", ".").
                                         replace(suffix, "").
                                         replace(prefix, "")) * factor)
        }

        onValueChanged: {
            if (Math.round(realValue * factor) !== value) {
                realValue = value * 1.0 / factor
            }
        }
    }
}
