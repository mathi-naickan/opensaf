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

#include <stdlib.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "immutil.h"
#include "lgs.h"
#include "lgs_util.h"
#include "lgs_fmt.h"

#define ALARM_STREAM_ENV_PREFIX "ALARM"
#define NOTIFICATION_STREAM_ENV_PREFIX "NOTIFICATION"
#define SYSTEM_STREAM_ENV_PREFIX "SYSTEM"
#define LGS_CREATE_CLOSE_TIME_LEN 16
#define START_YEAR 1900
#define LOG_VER_EXP "LOG_SVC_VERSION:"
#define FMAT_EXP "FORMAT:"
#define CFG_EXP_MAX_FILE_SIZE "MAX_FILE_SIZE:"
#define CFG_EXP_FIXED_LOG_REC_SIZE "FIXED_LOG_REC_SIZE:"
#define CFG_EXP_LOG_FULL_ACTION "LOG_FULL_ACTION:"

/**
 * Create config file according to spec.
 * @param abspath
 * @param stream
 * 
 * @return int
 */
int lgs_create_config_file(log_stream_t *stream)
{
	int rc, n;
	char pathname[PATH_MAX + NAME_MAX];
	FILE *filp;

	TRACE_ENTER();

	/* check the existence of logsv_root_dir/pathName, create the path if it doesn't */
	snprintf(pathname, PATH_MAX, "%s/%s", lgs_cb->logsv_root_dir, stream->pathName);
	struct stat statbuf;
	if (lstat(pathname, &statbuf) != 0) {
		char command[PATH_MAX + NAME_MAX];
		sprintf(command, "mkdir -p %s", pathname);
		if (system(command) != 0) {
			LOG_NO("Create directory '%s/%s' failed", lgs_cb->logsv_root_dir, stream->pathName);
			rc = -1;
			goto done;
		}
	}
	/* create absolute path for config file */
	n = snprintf(&pathname[strlen(pathname)], PATH_MAX, "/%s.cfg", stream->fileName);

	/* Check if path got truncated */
	if (n == sizeof(pathname)) {
		LOG_ER("Path too long");
		rc = -1;
		goto done;
	}

fopen_retry:
	if ((filp = fopen(pathname, "w")) == NULL) {
		if (errno == EINTR)
			goto fopen_retry;

		LOG_NO("Could not open '%s' - %s", pathname, strerror(errno));
		rc = -1;
		goto done;
	}

	/* version */
	if ((rc = fprintf(filp, "%s %c.%d.%d\n", LOG_VER_EXP,
			  lgs_cb->log_version.releaseCode,
			  lgs_cb->log_version.majorVersion, lgs_cb->log_version.minorVersion)) == -1)
		goto fprintf_done;

	/* Format expression */
	if ((rc = fprintf(filp, "%s%s\n", FMAT_EXP, stream->logFileFormat)) == -1)
		goto fprintf_done;

	/* Max logfile size */
	if ((rc = fprintf(filp, "%s %llu\n", CFG_EXP_MAX_FILE_SIZE, stream->maxLogFileSize)) == -1)
		goto fprintf_done;

	/* Fixed log record size */
	if ((rc = fprintf(filp, "%s %d\n", CFG_EXP_FIXED_LOG_REC_SIZE, stream->fixedLogRecordSize)) == -1)
		goto fprintf_done;

	/* Log file full action */
	rc = fprintf(filp, "%s %s %d\n", CFG_EXP_LOG_FULL_ACTION, DEFAULT_ALM_ACTION, stream->maxFilesRotated);

 fprintf_done:
	if (rc == -1)
		LOG_NO("Could not write to '%s'", pathname);

fclose_retry:
	if ((rc = fclose(filp)) == -1) {
		if (errno == EINTR)
			goto fclose_retry;

		LOG_NO("Could not close '%s' - '%s'", pathname, strerror(errno));
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**
 * Get date and time string as mandated by SAF LOG file naming
 * 
 * @return char*
 */
char *lgs_get_time(void)
{
	struct tm *timeStampData;
	static char timeStampString[LGS_CREATE_CLOSE_TIME_LEN];
	char srcString[5];
	uint32_t stringSize;
	time_t testTime;

	time(&testTime);
	timeStampData = localtime(&testTime);

	stringSize = 5 * sizeof(char);
	snprintf(srcString, (size_t)stringSize, "%d", (timeStampData->tm_year + START_YEAR));

	strncpy(timeStampString, srcString, stringSize);

	stringSize = 3 * sizeof(char);
	snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_mon + 1));

	strncat(timeStampString, srcString, stringSize);

	snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_mday));

	strncat(timeStampString, srcString, stringSize);

	strncat(timeStampString, "_", (2 * sizeof(char)));

	snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_hour));
	strncat(timeStampString, srcString, stringSize);

	snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_min));
	strncat(timeStampString, srcString, stringSize);

	snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_sec));
	strncat(timeStampString, srcString, stringSize);

	timeStampString[15] = '\0';
	return timeStampString;
}

SaTimeT lgs_get_SaTime(void)
{
	return time(NULL) * SA_TIME_ONE_SECOND;
}

/**
 * Rename a file to include a timestamp in the name
 * @param path
 * @param old_name
 * @param time_stamp
 * @param suffix
 * 
 * @return int
 */
int lgs_file_rename(const char *path, const char *old_name, const char *time_stamp, const char *suffix)
{
	int ret;
	char oldpath[PATH_MAX + NAME_MAX];
	char newpath[PATH_MAX + NAME_MAX];

	sprintf(oldpath, "%s/%s/%s%s", lgs_cb->logsv_root_dir, path, old_name, suffix);
	sprintf(newpath, "%s/%s/%s_%s%s", lgs_cb->logsv_root_dir, path, old_name, time_stamp, suffix);
	TRACE_4("Rename file from %s", oldpath);
	TRACE_4("              to %s", newpath);
	if ((ret = rename(oldpath, newpath)) == -1)
		LOG_NO("rename: FAILED - %s", strerror(errno));

	return ret;
}

void lgs_exit(const char *msg, SaAmfRecommendedRecoveryT rec_rcvr)
{
	LOG_ER("Exiting with message: %s", msg);
	(void)saAmfComponentErrorReport(lgs_cb->amf_hdl, &lgs_cb->comp_name, 0, rec_rcvr, SA_NTF_IDENTIFIER_UNUSED);
	exit(EXIT_FAILURE);
}

/****************************************************************************
 *                      
 * lgs_lga_entry_valid  
 *                              
 *  Searches the cb->client_tree for an reg_id entry whos MDS_DEST equals
 *  that passed DEST and returns true if itz found.
 *                                      
 * This routine is typically used to find the validity of the lga down rec from standby 
 * LGA_DOWN_LIST as  LGA client has gone away.
 *                              
 ****************************************************************************/
bool lgs_lga_entry_valid(lgs_cb_t *cb, MDS_DEST mds_dest)
{                                       
	log_client_t *rp = NULL;
                                       
	rp = (log_client_t *)ncs_patricia_tree_getnext(&cb->client_tree, (uint8_t *)0);
                                
	while (rp != NULL) {    
		if (m_NCS_MDS_DEST_EQUAL(&rp->mds_dest, &mds_dest)) {
			return true;
		}

		rp = (log_client_t *)ncs_patricia_tree_getnext(&cb->client_tree, (uint8_t *)&rp->client_id_net);
	}       
        
	return false;
}

/**
 * Send a write log ack (callback request) to a client
 * 
 * @param client_id
 * @param invocation
 * @param error
 * @param mds_dest
 */
void lgs_send_write_log_ack(uint32_t client_id, SaInvocationT invocation, SaAisErrorT error, MDS_DEST mds_dest)
{
	uint32_t rc;
	NCSMDS_INFO mds_info = {0};
	lgsv_msg_t msg;

	TRACE_ENTER();

	msg.type = LGSV_LGS_CBK_MSG;
	msg.info.cbk_info.type = LGSV_WRITE_LOG_CALLBACK_IND;
	msg.info.cbk_info.lgs_client_id = client_id;
	msg.info.cbk_info.inv = invocation;
	msg.info.cbk_info.write_cbk.error = error;

	mds_info.i_mds_hdl = lgs_cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_LGS;
	mds_info.i_op = MDS_SEND;
	mds_info.info.svc_send.i_msg = &msg;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_LGA;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	mds_info.info.svc_send.info.snd.i_to_dest = mds_dest;

	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_NO("Send of WRITE ack to %"PRIu64"x FAILED: %u", mds_dest, rc);

	TRACE_LEAVE();
}

/**
 * Free all dynamically allocated memory for a WRITE
 * @param param
 */
void lgs_free_write_log(const lgsv_write_log_async_req_t *param)
{
	if (param->logRecord->logHdrType == SA_LOG_GENERIC_HEADER) {
		SaLogGenericLogHeaderT *genLogH = &param->logRecord->logHeader.genericHdr;
		free(param->logRecord->logBuffer->logBuf);
		free(param->logRecord->logBuffer);
		free(genLogH->notificationClassId);
		free((void *)genLogH->logSvcUsrName);
		free(param->logRecord);
	} else {
		SaLogNtfLogHeaderT *ntfLogH = &param->logRecord->logHeader.ntfHdr;
		free(param->logRecord->logBuffer->logBuf);
		free(param->logRecord->logBuffer);
		free(ntfLogH->notificationClassId);
		free(ntfLogH->notifyingObject);
		free(ntfLogH->notificationObject);
		free(param->logRecord);
	}
}
