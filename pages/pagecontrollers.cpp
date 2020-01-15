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

#include "pagecontrollers.h"
#include "ui_pagecontrollers.h"

PageControllers::PageControllers(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageControllers)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;
}

PageControllers::~PageControllers()
{
    delete ui;
}

VescInterface *PageControllers::vesc() const
{
    return mVesc;
}

void PageControllers::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        reloadParams();
    }
}

void PageControllers::reloadParams()
{
    if (mVesc) {
        ui->paramTab->clearParams();
        ui->paramTab->addParamSubgroup(mVesc->mcConfig(), "pid controllers", "general");
    }
}
