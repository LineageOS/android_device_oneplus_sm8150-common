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

#define LOG_TAG "vendor.lineage.livedisplay@2.1-service.oneplus_msmnile"

#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <android-base/logging.h>
#include <binder/ProcessState.h>
#include <hidl/HidlTransportSupport.h>
#include <livedisplay/sdm/PictureAdjustment.h>
#include <vendor/lineage/livedisplay/2.1/IPictureAdjustment.h>

#include "AntiFlicker.h"
#include "DisplayModes.h"
#include "SunlightEnhancement.h"

using android::OK;
using android::sp;
using android::status_t;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

using ::vendor::lineage::livedisplay::V2_0::sdm::PictureAdjustment;
using ::vendor::lineage::livedisplay::V2_0::sdm::SDMController;
using ::vendor::lineage::livedisplay::V2_1::IAntiFlicker;
using ::vendor::lineage::livedisplay::V2_1::IDisplayModes;
using ::vendor::lineage::livedisplay::V2_1::IPictureAdjustment;
using ::vendor::lineage::livedisplay::V2_1::ISunlightEnhancement;
using ::vendor::lineage::livedisplay::V2_1::implementation::AntiFlicker;
using ::vendor::lineage::livedisplay::V2_1::implementation::DisplayModes;
using ::vendor::lineage::livedisplay::V2_1::implementation::SunlightEnhancement;

static void wait_sysfs_controls_ready() {
    // sysfs controls doesn't work (blocked by mutex) before kernel set native_display_loading_effect_mode to 1
    for (int counter = 0; counter < 100; counter++) {
        LOG(INFO) << "Waiting for sysfs controls become ready.";
        std::ifstream fs("/sys/class/drm/card0-DSI-1/native_display_loading_effect_mode");
        std::string line;
        std::getline(fs, line);
        if (fs.fail()) {
            continue;
        }
        LOG(DEBUG) << line;
        if (line.find_last_of("1") != std::string::npos) {
            return;
        }
        if (counter > 100) {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    LOG(ERROR) << "sysfs controls didn't become ready after 100 attempts, giving up.";
}

int main() {
    status_t status = OK;

    wait_sysfs_controls_ready();

    android::ProcessState::initWithDriver("/dev/vndbinder");

    LOG(INFO) << "LiveDisplay HAL service is starting.";

    std::shared_ptr<SDMController> controller = std::make_shared<SDMController>();
    sp<AntiFlicker> af = new AntiFlicker();
    sp<DisplayModes> dm = new DisplayModes(controller);
    sp<PictureAdjustment> pa = new PictureAdjustment(controller);
    sp<SunlightEnhancement> se = new SunlightEnhancement();

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    status = af->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Could not register service for LiveDisplay HAL AntiFlicker Iface ("
                   << status << ")";
        goto shutdown;
    }

    status = dm->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Could not register service for LiveDisplay HAL DisplayModes Iface ("
                   << status << ")";
        goto shutdown;
    }

    status = pa->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Could not register service for LiveDisplay HAL PictureAdjustment Iface ("
                   << status << ")";
        goto shutdown;
    }

    status = se->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Could not register service for LiveDisplay HAL SunlightEnhancement Iface ("
                   << status << ")";
        goto shutdown;
    }

    // Update default PA on setDisplayMode
    dm->registerDisplayModeSetCallback(
            std::bind(&PictureAdjustment::updateDefaultPictureAdjustment, pa));

    LOG(INFO) << "LiveDisplay HAL service is ready.";
    joinRpcThreadpool();
    // Should not pass this line

shutdown:
    // In normal operation, we don't expect the thread pool to shutdown
    LOG(ERROR) << "LiveDisplay HAL service is shutting down.";
    return 1;
}
