/*
    Desktop ImuPage — standalone IMU data display with 3D orientation view.
    Native implementation using QtQuick3D (no mobile dependencies).
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import Vedder.vesc

Item {
    id: imuPage

    property Commands mCommands: VescIf.commands()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        // IMU data display
        GridLayout {
            Layout.fillWidth: true
            columns: 6
            rowSpacing: 4
            columnSpacing: 12

            Label { text: "Roll:"; font.bold: true }
            Label { id: rollLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
            Label { text: "Pitch:"; font.bold: true }
            Label { id: pitchLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
            Label { text: "Yaw:"; font.bold: true }
            Label { id: yawLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }

            Label { text: "Acc X:"; font.bold: true }
            Label { id: accXLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
            Label { text: "Acc Y:"; font.bold: true }
            Label { id: accYLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
            Label { text: "Acc Z:"; font.bold: true }
            Label { id: accZLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }

            Label { text: "Gyro X:"; font.bold: true }
            Label { id: gyroXLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
            Label { text: "Gyro Y:"; font.bold: true }
            Label { id: gyroYLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
            Label { text: "Gyro Z:"; font.bold: true }
            Label { id: gyroZLabel; text: "--"; font.family: "DejaVu Sans Mono"; color: Utility.getAppHexColor("lightText") }
        }

        CheckBox {
            id: useYawBox
            text: "Use Yaw (will drift)"
            checked: false
        }

        // 3D orientation view
        View3D {
            id: imu3dView
            Layout.fillWidth: true
            Layout.fillHeight: true

            property real roll: 0.1
            property real pitch: 0.75
            property real yaw: 0.4

            environment: SceneEnvironment {
                clearColor: Utility.getAppHexColor("plotBackground")
                backgroundMode: SceneEnvironment.Color
                antialiasingMode: SceneEnvironment.MSAA
                antialiasingQuality: SceneEnvironment.High
            }

            PerspectiveCamera {
                position: Qt.vector3d(0, 200, 300)
                eulerRotation.x: -30
            }

            DirectionalLight {
                eulerRotation.x: -30
                eulerRotation.y: -70
                color: "#ffffff"
                brightness: 1.0
            }

            DirectionalLight {
                eulerRotation.x: 30
                eulerRotation.y: 70
                color: "#888888"
                brightness: 0.5
            }

            Model {
                source: "#Cube"
                scale: Qt.vector3d(2.0, 0.3, 1.2)

                materials: [
                    PrincipledMaterial {
                        baseColor: Utility.getAppHexColor("midAccent")
                        roughness: 0.4
                        metalness: 0.1
                    }
                ]

                eulerRotation: Qt.vector3d(
                    imu3dView.pitch * 180 / Math.PI,
                    imu3dView.yaw * 180 / Math.PI,
                    imu3dView.roll * 180 / Math.PI
                )
            }
        }
    }

    Connections {
        target: mCommands
        function onValuesImuReceived(values, mask) {
            rollLabel.text = (values.roll * 180 / Math.PI).toFixed(1) + " °"
            pitchLabel.text = (values.pitch * 180 / Math.PI).toFixed(1) + " °"
            yawLabel.text = (values.yaw * 180 / Math.PI).toFixed(1) + " °"
            accXLabel.text = values.accX.toFixed(3) + " G"
            accYLabel.text = values.accY.toFixed(3) + " G"
            accZLabel.text = values.accZ.toFixed(3) + " G"
            gyroXLabel.text = values.gyroX.toFixed(1) + " °/s"
            gyroYLabel.text = values.gyroY.toFixed(1) + " °/s"
            gyroZLabel.text = values.gyroZ.toFixed(1) + " °/s"

            imu3dView.roll = values.roll
            imu3dView.pitch = values.pitch
            imu3dView.yaw = useYawBox.checked ? values.yaw : 0
        }
    }
}
