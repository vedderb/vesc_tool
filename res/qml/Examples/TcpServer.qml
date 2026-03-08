import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import Vedder.vesc

Item {
    anchors.fill: parent
    anchors.margins: 10

    property Commands mCommands: VescIf.commands()
    
    Component.onCompleted: {
        tcp.startServer(12321)
        
        // to send and arraybuffer with data, use
        //tcp.sendData(dataToSend)
    }
    
    TcpServerSimple {
        id: tcp
        
        property var str: ""
        
        onDataRx: {
            // Put bytes in a string as they come and process for every newline character
            
            var dv = new DataView(data);
                        
            for (var i = 0;i < data.byteLength;i++) {
                var ch = String.fromCharCode(dv.getUint8(i))
                
                if (ch == "\n") {
                    console.log(str)
                    
                    var tokens = str.split(" ")
                                        
                    if (tokens[0] == "set_current") {
                        var current = parseInt(tokens[1], 10)
                        if (!isNaN(current)) {
                            console.log("Setting motor current to " + current + " A")
                            mCommands.setCurrent(current)
                        }
                    }
                    
                    str = ""
                } else {
                    str += ch
                }
            }
        }
    }
}
