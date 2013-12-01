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

#include "osaf_poll.h"
#include <errno.h>
#include <limits.h>
#include "osaf_time.h"
#include "osaf_utility.h"
#include "logtrace.h"

static unsigned osaf_poll_no_timeout(struct pollfd* io_fds, nfds_t i_nfds);

static unsigned osaf_poll_no_timeout(struct pollfd* io_fds, nfds_t i_nfds)
{
	int result;
	for (;;) {
		result = poll(io_fds, i_nfds, -1);
		if (result >= 0) break;
		if (result < 0 && errno != EINTR) osaf_abort(result);
	}
	return result;
}

unsigned osaf_poll(struct pollfd* io_fds, nfds_t i_nfds, int i_timeout)
{
	struct timespec timeout_ts;
	if (i_timeout < 0) return osaf_poll_no_timeout(io_fds, i_nfds);
	osaf_millis_to_timespec(i_timeout, &timeout_ts);
	return osaf_ppoll(io_fds, i_nfds, &timeout_ts, NULL);
}

unsigned osaf_ppoll(struct pollfd* io_fds, nfds_t i_nfds,
	const struct timespec* i_timeout_ts, const sigset_t* i_sigmask)
{
	/*
	 * The variable millisecond_round_up is used to round up the time-out
	 * value to the nearest higher number of milliseconds, since the poll()
	 * function only has millisecond resolution but the parameters to
	 * ppoll() have nanosecond resolution. This is done since it would be
	 * bad to time-out earlier than the requested time-out. Timing out later
	 * than requested is fine - in a time-share system you should always
	 * expect that some extra delays can happen at any time.
	 */
	static const struct timespec millisecond_round_up = {
		0, (kNanosPerSec / kMillisPerSec) - 1
	};
	/*
	 * The variable max_possible_timeout is used to check if the requested
	 * time-out is longer than INT_MAX milliseconds. In that case we have to
	 * sleep INT_MAX milliseconds first, and then when that time has elapsed
	 * we go to sleep again. This is done since poll() has a maximum sleep
	 * time of INT_MAX, which is much shorter than the maximum time that can
	 * be specified to ppoll() using a timespec structure.
	 */
	static const struct timespec max_possible_timeout = {
		INT_MAX / kMillisPerSec,
		(INT_MAX % kMillisPerSec) * (kNanosPerSec / kMillisPerSec)
	};
	struct timespec start_time;
	struct timespec time_left_ts;
	int result;

	if (i_sigmask != NULL) osaf_abort(EINVAL);
	if (i_timeout_ts == NULL) return osaf_poll_no_timeout(io_fds, i_nfds);

	osaf_clock_gettime(CLOCK_MONOTONIC, &start_time);
	time_left_ts = *i_timeout_ts;

	for (;;) {
		struct timespec current_time;
		struct timespec elapsed_time;
		int time_left;

		/* We don't want to time-out too early, so round up to next even
		 * number of milliseconds.
		 */
		osaf_timespec_add(&time_left_ts, &millisecond_round_up,
			&time_left_ts);

		if (osaf_timespec_compare(&time_left_ts,
			&max_possible_timeout) < 0) {
			time_left = osaf_timespec_to_millis(&time_left_ts);
		} else {
			time_left = INT_MAX;
		}

		/* ppoll() is currently not included in LSB, so we have to use
		 * poll() here unfortunately.
		 */
		result = poll(io_fds, i_nfds, time_left);
		if (result > 0 || (result == 0 && time_left < INT_MAX)) break;
		if (result < 0 && errno != EINTR) osaf_abort(result);

		osaf_clock_gettime(CLOCK_MONOTONIC, &current_time);
		if (osaf_timespec_compare(&current_time, &start_time) >= 0) {
			osaf_timespec_subtract(&current_time, &start_time,
				&elapsed_time);
		} else {
			/* Handle the unlikely case that the elapsed time is
			 * negative. Shouldn't happen with a monotonic clock,
			 * but just to be on the safe side.
			 */
			elapsed_time.tv_sec = 0;
			elapsed_time.tv_nsec = 0;
		}
		if (osaf_timespec_compare(&elapsed_time, i_timeout_ts) >= 0) {
			result = 0;
			break;
		}
		osaf_timespec_subtract(i_timeout_ts, &elapsed_time,
			&time_left_ts);
	}
	return result;
}

int osaf_poll_one_fd(int i_fd, int i_timeout)
{
	struct pollfd set;
	unsigned result;
	set.fd = i_fd;
	set.events = POLLIN;
	result = osaf_poll(&set, 1, i_timeout);
	if (result == 1) {
		if ((set.revents & (POLLNVAL | POLLERR)) != 0) {
			LOG_ER("osaf_poll_one_fd(%d, %d) called from %p "
			       "failed: revents=%hd",
			       i_fd, i_timeout, __builtin_return_address(0),
			       set.revents);
		}
		if ((set.revents & POLLNVAL) != 0) osaf_abort(set.revents);
		if ((set.revents & POLLERR) != 0) {
			errno = EIO;
			return -1;
		}
		if ((set.revents & POLLHUP) != 0) {
			errno = EPIPE;
			return -1;
		}
	}
	return result;
}
