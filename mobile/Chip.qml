/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Controls.Material
import Vedder.vesc.utility 1.0

AbstractButton {
    id: chip
    property string iconSource: ""
    property bool dot: false
    property color tint: Material.accent
    property color tintText: Utility.getAppHexColor("accentText")
    property bool tinted: false
    readonly property bool colored: checked || tinted

    implicitHeight: 32
    implicitWidth: row.implicitWidth + 24
    focusPolicy: Qt.NoFocus

    contentItem: Item {
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 6

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                height: 16
                visible: chip.iconSource !== ""
                source: chip.iconSource
                smooth: true
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: chip.text
                color: chip.colored ? chip.tintText : Utility.getAppHexColor("lightText")
                font.weight: chip.checked ? Font.Medium : Font.Normal
            }

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 6
                height: 6
                radius: 3
                visible: chip.dot
                color: Material.accent
            }
        }
    }

    background: Rectangle {
        radius: height / 2
        color: chip.colored ? Qt.alpha(chip.tint, 0.12) : Utility.getAppHexColor("cardBackground")
        border.width: chip.checked ? 1.5 : 1
        border.color: chip.checked ? chip.tint : chip.tinted ? Qt.alpha(chip.tint, 0.4) :
                                                              Utility.getAppHexColor("lightestBackground")
    }
}
