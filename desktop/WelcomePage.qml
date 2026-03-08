/*
    Desktop WelcomePage — faithful recreation of the original PageWelcome widget.
    Left side: Welcome text + Quick Configuration grid (8 buttons)
    Right side: QML panel (version info placeholder)
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    id: welcomePage

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // Left side: Background + Welcome text + Quick Config buttons
        Item {
            SplitView.preferredWidth: parent.width * 0.55
            SplitView.minimumWidth: 350

            // Background image
            Image {
                anchors.fill: parent
                source: "qrc:/res/bg.png"
                fillMode: Image.PreserveAspectCrop
                opacity: 0.15
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 16

                // Welcome text
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: "<p style='font-size:20pt;'>Welcome to <b>VESC® Tool</b></p>" +
                          "<p style='font-size:14pt;'>To get started, you can use the " +
                          "<i>Setup Wizards</i> to configure the motor and app settings " +
                          "respectively. Otherwise, you can connect to your VESC in the " +
                          "<i>Connection</i> page and go through the motor and app " +
                          "configuration pages manually.</p>"
                    textFormat: Text.RichText
                    wrapMode: Text.WordWrap
                    color: Utility.getAppHexColor("lightText")
                }

                // Quick Configuration group
                GroupBox {
                    title: "Quick Configuration"
                    Layout.fillWidth: true

                    GridLayout {
                        columns: 2
                        rowSpacing: 6
                        columnSpacing: 6
                        anchors.fill: parent

                        Button {
                            text: "AutoConnect"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                            icon.width: 45; icon.height: 45
                            Layout.fillWidth: true
                            ToolTip.text: "Automatically connect to the VESC using the USB connection."
                            ToolTip.visible: hovered
                            onClicked: VescIf.autoconnect()
                        }

                        Button {
                            text: "Setup Motors FOC"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Wizard-96.png"
                            icon.width: 45; icon.height: 45
                            Layout.fillWidth: true
                            ToolTip.text: "Simple FOC setup for all VESCs on CAN-bus"
                            ToolTip.visible: hovered
                            onClicked: {
                                // TODO: launch DetectAllFocDialog equivalent
                                VescIf.emitMessageDialog("Setup Motors FOC",
                                    "FOC detection wizard will be available in a future update.", false, false)
                            }
                        }

                        Button {
                            text: "Setup IMU"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/imu_off.png"
                            icon.width: 45; icon.height: 45
                            Layout.fillWidth: true
                            onClicked: {
                                VescIf.emitMessageDialog("Setup IMU",
                                    "IMU setup wizard will be available in a future update.", false, false)
                            }
                        }

                        Button {
                            text: "Setup Input"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Wizard-96.png"
                            icon.width: 45; icon.height: 45
                            Layout.fillWidth: true
                            onClicked: {
                                VescIf.emitMessageDialog("Setup Input",
                                    "Input setup wizard will be available in a future update.", false, false)
                            }
                        }

                        Button {
                            text: "Multi Settings"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Settings-96.png"
                            icon.width: 45; icon.height: 45
                            Layout.fillWidth: true
                            onClicked: {
                                VescIf.emitMessageDialog("Multi Settings",
                                    "Multi settings wizard will be available in a future update.", false, false)
                            }
                        }

                        Button {
                            text: "NRF Quick Pair"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/icons8-fantasy-96.png"
                            icon.width: 45; icon.height: 45
                            Layout.fillWidth: true
                            onClicked: {
                                VescIf.emitMessageDialog("NRF Quick Pair",
                                    "NRF pairing will be available in a future update.", false, false)
                            }
                        }

                        Button {
                            text: "Setup Bluetooth Module"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/bluetooth.png"
                            icon.width: 45; icon.height: 45
                            Layout.fillWidth: true
                            onClicked: {
                                VescIf.emitMessageDialog("Setup Bluetooth",
                                    "Bluetooth module setup will be available in a future update.", false, false)
                            }
                        }

                        Button {
                            text: "Invert Motor Directions"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Process-96.png"
                            icon.width: 45; icon.height: 45
                            Layout.fillWidth: true
                            onClicked: {
                                VescIf.emitMessageDialog("Invert Motor Directions",
                                    "Direction inversion will be available in a future update.", false, false)
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Right side: Info panel
        Item {
            SplitView.preferredWidth: parent.width * 0.45
            SplitView.minimumWidth: 250

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 16

                Image {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Math.min(parent.width * 0.5, 250)
                    Layout.preferredHeight: (sourceSize.height * Layout.preferredWidth) / sourceSize.width
                    source: "qrc" + Utility.getThemePath() + "logo.png"
                    fillMode: Image.PreserveAspectFit
                    antialiasing: true
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: "VESC® Tool"
                    font.pixelSize: 28
                    font.bold: true
                    color: Utility.getAppHexColor("lightText")
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Version " + Utility.versionText()
                    font.pixelSize: 14
                    color: Utility.getAppHexColor("normalText")
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: "Connect your VESC using the Connection page or the toolbar buttons.\n\n" +
                          "Motor and App configuration pages will appear in the sidebar once connected."
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    color: Utility.getAppHexColor("disabledText")
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
