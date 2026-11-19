/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Controls.Material
import QtQuick.Layouts
import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

// Write button with a count of unwritten changes. Tapping the count opens a
// review of the changes, and a successful write can be undone.
Button {
    id: writeButton
    property ConfigParams params: null
    property string ackText: ""
    readonly property var changedParams: params ? params.changedParams : []
    property var undoParams: []
    signal writeRequested()

    text: qsTr("Write")
    highlighted: changedParams.length > 0
    rightPadding: changedParams.length > 0 ? badge.width + 20 : leftPadding

    // Name and device value of every changed parameter, used to revert them
    function referenceValues() {
        return changedParams.map(function(p) { return [p, params.getReference().getParamCopy(p)] })
    }

    function write() {
        if (!writeReady(params)) return
        undoParams = referenceValues()
        writeRequested()
    }

    function review() {
        reviewSheet.open()
    }

    function revert(list) {
        for (var i = 0;i < list.length;i++) {
            params.updateParamFromOther(list[i][0], list[i][1], null)
        }
    }

    onClicked: write()

    // Write acknowledgements, including the ones for custom configs, end up as status messages
    Connections {
        target: VescIf

        function onFwRxChanged(rx, limited) {
            if (!rx) {
                undoParams = []
                reviewSheet.close()
            }
        }

        function onStatusMessage(msg, isGood) {
            if (msg === ackText && undoParams.length > 0) {
                var undo = undoParams
                var target = VescIf.getConnectedUuid() + fwCanId
                undoParams = []
                showToast(msg, qsTr("%1 changed").arg(undo.length === 1 ?
                                                              params.getLongName(undo[0][0]) :
                                                              qsTr("%1 parameters").arg(undo.length)),
                          "ok", qsTr("Undo"), function() {
                              if (target !== VescIf.getConnectedUuid() + fwCanId || !writeReady(params)) return
                              revert(undo)
                              writeRequested()
                          })
            }
        }
    }

    Rectangle {
        id: badge
        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(height, countText.implicitWidth + 10)
        height: 20
        radius: 10
        visible: writeButton.changedParams.length > 0
        color: Utility.getAppHexColor("white")

        Text {
            id: countText
            anchors.centerIn: parent
            text: writeButton.changedParams.length
            font.weight: Font.Bold
            color: Material.accent
        }

        MouseArea {
            anchors.fill: parent
            anchors.margins: -10
            onClicked: reviewSheet.open()
        }
    }

    SheetDialog {
        id: reviewSheet
        title: qsTr("Review changes")
        rejectText: qsTr("Keep editing")
        acceptText: qsTr("Write")
        onAccepted: writeButton.write()

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(changeColumn.implicitHeight, 360)
                contentWidth: availableWidth
                clip: true

                ColumnLayout {
                    id: changeColumn
                    width: parent.width
                    spacing: 0

                    Repeater {
                        model: reviewSheet.visible ? writeButton.changedParams : []

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.minimumHeight: 48
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                text: writeButton.params.getLongName(modelData)
                                wrapMode: Text.Wrap
                                color: Utility.getAppHexColor("lightText")
                            }

                            Text {
                                text: writeButton.params.getReference().getParamValueString(modelData)
                                font.strikeout: true
                                color: Utility.getAppHexColor("disabledText")
                            }

                            Text {
                                text: "→"
                                color: Utility.getAppHexColor("disabledText")
                            }

                            Text {
                                text: writeButton.params.getParamValueString(modelData)
                                font.weight: Font.Medium
                                color: Utility.getAppHexColor("accentText")
                            }
                        }
                    }
                }
            }

            Button {
                Layout.alignment: Qt.AlignLeft
                Layout.bottomMargin: 8
                // Keeps its height on landscape phones, the list above scrolls instead
                Layout.minimumHeight: implicitHeight
                leftPadding: 0
                flat: true
                Material.foreground: Utility.getAppHexColor("dangerText")
                text: qsTr("Discard changes")
                onClicked: {
                    revert(referenceValues())
                    reviewSheet.close()
                }
            }
        }
    }
}
