/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2015 The OpenSAF Foundation
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
#include <sys/types.h>
#include <sys/wait.h>
#include <saImm.h>
#include <saImmOm.h>
#include <limits.h>
#include <unistd.h>
#include <stdarg.h>
#include <osaf_utility.h>
#include <saf_error.h>
#include <osaf_poll.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>


/* Test cases for cloud resilience.
 * NOTE1: For the moment these test cases need interaction with the tester and
 * cannot be run fully automatic.
 * NOTE2: Must run on PL node!
 */

/*
 * GLOBAL
 */
static const int trymax = 20;
static const unsigned int sleep_delay_us = 1 * 100 * 1000; /* 0,1 sec */
static const char logfile_path_str[] = "logtest_sc_down_path";
static const SaUint32T logrec_max_size = 256;
static const SaUint64T logfile_max_size = 256 * 10;

#define STREAM_NAME_9 "suite9_stream"

#define STREAM_API_CLASS "safLgStr="
#define IMMLIST_CMD "immlist safLgStr=logtest_sc_down_stream"
#define LOGTEST_MSG "logtest_sc_down test message"

/* Tags for output to external test system. Used with option -t
 * The content of these tags must be consistent. Do not change existing
 * tag text
 */
#define TST_TAG_ND "TAG_ND\n"	/* Tag for take SC nodes down */
#define TST_TAG_NU "TAG_NU\n"	/* Tag for start SC nodes */

#define TST_PASS 0
#define TST_FAIL 1

/* Possible to save up to 10 handles for clients and
 * up to 10 open streams per client */
#define MAX_CLIENTS 10
#define MAX_STREAMS 10
typedef struct {
	SaLogHandleT glob_logHandle;
	SaLogStreamHandleT glob_logStreamHandle[MAX_STREAMS]; /*SaUint64T*/
	int stream_num; /* Number of saved stream handles */
} client_t;
static client_t g_client[MAX_CLIENTS];

typedef enum {
	NTF_LOG_REC,
	GEN_LOG_REC
} lgt_log_rec_type_t;

/* Updated in the write callback function*/
static struct {
	SaInvocationT invocation_in;
	SaInvocationT invocation_out;
	SaAisErrorT ais_errno;
} lgt_cb;

static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
#define TST_CALLBACK_ID 0
#define TST_WRITE_ID    1

static struct {
	uint32_t ais_ok;
	uint32_t ais_try_again;
	uint32_t ais_bad_handle;
	uint32_t ais_other;
	uint32_t write_cnt;
	uint32_t callback_cnt;
} ais_rc_cnt;

/* ***************
 * Help functions
 */

/**
 * sigusr2 handler with help functions
 *
 */
static bool gl_wait_flag = true;
static void sigusr2_handler(int sig)
{
	if (gl_wait_flag == true)
		gl_wait_flag = false;
}

static void arm_sigusr2_handler(void)
{
	gl_wait_flag = true;
	signal(SIGUSR2, sigusr2_handler);
}

/**
 * Wait for tester action
 * If option -t wait for SIGUSR2
 * else wait for tester to press Enter
 *
 * @param infostr[in] Info string printed if not -t
 */
static void wait_ext_act(char *infostr)
{
	if (tag_flg == false) {
		printf_s("%s", infostr);
		getchar();
	} else {
		arm_sigusr2_handler();
		while (gl_wait_flag == true) {
			sleep(1);
		}
	}
}

/**
 * Log callback function. See tst_LogInitialize()
 *
 * @param invocation
 * @param error
 */
static void log_write_callback(SaInvocationT invocation, SaAisErrorT error);

#if 1 /* Temporary remove */
/**
 * Delete the given file
 *
 * @param filename[in] Filename only. It's assumed that the file can be found
 *        in the suite 9 test directory
 * @return -1 on error
 */
static int delete_file(char *filename)
{
	int rc = 0;
	int n = 0;
	char filepath[PATH_MAX];

	n = snprintf(filepath, PATH_MAX, "/%s/%s/%s",
		log_root_path, logfile_path_str, filename);
	if ((n > PATH_MAX) || (n <= 0)) {
		fprintf(stderr, "%s Create file path Fail n=%d\n",
			__FUNCTION__, n);
		rc = -1;
		goto done;
	}

	if (unlink(filepath) == -1) {
		fprintf(stderr, "%s unlink %s Fail %s\n",
			__FUNCTION__, filepath, strerror(errno));
		rc = -1;
	}

	done:
	return rc;
}
#endif

/**
 * Remove the content of given file
 *
 * @param filename[in] Filename only. It's assumed that the file can be found
 *        in the suite 9 test directory
 * @return -1 on error
 */
static int clear_file(char *filename)
{
	int rc = 0;
	int n = 0;
	FILE *fs;
	char filepath[PATH_MAX];

	n = snprintf(filepath, PATH_MAX, "/%s/%s/%s",
		log_root_path, logfile_path_str, filename);
	if ((n > PATH_MAX) || (n <= 0)) {
		fprintf(stderr, "%s Create file path Fail n=%d\n",
			__FUNCTION__, n);
		rc = -1;
		goto done;
	}

	fs = fopen(filepath, "w");
	if (fs == NULL) {
		fprintf(stderr, "%s fopen \"%s\" Fail %s\n",
			__FUNCTION__, filepath, strerror(errno));
		rc = -1;
		goto done;
	}

	//fputs(const char *s, FILE *stream)
	if (fputs("", fs) == EOF) {
		fprintf(stderr, "%s fputs Fail\n",__FUNCTION__);
		rc = -1;
		goto done_close;
	}

	done_close:
	if (fclose(fs) == EOF) {
		fprintf(stderr, "%s fclose Fail %s\n",
			__FUNCTION__, strerror(errno));
		rc = -1;
		goto done;
	}

	done:
	return rc;
}

/******************************************************************************
 * Find the name of the current log file based on the name prefix
 * With help functions
 */
/**
 * Filter function used by scandir.
 * Find a current log file if it exist
 * - name as in file_name_find[]
 * - extension .log
 * - two timestamps (other log files has four)
 */
static char file_name_find[SA_MAX_NAME_LENGTH];
static int filter_logfile_name(const struct dirent *finfo)
{
	char name_format[SA_MAX_NAME_LENGTH];
	char time_stamps[] = "_%d_%d_%d_%d.log";
	int n = 0;
	int a, b, c, d;
	a = b = c = d = 0;

	/* Create format string for sscanf */
	n = snprintf(name_format, SA_MAX_NAME_LENGTH, "%s%s",
		file_name_find, time_stamps);
	/* Get number of timestamps */
	n = sscanf(finfo->d_name, name_format, &a, &b, &c, &d);

	/* Filter out current log file */
	if (n == 2)
		return 1; /* Match */
	else
		return 0;
}

/**
 * Find the name of the current log file based on the name prefix
 *
 * @param name_str[out]    Name of current log file
 * @param name_prefix[in]  Name prefix for log file
 * @return -1 on
 */
static int cur_logfile_name_get(char *name_str, char *name_prefix)
{
	int rc = 0;
	int n = 0;
	char scanpath_str[PATH_MAX] = {0};
	struct dirent **namelist;

	n = snprintf(scanpath_str, PATH_MAX, "%s/%s",
		log_root_path, logfile_path_str);
	if (n > PATH_MAX) {
		fprintf(stderr, "%s Create scan path Fail\n",__FUNCTION__);
		rc = -1;
		goto done;
	}

	n = snprintf(file_name_find, SA_MAX_NAME_LENGTH, "%s", name_prefix);
	if (n > SA_MAX_NAME_LENGTH) {
		fprintf(stderr, "%s File name prefix Fail\n",__FUNCTION__);
		rc = -1;
		goto done;
	}

	errno = 0;
	n = scandir(scanpath_str, &namelist, filter_logfile_name, alphasort);
	if (n < 0) {
		fprintf(stderr, "%s scandir Fail %s",
			__FUNCTION__, strerror(errno));
		rc = -1;
		goto done;
	} else if (n == 0) {
		fprintf(stderr, "%s No current log file found",__FUNCTION__);
		rc = -1;
		free(namelist);
		goto done;
	} else if (n > 1) {
		fprintf(stderr, "%s More than one file found n = %d Fail",
			__FUNCTION__, n);
		while (n--) {
			free(namelist[n]);
		}
		free(namelist);
		rc = -1;
		goto done;
	} else {
		/* Save file name */
		strcpy(name_str, namelist[0]->d_name);
		free(namelist[0]);
		free(namelist);
	}


	done:
	return rc;
}

/**
 * Initialize the log API
 * For API version see GLOBAL logVersion
 * No callback
 *
 * @param logHandle [out]
 * @param try_again_flg [in] If false no try again loop is used (1 attempt)
 * @return AIS return code
 */
static SaAisErrorT tst_LogInitialize(SaLogHandleT *log_handle, int trycnt_max)
{
	int trycnt = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;

	/* Prepare callback */
	logCallbacks.saLogWriteLogCallback = log_write_callback;

	/* Initialize log API */
	if (trycnt_max > 0) {
		do {
			/* Try again loop. If other error end loop */
			ais_rc = saLogInitialize(log_handle, &logCallbacks,
				&logVersion);
			if (ais_rc != SA_AIS_ERR_TRY_AGAIN) {
				break;
			}
			usleep(sleep_delay_us);
			if (++trycnt > trycnt_max) {
				/* Give up try again */
				break;
			}
		} while (ais_rc != SA_AIS_OK);
	} else {
		ais_rc = saLogInitialize(log_handle, &logCallbacks,
				&logVersion);
	}

	return ais_rc;
}

/**
 * Open an application log stream using the log API
 * See GLOBAL for stream name, file name rel path etc.
 * For stream name STREAM_NAME_9 is used together with a number.
 *
 * @param logHandle [in]
 * @param logStreamHandle [out] An array of stream handles. The array has to be
 *                              declared by the user. Must have enough instances
 *                              to hold num_streams stream handles
 * @param num_streams[in]       Number of streams to open
 *
 * @return -1 on error
 */
static int tst_StreamOpen_app_logtest_sc(
	SaLogHandleT logHandle,
	SaLogStreamHandleT *logStreamHandle,
	uint32_t num_streams)
{
	int trycnt = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;
	int rc = 0;
	int n;
	int i;

	/* Open num_streams */
	for (i = 0; i < num_streams; i++) {
		/* Prepare stream name and attributes for the stream to be opened */
		SaNameT stream_name;
		n = sprintf((char *) stream_name.value, "safLgStr=%s_%d",
			STREAM_NAME_9, i+1);
		if (n < 0) {
			fprintf(stderr, "\t%s [%d] sprintf Fail\n",
				__FUNCTION__, __LINE__);
			rc = -1;
			break;
		}
		stream_name.length = n;

		/* Create log file name */
		char logfile_name[256];
		(void) snprintf(logfile_name, 256, "%s_file_%d",
			STREAM_NAME_9, i);

		SaLogFileCreateAttributesT_2 stream_create_attributes =
		{
			.logFilePathName = (char *) logfile_path_str,
			.logFileName = logfile_name,
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
			ais_rc = saLogStreamOpen_2(logHandle,
				&stream_name,
				&stream_create_attributes,
				SA_LOG_STREAM_CREATE,
				SA_TIME_ONE_SECOND,
				&logStreamHandle[i]);
			if (ais_rc != SA_AIS_ERR_TRY_AGAIN) {
				break;
			}
			usleep(sleep_delay_us);
			if (++trycnt > trymax) {
				/* Give up try again */
				break;
			}
		} while (ais_rc != SA_AIS_OK);
		if (ais_rc != SA_AIS_OK) {
			fprintf(stderr, "saLogStreamOpen_2 Fail %s\n",
				saf_error(ais_rc));
			rc = -1;
			break;
		}
	}

	return rc;
}

/**
 * Open one configuration (notification) log stream using the log API
 * See GLOBAL for stream name, file name rel path etc.
 * For stream name STREAM_NAME_9 is used together with a number.
 *
 * @param logHandle [in]
 * @param logStreamHandle [out] Shall hold one stream handle
 *
 * @return -1 on error
 */
static int tst_StreamOpen_cfg_logtest_sc(
	SaLogHandleT logHandle,
	SaLogStreamHandleT *logStreamHandle)
{
	SaAisErrorT ais_rc = SA_AIS_OK;
	int rc = 0;

	int trycnt = 0;
	do {
		/* Try again loop. If other error end loop */
		ais_rc = saLogStreamOpen_2(logHandle, &notificationStreamName,
			NULL, 0, SA_TIME_ONE_SECOND, logStreamHandle);
		if (ais_rc != SA_AIS_ERR_TRY_AGAIN) {
			break;
		}
		usleep(sleep_delay_us);
		if (++trycnt > trymax) {
			/* Give up try again */
			break;
		}
	} while (ais_rc != SA_AIS_OK);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "saLogStreamOpen_2 Fail %s\n",
			saf_error(ais_rc));
		rc = -1;
	}

	return rc;
}

/**
 * Create a log message of given size as a string
 *
 * @param logrec_str[out]  Pointer to storage for created log record
 * @param lsize[in]        Size of log record to create including '\0'
 * @param log_msg[in]      Message for the log record
 */
static void tst_max_logrec_create(char *logrec_str, uint32_t lsize, char *log_msg)
{
	int n;
	size_t fill_size;
	char *ptr_fill_start = NULL;

	/* Create information part of message */
	n = snprintf(logrec_str, lsize, "%s size %d ", log_msg, lsize);

	if (n < lsize) {
		fill_size = lsize - n;
	} else {
		fill_size = 0;
	}

	/* Fill message with dummy char to make it the wanted size  */
	if (fill_size > 0) {
		ptr_fill_start = logrec_str + n;
		memset(ptr_fill_start, 'f', fill_size);
		logrec_str[lsize - 1] = '\0';
	}
}

/**
 * Write a log record to stream 'logStreamHandle'
 * See GLOBAL for predefined constant parameters
 *
 * @param rec_cnt [in] Number of log records to write
 * @param logStreamHandle [in] Stream handle from opening stream
 * @param log_handle[in] Log handle from initialize
 * @param log_message [in] If NULL a default message is used
 * @param try_again_flag [in] If false SA_AIS_TRY_AGAIN is handled as error
 * @param chdl_flag[in] If true handle callback. Wait for callback before
 *                      sending next log record.
 * @param rec_type[in] Type of log record Generic or NTF
 * @return AIS return code
 */
static SaAisErrorT tst_WriteRec(
	int rec_cnt,
	SaLogStreamHandleT logStreamHandle,
	SaLogHandleT log_handle,
	char *log_message,
	bool try_again_flag,
	bool chdl_flag,
	lgt_log_rec_type_t rec_type)
{
	int trycnt = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;
	int i;

	SaNtfIdentifierT notificationId = 0;
	SaNtfClassIdT notificationClassId;
	SaLogRecordT *log_record_tst;



	/* Prepare for polling callback if needed */
	SaSelectionObjectT selectionObject;
	struct pollfd fds[1];
	int poll_rc = 0;

	if (chdl_flag == true) {
		/* Get selection object for handling write callback */
		ais_rc = saLogSelectionObjectGet(log_handle, &selectionObject);
		if (ais_rc != SA_AIS_OK) {
			printf_v("\t%s; saLogSelectionObjectGet Fail %s\n",
				__FUNCTION__, saf_error(ais_rc));
			/* On error for now end test */
			exit(1);
		}

		/* Prepare for polling */
		fds[0].fd = (int) selectionObject;
		fds[0].events = POLLIN;
	}


	/* Prepare a test log record
	 *
	 * Example record that will be created:
	 * 36 10:23:04 10/24/2014 IN safLgStr=<name> "test message"
	 */

	SaLogBufferT log_buffer =
	{
		.logBuf = (SaUint8T *) LOGTEST_MSG,
		.logBufSize = sizeof(LOGTEST_MSG),
	};
	if (log_message != NULL) {
		log_buffer.logBuf = (uint8_t *) log_message;
		log_buffer.logBufSize = strlen(log_message)+1;
	}

	/* A generic log record */
	SaLogRecordT log_record_gen_tst =
	{
		.logTimeStamp = SA_TIME_UNKNOWN,
		.logHdrType = SA_LOG_GENERIC_HEADER,
		.logHeader.genericHdr.notificationClassId = NULL,
		.logHeader.genericHdr.logSeverity = SA_LOG_SEV_INFO,
		.logHeader.genericHdr.logSvcUsrName = &logSvcUsrName,
		.logBuffer = &log_buffer
	};

	/* A NTF log record */
	SaLogRecordT log_record_ntf_tst;
	notificationClassId.vendorId = 193;
	notificationClassId.majorId  = 1;
	notificationClassId.minorId  = 2;
	log_record_ntf_tst.logTimeStamp = getSaTimeT();
	log_record_ntf_tst.logHdrType = SA_LOG_NTF_HEADER;
	log_record_ntf_tst.logHeader.ntfHdr.notificationId = notificationId;
	log_record_ntf_tst.logHeader.ntfHdr.eventType = SA_NTF_ALARM_QOS;
	log_record_ntf_tst.logHeader.ntfHdr.notificationObject = &notificationObject;
	log_record_ntf_tst.logHeader.ntfHdr.notifyingObject = &notifyingObject;
	log_record_ntf_tst.logHeader.ntfHdr.notificationClassId = &notificationClassId;
	log_record_ntf_tst.logHeader.ntfHdr.eventTime = getSaTimeT();
	log_record_ntf_tst.logBuffer = &log_buffer;

	if (rec_type == NTF_LOG_REC) {
		log_record_tst = &log_record_ntf_tst;
	} else {
		log_record_tst = &log_record_gen_tst;
	}

	/* Write a test log record rec_cnt number of times. */
	for (i=0; i<rec_cnt; i++) {
		/* Write the log record */
		if (chdl_flag == true) {
			lgt_cb.invocation_in = i + 555;
		} else {
			lgt_cb.invocation_in = 555;
		}
		do {
			/* Try again loop. If other error end loop
			 * If no_try_again_flag = true do not loop if TRY AGAIN
			 */
			ais_rc = saLogWriteLogAsync(logStreamHandle,
				lgt_cb.invocation_in,
				SA_LOG_RECORD_WRITE_ACK, log_record_tst);

			if ((ais_rc == SA_AIS_ERR_TRY_AGAIN) &&
				(try_again_flag == false)) {
				/* Handle TRY AGAIN as an error.
				 * No try again loop */
				break;
			}
			if (ais_rc != SA_AIS_ERR_TRY_AGAIN) {
				break;
			}
			usleep(sleep_delay_us);
			if (++trycnt > trymax) {
				/* Give up try again */
				break;
			}
		} while (ais_rc != SA_AIS_OK);

		if (ais_rc != SA_AIS_OK) {
			/* Write failed. Do not try to write any more records */
			printf_v("\t%s Fail to write record no %d ais_rc \"%s\"\n",
				__FUNCTION__, i, saf_error(ais_rc));
			break;
		}

		/* If requested to do so:
		 *  - Wait for callback (poll)
		 *  - Return code and invocation id is verified
		 */
		if (chdl_flag == true) {
			poll_rc = osaf_poll(fds, 1, -1); /* Never timeout */
			if (poll_rc <= 0){
				/* Timeout (will do abort on other errors)
				 */
				fprintf(stderr, "\t%s; Poll timeout\n",
					__FUNCTION__);
				ais_rc = SA_AIS_ERR_TIMEOUT;
				break; /* End loop */
			}

			/* Invoke callback and wait for next */
			(void) saLogDispatch(log_handle, SA_DISPATCH_ONE);
			/* Callback error handling */
			if (lgt_cb.ais_errno != SA_AIS_OK) {
				fprintf(stderr, "\t%s Write callback error %s",
					__FUNCTION__, saf_error(lgt_cb.ais_errno));
				break;
			}
			if (lgt_cb.invocation_in != lgt_cb.invocation_out) {
				fprintf(stderr, "\t%s Write invocation id error"
					"in = %lld, out = %lld\n",__FUNCTION__,
					lgt_cb.invocation_in, lgt_cb.invocation_out);
				break;
			}
		}

		/* If rotation is done very often (very small log files) the
		 * rotated log files sometimes ends up with the
		 * same name => overwrite! This is because a timestamp is used
		 * in the file name name and the resolution is one second.
		 * In this test very small log files are used and to get around
		 * the problem a delay of 1 sec is inserted every 10th log
		 * record
		 */
		if (i%10 == 0) {
			printf_s(".");
			fflush(stdout);
			sleep(1);
		}
	}
	printf_s("\n");

	return ais_rc;
}

/**
 * Close the log stream 'logStreamHandle'
 *
 * @param logStreamHandle
 * @return AIS return code
 */
static SaAisErrorT tst_StreamClose(SaLogStreamHandleT logStreamHandle)
{
	int trycnt = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;

	/* Close log Stream */
	do {
		/* Try again loop. If other error end loop */
		ais_rc = saLogStreamClose(logStreamHandle);
		if (ais_rc != SA_AIS_ERR_TRY_AGAIN) {
			break;
		}
		usleep(sleep_delay_us);
		if (++trycnt > trymax) {
			/* Give up try again */
			break;
		}
	} while (ais_rc != SA_AIS_OK);

	return ais_rc;
}

/**
 * Finalize the log API
 *
 * @param logHandle
 * @return AIS return code
 */
static SaAisErrorT tst_LogFinalize(SaLogHandleT logHandle)
{
	int trycnt = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;

	/* Close log Stream */
	do {
		/* Try again loop. If other error end loop */
		ais_rc = saLogFinalize(logHandle);
		if (ais_rc != SA_AIS_ERR_TRY_AGAIN) {
			break;
		}
		usleep(sleep_delay_us);
		if (++trycnt > trymax) {
			/* Give up try again */
			break;
		}
	} while (ais_rc != SA_AIS_OK);

	return ais_rc;
}

#if 0
/**
 * Thread for handling write callback using a selection object and a poll loop
 *
 * @param logHandle [in]
 * @return
 */
static void *callback_hdl_thread(void *logHandle_in)
{
	SaSelectionObjectT selectionObject;
	SaAisErrorT ais_rc = SA_AIS_OK;
	SaLogHandleT log_handle = *(SaLogHandleT *) logHandle_in;
	struct pollfd fds[1];
	int poll_rc = 0;

	osaf_mutex_lock_ordie(&write_mutex);

	/* Get selection object for handling write callback */
	ais_rc = saLogSelectionObjectGet(log_handle, &selectionObject);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s; saLogSelectionObjectGet Fail %s\n",
			__FUNCTION__, saf_error(ais_rc));
		/* On error for now end test */
		exit(1);
	}

	/* Prepare for polling */
	fds[0].fd = (int) selectionObject;
	fds[0].events = POLLIN;

	osaf_mutex_unlock_ordie(&write_mutex);

	/* Wait until the log record is written before writing the next one.
	 * Wait for selection object and invoke the callback.
	 */
	while (1) {
		errno = 0;
		poll_rc = poll(fds, 1, -1); /* Never timeout */
		if (poll_rc <= 0){
			/* An error or timeout
			 */
			if (poll_rc == -1) {
				if (errno != EAGAIN) {
					fprintf(stderr, "\t%s; Poll fail\n",
						__FUNCTION__);
					break; /* End loop and thread */
				} else {
					continue; /* Continue wait on poll */
				}
			} else {
				fprintf(stderr, "\t%s; Poll timeout\n",
					__FUNCTION__);
				break; /* End loop and thread */
			}
		}

		/* Invoke callback and wait for next */
		(void) saLogDispatch(log_handle, SA_DISPATCH_ONE);
	}

	return NULL; /* Dummy. Return value not used */
}

typedef struct {
	int wr_delay; /* Delay in ms between writing records */
	int cnum; /* Client number */
	int snum; /* Clients stream number */
} write_rec_thread_in_t;
#endif

/**
 * Callback function used with the log service
 *
 * Global SaAisErrorT log_write_error = error
 * @param invocation [in]
 * @param error [in]
 */
static void log_write_callback(SaInvocationT invocation, SaAisErrorT error)
{
	printf_v(">> LLDTEST %s\n",__FUNCTION__);
	if (error != SA_AIS_OK) {
		fprintf(stderr,"\t%s: error = %s\n",__FUNCTION__, saf_error(error));
	}

	if (invocation != lgt_cb.invocation_in) {
		fprintf(stderr,"\t%s: invocation=%lld expected %lld\n",
			__FUNCTION__, invocation, lgt_cb.invocation_in);
	}

	if (error != SA_AIS_OK) {
		fprintf(stderr, "\t%s LGS error %s\n", __FUNCTION__, saf_error(error));
	}

	osaf_mutex_lock_ordie(&write_mutex);
	ais_rc_cnt.callback_cnt++;
	lgt_cb.ais_errno = error;
	lgt_cb.invocation_out = invocation;
	osaf_mutex_unlock_ordie(&write_mutex);
	printf_v("<< LLDTEST %s\n",__FUNCTION__);
}

/**
 * Wrapper for writing log records to a stream
 *
 * @param client_num[in]  Client number to write
 * @param stream_num[in]  Stream number to write
 * @param num_rec[in]     Number of records to write
 * @param log_msg[in]     Message in (all) log records
 * @return -1 on error
 */
static int saLogRecov_write(
	int client_num,
	int stream_num,
	int num_rec,
	char *log_msg,
	lgt_log_rec_type_t rec_type)
{
	int rc = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;
	char msg_str[256];

	(void) snprintf(msg_str, 256, "Client %d, stream %d, \"%s\"",
		client_num, stream_num, log_msg);

	ais_rc = tst_WriteRec(
		num_rec,				/* Number of records */
		g_client[client_num].glob_logStreamHandle[stream_num],
		g_client[client_num].glob_logHandle,
		msg_str,				/* Log message */
		true,					/* Handle TRY AGAIN */
		false,					/* Do not handle callback */
		rec_type
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "%s Write Fail %s\n",__FUNCTION__,
			saf_error(ais_rc));
		rc = -1;
	}

	return rc;
}

/* **********************
 * Selectable test cases
 */

/******************************************************************************
 * Test suite 9
 * Test headless version 2 if "scAbsenceAllowed_attr" is set to
 * "IS allowed" (recover)
 ******************************************************************************/

/**
 * Request tester to start at least one SC node and press enter when done
 * This is not a test case and will always pass
 */
void saLogRecov_req_node_start(void)
{
	/* Wait for tester to start SC nodes
	 */
	print_t(TST_TAG_NU);
	printf_s("\nStart at least one SC node. ");
	(void) wait_ext_act("Press Enter when started...");
}

/**
 * Request tester to stop both SC nodes.
 * This is not a test case and will always pass
 */
void saLogRecov_req_node_stop(void)
{
	/* Wait for tester to stop SC nodes
	 */
	print_t(TST_TAG_ND);
	printf_s("\nStop both SC nodes [stdby first]. ");
	(void) wait_ext_act("Press Enter when stopped...");
}

/**
 * Pause testing and wait for tester to press Enter.
 * Can be used e.g. at a point where examining the log files is a good idea
 */
void saLogRecov_pause_s(void)
{
	printf_s("\nTest paused. ");
	wait_ext_act("Press Enter to continue...");
}

/**
 * Delete all files in the test directory and remove the directory.
 * Do not fail if command is not successful
 */
void saLogRecov_clean_tstdir(void)
{
	int rc = 0;
	char command[PATH_MAX + 256];
	char tstpath[PATH_MAX + 256];

	sprintf(tstpath, "%s/%s", log_root_path, logfile_path_str);
	printf_v("Clean test dir \"%s\"\n", tstpath);

	sprintf(command, "rm -rf %s/", tstpath);
	rc = tet_system(command);
	if (rc != 0) {
		printf_v("%s system(%s) Fail %s\n",
			__FUNCTION__, command, strerror(errno));
	}
}

/**
 * Exit test program
 */
void saLogRecov_exit(void)
{
	printf_s("\nTest done. ");
	//wait_ext_act("Press Enter to exit...");
	exit(0);
}

/**
 * Prepare one client
 * Server shall be up and running in a normal way
 * - Initialize
 * - Open a test stream
 * - Write some log records
 *
 * Results in open stream and an existing current log file with content
 *
 * - Inform tester to take down SC nodes and press enter when done
 *
 */
void saLogRecov_prepare_client1(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;
	const int rec_no = 3;
	int rc = 0;

	ais_rc = tst_LogInitialize(&g_client[0].glob_logHandle, 5);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr,"\t%s Failed to initialize log service: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\nInitialized\n");

	rc = tst_StreamOpen_app_logtest_sc(g_client[0].glob_logHandle,
		g_client[0].glob_logStreamHandle, 1);
	if (rc == -1) {
		fprintf(stderr, "\t%s Failed to open log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}

	ais_rc = tst_WriteRec(
		rec_no,				/* Write no of records */
		g_client[0].glob_logStreamHandle[0],
		g_client[0].glob_logHandle,
		"Written before server down (client1)",	/* Log message */
		true,				/* Handle TRY AGAIN */
		true,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\n\t %d Log records written\n", rec_no);

	done:
	rc_validate(tst_res, TST_PASS);
	if (tst_res == TST_FAIL) {
		/* There is no meaning to continue */
		exit(TST_FAIL);
	}
}

/**
 * Write a log record while server down
 * The reply shall be TRY AGAIN directly from write API
 */
void saLogRecov_91(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;

	ais_rc = tst_WriteRec(
		1,					/* Write 1 record */
		g_client[0].glob_logStreamHandle[0],
		g_client[0].glob_logHandle,
		"Written while server down (client1)",	/* Log message */
		false,					/* Do not handle TRY AGAIN */
		false,					/* Do not handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_ERR_TRY_AGAIN) {
		printf_s("\tWrite when server down Fail\n"
			 "\tGot '%s' Expected 'SA_AIS_ERR_TRY_AGAIN'\n",
			saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}

	done:
	rc_validate(tst_res, TST_PASS);
}

/**
 * Write some log records to client 1 after server up
 * The reply shall be OK and a log record shall be written in correct file
 * and the log record shall have an Id that is previous Id + 1
 */
void saLogRecov_92(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;

	ais_rc = tst_WriteRec(
		3,				/* Write 3 record */
		g_client[0].glob_logStreamHandle[0],
		g_client[0].glob_logHandle,
		"Written after server up (client1)",	/* Log message */
		true,				/* Handle TRY AGAIN */
		true,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}

	done:
	rc_validate(tst_res, TST_PASS);
}

/**
 * Prepare one more client
 * Server shall be up and running in a normal way
 *
 * - Initialize
 * - Open a test stream
 * - Write some log records
 *
 * Results in two open streams and two current log files with content
 *
 * - Inform tester to take down SC nodes and press enter when done
 *
 */
void saLogRecov_prepare_client2(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;
	const int rec_no = 3;
	int rc = 0;

	ais_rc = tst_LogInitialize(&g_client[1].glob_logHandle, 5);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr,"\t%s Failed to initialize client 2: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\nInitialized\n");

	rc = tst_StreamOpen_app_logtest_sc(g_client[1].glob_logHandle,
		g_client[1].glob_logStreamHandle, 1);
	if (rc == -1) {
		fprintf(stderr, "\t%s Failed to open log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}

	ais_rc = tst_WriteRec(
		rec_no,				/* Write no of records */
		g_client[1].glob_logStreamHandle[0],
		g_client[1].glob_logHandle,
		"Written before server down (client2)",	/* Log message */
		true,				/* Handle TRY AGAIN */
		true,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\n %d Log records written\n", rec_no);

	done:
	rc_validate(tst_res, TST_PASS);
	if (tst_res == TST_FAIL) {
		/* There is no meaning to continue */
		exit(TST_FAIL);
	}
}

/**
 * Write some log records to client 2 after server up
 * The reply shall be OK and a log record shall be written in correct file
 * and the log record shall have an Id that is previous Id + 1
 */
void saLogRecov_93(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;

	ais_rc = tst_WriteRec(
		3,				/* Write 3 record */
		g_client[1].glob_logStreamHandle[0],
		g_client[1].glob_logHandle,
		"Written after server up (client2)",	/* Log message */
		true,				/* Handle TRY AGAIN */
		true,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}

	done:
	rc_validate(tst_res, TST_PASS);
}

/**
 * Write some log records to client 1 again after server up a second time
 * The reply shall be OK and a log record shall be written in correct file
 * and the log record shall have an Id that is previous Id + 1
 */
void saLogRecov_94(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;

	ais_rc = tst_WriteRec(
		3,				/* Write 3 record */
		g_client[0].glob_logStreamHandle[0],
		g_client[0].glob_logHandle,
		"Written after server up 2nd time (client1)",	/* Log message */
		true,				/* Handle TRY AGAIN */
		true,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}

	done:
	rc_validate(tst_res, TST_PASS);
}

/**
 * "SC nodes restarted: Close stream1"
 */
void saLogRecov_95(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;

	ais_rc = tst_StreamClose(g_client[0].glob_logStreamHandle[0]);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to close log stream: %s\n", __FUNCTION__,
			saf_error(ais_rc));
		tst_res = TST_FAIL;
	}

	rc_validate(tst_res, TST_PASS);
}

/**
 * "SC nodes restarted: Close stream2"
 */
void saLogRecov_96(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;

	ais_rc = tst_StreamClose(g_client[1].glob_logStreamHandle[0]);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to close log stream: %s\n", __FUNCTION__,
			saf_error(ais_rc));
		tst_res = TST_FAIL;
	}

	rc_validate(tst_res, TST_PASS);
}

/**
 * "SC nodes restarted: Finalize client1"
 */
void saLogRecov_97(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;

	ais_rc = tst_LogFinalize(g_client[0].glob_logHandle);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Unexpected AIS return code %s. "
			"Expected SA_AIS_OK\n", __FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
	}

	rc_validate(tst_res, TST_PASS);
}

/**
 * "SC nodes restarted: Finalize client2"
 */
void saLogRecov_98(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;

	ais_rc = tst_LogFinalize(g_client[1].glob_logHandle);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Unexpected AIS return code %s. "
			"Expected SA_AIS_OK\n", __FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
	}

	rc_validate(tst_res, TST_PASS);
}

/******************************************************************************
 * Test the following stream configurations
 * Stream configuration:
 *
 * "Old" rotated log files exist:
 * 0. Has more than one "old" log file and a log file with more than one log record
 * 1. Has more than one "old" log file and an empty current log file
 * 2. Has more than one "old" log file but no current log file
 * 3. Has one "old" log file but no current log file
 *
 * No "old" rotated log files:
 * 4. Has a current log file containing more than one log record
 * 5. Has a current log file containing one log record
 * 6. Has a current log file that is empty
 * 7. Has no current log file
 ******************************************************************************/

/**
 * Prepare a client with 8 streams.
 * Four streams shall have "rotated" log files
 * Two streams shall have empty log files
 * Two streams shall have no current log file
 */
void saLogRecov_prepare_client1_8streams(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;
	int rc = 0;
	char cur_logfile_str[256];
	char name_prefix[256];

	char log_rec_str[256];

	printf_s("Preparing 8 streams for test. Please wait...\n");
	ais_rc = tst_LogInitialize(&g_client[0].glob_logHandle, 5);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr,"\t%s Failed to initialize log service: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\tInitialized\n");

	rc = tst_StreamOpen_app_logtest_sc(g_client[0].glob_logHandle,
		g_client[0].glob_logStreamHandle,
		8); /* LLDTEST shall be 8 */
	if (rc != 0) {
		fprintf(stderr, "\t%s Failed to open log stream\n",
			__FUNCTION__);
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\tStreams opened\n");

	/* ********************************************************************
	 * Stream 0:
	 * Has more than one "old" log file and a log file with more than one
	 * log record
	 */
	printf_s("Preparing stream 0\n");
	 /* Create a big log record (200 bytes) */
	tst_max_logrec_create(log_rec_str, 200, "Before SC down ");
	ais_rc = tst_WriteRec(
		225,				/* Write no of records */
		g_client[0].glob_logStreamHandle[0],
		g_client[0].glob_logHandle,
		log_rec_str,			/* Log message */
		true,				/* Handle TRY AGAIN */
		false,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\tStream 0: rotated > 1, cur log > 1\n");

	/* ********************************************************************
	 * Stream 1:
	 * Has more than one "old" log file and an empty current log file
	 */
	printf_s("Preparing stream 1\n");
	 /* Create a big log record (200 bytes) */
	tst_max_logrec_create(log_rec_str, 200, "Before SC down ");
	ais_rc = tst_WriteRec(
		221,				/* Write no of records */
		g_client[0].glob_logStreamHandle[1],
		g_client[0].glob_logHandle,
		log_rec_str,			/* Log message */
		true,				/* Handle TRY AGAIN */
		false,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}

	sprintf(name_prefix, "%s_file_1", STREAM_NAME_9);
	rc = cur_logfile_name_get(cur_logfile_str, name_prefix);
	if (rc == -1) {
		fprintf(stderr, "%s Internal error. Test failed\n",__FUNCTION__);
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("Stream 1 current log file \"%s\"\n", cur_logfile_str);
	(void) clear_file(cur_logfile_str);
	printf_v("\tStream 1: rotated > 1, cur log = 0\n");

	/* ********************************************************************
	 * Stream 2:
	 * Has more than one "old" log file but no current log file
	 */
	printf_s("Preparing stream 2\n");
	 /* Create a big log record (200 bytes) */
	tst_max_logrec_create(log_rec_str, 200, "Before SC down ");
	ais_rc = tst_WriteRec(
		221,				/* Write no of records */
		g_client[0].glob_logStreamHandle[2],
		g_client[0].glob_logHandle,
		log_rec_str,			/* Log message */
		true,				/* Handle TRY AGAIN */
		false,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}

	sprintf(name_prefix, "%s_file_2", STREAM_NAME_9);
	rc = cur_logfile_name_get(cur_logfile_str, name_prefix);
	if (rc == -1) {
		fprintf(stderr, "%s Internal error. Test failed\n",__FUNCTION__);
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("Stream 2 current log file \"%s\"\n", cur_logfile_str);

	(void) delete_file(cur_logfile_str);
	printf_v("\tStream 2: rotated > 1, No cur log\n");

#if 1 /* Temporary remove */
	/* ********************************************************************
	 * Stream 3:
	 * Has one "old" log file but no current log file
	 */
	printf_s("Preparing stream 3\n");
	 /* Create a big log record (200 bytes) */
	tst_max_logrec_create(log_rec_str, 200, "Before SC down ");
	ais_rc = tst_WriteRec(
		15,				/* Write no of records */
		g_client[0].glob_logStreamHandle[3],
		g_client[0].glob_logHandle,
		log_rec_str,			/* Log message */
		true,				/* Handle TRY AGAIN */
		false,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}

	sprintf(name_prefix, "%s_file_3", STREAM_NAME_9);
	rc = cur_logfile_name_get(cur_logfile_str, name_prefix);
	if (rc == -1) {
		fprintf(stderr, "%s Internal error. Test failed\n",__FUNCTION__);
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("Stream 3 current log file \"%s\"\n", cur_logfile_str);

	(void) delete_file(cur_logfile_str);
	printf_v("\tStream 3: rotated = 1, No cur log\n");

	/* ********************************************************************
	 * Stream 4:
	 * Has a current log file containing more than one log record
	 */
	printf_s("Preparing stream 4\n");
	 /* Create a big log record (200 bytes) */
	tst_max_logrec_create(log_rec_str, 200, "Before SC down ");
	ais_rc = tst_WriteRec(
		15,				/* Write no of records */
		g_client[0].glob_logStreamHandle[4],
		g_client[0].glob_logHandle,
		log_rec_str,			/* Log message */
		true,				/* Handle TRY AGAIN */
		false,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\tStream 4: No rotated, cur log > 1\n");

	/* ********************************************************************
	 * Stream 5:
	 * Has a current log file containing one log record
	 */
	printf_s("Preparing stream 5\n");
	 /* Create a big log record (200 bytes) */
	tst_max_logrec_create(log_rec_str, 200, "Before SC down ");
	ais_rc = tst_WriteRec(
		1,				/* Write no of records */
		g_client[0].glob_logStreamHandle[5],
		g_client[0].glob_logHandle,
		log_rec_str,			/* Log message */
		true,				/* Handle TRY AGAIN */
		false,				/* Handle callback */
		GEN_LOG_REC
	);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr, "\t%s Failed to write to log stream: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\tStream 5: No rotated, cur log = 1\n");

	/* ********************************************************************
	 * Stream 6:
	 * Has a current log file that is empty
	 */
	printf_s("Preparing stream 6\n");
	/* Nothing to do after the stream is opened */
	printf_v("\tStream 6 No rotated, cur log = 0\n");

	/* ********************************************************************
	 * Stream 7:
	 * Has no current log file
	 */
	printf_s("Preparing stream 7\n");
	/* Just delete the current log file */
	sprintf(name_prefix, "%s_file_7", STREAM_NAME_9);
	rc = cur_logfile_name_get(cur_logfile_str, name_prefix);
	if (rc == -1) {
		fprintf(stderr, "%s Internal error. Test failed\n",__FUNCTION__);
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("Stream 7 current log file \"%s\"\n", cur_logfile_str);

	(void) delete_file(cur_logfile_str);
	printf_v("\tStream 7 No rotated, No cur log\n");
#endif

	done:
	rc_validate(tst_res, TST_PASS);
	if (tst_res == TST_FAIL) {
		/* There is no meaning to continue */
		exit(TST_FAIL);
	}
}

/**
 * Write log records to stream 0 after server down/up
 * Write 3 records to client 0, stream 0
 *
 */
void saLogRecov_write_0(void)
{
	int tst_res = TST_PASS;

	if (saLogRecov_write(0, 0, 3, "After Server UP ", GEN_LOG_REC) == -1)
		tst_res = TST_FAIL;

	rc_validate(tst_res, TST_PASS);

	// Close stream
	tst_StreamClose(g_client[0].glob_logStreamHandle[0]);
}

/**
 * Write log records to stream 1 after server down/up
 * Write 3 records to client 0, stream 1
 *
 */
void saLogRecov_write_1(void)
{
	int tst_res = TST_PASS;

	if (saLogRecov_write(0, 1, 3, "After Server UP ", GEN_LOG_REC) == -1)
		tst_res = TST_FAIL;

	rc_validate(tst_res, TST_PASS);

	// Close stream
	tst_StreamClose(g_client[0].glob_logStreamHandle[1]);
}

/**
 * Write log records to stream 2 after server down/up
 * Write 3 records to client 0, stream 2
 *
 */
void saLogRecov_write_2(void)
{
	int tst_res = TST_PASS;

	if (saLogRecov_write(0, 2, 3, "After Server UP ", GEN_LOG_REC) == -1)
		tst_res = TST_FAIL;

	rc_validate(tst_res, TST_PASS);

	// Close stream
	tst_StreamClose(g_client[0].glob_logStreamHandle[2]);
}

/**
 * Write log records to stream 3 after server down/up
 * Write 3 records to client 0, stream 3
 *
 */
void saLogRecov_write_3(void)
{
	int tst_res = TST_PASS;

	if (saLogRecov_write(0, 3, 3, "After Server UP ", GEN_LOG_REC) == -1)
		tst_res = TST_FAIL;

	rc_validate(tst_res, TST_PASS);

	// Close stream
	tst_StreamClose(g_client[0].glob_logStreamHandle[3]);
}

/**
 * Write log records to stream 4 after server down/up
 * Write 3 records to client 0, stream 4
 *
 */
void saLogRecov_write_4(void)
{
	int tst_res = TST_PASS;

	if (saLogRecov_write(0, 4, 3, "After Server UP ", GEN_LOG_REC) == -1)
		tst_res = TST_FAIL;

	rc_validate(tst_res, TST_PASS);

	// Close stream
	tst_StreamClose(g_client[0].glob_logStreamHandle[4]);
}

/**
 * Write log records to stream 5 after server down/up
 * Write 3 records to client 0, stream 5
 *
 */
void saLogRecov_write_5(void)
{
	int tst_res = TST_PASS;

	if (saLogRecov_write(0, 5, 3, "After Server UP ", GEN_LOG_REC) == -1)
		tst_res = TST_FAIL;

	rc_validate(tst_res, TST_PASS);

	// Close stream
	tst_StreamClose(g_client[0].glob_logStreamHandle[5]);
}

/**
 * Write log records to stream 6 after server down/up
 * Write 3 records to client 0, stream 6
 *
 */
void saLogRecov_write_6(void)
{
	int tst_res = TST_PASS;

	if (saLogRecov_write(0, 6, 3, "After Server UP ", GEN_LOG_REC) == -1)
		tst_res = TST_FAIL;

	rc_validate(tst_res, TST_PASS);

	// Close stream
	tst_StreamClose(g_client[0].glob_logStreamHandle[6]);
}

/**
 * Write log records to stream 7 after server down/up
 * Write 3 records to client 0, stream 7
 *
 */
void saLogRecov_write_7(void)
{
	int tst_res = TST_PASS;

	if (saLogRecov_write(0, 7, 3, "After Server UP ", GEN_LOG_REC) == -1)
		tst_res = TST_FAIL;

	rc_validate(tst_res, TST_PASS);

	// Close stream
	tst_StreamClose(g_client[0].glob_logStreamHandle[7]);
}

/**
 * Write log records to a stream with configuration object before SC nodes down
 * Writing to notification stream
 */
void saLogRecov_prepare_client2_1_cfg_stream(void)
{
	int tst_res = TST_PASS;
	SaAisErrorT ais_rc = SA_AIS_OK;
	int rc = 0;

	//char log_rec_str[256];

	/* Initialize log service */
	printf_s("Preparing 1 config stream (notification) for test . Please wait...\n");
	ais_rc = tst_LogInitialize(&g_client[1].glob_logHandle, 5);
	if (ais_rc != SA_AIS_OK) {
		fprintf(stderr,"\t%s Failed to initialize log service: %s\n",
			__FUNCTION__, saf_error(ais_rc));
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\tInitialized\n");

	/* Open stream */
	rc = tst_StreamOpen_cfg_logtest_sc(g_client[1].glob_logHandle,
		&g_client[1].glob_logStreamHandle[0]);
	if (rc != 0) {
		fprintf(stderr, "\t%s Failed to open log stream\n",
			__FUNCTION__);
		tst_res = TST_FAIL;
		goto done;
	}
	printf_v("\tStreams opened\n");

	/* Write to stream */
	if (saLogRecov_write(1, 0, 3, "Before SC down ", NTF_LOG_REC) == -1)
		tst_res = TST_FAIL;

	done:
	rc_validate(tst_res, TST_PASS);
	if (tst_res == TST_FAIL) {
		/* There is no meaning to continue */
		exit(TST_FAIL);
	}
}

/**
 * Write log records to Notification stream after server down/up
 * Write 3 records to client 1, stream 0
 *
 */
void saLogRecov_write_notif(void)
{
	int tst_res = TST_PASS;

	if (saLogRecov_write(1, 0, 3, "After Server UP ", NTF_LOG_REC) == -1)
		tst_res = TST_FAIL;

	rc_validate(tst_res, TST_PASS);
}

//>>
// Functions and global variables for test suite #11
//<<

static SaLogStreamHandleT logStrHandleRecv;
static SaLogHandleT logHandleRecv;

/**
 * Open an runtime stream
 */
void saLogRecov_openRtStream(void)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaNameT logStreamName = {.length = 0 };
	char command[300];
	int sysRc;

	strcpy((char *)logStreamName.value, "safLgStr=rtCleanup");
	logStreamName.length = strlen((char *)logStreamName.value);

	SaLogFileCreateAttributesT_2 appLogFileCreateAttributes;

	/* Cleanup the test directory */
	sprintf(command, "rm -rf %s/logtest_rtStream_cleanup", log_root_path);
	sysRc = system(command);
	if (WEXITSTATUS(sysRc)) {};

	/* App stream information */
	appLogFileCreateAttributes.logFilePathName = "logtest_rtStream_cleanup";
	appLogFileCreateAttributes.logFileName = "logtest_rtStream_cleanup";
	appLogFileCreateAttributes.maxLogFileSize = DEFAULT_APP_LOG_FILE_SIZE;
	appLogFileCreateAttributes.maxLogRecordSize = DEFAULT_APP_LOG_REC_SIZE;
	appLogFileCreateAttributes.haProperty = SA_TRUE;
	appLogFileCreateAttributes.logFileFullAction = SA_LOG_FILE_FULL_ACTION_ROTATE;
	appLogFileCreateAttributes.maxFilesRotated = 4;
	appLogFileCreateAttributes.logFileFmt = NULL;

	rc = saLogInitialize(&logHandleRecv, &logCallbacks, &logVersion);
	if (rc != SA_AIS_OK) {
		fprintf(stderr, "Failed at saLogInitialize: %d\n ", (int)rc);
		test_validate(rc, SA_AIS_OK);
		return;
	}

	rc = saLogStreamOpen_2(logHandleRecv, &logStreamName, &appLogFileCreateAttributes,
			       SA_LOG_STREAM_CREATE, SA_TIME_ONE_SECOND, &logStrHandleRecv);
	test_validate(rc, SA_AIS_OK);
}

/* Close the runtime log stream */
void saLogRecov_closeRtStream(void)
{
	SaAisErrorT rc = SA_AIS_OK;

	/* Make sure client informed headless */
	sleep(1);

	rc = saLogStreamClose(logStrHandleRecv);
	if (rc != SA_AIS_OK) {
		fprintf(stderr, "Failed at saLogStreamClose: %d\n ", (int)rc);
		test_validate(rc, SA_AIS_OK);
		return;
	}

	test_validate(rc, SA_AIS_OK);

	(void) saLogFinalize(logHandleRecv);
}

/**
 * Verify if the cfg/log file of recovered runtime stream
 * is renamed by appending close time to their names
 * when the timer (10 mins) in server is expired.
 */
void saLogRecov_verRtStream_cleanup(void)
{
	int rc = 0;
	char command[300];
	int dot = 11*60, line = 0;

	printf_s("\n");
	printf_s("########################################################\n");
	printf_s("### This TC will hang in 11 MINUTES                  ###\n");
	printf_s("### To make sure the tmr in server (10 mins) expired ###\n");
	printf_s("########################################################\n");

	while (dot--) {
		if (line == 0) printf_s("%02d.\t", line++);
		printf_s(".");
		fflush(stdout);
		sleep(1);
		if (dot != 0 && dot % 60 == 0) printf_s("\n%02d.\t", line++);
	}

	printf_s("\n");

	sprintf(command, "find %s/logtest_rtStream_cleanup -type f -name \"logtest_*\" "
		"| egrep \"%s_[0-9]{8}_[0-9]{6}_[0-9]{8}_[0-9]{6}\\.log$\" ",
		log_root_path, "logtest_rtStream_cleanup");
	rc = system(command);
	if (WEXITSTATUS(rc)) {
		/* Fail to perform command. Report test failed. */
		fprintf(stderr, "Failed to perform command = %s\n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	sprintf(command, "find %s/logtest_rtStream_cleanup -type f -name \"logtest_*\" "
		"| egrep \"%s_[0-9]{8}_[0-9]{6}\\.cfg$\" ",
		log_root_path, "logtest_rtStream_cleanup");
	if (WEXITSTATUS(rc)) {
		/* Fail to perform command. Report test failed. */
		fprintf(stderr, "Failed to perform command = %s\n", command);
		rc_validate(WEXITSTATUS(rc), 0);
		goto done;
	}

	rc_validate(WEXITSTATUS(rc), 0);
done:
	/* Cleanup the test directory */
	sprintf(command, "rm -rf %s/logtest_rtStream_cleanup", log_root_path);
	rc = system(command);
}


//>
// For test suite #12
//<
/**
 * Create 02 configurable app streams
 */
void saLogRecov_createCfgAppStreams()
{
	int rc;
	char command[256];

	/* DN under RDN safApp=safLogservice */
	sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=testCfgAppStream,safApp=safLogService "
		"-a saLogStreamFileName=testCfgAppStream -a saLogStreamPathName=suite12");
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);

	/* DN not under safApp=safLogService */
	sprintf(command, "immcfg -c SaLogStreamConfig safLgStrCfg=testCfgAppStream1 "
		"-a saLogStreamFileName=testCfgAppStream1 -a saLogStreamPathName=suite12");
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Create 02 configurable app streams
 */
void saLogRecov_delCfgAppStreams()
{
	int rc;
	char command[256];

	/* DN under RDN safApp=safLogservice */
	sprintf(command, "immcfg -d safLgStrCfg=testCfgAppStream,safApp=safLogService ");
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);

	/* DN not under safApp=safLogService */
	sprintf(command, "immcfg -d safLgStrCfg=testCfgAppStream1");
	rc = system(command);
	rc_validate(WEXITSTATUS(rc), 0);
}

/**
 * Test suite 9
 * Testing recovery of "runtime" app streams
 * Testing recovery of "configuration" stream. Notification stream is used
 * Tests that are not automatically added to logtest. These tests will not be
 * part of the test suites that is executed with command 'logtest' without any
 * options.
 */
void add_suite_9(void)
{
	test_suite_add(9, "LOG Server down / up");

	/* Clean and remove the test directory if exist */
	test_case_add(9, saLogRecov_clean_tstdir, "");

	/* Test 8 application streams. Different file combinations */
	test_case_add(9, saLogRecov_prepare_client1_8streams, "Before SC down: Prepare 8 streams");
	test_case_add(9, saLogRecov_prepare_client2_1_cfg_stream,
		"Before SC down: Prepare 1 cfg (notif) stream");
	test_case_add(9, saLogRecov_req_node_stop, ""); /* Stop SC nodes */

	test_case_add(9, saLogRecov_req_node_start, ""); /* Start SC nodes */
	test_case_add(9, saLogRecov_write_0, "After SC up: Write to stream 0");
	test_case_add(9, saLogRecov_write_1, "After SC up: Write to stream 1");
	test_case_add(9, saLogRecov_write_2, "After SC up: Write to stream 2");
	test_case_add(9, saLogRecov_write_3, "After SC up: Write to stream 3");
	test_case_add(9, saLogRecov_write_4, "After SC up: Write to stream 4");
	test_case_add(9, saLogRecov_write_5, "After SC up: Write to stream 5");
	test_case_add(9, saLogRecov_write_6, "After SC up: Write to stream 6");
	test_case_add(9, saLogRecov_write_7, "After SC up: Write to stream 7");
	test_case_add(9, saLogRecov_write_notif, "After SC up: Write to notification stream");

	/* Test one configuration stream */
	test_case_add(9, saLogRecov_prepare_client2_1_cfg_stream,
		"Before SC down: Prepare 1 cfg (notif) stream");
	test_case_add(9, saLogRecov_req_node_stop, ""); /* Stop SC nodes */

	test_case_add(9, saLogRecov_req_node_start, ""); /* Start SC nodes */
	test_case_add(9, saLogRecov_write_notif, "After SC up: Write to notification stream");
	//test_case_add(9, saLogRecov_exit, "");

	//test_case_add(9, saLogRecov_pause_s, "");
	//test_case_add(9, saLogRecov_exit, "");
	//test_case_add(9, saLogRecov_clean_tstdir, "");

}

/**
 * Test suite 10
 * Some additional test for testing recovery of application streams using more
 * than one client
 * Tests that are not automatically added to logtest. These tests will not be
 * part of the test suites that is executed with command 'logtest' without any
 * options.
 */
void add_suite_10(void)
{
	test_suite_add(10, "LOG Server down/up: additional tests");

	/* Clean and remove the test directory if exist */
	test_case_add(10, saLogRecov_clean_tstdir, "");

	test_case_add(10, saLogRecov_prepare_client1, "Prepare client1");
	test_case_add(10, saLogRecov_req_node_stop, ""); /* Stop SC nodes */
	test_case_add(10, saLogRecov_91, "SC nodes stopped: Write on open stream1");
	test_case_add(10, saLogRecov_req_node_start, ""); /* Start SC nodes */
	test_case_add(10, saLogRecov_92, "SC nodes started: Write on open stream1");

	test_case_add(10, saLogRecov_prepare_client2, "Prepare one more client and stop SC nodes");
	test_case_add(10, saLogRecov_req_node_stop, ""); /* Stop SC nodes */
	test_case_add(10, saLogRecov_req_node_start, ""); /* Start SC nodes */
	test_case_add(10, saLogRecov_93, "SC nodes started: Write on open stream2");
	test_case_add(10, saLogRecov_94, "SC nodes started: Write on open stream1 second time");
	test_case_add(10, saLogRecov_95, "SC nodes restarted: Close stream1");
	test_case_add(10, saLogRecov_96, "SC nodes restarted: Close stream2");
	test_case_add(10, saLogRecov_97, "SC nodes restarted: Finalize client1");
	test_case_add(10, saLogRecov_98, "SC nodes restarted: Finalize client2");

	//test_case_add(10, saLogRecov_clean_tstdir, "");
}

/**
 * Test suite 11
 * Verify streamClose is OK when headless. And also verify
 * if cfg/log file name of recovered runtime stream are appended close time
 * when the timer (10 mins) is expired in server side.
 */
void add_suite_11(void)
{
	test_suite_add(11, "LOG Server down/up: verify closeStream and cleanup abandoned streams");
	test_case_add(11, saLogRecov_openRtStream, "Open an runtime stream");
	test_case_add(11, saLogRecov_req_node_stop, ""); /* Stop SC nodes */
	test_case_add(11, saLogRecov_closeRtStream, "SC nodes stopped: close the stream, OK");
	test_case_add(11, saLogRecov_req_node_start, ""); /* Start SC nodes */
	test_case_add(11, saLogRecov_verRtStream_cleanup, "SC nodes started: after 10 mins, the stream is cleanup, OK");
}

/**
 * Test suite 12
 * Verify created configurable app streams can be deleted after headless.
 */
void add_suite_12(void)
{
	test_suite_add(12, "LOG Server down/up: verify app streams can be deleted after headless");
	test_case_add(12, saLogRecov_createCfgAppStreams, "Create 02 configurable app streams");
	test_case_add(12, saLogRecov_req_node_stop, ""); /* Stop SC nodes */
	test_case_add(12, saLogRecov_req_node_start, ""); /* Start SC nodes */
	test_case_add(12, saLogRecov_delCfgAppStreams, "SC nodes started: delete created app streams");
}
