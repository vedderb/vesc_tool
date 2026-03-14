/*
    Desktop CanAnalyzerPage — faithful recreation of the original PageCanAnalyzer widget.
    Features: CAN baudrate update, CAN mode/baudrate params, message table,
    send frame controls (ID + 8 data bytes), clear/auto-scroll.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    id: canAnalyzerPage

    property Commands mCommands: VescIf.commands()
    property ConfigParams mAppConf: VescIf.appConfig()
    property var _dynamicItems: []

    ParamEditors {
        id: editors
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // Update CAN Baudrate group
        GroupBox {
            title: "Update CAN Baudrate"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                spacing: 8

                Label { text: "KBits/sec" }
                ComboBox {
                    id: canBaudBox
                    textRole: "display"
                    model: Utility.stringListModel(["125", "250", "500", "1000", "10", "20", "50", "75", "100"])
                    currentIndex: 2
                }
                Button {
                    text: "Start Update"
                    onClicked: {
                        VescIf.emitMessageDialog("Update CAN Baudrate",
                            "This is going to update the CAN-bus baudrate on all connected " +
                            "VESC-based devices. This is only supported in firmware 6.06 or " +
                            "later. If the update fails the CAN-bus might become unusable until " +
                            "it is reconfigured manually. Do you want to continue?",
                            false, false)
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        // CAN mode/baudrate params
        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: paramCol.height + 8
            Layout.maximumHeight: 120
            clip: true; contentWidth: width; contentHeight: paramCol.height + 8
            flickableDirection: Flickable.VerticalFlick
            ColumnLayout {
                id: paramCol; width: parent.width; spacing: 4
                Item { Layout.preferredHeight: 1 }
            }
        }

        // Send CAN Frame group
        GroupBox {
            title: "Send CAN Frame"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                spacing: 4

                Label { text: "ID:" }
                TextField {
                    id: sendIdEdit
                    Layout.preferredWidth: 100
                    text: "0x00000000"
                    placeholderText: "0x or decimal"
                }

                ComboBox {
                    id: sendExtBox
                    textRole: "display"
                    model: Utility.stringListModel(["Standard", "Extended"])
                    currentIndex: 0
                }

                SpinBox { id: d0Box; from: -1; to: 255; value: 0; editable: true; Layout.preferredWidth: 70
                    textFromValue: function(v) { return v < 0 ? "--" : "0x" + v.toString(16).toUpperCase().padStart(2, "0") }
                    valueFromText: function(t) { if (t === "--") return -1; return parseInt(t.replace(/0[xX]/, ""), 16) } }
                SpinBox { id: d1Box; from: -1; to: 255; value: -1; editable: true; Layout.preferredWidth: 70
                    textFromValue: function(v) { return v < 0 ? "--" : "0x" + v.toString(16).toUpperCase().padStart(2, "0") }
                    valueFromText: function(t) { if (t === "--") return -1; return parseInt(t.replace(/0[xX]/, ""), 16) } }
                SpinBox { id: d2Box; from: -1; to: 255; value: -1; editable: true; Layout.preferredWidth: 70
                    textFromValue: function(v) { return v < 0 ? "--" : "0x" + v.toString(16).toUpperCase().padStart(2, "0") }
                    valueFromText: function(t) { if (t === "--") return -1; return parseInt(t.replace(/0[xX]/, ""), 16) } }
                SpinBox { id: d3Box; from: -1; to: 255; value: -1; editable: true; Layout.preferredWidth: 70
                    textFromValue: function(v) { return v < 0 ? "--" : "0x" + v.toString(16).toUpperCase().padStart(2, "0") }
                    valueFromText: function(t) { if (t === "--") return -1; return parseInt(t.replace(/0[xX]/, ""), 16) } }
                SpinBox { id: d4Box; from: -1; to: 255; value: -1; editable: true; Layout.preferredWidth: 70
                    textFromValue: function(v) { return v < 0 ? "--" : "0x" + v.toString(16).toUpperCase().padStart(2, "0") }
                    valueFromText: function(t) { if (t === "--") return -1; return parseInt(t.replace(/0[xX]/, ""), 16) } }
                SpinBox { id: d5Box; from: -1; to: 255; value: -1; editable: true; Layout.preferredWidth: 70
                    textFromValue: function(v) { return v < 0 ? "--" : "0x" + v.toString(16).toUpperCase().padStart(2, "0") }
                    valueFromText: function(t) { if (t === "--") return -1; return parseInt(t.replace(/0[xX]/, ""), 16) } }
                SpinBox { id: d6Box; from: -1; to: 255; value: -1; editable: true; Layout.preferredWidth: 70
                    textFromValue: function(v) { return v < 0 ? "--" : "0x" + v.toString(16).toUpperCase().padStart(2, "0") }
                    valueFromText: function(t) { if (t === "--") return -1; return parseInt(t.replace(/0[xX]/, ""), 16) } }
                SpinBox { id: d7Box; from: -1; to: 255; value: -1; editable: true; Layout.preferredWidth: 70
                    textFromValue: function(v) { return v < 0 ? "--" : "0x" + v.toString(16).toUpperCase().padStart(2, "0") }
                    valueFromText: function(t) { if (t === "--") return -1; return parseInt(t.replace(/0[xX]/, ""), 16) } }

                Button {
                    text: "Send"
                    onClicked: {
                        var vals = [d0Box.value, d1Box.value, d2Box.value, d3Box.value,
                                    d4Box.value, d5Box.value, d6Box.value, d7Box.value]
                        var bytes = []
                        for (var i = 0; i < 8; i++) {
                            if (vals[i] >= 0) bytes.push(vals[i])
                            else break
                        }

                        var idTxt = sendIdEdit.text.toLowerCase().replace(" ", "")
                        var id = 0
                        var ok = false
                        if (idTxt.startsWith("0x")) {
                            id = parseInt(idTxt.substring(2), 16)
                            ok = !isNaN(id)
                        } else {
                            id = parseInt(idTxt, 10)
                            ok = !isNaN(id)
                        }

                        if (ok) {
                            mCommands.forwardCanFrame(bytes, id, sendExtBox.currentIndex === 1)
                        } else {
                            VescIf.emitMessageDialog("Send CAN",
                                "Unable to parse ID. ID must be a decimal number, or " +
                                "a hexadecimal number starting with 0x", false, false)
                        }
                    }
                }
            }
        }

        // Message Table controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            CheckBox {
                id: autoScrollBox
                text: "Auto-scroll"
                checked: true
            }
            Button {
                text: "Clear"
                onClicked: messageModel.clear()
            }
            Item { Layout.fillWidth: true }
            Label {
                text: messageModel.count + " messages"
                color: Utility.getAppHexColor("disabledText")
            }
        }

        // Message list
        ListView {
            id: msgListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: ListModel { id: messageModel }
            headerPositioning: ListView.OverlayHeader

            header: Rectangle {
                width: msgListView.width
                height: 28
                z: 2
                color: Utility.getAppHexColor("normalBackground")
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    spacing: 0
                    Label { text: "Ext"; Layout.preferredWidth: 50; font.bold: true; font.pointSize: 11 }
                    Label { text: "ID"; Layout.preferredWidth: 110; font.bold: true; font.pointSize: 11 }
                    Label { text: "Len"; Layout.preferredWidth: 40; font.bold: true; font.pointSize: 11 }
                    Label { text: "Data"; Layout.fillWidth: true; font.bold: true; font.pointSize: 11 }
                }
            }

            delegate: Rectangle {
                width: msgListView.width
                height: 24
                color: index % 2 === 0 ? "transparent" : Utility.getAppHexColor("normalBackground")
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    spacing: 0
                    Label { text: model.ext; Layout.preferredWidth: 50; font.pointSize: 11 }
                    Label { text: model.canId; Layout.preferredWidth: 110; font.pointSize: 11; font.family: "monospace" }
                    Label { text: model.len; Layout.preferredWidth: 40; font.pointSize: 11 }
                    Label { text: model.data; Layout.fillWidth: true; font.pointSize: 11; font.family: "monospace" }
                }
            }

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }
    }

    Connections {
        target: mCommands
        function onCanFrameRx(data, id, isExtended) {
            if (messageModel.count > 3999) {
                messageModel.remove(0)
            }
            // data is a QByteArray exposed as ArrayBuffer in QML
            var view = new Uint8Array(data)
            var hexBytes = []
            for (var i = 0; i < view.length; i++) {
                hexBytes.push("0x" + view[i].toString(16).toUpperCase().padStart(2, "0"))
            }
            messageModel.append({
                ext: isExtended ? "Yes" : "No",
                canId: "0x" + id.toString(16).toUpperCase().padStart(8, "0"),
                len: view.length.toString(),
                data: hexBytes.join(" ")
            })
            if (autoScrollBox.checked) {
                msgListView.positionViewAtEnd()
            }
        }
    }

    function reloadParams() {
        for (var d = 0; d < _dynamicItems.length; d++) {
            if (_dynamicItems[d]) _dynamicItems[d].destroy()
        }
        _dynamicItems = []

        var paramNames = ["can_mode", "can_baud_rate"]
        for (var p = 0; p < paramNames.length; p++) {
            var e = editors.createEditorApp(paramCol, paramNames[p])
            if (e) { e.Layout.fillWidth = true; _dynamicItems.push(e) }
        }
    }

    Component.onCompleted: reloadParams()

    Connections {
        target: mAppConf
        function onUpdated() { reloadParams() }
    }
}
