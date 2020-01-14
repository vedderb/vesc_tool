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

#include "pageappgeneral.h"
#include "ui_pageappgeneral.h"

PageAppGeneral::PageAppGeneral(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageAppGeneral)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;
}

PageAppGeneral::~PageAppGeneral()
{
    delete ui;
}

VescInterface *PageAppGeneral::vesc() const
{
    return mVesc;
}

void PageAppGeneral::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        reloadParams();
    }
}

void PageAppGeneral::reloadParams()
{
    if (mVesc) {
        ui->appTab->clearParams();
        ui->appTab->addParamSubgroup(mVesc->appConfig(), "general", "general");
    }
}
