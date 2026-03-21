/*
    Reusable horizontal plot legend that reads from QtGraphs legendData.

    Usage (static series — auto-reads from series.legendData):
        PlotLegend { graphsView: myGraphsView }

    For dynamic series (created at runtime), bump `revision` after each replot:
        PlotLegend { graphsView: myGraphsView; revision: myRevisionCounter }
*/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Vedder.vesc

Flow {
    id: root

    // Point this at the GraphsView whose series should appear in the legend.
    property var graphsView: null

    // Bump this integer after dynamically adding/removing series to force a
    // legend refresh (seriesList is constant and does not notify).
    property int revision: 0

    // Derived model built from graphsView.seriesList + each series.legendData
    property var _items: {
        // depend on revision so the binding re-evaluates
        void(revision)
        var result = []
        if (!graphsView) return result
        var list = graphsView.seriesList
        if (!list) return result
        for (var i = 0; i < list.length; i++) {
            var s = list[i]
            if (!s || !s.visible) continue
            var ld = s.legendData
            if (ld && ld.length > 0) {
                result.push({ color: "" + ld[0].color, name: ld[0].label })
            } else if (s.name && s.name.length > 0) {
                result.push({ color: "" + s.color, name: s.name })
            }
        }
        return result
    }

    spacing: 12
    Layout.fillWidth: true

    Repeater {
        model: root._items

        Row {
            spacing: 4

            Rectangle {
                width: 14; height: 4
                radius: 2
                color: modelData.color
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                text: modelData.name
                font.pointSize: 9
                color: Utility.getAppHexColor("lightText")
            }
        }
    }
}
