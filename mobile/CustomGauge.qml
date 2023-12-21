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
import QtQuick.Layouts 1.3
import QtQuick.Extras 1.4
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0

Item {
    property double value: 0
    property double outerRadius: width/2
    height: width
    property double labelStep: 10
    property double labelInset: outerRadius * 0.28 + outerRadius*0.06
    property double tickmarkInset: outerRadius*0.07
    property double minorTickmarkInset: outerRadius*0.07
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
    property color nibColor: {nibColor = Utility.getAppHexColor("tertiary2")}
    property color traceColor: Qt.lighter(nibColor,1.5)
    property double precision: 0

    property color coveredColor: {coveredColor = Utility.getAppHexColor("lightText")}
    property color uncoveredColor: {uncoveredColor = Utility.getAppHexColor("disabledText")}

    property bool tickmarksVisible: true
    property bool centerTextVisible: true

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

    Rectangle {
        id: gauge
        x: outerRadius - width/2
        y: outerRadius - height/2
        width: outerRadius*2
        height: outerRadius*2
        z:0
        radius:outerRadius
        color: {color = Utility.isDarkMode() ? Utility.getAppHexColor("darkBackground") : Utility.getAppHexColor("normalBackground")}
        property double value: parent.value
        Behavior on value {
            NumberAnimation {
                easing.type: Easing.OutCirc
                duration: 100
            }
        }
        Text {
            id: speedLabel
            anchors.centerIn: parent
            visible: centerTextVisible
            text: gauge.value.toFixed(precision)
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: outerRadius * 0.3
            color: {color = Utility.getAppHexColor("lightText") }
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
            font.pixelSize: outerRadius * 0.12
            color: {color = Utility.getAppHexColor("lightText")}
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
            font.pixelSize: outerRadius * 0.12
            color: {color = Utility.getAppHexColor("lightText")}
            antialiasing: true
            font.family: "Roboto"
            font.capitalization: Font.AllUppercase
        }

        Item {
            x: outerRadius - width/2
            y: outerRadius * 0.05
            z:2
            height: outerRadius * 0.22
            width: outerRadius * 0.12
            transform: Rotation {
                id:needleTransform
                origin.x: outerRadius * 0.12/2
                origin.y: outerRadius*(1-0.05)
                angle: valueToAngle(gauge.value)
            }
            Canvas {
                id: pointerNib
                property double value: gauge.value
                property double gAngle: needleTransform.angle
                Component.onCompleted: requestPaint()
                onGAngleChanged: requestPaint()
                anchors.fill:parent
                onPaint:{
                    var ctx = getContext("2d");
                    if(nibColor != null) {
                        // the triangle
                        ctx.beginPath();
                        ctx.moveTo(parent.width/2, 0);
                        ctx.lineTo(parent.width, parent.height*0.015);
                        ctx.quadraticCurveTo(parent.width*0.7, parent.height/4, 0.6*parent.width , 0.9*parent.height);
                        ctx.quadraticCurveTo(parent.width*0.6, parent.height, parent.width/2, parent.height);
                        ctx.closePath();

                        var gradient = ctx.createLinearGradient(parent.width,parent.height*0.02,parent.width/2,0);
                        gradient.addColorStop(0, Qt.darker(nibColor,1.0 + 0.5* Math.sin(d2r(gAngle - 36))));
                        gradient.addColorStop(0.80, Qt.darker(nibColor,1.0 + 0.5* Math.sin(d2r(gAngle))));
                        gradient.addColorStop(0.92, '#ffffff');
                        gradient.addColorStop(1, '#ffffff');

                        // the fill color
                        ctx.fillStyle = gradient;
                        ctx.fill();

                        // the triangle
                        ctx.beginPath();
                        ctx.moveTo(parent.width/2, 0);
                        ctx.lineTo(0, parent.height*0.015);
                        ctx.quadraticCurveTo(parent.width*0.3, parent.height/4, 0.4*parent.width, 0.9*parent.height);
                        ctx.quadraticCurveTo(parent.width*0.4, parent.height, parent.width/2, parent.height);
                        ctx.closePath();

                        var gradient2 = ctx.createLinearGradient(0,parent.height*0.02,parent.width/2,0);
                        gradient2.addColorStop(0, Qt.darker(nibColor,1.0 + 0.5* Math.sin(d2r(gAngle + 144))));
                        gradient2.addColorStop(0.80, Qt.darker(nibColor,1.0 + 0.5* Math.sin(d2r(gAngle + 180))));
                        gradient2.addColorStop(0.92, '#ffffff');
                        gradient2.addColorStop(1, '#ffffff');
                        // the fill color
                        ctx.fillStyle = gradient2;
                        ctx.fill();
                    }
                }
            }
        }
        Canvas {
            id:foregroundpaint
            anchors.fill: parent
            z:3
            property double angle: needleTransform.angle
            onAngleChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                //create radial glow around outside of gauge
                var gradient1 = ctx.createRadialGradient(outerRadius, outerRadius, outerRadius - outerRadius * 0.13, outerRadius, outerRadius, outerRadius- outerRadius * 0.05);
                gradient1.addColorStop(0, '#00000000');
                gradient1.addColorStop(1, traceColor);
                ctx.beginPath();
                ctx.strokeStyle = gradient1
                ctx.lineWidth = outerRadius * 0.2
                ctx.arc(outerRadius,
                        outerRadius,
                        outerRadius - outerRadius * 0.12,
                        d2r(valueToAngle(0.0) - 90),
                        d2r(needleTransform.angle - 90),
                        (gauge.value*isInverted) < 0);
                ctx.stroke();
            }
            Canvas {
                anchors.fill: parent
                property color lightBG: {lightBG = Utility.getAppHexColor("lightestBackground")}
                property color darkBG: {darkBG = Utility.getAppHexColor("darkBackground")}
                property real borderWidth: outerRadius*0.035;
                Component.onCompleted: requestPaint()
                onPaint: {
                    var ctx = getContext("2d");
                    //create outer gauge metal bezel effect
                    ctx.beginPath();
                    var gradient2 = ctx.createLinearGradient(parent.width,0,0 ,parent.height);
                    // Add three color stops
                    gradient2.addColorStop(1, lightBG);
                    gradient2.addColorStop(0.7, darkBG);
                    gradient2.addColorStop(0.1, lightBG);
                    ctx.strokeStyle = gradient2;
                    ctx.lineWidth = borderWidth
                    ctx.arc(outerRadius,
                            outerRadius,
                            outerRadius - borderWidth/2,
                            0, 2 * Math.PI);
                    ctx.stroke();
                    ctx.beginPath();
                    var gradient3 = ctx.createLinearGradient(parent.width,0,0 ,parent.height);
                    // Add three color stops
                    gradient3.addColorStop(1, darkBG);
                    gradient3.addColorStop(0.8, lightBG);
                    gradient3.addColorStop(0, darkBG);
                    ctx.strokeStyle = gradient3;
                    ctx.lineWidth = borderWidth
                    ctx.arc(outerRadius,
                            outerRadius,
                            outerRadius - borderWidth*3/2 + 1,
                            0, 2 * Math.PI);
                    ctx.stroke();
                }
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
        Component {
            id: tickmarkLabel
            Text {
                font.pixelSize: outerRadius * 0.12
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
                implicitHeight: outerRadius * 0.1
                antialiasing: true
                smooth: true
                color: isCovered(value) ? coveredColor : uncoveredColor
            }
        }
        Component {
            id: minorTickmark
            Rectangle {
                implicitWidth: outerRadius * 0.015
                implicitHeight: outerRadius * 0.07
                antialiasing: true
                smooth: true
                color: isCovered(value) ? coveredColor : uncoveredColor
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

