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
            nameText.text = params.getLongName(paramName)
            enumBox.model = params.getParamEnumNames(paramName)
            enumBox.currentIndex = params.getParamEnum(paramName)

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
            anchors.bottomMargin: 2
            anchors.margins: 10

            Text {
                id: nameText
                color: "white"
                text: paramName
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                font.pointSize: 12
            }

            ComboBox {
                id: enumBox
                Layout.fillWidth: true

                background: Rectangle {
                    implicitHeight: 35
                    color: enumBox.pressed ? "#606060" : "#505050"
                    border.color: enumBox.hovered ? "#21be2b" : "#17a81a"
                    border.width: enumBox.visualFocus ? 2 : 1
                    radius: 5
                }

                onCurrentIndexChanged: {
                    if (params !== null && createReady) {
                        if (params.getUpdateOnly() !== paramName) {
                            params.setUpdateOnly("")
                        }
                        params.updateParamEnum(paramName, currentIndex, editor);
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

        onParamChangedEnum: {
            if (src !== editor && name == paramName) {
                enumBox.currentIndex = newParam
            }
        }
    }
}
