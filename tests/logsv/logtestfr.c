/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
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

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <poll.h>
#include <stdarg.h>

#include <saf_error.h>
#include <configmake.h>
#include <saAis.h>
#include <saLog.h>

#include <saImmOi.h>
#include <saImmOm.h>
#include <immutil.h>
#include <saImm.h>

/*******************************************************************************
 * Global variables and defines
 */
static SaVersionT logVersion = {'A', 0x02, 0x01}; 
static SaVersionT immVersion = {'A', 2, 11};

static SaLogHandleT logHandle;
static SaLogStreamHandleT logStreamHandle;
static char log_root_path[PATH_MAX];
static SaAisErrorT log_write_error = SA_AIS_OK;
static SaLogCallbacksT logCallbacks = {NULL, NULL, NULL};
static SaSelectionObjectT selectionObject;

static bool verbose_pr = false;
static bool step_exec = false;

const int trymax = 20;
const unsigned int sleep_delay_us = 500 * 1000 * 1000; /* 0,5 sec */
const char logfile_name_str[] = "logtestfr";
const char logfile_path_str[] = "logtestfr_path";
const SaUint32T logrec_max_size = 256;
const SaUint64T logfile_max_size = 256 * 10;

#define STREAM_NAME_STR "safLgStr=logtestfr_stream"
#define IMMLIST_CMD "immlist safLgStr=logtestfr_stream"
#define LOGTEST_MSG "logtestfr test message"

static void printf_v(const char *format, ...) __attribute__ ((format(printf, 1, 2)));
static void steptst(const char *format, ...) __attribute__ ((format(printf, 1, 2)));

/**
 * Return current HA state for a SC node
 * 
 * Note: Using immutil with default setting meaning abort if error
 * 
 * @return 1 SC-1, 2 SC-2
 */
static int get_active_sc(void)
{
	int active_sc = 0;
	SaImmHandleT omHandle;
	SaNameT objectName1 = { /* Read object for SC-1 */
		.value = "safSu=SC-1,safSg=2N,safApp=OpenSAF",
		.length = strlen("safSu=SC-1,safSg=2N,safApp=OpenSAF") + 1
	};
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 **attributes;
	const char saAmfSUNumCurrActiveSIs[] = "saAmfSUNumCurrActiveSIs";
	SaImmAttrNameT attributeNames[2] = {(char *) saAmfSUNumCurrActiveSIs, NULL};
	SaUint32T curr_act_sis = 0;
	
	/* NOTE: immutil init osaf_assert if error
	 */
	(void) immutil_saImmOmInitialize(&omHandle, NULL, &immVersion);
	(void) immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle);

	/* Get attributes of the object
	 * We may have to wait until a value is available
	 */
	int try_cnt = 0;
	while (1) {
		(void) immutil_saImmOmAccessorGet_2(accessorHandle, &objectName1,
											attributeNames, &attributes);
		if (attributes[0]->attrValuesNumber != 0)
			break;
		sleep(1);
		if (try_cnt++ >= 10) {
			fprintf(stderr ,"%s FAILED Attribute value could not be read\n",
					__FUNCTION__);
			abort();
		}
	}
	
	/* Checking SC-1 */
	curr_act_sis = *((SaUint32T *) attributes[0]->attrValues[0]);
	
	if (curr_act_sis > 0) {
		active_sc = 1;
	} else {
		active_sc = 2;
	}
	
	(void) immutil_saImmOmFinalize(omHandle);
	return active_sc;
}

/**
 * Verbose printing
 * If chosen extra trace printouts will be done during test.
 * Normally only error text is written on stderr
 * 
 * In-parameters same as printf()
 * @param format [in]
 * @param ... [in]
 */
static void printf_v(const char *format, ...)
{
	va_list argptr;
	
	if (!verbose_pr)
		return;
	
	va_start(argptr, format);
	(void) vfprintf(stdout, format, argptr);
	va_end(argptr);
}

/**
 * Print given information and stop execution waiting for user to press 'Enter'
 * key in order to continue.
 * 
 * In-parameters same as printf()
 * @param format [in]
 * @param ... [in]
 */
static void steptst(const char *format, ...)
{
	va_list argptr;
	
	if (!step_exec)
		return;
	
	va_start(argptr, format);
	printf("\n");
	(void) vfprintf(stdout, format, argptr);
	printf("\nPress 'Enter' to continue...\n");
	va_end(argptr);
	
	(void) getchar();
}

/**
 * Get the root path in log config object
 * @param &path_str[PATH_MAX+1] [out]
 */
void get_logRootDirectory(char *path_str)
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
		strncpy(path_str, *((char **) value), PATH_MAX);
	} else {
		/* We didn't get a root path from IMM. Use default */
		strncpy(path_str, PKGLOGDIR, PATH_MAX);
	}
	(void) immutil_saImmOmFinalize(omHandle);
}

/**
 * Callback function used with the log service
 * 
 * Global SaAisErrorT log_write_error = error
 * @param invocation [in]
 * @param error [in]
 */
static void log_write_callback(SaInvocationT invocation, SaAisErrorT error)
{
	log_write_error = error;
	if (error != SA_AIS_OK)
		fprintf(stderr,"%s: error = %s\n",__FUNCTION__, saf_error(error));
}

/**
 * Initialize log API. The handle is saved in a global variable.
 * Open a stream for test usage. Stream handle in global variable
 * Global: logHandle, logStreamHandle, selectionObject, logCallbacks
 * 
 * @param *ais_rc [out]
 * @return -1 on error. Return code in ais_rc
 */
static int init_log_open_stream(SaAisErrorT *ais_rc)
{
	int rc = 0;
	int trycnt = 0;
	
	/* Prepare callback */
	logCallbacks.saLogWriteLogCallback = log_write_callback;
	
	/* Initialize log API. No callbacks needed */
	do {
		/* Try again loop. If other error end loop */
		*ais_rc = saLogInitialize(&logHandle, &logCallbacks, &logVersion);
		if (*ais_rc != SA_AIS_ERR_TRY_AGAIN) {
			break;
		}
		usleep(sleep_delay_us);
		if (++trycnt > trymax) {
			/* Give up try again */
			break;
		}
	} while (*ais_rc != SA_AIS_OK);
	if (*ais_rc != SA_AIS_OK) {
		fprintf(stderr,"%s Failed to initialize log service: %s\n",__FUNCTION__,
				saf_error(*ais_rc));
		rc = -1;
		goto done;
	}
	
	/* Get selection object for handling write callback */
	(void) saLogSelectionObjectGet(logHandle, &selectionObject);
	
	/* Prepare stream name and attributes for the stream to be opened */
	SaNameT stream_name =
	{
		.value = STREAM_NAME_STR,
		.length = sizeof(STREAM_NAME_STR)
	};
	
	SaLogFileCreateAttributesT_2 stream_create_attributes =
	{
		.logFilePathName = (char *) logfile_path_str,
		.logFileName = (char *) logfile_name_str,
		.maxLogFileSize = logfile_max_size,
		.maxLogRecordSize = logrec_max_size,
		.haProperty = SA_TRUE,
		.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE,
		.maxFilesRotated = 4,
		.logFileFmt = NULL
	};

	
	/* Open an application stream for the test */
	trycnt = 0;
	do {
		/* Try again loop. If other error end loop */
		*ais_rc = saLogStreamOpen_2(logHandle,
				&stream_name,
				&stream_create_attributes,
                SA_LOG_STREAM_CREATE,
				SA_TIME_ONE_SECOND,
				&logStreamHandle);
		if (*ais_rc != SA_AIS_ERR_TRY_AGAIN) {
			break;
		}
		usleep(sleep_delay_us);
		if (++trycnt > trymax) {
			/* Give up try again */
			break;
		}
	} while (*ais_rc != SA_AIS_OK);
	if (*ais_rc != SA_AIS_OK) {
		fprintf(stderr, "%s Failed to open log stream: %s\n", __FUNCTION__,
				saf_error(*ais_rc));
		rc = -1;
		goto done;
	}
	
	done:
	
	return rc;
}


/**
 * Write log records to the stream opened with init_log_open_stream()
 * When this function returns it's guaranteed that all log records ar written
 * and that all files are created in case of log rotation.
 * 
 * Global: logStreamHandle
 * 
 * @param rec_cnt [in] Write this number of log records
 * @param *ais_rc [out]
 * @return -1 on error. Return code in ais_rc
 */
static int write_logrecord(int rec_cnt, SaAisErrorT *ais_rc)
{
	int rc = 0;
	int poll_rc = 0;
	int trycnt = 0;
	int i;
	struct pollfd fds[1];
	
	/* Prepare a test log record
	 * 
	 * Example record that will be created:
	 * 36 10:23:04 10/24/2014 IN safLgStr=logtestfr_stream "logtestfr test message"
	 */
	SaNameT logSvcUsrName = 
	{
		.value = STREAM_NAME_STR,
		.length = sizeof(STREAM_NAME_STR)
	};
	
	SaLogBufferT log_buffer =
	{
		.logBuf = (SaUint8T *) LOGTEST_MSG,
		.logBufSize = sizeof(LOGTEST_MSG),
	};
	
	SaLogRecordT log_record_tst =
	{
		.logTimeStamp = SA_TIME_UNKNOWN,
		.logHdrType = SA_LOG_GENERIC_HEADER,
		.logHeader.genericHdr.notificationClassId = NULL,
		.logHeader.genericHdr.logSeverity = SA_LOG_SEV_INFO,
		.logHeader.genericHdr.logSvcUsrName = &logSvcUsrName,
		.logBuffer = &log_buffer
	};

	/* Prepare for polling */
	fds[0].fd = (int) selectionObject;
	fds[0].events = POLLIN;
	
	/* Write a test log record rec_cnt number of times.
	 * Wait for callback for each log record.
	 * This is in order to know that all log records have been written
	 * and by that also know that log rotation is done and the old file exist
	 * and is closed.
	 */
	for (i=0; i<rec_cnt; i++) {
		/* Write the log record */
		do {
			/* Try again loop. If other error end loop */
			*ais_rc = saLogWriteLogAsync(logStreamHandle, 0,
					SA_LOG_RECORD_WRITE_ACK, &log_record_tst);
			if (*ais_rc != SA_AIS_ERR_TRY_AGAIN) {
				break;
			}
			usleep(sleep_delay_us);
			if (++trycnt > trymax) {
				/* Give up try again */
				break;
			}
		} while (*ais_rc != SA_AIS_OK);
		if (*ais_rc != SA_AIS_OK) {
			fprintf(stderr, "%s Failed to write log record: %s\n",__FUNCTION__,
					saf_error(*ais_rc));
			rc = -1;
			goto done;
		}
		
		/* Wait until the log record is written before writing the next one.
		 * Wait for selection object and invoke the callback.
		 */
		poll_rc = poll(fds, 1, 10000); /* 10 sec timeout */
		if (poll_rc <= 0) {
			/* An error or timeout. Could be EAGAIN but we handle this as an
			 * error
			 */
			if (rc == -1) {
				perror("write_logrecord() Poll fail");
			} else {
				printf_v("%s Poll timeout when writing log record\n",
						__FUNCTION__);
			}
			rc = -1;
			goto done;
		}
		(void) saLogDispatch(logHandle, SA_DISPATCH_ONE);
	}
	
	done:
	return rc;
}

/**
 * Filter function selecting files to include in file list when using
 * scandir().
 * We want to find the closed files for the app stream used in this test
 * The file name is:
 * logtestfr_stream_YYMMDD_HHMMSS_YYMMDD_HHMMSS.log
 * The filter use the fact that the name always contains 4 '_' characters
 * 
 * @param finfo
 * @return 1 if match
 */
static int rotated_files_filter(const struct dirent *finfo)
{
	char *str_p = (char *) finfo->d_name;
	int chr_cnt = 0;
	while ((str_p = strchr(str_p+1, '_')) != NULL) {
		chr_cnt++;
	}
	
	/* Check criteria for closed log file */
	if (chr_cnt == 4)
		return 1;
	else
		return 0;
}
	
/**
 * Find closed test log files and truncate the file name by removing the 14 last
 * characters. Rename the files to the truncated name
 */
static int truncate_filenames(void)
{
	struct dirent **namelist;
	int file_cnt = 0;
	int i;
	char file_path[256];
	char name_str[512];
	char trunc_str[512];
	int rc = 0;
	size_t str_end;
	
	/* Create path to log files */
	snprintf(file_path, 256, "%s/%s/", log_root_path, logfile_path_str);
	printf_v("%s: logdir \"%s\"\n",__FUNCTION__, file_path);

	/* Get the files to truncate */
	file_cnt = scandir(file_path, &namelist, rotated_files_filter, alphasort);
	if (file_cnt < 0) {
		perror("truncate_filenames() scandir fail");
		rc = -1;
		goto exit_func;
	}
	printf_v("%s: file_cnt = %d\n",__FUNCTION__, file_cnt);
	
	/* For all files truncate the file name and rename the file to the
	 * truncated name.
	 */
	printf_v("\nRename files:\n");
	for (i = 0; i < file_cnt; i++) {
		printf_v("Name: \"%s\"\n", namelist[i]->d_name);
		snprintf(name_str, 512, "%s%s", file_path, namelist[i]->d_name);
		printf_v("Full path: \"%s\"\n", name_str);
		/* Truncate. Remove last 14 characters */
		strcpy(trunc_str, name_str);
		str_end = strlen(trunc_str) - 14;
		trunc_str[str_end] = '\0';
		printf_v("Truncated path: \"%s\"\n", trunc_str);
		/* Rename file */
		rc = rename(name_str, trunc_str);
		if (rc < 0) {
			perror("truncate_filenames() Rename failed");
			goto done;
		}
	}
	
	

	done:
	/* Free scandir allocated memory */
	for (i = 0; i < file_cnt; i++)
		free(namelist[i]);
	free(namelist);
	
	exit_func:
	return rc;
}

/**
 * Remove files in directory and the directory given by path.
 * Uses unlink and rmdir.
 * 
 * @param path
 * @return -1 on error errno set as appropriate
 */
static int names(const struct dirent *finfo)
{
	if (finfo->d_name[0] == '.')
		return 0;
	else
		return 1;
}

static int remove_rf(const char *path)
{
	int rc = 0;
	struct dirent **namelist;
	int file_cnt = 0;
	int i = 0;
	char filepath[PATH_MAX];
	
	/* Get files to remove */
	file_cnt = scandir(path, &namelist, names, alphasort);
	if (file_cnt < 0) {
		rc = -1;
		perror("remove_rf() scandir fail");
		goto exit_func;
	}
	
	/* Remove files */
	for (i = 0; i < file_cnt; i++) {
		(void) snprintf(filepath, PATH_MAX, "%s/%s", path, namelist[i]->d_name);
		rc = remove(filepath);
		if (rc < 0) {
			goto done;
		}
	}
	/* Remove directory */
	rc = remove(path);
		
	done:
	/* Free scandir allocated memory */
	for (i = 0; i < file_cnt; i++)
		free(namelist[i]);
	free(namelist);
	
	exit_func:
	return rc;	
}

/**
 * Test that log server can handle rotation if old log files that has been
 * renamed so that filename is <name>_yymmdd_hhmmss_yyddmm.log i.e. the last
 * part of the close timestamp has been truncated.
 * This will test fix for #1175.
 * Without fix: Log server crash
 * With fix:    Log server handle files as if they where ok
 * 
 * NOTE: The original problem causes the log service to crash causing a node
 *       restart.
 * 
 * @return -1 if test fail else 0
 */
int tst_logrotation_truncated_filename(void)
{
	int rc = 0;
	SaAisErrorT ais_rc;
	int active_before_test = 0;
	int active_after_test = 0;
	
	/* Get active node before test
	 */
	active_before_test = get_active_sc();
	
	/* Get log root directory and save in global variable
	 */
	get_logRootDirectory(log_root_path);
	printf_v("log_root_path \"%s\"\n", log_root_path);

	/* Initiate the log API and open the test stream
	 */
	rc = init_log_open_stream(&ais_rc);
	printf_v("init_log_open_stream() rc = %d, ais_rc \"%s\"\n",rc, saf_error(ais_rc));
	if (rc == -1) {
		rc = -1;
		goto done;
	}
	
	/* Write some log records so that log rotation is done and one
	 * closed log file exists.
	 * NOTE: The log file is small meaning that several rotated files may be
	 * created each second. File naming is based on time with a resolution of
	 * 1 sec. This means that new file may get the same name as the previous
	 * causing the previous file to be overwritten!
	 * To make sure this will not happen in this test records are written in
	 * groups with a delay in between
	 */
	rc = write_logrecord((12), &ais_rc);
	printf_v("write_logrecord() rc = %d, ais_rc \"%s\"\n",rc, saf_error(ais_rc));
	if (rc == -1) {
		rc = -1;
		goto done;
	}
	sleep(1);
	rc = write_logrecord((12), &ais_rc);
	printf_v("write_logrecord() rc = %d, ais_rc \"%s\"\n",rc, saf_error(ais_rc));
	if (rc == -1) {
		rc = -1;
		goto done;
	}
	
	steptst("Log records written before truncation.\n"
			"Next truncate file names");
	
	/* Find closed log files and truncate the file names
	 */
	rc = truncate_filenames();
	if (rc == -1) {
		rc = -1;
		goto done;
	}
	
	steptst("Truncate_filenames() done.\n"
			"Next write more log records");
	
	/* Write some more log records to get log rotation after truncation
	 */
	rc = write_logrecord(12, &ais_rc);
	printf_v("\nwrite_logrecord() rc = %d, ais_rc \"%s\"\n",rc, saf_error(ais_rc));
	if (rc == -1) {
		rc = -1;
		goto done;
	}
	sleep(1);
	
	steptst("More log records written.\n"
			"Next write even more log records to rotate out the truncated");
	
	/* Write even more log records to get log rotation after truncation
	 */
	rc = write_logrecord(12, &ais_rc);
	printf_v("\nwrite_logrecord() rc = %d, ais_rc \"%s\"\n",rc, saf_error(ais_rc));
	if (rc == -1) {
		rc = -1;
		/* Fail test case but try to finalize and cleanup */
	}
	sleep(1);
	rc = write_logrecord(12, &ais_rc);
	printf_v("\nwrite_logrecord() rc = %d, ais_rc \"%s\"\n",rc, saf_error(ais_rc));
	if (rc == -1) {
		rc = -1;
		/* Fail test case but try to finalize and cleanup */
	}

	steptst("File rotation after truncation shall have been done, stream still open.\n"
			"Next log finalized (stream closed/removed)");
	
	/* Finalize log API
	 */
	int try_cnt = 0;
	do {
		ais_rc = saLogFinalize(logHandle);
		if (ais_rc == SA_AIS_OK)
			break; /* We don't have to wait */
		if (try_cnt++ >= 10)
			break; /* Timeout waiting */
		usleep(1000 * 100); /* Wait 100 ms */
	} while (ais_rc == SA_AIS_ERR_TRY_AGAIN);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr,"%s Failed to finalize log service \"%s\"\n",
				__FUNCTION__, saf_error(ais_rc));
		/* Fail test case but try to cleanup */
		rc = -1;
	}
	
	steptst("Cleanup, remove files and directories used in test.\n"
			"Next exit");
	
	/* Cleanup after test. Remove files and directories used in test
	 */
	/* Create path to remove*/
	char rem_path[PATH_MAX];
	(void) snprintf(rem_path, PATH_MAX, "%s/%s", log_root_path, logfile_path_str);
	printf_v("Path to remove \"%s\"\n",rem_path);
	rc = remove_rf(rem_path);
	if (rc < 0) {
		perror("tst_logrotation_truncated_filename() Failed to cleanup after test");
	}
	
	done:
	/* Get active node after test
	 * Fail over if not the same as before test
	 */
	active_after_test = get_active_sc();
	if (active_before_test != active_after_test) {
		/* Failover detected. Fail test */
		fprintf(stderr,"\nFailover detected. active before test SC-%d after SC-%d\n",
				active_before_test, active_after_test);
		rc = -1;
	}
			
	return rc;
}

/**
 * 
 */
static void usage(void)
{
	const char progname[] = "logtestfr";
	
	printf("\nNAME\n");
	printf("\t%s - Test log server handling of truncated file names\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [options]\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s Tests that the log server can handle when log filenames has\n"
			"\tbeen changed (truncated) in such a way that the time stamp consists\n"
			"\tof three 'fields' instead of two or four, see ticket #1175\n"
			"\tBefore ticket #1175 was fixed the active log server crashed with\n"
			"\tsegv causing node restart\n", progname);

	printf("\nOPTIONS\n");

	printf("  -h                    This help\n");
	printf("  -v                    Verbosed printouts during test\n");
	printf("  -s                    At each test step stop and wait for user to\n"
			"                        press 'Enter'. This makes it possible to\n"
			"                        manually check log files between each step\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	int rc = 0;
	int opt_val = 0;

	/* Handle options */
	while (1) {
		opt_val = getopt(argc, argv, "vsh");
		if (opt_val < 0)
			break;
		switch (opt_val) {
			case 'v':
				/* Verbose print */
				verbose_pr = true;
				break;
			case 's':
				/* Execute in steps. Confirm with Enter */
				step_exec = true;
				break;
			case 'h':
			default:
				/* Print help and exit */
				usage();
				exit(0);
		}
	}
	
	/* Run test */
	rc = tst_logrotation_truncated_filename();
	
	/* Report PASS (0) / FAIL (1) */
	if (rc == 0) {
		printf_v("\n\nTest PASS\n");
		exit(0);
	} else{
		printf_v("\n\nTest FAIL\n");
		exit(1);
	}
}
