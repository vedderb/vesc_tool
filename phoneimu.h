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

#ifndef PHONEIMU_H
#define PHONEIMU_H

/*
 * The IMU of the phone, which the Virtual ESC can report as its own. Vectors
 * are in the frame of the phone: x to the right, y to the top and z out of
 * the screen. Gravity and acceleration point up when the phone lies still,
 * like on a VESC. There is no phone IMU on the desktop.
 */
namespace PhoneImu {

struct Reading {
    double gravity[3]; // g
    double acc[3]; // g
    double gyro[3]; // deg/s
    double mag[3]; // uT
    double yaw; // rad, counterclockwise from north
};

bool available();
void setActive(bool active);
bool read(Reading &r);

}

#endif // PHONEIMU_H
