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
    implicitHeight: column.implicitHeight

    property real calState: 1
    property real count: 0
    property var mCal: []

    property Commands mCommands: VescIf.commands()
    property ConfigParams mAppConf: VescIf.appConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()

    function updateDisplay() {
        if(calState == 1){
            resultArea.text = "Keep IMU level and steady during calibration. Press \"Calibrate IMU\" to begin."
            yawBox.visible = true
        } else if(calState == 2){
            resultArea.text = "Calibrating..."
            yawBox.visible = false
        } else if(calState == 3){
            resultArea.text =
                "Roll  : " + parseFloat(mCal[0]).toFixed(2) + "\n" +
                "Pitch  : " + parseFloat(mCal[1]).toFixed(2) + "\n" +
                "Yaw  : " + parseFloat(mCal[2]).toFixed(2) + "\n" +
                "Accel X  : " + parseFloat(mCal[3]).toFixed(2) + "\n" +
                "Accel Y  : " + parseFloat(mCal[4]).toFixed(2) + "\n" +
                "Accel Z  : " + parseFloat(mCal[5]).toFixed(2) + "\n" +
                "Gyro X  : " + parseFloat(mCal[6]).toFixed(2) + "\n" +
                "Gyro Y  : " + parseFloat(mCal[7]).toFixed(2) + "\n" +
                "Gyro Z  : " + parseFloat(mCal[8]).toFixed(2) + "\n"
            yawBox.visible = true
        } else if(calState == 4){
            resultArea.text = "IMU Calibration has been applied."
            yawBox.visible = true
        }

    }

    function isValid() {
        return (msMax - msMin) > 0.4
    }

    function applyMapping() {
        mAppConf.updateParamDouble("imu_conf.rot_roll", mCal[0])
        mAppConf.updateParamDouble("imu_conf.rot_pitch", mCal[1])
        mAppConf.updateParamDouble("imu_conf.rot_yaw", mCal[2])
        mAppConf.updateParamDouble("imu_conf.accel_offsets__0", mCal[3])
        mAppConf.updateParamDouble("imu_conf.accel_offsets__1", mCal[4])
        mAppConf.updateParamDouble("imu_conf.accel_offsets__2", mCal[5])
        mAppConf.updateParamDouble("imu_conf.gyro_offsets__0", mCal[6])
        mAppConf.updateParamDouble("imu_conf.gyro_offsets__1", mCal[7])
        mAppConf.updateParamDouble("imu_conf.gyro_offsets__2", mCal[8])
        mCommands.setAppConf()
        VescIf.emitStatusMessage("IMU Calibration applied!", true)
        calState = 4
        updateDisplay()
    }

    function testConnected() {
        if (VescIf.isPortConnected()) {
            return true
        } else {
            VescIf.emitMessageDialog(
                        "Connection Error",
                        "The VESC is not connected. Please connect it to run detection.",
                        false, false)
            return false
        }
    }

    Component.onCompleted: {
        updateDisplay()
    }

    ColumnLayout {
        id: column

        anchors.fill: parent
        spacing: 0

        TextArea {
            id: resultArea
            Layout.fillWidth: true
            readOnly: true
            wrapMode: TextEdit.WordWrap
            font.family: "DejaVu Sans Mono"
        }

        DoubleSpinBox {
            id: yawBox
            Layout.fillWidth: true
            decimals: 2
            realValue: 0.0
            realFrom: -180.0
            realTo: 180.0
            prefix: "Yaw: "
            suffix: " °"
        }

        RowLayout {
            Layout.fillWidth: true
            Button {
                text: "Help"
                Layout.preferredWidth: 50
                Layout.fillWidth: true
                flat: true
                onClicked: {
                    VescIf.emitMessageDialog(
                                mInfoConf.getLongName("help_imu_calibration"),
                                mInfoConf.getDescription("help_imu_calibration"),
                                true, true)
                }
            }

            Button {
                text: "Calibrate IMU"
                Layout.preferredWidth: 50
                Layout.fillWidth: true
                flat: true
                onClicked: {
                    if (!testConnected()) {
                        return
                    }

                    calState = 2
                    updateDisplay()
                    mCommands.getImuCalibration(yawBox.realValue)
                }
            }
        }

        Button {
            text: "Apply and Write"
            Layout.fillWidth: true
            flat: true
            onClicked: {
                applyMapping()
            }
        }
    }

    Connections {
        target: mCommands

        onImuCalibrationReceived: {
            calState = 3
            mCal = cal
            updateDisplay()
        }
    }
}
