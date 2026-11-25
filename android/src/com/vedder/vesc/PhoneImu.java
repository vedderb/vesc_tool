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

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

// The IMU of the phone for the Virtual ESC, see phoneimu.h. Read returns
// gravity, acceleration (g), gyro (deg/s), magnetometer (uT) and yaw (rad).
public class PhoneImu implements SensorEventListener {
    // Without a magnetometer the yaw is relative to the start, like on a VESC
    private static final int[] SENSORS = {Sensor.TYPE_GRAVITY, Sensor.TYPE_ACCELEROMETER,
        Sensor.TYPE_GYROSCOPE, Sensor.TYPE_MAGNETIC_FIELD, Sensor.TYPE_ROTATION_VECTOR,
        Sensor.TYPE_GAME_ROTATION_VECTOR};
    private static PhoneImu mInstance = null;

    private final SensorManager mManager;
    private final float[] mValues = new float[13];
    private final float[] mRot = new float[9];
    private final float[] mOrientation = new float[3];
    private boolean mHasGravity = false;

    private PhoneImu(SensorManager manager) {
        mManager = manager;
    }

    private static SensorManager manager(Context context) {
        return (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
    }

    public static boolean available(Context context) {
        SensorManager m = manager(context);
        return m != null && m.getDefaultSensor(Sensor.TYPE_GRAVITY) != null;
    }

    public static synchronized void setActive(Context context, boolean active) {
        if (active && mInstance == null && available(context)) {
            mInstance = new PhoneImu(manager(context));
            SensorManager m = mInstance.mManager;
            boolean hasRotation = m.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR) != null;
            for (int type: SENSORS) {
                Sensor s = m.getDefaultSensor(type);
                if (s != null && !(hasRotation && type == Sensor.TYPE_GAME_ROTATION_VECTOR)) {
                    m.registerListener(mInstance, s, SensorManager.SENSOR_DELAY_GAME);
                }
            }
        } else if (!active && mInstance != null) {
            mInstance.mManager.unregisterListener(mInstance);
            mInstance = null;
        }
    }

    public static synchronized float[] read() {
        if (mInstance == null) {
            return null;
        }

        synchronized (mInstance) {
            return mInstance.mHasGravity ? mInstance.mValues.clone() : null;
        }
    }

    @Override
    public synchronized void onSensorChanged(SensorEvent e) {
        switch (e.sensor.getType()) {
        case Sensor.TYPE_GRAVITY:
            copy(e.values, 0, 1.0f / SensorManager.GRAVITY_EARTH);
            mHasGravity = true;
            break;
        case Sensor.TYPE_ACCELEROMETER:
            copy(e.values, 3, 1.0f / SensorManager.GRAVITY_EARTH);
            break;
        case Sensor.TYPE_GYROSCOPE:
            copy(e.values, 6, (float) (180.0 / Math.PI));
            break;
        case Sensor.TYPE_MAGNETIC_FIELD:
            copy(e.values, 9, 1.0f);
            break;
        case Sensor.TYPE_ROTATION_VECTOR:
        case Sensor.TYPE_GAME_ROTATION_VECTOR:
            // The azimuth is clockwise
            SensorManager.getRotationMatrixFromVector(mRot, e.values);
            SensorManager.getOrientation(mRot, mOrientation);
            mValues[12] = -mOrientation[0];
            break;
        default:
            break;
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

    private void copy(float[] values, int offset, float scale) {
        for (int i = 0;i < 3;i++) {
            mValues[offset + i] = values[i] * scale;
        }
    }
}
