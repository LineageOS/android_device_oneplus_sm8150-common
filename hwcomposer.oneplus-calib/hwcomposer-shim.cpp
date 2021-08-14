#define LOG_TAG "hwc-oneplus-calib"

#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <cerrno>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <string_view>
#include <fstream>
#include <filesystem>
#include <sys/mman.h>
#include <log/log.h>
#include <android-base/properties.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer2.h>
#include <dlfcn.h>

#include <dobby.h>

#include "member_func_ptr.hpp"
#include "hwcdefs.h"

using namespace std::string_view_literals;

//#ifdef __ANDROID_RECOVERY__
#define vendor_dlopen dlopen
#define vendor_dlclose dlclose
//#else
//#include <vndksupport/linker.h>
// we are already in shpal namespace
//#define vendor_dlopen android_load_sphal_library
//#define vendor_dlclose android_unload_sphal_library
//#endif
#if defined(__LP64__)
#define HAL_LIBRARY_PATH1 "/system/lib64/hw"
#define HAL_LIBRARY_PATH2 "/vendor/lib64/hw"
#define HAL_LIBRARY_PATH3 "/odm/lib64/hw"
#else
#define HAL_LIBRARY_PATH1 "/system/lib/hw"
#define HAL_LIBRARY_PATH2 "/vendor/lib/hw"
#define HAL_LIBRARY_PATH3 "/odm/lib/hw"
#endif

#define SYSFS_DSI1 "/sys/class/drm/card0-DSI-1/"

#define SYMBOL_SetPreferredColorModeInternal "_ZN3sdm12HWCColorMode29SetPreferredColorModeInternalERKNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEbPN7android8hardware8graphics6common4V1_19ColorModeEPNS_16DynamicRangeTypeE"
#define SYMBOL_HWCDisplayBuiltinInit "_ZN3sdm17HWCDisplayBuiltIn4InitEv"
#define SYMBOL_HWCDisplayBuiltinGetColorModes "_ZN3sdm17HWCDisplayBuiltIn13GetColorModesEPjPN7android8hardware8graphics6common4V1_19ColorModeE"

int hook_hwc2_open(const hw_module_t *module, const char *name, hw_device_t **dev);

static void *realSetPreferredColorModeInternal;

static hw_module_methods_t HookHwc2Methods {hook_hwc2_open};

extern "C" hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = 0x0003,
    .hal_api_version = 0,
    .id = HWC_HARDWARE_MODULE_ID,
    .name = "OnePlus SM8150 hardware panel calibration loader hwcomposer shim",
    .author = "LineageOS",
    .methods = &HookHwc2Methods,
    .dso = nullptr,
    .reserved = {0},
};

static hw_module_t *real_hmi;

static decltype(hook_hwc2_open) *real_hwc2_open;

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
        ALOGW("verify content of file %s failed, retrying", path.c_str());
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
    int /* __thiscall */ hookSetPreferredColorModeInternal(const std::string &mode_string, bool from_client, ColorMode *color_mode, DynamicRangeType *dynamic_range) {
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
};

static bool test_execmem() {
    void *result = mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (result == MAP_FAILED) return false;
    munmap(result, 4096);
    return true;
}

static bool setup_hook(void *module, const char *symbol, void *replace_call, void **origin_call) {
    auto sym = dlsym(module, symbol);
    if (sym) {
        auto err = DobbyHook(sym, replace_call, origin_call);
        if (err) {
            ALOGE("DobbyHook %s failed", symbol);
            return false;
        } else {
            ALOGI("installed %s hook", symbol);
            return true;
        }
    } else {
        ALOGE("symbol %s not found", symbol);
        return false;
    }
}

static void load_real_hwc() {
    if (real_hmi) return;
    auto handle = vendor_dlopen(HAL_LIBRARY_PATH2 "/hwcomposer.msmnile.so", RTLD_NOW);
    if (handle == NULL) {
        char const *err_str = dlerror();
        ALOGE("load module failed: %s", err_str?err_str:"unknown");
        abort();
    }
    auto hmi = (hw_module_t *)dlsym(handle, HAL_MODULE_INFO_SYM_AS_STR);
    if (hmi == NULL) {
        ALOGE("load: couldn't find symbol %s", HAL_MODULE_INFO_SYM_AS_STR);
        abort();
    }
    hmi->dso = handle;
    real_hmi = hmi;
    real_hwc2_open = hmi->methods->open;

    if (!test_execmem()) {
        ALOGE("cannot allocate executable memory, skipping hook");
        return;
    }
    setup_hook(handle, SYMBOL_SetPreferredColorModeInternal, member_func_ptr_cast<void*>(&ThisCallHookClass::hookSetPreferredColorModeInternal), &realSetPreferredColorModeInternal);

}

int hook_hwc2_open(const hw_module_t *module, const char *name, hw_device_t **dev) {
    load_real_hwc();
    return real_hwc2_open(module, name, dev);
}
