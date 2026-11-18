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
import QtQuick.Layouts
import Vedder.vesc.vescinterface 1.0
import Vedder.vesc.utility 1.0
import "ParamFormat.js" as ParamFormat

// Search for parameters by name, path or description, and for pages and tools
SheetDialog {
    id: search
    signal paramSelected(bool isApp, string group, string subgroup, string name)
    signal pageSelected(string title)

    // No title on landscape phones, which leaves room for results above the keyboard
    title: phoneLandscape ? "" : qsTr("Search")
    rejectText: ""
    fullHeight: true
    onOpened: field.forceActiveFocus()
    onClosed: field.text = ""

    function addConfig(conf, isApp, query) {
        var byName = []
        var byOther = []
        var groups = conf.getParamGroups()

        for (var g = 0;g < groups.length;g++) {
            var subgroups = conf.getParamSubgroups(groups[g])
            for (var s = 0;s < subgroups.length;s++) {
                var names = conf.getParamsFromSubgroup(groups[g], subgroups[s])
                for (var n = 0;n < names.length;n++) {
                    if (names[n].startsWith("::sep::")) {
                        continue
                    }

                    var label = conf.getLongName(names[n])
                    var path = groups[g] + " › " + subgroups[s]
                    var entry = {"section": isApp ? "App Cfg" : "Motor Cfg", "label": label, "path": path,
                                 "isApp": isApp, "group": groups[g], "subgroup": subgroups[s], "param": names[n]}

                    if (label.toLowerCase().indexOf(query) >= 0) {
                        byName.push(entry)
                    } else if (path.toLowerCase().indexOf(query) >= 0 ||
                               conf.getDescription(names[n]).replace(/<[^>]*>/g, "").toLowerCase().indexOf(query) >= 0) {
                        byOther.push(entry)
                    }
                }
            }
        }

        byName.concat(byOther).forEach(function(e) { resultModel.append(e) })
    }

    function updateResults() {
        resultModel.clear()
        var query = field.text.trim().toLowerCase()

        if (query.length < 2) {
            return
        }

        addConfig(VescIf.mcConfig(), false, query)
        addConfig(VescIf.appConfig(), true, query)

        var pages = ["Controls", "Multi Settings", "Settings"]
        for (var i = 0;i < tabBar.count;i++) {
            pages.push(tabBar.itemAt(i).text)
        }

        pages.forEach(function(p) {
            if (p.toLowerCase().indexOf(query) >= 0) {
                resultModel.append({"section": qsTr("Pages and tools"), "label": p, "path": "",
                                    "isApp": false, "group": "", "subgroup": "", "param": ""})
            }
        })
    }

    ListModel {
        id: resultModel
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        InputField {
            id: field
            Layout.fillWidth: true
            placeholderText: qsTr("Search parameters and pages")
            inputMethodHints: Qt.ImhNoPredictiveText
            onTextChanged: updateResults()
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: resultModel
            section.property: "section"
            section.delegate: SectionHeader {
                width: ListView.view.width
                text: section
            }

            delegate: ItemDelegate {
                width: ListView.view.width
                onClicked: {
                    search.close()
                    if (param !== "") {
                        search.paramSelected(isApp, group, subgroup, param)
                    } else {
                        search.pageSelected(label)
                    }
                }

                contentItem: RowLayout {
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Text {
                            Layout.fillWidth: true
                            text: label
                            elide: Text.ElideRight
                            color: Utility.getAppHexColor("lightText")
                        }

                        Text {
                            visible: path !== ""
                            text: path
                            font.pointSize: 9
                            color: Utility.getAppHexColor("normalText")
                        }
                    }

                    Text {
                        visible: param !== ""
                        text: param !== "" ? ParamFormat.valueString(isApp ? VescIf.appConfig() : VescIf.mcConfig(), param) : ""
                        font.weight: Font.Medium
                        color: Utility.getAppHexColor("lightText")
                    }
                }
            }
        }
    }
}
