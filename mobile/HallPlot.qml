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
import Vedder.vesc.utility 1.0

// The hall states at their angles on one electrical revolution. Well placed
// sensors give six states 60° apart
Item {
    id: hallPlot
    property var table: []
    readonly property real radius: width / 2 - 16

    implicitWidth: 170
    implicitHeight: width

    Rectangle {
        anchors.fill: parent
        anchors.margins: 16
        radius: width / 2
        color: "transparent"
        border.width: 2
        border.color: Utility.getAppHexColor("raisedBackground")
    }

    Repeater {
        model: hallPlot.table.length == 8 ? hallPlot.table : []

        Item {
            // 0 - 200 is one revolution, 255 marks a state that cannot occur
            readonly property real angle: modelData * 1.8 - 90

            anchors.fill: parent
            visible: modelData !== 255

            Rectangle {
                x: parent.width / 2
                y: parent.height / 2 - 1
                width: hallPlot.radius
                height: 2
                transformOrigin: Item.Left
                rotation: parent.angle
                color: Utility.getAppHexColor("lightAccent")
                opacity: 0.4
            }

            Rectangle {
                x: parent.width / 2 + hallPlot.radius * Math.cos(parent.angle * Math.PI / 180) - width / 2
                y: parent.height / 2 + hallPlot.radius * Math.sin(parent.angle * Math.PI / 180) - height / 2
                width: 28
                height: 28
                radius: 14
                color: Utility.getAppHexColor("cardBackground")
                border.width: 2
                border.color: Utility.getAppHexColor("lightAccent")

                Text {
                    anchors.centerIn: parent
                    text: index
                    color: Utility.getAppHexColor("lightText")
                    font.weight: Font.Medium
                }
            }
        }
    }
}
