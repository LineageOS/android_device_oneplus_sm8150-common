#include <dlfcn.h>

typedef void (*qcril_other_init_t)();
typedef void (*oem_qmi_access_func_t)();

extern "C" void qcril_other_init() {
    static auto qcril_other_init_orig = reinterpret_cast<qcril_other_init_t>(
            dlsym(RTLD_NEXT, "qcril_other_init"));
    static auto oem_qmi_access_func_orig = reinterpret_cast<oem_qmi_access_func_t>(
            dlsym(RTLD_NEXT, "_Z19oem_qmi_access_funcv"));

    oem_qmi_access_func_orig();
    qcril_other_init_orig();
}
