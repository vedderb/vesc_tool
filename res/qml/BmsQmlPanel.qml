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
import QtQuick.Controls

import Vedder.vesc.utility 1.0

import "qrc:/mobile"

// The BMS page of the mobile UI on the desktop, with the cells and temperatures side by side
Item {
    id: container

    // Globals that the mobile components expect from mobile/main.qml
    property bool mainIsTablet: true
    readonly property bool phoneLandscape: false
    property int notchLeft: 0
    property int notchRight: 0
    property int notchBot: 0
    property int notchTop: 0

    Rectangle {
        anchors.fill: parent
        color: Utility.getAppHexColor("darkBackground")
    }

    BMS {
        anchors.fill: parent
        forceWide: true
    }
}
