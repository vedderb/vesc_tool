/*
    Copyright 2017 Benjamin Vedder	benjamin@vedder.se

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

Item {
    property alias scanButton: scanButton
    property alias connectButton: connectButton
    property alias disconnectButton: disconnectButton
    property alias setNameButton: setNameButton
    property alias bleItems: bleItems
    property alias bleBox: bleBox
    property alias canIdBox: canIdBox
    property alias fwdCanBox: fwdCanBox

    id: item1
    width: 400
    height: 400

    ColumnLayout {
        anchors.fill: parent

        Image {
            id: image
            Layout.preferredWidth: Math.min(parent.width, parent.height)
            Layout.preferredHeight: (394 * Layout.preferredWidth) / 1549
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            source: "qrc:/res/logo.png"
        }

        Item {
            // Spacer
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        GridLayout {
            clip: false
            visible: true
            rowSpacing: 0
            columnSpacing: 5
            rows: 5
            columns: 2

            Label {
                id: connectionName
                text: "BLE Connection"
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                Layout.bottomMargin: 5
                Layout.columnSpan: 2
            }

            ComboBox {
                id: bleBox
                Layout.columnSpan: 2
                Layout.preferredHeight: 48
                Layout.fillHeight: false
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                transformOrigin: Item.Center

                textRole: "key"
                model: ListModel {
                    id: bleItems
                }
            }

            Button {
                id: scanButton
                text: qsTr("Scan")
                Layout.columnSpan: 2
                Layout.preferredHeight: 48
                Layout.fillWidth: true
            }

            Button {
                id: connectButton
                text: qsTr("Connect")
                enabled: false
                Layout.preferredHeight: 48
                Layout.preferredWidth: 100
                Layout.fillWidth: true
            }

            Button {
                id: disconnectButton
                text: qsTr("Disconnect")
                enabled: false
                Layout.preferredHeight: 48
                Layout.preferredWidth: 100
                Layout.fillWidth: true
            }

            Button {
                id: setNameButton
                text: qsTr("Set Name")
                Layout.columnSpan: 2
                Layout.preferredHeight: 48
                Layout.preferredWidth: 100
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.fillWidth: true

            CheckBox {
                id: fwdCanBox
                text: qsTr("CAN Forward")
            }

            SpinBox {
                id: canIdBox
                Layout.fillWidth: true
            }
        }

        Item {
            // Spacer
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Item {
            // Spacer
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
