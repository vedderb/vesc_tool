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

#include "detectfoc.h"
#include "ui_detectfoc.h"
#include "helpdialog.h"
#include <QMessageBox>

DetectFoc::DetectFoc(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DetectFoc)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = 0;
    mLastCalcOk = false;
    mAllValuesOk = false;
    mLastOkValuesApplied = false;
    mRunning = false;
    updateColors();
}

DetectFoc::~DetectFoc()
{
    delete ui;
}

void DetectFoc::on_rlButton_clicked()
{
    if (mVesc) {
        if (!mVesc->isPortConnected()) {
            QMessageBox::critical(this,
                                  tr("Connection Error"),
                                  tr("The VESC is not connected. Please connect it."));
            return;
        }

        QMessageBox::information(this,
                              tr("Measure R & L"),
                              tr("When measuring R & L the motor is going to make some noises, but "
                                 "not rotate. These noises are completely normal, so don't unplug "
                                 "anything unless you see smoke."));

        mVesc->commands()->measureRL();
        mRunning = true;
    }
}

void DetectFoc::on_lambdaButton_clicked()
{
    if (mVesc) {
        if (!mVesc->isPortConnected()) {
            QMessageBox::critical(this,
                                  tr("Connection Error"),
                                  tr("The VESC is not connected. Please connect it."));
            return;
        }

        if (ui->resistanceBox->value() < 1e-10) {
            QMessageBox::critical(this,
                                  tr("Measure Error"),
                                  tr("R is 0. Please measure it first."));
            return;
        }

        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(this,
                                     tr("Warning"),
                                     tr("<font color=\"red\">Warning: </font>"
                                        "This is going to spin up the motor. Make "
                                        "sure that nothing is in the way."),
                                     QMessageBox::Ok | QMessageBox::Cancel);

        if (reply == QMessageBox::Ok) {
            mVesc->commands()->measureLinkageOpenloop(ui->currentBox->value(),
                                                      ui->erpmBox->value(),
                                                      ui->dutyBox->value(),
                                                      ui->resistanceBox->value() / 1e3);

            mRunning = true;
        }
    }
}

void DetectFoc::on_helpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->infoConfig(), "help_foc_detect", false);
    }
}

VescInterface *DetectFoc::vesc() const
{
    return mVesc;
}

void DetectFoc::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(motorRLReceived(double,double)),
                this, SLOT(motorRLReceived(double,double)));
        connect(mVesc->commands(), SIGNAL(motorLinkageReceived(double)),
                this, SLOT(motorLinkageReceived(double)));
        connect(mVesc->mcConfig(), SIGNAL(paramChangedDouble(QObject*,QString,double)),
                this, SLOT(paramChangedDouble(QObject*,QString,double)));

        ui->currentBox->setValue(mVesc->mcConfig()->getParamDouble("l_current_max") / 3.0);
    }
}

void DetectFoc::motorRLReceived(double r, double l)
{
    if (!mRunning) {
        return;
    }

    mRunning = false;

    if (r < 1e-9 && l < 1e-9) {
        mVesc->emitStatusMessage(tr("Bad FOC Detection Result Received"), false);
        QMessageBox::critical(this,
                              tr("Bad Detection Result"),
                              tr("Could not measure the motor resistance and inductance."));
    } else {
        mVesc->emitStatusMessage(tr("FOC Detection Result Received"), true);
        ui->resistanceBox->setValue(r * 1e3);
        ui->inductanceBox->setValue(l);
        ui->kpBox->setValue(0.0);
        ui->kiBox->setValue(0.0);
        on_calcKpKiButton_clicked();

        // TODO: Use some rule to calculate time constant?
//        if (l > 50.0) {
//            ui->tcBox->setValue(2000);
//        } else {
//            ui->tcBox->setValue(1000);
//        }
    }

    mLastOkValuesApplied = false;
    updateColors();
}

void DetectFoc::motorLinkageReceived(double flux_linkage)
{
    if (!mRunning) {
        return;
    }

    mRunning = false;

    if (flux_linkage < 1e-9) {
        mVesc->emitStatusMessage(tr("Bad FOC Detection Result Received"), false);
        QMessageBox::critical(this,
                              tr("Bad Detection Result"),
                              tr("Could not measure the flux linkage properly. Adjust the start parameters "
                                 "according to the help text and try again."));
    } else {
        mVesc->emitStatusMessage(tr("FOC Detection Result Received"), true);
        ui->lambdaBox->setValue(flux_linkage * 1e3);
        ui->obsGainBox->setValue(0.0);
        on_calcGainButton_clicked();
    }

    mLastOkValuesApplied = false;
    updateColors();
}

void DetectFoc::paramChangedDouble(QObject *src, QString name, double newParam)
{
    (void)src;

    if (name == "l_current_max") {
        ui->currentBox->setValue(newParam / 3.0);
    }
}

void DetectFoc::on_applyAllButton_clicked()
{
    if (mVesc) {
        double r = ui->resistanceBox->value() / 1e3;
        double l = ui->inductanceBox->value();
        double lambda = ui->lambdaBox->value() / 1e3;

        if (r < 1e-10) {
            QMessageBox::critical(this,
                                  tr("Apply Error"),
                                  tr("R is 0. Please measure it first."));
            return;
        }

        if (l < 1e-10) {
            QMessageBox::critical(this,
                                  tr("Apply Error"),
                                  tr("L is 0. Please measure it first."));
            return;
        }

        if (lambda < 1e-10) {
            QMessageBox::critical(this,
                                  tr("Apply Error"),
                                  tr("\u03BB is 0. Please measure it first."));
            return;
        }

        mVesc->mcConfig()->updateParamDouble("foc_motor_r", r);
        mVesc->mcConfig()->updateParamDouble("foc_motor_l", l / 1e6);
        mVesc->mcConfig()->updateParamDouble("foc_motor_flux_linkage", lambda);

        mVesc->emitStatusMessage(tr("R, L and \u03BB Applied"), true);

        on_calcKpKiButton_clicked();
        on_calcGainButton_clicked();

        if (mLastCalcOk) {
            mVesc->mcConfig()->updateParamDouble("foc_current_kp", ui->kpBox->value());
            mVesc->mcConfig()->updateParamDouble("foc_current_ki", ui->kiBox->value());
            mVesc->mcConfig()->updateParamDouble("foc_observer_gain", ui->obsGainBox->value() * 1e6);
            mVesc->emitStatusMessage(tr("KP, KI and Observer Gain Applied"), true);
            mLastOkValuesApplied = true;
        } else {
            mVesc->emitStatusMessage(tr("Apply Parameters Failed"), false);
        }
    }
}

bool DetectFoc::lastOkValuesApplied() const
{
    return mLastOkValuesApplied;
}

bool DetectFoc::allValuesOk() const
{
    return mAllValuesOk;
}

void DetectFoc::updateColors()
{
    bool r_ok = ui->resistanceBox->value() > 1e-10;
    bool l_ok = ui->inductanceBox->value() > 1e-10;
    bool lambda_ok = ui->lambdaBox->value() > 1e-10;
    bool gain_ok = ui->obsGainBox->value() > 1e-10;
    bool kp_ok = ui->kpBox->value() > 1e-10;
    bool ki_ok = ui->kiBox->value() > 1e-10;

    QString style_red = "color: rgb(255, 255, 255);"
                        "background-color: rgb(150, 0, 0);";

    QString style_green = "color: rgb(255, 255, 255);"
                          "background-color: rgb(0, 150, 0);";

    ui->resistanceBox->setStyleSheet(QString("#resistanceBox {%1}").
                                     arg(r_ok ? style_green : style_red));
    ui->inductanceBox->setStyleSheet(QString("#inductanceBox {%1}").
                                     arg(l_ok ? style_green : style_red));
    ui->lambdaBox->setStyleSheet(QString("#lambdaBox {%1}").
                                 arg(lambda_ok ? style_green : style_red));
    ui->obsGainBox->setStyleSheet(QString("#obsGainBox {%1}").
                                  arg(gain_ok ? style_green : style_red));
    ui->kpBox->setStyleSheet(QString("#kpBox {%1}").
                             arg(kp_ok ? style_green : style_red));
    ui->kiBox->setStyleSheet(QString("#kiBox {%1}").
                             arg(ki_ok ? style_green : style_red));

    mAllValuesOk = r_ok && l_ok && lambda_ok && gain_ok && kp_ok && ki_ok;
}

void DetectFoc::on_calcKpKiButton_clicked()
{
    double r = ui->resistanceBox->value() / 1e3;
    double l = ui->inductanceBox->value();

    mLastCalcOk = false;

    if (r < 1e-10) {
        QMessageBox::critical(this,
                              tr("Calculate Error"),
                              tr("R is 0. Please measure it first."));
        return;
    }

    if (l < 1e-10) {
        QMessageBox::critical(this,
                              tr("Calculate Error"),
                              tr("L is 0. Please measure it first."));
        return;
    }

    l /= 1e6;

    // https://e2e.ti.com/blogs_/b/motordrivecontrol/archive/2015/07/20/teaching-your-pi-controller-to-behave-part-ii
    double tc = ui->tcBox->value();
    double bw = 1.0 / (tc * 1e-6);
    double kp = l * bw;
    double ki = r * bw;

    ui->kpBox->setValue(kp);
    ui->kiBox->setValue(ki);

    mLastOkValuesApplied = false;
    mLastCalcOk = true;
    updateColors();
}

void DetectFoc::on_calcGainButton_clicked()
{
    double lambda = ui->lambdaBox->value() / 1e3;
    mLastCalcOk = false;

    if (lambda < 1e-10) {
        QMessageBox::critical(this,
                              tr("Calculate Error"),
                              tr("\u03BB is 0. Please measure it first."));
        return;
    }

    ui->obsGainBox->setValue(0.001 / (lambda * lambda));

    mLastOkValuesApplied = false;
    mLastCalcOk = true;
    updateColors();
}

void DetectFoc::on_calcApplyLocalButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::information(this,
                                     tr("Calculate"),
                                     tr("This is going to calculate KP, KI and the observer "
                                        "gain from the old settings and the time constant, and "
                                        "apply them. This is useful for manually entering parameters, "
                                        "of for changing time constant without running detection. Do "
                                        "you want to proceed?"),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (reply == QMessageBox::Yes) {
        double tc = ui->tcBox->value();
        double l = mVesc->mcConfig()->getParamDouble("foc_motor_l");
        double r = mVesc->mcConfig()->getParamDouble("foc_motor_r");
        double lambda = mVesc->mcConfig()->getParamDouble("foc_motor_flux_linkage");

        double bw = 1.0 / (tc * 1e-6);
        double kp = l * bw;
        double ki = r * bw;
        double gain = 0.001 / (lambda * lambda);

        mVesc->mcConfig()->updateParamDouble("foc_current_kp", kp);
        mVesc->mcConfig()->updateParamDouble("foc_current_ki", ki);
        mVesc->mcConfig()->updateParamDouble("foc_observer_gain", gain * 1e6);
        mVesc->emitStatusMessage(tr("KP, KI and Observer Gain Applied"), true);
    }
}
