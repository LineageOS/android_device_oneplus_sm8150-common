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
#include <iostream>

// Refer to non existing
// kernel documentation on the detail usages for ABIs below
static constexpr char ACTIVATE_PATH[] = "/sys/class/leds/vibrator/activate";
static constexpr char BRIGHTNESS_PATH[] = "/sys/class/leds/vibrator/brightness";
static constexpr char CTRL_LOOP_PATH[] = "/sys/class/leds/vibrator/loop";
static constexpr char DURATION_PATH[] = "/sys/class/leds/vibrator/duration";
static constexpr char GAIN_PATH[] = "/sys/class/leds/vibrator/gain";
static constexpr char IGNORE_STORE_PATH[] = "/sys/class/leds/vibrator/ignore_store";
static constexpr char LP_TRIGGER_PATH[] = "/sys/class/leds/vibrator/haptic_audio";
static constexpr char LRA_WAVE_SHAPE_PATH[] = "/sys/class/leds/vibrator/lra_resistance";
static constexpr char MODE_PATH[] = "/sys/class/leds/vibrator/activate_mode";
static constexpr char RTP_INPUT_PATH[] = "/sys/class/leds/vibrator/rtp";
static constexpr char SCALE_PATH[] = "/sys/class/leds/vibrator/gain";
static constexpr char SEQ_PATH[] = "/sys/class/leds/vibrator/seq";
static constexpr char VMAX_PATH[] = "/sys/class/leds/vibrator/vmax";

// RTP mode
static constexpr char RTP_MODE[] = "rtp";
static constexpr uint8_t MAX_RTP_INPUT = 127;
static constexpr uint8_t MIN_RTP_INPUT = 0;

// Waveform mode
static constexpr char WAVEFORM_MODE[] = "waveform";
static constexpr uint8_t SQUARE_WAVE = 0;
static constexpr uint8_t SINE_WAVE = 1;

// General constants
static constexpr uint8_t GAIN = 128;
static constexpr uint8_t LOOP_MODE_OPEN = 1;
static constexpr uint8_t VMAX = 9;

// Effects
static const AwEffect WAVEFORM_CLICK_EFFECT {
    .sequences = std::vector<std::string>({ "0 1", "1 0" }),
    .ctrlLoops = std::vector<std::string>({ "0 0x0" }),
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 0
};
static const AwEffect WAVEFORM_TICK_EFFECT {
    .sequences = std::vector<std::string>({ "0 1", "1 0" }),
    .ctrlLoops = std::vector<std::string>({ "1 0x0", "0 0x0" }),
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 0
};
static const AwEffect WAVEFORM_DOUBLE_CLICK_EFFECT {
    .sequences = std::vector<std::string>({ "0 1" }),
    .ctrlLoops = std::vector<std::string>({ "0 0x0", "1 0x0", "0 0x0" }),
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 10
};
static const AwEffect WAVEFORM_HEAVY_CLICK_EFFECT {
    .sequences = std::vector<std::string>({ "0 1", "1 0" }),
    .ctrlLoops = std::vector<std::string>({ "1 0x1", "0 0x0" }),
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 10
};
static const AwEffect WAVEFORM_POP_EFFECT {
    .duration = 0,
    .vmax = VMAX,
    .gain = GAIN,
    .timeMS = 5
};
static const AwEffect WAVEFORM_THUD_EFFECT {
    .duration = 0,
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

Return<Status> Vibrator::on(uint32_t timeoutMs, bool isWaveform) {
    set(CTRL_LOOP_PATH, LOOP_MODE_OPEN);
    set(DURATION_PATH, timeoutMs);

    if (isWaveform) {
        set(MODE_PATH, WAVEFORM_MODE);
        set(LRA_WAVE_SHAPE_PATH, SINE_WAVE);
    } else {
        set(MODE_PATH, RTP_MODE);
        set(LRA_WAVE_SHAPE_PATH, SQUARE_WAVE);
    }

    if (mShouldSetBrightness) {
        set(BRIGHTNESS_PATH, 1);
    } else {
        set(BRIGHTNESS_PATH, 0);
        set(ACTIVATE_PATH, 1);
    }

    return Status::OK;
}

Return<Status> Vibrator::on(uint32_t timeoutMs) {
    mShouldSetBrightness = false;
    return on(timeoutMs, false /* isWaveform */);
}

Return<Status> Vibrator::off()  {
    set(BRIGHTNESS_PATH, 0);
    set(ACTIVATE_PATH, 0);
    return Status::OK;
}

Return<bool> Vibrator::supportsAmplitudeControl()  {
    return true;
}

Return<Status> Vibrator::setAmplitude(uint8_t amplitude) {
    if (amplitude == 0) {
        return Status::BAD_VALUE;
    }

    int32_t value = std::round((amplitude - 1) / 254.0 * (MAX_RTP_INPUT - MIN_RTP_INPUT) +
           MIN_RTP_INPUT);
    set(RTP_INPUT_PATH, value);

    return Status::OK;
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

    const auto setEffect = [](const AwEffect& effect, uint32_t& timeMS) {
        set(ACTIVATE_PATH, 0);
        set(IGNORE_STORE_PATH, 0);

        if (effect.duration.has_value()) {
            set(DURATION_PATH, *effect.duration);
        }

        if (effect.vmax.has_value()) {
            set(VMAX_PATH, *effect.vmax);
        }

        if (effect.gain.has_value()) {
            set(GAIN_PATH, *effect.gain);
        }

        if (effect.sequences.has_value()) {
            for (const auto& sequence : *effect.sequences) {
                set(SEQ_PATH, sequence);
            }
        }

        if (effect.ctrlLoops.has_value()) {
            for (const auto& ctrlLoop : *effect.ctrlLoops) {
                set(CTRL_LOOP_PATH, ctrlLoop);
            }
        }

        timeMS = effect.timeMS;
    };

    Status status = Status::OK;
    uint32_t timeMS;

    switch (effect) {
        case Effect::CLICK:
            setEffect(WAVEFORM_CLICK_EFFECT, timeMS);
            mShouldSetBrightness = true;
            break;
        case Effect::DOUBLE_CLICK:
            setEffect(WAVEFORM_DOUBLE_CLICK_EFFECT, timeMS);
            mShouldSetBrightness = true;
            break;
        case Effect::TICK:
            setEffect(WAVEFORM_TICK_EFFECT, timeMS);
            mShouldSetBrightness = true;
            break;
        case Effect::HEAVY_CLICK:
            setEffect(WAVEFORM_HEAVY_CLICK_EFFECT, timeMS);
            mShouldSetBrightness = true;
            break;
        case Effect::POP:
            setEffect(WAVEFORM_POP_EFFECT, timeMS);
            mShouldSetBrightness = true;
            break;
        case Effect::THUD:
            setEffect(WAVEFORM_THUD_EFFECT, timeMS);
            mShouldSetBrightness = true;
            break;
        default:
            _hidl_cb(Status::UNSUPPORTED_OPERATION, 0);
            mShouldSetBrightness = false;
            return Void();
    }

    set(SCALE_PATH, convertEffectStrength(strength));

    on(timeMS, true /* isWaveform */);

    _hidl_cb(status, timeMS);
    return Void();
}

}  // namespace implementation
}  // namespace V1_2
}  // namespace vibrator
}  // namespace hardware
}  // namespace android
