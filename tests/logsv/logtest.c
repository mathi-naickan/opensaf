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
#include <limits.h>
#include <configmake.h>
#include <saImmOm.h>
#include <immutil.h>
#include <saImm.h>

#include "logtest.h"

SaNameT systemStreamName =
{
    .value = SA_LOG_STREAM_SYSTEM,
    .length = sizeof(SA_LOG_STREAM_SYSTEM)
};

SaNameT alarmStreamName =
{
    .value = SA_LOG_STREAM_ALARM,
    .length = sizeof(SA_LOG_STREAM_ALARM)
};

SaNameT notificationStreamName =
{
    .value = SA_LOG_STREAM_NOTIFICATION,
    .length = sizeof(SA_LOG_STREAM_NOTIFICATION)
};

SaNameT app1StreamName =
{
    .value = SA_LOG_STREAM_APPLICATION1,
    .length = sizeof(SA_LOG_STREAM_APPLICATION1)
};

SaNameT notifyingObject =
{
    .value = DEFAULT_NOTIFYING_OBJECT,
    .length = sizeof(DEFAULT_NOTIFYING_OBJECT)
};

SaNameT notificationObject =
{
    .value = DEFAULT_NOTIFICATION_OBJECT,
    .length = sizeof(DEFAULT_NOTIFICATION_OBJECT)
};

static char buf[2048];

SaLogBufferT alarmStreamBuffer =
{
    .logBuf = (SaUint8T *) buf,
    .logBufSize = 0,
};

SaLogBufferT notificationStreamBuffer =
{
    .logBuf = (SaUint8T *) buf,
    .logBufSize = 0,
};

static SaLogBufferT genLogBuffer =
{
    .logBuf = (SaUint8T *) buf,
    .logBufSize = 0,
};

SaNameT logSvcUsrName = 
{
    .value = SA_LOG_STREAM_APPLICATION1,
    .length = sizeof(SA_LOG_STREAM_APPLICATION1)
};

SaLogRecordT genLogRecord =
{
    .logTimeStamp = SA_TIME_UNKNOWN,
    .logHdrType = SA_LOG_GENERIC_HEADER,
    .logHeader.genericHdr.notificationClassId = NULL,
    .logHeader.genericHdr.logSeverity = SA_LOG_SEV_INFO,
    .logHeader.genericHdr.logSvcUsrName = &logSvcUsrName,
    .logBuffer = &genLogBuffer
};

SaVersionT logVersion = {'A', 0x02, 0x01}; 
SaVersionT immVersion = {'A', 2, 11};
SaAisErrorT rc;
SaLogHandleT logHandle;
SaLogStreamHandleT logStreamHandle;
SaLogCallbacksT logCallbacks = {NULL, NULL, NULL};
SaSelectionObjectT selectionObject;
char log_root_path[PATH_MAX];

void init_logrootpath(void)
{
	SaImmHandleT omHandle;
	SaNameT objectName = {
		.value = "logConfig=1,safApp=safLogService",
		.length = strlen("logConfig=1,safApp=safLogService")
	};
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 *attribute;
	SaImmAttrValuesT_2 **attributes;
	SaAisErrorT ais_rc = SA_AIS_OK;
	const char logRootDirectory_name[] = "logRootDirectory";
	SaImmAttrNameT attributeNames[2] = {(char *) logRootDirectory_name, NULL};
	void *value;
	
	/* NOTE: immutil init osaf_assert if error */
	(void) immutil_saImmOmInitialize(&omHandle, NULL, &immVersion);
	(void) immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle);

	/* Get all attributes of the object */
	ais_rc = immutil_saImmOmAccessorGet_2(accessorHandle, &objectName,
										attributeNames, &attributes);
	if (ais_rc == SA_AIS_OK) {
		attribute = attributes[0];
		value = attribute->attrValues[0];
		strncpy(log_root_path, *((char **) value), PATH_MAX);
	} else {
		/* We didn't get a root path from IMM. Use default */
		strncpy(log_root_path, PKGLOGDIR, PATH_MAX);
	}
	(void) immutil_saImmOmFinalize(omHandle);
}

int main(int argc, char **argv) 
{
    int suite = ALL_SUITES, tcase = ALL_TESTS;

	init_logrootpath();
    srandom(getpid());

    if (argc > 1)
    {
        suite = atoi(argv[1]);
    }

    if (argc > 2)
    {
        tcase = atoi(argv[2]);
    }

    if (suite == 0)
    {
        test_list();
        return 0;
    }
	
	if (suite == 999) {
		return 0;
	}

    return test_run(suite, tcase);
}  


