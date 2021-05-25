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

#include "pagecustomconfig.h"
#include "ui_pagecustomconfig.h"
#include "widgets/paramtable.h"

PageCustomConfig::PageCustomConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageCustomConfig)
{
    ui->setupUi(this);
    mVesc = nullptr;
    mConfNum = 0;
}

PageCustomConfig::~PageCustomConfig()
{
    delete ui;
}

VescInterface *PageCustomConfig::vesc() const
{
    return mVesc;
}

void PageCustomConfig::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(customConfigRx(int,QByteArray)),
                this, SLOT(customConfigRx(int,QByteArray)));
        connect(mVesc, SIGNAL(customConfigLoadDone()),
                this, SLOT(customConfigLoadDone()));
    }
}

void PageCustomConfig::setConfNum(int num)
{
    mConfNum = num;
}

void PageCustomConfig::customConfigRx(int confInd, QByteArray data)
{
    if (confInd == mConfNum) {
        ConfigParams *params = mVesc->customConfig(mConfNum);
        if (params) {
            auto vb = VByteArray(data);
            if (params->deSerialize(vb)) {
                mVesc->emitStatusMessage(tr("Custom config %1 updated").arg(mConfNum), true);
            } else {
                mVesc->emitMessageDialog(tr("Custom Configuration"),
                                         tr("Could not deserialize custom config %1").arg(mConfNum),
                                         false, false);
            }
        }
    }
}

void PageCustomConfig::customConfigLoadDone()
{
    ConfigParams *p = mVesc->customConfig(mConfNum);
    if (p) {
        QStringList tabNames = p->getParamSubgroups("General");
        while (ui->tabWidget->count()) {
            delete ui->tabWidget->widget(0);
        }

        while (ui->tabWidget->count() < tabNames.size()) {
            ui->tabWidget->addTab(new ParamTable(ui->tabWidget), "");
        }

        for (int i = 0;i < tabNames.size();i++) {
            ui->tabWidget->setTabText(i, tabNames.at(i));
            ParamTable *t = dynamic_cast<ParamTable*>(ui->tabWidget->widget(i));
            t->clearParams();
            t->addParamSubgroup(p, "General", tabNames.at(i));
        }

        on_readButton_clicked();
    }
}

void PageCustomConfig::on_readButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->customConfigGet(mConfNum, false);
    }
}

void PageCustomConfig::on_readDefaultButton_clicked()
{
    if (mVesc) {
        mVesc->commands()->customConfigGet(mConfNum, true);
    }
}

void PageCustomConfig::on_writeButton_clicked()
{
    if (mVesc) {
        ConfigParams *params = mVesc->customConfig(mConfNum);
        if (params) {
            VByteArray data;
            params->serialize(data);
            mVesc->commands()->customConfigSet(mConfNum, data);
        }
    }
}
