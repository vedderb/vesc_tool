/*
    Copyright 2017 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

import QtQuick 2.4
import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.bleuart 1.0
import Vedder.vesc.commands 1.0

ConnectBleForm {
    property BleUart mBle: VescIf.bleDevice()
    property Commands mCommands: VescIf.commands()

    scanButton.onClicked: {
        mBle.startScan()
        scanButton.enabled = false
        scanButton.text = "Scanning"
    }

    connectButton.onClicked: {
        if (bleItems.rowCount() > 0) {
            connectButton.enabled = false
            VescIf.connectBle(bleItems.get(bleBox.currentIndex).value)
        }
    }

    disconnectButton.onClicked: {
        VescIf.disconnectPort()
    }

    fwdCanBox.onClicked: {
        mCommands.setSendCan(fwdCanBox.checked, canIdBox.value)
    }

    canIdBox.onValueChanged: {
        mCommands.setCanSendId(canIdBox.value)
    }

    Timer {
        interval: 500
        running: !scanButton.enabled
        repeat: true

        property int dots: 0
        onTriggered: {
            var text = "Scanning"
            for (var i = 0;i < dots;i++) {
                text = "-" + text + "-"
            }

            dots++;
            if (dots > 3) {
                dots = 0;
            }

            scanButton.text = text
        }
    }

    Timer {
        interval: 100
        running: true
        repeat: true

        onTriggered: {
            connectButton.enabled = (bleItems.rowCount() > 0) && !VescIf.isPortConnected() && !mBle.isConnecting()
            disconnectButton.enabled = VescIf.isPortConnected()
        }
    }

    Connections {
        target: mBle
        onScanDone: {
            if (done) {
                scanButton.enabled = true
                scanButton.text = qsTr("Scan")
            }

            bleItems.clear()

            for (var name in devs) {
                var name2 = name + " [" + devs[name] + "]"
                if (name.indexOf("VESC") !== -1) {
                    bleItems.insert(0, { key: name2, value: devs[name] })
                } else {
                    bleItems.append({ key: name2, value: devs[name] })
                }
            }

            connectButton.enabled = (bleItems.rowCount() > 0) && !VescIf.isPortConnected()

            bleBox.currentIndex = 0
        }
    }
}
