package msmnile

import (
    "android/soong/android"
    "android/soong/cc"
    "strings"
)

func sensorsFlags(ctx android.BaseContext) []string {
    var cflags []string

    var config = ctx.AConfig().VendorConfig("ONEPLUS_MSMNILE_SENSORS")
    var posX = strings.TrimSpace(config.String("ALS_POS_X"))
    var posY = strings.TrimSpace(config.String("ALS_POS_Y"))

    cflags = append(cflags, "-DALS_POS_X=" + posX, "-DALS_POS_Y=" + posY)
    return cflags
}

func sensorsHalBinary(ctx android.LoadHookContext) {
    type props struct {
        Target struct {
            Android struct {
                Cflags []string
            }
        }
    }

    p := &props{}
    p.Target.Android.Cflags = sensorsFlags(ctx)
    ctx.AppendProperties(p)
}

func sensorsHalBinaryFactory() android.Module {
    module, _ := cc.NewBinary(android.HostAndDeviceSupported)
    newMod := module.Init()
    android.AddLoadHook(newMod, sensorsHalBinary)
    return newMod
}
