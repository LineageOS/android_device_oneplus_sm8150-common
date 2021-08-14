#pragma once

#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <string_view>

class PanelModeController {
    std::string active_mode;
    std::mutex active_mode_lock;
    std::thread worker_thread;
    int event_trigger = -1;
    int delay_trigger = -1;

    bool real_set_panel_mode(std::string_view panel_mode);
    void loader_worker();

    public:
    PanelModeController();
    ~PanelModeController();
    bool set_panel_mode(std::string_view mode);
    void set_delayed_reload(std::chrono::nanoseconds delay);
};
