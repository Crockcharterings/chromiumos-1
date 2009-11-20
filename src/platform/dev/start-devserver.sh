#!/bin/bash

# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Start the Dev Server after making sure we are running under a chroot.

COMMON_SH="$(dirname "$0")/../../scripts/common.sh"
. "$COMMON_SH"

# Script must be run inside the chroot.
assert_inside_chroot

python devserver.py
 
