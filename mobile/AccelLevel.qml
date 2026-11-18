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

// Bubble level seen from above, looking down the axis being calibrated. The dot is
// gravity's share on the other two axes and points at the high side; it sits in the
// middle when the axis points straight up, which is where the axis reads its 1 g peak.
// Done = dot inside the 3 deg ring, the max has risen less than 0.5 mg for 2 s and the
// reading is still at the max, so a bump that overshot it is not taken
Item {
    id: lvl
    property int axis: 0            // 0..2 = X, Y, Z
    property var acc: [0, 0, 1]     // live accel x, y, z in g
    property real max: -10          // running max of the axis, -10 = cleared
    property bool running: true

    // Right-handed: with the axis toward the viewer, b points right and c up
    readonly property int b: (axis + 1) % 3
    readonly property int c: (axis + 2) % 3
    readonly property real tilt: Math.atan2(Math.hypot(acc[b], acc[c]), acc[axis]) * 180 / Math.PI
    // All zeros when the IMU is off, which would otherwise read as level
    readonly property bool hasData: Math.hypot(acc[0], acc[1], acc[2]) > 0.5
    readonly property bool level: hasData && tilt < 3
    readonly property bool overshot: max - acc[axis] > 0.002
    readonly property bool done: level && held >= 2 && !overshot
    readonly property string hint: {
        if (tilt > 45)
            return "Raise the +" + "XYZ"[axis] + " side"
        var i = Math.abs(acc[b]) > Math.abs(acc[c]) ? b : c
        return "Lower the " + (acc[i] > 0 ? "+" : "\u2212") + "XYZ"[i] + " side"
    }

    // Seconds since the max last rose by more than 0.5 mg
    property real held: 0
    property real peak: -10
    onMaxChanged: if (max < peak || max > peak + 0.0005) { peak = max; held = 0 }
    Timer { interval: 100; repeat: true; running: lvl.running && lvl.max > -10; onTriggered: lvl.held += 0.1 }
    // Square root spreads out the last few degrees, where the reading barely changes
    function ringR(deg) { return (dial.width / 2 - 24) * Math.sqrt(Math.min(deg, 90) / 90) }

    implicitWidth: 360
    implicitHeight: 150

    Item {
        id: dial
        width: Math.min(lvl.height, lvl.width * 0.45)
        height: width
        anchors.verticalCenter: parent.verticalCenter

        Repeater {
            model: [90, 45, 3]

            Rectangle {
                readonly property bool target: modelData === 3
                // The dot fits inside the target ring exactly when the tilt is under 3 deg
                width: (lvl.ringR(modelData) + (target ? dot.width / 2 + 1 : 0)) * 2
                height: width
                radius: width / 2
                anchors.centerIn: parent
                color: "transparent"
                border.width: target ? 2 : 1.5
                border.color: Utility.getAppHexColor(target && lvl.level ? "positiveText" : target ? "normalText" : "raisedBackground")
            }
        }
        Rectangle { anchors.centerIn: parent; width: lvl.ringR(90) * 2; height: 1; color: Utility.getAppHexColor("raisedBackground") }
        Rectangle { anchors.centerIn: parent; width: 1; height: lvl.ringR(90) * 2; color: Utility.getAppHexColor("raisedBackground") }

        Text {
            x: parent.width / 2 + lvl.ringR(45) * 0.71 + 2
            y: parent.height / 2 - lvl.ringR(45) * 0.71 - height + 2
            text: "45°"
            color: Utility.getAppHexColor("disabledText")
            font.pointSize: 8
        }
        // Outside the rim, clear of the dot when it parks there
        Text {
            x: parent.width / 2 + lvl.ringR(90) + 9
            anchors.verticalCenter: parent.verticalCenter
            text: "+" + "XYZ"[lvl.b]
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 9
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height / 2 - lvl.ringR(90) - height - 8
            text: "+" + "XYZ"[lvl.c]
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 9
        }

        // Past 90° the axis points down and the dot parks on the rim, hollow
        Rectangle {
            id: dot
            visible: lvl.hasData
            readonly property real a: Math.atan2(lvl.acc[lvl.c], lvl.acc[lvl.b])
            readonly property real r: lvl.ringR(lvl.tilt)
            x: parent.width / 2 + r * Math.cos(a) - width / 2
            y: parent.height / 2 - r * Math.sin(a) - height / 2
            width: 12
            height: 12
            radius: 6
            color: lvl.tilt > 90 ? Utility.getAppHexColor("cardBackground") : border.color
            border.width: 2
            border.color: Utility.getAppHexColor(lvl.level ? "positiveText" : "lightAccent")
        }
    }

    Column {
        anchors.left: dial.right
        anchors.right: parent.right
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        Row {
            spacing: 6
            Text {
                id: tiltText
                text: lvl.hasData ? lvl.tilt.toFixed(1) + "°" : "\u2013"
                color: Utility.getAppHexColor(lvl.level ? "positiveText" : "titleText")
                font.pointSize: 15
                font.weight: Font.Medium
                font.features: { "tnum": 1 }
            }
            Text {
                anchors.baseline: tiltText.baseline
                text: "from vertical"
                color: Utility.getAppHexColor("normalText")
                font.pointSize: 10
            }
        }
        Text {
            width: parent.width
            text: !lvl.hasData ? "No IMU data" : lvl.done ? "Done, Max has stopped rising" :
                  lvl.level && lvl.held >= 2 ? "Max overshot, tap Clear Max" : lvl.level ? "Centered, hold still" : lvl.hint
            color: Utility.getAppHexColor(lvl.done ? "positiveText" :
                                          !lvl.hasData || (lvl.level && lvl.held >= 2) ? "warningText" : "lightText")
            font.pointSize: 11
            wrapMode: Text.Wrap
        }
        Text {
            width: parent.width
            // Save stores max - 1 g as the offset
            wrapMode: Text.Wrap
            text: lvl.max <= -10 ? "Expect about 1 g at the top"
                  : "Max " + lvl.max.toFixed(3) + " g" + (lvl.done ? ", offset " + (lvl.max >= 1 ? "+" : "\u2212") + Math.abs(lvl.max - 1).toFixed(3) + " g" : "")
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 10
            font.features: { "tnum": 1 }
        }
    }
}
