/*
    Copyright 2019 Benjamin Vedder	benjamin@vedder.se

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

package com.vedder.vesc;

import android.content.Context;
import android.content.Intent;
import android.provider.Settings;
import android.os.Build;
import android.text.TextUtils;
import android.provider.Settings.SettingNotFoundException;
import android.location.LocationManager;

public class Utils
{
    public static void startVForegroundService(Context ctx) {
        Intent intent = new Intent(ctx, VForegroundService.class);
        intent.setAction(VForegroundService.ACTION_START_FOREGROUND_SERVICE);
        ctx.startService(intent);
    }

    public static void stopVForegroundService(Context ctx) {
        Intent intent = new Intent(ctx, VForegroundService.class);
        intent.setAction(VForegroundService.ACTION_STOP_FOREGROUND_SERVICE);
        ctx.startService(intent);
    }

    public static boolean checkLocationEnabled(Context ctx) {
        int locationMode = 0;
        String locationProviders;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            LocationManager lm = (LocationManager) ctx.getSystemService(Context.LOCATION_SERVICE);
            return lm.isLocationEnabled();
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT){
            try {
                locationMode = Settings.Secure.getInt(ctx.getContentResolver(), Settings.Secure.LOCATION_MODE);
            } catch (SettingNotFoundException e) {
                e.printStackTrace();
                return false;
            }

            return locationMode != Settings.Secure.LOCATION_MODE_OFF;
        } else {
            locationProviders = Settings.Secure.getString(ctx.getContentResolver(), Settings.Secure.LOCATION_PROVIDERS_ALLOWED);
            return !TextUtils.isEmpty(locationProviders);
        }
    }
}
