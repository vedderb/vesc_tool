/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

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
    property var editorsVisible: []

    ParamEditors {
        id: editors
    }

    PpmMap {
        id: ppmMap
        parentWidth: column.width
    }

    AdcMap {
        id: adcMap
        parentWidth: column.width
    }

    NrfPair {
        id: nrfPair
        parentWidth: column.width
    }

    function addSpacer() {
        editorsVisible.push(Qt.createQmlObject(
                                'import QtQuick 2.7; import QtQuick.Layouts 1.3; Rectangle {Layout.fillHeight: true}',
                                scrollCol,
                                "spacer1"))
    }

    function addSeparator(text) {
        editorsVisible.push(editors.createSeparator(scrollCol, text))
    }

    function destroyEditors() {
        for (var i = 0;i < editorsVisible.length;i++) {
            editorsVisible[i].destroy();
        }
        editorsVisible = []
    }

    function createEditorApp(param) {
        editorsVisible.push(editors.createEditorApp(scrollCol, param))
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
            addSpacer()
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
                addSeparator("Multiple VESCs over CAN-bus")
                createEditorApp("app_ppm_conf.multi_esc")
                createEditorApp("app_ppm_conf.tc")
                createEditorApp("app_ppm_conf.tc_max_diff")
                addSpacer()
                break;
            case "Mapping":
                createEditorApp("app_ppm_conf.pulse_start")
                createEditorApp("app_ppm_conf.pulse_end")
                createEditorApp("app_ppm_conf.pulse_center")
                createEditorApp("app_ppm_conf.hyst")
                addSpacer()
                break;
            case "Throttle Curve":
                createEditorApp("app_ppm_conf.throttle_exp")
                createEditorApp("app_ppm_conf.throttle_exp_brake")
                createEditorApp("app_ppm_conf.throttle_exp_mode")
                addSpacer()
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
                addSpacer()
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
                addSpacer()
                break;
            case "Throttle Curve":
                createEditorApp("app_adc_conf.throttle_exp")
                createEditorApp("app_adc_conf.throttle_exp_brake")
                createEditorApp("app_adc_conf.throttle_exp_mode")
                addSpacer()
                break;
            default:
                break;
            }
            break;

        case "UART":
            createEditorApp("app_uart_baudrate")
            addSpacer()
            break;

        case "Nunchuk":
            switch(tabBox.currentText) {
            case "General":
                createEditorApp("app_chuk_conf.ctrl_type")
                createEditorApp("app_chuk_conf.ramp_time_pos")
                createEditorApp("app_chuk_conf.ramp_time_neg")
                createEditorApp("app_chuk_conf.stick_erpm_per_s_in_cc")
                createEditorApp("app_chuk_conf.hyst")
                addSeparator("Multiple VESCs over CAN-bus")
                createEditorApp("app_chuk_conf.multi_esc")
                createEditorApp("app_chuk_conf.tc")
                createEditorApp("app_chuk_conf.tc_max_diff")
                addSpacer()
                break;
            case "Throttle Curve":
                createEditorApp("app_chuk_conf.throttle_exp")
                createEditorApp("app_chuk_conf.throttle_exp_brake")
                createEditorApp("app_chuk_conf.throttle_exp_mode")
                addSpacer()
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
            createEditorApp("app_nrf_conf.address_0")
            createEditorApp("app_nrf_conf.address_1")
            createEditorApp("app_nrf_conf.address_2")
            addSpacer()
            break;

        default:
            break;
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 0

        ComboBox {
            id: pageBox
            Layout.fillWidth: true
            model: [
                "General",
                "PPM",
                "ADC",
                "UART",
                "Nunchuk",
                "NRF"
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

                case "Nunchuk":
                    tabBox.model = [
                                "General",
                                "Throttle Curve"
                            ]
                    break;

                case "NRF":
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

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: column.width
            clip: true

            ColumnLayout {
                id: scrollCol
                anchors.fill: parent
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
                            ppmMap.openDialog()
                        }
                    }
                    MenuItem {
                        text: "ADC Mapping..."
                        onTriggered: {
                            adcMap.openDialog()
                        }
                    }
                    MenuItem {
                        text: "Pair NRF..."
                        onTriggered: {
                            nrfPair.openDialog()
                        }
                    }
                }
            }
        }
    }
}
