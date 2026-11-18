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
import QtQuick.Dialogs

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: confPageMotorItem
    property Commands mCommands: VescIf.commands()
    property bool isHorizontal: width > height
    property var dialogParent: Overlay.overlay

    ParamEditors {
        id: editors
    }

    DetectBldc {
        id: detectBldc
        dialogParent: confPageMotorItem.dialogParent
    }

    DetectFocParam {
        id: detectFocParam
        dialogParent: confPageMotorItem.dialogParent
    }

    DetectFocHall {
        id: detectFocHall
        dialogParent: confPageMotorItem.dialogParent
    }

    DetectFocEncoder {
        id: detectFocEncoder
        dialogParent: confPageMotorItem.dialogParent
    }

    SheetDialog {
        id: directionSetupDialog
        title: "Invert Motor Directions"
        iconSource: "qrc" + Utility.getThemePath() + "icons/Process-96.png"

        DirectionSetup {
            id: directionSetup
            anchors.fill: parent
            dialogParent: confPageMotorItem.dialogParent
        }
    }

    SheetDialog {
        id: measureOffsetDialog
        title: qsTr("Measure Offsets")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Offset-96.png"
        acceptText: qsTr("Measure")

        WarningBanner {
            width: parent.width
            text: qsTr("This measures and stores all offsets. Make sure that the motor is not moving, or disconnect it. Motor rotation during the measurement gives an invalid result.")
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
        e.Layout.columnSpan = Qt.binding(function() { return scrollCol.columns })
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

    function review() {
        writeButton.review()
    }

    function jumpTo(group, subgroup, name) {
        pageBox.currentIndex = pageBox.find(group)
        tabBox.currentIndex = tabBox.model.indexOf(subgroup)
        editors.showParam(scroll, scrollCol, name)
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

    ParamGroupList {
        id: groupList
        box: pageBox
        tracker: mcTracker
    }

    GridLayout {
        id: column
        anchors.fill: parent
        anchors.leftMargin: groupList.visible ? groupList.width + 10 : 0
        // Phone landscape: buttons stacked right of the parameters
        columns: phoneLandscape ? 2 : 1

        GridLayout {
            Layout.columnSpan: column.columns
            Layout.fillWidth: true
            Layout.topMargin: 5
            columns: isHorizontal ? 2 : 1
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                visible: !groupList.visible

                ComboBox {
                    id: pageBox
                    background: InputBackground {}
                    Layout.fillWidth: true

                    model: VescIf.mcConfig().getParamGroups()

                    onCurrentTextChanged: {
                        var tabTextOld = tabBox.currentText
                        var subgroups = VescIf.mcConfig().getParamSubgroups(currentText)

                        tabBox.model = subgroups

                        if (tabTextOld === tabBox.currentText && tabTextOld !== "") {
                            updateEditors()
                        }
                    }
                }

                ToolButton {
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Search-96.png"
                    Accessible.name: qsTr("Search parameters")
                    onClicked: paramSearch.open()
                }
            }

            SubgroupBar {
                id: tabBox
                visible: count > 1 || groupList.visible
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                tracker: mcTracker
                group: pageBox.currentText
                
                onCurrentTextChanged: {
                    updateEditors()
                }
            }
        }
        
        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: scroll.width
            contentHeight: scrollCol.preferredHeight
            clip: true

            GridLayout {
                id: scrollCol
                width: scroll.width
                columns: Math.max(1, Math.floor(width / 380))
            }
        }

        GridLayout {
            columns: phoneLandscape ? 1 : 3
            Layout.fillWidth: !phoneLandscape
            Layout.alignment: Qt.AlignTop
            WriteButton {
                id: writeButton
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                tracker: mcTracker
                ackText: "Motor config write OK"

                onWriteRequested: {
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
                Layout.preferredWidth: 56
                Layout.fillWidth: phoneLandscape
                text: "..."
                onClicked: menu.open()

                BottomMenu {
                    id: menu

                    MenuEntry {
                        text: "Read Default Settings"
                        iconFile: "Restart-96.png"
                        onTriggered: {
                            mCommands.getMcconfDefault()
                        }
                    }
                    MenuSeparator {}
                    MenuEntry {
                        text: "Detect BLDC Parameters..."
                        iconFile: "Detect BLDC-96.png"
                        onTriggered: {
                            detectBldc.openDialog()
                        }
                    }
                    MenuEntry {
                        text: "Detect FOC Parameters..."
                        iconFile: "Detect FOC-96.png"
                        onTriggered: {
                            detectFocParam.openDialog()
                        }
                    }
                    MenuEntry {
                        text: "Detect FOC Hall Sensors..."
                        iconFile: "Hall Sensors-96.png"
                        onTriggered: {
                            detectFocHall.openDialog()
                        }
                    }
                    MenuEntry {
                        text: "Detect FOC Encoder..."
                        iconFile: "Encoder-96.png"
                        onTriggered: {
                            detectFocEncoder.openDialog()
                        }
                    }
                    MenuEntry {
                        text: "Measure FOC Offsets"
                        iconFile: "Offset-96.png"
                        onTriggered: {
                            if (VescIf.isPortConnected()) {
                                measureOffsetDialog.open()
                            } else {
                                VescIf.emitMessageDialog("Measure FOC Offsets", "Not Connected", false, false)
                            }
                        }
                    }
                    MenuSeparator {}
                    MenuEntry {
                        text: "Invert Motor Directions..."
                        iconFile: "Process-96.png"
                        onTriggered: {
                            directionSetupDialog.open()
                            directionSetup.scanCan()
                        }
                    }
                    MenuEntry {
                        text: "Multi Settings..."
                        iconFile: "Horizontal Settings Mixer-96.png"
                        onTriggered: {
                            multiSettings.openDialog()
                        }
                    }
                    MenuEntry {
                        text: "Controls..."
                        iconFile: "Controller-96.png"
                        onTriggered: {
                            controls.openDialog()
                        }
                    }
                    MenuSeparator {}
                    MenuEntry {
                        text: "Save XML"
                        iconFile: "Save as-96.png"
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

                        FileDialog {
                            id: fileDialogSave
                            title: "Please choose a file"
                            fileMode: FileDialog.SaveFile
                            nameFilters: ["*"]

                            onAccepted: {
                                var path = selectedFile.toString()
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
                    MenuEntry {
                        text: "Load XML"
                        iconFile: "Open Folder-96.png"
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

                        FileDialog {
                            id: fileDialogLoad
                            title: "Please choose a file"
                            fileMode: FileDialog.OpenFile
                            nameFilters: ["*"]

                            onAccepted: {
                                var path = selectedFile.toString()
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
                var notUpdated = "The following parameters were truncated or not updated. That can happen because " +
                                "they were set outside their allowed limits or because the write failed. In case the " +
                                "write failed you can try writing them again:\n"

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

            updateEditors()
        }
    }
}
