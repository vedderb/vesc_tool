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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0

Item {
    function updateVisibleProfiles() {
        var i = 0

        for(var i = scrollCol.children.length;i > 0;i--) {
            scrollCol.children[i - 1].destroy(1) // Only works with delay on android, seems to be a bug
        }

        var prof = VescIf.getProfiles()
        for (i = 0;i < prof.length;i++) {
            var component = Qt.createComponent("ProfileDisplay.qml");
            var disp = component.createObject(scrollCol, {"index": i})
            disp.setFromMcConfTemp(prof[i])
            disp.editRequested.connect(handleProfileEdit)
            disp.deleteRequested.connect(handleProfileDelete)
            disp.checkActive()
        }

        Qt.createQmlObject(
                    'import QtQuick 2.7; import QtQuick.Layouts 1.3; Rectangle {Layout.fillHeight: true}',
                    scrollCol, "spacer1")
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

        onClosed: {
            if (ok) {
                VescIf.addProfile(getMcConfTemp())
                VescIf.storeSettings()
            }
        }
    }

    ProfileEditor {
        id: editButtonEditor
        property int indexNow: 0

        onClosed: {
            if (ok) {
                VescIf.updateProfile(indexNow, getMcConfTemp())
                VescIf.storeSettings()
            }
        }
    }

    Dialog {
        id: deleteDialog
        property int indexNow: 0
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Remove profile"
        x: 10
        y: 10 + parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        Text {
            color: "#ffffff"
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
            contentWidth: column.width
            clip: true

            ColumnLayout {
                id: scrollCol
                anchors.fill: parent
                anchors.topMargin: 5
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Button {
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                text: "Add Profile"

                onClicked: {
                    editor.profileName = "New profile"
                    editor.readCurrentConfig()
                    editor.openDialog()
                }
            }

            Button {
                Layout.preferredWidth: 50
                Layout.fillWidth: true
                text: "..."
                onClicked: menu.open()

                Menu {
                    id: menu
                    width: 500

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

    Dialog {
        id: clearDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: column.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Remove all profiles"

        x: 10
        y: column.y + column.height / 2 - height / 2

        Text {
            color: "#ffffff"
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

        onProfilesUpdated: {
            updateVisibleProfiles()
        }
    }
}
