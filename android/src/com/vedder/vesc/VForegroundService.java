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

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.os.Build;

import vedder.vesctool.R;

public class VForegroundService extends Service {
    public static final String ACTION_START_FOREGROUND_SERVICE = "ACTION_START_FOREGROUND_SERVICE";
    public static final String ACTION_STOP_FOREGROUND_SERVICE = "ACTION_STOP_FOREGROUND_SERVICE";
    public static final String ACTION_STOP = "ACTION_STOP";

    public VForegroundService() {
    }

    @Override
    public IBinder onBind(Intent intent) {
        throw new UnsupportedOperationException("Not implemented");
    }

    @Override
    public void onCreate() {
        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if(intent != null)
        {
            String action = intent.getAction();

            switch (action)
            {
                case ACTION_START_FOREGROUND_SERVICE:
                    startForegroundService();
                    break;
                case ACTION_STOP_FOREGROUND_SERVICE:
                    stopForegroundService();
                    break;
                case ACTION_STOP:
                    stopForegroundService();
                    break;
            }
        }
        return super.onStartCommand(intent, flags, startId);
    }

    private void startForegroundService()
    {
        // Create notification default intent.
        Intent intent = new Intent();
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);

        Notification.Builder builder;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            String channelId = "VESC_CHANNEL";
            NotificationChannel channel = new NotificationChannel(channelId, "VESC", NotificationManager.IMPORTANCE_DEFAULT);
            channel.setDescription("VESC GNSS");
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
            builder = new Notification.Builder(this, channelId);
        } else {
            builder = new Notification.Builder(this);
        }

        builder.setContentTitle("VESC Tool GNSS");
        builder.setContentText("VESC Tool is keeping GNSS alive.");

        builder.setWhen(System.currentTimeMillis());
        builder.setSmallIcon(R.drawable.icon);

//        builder.setFullScreenIntent(pendingIntent, true);

        Intent stopIntent = new Intent(this, VForegroundService.class);
        stopIntent.setAction(ACTION_STOP);
        PendingIntent pendingPrevIntent = PendingIntent.getService(this, 0, stopIntent, 0);
        Notification.Action prevAction = new Notification.Action(android.R.drawable.ic_media_pause, "Stop", pendingPrevIntent);
        builder.addAction(prevAction);

        startForeground(1, builder.build());
    }

    private void stopForegroundService()
    {
        stopForeground(true);
        stopSelf();
    }
}

