/*
    Copyright 2020 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import QtQuick.Window 2.2
import Vedder.vesc.utility 1.0

ApplicationWindow {
    id: mainWindow
    visible: true
    visibility: Window.Windowed
    width: 1920
    height: 1080
    title: qsTr("VESC Custom GUI")

    // Full screen iPhone X workaround:
    property int notchLeft: 0
    property int notchRight: 0
    property int notchBot: 0
    property int notchTop: 0

    property string lastFile: ""
    property bool wasFullscreen: false

    onClosing: {
        loader.source = ""
    }

    Loader {
        id: loader
        anchors.fill: parent
    }

    Connections {
        target: QmlUi

        onReloadFile: {
            loader.source = ""
            QmlUi.clearQmlCache()
            loadTimer.start()
            lastFile = fileName
        }

        onToggleFullscreen: {
            wasFullscreen = !wasFullscreen
            if (wasFullscreen) {
                mainWindow.visibility = Window.FullScreen
            } else {
                mainWindow.visibility = Window.Windowed
            }
        }

        onMoveToOtherScreen: {
            var screenInd = Qt.application.screens.length - 1
            mainWindow.x = Qt.application.screens[screenInd].virtualX;
            mainWindow.y = Qt.application.screens[screenInd].virtualY;
        }

        onMoveToFirstScreen: {
            mainWindow.x = Qt.application.screens[0].virtualX;
            mainWindow.y = Qt.application.screens[0].virtualY;
        }
    }

    Timer {
        id: loadTimer
        repeat: false
        running: false
        interval: 200
        onTriggered: loader.source = lastFile + "?t=" + Date.now()
    }
}
