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

// Wheel and touchpad scrolling across the pages of SwipeViews, one page per gesture. The page changes as soon as
// the gesture is long enough and moves like after a click on the tab bar. With the wheel the ListView of a SwipeView
// would coast and then settle on a page, and on Windows the touchpad keeps sending wheel events for a moment after
// the fingers lift, which moved the pages again.
//
// A gesture that scrolls something on the page, such as a list, does not change the page, also not when the list
// reaches its end during the gesture. ScrollViews keep the wheel events their content does not use, so attach()
// takes those from them too. It follows Loaders to the items they load.
QtObject {
    id: wheelPager

    // Without scroll phases, as on Windows, wheel events closer in time belong to the same gesture. Long enough
    // that a page loading for a moment does not end it.
    property int gestureGap: 150
    // Scrolling in one gesture that changes the page, 120 is one wheel step
    property int pageDistance: 100

    property var attached: []
    property var trackers: []

    function attach(item) {
        if (!item || attached.indexOf(item) >= 0) {
            return
        }

        if (item instanceof SwipeView || item instanceof ScrollView || item instanceof Loader) {
            // Without the ones destroyed since, such as a reloaded HwUi
            attached = attached.filter(o => o)
            trackers = trackers.filter(o => o)
            attached.push(item)
        }

        if (item instanceof SwipeView) {
            // A SwipeView makes pages of the items added to it or its ListView, but not of a Repeater. As large as
            // the ListView, as events are not looked for in children outside an item they all started inside of.
            var list = item.contentItem
            var above = host.createObject(list, { z: 1, width: Qt.binding(() => list.width),
                                                  height: Qt.binding(() => list.height) })
            trackers.push(gestureTracker.createObject(above, { view: item }))
            // Its handler gets what the pages leave over before the ListView, which would flick with it
            leftoverCatcher.createObject(list, { orientation: item.orientation })
            // The ListView takes in its pages only when it lays them out, and pages can be added later
            var view = item
            var attachPages = () => {
                for (var p = 0; p < view.contentChildren.length; p++) {
                    attach(view.contentChildren[p])
                }
            }
            view.contentChildrenChanged.connect(attachPages)
            attachPages()
        } else if (item instanceof ScrollView) {
            // And before the ScrollView, which would keep it
            leftoverCatcher.createObject(item, { orientation: Qt.Horizontal })
            leftoverCatcher.createObject(item, { orientation: Qt.Vertical })
        } else if (item instanceof Loader) {
            item.loaded.connect(() => attach(item.item))
        }

        for (var i = 0; i < item.children.length; i++) {
            attach(item.children[i])
        }
    }

    // Scrolling along orientation, 0 for scrolling mostly across it
    function delta(wheel, orientation) {
        var d = wheel.angleDelta.x !== 0 || wheel.angleDelta.y !== 0 ? wheel.angleDelta : wheel.pixelDelta
        if (orientation === Qt.Horizontal) {
            return Math.abs(d.x) > Math.abs(d.y) ? d.x : 0
        }
        return Math.abs(d.y) > Math.abs(d.x) ? d.y : 0
    }

    // The gesture tracker of the closest SwipeView with the given orientation, from item outwards
    function trackerFor(item, orientation) {
        for (var p = item; p; p = p.parent) {
            if (p instanceof SwipeView && p.orientation === orientation) {
                for (var i = 0; i < trackers.length; i++) {
                    if (trackers[i] && trackers[i].view === p) {
                        return trackers[i]
                    }
                }
            }
        }
        return null
    }

    property Component host: Component {
        Repeater { }
    }

    // Above the pages, so it gets every wheel event over them first and splits them into gestures. After the gesture
    // changed the page it takes the rest of it, so that nothing on the new page scrolls.
    property Component gestureTracker: Component {
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            // Leaves the cursor to the items below
            cursorShape: undefined

            property var view: null
            property real lastTime: 0
            property real lastDelta: 0
            // Events in a row against the direction of the gesture
            property int against: 0
            property real sum: 0
            property bool stepped: false
            property real stepTime: 0
            // Something on the page used the gesture
            property bool used: false
            // Delivered on to the pages, cleared when it is left over
            property bool pending: false
            // When the recent events of the gesture came
            property var times: []

            function startGesture() {
                sum = 0
                stepped = false
                used = false
                pending = false
                times = []
            }

            // A Windows touchpad sends events while the fingers move several times as often as the ones that keep
            // the scrolling going after they lift. Events coming that much more often again are the next swipe,
            // which stops the coasting without a pause. Not right after the page changed, when the events held up
            // by the new page can come all at once.
            function nextSwipe(now) {
                if (!stepped || now - stepTime < 200) {
                    return false
                }

                var recent = 0
                var before = 0
                for (var i = 0; i < times.length; i++) {
                    if (times[i] > now - 32) {
                        recent++
                    } else if (times[i] > now - 96) {
                        before++
                    }
                }
                return recent >= 5 && recent / 32 >= 2.5 * before / 64
            }

            onWheel: (wheel) => {
                         // Touchpads with scroll phases say where a gesture starts. Without them a pause, a turn or a
                         // next swipe ends it. One event against the direction is not a turn, some touchpads send
                         // those while coasting.
                         var phased = wheel.phase !== Qt.NoScrollPhase
                         if (wheel.phase === Qt.ScrollBegin) {
                             startGesture()
                         }

                         var d = wheelPager.delta(wheel, view.orientation)
                         if (d === 0) {
                             wheel.accepted = false
                             return
                         }

                         var now = Date.now()
                         against = (d < 0) !== (lastDelta < 0) ? against + 1 : 0
                         times = times.filter(t => t > now - 96).concat([now])
                         if (!phased && (now - lastTime > wheelPager.gestureGap || against >= 3 || nextSwipe(now))) {
                             startGesture()
                             against = 0
                             times = [now]
                         } else if (pending) {
                             used = true
                         }
                         lastTime = now
                         if (against === 0) {
                             lastDelta = d
                         }
                         pending = !stepped && against === 0
                         wheel.accepted = stepped
                     }

            // A wheel event that went past everything on the page. False when this SwipeView cannot use it, as it
            // is at its first or last page, so that one around it can.
            function leftover(wheel) {
                var d = wheelPager.delta(wheel, view.orientation)
                // Already handed over by another catcher
                if (!pending) {
                    return true
                }

                pending = false
                if (used) {
                    return true
                }
                if (!view.interactive || (d < 0 ? view.currentIndex >= view.count - 1 : view.currentIndex <= 0)) {
                    return false
                }

                sum += d
                if (Math.abs(sum) >= wheelPager.pageDistance) {
                    stepTime = Date.now()
                    if (sum < 0) {
                        view.incrementCurrentIndex()
                    } else {
                        view.decrementCurrentIndex()
                    }
                    stepped = true
                }
                return true
            }
        }
    }

    // Takes the wheel events left over for its parent and hands them to the closest SwipeView along the direction
    // they mostly go in. It only gets the ones with some scrolling along its orientation.
    property Component leftoverCatcher: Component {
        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            target: null
            // So that it goes back to looking at the orientation right after each event
            activeTimeout: 0

            onWheel: (wheel) => {
                         var along = wheelPager.delta(wheel, Qt.Horizontal) !== 0 ? Qt.Horizontal : Qt.Vertical
                         var tracker = wheelPager.trackerFor(parent, along)
                         while (tracker && !tracker.leftover(wheel)) {
                             tracker = wheelPager.trackerFor(tracker.view.parent, along)
                         }
                     }
        }
    }
}
