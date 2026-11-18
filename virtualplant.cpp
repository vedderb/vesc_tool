/*
    Copyright 2026 Dado Mista

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

#include "virtualplant.h"
#include "datatypes.h"

#include <cmath>

// Forward rotation goes through the hall states 1, 3, 2, 6, 4, 5. Angles are
// in the firmware format, where 200 is one electrical revolution.
const int VirtualPlant::hallTable[8] = {255, 11, 78, 44, 144, 178, 111, 255};

namespace {
const double MASS = 90.0;
const double GRAVITY = 9.81;
const double ROLL_RES = 0.015;
const double DRAG_AREA = 0.5; // Cd * A
const double AIR_DENSITY = 1.2;
const double CELL_RES = 0.006;
const double AMBIENT = 22.0;
const double GNSS_LAT = 57.7089;
const double GNSS_LON = 11.9746;

// Control type tables: current, duty or rpm, and what happens below 0:
// clamp, reverse or brake. Same order as ppm_control_type in the firmware
// and adc_control_type in datatypes.h, unknown types are off. Types without
// center use the full input range.
enum Kind {OFF, CURR, DUTY, RPM};
enum Neg {CLAMP, REVERSE, BRAKE};
struct CtrlType { Kind kind; Neg neg; bool center; };

const CtrlType PPM_TYPES[] = {
    {OFF, CLAMP, true}, {CURR, REVERSE, true}, {CURR, CLAMP, false}, {CURR, BRAKE, true},
    {DUTY, REVERSE, true}, {DUTY, CLAMP, false}, {RPM, REVERSE, true}, {RPM, CLAMP, false},
    {CURR, BRAKE, true}, {CURR, BRAKE, true}
};

const CtrlType ADC_TYPES[] = {
    {OFF, CLAMP, false}, {CURR, CLAMP, false}, {CURR, REVERSE, true}, {CURR, CLAMP, false},
    {CURR, CLAMP, false}, {CURR, BRAKE, true}, {CURR, BRAKE, true}, {CURR, CLAMP, false},
    {CURR, CLAMP, false}, {DUTY, CLAMP, false}, {DUTY, REVERSE, true}, {DUTY, CLAMP, false},
    {RPM, CLAMP, false}, {RPM, REVERSE, true}, {RPM, CLAMP, false}
};

// 0 at zeroAt, 1 at oneAt and linear in between
double fade(double value, double zeroAt, double oneAt)
{
    return qBound(0.0, (value - zeroAt) / (oneAt - zeroAt), 1.0);
}

// Map value to -1..1 (center) or 0..1 between start and end
double mapRange(double value, double start, double center, double end, bool useCenter)
{
    double res;
    if (useCenter) {
        res = value >= center ? (value - center) / (end - center) : (value - center) / (center - start);
    } else {
        res = (value - start) / (end - start);
    }
    return qBound(useCenter ? -1.0 : 0.0, std::isfinite(res) ? res : 0.0, 1.0);
}

double cellOcv(double soc)
{
    return 3.0 + 1.2 * pow(qBound(0.0, soc, 1.0), 0.6);
}
}

VirtualPlant::VirtualPlant(ConfigParams *mcConf, ConfigParams *appConf) :
    mMc(mcConf), mApp(appConf)
{
    scenario = CRUISE;
    throttleOverride = NAN;
    inputPolledS = -100.0;
    appDisabledS = 0.0;
    fault = FAULT_CODE_NONE;
    faultForced = FAULT_CODE_NONE;
    cellOverride.fill(NAN, packCells);
    soc = 0.85;

    timeS = speed = erpm = iq = vd = vq = duty = iIn = 0.0;
    angleE = accel = slope = heading = throttle = 0.0;
    ah = ahCharged = wh = whCharged = tacho = tachoAbs = dist = distAbs = 0.0;
    odometer = 1234567.0;
    tempFet = AMBIENT + 3.0;
    tempMotor = AMBIENT + 1.0;
    lat = GNSS_LAT;
    lon = GNSS_LON;
    vBus = packCells * cellOcv(soc);
    resetStats();

    mControl = CTRL_NONE;
    mControlValue = 0.0;
    mControlAgeS = 10.0;
    mRpmInt = 0.0;
    mSweepStartS = 0.0;

    updateInputs();
}

void VirtualPlant::setControl(Control mode, double value)
{
    mControl = mode;
    mControlValue = value;
    mControlAgeS = 0.0;
}

void VirtualPlant::resetStats()
{
    statTime = statSpeedSum = statSpeedMax = statPowerSum = statPowerMax = 0.0;
    statCurrSum = statCurrMax = statFetSum = statFetMax = statMotorSum = statMotorMax = 0.0;
}

double VirtualPlant::cellVoltage(int cell) const
{
    if (std::isfinite(cellOverride.at(cell))) {
        return cellOverride.at(cell);
    }

    // Small spread between the cells, as in a real pack
    return cellOcv(soc) - iIn * motorCount * CELL_RES + 0.004 * sin(cell * 2.1);
}

double VirtualPlant::erpmPerSpeed() const
{
    double wheelD = qMax(0.01, mMc->getParamDouble("si_wheel_diameter"));
    double rpmPerMs = mMc->getParamDouble("si_gear_ratio") * 60.0 / (M_PI * wheelD);
    return rpmPerMs * mMc->getParamInt("si_motor_poles") / 2.0;
}

double VirtualPlant::hallTorqueFactor() const
{
    // The torque follows the cosine of the angle error, so a wrong table
    // gives a weak, rough or reversed start.
    double sum = 0.0;
    for (int i = 1;i < 7;i++) {
        int conf = mMc->getParamInt(QString("foc_hall_table__%1").arg(i));
        if (conf <= 200) {
            sum += cos(double(conf - hallTable[i]) / 200.0 * 2.0 * M_PI);
        }
    }
    return sum / 6.0;
}

void VirtualPlant::updateRider(double dt)
{
    double target = 0.0;
    switch (scenario) {
    case CRUISE:
        target = 5.5 + 1.5 * sin(timeS * 2.0 * M_PI / 45.0) + 0.8 * sin(timeS * 2.0 * M_PI / 13.0);
        if (fmod(timeS, 120.0) > 108.0) {
            target = 0.0;
        }
        break;
    case HILL: target = 5.5; break;
    case STOPGO: target = fmod(timeS, 20.0) < 12.0 ? 6.5 : 0.0; break;
    case IDLE: break;
    }

    double cmd = scenario == IDLE ? 0.0 : qBound(-1.0, 0.35 * (target - speed), 1.0);

    // Something shows the input mapping: sweep 0, 1, 0, -1, 0 over 8 seconds
    // and rest in the center for 8 seconds, so the wizards can finish
    if ((timeS - inputPolledS) < 1.5) {
        double p = fmod(timeS - mSweepStartS, 16.0) / 2.0;
        cmd = p < 1.0 ? p : (p < 3.0 ? 2.0 - p : (p < 4.0 ? p - 4.0 : 0.0));
    } else {
        mSweepStartS = timeS;
    }

    if (std::isfinite(throttleOverride)) {
        cmd = throttleOverride;
    }

    throttle += (cmd - throttle) * qMin(1.0, dt / 0.25);
    slope = scenario == HILL ? 0.06 * sin(timeS * 2.0 * M_PI / 40.0) : 0.0;
}

void VirtualPlant::updateInputs()
{
    // The remote: PPM stick, ADC thumb throttle and brake lever, nunchuk
    ppmPulse = 1.5 + throttle * 0.45;
    adc1 = 0.85 + qMax(throttle, 0.0) * 1.75;
    adc2 = 0.85 + qMax(-throttle, 0.0) * 1.75;
    decodedChuk = throttle;

    // Decoded with the configured mapping, like the apps in the firmware. ADC
    // levels are 0 to 1, also with center.
    decodedPpm = mapRange(ppmPulse, mApp->getParamDouble("app_ppm_conf.pulse_start"),
                          mApp->getParamDouble("app_ppm_conf.pulse_center"),
                          mApp->getParamDouble("app_ppm_conf.pulse_end"), true);

    int adcType = mApp->getParamEnum("app_adc_conf.ctrl_type");
    bool center = adcType >= 0 && adcType < int(sizeof(ADC_TYPES) / sizeof(ADC_TYPES[0])) &&
            ADC_TYPES[adcType].center;
    decodedAdc = mapRange(adc1, mApp->getParamDouble("app_adc_conf.voltage_start"),
                          mApp->getParamDouble("app_adc_conf.voltage_center"),
                          mApp->getParamDouble("app_adc_conf.voltage_end"), center);
    if (center) {
        decodedAdc = (decodedAdc + 1.0) / 2.0;
    }
    if (mApp->getParamBool("app_adc_conf.voltage_inverted")) {
        decodedAdc = 1.0 - decodedAdc;
    }

    decodedAdc2 = mapRange(adc2, mApp->getParamDouble("app_adc_conf.voltage2_start"), 0.0,
                           mApp->getParamDouble("app_adc_conf.voltage2_end"), false);
    if (mApp->getParamBool("app_adc_conf.voltage2_inverted")) {
        decodedAdc2 = 1.0 - decodedAdc2;
    }
}

double VirtualPlant::requestedIq()
{
    double iMax = mMc->getParamDouble("l_current_max");
    double iBrake = -mMc->getParamDouble("l_current_min");

    Control mode = CTRL_NONE;
    double value = 0.0;

    if (mControlAgeS < 1.0) {
        mode = mControl;
        value = mControlValue;
    } else if (appDisabledS <= 0.0) {
        CtrlType type = {OFF, CLAMP, false};
        double level = 0.0;
        int ctrl = 0;
        bool adc = false;

        switch (mApp->getParamEnum("app_to_use")) {
        case 1: case 4: // PPM, PPM and UART
            ctrl = mApp->getParamEnum("app_ppm_conf.ctrl_type");
            if (ctrl >= 0 && ctrl < int(sizeof(PPM_TYPES) / sizeof(PPM_TYPES[0]))) {
                type = PPM_TYPES[ctrl];
            }
            level = type.center ? decodedPpm : (decodedPpm + 1.0) / 2.0;
            break;
        case 2: case 5: // ADC, ADC and UART
            ctrl = mApp->getParamEnum("app_adc_conf.ctrl_type");
            if (ctrl >= 0 && ctrl < int(sizeof(ADC_TYPES) / sizeof(ADC_TYPES[0]))) {
                type = ADC_TYPES[ctrl];
            }
            level = type.center ? decodedAdc * 2.0 - 1.0 : decodedAdc;
            adc = true;
            break;
        case 3: case 6: case 7: // UART, Nunchuk, NRF
            if (mApp->getParamEnum("app_chuk_conf.ctrl_type") != 0) {
                type = {CURR, BRAKE, true};
            }
            level = decodedChuk;
            break;
        default:
            break;
        }

        if (type.neg == CLAMP) {
            level = qMax(level, 0.0);
        }

        if (type.kind == CURR) {
            if (level >= 0.0 || type.neg == REVERSE) {
                mode = CTRL_CURRENT;
                value = level * (level >= 0.0 ? iMax : iBrake);
            } else {
                mode = CTRL_BRAKE;
                value = -level * iBrake;
            }
        } else if (type.kind == DUTY) {
            mode = CTRL_DUTY;
            value = level * mMc->getParamDouble("l_max_duty");
        } else if (type.kind == RPM) {
            mode = CTRL_RPM;
            if (adc) {
                value = level * (level >= 0.0 ? mMc->getParamDouble("l_max_erpm") :
                                                -mMc->getParamDouble("l_min_erpm"));
            } else {
                value = level * mApp->getParamDouble("app_ppm_conf.pid_max_erpm");
            }
        }

        // Brake on ADC2
        bool adc2Brake = ctrl == ADC_CTRL_TYPE_CURRENT_REV_BUTTON_BRAKE_ADC ||
                ctrl == ADC_CTRL_TYPE_CURRENT_NOREV_BRAKE_ADC;
        if (adc && adc2Brake && decodedAdc2 > 0.05) {
            mode = CTRL_BRAKE;
            value = decodedAdc2 * iBrake;
        }
    }

    if (mode != CTRL_RPM) {
        mRpmInt = 0.0;
    }

    double backEmf = erpm / 60.0 * 2.0 * M_PI * motorLambda;
    double vMax = vBus / sqrt(3.0);

    switch (mode) {
    case CTRL_CURRENT: return value;
    case CTRL_BRAKE: return -value * tanh(erpm / 200.0);
    case CTRL_DUTY: return (value * vMax - backEmf) / motorR;
    case CTRL_RPM: {
        double err = value - erpm;
        mRpmInt = qBound(-iMax, mRpmInt + err * 0.02 * 0.01, iMax);
        return err * 0.01 + mRpmInt;
    }
    default: return 0.0;
    }
}

void VirtualPlant::step(double dt)
{
    timeS += dt;
    mControlAgeS += dt;
    appDisabledS -= dt;

    updateRider(dt);
    updateInputs();

    double omegaE = erpm / 60.0 * 2.0 * M_PI;
    double backEmf = omegaE * motorLambda;
    double vMax = vBus * mMc->getParamDouble("l_max_duty") / sqrt(3.0);

    // Limits as in mc_interface: temperature, battery cutoff and ERPM
    double tempScale = 1.0 - qMax(fade(tempFet, mMc->getParamDouble("l_temp_fet_start"),
                                       mMc->getParamDouble("l_temp_fet_end")),
                                  fade(tempMotor, mMc->getParamDouble("l_temp_motor_start"),
                                       mMc->getParamDouble("l_temp_motor_end")));
    double battScale = fade(vBus, mMc->getParamDouble("l_battery_cut_end"),
                            mMc->getParamDouble("l_battery_cut_start"));
    double iMax = mMc->getParamDouble("l_current_max") * tempScale * battScale;
    double iMin = mMc->getParamDouble("l_current_min") * tempScale;

    double iqNew = qBound(iMin, requestedIq(), iMax);
    if ((erpm > mMc->getParamDouble("l_max_erpm") && iqNew > 0.0) ||
            (erpm < mMc->getParamDouble("l_min_erpm") && iqNew < 0.0)) {
        iqNew = 0.0;
    }

    // Voltage limit, which gives the top speed
    iqNew = qBound((-vMax - backEmf) / motorR, iqNew, (vMax - backEmf) / motorR);

    // Input current limit
    double iInNew = 1.5 * (motorR * iqNew + backEmf) * iqNew / vBus;
    double iInMax = mMc->getParamDouble("l_in_current_max");
    double iInMin = mMc->getParamDouble("l_in_current_min");
    if (iInNew > iInMax || iInNew < iInMin) {
        iqNew *= (iInNew > iInMax ? iInMax : iInMin) / iInNew;
    }

    // Under- and overvoltage faults, or one set from the terminal
    fault = faultForced;
    if (fault == FAULT_CODE_NONE && vBus < mMc->getParamDouble("l_min_vin")) {
        fault = FAULT_CODE_UNDER_VOLTAGE;
    } else if (fault == FAULT_CODE_NONE && vBus > mMc->getParamDouble("l_max_vin")) {
        fault = FAULT_CODE_OVER_VOLTAGE;
    }

    if (fault != FAULT_CODE_NONE) {
        iqNew = 0.0;
    }

    iq = iqNew;
    vq = motorR * iq + backEmf;
    vd = -omegaE * motorL * iq;
    duty = vq / (vBus / sqrt(3.0));
    iIn = 1.5 * vq * iq / vBus;

    // Hall sensors are used below the sensorless ERPM
    double torqueFactor = 1.0;
    if (mMc->getParamEnum("foc_sensor_mode") == 2 &&
            fabs(erpm) < mMc->getParamDouble("foc_sl_erpm")) {
        torqueFactor = hallTorqueFactor();
    }

    // Vehicle
    double wheelR = qMax(0.01, mMc->getParamDouble("si_wheel_diameter")) / 2.0;
    double torque = 1.5 * mMc->getParamInt("si_motor_poles") / 2.0 * motorLambda * iq * torqueFactor;
    double force = motorCount * torque * mMc->getParamDouble("si_gear_ratio") / wheelR;
    double drag = ROLL_RES * MASS * GRAVITY * tanh(speed / 0.1) +
            0.5 * AIR_DENSITY * DRAG_AREA * speed * fabs(speed) + MASS * GRAVITY * sin(slope);
    accel = (force - drag) / MASS;
    speed += accel * dt;
    erpm = speed * erpmPerSpeed();

    angleE = fmod(angleE + erpm / 60.0 * 2.0 * M_PI * dt, 2.0 * M_PI);
    tacho += erpm / 60.0 * 6.0 * dt;
    tachoAbs += fabs(erpm) / 60.0 * 6.0 * dt;
    dist += speed * dt;
    distAbs += fabs(speed) * dt;
    odometer += fabs(speed) * dt;

    // Battery and counters, per controller like the firmware
    double cap = qMax(0.1, mMc->getParamDouble("si_battery_ah"));
    soc = qBound(0.0, soc - iIn * motorCount * dt / 3600.0 / cap, 1.0);
    double v = 0.0;
    for (int i = 0;i < packCells;i++) {
        v += cellVoltage(i);
    }
    vBus = qMax(1.0, v);

    if (iIn >= 0.0) {
        ah += iIn * dt / 3600.0;
        wh += iIn * vBus * dt / 3600.0;
    } else {
        ahCharged -= iIn * dt / 3600.0;
        whCharged -= iIn * vBus * dt / 3600.0;
    }

    // First order thermal models
    tempMotor += dt * (1.5 * motorR * iq * iq - (tempMotor - AMBIENT) / 1.2) / 200.0;
    tempFet += dt * (1.5 * 0.003 * iq * iq + 1.5 - (tempFet - AMBIENT) / 1.5) / 80.0;

    // GNSS and IMU: the vehicle drives in a circle
    heading = fmod(heading + speed * dt / gnssRadius, 2.0 * M_PI);
    lat = GNSS_LAT + gnssRadius * (1.0 - cos(heading)) / 111320.0;
    lon = GNSS_LON + gnssRadius * sin(heading) / (111320.0 * cos(GNSS_LAT * M_PI / 180.0));

    if (fabs(speed) > 0.3) {
        double power = fabs(iIn * vBus * motorCount);
        statTime += dt;
        statSpeedSum += fabs(speed) * dt;
        statPowerSum += power * dt;
        statCurrSum += fabs(iq) * dt;
        statFetSum += tempFet * dt;
        statMotorSum += tempMotor * dt;
        statSpeedMax = qMax(statSpeedMax, fabs(speed));
        statPowerMax = qMax(statPowerMax, power);
        statCurrMax = qMax(statCurrMax, fabs(iq));
        statFetMax = qMax(statFetMax, tempFet);
        statMotorMax = qMax(statMotorMax, tempMotor);
    }
}
