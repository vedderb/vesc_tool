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

#include "pagemotorsettings.h"
#include "ui_pagemotorsettings.h"
#include "setupwizardmotor.h"
#include "utility.h"

PageMotorSettings::PageMotorSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageMotorSettings)
{

    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;

    ui->motorSetupWizardButton->setIcon(Utility::getIcon("icons/Wizard-96.png"));
}

PageMotorSettings::~PageMotorSettings()
{
    delete ui;
}

VescInterface *PageMotorSettings::vesc() const
{
    return mVesc;
}

void PageMotorSettings::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        reloadParams();
    }
}

void PageMotorSettings::reloadParams()
{
    if (mVesc) {
        ConfigParam *p = mVesc->infoConfig()->getParam("motor_setting_description");
        if (p != nullptr) {
            QRegExp rx("(<img src=)|( width=)");
            QStringList htmls = p->description.split(rx);
            QStringList imgs = {"motor_up", "motor_default" , "motor_down","Upload-96","Data Backup-96","Help-96"};
            QString theme = "<img src=\"" + Utility::getThemePath() + "icons/";
            QString out;
            if(imgs.length() > htmls.length()/2 - 1)
            {
                for(int i =0; i < htmls.length()-1; i+=2){
                    out.append(htmls[i] + theme + imgs[i/2]);
                    out.append(".png\" width=");
                }
                out.append(htmls.last());
                ui->textEdit->setHtml(out);
            }
            else
            {
                ui->textEdit->setHtml(p->description);
            }
        } else {
            ui->textEdit->setText("Motor Setting Description not found.");
        }
    }
}

void PageMotorSettings::on_motorSetupWizardButton_clicked()
{
    emit startFocWizard();
}
