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

import QtQuick 2.0
import QtQuick.Controls 2.10
import QtQuick.Controls.Material
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    property var dialogParent: Overlay.overlay
    property alias name: nameText.text
    property ConfigParams mMcConf: VescIf.mcConfig()
    property Commands mCommands: VescIf.commands()
    property int index: 0
    property bool active: false
    property var stats: []
    signal editRequested(int index)
    signal deleteRequested(int index)

    implicitHeight: column.implicitHeight + 2 * column.anchors.margins
    Layout.fillWidth: true

    function setFromMcConfTemp(cfg) {
        name = cfg.name

        var useImperial = VescIf.useImperialUnits()
        var impFact = useImperial ? 0.621371192 : 1.0
        var speedUnit = useImperial ? "mph" : "km/h"
        var powerLimited = cfg.watt_max < 1400000 || cfg.watt_min > -1400000

        stats = [["Speed Fwd / Rev", parseFloat(cfg.erpm_or_speed_max * 3.6 * impFact).toFixed(1) + " / " +
                  parseFloat(-cfg.erpm_or_speed_min * 3.6 * impFact).toFixed(1), speedUnit],
                 ["Accel / Brake", parseFloat(cfg.current_max_scale * 100).toFixed(0) + " / " +
                  parseFloat(cfg.current_min_scale * 100).toFixed(0), "%"],
                 ["Power Out / Regen", powerLimited ? parseFloat(cfg.watt_max / 1000).toFixed(1) + " / " +
                                                     parseFloat(cfg.watt_min / 1000).toFixed(1) : "-", powerLimited ? "kW" : ""]]
    }

    function checkActive() {
        active = VescIf.isProfileInUse(index)
    }

    Rectangle {
        id: rect
        anchors.fill: parent
        color: active ? Qt.alpha(Material.accent, 0.08) : Utility.getAppHexColor("cardBackground")
        radius: 12
        border.color: active ? Material.accent : Utility.getAppHexColor("raisedBackground")
        border.width: active ? 1.5 : 1

        ColumnLayout {
            id: column
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            RowLayout {
                spacing: 12

                Image {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    source: "qrc" + Utility.getThemePath() + "icons/Speed-96.png"
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    Text {
                        id: nameText
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        color: Utility.getAppHexColor("titleText")
                        font.weight: Font.Medium
                    }

                    Text {
                        text: active ? qsTr("Active") : qsTr("Not in use")
                        font.pointSize: 9
                        color: active ? Utility.getAppHexColor("accentText") : Utility.getAppHexColor("normalText")
                    }
                }

                Button {
                    text: qsTr("Apply")
                    onClicked: {
                        mCommands.setMcconfTemp(VescIf.getProfile(index),
                                                true, false, true, false, true)
                        VescIf.updateMcconfFromProfile(VescIf.getProfile(index))
                        showToast(qsTr("Applied until reboot"), name, "ok", qsTr("Use permanently"), function() {
                            permanentDialog.open()
                        })
                    }
                }

                ToolButton {
                    icon.source: "qrc" + Utility.getThemePath() + "icons_textedit/Menu 2-96.png"
                    Accessible.name: qsTr("Profile options")
                    onClicked: profileMenu.open()

                    PopupMenu {
                        id: profileMenu

                        MenuItem {
                            text: "Use permanently"
                            onTriggered: {
                                permanentDialog.open()
                            }
                        }

                        MenuItem {
                            text: "Edit"
                            onTriggered: {
                                editRequested(index)
                            }
                        }

                        MenuItem {
                            text: "Move Up"
                            onTriggered: {
                                VescIf.moveProfileUp(index)
                                VescIf.storeSettings()
                            }
                        }

                        MenuItem {
                            text: "Move Down"
                            onTriggered: {
                                VescIf.moveProfileDown(index)
                                VescIf.storeSettings()
                            }
                        }

                        MenuItem {
                            text: "Delete"
                            onTriggered: {
                                deleteRequested(index)
                            }
                        }
                    }
                }
            }

            RowLayout {
                spacing: 6

                Repeater {
                    model: stats

                    ValueTile {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        color: Utility.getAppHexColor("darkBackground")
                        border.width: 0
                        title: modelData[0]
                        value: modelData[1]
                        unit: modelData[2]
                    }
                }
            }
        }
    }

    SheetDialog {
        id: permanentDialog
        title: "Use Profile Permanently"
        acceptText: qsTr("Use permanently")
        parent: dialogParent

        Text {
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "This is going to apply this profile permanently, even after reboot. Are " +
                  "you sure?"
        }

        onAccepted: {
            mCommands.setMcconfTemp(VescIf.getProfile(index),
                                    true, true, true, false, true)
            VescIf.updateMcconfFromProfile(VescIf.getProfile(index))
        }
    }

    Connections {
        target: mMcConf

        function onParamChangedDouble(src, name, newParam) {
            checkActive()
        }
    }

    Connections {
        target: VescIf

        function onUseImperialUnitsChanged(useImperialUnits) {
            setFromMcConfTemp(VescIf.getProfile(index))
        }
    }
}
