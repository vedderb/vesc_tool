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

import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.configparams 1.0

Item {
    id: editor
    property string paramName: ""
    property ConfigParams params: null
    height: 140
    Layout.fillWidth: true
    property real maxVal: 1.0
    property bool createReady: false

    Component.onCompleted: {
        if (params !== null) {
            if (Math.abs(params.getParamMaxDouble(paramName)) > params.getParamMinDouble(paramName)) {
                maxVal = Math.abs(params.getParamMaxDouble(paramName))
            } else {
                maxVal = Math.abs(params.getParamMinDouble(paramName))
            }

            nameText.text = params.getLongName(paramName)
            valueBox.decimals = params.getParamDecimalsDouble(paramName)
            valueBox.realFrom = params.getParamMinDouble(paramName) * params.getParamEditorScale(paramName)
            valueBox.realTo = params.getParamMaxDouble(paramName) * params.getParamEditorScale(paramName)
            valueBox.realStepSize = params.getParamStepDouble(paramName)
            valueBox.visible = !params.getParamEditAsPercentage(paramName)
            valueBox.suffix = params.getParamSuffix(paramName)
            valueBox.realValue = params.getParamDouble(paramName) * params.getParamEditorScale(paramName)

            var p = (params.getParamDouble(paramName) * 100.0) / maxVal
            percentageBox.from = (100.0 * params.getParamMinDouble(paramName)) / maxVal
            percentageBox.to = (100.0 * params.getParamMaxDouble(paramName)) / maxVal
            percentageBox.visible = params.getParamEditAsPercentage(paramName)
            percentageBox.value = p

            if (params.getParamTransmittable(paramName)) {
                nowButton.visible = true
                defaultButton.visible = true
            } else {
                nowButton.visible = false
                defaultButton.visible = false
            }

            createReady = true
        }
    }

    function updateDisplay(value) {
        // TODO: No display for now...
    }

    Rectangle {
        id: rect
        anchors.fill: parent
        color: "#4c5a5a5a"
        radius: 5
        border.color: "#919191"
        border.width: 2

        ColumnLayout {
            id: column
            anchors.fill: parent
            anchors.topMargin: 10
            anchors.margins: 5

            Text {
                id: nameText
                color: "white"
                text: paramName
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                font.pointSize: 12
            }

            DoubleSpinBox {
                id: valueBox
                Layout.fillWidth: true

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

            SpinBox {
                id: percentageBox
                Layout.fillWidth: true
                editable: true

                onValueChanged: {
                    if (params.getParamEditAsPercentage(paramName)) {
                        var val = (value / 100.0) * maxVal

                        if (params !== null && createReady) {
                            if (params.getUpdateOnly() !== paramName) {
                                params.setUpdateOnly("")
                            }
                            params.updateParamDouble(paramName, val, editor);
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

            RowLayout {
                Layout.fillWidth: true
                Button {
                    id: nowButton
                    Layout.fillWidth: true
                    Layout.preferredWidth: 500
                    flat: true
                    text: "Current"
                    onClicked: {
                        params.setUpdateOnly(paramName)
                        params.requestUpdate()
                    }
                }

                Button {
                    id: defaultButton
                    Layout.fillWidth: true
                    Layout.preferredWidth: 500
                    flat: true
                    text: "Default"
                    onClicked: {
                        params.setUpdateOnly(paramName)
                        params.requestUpdateDefault()
                    }
                }

                Button {
                    id: helpButton
                    Layout.fillWidth: true
                    Layout.preferredWidth: 500
                    flat: true
                    text: "Help"
                    onClicked: {
                        VescIf.emitMessageDialog(
                                    params.getLongName(paramName),
                                    params.getDescription(paramName),
                                    true, true)
                    }
                }
            }
        }
    }

    Connections {
        target: params

        onParamChangedDouble: {
            if (src !== editor && name == paramName) {
                valueBox.realValue = newParam * params.getParamEditorScale(paramName)
                percentageBox.value = Math.round((100.0 * newParam) / maxVal)
            }
        }
    }
}
