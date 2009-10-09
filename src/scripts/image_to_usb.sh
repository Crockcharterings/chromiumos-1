#!/bin/bash

# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to convert the output of build_image.sh to a usb image.

# Load common constants.  This should be the first executable line.
# The path to common.sh should be relative to your script's location.
. "$(dirname "$0")/common.sh"

IMAGES_DIR="${DEFAULT_BUILD_ROOT}/images"
# Default to the most recent image
DEFAULT_FROM="${IMAGES_DIR}/`ls -t $IMAGES_DIR | head -1`"

# Script can be run either inside or outside the chroot.
if [ $INSIDE_CHROOT -eq 1 ]
then
  # Inside the chroot, so output to usb.img in the same dir as the other 
  # images.
  DEFAULT_TO="${DEFAULT_FROM}/usb.img"
  DEFAULT_TO_HELP="Destination file for USB image."
else
  # Outside the chroot, so output to the default device for a usb key.
  DEFAULT_TO="/dev/sdb"
  DEFAULT_TO_HELP="Destination device for USB keyfob."
fi

# Flags
DEFINE_string from "$DEFAULT_FROM" \
  "Directory containing rootfs.image and mbr.image"
DEFINE_string to "$DEFAULT_TO" "$DEFAULT_TO_HELP"
DEFINE_boolean yes $FLAGS_FALSE "Answer yes to all prompts" "y"

# Parse command line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# Die on any errors.
set -e

# Convert args to paths.  Need eval to un-quote the string so that shell 
# chars like ~ are processed; just doing FOO=`readlink -f $FOO` won't work.
FLAGS_from=`eval readlink -f $FLAGS_from`
FLAGS_to=`eval readlink -f $FLAGS_to`

# Copy MBR and rootfs to output image
if [ -b "$FLAGS_to" ]
then
  # Output to a block device (i.e., a real USB key), so need sudo dd
  echo "Copying USB image ${FLAGS_from} to device ${FLAGS_to}..."

  # Make sure this is really what the user wants, before nuking the device
  if [ $FLAGS_yes -ne $FLAGS_TRUE ]
  then
    echo "This will erase all data on this device:"
    sudo fdisk -l "$FLAGS_to" | grep Disk | head -1
    read -p "Are you sure (y/N)? " SURE
    SURE="${SURE:0:1}" # Get just the first character
    if [ "$SURE" != "y" ]
    then
      echo "Ok, better safe than sorry."
      exit 1
    fi
  fi

  sudo bash -c "cat \"${FLAGS_from}/mbr.image\" \
    \"${FLAGS_from}/rootfs.image\" \
    | dd of=\"$FLAGS_to\" bs=4M"
  sync
  echo "Done."
else
  # Output to a file, so just cat the source images together
  echo "Copying USB image to file ${FLAGS_to}..."
  cat "${FLAGS_from}/mbr.image" "${FLAGS_from}/rootfs.image" > "$FLAGS_to"

  echo "Done.  To copy to USB keyfob, outside the chroot, do something like:"
  echo "   sudo dd if=${FLAGS_to} of=/dev/sdb bs=4M"
  echo "where /dev/sdb is the entire keyfob."
  if [ $INSIDE_CHROOT -eq 1 ]
  then
    echo "NOTE: Since you are currently inside the chroot, and you'll need to"
    echo "run dd outside the chroot, the path to the USB image will be"
    echo "different (ex: ~/chromeos/trunk/src/build/images/SOME_DIR/usb.img)."
  fi
fi