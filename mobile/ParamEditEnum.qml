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
    property int currentIndex: -1
    useSheet: true

    Component.onCompleted: {
        if (params != null) {
            enumRepeater.model = params.getParamEnumNames(paramName)
            currentIndex = params.getParamEnum(paramName)
            createReady = true
        }
    }

    onCurrentIndexChanged: {
        if (params != null && createReady) {
            if (params.getUpdateOnly() !== paramName) {
                params.setUpdateOnly("")
            }
            params.updateParamEnum(paramName, currentIndex, editor);
        }
    }

    Repeater {
        id: enumRepeater

        RadioDelegate {
            Layout.fillWidth: true
            text: params.enumLabel(modelData)
            checked: index === editor.currentIndex
            onClicked: {
                editor.currentIndex = index
                editor.closeSheet()
            }
        }
    }

    Connections {
        target: params

        function onParamChangedEnum(src, name, newParam) {
            if (src !== editor && name === paramName) {
                currentIndex = newParam
            }
        }
    }
}
