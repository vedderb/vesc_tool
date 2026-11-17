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

// Bottom sheet with optional icon, title and accept/reject buttons
Dialog {
    id: sheet
    property string iconSource: ""
    property string acceptText: ""
    property string rejectText: acceptText === "" ? qsTr("Close") : qsTr("Cancel")
    property bool fullHeight: false
    // Moves the sheet down. Animated and dragged instead of y so that the y binding stays in effect
    property real slide: 0

    parent: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(parent.width, 640)
    height: fullHeight ? parent.height - notchTop - 40 : Math.min(implicitHeight, parent.height - notchTop - 40)
    x: (parent.width - width) / 2
    y: parent.height - height + slide
    leftPadding: 16 + notchLeft
    rightPadding: 16 + notchRight
    topPadding: 4
    bottomPadding: footer.visible ? 0 : 16 + notchBot

    Overlay.modal: Rectangle {
        color: scrimColor
    }

    // iOS keeps the keyboard and the view offset otherwise
    onAboutToHide: Qt.inputMethod.hide()

    enter: Transition {
        NumberAnimation { property: "slide"; from: sheet.height; to: 0; duration: 200; easing.type: Easing.OutCubic }
    }

    exit: Transition {
        NumberAnimation { property: "slide"; to: sheet.height; duration: 150; easing.type: Easing.InCubic }
    }

    NumberAnimation {
        id: slideBack
        target: sheet
        property: "slide"
        to: 0
        duration: 150
    }

    background: Rectangle {
        color: Utility.getAppHexColor("cardBackground")
        border.color: Utility.getAppHexColor("raisedBackground")
        topLeftRadius: 20
        topRightRadius: 20
    }

    header: ColumnLayout {
        spacing: 0

        // Dragging the header down closes the sheet like Escape does
        DragHandler {
            id: closeDrag
            target: null
            margin: 15
            enabled: sheet.closePolicy & Popup.CloseOnEscape
            onActiveTranslationChanged: sheet.slide = Math.max(activeTranslation.y, 0)
            onActiveChanged: {
                if (!active && sheet.opened) {
                    if (sheet.slide > sheet.height / 4) {
                        sheet.reject()
                    } else {
                        slideBack.start()
                    }
                }
            }
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10
            visible: closeDrag.enabled
            width: 36
            height: 4
            radius: 2
            color: Utility.getAppHexColor("disabledText")
        }

        RowLayout {
            Layout.margins: 16
            Layout.leftMargin: 16 + notchLeft
            Layout.bottomMargin: 8
            spacing: 12
            visible: sheet.title !== ""

            Rectangle {
                visible: sheet.iconSource !== ""
                width: 40
                height: 40
                radius: 12
                color: Utility.getAppHexColor("raisedBackground")

                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: sheet.iconSource
                    smooth: true
                }
            }

            Label {
                Layout.fillWidth: true
                text: sheet.title
                wrapMode: Text.Wrap
                font.pointSize: 15
                font.weight: Font.Medium
                color: Utility.getAppHexColor("titleText")
            }
        }
    }

    footer: RowLayout {
        visible: sheet.acceptText !== "" || sheet.rejectText !== ""
        spacing: 10

        Button {
            Layout.fillWidth: true
            Layout.leftMargin: 16 + notchLeft
            Layout.rightMargin: sheet.acceptText === "" ? 16 + notchRight : 0
            Layout.topMargin: 12
            Layout.bottomMargin: 12 + notchBot
            visible: text !== ""
            text: sheet.rejectText
            flat: true
            onClicked: sheet.reject()
        }

        Button {
            Layout.fillWidth: true
            Layout.rightMargin: 16 + notchRight
            Layout.leftMargin: sheet.rejectText === "" ? 16 + notchLeft : 0
            Layout.topMargin: 12
            Layout.bottomMargin: 12 + notchBot
            visible: text !== ""
            text: sheet.acceptText
            highlighted: true
            onClicked: sheet.accept()
        }
    }
}
