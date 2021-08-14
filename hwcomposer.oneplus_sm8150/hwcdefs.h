#pragma once

#include <android/hardware/graphics/common/1.1/types.h>
using android::hardware::graphics::common::V1_1::ColorMode;
using android::hardware::graphics::common::V1_1::Dataspace;
using android::hardware::graphics::common::V1_1::RenderIntent;

enum DynamicRangeType {
  kSdrType,
  kHdrType,
};
