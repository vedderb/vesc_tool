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

#include "pagemotor.h"
#include "ui_pagemotor.h"

PageMotor::PageMotor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageMotor)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
}

PageMotor::~PageMotor()
{
    delete ui;
}

VescInterface *PageMotor::vesc() const
{
    return mVesc;
}

void PageMotor::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ui->dirSetup->setVesc(mVesc);
        ui->batteryCalc->setVesc(mVesc);

        ui->motorTab->addParamRow(mVesc->mcConfig(), "motor_type");
        ui->motorTab->addParamRow(mVesc->mcConfig(), "m_invert_direction");
        ui->motorTab->addParamRow(mVesc->mcConfig(), "m_sensor_port_mode");
        ui->motorTab->addParamRow(mVesc->mcConfig(), "m_encoder_counts");

        ui->currentTab->addRowSeparator(tr("Motor"));
        ui->currentTab->addParamRow(mVesc->mcConfig(), "l_current_max");
        ui->currentTab->addParamRow(mVesc->mcConfig(), "l_current_min");
        ui->currentTab->addParamRow(mVesc->mcConfig(), "l_abs_current_max");
        ui->currentTab->addParamRow(mVesc->mcConfig(), "l_slow_abs_current");
        ui->currentTab->addParamRow(mVesc->mcConfig(), "l_current_max_scale");
        ui->currentTab->addParamRow(mVesc->mcConfig(), "l_current_min_scale");
        ui->currentTab->addRowSeparator(tr("Battery"));
        ui->currentTab->addParamRow(mVesc->mcConfig(), "l_in_current_max");
        ui->currentTab->addParamRow(mVesc->mcConfig(), "l_in_current_min");
        ui->currentTab->addRowSeparator(tr("DRV8301"));
        ui->currentTab->addParamRow(mVesc->mcConfig(), "m_drv8301_oc_mode");
        ui->currentTab->addParamRow(mVesc->mcConfig(), "m_drv8301_oc_adj");

        ui->voltageTab->addParamRow(mVesc->mcConfig(), "l_battery_cut_start");
        ui->voltageTab->addParamRow(mVesc->mcConfig(), "l_battery_cut_end");

        ui->rpmTab->addParamRow(mVesc->mcConfig(), "l_max_erpm");
        ui->rpmTab->addParamRow(mVesc->mcConfig(), "l_min_erpm");
        ui->rpmTab->addParamRow(mVesc->mcConfig(), "l_erpm_start");

        ui->wattageTab->addParamRow(mVesc->mcConfig(), "l_watt_max");
        ui->wattageTab->addParamRow(mVesc->mcConfig(), "l_watt_min");

        ui->tempTab->addRowSeparator(tr("General"));
        ui->tempTab->addParamRow(mVesc->mcConfig(), "l_temp_accel_dec");
        ui->tempTab->addRowSeparator(tr("MOSFET"));
        ui->tempTab->addParamRow(mVesc->mcConfig(), "l_temp_fet_start");
        ui->tempTab->addParamRow(mVesc->mcConfig(), "l_temp_fet_end");
        ui->tempTab->addRowSeparator(tr("Motor"));
        ui->tempTab->addParamRow(mVesc->mcConfig(), "l_temp_motor_start");
        ui->tempTab->addParamRow(mVesc->mcConfig(), "l_temp_motor_end");

        ui->advancedTab->addParamRow(mVesc->mcConfig(), "l_min_vin");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "l_max_vin");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "l_min_duty");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "l_max_duty");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "cc_min_current");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "m_fault_stop_time_ms");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "m_ntc_motor_beta");
        ui->advancedTab->addParamRow(mVesc->mcConfig(), "m_out_aux_mode");
    }
}
