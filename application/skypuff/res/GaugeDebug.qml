import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.12

// Test Gauge element

ColumnLayout {

    id: debug

    property var gauge
    property var battery
    property int fontSize: 13
    property int valFontSize: 12

    Layout.leftMargin: gauge.paddingLeft
    Layout.rightMargin: gauge.paddingRight

    spacing: -10

    width: gauge.diameter

    Layout.fillWidth: true

    function prettyNumber(number, tf = 1) {
        if (!number || !!isNaN(number)) return 0;

        if (Math.abs(number) < 1 && number !== 0 && tf === 1) {
            tf = 2;
        } else if (Number.isInteger(number) && tf === 1) {
            tf = 0;
        }

        return parseFloat(number.toFixed(tf));
    }

    GridLayout {
        columns: 3
        columnSpacing: 5
        rowSpacing: -20
        Layout.fillWidth: true

        Item {
            Layout.fillWidth: true
        }

        Text {
            text: '2000000'
            font.pixelSize: debug.fontSize
            color: 'transparent'
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: 'Rope val:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(ropeMeters.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: ropeMeters
            from: -1000
            to: 5000
            value: gauge.ropeMeters
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                gauge.ropeMeters = value;
            }
        }

        Label {
            text: 'Rope max val:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(maxRopeMeters.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: maxRopeMeters
            from: 0
            to: 5000
            value: gauge.maxRopeMeters
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                gauge.maxRopeMeters = value;
            }
        }

        Label {
            text: 'KG val:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(motorKg.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: motorKg
            from: gauge.minMotorKg
            to: 200
            value: gauge.motorKg
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                gauge.motorKg = value;
            }
        }

        Label {
            text: 'Power val:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(power.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: power
            from: -50 * 1000
            to: 50 * 1000
            value: gauge.power
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                gauge.power = value;
            }
        }

        Label {
            text: 'Speed val:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(speedMs.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: speedMs
            from: -150
            to: 150
            value: gauge.speedMs
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                gauge.speedMs = value;
            }
        }

        Label {
            text: 'Temps val:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(temp.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: temp
            from: -50
            to: 150
            value: 0
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft
            onValueChanged: {
                gauge.tempFets = value;
                gauge.tempMotor = value;
            }
        }

        Label {
            text: 'Batt val:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(batteryPercents.value)
            font.pixelSize: debug.valFontSize

        }

        Slider {
            id: batteryPercents
            from: 0
            to: 100
            value: battery.batteryPercents
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                battery.batteryPercents = value;
            }
        }

        Label {
            text: 'CellV val:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(batteryCellVolts.value)
            font.pixelSize: debug.valFontSize

        }

        Slider {
            id: batteryCellVolts
            from: 0
            to: 15
            value: battery.batteryCellVolts
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                battery.batteryCellVolts = value;
            }
        }

        Label {
            text: 'Cell Count:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(batteryCells.value)
            font.pixelSize: debug.valFontSize

        }

        Slider {
            id: batteryCells
            from: 0
            to: 100
            value: battery.batteryCells
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                battery.batteryCells = value;
            }
        }

        Label {
            text: 'Speed max:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(maxSpeedMs.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: maxSpeedMs
            from: 0
            to: 100
            value: gauge.maxSpeedMs
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                gauge.maxSpeedMs = value;
            }
        }

        Label {
            text: 'KG max:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(maxMotorKg.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: maxMotorKg
            from: 0
            to: 200
            value: gauge.maxMotorKg
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                gauge.maxMotorKg = value;
            }
        }

        Label {
            text: 'KG step:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(motorKgLabelStepSize.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: motorKgLabelStepSize
            from: 0
            to: 50
            value: gauge.motorKgLabelStepSize
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft

            onValueChanged: {
                gauge.motorKgLabelStepSize = value;
            }
        }

        Label {
            text: 'Power max:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(maxPower.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: maxPower
            from: 0
            to: 20000
            value: gauge.maxPower
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft
            stepSize: 2000

            onValueChanged: {
                gauge.maxPower = value;
            }
        }

        Label {
            text: 'Power min:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(minPower.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: minPower
            from: -20000
            to: 0
            value: gauge.minPower
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft
            stepSize: 2000

            onValueChanged: {
                gauge.minPower = value;
            }
        }

        Label {
            text: 'Power step:'
            font.pixelSize: debug.fontSize
        }

        Label {
            text: prettyNumber(powerLabelStepSize.value)
            font.pixelSize: debug.valFontSize
        }

        Slider {
            id: powerLabelStepSize
            from: 1
            to: 20
            value: gauge.powerLabelStepSize
            Layout.fillWidth: true
            Layout.rightMargin: debug.paddingLeft
            stepSize: 1

            onValueChanged: {
                gauge.powerLabelStepSize = parseInt(value, 10);
            }
        }
    }

    RowLayout {
        TextField {
            placeholderText: qsTr("StateText")
            font.pixelSize: 12
            Layout.fillWidth: true

            onTextChanged: {
                gauge.stateText = text;
            }

        }
        TextField {
            placeholderText: qsTr("Fault")
            font.pixelSize: 12
            Layout.fillWidth: true

            onTextChanged: {
                gauge.fault = text;
            }
        }
        TextField {
            placeholderText: qsTr("Status")
            font.pixelSize: 12
            Layout.fillWidth: true

            onTextChanged: {
                gauge.status = text;
            }
        }
    }


    RowLayout {
        spacing: 0
        Layout.fillWidth: true

        CheckBox {
            text: qsTr("Warning")
            checked: false
            Layout.fillWidth: true

            nextCheckState: {
                var value = checkState === Qt.Checked;
                gauge.isRopeWarning = value;
                gauge.isPowerWarning = value;
                gauge.isMotorKgWarning= value;
                gauge.isSpeedWarning = value;
                battery.isBatteryWarning = value;
            }
        }
        CheckBox {
            text: qsTr("Danger")
            checked: false
            Layout.fillWidth: true

            nextCheckState: {
                var value = checkState === Qt.Checked;
                gauge.isRopeBlinking = value;
                gauge.isPowerBlinking = value;
                gauge.isMotorKgBlinking = value;
                gauge.isSpeedBlinking = value;
                battery.isBatteryBlinking = value;
            }
        }


        Button {
            text: qsTr("Close")
            checked: true
            Layout.fillWidth: true


            onClicked: {
                debug.visible = false;
            }
        }
    }




}
