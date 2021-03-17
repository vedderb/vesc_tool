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
import QtQuick.Controls.Material 2.12
import QtQuick.Window 2.2

import "qrc:/mobile"

Item {
    id: container
    width: 100

    function setupMotors() {
        if (!VescIf.isPortConnected()) {
            VescIf.emitMessageDialog("Setup motors FOC",
                                     "Not connected. Please connect in order to run the FOC wizard.",
                                     false, false)
        } else {
            focWizard.openDialog()
        }
    }

    function nrfQuickPair() {
        if (!VescIf.isPortConnected()) {
            VescIf.emitMessageDialog("NRF Quick Pair",
                                     "You are not connected to the VESC. Please connect in order " +
                                     "to quick pair an NRF-based remote.", false, false)
        } else {
            nrfPairStartDialog.open()
        }
    }

    SetupWizardFoc {
        id: focWizard
        dialogParent: container
    }

    Rectangle {
        anchors.fill: parent
        color: Material.background
    }

    ColumnLayout {
        id: gaugeColumn
        anchors.fill: parent

        TabBar {
            id: tabBar
            currentIndex: swipeView.currentIndex
            Layout.fillWidth: true
            implicitWidth: 0
            clip: true

            background: Rectangle {
                opacity: 1
                color: "#4f4f4f"
            }

            property int buttonWidth: 120

            Repeater {
                id: rep
                model: ["RT Data", "Profiles"]

                TabButton {
                    text: modelData
                    width: Math.max(tabBar.buttonWidth, tabBar.width / rep.model.length)
                }
            }
        }

        SwipeView {
            id: swipeView
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Page {
                RtDataSetup {
                    anchors.fill: parent
                    dialogParent: container
                }
            }

            Page {
                Profiles {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    dialogParent: container
                }
            }
        }

        NrfPair {
            id: nrfPair
            Layout.fillWidth: true
            Layout.preferredWidth: 500
            visible: false
            hideAfterPair: true
        }
    }

    Dialog {
        id: nrfPairStartDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        rightMargin: 10
        leftMargin: 10
        closePolicy: Popup.CloseOnEscape
        title: "NRF Pairing"

        parent: container
        y: parent.y + parent.height / 2 - height / 2

        Text {
            color: "white"
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text:
                "After clicking OK the VESC will be put in pairing mode for 10 seconds. Switch" +
                "on your remote during this time to complete the pairing process."
        }

        onAccepted: {
            nrfPair.visible = true
            nrfPair.startPairing()
        }
    }
}
