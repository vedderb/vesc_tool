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

import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

ParamEditFrame {
    id: editor
    property real maxVal: 1.0
    property bool createReady: false

    Component.onCompleted: {
        if (params != null) {
            if (Math.abs(params.getParamMaxInt(paramName)) > params.getParamMinInt(paramName)) {
                maxVal = Math.abs(params.getParamMaxInt(paramName))
            } else {
                maxVal = Math.abs(params.getParamMinInt(paramName))
            }

            valueBox.realFrom = params.getParamMinInt(paramName) * params.getParamEditorScale(paramName)
            valueBox.realTo = params.getParamMaxInt(paramName) * params.getParamEditorScale(paramName)
            valueBox.realStepSize = params.getParamStepInt(paramName)
            valueBox.visible = !params.getParamEditAsPercentage(paramName)
            valueBox.suffix = params.getParamSuffix(paramName)
            valueBox.realValue = params.getParamInt(paramName) * params.getParamEditorScale(paramName)

            var p = (params.getParamInt(paramName) * 100.0) / maxVal
            percentageBox.from = (100.0 * params.getParamMinInt(paramName)) / maxVal
            percentageBox.to = (100.0 * params.getParamMaxInt(paramName)) / maxVal
            percentageBox.visible = params.getParamEditAsPercentage(paramName)
            percentageBox.value = p

            createReady = true
        }
    }

    function updateDisplay(value) {
        // TODO: No display for now...
    }

    DoubleSpinBox {
        id: valueBox
        Layout.fillWidth: true
        Layout.leftMargin: 5
        Layout.rightMargin: 5
        decimals: 0

        onRealValueChanged: {
            if (!params.getParamEditAsPercentage(paramName)) {
                var val = realValue / params.getParamEditorScale(paramName)

                if (params !== null && createReady) {
                    if (params.getUpdateOnly() !== paramName) {
                        params.setUpdateOnly("")
                    }
                    params.updateParamInt(paramName, val, editor);
                }

                updateDisplay(val);
            }
        }
    }

    SpinBox {
        id: percentageBox
        background: InputBackground {}
        Layout.fillWidth: true
        Layout.leftMargin: 5
        Layout.rightMargin: 5
        editable: true
        visible: false

        onValueChanged: {
            if (params.getParamEditAsPercentage(paramName)) {
                var val = (value / 100.0) * maxVal

                if (params != null && createReady) {
                    if (params.getUpdateOnly() !== paramName) {
                        params.setUpdateOnly("")
                    }
                    params.updateParamInt(paramName, val, editor);
                }

                updateDisplay(val);
            }
        }

        textFromValue: function(value, locale) {
            return Number(value).toLocaleString(locale, 'f', 0) + " %"
        }

        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text.replace("%", ""))
        }
    }

    Connections {
        target: params

        function onParamChangedInt(src, name, newParam) {
            if (src !== editor && name === paramName) {
                valueBox.realValue = newParam * params.getParamEditorScale(paramName)
                percentageBox.value = Math.round((100.0 * newParam) / maxVal)
            }
        }
    }
}
