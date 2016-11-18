/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2013 The OpenSAF Foundation
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
#include <errno.h>

const struct timespec kZeroSeconds = { 0, 0 };
const struct timespec kTenMilliseconds = { 0, 10000000 };
const struct timespec kHundredMilliseconds = { 0, 100000000 };
const struct timespec kOneSecond = { 1, 0 };
const struct timespec kTwoSeconds = { 2, 0 };
const struct timespec kFiveSeconds = { 5, 0 };
const struct timespec kTenSeconds = { 10, 0 };
const struct timespec kFifteenSeconds = { 15, 0 };
const struct timespec kOneMinute = { 60, 0 };
const struct timespec kOneHour = { 3600, 0 };

void osaf_nanosleep(const struct timespec* sleep_duration)
{
	struct timespec wakeup_time;
	osaf_clock_gettime(CLOCK_MONOTONIC, &wakeup_time);
	osaf_timespec_add(&wakeup_time, sleep_duration, &wakeup_time);
	int retval;
	do {
		retval = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
					 &wakeup_time, NULL);
	} while (retval == EINTR);
	if (retval != 0) osaf_abort(retval);
}
