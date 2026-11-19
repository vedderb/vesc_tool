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

// Yaw step, side and top view. Left: how far the nose is lifted against the 25-80° target. Right:
// the IMU X axis and the direction of the lifted nose, seen from above. yawOffset is rot_yaw in
// degrees. As in the firmware, a lifted +X end reads as negative pitch, so rot_yaw turns X' to
// point away from the nose
Item {
    id: view
    property real tilt: 0           // degrees from level
    property real yawOffset: 0
    property bool captured: false

    readonly property bool lifted: tilt > 15
    readonly property bool inTarget: tilt >= 25 && tilt <= 80
    // Lifted nose seen from above, clockwise from the IMU X axis
    readonly property real nose: yawOffset > 0 ? yawOffset - 180 : yawOffset + 180
    readonly property string liftColor: Utility.getAppHexColor(inTarget ? "positiveText" : tilt > 80 ? "warningText" : "lightAccent")

    implicitWidth: 360
    implicitHeight: 140

    // Hidden on the other steps, so it only repaints while shown
    function repaint() {
        if (visible) {
            sideCanvas.requestPaint()
            topCanvas.requestPaint()
        }
    }
    onTiltChanged: repaint()
    onYawOffsetChanged: repaint()
    onCapturedChanged: repaint()
    onVisibleChanged: repaint()

    component Caption: Text {
        color: Utility.getAppHexColor("normalText")
        font.pointSize: 9
        font.letterSpacing: 0.5
        font.capitalization: Font.AllUppercase
    }

    // Side view
    Item {
        id: sidePane
        width: parent.width * 0.5
        height: parent.height

        // Caption doubles as the instruction, so the user knows when to stop
        Caption {
            id: sideCap
            text: view.captured ? "Captured, tap Save" : !view.lifted ? "Lift the front" :
                  view.tilt < 25 ? "Keep lifting" : view.inTarget ? "Hold still" : "Too far"
            color: view.lifted ? view.liftColor : Utility.getAppHexColor("normalText")
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.baseline: sideCap.baseline
            text: view.tilt.toFixed(0) + "°"
            color: view.liftColor
            font.pointSize: 15
            font.weight: Font.Medium
            font.features: { "tnum": 1 }
        }

        Canvas {
            id: sideCanvas
            anchors.fill: parent
            anchors.topMargin: 24
            antialiasing: true
            readonly property real x0: 4
            readonly property real y0: height - 4
            readonly property real len: Math.min(width - 44, height - 8)
            readonly property real shown: Math.min(view.tilt, 90) * Math.PI / 180

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                var rad = a => -a * Math.PI / 180

                // Target wedge 25-80°
                ctx.globalAlpha = 0.15
                ctx.fillStyle = Utility.getAppHexColor("positiveText")
                ctx.beginPath()
                ctx.moveTo(x0, y0)
                ctx.arc(x0, y0, len, rad(25), rad(80), true)
                ctx.closePath()
                ctx.fill()
                ctx.globalAlpha = 1

                // Ground
                ctx.strokeStyle = Utility.getAppHexColor("lightestBackground")
                ctx.lineWidth = 1.5
                ctx.beginPath()
                ctx.moveTo(0, y0)
                ctx.lineTo(width, y0)
                ctx.stroke()

                // Board, tail on the ground, and the lift arc
                ctx.strokeStyle = view.liftColor
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.arc(x0, y0, 30, 0, -shown, true)
                ctx.stroke()

                ctx.lineWidth = 4
                ctx.lineCap = "round"
                ctx.beginPath()
                ctx.moveTo(x0, y0)
                ctx.lineTo(x0 + len * Math.cos(shown), y0 - len * Math.sin(shown))
                ctx.stroke()
            }
        }

        Text {
            x: sideCanvas.x + sideCanvas.x0 + sideCanvas.len * Math.cos(sideCanvas.shown) + 6
            y: sideCanvas.y + sideCanvas.y0 - sideCanvas.len * Math.sin(sideCanvas.shown) - height + 2
            text: "Front"
            color: view.liftColor
            font.pointSize: 9
        }
    }

    // Top view
    Item {
        id: topPane
        anchors.right: parent.right
        width: parent.width * 0.45 - 8
        height: parent.height

        Caption {
            anchors.left: parent.left
            anchors.leftMargin: 8
            text: "Front seen from above"
        }

        Canvas {
            id: topCanvas
            anchors.fill: parent
            anchors.topMargin: 24
            antialiasing: true
            readonly property real r: Math.min(width, height) / 2 - 4
            readonly property real xLen: arcR
            readonly property real arcR: r * 0.55

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.translate(width / 2, height / 2)
                // Arrow along the angle a (deg, clockwise from up) from radius r0 to r1
                var arrow = function(r0, r1, a, color, w) {
                    ctx.save()
                    ctx.rotate(a * Math.PI / 180)
                    ctx.strokeStyle = color
                    ctx.fillStyle = color
                    ctx.lineWidth = w
                    ctx.beginPath()
                    ctx.moveTo(0, -r0)
                    ctx.lineTo(0, -r1 + 6)
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.moveTo(0, -r1)
                    ctx.lineTo(-5, -r1 + 8)
                    ctx.lineTo(5, -r1 + 8)
                    ctx.closePath()
                    ctx.fill()
                    ctx.restore()
                }

                ctx.strokeStyle = Utility.getAppHexColor("raisedBackground")
                ctx.lineWidth = 1.5
                ctx.beginPath()
                ctx.arc(0, 0, r, 0, 2 * Math.PI)
                ctx.stroke()

                // IMU chip with its X axis, ending where the turn to the nose starts
                ctx.strokeStyle = Utility.getAppHexColor("normalText")
                ctx.strokeRect(-7, -7, 14, 14)
                ctx.fillStyle = Utility.getAppHexColor("normalText")
                ctx.beginPath()
                ctx.arc(-3.5, -3.5, 1.5, 0, 2 * Math.PI)
                ctx.fill()
                arrow(9, xLen, 0, Utility.getAppHexColor("normalText"), 1.5)

                if (!view.lifted)
                    return

                // Nose direction outside the arc, and the turn from X to it
                var c = view.liftColor
                ctx.strokeStyle = c
                ctx.lineWidth = 1.5
                ctx.beginPath()
                var a0 = -Math.PI / 2, a1 = a0 + view.nose * Math.PI / 180
                ctx.arc(0, 0, arcR, Math.min(a0, a1), Math.max(a0, a1))
                ctx.stroke()
                arrow(arcR, r + 2, view.nose, c, 2.5)
            }
        }

        // X label on the side away from the arc
        Text {
            x: topCanvas.x + topCanvas.width / 2 + (view.lifted && view.nose > 0 ? -width - 5 : 5)
            y: topCanvas.y + topCanvas.height / 2 - topCanvas.xLen - 1
            text: "X"
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 9
        }
    }
}
