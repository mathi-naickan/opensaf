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

/**
 * Due to the ticket #1443, if creating/deleting conf obj class continuously,
 * logsv will be crashed or deleting action gets failed.
 *
 * For now, until the ticket is fixed, adding some delay b/w creating/deleting
 * as a workaround to avoid impact on testing that frequently encounter
 * when running whole test suite in one go.
 *
 */

/* Add delay 10ms. Once the #1443 is done, undefine following macro */
#define __ADD_SLEEP__
#ifdef __ADD_SLEEP__
static void delay_ms(void) { usleep (10*1000); }
#else
static void delay_ms(void) {};
#endif // __ADD_SLEEP__

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
	delay_ms();
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
	delay_ms();
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
 * CCB Object Modify, saLogStreamFixedLogRecordSize=150, strC
 */
void saLogOi_30()
{
	int rc;
	char command[256];

	sprintf(command, "immcfg -a saLogStreamFixedLogRecordSize=150 safLgStrCfg=strC,safApp=safLogService");
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
	delay_ms();
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
	delay_ms();
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

	/* Remove 'tstdir' if it already exists */
	sprintf(command, "rm -rf %s/", tstdir);
	rc = tet_system(command);
	if (rc != 0) {
		fprintf(stderr, "'%s' Fail rc=%d\n", command, rc);
		goto done;
	}

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
		goto done;
	}

	/* Change back */
	sprintf(command, "immcfg -a logRootDirectory=%s logConfig=1,safApp=safLogService",log_root_path);
	rc = tet_system(command);
	if (rc != 0) {
		fprintf(stderr, "'%s' Fail rc=%d\n", command, rc);
		tst_stat = 1;
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
 * Create: saLogStreamSeverityFilter <= 0x7f, Ok
 */
void saLogOi_65(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig "
		"safLgStrCfg=str6,safApp=safLogService -a saLogStreamSeverityFilter=%d"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=.",
		0x7f);
	rc = system(command);
	
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	delay_ms();
	safassert(system(command),0);
	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Create: saLogStreamSeverityFilter > 0x7f, ERR
 */
void saLogOi_66(void)
{
	int rc;
	char command[512];
	
	sprintf(command, "immcfg -c SaLogStreamConfig "
		"safLgStrCfg=str6,safApp=safLogService -a saLogStreamSeverityFilter=%d"
		" -a saLogStreamFileName=str6file -a saLogStreamPathName=. 2> /dev/null",
		0x7f + 1);
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
 * Modify: saLogStreamSeverityFilter <= 0x7f, Ok
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
		0x7f);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	delay_ms();
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Modify: saLogStreamSeverityFilter > 0x7f, ERR
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
		0x7f + 1);
	rc = system(command);
	/* Delete */
	sprintf(command,"immcfg -d safLgStrCfg=str6,safApp=safLogService");
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
	safassert(system(command),0);
	sprintf(command,"immcfg -d safLgStrCfg=str6a,safApp=safLogService");
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
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
	delay_ms();
	safassert(system(command),0);

	rc_validate(WEXITSTATUS(rc), 1);
}

/****************************************
 * These below functions are used to test added new tokens:
 * @Nz/@Cz: Represents a field of 5 characters with format [+-][hhmm]
 * The content gives the deviation in hours and minutes btw local time/standard time
 *
 * @Nk/@Ck: Represents time in milliseconds of the seconds from logTimeStamp
 * Format will be 3 digits.
 *
 * Each test case will follow the sequences:
 *
 * 1. Get the current attribute from IMM
 * 2. Modify the attribute
 * 3. Send a log stream
 * 4. Check content of configuration (*.cfg)/log file (*.log)
 * 5. Restore the attribute to previous value
 *
 ***************************************/

/* Default format expression for alm/not streams */
#define DEFAULT_ALM_NOT_FORMAT_EXP "@Cr @Ct @Nt @Ne6 @No30 @Ng30 \"@Cb\""

/* Default format expression for sys/app streams */
#define DEFAULT_APP_SYS_FORMAT_EXP "@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Sv @Sl \"@Cb\""

/* Commands to search for specific pattern on files having 01 last minute
 * (?) accessed, but only openning file.
 * The flow of execution as below:
 *
 * 1. Find (linux `find` command) all files in the directory specificed by
 * %s (e.g: root_log_path) and subfolders which having 01 last minute accessed
 * (e.g: read/write access) or lesser.
 *
 * 2. With `egrep``, it helps to filter just openning log files
 * in list of files found - which have correct format
 * <logfilename>_<yyyymmdd>_<hhmmss>.log
 *
 * 3. With `egrep`, search content in the list of right-format
 * found files for a specific pattern
 * (e.g: timezone format = [+-][0-9]{4}, millisecond format = [0-9]{3})
 *
 * 4. Don't show found result (if tester wants to see the search result,
 * comment final line and its above slash)
 *
 * Note: If found pattern, exit status of these pipes will be
 * EXIT_SUCCESS(0), otherwise EXIT_FAILURE(1)/non-zero(?)
 */
#define VERIFY_CMD "find %s -type f -mmin -1"		\
	" | egrep \".*(%s_[0-9]{8}_[0-9]{6})\\.log$\""	\
	" | xargs egrep  \" %s \""			\
	" 1> /dev/null"

/* Search pattern for timezone token format (@Cz/@Nz) */
#define TIMEZ_PATTERN "[+-][0-9]{4}" 	/* format: 5 characters with format [+-][hhmm] */
/* Search pattern for millisecond token format (@Ck/@Nk) */
#define MILLI_PATTERN "[0-9]{3}"	/* format: 3 digits */

#define ALM_LOG_FILE "saLogAlarm"
#define NTF_LOG_FILE "saLogNotification"
#define SYS_LOG_FILE "saLogSystem"
#define MAX_DATA 256

/* immcfg command to modify saLogStreamLogFileFormat for a specific DN */
#define IMMCFG_CMD "immcfg -a saLogStreamLogFileFormat=\"%s\" %s"

/**
 * CCB Object Modify saLogStreamLogFileFormat of alarm log stream
 * by adding timezon token and check if configuration/log file
 * are reflected correctly.
 */
void modStrLogFileFmt_01(void)
{
	int rc;
	char command[MAX_DATA];
	char preLogStrFileFmt[MAX_DATA];
	/* Enable timezone format - @Nz */
	const char* modLogStrFileFmt = "@Cr @Ct @Nt @Nz @Ne6 @No30 @Ng30 @Cb";

	/* Get current value of the attribute, use default value if failed */
	rc = get_attr_value(&alarmStreamName, "saLogStreamLogFileFormat", preLogStrFileFmt);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		strncpy(preLogStrFileFmt, DEFAULT_ALM_NOT_FORMAT_EXP, MAX_DATA);
	}

	/* Modify the attribute */
	sprintf(command, IMMCFG_CMD, modLogStrFileFmt, SA_LOG_STREAM_ALARM);
	rc = system(command);
	if (WEXITSTATUS(rc) == 0) {
		/* Send an alarm to alarm log stream */
		rc = system("saflogger -l");
		if (WEXITSTATUS(rc)) {
			/* Failed to send the alarm to log stream */
			fprintf(stderr, "Failed to invoke saflogger -a\n");
			rc_validate(WEXITSTATUS(rc), 0);
			/* Exit the test case */
			return;
		}

		/* Verify the content of log file if it is reflected with right format */
		sprintf(command, VERIFY_CMD, log_root_path, ALM_LOG_FILE, TIMEZ_PATTERN);
		rc = system(command);
		if (rc == -1) {
			fprintf(stderr, "Failed to execute command (%s) on shell\n",
				VERIFY_CMD);
			test_validate(rc, 0);
			/* Exit the test case */
			return;
		}
		/* Command is executed succesfully on the shell, verify the result */
		rc_validate(WEXITSTATUS(rc), 0);

		/* Restore the attribute to previous value */
		sprintf(command, IMMCFG_CMD, preLogStrFileFmt, SA_LOG_STREAM_ALARM);
		rc = system(command);
		if (WEXITSTATUS(rc) != 0) {
			/* Failed to restore to privous value, print message */
			fprintf(stderr, "Failed to restore the attribute to previous value\n");
		}
	} else {
		rc_validate(WEXITSTATUS(rc), 0);
	}
}

/**
 * CCB Object Modify saLogStreamLogFileFormat of alarm log stream
 * by adding millisecond token and check if configuration/log file
 * are reflected correctly.
 */
void modStrLogFileFmt_02(void)
{
	int rc;
	char command[MAX_DATA];
	char preLogStrFileFmt[MAX_DATA];
	/* Enable millisecond format - @Nk */
	const char* modLogStrFileFmt = "@Cr @Ct @Nt @Nk @Ne6 @No30 @Ng30 @Cb";

	/* Get current value of the attribute, use default value if failed */
	rc = get_attr_value(&alarmStreamName, "saLogStreamLogFileFormat",
			    preLogStrFileFmt);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		strncpy(preLogStrFileFmt, DEFAULT_ALM_NOT_FORMAT_EXP, MAX_DATA);
	}

	/* Modify the attribute */
	sprintf(command, IMMCFG_CMD, modLogStrFileFmt, SA_LOG_STREAM_ALARM);
	rc = system(command);
	if (WEXITSTATUS(rc) == 0) {
		/* Send an alarm to alarm log stream */
		rc = system("saflogger -l");
		if (WEXITSTATUS(rc)) {
			/* Failed to send the alarm to alarm log stream. Report error, then exit */
			fprintf(stderr, "Failed to invoke saflogger -l\n");
			rc_validate(WEXITSTATUS(rc), 0);
			return;
		}

		/* Verify the content of log file if it is reflected with right format */
		sprintf(command, VERIFY_CMD, log_root_path, ALM_LOG_FILE, MILLI_PATTERN);
		rc = system(command);
		if (rc == -1) {
			/* Failed to execute command on shell. Report error, then exit */
			fprintf(stderr, "Failed to execute command (%s) on the shell \n",
				VERIFY_CMD);
			test_validate(rc, 0);
			/* Exit the test case */
			return;
		}
		/* Command is executed succesfully on the shell. Verify the result */
		rc_validate(WEXITSTATUS(rc), 0);

		/* Restore the attribute to previous value */
		sprintf(command, IMMCFG_CMD, preLogStrFileFmt, SA_LOG_STREAM_ALARM);
		rc = system(command);
		if (WEXITSTATUS(rc)) {
			/* Failed to restore the attribute to previous value */
			fprintf(stderr, "Failed to restore the attribute to previous value \n");
		}
	} else {
		rc_validate(WEXITSTATUS(rc), 0);
	}
}

/**
 * CCB Object Modify saLogStreamLogFileFormat of system log stream
 * by adding timezon token and check if configuration/log file
 * are reflected correctly.
 */
void modStrLogFileFmt_03(void)
{
	int rc;
	char command[MAX_DATA];
	char preLogStrFileFmt[MAX_DATA];
	/* Enable timezone format - @Cz */
	const char* modLogStrFileFmt = "@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Cz @Sv @Sl \"@Cb\"";

	/* Get current value of the attribute, use default value if failed (-1) */
	rc = get_attr_value(&systemStreamName, "saLogStreamLogFileFormat",
			    preLogStrFileFmt);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		strncpy(preLogStrFileFmt, DEFAULT_APP_SYS_FORMAT_EXP, MAX_DATA);
	}

	/* Modify the attribute */
	sprintf(command, IMMCFG_CMD, modLogStrFileFmt, SA_LOG_STREAM_SYSTEM);
	rc = system(command);
	if (WEXITSTATUS(rc) == 0) {
		/* Send an system log to system log stream */
		rc = system("saflogger -y");
		if (WEXITSTATUS(rc)) {
			/* Failed to send system log to system log stream */
			fprintf(stderr, "Failed to invoke saflogger -y");
			rc_validate(WEXITSTATUS(rc), 0);
			/* Exit the test case */
			return;
		}

		/* Verify the content of log file if it is reflected with right format */
		sprintf(command, VERIFY_CMD, log_root_path, SYS_LOG_FILE, TIMEZ_PATTERN);
		rc = system(command);
		if (rc == -1) {
			/* Failed to execute command on the shell. Report error, then exit */
			fprintf(stderr, "Failed to execute the command (%s) on the shell \n",
				VERIFY_CMD);
			test_validate(rc, 0);
			/* Exit the test case */
			return;
		}
		/* The command executed succesfully on the shell. Verify the result */
		rc_validate(WEXITSTATUS(rc), 0);

		/* Restore the attribute to previous value */
		sprintf(command, IMMCFG_CMD, preLogStrFileFmt, SA_LOG_STREAM_SYSTEM);
		rc = system(command);
		if (WEXITSTATUS(rc)) {
			/* Failed to restore the attribute to previous value */
			fprintf(stderr, "Failed to restore the attribute to previous value \n");
		}

	} else {
		rc_validate(WEXITSTATUS(rc), 0);
	}
}

/**
 * CCB Object Modify saLogStreamLogFileFormat of system log stream
 * by adding millisecond token and check if configuration/log file
 * are reflected correctly.
 */
void modStrLogFileFmt_04(void)
{
	int rc;
	char command[MAX_DATA];
	char preLogStrFileFmt[MAX_DATA];
	/* Enable timezone format - @Ck */
	const char* modLogStrFileFmt = "@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Ck @Sv @Sl @Cb";

	/* Get current value of the attribute, use default value if failed (-1) */
	rc = get_attr_value(&systemStreamName, "saLogStreamLogFileFormat",
			    preLogStrFileFmt);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		strncpy(preLogStrFileFmt, DEFAULT_APP_SYS_FORMAT_EXP, MAX_DATA);
	}

	/* Modify the attribute */
	sprintf(command, IMMCFG_CMD, modLogStrFileFmt, SA_LOG_STREAM_SYSTEM);
	rc = system(command);
	if (WEXITSTATUS(rc) == 0) {
		/* Send an system log to system log stream */
		rc = system("saflogger -y");
		if (WEXITSTATUS(rc)) {
			/* Failed to send system log to system log stream */
			fprintf(stderr, "Failed to invoke saflogger -y\n");
			rc_validate(WEXITSTATUS(rc), 0);
			return;
		}

		/* Verify the content of log file if it is reflected with right format */
		sprintf(command, VERIFY_CMD, log_root_path, SYS_LOG_FILE, MILLI_PATTERN);
		rc = system(command);
		if (rc == -1) {
			/* Failed to execute the command on the shell. Report error, then exit. */
			fprintf(stderr, "Failed to execute command (%s) on the shell. \n",
				VERIFY_CMD);
			test_validate(rc, 0);
			/* Exit the test case */
			return;
		}

		rc_validate(WEXITSTATUS(rc), 0);

		/* Restore the attribute to previous value */
		sprintf(command, IMMCFG_CMD, preLogStrFileFmt, SA_LOG_STREAM_SYSTEM);
		rc = system(command);
		if (WEXITSTATUS(rc)) {
			fprintf(stderr, "Failed to restore the attribute to previous value \n");
		}
	} else {
		rc_validate(WEXITSTATUS(rc), 0);
	}
}

/**
 * CCB Object Modify saLogStreamLogFileFormat of system log stream
 * by adding timezone and millisecond token
 * then check if configuration/log file are reflected correctly.
 */
void modStrLogFileFmt_05(void)
{
	int rc;
	char command[MAX_DATA];
	char preLogStrFileFmt[MAX_DATA];
	/* Enable timezone and millisecond token - @Cz @Ck */
	const char* modLogStrFileFmt = "@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Cz @Ck @Sv @Sl @Cb";

	/* Get current value of the attribute, use default value if failed (-1) */
	rc = get_attr_value(&systemStreamName, "saLogStreamLogFileFormat",
			    preLogStrFileFmt);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		strncpy(preLogStrFileFmt, DEFAULT_APP_SYS_FORMAT_EXP, MAX_DATA);
	}

	/* Modify the attribute */
	sprintf(command, IMMCFG_CMD, modLogStrFileFmt, SA_LOG_STREAM_SYSTEM);
	rc = system(command);
	if (WEXITSTATUS(rc) == 0) {
		/* Send an system log to system log stream */
		rc = system("saflogger -y");
		if (WEXITSTATUS(rc)) {
			/* Failed to send system log to sys log stream */
			fprintf(stderr, "Failed to invoke saflogger -y\n");
			rc_validate(WEXITSTATUS(rc), 0);
			return;
		}

		char tZoneMillP[MAX_DATA];
		sprintf(tZoneMillP, "%s %s", TIMEZ_PATTERN, MILLI_PATTERN);

		/* Verify the content of log file if it is reflected with right format */
		sprintf(command, VERIFY_CMD, log_root_path, SYS_LOG_FILE, tZoneMillP);
		rc = system(command);
		if (rc == -1) {
			/* Failed to execute command on the shell. Report error, then exit */
			fprintf(stderr, "Failed to execute command (%s) on the shell. \n",
				VERIFY_CMD);
			test_validate(rc, 0);
			return;
		}
		rc_validate(WEXITSTATUS(rc), 0);

		/* Restore the attribute to previous value */
		sprintf(command, IMMCFG_CMD, preLogStrFileFmt, SA_LOG_STREAM_SYSTEM);
		rc = system(command);
		if (WEXITSTATUS(rc)) {
			/* Failed to restore the attribute to previous value */
			fprintf(stderr, "Failed to restore to previous value \n");
		}
	} else {
		rc_validate(WEXITSTATUS(rc), 0);
	}
}

/**
 * Change logStreamFileFormat attribute, then create application stream
 * with logFileFmt set to NULL. Write a log record to that stream.
 *
 * Check whether written log file format is reflected correctly
 * with one defined by logStreamFileFormat.
 *
 */
void verDefaultLogFileFmt(void)
{
	int rc;
	char command[MAX_DATA];
	char preLogStrFileFmt[MAX_DATA];
	char appLogPath[MAX_DATA];

	/* Enable timezone and millisecond token - @Cz @Ck */
	const char* modLogStrFileFmt = "@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Cz @Ck @Sv @Sl @Cb";
	/*
	  The description for this macro is same as one for VERIFY_CMD.
	  Except, it includes close timestamp.
	*/
#define VERIFY_CMD_ "find %s -type f -mmin -1"                          \
		" | egrep \"%s_[0-9]{8}_[0-9]{6}_[0-9]{8}_[0-9]{6}\\.log$\"" \
		" | xargs egrep  \" %s \""				\
		" 1> /dev/null"

	/* Get current value of the attribute, use default value if failed (-1) */
	rc = get_attr_value(&configurationObject, "logStreamFileFormat",
			    preLogStrFileFmt);
	if (rc == -1) {
		/* Failed, use default one */
		strncpy(preLogStrFileFmt, DEFAULT_APP_SYS_FORMAT_EXP, MAX_DATA);
	}

	/* Modify the attribute */
	sprintf(command, "immcfg -a logStreamFileFormat=\"%s\" %s",
		modLogStrFileFmt, SA_LOG_CONFIGURATION_OBJECT);

	rc = system(command);
	if (WEXITSTATUS(rc) == 0) {
		/* Send an system log to app stream */
		rc = system("saflogger -a safLgStr=verDefaultLogFileFmt");
		if (WEXITSTATUS(rc)) {
			/* Failed to send log record to app stream */
			fprintf(stderr, "Failed to invoke saflogger -a\n");
			rc_validate(WEXITSTATUS(rc), 0);
			return;
		}

		char tZoneMillP[MAX_DATA];
		sprintf(tZoneMillP, "%s %s", TIMEZ_PATTERN, MILLI_PATTERN);

		/* Verify the content of log file if it is reflected with right format */
		sprintf(appLogPath, "%s/saflogger", log_root_path);
		sprintf(command, VERIFY_CMD_, appLogPath,
			"safLgStr=verDefaultLogFileFmt",	tZoneMillP);

		rc = system(command);
		if (rc == -1) {
			/* Failed to execute command on the shell. Report error, then exit */
			fprintf(stderr, "Failed to execute command (%s) on the shell. \n",
				VERIFY_CMD_);
			test_validate(rc, 0);
			return;
		}
		rc_validate(WEXITSTATUS(rc), 0);

		/* Restore the attribute to previous value */
		sprintf(command, "immcfg -a logStreamFileFormat=\"%s\" %s",
			preLogStrFileFmt, SA_LOG_CONFIGURATION_OBJECT);

		rc = system(command);
		if (WEXITSTATUS(rc)) {
			/* Failed to restore the attribute to previous value */
			fprintf(stderr, "Failed to restore to previous value \n");
		}
	} else {
		rc_validate(WEXITSTATUS(rc), 0);
	}
}

#undef MAX_LOGRECSIZE

/**
 * Add test cases for ticket #1288
 * - logFileIoTimeout is changeable in runtime
 * - logMaxLogrecsize is changeable in runtime
 * - Max log record is up to 65535
 */

#define MAX_LOGRECSIZE (65535)

/**
 * CCB Object Modify, logFileIoTimeout.
 * Verify it is possible to change the value
 */
void verLogFileIoTimeout(void)
{
	int rc;
	char command[256];
	uint32_t val = 0;

	/* Get current value of logMaxApplicationStreams */
	rc = get_attr_value(&configurationObject, "logFileIoTimeout", &val);
	if (rc == -1) {
		/* if failed, use default value */
		fprintf(stderr, "Failed to get attribute value from IMM \n");
		val = 500;
	}

	sprintf(command, "immcfg -a logFileIoTimeout=%d"
		" logConfig=1,safApp=safLogService 2> /dev/null", 600);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);

	/* If perform the command succesfully, restore to previous value. */
	if (WEXITSTATUS(rc) == 0) {
		sprintf(command, "immcfg -a logFileIoTimeout=%u"
			" logConfig=1,safApp=safLogService 2> /dev/null", val);
		rc = system(command);
	}
}

/**
 * CCB Object Modify, logFileIoTimeout < 500. Not allowed
 * Result, Reject
 */
void verLogFileIoTimeout_Err_01(void)
{
	int rc;
	char command[256];

	sprintf(command, "immcfg -a logFileIoTimeout=%d"
		" logConfig=1,safApp=safLogService 2> /dev/null", 400);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify, logFileIoTimeout > 5000. Not allowed
 * Result, Reject
 */
void verLogFileIoTimeout_Err_02(void)
{
	int rc;
	char command[256];

	sprintf(command, "immcfg -a logFileIoTimeout=%d"
		" logConfig=1,safApp=safLogService 2> /dev/null", 6000);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify, valid logMaxLogrecsize value is in range [150 - 65535].
 */
void verLogMaxLogrecsize(void)
{
	int rc;
	char command[256];
	uint32_t logMaxRec = 0;

	/* Get current value of the attribute, use default value if failed (-1) */
	rc = get_attr_value(&configurationObject, "logMaxLogrecsize", &logMaxRec);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		logMaxRec = 1024;
	}

	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", MAX_LOGRECSIZE);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);

	/* Restore logMaxLogrecsize to previous value */
	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", logMaxRec);
	rc = system(command);
}

/**
 * CCB Object Modify, verify the rule:
 * saLogStreamMaxLogFileSize > logMaxLogrecsize >= saLogStreamFixedLogRecordSize
 */
void verLogMaxLogrecsize_dep(void)
{
	int rc;
	char command[256];
	uint32_t logMaxRec = 0;

	/* Get current value of the attribute, use default value if failed (-1) */
	rc = get_attr_value(&configurationObject, "logMaxLogrecsize", &logMaxRec);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		logMaxRec = 1024;
	}

	/* Change the logMaxLogrecsize */
	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", 1000);
	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Failed to perform command. Report test failed */
		fprintf(stderr, "Failed to perfom command - %s \n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		return;
	}

	/* Create an stream with valid values */
	sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=Test,safApp=safLogService"
		" -a saLogStreamFileName=Test -a saLogStreamPathName=Test"
		" -a saLogStreamFixedLogRecordSize=1000 "
		" -a saLogStreamMaxLogFileSize=5000");
	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Failed to perform command. Report test failed */
		fprintf(stderr, "Failed to perfom command - %s \n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	/* Change the logMaxLogrecsize to value less than above fixedLogRecordSize */
	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", 500);
	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Failed to perform command. Report test failed */
		fprintf(stderr, "Failed to perfom command - %s \n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		goto free_class;
	}

	/*
	 * Change the saLogStreamMaxLogFileSize to valid value.
	 * Verify that the command performs succesfully.
	 */
	sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=%d "
		"safLgStrCfg=Test,safApp=safLogService 2> /dev/null", 6000);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);

free_class:
	/* Delete created obj class */
	sprintf(command, "immcfg -d safLgStrCfg=Test,safApp=safLogService "
		"2> /dev/null");
	rc = system(command);

done:
	/* Restore logMaxLogrecsize to previous value */
	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", logMaxRec);
	rc = system(command);
}

/**
 * CCB Object Modify, logMaxLogrecsize < 150. Not allowed.
 */
void verLogMaxLogrecsize_Err_01(void)
{
	int rc;
	char command[256];

	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", 149);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * CCB Object Modify, logMaxLogrecsize > 65535. Not allowed.
 */
void verLogMaxLogrecsize_Err_02(void)
{
	int rc;
	char command[256];

	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", MAX_LOGRECSIZE + 1);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Also add test cases to verify the rules:
 * ---------------------------------------
 * 1 - saLogStreamFixedLogRecordSize must be less than saLogStreamMaxLogFileSize
 * 2 - saLogStreamFixedLogRecordSize must be less than or equal to logMaxLogrecsize
 * 3 - saLogStreamFixedLogRecordSize can be 0. Means variable record size
 * 4 - saLogStreamMaxLogFileSize must be bigger than 0. No limit is not supported
 * 5 - saLogStreamMaxLogFileSize must be bigger than logMaxLogrecsize
 * 6 - saLogStreamMaxLogFileSize must be bigger than saLogStreamFixedLogRecordSize
 * ----------------------------------------
 *
 * For rules #2, #3, #5, test cases are already added above.
 */

/**
 * CCB Object modify, verify rule #1/#6
 * saLogStreamFixedLogRecordSize < saLogStreamMaxLogFileSize. Allowed.
 */
void verFixLogRec_MaxFileSize(void)
{
	int rc;
	char command[256];
	uint32_t fixLogRec = 0;
	uint64_t maxFileSize = 0;
	uint32_t logMaxRec = 0;

	/* Get current value of logMaxLogrecsize attribute, use default value if failed (-1) */
	rc = get_attr_value(&configurationObject, "logMaxLogrecsize", &logMaxRec);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		logMaxRec = 1024;
	}

	/* Get current value of saLogStreamFixedLogRecordSize */
	rc = get_attr_value(&alarmStreamName, "saLogStreamFixedLogRecordSize", &fixLogRec);
	if (rc == -1) {
		/* Report test failed and exit if getting value failed */
		fprintf(stderr, "Failed to get attribute value from IMM \n");
		test_validate(rc, 0);
		return;
	}

	/* Get current value of saLogStreamMaxLogFileSize */
	rc = get_attr_value(&alarmStreamName, "saLogStreamMaxLogFileSize", &maxFileSize);
	if (rc == -1) {
		/* Report test failed and exit if getting value failed */
		fprintf(stderr, "Failed to get attribute value from IMM \n");
		test_validate(rc, 0);
		return;
	}

	/* Allow to set saLogStreamMaxLogFileSize > saLogStreamFixedLogRecordSize */
	sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=%u "
		"-a saLogStreamFixedLogRecordSize=%u %s 2> /dev/null",
		logMaxRec + 1, logMaxRec, SA_LOG_STREAM_ALARM);

	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);

	/* Restore the changed value */
	if (WEXITSTATUS(rc) == 0) {
		sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=%" PRIu64
			" -a saLogStreamFixedLogRecordSize=%d %s 2> /dev/null",
			maxFileSize, fixLogRec, SA_LOG_STREAM_ALARM);
		rc = system(command);
	}
}

/**
 * CCB Object modify, verify rule #1/#6
 * saLogStreamFixedLogRecordSize > saLogStreamMaxLogFileSize. Not allowed.
 */
void verFixLogRec_MaxFileSize_Err(void)
{
	int rc;
	char command[256];
	uint32_t fixLogRec = 0;
	uint64_t maxFileSize = 0;

	/* Get current value of saLogStreamFixedLogRecordSize */
	rc = get_attr_value(&alarmStreamName, "saLogStreamFixedLogRecordSize", &fixLogRec);
	if (rc == -1) {
		/* Report test failed and exit if getting value failed */
		fprintf(stderr, "Failed to get attribute value from IMM \n");
		test_validate(rc, 0);
		return;
	}

	/* Get current value of saLogStreamMaxLogFileSize */
	rc = get_attr_value(&alarmStreamName, "saLogStreamMaxLogFileSize", &maxFileSize);
	if (rc == -1) {
		/* Report test failed and exit if getting value failed */
		fprintf(stderr, "Failed to get attribute value from IMM \n");
		test_validate(rc, 0);
		return;
	}

	/* Not allow to set saLogStreamMaxLogFileSize < saLogStreamFixedLogRecordSize */
	sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=%d %s 2> /dev/null",
		fixLogRec - 1, SA_LOG_STREAM_ALARM);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);

	/* Restore the changed value */
	if (WEXITSTATUS(rc) == 0) {
		sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=%" PRIu64 " %s 2> /dev/null",
			maxFileSize, SA_LOG_STREAM_ALARM);
		rc = system(command);
	}
}

/**
 * CCB Object modify, verify rule #4
 * saLogStreamMaxLogFileSize = 0. Not allowed.
 */
void verMaxLogFileSize_Err(void)
{
	int rc;
	char command[256];
	uint64_t val = 0;

	/* Get current value of logMaxApplicationStreams */
	rc = get_attr_value(&alarmStreamName, "saLogStreamMaxLogFileSize", &val);
	if (rc == -1) {
		/* Report test failed and exit if getting value failed */
		fprintf(stderr, "Failed to get attribute value from IMM \n");
		test_validate(rc, 0);
		return;
	}

	/* Allow to set saLogStreamMaxLogFileSize=0 */
	sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=0 %s 2> /dev/null",
		SA_LOG_STREAM_ALARM);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);

	/* Restore the changed value */
	if (WEXITSTATUS(rc) == 0) {
		sprintf(command, "immcfg -a saLogStreamMaxLogFileSize=%" PRIu64 " %s 2> /dev/null",
			val, SA_LOG_STREAM_ALARM);
		rc = system(command);
	}
}

/**
 * Write a maximum log record to app stream.
 * Then, verify if it is done succesfully or not.
 *
 * Steps:
 *
 * 1. Change the logMaxLogrecsize value to maximum one.
 * 2. Create application stream with saLogStreamFixedLogRecordSize
 *    equal to logMaxLogrecsize (maximum value).
 * 3. Create a log record buffer with 65535 bytes.
 * 4. Using saflogger tool, writing that buffer to the created app stream.
 * 5. Get file size of the app stream log file
 * 6. Verify if the size of file is larger than 65535 bytes or not
 * 7. Delete created app class
 */

void verMaxLogRecord_01(void)
{
	int rc;
	char command[66000];
	char logRecord[MAX_LOGRECSIZE];
	uint32_t logMaxRec = 0;

	/* Get current value of the attribute, use default value if failed (-1) */
	rc = get_attr_value(&configurationObject, "logMaxLogrecsize", &logMaxRec);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		logMaxRec = 1024;
	}

	/* Change the attribute value to maximum one */
	sprintf(command, "immcfg -a logMaxLogrecsize=%u"
		" logConfig=1,safApp=safLogService 2> /dev/null", MAX_LOGRECSIZE);

	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Failed to set logMaxLogrecsize. Report test failed */
		fprintf(stderr, "Failed to perform command = %s\n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		return;
	}

	/* Set saLogStreamFixedLogRecordSize = 0, fix log record = maxLogRecordSize */
	sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=maxrecsize,safApp=safLogService"
		" -a saLogStreamFileName=verMaxLogrecsize -a saLogStreamPathName=vermaxsize"
		" -a saLogStreamFixedLogRecordSize=0");

	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Fail to perform the command. Report TC failed */
		fprintf(stderr, "Failed to perform command = %s", command);
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	/* Prepare log record data */
	memset(logRecord, 'A', MAX_LOGRECSIZE - 2);
	logRecord[MAX_LOGRECSIZE - 1] = '\0';

	sprintf(command, "saflogger -a safLgStrCfg=maxrecsize,safApp=safLogService \"%s\"",
		logRecord);

	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Fail to perform command. Report test failed. */
		fprintf(stderr, "Failed to perform command = %s\n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		goto free_class;
	}
	/* Write log record succesfully. Then, measure the log file size. */
	FILE *fp = NULL;
	char fileSize_c[10];
	uint64_t fileSize = 0;

	/* Command to get exact app stream log file, and measure that file size. */
	sprintf(command, "find %s/vermaxsize -type f -mmin -1 "
		"| egrep \"%s_([0-9]{8}_[0-9]{6}\\.log$)\" "
		"| xargs wc -c | awk '{printf $1}'",
		log_root_path, "verMaxLogrecsize");

	fp = popen(command, "r");

	if (fp == NULL) {
		/* Fail to read size of log file. Report test failed. */
		fprintf(stderr, "Failed to run command = %s\n", command);
		test_validate(1, 0);
		goto free_class;
	}
	/* Get output of the command - actually the file size in chars */
	while (fgets(fileSize_c, sizeof(fileSize_c) - 1, fp) != NULL) {};
	pclose(fp);

	/* Convert chars to number */
	fileSize = atoi(fileSize_c);

	if (fileSize < MAX_LOGRECSIZE) {
		fprintf(stderr, "Log file size (%" PRIu64 ") is less than %d \n",
			fileSize, MAX_LOGRECSIZE);
		test_validate(fileSize, MAX_LOGRECSIZE);
		goto free_class;
	} else {
		rc_validate(WEXITSTATUS(rc), 0);
	}

free_class:
	/* Delete class created */
	(void)strcpy(command, "immcfg -d safLgStrCfg=maxrecsize,safApp=safLogService");
	rc = system(command);

done:
	/* Restore logMaxLogrecsize to previous value */
	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", logMaxRec);
	rc = system(command);
}

/**
 * Write a log record containing special characters.
 * Make sure the logsv functions normally.
 */
void verMaxLogRecord_02(void)
{
	int rc;
	char command[66000];
	char logRecord[MAX_LOGRECSIZE];
	uint32_t logMaxRec = 0;

	/* Get current value of the attribute, use default value if failed (-1) */
	rc = get_attr_value(&configurationObject, "logMaxLogrecsize", &logMaxRec);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		logMaxRec = 1024;
	}

	/* Change the attribute value to maximum one */
	sprintf(command, "immcfg -a logMaxLogrecsize=%u"
		" logConfig=1,safApp=safLogService 2> /dev/null", MAX_LOGRECSIZE);

	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Failed to set logMaxLogrecsize. Report test failed */
		fprintf(stderr, "Failed to perform command = %s\n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		return;
	}
	/* Set saLogStreamFixedLogRecordSize = max */
	sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=specialCharactor,safApp=safLogService"
		" -a saLogStreamFileName=specialCharactor -a saLogStreamPathName=vermaxsize"
		" -a saLogStreamFixedLogRecordSize=%d", MAX_LOGRECSIZE);

	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Fail to perform the command. Report TC failed */
		fprintf(stderr, "Failed to perform command = %s", command);
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	/* Prepare log record data with special characters */
	memset(logRecord, 'A', MAX_LOGRECSIZE - 2);
	logRecord[1] = '\t';
	logRecord[2] = 't';
	logRecord[3] = '\n';
	logRecord[4] = 'n';
	logRecord[5] = ' ';
	logRecord[6] = 'S';
	logRecord[MAX_LOGRECSIZE - 1] = '\0';

	sprintf(command, "saflogger -a safLgStrCfg=specialCharactor,safApp=safLogService \"%s\"",
		logRecord);

	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Fail to perform command. Report test failed. */
		fprintf(stderr, "Failed to perform command = %s\n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		goto free_class;
	}
	/* Write log record succesfully. Then, measure the log file size. */
	FILE *fp = NULL;
	char fileSize_c[10];
	uint32_t fileSize = 0;

	/* Command to get exact app stream log file, and measure that file size. */
	sprintf(command, "find %s/vermaxsize -type f -mmin -1 "
		"| egrep \"%s_([0-9]{8}_[0-9]{6}\\.log$)\" "
		"| xargs wc -c | awk '{printf $1}'",
		log_root_path, "specialCharactor");

	fp = popen(command, "r");

	if (fp == NULL) {
		/* Fail to read size of log file. Report test failed. */
		fprintf(stderr, "Failed to run command = %s\n", command);
		test_validate(1, 0);
		goto free_class;
	}
	/* Get output of the command - actually the file size in chars */
	while (fgets(fileSize_c, sizeof(fileSize_c) - 1, fp) != NULL) {};
	pclose(fp);

	/* Convert chars to number */
	fileSize = atoi(fileSize_c);

	if (fileSize < MAX_LOGRECSIZE) {
		fprintf(stderr, "Log file size (%u) is less than %d \n",
			fileSize, MAX_LOGRECSIZE);
		test_validate(fileSize, MAX_LOGRECSIZE);
		goto free_class;
	} else {
		rc_validate(WEXITSTATUS(rc), 0);
	}

free_class:
	/* Delete class created */
	(void)strcpy(command, "immcfg -d safLgStrCfg=specialCharactor,safApp=safLogService");
	rc = system(command);

done:
	/* Restore logMaxLogrecsize to previous value */
	sprintf(command, "immcfg -a logMaxLogrecsize=%d "
		"logConfig=1,safApp=safLogService 2> /dev/null", logMaxRec);
	rc = system(command);
}

/**
 * Add test case for ticket #1399:
 * logsv gets stuck in while loop when setting maxFilesRotated=0
 *
 * This test case verifies logsv returns error if
 * setting 0 to saLogStreamMaxFilesRotated attribute.
 */
void verMaxFilesRotated(void)
{
	int rc;
	char command[256];

	/* Expect getting exist code = 1 as saLogStreamMaxFilesRotated=0 */
	strcpy(command, "immcfg -c SaLogStreamConfig safLgStrCfg=verMaxFilesRotated,safApp=safLogService "
	       "-a saLogStreamFileName=str6file -a saLogStreamPathName=. -a saLogStreamMaxFilesRotated=0 "
	       "2> /dev/null");
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/* Add test cases to test #1490 */

/* Verify that logStreamFileFormat value is allowed to delete */
void verLogStreamFileFormat(void)
{
	int rc;
	char command[256];
	char logFileFmt[256];
	bool logFmtEmtpy = true;

	/* Get current value of logStreamFileFormat attribute */
	rc = get_attr_value(&configurationObject, "logStreamFileFormat", logFileFmt);
	if (rc != -1) {
		logFmtEmtpy = false;
	}

	sprintf(command, "immcfg -a logStreamFileFormat= %s 2> /dev/null",
		LOGTST_IMM_LOG_CONFIGURATION);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);

	if ((logFmtEmtpy == false) && (WEXITSTATUS(rc) == 0)) {
		/* The value is deleted. Restore it */
		sprintf(command, "immcfg -a logStreamFileFormat=\"%s\" %s 2> /dev/null",
			logFileFmt, LOGTST_IMM_LOG_CONFIGURATION);
		rc = system(command);
	}
}

/* Verify that logDataGroupname value is allowed to delete */
void verLogDataGroupName(void)
{
	int rc;
	char command[256];
	char logGroupName[256];
	bool logGroupEmpty = true;

	/* Get current value of logDataGroupname attribute */
	rc = get_attr_value(&configurationObject, "logDataGroupname", logGroupName);
	if (rc != -1) {
		logGroupEmpty = false;
	}

	sprintf(command, "immcfg -a logDataGroupname= %s",
		LOGTST_IMM_LOG_CONFIGURATION);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);

	if ((logGroupEmpty == false) && (WEXITSTATUS(rc) == 0)) {
		/* The value is deleted. Restore it */
		sprintf(command, "immcfg -a logDataGroupname=\"%s\" %s",
			logGroupName, LOGTST_IMM_LOG_CONFIGURATION);
		rc = system(command);
	}
}

/* One CCB with valid attributes, Allowed */
void verCCBWithValidValues(void)
{
	int rc;
	char command[256];
	uint32_t timeout = 500;
	uint32_t maxRec = 0;

	/* Get current values of ttributes */
	(void)get_attr_value(&configurationObject, "logFileIoTimeout", &timeout);
	(void)get_attr_value(&configurationObject, "logMaxLogrecsize", &maxRec);

	sprintf(command, "immcfg -a logFileIoTimeout=600 -a logMaxLogrecsize=2000 %s",
		LOGTST_IMM_LOG_CONFIGURATION);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);

	if (WEXITSTATUS(rc) == 0) {
		/* Restore the values */
		sprintf(command, "immcfg -a logFileIoTimeout=%d -a logMaxLogrecsize=%d %s",
			timeout,
			maxRec,
			LOGTST_IMM_LOG_CONFIGURATION);
		rc = system(command);
	}
}

/* One CCB with many valid attributes, but one of them invalid. Not Allowed */
void verCCBWithInvalidValues(void)
{
	int rc;
	char command[256];
	uint32_t timeout = 500;
	uint32_t *tmp_to = &timeout;
	uint32_t maxRec = 0;
	uint32_t *tmp_max = &maxRec;

	/* Get current values of ttributes */
	rc = get_attr_value(&configurationObject, "logFileIoTimeout", &timeout);
	if (rc != -1) {
		timeout = *tmp_to;
	}

	rc = get_attr_value(&configurationObject, "logMaxLogrecsize", &maxRec);
	if (rc != -1) {
		maxRec = *tmp_max;
	}

	/* invalid value to logMaxLogrecsize */
	sprintf(command, "immcfg -a logFileIoTimeout=600 -a logMaxLogrecsize=800000 %s "
		" 2> /dev/null ", LOGTST_IMM_LOG_CONFIGURATION);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);

	if (WEXITSTATUS(rc) == 0) {
		/* Restore the values */
		sprintf(command, "immcfg -a logFileIoTimeout=%d -a logMaxLogrecsize=%d %s",
			timeout,
			maxRec,
			LOGTST_IMM_LOG_CONFIGURATION);
		rc = system(command);
	}
}

/**
 * Add test case for ticket #1420
 * logsv crashed if performing admin op on configurable obj class.
 * Test steps:
 * 1. Create an configurable obj class for app stream.
 * 2. Perform admin op - changing saLogStreamSeverityFilter with valid value.
 * 3. Verify that it fails to perform that op.
 * 4. Delete previous created class.
 */
void verAdminOpOnConfClass(void)
{
	int rc;
	char command[256];

	/* Create an configurable obj class */
	strcpy(command, "immcfg -c SaLogStreamConfig safLgStrCfg=adminOp,safApp=safLogService "
	       "-a saLogStreamFileName=adminOp -a saLogStreamPathName=. -a saLogStreamMaxFilesRotated=4 "
	       "2> /dev/null");
	rc = system(command);

	if (WEXITSTATUS(rc) == 1) {
		/* Failed to perfom command. Report test failed and return. */
		fprintf(stderr, "Failed to perform command = %s \n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		return;
	}

	/* Perfom admin operation on the configurable obj class */
	strcpy(command, "immadm -o 1 -p saLogStreamSeverityFilter:SA_UINT32_T:100 "
	       "safLgStrCfg=adminOp,safApp=safLogService 2> /dev/null");
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);

	/* Delete the created class */
	strcpy(command, "immcfg -d  safLgStrCfg=adminOp,safApp=safLogService");
	rc = system(command);
}

/* Add test case to verify #1466 */

/**
 * saLogStreamFixedLogRecordSize is in range [150 - MAX_RECSIZE] if not 0.
 * Verify that setting to value less than 150 is not allowed.
 */
void verFixLogRec_Min(void)
{
	int rc;
	char command[256];

	sprintf(command, "immcfg -a saLogStreamFixedLogRecordSize=149 %s 2> /dev/null",
		SA_LOG_STREAM_ALARM);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}


/**
 * Test cases to verify #1493
 */

/* Verify that it is not allowed to pass a string value to
 * saLogStreamFileName which its length is larger than 218.
 */
void verFilenameLength_01(void)
{
	int rc;
	char command[500];
	char fileName[220];

	/* Create a string with 218 characters */
	memset(fileName, 'A', sizeof(fileName));
	fileName[219] = '\0';

	/* Create an configurable obj class with invalid filename length */
	sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=TestLength "
		"-a saLogStreamFileName=\"%s\" -a saLogStreamPathName=. "
		"2> /dev/null", fileName);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/* Modify an existing value */
void verFilenameLength_02(void)
{
	int rc;
	char command[500];
	char fileName[220];

	/* Create a string with 218 characters */
	memset(fileName, 'A', sizeof(fileName));
	fileName[219] = '\0';

	/* Create an configurable obj class with invalid filename length */
	sprintf(command, "immcfg -a saLogStreamFileName=\"%s\" -a saLogStreamPathName=. %s "
		"2> /dev/null", fileName, SA_LOG_STREAM_ALARM);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Add test case for ticket #1421
 * logsv not verify if created/modified log file name is valid.
 * Test steps:
 * 1. Get current saLogStreamFileName
 * 2. Iterate all not-allowed characters
 * 3. Update saLogStreamFileName containing one of that character
 * 4. Verify that it fails to perform that command.
 * 5. Restore to previous value.
 */
void verLogFileName(void)
{
	int rc;
	char command[256];
	/* const char *str = "|;,!@#$()<>/\\\"'`~{}[]+&^?*%/"; */

	/* int i = 0; */
	/* for (; i < strlen(str); i++) { */
	/* 	if (str[i] != '\'') { */
	/* 		sprintf(command, "immcfg -a saLogStreamFileName='tmp_%c' %s 2> /dev/null", */
	/* 				str[i], alarmStreamName.value); */
	/* 	} else { */
	/* 		sprintf(command, "immcfg -a saLogStreamFileName=\"tmp_'\" %s 2> /dev/null", */
	/* 				alarmStreamName.value); */
	/* 	} */
	/* 	rc = system(command); */
	/* 	if (WEXITSTATUS(rc) == 0) { */
	/* 		fprintf(stderr, "Failed - command = %s\n", command); */
	/* 		break; */
	/* 	} */
	/* } */

	/* Invalid filename with forward slash in */
	sprintf(command, "immcfg -a saLogStreamFileName='invalidFile/Name' %s 2> /dev/null",
		SA_LOG_STREAM_ALARM);
	rc = system(command);

	rc_validate(WEXITSTATUS(rc), 1);
}

/**
 * Add test case for ticket #1619
 * Verify three digits millisecond token writes to log file exactly
 *
 * Steps:
 * 1. Change log file format configure for System and Notification stream.
 * 2. Write 20 log records continuously to to System and Notification log stream.
 * 3. Manually check millisecond token display in System and Notification log file.
 *    Make sure that three digits shows in millisecond token increases continuously.
 */
void verMiliToken(void)
{
	SaAisErrorT rc;
	char command[1000];
	char defaultForSysExp[256];
	char defaultForNotifExp[256];
	const char* forSysfExp = "@Cr @Ch:@Cn:@Cs @Ck @Cm/@Cd/@CY @Sv @Sl \"@Cb\"";
	const char* forNotifExp = "@Cr @Nh:@Nn:@Ns @Nk @Nt @No \"@Cb\"";

	/* Get and save saLogStreamLogFileFormat */
	rc = get_attr_value(&systemStreamName, "saLogStreamLogFileFormat",
	                    defaultForSysExp);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		strncpy(defaultForSysExp, DEFAULT_FORMAT_EXPRESSION, 256);
	}

	rc = get_attr_value(&notificationStreamName, "saLogStreamLogFileFormat",
	                    defaultForNotifExp);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		strncpy(defaultForNotifExp, DEFAULT_ALM_NOT_FORMAT_EXP, 256);
	}

	/* Change log file format configure for Notification stream */
	sprintf(command,"immcfg %s -a saLogStreamLogFileFormat='%s'",
		SA_LOG_STREAM_NOTIFICATION, forNotifExp);
	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Fail to perform the command. Report test case failed */
		fprintf(stderr, "Failed to perform command = %s", command);
		rc_validate(WEXITSTATUS(rc), 0);
		return;
	}

	/* Change log file format configure for System stream */
	sprintf(command,"immcfg %s -a saLogStreamLogFileFormat='%s'",
		SA_LOG_STREAM_SYSTEM, forSysfExp);
	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Fail to perform the command. Report test case failed */
		fprintf(stderr, "Failed to perform command = %s", command);
		rc_validate(WEXITSTATUS(rc), 0);
		goto restore_notif;
	}

	/* Write 20 log records to System and Notification stream */
	for (int i = 0; i < 20; i ++) {
		sprintf(command, "saflogger -y  \"Test for ticket #1619\"");
		rc = system(command);
		if (WEXITSTATUS(rc)) {
			/* Fail to perform command. Report test case failed. */
			fprintf(stderr, "Failed to perform command = %s\n", command);
			rc_validate(WEXITSTATUS(rc), 0);
			goto restore_sys;
		}

		sprintf(command, "saflogger -n  \"Test for ticket #1619\"");
		rc = system(command);
		if (WEXITSTATUS(rc)) {
			/* Fail to perform command. Report test case failed. */
			fprintf(stderr, "Failed to perform command = %s\n", command);
			rc_validate(WEXITSTATUS(rc), 0);
			goto restore_sys;
		}
	}

	rc_validate(WEXITSTATUS(rc), 0);

restore_sys:
	/* Reset saLogStreamLogFileFormat for System and Notification stream */
	sprintf(command,"immcfg %s -a saLogStreamLogFileFormat='%s' 2> /dev/null",
		SA_LOG_STREAM_SYSTEM, defaultForSysExp);
	rc = system(command);

restore_notif:
	sprintf(command,"immcfg %s -a saLogStreamLogFileFormat='%s' 2> /dev/null",
		SA_LOG_STREAM_NOTIFICATION, defaultForNotifExp);
	rc = system(command);
}

/****************************************
 * These below function(s) are used to test added new tokens @Cq and @Cp:
 * @Cp: represents network name.
 * @Cq: represent node name.
 *
 * The test case will do following sequences:
 *
 * 1. Backup the current `opensafNetworkName` attribute.
 * 2. Modify the attribute to a specific name.
 * 3. Backup the current `saLogStreamLogFileFormat`
 * 4. Modify the attribute by adding new @Cq, @Cp tokens
 * 5. Send a log stream
 * 6. Check content of log file (*.log) cotaining the node name & network name.
 * 7. Restore the modified attributes to previous values
 *
 ***************************************/

/* Commands to search for specific pattern on files having 01 last minute
 * (?) accessed, but only openning file.
 *
 * The flow of execution as below:
 *
 * 1. Find (linux `find` command) all files in the directory specificed by
 * %s (e.g: root_log_path) and subfolders which having 01 last minute accessed
 * (e.g: read/write access) or lesser.
 *
 * 2. With `egrep``, it helps to filter just openning log files
 * in list of files found - which have correct format
 * <logfilename>_<yyyymmdd>_<hhmmss>.log
 *
 * 3. With `egrep`, search content in the list of right-format
 * found files for a specific pattern.
 * (e.g: timezone format = [+-][0-9]{4}, millisecond format = [0-9]{3})
 *
 * 4. Don't show found result (if tester wants to see the search result,
 * comment final line and its above slash)
 *
 * Note: If found pattern, exit status of these pipes will be
 * EXIT_SUCCESS(0), otherwise EXIT_FAILURE(1)/non-zero(?)
 */
#define VERIFY_CMD_1 "find %s -type f -mmin -1"         \
	" | egrep \".*(%s_[0-9]{8}_[0-9]{6})\\.log$\""	\
	" | xargs egrep  \" %s \""			\
	" 1> /dev/null"

/* Search pattern for node name pattern, including prefix for testing */
#define NODENAME_PATTERN "prefNd_[a-zA-Z0-9,_,-]*"
/* Search pattern for network name pattern, including prefix for testing */
#define NWNAME_PATTERN "prefNw_Network A"

/**
 * CCB Object Modify saLogStreamLogFileFormat of alarm log stream
 * by adding node name token and check if log file are reflected correctly.
 */
void verNodeName_01(void)
{
	int rc;
	char command[MAX_DATA];
	char preLogStrFileFmt[MAX_DATA];

	/* Enable node name token - @Cq */
	const char* modLogStrFileFmt = "@Cr @Ct prefNd_@Cq @Nz @Ne6 @No30 @Ng30 @Cb";

	/* Get current value of the attribute, use default value if failed */
	rc = get_attr_value(&alarmStreamName, "saLogStreamLogFileFormat",
			    preLogStrFileFmt);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		strncpy(preLogStrFileFmt, DEFAULT_ALM_NOT_FORMAT_EXP, MAX_DATA);
	}

	/* Modify the attribute */
	sprintf(command, IMMCFG_CMD, modLogStrFileFmt, SA_LOG_STREAM_ALARM);
	rc = system(command);
	if (WEXITSTATUS(rc) == 0) {
		/* Send an alarm to alarm log stream */
		rc = system("saflogger -l");
		if (WEXITSTATUS(rc)) {
			/* Failed to send the alarm to log stream */
			fprintf(stderr, "Failed to invoke saflogger -a\n");
			rc_validate(WEXITSTATUS(rc), 0);
			/* Exit the test case */
			return;
		}
		/* Verify the content of log file if it is reflected with right format */
		sprintf(command, VERIFY_CMD_1, log_root_path, ALM_LOG_FILE, NODENAME_PATTERN);
		rc = system(command);
		if (rc == -1) {
			fprintf(stderr, "Failed to execute command (%s) on shell\n",
				VERIFY_CMD_1);
			test_validate(rc, 0);
			/* Exit the test case */
			return;
		}

		/* Command is executed succesfully on the shell, verify the result */
		rc_validate(WEXITSTATUS(rc), 0);

		/* Restore the attribute to previous value */
		sprintf(command, IMMCFG_CMD, preLogStrFileFmt, SA_LOG_STREAM_ALARM);
		rc = system(command);
		if (WEXITSTATUS(rc) != 0) {
			/* Failed to restore to privous value, print message */
			fprintf(stderr, "Failed to restore the attribute to previous value\n");
		}
	} else {
		rc_validate(WEXITSTATUS(rc), 0);
	}
}

/**
 * CCB Object Modify saLogStreamLogFileFormat of alarm log stream
 * by adding node name token and check if log file are reflected correctly.
 */
void verNetworkName_01(void)
{
	int rc;
	char command[MAX_DATA];
	char preLogStrFileFmt[MAX_DATA];
	char networkName[MAX_DATA];

	/* Enable network name token - @Cp */
	const char* modLogStrFileFmt = "@Cr @Ct prefNw_@Cp @Nz @Ne6 @No30 @Ng30 @Cb";

	/* Get current value of the attribute, use default value if failed */
	rc = get_attr_value(&alarmStreamName, "saLogStreamLogFileFormat",
			    preLogStrFileFmt);
	if (rc == -1) {
		/* Failed, use default one */
		fprintf(stderr, "Failed to get attribute value from IMM\n");
		strncpy(preLogStrFileFmt, DEFAULT_ALM_NOT_FORMAT_EXP, MAX_DATA);
	}

	/* Get current value of the attribute, use default value if failed */
	rc = get_attr_value(&globalConfig, "opensafNetworkName",
			    networkName);
	if (rc == -1) {
		/* Failed, use default one */
		strncpy(networkName, "", MAX_DATA);
	}

	/* Modify attribute network name */
	sprintf(command, "immcfg -a opensafNetworkName=\"Network A\" %s",
		LOGTST_IMM_LOG_GCFG);
	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Failed to send the alarm to log stream */
		fprintf(stderr, "Failed to invoke command %s\n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		/* Exit the test case */
		return;
	}

	/* Modify the logfileFmt attribute */
	sprintf(command, IMMCFG_CMD, modLogStrFileFmt, SA_LOG_STREAM_ALARM);
	rc = system(command);
	if (WEXITSTATUS(rc) == 0) {
		/* Send an alarm to alarm log stream */
		rc = system("saflogger -l");
		if (WEXITSTATUS(rc)) {
			/* Failed to send the alarm to log stream */
			fprintf(stderr, "Failed to invoke saflogger -a\n");
			rc_validate(WEXITSTATUS(rc), 0);
			/* Exit the test case */
			return;
		}

		/* Verify the content of log file if it is reflected with right format */
		sprintf(command, VERIFY_CMD_1, log_root_path, ALM_LOG_FILE, NWNAME_PATTERN);
		rc = system(command);
		if (rc == -1) {
			fprintf(stderr, "Failed to execute command (%s) on shell\n",
				VERIFY_CMD_1);
			test_validate(rc, 0);
			/* Exit the test case */
			return;
		}
		/* Command is executed succesfully on the shell, verify the result */
		rc_validate(WEXITSTATUS(rc), 0);

		/* Restore the attribute to previous value */
		sprintf(command, IMMCFG_CMD, preLogStrFileFmt, SA_LOG_STREAM_ALARM);
		rc = system(command);
		if (WEXITSTATUS(rc) != 0) {
			/* Failed to restore to privous value, print message */
			fprintf(stderr, "Failed to restore logFileFmt\n");
		}

		sprintf(command, "immcfg -a opensafNetworkName=%s %s",
			networkName, LOGTST_IMM_LOG_GCFG);
		rc = system(command);
		if (WEXITSTATUS(rc) != 0) {
			/* Failed to restore to privous value, print message */
			fprintf(stderr, "Failed to restore opensafNetworkName\n");
		}
	} else {
		rc_validate(WEXITSTATUS(rc), 0);
	}
}


/*
 * Add test case for ticket #1446
 * Verify that invalid Object if number of app streams has reached the limitation
 *
 */
void verStrLimit(void)
{
	SaAisErrorT rc;
	char command[1000];
	uint32_t curAppCount, maxAppStream = 64;
	uint32_t *tmp_to = &maxAppStream;
	int num = 0;
	FILE *fp = NULL;
	char curAppCount_c[10];

	/* Get current max app stream values of attributes */
	rc = get_attr_value(&configurationObject, "logMaxApplicationStreams",
	                    tmp_to);
	if (rc != -1) {
		maxAppStream = *tmp_to;
	}

	/*  Get current app stream */
	sprintf(command,"(immfind -c SaLogStreamConfig && immfind -c SaLogStream) | wc -l");
	fp = popen(command, "r");

	while (fgets(curAppCount_c, sizeof(curAppCount_c) - 1, fp) != NULL) {};
	pclose(fp);
	/* Convert chars to number */
	curAppCount = atoi(curAppCount_c) - 3;

	for (num = curAppCount; num < maxAppStream; num++) {
		/* Create configurable app stream */
		sprintf(command,"immcfg -c SaLogStreamConfig  safLgStrCfg=test%d"
		        	" -a saLogStreamPathName=."
		        	" -a saLogStreamFileName=test%d",
		        	num, num);
		rc = system(command);
		if (WEXITSTATUS(rc)) {
			/* Fail to perform the command. Report test case failed */
			fprintf(stderr, "Failed to perform command = %s\n", command);
			rc_validate(WEXITSTATUS(rc), 0);
			goto done;
		}
	}

	if (curAppCount >= maxAppStream) {
		num += 1;
	}

	/* Create configurable app stream while number of app stream limitation */
	sprintf(command,"immcfg -c SaLogStreamConfig  safLgStrCfg=test%d"
	        	" -a saLogStreamPathName=."
	        	" -a saLogStreamFileName=test%d 2> /dev/null",
	        	num, num);
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 1);

	if (WEXITSTATUS(rc) == 0) {
		sprintf(command,"immcfg -d safLgStrCfg=test%d 2> /dev/null", num);
		rc = system(command);
	}

done:
	/* Delete app stream  */
	for (num = curAppCount; num < maxAppStream; num++) {
		sprintf(command,"immcfg -d safLgStrCfg=test%d 2> /dev/null", num);
		rc = system(command);
	}
}

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
	test_case_add(4, saLogOi_30, "CCB Object Modify, saLogStreamFixedLogRecordSize=150, strC");
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
	test_case_add(4, modStrLogFileFmt_01, "CCB Object Modify, saLogstreamLogFileFormat, timezone token (@Nz)");
	test_case_add(4, modStrLogFileFmt_02, "CCB Object Modify, saLogstreamLogFileFormat, millisecond token (@Nk)");
	test_case_add(4, modStrLogFileFmt_03, "CCB Object Modify, saLogstreamLogFileFormat, timezone token (@Cz)");
	test_case_add(4, modStrLogFileFmt_04, "CCB Object Modify, saLogstreamLogFileFormat, millisecond token (@Ck)");
	test_case_add(4, modStrLogFileFmt_05, "CCB Object Modify, saLogstreamLogFileFormat, timezone & millisecond token (@Cz @Ck)");
	test_case_add(4, verNodeName_01, "CCB Object Modify, saLogStreamLogFileFormat, node name token (@Cq)");
	test_case_add(4, verNetworkName_01, "CCB Object Modify, saLogStreamLogFileFormat, network name token (@Cp)");
	test_case_add(4, verDefaultLogFileFmt, "Application stream with default log file format");

	/* Configuration object */
	test_suite_add(5, "LOG OI tests, Service configuration object");
	test_case_add(5, saLogOi_52, "CCB Object Modify, root directory. Path does not exist. Not allowed");
	test_case_add(5, saLogOi_48, "CCB Object Modify, root directory. Path exist. OK");
	test_case_add(5, saLogOi_79, "CCB Object Modify, data group. Group does not exist. Not allowed");
	test_case_add(5, saLogOi_80, "CCB Object Modify, data group. Group exists. OK");
	test_case_add(5, saLogOi_81, "CCB Object Modify, delete data group. OK");
	test_case_add(5, saLogOi_54, "CCB Object Modify, logStreamSystemHighLimit > logStreamSystemLowLimit. OK");
	test_case_add(5, saLogOi_55, "CCB Object Modify, logStreamSystemHighLimit = logStreamSystemLowLimit, != 0. Ok");
	test_case_add(5, saLogOi_56, "CCB Object Modify, logStreamSystemHighLimit < logStreamSystemLowLimit. Error");
	test_case_add(5, saLogOi_57, "CCB Object Modify, logStreamSystemHighLimit = logStreamSystemLowLimit = 0. OK");
	test_case_add(5, saLogOi_58, "CCB Object Modify, logStreamAppHighLimit > logStreamAppLowLimit. OK");
	test_case_add(5, saLogOi_59, "CCB Object Modify, logStreamAppHighLimit = logStreamAppLowLimit, != 0. Ok");
	test_case_add(5, saLogOi_60, "CCB Object Modify, logStreamAppHighLimit < logStreamAppLowLimit. Error");
	test_case_add(5, saLogOi_61, "CCB Object Modify, logStreamAppHighLimit = logStreamAppLowLimit = 0. OK");
	test_case_add(5, saLogOi_62, "CCB Object Modify, logMaxApplicationStreams. Not allowed");
	test_case_add(5, saLogOi_64, "CCB Object Modify, logFileSysConfig. Not allowed");
	test_case_add(5, verLogFileName, "CCB Object Modify, saLogStreamFileName with special character. ER");

	/* Add test cases to test #1288 */
	test_case_add(5, verLogFileIoTimeout, "CCB Object Modify: logFileIoTimeout is in range [500 - 5000], OK");
	test_case_add(5, verLogFileIoTimeout_Err_01, "CCB Object Modify: logFileIoTimeout < 500, ERR");
	test_case_add(5, verLogFileIoTimeout_Err_02, "CCB Object Modify: logFileIoTimeout > 5000, ERR");
	test_case_add(5, verLogMaxLogrecsize, "CCB Object Modify: logMaxLogrecsize is in range [150 - 65535], OK");
	test_case_add(5, verLogMaxLogrecsize_dep, "CCB Object Modify: logMaxLogrecsize dependencies, OK");
	test_case_add(5, verLogMaxLogrecsize_Err_01, "CCB Object Modify: logMaxLogrecsize < 150, ERR");
	test_case_add(5, verLogMaxLogrecsize_Err_02, "CCB Object Modify: logMaxLogrecsize > 65535, ERR");

	/* Add test cases to test #1490 */
	test_case_add(5, verLogStreamFileFormat, "CCB Object Modify: delete logStreamFileFormat, OK");
	test_case_add(5, verLogDataGroupName, "CCB Object Modify: delete logDataGroupname, OK");
	test_case_add(5, verCCBWithValidValues, "CCB Object Modify many attributes with valid values, OK");
	test_case_add(5, verCCBWithInvalidValues, "CCB Object Modify many attributes with one invalid values, ERR");
	/* Add test case to test #1446 */
	test_case_add(5, verStrLimit, "CCB Object Create: invalid Object if number of app streams has reached the limitation, ERR");

	/* Stream configuration object */
	/* Tests for create */
	test_suite_add(6, "LOG OI tests, Stream configuration object attribute validation");
	test_case_add(6, saLogOi_65, "Create: saLogStreamSeverityFilter <= 0x7f, Ok");
	test_case_add(6, saLogOi_66, "Create: saLogStreamSeverityFilter > 0x7f, ERR");
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
	test_case_add(6, verFilenameLength_01, "Create: saLogStreamFileName > 218 characters, ERR");
	test_case_add(6, verMaxFilesRotated, "Create: saLogStreamMaxFilesRotated = 0, ERR");
	test_case_add(6, verAdminOpOnConfClass, "Perform admin op on configurable obj class, ERR");

	/* Tests for modify */
	test_case_add(6, saLogOi_100, "Modify: saLogStreamSeverityFilter <= 0x7f, Ok");
	test_case_add(6, saLogOi_101, "Modify: saLogStreamSeverityFilter > 0x7f, ERR");
	test_case_add(6, saLogOi_102, "Modify: saLogStreamPathName \"Test/\" (Not possible to modify)");
	test_case_add(6, saLogOi_103, "Modify: saLogStreamFileName, Name and path already used by an existing stream, ERR");
	test_case_add(6, saLogOi_104, "Modify: saLogStreamFileName, Name exist but in other path, Ok");
	test_case_add(6, saLogOi_105, "Modify: saLogStreamFileName, New name, Ok");
	test_case_add(6, saLogOi_106, "Modify: saLogStreamMaxLogFileSize > logMaxLogrecsize, Ok");
	test_case_add(6, saLogOi_107, "Modify: saLogStreamMaxLogFileSize == logMaxLogrecsize, ERR");
	test_case_add(6, saLogOi_108, "Modify: saLogStreamMaxLogFileSize < logMaxLogrecsize, ERR");
	test_case_add(6, verFixLogRec_MaxFileSize, "Modify: saLogStreamMaxLogFileSize > saLogStreamFixedLogRecordSize, OK");
	test_case_add(6, verFixLogRec_MaxFileSize_Err, "Modify: saLogStreamMaxLogFileSize <= saLogStreamFixedLogRecordSize, ERR");
	test_case_add(6, verMaxLogFileSize_Err, "Modify: saLogStreamMaxLogFileSize == 0, ERR");
	test_case_add(6, saLogOi_109, "Modify: saLogStreamFixedLogRecordSize < logMaxLogrecsize, Ok");
	test_case_add(6, saLogOi_110, "Modify: saLogStreamFixedLogRecordSize == 0, Ok");
	test_case_add(6, saLogOi_111, "Modify: saLogStreamFixedLogRecordSize == logMaxLogrecsize, Ok");
	test_case_add(6, saLogOi_112, "Modify: saLogStreamFixedLogRecordSize > logMaxLogrecsize, ERR");
	test_case_add(6, verFixLogRec_Min, "Modify: saLogStreamFixedLogRecordSize < 150, ERR");
	test_case_add(6, saLogOi_113, "Modify: saLogStreamMaxFilesRotated < 128, Ok");
	test_case_add(6, saLogOi_114, "Modify: saLogStreamMaxFilesRotated > 128, ERR");
	test_case_add(6, saLogOi_115, "Modify: saLogStreamMaxFilesRotated == 128, ERR");
	test_case_add(6, verFilenameLength_02, "Modify: saLogStreamFileName > 218 characters, ERR");

	/* Add test cases to test #1288 */
	test_case_add(6, verMaxLogRecord_01, "Modify: saLogStreamFixedLogRecordSize == 0, write a record = 65535 bytes, OK");
	test_case_add(6, verMaxLogRecord_02, "Modify: saLogStreamFixedLogRecordSize == 65535, Write a record = 65535 bytes with special characters, OK");
	test_case_add(6, verMiliToken, "Write 20 log records to System/Notification stream. MANUALLY verify millisecond increased continuously");
}
