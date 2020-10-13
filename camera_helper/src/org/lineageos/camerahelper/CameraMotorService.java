/*
 * Copyright (C) 2019 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.lineageos.camerahelper;

import android.annotation.NonNull;
import android.app.Service;
import android.content.Intent;
import android.hardware.camera2.CameraManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.SystemClock;
import android.util.Log;

public class CameraMotorService extends Service implements Handler.Callback {
    private static final boolean DEBUG = true;
    private static final String TAG = "CameraMotorService";

    public static final int CAMERA_EVENT_DELAY_TIME = 100; // ms

    public static final String FRONT_CAMERA_ID = "1";

    public static final int MSG_CAMERA_CLOSED = 1000;
    public static final int MSG_CAMERA_OPEN = 1001;

    private Handler mHandler = new Handler(this);

    private long mClosedEvent;
    private long mOpenEvent;

    private CameraManager.AvailabilityCallback mAvailabilityCallback =
            new CameraManager.AvailabilityCallback() {
                @Override
                public void onCameraAvailable(@NonNull String cameraId) {
                    super.onCameraAvailable(cameraId);

                    if (cameraId.equals(FRONT_CAMERA_ID)) {
                        mClosedEvent = SystemClock.elapsedRealtime();
                        if (SystemClock.elapsedRealtime() - mOpenEvent < CAMERA_EVENT_DELAY_TIME
                                && mHandler.hasMessages(MSG_CAMERA_OPEN)) {
                            mHandler.removeMessages(MSG_CAMERA_OPEN);
                        }
                        mHandler.sendEmptyMessageDelayed(MSG_CAMERA_CLOSED,
                                CAMERA_EVENT_DELAY_TIME);
                    }
                }

                @Override
                public void onCameraUnavailable(@NonNull String cameraId) {
                    super.onCameraAvailable(cameraId);

                    if (cameraId.equals(FRONT_CAMERA_ID)) {
                        mOpenEvent = SystemClock.elapsedRealtime();
                        if (SystemClock.elapsedRealtime() - mClosedEvent < CAMERA_EVENT_DELAY_TIME
                                && mHandler.hasMessages(MSG_CAMERA_CLOSED)) {
                            mHandler.removeMessages(MSG_CAMERA_CLOSED);
                        }
                        mHandler.sendEmptyMessageDelayed(MSG_CAMERA_OPEN,
                                CAMERA_EVENT_DELAY_TIME);
                    }
                }
            };

    @Override
    public void onCreate() {
        CameraMotorController.calibrate();

        CameraManager cameraManager = getSystemService(CameraManager.class);
        cameraManager.registerAvailabilityCallback(mAvailabilityCallback, null);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (DEBUG) Log.d(TAG, "Starting service");
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG, "Destroying service");
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public boolean handleMessage(Message msg) {
        switch (msg.what) {
            case MSG_CAMERA_CLOSED:
                CameraMotorController.setMotorDirection(CameraMotorController.DIRECTION_DOWN);
                CameraMotorController.setMotorEnabled();
                break;
            case MSG_CAMERA_OPEN:
                CameraMotorController.setMotorDirection(CameraMotorController.DIRECTION_UP);
                CameraMotorController.setMotorEnabled();
                break;
        }
        return true;
    }
}
