#!/system/bin/sh
    echo "ftm_power_config start" > /dev/kmsg
    /vendor/bin/sh /vendor/bin/init.qcom.post_boot.sh
    sleep 5
    echo 0 > /sys/devices/system/cpu/cpu4/online
    echo 0 > /sys/devices/system/cpu/cpu5/online
    echo 0 > /sys/devices/system/cpu/cpu6/online
    echo 0 > /sys/devices/system/cpu/cpu7/online

    echo 0 > /sys/module/lpm_levels/parameters/sleep_disabled
    echo "ftm_power_config done" > /dev/kmsg
