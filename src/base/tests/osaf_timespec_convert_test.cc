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

#include "base/osaf_time.h"
#include "gtest/gtest.h"

static const timespec kTimespec = { 1576315623, 358743382 };
static const timeval kTimeval = { 1576315623, 358743 };
static const timespec kTruncatedTimespec = { 1576315623, 358743000 };

TEST(OsafTimespecConvert, TimespecToTimeval) {
  timeval result = { 0, 0 };
  osaf_timespec_to_timeval(&kTimespec, &result);
  EXPECT_EQ(result.tv_sec, kTimeval.tv_sec);
  EXPECT_EQ(result.tv_usec, kTimeval.tv_usec);
}

TEST(OsafTimespecConvert, TimevalToTimespec) {
  timespec result = { 0,  0 };
  osaf_timeval_to_timespec(&kTimeval, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &kTruncatedTimespec), 0);
}

TEST(OsafTimespecConvert, MillisToTimespec) {
  const timespec expected_result = { 1428499100, 961000000 };
  timespec result = { 0,  0 };
  osaf_millis_to_timespec(1428499100961ull, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &expected_result), 0);
}

TEST(OsafTimespecConvert, MicrosToTimespec) {
  const timespec expected_result = { 2125428499, 100961000 };
  timespec result = { 0,  0 };
  osaf_micros_to_timespec(2125428499100961ull, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &expected_result), 0);
}

TEST(OsafTimespecConvert, NanosToTimespec) {
  const struct timespec expected_result = { 1725125428, 499100961 };
  timespec result = { 0,  0 };

  osaf_nanos_to_timespec(1725125428499100961ull, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &expected_result), 0);
}

TEST(OsafTimespecConvert, DoubleToTimespec) {
  const struct timespec expected_result = { 15714713, 125433700 };
  const struct timespec expected_result2 = { -15714714, 874566300 };
  timespec result = { 0,  0 };

  osaf_double_to_timespec(15714713.1254337, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &expected_result), 0);
  osaf_double_to_timespec(-15714713.1254337, &result);
  EXPECT_EQ(osaf_timespec_compare(&result, &expected_result2), 0);
}

TEST(OsafTimespecConvert, TimespecToMillis) {
  const struct timespec ts = { 1428499100, 961923266 };

  uint64_t result = osaf_timespec_to_millis(&ts);
  EXPECT_EQ(result, 1428499100961ull);
}

TEST(OsafTimespecConvert, TimespecToMicros) {
  const struct timespec ts = { 1125428499, 100961923 };

  uint64_t result = osaf_timespec_to_micros(&ts);
  EXPECT_EQ(result, 1125428499100961ull);
}

TEST(OsafTimespecConvert, TimespecToNanos) {
  const struct timespec ts = { 1725125428, 499100961 };

  uint64_t result = osaf_timespec_to_nanos(&ts);
  EXPECT_EQ(result, 1725125428499100961ull);
}

TEST(OsafTimespecConvert, TimespecToDouble) {
  const struct timespec ts = { 15714713, 125433700 };
  const struct timespec ts2 = { -15714714, 874566300 };

  double result1 = osaf_timespec_to_double(&ts);
  double result2 = osaf_timespec_to_double(&ts2);
  EXPECT_EQ(result1, 15714713.1254337);
  EXPECT_EQ(result2, -15714713.1254337);
}
