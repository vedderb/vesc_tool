/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    function openDialog() {
        dialog.open()
        loadUuids()
    }

    function loadUuids() {
        pairModel.clear()
        var uuids = VescIf.getPairedUuids()
        for (var i = 0;i < uuids.length;i++) {
            pairModel.append({"uuid": uuids[i]})
        }
    }

    Dialog {
        property ConfigParams mAppConf: VescIf.appConfig()
        property Commands mCommands: VescIf.commands()

        id: dialog
        modal: true
        focus: true
        width: parent.width - 20
        height: parent.height - 60
        closePolicy: Popup.CloseOnEscape
        x: 10
        y: 50
        parent: ApplicationWindow.overlay
        padding: 10

        ColumnLayout {
            anchors.fill: parent

            Text {
                id: text
                Layout.fillWidth: true
                color: "white"
                text: qsTr("These are the VESCs paired to this instance of VESC Tool.")
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            ListModel {
                id: pairModel
            }

            ListView {
                id: pairList
                Layout.fillWidth: true
                Layout.fillHeight: true
                focus: true
                clip: true
                spacing: 5

                Component {
                    id: pairDelegate

                    Rectangle {
                        property variant modelData: model

                        width: pairList.width
                        height: 60
                        color: "#30000000"
                        radius: 5

                        RowLayout {
                            anchors.fill: parent
                            spacing: 10

                            Image {
                                id: image
                                fillMode: Image.PreserveAspectFit
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                Layout.alignment: Qt.AlignVCenter
                                Layout.leftMargin: 10
                                source: "qrc:/res/icon.png"
                            }

                            Text {
                                Layout.fillWidth: true
                                color: "white"
                                text: uuid
                                wrapMode: Text.Wrap
                            }

                            Button {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.rightMargin: 10
                                text: "Delete"

                                onClicked: {
                                    deleteDialog.open()
                                }

                                Dialog {
                                    id: deleteDialog
                                    property int indexNow: 0
                                    standardButtons: Dialog.Ok | Dialog.Cancel
                                    modal: true
                                    focus: true
                                    width: parent.width - 20
                                    closePolicy: Popup.CloseOnEscape
                                    title: "Delete paired VESC"
                                    x: 10
                                    y: 10 + parent.height / 2 - height / 2
                                    parent: ApplicationWindow.overlay

                                    Text {
                                        color: "#ffffff"
                                        verticalAlignment: Text.AlignVCenter
                                        anchors.fill: parent
                                        wrapMode: Text.WordWrap
                                        text: "This is going to delete this VESC from the paired list. If that VESC " +
                                              "has the pairing flag set you won't be able to connect to it over BLE " +
                                              "any more. Are you sure?"
                                    }

                                    onAccepted: {
                                        VescIf.deletePairedUuid(uuid)
                                        VescIf.storeSettings()
                                    }
                                }
                            }
                        }
                    }
                }

                model: pairModel
                delegate: pairDelegate
            }

            RowLayout {
                Layout.fillWidth: true
                Button {
                    Layout.preferredWidth: 50
                    Layout.fillWidth: true
                    text: "..."
                    onClicked: menu.open()

                    Menu {
                        id: menu
                        width: 500

                        MenuItem {
                            text: "Add current without pairing"
                            onTriggered: {
                                if (VescIf.isPortConnected()) {
                                    VescIf.addPairedUuid(VescIf.getConnectedUuid());
                                    VescIf.storeSettings()
                                } else {
                                    VescIf.emitMessageDialog("Add UUID",
                                                             "You are not connected to the VESC. Connect in order to add it.",
                                                             false, false)
                                }
                            }
                        }

                        MenuItem {
                            text: "Add from UUID"
                            onTriggered: {
                                uuidEnter.open()
                            }
                        }

                        MenuItem {
                            text: "Unpair connected"
                            onTriggered: {
                                if (VescIf.isPortConnected()) {
                                    if (mCommands.isLimitedMode()) {
                                        VescIf.emitMessageDialog("Unpair VESC",
                                                                 "The fiwmare must be updated to unpair this VESC.",
                                                                 false, false)
                                    } else {
                                        unpairConnectedDialog.open()
                                    }
                                } else {
                                    VescIf.emitMessageDialog("Unpair VESC",
                                                             "You are not connected to the VESC. Connect in order to unpair it.",
                                                             false, false)
                                }
                            }
                        }
                    }
                }

                Button {
                    id: pairConnectedButton
                    text: "Pair VESC"
                    Layout.fillWidth: true
                    onClicked: {
                        if (VescIf.isPortConnected()) {
                            if (mCommands.isLimitedMode()) {
                                VescIf.emitMessageDialog("Pair VESC",
                                                         "The fiwmare must be updated to pair this VESC.",
                                                         false, false)
                            } else {
                                pairConnectedDialog.open()
                            }
                        } else {
                            VescIf.emitMessageDialog("Pair VESC",
                                                     "You are not connected to the VESC. Connect in order to pair it.",
                                                     false, false)
                        }
                    }
                }

                Button {
                    text: "Close"
                    Layout.fillWidth: true
                    onClicked: {
                        dialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: pairConnectedDialog
        property int indexNow: 0
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Pair connected VESC"
        x: 10
        y: 10 + Math.max((parent.height - height) / 2, 10)
        parent: ApplicationWindow.overlay

        Text {
            color: "#ffffff"
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "This is going to pair the connected VESC with this instance of VESC Tool. VESC Tool instances " +
                  "that are not paired with this VESC will not be able to connect over bluetooth any more. Continue?"
        }

        onAccepted: {
            VescIf.addPairedUuid(VescIf.getConnectedUuid());
            VescIf.storeSettings()
            mAppConf.updateParamBool("pairing_done", true, 0)
            mCommands.setAppConf()
            if (Utility.waitSignal(mCommands, "2ackReceived(QString)", 2000)) {
                VescIf.emitMessageDialog("Pairing Successful!",
                                         "Pairing is done! Please note the UUID if this VESC (or take a screenshot) in order " +
                                         "to add it to VESC Tool instances that are not paired in the future. The UUID is:\n" +
                                         VescIf.getConnectedUuid(),
                                         true, false)
            }
        }
    }

    Dialog {
        id: unpairConnectedDialog
        property int indexNow: 0
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Unpair connected VESC"
        x: 10
        y: 10 + parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        Text {
            color: "#ffffff"
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "This is going to unpair the connected VESC. Continue?"
        }

        onAccepted: {
            VescIf.deletePairedUuid(VescIf.getConnectedUuid());
            VescIf.storeSettings()
            mAppConf.updateParamBool("pairing_done", false, 0)
            mCommands.setAppConf()
        }
    }

    Dialog {
        id: uuidEnter
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        title: "Add UUID"

        width: parent.width - 20
        height: 200
        closePolicy: Popup.CloseOnEscape
        x: 10
        y: parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        Rectangle {
            anchors.fill: parent
            height: 20
            border.width: 2
            border.color: "#8d8d8d"
            color: "#33a8a8a8"
            radius: 3
            TextInput {
                id: stringInput
                color: "#ffffff"
                anchors.fill: parent
                anchors.margins: 7
                font.pointSize: 12
                focus: true
            }
        }

        onAccepted: {
            if (stringInput.text.length > 0) {
                VescIf.addPairedUuid(stringInput.text)
            }
        }
    }

    Connections {
        target: VescIf

        onPairingListUpdated: {
            loadUuids()
        }
    }
}
