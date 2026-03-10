/*
    Desktop ConnectionPage — faithful replica of PageConnection (widgets).
    Tabs: "(USB-)Serial", "CAN bus", "TCP", "UDP", "Bluetooth LE", "TCP Hub"
    Below tabs: CAN Forward group, Autoconnect button, status label.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vedder.vesc

Item {
    id: root

    property Commands mCommands: VescIf.commands()

    // ---- Helper: icon-only button (Fixed size, icon only, no text) ----
    component IconButton: Button {
        implicitWidth: implicitHeight
        display: AbstractButton.IconOnly
    }

    // ---- 20ms timer matching original timerSlot ----
    Timer {
        id: pollTimer
        interval: 20
        running: true
        repeat: true
        onTriggered: {
            // Update status label
            var portName = VescIf.getConnectedPortName()
            if (portName !== statusLabel.text) {
                statusLabel.text = portName
            }

            // Sync CAN fwd button with commands state
            if (canFwdButton.checked !== mCommands.getSendCan()) {
                canFwdButton.checked = mCommands.getSendCan()
            }

            // TCP server info
            var tcpIpTxt = "Server IPs\n"
            var tcpClientTxt = "Connected Clients\n"
            if (VescIf.tcpServerIsRunning()) {
                var addrs = Utility.getNetworkAddresses()
                for (var i = 0; i < addrs.length; i++) {
                    tcpIpTxt += addrs[i] + "\n"
                }
                if (VescIf.tcpServerIsClientConnected()) {
                    tcpClientTxt += VescIf.tcpServerClientIp()
                }
            } else {
                tcpServerPortBox.enabled = true
            }
            if (tcpServerAddressesEdit.text !== tcpIpTxt) {
                tcpServerAddressesEdit.text = tcpIpTxt
            }
            if (tcpServerClientsEdit.text !== tcpClientTxt) {
                tcpServerClientsEdit.text = tcpClientTxt
            }

            // UDP server info
            var udpIpTxt = "Server IPs\n"
            var udpClientTxt = "Connected Clients\n"
            if (VescIf.udpServerIsRunning()) {
                var uAddrs = Utility.getNetworkAddresses()
                for (var j = 0; j < uAddrs.length; j++) {
                    udpIpTxt += uAddrs[j] + "\n"
                }
                if (VescIf.udpServerIsClientConnected()) {
                    udpClientTxt += VescIf.udpServerClientIp()
                }
            } else {
                udpServerPortBox.enabled = true
            }
            if (udpServerAddressesEdit.text !== udpIpTxt) {
                udpServerAddressesEdit.text = udpIpTxt
            }
            if (udpServerClientsEdit.text !== udpClientTxt) {
                udpServerClientsEdit.text = udpClientTxt
            }
        }
    }

    // ---- Connections ----
    Connections {
        target: mCommands
        function onPingCanRx(devs, isTimeout) {
            canRefreshButton.enabled = true
            canFwdBox.model = []
            var items = []
            for (var i = 0; i < devs.length; i++) {
                items.push({"text": "VESC " + devs[i], "value": devs[i]})
            }
            canFwdModel.clear()
            for (var k = 0; k < items.length; k++) {
                canFwdModel.append(items[k])
            }
        }
    }

    Connections {
        target: VescIf.bleDevice()
        function onScanDone(devs, done) {
            if (done) {
                bleScanButton.enabled = true
            }

            bleDevBox.model = []
            var vescItems = []
            var otherItems = []

            for (var addr in devs) {
                var devName = devs[addr]
                var setName = VescIf.getBleName(addr)
                var displayName

                if (setName !== "") {
                    displayName = setName + " [" + addr + "]"
                    vescItems.unshift({"text": displayName, "addr": addr})
                } else if (devName.indexOf("VESC") >= 0) {
                    displayName = devName + " [" + addr + "]"
                    vescItems.unshift({"text": displayName, "addr": addr})
                } else {
                    displayName = devName + " [" + addr + "]"
                    otherItems.push({"text": displayName, "addr": addr})
                }
            }

            bleDevModel.clear()
            for (var i = 0; i < vescItems.length; i++) {
                bleDevModel.append(vescItems[i])
            }
            for (var j = 0; j < otherItems.length; j++) {
                bleDevModel.append(otherItems[j])
            }
            bleDevBox.currentIndex = 0
        }
    }

    Connections {
        target: VescIf
        ignoreUnknownSignals: true
        function onCANbusNewNode(node) {
            canbusTargetModel.append({"text": node.toString(), "value": node})
        }
        function onCANbusInterfaceListUpdated() {
            canbusInterfaceBox.model = Utility.stringListModel(VescIf.listCANbusInterfaceNames())
            canbusInterfaceBox.currentIndex = 0
        }
        function onPairingListUpdated() {
            updatePairedList()
        }
    }

    function updatePairedList() {
        pairedModel.clear()
        var uuids = VescIf.getPairedUuids()
        for (var i = 0; i < uuids.length; i++) {
            pairedModel.append({"uuid": uuids[i]})
        }
        if (pairedListView.count > 0) {
            pairedListView.currentIndex = 0
        }
    }

    // ---- Models ----
    ListModel { id: canFwdModel }
    ListModel { id: bleDevModel }
    ListModel { id: canbusTargetModel }
    ListModel { id: pairedModel }

    // ---- Main layout ----
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        TabBar {
            id: connTabBar
            Layout.fillWidth: true

            TabButton { text: "(USB-)Serial"; topPadding: 9; bottomPadding: 9 }
            TabButton { text: "CAN bus"; topPadding: 9; bottomPadding: 9 }
            TabButton { text: "TCP"; topPadding: 9; bottomPadding: 9 }
            TabButton { text: "UDP"; topPadding: 9; bottomPadding: 9 }
            TabButton { text: "Bluetooth LE"; topPadding: 9; bottomPadding: 9 }
            TabButton { text: "TCP Hub"; topPadding: 9; bottomPadding: 9 }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: connTabBar.currentIndex

            // ==========================================
            // Tab 0: (USB-)Serial
            // ==========================================
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true

                        Label { text: "Port" }

                        ComboBox {
                            id: serialPortBox
                            Layout.fillWidth: true
                            textRole: "name"
                            valueRole: "systemPath"
                            model: VescIf.listSerialPorts()
                        }

                        SpinBox {
                            id: serialBaudBox
                            from: 0
                            to: 3000000
                            value: VescIf.getLastSerialBaud()
                            editable: true
                            property string prefix: "Baud: "
                            property string suffix: " bps"
                            textFromValue: function(value, locale) {
                                return prefix + value + suffix
                            }
                            valueFromText: function(text, locale) {
                                return parseInt(text.replace(prefix, "").replace(suffix, ""))
                            }
                        }

                        IconButton {
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Refresh-96.png"
                            ToolTip.visible: hovered
                            ToolTip.text: "Refresh serial port list"
                            onClicked: serialPortBox.model = VescIf.listSerialPorts()
                        }

                        IconButton {
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Disconnected-96.png"
                            ToolTip.visible: hovered
                            ToolTip.text: "Disconnect"
                            onClicked: VescIf.disconnectPort()
                        }

                        IconButton {
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                            ToolTip.visible: hovered
                            ToolTip.text: "Connect"
                            onClicked: {
                                if (serialPortBox.currentValue !== undefined && serialPortBox.currentValue !== "") {
                                    VescIf.connectSerial(serialPortBox.currentValue, serialBaudBox.value)
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // ==========================================
            // Tab 1: CAN bus
            // ==========================================
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true

                        Label { text: "Interface" }

                        ComboBox {
                            id: canbusInterfaceBox
                            textRole: "display"
                            model: Utility.stringListModel(VescIf.listCANbusInterfaceNames())
                        }

                        Label { text: "VESC ID" }

                        ComboBox {
                            id: canbusTargetIdBox
                            Layout.fillWidth: true
                            model: canbusTargetModel
                            textRole: "text"
                            ToolTip.visible: hovered
                            ToolTip.text: "Discovered VESC nodes in the bus"
                        }

                        SpinBox {
                            id: canbusBitrateBox
                            enabled: false
                            from: 0
                            to: 1000
                            value: typeof VescIf.getLastCANbusBitrate === "function" ? VescIf.getLastCANbusBitrate() : 500
                            editable: false
                            property string prefix: "Bit rate: "
                            property string suffix: " kbit/s"
                            textFromValue: function(value, locale) {
                                return prefix + value + suffix
                            }
                            valueFromText: function(text, locale) {
                                return parseInt(text.replace(prefix, "").replace(suffix, ""))
                            }
                            ToolTip.visible: hovered
                            ToolTip.text: "This configuration is not supported by the socketcan backend. " +
                                          "However it is possible to set the rate when configuring the CAN " +
                                          "network interface using the ip link command."
                        }

                        Button {
                            text: "Scan"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Refresh-96.png"
                            ToolTip.visible: hovered
                            ToolTip.text: "Discover VESC nodes in the CAN bus"
                            onClicked: {
                                VescIf.connectCANbus("socketcan",
                                                     canbusInterfaceBox.currentText,
                                                     canbusBitrateBox.value)
                                canbusTargetModel.clear()
                                VescIf.scanCANbus()
                            }
                        }

                        IconButton {
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Disconnected-96.png"
                            ToolTip.visible: hovered
                            ToolTip.text: "Disconnect"
                            onClicked: VescIf.disconnectPort()
                        }

                        IconButton {
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                            ToolTip.visible: hovered
                            ToolTip.text: "Connect to a VESC node"
                            onClicked: {
                                if (canbusTargetIdBox.currentIndex >= 0) {
                                    var node = canbusTargetModel.get(canbusTargetIdBox.currentIndex).value
                                    VescIf.setCANbusReceiverID(node)
                                    VescIf.connectCANbus("socketcan",
                                                         canbusInterfaceBox.currentText,
                                                         canbusBitrateBox.value)
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // ==========================================
            // Tab 2: TCP
            // ==========================================
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    GroupBox {
                        title: "TCP Client"
                        Layout.fillWidth: true

                        GridLayout {
                            anchors.fill: parent
                            columns: 5
                            columnSpacing: 6
                            rowSpacing: 2

                            // Row 0: Address | serverEdit | portSpinBox | disconnect | connect
                            Label { text: "Address" }
                            TextField {
                                id: tcpServerEdit
                                Layout.fillWidth: true
                                text: VescIf.getLastTcpServer()
                            }
                            SpinBox {
                                id: tcpPortBox
                                from: 0; to: 65535
                                value: VescIf.getLastTcpPort()
                                editable: true
                                property string prefix: "Port: "
                                textFromValue: function(v, l) { return prefix + v }
                                valueFromText: function(t, l) { return parseInt(t.replace(prefix, "")) }
                            }
                            IconButton {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Disconnected-96.png"
                                ToolTip.visible: hovered
                                ToolTip.text: "Disconnect"
                                onClicked: VescIf.disconnectPort()
                            }
                            IconButton {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                                ToolTip.visible: hovered
                                ToolTip.text: "Connect"
                                onClicked: VescIf.connectTcp(tcpServerEdit.text, tcpPortBox.value)
                            }

                            // Row 1: Detected Devices | detectBox | detectDisconnect | detectConnect
                            Label { text: "Detected Devices" }
                            ComboBox {
                                id: tcpDetectBox
                                Layout.fillWidth: true
                                Layout.columnSpan: 2
                            }
                            IconButton {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Disconnected-96.png"
                                onClicked: VescIf.disconnectPort()
                            }
                            IconButton {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                                onClicked: {
                                    if (tcpDetectBox.currentIndex >= 0 && tcpDetectData.length > 0) {
                                        var d = tcpDetectData[tcpDetectBox.currentIndex]
                                        if (d) {
                                            VescIf.connectTcp(d.ip, parseInt(d.port))
                                        }
                                    }
                                }
                            }
                        }
                    }

                    GroupBox {
                        title: "TCP Server"
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent

                            RowLayout {
                                CheckBox {
                                    id: tcpServerEnableBox
                                    text: "Enable TCP Server"
                                    onToggled: {
                                        if (checked) {
                                            VescIf.tcpServerStart(tcpServerPortBox.value)
                                            tcpServerPortBox.enabled = false
                                        } else {
                                            VescIf.tcpServerStop()
                                        }
                                    }
                                }
                                SpinBox {
                                    id: tcpServerPortBox
                                    Layout.fillWidth: true
                                    from: 0; to: 65535
                                    value: 65102
                                    editable: true
                                    property string prefix: "TCP Port: "
                                    textFromValue: function(v, l) { return prefix + v }
                                    valueFromText: function(t, l) { return parseInt(t.replace(prefix, "")) }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 80

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    TextArea {
                                        id: tcpServerAddressesEdit
                                        readOnly: true
                                        wrapMode: TextArea.Wrap
                                    }
                                }
                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    TextArea {
                                        id: tcpServerClientsEdit
                                        readOnly: true
                                        wrapMode: TextArea.Wrap
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ==========================================
            // Tab 3: UDP
            // ==========================================
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    GroupBox {
                        title: "UDP Client"
                        Layout.fillWidth: true

                        RowLayout {
                            anchors.fill: parent

                            Label { text: "Address" }
                            TextField {
                                id: udpServerEdit
                                Layout.fillWidth: true
                                text: VescIf.getLastUdpServer()
                            }
                            SpinBox {
                                id: udpPortBox
                                from: 0; to: 65535
                                value: VescIf.getLastUdpPort()
                                editable: true
                                property string prefix: "Port: "
                                textFromValue: function(v, l) { return prefix + v }
                                valueFromText: function(t, l) { return parseInt(t.replace(prefix, "")) }
                            }
                            IconButton {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Disconnected-96.png"
                                ToolTip.visible: hovered
                                ToolTip.text: "Disconnect"
                                onClicked: VescIf.disconnectPort()
                            }
                            IconButton {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                                ToolTip.visible: hovered
                                ToolTip.text: "Connect"
                                onClicked: VescIf.connectUdp(udpServerEdit.text, udpPortBox.value)
                            }
                        }
                    }

                    GroupBox {
                        title: "UDP Server"
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent

                            RowLayout {
                                CheckBox {
                                    id: udpServerEnableBox
                                    text: "Enable UDP Server"
                                    onToggled: {
                                        if (checked) {
                                            VescIf.udpServerStart(udpServerPortBox.value)
                                            udpServerPortBox.enabled = false
                                        } else {
                                            VescIf.udpServerStop()
                                        }
                                    }
                                }
                                SpinBox {
                                    id: udpServerPortBox
                                    Layout.fillWidth: true
                                    from: 0; to: 65535
                                    value: 65102
                                    editable: true
                                    property string prefix: "UDP Port: "
                                    textFromValue: function(v, l) { return prefix + v }
                                    valueFromText: function(t, l) { return parseInt(t.replace(prefix, "")) }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 80

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    TextArea {
                                        id: udpServerAddressesEdit
                                        readOnly: true
                                        wrapMode: TextArea.Wrap
                                    }
                                }
                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    TextArea {
                                        id: udpServerClientsEdit
                                        readOnly: true
                                        wrapMode: TextArea.Wrap
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ==========================================
            // Tab 4: Bluetooth LE
            // ==========================================
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    // BLE Device row
                    RowLayout {
                        Layout.fillWidth: true

                        Label { text: "BLE Device" }

                        ComboBox {
                            id: bleDevBox
                            Layout.fillWidth: true
                            model: bleDevModel
                            textRole: "text"
                        }

                        IconButton {
                            id: bleScanButton
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Refresh-96.png"
                            ToolTip.visible: hovered
                            ToolTip.text: "Refresh serial port list"
                            onClicked: {
                                VescIf.bleDevice().startScan()
                                bleScanButton.enabled = false
                            }
                        }

                        IconButton {
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Disconnected-96.png"
                            ToolTip.visible: hovered
                            ToolTip.text: "Disconnect"
                            onClicked: VescIf.disconnectPort()
                        }

                        IconButton {
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                            ToolTip.visible: hovered
                            ToolTip.text: "Connect"
                            onClicked: {
                                if (bleDevBox.currentIndex >= 0 && bleDevModel.count > 0) {
                                    VescIf.connectBle(bleDevModel.get(bleDevBox.currentIndex).addr)
                                }
                            }
                        }
                    }

                    // Set Device Name row
                    RowLayout {
                        Layout.fillWidth: true

                        Label { text: "Set Device Name" }

                        TextField {
                            id: bleNameEdit
                            Layout.fillWidth: true
                        }

                        Button {
                            text: "Set"
                            icon.source: "qrc" + Utility.getThemePath() + "icons/Ok-96.png"
                            onClicked: {
                                if (bleNameEdit.text !== "" && bleDevBox.currentIndex >= 0 && bleDevModel.count > 0) {
                                    var addr = bleDevModel.get(bleDevBox.currentIndex).addr
                                    VescIf.storeBleName(addr, bleNameEdit.text)
                                    var newName = bleNameEdit.text + " [" + addr + "]"
                                    bleDevModel.set(bleDevBox.currentIndex, {"text": newName, "addr": addr})
                                }
                            }
                        }
                    }

                    // VESC Devices group
                    GroupBox {
                        title: "VESC Devices"
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        RowLayout {
                            anchors.fill: parent

                            ListView {
                                id: pairedListView
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: pairedModel
                                currentIndex: 0
                                highlight: Rectangle {
                                    color: Utility.getAppHexColor("lightAccent")
                                    opacity: 0.3
                                }

                                delegate: ItemDelegate {
                                    width: pairedListView.width
                                    text: "UUID: " + uuid
                                    highlighted: pairedListView.currentIndex === index
                                    onClicked: pairedListView.currentIndex = index
                                }

                                ScrollBar.vertical: ScrollBar {}
                            }

                            ColumnLayout {
                                spacing: 3

                                Button {
                                    text: "Pair"
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/Circled Play-96.png"
                                    Layout.fillWidth: true
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Pair the connected VESC"
                                    onClicked: {
                                        if (!VescIf.isPortConnected()) {
                                            VescIf.emitMessageDialog("Pair VESC",
                                                "You are not connected to the VESC. Connect in order to pair it.",
                                                false, false)
                                            return
                                        }
                                        if (mCommands.isLimitedMode()) {
                                            VescIf.emitMessageDialog("Pair VESC",
                                                "The fiwmare must be updated to pair this VESC.",
                                                false, false)
                                            return
                                        }
                                        pairConfirmDialog.open()
                                    }
                                }
                                Button {
                                    text: "Add"
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/Plus Math-96.png"
                                    Layout.fillWidth: true
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Add the connected VESC without pairing it."
                                    onClicked: {
                                        if (VescIf.isPortConnected()) {
                                            VescIf.addPairedUuid(VescIf.getConnectedUuid())
                                            VescIf.storeSettings()
                                        } else {
                                            VescIf.emitMessageDialog("Add UUID",
                                                "You are not connected to the VESC. Connect in order to add it.",
                                                false, false)
                                        }
                                    }
                                }
                                Button {
                                    text: "Add UUID"
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/Plus Math-96.png"
                                    Layout.fillWidth: true
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Manually add UUID to this instance of VESC Tool"
                                    onClicked: addUuidDialog.open()
                                }
                                Button {
                                    text: "Unpair"
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/Restart-96.png"
                                    Layout.fillWidth: true
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Unpair this VESC, and make it possible to connect to it from any VESC Tool instance."
                                    onClicked: {
                                        if (!VescIf.isPortConnected()) {
                                            VescIf.emitMessageDialog("Unpair VESC",
                                                "You are not connected to the VESC. Connect in order to unpair it.",
                                                false, false)
                                            return
                                        }
                                        if (mCommands.isLimitedMode()) {
                                            VescIf.emitMessageDialog("Unpair VESC",
                                                "The fiwmare must be updated on this VESC first.",
                                                false, false)
                                            return
                                        }
                                        if (pairedListView.currentIndex >= 0) {
                                            unpairConfirmDialog.open()
                                        }
                                    }
                                }
                                Button {
                                    text: "Delete"
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/Delete-96.png"
                                    Layout.fillWidth: true
                                    onClicked: {
                                        if (pairedListView.currentIndex >= 0) {
                                            deleteConfirmDialog.open()
                                        }
                                    }
                                }
                                Button {
                                    text: "Clear"
                                    icon.source: "qrc" + Utility.getThemePath() + "icons/Delete-96.png"
                                    Layout.fillWidth: true
                                    onClicked: {
                                        if (pairedModel.count > 0) {
                                            clearConfirmDialog.open()
                                        }
                                    }
                                }
                                Item { Layout.fillHeight: true }
                            }
                        }
                    }
                }
            }

            // ==========================================
            // Tab 5: TCP Hub
            // ==========================================
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    GroupBox {
                        title: "Login Details"
                        Layout.fillWidth: true

                        GridLayout {
                            anchors.fill: parent
                            columns: 6
                            columnSpacing: 6
                            rowSpacing: 2

                            // Row 0: Address | serverEdit | defaultBtn | portSpinBox | disconnect | connect
                            Label { text: "Address" }
                            TextField {
                                id: tcpHubServerEdit
                                Layout.fillWidth: true
                                text: VescIf.getLastTcpHubServer()
                            }
                            IconButton {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Restart-96.png"
                                ToolTip.visible: hovered
                                ToolTip.text: "Restore default"
                                onClicked: {
                                    tcpHubServerEdit.text = "veschub.vedder.se"
                                    tcpHubPortBox.value = 65101
                                }
                            }
                            SpinBox {
                                id: tcpHubPortBox
                                from: 0; to: 65535
                                value: VescIf.getLastTcpHubPort()
                                editable: true
                                property string prefix: "Port: "
                                textFromValue: function(v, l) { return prefix + v }
                                valueFromText: function(t, l) { return parseInt(t.replace(prefix, "")) }
                            }
                            IconButton {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Disconnected-96.png"
                                ToolTip.visible: hovered
                                ToolTip.text: "Disconnect"
                                onClicked: {
                                    if (hubClientButton.checked) {
                                        VescIf.disconnectPort()
                                    } else {
                                        VescIf.tcpServerStop()
                                    }
                                }
                            }
                            IconButton {
                                icon.source: "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                                ToolTip.visible: hovered
                                ToolTip.text: "Connect"
                                onClicked: {
                                    var server = tcpHubServerEdit.text
                                    var port = tcpHubPortBox.value
                                    var vescId = tcpHubVescIdEdit.text.replace(/ /g, "").replace(/:/g, "").toUpperCase()
                                    var pass = tcpHubPasswordEdit.text

                                    if (hubClientButton.checked) {
                                        VescIf.connectTcpHub(server, port, vescId, pass)
                                    } else {
                                        VescIf.tcpServerConnectToHub(server, port, vescId, pass)
                                    }
                                }
                            }

                            // Row 1: VESC ID | idEdit(span 2) | Client radio(span 3)
                            Label { text: "VESC ID" }
                            TextField {
                                id: tcpHubVescIdEdit
                                Layout.fillWidth: true
                                Layout.columnSpan: 2
                                text: VescIf.getLastTcpHubVescID()
                            }
                            RadioButton {
                                id: hubClientButton
                                text: "Client"
                                checked: true
                                Layout.columnSpan: 3
                            }

                            // Row 2: Password | passEdit(span 2) | Server radio(span 3)
                            Label { text: "Password" }
                            TextField {
                                id: tcpHubPasswordEdit
                                Layout.fillWidth: true
                                Layout.columnSpan: 2
                                text: VescIf.getLastTcpHubVescPass()
                            }
                            RadioButton {
                                id: hubServerButton
                                text: "Server"
                                Layout.columnSpan: 3
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // ==========================================
        // CAN Forward (always visible below tabs)
        // ==========================================
        GroupBox {
            title: "CAN Forward"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent

                Button {
                    text: "Manual"
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Bug-96.png"
                    ToolTip.visible: hovered
                    ToolTip.text: "Populate box without scanning, in case there are problems with scanning or the firmware does not support it."
                    onClicked: {
                        canFwdModel.clear()
                        for (var i = 0; i < 255; i++) {
                            canFwdModel.append({"text": "VESC " + i, "value": i})
                        }
                    }
                }

                Button {
                    id: canRefreshButton
                    text: "Scan"
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Refresh-96.png"
                    ToolTip.visible: hovered
                    ToolTip.text: "Scan for CAN devices"
                    onClicked: {
                        canRefreshButton.enabled = false
                        mCommands.pingCan()
                    }
                }

                ComboBox {
                    id: canFwdBox
                    Layout.fillWidth: true
                    model: canFwdModel
                    textRole: "text"
                    onActivated: {
                        if (canFwdModel.count > 0) {
                            mCommands.setCanSendId(canFwdModel.get(currentIndex).value)
                        }
                    }
                }

                IconButton {
                    icon.source: "qrc" + Utility.getThemePath() + "icons/Help-96.png"
                    ToolTip.visible: hovered
                    ToolTip.text: "Show help"
                    onClicked: {
                        VescIf.emitMessageDialog("CAN Forward",
                            "CAN forward allows you to communicate with other VESCs connected over CAN-bus. " +
                            "Click scan to look for VESCs on the CAN-bus. If scanning does not work, " +
                            "manually add all IDs and select the one to communicate with.",
                            true, false)
                    }
                }

                IconButton {
                    id: canFwdButton
                    checkable: true
                    icon.source: checked
                        ? ("qrc" + Utility.getThemePath() + "icons/can_on.png")
                        : ("qrc" + Utility.getThemePath() + "icons/can_off.png")
                    ToolTip.visible: hovered
                    ToolTip.text: "Forward communication over CAN-bus"
                    onClicked: {
                        if (mCommands.getCanSendId() >= 0 || !checked) {
                            mCommands.setSendCan(checked)
                        } else {
                            checked = false
                            VescIf.emitMessageDialog("CAN Forward",
                                "No CAN device is selected. Click on the refresh button " +
                                "if the selection box is empty.",
                                false, false)
                        }
                    }
                }
            }
        }

        // ==========================================
        // Autoconnect button
        // ==========================================
        Button {
            Layout.fillWidth: true
            text: "Autoconnect"
            icon.source: "qrc" + Utility.getThemePath() + "icons/Wizard-96.png"
            icon.width: 45
            icon.height: 45
            ToolTip.visible: hovered
            ToolTip.text: "Try to automatically connect using the USB connection"
            onClicked: VescIf.autoconnect()
        }

        // ==========================================
        // Status label
        // ==========================================
        Label {
            id: statusLabel
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: ""
        }
    }

    // ---- TCP detect data storage ----
    property var tcpDetectData: []

    // ---- Confirmation dialogs ----
    Dialog {
        id: pairConfirmDialog
        title: "Pair connected VESC"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        Label {
            text: "This is going to pair the connected VESC with this instance of VESC Tool. " +
                  "VESC Tool instances that are not paired with this VESC will not be able to " +
                  "connect over bluetooth any more. Continue?"
            wrapMode: Text.WordWrap
            width: parent.width
        }
        onAccepted: {
            VescIf.addPairedUuid(VescIf.getConnectedUuid())
            VescIf.storeSettings()
            // Set pairing_done flag on VESC
            var ap = VescIf.appConfig()
            mCommands.getAppConf()
            if (Utility.waitSignal(ap, "updated", 1500)) {
                ap.updateParamBool("pairing_done", true, null)
                mCommands.setAppConf()
            }
        }
    }

    Dialog {
        id: unpairConfirmDialog
        title: "Unpair connected VESC"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        Label {
            text: "This is going to unpair the connected VESC. Continue?"
            wrapMode: Text.WordWrap
            width: parent.width
        }
        onAccepted: {
            var ap = VescIf.appConfig()
            mCommands.getAppConf()
            if (Utility.waitSignal(ap, "updated", 1500)) {
                ap.updateParamBool("pairing_done", false, null)
                mCommands.setAppConf()
                VescIf.deletePairedUuid(VescIf.getConnectedUuid())
                VescIf.storeSettings()
            }
        }
    }

    Dialog {
        id: deleteConfirmDialog
        title: "Delete paired VESC"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        Label {
            text: "This is going to delete this VESC from the paired list. If that VESC " +
                  "has the pairing flag set you won't be able to connect to it over BLE " +
                  "any more. Are you sure?"
            wrapMode: Text.WordWrap
            width: parent.width
        }
        onAccepted: {
            if (pairedListView.currentIndex >= 0) {
                var uuid = pairedModel.get(pairedListView.currentIndex).uuid
                VescIf.deletePairedUuid(uuid)
                VescIf.storeSettings()
            }
        }
    }

    Dialog {
        id: clearConfirmDialog
        title: "Clear paired VESCs"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        Label {
            text: "This is going to clear the pairing list of this instance of VESC Tool. Are you sure?"
            wrapMode: Text.WordWrap
            width: parent.width
        }
        onAccepted: {
            VescIf.clearPairedUuids()
            VescIf.storeSettings()
        }
    }

    Dialog {
        id: addUuidDialog
        title: "Add UUID"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        ColumnLayout {
            anchors.fill: parent
            Label { text: "UUID:" }
            TextField {
                id: addUuidEdit
                Layout.fillWidth: true
            }
        }
        onAccepted: {
            if (addUuidEdit.text !== "") {
                VescIf.addPairedUuid(addUuidEdit.text)
                VescIf.storeSettings()
            }
        }
    }

    // ---- Init ----
    Component.onCompleted: {
        // Populate initial BLE device if last known
        var lastBleAddr = VescIf.getLastBleAddr()
        if (lastBleAddr !== "") {
            var setName = VescIf.getBleName(lastBleAddr)
            var name
            if (setName !== "") {
                name = setName + " [" + lastBleAddr + "]"
            } else {
                name = lastBleAddr
            }
            bleDevModel.append({"text": name, "addr": lastBleAddr})
        }

        updatePairedList()
    }
}
