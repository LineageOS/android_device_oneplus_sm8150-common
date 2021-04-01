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

#include <android-base/properties.h>
#include <gui/SurfaceComposerClient.h>

#include <cstdio>
#include <signal.h>
#include <time.h>
#include <unistd.h>

using android::base::SetProperty;
using android::GraphicBuffer;
using android::Rect;
using android::ScreenshotClient;
using android::sp;
using android::SurfaceComposerClient;

constexpr int SCREENSHOT_INTERVAL = 1;

void updateScreenBuffer() {
    static time_t lastScreenUpdate = 0;
    static sp<GraphicBuffer> outBuffer = new GraphicBuffer(
            10, 10, android::PIXEL_FORMAT_RGB_888,
            GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_OFTEN);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (now.tv_sec - lastScreenUpdate >= SCREENSHOT_INTERVAL) {
        // Update Screenshot at most every second
        ScreenshotClient::capture(SurfaceComposerClient::getInternalDisplayToken(),
                                  android::ui::Dataspace::V0_SRGB,
                                  android::ui::PixelFormat::RGBA_8888,
                                  Rect(ALS_POS_X, ALS_POS_Y, ALS_POS_X + 10, ALS_POS_Y + 10),
                                  10, 10, true, android::ui::ROTATION_0, &outBuffer);
        lastScreenUpdate = now.tv_sec;
    }

    uint8_t *out;
    outBuffer->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, reinterpret_cast<void **>(&out));
    SetProperty("vendor.sensors.als_correction.r", std::to_string(static_cast<uint8_t>(out[0])));
    SetProperty("vendor.sensors.als_correction.g", std::to_string(static_cast<uint8_t>(out[1])));
    SetProperty("vendor.sensors.als_correction.b", std::to_string(static_cast<uint8_t>(out[2])));
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
        sleep(1);
    }

    return 0;
}
