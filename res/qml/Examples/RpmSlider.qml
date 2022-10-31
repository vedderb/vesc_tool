/*
    Copyright 2020 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0

import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import "qrc:/mobile"

Item {
    id: mainItem
    anchors.fill: parent

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()

    ColumnLayout {
        id: gaugeColumn
        anchors.fill: parent
        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            CustomGauge {
                id: rpmGauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 12000
                minimumValue: 0
                tickmarkScale: 0.001
                tickmarkSuffix: "k"
                labelStep: 1000
                value: 0
                unitText: "RPM"
                typeText: "Speed"
            }

            CustomGauge {
                id: voltageGauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 100
                minimumValue: 0
                tickmarkScale: 1
                tickmarkSuffix: ""
                labelStep: 20
                value: 0
                unitText: "V"
                typeText: "Motor\nVoltage"
            }
        }
        
        RowLayout {
            Slider {
                from: 0
                to: 100
                Layout.fillWidth: true
                
                onValueChanged: {
                    if (activeBox.checked) {
                        mCommands.setDutyCycle(value / 100)
                    } else {
                        value = 0
                    }
                }
            }
            
            CheckBox {
                id: activeBox
                text: "Active"
            }
        }
        
        Timer {
            running: true
            repeat: true
            interval: 50
            
            onTriggered: {
                mCommands.getValues()
                if (activeBox.checked) {
                    mCommands.sendAlive()
                }
            }
        }

        Connections {
            target: mCommands

            function onValuesReceived(values, mask) {
                rpmGauge.value = Math.abs(values.rpm) / 5
                voltageGauge.value = Math.abs(values.v_in * values.duty_now)
            }
        }
    }
}
