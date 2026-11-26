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

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    property ConfigParams mInfoConf: VescIf.infoConfig()
    property var dialogParent: Overlay.overlay

    function openDialog() {
        dialog.open()
    }

    // Rich text from the info config in a scrolling card, implicitly as tall as its text
    component TextCard: Rectangle {
        property alias text: cardText.text

        implicitHeight: cardScroll.implicitHeight + 24
        radius: 12
        color: Utility.getAppHexColor("cardBackground")
        border.color: Utility.getAppHexColor("raisedBackground")

        ScrollView {
            id: cardScroll
            anchors.fill: parent
            anchors.topMargin: 12
            anchors.bottomMargin: 12
            leftPadding: 12
            rightPadding: 12
            clip: true
            contentWidth: availableWidth

            Text {
                id: cardText
                width: cardScroll.availableWidth
                color: Utility.getAppHexColor("lightText")
                linkColor: Utility.getAppHexColor("lightAccent")
                wrapMode: Text.WordWrap
                textFormat: Text.RichText
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }
        }
    }

    component AcceptBox: CheckBox {
        id: acceptBox
        Layout.fillWidth: true
        leftPadding: 12
        rightPadding: 12
        text: qsTr("Yes, I understand and accept")

        background: Rectangle {
            radius: 12
            color: Utility.getAppHexColor("cardBackground")
            border.width: acceptBox.checked ? 2 : 1
            border.color: acceptBox.checked ? Material.accent : Utility.getAppHexColor("raisedBackground")
        }
    }

    Component.onCompleted: {
        introText.text = mInfoConf.getDescription("wizard_startup_intro")
        usageText.text = mInfoConf.getDescription("wizard_startup_usage")
        warrantyText.text = mInfoConf.getDescription("wizard_startup_warranty")
        conclusionText.text = mInfoConf.getDescription("wizard_startup_conclusion")
    }

    Dialog {
        id: dialog
        modal: true
        focus: true
        // Covers the app bars on landscape phones
        readonly property bool fullWindow: phoneLandscape
        width: mainIsTablet ? 640 : parent.width - 10 - notchLeft - notchRight
        height: parent.height - 10 - (fullWindow ? notchTop + notchBot : 0)
        closePolicy: Popup.NoAutoClose
        x: mainIsTablet ? (parent.width - width) / 2 : 5 + notchLeft
        y: 5 + (fullWindow ? notchTop : 0)
        parent: fullWindow ? Overlay.overlay : dialogParent
        bottomMargin: 0
        rightMargin: 0
        padding: 10

        background: Rectangle {
            color: Utility.getAppHexColor("darkBackground")
            radius: 20
        }

        Overlay.modal: Rectangle {
            color: Utility.getAppHexColor("overlay")
        }

        StackLayout {
            id: stackLayout
            anchors.fill: parent

            // Each step: hint, text card as tall as its text (scrolls when longer), optional accept box
            ColumnLayout {
                StepHint {
                    Layout.bottomMargin: 4
                    title: qsTr("Before you start")
                    hint: qsTr("Read the two notices that follow and accept both to start using VESC Tool.")
                }

                TextCard {
                    id: introText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumHeight: implicitHeight
                }

                Item { Layout.fillHeight: true }
            }

            ColumnLayout {
                StepHint {
                    Layout.bottomMargin: 4
                    title: qsTr("Safe use")
                    hint: qsTr("Read how VESC Tool may be used, then tick the box below to continue.")
                }

                TextCard {
                    id: usageText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumHeight: implicitHeight
                }

                AcceptBox {
                    id: acceptUsageBox

                    onToggled: {
                        updateButtonsEnabled()
                    }
                }

                Item { Layout.fillHeight: true }
            }

            ColumnLayout {
                StepHint {
                    Layout.bottomMargin: 4
                    title: qsTr("Warranty")
                    hint: qsTr("Read the warranty terms, then tick the box below to continue.")
                }

                TextCard {
                    id: warrantyText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumHeight: implicitHeight
                }

                AcceptBox {
                    id: acceptWarrantyBox

                    onToggled: {
                        updateButtonsEnabled()
                    }
                }

                Item { Layout.fillHeight: true }
            }

            ColumnLayout {
                StepHint {
                    Layout.bottomMargin: 4
                    title: qsTr("All set")
                    hint: qsTr("Tap Finish to start using VESC Tool.")
                }

                TextCard {
                    id: conclusionText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumHeight: implicitHeight
                }

                Item { Layout.fillHeight: true }
            }
        }

        header: WizardHeader {
            title: qsTr("Welcome to VESC Tool")
            steps: [qsTr("Intro"), qsTr("Usage"), qsTr("Warranty"), qsTr("Conclusion")]
            currentIndex: stackLayout.currentIndex
        }

        footer: RowLayout {
            spacing: 0

            Button {
                id: prevButton
                Layout.margins: 10
                Layout.rightMargin: 5
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: "Cancel"
                flat: true

                onClicked: {
                    if (stackLayout.currentIndex == 0) {
                        cancelDialog.open()
                    } else {
                        stackLayout.currentIndex--
                    }

                    updateButtonText()
                }
            }

            Button {
                id: nextButton
                Layout.margins: 10
                Layout.leftMargin: 5
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: "Next"
                highlighted: true

                onClicked: {
                    if (stackLayout.currentIndex == (stackLayout.count - 1)) {
                        VescIf.setIntroDone(true)
                        dialog.close()
                    } else {
                        stackLayout.currentIndex++
                    }

                    updateButtonText()
                }
            }
        }
    }

    SheetDialog {
        id: cancelDialog
        title: qsTr("Quit VESC Tool?")
        acceptText: qsTr("Continue setup")
        rejectText: qsTr("Quit")
        closePolicy: Popup.NoAutoClose

        ScrollView {
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth

            Text {
                id: resultLabel
                color: Utility.getAppHexColor("lightText")
                verticalAlignment: Text.AlignVCenter
                width: parent.parent.width
                wrapMode: Text.WordWrap
                text: "You must finish the introduction in order to use VESC Tool."
            }
        }

        onRejected: {
            Qt.quit()
        }
    }

    function updateButtonText() {
        if (stackLayout.currentIndex == (stackLayout.count - 1)) {
            nextButton.text = "Finish"
        } else {
            nextButton.text = "Next"
        }

        if (stackLayout.currentIndex == 0) {
            prevButton.text = "Cancel"
        } else {
            prevButton.text = "Previous"
        }

        updateButtonsEnabled()
    }

    function updateButtonsEnabled() {
        if (stackLayout.currentIndex == 0) {
            nextButton.enabled = true
        } else if (stackLayout.currentIndex == 1) {
            nextButton.enabled = acceptUsageBox.checked
        } else if (stackLayout.currentIndex == 2) {
            nextButton.enabled = acceptWarrantyBox.checked
        } else if (stackLayout.currentIndex == 3) {
            nextButton.enabled = true
        }
    }
}
