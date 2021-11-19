/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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

#ifndef PAGEMOTORCOMPARISON_H
#define PAGEMOTORCOMPARISON_H

#include <QWidget>
#include <QPair>
#include "vescinterface.h"
#include "configparams.h"
#include "widgets/qcustomplot.h"

namespace Ui {
class PageMotorComparison;
}

class PageMotorComparison : public QWidget
{
    Q_OBJECT

public:
    explicit PageMotorComparison(QWidget *parent = nullptr);
    ~PageMotorComparison();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

private slots:
    void on_testRunButton_clicked();

private:
    Ui::PageMotorComparison *ui;
    VescInterface *mVesc;
    void settingChanged();
    bool reloadConfigs();
    void updateDataAndPlot(double posx, double yMin, double yMax);

    ConfigParams mM1Config;
    ConfigParams mM2Config;
    bool mM1ConfigLoaded;
    bool mM2ConfigLoaded;
    QCPCurve *mVerticalLine;
    double mVerticalLinePosLast;
    QPair<double, double> mVerticalLineYLast;
    bool mRunDone;

    struct MotorData {
        MotorData() {
            torque_out = 0.0;
            torque_motor_shaft = 0.0;
            rpm_out = 0.0;
            rpm_motor_shaft = 0.0;
            erpm = 0.0;
            iq = 0.0;
            loss_motor_res = 0.0;
            loss_motor_other = 0.0;
            loss_motor_tot = 0.0;
            loss_gearing = 0.0;
            p_out = 0.0;
            p_in = 0.0;
            efficiency = 0.0;
            vq = 0.0;
            vd = 0.0;
            vbus_min = 0.0;
        }

        void update(ConfigParams &config, double rpm, double torque,
                    double gearing, double gear_eff, double motors, double temp_inc) {

            double r = config.getParamDouble("foc_motor_r");
            double l = config.getParamDouble("foc_motor_l");
            double lambda = config.getParamDouble("foc_motor_flux_linkage");
            double i_nl = config.getParamDouble("si_motor_nl_current");
            double poles_pairs = double(config.getParamInt("si_motor_poles")) / 2.0;

            r += r * 0.00386 * (temp_inc);

            torque_out = torque;
            rpm_out = rpm;
            rpm_motor_shaft = rpm * gearing;
            erpm = rpm_motor_shaft * poles_pairs;
            torque_motor_shaft = torque / (gearing * motors * gear_eff);

            double rps_out = rpm * 2.0 * M_PI / 60.0;
            double rps_motor = rps_out * gearing;
            double e_rps = rps_motor * poles_pairs;

            iq = (torque_motor_shaft / (lambda * poles_pairs)) + i_nl;
            loss_motor_res = iq * iq * r * 1.5 * motors;
            loss_motor_other = e_rps * lambda * i_nl * motors;
            loss_motor_tot = loss_motor_res + loss_motor_other;
            loss_gearing = torque_motor_shaft * (1.0 - gear_eff) * rps_motor;
            p_out = rps_out * torque_out;
            p_in = rps_motor * torque_motor_shaft * motors + loss_motor_tot;
            efficiency = p_out / p_in;
            vq = r * iq + e_rps * lambda;
            vd = -e_rps * l * iq;
            vbus_min = (3.0 / 2.0) * sqrt(vq * vq + vd * vd) / (sqrt(3.0) / 2.0) / 0.95;
        }

        double torque_out;
        double torque_motor_shaft;
        double rpm_out;
        double rpm_motor_shaft;
        double erpm;
        double iq;
        double loss_motor_res;
        double loss_motor_other;
        double loss_motor_tot;
        double loss_gearing;
        double p_out;
        double p_in;
        double efficiency;
        double vq;
        double vd;
        double vbus_min;
    };
};

#endif // PAGEMOTORCOMPARISON_H
