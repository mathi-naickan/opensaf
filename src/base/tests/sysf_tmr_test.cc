/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>

#include "base/ncssysf_tmr.h"
#include "gtest/gtest.h"

using namespace std::chrono;

// The fixture for testing c-function sysf_tmr
class SysfTmrTest : public ::testing::Test {
 public:
 protected:

  SysfTmrTest() :
      distribution_(1 * 60 * 60 * 100, 2 * 60 * 60 * 100 - 1),
      timers_{} {
    // Setup work can be done here for each test.
  }

  virtual ~SysfTmrTest() {
    // Cleanup work that doesn't throw exceptions here.
  }

  virtual void SetUp() {
    // Code here will be called immediately after the constructor (right
    // before each test).
    bool rc = sysfTmrCreate();
    ASSERT_EQ(rc, true);
  }

  virtual void TearDown() {
    // Code here will be called immediately after each test (right
    // before the destructor).
    bool rc = sysfTmrDestroy();
    ASSERT_EQ(rc, true);
  }

  // Objects declared here can be used by all tests in the test case.
  static const int max_counter = 100;
  static int counter;
  static steady_clock::time_point last_time;
  static steady_clock::duration times[max_counter];
  static std::atomic<bool> finished;
  static std::atomic<bool> first_time_through;
  void createIntervalTimers(int64_t timeout_in_ms, int no_of_counters);
  static void TimerCallback(void* arg);
  static void IntervalTimerCallback(void *arg);
  std::mt19937 generator_;
  std::uniform_int_distribution<uint32_t> distribution_;
  tmr_t timers_[1000000];
  static bool expired_;
};

int SysfTmrTest::counter {0};
steady_clock::time_point SysfTmrTest::last_time {};
steady_clock::duration SysfTmrTest::times[max_counter] {};
std::atomic<bool> SysfTmrTest::finished {false};
std::atomic<bool> SysfTmrTest::first_time_through {true};

bool SysfTmrTest::expired_ {false};

//
void SysfTmrTest::IntervalTimerCallback(void *) {
  if (counter >= max_counter) {
    for (int i = 0; i < max_counter; i++) {
      std::cout << duration_cast<microseconds>(times[i]).count() << "\n";
    }
    finished = true;
    return;
  }

  auto elapsed = steady_clock::now();

  if (!first_time_through) {
    times[counter] = elapsed - last_time;
    ++counter;
  } else {
    first_time_through = false;
  }
  last_time = elapsed;
}

void SysfTmrTest::TimerCallback(void*) {
  expired_ = true;
}

//
void SysfTmrTest::createIntervalTimers(int64_t timeout_in_ms, int no_of_counters) {
  for (int i = 1; i <= no_of_counters; i++) {
    tmr_t tmr_id = ncs_tmr_alloc((char*) __FILE__, __LINE__);
    ASSERT_NE(tmr_id, nullptr);
    ncs_tmr_start(tmr_id, (timeout_in_ms * i) / 10, IntervalTimerCallback, 0, (char*) __FILE__, __LINE__);
  }
}

// Tests sysf_tmr as an interval timer and measure "jitter". Timeout values are written
// to stdout and can be read by e.g. gnuplot
TEST_F(SysfTmrTest, TestIntervalTimer) {
  createIntervalTimers(100, max_counter+2);

  while (!finished) {
    std::this_thread::yield();
  }
}

// Test creating a large number of sysf timers
TEST_F(SysfTmrTest, CreateOneMillionTimers) {
  expired_ = false;
  for (uint32_t i = 0; i != 1000000; ++i) {
    tmr_t tmr1 = ncs_tmr_alloc((char*)__FILE__, __LINE__);
    ASSERT_NE(tmr1, TMR_T_NULL);
    int64_t timeout = distribution_(generator_);
    tmr_t tmr2 = ncs_tmr_start(tmr1, timeout, TimerCallback, nullptr, (char*) __FILE__, __LINE__);
    ASSERT_NE(tmr2, TMR_T_NULL);
    timers_[i] = tmr2;
  }
  for (uint32_t i = 0;;) {
    ASSERT_NE(timers_[i], TMR_T_NULL);
    ncs_tmr_stop(timers_[i]);
    ncs_tmr_free(timers_[i]);
    timers_[i] = TMR_T_NULL;
    i = (i + 197167) % 1000000;
    if (i == 0) break;
  }
  for (uint32_t i = 0; i != 1000000; ++i) {
    ASSERT_EQ(timers_[i], TMR_T_NULL);
  }
  EXPECT_EQ(expired_, false);
}

