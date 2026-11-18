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
import "GaugeStyle.js" as Style

// Upright battery cell in the gauge housing, with charge and range below it
Item {
    id: battery
    property double value: 0
    property string rangeText: "∞"
    property string rangeUnit: "km"
    property color nibColor: Utility.getAppHexColor("positive")

    property var style: Style.palette(Utility.isDarkMode())
    property real u: width
    property real cellWidth: u * 0.4
    property real cellBottom: height - u * 1.2
    property real cellTop: u * 0.62

    Canvas {
        property real m: u * 0.3
        anchors.fill: parent
        anchors.margins: -m
        onVisibleChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d")
            var x = battery.width / 2 - cellWidth / 2
            ctx.reset()
            ctx.translate(m, m)
            Style.frame(ctx, battery.width, battery.height, u * 0.24, u * 0.05, style)
            ctx.fillStyle = style.groove
            ctx.beginPath()
            ctx.roundedRect(x - cellWidth * 0.12, cellTop - cellWidth * 0.12, cellWidth * 1.24,
                            cellBottom - cellTop + cellWidth * 0.24, cellWidth * 0.16, cellWidth * 0.16)
            ctx.roundedRect(battery.width / 2 - cellWidth * 0.24, cellTop - cellWidth * 0.32,
                            cellWidth * 0.48, cellWidth * 0.16, cellWidth * 0.05, cellWidth * 0.05)
            ctx.fill()
        }
    }

    // Lit cells. Painted again only when the shown percentage or the colour changes
    Canvas {
        property int level: Math.round(Math.min(Math.max(value, 0), 100))
        property color c: nibColor
        anchors.fill: parent
        onLevelChanged: requestPaint()
        onCChanged: requestPaint()
        onVisibleChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d")
            var t = cellWidth * 0.6
            var cells = 10
            var top = cellBottom - (cellBottom - cellTop) * level / 100
            ctx.reset()
            if (level > 0) {
                ctx.strokeStyle = Qt.rgba(c.r, c.g, c.b, Utility.isDarkMode() ? 0.025 : 0.015)
                for (var i = 16; i >= 1; i--) {
                    ctx.beginPath()
                    ctx.lineWidth = t + u * 0.36 * Math.pow(i / 16, 1.6)
                    ctx.moveTo(width / 2, cellBottom)
                    ctx.lineTo(width / 2, top)
                    ctx.stroke()
                }
                ctx.fillStyle = c
                ctx.fillRect(width / 2 - t / 2, top, t, cellBottom - top)
                ctx.fillStyle = Utility.isDarkMode() ? "#80ffffff" : "#50ffffff"
                ctx.fillRect(width / 2 - t * 0.38, top, t * 0.14, cellBottom - top)
            }
            ctx.fillStyle = style.groove
            for (i = 1; i < cells; i++) {
                ctx.fillRect(width / 2 - cellWidth / 2, cellTop + (cellBottom - cellTop) * i / cells - cellWidth * 0.05,
                             cellWidth, cellWidth * 0.1)
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: u * 0.15
        text: qsTr("BATTERY")
        font.pixelSize: u * 0.15
        font.letterSpacing: u * 0.012
        font.family: "Roboto"
        color: Utility.getAppHexColor("normalText")
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        y: cellBottom + u * 0.12

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: u * 0.03

            Text {
                id: levelText
                text: value.toFixed(0)
                font.pixelSize: u * 0.3
                font.weight: Font.Medium
                font.family: "Roboto"
                color: Utility.getAppHexColor("titleText")
            }

            Text {
                anchors.baseline: levelText.baseline
                text: "%"
                font.pixelSize: u * 0.16
                font.family: "Roboto"
                color: Utility.getAppHexColor("normalText")
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("RANGE")
            font.pixelSize: Math.max(8, u * 0.13)
            font.letterSpacing: 0.5
            font.family: "Roboto"
            color: Utility.getAppHexColor("normalText")
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 2

            Text {
                id: rangeValue
                text: rangeText
                font.pixelSize: u * 0.255
                font.weight: Font.Medium
                font.family: "Roboto"
                color: Utility.getAppHexColor("titleText")
            }

            Text {
                anchors.baseline: rangeValue.baseline
                text: rangeUnit
                font.pixelSize: Math.max(9, u * 0.165)
                font.family: "Roboto"
                color: Utility.getAppHexColor("normalText")
            }
        }
    }
}
