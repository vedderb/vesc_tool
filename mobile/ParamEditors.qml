/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

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

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mAppConf: VescIf.appConfig()

    function createEditor(parent, name, conf) {
        if (conf.hasParam(name)) {
            if (conf.isParamDouble(name)) {
                var component = Qt.createComponent("ParamEditDouble.qml");
                return component.createObject(parent, {"params": conf, "paramName": name});
            } else if (conf.isParamInt(name)) {
                var component2 = Qt.createComponent("ParamEditInt.qml");
                return component2.createObject(parent, {"params": conf, "paramName": name});
            } else if (conf.isParamEnum(name)) {
                var component3 = Qt.createComponent("ParamEditEnum.qml");
                return component3.createObject(parent, {"params": conf, "paramName": name});
            } else if (conf.isParamBool(name)) {
                var component4 = Qt.createComponent("ParamEditBool.qml");
                return component4.createObject(parent, {"params": conf, "paramName": name});
            } else if (conf.isParamQString(name)) {
                var component5 = Qt.createComponent("ParamEditString.qml");
                return component5.createObject(parent, {"params": conf, "paramName": name});
            }
        } else {
            console.log("Parameter " + name + " not found.")
        }

        return null
    }

    function createEditorMc(parent, name) {
        return createEditor(parent, name, mMcConf)
    }

    function createEditorApp(parent, name) {
        return createEditor(parent, name, mAppConf)
    }

    function createSeparator(parent, text) {
        var component = Qt.createComponent("ParamEditSeparator.qml");
        return component.createObject(parent, {"sepName": text});
    }

    function createSpacer(parent) {
        return Qt.createQmlObject(
                    'import QtQuick 2.7; import QtQuick.Layouts 1.3; Rectangle {Layout.fillHeight: true}',
                    parent,
                    "spacer1")
    }
}
