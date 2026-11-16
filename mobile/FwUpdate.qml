/*
    Copyright 2017 - 2023 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material
import QtQuick.Layouts 1.3
import QtQuick.Dialogs

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.fwhelper 1.0
import Vedder.vesc.utility 1.0

Item {
    property Commands mCommands: VescIf.commands()
    property ConfigParams mInfoConf: VescIf.infoConfig()
    property bool showUploadAllButton: true
    property int uploadStep: -1
    property real uploadStart: 0
    property real uploadStartProgress: 0
    property real etaUpdated: 0
    anchors.fill: parent

    FwHelper {
        id: fwHelper
    }

    component FieldLabel: Text {
        Layout.topMargin: 4
        color: Utility.getAppHexColor("normalText")
        font.pointSize: 9
        font.letterSpacing: 0.5
        font.capitalization: Font.AllUppercase
    }

    component Note: Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: Utility.getAppHexColor("normalText")
    }

    component Card: Rectangle {
        default property alias content: cardColumn.data

        Layout.fillWidth: true
        implicitHeight: cardColumn.implicitHeight + 28
        radius: 12
        color: Utility.getAppHexColor("cardBackground")
        border.color: Utility.getAppHexColor("raisedBackground")

        ColumnLayout {
            id: cardColumn
            anchors.fill: parent
            anchors.margins: 14
            spacing: 6
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                x: 14
                width: scroll.availableWidth - 28
                spacing: 10

                // Installed and available firmware
                Card {
                    Layout.topMargin: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Rectangle {
                            width: 44
                            height: 44
                            radius: 12
                            color: Utility.getAppHexColor("raisedBackground")

                            Image {
                                anchors.centerIn: parent
                                width: 26
                                height: 26
                                source: "qrc" + Utility.getThemePath() + "icons/Electronics-96.png"
                            }
                        }

                        Repeater {
                            model: [[qsTr("Installed"), fwVersion !== "" ? fwVersion : "-"], [qsTr("Available"), fwLatest]]

                            ColumnLayout {
                                visible: index === 0 || fwUpdateAvailable
                                spacing: 0

                                FieldLabel {
                                    Layout.topMargin: 0
                                    text: modelData[0]
                                }

                                Text {
                                    text: modelData[1]
                                    font.pointSize: 18
                                    font.weight: Font.Medium
                                    color: Utility.getAppHexColor(index === 0 ? "titleText" : "accentText")
                                }
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Chip {
                            text: fwUpdateAvailable ? qsTr("What's new") : qsTr("Changelog")
                            tinted: fwUpdateAvailable
                            onClicked: VescIf.emitMessageDialog("Firmware Changelog", Utility.fwChangeLog(), true)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        id: versionText
                        visible: text !== ""
                        color: Utility.getAppHexColor("normalText")
                        font.pointSize: 9
                        elide: Text.ElideRight
                    }
                }

                SegmentedBar {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    model: [qsTr("Included"), qsTr("Custom"), qsTr("Bootloader"), qsTr("Archive")]
                    currentIndex: swipeView.currentIndex
                    onActivated: function(index) { swipeView.currentIndex = index }
                }

                StackLayout {
                    id: swipeView
                    Layout.fillWidth: true
                    Layout.preferredHeight: children[currentIndex].implicitHeight

                    Card {
                        Text {
                            id: fwIncludedText
                            visible: false
                            Layout.fillWidth: true
                            textFormat: Text.RichText
                            color: Utility.getAppHexColor("lightText")
                            wrapMode: Text.WordWrap
                        }

                        ColumnLayout {
                            id: fwIncludedColumn
                            Layout.fillWidth: true
                            spacing: 6

                            FieldLabel {
                                text: "Hardware"
                            }

                            ComboBox {
                                id: hwBox
                                background: InputBackground {}
                                Layout.fillWidth: true

                                textRole: "key"
                                model: ListModel {
                                    id: hwItems
                                }

                                Component.onCompleted: {
                                    updateFwText()
                                }

                                onCurrentIndexChanged: {
                                    updateFws()
                                }

                                function updateFws() {
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

                            FieldLabel {
                                text: "Firmware"
                            }

                            ComboBox {
                                id: fwBox
                                background: InputBackground {}
                                Layout.fillWidth: true

                                textRole: "key"
                                model: ListModel {
                                    id: fwItems
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 4

                            Button {
                                id: latestUpdateButton
                                text: "Download Latest"
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Download-96.png"

                                onClicked: {
                                    latestUpdateButton.enabled = false
                                    workaroundTimerDlLatest.start()
                                }

                                Timer {
                                    id: workaroundTimerDlLatest
                                    interval: 0
                                    repeat: false
                                    running: false
                                    onTriggered: {
                                        VescIf.downloadFwLatest()
                                        VescIf.downloadConfigs()
                                        fwHelper.reloadLatest()
                                        updateFwText()
                                        latestUpdateButton.enabled = true
                                    }
                                }
                            }

                            ProgressBar {
                                id: dlProgLatest
                                Layout.fillWidth: true
                                visible: value > 0 && value < 1
                            }
                        }
                    }

                    Card {
                        FieldLabel {
                            Layout.topMargin: 0
                            text: "Custom File"
                        }

                        InputField {
                            id: customFwText
                            Layout.fillWidth: true
                            placeholderText: "Custom Firmware Path"
                        }

                        Button {
                            text: "Choose File..."
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Open Folder-96.png"

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

                        FileDialog {
                            id: fileDialog
                            title: "Choose a firmware file"
                            fileMode: FileDialog.OpenFile
                            nameFilters: ["*"]
                            onAccepted: {
                                customFwText.text = selectedFile
                                close()
                                parent.forceActiveFocus()
                            }
                            onRejected: {
                                close()
                                parent.forceActiveFocus()
                            }
                        }
                    }

                    Card {
                        Note {
                            visible: blItems.count === 0
                            text: qsTr("There is no bootloader for this hardware.")
                        }

                        FieldLabel {
                            Layout.topMargin: 0
                            visible: blItems.count > 0
                            text: "Hardware"
                        }

                        ComboBox {
                            id: blBox
                            background: InputBackground {}
                            Layout.fillWidth: true
                            visible: blItems.count > 0

                            textRole: "key"
                            model: ListModel {
                                id: blItems
                            }

                            Component.onCompleted: {
                                updateBl(VescIf.getLastFwRxParams())
                            }
                        }
                    }

                    Card {
                        Note {
                            visible: archVerItems.count === 0
                            text: qsTr("The firmware archive is not downloaded. It contains older firmware versions for most hardware.")
                        }

                        FieldLabel {
                            Layout.topMargin: 0
                            visible: archVerItems.count > 0
                            text: "Firmware Version"
                        }

                        ComboBox {
                            id: archVerBox
                            background: InputBackground {}
                            Layout.fillWidth: true
                            visible: archVerItems.count > 0
                            popup.bottomMargin: 100

                            textRole: "key"
                            model: ListModel {
                                id: archVerItems
                            }

                            Component.onCompleted: {
                                updateArch()
                            }

                            onCurrentIndexChanged: {
                                if (archVerItems.rowCount() === 0) {
                                    return
                                }

                                archFwItems.clear()
                                var fws = fwHelper.getArchiveFirmwares(
                                            archVerItems.get(archVerBox.currentIndex).value,
                                            VescIf.getLastFwRxParams())

                                for (var name in fws) {
                                    archFwItems.append({ key: name, value: fws[name] })
                                }

                                archFwBox.currentIndex = 0
                            }
                        }

                        FieldLabel {
                            visible: archVerItems.count > 0
                            text: "File"
                        }

                        ComboBox {
                            id: archFwBox
                            background: InputBackground {}
                            Layout.fillWidth: true
                            visible: archVerItems.count > 0
                            popup.bottomMargin: 100

                            textRole: "key"
                            model: ListModel {
                                id: archFwItems
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 4

                            Button {
                                id: archUpdateButton
                                text: "Update Archive"
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Download-96.png"

                                onClicked: {
                                    dlArchiveDialog.open()
                                }
                            }

                            ProgressBar {
                                id: dlProg
                                Layout.fillWidth: true
                                visible: value > 0 && value < 1
                            }
                        }

                        Text {
                            id: dlText
                            Layout.fillWidth: true
                            visible: text !== ""
                            wrapMode: Text.WordWrap
                            font.pointSize: 9
                            color: Utility.getAppHexColor("normalText")
                        }
                    }
                }

                Card {
                    visible: showUploadAllButton

                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: qsTr("Include CAN devices")
                                color: Utility.getAppHexColor("titleText")
                            }

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("All devices must have the same hardware version")
                                wrapMode: Text.WordWrap
                                font.pointSize: 9
                                color: Utility.getAppHexColor("normalText")
                            }
                        }

                        Switch {
                            id: uploadAllButton
                        }
                    }
                }

                Card {
                    visible: uploadText.text !== qsTr("Not Uploading")

                    RowLayout {
                        spacing: 6

                        Repeater {
                            model: [qsTr("Erase"), qsTr("Upload"), qsTr("Reboot")]

                            Chip {
                                enabled: false
                                topInset: 0
                                checked: index === uploadStep
                                text: (index < uploadStep ? "\u2713 " : "") + modelData
                            }
                        }
                    }

                    ProgressBar {
                        id: uploadProgress
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            Layout.fillWidth: true
                            color: Utility.getAppHexColor("lightText")
                            id: uploadText
                            elide: Text.ElideRight
                            text: qsTr("Not Uploading")
                        }

                        Text {
                            id: etaText
                            color: Utility.getAppHexColor("normalText")
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 12
                    implicitHeight: warnRow.implicitHeight + 20
                    visible: cancelButton.enabled
                    radius: 12
                    color: Qt.alpha(Utility.getAppHexColor("warning"), 0.15)
                    border.color: Qt.alpha(Utility.getAppHexColor("warningText"), 0.33)

                    RowLayout {
                        id: warnRow
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Image {
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            source: "qrc" + Utility.getThemePath() + "icons/Brake Warning-96.png"
                        }

                        Text {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            color: Utility.getAppHexColor("warningText")
                            text: qsTr("Keep the app open and the board powered.")
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            Layout.topMargin: 8

            Button {
                id: cancelButton
                text: qsTr("Cancel upload")
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                visible: enabled
                flat: true
                enabled: false

                onClicked: {
                    VescIf.fwUploadCancel()
                }
            }

            Button {
                id: uploadButton
                text: uploadAllButton.checked && uploadAllButton.enabled && VescIf.getCanDevsLast().length > 0 ?
                          qsTr("Upload to %1 devices").arg(VescIf.getCanDevsLast().length + 1) : qsTr("Upload")
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                highlighted: true

                onClicked: {
                    uploadFw(uploadAllButton.checked && uploadAllButton.enabled)
                }
            }
        }
    }

    SheetDialog {
        id: uploadDialog
        acceptText: qsTr("Upload")
        property bool fwdCan: false

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
            } else if (swipeView.currentIndex == 3) {
                okUploadFw = fwHelper.uploadFirmwareSingleShotTimer(archFwItems.get(archFwBox.currentIndex).value, VescIf, false, false, fwdCan, "")
            }
        }
    }

    SheetDialog {
        id: dlArchiveDialog
        acceptText: qsTr("Download")
        property bool fwdCan: false

        Text {
            color: Utility.getAppHexColor("lightText")
            text: "This is going do download a few 100 MB of old firmwares. Continue?"
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            archUpdateButton.enabled = false
            workaroundTimerDl.start()
        }
    }

    Timer {
        id: workaroundTimerDl
        interval: 0
        repeat: false
        running: false
        onTriggered: {
            // dlArchive...
            VescIf.downloadFwArchive()
            updateArch(VescIf.getLastFwRxParams())
            archUpdateButton.enabled = true
        }
    }

    function updateHw(params) {
        var hws = fwHelper.getHardwares(params, params.hw)

        hwItems.clear()
        fwItems.clear()

        for (var name in hws) {
            hwItems.append({ key: name, value: hws[name] })
        }

        hwBox.currentIndex = 0
        hwBox.updateFws()
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

    function updateArch() {
        var fws = fwHelper.getArchiveDirs()

        archVerItems.clear()
        archFwItems.clear()

        for (var name in fws) {
            archVerItems.append({ key: name, value: fws[name] })
        }

        archVerBox.currentIndex = 0
    }

    function uploadFw(fwdCan) {
        if (!VescIf.isPortConnected()) {
            VescIf.emitMessageDialog(
                        "Connection Error",
                        "Not connected to device. Please connect first.",
                        false)
            return
        }

        var msg = "CAREFUL: If you continue you will lose your package and all settings, are you sure "
                  "you know what you're doing?\n\n" +
                  "This will upload new firmware to the connected device"
        var msgBl = "You are about to upload a bootloader to the connected device"

        var msgEnd = "."
        if (fwdCan) {
            msgEnd = ", as well as all devices found on the CAN-bus. \n\n" +
                    "WARNING: The upload all function should ONLY be used if all " +
                    "devices on the CAN-bus have the same hardware version. If that " +
                    "is not the case, you must upload firmware to the devices individually."
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
                            "Uploading new firmware will clear all settings in the VESC firmware. You can make " +
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
        } else if (swipeView.currentIndex === 3) {
            if (archFwItems.rowCount() === 0) {
                VescIf.emitMessageDialog(
                            "Upload Error",
                            "This version of VESC Tool does not include the selected firmware " +
                            "for your hardware version. You can try to update the archive and see " +
                            "if it has been added since your last update.",
                            false)
                return;
            }

            uploadDialog.title = "Warning"

            msg += "\n\n" +
                    "Uploading new firmware will clear all settings on your device " +
                    "and you have to do the configuration again. Do you want to " +
                    "continue?"

            uploadDialogLabel.text = msg
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
                versionText.text = ""
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

            if (status.startsWith("Uploading")) {
                if (uploadStep !== 1) {
                    uploadStart = Date.now()
                    uploadStartProgress = progress
                }
                uploadStep = 1
            } else if (status === "Upload done") {
                uploadStep = 2
            } else if (isOngoing) {
                uploadStep = 0
            } else if (status !== "Erase done") {
                uploadStep = -1
            }

            // Coarse and at most once per second, so the estimate reads calmly
            if (uploadStep !== 1) {
                etaText.text = ""
            } else if (progress > uploadStartProgress + 0.02 && Date.now() - etaUpdated > 1000) {
                var left = (Date.now() - uploadStart) * (1 - progress) / (progress - uploadStartProgress) / 1000
                etaText.text = left > 60 ? qsTr("\u2248 %1 min left").arg(Math.ceil(left / 60)) :
                                           qsTr("\u2248 %1 s left").arg(Math.ceil(left / 5) * 5)
                etaUpdated = Date.now()
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
            if (rx) {
                updateFwText()
            }
        }

        function onFwArchiveDlProgress(msg, prog) {
            dlProg.value = prog
            dlText.text = msg
            dlProgLatest.value = prog
        }
    }

    function updateFwText() {
        var params = VescIf.getLastFwRxParams()

        updateHw(params)

        var dlText = fwHelper.fwDlText(VescIf)
        fwIncludedText.text = dlText

        if (dlText.length > 0) {
            fwIncludedColumn.visible = false
            fwIncludedText.visible = true
        } else {
            fwIncludedText.visible = false
            fwIncludedColumn.visible = true
        }

        updateBl(params)
        updateArch()

        var testFwStr = "";
        var fwNameStr = "";

        if (params.isTestFw > 0) {
            testFwStr = " BETA " +  params.isTestFw
        }

        if (params.fwName !== "") {
            fwNameStr = " (" + params.fwName + ")"
        }

        versionText.text =
                "FW v" + params.major + "." + (1e5 + params.minor + '').slice(-2) + fwNameStr + testFwStr +
                " · HW " + params.hw + " · UUID " + Utility.uuid2Str(params.uuid, false)
    }
}
