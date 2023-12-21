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
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: topItem

    property ConfigParams mMcConf: VescIf.mcConfig()
    property Commands mCommands: VescIf.commands()
    property var dialogParent: ApplicationWindow.overlay
    property var canDevs: []

    signal dialogClosed()

    function openDialog() {
        dialog.open()
        openTimer.start()
        tabBar.currentIndex = 0
    }

    Timer {
        id: openTimer

        running: false
        interval: 100
        repeat: false

        onTriggered: {
            disableDialog()
            canDevs = Utility.scanCanVescOnly(VescIf)
            enableDialog()

            if (!Utility.isConnectedToHwVesc(VescIf)) {
                if (canDevs.length > 0) {
                    mCommands.setSendCan(true, canDevs[0])
                }
            }

            updateUsageListParams()
            loadDefaultDialog.open()
        }
    }

    function updateUsageListParams() {
        mMcConf.updateParamDouble("l_duty_start", usageList.currentItem.modelData.duty_start, null)
        mMcConf.updateParamInt("m_fault_stop_time_ms", usageList.currentItem.modelData.fault_stop_ms, null)
        mMcConf.updateParamInt("bms.limit_mode", usageList.currentItem.modelData.bms_limit_mode, null)
    }

    Component.onCompleted: {
        paramListUsage.addEditorMc("bms.limit_mode")
        paramListUsage.addEditorMc("l_duty_start")
        paramListUsage.addEditorMc("m_fault_stop_time_ms")

        paramListBatt.addEditorMc("si_battery_type")
        paramListBatt.addEditorMc("si_battery_cells")
        paramListBatt.addEditorMc("si_battery_ah")

        paramListSetup.addEditorMc("si_wheel_diameter")
        paramListSetup.addSeparator("↓ Only change if needed ↓")
        paramListSetup.addEditorMc("si_motor_poles")
        paramListSetup.addEditorMc("m_motor_temp_sens_type")
        paramListSetup.addEditorMc("m_ntc_motor_beta")
    }

    Dialog {
        id: dialog
        modal: true
        focus: true
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
            id: stackLayout
            anchors.fill: parent
            onCurrentIndexChanged: {tabBar.currentIndex = currentIndex}

            Item {
                ListModel {
                    id: usageModel
                    property string iconPath: {iconPath = "qrc" + Utility.getThemePath() + "icons/"}
                    Component.onCompleted: {
                        append({name: "Generic", usageImg:iconPath + "motor.png",
                                   duty_start: 1.0, hfi_start: false, fault_stop_ms: 500,
                                   bms_limit_mode: 3, batt_cut_cautious: true})
                        append({name: "E-Skate", usageImg:"qrc:/res/images/esk8.jpg",
                                   duty_start: 0.85, hfi_start: true, fault_stop_ms: 50,
                                   bms_limit_mode: 3, batt_cut_cautious: true})
                        append({name: "Balance", usageImg:iconPath + "EUC-96.png",
                                   duty_start: 1.0, hfi_start: false, fault_stop_ms: 50,
                                   bms_limit_mode: 0, batt_cut_cautious: false})
                        append({name: "Propeller", usageImg:"qrc:/res/images/propeller.jpg",
                                   duty_start: 1.0, hfi_start: false, fault_stop_ms: 500,
                                   bms_limit_mode: 3, batt_cut_cautious: true})
                    }
                }

                GridLayout {
                    id: usageColumn
                    anchors.fill: parent
                    columns: dialog.isHorizontal ? 2 : 1
                    rows: dialog.isHorizontal ? 1 : 2
                    ListView {
                        id: usageList
                        focus: true
                        clip: true
                        spacing: 5
                        Layout.row: 0
                        Layout.fillHeight: true
                        Layout.preferredWidth:  dialog.isHorizontal ? parent.width/2 : parent.width
                        Layout.column: dialog.isHorizontal ? 1 : 0

                        Component {
                            id: usageDelegate

                            Rectangle {
                                id: imgRect2
                                property variant modelData: model

                                width: usageList.width
                                height: 90
                                color: ListView.isCurrentItem ? Utility.getAppHexColor("lightAccent") : Utility.getAppHexColor("normalBackground")
                                radius: 5
                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10

                                    Item {
                                        Layout.preferredWidth: 80
                                        Layout.preferredHeight: 80
                                        Layout.leftMargin: 5
                                        opacity: imgRect2.ListView.isCurrentItem ? 1.0 : 0.5

                                        Image {
                                            id: image2
                                            fillMode: Image.PreserveAspectFit
                                            source: usageImg
                                            width: 80
                                            height: 80
                                            smooth: true
                                            mipmap: false
                                            visible: false
                                        }

                                        Rectangle {
                                            id: mask2
                                            width: 80
                                            height: 80
                                            radius: 40
                                            visible: false
                                        }

                                        OpacityMask {
                                            anchors.fill: image2
                                            source: image2
                                            maskSource: mask2
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: name
                                        color: Utility.getAppHexColor("lightText")
                                        wrapMode: Text.WordWrap
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onPressed: {
                                        usageList.currentIndex = index
                                        usageList.focus = true
                                    }
                                }
                            }
                        }

                        model: usageModel
                        delegate: usageDelegate

                        onCurrentIndexChanged: {
                            updateUsageListParams()
                        }
                    }

                    GroupBox {
                        Layout.preferredHeight: dialog.isHorizontal ? implicitHeight : parent.height * 0.5
                        Layout.fillWidth: true
                        Layout.column: 0
                        Layout.row: dialog.isHorizontal ? 0 : 1
                        Layout.alignment: Qt.AlignTop
                        contentWidth: dialog.isHorizontal ? parent.width/2 : parent.width

                        label: CheckBox {
                            id: overrideUsageBox
                            checked: false
                            text: qsTr("Override (Advanced)")

                            onToggled: {
                                if (!checked) {
                                    currentInMinBox.realValue = 0
                                    currentInMaxBox.realValue = 0
                                }
                            }
                        }

                        ScrollView {
                            anchors.fill: parent
                            clip: true

                            ParamList {
                                id: paramListUsage
                                enabled: overrideUsageBox.checked
                                anchors.fill: parent
                            }
                        }
                    }
                }
            }

            Item {

                ListModel {
                    id: motorModel
                    property string iconPath: {iconPath = "qrc" + Utility.getThemePath() + "icons/"}
                    Component.onCompleted: {
                        [
                        ["Mini Outrunner (~75 g)", "qrc:/res/images/motors/outrunner_mini.jpg", 20, 1400, 4000, false,14],
                        ["Small Outrunner (~200 g)","qrc:/res/images/motors/outrunner_small.jpg", 50, 1400, 4000, false,14],
                        ["Medium Outrunner (~750 g)","qrc:/res/images/motors/6374.jpg", 120, 700, 4000, true, 14],
                        ["Large Outrunner (~2000 g)",iconPath + "motor.png", 400, 700, 4000, false, 14],
                        ["Small Inrunner (~200 g)","qrc:/res/images/motors/inrunner_small.jpg", 50, 1400, 4000, false, 2],
                        ["Medium Inrunner (~750 g)","qrc:/res/images/motors/inrunner_medium.jpg", 140, 1400, 4000, false, 4],
                        ["Large Inrunner (~2000 g)",iconPath + "motor.png", 400, 1000, 4000, false, 4],
                        ["E-Bike DD hub motor (~6 kg)","qrc:/res/images/motors/ebike_dd_1kw.jpg", 150, 300, 2000, false, 46],
                        ["EDF Inrunner Small (~200 g)","qrc:/res/images/motors/edf_small.jpg", 110, 1400, 4000, false, 6]
                        ].forEach(function(element) {
                            append({
                                       name: element[0],
                                       motorImg: element[1],
                                       maxLosses: element[2],
                                       openloopErpm: element[3],
                                       sensorlessErpm: element[4],
                                       hfi_start: element[5],
                                       poles: element[6]
                                   });
                        });
                    }
                }

                GridLayout {
                    id: motorColumn
                    anchors.fill: parent
                    columns: dialog.isHorizontal ? 2 : 1
                    rows: dialog.isHorizontal ? 1 : 2

                    ListView {
                        id: motorList
                        focus: true
                        clip: true
                        spacing: 5
                        Layout.row: 0
                        Layout.fillHeight: true
                        Layout.preferredWidth:  dialog.isHorizontal ? parent.width/2 : parent.width
                        Layout.column: dialog.isHorizontal ? 1 : 0

                        Component {
                            id: motorDelegate

                            Rectangle {
                                id: imgRect
                                property variant modelData: model

                                width: motorList.width
                                height: 90
                                color: ListView.isCurrentItem ? Utility.getAppHexColor("lightAccent") : Utility.getAppHexColor("normalBackground")
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
                                            mipmap: false
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
                                        color: Utility.getAppHexColor("lightText")
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
                        Layout.preferredHeight: implicitHeight
                        Layout.fillWidth: true
                        Layout.column: 0
                        Layout.row: dialog.isHorizontal ? 0 : 1
                        contentWidth: dialog.isHorizontal ? parent.width/2 : parent.width

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
                                color: {color = Utility.getAppHexColor("lightText")}
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
                GridLayout {
                    anchors.fill: parent
                    columns: dialog.isHorizontal ? 2 : 1
                    rows: dialog.isHorizontal ? 1 : 2

                    ScrollView {
                        Layout.row: 0
                        Layout.fillHeight: true
                        Layout.preferredWidth:  dialog.isHorizontal ? parent.width/2 : parent.width
                        Layout.column: dialog.isHorizontal ? 1 : 0
                        contentWidth: dialog.isHorizontal ? parent.width/2 : parent.width
                        clip: true

                        ParamList {
                            id: paramListBatt
                            anchors.fill: parent
                        }
                    }

                    GroupBox {
                        Layout.preferredHeight: implicitHeight
                        Layout.fillWidth: true
                        Layout.column: 0
                        Layout.row: dialog.isHorizontal ? 0 : 1
                        contentWidth: dialog.isHorizontal ? parent.width/2 : parent.width

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

            Item {
                GridLayout {
                    anchors.fill: parent
                    columns: dialog.isHorizontal ? 2 : 1
                    GroupBox {
                        id: gearRatioBox
                        title: qsTr("Gear Ratio")
                        Layout.preferredHeight: implicitHeight
                        Layout.fillWidth: true
                        Layout.column: 0
                        Layout.row: 0
                        contentWidth: dialog.isHorizontal ? parent.width/2 : parent.width

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
                                    color: {color = Utility.getAppHexColor("lightText")}
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
                                    color: {color = Utility.getAppHexColor("lightText")}
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
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.row: dialog.isHorizontal ? 0 : 1
                        Layout.column: dialog.isHorizontal ? 1 : 0
                        Layout.preferredWidth:  dialog.isHorizontal ? parent.width/2 : parent.width
                        contentWidth: availableWidth
                        clip: true
                        ParamList {
                            id: paramListSetup
                            anchors.fill: parent

                        }
                    }
                }
            }

            Item {
                DirectionSetup {
                    id: dirSetup
                    anchors.fill: parent
                    dialogParent: topItem.dialogParent
                }
            }
        }

        header: Rectangle {
            color: {color = Utility.getAppHexColor("lightText")}
            height: tabBar.implicitHeight

            TabBar {
                id: tabBar
                currentIndex: 0
                anchors.fill: parent
                clip: true
                enabled: false

                background: Rectangle {
                    opacity: 1
                    color: {color = Utility.getAppHexColor("lightBackground")}
                }
                property int buttons: 5
                property int buttonWidth: 120

                Repeater {
                    model: ["Usage", "Motor", "Battery", "Setup", "Direction"]
                    TabButton {
                        text: modelData
                        width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                    }
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
                        dialogClosed()
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
                        stackLayout.currentIndex++
                    } else if (stackLayout.currentIndex == 1) {
                        startWarningDialog.open()
                    } else if (stackLayout.currentIndex == 2) {
                        if (overrideBattBox.checked) {
                            stackLayout.currentIndex++
                        } else {
                            batteryWarningDialog.open()
                        }
                    } else if (stackLayout.currentIndex == 3) {
                        if (stackLayout.currentIndex == (stackLayout.count - 2)) {
                            if (VescIf.isPortConnected()) {
                                detectDialog.open()
                            } else {
                                VescIf.emitMessageDialog("Detect Motors",
                                                         "Not connected to the VESC. Please connect in order to run detection.",
                                                         false, false)
                            }
                        }
                    } else if (stackLayout.currentIndex == 4) {
                        stackLayout.currentIndex = 0
                        dialog.close()
                        dialogClosed()
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
        rightMargin: 10 + notchRight
        leftMargin: 10 + notchLeft
        closePolicy: Popup.CloseOnEscape
        title: "Load Default Parameters"
        parent: dialogParent
        width: parent.width - (rightMargin + leftMargin)
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        y: dialog.y + dialog.height / 2 - height / 2

        Text {
            color: {color = Utility.getAppHexColor("lightText")}
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "Would you like to restore this VESC, and all VESCs on the CAN-bus (if any), " +
                  "to their default settings before proceeding?"
        }

        onAccepted: {
            disableDialog()
            workaroundTimerLoadDefaultDialog.start()
        }
        Timer {
            id: workaroundTimerLoadDefaultDialog
            interval: 0
            repeat: false
            running: false
            onTriggered: {
                Utility.restoreConfAll(VescIf, true, true, true)
                enableDialog()
            }
        }
    }

    Dialog {
        id: startWarningDialog
        property int indexNow: 0
        standardButtons: Dialog.Yes | Dialog.Cancel
        modal: true
        focus: true
        rightMargin: 10 + notchRight
        leftMargin: 10 + notchLeft
        closePolicy: Popup.CloseOnEscape
        title: "Motor Selection"
        y: 10 + parent.height / 2 - height / 2
        parent: dialogParent
        width: parent.width - (rightMargin + leftMargin)
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        Text {
            color: {color = Utility.getAppHexColor("lightText")}
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "Warning: This selection determines the recommended max current for your motor based on heat dissipation. " +
                  "Selecting a motor here that is significantly larger than your motor is likely to destroy your motor " +
                  "during detection. It is important to choose a motor size similar to the motor you are using. " +
                  "Are you sure that your motor selection is within range?"
        }

        onAccepted: {
            stackLayout.currentIndex++
            updateButtonText()

            // Check if a 12s battery seems to be used and set the default number of cells accordingly.
            // The reason is that this is one of the most common configurations and several people
            // have damaged their battery by forgetting to set this number properly and overdischarge
            // their battery.
            disableDialog()
            workaroundTimerStartWarningDialog.start()
        }
        Timer {
            id: workaroundTimerStartWarningDialog
            interval: 0
            repeat: false
            running: false
            onTriggered: {
                var val = Utility.getMcValuesBlocking(VescIf)
                enableDialog()
                if (val.v_in > 39.0 && val.v_in < 51.0) {
                    mMcConf.updateParamInt("si_battery_cells", 12)
                } else {
                    // Roughly work out by dividing by 4.2 (most common max voltage) and ceil
                    // This will give the minimum cell count
                    var cells = Math.ceil(val.v_in / 4.2)
                    mMcConf.updateParamInt("si_battery_cells", cells)
                }
            }
        }
    }

    Dialog {
        id: batteryWarningDialog
        property int indexNow: 0
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        rightMargin: 10 + notchRight
        leftMargin: 10 + notchLeft
        closePolicy: Popup.CloseOnEscape
        title: "Battery Settings"
        y: 10 + parent.height / 2 - height / 2
        parent: dialogParent
        width: parent.width - (rightMargin + leftMargin)
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        Text {
            color: {color = Utility.getAppHexColor("lightText")}
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: "Warning: You have not specified battery current limits (advanced box). This means that " +
                  "the battery current will only be limited if, the battery voltage drops so much from the " +
                  "internal resistance of the battery that it falls below the battery cutoff voltage. " +
                  "This is fine in most cases, but check with your battery and BMS specification to be safe. " +
                  "Keep in mind that you have to divide the battery current settings by the number of VESCs."
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
        rightMargin: 10 + notchRight
        leftMargin: 10 + notchLeft
        closePolicy: Popup.CloseOnEscape
        title: "Detect FOC Parameters"
        parent: dialogParent
        width: parent.width - (rightMargin + leftMargin)

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        y: dialog.y + dialog.height / 2 - height / 2

        ColumnLayout {
            anchors.fill: parent

            Text {
                Layout.fillWidth: true
                color: {color = Utility.getAppHexColor("lightText")}
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text: "WARNING: This is going to spin up all motors. Make " +
                      "sure that nothing is in the way."
            }

            CheckBox {
                id: detectCanBox
                checked: true
                text: "Detect all motors over CAN Bus"
            }
        }

        onAccepted: {
            disableDialog()

            mMcConf.updateParamDouble("si_gear_ratio", directDriveBox.checked ?
                                          1 : (wheelPulleyBox.value / motorPulleyBox.value), null)
            mCommands.setMcconf(false)
            workaroundTimerDetectDialog.start()
        }
        Timer {
            id: workaroundTimerDetectDialog
            interval: 0
            repeat: false
            running: false

            onTriggered: {
                Utility.waitSignal(mCommands, "2ackReceived(QString)", 4000)

                if (!Utility.setBatteryCutCan(VescIf, canDevs, 6.0, 6.0)) {
                    enableDialog()
                    return
                }

                Utility.setMcParamsFromCurrentConfigAllCan(VescIf, canDevs, ["m_motor_temp_sens_type", "m_ntc_motor_beta"])

                var res  = Utility.detectAllFoc(VescIf, detectCanBox.checked,
                                                maxPowerLossBox.realValue,
                                                currentInMinBox.realValue,
                                                currentInMaxBox.realValue,
                                                openloopErpmBox.realValue,
                                                sensorlessBox.realValue)

                var resDetect = false
                if (res.startsWith("Success!")) {
                    resDetect = true

                    Utility.setBatteryCutCanFromCurrentConfig(VescIf, canDevs, usageList.currentItem.modelData.batt_cut_cautious)

                    var updateAllParams = ["l_duty_start"]

                    // Temperature compensation means that the motor can be tracked at lower
                    // speed across a broader temperature range. Therefore openloop_erpm
                    // can be decreased.
                    if (mMcConf.getParamBool("foc_temp_comp")) {
                        var openloopErpm = mMcConf.getParamDouble("foc_openloop_rpm")
                        mMcConf.updateParamDouble("foc_openloop_rpm", openloopErpm / 2.0, null)
                        updateAllParams.push("foc_openloop_rpm")
                    }

                    // Set sensor mode to HFI start if the motor is sensorless, the firmware supports it
                    // and the motor and application suggest it.
                    if (mMcConf.getParamEnumNames("foc_sensor_mode").length >= 5 &&
                            mMcConf.getParamEnum("foc_sensor_mode") === 0 &&
                            usageList.currentItem.modelData.hfi_start &&
                            motorList.currentItem.modelData.hfi_start) {
                        mMcConf.updateParamEnum("foc_sensor_mode", 4, null)
                        updateAllParams.push("foc_sensor_mode")
                    }

                    Utility.setMcParamsFromCurrentConfigAllCan(VescIf, canDevs, updateAllParams)
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
    }

    Dialog {
        id: resultDialog
        standardButtons: Dialog.Ok
        modal: true
        focus: true
        width: parent.width - 20 - notchLeft - notchRight
        height: parent.height - 40
        closePolicy: Popup.CloseOnEscape
        parent: dialogParent
        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        ScrollView {
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth

            Text {
                id: resultLabel
                color: {color = Utility.getAppHexColor("lightText")}
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
            dirSetup.setCanDevs(canDevs)
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

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 20 - notchLeft - notchRight
        x: parent.width / 2 - width / 2
        y: parent.height / 2 - height / 2
        parent: dialogParent

        ProgressBar {
            anchors.fill: parent
            indeterminate: visible
        }
    }
}
