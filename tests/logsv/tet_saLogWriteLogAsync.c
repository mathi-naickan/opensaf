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
#include <saLog.h>
#include <poll.h>

#include "logtest.h"

static void salogWriteLogCallback(SaInvocationT invocation, SaAisErrorT error);
static SaLogCallbacksT logCallbacks_on = { 0, 0, salogWriteLogCallback };
static SaInvocationT cb_invocation;
static SaAisErrorT cb_error;

static void salogWriteLogCallback(SaInvocationT invocation, SaAisErrorT error)
{
	cb_invocation = invocation;
	cb_error = error;
	printf("salogWriteLogCallback invoked\n");
}

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

static wlc_rc_t wait_log_callback(SaLogHandleT logHandle,
		SaSelectionObjectT selectionObject,
		SaInvocationT invocation)
{
	struct pollfd fds[1];
	int rc = WLC_OK;
	SaAisErrorT errorCode;

	printf("Waiting for log write callback\n");
	
	fds[0].fd = (int) selectionObject;
	fds[0].events = POLLIN;
	
	while ((rc = poll(fds, 1, 20000)) == EINTR);
	
	if (rc == -1) {
		printf("Write callback poll FAILED: %s\n", strerror(errno));
		return WLC_POLL_FAIL;
	}

	if (rc == 0) {
		printf("Poll timeout\n");
		return WLC_POLL_TIMEOUT;
	}

	errorCode = saLogDispatch(logHandle, SA_DISPATCH_ONE);
	if (errorCode != SA_AIS_OK) {
		printf("saLogDispatch FAILED: %u\n", errorCode);
		return WLC_DISPATCH_FAIL;
	}
	
	/* Verify the callback info */
	rc = 0;
	if (cb_invocation != invocation) {
		printf("Write callback invocation error\n");
		rc = WLC_INVOCATION_ERR;
	}
	if (cb_error != SA_AIS_OK) {
		printf("Write callback error response: %d", cb_error);
		rc = WLC_CB_ERR;
	}
	return rc;
}

void saLogWriteLogAsync_01(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_02(void)
{
    SaInvocationT invocation = 0;
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

    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &alarmStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &ntfLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_03(void)
{
    SaInvocationT invocation = 0;
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

    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &notificationStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &ntfLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_04(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(0, invocation, 0, &genLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saLogWriteLogAsync_05(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(-1, invocation, 0, &genLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saLogWriteLogAsync_06(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, -1, &genLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_FLAGS);
}

void saLogWriteLogAsync_07(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, NULL);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saLogWriteLogAsync_09(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    genLogRecord.logHeader.genericHdr.logSvcUsrName = NULL;
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saLogWriteLogAsync_10(void)
{
    SaInvocationT invocation = 0;

    assert(setenv("SA_AMF_COMPONENT_NAME", "safComp=CompT_test_svc", 1) == 0);
    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    genLogRecord.logHeader.genericHdr.logSvcUsrName = NULL;
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}
void saLogWriteLogAsync_11(void)
{
    struct timeval currentTime;
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    /* Fetch current system time for time stamp value */
    (void)gettimeofday(&currentTime, 0);
    genLogRecord.logTimeStamp = 
       ((unsigned)currentTime.tv_sec * 1000000000ULL) + \
       ((unsigned)currentTime.tv_usec * 1000ULL);

    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_12(void)
{
    SaInvocationT invocation = 0;

    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

void saLogWriteLogAsync_13(void)
{
    SaLogFileCreateAttributesT_2 appStream1LogFileCreateAttributes = {
        .logFilePathName = DEFAULT_APP_FILE_PATH_NAME,
        .logFileName = (SaStringT) "ticket203",
        .maxLogFileSize = 4096,
        .maxLogRecordSize = 2048,
        .haProperty = SA_TRUE,
        .logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE,
        .maxFilesRotated = DEFAULT_MAX_FILE_ROTATED,
        .logFileFmt = DEFAULT_FORMAT_EXPRESSION
    };
    SaInvocationT invocation = 0;
    SaNameT appStreamName = {
        .value = "safLgStr=ticket203",
        .length = 0
    };
	appStreamName.length = strlen((char *) appStreamName.value);

    memset(genLogRecord.logBuffer->logBuf, 'X', 1800);
    genLogRecord.logBuffer->logBufSize = 1800;
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &appStreamName, &appStream1LogFileCreateAttributes,
                             SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
    sleep(1);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

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
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    genLogRecord.logHeader.genericHdr.logSeverity = SA_LOG_SEV_INFO;
}

/**
 * Check notificationObject length == 256
 * NTF Header
 */
void saLogWriteLogAsync_15(void)
{
    SaInvocationT invocation = random();
    SaLogRecordT ntfLogRecord;
    SaNtfIdentifierT notificationId = random();
    SaNtfClassIdT notificationClassId;
	SaAisErrorT rc1 = SA_AIS_OK;
	wlc_rc_t rc2 = WLC_OK;

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
    ntfLogRecord.logHeader.ntfHdr.notificationObject = &saNameT_Object_256;
    ntfLogRecord.logHeader.ntfHdr.notifyingObject = &notifyingObject;
    ntfLogRecord.logHeader.ntfHdr.notificationClassId = &notificationClassId;
    ntfLogRecord.logHeader.ntfHdr.eventTime = getSaTimeT();

    safassert(saLogInitialize(&logHandle, &logCallbacks_on, &logVersion), SA_AIS_OK);
	safassert(saLogSelectionObjectGet(logHandle, &selectionObject), SA_AIS_OK);
	
    safassert(saLogStreamOpen_2(logHandle, &alarmStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc1 = saLogWriteLogAsync(logStreamHandle, invocation, SA_LOG_RECORD_WRITE_ACK,
			&ntfLogRecord);
	if (rc1 == SA_AIS_OK) {
		rc2 = wait_log_callback(logHandle, selectionObject, invocation);
	}
	
    safassert(saLogFinalize(logHandle), SA_AIS_OK);
	
	/* If tested length is >= 256 the server will stop decoding write message
	 * and the message is ignored. This will result in no callback message from
	 * the server and waiting for callback will timeout.
	 * Note that normally the API is checking length. But if testing with this
	 * check disabled the server shall not crash.
	 */
	if (rc2 != WLC_OK) {
		test_validate(rc2, WLC_POLL_TIMEOUT);
	} else {
		test_validate(rc1, SA_AIS_ERR_INVALID_PARAM);
	}
}

/**
 * Check notifyingObject length == 256
 * NTF Header
 */
void saLogWriteLogAsync_16(void)
{
    SaInvocationT invocation = random();
    SaLogRecordT ntfLogRecord;
    SaNtfIdentifierT notificationId = random();
    SaNtfClassIdT notificationClassId;
	SaAisErrorT rc1 = SA_AIS_OK;
	wlc_rc_t rc2 = WLC_OK;

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
    ntfLogRecord.logHeader.ntfHdr.notifyingObject = &saNameT_Object_256;
    ntfLogRecord.logHeader.ntfHdr.notificationClassId = &notificationClassId;
    ntfLogRecord.logHeader.ntfHdr.eventTime = getSaTimeT();

    safassert(saLogInitialize(&logHandle, &logCallbacks_on, &logVersion), SA_AIS_OK);
	safassert(saLogSelectionObjectGet(logHandle, &selectionObject), SA_AIS_OK);
	
    safassert(saLogStreamOpen_2(logHandle, &alarmStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc1 = saLogWriteLogAsync(logStreamHandle, invocation, 0, &ntfLogRecord);
	
	if (rc1 == SA_AIS_OK) {
		rc2 = wait_log_callback(logHandle, selectionObject, invocation);
	}
	
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

	/* If tested length is >= 256 the server will stop decoding write message
	 * and the message is ignored. This will result in no callback message from
	 * the server and waiting for callback will timeout.
	 * Note that normally the API is checking length. But if testing with this
	 * check disabled the server shall not crash.
	 */
	if (rc2 != WLC_OK) {
		test_validate(rc2, WLC_POLL_TIMEOUT);
	} else {
		test_validate(rc1, SA_AIS_ERR_INVALID_PARAM);
	}
}

/**
 * saLogWriteLogAsync() Generic header logSvcUsrName length = 256
 */
void saLogWriteLogAsync_17(void)
{
    SaInvocationT invocation = random();
	SaLogRecordT genLogRecord_is;
	SaLogBufferT log_buffer;
	char record[] ="saLogWriteLogAsync_17";
	SaAisErrorT rc1 = SA_AIS_OK;
	wlc_rc_t rc2 = WLC_OK;

	log_buffer.logBuf = (SaUint8T *) record;
	log_buffer.logBufSize = strlen(record) + 1;
    genLogRecord_is.logBuffer = &log_buffer;
	genLogRecord_is.logHdrType = SA_LOG_GENERIC_HEADER;
	genLogRecord_is.logHeader.genericHdr.notificationClassId = NULL;
    genLogRecord_is.logHeader.genericHdr.logSeverity = SA_LOG_SEV_INFO;
	genLogRecord_is.logHeader.genericHdr.logSvcUsrName = &saNameT_appstream_name_256;
	
    safassert(saLogInitialize(&logHandle, &logCallbacks_on, &logVersion), SA_AIS_OK);
	safassert(saLogSelectionObjectGet(logHandle, &selectionObject), SA_AIS_OK);
	
    safassert(saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                             SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    rc1 = saLogWriteLogAsync(logStreamHandle, invocation, 0, &genLogRecord_is);
	
	if (rc1 == SA_AIS_OK) {
		rc2 = wait_log_callback(logHandle, selectionObject, invocation);
	}
	
    safassert(saLogFinalize(logHandle), SA_AIS_OK);

	/* If tested length is >= 256 the server will stop decoding write message
	 * and the message is ignored. This will result in no callback message from
	 * the server and waiting for callback will timeout.
	 * Note that normally the API is checking length. But if testing with this
	 * check disabled the server shall not crash.
	 */
	if (rc2 != WLC_OK) {
		test_validate(rc2, WLC_POLL_TIMEOUT);
	} else {
		test_validate(rc1, SA_AIS_ERR_INVALID_PARAM);
	}
}
