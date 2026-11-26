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

// Bubble level seen from above, looking along whichever axis is closest to vertical. The dot is
// gravity's share on the other two axes and points at the high side. Once that axis has been
// within 2° of vertical and steady for 2 s, the average reading is captured for that side. A
// captured side keeps its value
Item {
    id: lvl
    property var acc: [0, 0, 1]     // live accel x, y, z in g
    property bool running: true
    property var faces: [[NaN, NaN], [NaN, NaN], [NaN, NaN]]  // captured [up, down] per axis
    signal captured(int axis, int sign, real value)

    // All zeros when the IMU is off, which would otherwise read as level
    readonly property bool hasData: Math.hypot(acc[0], acc[1], acc[2]) > 0.5
    readonly property int axis: {
        var a = acc.map(Math.abs)
        return a[0] >= a[1] && a[0] >= a[2] ? 0 : a[1] >= a[2] ? 1 : 2
    }
    readonly property int sign: acc[axis] >= 0 ? 1 : -1
    // Right-handed: with the axis toward the viewer, b points right and c up
    readonly property int b: (axis + 1) % 3
    readonly property int c: (axis + 2) % 3
    readonly property real tilt: Math.atan2(Math.hypot(acc[b], acc[c]), Math.abs(acc[axis])) * 180 / Math.PI
    readonly property bool level: hasData && tilt < 2
    readonly property string face: (sign > 0 ? "+" : "\u2212") + "XYZ"[axis]
    readonly property string hint: {
        var i = Math.abs(acc[b]) > Math.abs(acc[c]) ? b : c
        return "Lower the " + (acc[i] > 0 ? "+" : "\u2212") + "XYZ"[i] + " side"
    }

    readonly property real value: faces[axis][sign > 0 ? 0 : 1]
    readonly property bool steady: !isNaN(value)
    readonly property bool allAxes: faces.every(f => !isNaN(f[0]) || !isNaN(f[1]))

    // Readings of the vertical axis at 10 Hz while it stays level and still on a side that is not
    // captured yet. Moving starts it over, so it only counts up
    property var history: []
    readonly property real progress: history.length / 20
    Timer {
        interval: 100
        repeat: true
        running: lvl.running
        onTriggered: {
            var x = lvl.acc[lvl.axis]
            var h = lvl.level && !lvl.steady && lvl.history.length && lvl.history[0][0] === (lvl.axis + 1) * lvl.sign &&
                    Math.abs(x - lvl.history[0][1]) < 0.003 ? lvl.history : []
            h = lvl.level && !lvl.steady ? h.concat([[(lvl.axis + 1) * lvl.sign, x]]) : []
            if (h.length === 20) {
                lvl.captured(lvl.axis, lvl.sign, h.reduce((s, v) => s + v[1], 0) / 20)
                h = []
            }
            lvl.history = h
        }
    }
    onRunningChanged: history = []

    // Square root spreads out the last few degrees, where the reading barely changes
    function ringR(deg) { return (dial.width / 2 - 24) * Math.sqrt(Math.min(deg, 60) / 60) }

    implicitWidth: 360
    implicitHeight: 150

    Item {
        id: dial
        width: Math.min(lvl.height, lvl.width * 0.45)
        height: width
        anchors.verticalCenter: parent.verticalCenter

        Repeater {
            model: [60, 20, 2]

            Rectangle {
                readonly property bool target: modelData === 2
                // The dot fits inside the target ring exactly when the tilt is under 2 deg
                width: (lvl.ringR(modelData) + (target ? dot.width / 2 + 1 : 0)) * 2
                height: width
                radius: width / 2
                anchors.centerIn: parent
                color: "transparent"
                border.width: target ? 2 : 1.5
                border.color: Utility.getAppHexColor(target && (lvl.level || lvl.steady) ? "positiveText" : target ? "normalText" : "raisedBackground")
            }
        }
        Rectangle {
            anchors.centerIn: parent
            width: lvl.ringR(60) * 2
            height: 1
            color: Utility.getAppHexColor("raisedBackground")
        }
        Rectangle {
            anchors.centerIn: parent
            width: 1
            height: lvl.ringR(60) * 2
            color: Utility.getAppHexColor("raisedBackground")
        }

        Text {
            x: parent.width / 2 + lvl.ringR(20) * 0.71 + 2
            y: parent.height / 2 - lvl.ringR(20) * 0.71 - height + 2
            text: "20°"
            color: Utility.getAppHexColor("disabledText")
            font.pointSize: 8
        }
        // Outside the rim, clear of the dot when it parks there. Seen from above an axis that
        // points down, c points down
        Text {
            x: parent.width / 2 + lvl.ringR(60) + 9
            anchors.verticalCenter: parent.verticalCenter
            visible: lvl.hasData
            text: "+" + "XYZ"[lvl.b]
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 9
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height / 2 - lvl.sign * (lvl.ringR(60) + 8) - (lvl.sign > 0 ? height : 0)
            visible: lvl.hasData
            text: "+" + "XYZ"[lvl.c]
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 9
        }

        Rectangle {
            id: dot
            readonly property real h: Math.max(1e-6, Math.hypot(lvl.acc[lvl.b], lvl.acc[lvl.c]))
            readonly property real r: lvl.ringR(lvl.tilt)
            visible: lvl.hasData
            x: parent.width / 2 + r * lvl.acc[lvl.b] / h - width / 2
            y: parent.height / 2 - lvl.sign * r * lvl.acc[lvl.c] / h - height / 2
            width: 12
            height: 12
            radius: 6
            color: border.color
            border.width: 2
            // Green while level, and on a side that is captured already
            border.color: Utility.getAppHexColor(lvl.level || lvl.steady ? "positiveText" : "lightAccent")
        }
    }

    Column {
        anchors.left: dial.right
        anchors.right: parent.right
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Text {
            text: lvl.hasData ? lvl.face + " up" : "No IMU data"
            color: Utility.getAppHexColor(lvl.hasData ? "normalText" : "warningText")
            font.pointSize: 11
        }
        Text {
            visible: lvl.hasData
            // The result, so it is the largest text here
            text: (lvl.steady ? lvl.value : lvl.acc[lvl.axis]).toFixed(3) + " g"
            color: Utility.getAppHexColor(lvl.steady ? "positiveText" : "titleText")
            font.pointSize: 22
            font.weight: Font.Medium
        }
        Text {
            width: parent.width
            visible: lvl.hasData
            text: lvl.steady ? (lvl.allAxes ? "Captured, all axes done" : "Captured, turn to another side") :
                  lvl.level ? "Hold still, " + ((20 - lvl.history.length) / 10).toFixed(1) + " s" : lvl.hint + ", " + lvl.tilt.toFixed(1) + "° off"
            color: Utility.getAppHexColor(lvl.steady ? "positiveText" : "lightText")
            font.pointSize: 11
            wrapMode: Text.Wrap
        }
        // How long it has been held, it fills in 2 s
        Rectangle {
            visible: lvl.hasData && lvl.level && !lvl.steady
            width: parent.width
            height: 4
            radius: 2
            color: Utility.getAppHexColor("raisedBackground")

            Rectangle {
                width: parent.width * lvl.progress
                height: parent.height
                radius: 2
                color: Utility.getAppHexColor("positiveText")
            }
        }
        // The six sides, green once captured, the one facing up in bold
        Row {
            visible: lvl.hasData
            spacing: 8

            Repeater {
                model: 6

                Text {
                    readonly property int ax: Math.floor(index / 2)
                    readonly property int sg: index % 2 === 0 ? 1 : -1
                    text: (sg > 0 ? "+" : "\u2212") + "XYZ"[ax]
                    color: Utility.getAppHexColor(!isNaN(lvl.faces[ax][sg > 0 ? 0 : 1]) ? "positiveText" : "normalText")
                    font.pointSize: 10
                    font.weight: ax === lvl.axis && sg === lvl.sign ? Font.Bold : Font.Normal
                }
            }
        }
    }
}
