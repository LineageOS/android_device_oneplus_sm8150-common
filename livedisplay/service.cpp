/*
 * Copyright (C) 2019 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vendor.lineage.livedisplay@2.1-service.oneplus_msmnile"

#include <android-base/logging.h>
#include <binder/ProcessState.h>
#include <hidl/HidlTransportSupport.h>
#include <livedisplay/sdm/PictureAdjustment.h>
#include <vendor/lineage/livedisplay/2.1/IPictureAdjustment.h>

#include "DisplayModes.h"

using android::OK;
using android::sp;
using android::status_t;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

using ::vendor::lineage::livedisplay::V2_0::sdm::PictureAdjustment;
using ::vendor::lineage::livedisplay::V2_0::sdm::SDMController;
using ::vendor::lineage::livedisplay::V2_1::IDisplayModes;
using ::vendor::lineage::livedisplay::V2_1::IPictureAdjustment;
using ::vendor::lineage::livedisplay::V2_1::implementation::DisplayModes;

int main() {
    status_t status = OK;

    android::ProcessState::initWithDriver("/dev/vndbinder");

    LOG(INFO) << "LiveDisplay HAL service is starting.";

    std::shared_ptr<SDMController> controller = std::make_shared<SDMController>();
    sp<DisplayModes> dm = new DisplayModes(controller);
    sp<PictureAdjustment> pa = new PictureAdjustment(controller);

    configureRpcThreadpool(1, true /*callerWillJoin*/);

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
