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

// Small centered card that blocks input while something is in progress
Dialog {
    id: busy
    property bool spinner: true
    property string cancelText: ""
    default property alias content: column.data

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 64, 320)
    modal: true
    closePolicy: Popup.NoAutoClose
    padding: 24
    header: null

    Overlay.modal: Rectangle {
        color: scrimColor
    }

    background: Rectangle {
        radius: 16
        color: Utility.getAppHexColor("cardBackground")
        border.color: Utility.getAppHexColor("raisedBackground")
    }

    contentItem: ColumnLayout {
        id: column
        spacing: 16

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            visible: busy.spinner
            running: visible && busy.opened
        }

        Label {
            Layout.fillWidth: true
            text: busy.title
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.weight: Font.Medium
            color: Utility.getAppHexColor("titleText")
        }
    }

    footer: Item {
        visible: busy.cancelText !== ""
        implicitHeight: visible ? cancelButton.implicitHeight + 12 : 0

        Button {
            id: cancelButton
            anchors.centerIn: parent
            text: busy.cancelText
            flat: true
            onClicked: busy.reject()
        }
    }
}
