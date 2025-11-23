#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/oneplus/sm8150-common',
    'hardware/oplus',
    'hardware/qcom-caf/sm8150',
    'hardware/qcom-caf/wlan',
    'vendor/qcom/opensource/commonsys-intf/display',
    'vendor/qcom/opensource/commonsys/display',
    'vendor/qcom/opensource/dataservices',
    'vendor/qcom/opensource/display',
]


def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'vendor' else None


lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    (
        'com.qualcomm.qti.dpm.api@1.0',
        'libmmosal',
        'vendor.qti.hardware.wifidisplaysession@1.0',
        'vendor.qti.imsrtpservice@3.0',
    ): lib_fixup_vendor_suffix,
}

blob_fixups: blob_fixups_user_type = {
    'odm/bin/hw/vendor.oplus.hardware.biometrics.fingerprint@2.1-service': blob_fixup()
        .add_needed('libshims_fingerprint.oplus.so'),
    'odm/etc/vintf/manifest/manifest_oplus_fingerprint.xml': blob_fixup()
        .patch_file('blob-patches/manifest_oplus_fingerprint.patch'),
    ('odm/lib64/mediadrm/libwvdrmengine.so', 'odm/lib64/libwvhidl.so'): blob_fixup()
        .add_needed('libcrypto_shim.so'),
    ('odm/lib64/libarcsoft_dualcam_refocus_preview.so', 'vendor/lib64/libarcsoft_super_night_raw.so'): blob_fixup()
        .clear_symbol_version('remote_handle_close')
        .clear_symbol_version('remote_handle_invoke')
        .clear_symbol_version('remote_handle_open')
        .clear_symbol_version('remote_register_buf_attr')
        .clear_symbol_version('remote_register_buf'),
    'product/app/PowerOffAlarm/PowerOffAlarm.apk': blob_fixup()
        .apktool_patch('blob-patches/PowerOffAlarm.patch'),
    'product/etc/sysconfig/com.android.hotwordenrollment.common.util.xml': blob_fixup()
        .regex_replace('/my_product', '/product'),
    'system_ext/framework/oplus-ims-ext.jar': blob_fixup()
        .apktool_patch('blob-patches/oplus-ims-ext.patch'),
    'system_ext/lib64/libwfdnative.so': blob_fixup()
        .add_needed('libinput_shim.so'),
    'vendor/etc/libnfc-nci.conf': blob_fixup()
        .regex_replace('NFC_DEBUG_ENABLED=0x01', 'NFC_DEBUG_ENABLED=0x00'),
    'vendor/etc/libnfc-nxp.conf': blob_fixup()
        .regex_replace('NXP_NFC_DEV_NODE="/dev/pn553"', 'NXP_NFC_DEV_NODE="/dev/nq-nci"')
        .regex_replace('(NXPLOG_.*_LOGLEVEL)=0x03', '\\1=0x02')
        .regex_replace('NFC_DEBUG_ENABLED=0x01', 'NFC_DEBUG_ENABLED=0x00'),
    'vendor/etc/wfdconfig.xml': blob_fixup()
        .regex_replace('<AudioStreamInSuspend>0</AudioStreamInSuspend>', '<AudioStreamInSuspend>1</AudioStreamInSuspend>')
        .regex_replace('<HID>0</HID>', '<HID>1</HID>'),
    'vendor/lib64/hw/com.qti.chi.override.so': blob_fixup()
        .add_needed('libcamera_metadata_shim.so'),
}  # fmt: skip

module = ExtractUtilsModule(
    'sm8150-common',
    'oneplus',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
