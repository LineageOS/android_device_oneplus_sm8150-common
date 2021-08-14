#define LOG_TAG "hwc-oneplus-calib"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <fstream>
#include <filesystem>
#include <log/log.h>
#include <android-base/properties.h>
#include <hardware/hwcomposer2.h>
#include <dlfcn.h>

#include "member_func_ptr.hpp"
#include "hwcdefs.h"

using namespace std::string_view_literals;

#define SYSFS_DSI1 "/sys/class/drm/card0-DSI-1/"

#define SYMBOL_SetPreferredColorModeInternal "_ZN3sdm12HWCColorMode29SetPreferredColorModeInternalERKNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEbPN7android8hardware8graphics6common4V1_19ColorModeEPNS_16DynamicRangeTypeE"

static void *realSetPreferredColorModeInternal;

static bool verify_file(const std::filesystem::path &path, std::string_view content) {
    std::ifstream fs(path);
    std::string line;
    std::getline(fs, line);
    // ALOGI("after setting %s content %s", path.c_str(), line.c_str());
    return std::string_view(line).find_last_of(content) != std::string_view::npos;
}

static bool write_file(const std::filesystem::path &path, std::string_view content) {
    std::ofstream fs(path);
    fs << content;
    if (fs.fail()) {
        ALOGE("write to %s failed", path.c_str());
        return false;
    }
    return true;
}

static int write_and_verify_file(const std::filesystem::path &path, std::string_view content) {
    if (!write_file(path, content)) return false;
    if (!verify_file(path, content)) {
        ALOGW("verify content of file %s failed", path.c_str());
        return 0;
    }
    return 1;
}

static bool set_panel_mode(std::string_view panel_mode) {
    int result = 1;
    ALOGI("load_calibration for sdm color mode %s", std::string(panel_mode).c_str());
    if (panel_mode == "native_srgb"sv) {
        ALOGI("setting panel to srgb mode");
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "1"sv);
    } else if (panel_mode == "native_p3"sv) {
        ALOGI("setting panel to display p3 mode");
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "dither_en", "1"sv);
    } else if (panel_mode == "hal_hdr"sv) {
        ALOGI("setting panel to display p3 hdr mode");
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "dither_en", "1"sv);
    } else if (panel_mode == "advance_p3"sv ) {
        ALOGI("setting panel to advanced display p3 mode");
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "dither_en", "1"sv);
    } else if (panel_mode == "advance_srgb"sv) {
        ALOGI("setting panel to advanced srgb mode");
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "dither_en", "0"sv);
    } else if (panel_mode == "panel_native"sv) {
        ALOGI("setting panel to AMOLED native mode");
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "dither_en", "0"sv);
    } else {
        ALOGE("unrecognized panel mode %s", std::string(panel_mode).c_str());
        result = 0;
    }
    return result;
}

static bool load_calibration(std::string_view sdm_mode) {
    if (sdm_mode == "hal_srgb"sv) {
        auto panel_mode = android::base::GetProperty("persist.vendor.display.hal_srgb_mode", "native_srgb");
        return set_panel_mode(panel_mode);
    } else if (sdm_mode == "hal_display_p3"sv) {
        auto panel_mode = android::base::GetProperty("persist.vendor.display.hal_display_p3_mode", "native_p3");
        return set_panel_mode(panel_mode);
    } else if (sdm_mode == "hal_saturated"sv) {
        auto panel_mode = android::base::GetProperty("persist.vendor.display.hal_saturated_mode", "native_p3");
        return set_panel_mode(panel_mode);
    } else if (sdm_mode == "hal_hdr"sv) {
        return set_panel_mode("hal_hdr"sv);
    } else {
        ALOGE("unrecognized sdm mode %s", std::string(sdm_mode).c_str());
        return false;
    }
}

struct ThisCallHookClass {
    int32_t hookSetPreferredColorModeInternal(const std::string &mode_string, bool from_client, ColorMode *color_mode, DynamicRangeType *dynamic_range) __asm__(SYMBOL_SetPreferredColorModeInternal);
};

int32_t ThisCallHookClass::hookSetPreferredColorModeInternal(const std::string &mode_string, bool from_client, ColorMode *color_mode, DynamicRangeType *dynamic_range) {
    ColorMode result_color_mode;
    DynamicRangeType result_dynamic_range;
    auto origin_func = member_func_ptr_cast<decltype(&ThisCallHookClass::hookSetPreferredColorModeInternal)>(realSetPreferredColorModeInternal);
    // auto err = (this->*(caster.func))(mode_string, from_client, &result_color_mode, &result_dynamic_range);
    auto err = std::invoke(origin_func, this, mode_string, from_client, &result_color_mode, &result_dynamic_range);
    if (err) return err;
    if (color_mode) *color_mode = result_color_mode;
    if (dynamic_range) *dynamic_range = result_dynamic_range;
    ALOGI("set sdm color mode %s, color mode %d, dynamic range %d, from client %d, loading calibration...", mode_string.c_str(), result_color_mode, result_dynamic_range, from_client);
    auto result = load_calibration(mode_string);
    return result ? err : HWC2_ERROR_NO_RESOURCES;
}

__attribute__((constructor)) void __hwc_shim_init()
{
    realSetPreferredColorModeInternal = dlsym(RTLD_NEXT, SYMBOL_SetPreferredColorModeInternal);
    if (!realSetPreferredColorModeInternal) {
        LOG_ALWAYS_FATAL("cannot get original %s", SYMBOL_SetPreferredColorModeInternal);
    }
}

// a symbol reference is needed to fool the linker to add a DT_NEEDED to hwcomposer.msmnile.so
// the symbol needs to exist in both real and fake hwcomposer.msmnile.so
void fool_linker() {
	void fool_linker_external() __asm__("_ZN3sdm8HWCLayerD1Ev");
	fool_linker_external();
}
