// Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pam_google/pipe_reader.h"

#include "base/scoped_ptr.h"

std::string PipeReader::Read(const uint32 bytes_to_read) {
  scoped_array<char> buffer(new char[bytes_to_read]);
  if (pipe_ || (pipe_ = fopen(pipe_name_.c_str(), "r"))) {
    const char* to_return = fgets(buffer.get(), bytes_to_read, pipe_);
    if (to_return)
      return to_return;  // auto-coerced to a std::string.
  }
  return std::string();
}