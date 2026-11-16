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
import "ParamFormat.js" as ParamFormat

// Compact parameter row. Tapping it opens the editor content together with
// the Current, Default and Help actions, either inline or in a sheet.
Item {
    id: frame
    property string paramName: ""
    property ConfigParams params: null
    property Item headerControl: null
    property bool useSheet: false
    property bool expanded: false
    readonly property ChangeTracker tracker: params === VescIf.mcConfig() ? mcTracker :
                                             params === VescIf.appConfig() ? appTracker : null
    readonly property bool changed: tracker !== null && tracker.changedParams.indexOf(paramName) >= 0
    property SheetDialog sheet: null
    property string valueText: ""
    property string referenceText: ""
    default property alias content: editorColumn.data

    Layout.fillWidth: true
    implicitHeight: card.implicitHeight

    function closeSheet() {
        if (sheet) {
            sheet.close()
        }
    }

    function flash() {
        flashAnimation.restart()
    }

    function refresh() {
        valueText = ParamFormat.valueString(params, paramName)
        referenceText = changed ? ParamFormat.valueString(params.getReference(), paramName) : ""
    }

    onHeaderControlChanged: headerControl.parent = headerHolder

    Component.onCompleted: {
        if (useSheet) {
            sheet = sheetComponent.createObject(frame)
        }

        if (params != null) {
            nameText.text = params.getLongName(paramName)
            helpText.text = params.getDescription(paramName)
            nowButton.visible = params.getParamTransmittable(paramName)
            defaultButton.visible = params.getParamTransmittable(paramName)
            refresh()
        }
    }

    onChangedChanged: refresh()

    ChangeTracker {
        params: frame.params
        trackChanges: false
        onParamChanged: function(name) {
            if (name === paramName) {
                refresh()
            }
        }
    }

    Rectangle {
        id: card
        anchors.fill: parent
        implicitHeight: column.implicitHeight
        radius: 11
        color: frame.changed ? Qt.alpha(Material.accent, 0.08) : Utility.getAppHexColor("rowBackground")
        border.width: 1
        border.color: frame.changed ? Qt.alpha(Material.accent, 0.3) : Utility.getAppHexColor("lightBackground")

        Rectangle {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 4
            height: parent.height - 18
            radius: 2
            visible: frame.changed
            color: Material.accent
        }

        Rectangle {
            id: flashRect
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 2
            border.color: Utility.getAppHexColor("lightAccent")
            opacity: 0

            SequentialAnimation on opacity {
                id: flashAnimation
                running: false
                loops: 2
                NumberAnimation { to: 1; duration: 250 }
                NumberAnimation { to: 0; duration: 450 }
            }
        }

        ColumnLayout {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumHeight: 52
                Layout.leftMargin: 14
                Layout.rightMargin: 12
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                    Layout.bottomMargin: 6
                    spacing: 1

                    Text {
                        id: nameText
                        Layout.fillWidth: true
                        text: paramName
                        wrapMode: Text.Wrap
                        color: Utility.getAppHexColor("titleText")
                        font.weight: frame.expanded ? Font.Medium : Font.Normal
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: text !== ""
                        text: frame.changed ? qsTr("was %1").arg(frame.referenceText) :
                                              frame.expanded ? ParamFormat.rangeString(params, paramName) : ""
                        elide: Text.ElideRight
                        font.pointSize: 9
                        color: Utility.getAppHexColor("disabledText")
                    }
                }

                Text {
                    visible: frame.headerControl === null
                    text: frame.valueText
                    elide: Text.ElideRight
                    Layout.maximumWidth: frame.width * 0.45
                    font.weight: Font.Medium
                    font.features: { "tnum": 1 }
                    color: frame.changed ? Utility.getAppHexColor("accentText") : Utility.getAppHexColor("titleText")
                }

                Item {
                    id: headerHolder
                    implicitWidth: frame.headerControl ? frame.headerControl.implicitWidth : 0
                    implicitHeight: frame.headerControl ? frame.headerControl.implicitHeight : 0
                }

                Text {
                    opacity: frame.useSheet ? 1 : 0
                    text: "›"
                    font.pointSize: 18
                    color: Utility.getAppHexColor("disabledText")
                }
            }

            ColumnLayout {
                id: inlineArea
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 12
                Layout.bottomMargin: 6
                visible: frame.expanded
                spacing: 8

                ColumnLayout {
                    id: editorColumn
                    parent: frame.sheet ? frame.sheet.column : inlineArea
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    id: helpArea
                    property bool full: false

                    parent: frame.sheet ? frame.sheet.column : inlineArea
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        id: helpText
                        Layout.fillWidth: true
                        Layout.preferredHeight: helpArea.full ? implicitHeight : Math.min(implicitHeight, helpMetrics.lineSpacing * 3)
                        visible: text !== ""
                        clip: true
                        wrapMode: Text.WordWrap
                        textFormat: Text.RichText
                        font.pointSize: 10
                        color: Utility.getAppHexColor("normalText")

                        FontMetrics {
                            id: helpMetrics
                            font: helpText.font
                        }
                        linkColor: Utility.getAppHexColor("lightAccent")
                        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Material.foreground: Utility.getAppHexColor("accentText")

                        Button {
                            id: nowButton
                            Layout.fillWidth: true
                            flat: true
                            focusPolicy: Qt.NoFocus
                            text: qsTr("Current")
                            onClicked: {
                                params.setUpdateOnly(paramName)
                                params.requestUpdate()
                            }
                        }

                        Button {
                            id: defaultButton
                            Layout.fillWidth: true
                            flat: true
                            focusPolicy: Qt.NoFocus
                            text: qsTr("Default")
                            onClicked: {
                                params.setUpdateOnly(paramName)
                                params.requestUpdateDefault()
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            flat: true
                            focusPolicy: Qt.NoFocus
                            text: qsTr("Help")
                            onClicked: helpArea.full = !helpArea.full
                        }
                    }
                }
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            height: 52
            z: -1
            onClicked: {
                if (frame.useSheet) {
                    frame.sheet.open()
                } else {
                    frame.expanded = !frame.expanded
                }
            }
        }
    }

    Component {
        id: sheetComponent

        SheetDialog {
            property alias column: sheetColumn
            title: nameText.text

            ScrollView {
                anchors.fill: parent
                contentWidth: availableWidth
                clip: true

                ColumnLayout {
                    id: sheetColumn
                    width: parent.width
                }
            }
        }
    }
}
