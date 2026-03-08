/*
    Desktop parameter editor factory.
    Creates compact, single-row editors identical to the original widget version.
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mAppConf: VescIf.appConfig()

    function createEditor(parent, name, conf) {
        if (conf === null) {
            console.log("Config is null, cannot create editor")
            return null
        }

        if (!conf.hasParam(name)) {
            console.log("Parameter " + name + " not found.")
            return null
        }

        if (conf.isParamDouble(name)) {
            var c = Qt.createComponent("ParamEditDouble.qml")
            return c.createObject(parent, {"params": conf, "paramName": name})
        } else if (conf.isParamInt(name)) {
            var c2 = Qt.createComponent("ParamEditInt.qml")
            return c2.createObject(parent, {"params": conf, "paramName": name})
        } else if (conf.isParamEnum(name)) {
            var c3 = Qt.createComponent("ParamEditEnum.qml")
            return c3.createObject(parent, {"params": conf, "paramName": name})
        } else if (conf.isParamBool(name)) {
            var c4 = Qt.createComponent("ParamEditBool.qml")
            return c4.createObject(parent, {"params": conf, "paramName": name})
        } else if (conf.isParamQString(name)) {
            var c5 = Qt.createComponent("ParamEditString.qml")
            return c5.createObject(parent, {"params": conf, "paramName": name})
        } else if (conf.isParamBitfield(name)) {
            var c6 = Qt.createComponent("ParamEditBitfield.qml")
            return c6.createObject(parent, {"params": conf, "paramName": name})
        }

        return null
    }

    function createEditorMc(parent, name) {
        return createEditor(parent, name, mMcConf)
    }

    function createEditorApp(parent, name) {
        return createEditor(parent, name, mAppConf)
    }

    function createEditorCustom(parent, name, customInd) {
        return createEditor(parent, name, VescIf.customConfig(customInd))
    }

    function createSeparator(parent, text) {
        var c = Qt.createComponent("ParamEditSeparator.qml")
        return c.createObject(parent, {"sepName": text})
    }

    function createSpacer(parent) {
        return Qt.createQmlObject(
            'import QtQuick; import QtQuick.Layouts; Rectangle {Layout.fillHeight: true}',
            parent, "spacer1")
    }
}
