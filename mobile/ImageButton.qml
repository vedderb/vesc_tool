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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import Vedder.vesc.utility 1.0

Item {
    id: rootItem
    property var imageSrc: ""
    property var buttonText: ""
    signal clicked()

    Button {
        id: control
        anchors.fill: parent
        flat: true

        onClicked: {
            rootItem.clicked()
        }

        background: Rectangle {
            anchors.fill: parent
            radius: 10
            color: control.pressed ? Utility.getAppHexColor("darkAccent") :Utility.getAppHexColor("normalBackground")
            border.color: Utility.getAppHexColor("disabledText")
            border.width: control.activeFocus ? 2 : 1

            RowLayout {
                anchors.fill: parent

                Image {
                    id: img
                    Layout.fillHeight: true
                    Layout.preferredWidth: height * 0.7
                    Layout.margins: 6
                    Layout.leftMargin: 6
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    source: imageSrc
                    smooth: true
                }

                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: buttonText
                    color: Utility.getAppHexColor("lightText")
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
