/*
    Copyright 2016 - 2020 Benjamin Vedder	benjamin@vedder.se

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
    mVesc = nullptr;
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

        reloadParams();
    }
}

void PageFoc::reloadParams()
{
    if (mVesc) {
        ui->generalTab->clearParams();
        ui->sensorlessTab->clearParams();
        ui->hallTab->clearParams();
        ui->encoderTab->clearParams();
        ui->hfiTab->clearParams();
        ui->advancedTab->clearParams();

        ui->generalTab->addParamSubgroup(mVesc->mcConfig(), "foc", "general");
        ui->sensorlessTab->addParamSubgroup(mVesc->mcConfig(), "foc", "sensorless");
        ui->hallTab->addParamSubgroup(mVesc->mcConfig(), "foc", "hall sensors");
        ui->encoderTab->addParamSubgroup(mVesc->mcConfig(), "foc", "encoder");
        ui->hfiTab->addParamSubgroup(mVesc->mcConfig(), "foc", "hfi");
        ui->advancedTab->addParamSubgroup(mVesc->mcConfig(), "foc", "advanced");
    }
}

void PageFoc::on_openloopOldButton_clicked()
{
    if (mVesc) {
        mVesc->mcConfig()->updateParamDouble("foc_openloop_rpm", 700.0);
        mVesc->mcConfig()->updateParamDouble("foc_openloop_rpm_low", 0.0);
        mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_hyst", 0.1);
        mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time", 0.1);
        mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time_lock", 0.0);
        mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time_ramp", 0.0);
    }
}

void PageFoc::on_openloopGenericButton_clicked()
{
    mVesc->mcConfig()->updateParamDouble("foc_openloop_rpm", 1200.0);
    mVesc->mcConfig()->updateParamDouble("foc_openloop_rpm_low", 0.1);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_hyst", 0.1);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time", 0.05);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time_lock", 0.0);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time_ramp", 0.1);
}

void PageFoc::on_openloopFastStartButton_clicked()
{
    mVesc->mcConfig()->updateParamDouble("foc_openloop_rpm", 1500.0);
    mVesc->mcConfig()->updateParamDouble("foc_openloop_rpm_low", 0.0);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_hyst", 0.1);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time", 0.1);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time_lock", 0.0);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time_ramp", 0.0);
}

void PageFoc::on_openloopPropellerButton_clicked()
{
    mVesc->mcConfig()->updateParamDouble("foc_openloop_rpm", 1500.0);
    mVesc->mcConfig()->updateParamDouble("foc_openloop_rpm_low", 0.1);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_hyst", 0.1);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time", 0.1);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time_lock", 0.1);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time_ramp", 0.1);
}

void PageFoc::on_openloopHeavyInertialButton_clicked()
{
    mVesc->mcConfig()->updateParamDouble("foc_openloop_rpm", 1500.0);
    mVesc->mcConfig()->updateParamDouble("foc_openloop_rpm_low", 0.1);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_hyst", 0.1);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time", 0.2);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time_lock", 0.5);
    mVesc->mcConfig()->updateParamDouble("foc_sl_openloop_time_ramp", 0.5);
}
