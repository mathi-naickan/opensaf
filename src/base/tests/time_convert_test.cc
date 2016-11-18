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

#include "base/time.h"
#include "gtest/gtest.h"

static const timespec kTimespec = { 1576315623, 358743382 };
static const timeval kTimeval = { 1576315623, 358743 };
static const timespec kTruncatedTimespec = { 1576315623, 358743000 };

TEST(BaseTimeConvert, TimespecToTimeval) {
  timeval result = base::TimespecToTimeval(kTimespec);
  EXPECT_EQ(result.tv_sec, kTimeval.tv_sec);
  EXPECT_EQ(result.tv_usec, kTimeval.tv_usec);
}

TEST(BaseTimeConvert, TimevalToTimespec) {
  EXPECT_EQ(base::TimevalToTimespec(kTimeval), kTruncatedTimespec);
}

TEST(BaseTimeConvert, MillisToTimespec) {
  const timespec expected_result = { 1428499100, 961000000 };
  EXPECT_EQ(base::MillisToTimespec(1428499100961ull), expected_result);
}

TEST(BaseTimeConvert, MicrosToTimespec) {
  const timespec expected_result = { 2125428499, 100961000 };
  EXPECT_EQ(base::MicrosToTimespec(2125428499100961ull), expected_result);
}

TEST(BaseTimeConvert, NanosToTimespec) {
  const struct timespec expected_result = { 1725125428, 499100961 };
  EXPECT_EQ(base::NanosToTimespec(1725125428499100961ull), expected_result);
}

TEST(BaseTimeConvert, DoubleToTimespec) {
  const struct timespec expected_result = { 15714713, 125433700 };
  const struct timespec expected_result2 = { -15714714, 874566300 };
  EXPECT_EQ(base::DoubleToTimespec(15714713.1254337), expected_result);
  EXPECT_EQ(base::DoubleToTimespec(-15714713.1254337), expected_result2);
}

TEST(BaseTimeConvert, DurationToTimespec) {
  const struct timespec expected_result = { 1725125428, 499100961 };
  std::chrono::nanoseconds duration{1725125428499100961ull};
  EXPECT_EQ(base::DurationToTimespec(duration), expected_result);
}

TEST(BaseTimeConvert, TimespecToMillis) {
  const struct timespec ts = { 1428499100, 961923266 };
  EXPECT_EQ(base::TimespecToMillis(ts), 1428499100961ull);
}

TEST(BaseTimeConvert, TimespecToMicros) {
  const struct timespec ts = { 1125428499, 100961923 };
  EXPECT_EQ(base::TimespecToMicros(ts), 1125428499100961ull);
}

TEST(BaseTimeConvert, TimespecToNanos) {
  const struct timespec ts = { 1725125428, 499100961 };
  EXPECT_EQ(base::TimespecToNanos(ts), 1725125428499100961ull);
}

TEST(BaseTimeConvert, TimespecToDouble) {
  const struct timespec ts = { 15714713, 125433700 };
  const struct timespec ts2 = { -15714714, 874566300 };
  EXPECT_EQ(base::TimespecToDouble(ts), 15714713.1254337);
  EXPECT_EQ(base::TimespecToDouble(ts2), -15714713.1254337);
}

TEST(BaseTimeConvert, TimespecToDuration) {
  const struct timespec ts = { 1725125428, 499100961 };
  std::chrono::nanoseconds expected_result{1725125428499100961ull};
  EXPECT_EQ(base::TimespecToDuration(ts), expected_result);
}
