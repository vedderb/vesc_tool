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

#include "pagefoc.h"
#include "ui_pagefoc.h"

PageFoc::PageFoc(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageFoc)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
}

PageFoc::~PageFoc()
{
    delete ui;
}

VescInterface *PageFoc::vesc() const
{
    return mVesc;
}

void PageFoc::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ui->detectFoc->setVesc(mVesc);
        ui->detectFocHall->setVesc(mVesc);
        ui->detectFocEncoder->setVesc(mVesc);

        ui->generalTab->addParamRow(mVesc->mcConfig(), "foc_sensor_mode");
        ui->generalTab->addParamRow(mVesc->mcConfig(), "foc_motor_r");
        ui->generalTab->addParamRow(mVesc->mcConfig(), "foc_motor_l");
        ui->generalTab->addParamRow(mVesc->mcConfig(), "foc_motor_flux_linkage");
        ui->generalTab->addParamRow(mVesc->mcConfig(), "foc_current_kp");
        ui->generalTab->addParamRow(mVesc->mcConfig(), "foc_current_ki");
        ui->generalTab->addParamRow(mVesc->mcConfig(), "foc_observer_gain");

        ui->sensorlessTab->addParamRow(mVesc->mcConfig(), "foc_openloop_rpm");
        ui->sensorlessTab->addParamRow(mVesc->mcConfig(), "foc_sl_openloop_hyst");
        ui->sensorlessTab->addParamRow(mVesc->mcConfig(), "foc_sl_openloop_time");
        ui->sensorlessTab->addParamRow(mVesc->mcConfig(), "foc_sat_comp");
        ui->sensorlessTab->addParamRow(mVesc->mcConfig(), "foc_temp_comp");
        ui->sensorlessTab->addParamRow(mVesc->mcConfig(), "foc_temp_comp_base_temp");

        ui->hallTab->addParamRow(mVesc->mcConfig(), "foc_sl_erpm");
        ui->hallTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__0");
        ui->hallTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__1");
        ui->hallTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__2");
        ui->hallTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__3");
        ui->hallTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__4");
        ui->hallTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__5");
        ui->hallTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__6");
        ui->hallTab->addParamRow(mVesc->mcConfig(), "foc_hall_table__7");

        ui->encoderTab->addParamRow(mVesc->mcConfig(), "foc_sl_erpm");
        ui->encoderTab->addParamRow(mVesc->mcConfig(), "foc_encoder_offset");
        ui->encoderTab->addParamRow(mVesc->mcConfig(), "foc_encoder_ratio");
        ui->encoderTab->addParamRow(mVesc->mcConfig(), "foc_encoder_inverted");
        ui->encoderTab->addParamRow(mVesc->mcConfig(), "foc_encoder_sin_gain");
        ui->encoderTab->addParamRow(mVesc->mcConfig(), "foc_encoder_sin_offset");
        ui->encoderTab->addParamRow(mVesc->mcConfig(), "foc_encoder_cos_gain");
        ui->encoderTab->addParamRow(mVesc->mcConfig(), "foc_encoder_cos_offset");
        ui->encoderTab->addParamRow(mVesc->mcConfig(), "foc_encoder_sincos_filter_constant");

        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_f_sw");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_dt_us");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_pll_kp");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_pll_ki");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_duty_dowmramp_kp");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_duty_dowmramp_ki");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_sl_d_current_duty");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_sl_d_current_factor");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_sample_v0_v7");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_sample_high_current");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_observer_gain_slow");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "foc_current_filter_const");
    }
}
