/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_SENSORS_V2_0_SENSOR_H
#define ANDROID_HARDWARE_SENSORS_V2_0_SENSOR_H

#include <android/hardware/sensors/1.0/types.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using ::android::hardware::sensors::V1_0::Event;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SensorInfo;
using ::android::hardware::sensors::V1_0::SensorType;

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

class ISensorsEventCallback {
   public:
    virtual ~ISensorsEventCallback(){};
    virtual void postEvents(const std::vector<Event>& events, bool wakeup) = 0;
};

class Sensor {
   public:
    Sensor(ISensorsEventCallback* callback);
    virtual ~Sensor();

    const SensorInfo& getSensorInfo() const;
    void batch(int32_t samplingPeriodNs);
    virtual void activate(bool enable);
    Result flush();

    void setOperationMode(OperationMode mode);
    bool supportsDataInjection() const;
    Result injectEvent(const Event& event);

   protected:
    void run();
    virtual std::vector<Event> readEvents();
    static void startThread(Sensor* sensor);

    bool isWakeUpSensor();

    bool mIsEnabled;
    int64_t mSamplingPeriodNs;
    int64_t mLastSampleTimeNs;
    SensorInfo mSensorInfo;

    std::atomic_bool mStopThread;
    std::condition_variable mWaitCV;
    std::mutex mRunMutex;
    std::thread mRunThread;

    ISensorsEventCallback* mCallback;

    OperationMode mMode;
};

class OnChangeSensor : public Sensor {
   public:
    OnChangeSensor(ISensorsEventCallback* callback);

    virtual void activate(bool enable) override;

   protected:
    virtual std::vector<Event> readEvents() override;

   protected:
    Event mPreviousEvent;
    bool mPreviousEventSet;
};

class AccelSensor : public Sensor {
   public:
    AccelSensor(int32_t sensorHandle, ISensorsEventCallback* callback);
};

class GyroSensor : public Sensor {
   public:
    GyroSensor(int32_t sensorHandle, ISensorsEventCallback* callback);
};

class AmbientTempSensor : public OnChangeSensor {
   public:
    AmbientTempSensor(int32_t sensorHandle, ISensorsEventCallback* callback);
};

class DeviceTempSensor : public OnChangeSensor {
   public:
    DeviceTempSensor(int32_t sensorHandle, ISensorsEventCallback* callback);
};

class PressureSensor : public Sensor {
   public:
    PressureSensor(int32_t sensorHandle, ISensorsEventCallback* callback);
};

class MagnetometerSensor : public Sensor {
   public:
    MagnetometerSensor(int32_t sensorHandle, ISensorsEventCallback* callback);
};

class LightSensor : public OnChangeSensor {
   public:
    LightSensor(int32_t sensorHandle, ISensorsEventCallback* callback);
};

class ProximitySensor : public OnChangeSensor {
   public:
    ProximitySensor(int32_t sensorHandle, ISensorsEventCallback* callback);
};

class RelativeHumiditySensor : public OnChangeSensor {
   public:
    RelativeHumiditySensor(int32_t sensorHandle, ISensorsEventCallback* callback);
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SENSORS_V2_0_SENSOR_H
