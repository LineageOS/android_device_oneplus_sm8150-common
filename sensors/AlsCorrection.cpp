/*
 * Copyright (C) 2021 The LineageOS Project
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

#include "AlsCorrection.h"

#include <cutils/properties.h>
#include <optional>
#include <fstream>
#include <log/log.h>
#include <time.h>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace implementation {

static int red_max_lux, green_max_lux, blue_max_lux, white_max_lux, max_brightness;
static int als_bias;

static std::optional<int32_t> property_get_opt(const char *key) {
    char buf[PROPERTY_VALUE_MAX] = {};
    int len = property_get(key, buf, "");
    if (len) {
        return atoi(buf);
    }
    return {};
}

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

template <typename T>
static std::optional<T> get(const std::string& path) {
    std::ifstream file(path);
    T result;

    file >> result;
    if (file.fail()){
        return {};
    } else {
        return {result};
    }
}

void AlsCorrection::init() {
    red_max_lux = get<int32_t>("/mnt/vendor/persist/als_correction.lineage/red_max_lux").value_or(
                    property_get_opt("ro.vendor.sensors.als_correction_defaults.red_max_lux").value_or(
                        get("/mnt/vendor/persist/engineermode/red_max_lux", 0)));
    green_max_lux = get<int32_t>("/mnt/vendor/persist/als_correction.lineage/green_max_lux").value_or(
                        property_get_opt("ro.vendor.sensors.als_correction_defaults.green_max_lux").value_or(
                            get("/mnt/vendor/persist/engineermode/green_max_lux", 0)));
    blue_max_lux = get<int32_t>("/mnt/vendor/persist/als_correction.lineage/blue_max_lux").value_or(
                        property_get_opt("ro.vendor.sensors.als_correction_defaults.blue_max_lux").value_or(
                            get("/mnt/vendor/persist/engineermode/blue_max_lux", 0)));
    white_max_lux = get<int32_t>("/mnt/vendor/persist/als_correction.lineage/white_max_lux").value_or(
                        property_get_opt("ro.vendor.sensors.als_correction_defaults.white_max_lux").value_or(
                            get("/mnt/vendor/persist/engineermode/white_max_lux", 0)));
    als_bias = get<int32_t>("/mnt/vendor/persist/als_correction.lineage/als_bias").value_or(
                    property_get_opt("ro.vendor.sensors.als_correction_defaults.als_bias").value_or(
                        get("/mnt/vendor/persist/engineermode/als_bias", 0)));
    max_brightness = property_get_opt("").value_or(get("/sys/class/backlight/panel0-backlight/max_brightness", 255));
    ALOGV("max r = %d, max g = %d, max b = %d", red_max_lux, green_max_lux, blue_max_lux);
}

void AlsCorrection::correct(float& light) {
    pid_t pid = property_get_int32("vendor.sensors.als_correction.pid", 0);
    if (pid != 0) {
        kill(pid, SIGUSR1);
    }
    // TODO: HIDL service and pass float instead
    int32_t r = property_get_int32("vendor.sensors.als_correction.r", 0);
    int32_t g = property_get_int32("vendor.sensors.als_correction.g", 0);
    int32_t b = property_get_int32("vendor.sensors.als_correction.b", 0);
    ALOGV("Screen Color Above Sensor: %d, %d, %d", r, g, b);
    ALOGV("Original reading: %f", light);
    int screen_brightness = get("/sys/class/backlight/panel0-backlight/brightness", 0);
    float correction = 0.0f;
    if (red_max_lux > 0 && green_max_lux > 0 && blue_max_lux > 0 && white_max_lux > 0) {
        constexpr float rgb_scale = 0x7FFFFFFF;
        uint16_t rgb_min = std::min({r, g, b});
        correction += ((float) rgb_min) / rgb_scale * ((float) white_max_lux);
        r -= rgb_min;
        g -= rgb_min;
        b -= rgb_min;
        correction += ((float) r) / rgb_scale * ((float) red_max_lux);
        correction += ((float) g) / rgb_scale * ((float) green_max_lux);
        correction += ((float) b) / rgb_scale * ((float) blue_max_lux);
        correction = correction * (((float) screen_brightness) / ((float) max_brightness));
        correction += als_bias;
    }
    // Do not apply correction if < 0, prevent unstable adaptive brightness
    if (light - correction >= 0) {
        light -= correction;
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
