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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2

import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    anchors.fill: parent
    anchors.margins: 10

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property bool motorRunning: false
    property double posSet: 0
    
    Behavior on posSet {
        NumberAnimation {
            easing.type: Easing.Linear
            duration: 200
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        Slider {
            id: posSlider
            Layout.fillWidth: true
            
            from: 0
            to: 360
            
            onValueChanged: {
                if (motorRunning) {
                    posSet = value
                }
            }
        }
        
        RowLayout {
            Button {
                Layout.fillWidth: true
                text: "Start"
                
                onClicked: {
                    mCommands.setPos(posSlider.value)
                    motorRunning = true
                }
            }
            
            Button {
                Layout.fillWidth: true
                text: "Release"
                
                onClicked: {
                    mCommands.setCurrent(0.0)
                    motorRunning = false
                }
            }
        }
        
        Item {
            Layout.fillHeight: true
        }
    }
    
    Connections {
        target: mCommands
        
        function onValuesReceived(values, mask) {
            if (!motorRunning) {
                posSlider.value = values.position
                posSet = values.position
            }
        }
    }
    
    Timer {
        id: aliveTimer
        interval: 20
        running: true
        repeat: true
        
        onTriggered: {
            if (VescIf.isPortConnected()) {
                mCommands.getValues()
                if (motorRunning) {
                    mCommands.setPos(posSet)
                }
            }
        }
    }
}
