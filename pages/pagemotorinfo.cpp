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

#include "pagemotorinfo.h"
#include "ui_pagemotorinfo.h"
#include "widgets/helpdialog.h"

PageMotorInfo::PageMotorInfo(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageMotorInfo)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;
}

PageMotorInfo::~PageMotorInfo()
{
    delete ui;
}

VescInterface *PageMotorInfo::vesc() const
{
    return mVesc;
}

void PageMotorInfo::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        reloadParams();

        connect(mVesc->mcConfig(), SIGNAL(paramChangedQString(QObject*,QString,QString)),
                this, SLOT(paramChangedQString(QObject*,QString,QString)));
        connect(mVesc->mcConfig(), SIGNAL(savingXml()),
                this, SLOT(savingXml()));
    }
}

void PageMotorInfo::reloadParams()
{
    if (mVesc) {
        ui->setupTab->clearParams();
        ui->generalTab->clearParams();
        ui->qualityTab->clearParams();

        ui->setupTab->addParamSubgroup(mVesc->mcConfig(), "Additional Info", "setup");
        ui->generalTab->addParamSubgroup(mVesc->mcConfig(), "Additional Info", "general");
        ui->qualityTab->addParamSubgroup(mVesc->mcConfig(), "Additional Info", "quality");

        ui->descriptionEdit->document()->setHtml(mVesc->mcConfig()->getParamQString("motor_description"));
        ui->qualityEdit->document()->setHtml(mVesc->mcConfig()->getParamQString("motor_quality_description"));
    }
}

void PageMotorInfo::savingXml()
{
    mVesc->mcConfig()->updateParamString("motor_description",
                                         ui->descriptionEdit->document()->toHtml(),
                                         this);

    mVesc->mcConfig()->updateParamString("motor_quality_description",
                                         ui->qualityEdit->document()->toHtml(),
                                         this);
}

void PageMotorInfo::paramChangedQString(QObject *src, QString name, QString newParam)
{
    if (src != this) {
        if (name == "motor_description") {
            ui->descriptionEdit->document()->setHtml(newParam);
        } else if (name == "motor_quality_description") {
            ui->qualityEdit->document()->setHtml(newParam);
        }
    }
}

void PageMotorInfo::on_descriptionHelpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->mcConfig(), "motor_description");
    }
}
