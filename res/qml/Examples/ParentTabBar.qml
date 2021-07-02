/*
 This example shows how to use the TabBar of the parent component for
 e.g. our own TabBar. This is useful to take advantage of the space
 from the original tab bar in UIs that run in fullscreen mode (which
 means that the other tabs are disabled).
 */
    
import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0

Item {
    id: mainItem
    anchors.fill: parent

    Material.theme: Utility.isDarkMode() ? "Dark" : "Light"
    Material.accent: Utility.getAppHexColor("lightAccent")

    property var parentTabBar: parent.tabBarItem
    
    Component.onCompleted: {
        parentTabBar.visible = true
        parentTabBar.enabled = true
    }
    
    TabBar {
        id: localTabBar
        parent: parentTabBar
        anchors.fill: parent
        currentIndex: swipeView.currentIndex
        
        background: Rectangle {
            opacity: 1
            color: Utility.getAppHexColor("lightBackground")
        }
        
        property int buttonWidth: Math.max(120, localTabBar.width / (rep.model.length))

        Repeater {
            id: rep
            model: ["Tab 1", "Tab 2"]
            
            TabButton {
                text: modelData
                width: localTabBar.buttonWidth
            }
        }
    }
    
    SwipeView {
        id: swipeView
        currentIndex: localTabBar.currentIndex
        anchors.fill: parent
        clip: true
        
        Page {
            Text {
                anchors.fill: parent
                anchors.margins: 5
                text: "Tab 1"
                color: "white"
            }
        }
        
        Page {
            Text {
                anchors.fill: parent
                anchors.margins: 5
                text: "Tab 2"
                color: "white"
            }
        }
    }
}
