/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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

Item {
    implicitHeight: scrollCol.implicitHeight
    property var editorsVisible: []

    function clear() {
        for (var i = 0;i < editorsVisible.length;i++) {
            editorsVisible[i].destroy();
        }
        editorsVisible = []
    }

    function addEditorMc(param) {
        editorsVisible.push(editors.createEditorMc(scrollCol, param))
    }

    function addEditorApp(param) {
        editorsVisible.push(editors.createEditorApp(scrollCol, param))
    }

    function addSpacer() {
        editorsVisible.push(editors.createSpacer(scrollCol))
    }

    function addSeparator(text) {
        editorsVisible.push(editors.createSeparator(scrollCol, text))
    }

    ParamEditors {
        id: editors
    }

    ColumnLayout {
        id: scrollCol
        anchors.fill: parent
    }
}
