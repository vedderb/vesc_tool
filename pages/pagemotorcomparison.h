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
#include <QTimer>
#include "vescinterface.h"
#include "configparams.h"
#include "widgets/qcustomplot.h"
#include "utility.h"

struct MotorDataParams {
    MotorDataParams() {
        gearing = 1.0;
        maxRpm = 50000.0;
        gearingEfficiency = 1.0;
        fwCurrent = 0.0;
        motorNum = 1.0;
        tempInc = 0.0;
        mtpa = false;
    }

    double gearing;
    double maxRpm;
    double gearingEfficiency;
    double fwCurrent;
    double motorNum;
    double tempInc;
    bool mtpa;
};

struct MotorData {
    Q_GADGET

    Q_PROPERTY(double torque_out MEMBER torque_out)
    Q_PROPERTY(double torque_motor_shaft MEMBER torque_motor_shaft)
    Q_PROPERTY(double rpm_out MEMBER rpm_out)
    Q_PROPERTY(double rpm_motor_shaft MEMBER rpm_motor_shaft)
    Q_PROPERTY(double erpm MEMBER erpm)
    Q_PROPERTY(double iq MEMBER iq)
    Q_PROPERTY(double id MEMBER id)
    Q_PROPERTY(double i_mag MEMBER i_mag)
    Q_PROPERTY(double loss_motor_res MEMBER loss_motor_res)
    Q_PROPERTY(double loss_motor_other MEMBER loss_motor_other)
    Q_PROPERTY(double loss_motor_tot MEMBER loss_motor_tot)
    Q_PROPERTY(double loss_gearing MEMBER loss_gearing)
    Q_PROPERTY(double loss_tot MEMBER loss_tot)
    Q_PROPERTY(double p_out MEMBER p_out)
    Q_PROPERTY(double p_in MEMBER p_in)
    Q_PROPERTY(double efficiency MEMBER efficiency)
    Q_PROPERTY(double vq MEMBER vq)
    Q_PROPERTY(double vd MEMBER vd)
    Q_PROPERTY(double vbus_min MEMBER vbus_min)
    Q_PROPERTY(double km_h MEMBER km_h)
    Q_PROPERTY(double mph MEMBER mph)
    Q_PROPERTY(double wh_km MEMBER wh_km)
    Q_PROPERTY(double wh_mi MEMBER wh_mi)
    Q_PROPERTY(double kv_bldc MEMBER kv_bldc)
    Q_PROPERTY(double kv_bldc_noload MEMBER kv_bldc_noload)

    Q_PROPERTY(double extraVal MEMBER extraVal)
    Q_PROPERTY(double extraVal2 MEMBER extraVal2)
    Q_PROPERTY(double extraVal3 MEMBER extraVal3)
    Q_PROPERTY(double extraVal4 MEMBER extraVal4)

public:
    MotorData() {
        config = nullptr;

        torque_out = 0.0;
        torque_motor_shaft = 0.0;
        rpm_out = 0.0;
        rpm_motor_shaft = 0.0;
        erpm = 0.0;
        iq = 0.0;
        id = 0.0;
        i_mag = 0.0;
        loss_motor_res = 0.0;
        loss_motor_other = 0.0;
        loss_motor_tot = 0.0;
        loss_gearing = 0.0;
        loss_tot = 0.0;
        p_out = 0.0;
        p_in = 0.0;
        efficiency = 0.0;
        vq = 0.0;
        vd = 0.0;
        vbus_min = 0.0;
        km_h = 0.0;
        mph = 0.0;
        wh_km = 0.0;
        wh_mi = 0.0;
        kv_bldc = 0.0;
        kv_bldc_noload = 0.0;

        extraVal = 0.0;
        extraVal2 = 0.0;
        extraVal3 = 0.0;
        extraVal4 = 0.0;
    }

    MotorData(ConfigParams *cfg, MotorDataParams prm) : MotorData() {
        configure(cfg, prm);
    }

    bool operator==(const MotorData &other) const {
        (void)other;
        // compare members
        return true;
    }

    bool operator!=(MotorData const &other) const {
        return !(*this == other);
    }

    void configure(ConfigParams *cfg, MotorDataParams prm) {
        config = cfg;
        params = prm;
    }

    Q_INVOKABLE bool updateRpmVBusFW(double torque, double rpm, double vbus) {
        double fw_max = params.fwCurrent;
        params.fwCurrent = 0.0;

        if (!update(rpm, torque)) {
            params.fwCurrent = fw_max;
            return false;
        }

        if (vbus_min < vbus) {
            params.fwCurrent = fw_max;
            return true;
        }

        double vbus_lower = vbus_min;
        params.fwCurrent = fw_max;
        update(rpm, torque);
        double vbus_upper = vbus_min;

        if (vbus_upper > vbus) {
            return updateRpmVBus(rpm, vbus);
        }

        params.fwCurrent = Utility::map(vbus, vbus_lower, vbus_upper, 0.0, fw_max);

        for (int i = 0;i < 20;i++) {
            if (!update(rpm, torque)) {
                params.fwCurrent = fw_max;
                return false;
            }

            params.fwCurrent *= vbus_min / vbus;

            if (params.fwCurrent > fw_max) {
                params.fwCurrent = fw_max;
            }

            if (params.fwCurrent < 0.0) {
                params.fwCurrent = 0.0;
            }
        }

        params.fwCurrent = fw_max;
        return true;
    }

    Q_INVOKABLE bool updateRpmVBus(double rpm, double vbus, double torque_guess = 5.0) {
        for (int i = 0;i < 20;i++) {
            if (!update(rpm, torque_guess)) {
                return false;
            }

            torque_guess *= vbus / vbus_min;
        }

        return true;
    }

    Q_INVOKABLE bool updateTorqueVBusFW(double torque, double rpm, double vbus) {
        double fw_max = params.fwCurrent;
        params.fwCurrent = 0.0;

        if (!update(rpm, torque)) {
            params.fwCurrent = fw_max;
            return false;
        }

        if (vbus_min < vbus) {
            params.fwCurrent = fw_max;
            return true;
        }

        double vbus_lower = vbus_min;
        params.fwCurrent = fw_max;
        update(rpm, torque);
        double vbus_upper = vbus_min;

        if (vbus_upper > vbus) {
            return updateTorqueVBus(torque, vbus);
        }

        params.fwCurrent = Utility::map(vbus, vbus_lower, vbus_upper, 0.0, fw_max);

        for (int i = 0;i < 20;i++) {
            if (!update(rpm, torque)) {
                params.fwCurrent = fw_max;
                return false;
            }

            params.fwCurrent *= vbus_min / vbus;

            if (params.fwCurrent > fw_max) {
                params.fwCurrent = fw_max;
            }

            if (params.fwCurrent < 0.0) {
                params.fwCurrent = 0.0;
            }
        }

        params.fwCurrent = fw_max;
        return true;
    }

    Q_INVOKABLE bool updateTorqueVBus(double torque, double vbus, double rpm_guess = 1000.0) {
        for (int i = 0;i < 20;i++) {
            if (!update(rpm_guess, torque)) {
                return false;
            }

            rpm_guess *= vbus / vbus_min;
        }

        return true;
    }

    Q_INVOKABLE bool update(double rpm, double torque) {
        if (config == nullptr) {
            return false;
        }

        // See https://www.mathworks.com/help/physmod/sps/ref/pmsm.html
        // for the motor equations

        double r = config->getParamDouble("foc_motor_r");
        double l = config->getParamDouble("foc_motor_l");
        double ld_lq_diff = config->getParamDouble("foc_motor_ld_lq_diff");
        double lq = l + ld_lq_diff / 2.0;
        double ld = l - ld_lq_diff / 2.0;
        double lambda = config->getParamDouble("foc_motor_flux_linkage");
        double i_nl = config->getParamDouble("si_motor_nl_current");
        double pole_pairs = double(config->getParamInt("si_motor_poles")) / 2.0;
        double wheel_diam = config->getParamDouble("si_wheel_diameter");

        r += r * 0.00386 * (params.tempInc);

        torque_out = torque;
        rpm_out = rpm;
        rpm_motor_shaft = rpm * params.gearing;
        erpm = rpm_motor_shaft * pole_pairs;
        torque_motor_shaft = torque / (params.gearing * params.motorNum * params.gearingEfficiency);

        double rps_out = rpm * 2.0 * M_PI / 60.0;
        double rps_motor = rps_out * params.gearing;
        double e_rps = rps_motor * pole_pairs;
        double t_nl = SIGN(rpm) * (3.0 / 2.0) * i_nl * lambda * pole_pairs; // No-load torque from core losses

        iq = ((torque_motor_shaft + t_nl) * (2.0 / 3.0) / (lambda * pole_pairs));
        id = -params.fwCurrent;

        // Iterate taking motor saliency into account to get the current that produces the desired torque
        double torque_motor_shaft_updated = torque_motor_shaft;
        double iq_adj = 0.0;
        for (int i = 0;i < 30;i++) {
            iq -= 0.2 * iq * (torque_motor_shaft_updated - torque_motor_shaft) /
                    (SIGN(torque_motor_shaft_updated) * fmax(fabs(torque_motor_shaft_updated), 1.0));
            iq += iq_adj;

            // See https://github.com/vedderb/bldc/pull/179
            if (params.mtpa && fabs(ld_lq_diff) > 1e-9) {
                id = (lambda - sqrt(SQ(lambda) + 8.0 * SQ(ld_lq_diff) * SQ(iq))) / (4.0 * ld_lq_diff);
                iq_adj = iq - SIGN(iq) * sqrt(SQ(iq) - SQ(id));
                iq = SIGN(iq) * sqrt(SQ(iq) - SQ(id));
                id -= params.fwCurrent;
            }

            torque_motor_shaft_updated = (3.0 / 2.0) * pole_pairs * (iq * lambda + iq * id * (ld - lq)) - t_nl;
        }

        torque_motor_shaft = torque_motor_shaft_updated;
        torque_out = torque_motor_shaft * params.gearing * params.motorNum * params.gearingEfficiency;

        i_mag = sqrt(iq * iq + id * id);
        loss_motor_res = i_mag * i_mag * r * (3.0 / 2.0) * params.motorNum;
        loss_motor_other = rps_motor * t_nl * params.motorNum;
        loss_motor_tot = loss_motor_res + loss_motor_other;
        loss_gearing = torque_motor_shaft * (1.0 - params.gearingEfficiency) * rps_motor;
        loss_tot = loss_motor_tot + loss_gearing;
        p_out = rps_motor * torque_motor_shaft * params.motorNum * params.gearingEfficiency;
        p_in = rps_motor * torque_motor_shaft * params.motorNum + loss_motor_tot;

        efficiency = fmin(fabs(p_out), fabs(p_in)) / fmax(fabs(p_out), fabs(p_in));

        vq = r * iq + e_rps * (lambda + id * ld);
        vd = r * id - e_rps * lq * iq;
        vbus_min = (3.0 / 2.0) * sqrt(vq * vq + vd * vd) / (sqrt(3.0) / 2.0) / 0.95;

        km_h = 3.6 * M_PI * wheel_diam * rpm_out / 60.0;
        mph = km_h * 0.621371192;

        wh_km = p_in / km_h;
        wh_mi = p_in / mph;

        kv_bldc = rpm_motor_shaft / (vbus_min * (sqrt(3.0) / 2.0));
        kv_bldc_noload = (60.0 * 0.95) / (lambda * (3.0 / 2.0) * M_PI * 2.0 * pole_pairs);

        return true;
    }

    ConfigParams *config;
    MotorDataParams params;

    double torque_out;
    double torque_motor_shaft;
    double rpm_out;
    double rpm_motor_shaft;
    double erpm;
    double iq;
    double id;
    double i_mag;
    double loss_motor_res;
    double loss_motor_other;
    double loss_motor_tot;
    double loss_gearing;
    double loss_tot;
    double p_out;
    double p_in;
    double efficiency;
    double vq;
    double vd;
    double vbus_min;
    double km_h;
    double mph;
    double wh_km;
    double wh_mi;
    double kv_bldc;
    double kv_bldc_noload;

    double extraVal;
    double extraVal2;
    double extraVal3;
    double extraVal4;
};

Q_DECLARE_METATYPE(MotorData)

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

signals:
    void reloadQml(QString str);

private slots:
    void on_testRunButton_clicked();
    void on_qmlChooseButton_clicked();
    void on_qmlRunButton_clicked();
    void on_qmlStopButton_clicked();

    void qmlTestChanged();
    void qmlNamesUpdated();

private:
    struct QmlParams {
        QmlParams() {
            rpmM1 = 0.0;
            torqueM1 = 0.0;
            extraM1 = 0.0;
            extraM1_2 = 0.0;
            extraM1_3 = 0.0;
            extraM1_4 = 0.0;

            rpmM2 = 0.0;
            torqueM2 = 0.0;
            extraM2 = 0.0;
            extraM2_2 = 0.0;
            extraM2_3 = 0.0;
            extraM2_4 = 0.0;
        }

        double rpmM1;
        double torqueM1;
        double extraM1;
        double extraM1_2;
        double extraM1_3;
        double extraM1_4;

        double rpmM2;
        double torqueM2;
        double extraM2;
        double extraM2_2;
        double extraM2_3;
        double extraM2_4;
    };

    Ui::PageMotorComparison *ui;
    VescInterface *mVesc;
    void settingChanged();
    bool reloadConfigs();
    void updateDataAndPlot(double posx, double yMin, double yMax);
    MotorDataParams getParamsUi(int motor);
    QmlParams getQmlParam(double progress);
    bool qmlUpdateNames();
    QString getQmlXName();
    double getQmlXMin();
    double getQmlXMax();
    void setQmlProgressSelected(double progress);
    void setQmlMotorParams();

    ConfigParams mM1Config;
    ConfigParams mM2Config;
    bool mM1ConfigLoaded;
    bool mM2ConfigLoaded;
    QCPCurve *mVerticalLine;
    double mVerticalLinePosLast;
    QPair<double, double> mVerticalLineYLast;
    bool mRunDone;
    Utility mUtil;
    QTimer *mSettingUpdateTimer;
    bool mSettingUpdateRequired;

    bool mQmlXNameOk;
    bool mQmlXMinOk;
    bool mQmlXMaxOk;
    bool mQmlProgressOk;
    bool mQmlMotorParamsOk;
    bool mQmlReadNamesDone;
};

#endif // PAGEMOTORCOMPARISON_H
