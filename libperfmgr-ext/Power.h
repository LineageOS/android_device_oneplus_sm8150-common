#include <aidl/android/hardware/power/BnPower.h>

using aidl::android::hardware::power::BnPower;
using aidl::android::hardware::power::Boost;
using aidl::android::hardware::power::IPowerHintSession;
using aidl::android::hardware::power::Mode;

class Power : public BnPower {
  public:
    Power() = default;
    ndk::ScopedAStatus setMode(Mode type, bool enabled) override;
    ndk::ScopedAStatus isModeSupported(Mode type, bool *_aidl_return) override;
    ndk::ScopedAStatus setBoost(Boost type, int32_t durationMs) override;
    ndk::ScopedAStatus isBoostSupported(Boost type, bool *_aidl_return) override;
    ndk::ScopedAStatus createHintSession(int32_t tgid, int32_t uid,
                                         const std::vector<int32_t> &threadIds,
                                         int64_t durationNanos,
                                         std::shared_ptr<IPowerHintSession> *_aidl_return) override;
    ndk::ScopedAStatus getHintSessionPreferredRate(int64_t *outNanoseconds) override;
};
