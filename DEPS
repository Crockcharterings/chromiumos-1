# -*- python -*-

# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# NOTES
#
# This file must exist both in trunk and trunk/src, since some users sync
# trunk and some sync src.  (In the end, src will win, since src-internal will
# be in a different repo.)
#
# Source packages should go into
#    _third_party_base + "PACKAGENAME/files"
# so that we can put our build wrapper and/or patches for each package into
#    _third_party_base + "PACKAGENAME"
# (DEPS must point to empty subdirectories)
#
# No trailing backslash when specifying SVN paths.  That confuses gclient and
# causes it to re-download the files each time.

# Base is prefixed with "trunk/src/" if this is trunk/DEPS, or "src/" if this
# is trunk/src/DEPS.
_third_party_base = "trunk/src/third_party/"

deps = {
    # gflags 1.1
    _third_party_base + "gflags/files":
        "http://google-gflags.googlecode.com/svn/trunk@31",

    # glog
    # Need r49 to include fix for compiling with gflags support
    _third_party_base + "glog/files":
        "http://google-glog.googlecode.com/svn/trunk@49",

    # google-breakpad
    _third_party_base + "google-breakpad/files":
        "http://google-breakpad.googlecode.com/svn/trunk@400",

    # gtest 1.3.0
    _third_party_base + "gtest/files":
        "http://googletest.googlecode.com/svn/trunk@209",

    # shflags 1.0.3
    _third_party_base + "shflags/files":
        "http://shflags.googlecode.com/svn/tags/1.0.3@137",

    # shunit2 2.1.5
    _third_party_base + "shunit2/files":
        "http://shunit2.googlecode.com/svn/tags/source/2.1.5@294",
}