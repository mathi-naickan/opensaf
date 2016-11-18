/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2012 The OpenSAF Foundation
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
 * This file contains utility/convenience functions provided by libopensaf_core.
 * The definitions in this file are for internal use within OpenSAF only.
 */

#ifndef BASE_OSAF_UTILITY_H_
#define BASE_OSAF_UTILITY_H_

#include <pthread.h>

#ifdef  __cplusplus
extern "C" {
#endif

enum {
  kOsafUseSafeReboot = 1
};

/**
 * @brief Lock a pthreads mutex and abort the process in case of failure.
 *
 * This is a convenience function that behaves exactly like the POSIX function
 * pthread_mutex_lock(3P), except that instead of returning an error code it
 * aborts the process in case of an error. A message is logged to syslog before
 * aborting the process.
 */
static inline void osaf_mutex_lock_ordie(pthread_mutex_t* io_mutex)
    __attribute__ ((nonnull, nothrow, always_inline));

/**
 * @brief Unlock a pthreads mutex and abort the process in case of failure.
 *
 * This is a convenience function that behaves exactly like the POSIX function
 * pthread_mutex_unlock(3P), except that instead of returning an error code it
 * aborts the process in case of an error. A message is logged to syslog before
 * aborting the process.
 */
static inline void osaf_mutex_unlock_ordie(pthread_mutex_t* io_mutex)
    __attribute__ ((nonnull, nothrow, always_inline));

/**
 * @brief Log an error message and abort the process.
 *
 * This is a convenience function that calls syslog(3) to log an error message,
 * and then aborts the process by calling abort(3). The caller can provide a
 * numerical value @a i_cause specifying the cause of the abort, which for
 * example could be an error code. In addition, this function will log the
 * current value of errno.
 */
extern void osaf_abort(long i_cause)
    __attribute__ ((
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
        cold,
#endif
        nothrow, noreturn));

extern void osaf_safe_reboot(void)
    __attribute__ ((nothrow));

static inline void osaf_mutex_lock_ordie(pthread_mutex_t* io_mutex) {
  int result = pthread_mutex_lock(io_mutex);
  if (result != 0) osaf_abort(result);
}

static inline void osaf_mutex_unlock_ordie(pthread_mutex_t* io_mutex) {
  int result = pthread_mutex_unlock(io_mutex);
  if (result != 0) osaf_abort(result);
}

#ifdef  __cplusplus
}
#endif

#endif  // BASE_OSAF_UTILITY_H_
