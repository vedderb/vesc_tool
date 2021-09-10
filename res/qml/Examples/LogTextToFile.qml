import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0

import Vedder.vesc.logwriter 1.0
import Vedder.vesc.logreader 1.0

Item {
    id: mainItem
    anchors.fill: parent
    anchors.margins: 5

    Material.theme: Utility.isDarkMode() ? "Dark" : "Light"
    Material.accent: Utility.getAppHexColor("lightAccent")
    
    property var cnt: 0
    
    LogWriter {
        id: mLogWriter
    }
    
    LogReader {
        id: mLogReader
    }
    
    ColumnLayout {
        anchors.fill: parent
           
        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: "Go to Settings > Paths to change the source and destination directories."
            color: "white"
            wrapMode: Text.WordWrap
        }
        
        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            id: txt
            text: ""
            color: "white"
        }
        
        // This button is an example of reading the log back and parsing it.
        Button {
            Layout.fillWidth: true
            text: "Read Log"
            
            onClicked: {                
                mLogReader.openLogFile("TestLogFile.csv")

                var text = ""
                for (var i = 0;i < 5;i++) {
                    var line = mLogReader.readLine()
                    text += line
                    
                    // Split line into tokens and parse them
                    var tokens = line.split(",")
                    console.log("First token: " + tokens[0])
                    console.log("Sine of second token: " + Math.sin(parseFloat(tokens[1])))
                    
                    // atEnd() can be used to check if the end of file is reached when reading lines
                    if (mLogReader.atEnd()) {
                        console.log("File ended")
                        break
                    }
                }                
                txt.text = text
                
                // readAll() can also be used to get all lines at once
                // txt.text = mLogReader.readAll()

                mLogReader.closeLogFile()
            }
        }
    }
    
    Component.onCompleted: {
        mLogWriter.openLogFile("TestLogFile.csv")
    }
    
    Component.onDestruction: {
        // Close file when done to ensure that all data is written.
        mLogWriter.closeLogFile()
    }
    
    Timer {
        running: true
        repeat: true
        interval: 500
        
        onTriggered: {
            cnt += 0.5
            mLogWriter.writeToLogFile("Counter," + parseFloat(cnt).toFixed(2) + "\n")
        }
    }
}
