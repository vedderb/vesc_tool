/*
    Copyright 2021 - 2026 Benjamin Vedder  benjamin@vedder.se

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

#include "motorcomparisonhelper.h"
#include "vescinterface.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

MotorComparisonHelper::MotorComparisonHelper(QObject *parent)
    : QObject(parent), mVesc(nullptr), mM1Loaded(false), mM2Loaded(false)
{
}

void MotorComparisonHelper::setVescInterface(QObject *vescIf)
{
    mVesc = qobject_cast<VescInterface*>(vescIf);
}

bool MotorComparisonHelper::loadM1ConfigLocal()
{
    if (!mVesc) return false;
    mM1Config = *mVesc->mcConfig();
    mM1Loaded = true;
    return true;
}

bool MotorComparisonHelper::loadM1ConfigFile(const QString &path)
{
    if (!mVesc) return false;
    mM1Config = *mVesc->mcConfig();
    if (!mM1Config.loadXml(path, "MCConfiguration")) {
        return false;
    }
    mM1Loaded = true;
    return true;
}

bool MotorComparisonHelper::loadM2ConfigLocal()
{
    if (!mVesc) return false;
    mM2Config = *mVesc->mcConfig();
    mM2Loaded = true;
    return true;
}

bool MotorComparisonHelper::loadM2ConfigFile(const QString &path)
{
    if (!mVesc) return false;
    mM2Config = *mVesc->mcConfig();
    if (!mM2Config.loadXml(path, "MCConfiguration")) {
        return false;
    }
    mM2Loaded = true;
    return true;
}

MotorDataParams MotorComparisonHelper::paramsFromMap(const QVariantMap &m) const
{
    MotorDataParams p;
    p.gearing = m.value("gearing", 1.0).toDouble();
    p.maxRpm = m.value("maxRpm", 50000.0).toDouble();
    p.gearingEfficiency = m.value("gearingEfficiency", 1.0).toDouble();
    p.fwCurrent = m.value("fwCurrent", 0.0).toDouble();
    p.motorNum = m.value("motorNum", 1.0).toDouble();
    p.tempInc = m.value("tempInc", 0.0).toDouble();
    p.mtpa = m.value("mtpa", false).toBool();
    return p;
}

QVariantMap MotorComparisonHelper::motorDataToMap(const MotorData &md) const
{
    QVariantMap r;
    r["efficiency"] = md.efficiency * 100.0;
    r["loss_motor_tot"] = md.loss_motor_tot;
    r["loss_motor_res"] = md.loss_motor_res;
    r["loss_motor_other"] = md.loss_motor_other;
    r["loss_gearing"] = md.loss_gearing;
    r["loss_tot"] = md.loss_tot;
    r["iq"] = md.iq;
    r["id"] = md.id;
    r["i_mag"] = md.i_mag;
    r["p_in"] = md.p_in;
    r["p_out"] = md.p_out;
    r["vq"] = md.vq;
    r["vd"] = md.vd;
    r["vbus_min"] = md.vbus_min;
    r["torque_out"] = md.torque_out;
    r["torque_motor_shaft"] = md.torque_motor_shaft;
    r["rpm_out"] = md.rpm_out;
    r["rpm_motor_shaft"] = md.rpm_motor_shaft;
    r["erpm"] = md.erpm;
    r["km_h"] = md.km_h;
    r["mph"] = md.mph;
    r["wh_km"] = md.wh_km;
    r["wh_mi"] = md.wh_mi;
    r["kv_bldc"] = md.kv_bldc;
    r["kv_bldc_noload"] = md.kv_bldc_noload;
    return r;
}

double MotorComparisonHelper::extractField(const MotorData &md, int index, double scale) const
{
    switch (index) {
    case 0: return md.efficiency * 100.0 * scale;
    case 1: return md.loss_motor_tot * scale;
    case 2: return md.loss_motor_res * scale;
    case 3: return md.loss_motor_other * scale;
    case 4: return md.loss_gearing * scale;
    case 5: return md.loss_tot * scale;
    case 6: return md.iq * scale;
    case 7: return md.id * scale;
    case 8: return md.i_mag * scale;
    case 9: return md.p_in * scale;
    case 10: return md.p_out * scale;
    case 11: return md.vq * scale;
    case 12: return md.vd * scale;
    case 13: return md.vbus_min * scale;
    case 14: return md.torque_out * scale;
    case 15: return md.torque_motor_shaft * scale;
    case 16: return md.rpm_out * scale;
    case 17: return md.rpm_motor_shaft * scale;
    case 18: return md.extraVal * scale;
    case 19: return md.extraVal2 * scale;
    case 20: return md.extraVal3 * scale;
    case 21: return md.extraVal4 * scale;
    case 22: return md.erpm * scale;
    case 23: return md.km_h * scale;
    default: return 0.0;
    }
}

QVariantList MotorComparisonHelper::dataItemNames() const
{
    return QVariantList {
        "Efficiency", "Mot Loss Tot", "Mot Loss Res", "Mot Loss Other",
        "Gearing Loss", "Total Losses", "iq (per motor)", "id (per motor)",
        "i_abs (per motor)", "Power In", "Power Out", "Vq", "Vd", "VBus Min",
        "Torque Out", "Torque Shaft", "RPM Out", "RPM Shaft",
        "ExtraVal", "ExtraVal2", "ExtraVal3", "ExtraVal4", "ERPM",
        "km/h", "mph", "wh/km", "wh/mi", "KV (BLDC)", "KV Noload (BLDC)"
    };
}

// Helper: build the sweep result for one motor
static void appendSeries(MotorData &md, const QVariantList &selected,
                          QMap<int, QVariantList> &seriesData,
                          const MotorComparisonHelper *helper)
{
    for (const auto &sel : selected) {
        QVariantMap s = sel.toMap();
        int idx = s.value("index").toInt();
        double scale = s.value("scale", 1.0).toDouble();
        double val = helper->extractField(md, idx, scale);
        seriesData[idx].append(val);
    }
}

static QVariantMap buildResult(const QVector<double> &xAxis,
                                const QVariantList &m1Selected,
                                const QVariantList &m2Selected,
                                const QVector<MotorData> &m1Data,
                                const QVector<MotorData> &m2Data,
                                const QString &xLabel,
                                const MotorComparisonHelper *helper,
                                const QString &m1Name = "Motor A",
                                const QString &m2Name = "Motor B")
{
    QVariantList xList;
    for (double v : xAxis) xList.append(v);

    QVariantList series;

    // Motor A series
    for (const auto &sel : m1Selected) {
        QVariantMap s = sel.toMap();
        int idx = s.value("index").toInt();
        double scale = s.value("scale", 1.0).toDouble();
        QString name = s.value("name").toString();

        QVariantList data;
        for (const auto &md : m1Data) {
            data.append(helper->extractField(md, idx, scale));
        }

        QVariantMap entry;
        entry["name"] = m1Name + " " + name;
        entry["data"] = data;
        entry["motor"] = 1;
        series.append(entry);
    }

    // Motor B series
    for (const auto &sel : m2Selected) {
        QVariantMap s = sel.toMap();
        int idx = s.value("index").toInt();
        double scale = s.value("scale", 1.0).toDouble();
        QString name = s.value("name").toString();

        QVariantList data;
        for (const auto &md : m2Data) {
            data.append(helper->extractField(md, idx, scale));
        }

        QVariantMap entry;
        entry["name"] = m2Name + " " + name;
        entry["data"] = data;
        entry["motor"] = 2;
        series.append(entry);
    }

    QVariantMap result;
    result["xAxis"] = xList;
    result["series"] = series;
    result["xLabel"] = xLabel;
    return result;
}

QVariantMap MotorComparisonHelper::runTorqueSweep(double rpm, double torqueMax, int points, bool negative,
                                                    const QVariantMap &m1Params, const QVariantMap &m2Params,
                                                    const QVariantList &m1Selected, const QVariantList &m2Selected)
{
    if (!mM1Loaded) loadM1ConfigLocal();
    if (!mM2Loaded) loadM2ConfigLocal();

    auto p1 = paramsFromMap(m1Params);
    auto p2 = paramsFromMap(m2Params);
    double tStart = negative ? -torqueMax : torqueMax / points;

    QVector<double> xAxis;
    QVector<MotorData> d1, d2;

    for (double t = tStart; t < torqueMax; t += torqueMax / points) {
        MotorData md1; md1.configure(&mM1Config, p1); md1.update(rpm, t);
        MotorData md2; md2.configure(&mM2Config, p2); md2.update(rpm, t);
        xAxis.append(t);
        d1.append(md1);
        d2.append(md2);
        if (md1.rpm_motor_shaft >= p1.maxRpm || md2.rpm_motor_shaft >= p2.maxRpm) break;
    }

    return buildResult(xAxis, m1Selected, m2Selected, d1, d2, "Torque (Nm)", this);
}

QVariantMap MotorComparisonHelper::runRpmSweep(double torque, double rpmMax, int points, bool negative,
                                                 const QVariantMap &m1Params, const QVariantMap &m2Params,
                                                 const QVariantList &m1Selected, const QVariantList &m2Selected)
{
    if (!mM1Loaded) loadM1ConfigLocal();
    if (!mM2Loaded) loadM2ConfigLocal();

    auto p1 = paramsFromMap(m1Params);
    auto p2 = paramsFromMap(m2Params);
    double rStart = negative ? -rpmMax : rpmMax / points;

    QVector<double> xAxis;
    QVector<MotorData> d1, d2;

    for (double r = rStart; r < rpmMax; r += rpmMax / points) {
        MotorData md1; md1.configure(&mM1Config, p1); md1.update(r, torque);
        MotorData md2; md2.configure(&mM2Config, p2); md2.update(r, torque);
        xAxis.append(r);
        d1.append(md1);
        d2.append(md2);
        if (md1.rpm_motor_shaft >= p1.maxRpm || md2.rpm_motor_shaft >= p2.maxRpm) break;
    }

    return buildResult(xAxis, m1Selected, m2Selected, d1, d2, "RPM", this);
}

QVariantMap MotorComparisonHelper::runRpmPowerSweep(double power, double rpmStart, double rpmMax, int points,
                                                      const QVariantMap &m1Params, const QVariantMap &m2Params,
                                                      const QVariantList &m1Selected, const QVariantList &m2Selected)
{
    if (!mM1Loaded) loadM1ConfigLocal();
    if (!mM2Loaded) loadM2ConfigLocal();

    auto p1 = paramsFromMap(m1Params);
    auto p2 = paramsFromMap(m2Params);

    QVector<double> xAxis;
    QVector<MotorData> d1, d2;

    for (double r = rpmStart; r < rpmMax; r += rpmMax / points) {
        double rps = r * 2.0 * M_PI / 60.0;
        double torque = power / rps;
        MotorData md1; md1.configure(&mM1Config, p1); md1.update(r, torque);
        MotorData md2; md2.configure(&mM2Config, p2); md2.update(r, torque);
        xAxis.append(r);
        d1.append(md1);
        d2.append(md2);
        if (md1.rpm_motor_shaft >= p1.maxRpm || md2.rpm_motor_shaft >= p2.maxRpm) break;
    }

    return buildResult(xAxis, m1Selected, m2Selected, d1, d2, "RPM", this);
}

QVariantMap MotorComparisonHelper::runExpSweep(double power, double rpmStart, double rpmMax, double exponent,
                                                 double baseTorque, int points,
                                                 const QVariantMap &m1Params, const QVariantMap &m2Params,
                                                 const QVariantList &m1Selected, const QVariantList &m2Selected)
{
    if (!mM1Loaded) loadM1ConfigLocal();
    if (!mM2Loaded) loadM2ConfigLocal();

    auto p1 = paramsFromMap(m1Params);
    auto p2 = paramsFromMap(m2Params);
    double p_max_const = power / pow(rpmMax - rpmStart, exponent);

    QVector<double> xAxis;
    QVector<MotorData> d1, d2;

    for (double r = rpmMax / points; r < rpmMax; r += rpmMax / points) {
        double rps = r * 2.0 * M_PI / 60.0;
        double pw = p_max_const * pow(r > rpmStart ? (r - rpmStart) : 0.0, exponent);
        double torque = pw / rps + baseTorque;
        MotorData md1; md1.configure(&mM1Config, p1); md1.update(r, torque);
        MotorData md2; md2.configure(&mM2Config, p2); md2.update(r, torque);
        xAxis.append(r);
        d1.append(md1);
        d2.append(md2);
        if (md1.rpm_motor_shaft >= p1.maxRpm || md2.rpm_motor_shaft >= p2.maxRpm) break;
    }

    return buildResult(xAxis, m1Selected, m2Selected, d1, d2, "RPM", this);
}

QVariantMap MotorComparisonHelper::runVbusSweep(double torqueMax, double vbus, int points, bool negative,
                                                  const QVariantMap &m1Params, const QVariantMap &m2Params,
                                                  const QVariantList &m1Selected, const QVariantList &m2Selected)
{
    if (!mM1Loaded) loadM1ConfigLocal();
    if (!mM2Loaded) loadM2ConfigLocal();

    auto p1 = paramsFromMap(m1Params);
    auto p2 = paramsFromMap(m2Params);
    double tStart = negative ? -torqueMax : torqueMax / points;

    QVector<double> xAxis;
    QVector<MotorData> d1, d2;

    for (double t = tStart; t < torqueMax; t += torqueMax / points) {
        MotorData md1; md1.configure(&mM1Config, p1); md1.updateTorqueVBus(t, vbus);
        MotorData md2; md2.configure(&mM2Config, p2); md2.updateTorqueVBus(t, vbus);
        xAxis.append(t);
        d1.append(md1);
        d2.append(md2);
        if (md1.rpm_motor_shaft >= p1.maxRpm || md2.rpm_motor_shaft >= p2.maxRpm) break;
    }

    return buildResult(xAxis, m1Selected, m2Selected, d1, d2, "Torque (Nm)", this);
}

QVariantMap MotorComparisonHelper::runVbFwSweep(double torqueMax, double rpm, double vbus, int points, bool negative,
                                                  const QVariantMap &m1Params, const QVariantMap &m2Params,
                                                  const QVariantList &m1Selected, const QVariantList &m2Selected)
{
    if (!mM1Loaded) loadM1ConfigLocal();
    if (!mM2Loaded) loadM2ConfigLocal();

    auto p1 = paramsFromMap(m1Params);
    auto p2 = paramsFromMap(m2Params);
    double tStart = negative ? -torqueMax : torqueMax / points;

    QVector<double> xAxis;
    QVector<MotorData> d1, d2;

    for (double t = tStart; t < torqueMax; t += torqueMax / points) {
        MotorData md1; md1.configure(&mM1Config, p1); md1.updateTorqueVBusFW(t, rpm, vbus);
        MotorData md2; md2.configure(&mM2Config, p2); md2.updateTorqueVBusFW(t, rpm, vbus);
        xAxis.append(t);
        d1.append(md1);
        d2.append(md2);
        if (md1.rpm_motor_shaft >= p1.maxRpm || md2.rpm_motor_shaft >= p2.maxRpm) break;
    }

    return buildResult(xAxis, m1Selected, m2Selected, d1, d2, "Torque (Nm)", this);
}

QVariantMap MotorComparisonHelper::runVbRpmSweep(double torque, double rpmMax, double vbus, int points, bool negative,
                                                   const QVariantMap &m1Params, const QVariantMap &m2Params,
                                                   const QVariantList &m1Selected, const QVariantList &m2Selected)
{
    if (!mM1Loaded) loadM1ConfigLocal();
    if (!mM2Loaded) loadM2ConfigLocal();

    auto p1 = paramsFromMap(m1Params);
    auto p2 = paramsFromMap(m2Params);
    double rStart = negative ? -rpmMax : rpmMax / points;

    QVector<double> xAxis;
    QVector<MotorData> d1, d2;

    for (double r = rStart; r < rpmMax; r += rpmMax / points) {
        MotorData md1; md1.configure(&mM1Config, p1); md1.updateRpmVBusFW(torque, r, vbus);
        MotorData md2; md2.configure(&mM2Config, p2); md2.updateRpmVBusFW(torque, r, vbus);
        xAxis.append(r);
        d1.append(md1);
        d2.append(md2);
        if (md1.rpm_motor_shaft >= p1.maxRpm || md2.rpm_motor_shaft >= p2.maxRpm) break;
    }

    return buildResult(xAxis, m1Selected, m2Selected, d1, d2, "RPM", this);
}

QVariantMap MotorComparisonHelper::queryDataAtX(double x, int testMode,
                                                  double torque, double rpm, double power,
                                                  double rpmStart, double exponent, double baseTorque,
                                                  double vbus,
                                                  const QVariantMap &m1Params, const QVariantMap &m2Params)
{
    if (!mM1Loaded) loadM1ConfigLocal();
    if (!mM2Loaded) loadM2ConfigLocal();

    auto p1 = paramsFromMap(m1Params);
    auto p2 = paramsFromMap(m2Params);

    MotorData md1, md2;
    md1.configure(&mM1Config, p1);
    md2.configure(&mM2Config, p2);

    switch (testMode) {
    case 0: // Torque sweep: x = torque
        md1.update(rpm, x);
        md2.update(rpm, x);
        break;
    case 1: // RPM sweep: x = rpm
        md1.update(x, torque);
        md2.update(x, torque);
        break;
    case 2: { // RPM/Power
        double rps = x * 2.0 * M_PI / 60.0;
        double t = power / rps;
        md1.update(x, t);
        md2.update(x, t);
        break;
    }
    case 3: { // Exp
        double p_max_const = power / pow(rpm - rpmStart, exponent);
        double rps = x * 2.0 * M_PI / 60.0;
        double pw = p_max_const * pow(x > rpmStart ? (x - rpmStart) : 0.0, exponent);
        double t = pw / rps + baseTorque;
        md1.update(x, t);
        md2.update(x, t);
        break;
    }
    case 4: // VBus
        md1.updateTorqueVBus(x, vbus);
        md2.updateTorqueVBus(x, vbus);
        break;
    case 5: // VB+FW
        md1.updateTorqueVBusFW(x, rpm, vbus);
        md2.updateTorqueVBusFW(x, rpm, vbus);
        break;
    case 6: // VB+RPM
        md1.updateRpmVBusFW(torque, x, vbus);
        md2.updateRpmVBusFW(torque, x, vbus);
        break;
    }

    QVariantMap result;
    result["m1"] = motorDataToMap(md1);
    result["m2"] = motorDataToMap(md2);
    return result;
}
