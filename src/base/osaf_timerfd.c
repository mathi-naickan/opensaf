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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "base/osaf_timerfd.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "base/osaf_utility.h"
#include "base/osaf_time.h"

// Define a few constants that are missing in the LSB compiler
#ifndef MSG_DONTWAIT
enum {
	MSG_DONTWAIT = 0x40
};
#define MSG_DONTWAIT MSG_DONTWAIT
#endif
#ifndef SOCK_CLOEXEC
enum {
	SOCK_CLOEXEC = 0x80000
};
#define SOCK_CLOEXEC SOCK_CLOEXEC
#endif
#ifndef SOCK_NONBLOCK
enum {
	SOCK_NONBLOCK = 0x800
};
#define SOCK_NONBLOCK SOCK_NONBLOCK
#endif
#ifndef sigev_notify_function
#define sigev_notify_function   _sigev_un._sigev_thread._function
#endif
#ifndef sigev_notify_attributes
#define sigev_notify_attributes _sigev_un._sigev_thread._attribute
#endif

/*
 *  A data structure that keeps information about allocated timers.
 */
struct Timer {
	/*
	 *  Seqence number of this timer. Used by the event handler as a key to
	 *  find the timer related to the event. If the timer has already been
	 *  deleted when the event handler is called, then the event is ignored.
	 */
	int sequence_no;
	/*
	 *  The file descriptor that we return to the user. Part of a socket
	 *  pair together with write_socket.
	 */
	int read_socket;
	/*
	 *  The opposite file descriptor to read_socket, which both form a
	 *  socket pair. We use this file descriptor for writing, to signal that
	 *  a timer has expired.
	 */
	int write_socket;
	/*
	 *  The timer that we have allocated using timer_create().
	 */
	timer_t timer_id;
	/*
	 *  Next timer in the linked list pointed to by the variable timer_list.
	 */
	struct Timer* next_timer;
};

static struct Timer* FindTimer(int read_socket);
static void EventHandler(union sigval value);

/*
 *  Mutex that protects all the shared data (i.e. all other static variables in
 *  this file).
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 *  Pointer to the first entry in a single linked list of allocated Timer
 *  structures. The member next_timer in the Timer structure points to the next
 *  Timer in the list.
 */
static struct Timer* timer_list = NULL;

/*
 *  Next seqence number to be used when allocating a new timer. The sequence
 *  number will be stored in the member sequence_no and is used by the event
 *  handler as a key for finding the timer related to the event. This variable
 *  is initialized to a randomly chosen integer.
 */
static int next_sequence_no = 2124226019;

/*
 * Return a pointer to the timer whose read socket (the socket we return to the
 * user) equals @a read_socket.
 */
static struct Timer* FindTimer(int read_socket)
{
	struct Timer* timer;
	for (timer = timer_list; timer != NULL; timer = timer->next_timer) {
		if (timer->read_socket == read_socket) return timer;
	}
	// Not found
	osaf_abort(read_socket);
}

/*
 * Event handler for the timers. This function will be called in a separate
 * thread when a timer has expired. The sequence number of the expired timer is
 * found in the parameter value.sival_int, and is used as a key for looking up
 * the timer in the single linked list timer_list. If the timer is not found in
 * the list (since it has already been deleted), then the event will be ignored.
 */
static void EventHandler(union sigval value)
{
	osaf_mutex_lock_ordie(&mutex);
	struct Timer* timer = timer_list;
	while (timer != NULL && timer->sequence_no != value.sival_int) {
		timer = timer->next_timer;
	}
	if (timer != NULL) {
		uint64_t expirations = 1;
		ssize_t result;
		do {
			result = send(timer->write_socket,
				      &expirations, sizeof(expirations),
				      MSG_DONTWAIT | MSG_NOSIGNAL);
		} while (result == -1 && errno == EINTR);
		if (result != sizeof(expirations)) osaf_abort(result);
	}
	osaf_mutex_unlock_ordie(&mutex);
}

int osaf_timerfd_create(clockid_t clock_id, int flags)
{
	// Validate input parameters, abort on error
	if ((clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC) ||
	    (flags & ~(int) (OSAF_TFD_NONBLOCK | OSAF_TFD_CLOEXEC)) != 0) {
		osaf_abort(flags);
	}

	int sockflags = SOCK_DGRAM;
	if ((flags & OSAF_TFD_NONBLOCK) == OSAF_TFD_NONBLOCK) {
		sockflags |= SOCK_NONBLOCK;
	}
	if ((flags & OSAF_TFD_CLOEXEC) == OSAF_TFD_CLOEXEC) {
		sockflags |= SOCK_CLOEXEC;
	}

	int sfd[2];
	if (socketpair(AF_UNIX, sockflags, 0, sfd) != 0) osaf_abort(0);

	struct Timer* timer = (struct Timer*) malloc(sizeof(struct Timer));
	if (timer == NULL) osaf_abort(0);
	osaf_mutex_lock_ordie(&mutex);
	timer->sequence_no = next_sequence_no++;
	osaf_mutex_unlock_ordie(&mutex);
	timer->read_socket = sfd[0];
	timer->write_socket = sfd[1];

	struct sigevent event;
	memset(&event, 0, sizeof(event));
	event.sigev_notify = SIGEV_THREAD;
	event.sigev_value.sival_int = timer->sequence_no;
	event.sigev_notify_function = EventHandler;
	event.sigev_notify_attributes = NULL;
	int result;
	for (;;) {
		result = timer_create(clock_id, &event, &timer->timer_id);
		if (result != -1 || errno != EAGAIN) break;
		static const struct timespec sleep_time = { 0, 10000000 };
		osaf_nanosleep(&sleep_time);
	}
	if (result != 0) osaf_abort(clock_id);

	osaf_mutex_lock_ordie(&mutex);
	timer->next_timer = timer_list;
	timer_list = timer;
	osaf_mutex_unlock_ordie(&mutex);
	return sfd[0];
}

void osaf_timerfd_settime(int ufd, int flags,
			  const struct itimerspec* utmr,
			  struct itimerspec* otmr)
{
	if ((flags != 0 && flags != OSAF_TFD_TIMER_ABSTIME) ||
	    utmr->it_interval.tv_sec != 0 ||
	    utmr->it_interval.tv_nsec != 0) {
		osaf_abort(flags);
	}

	osaf_mutex_lock_ordie(&mutex);
	struct Timer* timer = FindTimer(ufd);
	for (;;) {
		uint64_t expirations;
		if (recv(timer->read_socket, &expirations,
			 sizeof(expirations), MSG_DONTWAIT) < 0) {
			if (errno == EINTR) continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK) break;
			osaf_abort(timer->read_socket);
		}
	}
	if (timer_settime(timer->timer_id,
			  flags == OSAF_TFD_TIMER_ABSTIME ? TIMER_ABSTIME : 0,
			  utmr,
			  otmr) != 0) {
		osaf_abort(flags);
	}
	osaf_mutex_unlock_ordie(&mutex);
}

void osaf_timerfd_gettime(int ufd, struct itimerspec* otmr)
{
	osaf_mutex_lock_ordie(&mutex);
	struct Timer* timer = FindTimer(ufd);
	if (timer_gettime(timer->timer_id, otmr) != 0) osaf_abort(ufd);
	osaf_mutex_unlock_ordie(&mutex);
}

void osaf_timerfd_close(int ufd)
{
	osaf_mutex_lock_ordie(&mutex);
	struct Timer** prev = &timer_list;
	struct Timer* timer;
	for (;;) {
		timer = *prev;
		if (timer == NULL) osaf_abort(ufd);
		if (timer->read_socket == ufd) {
			*prev = timer->next_timer;
			break;
		}
		prev = &timer->next_timer;
	}
	osaf_mutex_unlock_ordie(&mutex);

	if (timer_delete(timer->timer_id) != 0 ||
	    (close(timer->write_socket) != 0 && errno != EINTR) ||
	    (close(timer->read_socket) != 0 && errno != EINTR)) {
		osaf_abort(ufd);
	}
	free(timer);
}
