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

#include "pageappnrf.h"
#include "ui_pageappnrf.h"

PageAppNrf::PageAppNrf(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageAppNrf)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
}

PageAppNrf::~PageAppNrf()
{
    delete ui;
}

VescInterface *PageAppNrf::vesc() const
{
    return mVesc;
}

void PageAppNrf::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ui->nrfPair->setVesc(mVesc);

        ui->generalTab->addRowSeparator(tr("Radio"));
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_nrf_conf.power");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_nrf_conf.speed");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_nrf_conf.channel");
        ui->generalTab->addRowSeparator(tr("Integrity"));
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_nrf_conf.crc_type");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_nrf_conf.send_crc_ack");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_nrf_conf.retry_delay");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_nrf_conf.retries");
        ui->generalTab->addRowSeparator(tr("Address"));
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_nrf_conf.address__0");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_nrf_conf.address__1");
        ui->generalTab->addParamRow(mVesc->appConfig(), "app_nrf_conf.address__2");
    }
}
