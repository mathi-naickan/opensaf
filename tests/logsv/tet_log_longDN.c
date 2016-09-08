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

#include <unistd.h>
#include <saf_error.h>
#include <poll.h>
#include <stdbool.h>
#include "logtest.h"

#include "osaf_extended_name.h"
#include <saLog.h>
#include "saf_error.h"

/* Default format expression for alm/not streams */
static const char *fmt_alarm_default="@Cr @Ct @Nt @Ne6 @No30 @Ng30 \"@Cb\"";
/* Default format expression for sys/app streams */
static const char *fmt_sys_default="@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Sv @Sl \"@Cb\"";
/* Format expression for testing Long DN at @No */
static const char *test_alarm_fmt="@Cr @Ct @Nt @Ne6 START_@No_END \"@Cb\"";
/* Format expression for testing Long DN at @Ng */
static const char *test_notif_fmt="@Cr @Ct @Nt @Ne6 START_@Ng_END \"@Cb\"";
/* Format expression for testing Long DN at @Sl */
static const char *test_sys_fmt="@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Sv START_@Sl_END \"@Cb\"";
// @NOTE:
// The purpose for adding START_*_END in fmt expression
// is for searching/test verifying using regular expression.

#define ALM_LOG_FILE "saLogAlarmLongDN"
#define NTF_LOG_FILE "saLogNotificationLongDN"
#define SYS_LOG_FILE "saLogSystemLongDN"
#define MAX_DATA 3000

//>>
// Steps to test Long DN in each test case
// 1) Change global `logMaxLogrecsize` to max number `SA_LOG_MAX_RECORD_SIZE`
// 2) Change the `saLogStreamLogFileFormat`
// 3) Set up the logBuf containing long DN data/or `logSvcUsrName` with long DN (> 255 bytes)
// 4) Sending the record to log stream
// 5) Read the log file and verify the content
// 6) Restore to default settings
//<<

/* Vars holding default values - they have to change to test Long DN */
static uint32_t v_logMaxLogrecsize = 1024;
static char v_saLogStreamLogFileFormat[1024] = {0};
static uint32_t v_saLogStreamFixedLogRecordSize = 200;
static char v_saLogStreamFileName[256] = {0};
static uint32_t v_longDnsAllowed = 0;

typedef enum {
	E_ALARM,
	E_NOTIF,
	E_SYSTE,
	E_APPLI
} stream_type_t;

SaConstStringT s_opensafImm = "opensafImm=opensafImm,safApp=safImmService";
SaNameT sa_opensafImm;

//>>
// Global data for handling LOG API
//<<
static SaLogHandleT logHandleLd = -1;
static SaLogStreamHandleT logStreamHandleLd = -1;
static SaLogRecordT logRecordLd;
static SaSelectionObjectT selectionObjectLd = -1;
static SaLogFileCreateAttributesT_2 *logFileCreateAttributesLd = NULL;

static SaNameT logSvcUsrNameLd;
static SaNameT notificationObjLd;
static SaNameT notifyingObjLd;
static SaNameT logStreamNameLd;
static SaLogBufferT logBufferLd;
static SaNtfClassIdT notificationClassIdLd = { 1, 2, 3 };
static SaVersionT logVersionLd = { 'A', 2, 2 };
static SaInvocationT invocationLd;
static SaAisErrorT errorLd;


/* Try for 10 seconds before giving up on an API */
#define TEN_SECONDS 10*1000*1000
/* Sleep for 100 ms before retrying an API */
#define HUNDRED_MS 100*1000

SaConstStringT logSvcUsrNameDf = "SvcUserName_Test_LongDN";
SaConstStringT notifyingObjDf = "NotifyingObj_Test_LongDN";
SaConstStringT notificationObjDf = "NotificationObj_Test_LongDN";

static void logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error);
static SaLogCallbacksT logCallbacksLd = { 0, 0, logWriteLogCallbackT };

//>
// Following attributes are backup before performing testing
// logMaxLogrecsize;
// saLogStreamLogFileFormat
// saLogStreamFixedLogRecordSize;
// saLogStreamFileName;
// longDnsAllowed
//<
static int backupData(stream_type_t type)
{
	int rc;

	saAisNameLend(s_opensafImm, &sa_opensafImm);
	rc = get_attr_value(&sa_opensafImm, "longDnsAllowed", &v_longDnsAllowed);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute longDnsAllowed value from IMM\n");
	}
	rc = get_attr_value(&configurationObject, "logMaxLogrecsize", &v_logMaxLogrecsize);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute logMaxLogrecsize value from IMM\n");
	}

	switch (type) {
	case E_ALARM:
		/* Get and save saLogStreamLogFileFormat */
		rc = get_attr_value(&alarmStreamName, "saLogStreamLogFileFormat", v_saLogStreamLogFileFormat);
		if (rc == -1) {
			/* Failed, use default one */
			fprintf(stderr, "Failed to get saLogStreamLogFileFormat attribute value from IMM\n");
			strncpy(v_saLogStreamLogFileFormat, fmt_alarm_default, 500);
		}
		/* Get and save salogStreamFileName */
		rc = get_attr_value(&alarmStreamName, "saLogStreamFileName", v_saLogStreamFileName);
		if (rc == -1) {
			/* Failed, use default one */
			fprintf(stderr, "Failed to get saLogStreamFileName attribute value from IMM\n");
			strncpy(v_saLogStreamFileName, "saLogAlarm", 256);
		}
		/* Get and save saLogStreamFixedLogRecordSize */
		rc = get_attr_value(&alarmStreamName, "saLogStreamFixedLogRecordSize",
				    &v_saLogStreamFixedLogRecordSize);
		if (rc == -1) {
			/* Failed, use default one */
			fprintf(stderr, "Failed to get saLogStreamFixedLogRecordSize attribute value from IMM\n");
		}
		break;

	case E_NOTIF:
		/* Get and save saLogStreamLogFileFormat */
		rc = get_attr_value(&notificationStreamName, "saLogStreamLogFileFormat",
				    v_saLogStreamLogFileFormat);
		if (rc == -1) {
			/* Failed, use default one */
			fprintf(stderr, "Failed to get attribute value from IMM\n");
			strncpy(v_saLogStreamLogFileFormat, fmt_alarm_default, 256);
		}
		/* Get and save salogStreamFileName */
		rc = get_attr_value(&notificationStreamName, "saLogStreamFileName", v_saLogStreamFileName);
		if (rc == -1) {
			/* Failed, use default one */
			fprintf(stderr, "Failed to get saLogStreamFileName attribute value from IMM\n");
			strncpy(v_saLogStreamFileName, "saLogNotification", 256);
		}
		/* Get and save saLogStreamFixedLogRecordSize */
		rc = get_attr_value(&notificationStreamName, "saLogStreamFixedLogRecordSize",
				    &v_saLogStreamFixedLogRecordSize);
		if (rc == -1) {
			/* Failed, use default one */
			fprintf(stderr, "Failed to get saLogStreamFixedLogRecordSize attribute value from IMM\n");
		}
		break;

	case E_SYSTE:
		/* Get and save saLogStreamLogFileFormat */
		rc = get_attr_value(&systemStreamName, "saLogStreamLogFileFormat",
				    v_saLogStreamLogFileFormat);
		if (rc == -1) {
			/* Failed, use default one */
			fprintf(stderr, "Failed to get attribute value from IMM\n");
			strncpy(v_saLogStreamLogFileFormat, fmt_sys_default, 256);
		}
		/* Get and save salogStreamFileName */
		rc = get_attr_value(&systemStreamName, "saLogStreamFileName", v_saLogStreamFileName);
		if (rc == -1) {
			/* Failed, use default one */
			fprintf(stderr, "Failed to get saLogStreamFileName attribute value from IMM\n");
			strncpy(v_saLogStreamFileName, "saLogSystem", 256);
		}
		/* Get and save saLogStreamFixedLogRecordSize */
		rc = get_attr_value(&systemStreamName, "saLogStreamFixedLogRecordSize",
				    &v_saLogStreamFixedLogRecordSize);
		if (rc == -1) {
			/* Failed, use default one */
			fprintf(stderr, "Failed to get saLogStreamFixedLogRecordSize attribute value from IMM\n");
		}
		break;

	case E_APPLI:
		/* Do nothing here */
		break;

	default:
		fprintf(stderr, "Not supported!\n");
		rc = -1;
		break;
	}

	return rc;
}

//>
// Set up environments before performing testing
// 1) Tell IMM to enable long DN feature
// 2) Change max log record size (record contains long DN data could be huge)
// 3) Change log file format, so long DN can be saved fully in log record
// 4) Change log record size to variable record (not fix one)
//
// Return value must be 0, report test failed otherwise.
//<
static int setUpTestEnv(stream_type_t type)
{
	int rc;
	char command[MAX_DATA];

	/* Enable long DN feature */
	rc = system("immcfg -m -a longDnsAllowed=1 opensafImm=opensafImm,safApp=safImmService");
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to enable long DN \n");
		return -1;
	}
	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", SA_LOG_MAX_RECORD_SIZE);
	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to execute command %s\n", command);
		return -1;
	}

	switch (type) {
	case E_ALARM:
		sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"%s\" "
			" -a saLogStreamFixedLogRecordSize=0 %s",
			test_alarm_fmt, SA_LOG_STREAM_ALARM);
		break;

	case E_NOTIF:
		sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"%s\" "
			" -a saLogStreamFixedLogRecordSize=0 %s",
			test_notif_fmt, SA_LOG_STREAM_NOTIFICATION);
		break;

	case E_SYSTE:
		sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"%s\" "
			" -a saLogStreamFixedLogRecordSize=0 %s",
			test_sys_fmt, SA_LOG_STREAM_SYSTEM);
		break;

	case E_APPLI:
		/* Do nothing here */
		break;

	default:
		fprintf(stderr, "Not supported\n");
		return -1;
	}

	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to perform cmd = %s\n", command);
		return -1;
	}

	return 0;
}

void restoreData(stream_type_t type)
{
	int rc;
	char command[MAX_DATA];

	sprintf(command, "immcfg -a longDnsAllowed=%d %s", v_longDnsAllowed, s_opensafImm);
	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to perform cmd = %s\n", command);
	}

	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", v_logMaxLogrecsize);
	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to perform cmd = %s\n", command);
	}

	switch (type) {
	case E_ALARM:
		sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"%s\" "
			" -a saLogStreamFixedLogRecordSize=%d %s", v_saLogStreamLogFileFormat,
			v_saLogStreamFixedLogRecordSize, SA_LOG_STREAM_ALARM);
		break;

	case E_NOTIF:
		sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"%s\" "
			" -a saLogStreamFixedLogRecordSize=%d %s", v_saLogStreamLogFileFormat,
			v_saLogStreamFixedLogRecordSize, SA_LOG_STREAM_NOTIFICATION);
		break;

	case E_SYSTE:
		sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"%s\" "
			" -a saLogStreamFixedLogRecordSize=%d %s", v_saLogStreamLogFileFormat,
			v_saLogStreamFixedLogRecordSize, SA_LOG_STREAM_SYSTEM);
		break;

	case E_APPLI:
		/* Do nothing here */
		break;

	default:
		fprintf(stderr, "Not supported\n");
		break;
	}

	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "failed to perform cmd = %s\n", command);
	}

}

//
// Functions
//

static void logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
{
	invocationLd = invocation;
	errorLd = error;
}

static SaTimeT currentTime(void)
{
	struct timeval tv;
	SaTimeT ntfTime;
	gettimeofday(&tv, 0);
	ntfTime = ((unsigned)tv.tv_sec * 1000000000ULL) +
		((unsigned)tv.tv_usec * 1000ULL);
	return ntfTime;
}

static SaAisErrorT initLog(void)
{
	SaAisErrorT error;
	unsigned int wait_time = 0;

	error = saLogInitialize(&logHandleLd, &logCallbacksLd, &logVersionLd);
	while (error == SA_AIS_ERR_TRY_AGAIN && wait_time < TEN_SECONDS) {
		usleep(HUNDRED_MS);
		wait_time += HUNDRED_MS;
		error = saLogInitialize(&logHandleLd, &logCallbacksLd, &logVersionLd);
	}

	return error;
}

static SaAisErrorT openLog(stream_type_t type)
{
	SaAisErrorT error;
	unsigned int wait_time = 0;
	SaLogStreamOpenFlagsT logStreamOpenFlagsIn = 0;
	SaLogFileCreateAttributesT_2 appLogFileCreateAttributesIn;

	// Set up default data
	saAisNameLend(logSvcUsrNameDf, &logSvcUsrNameLd);
	saAisNameLend(notifyingObjDf, &notifyingObjLd);
	saAisNameLend(notificationObjDf, &notificationObjLd);


	saAisNameLend(SA_LOG_STREAM_SYSTEM, &logStreamNameLd);
	logRecordLd.logTimeStamp = SA_TIME_UNKNOWN;	/* LOG service should supply timestamp */
	logRecordLd.logHdrType = SA_LOG_GENERIC_HEADER;
	logRecordLd.logHeader.genericHdr.notificationClassId = NULL;
	logRecordLd.logHeader.genericHdr.logSeverity = SA_LOG_SEV_INFO;
	logRecordLd.logHeader.genericHdr.logSvcUsrName = &logSvcUsrNameLd;
	logRecordLd.logBuffer = NULL;

	/* Initial data for E_APPLI stream type */
	char appStreamDN[1000] = {0};

	switch (type) {
	case E_ALARM:
		saAisNameLend(SA_LOG_STREAM_ALARM, &logStreamNameLd);
		logRecordLd.logHdrType = SA_LOG_NTF_HEADER;
		break;

	case E_NOTIF:
		saAisNameLend(SA_LOG_STREAM_NOTIFICATION, &logStreamNameLd);
		logRecordLd.logHdrType = SA_LOG_NTF_HEADER;
		break;

	case E_SYSTE:
		/* Do nothing here */
		break;

	case E_APPLI:
		appLogFileCreateAttributesIn.logFilePathName = "testLongDN";
		appLogFileCreateAttributesIn.maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
		appLogFileCreateAttributesIn.maxLogRecordSize = DEFAULT_APP_LOG_REC_SIZE;
		appLogFileCreateAttributesIn.haProperty = SA_TRUE;
		appLogFileCreateAttributesIn.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
		appLogFileCreateAttributesIn.maxFilesRotated = 4;

		char tmp [1000] = {0};
		memset(tmp, 'L', sizeof(tmp) - 1);
		snprintf(appStreamDN, sizeof(appStreamDN) - 2, "safLgStr=%s", tmp);

		/* Use built-in log file format in log server for app stream */
		appLogFileCreateAttributesIn.logFileFmt = NULL;
		appLogFileCreateAttributesIn.logFileName = "longDNAppStream";

		logFileCreateAttributesLd = &appLogFileCreateAttributesIn;
		logStreamOpenFlagsIn = SA_LOG_STREAM_CREATE;
		saAisNameLend(appStreamDN, &logStreamNameLd);

		break;

	default:
		fprintf(stderr, "Not supported \n");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (logRecordLd.logHdrType == SA_LOG_NTF_HEADER) {
		/* Setup some valid values */
		logRecordLd.logHeader.ntfHdr.notificationId = SA_NTF_IDENTIFIER_UNUSED;
		logRecordLd.logHeader.ntfHdr.eventType = SA_NTF_ALARM_PROCESSING;
		logRecordLd.logHeader.ntfHdr.notificationObject = &notificationObjLd;
		logRecordLd.logHeader.ntfHdr.notifyingObject = &notifyingObjLd;
		logRecordLd.logHeader.ntfHdr.notificationClassId = &notificationClassIdLd;
		logRecordLd.logHeader.ntfHdr.eventTime = currentTime();
	}

	error = saLogStreamOpen_2(logHandleLd, &logStreamNameLd, NULL, 0,
				  SA_TIME_ONE_SECOND, &logStreamHandleLd);
	while (error == SA_AIS_ERR_TRY_AGAIN && wait_time < TEN_SECONDS) {
		usleep(HUNDRED_MS);
		wait_time += HUNDRED_MS;
		error = saLogStreamOpen_2(logHandleLd, &logStreamNameLd, NULL, 0,
					  SA_TIME_ONE_SECOND, &logStreamHandleLd);
	}

	if (error == SA_AIS_ERR_NOT_EXIST && type == E_APPLI) {
		wait_time = 0;
		error = saLogStreamOpen_2(logHandleLd, &logStreamNameLd, logFileCreateAttributesLd,
					  logStreamOpenFlagsIn, SA_TIME_ONE_SECOND, &logStreamHandleLd);
		while (error == SA_AIS_ERR_TRY_AGAIN && wait_time < TEN_SECONDS) {
			usleep(HUNDRED_MS);
			wait_time += HUNDRED_MS;
			error = saLogStreamOpen_2(logHandleLd, &logStreamNameLd, logFileCreateAttributesLd,
						  logStreamOpenFlagsIn, SA_TIME_ONE_SECOND, &logStreamHandleLd);
		}
	}

	return error;
}

static SaAisErrorT endLog(void)
{
	SaAisErrorT error;
	unsigned int wait_time = 0;

	wait_time = 0;
	error = saLogStreamClose(logStreamHandleLd);
	while (error == SA_AIS_ERR_TRY_AGAIN && wait_time < TEN_SECONDS) {
		usleep(HUNDRED_MS);
		wait_time += HUNDRED_MS;
		error = saLogStreamClose(logStreamHandleLd);
	}
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saLogStreamClose FAILED: %s\n", saf_error(error));
	}

	wait_time = 0;
	error = saLogFinalize(logHandleLd);
	while (error == SA_AIS_ERR_TRY_AGAIN && wait_time < TEN_SECONDS) {
		usleep(HUNDRED_MS);
		wait_time += HUNDRED_MS;
		error = saLogFinalize(logHandleLd);
	}
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saLogFinalize FAILED: %s\n", saf_error(error));
	}

	/* Reset data */
	logStreamHandleLd = -1;
	logHandleLd = -1;
	selectionObjectLd = -1;

	return error;

}

static SaAisErrorT writeLog(void)
{
	SaAisErrorT error;
	unsigned int wait_time = 0;
	SaInvocationT invocation;
	int sz;
	struct pollfd fds[1];
	const char *logBuf = "test long DN";
	int ret;

	sz = strlen(logBuf);
	logBufferLd.logBufSize = sz;
	logBufferLd.logBuf = (SaUint8T *)logBuf;
	logRecordLd.logBuffer = &logBufferLd;
	invocation = random();

retry:
	error = saLogWriteLogAsync(logStreamHandleLd, invocation, SA_LOG_RECORD_WRITE_ACK, &logRecordLd);
	while (error == SA_AIS_ERR_TRY_AGAIN && wait_time < TEN_SECONDS) {
		usleep(HUNDRED_MS);
		wait_time += HUNDRED_MS;
		goto retry;
	}

	if (error != SA_AIS_OK) {
		return error;
	}

	fds[0].fd = (int)selectionObjectLd;
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
		fprintf(stderr, "poll timeout\n");
		return SA_AIS_ERR_BAD_OPERATION;
	}

	error = saLogDispatch(logHandleLd, SA_DISPATCH_ONE);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "saLogDispatch FAILED: %s\n", saf_error(error));
		return error;
	}

	if (invocationLd != invocation) {
		fprintf(stderr, "logWriteLogCallbackT FAILED: wrong invocation\n");
		return SA_AIS_ERR_BAD_OPERATION;
	}
	if (errorLd == SA_AIS_ERR_TRY_AGAIN && wait_time < TEN_SECONDS) {
		usleep(HUNDRED_MS);
		wait_time += HUNDRED_MS;
		goto retry;
	}
	if (errorLd == SA_AIS_ERR_TIMEOUT) {
		usleep(HUNDRED_MS);
		fprintf(stderr, "got SA_AIS_ERR_TIMEOUT, retry\n");
		goto retry;
	}
	if (errorLd != SA_AIS_OK) {
		fprintf(stderr, "logWriteLogCallbackT FAILED: %s\n", saf_error(errorLd));
		return errorLd;
	}

	return error;
}

//>
// Verify the log file content to see if the long DN exists at specific token or not
//
// 1) Find all log files that have changes within last 1 minute
// 2) Take all log files which match the pattern
// 3) Search in file to see the long DN exist
// 4) Return 0 if found, otherwise -1
//
// @param type stream type
// @ret 0 if found long DN in log record, 1 otherwise
//
// NOTES:
// 1) If environment variable `LOGTEST_ENABLE_STDOUT` set, user will set
//    the log record that contains the long DN.
// 2) In step #1, if the timer is different b/w active and other nodes,
//    running test on other node might get failed at verifyData step.
//<
static int verifyData(void)
{
	int rc = 0;
	char command[500];
	bool disable_stdout = true;
	SaConstStringT s_stdout = "1> /dev/null";

	if (getenv("LOGTEST_ENABLE_STDOUT")) disable_stdout = false;

	sprintf(command, "find %s -type f -mmin -1 | egrep \"%s_[0-9]{8}_[0-9]{6}\\.log$\" "
		" | xargs egrep \" START.*END \" %s ",
		log_root_path, v_saLogStreamFileName, disable_stdout ? s_stdout : " ");
	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "failed to perform cmd = %s\n", command);
		rc = 1;
	}

	return rc;
}

//>>
// UC1: Write an log records using the long DN in @No tokens using Log API.
//<<
void longDNAt_No_token(void)
{
	int rc;
	SaAisErrorT ais;

	rc = backupData(E_ALARM);
	if (rc != 0) {
		fprintf(stderr, "Backup data failed\n");
		// Not report test failed as can use default data to restore.
		// rc_validate(WEXITSTATUS(rc), 0);
		// return;
	}

	rc = setUpTestEnv(E_ALARM);
	if (rc != 0) {
		fprintf(stderr, "set up env failed\n");
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	ais = initLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "initLog FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done;
	}

	ais = openLog(E_ALARM);
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "openLog FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	ais = saLogSelectionObjectGet(logHandleLd, &selectionObjectLd);
	if (ais != SA_AIS_OK) {
		printf("saLogSelectionObjectGet FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	// Long DN in @No token
	char notificationObj[1000] = {0};
	memset(notificationObj, 'A', sizeof(notificationObj) - 1);
	saAisNameLend(notificationObj, &notificationObjLd);

	ais = writeLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogOpen FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	// Verify if log file contains long DN record or not
	rc = verifyData();
	rc_validate(rc, 0);

	// End testing. Close handles and restore data
done_init:
	endLog();
done:
	restoreData(E_ALARM);
}

//>>
// UC2: Write an log records using the long DN in @Ng tokens using Log API.
//<<
void longDNAt_Ng_token(void)
{
	int rc;
	SaAisErrorT ais;

	rc = backupData(E_NOTIF);
	if (rc != 0) {
		fprintf(stderr, "Backup data failed\n");
		// Not report test failed as can use default data to restore.
		// rc_validate(WEXITSTATUS(rc), 0);
		// return;
	}

	rc = setUpTestEnv(E_NOTIF);
	if (rc != 0) {
		fprintf(stderr, "set up env failed\n");
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	ais = initLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogInitialize FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done;
	}

	ais = openLog(E_NOTIF);
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogOpen FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	ais = saLogSelectionObjectGet(logHandleLd, &selectionObjectLd);
	if (ais != SA_AIS_OK) {
		printf("saLogSelectionObjectGet FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	// Long DN in @No token
	char notifyingObj[1000] = {0};
	memset(notifyingObj, 'B', sizeof(notifyingObj) - 1);
	saAisNameLend(notifyingObj, &notifyingObjLd);

	ais = writeLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogOpen FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	// Verify if log file contains long DN record or not
	rc = verifyData();
	rc_validate(rc, 0);

	// End testing. Close handles and restore data
done_init:
	endLog();
done:
	restoreData(E_NOTIF);
}

//>>
// UC3: Write an log records using the long DN in @Sl tokens using Log API.
//<<
void longDNAt_Sl_token(void)
{
	int rc;
	SaAisErrorT ais;

	rc = backupData(E_SYSTE);
	if (rc != 0) {
		fprintf(stderr, "Backup data failed\n");
		// Not report test failed as can use default data to restore.
		// rc_validate(WEXITSTATUS(rc), 0);
		// return;
	}

	rc = setUpTestEnv(E_SYSTE);
	if (rc != 0) {
		fprintf(stderr, "set up env failed\n");
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	ais = initLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogInitialize FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done;
	}

	ais = openLog(E_SYSTE);
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogOpen FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	ais = saLogSelectionObjectGet(logHandleLd, &selectionObjectLd);
	if (ais != SA_AIS_OK) {
		printf("saLogSelectionObjectGet FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	// Long DN in @Sl token
	char serviceName[1000] = {0};
	memset(serviceName, 'C', sizeof(serviceName) - 1);
	saAisNameLend(serviceName, &logSvcUsrNameLd);

	ais = writeLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogOpen FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	// Verify if log file contains long DN record or not
	rc = verifyData();
	rc_validate(rc, 0);

	// End testing. Close handles and restore data
done_init:
	endLog();
done:
	restoreData(E_SYSTE);
}

//>>
// UC4: Write an log records to long DN app stream using Log API.
//<<
void longDN_AppStream(void)
{
	int rc;
	SaAisErrorT ais;

	rc = backupData(E_APPLI);
	if (rc != 0) {
		fprintf(stderr, "Backup data failed\n");
		// Not report test failed as can use default data to restore.
		// rc_validate(WEXITSTATUS(rc), 0);
		// return;
	}

	rc = setUpTestEnv(E_APPLI);
	if (rc != 0) {
		fprintf(stderr, "set up env failed\n");
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	ais = initLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogInitialize FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done;
	}

	ais = openLog(E_APPLI);
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogOpen FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	ais = saLogSelectionObjectGet(logHandleLd, &selectionObjectLd);
	if (ais != SA_AIS_OK) {
		printf("saLogSelectionObjectGet FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	ais = writeLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogOpen FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	rc_validate(rc, 0);

	// End testing. Close handles and restore data
done_init:
	endLog();
done:
	restoreData(E_APPLI);
}

//>>
// UC5: Write a log record to long DN runtime app stream using saflogger
//<<
void longDNIn_AppStreamDN(void)
{
	int rc;
	char command[3000];

	// Backup default setting
	saAisNameLend(s_opensafImm, &sa_opensafImm);
	rc = get_attr_value(&sa_opensafImm, "longDnsAllowed", &v_longDnsAllowed);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute longDnsAllowed value from IMM\n");
	}

	/* Enable long DN feature */
	rc = system("immcfg -m -a longDnsAllowed=1 opensafImm=opensafImm,safApp=safImmService");
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to enable long DN \n");
		rc_validate(WEXITSTATUS(rc), 0);
		return;
	}

	// Preparing data
	char appStreamDN[1000] = {0};
	memset(appStreamDN, 'D', sizeof(appStreamDN) - 1);

	// Perform testing
	sprintf(command, "saflogger -a safLgStr=%s -f longDN longDN_test", appStreamDN);
	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to perform cmd = %s\n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	rc_validate(WEXITSTATUS(rc), 0);
done:
	// Restore data
	sprintf(command, "immcfg -a longDnsAllowed=%d %s", v_longDnsAllowed, s_opensafImm);
	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to perform cmd = %s\n", command);
	}

}

//>
// ABNORMAL USE CASES
//<

//>>
// A-UC1: Write an log records with `notificationObject` over kOsafMaxDnLength (2048)
// using Log API.
//<<
void longDN_No_Over_MaxDn(void)
{
	int rc;
	SaAisErrorT ais;

	rc = backupData(E_ALARM);
	if (rc != 0) {
		fprintf(stderr, "Backup data failed\n");
		// Not report test failed as can use default data to restore.
		// rc_validate(WEXITSTATUS(rc), 0);
		// return;
	}

	rc = setUpTestEnv(E_ALARM);
	if (rc != 0) {
		fprintf(stderr, "set up env failed\n");
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	ais = initLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "initLog FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done;
	}

	ais = openLog(E_ALARM);
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "openLog FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	ais = saLogSelectionObjectGet(logHandleLd, &selectionObjectLd);
	if (ais != SA_AIS_OK) {
		printf("saLogSelectionObjectGet FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	// Long DN in @No token
	char notificationObj[kOsafMaxDnLength + 1] = {0};
	memset(notificationObj, 'X', sizeof(notificationObj) - 1);
	saAisNameLend(notificationObj, &notificationObjLd);

	ais = writeLog();
	rc_validate(ais, SA_AIS_ERR_INVALID_PARAM);

	// End testing. Close handles and restore data
done_init:
	endLog();
done:
	restoreData(E_ALARM);
}

//>>
// A-UC3: Write an log records with `notifyingObject` over kOsafMaxDnLength (2048)
// using Log API.
//<<
void longDN_Ng_Over_MaxDn(void)
{
	int rc;
	SaAisErrorT ais;

	rc = backupData(E_NOTIF);
	if (rc != 0) {
		fprintf(stderr, "Backup data failed\n");
		// Not report test failed as can use default data to restore.
		// rc_validate(WEXITSTATUS(rc), 0);
		// return;
	}

	rc = setUpTestEnv(E_NOTIF);
	if (rc != 0) {
		fprintf(stderr, "set up env failed\n");
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	ais = initLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogInitialize FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done;
	}

	ais = openLog(E_NOTIF);
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogOpen FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	ais = saLogSelectionObjectGet(logHandleLd, &selectionObjectLd);
	if (ais != SA_AIS_OK) {
		printf("saLogSelectionObjectGet FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	// Long DN in @No token
	char notifyingObj[kOsafMaxDnLength + 1] = {0};
	memset(notifyingObj, 'Y', sizeof(notifyingObj) - 1);
	saAisNameLend(notifyingObj, &notifyingObjLd);

	ais = writeLog();
	rc_validate(ais, SA_AIS_ERR_INVALID_PARAM);

	// End testing. Close handles and restore data
done_init:
	endLog();
done:
	restoreData(E_NOTIF);
}

//>>
// A-UC3: Write an log records with `logSvcUsrName` over kOsafMaxDnLength (2048)
// using Log API.
//<<
void longDN_Sl_Over_MaxDn(void)
{
	int rc;
	SaAisErrorT ais;

	rc = backupData(E_SYSTE);
	if (rc != 0) {
		fprintf(stderr, "Backup data failed\n");
		// Not report test failed as can use default data to restore.
		// rc_validate(WEXITSTATUS(rc), 0);
		// return;
	}

	rc = setUpTestEnv(E_SYSTE);
	if (rc != 0) {
		fprintf(stderr, "set up env failed\n");
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	ais = initLog();
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogInitialize FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done;
	}

	ais = openLog(E_SYSTE);
	if (ais != SA_AIS_OK) {
		fprintf(stderr, "saLogOpen FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	ais = saLogSelectionObjectGet(logHandleLd, &selectionObjectLd);
	if (ais != SA_AIS_OK) {
		printf("saLogSelectionObjectGet FAILED: %s\n", saf_error(ais));
		rc_validate(ais, SA_AIS_OK);
		goto done_init;
	}

	// Long DN in @Sl token
	char serviceName[kOsafMaxDnLength + 1] = {0};
	memset(serviceName, 'Z', sizeof(serviceName) - 1);
	saAisNameLend(serviceName, &logSvcUsrNameLd);

	ais = writeLog();
	rc_validate(ais, SA_AIS_ERR_INVALID_PARAM);

	// End testing. Close handles and restore data
done_init:
	endLog();
done:
	restoreData(E_SYSTE);
}

//>>
// UC4: Write a log record to long DN runtime app stream using saflogger
//<<
void longDN_AppStrDN_Over_MaxDn(void)
{
	int rc;
	char command[kOsafMaxDnLength + 100];

	// Backup default setting
	saAisNameLend(s_opensafImm, &sa_opensafImm);
	rc = get_attr_value(&sa_opensafImm, "longDnsAllowed", &v_longDnsAllowed);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute longDnsAllowed value from IMM\n");
	}

	/* Enable long DN feature */
	rc = system("immcfg -m -a longDnsAllowed=1 opensafImm=opensafImm,safApp=safImmService");
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to enable long DN \n");
		rc_validate(WEXITSTATUS(rc), 0);
		return;
	}

	// Preparing data
	char appStreamDN[kOsafMaxDnLength + 1] = {0};
	memset(appStreamDN, 'x', sizeof(appStreamDN) - 1);

	// Perform testing
	sprintf(command, "saflogger -a safLgStrCfg=%s -f longDN_overMax longDN_test 2> /dev/null", appStreamDN);
	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		rc_validate(0, 0);
		goto done;
	}

	rc_validate(1, 0);
done:
	// Restore data
	sprintf(command, "immcfg -a longDnsAllowed=%d %s", v_longDnsAllowed, s_opensafImm);
	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to perform cmd = %s\n", command);
	}

}

//>>
// A-UC4: Write a log record to long DN runtime app stream using saflogger,
// but not using f option (ER)
//<<
void longDNIn_AppStreamDN_ButNoF(void)
{
	int rc;
	char command[3000];

	// Backup default setting
	saAisNameLend(s_opensafImm, &sa_opensafImm);
	rc = get_attr_value(&sa_opensafImm, "longDnsAllowed", &v_longDnsAllowed);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute longDnsAllowed value from IMM\n");
	}

	/* Enable long DN feature */
	rc = system("immcfg -m -a longDnsAllowed=1 opensafImm=opensafImm,safApp=safImmService");
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to enable long DN \n");
		rc_validate(WEXITSTATUS(rc), 0);
		return;
	}

	// Preparing data
	char appStreamDN[1000] = {0};
	memset(appStreamDN, 'E', sizeof(appStreamDN) - 1);

	// Perform testing
	sprintf(command, "saflogger -a safLgStrCfg=%s longDN_test_no_f 2> /dev/null", appStreamDN);
	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		rc_validate(0, 0);
		goto done;
	}

	rc_validate(WEXITSTATUS(rc), 0);
done:
	// Restore data
	sprintf(command, "immcfg -a longDnsAllowed=%d %s", v_longDnsAllowed, s_opensafImm);
	rc = system(command);
	if (WEXITSTATUS(rc) != 0) {
		fprintf(stderr, "Failed to perform cmd = %s\n", command);
	}

}

/*
 * Suite #13
 */
__attribute__ ((constructor)) static void longDN_constructor(void)
{
	test_suite_add(13, "Test Long DN support");
	// Normal test cases
	test_case_add(13, longDNAt_No_token, "Write a log record using the long DN in @No");
	test_case_add(13, longDNAt_Ng_token, "Write a log record using the long DN in @Ng");
	test_case_add(13, longDNAt_Sl_token, "Write a log record using the long DN in @Sl");
	test_case_add(13, longDN_AppStream, "Write a log record to long DN app stream");
	test_case_add(13, longDNIn_AppStreamDN, "Write a log record to long DN runtime app stream using saflogger tool");
	// Abnormal test cases
	test_case_add(13, longDN_No_Over_MaxDn, "Write a log record with notificationObj (@No) over max length");
	test_case_add(13, longDN_Ng_Over_MaxDn, "Write a log record with notifyingObj (@Ng) over max length");
	test_case_add(13, longDN_Sl_Over_MaxDn, "Write a log record with logSvcUsrName (@Sl) over max length");
	test_case_add(13, longDN_AppStrDN_Over_MaxDn, "Write a log record to app stream with DN over max using saflogger tool");
	test_case_add(13, longDNIn_AppStreamDN_ButNoF, "Write a log record to long DN runtime app using saflogger, but no f option");


}
