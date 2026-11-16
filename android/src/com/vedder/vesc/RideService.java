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

package com.vedder.vesc;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;

import org.json.JSONObject;

import java.util.Locale;

import vedder.vesctool.R;

// Ongoing ride notification while connected to a VESC, the Android counterpart
// of the iOS Live Activity. Driven from RideSession through android/ridebridge.cpp.
public class RideService extends Service {
    private static final String CHANNEL_ID = "VESC_RIDE";
    private static final String ACTION_DISMISSED = "com.vedder.vesc.RIDE_DISMISSED";
    private static final int NOTIFICATION_ID = 2;

    private static volatile boolean sActive = false;
    private static volatile String sJson = "{}";

    public static boolean enabled(Context ctx) {
        return ctx.getSystemService(NotificationManager.class).areNotificationsEnabled();
    }

    public static boolean start(Context ctx, String json) {
        sJson = json;
        try {
            ctx.startForegroundService(new Intent(ctx, RideService.class));
        } catch (RuntimeException e) {
            // Starting a foreground service from the background is not allowed on newer Android versions
            return false;
        }

        sActive = true;
        return true;
    }

    public static void update(Context ctx, String json) {
        sJson = json;
        if (sActive) {
            ctx.getSystemService(NotificationManager.class).notify(NOTIFICATION_ID, build(ctx));
        }
    }

    public static void end(Context ctx) {
        sActive = false;
        ctx.stopService(new Intent(ctx, RideService.class));
    }

    public static boolean isActive() {
        return sActive;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null && ACTION_DISMISSED.equals(intent.getAction())) {
            sActive = false;
            stopSelf();
            return START_NOT_STICKY;
        }

        if (Build.VERSION.SDK_INT >= 29) {
            startForeground(NOTIFICATION_ID, build(this), ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE);
        } else {
            startForeground(NOTIFICATION_ID, build(this));
        }
        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private static Notification build(Context ctx) {
        JSONObject o;
        try {
            o = new JSONObject(sJson);
        } catch (Exception e) {
            o = new JSONObject();
        }

        boolean imperial = o.optBoolean("useImperial");
        boolean fresh = o.optBoolean("fresh");
        double distFactor = imperial ? 0.000621371192 : 0.001;
        String distUnit = imperial ? "mi" : "km";
        String speed = String.format(Locale.getDefault(), "%.0f %s",
                o.optDouble("speedMps", 0) * (imperial ? 2.236936292 : 3.6), imperial ? "mph" : "km/h");
        double battery = o.optDouble("batteryLevel", -1);
        double range = o.optDouble("estimatedRangeMeters", -1);

        StringBuilder text = new StringBuilder(String.format(Locale.getDefault(), "%s%.2f %s",
                o.optBoolean("distanceIncomplete") ? "≥ " : "", o.optDouble("distanceMeters", 0) * distFactor, distUnit));
        if (fresh && battery >= 0) {
            text.append(String.format(Locale.getDefault(), " · Battery %d%%", Math.round(battery * 100)));
        }
        if (fresh && range >= 0) {
            text.append(String.format(Locale.getDefault(), " · Range %.1f %s", range * distFactor, distUnit));
        }

        NotificationManager nm = ctx.getSystemService(NotificationManager.class);
        if (nm.getNotificationChannel(CHANNEL_ID) == null) {
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, "Ride", NotificationManager.IMPORTANCE_LOW);
            channel.setDescription("Speed, battery and trip while connected to a VESC");
            nm.createNotificationChannel(channel);
        }
        Notification.Builder builder = new Notification.Builder(ctx, CHANNEL_ID);

        Intent open = ctx.getPackageManager().getLaunchIntentForPackage(ctx.getPackageName());
        Intent dismissed = new Intent(ctx, RideService.class).setAction(ACTION_DISMISSED);

        builder.setSmallIcon(R.drawable.ic_notification)
                .setContentTitle(fresh ? speed : o.optBoolean("connected") ? "Waiting for data" : "Disconnected")
                .setContentText(text)
                .setOngoing(true)
                .setOnlyAlertOnce(true)
                .setCategory(Notification.CATEGORY_STATUS)
                .setWhen((long) (o.optDouble("startedAt", 0) * 1000))
                .setUsesChronometer(true)
                .setShowWhen(true)
                .setContentIntent(PendingIntent.getActivity(ctx, 0, open, PendingIntent.FLAG_IMMUTABLE))
                .setDeleteIntent(PendingIntent.getService(ctx, 1, dismissed, PendingIntent.FLAG_IMMUTABLE));

        if (fresh && battery >= 0) {
            builder.setProgress(100, (int) Math.round(battery * 100), false);
        }

        // Android 16 shows promoted ongoing notifications as a Live Update chip in the status bar.
        // Set as extras, the builder methods only exist from 36.1.
        if (Build.VERSION.SDK_INT >= 36) {
            Bundle extras = new Bundle();
            extras.putBoolean("android.requestPromotedOngoing", true);
            if (fresh) {
                extras.putCharSequence("android.shortCriticalText", speed);
            }
            builder.addExtras(extras);
        }

        return builder.build();
    }
}
