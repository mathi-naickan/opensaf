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

#include <poll.h>
#include <unistd.h>
#include "base/saf_error.h"

#include "logtest.h"

static SaInvocationT cb_invocation[8];
static SaAisErrorT cb_error[8];
static int cb_index;

static void logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
{
    cb_invocation[cb_index] = invocation;
    cb_error[cb_index] = error;
    cb_index++;
}


SaAisErrorT logWrite(SaInvocationT invocation, SaLogAckFlagsT ackFlags, const SaLogRecordT *logRecord)
{
        SaAisErrorT rc = saLogWriteLogAsync(logStreamHandle, invocation, ackFlags, logRecord);
        unsigned int nTries = 1;
        while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < logProfile.nTries) {
                usleep(logProfile.retryInterval * 1000);
                rc = saLogWriteLogAsync(logStreamHandle, invocation, ackFlags, logRecord);
                nTries++;
        }

        if (rc != SA_AIS_OK)
        	fprintf(stderr, " saLogWriteLogAsync FAILED: %s\n", saf_error(rc));
        return rc;
}

void saLogWriteLogCallbackT_01(void)
{
    SaInvocationT invocation;
    struct pollfd fds[1];
    int ret;

    invocation = random();
    logCallbacks.saLogWriteLogCallback = logWriteLogCallbackT;
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = saLogSelectionObjectGet(logHandle, &selectionObject);
    if (rc != SA_AIS_OK) {
        fprintf(stderr, " saLogSelectionObjectGet failed: %d \n", (int)rc);
        test_validate(rc, SA_AIS_OK);
        goto done;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }
    cb_index = 0;
    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    rc = logWrite(invocation, SA_LOG_RECORD_WRITE_ACK, &genLogRecord);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    fds[0].fd = (int) selectionObject;
    fds[0].events = POLLIN;
    ret = poll(fds, 1, 1000);
    if (ret != 1) {
        fprintf(stderr, " poll log callback failed: %d \n", ret);
        test_validate(ret, 1);
        goto done;
    }
    rc = saLogDispatch(logHandle, SA_DISPATCH_ONE);
    if (rc != SA_AIS_OK) {
        fprintf(stderr, " saLogDispatch failed: %d \n", (int)rc);
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    if (cb_invocation[0] == invocation)
        test_validate(cb_error[0], SA_AIS_OK);
    else
        test_validate(SA_AIS_ERR_LIBRARY, SA_AIS_OK);

done:
    logFinalize();
}

void saLogWriteLogCallbackT_02(void)
{
    SaInvocationT invocation[3];
    struct pollfd fds[1];
    int ret, i;

    invocation[0] = random();
    invocation[1] = random();
    invocation[2] = random();
    logCallbacks.saLogWriteLogCallback = logWriteLogCallbackT;
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        fprintf(stderr, " logInitialize failed: %d \n", (int)rc);
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = saLogSelectionObjectGet(logHandle, &selectionObject);
    if (rc != SA_AIS_OK) {
        fprintf(stderr, " saLogSelectionObjectGet failed: %d \n", (int)rc);
        test_validate(rc, SA_AIS_OK);
        goto done;
    }
    rc = logStreamOpen(&systemStreamName);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    cb_index = 0;
    strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
    genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
    rc = logWrite(invocation[0], SA_LOG_RECORD_WRITE_ACK, &genLogRecord);
    if (rc == SA_AIS_OK)
        rc = logWrite(invocation[1], SA_LOG_RECORD_WRITE_ACK, &genLogRecord);
    if (rc == SA_AIS_OK)
        rc = logWrite(invocation[2], SA_LOG_RECORD_WRITE_ACK, &genLogRecord);

    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    sleep(1); /* Let the writes make it to disk and back */
    fds[0].fd = (int) selectionObject;
    fds[0].events = POLLIN;
    ret = poll(fds, 1, 1000);
    if (ret != 1) {
        fprintf(stderr, " poll log callback failed: %d \n", ret);
        test_validate(ret, 1);
        goto done;
    }
    rc = saLogDispatch(logHandle, SA_DISPATCH_ALL);
    if (rc != SA_AIS_OK) {
        fprintf(stderr, " saLogDispatch failed: %d \n", (int)rc);
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    if (cb_index != 3)
    {
        printf("cb_index = %u\n", cb_index);
        test_validate(SA_AIS_ERR_LIBRARY, SA_AIS_OK);
        return;
    }

    for (i = 0; i < 3; i++)
    {
        if ((cb_invocation[i] != invocation[i]) || (cb_error[i] != SA_AIS_OK))
        {
            printf("%llu expected %llu, %u\n", cb_invocation[i], invocation[i], cb_error[i]);
            test_validate(cb_error[i], SA_AIS_OK);
            return;
        }
    }

    test_validate(SA_AIS_OK, SA_AIS_OK);

done:
    logFinalize();
}

