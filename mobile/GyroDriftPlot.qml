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

// How much the filtered gyro offsets changed over the last second (largest axis), on a log scale.
// The offsets are a 0.99 / 0.01 low-pass per IMU reply, so once the IMU is still the change decays
// along a straight line on this scale and the dashed line predicts when it drops below the limit
Item {
    id: plot
    property real gyroX
    property real gyroY
    property real gyroZ
    property bool running: true
    property var samples: []                // [x, y, z] at 10 Hz, newest last
    property real limit: 0.02               // °/s per second
    readonly property real past: 8          // s shown before now
    readonly property real future: 4        // s shown after now

    readonly property real now: change(samples.length - 1)
    readonly property bool settled: samples.length > 10 && now < limit
    // Measured decay per half second, as the reply rate depends on the link. NaN unless decaying
    readonly property real ratio: decayRatio()
    readonly property real tau: -0.5 / Math.log(ratio)
    readonly property real eta: !settled && ratio > 0 ? tau * Math.log(now / limit) : -1

    readonly property real minV: 0.005
    readonly property real maxV: 10
    readonly property real padL: 34
    readonly property real padT: 22
    readonly property real plotW: width - padL - 4
    readonly property real plotH: height - padT - 16

    implicitWidth: 360
    implicitHeight: 140

    // Largest spread of any axis over the second up to sample i
    function change(i) {
        if (i < 10) return NaN
        var r = 0
        for (var k = 0; k < 3; k++) {
            var w = samples.slice(i - 10, i + 1).map(function(s) { return s[k] })
            r = Math.max(r, Math.max.apply(null, w) - Math.min.apply(null, w))
        }
        return r
    }
    // A still IMU's change shrinks by the same ratio every half second, 0.78 at 50 replies per
    // second. Only predict when the last 2 s follow that, so a moving IMU gets no countdown. Near
    // the limit sensor noise breaks the pattern, so the status falls back to "Almost still" there
    function decayRatio() {
        var n = samples.length - 1, p = 1
        for (var j = 0; j < 4; j++) {
            var r = change(n - 5 * j) / change(n - 5 * j - 5)
            if (!(r > 0.6 && r < 0.92)) return NaN
            p *= r
        }
        return Math.pow(p, 0.25)
    }
    function xOf(t) { return padL + (t + past) / (past + future) * plotW }
    function yOf(v) {
        var l = Math.log(Math.min(Math.max(v, minV), maxV)) / Math.LN10
        var l0 = Math.log(minV) / Math.LN10, l1 = Math.log(maxV) / Math.LN10
        return padT + (l1 - l) / (l1 - l0) * plotH
    }

    onSamplesChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    // History restarts each time the page is entered, and clears when the IMU drops out
    onRunningChanged: samples = []

    Timer {
        interval: 100
        repeat: true
        running: plot.running
        onTriggered: plot.samples = plot.samples.concat([[plot.gyroX, plot.gyroY, plot.gyroZ]]).slice(-120)
    }

    Text {
        text: "Offset change in 1 s (\u00B0/s)"
        color: Utility.getAppHexColor("normalText")
        font.pointSize: 9
    }
    Text {
        anchors.right: parent.right
        text: plot.samples.length <= 10 ? "Waiting for data" :
              plot.settled ? "Still, ready to save" :
              plot.eta > 0 ? "Ready in ~" + Math.ceil(plot.eta) + " s" :
              plot.now < 4 * plot.limit ? "Almost still" : "Keep it still"
        color: Utility.getAppHexColor(plot.settled ? "positiveText" : "warningText")
        font.pointSize: 9
        font.weight: Font.Medium
        font.features: { "tnum": 1 }
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
            ;[0.1, 1, 10].forEach(function(v) { ctx.moveTo(x0, plot.yOf(v)); ctx.lineTo(x1, plot.yOf(v)) })
            ctx.moveTo(x0, yb); ctx.lineTo(x1, yb)
            ctx.moveTo(plot.xOf(0), plot.padT); ctx.lineTo(plot.xOf(0), yb)
            ctx.stroke()

            ctx.strokeStyle = Utility.getAppHexColor("positiveText")
            ctx.beginPath(); ctx.moveTo(x0, yl); ctx.lineTo(x1, yl); ctx.stroke()

            // History
            var n = plot.samples.length
            ctx.strokeStyle = Utility.getAppHexColor("lightAccent")
            ctx.lineWidth = 2
            ctx.lineJoin = "round"
            ctx.beginPath()
            var first = true
            for (var i = Math.max(10, n - plot.past * 10 - 1); i < n; i++) {
                var x = plot.xOf((i - n + 1) / 10), y = plot.yOf(plot.change(i))
                if (first) ctx.moveTo(x, y); else ctx.lineTo(x, y)
                first = false
            }
            ctx.stroke()
            if (n <= 10) return

            // Prediction if kept still
            var yn = plot.yOf(plot.now)
            if (plot.eta > 0) {
                var te = Math.min(plot.eta, plot.future)
                ctx.setLineDash([3, 3])
                ctx.lineWidth = 1.5
                ctx.beginPath()
                ctx.moveTo(plot.xOf(0), yn)
                ctx.lineTo(plot.xOf(te), plot.yOf(plot.now * Math.exp(-te / plot.tau)))
                ctx.stroke()
                ctx.setLineDash([])
            }

            ctx.fillStyle = Utility.getAppHexColor(plot.settled ? "positiveText" : "warningText")
            ctx.beginPath(); ctx.arc(plot.xOf(0), yn, 4, 0, 2 * Math.PI); ctx.fill()
        }
    }

    // Axis labels
    Repeater {
        model: [10, 1, 0.1]
        Text {
            x: plot.padL - 6 - width
            y: plot.yOf(modelData) - height / 2
            text: modelData
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 8
        }
    }
    Text {
        x: plot.padL - 6 - width
        y: plot.yOf(plot.limit) - height / 2
        text: plot.limit
        color: Utility.getAppHexColor("positiveText")
        font.pointSize: 8
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
        x: plot.xOf(0) - width / 2
        y: plot.padT + plot.plotH + 2
        text: "now"
        color: Utility.getAppHexColor("normalText")
        font.pointSize: 8
    }
    Text {
        x: plot.padL + plot.plotW - width
        y: plot.padT + plot.plotH + 2
        text: "+" + plot.future + " s"
        color: Utility.getAppHexColor("normalText")
        font.pointSize: 8
    }
}
