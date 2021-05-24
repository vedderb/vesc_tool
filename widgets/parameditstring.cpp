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

#include "parameditstring.h"
#include "ui_parameditstring.h"
#include "helpdialog.h"

ParamEditString::ParamEditString(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamEditString)
{
    ui->setupUi(this);
}

ParamEditString::~ParamEditString()
{
    delete ui;
}

void ParamEditString::setConfig(ConfigParams *config)
{
    mConfig = config;

    ConfigParam *param = mConfig->getParam(mName);

    if (param) {
        ui->valueEdit->setText(param->valString);
    }

    connect(mConfig, SIGNAL(paramChangedQString(QObject*,QString,QString)),
            this, SLOT(paramChangedQString(QObject*,QString,QString)));
}

QString ParamEditString::name() const
{
    return mName;
}

void ParamEditString::setName(const QString &name)
{
    mName = name;
}

void ParamEditString::paramChangedQString(QObject *src, QString name, QString newParam)
{
    if (src != this && name == mName) {
        ui->valueEdit->setText(newParam);
    }
}

void ParamEditString::on_helpButton_clicked()
{
    if (mConfig) {
        HelpDialog::showHelp(this, mConfig, mName);
    }
}

void ParamEditString::on_valueEdit_textChanged(const QString &arg1)
{
    if (mConfig) {
        if (mConfig->getUpdateOnly() != mName) {
            mConfig->setUpdateOnly("");
        }
        mConfig->updateParamString(mName, arg1, this);
    }
}
