/*
    Copyright 2018 - 2019 Benjamin Vedder	benjamin@vedder.se

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

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.utility 1.0

Item {
    id: topItem
    property var dialogParent: Overlay.overlay

    function updateVisibleProfiles() {
        for(var i = scrollCol.children.length;i > 0;i--) {
            scrollCol.children[i - 1].destroy(1) // Only works with delay on android, seems to be a bug
        }

        var prof = VescIf.getProfiles()
        for (i = 0;i < prof.length;i++) {
            var component = Qt.createComponent("ProfileDisplay.qml");
            var disp = component.createObject(scrollCol, {"index": i, "dialogParent": dialogParent})
            disp.setFromMcConfTemp(prof[i])
            disp.editRequested.connect(handleProfileEdit)
            disp.deleteRequested.connect(handleProfileDelete)
            disp.checkActive()
        }
    }

    function handleProfileEdit(index) {
        editButtonEditor.indexNow = index
        editButtonEditor.updateFromMcConfTemp(VescIf.getProfile(index))
        editButtonEditor.openDialog()
    }

    function handleProfileDelete(index) {
        deleteDialog.indexNow = index
        deleteDialog.open()
    }

    Component.onCompleted: {
        updateVisibleProfiles()
    }

    ProfileEditor {
        id: editor
        dialogParent: topItem.dialogParent

        onClosed: {
            if (ok) {
                VescIf.addProfile(getMcConfTemp())
                VescIf.storeSettings()
            }
        }
    }

    ProfileEditor {
        id: editButtonEditor
        dialogParent: topItem.dialogParent
        property int indexNow: 0

        onClosed: {
            if (ok) {
                VescIf.updateProfile(indexNow, getMcConfTemp())
                VescIf.storeSettings()
            }
        }
    }

    SheetDialog {
        id: deleteDialog
        property int indexNow: 0
        title: "Remove profile"
        acceptText: qsTr("Remove")
        parent: topItem.dialogParent

        Text {
            color:{color = Utility.getAppHexColor("lightText")}
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "This is going to delete this profile. Are you sure?"
        }

        onAccepted: {
            VescIf.deleteProfile(indexNow)
            VescIf.storeSettings()
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 0

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 8

                ColumnLayout {
                    id: scrollCol
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    spacing: 8
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 10
                    implicitHeight: hintText.implicitHeight + 32
                    radius: 12
                    color: "transparent"
                    border.color: Utility.getAppHexColor("lightestBackground")
                    border.width: 1.5

                    Text {
                        id: hintText
                        anchors.centerIn: parent
                        width: parent.width - 32
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        font.pointSize: 10
                        color: Utility.getAppHexColor("normalText")
                        text: qsTr("Profiles switch speed, current and power limits in one tap. " +
                                   "New profiles start from the current motor configuration.")
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.bottomMargin: 8

            Button {
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                highlighted: true
                text: "Add Profile"

                onClicked: {
                    editor.profileName = "New profile"
                    editor.readCurrentConfig()
                    editor.openDialog()
                }
            }

            Button {
                Layout.preferredWidth: 56
                text: "..."
                onClicked: menu.open()

                BottomMenu {
                    id: menu
                    parent: topItem

                    MenuItem {
                        text: "Remove All Profiles"
                        onTriggered: {
                            clearDialog.open()
                        }
                    }
                }
            }
        }
    }

    SheetDialog {
        id: clearDialog
        title: "Remove all profiles"
        acceptText: qsTr("Remove all")

        Text {
            color: {color = Utility.getAppHexColor("lightText")}
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text:
                "This is going to remove all your profiles. Are you sure?"
        }

        onAccepted: {
            VescIf.clearProfiles()
            VescIf.storeSettings()
        }
    }

    Connections {
        target: VescIf

        function onProfilesUpdated() {
            updateVisibleProfiles()
        }
    }
}
