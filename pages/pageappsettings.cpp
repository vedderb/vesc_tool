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

#include "pageappsettings.h"
#include "ui_pageappsettings.h"
#include "setupwizardapp.h"

PageAppSettings::PageAppSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageAppSettings)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;
}

PageAppSettings::~PageAppSettings()
{
    delete ui;
}

VescInterface *PageAppSettings::vesc() const
{
    return mVesc;
}

void PageAppSettings::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        reloadParams();
    }
}

void PageAppSettings::reloadParams()
{
    if (mVesc) {
        ConfigParam *p = mVesc->infoConfig()->getParam("app_setting_description");
        if (p != nullptr) {
            ui->textEdit->setHtml(p->description);
        } else {
            ui->textEdit->setText("App Setting Description not found.");
        }
    }
}

void PageAppSettings::on_appWizardButton_clicked()
{
    if (mVesc) {
        SetupWizardApp w(mVesc, this);
        w.exec();
    }
}
