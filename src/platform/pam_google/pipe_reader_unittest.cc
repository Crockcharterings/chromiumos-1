// Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pam_google/pipe_reader.h"

#include <errno.h>

#include "gtest/gtest.h"

typedef testing::Test PipeReaderTest;

TEST_F(PipeReaderTest, SuccessfulReadTest) {
  std::string pipe_name("/tmp/MYFIFO");
  /* Create the FIFO if it does not exist */
  umask(0);
  mknod(pipe_name.c_str(), S_IFIFO|0666, 0);
  const char line[] = "foo";

  pid_t pID = fork();
  if (pID == 0) {
    int pipe = open(pipe_name.c_str(), O_WRONLY);
    EXPECT_NE(pipe, -1) << strerror(errno);
    write(pipe, line, strlen(line));
    close(pipe);
    exit(1);
  } else {
    PipeReader reader(pipe_name);
    // asking for more should still just return the amount that was written.
    EXPECT_EQ(line, reader.Read(5 * strlen(line)));
  }
}

TEST_F(PipeReaderTest, SuccessfulMultiLineReadTest) {
  std::string pipe_name("/tmp/TESTFIFO");
  /* Create the FIFO if it does not exist */
  umask(0);
  mknod(pipe_name.c_str(), S_IFIFO|0666, 0);
  const char foo[] = "foo";
  const char boo[] = "boo";
  std::string line(foo);
  line.append("\n");
  line.append(boo);
  line.append("\n");

  pid_t pID = fork();
  if (pID == 0) {
    int pipe = open(pipe_name.c_str(), O_WRONLY);
    EXPECT_NE(pipe, -1) << strerror(errno);
    write(pipe, line.c_str(), line.length());
    close(pipe);
    exit(1);
  } else {
    PipeReader reader(pipe_name);
    // asking for more should still just return the amount that was written.
    std::string my_foo = reader.Read(5 * line.length());
    EXPECT_EQ(my_foo[my_foo.length() - 1], '\n');
    my_foo.resize(my_foo.length() - 1);
    EXPECT_EQ(my_foo, foo);

    std::string my_boo = reader.Read(5 * line.length());
    EXPECT_EQ(my_boo[my_boo.length() - 1], '\n');
    my_boo.resize(my_boo.length() - 1);
    EXPECT_EQ(my_boo, boo);
  }
}

TEST_F(PipeReaderTest, SuccessfulMultiLineReadNoEndingNewlineTest) {
  std::string pipe_name("/tmp/TESTFIFO");
  /* Create the FIFO if it does not exist */
  umask(0);
  mknod(pipe_name.c_str(), S_IFIFO|0666, 0);
  const char foo[] = "foo";
  const char boo[] = "boo";
  std::string line(foo);
  line.append("\n");
  line.append(boo);

  pid_t pID = fork();
  if (pID == 0) {
    int pipe = open(pipe_name.c_str(), O_WRONLY);
    EXPECT_NE(pipe, -1) << strerror(errno);
    write(pipe, line.c_str(), line.length());
    close(pipe);
    exit(1);
  } else {
    PipeReader reader(pipe_name);
    // asking for more should still just return the amount that was written.
    std::string my_foo = reader.Read(5 * line.length());
    EXPECT_EQ(my_foo[my_foo.length() - 1], '\n');
    my_foo.resize(my_foo.length() - 1);
    EXPECT_EQ(my_foo, foo);

    std::string my_boo = reader.Read(5 * line.length());
    EXPECT_EQ(my_boo, boo);
  }
}