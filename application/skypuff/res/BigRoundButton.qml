import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12


RoundButton {
    property bool busy: false
    property real iconPercent: 32
    property int size: 90
    id: control
    icon.width: implicitWidth * iconPercent / 100
    icon.height: implicitHeight * iconPercent / 100
    implicitWidth: size
    implicitHeight: size

    BusyIndicator {
        id: busyIndicator
        z: -1
        anchors.centerIn: control
        implicitWidth: size + 7
        implicitHeight: size + 7
        running: false

        NumberAnimation {
            id: busyOn
            target: busyIndicator
            properties: "opacity"; duration: 3000;
            from: 0
            to: 0.4
        }

        NumberAnimation {
            id: busyOff
            target: busyIndicator
            properties: "opacity"; duration: 2000;
            from: 0.4
            to: 0
            onFinished: {busyIndicator.running = false; control.enabled = true }
        }

        states: [
            State {
                name: "busy"; when: control.busy
                PropertyChanges {
                    target: busyIndicator;
                    restoreEntryValues: false
                    running: true
                }
                PropertyChanges {
                    target: control;
                    restoreEntryValues: false
                    enabled: false
                }
                PropertyChanges {
                    target: busyOn;
                    running: true
                }

            }
        ]
        transitions: Transition {
            from: "busy"
            onRunningChanged: {
                if(running)
                    busyOff.start()
            }
        }
    }

    background: Rectangle {
        implicitWidth: control.Material.buttonHeight
        implicitHeight: control.Material.buttonHeight

        radius: control.radius
        color: !control.enabled ? control.Material.buttonDisabledColor
            : control.checked || control.highlighted ? control.Material.highlightedButtonColor : control.Material.buttonColor

        Rectangle {
            width: parent.width
            height: parent.height
            radius: control.radius
            visible: control.hovered || control.visualFocus
            color: control.Material.rippleColor
        }

        Rectangle {
            width: parent.width
            height: parent.height
            radius: control.radius
            visible: control.down
            color: control.Material.rippleColor
        }

        // The layer is disabled when the button color is transparent so that you can do
        // Material.background: "transparent" and get a proper flat button without needing
        // to set Material.elevation as well
        layer.enabled: control.enabled && control.Material.buttonColor.a > 0
        layer.effect: ElevationEffect {
            elevation: control.Material.elevation
        }
    }
}
