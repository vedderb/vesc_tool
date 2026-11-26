/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import Vedder.vesc.utility 1.0

// Background that styles ComboBox, SpinBox and TextField like the cards
Rectangle {
    // Same implicit size as the stock backgrounds
    implicitWidth: parent instanceof SpinBox ? 140 : 120
    implicitHeight: Material.textFieldHeight
    radius: 12
    color: Utility.getAppHexColor("cardBackground")
    border.color: parent.activeFocus ? Material.accent : Utility.getAppHexColor("raisedBackground")
}
