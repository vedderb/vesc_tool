/*
* IMU Calibration Wizard
*/

import QtQuick 2.7
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3
//import QtGraphicalEffects 1.0
//import QtQuick.Controls.Material 2.2

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: topItem
    anchors.fill: parent

    property real pi: 3.14159265358

    property ConfigParams mAppConf: VescIf.appConfig()
    property Commands mCommands: VescIf.commands()
    property var dialogParent: ApplicationWindow.overlay

    property bool workingIMU: false
    property int imuType: 0
    property string imuString: "Unknown"
    property var filteredIMUValues: {
        'roll': 0.0,
        'pitch': 0.0,
        'yaw': 0.0,
        'accX': 0.0,
        'accY': 0.0,
        'accZ': 0.0,
        'gyroX': 0.0,
        'gyroY': 0.0,
        'gyroZ': 0.0,
        'gyroXNoise': 0.0,
        'gyroXPrevious': 0.0,
    }
    property var maxAccelValues: {
        'x': -10.0, 'y': -10.0, 'z': -10.0
    }
    property real calculatedYawOffset: 0.0

    property var pages: {
        'menu': 0,
        'gyro': 1,
        'accelerometerX': 2,
        'accelerometerY': 3,
        'accelerometerZ': 4,
        'orientationRoll': 5,
        'orientationPitch': 6,
        'orientationYaw': 7,
        'configurator': 8
    }

    function openDialog() {
        dialog.open()
        openTimer.start()
    }

    Timer {
        id: openTimer

        running: false
        repeat: true
        interval: 20

        onTriggered: {
            mCommands.getImuData(0x1FF)
        }
    }

    Timer {
        id: delayTimer
        repeat: false

    }
    function delay(delayTime, cb) {
        delayTimer.interval = delayTime
        delayTimer.triggered.connect(cb)
        delayTimer.start()
    }

    Connections {
        target: mCommands

        //        IMU_VALUES() {
        //            roll = 0; pitch = 0; yaw = 0;
        //            accX = 0; accY = 0; accZ = 0;
        //            gyroX = 0; gyroY = 0; gyroZ = 0;
        //            magX = 0; magY = 0; magZ = 0;
        //            q0 = 1; q1 = 0; q2 = 0; q3 = 0;
        //            vesc_id = 0;
        //        }
        onValuesImuReceived: {

            // Update values
            filteredIMUValues.roll = values.roll
            filteredIMUValues.pitch = values.pitch
            filteredIMUValues.yaw = values.yaw
            filteredIMUValues.accX = (filteredIMUValues.accX * .98) + (values.accX * .02)
            filteredIMUValues.accY = (filteredIMUValues.accY * .98) + (values.accY * .02)
            filteredIMUValues.accZ = (filteredIMUValues.accZ * .98) + (values.accZ * .02)
            filteredIMUValues.gyroX = (filteredIMUValues.gyroX * .99) + (values.gyroX * .01)
            filteredIMUValues.gyroY = (filteredIMUValues.gyroY * .99) + (values.gyroY * .01)
            filteredIMUValues.gyroZ = (filteredIMUValues.gyroZ * .99) + (values.gyroZ * .01)
            filteredIMUValues = filteredIMUValues // Spam UI refreshes lol

            // Update peak accel values
            maxAccelValues.x = Math.max(maxAccelValues.x, filteredIMUValues.accX)
            maxAccelValues.y = Math.max(maxAccelValues.y, filteredIMUValues.accY)
            maxAccelValues.z = Math.max(maxAccelValues.z, filteredIMUValues.accZ)
            maxAccelValues = maxAccelValues

            // Search for reccomended yaw offset
            var currentMaxPitch = getPitchForYawOffset(calculatedYawOffset)
            if(getPitchForYawOffset(calculatedYawOffset + 0.261799387798) > currentMaxPitch){
                calculatedYawOffset += 0.261799387798
            }else if (getPitchForYawOffset(calculatedYawOffset - 0.261799387798) > currentMaxPitch){
                calculatedYawOffset -= 0.261799387798
            }
            if(calculatedYawOffset > pi){
                calculatedYawOffset = -pi
            }else if(calculatedYawOffset < -pi){
                calculatedYawOffset = pi
            }

            // Detect working IMU based on noise levels of the gyro X axis
            var noise = Math.abs(values.gyroX - filteredIMUValues.gyroXPrevious)
            filteredIMUValues.gyroXPrevious = values.gyroX
            filteredIMUValues.gyroXNoise = (filteredIMUValues.gyroXNoise * .9) + (noise * .1)
            if(filteredIMUValues.gyroXNoise < 0.01){
                workingIMU = false
            } else {
                workingIMU = true
            }
        }
    }



    Dialog{
        id: dialog
        modal: true
        focus: true
        visible: false
        property bool isHorizontal: width > height
        width: parent.width - 10 - notchLeft - notchRight
        height: parent.height - 10 - notchBot - notchTop
        closePolicy: Popup.NoAutoClose
        x: 5 + notchLeft
        y: 5 + notchTop
        parent: dialogParent
        bottomMargin: 0
        rightMargin: 0
        padding: 10
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        StackLayout {
            width: parent.width
            height: parent.height
            id: stackLayout
            currentIndex: 0

            Item{// Welcome Screen #0
                width: parent.width
                height: parent.height

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    Text {
                        id: welcomeText1
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                        font.underline: true
                        font.pointSize: 20
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: parent.width
                        text: "IMU Wizard"
                    }// Text
                    Text {
                        id: welcomeText2
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        font.pointSize: 12
                        Layout.preferredWidth: parent.width
                        text: "Welcome to the IMU Wizard. This tool is split into tasks which help simplify IMU config and calibration. " +
                              "They are intented to be ran sequentially, but can be run as one offs and in any order."
                    }
                    Text {
                        id: welcomeText3
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        font.pointSize: 12
                        Layout.preferredWidth: parent.width
                        visible: !workingIMU
                        text: "We have detected that your IMU is <font color=\"#FF0000\">not working</font>. The IMU Configurator can help set it up."
                    }
                    Button {
                        id: configuratorButton
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        text: "IMU Configurator"
                        flat: false

                        onClicked: {
                            configuratorRadioDefault.checked = true
                            configuratorRadioDefault.onClicked()
                            updateIMUType()
                            stackLayout.currentIndex = pages.configurator
                        }
                    }
                    Button {
                        id: gyroCalibrationButton
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        text: "Gyroscope Calibration"
                        flat: false

                        onClicked: {
                            stackLayout.currentIndex = pages.gyro
                        }
                    }
                    Button {
                        id: accelerometerCalibrationButton
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        text: "Accelerometer Calibration"
                        flat: false

                        onClicked: {
                            maxAccelValues.x = -10
                            maxAccelValues.y = -10
                            maxAccelValues.z = -10
                            stackLayout.currentIndex = pages.accelerometerX
                        }
                    }
                    Button {
                        id: orientationCalibrationButton
                        Layout.fillWidth: true
                        Layout.preferredWidth: 500
                        text: "Orientation Calibration"
                        flat: false

                        onClicked: {
                            stackLayout.currentIndex = pages.orientationRoll
                        }
                    }
                }//Column Layout
            }//Item (Welcome Screen)

            Item{// Gyro Calibration #1
                width: parent.width
                height: parent.height

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    Text {
                        id: gyroText1
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                        font.underline: true
                        font.pointSize: 20
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: parent.width
                        text: "Gyroscope calibration"
                    }// Text
                    Text {
                        id: gyroText2
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        font.pointSize: 12
                        Layout.preferredWidth: parent.width
                        text: "To find the gyroscope offsets, " +
                        "the IMU must be left in a stable position without any vibration. " +
                        "However any orientation is acceptable.\n\n" +
                        "Wait for the offsets to stabilize then hit 'SAVE'."
                    }// Text
                    Text {
                        id: gyroText3
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text:"X Offset: " + filteredIMUValues.gyroX.toFixed(2) +
                        "\nY Offset: " + filteredIMUValues.gyroY.toFixed(2) +
                        "\nZ Offset: " + filteredIMUValues.gyroZ.toFixed(2)
                    }// Text
                }// Column Layout
            }// Item (Gyro Calibration)

            Item{// Accel X Calibration #2
                width: parent.width
                height: parent.height

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    Text {
                        id: accelXText1
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                        font.underline: true
                        font.pointSize: 20
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: parent.width
                        text: "Accel X calibration"
                    }// Text
                    Text {
                        id: accelXText2
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.pointSize: 12
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: "To find the accelerometer offsets, " +
                        "the IMU must be rotated to the max value for all 3 axes. " +
                        "Avoid sharp movements & bumping the IMU to keep the reading stable. " +
                        "If you've overshot the reading, hit the 'Clear Max' button to reset the reading. " +
                        "\n\nAfter finding a stable max X value, hit 'SAVE' to continue.\n"
                    }// Text
                    Text {
                        id: accelXText3
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: "X:\t" + filteredIMUValues.accX.toFixed(3) +
                        "\nMax X:\t" + maxAccelValues.x.toFixed(3)
                    }// Text
                    RowLayout {
                        width: parent.width
                        height: parent.height

                        Button {
                            id: clearAccelXButton
                            Layout.fillWidth: true
                            Layout.preferredWidth: 500
                            text: "Clear Max"
                            flat: true

                            onClicked: {
                                // Scale down filtered value so it looks like something happened
                                filteredIMUValues.accX = filteredIMUValues.accX * .9
                                maxAccelValues.x = -10.0
                            }
                        }// Button clear
                        Button {
                            id: skipAccelXButton
                            Layout.fillWidth: true
                            Layout.preferredWidth: 500
                            text: "Skip"
                            flat: true

                            onClicked: {
                                stackLayout.currentIndex = pages.accelerometerY
                            }
                        }// Button
                    }
                }// Column Layout
            }// Item (Accel X Calibration)

            Item{// Accel Y Calibration #3
                width: parent.width
                height: parent.height

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    Text {
                        id: accelYText1
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                        font.underline: true
                        font.pointSize: 20
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: parent.width
                        text: "Accel Y calibration"
                    }// Text
                    Text {
                        id: accelYText2
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.pointSize: 12
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: "To find the accelerometer offsets, " +
                        "the IMU must be rotated to the max value for all 3 axes. " +
                        "Avoid sharp movements & bumping the IMU to keep the reading stable. " +
                        "If you've overshot the reading, hit the 'Clear Max' button to reset the reading. " +
                        "\n\nAfter finding a stable max Y value, hit 'SAVE' to continue.\n"
                    }// Text
                    Text {
                        id: accelYText3
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: "Y:\t" + filteredIMUValues.accY.toFixed(3) +
                        "\nMax Y:\t" + maxAccelValues.y.toFixed(3)
                    }// Text
                    RowLayout {
                        width: parent.width
                        height: parent.height

                        Button {
                            id: clearAccelYButton
                            Layout.fillWidth: true
                            Layout.preferredWidth: 500
                            text: "Clear Max"
                            flat: true

                            onClicked: {
                                // Scale down filtered value so it looks like something happened
                                filteredIMUValues.accY = filteredIMUValues.accY * .9
                                maxAccelValues.y = -10.0
                            }
                        }// Button clear
                        Button {
                            id: skipAccelYButton
                            Layout.fillWidth: true
                            Layout.preferredWidth: 500
                            text: "Skip"
                            flat: true

                            onClicked: {
                                stackLayout.currentIndex = pages.accelerometerZ
                            }
                        }// Button
                    }
                }// Column Layout
            }// Item (Accel Y Calibration)

            Item{// Accel Z Calibration #4
                width: parent.width
                height: parent.height

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    Text {
                        id: accelZText1
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                        font.underline: true
                        font.pointSize: 20
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: parent.width
                        text: "Accel Z calibration"
                    }// Text
                    Text {
                        id: accelZText2
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.pointSize: 12
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: "To find the accelerometer offsets, " +
                        "the IMU must be rotated to the max value for all 3 axes. " +
                        "Avoid sharp movements & bumping the IMU to keep the reading stable. " +
                        "If you've overshot the reading, hit the 'Clear Max' button to reset the reading. " +
                        "\n\nAfter finding a stable max Z value, hit 'SAVE' to continue.\n"
                    }// Text
                    Text {
                        id: accelZText3
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: "Z:\t" + filteredIMUValues.accZ.toFixed(3) +
                        "\nMax Z:\t" + maxAccelValues.z.toFixed(3)
                    }// Text
                    RowLayout {
                        width: parent.width
                        height: parent.height

                        Button {
                            id: clearAccelZButton
                            Layout.fillWidth: true
                            Layout.preferredWidth: 500
                            text: "Clear Max"
                            flat: true

                            onClicked: {
                                // Scale down filtered value so it looks like something happened
                                filteredIMUValues.accZ = filteredIMUValues.accZ * .9
                                maxAccelValues.z = -10.0
                            }
                        }// Button clear
                        Button {
                            id: skipAccelZButton
                            Layout.fillWidth: true
                            Layout.preferredWidth: 500
                            text: "Skip"
                            flat: true

                            onClicked: {
                                stackLayout.currentIndex = pages.menu
                            }
                        }// Button
                    }
                }// Column Layout
            }// Item (Accel Z Calibration)

            Item{// roll Callibration #5
                width: parent.width
                height: parent.height

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    Text {
                        id: rollText1
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                        font.underline: true
                        font.pointSize: 20
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: parent.width
                        text: "Roll Angle Calibration"
                    }// Text
                    Text {
                        id: rollText2
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        font.pointSize: 12
                        Layout.preferredWidth: parent.width
                        text: "To calibrate the roll angle, " +
                        "place the IMU on a flat stable surface, " +
                        "with both the pitch & roll angles leveled out.\n\n" +
                        "Wait for the offset to stabilize then hit 'SAVE'."
                    }// Text
                    Text {
                        id: rollText3
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: "Roll Offset: " + (filteredIMUValues.roll * 180.0/pi).toFixed(2)
                    }// Text

                    Button {
                        id: skipRollButton
                        Layout.fillWidth: true
                        text: "Skip"
                        flat: true

                        onClicked: {
                            stackLayout.currentIndex = pages.orientationPitch
                        }
                    }// Button
                }// Column Layout
            }// Item (roll Calibration)

            Item{// pitch Callibration #6
                width: parent.width
                height: parent.height

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    Text {
                        id: pitchText1
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                        font.underline: true
                        font.pointSize: 20
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: parent.width
                        text: "Pitch Angle Calibration"
                    }// Text
                    Text {
                        id: pitchText2
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        font.pointSize: 12
                        Layout.preferredWidth: parent.width
                        text: "To calibrate the pitch angle, " +
                        "place the IMU on a flat stable surface, " +
                        "with both the pitch & roll angles leveled out.\n\n" +
                        "Wait for the offset to stabilize then hit 'SAVE'."
                    }// Text
                    Text {
                        id: pitchText3
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: "Pitch Offset: " + (filteredIMUValues.pitch * 180.0/pi).toFixed(2)
                    }// Text
                    Button {
                        id: skipPitchButton
                        Layout.fillWidth: true
                        text: "Skip"
                        flat: true

                        onClicked: {
                            stackLayout.currentIndex = pages.orientationYaw
                        }
                    }// Button
                }// Column Layout
            }// Item (pitch Calibration)

            Item{// yaw Callibration #7
                width: parent.width
                height: parent.height

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    Text {
                        id: yawText1
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                        font.underline: true
                        font.pointSize: 20
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: parent.width
                        text: "Yaw Angle Calibration"
                    }// Text
                    Text {
                        id: yawText2
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        font.pointSize: 12
                        Layout.preferredWidth: parent.width
                        text: "To calculate the yaw orientation, " +
                        "place the IMU on a stable surface, " +
                        "with roll angle leveled out, and the <b>pitch angle raised to roughly <font color=\"#FF0000\">45 degrees</font></b>.<br><br>" +
                        "Wait for the offset to stabilize then hit 'SAVE'."
                    }// Text
                    Text {
                        id: yawText3
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: "Yaw Offset: " + (calculatedYawOffset * 180.0/pi).toFixed(2)
                    }// Text
                    Button {
                        id: skipYawButton
                        Layout.fillWidth: true
                        text: "Skip"
                        flat: true

                        onClicked: {
                            stackLayout.currentIndex = pages.menu
                        }
                    }// Button
                }// Column Layout
            }// Item (yaw Calibration)

            Item{// configurator #8
                width: parent.width
                height: parent.height

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    Text {
                        id: configuratorText1
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                        font.underline: true
                        font.pointSize: 20
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: parent.width
                        text: "IMU Detector"
                    }// Text
                    Text {
                        id: configuratorText2
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: "IMU Detected: " + (workingIMU ? "<font color=\"#00FF00\">True</font>" : "<font color=\"#FF0000\">False</font>") + "<br>" +
                        "IMU Type: " + imuString
                    }// Text
                    Text {
                        id: configuratorText3
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        font.pointSize: 10
                        Layout.preferredWidth: parent.width
                        text: "After searching, IMU Type of \"Off\" indicates your IMU is not connected (properly). " +
                        "If IMU Detected is \"true\" but changes to \"false\" after searching, the configured app has a conflicting comm port use."
                    }// Text
                    Button {
                        id: searchIMUButton
                        Layout.fillWidth: true
                        text: "Search for IMU"
                        flat: false

                        onClicked: {
                            searchIMUButton.enabled = false
                            searchIMUButton.text = "Searching...."

                            var previousApp = mAppConf.getParamEnum("app_to_use")
                            mAppConf.updateParamEnum("app_to_use", 0)
                            mAppConf.updateParamEnum("imu_conf.type", 1)
                            mCommands.setAppConfNoStore()
                            updateIMUType()
                            delay(2000, function(){
                                delayTimer.stop()
                                if(!workingIMU){
                                    mAppConf.updateParamEnum("imu_conf.type", 2)
                                    mCommands.setAppConfNoStore()
                                    updateIMUType()
                                    delay(2000, function(){
                                        delayTimer.stop()
                                        if(!workingIMU){
                                            mAppConf.updateParamEnum("imu_conf.type", 3)
                                            mCommands.setAppConfNoStore()
                                            updateIMUType()
                                            delay(2000, function(){
                                                delayTimer.stop()
                                                if(!workingIMU){
                                                    mAppConf.updateParamEnum("imu_conf.type", 4)
                                                    mCommands.setAppConfNoStore()
                                                    updateIMUType()
                                                    delay(2000, function(){
                                                        delayTimer.stop()
                                                        if(!workingIMU){
                                                            mAppConf.updateParamEnum("imu_conf.type", 5)
                                                            mCommands.setAppConfNoStore()
                                                            updateIMUType()
                                                            delay(2000, function(){
                                                                delayTimer.stop()
                                                                if(!workingIMU){
                                                                    mAppConf.updateParamEnum("imu_conf.type", 0)
                                                                }
                                                                searchIMUButton.enabled = true
                                                                searchIMUButton.text = "Search for IMU"
                                                                mAppConf.updateParamEnum("app_to_use", previousApp)
                                                                mCommands.setAppConfNoStore()
                                                                updateIMUType()
                                                            })
                                                        }else{
                                                            searchIMUButton.enabled = true
                                                            searchIMUButton.text = "Search for IMU"
                                                            mAppConf.updateParamEnum("app_to_use", previousApp)
                                                            mCommands.setAppConfNoStore()
                                                        }
                                                    })
                                                }else{
                                                    searchIMUButton.enabled = true
                                                    searchIMUButton.text = "Search for IMU"
                                                    mAppConf.updateParamEnum("app_to_use", previousApp)
                                                    mCommands.setAppConfNoStore()
                                                }
                                            })
                                        }else{
                                            searchIMUButton.enabled = true
                                            searchIMUButton.text = "Search for IMU"
                                            mAppConf.updateParamEnum("app_to_use", previousApp)
                                            mCommands.setAppConfNoStore()
                                        }
                                    })
                                }else{
                                    searchIMUButton.enabled = true
                                    searchIMUButton.text = "Search for IMU"
                                    mAppConf.updateParamEnum("app_to_use", previousApp)
                                    mCommands.setAppConfNoStore()
                                }
                            })
                        }
                    }// Button
                    ToolSeparator {
                        Layout.preferredWidth: parent.width
                        orientation: Qt.Horizontal
                    }
                    Text {
                        id: configuratorText4
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                        font.underline: true
                        font.pointSize: 20
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: parent.width
                        text: "IMU Profile Selector"
                    }// Text
                    GridLayout {
                        columns: 2
                        RadioButton {
                            id: configuratorRadioDefault
                            text: "Default"
                            onClicked: {
                                mAppConf.updateParamInt("imu_conf.sample_rate_hz", 200)
                                mAppConf.updateParamEnum("imu_conf.mode", 0)
                                mAppConf.updateParamDouble("imu_conf.accel_confidence_decay", 1.0)
                                mAppConf.updateParamDouble("imu_conf.mahony_kp", 0.03)
                                mAppConf.updateParamDouble("imu_conf.accel_lowpass_filter_z", 0.0)
                                configuratorText5.text = configuratorText5.getText()
                            }
                        }
                        RadioButton {
                            id: configuratorRadioLogs
                            text: "Logs"
                            onClicked: {
                                mAppConf.updateParamInt("imu_conf.sample_rate_hz", 200)
                                mAppConf.updateParamEnum("imu_conf.mode", 1)
                                mAppConf.updateParamDouble("imu_conf.accel_confidence_decay", 1.0)
                                mAppConf.updateParamDouble("imu_conf.mahony_kp", 0.03)
                                mAppConf.updateParamDouble("imu_conf.accel_lowpass_filter_z", 0.0)
                                configuratorText5.text = configuratorText5.getText()
                            }
                        }
                        RadioButton {
                            id: configuratorRadioBUni
                            text: "Balance Unicycle"
                            onClicked: {
                                if(imuType == 4){
                                    mAppConf.updateParamInt("imu_conf.sample_rate_hz", 800)
                                }else if(imuType == 5){
                                    mAppConf.updateParamInt("imu_conf.sample_rate_hz", 833)
                                }else{
                                    mAppConf.updateParamInt("imu_conf.sample_rate_hz", 1000)
                                }
                                mAppConf.updateParamEnum("imu_conf.mode", 1)
                                mAppConf.updateParamDouble("imu_conf.accel_confidence_decay", 1.0)
                                mAppConf.updateParamDouble("imu_conf.mahony_kp", 0.02)
                                mAppConf.updateParamDouble("imu_conf.accel_lowpass_filter_z", 0.0)
                                configuratorText5.text = configuratorText5.getText()
                            }
                        }
                        RadioButton {
                            id: configuratorRadioBSkate
                            text: "Balance Skateboard"
                            onClicked: {
                                if(imuType == 4){
                                    mAppConf.updateParamInt("imu_conf.sample_rate_hz", 400)
                                }else if(imuType == 5){
                                    mAppConf.updateParamInt("imu_conf.sample_rate_hz", 416)
                                }else{
                                    mAppConf.updateParamInt("imu_conf.sample_rate_hz", 1000)
                                }
                                mAppConf.updateParamEnum("imu_conf.mode", 1)
                                mAppConf.updateParamDouble("imu_conf.accel_confidence_decay", 0.02)
                                mAppConf.updateParamDouble("imu_conf.mahony_kp", 2.0)
                                mAppConf.updateParamDouble("imu_conf.accel_lowpass_filter_z", 1.0)
                                configuratorText5.text = configuratorText5.getText()
                            }
                        }
                    }
                    Text {
                        function getText(){
                            return "Frequency: " + mAppConf.getParamInt("imu_conf.sample_rate_hz") + "\n" +
                                "AHRS: " + getAHRS() + "\n" +
                                "Acc Decay: " + mAppConf.getParamDouble("imu_conf.accel_confidence_decay").toFixed(3) + "\n" +
                                "Mahony kp: " + mAppConf.getParamDouble("imu_conf.mahony_kp").toFixed(3) + "\n" +
                                "Accel Z Filter: " + mAppConf.getParamDouble("imu_conf.accel_lowpass_filter_z").toFixed(1)
                        }

                        id: configuratorText5
                        color: Utility.getAppHexColor("lightText")
                        font.family: "DejaVu Sans Mono"
                        wrapMode: Text.Wrap
                        Layout.preferredWidth: parent.width
                        text: getText()
                    }// Text



                }// Column Layout
            }// Item (configurator)

        }

//        header: Rectangle {
//            color: {
//                color = Utility.getAppHexColor("lightText")
//            }
//            height: tabBar.implicitHeight
//            width: parent.width
//        }//Header

        footer: RowLayout {
            spacing: 0
            Button {
                id: prevButton
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: "Cancel"
                flat: true

                onClicked: {
                    if (stackLayout.currentIndex === pages.menu) {
                        dialog.close()
                        openTimer.stop()
                    } else if(stackLayout.currentIndex === pages.configurator){
                        mCommands.getAppConf()
                        mCommands.setAppConf()
                        stackLayout.currentIndex = pages.menu
                    } else {
                        stackLayout.currentIndex = pages.menu
                    }
                }
            }

            Button {
                id: nextButton
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: "Save"
                flat: true
                visible: (stackLayout.currentIndex != 0) ? true : false

                onClicked: {
                    if(stackLayout.currentIndex === pages.gyro){
                        // Save Gyro Offsets
                        mAppConf.updateParamDouble(
                        "imu_conf.gyro_offsets__0",
                        (mAppConf.getParamDouble("imu_conf.gyro_offsets__0") + filteredIMUValues.gyroX),
                        null
                        )
                        mAppConf.updateParamDouble(
                        "imu_conf.gyro_offsets__1",
                        (mAppConf.getParamDouble("imu_conf.gyro_offsets__1") + filteredIMUValues.gyroY),
                        null
                        )
                        mAppConf.updateParamDouble(
                        "imu_conf.gyro_offsets__2",
                        (mAppConf.getParamDouble("imu_conf.gyro_offsets__2") + filteredIMUValues.gyroZ),
                        null
                        )

                        mCommands.setAppConf()
                        stackLayout.currentIndex = pages.menu

                    }else if(stackLayout.currentIndex === pages.accelerometerX){
                        // Save Accelerometer Offset
                        mAppConf.updateParamDouble(
                        "imu_conf.accel_offsets__0",
                        (mAppConf.getParamDouble("imu_conf.accel_offsets__0") + maxAccelValues.x -1),
                        null
                        )
                        mCommands.setAppConf()
                        stackLayout.currentIndex = pages.accelerometerY
                    }else if(stackLayout.currentIndex === pages.accelerometerY){
                        // Save Accelerometer Offset
                        mAppConf.updateParamDouble(
                        "imu_conf.accel_offsets__1",
                        (mAppConf.getParamDouble("imu_conf.accel_offsets__1") + maxAccelValues.y -1),
                        null
                        )
                        mCommands.setAppConf()
                        stackLayout.currentIndex = pages.accelerometerZ
                    }else if(stackLayout.currentIndex === pages.accelerometerZ){
                        // Save Accelerometer Offset
                        mAppConf.updateParamDouble(
                        "imu_conf.accel_offsets__2",
                        (mAppConf.getParamDouble("imu_conf.accel_offsets__2") + maxAccelValues.z -1),
                        null
                        )
                        mCommands.setAppConf()
                        stackLayout.currentIndex = pages.menu
                    }else if(stackLayout.currentIndex === pages.orientationRoll){
                        var roll = -filteredIMUValues.roll
                        mAppConf.updateParamDouble(
                        "imu_conf.rot_roll",
                        (mAppConf.getParamDouble("imu_conf.rot_roll") + (roll * 180.0/pi)),
                        null
                        )
                        var yawRotation = mAppConf.getParamDouble("imu_conf.rot_yaw") * pi/180
                        var pitchRotation = mAppConf.getParamDouble("imu_conf.rot_pitch") * pi/180
                        applyRotationToCalibrationConfig(0, 0, -yawRotation)
                        applyRotationToCalibrationConfig(0, -pitchRotation, 0)
                        applyRotationToCalibrationConfig(roll, 0, 0)
                        applyRotationToCalibrationConfig(0, pitchRotation, 0)
                        applyRotationToCalibrationConfig(0, 0, yawRotation)

                        mCommands.setAppConf()
                        stackLayout.currentIndex = pages.orientationPitch
                    }else if(stackLayout.currentIndex === pages.orientationPitch){
                        var pitch = filteredIMUValues.pitch
                        mAppConf.updateParamDouble(
                        "imu_conf.rot_pitch",
                        (mAppConf.getParamDouble("imu_conf.rot_pitch") + (pitch * 180.0/pi)),
                        null
                        )

                        var yawRotation = mAppConf.getParamDouble("imu_conf.rot_yaw") * pi/180
                        applyRotationToCalibrationConfig(0, 0, -yawRotation)
                        applyRotationToCalibrationConfig(0, pitch, 0)
                        applyRotationToCalibrationConfig(0, 0, yawRotation)

                        mCommands.setAppConf()
                        stackLayout.currentIndex = pages.orientationYaw
                    }else if(stackLayout.currentIndex === pages.orientationYaw){
                        var yaw = calculatedYawOffset
                        mAppConf.updateParamDouble(
                        "imu_conf.rot_yaw",
                        (mAppConf.getParamDouble("imu_conf.rot_yaw") + (yaw * 180.0/pi)),
                        null
                        )
                        applyRotationToCalibrationConfig(0, 0, yaw)

                        mCommands.setAppConf()
                        stackLayout.currentIndex = pages.menu
                    }else if(stackLayout.currentIndex === pages.configurator){
                        mCommands.setAppConf()
                        stackLayout.currentIndex = pages.menu
                    }else{
                        stackLayout.currentIndex = pages.menu
                    }
                    // updateButtonText()
                }
            }
        }//Footer


    }//Dialog

    function applyRotationToCalibrationConfig(roll, pitch, yaw){
        // Calculate sin & cos
        var s1, c1, s2, c2, s3, c3

        if (yaw !== 0.0) {
            s1 = Math.sin(yaw)
            c1 = Math.cos(yaw)
        } else {
            s1 = 0.0
            c1 = 1.0
        }

        if (pitch !== 0.0) {
            s2 = Math.sin(pitch)
            c2 = Math.cos(pitch)
        } else {
            s2 = 0.0
            c2 = 1.0
        }

        if (roll !== 0.0) {
            s3 = Math.sin(roll)
            c3 = Math.cos(roll)
        } else {
            s3 = 0.0
            c3 = 1.0
        }

        // Create rotation matrix
        var m11 = c1 * c2
        var m12 = c1 * s2 * s3 - c3 * s1
        var m13 = s1 * s3 + c1 * c3 * s2

        var m21 = c2 * s1
        var m22 = c1 * c3 + s1 * s2 * s3
        var m23 = c3 * s1 * s2 - c1 * s3

        var m31 = -s2
        var m32 = c2 * s3
        var m33 = c2 * c3

        // Read current calibration
        var offsetGyroX = mAppConf.getParamDouble("imu_conf.gyro_offsets__0")
        var offsetGyroY = mAppConf.getParamDouble("imu_conf.gyro_offsets__1")
        var offsetGyroZ = mAppConf.getParamDouble("imu_conf.gyro_offsets__2")

        var offsetAccelX = mAppConf.getParamDouble("imu_conf.accel_offsets__0")
        var offsetAccelY = mAppConf.getParamDouble("imu_conf.accel_offsets__1")
        var offsetAccelZ = mAppConf.getParamDouble("imu_conf.accel_offsets__2")

        // Calculate rotated calibration
        var rotatedOffsetGyroX = offsetGyroX * m11 + offsetGyroY * m12 + offsetGyroZ * m13;
        var rotatedOffsetGyroY = offsetGyroX * m21 + offsetGyroY * m22 + offsetGyroZ * m23;
        var rotatedOffsetGyroZ = offsetGyroX * m31 + offsetGyroY * m32 + offsetGyroZ * m33;

        var rotatedOffsetAccelX = offsetAccelX * m11 + offsetAccelY * m12 + offsetAccelZ * m13;
        var rotatedOffsetAccelY = offsetAccelX * m21 + offsetAccelY * m22 + offsetAccelZ * m23;
        var rotatedOffsetAccelZ = offsetAccelX * m31 + offsetAccelY * m32 + offsetAccelZ * m33;

        // Update settings
        mAppConf.updateParamDouble("imu_conf.gyro_offsets__0", rotatedOffsetGyroX, null)
        mAppConf.updateParamDouble("imu_conf.gyro_offsets__1", rotatedOffsetGyroY, null)
        mAppConf.updateParamDouble("imu_conf.gyro_offsets__2", rotatedOffsetGyroZ, null)

        mAppConf.updateParamDouble("imu_conf.accel_offsets__0", rotatedOffsetAccelX, null)
        mAppConf.updateParamDouble("imu_conf.accel_offsets__1", rotatedOffsetAccelY, null)
        mAppConf.updateParamDouble("imu_conf.accel_offsets__2", rotatedOffsetAccelZ, null)
    }

    function getPitchForYawOffset(yaw){
        var roll = 0
        var pitch = 0

        // Calculate sin & cos
        var s1, c1, s2, c2, s3, c3

        if (yaw !== 0.0) {
            s1 = Math.sin(yaw)
            c1 = Math.cos(yaw)
        } else {
            s1 = 0.0
            c1 = 1.0
        }

        if (pitch !== 0.0) {
            s2 = Math.sin(pitch)
            c2 = Math.cos(pitch)
        } else {
            s2 = 0.0
            c2 = 1.0
        }

        if (roll !== 0.0) {
            s3 = Math.sin(roll)
            c3 = Math.cos(roll)
        } else {
            s3 = 0.0
            c3 = 1.0
        }

        // Create rotation matrix
        var m11 = c1 * c2
        var m12 = c1 * s2 * s3 - c3 * s1
        var m13 = s1 * s3 + c1 * c3 * s2

        var m21 = c2 * s1
        var m22 = c1 * c3 + s1 * s2 * s3
        var m23 = c3 * s1 * s2 - c1 * s3

        var m31 = -s2
        var m32 = c2 * s3
        var m33 = c2 * c3

        // Apply rotation to calibrations
        var rotatedRoll  = filteredIMUValues.roll * m11 + filteredIMUValues.pitch * m12 + filteredIMUValues.yaw * m13;
        var rotatedPitch = filteredIMUValues.roll * m21 + filteredIMUValues.pitch * m22 + filteredIMUValues.yaw * m23;
        var rotatedYaw   = filteredIMUValues.roll * m31 + filteredIMUValues.pitch * m32 + filteredIMUValues.yaw * m33;

        return rotatedPitch
    }

    function updateIMUType(){
        imuType = mAppConf.getParamEnum("imu_conf.type")
        if(imuType === 0){ //off
            imuString = "Off"
        }else if(imuType === 1){ //internal
            var internalTypeString = Utility.readInternalImuType(VescIf)
            imuString = "Internal " + internalTypeString
            if(internalTypeString === "MPU6050"){
                imuType = 2
            }else if(internalTypeString === "ICM20948"){
                imuType = 3
            }else if(internalTypeString === "BMI160"){
                imuType = 4
            }else if(internalTypeString === "LSM6DS3"){
                imuType = 5
            }
        }else if(imuType === 2){ //MPU6050
            imuString = "MPU6050"
        }else if(imuType === 3){ //ICM20948
            imuString = "ICM20948"
        }else if(imuType === 4){ //BMI160
            imuString = "BMI160"
        }else if(imuType === 5){ //LSM6DS2(tr-c?)
            imuString = "LSM6DS3"
        }
    }

    function getAHRS(){
        var imuMode = mAppConf.getParamEnum("imu_conf.mode")
        if(imuMode === 0){
            return "Madgwick"
        }else if(imuMode === 1){
            return "Mahony"
        }else if(imuMode === 2){
            return "Madgwick Fusion"
        }else if(imuMode === 3){
            return "Unknown"
        }
    }

}//Item

