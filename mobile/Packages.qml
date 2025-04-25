/*
    Copyright 2022 - 2025 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.3 as Dl

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0
import Vedder.vesc.codeloader 1.0

Item {
    id: appPageItem
    property var dialogParent: ApplicationWindow.overlay
    property ConfigParams mAppConf: VescIf.appConfig()
    property Commands mCommands: VescIf.commands()

    function updateArchive() {
        disableDialog()
        workaroundTimerDl.start()
    }

    Timer {
        id: workaroundTimerDl
        interval: 0
        repeat: false
        running: false
        onTriggered: {
            reloadArchive()
        }
    }

    CodeLoader {
        id: mLoader
        Component.onCompleted: {
            mLoader.setVesc(VescIf)
        }
    }

    Component.onCompleted: {
        reloadArchive()
    }

    function reloadArchive() {
        pkgModel.clear()
        var pkgs = mLoader.reloadPackageArchive()

        for (var i = 0;i < pkgs.length;i++) {
            if (!pkgs[i].isLibrary && mLoader.shouldShowPackage(pkgs[i])) {
                pkgModel.append({"pkgName": pkgs[i].name,
                                    "pkgDescription": pkgs[i].description,
                                    "pkg": pkgs[i]})
            }
        }

        enableDialog()
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.leftMargin: notchLeft
        anchors.rightMargin: notchRight

        ListModel {
            id: pkgModel
        }

        ListView {
            id: pkgList
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true
            clip: true
            spacing: 5

            Component {
                id: pkgDelegate

                ImageButton {
                    id: connectButton
                    width: pkgList.width
                    height: 80

                    buttonText: pkgName
                    imageSrc: "qrc" + Utility.getThemePath() + "icons/Package-96.png"
                    onClicked: {
                        openPkgDialog(pkg)
                    }
                }
            }

            model: pkgModel
            delegate: pkgDelegate
        }

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "Update Archive"
                Layout.fillWidth: true
                onClicked: {
                    disableDialog()
                    workaroundTimerArchive.start()
                }

                Timer {
                    id: workaroundTimerArchive
                    interval: 0
                    repeat: false
                    running: false
                    onTriggered: {
                        mLoader.downloadPackageArchive()
                        reloadArchive()
                        enableDialog()
                    }
                }
            }

            Button {
                text: "Uninstall Current"
                Layout.fillWidth: true
                onClicked: {
                    disableDialog()
                    workaroundTimerUninstall.start()
                }

                Timer {
                    id: workaroundTimerUninstall
                    interval: 0
                    repeat: false
                    running: false
                    onTriggered: {
                        var resLisp = mLoader.lispErase(16)
                        var resQml = mLoader.qmlErase(16)
                        Utility.sleepWithEventLoop(500)

                        if (resLisp || resQml) {
                            VescIf.reloadFirmware()
                        }

                        enableDialog()

                        if (resLisp && resQml) {
                            VescIf.emitMessageDialog("Uninstall Package",
                                                     "Uninstallation Done!",
                                                     true, false)
                        }
                    }
                }
            }

            Button {
                Layout.preferredWidth: 50
                Layout.fillWidth: true
                text: "..."
                onClicked: menu.open()

                Menu {
                    id: menu
                    bottomPadding: notchBot
                    leftPadding: notchLeft
                    rightPadding: notchRight
                    parent: appPageItem
                    y: parent.height - implicitHeight
                    width: parent.width

                    MenuItem {
                        text: "Install from file..."
                        onTriggered: {
                            if (Utility.requestFilePermission()) {
                                fileDialogLoad.close()
                                fileDialogLoad.open()
                            } else {
                                VescIf.emitMessageDialog(
                                            "File Permissions",
                                            "Unable to request file system permission.",
                                            false, false)
                            }
                        }

                        Dl.FileDialog {
                            id: fileDialogLoad
                            title: "Please choose a file"
                            nameFilters: ["*"]
                            selectExisting: true
                            selectMultiple: false
                            onAccepted: {
                                var pkg = mLoader.unpackVescPackageFromPath(fileUrl.toString())

                                if (!pkg.loadOk) {
                                    return
                                }

                                openPkgDialog(pkg)
                            }

                            onRejected: {
                                close()
                                parent.forceActiveFocus()
                            }
                        }
                    }
                }
            }
        }
    }

    function openPkgDialog(pkg) {
        var line1 = pkg.description.slice(0, pkg.description.indexOf("\n"))
        if (line1.toUpperCase().includes("<!DOCTYPE HTML PUBLIC")) {
            installFromPathText.text = pkg.description
        } else {
            installFromPathText.text = Utility.md2html(pkg.description)
        }

        installPkgCompatibleText.visible = !mLoader.shouldShowPackage(pkg)
        workaroundTimerInstall.pkg = pkg
        installFromPathDialog.open()
    }

    function disableDialog() {
        dlDialog.open()
        column.enabled = false
    }

    function enableDialog() {
        dlDialog.close()
        column.enabled = true
    }

    Dialog {
        id: dlDialog
        title: "Processing..."
        closePolicy: Popup.NoAutoClose
        modal: true
        focus: true
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 20
        x: 10
        y: parent.height / 2 - height / 2
        parent: dialogParent

        ProgressBar {
            anchors.fill: parent
            indeterminate: visible
        }
    }

    Dialog {
        title: "Install Package"

        id: installFromPathDialog
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 20 - notchLeft - notchRight
        height: Math.min(implicitHeight, parent.height - 40 - notchBot - notchTop)
        x: (parent.width - width) / 2
        y: (parent.height - height + notchTop) / 2
        parent: dialogParent

        ColumnLayout {
            anchors.fill: parent

            ScrollView {
                id: vescDialogScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth

                Text {
                    id: installFromPathText
                    color: {color = Utility.getAppHexColor("lightText")}
                    linkColor: {linkColor = Utility.getAppHexColor("lightAccent")}
                    verticalAlignment: Text.AlignVCenter
                    anchors.fill: parent
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    textFormat: Text.RichText
                    onLinkActivated: {
                        Qt.openUrlExternally(link)
                    }
                }
            }

            Text {
                id: installPkgCompatibleText
                visible: false
                color: {color = Utility.getAppHexColor("red")}
                verticalAlignment: Text.AlignVCenter
                Layout.fillWidth: true
                font.bold: true
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                text: "Warning: This package is NOT compatble with the connected device. " +
                      "Install it only if you know what you are doing!"
            }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 500
                    text: "Install Package"

                    onClicked: {
                        disableDialog()
                        installFromPathDialog.close()
                        workaroundTimerInstall.start()
                    }
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 500
                    text: "Cancel"

                    onClicked: {
                        installFromPathDialog.close()
                    }
                }
            }

            Timer {
                property var pkg: []
                id: workaroundTimerInstall
                interval: 0
                repeat: false
                running: false
                onTriggered: {
                    if (mLoader.installVescPackage(pkg.compressedData)) {
                        enableDialog()
                        VescIf.emitMessageDialog("Install Package",
                                                 "Installation Done!",
                                                 true, false)
                    } else {
                        enableDialog()
                        VescIf.emitMessageDialog("Install Package",
                                                 "Installation failed",
                                                 false, false)
                    }
                }
            }
        }
    }

    Connections {
        target: VescIf

        function onPortConnectedChanged() {
            if (!VescIf.isPortConnected()) {
                reloadArchive()
            }
        }

        function onCustomConfigLoadDone() {
            reloadArchive()
        }
    }
}
