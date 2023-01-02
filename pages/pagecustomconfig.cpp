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
#include "utility.h"
#include <QFileDialog>

PageCustomConfig::PageCustomConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageCustomConfig)
{
    ui->setupUi(this);
    mVesc = nullptr;
    mConfNum = 0;

    ui->readButton->setIcon(Utility::getIcon("/icons/Upload-96.png"));
    ui->readDefaultButton->setIcon(Utility::getIcon("/icons/Upload-96.png"));
    ui->writeButton->setIcon(Utility::getIcon("/icons/Download-96.png"));
    ui->saveXmlButton->setIcon(Utility::getIcon("/icons/Save as-96.png"));
    ui->loadXmlButton->setIcon(Utility::getIcon("/icons/Open Folder-96.png"));
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
        connect(mVesc, SIGNAL(customConfigLoadDone()),
                this, SLOT(customConfigLoadDone()));
    }
}

void PageCustomConfig::setConfNum(int num)
{
    mConfNum = num;
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
            mVesc->commands()->customConfigSet(mConfNum, params);
        }
    }
}

void PageCustomConfig::on_saveXmlButton_clicked()
{
    QString path;
    path = QFileDialog::getSaveFileName(this,
                                        tr("Choose where to save the motor configuration XML file"),
                                        ".",
                                        tr("Xml files (*.xml)"));

    if (path.isNull()) {
        return;
    }

    if (!path.toLower().endsWith(".xml")) {
        path += ".xml";
    }

    bool res = mVesc->customConfig(mConfNum)->saveXml(path, "CustomConfiguration");

    if (res) {
        mVesc->emitStatusMessage("Saved custom configuration", true);
    } else {
        mVesc->emitMessageDialog(tr("Save custom configuration"),
                                 tr("Could not save custom configuration:<BR>"
                                    "%1").arg(mVesc->mcConfig()->xmlStatus()),
                                 false, false);
    }
}

void PageCustomConfig::on_loadXmlButton_clicked()
{
    QString path;
    path = QFileDialog::getOpenFileName(this,
                                        tr("Choose custom configuration file to load"),
                                        ".",
                                        tr("Xml files (*.xml)"));

    if (path.isNull()) {
        return;
    }

    bool res = mVesc->customConfig(mConfNum)->loadXml(path, "CustomConfiguration");

    if (res) {
        mVesc->emitStatusMessage("Loaded custom configuration", true);
    } else {
        mVesc->emitMessageDialog(tr("Load custom configuration"),
                                 tr("Could not load custom configuration:<BR>"
                                    "%1").arg(mVesc->mcConfig()->xmlStatus()),
                                 false, false);
    }
}
