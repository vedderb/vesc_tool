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

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.fwhelper 1.0
import Vedder.vesc.utility 1.0

Item {
    property Commands mCommands: VescIf.commands()
    property ConfigParams mInfoConf: VescIf.infoConfig()

    FwHelper {
        id: fwHelper
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true

            PageIndicator {
                id: indicator

                count: swipeView.count
                currentIndex: swipeView.currentIndex

                Layout.preferredWidth: 15
                Layout.alignment: Qt.AlignHCenter |  Qt.AlignVCenter
                rotation: 90
            }

            SwipeView {
                id: swipeView
                enabled: true
                clip: true

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.rightMargin: 15
                orientation: Qt.Vertical

                Page {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10

                        Rectangle {
                            Layout.fillWidth: true
                            height: 30;
                            border.width: 0
                            gradient: Gradient {
                                GradientStop {
                                    position: 0
                                    color: "#002dcbff"
                                }

                                GradientStop {
                                    position: 0.534
                                    color: "#4c4de43a"
                                }

                                GradientStop {
                                    position: 1
                                    color: "#000dc3ff"
                                }

                            }
                            border.color: "#00000000"

                            Text {
                                anchors.centerIn: parent
                                text: "Included Files"
                                font.bold: true
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        Item {
                            // Spacer
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }

                        Text {
                            Layout.fillWidth: true
                            height: 30;
                            text: "Hardware"
                            horizontalAlignment: Text.AlignHCenter
                        }

                        ComboBox {
                            id: hwBox
                            Layout.preferredHeight: 48
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            textRole: "key"
                            model: ListModel {
                                id: hwItems
                            }

                            Component.onCompleted: {
                                updateHw("")
                            }

                            onCurrentIndexChanged: {
                                if (hwItems.rowCount() === 0) {
                                    return
                                }

                                var fws = fwHelper.getFirmwares(hwItems.get(hwBox.currentIndex).value)

                                fwItems.clear()

                                for (var name in fws) {
                                    if (name.toLowerCase().indexOf("vesc_default.bin") !== -1) {
                                        fwItems.insert(0, { key: name, value: fws[name] })
                                    } else {
                                        fwItems.append({ key: name, value: fws[name] })
                                    }
                                }

                                fwBox.currentIndex = 0
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            height: 30;
                            text: "Firmware"
                            horizontalAlignment: Text.AlignHCenter
                        }

                        ComboBox {
                            id: fwBox
                            Layout.preferredHeight: 48
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            textRole: "key"
                            model: ListModel {
                                id: fwItems
                            }
                        }

                        Button {
                            text: "Show Changelog"
                            Layout.fillWidth: true

                            onClicked: {
                                VescIf.emitMessageDialog(
                                            "Firmware Changelog",
                                            Utility.fwChangeLog(),
                                            true)
                            }
                        }

                        Item {
                            // Spacer
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }
                }

                Page {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10

                        Rectangle {
                            Layout.fillWidth: true
                            height: 30;
                            border.width: 0
                            gradient: Gradient {
                                GradientStop {
                                    position: 0
                                    color: "#002dcbff"
                                }

                                GradientStop {
                                    position: 0.534
                                    color: "#4c4de43a"
                                }

                                GradientStop {
                                    position: 1
                                    color: "#000dc3ff"
                                }

                            }
                            border.color: "#00000000"

                            Text {
                                anchors.centerIn: parent
                                text: "Custom File"
                                font.bold: true
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        Item {
                            // Spacer
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }

                        TextInput {
                            id: customFwText
                            Layout.fillWidth: true
                        }

                        Button {
                            text: "Choose File..."
                            Layout.fillWidth: true

                            onClicked: {
                                if (Utility.requestFilePermission()) {
                                    filePicker.enabled = true
                                    filePicker.visible = true
                                } else {
                                    VescIf.emitMessageDialog(
                                                "File Permissions",
                                                "Unable to request file system permission.",
                                                false, false)
                                }
                            }
                        }

                        FilePicker {
                            id: filePicker
                            anchors.fill: parent
                            showDotAndDotDot: true
                            nameFilters: "*.bin"
                            visible: false
                            enabled: false

                            onFileSelected: {
                                customFwText.text = currentFolder() + "/" + fileName
                                visible = false
                                enabled = false
                            }
                        }

                        Item {
                            // Spacer
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }
                }

                Page {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10

                        Rectangle {
                            Layout.fillWidth: true
                            height: 30;
                            border.width: 0
                            gradient: Gradient {
                                GradientStop {
                                    position: 0
                                    color: "#002dcbff"
                                }

                                GradientStop {
                                    position: 0.534
                                    color: "#4c4de43a"
                                }

                                GradientStop {
                                    position: 1
                                    color: "#000dc3ff"
                                }

                            }
                            border.color: "#00000000"

                            Text {
                                anchors.centerIn: parent
                                text: "Bootloader"
                                font.bold: true
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        Item {
                            // Spacer
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }

                        Text {
                            Layout.fillWidth: true
                            height: 30;
                            text: "Hardware"
                            horizontalAlignment: Text.AlignHCenter
                        }

                        ComboBox {
                            id: blBox
                            Layout.preferredHeight: 48
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            textRole: "key"
                            model: ListModel {
                                id: blItems
                            }

                            Component.onCompleted: {
                                updateBl("")
                            }
                        }

                        Item {
                            // Spacer
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: asd.implicitHeight + 20
            gradient: Gradient {
                GradientStop {
                    position: 0.00;
                    color: "#e4eaff";
                }
                GradientStop {
                    position: 0.91;
                    color: "#d8e4d2";
                }
                GradientStop {
                    position: 1.00;
                    color: "#a4c096";
                }
            }

            ColumnLayout {
                id: asd
                anchors.fill: parent
                anchors.margins: 10

                Text {
                    Layout.fillWidth: true
                    id: uploadText
                    text: qsTr("Not Uploading")
                    horizontalAlignment: Text.AlignHCenter
                }

                ProgressBar {
                    id: uploadProgress
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        id: uploadButton
                        text: qsTr("Upload")
                        Layout.fillWidth: true

                        onClicked: {
                            if (!VescIf.isPortConnected()) {
                                VescIf.emitMessageDialog(
                                            "Connection Error",
                                            "The VESC is not connected. Please open a connection.",
                                            false)
                                return
                            }

                            if (swipeView.currentIndex == 0) {
                                if (fwItems.rowCount() === 0) {
                                    VescIf.emitMessageDialog(
                                                "Upload Error",
                                                "This version of VESC Tool does not include any firmware " +
                                                "for your hardware version. You can either " +
                                                "upload a custom file or look for a later version of VESC " +
                                                "Tool that might support your hardware.",
                                                false)
                                    return;
                                }

                                if (hwItems.rowCount() === 1) {
                                    uploadDialog.title = "Warning"
                                    uploadDialogLabel.text =
                                            "Uploading new firmware will clear all settings on your VESC " +
                                            "and you have to do the configuration again. Do you want to " +
                                            "continue?"
                                    uploadDialog.open()
                                } else {
                                    uploadDialog.title = "Warning"
                                    uploadDialogLabel.text =
                                            "Uploading firmware for the wrong hardware version " +
                                            "WILL damage the VESC for sure. Are you sure that you have " +
                                            "chosen the correct hardware version?"
                                    uploadDialog.open()
                                }
                            } else if (swipeView.currentIndex == 1) {
                                if (customFwText.text.length > 0) {
                                    uploadDialog.title = "Warning"
                                    uploadDialogLabel.text =
                                            "Uploading firmware for the wrong hardware version " +
                                            "WILL damage the VESC for sure. Are you sure that you have " +
                                            "chosen the correct hardware version?"
                                    uploadDialog.open()
                                } else {
                                    VescIf.emitMessageDialog(
                                                "Error",
                                                "Please select a file",
                                                false, false)
                                }
                            } else if (swipeView.currentIndex == 2) {
                                if (blItems.rowCount() === 0) {
                                    VescIf.emitMessageDialog(
                                                "Upload Error",
                                                "This version of VESC Tool does not include any bootloader " +
                                                "for your hardware version.",
                                                false)
                                    return;
                                }

                                uploadDialog.title = "Warning"
                                uploadDialogLabel.text =
                                        "This will attempt to upload a bootloader to the connected VESC. " +
                                        "If the connected VESC already has a bootloader this will destroy " +
                                        "the bootloader and firmware updates cannot be done anymore. Do " +
                                        "you want to continue?"
                                uploadDialog.open()
                            }
                        }
                    }

                    Button {
                        id: cancelButton
                        text: qsTr("Cancel")
                        Layout.fillWidth: true
                        enabled: false

                        onClicked: {
                            mCommands.cancelFirmwareUpload()
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    id: versionText
                    color: "#606060"
                    text:
                        "FW   : \n" +
                        "HW   : \n" +
                        "UUID : "
                    font.family: "DejaVu Sans Mono"
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    Dialog {
        id: uploadDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        Text {
            id: uploadDialogLabel
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            if (swipeView.currentIndex == 0) {
                fwHelper.uploadFirmware(fwItems.get(fwBox.currentIndex).value, VescIf, false, false)
            } else if (swipeView.currentIndex == 1) {
                fwHelper.uploadFirmware(customFwText.text, VescIf, false, true)
            } else if (swipeView.currentIndex == 2) {
                fwHelper.uploadFirmware(blItems.get(blBox.currentIndex).value, VescIf, true, false)
            }
        }
    }

    function updateHw(hw) {
        var hws = fwHelper.getHardwares(hw)

        hwItems.clear()

        for (var name in hws) {
            if (name.indexOf("412") !== -1) {
                hwItems.insert(0, { key: name, value: hws[name] })
            } else {
                hwItems.append({ key: name, value: hws[name] })
            }
        }

        hwBox.currentIndex = 0
    }

    function updateBl(hw) {
        var bls = fwHelper.getBootloaders(hw)

        blItems.clear()

        for (var name in bls) {
            if (name.indexOf("412") !== -1) {
                blItems.insert(0, { key: name, value: bls[name] })
            } else {
                blItems.append({ key: name, value: bls[name] })
            }
        }

        blBox.currentIndex = 0
    }

    Connections {
        target: VescIf

        onFwUploadStatus: {
            if (isOngoing) {
                uploadText.text = status + " (" + parseFloat(progress * 100.0).toFixed(2) + " %)"
            } else {
                uploadText.text = status
            }

            uploadProgress.value = progress
            uploadButton.enabled = !isOngoing
            cancelButton.enabled = isOngoing
        }
    }

    Connections {
        target: mCommands

        onFwVersionReceived: {
            updateHw(hw)
            updateBl(hw)

            versionText.text =
                    "FW   : " + major + "." + minor + "\n" +
                    "HW   : " + hw + "\n" +
                    "UUID : " + Utility.uuid2Str(uuid, false)
        }
    }
}
