#define LOG_TAG "hwc-oneplus-sm8150"
#ifndef HWC2_USE_CPP11
#define HWC2_USE_CPP11
#define HWC2_INCLUDE_STRINGIFICATION
#endif

#include <cinttypes>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <dlfcn.h>
#include <android/hardware/graphics/common/1.1/types.h>
#include <android-base/properties.h>
#include <hardware/hwcomposer2.h>
#include <log/log.h>

#include <core/display_interface.h>
#include <private/color_params.h>

#include "PanelModeController.h"
#include "member_func_ptr.hpp"

using android::hardware::graphics::common::V1_1::ColorMode;
using android::hardware::graphics::common::V1_1::Dataspace;
using android::hardware::graphics::common::V1_1::RenderIntent;

using namespace std::string_view_literals;
using namespace sdm;

#define SYMBOL_DisplayBuiltinInit            "_ZN3sdm17HWCDisplayBuiltIn4InitEv"
#define SYMBOL_DisplayBuiltinDeinit          "_ZN3sdm17HWCDisplayBuiltIn6DeinitEv"
#define SYMBOL_DisplayBuiltinSetPowerMode    "_ZN3sdm17HWCDisplayBuiltIn12SetPowerModeEN4HWC29PowerModeEb"
#define SYMBOL_DisplayBaseSetColorMode       "_ZN3sdm11DisplayBase12SetColorModeERKNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE"
#define SYMBOL_DisplayBaseSetColorModeById   "_ZN3sdm11DisplayBase16SetColorModeByIdEi"
#define SYMBOL_ColorModePopulateColorModes   "_ZN3sdm12HWCColorMode18PopulateColorModesEv"

static void *realDisplayBuiltinInit;
static void *realDisplayBuiltinDeinit;
static void *realDisplayBuiltinSetPowerMode;
static void *realDisplayBaseSetColorMode;
static void *realDisplayBaseSetColorModeById;
static void *realColorModePopulateColorModes;

static void *builtin_display_id;
static void *last_initialized_color_mode;
static void *builtin_display_color_mode;

std::unique_ptr<PanelModeController> panel_mode_controller;

static bool set_panel_mode_for_color_mode(std::string_view sdm_mode) {
    ALOGV("setting panel mode for sdm color mode %s", std::string(sdm_mode).c_str());
    if (sdm_mode == "hal_srgb"sv) {
        auto panel_mode = android::base::GetProperty("persist.vendor.display.hal_srgb_mode", "native_srgb");
        return panel_mode_controller->set_panel_mode(panel_mode);
    } else if (sdm_mode == "hal_display_p3"sv) {
        auto panel_mode = android::base::GetProperty("persist.vendor.display.hal_display_p3_mode", "native_p3");
        return panel_mode_controller->set_panel_mode(panel_mode);
    } else if (sdm_mode == "hal_saturated"sv) {
        auto panel_mode = android::base::GetProperty("persist.vendor.display.hal_saturated_mode", "native_p3");
        return panel_mode_controller->set_panel_mode(panel_mode);
    } else if (sdm_mode == "hal_hdr"sv) {
        return panel_mode_controller->set_panel_mode("hal_hdr"sv);
    } else {
        ALOGE("unrecognized sdm mode %s", std::string(sdm_mode).c_str());
        return false;
    }
};

struct HookHWCDisplayBuiltin {
    int Init() __asm__(SYMBOL_DisplayBuiltinInit);
    int Deinit() __asm__(SYMBOL_DisplayBuiltinDeinit);
    HWC2::Error SetPowerMode(HWC2::PowerMode mode, bool teardown) __asm__(SYMBOL_DisplayBuiltinSetPowerMode);

    void *vtbl;
    bool validated;
    bool layer_stack_invalid_;
    /*CoreInterface*/ void *core_intf_;
    /*HWCBufferAllocator*/ void *buffer_allocator_;
    /*HWCCallbacks*/ void *callbacks_;
    /*HWCDisplayEventHandler*/ void *event_handler_;
    /*DisplayType*/ int type_;
    hwc2_display_t hal_id_;
    int32_t sdm_id_;
    bool needs_blit_;
    /*DisplayInterface*/ void *display_intf_;
};

struct HookHWCColorMode {
    HWC2::Error PopulateColorModes() __asm__(SYMBOL_ColorModePopulateColorModes);
    DisplayInterface *display_intf_;
    bool apply_mode_;
    ColorMode current_color_mode_;
    RenderIntent current_render_intent_;
    DynamicRangeType curr_dynamic_range_;
    typedef std::map<DynamicRangeType, std::string> DynamicRangeMap;
    typedef std::map<RenderIntent, DynamicRangeMap> RenderIntentMap;
    std::map<ColorMode, RenderIntentMap> color_mode_map_;
    double color_matrix_[16];
    std::map<ColorMode, DynamicRangeMap> preferred_mode_;
};

struct HookDisplayBase /* : public DisplayInterface */ {
    DisplayError SetColorMode(const std::string &color_mode) __asm__(SYMBOL_DisplayBaseSetColorMode);
    DisplayError SetColorModeById(int32_t color_mode_id) __asm__(SYMBOL_DisplayBaseSetColorModeById);
};

void update_mode_preference(HookHWCColorMode *color_mode_) {
    // WIP: looking for a victim function to make changed value applied immediately.
    // once implemented, we can switch back to stock qdcm_calib_data and make use of extra color modes.
    auto srgb_mode = android::base::GetProperty("persist.vendor.display.hal_srgb_mode", "native_srgb");
    auto display_p3_mode = android::base::GetProperty("persist.vendor.display.hal_display_p3_mode", "native_p3");
    auto native_mode = android::base::GetProperty("persist.vendor.display.hal_native_mode", "native_p3");
    color_mode_->color_mode_map_[ColorMode::SRGB][RenderIntent::COLORIMETRIC][kSdrType] = srgb_mode;
    color_mode_->color_mode_map_[ColorMode::SRGB][RenderIntent::ENHANCE][kSdrType] = srgb_mode;
    color_mode_->preferred_mode_[ColorMode::SRGB][kSdrType] = srgb_mode;
    color_mode_->color_mode_map_[ColorMode::DISPLAY_P3][RenderIntent::COLORIMETRIC][kSdrType] = display_p3_mode;
    color_mode_->color_mode_map_[ColorMode::DISPLAY_P3][RenderIntent::ENHANCE][kSdrType] = display_p3_mode;
    color_mode_->preferred_mode_[ColorMode::DISPLAY_P3][kSdrType] = display_p3_mode;
    color_mode_->color_mode_map_[ColorMode::NATIVE][RenderIntent::COLORIMETRIC][kSdrType] = native_mode;
    color_mode_->preferred_mode_[ColorMode::NATIVE][kSdrType] = native_mode;
}

DisplayError HookDisplayBase::SetColorMode(const std::string &color_mode) {
    auto origin_func = member_func_ptr_cast<decltype(&HookDisplayBase::SetColorMode)>(realDisplayBaseSetColorMode);
    auto err = std::invoke(origin_func, this, color_mode);
    ALOGV("set sdm color mode %s for display %p", color_mode.c_str(), this);
    if (reinterpret_cast<void*>(this) != builtin_display_id || err) return err;
    auto result = set_panel_mode_for_color_mode(color_mode);
    return result ? kErrorNone : kErrorParameters;
}

DisplayError HookDisplayBase::SetColorModeById(int32_t color_mode_id) {
    auto origin_func = member_func_ptr_cast<decltype(&HookDisplayBase::SetColorModeById)>(realDisplayBaseSetColorModeById);
    auto err = std::invoke(origin_func, this, color_mode_id);
    if (reinterpret_cast<void*>(this) != builtin_display_id || err) return err;
    auto display_intf = reinterpret_cast<DisplayInterface*>(this);
    std::string name;
    err = display_intf->GetColorModeName(color_mode_id, &name);
    if (err != kErrorNone) return err;
    ALOGV("set sdm color mode %d - %s for display %p", color_mode_id, name.c_str(), this);
    auto result = set_panel_mode_for_color_mode(name);
    return result ? kErrorNone : kErrorParameters;
}

int HookHWCDisplayBuiltin::Init() {
    auto origin_func = member_func_ptr_cast<decltype(&HookHWCDisplayBuiltin::Init)>(realDisplayBuiltinInit);
    auto err = std::invoke(origin_func, this);
    if (!err && !builtin_display_id) {
        builtin_display_id = display_intf_;
        builtin_display_color_mode = last_initialized_color_mode;
        panel_mode_controller = std::make_unique<PanelModeController>();
        ALOGI("HWCDisplayBuiltin::Init on display_intf %p color_mode %p", display_intf_, builtin_display_color_mode);
    }
    return err;
}

int HookHWCDisplayBuiltin::Deinit() {
    auto origin_func = member_func_ptr_cast<decltype(&HookHWCDisplayBuiltin::Deinit)>(realDisplayBuiltinDeinit);
    auto err = std::invoke(origin_func, this);
    if (display_intf_ == builtin_display_id) {
        panel_mode_controller.reset();
    }
    return err;
}

HWC2::Error HookHWCDisplayBuiltin::SetPowerMode(HWC2::PowerMode mode, bool teardown) {
    static bool first_call = true;
    auto origin_func = member_func_ptr_cast<decltype(&HookHWCDisplayBuiltin::SetPowerMode)>(realDisplayBuiltinSetPowerMode);
    auto err = std::invoke(origin_func, this, mode, teardown);
    if (err == HWC2::Error::None && !first_call && display_intf_ == builtin_display_id && mode == HWC2::PowerMode::On) {
        ALOGV("reloading panel mode in 100ms");
        panel_mode_controller->set_delayed_reload(std::chrono::milliseconds(100));
    }
    if (first_call) {
        first_call = false;
    }
    return err;
}

HWC2::Error HookHWCColorMode::PopulateColorModes() {
    ALOGV("PopulateColorModes on HWCColor %p for display %p", this, display_intf_);
    last_initialized_color_mode = reinterpret_cast<void*>(this);
    auto origin_func = member_func_ptr_cast<decltype(&HookHWCColorMode::PopulateColorModes)>(realColorModePopulateColorModes);
    return std::invoke(origin_func, this);
}

__attribute__((constructor)) void __hwc_shim_init()
{
    #define FILL_SHIM(name) do { \
        real ## name = dlsym(RTLD_NEXT, SYMBOL_ ## name); \
        if (!real ## name) LOG_ALWAYS_FATAL("cannot get original %s", #name); \
    } while(0)

    FILL_SHIM(DisplayBaseSetColorMode);
    FILL_SHIM(DisplayBaseSetColorModeById);
    FILL_SHIM(DisplayBuiltinInit);
    FILL_SHIM(DisplayBuiltinDeinit);
    FILL_SHIM(DisplayBuiltinSetPowerMode);
    FILL_SHIM(ColorModePopulateColorModes);

    #undef FILL_SHIM
}

// a symbol reference is needed to fool the linker to add a DT_NEEDED to hwcomposer.msmnile.so
// the symbol needs to exist in both real and fake hwcomposer.msmnile.so
void fool_linker() {
    void fool_linker_external() __asm__("_ZN3sdm8HWCLayerD1Ev");
    fool_linker_external();
}
