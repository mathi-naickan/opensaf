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

/** @file
 *
 * This file contains an OpenSAF replacement of the POSIX function poll(). The
 * definitions in this file are for internal use within OpenSAF only.
 */

#ifndef OPENSAF_BASE_OSAF_POLL_H_
#define OPENSAF_BASE_OSAF_POLL_H_

#include <poll.h>
#include <time.h>
#include <signal.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * @brief Wait for events on file descriptors
 *
 * This is a convenience function that behaves exactly like the POSIX function
 * poll(3P), except that it will abort the process instead of returning an error
 * code in case of a failure. It handles the EINTR case internally with a loop,
 * and ensures that the function will time out at the same time no matter if the
 * system call was interrupted by a signal or not.
 *
 * The return value will always be in the range [0, i_nfds].
 */
extern unsigned osaf_poll(struct pollfd* io_fds, nfds_t i_nfds, int i_timeout);

/**
 * @brief Wait for events on file descriptors
 *
 * This is a convenience function that behaves exactly like the Linux function
 * ppoll(3), except that it will abort the process instead of returning an error
 * code in case of a failure. It handles the EINTR case internally with a loop,
 * and ensures that the function will time out at the same time no matter if the
 * system call was interrupted by a signal or not. Note that since the ppoll()
 * function is currently not included in LSB, this function is implemented
 * internally using poll(). Therefore, the @a i_sigmask parameter is not
 * supported and must be set to NULL.
 *
 * The return value will always be in the range [0, i_nfds].
 */
extern unsigned osaf_ppoll(struct pollfd* io_fds, nfds_t i_nfds,
	const struct timespec* i_timeout_ts, const sigset_t* i_sigmask);

/**
 * @brief Wait for events on a file descriptor
 *
 * This is a convenience function that behaves exactly like the POSIX function
 * poll(3P), except that it only supports waiting for read events on one single
 * file descriptor passed as the parameter @a i_fd. Also, it will abort the
 * process instead of returning an error code in case the poll() function
 * fails. It handles the EINTR case internally with a loop, and ensures that the
 * function will time out at the same time no matter if the system call was
 * interrupted by a signal or not.
 *
 * Since there is no struct pollfd* parameter, this convenience function has a
 * different way of reporting what events were received on the file descriptor
 * @a i_fd. It will return 1 if there was a POLLIN event on the file descriptor
 * (just like poll() does). But if poll() returned an error event for the file
 * descriptor @a i_fd, this function will handle the there different cases as
 * follows:
 *
 * POLLNVAL - in this case the process will be aborted.
 *
 * POLLERR - errno will be set to EIO and -1 will be returned.
 *
 * POLLHUP - errno will be set to EPIPE and -1 will be returned.
 *
 * The return value from this function will always be in the range [-1, 1].
 */
extern int osaf_poll_one_fd(int i_fd, int i_timeout);

#ifdef	__cplusplus
}
#endif

#endif	/* OPENSAF_BASE_OSAF_POLL_H_ */
