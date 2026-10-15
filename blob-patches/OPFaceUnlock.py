#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

import os
import shutil
import zipfile

from extract_utils.fixups_blob import (
    BlobFixupCtx,
    File,
)


def blob_fixup(
    ctx: BlobFixupCtx,
    file: File,
    file_path: str,
    *args,
    tmp_dir: str,
    **kwargs,
):
    page = 4096
    tmp_path = os.path.join(tmp_dir, "file.zip")

    with zipfile.ZipFile(file_path) as zin, zipfile.ZipFile(tmp_path, 'w') as zout:
        for info in zin.infolist():
            data = zin.read(info.filename)

            out = zipfile.ZipInfo(info.filename, date_time=info.date_time)
            out.external_attr = info.external_attr
            out.internal_attr = info.internal_attr
            out.create_system = info.create_system

            if info.filename.endswith('.so'):
                out.compress_type = zipfile.ZIP_STORED
                base = zout.fp.tell() + 30 + len(out.filename.encode('utf-8'))
                pad = (-(base + 4)) % page
                out.extra = b'\x9d\x99' + pad.to_bytes(2, 'little') + b'\x00' * pad
            else:
                out.compress_type = zipfile.ZIP_DEFLATED

            zout.writestr(out, data)

    shutil.copy2(tmp_path, file_path)
