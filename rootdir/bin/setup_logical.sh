#!/sbin/sh

for dm in `ls -d /sys/devices/virtual/block/dm-*`; do
    partition=$(cat $dm/dm/name)
    dm_block=/dev/block/$(echo $dm | sed 's|.*/||')
    ln -s $dm_block /dev/block/bootdevice/by-name/$partition
done
