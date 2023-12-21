/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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
            // dlArchive...
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
            if (!pkgs[i].isLibrary) {
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
            id: canIdList
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true
            clip: true
            spacing: 5

            Component {
                id: canIdDelegate

                Rectangle {
                    property variant modelData: model

                    width: canIdList.width
                    height: 130
                    color: Utility.getAppHexColor("darkBackground")
                    radius: 5

                    RowLayout {
                        anchors.fill: parent
                        spacing: 10

                        Rectangle {
                            Layout.leftMargin: 5
                            Layout.fillWidth: true

                            color: Utility.getAppHexColor("lightBackground")
                            height: column.height
                            radius: height / 2

                            ColumnLayout {
                                id: column
                                anchors.centerIn: parent

                                Image {
                                    id: image
                                    fillMode: Image.PreserveAspectFit
                                    Layout.preferredWidth: 60
                                    Layout.preferredHeight: 60
                                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                    source: "qrc" + Utility.getThemePath() + "icons/Package-96.png"
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                    text: pkgName
                                    horizontalAlignment: Text.AlignHCenter
                                    color: Utility.getAppHexColor("lightText")
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.rightMargin: 10
                            spacing: -5

                            Button {
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                                Layout.preferredWidth: 120
                                Layout.preferredHeight: 55
                                text: "Install"

                                onClicked: {
                                    disableDialog()
                                    workaroundTimerFWD.start()
                                }
                                Timer {
                                    id: workaroundTimerFWD
                                    interval: 0
                                    repeat: false
                                    running: false
                                    onTriggered: {
                                        var res = mLoader.installVescPackage(pkg.compressedData)
                                        enableDialog()

                                        if (res) {
                                            VescIf.emitMessageDialog("Install Package",
                                                                     "Install Done! Please disconnect and reconnect to " +
                                                                     "apply possible VESC Tool GUI updates from this package.",
                                                                     true, false)
                                        }
                                    }
                                }
                            }

                            Button {
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                                Layout.preferredWidth: 120
                                Layout.preferredHeight: 55
                                text: "Info"

                                onClicked: {
                                    VescIf.emitMessageDialog(pkgName, Utility.md2html(pkgDescription), true, true)
                                }
                            }
                        }
                    }
                }
            }

            model: pkgModel
            delegate: canIdDelegate
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
                                workaroundTimerFile.pkgPath = fileUrl.toString()
                                close()
                                parent.forceActiveFocus()

                                disableDialog()
                                workaroundTimerFile.start()
                            }

                            onRejected: {
                                close()
                                parent.forceActiveFocus()
                            }
                        }

                        Timer {
                            property string pkgPath: ""
                            id: workaroundTimerFile
                            interval: 0
                            repeat: false
                            running: false
                            onTriggered: {
                                if (mLoader.installVescPackageFromPath(pkgPath)) {
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
            }
        }
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
}
