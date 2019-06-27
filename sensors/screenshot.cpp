#define LOG_TAG "sensors-screenshot-oneplus-msmnile"
#define LOG_NDEBUG 0
#include <log/log.h>
#include "screenshot.h"
#include <gui/SurfaceComposerClient.h>
#include <time.h>
#include <cutils/properties.h>

using namespace android;

sp<GraphicBuffer> screen_buffer = NULL;
time_t last_screen_update = 0;
int sensor_loc_x = -1, sensor_loc_y = -1;

void update_screen_buffer(void **out) {
    if (sensor_loc_x == -1 || sensor_loc_y == -1) {
        char prop[255];
        property_get("persist.vendor.sensors.light.location_x", prop, "0");
        sensor_loc_x = atoi(prop);
        property_get("persist.vendor.sensors.light.location_y", prop, "0");
        sensor_loc_y = atoi(prop);
    }
    if (screen_buffer == NULL) {
        screen_buffer = new GraphicBuffer(10, 10, PIXEL_FORMAT_RGB_888, GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_OFTEN);
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (now.tv_sec - last_screen_update >= SCREENSHOT_INTERVAL) {
        // Update Screenshot at most every second
        ScreenshotClient::capture(SurfaceComposerClient::getBuiltInDisplay(0),
                                    Rect(sensor_loc_x, sensor_loc_y, sensor_loc_x + 10, sensor_loc_y + 10),
                                    10, 10, 0, 65535, true, 0, &screen_buffer);
        last_screen_update = now.tv_sec;
    }
    screen_buffer->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, out);
}

void free_screen_buffer() {
    screen_buffer->unlock();
}
