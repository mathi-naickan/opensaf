/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Ericsson AB
 *
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <string>
#include "gtest/gtest.h"
#include "dtm/transport/tests/mock_osaf_poll.h"
#include "dtm/transport/transport_monitor.h"

class TransportMonitorTest : public ::testing::Test {
 protected:
  TransportMonitorTest() :
      tmpdir_{},
      monitor_{nullptr} {
  }

  virtual ~TransportMonitorTest() {
  }

  virtual void SetUp() {
    char tmpdir[] = "/tmp/transport_monitor_test_XXXXXX";
    char* result = mkdtemp(tmpdir);
    ASSERT_NE(result, nullptr);
    tmpdir_ = result;
    int retval = setenv("pkgpiddir", tmpdir_.c_str(), 1);
    ASSERT_EQ(retval, 0);
    retval = setenv("pkglogdir", tmpdir_.c_str(), 1);
    ASSERT_EQ(retval, 0);
    mock_osaf_poll.reset();
    monitor_ = new TransportMonitor(0);
  }

  virtual void TearDown() {
    delete monitor_;
    monitor_ = nullptr;
    std::string cmd = std::string("rm -f ") + tmpdir_ + std::string("/*");
    int result = system(cmd.c_str());
    ASSERT_EQ(result, 0);
    result = rmdir(tmpdir_.c_str());
    ASSERT_EQ(result, 0);
    result = unsetenv("pkglogdir");
    ASSERT_EQ(result, 0);
    result = unsetenv("pkgpiddir");
    ASSERT_EQ(result, 0);
  }

  void CreatePidFile(const char* daemon_name, pid_t pid, bool newline) {
    std::ofstream str(tmpdir_ + std::string("/") + std::string(daemon_name)
                      + std::string(".pid"));
    ASSERT_TRUE(str.good());
    str << pid;
    ASSERT_TRUE(str.good());
    if (newline) str << "\n";
    ASSERT_TRUE(str.good());
    str.close();
    ASSERT_TRUE(str.good());
  }

  std::string tmpdir_;
  TransportMonitor* monitor_;
};

TEST_F(TransportMonitorTest, WaitForNonexistentDaemonName) {
  pid_t pid = monitor_->WaitForDaemon("name_does_not_exist", 17);
  EXPECT_EQ(pid, pid_t{-1});
  EXPECT_EQ(mock_osaf_poll.invocations, 0);
}

TEST_F(TransportMonitorTest, WaitForNonexistentDaemonPid) {
  CreatePidFile("pid_does_not_exist", 1234567890, false);
  pid_t pid = monitor_->WaitForDaemon("pid_does_not_exist", 1);
  EXPECT_EQ(pid, pid_t{-1});
  EXPECT_EQ(mock_osaf_poll.invocations, 0);
}

TEST_F(TransportMonitorTest, WaitForExistingPid) {
  pid_t mypid = getpid();
  CreatePidFile("existing", mypid, false);
  pid_t pid = monitor_->WaitForDaemon("existing", 1);
  EXPECT_EQ(pid, mypid);
  EXPECT_EQ(mock_osaf_poll.invocations, 0);
}

TEST_F(TransportMonitorTest, WaitForExistingPidWithNewline) {
  pid_t mypid = getpid();
  CreatePidFile("existing", mypid, true);
  pid_t pid = monitor_->WaitForDaemon("existing", 1);
  EXPECT_EQ(pid, mypid);
  EXPECT_EQ(mock_osaf_poll.invocations, 0);
}
