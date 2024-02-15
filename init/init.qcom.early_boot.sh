#! /vendor/bin/sh

export PATH=/vendor/bin

#For drm based display driver
echo -1 >  /sys/module/drm/parameters/vblankoffdelay

boot_reason=`cat /proc/sys/kernel/boot_reason`
reboot_reason=`getprop ro.boot.alarmboot`
if [ "$boot_reason" = "3" ] || [ "$reboot_reason" = "true" ]; then
    setprop ro.vendor.alarm_boot true
else
    setprop ro.vendor.alarm_boot false
fi
