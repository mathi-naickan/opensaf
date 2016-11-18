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
#include "mock_clock_nanosleep.h"
#include "gtest/gtest.h"

TEST(OsafNanosleep, SleepTenMilliseconds) {
  time_t start_sec = 1234;
  long start_nsec = 5678;
  monotonic_clock = { start_sec, start_nsec };
  mock_clock_gettime.execution_time = kZeroSeconds;
  mock_clock_gettime.return_value = 0;
  current_nanosleep_index = 0;
  number_of_nanosleep_instances = 2;
  mock_clock_nanosleep[0].return_value = 0;

  timespec sleep_duration = kTenMilliseconds;
  osaf_nanosleep(&sleep_duration);

  EXPECT_EQ(monotonic_clock.tv_sec, start_sec);
  EXPECT_EQ(monotonic_clock.tv_nsec, start_nsec + 10000000);
  EXPECT_EQ(current_nanosleep_index, 1);
}

TEST(OsafNanosleep, SleepOneHourWithInterrupt) {
  monotonic_clock = kZeroSeconds;
  mock_clock_gettime.execution_time = kZeroSeconds;
  mock_clock_gettime.return_value = 0;
  current_nanosleep_index = 0;
  number_of_nanosleep_instances = 3;
  mock_clock_nanosleep[0].return_value = EINTR;
  mock_clock_nanosleep[1].return_value = 0;

  timespec sleep_duration = kOneHour;
  osaf_nanosleep(&sleep_duration);

  EXPECT_EQ(monotonic_clock.tv_sec, 60 * 60);
  EXPECT_EQ(monotonic_clock.tv_nsec, 0);
  EXPECT_EQ(current_nanosleep_index, 2);
}

TEST(OsafNanosleepDeathTest, FailWithEFAULT) {
  mock_clock_gettime.return_value = 0;
  current_nanosleep_index = 0;
  number_of_nanosleep_instances = 1;
  mock_clock_nanosleep[0].return_value = EFAULT;

  timespec sleep_duration = kOneSecond;
  ASSERT_EXIT(osaf_nanosleep(&sleep_duration), ::testing::KilledBySignal(SIGABRT), "");
}

TEST(OsafNanosleepDeathTest, FailWithEINVAL) {
  mock_clock_gettime.return_value = 0;
  current_nanosleep_index = 0;
  number_of_nanosleep_instances = 1;
  mock_clock_nanosleep[0].return_value = EINVAL;

  timespec sleep_duration = kOneSecond;
  ASSERT_EXIT(osaf_nanosleep(&sleep_duration), ::testing::KilledBySignal(SIGABRT), "");
}
