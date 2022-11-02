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
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import QtQuick.Window 2.2

import Vedder.vesc.utility 1.0

import "qrc:/mobile"

Item {
    id: container
    width: 100   

    // Full screen iPhone X workaround:
    property int notchLeft: 0
    property int notchRight: 0
    property int notchBot: 0
    property int notchTop: 0

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

    function dirSetup() {
        if (!VescIf.isPortConnected()) {
            VescIf.emitMessageDialog("Directions",
                                     "You are not connected to the VESC. Please connect in order " +
                                     "to map directions.", false, false)
        } else {
            enabled = false
            directionSetupDialog.open()
            directionSetup.scanCan()
            enabled = true
        }
    }

    function openMultiSettings() {
        multiSettings.openDialog()
    }

    function openWizardIMU() {
        imuWizard.openDialog()
    }

    function openBleSetup() {
        if (!VescIf.isPortConnected()) {
            VescIf.emitMessageDialog("BLE Setup",
                                     "You are not connected to the VESC.", false, false)
        } else {
            if (VescIf.getLastFwRxParams().nrfNameSupported &&
                    VescIf.getLastFwRxParams().nrfPinSupported) {
                bleSetupDialog.openDialog()
            } else {
                VescIf.emitMessageDialog("BLE Setup",
                                         "The BLE module does not support setup. You can try " +
                                         "updating the firmware on it from the SWD programmer page.",
                                         false, false)
            }
        }
    }

    SetupWizardFoc {
        id: focWizard
        dialogParent: container
    }

    SetupWizardIMU {
        id: imuWizard
        dialogParent: container
    }

    MultiSettings {
        id: multiSettings
        dialogParent: container
    }

    BleSetupDialog {
        id: bleSetupDialog
        dialogParent: container
    }

    Dialog {
        id: directionSetupDialog
        title: "Direction Setup"
        standardButtons: Dialog.Close
        modal: true
        focus: true
        padding: 10

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 10
        closePolicy: Popup.CloseOnEscape
        x: 5
        y: parent.height / 2 - height / 2
        parent: container

        DirectionSetup {
            id: directionSetup
            anchors.fill: parent
            dialogParent: container
        }
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
                color: {color = Utility.getAppHexColor("lightBackground")}
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
                PageIndicator {
                    count: rtSwipeView.count
                    currentIndex: rtSwipeView.currentIndex
                    anchors.left: parent.left
                    width:25
                    anchors.verticalCenter: parent.verticalCenter
                    rotation: 90
                    z:2
                }

                SwipeView {
                    id: rtSwipeView
                    enabled: true
                    clip: true
                    currentIndex: 0
                    anchors.fill: parent
                    orientation: Qt.Vertical

                    Page {
                        RtDataSetup {
                            anchors.fill: parent
                            dialogParent: container
                        }
                    }

                    Page {
                        StatPage {
                            anchors.fill: parent
                            anchors.margins: 20
                        }
                    }
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

    Rectangle {
        parent: container
        anchors.fill: parent
        color: "black"

        ConnectScreen {
            id: connScreen
            x: 0
            y: 0
            height: parent.height
            width: parent.width
            opened: true

            Component.onCompleted: {
                VescIf.bleDevice().emitScanDone()
            }

            onYChanged: {
                parent.color.a = Math.min(1, Math.max(1 - y / height, 0))
            }
        }
    }

    Connections {
        target: VescIf
        function onPortConnectedChanged() {
            connScreen.opened = VescIf.isPortConnected() ? false : true
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

        function onFwRxChanged(rx, limited) {
            updateHwUi()
            updateAppUi()
        }

        function onQmlLoadDone() {
            if (VescIf.askQmlLoad()) {
                qmlLoadDialog.open()
            } else {
                updateHwUi()
                updateAppUi()
            }
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

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        parent: container
        y: parent.y + parent.height / 2 - height / 2

        Text {
            color: {color = Utility.getAppHexColor("lightText")}
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

    Dialog {
        id: qmlLoadDialog
        standardButtons: Dialog.Yes | Dialog.Cancel
        modal: true
        focus: true
        rightMargin: 10
        leftMargin: 10
        closePolicy: Popup.CloseOnEscape
        title: "Load Custom User Interface"

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        parent: container
        y: parent.y + parent.height / 2 - height / 2

        ColumnLayout {
            anchors.fill: parent

            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Utility.getAppHexColor("lightText")
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text:
                    "The hardware you are connecting to contains code that will alter the " +
                    "user interface of VESC Tool. This code has not been verified by the " +
                    "authors of VESC Tool and could contain bugs and security problems. \n\n" +
                    "Do you want to load this custom user interface?"
            }

            CheckBox {
                Layout.fillWidth: true
                id: qmlDoNotAskAgainBox
                text: "Load without asking"
            }
        }

        onAccepted: {
            VescIf.setAskQmlLoad(!qmlDoNotAskAgainBox.checked)
            updateHwUi()
            updateAppUi()
        }

        onRejected: {
            VescIf.disconnectPort()
        }
    }
}
