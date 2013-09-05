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

#include <sys/types.h>
#include <sys/wait.h>
#include <saImm.h>
#include <saImmOm.h>
#include <limits.h>
#include "logtest.h"

static SaLogFileCreateAttributesT_2 appStreamLogFileCreateAttributes =
{
    .logFilePathName = DEFAULT_APP_FILE_PATH_NAME,
    .logFileName = DEFAULT_APP_FILE_NAME,
    .maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE,
    .maxLogRecordSize = DEFAULT_APP_LOG_REC_SIZE,
    .haProperty = SA_TRUE,
    .logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE,
    .maxFilesRotated = DEFAULT_MAX_FILE_ROTATED,
    .logFileFmt = DEFAULT_FORMAT_EXPRESSION
};

void saLogOi_01(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamFileName=notification %s",
        SA_LOG_STREAM_NOTIFICATION);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify saLogStreamPathName, ERR not allowed
 */
void saLogOi_02(void)
{
    int rc;
    char command[256];

	/* Create an illegal path name (log_root_path> cd ../) */
	char tststr[PATH_MAX];
	char *tstptr;
	strcpy(tststr,log_root_path);
	tstptr = strrchr(tststr, '/');
	*tstptr = '\0';
 
    sprintf(command, "immcfg -a saLogStreamPathName=/%s %s 2> /dev/null",
			tststr,
			SA_LOG_STREAM_ALARM);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);
}

void saLogOi_03(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=1000000 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_04(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamFixedLogRecordSize=300 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_05(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullAction=1 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_06(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullAction=2 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_07(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullAction=3 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_08(void)
{
    int rc;

    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullAction=4 %s 2> /dev/null",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 1);
}

void saLogOi_09(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullHaltThreshold=90 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_10(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullHaltThreshold=101 %s 2> /dev/null",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 1);
}

void saLogOi_11(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamMaxFilesRotated=10 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_12(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"@Cr @Ct @Nh:@Nn:@Ns @Nm/@Nd/@NY @Ne5 @No30 @Ng30 \"@Cb\"\" %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_13(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"@Cr @Ct @Sv @Ne5 @No30 @Ng30 \"@Cb\"\" %s 2> /dev/null",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 1);
}

void saLogOi_14(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamSeverityFilter=7 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_15(void)
{
    int rc;
    char command[256];

    sprintf(command, "immlist %s > /dev/null", SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_16(void)
{
    int rc;
    char command[256];

    sprintf(command, "immadm -o 1 -p saLogStreamSeverityFilter:SA_UINT32_T:7 %s 2> /dev/null",
        SA_LOG_STREAM_APPLICATION1);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &app1StreamName, &appStreamLogFileCreateAttributes,
        SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    assert((rc = system(command)) != -1);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);
    rc_validate(WEXITSTATUS(rc), 0);
}

void saLogOi_17(void)
{
    int rc;
    char command[256];

    sprintf(command, "immadm -o 1 -p saLogStreamSeverityFilter:SA_UINT32_T:7 %s 2> /dev/null",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 1);
}

void saLogOi_18(void)
{
    int rc;
    char command[256];

    sprintf(command, "immadm -o 1 -p saLogStreamSeverityFilter:SA_UINT64_T:7 %s 2> /dev/null",
        SA_LOG_STREAM_APPLICATION1);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &app1StreamName, &appStreamLogFileCreateAttributes,
        SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    assert((rc = system(command)) != -1);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);
    rc_validate(WEXITSTATUS(rc), 1);
}

void saLogOi_19(void)
{
    int rc;
    char command[256];

    sprintf(command, "immadm -o 1 -p saLogStreamSeverityFilter:SA_UINT32_T:1024 %s 2> /dev/null",
        SA_LOG_STREAM_APPLICATION1);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &app1StreamName, &appStreamLogFileCreateAttributes,
        SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    assert((rc = system(command)) != -1);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);
    rc_validate(WEXITSTATUS(rc), 1);
}

void saLogOi_20(void)
{
    int rc;
    char command[256];

    sprintf(command, "immadm -o 1 -p severityFilter:SA_UINT32_T:7 %s 2> /dev/null",
        SA_LOG_STREAM_APPLICATION1);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &app1StreamName, &appStreamLogFileCreateAttributes,
        SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    assert((rc = system(command)) != -1);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);
    rc_validate(WEXITSTATUS(rc), 1);
}

void saLogOi_21(void)
{
    int rc;
    char command[256];

    sprintf(command, "immadm -o 1 -p saLogStreamSeverityFilter:SA_UINT32_T:7 %s 2> /dev/null",
        SA_LOG_STREAM_APPLICATION1);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &app1StreamName, &appStreamLogFileCreateAttributes,
        SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    assert((rc = system(command)) != -1); /* SA_AIS_OK */
    assert((rc = system(command)) != -1); /* will give SA_AIS_ERR_NO_OP */
    safassert(saLogFinalize(logHandle), SA_AIS_OK);
    rc_validate(WEXITSTATUS(rc), 1);
}

void saLogOi_22(void)
{
    int rc;
    char command[256];

    sprintf(command, "immadm -o 99 -p saLogStreamSeverityFilter:SA_UINT32_T:127 %s 2> /dev/null",
        SA_LOG_STREAM_APPLICATION1);
    safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
    safassert(saLogStreamOpen_2(logHandle, &app1StreamName, &appStreamLogFileCreateAttributes,
        SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
    assert((rc = system(command)) != -1);
    safassert(saLogFinalize(logHandle), SA_AIS_OK);
    rc_validate(WEXITSTATUS(rc), 1);
}

__attribute__ ((constructor)) static void saOiOperations_constructor(void)
{
    test_suite_add(4, "LOG OI tests");
    test_case_add(4, saLogOi_01, "CCB Object Modify saLogStreamFileName");
    test_case_add(4, saLogOi_02, "CCB Object Modify saLogStreamPathName, ERR not allowed");
    test_case_add(4, saLogOi_03, "CCB Object Modify saLogStreamMaxLogFileSize");
    test_case_add(4, saLogOi_04, "CCB Object Modify saLogStreamFixedLogRecordSize");
    test_case_add(4, saLogOi_05, "CCB Object Modify saLogStreamLogFullAction=1");
    test_case_add(4, saLogOi_06, "CCB Object Modify saLogStreamLogFullAction=2");
    test_case_add(4, saLogOi_07, "CCB Object Modify saLogStreamLogFullAction=3");
    test_case_add(4, saLogOi_08, "CCB Object Modify saLogStreamLogFullAction=4, ERR invalid");
    test_case_add(4, saLogOi_09, "CCB Object Modify saLogStreamLogFullHaltThreshold=90%");
    test_case_add(4, saLogOi_10, "CCB Object Modify saLogStreamLogFullHaltThreshold=101%, invalid");
    test_case_add(4, saLogOi_11, "CCB Object Modify saLogStreamMaxFilesRotated");
    test_case_add(4, saLogOi_12, "CCB Object Modify saLogStreamLogFileFormat");
    test_case_add(4, saLogOi_13, "CCB Object Modify saLogStreamLogFileFormat - wrong format");
    test_case_add(4, saLogOi_14, "CCB Object Modify saLogStreamSeverityFilter");
    test_case_add(4, saLogOi_15, "saImmOiRtAttrUpdateCallback");
    test_case_add(4, saLogOi_16, "Log Service Administration API, change sev filter for app stream OK");
    test_case_add(4, saLogOi_17, "Log Service Administration API, change sev filter, ERR invalid stream");
    test_case_add(4, saLogOi_18, "Log Service Administration API, change sev filter, ERR invalid arg type");
    test_case_add(4, saLogOi_19, "Log Service Administration API, change sev filter, ERR invalid severity");
    test_case_add(4, saLogOi_20, "Log Service Administration API, change sev filter, ERR invalid param name");
    test_case_add(4, saLogOi_21, "Log Service Administration API, no change in sev filter, ERR NO OP");
    test_case_add(4, saLogOi_22, "Log Service Administration API, invalid opId");
}

