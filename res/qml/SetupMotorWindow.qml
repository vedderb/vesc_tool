/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import QtQuick.Window 2.2

import Vedder.vesc.utility 1.0
import "qrc:/mobile"

ApplicationWindow {
    id: mainWindow
    visible: true
    visibility: Window.Windowed
    width: 500
    height: 800
    title: qsTr("Motor Setup")

    Material.theme: Utility.isDarkMode() ? "Dark" : "Light"
    Material.accent: Utility.getAppHexColor("lightAccent")

    Component.onCompleted: {
        x = Screen.width / 2 - width / 2
        y = Screen.height / 2 - height / 2

        if (!VescIf.isPortConnected()) {
            close()
            VescIf.emitMessageDialog("Setup motors FOC",
                                     "Not connected. Please connect in order to run the FOC wizard.",
                                     false, false)
        }

        focWizard.openDialog()
    }

    SetupWizardFoc {
        id: focWizard
        onDialogClosed: close()
    }
}
