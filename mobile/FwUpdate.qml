/*
    Copyright 2017 - 2021 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick 2.11
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.3 as Dl

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.fwhelper 1.0
import Vedder.vesc.utility 1.0

Item {
    property Commands mCommands: VescIf.commands()
    property ConfigParams mInfoConf: VescIf.infoConfig()
    property bool isHorizontal: width > height
    property bool showUploadAllButton: true
    anchors.fill: parent

    FwHelper {
        id: fwHelper
    }

    GridLayout {
        anchors.fill: parent
        columns: isHorizontal ? 2 : 1
        rows: isHorizontal ? 1 : 2

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.column: isHorizontal ? 1 : 0
            Layout.row: 0
            Layout.preferredWidth: isHorizontal ? parent.width/2 : parent.width
            spacing: 0

            Rectangle {
                color: Utility.getAppHexColor("normalBackground")
                width: 16
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter |  Qt.AlignVCenter

                PageIndicator {
                    id: indicator
                    count: swipeView.count
                    currentIndex: swipeView.currentIndex
                    anchors.centerIn: parent
                    rotation: 90
                }
            }

            SwipeView {
                id: swipeView
                enabled: true
                clip: true

                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Vertical

                contentItem: ListView {
                    model: swipeView.contentModel
                    interactive: swipeView.interactive
                    currentIndex: swipeView.currentIndex

                    spacing: swipeView.spacing
                    orientation: swipeView.orientation
                    snapMode: ListView.SnapOneItem
                    boundsBehavior: Flickable.StopAtBounds

                    highlightRangeMode: ListView.StrictlyEnforceRange
                    preferredHighlightBegin: 0
                    preferredHighlightEnd: 0
                    highlightMoveDuration: 250

                    maximumFlickVelocity: 8 * (swipeView.orientation ===
                                               Qt.Horizontal ? width : height)
                }

                Page {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        anchors.topMargin: 10

                        Rectangle {
                            Layout.fillWidth: true
                            height: 30;
                            border.width: 0
                            color: "#55" + Utility.getAppHexColor("darkAccent").slice(1)
                            border.color: Utility.getAppHexColor("lightestBackground")

                            Text {
                                anchors.centerIn: parent
                                color: Utility.getAppHexColor("lightText")
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
                            color: Utility.getAppHexColor("lightText")
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
                                updateHw(VescIf.getLastFwRxParams())
                                var params = VescIf.getLastFwRxParams()

                                updateHw(params)
                                updateBl(params)

                                var testFwStr = "";
                                var fwNameStr = "";

                                if (params.isTestFw > 0) {
                                    testFwStr = " BETA " +  params.isTestFw
                                }

 
                                if (params.fwName !== "") {
                                    fwNameStr = " (" + params.fwName + ")"
                                }
                                    
                                versionText.text =
                                        "FW   : v" + params.major + "." + params.minor + fwNameStr + testFwStr + "\n" +
                                        "HW   : " + params.hw + "\n" +
                                        "UUID : " + Utility.uuid2Str(params.uuid, false)
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
                            color: Utility.getAppHexColor("lightText")
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
                        anchors.topMargin: 10

                        Rectangle {
                            Layout.fillWidth: true
                            height: 30;
                            border.width: 0
                            color: "#55" + Utility.getAppHexColor("darkAccent").slice(1)

                            border.color: "#00000000"

                            Text {
                                anchors.centerIn: parent
                                color: Utility.getAppHexColor("lightText")
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
                            color: Utility.getAppHexColor("lightText")
                            id: customFwText
                            Layout.fillWidth: true
                        }

                        Button {
                            text: "Choose File..."
                            Layout.fillWidth: true

                            onClicked: {
                                if (Utility.requestFilePermission()) {
                                    fileDialog.close()
                                    fileDialog.open()
                                } else {
                                    VescIf.emitMessageDialog(
                                                "File Permissions",
                                                "Unable to request file system permission.",
                                                false, false)
                                }
                            }
                        }

                        Item {
                            // Spacer
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    Dl.FileDialog {
                        id: fileDialog
                        title: "Choose a firmware file"
                        nameFilters: ["*"]
                        selectedNameFilter: "*"
                        onAccepted: {
                            customFwText.text = fileUrl
                            close()
                            parent.forceActiveFocus()
                        }
                        onRejected: {
                            close()
                            parent.forceActiveFocus()
                        }
                    }
                }

                Page {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        anchors.topMargin: 10

                        Rectangle {
                            Layout.fillWidth: true
                            height: 30;
                            border.width: 0
                            color: "#55" + Utility.getAppHexColor("darkAccent").slice(1)
                            border.color: "#00000000"

                            Text {
                                anchors.centerIn: parent
                                color: Utility.getAppHexColor("lightText")
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
                            color: Utility.getAppHexColor("lightText")
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
                                updateBl(VescIf.getLastFwRxParams())
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
            Layout.fillHeight: isHorizontal ? true : false
            Layout.preferredWidth: isHorizontal ? parent.width / 2 : parent.width
            Layout.column:0
            Layout.row: isHorizontal ? 0 : 1
            Layout.preferredHeight: asd.implicitHeight + asd.anchors.margins * 2
            color: Utility.getAppHexColor("lightBackground")

            ColumnLayout {
                id: asd
                anchors.fill: parent
                anchors.margins: 10
                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignBottom
                    color: Utility.getAppHexColor("lightText")
                    id: uploadText
                    text: qsTr("Not Uploading")
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
                            uploadFw(false)
                        }
                    }

                    Button {
                        id: uploadAllButton
                        text: qsTr("Upload All")
                        Layout.fillWidth: true
                        visible: showUploadAllButton

                        onClicked: {
                            uploadFw(true)
                        }
                    }

                    Button {
                        id: cancelButton
                        text: qsTr("Cancel")
                        Layout.fillWidth: true
                        enabled: false

                        onClicked: {
                            VescIf.fwUploadCancel()
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    id: versionText
                    color: Utility.getAppHexColor("lightText")
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
        property bool fwdCan: false
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: parent.width - 20
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        Text {
            color: Utility.getAppHexColor("lightText")
            id: uploadDialogLabel
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            var okUploadFw = false
            if (swipeView.currentIndex == 0) {
                if (mCommands.getLimitedSupportsEraseBootloader() && blItems.count > 0) {
                    fwHelper.uploadFirmwareSingleShotTimer(fwItems.get(fwBox.currentIndex).value, VescIf, false, false,
                                                           fwdCan, blItems.get(blBox.currentIndex).value)
                } else {
                    okUploadFw = fwHelper.uploadFirmwareSingleShotTimer(fwItems.get(fwBox.currentIndex).value, VescIf, false, false, fwdCan, "")
                }
            } else if (swipeView.currentIndex == 1) {
                okUploadFw = fwHelper.uploadFirmwareSingleShotTimer(customFwText.text, VescIf, false, Qt.platform.os != "android", fwdCan,"")
            } else if (swipeView.currentIndex == 2) {
                fwHelper.uploadFirmwareSingleShotTimer(blItems.get(blBox.currentIndex).value, VescIf, true, false, fwdCan,"")
            }
        }
    }

    function updateHw(params) {
        var hws = fwHelper.getHardwares(params, params.hw)

        hwItems.clear()
        fwItems.clear()

        for (var name in hws) {
            if (name.indexOf("412") !== -1) {
                hwItems.insert(0, { key: name, value: hws[name] })
            } else {
                hwItems.append({ key: name, value: hws[name] })
            }
        }

        hwBox.currentIndex = 0
    }

    function updateBl(params) {
        var bls = fwHelper.getBootloaders(params, params.hw)

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

    function uploadFw(fwdCan) {
        if (!VescIf.isPortConnected()) {
            VescIf.emitMessageDialog(
                        "Connection Error",
                        "Not connected to device. Please connect first.",
                        false)
            return
        }

        var msg = "You are about to upload new firmware to the connected device"
        var msgBl = "You are about to upload a bootloader to the connected device"

        var msgEnd = "."
        if (fwdCan) {
            msgEnd = ", as well as all decices found on the CAN-bus. \n\n" +
                    "WARNING: The upload all function should ONLY be used if all " +
                    "decices on the CAN-bus have the same hardware version. If that " +
                    "is not the case, you must upload firmware to the decices individually."
        }

        msg += msgEnd
        msgBl += msgEnd

        uploadDialog.fwdCan = fwdCan

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

                if (VescIf.getFwSupportsConfiguration()) {
                    msg += "\n\n" +
                            "Uploading new firmware will clear all settings on . You can make " +
                            "a backup of the settings from the connection page and restore them after the " +
                            "update if you'd like (if you haven't done the backup already). " +
                            "Do you want to continue with the update, or cancel and do the backup first?"
                } else {
                    msg += "\n\n" +
                            "Uploading new firmware will clear all settings on your device " +
                            "and you have to do the configuration again. Do you want to " +
                            "continue?"
                }

                uploadDialogLabel.text = msg
                uploadDialog.open()
            } else {
                uploadDialog.title = "Warning"
                uploadDialogLabel.text =
                        msg + "\n\n" +
                        "Uploading firmware for the wrong hardware version " +
                        "WILL damage the hardware. Are you sure that you have " +
                        "chosen the correct hardware version?"
                uploadDialog.open()
            }
        } else if (swipeView.currentIndex == 1) {
            if (customFwText.text.length > 0) {
                uploadDialog.title = "Warning"
                uploadDialogLabel.text =
                        msg + "\n\n" +
                        "Uploading firmware for the wrong hardware version " +
                        "WILL damage the hardware. Are you sure that you have " +
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

            var msgBl2 = ""
            if (!mCommands.getLimitedSupportsEraseBootloader()) {
                msgBl2 = "If the device already has a bootloader this will destroy " +
                        "the bootloader and firmware updates cannot be done anymore. "
            }

            uploadDialogLabel.text =
                    msgBl + "\n\n" + msgBl2 +
                    "Do you want to continue?"
            uploadDialog.open()
        }
    }

    Timer {
        interval: 100
        running: true
        repeat: true

        onTriggered: {
            uploadAllButton.enabled = mCommands.getLimitedSupportsFwdAllCan() &&
                    !mCommands.getSendCan() && VescIf.getFwUploadProgress() < 0

            if (!VescIf.isPortConnected()) {
                versionText.text =
                        "FW   : \n" +
                        "HW   : \n" +
                        "UUID : "
            }
        }
    }

    Connections {
        target: VescIf

        function onFwUploadStatus(status, progress, isOngoing) {
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
        target: fwHelper

        function onFwUploadRes(res, isBootloader) {
            if (res) {
                if(isBootloader) {
                    VescIf.emitMessageDialog("Bootloader Finished",
                                             "Bootloader upload is done.",
                                             true, false)
                } else {
                    VescIf.disconnectPort()
                    VescIf.emitMessageDialog("Warning",
                                             "The firmware upload is done. The device should reboot automatically within 10 seconds. Do " +
                                             "NOT remove power before the reboot is done as that can brick the CPU and requires a programmer " +
                                             "to fix.",
                                             true, false)
                }
            }
        }
    }

    Connections {
        target: VescIf

        function onFwRxChanged(rx, limited) {
            if (!rx) {
                return;
            }

            var params = VescIf.getLastFwRxParams()

            updateHw(params)
            updateBl(params)

            var testFwStr = "";
            var fwNameStr = "";

            if (params.isTestFw > 0) {
                testFwStr = " BETA " +  params.isTestFw
            }

            if (params.fwName !== "") {
                fwNameStr = " (" + params.fwName + ")"
            }

            versionText.text =
                    "FW   : v" + params.major + "." + params.minor + fwNameStr + testFwStr + "\n" +
                    "HW   : " + params.hw + "\n" +
                    "UUID : " + Utility.uuid2Str(params.uuid, false)
        }
    }
}
