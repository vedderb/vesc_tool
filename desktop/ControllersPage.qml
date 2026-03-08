/*
    Desktop ControllersPage — faithful recreation of the original PageControllers widget.
    ParamTable for "pid controllers" / "general" subgroup
    + Position Offset Calculator group at the bottom.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    id: controllersPage

    property ConfigParams mMcConf: VescIf.mcConfig()
    property Commands mCommands: VescIf.commands()
    property var _dynamicItems: []

    ParamEditors {
        id: editors
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
        }

        // Position Offset Calculator — matches original
        GroupBox {
            title: "Position Offset Calculator"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                spacing: 8

                SpinBox {
                    id: posOffsetBox
                    from: -360000
                    to: 360000
                    value: 0
                    stepSize: 100
                    editable: true
                    property real realValue: value / 1000.0
                    textFromValue: function(v, locale) {
                        return "Pos Now: " + (v / 1000).toFixed(3) + " °"
                    }
                    valueFromText: function(text, locale) {
                        var s = text.replace("Pos Now: ", "").replace(" °", "")
                        return Math.round(parseFloat(s) * 1000)
                    }
                    Layout.fillWidth: true
                }

                Button {
                    text: "Apply"
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Download-96.png"
                    onClicked: {
                        mCommands.sendTerminalCmdSync(
                            "update_pid_pos_offset " + posOffsetBox.realValue.toFixed(3) + " 1")
                        mCommands.getMcconf()
                    }
                }
            }
        }
    }

    function reloadAll() {
        for (var d = 0; d < _dynamicItems.length; d++) {
            if (_dynamicItems[d]) _dynamicItems[d].destroy()
        }
        _dynamicItems = []

        var params = mMcConf.getParamsFromSubgroup("pid controllers", "general")
        for (var p = 0; p < params.length; p++) {
            var paramName = params[p]
            if (paramName.indexOf("::sep::") === 0) {
                var sep = editors.createSeparator(paramCol, paramName.substring(7))
                if (sep) _dynamicItems.push(sep)
                continue
            }
            var e = editors.createEditorMc(paramCol, paramName)
            if (e) { e.Layout.fillWidth = true; _dynamicItems.push(e) }
        }
    }

    Component.onCompleted: reloadAll()

    Connections {
        target: mMcConf
        function onUpdated() { reloadAll() }
    }
}
