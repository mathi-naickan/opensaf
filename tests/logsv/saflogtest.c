
/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2011 The OpenSAF Foundation
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
 * This file contains a command line test utility to write to the SAF LOG.
 *
 * The main purpose of the utility is to send a burst of log messages, by
 * default not waiting for ack for each message.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

#define DEFAULT_FORMAT_EXPRESSION "@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Sv @Sl \"@Cb\""
#define DEFAULT_APP_LOG_REC_SIZE 128
#define DEFAULT_APP_LOG_FILE_SIZE 1024 * 1024
#define VENDOR_ID 193
#define DEFAULT_MAX_FILES_ROTATED 4

#define MAX_NUM_STREAM 4

static void logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error);

static SaLogCallbacksT logCallbacks = { 0, 0, logWriteLogCallbackT };
static SaVersionT logVersion = { 'A', 2, 1 };

static char *progname = "saflogtest";
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
	printf("\t%s - write log record(s) to log stream\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [options] [message ...]\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is a SAF LOG client used to write a log record into a specified log stream.\n", progname);

	printf("\nOPTIONS\n");

	printf("  -h or --help                   this help\n");
	printf("  -k or --ack                    do not wait for ack after write, default wait\n");
	printf("  -l or --alarm                  write to alarm stream\n");
	printf("  -n or --notification           write to notification stream\n");
	printf("  -y or --system                 write to system stream (default)\n");
	printf("%s%s%s",
		   "  -a NAME or --application=NAME  write to application stream NAME\n",
		   "     Can be used more than once, more than one stream can be opened\n",
		   "     Cannot be used in combination with other streams\n");
	printf("     Max %d application streams can be created\n",MAX_NUM_STREAM);
	printf("     Example: > saflogtest -a tst1 \"tst1 msg\" -a tst2 -a tst3 \"tst3 msg\"\n");
	printf("  -b NAME or --capplication=NAME write to config application stream NAME\n");
	printf("  -s SEV or --severity=SEV       use severity SEV, default INFO\n");
	printf("     valid severity names: emerg, alert, crit, error, warn, notice, info\n");
	printf("  -i INT or --interval=INT       write with interval INT us (only with --count, default 0us)\n");
	printf("  -c CNT or --count=CNT          write CNT number of times, -1 forever (with interval INT) \n");
	printf("  -o Open log stream(s) and wait forever. Exit on any key\n");
	printf("  -e Exit without closing stream(s) or finalizing after log records are written\n");
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
				    const SaLogRecordT *logRecord,
				    bool wait_for_ack)
{
	SaAisErrorT errorCode;
	SaInvocationT invocation = random();
	struct pollfd fds[1];
	int ret;
	static int i = 0;
	int try_agains = 0;
	SaLogAckFlagsT ackflags;
	
	if (wait_for_ack)
		ackflags = SA_LOG_RECORD_WRITE_ACK;
	else
		ackflags = 0;

 retry:
	errorCode = saLogWriteLogAsync(logStreamHandle, invocation, ackflags, logRecord);

	if (errorCode != SA_AIS_OK) {
		fprintf(stderr, "saLogWriteLogAsync FAILED: %u\n", errorCode);
		return errorCode;
	}

	if (wait_for_ack) {
		fds[0].fd = (int)selectionObject;
		fds[0].events = POLLIN;
		ackflags = SA_LOG_RECORD_WRITE_ACK;

 poll_retry:
		ret = poll(fds, 1, 20000);

		if (ret == EINTR)
			goto poll_retry;

		if (ret == -1) {
			fprintf(stderr, "poll FAILED: %u\n", ret);
			return SA_AIS_ERR_BAD_OPERATION;
		}

		if (ret == 0) {
			fprintf(stderr, "poll timeout, message %u was most likely lost\n", i++);
			return SA_AIS_ERR_BAD_OPERATION;
		}

		errorCode = saLogDispatch(logHandle, SA_DISPATCH_ONE);
		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saLogDispatch FAILED: %u\n", errorCode);
			return errorCode;
		}

		if (cb_invocation != invocation) {
			fprintf(stderr, "logWriteLogCallbackT FAILED: wrong invocation\n");
			return SA_AIS_ERR_BAD_OPERATION;
		}

		if ((cb_error != SA_AIS_ERR_TRY_AGAIN) && (cb_error != SA_AIS_OK)) {
			fprintf(stderr, "logWriteLogCallbackT FAILED: %u\n", cb_error);
			return cb_error;
		}

		if (cb_error == SA_AIS_ERR_TRY_AGAIN) {
			usleep(100000);	/* 100 ms */
			try_agains++;
			goto retry;
		}

		if (try_agains > 0) {
			fprintf(stderr, "\tgot %u SA_AIS_ERR_TRY_AGAIN, waited %u secs\n", try_agains, try_agains / 10);
		}
	}

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

/**
 * Creates a log record by filling in the log service API struct of
 * type SaLogRecordT.
 * Note: When the log record is written it has to be freed.
 *       Use free_log_record() function 
 * 
 * @param logRecord[out]
 * @param logHdrType[in]
 * @param logSvcUsrName[in]
 * @param notificationClassId[in]
 *        Can be set to NULL if Hdr-type is SA_LOG_GENERIC_HEADER
 * @param log_message[in]
 *        Set to NULL if there is no message
 */
static void create_log_record(SaLogRecordT *logRecord,
						SaLogHeaderTypeT logHdrType,
						SaNameT *logSvcUsrName,
						SaNtfClassIdT *notificationClassId,
						char *log_message,
						SaLogBufferT *logBuffer)
{
	logRecord->logTimeStamp = SA_TIME_UNKNOWN;	/* LOG service should supply timestamp */
	logRecord->logBuffer = NULL;
		
	if (logHdrType == SA_LOG_GENERIC_HEADER) {
		/* Setup generic header */
		logRecord->logHdrType = SA_LOG_GENERIC_HEADER;
		logRecord->logHeader.genericHdr.notificationClassId = NULL;
		logRecord->logHeader.genericHdr.logSeverity = SA_LOG_SEV_INFO;
		logRecord->logHeader.genericHdr.logSvcUsrName = logSvcUsrName;
	} else if (logHdrType == SA_LOG_NTF_HEADER) {
		/* Setup header for Alarm and Notification record */
		logRecord->logHdrType = SA_LOG_NTF_HEADER;
		logRecord->logHeader.ntfHdr.notificationId = SA_NTF_IDENTIFIER_UNUSED;
		logRecord->logHeader.ntfHdr.eventType = SA_NTF_ALARM_PROCESSING;
		logRecord->logHeader.ntfHdr.notificationObject = logSvcUsrName;
		logRecord->logHeader.ntfHdr.notifyingObject = logSvcUsrName;
		logRecord->logHeader.ntfHdr.notificationClassId = notificationClassId;
		logRecord->logHeader.ntfHdr.eventTime = get_current_SaTime();
	} else {
		printf("\nInternal error when setting up log header\n");
		exit(EXIT_FAILURE);
	}

	/* Create body of log record (if any) */
	int sz;
	char *logBuf = NULL;
	
	if (log_message != NULL) {
		sz = strlen(log_message)+1;
		logBuf = malloc(sz + 64);	/* add space for index/id in periodic writes */
		strcpy(logBuf, log_message);
		logBuffer->logBufSize = sz;
		logBuffer->logBuf = (SaUint8T *)logBuf;
		logRecord->logBuffer = logBuffer;
	} else {
		logRecord->logBuffer = NULL;
	}
}

static void free_loc_record(SaLogRecordT *logRecord)
{
	if (logRecord->logBuffer != NULL) {
		free(logRecord->logBuffer->logBuf);
	}
}

static void set_logFileCreateAttributes(
					SaLogFileCreateAttributesT_2 *logFileCreateAttributes,
					SaStringT logFileName)
{
	/* Setup default values used when creating an application stream using API */
	logFileCreateAttributes->logFilePathName = "saflogtest";
	logFileCreateAttributes->maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
	logFileCreateAttributes->maxLogRecordSize = DEFAULT_APP_LOG_REC_SIZE;
	logFileCreateAttributes->haProperty = SA_TRUE;
	logFileCreateAttributes->logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
	logFileCreateAttributes->maxFilesRotated = DEFAULT_MAX_FILES_ROTATED;
	logFileCreateAttributes->logFileFmt = DEFAULT_FORMAT_EXPRESSION;
	logFileCreateAttributes->logFileName = strdup(logFileName);
}

static void free_logFileCreateAttributes(
					SaLogFileCreateAttributesT_2 *logFileCreateAttributes)
{
	if (logFileCreateAttributes != NULL) {
		free(logFileCreateAttributes->logFileName);
	}
}


int main(int argc, char *argv[])
{
	int c, i;
	//SaNameT logStreamName;
	SaLogStreamOpenFlagsT logStreamOpenFlags = 0;
	SaNameT logSvcUsrName;
	SaNtfClassIdT notificationClassId = { 1, 2, 3 };
	struct option long_options[] = {
		{"ack", no_argument, 0, 'k'},
		{"application", required_argument, 0, 'a'},
		{"capplication", required_argument, 0, 'b'},
		{"alarm", no_argument, 0, 'l'},
		{"notification", no_argument, 0, 'n'},
		{"system", no_argument, 0, 'y'},
		{"severity", required_argument, 0, 's'},
		{"interval", required_argument, 0, 'i'},
		{"count", required_argument, 0, 'c'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	int interval = 0;
	char hostname[_POSIX_HOST_NAME_MAX];
	int write_count = 1;
	SaAisErrorT error;
	SaLogHandleT logHandle;
	SaLogStreamHandleT logStreamHandle;
	SaSelectionObjectT selectionObject;
	bool wait_for_ack = true;
	bool do_not_exit_f = false;
	bool no_finalize_exit = false;
	char *log_message;
	SaLogSeverityT logSeverity = SA_LOG_SEV_INFO;
		
	int stream_cnt = 0;
	
	/* Used with application streams to save info about all app streams to
	 * create and send log-records to.
	 */
	struct {
	   SaNameT logStreamName;
	   SaLogRecordT logRecord;
	} stream_info[MAX_NUM_STREAM];
	
	SaLogBufferT logBuffer[MAX_NUM_STREAM];
	SaLogFileCreateAttributesT_2 logFileCreateAttributes[MAX_NUM_STREAM];
	SaLogFileCreateAttributesT_2 *logFileCreateAttributes_ptr = NULL;
	bool is_appl_stream = false;
	
	srandom(getpid());

	if (gethostname(hostname, _POSIX_HOST_NAME_MAX) == -1) {
		fprintf(stderr, "gethostname failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	sprintf((char *)logSvcUsrName.value, "%s.%u@%s", "saflogtest", getpid(), hostname);
	logSvcUsrName.length = strlen((char *)logSvcUsrName.value);

	while (1) {
		c = getopt_long(argc, argv, "eohklnya:b:s:i:c:", long_options, NULL);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 'e': /* Exit without closing any stream and without finalize */
			no_finalize_exit = true;
			break;
		case 'o': /* Open log stream and wait forever. Exit on Enter key */
			do_not_exit_f = true;
			break;
		case 'c':
			write_count = atoi(optarg);
			break;
		case 'k':
			wait_for_ack = false;
			break;
		case 'l':
			strcpy((char *) stream_info[0].logStreamName.value, SA_LOG_STREAM_ALARM);
			stream_info[0].logStreamName.length = strlen(SA_LOG_STREAM_ALARM);
			if (optind < argc) {
				log_message = argv[optind];
			} else {
				log_message = NULL;
			}
			create_log_record(&stream_info[0].logRecord, SA_LOG_NTF_HEADER, &logSvcUsrName,
					&notificationClassId, log_message, &logBuffer[0]);
			stream_cnt = 1;
			break;
		case 'n':
			strcpy((char *)stream_info[0].logStreamName.value, SA_LOG_STREAM_NOTIFICATION);
			stream_info[0].logStreamName.length = strlen(SA_LOG_STREAM_NOTIFICATION);
			if (optind < argc) {
				log_message = argv[optind];
			} else {
				log_message = NULL;
			}
			create_log_record(&stream_info[0].logRecord, SA_LOG_NTF_HEADER, &logSvcUsrName,
					&notificationClassId, log_message, &logBuffer[0]);
			stream_cnt = 1;
			break;
		case 'y':
			strcpy((char *)stream_info[0].logStreamName.value, SA_LOG_STREAM_SYSTEM);
			stream_info[0].logStreamName.length = strlen(SA_LOG_STREAM_SYSTEM);
			if (optind < argc) {
				log_message = argv[optind];
			} else {
				log_message = NULL;
			}
			create_log_record(&stream_info[0].logRecord, SA_LOG_GENERIC_HEADER, &logSvcUsrName,
					&notificationClassId, log_message, &logBuffer[0]);
			stream_cnt = 1;
			break;
		case 'a':
			if (stream_cnt >= MAX_NUM_STREAM) {
				printf("Max %d streams can be opened\n",MAX_NUM_STREAM);
				break;
			}
			is_appl_stream = true;
			set_logFileCreateAttributes(&logFileCreateAttributes[stream_cnt],optarg);
			logStreamOpenFlags = SA_LOG_STREAM_CREATE;
			sprintf((char *)stream_info[stream_cnt].logStreamName.value,
					"safLgStr=%s", optarg);
			stream_info[stream_cnt].logStreamName.length = 
					strlen((char *)stream_info[stream_cnt].logStreamName.value);
			if (optind < argc) {
				if (strcmp(argv[optind], "-a") != 0) {
					log_message = argv[optind];
				} else {
					log_message = NULL;
				}
			} else {
				log_message = NULL;
			}
			create_log_record(&stream_info[stream_cnt].logRecord,
					SA_LOG_GENERIC_HEADER, &logSvcUsrName,
					&notificationClassId, log_message, &logBuffer[stream_cnt]);
			stream_cnt++;
			break;
		case 'b':
			sprintf((char *) stream_info[0].logStreamName.value, "safLgStrCfg=%s", optarg);
			strcat((char *) stream_info[0].logStreamName.value, ",safApp=safLogService");
			stream_info[0].logStreamName.length = strlen((char *) stream_info[0].logStreamName.value);
			if (optind < argc) {
				log_message = argv[optind];
			} else {
				log_message = NULL;
			}
			create_log_record(&stream_info[0].logRecord, SA_LOG_GENERIC_HEADER, &logSvcUsrName,
					&notificationClassId, log_message, &logBuffer[0]);
			stream_cnt = 1;
			break;
		case 's':
			logSeverity = get_severity(optarg);
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
	
	error = saLogInitialize(&logHandle, &logCallbacks, &logVersion);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "saLogInitialize FAILED: %u\n", error);
		exit(EXIT_FAILURE);
	}

	error = saLogSelectionObjectGet(logHandle, &selectionObject);
	if (error != SA_AIS_OK) {
		exit(EXIT_FAILURE);
	}

	int j;
	for (j=0; j < stream_cnt; j++) {
		if (stream_info[j].logRecord.logHdrType == SA_LOG_GENERIC_HEADER) {
			stream_info[j].logRecord.logHeader.genericHdr.logSeverity = logSeverity;
		}
		
		if (is_appl_stream) {
			logFileCreateAttributes_ptr = &logFileCreateAttributes[j];
		} else {
			logFileCreateAttributes_ptr = NULL;
		}

		error = saLogStreamOpen_2(
				logHandle,
				&stream_info[j].logStreamName,
				logFileCreateAttributes_ptr,
				logStreamOpenFlags,
				SA_TIME_ONE_SECOND,
				&logStreamHandle);
		if (error != SA_AIS_OK) {
			fprintf(stderr, "saLogStreamOpen_2 FAILED: %u\n", error);
			exit(EXIT_FAILURE);
		}
		free_logFileCreateAttributes(logFileCreateAttributes_ptr);

		if (stream_info[j].logRecord.logBuffer->logBuf != NULL) {
			for (i = 0; i < write_count; i++) {
				write_log_record(
						logHandle,
						logStreamHandle,
						selectionObject,
						&stream_info[j].logRecord,
						wait_for_ack);
				if (interval > 0)
					usleep(interval);
			}
		}
		
		free_loc_record(&stream_info[j].logRecord);
	} /* loop j < stream_cnt */

	/* Wait for key press (any key) before closing stream(s) and or finalize */
	if (do_not_exit_f == true) {
		printf("\nWaiting for any key to close stream(s) and exit...\n");
		(void) getchar();
		do_not_exit_f = false;
	}
	
	/* Exit without closing streams or finalize log API */
	if (no_finalize_exit == true) {
		printf("Exit witout stream close or finalize\n");
		exit(EXIT_SUCCESS);
	}

	/* If only one stream is open it's closed before Finalize
	 * If there are several open streams, just Finalize (streams will be closed
	 * automatically)
	 */
	if (stream_cnt == 1) {
		error = saLogStreamClose(logStreamHandle);
		if (SA_AIS_OK != error) {
			fprintf(stderr, "saLogStreamClose FAILED: %u\n", error);
			exit(EXIT_FAILURE);
		}
	}

	error = saLogFinalize(logHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saLogFinalize FAILED: %u\n", error);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

