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
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3
import Qt5Compat.GraphicalEffects

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    property ConfigParams mInfoConf: VescIf.infoConfig()
    property var dialogParent: Overlay.overlay

    function openDialog() {
        dialog.open()
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
        width: mainIsTablet ? 640 : parent.width - 10 - notchLeft - notchRight
        height: parent.height - 10
        closePolicy: Popup.NoAutoClose
        x: mainIsTablet ? (parent.width - width) / 2 : 5 + (notchLeft + notchRight) / 2
        y: 5
        parent: dialogParent
        bottomMargin: 0
        rightMargin: 0
        padding: 10

        background: Rectangle {
            color: Utility.getAppHexColor("darkBackground")
            radius: 20
        }

        Overlay.modal: Rectangle {
            color: scrimColor
        }

        StackLayout {
            id: stackLayout
            anchors.fill: parent

            Item {
                Text {
                    id: introText
                    color: Utility.getAppHexColor("lightText")
                    linkColor: "lightblue"
                    verticalAlignment: Text.AlignTop
                    anchors.fill: parent
                    wrapMode: Text.WordWrap
                    textFormat: Text.RichText
                    onLinkActivated: {
                        Qt.openUrlExternally(link)
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent

                    ScrollView {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        clip: true
                        contentWidth: availableWidth

                        Text {
                            id: usageText
                            color: Utility.getAppHexColor("lightText")
                            linkColor: "lightblue"
                            verticalAlignment: Text.AlignVCenter
                            width: parent.parent.width - 20
                            wrapMode: Text.WordWrap
                            textFormat: Text.RichText
                            onLinkActivated: {
                                Qt.openUrlExternally(link)
                            }
                        }
                    }

                    CheckBox {
                        id: acceptUsageBox
                        Layout.fillWidth: true
                        text: "Yes, I understand and accept"

                        onToggled: {
                            updateButtonsEnabled()
                        }
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent

                    ScrollView {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        clip: true
                        contentWidth: parent.width - 20

                        Text {
                            id: warrantyText
                            color: Utility.getAppHexColor("lightText")
                            linkColor: "lightblue"
                            verticalAlignment: Text.AlignVCenter
                            width: parent.parent.width - 20
                            wrapMode: Text.WordWrap
                            textFormat: Text.RichText
                            onLinkActivated: {
                                Qt.openUrlExternally(link)
                            }
                        }
                    }

                    CheckBox {
                        id: acceptWarrantyBox
                        Layout.fillWidth: true
                        text: "Yes, I understand and accept"

                        onToggled: {
                            updateButtonsEnabled()
                        }
                    }
                }
            }

            Item {
                Text {
                    id: conclusionText
                    color: Utility.getAppHexColor("lightText")
                    linkColor: "lightblue"
                    verticalAlignment: Text.AlignTop
                    anchors.fill: parent
                    wrapMode: Text.WordWrap
                    textFormat: Text.RichText
                    onLinkActivated: {
                        Qt.openUrlExternally(link)
                    }
                }
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
        acceptText: qsTr("Continue setup")
        rejectText: qsTr("Quit")
        closePolicy: Popup.NoAutoClose

        ScrollView {
            anchors.fill: parent
            clip: true
            contentWidth: parent.width - 20

            Text {
                id: resultLabel
                color: Utility.getAppHexColor("lightText")
                verticalAlignment: Text.AlignVCenter
                width: parent.parent.width - 20
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
