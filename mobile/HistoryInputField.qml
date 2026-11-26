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

// InputField that remembers sent commands, newest first. Up steps to older commands and
// wraps around, down steps back to newer ones and then to the text that was being typed.
InputField {
    property var history: []
    property int historyIndex: -1
    property string draft: ""

    function add(cmd) {
        history = [cmd].concat(history.filter(h => h !== cmd))
        historyIndex = -1
    }

    function older() {
        if (history.length === 0) {
            return
        }

        if (historyIndex < 0) {
            draft = text
        }

        historyIndex = (historyIndex + 1) % history.length
        text = history[historyIndex]
    }

    function newer() {
        if (historyIndex < 0) {
            return
        }

        historyIndex--
        text = historyIndex < 0 ? draft : history[historyIndex]
    }

    Keys.onUpPressed: older()
    Keys.onDownPressed: newer()
}
