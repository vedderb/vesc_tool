/*
    Desktop AppNrfPage — exact parity with PageAppNrf widget.
    Layout: VBoxLayout
      ParamTable for "nrf" / "general"
      NRF Pairing GroupBox at bottom
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    id: root

    property ConfigParams mAppConf: VescIf.appConfig()
    property Commands mCommands: VescIf.commands()
    property var _dynamicItems: []

    // NRF pairing state
    property double _pairCnt: 0.0
    property bool _pairRunning: false

    ParamEditors { id: editors }

    Connections {
        target: mCommands
        function onNrfPairingRes(res) {
            if (!_pairRunning) return
            if (res === 0) {
                _pairCnt = nrfTimeBox.realValue
                nrfStartBtn.enabled = false
            } else if (res === 1) {
                nrfStartBtn.enabled = true
                _pairCnt = 0
                VescIf.emitStatusMessage("Pairing NRF Successful", true)
                _pairRunning = false
            } else if (res === 2) {
                nrfStartBtn.enabled = true
                _pairCnt = 0
                VescIf.emitStatusMessage("Pairing NRF Timed Out", false)
                _pairRunning = false
            }
        }
    }

    Timer {
        interval: 100; running: true; repeat: true
        onTriggered: {
            if (_pairCnt > 0.01) {
                _pairCnt -= 0.1
                if (_pairCnt <= 0.01) {
                    nrfStartBtn.enabled = true
                    _pairCnt = 0
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true; contentWidth: width; contentHeight: genCol.height + 16
            flickableDirection: Flickable.VerticalFlick
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            ColumnLayout {
                id: genCol
                width: parent.width
                spacing: 4
                Item { Layout.preferredHeight: 1 }
            }
        }

        // NRF Pairing GroupBox (at bottom, matching PageAppNrf layout)
        GroupBox {
            title: "NRF Pairing"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                spacing: 4

                Button {
                    id: nrfStartBtn
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                    ToolTip.text: "Start Pairing"; ToolTip.visible: hovered
                    onClicked: {
                        mCommands.pairNrf(Math.round(nrfTimeBox.realValue * 1000))
                        _pairRunning = true
                    }
                }

                SpinBox {
                    id: nrfTimeBox
                    from: 10; to: 1000; value: 100; stepSize: 10
                    editable: true
                    property double realValue: value / 10.0
                    textFromValue: function(v) { return "Time: " + (v / 10.0).toFixed(1) + " s" }
                    valueFromText: function(t) { return Math.round(parseFloat(t.replace("Time: ", "").replace(" s", "")) * 10) || 100 }
                    ToolTip.text: "Stay in pairing mode for this amount of time"
                    ToolTip.visible: nrfTimeHover.hovered; ToolTip.delay: 500
                    HoverHandler { id: nrfTimeHover }
                }

                Label {
                    text: _pairCnt.toFixed(1)
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    Layout.preferredWidth: 40
                    background: Rectangle { border.width: 1; border.color: palette.mid; radius: 2; color: "transparent" }
                    padding: 4
                }
            }
        }
    }

    function loadSubgroup(parentCol, subgroup) {
        var params = mAppConf.getParamsFromSubgroup("nrf", subgroup)
        for (var p = 0; p < params.length; p++) {
            var paramName = params[p]
            if (paramName.indexOf("::sep::") === 0) {
                var sep = editors.createSeparator(parentCol, paramName.substring(7))
                if (sep) _dynamicItems.push(sep)
                continue
            }
            var e = editors.createEditorApp(parentCol, paramName)
            if (e) { e.Layout.fillWidth = true; _dynamicItems.push(e) }
        }
    }

    Component.onCompleted: {
        loadSubgroup(genCol, "general")
    }
}
