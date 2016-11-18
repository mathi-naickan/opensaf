/*      -*- OpenSAF  -*-
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

#ifndef LOG_APITEST_LOGUTIL_H_
#define LOG_APITEST_LOGUTIL_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <stdarg.h>

#include <saLog.h>

extern struct LogProfile logProfile;

/**
 * Controls the behavior of the wrapper LOG API functions.
 */
struct LogProfile {
	/* Number of tries before we give up. */
	unsigned int nTries;
	/* Interval in milli-seconds between tries. */
	unsigned int retryInterval;
};

/* Same as system() but returns WEXITSTATUS if not -1 */
int systemCall(const char *command);

// Common interfaces
const char *hostname(void);
bool is_test_done_on_pl(void);
void cond_check(void);

SaAisErrorT logInitialize();
SaAisErrorT logFinalize();
SaAisErrorT logStreamOpen(const SaNameT *logStreamName);
SaAisErrorT logAppStreamOpen(const SaNameT *logStreamName,
			     const SaLogFileCreateAttributesT_2 *logFileCreateAttributes);
SaAisErrorT logStreamClose();
SaAisErrorT logWriteAsync(const SaLogRecordT *logRecord);

#endif  // LOG_APITEST_LOGUTIL_H_
