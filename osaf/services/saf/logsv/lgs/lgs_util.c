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

/*****************************************************************************
 * Important information
 * ---------------------
 * To prevent log service thread from "hanging" if communication with NFS is
 * not working or is very slow all functions using file I/O must run their file
 * handling in a separate thread.
 * For more information on how to do that see the following files:
 * lgs_file.h, lgs_file.c, lgs_filendl.c and lgs_filehdl.h
 * Examples can be found in file lgs_stream.c, e.g. function fileopen(...)
 */

#include "lgs_util.h"
#include <stdlib.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "immutil.h"
#include "lgs.h"
#include "lgs_util.h"
#include "lgs_fmt.h"
#include "lgs_file.h"
#include "lgs_filehdl.h"

#define ALARM_STREAM_ENV_PREFIX "ALARM"
#define NOTIFICATION_STREAM_ENV_PREFIX "NOTIFICATION"
#define SYSTEM_STREAM_ENV_PREFIX "SYSTEM"
#define LGS_CREATE_CLOSE_TIME_LEN 16
#define START_YEAR 1900

/**
 * Create config file according to spec.
 * @param abspath
 * @param stream
 * 
 * @return -1 on error
 */
int lgs_create_config_file_h(log_stream_t *stream)
{
	lgsf_apipar_t apipar;
	lgsf_retcode_t api_rc;
	void *params_in;
	ccfh_t *header_in_p;
	size_t params_in_size;
	char *logFileFormat_p;
	char *pathname_p;
	
	int rc;
	int path_len, n;
	char pathname[PATH_MAX];

	TRACE_ENTER();

	/* check the existence of logsv_root_dir/pathName,
	 * check that the path is safe.
	 * If ok, create the path if it doesn't already exits
	 */
	path_len = snprintf(pathname, PATH_MAX, "%s/%s", lgs_cb->logsv_root_dir, stream->pathName);
	if (path_len >= PATH_MAX) {
		LOG_WA("logsv_root_dir + pathName > PATH_MAX");
		rc = -1;
		goto done;
	}
	
	if (lgs_relative_path_check_ts(pathname) == true) {
		LOG_WA("Directory path \"%s\" not allowed", pathname);
		rc = -1;
		goto done;
	}
	
	if (lgs_make_reldir_h(stream->pathName) != 0) {
		LOG_WA("Create directory '%s/%s' failed", lgs_cb->logsv_root_dir, stream->pathName);
		rc = -1;
		goto done;
	}

	/* create absolute path for config file */
	n = snprintf(pathname, PATH_MAX, "%s/%s/%s.cfg",
			lgs_cb->logsv_root_dir, stream->pathName, stream->fileName);
	
	if (n >= PATH_MAX) {
		LOG_WA("Complete pathname > PATH_MAX");
		rc = -1;
		goto done;
	}
	TRACE("%s - Config file path \"%s\"",__FUNCTION__, pathname);

	/* 
	 * Create the configuration file.
	 * Open the file, write it's content and close the file
	 */
	
	/* Allocate memory for parameters */
	params_in_size = sizeof(ccfh_t) + (strlen(stream->logFileFormat) + 1) +
			(strlen(pathname) + 1);
	params_in = malloc(params_in_size);
	
	/* Set pointers to allocated memory */
	header_in_p = params_in;
	logFileFormat_p = params_in + sizeof(ccfh_t);
	size_t logFileFormat_size = params_in_size - sizeof(ccfh_t);
	pathname_p = logFileFormat_p + strlen(stream->logFileFormat) + 1;
	size_t pathname_size = logFileFormat_size - (strlen(stream->logFileFormat) + 1);

	/* Fill in in parameters */
	header_in_p->version.releaseCode = lgs_cb->log_version.releaseCode;
	header_in_p->version.majorVersion = lgs_cb->log_version.majorVersion;
	header_in_p->version.minorVersion = lgs_cb->log_version.minorVersion;
	header_in_p->logFileFormat_size = strlen(stream->logFileFormat)+1;
	header_in_p->maxLogFileSize = stream->maxLogFileSize;
	header_in_p->fixedLogRecordSize = stream->fixedLogRecordSize;
	header_in_p->maxFilesRotated = stream->maxFilesRotated;
	
	n = snprintf(logFileFormat_p, logFileFormat_size, "%s", stream->logFileFormat);
	if (n >= logFileFormat_size) {
		rc = -1;
		LOG_WA("Log file format string too long");
		goto done_free;
	}
	n = snprintf(pathname_p, pathname_size, "%s", pathname);
	if (n >= pathname_size) {
		rc = -1;
		LOG_WA("Path name string too long");
		goto done_free;
	}
	
	/* Fill in API structure */
	apipar.req_code_in = LGSF_CREATECFGFILE;
	apipar.data_in_size = params_in_size;
	apipar.data_in = (void*) params_in;
	apipar.data_out_size = 0;
	apipar.data_out = NULL;
	
	api_rc = log_file_api(&apipar);
	if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		rc = -1;
	} else {
		rc = apipar.hdl_ret_code_out;
	}
	
done_free:
	free(params_in);
	
done:
	TRACE_LEAVE2("rc = %d", rc);
	return rc;
}

/**
 * Get date and time string as mandated by SAF LOG file naming
 * Format: YYYYMMDD_HHMMSS
 * 
 * @param time_in
 *        Time to format.
 *        If NULL time is fetched using time()
 * @return char*
 */
char *lgs_get_time(time_t *time_in)
{
	struct tm *timeStampData;
	static char timeStampString[LGS_CREATE_CLOSE_TIME_LEN];
	char srcString[5];
	uint32_t stringSize;
	time_t testTime;

	if (time_in == NULL) {
		time(&testTime);
	} else {
		testTime = *time_in;
	}
	struct tm tm_info;
	timeStampData = localtime_r(&testTime, &tm_info);
	osafassert(timeStampData);

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
 * @param path[in]
 * @param old_name[in]
 * @param time_stamp[in] 
 *        Can be set to NULL but then new_name must be the complete new name
 *        including time stamps but without suffix
 * @param suffix[in]
 * @param new_name[in/out] 
 *        Pointer to char string of NAME_MAX size
 *        Filename of renamed file. Can be set to NULL
 * 
 * @return -1 if error
 */
int lgs_file_rename_h(
		const char *path,
		const char *old_name,
		const char *time_stamp,
		const char *suffix,
		char *new_name)
{
	int rc;
	char oldpath[PATH_MAX];
	char newpath[PATH_MAX];
	char new_name_loc[NAME_MAX];
	size_t n;
	lgsf_apipar_t apipar;
	void *params_in_p;
	size_t params_in_size;
	lgsf_retcode_t api_rc;
	char *oldpath_in_p;
	char *newpath_in_p;
	size_t *oldpath_str_size_p;
	
	TRACE_ENTER();

	n = snprintf(oldpath, PATH_MAX, "%s/%s/%s%s",
			lgs_cb->logsv_root_dir, path, old_name, suffix);
	if (n >= PATH_MAX) {
		LOG_ER("Cannot rename file, old path > PATH_MAX");
		rc = -1;
		goto done;
	}

	if (time_stamp != NULL) {
		snprintf(new_name_loc, NAME_MAX, "%s_%s%s", old_name, time_stamp, suffix);
	} else {
		snprintf(new_name_loc, NAME_MAX, "%s%s", new_name, suffix);
	}
	n = snprintf(newpath, PATH_MAX, "%s/%s/%s",
			lgs_cb->logsv_root_dir, path, new_name_loc);
	if (n >= PATH_MAX) {
		LOG_ER("Cannot rename file, new path > PATH_MAX");
		rc = -1;
		goto done;
	}
	
	if (new_name != NULL) {
		strcpy(new_name, new_name_loc);
	}
	
	TRACE_4("Rename file from %s", oldpath);
	TRACE_4("              to %s", newpath);
	
	/* Allocate memory for parameters */
	size_t oldpath_size = strlen(oldpath)+1;
	size_t newpath_size = strlen(newpath)+1;
	params_in_size = sizeof(size_t) + oldpath_size + newpath_size;
	
	params_in_p = malloc(params_in_size);
	
	/* Fill in pointer addresses */
	oldpath_str_size_p = params_in_p;
	oldpath_in_p = params_in_p + sizeof(size_t);
	newpath_in_p = oldpath_in_p + oldpath_size;
	
	/* Fill in parameters */
	*oldpath_str_size_p = oldpath_size;
	memcpy(oldpath_in_p, oldpath, oldpath_size);
	memcpy(newpath_in_p, newpath, newpath_size);
	
	/* Fill in API structure */
	apipar.req_code_in = LGSF_RENAME_FILE;
	apipar.data_in_size = params_in_size;
	apipar.data_in = params_in_p;
	apipar.data_out_size = 0;
	apipar.data_out = NULL;
	
	api_rc = log_file_api(&apipar);
	if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		rc = -1;
	} else {
		rc = apipar.hdl_ret_code_out;
	}
	
	free(params_in_p);
	
done:
	TRACE_LEAVE();
	return rc;
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

/**
 * Check if a relative ("/../") path occurs in the path.
 * Note: This function must be thread safe
 * 
 * @param path
 * @return true if path is allowed
 */
bool lgs_relative_path_check_ts(const char* path)
{
	bool rc = false;
	int len_path = strlen(path);
	if (len_path >= 3) {
		if (strstr(path, "/../") != NULL || strstr(path, "/..") != NULL) {
			/* /..dir/ is not allowed, however /dir../ is allowed. */
			rc = true;
		} else if (path[0] == '.' && path[1] == '.' && path[2] == '/') {
			rc = true;
		}
	}
	return rc;
}

/**
 * Create directory structure, if not already created.
 * The structure is created in the log service root directory pointed to in
 * lgs_cb->logsv_root_dir.
 * 
 * TBD: Fix with separate ticket. 
 * Note: If the lgsv root directory does not exist a root directory is
 * created based on the default path in PKGLOGDIR. The path in the configuration
 * object is not updated. The reason is to make it possible for the log service
 * to work even if there is no existing root path in the file system.
 * 
 * @param path, Path relative log service root directory
 * @return -1 on error
 */
int lgs_make_reldir_h(const char* path)
{
	lgsf_apipar_t apipar;
	lgsf_retcode_t api_rc;
	int rc = -1;
	mld_in_t params_in;
	char new_rootstr[PATH_MAX];
	size_t n1, n2;
	
	new_rootstr[0] = '\0'; /* Initiate to empty string */
	
	TRACE_ENTER();
	
	TRACE("lgs_cb->logsv_root_dir \"%s\"",lgs_cb->logsv_root_dir);
	TRACE("path \"%s\"",path);
	
	n1 = snprintf(params_in.root_dir, PATH_MAX, "%s", lgs_cb->logsv_root_dir);
	if (n1 >= PATH_MAX) {
		LOG_WA("logsv_root_dir > PATH_MAX");
		goto done;
	}
	n2 = snprintf(params_in.rel_path, PATH_MAX, "%s", path);
	if (n2 >= PATH_MAX) {
		LOG_WA("path > PATH_MAX");
		goto done;
	}
	
	/* Check that the complete path is not longer than PATH_MAX */
	if ((strlen(params_in.root_dir) + strlen(params_in.rel_path)) >= PATH_MAX) {
		LOG_WA("root_dir + rel_path > PATH_MAX");
		goto done;
	}
	
	/* Fill in API structure */
	apipar.req_code_in = LGSF_MAKELOGDIR;
	apipar.data_in_size = sizeof(mld_in_t);
	apipar.data_in = (void*) &params_in;
	apipar.data_out_size = PATH_MAX;
	apipar.data_out = &new_rootstr;
	
	api_rc = log_file_api(&apipar);
	if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		rc = -1;
	} else {
		rc = apipar.hdl_ret_code_out;
	}
	
	/* Handle a possible change of root dir to default */
	if (new_rootstr[0] != '\0') {
		lgs_imm_rootpathconf_set(new_rootstr);
		TRACE("%s - new_rootstr \"%s\"",__FUNCTION__,new_rootstr);
	}

done:
	TRACE_LEAVE2("rc = %d",rc);
	
	return rc;
}

/**
 * Check if a path exists. See also stat(..)
 * 
 * @param path_to_check
 * @return (-1) if not exists, 0 if exists
 */
int lgs_check_path_exists_h(const char *path_to_check)
{
	lgsf_apipar_t apipar;
	lgsf_retcode_t api_rc;
	void *params_in_p;
	int rc = 0;
	
	TRACE_ENTER2("path \"%s\"",path_to_check);
	/* Allocate memory for parameter */
	size_t params_in_size = strlen(path_to_check)+1;
	params_in_p = malloc(params_in_size);
	
	/* Fill in path */
	memcpy(params_in_p, path_to_check, params_in_size);
	
	/* Fill in API structure */
	apipar.req_code_in = LGSF_CHECKPATH;
	apipar.data_in_size = params_in_size;
	apipar.data_in = params_in_p;
	apipar.data_out_size = 0;
	apipar.data_out = NULL;
	
	api_rc = log_file_api(&apipar);
	if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		rc = -1;
	} else {
		rc = apipar.hdl_ret_code_out;
	}
	
	free(params_in_p);
	TRACE_LEAVE2("rc = %d",rc);
	return rc;
}
