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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import QtQuick.Controls.Material 2.2

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: topItem

    property ConfigParams mMcConf: VescIf.mcConfig()
    property Commands mCommands: VescIf.commands()
    property var dialogParent: Overlay.overlay
    property bool isHorizontal: width > height
    property var canDevs: []
    property int battCells: mMcConf.getParamInt("si_battery_cells")
    property string detectResult: ""
    property bool detecting: false
    // Title and hint per step, the direction step has its hint in DirectionSetup
    readonly property var stepHints: [
        [qsTr("What will the motor drive?"),
         qsTr("Tunes fault handling, battery limits and other defaults for the application.")],
        [qsTr("Pick the motor closest in size to yours"),
         qsTr("Its size sets the current limits. Picking a much larger motor can damage yours during detection.")],
        [qsTr("Describe your battery"),
         qsTr("Used for the voltage cutoffs. The cell count is estimated from the measured voltage, check that it matches your pack.")],
        [qsTr("Describe the drivetrain"),
         qsTr("Used for speed and distance. Run Detection then measures the motors.")],
        [qsTr("Check the motor direction"), ""]
    ]

    signal dialogClosed()

    function openDialog() {
        dialog.open()
        openTimer.start()
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
        mMcConf.updateParamDouble("l_duty_start", usageModel.get(usageList.currentIndex).duty_start, null)
        mMcConf.updateParamInt("m_fault_stop_time_ms", usageModel.get(usageList.currentIndex).fault_stop_ms, null)
        mMcConf.updateParamInt("bms.limit_mode", usageModel.get(usageList.currentIndex).bms_limit_mode, null)
    }

    function stepSummaries() {
        return [usageModel.get(usageList.currentIndex).name,
                motorModel.get(motorList.currentIndex).name + (overrideBox.checked ? qsTr(" (custom)") : ""),
                battCells + "S " + mMcConf.getParamDouble("si_battery_ah").toFixed(1) + " Ah",
                (directDriveBox.checked ? qsTr("Direct") : motorPulleyBox.value + ":" + wheelPulleyBox.value) + ", " +
                (mMcConf.getParamDouble("si_wheel_diameter") * 1000).toFixed(0) + " mm"]
    }

    Connections {
        target: mMcConf

        function onParamChangedInt(src, name, newParam) {
            if (name === "si_battery_cells") {
                battCells = newParam
            }
        }

        function onUpdated() {
            battCells = mMcConf.getParamInt("si_battery_cells")
        }
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
        property bool isHorizontal: topItem.isHorizontal
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
        padding: 10
        Overlay.modal: Rectangle {
            color: scrimColor
        }

        background: Rectangle {
            color: Utility.getAppHexColor("darkBackground")
            radius: 20
        }

        StackLayout {
            id: stackLayout
            anchors.fill: parent

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
                    GridView {
                        id: usageList
                        focus: true
                        clip: true
                        Layout.row: 0
                        Layout.fillHeight: true
                        Layout.preferredWidth:  dialog.isHorizontal ? parent.width/2 : parent.width
                        Layout.column: dialog.isHorizontal ? 1 : 0
                        currentIndex: 0
                        cellWidth: width / 2
                        // All four fit beside the options on landscape phones
                        cellHeight: phoneLandscape ? height / 2 : 170

                        Component {
                            id: usageDelegate

                            Item {
                                property bool selected: GridView.isCurrentItem
                                width: usageList.cellWidth
                                height: usageList.cellHeight

                                Rectangle {
                                    id: card
                                    anchors.fill: parent
                                    anchors.margins: 5
                                    radius: 12
                                    color: Utility.getAppHexColor("cardBackground")
                                    border.color: Utility.getAppHexColor("raisedBackground")

                                    Rectangle {
                                        id: imgContent
                                        width: parent.width
                                        height: parent.height - 44
                                        visible: false
                                        color: Utility.getAppHexColor("raisedBackground")

                                        Image {
                                            anchors.fill: parent
                                            anchors.margins: usageImg.endsWith(".jpg") ? 0 : phoneLandscape ? 10 : 26
                                            fillMode: usageImg.endsWith(".jpg") ? Image.PreserveAspectCrop : Image.PreserveAspectFit
                                            source: usageImg
                                            smooth: true
                                        }
                                    }

                                    Rectangle {
                                        id: imgMask
                                        anchors.fill: imgContent
                                        topLeftRadius: card.radius
                                        topRightRadius: card.radius
                                        visible: false
                                    }

                                    OpacityMask {
                                        anchors.fill: imgContent
                                        source: imgContent
                                        maskSource: imgMask
                                    }

                                    Text {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        anchors.margins: 12
                                        text: name
                                        elide: Text.ElideRight
                                        color: Utility.getAppHexColor("lightText")
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: card.radius
                                        color: "transparent"
                                        border.width: 2
                                        border.color: Material.accent
                                        visible: selected

                                        Rectangle {
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            anchors.margins: 8
                                            width: 24
                                            height: 24
                                            radius: 12
                                            color: Material.accent

                                            Text {
                                                anchors.centerIn: parent
                                                text: "\u2713"
                                                color: Utility.getAppHexColor("white")
                                            }
                                        }
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

                    ColumnLayout {
                        Layout.preferredHeight: overrideUsageBox.checked && !dialog.isHorizontal ? parent.height * 0.5 : implicitHeight
                        Layout.fillHeight: phoneLandscape && overrideUsageBox.checked
                        Layout.fillWidth: true
                        Layout.column: 0
                        Layout.row: dialog.isHorizontal ? 0 : 1
                        Layout.alignment: Qt.AlignTop

                        StepHint {
                            Layout.fillWidth: true
                            visible: phoneLandscape
                            title: stepHints[0][0]
                            hint: stepHints[0][1]
                        }

                        CheckBox {
                            id: overrideUsageBox
                            Layout.fillWidth: true
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
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            // Only the height left under the hint on landscape phones
                            Layout.preferredHeight: phoneLandscape ? 0 : -1
                            visible: overrideUsageBox.checked
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

                    GridView {
                        id: motorList
                        property int cols: mainIsTablet && dialog.isHorizontal ? 2 : 1
                        focus: true
                        clip: true
                        cellWidth: width / cols
                        cellHeight: phoneLandscape ? 70 : 95
                        Layout.row: 0
                        Layout.fillHeight: true
                        Layout.preferredWidth:  dialog.isHorizontal ? parent.width/2 : parent.width
                        Layout.column: dialog.isHorizontal ? 1 : 0

                        Component {
                            id: motorDelegate

                            Rectangle {
                                id: imgRect
                                property variant modelData: model
                                readonly property int imgSize: height - 10

                                width: motorList.cellWidth - (motorList.cols > 1 ? 5 : 0)
                                height: motorList.cellHeight - 5
                                color: Utility.getAppHexColor("cardBackground")
                                border.width: GridView.isCurrentItem ? 2 : 1
                                border.color: GridView.isCurrentItem ? Material.accent : Utility.getAppHexColor("raisedBackground")
                                radius: 12
                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10

                                    Item {
                                        Layout.preferredWidth: imgRect.imgSize
                                        Layout.preferredHeight: imgRect.imgSize
                                        Layout.leftMargin: 5
                                        opacity: imgRect.GridView.isCurrentItem ? 1.0 : 0.5

                                        Image {
                                            id: image
                                            fillMode: Image.PreserveAspectCrop
                                            source: motorImg
                                            width: imgRect.imgSize
                                            height: imgRect.imgSize
                                            smooth: true
                                            mipmap: false
                                            visible: false
                                        }

                                        Rectangle {
                                            id: mask
                                            width: imgRect.imgSize
                                            height: imgRect.imgSize
                                            radius: 12
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
                            maxPowerLossBox.realValue = motorModel.get(motorList.currentIndex).maxLosses
                            openloopErpmBox.realValue = motorModel.get(motorList.currentIndex).openloopErpm
                            sensorlessBox.realValue = motorModel.get(motorList.currentIndex).sensorlessErpm
                            motorPolesBox.realValue = motorModel.get(motorList.currentIndex).poles
                        }
                    }

                    SidePane {
                        Layout.column: 0
                        Layout.row: dialog.isHorizontal ? 0 : 1
                        title: stepHints[1][0]
                        hint: stepHints[1][1]

                        CheckBox {
                            id: overrideBox
                            checked: false
                            text: qsTr("Override (Advanced)")

                            onToggled: {
                                if (!checked) {
                                    maxPowerLossBox.realValue = motorModel.get(motorList.currentIndex).maxLosses
                                    openloopErpmBox.realValue = motorModel.get(motorList.currentIndex).openloopErpm
                                    sensorlessBox.realValue = motorModel.get(motorList.currentIndex).sensorlessErpm
                                    motorPolesBox.realValue = motorModel.get(motorList.currentIndex).poles
                                }
                            }
                        }

                        // Tiles show the values from the selected motor, the boxes replace them when overriding
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 8
                            rowSpacing: 8
                            uniformCellWidths: true

                            ValueTile {
                                Layout.fillWidth: true
                                visible: !overrideBox.checked
                                title: qsTr("Max power loss")
                                value: maxPowerLossBox.realValue.toFixed(0)
                                unit: "W"
                            }

                            ValueTile {
                                Layout.fillWidth: true
                                visible: !overrideBox.checked
                                title: qsTr("Openloop ERPM")
                                value: openloopErpmBox.realValue.toFixed(0)
                            }

                            ValueTile {
                                Layout.fillWidth: true
                                visible: !overrideBox.checked
                                title: qsTr("Sensorless ERPM")
                                value: sensorlessBox.realValue.toFixed(0)
                            }

                            ValueTile {
                                Layout.fillWidth: true
                                visible: !overrideBox.checked
                                title: qsTr("Motor poles")
                                value: motorPolesBox.realValue.toFixed(0)
                            }

                            LabeledSpinBox {
                                id: maxPowerLossBox
                                Layout.fillWidth: true
                                visible: overrideBox.checked
                                label: qsTr("Max power loss")
                                decimals: 1
                                realFrom: 0
                                realTo: 5000
                                realValue: 10
                                suffix: " W"
                            }

                            LabeledSpinBox {
                                id: openloopErpmBox
                                Layout.fillWidth: true
                                visible: overrideBox.checked
                                label: qsTr("Openloop ERPM")
                                decimals: 0
                                realFrom: 0
                                realTo: 50000
                                realValue: 500
                            }

                            LabeledSpinBox {
                                id: sensorlessBox
                                Layout.fillWidth: true
                                visible: overrideBox.checked
                                label: qsTr("Sensorless ERPM")
                                decimals: 0
                                realFrom: 0
                                realTo: 50000
                                realValue: 1500
                            }

                            LabeledSpinBox {
                                id: motorPolesBox
                                Layout.fillWidth: true
                                visible: overrideBox.checked
                                label: qsTr("Motor poles")
                                decimals: 0
                                realFrom: 2
                                realTo: 512
                                realValue: 2
                                realStepSize: 2

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

                        ColumnLayout {
                            anchors.fill: parent

                            ParamList {
                                id: paramListBatt
                                Layout.fillWidth: true
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: rtVin > 0 && battCells > 0
                                color: Utility.getAppHexColor("normalText")
                                font.pointSize: 11
                                wrapMode: Text.WordWrap
                                text: qsTr("Input %1 V = %2 V per cell at %3S").arg(rtVin.toFixed(1))
                                      .arg((rtVin / battCells).toFixed(2)).arg(battCells)
                            }
                        }
                    }

                    SidePane {
                        Layout.column: 0
                        Layout.row: dialog.isHorizontal ? 0 : 1
                        title: stepHints[2][0]
                        hint: stepHints[2][1]

                        CheckBox {
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

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 10
                            rowSpacing: 10
                            uniformCellWidths: true
                            enabled: overrideBattBox.checked

                            LabeledSpinBox {
                                id: currentInMinBox
                                Layout.fillWidth: true
                                label: qsTr("Battery current regen")
                                decimals: 1
                                realStepSize: 5
                                realFrom: 0
                                realTo: -9999
                                realValue: 0
                                suffix: " A"
                            }

                            LabeledSpinBox {
                                id: currentInMaxBox
                                Layout.fillWidth: true
                                label: qsTr("Battery current max")
                                decimals: 1
                                realStepSize: 5
                                realFrom: 0
                                realTo: 9999
                                realValue: 0
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
                    SidePane {
                        Layout.preferredWidth: parent.width / 2
                        Layout.column: 0
                        Layout.row: 0
                        title: stepHints[3][0]
                        hint: stepHints[3][1]

                        SectionHeader {
                            Layout.topMargin: 0
                            text: qsTr("Gear ratio") + "  " + (directDriveBox.checked ? 1 :
                                  wheelPulleyBox.value / motorPulleyBox.value).toFixed(2) + " : 1"
                        }

                        CheckBox {
                            id: directDriveBox
                            text: "Direct Drive"
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 10
                            rowSpacing: 10
                            uniformCellWidths: true

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Text {
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    color: Utility.getAppHexColor("normalText")
                                    font.pointSize: 11
                                    text: qsTr("Motor pulley")
                                }

                                SpinBox {
                                    enabled: !directDriveBox.checked
                                    id: motorPulleyBox
                                    Layout.fillWidth: true
                                    background: InputBackground {}
                                    from: 1
                                    to: 999
                                    value: 13
                                    editable: true

                                    textFromValue: function(value) {
                                        return !directDriveBox.checked ? value : 1
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Text {
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    color: Utility.getAppHexColor("normalText")
                                    font.pointSize: 11
                                    text: qsTr("Wheel pulley")
                                }

                                SpinBox {
                                    enabled: !directDriveBox.checked
                                    id: wheelPulleyBox
                                    Layout.fillWidth: true
                                    background: InputBackground {}
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
                GridLayout {
                    anchors.fill: parent
                    columns: mainIsTablet && dialog.isHorizontal ? 2 : 1

                    DirectionSetup {
                        id: dirSetup
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        dialogParent: topItem.dialogParent
                        title: stepHints[4][0]
                    }

                    // Detection result, stays visible while testing directions
                    ScrollView {
                        id: resultPane
                        visible: mainIsTablet
                        Layout.preferredWidth: dialog.isHorizontal ? 360 : -1
                        Layout.preferredHeight: dialog.isHorizontal ? -1 : Math.min(implicitHeight, parent.height * 0.4)
                        Layout.fillWidth: !dialog.isHorizontal
                        Layout.fillHeight: dialog.isHorizontal
                        contentWidth: availableWidth
                        clip: true
                        padding: 12

                        background: Rectangle {
                            radius: 12
                            color: Utility.getAppHexColor("cardBackground")
                            border.color: Utility.getAppHexColor("raisedBackground")
                        }

                        DetectResultView {
                            width: resultPane.availableWidth
                            result: detectResult
                        }
                    }
                }
            }
        }

        header: ColumnLayout {
            spacing: 0

            WizardHeader {
                Layout.fillWidth: true
                title: qsTr("Setup Motors")
                steps: ["Usage", "Motor", "Battery", "Setup", "Direction"]
                currentIndex: stackLayout.currentIndex
                summaries: mainIsTablet && stackLayout.currentIndex > 0 ? stepSummaries() : []

                onStepClicked: (index) => {
                    stackLayout.currentIndex = index
                    updateButtonText()
                }
            }

            // Landscape phones lack the height, the steps show it beside their content there
            StepHint {
                Layout.fillWidth: true
                Layout.margins: 14
                Layout.bottomMargin: 0
                visible: !phoneLandscape
                title: stepHints[stackLayout.currentIndex][0]
                hint: stepHints[stackLayout.currentIndex][1]
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
                Layout.margins: 10
                Layout.leftMargin: 5
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: "Next"
                highlighted: true

                onClicked: {
                    if (stackLayout.currentIndex == 0) {
                        stackLayout.currentIndex++
                    } else if (stackLayout.currentIndex == 1) {
                        // Nothing is selected until a motor is tapped, detection would use the limits of no motor
                        if (motorList.currentIndex < 0 && !overrideBox.checked) {
                            VescIf.emitMessageDialog(qsTr("Select a Motor"),
                                                     qsTr("Pick the motor closest in size to yours first."),
                                                     false, false)
                        } else {
                            startWarningDialog.open()
                        }
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

    SheetDialog {
        id: loadDefaultDialog
        acceptText: qsTr("Restore defaults")
        rejectText: qsTr("Keep current")
        title: "Load Default Parameters"

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

    SheetDialog {
        id: startWarningDialog
        acceptText: qsTr("Continue")
        property int indexNow: 0
        title: "Motor Selection"

        WarningBanner {
            width: parent.width
            text: "This selection determines the recommended max current for your motor based on heat dissipation. " +
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

    SheetDialog {
        id: batteryWarningDialog
        acceptText: qsTr("Continue")
        property int indexNow: 0
        title: "Battery Settings"

        WarningBanner {
            width: parent.width
            text: "You have not specified battery current limits (advanced box). This means that " +
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

    SheetDialog {
        id: detectDialog
        acceptText: qsTr("Run detection")
        title: "Detect FOC Parameters"
        iconSource: "qrc" + Utility.getThemePath() + "icons/Detect FOC-96.png"

        ColumnLayout {
            anchors.fill: parent

            WarningBanner {
                Layout.fillWidth: true
                text: "This is going to spin up all motors. Make " +
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

                detecting = true
                var res  = Utility.detectAllFoc(VescIf, detectCanBox.checked,
                                                maxPowerLossBox.realValue,
                                                currentInMinBox.realValue,
                                                currentInMaxBox.realValue,
                                                openloopErpmBox.realValue,
                                                sensorlessBox.realValue)
                detecting = false

                var resDetect = false
                if (res.startsWith("Success!")) {
                    resDetect = true

                    Utility.setBatteryCutCanFromCurrentConfig(VescIf, canDevs, usageModel.get(usageList.currentIndex).batt_cut_cautious)

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
                            usageModel.get(usageList.currentIndex).hfi_start &&
                            motorModel.get(motorList.currentIndex).hfi_start) {
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
                detectResult = res
                resultDialog.open()
            }
        }
    }

    // Options of a step. Beside the list on landscape phones, with the step hint and scrolling when taller than the dialog
    component SidePane: Flickable {
        id: pane
        default property alias content: paneColumn.data
        property alias title: paneHint.title
        property alias hint: paneHint.hint

        Layout.fillWidth: true
        Layout.fillHeight: phoneLandscape
        Layout.preferredHeight: contentHeight
        Layout.alignment: Qt.AlignTop
        contentHeight: paneColumn.implicitHeight
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollIndicator.vertical: ScrollIndicator {}

        ColumnLayout {
            id: paneColumn
            width: pane.width
            spacing: 8

            StepHint {
                id: paneHint
                Layout.fillWidth: true
                visible: phoneLandscape
            }
        }
    }

    // Utility.detectAllFoc returns text: a status line, then one block of "name : value" lines per VESC
    component DetectResultView: ColumnLayout {
        id: resultView
        property string result
        readonly property bool success: result.startsWith("Success!")
        readonly property var names: ({"Motor current": qsTr("Max current"), "Motor R": qsTr("Resistance"),
                                         "Motor L": qsTr("Inductance"), "Motor Lq-Ld": qsTr("Lq - Ld"),
                                         "Motor Flux Linkage": qsTr("Flux linkage"), "Temp Comp": qsTr("Temp comp")})

        function devices() {
            var devs = []
            var can = false
            result.split("\n\n").forEach(function(block) {
                if (block.startsWith("VESCs on CAN-bus")) {
                    can = true
                } else if (block.startsWith("VESC ID")) {
                    var rows = block.split("\n").map(function(line) {
                        var sep = line.indexOf(":")
                        return [line.slice(0, sep).trim(), line.slice(sep + 1).trim()]
                    })
                    devs.push({id: rows[0][1], can: can, rows: rows.slice(1)})
                }
            })
            return devs
        }

        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: statusColumn.implicitHeight + 20
            radius: 12
            color: Qt.alpha(Utility.getAppHexColor(resultView.success ? "positive" : "danger"), 0.15)
            border.color: Qt.alpha(Utility.getAppHexColor(resultView.success ? "positiveText" : "dangerText"), 0.33)

            ColumnLayout {
                id: statusColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    font.weight: Font.Medium
                    color: Utility.getAppHexColor(resultView.success ? "positiveText" : "dangerText")
                    text: resultView.success ? qsTr("Detection successful") : qsTr("Detection failed")
                }

                Text {
                    Layout.fillWidth: true
                    visible: !resultView.success
                    wrapMode: Text.WordWrap
                    color: Utility.getAppHexColor("lightText")
                    text: resultView.result.substring(resultView.result.indexOf("\n") + 1)
                }
            }
        }

        Repeater {
            model: resultView.success ? resultView.devices() : []

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                SectionHeader {
                    Layout.topMargin: 0
                    text: (modelData.can ? qsTr("VESC %1 on CAN") : qsTr("VESC %1")).arg(modelData.id)
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 8
                    rowSpacing: 8
                    uniformCellWidths: true

                    Repeater {
                        model: modelData.rows

                        ValueTile {
                            // Number and unit, otherwise text such as the sensor type
                            readonly property var number: modelData[1].match(/^(-?[\d.]+) (.+)$/)
                            Layout.fillWidth: true
                            title: resultView.names[modelData[0]] || modelData[0]
                            value: number ? number[1] : modelData[1] === "True" ? qsTr("On") :
                                   modelData[1] === "False" ? qsTr("Off") : modelData[1]
                            unit: number ? number[2] : ""
                        }
                    }
                }
            }
        }
    }

    SheetDialog {
        id: resultDialog
        acceptText: qsTr("OK")
        rejectText: ""
        iconSource: "qrc" + Utility.getThemePath() + "icons/Detect FOC-96.png"

        ScrollView {
            id: resultScroll
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: resultScroll.availableWidth
                spacing: 12

                DetectResultView {
                    Layout.fillWidth: true
                    result: detectResult
                }

                Text {
                    Layout.fillWidth: true
                    visible: detectResult.startsWith("Success!")
                    wrapMode: Text.WordWrap
                    color: Utility.getAppHexColor("normalText")
                    font.pointSize: 11
                    text: qsTr("Next, check that each motor spins the right way.")
                }
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

    BusyDialog {
        id: commDialog
        title: detecting ? qsTr("Detecting motors...") : "Processing..."

        Text {
            Layout.fillWidth: true
            visible: detecting
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 11
            text: qsTr("The motors make some noise and then spin. This can take a minute or two.")
        }
    }
}
