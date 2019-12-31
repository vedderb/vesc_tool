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

#include "pagebldc.h"
#include "ui_pagebldc.h"

PageBldc::PageBldc(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageBldc)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
}

PageBldc::~PageBldc()
{
    delete ui;
}

VescInterface *PageBldc::vesc() const
{
    return mVesc;
}

void PageBldc::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ui->detectBldc->setVesc(mVesc);

        ui->generalTab->addParamRow(mVesc->mcConfig(), "sensor_mode");
        ui->generalTab->addParamRow(mVesc->mcConfig(), "comm_mode");
        ui->generalTab->addParamRow(mVesc->mcConfig(), "cc_startup_boost_duty");

        ui->sensorlessTab->addParamRow(mVesc->mcConfig(), "sl_cycle_int_limit");
        ui->sensorlessTab->addParamRow(mVesc->mcConfig(), "sl_min_erpm");
        ui->sensorlessTab->addParamRow(mVesc->mcConfig(), "sl_min_erpm_cycle_int_limit");
        ui->sensorlessTab->addParamRow(mVesc->mcConfig(), "sl_bemf_coupling_k");

        ui->sensorTab->addParamRow(mVesc->mcConfig(), "hall_sl_erpm");
        for (int i = 0;i < 8;i++) {
            QString str;
            str.sprintf("hall_table__%d", i);
            ui->sensorTab->addParamRow(mVesc->mcConfig(), str);
        }

        ui->advancedTab->addParamRow(mVesc->mcConfig(), "sl_phase_advance_at_br");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "sl_cycle_int_rpm_br");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "l_max_erpm_fbrake");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "pwm_mode");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "cc_gain");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "cc_ramp_step_max");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "m_duty_ramp_step");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "m_current_backoff_gain");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "m_bldc_f_sw_min");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "m_bldc_f_sw_max");
    }
}
