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

/**
 * CCB Object Modify saLogStreamFileName
 */
void saLogOi_01(void)
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a saLogStreamFileName=notification %s",
        SA_LOG_STREAM_NOTIFICATION);
    rc = system(command);
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
 * CCB Object Create, strA
 */
void saLogOi_23()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=strA,safApp=safLogService "
			"-a saLogStreamFileName=strA -a saLogStreamPathName=strAdir");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Create, strB
 */
void saLogOi_24()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=strB,safApp=safLogService "
			"-a saLogStreamFileName=strB -a saLogStreamPathName=strBdir");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Create, strC
 */
void saLogOi_25()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=strC,safApp=safLogService "
			"-a saLogStreamFileName=strC -a saLogStreamPathName=strCdir");
    rc = system(command);
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
 * CCB Object Modify, root directory
 */
void saLogOi_48()
{
    int rc;
    char command[256];
	
    sprintf(command, "mkdir -p %s/xxtest",log_root_path);
    rc = system(command);
    sprintf(command, "immcfg -a logRootDirectory=%s/xxtest logConfig=1,safApp=safLogService",log_root_path);
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * CCB Object Modify, logStreamSystemHighLimit, not allowed
 */
void saLogOi_49()
{
    int rc;
    char command[256];

    sprintf(command, "immcfg -a logStreamSystemHighLimit=90 logConfig=1,safApp=safLogService 2> /dev/null");
    rc = system(command);
    rc_validate(WEXITSTATUS(rc), 1);
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
    test_case_add(4, saLogOi_48, "CCB Object Modify, root directory");
    test_case_add(4, saLogOi_49, "CCB Object Modify, logStreamSystemHighLimit, not allowed");
    test_case_add(4, saLogOi_40, "CCB Object Delete, strA");
    test_case_add(4, saLogOi_50, "saflogtest, writing to appTest");
    test_case_add(4, saLogOi_51, "saflogtest, writing to saLogApplication1, severity filtering check");
}

