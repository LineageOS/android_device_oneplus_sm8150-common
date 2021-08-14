#define LOG_TAG "PanelModeController"

#include "PanelModeController.h"

#include <fstream>
#include <filesystem>

#include <poll.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#include <log/log.h>

#define SYSFS_DSI1 "/sys/class/drm/card0-DSI-1/"

using namespace std::string_view_literals;

static bool verify_file(const std::filesystem::path &path, std::string_view content) {
    std::ifstream fs(path);
    std::string line;
    std::getline(fs, line);
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

bool PanelModeController::real_set_panel_mode(std::string_view panel_mode) {
    int result = 1;
    ALOGI("setting panel mode %s", std::string(panel_mode).c_str());
    if (panel_mode == "native_srgb"sv) {
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "1"sv);
    } else if (panel_mode == "native_p3"sv) {
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "dither_en", "1"sv);
    } else if (panel_mode == "hal_hdr"sv) {
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "dither_en", "1"sv);
    } else if (panel_mode == "advance_p3"sv) {
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "dither_en", "1"sv);
    } else if (panel_mode == "advance_srgb"sv) {
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_wide_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_srgb_color_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_p3_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_customer_srgb_mode", "1"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "native_display_loading_effect_mode", "0"sv);
        result &= write_and_verify_file(SYSFS_DSI1 "dither_en", "0"sv);
    } else if (panel_mode == "panel_native"sv) {
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

void PanelModeController::loader_worker() {
    pollfd fds[] = {
        {event_trigger, POLLIN, 0},
        {delay_trigger, POLLIN, 0}
    };
    constexpr size_t nfds = 2;
    while (true) {
        auto count = poll(fds, nfds, -1);
        if (count == -1) {
            ALOGE("poll: %s", strerror(errno));
            break;
        }
        bool reload = false;
        for (size_t i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                uint64_t buf;
                read(fds[i].fd, &buf, sizeof(uint64_t));
                fds[i].revents &= ~POLLIN;
                reload = true;
            }
        }
        if (reload) {
            std::lock_guard<std::mutex> guard{active_mode_lock};
            real_set_panel_mode(active_mode);
            fds[1].revents = 0;
        }
        if (fds[0].revents & (POLLHUP | POLLERR) || fds[1].revents & (POLLHUP | POLLERR)) {
            ALOGV("closing worker");
            break;
        }
    }
}

PanelModeController::PanelModeController() {
    event_trigger = eventfd(0, EFD_CLOEXEC);
    if (event_trigger == -1) {
        ALOGE("eventfd: %s", strerror(errno));
    }
    delay_trigger = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (delay_trigger == -1) {
        ALOGE("timerfd_create: %s", strerror(errno));
    }
    worker_thread = std::thread([this]{ loader_worker(); });
}

PanelModeController::~PanelModeController() {
    if (event_trigger != -1) {
        close(event_trigger);
        event_trigger = -1;
    }
    if (delay_trigger != -1) {
        close(delay_trigger);
        delay_trigger = -1;
    }
    worker_thread.join();
}

bool PanelModeController::set_panel_mode(std::string_view mode) {
    std::lock_guard<std::mutex> lock{active_mode_lock};
    active_mode = mode;
    constexpr uint64_t one = 1;
    if (event_trigger == -1) return false;
    write(event_trigger, &one, sizeof(uint64_t));
    return true;
}

void PanelModeController::set_delayed_reload(std::chrono::nanoseconds delay) {
    if (delay_trigger != -1) {
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(delay);
        delay -= secs;
        itimerspec timeout{ {0, 0}, {static_cast<time_t>(secs.count()), static_cast<long>(delay.count())} };
        timerfd_settime(delay_trigger, 0, &timeout, nullptr);
    }
}
