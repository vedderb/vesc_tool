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
import QtQuick.Dialogs 1.3 as Dl

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: confPageMotorItem
    property Commands mCommands: VescIf.commands()
    property bool isHorizontal: width > height

    ParamEditors {
        id: editors
    }

    DetectBldc {
        id: detectBldc
    }

    DetectFocParam {
        id: detectFocParam
    }

    DetectFocHall {
        id: detectFocHall
    }

    DetectFocEncoder {
        id: detectFocEncoder
    }

    Dialog {
        id: directionSetupDialog
        title: "Direction Setup"
        standardButtons: Dialog.Close
        modal: true
        focus: true
        padding: 10

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        width: parent.width - 10 - notchLeft - notchRight
        closePolicy: Popup.CloseOnEscape
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        parent: ApplicationWindow.overlay

        DirectionSetup {
            id: directionSetup
            anchors.fill: parent
        }
    }

    Dialog {
        id: measureOffsetDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        focus: true
        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape
        title: "Measure Offsets"

        x: 10
        y: Math.max((parent.height - height) / 2, 10)
        parent: ApplicationWindow.overlay

        Overlay.modal: Rectangle {
            color: "#AA000000"
        }

        Text {
            anchors.fill: parent
            id: detectLambdaLabel
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text:
                "This is going to measure and store all offsets. Make sure " +
                "that the motor is not moving or disconnected. Motor rotation " +
                "during the measurement will cause an invalid result. Do you " +
                "want to continue?"
        }

        Timer {
            id: offsetReadConfTimer
            interval: 5000
            repeat: false
            running: false
            onTriggered: {
                mCommands.getMcconf()
            }
        }

        onAccepted: {
            mCommands.sendTerminalCmd("foc_dc_cal")
            offsetReadConfTimer.restart()
        }
    }

    onIsHorizontalChanged: {
        updateEditors()
    }

    function addSeparator(text) {
        var e = editors.createSeparator(scrollCol, text)
        e.Layout.columnSpan = isHorizontal ? 2 : 1
    }

    function destroyEditors() {
        for(var i = scrollCol.children.length;i > 0;i--) {
            scrollCol.children[i - 1].destroy(1) // Only works with delay on android, seems to be a bug
        }
    }

    function createEditorMc(param) {
        var e = editors.createEditorMc(scrollCol, param)
        e.Layout.preferredWidth = 500
        e.Layout.fillsWidth = true
    }

    function updateEditors() {
        destroyEditors()

        var params = VescIf.mcConfig().getParamsFromSubgroup(pageBox.currentText, tabBox.currentText)

        for (var i = 0;i < params.length;i++) {
            if (params[i].startsWith("::sep::")) {
                addSeparator(params[i].substr(7))
            } else {
                createEditorMc(params[i])
            }
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 0

        GridLayout {
            Layout.fillWidth: true
            columns: isHorizontal ? 2 : 1
            rowSpacing: -5
            ComboBox {
                id: pageBox
                Layout.fillWidth: true

                model: VescIf.mcConfig().getParamGroups()

                onCurrentTextChanged: {
                    var tabTextOld = tabBox.currentText
                    var subgroups = VescIf.mcConfig().getParamSubgroups(currentText)

                    tabBox.model = subgroups
                    tabBox.visible = subgroups.length > 1

                    if (tabTextOld === tabBox.currentText && tabTextOld !== "") {
                        updateEditors()
                    }
                }
            }
            
            ComboBox {
                id: tabBox
                Layout.fillWidth: true
                
                onCurrentTextChanged: {
                    updateEditors()
                }
            }
        }
        
        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: column.width
            contentHeight: scrollCol.preferredHeight
            clip: true

            GridLayout {
                id: scrollCol
                width: column.width
                columns: isHorizontal ? 2 : 1
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Button {
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                text: "Write"

                onClicked: {
                    mCommands.setMcconf(true)
                }
            }

            Button {
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                text: "Read"

                onClicked: {
                    mCommands.getMcconf()
                }
            }

            Button {
                id: menuButton
                Layout.preferredWidth: 50
                Layout.fillWidth: true
                text: "..."
                onClicked: menu.open()

                Menu {
                    id: menu
                    bottomPadding: notchBot
                    leftPadding: notchLeft
                    rightPadding: notchRight
                    parent: confPageMotorItem
                    y: parent.height - implicitHeight
                    width: parent.width

                    MenuItem {
                        text: "Read Default Settings"
                        onTriggered: {
                            mCommands.getMcconfDefault()
                        }
                    }
                    MenuItem {
                        text: "Detect BLDC Parameters..."
                        onTriggered: {
                            detectBldc.openDialog()
                        }
                    }
                    MenuItem {
                        text: "Detect FOC Parameters..."
                        onTriggered: {
                            detectFocParam.openDialog()
                        }
                    }
                    MenuItem {
                        text: "Detect FOC Hall Sensors..."
                        onTriggered: {
                            detectFocHall.openDialog()
                        }
                    }
                    MenuItem {
                        text: "Detect FOC Encoder..."
                        onTriggered: {
                            detectFocEncoder.openDialog()
                        }
                    }
                    MenuItem {
                        text: "Measure FOC Offsets"
                        onTriggered: {
                            if (VescIf.isPortConnected()) {
                                measureOffsetDialog.open()
                            } else {
                                VescIf.emitMessageDialog("Measure FOC Offsets", "Not Connected", false, false)
                            }
                        }
                    }
                    MenuItem {
                        text: "Setup Motor Directions..."
                        onTriggered: {
                            directionSetupDialog.open()
                            directionSetup.scanCan()
                        }
                    }
                    MenuItem {
                        text: "Save XML"
                        onTriggered: {
                            if (Utility.requestFilePermission()) {
                                fileDialogSave.close()
                                fileDialogSave.open()
                            } else {
                                VescIf.emitMessageDialog(
                                            "File Permissions",
                                            "Unable to request file system permission.",
                                            false, false)
                            }
                        }

                        Dl.FileDialog {
                            id: fileDialogSave
                            title: "Please choose a file"
                            nameFilters: ["*"]
                            selectExisting: false
                            selectMultiple: false
                            onAccepted: {
                                var path = fileUrl.toString()
                                if (VescIf.mcConfig().saveXml(path, "MCConfiguration")) {
                                    VescIf.emitStatusMessage("Mcconf Saved", true)
                                } else {
                                    VescIf.emitStatusMessage("Mcconf Save Failed", false)
                                }

                                close()
                                parent.forceActiveFocus()
                            }
                            onRejected: {
                                close()
                                parent.forceActiveFocus()
                            }
                        }
                    }
                    MenuItem {
                        text: "Load XML"
                        onTriggered: {
                            if (Utility.requestFilePermission()) {
                                fileDialogLoad.close()
                                fileDialogLoad.open()
                            } else {
                                VescIf.emitMessageDialog(
                                            "File Permissions",
                                            "Unable to request file system permission.",
                                            false, false)
                            }
                        }

                        Dl.FileDialog {
                            id: fileDialogLoad
                            title: "Please choose a file"
                            nameFilters: ["*"]
                            selectExisting: true
                            selectMultiple: false
                            onAccepted: {
                                var path = fileUrl.toString()
                                if (VescIf.mcConfig().loadXml(path, "MCConfiguration")) {
                                    VescIf.emitStatusMessage("Mcconf Loaded", true)
                                } else {
                                    VescIf.emitStatusMessage("Mcconf Load Failed", false)
                                }

                                close()
                                parent.forceActiveFocus()
                            }
                            onRejected: {
                                close()
                                parent.forceActiveFocus()
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: mCommands

        // TODO: For some reason this does not work
        function onMcConfigCheckResult(paramsNotSet) {
            if (paramsNotSet.length > 0) {
                var notUpdated = "The following parameters were truncated because " +
                        "they were beyond the hardware limits:\n"

                for (var i = 0;i < paramsNotSet.length;i++) {
                    notUpdated += mMcConf.getLongName(paramsNotSet[i]) + "\n"
                }

                VescIf.emitMessageDialog("Parameters truncated", notUpdated, false, false)
            }
        }
    }

    Connections {
        target: VescIf
        function onConfigurationChanged() {
            pageBox.model = VescIf.mcConfig().getParamGroups()

            var tabTextOld = tabBox.currentText
            var subgroups = VescIf.mcConfig().getParamSubgroups(pageBox.currentText)

            tabBox.model = subgroups
            tabBox.visible = subgroups.length > 1

            updateEditors()
        }
    }
}
