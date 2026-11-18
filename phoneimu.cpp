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

#include <QtGlobal>

// iOS is in ios/src/phoneImu.mm
#if defined(Q_OS_ANDROID)
#include <QCoreApplication>
#include <QJniObject>
#include <QJniEnvironment>

namespace {
const char *JAVA_CLASS = "com/vedder/vesc/PhoneImu";
}

bool PhoneImu::available()
{
    return QJniObject::callStaticMethod<jboolean>(
                JAVA_CLASS, "available", "(Landroid/content/Context;)Z",
                QNativeInterface::QAndroidApplication::context().object());
}

void PhoneImu::setActive(bool active)
{
    QJniObject::callStaticMethod<void>(
                JAVA_CLASS, "setActive", "(Landroid/content/Context;Z)V",
                QNativeInterface::QAndroidApplication::context().object(), jboolean(active));
}

bool PhoneImu::read(Reading &r)
{
    QJniObject values = QJniObject::callStaticObjectMethod(JAVA_CLASS, "read", "()[F");
    if (!values.isValid()) {
        return false;
    }

    // Gravity, acceleration, gyro, magnetometer and yaw
    jfloat v[13];
    QJniEnvironment env;
    env->GetFloatArrayRegion(values.object<jfloatArray>(), 0, 13, v);
    for (int i = 0;i < 3;i++) {
        r.gravity[i] = v[i];
        r.acc[i] = v[i + 3];
        r.gyro[i] = v[i + 6];
        r.mag[i] = v[i + 9];
    }
    r.yaw = v[12];
    return true;
}
#elif !defined(Q_OS_IOS)
bool PhoneImu::available()
{
    return false;
}

void PhoneImu::setActive(bool active)
{
    (void)active;
}

bool PhoneImu::read(Reading &r)
{
    (void)r;
    return false;
}
#endif
