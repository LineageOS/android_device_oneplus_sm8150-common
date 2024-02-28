/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "AlsCorrection.h"

#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <fstream>
#include <log/log.h>
#include <utils/Timers.h>

using aidl::vendor::lineage::oplus_als::AreaRgbCaptureResult;
using aidl::vendor::lineage::oplus_als::IAreaCapture;
using android::base::GetIntProperty;

#define ALS_CALI_DIR "/proc/sensor/als_cali/"
#define BRIGHTNESS_DIR "/sys/class/backlight/panel0-backlight/"

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace implementation {

static int red_max_lux, green_max_lux, blue_max_lux, white_max_lux, max_brightness;
static int als_bias;
static std::shared_ptr<IAreaCapture> service;

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

void AlsCorrection::init() {
    als_bias = GetIntProperty("vendor.sensors.als_correction.bias", 0);
    red_max_lux = get(ALS_CALI_DIR "red_max_lux", 0);
    green_max_lux = get(ALS_CALI_DIR "green_max_lux", 0);
    blue_max_lux = get(ALS_CALI_DIR "blue_max_lux", 0);
    white_max_lux = get(ALS_CALI_DIR "white_max_lux", 0);
    max_brightness = get(BRIGHTNESS_DIR "max_brightness", 1023.0);
    ALOGV("Display maximums: R=%d G=%d B=%d W=%d, Brightness=%d",
        red_max_lux, green_max_lux, blue_max_lux, white_max_lux, max_brightness);

    const auto instancename = std::string(IAreaCapture::descriptor) + "/default";

    if (AServiceManager_isDeclared(instancename.c_str())) {
        service = IAreaCapture::fromBinder(::ndk::SpAIBinder(
            AServiceManager_waitForService(instancename.c_str())));
    } else {
        ALOGE("Service is not registered");
    }
}

void AlsCorrection::correct(float& light) {
    static nsecs_t last_update = 0;
    static AreaRgbCaptureResult screenshot = { 0.0, 0.0, 0.0 };

    nsecs_t now = systemTime(SYSTEM_TIME_BOOTTIME);
    if ((now - last_update) > ms2ns(500)) {
        if (service != nullptr && service->getAreaBrightness(&screenshot).isOk()) {
            last_update = now;
        } else {
            ALOGE("Could not get area above sensor");
        }
    } else {
        ALOGV("Capture skipped because interval not expired at %ld", now);
    }

    float r = screenshot.r / 255, g = screenshot.g / 255, b = screenshot.b / 255;
    ALOGV("Screen Color Above Sensor: %f, %f, %f", r, g, b);
    ALOGV("Original reading: %f", light);
    int screen_brightness = get(BRIGHTNESS_DIR "brightness", 0);
    float correction = 0.0f, correction_scaled = 0.0f;
    if (red_max_lux > 0 && green_max_lux > 0 && blue_max_lux > 0 && white_max_lux > 0) {
        float rgb_min = std::min({r, g, b});
        r -= rgb_min;
        g -= rgb_min;
        b -= rgb_min;
        correction += rgb_min * ((float) white_max_lux);
        correction += r * ((float) red_max_lux);
        correction += g * ((float) green_max_lux);
        correction += b * ((float) blue_max_lux);
        correction = correction * (((float) screen_brightness) / ((float) max_brightness));
        correction += als_bias;
        correction_scaled = correction * (((float) white_max_lux) /
                                          (red_max_lux + green_max_lux + blue_max_lux));
    }
    if (light - correction >= 0) {
        // Apply correction if light - correction >= 0
        light -= correction;
    } else if (light - correction > -4) {
        // Return positive value if light - correction > -4
        light = correction - light;
    } else if (light - correction_scaled >= 0) {
        // Substract scaled correction if light - correction_scaled >= 0
        light -= correction_scaled;
    } else {
        // In low light conditions, sensor is just reporting bad values, using
        // computed correction instead allows to fix the issue
        light = correction;
    }
    ALOGV("Corrected reading: %f", light);
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android
