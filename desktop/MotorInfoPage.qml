/*
    Desktop MotorInfoPage — faithful recreation of the original PageMotorInfo widget.
    4 sub-tabs: Setup, Motor General, Description, Quality
    Setup + Motor General show param editors.
    Description + Quality show rich-text editors for motor description/quality strings.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    id: motorInfoPage

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
            id: infoTabBar
            Layout.fillWidth: true
            TabButton { text: "Setup" }
            TabButton { text: "Motor General" }
            TabButton { text: "Description" }
            TabButton { text: "Quality" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: infoTabBar.currentIndex

            // Tab 0: Setup
            Flickable {
                clip: true; contentWidth: width; contentHeight: setupCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: setupCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 1: Motor General
            Flickable {
                clip: true; contentWidth: width; contentHeight: generalCol.height + 16
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ColumnLayout {
                    id: generalCol; width: parent.width; spacing: 4
                    Item { Layout.preferredHeight: 1 }
                }
            }

            // Tab 2: Description
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    Label {
                        text: "Motor Description"
                        font.bold: true
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        TextArea {
                            id: descriptionEdit
                            textFormat: TextEdit.RichText
                            wrapMode: TextEdit.Wrap
                            placeholderText: "Enter motor description here..."
                            text: mMcConf ? mMcConf.getParamQString("motor_description") : ""

                            onTextChanged: {
                                if (mMcConf) {
                                    mMcConf.updateParamString("motor_description", text)
                                }
                            }
                        }
                    }
                }
            }

            // Tab 3: Quality
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    Flickable {
                        Layout.fillWidth: true
                        Layout.preferredHeight: qualityCol.height + 16
                        Layout.maximumHeight: parent.height * 0.4
                        clip: true; contentWidth: width; contentHeight: qualityCol.height + 16
                        flickableDirection: Flickable.VerticalFlick
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                        ColumnLayout {
                            id: qualityCol; width: parent.width; spacing: 4
                            Item { Layout.preferredHeight: 1 }
                        }
                    }

                    Label {
                        text: "Quality Description"
                        font.bold: true
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        TextArea {
                            id: qualityEdit
                            textFormat: TextEdit.RichText
                            wrapMode: TextEdit.Wrap
                            placeholderText: "Enter quality description here..."
                            text: mMcConf ? mMcConf.getParamQString("motor_quality_description") : ""

                            onTextChanged: {
                                if (mMcConf) {
                                    mMcConf.updateParamString("motor_quality_description", text)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    function loadSubgroup(parentCol, subgroup) {
        var params = mMcConf.getParamsFromSubgroup("Additional Info", subgroup)
        for (var p = 0; p < params.length; p++) {
            var paramName = params[p]
            if (paramName.indexOf("::sep::") === 0) {
                var sep = editors.createSeparator(parentCol, paramName.substring(7))
                if (sep) _dynamicItems.push(sep)
                continue
            }
            var e = editors.createEditorMc(parentCol, paramName)
            if (e) { e.Layout.fillWidth = true; _dynamicItems.push(e) }
        }
    }

    function reloadAll() {
        for (var d = 0; d < _dynamicItems.length; d++) {
            if (_dynamicItems[d]) _dynamicItems[d].destroy()
        }
        _dynamicItems = []

        loadSubgroup(setupCol, "setup")
        loadSubgroup(generalCol, "general")
        loadSubgroup(qualityCol, "quality")

        if (mMcConf) {
            descriptionEdit.text = mMcConf.getParamQString("motor_description")
            qualityEdit.text = mMcConf.getParamQString("motor_quality_description")
        }
    }

    Component.onCompleted: reloadAll()

    Connections {
        target: mMcConf
        function onUpdated() { reloadAll() }
    }
}
