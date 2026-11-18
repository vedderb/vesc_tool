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
import QtQuick.Layouts
import Vedder.vesc.utility 1.0

// Title, step chips and progress for the setup wizards
ColumnLayout {
    id: wizardHeader
    property string title: ""
    property var steps: []
    property int currentIndex: 0
    // Optional text per step, finished steps then show it and can be clicked
    property var summaries: []

    signal stepClicked(int index)

    spacing: 10

    RowLayout {
        Layout.topMargin: 14
        Layout.leftMargin: 14
        Layout.rightMargin: 14

        Label {
            Layout.fillWidth: true
            text: wizardHeader.title
            font.pointSize: 15
            font.weight: Font.Medium
            color: Utility.getAppHexColor("titleText")
        }

        Text {
            text: qsTr("Step %1 of %2").arg(wizardHeader.currentIndex + 1).arg(wizardHeader.steps.length)
            font.pointSize: 10
            color: Utility.getAppHexColor("normalText")
        }
    }

    ListView {
        Layout.fillWidth: true
        Layout.leftMargin: 14
        Layout.rightMargin: 14
        Layout.preferredHeight: 32
        orientation: ListView.Horizontal
        spacing: 6
        clip: true
        model: wizardHeader.steps
        currentIndex: wizardHeader.summaries.length > 0 ? wizardHeader.currentIndex : -1

        delegate: Chip {
            property bool done: index < wizardHeader.currentIndex && wizardHeader.summaries.length > 0

            enabled: done
            tinted: done
            topInset: 0
            checked: index === wizardHeader.currentIndex
            text: (index < wizardHeader.currentIndex ? "✓ " : "") + modelData +
                  (done && wizardHeader.summaries[index] ? ": " + wizardHeader.summaries[index] : "")

            onClicked: wizardHeader.stepClicked(index)
        }
    }

    ProgressBar {
        Layout.fillWidth: true
        Layout.leftMargin: 14
        Layout.rightMargin: 14
        value: (wizardHeader.currentIndex + 1) / wizardHeader.steps.length
    }
}
