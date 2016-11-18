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

#include "base/saf_error.h"
#include "logtest.h"

#include "log/apitest/logutil.h"

struct LogProfile logProfile = { 25, 100 };
static char host_name[_POSIX_HOST_NAME_MAX] = {0};

const char *hostname(void)
{
	gethostname(host_name, _POSIX_HOST_NAME_MAX);
	return host_name;
}

bool is_test_done_on_pl(void)
{
	return (strncmp(hostname(), "PL-", strlen("PL-")) == 0);
}

void cond_check(void)
{
	if (is_test_done_on_pl() == false) {
		fprintf(stderr, "Test must be done on payload node \n");
		exit(EXIT_FAILURE);
	}
}

/**
 * Wrapper function for 'system'
 *
 * @param command[in] Same as system()
 * @return -1 on system error else return WEXITSTATUS return code
 *
 */
int systemCall(const char *command)
{
	int rc = system(command);
	if (rc == -1) {
		fprintf(stderr, "system() retuned -1 Failed \n");
	} else {
		rc = WEXITSTATUS(rc);
		if (rc != 0)
			fprintf(stderr, " Failed in command: %s \n", command);
	}

	return rc;
}

/*
 * Wrapper function for 'saLogInitialize'
 */
SaAisErrorT logInitialize()
{
	SaAisErrorT rc = saLogInitialize(&logHandle, &logCallbacks, &logVersion);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < logProfile.nTries) {
		usleep(logProfile.retryInterval * 1000);
		rc = saLogInitialize(&logHandle, &logCallbacks, &logVersion);
		nTries++;
	}

	return rc;
}

/*
 * Wrapper function for 'saLogFinalize'
 */
SaAisErrorT logFinalize()
{
	SaAisErrorT rc = saLogFinalize(logHandle);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < logProfile.nTries) {
		usleep(logProfile.retryInterval * 1000);
		rc = saLogFinalize(logHandle);
		nTries++;
	}

	return rc;
}

/*
 * Wrapper function for 'saLogStreamOpen_2'
 */
SaAisErrorT logStreamOpen(const SaNameT *logStreamName)
{
	SaAisErrorT rc = saLogStreamOpen_2(logHandle, logStreamName, NULL, 0,
					SA_TIME_ONE_SECOND, &logStreamHandle);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < logProfile.nTries) {
		usleep(logProfile.retryInterval * 1000);
		rc = saLogStreamOpen_2(logHandle, logStreamName, NULL, 0,
				SA_TIME_ONE_SECOND, &logStreamHandle);
		nTries++;
	}

	return rc;
}

/*
 * Wrapper function for 'saLogStreamOpen_2 for app stream'
 */
SaAisErrorT logAppStreamOpen(const SaNameT *logStreamName,
			const SaLogFileCreateAttributesT_2 *logFileCreateAttributes)
{
	SaAisErrorT rc = saLogStreamOpen_2(logHandle, logStreamName, logFileCreateAttributes,
					SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < logProfile.nTries) {
		usleep(logProfile.retryInterval * 1000);
		rc = saLogStreamOpen_2(logHandle, logStreamName, logFileCreateAttributes,
				SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
		nTries++;
	}

	return rc;
}

/*
 * Wrapper function for 'saLogWriteLogAsync'
 */
SaAisErrorT logWriteAsync(const SaLogRecordT *logRecord)
{
	SaAisErrorT rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, logRecord);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < logProfile.nTries) {
		usleep(logProfile.retryInterval * 1000);
		rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, logRecord);
		nTries++;
	}

	return rc;
}


/*
 * Wrapper function for 'saLogStreamClose'
 */
SaAisErrorT logStreamClose()
{
	SaAisErrorT rc = saLogStreamClose(logStreamHandle);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < logProfile.nTries) {
		usleep(logProfile.retryInterval * 1000);
		rc = saLogStreamClose(logStreamHandle);
		nTries++;
	}

	return rc;
}
