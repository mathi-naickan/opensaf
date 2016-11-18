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

/** @file
 *
 * This file contains an OpenSAF replacement of the Linux functions
 * timerfd_create(), timerfd_settime() and timerfd_gettime(). The main reason
 * for implementing these functions here is that they are missing in LSB (Linux
 * Standard Base), which means that the use these Linux functions are currently
 * prohibited in OpenSAF. As an additional benefit, the variants implemented
 * here can never fail (they will abort() on failure), which means that the user
 * does not have implement code for error handling.
 *
 * When using the functions declared in this header file, you must be consistent
 * and always use them (i.e. not mix the usage of these functions with the Linux
 * conuterparts). Also, when using the flags parameter to osaf_timerfd_create()
 * and osaf_timerfd_settime(), you must add the OSAF_ prefix to any flag value
 * that you use, e.g. use OSAF_TFD_TIMER_ABSTIME instead of TFD_TIMER_ABSTIME.
 *
 * Note that these functions are currently implemented using a linear search
 * algorithm to find the internal timer data structures. Therefore, they will
 * not be able to handle a large number of timers efficiently. The
 * reccomendation is to use a maximum of around ten timers per Linux process. If
 * more timers are needed, then these functions should be improved to use a more
 * efficient data structure.
 *
 * The definitions in this file are for internal use within OpenSAF only.
 */

#ifndef BASE_OSAF_TIMERFD_H_
#define BASE_OSAF_TIMERFD_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  OSAF_TFD_CLOEXEC = 0x80000,
  OSAF_TFD_NONBLOCK = 0x800
};

enum {
  OSAF_TFD_TIMER_ABSTIME = 0x1
};

#define OSAF_TFD_CLOEXEC OSAF_TFD_CLOEXEC
#define OSAF_TFD_NONBLOCK OSAF_TFD_NONBLOCK
#define OSAF_TFD_TIMER_ABSTIME OSAF_TFD_TIMER_ABSTIME

/**
 * @brief Create a timer.
 *
 * This is a convenience function that behaves exactly like the Linux function
 * timerfd_create(2), except that it will abort the process instead of returning
 * an error code in case of a failure. Note that you must use the
 * osaf_timerfd_settime(), osaf_timerfd_gettime() and osaf_timerfd_close()
 * functions instead of the standard Linux functions timerfd_settime(),
 * timerfd_gettime() and close() for manipulating timers created using this
 * function.
 *
 * The @a flags parameter can be a bitwise OR of the two flags OSAF_TFD_CLOEXEC
 * and OSAF_TFD_NONBLOCK, which have the meaning corresponding to the
 * TFD_CLOEXEC and TFD_NONBLOCK flags of timerfd_create(), respectively.
 *
 * The return value will always be greater than or equal to zero (i.e. this
 * function will never fail). To free the allocated timer, call
 * osaf_timerfd_close().
 */
extern int osaf_timerfd_create(clockid_t clock_id, int flags)
    __attribute__ ((__nothrow__, __leaf__));

/**
 * @brief Start, stop or change the time-out value of a timer.
 *
 * This is a convenience function that behaves exactly like the Linux function
 * timerfd_settime(2), except that it will abort the process instead of
 * returning an error code in case of a failure. Aslo, it currently has the
 * restriction that utmr->it_interval must be zero (i.e. periodic timeouts are
 * currently not supported). This function can only be used with timers created
 * using the osaf_timerfd_create() function.
 *
 * The @a flags parameter can be either zero or have the value
 * OSAF_TFD_TIMER_ABSTIME, which has the meaning corresponding to the
 * TFD_TIMER_ABSTIME flag of the timerfd_settime() function.
 */
extern void osaf_timerfd_settime(int ufd, int flags,
                                 const struct itimerspec* utmr,
                                 struct itimerspec* otmr)
    __attribute__ ((__nothrow__, __leaf__));

/**
 * @brief Start, stop or change the time-out value of a timer.
 *
 * This is a convenience function that behaves exactly like the Linux function
 * timerfd_gettime(2), except that it will abort the process instead of
 * returning an error code in case of a failure. This function can only be used
 * with timers created using the osaf_timerfd_create() function.
 */
extern void osaf_timerfd_gettime(int ufd, struct itimerspec* otmr)
    __attribute__ ((__nothrow__, __leaf__));

/**
 * @brief Delete a timer.
 *
 * This function is a convenience function that behaves exactly like the Linux
 * function close(2), except that it will abort the process instead of returning
 * an error code in case of a failure. This function can only be used with
 * timers created using the osaf_timerfd_create() function.
 */
extern void osaf_timerfd_close(int ufd);

#ifdef __cplusplus
}
#endif

#endif  // BASE_OSAF_TIMERFD_H_
