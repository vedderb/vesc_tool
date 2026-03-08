/*
    Reusable parameter group page for the desktop UI.
    Shows all parameters from a given group of a ConfigParams instance.

    Usage:
        ParamGroupPage {
            configParams: VescIf.mcConfig()
            groupName: "FOC"
        }
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Vedder.vesc

Item {
    id: root

    property ConfigParams configParams: null
    property string groupName: ""

    // Track dynamically-created items so we can destroy them on reload
    property var _dynamicItems: []

    ParamEditors {
        id: editors
    }

    Flickable {
        id: flick
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: paramCol.height + 16
        flickableDirection: Flickable.VerticalFlick
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: paramCol
            width: flick.width
            spacing: 4

            // A placeholder item that is always present so the column has a child
            Item { Layout.preferredHeight: 1 }
        }
    }

    function reload() {
        // Destroy previously-created editors
        for (var d = 0; d < _dynamicItems.length; d++) {
            if (_dynamicItems[d]) {
                _dynamicItems[d].destroy()
            }
        }
        _dynamicItems = []

        if (configParams === null || groupName === "") {
            return
        }

        var subgroups = configParams.getParamSubgroups(groupName)

        for (var sg = 0; sg < subgroups.length; sg++) {
            // Subgroup separator
            var sep = editors.createSeparator(paramCol, subgroups[sg])
            if (sep) {
                _dynamicItems.push(sep)
            }

            var params = configParams.getParamsFromSubgroup(groupName, subgroups[sg])
            for (var p = 0; p < params.length; p++) {
                var paramName = params[p]

                // Items starting with "::sep::" are separators, not parameters
                if (paramName.indexOf("::sep::") === 0) {
                    var sepText = paramName.substring(7) // strip "::sep::"
                    var sep2 = editors.createSeparator(paramCol, sepText)
                    if (sep2) {
                        _dynamicItems.push(sep2)
                    }
                    continue
                }

                var e = null
                if (configParams === VescIf.mcConfig()) {
                    e = editors.createEditorMc(paramCol, paramName)
                } else if (configParams === VescIf.appConfig()) {
                    e = editors.createEditorApp(paramCol, paramName)
                } else {
                    e = editors.createEditorCustom(paramCol, paramName, 0)
                }

                if (e) {
                    e.Layout.fillWidth = true
                    _dynamicItems.push(e)
                }
            }
        }
    }

    onConfigParamsChanged: reload()
    onGroupNameChanged: reload()

    Connections {
        target: configParams
        function onUpdated() {
            reload()
        }
    }
}
