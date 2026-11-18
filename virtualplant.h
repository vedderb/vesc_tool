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

#ifndef VIRTUALPLANT_H
#define VIRTUALPLANT_H

#include <QVector>
#include "configparams.h"

/*
 * The hardware behind the Virtual ESC: a dual motor vehicle with a battery,
 * hall sensors and a rider with a remote. The motor, battery and hall
 * sensors have fixed "true" values that detection measures. The motor and
 * app configurations act as the controller, so limits, input mappings and
 * a wrong hall table behave like on real hardware.
 *
 * Simplifications: the current controller is ideal up to the voltage limit,
 * id is 0 and both motors share the state of motor 1.
 */
class VirtualPlant
{
public:
    VirtualPlant(ConfigParams *mcConf, ConfigParams *appConf);

    void step(double dt);

    // True motor, as measured by detection
    static constexpr double motorR = 0.0213;
    static constexpr double motorL = 17.6e-6;
    static constexpr double motorLdLqDiff = 3.1e-6;
    static constexpr double motorLambda = 0.00411;
    static const int hallTable[8];
    static constexpr double encOffset = 123.4;
    static constexpr int motorCount = 2;
    static constexpr int packCells = 12; // The pack does not follow si_battery_cells, so the wizards can measure it
    static constexpr double gnssRadius = 150.0;

    // Rider and remote
    enum Scenario {CRUISE, HILL, STOPGO, IDLE}; // Same order as the names of the sim terminal command
    Scenario scenario;
    double throttleOverride; // NAN when the rider is in control
    double inputPolledS; // Last poll of the decoded input, sweeps the throttle

    // Direct control from VESC Tool, expires after a second
    enum Control {CTRL_NONE, CTRL_CURRENT, CTRL_BRAKE, CTRL_DUTY, CTRL_RPM};
    void setControl(Control mode, double value);
    void alive() { mControlAgeS = 0.0; }
    double appDisabledS;

    int fault; // mc_fault_code, the motor is off when set
    int faultForced;
    QVector<double> cellOverride; // NAN when not overridden
    double soc;

    // State
    double timeS, speed, erpm, iq, vd, vq, duty, vBus, iIn;
    double tempFet, tempMotor, angleE;
    double ah, ahCharged, wh, whCharged, tacho, tachoAbs, dist, distAbs, odometer;
    double slope, heading, lat, lon, accel;
    double throttle, ppmPulse, adc1, adc2, decodedPpm, decodedAdc, decodedAdc2, decodedChuk;

    // Statistics as in COMM_GET_STATS
    double statTime, statSpeedSum, statSpeedMax, statPowerSum, statPowerMax;
    double statCurrSum, statCurrMax, statFetSum, statFetMax, statMotorSum, statMotorMax;
    void resetStats();

    double cellVoltage(int cell) const;

private:
    ConfigParams *mMc;
    ConfigParams *mApp;
    Control mControl;
    double mControlValue;
    double mControlAgeS;
    double mRpmInt;
    double mSweepStartS;

    double erpmPerSpeed() const;
    double hallTorqueFactor() const;
    void updateRider(double dt);
    void updateInputs();
    double requestedIq();
};

#endif // VIRTUALPLANT_H
