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

#include "ppmmap.h"
#include "ui_ppmmap.h"
#include "helpdialog.h"
#include "util.h"
#include <QMessageBox>

PpmMap::PpmMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PpmMap)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
    mResetDone = true;
}

PpmMap::~PpmMap()
{
    delete ui;
}

VescInterface *PpmMap::vesc() const
{
    return mVesc;
}

void PpmMap::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ConfigParam *p = mVesc->appConfig()->getParam("app_ppm_conf.ctrl_type");
        if (p) {
            ui->controlTypeBox->addItems(p->enumNames);
        }

        connect(mVesc->commands(), SIGNAL(decodedPpmReceived(double,double)),
                this, SLOT(decodedPpmReceived(double,double)));
    }
}

void PpmMap::decodedPpmReceived(double value, double last_len)
{
    double min_now = mVesc->appConfig()->getParamDouble("app_ppm_conf.pulse_start");
    double max_now = mVesc->appConfig()->getParamDouble("app_ppm_conf.pulse_end");
    double center_now = mVesc->appConfig()->getParamDouble("app_ppm_conf.pulse_center");

    if (ui->display->isDual()) {
        double p = 0.0;

        if (last_len < center_now) {
            p = util::map(last_len, min_now, center_now, -100.0, 0.0);
        } else {
            p = util::map(last_len, center_now, max_now, 0.0, 100.0);
        }

        ui->display->setValue(p);
        ui->display->setText(tr("%1 ms (%2 %)").
                             arg(last_len, 0, 'f', 4).
                             arg(p, 0, 'f', 1));

        double p2 = value * 100.0;
        ui->displayVesc->setValue(p2);
        ui->displayVesc->setText(tr("%1 ms (%2 %)").
                                 arg(last_len, 0, 'f', 4).
                                 arg(p2, 0, 'f', 1));
    } else {
        double p = util::map(last_len, min_now, max_now, 0.0, 100.0);

        ui->display->setValue(p);
        ui->display->setText(tr("%1 ms (%2 %)").
                             arg(last_len, 0, 'f', 4).
                             arg(p, 0, 'f', 1));

        double p2 = (value + 1.0) * 50.0;
        ui->displayVesc->setValue(p2);
        ui->displayVesc->setText(tr("%1 ms (%2 %)").
                             arg(last_len, 0, 'f', 4).
                             arg(p2, 0, 'f', 1));
    }

    if (mResetDone) {
        mResetDone = false;
        ui->minBox->setValue(last_len);
        ui->maxBox->setValue(last_len);
    } else {
        if (last_len < ui->minBox->value() && last_len > 1e-3) {
            ui->minBox->setValue(last_len);
        }

        if (last_len > ui->maxBox->value()) {
            ui->maxBox->setValue(last_len);
        }
    }

    double range = ui->maxBox->value() - ui->minBox->value();
    double pos = last_len - ui->minBox->value();

    if (pos > (range / 4.0) && pos < ((3.0 * range) / 4.0)) {
        ui->centerBox->setValue(last_len);
    } else {
        ui->centerBox->setValue(range / 2.0 + ui->minBox->value());
    }
}

void PpmMap::on_controlTypeBox_currentIndexChanged(int index)
{
    switch (index) {
    case 0: // Off
    case 1: // Current
    case 3: // Current No Reverse With Brake
    case 4: // Duty Cycle
    case 6: // PID Speed Control
        ui->display->setDual(true);
        ui->displayVesc->setDual(true);
        break;

    case 2: // Current No Reverse
    case 5: // Duty Cycle No Reverse
    case 7: // PID Speed Control No Reverse
        ui->display->setDual(false);
        ui->displayVesc->setDual(false);
        break;

    default:
        break;
    }

    ui->display->setEnabled(index != 0);
}

void PpmMap::on_helpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->infoConfig(), "app_ppm_mapping_help");
    }
}

void PpmMap::on_resetButton_clicked()
{
    mResetDone = true;
    ui->minBox->setValue(0.0);
    ui->maxBox->setValue(0.0);
}

void PpmMap::on_applyButton_clicked()
{
    if (mVesc) {
        if (ui->maxBox->value() > 1e-10) {
            mVesc->appConfig()->updateParamDouble("app_ppm_conf.pulse_start", ui->minBox->value());
            mVesc->appConfig()->updateParamDouble("app_ppm_conf.pulse_end", ui->maxBox->value());
            mVesc->appConfig()->updateParamDouble("app_ppm_conf.pulse_center", ui->centerBox->value());
            mVesc->emitStatusMessage(tr("Start, End and Center Pulselengths Applied"), true);
        } else {
            mVesc->emitStatusMessage(tr("Applying Pulselengths Failed"), false);
            QMessageBox::warning(this,
                                 tr("Apply Pulselengths"),
                                 tr("Please activate RT app data and measure the pulselengths first."));
        }
    }
}
