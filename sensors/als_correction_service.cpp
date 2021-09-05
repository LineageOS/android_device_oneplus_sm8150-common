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

#include "AsyncScreenCaptureListener.h"

#include <android-base/properties.h>
#include <binder/ProcessState.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SyncScreenCaptureListener.h>
#include <ui/DisplayState.h>

#include <cstdio>
#include <signal.h>
#include <unistd.h>
#include <sysutils/FrameworkCommand.h>
#include <sysutils/FrameworkListener.h>
#include <utils/Timers.h>

using android::base::SetProperty;
using android::gui::ScreenCaptureResults;
using android::ui::DisplayState;
using android::ui::PixelFormat;
using android::AsyncScreenCaptureListener;
using android::DisplayCaptureArgs;
using android::GraphicBuffer;
using android::IBinder;
using android::Rect;
using android::ScreenshotClient;
using android::sp;
using android::SurfaceComposerClient;

#if defined(DEVICE_guacamole) || defined(DEVICE_guacamoleg)
static Rect screenshot_rect(251, 988, 305, 1042);
#elif defined(DEVICE_guacamoleb)
static Rect screenshot_rect(626, 192, 666, 232);
#elif defined(DEVICE_hotdog) || defined(DEVICE_hotdogg)
static Rect screenshot_rect(255, 903, 301, 949);
#elif defined(DEVICE_hotdogb)
static Rect screenshot_rect(499, 110, 525, 136);
#else
#error No ALS configuration for this device
#endif

<<<<<<< HEAD   (80ba8a sm8150-common: Drop console from kernel cmdline)
// See frameworks/base/services/core/jni/com_android_server_display_DisplayControl.cpp and
// frameworks/base/core/java/android/view/SurfaceControl.java
static sp<IBinder> getInternalDisplayToken() {
    const auto displayIds = SurfaceComposerClient::getPhysicalDisplayIds();
    sp<IBinder> token = SurfaceComposerClient::getPhysicalDisplayToken(displayIds[0]);
    return token;
}

void updateScreenBuffer() {
    static time_t lastScreenUpdate = 0;
    static sp<GraphicBuffer> outBuffer = new GraphicBuffer(
            10, 10, android::PIXEL_FORMAT_RGB_888,
            GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_OFTEN);
=======
class TakeScreenshotCommand : public FrameworkCommand {
  public:
    TakeScreenshotCommand() : FrameworkCommand("take_screenshot") {}
    ~TakeScreenshotCommand() override = default;
>>>>>>> CHANGE (f1f730 sm8150-common: implement ALS compensation algorithm)

<<<<<<< HEAD   (80ba8a sm8150-common: Drop console from kernel cmdline)
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (now.tv_sec - lastScreenUpdate < SCREENSHOT_INTERVAL) {
        ALOGV("Update skipped because interval not expired at %ld", now.tv_sec);
        return;
=======
    int runCommand(SocketClient* cli, int /*argc*/, char **/*argv*/) {
        auto screenshot = takeScreenshot();
        cli->sendData(&screenshot, sizeof(screenshot_t));
        return 0;
>>>>>>> CHANGE (f1f730 sm8150-common: implement ALS compensation algorithm)
    }
<<<<<<< HEAD   (80ba8a sm8150-common: Drop console from kernel cmdline)

    sp<IBinder> display = getInternalDisplayToken();

    DisplayCaptureArgs captureArgs;
    captureArgs.displayToken = getInternalDisplayToken();
    captureArgs.pixelFormat = PixelFormat::RGBA_8888;
    captureArgs.sourceCrop = Rect(
            ALS_POS_X - ALS_RADIUS, ALS_POS_Y - ALS_RADIUS,
            ALS_POS_X + ALS_RADIUS, ALS_POS_Y + ALS_RADIUS);
    captureArgs.width = ALS_RADIUS * 2;
    captureArgs.height = ALS_RADIUS * 2;
    captureArgs.useIdentityTransform = true;
    captureArgs.captureSecureLayers = true;

    DisplayState state;
    SurfaceComposerClient::getDisplayState(display, &state);

    sp<AsyncScreenCaptureListener> captureListener = new AsyncScreenCaptureListener(
        [](const ScreenCaptureResults& captureResults) {
            ALOGV("Capture results received");

            uint8_t *out;
            auto resultWidth = outBuffer->getWidth();
            auto resultHeight = outBuffer->getHeight();
            auto stride = outBuffer->getStride();

            captureResults.buffer->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, reinterpret_cast<void **>(&out));
            // we can sum this directly on linear light
            uint32_t rsum = 0, gsum = 0, bsum = 0;
            for (int y = 0; y < resultHeight; y++) {
                for (int x = 0; x < resultWidth; x++) {
                    rsum += out[y * (stride * 4) + x * 4];
                    gsum += out[y * (stride * 4) + x * 4 + 1];
                    bsum += out[y * (stride * 4) + x * 4 + 2];
                }
            }
            uint32_t max = 255 * resultWidth * resultHeight;
            SetProperty("vendor.sensors.als_correction.r", std::to_string(rsum * 0x7FFFFFFFuLL / max));
            SetProperty("vendor.sensors.als_correction.g", std::to_string(gsum * 0x7FFFFFFFuLL / max));
            SetProperty("vendor.sensors.als_correction.b", std::to_string(bsum * 0x7FFFFFFFuLL / max));
            captureResults.buffer->unlock();
        }, 500);

    ScreenshotClient::captureDisplay(captureArgs, captureListener);
    ALOGV("Capture started at %ld", now.tv_sec);

    lastScreenUpdate = now.tv_sec;
}

int main() {
    android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
    android::ProcessState::self()->startThreadPool();

    struct sigaction action{};
    sigfillset(&action.sa_mask);

    action.sa_flags = SA_RESTART;
    action.sa_handler = [](int signal) {
        if (signal == SIGUSR1) {
            ALOGV("Signal received");
            static std::mutex updateLock;
            if (!updateLock.try_lock()) {
                ALOGV("Signal dropped due to multiple call at the same time");
                return;
            }
            updateScreenBuffer();
            updateLock.unlock();
        }
=======
  private:
    struct screenshot_t {
        uint32_t r, g, b;
        nsecs_t timestamp;
>>>>>>> CHANGE (f1f730 sm8150-common: implement ALS compensation algorithm)
    };

    screenshot_t takeScreenshot() {
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
        outBuffer->unlock();

        return { rsum / max, gsum / max, bsum / max, systemTime(SYSTEM_TIME_BOOTTIME) };
    }
};

class AlsCorrectionListener : public FrameworkListener {
  public:
    AlsCorrectionListener() : FrameworkListener("als_correction") {
        registerCmd(new TakeScreenshotCommand);
    }
};

int main() {
    auto listener = new AlsCorrectionListener();
    listener->startListener();

    while (true) {
        pause();
    }

    return 0;
}
