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
    property color traceColor: Utility.getAppHexColor("lightestBackground")
    property double maxAngle: 144
    property double minAngle: -144
    property double precision: 0

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
                    ctx.reset();

                    ctx.beginPath();
                    var gradient2 = ctx.createLinearGradient(parent.width*0.5,0,0 ,parent.height);

                    // Add three color stops
                    gradient2.addColorStop(1, '#111111');
                    gradient2.addColorStop(0.2, '#222222');
                    gradient2.addColorStop(0.7, '#222222');
                    gradient2.addColorStop(0, '#111111');

                    ctx.fillStyle = gradient2;
                    ctx.lineWidth = 0;
                    ctx.arc(outerRadius,
                            outerRadius,
                            outerRadius  ,
                            0, 2 * Math.PI);
                    //ctx.fill();


                    var gradient6 = ctx.createRadialGradient(outerRadius, outerRadius, outerRadius - outerRadius * 0.13, outerRadius, outerRadius, outerRadius- outerRadius * 0.05);
                    gradient6.addColorStop(0, '#00000000');
                    gradient6.addColorStop(1, '#ffffdd80');

                    if (gauge.value < 0) {
                        ctx.beginPath();
                        ctx.strokeStyle = gradient6
                        ctx.lineWidth = outerRadius * 0.18
                        ctx.arc(outerRadius,
                                outerRadius,
                                outerRadius - outerRadius * 0.1,
                                d2r(valueToAngle(gauge.value) - 90),
                                d2r(valueToAngle(0) - 90));
                        ctx.stroke();
                    } else {
                        ctx.beginPath();
                        ctx.strokeStyle = gradient6
                        ctx.lineWidth = outerRadius * 0.18
                        ctx.arc(outerRadius,
                                outerRadius,
                                outerRadius - outerRadius * 0.1,
                                d2r(valueToAngle(0) - 90),
                                d2r(valueToAngle(gauge.value) - 90));
                        ctx.stroke();
                    }


                    ctx.beginPath();
                    var gradient3 = ctx.createLinearGradient(parent.width,0,0 ,parent.height);

                    // Add three color stops
                    gradient3.addColorStop(1, '#dddddd');
                    gradient3.addColorStop(0.7, '#333333');
                    gradient3.addColorStop(0.1, '#eeeeee');

                    ctx.strokeStyle = gradient3;
                    ctx.lineWidth = outerRadius*0.03
                    ctx.arc(outerRadius,
                            outerRadius,
                            outerRadius*0.985  ,
                            0, 2 * Math.PI);
                    ctx.stroke();

                    ctx.beginPath();
                    var gradient4 = ctx.createLinearGradient(parent.width,0,0 ,parent.height);

                    // Add three color stops
                    gradient4.addColorStop(1, '#333333');
                    gradient4.addColorStop(0.8, '#dddddd');
                    gradient4.addColorStop(0, '#555555');

                    ctx.strokeStyle = gradient4;
                    ctx.lineWidth = outerRadius*0.03
                    ctx.arc(outerRadius,
                            outerRadius,
                            0.96*outerRadius ,
                            0, 2 * Math.PI);
                    ctx.stroke();


                    ctx.beginPath();
                    var gradient5 = ctx.createLinearGradient(parent.width,0,0 ,parent.height);

                    // Add three color stops
                    gradient5.addColorStop(1, '#333333');
                    gradient5.addColorStop(0.8, '#dddddd');
                    gradient5.addColorStop(0, '#555555');

                    ctx.strokeStyle = gradient5;
                    ctx.lineWidth = outerRadius*0.03
                    ctx.arc(outerRadius,
                            outerRadius,
                            0.96*outerRadius ,
                            0, 2 * Math.PI);
                    ctx.stroke();

                    ctx.beginPath();

                    ctx.strokeStyle = 'black';
                    ctx.lineWidth = outerRadius*0.005
                    ctx.arc(outerRadius,
                            outerRadius,
                            outerRadius*0.94  ,
                            0, 2 * Math.PI);
                    ctx.stroke();



                }
            }

            needle: Item {
                y: -outerRadius * 0.82 + outerRadius*0.06
                height: outerRadius * 0.18
                width: outerRadius * 0.15

                Canvas{
                    id: pointerNib
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
                        gradient.addColorStop(0, '#776600');
                        gradient.addColorStop(0.80, '#ffbb00');
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
                        gradient2.addColorStop(0, '#ffbb00');
                        gradient2.addColorStop(0.80, '#776600');
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

            foreground: Item {
                Text {
                    id: speedLabel
                    anchors.centerIn: parent
                    text: gauge.value.toFixed(precision)
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: outerRadius * 0.3
                    color: Utility.getAppHexColor("lightText")
                    antialiasing: true
                }

                Text {
                    id: speedLabelUnit
                    text: unitText
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: speedLabel.bottom
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: outerRadius * 0.15
                    color: Utility.getAppHexColor("lightText")
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
                    color: Utility.getAppHexColor("lightText")
                    antialiasing: true
                }


                RadialGradient{
                    id: glassEffect2
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
                RadialGradient{
                    id: glassEffect3
                    horizontalOffset: outerRadius*0.8 - width/2
                    horizontalRadius: outerRadius*1.9
                    verticalOffset: outerRadius*2.3 - height/2
                    verticalRadius: outerRadius*1.9
                    anchors.fill: glassEffect2
                    visible:  false
                    z: glassEffect2.z + 1
                    gradient: Gradient {
                        GradientStop { position: 1.2; color: "#33000000" }
                        GradientStop { position: 1.1; color: "#44000000" }
                        GradientStop { position: 1.05; color: "#33000000" }
                        GradientStop { position: 1; color: "#22000000" }
                        GradientStop { position: 0.98; color: "transparent" } // Just a part of the canvas
                    }
                }
                OpacityMask {
                    id: glassEffect4
                    anchors.fill: glassEffect2
                    source: glassEffect2
                    maskSource: glassEffect3
                    invert: false
                    visible: true
                }

                LinearGradient {
                    id: glassEffect5
                    anchors.fill: glassEffect2
                    start: Qt.point(0.6*outerRadius, outerRadius)
                    end: Qt.point(outerRadius, 0.2*outerRadius)

                    gradient: Gradient {
                        GradientStop { position: 1.8; color: "transparent" }
                        GradientStop { position: -6; color: "white" } // Just a part of the canvas
                    }
                    visible: false// Not visible (it will be painted by the mask)
                }

                OpacityMask {
                    id: glassEffect6
                    anchors.fill: glassEffect2
                    source: glassEffect4
                    maskSource: glassEffect5
                    invert: false
                    visible: false
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
                color: isCovered(styleData.value) ? Utility.getAppHexColor("lightText") : Utility.getAppHexColor("disabledText")
                antialiasing: true
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

