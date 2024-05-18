#include "Power.h"

ndk::ScopedAStatus Power::setMode(Mode type, bool enabled) {
    (void) type;
    (void) enabled;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::isModeSupported(Mode type, bool *_aidl_return) {
    (void) type;
    (void) _aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::setBoost(Boost type, int32_t durationMs) {
    (void) type;
    (void) durationMs;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::isBoostSupported(Boost type, bool *_aidl_return) {
    (void) type;
    (void) _aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::createHintSession(int32_t tgid, int32_t uid,
                                            const std::vector<int32_t> &threadIds,
                                            int64_t durationNanos,
                                            std::shared_ptr<IPowerHintSession> *_aidl_return) {
    (void) tgid;
    (void) uid;
    (void) threadIds;
    (void) durationNanos;
    (void) _aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::getHintSessionPreferredRate(int64_t *outNanoseconds) {
    (void) outNanoseconds;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

__attribute__((constructor))
static void init() {
    Power::setDefaultImpl(ndk::SharedRefBase::make<Power>());
}
