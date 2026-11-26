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
import QtQuick.Layouts
import Vedder.vesc.utility 1.0

// Title and hint of a wizard step
ColumnLayout {
    property alias title: titleText.text
    property alias hint: hintText.text

    Layout.fillWidth: true
    spacing: 2

    Text {
        id: titleText
        Layout.fillWidth: true
        color: Utility.getAppHexColor("lightText")
        font.bold: true
        wrapMode: Text.WordWrap
    }

    Text {
        id: hintText
        Layout.fillWidth: true
        visible: text !== ""
        color: Utility.getAppHexColor("normalText")
        font.pointSize: 11
        wrapMode: Text.WordWrap
    }
}
