/*
    Desktop AppGeneralPage — exact parity with PageAppGeneral widget.
    Layout: TabWidget (General | Tools)
      General: ParamTable of "general" / "general"
      Tools: Servo Output GroupBox with slider + Center button, then spacer
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    id: root

    property ConfigParams mAppConf: VescIf.appConfig()
    property Commands mCommands: VescIf.commands()
    property var _dynamicItems: []

    ParamEditors { id: editors }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            TabButton { text: "General" }
            TabButton { text: "Tools" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Tab 0: General params
            Flickable {
                clip: true
                contentWidth: width
                contentHeight: genCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: genCol
                    width: parent.width
                    spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 1: Tools (Servo Output)
            ColumnLayout {
                spacing: 0

                GroupBox {
                    title: "Servo Output"
                    Layout.fillWidth: true
                    Layout.margins: 0

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Slider {
                            id: servoSlider
                            Layout.fillWidth: true
                            from: 0; to: 1000; value: 500; stepSize: 1
                            snapMode: Slider.SnapOnRelease
                            onValueChanged: mCommands.setServoPos(value / 1000.0)
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Item { Layout.fillWidth: true }
                            Button {
                                text: "Center"
                                onClicked: servoSlider.value = 500
                            }
                            Item { Layout.fillWidth: true }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    function loadParams() {
        for (var d = 0; d < _dynamicItems.length; d++) {
            if (_dynamicItems[d]) _dynamicItems[d].destroy()
        }
        _dynamicItems = []

        var params = mAppConf.getParamsFromSubgroup("General", "general")
        for (var p = 0; p < params.length; p++) {
            var paramName = params[p]
            if (paramName.indexOf("::sep::") === 0) {
                var sep = editors.createSeparator(genCol, paramName.substring(7))
                if (sep) _dynamicItems.push(sep)
                continue
            }
            var e = editors.createEditorApp(genCol, paramName)
            if (e) { e.Layout.fillWidth = true; _dynamicItems.push(e) }
        }
    }

    Component.onCompleted: loadParams()
}
