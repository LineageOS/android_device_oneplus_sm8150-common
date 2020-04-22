package msmnile

import (
    "android/soong/android"
    "android/soong/cc"
    "strings"
)

func bootctrlFlags(ctx android.BaseContext) []string {
    var cflags []string

    var deviceName = ctx.Config().DeviceName()

    if strings.HasPrefix(deviceName, "hotdog") {
        cflags = append(cflags, "-DUSE_DYNAMIC_PARTITIONS")
    }
    return cflags
}

func bootctrlLibrary(ctx android.LoadHookContext) {
    type props struct {
        Target struct {
            Android struct {
                Cflags []string
            }
        }
    }

    p := &props{}
    p.Target.Android.Cflags = bootctrlFlags(ctx)
    ctx.AppendProperties(p)
}

func bootctrlLibraryFactory() android.Module {
    module, _ := cc.NewLibrary(android.DeviceSupported)
    newMod := module.Init()
    android.AddLoadHook(newMod, bootctrlLibrary)
    return newMod
}
