/*
    Copyright 2016 - 2017 Benjamin Vedder	benjamin@vedder.se

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

#include "parameditbool.h"
#include "ui_parameditbool.h"
#include <QMessageBox>
#include "helpdialog.h"

ParamEditBool::ParamEditBool(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamEditBool)
{
    ui->setupUi(this);
}

ParamEditBool::~ParamEditBool()
{
    delete ui;
}

void ParamEditBool::setConfig(ConfigParams *config)
{
    mConfig = config;

    ConfigParam *param = mConfig->getParam(mName);

    if (param) {
        ui->readButton->setVisible(param->transmittable);
        ui->readDefaultButton->setVisible(param->transmittable);
        ui->valueBox->setCurrentIndex(param->valInt ? 1 : 0);
    }

    connect(mConfig, SIGNAL(paramChangedBool(QObject*,QString,bool)),
            this, SLOT(paramChangedBool(QObject*,QString,bool)));
}

QString ParamEditBool::name() const
{
    return mName;
}

void ParamEditBool::setName(const QString &name)
{
    mName = name;
}

void ParamEditBool::paramChangedBool(QObject *src, QString name, bool newParam)
{
    if (src != this && name == mName) {
        ui->valueBox->setCurrentIndex(newParam ? 1 : 0);
    }
}

void ParamEditBool::on_readButton_clicked()
{
    if (mConfig) {
        mConfig->setUpdateOnly(mName);
        mConfig->requestUpdate();
    }
}

void ParamEditBool::on_readDefaultButton_clicked()
{
    if (mConfig) {
        mConfig->setUpdateOnly(mName);
        mConfig->requestUpdateDefault();
    }
}

void ParamEditBool::on_helpButton_clicked()
{
    if (mConfig) {
        HelpDialog::showHelp(this, mConfig, mName);
    }
}

void ParamEditBool::on_valueBox_currentIndexChanged(int index)
{
    if (mConfig) {
        if (mConfig->getUpdateOnly() != mName) {
            mConfig->setUpdateOnly("");
        }
        mConfig->updateParamBool(mName, index, this);
    }
}
