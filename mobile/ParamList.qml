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
import QtQuick.Window 2.0

Item {
    implicitHeight: scrollCol.implicitHeight
    property var editorsVisible: []
    property var paramNames: []
    property var paramNamesAll: []
    property bool isHorizontal: Screen.width > Screen.height & parent.width > Screen.width/2

    onIsHorizontalChanged: {
        updateEditors()
    }

    function updateEditors() {
        for (var i = 0;i < paramNamesAll.length;i++) {
            if (paramNamesAll[i].startsWith("::sep::")) {
                editorsVisible[i].Layout.columnSpan = isHorizontal ? 2 : 1
            }
        }
    }

    function clear() {
        for (var i = 0;i < editorsVisible.length;i++) {
            editorsVisible[i].destroy();
        }
        editorsVisible = []
        paramNames = []
        paramNamesAll = []
    }

    function addEditorMc(param) {
        var e = editors.createEditorMc(scrollCol, param)

        if (e === null) {
            return
        }

        e.Layout.preferredWidth = 500
        e.Layout.fillsWidth = true
        editorsVisible.push(e)
        paramNames.push(param)
        paramNamesAll.push(param)
    }

    function addEditorApp(param) {
        var e = editors.createEditorApp(scrollCol, param)

        if (e === null) {
            return
        }

        e.Layout.preferredWidth = 500
        e.Layout.fillsWidth = true
        editorsVisible.push(e)
        paramNames.push(param)
        paramNamesAll.push(param)
    }

    function addEditorCustom(param, customInd) {
        var e = editors.createEditorCustom(scrollCol, param, customInd)

        if (e === null) {
            return
        }

        e.Layout.preferredWidth = 500
        e.Layout.fillsWidth = true
        editorsVisible.push(e)
        paramNames.push(param)
        paramNamesAll.push(param)
    }

    function addSpacer() {
        var e = editors.createSpacer(scrollCol)
        editorsVisible.push(e)
    }

    function addSeparator(text) {
        var e = editors.createSeparator(scrollCol, text)
        e.Layout.columnSpan = isHorizontal ? 2 : 1
        editorsVisible.push(e)
        paramNamesAll.push("::sep::" + text)
    }

    function getParamNames() {
        return paramNames
    }

    ParamEditors {
        id: editors
    }

    GridLayout {
        id: scrollCol
        columns: isHorizontal ? 2 : 1
        anchors.fill: parent
    }
}
