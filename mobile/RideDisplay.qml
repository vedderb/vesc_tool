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
import QtQuick.Layouts

import Vedder.vesc.utility 1.0

// Live ride data in the app, mirrored by the Live Activity (iOS) or the ride notification (Android)
SheetDialog {
    id: dialog
    readonly property var session: RideSession
    property bool connected: false
    readonly property real distanceFactor: RideSession.useImperial ? 0.000621371192 : 0.001
    readonly property string distanceUnit: RideSession.useImperial ? "mi" : "km"
    readonly property real speedFactor: RideSession.useImperial ? 2.236936292 : 3.6
    readonly property bool showRange: RideSession.active && RideSession.fresh && RideSession.estimatedRangeMeters >= 0
    readonly property string notice: !RideSession.enabled ? (Qt.platform.os === "android" ?
                                                                 qsTr("Allow notifications for VESC Tool in Settings") :
                                                                 qsTr("Allow Live Activities in Settings")) :
                                     RideSession.active && !RideSession.fresh ? qsTr("Waiting for connection") :
                                     RideSession.distanceIncomplete ? qsTr("Distance may be incomplete") :
                                     !RideSession.active && !connected ? qsTr("Connect to start") : ""

    title: qsTr("Ride")
    acceptText: RideSession.active ? qsTr("End ride") : connected && RideSession.enabled ? qsTr("Start ride") : ""
    rejectText: qsTr("Close")
    onOpened: RideSession.refreshAvailability()
    onAccepted: {
        if (RideSession.active) {
            RideSession.end()
        } else {
            // Stay open to show the ride, or why it could not start
            RideSession.start()
            open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Text {
            Layout.alignment: Qt.AlignRight
            text: new Date(RideSession.elapsedSeconds * 1000).toISOString().substr(11, 8)
            font.features: { "tnum": 1 }
            color: Utility.getAppHexColor("normalText")
        }

        Text {
            Layout.fillWidth: true
            text: RideSession.active && RideSession.fresh ?
                      (RideSession.speedMps * dialog.speedFactor).toFixed(0) : "\u2014"
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 54
            font.weight: Font.Medium
            font.features: { "tnum": 1 }
            color: Utility.getAppHexColor(RideSession.fresh ? "titleText" : "normalText")
            Accessible.name: qsTr("Speed") + ": " + text
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: -12
            text: RideSession.useImperial ? "mph" : "km/h"
            horizontalAlignment: Text.AlignHCenter
            font.capitalization: Font.AllUppercase
            color: Utility.getAppHexColor("normalText")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: [[qsTr("Trip"), (RideSession.distanceIncomplete ? "\u2265 " : "") +
                         (RideSession.distanceMeters * dialog.distanceFactor).toFixed(2), dialog.distanceUnit],
                        [qsTr("Battery"), RideSession.fresh && RideSession.batteryLevel >= 0 ?
                             Math.round(RideSession.batteryLevel * 100) : "\u2014", "%"],
                        [qsTr("Est. Range"), (RideSession.estimatedRangeMeters * dialog.distanceFactor).toFixed(1),
                         dialog.distanceUnit]]

                ValueTile {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    visible: index < 2 || dialog.showRange
                    title: modelData[0]
                    value: modelData[1]
                    unit: modelData[2]
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: text !== ""
            text: RideSession.error !== "" ? RideSession.error : dialog.notice
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            color: Utility.getAppHexColor(RideSession.error !== "" ? "dangerText" : "normalText")
        }
    }
}
