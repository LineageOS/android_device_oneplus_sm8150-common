package msmnile

import (
    "android/soong/android"
)

func init() {
    android.RegisterModuleType("oneplus_msmnile_fod_hal_binary", fodHalBinaryFactory)
    android.RegisterModuleType("oneplus_msmnile_sensors_hal_library_static", sensorsHalLibraryFactory)
}
