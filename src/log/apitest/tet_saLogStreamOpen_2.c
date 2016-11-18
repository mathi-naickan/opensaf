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

#include "logtest.h"

extern struct LogProfile logProfile;

static SaLogFileCreateAttributesT_2 appStream1LogFileCreateAttributes =
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

static void init_file_create_attributes(void)
{
    appStream1LogFileCreateAttributes.logFilePathName = DEFAULT_APP_FILE_PATH_NAME;
    appStream1LogFileCreateAttributes.logFileName = DEFAULT_APP_FILE_NAME;
    appStream1LogFileCreateAttributes.maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
    appStream1LogFileCreateAttributes.maxLogRecordSize = DEFAULT_APP_LOG_REC_SIZE;
    appStream1LogFileCreateAttributes.haProperty = SA_TRUE;
    appStream1LogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
    appStream1LogFileCreateAttributes.maxFilesRotated = DEFAULT_MAX_FILE_ROTATED;
    appStream1LogFileCreateAttributes.logFileFmt = DEFAULT_FORMAT_EXPRESSION;
}

void saLogStreamOpen_2_01(void)
{
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&systemStreamName);
    logFinalize();
    test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_2_02(void)
{
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&notificationStreamName);
    logFinalize();
    test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_2_03(void)
{
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logStreamOpen(&alarmStreamName);
    logFinalize();
    test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_2_04(void)
{
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    logFinalize();
    test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_2_05(void)
{
//    SaLogStreamHandleT logStreamHandle1, logStreamHandle2;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    if (rc != SA_AIS_OK)
        goto done;
    /* Reopen with NULL create attrs and CREATE flag not set */
    rc = logStreamOpen(&app1StreamName);

done:
    logFinalize();
    test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_2_06(void)
{
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
			   SA_TIME_ONE_SECOND, NULL);
    logFinalize();
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saLogStreamOpen_2_08(void)
{
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = saLogStreamOpen_2(logHandle, NULL, NULL, 0,
			   SA_TIME_ONE_SECOND, &logStreamHandle);
    logFinalize();
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saLogStreamOpen_2_09(void)
{
    init_file_create_attributes();
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    appStream1LogFileCreateAttributes.logFileName = "changed_filename";
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    test_validate(rc, SA_AIS_ERR_EXIST);

done:
    logFinalize();

}

void saLogStreamOpen_2_10(void)
{
    init_file_create_attributes();
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    appStream1LogFileCreateAttributes.logFilePathName = "/new_file/path";
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    test_validate(rc, SA_AIS_ERR_EXIST);

done:
    logFinalize();
}

void saLogStreamOpen_2_11(void)
{
    init_file_create_attributes();
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    appStream1LogFileCreateAttributes.logFileFmt = "@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Sl @Sv\"@Cb\"";
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    test_validate(rc, SA_AIS_ERR_EXIST);

done:
    logFinalize();
}

void saLogStreamOpen_2_12(void)
{
    init_file_create_attributes();
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    appStream1LogFileCreateAttributes.maxLogFileSize++;
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    test_validate(rc, SA_AIS_ERR_EXIST);

done:
    logFinalize();
}

void saLogStreamOpen_2_13(void)
{
    init_file_create_attributes();
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    appStream1LogFileCreateAttributes.maxLogRecordSize++;
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    test_validate(rc, SA_AIS_ERR_EXIST);

done:
    logFinalize();
}

void saLogStreamOpen_2_14(void)
{
    init_file_create_attributes();
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    appStream1LogFileCreateAttributes.maxFilesRotated++;
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    test_validate(rc, SA_AIS_ERR_EXIST);

done:
    logFinalize();
}

void saLogStreamOpen_2_15(void)
{
    init_file_create_attributes();
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    if (rc != SA_AIS_OK)
        goto done;

    appStream1LogFileCreateAttributes.haProperty = SA_FALSE;
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
	/* haProperty value is not checked by logsv */

done:
    logFinalize();
    test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_2_16(void)
{
    SaNameT streamName;
    SaConstStringT data = "safLgStr=saLogStreamOpen_2_16";
    saAisNameLend(data, &streamName);

    init_file_create_attributes();
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    appStream1LogFileCreateAttributes.logFileFmt = NULL;
    appStream1LogFileCreateAttributes.logFileName = (SaStringT) __FUNCTION__;
    rc = logAppStreamOpen(&streamName, &appStream1LogFileCreateAttributes);

    logFinalize();
    test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_2_17(void)
{
    SaNameT streamName;
    SaConstStringT data = "safLgStr=saLogStreamOpen_2_17";
    saAisNameLend(data, &streamName);

    init_file_create_attributes();
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    appStream1LogFileCreateAttributes.logFileFmt = NULL;
    appStream1LogFileCreateAttributes.logFileName = (SaStringT) __FUNCTION__;
    rc = logAppStreamOpen(&streamName, &appStream1LogFileCreateAttributes);
    if (rc != SA_AIS_OK)
        goto done;
    rc = logAppStreamOpen(&streamName, &appStream1LogFileCreateAttributes);

done:
    logFinalize();
    test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_2_18(void)
{
    init_file_create_attributes();
    appStream1LogFileCreateAttributes.logFileName = (SaStringT) __FUNCTION__;
    appStream1LogFileCreateAttributes.logFilePathName = NULL;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    logFinalize();
    test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_2_19(void)
{
    init_file_create_attributes();
    appStream1LogFileCreateAttributes.logFileName = (SaStringT) __FUNCTION__;
    appStream1LogFileCreateAttributes.logFilePathName = ".";

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
    logFinalize();
    test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_2_20(void)
{
    init_file_create_attributes();
    appStream1LogFileCreateAttributes.logFileName = (SaStringT) __FUNCTION__;
    appStream1LogFileCreateAttributes.logFileFmt = "@Cr @Ch:@Cn:@Cs @Ni @Cm/@Cd/@CY @Sv @Sl \"@Cb\"";

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = saLogStreamOpen_2(logHandle, &app1StreamName, &appStream1LogFileCreateAttributes,
			   SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
    logFinalize();
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saLogStreamOpen_2_21(void)
{
    init_file_create_attributes();
    appStream1LogFileCreateAttributes.logFileName = (SaStringT) __FUNCTION__;
    appStream1LogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_HALT;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = saLogStreamOpen_2(logHandle, &app1StreamName, &appStream1LogFileCreateAttributes,
			   SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
    if (rc != SA_AIS_ERR_NOT_SUPPORTED)
        goto done;

    appStream1LogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_WRAP;
    rc = saLogStreamOpen_2(logHandle, &app1StreamName, &appStream1LogFileCreateAttributes,
			   SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);

done:
    logFinalize();
    test_validate(rc, SA_AIS_ERR_NOT_SUPPORTED);
}

void saLogStreamOpen_2_22(void)
{
    init_file_create_attributes();
    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }
    rc = saLogStreamOpen_2(logHandle, &app1StreamName, NULL,
			   0, SA_TIME_ONE_SECOND, &logStreamHandle);
    logFinalize();
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
}

/**
 * Open with stream name length == 256
 */
void saLogStreamOpen_2_23(void)
{
	strcpy((char*)genLogRecord.logBuffer->logBuf, __FUNCTION__);
	genLogRecord.logBuffer->logBufSize = strlen(__FUNCTION__);
	genLogRecord.logHeader.genericHdr.logSvcUsrName = &saNameT_appstream_name_256;
    
	rc = logInitialize();
	if (rc != SA_AIS_OK) {
		test_validate(rc, SA_AIS_OK);
		return;
	}
	rc = saLogStreamOpen_2(logHandle, &saNameT_appstream_name_256, NULL, 0,
			       SA_TIME_ONE_SECOND, &logStreamHandle);
	logFinalize();

	/* restore genLogRecord */
	genLogRecord.logHeader.genericHdr.logSvcUsrName = &logSvcUsrName;

	/* If tested length is >= 256 the server will stop decoding write message
	 * and the message is ignored. This will result in no answer message from
	 * the server and waiting for message will timeout. When this happen the
	 * saLogStreamOpen_2 returns SA_AIS_ERR_TRY_AGAIN
	 * Note that normally the API is checking length. But if testing with this
	 * check disabled the server shall not crash.
	 */
	test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

/**
 * Added test cases to verify ticket #1399:
 * logsv gets stuck in while loop if setting maxFilesRotated = 0.
 *
 * This test case verifies logsv returns error if
 * setting 0 to saLogStreamMaxFilesRotated attribute.
 */
void saLogStreamOpen_2_46(void)
{
    SaAisErrorT rc = SA_AIS_OK;
    SaNameT logStreamName;

    saAisNameLend(SA_LOG_STREAM_APPLICATION1, &logStreamName);

    SaLogFileCreateAttributesT_2 appLogFileCreateAttributes;

    /* App stream information */
    appLogFileCreateAttributes.logFilePathName = "appstream";
    appLogFileCreateAttributes.logFileName = "appstream";
    appLogFileCreateAttributes.maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
    appLogFileCreateAttributes.maxLogRecordSize = DEFAULT_APP_LOG_REC_SIZE;
    appLogFileCreateAttributes.haProperty = SA_TRUE;
    appLogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
    appLogFileCreateAttributes.maxFilesRotated = 0;
    appLogFileCreateAttributes.logFileFmt = NULL;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }

    rc = saLogSelectionObjectGet(logHandle, &selectionObject);
    if (rc != SA_AIS_OK) {
        fprintf(stderr, "Failed at saLogSelectionObjectGet: %d \n ", (int)rc);
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    rc = saLogStreamOpen_2(logHandle, &logStreamName, &appLogFileCreateAttributes,
			   SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
    logFinalize();
}

/**
 * Verify that setting saLogStreamMaxFilesRotated > 127 is not allowed.
 * Valid range is in [1 - 127]
 */
void saLogStreamOpen_2_47(void)
{
    SaAisErrorT rc = SA_AIS_OK;
    SaNameT logStreamName;
    saAisNameLend(SA_LOG_STREAM_APPLICATION1, &logStreamName);

    SaLogFileCreateAttributesT_2 appLogFileCreateAttributes;

    /* App stream information */
    appLogFileCreateAttributes.logFilePathName = "appstream";
    appLogFileCreateAttributes.logFileName = "appstream";
    appLogFileCreateAttributes.maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
    appLogFileCreateAttributes.maxLogRecordSize = DEFAULT_APP_LOG_REC_SIZE;
    appLogFileCreateAttributes.haProperty = SA_TRUE;
    appLogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
    appLogFileCreateAttributes.maxFilesRotated = 128;
    appLogFileCreateAttributes.logFileFmt = NULL;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }

    rc = saLogSelectionObjectGet(logHandle, &selectionObject);
    if (rc != SA_AIS_OK) {
        fprintf(stderr, "Failed at saLogSelectionObjectGet: %d \n ", (int)rc);
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    rc = saLogStreamOpen_2(logHandle, &logStreamName, &appLogFileCreateAttributes,
			   SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
    logFinalize();
}

/**
 * Add test cases to verify #1466
 * maxLogRecordSize must be in range [150 - MAX_RECSIZE] if not 0.
 */

/**
 * Verify that setting maxLogRecordSize > MAX_RECSIZE (65535) is not allowed.
 */
void verFixLogRec_Max_Err(void)
{
    SaAisErrorT rc = SA_AIS_OK;
    SaNameT logStreamName;
    saAisNameLend(SA_LOG_STREAM_APPLICATION1, &logStreamName);

    SaLogFileCreateAttributesT_2 appLogFileCreateAttributes;

    /* App stream information */
    appLogFileCreateAttributes.logFilePathName = "appstream";
    appLogFileCreateAttributes.logFileName = "appstream";
    appLogFileCreateAttributes.maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
    appLogFileCreateAttributes.maxLogRecordSize = 65535 + 1;
    appLogFileCreateAttributes.haProperty = SA_TRUE;
    appLogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
    appLogFileCreateAttributes.maxFilesRotated = 4;
    appLogFileCreateAttributes.logFileFmt = NULL;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }

    rc = saLogSelectionObjectGet(logHandle, &selectionObject);
    if (rc != SA_AIS_OK) {
        fprintf(stderr, "Failed at saLogSelectionObjectGet: %d \n ", (int)rc);
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    rc = saLogStreamOpen_2(logHandle, &logStreamName, &appLogFileCreateAttributes,
			   SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
    logFinalize();
}


/**
 * Verify that setting maxLogRecordSize > MAX_RECSIZE (65535) is not allowed.
 */
void verFixLogRec_Min_Err(void)
{
    SaAisErrorT rc = SA_AIS_OK;
    SaNameT logStreamName;
    saAisNameLend(SA_LOG_STREAM_APPLICATION1, &logStreamName);

    SaLogFileCreateAttributesT_2 appLogFileCreateAttributes;

    /* App stream information */
    appLogFileCreateAttributes.logFilePathName = "appstream";
    appLogFileCreateAttributes.logFileName = "appstream";
    appLogFileCreateAttributes.maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
    appLogFileCreateAttributes.maxLogRecordSize = 150 - 1;
    appLogFileCreateAttributes.haProperty = SA_TRUE;
    appLogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
    appLogFileCreateAttributes.maxFilesRotated = 4;
    appLogFileCreateAttributes.logFileFmt = NULL;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }

    rc = saLogSelectionObjectGet(logHandle, &selectionObject);
    if (rc != SA_AIS_OK) {
        fprintf(stderr, "Failed at saLogSelectionObjectGet: %d \n ", (int)rc);
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    rc = saLogStreamOpen_2(logHandle, &logStreamName, &appLogFileCreateAttributes,
			   SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
    logFinalize();
}

/**
 * Verify that logFileName length > 218 characters is not allowed.
 */
void saLogStreamOpen_2_48(void)
{
    SaAisErrorT rc = SA_AIS_OK;
    SaNameT logStreamName;
    char fileName[220];

    memset(fileName, 'A', 218);
    fileName[219] = '\0';
    saAisNameLend(SA_LOG_STREAM_APPLICATION1, &logStreamName);

    SaLogFileCreateAttributesT_2 appLogFileCreateAttributes;

    /* App stream information */
    appLogFileCreateAttributes.logFilePathName = "appstream";
    appLogFileCreateAttributes.logFileName = fileName;
    appLogFileCreateAttributes.maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
    appLogFileCreateAttributes.maxLogRecordSize = DEFAULT_APP_LOG_REC_SIZE;
    appLogFileCreateAttributes.haProperty = SA_TRUE;
    appLogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
    appLogFileCreateAttributes.maxFilesRotated = 128;
    appLogFileCreateAttributes.logFileFmt = NULL;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }

    rc = saLogSelectionObjectGet(logHandle, &selectionObject);
    if (rc != SA_AIS_OK) {
        fprintf(stderr, "Failed at saLogSelectionObjectGet: %d \n ", (int)rc);
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    rc = saLogStreamOpen_2(logHandle, &logStreamName, &appLogFileCreateAttributes,
			   SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
    logFinalize();
}


/**
 * Added test case to verify ticket #1421
 * Verify logsv failed to create app stream with invalid file name
 */
void saLogStreamOpen_2_49(void)
{
    SaAisErrorT rc = SA_AIS_OK;
    SaNameT logStreamName;
    saAisNameLend(SA_LOG_STREAM_APPLICATION1, &logStreamName);

    SaLogFileCreateAttributesT_2 appLogFileCreateAttributes;

    /* App stream information */
    appLogFileCreateAttributes.logFilePathName = "appstream";
    appLogFileCreateAttributes.logFileName = "appstream/";
    appLogFileCreateAttributes.maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
    appLogFileCreateAttributes.maxLogRecordSize = DEFAULT_APP_LOG_REC_SIZE;
    appLogFileCreateAttributes.haProperty = SA_TRUE;
    appLogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
    appLogFileCreateAttributes.maxFilesRotated = 4;
    appLogFileCreateAttributes.logFileFmt = NULL;

    rc = logInitialize();
    if (rc != SA_AIS_OK) {
        test_validate(rc, SA_AIS_OK);
        return;
    }

    rc = saLogSelectionObjectGet(logHandle, &selectionObject);
    if (rc != SA_AIS_OK) {
        fprintf(stderr, "Failed at saLogSelectionObjectGet: %d \n", (int)rc);
        test_validate(rc, SA_AIS_OK);
        goto done;
    }

    rc = saLogStreamOpen_2(logHandle, &logStreamName, &appLogFileCreateAttributes,
			   SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

done:
    logFinalize();
}

/*
 * Add test case for ticket #1446
 * Verify that logsv failed to create app stream  if number of app
 * streams has reached the limitation
 *
 */
void saLogStreamOpen_2_50(void)
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

	rc = logInitialize();
	if (rc != SA_AIS_OK) {
		test_validate(rc, SA_AIS_OK);
		goto done;
	}

	rc = saLogStreamOpen_2(logHandle, &app1StreamName, &appStream1LogFileCreateAttributes,
			       SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
	test_validate(rc, SA_AIS_ERR_NO_RESOURCES);

	logFinalize();

done:
	/* Delete app stream  */
	for (num = curAppCount; num < maxAppStream; num++) {
		sprintf(command,"immcfg -d safLgStrCfg=test%d 2> /dev/null", num);
		rc = system(command);
	}
}

void *saLogInitialize_1 (void *arg)
{
        SaAisErrorT *rt = (SaAisErrorT *)arg;
        SaLogStreamHandleT logStreamHandle1;
        SaLogHandleT logHandle1;

        *rt = saLogInitialize(&logHandle1, &logCallbacks, &logVersion);
        unsigned int nTries = 1;
        while (*rt == SA_AIS_ERR_TRY_AGAIN && nTries < 50) {
                usleep(100 * 1000);
                *rt = saLogInitialize(&logHandle1, &logCallbacks, &logVersion);
                nTries++;
        }
        if (*rt != SA_AIS_OK) {
                goto done;
        }

        *rt = saLogStreamOpen_2(logHandle1, &app1StreamName, &appStream1LogFileCreateAttributes,
                                SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle1);
        nTries = 1;
        while (*rt == SA_AIS_ERR_TRY_AGAIN && nTries < 50) {
                usleep(100 * 1000);
                *rt = saLogStreamOpen_2(logHandle1, &app1StreamName, &appStream1LogFileCreateAttributes,
                                        SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle1);
                nTries++;
        }
        if (*rt != SA_AIS_OK) {
                goto done;
        }

        *rt = saLogFinalize(logHandle1);
        nTries = 1;
        while (*rt == SA_AIS_ERR_TRY_AGAIN && nTries < 50) {
                usleep(100 * 1000);
                *rt = saLogFinalize(logHandle1);
                nTries++;
        }
        if (*rt != SA_AIS_OK) {
                goto done;
        }

done:
        pthread_exit(NULL);
}

/*
 * Ticket 1396
 * Verify that logInitialize then logFinalize multiple times OK
 */
void saLogMultipleInitialize(void)
{
	int count = 100;
	SaAisErrorT rc = SA_AIS_OK;

	while (count--) {
		rc = logInitialize();
		if (rc != SA_AIS_OK) {
			break;
		}

		rc = logAppStreamOpen(&app1StreamName, &appStream1LogFileCreateAttributes);
		if (rc != SA_AIS_OK) {
			break;
		}

		rc = logFinalize();
		if (rc != SA_AIS_OK)
			break;
	}
	test_validate(rc, SA_AIS_OK);
}

/*
 * Ticket 1396
 * Verify that logInitialize then logFinalize multiple times in multiple threads OK
 */
void saLogMultiThreadMultiInit(void)
{
	int i, errno, nThreads = 100;
	SaAisErrorT rt[nThreads], rc = SA_AIS_OK;
	pthread_t threads[nThreads];

	logProfile.nTries = 50;
	for (i = 0; i < nThreads; i++) {
		errno = pthread_create(&threads[i], NULL, saLogInitialize_1, (void *) &rt[i]);
		if (errno == EAGAIN) {
			nThreads = i;
			fprintf(stderr, " Insufficient resource or limited system-imposed to create"
					" no more than %d threads \n", nThreads);
			break;
		}
	}

	/* Wait for all threads terminated */
	for (i = 0; i < nThreads; i++) {
		pthread_join(threads[i], NULL);
	}

	for (i = 0; i < nThreads; i++) {
		if (rt[i] != SA_AIS_OK) {
			rc = rt[i];
			break;
		}
	}

	logProfile.nTries = 25;
	test_validate(rc, SA_AIS_OK);
}

//
// This test to verify Open call gets BAD_OPERATION or not during si-swap
//
void saLogStreamOpen_BadOp(void)
{
	SaAisErrorT rc;
	char command[1000];
	int loop;

	cond_check();
	rc = saLogInitialize(&logHandle, &logCallbacks, &logVersion);
	if (rc != SA_AIS_OK) {
		fprintf(stderr, "Failed at saLogInitialize: %d \n", (int)rc);
		test_validate(rc, SA_AIS_OK);
		return;
	}

	// Perform si-swap in background
	sprintf(command, "amf-adm si-swap safSi=SC-2N,safApp=OpenSAF &");
	loop = system(command);

	// To make Open probably processed righ after logsv HA is set to QUEISED state.
	// This test is created to reproduce the issue reported in ticket #2093
	// The amount of sleep is an guess. It works when running on UML
	// If running on other power test enviroment, si-swap may happens quick.
	usleep(100*1000);

	loop = 100;
tryagain:
	rc = saLogStreamOpen_2(logHandle, &app1StreamName, &appStream1LogFileCreateAttributes,
	                       SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
	if (rc == SA_AIS_ERR_TRY_AGAIN && loop--) {
		usleep(100*1000);
		goto tryagain;
	}

	test_validate(rc, SA_AIS_OK);

tryagain_2:
	rc = saLogStreamClose(logStreamHandle);
	if (rc == SA_AIS_ERR_TRY_AGAIN && loop--) {
		usleep(100*1000);
		goto tryagain_2;
	}


	rc = saLogFinalize(logHandle);
	if (rc != SA_AIS_OK) {
		fprintf(stderr, "Failed to call salogFinalize: %d \n", (int) rc);
	}
}

//
// This test to verify Close call gets BAD_OPERATION in syslog or not during si-swap
//
void saLogStreamClose_BadOp(void)
{
	SaAisErrorT rc;
	char command[1000];
	int loop;

	cond_check();
	rc = saLogInitialize(&logHandle, &logCallbacks, &logVersion);
	if (rc != SA_AIS_OK) {
		fprintf(stderr, "Failed at saLogInitialize: %d \n", (int)rc);
		test_validate(rc, SA_AIS_OK);
		return;
	}

	loop = 100;
tryagain:
	rc = saLogStreamOpen_2(logHandle, &app1StreamName, &appStream1LogFileCreateAttributes,
	                       SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStreamHandle);
	if (rc == SA_AIS_ERR_TRY_AGAIN && loop--) {
		usleep(100*1000);
		goto tryagain;
	}

	if (rc != SA_AIS_OK) {
		fprintf(stderr, "Failed to open stream (%d)\n", rc);
		test_validate(rc, SA_AIS_OK);
		goto done;
	}

	// Perform si-swap in background
	sprintf(command, "amf-adm si-swap safSi=SC-2N,safApp=OpenSAF &");
	loop = system(command);

	// To make Close probably processed righ after logsv HA is set to QUEISED state.
	// This test is created to reproduce the issue reported in ticket #2093
	// The amount of sleep is an guess. It works when running on UML.
	// If running on other power test enviroment, si-swap may happens quick.
	usleep(100*1000);

	loop = 100;
tryagain_2:
	rc = saLogStreamClose(logStreamHandle);
	if (rc == SA_AIS_ERR_TRY_AGAIN && loop--) {
		usleep(100*1000);
		goto tryagain_2;
	}

	test_validate(rc, SA_AIS_OK);

done:
	rc = saLogFinalize(logHandle);
	if (rc != SA_AIS_OK) {
		fprintf(stderr, "Failed to call salogFinalize: %d \n", (int) rc);
	}
}

// For verifying the fault reported in ticket #2093
// The test must be performed on PL node
// As running si-swap in background, then need to put these 02 cases
// into 02 separate test suites
void add_suite_15()
{
	test_suite_add(15, "Test Open API while si-swap");
	test_case_add(15, saLogStreamOpen_BadOp, "saLogStreamOpen_2 while si-swap");
}

void add_suite_16()
{
	test_suite_add(16, "Test Close API while si-swap");
	test_case_add(16, saLogStreamClose_BadOp, "saLogStreamClose while si-swap");
}

extern void saLogStreamOpenAsync_2_01(void);
extern void saLogStreamOpenCallbackT_01(void);
extern void saLogWriteLog_01(void);
extern void saLogWriteLogAsync_01(void);
extern void saLogWriteLogAsync_02(void);
extern void saLogWriteLogAsync_03(void);
extern void saLogWriteLogAsync_04(void);
extern void saLogWriteLogAsync_05(void);
extern void saLogWriteLogAsync_06(void);
extern void saLogWriteLogAsync_07(void);
extern void saLogWriteLogAsync_08(void);
extern void saLogWriteLogAsync_09(void);
extern void saLogWriteLogAsync_10(void);
extern void saLogWriteLogAsync_11(void);
extern void saLogWriteLogAsync_12(void);
extern void saLogWriteLogAsync_13(void);
extern void saLogWriteLogAsync_14(void);
extern void saLogWriteLogAsync_15(void);
extern void saLogWriteLogAsync_16(void);
extern void saLogWriteLogAsync_17(void);
extern void saLogWriteLogAsync_18(void);
extern void saLogWriteLogAsync_19(void);
extern void saLogWriteLogCallbackT_01(void);
extern void saLogWriteLogCallbackT_02(void);
extern void saLogWriteLogCallbackT_03(void);
extern void saLogFilterSetCallbackT_01(void);
extern void saLogStreamClose_01(void);

__attribute__ ((constructor)) static void saLibraryLifeCycle_constructor(void)
{
    test_suite_add(2, "Log Service Operations");
    test_case_add(2, saLogStreamOpen_2_01, "saLogStreamOpen_2() system stream OK");
    test_case_add(2, saLogStreamOpen_2_02, "saLogStreamOpen_2() notification stream OK");
    test_case_add(2, saLogStreamOpen_2_03, "saLogStreamOpen_2() alarm stream OK");
    test_case_add(2, saLogStreamOpen_2_04, "Create app stream OK");
    test_case_add(2, saLogStreamOpen_2_05, "Create and open app stream");
    test_case_add(2, saLogStreamOpen_2_06, "saLogStreamOpen_2() - NULL ptr to handle");
    test_case_add(2, saLogStreamOpen_2_08, "saLogStreamOpen_2() - NULL logStreamName");
    test_case_add(2, saLogStreamOpen_2_09, "Open app stream second time with altered logFileName");
    test_case_add(2, saLogStreamOpen_2_10, "Open app stream second time with altered logFilePathName");
    test_case_add(2, saLogStreamOpen_2_11, "Open app stream second time with altered logFileFmt");
    test_case_add(2, saLogStreamOpen_2_12, "Open app stream second time with altered maxLogFileSize");
    test_case_add(2, saLogStreamOpen_2_13, "Open app stream second time with altered maxLogRecordSize");
    test_case_add(2, saLogStreamOpen_2_14, "Open app stream second time with altered maxFilesRotated");
    test_case_add(2, saLogStreamOpen_2_15, "Open app stream second time with altered haProperty");
    test_case_add(2, saLogStreamOpen_2_16, "Open app with logFileFmt == NULL");
    test_case_add(2, saLogStreamOpen_2_17, "Open app stream second time with logFileFmt == NULL");
    test_case_add(2, saLogStreamOpen_2_18, "Open app stream with NULL logFilePathName");
    test_case_add(2, saLogStreamOpen_2_19, "Open app stream with '.' logFilePathName");
    test_case_add(2, saLogStreamOpen_2_20, "Open app stream with invalid logFileFmt");
    test_case_add(2, saLogStreamOpen_2_21, "Open app stream with unsupported logFullAction");
    test_case_add(2, saLogStreamOpen_2_22, "Open non exist app stream with NULL create attrs");
    //This test case is no longer valid when Long DN is supported
    //test_case_add(2, saLogStreamOpen_2_23, "Open with stream name length == 256");
    test_case_add(2, saLogStreamOpenAsync_2_01, "saLogStreamOpenAsync_2(), Not supported");
    test_case_add(2, saLogStreamOpenCallbackT_01, "saLogStreamOpenCallbackT() OK");
    test_case_add(2, saLogWriteLog_01, "saLogWriteLog(), Not supported");
    test_case_add(2, saLogWriteLogAsync_01, "saLogWriteAsyncLog() system OK");
    test_case_add(2, saLogWriteLogAsync_02, "saLogWriteAsyncLog() alarm OK");
    test_case_add(2, saLogWriteLogAsync_03, "saLogWriteAsyncLog() notification OK");
    test_case_add(2, saLogWriteLogAsync_04, "saLogWriteAsyncLog() with NULL logStreamHandle");
    test_case_add(2, saLogWriteLogAsync_05, "saLogWriteAsyncLog() with invalid logStreamHandle");
    test_case_add(2, saLogWriteLogAsync_06, "saLogWriteAsyncLog() with invalid ackFlags");
    test_case_add(2, saLogWriteLogAsync_07, "saLogWriteAsyncLog() with NULL logRecord ptr");
    test_case_add(2, saLogWriteLogAsync_09, "saLogWriteAsyncLog() logSvcUsrName == NULL");
    test_case_add(2, saLogWriteLogAsync_10, "saLogWriteAsyncLog() logSvcUsrName == NULL and envset");
    test_case_add(2, saLogWriteLogAsync_11, "saLogWriteAsyncLog() with logTimeStamp set");
    test_case_add(2, saLogWriteLogAsync_12, "saLogWriteAsyncLog() without logTimeStamp set");
    test_case_add(2, saLogWriteLogAsync_13, "saLogWriteAsyncLog() 1800 bytes logrecord (ticket #203)");
    test_case_add(2, saLogWriteLogAsync_14, "saLogWriteAsyncLog() invalid severity");
    //These test cases are no longer valid when Long DN is supported
    //test_case_add(2, saLogWriteLogAsync_15, "saLogWriteAsyncLog() NTF notificationObject length == 256");
    //test_case_add(2, saLogWriteLogAsync_16, "saLogWriteAsyncLog() NTF notifyingObject length == 256");
    //test_case_add(2, saLogWriteLogAsync_17, "saLogWriteLogAsync() Generic logSvcUsrName length == 256");
    test_case_add(2, saLogWriteLogAsync_18, "saLogWriteLogAsync() logBufSize > strlen(logBuf) + 1");
    test_case_add(2, saLogWriteLogAsync_19, "saLogWriteLogAsync() logBufSize > SA_LOG_MAX_RECORD_SIZE");
    test_case_add(2, saLogWriteLogCallbackT_01, "saLogWriteLogCallbackT() SA_DISPATCH_ONE");
    test_case_add(2, saLogWriteLogCallbackT_02, "saLogWriteLogCallbackT() SA_DISPATCH_ALL");
    test_case_add(2, saLogFilterSetCallbackT_01, "saLogFilterSetCallbackT OK");
    test_case_add(2, saLogStreamClose_01, "saLogStreamClose OK");
    test_case_add(2, saLogStreamOpen_2_46, "saLogStreamOpen_2 with maxFilesRotated = 0, ERR");
    test_case_add(2, saLogStreamOpen_2_47, "saLogStreamOpen_2 with maxFilesRotated = 128, ERR");
    test_case_add(2, saLogStreamOpen_2_48, "saLogStreamOpen_2 with logFileName > 218 characters, ERR");
    test_case_add(2, saLogStreamOpen_2_49, "saLogStreamOpen_2 with invalid filename");

    test_case_add(2, verFixLogRec_Max_Err, "saLogStreamOpen_2 with maxLogRecordSize > MAX_RECSIZE, ERR");
    test_case_add(2, verFixLogRec_Min_Err, "saLogStreamOpen_2 with maxLogRecordSize < 150, ERR");
    test_case_add(2, saLogStreamOpen_2_50, "saLogStreamOpen_2 with stream number out of the limitation, ERR");
    test_case_add(2, saLogMultipleInitialize, "saLogInitialize() then saLogFinalize() multiple times. keep MDS connection, OK");
    test_case_add(2, saLogMultiThreadMultiInit, "saLogInitialize() then saLogFinalize() multiple times in multiple threads, OK");
}
