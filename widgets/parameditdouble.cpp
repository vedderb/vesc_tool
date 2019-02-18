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

#include "parameditdouble.h"
#include "ui_parameditdouble.h"
#include <QMessageBox>
#include "helpdialog.h"
#include <cmath>

ParamEditDouble::ParamEditDouble(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamEditDouble)
{
    ui->setupUi(this);
    mConfig = 0;
    mMaxVal = 1.0;

    mDisplay = new DisplayPercentage(this);
    mDoubleBox = new QDoubleSpinBox(this);
    mPercentageBox = new QSpinBox(this);

    mPercentageBox->setSuffix(" %");

    ui->mainLayout->insertWidget(0, mDisplay);
    ui->mainLayout->insertWidget(0, mDoubleBox);
    ui->mainLayout->insertWidget(0, mPercentageBox);

    ui->mainLayout->setStretchFactor(mDoubleBox, 1);
    ui->mainLayout->setStretchFactor(mPercentageBox, 1);
    ui->mainLayout->setStretchFactor(mDisplay, 10);

    mPercentageBox->setVisible(false);
    mDisplay->setVisible(false);

    connect(mPercentageBox, SIGNAL(valueChanged(int)),
            this, SLOT(percentageChanged(int)));
    connect(mDoubleBox, SIGNAL(valueChanged(double)),
            this, SLOT(doubleChanged(double)));
}

ParamEditDouble::~ParamEditDouble()
{
    delete ui;
}

void ParamEditDouble::setConfig(ConfigParams *config)
{
    mConfig = config;

    ConfigParam *param = mConfig->getParam(mName);

    if (param) {
        ui->readButton->setVisible(param->transmittable);
        ui->readDefaultButton->setVisible(param->transmittable);

        mParam = *param;

        mMaxVal = fabs(mParam.maxDouble) > fabs(mParam.minDouble) ?
                    fabs(mParam.maxDouble) : fabs(mParam.minDouble);

        mDoubleBox->setMaximum(mParam.maxDouble * mParam.editorScale);
        mDoubleBox->setMinimum(mParam.minDouble * mParam.editorScale);
        mDoubleBox->setSingleStep(mParam.stepDouble);
        mDoubleBox->setValue(mParam.valDouble * mParam.editorScale);

        int p = (mParam.valDouble * 100.0) / mMaxVal;
        mPercentageBox->setMaximum((100.0 * mParam.maxDouble) / mMaxVal);
        mPercentageBox->setMinimum((100.0 * mParam.minDouble) / mMaxVal);
        mPercentageBox->setValue(p);
        mDisplay->setDual(mParam.minDouble < 0.0 && mParam.maxDouble > 0.0);

        // Rounding...
        if (!mParam.editAsPercentage) {
            updateDisplay(mParam.valDouble);
        }
    }

    connect(mConfig, SIGNAL(paramChangedDouble(QObject*,QString,double)),
            this, SLOT(paramChangedDouble(QObject*,QString,double)));
}

QString ParamEditDouble::name() const
{
    return mName;
}

void ParamEditDouble::setName(const QString &name)
{
    mName = name;
}

void ParamEditDouble::setSuffix(const QString &suffix)
{
    mDoubleBox->setSuffix(suffix);
}

void ParamEditDouble::setDecimals(int decimals)
{
    mDoubleBox->setDecimals(decimals);
}

void ParamEditDouble::setShowAsPercentage(bool showAsPercentage)
{
    mDoubleBox->setVisible(!showAsPercentage);
    mPercentageBox->setVisible(showAsPercentage);
}

void ParamEditDouble::showDisplay(bool show)
{
    mDisplay->setVisible(show);
}

void ParamEditDouble::paramChangedDouble(QObject *src, QString name, double newParam)
{
    if (src != this && name == mName) {
        mPercentageBox->setValue(round((100.0 * newParam) / mMaxVal));
        mDoubleBox->setValue(newParam * mParam.editorScale);
        updateDisplay(newParam);
    }
}

void ParamEditDouble::percentageChanged(int p)
{
    if (mParam.editAsPercentage) {
        double val = ((double)p / 100.0) * mMaxVal;

        if (mConfig) {
            if (mConfig->getUpdateOnly() != mName) {
                mConfig->setUpdateOnly("");
            }
            mConfig->updateParamDouble(mName, val, this);
        }

        updateDisplay(val);
    }
}

void ParamEditDouble::doubleChanged(double d)
{
    if (!mParam.editAsPercentage) {
        double val = d / mParam.editorScale;

        if (mConfig) {
            if (mConfig->getUpdateOnly() != mName) {
                mConfig->setUpdateOnly("");
            }
            mConfig->updateParamDouble(mName, val, this);
        }

        updateDisplay(val);
    }
}

void ParamEditDouble::on_readButton_clicked()
{
    if (mConfig) {
        mConfig->setUpdateOnly(mName);
        mConfig->requestUpdate();
    }
}

void ParamEditDouble::on_readDefaultButton_clicked()
{
    if (mConfig) {
        mConfig->setUpdateOnly(mName);
        mConfig->requestUpdateDefault();
    }
}

void ParamEditDouble::on_helpButton_clicked()
{
    if (mConfig) {
        HelpDialog::showHelp(this, mConfig, mName);
    }
}

void ParamEditDouble::updateDisplay(double val)
{
    double p = (100.0 * val) / mMaxVal;
    mDisplay->setValue(p);
    mDisplay->setText(tr("%1%2").
                      arg(val * mParam.editorScale, 0, 'f', mParam.editorDecimalsDouble).
                      arg(mParam.suffix));
}
