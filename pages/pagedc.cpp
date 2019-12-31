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

#include "pagedc.h"
#include "ui_pagedc.h"

PageDc::PageDc(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageDc)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
}

PageDc::~PageDc()
{
    delete ui;
}

VescInterface *PageDc::vesc() const
{
    return mVesc;
}

void PageDc::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ui->paramTab->addParamRow(mVesc->mcConfig(), "cc_gain");
        ui->paramTab->addParamRow(mVesc->mcConfig(), "cc_ramp_step_max");
        ui->paramTab->addParamRow(mVesc->mcConfig(), "m_duty_ramp_step");
        ui->paramTab->addParamRow(mVesc->mcConfig(), "m_current_backoff_gain");
        ui->paramTab->addParamRow(mVesc->mcConfig(), "m_dc_f_sw");
        ui->paramTab->addParamRow(mVesc->mcConfig(), "foc_pll_kp");
        ui->paramTab->addParamRow(mVesc->mcConfig(), "foc_pll_ki");
    }
}
