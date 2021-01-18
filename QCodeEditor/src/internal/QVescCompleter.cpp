/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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

#include <QVescCompleter>
#include <QLanguage>
#include <QStringListModel>
#include <QFile>
#include <QStandardItemModel>

QVescCompleter::QVescCompleter(QObject *parent) :
    QCompleter(parent)
{
    QStandardItemModel *model = new QStandardItemModel(this);
    QVector<QStandardItem *> parents(10);
    parents[0] = model->invisibleRootItem();
    int level = 0;
    QStringList level0Names;

    auto addItem = [&](QString text) {
        if (level == 0) {
            if (level0Names.contains(text)) {
                return;
            }

            level0Names.append(text);
        }

        QStandardItem *item = new QStandardItem;
        item->setText(text);
        parents[level]->appendRow(item);
        parents[level + 1] = item;

        if (parents.size() <= (level + 1)) {
            parents.resize(parents.size() * 2);
        }
    };

    auto addCommandCmds = [&]() {
        level++;
        addItem("setSendCan(send, id)");
        addItem("getValues()");
        addItem("getValuesSetup()");
        addItem("bmsGetValues()");
        addItem("sendAlive()");
        addItem("sendTerminalCmd(cmdStr)");
        addItem("sendTerminalCmdSync(cmdStr)");
        addItem("setDutyCycle(duty)");
        addItem("setCurrent(current)");
        addItem("setCurrentBrake(current)");
        addItem("setRpm(rpm)");
        addItem("setPos(degrees)");
        addItem("setHandbrake(current)");
        addItem("getAppConf()");
        addItem("getAppConfDefault()");
        addItem("setAppConf()");
        addItem("getMcconf()");
        addItem("getMcconfDefault()");
        addItem("setMcconf(true)");
        addItem("forwardCanFrame(data, id, isExt)");
        level--;
    };

    auto addConfigCmds = [&]() {
        level++;
        addItem("getParamDouble(paramName)");
        addItem("getParamInt(paramName)");
        addItem("getParamEnum(paramName)");
        addItem("getParamBool(paramName)");
        addItem("updateParamDouble(paramName, paramValue, caller)");
        addItem("updateParamInt(paramName, paramValue, caller)");
        addItem("updateParamEnum(paramName, paramValue, caller)");
        addItem("updateParamBool(paramName, paramValue, caller)");
        level--;
    };

    addItem("VescIf");
    {
        level++;
        addItem("commands()");
        addCommandCmds();
        addItem("autoconnect()");
        addItem("disconnectPort()");
        addItem("isPortConnected()");
        addItem("getConnectedPortName()");
        addItem("scanCANbus()");
        addItem("mcConfig()");
        addConfigCmds();
        addItem("appConfig()");
        addConfigCmds();
        level--;
    }

    addItem("Component");
    {
        level++;
        addItem("onCompleted:");
        level--;
    }

    addItem("console");
    {
        level++;
        addItem("log(str)");
        level--;
    }

    addItem("anchors");
    {
        level++;
        addItem("fill: ");
        addItem("top: ");
        addItem("bottom: ");
        addItem("left: ");
        addItem("right: ");
        addItem("margins: ");
        addItem("leftMargin: ");
        addItem("rightMargin: ");
        addItem("topMargin: ");
        addItem("bottomMargin: ");
        level--;
    }

    addItem("parent");
    {
        level++;
        addItem("top");
        addItem("bottom");
        addItem("left");
        addItem("right");
        addItem("width");
        addItem("height");
        level--;
    }

    addItem("Layout");
    {
        level++;
        addItem("fillWidth: ");
        addItem("fillHeight: ");
        addItem("margins: ");
        addItem("leftMargin: ");
        addItem("rightMargin: ");
        addItem("topMargin: ");
        addItem("bottomMargin: ");
        addItem("preferredWidth: ");
        addItem("preferredHeight: ");
        level--;
    }

    addItem("mCommands");
    addCommandCmds();

    addItem("mMcConf");
    addConfigCmds();

    addItem("mAppConf");
    addConfigCmds();

    Q_INIT_RESOURCE(qcodeeditor_resources);
    QFile fl(":/languages/qml.xml");

    if (fl.open(QIODevice::ReadOnly)) {
        QLanguage language(&fl);

        if (language.isLoaded()) {
            auto keys = language.keys();
            for (auto&& key : keys) {
                for (auto name: language.names(key)) {
                    addItem(name);
                }
            }
        }

        fl.close();
    }

    setCompletionMode(QCompleter::PopupCompletion);

    setModel(model);
    setModelSorting(QCompleter::UnsortedModel);
    setCaseSensitivity(Qt::CaseInsensitive);
    setWrapAround(true);
}

QStringList QVescCompleter::splitPath(const QString &path) const
{
    return path.split(".");
}

QString QVescCompleter::pathFromIndex(const QModelIndex &index) const
{
    QStringList dataList;
    for (QModelIndex i = index; i.isValid(); i = i.parent()) {
        dataList.prepend(model()->data(i, completionRole()).toString());
    }

    return dataList.join(".");
}
