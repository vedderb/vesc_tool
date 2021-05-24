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

    Component.onCompleted: {
        if (params !== null) {
            nameText.text = params.getLongName(paramName)
            stringInput.text = params.getParamQString(paramName)

            if (params.getParamTransmittable(paramName)) {
                nowButton.visible = true
                defaultButton.visible = true
            } else {
                nowButton.visible = false
                defaultButton.visible = false
            }
        }
    }

    Rectangle {
        id: rect
        anchors.fill: parent
        color: "#4cbfbfbf"
        radius: 10
        border.color: "#cfcfcf"
        border.width: 3

        ColumnLayout {
            id: column
            anchors.fill: parent
            anchors.margins: 10

            Text {
                id: nameText
                color: "white"
                text: paramName
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                font.pointSize: 12
            }

            Rectangle {
                Layout.fillWidth: true
                height: stringInput.implicitHeight + 14
                border.width: 2
                border.color: "#8d8d8d"
                color: "#33a8a8a8"
                radius: 3
                TextInput {
                    id: stringInput
                    color: "white"
                    anchors.fill: parent
                    anchors.margins: 7
                    font.pointSize: 12
                    focus: true

                    onTextChanged: {
                        if (params !== null) {
                            if (params.getUpdateOnly() !== paramName) {
                                params.setUpdateOnly("")
                            }
                            params.updateParamString(paramName, text, editor);
                        }
                    }
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

        onParamChangedQString: {
            if (src !== editor && name == paramName) {
                stringInput.text = newParam
            }
        }
    }
}
