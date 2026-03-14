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
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import QtQuick.Controls.Material

import Vedder.vesc

// CustomGaugeV2: Qt6 version - delegates to CustomGauge (Canvas-based)
// The original used CircularGauge/CircularGaugeStyle from QtQuick.Extras
// which was removed in Qt6.

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

    CustomGauge {
        id: gauge
        anchors.fill: parent
        unitText: parent.unitText
        typeText: parent.typeText
        tickmarkSuffix: parent.tickmarkSuffix
        labelStep: parent.labelStep
        tickmarkScale: parent.tickmarkScale
        traceColor: parent.traceColor
        maxAngle: parent.maxAngle
        minAngle: parent.minAngle
    }
}
