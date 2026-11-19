/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import Vedder.vesc.utility 1.0

Item {
    property var dialogParent: Overlay.overlay
    property ConfigParams mAppConf: VescIf.appConfig()
    property Commands mCommands: VescIf.commands()
    property bool mSendCanAtStart: false
    property int mCanIdAtStart: 0
    property bool isHorizontal: width > height

    // Scrolls only when the content is taller than the space, as beside the guidance on landscape phones
    component FitColumn: Flickable {
        id: fitFlick
        default property alias content: fitLayout.data

        implicitHeight: fitLayout.implicitHeight
        contentWidth: width
        contentHeight: fitLayout.implicitHeight
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds
        clip: interactive

        ScrollIndicator.vertical: ScrollIndicator {}

        ColumnLayout {
            id: fitLayout
            width: fitFlick.width
            spacing: 12
        }
    }

    function openDialog() {
        workaroundTimerOpen.start()
    }
    Timer {
        id: workaroundTimerOpen
        interval: 0
        repeat: false
        running: false
        onTriggered: {
            canIdModel.clear()
            stackLayout.currentIndex = 0
            updateButtonText()

            mSendCanAtStart = mCommands.getSendCan()
            mCanIdAtStart = mCommands.getCanSendId()

            if (mCommands.getSendCan()) {
                canIdModel.append({"name": "This VESC (CAN FWD)",
                                      "canId": mCanIdAtStart,
                                      "isCan": true})
                dialog.open()
                canScanBar.value = 100
            } else {
                canIdModel.append({"name": "This VESC",
                                      "canId": mAppConf.getParamInt("controller_id"),
                                      "isCan": false})
                dialog.open()
                disableDialog()
                var canDevs = Utility.scanCanVescOnly(VescIf)

                for (var i = 0;i < canDevs.length;i++) {
                    canIdModel.append({"name": "VESC on CAN-bus",
                                          "canId": canDevs[i],
                                          "isCan": true})
                }
                enableDialog()
            }
        }
    }

    function disableDialog() {
        nextButton.enabled = false
        prevButton.enabled = false
        stackLayout.enabled = false

        if (canScanBar.visible) {
            canScanBar.indeterminate = true
        } else {
            commDialog.open()
        }
    }

    function enableDialog() {
        canScanBar.indeterminate = false
        canScanBar.value = 100
        nextButton.enabled = true
        prevButton.enabled = true
        stackLayout.enabled = true
        commDialog.close()
    }

    BusyDialog {
        id: commDialog
        title: "Communicating..."
    }

    Dialog {
        id: dialog
        modal: true
        focus: true
        // Covers the app bars on landscape phones
        readonly property bool fullWindow: phoneLandscape
        width: parent.width - 10 - notchLeft - notchRight
        height: parent.height - 10 - (fullWindow ? notchTop + notchBot : 0)
        x: 5 + notchLeft
        y: 5 + (fullWindow ? notchTop : 0)
        closePolicy: Popup.NoAutoClose
        parent: fullWindow ? Overlay.overlay : dialogParent
        bottomMargin: 0
        rightMargin: 0
        padding: 10

        background: Rectangle {
            color: Utility.getAppHexColor("darkBackground")
            radius: 20
        }

        Overlay.modal: Rectangle {
            color: Utility.getAppHexColor("overlay")
        }

        StackLayout {
            id: stackLayout
            anchors.fill: parent
            anchors.rightMargin: sideMaps.visible ? sideMaps.width + 10 : 0

            Item {
                // Guidance beside the list on landscape phones
                GridLayout {
                    id: connGrid
                    anchors.fill: parent
                    columns: phoneLandscape ? 2 : 1
                    uniformCellWidths: true
                    rowSpacing: 12
                    columnSpacing: 12

                    StepHint {
                        id: connTitle
                        Layout.rowSpan: connGrid.columns
                        Layout.alignment: Qt.AlignTop
                        title: qsTr("Which VESC is the input connected to?")
                        hint: qsTr("Pick the VESC the receiver or throttle is plugged into.")
                    }

                    ListModel {
                        id: canIdModel
                    }

                    ListView {
                        id: canIdList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        focus: true
                        clip: true
                        spacing: 5

                        Component {
                            id: canIdDelegate

                            Rectangle {
                                property variant modelData: model

                                width: canIdList.width
                                height: 90
                                color: Utility.getAppHexColor("cardBackground")
                                border.width: ListView.isCurrentItem ? 2 : 1
                                border.color: ListView.isCurrentItem ? Material.accent : Utility.getAppHexColor("raisedBackground")
                                radius: 12
                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10

                                    // Centred on the same column as the input type pictures
                                    Image {
                                        id: image
                                        fillMode: Image.PreserveAspectFit
                                        Layout.preferredWidth: 48
                                        Layout.preferredHeight: 48
                                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                        Layout.leftMargin: 21
                                        Layout.rightMargin: 16
                                        source: isCan ? "qrc" + Utility.getThemePath() + "icons/can_off.png" : "qrc" + Utility.getThemePath() + "icons/Connected-96.png"
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.rightMargin: 10
                                        spacing: 2

                                        Text {
                                            Layout.fillWidth: true
                                            text: name
                                            color: Utility.getAppHexColor("lightText")
                                            font.weight: Font.Medium
                                            wrapMode: Text.WordWrap
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: qsTr("ID %1").arg(canId)
                                            color: Utility.getAppHexColor("normalText")
                                            font.pointSize: 11
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onPressed: {
                                        canIdList.currentIndex = index
                                        canIdList.focus = true
                                    }
                                }
                            }
                        }

                        model: canIdModel
                        delegate: canIdDelegate
                    }

                    ProgressBar {
                        id: canScanBar
                        Layout.fillWidth: true
                        visible: value > 0 && value < 100
                        from: 0
                        to: 100
                        value: 0
                    }

                    // Under the guidance on landscape phones
                    WarningBanner {
                        parent: phoneLandscape ? connTitle : connGrid
                        Layout.fillWidth: true
                        Layout.topMargin: phoneLandscape ? 10 : 0
                        text: qsTr("Next resets the app configuration of all VESCs on the CAN-bus to defaults, keeping their CAN IDs.")
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    StepHint {
                        title: qsTr("Select the Input Type")
                        hint: qsTr("Pick how the remote or throttle connects to the VESC.")
                    }

                    ListModel {
                        id: inputModel

                        ListElement {
                            name: "PPM input"
                            desc: "Conventional RC receivers"
                            warning: ""
                            img: "qrc:/res/images/rc_rx.jpg"
                            apptype: 4 // PPM_UART
                        }

                        ListElement {
                            name: "NRF controller"
                            desc: "Remotes paired over NRF"
                            warning: ""
                            img: "qrc:/res/images/vedder_nunchuk.jpg"
                            apptype: 3 // UART, assume permanent NRF or NRF51 on UART
                        }

                        ListElement {
                            name: "ADC input"
                            desc: "Conventional e-bike throttles"
                            warning: ""
                            img: "qrc:/res/images/ebike_throttle.jpg"
                            apptype: 5 // ADC
                        }

                        ListElement {
                            name: "Nyko Kama nunchuk"
                            desc: "Wireless nunchuk"
                            warning: "Will disable UART"
                            img: "qrc:/res/images/nunchuk.jpg"
                            apptype: 6 // NUNCHUK
                        }
                    }

                    // All four cards in two rows on landscape phones
                    GridView {
                        id: inputList
                        property int cols: phoneLandscape ? 2 : 1
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        focus: true
                        clip: true
                        cellWidth: width / cols
                        cellHeight: phoneLandscape ? height / 2 : 95

                        Component {
                            id: inputDelegate

                            Rectangle {
                                id: imgRect
                                property variant modelData: model

                                width: inputList.cellWidth - (inputList.cols > 1 ? 5 : 0)
                                height: inputList.cellHeight - 5
                                color: Utility.getAppHexColor("cardBackground")
                                border.width: GridView.isCurrentItem ? 2 : 1
                                border.color: GridView.isCurrentItem ? Material.accent : Utility.getAppHexColor("raisedBackground")
                                radius: 12
                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10

                                    Item {
                                        Layout.preferredWidth: 80
                                        Layout.preferredHeight: 80
                                        Layout.leftMargin: 5
                                        opacity: inputList.currentIndex < 0 || imgRect.GridView.isCurrentItem ? 1.0 : 0.5

                                        Image {
                                            id: image
                                            fillMode: Image.PreserveAspectFit
                                            source: img
                                            sourceSize: Qt.size(parent.width, parent.height)
                                            smooth: true
                                            visible: false
                                        }

                                        Rectangle {
                                            id: mask
                                            width: 80
                                            height: 80
                                            radius: 12
                                            visible: false
                                        }

                                        OpacityMask {
                                            anchors.fill: image
                                            source: image
                                            maskSource: mask
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.rightMargin: 10
                                        spacing: 2

                                        Text {
                                            Layout.fillWidth: true
                                            text: name
                                            color: Utility.getAppHexColor("lightText")
                                            font.weight: Font.Medium
                                            wrapMode: Text.WordWrap
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: desc
                                            color: Utility.getAppHexColor("normalText")
                                            font.pointSize: 11
                                            wrapMode: Text.WordWrap
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            visible: warning !== ""
                                            text: warning
                                            color: Utility.getAppHexColor("warningText")
                                            font.pointSize: 11
                                            wrapMode: Text.WordWrap
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onPressed: {
                                        inputList.currentIndex = index
                                        inputList.focus = true
                                    }
                                }
                            }
                        }

                        model: inputModel
                        delegate: inputDelegate
                    }
                }
            }

            Item {
                // Guidance and options beside the map on landscape phones
                GridLayout {
                    id: mapGrid
                    anchors.fill: parent
                    columns: phoneLandscape ? 2 : 1
                    uniformCellWidths: true
                    rowSpacing: 12
                    columnSpacing: 12

                    StepHint {
                        title: qsTr("Map the Input Range")
                        hint: qsTr("Throttle control is off while you map the input. Next saves the range once it is valid.")
                    }

                    // The ADC map is taller than a landscape phone
                    FitColumn {
                        Layout.fillWidth: true
                        Layout.fillHeight: phoneLandscape
                        Layout.rowSpan: mapGrid.columns

                        // Keeps the maps above the parameters when they come back from the side
                        ColumnLayout {
                            id: mapBox
                            Layout.fillWidth: true

                            // Next applies the mapping
                            PpmMap {
                                id: ppmMap
                                parent: sideMaps.visible ? sideMaps : mapBox
                                Layout.fillWidth: true
                                visible: false
                                showApply: false
                            }

                            AdcMap {
                                id: adcMap
                                parent: sideMaps.visible ? sideMaps : mapBox
                                Layout.fillWidth: true
                                visible: false
                                showApply: false
                            }
                        }
                    }

                    ParamListScroll {
                        id: paramsMap
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        // One column when beside the map
                        isHorizontal: Screen.width > Screen.height && width > Screen.width / 2
                    }
                }
            }

            Item {
                // Guidance and pairing beside the parameters on landscape phones
                GridLayout {
                    id: confGrid
                    anchors.fill: parent
                    columns: phoneLandscape ? 2 : 1
                    uniformCellWidths: true
                    rowSpacing: 12
                    columnSpacing: 12

                    FitColumn {
                        Layout.fillWidth: true
                        Layout.fillHeight: phoneLandscape

                        StepHint {
                            title: qsTr("Adjust the Input Settings")
                            hint: qsTr("Tune the response, write it to try it, then tap Finish.")
                        }

                        WarningBanner {
                            Layout.fillWidth: true
                            text: qsTr("The input now drives the motor. Keep the wheels clear while testing.")
                        }

                        ColumnLayout {
                            id: nrfPairBox
                            Layout.fillWidth: true
                            spacing: 12
                            visible: false

                            SectionHeader {
                                Layout.topMargin: 0
                                text: qsTr("NRF Pairing")
                            }

                            NrfPair {
                                id: nrfPair
                                Layout.fillWidth: true
                            }
                        }
                    }

                    ParamListScroll {
                        id: paramsConf
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        // Full height beside the guidance, with the button under the guidance
                        Layout.rowSpan: confGrid.columns
                        // One column when beside the guidance
                        isHorizontal: Screen.width > Screen.height && width > Screen.width / 2
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Write Configuration")

                        onClicked: {
                            mCommands.setAppConf()
                        }
                    }
                }
            }
        }

        // The live input stays beside the setup page, read-only
        ColumnLayout {
            id: sideMaps
            anchors.top: parent.top
            anchors.right: parent.right
            width: 360
            enabled: false
            visible: mainIsTablet && isHorizontal && stackLayout.currentIndex === 3 && stackLayout.enabled &&
                     [4, 5].includes(inputModel.get(inputList.currentIndex).apptype)
        }

        header: WizardHeader {
            title: qsTr("Setup Input")
            steps: [qsTr("Connection"), qsTr("Type"), qsTr("Mapping"), qsTr("Setup")]
            currentIndex: stackLayout.currentIndex
        }

        footer: RowLayout {
            spacing: 0

            Button {
                id: prevButton
                Layout.margins: 10
                Layout.rightMargin: 5
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: "Cancel"
                flat: true

                onClicked: {
                    if (stackLayout.currentIndex === 0) {
                        closeWizard(false)
                    } else {
                        stackLayout.currentIndex--

                        if (stackLayout.currentIndex === 2 &&
                                !ppmMap.visible && !adcMap.visible) {
                            stackLayout.currentIndex--
                        }

                        if (stackLayout.currentIndex === 2) {
                            mAppConf.updateParamEnum("app_ppm_conf.ctrl_type", 0, null)
                            mAppConf.updateParamEnum("app_adc_conf.ctrl_type", 0, null)
                            mCommands.setAppConf()
                        }
                    }

                    updateButtonText()
                }
            }

            Button {
                id: nextButton
                Layout.margins: 10
                Layout.leftMargin: 5
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                text: "Next"
                highlighted: true

                onClicked: {
                    workaroundTimerNext.start()
                }
                Timer {
                    id: workaroundTimerNext
                    interval: 0
                    repeat: false
                    running: false
                    onTriggered: {
                        if (stackLayout.currentIndex < (stackLayout.count - 1)) {
                            var apptype = inputModel.get(inputList.currentIndex).apptype
                            var ppmMap_wasVisible = ppmMap.visible
                            var adcMap_wasVisible = adcMap.visible

                            stackLayout.currentIndex++

                            if (stackLayout.currentIndex === 1) {
                                // Type page
                                mCommands.setSendCan(canIdModel.get(canIdList.currentIndex).isCan,
                                                     canIdModel.get(canIdList.currentIndex).canId)

                                disableDialog()
                                var res = Utility.resetInputCan(VescIf, VescIf.getCanDevsLast())
                                enableDialog()

                                if (!res) {
                                    closeWizard(false)
                                }
                            } else if (stackLayout.currentIndex === 2) {
                                // Map page
                                adcMap.visible = false
                                ppmMap.visible = false
                                nrfPairBox.visible = false
                                paramsMap.clear()

                                mAppConf.updateParamEnum("app_ppm_conf.ctrl_type", 0, null)
                                mAppConf.updateParamEnum("app_adc_conf.ctrl_type", 0, null)
                                mAppConf.updateParamEnum("app_to_use", apptype, null)
                                mAppConf.updateParamBool("app_ppm_conf.multi_esc", true, null)
                                mAppConf.updateParamBool("app_adc_conf.multi_esc", true, null)
                                mAppConf.updateParamBool("app_chuk_conf.multi_esc", true, null)
                                mCommands.setAppConf()

                                if (apptype === 4) {
                                    // PPM and UART
                                    ppmMap.visible = true
                                } else if (apptype === 3) {
                                    // UART only (for NRF nunchuk)
                                    nrfPairBox.visible = true
                                    nextButton.clicked()
                                    nrfPairStartDialog.open()
                                    return
                                } else if (apptype === 5) {
                                    // ADC and UART
                                    adcMap.visible = true
                                    paramsMap.addEditorApp("app_adc_conf.voltage_inverted")
                                    paramsMap.addEditorApp("app_adc_conf.voltage2_inverted")
                                    paramsMap.addSpacer()
                                } else if (apptype === 6) {
                                    // Nyko Kama
                                    nextButton.clicked()
                                    return
                                }

                                mapResetTimer.restart()
                            } else if (stackLayout.currentIndex == 3) {
                                // Config page
                                disableDialog()

                                if (ppmMap_wasVisible) {
                                    if (ppmMap.isValid()) {
                                        ppmMap.applyMapping()
                                        Utility.waitSignal(mCommands, "2ackReceived(QString)", 2000)
                                    }
                                }

                                if (adcMap_wasVisible) {
                                    if (adcMap.isValid()) {
                                        adcMap.applyMapping()
                                        Utility.waitSignal(mCommands, "2ackReceived(QString)", 2000)
                                    }
                                }

                                // Setup page
                                paramsConf.clear()
                                mAppConf.updateParamEnum("app_ppm_conf.ctrl_type", 3, null)
                                mAppConf.updateParamEnum("app_adc_conf.ctrl_type", 1, null)
                                mCommands.setAppConf()
                                Utility.waitSignal(mCommands, "2ackReceived(QString)", 2000)

                                enableDialog()

                                if (apptype === 4) {
                                    // PPM and UART
                                    paramsConf.addSeparator("General")
                                    paramsConf.addEditorApp("app_ppm_conf.ctrl_type")
                                    paramsConf.addEditorApp("app_ppm_conf.median_filter")
                                    paramsConf.addEditorApp("app_ppm_conf.safe_start")
                                    paramsConf.addEditorApp("app_ppm_conf.ramp_time_pos")
                                    paramsConf.addEditorApp("app_ppm_conf.ramp_time_neg")
                                    paramsConf.addEditorApp("app_ppm_conf.pid_max_erpm")
                                    paramsConf.addEditorApp("app_ppm_conf.max_erpm_for_dir")
                                    paramsConf.addEditorApp("app_ppm_conf.smart_rev_max_duty")
                                    paramsConf.addEditorApp("app_ppm_conf.smart_rev_ramp_time")
                                    paramsConf.addSeparator("Multiple VESCs over CAN-bus")
                                    paramsConf.addEditorApp("app_ppm_conf.tc")
                                    paramsConf.addEditorApp("app_ppm_conf.tc_max_diff")
                                } else if (apptype === 3 || apptype === 6) {
                                    // NRF nunchuk or nyko kama
                                    paramsConf.addSeparator("General")
                                    paramsConf.addEditorApp("app_chuk_conf.ctrl_type")
                                    paramsConf.addEditorApp("app_chuk_conf.ramp_time_pos")
                                    paramsConf.addEditorApp("app_chuk_conf.ramp_time_neg")
                                    paramsConf.addEditorApp("app_chuk_conf.stick_erpm_per_s_in_cc")
                                    paramsConf.addEditorApp("app_chuk_conf.hyst")
                                    paramsConf.addEditorApp("app_chuk_conf.use_smart_rev")
                                    paramsConf.addEditorApp("app_chuk_conf.smart_rev_max_duty")
                                    paramsConf.addEditorApp("app_chuk_conf.smart_rev_ramp_time")
                                    paramsConf.addSeparator("Multiple VESCs over CAN-bus")
                                    paramsConf.addEditorApp("app_chuk_conf.tc")
                                    paramsConf.addEditorApp("app_chuk_conf.tc_max_diff")
                                } else if (apptype === 5) {
                                    // ADC and UART
                                    paramsConf.addSeparator("General")
                                    paramsConf.addEditorApp("app_adc_conf.ctrl_type")
                                    paramsConf.addEditorApp("app_adc_conf.use_filter")
                                    paramsConf.addEditorApp("app_adc_conf.safe_start")
                                    paramsConf.addEditorApp("app_adc_conf.cc_button_inverted")
                                    paramsConf.addEditorApp("app_adc_conf.rev_button_inverted")
                                    paramsConf.addEditorApp("app_adc_conf.ramp_time_pos")
                                    paramsConf.addEditorApp("app_adc_conf.ramp_time_neg")
                                    paramsConf.addSeparator("Multiple VESCs over CAN-bus")
                                    paramsConf.addEditorApp("app_adc_conf.tc")
                                    paramsConf.addEditorApp("app_adc_conf.tc_max_diff")
                                }

                                paramsConf.addSpacer()
                            }
                        } else {
                            closeWizard(true)
                        }

                        updateButtonText()
                    }
                }
            }
        }
    }

    Timer {
        id: mapResetTimer
        interval: 1500
        running: false
        repeat: false

        onTriggered: {
            ppmMap.reset()
            adcMap.reset()
        }
    }

    SheetDialog {
        id: nrfPairStartDialog
        acceptText: qsTr("Start pairing")
        title: qsTr("NRF Pairing")
        iconSource: "qrc" + Utility.getThemePath() + "icons/icons8-fantasy-96.png"

        Text {
            id: detectLambdaLabel
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            wrapMode: Text.WordWrap
            text: qsTr("NRF remotes have to be paired with the VESC. Tap Start pairing, then switch on " +
                       "the remote within 10 seconds. You can pair again from the setup page.")
        }

        onAccepted: {
            nrfPair.startPairing()
        }
    }

    function updateButtonText() {
        if (stackLayout.currentIndex == (stackLayout.count - 1)) {
            nextButton.text = "Finish"
        } else {
            nextButton.text = "Next"
        }

        if (stackLayout.currentIndex == 0) {
            prevButton.text = "Cancel"
        } else {
            prevButton.text = "Previous"
        }
    }

    function closeWizard(finished) {
        if (finished) {
            var receiver = canIdModel.get(canIdList.currentIndex)
            if (receiver.isCan !== mCommands.getSendCan() ||
                    (receiver.isCan && receiver.canId !== mCommands.getCanSendId())) {
                VescIf.emitMessageDialog(qsTr("Write Input Configuration"),
                                         qsTr("The selected VESC is no longer connected, so the input configuration was not written."),
                                         false, false)
            } else {
                disableDialog()
                var saved = Utility.writeConfigBlocking(VescIf, false)
                enableDialog()
                if (!saved) {
                    VescIf.emitMessageDialog(qsTr("Write Input Configuration"),
                                             qsTr("The VESC did not confirm the write. Check the connection and try Finish again."),
                                             false, false)
                    return
                }
            }
        }
        mCommands.setSendCan(mSendCanAtStart, mCanIdAtStart)
        dialog.close()
    }
}
