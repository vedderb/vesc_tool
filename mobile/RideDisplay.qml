import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Vedder.vesc.utility 1.0

Dialog {
    id: dialog
    parent: Overlay.overlay
    readonly property var session: RideSession
    property bool connected: false
    readonly property real distanceFactor: RideSession.useImperial ? 0.000621371192 : 0.001
    readonly property string distanceUnit: RideSession.useImperial ? " mi" : " km"
    readonly property real speedFactor: RideSession.useImperial ? 2.236936292 : 3.6
    readonly property color muted: Utility.getAppHexColor("normalText")
    readonly property color ink: Utility.getAppHexColor("lightText")
    readonly property bool showRange: RideSession.active && RideSession.fresh && RideSession.estimatedRangeMeters >= 0
    readonly property string elapsed: Math.floor(RideSession.elapsedSeconds / 3600).toString().padStart(2, "0") + ":" +
                                      Math.floor((RideSession.elapsedSeconds % 3600) / 60).toString().padStart(2, "0") + ":" +
                                      Math.floor(RideSession.elapsedSeconds % 60).toString().padStart(2, "0")
    readonly property string notice: !RideSession.enabled ? qsTr("Allow Live Activities in Settings") :
                                     RideSession.active && !RideSession.fresh ? qsTr("Waiting for connection") :
                                     RideSession.distanceIncomplete ? qsTr("Distance may be incomplete") :
                                     !RideSession.active && !connected ? qsTr("Connect to start") : ""

    modal: true
    focus: true
    padding: 0
    width: Math.min(parent ? parent.width - 32 : 360, 380)
    height: Math.min(implicitHeight, parent ? parent.height - 32 : implicitHeight)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    header: null
    footer: null
    background: Rectangle { radius: 8; color: "transparent" }
    onOpened: RideSession.refreshAvailability()
    Overlay.modal: Rectangle { color: "#AA000000" }

    contentItem: Rectangle {
        implicitHeight: panel.implicitHeight + 40
        color: Utility.getAppHexColor("darkBackground")
        radius: 8
        border.color: Utility.getAppHexColor("lightBackground")

        ScrollView {
            id: scroll
            anchors.fill: parent
            anchors.margins: 20
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                id: panel
                width: scroll.availableWidth
                spacing: 18

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Image {
                        Layout.preferredWidth: 18
                        Layout.preferredHeight: 18
                        source: "../res/icons/v_icon-256.png"
                        fillMode: Image.PreserveAspectFit
                        opacity: 0.86
                    }
                    Label {
                        text: qsTr("RIDE")
                        font.pixelSize: 13
                        font.bold: true
                        font.letterSpacing: 2
                        color: dialog.ink
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: dialog.elapsed
                        font.pixelSize: 13
                        color: dialog.muted
                    }
                    ToolButton {
                        text: "×"
                        implicitWidth: 36
                        implicitHeight: 36
                        font.pixelSize: 24
                        Accessible.name: qsTr("Close ride panel")
                        onClicked: dialog.close()
                        contentItem: Label {
                            text: parent.text
                            font: parent.font
                            color: dialog.muted
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Item {}
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: -4
                    Label {
                        Layout.fillWidth: true
                        text: RideSession.active && RideSession.fresh ?
                                  (Math.abs(RideSession.speedMps) * dialog.speedFactor).toFixed(0) : "—"
                        font.pixelSize: 76
                        font.family: "Roboto"
                        font.weight: Font.Normal
                        color: RideSession.fresh ? dialog.ink : dialog.muted
                        horizontalAlignment: Text.AlignHCenter
                        Accessible.name: qsTr("Speed") + ": " + text
                    }
                    Label {
                        Layout.fillWidth: true
                        text: RideSession.useImperial ? "mph" : "km/h"
                        font.pixelSize: 14
                        font.family: "Roboto"
                        font.capitalization: Font.AllUppercase
                        color: dialog.muted
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Utility.getAppHexColor("lightBackground")
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    Repeater {
                        model: [
                            {label: qsTr("TRIP"), value: (RideSession.distanceIncomplete ? "≥ " : "") +
                                (RideSession.distanceMeters * dialog.distanceFactor).toFixed(2) + dialog.distanceUnit},
                            {label: qsTr("BATTERY"), value: RideSession.fresh && RideSession.batteryLevel >= 0 ?
                                Math.round(RideSession.batteryLevel * 100) + "%" : "—"},
                            {label: qsTr("EST. RANGE"), value:
                                (RideSession.estimatedRangeMeters * dialog.distanceFactor).toFixed(1) + dialog.distanceUnit}
                        ]
                        delegate: ColumnLayout {
                            visible: index < 2 || dialog.showRange
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            Layout.minimumWidth: 0
                            spacing: 4
                            property int alignment: index === 0 ? Text.AlignLeft :
                                                    index === 1 && dialog.showRange ? Text.AlignHCenter : Text.AlignRight
                            Label {
                                Layout.fillWidth: true
                                horizontalAlignment: parent.alignment
                                text: modelData.label
                                font.pixelSize: 10
                                font.letterSpacing: 1
                                color: dialog.muted
                            }
                            Label {
                                Layout.fillWidth: true
                                horizontalAlignment: parent.alignment
                                text: modelData.value
                                font.pixelSize: dialog.showRange ? 20 : 22
                                fontSizeMode: Text.Fit
                                minimumPixelSize: 14
                                color: dialog.ink
                            }
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: dialog.notice
                    visible: text.length > 0
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                    color: dialog.muted
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    Layout.fillWidth: true
                    text: RideSession.error
                    visible: text.length > 0
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                    color: dialog.ink
                }

                Button {
                    id: rideButton
                    Layout.fillWidth: true
                    implicitHeight: 46
                    text: RideSession.active ? qsTr("End Ride") : qsTr("Start Ride")
                    enabled: RideSession.active || (dialog.connected && RideSession.enabled)
                    onClicked: RideSession.active ? RideSession.end() : RideSession.start()
                    contentItem: Label {
                        text: rideButton.text
                        color: rideButton.enabled ? dialog.ink : dialog.muted
                        font.pixelSize: 15
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 4
                        color: rideButton.down ? Utility.getAppHexColor("lightestBackground") :
                               RideSession.active || !rideButton.enabled ? Utility.getAppHexColor("lightBackground") :
                               Utility.getAppHexColor("darkAccent")
                    }
                }
            }
        }
    }
}
