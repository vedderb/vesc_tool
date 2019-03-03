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

#include "detectbldc.h"
#include "ui_detectbldc.h"
#include <QMessageBox>
#include <QDebug>
#include "helpdialog.h"

DetectBldc::DetectBldc(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DetectBldc)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
    mResultReceived = false;
    mRunning = false;
}

DetectBldc::~DetectBldc()
{
    delete ui;
}

void DetectBldc::on_runButton_clicked()
{
    if (mVesc) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(this,
                                     tr("Detect BLDC Parameters"),
                                     tr("This is going to spin up the motor. Make "
                                        "sure that nothing is in the way."),
                                     QMessageBox::Ok | QMessageBox::Cancel);

        if (reply == QMessageBox::Ok) {
            mVesc->commands()->detectMotorParam(ui->currentBox->value(),
                                                ui->erpmBox->value(),
                                                ui->dutyBox->value());

            mRunning = true;
        }
    }
}

void DetectBldc::on_applyButton_clicked()
{
    if (mVesc) {
        if (mResultReceived) {
            mVesc->mcConfig()->updateParamDouble("sl_bemf_coupling_k", mResult.bemf_coupling_k);
            mVesc->mcConfig()->updateParamDouble("sl_cycle_int_limit", mResult.cycle_int_limit);

            if (mResult.hall_res == 0) {
                mVesc->mcConfig()->updateParamInt("hall_table__0", mResult.hall_table.at(0));
                mVesc->mcConfig()->updateParamInt("hall_table__1", mResult.hall_table.at(1));
                mVesc->mcConfig()->updateParamInt("hall_table__2", mResult.hall_table.at(2));
                mVesc->mcConfig()->updateParamInt("hall_table__3", mResult.hall_table.at(3));
                mVesc->mcConfig()->updateParamInt("hall_table__4", mResult.hall_table.at(4));
                mVesc->mcConfig()->updateParamInt("hall_table__5", mResult.hall_table.at(5));
                mVesc->mcConfig()->updateParamInt("hall_table__6", mResult.hall_table.at(6));
                mVesc->mcConfig()->updateParamInt("hall_table__7", mResult.hall_table.at(7));
            }

            mVesc->emitStatusMessage(tr("Detection Result Applied"), true);
        } else {
            QMessageBox::warning(this,
                                 tr("Apply Detection Result"),
                                 tr("Detection result not received. Make sure to run the detection first."));
        }
    }
}

void DetectBldc::on_helpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->infoConfig(), "help_bldc_detect", false);
    }
}

VescInterface *DetectBldc::vesc() const
{
    return mVesc;
}

void DetectBldc::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(bldcDetectReceived(bldc_detect)),
                this, SLOT(bldcDetectReceived(bldc_detect)));
    }
}

void DetectBldc::showEvent(QShowEvent *event)
{
    ui->resultBrowser->setMaximumHeight(ui->gridLayout->geometry().height());
    QWidget::showEvent(event);
}

void DetectBldc::bldcDetectReceived(bldc_detect param)
{
    if (!mRunning) {
        return;
    }

    mRunning = false;

    if (param.cycle_int_limit < 0.01 && param.bemf_coupling_k < 0.01) {
        mVesc->emitStatusMessage(tr("Bad Detection Result Received"), false);
        ui->resultBrowser->setText(tr("Detection failed."));
    } else {
        mVesc->emitStatusMessage(tr("Detection Result Received"), true);

        mResult = param;
        mResultReceived = true;

        if (mResult.bemf_coupling_k > 900) {
            mResult.bemf_coupling_k = 900;
        }

        QString hall_str;
        if (param.hall_res == 0) {
            hall_str.sprintf("Detected hall sensor table:\n"
                             "%i, %i, %i, %i, %i, %i, %i, %i\n",
                             param.hall_table.at(0), param.hall_table.at(1),
                             param.hall_table.at(2), param.hall_table.at(3),
                             param.hall_table.at(4), param.hall_table.at(5),
                             param.hall_table.at(6), param.hall_table.at(7));
        } else if (param.hall_res == -1) {
            hall_str.sprintf("Hall sensor detection failed:\n"
                             "%i, %i, %i, %i, %i, %i, %i, %i\n",
                             param.hall_table.at(0), param.hall_table.at(1),
                             param.hall_table.at(2), param.hall_table.at(3),
                             param.hall_table.at(4), param.hall_table.at(5),
                             param.hall_table.at(6), param.hall_table.at(7));
        } else if (param.hall_res == -2) {
            hall_str.sprintf("WS2811 enabled. Hall sensors cannot be used.\n");
        } else if (param.hall_res == -3) {
            hall_str.sprintf("Encoder enabled. Hall sensors cannot be used.\n");
        } else {
            hall_str.sprintf("Unknown hall error: %d\n", param.hall_res);
        }

        ui->resultBrowser->setText(QString().sprintf("Detection results:\n"
                                                     "Integrator limit: %.2f\n"
                                                     "BEMF Coupling: %.2f\n"
                                                     "%s",
                                                     param.cycle_int_limit,
                                                     param.bemf_coupling_k,
                                                     hall_str.toLocal8Bit().data()));
    }
}
