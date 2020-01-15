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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    property int parentWidth: 10
    property int parentHeight: 10

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mInfoConf: VescIf.infoConfig()

    function openDialog() {
        dialog.open()
        opacitySlider.value = 1
    }

    function testConnected() {
        if (VescIf.isPortConnected()) {
            return true
        } else {
            VescIf.emitMessageDialog(
                        "Connection Error",
                        "The VESC is not connected. Please connect it to run detection.",
                        false, false)
            return false
        }
    }

    Dialog {
        id: dialog
        standardButtons: Dialog.Close
        modal: false
        focus: true
        width: parentWidth - 40
        height: parentHeight - 40
        closePolicy: Popup.CloseOnEscape
        x: 20
        y: 10
        background.opacity: opacitySlider.value

        ColumnLayout {
            anchors.fill: parent

            SwipeView {
                id: swipeView
                currentIndex: tabBar.currentIndex
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true

                Page {
                    background: Rectangle {
                        opacity: 0.0
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        DoubleSpinBox {
                            id: currentBox
                            Layout.fillWidth: true
                            decimals: 1
                            prefix: "I: "
                            suffix: " A"
                            realFrom: 0.0
                            realTo: 500.0
                            realValue: 3.0
                            realStepSize: 1.0
                        }

                        Rectangle {
                            Layout.fillHeight: true
                        }

                        CheckBox {
                            id: maintainCurrentBox
                            text: "Continue after release"
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 100
                                text: "Set REV"

                                onPressedChanged: {
                                    if (pressed) {
                                        mCommands.setCurrent(-currentBox.realValue)
                                    } else if (!maintainCurrentBox.checked) {
                                        mCommands.setCurrent(0.0)
                                    }
                                }
                            }

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 100
                                text: "Set FWD"

                                onPressedChanged: {
                                    if (pressed) {
                                        mCommands.setCurrent(currentBox.realValue)
                                    } else if (!maintainCurrentBox.checked) {
                                        mCommands.setCurrent(0.0)
                                    }
                                }
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Brake"

                            onPressedChanged: {
                                if (pressed) {
                                    mCommands.setCurrentBrake(currentBox.realValue)
                                } else if (!maintainCurrentBox.checked) {
                                    mCommands.setCurrent(0.0)
                                }
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Release"

                            onClicked: {
                                mCommands.setCurrent(0.0)
                            }
                        }
                    }
                }

                Page {
                    background: Rectangle {
                        opacity: 0.0
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        DoubleSpinBox {
                            id: dutyBox
                            Layout.fillWidth: true
                            prefix: "D: "
                            decimals: 3
                            realFrom: 0.0
                            realTo: 1.0
                            realValue: 0.1
                            realStepSize: 0.01
                        }

                        Rectangle {
                            Layout.fillHeight: true
                        }

                        CheckBox {
                            id: maintainDutyBox
                            text: "Continue after release"
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 100
                                text: "Set REV"

                                onPressedChanged: {
                                    if (pressed) {
                                        mCommands.setDutyCycle(-dutyBox.realValue)
                                    } else if (!maintainDutyBox.checked) {
                                        mCommands.setCurrent(0.0)
                                    }
                                }
                            }

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 100
                                text: "Set FWD"

                                onPressedChanged: {
                                    if (pressed) {
                                        mCommands.setDutyCycle(dutyBox.realValue)
                                    } else if (!maintainDutyBox.checked) {
                                        mCommands.setCurrent(0.0)
                                    }
                                }
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "0 Duty (brake)"

                            onPressedChanged: {
                                if (pressed) {
                                    mCommands.setDutyCycle(0.0)
                                } else if (!maintainDutyBox.checked) {
                                    mCommands.setCurrent(0.0)
                                }
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Release"

                            onClicked: {
                                mCommands.setCurrent(0.0)
                            }
                        }
                    }
                }

                Page {
                    background: Rectangle {
                        opacity: 0.0
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        DoubleSpinBox {
                            id: speedBox
                            Layout.fillWidth: true
                            prefix: "\u03C9: "
                            suffix: " ERPM"
                            decimals: 1
                            realFrom: 0.0
                            realTo: 500000
                            realValue: 3000
                            realStepSize: 500
                        }

                        Rectangle {
                            Layout.fillHeight: true
                        }

                        CheckBox {
                            id: maintainSpeedBox
                            text: "Continue after release"
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 100
                                text: "Set REV"

                                onPressedChanged: {
                                    if (pressed) {
                                        mCommands.setRpm(-speedBox.realValue)
                                    } else if (!maintainSpeedBox.checked) {
                                        mCommands.setCurrent(0.0)
                                    }
                                }
                            }

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 100
                                text: "Set FWD"

                                onPressedChanged: {
                                    if (pressed) {
                                        mCommands.setRpm(speedBox.realValue)
                                    } else if (!maintainSpeedBox.checked) {
                                        mCommands.setCurrent(0.0)
                                    }
                                }
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "0 ERPM (brake)"

                            onPressedChanged: {
                                if (pressed) {
                                    mCommands.setRpm(0.0)
                                } else if (!maintainSpeedBox.checked) {
                                    mCommands.setCurrent(0.0)
                                }
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Release"

                            onClicked: {
                                mCommands.setCurrent(0.0)
                            }
                        }
                    }
                }

                Page {
                    background: Rectangle {
                        opacity: 0.0
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        DoubleSpinBox {
                            id: posBox
                            Layout.fillWidth: true
                            prefix: "P: "
                            suffix: " \u00B0"
                            decimals: 3
                            realFrom: 0
                            realTo: 360
                            realValue: 20
                            realStepSize: 0.1
                        }

                        Rectangle {
                            Layout.fillHeight: true
                        }

                        CheckBox {
                            id: maintainPosBox
                            text: "Continue after release"
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Set"

                            onPressedChanged: {
                                if (pressed) {
                                    mCommands.setPos(posBox.realValue)
                                } else if (!maintainPosBox.checked) {
                                    mCommands.setCurrent(0.0)
                                }
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Release"

                            onClicked: {
                                mCommands.setCurrent(0.0)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Text {
                    color: "white"
                    text: qsTr("Opacity")
                }

                Slider {
                    id: opacitySlider
                    Layout.fillWidth: true
                    from: 0.1
                    to: 1.0
                    value: 1.0
                }
            }
        }

        header: Rectangle {
            color: "#dbdbdb"
            height: tabBar.height
            opacity: opacitySlider.value

            TabBar {
                id: tabBar
                currentIndex: swipeView.currentIndex
                anchors.fill: parent
                implicitWidth: 0
                clip: true

                background: Rectangle {
                    opacity: 1
                    color: "#4f4f4f"
                }

                property int buttons: 4
                property int buttonWidth: 120

                TabButton {
                    text: qsTr("Current")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("Duty")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("RPM")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
                TabButton {
                    text: qsTr("Position")
                    width: Math.max(tabBar.buttonWidth, tabBar.width / tabBar.buttons)
                }
            }
        }
    }

    Timer {
        id: aliveTimer
        interval: 200
        running: true
        repeat: true

        onTriggered: {
            if (VescIf.isPortConnected() && dialog.visible) {
                mCommands.sendAlive()
            }
        }
    }
}
