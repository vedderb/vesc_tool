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
import QtGraphicalEffects 1.0

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    property ConfigParams mMcConf: VescIf.mcConfig()
    property Commands mCommands: VescIf.commands()

    function openDialog() {
        // Set few battery cells by default to avoid confusion
        // if the motor does not spin due to battery cut.
        mMcConf.updateParamInt("si_battery_cells", 3)
        dialog.open()
        loadDefaultDialog.open()
    }

    Component.onCompleted: {
        paramListBatt.addEditorMc("si_battery_type")
        paramListBatt.addEditorMc("si_battery_cells")
        paramListBatt.addEditorMc("si_battery_ah")

        paramListSetup.addEditorMc("si_wheel_diameter")
        paramListSetup.addSeparator("↓ Only change if needed ↓")
        paramListSetup.addEditorMc("si_motor_poles")
    }

    Dialog {
        id: dialog
        modal: true
        focus: true
        width: parent.width - 10
        height: parent.height - 10
        closePolicy: Popup.NoAutoClose
        x: 5
        y: 5
        parent: ApplicationWindow.overlay
        bottomMargin: 0
        rightMargin: 0
        padding: 10

        StackLayout {
            id: stackLayout
            anchors.fill: parent

            Item {
                ColumnLayout {
                    id: motorColumn
                    anchors.fill: parent

                    ListModel {
                        id: motorModel

                        ListElement {
                            name: "Mini Outrunner (~75 g)"
                            motorImg: "qrc:/res/images/motors/outrunner_mini.jpg"
                            maxLosses: 10
                            openloopErpm: 1400
                            sensorlessErpm: 4000
                            poles: 14
                        }
                        ListElement {
                            name: "Small Outrunner (~200 g)"
                            motorImg: "qrc:/res/images/motors/outrunner_small.jpg"
                            maxLosses: 25
                            openloopErpm: 1400
                            sensorlessErpm: 4000
                            poles: 14
                        }
                        ListElement {
                            name: "Medium Outrunner (~750 g)"
                            motorImg: "qrc:/res/images/motors/6374.jpg"
                            maxLosses: 60
                            openloopErpm: 700
                            sensorlessErpm: 4000
                            poles: 14
                        }
                        ListElement {
                            name: "Large Outrunner (~2000 g)"
                            motorImg: "qrc:/res/icons/motor.png"
                            maxLosses: 200
                            openloopErpm: 700
                            sensorlessErpm: 4000
                            poles: 14
                        }
                        ListElement {
                            name: "Small Inrunner (~200 g)"
                            motorImg: "qrc:/res/images/motors/inrunner_small.jpg"
                            maxLosses: 25
                            openloopErpm: 1400
                            sensorlessErpm: 4000
                            poles: 2
                        }
                        ListElement {
                            name: "Medium Inrunner (~750 g)"
                            motorImg: "qrc:/res/images/motors/inrunner_medium.jpg"
                            maxLosses: 70
                            openloopErpm: 1400
                            sensorlessErpm: 4000
                            poles: 4
                        }
                        ListElement {
                            name: "Large Inrunner (~2000 g)"
                            motorImg: "qrc:/res/icons/motor.png"
                            maxLosses: 200
                            openloopErpm: 1000
                            sensorlessErpm: 4000
                            poles: 4
                        }
                        ListElement {
                            name: "E-Bike DD hub motor (~6 kg)"
                            motorImg: "qrc:/res/images/motors/ebike_dd_1kw.jpg"
                            maxLosses: 75
                            openloopErpm: 300
                            sensorlessErpm: 2000
                            poles: 46
                        }
                        ListElement {
                            name: "EDF Inrunner Small (~200 g)"
                            motorImg: "qrc:/res/images/motors/edf_small.jpg"
                            maxLosses: 55
                            openloopErpm: 1400
                            sensorlessErpm: 4000
                            poles: 6
                        }
                    }

                    ListView {
                        id: motorList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        focus: true
                        clip: true
                        spacing: 5

                        Component {
                            id: motorDelegate

                            Rectangle {
                                id: imgRect
                                property variant modelData: model

                                width: motorList.width
                                height: 90
                                color: ListView.isCurrentItem ? "#41418f" : "#30000000"
                                radius: 5
                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10

                                    Item {
                                        Layout.preferredWidth: 80
                                        Layout.preferredHeight: 80
                                        Layout.leftMargin: 5
                                        opacity: imgRect.ListView.isCurrentItem ? 1.0 : 0.5

                                        Image {
                                            id: image
                                            fillMode: Image.PreserveAspectFit
                                            source: motorImg
                                            width: 80
                                            height: 80
                                            smooth: true
                                            visible: false
                                        }

                                        Rectangle {
                                            id: mask
                                            width: 80
                                            height: 80
                                            radius: 40
                                            visible: false
                                        }

                                        OpacityMask {
                                            anchors.fill: image
                                            source: image
                                            maskSource: mask
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: name
                                        color: "white"
                                        wrapMode: Text.WordWrap
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onPressed: {
                                        motorList.currentIndex = index
                                        motorList.focus = true
                                    }
                                }
                            }
                        }

                        model: motorModel
                        delegate: motorDelegate

                        onCurrentIndexChanged: {
                            maxPowerLossBox.realValue = motorList.currentItem.modelData.maxLosses
                            openloopErpmBox.realValue = motorList.currentItem.modelData.openloopErpm
                            sensorlessBox.realValue = motorList.currentItem.modelData.sensorlessErpm
                            motorPolesBox.realValue = motorList.currentItem.modelData.poles
                        }
                    }

                    GroupBox {
                        Layout.fillWidth: true

                        label: CheckBox {
                            id: overrideBox
                            checked: false
                            text: qsTr("Override (Advanced)")

                            onToggled: {
                                if (!checked) {
                                    maxPowerLossBox.realValue = motorList.currentItem.modelData.maxLosses
                                    openloopErpmBox.realValue = motorList.currentItem.modelData.openloopErpm
                                    sensorlessBox.realValue = motorList.currentItem.modelData.sensorlessErpm
                                    motorPolesBox.realValue = motorList.currentItem.modelData.poles
                                }
                            }
                        }

                        ColumnLayout {
                            anchors.fill: parent

                            Text {
                                visible: !overrideBox.checked
                                color: "white"
                                font.family: "DejaVu Sans Mono"
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                text: maxPowerLossBox.prefix + maxPowerLossBox.realValue + maxPowerLossBox.suffix + "\n" +
                                      openloopErpmBox.prefix + openloopErpmBox.realValue + openloopErpmBox.suffix + "\n" +
                                      sensorlessBox.prefix + sensorlessBox.realValue + sensorlessBox.suffix + "\n" +
                                      motorPolesBox.prefix + motorPolesBox.realValue + motorPolesBox.suffix
                            }

                            DoubleSpinBox {
                                visible: overrideBox.checked
                                Layout.fillWidth: true
                                id: maxPowerLossBox
                                decimals: 1
                                realFrom: 0
                                realTo: 5000
                                realValue: 10
                                prefix: "Max Power Loss : "
                                suffix: " W"
                            }

                            DoubleSpinBox {
                                visible: overrideBox.checked
                                Layout.fillWidth: true
                                id: openloopErpmBox
                                decimals: 0
                                realFrom: 0
                                realTo: 50000
                                realValue: 500
                                prefix: "Openloop ERPM  : "
                            }

                            DoubleSpinBox {
                                visible: overrideBox.checked
                                Layout.fillWidth: true
                                id: sensorlessBox
                                decimals: 0
                                realFrom: 0
                                realTo: 50000
                                realValue: 1500
                                prefix: "Sensorless ERPM: "
                            }

                            DoubleSpinBox {
                                visible: overrideBox.checked
                                Layout.fillWidth: true
                                id: motorPolesBox
                                decimals: 0
                                realFrom: 2
                                realTo: 512
                                realValue: 2
                                realStepSize: 2
                                prefix: "Motor Poles    : "

                                onRealValueChanged: {
                                    mMcConf.updateParamInt("si_motor_poles", realValue, null)
                                }
                            }
                        }
                    }
                }
            }

            Item {
                ScrollView {
                    anchors.fill: parent
                    contentWidth: parent.width
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent

                        ParamList {
                            id: paramListBatt
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }

                        GroupBox {
                            Layout.fillWidth: true

                            label: CheckBox {
                                id: overrideBattBox
                                checked: false
                                text: qsTr("Advanced (0 = defaults)")

                                onToggled: {
                                    if (!checked) {
                                        currentInMinBox.realValue = 0
                                        currentInMaxBox.realValue = 0
                                    }
                                }
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                enabled: overrideBattBox.checked

                                DoubleSpinBox {
                                    Layout.fillWidth: true
                                    id: currentInMinBox
                                    decimals: 1
                                    realStepSize: 5
                                    realFrom: 0
                                    realTo: -9999
                                    realValue: 0
                                    prefix: "Battery Current Regen: "
                                    suffix: " A"
                                }

                                DoubleSpinBox {
                                    Layout.fillWidth: true
                                    id: currentInMaxBox
                                    decimals: 1
                                    realStepSize: 5
                                    realFrom: 0
                                    realTo: 9999
                                    realValue: 0
                                    prefix: "Battery Current Max: "
                                    suffix: " A"
                                }
                            }
                        }
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentWidth: parent.width
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent

                            GroupBox {
                                id: canFwdBox
                                title: qsTr("Gear Ratio")
                                Layout.fillWidth: true

                                ColumnLayout {
                                    anchors.fill: parent

                                    CheckBox {
                                        id: directDriveBox
                                        text: "Direct Drive"
                                        Layout.fillWidth: true
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            Layout.fillWidth: true
                                            color: "white"
                                            text: qsTr("Motor Pulley")
                                        }

                                        SpinBox {
                                            enabled: !directDriveBox.checked
                                            id: motorPulleyBox
                                            from: 1
                                            to: 999
                                            value: 13
                                            editable: true

                                            textFromValue: function(value) {
                                                return !directDriveBox.checked ? value : 1
                                            }
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            Layout.fillWidth: true
                                            color: "white"
                                            text: qsTr("Wheel Pulley")
                                        }

                                        SpinBox {
                                            enabled: !directDriveBox.checked
                                            id: wheelPulleyBox
                                            from: 1
                                            to: 999
                                            value: 36
                                            editable: true

                                            textFromValue: function(value) {
                                                return !directDriveBox.checked ? value : 1
                                            }
                                        }
                                    }
                                }
                            }

                            ParamList {
                                id: paramListSetup
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            Item {
                DirectionSetup {
                    id: dirSetup
                    anchors.fill: parent
                }
            }
        }

        header: Rectangle {
            color: "#dbdbdb"
            height: tabBar.height

            TabBar {
                id: tabBar
                currentIndex: stackLayout.currentIndex
                anchors.fill: parent
                implicitWidth: 0
                clip: true
                enabled: false

                background: Rectangle {
                    opacity: 1
                    color: "#4f4f4f"
                }

                property int buttons: 4
                property int buttonWidth: 120

                TabButton {
                    text: qsTr("Motor")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("Battery")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("Setup")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("Direction")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
            }
        }

        footer: RowLayout {
            spacing: 0
            Button {
                id: prevButton
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: "Cancel"
                flat: true

                onClicked: {
                    if (stackLayout.currentIndex == 0) {
                        dialog.close()
                    } else {
                        stackLayout.currentIndex--
                    }

                    updateButtonText()
                }
            }

            Button {
                id: nextButton
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: "Next"
                flat: true

                onClicked: {
                    if (stackLayout.currentIndex == 0) {
                        startWarningDialog.open()
                    } else if (stackLayout.currentIndex == 1) {
                        if (overrideBattBox.checked) {
                            stackLayout.currentIndex++
                        } else {
                            batteryWarningDialog.open()
                        }
                    } else if (stackLayout.currentIndex == 2) {
                        if (stackLayout.currentIndex == (stackLayout.count - 2)) {
                            if (VescIf.isPortConnected()) {
                                detectDialog.open()
                            } else {
                                VescIf.emitMessageDialog("Detect Motors",
                                                         "Not connected to the VESC. Please connect in order to run detection.",
                                                         false, false)
                            }
                        }
                    } else if (stackLayout.currentIndex == 3) {
                        stackLayout.currentIndex = 0
                        dialog.close()
                    }

                    updateButtonText()
                }
            }
        }
    }

    Dialog {
        id: loadDefaultDialog
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Load Default Parameters"
        parent: ApplicationWindow.overlay

        x: 10
        y: dialog.y + dialog.height / 2 - height / 2

        Text {
            color: "#ffffff"
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "Would you like to restore this VESC, and all VESCs on the CAN-bus (if any), " +
                  "to their default settings before proceeding?"
        }

        onAccepted: {
            disableDialog()
            Utility.restoreConfAll(VescIf, true, true, true)
            enableDialog()
        }
    }

    Dialog {
        id: startWarningDialog
        property int indexNow: 0
        standardButtons: Dialog.Yes | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Motor Selection"
        x: 10
        y: 10 + parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        Text {
            color: "#ffffff"
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "Warning: Choosing a significantly too large motor for detection is likely to destroy the motor " +
                  "during detection. It is important to choose a motor size similar to the motor you are using. " +
                  "Are you sure that your motor selection is within range?"
        }

        onAccepted: {
            stackLayout.currentIndex++
            updateButtonText()
        }
    }

    Dialog {
        id: batteryWarningDialog
        property int indexNow: 0
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Battery Settings"
        x: 10
        y: 10 + parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        Text {
            color: "#ffffff"
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "Warning: You have not specified battery current limits, which essentially only limits " +
                  "the current if the voltage drops too much. This is fine in most cases, but check with " +
                  "your battery and BMS specification to be safe. Keep in mind that you have to divide the " +
                  "battery current settings by the number of VESCs."
        }

        onAccepted: {
            stackLayout.currentIndex++
            updateButtonText()
        }
    }

    Dialog {
        id: detectDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Detect FOC Parameters"
        parent: ApplicationWindow.overlay

        x: 10
        y: dialog.y + dialog.height / 2 - height / 2

        Text {
            color: "#ffffff"
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "This is going to spin up all motors. Make " +
                  "sure that nothing is in the way."
        }

        onAccepted: {
            disableDialog()

            mMcConf.updateParamDouble("si_gear_ratio", directDriveBox.checked ?
                                          1 : (wheelPulleyBox.value / motorPulleyBox.value), 0)

            mCommands.setMcconf(false)
            Utility.waitSignal(mCommands, "2ackReceived(QString)", 2000)
            var canDevs = VescIf.scanCan();
            if (!Utility.setBatteryCutCan(VescIf, canDevs, 6.0, 6.0)) {
                enableDialog()
                return
            }

            var res  = Utility.detectAllFoc(VescIf, true,
                                            maxPowerLossBox.realValue,
                                            currentInMinBox.realValue,
                                            currentInMaxBox.realValue,
                                            openloopErpmBox.realValue,
                                            sensorlessBox.realValue)

            var resDetect = false
            if (res.startsWith("Success!")) {
                resDetect = true
                Utility.setBatteryCutCanFromCurrentConfig(VescIf, canDevs);
            }

            enableDialog()

            if (resDetect) {
                stackLayout.currentIndex++
                updateButtonText()
            }

            resultDialog.title = "Detection Result"
            resultLabel.text = res
            resultDialog.open()
        }
    }

    Dialog {
        id: resultDialog
        standardButtons: Dialog.Ok
        modal: true
        focus: true
        width: parent.width - 20
        height: parent.height - 40
        closePolicy: Popup.CloseOnEscape
        parent: ApplicationWindow.overlay

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        ScrollView {
            anchors.fill: parent
            clip: true
            contentWidth: parent.width - 20

            Text {
                id: resultLabel
                color: "#ffffff"
                font.family: "DejaVu Sans Mono"
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
                wrapMode: Text.WordWrap
            }
        }

        onClosed: {
            if (stackLayout.currentIndex == (stackLayout.count - 1)) {
                workaroundNotFocusTimer.start()
            }
        }
    }

    Timer {
        id: workaroundNotFocusTimer
        interval: 0
        repeat: false
        running: false

        onTriggered: {
            dirSetup.scanCan()
        }
    }

    function updateButtonText() {
        if (stackLayout.currentIndex == (stackLayout.count - 1)) {
            nextButton.text = "Finish"
        } else if (stackLayout.currentIndex == (stackLayout.count - 2)) {
            nextButton.text = "Run Detection"
        } else {
            nextButton.text = "Next"
        }

        if (stackLayout.currentIndex == 0) {
            prevButton.text = "Cancel"
        } else {
            prevButton.text = "Previous"
        }
    }

    function disableDialog() {
        commDialog.open()
        stackLayout.enabled = false
        prevButton.enabled = false
        nextButton.enabled = false
    }

    function enableDialog() {
        commDialog.close()
        stackLayout.enabled = true
        prevButton.enabled = true
        nextButton.enabled = true
    }

    Dialog {
        id: commDialog
        title: "Processing..."
        closePolicy: Popup.NoAutoClose
        modal: true
        focus: true

        width: parent.width - 20
        x: 10
        y: parent.height / 2 - height / 2
        parent: ApplicationWindow.overlay

        ProgressBar {
            anchors.fill: parent
            indeterminate: visible
        }
    }
}
