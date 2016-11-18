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

#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <aio.h>
#include "log/saf/saLog.h"
#include <poll.h>

#include "logtest.h"

/**
 * Wait for log server asynchronous event and dispatch the callback
 * 
 * @param logHandle
 * @param selectionObject
 * 
 * @return wlc_rc_t
 *			
 */
typedef enum {
	WLC_OK,
	WLC_POLL_FAIL,
	WLC_POLL_TIMEOUT,
	WLC_DISPATCH_FAIL,
	WLC_INVOCATION_ERR,
	WLC_CB_ERR
} wlc_rc_t;

void saLogWriteLogAsync_01(void)
{
    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc == SA_AIS_OK)
        rc = logWriteAsync(&genLogRecord);
    logFinalize();

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_02(void)
{
    SaLogRecordT ntfLogRecord;
    SaNtfIdentifierT notificationId = random();
    SaNtfClassIdT notificationClassId;

    ntfLogRecord.logBuffer = &alarmStreamBuffer;
    strcpy((char*)ntfLogRecord.logBuffer->logBuf, __FUNCTION__);
    ntfLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);

    notificationClassId.vendorId = 193;
    notificationClassId.majorId  = 1;
    notificationClassId.minorId  = 2;
    ntfLogRecord.logTimeStamp = getSaTimeT();
    ntfLogRecord.logHdrType = SA_LOG_NTF_HEADER;
    ntfLogRecord.logHeader.ntfHdr.notificationId = notificationId;
    ntfLogRecord.logHeader.ntfHdr.eventType = SA_NTF_ALARM_QOS;
    ntfLogRecord.logHeader.ntfHdr.notificationObject = &notificationObject;
    ntfLogRecord.logHeader.ntfHdr.notifyingObject = &notifyingObject;
    ntfLogRecord.logHeader.ntfHdr.notificationClassId = &notificationClassId;
    ntfLogRecord.logHeader.ntfHdr.eventTime = getSaTimeT();

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&alarmStreamName);
    if (rc == SA_AIS_OK)
        rc = logWriteAsync(&ntfLogRecord);
    logFinalize();

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_03(void)
{
    SaLogRecordT ntfLogRecord;
    SaNtfIdentifierT notificationId = random();
    SaNtfClassIdT notificationClassId;

    ntfLogRecord.logBuffer = &notificationStreamBuffer;
    strcpy((char*)ntfLogRecord.logBuffer->logBuf, __FUNCTION__);
    ntfLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);

    notificationClassId.vendorId = 193;
    notificationClassId.majorId  = 1;
    notificationClassId.minorId  = 2;
    ntfLogRecord.logTimeStamp = getSaTimeT();
    ntfLogRecord.logHdrType = SA_LOG_NTF_HEADER;
    ntfLogRecord.logHeader.ntfHdr.notificationId = notificationId;
    ntfLogRecord.logHeader.ntfHdr.eventType = SA_NTF_ALARM_QOS;
    ntfLogRecord.logHeader.ntfHdr.notificationObject = &notificationObject;
    ntfLogRecord.logHeader.ntfHdr.notifyingObject = &notifyingObject;
    ntfLogRecord.logHeader.ntfHdr.notificationClassId = &notificationClassId;
    ntfLogRecord.logHeader.ntfHdr.eventTime = getSaTimeT();

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&notificationStreamName);
    if (rc == SA_AIS_OK)
        rc = logWriteAsync(&ntfLogRecord);
    logFinalize();

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_04(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }
    rc = saLogWriteLogAsync(0, invocation, 0, &genLogRecord);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

done:
    logFinalize();
}

void saLogWriteLogAsync_05(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }
    rc = saLogWriteLogAsync(-1, invocation, 0, &genLogRecord);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

done:
    logFinalize();
}

void saLogWriteLogAsync_06(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }
    rc = saLogWriteLogAsync(logStreamHandle, invocation, -1, &genLogRecord);
    test_validate(rc, SA_AIS_ERR_BAD_FLAGS);

done:
    logFinalize();
}

void saLogWriteLogAsync_07(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, NULL);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
    logFinalize();
}

void saLogWriteLogAsync_09(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    genLogRecord.logHeader.genericHdr.logSvcUsrName = NULL;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
    logFinalize();
}

void saLogWriteLogAsync_10(void)
{
    int ret;
    ret = setenv("SA_AMF_COMPONENT_NAME", "safComp=CompT_test_svc", 1);
    if (ret != 0) {
        test_validate(ret, 0);
        return;
    }
    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    genLogRecord.logHeader.genericHdr.logSvcUsrName = NULL;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc == SA_AIS_OK)
        rc = logWriteAsync(&genLogRecord);
    logFinalize();

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_11(void)
{
    struct timeval currentTime;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    /* Fetch current system time for time stamp value */
    (void)gettimeofday(&currentTime, 0);
    genLogRecord.logTimeStamp = 
       ((unsigned)currentTime.tv_sec * 1000000000ULL) + \
       ((unsigned)currentTime.tv_usec * 1000ULL);

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc == SA_AIS_OK)
        rc = logWriteAsync(&genLogRecord);
    logFinalize();

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_12(void)
{
    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc == SA_AIS_OK)
        rc = logWriteAsync(&genLogRecord);
    logFinalize();

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_13(void)
{
    SaLogFileCreateAttributesT_2 appStream1LogFileCreateAttributes = {
        .logFilePathName = DEFAULT_APP_FILE_PATH_NAME,
        .logFileName = (SaStringT) "ticket203",
        .maxLogFileSize = 4096,
        .maxLogRecordSize = 1024,
        .haProperty = SA_TRUE,
        .logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE,
        .maxFilesRotated = DEFAULT_MAX_FILE_ROTATED,
        .logFileFmt = DEFAULT_FORMAT_EXPRESSION
    };
    SaNameT appStreamName;
    SaConstStringT data = "safLgStr=ticket203";
    saAisNameLend(data, &appStreamName);

    memset(genLogRecord.logBuffer->logBuf, 'X', 1000);
    genLogRecord.logBuffer->logBufSize = 1000;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&appStreamName, &appStream1LogFileCreateAttributes);
    if (rc == SA_AIS_OK)
        rc = logWriteAsync(&genLogRecord);
    sleep(1);
    logFinalize();

    test_validate(rc, SA_AIS_OK);
}

/**
 * saLogWriteAsyncLog() invalid severity
 */
void saLogWriteLogAsync_14(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    genLogRecord.logHeader.genericHdr.logSeverity = 0xff;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
    logFinalize();
    genLogRecord.logHeader.genericHdr.logSeverity = SA_LOG_SEV_INFO;
}

/**
 * saLogWriteAsyncLog() - logBufSize > strlen(logBuf) + 1
 */
void saLogWriteLogAsync_18(void)
{
	SaInvocationT invocation = 0;

	strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
	genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__) + 2;

	rc = logInitialize();
	if (rc != SA_AIS_OK) {
		fprintf(stderr, " logInitialize failed: %d \n", (int)rc);
		test_validate(rc, SA_AIS_OK);
		return;
	}
	rc = logStreamOpen(&systemStreamName);
	if (rc != SA_AIS_OK) {
		test_validate(rc, SA_AIS_OK);
		goto done;
	}
	rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
	test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
	logFinalize();
}

/**
 * saLogWriteAsyncLog() - big logBufSize > SA_LOG_MAX_RECORD_SIZE
 */
void saLogWriteLogAsync_19(void)
{
	SaInvocationT invocation = 0;
	char logBuf[SA_LOG_MAX_RECORD_SIZE + 10];

	memset(logBuf, 'A', sizeof(logBuf));
	logBuf[sizeof(logBuf) - 1] = '\0';
	genLogRecord.logBuffer->logBuf = (SaUint8T *)&logBuf;
	genLogRecord.logBuffer->logBufSize = SA_LOG_MAX_RECORD_SIZE + 10;

	rc = logInitialize();
	if (rc != SA_AIS_OK) {
		fprintf(stderr, " logInitialize failed: %d \n", (int)rc);
		test_validate(rc, SA_AIS_OK);
		return;
	}
	rc = logStreamOpen(&systemStreamName);
	if (rc != SA_AIS_OK) {
		test_validate(rc, SA_AIS_OK);
		goto done;
	}
	rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
	test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
	logFinalize();
}
