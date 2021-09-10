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

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0 as QSettings

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.utility 1.0

Item {
    implicitHeight: grid.implicitHeight

    property var dialogParent: ApplicationWindow.overlay

    GridLayout {
        id: grid

        anchors.fill: parent
        anchors.topMargin: -5
        anchors.bottomMargin: -5

        clip: false
        visible: true
        rowSpacing: -10
        columnSpacing: 5
        rows: 3
        columns: 2

        Button {
            text: "Help"
            Layout.fillWidth: true

            onClicked: {
                VescIf.emitMessageDialog(
                            mInfoConf.getLongName("help_rt_logging"),
                            mInfoConf.getDescription("help_rt_logging"),
                            true, true)
            }
        }

        Button {
            text: "Choose Log Directory..."
            Layout.fillWidth: true

            onClicked: {
                if (Utility.requestFilePermission()) {
                    logFilePicker.enabled = true
                    logFilePicker.visible = true
                } else {
                    VescIf.emitMessageDialog(
                                "File Permissions",
                                "Unable to request file system permission.",
                                false, false)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.columnSpan: 2
            Layout.topMargin: 6
            Layout.bottomMargin: 6
            height: rtLogFileText.implicitHeight + 14
            border.width: 2
            border.color: Utility.getAppHexColor("disabledText")
            color: Utility.getAppHexColor("normalBackground")
            radius: 3

            TextInput {
                color: Utility.getAppHexColor("lightText")
                id: rtLogFileText
                anchors.fill: parent
                anchors.margins: 7
                font.pointSize: 12
                text: "./log"
                clip: true

                QSettings.Settings {
                    property alias path_rt_log: rtLogFileText.text
                }
            }
        }

        CheckBox {
            id: rtLogEnBox
            text: "Enable RT Data Logging"
            Layout.fillWidth: true
            Layout.columnSpan: 2

            onClicked: {
                if (checked) {
                    if (VescIf.openRtLogFile(rtLogFileText.text)) {
                        Utility.startGnssForegroundService()
                        VescIf.setWakeLock(true)
                    }
                } else {
                    VescIf.closeRtLogFile()
                    Utility.stopGnssForegroundService()

                    if (!VescIf.useWakeLock()) {
                        VescIf.setWakeLock(false)
                    }
                }
            }

            Timer {
                repeat: true
                running: true
                interval: 500

                onTriggered: {
                    if (rtLogEnBox.checked && !VescIf.isRtLogOpen()) {
                        Utility.stopGnssForegroundService()

                        if (!VescIf.useWakeLock()) {
                            VescIf.setWakeLock(false)
                        }
                    }

                    rtLogEnBox.checked = VescIf.isRtLogOpen()
                }
            }
        }
    }

    DirectoryPicker {
        id: logFilePicker
        anchors.fill: parent
        showDotAndDotDot: true
        visible: false
        enabled: false
        parent: dialogParent

        onDirSelected: {
            rtLogFileText.text = fileName
        }
    }
}
