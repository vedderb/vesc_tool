/*
    Desktop FocPage — faithful recreation of the original PageFoc widget.
    10 sub-tabs: General, Sensorless, Hall Sensors, Encoder, HFI, VSS,
                 Filters, Offsets, Field Weakening, Advanced
    Each tab shows parameter editors for the corresponding subgroup of "foc".
    Sensorless tab includes Openloop Presets.
    Offsets tab includes Measure Offsets button.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    id: focPage

    property ConfigParams mMcConf: VescIf.mcConfig()
    property Commands mCommands: VescIf.commands()
    property var _dynamicItems: []

    ParamEditors {
        id: editors
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 0

        TabBar {
            id: focTabBar
            Layout.fillWidth: true
            TabButton { text: "General" }
            TabButton { text: "Sensorless" }
            TabButton { text: "Hall Sensors" }
            TabButton { text: "Encoder" }
            TabButton { text: "HFI" }
            TabButton { text: "VSS" }
            TabButton { text: "Filters" }
            TabButton { text: "Offsets" }
            TabButton { text: "Field Weakening" }
            TabButton { text: "Advanced" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: focTabBar.currentIndex

            // Tab 0: General
            Flickable {
                clip: true; contentWidth: width; contentHeight: generalCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: generalCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 1: Sensorless + Openloop Presets
            Flickable {
                clip: true; contentWidth: width; contentHeight: sensorlessCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: sensorlessCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 2: Hall Sensors
            Flickable {
                clip: true; contentWidth: width; contentHeight: hallCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: hallCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 3: Encoder
            Flickable {
                clip: true; contentWidth: width; contentHeight: encoderCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: encoderCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 4: HFI
            Flickable {
                clip: true; contentWidth: width; contentHeight: hfiCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: hfiCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 5: VSS
            Flickable {
                clip: true; contentWidth: width; contentHeight: vssCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: vssCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 6: Filters
            Flickable {
                clip: true; contentWidth: width; contentHeight: filterCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: filterCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 7: Offsets + Measure Offsets button
            Flickable {
                clip: true; contentWidth: width; contentHeight: offsetCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: offsetCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 8: Field Weakening
            Flickable {
                clip: true; contentWidth: width; contentHeight: fwCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: fwCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 9: Advanced
            Flickable {
                clip: true; contentWidth: width; contentHeight: advancedCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: advancedCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }
        }
    }

    // --- Openloop Presets (appended to sensorlessCol) ---
    Component {
        id: openloopPresetsComponent
        GroupBox {
            title: "Openloop Presets"
            Layout.fillWidth: true
            GridLayout {
                columns: 2; rowSpacing: 2; columnSpacing: 2; anchors.fill: parent
                Button {
                    text: "Same as FW < 5.02"; Layout.fillWidth: true
                    onClicked: applyOpenloopPreset(700, 0, 0.1, 0.1, 0, 0)
                }
                Button {
                    text: "Propeller"; Layout.fillWidth: true
                    onClicked: applyOpenloopPreset(1500, 0.1, 0.1, 0.1, 0.1, 0.1)
                }
                Button {
                    text: "Generic"; Layout.fillWidth: true
                    onClicked: applyOpenloopPreset(1200, 0.1, 0.1, 0.05, 0, 0.1)
                }
                Button {
                    text: "Heavy Inertial Load"; Layout.fillWidth: true
                    onClicked: applyOpenloopPreset(1500, 0.1, 0.1, 0.2, 0.5, 0.5)
                }
                Button {
                    text: "Fast Start"; Layout.fillWidth: true
                    onClicked: applyOpenloopPreset(1500, 0, 0.1, 0.1, 0, 0)
                }
            }
        }
    }

    // --- Measure Offsets button (appended to offsetCol) ---
    Component {
        id: measureOffsetsComponent
        Button {
            text: "Measure Offsets"
            Layout.fillWidth: true
            onClicked: {
                if (!VescIf.isPortConnected()) {
                    VescIf.emitMessageDialog("Connection Error",
                        "The VESC is not connected. Please connect it.", false, false)
                    return
                }
                VescIf.emitMessageDialog("Measure Offsets",
                    "This is going to measure and store all offsets. Make sure " +
                    "that the motor is not moving or disconnected. Motor rotation " +
                    "during the measurement will cause an invalid result.",
                    false, false)
                enabled = false
                mCommands.sendTerminalCmd("foc_dc_cal")
                measureOffsetsTimer.start()
            }
        }
    }

    Timer {
        id: measureOffsetsTimer
        interval: 5000; repeat: false
        onTriggered: mCommands.getMcconf()
    }

    function applyOpenloopPreset(rpm, rpmLow, hyst, time, timeLock, timeRamp) {
        mMcConf.updateParamDouble("foc_openloop_rpm", rpm)
        mMcConf.updateParamDouble("foc_openloop_rpm_low", rpmLow)
        mMcConf.updateParamDouble("foc_sl_openloop_hyst", hyst)
        mMcConf.updateParamDouble("foc_sl_openloop_time", time)
        mMcConf.updateParamDouble("foc_sl_openloop_time_lock", timeLock)
        mMcConf.updateParamDouble("foc_sl_openloop_time_ramp", timeRamp)
    }

    function loadSubgroup(parentCol, subgroup) {
        var params = mMcConf.getParamsFromSubgroup("foc", subgroup)
        for (var p = 0; p < params.length; p++) {
            var paramName = params[p]
            if (paramName.indexOf("::sep::") === 0) {
                var sep = editors.createSeparator(parentCol, paramName.substring(7))
                if (sep) _dynamicItems.push(sep)
                continue
            }
            var e = editors.createEditorMc(parentCol, paramName)
            if (e) { e.Layout.fillWidth = true; _dynamicItems.push(e) }
        }
    }

    function reloadAll() {
        for (var d = 0; d < _dynamicItems.length; d++) {
            if (_dynamicItems[d]) _dynamicItems[d].destroy()
        }
        _dynamicItems = []

        loadSubgroup(generalCol, "general")
        loadSubgroup(sensorlessCol, "sensorless")
        loadSubgroup(hallCol, "hall sensors")
        loadSubgroup(encoderCol, "encoder")
        loadSubgroup(hfiCol, "hfi")
        loadSubgroup(vssCol, "vss")
        loadSubgroup(filterCol, "filters")
        loadSubgroup(offsetCol, "offsets")
        loadSubgroup(fwCol, "field weakening")
        loadSubgroup(advancedCol, "advanced")

        // Add openloop presets to sensorless tab
        var preset = openloopPresetsComponent.createObject(sensorlessCol)
        if (preset) _dynamicItems.push(preset)

        // Add measure offsets button to offsets tab
        var measureBtn = measureOffsetsComponent.createObject(offsetCol)
        if (measureBtn) _dynamicItems.push(measureBtn)
    }

    Component.onCompleted: reloadAll()

    Connections {
        target: mMcConf
        function onUpdated() { reloadAll() }
    }
}
