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
import QtQuick.Layouts
import Vedder.vesc.utility 1.0

Rectangle {
    id: bar
    property var model: []
    property int currentIndex: 0
    signal activated(int index)

    implicitHeight: 36
    radius: height / 2
    color: Utility.getAppHexColor("darkBackground")
    border.color: Utility.getAppHexColor("raisedBackground")

    RowLayout {
        anchors.fill: parent
        anchors.margins: 3
        spacing: 0

        Repeater {
            model: bar.model

            AbstractButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                focusPolicy: Qt.NoFocus
                onClicked: bar.activated(index)

                background: Rectangle {
                    radius: height / 2
                    color: index === bar.currentIndex ? Utility.getAppHexColor("raisedBackground") : "transparent"
                }

                contentItem: Text {
                    text: modelData
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Medium
                    color: index === bar.currentIndex ? Utility.getAppHexColor("titleText") :
                                                        Utility.getAppHexColor("normalText")
                }
            }
        }
    }
}
