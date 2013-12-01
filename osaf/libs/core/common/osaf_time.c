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

#include "osaf_time.h"
#include <errno.h>

void osaf_nanosleep(const struct timespec* i_req)
{
	struct timespec req = *i_req;
	struct timespec start_time;
	osaf_clock_gettime(CLOCK_MONOTONIC, &start_time);
	for (;;) {
		struct timespec current_time;
		struct timespec elapsed_time;
		/* We could have utilised the second parameter to nanosleep(),
		 * which will return the remaining sleep time in the case
		 * nanosleep() was interrupted by a signal. But this gives
		 * inaccurate sleep time, for various reasons. See the man page
		 * of nanosleep(2) for details.
		 */
		int result = nanosleep(&req, NULL);
		if (result == 0) break;
		if (errno != EINTR) osaf_abort(result);
		osaf_clock_gettime(CLOCK_MONOTONIC, &current_time);
		osaf_timespec_subtract(&current_time, &start_time,
			&elapsed_time);
		if (osaf_timespec_compare(&current_time, &start_time) < 0) {
			/* Handle the unlikely case that the elapsed time is
			 * negative. Shouldn't happen with a monotonic clock,
			 * but just to be on the safe side.
			 */
			elapsed_time.tv_sec = 0;
			elapsed_time.tv_nsec = 0;
		}
		if (osaf_timespec_compare(&elapsed_time, i_req) >= 0) break;
		osaf_timespec_subtract(i_req, &elapsed_time, &req);
	}
}
