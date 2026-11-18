/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

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

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    property real res: 0.0
    property real ind: 0.0
    property real lambda: 0.0
    property real kp: 0.0
    property real ki: 0.0
    property real gain: 0.0

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()
    property var dialogParent: Overlay.overlay

    function openDialog() {
        dialog.open()
    }

    function calcKpKi() {
        if (res < 1e-10) {
            VescIf.emitMessageDialog("Calculate Error",
                                     "R is 0. Please measure it first.",
                                     false, false)
            return;
        }

        if (ind < 1e-10) {
            VescIf.emitMessageDialog("Calculate Error",
                                     "L is 0. Please measure it first.",
                                     false, false)
            return;
        }

        // https://e2e.ti.com/blogs_/b/motordrivecontrol/archive/2015/07/20/teaching-your-pi-controller-to-behave-part-ii
        var tc = tcBox.realValue * 1e-6
        var bw = 1.0 / tc
        kp = ind * bw;
        ki = res * bw;
    }

    function calcGain() {
        if (lambda < 1e-10) {
            VescIf.emitMessageDialog("Calculate Error",
                                     "\u03BB is 0. Please measure it first.",
                                     false, false)
            return;
        }

        if (res < 1e-10) {
            VescIf.emitMessageDialog("Calculate Error",
                                     "R is 0. Please measure it first.",
                                     false, false)
            return;
        }

        gain = 1.0e-3 / (lambda * lambda)
    }

    function testConnected() {
        if (VescIf.isPortConnected()) {
            return true
        } else {
            VescIf.emitMessageDialog(
                        "Connection Error",
                        "The VESC is not connected. Please connect it to run detection.",
                        false, false)
            return false
        }
    }

    SheetDialog {
        id: dialog
        fullHeight: true
        title: qsTr("FOC Detection")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Detect FOC-96.png"
        acceptText: qsTr("Apply")
        acceptEnabled: res > 0 && ind > 0 && lambda > 0

        onAccepted: {
            calcKpKi()
            calcGain()

            mMcConf.updateParamDouble("foc_motor_r", res)
            mMcConf.updateParamDouble("foc_motor_l", ind)
            mMcConf.updateParamDouble("foc_motor_flux_linkage", lambda)
            mMcConf.updateParamDouble("foc_current_kp", kp)
            mMcConf.updateParamDouble("foc_current_ki", ki)
            mMcConf.updateParamDouble("foc_observer_gain", gain * 1e6)
        }

        ScrollView {
            id: scroll
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 12

                SectionHeader {
                    Layout.topMargin: 0
                    text: qsTr("Measurement")
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 10
                    uniformCellWidths: true

                    LabeledSpinBox {
                        id: currentBox
                        Layout.fillWidth: true
                        label: qsTr("Current")
                        decimals: 2
                        realValue: mMcConf.getParamDouble("l_current_max") / 3.0
                        realFrom: 0.0
                        realTo: 200.0
                        suffix: " A"
                    }

                    LabeledSpinBox {
                        id: dutyBox
                        Layout.fillWidth: true
                        label: qsTr("Duty cycle")
                        decimals: 2
                        realValue: 0.3
                        realFrom: 0.0
                        realTo: 1.0
                        realStepSize: 0.1
                    }

                    LabeledSpinBox {
                        id: erpmBox
                        Layout.fillWidth: true
                        label: qsTr("Ramp rate (ERPM/s)")
                        decimals: 1
                        realValue: 2000.0
                        realFrom: 0.0
                        realTo: 20000.0
                        realStepSize: 10.0
                    }

                    LabeledSpinBox {
                        id: tcBox
                        Layout.fillWidth: true
                        label: qsTr("Time constant")
                        decimals: 1
                        realValue: 1000.0
                        realFrom: 0.0
                        realTo: 1000000.0
                        realStepSize: 100.0
                        suffix: " µs"
                        onRealValueChanged: if (res > 0 && ind > 0) calcKpKi()
                    }
                }

                Button {
                    Layout.alignment: Qt.AlignLeft
                    text: qsTr("How detection works")
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
                    flat: true
                    onClicked: {
                        VescIf.emitMessageDialog(
                                    mInfoConf.getLongName("help_foc_detect"),
                                    mInfoConf.getDescription("help_foc_detect"),
                                    true, true)
                    }
                }

                SectionHeader {
                    text: qsTr("Detect")
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: qsTr("Measure R && L")
                        highlighted: res === 0
                        onClicked: {
                            if (testConnected()) {
                                detectRlDialog.open()
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: qsTr("Measure \u03BB")
                        enabled: res > 0
                        highlighted: res > 0 && lambda === 0
                        onClicked: {
                            if (testConnected()) {
                                detectLambdaDialog.open()
                            }
                        }
                    }
                }

                SectionHeader {
                    text: qsTr("Results")
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 8
                    rowSpacing: 8
                    uniformCellWidths: true

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("Resistance")
                        value: res > 0 ? (res * 1e3).toFixed(2) : "\u2013"
                        unit: "m\u03A9"
                    }

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("Inductance")
                        value: ind > 0 ? (ind * 1e6).toFixed(2) : "\u2013"
                        unit: "µH"
                    }

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("Flux linkage")
                        value: lambda > 0 ? (lambda * 1e3).toFixed(3) : "\u2013"
                        unit: "mWb"
                    }

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("Current KP")
                        value: kp > 0 ? kp.toFixed(4) : "\u2013"
                    }

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("Current KI")
                        value: ki > 0 ? ki.toFixed(2) : "\u2013"
                    }

                    ValueTile {
                        Layout.fillWidth: true
                        title: qsTr("Observer gain")
                        value: gain > 0 ? gain.toFixed(2) : "\u2013"
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Utility.getAppHexColor("normalText")
                    font.pointSize: 11
                    text: qsTr("Apply puts the results in the motor configuration. Write it to save them on the VESC.")
                }
            }
        }
    }

    SheetDialog {
        id: detectRlDialog
        acceptText: qsTr("Measure")
        title: qsTr("Measure R & L")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Detect FOC-96.png"

        Text {
            width: parent.width
            color: Utility.getAppHexColor("lightText")
            wrapMode: Text.WordWrap
            text: qsTr("The motor will make some noise but not rotate. This is normal, so don't unplug anything unless you see smoke.")
        }

        onAccepted: {
            mCommands.measureRL()
        }
    }

    SheetDialog {
        id: detectLambdaDialog
        acceptText: qsTr("Measure")
        title: qsTr("Measure \u03BB")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Detect FOC-96.png"

        WarningBanner {
            width: parent.width
            text: qsTr("This spins up the motor. Make sure that nothing is in the way.")
        }

        onAccepted: {
            mCommands.measureLinkageOpenloop(currentBox.realValue, erpmBox.realValue,
                                             dutyBox.realValue, res, ind)
        }
    }

    Connections {
        target: mCommands

        function onMotorRLReceived(r, l, ld_lq_diff) {
            if (r < 1e-9 && l < 1e-9) {
                VescIf.emitStatusMessage("Bad FOC Detection Result Received", false)
                VescIf.emitMessageDialog("Bad Detection Result",
                                         "Could not measure the motor resistance and inductance.",
                                         false, false)
            } else {
                VescIf.emitStatusMessage("FOC Detection Result Received", true)
                res = r
                ind = l * 1e-6
                calcKpKi()
            }
        }

        function onMotorLinkageReceived(flux_linkage) {
            if (flux_linkage < 1e-9) {
                VescIf.emitStatusMessage("Bad FOC Detection Result Received", false)
                VescIf.emitMessageDialog("Bad Detection Result",
                                         "Could not measure the flux linkage properly. Adjust " +
                                         "the start parameters according to the help text and try again.",
                                         false, false)
            } else {
                VescIf.emitStatusMessage("FOC Detection Result Received", true)
                lambda = flux_linkage
                calcGain()
            }
        }
    }

    Connections {
        target: mMcConf

        function onParamChangedDouble(src, name, newParam) {
            if (name === "l_current_max") {
                currentBox.realValue = newParam / 3.0
            }
        }
    }
}
