import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.12

import Vedder.vesc.logwriter 1.0

Item {
    id: mainItem
    anchors.fill: parent
    anchors.margins: 5
    
    property var cnt: 0
    
    LogWriter {
        id: mLog
    }
    
    Text {
        anchors.fill: parent
        id: txt
        text: "Go to Settings > Paths to change the destination directory."
        color: "white"
    }
    
    Component.onCompleted: {
        mLog.openLogFile("TestLogFile.csv")
    }
    
    Component.onDestruction: {
        // Close file when done to ensure that all data is written.
        mLog.closeLogFile()
    }
    
    Timer {
        running: true
        repeat: true
        interval: 100
        
        onTriggered: {
            cnt += 0.1
            mLog.writeToLogFile("Counter, " + parseFloat(cnt).toFixed(2) + "\n")
        }
    }
}
