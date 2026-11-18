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

// Roll and pitch over the last 5 s. On the axis being saved, the shaded band is the spread
// of the last 2 s: it turns green once the reading holds still. A saved axis shows its step
// to the dashed zero line
Item {
    id: trace

    property real roll              // degrees
    property real pitch             // degrees
    property int axis: 0            // 0 roll, 1 pitch, 2 both saved
    property real steadyLimit: 0.2  // peak to peak over the last 2 s
    property bool running: true
    property var history: []        // [roll, pitch] at 10 Hz, newest last

    implicitWidth: 360
    implicitHeight: 140

    // A new run starts empty, and the step from roll to pitch stays as it runs on through both
    onRunningChanged: if (running) history = []

    Timer {
        interval: 100
        repeat: true
        running: trace.running
        onTriggered: trace.history = trace.history.concat([[trace.roll, trace.pitch]]).slice(-50)
    }

    Column {
        anchors.fill: parent
        spacing: 14

        Repeater {
            model: ["Roll", "Pitch"]

            Item {
                id: strip

                readonly property bool active: index === trace.axis
                readonly property bool saved: index < trace.axis
                readonly property var v: trace.history.length ? trace.history.map(p => p[index]) : [0]
                readonly property var last: v.slice(-20)
                readonly property real lastMin: Math.min(...last)
                readonly property real lastMax: Math.max(...last)
                readonly property bool steady: last.length === 20 && lastMax - lastMin <= trace.steadyLimit
                // At least 1° tall, limits on 0.5° steps
                readonly property real vMin: Math.min(...v)
                readonly property real vMax: Math.max(...v)
                readonly property real mid: (vMin + vMax) / 2
                readonly property real half: Math.max(0.5, (vMax - vMin) / 2)
                readonly property real lo: Math.floor((mid - half) * 2) / 2
                readonly property real hi: Math.ceil((mid + half) * 2) / 2
                function toY(val) { return 1 + (hi - val) / (hi - lo) * (height - 2) }

                width: parent.width
                height: (parent.height - parent.spacing) / 2
                opacity: active || saved ? 1 : 0.6

                Column {
                    id: label
                    width: 60
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    Text {
                        text: modelData
                        color: Utility.getAppHexColor(strip.active ? "lightAccent" : "normalText")
                        font.pointSize: 9
                        font.letterSpacing: 0.5
                        font.capitalization: Font.AllUppercase
                    }
                    Text {
                        text: strip.saved ? "saved" : !strip.active ? "next" : strip.steady ? "steady" : "moving"
                        color: Utility.getAppHexColor(strip.saved || (strip.active && strip.steady) ? "positiveText" :
                                                      strip.active ? "warningText" : "normalText")
                        font.pointSize: 12
                        font.weight: strip.active ? Font.Medium : Font.Normal
                    }
                }

                Canvas {
                    id: plot

                    readonly property real lo: strip.lo
                    readonly property real hi: strip.hi

                    anchors.left: label.right
                    anchors.right: scaleLabels.left
                    anchors.rightMargin: 6
                    height: parent.height
                    onLoChanged: requestPaint()
                    onHiChanged: requestPaint()
                    onWidthChanged: requestPaint()
                    Connections { target: trace; function onHistoryChanged() { plot.requestPaint() } }

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        var x = i => width * (i + 50 - strip.v.length) / 49

                        ctx.lineWidth = 1
                        ctx.strokeStyle = Utility.getAppHexColor("raisedBackground")
                        ctx.strokeRect(0.5, 0.5, width - 1, height - 1)
                        // Zero is where a saved axis lands
                        if (lo < 0 && hi > 0) {
                            var z = Math.round(strip.toY(0)) + 0.5
                            ctx.strokeStyle = Utility.getAppHexColor("lightestBackground")
                            ctx.setLineDash([3, 3])
                            ctx.beginPath()
                            ctx.moveTo(0, z)
                            ctx.lineTo(width, z)
                            ctx.stroke()
                            ctx.setLineDash([])
                        }

                        if (strip.active && strip.last.length === 20) {
                            var x0 = x(strip.v.length - 20), top = strip.toY(strip.lastMax)
                            ctx.globalAlpha = 0.25
                            ctx.fillStyle = Utility.getAppHexColor(strip.steady ? "positiveText" : "warningText")
                            ctx.fillRect(x0, top - 2, width - x0, strip.toY(strip.lastMin) - top + 4)
                            ctx.globalAlpha = 1
                        }

                        ctx.lineWidth = 1.5
                        ctx.lineJoin = "round"
                        ctx.strokeStyle = Utility.getAppHexColor(strip.active ? "lightAccent" : "normalText")
                        ctx.beginPath()
                        for (var i = 0; i < strip.v.length; i++)
                            ctx.lineTo(x(i), strip.toY(strip.v[i]))
                        ctx.stroke()
                    }
                }

                // Limits top and bottom, plus the zero line when it is in view
                Item {
                    id: scaleLabels
                    width: 34
                    height: parent.height
                    anchors.right: parent.right

                    Repeater {
                        model: {
                            var t = [strip.hi, strip.lo]
                            if (strip.lo < 0 && strip.hi > 0)
                                t = t.filter(l => Math.abs(strip.toY(l) - strip.toY(0)) > 16).concat([0])
                            return t
                        }

                        Text {
                            y: Math.max(-2, Math.min(parent.height - height + 2, strip.toY(modelData) - height / 2))
                            text: modelData.toFixed(modelData ? 1 : 0) + "°"
                            color: Utility.getAppHexColor(modelData ? "normalText" : "lightText")
                            font.pointSize: 8
                            font.features: { "tnum": 1 }
                        }
                    }
                }
            }
        }
    }
}
