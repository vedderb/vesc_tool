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

    Dialog {
        id: ppmMap
        title: "PPM Mapping"
        standardButtons: Dialog.Close
        modal: true
        focus: true

        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        x: 10
        y: (parent.height - height) / 2
        parent: ApplicationWindow.overlay

        PpmMap {
            anchors.fill: parent
        }
    }

    Dialog {
        id: adcMap
        title: "ADC Mapping"
        standardButtons: Dialog.Close
        modal: true
        focus: true

        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        x: 10
        y: parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        AdcMap {
            anchors.fill: parent
        }
    }

    Dialog {
        id: nrfPair
        title: "NRF Pairing"
        standardButtons: Dialog.Close
        modal: true
        focus: true

        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        x: 10
        y: parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        NrfPair {
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

    function createEditorApp(param) {
        var e = editors.createEditorApp(scrollCol, param)
        e.Layout.preferredWidth = 500
        e.Layout.fillsWidth = true
    }

    function updateEditors() {
        destroyEditors()

        switch (pageBox.currentText) {
        case "General":
            createEditorApp("app_to_use")
            createEditorApp("controller_id")
            createEditorApp("timeout_msec")
            createEditorApp("timeout_brake_current")
            createEditorApp("send_can_status")
            createEditorApp("send_can_status_rate_hz")
            createEditorApp("can_baud_rate")
            createEditorApp("pairing_done")
            createEditorApp("permanent_uart_enabled")
            createEditorApp("shutdown_mode")
            createEditorApp("uavcan_enable")
            createEditorApp("uavcan_esc_index")
            break;

        case "PPM":
            switch(tabBox.currentText) {
            case "General":
                createEditorApp("app_ppm_conf.ctrl_type")
                createEditorApp("app_ppm_conf.median_filter")
                createEditorApp("app_ppm_conf.safe_start")
                createEditorApp("app_ppm_conf.pid_max_erpm")
                createEditorApp("app_ppm_conf.ramp_time_pos")
                createEditorApp("app_ppm_conf.ramp_time_neg")
                createEditorApp("app_ppm_conf.max_erpm_for_dir")
                createEditorApp("app_ppm_conf.smart_rev_max_duty")
                createEditorApp("app_ppm_conf.smart_rev_ramp_time")
                addSeparator("Multiple VESCs over CAN-bus")
                createEditorApp("app_ppm_conf.multi_esc")
                createEditorApp("app_ppm_conf.tc")
                createEditorApp("app_ppm_conf.tc_max_diff")
                break;
            case "Mapping":
                createEditorApp("app_ppm_conf.pulse_start")
                createEditorApp("app_ppm_conf.pulse_end")
                createEditorApp("app_ppm_conf.pulse_center")
                createEditorApp("app_ppm_conf.hyst")
                break;
            case "Throttle Curve":
                createEditorApp("app_ppm_conf.throttle_exp")
                createEditorApp("app_ppm_conf.throttle_exp_brake")
                createEditorApp("app_ppm_conf.throttle_exp_mode")
                break;
            default:
                break;
            }
            break;

        case "ADC":
            switch(tabBox.currentText) {
            case "General":
                createEditorApp("app_adc_conf.ctrl_type")
                createEditorApp("app_adc_conf.use_filter")
                createEditorApp("app_adc_conf.safe_start")
                createEditorApp("app_adc_conf.cc_button_inverted")
                createEditorApp("app_adc_conf.rev_button_inverted")
                createEditorApp("app_adc_conf.update_rate_hz")
                createEditorApp("app_adc_conf.ramp_time_pos")
                createEditorApp("app_adc_conf.ramp_time_neg")
                addSeparator("Multiple VESCs over CAN-bus")
                createEditorApp("app_adc_conf.multi_esc")
                createEditorApp("app_adc_conf.tc")
                createEditorApp("app_adc_conf.tc_max_diff")
                break;
            case "Mapping":
                createEditorApp("app_adc_conf.hyst")
                addSeparator("ADC 1")
                createEditorApp("app_adc_conf.voltage_start")
                createEditorApp("app_adc_conf.voltage_end")
                createEditorApp("app_adc_conf.voltage_center")
                createEditorApp("app_adc_conf.voltage_inverted")
                addSeparator("ADC 2")
                createEditorApp("app_adc_conf.voltage2_start")
                createEditorApp("app_adc_conf.voltage2_end")
                createEditorApp("app_adc_conf.voltage2_inverted")
                break;
            case "Throttle Curve":
                createEditorApp("app_adc_conf.throttle_exp")
                createEditorApp("app_adc_conf.throttle_exp_brake")
                createEditorApp("app_adc_conf.throttle_exp_mode")
                break;
            default:
                break;
            }
            break;

        case "UART":
            createEditorApp("app_uart_baudrate")
            break;

        case "VESC Remote":
            switch(tabBox.currentText) {
            case "General":
                createEditorApp("app_chuk_conf.ctrl_type")
                createEditorApp("app_chuk_conf.ramp_time_pos")
                createEditorApp("app_chuk_conf.ramp_time_neg")
                createEditorApp("app_chuk_conf.stick_erpm_per_s_in_cc")
                createEditorApp("app_chuk_conf.hyst")
                createEditorApp("app_chuk_conf.use_smart_rev")
                createEditorApp("app_chuk_conf.smart_rev_max_duty")
                createEditorApp("app_chuk_conf.smart_rev_ramp_time")
                addSeparator("Multiple VESCs over CAN-bus")
                createEditorApp("app_chuk_conf.multi_esc")
                createEditorApp("app_chuk_conf.tc")
                createEditorApp("app_chuk_conf.tc_max_diff")
                break;
            case "Throttle Curve":
                createEditorApp("app_chuk_conf.throttle_exp")
                createEditorApp("app_chuk_conf.throttle_exp_brake")
                createEditorApp("app_chuk_conf.throttle_exp_mode")
                break;
            default:
                break;
            }
            break;

        case "NRF":
            addSeparator("Radio")
            createEditorApp("app_nrf_conf.power")
            createEditorApp("app_nrf_conf.speed")
            createEditorApp("app_nrf_conf.channel")
            addSeparator("Integrity")
            createEditorApp("app_nrf_conf.crc_type")
            createEditorApp("app_nrf_conf.send_crc_ack")
            createEditorApp("app_nrf_conf.retry_delay")
            createEditorApp("app_nrf_conf.retries")
            addSeparator("Address")
            createEditorApp("app_nrf_conf.address__0")
            createEditorApp("app_nrf_conf.address__1")
            createEditorApp("app_nrf_conf.address__2")
            break;

        case "Balance":
            switch(tabBox.currentText) {
            case "Config":
                addSeparator("Startup")
                createEditorApp("app_balance_conf.startup_pitch_tolerance")
                createEditorApp("app_balance_conf.startup_roll_tolerance")
                createEditorApp("app_balance_conf.startup_speed")
                addSeparator("Tiltback")
                createEditorApp("app_balance_conf.tiltback_duty")
                createEditorApp("app_balance_conf.tiltback_angle")
                createEditorApp("app_balance_conf.tiltback_speed")
                createEditorApp("app_balance_conf.tiltback_high_voltage")
                createEditorApp("app_balance_conf.tiltback_low_voltage")
                addSeparator("Overspeed")
                createEditorApp("app_balance_conf.overspeed_duty")
                addSeparator("Fault")
                createEditorApp("app_balance_conf.pitch_fault")
                createEditorApp("app_balance_conf.roll_fault")
                createEditorApp("app_balance_conf.use_switches")
                break;
            case "Tune":
                addSeparator("PID")
                createEditorApp("app_balance_conf.kp")
                createEditorApp("app_balance_conf.ki")
                createEditorApp("app_balance_conf.kd")
                addSeparator("Main Loop")
                createEditorApp("app_balance_conf.hertz")
                addSeparator("Experimental")
                createEditorApp("app_balance_conf.deadzone")
                createEditorApp("app_balance_conf.current_boost")
                break;
            default:
                break;
            }
            break;

        case "IMU":
            createEditorApp("imu_conf.type")
            createEditorApp("imu_conf.sample_rate_hz")
            addSeparator("Filters")
            createEditorApp("imu_conf.mode")
            createEditorApp("imu_conf.accel_confidence_decay")
            createEditorApp("imu_conf.mahony_kp")
            createEditorApp("imu_conf.mahony_ki")
            createEditorApp("imu_conf.madgwick_beta")
            addSeparator("Rotation")
            createEditorApp("imu_conf.rot_roll")
            createEditorApp("imu_conf.rot_pitch")
            createEditorApp("imu_conf.rot_yaw")
            addSeparator("Offsets")
            createEditorApp("imu_conf.accel_offsets__0")
            createEditorApp("imu_conf.accel_offsets__1")
            createEditorApp("imu_conf.accel_offsets__2")
            createEditorApp("imu_conf.gyro_offsets__0")
            createEditorApp("imu_conf.gyro_offsets__1")
            createEditorApp("imu_conf.gyro_offsets__2")
            createEditorApp("imu_conf.gyro_offset_comp_fact__0")
            createEditorApp("imu_conf.gyro_offset_comp_fact__1")
            createEditorApp("imu_conf.gyro_offset_comp_fact__2")
            createEditorApp("imu_conf.gyro_offset_comp_clamp")
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
                    "PPM",
                    "ADC",
                    "UART",
                    "VESC Remote",
                    "NRF",
                    "Balance",
                    "IMU"
                ]

                onCurrentTextChanged: {
                    var tabTextOld = tabBox.currentText

                    switch(currentText) {
                    case "General":
                        tabBox.model = []
                        break;

                    case "PPM":
                        tabBox.model = [
                                    "General",
                                    "Mapping",
                                    "Throttle Curve"
                                ]
                        break;

                    case "ADC":
                        tabBox.model = [
                                    "General",
                                    "Mapping",
                                    "Throttle Curve"
                                ]
                        break;

                    case "UART":
                        tabBox.model = []
                        break;

                    case "VESC Remote":
                        tabBox.model = [
                                    "General",
                                    "Throttle Curve"
                                ]
                        break;

                    case "NRF":
                        tabBox.model = []
                        break;

                    case "Balance":
                        tabBox.model = [
                                    "Config",
                                    "Tune"
                                ]
                        break;

                    case "IMU":
                        tabBox.model = []
                        break;

                    default:
                        tabBox.model = []
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
                    mCommands.setAppConf()
                }
            }

            Button {
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                text: "Read"

                onClicked: {
                    mCommands.getAppConf()
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
                            mCommands.getAppConfDefault()
                        }
                    }
                    MenuItem {
                        text: "PPM Mapping..."
                        onTriggered: {
                            ppmMap.open()
                        }
                    }
                    MenuItem {
                        text: "ADC Mapping..."
                        onTriggered: {
                            adcMap.open()
                        }
                    }
                    MenuItem {
                        text: "Pair NRF..."
                        onTriggered: {
                            nrfPair.open()
                        }
                    }
                }
            }
        }
    }
}
