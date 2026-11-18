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
    property bool acceptEnabled: true
    property bool fullHeight: false
    // Above 0 the sheet is a full height panel right of the parent. The parent has to leave room for it
    property real dockWidth: 0
    // Moves the sheet down. Animated and dragged instead of y so that the y binding stays in effect
    property real slide: 0
    // Safe area under the sheet, none when docked or when a centred sheet is clear of it
    // (from sheetWidth, as reading width gives a binding loop through the padding)
    property real sheetWidth: Math.min(parent.width, 640)
    property real insetLeft: dockWidth > 0 ? 0 : Math.max(0, notchLeft - (parent.width - sheetWidth) / 2)
    property real insetRight: dockWidth > 0 ? 0 : Math.max(0, notchRight - (parent.width - sheetWidth) / 2)

    parent: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: dockWidth > 0 ? dockWidth : sheetWidth
    height: dockWidth > 0 ? parent.height : fullHeight ? parent.height - notchTop - 40 : Math.min(implicitHeight, parent.height - notchTop - 40)
    x: dockWidth > 0 ? parent.width : (parent.width - width) / 2
    y: parent.height - height + slide
    leftPadding: 16 + sheet.insetLeft
    rightPadding: 16 + sheet.insetRight
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
        topLeftRadius: sheet.dockWidth > 0 ? 0 : 20
        topRightRadius: sheet.dockWidth > 0 ? 0 : 20
    }

    header: ColumnLayout {
        spacing: 0

        // Dragging the header down closes the sheet like Escape does
        DragHandler {
            id: closeDrag
            target: null
            margin: 15
            enabled: sheet.dockWidth === 0 && (sheet.closePolicy & Popup.CloseOnEscape)
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
            Layout.leftMargin: 16 + sheet.insetLeft
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
            Layout.leftMargin: 16 + sheet.insetLeft
            Layout.rightMargin: sheet.acceptText === "" ? 16 + sheet.insetRight : 0
            Layout.topMargin: 12
            Layout.bottomMargin: 12 + notchBot
            visible: text !== ""
            text: sheet.rejectText
            flat: true
            onClicked: sheet.reject()
        }

        Button {
            Layout.fillWidth: true
            Layout.rightMargin: 16 + sheet.insetRight
            Layout.leftMargin: sheet.rejectText === "" ? 16 + sheet.insetLeft : 0
            Layout.topMargin: 12
            Layout.bottomMargin: 12 + notchBot
            visible: text !== ""
            enabled: sheet.acceptEnabled
            text: sheet.acceptText
            highlighted: true
            onClicked: sheet.accept()
        }
    }
}
