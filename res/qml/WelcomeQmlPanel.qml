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
import QtQuick.Controls.Material 2.2
import QtQuick.Window 2.2

import Vedder.vesc.utility 1.0

import "qrc:/mobile"

Item {
    id: container
    width: 100
    Material.theme: Utility.isDarkMode() ? "Dark" : "Light"
    Material.accent: Utility.getAppHexColor("lightAccent")


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
                color: Utility.getAppHexColor("lightBackground")
            }

            property int buttonWidth: Math.max(120,
                                               tabBar.width /
                                               (rep.model.length +
                                                (uiHwPage.visible ? 1 : 0) +
                                               (uiAppPage.visible ? 1 : 0)))

            Repeater {
                id: rep
                model: ["RT Data", "Profiles"]

                TabButton {
                    text: modelData
                    width: tabBar.buttonWidth
                }
            }
        }

        TabButton {
            id: uiHwButton
            visible: uiHwPage.visible
            text: "HwUi"
            width: tabBar.buttonWidth
        }

        Page {
            id: uiHwPage
            visible: false

            Item {
                id: uiHw
                anchors.fill: parent
                property var tabBarItem: tabBar
            }
        }

        TabButton {
            id: uiAppButton
            visible: uiAppPage.visible
            text: "AppUi"
            width: tabBar.buttonWidth
        }

        Page {
            id: uiAppPage
            visible: false

            Item {
                id: uiApp
                anchors.fill: parent
                property var tabBarItem: tabBar
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

    property var hwUiObj: 0

    function updateHwUi () {
        if (hwUiObj != 0) {
            hwUiObj.destroy()
            hwUiObj = 0
        }

        swipeView.interactive = true
        tabBar.visible = true
        tabBar.enabled = true

        if (VescIf.isPortConnected() && VescIf.qmlHwLoaded()) {
            if (VescIf.getLastFwRxParams().qmlHwFullscreen) {
                swipeView.interactive = false
                tabBar.visible = false
                tabBar.enabled = false
            }

            hwUiObj = Qt.createQmlObject(VescIf.qmlHw(), uiHw, "HwUi")
            uiHwButton.visible = true
            swipeView.insertItem(0, uiHwPage)
            tabBar.insertItem(0, uiHwButton)
            uiHwPage.visible = true
            swipeView.setCurrentIndex(1)
            swipeView.setCurrentIndex(0)
        } else {
            uiHwPage.visible = false
            uiHwPage.parent = null
            uiHwButton.parent = null
        }
    }

    property var appUiObj: 0

    function updateAppUi () {
        if (appUiObj != 0) {
            appUiObj.destroy()
            appUiObj = 0
        }

        swipeView.interactive = true
        tabBar.visible = true
        tabBar.enabled = true

        if (VescIf.isPortConnected() && VescIf.qmlAppLoaded()) {
            if (VescIf.getLastFwRxParams().qmlAppFullscreen) {
                swipeView.interactive = false
                tabBar.visible = false
                tabBar.enabled = false
            }

            appUiObj = Qt.createQmlObject(VescIf.qmlApp(), uiApp, "AppUi")
            uiAppButton.visible = true
            swipeView.insertItem(0, uiAppPage)
            tabBar.insertItem(0, uiAppButton)
            uiAppPage.visible = true
            swipeView.setCurrentIndex(1)
            swipeView.setCurrentIndex(0)
        } else {
            uiAppPage.visible = false
            uiAppPage.parent = null
            uiAppButton.parent = null
        }
    }

    Connections {
        target: VescIf

        onFwRxChanged: {
            updateHwUi()
            updateAppUi()
        }

        onQmlLoadDone: {
            updateHwUi()
            updateAppUi()
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
            color: Utility.getAppHexColor("lightText")
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
