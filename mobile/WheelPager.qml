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

// Wheel and touchpad paging of SwipeViews, one page per gesture. With scroll phases (macOS, Wayland, Windows
// precision touchpads) the pages follow the fingers and snap when they lift. Other wheel events turn one page per
// burst. A gesture that scrolls something on the page does not change the page.
QtObject {
    id: wheelPager

    // Without scroll phases a pause this long starts a new burst
    property int gestureGap: 150
    // Scrolling in one burst that changes the page, 120 is one wheel step
    property int pageDistance: 100

    property var attached: []
    property var trackers: []

    function attach(item) {
        if (!item || attached.indexOf(item) >= 0) {
            return
        }

        if (item instanceof SwipeView || item instanceof ScrollView || item instanceof Loader) {
            // Drops destroyed ones
            attached = attached.filter(o => o)
            trackers = trackers.filter(o => o)
            attached.push(item)
        }

        if (item instanceof SwipeView) {
            // Hosted in Repeaters, which a SwipeView does not make pages of, sized to receive events
            var list = item.contentItem
            var above = host.createObject(list, { z: 1, width: Qt.binding(() => list.width),
                                                  height: Qt.binding(() => list.height) })
            trackers.push(gestureTracker.createObject(above, { view: item }))
            // Under the pages, above the ListView
            list.contentItem.z = 0.5
            pageCatcher.createObject(host.createObject(list, { width: Qt.binding(() => list.width),
                                                               height: Qt.binding(() => list.height) }),
                                     { orientation: item.orientation })
            // Pages can be added later
            var view = item
            var attachPages = () => {
                for (var p = 0; p < view.contentChildren.length; p++) {
                    attach(view.contentChildren[p])
                }
            }
            view.contentChildrenChanged.connect(attachPages)
            attachPages()
        } else if (item instanceof ScrollView) {
            // A ScrollView keeps what its content leaves
            leftoverCatcher.createObject(item, { orientation: Qt.Horizontal })
            leftoverCatcher.createObject(item, { orientation: Qt.Vertical })
        } else if (item instanceof Loader) {
            item.loaded.connect(() => attach(item.item))
        }

        for (var i = 0; i < item.children.length; i++) {
            attach(item.children[i])
        }
    }

    // Scrolling along orientation, 0 when mostly across it
    function delta(wheel, orientation) {
        var d = wheel.angleDelta.x !== 0 || wheel.angleDelta.y !== 0 ? wheel.angleDelta : wheel.pixelDelta
        if (orientation === Qt.Horizontal) {
            return Math.abs(d.x) > Math.abs(d.y) ? d.x : 0
        }
        return Math.abs(d.y) >= Math.abs(d.x) ? d.y : 0
    }

    // To the closest SwipeView along the event, or the next one out at its first or last page
    function handOver(item, wheel) {
        var along = delta(wheel, Qt.Horizontal) !== 0 ? Qt.Horizontal : Qt.Vertical
        for (var p = item; p; p = p.parent) {
            var tracker = p instanceof SwipeView && p.orientation === along ? trackers.find(t => t && t.view === p)
                                                                             : null
            if (tracker && tracker.leftover(wheel)) {
                return
            }
        }
    }

    property Component host: Component {
        Repeater { }
    }

    // Above the pages, sees every event first. Takes the rest of a gesture that changed the page.
    property Component gestureTracker: Component {
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            cursorShape: undefined

            property var view: null
            property bool stepped: false
            // Something on the page used the gesture
            property bool used: false
            // Cleared when left over
            property bool pending: false
            property real lastTime: 0
            // Scrolling along and across the pages in the gesture
            property real sum: 0
            property real across: 0

            // With scroll phases: offset from the page at startIndex, fling in pixels per ms
            property int lastPhase: Qt.NoScrollPhase
            property bool tracking: false
            property int startIndex: 0
            property real offset: 0
            property real fling: 0

            onWheel: (wheel) => wheel.phase === Qt.NoScrollPhase ? unphased(wheel) : phased(wheel)

            function startGesture() {
                sum = 0
                across = 0
                stepped = false
                used = false
                pending = false
            }

            function unphased(wheel) {
                if (wheelPager.delta(wheel, view.orientation) === 0) {
                    wheel.accepted = false
                    return
                }

                var now = Date.now()
                if (now - lastTime > wheelPager.gestureGap) {
                    startGesture()
                } else if (pending) {
                    used = true
                }
                lastTime = now
                pending = !stepped
                wheel.accepted = stepped
            }

            // A gesture starts with its first update, as macOS can send coasting as a gesture without updates
            function phased(wheel) {
                var update = wheel.phase === Qt.ScrollUpdate
                if (tracking) {
                    if (update) {
                        follow(wheel)
                    } else {
                        release()
                    }
                } else if (update && lastPhase !== Qt.ScrollUpdate) {
                    startGesture()
                } else if (pending) {
                    used = true
                }
                lastPhase = wheel.phase
                var horizontal = view.orientation === Qt.Horizontal
                sum += horizontal ? wheel.angleDelta.x : wheel.angleDelta.y
                across += horizontal ? wheel.angleDelta.y : wheel.angleDelta.x
                pending = !stepped && !tracking && update && wheelPager.delta(wheel, view.orientation) !== 0
                wheel.accepted = (tracking || stepped) && (update || wheel.phase === Qt.ScrollMomentum)
            }

            // An event nothing on the page took. False at the first or last page, for the SwipeView around it.
            function leftover(wheel) {
                // Already handed over, or coasting
                if (!pending) {
                    return true
                }

                pending = false
                if (used) {
                    return true
                }
                var d = wheelPager.delta(wheel, view.orientation)
                if (!view.interactive || (d < 0 ? view.currentIndex >= view.count - 1 : view.currentIndex <= 0)) {
                    return false
                }
                if (wheel.phase !== Qt.NoScrollPhase) {
                    // Only once clearly along the pages, so a list can take a diagonal start
                    if (Math.abs(sum) > 2 * Math.abs(across)) {
                        tracking = true
                        startIndex = view.currentIndex
                        offset = 0
                        settle.stop()
                        follow(wheel)
                    }
                    return true
                }

                sum += d
                if (Math.abs(sum) >= wheelPager.pageDistance) {
                    if (sum < 0) {
                        view.incrementCurrentIndex()
                    } else {
                        view.decrementCurrentIndex()
                    }
                    stepped = true
                }
                return true
            }

            function follow(wheel) {
                var horizontal = view.orientation === Qt.Horizontal
                var size = horizontal ? view.width : view.height
                var px = horizontal ? wheel.pixelDelta.x : wheel.pixelDelta.y
                var now = Date.now()
                fling = px / Math.max(8, now - lastTime)
                lastTime = now
                lost.restart()
                offset = Math.max(startIndex > 0 ? -size : 0,
                                  Math.min(startIndex < view.count - 1 ? size : 0, offset - px))
                var page = view.itemAt(startIndex)
                if (page) {
                    view.contentItem[settle.property] = (horizontal ? page.x : page.y) + offset
                }
            }

            // To the next page after a fifth of it or a recent flick, else back
            function release() {
                tracking = false
                stepped = true
                var horizontal = view.orientation === Qt.Horizontal
                var size = horizontal ? view.width : view.height
                var flick = Date.now() - lastTime < 50 ? fling : 0
                var next = Math.abs(flick) > 0.3 ? flick * offset < 0 && Math.abs(offset) > 0.03 * size
                                               : Math.abs(offset) > 0.2 * size
                var target = startIndex + (next ? (offset > 0 ? 1 : -1) : 0)
                var list = view.contentItem
                if (view.currentIndex !== target) {
                    // It animates from its highlight, which can be stale
                    if (list.highlightItem) {
                        list.highlightItem[horizontal ? "x" : "y"] = list[settle.property]
                    }
                    view.setCurrentIndex(target)
                } else if (view.itemAt(target)) {
                    // No index change, so no animation of its own
                    settle.to = horizontal ? view.itemAt(target).x : view.itemAt(target).y
                    settle.restart()
                }
            }

            NumberAnimation {
                id: settle
                target: view ? view.contentItem : null
                property: view && view.orientation === Qt.Vertical ? "contentY" : "contentX"
                duration: 250
                easing.type: Easing.OutCubic
            }
            // Something else changed the page
            Connections {
                target: view
                function onCurrentIndexChanged() { settle.stop() }
            }
            // When the end of the gesture got lost. Long, as resting fingers send nothing.
            Timer {
                id: lost
                interval: 3000
                onTriggered: if (tracking) release()
            }
        }
    }

    // Keeps from the ListView what it would flick with, and gesture starts and ends, after which it would count as
    // pressed and not move to the new page
    property Component pageCatcher: Component {
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            cursorShape: undefined
            property int orientation: Qt.Horizontal
            onWheel: (wheel) => {
                         var along = orientation === Qt.Horizontal ? wheel.angleDelta.x || wheel.pixelDelta.x
                                                                   : wheel.angleDelta.y || wheel.pixelDelta.y
                         if (along !== 0) {
                             wheelPager.handOver(parent, wheel)
                         } else if (wheel.phase !== Qt.ScrollBegin && wheel.phase !== Qt.ScrollEnd) {
                             wheel.accepted = false
                         }
                     }
        }
    }

    // Takes what a ScrollView leaves along its orientation
    property Component leftoverCatcher: Component {
        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            target: null
            // Checks the orientation for every event
            activeTimeout: 0

            onWheel: (wheel) => wheelPager.handOver(parent, wheel)
        }
    }
}
