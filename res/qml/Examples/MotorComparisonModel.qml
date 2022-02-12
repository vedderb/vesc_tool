// This QML code is a template for a load model that can be used in the custom tab of
// the motor comparison tool.

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0

Item {
    id: mainItem
    anchors.fill: parent
    anchors.margins: 5
    
    // The objectname has to be set to idComp for the comparison tool to find it
    objectName: "idComp"
    
    // This signal can be emitted when test parameters change to reload the graphs
    signal testChanged()
    
    property string xName: "Torque (Nm)"
    property double xMin: 0
    property double xMax: 1
    
    property double rpmNow: 1200
    property double torqueNow: 0
    
    // This function is called with a progress from xAxisMin to xAxisMax and
    // expects that the following list is returned:
    // [motor1_rpm motor1_torque extraParam1_m1 extraParam2_m1, motor2_rpm motor2_torque extraParam1_m2 extraParam2_m2]
    // extraParam are extra fields that will be added to the plot. They can be left as 0 if not needed.
    //
    // It is important that this function runs fast as it is called for every point
    // in the graph when it is updated. Avoiding UI-updates here and just doing
    // calculations is usually fine.
    function progressToParams(progress) {
        torqueNow = progress
        return [rpmNow, torqueNow, 0, 0, rpmNow, torqueNow, 0, 0]
    }
    
    // Name for the x-axis in the plot
    function xAxisName() {
        return xName
    }
    
    // Minimum value for the x-axis in the plot
    function xAxisMin() {
        return xMin
    }
    
    // Maximum valie for the x-axis in the plot
    function xAxisMax() {
        return xMax
    }
    
    // This function is called when the plot is right-clicked and and dragged. It can
    // be used to display information for that progress in the UI.
    function progressSelected(prog) {
        text.text = "Progress selected: " + parseFloat(prog).toFixed(2)
    }
    
    // This function is called to get the names of the optional extra parameters
    function extraNames() {
        return ["ParamA_m1", "ParamB_m1", "ParamA_m2", "ParamB_m2"]
    }

    GridLayout {
        Text {
            Layout.fillWidth: true
            Layout.columnSpan: 2
            Layout.topMargin: 10
            Layout.bottomMargin: 20
            color: "White"
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 20
            text: "Custom Calculator"
        }
        
        anchors.fill: parent
        columns: 2
        rowSpacing: -10
        
        Text {
            text: "Load"
            color: "white"
        }
        
        Slider {
            Layout.fillWidth: true
            from: 0
            to: 10
            value: 0
            
            onValueChanged: {
                xMax = value
                testChanged()
            }
        }
        
        Text {
            text: "RPM:"
            color: "white"
        }
        
        ComboBox {
            id: dropDownBox
            Layout.fillWidth: true
            editable: false
            
            model: ListModel {
                id: model
                ListElement { text: "RPM1" }
                ListElement { text: "RPM2" }
            }
            
            onCurrentIndexChanged: {
                if (currentIndex == 0) {
                    rpmNow = 1200
                } else if (currentIndex == 1) {
                    rpmNow = 1800
                }
                testChanged()
            }
        }
        
        Text {
            id:text
            Layout.topMargin: 20
            Layout.fillHeight: true
            Layout.columnSpan: 2
            Layout.fillWidth: true
            font.family: "DejaVu Sans Mono"
            color: "white"
            text: "test"
        }
    }
}
