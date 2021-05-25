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

#include "parameditenum.h"
#include "ui_parameditenum.h"
#include <QDebug>
#include "helpdialog.h"

ParamEditEnum::ParamEditEnum(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamEditEnum)
{
    ui->setupUi(this);
    mConfig = 0;
}

ParamEditEnum::~ParamEditEnum()
{
    delete ui;
}

void ParamEditEnum::setConfig(ConfigParams *config)
{
    mConfig = config;

    ConfigParam *param = mConfig->getParam(mName);

    if (param) {
        ui->readButton->setVisible(param->transmittable);
        ui->readDefaultButton->setVisible(param->transmittable);

        int val = param->valInt;
        ui->valueBox->insertItems(0, param->enumNames);
        if (val >= 0 && val < param->enumNames.size()) {
            ui->valueBox->setCurrentIndex(val);
        }
    }

    connect(mConfig, SIGNAL(paramChangedEnum(QObject*,QString,int)),
            this, SLOT(paramChangedEnum(QObject*,QString,int)));
}

QString ParamEditEnum::name() const
{
    return mName;
}

void ParamEditEnum::setName(const QString &name)
{
    mName = name;
}

void ParamEditEnum::paramChangedEnum(QObject *src, QString name, int newParam)
{
    if (src != this && name == mName) {
        ui->valueBox->setCurrentIndex(newParam);
    }
}

void ParamEditEnum::on_readButton_clicked()
{
    if (mConfig) {
        mConfig->setUpdateOnly(mName);
        mConfig->requestUpdate();
    }
}

void ParamEditEnum::on_readDefaultButton_clicked()
{
    if (mConfig) {
        mConfig->setUpdateOnly(mName);
        mConfig->requestUpdateDefault();
    }
}

void ParamEditEnum::on_helpButton_clicked()
{
    if (mConfig) {
        HelpDialog::showHelp(this, mConfig, mName);
    }
}

void ParamEditEnum::on_valueBox_currentIndexChanged(int index)
{
    if (mConfig) {
        if (mConfig->getUpdateOnly() != mName) {
            mConfig->setUpdateOnly("");
        }
        mConfig->updateParamEnum(mName, index, this);
    }
}
