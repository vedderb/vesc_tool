/*
    Copyright 2022 Benjamin Vedder	benjamin@vedder.se

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

#include "parameditbitfield.h"
#include "ui_parameditbitfield.h"
#include "utility.h"
#include "helpdialog.h"

ParamEditBitfield::ParamEditBitfield(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamEditBitfield)
{
    ui->setupUi(this);

    QString theme = Utility::getThemePath();
    ui->helpButton->setIcon(QPixmap(theme + "icons/Help-96.png"));
    ui->readButton->setIcon(QPixmap(theme + "icons/Upload-96.png"));
    ui->readDefaultButton->setIcon(QPixmap(theme + "icons/Data Backup-96.png"));

    auto updateFun = [this]() {
        if (mConfig) {
            if (mConfig->getUpdateOnly() != mName) {
                mConfig->setUpdateOnly("");
            }
            mConfig->updateParamInt(mName, getNum(), this);
        }
    };

    connect(ui->b0Box, &QCheckBox::clicked, updateFun);
    connect(ui->b1Box, &QCheckBox::clicked, updateFun);
    connect(ui->b2Box, &QCheckBox::clicked, updateFun);
    connect(ui->b3Box, &QCheckBox::clicked, updateFun);
    connect(ui->b4Box, &QCheckBox::clicked, updateFun);
    connect(ui->b5Box, &QCheckBox::clicked, updateFun);
    connect(ui->b6Box, &QCheckBox::clicked, updateFun);
    connect(ui->b7Box, &QCheckBox::clicked, updateFun);
}

void ParamEditBitfield::setConfig(ConfigParams *config)
{
    mConfig = config;

    ConfigParam *param = mConfig->getParam(mName);

    if (param) {
        ui->readButton->setVisible(param->transmittable);
        ui->readDefaultButton->setVisible(param->transmittable);
        setBits(param->valInt);

        if (param->enumNames.size() > 0) ui->b0Box->setText(param->enumNames.at(0));
        if (param->enumNames.size() > 1) ui->b1Box->setText(param->enumNames.at(1));
        if (param->enumNames.size() > 2) ui->b2Box->setText(param->enumNames.at(2));
        if (param->enumNames.size() > 3) ui->b3Box->setText(param->enumNames.at(3));
        if (param->enumNames.size() > 4) ui->b4Box->setText(param->enumNames.at(4));
        if (param->enumNames.size() > 5) ui->b5Box->setText(param->enumNames.at(5));
        if (param->enumNames.size() > 6) ui->b6Box->setText(param->enumNames.at(6));
        if (param->enumNames.size() > 7) ui->b7Box->setText(param->enumNames.at(7));

        ui->b0Box->setVisible(ui->b0Box->text().toLower() != "unused");
        ui->b1Box->setVisible(ui->b1Box->text().toLower() != "unused");
        ui->b2Box->setVisible(ui->b2Box->text().toLower() != "unused");
        ui->b3Box->setVisible(ui->b3Box->text().toLower() != "unused");
        ui->b4Box->setVisible(ui->b4Box->text().toLower() != "unused");
        ui->b5Box->setVisible(ui->b5Box->text().toLower() != "unused");
        ui->b6Box->setVisible(ui->b6Box->text().toLower() != "unused");
        ui->b7Box->setVisible(ui->b7Box->text().toLower() != "unused");
    }

    connect(mConfig, SIGNAL(paramChangedInt(QObject*,QString,int)),
            this, SLOT(paramChangedInt(QObject*,QString,int)));
}

QString ParamEditBitfield::name() const
{
    return mName;
}

void ParamEditBitfield::setName(const QString &name)
{
    mName = name;
}

void ParamEditBitfield::paramChangedInt(QObject *src, QString name, int newParam)
{
    if (src != this && name == mName) {
        setBits(newParam);
    }
}

ParamEditBitfield::~ParamEditBitfield()
{
    delete ui;
}

void ParamEditBitfield::on_readButton_clicked()
{
    if (mConfig) {
        mConfig->setUpdateOnly(mName);
        mConfig->requestUpdate();
    }
}

void ParamEditBitfield::on_readDefaultButton_clicked()
{
    if (mConfig) {
        mConfig->setUpdateOnly(mName);
        mConfig->requestUpdateDefault();
    }
}

void ParamEditBitfield::on_helpButton_clicked()
{
    if (mConfig) {
        HelpDialog::showHelp(this, mConfig, mName);
    }
}

void ParamEditBitfield::setBits(int num)
{
    ui->b0Box->setChecked((num >> 0) & 1);
    ui->b1Box->setChecked((num >> 1) & 1);
    ui->b2Box->setChecked((num >> 2) & 1);
    ui->b3Box->setChecked((num >> 3) & 1);
    ui->b4Box->setChecked((num >> 4) & 1);
    ui->b5Box->setChecked((num >> 5) & 1);
    ui->b6Box->setChecked((num >> 6) & 1);
    ui->b7Box->setChecked((num >> 7) & 1);
}

int ParamEditBitfield::getNum()
{
    int res = 0;

    if (ui->b0Box->isChecked()) res += 1 << 0;
    if (ui->b1Box->isChecked()) res += 1 << 1;
    if (ui->b2Box->isChecked()) res += 1 << 2;
    if (ui->b3Box->isChecked()) res += 1 << 3;
    if (ui->b4Box->isChecked()) res += 1 << 4;
    if (ui->b5Box->isChecked()) res += 1 << 5;
    if (ui->b6Box->isChecked()) res += 1 << 6;
    if (ui->b7Box->isChecked()) res += 1 << 7;

    return res;
}
