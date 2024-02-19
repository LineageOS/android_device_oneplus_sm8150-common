/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/vendor/lineage/oplus_als/BnAreaCapture.h>

namespace aidl {
namespace vendor {
namespace lineage {
namespace oplus_als {

class AreaCapture : public BnAreaCapture {
  public:
    AreaCapture();
    ndk::ScopedAStatus getAreaBrightness(AreaRgbCaptureResult* _aidl_return) override;

  private:
    static ::android::sp<::android::IBinder> getInternalDisplayToken();
};

}  // namespace oplus_als
}  // namespace lineage
}  // namespace vendor
}  // namespace aidl
