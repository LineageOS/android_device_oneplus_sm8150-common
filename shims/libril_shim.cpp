/*
 * Copyright (C) 2021 The LineageOS Project
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
