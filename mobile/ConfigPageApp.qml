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
    id: appPageItem

    property Commands mCommands: VescIf.commands()
    property bool isHorizontal: width > height
    property var dialogParent: Overlay.overlay
    property bool active: false
    // Group shown while this is the current page, to keep the live input idle otherwise
    readonly property string mapGroup: mainIsTablet && groupList.visible && active ? pageBox.currentText : ""

    ParamEditors {
        id: editors
    }

    SheetDialog {
        id: ppmMap
        title: qsTr("PPM Mapping")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Pulse Width-96.png"

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth
            clip: true

            PpmMap {
                width: parent.width
            }
        }
    }

    SheetDialog {
        id: adcMap
        title: qsTr("ADC Mapping")
        iconSource: "qrc" + Utility.getThemePath() + "icons/Potentiometer-96.png"

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth
            clip: true

            AdcMap {
                width: parent.width
            }
        }
    }

    SheetDialog {
        id: nrfPair
        title: qsTr("NRF Pairing")
        iconSource: "qrc" + Utility.getThemePath() + "icons/icons8-fantasy-96.png"

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth
            clip: true

            NrfPair {
                width: parent.width
            }
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

    function createEditorApp(param) {
        var e = editors.createEditorApp(scrollCol, param)
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

        var params = VescIf.appConfig().getParamsFromSubgroup(pageBox.currentText, tabBox.currentText)

        for (var i = 0;i < params.length;i++) {
            if (params[i].startsWith("::sep::")) {
                addSeparator(params[i].substr(7))
            } else {
                createEditorApp(params[i])
            }
        }
    }

    ParamGroupList {
        id: groupList
        box: pageBox
        tracker: appTracker
    }

    ColumnLayout {
        id: liveInput
        anchors.right: parent.right
        y: scroll.y
        width: 340
        visible: mapGroup === "PPM" || mapGroup === "ADC"

        SectionHeader {
            text: qsTr("Live input")
        }

        PpmMap {
            Layout.fillWidth: true
            visible: mapGroup === "PPM"
        }

        AdcMap {
            Layout.fillWidth: true
            visible: mapGroup === "ADC"
        }
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

                    model: VescIf.appConfig().getParamGroups()

                    onCurrentTextChanged: {
                        var tabTextOld = tabBox.currentText
                        var subgroups = VescIf.appConfig().getParamSubgroups(currentText)

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
                tracker: appTracker
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
            Layout.rightMargin: liveInput.visible ? liveInput.width + 10 : 0
            contentWidth: availableWidth
            contentHeight: scrollCol.preferredHeight
            clip: true

            GridLayout {
                id: scrollCol
                width: scroll.availableWidth
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
                tracker: appTracker
                ackText: "App config write OK"

                onWriteRequested: {
                    mCommands.setAppConf()
                }
            }

            Button {
                Layout.preferredWidth: 100
                Layout.fillWidth: true
                text: "Read"

                onClicked: {
                    mCommands.getAppConf()
                }
            }

            Button {
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
                            mCommands.getAppConfDefault()
                        }
                    }
                    MenuSeparator {}
                    MenuEntry {
                        text: "PPM Mapping..."
                        iconFile: "Pulse Width-96.png"
                        onTriggered: mainIsTablet && groupList.visible ? jumpTo("PPM", "Mapping", "") : ppmMap.open()
                    }
                    MenuEntry {
                        text: "ADC Mapping..."
                        iconFile: "Potentiometer-96.png"
                        onTriggered: mainIsTablet && groupList.visible ? jumpTo("ADC", "Mapping", "") : adcMap.open()
                    }
                    MenuEntry {
                        text: "Pair NRF..."
                        iconFile: "icons8-fantasy-96.png"
                        onTriggered: {
                            nrfPair.open()
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
                            fileMode: FileDialog.SaveFile
                            title: "Please choose a file"
                            nameFilters: ["*"]

                            onAccepted: {
                                var path = selectedFile.toString()
                                if (!path.toLowerCase().endsWith(".xml")) {
                                    path += ".xml"
                                }

                                if (VescIf.appConfig().saveXml(path, "APPConfiguration")) {
                                    VescIf.emitStatusMessage("Appconf Saved", true)
                                } else {
                                    VescIf.emitStatusMessage("Appconf Save Failed", false)
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
                            fileMode: FileDialog.OpenFile
                            title: "Please choose a file"
                            nameFilters: ["*"]

                            onAccepted: {
                                var path = selectedFile.toString()
                                if (VescIf.appConfig().loadXml(path, "APPConfiguration")) {
                                    VescIf.emitStatusMessage("Appconf Loaded", true)
                                } else {
                                    VescIf.emitStatusMessage("Appconf Load Failed", false)
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
        target: VescIf
        function onConfigurationChanged() {
            pageBox.model = VescIf.appConfig().getParamGroups()

            var tabTextOld = tabBox.currentText
            var subgroups = VescIf.appConfig().getParamSubgroups(pageBox.currentText)

            tabBox.model = subgroups

            updateEditors()
        }
    }
}
