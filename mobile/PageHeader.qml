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

// Title and Close button for full-page dialogs, which cover the pages inside the safe area
RowLayout {
    property alias title: label.text
    signal closeClicked()

    Label {
        id: label
        Layout.fillWidth: true
        Layout.margins: 16
        font.pointSize: 15
        font.weight: Font.Medium
        color: Utility.getAppHexColor("titleText")
    }

    Button {
        Layout.rightMargin: 8
        text: qsTr("Close")
        flat: true
        onClicked: closeClicked()
    }
}
