/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2015 The OpenSAF Foundation
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

#include <cerrno>
#include "base/osaf_time.h"
#include "base/osaf_utility.h"
#include "mock_clock_gettime.h"
#include "gtest/gtest.h"

TEST(OsafClockGettime, ReadRealtimeClock) {
  time_t sec = 123456789;
  long nsec = 987654321;
  realtime_clock = { sec, nsec };
  monotonic_clock = { 0, 0 };
  mock_clock_gettime.return_value = 0;

  struct timespec ts = { 0, 0 };
  osaf_clock_gettime(CLOCK_REALTIME, &ts);

  EXPECT_EQ(ts.tv_sec, sec);
  EXPECT_EQ(ts.tv_nsec, nsec);
}

TEST(OsafClockGettime, ReadMonotonicClock) {
  time_t sec = 212121212;
  long nsec = 565656565;
  realtime_clock = { 0, 0 };
  monotonic_clock = { sec, nsec };
  mock_clock_gettime.return_value = 0;

  struct timespec ts = { 0, 0 };
  osaf_clock_gettime(CLOCK_MONOTONIC, &ts);

  EXPECT_EQ(ts.tv_sec, sec);
  EXPECT_EQ(ts.tv_nsec, nsec);
}

TEST(OsafClockGettimeDeathTest, FailWithEFAULT) {
  mock_clock_gettime.return_value = -1;
  mock_clock_gettime.errno_value = EFAULT;

  ASSERT_EXIT(osaf_clock_gettime(CLOCK_REALTIME, NULL), ::testing::KilledBySignal(SIGABRT), "");
}

TEST(OsafClockGettimeDeathTest, FailWithEINVAL) {
  mock_clock_gettime.return_value = -1;
  mock_clock_gettime.errno_value = EINVAL;

  struct timespec ts;
  ASSERT_EXIT(osaf_clock_gettime(CLOCK_REALTIME, &ts), ::testing::KilledBySignal(SIGABRT), "");
}

TEST(OsafClockGettimeDeathTest, FailWithEPERM) {
  mock_clock_gettime.return_value = -1;
  mock_clock_gettime.errno_value = EPERM;

  struct timespec ts;
  ASSERT_EXIT(osaf_clock_gettime(CLOCK_REALTIME, &ts), ::testing::KilledBySignal(SIGABRT), "");
}
