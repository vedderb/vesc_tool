import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
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
