/*
    Copyright 2026 Jeffrey M. Friesen

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

#include "phoneimu.h"

#include <CoreMotion/CoreMotion.h>
#include <cmath>

namespace {
CMMotionManager *manager = nil;
}

bool PhoneImu::available()
{
    if (!manager) {
        manager = [[CMMotionManager alloc] init];
        manager.deviceMotionUpdateInterval = 0.02;
    }
    return manager.deviceMotionAvailable;
}

void PhoneImu::setActive(bool active)
{
    if (!available()) {
        return;
    }

    if (active && !manager.deviceMotionActive) {
        [manager startDeviceMotionUpdatesUsingReferenceFrame:CMAttitudeReferenceFrameXMagneticNorthZVertical];
    } else if (!active) {
        [manager stopDeviceMotionUpdates];
    }
}

bool PhoneImu::read(Reading &r)
{
    CMDeviceMotion *m = manager.deviceMotion;
    if (!m) {
        return false;
    }

    // Core Motion has gravity pointing down and the gyro in rad/s. The yaw is
    // that of the x axis, which is 90 degrees to the right of the top.
    const double g[3] = {m.gravity.x, m.gravity.y, m.gravity.z};
    const double a[3] = {m.userAcceleration.x, m.userAcceleration.y, m.userAcceleration.z};
    const double w[3] = {m.rotationRate.x, m.rotationRate.y, m.rotationRate.z};
    const double b[3] = {m.magneticField.field.x, m.magneticField.field.y, m.magneticField.field.z};
    for (int i = 0;i < 3;i++) {
        r.gravity[i] = -g[i];
        r.acc[i] = -(g[i] + a[i]);
        r.gyro[i] = w[i] * 180.0 / M_PI;
        r.mag[i] = b[i];
    }
    r.yaw = remainder(m.attitude.yaw + M_PI / 2.0, 2.0 * M_PI);
    return true;
}
