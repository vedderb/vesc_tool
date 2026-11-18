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
import QtQuick.Controls.Material
import QtQuick.Layouts 1.3
import QtQuick.Dialogs

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0
import Vedder.vesc.codeloader 1.0

Item {
    id: appPageItem
    property var dialogParent: Overlay.overlay
    property ConfigParams mAppConf: VescIf.appConfig()
    property Commands mCommands: VescIf.commands()
    property var mPkgs: []
    // Wide enough to show the selected package beside the list
    readonly property bool split: mainIsTablet && width >= 700
    property int selectedInd: -1

    onSplitChanged: installFromPathDialog.close()

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
        mPkgs = mLoader.reloadPackageArchive()

        var filter = filterInput.text

        for (var i = 0;i < mPkgs.length;i++) {
            if (filter.length === 0  || mPkgs[i].name.toLowerCase().indexOf(filter.toLowerCase()) !== -1) {
                if (!mPkgs[i].isLibrary && mLoader.shouldShowPackage(mPkgs[i])) {
                    pkgModel.append({"pkgName": mPkgs[i].name,
                                        "pkgDescription": descriptionLine(mPkgs[i]),
                                        "ind": i})
                }
            }
        }

        // Refresh the detail pane for the new archive or device
        if (split && selectedInd >= 0) {
            selectedInd = mPkgs.findIndex(p => p.name === workaroundTimerInstall.pkg.name)
            if (selectedInd >= 0) {
                openPkgDialog(mPkgs[selectedInd], selectedInd)
            } else {
                workaroundTimerInstall.pkg = []
            }
        }

        enableDialog()
    }

    // First text line of the description, shown under the package name
    function descriptionLine(pkg) {
        var desc = pkg.description_md !== "" ? pkg.description_md : pkg.description
        if (desc.toUpperCase().includes("<!DOCTYPE HTML")) {
            return ""
        }

        // Skips headings, HTML, images, links and code on their own line
        var line = desc.split("\n").map(l => l.trim()).find(l => /^[^#<!\[`]/.test(l))
        return line ? line.replace(/\[([^\]]*)\]\([^)]*\)/g, "$1").replace(/[*`]/g, "") : ""
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.leftMargin: notchLeft
        anchors.rightMargin: split ? parent.width - notchLeft - 320 : notchRight
        spacing: 8

        ListModel {
            id: pkgModel
        }

        InputField {
            id: filterInput
            Layout.fillWidth: true
            placeholderText: qsTr("Search packages")
            inputMethodHints: Qt.ImhNoPredictiveText
            onTextChanged: reloadArchive()
        }

        ListView {
            id: pkgList
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true
            clip: true

            Text {
                anchors.centerIn: parent
                width: parent.width - 40
                visible: pkgModel.count === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("No packages loaded. Update the archive to download the package list.")
                color: Utility.getAppHexColor("disabledText")
            }

            Component {
                id: pkgDelegate

                ItemDelegate {
                    width: pkgList.width
                    highlighted: split && ind === selectedInd
                    onClicked: {
                        openPkgDialog(mPkgs[ind], ind)
                    }

                    background: Rectangle {
                        radius: 10
                        color: highlighted ? Qt.alpha(Material.accent, 0.12) :
                               down ? Qt.alpha(Material.foreground, 0.08) : "transparent"
                    }

                    contentItem: RowLayout {
                        spacing: 11

                        Rectangle {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            radius: 12
                            color: Utility.getAppHexColor("raisedBackground")

                            Image {
                                anchors.centerIn: parent
                                width: 24
                                height: 24
                                source: "qrc" + Utility.getThemePath() + "icons/Package-96.png"
                                smooth: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                Layout.fillWidth: true
                                text: pkgName
                                elide: Text.ElideRight
                                color: Utility.getAppHexColor(highlighted ? "accentText" : "lightText")
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: text !== ""
                                text: pkgDescription
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                                font.pointSize: 9
                                color: Utility.getAppHexColor("normalText")
                            }
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: Utility.getAppHexColor("lightestBackground")
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
                highlighted: true
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
                flat: true
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
                Layout.preferredWidth: 56
                text: "..."
                onClicked: menu.open()

                BottomMenu {
                    id: menu
                    parent: split ? column : appPageItem

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

                        FileDialog {
                            id: fileDialogLoad
                            title: "Please choose a file"
                            fileMode: FileDialog.OpenFile
                            nameFilters: ["*"]

                            onAccepted: {
                                var pkg = mLoader.unpackVescPackageFromPath(selectedFile.toString())

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

    Item {
        id: detailPane
        anchors.left: column.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: notchRight
        visible: split

        Text {
            anchors.centerIn: parent
            visible: !installContent.visible
            text: qsTr("Select a package")
            color: Utility.getAppHexColor("disabledText")
        }
    }

    // HERE install
    function openPkgDialog(pkg, ind) {
        var line1 = pkg.description.slice(0, pkg.description.indexOf("\n"))
        if (line1.toUpperCase().includes("<!DOCTYPE HTML PUBLIC")) {
            installFromPathText.text = pkg.description
        } else {
            installFromPathText.text = Utility.md2html(pkg.description)
        }

        installPkgCompatibleText.visible = !mLoader.shouldShowPackage(pkg)
        workaroundTimerInstall.pkg = pkg

        if (split && ind !== selectedInd) {
            vescDialogScroll.contentItem.contentY = 0
        }

        selectedInd = ind ?? -1

        if (!split) {
            installFromPathDialog.open()
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

    BusyDialog {
        id: dlDialog
        title: "Processing..."
    }

    SheetDialog {
        title: "Install Package"

        id: installFromPathDialog
        iconSource: "qrc" + Utility.getThemePath() + "icons/Package-96.png"
        rejectText: ""

        ColumnLayout {
            id: installContent
            parent: split ? detailPane : installFromPathDialog.contentItem
            anchors.fill: parent
            visible: !split || workaroundTimerInstall.pkg.compressedData !== undefined

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
                    width: vescDialogScroll.availableWidth
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    textFormat: Text.RichText
                    onLinkActivated: {
                        Qt.openUrlExternally(link)
                    }
                }
            }

            WarningBanner {
                id: installPkgCompatibleText
                visible: false
                Layout.fillWidth: true
                text: qsTr("This package is NOT compatible with the connected device. " +
                           "Install it only if you know what you are doing!")
            }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 500
                    highlighted: true
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
                    visible: !split
                    flat: true
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
