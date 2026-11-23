/*
    Copyright 2020 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Controls 2.10
import QtQuick.Layouts 1.3

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.utility 1.0

Item {
    id: bmsPageItem
    property Commands mCommands: VescIf.commands()
    property var mVal
    property bool mValSet: false
    // Cell and temperature bars side by side, as on the desktop BMS page or where there is room
    property bool forceWide: false
    readonly property bool wide: forceWide || width >= 900 || (width > height && width >= 640)

    readonly property bool hasCells: mValSet && mVal.v_cells.length > 0
    readonly property real cvMin: hasCells ? Math.min(...mVal.v_cells) : 0
    readonly property real cvMax: hasCells ? Math.max(...mVal.v_cells) : 0

    readonly property var temps: mValSet ? mVal.temps : []
    readonly property var tempLabels: mValSet && mVal.data_version === 1 ? ["IC", "Cell Min", "Cell Max", "Mosfet", "Ambient"] : []
    readonly property var validTemps: temps.filter(t => tempValid(t))
    readonly property real tMin: validTemps.length > 0 ? Math.min(...validTemps) : 0
    readonly property real tMax: validTemps.length > 0 ? Math.max(...validTemps) : 0
    // The PCB sensors are the humidity sensor and, with labels, the IC and the mosfets
    readonly property real tempPcbMax: {
        if (!mValSet) {
            return 0
        }
        var pcb = (mVal.data_version === 1 && temps.length > 3 ? [mVal.temp_hum_sensor, temps[0], temps[3]] :
                                                                 [mVal.temp_hum_sensor]).filter(t => tempValid(t))
        return pcb.length > 0 ? Math.max(...pcb) : -300
    }
    // Cell bar scale: 3.0 to 4.2 V for lithium ion cells, around the cells where they are outside of that, such as
    // LTO cells. The bars round it to their grid.
    readonly property real cellRangeLo: cvMin < 3.0 ? cvMin - 0.15 : 3.0
    readonly property real cellRangeHi: cvMax > 4.2 ? cvMax + 0.05 : cvMax < 3.0 ? cvMax + 0.35 : 4.2

    function tempValid(t) { return t > -280.0 }
    function tempLabel(i) { return i < tempLabels.length ? tempLabels[i] : "T" + (i + 1 - tempLabels.length) }
    function tempColor(t) {
        return Utility.getAppHexColor(t >= 55 ? "danger" : t >= 45 ? "warning" : "lightAccent")
    }
    // For text, which needs more contrast than the bars in the light theme
    function tempTextColor(t) {
        return Utility.getAppHexColor(t >= 55 ? "dangerText" : t >= 45 ? "warningText" : "lightText")
    }
    function tempText(t) { return tempValid(t) ? fmt(t, 1) + " °C" : qsTr("N/A") }
    function fmt(v, decimals) { return parseFloat(v).toFixed(decimals) }

    // The font of the bar labels
    Text {
        id: barFont
        visible: false
        font.pointSize: 10
    }

    TextMetrics {
        id: tempLabelMetrics
        font: barFont.font
        text: tempLabels.length > 0 ? "Cell Max" : "T" + temps.length
    }

    // Bars side by side are as tall as the taller of the two, the other one spreads its rows
    readonly property real barsHeight: wide ? Math.max(cellCard.naturalHeight, tempCard.naturalHeight) : 0

    component Card: Rectangle {
        radius: 12
        color: Utility.getAppHexColor("cardBackground")
        border.color: Utility.getAppHexColor("raisedBackground")
    }

    // Title and a summary line on the right
    component CardHeader: RowLayout {
        property alias title: titleText.text
        property alias summary: summaryText.text
        property color summaryColor: Utility.getAppHexColor("normalText")

        spacing: 8

        Text {
            id: titleText
            color: Utility.getAppHexColor("normalText")
            font.pointSize: 9
            font.letterSpacing: 0.5
            font.capitalization: Font.AllUppercase
        }

        Text {
            id: summaryText
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideLeft
            color: parent.summaryColor
            font.pointSize: 9
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // A Flickable rather than a ScrollView, which rounds the scroll position to whole pixels, so that scrolling
        // glides smoothly, and which does not scroll with a mouse drag on the desktop
        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: content.height + 20
            clip: true
            ScrollBar.vertical: ScrollBar {
                // Only an indicator on phones, as from a ScrollView after a touch
                interactive: Qt.platform.os !== "ios" && Qt.platform.os !== "android"
            }

            ColumnLayout {
                id: content
                x: 10
                y: 10
                width: flick.width - 20
                spacing: 10

                GridLayout {
                    Layout.fillWidth: true
                    columns: 4
                    columnSpacing: 6

                    Repeater {
                        model: [["Pack", mValSet ? fmt(mVal.v_tot, 2) : "-", "V"],
                            ["Current", mValSet ? fmt(mVal.i_in_ic, 1) : "-", "A"],
                            ["SoC", mValSet ? fmt(mVal.soc * 100, 0) : "-", "%"],
                            ["Min-Max", hasCells ? fmt((cvMax - cvMin) * 1000, 0) : "-", "mV"]]

                        ValueTile {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            title: modelData[0]
                            value: modelData[1]
                            unit: modelData[2]
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    visible: !mValSet
                    horizontalAlignment: Text.AlignHCenter
                    color: Utility.getAppHexColor("normalText")
                    text: qsTr("No Data")
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: wide ? 2 : 1
                    columnSpacing: 10
                    rowSpacing: 10
                    visible: mValSet

                    Card {
                        id: cellCard
                        readonly property real naturalHeight: cellHeader.implicitHeight + cellCanvas.naturalHeight + 30
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.alignment: Qt.AlignTop
                        implicitHeight: wide ? barsHeight : naturalHeight
                        visible: hasCells

                        CardHeader {
                            id: cellHeader
                            x: 12
                            y: 10
                            width: parent.width - 24
                            title: qsTr("Cells") + (hasCells ? " · " + mVal.v_cells.length : "")
                            summary: hasCells ? qsTr("min %1 V (C%2) · max %3 V (C%4)").
                                                arg(fmt(cvMin, 3)).arg(mVal.v_cells.indexOf(cvMin) + 1).
                                                arg(fmt(cvMax, 3)).arg(mVal.v_cells.indexOf(cvMax) + 1) : ""
                        }

                        Canvas {
                            id: cellCanvas
                            readonly property int minRowHeight: 24
                            readonly property int rows: hasCells ? mVal.v_cells.length : 0
                            readonly property real naturalHeight: rows * minRowHeight + 18
                            // Spread to the height of the temperatures next to it, but not into a sparse list
                            readonly property real rowHeight: rows > 0 ? Math.min(Math.max(minRowHeight, (height - 18) / rows),
                                                                                  minRowHeight * 1.6) : minRowHeight
                            x: 12
                            y: cellHeader.y + cellHeader.implicitHeight + 8
                            width: parent.width - 24
                            height: parent.height - y - 10

                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()

                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.reset()

                                if (!hasCells) {
                                    return
                                }

                                var vCells = mVal.v_cells
                                var isBal = mVal.is_balancing
                                var xOfsLeft = 34
                                var cellWidth = width - xOfsLeft - 96
                                var heightBars = vCells.length * rowHeight

                                // Grid steps with round labels, at least 40 px apart
                                var steps = [0.1, 0.2, 0.25, 0.5, 1.0]
                                var step = 1.0
                                for (var si = 0; si < steps.length; si++) {
                                    var divs = Math.ceil(cellRangeHi / steps[si] - 1e-6) - Math.floor(cellRangeLo / steps[si] + 1e-6)
                                    if (cellWidth / divs >= 40) {
                                        step = steps[si]
                                        break
                                    }
                                }
                                var cellLo = Math.floor(cellRangeLo / step + 1e-6) * step
                                var cellHi = Math.ceil(cellRangeHi / step - 1e-6) * step
                                var lineDivs = Math.round((cellHi - cellLo) / step)
                                var lineStep = cellWidth / lineDivs

                                ctx.font = '%1pt \"%2\"'.arg(barFont.font.pointSize).arg(barFont.font.family)
                                ctx.textBaseline = "middle"

                                ctx.beginPath()

                                for (var i = 0; i <= lineDivs;i++) {
                                    ctx.moveTo(xOfsLeft + lineStep * i, 0)
                                    ctx.lineTo(xOfsLeft + lineStep * i, heightBars)
                                }

                                ctx.strokeStyle = Utility.getAppHexColor("raisedBackground")
                                ctx.lineWidth = 1
                                ctx.stroke()

                                ctx.fillStyle = Utility.getAppHexColor("normalText")
                                ctx.textAlign = "center"

                                for (i = 0; i <= lineDivs;i++) {
                                    ctx.fillText(fmt(cellLo + i * step, step === 0.25 ? 2 : 1), xOfsLeft + lineStep * i, heightBars + 9)
                                }

                                for (i = 0;i < vCells.length;i++) {
                                    var y = i * rowHeight + rowHeight / 2
                                    var isMin = i === vCells.indexOf(cvMin) && Math.round((cvMax - cvMin) * 1000) >= 50
                                    var tagColor = Utility.getAppHexColor(isBal[i] ? "infoText" : "warningText")
                                    var cellW = Math.max(Math.min((vCells[i] - cellLo) / (cellHi - cellLo) * cellWidth, cellWidth), 1.2)

                                    ctx.fillStyle = Utility.getAppHexColor("normalText")
                                    ctx.textAlign = "start"
                                    ctx.fillText("C" + (i + 1), 0, y)

                                    ctx.beginPath()
                                    ctx.roundedRect(xOfsLeft, y - 6, cellW, 12, 3, 3)
                                    ctx.fillStyle = isBal[i] ? Utility.getAppHexColor(Utility.isDarkMode() ? "infoText" : "vescBlue") :
                                                               Utility.getAppHexColor(isMin ? "warning" : "cellBar")
                                    ctx.fill()

                                    // Stripes on balancing cells
                                    if (isBal[i]) {
                                        ctx.save()
                                        ctx.clip()
                                        ctx.beginPath()

                                        for (var x = xOfsLeft - 12; x < xOfsLeft + cellW; x += 8) {
                                            ctx.moveTo(x, y + 6)
                                            ctx.lineTo(x + 12, y - 6)
                                        }

                                        ctx.strokeStyle = Utility.isDarkMode() ? Qt.darker(tagColor, 1.6) : Qt.rgba(1, 1, 1, 0.35)
                                        ctx.lineWidth = 3
                                        ctx.stroke()
                                        ctx.restore()
                                    }

                                    ctx.textAlign = "end"
                                    ctx.fillStyle = isMin ? tagColor : Utility.getAppHexColor("lightText")
                                    ctx.fillText(fmt(vCells[i], 3) + " V", width - 30, y)

                                    if (isBal[i] || isMin) {
                                        ctx.fillStyle = tagColor
                                        ctx.fillText(isBal[i] ? "BAL" : "MIN", width, y)
                                    }
                                }
                            }
                        }
                    }

                    Card {
                        id: tempCard
                        // Up to 12 sensors as bars like the cells, more as a grid of tiles, so that 50 stay readable
                        readonly property bool tiles: temps.length > 12
                        readonly property real naturalHeight: tempHeader.implicitHeight + 30 +
                                                              (tiles ? tileGrid.naturalHeight : tempCanvas.naturalHeight)
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.alignment: Qt.AlignTop
                        implicitHeight: wide ? barsHeight : naturalHeight
                        visible: temps.length > 0

                        // Scale in steps of 10 degrees, 0 to 60 at least
                        readonly property real tLo: Math.min(0, Math.floor((tMin - 5) / 10) * 10)
                        readonly property real tHi: Math.max(60, Math.ceil((tMax + 5) / 10) * 10)

                        CardHeader {
                            id: tempHeader
                            x: 12
                            y: 10
                            width: parent.width - 24
                            title: qsTr("Temperatures") + " · " + temps.length
                            summary: validTemps.length > 0 ?
                                         qsTr("min %1 °C · max %2 °C (%3)").arg(fmt(tMin, 1)).arg(fmt(tMax, 1)).
                                         arg(tempLabel(temps.indexOf(tMax))) : qsTr("No sensors")
                            summaryColor: validTemps.length > 0 && tMax >= 45 ? tempTextColor(tMax) :
                                                                                 Utility.getAppHexColor("normalText")
                        }

                        Canvas {
                            id: tempCanvas
                            readonly property int minRowHeight: 24
                            readonly property real labelWidth: tempLabelMetrics.advanceWidth + 10
                            readonly property real valueWidth: 64
                            readonly property int rows: temps.length
                            readonly property real naturalHeight: rows * minRowHeight + 18
                            // Spread to the height of the cells next to it, but not into a sparse list
                            readonly property real rowHeight: rows > 0 ? Math.min(Math.max(minRowHeight, (height - 18) / rows),
                                                                                  minRowHeight * 1.6) : minRowHeight
                            x: 12
                            y: tempHeader.y + tempHeader.implicitHeight + 8
                            width: parent.width - 24
                            height: parent.height - y - 10
                            visible: !tempCard.tiles

                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()

                                if (!visible || temps.length === 0) {
                                    return
                                }

                                var tLo = tempCard.tLo
                                var span = tempCard.tHi - tLo
                                var barWidth = width - labelWidth - valueWidth
                                var heightBars = rows * rowHeight
                                // Grid lines every 10 or 20 degrees, depending on the room
                                var step = barWidth / (span / 10) >= 28 ? 10 : 20

                                ctx.font = '%1pt \"%2\"'.arg(barFont.font.pointSize).arg(barFont.font.family)
                                ctx.textBaseline = "middle"

                                ctx.beginPath()
                                for (var g = tLo; g <= tLo + span; g += step) {
                                    var gx = labelWidth + (g - tLo) / span * barWidth
                                    ctx.moveTo(gx, 0)
                                    ctx.lineTo(gx, heightBars)
                                }
                                ctx.strokeStyle = Utility.getAppHexColor("raisedBackground")
                                ctx.lineWidth = 1
                                ctx.stroke()

                                ctx.fillStyle = Utility.getAppHexColor("normalText")
                                ctx.textAlign = "center"
                                for (g = tLo; g <= tLo + span; g += step) {
                                    ctx.fillText(g.toFixed(0), labelWidth + (g - tLo) / span * barWidth, heightBars + 9)
                                }

                                for (var i = 0; i < temps.length; i++) {
                                    var y = i * rowHeight + rowHeight / 2
                                    var t = temps[i]
                                    var valid = tempValid(t)

                                    ctx.textAlign = "start"
                                    ctx.fillStyle = Utility.getAppHexColor("normalText")
                                    ctx.fillText(tempLabel(i), 0, y)

                                    if (valid) {
                                        var w = Math.max(Math.min((t - tLo) / span * barWidth, barWidth), 1.2)
                                        ctx.beginPath()
                                        ctx.roundedRect(labelWidth, y - 6, w, 12, 3, 3)
                                        ctx.fillStyle = tempColor(t)
                                        ctx.fill()
                                    }

                                    ctx.textAlign = "end"
                                    ctx.fillStyle = !valid ? Utility.getAppHexColor("disabledText") :
                                                             tempTextColor(t)
                                    ctx.fillText(valid ? fmt(t, 1) + " °C" : "N/A", width, y)
                                }
                            }
                        }

                        Grid {
                            id: tileGrid
                            readonly property int minTileHeight: 46
                            readonly property real minTileWidth: Math.max(tempLabelMetrics.advanceWidth + 20, 76)
                            readonly property int tileRows: Math.ceil(temps.length / columns)
                            readonly property real naturalHeight: tileRows * (minTileHeight + spacing) - spacing
                            readonly property real tileWidth: (width - (columns - 1) * spacing) / columns
                            // Taller up to a point next to more cells
                            readonly property real tileHeight: Math.min(Math.max(minTileHeight,
                                                                                 (height - (tileRows - 1) * spacing) / tileRows),
                                                                        minTileHeight * 1.4)
                            x: 12
                            y: tempHeader.y + tempHeader.implicitHeight + 8
                            width: parent.width - 24
                            height: parent.height - y - 12
                            visible: tempCard.tiles
                            columns: Math.max(1, Math.floor((width + spacing) / (minTileWidth + spacing)))
                            spacing: 6

                            Repeater {
                                model: tempCard.tiles ? temps.length : 0

                                Rectangle {
                                    readonly property real t: temps[index]
                                    readonly property bool valid: tempValid(t)
                                    width: tileGrid.tileWidth
                                    height: tileGrid.tileHeight
                                    radius: 8
                                    clip: true
                                    // Warm and hot sensors stand out in the colors of the bars
                                    color: valid && t >= 45 ? Qt.alpha(tempColor(t), Utility.isDarkMode() ? 0.25 : 0.18) :
                                                              Utility.getAppHexColor("raisedBackground")
                                    opacity: valid ? 1 : 0.5

                                    Text {
                                        x: 8
                                        y: 5
                                        width: parent.width - 16
                                        elide: Text.ElideRight
                                        text: tempLabel(index)
                                        color: Utility.getAppHexColor("normalText")
                                        font.pointSize: 8
                                    }

                                    Text {
                                        x: 8
                                        anchors.bottom: parent.bottom
                                        anchors.bottomMargin: 7
                                        text: valid ? fmt(t, 1) + "°" : "N/A"
                                        color: valid ? tempTextColor(t) : Utility.getAppHexColor("lightText")
                                        font.pointSize: 11
                                        font.weight: Font.Medium
                                    }

                                    // Where it is on the scale of the bars
                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        height: 3
                                        width: valid ? parent.width * Math.min(Math.max((t - tempCard.tLo) /
                                                                                        (tempCard.tHi - tempCard.tLo), 0.03), 1) : 0
                                        color: tempColor(t)
                                    }
                                }
                            }
                        }
                    }
                }

                // Where the sensors of the 12s7p BMS are on its board
                Card {
                    id: boardCard
                    readonly property real s: boardImage.paintedWidth / 3766
                    Layout.fillWidth: true
                    implicitHeight: boardImage.height + 20
                    visible: mValSet && temps.length >= 6 && VescIf.getLastFwRxParams().hw === "12s7p"

                    Image {
                        id: boardImage
                        y: 10
                        width: Math.min(parent.width - 20, 900)
                        height: width * 2011 / 3766
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: boardCard.visible ? "qrc:/res/images/12s7p_pcb.png" : ""
                        fillMode: Image.PreserveAspectFit
                        smooth: true

                        // Positions on the image, at the baseline of the text
                        Repeater {
                            model: boardCard.visible ? [
                                [2985, 1840, tempText(mVal.temp_ic), mVal.temp_ic],
                                [2985, 1467, tempText(temps[0]), temps[0]],
                                [30, 890, tempText(temps[1]), temps[1]],
                                [2985, 260, tempText(temps[2]), temps[2]],
                                [2985, 600, tempText(temps[3]), temps[3]],
                                [2985, 945, tempText(mVal.temp_hum_sensor), mVal.temp_hum_sensor],
                                [2985, 1050, fmt(mVal.humidity, 1) + " %RH", -300],
                                [30, 1506, tempText(temps[5]), temps[5]],
                                [30, 1794, tempText(temps[4]), temps[4]]] : []

                            Text {
                                x: modelData[0] * boardCard.s
                                y: modelData[1] * boardCard.s - ascentMetrics.ascent
                                text: modelData[2]
                                color: tempValid(modelData[3]) && modelData[3] >= 45 ? tempTextColor(modelData[3]) :
                                                                                      Utility.getAppHexColor("positiveText")
                                font: ascentMetrics.font
                            }
                        }

                        FontMetrics {
                            id: ascentMetrics
                            font.family: "DejaVu Sans Mono"
                            font.pixelSize: Math.max(8, 96 * boardCard.s)
                            font.weight: Font.Medium
                        }
                    }
                }

                // The other values, in groups
                GridLayout {
                    Layout.fillWidth: true
                    columns: wide ? 3 : 1
                    columnSpacing: 10
                    rowSpacing: 10
                    visible: mValSet

                    Repeater {
                        model: mValSet ? [
                            [qsTr("Pack"), [
                                    [qsTr("Status"), mVal.status !== "" ? mVal.status : "-"],
                                    [qsTr("Input Current"), fmt(mVal.i_in, 2) + " A"],
                                    [qsTr("Power"), fmt(mVal.v_tot * mVal.i_in_ic, 1) + " W"],
                                    [qsTr("Charge Voltage"), fmt(mVal.v_charge, 2) + " V"],
                                    [qsTr("State of Health"), fmt(mVal.soh * 100, 0) + " %"]]],
                            [qsTr("Energy"), [
                                    [qsTr("Counters"), fmt(mVal.ah_cnt, 3) + " Ah · " + fmt(mVal.wh_cnt, 2) + " Wh"],
                                    [qsTr("Charged Total"), fmt(mVal.ah_cnt_chg_total, 3) + " Ah · " + fmt(mVal.wh_cnt_chg_total, 2) + " Wh"],
                                    [qsTr("Discharged Total"), fmt(mVal.ah_cnt_dis_total, 3) + " Ah · " + fmt(mVal.wh_cnt_dis_total, 2) + " Wh"]]],
                            [qsTr("Environment"), [
                                    [qsTr("Cells Max"), tempText(mVal.temp_cells_highest), mVal.temp_cells_highest],
                                    [qsTr("PCB Max"), tempText(tempPcbMax), tempPcbMax],
                                    [qsTr("Humidity"), fmt(mVal.humidity, 1) + " % · " + tempText(mVal.temp_hum_sensor)],
                                    [qsTr("Pressure"), fmt(mVal.pressure, 0) + " Pa"]]]] : []

                        Card {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            Layout.fillHeight: true
                            implicitHeight: groupColumn.implicitHeight + 16

                            Column {
                                id: groupColumn
                                x: 12
                                y: 8
                                width: parent.width - 24

                                Text {
                                    height: 24
                                    verticalAlignment: Text.AlignVCenter
                                    text: modelData[0]
                                    color: Utility.getAppHexColor("normalText")
                                    font.pointSize: 9
                                    font.letterSpacing: 0.5
                                    font.capitalization: Font.AllUppercase
                                }

                                Repeater {
                                    model: modelData[1]

                                    // The value under the label where both do not fit on one line
                                    Item {
                                        readonly property bool stacked: label.implicitWidth + 8 + value.implicitWidth > width
                                        width: groupColumn.width
                                        height: stacked ? 48 : 30

                                        Text {
                                            id: label
                                            width: parent.width
                                            y: parent.stacked ? 4 : (30 - height) / 2
                                            elide: Text.ElideRight
                                            text: modelData[0]
                                            color: Utility.getAppHexColor("normalText")
                                        }

                                        Text {
                                            id: value
                                            anchors.right: parent.right
                                            width: Math.min(implicitWidth, parent.width)
                                            y: parent.stacked ? parent.height - height - 4 : (30 - height) / 2
                                            elide: Text.ElideLeft
                                            text: modelData[1]
                                            // Temperatures in the colors of the bars once they get warm
                                            color: modelData.length > 2 && tempValid(modelData[2]) ? tempTextColor(modelData[2]) :
                                                                                                    Utility.getAppHexColor("lightText")
                                            font.weight: Font.Medium
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.margins: 10
            Layout.fillWidth: true

            Button {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                text: "Balance ▾"
                onClicked: balanceMenu.open()

                BottomMenu {
                    id: balanceMenu

                    MenuEntry {
                        text: "Balance On"
                        iconFile: "Balance On-96.png"
                        onTriggered: {
                            mCommands.bmsForceBalance(true)
                        }
                    }
                    MenuEntry {
                        text: "Balance Off"
                        iconFile: "Balance Off-96.png"
                        onTriggered: {
                            mCommands.bmsForceBalance(false)
                        }
                    }
                }
            }

            Button {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                text: "Charging ▾"
                onClicked: chargeMenu.open()

                BottomMenu {
                    id: chargeMenu

                    MenuEntry {
                        text: "Allow Charging"
                        iconFile: "Charge On-96.png"
                        onTriggered: {
                            mCommands.bmsSetChargeAllowed(true)
                        }
                    }
                    MenuEntry {
                        text: "Disable Charging"
                        iconFile: "Charge Off-96.png"
                        onTriggered: {
                            mCommands.bmsSetChargeAllowed(false)
                        }
                    }
                }
            }

            Button {
                id: menuButton
                Layout.preferredWidth: 56
                text: "..."
                onClicked: menu.open()

                BottomMenu {
                    id: menu

                    MenuEntry {
                        text: "Reset Ah Counter"
                        iconFile: "Reset Counter-96.png"
                        onTriggered: {
                            mCommands.bmsResetCounters(true, false)
                        }
                    }
                    MenuEntry {
                        text: "Reset Wh Counter"
                        iconFile: "Reset Counter-96.png"
                        onTriggered: {
                            mCommands.bmsResetCounters(false, true)
                        }
                    }
                    MenuEntry {
                        text: "Zero Current Offset"
                        iconFile: "Offset-96.png"
                        onTriggered: {
                            mCommands.bmsZeroCurrentOffset()
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: mCommands

        function onBmsValuesRx(val) {
            mVal = val
            mValSet = true
            cellCanvas.requestPaint()
            tempCanvas.requestPaint()
        }
    }
}
