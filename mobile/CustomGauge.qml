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

import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Extras 1.4
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2

Item {
    property alias minimumValue: gauge.minimumValue
    property alias maximumValue: gauge.maximumValue
    property alias value: gauge.value
    property string unitText: ""
    property string typeText: ""
    property string tickmarkSuffix: ""
    property double labelStep: 10
    property double tickmarkScale: 1
    property color traceColor: "#606060"
    property double maxAngle: 144
    property double minAngle: -144

    CircularGauge {
        id: gauge
        anchors.fill: parent

        Behavior on value {
            NumberAnimation {
                easing.type: Easing.OutCirc
                duration: 100
            }
        }

        style: CircularGaugeStyle {
            id: style
            labelStepSize: labelStep
            tickmarkStepSize: labelStep
            labelInset: outerRadius * 0.28
            tickmarkInset: 2
            minorTickmarkInset: 2
            minimumValueAngle: minAngle
            maximumValueAngle: maxAngle

            background:
                Canvas {
                property double value: gauge.value

                anchors.fill: parent
                onValueChanged: requestPaint()

                function d2r(degrees) {
                    return degrees * (Math.PI / 180.0);
                }

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();

                    ctx.beginPath();
                    var gradient = ctx.createRadialGradient(outerRadius, outerRadius, 0,
                                                            outerRadius, outerRadius, outerRadius)
                    gradient.addColorStop(0.7, Material.background)
                    gradient.addColorStop(1, traceColor)
                    ctx.fillStyle = gradient
                    ctx.arc(outerRadius, outerRadius, outerRadius, 0, Math.PI * 2)
                    ctx.fill()

                    ctx.beginPath();
                    ctx.strokeStyle = Material.background
                    ctx.lineWidth = outerRadius
                    ctx.arc(outerRadius,
                            outerRadius,
                            outerRadius / 2,
                            d2r(valueToAngle(Math.max(gauge.value, 0)) - 90),
                            d2r(valueToAngle(gauge.maximumValue + 1) - 90));
                    ctx.arc(outerRadius,
                            outerRadius,
                            outerRadius / 2,
                            d2r(valueToAngle(gauge.minimumValue) - 90),
                            d2r(valueToAngle(Math.min(gauge.value, 0)) - 90));
                    ctx.stroke();
                    ctx.beginPath();
                    ctx.arc(outerRadius,
                            outerRadius,
                            outerRadius / 2,
                            d2r(valueToAngle(gauge.maximumValue) - 90),
                            d2r(valueToAngle(gauge.minimumValue) - 90));
                    ctx.stroke();

                    ctx.beginPath();
                    ctx.strokeStyle = "darkGray"
                    ctx.lineWidth = 1
                    ctx.arc(outerRadius,
                            outerRadius,
                            outerRadius - 0.5,
                            0, 2 * Math.PI);
                    ctx.stroke();
                }
            }

            needle: Item {
                y: -outerRadius * 0.82
                height: outerRadius * 0.18
                Rectangle {
                    id: needle
                    height: parent.height
                    color: "red"
                    width: height * 0.13
                    antialiasing: true
                    radius: 10
                }

                Glow {
                    anchors.fill: needle
                    radius: 5
                    samples: 10
                    spread: 0.6
                    color: "darkred"
                    source: needle
                }
            }

            foreground: Item {
                Text {
                    id: speedLabel
                    anchors.centerIn: parent
                    text: gauge.value.toFixed(0)
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: outerRadius * 0.3
                    color: "white"
                    antialiasing: true
                }

                Text {
                    id: speedLabelUnit
                    text: unitText
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: speedLabel.bottom
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: outerRadius * 0.15
                    color: "white"
                    antialiasing: true
                }

                Text {
                    id: typeLabel
                    text: typeText
                    verticalAlignment: Text.AlignVCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: speedLabel.top
                    anchors.bottomMargin: outerRadius * 0.1
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: outerRadius * 0.15
                    color: "white"
                    antialiasing: true
                }
            }

            function isCovered(value) {
                var res = false
                if (gauge.value > 0) {
                    if (value <= gauge.value && value >= 0) {
                        res = true
                    }
                } else {
                    if (value >= gauge.value && value <= 0) {
                        res = true
                    }
                }

                return res
            }

            tickmarkLabel:  Text {
                font.pixelSize: outerRadius * 0.15
                text: parseFloat(styleData.value * tickmarkScale).toFixed(0) + tickmarkSuffix
                color: isCovered(styleData.value) ? "white" : "darkGray"
                antialiasing: true
            }

            tickmark: Rectangle {
                implicitWidth: 2
                implicitHeight: outerRadius * 0.09

                antialiasing: true
                smooth: true
                color: isCovered(styleData.value) ? "white" : "darkGray"
            }

            minorTickmark: Rectangle {
                implicitWidth: 1.5
                implicitHeight: outerRadius * 0.05

                antialiasing: true
                smooth: true
                color: isCovered(styleData.value) ? "white" : "darkGray"
            }
        }
    }
}
