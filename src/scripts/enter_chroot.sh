#!/bin/bash

# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to enter the chroot environment

# Load common constants.  This should be the first executable line.
# The path to common.sh should be relative to your script's location.
. "$(dirname "$0")/common.sh"

# Script must be run outside the chroot
assert_outside_chroot

# Define command line flags
# See http://code.google.com/p/shflags/wiki/Documentation10x
DEFINE_string chroot "$DEFAULT_CHROOT_DIR" \
  "The destination dir for the chroot environment." "d"
DEFINE_string trunk "$GCLIENT_ROOT" \
  "The source trunk to bind mount within the chroot." "s"

DEFINE_boolean mount $FLAGS_FALSE "Only set up mounts."
DEFINE_boolean unmount $FLAGS_FALSE "Only tear down mounts."
DEFINE_boolean revision $FLAGS_FALSE "Pass subversion revision into chroot."

# More useful help
FLAGS_HELP="USAGE: $0 [flags] [VAR=value] [-- \"command\"]

One or more VAR=value pairs can be specified to export variables into
the chroot environment.  For example:

   $0 FOO=bar BAZ=bel

If [-- \"command\"] is present, runs the command inside the chroot,
after changing directory to /$USER/trunk/src/scripts.  Note that the
command should be enclosed in quotes to prevent interpretation by the
shell before getting into the chroot.  For example:

    $0 -- \"./build_platform_packages.sh\"

Otherwise, provides an interactive shell.
"

# Parse command line flags
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# Only now can we die on error.  shflags functions leak non-zero error codes,
# so will die prematurely if 'set -e' is specified before now.
# TODO: replace shflags with something less error-prone, or contribute a fix.
set -e

function setup_env {
  echo "Mounting chroot environment."

  # Mount only if not already mounted
  MOUNTED_PATH="$(readlink -f "$FLAGS_chroot/proc")"
  if [ -z "$(mount | grep -F "on $MOUNTED_PATH")" ]
  then
    sudo mount none -t proc "$MOUNTED_PATH"
  fi

  MOUNTED_PATH="$(readlink -f "$FLAGS_chroot/dev/pts")"
  if [ -z "$(mount | grep -F "on $MOUNTED_PATH")" ]
  then
    sudo mount none -t devpts "$MOUNTED_PATH"
  fi

  MOUNTED_PATH="$(readlink -f "${FLAGS_chroot}$CHROOT_TRUNK_DIR")"
  if [ -z "$(mount | grep -F "on $MOUNTED_PATH")" ]
  then
    sudo mount --bind "$FLAGS_trunk" "$MOUNTED_PATH"
  fi
}

function teardown_env {
  echo "Unmounting chroot environment."
  mount | grep "on $(readlink -f "$FLAGS_chroot")" | awk '{print $3}' \
    | xargs -r -L1 sudo umount
}

if [ $FLAGS_mount -eq $FLAGS_TRUE ]
then
  setup_env
  echo "Make sure you run"
  echo "    $0 --unmount"
  echo "before deleting $FLAGS_chroot"
  echo "or you'll end up deleting $FLAGS_trunk too!"
  exit 0
fi

if [ $FLAGS_unmount -eq $FLAGS_TRUE ]
then
  teardown_env
  exit 0
fi

# Make sure we unmount before exiting
trap teardown_env EXIT
setup_env

if [ $FLAGS_revision -eq $FLAGS_TRUE ]
then
  # Get the subversion revision to pass into the chroot.  
  #
  # This must be determined outside the chroot because (1) there is no
  # svn/git inside the chroot, and (2) if there were it would likely be
  # the wrong version, which would mess up the .svn/.git directories.
  #
  # Note that this fixes $CHROMEOS_REVISION at the time the chroot is
  # entered.  That's ok for the main use case of automated builds,
  # which pass each command line into a separate call to enter_chroot
  # so always have up-to-date info.  For developer builds, there isn't
  # really a single revision anyway, since the developer may have
  # hand-sync'd some subdirs and edited files in others.
  REVISION="$(svn info 2>&- | grep "Revision: " | awk '{print $2}')"
  if [ -z "$REVISION" ]
  then
     # Use git:8 chars of sha1
     REVISION=$(git rev-parse HEAD)
     REVISION="${REVISION:8}"
  fi
  REVISION="CHROMEOS_REVISION=$REVISION"
fi

# Run command or interactive shell
sudo chroot "$FLAGS_chroot" sudo -i -u $USER $REVISION "$@"

# Remove trap and explicitly unmount
trap - EXIT
teardown_env

