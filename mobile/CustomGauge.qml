/*
    Copyright 2018 - 2019 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Layouts 1.3
import QtQuick.Shapes
import Qt5Compat.GraphicalEffects
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0
import "GaugeStyle.js" as Style

Item {
    property double value: 0
    property double outerRadius: width/2
    height: width
    property double labelStep: 10
    property double labelInset: outerRadius * 0.28 + outerRadius*0.06
    property double tickmarkInset: outerRadius * 0.125
    property double minorTickmarkInset: outerRadius * 0.125
    property double minAngle: -140
    property double maxAngle: 140
    property int isInverted: maxAngle > minAngle ? 1 : -1

    property real angleRange: maxAngle - minAngle
    property int tickmarkCount: Math.min(100, Math.floor(labelStep > 0 ? (maximumValue - minimumValue) / labelStep + 1 : 0))
    property int minorTickmarkCount:4
    property double minimumValue: 0
    property double maximumValue: 100

    property string unitText: ""
    property string typeText: ""
    property string tickmarkSuffix: ""
    property double tickmarkScale: 1
    property color nibColor: {nibColor = Utility.getAppHexColor("lightAccent")}
    property color traceColor: nibColor
    property double precision: 0

    property color coveredColor: {coveredColor = Utility.getAppHexColor("titleText")}
    property color uncoveredColor: {uncoveredColor = Utility.getAppHexColor("disabledText")}

    property bool tickmarksVisible: true
    property bool centerTextVisible: true

    // Housing colours, inner edge of the bezel and the channel the lit bar runs in
    property var style: Style.palette(Utility.isDarkMode())
    property real bezelInner: outerRadius * 0.95
    property real channelWidth: outerRadius * 0.052
    property real channelRadius: bezelInner - outerRadius * 0.004 - channelWidth / 2

    onTickmarkCountChanged: valueTextModel.update()

    function valueToAngle(value) {
        var normalised = (value - minimumValue) / (maximumValue - minimumValue);
        normalised = normalised > 1 ? 1 : normalised < 0 ? 0 : normalised
        return (maxAngle - minAngle) * normalised + minAngle;
    }
    function d2r(degrees) {
        return degrees * (Math.PI / 180.0);
    }
    function r2d(radians) {
        return radians * (180.0 / Math.PI);
    }
    function isCovered(value) {
        if (gauge.value > 0) {
            if (value <= gauge.value && value >= 0) {
                return true
            }
        } else {
            if (value >= gauge.value && value <= 0) {
                return true
            }
        }
        return false
    }

    Item {
        id: gauge
        x: outerRadius - width/2
        y: outerRadius - height/2
        width: outerRadius*2
        height: outerRadius*2
        property double value: parent.value
        property real zeroAngle: valueToAngle(0)
        property real sweep: needleTransform.angle - zeroAngle
        Behavior on value {
            NumberAnimation {
                easing.type: Easing.OutCirc
                duration: 100
            }
        }

        // Shadow, bezel, face and channel. Painted again only when the size or range changes
        Canvas {
            property real m: outerRadius * 0.12
            property string range: minAngle + " " + maxAngle
            anchors.fill: parent
            anchors.margins: -m
            onRangeChanged: requestPaint()
            // Canvas does not repaint on resizes while hidden, such as in a page that is still loading
            onVisibleChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                var R = outerRadius
                ctx.reset()
                ctx.translate(m, m)
                Style.shadow(ctx, 0, 0, 2 * R, 2 * R, R, R * 0.07, style)
                ctx.beginPath()
                ctx.fillStyle = Style.metal(ctx, R, R, d2r(135), style)
                ctx.arc(R, R, R, 0, 2 * Math.PI)
                ctx.fill()
                ctx.beginPath()
                ctx.fillStyle = Style.metal(ctx, R, R, d2r(315), style)
                ctx.arc(R, R, bezelInner + R * 0.016, 0, 2 * Math.PI)
                ctx.fill()
                ctx.strokeStyle = style.line
                ctx.lineWidth = Math.max(1, R * 0.005)
                ctx.stroke()
                var face = ctx.createRadialGradient(R, R, R * 0.2, R, R, bezelInner)
                face.addColorStop(0, style.face0)
                face.addColorStop(1, style.face1)
                ctx.beginPath()
                ctx.fillStyle = face
                ctx.arc(R, R, bezelInner, 0, 2 * Math.PI)
                ctx.fill()
                ctx.clip()
                ctx.beginPath()
                ctx.lineCap = "round"
                ctx.strokeStyle = style.groove
                ctx.lineWidth = channelWidth
                ctx.arc(R, R, channelRadius, d2r(minAngle - 90), d2r(maxAngle - 90), isInverted < 0)
                ctx.stroke()
                // Shadow of the bezel lip on the face
                ctx.strokeStyle = Qt.rgba(0, 0, 0, style.shadow / 12)
                for (var i = 1; i <= 5; i++) {
                    ctx.beginPath()
                    ctx.lineWidth = R * 0.016 * i
                    ctx.arc(R, R + R * 0.008 * i, bezelInner + R * 0.008 * i, 0, 2 * Math.PI)
                    ctx.stroke()
                }
            }
        }

        // Unlit channel, in a faint tint of the trace colour
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                fillColor: "transparent"
                strokeColor: Qt.rgba(traceColor.r, traceColor.g, traceColor.b, Utility.isDarkMode() ? 0.13 : 0.22)
                strokeWidth: channelWidth * 0.6
                capStyle: ShapePath.RoundCap
                PathAngleArc {
                    centerX: outerRadius
                    centerY: outerRadius
                    radiusX: channelRadius
                    radiusY: channelRadius
                    startAngle: minAngle - 90
                    sweepAngle: angleRange
                }
            }
        }

        // Glow of the lit bar over the full range, as an alpha mask. Painted once; the
        // gradient below colours it and shows only the part between zero and the value.
        Canvas {
            id: channelGlow
            property string range: minAngle + " " + maxAngle
            anchors.fill: parent
            visible: false
            onRangeChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                var R = outerRadius
                var a0 = d2r(minAngle - 90)
                var a1 = d2r(maxAngle - 90)
                ctx.reset()
                ctx.lineCap = "round"
                // Light on the bezel wall
                ctx.beginPath()
                ctx.strokeStyle = Qt.rgba(1, 1, 1, Utility.isDarkMode() ? 0.3 : 0.2)
                ctx.lineWidth = R * 0.016
                ctx.arc(R, R, bezelInner + R * 0.008, a0, a1, isInverted < 0)
                ctx.stroke()
                ctx.beginPath()
                ctx.arc(R, R, bezelInner, 0, 2 * Math.PI)
                ctx.clip()
                ctx.strokeStyle = Qt.rgba(1, 1, 1, Utility.isDarkMode() ? 0.016 : 0.009)
                for (var i = 28; i >= 1; i--) {
                    ctx.beginPath()
                    ctx.lineWidth = channelWidth + R * 0.34 * Math.pow(i / 28, 1.5)
                    ctx.arc(R, R, channelRadius, a0, a1, isInverted < 0)
                    ctx.stroke()
                }
            }
        }

        ConicalGradient {
            id: glowMask
            property real span: Math.abs(gauge.sweep) / 360
            anchors.fill: parent
            source: channelGlow
            visible: gauge.sweep !== 0
            angle: Math.min(gauge.zeroAngle, needleTransform.angle) - 4
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.rgba(traceColor.r, traceColor.g, traceColor.b, 0) }
                GradientStop { position: 4 / 360; color: traceColor }
                GradientStop { position: Math.min(glowMask.span + 4 / 360, 1); color: traceColor }
                GradientStop { position: Math.min(glowMask.span + 10 / 360, 1); color: Qt.rgba(traceColor.r, traceColor.g, traceColor.b, 0) }
            }
        }

        // Lit bar with a bright core, from zero to the value
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            visible: gauge.sweep !== 0
            ShapePath {
                fillColor: "transparent"
                strokeColor: traceColor
                strokeWidth: channelWidth * 0.72
                capStyle: ShapePath.RoundCap
                PathAngleArc {
                    centerX: outerRadius
                    centerY: outerRadius
                    radiusX: channelRadius
                    radiusY: channelRadius
                    startAngle: gauge.zeroAngle - 90
                    sweepAngle: gauge.sweep
                }
            }
            ShapePath {
                fillColor: "transparent"
                strokeColor: Utility.isDarkMode() ? "#99ffffff" : "#55ffffff"
                strokeWidth: channelWidth * 0.22
                capStyle: ShapePath.RoundCap
                PathAngleArc {
                    centerX: outerRadius
                    centerY: outerRadius
                    radiusX: channelRadius
                    radiusY: channelRadius
                    startAngle: gauge.zeroAngle - 90
                    sweepAngle: gauge.sweep
                }
            }
        }

        Text {
            id: speedLabel
            anchors.centerIn: parent
            visible: centerTextVisible
            text: gauge.value.toFixed(precision)
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: outerRadius * 0.34
            font.weight: Font.Medium
            color: {color = Utility.getAppHexColor("titleText") }
            antialiasing: true
            font.family: "Roboto"
        }
        Text {
            id: speedLabelUnit
            text: unitText
            visible: centerTextVisible
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: speedLabel.bottom
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: outerRadius * 0.105
            font.letterSpacing: outerRadius * 0.008
            color: {color = Utility.getAppHexColor("normalText")}
            antialiasing: true
            font.family: "Roboto"
            font.capitalization: Font.AllUppercase
        }

        Text {
            id: typeLabel
            text: typeText
            visible: centerTextVisible
            verticalAlignment: Text.AlignVCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: speedLabel.top
            //anchors.bottomMargin: outerRadius * 0.1
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: outerRadius * 0.105
            font.letterSpacing: outerRadius * 0.008
            color: {color = Utility.getAppHexColor("normalText")}
            antialiasing: true
            font.family: "Roboto"
            font.capitalization: Font.AllUppercase
        }

        // Needle, rotated about the gauge centre
        Item {
            id: needle
            x: outerRadius - width/2
            z:2
            height: outerRadius * 0.5
            width: outerRadius * 0.4
            transform: Rotation {
                id:needleTransform
                origin.x: needle.width/2
                origin.y: outerRadius
                angle: valueToAngle(gauge.value)
            }
            Canvas {
                property color glow: traceColor
                anchors.fill:parent
                onGlowChanged: requestPaint()
                onVisibleChanged: requestPaint()
                onPaint:{
                    var ctx = getContext("2d")
                    var R = outerRadius
                    var cx = width / 2, y0 = R - bezelInner, a = R * 0.056, h = R * 0.29, c = glow, dark = Utility.isDarkMode()
                    function half(s) {
                        ctx.beginPath()
                        ctx.moveTo(cx, y0)
                        ctx.lineTo(cx + s * a, y0 + h * 0.015)
                        ctx.quadraticCurveTo(cx + s * a * 0.4, y0 + h / 4, cx + s * a * 0.2, y0 + 0.9 * h)
                        ctx.quadraticCurveTo(cx + s * a * 0.2, y0 + h, cx, y0 + h)
                        ctx.closePath()
                    }

                    ctx.reset()
                    // Keep the glow on the face
                    ctx.beginPath()
                    ctx.arc(cx, R, bezelInner, 0, 2 * Math.PI)
                    ctx.clip()

                    ctx.shadowColor = dark ? "#c0000000" : "#50000000"
                    ctx.shadowBlur = a * 0.8
                    ctx.fillStyle = "#000000"
                    half(-1); ctx.fill(); half(1); ctx.fill()
                    ctx.shadowColor = "transparent"

                    // Most layers sit close to the blade, so the light falls off quickly
                    ctx.lineCap = "round"
                    ctx.strokeStyle = Qt.rgba(c.r, c.g, c.b, dark ? 0.026 : 0.016)
                    for (var i = 26; i >= 1; i--) {
                        ctx.beginPath()
                        ctx.lineWidth = a * (1.1 + 3.4 * Math.pow(i / 26, 2.4))
                        ctx.moveTo(cx, y0 + h * 0.1)
                        ctx.lineTo(cx, y0 + h * 0.72)
                        ctx.stroke()
                    }

                    half(-1); ctx.fillStyle = dark ? "#ffffff" : "#3c3c42"; ctx.fill()
                    half(1); ctx.fillStyle = dark ? Qt.rgba(c.r + (1 - c.r) * 0.72, c.g + (1 - c.g) * 0.72, c.b + (1 - c.b) * 0.72, 1) : "#18181b"; ctx.fill()
                }
            }
        }

        // Glass
        Canvas {
            z: 3
            visible: Utility.isDarkMode()
            anchors.fill: parent
            onVisibleChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                var R = outerRadius
                ctx.reset()
                ctx.beginPath()
                ctx.arc(R, R, bezelInner, 0, 2 * Math.PI)
                ctx.clip()
                var g = ctx.createLinearGradient(0, R * 0.08, 0, R * 1.1)
                g.addColorStop(0, "#16ffffff")
                g.addColorStop(1, "#03ffffff")
                ctx.fillStyle = g
                ctx.arc(R * 0.85, R * 2.42, R * 1.9, 0, 2 * Math.PI, true)
                ctx.fill()
            }
        }

        Component {
            id: tickmarkLabel
            Text {
                font.pixelSize: outerRadius * 0.11
                text: parseFloat(value * tickmarkScale).toFixed(0) + tickmarkSuffix
                color: isCovered(value) ? coveredColor : uncoveredColor
                antialiasing: true
                font.family: "Roboto"
            }
        }
        Component {
            id: tickmark
            Rectangle {
                implicitWidth: outerRadius * 0.02
                implicitHeight: outerRadius * 0.08
                antialiasing: true
                smooth: true
                color: isCovered(value) ? coveredColor : uncoveredColor
            }
        }
        Component {
            id: minorTickmark
            Rectangle {
                implicitWidth: outerRadius * 0.013
                implicitHeight: outerRadius * 0.042
                antialiasing: true
                smooth: true
                color: isCovered(value) ? coveredColor : uncoveredColor
                opacity: isCovered(value) ? 1 : 0.55
            }
        }
        function rangeUsed(count, stepSize) {
            return (((count - 1) * stepSize) / (maximumValue - minimumValue)) * angleRange;
        }

        property real tickmarkSectionSize: rangeUsed(tickmarkCount, labelStep) / (tickmarkCount - 1)
        property real tickmarkSectionValue: (maximumValue - minimumValue) / (tickmarkCount - 1)
        property real minorTickmarkSectionSize: tickmarkSectionSize / (minorTickmarkCount + 1)
        property real minorTickmarkSectionValue: tickmarkSectionValue / (minorTickmarkCount + 1)
        property int totalMinorTickmarkCount: {
            // The size of each section within two major tickmarks, expressed as a percentage.
            var minorSectionPercentage = 1 / (minorTickmarkCount + 1);
            // The amount of major tickmarks not able to be displayed; will be 0 if they all fit.
            var tickmarksNotDisplayed = tickmarkCount - tickmarkCount;
            var count = minorTickmarkCount * (tickmarkCount - 1);
            // We'll try to display as many minor tickmarks as we can to fill up the space.
            count + tickmarksNotDisplayed / minorSectionPercentage;
        }
        /*!
                    Returns the angle of \a marker (in the range 0 ... n - 1, where n
                    is the amount of markers) on the gauge where sections are of size
                    tickmarkSectionSize.
                */
        function tickmarkAngleFromIndex(tickmarkIndex) {
            return tickmarkIndex * tickmarkSectionSize + minAngle;
        }

        function minorTickmarkAngleFromIndex(minorTickmarkIndex) {
            var baseAngle = tickmarkAngleFromIndex(Math.floor(minorTickmarkIndex / minorTickmarkCount));
            // + minorTickmarkSectionSize because we don't want the first minor tickmark to start on top of its "parent" tickmark.
            var relativeMinorAngle = (minorTickmarkIndex % minorTickmarkCount * minorTickmarkSectionSize) + minorTickmarkSectionSize;
            return baseAngle + relativeMinorAngle;
        }

        function tickmarkValueFromIndex(majorIndex) {
            return (majorIndex * tickmarkSectionValue) + minimumValue;
        }

        function tickmarkValueFromMinorIndex(minorIndex) {
            var majorIndex = Math.floor(minorIndex / minorTickmarkCount);
            var relativeMinorIndex = minorIndex % minorTickmarkCount;
            return tickmarkValueFromIndex(majorIndex) + ((relativeMinorIndex * minorTickmarkSectionValue) + minorTickmarkSectionValue);
        }
        Repeater {
            id: tickmarkRepeater
            anchors.fill: parent
            model: tickmarkCount
            delegate: Loader {
                id: tickmarkLoader
                objectName: "tickmark" + index
                x: tickmarkRepeater.width / 2
                y: tickmarkRepeater.height / 2
                z:0
                transform: [
                    Translate {
                        y: -outerRadius + tickmarkInset
                    },
                    Rotation {
                        angle: gauge.tickmarkAngleFromIndex(index) - tickmarkWidthAsAngle / 2
                    }
                ]
                sourceComponent: tickmark
                property real value: gauge.tickmarkValueFromIndex(index)
                property real tickmarkWidthAsAngle: r2d(width / outerRadius)
            }
        }
        Repeater {
            id: minorRepeater
            anchors.fill: parent
            model: gauge.totalMinorTickmarkCount
            delegate: Loader {
                id: minorTickmarkLoader
                objectName: "minorTickmark" + index
                x: minorRepeater.width / 2
                y: minorRepeater.height / 2
                transform: [
                    Translate {
                        y: -outerRadius + minorTickmarkInset
                    },
                    Rotation {
                        angle: gauge.minorTickmarkAngleFromIndex(index) - minorTickmarkWidthAsAngle / 2
                    }
                ]
                sourceComponent: minorTickmark
                property real value: gauge.tickmarkValueFromMinorIndex(index)
                property real minorTickmarkWidthAsAngle: r2d(width / outerRadius)
            }
        }
        Repeater {
            id: labelItemRepeater
            anchors.fill: parent
            Component.onCompleted: valueTextModel.update();
            model: ListModel {
                id: valueTextModel
                function update() {
                    if (labelStep === 0) {
                        return;
                    }
                    // Make bigger if it's too small and vice versa.
                    // +1 because we want to show 11 values, with, for example: 0, 10, 20... 100.
                    var difference = tickmarkCount - count;
                    if (difference > 0) {
                        for (; difference > 0; --difference) {
                            append({ value: 0 });
                        }
                    } else if (difference < 0) {
                        for (; difference < 0; ++difference) {
                            remove(count - 1);
                        }
                    }
                    var index = 0;
                    for (var value = minimumValue;
                         value <= maximumValue && index < count;
                         value += labelStep, ++index) {
                        setProperty(index, "value", value);
                    }
                }
            }
            delegate: Loader {
                id: tickmarkLabelDelegateLoader
                sourceComponent: tickmarkLabel
                x: (outerRadius - width / 2) + ((outerRadius - labelInset) * Math.cos((d2r(gauge.tickmarkAngleFromIndex(index) - 90))))
                y: (outerRadius - height / 2) + ((outerRadius - labelInset) * Math.sin((d2r(gauge.tickmarkAngleFromIndex(index) - 90))))
                property real value: index > -1 ? labelItemRepeater.model.get(index).value : 0
            }
        }
    }
}

