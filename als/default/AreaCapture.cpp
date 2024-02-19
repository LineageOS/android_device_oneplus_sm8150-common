/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "AreaCapture.h"

#include <android-base/properties.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SyncScreenCaptureListener.h>
#include <ui/DisplayState.h>
#include <ui/PixelFormat.h>

#include <cstdio>
#include <signal.h>
#include <time.h>
#include <unistd.h>

using android::base::GetProperty;
using android::gui::ScreenCaptureResults;
using android::ui::PixelFormat;
using android::DisplayCaptureArgs;
using android::GraphicBuffer;
using android::IBinder;
using android::Rect;
using android::ScreenshotClient;
using android::sp;
using android::SurfaceComposerClient;
using android::SyncScreenCaptureListener;
using aidl::vendor::lineage::oplus_als::AreaCapture;

static Rect screenshot_rect;

AreaCapture::AreaCapture() {
    int32_t left, top, right, bottom;
    std::istringstream is(GetProperty("vendor.sensors.als_correction.grabrect", ""));

    is >> left >> top >> right >> bottom;
    if (left == 0) {
        ALOGE("No screenshot grab area config");
        return;
    }

    ALOGI("Screenshot grab area: %d %d %d %d", left, top, right, bottom);
    screenshot_rect = Rect(left, top, right, bottom);
}

// See frameworks/base/services/core/jni/com_android_server_display_DisplayControl.cpp and
// frameworks/base/core/java/android/view/SurfaceControl.java
sp<IBinder> AreaCapture::getInternalDisplayToken() {
    const auto displayIds = SurfaceComposerClient::getPhysicalDisplayIds();
    sp<IBinder> token = SurfaceComposerClient::getPhysicalDisplayToken(displayIds[0]);
    return token;
}

ndk::ScopedAStatus AreaCapture::getAreaBrightness(AreaRgbCaptureResult* _aidl_return) {
    static sp<GraphicBuffer> outBuffer = new GraphicBuffer(screenshot_rect.getWidth(),
        screenshot_rect.getHeight(), ::android::PIXEL_FORMAT_RGB_888,
        GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_OFTEN);

    sp<IBinder> display = getInternalDisplayToken();

    DisplayCaptureArgs captureArgs;
    captureArgs.displayToken = getInternalDisplayToken();
    captureArgs.pixelFormat = PixelFormat::RGBA_8888;
    captureArgs.sourceCrop = screenshot_rect;
    captureArgs.width = screenshot_rect.getWidth();
    captureArgs.height = screenshot_rect.getHeight();
    captureArgs.useIdentityTransform = true;
    captureArgs.captureSecureLayers = true;

    sp<SyncScreenCaptureListener> captureListener = new SyncScreenCaptureListener();
    if (ScreenshotClient::captureDisplay(captureArgs, captureListener) != ::android::NO_ERROR) {
        ALOGE("Capture failed");
        return ndk::ScopedAStatus::fromServiceSpecificError(-1);
    }
    ScreenCaptureResults captureResults = captureListener->waitForResults();
    if (!captureResults.fenceResult.ok()) {
        ALOGE("Fence result error");
        return ndk::ScopedAStatus::fromServiceSpecificError(-1);
    }

    outBuffer = captureResults.buffer;

    uint8_t* out;
    auto resultWidth = outBuffer->getWidth();
    auto resultHeight = outBuffer->getHeight();
    auto stride = outBuffer->getStride();

    captureResults.buffer->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, reinterpret_cast<void**>(&out));
    // we can sum this directly on linear light
    uint32_t rsum = 0, gsum = 0, bsum = 0;
    for (int y = 0; y < resultHeight; y++) {
        for (int x = 0; x < resultWidth; x++) {
            rsum += out[y * (stride * 4) + x * 4];
            gsum += out[y * (stride * 4) + x * 4 + 1];
            bsum += out[y * (stride * 4) + x * 4 + 2];
        }
    }
    float max = resultWidth * resultHeight;
    _aidl_return->r = rsum / max;
    _aidl_return->g = gsum / max;
    _aidl_return->b = bsum / max;
    
    captureResults.buffer->unlock();

    return ndk::ScopedAStatus::ok();
}
