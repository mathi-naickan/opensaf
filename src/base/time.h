/*
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

#ifndef BASE_TIME_H_
#define BASE_TIME_H_

#include <stdint.h>
#include <cassert>
#include <chrono>
#include <climits>
#include <ctime>
#include "base/macros.h"
#include "base/osaf_time.h"

static inline bool operator<(const timespec& ts1, const timespec& ts2) {
  return osaf_timespec_compare(&ts1, &ts2) < 0;
}

static inline bool operator<=(const timespec& ts1, const timespec& ts2) {
  return osaf_timespec_compare(&ts1, &ts2) <= 0;
}

static inline bool operator==(const timespec& ts1, const timespec& ts2) {
  return osaf_timespec_compare(&ts1, &ts2) == 0;
}

static inline bool operator!=(const timespec& ts1, const timespec& ts2) {
  return osaf_timespec_compare(&ts1, &ts2) != 0;
}

static inline bool operator>=(const timespec& ts1, const timespec& ts2) {
  return osaf_timespec_compare(&ts1, &ts2) >= 0;
}

static inline bool operator>(const timespec& ts1, const timespec& ts2) {
  return osaf_timespec_compare(&ts1, &ts2) > 0;
}

static inline timespec operator+(const timespec& ts1, const timespec& ts2) {
  timespec sum;
  osaf_timespec_add(&ts1, &ts2, &sum);
  return sum;
}

static inline timespec operator-(const timespec& ts1, const timespec& ts2) {
  timespec difference;
  assert(ts1 >= ts2);
  osaf_timespec_subtract(&ts1, &ts2, &difference);
  return difference;
}

static inline timespec& operator+=(timespec& ts1, const timespec& ts2) {
  osaf_timespec_add(&ts1, &ts2, &ts1);
  return ts1;
}

static inline timespec& operator-=(timespec& ts1, const timespec& ts2) {
  assert(ts1 >= ts2);
  osaf_timespec_subtract(&ts1, &ts2, &ts1);
  return ts1;
}

namespace base {

constexpr static const timespec kZeroSeconds{ 0, 0 };
constexpr static const timespec kOneMillisecond{ 0, 1000000 };
constexpr static const timespec kTenMilliseconds{ 0, 10000000 };
constexpr static const timespec kTwentyMilliseconds{ 0, 20000000 };
constexpr static const timespec kThirtyMilliseconds{ 0, 30000000 };
constexpr static const timespec kFourtyMilliseconds{ 0, 40000000 };
constexpr static const timespec kFiftyMilliseconds{ 0, 50000000 };
constexpr static const timespec kOneHundredMilliseconds{ 0, 100000000 };
constexpr static const timespec kTwoHundredMilliseconds{ 0, 200000000 };
constexpr static const timespec kThreeHundredMilliseconds{ 0, 300000000 };
constexpr static const timespec kFourHundredMilliseconds{ 0, 400000000 };
constexpr static const timespec kFiveHundredMilliseconds{ 0, 500000000 };
constexpr static const timespec kOneSecond{ 1, 0 };
constexpr static const timespec kFiveSeconds{ 5, 0 };
constexpr static const timespec kTenSeconds{ 10, 0 };
constexpr static const timespec kFifteenSeconds{ 15, 0 };
constexpr static const timespec kTwentySeconds{ 20, 0 };
constexpr static const timespec kTwentyfiveSeconds{ 25, 0 };
constexpr static const timespec kThirtySeconds{ 30, 0 };
constexpr static const timespec kOneMinute{ 1 * 60, 0 };
constexpr static const timespec kFiveMinutes{ 5 * 60, 0 };
constexpr static const timespec kTenMinutes{ 10 * 60, 0 };
constexpr static const timespec kFifteenMinutes{ 15 * 60, 0 };
constexpr static const timespec kTwentyMinutes{ 20 * 60, 0 };
constexpr static const timespec kTwentyfiveMinutes{ 25 * 60, 0 };
constexpr static const timespec kThirtyMinutes{ 30 * 60, 0 };
constexpr static const timespec kOneHour{ 60 * 60, 0 };
constexpr static const timespec kTimespecMax{ sizeof(time_t) == 8 ? INT64_MAX :
      INT32_MAX, 999999999 };

/**
 *  Read the current value of the system's real-time clock. For more
 *  information, refer to the POSIX function clock_gettime() and the clock id
 *  CLOCK_REALTIME.
 */
static inline timespec ReadRealtimeClock() {
  timespec ts;
  osaf_clock_gettime(CLOCK_REALTIME, &ts);
  return ts;
}

/**
 *  Read the current value of the system's monotonic clock. For more
 *  information, refer to the POSIX function clock_gettime() and the clock id
 *  CLOCK_MONOTONIC.
 */
static inline timespec ReadMonotonicClock() {
  timespec ts;
  osaf_clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts;
}

/**
 *  Suspend execution of the calling thread for the specified @a duration.
 */
static inline void Sleep(const timespec& duration) {
  osaf_nanosleep(&duration);
}

/**
 *  Convert the specified @a duration to a timespec structure.
 */
template<class Rep, class Period>
static inline timespec DurationToTimespec(
    const std::chrono::duration<Rep, Period>& duration) {
  timespec ts;
  std::chrono::nanoseconds nanos = duration;
  osaf_nanos_to_timespec(nanos.count(), &ts);
  return ts;
}

/**
 *  Convert the specified timeval structure to a timespec structure.
 */
static inline timespec TimevalToTimespec(const timeval& tv) {
  timespec ts;
  osaf_timeval_to_timespec(&tv, &ts);
  return ts;
}

/**
 *  Convert the specified number of @a seconds to a timespec structure.
 */
static inline timespec DoubleToTimespec(double seconds) {
  timespec ts;
  osaf_double_to_timespec(seconds, &ts);
  return ts;
}

/**
 *  Convert the specified number of @a milliseconds to a timespec structure.
 */
static inline timespec MillisToTimespec(uint64_t milliseconds) {
  timespec ts;
  osaf_millis_to_timespec(milliseconds, &ts);
  return ts;
}

/**
 *  Convert the specified number of @a microseconds to a timespec structure.
 */
static inline timespec MicrosToTimespec(uint64_t microseconds) {
  timespec ts;
  osaf_micros_to_timespec(microseconds, &ts);
  return ts;
}

/**
 *  Convert the specified number of @a nanoseconds to a timespec structure.
 */
static inline timespec NanosToTimespec(uint64_t nanoseconds) {
  timespec ts;
  osaf_nanos_to_timespec(nanoseconds, &ts);
  return ts;
}

/**
 *  Convert the specified timespec structure @a ts to a duration with nanosecond
 *  resolution.
 */
static inline std::chrono::nanoseconds TimespecToDuration(const timespec& ts) {
  return std::chrono::nanoseconds{osaf_timespec_to_nanos(&ts)};
}

/**
 *  Convert the specified timeval structure to a timespec structure.
 */
static inline timeval TimespecToTimeval(const timespec& ts) {
  timeval tv;
  osaf_timespec_to_timeval(&ts, &tv);
  return tv;
}

/**
 *  Convert the specified timespec structure @a ts to a double representing time
 *  in the unit of seconds.
 */
static inline double TimespecToDouble(const timespec& ts) {
  return osaf_timespec_to_double(&ts);
}

/**
 *  Convert the specified timespec structure @a ts to an integer representing
 *  time in the unit of milliseconds.
 */
static inline uint64_t TimespecToMillis(const timespec& ts) {
  return osaf_timespec_to_millis(&ts);
}

/**
 *  Convert the specified timespec structure @a ts to an integer representing
 *  time in the unit of microseconds.
 */
static inline uint64_t TimespecToMicros(const timespec& ts) {
  return osaf_timespec_to_micros(&ts);
}

/**
 *  Convert the specified timespec structure @a ts to an integer representing
 *  time in the unit of nanoseconds.
 */
static inline uint64_t TimespecToNanos(const timespec& ts) {
  return osaf_timespec_to_nanos(&ts);
}

/**
 *  Return the smallest of the two specified timespec structures.
 */
static inline timespec Min(const timespec& ts1, const timespec& ts2) {
  return osaf_timespec_compare(&ts1, &ts2) <= 0 ? ts1 : ts2;
}

/**
 *  Return the smallest of the three specified timespec structures.
 */
static inline timespec Min(const timespec& ts1, const timespec& ts2,
                           const timespec& ts3) {
  return osaf_timespec_compare(&ts1, &ts2) <= 0 ? Min(ts1, ts3) : Min(ts2, ts3);
}

/**
 *  Return the largest of the two specified timespec structures.
 */
static inline timespec Max(const timespec& ts1, const timespec& ts2) {
  return osaf_timespec_compare(&ts1, &ts2) >= 0 ? ts1 : ts2;
}

/**
 *  Return the largest of the three specified timespec structures.
 */
static inline timespec Max(const timespec& ts1, const timespec& ts2,
                           const timespec& ts3) {
  return osaf_timespec_compare(&ts1, &ts2) >= 0 ? Max(ts1, ts3) : Max(ts2, ts3);
}

/**
 * Set a timeout in milliseconds and check if that time has elapsed
 * The time can be set when the object is created and using a set method
 */
class Timer {
 public:
  // Set time in ms when creating Timer object
  explicit Timer(uint64_t i_millis)
    : m_ts_timeout_time({0, 0}) {
    set_timeout_time(i_millis);
  }
  ~Timer() {}

  // Set time after Timer object is created
  void set_timeout_time(uint64_t i_millis) {
    osaf_set_millis_timeout(i_millis, &m_ts_timeout_time);
  }

  // Return true if on set time or set time has expired
  const bool is_timeout() {
    return osaf_is_timeout(&m_ts_timeout_time);
  }

  // Return time left before timeout in ms
  const uint64_t time_left() {
    return osaf_timeout_time_left(&m_ts_timeout_time);
  }

 private:
  struct timespec m_ts_timeout_time;

  DELETE_COPY_AND_MOVE_OPERATORS(Timer);
};

} // namespace base

#endif  // BASE_TIME_H_
