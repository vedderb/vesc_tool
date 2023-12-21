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

    Item {
        id: loaderContainer
        anchors.centerIn: parent
        width: mainWindow.width
        height: mainWindow.height
        rotation: 0

        Loader {
            id: loader
            anchors.fill: parent
        }
    }

    Connections {
        target: QmlUi

        function onReloadFile(fileName) {
            loader.source = ""
            QmlUi.clearQmlCache()
            loadTimer.start()
            lastFile = fileName
        }

        function onToggleFullscreen() {
            wasFullscreen = !wasFullscreen
            if (wasFullscreen) {
                mainWindow.visibility = Window.FullScreen
            } else {
                mainWindow.visibility = Window.Windowed
            }
        }

        function onMoveToOtherScreen() {
            var screenInd = Qt.application.screens.length - 1
            mainWindow.x = Qt.application.screens[screenInd].virtualX;
            mainWindow.y = Qt.application.screens[screenInd].virtualY;
        }

        function onMoveToFirstScreen() {
            mainWindow.x = Qt.application.screens[0].virtualX;
            mainWindow.y = Qt.application.screens[0].virtualY;
        }

        // See https://stackoverflow.com/questions/5789239/calculate-largest-rectangle-in-a-rotated-rectangle
        function getCropCoordinates(angleInRadians, imageDimensions) {
            var ang = angleInRadians;
            var img = imageDimensions;

            var quadrant = Math.floor(ang / (Math.PI / 2)) & 3;
            var sign_alpha = (quadrant & 1) === 0 ? ang : Math.PI - ang;
            var alpha = (sign_alpha % Math.PI + Math.PI) % Math.PI;

            var bb = {
                w: img.w * Math.cos(alpha) + img.h * Math.sin(alpha),
                h: img.w * Math.sin(alpha) + img.h * Math.cos(alpha)
            };

            var gamma = img.w < img.h ? Math.atan2(bb.w, bb.h) : Math.atan2(bb.h, bb.w);

            var delta = Math.PI - alpha - gamma;

            var length = img.w < img.h ? img.h : img.w;
            var d = length * Math.cos(alpha);
            var a = d * Math.sin(alpha) / Math.sin(delta);

            var y = a * Math.cos(gamma);
            var x = y * Math.tan(gamma);

            return {
                x: x,
                y: y,
                w: bb.w - 2 * x,
                h: bb.h - 2 * y
            };
        }

        function onRotateScreen(rot) {
            loaderContainer.rotation = rot

            loaderContainer.width = Qt.binding(function() {
                var cor = getCropCoordinates(rot * Math.PI / 180, {w: mainWindow.width, h: mainWindow.height})
                return cor.w
            })
            loaderContainer.height = Qt.binding(function() {
                var cor = getCropCoordinates(rot * Math.PI / 180, {w: mainWindow.width, h: mainWindow.height})
                return cor.h
            })
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
