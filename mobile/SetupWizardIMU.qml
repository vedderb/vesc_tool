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
        'accelerometer': 2,
        'orientation': 3,
        'configurator': 4
    }

    // The axis the open calibration is on, and what was saved for the ones before it
    property int calAxis: 0
    property var calResults: []

    // Moves on to the next axis, or back to the menu after the last one
    function nextAxis(result) {
        var r = calResults.slice()
        r[calAxis] = result
        calResults = r
        if (calAxis < 2) {
            calAxis++
        } else {
            stackLayout.currentIndex = pages.menu
        }
    }

    // Offset in degrees that saving the roll, pitch or yaw step applies. Only the asked axis is
    // read, so bindings on roll do not update with the yaw search
    function orientationOffset(axis) {
        if (axis === 0) {
            return -filteredIMUValues.roll * 180.0/pi - orientationRestore.rot_roll
        } else if (axis === 1) {
            return filteredIMUValues.pitch * 180.0/pi - orientationRestore.rot_pitch
        }
        return -calculatedYawOffset * 180.0/pi - orientationRestore.rot_yaw
    }

    function applyOrientationOffset(axis, offset) {
        if (axis === 0) {
            applyRollOffset(offset)
        } else if (axis === 1) {
            applyPitchOffset(offset)
        } else {
            applyYawOffset(offset)
        }
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
                            calAxis = 0
                            calResults = []
                            stackLayout.currentIndex = pages.accelerometer
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
                            calAxis = 0
                            calResults = []
                            stackLayout.currentIndex = pages.orientation
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

                // The plot beside the offsets on landscape phones
                GridLayout {
                    width: parent.width
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 8
                    uniformCellWidths: true
                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "Gyroscope Calibration"
                        instruction: "Leave the IMU completely still"
                        hint: "Any orientation works, but avoid vibration. Save once the offsets have settled."
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
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
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        GyroDriftPlot {
                            id: gyroPlot
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight
                            running: dialog.visible && stackLayout.currentIndex === pages.gyro && workingIMU
                            gyroX: filteredIMUValues.gyroX
                            gyroY: filteredIMUValues.gyroY
                            gyroZ: filteredIMUValues.gyroZ
                        }
                        Text {
                            visible: !workingIMU
                            text: "No IMU data, check the IMU Configurator"
                            color: Utility.getAppHexColor("warningText")
                        }
                    }
                }// Grid Layout
            }// Item (Gyro Calibration)

            Item{// Accel Calibration #2
                width: parent.width
                height: parent.height

                // Clear Max and Skip beside the axes on landscape phones
                GridLayout {
                    width: parent.width
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 8
                    uniformCellWidths: true
                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "Accelerometer Calibration"
                        step: "Step " + (calAxis + 1) + " of 3"
                        hint: accelHint
                    }
                    AxisSteps {
                        current: calAxis
                        titles: ["Accel X", "Accel Y", "Accel Z"]
                        values: [filteredIMUValues.accX, filteredIMUValues.accY, filteredIMUValues.accZ].map(v => v.toFixed(3) + " g")
                        results: calResults
                        instructions: ["X", "Y", "Z"].map(a => "Point +" + a + " straight up and hold it until Max stops rising")
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        AccelLevel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight
                            running: dialog.visible && stackLayout.currentIndex === pages.accelerometer
                            axis: calAxis
                            acc: [filteredIMUValues.accX, filteredIMUValues.accY, filteredIMUValues.accZ]
                            max: maxAccelValues["xyz"[calAxis]]
                        }
                        RowLayout {
                            Layout.fillWidth: true

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 500
                                text: "Clear Max"
                                flat: true

                                onClicked: {
                                    // Scale down filtered value so it looks like something happened
                                    var a = "XYZ"[calAxis]
                                    filteredIMUValues["acc" + a] = filteredIMUValues["acc" + a] * .9
                                    maxAccelValues[a.toLowerCase()] = -10.0
                                }
                            }// Button clear
                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 500
                                text: "Skip"
                                flat: true

                                onClicked: nextAxis("Skipped")
                            }// Button
                        }
                    }
                }// Grid Layout
            }// Item (Accel Calibration)

            Item{// Orientation Calibration #3
                width: parent.width
                height: parent.height

                // Skip beside the axes on landscape phones
                GridLayout {
                    width: parent.width
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 8
                    uniformCellWidths: true
                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "Orientation Calibration"
                        step: "Step " + (calAxis + 1) + " of 3"
                        hint: calAxis < 2 ? "Both pitch and roll should be level. Save once the offset has settled." :
                                                    "Keep the IMU stable. Save once the offset has settled."
                    }
                    AxisSteps {
                        current: calAxis
                        titles: ["Roll offset", "Pitch offset", "Yaw offset"]
                        // Only the open row shows a live value, so the others do not follow the yaw search
                        values: [0, 1, 2].map(i => i === calAxis ? orientationOffset(i).toFixed(2) + "\u00B0" : "")
                        results: calResults
                        instructions: ["Place the IMU level on a flat, stable surface",
                            "Keep the IMU level on a flat, stable surface",
                            "Raise the pitch to roughly <font color=\"" + Utility.getAppHexColor("warningText") + "\">45 degrees</font>, roll still level"]
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        LevelTrace {
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight
                            visible: calAxis < 2
                            running: dialog.visible && stackLayout.currentIndex === pages.orientation && calAxis < 2
                            axis: calAxis
                            // The offsets Save stores, as in the rows. Once roll is saved the IMU reports
                            // the new mounting, so the roll strip shows what is left of it
                            roll: orientationOffset(0) + (calAxis > 0 ? orientationRestore.rot_roll : 0)
                            pitch: orientationOffset(1)
                        }
                        YawLiftView {
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight
                            visible: calAxis === 2
                            roll: filteredIMUValues.roll * 180 / pi
                            pitch: filteredIMUValues.pitch * 180 / pi
                            yawOffset: -calculatedYawOffset * 180 / pi
                        }
                        Button {
                            Layout.fillWidth: true
                            text: "Skip"
                            flat: true

                            onClicked: {
                                var restore = [orientationRestore.rot_roll, orientationRestore.rot_pitch, orientationRestore.rot_yaw][calAxis] * pi/180
                                applyOrientationOffset(calAxis, restore)
                                nextAxis("Skipped")
                            }
                        }// Button
                    }
                }// Grid Layout
            }// Item (Orientation Calibration)

            Item{// configurator #4
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
                    } else if(stackLayout.currentIndex === pages.orientation){
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
                highlighted: stackLayout.currentIndex !== pages.gyro || (gyroPlot.settled && workingIMU)
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

                    }else if(stackLayout.currentIndex === pages.accelerometer){
                        // Save Accelerometer Offset
                        var key = "imu_conf.accel_offsets__" + calAxis
                        var max = maxAccelValues["xyz"[calAxis]]
                        mAppConf.updateParamDouble(key, mAppConf.getParamDouble(key) + max - 1, null)
                        mCommands.setAppConf()
                        nextAxis(max.toFixed(3) + " g")
                    }else if(stackLayout.currentIndex === pages.orientation){
                        var offset = orientationOffset(calAxis).toFixed(2) + "\u00B0"
                        applyOrientationOffset(calAxis, [-filteredIMUValues.roll, filteredIMUValues.pitch, -calculatedYawOffset][calAxis])
                        nextAxis(offset)
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
            visible: text !== ""
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

    // One row per axis. Rows that are done show what was saved, the current one opens with its
    // instruction and live value, and the ones after it wait greyed out
    component AxisSteps: ColumnLayout {
        id: axisSteps
        property int current: 0
        property var titles: []
        property var values: []
        property var results: []
        property var instructions: []

        Layout.fillWidth: true
        spacing: 8

        Repeater {
            model: axisSteps.titles

            Rectangle {
                id: axisRow
                readonly property bool done: index < axisSteps.current
                readonly property bool active: index === axisSteps.current
                readonly property bool skipped: done && axisSteps.results[index] === "Skipped"

                Layout.fillWidth: true
                implicitHeight: axisCol.implicitHeight + 20
                Behavior on implicitHeight { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
                radius: 12
                clip: true
                color: Utility.getAppHexColor("cardBackground")
                border.width: active ? 2 : 1
                border.color: Utility.getAppHexColor(active ? "lightAccent" : "raisedBackground")
                opacity: active || done ? 1.0 : 0.45
                Behavior on opacity { NumberAnimation { duration: 250 } }

                ColumnLayout {
                    id: axisCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 10
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            implicitWidth: 26
                            implicitHeight: 26
                            radius: 13
                            color: Utility.getAppHexColor(axisRow.skipped ? "raisedBackground" : axisRow.done ? "positiveText" :
                                                          axisRow.active ? "lightAccent" : "raisedBackground")

                            Text {
                                anchors.centerIn: parent
                                text: axisRow.skipped ? "\u2013" : axisRow.done ? "\u2713" : index + 1
                                color: Utility.getAppHexColor(axisRow.skipped ? "normalText" : axisRow.done || axisRow.active ? "darkBackground" : "normalText")
                                font.weight: Font.Bold
                                font.pointSize: 11
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData
                            elide: Text.ElideRight
                            color: Utility.getAppHexColor(axisRow.active ? "titleText" : "normalText")
                            font.weight: Font.Medium
                            font.pointSize: 12
                        }
                        Text {
                            text: axisRow.done ? (axisSteps.results[index] || "") : axisRow.active ? axisSteps.values[index] : ""
                            color: Utility.getAppHexColor(axisRow.skipped ? "normalText" : axisRow.done ? "positiveText" : "titleText")
                            font.pointSize: axisRow.active ? 15 : 12
                            font.features: { "tnum": 1 }
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 36
                        visible: axisRow.active
                        text: axisSteps.instructions[index]
                        color: Utility.getAppHexColor("lightText")
                        font.pointSize: 11
                        wrapMode: Text.Wrap
                    }
                }
            }
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
            }else if(internalTypeString === "Timeout"){
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

