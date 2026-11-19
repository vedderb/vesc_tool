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
import QtQuick.Layouts
import Vedder.vesc.utility 1.0

// What level means for common vehicles, side views with the level part in green. With lift,
// the same vehicles with the front lifted 45° for the yaw step
GridLayout {
    id: examples
    property bool lift: false
    columns: width > 340 ? 4 : 2
    columnSpacing: 8
    rowSpacing: 8

    Repeater {
        model: examples.lift ? [["Balance board", "Front up, tail down"],
                                ["Unicycle", "Tilted back, pedals front up"],
                                ["Skateboard", "Nose up, rear wheels down"],
                                ["Bike or scooter", "Front up, rear wheel down"]] :
                               [["Balance board", "Footpads level, low end raised"],
                                ["Unicycle", "Upright, pedals level"],
                                ["Skateboard", "On its wheels"],
                                ["Bike or scooter", "Upright on both wheels"]]

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            spacing: 2

            Canvas {
                // Taller when lifted, so that the front fits
                readonly property real boxH: examples.lift ? 64 : 48
                Layout.fillWidth: true
                Layout.preferredHeight: boxH
                onWidthChanged: requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    // Drawn in a 100 x boxH box, centred
                    var s = Math.min(width / 100, 1)
                    ctx.translate((width - 100 * s) / 2, 0)
                    ctx.scale(s, s)
                    var grey = Utility.getAppHexColor("normalText"), green = Utility.getAppHexColor("positiveText")
                    var line = (c, w, pts) => {
                        ctx.strokeStyle = c
                        ctx.lineWidth = w
                        ctx.beginPath()
                        ctx.moveTo(pts[0], pts[1])
                        for (var i = 2; i < pts.length; i += 2) ctx.lineTo(pts[i], pts[i + 1])
                        ctx.stroke()
                    }
                    var wheel = (x, y, r) => {
                        ctx.strokeStyle = grey
                        ctx.lineWidth = 2
                        ctx.beginPath()
                        ctx.arc(x, y, r, 0, 2 * Math.PI)
                        ctx.stroke()
                    }
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    var g = boxH - 2
                    // Ground
                    line(Utility.getAppHexColor("lightestBackground"), 1.5, [4, g, 96, g])

                    if (examples.lift) {
                        // Front to the right, lifted 45° on the end or wheels that stay on the ground.
                        // d runs along the vehicle toward the front, u is up from it
                        var c = Math.SQRT1_2, d = [c, -c], u = [-c, -c]
                        var at = (p, a, b) => [p[0] + a * d[0] + b * u[0], p[1] + a * d[1] + b * u[1]]
                        var seg = (col, w, pts) => line(col, w, [].concat(...pts))
                        if (index === 0) {
                            // Resting on the end of the deck, the wheel in the middle of it
                            var tail = [14, g]
                            wheel(...at(tail, 40, 0), 13)
                            seg(green, 3, [tail, at(tail, 80, 0)])
                        } else if (index === 1) {
                            // Rolled on the tire, the shell and pedals tilt back
                            var hub1 = [50, g - 20]
                            wheel(hub1[0], hub1[1], 20)
                            seg(grey, 2, [at(hub1, -6, 0), at(hub1, -6, 30), at(hub1, 6, 30), at(hub1, 6, 0)])
                            seg(green, 3, [at(hub1, -18, -4), at(hub1, 18, -4)])
                        } else if (index === 2) {
                            // Rear wheels on the ground, nose up
                            var rear = [30, g - 5]
                            wheel(rear[0], rear[1], 5)
                            var deck0 = at(rear, -6, 7)
                            wheel(...at(rear, 44, 0), 5)
                            seg(green, 3, [deck0, at(deck0, 62, 0)])
                        } else {
                            // A wheelie on the rear wheel
                            var axle = [22, g - 12]
                            wheel(axle[0], axle[1], 12)
                            var front = at(axle, 46, 0)
                            wheel(front[0], front[1], 12)
                            seg(grey, 2, [axle, at(axle, 16, 14), at(axle, 36, 14), front])
                            seg(grey, 2, [at(axle, 34, 14), at(axle, 36, 24)])
                            seg(green, 3, [axle, front])
                        }
                    } else if (index === 0) {
                        // Resting on one end, faint, then lifted to level
                        wheel(50, 34, 12)
                        ctx.globalAlpha = 0.35
                        line(grey, 3, [14, 46, 86, 22])
                        ctx.globalAlpha = 1
                        line(green, 3, [14, 34, 86, 34])
                        line(green, 1.5, [14, 44, 14, 38])
                        line(green, 1.5, [11, 41, 14, 37, 17, 41])
                    } else if (index === 1) {
                        wheel(50, 26, 20)
                        line(grey, 2, [44, 2, 56, 2, 56, 26, 44, 26, 44, 2])
                        line(green, 3, [30, 30, 70, 30])
                        line(green, 1.5, [50, 0, 50, 44])
                    } else if (index === 2) {
                        wheel(30, 41, 5)
                        wheel(74, 41, 5)
                        line(green, 3, [16, 34, 88, 34])
                    } else {
                        wheel(24, 34, 12)
                        wheel(76, 34, 12)
                        line(grey, 2, [24, 34, 44, 18, 66, 18, 76, 34])
                        line(grey, 2, [62, 18, 60, 8])
                        line(green, 3, [24, 34, 76, 34])
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: modelData[0]
                color: Utility.getAppHexColor("lightText")
                font.pointSize: 9
                font.weight: Font.Medium
            }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: modelData[1]
                color: Utility.getAppHexColor("normalText")
                font.pointSize: 8
            }
        }
    }
}
