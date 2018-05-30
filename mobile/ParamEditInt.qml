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
    height: column.implicitHeight + 2 * column.anchors.margins
    Layout.fillWidth: true
    property real maxVal: 1.0
    property bool createReady: false

    Component.onCompleted: {
        if (params !== null) {
            if (Math.abs(params.getParamMaxInt(paramName)) > params.getParamMinInt(paramName)) {
                maxVal = Math.abs(params.getParamMaxInt(paramName))
            } else {
                maxVal = Math.abs(params.getParamMinInt(paramName))
            }

            nameText.text = params.getLongName(paramName)
            valueBox.from = params.getParamMinInt(paramName) * params.getParamEditorScale(paramName)
            valueBox.to = params.getParamMaxInt(paramName) * params.getParamEditorScale(paramName)
            valueBox.value = params.getParamInt(paramName) * params.getParamEditorScale(paramName)
            valueBox.stepSize = params.getParamStepInt(paramName)
            valueBox.visible = !params.getParamEditAsPercentage(paramName)
            valueBox.suffix = params.getParamSuffix(paramName)

            var p = (params.getParamInt(paramName) * 100.0) / maxVal
            percentageBox.from = (100.0 * params.getParamMinInt(paramName)) / maxVal
            percentageBox.to = (100.0 * params.getParamMaxInt(paramName)) / maxVal
            percentageBox.value = p
            percentageBox.visible = params.getParamEditAsPercentage(paramName)

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
        color: "#4cbfbfbf"
        radius: 10
        border.color: "#4c000000"
        border.width: 3

        ColumnLayout {
            id: column
            anchors.fill: parent
            anchors.margins: 10

            Text {
                id: nameText
                text: paramName
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                font.pointSize: 12
            }

            SpinBox {
                id: valueBox
                Layout.fillWidth: true
                property string suffix: ""
                editable: true

                onValueChanged: {
                    if (!params.getParamEditAsPercentage(paramName)) {
                        var val = value / params.getParamEditorScale(paramName)

                        if (params !== null && createReady) {
                            if (params.getUpdateOnly() !== paramName) {
                                params.setUpdateOnly("")
                            }
                            params.updateParamInt(paramName, val, editor);
                        }

                        updateDisplay(val);
                    }
                }

                textFromValue: function(value, locale) {
                    return Number(value).toLocaleString(locale, 'f', 0) + suffix
                }

                valueFromText: function(text, locale) {
                    return Number.fromLocaleString(locale, text.replace(suffix, ""))
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

        onParamChangedInt: {
            if (src !== editor && name == paramName) {
                valueBox.value = newParam * params.getParamEditorScale(paramName)
                percentageBox.value = Math.round((100.0 * newParam) / maxVal)
            }
        }
    }
}
