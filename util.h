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

#ifndef UTIL_H
#define UTIL_H

#include "vescinterface.h"
#include <QWidget>

namespace util {

double map(double x, double in_min, double in_max, double out_min, double out_max);
float throttle_curve(float val, float curve_acc, float curve_brake, int mode);
bool autoconnectBlockingWithProgress(VescInterface *vesc, QWidget *parent = 0);
void checkVersion(QString version, VescInterface *vesc = 0);

}

#endif // UTIL_H
