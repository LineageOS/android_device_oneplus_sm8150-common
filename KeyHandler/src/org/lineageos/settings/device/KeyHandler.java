/*
 * Copyright (C) 2018 The LineageOS Project
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

package org.lineageos.settings.device;

import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Looper;
import android.os.Vibrator;
import android.util.Log;
import android.view.KeyEvent;
import android.view.WindowManager;

import com.android.internal.os.DeviceKeyHandler;

public class KeyHandler implements DeviceKeyHandler {
    private static final String TAG = KeyHandler.class.getSimpleName();

    // Slider key codes
    private static final int MODE_NORMAL = 601;
    private static final int MODE_VIBRATION = 602;
    private static final int MODE_SILENCE = 603;

    // Camera motor press sensor key codes
    private static final int KEY_F14 = 0x00b8;

    private final Context mContext;
    private final AudioManager mAudioManager;
    private final Vibrator mVibrator;

    public KeyHandler(Context context) {
        mContext = context;

        mAudioManager = mContext.getSystemService(AudioManager.class);
        mVibrator = mContext.getSystemService(Vibrator.class);
    }

    public KeyEvent handleKeyEvent(KeyEvent event) {
        int scanCode = event.getScanCode();

        switch (scanCode) {
            case MODE_NORMAL:
                mAudioManager.setRingerModeInternal(AudioManager.RINGER_MODE_NORMAL);
                break;
            case MODE_VIBRATION:
                mAudioManager.setRingerModeInternal(AudioManager.RINGER_MODE_VIBRATE);
                break;
            case MODE_SILENCE:
                mAudioManager.setRingerModeInternal(AudioManager.RINGER_MODE_SILENT);
                break;
            case KEY_F14:
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    new Handler(Looper.getMainLooper()).post(() -> {
                        showCameraMotorPressWarning();
                    });
                }
                break;
            default:
                return event;
        }
        doHapticFeedback();

        return null;
    }

    private void doHapticFeedback() {
        if (mVibrator == null || !mVibrator.hasVibrator()) {
            return;
        }

        mVibrator.vibrate(50);
    }

    /*
     * On OnePlus 7 Pro, alert the user to avoid pressing down the
     * front-facing camera manually.
     */
    private void showCameraMotorPressWarning() {
        // Go back to home to close all camera apps first
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_HOME);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(intent);

        // Show the alert
        Context packageContext;
        try {
            packageContext = mContext.createPackageContext("org.lineageos.settings.device", 0);
        } catch (PackageManager.NameNotFoundException | SecurityException e) {
            Log.e(TAG, Log.getStackTraceString(e));
            return;
        }
        AlertDialog dialog = new AlertDialog.Builder(packageContext)
                .setTitle(R.string.motor_press_title)
                .setMessage(R.string.motor_press_message)
                .setPositiveButton(R.string.motor_press_ok, (diag, which) -> {
                    diag.dismiss();
                })
                .create();
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }
}
