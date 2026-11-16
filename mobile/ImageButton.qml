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

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts 1.3
import Vedder.vesc.utility 1.0

Item {
    id: rootItem
    property string imageSrc: ""
    property string buttonText: ""
    property string hintText: ""
    property bool locked: false
    property bool badge: false
    signal clicked()

    Button {
        id: control
        anchors.fill: parent
        enabled: !rootItem.locked
        flat: true
        focusPolicy: Qt.NoFocus

        onClicked: {
            rootItem.clicked()
        }

        background: Rectangle {
            anchors.fill: parent
            radius: 12
            property color hoveredColor: {hoveredColor = Utility.getAppHexColor("lightBackground")}
            property color pressedColor: {pressedColor = Utility.getAppHexColor("darkAccent")}
            property color normalColor: {normalColor = Utility.getAppHexColor("cardBackground")}
            color: control.pressed ? pressedColor: control.hovered ? hoveredColor : normalColor
            border.color: {border.color = Utility.getAppHexColor("raisedBackground")}
            border.width: control.activeFocus ? 2 : 1
            opacity: rootItem.locked ? 0.45 : 1
            Behavior on color {
                ColorAnimation {
                    duration: 200;
                    easing.type: Easing.InOutSine
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 11
                anchors.rightMargin: 14
                spacing: 11

                Rectangle {
                    Layout.preferredHeight: 38
                    Layout.preferredWidth: height
                    radius: 12
                    color: Utility.getAppHexColor("raisedBackground")

                    Image {
                        id: img
                        anchors.centerIn: parent
                        width: parent.width * 0.68
                        height: width
                        fillMode: Image.PreserveAspectFit
                        source: imageSrc
                        smooth: true
                    }

                    Rectangle {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        width: 12
                        height: 12
                        radius: 6
                        visible: rootItem.badge
                        color: Material.accent
                        border.width: 2
                        border.color: Utility.getAppHexColor("cardBackground")
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    Text {
                        Layout.fillWidth: true
                        text: buttonText
                        wrapMode: Text.WordWrap
                        font.weight: Font.Medium
                        color: Utility.getAppHexColor("lightText")
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: text !== ""
                        text: rootItem.locked ? qsTr("Connect to use") : hintText
                        elide: Text.ElideRight
                        font.pointSize: 9
                        color: Utility.getAppHexColor("normalText")
                    }
                }

                Image {
                    Layout.preferredWidth: 14
                    Layout.preferredHeight: 14
                    visible: rootItem.locked
                    source: "qrc" + Utility.getThemePath() + "icons/Lock-96.png"
                }
            }
        }
    }
}
