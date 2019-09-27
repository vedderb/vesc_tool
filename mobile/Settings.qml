/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.utility 1.0

Item {
    function openDialog() {
        dialog.open()
    }

    Dialog {
        id: dialog
        standardButtons: Dialog.Close
        modal: true
        focus: true
        title: "VESC Tool Settings"

        width: parent.width - 20
        closePolicy: Popup.CloseOnEscape

        x: 10
        y: Math.max((parent.height - height) / 2, 10)
        parent: ApplicationWindow.overlay

        ColumnLayout {
            anchors.fill: parent

            CheckBox {
                id: imperialBox
                Layout.fillWidth: true
                text: "Use Imperial Units"
                checked: VescIf.useImperialUnits()
            }

            CheckBox {
                id: screenOnBox
                Layout.fillWidth: true
                text: "Keep Screen On"
                checked: VescIf.keepScreenOn()
            }

            CheckBox {
                id: wakeLockBox
                Layout.fillWidth: true
                text: "Use Wake Lock (experimental)"
                checked: VescIf.useWakeLock()
            }

            Item {
                // Spacer
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        onClosed: {
            VescIf.setUseImperialUnits(imperialBox.checked)
            VescIf.setKeepScreenOn(screenOnBox.checked)
            VescIf.setUseWakeLock(wakeLockBox.checked)
            VescIf.storeSettings()

            Utility.keepScreenOn(VescIf.keepScreenOn())

            if (VescIf.useWakeLock()) {
                VescIf.setWakeLock(VescIf.isPortConnected())
            }
        }
    }
}
