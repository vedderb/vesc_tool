import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.bleuart 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

import SkyPuff.vesc.winch 1.0

Page {
    id: page
    property BleUart mBle: VescIf.bleDevice()
    property Commands mCommands: VescIf.commands()
    property ConfigParams mInfoConf: VescIf.infoConfig()
    //property alias disconnectButton: disconnectButton

    function setConnectButtonsEnabled(e)
    {
        usbConnectButton.enabled = e
        bleConnectButton.enabled = e && (bleItems.rowCount() > 0)
    }

    function updateConnected()
    {
        if(VescIf.isPortConnected()) {
            setConnectButtonsEnabled(false)
            disconnectButton.enabled = true
        }
        else {
            setConnectButtonsEnabled(true)
            disconnectButton.enabled = false
            statusMessage.text = qsTr("Not connected")
            statusMessage.color = "red"
        }
    }

    Dialog {
        id: vescDialog
        standardButtons: Dialog.Ok
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape

        width: parent.width - 20
        height: Math.min(implicitHeight, parent.height - 40)
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        parent: ApplicationWindow.overlay

        ScrollView {
            anchors.fill: parent
            clip: true
            contentWidth: parent.width - 20

            Text {
                id: vescDialogLabel
                linkColor: "lightblue"
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
                wrapMode: Text.WordWrap
                textFormat: Text.RichText
                onLinkActivated: {
                    Qt.openUrlExternally(link)
                }
            }
        }

        Connections {
            target: VescIf

            // Display modal dialog with errors from VESC interface
            onMessageDialog: {
                vescDialog.title = title
                vescDialogLabel.text = (richText ? "<style>a:link { color: lightblue; }</style>" : "") + msg
                vescDialogLabel.textFormat = richText ? Text.RichText : Text.AutoText
                vescDialog.open()
            }
        }
    }


    // Enter IP address and port dialog
    Popup {
        id: newUdpAddressDialog
        modal: true
        anchors.centerIn: parent
        contentWidth: parent.width - 50

        ColumnLayout {
            anchors.fill: parent

            Label {
                text: qsTr("Add VESC UDP address...")
                font.bold: true
            }

            TextField {
                Layout.fillWidth: true
                Layout.margins: 20

                id: udpAddress

                text: "192.168.4.1:65102"
                focus: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 5
                Button {
                    text: qsTr('Cancel')
                    onClicked: newUdpAddressDialog.close()
                }
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr('Save')
                    onClicked: {
                        newUdpAddressDialog.close()
                        var host = Skypuff.urlHost("udp://" + udpAddress.text)
                        var port = Skypuff.urlPort("udp://" + udpAddress.text)
                        if(host && port !== -1) {
                            var addr = host + ":" + port
                            if(listModel.find("udp", addr) === -1)
                                listModel.insert(0, {type: "udp", name: "UDP [%1:%2]".arg(host).arg(port), addr: addr, isVesc: true})
                        }
                    }
                }
            }
        }
    }
    /*PairingDialog {
        id: pairDialog
    }*/

    ColumnLayout {
        anchors.fill: parent

        Label {
            text: qsTr("Find Skypuff")

            Layout.fillWidth: true
            Layout.topMargin: bBluetooth.size / 3
            Layout.bottomMargin: bBluetooth.size / 4

            horizontalAlignment: Text.AlignHCenter

            font.pixelSize: bBluetooth.size / 3
            font.bold: true
        }

        // Methods buttons
        RowLayout {
            Layout.fillWidth: true

            Item {
                Layout.fillWidth: true
            }
            BigRoundButton {
                id: bBluetooth
                icon.source: "qrc:/res/icons/bluetooth.svg"
                Material.foreground: Material.Blue
                onClicked: {
                    if(!busy) {
                        listModel.clearByType('bt')
                        mBle.startScan()
                        busy = true;
                    }
                }
                Component.onCompleted: clicked()
            }
            BigRoundButton {
                id: bUsb
                //Layout.margins: parent.width / 20
                icon.source: "qrc:/res/icons/usb.svg"
                iconPercent: 40
                Material.foreground: Material.Teal

                onClicked: {
                    listModel.clearByType('usb')
                    var ports = Skypuff.serialPortsToQml();
                    for(var i = 0; i < ports.length; i++) {
                        if(ports[i].isVesc) {
                            listModel.insert(0, {type: "usb", name: ports[i].name, addr: ports[i].addr, isVesc: true})
                        } else {
                            listModel.append({type: "usb", name: ports[i].name, addr: ports[i].addr, isVesc: false})
                        }
                    }
                }
                Component.onCompleted: clicked()
            }
            BigRoundButton {
                id: bWifi
                icon.source: "qrc:/res/icons/wifi.svg"
                Material.foreground: Material.Indigo

                onClicked: newUdpAddressDialog.open()

                Component.onCompleted: {
                    var host = VescIf.getLastUdpServer();
                    host = host === "127.0.0.1" ? "192.168.4.1" : host
                    var port = VescIf.getLastUdpPort()
                    var addr = host + ":" + port
                    if(listModel.find("udp", addr) === -1)
                        listModel.insert(0, {type: "udp", name: "UDP [%1:%2]".arg(host).arg(port), addr: addr, isVesc: true})
                }
            }
            Item {
                Layout.fillWidth: true
            }

            // To prevent binding loop
            Component.onCompleted: {
                bBluetooth.size = page.width / 4
                bUsb.size = bBluetooth.size
                bWifi.size = bBluetooth.size
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 20
            ScrollBar.vertical: ScrollBar {}
            clip: true
            //highlightFollowsCurrentItem: false
            //focus: true

            delegate: Item {
                id: delegateItem
                width: listView.width
                clip: true
                /*Rectangle {
                    anchors.fill: parent
                    color: "#aaddaa"
                    visible: isVesc
                }*/

                opacity: isVesc ? 1 : 0.4

                Image {
                    id: connectionType
                    anchors {
                        left: parent.left
                        leftMargin: 5
                        verticalCenter: parent.verticalCenter
                    }

                    smooth: true

                    source: type === "bt" ? "qrc:/res/icons/bluetooth.svg" :
                            type === "usb" ? "qrc:/res/icons/usb.svg" :
                            "qrc:/res/icons/wifi.svg"

                    sourceSize.width: bUsb.icon.width
                    sourceSize.height: bBluetooth.icon.height

                    visible: false

                }
                ColorOverlay {
                    anchors.fill: connectionType
                    source: connectionType
                    color: type === "bt" ? Material.color(Material.Blue) :
                           type === "usb" ? Material.color(Material.Teal) :
                                            Material.color(Material.Indigo)

                    RotationAnimator on rotation {
                        id: connectionAnime
                        from: 0;
                        to: 360;
                        duration: 1100
                        running: false
                        easing.type: Easing.InQuad
                    }
                }

                Text {
                    anchors {
                        left: connectionType.right
                        leftMargin: 10
                        verticalCenter: parent.verticalCenter;
                    }
                    id: tName
                    text: name
                    font.pixelSize: 15
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        connectionAnime.running = true

                        switch(type) {
                        case 'bt':
                            VescIf.connectBle(addr)
                            break
                        case 'usb':
                            Skypuff.connectSerial(addr)
                            break
                        case 'udp':
                            var host = Skypuff.urlHost("udp://" + addr)
                            var port = Skypuff.urlPort("udp://" + addr)
                            VescIf.connectUdp(host, port)
                            break
                        default:
                            console.log('Connection type', type, 'not implemented')
                        }
                    }
                }

                ListView.onAdd: SequentialAnimation {
                    PropertyAction { target: delegateItem; property: "height"; value: 0 }
                    NumberAnimation { target: delegateItem; property: "height"; to: tName.contentHeight + 30; duration: 700; easing.type: Easing.InOutQuad }
                }
            }

            model: ListModel {
                id: listModel

                function find(type, addr) {
                    for(var i = 0; i < count; i++) {
                        var e = get(i);
                        if(e.addr === addr && e.type === type)
                            return i
                    }
                    return -1
                }

                function clearByType(t) {
                    for(var i = 0; i < count; i++) {
                        var e = get(i);
                        if(e.type === t) {
                            remove(i)
                            i--
                        }
                    }
                }
            }

        }
    }

    /*footer: Label {
        verticalAlignment: Text.AlignVCenter;
        x: 5;
        height: contentHeight + 8
        text: qsTr("Not connected")
    }*/

    // Bluetooth
    Connections {
        target: mBle


        onScanDone: {
            if (done) {
                bBluetooth.busy = false
            }

            for (var addr in devs) {
                if(listModel.find("bt", addr) !== -1) {
                    // TODO: replace
                    continue
                }

                var name = devs[addr]
                var name2 = name + " [" + addr + "]"
                var setName = VescIf.getBleName(addr)
                if (setName.length > 0) {
                    setName += " [" + addr + "]"
                    listModel.insert(0, {type: "bt", name: setName, addr: addr, isVesc: true})
                } else if (name.indexOf("VESC") !== -1) {
                    listModel.insert(0, {type: "bt", name: name2, addr: addr, isVesc: true})
                } else {
                    listModel.append({type: "bt", name: name2, addr: addr, isVesc: false})
                }
            }
        }

        onBleError: {
            VescIf.emitMessageDialog("BLE Error", info, false, false)
        }
    }

}
