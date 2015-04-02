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
#include <unistd.h>
#include <saf_error.h>
#include "logtest.h"

#define opensaf_user "opensaf"
#define data_group "log-data"

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
	int rc_tmp = 0;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamFileName=notification %s",
        SA_LOG_STREAM_NOTIFICATION);
    rc = system(command);
	
	if (WEXITSTATUS(rc) == 0) {
		/* Clean up after test by setting name back to original
		 * Note: Has to be done only if name change succeeded
		 */
		sprintf(command, "immcfg -a saLogStreamFileName=saLogNotification %s",
			SA_LOG_STREAM_NOTIFICATION);
		rc_tmp = system(command);
		if (WEXITSTATUS(rc_tmp) != 0) {
			fprintf(stderr, "Failed to modify filename back to saLogNotification\n");
		}
	}
	
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command);
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
    rc = system(command); /* SA_AIS_OK */
    rc = system(command); /* will give SA_AIS_ERR_NO_OP */
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
    rc = system(command);

    safassert(saLogFinalize(logHandle), SA_AIS_OK);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Log Service Administration API, no parameters
 */
void saLogOi_116(void)
{
	int rc = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;
	char command[256];
	int active_sc_before_test = 0;
	int active_sc_after_test = 0;

	active_sc_before_test = get_active_sc();
	
	ais_rc = saLogInitialize(&logHandle, NULL, &logVersion);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "saLogInitialize Fail: %s\n", saf_error(ais_rc));
		rc = 255;
		goto done;
	}
	
	ais_rc = saLogStreamOpen_2(logHandle, &app1StreamName, &appStreamLogFileCreateAttributes,
        SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "saLogStreamOpen_2 Fail: %s\n", saf_error(ais_rc));
		rc = 255;
		goto done;
	}
	
	snprintf(command, 256, "immadm -o 1 %s 2> /dev/null", SA_LOG_STREAM_APPLICATION1);
	rc = system(command);
	rc = WEXITSTATUS(rc);
	
	int try_cnt = 0;
	do {
		ais_rc = saLogFinalize(logHandle);
		if (ais_rc == SA_AIS_OK)
			break; /* We don't have to TRY AGAIN */
		if (try_cnt++ >= 10)
			break; /* Don't wait any longer */
		usleep(1000 * 100); /* Wait 100 ms */
	} while (ais_rc == SA_AIS_ERR_TRY_AGAIN);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "saLogFinalize Fail: %s\n", saf_error(ais_rc));
		rc = 255;
		goto done;
	}
	
	done:
	active_sc_after_test = get_active_sc();
	
	if (active_sc_before_test != active_sc_after_test) {
		/* Fail over has been done */
		fprintf(stderr, "\nnFailover during test:\nActive node before test was SC-%d\n"
				"Active node after test is SC-%d\n",active_sc_before_test,
				active_sc_after_test);
		rc = 255;
	}
	rc_validate(rc, 1);
}

/**
 * CCB Object Create, strA
 */
void saLogOi_23()
{
    int rc;
	int i;
    char command[256];

    sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=strA,safApp=safLogService "
			"-a saLogStreamFileName=strA -a saLogStreamPathName=strAdir");
	/* Make more than one attempt */
	for (i=0; i<3; i++) {
		if ((rc = system(command)) == 0)
			break;
	}
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Create, strB
 */
void saLogOi_24()
{
    int rc;
	int i;
    char command[256];

    sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=strB,safApp=safLogService "
			"-a saLogStreamFileName=strB -a saLogStreamPathName=strBdir");
	/* Make more than one attempt */
	for (i=0; i<3; i++) {
		if ((rc = system(command)) == 0)
			break;
	}
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Create, strC
 */
void saLogOi_25()
{
    int rc;
	int i;
    char command[256];

    sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=strC,safApp=safLogService "
			"-a saLogStreamFileName=strC -a saLogStreamPathName=strCdir");
	/* Make more than one attempt */
	for (i=0; i<3; i++) {
		if ((rc = system(command)) == 0)
			break;
	}
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Delete, strB
 */
void saLogOi_26()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -d safLgStrCfg=strB,safApp=safLogService");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Delete, strC
 */
void saLogOi_27()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -d safLgStrCfg=strC,safApp=safLogService");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify, saLogStreamMaxFilesRotated=1, strA
 */
void saLogOi_28()
{
    int rc;
    char command[256];
    sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=2001 safLgStrCfg=strA,safApp=safLogService");
    rc = system(command);
	safassert(rc, 0);
    sprintf(command, "immcfg -a saLogStreamMaxFilesRotated=1 safLgStrCfg=strA,safApp=safLogService");
    rc = system(command);
	safassert(rc, 0);
    sprintf(command, "saflogtest -b strA --count=1000 --interval=5000 \"saflogtest (1000,5000) strA\"");
    rc = system(command);
	safassert(rc, 0);
    sprintf(command, "immcfg -a saLogStreamMaxFilesRotated=3 safLgStrCfg=strA,safApp=safLogService");
    rc = system(command);
	safassert(rc, 0);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify, saLogStreamMaxLogFileSize=0, strB, ERR Not supported
 */
void saLogOi_29()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=0 safLgStrCfg=strB,safApp=safLogService 2> /dev/null");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify, saLogStreamFixedLogRecordSize=80, strC
 */
void saLogOi_30()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamFixedLogRecordSize=80 safLgStrCfg=strC,safApp=safLogService");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * immlist strA-strC
 */
void saLogOi_31()
{
    int rc;
    char command[256];

    sprintf(command, "immlist safLgStrCfg=strA,safApp=safLogService > /dev/null");
    rc = system(command);
	safassert(rc, 0);
    sprintf(command, "immlist safLgStrCfg=strB,safApp=safLogService > /dev/null");
    rc = system(command);
	safassert(rc, 0);
    sprintf(command, "immlist safLgStrCfg=strC,safApp=safLogService > /dev/null");
    rc = system(command);
	safassert(rc, 0);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * immfind strA-strC
 */
void saLogOi_32()
{
    int rc;
    char command[256];

    sprintf(command, "immfind safLgStrCfg=strA,safApp=safLogService > /dev/null");
    rc = system(command);
	safassert(rc, 0);
	sprintf(command, "immfind safLgStrCfg=strB,safApp=safLogService > /dev/null");
    rc = system(command);
	safassert(rc, 0);
	sprintf(command, "immfind safLgStrCfg=strC,safApp=safLogService > /dev/null");
    rc = system(command);
	safassert(rc, 0);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * saflogger, writing to notification
 */
void saLogOi_33()
{
    int rc;
    char command[256];

    sprintf(command, "saflogger -n");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * saflogtest, writing to strA
 */
void saLogOi_34()
{
    int rc;
    char command[256];

    sprintf(command, "saflogtest -b strA --count=10 --interval=10000 \"saflogtest (10,10000) strA\"");
    rc = system(command);
    sprintf(command, "saflogtest -b strA --count=5 --interval=100000 \"saflogtest (5,100000) strA\"");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * saflogtest, writing to strB
 */
void saLogOi_35()
{
    int rc;
    char command[256];

    sprintf(command, "saflogtest -b strB --count=500 --interval=5 \"saflogtest (500,5) strB\"");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * saflogtest, writing to strC
 */
void saLogOi_36()
{
    int rc;
    char command[256];

    sprintf(command, "saflogtest -b strC --count=700 --interval=5 \"saflogtest (700,5) strC\"");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify, saLogStreamMaxLogFileSize=2000, strC
 */
void saLogOi_37()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=2000 safLgStrCfg=strC,safApp=safLogService");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify, saLogStreamFixedLogRecordSize=2048, strC, Error
 */
void saLogOi_38()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamFixedLogRecordSize=2048 safLgStrCfg=strC,safApp=safLogService "
			"2> /dev/null");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify, saLogStreamMaxLogFileSize=70, strC, Error
 */
void saLogOi_39()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=70 safLgStrCfg=strC,safApp=safLogService "
			"2> /dev/null");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Delete, strA
 */
void saLogOi_40()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -d safLgStrCfg=strA,safApp=safLogService");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Create, strD, illegal path, Error
 */
void saLogOi_41()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=strD,safApp=safLogService "
			"-a saLogStreamFileName=strDtest -a saLogStreamPathName=../strDdir 2> /dev/null");
    rc = system(command);
	if (rc <= 0) safassert(rc, 0);
	sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=strD,safApp=safLogService "
			"-a saLogStreamFileName=strDtest -a saLogStreamPathName=strDdir/.. 2> /dev/null");
    rc = system(command);
	if (rc <= 0) safassert(rc, 0);
	sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=strD,safApp=safLogService "
			"-a saLogStreamFileName=strDtest -a saLogStreamPathName=strDdir/../xyz 2> /dev/null");
    rc = system(command);
	if (rc <= 0) safassert(rc, 0);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Create, strD
 */
void saLogOi_42()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=strD,safApp=safLogService "
			"-a saLogStreamFileName=CrCtCdCyCb50CaSl30SvCx -a saLogStreamPathName=strDdir");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify, saLogStreamLogFileFormat (strD)
 */
void saLogOi_43(void)
{
    int rc;
    char command[256];
	char logFileFormat[256];
	strcpy(logFileFormat, "@Cr @Ct @Cd @Cy @Cb40 @Ca @Sl30 @Sv @Cx");
    sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"%s\" safLgStrCfg=strD,safApp=safLogService",
			logFileFormat);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * saflogtest, writing to strD
 */
void saLogOi_44()
{
    int rc;
    char command[256];

    sprintf(command, "saflogtest -b strD --count=500 --interval=5 \"saflogtest (500,5) strD\"");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify, saLogStreamFileName (strD)
 */
void saLogOi_45(void)
{
    int rc;
    char command[256];
	char logFileName[256];
	strcpy(logFileName, "CrCtChCnCsCaCmCMCdCyCYCcCxCb40Ci40");
    sprintf(command, "immcfg -a saLogStreamFileName=\"%s\" safLgStrCfg=strD,safApp=safLogService",
			logFileName);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify, saLogStreamLogFileFormat (all tokens) (strD)
 */
void saLogOi_46()
{
    int rc;
    char command[256];
	char logFileFormat[256];
	strcpy(logFileFormat, "@Cr @Ct @Ch @Cn @Cs @Ca @Cm @CM @Cd @Cy @CY @Cc @Cx @Cb40 @Ci40");
    sprintf(command, "immcfg -a saLogStreamLogFileFormat=\"%s\" safLgStrCfg=strD,safApp=safLogService",
			logFileFormat);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Delete, strD
 */
void saLogOi_47()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -d safLgStrCfg=strD,safApp=safLogService");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * saflogtest, writing to appTest
 */
void saLogOi_50()
{
    int rc;
    char command[256];
    int noTimes = 5;

    while (noTimes--)
    {
		sprintf(command, "saflogtest -a appTest --count=100 --interval=1 "
				"\"saflogtest (100,1,%d) appTest\"", noTimes);
        rc = system(command);
		safassert(rc, 0);
    }
    rc_validate(WEXITSTATUS(rc), 0);
}

static int get_filter_cnt_attr(const SaNameT* objName)
{
	SaVersionT immVersion = {'A', 2, 11};
	int filter_cnt = -1;
	SaImmHandleT immOmHandle;
	SaImmAccessorHandleT immAccHandle;
	saImmOmInitialize(&immOmHandle, NULL, &immVersion);
	saImmOmAccessorInitialize(immOmHandle, &immAccHandle);

	SaImmAttrNameT fobj = {"logStreamDiscardedCounter"};
	SaImmAttrNameT filterAttr[2] = {fobj, NULL};
	SaImmAttrValuesT_2** filterVal;
	if (saImmOmAccessorGet_2(immAccHandle, objName, filterAttr, &filterVal) == SA_AIS_OK) {
		filter_cnt = *(int*) (filterVal[0]->attrValues[0]);
	}
	saImmOmAccessorFinalize(immAccHandle);
	saImmOmFinalize(immOmHandle);
	return filter_cnt;
}

/**
 * saflogtest, writing to saLogApplication1, severity filtering check
 */
void saLogOi_51(void)
{
	int rc;
	char command[2][256];
	unsigned int sevType = 0;
	int filterCnt = 0;

	safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
	safassert(saLogStreamOpen_2(logHandle, &app1StreamName, &appStreamLogFileCreateAttributes,
			SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle), SA_AIS_OK);
	while (sevType < (SA_LOG_SEV_INFO + 1)) {
		sprintf(command[0], "immadm -o 1 -p saLogStreamSeverityFilter:SA_UINT32_T:%u %s 2> /dev/null",
				1 << sevType, SA_LOG_STREAM_APPLICATION1);
		rc = system(command[0]);
		/* saflogtest sends SA_LOG_SEV_INFO messages */
		sprintf(command[1], "saflogtest -a saLogApplication1 "
				"--count=100 --interval=20 \"writing (100,20,%d) saLogApplication1\"", sevType);
		assert((rc = system(command[1])) != -1);
		filterCnt = get_filter_cnt_attr(&app1StreamName);
		if (sevType < SA_LOG_SEV_INFO) {
			if (filterCnt != ((sevType + 1) * 100)) {
				rc = -1;
				break;
			}
		} else {
			if (filterCnt != sevType * 100) {
				rc = -2;
				break;
			}
		}
		sevType++;
	}
	safassert(saLogFinalize(logHandle), SA_AIS_OK);
	rc_validate(WEXITSTATUS(rc), 0);
}

/* =============================================================================
 * Test log service configuration object, suite 5
 * =============================================================================
 */

/**
 * CCB Object Modify, root directory. Path does not exist. Not allowed
 * Result shall be reject
 */
void saLogOi_52(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logRootDirectory=%s/yytest "
			"logConfig=1,safApp=safLogService 2> /dev/null",log_root_path);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);	
}

/**
 * CCB Object Modify, root directory. Path exist. OK
 * Result shall be OK
 */
void saLogOi_48(void)
{
    int rc = 0, tst_stat = 0;
    char command[256];
    char tstdir[256];

    /* Path to test directory */
    sprintf(tstdir, "%s/xxtest", log_root_path);

    /* Create test directory */
    sprintf(command, "mkdir -p %s", tstdir);
    rc = tet_system(command);
    if (rc != 0) {
	    fprintf(stderr, "'%s' Fail rc=%d\n", command, rc);
	    tst_stat = 1;
	    goto done;
    }
    /* Make sure it can be accessed by server */
    sprintf(command, "chmod ugo+w,ugo+r %s", tstdir);
    rc = tet_system(command);
    if (rc != 0) {
	    fprintf(stderr, "'%s' Fail rc=%d\n", command, rc);
	    tst_stat = 1;
	    goto done;
    }

    /* Change to xxtest */
    sprintf(command, "immcfg -a logRootDirectory=%s logConfig=1,safApp=safLogService",tstdir);
    rc = tet_system(command);
    if (rc != 0) {
	    fprintf(stderr, "'%s' Fail rc=%d\n", command, rc);
	    tst_stat = 1;
	    goto done_remove;
    }

    /* Change back */
    sprintf(command, "immcfg -a logRootDirectory=%s logConfig=1,safApp=safLogService",log_root_path);
    rc = tet_system(command);
    if (rc != 0) {
	    fprintf(stderr, "'%s' Fail rc=%d\n", command, rc);
	    tst_stat = 1;
	    goto done_remove;
    }

    done_remove:
    /* Remove xxtest no longer used */
    sprintf(command, "rm -rf %s/", tstdir);
    rc = tet_system(command);
    if (rc != 0) {
	    fprintf(stderr, "'%s' Fail rc=%d\n", command, rc);
	    goto done;
    }

    done:
    rc_validate(tst_stat, 0);
}

/**
 * CCB Object Modify, data group. Group does not exist. Not allowed
 * Result shall be reject
 */
void saLogOi_79(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a logDataGroupname=dummyGroup logConfig=1,safApp=safLogService > /dev/null 2>&1");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify, data group. Group exist. OK
 * Result shall be OK
 */
void saLogOi_80(void)
{
    int rc;
    char command[256];
#ifndef RUNASROOT
    /**
     * OpenSAF is running under opensaf user
     * Check if log-data is a supplementary group of that user
     */
    sprintf(command, "groups %s | grep %s > /dev/null", opensaf_user, data_group);
#else
    /* OpenSAF is running under root user */
    sprintf(command, "groups root | grep %s > /dev/null", data_group);
#endif

    rc = system(command);
    if (rc != 0) {
    	fprintf(stderr, "Data group %s is currently not a primary group of LOGSV. Skip this TC.\n", data_group);
    	rc = 0;
    } else {
    	sprintf(command, "immcfg -a logDataGroupname=%s logConfig=1,safApp=safLogService > /dev/null 2>&1", data_group);
    	rc = system(command);
    }
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify, delete data group. OK
 * Result shall be OK
 */
void saLogOi_81(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a logDataGroupname= logConfig=1,safApp=safLogService > /dev/null 2>&1");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify, logMaxLogrecsize. Not allowed
 * Result, Reject
 */
void saLogOi_53(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logMaxLogrecsize=%d "
			"logConfig=1,safApp=safLogService 2> /dev/null",1025);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);	
}

/**
 * CCB Object Modify, logStreamSystemHighLimit > logStreamSystemLowLimit. OK
 * Result OK
 */
void saLogOi_54(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logStreamSystemHighLimit=%d -a logStreamSystemLowLimit=%d"
			" logConfig=1,safApp=safLogService",50000,5000);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);	
}

/**
 * CCB Object Modify, logStreamSystemHighLimit = logStreamSystemLowLimit, != 0. Ok
 * Result Ok
 */
void saLogOi_55(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logStreamSystemHighLimit=%d -a logStreamSystemLowLimit=%d"
			" logConfig=1,safApp=safLogService 2> /dev/null",5000,5000);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);	
}

/**
 * CCB Object Modify, logStreamSystemHighLimit < logStreamSystemLowLimit. Error
 * Result, Reject
 */
void saLogOi_56(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logStreamSystemHighLimit=%d -a logStreamSystemLowLimit=%d"
			" logConfig=1,safApp=safLogService 2> /dev/null",5000,6000);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);	
}

/**
 * CCB Object Modify, logStreamSystemHighLimit = logStreamSystemLowLimit = 0. OK
 * Result OK
 */
void saLogOi_57(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logStreamSystemHighLimit=%d -a logStreamSystemLowLimit=%d"
			" logConfig=1,safApp=safLogService",0,0);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);	
}

/**
 * CCB Object Modify, logStreamAppHighLimit > logStreamAppLowLimit. OK
 * Result OK
 */
void saLogOi_58(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logStreamAppHighLimit=%d -a logStreamAppLowLimit=%d"
			" logConfig=1,safApp=safLogService",50000,5000);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);	
}

/**
 * CCB Object Modify, logStreamAppHighLimit = logStreamAppLowLimit, != 0. Ok
 * Result Ok
 */
void saLogOi_59(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logStreamAppHighLimit=%d -a logStreamAppLowLimit=%d"
			" logConfig=1,safApp=safLogService 2> /dev/null",5000,5000);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);	
}

/**
 * CCB Object Modify, logStreamAppHighLimit < logStreamAppLowLimit. Error
 * Result, Reject
 */
void saLogOi_60(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logStreamAppHighLimit=%d -a logStreamAppLowLimit=%d"
			" logConfig=1,safApp=safLogService 2> /dev/null",5000,6000);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);	
}

/**
 * CCB Object Modify, logStreamAppHighLimit = logStreamAppLowLimit = 0. OK
 * Result OK
 */
void saLogOi_61(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logStreamAppHighLimit=%d -a logStreamAppLowLimit=%d"
			" logConfig=1,safApp=safLogService",0,0);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);	
}

/**
 * CCB Object Modify, logMaxApplicationStreams. Not allowed
 * Result, Reject
 */
void saLogOi_62(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logMaxApplicationStreams=%d"
			" logConfig=1,safApp=safLogService 2> /dev/null",65);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);	
}

/**
 * CCB Object Modify, logFileIoTimeout. Not allowed
 * Result, Reject
 */
void saLogOi_63(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logFileIoTimeout=%d"
			" logConfig=1,safApp=safLogService 2> /dev/null",600);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);	
}

/**
 * CCB Object Modify, logFileSysConfig. Not allowed
 * Result, Reject
 */
void saLogOi_64(void)
{
    int rc;
    char command[256];
	
    sprintf(command, "immcfg -a logFileSysConfig=%d"
			" logConfig=1,safApp=safLogService 2> /dev/null",2);
    rc = system(command);
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
	int rc = 0, tst_stat = 0;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamFixedLogRecordSize=%d",
			MAX_LOGRECSIZE);
	rc = tet_system(command);
	if (rc != 0) {
		fprintf(stderr, "%s Fail rc=%d\n", command, rc);
		tst_stat = 1;
		goto done;
	}

	/* Delete the test object */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	rc = tet_system(command);
	if (rc != 0) {
		fprintf(stderr, "%s Fail rc=%d\n", command, rc);
		tst_stat = 1;
		goto done;
	}

	done:
	rc_validate(tst_stat, 0);
}

/**
 * Create: saLogStreamFixedLogRecordSize == 0, Ok
 */
void saLogOi_74(void)
{
	int rc = 0, tst_stat = 0;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig"
		" safLgStrCfg=str6,safApp=safLogService"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=."
		" -a saLogStreamFixedLogRecordSize=%d",
			0);
	rc = tet_system(command);
	if (rc != 0) {
		fprintf(stderr, "'%s' Fail rc=%d\n", command, rc);
		tst_stat = 1;
	}
	/* Delete the test object */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	rc = tet_system(command);
	if (rc != 0) {
		fprintf(stderr, "'%s' Fail rc=%d\n", command, rc);
		tst_stat = 1;
	}

	rc_validate(tst_stat, 0);
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
 * Note2:
 * Run PREPARE before running any individual test case in suite 6.
 * This will create a test object. Is done automatically if running whole suite.
 * 
 * Note3:
 * CLEAN6 can be used to delete the test object for clean up.
 * Is done automatically if running whole suite.
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
	/* Stream objects */
    test_suite_add(4, "LOG OI tests, stream objects");
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
	test_case_add(4, saLogOi_116,"Log Service Administration API, no parameters");
	test_case_add(4, saLogOi_23, "CCB Object Create, strA");
    test_case_add(4, saLogOi_24, "CCB Object Create, strB");
    test_case_add(4, saLogOi_25, "CCB Object Create, strC");
    test_case_add(4, saLogOi_26, "CCB Object Delete, strB");
    test_case_add(4, saLogOi_27, "CCB Object Delete, strC");
    test_case_add(4, saLogOi_24, "CCB Object Create, strB");
    test_case_add(4, saLogOi_25, "CCB Object Create, strC");
    test_case_add(4, saLogOi_28, "CCB Object Modify, saLogStreamMaxFilesRotated=1, strA");
    test_case_add(4, saLogOi_29, "CCB Object Modify, saLogStreamMaxLogFileSize=0, strB, ERR not supported");
    test_case_add(4, saLogOi_30, "CCB Object Modify, saLogStreamFixedLogRecordSize=80, strC");
    test_case_add(4, saLogOi_31, "immlist strA-strC");
    test_case_add(4, saLogOi_32, "immfind strA-strC");
    test_case_add(4, saLogOi_33, "saflogger, writing to notification");
    test_case_add(4, saLogOi_34, "saflogtest, writing to strA");
    test_case_add(4, saLogOi_35, "saflogtest, writing to strB");
    test_case_add(4, saLogOi_36, "saflogtest, writing to strC");
    test_case_add(4, saLogOi_37, "CCB Object Modify, saLogStreamMaxLogFileSize=2000, strC");
    test_case_add(4, saLogOi_38, "CCB Object Modify, saLogStreamFixedLogRecordSize=2048, strC, Error");
    test_case_add(4, saLogOi_39, "CCB Object Modify, saLogStreamMaxLogFileSize=70, strC, Error");
    test_case_add(4, saLogOi_26, "CCB Object Delete, strB");
    test_case_add(4, saLogOi_27, "CCB Object Delete, strC");
    test_case_add(4, saLogOi_40, "CCB Object Delete, strA");
    test_case_add(4, saLogOi_41, "CCB Object Create, strD, illegal path, Error");
	test_case_add(4, saLogOi_42, "CCB Object Create, strD");
    test_case_add(4, saLogOi_43, "CCB Object Modify, saLogStreamLogFileFormat (strD)");
    test_case_add(4, saLogOi_44, "saflogtest, writing to strD");
	test_case_add(4, saLogOi_45, "CCB Object Modify, saLogStreamFileName (strD)");
	test_case_add(4, saLogOi_46, "CCB Object Modify, saLogStreamLogFileFormat (all tokens) (strD)");
	test_case_add(4, saLogOi_44, "saflogtest, writing to strD");
    test_case_add(4, saLogOi_47, "CCB Object Delete, strD");
    test_case_add(4, saLogOi_23, "CCB Object Create, strA");
    test_case_add(4, saLogOi_40, "CCB Object Delete, strA");
    test_case_add(4, saLogOi_50, "saflogtest, writing to appTest");
    test_case_add(4, saLogOi_51, "saflogtest, writing to saLogApplication1, severity filtering check");
	
	/* Configuration object */
    test_suite_add(5, "LOG OI tests, Service configuration object");
    test_case_add(5, saLogOi_52, "CCB Object Modify, root directory. Path does not exist. Not allowed");
    test_case_add(5, saLogOi_48, "CCB Object Modify, root directory. Path exist. OK");
    test_case_add(5, saLogOi_79, "CCB Object Modify, data group. Group does not exist. Not allowed");
    test_case_add(5, saLogOi_80, "CCB Object Modify, data group. Group exists. OK");
    test_case_add(5, saLogOi_81, "CCB Object Modify, delete data group. OK");
    test_case_add(5, saLogOi_53, "CCB Object Modify, logMaxLogrecsize. Not allowed");
    test_case_add(5, saLogOi_54, "CCB Object Modify, logStreamSystemHighLimit > logStreamSystemLowLimit. OK");
    test_case_add(5, saLogOi_55, "CCB Object Modify, logStreamSystemHighLimit = logStreamSystemLowLimit, != 0. Ok");
    test_case_add(5, saLogOi_56, "CCB Object Modify, logStreamSystemHighLimit < logStreamSystemLowLimit. Error");
    test_case_add(5, saLogOi_57, "CCB Object Modify, logStreamSystemHighLimit = logStreamSystemLowLimit = 0. OK");
    test_case_add(5, saLogOi_58, "CCB Object Modify, logStreamAppHighLimit > logStreamAppLowLimit. OK");
    test_case_add(5, saLogOi_59, "CCB Object Modify, logStreamAppHighLimit = logStreamAppLowLimit, != 0. Ok");
    test_case_add(5, saLogOi_60, "CCB Object Modify, logStreamAppHighLimit < logStreamAppLowLimit. Error");
    test_case_add(5, saLogOi_61, "CCB Object Modify, logStreamAppHighLimit = logStreamAppLowLimit = 0. OK");
    test_case_add(5, saLogOi_62, "CCB Object Modify, logMaxApplicationStreams. Not allowed");
    test_case_add(5, saLogOi_63, "CCB Object Modify, logFileIoTimeout. Not allowed");
    test_case_add(5, saLogOi_64, "CCB Object Modify, logFileSysConfig. Not allowed");
	
	/* Stream configuration object */
	/* Tests for create */
	test_suite_add(6, "LOG OI tests, Stream configuration object attribute validation");
	test_case_add(6, saLogOi_65, "Create: saLogStreamSeverityFilter < 0x7f, Ok");
	test_case_add(6, saLogOi_66, "Create: saLogStreamSeverityFilter >= 0x7f, ERR");
	test_case_add(6, saLogOi_67, "Create: saLogStreamPathName \"../Test/\" (Outside root path), ERR");
	test_case_add(6, saLogOi_68, "Create: saLogStreamFileName, Name and path already used by an existing stream, ERR");
	test_case_add(6, saLogOi_69, "Create: saLogStreamMaxLogFileSize > logMaxLogrecsize, Ok");
	test_case_add(6, saLogOi_70, "Create: saLogStreamMaxLogFileSize == logMaxLogrecsize, ERR");
	test_case_add(6, saLogOi_71, "Create: saLogStreamMaxLogFileSize < logMaxLogrecsize, ERR");
	test_case_add(6, saLogOi_72, "Create: saLogStreamFixedLogRecordSize < logMaxLogrecsize, Ok");
	test_case_add(6, saLogOi_73, "Create: saLogStreamFixedLogRecordSize == logMaxLogrecsize, Ok");
	test_case_add(6, saLogOi_74, "Create: saLogStreamFixedLogRecordSize == 0, Ok");
	test_case_add(6, saLogOi_75, "Create: saLogStreamFixedLogRecordSize > logMaxLogrecsize, ERR");
	test_case_add(6, saLogOi_76, "Create: saLogStreamMaxFilesRotated < 128, Ok");
	test_case_add(6, saLogOi_77, "Create: saLogStreamMaxFilesRotated > 128, ERR");
	test_case_add(6, saLogOi_78, "Create: saLogStreamMaxFilesRotated == 128, ERR");
	/* Tests for modify */
	test_case_add(6, saLogOi_100, "Modify: saLogStreamSeverityFilter < 0x7f, Ok");
	test_case_add(6, saLogOi_101, "Modify: saLogStreamSeverityFilter >= 0x7f, ERR");
	test_case_add(6, saLogOi_102, "Modify: saLogStreamPathName \"Test/\" (Not possible to modify)");
	test_case_add(6, saLogOi_103, "Modify: saLogStreamFileName, Name and path already used by an existing stream, ERR");
	test_case_add(6, saLogOi_104, "Modify: saLogStreamFileName, Name exist but in other path, Ok");
	test_case_add(6, saLogOi_105, "Modify: saLogStreamFileName, New name, Ok");
	test_case_add(6, saLogOi_106, "Modify: saLogStreamMaxLogFileSize > logMaxLogrecsize, Ok");
	test_case_add(6, saLogOi_107, "Modify: saLogStreamMaxLogFileSize == logMaxLogrecsize, ERR");
	test_case_add(6, saLogOi_108, "Modify: saLogStreamMaxLogFileSize < logMaxLogrecsize, ERR");
	test_case_add(6, saLogOi_109, "Modify: saLogStreamFixedLogRecordSize < logMaxLogrecsize, Ok");
	test_case_add(6, saLogOi_110, "Modify: saLogStreamFixedLogRecordSize == 0, Ok");
	test_case_add(6, saLogOi_111, "Modify: saLogStreamFixedLogRecordSize == logMaxLogrecsize, Ok");
	test_case_add(6, saLogOi_112, "Modify: saLogStreamFixedLogRecordSize > logMaxLogrecsize, ERR");
	test_case_add(6, saLogOi_113, "Modify: saLogStreamMaxFilesRotated < 128, Ok");
	test_case_add(6, saLogOi_114, "Modify: saLogStreamMaxFilesRotated > 128, ERR");
	test_case_add(6, saLogOi_115, "Modify: saLogStreamMaxFilesRotated == 128, ERR");
}
