package msmnile

import (
    "android/soong/android"
)

func init() {
    android.RegisterModuleType("oneplus_msmnile_sensors_hal_binary", sensorsHalBinaryFactory)
}
