import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0

import Vedder.vesc.commands 1.0

Item {
    id: mainItem
    anchors.fill: parent
    anchors.margins: 5

    Material.theme: Utility.isDarkMode() ? "Dark" : "Light"
    Material.accent: Utility.getAppHexColor("lightAccent")
    
    property Commands mCommands: VescIf.commands()
    property var plotx: 0
    
    Text {
        anchors.fill: parent
        id: "txt"
        text: "Go to Realtime Data > Experiment plot to see the output."
        color: "white"
    }
    
    Component.onCompleted: {
        mCommands.emitPlotInit("x axis name", "y axis name")
        mCommands.emitPlotAddGraph("First graph")
        mCommands.emitPlotAddGraph("Second graph")
    }
    
    Timer {
        running: true
        repeat: true
        interval: 100
        
        onTriggered: {
            plotx += 0.1
            mCommands.emitPlotSetGraph(0)
            mCommands.emitPlotData(plotx, Math.sin(plotx))
            mCommands.emitPlotSetGraph(1)
            mCommands.emitPlotData(plotx, Math.cos(plotx) * 0.7)
        }
    }
}
