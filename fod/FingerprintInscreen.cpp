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

#define LOG_TAG "FingerprintInscreenService"

#include "FingerprintInscreen.h"
#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <fstream>

#define FINGERPRINT_ACQUIRED_VENDOR 6
#define FINGERPRINT_ERROR_VENDOR 8

#define OP_ENABLE_FP_LONGPRESS 3
#define OP_DISABLE_FP_LONGPRESS 4
#define OP_RESUME_FP_ENROLL 8
#define OP_FINISH_FP_ENROLL 10

#define OP_DISPLAY_AOD_MODE 8
#define OP_DISPLAY_NOTIFY_PRESS 9
#define OP_DISPLAY_SET_DIM 10

namespace vendor {
namespace lineage {
namespace biometrics {
namespace fingerprint {
namespace inscreen {
namespace V1_0 {
namespace implementation {

/*
 * Write value to path and close file.
 */
template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

struct ba {
    std::uint32_t brightness;
    std::uint32_t alpha;
};

struct ba brightness_alpha_lut_1[] = {
    {0, 0xff},
    {1, 0xf1},
    {2, 0xec},
    {4, 0xeb},
    {5, 0xea},
    {6, 0xe8},
    {10, 0xe4},
    {20, 0xdc},
    {30, 0xd4},
    {45, 0xcc},
    {70, 0xbe},
    {100, 0xb3},
    {150, 0xa6},
    {227, 0x90},
    {300, 0x83},
    {400, 0x70},
    {500, 0x60},
    {600, 0x53},
    {800, 0x3c},
    {1023, 0x22},
    {2000, 0x83},
};

struct ba brightness_alpha_lut[21] = {};

static int interpolate(int x, int xa, int xb, int ya, int yb) {
    int bf, factor, plus;
    int sub = 0;

    bf = 2 * (yb - ya) * (x - xa) / (xb - xa);
    factor = bf / 2;
    plus = bf % 2;
    if ((xa - xb) && (yb - ya))
        sub = 2 * (x - xa) * (x - xb) / (yb - ya) / (xa - xb);

    return ya + factor + plus + sub;
}

int brightness_to_alpha(int brightness) {
    int level = ARRAY_SIZE(brightness_alpha_lut);
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(brightness_alpha_lut); i++){
        if (brightness_alpha_lut[i].brightness >= brightness)
            break;
    }

    if (i == 0)
        return brightness_alpha_lut[0].alpha;
    else if (i == level)
        return brightness_alpha_lut[level - 1].alpha;

    return interpolate(brightness,
            brightness_alpha_lut[i-1].brightness,
            brightness_alpha_lut[i].brightness,
            brightness_alpha_lut[i-1].alpha,
            brightness_alpha_lut[i].alpha);
}

FingerprintInscreen::FingerprintInscreen() {
    int i;
    this->mFodCircleVisible = false;
    this->mVendorFpService = IVendorFingerprintExtensions::getService();
    this->mVendorDisplayService = IOneplusDisplay::getService();
    for (i = 0; i < 21; i++)
        brightness_alpha_lut[i] = brightness_alpha_lut_1[i];
}

Return<void> FingerprintInscreen::onStartEnroll() {
    this->mVendorFpService->updateStatus(OP_DISABLE_FP_LONGPRESS);
    this->mVendorFpService->updateStatus(OP_RESUME_FP_ENROLL);

    return Void();
}

Return<void> FingerprintInscreen::onFinishEnroll() {
    this->mVendorFpService->updateStatus(OP_FINISH_FP_ENROLL);

    return Void();
}

Return<void> FingerprintInscreen::onPress() {
    this->mVendorDisplayService->setMode(OP_DISPLAY_NOTIFY_PRESS, 1);

    return Void();
}

Return<void> FingerprintInscreen::onRelease() {
    this->mVendorDisplayService->setMode(OP_DISPLAY_NOTIFY_PRESS, 0);

    return Void();
}

Return<void> FingerprintInscreen::onShowFODView() {
    this->mFodCircleVisible = true;
    this->mVendorDisplayService->setMode(OP_DISPLAY_SET_DIM, 1);

    return Void();
}

Return<void> FingerprintInscreen::onHideFODView() {
    this->mFodCircleVisible = false;
    this->mVendorDisplayService->setMode(OP_DISPLAY_SET_DIM, 0);
    this->mVendorDisplayService->setMode(OP_DISPLAY_NOTIFY_PRESS, 0);

    return Void();
}

Return<bool> FingerprintInscreen::handleAcquired(int32_t acquiredInfo, int32_t vendorCode) {
    std::lock_guard<std::mutex> _lock(mCallbackLock);
    if (mCallback == nullptr) {
        return false;
    }

    if (acquiredInfo == FINGERPRINT_ACQUIRED_VENDOR) {
        if (mFodCircleVisible && vendorCode == 0) {
            Return<void> ret = mCallback->onFingerDown();
            if (!ret.isOk()) {
                LOG(ERROR) << "FingerDown() error: " << ret.description();
            }
            return true;
        }

        if (mFodCircleVisible && vendorCode == 1) {
            Return<void> ret = mCallback->onFingerUp();
            if (!ret.isOk()) {
                LOG(ERROR) << "FingerUp() error: " << ret.description();
            }
            return true;
        }
    }

    return false;
}

Return<bool> FingerprintInscreen::handleError(int32_t error, int32_t vendorCode) {
    return error == FINGERPRINT_ERROR_VENDOR && vendorCode == 6;
}

Return<void> FingerprintInscreen::setLongPressEnabled(bool enabled) {
    this->mVendorFpService->updateStatus(
            enabled ? OP_ENABLE_FP_LONGPRESS : OP_DISABLE_FP_LONGPRESS);

    return Void();
}

Return<int32_t> FingerprintInscreen::getDimAmount(int32_t brightness) {
    int realBrightness = brightness * 1023 / 255;
    int dimAmount = (brightness_to_alpha(realBrightness) * 70) / 100;

    LOG(INFO) << "dimAmount = " << dimAmount;

    return dimAmount;
}

Return<bool> FingerprintInscreen::shouldBoostBrightness() {
    return false;
}

Return<void> FingerprintInscreen::setCallback(const sp<IFingerprintInscreenCallback>& callback) {
    {
        std::lock_guard<std::mutex> _lock(mCallbackLock);
        mCallback = callback;
    }

    return Void();
}

Return<int32_t> FingerprintInscreen::getPositionX() {
    return FOD_POS_X;
}

Return<int32_t> FingerprintInscreen::getPositionY() {
    return FOD_POS_Y;
}

Return<int32_t> FingerprintInscreen::getSize() {
    return FOD_SIZE;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace inscreen
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace lineage
}  // namespace vendor
