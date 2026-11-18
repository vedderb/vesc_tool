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

// Menu that slides up from the bottom of the window like the sheets. Landscape phones are
// too short for that, there it is a card at the bottom right, next to the buttons that open it
Menu {
    id: menu
    readonly property bool side: phoneLandscape
    // Moves the menu down, animated instead of y so that the y binding stays in effect
    property real slide: 0

    parent: Overlay.overlay
    // No clamping inside the window, which would cancel the slide. x, y and height keep it on screen
    margins: -1
    modal: true
    dim: true
    topPadding: side ? 8 : 22
    bottomPadding: 8 + (side ? 0 : notchBot)
    leftPadding: 8
    rightPadding: 8
    width: side ? 340 : Math.min(parent.width, 640)
    height: Math.min(implicitHeight, parent.height - notchTop - (side ? notchBot + 20 : 40))
    x: side ? parent.width - width - 10 - notchRight : (parent.width - width) / 2
    y: parent.height - height - (side ? 10 + notchBot : 0) + slide

    Overlay.modal: Rectangle {
        color: scrimColor
    }

    enter: Transition {
        NumberAnimation { property: "slide"; from: menu.height; to: 0; duration: 200; easing.type: Easing.OutCubic }
    }

    exit: Transition {
        NumberAnimation { property: "slide"; to: menu.height; duration: 150; easing.type: Easing.InCubic }
    }

    background: Rectangle {
        color: Utility.getAppHexColor("cardBackground")
        border.color: Utility.getAppHexColor("raisedBackground")
        radius: 20
        bottomLeftRadius: menu.side ? 20 : 0
        bottomRightRadius: menu.side ? 20 : 0

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 10
            visible: !menu.side
            width: 36
            height: 4
            radius: 2
            color: Utility.getAppHexColor("disabledText")
        }
    }
}
