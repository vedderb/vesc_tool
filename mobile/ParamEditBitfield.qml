/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: editor
    property string paramName: ""
    property ConfigParams params: null
    height: 240
    Layout.fillWidth: true
    property real maxVal: 1.0
    property bool createReady: false

    Component.onCompleted: {
        if (params != null) {
            nameText.text = params.getLongName(paramName)
            setBits(params.getParamInt(paramName))

            var names = params.getParamEnumNames(paramName)
            for (var i = 0;i < names.length;i++) {
                if (i === 0) b0Box.text = names[i]
                if (i === 1) b1Box.text = names[i]
                if (i === 2) b2Box.text = names[i]
                if (i === 3) b3Box.text = names[i]
                if (i === 4) b4Box.text = names[i]
                if (i === 5) b5Box.text = names[i]
                if (i === 6) b6Box.text = names[i]
                if (i === 7) b7Box.text = names[i]
            }

            b0Box.visible = b0Box.text.toLowerCase() != "unused"
            b1Box.visible = b1Box.text.toLowerCase() != "unused"
            b2Box.visible = b2Box.text.toLowerCase() != "unused"
            b3Box.visible = b3Box.text.toLowerCase() != "unused"
            b4Box.visible = b4Box.text.toLowerCase() != "unused"
            b5Box.visible = b5Box.text.toLowerCase() != "unused"
            b6Box.visible = b6Box.text.toLowerCase() != "unused"
            b7Box.visible = b7Box.text.toLowerCase() != "unused"

            var visibleCount = 0
            if (b0Box.text.toLowerCase() != "unused") visibleCount++
            if (b1Box.text.toLowerCase() != "unused") visibleCount++
            if (b2Box.text.toLowerCase() != "unused") visibleCount++
            if (b3Box.text.toLowerCase() != "unused") visibleCount++
            if (b4Box.text.toLowerCase() != "unused") visibleCount++
            if (b5Box.text.toLowerCase() != "unused") visibleCount++
            if (b6Box.text.toLowerCase() != "unused") visibleCount++
            if (b7Box.text.toLowerCase() != "unused") visibleCount++

            visibleCount = Math.ceil(visibleCount / 2.0) - 1.0
            if (visibleCount < 0.0) visibleCount = 0.0

            height = 120 + visibleCount * 40
            Layout.preferredHeight = height

            if (params.getParamTransmittable(paramName)) {
                nowButton.visible = true
                defaultButton.visible = true
            } else {
                nowButton.visible = false
                defaultButton.visible = false
            }

            createReady = true
        }
    }

    function setBits(bits) {
        b0Box.checked = (bits >> 0) & 1
        b1Box.checked = (bits >> 1) & 1
        b2Box.checked = (bits >> 2) & 1
        b3Box.checked = (bits >> 3) & 1
        b4Box.checked = (bits >> 4) & 1
        b5Box.checked = (bits >> 5) & 1
        b6Box.checked = (bits >> 6) & 1
        b7Box.checked = (bits >> 7) & 1
    }

    function updateFromBoxes() {
        var res = 0;

        if (b0Box.checked) res += 1 << 0
        if (b1Box.checked) res += 1 << 1
        if (b2Box.checked) res += 1 << 2
        if (b3Box.checked) res += 1 << 3
        if (b4Box.checked) res += 1 << 4
        if (b5Box.checked) res += 1 << 5
        if (b6Box.checked) res += 1 << 6
        if (b7Box.checked) res += 1 << 7

        if (params !== null && createReady) {
            if (params.getUpdateOnly() !== paramName) {
                params.setUpdateOnly("")
            }
            params.updateParamInt(paramName, res, editor);
        }
    }

    Rectangle {
        id: rect
        anchors.fill: parent
        color: Utility.getAppHexColor("lightBackground")
        radius: 5
        border.color:  Utility.getAppHexColor("disabledText")
        border.width: 2

        ColumnLayout {
            id: column
            anchors.fill: parent
            anchors.topMargin: 10
            anchors.margins: 5

            Text {
                id: nameText
                color: Utility.getAppHexColor("lightText")
                text: paramName
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                font.pointSize: 12
            }

            GridLayout {
                Layout.fillWidth: true;
                columns: 2
                CheckBox { Layout.topMargin: 0; Layout.bottomMargin: -8; Layout.fillWidth: true;
                    id: b0Box; text: "B0"; onCheckedChanged: updateFromBoxes(); }
                CheckBox { Layout.topMargin: 0; Layout.bottomMargin: -8; Layout.fillWidth: true;
                    id: b1Box; text: "B1"; onCheckedChanged: updateFromBoxes(); }
                CheckBox { Layout.topMargin: -8; Layout.bottomMargin: -8; Layout.fillWidth: true;
                    id: b2Box; text: "B2"; onCheckedChanged: updateFromBoxes(); }
                CheckBox { Layout.topMargin: -8; Layout.bottomMargin: -8; Layout.fillWidth: true;
                    id: b3Box; text: "B3"; onCheckedChanged: updateFromBoxes(); }
                CheckBox { Layout.topMargin: -8; Layout.bottomMargin: -8; Layout.fillWidth: true;
                    id: b4Box; text: "B4"; onCheckedChanged: updateFromBoxes(); }
                CheckBox { Layout.topMargin: -8; Layout.bottomMargin: -8; Layout.fillWidth: true;
                    id: b5Box; text: "B5"; onCheckedChanged: updateFromBoxes(); }
                CheckBox { Layout.topMargin: -8; Layout.bottomMargin: -8; Layout.fillWidth: true;
                    id: b6Box; text: "B6"; onCheckedChanged: updateFromBoxes(); }
                CheckBox { Layout.topMargin: -8; Layout.bottomMargin: -8; Layout.fillWidth: true;
                    id: b7Box; text: "B7"; onCheckedChanged: updateFromBoxes(); }
            }

            RowLayout {
                Layout.fillWidth: true
                Button {
                    id: nowButton
                    Layout.fillWidth: true
                    Layout.preferredWidth: 500
                    flat: true
                    text: "Current"
                    onClicked: {
                        params.setUpdateOnly(paramName)
                        params.requestUpdate()
                    }
                }

                Button {
                    id: defaultButton
                    Layout.fillWidth: true
                    Layout.preferredWidth: 500
                    flat: true
                    text: "Default"
                    onClicked: {
                        params.setUpdateOnly(paramName)
                        params.requestUpdateDefault()
                    }
                }

                Button {
                    id: helpButton
                    Layout.fillWidth: true
                    Layout.preferredWidth: 500
                    flat: true
                    text: "Help"
                    onClicked: {
                        VescIf.emitMessageDialog(
                                    params.getLongName(paramName),
                                    params.getDescription(paramName),
                                    true, true)
                    }
                }
            }
        }
    }

    Connections {
        target: params

        function onParamChangedInt(src, name, newParam) {
            if (src !== editor && name === paramName) {
                setBits(newParam)
            }
        }
    }
}
