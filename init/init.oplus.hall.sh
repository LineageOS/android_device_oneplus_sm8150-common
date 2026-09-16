#! /vendor/bin/sh
#
# Copyright (C) 2026 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

if [ -w /sys/class/motor/hall_calibration ]; then
    HALL_CALIBRATION=$(cat /mnt/vendor/persist/engineermode/hall_calibration)

    IFS=,
    set -- $HALL_CALIBRATION

    case $# in
        12) ;;
        11) HALL_CALIBRATION="$HALL_CALIBRATION,2" ;;
        *)  HALL_CALIBRATION="1170,170,480,0,0,480,500,0,0,500,1500,2" ;;
    esac

    echo "$HALL_CALIBRATION" > /sys/class/motor/hall_calibration
fi
