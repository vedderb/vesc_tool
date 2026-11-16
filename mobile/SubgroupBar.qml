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
import QtQuick.Layouts

// Horizontal chips for the subgroups of a parameter group. A dot marks
// subgroups with unwritten changes.
ListView {
    id: bar
    property ChangeTracker tracker: null
    property string group: ""
    readonly property var changedParams: tracker ? tracker.changedParams : []
    readonly property string currentText: currentIndex >= 0 && currentIndex < count ? model[currentIndex] : ""

    implicitHeight: 44
    // Keeps the pills in place, the chips reach 6 above and below them
    Layout.topMargin: -6
    Layout.bottomMargin: -6
    orientation: ListView.Horizontal
    spacing: 6
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    highlightMoveDuration: 200

    function hasChanges(subgroup) {
        var names = tracker.params.getParamsFromSubgroup(group, subgroup)
        return changedParams.some(function(p) { return names.indexOf(p) >= 0 })
    }

    onModelChanged: currentIndex = 0

    delegate: Chip {
        text: modelData
        checked: index === bar.currentIndex
        dot: bar.changedParams.length > 0 && bar.hasChanges(modelData)
        onClicked: bar.currentIndex = index
    }
}
