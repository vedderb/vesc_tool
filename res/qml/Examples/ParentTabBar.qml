/*
 This example shows how to use the TabBar of the parent component for
 e.g. our own TabBar. This is useful to take advantage of the space
 from the original tab bar in UIs that run in fullscreen mode (which
 means that the other tabs are disabled).
 */
    
import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.12

Item {
    id: mainItem
    anchors.fill: parent

    property var parentTabBar: parent.tabBarItem
    
    Component.onCompleted: {
        parentTabBar.visible = true
        parentTabBar.enabled = true
    }
    
    TabBar {
        parent: parentTabBar
        anchors.fill: parent
        
        background: Rectangle {
            opacity: 1
            color: "#4f4f4f"
        }
        
        TabButton {
            text: "Test Button"
        }
    }
}
