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

#ifndef MOTORCOMPARISONHELPER_H
#define MOTORCOMPARISONHELPER_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include "configparams.h"
#include "pages/pagemotorcomparison.h"

class VescInterface;

class MotorComparisonHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit MotorComparisonHelper(QObject *parent = nullptr);

    Q_INVOKABLE void setVescInterface(QObject *vescIf);

    // Config loading
    Q_INVOKABLE bool loadM1ConfigLocal();
    Q_INVOKABLE bool loadM1ConfigFile(const QString &path);
    Q_INVOKABLE bool loadM2ConfigLocal();
    Q_INVOKABLE bool loadM2ConfigFile(const QString &path);

    // Run a sweep and return {xAxis: [...], series: [{name, data: [...]}]}
    Q_INVOKABLE QVariantMap runTorqueSweep(double rpm, double torqueMax, int points, bool negative,
                                           const QVariantMap &m1Params, const QVariantMap &m2Params,
                                           const QVariantList &m1Selected, const QVariantList &m2Selected);
    Q_INVOKABLE QVariantMap runRpmSweep(double torque, double rpmMax, int points, bool negative,
                                         const QVariantMap &m1Params, const QVariantMap &m2Params,
                                         const QVariantList &m1Selected, const QVariantList &m2Selected);
    Q_INVOKABLE QVariantMap runRpmPowerSweep(double power, double rpmStart, double rpmMax, int points,
                                              const QVariantMap &m1Params, const QVariantMap &m2Params,
                                              const QVariantList &m1Selected, const QVariantList &m2Selected);
    Q_INVOKABLE QVariantMap runExpSweep(double power, double rpmStart, double rpmMax, double exponent,
                                         double baseTorque, int points,
                                         const QVariantMap &m1Params, const QVariantMap &m2Params,
                                         const QVariantList &m1Selected, const QVariantList &m2Selected);
    Q_INVOKABLE QVariantMap runVbusSweep(double torqueMax, double vbus, int points, bool negative,
                                          const QVariantMap &m1Params, const QVariantMap &m2Params,
                                          const QVariantList &m1Selected, const QVariantList &m2Selected);
    Q_INVOKABLE QVariantMap runVbFwSweep(double torqueMax, double rpm, double vbus, int points, bool negative,
                                          const QVariantMap &m1Params, const QVariantMap &m2Params,
                                          const QVariantList &m1Selected, const QVariantList &m2Selected);
    Q_INVOKABLE QVariantMap runVbRpmSweep(double torque, double rpmMax, double vbus, int points, bool negative,
                                           const QVariantMap &m1Params, const QVariantMap &m2Params,
                                           const QVariantList &m1Selected, const QVariantList &m2Selected);

    // Query data at a specific x position (for vertical-line / table display)
    Q_INVOKABLE QVariantMap queryDataAtX(double x, int testMode,
                                          double torque, double rpm, double power,
                                          double rpmStart, double exponent, double baseTorque,
                                          double vbus,
                                          const QVariantMap &m1Params, const QVariantMap &m2Params);

    // Return list of all data item names
    Q_INVOKABLE QVariantList dataItemNames() const;

    // Public so free helper functions can use it
    double extractField(const MotorData &md, int index, double scale) const;

private:
    MotorDataParams paramsFromMap(const QVariantMap &m) const;
    QVariantMap motorDataToMap(const MotorData &md) const;

    VescInterface *mVesc;
    ConfigParams mM1Config;
    ConfigParams mM2Config;
    bool mM1Loaded;
    bool mM2Loaded;
};

#endif // MOTORCOMPARISONHELPER_H
