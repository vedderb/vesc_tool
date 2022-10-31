/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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
import Vedder.vesc.utility 1.0

Item {
    // Full screen iPhone X workaround:
    property int notchLeft: 0
    property int notchRight: 0
    property int notchBot: 0
    property int notchTop: 0

    Rectangle {
        anchors.fill: parent
        color: Material.background
    }

    ColumnLayout {
        anchors.fill: parent

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            implicitWidth: 0
            clip: true
            visible: false

            background: Rectangle {
                opacity: 1
                color: Utility.getAppHexColor("lightBackground")
            }

            TabButton {
                text: "Test"
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            id: container
            property var tabBarItem: tabBar
        }
    }

    Connections {
        target: QmlUi

        function onReloadQml(str) {
            Qt.createQmlObject(str, container, "myCode");
        }
    }
}
