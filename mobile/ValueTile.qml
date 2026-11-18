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

Rectangle {
    property alias title: titleText.text
    property alias value: valueText.text
    property alias unit: unitText.text
    property alias valueColor: valueText.color
    property real level: -1
    property color levelColor: Utility.getAppHexColor("lightAccent")

    implicitWidth: 150
    implicitHeight: 54
    radius: 12
    clip: true
    color: Utility.getAppHexColor("cardBackground")
    border.color: Utility.getAppHexColor("raisedBackground")

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 10
        anchors.rightMargin: 10

        Text {
            id: titleText
            width: parent.width
            elide: Text.ElideRight
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 9
            font.letterSpacing: 0.5
            font.capitalization: Font.AllUppercase
        }

        Row {
            spacing: 4

            Text {
                id: valueText
                // Shrinks instead of pushing the unit out of the tile
                width: Math.min(implicitWidth, titleText.width - unitText.width - parent.spacing)
                fontSizeMode: Text.HorizontalFit
                minimumPointSize: 9
                elide: Text.ElideRight
                color: Utility.getAppHexColor("titleText")
                font.pointSize: 15
                font.weight: Font.Medium
                font.features: { "tnum": 1 }
            }

            Text {
                id: unitText
                anchors.baseline: valueText.baseline
                color: Utility.getAppHexColor("normalText")
                font.pointSize: 10
            }
        }
    }

    Rectangle {
        x: 6
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        width: (parent.width - 12) * Math.min(Math.max(level, 0), 1)
        height: 3
        radius: 1.5
        visible: level >= 0
        color: levelColor
    }
}
