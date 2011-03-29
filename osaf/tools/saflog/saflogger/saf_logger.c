
/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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

/*
 * This file contains a command line utility to write to the SAF LOG.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <poll.h>
#include <unistd.h>
#include <limits.h>

#include <saAis.h>
#include <saLog.h>

#include "saf_error.h"

#define DEFAULT_FORMAT_EXPRESSION "@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Sv @Sl \"@Cb\""
#define DEFAULT_APP_LOG_REC_SIZE 128
#define DEFAULT_APP_LOG_FILE_SIZE 1024
#define VENDOR_ID 193
#define DEFAULT_MAX_FILES_ROTATED 4

static void logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error);

static SaLogCallbacksT logCallbacks = { 0, 0, logWriteLogCallbackT };
static SaVersionT logVersion = { 'A', 2, 1 };

static char *progname = "saflogger";
static SaInvocationT cb_invocation;
static SaAisErrorT cb_error;

static SaTimeT get_current_SaTime(void)
{
	struct timeval tv;
	SaTimeT ntfTime;

	gettimeofday(&tv, 0);

	ntfTime = ((unsigned)tv.tv_sec * 1000000000ULL) + ((unsigned)tv.tv_usec * 1000ULL);

	return ntfTime;
}

/**
 * 
 */
static void usage(void)
{
	printf("\nNAME\n");
	printf("\t%s - write log record to log stream\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [options] [message ...]\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is a SAF LOG client used to write a log record into a specified log stream.\n", progname);

	printf("\nOPTIONS\n");

	printf("  -h or --help                   this help\n");
	printf("  -l or --alarm                  write to alarm stream\n");
	printf("  -n or --notification           write to notification stream\n");
	printf("  -y or --system                 write to system stream (default)\n");
	printf("  -a NAME or --application=NAME  write to application stream NAME\n");
	printf("  -s SEV or --severity=SEV       use severity SEV, default INFO\n");
	printf("  -i INT or --interval=INT       write with interval INT (only with --count, default 1s)\n");
	printf("  -c CNT or --count=CNT          write CNT number of times, -1 forever (with interval INT) \n");
	printf("      valid severity names: emerg, alert, crit, error, warn, notice, info\n");
}

static void logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
{
	cb_invocation = invocation;
	cb_error = error;
}

/**
 * Write log record to stream, wait for ack
 * @param logRecord
 * @param interval
 * @param write_count
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT write_log_record(SaLogHandleT logHandle,
				    SaLogStreamHandleT logStreamHandle,
				    SaSelectionObjectT selectionObject,
				    const SaLogRecordT *logRecord, int interval, unsigned int write_count)
{
	SaAisErrorT errorCode;
	SaInvocationT invocation;
	int i = 0;
	int write_index = 0;
	int try_agains = 0;
	struct pollfd fds[1];
	int ret;
	unsigned int writes = 0;

	if (logRecord->logBuffer != NULL)
		write_index = strlen((char *)logRecord->logBuffer->logBuf);

	do {
		i++;

		/* Only add a unique ID if have a logBuffer AND periodic writes are requested */
		if (logRecord->logBuffer != NULL && write_count > 1) {
			/* add unique ID to each log */
			sprintf((char *)(&logRecord->logBuffer->logBuf[write_index]), " - %u", i);
			logRecord->logBuffer->logBufSize = strlen((char *)logRecord->logBuffer->logBuf);
		}

		invocation = random();
 retry:
		errorCode = saLogWriteLogAsync(logStreamHandle, invocation, SA_LOG_RECORD_WRITE_ACK, logRecord);
		if (errorCode == SA_AIS_ERR_TRY_AGAIN) {
			usleep(100000);	/* 100 ms */
			try_agains++;
			goto retry;
		}

		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saLogWriteLogAsync FAILED: %s\n", saf_error(errorCode));
			return errorCode;
		}

		fds[0].fd = (int)selectionObject;
		fds[0].events = POLLIN;
 poll_retry:
		ret = poll(fds, 1, 20000);

		if (ret == EINTR)
			goto poll_retry;

		if (ret == -1) {
			fprintf(stderr, "poll FAILED: %u\n", ret);
			return SA_AIS_ERR_BAD_OPERATION;
		}

		if (ret == 0) {
			fprintf(stderr, "poll timeout, message %u was most likely lost\n", i);
			continue;
		}

		errorCode = saLogDispatch(logHandle, SA_DISPATCH_ONE);
		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saLogDispatch FAILED: %s\n", saf_error(errorCode));
			return errorCode;
		}

		if (cb_invocation != invocation) {
			fprintf(stderr, "logWriteLogCallbackT FAILED: wrong invocation\n");
			return errorCode;
		}

		if ((cb_error != SA_AIS_ERR_TRY_AGAIN) && (cb_error != SA_AIS_OK)) {
			fprintf(stderr, "logWriteLogCallbackT FAILED: %s\n", saf_error(cb_error));
			return errorCode;
		}

		if (cb_error == SA_AIS_ERR_TRY_AGAIN) {
			usleep(100000);	/* 100 ms */
			try_agains++;
			goto retry;
		}

		if (try_agains > 0) {
			fprintf(stderr, "got %u SA_AIS_ERR_TRY_AGAIN, waited %u secs\n", try_agains, try_agains / 10);
			try_agains = 0;
		}

		writes++;

		if (writes < write_count)
			sleep(interval);

	} while (writes < write_count);

	return errorCode;
}

static SaLogSeverityT get_severity(char *severity)
{
	if (strcmp(severity, "emerg") == 0)
		return SA_LOG_SEV_EMERGENCY;

	if (strcmp(severity, "alert") == 0)
		return SA_LOG_SEV_ALERT;

	if (strcmp(severity, "crit") == 0)
		return SA_LOG_SEV_CRITICAL;

	if (strcmp(severity, "error") == 0)
		return SA_LOG_SEV_ERROR;

	if (strcmp(severity, "warn") == 0)
		return SA_LOG_SEV_WARNING;

	if (strcmp(severity, "notice") == 0)
		return SA_LOG_SEV_NOTICE;

	if (strcmp(severity, "info") == 0)
		return SA_LOG_SEV_INFO;

	usage();
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int c;
	SaNameT logStreamName = {.length = 0 };
	SaLogFileCreateAttributesT_2 *logFileCreateAttributes = NULL;
	SaLogFileCreateAttributesT_2 appLogFileCreateAttributes;
	SaLogStreamOpenFlagsT logStreamOpenFlags = 0;
	SaLogRecordT logRecord;
	SaNameT logSvcUsrName;
	SaLogBufferT logBuffer;
	SaNtfClassIdT notificationClassId = { 1, 2, 3 };
	struct option long_options[] = {
		{"application", required_argument, 0, 'a'},
		{"alarm", no_argument, 0, 'l'},
		{"notification", no_argument, 0, 'n'},
		{"system", no_argument, 0, 'y'},
		{"severity", required_argument, 0, 's'},
		{"interval", required_argument, 0, 'i'},
		{"count", required_argument, 0, 'c'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	int interval = 1;
	char hostname[_POSIX_HOST_NAME_MAX];
	int write_count = 1;
	SaAisErrorT error;
	SaLogHandleT logHandle;
	SaLogStreamHandleT logStreamHandle;
	SaSelectionObjectT selectionObject;

	srandom(getpid());

	if (gethostname(hostname, _POSIX_HOST_NAME_MAX) == -1) {
		fprintf(stderr, "gethostname failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	sprintf((char *)logSvcUsrName.value, "%s.%u@%s", "saflogger", getpid(), hostname);
	logSvcUsrName.length = strlen((char *)logSvcUsrName.value);

	/* Setup default values */
	strcpy((char *)logStreamName.value, SA_LOG_STREAM_SYSTEM);	/* system stream is default */
	logRecord.logTimeStamp = SA_TIME_UNKNOWN;	/* LOG service should supply timestamp */
	logRecord.logHdrType = SA_LOG_GENERIC_HEADER;
	logRecord.logHeader.genericHdr.notificationClassId = NULL;
	logRecord.logHeader.genericHdr.logSeverity = SA_LOG_SEV_INFO;
	logRecord.logHeader.genericHdr.logSvcUsrName = &logSvcUsrName;
	logRecord.logBuffer = NULL;
	appLogFileCreateAttributes.logFilePathName = "saflogger";
	appLogFileCreateAttributes.maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
	appLogFileCreateAttributes.maxLogRecordSize = DEFAULT_APP_LOG_REC_SIZE;
	appLogFileCreateAttributes.haProperty = SA_TRUE;
	appLogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
	appLogFileCreateAttributes.maxFilesRotated = DEFAULT_MAX_FILES_ROTATED;
	appLogFileCreateAttributes.logFileFmt = DEFAULT_FORMAT_EXPRESSION;

	while (1) {
		c = getopt_long(argc, argv, "hlnya:s:i:c:", long_options, NULL);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 'c':
			write_count = atoi(optarg);
			break;
		case 'l':
			strcpy((char *)logStreamName.value, SA_LOG_STREAM_ALARM);
			logRecord.logHdrType = SA_LOG_NTF_HEADER;
			break;
		case 'n':
			strcpy((char *)logStreamName.value, SA_LOG_STREAM_NOTIFICATION);
			logRecord.logHdrType = SA_LOG_NTF_HEADER;
			break;
		case 'y':
			strcpy((char *)logStreamName.value, SA_LOG_STREAM_SYSTEM);
			break;
		case 'a':
			sprintf((char *)logStreamName.value, "safLgStr=%s", optarg);
			logFileCreateAttributes = &appLogFileCreateAttributes;
			appLogFileCreateAttributes.logFileName = strdup(optarg);
			logStreamOpenFlags = SA_LOG_STREAM_CREATE;
			break;
		case 's':
			logRecord.logHeader.genericHdr.logSeverity = get_severity(optarg);
			break;
		case 'i':
			interval = atoi(optarg);
			break;
		case 'h':
		case '?':
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
		}
	}

	if (logRecord.logHdrType == SA_LOG_NTF_HEADER) {
		/* Setup some valid values */
		logRecord.logHeader.ntfHdr.notificationId = SA_NTF_IDENTIFIER_UNUSED;
		logRecord.logHeader.ntfHdr.eventType = SA_NTF_ALARM_PROCESSING;
		logRecord.logHeader.ntfHdr.notificationObject = &logSvcUsrName;
		logRecord.logHeader.ntfHdr.notifyingObject = &logSvcUsrName;
		logRecord.logHeader.ntfHdr.notificationClassId = &notificationClassId;
		logRecord.logHeader.ntfHdr.eventTime = get_current_SaTime();
	}

	logStreamName.length = strlen((char *)logStreamName.value);

	/* Create body of log record (if any) */
	if (optind < argc) {
		int sz;
		char *logBuf = NULL;
		sz = strlen(argv[optind]);
		logBuf = malloc(sz + 64);	/* add space for index/id in periodic writes */
		strcpy(logBuf, argv[optind]);
		logBuffer.logBufSize = sz;
		logBuffer.logBuf = (SaUint8T *)logBuf;
		logRecord.logBuffer = &logBuffer;
	}

	error = saLogInitialize(&logHandle, &logCallbacks, &logVersion);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "saLogInitialize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	error = saLogSelectionObjectGet(logHandle, &selectionObject);
	if (error != SA_AIS_OK) {
		printf("saLogSelectionObjectGet FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	error = saLogStreamOpen_2(logHandle, &logStreamName,
				  logFileCreateAttributes, logStreamOpenFlags, SA_TIME_ONE_SECOND, &logStreamHandle);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "saLogStreamOpen_2 FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	if (write_log_record(logHandle, logStreamHandle, selectionObject,
			     &logRecord, interval, (unsigned int)write_count) != SA_AIS_OK) {
		exit(EXIT_FAILURE);
	}

	error = saLogStreamClose(logStreamHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saLogStreamClose FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	error = saLogFinalize(logHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saLogFinalize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
