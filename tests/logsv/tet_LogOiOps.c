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
#include <limits.h>
#include <unistd.h>
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

/**
 * CCB Object Modify saLogStreamFileName
 */
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

/**
 * CCB Object Modify saLogStreamMaxLogFileSize
 */
void saLogOi_03(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=1000000 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify saLogStreamFixedLogRecordSize
 */
void saLogOi_04(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamFixedLogRecordSize=300 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify saLogStreamLogFullAction=1
 */
void saLogOi_05(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullAction=1 %s 2> /dev/null",
        SA_LOG_STREAM_ALARM);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify saLogStreamLogFullAction=2
 */
void saLogOi_06(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullAction=2 %s 2> /dev/null",
        SA_LOG_STREAM_ALARM);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify saLogStreamLogFullAction=3
 */
void saLogOi_07(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullAction=3 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify saLogStreamLogFullAction=4, ERR invalid
 */
void saLogOi_08(void)
{
    int rc;

    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullAction=4 %s 2> /dev/null",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify saLogStreamLogFullHaltThreshold=90%
 */
void saLogOi_09(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullHaltThreshold=90 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify saLogStreamLogFullHaltThreshold=101%, invalid
 */
void saLogOi_10(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFullHaltThreshold=101 %s 2> /dev/null",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify saLogStreamMaxFilesRotated
 */
void saLogOi_11(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamMaxFilesRotated=10 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify saLogStreamLogFileFormat
 */
void saLogOi_12(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"@Cr @Ct @Nh:@Nn:@Ns @Nm/@Nd/@NY @Ne5 @No30 @Ng30 \"@Cb\"\" %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify saLogStreamLogFileFormat - wrong format
 */
void saLogOi_13(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"@Cr @Ct @Sv @Ne5 @No30 @Ng30 \"@Cb\"\" %s 2> /dev/null",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify saLogStreamSeverityFilter
 */
void saLogOi_14(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamSeverityFilter=7 %s",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * saImmOiRtAttrUpdateCallback
 */
void saLogOi_15(void)
{
    int rc;
    char command[256];

    sprintf(command, "immlist %s > /dev/null", SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Log Service Administration API, change sev filter for app stream OK
 */
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

/**
 * Log Service Administration API, change sev filter, ERR invalid stream
 */
void saLogOi_17(void)
{
    int rc;
    char command[256];

    sprintf(command, "immadm -o 1 -p saLogStreamSeverityFilter:SA_UINT32_T:7 %s 2> /dev/null",
        SA_LOG_STREAM_ALARM);
    assert((rc = system(command)) != -1);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Log Service Administration API, change sev filter, ERR invalid arg type
 */
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

/**
 * Log Service Administration API, change sev filter, ERR invalid severity
 */
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

/**
 * Log Service Administration API, change sev filter, ERR invalid param name
 */
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

/**
 * Log Service Administration API, no change in sev filter, ERR NO OP
 */
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

/**
 * Log Service Administration API, invalid opId
 */
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

/* =============================================================================
 * Test stream configuration object attribute validation, suite 6
 * Note:
 * saLogStreamLogFileFormat see saLogOi_12 in suite 4
 * saLogStreamLogFullAction see saLogOi_05 - saLogOi_08 in suite 4
 * saLogStreamLogFullHaltThreshold see saLogOi_09
 * =============================================================================
 */

/* Note:
 * When testing 'object modify' immcfg will return with a timeout error from IMM if
 * the tests are done in full speed.
 * TST_DLY is a delay in seconds between tests for 'object Modify'
 */
#define TST_DLY 3

/* Note:
 * Tests using logMaxLogrecsize requires that logMaxLogrecsize
 * is default set to 1024 in the OpenSafLogConfig class definition
 */
#define MAX_LOGRECSIZE 1024

/* ***************************
 * Validate when object Create
 * ***************************/

/**
 * Create: saLogStreamSeverityFilter < 0x7f, Ok
 */
void saLogOi_65(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig "
		"safLgStrCfg=str6,safApp=safLogService -a saLogStreamSeverityFilter=%d"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.",
			0x7e);
	rc = system(command);
	
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);
	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Create: saLogStreamSeverityFilter >= 0x7f, ERR
 */
void saLogOi_66(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig "
		"safLgStrCfg=str6,safApp=safLogService -a saLogStreamSeverityFilter=%d"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=. 2> /dev/null",
			0x7f);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Create: saLogStreamPathName "../Test/" (Outside root path), ERR
 */
void saLogOi_67(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig "
		"safLgStrCfg=str6,safApp=safLogService "
		"-a saLogStreamFileName=str6file -a saLogStreamPathName=../Test 2> /dev/null");
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Create: saLogStreamFileName, Name and path already used by an existing stream, ERR
 */
void saLogOi_68(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig "
		"safLgStrCfg=str6,safApp=safLogService "
		"-a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);
	
	sprintf(command, "immcfg -c SaLogStreamConfig "
		"safLgStrCfg=str6,safApp=safLogService "
		"-a saLogStreamFileName=str6file -a saLogStreamPathName=. 2> /dev/null");
	rc = system(command);
	/* Delete the test object */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Create: saLogStreamMaxLogFileSize > logMaxLogrecsize, Ok
 */
void saLogOi_69(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamMaxLogFileSize=%d",
			MAX_LOGRECSIZE + 1);
	rc = system(command);
	/* Delete the test object */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Create: saLogStreamMaxLogFileSize == logMaxLogrecsize, ERR
 */
void saLogOi_70(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamMaxLogFileSize=%d 2> /dev/null",
			MAX_LOGRECSIZE);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Create: saLogStreamMaxLogFileSize < logMaxLogrecsize, ERR
 */
void saLogOi_71(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamMaxLogFileSize=%d 2> /dev/null",
			MAX_LOGRECSIZE - 1);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Create: saLogStreamFixedLogRecordSize < logMaxLogrecsize, Ok
 */
void saLogOi_72(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamFixedLogRecordSize=%d",
			MAX_LOGRECSIZE - 1);
	rc = system(command);
	/* Delete the test object */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);
	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Create: saLogStreamFixedLogRecordSize == logMaxLogrecsize, Ok
 */
void saLogOi_73(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamFixedLogRecordSize=%d",
			MAX_LOGRECSIZE);
	rc = system(command);
	/* Delete the test object */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);
	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Create: saLogStreamFixedLogRecordSize == 0, Ok
 */
void saLogOi_74(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamFixedLogRecordSize=%d",
			0);
	rc = system(command);
	/* Delete the test object */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);
	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Create: saLogStreamFixedLogRecordSize > logMaxLogrecsize, ERR
 */
void saLogOi_75(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamFixedLogRecordSize=%d 2> /dev/null",
			MAX_LOGRECSIZE + 1);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Create: saLogStreamMaxFilesRotated < 128, Ok
 */
void saLogOi_76(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamMaxFilesRotated=%d",
			127);
	rc = system(command);
	/* Delete the test object */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);
	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Create: saLogStreamMaxFilesRotated > 128, ERR
 */
void saLogOi_77(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamMaxFilesRotated=%d 2> /dev/null",
			129);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Create: saLogStreamMaxFilesRotated == 128, ERR
 */
void saLogOi_78(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamMaxFilesRotated=%d 2> /dev/null",
			128);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/* ***************************
 * Validate when object Modify
 * 
 * Note1:
 * saLogStreamLogFileFormat see saLogOi_12 in suite 4
 * saLogStreamLogFullAction see saLogOi_05 - saLogOi_08 in suite 4
 * saLogStreamLogFullHaltThreshold see saLogOi_09
 * 
 * ***************************/

/**
 * Modify: saLogStreamSeverityFilter < 0x7f, Ok
 */
void saLogOi_100(void)
{
	int rc;
	char command[512];

	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamSeverityFilter=%d "
		"safLgStrCfg=str6,safApp=safLogService",
			0x7e);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Modify: saLogStreamSeverityFilter >= 0x7f, ERR
 */
void saLogOi_101(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamSeverityFilter=%d"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			0x7f);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Modify: saLogStreamPathName "Test/" (Not possible to modify)
 */
void saLogOi_102(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamPathName=%s"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			"Test/");
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Modify: saLogStreamFileName, Name and path already used by an existing stream, ERR
 */
void saLogOi_103(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamFileName=%s"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			"str6file");
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Modify: saLogStreamFileName, Name exist but in other path, Ok
 */
void saLogOi_104(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6a,safApp=safLogService"
		" -a saLogStreamFileName=str6afile -a saLogStreamPathName=str6adir/");
	safassert(system(command),0);

	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=%s -a saLogStreamPathName=.",
			"str6file");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamFileName=%s"
		" safLgStrCfg=str6a,safApp=safLogService",
			"str6file");
	rc = system(command);
	
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);
	sprintf(command,"immcfg -d safLgStrCfg=str6a,safApp=safLogService");
	safassert(system(command),0);
	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Modify: saLogStreamFileName, New name, Ok
 */
void saLogOi_105(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6a,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamFileName=%s"
		" safLgStrCfg=str6a,safApp=safLogService",
			"str6new");
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6a,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Modify: saLogStreamMaxLogFileSize > logMaxLogrecsize, Ok
 */
void saLogOi_106(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=%d "
		"safLgStrCfg=str6,safApp=safLogService",
			MAX_LOGRECSIZE + 1);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Modify: saLogStreamMaxLogFileSize == logMaxLogrecsize, ERR
 */
void saLogOi_107(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=%d "
		"safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			MAX_LOGRECSIZE);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Modify: saLogStreamMaxLogFileSize < logMaxLogrecsize, ERR
 */
void saLogOi_108(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=%d"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			MAX_LOGRECSIZE - 1);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Modify: saLogStreamFixedLogRecordSize < logMaxLogrecsize, Ok
 */
void saLogOi_109(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamFixedLogRecordSize=%d"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			MAX_LOGRECSIZE - 1);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Modify: saLogStreamFixedLogRecordSize == 0, Ok
 */
void saLogOi_110(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamFixedLogRecordSize=%d"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			0);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Modify: saLogStreamFixedLogRecordSize == logMaxLogrecsize, Ok
 */
void saLogOi_111(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamFixedLogRecordSize=%d"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			MAX_LOGRECSIZE);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Modify: saLogStreamFixedLogRecordSize > logMaxLogrecsize, ERR
 */
void saLogOi_112(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamFixedLogRecordSize=%d"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			MAX_LOGRECSIZE + 1);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Modify: saLogStreamMaxFilesRotated < 128, Ok
 */
void saLogOi_113(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamMaxFilesRotated=%d"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			127);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Modify: saLogStreamMaxFilesRotated > 128, ERR
 */
void saLogOi_114(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamMaxFilesRotated=%d"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			129);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Modify: saLogStreamMaxFilesRotated == 128, ERR
 */
void saLogOi_115(void)
{
	int rc;
	char command[512];
	
	sleep(TST_DLY);
	/* Create */
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.");
	safassert(system(command),0);

	/* Test modify */
	sprintf(command, "immcfg -a saLogStreamMaxFilesRotated=%d"
		" safLgStrCfg=str6,safApp=safLogService"
		" 2> /dev/null",
			128);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 1);
}

#undef MAX_LOGRECSIZE

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
	
	/* Stream configuration object */
	/* Tests for create */
	test_suite_add(5, "LOG OI tests, Stream configuration object attribute validation");
	test_case_add(5, saLogOi_65, "Create: saLogStreamSeverityFilter < 0x7f, Ok");
	test_case_add(5, saLogOi_66, "Create: saLogStreamSeverityFilter >= 0x7f, ERR");
	test_case_add(5, saLogOi_67, "Create: saLogStreamPathName \"../Test/\" (Outside root path), ERR");
	test_case_add(5, saLogOi_68, "Create: saLogStreamFileName, Name and path already used by an existing stream, ERR");
	test_case_add(5, saLogOi_69, "Create: saLogStreamMaxLogFileSize > logMaxLogrecsize, Ok");
	test_case_add(5, saLogOi_70, "Create: saLogStreamMaxLogFileSize == logMaxLogrecsize, ERR");
	test_case_add(5, saLogOi_71, "Create: saLogStreamMaxLogFileSize < logMaxLogrecsize, ERR");
	test_case_add(5, saLogOi_72, "Create: saLogStreamFixedLogRecordSize < logMaxLogrecsize, Ok");
	test_case_add(5, saLogOi_73, "Create: saLogStreamFixedLogRecordSize == logMaxLogrecsize, Ok");
	test_case_add(5, saLogOi_74, "Create: saLogStreamFixedLogRecordSize == 0, Ok");
	test_case_add(5, saLogOi_75, "Create: saLogStreamFixedLogRecordSize > logMaxLogrecsize, ERR");
	test_case_add(5, saLogOi_76, "Create: saLogStreamMaxFilesRotated < 128, Ok");
	test_case_add(5, saLogOi_77, "Create: saLogStreamMaxFilesRotated > 128, ERR");
	test_case_add(5, saLogOi_78, "Create: saLogStreamMaxFilesRotated == 128, ERR");
	/* Tests for modify */
	test_case_add(5, saLogOi_100, "Modify: saLogStreamSeverityFilter < 0x7f, Ok");
	test_case_add(5, saLogOi_101, "Modify: saLogStreamSeverityFilter >= 0x7f, ERR");
	test_case_add(5, saLogOi_102, "Modify: saLogStreamPathName \"Test/\" (Not possible to modify)");
	test_case_add(5, saLogOi_103, "Modify: saLogStreamFileName, Name and path already used by an existing stream, ERR");
	test_case_add(5, saLogOi_104, "Modify: saLogStreamFileName, Name exist but in other path, Ok");
	test_case_add(5, saLogOi_105, "Modify: saLogStreamFileName, New name, Ok");
	test_case_add(5, saLogOi_106, "Modify: saLogStreamMaxLogFileSize > logMaxLogrecsize, Ok");
	test_case_add(5, saLogOi_107, "Modify: saLogStreamMaxLogFileSize == logMaxLogrecsize, ERR");
	test_case_add(5, saLogOi_108, "Modify: saLogStreamMaxLogFileSize < logMaxLogrecsize, ERR");
	test_case_add(5, saLogOi_109, "Modify: saLogStreamFixedLogRecordSize < logMaxLogrecsize, Ok");
	test_case_add(5, saLogOi_110, "Modify: saLogStreamFixedLogRecordSize == 0, Ok");
	test_case_add(5, saLogOi_111, "Modify: saLogStreamFixedLogRecordSize == logMaxLogrecsize, Ok");
	test_case_add(5, saLogOi_112, "Modify: saLogStreamFixedLogRecordSize > logMaxLogrecsize, ERR");
	test_case_add(5, saLogOi_113, "Modify: saLogStreamMaxFilesRotated < 128, Ok");
	test_case_add(5, saLogOi_114, "Modify: saLogStreamMaxFilesRotated > 128, ERR");
	test_case_add(5, saLogOi_115, "Modify: saLogStreamMaxFilesRotated == 128, ERR");
}

