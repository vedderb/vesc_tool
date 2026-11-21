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
import QtQuick.Controls.Material
import QtQuick.Layouts
import Vedder.vesc.utility 1.0

// Menu row with an icon tile like the cards on the start page
MenuItem {
    id: entry
    // File in the theme's icons folder
    property string iconFile: ""

    // Lower without the icon tile
    implicitHeight: iconFile !== "" ? 52 : 44
    leftPadding: 10
    rightPadding: 10

    contentItem: RowLayout {
        spacing: 14
        opacity: entry.enabled ? 1 : 0.4

        Rectangle {
            visible: entry.iconFile !== ""
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            radius: 10
            color: entry.down ? Qt.alpha(Material.accent, 0.25) : Utility.getAppHexColor("raisedBackground")

            Image {
                anchors.centerIn: parent
                width: 22
                height: 22
                source: entry.iconFile !== "" ? "qrc" + Utility.getThemePath() + "icons/" + entry.iconFile : ""
                smooth: true
            }
        }

        Label {
            Layout.fillWidth: true
            text: entry.text
            elide: Text.ElideRight
            font.pointSize: 13
            color: Utility.getAppHexColor("lightText")
        }
    }

    background: Rectangle {
        radius: 12
        color: entry.down ? Qt.alpha(Material.accent, 0.12) : entry.highlighted ? Utility.getAppHexColor("darkBackground") : "transparent"
    }
}
