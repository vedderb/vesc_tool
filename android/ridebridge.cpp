/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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

#include "../mobile/ridebridge.h"

#include <QCoreApplication>
#include <QJniObject>

// Android side of the ride display, implemented by com.vedder.vesc.RideService
namespace {
const char *ServiceClass = "com/vedder/vesc/RideService";
const char *LastError = "";

QJniObject context()
{
    return QNativeInterface::QAndroidApplication::context();
}
}

int vesc_live_activity_supported(void)
{
    return 1;
}

int vesc_live_activity_enabled(void)
{
    return QJniObject::callStaticMethod<jboolean>(ServiceClass, "enabled", "(Landroid/content/Context;)Z",
                                                  context().object());
}

int vesc_live_activity_start(const char *json)
{
    const bool ok = QJniObject::callStaticMethod<jboolean>(
                ServiceClass, "start", "(Landroid/content/Context;Ljava/lang/String;)Z", context().object(),
                QJniObject::fromString(QString::fromUtf8(json)).object<jstring>());
    LastError = ok ? "" : "The ride notification can only start while VESC Tool is open.";
    return ok;
}

void vesc_live_activity_update(const char *json)
{
    QJniObject::callStaticMethod<void>(ServiceClass, "update", "(Landroid/content/Context;Ljava/lang/String;)V",
                                       context().object(),
                                       QJniObject::fromString(QString::fromUtf8(json)).object<jstring>());
}

void vesc_live_activity_end(void)
{
    QJniObject::callStaticMethod<void>(ServiceClass, "end", "(Landroid/content/Context;)V", context().object());
}

int vesc_live_activity_is_active(void)
{
    return QJniObject::callStaticMethod<jboolean>(ServiceClass, "isActive", "()Z");
}

const char *vesc_live_activity_last_error(void)
{
    return LastError;
}
