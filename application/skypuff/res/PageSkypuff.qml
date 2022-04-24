import QtQuick 2.12
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.2
import QtQuick.Extras 1.4
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0

import SkyPuff.vesc.winch 1.0

Page {
    id: page
    state: "DISCONNECTED"

    property string bgGreenColor: '#A5D6A7'
    property string bgBlueColor: '#90CAF9'
    property string bgRedColor: '#EF9A9A'
    property string bgYellowColor: '#FFE082'

    property real bFontSize: Math.max(page.width * 0.04, 10)
    property real bHeight: Math.max(page.width * 0.17, 10)
    property real kgValFontSize: Math.max(page.width * 0.04, 10)

    // Get normal text color from this palette
    SystemPalette {id: systemPalette; colorGroup: SystemPalette.Active}

    // Confirm Set Zero Here
    Popup {
        id: confirmResetZero
        modal: true
        anchors.centerIn: parent
        contentWidth: parent.width - 50

        ColumnLayout {
            anchors.fill: parent

            Label {
                Layout.margins: 10
                text: qsTr('Set zero here')
                font.bold: true
            }

            Text {
                Layout.margins: 10
                Layout.fillWidth: true
                elide: Text.ElideMiddle
                wrapMode: Text.WordWrap
                text: qsTr("Reset rope position?")
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 20
                Button {
                    text: qsTr('Yes')
                    onClicked: {
                        confirmResetZero.close()
                        Skypuff.sendTerminal("set_zero");
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr('No')
                    onClicked: confirmResetZero.close()
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            spacing: 0

            BigRoundButton {
                id: bCut
                enabled: true
                radius: 0

                text: qsTr("Cut")
                font.pixelSize: page.bFontSize
                Layout.preferredHeight: page.bHeight
                Layout.fillWidth: true
                Layout.preferredWidth: page.width / 10
                background.anchors.fill: bCut
                Material.background: page.bgRedColor

                onClicked: {Skypuff.sendTerminal("adc2_tick")}
            }

            BigRoundButton {
                id: bStop
                enabled: false
                radius: 0

                text: qsTr("Stop")
                font.pixelSize: page.bFontSize
                Layout.preferredHeight: page.bHeight
                Layout.fillWidth: true
                background.anchors.fill: bStop
                Material.background: page.bgYellowColor

                CustomBorder {
                    visible: parent.enabled
                    commonBorder: false
                    lBorderwidth: 1
                    rBorderwidth: 0
                    tBorderwidth: 0
                    bBorderwidth: 0
                    borderColor: Qt.darker(page.bgYellowColor, 1.2)
                }

                onClicked: {Skypuff.sendTerminal("set MANUAL_BRAKING")}
            }
        }

        /*Label {
            visible: true
            id: lState
            text: Skypuff.stateText

            Layout.fillWidth: true
            Layout.topMargin: 10
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 16
            font.bold: true

            color: page.state === "MANUAL_BRAKING" ? "red" : systemPalette.text;
        }*/

        // Status messages from skypuff with normal text color
        // or blinking faults
        /*Text {
            id: tStatus
            visible: true
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter

            SequentialAnimation on color {
                id: faultsBlinker
                loops: Animation.Infinite
                ColorAnimation { easing.type: Easing.OutExpo; from: systemPalette.window; to: "red"; duration: 400 }
                ColorAnimation { easing.type: Easing.OutExpo; from: "red"; to: systemPalette.window;  duration: 200 }
            }

            Timer {
                id: statusCleaner
                interval: 5 * 1000

                onTriggered: {
                    tStatus.text = Skypuff.fault

                    if(Skypuff.fault)
                        faultsBlinker.start()
                    else
                        faultsBlinker.stop()
                }
            }

            Connections {
                target: Skypuff

                onStatusChanged: {
                    tStatus.text = newStatus
                    tStatus.color = isWarning ? "red" : systemPalette.text

                    statusCleaner.restart()
                    faultsBlinker.stop()
                }

                onFaultChanged:  {
                    if(newFault) {
                        tStatus.text = newFault
                        faultsBlinker.start()
                    }
                    else
                        statusCleaner.restart()
                }
            }
        }*/

        SkypuffGauge {
            id: sGauge

            rootDiameter: page.width

            smallDimension: true // отключи, если изменение скорости или веревки слишком грузит цпу

            paddingLeft: 10
            paddingRight: 10
            marginTop: 15
            //maxRopeMeters: 20

            // Temps
            tempFets: Skypuff.tempFets
            tempMotor: Skypuff.tempMotor

            // Statuses
            stateText: Skypuff.stateText
            status: Skypuff.fault

            Connections {
                target: Skypuff

                //onMotorModeChanged: { sGauge.motorMode = Skypuff.motorMode; }
                onMotorKgChanged: { sGauge.motorKg = Math.abs(Skypuff.motorKg); }
                onSpeedMsChanged: { sGauge.speedMs = Skypuff.speedMs; }
                onPowerChanged: { sGauge.power = Skypuff.power; }

                onLeftMetersChanged: { sGauge.leftRopeMeters = Skypuff.leftMeters.toFixed(1); }
                onDrawnMetersChanged: { sGauge.ropeMeters = Skypuff.drawnMeters; }
                onRopeMetersChanged: { sGauge.maxRopeMeters = Skypuff.ropeMeters.toFixed(); }

                // Warning and Blink (bool) | I don't know names of this params
                //onIsMotorKgWarningChanged: { sGauge.isMotorKgWarning = false; } // Warning
                //onIsMotorKgBlinkingChanged: { sGauge.isMotorKgBlinking = false; } // Blink

                //onIsRopeWarningChanged: { sGauge.isRopeWarning = false; }
                //onIsRopeBlinkingChanged: { sGauge.isRopeBlinking = false; }

                //onIsPowerWarningChanged: { sGauge.isPowerWarning = false; }
                //onIsPowerBlinkingChanged: { sGauge.ispowerBlinking = false; }

                //onIsSpeedWarningChanged: { sGauge.isSpeedWarning = false; }
                //onIsSpeedBlinkingChanged: { sGauge.isSpeedBlinking = false; }

                //onIsTempBatteryWarningChanged: { sGauge.isTempBatteryWarning = false; }
                //onIsTempMcuWarningChanged: { sGauge.isTempMcuWarning = false; }
                //onIsTempMotorWarningChanged: { sGauge.isTempMotorWarning = false; }

                onSettingsChanged: {
                    sGauge.maxMotorKg = cfg.motor_max_kg;
                    sGauge.maxPower = cfg.power_max;
                    sGauge.minPower = cfg.power_min;
                    sGauge.maxSpeedMs = cfg.max_speed_ms;
                }

                onStateChanged: {
                    sGauge.state = newState;
                }

                onStatusChanged: {
                    sGauge.status = newStatus;
                    sGauge.isWarningStatus = isWarning;
                }

                onFaultChanged:  {
                    if(newFault) {
                        sGauge.status = newFault;
                    }

                }
            }
        }

        RowLayout {
            Layout.topMargin: 15
            function isManualSlowButtonsEnabled() {
                return ["MANUAL_SLOW_SPEED_UP",
                         "MANUAL_SLOW",
                         "MANUAL_SLOW_BACK_SPEED_UP",
                         "MANUAL_SLOW_BACK"].indexOf(page.state) === -1
            }

            function isManualSlowButtonsVisible() {
                return ["MANUAL_BRAKING",
                        "MANUAL_SLOW_SPEED_UP",
                        "MANUAL_SLOW",
                        "MANUAL_SLOW_BACK_SPEED_UP",
                        "MANUAL_SLOW_BACK"].indexOf(page.state) !== -1
            }

            RoundButton {
                id: rManualSlowBack
                Layout.topMargin: -5
                Layout.leftMargin: 5
                text: "←";
                enabled: parent.isManualSlowButtonsEnabled()
                visible: parent.isManualSlowButtonsVisible()
                onClicked: Skypuff.sendTerminal("set manual_slow")
                Material.background: page.bgGreenColor
            }

            Item {
                Layout.fillWidth: true
            }

            SkypuffBattery {
                id: batteryBlock
                gauge: sGauge
                Layout.alignment: Qt.AlignHCenter

                Connections {
                    target: Skypuff
                    onIsBatteryBlinkingChanged: { batteryBlock.isBatteryBlinking = Skypuff.isBatteryBlinking; }
                    onIsBatteryWarningChanged: { batteryBlock.isBatteryWarning = Skypuff.isBatteryWarning; }
                    onIsBatteryScaleValidChanged: { batteryBlock.isBatteryScaleValid = Skypuff.isBatteryScaleValid; }

                    onBatteryPercentsChanged: { batteryBlock.batteryPercents = Skypuff.batteryPercents; }
                    onBatteryCellVoltsChanged: { batteryBlock.batteryCellVolts = Skypuff.batteryCellVolts; }

                    onSettingsChanged: {
                        batteryBlock.batteryCells = cfg.battery_cells;
                    }
                }
            }
            Item {
                Layout.fillWidth: true
            }
            RoundButton {
                id: rManualSlowForward
                Layout.topMargin: -5
                Layout.rightMargin: 5
                text: "→";
                enabled: parent.isManualSlowButtonsEnabled()
                visible: parent.isManualSlowButtonsVisible()
                onClicked: Skypuff.sendTerminal("set manual_slow_back")
                Material.background: page.bgGreenColor
            }
        }

        /*GaugeDebug {
            id: debugBlock
            gauge: sGauge
            battery: batteryBlock
            visible: true
        }*/

        // Vertical space
        Item {
            Layout.fillHeight: true
        }

        RealSpinBox {
            id: pullForce

            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 30

            enabled: false
            font.pointSize: page.kgValFontSize
            font.bold: true

            decimals: 1
            from: 1
            suffix: qsTr("Kg")

            onValueModified: Skypuff.sendTerminal("force %1".arg(value))
        }

        RowLayout {
            spacing: 0

            BigRoundButton {
                id: bSetZero
                visible: false
                radius: 0

                text: qsTr("Set zero here")
                font.pixelSize: page.bFontSize

                Layout.fillWidth: true
                Layout.preferredHeight: page.bHeight
                background.anchors.fill: bSetZero
                Material.background: page.bgBlueColor

                onClicked: { confirmResetZero.open() }
            }

            BigRoundButton {
                id: bPrePull
                radius: 0
                enabled: false

                font.pixelSize: page.bFontSize
                Layout.preferredHeight: page.bHeight
                Layout.fillWidth: true
                background.anchors.fill: bPrePull
                Material.background: page.bgBlueColor

                state: "PRE_PULL"
                states: [
                    State {name: "PRE_PULL"; PropertyChanges {target: bPrePull;text: qsTr("Pre Pull")}},
                    State {name: "TAKEOFF_PULL"; PropertyChanges {target: bPrePull;text: qsTr("Takeoff Pull")}},
                    State {name: "PULL"; PropertyChanges {target: bPrePull;text: qsTr("Pull")}},
                    State {name: "FAST_PULL"; PropertyChanges {target: bPrePull;text: qsTr("Fast Pull")}}
                ]

                onClicked: {Skypuff.sendTerminal("set %1".arg(state))}
            }

            BigRoundButton {
                id: bUnwinding
                radius: 0
                enabled: false

                text: qsTr("Unwinding")
                font.pixelSize: page.bFontSize

                Layout.fillWidth: true
                Material.background: page.bgGreenColor
                Layout.preferredHeight: page.bHeight
                background.anchors.fill: bUnwinding

                CustomBorder {
                    visible: parent.enabled
                    commonBorder: false
                    lBorderwidth: 1
                    rBorderwidth: 0
                    tBorderwidth: 0
                    bBorderwidth: 0
                    borderColor: Qt.darker(page.bgGreenColor, 1.2)
                }

                state: "UNWINDING"
                states: [
                    State {name: "UNWINDING"; PropertyChanges {target: bUnwinding; text: qsTr("Unwinding")}},
                    State {name: "BRAKING_EXTENSION"; PropertyChanges {target: bUnwinding; text: qsTr("Brake")}}
                ]

                onClicked: {
                    Skypuff.sendTerminal("set %1".arg(bUnwinding.state))
                }

                Connections {
                    target: Skypuff

                    onBrakingExtensionRangeChanged: {
                        // Brake if possible
                        switch(Skypuff.state) {
                        case "MANUAL_BRAKING":
                            bUnwinding.state = isBrakingExtensionRange ? "BRAKING_EXTENSION" : "UNWINDING"
                            break
                        case "UNWINDING":
                        case "REWINDING":
                            bUnwinding.enabled = isBrakingExtensionRange
                            break
                        }
                    }
                }
            }
        }
    }


    Connections {
        target: Skypuff

        function set_manual_state_visible() {
            // Make MANUAL_BRAKING controls visible
            bSetZero.visible = true
            rManualSlowForward.visible = true
            rManualSlowBack.visible = true

            // Disable normal controls
            bPrePull.visible = false

            // Go back to UNWINDING or BRAKING_EXTENSION?
            bUnwinding.state = Skypuff.isBrakingExtensionRange ? "BRAKING_EXTENSION" : "UNWINDING"
        }

        function set_manual_state_invisible() {
            // Make MANUAL_BRAKING controls visible
            bSetZero.visible = false
            rManualSlowForward.visible = false
            rManualSlowBack.visible = false

            // Disable normal controls
            bPrePull.visible = true
            bPrePull.state = "PRE_PULL"

            // Go back to UNWINDING or BRAKING_EXTENSION?
            bUnwinding.state = Skypuff.isBrakingExtensionRange ? "BRAKING_EXTENSION" : "UNWINDING"
        }

        function onExit(state) {
            switch(state) {
            case "MANUAL_SLOW_SPEED_UP":
            case "MANUAL_SLOW_BACK_SPEED_UP":
            case "MANUAL_SLOW":
            case "MANUAL_SLOW_BACK":
            case "MANUAL_BRAKING":
                bStop.enabled = true

                set_manual_state_invisible()
                break
            case "REWINDING":
            case "UNWINDING":
                bUnwinding.enabled = true
                bUnwinding.state = "UNWINDING"
                break
            case "BRAKING":
                bPrePull.enabled = true
                break
            case "DISCONNECTED":
                bStop.enabled = true
                bUnwinding.enabled = true
                bPrePull.enabled = true
                pullForce.enabled = true
                break
            case "SLOW":
                bPrePull.enabled = true
                break
            case "FAST_PULL":
                bPrePull.enabled = true
                bPrePull.state = "PRE_PULL"
                break
            }
        }

        function onEnter(state) {
            switch(state) {
            case "MANUAL_BRAKING":
                set_manual_state_visible()
                bStop.enabled = false
                bUnwinding.enabled = !Skypuff.isPositiveTachometer
                bSetZero.enabled = true
                break
            case "MANUAL_SLOW_SPEED_UP":
            case "MANUAL_SLOW_BACK_SPEED_UP":
            case "MANUAL_SLOW":
            case "MANUAL_SLOW_BACK":
                set_manual_state_visible()
                bUnwinding.enabled = false
                bSetZero.enabled = false
                break
            case "BRAKING":
                bUnwinding.enabled = false
                bUnwinding.state = "UNWINDING"
                bPrePull.enabled = false
                bPrePull.state = "PRE_PULL"
                break
            case "BRAKING_EXTENSION":
                bUnwinding.enabled = true
                bUnwinding.state = "UNWINDING"
                break
            case "REWINDING":
            case "UNWINDING":
                bUnwinding.enabled = Skypuff.isBrakingExtensionRange
                bUnwinding.state = "BRAKING_EXTENSION"
                bPrePull.state = "PRE_PULL"
                break
            case "SLOWING":
                bUnwinding.enabled = false
                bPrePull.enabled = false
                bPrePull.state = "PRE_PULL"
                break
            case "PRE_PULL":
                bPrePull.state = "TAKEOFF_PULL"
                break
            case "TAKEOFF_PULL":
                bPrePull.state = "PULL"
                break
            case "PULL":
                bPrePull.state = "FAST_PULL"
                break
            case "FAST_PULL":
                bPrePull.enabled = false
                break
            case "DISCONNECTED":
                bStop.enabled = false
                bPrePull.enabled = false
                pullForce.enabled = false
                break
            }
        }

        onPositiveTachometerChanged: {
            if (page.state == "MANUAL_BRAKING") {
                bUnwinding.enabled = !isPositiveTachometer
            }
        }

        onStateChanged: {
            if(page.state !== newState) {
                onExit(page.state)
                onEnter(newState)
            }

            page.state = newState
        }

        onSettingsChanged: {
            pullForce.to = cfg.motor_max_kg
            pullForce.stepSize = cfg.motor_max_kg / 30
            pullForce.value = cfg.pull_kg
        }
    }
}
