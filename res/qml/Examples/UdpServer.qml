import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc.udpserversimple 1.0

Item {
    anchors.fill: parent
    anchors.margins: 10
    
    Component.onCompleted: {
        udp.startServerBroadcast(65102)
        console.log("Started")
    }
    
    UdpServerSimple {
        id: udp
        
        onDataRx: {
            console.log(data)
        }
    }
}
