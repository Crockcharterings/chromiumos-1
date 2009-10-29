#!/bin/bash

# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to convert the output of build_image.sh to a VMware image.

# Load common constants.  This should be the first executable line.
# The path to common.sh should be relative to your script's location.
. "$(dirname "$0")/common.sh"

IMAGES_DIR="${DEFAULT_BUILD_ROOT}/images"
# Default to the most recent image
DEFAULT_FROM="${IMAGES_DIR}/`ls -t $IMAGES_DIR | head -1`"
DEFAULT_TO="${DEFAULT_FROM}/ide.vmdk"

# Flags
DEFINE_string from "$DEFAULT_FROM" \
  "Directory containing rootfs.image and mbr.image"
DEFINE_string to "$DEFAULT_TO" \
  "Destination file for VMware image"

# Parse command line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# Die on any errors.
set -e

# Convert args to paths.  Need eval to un-quote the string so that shell 
# chars like ~ are processed; just doing FOO=`readlink -f $FOO` won't work.
FLAGS_from=`eval readlink -f $FLAGS_from`
FLAGS_to=`eval readlink -f $FLAGS_to`

# Make two sparse files. One for an empty partition, another for
# stateful partition.
PART_SIZE=$(stat -c%s "${FLAGS_from}/rootfs.image")
dd if=/dev/zero of="${FLAGS_from}/empty.image" bs=1 count=1 \
    seek=$(( $PART_SIZE - 1 ))
dd if=/dev/zero of="${FLAGS_from}/state.image" bs=1 count=1 \
    seek=$(( $PART_SIZE - 1 ))
mkfs.ext3 -F -L C-STATE "${FLAGS_from}/state.image"

# Copy MBR and rootfs to output image
qemu-img convert -f raw \
  "${FLAGS_from}/mbr.image" "${FLAGS_from}/state.image" \
  "${FLAGS_from}/empty.image" "${FLAGS_from}/rootfs.image" \
  -O vmdk "${FLAGS_to}"

rm -f "${FLAGS_from}/empty.image" "${FLAGS_from}/state.image"

echo "Done. Created VMware image ${FLAGS_to}"

