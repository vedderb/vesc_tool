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

#include "parameditint.h"
#include "ui_parameditint.h"
#include <QMessageBox>
#include <cstdlib>
#include "helpdialog.h"

ParamEditInt::ParamEditInt(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamEditInt)
{
    ui->setupUi(this);
    mConfig = 0;
    mMaxVal = 1;

    mDisplay = new DisplayPercentage(this);
    mIntBox = new QSpinBox(this);
    mPercentageBox = new QSpinBox(this);

    mPercentageBox->setSuffix(" %");

    ui->mainLayout->insertWidget(0, mDisplay);
    ui->mainLayout->insertWidget(0, mIntBox);
    ui->mainLayout->insertWidget(0, mPercentageBox);

    ui->mainLayout->setStretchFactor(mIntBox, 1);
    ui->mainLayout->setStretchFactor(mPercentageBox, 1);
    ui->mainLayout->setStretchFactor(mDisplay, 10);

    mPercentageBox->setVisible(false);
    mDisplay->setVisible(false);

    connect(mIntBox, SIGNAL(valueChanged(int)),
            this, SLOT(intChanged(int)));
    connect(mPercentageBox, SIGNAL(valueChanged(int)),
            this, SLOT(percentageChanged(int)));
}

ParamEditInt::~ParamEditInt()
{
    delete ui;
}

void ParamEditInt::setConfig(ConfigParams *config)
{
    mConfig = config;

    ConfigParam *param = mConfig->getParam(mName);

    if (param) {
        ui->readButton->setVisible(param->transmittable);
        ui->readDefaultButton->setVisible(param->transmittable);

        mParam = *param;

        mMaxVal = abs(mParam.maxInt) > abs(mParam.minInt) ?
                    abs(mParam.maxInt) : abs(mParam.minInt);

        mIntBox->setMaximum(multScale(mParam.maxInt));
        mIntBox->setMinimum(multScale(mParam.minInt));
        mIntBox->setSingleStep(mParam.stepInt);
        mIntBox->setValue(multScale(mParam.valInt));

        int p = (mParam.valInt * 100) / mMaxVal;
        mPercentageBox->setMaximum((100 * mParam.maxInt) / mMaxVal);
        mPercentageBox->setMinimum((100 * mParam.minInt) / mMaxVal);
        mPercentageBox->setValue(p);
        mDisplay->setDual(mParam.minInt < 0 && mParam.maxInt > 0);

        // Rounding...
        if (!mParam.editAsPercentage) {
            updateDisplay(mParam.valInt);
        }
    }

    connect(mConfig, SIGNAL(paramChangedInt(QObject*,QString,int)),
            this, SLOT(paramChangedInt(QObject*,QString,int)));
}

QString ParamEditInt::name() const
{
    return mName;
}

void ParamEditInt::setName(const QString &name)
{
    mName = name;
}

void ParamEditInt::setSuffix(const QString &suffix)
{
    mIntBox->setSuffix(suffix);
}

void ParamEditInt::setShowAsPercentage(bool showAsPercentage)
{
    mIntBox->setVisible(!showAsPercentage);
    mPercentageBox->setVisible(showAsPercentage);
}

void ParamEditInt::showDisplay(bool show)
{
    mDisplay->setVisible(show);
}

void ParamEditInt::paramChangedInt(QObject *src, QString name, int newParam)
{
    if (src != this && name == mName) {
        mIntBox->setValue(multScale(newParam));
    }
}

void ParamEditInt::percentageChanged(int p)
{
    if (mParam.editAsPercentage) {
        int val = (p * mMaxVal) / 100;

        if (mConfig) {
            if (mConfig->getUpdateOnly() != mName) {
                mConfig->setUpdateOnly("");
            }
            mConfig->updateParamInt(mName, val, this);
        }

        updateDisplay(val);
    }
}

void ParamEditInt::intChanged(int i)
{
    if (!mParam.editAsPercentage) {
        int val = divScale(i);

        if (mConfig) {
            if (mConfig->getUpdateOnly() != mName) {
                mConfig->setUpdateOnly("");
            }
            mConfig->updateParamInt(mName, val, this);
        }

        updateDisplay(val);
    }
}

void ParamEditInt::on_readButton_clicked()
{
    if (mConfig) {
        mConfig->setUpdateOnly(mName);
        mConfig->requestUpdate();
    }
}

void ParamEditInt::on_readDefaultButton_clicked()
{
    if (mConfig) {
        mConfig->setUpdateOnly(mName);
        mConfig->requestUpdateDefault();
    }
}

void ParamEditInt::on_helpButton_clicked()
{
    if (mConfig) {
        HelpDialog::showHelp(this, mConfig, mName);
    }
}

void ParamEditInt::updateDisplay(int val)
{
    int p = (100 * val) / mMaxVal;
    mDisplay->setValue(p);

    if (mParam.editAsPercentage) {
        mDisplay->setText(tr("%1%2").
                          arg((double)val * mParam.editorScale, 0, 'f', 2).
                          arg(mParam.suffix));
    } else {
        mDisplay->setText(tr("%1%2").
                          arg(multScale(val)).
                          arg(mParam.suffix));
    }
}

int ParamEditInt::multScale(int val)
{
    return (int)((double)val * mParam.editorScale);
}

int ParamEditInt::divScale(int val)
{
    return (int)((double)val / mParam.editorScale);
}
