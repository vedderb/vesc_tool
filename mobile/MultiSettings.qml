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
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    property var dialogParent: ApplicationWindow.overlay

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()
    property var canDevs: []

    function openDialog() {
        if (!VescIf.isPortConnected()) {
            VescIf.emitMessageDialog("Multi Setup",
                                     "You are not connected. Please connect first.", false, false)
            return
        }

        workaroundTimerOpenDialog.start()
    }
    Timer {
        id: workaroundTimerOpenDialog
        interval: 0
        repeat: false
        running: false
        onTriggered: {
            dialog.open()

            disableDialog("Scanning CAN-bus...")
            canDevs = Utility.scanCanVescOnly(VescIf)

            if (!Utility.isConnectedToHwVesc(VescIf)) {
                if (canDevs.length > 0) {
                    mCommands.setSendCan(true, canDevs[0])
                }
            }

            enableDialog()
        }
    }

    Dialog {
        id: dialog
        parent: dialogParent
        modal: true
        focus: true
        width: parent.width - 40 - notchLeft - notchRight
        height: parent.height - 40 - notchBot - notchTop
        closePolicy: Popup.CloseOnEscape
        x: 20 + (notchLeft + notchRight)/2
        y: 10 + notchTop
        topPadding: 5
        bottomPadding: 0
        leftMargin: 5
        rightMargin: 5

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        Component.onCompleted: {
            // General
            paramListGeneral.addEditorMc("foc_sensor_mode")
            paramListGeneral.addEditorMc("l_duty_start")
            paramListGeneral.addEditorMc("m_motor_temp_sens_type")
            paramListGeneral.addEditorMc("m_ntc_motor_beta")

            // Limits
            paramListLimits.addSeparator("Current")
            paramListLimits.addEditorMc("l_current_max")
            paramListLimits.addEditorMc("l_current_min")
            paramListLimits.addEditorMc("l_in_current_max")
            paramListLimits.addEditorMc("l_in_current_min")
            paramListLimits.addEditorMc("l_abs_current_max")

            paramListLimits.addSeparator("ERPM")
            paramListLimits.addEditorMc("l_min_erpm")
            paramListLimits.addEditorMc("l_max_erpm")

            paramListLimits.addSeparator("Voltage")
            paramListLimits.addEditorMc("l_battery_cut_start")
            paramListLimits.addEditorMc("l_battery_cut_end")

            paramListLimits.addSeparator("Temperature")
            paramListLimits.addEditorMc("l_temp_fet_start")
            paramListLimits.addEditorMc("l_temp_fet_end")
            paramListLimits.addEditorMc("l_temp_motor_start")
            paramListLimits.addEditorMc("l_temp_motor_end")
            paramListLimits.addEditorMc("l_temp_accel_dec")

            paramListLimits.addSeparator("Wattage")
            paramListLimits.addEditorMc("l_watt_max")
            paramListLimits.addEditorMc("l_watt_min")

            // FOC
            paramListFoc.addEditorMc("foc_f_zv")
            paramListFoc.addEditorMc("foc_openloop_rpm")
            paramListFoc.addEditorMc("foc_motor_r")
            paramListFoc.addEditorMc("foc_motor_l")
            paramListFoc.addEditorMc("foc_motor_flux_linkage")
            paramListFoc.addEditorMc("foc_current_kp")
            paramListFoc.addEditorMc("foc_current_ki")

            // BMS
            paramListBms.addEditorMc("bms.t_limit_start")
            paramListBms.addEditorMc("bms.t_limit_end")
            paramListBms.addEditorMc("bms.soc_limit_start")
            paramListBms.addEditorMc("bms.soc_limit_end")
        }

        contentItem: ColumnLayout {
            anchors.fill: parent
            spacing: 0
            anchors.margins: 10
            anchors.topMargin: tabBar.height + 10
            anchors.bottomMargin: 5

            SwipeView {
                id: swipeView
                currentIndex: tabBar.currentIndex
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true

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
                    background: Rectangle {
                        opacity: 0.0
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            ParamList {
                                id: paramListGeneral
                                anchors.fill: parent
                            }
                        }

                        RowLayout {
                            Button {
                                Layout.fillWidth: true
                                text: "Write General to All"

                                onClicked: {
                                    disableDialog("Writing Parameters...")
                                    workaroundTimerWriteGeneralAll.start()
                                }
                                Timer {
                                    id: workaroundTimerWriteGeneralAll
                                    interval: 0
                                    repeat: false
                                    running: false
                                    onTriggered: {
                                        Utility.setMcParamsFromCurrentConfigAllCan(VescIf, canDevs, paramListGeneral.getParamNames())
                                        enableDialog()
                                    }
                                }
                            }
                            Button {
                                Layout.fillWidth: true
                                text: "Close"
                                onClicked: {
                                    dialog.close()
                                }
                            }
                        }
                    }
                }

                Page {
                    background: Rectangle {
                        opacity: 0.0
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            ParamList {
                                id: paramListLimits
                                anchors.fill: parent
                            }
                        }
                        RowLayout {
                            Button {
                                Layout.fillWidth: true
                                text: "Write Limits to All"
                                onClicked: {
                                    disableDialog("Writing Parameters...")
                                    workaroundTimerWriteLimitsAll.start()
                                }
                                Timer {
                                    id: workaroundTimerWriteLimitsAll
                                    interval: 0
                                    repeat: false
                                    running: false
                                    onTriggered: {
                                        Utility.setMcParamsFromCurrentConfigAllCan(VescIf, canDevs, paramListLimits.getParamNames())
                                        enableDialog()
                                    }
                                }
                            }
                            Button {
                                Layout.fillWidth: true
                                text: "Close"
                                onClicked: {
                                    dialog.close()
                                }
                            }
                        }
                    }
                }

                Page {
                    background: Rectangle {
                        opacity: 0.0
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            ParamList {
                                id: paramListFoc
                                anchors.fill: parent
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            DoubleSpinBox {
                                id: tcBox
                                Layout.fillWidth: true
                                decimals: 1
                                realValue: 1000.0
                                realFrom: 0.0
                                realTo: 1000000.0
                                realStepSize: 100.0
                                prefix: "Ts: "
                                suffix: " µS"
                            }

                            Button {
                                text: "Calculate Kp&Ki"

                                onClicked: {
                                    var ind = mMcConf.getParamDouble("foc_motor_l")
                                    var res = mMcConf.getParamDouble("foc_motor_r")

                                    var tc = tcBox.realValue * 1e-6
                                    var bw = 1.0 / tc
                                    var kp = ind * bw;
                                    var ki = res * bw;

                                    mMcConf.updateParamDouble("foc_current_kp", kp)
                                    mMcConf.updateParamDouble("foc_current_ki", ki)
                                }
                            }
                        }
                        RowLayout {
                            Button {
                                Layout.fillWidth: true
                                text: "Write FOC to All"

                                onClicked: {
                                    disableDialog("Writing Parameters...")
                                    workaroundTimerWriteFOCAll.start()
                                }
                                Timer {
                                    id: workaroundTimerWriteFOCAll
                                    interval: 0
                                    repeat: false
                                    running: false
                                    onTriggered: {
                                        Utility.setMcParamsFromCurrentConfigAllCan(VescIf, canDevs, paramListFoc.getParamNames())
                                        enableDialog()
                                    }
                                }
                            }
                            Button {
                                Layout.fillWidth: true
                                text: "Close"
                                onClicked: {
                                    dialog.close()
                                }
                            }
                        }
                    }
                }

                Page {
                    background: Rectangle {
                        opacity: 0.0
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            ParamList {
                                id: paramListBms
                                anchors.fill: parent
                            }
                        }
                        RowLayout {
                            Button {
                                Layout.fillWidth: true
                                text: "Write BMS to All"

                                onClicked: {
                                    disableDialog("Writing Parameters...")
                                    workaroundTimerWriteBMSAll.start()
                                }
                                Timer {
                                    id: workaroundTimerWriteBMSAll
                                    interval: 0
                                    repeat: false
                                    running: false
                                    onTriggered: {
                                        Utility.setMcParamsFromCurrentConfigAllCan(VescIf, canDevs, paramListBms.getParamNames())
                                        enableDialog()
                                    }
                                }
                            }
                            Button {
                                Layout.fillWidth: true
                                text: "Close"
                                onClicked: {
                                    dialog.close()
                                }
                            }
                        }
                    }
                }
            }
        }

        header: Rectangle {
            color: {color = Utility.getAppHexColor("lightText")}
            height: tabBar.height

            TabBar {
                id: tabBar
                currentIndex: swipeView.currentIndex
                anchors.fill: parent
                implicitWidth: 0
                clip: true

                background: Rectangle {
                    opacity: 1
                    color: { color = Utility.getAppHexColor("lightestBackground") }
                }

                property int buttonWidth: Math.max(120, tabBar.width / (rep.model.length))

                Repeater {
                    id: rep
                    model: ["General", "Limits", "FOC", "BMS"]

                    TabButton {
                        text: modelData
                        width: tabBar.buttonWidth
                    }
                }
            }
        }
    }

    function disableDialog(reason) {
        commDialog.title = reason
        commDialog.open()
        dialog.enabled = false
    }

    function enableDialog() {
        commDialog.close()
        dialog.enabled = true
    }

    Dialog {
        id: commDialog
        title: "Communicating..."
        closePolicy: Popup.NoAutoClose
        modal: true
        focus: true

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 20 - notchLeft - notchRight
        x: 10 + (notchLeft + notchRight)/2
        y: parent.height / 2 - height / 2
        parent: dialogParent

        ProgressBar {
            anchors.fill: parent
            indeterminate: visible
        }
    }
}
