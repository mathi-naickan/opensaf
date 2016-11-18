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

#include "mock_clock_nanosleep.h"
#include <cerrno>
#include "base/osaf_time.h"
#include "mock_clock_gettime.h"

int current_nanosleep_index = 0;
int number_of_nanosleep_instances = 1;
MockClockNanosleep mock_clock_nanosleep[kMockClockNanosleepInstances];

int clock_nanosleep(clockid_t clock_id, int flags,
                    const struct timespec *request,
                    struct timespec *remain) {
  MockClockNanosleep& mock_instance = mock_clock_nanosleep[current_nanosleep_index++];
  if (current_nanosleep_index >= number_of_nanosleep_instances) current_nanosleep_index = 0;
  struct timespec* clock_source = nullptr;
  if (clock_id == CLOCK_REALTIME) {
    clock_source = &realtime_clock;
  } else if (clock_id == CLOCK_MONOTONIC) {
    clock_source = &monotonic_clock;
  } else {
    return EINVAL;
  }
  if (request->tv_sec < 0 || request->tv_nsec < 0 || request->tv_nsec >= kNanosPerSec) {
    return EINVAL;
  }
  struct timespec sleep_duration = kZeroSeconds;
  if (flags == 0) {
    sleep_duration = *request;
  } else if (flags == TIMER_ABSTIME) {
    if (osaf_timespec_compare(request, clock_source) >= 0) {
      osaf_timespec_subtract(request, clock_source, &sleep_duration);
    }
  } else {
    return EINVAL;
  }
  if (mock_instance.return_value == EINTR) {
    osaf_nanos_to_timespec(osaf_timespec_to_nanos(&sleep_duration) / 2, &sleep_duration);
  }
  if (flags == 0 && remain != nullptr) {
    osaf_timespec_subtract(request, &sleep_duration, remain);
  }
  osaf_timespec_add(&realtime_clock, &sleep_duration, &realtime_clock);
  osaf_timespec_add(&monotonic_clock, &sleep_duration, &monotonic_clock);
  return mock_instance.return_value;
}
