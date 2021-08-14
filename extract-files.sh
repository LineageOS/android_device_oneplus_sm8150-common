#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2020 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

set -e

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

ONLY_COMMON=
ONLY_TARGET=
KANG=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        --only-common )
                ONLY_COMMON=true
                ;;
        --only-target )
                ONLY_TARGET=true
                ;;
        -n | --no-cleanup )
                CLEAN_VENDOR=false
                ;;
        -k | --kang )
                KANG="--kang"
                ;;
        -s | --section )
                SECTION="${2}"; shift
                CLEAN_VENDOR=false
                ;;
        * )
                SRC="${1}"
                ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC="adb"
fi

function blob_fixup() {
    case "${1}" in
        system_ext/lib64/libwfdnative.so)
            sed -i "s/android.hidl.base@1.0.so/libhidlbase.so\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00/" "${2}"
            ;;
        vendor/bin/hw/qcrild)
            "${PATCHELF}" --add-needed libril_shim.so "${2}"
            ;;
        vendor/etc/qdcm_calib_data_*.xml)
            python - "${2}" <<EOF
import sys
import copy
import xml.etree.cElementTree as ET
tree = ET.parse(sys.argv[1])
root = tree.getroot()
modes = root.findall('Disp_Modes')[0]
for mode_node in modes.findall('Mode'):
    mode_id = mode_node.get('ModeID')
    if mode_id == '1':
        mode_node.set('Name', 'hal_srgb')
        mode_node.set('ColorGamut', 'srgb')
        mode_node.set('DynamicRange', 'sdr')
    elif mode_id == '5':
        mode_node.set('Name', 'hal_display_p3')
        mode_node.set('ColorGamut', 'dcip3')
        mode_node.set('DynamicRange', 'sdr')
        dup_node = copy.deepcopy(mode_node)
        dup_node.set('Name', 'hal_saturated')
        dup_node.set('ModeID', '11')
        dup_node.set('ColorGamut', 'native')
        modes.append(dup_node)
    elif mode_id == '6':
        mode_node.set('Name', 'hal_hdr')
        mode_node.set('ColorGamut', 'dcip3')
        mode_node.set('DynamicRange', 'hdr')
    else:
        modes.remove(mode_node)
tree.write(sys.argv[1], encoding='utf-8', xml_declaration=True)
EOF
            ;;
    esac
}

if [ -z "${ONLY_TARGET}" ]; then
    # Initialize the helper for common device
    setup_vendor "${DEVICE_COMMON}" "${VENDOR}" "${ANDROID_ROOT}" true "${CLEAN_VENDOR}"

    extract "${MY_DIR}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

if [ -z "${ONLY_COMMON}" ] && [ -s "${MY_DIR}/../${DEVICE}/proprietary-files.txt" ]; then
    # Reinitialize the helper for device
    source "${MY_DIR}/../${DEVICE}/extract-files.sh"
    setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

    extract "${MY_DIR}/../${DEVICE}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

"${MY_DIR}/setup-makefiles.sh"
