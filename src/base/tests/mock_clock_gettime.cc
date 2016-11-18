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

#include "mock_clock_gettime.h"
#include <cerrno>
#include "base/osaf_time.h"

timespec realtime_clock;
timespec monotonic_clock;
MockClockGettime mock_clock_gettime;

int clock_gettime(clockid_t clock_id, struct timespec* tp) {
  struct timespec* clock_source = nullptr;
  if (clock_id == CLOCK_REALTIME) {
    clock_source = &realtime_clock;
  } else if (clock_id == CLOCK_MONOTONIC) {
    clock_source = &monotonic_clock;
  } else {
    errno = EINVAL;
    return -1;
  }
  if (tp == nullptr) {
    errno = EFAULT;
    return -1;
  }
  if (tp != nullptr && clock_source != nullptr) *tp = *clock_source;
  osaf_timespec_add(&realtime_clock, &mock_clock_gettime.execution_time, &realtime_clock);
  osaf_timespec_add(&monotonic_clock, &mock_clock_gettime.execution_time, &monotonic_clock);
  if (mock_clock_gettime.return_value < 0) errno = mock_clock_gettime.errno_value;
  return mock_clock_gettime.return_value;
}
