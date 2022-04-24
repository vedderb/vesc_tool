/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

import QtQuick 2.7
import Qt.labs.settings 1.0
import QtQuick.Controls 2.2

import SkyPuff.vesc.winch 1.0

ApplicationWindow {
    id: app
    visible: true
    title: "SkyPUFF"
    
    SwipeView {
        id: swipeView
        anchors.fill: parent

        PageConnection {}
        PageSkypuff {}
        PageConfig {}
        PageTerminal {}
    }

    // Save window size per app restarts
    Settings {
        property alias width: app.width
        property alias height: app.height
    }

    // Switch pages automatically on state changes
    Connections {
        target: Skypuff

        onStateChanged: {
            switch(newState) {
            case "DISCONNECTED":
                if(swipeView.currentIndex != 0)
                    swipeView.currentIndex = 0
                break
            case "UNITIALIZED":
                if(swipeView.currentIndex != 2)
                    swipeView.currentIndex = 2
                break
            default:
                if(swipeView.currentIndex != 1)
                    swipeView.currentIndex = 1
            }
        }
    }
}
