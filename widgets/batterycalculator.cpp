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

#include "batterycalculator.h"
#include "ui_batterycalculator.h"
#include "widgets/helpdialog.h"
#include "utility.h"

BatteryCalculator::BatteryCalculator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BatteryCalculator)
{
    ui->setupUi(this);

    ui->batteryCalcButton->setIcon(Utility::getIcon("icons/apply.png"));
    ui->helpButton->setIcon(Utility::getIcon("icons/Help-96.png"));

    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
    on_batteryCellBox_valueChanged(ui->batteryCellBox->value());
}

BatteryCalculator::~BatteryCalculator()
{
    delete ui;
}

void BatteryCalculator::on_batteryCalcButton_clicked()
{
    if (mVesc) {
        mVesc->mcConfig()->updateParamDouble("l_battery_cut_start", mValStart);
        mVesc->mcConfig()->updateParamDouble("l_battery_cut_end", mValEnd);
    }
}

VescInterface *BatteryCalculator::vesc() const
{
    return mVesc;
}

void BatteryCalculator::setVesc(VescInterface *vesc)
{
    mVesc = vesc;
}

void BatteryCalculator::on_batteryCellBox_valueChanged(int arg1)
{
    (void)arg1;
    calc();
}

void BatteryCalculator::on_batteryTypeBox_currentIndexChanged(int index)
{
    (void)index;
    calc();
}

void BatteryCalculator::calc()
{
    switch (ui->batteryTypeBox->currentIndex()) {
    case 0:
        mValStart = 3.4;
        mValEnd = 3.1;
        break;

    default:
        break;
    }

    mValStart *= (double)ui->batteryCellBox->value();
    mValEnd *= (double)ui->batteryCellBox->value();

    ui->batteryCutoffLabel->setText(tr("[%1, %2]").
                                    arg(mValStart, 0, 'f', 2).
                                    arg(mValEnd, 0, 'f', 2));
}

void BatteryCalculator::on_helpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->infoConfig(), "help_battery_cutoff");
    }
}
