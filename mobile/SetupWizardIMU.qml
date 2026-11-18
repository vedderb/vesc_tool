/*
* IMU Calibration Wizard
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt5Compat.GraphicalEffects

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0
import Vedder.vesc.vesc3ditem 1.0

Item {
    id: topItem
    anchors.fill: parent

    property real pi: 3.14159265358

    property ConfigParams mAppConf: VescIf.appConfig()
    property Commands mCommands: VescIf.commands()
    property var dialogParent: Overlay.overlay

    property bool workingIMU: false
    property int imuType: 0
    property string imuString: "Unknown"
    property var configuratorRestore: ({})
    property var orientationRestore: ({})
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
    readonly property string accelHint: "Avoid sharp movements and bumps. If the reading overshoots, tap Clear Max. Save once the max is stable."

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
        function onValuesImuReceived(values) {

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

            // Update peak accel values
            maxAccelValues.x = Math.max(maxAccelValues.x, filteredIMUValues.accX)
            maxAccelValues.y = Math.max(maxAccelValues.y, filteredIMUValues.accY)
            maxAccelValues.z = Math.max(maxAccelValues.z, filteredIMUValues.accZ)

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

    // The values above update at the 50 Hz poll rate, the pages only need to redraw at 10 Hz
    Timer {
        interval: 100
        repeat: true
        running: dialog.visible

        onTriggered: {
            filteredIMUValuesChanged()
            maxAccelValuesChanged()
        }
    }

    Dialog{
        id: dialog
        modal: true
        focus: true
        visible: false
        property bool isHorizontal: width > height
        // Covers the app bars on landscape phones
        readonly property bool fullWindow: phoneLandscape
        width: parent.width - 10 - notchLeft - notchRight
        height: parent.height - 10 - (fullWindow ? notchTop + notchBot : 0)
        closePolicy: Popup.NoAutoClose
        x: 5 + notchLeft
        y: 5 + (fullWindow ? notchTop : 0)
        parent: fullWindow ? Overlay.overlay : dialogParent
        bottomMargin: 0
        rightMargin: 0
        padding: 16
        Overlay.modal: Rectangle {
            color: Utility.getAppHexColor("overlay")
        }

        background: Rectangle {
            color: Utility.getAppHexColor("darkBackground")
            radius: 20
        }

        // The steps scroll when they do not fit
        Flickable {
            id: stepFlick
            width: mainIsTablet && dialog.isHorizontal ? (parent.width - 12) / 2 : parent.width
            height: mainIsTablet && !dialog.isHorizontal ? (parent.height - 12) / 2 : parent.height
            contentWidth: width
            contentHeight: Math.max(height, stackLayout.children[stackLayout.currentIndex].children[0].implicitHeight)
            boundsBehavior: Flickable.StopAtBounds
            clip: true
        }

        StackLayout {
            parent: stepFlick.contentItem
            width: parent.width
            height: parent.height
            id: stackLayout
            currentIndex: 0
            onCurrentIndexChanged: stepFlick.contentY = 0

            Item{// Welcome Screen #0
                width: parent.width
                height: parent.height

                // The tasks as a 2x2 grid on landscape phones
                GridLayout {
                    width: parent.width
                    height: parent.height
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 10
                    uniformCellWidths: true

                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "IMU Wizard"
                        instruction: "Run the tasks in order the first time"
                        hint: "Each task can also be run on its own later, in any order."
                    }
                    WarningBanner {
                        id: welcomeText3
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        Layout.fillWidth: true
                        visible: !workingIMU
                        text: "We have detected that your IMU is not working. The IMU Configurator can help set it up."
                    }
                    ImageButton {
                        id: configuratorButton
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        buttonText: "1. IMU Configurator"
                        hintText: "Find the IMU and choose a profile"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Search-96.png"
                        badge: !workingIMU

                        onClicked: {
                            configuratorRestore = {
                                "type": mAppConf.getParamEnum("imu_conf.type"),
                                "sample_rate_hz": mAppConf.getParamInt("imu_conf.sample_rate_hz"),
                                "mode": mAppConf.getParamEnum("imu_conf.mode"),
                                "accel_confidence_decay": mAppConf.getParamDouble("imu_conf.accel_confidence_decay"),
                                "mahony_kp": mAppConf.getParamDouble("imu_conf.mahony_kp"),
                                "accel_lowpass_filter_z": mAppConf.getParamDouble("imu_conf.accel_lowpass_filter_z"),
                                "gyro_lowpass_filter": mAppConf.getParamDouble("imu_conf.gyro_lowpass_filter")
                            }
                            configuratorRadioDefault.checked = true
                            configuratorRadioDefault.onClicked()
                            updateIMUType()
                            stackLayout.currentIndex = pages.configurator
                        }
                    }
                    ImageButton {
                        id: gyroCalibrationButton
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        buttonText: "2. Gyroscope Calibration"
                        hintText: "Measure the gyro offsets at rest"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Gyroscope-96.png"

                        onClicked: {
                            stackLayout.currentIndex = pages.gyro
                        }
                    }
                    ImageButton {
                        id: accelerometerCalibrationButton
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        buttonText: "3. Accelerometer Calibration"
                        hintText: "Find the peak reading on each axis"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Offset-96.png"

                        onClicked: {
                            maxAccelValues.x = -10
                            maxAccelValues.y = -10
                            maxAccelValues.z = -10
                            stackLayout.currentIndex = pages.accelerometerX
                        }
                    }
                    ImageButton {
                        id: orientationCalibrationButton
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        buttonText: "4. Orientation Calibration"
                        hintText: "Level roll and pitch, then set yaw"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/icons8-target-96.png"

                        onClicked: {
                            orientationRestore = {
                                "rot_roll": mAppConf.getParamDouble("imu_conf.rot_roll"),
                                "rot_pitch": mAppConf.getParamDouble("imu_conf.rot_pitch"),
                                "rot_yaw": mAppConf.getParamDouble("imu_conf.rot_yaw"),
                                "gyro_offsets__0": mAppConf.getParamDouble("imu_conf.gyro_offsets__0"),
                                "gyro_offsets__1": mAppConf.getParamDouble("imu_conf.gyro_offsets__1"),
                                "gyro_offsets__2": mAppConf.getParamDouble("imu_conf.gyro_offsets__2"),
                                "accel_offsets__0": mAppConf.getParamDouble("imu_conf.accel_offsets__0"),
                                "accel_offsets__1": mAppConf.getParamDouble("imu_conf.accel_offsets__1"),
                                "accel_offsets__2": mAppConf.getParamDouble("imu_conf.accel_offsets__2"),
                            }
                            mAppConf.updateParamDouble("imu_conf.rot_roll", 0, null)
                            mAppConf.updateParamDouble("imu_conf.rot_pitch", 0, null)
                            mAppConf.updateParamDouble("imu_conf.rot_yaw", 0, null)
                            applyRotationToCalibrationConfig(0, 0, -orientationRestore.rot_yaw * pi/180)
                            applyRotationToCalibrationConfig(0, -orientationRestore.rot_pitch * pi/180, 0)
                            applyRotationToCalibrationConfig(-orientationRestore.rot_roll * pi/180, 0, 0)
                            mCommands.setAppConfNoStore()
                            stackLayout.currentIndex = pages.orientationRoll
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }//Grid Layout
            }//Item (Welcome Screen)

            Item{// Gyro Calibration #1
                width: parent.width
                height: parent.height

                ColumnLayout {
                    width: parent.width
                    spacing: 10
                    TaskIntro {
                        title: "Gyroscope Calibration"
                        instruction: "Leave the IMU completely still"
                        hint: "Any orientation works, but avoid vibration. Save once the offsets have settled."
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: 8
                        uniformCellWidths: true

                        ValueTile {
                            Layout.fillWidth: true
                            title: "X offset"
                            value: filteredIMUValues.gyroX.toFixed(2)
                            unit: "\u00B0/s"
                        }
                        ValueTile {
                            Layout.fillWidth: true
                            title: "Y offset"
                            value: filteredIMUValues.gyroY.toFixed(2)
                            unit: "\u00B0/s"
                        }
                        ValueTile {
                            Layout.fillWidth: true
                            title: "Z offset"
                            value: filteredIMUValues.gyroZ.toFixed(2)
                            unit: "\u00B0/s"
                        }
                    }
                    RowLayout {
                        spacing: 8

                        Rectangle {
                            implicitWidth: 10
                            implicitHeight: 10
                            radius: 5
                            color: Utility.getAppHexColor(gyroSettle.stable ? "positiveText" : "warningText")
                        }
                        Text {
                            text: !workingIMU ? "No IMU data, check the IMU Configurator"
                                  : gyroSettle.stable ? "Settled, ready to save" : "Settling, keep the IMU still..."
                            color: Utility.getAppHexColor(gyroSettle.stable ? "positiveText" : "warningText")
                        }
                    }
                }// Column Layout

                // Settled when no offset moved more than 0.02 deg/s in the last second. The filter only removes
                // ~40% of the remaining error per second, so a looser limit turns green long before convergence.
                Timer {
                    id: gyroSettle
                    property bool stable: false
                    property var last: [NaN, NaN, NaN]

                    interval: 1000
                    repeat: true
                    running: dialog.visible && stackLayout.currentIndex === pages.gyro
                    onRunningChanged: {
                        gyroSettle.stable = false
                        gyroSettle.last = [NaN, NaN, NaN]
                    }
                    onTriggered: {
                        var now = [filteredIMUValues.gyroX, filteredIMUValues.gyroY, filteredIMUValues.gyroZ]
                        gyroSettle.stable = workingIMU && now.every(function(v, i) { return Math.abs(v - gyroSettle.last[i]) < 0.02 })
                        gyroSettle.last = now
                    }
                }
            }// Item (Gyro Calibration)

            Item{// Accel X Calibration #2
                width: parent.width
                height: parent.height

                // Clear Max and Skip beside the tiles on landscape phones
                GridLayout {
                    width: parent.width
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 8
                    uniformCellWidths: true
                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "Accel X Calibration"
                        step: "Step 1 of 3"
                        instruction: "Slowly tilt the IMU until Max X stops rising"
                        hint: accelHint
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8
                        uniformCellWidths: true

                        ValueTile {
                            Layout.fillWidth: true
                            title: "Accel X"
                            value: filteredIMUValues.accX.toFixed(3)
                            unit: "g"
                            level: filteredIMUValues.accX
                        }
                        ValueTile {
                            Layout.fillWidth: true
                            title: "Max X"
                            value: maxAccelValues.x.toFixed(3)
                            unit: "g"
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true

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
                }// Grid Layout
            }// Item (Accel X Calibration)

            Item{// Accel Y Calibration #3
                width: parent.width
                height: parent.height

                // Clear Max and Skip beside the tiles on landscape phones
                GridLayout {
                    width: parent.width
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 8
                    uniformCellWidths: true
                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "Accel Y Calibration"
                        step: "Step 2 of 3"
                        instruction: "Slowly tilt the IMU until Max Y stops rising"
                        hint: accelHint
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8
                        uniformCellWidths: true

                        ValueTile {
                            Layout.fillWidth: true
                            title: "Accel Y"
                            value: filteredIMUValues.accY.toFixed(3)
                            unit: "g"
                            level: filteredIMUValues.accY
                        }
                        ValueTile {
                            Layout.fillWidth: true
                            title: "Max Y"
                            value: maxAccelValues.y.toFixed(3)
                            unit: "g"
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true

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
                }// Grid Layout
            }// Item (Accel Y Calibration)

            Item{// Accel Z Calibration #4
                width: parent.width
                height: parent.height

                // Clear Max and Skip beside the tiles on landscape phones
                GridLayout {
                    width: parent.width
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 8
                    uniformCellWidths: true
                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "Accel Z Calibration"
                        step: "Step 3 of 3"
                        instruction: "Slowly tilt the IMU until Max Z stops rising"
                        hint: accelHint
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8
                        uniformCellWidths: true

                        ValueTile {
                            Layout.fillWidth: true
                            title: "Accel Z"
                            value: filteredIMUValues.accZ.toFixed(3)
                            unit: "g"
                            level: filteredIMUValues.accZ
                        }
                        ValueTile {
                            Layout.fillWidth: true
                            title: "Max Z"
                            value: maxAccelValues.z.toFixed(3)
                            unit: "g"
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true

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
                }// Grid Layout
            }// Item (Accel Z Calibration)

            Item{// roll Callibration #5
                width: parent.width
                height: parent.height

                // Skip beside the offset on landscape phones
                GridLayout {
                    width: parent.width
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 8
                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "Roll Angle Calibration"
                        step: "Step 1 of 3"
                        instruction: "Place the IMU level on a flat, stable surface"
                        hint: "Both pitch and roll should be level. Save once the offset has settled."
                    }
                    ValueTile {
                        id: rollText3
                        Layout.fillWidth: true
                        title: "Roll offset"
                        value: ((-filteredIMUValues.roll * 180.0/pi) - orientationRestore.rot_roll).toFixed(2)
                        unit: "\u00B0"
                    }

                    Button {
                        id: skipRollButton
                        Layout.fillWidth: !phoneLandscape
                        text: "Skip"
                        flat: true

                        onClicked: {
                            applyRollOffset(orientationRestore.rot_roll * pi/180)
                            stackLayout.currentIndex = pages.orientationPitch
                        }
                    }// Button
                }// Grid Layout
            }// Item (roll Calibration)

            Item{// pitch Callibration #6
                width: parent.width
                height: parent.height

                // Skip beside the offset on landscape phones
                GridLayout {
                    width: parent.width
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 8
                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "Pitch Angle Calibration"
                        step: "Step 2 of 3"
                        instruction: "Keep the IMU level on a flat, stable surface"
                        hint: "Both pitch and roll should be level. Save once the offset has settled."
                    }
                    ValueTile {
                        id: pitchText3
                        Layout.fillWidth: true
                        title: "Pitch offset"
                        value: ((filteredIMUValues.pitch * 180.0/pi) - orientationRestore.rot_pitch).toFixed(2)
                        unit: "\u00B0"
                    }
                    Button {
                        id: skipPitchButton
                        Layout.fillWidth: !phoneLandscape
                        text: "Skip"
                        flat: true

                        onClicked: {
                            applyPitchOffset(orientationRestore.rot_pitch * pi/180)
                            stackLayout.currentIndex = pages.orientationYaw
                        }
                    }// Button
                }// Grid Layout
            }// Item (pitch Calibration)

            Item{// yaw Callibration #7
                width: parent.width
                height: parent.height

                // Skip beside the offset on landscape phones
                GridLayout {
                    width: parent.width
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 8
                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "Yaw Angle Calibration"
                        step: "Step 3 of 3"
                        instruction: "Raise the pitch to roughly <font color=\"" + Utility.getAppHexColor("warningText") + "\">45 degrees</font>, roll still level"
                        hint: "Keep the IMU stable. Save once the offset has settled."
                    }
                    ValueTile {
                        id: yawText3
                        Layout.fillWidth: true
                        title: "Yaw offset"
                        value: ((-calculatedYawOffset * 180.0/pi) - orientationRestore.rot_yaw).toFixed(2)
                        unit: "\u00B0"
                    }
                    Button {
                        id: skipYawButton
                        Layout.fillWidth: !phoneLandscape
                        text: "Skip"
                        flat: true

                        onClicked: {
                            applyYawOffset(orientationRestore.rot_yaw * pi/180)
                            stackLayout.currentIndex = pages.menu
                        }
                    }// Button
                }// Grid Layout
            }// Item (yaw Calibration)

            Item{// configurator #8
                width: parent.width
                height: parent.height

                // Detection on the left and the profile on the right on landscape phones
                GridLayout {
                    width: parent.width
                    flow: GridLayout.TopToBottom
                    rows: phoneLandscape ? 5 : -1
                    rowSpacing: 10
                    columnSpacing: 16
                    uniformCellWidths: true
                    TaskIntro {
                        title: "IMU Configurator"
                        instruction: "Search for the IMU, then pick a profile"
                        hint: "Save writes the IMU type and profile, Cancel restores the previous settings."
                    }
                    SectionHeader {
                        Layout.topMargin: 0
                        text: "Detection"
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8
                        uniformCellWidths: true

                        ValueTile {
                            Layout.fillWidth: true
                            title: "IMU detected"
                            value: workingIMU ? "True" : "False"
                            valueColor: Utility.getAppHexColor(workingIMU ? "positiveText" : "dangerText")
                        }
                        ValueTile {
                            Layout.fillWidth: true
                            title: "IMU type"
                            value: imuString
                        }
                    }
                    Text {
                        id: configuratorText3
                        color: Utility.getAppHexColor("normalText")
                        wrapMode: Text.Wrap
                        font.pointSize: 11
                        Layout.fillWidth: true
                        text: "After searching, IMU Type of \"Off\" indicates your IMU is not connected (properly). " +
                        "If IMU Detected is \"true\" but changes to \"false\" after searching, the configured app has a conflicting comm port use."
                    }// Text
                    Button {
                        id: searchIMUButton
                        Layout.fillWidth: true
                        text: "Search for IMU"
                        flat: false
                        highlighted: !workingIMU

                        function onIMUSearchCompleted(previousApp) {
                            searchIMUButton.enabled = true
                            searchIMUButton.text = "Search for IMU"
                            mAppConf.updateParamEnum("app_to_use", previousApp)
                            mCommands.setAppConfNoStore()
                            updateIMUType()
                            if(configuratorRadioDefault.checked){
                                configuratorRadioDefault.onClicked()
                            }else if(configuratorRadioLogs.checked){
                                configuratorRadioLogs.onClicked()
                            }else if(configuratorRadioBUni.checked){
                                configuratorRadioBUni.onClicked()
                            }else if(configuratorRadioBSkate.checked){
                                configuratorRadioBSkate.onClicked()
                            }
                        }

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
                                                                onIMUSearchCompleted(previousApp)
                                                            })
                                                        }else{
                                                            onIMUSearchCompleted(previousApp)
                                                        }
                                                    })
                                                }else{
                                                    onIMUSearchCompleted(previousApp)
                                                }
                                            })
                                        }else{
                                            onIMUSearchCompleted(previousApp)
                                        }
                                    })
                                }else{
                                    onIMUSearchCompleted(previousApp)
                                }
                            })
                        }
                    }// Button
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.rowSpan: phoneLandscape ? 5 : 1
                        Layout.alignment: Qt.AlignTop
                        spacing: 10

                        SectionHeader {
                            text: "Profile"
                        }
                        GridLayout {
                            columns: 2
                            RadioButton {
                                id: configuratorRadioDefault
                                text: "Default"
                                onClicked: {
                                    mAppConf.updateParamInt("imu_conf.sample_rate_hz", 200)
                                    mAppConf.updateParamEnum("imu_conf.mode", 0)
                                    mAppConf.updateParamDouble("imu_conf.accel_confidence_decay", 1.0)
                                    mAppConf.updateParamDouble("imu_conf.mahony_kp", 0.3)
                                    mAppConf.updateParamDouble("imu_conf.accel_lowpass_filter_z", 0.0)
                                    mAppConf.updateParamDouble("imu_conf.gyro_lowpass_filter", 0.0)
                                    profileTiles.model = profileTiles.getModel()
                                }
                            }
                            RadioButton {
                                id: configuratorRadioLogs
                                text: "Logs"
                                onClicked: {
                                    mAppConf.updateParamInt("imu_conf.sample_rate_hz", 200)
                                    mAppConf.updateParamEnum("imu_conf.mode", 1)
                                    mAppConf.updateParamDouble("imu_conf.accel_confidence_decay", 1.0)
                                    mAppConf.updateParamDouble("imu_conf.mahony_kp", 0.3)
                                    mAppConf.updateParamDouble("imu_conf.accel_lowpass_filter_z", 0.0)
                                    mAppConf.updateParamDouble("imu_conf.gyro_lowpass_filter", 0.0)
                                    profileTiles.model = profileTiles.getModel()
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
                                    mAppConf.updateParamDouble("imu_conf.mahony_kp", 0.2)
                                    mAppConf.updateParamDouble("imu_conf.accel_lowpass_filter_z", 0.0)
                                    mAppConf.updateParamDouble("imu_conf.gyro_lowpass_filter", 0.0)
                                    profileTiles.model = profileTiles.getModel()
                                }
                            }
                            RadioButton {
                                id: configuratorRadioBSkate
                                text: "Balance Skateboard"
                                onClicked: {
                                    if(imuType == 4){
                                        mAppConf.updateParamInt("imu_conf.sample_rate_hz", 800)
                                    }else if(imuType == 5){
                                        mAppConf.updateParamInt("imu_conf.sample_rate_hz", 832)
                                    }else{
                                        mAppConf.updateParamInt("imu_conf.sample_rate_hz", 1000)
                                    }
                                    mAppConf.updateParamEnum("imu_conf.mode", 1)
                                    mAppConf.updateParamDouble("imu_conf.accel_confidence_decay", 0.02)
                                    mAppConf.updateParamDouble("imu_conf.mahony_kp", 2.0)
                                    mAppConf.updateParamDouble("imu_conf.accel_lowpass_filter_z", 1.0)
                                    mAppConf.updateParamDouble("imu_conf.gyro_lowpass_filter", 0.0)
                                    profileTiles.model = profileTiles.getModel()
                                }
                            }
                        }
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 3
                            columnSpacing: 8
                            rowSpacing: 8
                            uniformCellWidths: true

                            Repeater {
                                function getModel(){
                                    return [["Frequency", mAppConf.getParamInt("imu_conf.sample_rate_hz"), "Hz"],
                                            ["AHRS", getAHRS(), ""],
                                            ["Acc decay", mAppConf.getParamDouble("imu_conf.accel_confidence_decay").toFixed(3), ""],
                                            ["Mahony KP", mAppConf.getParamDouble("imu_conf.mahony_kp").toFixed(3), ""],
                                            ["Accel Z filter", mAppConf.getParamDouble("imu_conf.accel_lowpass_filter_z").toFixed(1), ""],
                                            ["Gyro filter", mAppConf.getParamDouble("imu_conf.gyro_lowpass_filter").toFixed(1), ""]]
                                }

                                id: profileTiles
                                model: getModel()

                                ValueTile {
                                    Layout.fillWidth: true
                                    title: modelData[0]
                                    value: modelData[1]
                                    unit: modelData[2]
                                }
                            }
                        }
                    }
                }// Grid Layout
            }// Item (configurator)

        }

        // Live IMU values and 3D view beside the steps, below them in portrait
        Loader {
            active: dialog.visible && mainIsTablet
            asynchronous: true
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: stepFlick.width
            height: stepFlick.height

            sourceComponent: Rectangle {
                radius: 12
                color: Utility.getAppHexColor("normalBackground")

                Connections {
                    target: mCommands

                    function onValuesImuReceived(values) {
                        imuTiles.updateText(values)
                        imu3d.setRotation(values.roll, values.pitch, 0)
                    }
                }

                RtDataIMU {
                    id: imuTiles
                    anchors.top: parent.top
                    width: parent.width
                }

                Vesc3dItem {
                    id: imu3d
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: imuTiles.height / 2
                    width: Math.min(parent.width - 16, parent.height - imuTiles.height - 16) * Screen.devicePixelRatio
                    height: width
                    scale: 1.0 / Screen.devicePixelRatio
                }
            }
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
                Layout.margins: 10
                Layout.rightMargin: 5
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: (stackLayout.currentIndex != 0) ? "Cancel" : "Close"
                flat: true

                onClicked: {
                    if (stackLayout.currentIndex === pages.menu) {
                        dialog.close()
                        openTimer.stop()
                    } else if(stackLayout.currentIndex === pages.configurator){
                        mAppConf.updateParamEnum("imu_conf.type", configuratorRestore.type)
                        mAppConf.updateParamInt("imu_conf.sample_rate_hz", configuratorRestore.sample_rate_hz)
                        mAppConf.updateParamEnum("imu_conf.mode", configuratorRestore.mode)
                        mAppConf.updateParamDouble("imu_conf.accel_confidence_decay", configuratorRestore.accel_confidence_decay)
                        mAppConf.updateParamDouble("imu_conf.mahony_kp", configuratorRestore.mahony_kp)
                        mAppConf.updateParamDouble("imu_conf.accel_lowpass_filter_z", configuratorRestore.accel_lowpass_filter_z)
                        mAppConf.updateParamDouble("imu_conf.gyro_lowpass_filter", configuratorRestore.gyro_lowpass_filter)
                        mCommands.setAppConfNoStore()
                        stackLayout.currentIndex = pages.menu
                    } else if(stackLayout.currentIndex === pages.orientationRoll ||
                              stackLayout.currentIndex === pages.orientationPitch ||
                              stackLayout.currentIndex === pages.orientationYaw){
                        mAppConf.updateParamDouble("imu_conf.rot_roll", orientationRestore.rot_roll, null)
                        mAppConf.updateParamDouble("imu_conf.rot_pitch", orientationRestore.rot_pitch, null)
                        mAppConf.updateParamDouble("imu_conf.rot_yaw", orientationRestore.rot_yaw, null)
                        mAppConf.updateParamDouble("imu_conf.gyro_offsets__0", orientationRestore.gyro_offsets__0, null)
                        mAppConf.updateParamDouble("imu_conf.gyro_offsets__1", orientationRestore.gyro_offsets__1, null)
                        mAppConf.updateParamDouble("imu_conf.gyro_offsets__2", orientationRestore.gyro_offsets__2, null)
                        mAppConf.updateParamDouble("imu_conf.accel_offsets__0", orientationRestore.accel_offsets__0, null)
                        mAppConf.updateParamDouble("imu_conf.accel_offsets__1", orientationRestore.accel_offsets__1, null)
                        mAppConf.updateParamDouble("imu_conf.accel_offsets__2", orientationRestore.accel_offsets__2, null)
                        mCommands.setAppConfNoStore()
                        stackLayout.currentIndex = pages.menu
                    } else {
                        stackLayout.currentIndex = pages.menu
                    }
                }
            }

            Button {
                id: nextButton
                Layout.margins: 10
                Layout.leftMargin: 5
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: "Save"
                highlighted: stackLayout.currentIndex !== pages.gyro || gyroSettle.stable
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
                        applyRollOffset(-filteredIMUValues.roll)
                        stackLayout.currentIndex = pages.orientationPitch
                    }else if(stackLayout.currentIndex === pages.orientationPitch){
                        applyPitchOffset(filteredIMUValues.pitch)
                        stackLayout.currentIndex = pages.orientationYaw
                    }else if(stackLayout.currentIndex === pages.orientationYaw){
                        applyYawOffset(-calculatedYawOffset)
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

    // Title, step count and instruction at the top of each task
    component TaskIntro: ColumnLayout {
        id: intro
        property string title
        property string step
        property string instruction
        property string hint

        Layout.fillWidth: true
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 6

            Text {
                Layout.fillWidth: true
                text: intro.title
                color: Utility.getAppHexColor("titleText")
                font.weight: Font.Medium
                font.pointSize: 15
                wrapMode: Text.Wrap
            }
            Text {
                visible: text !== ""
                text: intro.step
                color: Utility.getAppHexColor("normalText")
                font.pointSize: 10
            }
        }
        Text {
            Layout.fillWidth: true
            text: intro.instruction
            color: Utility.getAppHexColor("lightText")
            font.weight: Font.Bold
            font.pointSize: 12
            wrapMode: Text.Wrap
        }
        Text {
            Layout.fillWidth: true
            text: intro.hint
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 11
            wrapMode: Text.Wrap
        }
    }

    function constrainDegrees(degrees){
        if(degrees > 360){
            return degrees - 360
        }else if (degrees < -360){
            return degrees + 360
        }else{
            return degrees
        }
    }

    function constrainRadians(radians){
        if(radians > 360){
            return radians - 360
        }else if (radians < -360){
            return radians + 360
        }else{
            return radians
        }
    }

    function rotateEulerAngles(angles, rotation){
        // Calculate sin & cos
        var s1, c1, s2, c2, s3, c3

        if (rotation.yaw !== 0.0) {
            s1 = Math.sin(rotation.yaw)
            c1 = Math.cos(rotation.yaw)
        } else {
            s1 = 0.0
            c1 = 1.0
        }

        if (rotation.pitch !== 0.0) {
            s2 = Math.sin(rotation.pitch)
            c2 = Math.cos(rotation.pitch)
        } else {
            s2 = 0.0
            c2 = 1.0
        }

        if (rotation.roll !== 0.0) {
            s3 = Math.sin(rotation.roll)
            c3 = Math.cos(rotation.roll)
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


        // Calculate and return rotated angles
        return {
            "roll": angles.roll * m11 + angles.pitch * m12 + angles.yaw * m13,
            "pitch": angles.roll * m21 + angles.pitch * m22 + angles.yaw * m23,
            "yaw": angles.roll * m31 + angles.pitch * m32 + angles.yaw * m33
        }
    }

    function applyRollOffset(roll){
        mAppConf.updateParamDouble("imu_conf.rot_roll", roll * 180.0/pi, null)
        applyRotationToCalibrationConfig(roll, 0, 0)
        mCommands.setAppConfNoStore()
    }

    function applyPitchOffset(pitch){
        mAppConf.updateParamDouble("imu_conf.rot_pitch", pitch * 180.0/pi, null)
        applyRotationToCalibrationConfig(0, pitch, 0)
        mCommands.setAppConfNoStore()
    }

    function applyYawOffset(yaw){
        mAppConf.updateParamDouble("imu_conf.rot_yaw", yaw * 180.0/pi, null)
        applyRotationToCalibrationConfig(0, 0, yaw)
        mCommands.setAppConf()
    }

    function applyRotationToCalibrationConfig(roll, pitch, yaw){
        // Read current calibration
        var offsetGyroX = mAppConf.getParamDouble("imu_conf.gyro_offsets__0")
        var offsetGyroY = mAppConf.getParamDouble("imu_conf.gyro_offsets__1")
        var offsetGyroZ = mAppConf.getParamDouble("imu_conf.gyro_offsets__2")

        var offsetAccelX = mAppConf.getParamDouble("imu_conf.accel_offsets__0")
        var offsetAccelY = mAppConf.getParamDouble("imu_conf.accel_offsets__1")
        var offsetAccelZ = mAppConf.getParamDouble("imu_conf.accel_offsets__2")

        // Calculate rotated calibration
        var rotatedGyroOffsets = rotateEulerAngles(
                    {"roll": offsetGyroX, "pitch": offsetGyroY, "yaw": offsetGyroZ},
                    {"roll": roll, "pitch": pitch, "yaw": yaw}
        )
        var rotatedAccelOffsets = rotateEulerAngles(
                    {"roll": offsetAccelX, "pitch": offsetAccelY, "yaw": offsetAccelZ},
                    {"roll": roll, "pitch": pitch, "yaw": yaw}
        )

        // Update settings (roll = x, pitch = y, yaw = z)
        mAppConf.updateParamDouble("imu_conf.gyro_offsets__0", rotatedGyroOffsets.roll, null)
        mAppConf.updateParamDouble("imu_conf.gyro_offsets__1", rotatedGyroOffsets.pitch, null)
        mAppConf.updateParamDouble("imu_conf.gyro_offsets__2", rotatedGyroOffsets.yaw, null)

        mAppConf.updateParamDouble("imu_conf.accel_offsets__0", rotatedAccelOffsets.roll, null)
        mAppConf.updateParamDouble("imu_conf.accel_offsets__1", rotatedAccelOffsets.pitch, null)
        mAppConf.updateParamDouble("imu_conf.accel_offsets__2", rotatedAccelOffsets.yaw, null)
    }

    function getPitchForYawOffset(yaw){
        var rotated = rotateEulerAngles(
                    {"roll": filteredIMUValues.roll, "pitch": filteredIMUValues.pitch, "yaw": filteredIMUValues.yaw},
                    {"roll": 0, "pitch": 0, "yaw": yaw}
        )

        return rotated.pitch
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
            }else if(internalTypeString === "-> imu_type_internal \n"){
                imuString = "Internal: Read error, redo search."
                imuType = 1
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

