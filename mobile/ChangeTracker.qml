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
import Vedder.vesc.configparams 1.0

// Tracks the parameters that differ from the last read or write
QtObject {
    id: tracker
    property ConfigParams params: null
    property bool trackChanges: true
    property var changedParams: []
    signal paramChanged(string name)

    function update() {
        changedParams = params && trackChanges ? params.getChangedParams() : []
    }

    function changed(name) {
        paramChanged(name)
        if (trackChanges) {
            Qt.callLater(update)
        }
    }

    // Name and reference value of every changed parameter, used to revert them
    function referenceValues() {
        return changedParams.map(function(p) { return [p, params.getReference().getParamCopy(p)] })
    }

    onParamsChanged: update()

    property Connections connections: Connections {
        target: tracker.params

        function onParamChangedDouble(src, name, newParam) { tracker.changed(name) }
        function onParamChangedInt(src, name, newParam) { tracker.changed(name) }
        function onParamChangedEnum(src, name, newParam) { tracker.changed(name) }
        function onParamChangedQString(src, name, newParam) { tracker.changed(name) }
        function onParamChangedBool(src, name, newParam) { tracker.changed(name) }
        function onReferenceStored() {
            if (tracker.trackChanges) {
                Qt.callLater(tracker.update)
            }
        }
    }
}
