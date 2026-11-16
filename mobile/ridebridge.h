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

#ifndef VESC_RIDE_BRIDGE_H
#define VESC_RIDE_BRIDGE_H

// Platform side of the ride display: a Live Activity on iOS
// (ios/LiveActivity/LiveActivityBridge.swift) and an ongoing notification
// on Android (android/ridebridge.cpp).

#ifdef __cplusplus
extern "C" {
#endif

// On iOS this requires iOS 18, matching Apple Watch Live Activity mirroring.
// All boolean results use 0/1. The returned error string remains valid until
// the next bridge operation changes the error; callers should copy it.
int vesc_live_activity_supported(void);
int vesc_live_activity_enabled(void);
int vesc_live_activity_start(const char *json);
void vesc_live_activity_update(const char *json);
void vesc_live_activity_end(void);
int vesc_live_activity_is_active(void);
// 1 once after the logging button on the ride display was pressed
int vesc_live_activity_take_log_toggle(void);
const char *vesc_live_activity_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
