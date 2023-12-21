import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    anchors.fill: parent
    anchors.margins: 10
    
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mAppConf: VescIf.appConfig()
    property ConfigParams mBalConf: VescIf.customConfig(0)
    property var outPath: "/home/benjamin/test/params.xml"
    
    ConfigParams {
        id: myCfg
    }
    
    function loadConfig() {
        myCfg.clearAll()
        
        for (var p of mMcConf.getParamOrder()) myCfg.addParam("mot_" + p, mMcConf.getParamCopy(p))
        for (var p of mAppConf.getParamOrder()) myCfg.addParam("app_" + p, mAppConf.getParamCopy(p))
        for (var p of mBalConf.getParamOrder()) myCfg.addParam("bal_" + p, mBalConf.getParamCopy(p))
        
        myCfg.loadXml(outPath, "balcfg")
        
        for (var p of mMcConf.getParamOrder()) mMcConf.updateParamFromOther(p, myCfg.getParamCopy("mot_" + p), null)
        for (var p of mAppConf.getParamOrder()) mAppConf.updateParamFromOther(p, myCfg.getParamCopy("app_" + p), null)
        for (var p of mBalConf.getParamOrder()) mBalConf.updateParamFromOther(p, myCfg.getParamCopy("bal_" + p), null)
    }
    
    function addParamToMyCfg(cfg, p) {
        if (cfg === mMcConf) myCfg.addParam("mot_" + p, cfg.getParamCopy(p))
        if (cfg === mAppConf) myCfg.addParam("app_" + p, cfg.getParamCopy(p))
        if (cfg === mBalConf) myCfg.addParam("bal_" + p, cfg.getParamCopy(p))
    }
    
    function saveConfig() {
        myCfg.clearAll()
        
        addParamToMyCfg(mMcConf, "l_current_max")
        addParamToMyCfg(mAppConf, "controller_id")
        addParamToMyCfg(mBalConf, "kp")
                
        myCfg.saveXml(outPath, "balcfg")
    }
    
    Component.onCompleted: {
        saveConfig()
        loadConfig()
        console.log("Done!")
    }
}
