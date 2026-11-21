/*
    Copyright 2018 - 2026 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Controls
import Vedder.vesc.utility 1.0

DoubleSpinBox {
    id: root

    property alias realValue: root.value
    property alias realFrom: root.from
    property alias realTo: root.to
    property alias realStepSize: root.stepSize
    property string suffix: ""
    property string prefix: ""

    editable: true
    decimals: 2
    from: 0.0
    to: 100.0
    stepSize: 1.0

    inputMethodHints: Qt.platform.os === "ios"
                      ? (Qt.ImhPreferNumbers | Qt.ImhNoPredictiveText)
                      : Qt.ImhFormattedNumbersOnly

    Binding {
        target: root.contentItem
        property: "leftPadding"
        value: prefixText.width
    }
    Binding {
        target: root.contentItem
        property: "rightPadding"
        value: suffixText.width
    }

    Row {
        parent: root.contentItem
        anchors.centerIn: parent

        Text {
            id: prefixText
            text: root.prefix
            font: root.font
            color: Utility.getAppHexColor("disabledText")
        }

        Item {
            width: root.contentItem.contentWidth
            height: 1
        }

        Text {
            id: suffixText
            text: root.suffix
            font: root.font
            color: Utility.getAppHexColor("disabledText")
        }
    }
}
