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

#define LOG_TAG "VibratorService"

#include <android-base/logging.h>
#include <hardware/hardware.h>
#include <hardware/vibrator.h>

#include "Vibrator.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

// Refer to non existing
// kernel documentation on the detail usages for ABIs below
static constexpr char ACTIVATE_PATH[] = "/sys/class/leds/vibrator/activate";
static constexpr char BRIGHTNESS_PATH[] = "/sys/class/leds/vibrator/brightness";
static constexpr char DURATION_PATH[] = "/sys/class/leds/vibrator/duration";
static constexpr char GAIN_PATH[] = "/sys/class/leds/vibrator/gain";
static constexpr char IGNORE_STORE_PATH[] = "/sys/class/leds/vibrator/ignore_store";
static constexpr char LOOP_PATH[] = "/sys/class/leds/vibrator/loop";
static constexpr char LP_TRIGGER_PATH[] = "/sys/class/leds/vibrator/haptic_audio";
static constexpr char SCALE_PATH[] = "/sys/class/leds/vibrator/gain";
static constexpr char SEQ_PATH[] = "/sys/class/leds/vibrator/seq";
static constexpr char STATE_PATH[] = "/sys/class/leds/vibrator/state";
static constexpr char VMAX_PATH[] = "/sys/class/leds/vibrator/vmax";

// General constants
static constexpr char GAIN[] = "0x80";
static constexpr char VMAX[] = "0x09";

// Effects
static constexpr AwEffect WAVEFORM_CLICK_EFFECT {
    .sequences = std::array<uint8_t, 8>({ 1, 0, 0, 0, 0, 0, 0, 0 }),
    .loops = std::array<uint8_t, 8>({ 0, 0, 0, 0, 0, 0, 0, 0 }),
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 0
};
static constexpr AwEffect WAVEFORM_TICK_EFFECT {
    .sequences = std::array<uint8_t, 8>({ 1, 0, 0, 0, 0, 0, 0, 0 }),
    .loops = std::array<uint8_t, 8>({ 0, 0, 0, 0, 0, 0, 0, 0 }),
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 0
};
static constexpr AwEffect WAVEFORM_DOUBLE_CLICK_EFFECT {
    .sequences = std::array<uint8_t, 8>({ 1, 0, 0, 0, 0, 0, 0, 0 }),
    .loops = std::array<uint8_t, 8>({ 0, 0, 0, 0, 0, 0, 0, 0 }),
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 10
};
static constexpr AwEffect WAVEFORM_HEAVY_CLICK_EFFECT {
    .sequences = std::array<uint8_t, 8>({ 1, 0, 0, 0, 0, 0, 0, 0 }),
    .loops = std::array<uint8_t, 8>({ 0, 0, 0, 0, 0, 0, 0, 0 }),
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 10
};
static constexpr AwEffect WAVEFORM_POP_EFFECT {
    .loops = std::array<uint8_t, 8>({ 0, 0, 0, 0, 0, 0, 0, 0 }),
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 5
};
static constexpr AwEffect WAVEFORM_THUD_EFFECT {
    .loops = std::array<uint8_t, 8>({ 0, 0, 0, 0, 0, 0, 0, 0 }),
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 10
};

namespace android {
namespace hardware {
namespace vibrator {
namespace V1_2 {
namespace implementation {

/*
 * Write value to path and close file.
 */
template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);

    if (!file.is_open()) {
        LOG(ERROR) << "Unable to open: " << path << " (" <<  strerror(errno) << ")";
        return;
    }

    file << value;
}

Vibrator::Vibrator() {
    // This enables effect #1 from the waveform library to be triggered by SLPI
    // while the AP is in suspend mode
    set(LP_TRIGGER_PATH, 1);
}

Return<Status> Vibrator::on(uint32_t timeoutMs) {
    set(STATE_PATH, 1);
    set(DURATION_PATH, timeoutMs);
    set(ACTIVATE_PATH, 1);
    return Status::OK;
}

Return<Status> Vibrator::off()  {
    set(BRIGHTNESS_PATH, 0);
    return Status::OK;
}

Return<bool> Vibrator::supportsAmplitudeControl()  {
    return false;
}

Return<Status> Vibrator::setAmplitude(uint8_t) {
    return Status::UNSUPPORTED_OPERATION;
}

Return<void> Vibrator::perform(V1_0::Effect effect, EffectStrength strength, perform_cb _hidl_cb) {
    return performEffect(static_cast<Effect>(effect), strength, _hidl_cb);
}

Return<void> Vibrator::perform_1_1(V1_1::Effect_1_1 effect, EffectStrength strength,
        perform_cb _hidl_cb) {
    return performEffect(static_cast<Effect>(effect), strength, _hidl_cb);
}

Return<void> Vibrator::perform_1_2(Effect effect, EffectStrength strength, perform_cb _hidl_cb) {
    return performEffect(static_cast<Effect>(effect), strength, _hidl_cb);
}

Return<void> Vibrator::performEffect(Effect effect, EffectStrength strength, perform_cb _hidl_cb) {
    const auto convertEffectStrength = [](EffectStrength strength) -> uint8_t {
        switch (strength) {
            case EffectStrength::LIGHT:
                return 54; // 50%
                break;
            case EffectStrength::MEDIUM:
            case EffectStrength::STRONG:
                return 107; // 100%
                break;
            default:
                return 0;
        }
    };

    const auto uint8ToHexString = [](uint8_t v) -> std::string {
        std::stringstream ss;
        ss << std::hex << "0x" << std::setfill('0') << std::setw(2) << static_cast<int>(v);
        return ss.str();
    };

    const auto setEffect = [&](const AwEffect& effect, uint32_t& timeMS) {
        set(ACTIVATE_PATH, 0);
        set(IGNORE_STORE_PATH, 0);

        if (effect.vmax.has_value()) {
            set(VMAX_PATH, *effect.vmax);
        }

        if (effect.gain.has_value()) {
            set(GAIN_PATH, *effect.gain);
        }

        if (effect.sequences.has_value()) {
            for (size_t i = 0; i < effect.sequences->size(); i++) {
                set(SEQ_PATH,
                        uint8ToHexString(i) + " " + uint8ToHexString(effect.sequences->at(i)));
            }
        }

        if (effect.loops.has_value()) {
            for (size_t i = 0; i < effect.loops->size(); i++) {
                set(LOOP_PATH,
                        uint8ToHexString(i) + " " + uint8ToHexString(effect.loops->at(i)));
            }
        }

        set(SCALE_PATH, convertEffectStrength(strength));
        set(DURATION_PATH, effect.timeMS);
        set(BRIGHTNESS_PATH, 1);

        timeMS = effect.timeMS;
    };

    Status status = Status::OK;
    uint32_t timeMS;

    switch (effect) {
        case Effect::CLICK:
            setEffect(WAVEFORM_CLICK_EFFECT, timeMS);
            break;
        case Effect::DOUBLE_CLICK:
            setEffect(WAVEFORM_DOUBLE_CLICK_EFFECT, timeMS);
            break;
        case Effect::TICK:
            setEffect(WAVEFORM_TICK_EFFECT, timeMS);
            break;
        case Effect::HEAVY_CLICK:
            setEffect(WAVEFORM_HEAVY_CLICK_EFFECT, timeMS);
            break;
        case Effect::POP:
            setEffect(WAVEFORM_POP_EFFECT, timeMS);
            break;
        case Effect::THUD:
            setEffect(WAVEFORM_THUD_EFFECT, timeMS);
            break;
        default:
            _hidl_cb(Status::UNSUPPORTED_OPERATION, 0);
            return Void();
    }

    _hidl_cb(status, timeMS);
    return Void();
}

}  // namespace implementation
}  // namespace V1_2
}  // namespace vibrator
}  // namespace hardware
}  // namespace android
