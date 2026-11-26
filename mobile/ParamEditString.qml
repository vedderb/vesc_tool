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
import QtQuick.Layouts 1.3

ParamEditFrame {
    id: editor
    property real maxVal: 1.0

    Component.onCompleted: {
        if (params != null) {
            stringInput.text = params.getParamQString(paramName)

            var maxLen = params.getParamMaxLen(paramName)
            if (maxLen > 0) {
                stringInput.maximumLength = maxLen
            }
        }
    }

    InputField {
        id: stringInput
        Layout.fillWidth: true
        Layout.topMargin: 5
        Layout.bottomMargin: 5
        focus: true

        onTextChanged: {
            if (params != null) {
                if (params.getUpdateOnly() !== paramName) {
                    params.setUpdateOnly("")
                }
                params.updateParamString(paramName, text, editor);
            }
        }
    }

    Connections {
        target: params

        function onParamChangedQString(src, name, newParam) {
            if (src !== editor && name === paramName) {
                stringInput.text = newParam
            }
        }
    }
}
