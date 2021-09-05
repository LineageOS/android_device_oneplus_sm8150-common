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

#include <binder/ProcessState.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SyncScreenCaptureListener.h>
#include <sysutils/FrameworkCommand.h>
#include <sysutils/FrameworkListener.h>
#include <ui/DisplayState.h>

#include <cstdio>
#include <signal.h>
#include <time.h>
#include <unistd.h>

using android::gui::ScreenCaptureResults;
using android::ui::DisplayState;
using android::ui::PixelFormat;
using android::DisplayCaptureArgs;
using android::GraphicBuffer;
using android::IBinder;
using android::Rect;
using android::ScreenshotClient;
using android::sp;
using android::SurfaceComposerClient;
using android::SyncScreenCaptureListener;

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

class TakeScreenshotCommand : public FrameworkCommand {
  public:
    TakeScreenshotCommand() : FrameworkCommand("take_screenshot") {}
    ~TakeScreenshotCommand() override = default;

    int runCommand(SocketClient* cli, int /*argc*/, char **/*argv*/) {
        auto screenshot = takeScreenshot();
        cli->sendData(&screenshot, sizeof(screenshot_t));
        return 0;
    }

  private:
    struct screenshot_t {
        uint32_t r, g, b;
        nsecs_t timestamp;
    };

    // See frameworks/base/services/core/jni/com_android_server_display_DisplayControl.cpp and
    // frameworks/base/core/java/android/view/SurfaceControl.java
    static sp<IBinder> getInternalDisplayToken() {
        const auto displayIds = SurfaceComposerClient::getPhysicalDisplayIds();
        sp<IBinder> token = SurfaceComposerClient::getPhysicalDisplayToken(displayIds[0]);
        return token;
    }

    screenshot_t takeScreenshot() {
        static sp<GraphicBuffer> outBuffer = new GraphicBuffer(
                screenshot_rect.getWidth(), screenshot_rect.getHeight(),
                android::PIXEL_FORMAT_RGB_888,
                GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_OFTEN);

        DisplayCaptureArgs captureArgs;
        captureArgs.displayToken = getInternalDisplayToken();
        captureArgs.pixelFormat = PixelFormat::RGBA_8888;
        captureArgs.sourceCrop = screenshot_rect;
        captureArgs.width = screenshot_rect.getWidth();
        captureArgs.height = screenshot_rect.getHeight();
        captureArgs.useIdentityTransform = true;
        captureArgs.captureSecureLayers = true;

        sp<SyncScreenCaptureListener> captureListener = new SyncScreenCaptureListener();
        if (ScreenshotClient::captureDisplay(captureArgs, captureListener) == android::NO_ERROR) {
            ScreenCaptureResults captureResults = captureListener->waitForResults();
            ALOGV("Capture results received");
            if (captureResults.fenceResult.ok()) {
                outBuffer = captureResults.buffer;
            }
        }

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

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

        ALOGV("Captured at %ld", now.tv_sec);
        return { rsum / max, gsum / max, bsum / max, now.tv_nsec };
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
