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

// How much the gyro moved over the last half second (largest axis) on a log scale, against the
// still limit. The offsets are taken once it stays under the limit for the whole window
Item {
    id: plot
    property real motion: NaN       // °/s
    property real limit: 3          // °/s
    property real countdown: -1     // s until the capture, -1 while moving
    property bool captured: false
    property bool running: true
    property var samples: []        // motion at 10 Hz, newest last

    readonly property real past: 8  // s shown
    readonly property real minV: 0.1
    readonly property real maxV: 1000
    readonly property real padL: 34
    readonly property real padT: 22
    readonly property real plotW: width - padL - 4
    readonly property real plotH: height - padT - 16

    implicitWidth: 360
    implicitHeight: 140

    function xOf(t) { return padL + (t + past) / past * plotW }
    function yOf(v) {
        var l = Math.log(Math.min(Math.max(v, minV), maxV)) / Math.LN10
        var l0 = Math.log(minV) / Math.LN10, l1 = Math.log(maxV) / Math.LN10
        return padT + (l1 - l) / (l1 - l0) * plotH
    }

    onSamplesChanged: canvas.requestPaint()
    onLimitChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    // History restarts each time the page is entered, and clears when the IMU drops out
    onRunningChanged: samples = []

    Timer {
        interval: 100
        repeat: true
        running: plot.running && !isNaN(plot.motion)
        onTriggered: plot.samples = plot.samples.concat([plot.motion]).slice(-plot.past * 10)
    }

    Text {
        text: "Motion (\u00B0/s)"
        color: Utility.getAppHexColor("normalText")
        font.pointSize: 9
    }
    Text {
        anchors.right: parent.right
        text: !plot.running || plot.samples.length === 0 ? "Waiting for data" :
              plot.captured ? "Captured, tap Save" :
              plot.countdown >= 0 ? "Hold still, " + Math.ceil(plot.countdown) + " s" : "Keep it still"
        color: Utility.getAppHexColor(plot.captured ? "positiveText" : "warningText")
        font.pointSize: 9
        font.weight: Font.Medium
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var x0 = plot.padL, x1 = plot.padL + plot.plotW, yb = plot.padT + plot.plotH
            var yl = plot.yOf(plot.limit)

            // Still enough zone
            ctx.fillStyle = Utility.getAppHexColor("positiveText")
            ctx.globalAlpha = 0.12
            ctx.fillRect(x0, yl, plot.plotW, yb - yl)
            ctx.globalAlpha = 1

            ctx.lineWidth = 1
            ctx.strokeStyle = Utility.getAppHexColor("raisedBackground")
            ctx.beginPath()
            ;[1, 10, 100, 1000].forEach(function(v) { ctx.moveTo(x0, plot.yOf(v)); ctx.lineTo(x1, plot.yOf(v)) })
            ctx.moveTo(x0, yb); ctx.lineTo(x1, yb)
            ctx.stroke()

            ctx.strokeStyle = Utility.getAppHexColor("positiveText")
            ctx.beginPath(); ctx.moveTo(x0, yl); ctx.lineTo(x1, yl); ctx.stroke()

            var n = plot.samples.length
            if (n === 0) return
            ctx.strokeStyle = Utility.getAppHexColor("lightAccent")
            ctx.lineWidth = 2
            ctx.lineJoin = "round"
            ctx.beginPath()
            for (var i = 0; i < n; i++) {
                var x = plot.xOf((i - n + 1) / 10), y = plot.yOf(plot.samples[i])
                if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
            }
            ctx.stroke()

            var still = plot.samples[n - 1] < plot.limit
            ctx.fillStyle = Utility.getAppHexColor(plot.captured || still ? "positiveText" : "warningText")
            ctx.beginPath(); ctx.arc(x1 - 4, plot.yOf(plot.samples[n - 1]), 4, 0, 2 * Math.PI); ctx.fill()
        }
    }

    // Axis labels
    Repeater {
        model: [1000, 100, 10, 1]
        Text {
            x: plot.padL - 6 - width
            y: plot.yOf(modelData) - height / 2
            text: modelData
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 8
        }
    }
    Text {
        x: plot.padL + plot.plotW - width - 4
        y: (plot.yOf(plot.limit) + plot.padT + plot.plotH - height) / 2
        text: "still"
        color: Utility.getAppHexColor("positiveText")
        font.pointSize: 8
    }
    Text {
        x: plot.padL
        y: plot.padT + plot.plotH + 2
        text: "-" + plot.past + " s"
        color: Utility.getAppHexColor("normalText")
        font.pointSize: 8
    }
    Text {
        x: plot.padL + plot.plotW - width
        y: plot.padT + plot.plotH + 2
        text: "now"
        color: Utility.getAppHexColor("normalText")
        font.pointSize: 8
    }
}
