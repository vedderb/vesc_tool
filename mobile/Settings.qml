/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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
    property bool mLastDarkMode: false
    property var dialogParent: Overlay.overlay

    function openDialog() {
        mLastDarkMode = Utility.isDarkMode()
        dialog.open()
    }

    component SettingSwitch: SwitchDelegate {
        id: control
        property string hint: ""

        Layout.fillWidth: true

        contentItem: Item {
            implicitHeight: labels.implicitHeight

            Column {
                id: labels
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - control.indicator.width - control.spacing
                spacing: 2

                Text {
                    width: parent.width
                    text: control.text
                    color: Utility.getAppHexColor("titleText")
                    wrapMode: Text.WordWrap
                }

                Text {
                    width: parent.width
                    visible: control.hint !== ""
                    text: control.hint
                    font.pointSize: 9
                    color: Utility.getAppHexColor("normalText")
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    component SettingGroup: Rectangle {
        default property alias content: groupColumn.data

        Layout.fillWidth: true
        implicitHeight: groupColumn.implicitHeight
        radius: 12
        color: Utility.getAppHexColor("cardBackground")
        border.color: Utility.getAppHexColor("raisedBackground")

        ColumnLayout {
            id: groupColumn
            width: parent.width
            spacing: 0
        }
    }

    Dialog {
        id: dialog
        modal: true
        focus: true
        title: "VESC Tool Settings"

        Overlay.modal: Rectangle {
            color: scrimColor
        }

        width: parent.width
        height: parent.height
        closePolicy: Popup.CloseOnEscape
        parent: dialogParent
        leftPadding: 14 + notchLeft
        rightPadding: 14 + notchRight
        topPadding: 0

        background: Rectangle {
            color: Utility.getAppHexColor("darkBackground")
        }

        header: PageHeader {
            title: dialog.title
            onCloseClicked: dialog.close()
        }

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 8

                SectionHeader {
                    text: "Units"
                }

                SettingGroup {
                    SettingSwitch {
                        id: imperialBox
                        text: "Use Imperial Units"
                        hint: qsTr("Speed and distance in mph and miles")
                        checked: VescIf.useImperialUnits()
                    }

                    SettingSwitch {
                        id: negativeSpeedBox
                        text: "Use Negative Speed"
                        hint: qsTr("Show reverse as negative speed")
                        checked: VescIf.speedGaugeUseNegativeValues()
                    }
                }

                SectionHeader {
                    text: "Display"
                }

                SettingGroup {
                    SettingSwitch {
                        id: darkModeBox
                        text: "Use Dark Mode"
                        hint: qsTr("Applies after restart")
                        checked: Utility.isDarkMode()
                        onCheckedChanged: {
                            Utility.setDarkMode(checked)
                        }
                    }

                    SettingSwitch {
                        id: screenOnBox
                        text: "Keep Screen On"
                        checked: VescIf.keepScreenOn()
                        visible: Qt.platform.os !== "ios"
                        enabled: Qt.platform.os !== "ios"
                    }

                    SettingSwitch {
                        id: screenRotBox
                        text: "Allow Screen Rotation"
                        checked: VescIf.getAllowScreenRotation()
                        visible: Qt.platform.os !== "ios"
                        enabled: Qt.platform.os !== "ios"
                    }
                }

                SectionHeader {
                    text: "Connection"
                }

                SettingGroup {
                    SettingSwitch {
                        id: reconnectLastCanBox
                        text: "Reconnect CAN on connect"
                        checked: VescIf.reconnectLastCan()
                    }

                    SettingSwitch {
                        id: scanCanConnectBox
                        text: "Scan CAN on connect"
                        checked: VescIf.scanCanOnConnect()
                    }

                    SettingSwitch {
                        visible: rideSession !== null && rideSession.supported
                        text: qsTr("Ride display on connect")
                        hint: Qt.platform.os === "android" ? qsTr("Notification with speed, battery and trip") :
                                                             qsTr("Live Activity with speed, battery and trip")
                        checked: rideSession !== null && rideSession.autoStart
                        onToggled: rideSession.autoStart = checked
                    }
                }

                SectionHeader {
                    text: "Custom UI & Notifications"
                }

                SettingGroup {
                    SettingSwitch {
                        id: qmlUiBox
                        text: "Load QML UI on Connect"
                        checked: VescIf.getLoadQmlUiOnConnect()
                    }

                    SettingSwitch {
                        id: qmlUiAskBox
                        text: "Ask before loading QML UI"
                        hint: qsTr("Code from the device is not verified")
                        checked: VescIf.askQmlLoad()
                    }

                    SettingSwitch {
                        id: fwUpdateAvailableBox
                        text: "Show firmware update message"
                        checked: VescIf.showFwUpdateAvailable()
                    }
                }
            }
        }

        onOpened: {
            imperialBox.checked = VescIf.useImperialUnits()
            negativeSpeedBox.checked = VescIf.speedGaugeUseNegativeValues()
            screenOnBox.checked = VescIf.keepScreenOn()
            screenRotBox.checked = VescIf.getAllowScreenRotation()
            qmlUiBox.checked = VescIf.getLoadQmlUiOnConnect()
            qmlUiAskBox.checked = VescIf.askQmlLoad()
            reconnectLastCanBox.checked = VescIf.reconnectLastCan()
            scanCanConnectBox.checked = VescIf.scanCanOnConnect()
            fwUpdateAvailableBox.checked = VescIf.showFwUpdateAvailable()
        }

        onClosed: {
            VescIf.setUseImperialUnits(imperialBox.checked)
            VescIf.setSpeedGaugeUseNegativeValues(negativeSpeedBox.checked)
            VescIf.setKeepScreenOn(screenOnBox.checked)
            VescIf.setAllowScreenRotation(screenRotBox.checked)
            VescIf.setLoadQmlUiOnConnect(qmlUiBox.checked)
            VescIf.setAskQmlLoad(qmlUiAskBox.checked)
            VescIf.setReconnectLastCan(reconnectLastCanBox.checked)
            VescIf.setScanCanOnConnect(scanCanConnectBox.checked)
            VescIf.setShowFwUpdateAvailable(fwUpdateAvailableBox.checked)
            VescIf.storeSettings()

            Utility.keepScreenOn(VescIf.keepScreenOn())

            if (VescIf.useWakeLock()) {
                VescIf.setWakeLock(VescIf.isPortConnected())
            }

            if (Utility.isDarkMode() !== mLastDarkMode) {
                darkChangedDialog.open()
            }

            VescIf.commands().emitEmptySetupValues()
        }
    }

    SheetDialog {
        id: darkChangedDialog
        acceptText: qsTr("Quit now")
        rejectText: qsTr("Later")
        title: "Theme Changed"

        Text {
            id: detectRlLabel
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text:
                "The theme has been changed. This requires restarting VESC Tool to take effect. " +
                "Do you want to close VESC Tool now?"
        }

        onAccepted: {
            Qt.quit()
        }
    }
}
