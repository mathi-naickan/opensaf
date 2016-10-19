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

#include "osaf_time.h"
#include "gtest/gtest.h"
#include "mock_clock_gettime.h"

time_t sec = 1000;
long nsec = 1000;
struct timespec ts_expresult = {0, 0};

void setupTest(uint64_t timeout_ms) {
  realtime_clock = { 0, 0 };
  monotonic_clock = { sec, nsec };
  mock_clock_gettime.return_value = 0;

  struct timespec ts_add = {0, 0};
  osaf_millis_to_timespec(timeout_ms, &ts_add);
  struct timespec ts_cur = {0, 0};
  osaf_clock_gettime(CLOCK_MONOTONIC, &ts_cur);
  osaf_timespec_add(&ts_cur, &ts_add, &ts_expresult);
}

void advanceTime(uint64_t i_millis) {
  struct timespec ts_cur = {0, 0};
  osaf_clock_gettime(CLOCK_MONOTONIC, &ts_cur);
  struct timespec ts_add = {0, 0};
  osaf_millis_to_timespec(i_millis, &ts_add);
  osaf_timespec_add(&ts_cur, &ts_add, &monotonic_clock);
}

TEST(OsafSetMillisTimeout, FiveSecTimeout) {
  setupTest(5000);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(5000, &ts_timeout);

  EXPECT_EQ(ts_timeout.tv_sec, ts_expresult.tv_sec);
  EXPECT_EQ(ts_timeout.tv_nsec, ts_expresult.tv_nsec);
}

TEST(OsafSetMillisTimeout, ZeroSecTimeout) {
  setupTest(0);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(0, &ts_timeout);

  EXPECT_EQ(ts_timeout.tv_sec, ts_expresult.tv_sec);
  EXPECT_EQ(ts_timeout.tv_nsec, ts_expresult.tv_nsec);
}

TEST(OsafSetMillisTimeout, OneMSecTimeout) {
  setupTest(1);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(1, &ts_timeout);

  EXPECT_EQ(ts_timeout.tv_sec, ts_expresult.tv_sec);
  EXPECT_EQ(ts_timeout.tv_nsec, ts_expresult.tv_nsec);
}

TEST(OsafCheckIsTimeout, NotTimeutOneSecLeft) {
    setupTest(5000);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(5000, &ts_timeout);
  advanceTime(4000);
  bool rc_timeout = true;
  rc_timeout = osaf_is_timeout(&ts_timeout);

  EXPECT_EQ(rc_timeout, false);
}

TEST(OsafCheckIsTimeout, NotTimeutOneMilliSecLeft) {
    setupTest(5000);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(5000, &ts_timeout);
  advanceTime(4999);
  bool rc_timeout = true;
  rc_timeout = osaf_is_timeout(&ts_timeout);

  EXPECT_EQ(rc_timeout, false);
}

TEST(OsafCheckIsTimeout, TimeutZeroMilliSecLeft) {
    setupTest(5000);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(5000, &ts_timeout);
  advanceTime(5000);
  bool rc_timeout = true;
  rc_timeout = osaf_is_timeout(&ts_timeout);

  EXPECT_EQ(rc_timeout, true);
}

TEST(OsafCheckIsTimeout, TimeutTimePassed) {
    setupTest(5000);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(5000, &ts_timeout);
  advanceTime(5500);
  bool rc_timeout = true;
  rc_timeout = osaf_is_timeout(&ts_timeout);

  EXPECT_EQ(rc_timeout, true);
}

TEST(OsafCheckTimeLeft, TimeutFourSecLeft) {
    setupTest(5000);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(5000, &ts_timeout);
  advanceTime(1000);
  uint64_t time_left = 0;
  time_left = osaf_timeout_time_left(&ts_timeout);

  EXPECT_EQ(time_left, 4000);
}

TEST(OsafCheckTimeLeft, TimeutOneMsLeft) {
    setupTest(5000);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(5000, &ts_timeout);
  advanceTime(4999);
  uint64_t time_left = 0;
  time_left = osaf_timeout_time_left(&ts_timeout);

  EXPECT_EQ(time_left, 1);
}

TEST(OsafCheckTimeLeft, TimeutZeroMsLeft) {
    setupTest(5000);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(5000, &ts_timeout);
  advanceTime(5000);
  uint64_t time_left = 0;
  time_left = osaf_timeout_time_left(&ts_timeout);

  EXPECT_EQ(time_left, 0);
}

TEST(OsafCheckTimeLeft, TimeutPassedTimeout) {
    setupTest(5000);

  struct timespec ts_timeout = { 0, 0 };
  osaf_set_millis_timeout(5000, &ts_timeout);
  advanceTime(5500);
  uint64_t time_left = 0;
  time_left = osaf_timeout_time_left(&ts_timeout);

  EXPECT_EQ(time_left, 0);
}
