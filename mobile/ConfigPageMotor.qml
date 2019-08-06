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

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    property Commands mCommands: VescIf.commands()
    property bool isHorizontal: width > height

    ParamEditors {
        id: editors
    }

    DetectBldc {
        id: detectBldc
    }

    DetectFocParam {
        id: detectFocParam
    }

    DetectFocHall {
        id: detectFocHall
    }

    DetectFocEncoder {
        id: detectFocEncoder
    }

    Dialog {
        id: directionSetupDialog
        title: "Direction Setup"
        standardButtons: Dialog.Close
        modal: true
        focus: true
        padding: 10

        width: parent.width - 10
        closePolicy: Popup.CloseOnEscape
        x: 5
        y: (parent.height - height) / 2
        parent: ApplicationWindow.overlay

        DirectionSetup {
            id: directionSetup
            anchors.fill: parent
        }
    }

    onIsHorizontalChanged: {
        updateEditors()
    }

    function addSeparator(text) {
        var e = editors.createSeparator(scrollCol, text)
        e.Layout.columnSpan = isHorizontal ? 2 : 1
    }

    function destroyEditors() {
        for(var i = scrollCol.children.length;i > 0;i--) {
            scrollCol.children[i - 1].destroy(1) // Only works with delay on android, seems to be a bug
        }
    }

    function createEditorMc(param) {
        var e = editors.createEditorMc(scrollCol, param)
        e.Layout.preferredWidth = 500
        e.Layout.fillsWidth = true
    }

    function updateEditors() {
        destroyEditors()

        switch (pageBox.currentText) {
        case "General":
            switch(tabBox.currentText) {
            case "General":
                createEditorMc("motor_type")
                createEditorMc("m_invert_direction")
                createEditorMc("m_sensor_port_mode")
                createEditorMc("m_encoder_counts")
                break;
            case "Current":
                addSeparator("Motor")
                createEditorMc("l_current_max")
                createEditorMc("l_current_min")
                createEditorMc("l_abs_current_max")
                createEditorMc("l_slow_abs_current")
                createEditorMc("l_current_max_scale")
                createEditorMc("l_current_min_scale")
                addSeparator("Battery")
                createEditorMc("l_in_current_max")
                createEditorMc("l_in_current_min")
                addSeparator("DRV8301")
                createEditorMc("m_drv8301_oc_mode")
                createEditorMc("m_drv8301_oc_adj")
                break;
            case "Voltage":
                createEditorMc("l_battery_cut_start")
                createEditorMc("l_battery_cut_end")
                break;
            case "RPM":
                createEditorMc("l_max_erpm")
                createEditorMc("l_min_erpm")
                createEditorMc("l_erpm_start")
                break;
            case "Wattage":
                createEditorMc("l_watt_max")
                createEditorMc("l_watt_min")
                break;
            case "Temperature":
                addSeparator("General")
                createEditorMc("l_temp_accel_dec")
                addSeparator("MOSFET")
                createEditorMc("l_temp_fet_start")
                createEditorMc("l_temp_fet_end")
                addSeparator("Motor")
                createEditorMc("l_temp_motor_start")
                createEditorMc("l_temp_motor_end")
                break;
            case "Advanced":
                createEditorMc("l_min_vin")
                createEditorMc("l_max_vin")
                createEditorMc("l_min_duty")
                createEditorMc("l_max_duty")
                createEditorMc("cc_min_current")
                createEditorMc("m_fault_stop_time_ms")
                createEditorMc("m_ntc_motor_beta")
                createEditorMc("m_out_aux_mode")
                break;
            default:
                break;
            }
            break;

        case "BLDC":
            switch(tabBox.currentText) {
            case "General":
                createEditorMc("sensor_mode")
                createEditorMc("comm_mode")
                createEditorMc("cc_startup_boost_duty")
                break;
            case "Sensorless":
                createEditorMc("sl_cycle_int_limit")
                createEditorMc("sl_min_erpm")
                createEditorMc("sl_min_erpm_cycle_int_limit")
                createEditorMc("sl_bemf_coupling_k")
                break;
            case "Sensors":
                createEditorMc("hall_sl_erpm")
                createEditorMc("hall_table__0")
                createEditorMc("hall_table__1")
                createEditorMc("hall_table__2")
                createEditorMc("hall_table__3")
                createEditorMc("hall_table__4")
                createEditorMc("hall_table__5")
                createEditorMc("hall_table__6")
                createEditorMc("hall_table__7")
                break;
            case "Advanced":
                createEditorMc("sl_phase_advance_at_br")
                createEditorMc("sl_cycle_int_rpm_br")
                createEditorMc("l_max_erpm_fbrake")
                createEditorMc("pwm_mode")
                createEditorMc("cc_gain")
                createEditorMc("cc_ramp_step_max")
                createEditorMc("m_duty_ramp_step")
                createEditorMc("m_current_backoff_gain")
                createEditorMc("m_bldc_f_sw_min")
                createEditorMc("m_bldc_f_sw_max")
                break;
            default:
                break;
            }
            break;

        case "DC":
            createEditorMc("cc_gain")
            createEditorMc("cc_ramp_step_max")
            createEditorMc("m_duty_ramp_step")
            createEditorMc("m_current_backoff_gain")
            createEditorMc("m_dc_f_sw")
            break;

        case "FOC":
            switch(tabBox.currentText) {
            case "General":
                createEditorMc("foc_sensor_mode")
                createEditorMc("foc_motor_r")
                createEditorMc("foc_motor_l")
                createEditorMc("foc_motor_flux_linkage")
                createEditorMc("foc_current_kp")
                createEditorMc("foc_current_ki")
                createEditorMc("foc_observer_gain")
                break;
            case "Sensorless":
                createEditorMc("foc_openloop_rpm")
                createEditorMc("foc_sl_openloop_hyst")
                createEditorMc("foc_sl_openloop_time")
                createEditorMc("foc_sat_comp")
                createEditorMc("foc_temp_comp")
                createEditorMc("foc_temp_comp_base_temp")
                break;
            case "Hall Sensors":
                createEditorMc("foc_sl_erpm")
                createEditorMc("foc_hall_table__0")
                createEditorMc("foc_hall_table__1")
                createEditorMc("foc_hall_table__2")
                createEditorMc("foc_hall_table__3")
                createEditorMc("foc_hall_table__4")
                createEditorMc("foc_hall_table__5")
                createEditorMc("foc_hall_table__6")
                createEditorMc("foc_hall_table__7")
                break;
            case "Encoder":
                createEditorMc("foc_sl_erpm")
                createEditorMc("foc_encoder_offset")
                createEditorMc("foc_encoder_ratio")
                createEditorMc("foc_encoder_inverted")
                createEditorMc("foc_encoder_sin_gain")
                createEditorMc("foc_encoder_sin_offset")
                createEditorMc("foc_encoder_cos_gain")
                createEditorMc("foc_encoder_cos_offset")
                createEditorMc("foc_encoder_sincos_filter_constant")
                break;
            case "Advanced":
                createEditorMc("foc_f_sw")
                createEditorMc("foc_dt_us")
                createEditorMc("foc_pll_kp")
                createEditorMc("foc_pll_ki")
                createEditorMc("foc_duty_dowmramp_kp")
                createEditorMc("foc_duty_dowmramp_ki")
                createEditorMc("foc_sl_d_current_duty")
                createEditorMc("foc_sl_d_current_factor")
                createEditorMc("foc_sample_v0_v7")
                createEditorMc("foc_sample_high_current")
                createEditorMc("foc_observer_gain_slow")
                createEditorMc("foc_current_filter_const")
                break;
            default:
                break;
            }
            break;

        case "GPD":
            createEditorMc("pwm_mode")
            createEditorMc("gpd_buffer_notify_left")
            createEditorMc("gpd_buffer_interpol")
            createEditorMc("gpd_current_filter_const")
            createEditorMc("gpd_current_kp")
            createEditorMc("gpd_current_ki")
            break;

        case "PID Controllers":
            addSeparator("Speed Controller")
            createEditorMc("s_pid_kp")
            createEditorMc("s_pid_ki")
            createEditorMc("s_pid_kd")
            createEditorMc("s_pid_kd_filter")
            createEditorMc("s_pid_min_erpm")
            createEditorMc("s_pid_allow_braking")
            addSeparator("Position Controller")
            createEditorMc("p_pid_kp")
            createEditorMc("p_pid_ki")
            createEditorMc("p_pid_kd")
            createEditorMc("p_pid_kd_filter")
            createEditorMc("p_pid_ang_div")
            break;

        case "Additional Info":
            switch (tabBox.currentText) {
            case "Setup":
                createEditorMc("si_motor_poles")
                createEditorMc("si_gear_ratio")
                createEditorMc("si_wheel_diameter")
                createEditorMc("si_battery_type")
                createEditorMc("si_battery_cells")
                createEditorMc("si_battery_ah")
                break;
            case "General":
                createEditorMc("motor_brand")
                createEditorMc("motor_model")
                createEditorMc("motor_weight")
                createEditorMc("motor_poles")
                createEditorMc("motor_sensor_type")
                createEditorMc("motor_loss_torque")
                break;
            case "Quality":
                createEditorMc("motor_quality_bearings")
                createEditorMc("motor_quality_magnets")
                createEditorMc("motor_quality_construction")
                break;

            default:
                break;
            }

            break;

        default:
            break;
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 0

        GridLayout {
            Layout.fillWidth: true
            columns: isHorizontal ? 2 : 1
            rowSpacing: -5
            ComboBox {
                id: pageBox
                Layout.fillWidth: true
                model: [
                    "General",
                    "BLDC",
                    "DC",
                    "FOC",
                    "GPD",
                    "PID Controllers",
                    "Additional Info"
                ]
                
                onCurrentTextChanged: {
                    var tabTextOld = tabBox.currentText
                    
                    switch(currentText) {
                    case "General":
                        tabBox.model = [
                                    "General",
                                    "Current",
                                    "Voltage",
                                    "RPM",
                                    "Wattage",
                                    "Temperature",
                                    "Advanced"
                                ]
                        break;
                        
                    case "BLDC":
                        tabBox.model = [
                                    "General",
                                    "Sensorless",
                                    "Sensors",
                                    "Advanced"
                                ]
                        break;
                        
                    case "DC":
                        tabBox.model = []
                        break;
                        
                    case "FOC":
                        tabBox.model = [
                                    "General",
                                    "Sensorless",
                                    "Hall Sensors",
                                    "Encoder",
                                    "Advanced"
                                ]
                        break;
                        
                    case "GPD":
                        tabBox.model = []
                        break;
                        
                    case "PID Controllers":
                        tabBox.model = []
                        break;
                        
                    case "Additional Info":
                        tabBox.model = [
                                    "Setup",
                                    "General",
                                    "Quality"
                                ]
                        break;
                        
                    default:
                        break;
                    }
                    
                    tabBox.visible = tabBox.currentText.length !== 0
                    
                    if (tabTextOld == tabBox.currentText) {
                        updateEditors()
                    }
                }
            }
            
            ComboBox {
                id: tabBox
                Layout.fillWidth: true
                
                onCurrentTextChanged: {
                    updateEditors()
                }
            }
        }
        
        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: column.width
            clip: true

            GridLayout {
                id: scrollCol
                anchors.fill: parent
                columns: isHorizontal ? 2 : 1
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Button {
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                text: "Write"

                onClicked: {
                    mCommands.setMcconf(true)
                }
            }

            Button {
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                text: "Read"

                onClicked: {
                    mCommands.getMcconf()
                }
            }

            Button {
                Layout.preferredWidth: 50
                Layout.fillWidth: true
                text: "..."
                onClicked: menu.open()

                Menu {
                    id: menu
                    width: 500

                    MenuItem {
                        text: "Read Default Settings"
                        onTriggered: {
                            mCommands.getMcconfDefault()
                        }
                    }
                    MenuItem {
                        text: "Detect BLDC Parameters..."
                        onTriggered: {
                            detectBldc.openDialog()
                        }
                    }
                    MenuItem {
                        text: "Detect FOC Parameters..."
                        onTriggered: {
                            detectFocParam.openDialog()
                        }
                    }
                    MenuItem {
                        text: "Detect FOC Hall Sensors..."
                        onTriggered: {
                            detectFocHall.openDialog()
                        }
                    }
                    MenuItem {
                        text: "Detect FOC Encoder..."
                        onTriggered: {
                            detectFocEncoder.openDialog()
                        }
                    }
                    MenuItem {
                        text: "Setup Motor Directions..."
                        onTriggered: {
                            directionSetupDialog.open()
                            directionSetup.scanCan()
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: mCommands

        // TODO: For some reason this does not work
        onMcConfigCheckResult: {
            if (paramsNotSet.length > 0) {
                var notUpdated = "The following parameters were truncated because " +
                        "they were beyond the hardware limits:\n"

                for (var i = 0;i < paramsNotSet.length;i++) {
                    notUpdated += mMcConf.getLongName(paramsNotSet[i]) + "\n"
                }

                VescIf.emitMessageDialog("Parameters truncated", notUpdated, false, false)
            }
        }
    }
}
