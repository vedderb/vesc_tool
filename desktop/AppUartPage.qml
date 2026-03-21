import QtQuick
import QtQuick.Controls
import Vedder.vesc

ParamGroupPage {
    configParams: VescIf.appConfig()
    groupName: "UART"
}
