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
import Vedder.vesc.utility 1.0

Item {
    property alias minimumValue: gauge.minimumValue
    property alias maximumValue: gauge.maximumValue
    property alias value: gauge.value
    property string unitText: ""
    property string typeText: ""
    property string tickmarkSuffix: ""
    property double labelStep: 10
    property double tickmarkScale: 1
    property color nibColor: Utility.getAppHexColor("tertiary1")
    property color traceColor: Qt.lighter(nibColor,1.5)
    property double maxAngle: 144
    property double minAngle: -144
    property double precision: 0
    property int isInverted: 1
    property bool centerTextVisible: true

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
            labelInset: outerRadius * 0.28 + outerRadius*0.03
            tickmarkInset: outerRadius*0.07
            minorTickmarkInset: outerRadius*0.07
            minimumValueAngle: minAngle
            maximumValueAngle: maxAngle
            function d2r(degrees) {
                return degrees * (Math.PI / 180.0);
            }
            //onDataChanged:  pointerNib.requestPaint()
            background: Item {
                Canvas {
                    id:backgroundpaint
                    property double value: gauge.value
                    anchors.fill: parent
                    onValueChanged: requestPaint()
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset()
                        //Draw background color with highlighted pie slice
                        ctx.beginPath()
                        ctx.fillStyle = Utility.getAppHexColor("darkBackground")
                        ctx.arc(outerRadius, outerRadius, outerRadius, 0, Math.PI * 2)
                        ctx.fill()
                        ctx.beginPath();

                        //create radial glow around outside of gauge
                        var gradient1 = ctx.createRadialGradient(outerRadius, outerRadius, outerRadius - outerRadius * 0.13, outerRadius, outerRadius, outerRadius- outerRadius * 0.05);
                        gradient1.addColorStop(0, '#00000000');
                        gradient1.addColorStop(0.8, traceColor);
                        if (gauge.value*isInverted < 0) {
                            ctx.beginPath();
                            ctx.strokeStyle = gradient1
                            ctx.lineWidth = outerRadius * 0.18
                            ctx.arc(outerRadius,
                                    outerRadius,
                                    outerRadius - outerRadius * 0.1,
                                    d2r(valueToAngle(gauge.value) - 90),
                                    d2r(valueToAngle(0) - 90));
                            ctx.stroke();
                        } else {
                            ctx.beginPath();
                            ctx.strokeStyle = gradient1
                            ctx.lineWidth = outerRadius * 0.18
                            ctx.arc(outerRadius,
                                    outerRadius,
                                    outerRadius - outerRadius * 0.1,
                                    d2r(valueToAngle(0) - 90),
                                    d2r(valueToAngle(gauge.value) - 90));
                            ctx.stroke();
                        }

                        //create outer gauge metal bezel effect
                        ctx.beginPath();
                        var gradient2 = ctx.createLinearGradient(parent.width,0,0 ,parent.height);
                        // Add three color stops
                        gradient2.addColorStop(1, '#cccccc');
                        gradient2.addColorStop(0.7, '#333333');
                        gradient2.addColorStop(0.1, '#eeeeee');
                        ctx.strokeStyle = gradient2;
                        ctx.lineWidth = outerRadius*0.03
                        ctx.arc(outerRadius,
                                outerRadius,
                                outerRadius*0.985  ,
                                0, 2 * Math.PI);
                        ctx.stroke();
                        ctx.beginPath();
                        var gradient3 = ctx.createLinearGradient(parent.width,0,0 ,parent.height);
                        // Add three color stops
                        gradient3.addColorStop(1, '#111111');
                        gradient3.addColorStop(0.8, '#cccccc');
                        gradient3.addColorStop(0, '#222222');
                        ctx.strokeStyle = gradient3;
                        ctx.lineWidth = outerRadius*0.03
                        ctx.arc(outerRadius,
                                outerRadius,
                                0.96*outerRadius ,
                                0, 2 * Math.PI);
                        ctx.stroke();

                        //small black inset line on inner gauge to give seam
                        ctx.beginPath();
                        ctx.strokeStyle = 'black';
                        ctx.lineWidth = outerRadius*0.002
                        ctx.arc(outerRadius,
                                outerRadius,
                                outerRadius*0.942  ,
                                0, 2 * Math.PI);
                        ctx.stroke();
                    }
                }
                DropShadow {
                    anchors.fill: parent
                    horizontalOffset: -1
                    verticalOffset: 1
                    radius: 3
                    samples: 1 + radius*2
                    color: "#40000000"
                    source: backgroundpaint
                }
            }
            needle: Rectangle{
                color: "transparent"
            }

            foreground: Item {
                Text {
                    id: speedLabel
                    anchors.centerIn: parent
                    visible: centerTextVisible
                    text: gauge.value.toFixed(precision)
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: outerRadius * 0.3
                    color: Utility.getAppHexColor("lightText")
                    antialiasing: true
                    font.family: "Roboto Mono"
                }

                Text {
                    id: speedLabelUnit
                    text: unitText
                    visible: centerTextVisible
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: speedLabel.bottom
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: outerRadius * 0.12
                    color: Utility.getAppHexColor("lightText")
                    antialiasing: true
                    font.family: "Roboto Mono"
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
                    font.pixelSize: outerRadius * 0.12
                    color: Utility.getAppHexColor("lightText")
                    antialiasing: true
                    font.family: "Roboto Mono"
                    font.capitalization: Font.AllUppercase
                }

                Item {
                    x: outerRadius - width/2
                    y: outerRadius * 0.058
                    height: outerRadius * 0.18
                    width: outerRadius * 0.15

                    transform: Rotation {
                        id:needleTransform
                        origin.x: outerRadius * 0.15/2
                        origin.y: outerRadius*(1-0.058)
                        angle: valueToAngle(gauge.value)
                    }
                    Canvas {
                        id: pointerNib
                        property alias gAngle: needleTransform.angle
                        onGAngleChanged: requestPaint()
                        anchors.fill:parent
                        onPaint:{
                            var ctx = getContext("2d");
                            // the triangle
                            ctx.beginPath();
                            ctx.moveTo(parent.width/2, 0);
                            ctx.lineTo(parent.width, parent.height*0.015);
                            ctx.quadraticCurveTo(parent.width*0.7, parent.height/4, parent.width*0.58, parent.height);
                            ctx.lineTo(parent.width/2, parent.height);
                            ctx.closePath();

                            var gradient = ctx.createLinearGradient(parent.width,parent.height*0.02,parent.width/2,0);
                            gradient.addColorStop(0, Qt.darker(nibColor,1.0 + 0.5* Math.sin(gAngle*3.14/180 - 3.14*0.2)));
                            gradient.addColorStop(0.80, Qt.darker(nibColor,1.0 + 0.5* Math.sin(gAngle*3.14/180 + 3.14*0.0)));
                            gradient.addColorStop(0.92, '#ffffff');
                            gradient.addColorStop(1, '#ffffff');


                            // the fill color
                            ctx.fillStyle = gradient;
                            ctx.fill();

                            // the triangle
                            ctx.beginPath();
                            ctx.moveTo(parent.width/2, 0);
                            ctx.lineTo(0, parent.height*0.015);
                            ctx.quadraticCurveTo(parent.width*0.3, parent.height/4, parent.width*0.42, parent.height);
                            ctx.lineTo(parent.width/2, parent.height);
                            ctx.closePath();

                            var gradient2 = ctx.createLinearGradient(0,parent.height*0.02,parent.width/2,0);
                            gradient2.addColorStop(0, Qt.darker(nibColor,1.0 + 0.5* Math.sin(gAngle*3.14/180 + 3.14*0.8)));
                            gradient2.addColorStop(0.80, Qt.darker(nibColor,1.0 + 0.5* Math.sin(gAngle*3.14/180 + 3.14*1.0)));
                            gradient2.addColorStop(0.92, '#ffffff');
                            gradient2.addColorStop(1, '#ffffff');
                            // the fill color
                            ctx.fillStyle = gradient2;
                            ctx.fill();
                        }
                    }
                    DropShadow
                    {
                        anchors.fill: pointerNib
                        horizontalOffset: 0
                        verticalOffset: 0
                        radius: pointerNib.width/6
                        samples: 10
                        color: "#90000000"
                        source: pointerNib
                    }

                }

                //white circle to overlay glass
                RadialGradient {
                    id: glassEffect1
                    x: outerRadius - width/2
                    y: outerRadius - height/2
                    height: 2*outerRadius*0.96
                    width: height
                    visible: false

                    gradient: Gradient {
                        GradientStop { position: 1.0; color: "transparent" }
                        GradientStop { position: 0.5; color: "transparent" }
                        GradientStop { position: 0.495; color: "white" } // Just a part of the canvas
                    }
                }
                //Cutout circle for part where glare isn't
                RadialGradient{
                    id: glassEffect2
                    horizontalOffset: outerRadius*0.8 - width/2
                    horizontalRadius: outerRadius*1.9
                    verticalOffset: outerRadius*2.3 - height/2
                    verticalRadius: outerRadius*1.9
                    anchors.fill: glassEffect1
                    visible:  false
                    z: glassEffect1.z + 1
                    gradient: Gradient {
                        GradientStop { position: 1.2; color: "#22000000" }
                        GradientStop { position: 1.1; color: "#33000000" }
                        GradientStop { position: 1.05; color: "#22000000" }
                        GradientStop { position: 1; color: "#11000000" }
                        GradientStop { position: 0.98; color: "transparent" } // Just a part of the canvas
                    }
                }
                //Subtract both
                OpacityMask {
                    id: glassEffect3
                    anchors.fill: glassEffect1
                    source: glassEffect1
                    maskSource: glassEffect2
                    invert: false
                    visible: true
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
                font.pixelSize: outerRadius * 0.12
                text: parseFloat(styleData.value * tickmarkScale).toFixed(0) + tickmarkSuffix
                color: isCovered(styleData.value) ? Utility.getAppHexColor("lightText") : Utility.getAppHexColor("disabledText")
                antialiasing: true
                font.family: "Roboto Mono"
            }
            tickmark: Rectangle {
                implicitWidth: 2
                implicitHeight: outerRadius * 0.09
                antialiasing: true
                smooth: true
                color: isCovered(styleData.value) ? Utility.getAppHexColor("lightText") : Utility.getAppHexColor("disabledText")
            }
            minorTickmark: Rectangle {
                implicitWidth: 1.5
                implicitHeight: outerRadius * 0.05
                antialiasing: true
                smooth: true
                color: isCovered(styleData.value) ? Utility.getAppHexColor("lightText") : Utility.getAppHexColor("disabledText")
            }
        }
    }
}

