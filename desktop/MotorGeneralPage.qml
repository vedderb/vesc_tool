/*
    Desktop MotorGeneralPage — native implementation matching the original widget page.
    Sub-tabs: General, Sensors, Current, Voltage, RPM, Wattage, Temperature, BMS, Advanced
    Each tab shows parameter editors for the corresponding subgroup of "General".
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    id: motorGeneralPage

    property ConfigParams mMcConf: VescIf.mcConfig()
    property var _dynamicItems: []

    ParamEditors {
        id: editors
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 0

        TabBar {
            id: motorTabBar
            Layout.fillWidth: true
            TabButton { text: "General" }
            TabButton { text: "Sensors" }
            TabButton { text: "Current" }
            TabButton { text: "Voltage" }
            TabButton { text: "RPM" }
            TabButton { text: "Wattage" }
            TabButton { text: "Temperature" }
            TabButton { text: "BMS" }
            TabButton { text: "Advanced" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: motorTabBar.currentIndex

            Repeater {
                model: ["general", "sensors", "current", "voltage", "rpm", "wattage", "temperature", "bms", "advanced"]
                delegate: Item {
                    Flickable {
                        anchors.fill: parent
                        anchors.margins: 4
                        clip: true
                        contentWidth: width
                        contentHeight: paramCol.height + 16
                        flickableDirection: Flickable.VerticalFlick
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        ColumnLayout {
                            id: paramCol
                            width: parent.width
                            spacing: 4
                            Item { Layout.preferredHeight: 1 }
                        }

                        Component.onCompleted: {
                            loadSubgroup(paramCol, modelData)
                        }
                    }
                }
            }
        }
    }

    function loadSubgroup(parentCol, subgroup) {
        var params = mMcConf.getParamsFromSubgroup("General", subgroup)
        for (var p = 0; p < params.length; p++) {
            var paramName = params[p]
            if (paramName.indexOf("::sep::") === 0) {
                var sep = editors.createSeparator(parentCol, paramName.substring(7))
                if (sep) _dynamicItems.push(sep)
                continue
            }
            var e = editors.createEditorMc(parentCol, paramName)
            if (e) {
                e.Layout.fillWidth = true
                _dynamicItems.push(e)
            }
        }
    }

    Connections {
        target: mMcConf
        function onUpdated() {
            // Destroy and reload
            for (var d = 0; d < _dynamicItems.length; d++) {
                if (_dynamicItems[d]) _dynamicItems[d].destroy()
            }
            _dynamicItems = []
            // Trigger reload by recreating the StackLayout content
            // This is handled by Repeater's Component.onCompleted
        }
    }
}
