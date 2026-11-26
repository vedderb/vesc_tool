/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Layouts 1.3
import QtCore
import Qt.labs.platform 1.1

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.utility 1.0

Item {
    implicitHeight: column.implicitHeight
    property var dialogParent: Overlay.overlay
    // Folders picked outside of the app are not accessible after a restart on iOS
    property bool fixedDir: Qt.platform.os === "ios"
    property bool logging: false
    property string logName: ""

    // The ride display on the lock screen toggles logging while the app is not visible
    function toggleLogging(showDialogs) {
        if (VescIf.isRtLogOpen()) {
            VescIf.closeRtLogFile()
            VescIf.emitStatusMessage("Log Saved", true)
            Utility.stopGnssForegroundService()
            logging = false

            if (!VescIf.useWakeLock()) {
                VescIf.setWakeLock(false)
            }
        } else if (!VescIf.isPortConnected()) {
            if (showDialogs) {
                VescIf.emitMessageDialog("Log Data",
                                         "You must be connected to the VESC in order to log data.",
                                         false, false)
            }
        } else if (VescIf.openRtLogFile(rtLogFileText.text)) {
            VescIf.emitStatusMessage("Logging Started", true)
            Utility.requestNotificationPermission()
            Utility.startGnssForegroundService()
            VescIf.setWakeLock(true)

            if (showDialogs) {
                VescIf.emitMessageDialog("Logging Started",
                                         "Motor data together with your location is now being logged to:\n\n" +
                                         displayPath(VescIf.rtLogFilePath()) + "\n\n" +
                                         "You can open it in LogView, or transfer the file to a computer for " +
                                         "analysis in the desktop version of VESC Tool (under Data Analysis->Log Analysis).",
                                         true, false)
            }
        } else {
            VescIf.emitStatusMessage("Logging Failed", false)
        }
    }

    Connections {
        target: rideSession

        function onLogToggleRequested() {
            toggleLogging(false)
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: logging ? qsTr("Stop Logging") : qsTr("Start Logging")
                Layout.fillWidth: true
                highlighted: true
                Material.accent: logging ? Utility.getAppHexColor("danger") : undefined
                focusPolicy: Qt.NoFocus

                onClicked: toggleLogging(true)

                Timer {
                    repeat: true
                    running: true
                    interval: 500

                    onTriggered: {
                        if (logging && !VescIf.isRtLogOpen()) {
                            Utility.stopGnssForegroundService()

                            if (!VescIf.useWakeLock()) {
                                VescIf.setWakeLock(false)
                            }
                        }

                        logging = VescIf.isRtLogOpen()
                        var path = VescIf.rtLogFilePath()
                        logName = (path.startsWith("content:") ? decodeURIComponent(path) : path).split("/").pop()
                    }
                }
            }

            Button {
                text: "Help"

                onClicked: {
                    VescIf.emitMessageDialog(
                                mInfoConf.getLongName("help_rt_logging"),
                                mInfoConf.getDescription("help_rt_logging"),
                                true, true)
                }
            }
        }

        Label {
            visible: fixedDir || logging
            Layout.fillWidth: true
            Layout.margins: 4
            wrapMode: Text.Wrap
            text: logging ? qsTr("Logging to %1. The data is saved while logging.").arg(logName) :
                            qsTr("Logs are saved to %1").arg(displayPath(rtLogFileText.text))
            color: Utility.getAppHexColor(logging ? "lightText" : "disabledText")
        }

        InputField {
            id: rtLogFileText
            visible: !fixedDir
            Layout.fillWidth: true
            placeholderText: "File Path"
            text: StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/logs"
            clip: true

            Component.onCompleted: {
                // The absolute path of the app folder changes with app updates on iOS
                if (text.length === 0 || fixedDir) {
                    text = StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/logs"
                }
            }

            Settings {
                property alias path_rt_log: rtLogFileText.text
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: qsTr("Open in LogView")
                Layout.fillWidth: true
                focusPolicy: Qt.NoFocus

                onClicked: {
                    mainSwipeView.setCurrentIndex(logViewPage.SwipeView.index)
                    logViewLoader.item.openRtLogBrowser()
                }
            }

            Button {
                text: qsTr("Show in Files")
                visible: fixedDir
                Layout.fillWidth: true
                focusPolicy: Qt.NoFocus

                onClicked: {
                    // The log folder is created when logging for the first time
                    var dir = rtLogFileText.text
                    if (Utility.listFilesInDir(dir).length === 0) {
                        dir = dir.slice(0, dir.lastIndexOf("/"))
                    }
                    Qt.openUrlExternally(dir.replace("file://", "shareddocuments://"))
                }
            }

            Button {
                text: "Browse"
                visible: !fixedDir
                Layout.fillWidth: true
                focusPolicy: Qt.NoFocus

                onClicked: {
                    if (Utility.requestFilePermission()) {
                        folderDialog.close()
                        folderDialog.open()
                    } else {
                        VescIf.emitMessageDialog(
                                    "File Permissions",
                                    "Unable to request file system permission.",
                                    false, false)
                    }
                }

                FolderDialog {
                    id: folderDialog
                    title: "Please choose a folder"
                    onAccepted: {
                        rtLogFileText.text = folderDialog.folder.toString()
                        close()
                        parent.forceActiveFocus()
                    }
                    onRejected: {
                        close()
                        parent.forceActiveFocus()
                    }
                }
            }

            Button {
                text: "Reset"
                visible: !fixedDir
                Layout.fillWidth: true
                focusPolicy: Qt.NoFocus

                onClicked: {
                    rtLogFileText.text = StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/logs"
                }
            }
        }
    }
}
