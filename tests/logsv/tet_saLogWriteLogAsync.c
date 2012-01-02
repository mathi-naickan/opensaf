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

#include "logtest.h"

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
        .length = sizeof(appStreamName.value)
    };

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

