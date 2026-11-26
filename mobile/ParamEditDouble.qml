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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ParamEditFrame {
    id: editor
    property real maxVal: 1.0
    property bool createReady: false

    Component.onCompleted: {
        if (params != null) {
            if (Math.abs(params.getParamMaxDouble(paramName)) > params.getParamMinDouble(paramName)) {
                maxVal = Math.abs(params.getParamMaxDouble(paramName))
            } else {
                maxVal = Math.abs(params.getParamMinDouble(paramName))
            }

            valueBox.decimals = params.getParamDecimalsDouble(paramName)
            valueBox.realFrom = params.getParamMinDouble(paramName) * params.getParamEditorScale(paramName)
            valueBox.realTo = params.getParamMaxDouble(paramName) * params.getParamEditorScale(paramName)
            valueBox.realStepSize = params.getParamStepDouble(paramName)
            valueBox.visible = !params.getParamEditAsPercentage(paramName)
            valueBox.suffix = params.getParamSuffix(paramName)
            valueBox.realValue = params.getParamDouble(paramName) * params.getParamEditorScale(paramName)

            var p = (params.getParamDouble(paramName) * 100.0) / maxVal
            percentageBox.realFrom = (100.0 * params.getParamMinDouble(paramName)) / maxVal
            percentageBox.realTo = (100.0 * params.getParamMaxDouble(paramName)) / maxVal
            percentageBox.visible = params.getParamEditAsPercentage(paramName)
            percentageBox.realValue = Math.round(p)

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

        onRealValueChanged: {
            if (!params.getParamEditAsPercentage(paramName)) {
                var val = realValue / params.getParamEditorScale(paramName)

                if (params !== null && createReady) {
                    if (params.getUpdateOnly() !== paramName) {
                        params.setUpdateOnly("")
                    }
                    params.updateParamDouble(paramName, val, editor);
                }

                updateDisplay(val);
            }
        }
    }

    DoubleSpinBox {
        id: percentageBox
        Layout.fillWidth: true
        Layout.leftMargin: 5
        Layout.rightMargin: 5
        decimals: 0
        suffix: " %"

        onRealValueChanged: {
            if (params.getParamEditAsPercentage(paramName)) {
                var val = (realValue / 100.0) * maxVal

                if (params !== null && createReady) {
                    if (params.getUpdateOnly() !== paramName) {
                        params.setUpdateOnly("")
                    }
                    params.updateParamDouble(paramName, val, editor);
                }

                updateDisplay(val);
            }
        }
    }

    Connections {
        target: params

        function onParamChangedDouble(src, name, newParam) {
            if (src !== editor && name === paramName) {
                valueBox.realValue = newParam * params.getParamEditorScale(paramName)
                percentageBox.realValue = Math.round((100.0 * newParam) / maxVal)
            }
        }
    }
}
