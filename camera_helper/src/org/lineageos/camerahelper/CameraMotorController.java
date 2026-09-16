/*
 * Copyright (c) 2019 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.camerahelper;

import android.os.FileUtils;
import android.util.Log;

import java.io.File;
import java.io.IOException;

public class CameraMotorController {
    private static final String TAG = "CameraMotorController";

    // Camera motor paths
    private static final String CAMERA_MOTOR_ENABLE_PATH =
            "/sys/class/motor/enable";
    private static final String CAMERA_MOTOR_DIRECTION_PATH =
            "/sys/class/motor/direction";
    private static final String CAMERA_MOTOR_POSITION_PATH =
            "/sys/class/motor/position";

    // Motor control values
    public static final String DIRECTION_DOWN = "0";
    public static final String DIRECTION_UP = "1";
    public static final String ENABLED = "1";
    public static final String POSITION_DOWN = "1";
    public static final String POSITION_UP = "0";

    private CameraMotorController() {
        // This class is not supposed to be instantiated
    }

    public static void setMotorDirection(String direction) {
        try {
            FileUtils.stringToFile(CAMERA_MOTOR_DIRECTION_PATH, direction);
        } catch (IOException e) {
            Log.e(TAG, "Failed to write to " + CAMERA_MOTOR_DIRECTION_PATH, e);
        }
    }

    public static void setMotorEnabled() {
        try {
            FileUtils.stringToFile(CAMERA_MOTOR_ENABLE_PATH, ENABLED);
        } catch (IOException e) {
            Log.e(TAG, "Failed to write to " + CAMERA_MOTOR_ENABLE_PATH, e);
        }
    }

    public static String getMotorPosition() {
        try {
            return FileUtils.readTextFile(new File(CAMERA_MOTOR_POSITION_PATH), 1, null);
        } catch (IOException e) {
            Log.e(TAG, "Failed to read " + CAMERA_MOTOR_POSITION_PATH, e);
        }
        return null;
    }
}
