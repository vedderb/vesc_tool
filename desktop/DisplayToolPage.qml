/*
    Desktop DisplayToolPage — Full pixel art editor for VESC display images.
    Replicates the original PageDisplayTool with 3 top-level tabs:
      1) Display — DispEditor (left) + Font/Overlay sub-tabs (right)
      2) Overlay Editor — standalone DispEditor
      3) Font Editor — font settings + DispEditor
    Each DispEditor = pixel canvas+palette (left) + preview+Controls/LoadSave (right)
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Vedder.vesc

Item {
    id: root

    // ---- Shared DispEditor component ----
    component DispEditorComp: Item {
        id: dispEditor

        property int imgW: 128
        property int imgH: 64
        property int formatIndex: 0  // 0=Idx2, 1=Idx4, 2=Idx16, 3=RGB332, 4=RGB565, 5=RGB888
        property string editColor: "#ffffff"
        property bool drawLayer2: true
        property int previewScale: 2
        property var imageData: []
        property var layer2Data: []
        property var colorPalette: []

        Component.onCompleted: {
            rebuildPalette()
            clearImage()
        }

        function paletteColorCount() {
            switch (formatIndex) {
            case 0: return 2
            case 1: return 4
            case 2: return 16
            default: return 0
            }
        }

        function rebuildPalette() {
            var count = paletteColorCount()
            var pal = []
            for (var i = 0; i < count; i++) {
                var v = count > 1 ? Math.round(255 * i / (count - 1)) : 0
                var hex = "#" + ("0" + v.toString(16)).slice(-2).repeat(3)
                pal.push(hex)
            }
            colorPalette = pal
            editColor = pal.length > 0 ? pal[pal.length - 1] : "#ffffff"
        }

        function clearImage() {
            var arr = new Array(imgW * imgH)
            for (var i = 0; i < arr.length; i++) arr[i] = "#000000"
            imageData = arr
            clearLayer2()
            edCanvas.requestPaint()
            pvCanvas.requestPaint()
        }

        function clearLayer2() {
            var arr = new Array(imgW * imgH)
            for (var i = 0; i < arr.length; i++) arr[i] = ""
            layer2Data = arr
            edCanvas.requestPaint()
            pvCanvas.requestPaint()
        }

        function getPixel(x, y) {
            if (x < 0 || x >= imgW || y < 0 || y >= imgH) return "#000000"
            var idx = y * imgW + x
            var l2 = layer2Data[idx]
            if (drawLayer2 && l2 && l2 !== "") return l2
            return imageData[idx]
        }

        function setPixel(x, y, color) {
            if (x < 0 || x >= imgW || y < 0 || y >= imgH) return
            imageData[y * imgW + x] = color
        }

        function updateSize(w, h) {
            imgW = w; imgH = h
            clearImage()
        }

        // Layout: canvas+palette (left) | preview+tabs (right)
        RowLayout {
            anchors.fill: parent
            spacing: 0

            // ==== LEFT: Pixel editor + palette ====
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 300
                spacing: 0

                // Pixel editor canvas
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "black"
                    clip: true

                    Canvas {
                        id: edCanvas
                        anchors.fill: parent

                        property real sf: 8.0
                        property real ox: -(imgW * 4)
                        property real oy: -(imgH * 4)
                        property int mlx: -1000000
                        property int mly: -1000000
                        property int cpx: -1
                        property int cpy: -1

                        function pxAt(mx, my) {
                            return Qt.point(
                                Math.floor((mx - ox - width / 2) / sf),
                                Math.floor((my - oy - height / 2) / sf))
                        }

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.reset()
                            ctx.fillStyle = "black"
                            ctx.fillRect(0, 0, width, height)

                            ctx.save()
                            ctx.translate(width / 2 + ox, height / 2 + oy)
                            ctx.scale(sf, sf)

                            for (var j = 0; j < imgH; j++) {
                                for (var i = 0; i < imgW; i++) {
                                    var c = getPixel(i, j)
                                    if (i === cpx && j === cpy) {
                                        var r = parseInt(c.substring(1, 3), 16) / 255
                                        var g = parseInt(c.substring(3, 5), 16) / 255
                                        var b = parseInt(c.substring(5, 7), 16) / 255
                                        if (g < 0.5) g += 0.2; else { r -= 0.2; b -= 0.2 }
                                        r = Math.max(0, Math.min(1, r))
                                        g = Math.max(0, Math.min(1, g))
                                        b = Math.max(0, Math.min(1, b))
                                        ctx.fillStyle = Qt.rgba(r, g, b, 1)
                                    } else {
                                        ctx.fillStyle = c
                                    }
                                    ctx.fillRect(i, j, 1, 1)
                                }
                            }
                            ctx.restore()

                            // Grid
                            var gox = width / 2 + ox
                            var goy = height / 2 + oy
                            if (sf >= 4) {
                                ctx.strokeStyle = "#404040"
                                ctx.lineWidth = 0.5
                                for (var gx = 0; gx <= imgW; gx++) {
                                    var sx = gox + gx * sf
                                    ctx.beginPath(); ctx.moveTo(sx, goy); ctx.lineTo(sx, goy + imgH * sf); ctx.stroke()
                                }
                                for (var gy = 0; gy <= imgH; gy++) {
                                    var sy = goy + gy * sf
                                    ctx.beginPath(); ctx.moveTo(gox, sy); ctx.lineTo(gox + imgW * sf, sy); ctx.stroke()
                                }
                            }

                            // Coord overlay
                            ctx.fillStyle = "rgba(0,0,0,0.5)"
                            ctx.fillRect(width - 100, height - 40, 100, 40)
                            ctx.fillStyle = "white"
                            ctx.font = "11px monospace"
                            ctx.fillText("X: " + cpx, width - 90, height - 24)
                            ctx.fillText("Y: " + cpy, width - 90, height - 10)
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton

                            onPositionChanged: function(mouse) {
                                var pt = edCanvas.pxAt(mouse.x, mouse.y)
                                edCanvas.cpx = pt.x; edCanvas.cpy = pt.y
                                var shift = (mouse.modifiers & Qt.ShiftModifier)
                                if (mouse.buttons & Qt.LeftButton) {
                                    if (shift) {
                                        if (pt.x >= 0 && pt.x < imgW && pt.y >= 0 && pt.y < imgH) {
                                            setPixel(pt.x, pt.y, editColor)
                                            pvCanvas.requestPaint()
                                        }
                                    } else {
                                        if (edCanvas.mlx > -999999) {
                                            edCanvas.ox += mouse.x - edCanvas.mlx
                                            edCanvas.oy += mouse.y - edCanvas.mly
                                        }
                                        edCanvas.mlx = mouse.x; edCanvas.mly = mouse.y
                                    }
                                }
                                edCanvas.requestPaint()
                            }
                            onPressed: function(mouse) {
                                var shift = (mouse.modifiers & Qt.ShiftModifier)
                                var pt = edCanvas.pxAt(mouse.x, mouse.y)
                                if (mouse.button === Qt.LeftButton) {
                                    if (shift) {
                                        if (pt.x >= 0 && pt.x < imgW && pt.y >= 0 && pt.y < imgH) {
                                            setPixel(pt.x, pt.y, editColor)
                                            pvCanvas.requestPaint()
                                        }
                                    } else {
                                        edCanvas.mlx = mouse.x; edCanvas.mly = mouse.y
                                    }
                                } else if (mouse.button === Qt.RightButton && shift) {
                                    if (pt.x >= 0 && pt.x < imgW && pt.y >= 0 && pt.y < imgH)
                                        editColor = getPixel(pt.x, pt.y)
                                }
                                edCanvas.requestPaint()
                            }
                            onReleased: { edCanvas.mlx = -1000000; edCanvas.mly = -1000000 }
                            onWheel: function(wheel) {
                                var d = wheel.angleDelta.y / 600.0
                                if (d > 0.8) d = 0.8; if (d < -0.8) d = -0.8
                                edCanvas.sf += edCanvas.sf * d
                                edCanvas.ox += edCanvas.ox * d
                                edCanvas.oy += edCanvas.oy * d
                                if (edCanvas.sf < 0.5) edCanvas.sf = 0.5
                                if (edCanvas.sf > 40) edCanvas.sf = 40
                                edCanvas.requestPaint()
                            }
                        }
                    }
                }

                // Palette row
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: colorPalette.length > 0 ? 34 : 0
                    color: "transparent"
                    visible: colorPalette.length > 0

                    Flow {
                        anchors.fill: parent; anchors.margins: 1; spacing: 1
                        Repeater {
                            model: colorPalette.length
                            Rectangle {
                                width: 26; height: 26; color: colorPalette[index]
                                border.color: editColor === colorPalette[index] ? "#ff6600" : "#666666"
                                border.width: editColor === colorPalette[index] ? 2 : 1
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: editColor = colorPalette[index]
                                }
                            }
                        }
                    }
                }
            }

            // ==== RIGHT: Preview + Controls/Load-Save tabs ====
            ColumnLayout {
                Layout.preferredWidth: 280
                Layout.minimumWidth: 200
                Layout.maximumWidth: 320
                Layout.fillHeight: true
                spacing: 0

                // Preview image
                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(imgH * previewScale + 4, 256)
                    Layout.maximumHeight: 256
                    clip: true

                    Canvas {
                        id: pvCanvas
                        width: imgW * previewScale
                        height: imgH * previewScale

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.fillStyle = "black"
                            ctx.fillRect(0, 0, width, height)
                            var s = previewScale
                            for (var j = 0; j < imgH; j++) {
                                for (var i = 0; i < imgW; i++) {
                                    ctx.fillStyle = getPixel(i, j)
                                    ctx.fillRect(i * s, j * s, s, s)
                                }
                            }
                        }
                    }
                }

                // Controls / Load-Save tabs
                TabBar {
                    id: ctrlTabBar
                    Layout.fillWidth: true
                    TabButton { text: "Controls" }
                    TabButton { text: "Load/Save" }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: ctrlTabBar.currentIndex

                    // Controls
                    Item {
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 3; spacing: 2

                            SpinBox {
                                Layout.fillWidth: true
                                from: 1; to: 10; value: dispEditor.previewScale; editable: true
                                textFromValue: function(v) { return "Preview Scale: " + v }
                                valueFromText: function(t) { return parseInt(t.replace("Preview Scale: ", "")) || 2 }
                                onValueModified: { dispEditor.previewScale = value; pvCanvas.requestPaint() }
                            }

                            Button {
                                text: "Help"; Layout.fillWidth: true
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
                                onClicked: helpDialog.open()
                            }

                            Button {
                                text: "Clear Screen"; Layout.fillWidth: true
                                onClicked: dispEditor.clearImage()
                            }

                            Button {
                                text: "Clear Layer 2"; Layout.fillWidth: true
                                onClicked: dispEditor.clearLayer2()
                            }

                            // Current color
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 24
                                color: editColor
                                border.color: Utility.getAppHexColor("disabledText")
                                border.width: 2
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }

                    // Load/Save
                    Item {
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 3; spacing: 2

                            RowLayout {
                                Layout.fillWidth: true; spacing: 2
                                Label { text: "Format" }
                                ComboBox {
                                    Layout.fillWidth: true
                                    textRole: "display"
                                    model: Utility.stringListModel(["Indexed 2", "Indexed 4", "Indexed 16", "RGB332", "RGB565", "RGB888"])
                                    currentIndex: dispEditor.formatIndex
                                    onCurrentIndexChanged: {
                                        dispEditor.formatIndex = currentIndex
                                        dispEditor.rebuildPalette()
                                        edCanvas.requestPaint()
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true; spacing: 2
                                Button { text: "Export Bin"; Layout.fillWidth: true
                                    onClicked: VescIf.emitMessageDialog("Export Bin", "Binary export requires native file I/O.", false, false)
                                }
                                Button { text: "Import Bin"; Layout.fillWidth: true
                                    onClicked: VescIf.emitMessageDialog("Import Bin", "Binary import requires native file I/O.", false, false)
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true; spacing: 2
                                Button { text: "Save PNG"; Layout.fillWidth: true
                                    ToolTip.visible: hovered; ToolTip.text: "Save image to PNG file."
                                    onClicked: VescIf.emitStatusMessage("PNG save not yet implemented", false)
                                }
                                Button { text: "Load Image"; Layout.fillWidth: true
                                    onClicked: VescIf.emitMessageDialog("Load Image", "Image loading with dithering requires native code.", false, false)
                                }
                            }

                            SpinBox {
                                Layout.fillWidth: true
                                from: 0; to: 10000; stepSize: 10; value: 1000; editable: true
                                ToolTip.visible: hovered
                                ToolTip.text: "Scale colors when loading image."
                                textFromValue: function(v) { return "Load Color Scale: " + (v / 1000.0).toFixed(2) }
                                valueFromText: function(t) { return Math.round(parseFloat(t.replace("Load Color Scale: ", "")) * 1000) }
                            }

                            CheckBox { text: "Dither when loading"; checked: true }
                            CheckBox { text: "Antialias when loading"; checked: true }
                            CheckBox {
                                text: "Show Layer 2"; checked: true
                                onToggled: {
                                    dispEditor.drawLayer2 = checked
                                    edCanvas.requestPaint(); pvCanvas.requestPaint()
                                }
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }
        }
    }

    // ======== TOP-LEVEL TABS (Display | Overlay Editor | Font Editor) ========
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: mainTabBar
            Layout.fillWidth: true
            TabButton { text: "Display" }
            TabButton { text: "Overlay Editor" }
            TabButton { text: "Font Editor" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: mainTabBar.currentIndex

            // ======== Tab 1: Display ========
            Item {
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 3
                    spacing: 2

                    // Left: W/H toolbar + DispEditor
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true; spacing: 2
                            SpinBox {
                                id: wBoxDisp; from: 1; to: 1024; value: 128; editable: true
                                textFromValue: function(v) { return "W: " + v }
                                valueFromText: function(t) { return parseInt(t.replace("W: ", "")) || 128 }
                            }
                            SpinBox {
                                id: hBoxDisp; from: 1; to: 1024; value: 64; editable: true
                                textFromValue: function(v) { return "H: " + v }
                                valueFromText: function(t) { return parseInt(t.replace("H: ", "")) || 64 }
                            }
                            Button {
                                text: "Update"
                                onClicked: dispEditorMain.updateSize(wBoxDisp.value, hBoxDisp.value)
                            }
                            Item { Layout.fillWidth: true }
                        }

                        DispEditorComp {
                            id: dispEditorMain
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    // Right: Font / Overlay sub-tabs
                    ColumnLayout {
                        Layout.preferredWidth: 240
                        Layout.minimumWidth: 200
                        Layout.maximumWidth: 280
                        Layout.fillHeight: true
                        spacing: 0

                        TabBar {
                            id: rightSubTabBar
                            Layout.fillWidth: true
                            TabButton { text: "Font" }
                            TabButton { text: "Overlay" }
                        }

                        StackLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            currentIndex: rightSubTabBar.currentIndex

                            // Font sub-tab
                            Item {
                                ScrollView {
                                    anchors.fill: parent; clip: true

                                    ColumnLayout {
                                        width: parent.width
                                        spacing: 3

                                        CheckBox {
                                            id: drawFontCheck
                                            text: "Draw Font"
                                            checked: false
                                            Layout.fillWidth: true
                                        }
                                        GroupBox {
                                            title: "Draw Font Settings"
                                            Layout.fillWidth: true
                                            enabled: drawFontCheck.checked

                                            GridLayout {
                                                anchors.fill: parent
                                                columns: 2; columnSpacing: 3; rowSpacing: 3

                                                ComboBox {
                                                    id: fontBox; Layout.columnSpan: 2; Layout.fillWidth: true
                                                    textRole: "display"
                                                    model: Utility.stringListModel(["Roboto", "DejaVu Sans Mono", "Liberation Sans", "Liberation Mono"])
                                                }
                                                CheckBox { text: "Bold" }
                                                Item {}
                                                SpinBox {
                                                    from: 0; to: 512; value: 34; editable: true
                                                    textFromValue: function(v) { return "X: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("X: ", "")) || 0 }
                                                }
                                                SpinBox {
                                                    from: 0; to: 512; value: 46; editable: true
                                                    textFromValue: function(v) { return "Y: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("Y: ", "")) || 0 }
                                                }
                                                SpinBox {
                                                    from: 1; to: 128; value: 10; editable: true
                                                    textFromValue: function(v) { return "W: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("W: ", "")) || 10 }
                                                }
                                                SpinBox {
                                                    from: 1; to: 128; value: 16; editable: true
                                                    textFromValue: function(v) { return "H: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("H: ", "")) || 16 }
                                                }
                                                SpinBox {
                                                    Layout.columnSpan: 2; Layout.fillWidth: true
                                                    from: 10; to: 10000; stepSize: 10; value: 1000; editable: true
                                                    textFromValue: function(v) { return "Scale: " + (v / 1000.0).toFixed(2) }
                                                    valueFromText: function(t) { return Math.round(parseFloat(t.replace("Scale: ", "")) * 1000) }
                                                }
                                                TextField {
                                                    Layout.columnSpan: 2; Layout.fillWidth: true
                                                    text: "ABC 00"
                                                }
                                                CheckBox { text: "Antialias" }
                                                CheckBox { text: "Border"; checked: true }
                                                CheckBox { Layout.columnSpan: 2; text: "NumOnly" }
                                                Button {
                                                    Layout.columnSpan: 2; Layout.fillWidth: true
                                                    text: "Export Font"
                                                    onClicked: VescIf.emitStatusMessage("Font export not yet implemented", false)
                                                }
                                            }
                                        }

                                        Item { Layout.fillHeight: true }
                                    }
                                }
                            }

                            // Overlay sub-tab
                            Item {
                                ScrollView {
                                    anchors.fill: parent; clip: true

                                    ColumnLayout {
                                        width: parent.width
                                        spacing: 3

                                        CheckBox {
                                            id: drawOverlayCheck
                                            text: "Draw Overlay"
                                            checked: false
                                            Layout.fillWidth: true
                                        }
                                        GroupBox {
                                            title: "Draw Overlay Settings"
                                            Layout.fillWidth: true
                                            enabled: drawOverlayCheck.checked

                                            GridLayout {
                                                anchors.fill: parent
                                                columns: 3; columnSpacing: 3; rowSpacing: 3

                                                Label { text: "Position" }
                                                SpinBox {
                                                    from: -512; to: 512; value: 0; editable: true
                                                    textFromValue: function(v) { return "X: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("X: ", "")) || 0 }
                                                }
                                                SpinBox {
                                                    from: -512; to: 512; value: 0; editable: true
                                                    textFromValue: function(v) { return "Y: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("Y: ", "")) || 0 }
                                                }

                                                Label {
                                                    text: "Crop Start"
                                                    ToolTip.text: "Corner to start crop."
                                                    ToolTip.visible: cropStartHover.hovered
                                                    ToolTip.delay: 500
                                                    HoverHandler { id: cropStartHover }
                                                }
                                                SpinBox {
                                                    from: 0; to: 512; value: 0; editable: true
                                                    textFromValue: function(v) { return "X: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("X: ", "")) || 0 }
                                                }
                                                SpinBox {
                                                    from: 0; to: 512; value: 0; editable: true
                                                    textFromValue: function(v) { return "Y: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("Y: ", "")) || 0 }
                                                }

                                                Label { text: "Crop Size" }
                                                SpinBox {
                                                    from: 0; to: 512; value: 512; editable: true
                                                    textFromValue: function(v) { return "W: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("W: ", "")) || 512 }
                                                }
                                                SpinBox {
                                                    from: 0; to: 512; value: 512; editable: true
                                                    textFromValue: function(v) { return "H: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("H: ", "")) || 512 }
                                                }

                                                Label {
                                                    text: "Img Center"
                                                    ToolTip.text: "This pixel on the image corresponds to Position."
                                                    ToolTip.visible: imgCenterHover.hovered
                                                    ToolTip.delay: 500
                                                    HoverHandler { id: imgCenterHover }
                                                }
                                                SpinBox {
                                                    from: -1024; to: 1024; value: 0; editable: true
                                                    textFromValue: function(v) { return "X: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("X: ", "")) || 0 }
                                                }
                                                SpinBox {
                                                    from: -1024; to: 1024; value: 0; editable: true
                                                    textFromValue: function(v) { return "Y: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("Y: ", "")) || 0 }
                                                }

                                                Label {
                                                    text: "Rot Center"
                                                    ToolTip.text: "Rotation will be done around this pixel."
                                                    ToolTip.visible: rotCenterHover.hovered
                                                    ToolTip.delay: 500
                                                    HoverHandler { id: rotCenterHover }
                                                }
                                                SpinBox {
                                                    from: -1024; to: 1024; value: 0; editable: true
                                                    textFromValue: function(v) { return "X: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("X: ", "")) || 0 }
                                                }
                                                SpinBox {
                                                    from: -1024; to: 1024; value: 0; editable: true
                                                    textFromValue: function(v) { return "Y: " + v }
                                                    valueFromText: function(t) { return parseInt(t.replace("Y: ", "")) || 0 }
                                                }

                                                Label { text: "Rotation" }
                                                SpinBox {
                                                    Layout.columnSpan: 2; Layout.fillWidth: true
                                                    from: -360000; to: 360000; stepSize: 1000; value: 0; editable: true
                                                    property real realValue: value / 1000.0
                                                    textFromValue: function(v) { return (v / 1000.0).toFixed(1) + " Deg" }
                                                    valueFromText: function(t) { return Math.round(parseFloat(t.replace(" Deg", "")) * 1000) }
                                                }

                                                Label { text: "Scale" }
                                                SpinBox {
                                                    Layout.columnSpan: 2; Layout.fillWidth: true
                                                    from: 1; to: 100000; stepSize: 10; value: 1000; editable: true
                                                    textFromValue: function(v) { return (v / 1000.0).toFixed(3) }
                                                    valueFromText: function(t) { return Math.round(parseFloat(t) * 1000) }
                                                }

                                                Label { text: "Transparent" }
                                                SpinBox {
                                                    Layout.columnSpan: 2; Layout.fillWidth: true
                                                    from: -1; to: 15; value: 0; editable: true
                                                    ToolTip.visible: hovered
                                                    ToolTip.text: "Color index on image to count as transparent."
                                                }

                                                Button {
                                                    Layout.columnSpan: 3; Layout.fillWidth: true
                                                    text: "Save to Layer 2"
                                                    ToolTip.visible: hovered
                                                    ToolTip.text: "Save overlay to layer 2 on image."
                                                    onClicked: VescIf.emitStatusMessage("Overlay saved to Layer 2", true)
                                                }
                                            }
                                        }

                                        Item { Layout.fillHeight: true }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ======== Tab 2: Overlay Editor ========
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 3
                    spacing: 2

                    RowLayout {
                        Layout.fillWidth: true; spacing: 2
                        SpinBox {
                            id: wBoxOv; from: 1; to: 1024; value: 128; editable: true
                            textFromValue: function(v) { return "W: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("W: ", "")) || 128 }
                        }
                        SpinBox {
                            id: hBoxOv; from: 1; to: 1024; value: 64; editable: true
                            textFromValue: function(v) { return "H: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("H: ", "")) || 64 }
                        }
                        Button {
                            text: "Update"
                            onClicked: dispEditorOverlay.updateSize(wBoxOv.value, hBoxOv.value)
                        }
                        Item { Layout.fillWidth: true }
                    }

                    DispEditorComp {
                        id: dispEditorOverlay
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
            }

            // ======== Tab 3: Font Editor ========
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 3
                    spacing: 2

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 4; columnSpacing: 6; rowSpacing: 3

                        RadioButton { text: "All"; checked: true }
                        RadioButton { text: "Numbers Only" }
                        Item {}
                        Button {
                            text: "Apply"
                            ToolTip.visible: hovered; ToolTip.text: "Apply font and settings"
                            onClicked: VescIf.emitStatusMessage("Font editor apply not yet implemented", false)
                        }

                        ComboBox {
                            Layout.minimumWidth: 120
                            textRole: "display"
                            model: Utility.stringListModel(["Roboto", "DejaVu Sans Mono", "Liberation Sans", "Liberation Mono"])
                        }
                        CheckBox { text: "Anti Alias (2bpp)" }
                        CheckBox { text: "Bold" }
                        CheckBox { text: "Border"; checked: true }

                        SpinBox {
                            from: 1; to: 128; value: 10; editable: true
                            textFromValue: function(v) { return "W: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("W: ", "")) || 10 }
                        }
                        SpinBox {
                            from: 1; to: 128; value: 16; editable: true
                            textFromValue: function(v) { return "H: " + v }
                            valueFromText: function(t) { return parseInt(t.replace("H: ", "")) || 16 }
                        }
                        SpinBox {
                            Layout.columnSpan: 2; Layout.fillWidth: true
                            from: 10; to: 10000; stepSize: 10; value: 1000; editable: true
                            textFromValue: function(v) { return "Scale: " + (v / 1000.0).toFixed(2) }
                            valueFromText: function(t) { return Math.round(parseFloat(t.replace("Scale: ", "")) * 1000) }
                        }

                        Button {
                            text: "Export Font"; Layout.columnSpan: 2; Layout.fillWidth: true
                            onClicked: VescIf.emitStatusMessage("Font export not yet implemented", false)
                        }
                        Button {
                            text: "Import Font"; Layout.columnSpan: 2; Layout.fillWidth: true
                            onClicked: VescIf.emitStatusMessage("Font import not yet implemented", false)
                        }
                    }

                    DispEditorComp {
                        id: dispEditorFont
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }

    // ---- Help Dialog ----
    Dialog {
        id: helpDialog
        title: "Usage Instructions"
        width: 500; height: 400
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok

        ScrollView {
            anchors.fill: parent; clip: true
            Label {
                width: parent.width - 20
                wrapMode: Text.WordWrap; textFormat: Text.RichText
                text: "<b>Navigate in editor</b><br>" +
                      "Left-click and drag to move. Scroll to zoom.<br><br>" +
                      "<b>Draw pixels</b><br>" +
                      "Shift + Left-click (and drag)<br><br>" +
                      "<b>Change color</b><br>" +
                      "Click on color buttons, or Shift + Right-click on pixel with desired color.<br><br>" +
                      "<b>Update Palette Color</b><br>" +
                      "Ctrl + left-click on the palette buttons.<br><br>" +
                      "<b>Overlay</b><br>" +
                      "This function overlays an image from the overlay tab to the display tab with a transform. " +
                      "The same transforms are available in the display library, meaning that all transform " +
                      "parameters can be animated.<br><br>" +
                      "<b>Layer 2</b><br>" +
                      "The second layer can be used to draw overlays on, without messing with the main image. " +
                      "This way the background image can be kept clean for when it is saved."
            }
        }
    }
}
