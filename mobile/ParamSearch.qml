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

// Search for parameters by name, path or description, and for pages and tools
SheetDialog {
    id: search
    // Configs to search, each with title, params, pageIndex and optionally the groups to include
    property var sources: []
    signal paramSelected(int sourceIndex, string group, string subgroup, string name)
    signal pageSelected(string title)

    // No title on landscape phones, which leaves room for results above the keyboard
    title: phoneLandscape ? "" : qsTr("Search")
    rejectText: ""
    fullHeight: true
    onOpened: field.forceActiveFocus()
    onClosed: field.text = ""

    function addConfig(sourceIndex, query) {
        var source = sources[sourceIndex]
        var conf = source.params
        if (!conf) {
            return
        }

        var byName = []
        var byOther = []
        var groups = source.groups || conf.getParamGroups()

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
                    var entry = {"section": source.title, "label": label, "path": path,
                                 "sourceIndex": sourceIndex, "group": groups[g], "subgroup": subgroups[s], "param": names[n]}

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
        var query = field.displayText.trim().toLowerCase()

        if (query.length < 2) {
            return
        }

        for (var s = 0;s < sources.length;s++) {
            addConfig(s, query)
        }

        var pages = ["Controls", "Multi Settings", "Settings"]
        for (var i = 0;i < tabBar.count;i++) {
            pages.push(tabBar.itemAt(i).text)
        }

        pages.forEach(function(p) {
            if (p.toLowerCase().indexOf(query) >= 0) {
                resultModel.append({"section": qsTr("Pages and tools"), "label": p, "path": "",
                                    "sourceIndex": -1, "group": "", "subgroup": "", "param": ""})
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
            onDisplayTextChanged: updateResults()
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
                enabled: sourceIndex < 0 || search.sources[sourceIndex].pageIndex >= 0
                onClicked: {
                    search.close()
                    if (param !== "") {
                        search.paramSelected(sourceIndex, group, subgroup, param)
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
                        text: param !== "" && search.sources[sourceIndex].params ?
                                  search.sources[sourceIndex].params.getParamValueString(param) : ""
                        font.weight: Font.Medium
                        color: Utility.getAppHexColor("lightText")
                    }
                }
            }
        }
    }
}
