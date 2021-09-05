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

#include "AlsConfig.h"

#include <android-base/properties.h>
#include <gui/SurfaceComposerClient.h>

#include <cstdio>
#include <signal.h>
#include <unistd.h>
#include <utils/Timers.h>

using android::base::SetProperty;
using android::GraphicBuffer;
using android::Rect;
using android::ScreenshotClient;
using android::sp;
using android::SurfaceComposerClient;

void updateScreenBuffer() {
    static Rect screenshot_rect(ALS_SCREEN_RECT);
    static sp<GraphicBuffer> outBuffer = new GraphicBuffer(
            screenshot_rect.getWidth(), screenshot_rect.getHeight(),
            android::PIXEL_FORMAT_RGB_888,
            GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_OFTEN);

    ScreenshotClient::capture(
            SurfaceComposerClient::getInternalDisplayToken(),
            android::ui::Dataspace::V0_SRGB, android::ui::PixelFormat::RGBA_8888,
            screenshot_rect, screenshot_rect.getWidth(), screenshot_rect.getHeight(),
            false, android::ui::ROTATION_0, &outBuffer);

    uint8_t *out;
    auto resultWidth = outBuffer->getWidth();
    auto resultHeight = outBuffer->getHeight();
    auto stride = outBuffer->getStride();

    outBuffer->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, reinterpret_cast<void **>(&out));
    // we can sum this directly on linear light
    uint32_t rsum = 0, gsum = 0, bsum = 0;
    for (int y = 0; y < resultHeight; y++) {
        for (int x = 0; x < resultWidth; x++) {
            rsum += out[y * (stride * 4) + x * 4];
            gsum += out[y * (stride * 4) + x * 4 + 1];
            bsum += out[y * (stride * 4) + x * 4 + 2];
        }
    }
    uint32_t max = resultWidth * resultHeight;
    SetProperty("vendor.sensors.als_correction.r", std::to_string(rsum / max));
    SetProperty("vendor.sensors.als_correction.g", std::to_string(gsum / max));
    SetProperty("vendor.sensors.als_correction.b", std::to_string(bsum / max));
    nsecs_t now = systemTime(SYSTEM_TIME_BOOTTIME);
    SetProperty("vendor.sensors.als_correction.time", std::to_string(ns2ms(now)));
    outBuffer->unlock();
}

int main() {
    struct sigaction action{};
    sigfillset(&action.sa_mask);

    action.sa_flags = SA_RESTART;
    action.sa_handler = [](int signal) {
        if (signal == SIGUSR1) {
            updateScreenBuffer();
        }
    };

    if (sigaction(SIGUSR1, &action, nullptr) == -1) {
        return -1;
    }

    SetProperty("vendor.sensors.als_correction.pid", std::to_string(getpid()));

    while (true) {
        pause();
    }

    return 0;
}
