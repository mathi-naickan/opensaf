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

	/* create absolute path for config file */
	n = snprintf(pathname, PATH_MAX, "%s/%s/%s.cfg", lgs_cb->logsv_root_dir, stream->pathName, stream->fileName);

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
	uns32 stringSize;
	uns32 characters = 0;
	time_t testTime;

	time(&testTime);
	timeStampData = localtime(&testTime);

	stringSize = 5 * sizeof(char);
	characters = snprintf(srcString, (size_t)stringSize, "%d", (timeStampData->tm_year + START_YEAR));

	strncpy(timeStampString, srcString, stringSize);

	stringSize = 3 * sizeof(char);
	characters = snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_mon + 1));

	strncat(timeStampString, srcString, stringSize);

	characters = snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_mday));

	strncat(timeStampString, srcString, stringSize);

	strncat(timeStampString, "_", (2 * sizeof(char)));

	characters = snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_hour));
	strncat(timeStampString, srcString, stringSize);

	characters = snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_min));
	strncat(timeStampString, srcString, stringSize);

	characters = snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_sec));
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
 *  that passed DEST and returns TRUE if itz found.
 *                                      
 * This routine is typically used to find the validity of the lga down rec from standby 
 * LGA_DOWN_LIST as  LGA client has gone away.
 *                              
 ****************************************************************************/
NCS_BOOL lgs_lga_entry_valid(lgs_cb_t *cb, MDS_DEST mds_dest)
{                                       
	log_client_t *rp = NULL;
                                       
	rp = (log_client_t *)ncs_patricia_tree_getnext(&cb->client_tree, (uns8 *)0);
                                
	while (rp != NULL) {    
		if (m_NCS_MDS_DEST_EQUAL(&rp->mds_dest, &mds_dest)) {
			return TRUE;
		}

		rp = (log_client_t *)ncs_patricia_tree_getnext(&cb->client_tree, (uns8 *)&rp->client_id_net);
	}       
        
	return FALSE;
}  
