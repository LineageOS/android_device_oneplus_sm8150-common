/*
 * Copyright (c) 2019 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.camerahelper;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class BootCompletedReceiver extends BroadcastReceiver {
    private static final String TAG = "OnePlusCameraHelper";

    @Override
    public void onReceive(final Context context, Intent intent) {
        Log.d(TAG, "Starting");
        context.startService(new Intent(context, CameraMotorService.class));
        context.startService(new Intent(context, FallSensorService.class));
    }
}
