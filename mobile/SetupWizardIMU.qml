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
    // IMU replies since the dialog opened, workingIMU is only known after a few
    property int imuReplies: 0
    property double lastReply: 0
    readonly property bool imuChecked: imuReplies > 25
    property int imuType: 0
    property string imuString: "Unknown"
    property var configuratorRestore: ({})
    property var orientationRestore: ({})
    property var filteredIMUValues: {
        'accX': 0.0,
        'accY': 0.0,
        'accZ': 0.0,
        'gyroXNoise': 0.0,
        'gyroXPrevious': 0.0,
    }

    property var pages: {
        'menu': 0,
        'gyro': 1,
        'accelerometer': 2,
        'orientation': 3,
        'configurator': 4
    }

    // Tasks saved this session, for the ticks on the menu, and whether the tasks run one after
    // the other instead of returning to the menu
    property var doneTasks: ({})
    property bool flowMode: false

    // Gyro: raw replies [time ms, x, y, z] while on the page, newest last, and the absolute
    // offsets taken once they stayed still
    property var gyroWindow: []
    property var gyroMean: null
    property var gyroCapture: null
    property real gyroMotion: NaN
    property real gyroLimit: 3
    property real gyroCountdown: -1

    // Captured accelerometer readings per axis, [up, down], NaN until captured
    property var accelFaces: [[NaN, NaN], [NaN, NaN], [NaN, NaN]]

    // Orientation: step 0 the level, 1 the yaw, and what was stored for each
    property int calAxis: 0
    property var calResults: []
    // The yaw is only taken after the vehicle has been level in the new frame
    property real liftYaw: 0
    property real capturedYaw: NaN
    property var yawHold: ({ref: 0, start: 0})

    // Configurator
    property bool searching: false
    property int searchType: 0
    property int searchPreviousApp: 0
    property int profileIndex: -1
    readonly property var profiles: [
        {name: "Default", mode: 0, decay: 1.0, kp: 0.3, accZ: 0.0, fast: false},
        {name: "Logs", mode: 1, decay: 1.0, kp: 0.3, accZ: 0.0, fast: false},
        {name: "Balance Unicycle", mode: 1, decay: 1.0, kp: 0.2, accZ: 0.0, fast: true},
        {name: "Balance Skateboard", mode: 1, decay: 0.02, kp: 2.0, accZ: 1.0, fast: true}
    ]

    function openDialog() {
        imuReplies = 0
        filteredIMUValues.gyroXNoise = 0
        dialog.open()
        openTimer.start()
    }

    // Resets the state of a task and shows it
    function openTask(page) {
        if (page === pages.configurator) {
            configuratorRestore = {
                "type": mAppConf.getParamEnum("imu_conf.type"),
                "sample_rate_hz": mAppConf.getParamInt("imu_conf.sample_rate_hz"),
                "mode": mAppConf.getParamEnum("imu_conf.mode"),
                "accel_confidence_decay": mAppConf.getParamDouble("imu_conf.accel_confidence_decay"),
                "mahony_kp": mAppConf.getParamDouble("imu_conf.mahony_kp"),
                "accel_lowpass_filter_z": mAppConf.getParamDouble("imu_conf.accel_lowpass_filter_z"),
                "gyro_lowpass_filter": mAppConf.getParamDouble("imu_conf.gyro_lowpass_filter")
            }
            updateIMUType()
            profileIndex = matchingProfile()
            profileTiles.model = profileTiles.getModel()
        } else if (page === pages.gyro) {
            gyroWindow = []
            gyroMean = null
            gyroCapture = null
            gyroMotion = NaN
            gyroCountdown = -1
        } else if (page === pages.accelerometer) {
            accelFaces = [[NaN, NaN], [NaN, NaN], [NaN, NaN]]
        } else if (page === pages.orientation) {
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
            // Measure in the frame of the IMU itself
            mAppConf.updateParamDouble("imu_conf.rot_roll", 0, null)
            mAppConf.updateParamDouble("imu_conf.rot_pitch", 0, null)
            mAppConf.updateParamDouble("imu_conf.rot_yaw", 0, null)
            applyRotationToCalibrationConfig(0, 0, -orientationRestore.rot_yaw * pi/180)
            applyRotationToCalibrationConfig(0, -orientationRestore.rot_pitch * pi/180, 0)
            applyRotationToCalibrationConfig(-orientationRestore.rot_roll * pi/180, 0, 0)
            rotateFiltered(0, 0, -orientationRestore.rot_yaw * pi/180)
            rotateFiltered(0, -orientationRestore.rot_pitch * pi/180, 0)
            rotateFiltered(-orientationRestore.rot_roll * pi/180, 0, 0)
            mCommands.setAppConfNoStore()
            calAxis = 0
            calResults = []
            capturedYaw = NaN
        }
        stackLayout.currentIndex = page
    }

    // Leaves a task, marking it done when saved, and opens the next one when running them all
    function finishTask(page, saved) {
        if (saved) {
            var d = Object.assign({}, doneTasks)
            d[page] = true
            // The level depends on the accelerometer offsets
            if (page === pages.accelerometer) {
                delete d[pages.orientation]
            }
            doneTasks = d
        }
        var next = page === pages.gyro ? pages.accelerometer : page === pages.accelerometer ? pages.orientation : -1
        if (flowMode && next >= 0) {
            openTask(next)
        } else {
            flowMode = false
            stackLayout.currentIndex = pages.menu
        }
    }

    // Offsets are the mean of the last 150 replies once all of them stayed within the still limit
    function updateGyro() {
        var w = gyroWindow
        if (w.length < 2) {
            gyroMotion = NaN
            return
        }
        // 3 °/s, or 8 times the reply to reply noise for noisy IMUs, at most 8 °/s so that holding
        // it in a hand does not raise it
        var d = []
        for (var i = 1; i < w.length; i++) {
            for (var k = 1; k < 4; k++) {
                d.push(Math.abs(w[i][k] - w[i - 1][k]))
            }
        }
        d.sort((a, b) => a - b)
        gyroLimit = Math.min(8, Math.max(3, 8 * d[Math.floor(d.length / 2)]))

        // Largest axis spread over the newest n replies
        var spread = n => Math.max(...[1, 2, 3].map(k => {
            var v = w.slice(-n).map(s => s[k])
            return Math.max(...v) - Math.min(...v)
        }))
        var run = 1
        while (run < w.length && spread(run + 1) < gyroLimit) {
            run++
        }
        gyroMotion = spread(25)
        gyroMean = [1, 2, 3].map(k => w.reduce((s, x) => s + x[k], 0) / w.length)
        var interval = (w[w.length - 1][0] - w[0][0]) / (w.length - 1)
        gyroCountdown = gyroMotion < gyroLimit ? (150 - run) * interval / 1000 : -1
        if (w.length === 150 && run === 150) {
            gyroCapture = [0, 1, 2].map(k => mAppConf.getParamDouble("imu_conf.gyro_offsets__" + k) + gyroMean[k])
        }
    }

    // The offset Save writes for a gyro axis
    function gyroOffset(axis) {
        return gyroCapture ? gyroCapture[axis] : gyroMean ? mAppConf.getParamDouble("imu_conf.gyro_offsets__" + axis) + gyroMean[axis] : NaN
    }

    // Captured sides of each axis for the rows, empty when none
    function accelResults() {
        return accelFaces.map(f => [f[0], f[1]].filter(v => !isNaN(v)).map(v => (v > 0 ? "+" : "\u2212") + Math.abs(v).toFixed(3))
                              .join(" / ") + (isNaN(f[0]) && isNaN(f[1]) ? "" : " g"))
    }

    // Accelerometer offset change for an axis. Up should read +1 g and down -1 g, with both the
    // offset is their mean, which a scale error does not move
    function accelOffsetChange(f) {
        return !isNaN(f[0]) && !isNaN(f[1]) ? (f[0] + f[1]) / 2 : !isNaN(f[0]) ? f[0] - 1 : f[1] + 1
    }

    // Gravity seen by the IMU in g, filtered
    function acc() {
        return [filteredIMUValues.accX, filteredIMUValues.accY, filteredIMUValues.accZ]
    }

    function levelOk() {
        var a = acc()
        return workingIMU && Math.abs(Math.hypot(a[0], a[1], a[2]) - 1) < 0.1
    }

    // rot_roll and rot_pitch in degrees that level the gravity reading, with the rotation zeroed
    function levelAngles() {
        var a = acc()
        return [Math.atan2(a[1], a[2]) * 180 / pi, Math.atan2(-a[0], Math.hypot(a[1], a[2])) * 180 / pi]
    }

    // How far the board is tilted from the level, in degrees
    function liftTilt() {
        var a = acc()
        return Math.acos(Math.max(-1, Math.min(1, a[2] / Math.max(1e-6, Math.hypot(a[0], a[1], a[2]))))) * 180 / pi
    }

    // Stores the level live and moves on to the yaw, which persists both
    function captureLevel(roll, pitch, result) {
        mAppConf.updateParamDouble("imu_conf.rot_roll", roll, null)
        mAppConf.updateParamDouble("imu_conf.rot_pitch", pitch, null)
        applyRotationToCalibrationConfig(roll * pi/180, pitch * pi/180, 0)
        rotateFiltered(roll * pi/180, pitch * pi/180, 0)
        mCommands.setAppConfNoStore()
        calResults = [result]
        calAxis = 1
        capturedYaw = NaN
        yawHold = {ref: 0, start: Date.now(), armed: false}
    }

    function storeYaw(yaw, result) {
        applyYawOffset(yaw * pi/180)
        calResults = [calResults[0], result]
        finishTask(pages.orientation, true)
    }

    // rot_yaw that turns the lifted nose to -X, so that it reads as positive pitch like in the
    // firmware. Taken once it holds within 2° for 1.5 s with the nose lifted 25-80°
    function updateYaw() {
        var a = acc()
        var tilt = liftTilt()
        liftYaw = Math.atan2(a[1], -a[0]) * 180 / pi
        if (!yawHold.armed || tilt < 25 || tilt > 80 || Math.abs(((liftYaw - yawHold.ref + 540) % 360) - 180) >= 2) {
            yawHold = {ref: liftYaw, start: Date.now(), armed: yawHold.armed || tilt < 15}
        } else if (Date.now() - yawHold.start > 1500) {
            capturedYaw = snapYaw(liftYaw)
        }
    }

    // The filtered gravity follows a rotation change at once, as the firmware readings do,
    // instead of sweeping through other directions that could pass for a lift
    function rotateFiltered(roll, pitch, yaw) {
        var r = rotateEulerAngles({"roll": filteredIMUValues.accX, "pitch": filteredIMUValues.accY, "yaw": filteredIMUValues.accZ},
                                  {"roll": roll, "pitch": pitch, "yaw": yaw})
        filteredIMUValues.accX = r.roll
        filteredIMUValues.accY = r.pitch
        filteredIMUValues.accZ = r.yaw
    }

    // A square mounting lands on a multiple of 90° when the lift was a little crooked. Not with a
    // steep level pitch, then the roll of the level step folds into the yaw
    function snapYaw(yaw) {
        var s = Math.round(yaw / 90) * 90
        if (s === -180) {
            s = 180
        }
        return Math.abs(yaw - s) < 8 && Math.abs(mAppConf.getParamDouble("imu_conf.rot_pitch")) < 45 ? s : Math.round(yaw)
    }

    // Profile that matches the current settings, -1 when none
    function matchingProfile() {
        var near = (a, b) => Math.abs(a - b) < 1e-4
        return profiles.findIndex(p => p.mode === mAppConf.getParamEnum("imu_conf.mode") &&
                                  near(p.decay, mAppConf.getParamDouble("imu_conf.accel_confidence_decay")) &&
                                  near(p.kp, mAppConf.getParamDouble("imu_conf.mahony_kp")) &&
                                  near(p.accZ, mAppConf.getParamDouble("imu_conf.accel_lowpass_filter_z")) &&
                                  near(0, mAppConf.getParamDouble("imu_conf.gyro_lowpass_filter")))
    }

    function applyProfile(index) {
        var p = profiles[index]
        // The balance profiles run as fast as the IMU allows
        mAppConf.updateParamInt("imu_conf.sample_rate_hz", !p.fast ? 200 : imuType === 4 ? 800 : imuType === 5 ? 833 : 1000)
        mAppConf.updateParamEnum("imu_conf.mode", p.mode)
        mAppConf.updateParamDouble("imu_conf.accel_confidence_decay", p.decay)
        mAppConf.updateParamDouble("imu_conf.mahony_kp", p.kp)
        mAppConf.updateParamDouble("imu_conf.accel_lowpass_filter_z", p.accZ)
        mAppConf.updateParamDouble("imu_conf.gyro_lowpass_filter", 0.0)
        profileIndex = index
        profileTiles.model = profileTiles.getModel()
    }

    // Tries each IMU type for 2 s with the app off, so that it does not hold the COMM port
    function startSearch() {
        searchPreviousApp = mAppConf.getParamEnum("app_to_use")
        mAppConf.updateParamEnum("app_to_use", 0)
        searchType = 1
        mAppConf.updateParamEnum("imu_conf.type", searchType)
        mCommands.setAppConfNoStore()
        updateIMUType()
        searching = true
        searchTimer.start()
    }

    function searchStep() {
        if (workingIMU) {
            stopSearch(true)
        } else if (searchType < mAppConf.getParamEnumNames("imu_conf.type").length - 1) {
            searchType++
            mAppConf.updateParamEnum("imu_conf.type", searchType)
            mCommands.setAppConfNoStore()
            updateIMUType()
        } else {
            stopSearch(false)
        }
    }

    function stopSearch(found) {
        searchTimer.stop()
        searching = false
        if (!found) {
            mAppConf.updateParamEnum("imu_conf.type", configuratorRestore.type)
        }
        mAppConf.updateParamEnum("app_to_use", searchPreviousApp)
        mCommands.setAppConfNoStore()
        updateIMUType()
        if (profileIndex >= 0) {
            applyProfile(profileIndex)
        }
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
        id: searchTimer
        interval: 2000
        repeat: true
        onTriggered: searchStep()
    }

    Connections {
        target: mCommands

        function onValuesImuReceived(values) {
            imuReplies++
            lastReply = Date.now()
            filteredIMUValues.accX = (filteredIMUValues.accX * .98) + (values.accX * .02)
            filteredIMUValues.accY = (filteredIMUValues.accY * .98) + (values.accY * .02)
            filteredIMUValues.accZ = (filteredIMUValues.accZ * .98) + (values.accZ * .02)

            if (stackLayout.currentIndex === pages.gyro) {
                gyroWindow.push([Date.now(), values.gyroX, values.gyroY, values.gyroZ])
                if (gyroWindow.length > 150) {
                    gyroWindow.shift()
                }
            }

            // Detect working IMU based on noise levels of the gyro X axis
            var noise = Math.abs(values.gyroX - filteredIMUValues.gyroXPrevious)
            filteredIMUValues.gyroXPrevious = values.gyroX
            filteredIMUValues.gyroXNoise = (filteredIMUValues.gyroXNoise * .9) + (noise * .1)
            workingIMU = filteredIMUValues.gyroXNoise >= 0.01
        }
    }

    // The values above update at the 50 Hz poll rate, the pages only need to redraw at 10 Hz
    Timer {
        interval: 100
        repeat: true
        running: dialog.visible

        onTriggered: {
            // Frozen readings would pass every steadiness test
            if (Date.now() - lastReply > 300) {
                workingIMU = false
            }
            if (stackLayout.currentIndex === pages.gyro) {
                updateGyro()
            } else if (stackLayout.currentIndex === pages.orientation) {
                if (calAxis === 1) {
                    updateYaw()
                }
            }
            filteredIMUValuesChanged()
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

                // The three calibrations side by side on landscape phones
                GridLayout {
                    width: parent.width
                    height: parent.height
                    columns: phoneLandscape ? 3 : 1
                    rowSpacing: 10
                    columnSpacing: 10
                    uniformCellWidths: true

                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 3 : 1
                        title: "IMU Wizard"
                        instruction: "Set up the IMU, then calibrate it"
                        hint: "Calibrate all runs the three calibrations one after the other, each can also be run on its own. " +
                              "Run the accelerometer before the orientation, the level depends on it."
                    }
                    Text {
                        Layout.columnSpan: phoneLandscape ? 3 : 1
                        visible: !imuChecked
                        text: "Checking IMU\u2026"
                        color: Utility.getAppHexColor("normalText")
                    }
                    WarningBanner {
                        Layout.columnSpan: phoneLandscape ? 3 : 1
                        Layout.fillWidth: true
                        visible: imuChecked && !workingIMU
                        text: "We have detected that your IMU is not working. The IMU Configurator can help set it up."
                    }
                    ImageButton {
                        Layout.columnSpan: phoneLandscape ? 3 : 1
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        buttonText: "IMU Configurator"
                        hintText: (doneTasks[pages.configurator] ? "\u2713 Saved. " : "") + "Find the IMU and choose a profile"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Search-96.png"
                        badge: imuChecked && !workingIMU

                        onClicked: {
                            flowMode = false
                            openTask(pages.configurator)
                        }
                    }
                    // Calibrate all runs the three tasks below it
                    SectionHeader {
                        Layout.columnSpan: phoneLandscape ? 3 : 1
                        Layout.topMargin: 6
                        text: "Calibration"
                    }
                    Button {
                        Layout.columnSpan: phoneLandscape ? 3 : 1
                        Layout.fillWidth: true
                        text: "Calibrate all"
                        highlighted: true
                        enabled: imuChecked && workingIMU

                        onClicked: {
                            flowMode = true
                            openTask(pages.gyro)
                        }
                    }
                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        buttonText: "Gyroscope Calibration"
                        hintText: (doneTasks[pages.gyro] ? "\u2713 Saved. " : "") + "Measure the gyro offsets at rest"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Gyroscope-96.png"
                        locked: !imuChecked || !workingIMU
                        lockedText: "Needs a working IMU"

                        onClicked: {
                            flowMode = false
                            openTask(pages.gyro)
                        }
                    }
                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        buttonText: "Accelerometer Calibration"
                        hintText: (doneTasks[pages.accelerometer] ? "\u2713 Saved. " : "") + "Hold each side up, usually not needed"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/Offset-96.png"
                        locked: !imuChecked || !workingIMU
                        lockedText: "Needs a working IMU"

                        onClicked: {
                            flowMode = false
                            openTask(pages.accelerometer)
                        }
                    }
                    ImageButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        buttonText: "Orientation Calibration"
                        hintText: doneTasks[pages.orientation] ? "\u2713 Saved. Level it, then lift the front" :
                                  doneTasks[pages.accelerometer] ? "Redo, the accelerometer offsets changed" : "Level it, then lift the front"
                        imageSrc: "qrc" + Utility.getThemePath() + "icons/icons8-target-96.png"
                        locked: !imuChecked || !workingIMU
                        lockedText: "Needs a working IMU"

                        onClicked: {
                            flowMode = false
                            openTask(pages.orientation)
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
                        instruction: "Set it down and leave it still for 3 seconds"
                        hint: "Best in its riding position. The offsets are taken automatically."
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        columns: 3
                        columnSpacing: 8
                        uniformCellWidths: true

                        Repeater {
                            model: ["X", "Y", "Z"]

                            ValueTile {
                                readonly property real offset: gyroOffset(index)
                                Layout.fillWidth: true
                                title: modelData + " offset"
                                value: isNaN(offset) ? "\u2013" : offset.toFixed(2)
                                unit: "\u00B0/s"
                                valueColor: Utility.getAppHexColor(gyroCapture ? "positiveText" : "normalText")
                            }
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        GyroDriftPlot {
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight
                            running: dialog.visible && stackLayout.currentIndex === pages.gyro && workingIMU
                            motion: gyroMotion
                            limit: gyroLimit
                            countdown: gyroCountdown
                            captured: gyroCapture !== null
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

                // The level beside the axes on landscape phones
                GridLayout {
                    width: parent.width
                    columns: phoneLandscape ? 2 : 1
                    rowSpacing: 10
                    columnSpacing: 8
                    uniformCellWidths: true
                    TaskIntro {
                        Layout.columnSpan: phoneLandscape ? 2 : 1
                        title: "Accelerometer Calibration"
                        step: accelResults().filter(r => r).length + " of 3 axes"
                        hint: "Turn the vehicle or controller onto each side in any order and hold it still. Each axis needs one side facing up or down, " +
                              "both sides also cancel scale errors. Save is ready once all three axes are captured" +
                              (flowMode ? ", Skip leaves it out, it is usually not needed." : ".")
                    }
                    AxisSteps {
                        current: accelLevel.hasData ? accelLevel.axis : -1
                        currentDone: accelLevel.steady
                        titles: ["Accel X", "Accel Y", "Accel Z"]
                        // A captured side shows its locked value, like the level
                        values: acc().map((v, i) => (i === accelLevel.axis && accelLevel.steady ? accelLevel.value : v).toFixed(3) + " g")
                        results: accelResults()
                        instructions: ["X", "Y", "Z"].map(a => "Keep " + a + " within 2° of vertical until the reading turns green")
                    }
                    AccelLevel {
                        id: accelLevel
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        running: dialog.visible && stackLayout.currentIndex === pages.accelerometer && workingIMU
                        acc: [filteredIMUValues.accX, filteredIMUValues.accY, filteredIMUValues.accZ]
                        faces: accelFaces
                        onCaptured: (axis, sign, value) => {
                            var f = accelFaces.slice()
                            f[axis] = f[axis].slice()
                            f[axis][sign > 0 ? 0 : 1] = value
                            accelFaces = f
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
                        step: "Step " + (calAxis + 1) + " of 2"
                        hint: calAxis === 0 ? "Level is the pose the IMU reports as 0\u00B0 roll and pitch, normally the riding pose, see the examples. " +
                                              "Save takes it once it holds still." :
                                              "The front is the end that reads positive pitch when lifted. The yaw is taken once the lift holds still, Save stores the level and the yaw, Cancel discards both."
                    }
                    // The examples under the rows, beside the trace on landscape phones
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 10

                        AxisSteps {
                            current: calAxis
                            titles: ["Level", "Yaw offset"]
                            // Only the open row shows a live value
                            values: [calAxis === 0 ? levelAngles().map(v => v.toFixed(1) + "\u00B0").join(" / ") : "",
                                     calAxis !== 1 ? "" : !isNaN(capturedYaw) ? capturedYaw.toFixed(0) + "\u00B0" :
                                                          liftTilt() >= 25 ? liftYaw.toFixed(0) + "\u00B0" : ""]
                            results: calResults
                            instructions: ["Set it in the pose that should read 0\u00B0 roll and pitch, usually how it sits when ridden, and let go",
                                "Rest the back on the ground and lift the front <font color=\"" + Utility.getAppHexColor("warningText") +
                                "\">25\u00B0 or more</font>, keeping it straight"]
                        }
                        LevelExamples {
                            Layout.fillWidth: true
                            lift: calAxis === 1
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 6

                        LevelTrace {
                            id: levelTrace
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight
                            visible: calAxis === 0
                            running: dialog.visible && stackLayout.currentIndex === pages.orientation && calAxis === 0
                            roll: levelAngles()[0]
                            pitch: levelAngles()[1]
                            // Roll is barely defined with X near vertical
                            rollWeight: Math.hypot(acc()[1], acc()[2]) / Math.max(1e-6, Math.hypot(...acc()))
                        }
                        YawLiftView {
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight
                            visible: calAxis === 1
                            tilt: liftTilt()
                            yawOffset: isNaN(capturedYaw) ? liftYaw : capturedYaw
                            captured: !isNaN(capturedYaw)
                        }
                        Button {
                            Layout.fillWidth: true
                            text: calAxis === 0 ? "Keep current level" : "Keep current yaw"
                            flat: true

                            onClicked: {
                                if (calAxis === 0) {
                                    captureLevel(orientationRestore.rot_roll, orientationRestore.rot_pitch, "Kept")
                                } else {
                                    storeYaw(orientationRestore.rot_yaw, "Kept")
                                }
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
                    rows: phoneLandscape ? 4 : -1
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
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        font.pointSize: 13
                        color: Utility.getAppHexColor(searching ? "lightText" : workingIMU ? "positiveText" : "warningText")
                        text: searching ? "Searching, trying " + mAppConf.getParamEnumNames("imu_conf.type")[searchType] + "\u2026" :
                              !imuChecked ? "Checking IMU\u2026" : workingIMU ? "Found: " + imuString :
                              "No IMU found. Check the wiring, an app using the COMM port can block an external IMU."
                    }
                    Button {
                        Layout.fillWidth: true
                        text: searching ? "Searching\u2026" : "Search for IMU"
                        enabled: !searching
                        flat: false
                        highlighted: imuChecked && !workingIMU && !searching
                        onClicked: startSearch()
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.rowSpan: phoneLandscape ? 4 : 1
                        Layout.alignment: Qt.AlignTop
                        spacing: 10

                        SectionHeader {
                            text: "Profile"
                        }
                        GridLayout {
                            columns: 2

                            Repeater {
                                model: profiles

                                RadioButton {
                                    text: modelData.name
                                    checked: profileIndex === index
                                    onClicked: {
                                        applyProfile(index)
                                        checked = Qt.binding(() => profileIndex === index)
                                    }
                                }
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: profileIndex < 0
                            wrapMode: Text.Wrap
                            text: "The current settings match none of the profiles, they stay unless one is picked."
                            color: Utility.getAppHexColor("normalText")
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
                    flowMode = false
                    if (stackLayout.currentIndex === pages.menu) {
                        dialog.close()
                        openTimer.stop()
                    } else if(stackLayout.currentIndex === pages.configurator){
                        if (searching) {
                            stopSearch(false)
                        }
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

            // Running all tasks, the accelerometer can be left out, it is usually not needed
            Button {
                Layout.margins: 10
                Layout.leftMargin: 5
                Layout.rightMargin: 5
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                visible: flowMode && stackLayout.currentIndex === pages.accelerometer
                text: "Skip"
                flat: true
                onClicked: finishTask(pages.accelerometer, false)
            }

            Button {
                id: nextButton
                Layout.margins: 10
                Layout.leftMargin: 5
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: flowMode && (stackLayout.currentIndex === pages.gyro || stackLayout.currentIndex === pages.accelerometer) ?
                          "Save && next" : "Save"
                visible: stackLayout.currentIndex !== pages.menu
                // Only once there is something good to save
                readonly property bool ready: {
                    var page = stackLayout.currentIndex
                    return page === pages.gyro ? gyroCapture !== null && workingIMU :
                           page === pages.accelerometer ? accelResults().every(r => r) :
                           page === pages.orientation ? (calAxis === 0 ? levelTrace.steady && levelOk() : !isNaN(capturedYaw)) :
                           !searching
                }
                enabled: ready
                highlighted: ready

                onClicked: {
                    if(stackLayout.currentIndex === pages.gyro){
                        for (var k = 0; k < 3; k++) {
                            mAppConf.updateParamDouble("imu_conf.gyro_offsets__" + k, gyroCapture[k], null)
                        }
                        mCommands.setAppConf()
                        finishTask(pages.gyro, true)
                    }else if(stackLayout.currentIndex === pages.accelerometer){
                        // Save the captured axes
                        for (var i = 0; i < 3; i++) {
                            if (!isNaN(accelFaces[i][0]) || !isNaN(accelFaces[i][1])) {
                                var key = "imu_conf.accel_offsets__" + i
                                mAppConf.updateParamDouble(key, mAppConf.getParamDouble(key) + accelOffsetChange(accelFaces[i]), null)
                            }
                        }
                        mCommands.setAppConf()
                        finishTask(pages.accelerometer, true)
                    }else if(stackLayout.currentIndex === pages.orientation){
                        if (calAxis === 0) {
                            var l = levelAngles()
                            captureLevel(l[0], l[1], l.map(v => v.toFixed(1) + "\u00B0").join(" / "))
                        } else {
                            storeYaw(capturedYaw, capturedYaw.toFixed(0) + "\u00B0")
                        }
                    }else if(stackLayout.currentIndex === pages.configurator){
                        mCommands.setAppConf()
                        finishTask(pages.configurator, true)
                    }
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

    // One row per axis. Rows with a result show it, the current one opens with its instruction and
    // live value, and the others wait greyed out
    component AxisSteps: ColumnLayout {
        id: axisSteps
        property int current: 0
        property bool currentDone: false    // the open row already has its result, it stays green
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
                readonly property bool active: index === axisSteps.current
                readonly property bool done: !active && !!axisSteps.results[index]
                readonly property bool skipped: done && axisSteps.results[index] === "Skipped"
                readonly property bool locked: active && axisSteps.currentDone

                Layout.fillWidth: true
                implicitHeight: axisCol.implicitHeight + 20
                Behavior on implicitHeight { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
                radius: 12
                clip: true
                color: Utility.getAppHexColor("cardBackground")
                border.width: active ? 2 : 1
                border.color: Utility.getAppHexColor(locked ? "positiveText" : active ? "lightAccent" : "raisedBackground")
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
                            color: Utility.getAppHexColor(axisRow.skipped ? "raisedBackground" : axisRow.done || axisRow.locked ? "positiveText" :
                                                          axisRow.active ? "lightAccent" : "raisedBackground")

                            Text {
                                anchors.centerIn: parent
                                text: axisRow.skipped ? "\u2013" : axisRow.done || axisRow.locked ? "\u2713" : index + 1
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
                            color: Utility.getAppHexColor(axisRow.skipped ? "normalText" : axisRow.done || axisRow.locked ? "positiveText" : "titleText")
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

    function updateIMUType(){
        imuType = mAppConf.getParamEnum("imu_conf.type")
        if (imuType === 0) {
            imuString = "Off"
        } else if (imuType === 1) {
            var internal = Utility.readInternalImuType(VescIf)
            imuString = internal === "Timeout" ? "Internal: Read error, redo search." : "Internal " + internal
            // As the external type of the same chip, for the rate of the profiles
            imuType = internal.startsWith("MPU") ? 2 : internal === "ICM20948" ? 3 : internal.startsWith("BMI160") ? 4 :
                      internal.startsWith("LSM6DS3") ? 5 : 1
        } else {
            imuString = mAppConf.getParamEnumNames("imu_conf.type")[imuType]
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

