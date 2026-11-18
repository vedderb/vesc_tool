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

Rectangle {
    property alias text: warnText.text

    implicitHeight: warnRow.implicitHeight + 20
    radius: 12
    color: Qt.alpha(Utility.getAppHexColor("warning"), 0.15)
    border.color: Qt.alpha(Utility.getAppHexColor("warningText"), 0.33)

    RowLayout {
        id: warnRow
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Image {
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            source: "qrc" + Utility.getThemePath() + "icons/Brake Warning-96.png"
        }

        Text {
            id: warnText
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Utility.getAppHexColor("warningText")
        }
    }
}
