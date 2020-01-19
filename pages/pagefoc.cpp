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
