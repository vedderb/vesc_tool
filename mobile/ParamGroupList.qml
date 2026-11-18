/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Vedder.vesc.utility 1.0

// Side list for the parameter groups of a configuration page, as a second view of
// its group box. A dot marks groups with unwritten changes.
ListView {
    id: list
    property ComboBox box
    property ChangeTracker tracker: null
    readonly property var changedParams: tracker ? tracker.changedParams : []

    y: 5
    width: 180
    height: parent.height - y
    visible: mainIsTablet && parent.width >= 900
    model: visible ? box.model : null
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    function hasChanges(group) {
        return tracker.params.getParamSubgroups(group).some(function(subgroup) {
            var names = tracker.params.getParamsFromSubgroup(group, subgroup)
            return changedParams.some(function(p) { return names.indexOf(p) >= 0 })
        })
    }

    header: ItemDelegate {
        width: list.width
        text: qsTr("Search")
        icon.source: "qrc" + Utility.getThemePath() + "icons/Search-96.png"
        onClicked: paramSearch.open()
    }

    delegate: ItemDelegate {
        width: list.width
        height: 44
        highlighted: index === list.box.currentIndex
        onClicked: list.box.currentIndex = index

        background: Rectangle {
            radius: 10
            color: highlighted ? Qt.alpha(Material.accent, 0.12) : "transparent"
        }

        contentItem: RowLayout {
            Text {
                Layout.fillWidth: true
                text: modelData
                elide: Text.ElideRight
                color: Utility.getAppHexColor(highlighted ? "accentText" : "lightText")
                font.weight: highlighted ? Font.Medium : Font.Normal
            }

            Rectangle {
                width: 6
                height: 6
                radius: 3
                visible: list.changedParams.length > 0 && list.hasChanges(modelData)
                color: Material.accent
            }
        }
    }
}
