#!/system/bin/sh

if [ -f /sys/kernel/debug/kmemleak ]; then
    echo clear > /sys/kernel/debug/kmemleak
    echo scan > /sys/kernel/debug/kmemleak

    while true
    do
        cat /sys/kernel/debug/kmemleak > /sdcard/oem_log/kmemleak.txt
        echo `date` >> /sdcard/oem_log/ps.txt
        ps >> /sdcard/oem_log/ps.txt
        echo `date` >> /sdcard/oem_log/ion_system.txt
        cat /sys/kernel/debug/ion/heaps/system >> /sdcard/oem_log/ion_system.txt
        echo `date` >> /sdcard/oem_log/ion_qsecom.txt
        cat /sys/kernel/debug/ion/heaps/qsecom >> /sdcard/oem_log/ion_qsecom.txt
        sleep 3600
    done
fi

