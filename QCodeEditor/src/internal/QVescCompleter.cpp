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
        addItem("setAppConfNoStore()");
        addItem("getMcconf()");
        addItem("getMcconfDefault()");
        addItem("setMcconf(check)");
        addItem("forwardCanFrame(data, id, isExt)");
        addItem("pswGetStatus(by_id, id_ind)");
        addItem("pswSwitch(id, is_on, plot)");
        addItem("getSendCan()");
        addItem("getCanSendId()");
        addItem("bmsGetCanDevNum()");
        addItem("bmsGetCanValues(can_id)");
        addItem("bmsHasCanValues(can_id)");
        addItem("bmsSetChargeAllowed(allowed)");
        addItem("bmsSetBalanceOverride(cell, override)");
        addItem("bmsResetCounters(ah, wh)");
        addItem("bmsForceBalance(bal_en)");
        addItem("bmsZeroCurrentOffset()");
        addItem("ioBoardGetAll(id)");
        addItem("ioBoardSetPwm(id, channel, duty)");
        addItem("ioBoardSetDigital(id, channel, on)");
        addItem("emitPlotInit(xLabel, yLabel)");
        addItem("emitPlotData(x, y)");
        addItem("emitPlotAddGraph(name)");
        addItem("emitPlotSetGraph(graph)");
        addItem("getStats(mask)");
        addItem("getImuData(mask)");
        addItem("setBleName(name)");
        addItem("setBlePin(pin)");
        addItem("sendCustomAppData(data)");
        addItem("customConfigGet(confInd, isDefault)");
        addItem("customConfigSet(confInd, config)");
        addItem("fileBlockList(path)");
        addItem("fileBlockRead(path)");
        addItem("fileBlockWrite(path, data)");
        addItem("fileBlockMkdir(path)");
        addItem("fileBlockRemove(path)");
        addItem("getGnss(mask)");
        level--;
    };

    auto addConfigCmds = [&]() {
        level++;
        addItem("addParam(name, param)");
        addItem("getParamCopy(name)");
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
        addItem("reconnectLastPort()");
        addItem("disconnectPort()");
        addItem("isPortConnected()");
        addItem("getConnectedPortName()");
        addItem("scanCANbus()");
        addItem("getCanDevsLast()");
        addItem("ignoreCanChange(ignore)");
        addItem("mcConfig()");
        addConfigCmds();
        addItem("appConfig()");
        addConfigCmds();
        addItem("canTmpOverride(fwdCan, canId)");
        addItem("canTmpOverrideEnd()");
        addItem("emitMessageDialog(title, text, isGood, isRichText)");
        addItem("emitStatusMessage(msg, isGood)");
        addItem("customConfig(confInd)");
        level--;
    }

    addItem("Component");
    {
        level++;
        addItem("onCompleted:");
        addItem("onDestruction:");
        level--;
    }

    addItem("Math");
    {
        level++;
        addItem("E");
        addItem("LN2");
        addItem("LN10");
        addItem("LOG2E");
        addItem("LOG10E");
        addItem("PI");
        addItem("SQRT1_2");
        addItem("SQRT2");

        addItem("abs(x)");
        addItem("acos(x)");
        addItem("acosh(x)");
        addItem("asin(x)");
        addItem("asinh(x)");
        addItem("atan(x)");
        addItem("atanh(x)");
        addItem("atan2(y, x)");
        addItem("cbrt(x)");
        addItem("ceil(x)");
        addItem("clz32(x)");
        addItem("cos(x)");
        addItem("cosh(x)");
        addItem("exp(x)");
        addItem("expm1(x)");
        addItem("floor(x)");
        addItem("fround(x)");
        addItem("hypot(...");
        addItem("imul(x, y)");
        addItem("log(x)");
        addItem("log1p(x)");
        addItem("log10(x)");
        addItem("log2(x)");
        addItem("max(...");
        addItem("min(...)");
        addItem("pow(x, y)");
        addItem("random()");
        addItem("round(x)");
        addItem("sign(x)");
        addItem("sin(x)");
        addItem("sinh(x)");
        addItem("sqrt(x)");
        addItem("tan(x)");
        addItem("tanh(x)");
        addItem("trunc(x)");
        level--;
    }

    addItem("console");
    {
        level++;
        addItem("log(str)");
        level--;
    }

    addItem("ApplicationWindow");
    {
        level++;
        addItem("overlay");
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
        addItem("columnSpan: ");
        level--;
    }

    addItem("Utility");
    {
        level++;
        addItem("measureRLBlocking(VescIf)");
        addItem("measureLinkageOpenloopBlocking(VescIf, current, erpm_per_sec, low_duty, resistance, inductance)");
        addItem("measureHallFocBlocking(VescIf, current)");
        addItem("getMcValuesBlocking(VescIf)");
        addItem("waitMotorStop(VescIf, erpmTres, timeoutMs)");
        addItem("measureEncoderBlocking(VescIf, current)");
        addItem("sleepWithEventLoop(timeMs)");
        addItem("readInternalImuType(VescIf)");
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
