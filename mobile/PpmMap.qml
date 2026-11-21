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

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    id: root
    implicitHeight: column.implicitHeight

    property real msMin: 0.0
    property real msMax: 0.0
    property real msCenter: 0.0
    property real msNow: 0.0
    property real valueNow: 0.0
    property bool resetDone: true
    // Off where the host applies the mapping, like the setup wizard
    property bool showApply: true

    property Commands mCommands: VescIf.commands()
    property ConfigParams mAppConf: VescIf.appConfig()

    // One channel: the live input on its full span with the captured range, and the output
    // of the input as it is mapped now. AdcMap uses it as PpmMap.MapChannel
    component MapChannel: ColumnLayout {
        id: channel
        property alias title: titleText.text
        property string unit: ""
        property int decimals: 2
        property real from: 0.0
        property real to: 1.0
        property real now: 0.0
        property real start: 0.0
        property real end: 0.0
        // Below 0 there is no center mark
        property real center: -1.0
        property real output: 0.0
        // Output from -1 to 1 instead of 0 to 1
        property bool centered: false
        readonly property bool captured: end > start
        // The value labels need some room, not while the range only starts to open up
        readonly property bool labelsFit: captured && posOf(end) - posOf(start) > 100

        function posOf(v) {
            return (Math.min(Math.max(v, from), to) - from) / (to - from) * span.width
        }

        Layout.fillWidth: true
        spacing: 4

        RowLayout {
            Layout.fillWidth: true

            Column {
                Layout.fillWidth: true

                Text {
                    id: titleText
                    color: Utility.getAppHexColor("normalText")
                    font.pointSize: 9
                    font.letterSpacing: 0.5
                    font.capitalization: Font.AllUppercase
                }

                Text {
                    text: (channel.centered && channel.output > 0 ? "+" : "") + Math.round(channel.output * 100) + " %"
                    color: Utility.getAppHexColor("titleText")
                    font.pointSize: 22
                    font.weight: Font.Medium
                }
            }

            Text {
                Layout.alignment: Qt.AlignBottom
                Layout.bottomMargin: 4
                text: channel.now.toFixed(channel.decimals + 1) + " " + channel.unit
                color: Utility.getAppHexColor("normalText")
                font.pointSize: 11
            }
        }

        Item {
            Layout.fillWidth: true
            implicitHeight: 22

            // Inset by the knob radius, so the knob stays inside at the ends
            Item {
                id: span
                anchors.fill: parent
                anchors.leftMargin: 9
                anchors.rightMargin: 9

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    height: 6
                    radius: 3
                    color: Utility.getAppHexColor("raisedBackground")
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    x: channel.posOf(channel.start)
                    width: channel.posOf(channel.end) - x
                    height: 6
                    radius: 3
                    visible: channel.captured
                    color: Utility.getAppHexColor("lightAccent")
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    x: channel.posOf(channel.center) - 1
                    width: 2
                    height: 14
                    radius: 1
                    visible: channel.captured && channel.center >= 0
                    color: Utility.getAppHexColor("titleText")
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    x: channel.posOf(channel.now) - width / 2
                    width: 18
                    height: 18
                    radius: 9
                    color: Utility.getAppHexColor("titleText")
                    border.width: 4
                    border.color: Utility.getAppHexColor("lightAccent")
                }
            }
        }

        // The captured values under their places on the bar
        Item {
            Layout.fillWidth: true
            Layout.leftMargin: 9
            Layout.rightMargin: 9
            implicitHeight: 14

            Repeater {
                model: channel.labelsFit ? (channel.center >= 0 ? [channel.start, channel.center, channel.end]
                                                               : [channel.start, channel.end]) : []

                Text {
                    x: Math.min(Math.max(channel.posOf(modelData) - width / 2, 0), parent.width - width)
                    text: modelData.toFixed(channel.decimals) + " " + channel.unit
                    color: Utility.getAppHexColor("normalText")
                    font.pointSize: 10
                }
            }
        }
    }

    // Green dot and the next step once the range is captured
    component MapStatus: RowLayout {
        id: status
        property bool valid: false
        property alias text: statusText.text

        Layout.fillWidth: true
        spacing: 8

        Rectangle {
            implicitWidth: 8
            implicitHeight: 8
            radius: 4
            color: Utility.getAppHexColor(status.valid ? "green" : "lightAccent")
        }

        Text {
            id: statusText
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Utility.getAppHexColor("lightText")
            font.pointSize: 11
        }
    }

    component MapButtons: RowLayout {
        id: buttons
        property string helpName: ""
        property bool showApply: true
        property bool canApply: false
        signal reset()
        signal apply()

        Layout.fillWidth: true
        spacing: 8

        ToolButton {
            icon.source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
            Accessible.name: qsTr("Help")
            onClicked: VescIf.emitMessageDialog(VescIf.infoConfig().getLongName(buttons.helpName),
                                                VescIf.infoConfig().getDescription(buttons.helpName), true, true)
        }

        Button {
            Layout.fillWidth: buttons.showApply
            text: qsTr("Reset")
            flat: true
            onClicked: buttons.reset()
        }

        Button {
            Layout.fillWidth: true
            visible: buttons.showApply
            enabled: buttons.canApply
            highlighted: true
            text: qsTr("Apply && write")
            onClicked: buttons.apply()
        }
    }

    function isValid() {
        return (msMax - msMin) > 0.4
    }

    function applyMapping() {
        mAppConf.updateParamDouble("app_ppm_conf.pulse_start", msMin)
        mAppConf.updateParamDouble("app_ppm_conf.pulse_end", msMax)
        mAppConf.updateParamDouble("app_ppm_conf.pulse_center", msCenter)
        VescIf.emitStatusMessage("Start, End and Center Pulselengths Applied", true)
        mCommands.setAppConf()
    }

    function reset() {
        msMin = 0.0
        msMax = 0.0
        msCenter = 0.0
        resetDone = true
    }

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 12

        MapChannel {
            title: qsTr("Pulse width")
            unit: "ms"
            from: 0.5
            to: 2.5
            now: msNow
            start: msMin
            end: msMax
            center: msCenter
            output: valueNow
            centered: true
        }

        MapStatus {
            valid: isValid()
            text: !isValid() ? qsTr("Move the throttle to both ends of its travel") :
                               showApply ? qsTr("Let it return to the center, then apply") :
                                           qsTr("Let it return to the center")
        }

        MapButtons {
            helpName: "app_ppm_mapping_help"
            showApply: root.showApply
            canApply: isValid()
            onReset: root.reset()
            onApply: applyMapping()
        }
    }

    Timer {
        id: rtTimer
        interval: 50
        running: true
        repeat: true

        onTriggered: {
            if (VescIf.isPortConnected() && visible) {
                mCommands.getDecodedPpm()
            }
        }
    }

    Connections {
        target: mCommands

        function onDecodedPpmReceived(value, last_len) {
            valueNow = value
            msNow = last_len

            // The firmware reports 0 until it has received a pulse
            if (msNow < 1e-3) {
                return
            }

            if (resetDone) {
                resetDone = false
                msMin = msNow
                msMax = msNow
            }

            if (msNow < msMin) {
                msMin = msNow
            }

            if (msNow > msMax) {
                msMax = msNow
            }

            var range = msMax - msMin
            var pos = msNow - msMin

            if (pos > (range / 4.0) && pos < ((3.0 * range) / 4.0)) {
                msCenter = msNow
            } else {
                msCenter = range / 2.0 + msMin
            }
        }
    }
}
