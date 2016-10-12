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
 * Examples can be found in this file, e.g. function fileopen(...) below
 */

#include "lgs.h"
#include "lgs_config.h"
#include "lgs_file.h"
#include "lgs_filehdl.h"
#include "osaf_time.h"

#define DEFAULT_NUM_APP_LOG_STREAMS 64

static NCS_PATRICIA_TREE stream_dn_tree;

static log_stream_t **stream_array;
/* We have at least the 3 well known streams. */
static unsigned int stream_array_size = 3;
/* Current number of streams */
static unsigned int numb_of_streams;

static const uint32_t kInvalidId = static_cast<uint32_t> (-1);

static int lgs_stream_array_insert(log_stream_t *stream, uint32_t id);
static int lgs_stream_array_insert_new(log_stream_t *stream, uint32_t *id);
static int lgs_stream_array_remove(int id);
static int get_number_of_log_files_h(log_stream_t *logStream, char *oldest_file);

/**
 * Open/create a file for append in non blocking mode.
 *
 * @param filepath[in] Complete path including file name
 * @param errno_save[out], errno if error
 * @return File descriptor or -1 if error
 */
static int fileopen_h(const std::string &filepath, int *errno_save)
{
	lgsf_apipar_t apipar;
	lgsf_retcode_t api_rc;
	int fd;
	fopen_in_t data_in;
	const char *groupname = NULL;
	std::string filePath;

	TRACE_ENTER();

	osafassert(filepath.empty() != true);

	if (filepath.size() > PATH_MAX) {
		LOG_WA("Cannot open file, File path > PATH_MAX");
		fd = -1;
		goto done;
	}

	groupname = static_cast<const char *>(lgs_cfg_get(LGS_IMM_DATA_GROUPNAME));

	filePath = filepath;
	data_in.filepath = const_cast<char *>(filePath.c_str());
	strcpy(data_in.groupname, groupname);

	TRACE("%s - filepath \"%s\"",__FUNCTION__, filepath.c_str());

	/* Fill in API structure */
	apipar.req_code_in = LGSF_FILEOPEN;
	apipar.data_in_size = sizeof(fopen_in_t);
	apipar.data_in = &data_in;
	apipar.data_out_size = sizeof(int);
	apipar.data_out = errno_save;
	
	api_rc = log_file_api(&apipar);
	if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		fd = -1;
	} else {
		fd = apipar.hdl_ret_code_out;
	}

done:
	TRACE_LEAVE();
	return fd;
}

/**
 * Close with retry at EINTR
 * 
 * @param fd [in]
 * @param errno_save [out], errno if error
 * @return -1 on error
 */
static int fileclose_h(int fd, int *errno_save)
{
	lgsf_apipar_t apipar;
	lgsf_retcode_t api_rc;
	int rc = 0;
	
	TRACE_ENTER2("fd=%d", fd);
	
	/* Fill in API structure */
	apipar.req_code_in = LGSF_FILECLOSE;
	apipar.data_in_size = sizeof(int);
	apipar.data_in = &fd;
	apipar.data_out_size = 0;
	apipar.data_out = NULL;
	
	api_rc = log_file_api(&apipar);
	if (api_rc == LGSF_BUSY) {
		/* The fd is not invalidated. Save to close later */
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		lgs_fd_list_add(fd);
		rc = -1;
		*errno_save = EBUSY;
	} else if (api_rc == LGSF_TIMEOUT) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		rc = -1;
		*errno_save = ETIMEDOUT;
	} else if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		rc = -1;
		*errno_save = errno;
	} else {
		rc = apipar.hdl_ret_code_out;
		*errno_save = 0;
	}

	TRACE_LEAVE2("rc = %d",rc);
	return rc;
}

/**
 * Delete a file (unlink())
 * 
 * @param filepath A null terminated string containing the name of the file to
 *        be deleted
 * @return (-1) if error
 */
static int file_unlink_h(const std::string &filepath)
{
	int rc = 0;
	lgsf_apipar_t apipar;
	lgsf_retcode_t api_rc;
	size_t filepath_len;
	
	TRACE_ENTER();

	filepath_len = filepath.size() + 1;
	if (filepath_len > PATH_MAX) {
		LOG_WA("Cannot delete file, File path > PATH_MAX");
		rc = -1;
		goto done;
	}

	/* Fill in API structure */
	apipar.req_code_in = LGSF_DELETE_FILE;
	apipar.data_in_size = filepath_len;
	apipar.data_in = const_cast<char *>(filepath.c_str());
	apipar.data_out_size = 0;
	apipar.data_out = NULL;
	
	api_rc = log_file_api(&apipar);
	if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		rc = -1;
	} else {
		rc = apipar.hdl_ret_code_out;
	}

done:
	TRACE_LEAVE2("rc = %d", rc);
	return rc;
	
}

/**
 * Delete config file.
 * @param stream
 * 
 * @return -1 if error
 */
static int delete_config_file(log_stream_t *stream)
{
	int rc = 0;
	std::string pathname;

	TRACE_ENTER();

	/* create absolute path for config file */
	std::string logsv_root_dir = static_cast<const char *>(
		lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));

	pathname = logsv_root_dir + "/" + stream->pathName +
			"/" + stream->fileName + ".cfg";

	rc = file_unlink_h(pathname);

	TRACE_LEAVE2("rc = %d", rc);
	return rc;
}

/**
 * Remove oldest log file until there are 'maxFilesRotated' - 1 files left
 * 
 * @param stream
 * @return -1 on error
 */
static int rotate_if_needed(log_stream_t *stream)
{
	char oldest_file[PATH_MAX];
	int rc = 0;
	int file_cnt;

	TRACE_ENTER();

	/* Rotate out files from previous lifes */
	if ((file_cnt = get_number_of_log_files_h(stream, oldest_file)) == -1) {
		rc = -1;
		goto done;
	}
	
	/*
	 ** Remove until we have one less than allowed, we are just about to
	 ** create a new one again.
	 */
	while (file_cnt >= static_cast<int>(stream->maxFilesRotated)) {
		if ((rc = file_unlink_h(oldest_file)) == -1) {
			LOG_NO("Could not delete: %s - %s", oldest_file, strerror(errno));
			goto done;
		}

		if ((file_cnt = get_number_of_log_files_h(stream, oldest_file)) == -1) {
			rc = -1;
			goto done;
		}
	}

 done:
	TRACE_LEAVE2("rc = %d", rc);
	return rc;
}

static uint32_t log_stream_add(NCS_PATRICIA_NODE *node, const char *key)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	node->key_info = (uint8_t *)key;

	if ( NULL == ncs_patricia_tree_get(&stream_dn_tree,node->key_info)){
		rc = ncs_patricia_tree_add(&stream_dn_tree, node);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_WA("ncs_patricia_tree_add FAILED for '%s' %u", key, rc);
			node->key_info = NULL;
			goto done;
		}
	}

 done:
	return rc;
}

static uint32_t log_stream_remove(const char *key)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	log_stream_t *stream;

	stream = reinterpret_cast<log_stream_t *>(ncs_patricia_tree_get(&stream_dn_tree, (uint8_t *)key));
	if (stream == NULL) {
		TRACE_2("ncs_patricia_tree_get FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
	
	if (ncs_patricia_tree_get(&stream_dn_tree,stream->pat_node.key_info)){
		if ((rc = ncs_patricia_tree_del(&stream_dn_tree, &stream->pat_node)) != NCSCC_RC_SUCCESS) {
			LOG_WA("ncs_patricia_tree_del FAILED for  '%s' %u",key,rc);
			goto done;
		}
	}

 done:
	return rc;
}

/**
 * Initiate the files belonging to a stream
 * 
 * @param stream
 */
void log_initiate_stream_files(log_stream_t *stream)
{
	int errno_save;
	const std::string log_root_path = static_cast<const char *>(
		lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));

	TRACE_ENTER();
	
	/* Initiate standby stream file parameters. Only needed if we are standby
	 * and configured for split file system.
	 */
	if ((lgs_cb->ha_state == SA_AMF_HA_STANDBY) && lgs_is_split_file_system()) {
		stream->stb_curFileSize = 0;
		stream->stb_logFileCurrent = stream->logFileCurrent;
	}

	/* Initiate stream file descriptor to flag that no valid descriptor exist */
	*stream->p_fd = -1;
	
	/* Delete to get counting right. It might not exist. */
	(void)delete_config_file(stream);

	/* Remove files from a previous life if needed */
	if (rotate_if_needed(stream) == -1) {
		TRACE("%s - rotate_if_needed() FAIL",__FUNCTION__);
		goto done;
	}

	if (lgs_make_reldir_h(stream->pathName) != 0){
		TRACE("%s - lgs_make_dir_h() FAIL",__FUNCTION__);
		goto done;
	}

	if (lgs_create_config_file_h(log_root_path, stream) != 0) {
		TRACE("%s - lgs_create_config_file_h() FAIL",__FUNCTION__);
		goto done;
	}

	if ((*stream->p_fd = log_file_open(log_root_path, stream,
			stream->logFileCurrent,
			&errno_save)) == -1) {
		TRACE("%s - Could not open '%s' - %s", __FUNCTION__,
				stream->logFileCurrent.c_str(), strerror(errno_save));
		goto done;
	}
	
done:
	TRACE_LEAVE();
}

log_stream_t *log_stream_get_by_name(const char *name)
{
	char nname[SA_MAX_NAME_LENGTH + 1];

	/* Create null-terminated stream name */
	strcpy(nname, name);
	memset(&nname[strlen(name)], 0, SA_MAX_NAME_LENGTH + 1 - strlen(name));

	return reinterpret_cast<log_stream_t *>(ncs_patricia_tree_get(&stream_dn_tree, (uint8_t *)nname));
}

log_stream_t *log_stream_getnext_by_name(const char *name)
{
	char nname[SA_MAX_NAME_LENGTH + 1];

	if (name != NULL) {
		/* Create SA_MAX_NAME_LENGTH stream name */
		strcpy(nname, name);
		memset(&nname[strlen(name)], 0, SA_MAX_NAME_LENGTH + 1 - strlen(name));
		return reinterpret_cast<log_stream_t *>(ncs_patricia_tree_getnext(&stream_dn_tree, (uint8_t *)nname));
	} else
		return reinterpret_cast<log_stream_t *>(ncs_patricia_tree_getnext(&stream_dn_tree, NULL));
}

void log_stream_print(log_stream_t *stream)
{
	osafassert(stream != NULL);

	TRACE_2("******** Stream %s ********", stream->name);
	TRACE_2("  fileName:             %s", stream->fileName.c_str());
	TRACE_2("  pathName:             %s", stream->pathName.c_str());
	TRACE_2("  maxLogFileSize:       %llu", stream->maxLogFileSize);
	TRACE_2("  fixedLogRecordSize:   %u", stream->fixedLogRecordSize);
	if (stream->streamType == STREAM_TYPE_APPLICATION)
		TRACE_2("  haProperty:           %u", stream->haProperty);
	TRACE_2("  logFullAction:        %u", stream->logFullAction);
	TRACE_2("  logFullHaltThreshold: %u", stream->logFullHaltThreshold);
	TRACE_2("  maxFilesRotated:      %u", stream->maxFilesRotated);
	TRACE_2("  logFileFormat:        %s", stream->logFileFormat);
	TRACE_2("  severityFilter:       %x", stream->severityFilter);
	TRACE_2("  creationTimeStamp:    %llu", stream->creationTimeStamp);
	TRACE_2("  numOpeners:           %u", stream->numOpeners);
	TRACE_2("  streamId:             %u", stream->streamId);
	TRACE_2("  fd:                   %d", *stream->p_fd);
	TRACE_2("  logFileCurrent:       %s", stream->logFileCurrent.c_str());
	TRACE_2("  curFileSize:          %u", stream->curFileSize);
	TRACE_2("  logRecordId:          %u", stream->logRecordId);
	TRACE_2("  streamType:           %u", stream->streamType);
	TRACE_2("  filtered:             %llu", stream->filtered);
}

/**
 * Free stream resources
 *
 * @param stream[in]
 */
void log_free_stream_resources(log_stream_t *stream)
{
	if (stream->streamId != 0)
		lgs_stream_array_remove(stream->streamId);

	if (stream->pat_node.key_info != NULL)
		log_stream_remove(stream->name);

	if (stream->logFileFormat != NULL)
		free(stream->logFileFormat);

	delete stream;
	stream = NULL;
}

/**
 * Remove a log stream including:
 * - Runtime object if application stream and active
 * - Remove stream resources (allocated memory)
 *
 * @param s[in] Pointer to the array of streams
 */
void log_stream_delete(log_stream_t **s)
{
	log_stream_t *stream;

	osafassert(s != NULL && *s != NULL);
	stream = *s;

	TRACE_ENTER2("%s", stream->name);

	if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE) {
		if (stream->streamType == STREAM_TYPE_APPLICATION) {
			SaAisErrorT rv;
			TRACE("Stream is closed, I am HA active so remove IMM object");
			SaNameT objectName;
			strcpy((char *)objectName.value, stream->name);
			objectName.length = strlen((char *)objectName.value);
			rv = saImmOiRtObjectDelete(lgs_cb->immOiHandle, &objectName);
			if (rv != SA_AIS_OK) {
				/* no retry; avoid blocking LOG service #1886 */
				LOG_WA("saImmOiRtObjectDelete returned %u for %s",
						rv, stream->name);
			}
		}
	}

	if (stream->streamId != 0)
		lgs_stream_array_remove(stream->streamId);

	if (stream->pat_node.key_info != NULL)
		log_stream_remove(stream->name);

	if (stream->logFileFormat != NULL)
		free(stream->logFileFormat);

	delete stream;
	*s = NULL;
	TRACE_LEAVE();
}

/**
 * Point to local or shared fd dependant on shared or split file system
 * Initiate the fd to -1
 * 
 * @param stream
 */
static void init_log_stream_fd(log_stream_t *stream)
{
	if (lgs_is_split_file_system()) {
		stream->p_fd = &stream->fd_local;
	} else {
		stream->p_fd = &stream->fd_shared;
	}
	
	*stream->p_fd = -1;	
}

/**
 * Create a new stream object. If HA state active, create the
 * correspronding IMM runtime object.
 * 
 * Note: log_stream_new() is replaced by this function.
 *       The new function is doing the same as the old but the possibility
 *       to create a stream without creating a corresponding runtime object
 *       is added. See creationFlag parameter
 *
 * Stream attributes[in]:
 * @param name
 * @param filename
 * @param pathname
 * @param maxLogFileSize
 * @param fixedLogRecordSize
 * @param logFullAction
 * @param maxFilesRotated
 * @param logFileFormat
 * @param streamType
 * @param stream_id
 * @param twelveHourModeFlag
 * @param logRecordId
 * 
 * @return log_stream_t*
 */
log_stream_t *log_stream_new_1(
	SaNameT *dn,
	const std::string &filename,
	const std::string &pathname,
	SaUint64T maxLogFileSize,
	SaUint32T fixedLogRecordSize,
	SaLogFileFullActionT logFullAction,
	SaUint32T maxFilesRotated,
	const char *logFileFormat,
	logStreamTypeT streamType,
	int stream_id,
	SaBoolT twelveHourModeFlag,
	uint32_t logRecordId,
	int creationFlag
	)
{
	int rc;
	log_stream_t *stream = NULL;

	osafassert(dn != NULL);
	TRACE_ENTER2("%s, l: %u", dn->value, dn->length);

	if (lgs_relative_path_check_ts(pathname)) {
		goto done;
	}

	stream = new (std::nothrow) log_stream_t();
	if (stream == NULL) {
		LOG_WA("log_stream_new calloc FAILED");
		goto done;
	}
	memcpy(stream->name, dn->value, dn->length);
	stream->name[SA_MAX_NAME_LENGTH] = '\0';
	stream->fileName = filename;
	stream->pathName = pathname;
	stream->maxLogFileSize = maxLogFileSize;
	stream->fixedLogRecordSize = fixedLogRecordSize;
	stream->haProperty = SA_TRUE;
	stream->logFullAction = logFullAction;
	stream->streamId = stream_id;
	stream->logFileFormat = strdup(logFileFormat);
	if (stream->logFileFormat == NULL) {
		log_stream_delete(&stream);
		goto done;
	}
	stream->maxFilesRotated = maxFilesRotated;
	stream->creationTimeStamp = lgs_get_SaTime();
	stream->severityFilter = 0x7f;	/* by default all levels are allowed */
	stream->streamType = streamType;
	stream->twelveHourModeFlag = twelveHourModeFlag;
	stream->logRecordId = logRecordId;
	stream->stb_logRecordId = 0;
	
	/* Initiate local or shared stream file descriptor dependant on shared or
	 * split file system
	 */
	init_log_stream_fd(stream);

	/* Add stream to tree */
	if (log_stream_add(&stream->pat_node, stream->name) != NCSCC_RC_SUCCESS) {
		log_stream_delete(&stream);
		goto done;
	}

	/* Add stream to array */
	if (stream->streamId == kInvalidId)
		rc = lgs_stream_array_insert_new(stream, &stream->streamId);
	else
		rc = lgs_stream_array_insert(stream, stream->streamId);

	if (rc < 0) {
		LOG_WA("Add stream to array FAILED");
		log_stream_delete(&stream);
		goto done;
	}

	/* Create IMM runtime object for stream (if ACTIVE) */
	if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE) {
		char *dndup = strdup(stream->name);
		char *parent_name = strchr(stream->name, ',');
		char *rdnstr;
		SaNameT parent, *parentName = NULL;

		if (parent_name != NULL) {
			rdnstr = strtok(dndup, ",");
			parent_name++;	/* FIX, vulnerable for malformed DNs */
			parentName = &parent;
			strcpy((char *)parent.value, parent_name);
			parent.length = strlen((char *)parent.value);
		} else
			rdnstr = stream->name;

		void *arr1[] = { &rdnstr };
		const SaImmAttrValuesT_2 attr_safLgStr = {
			.attrName = const_cast<SaImmAttrNameT>("safLgStr"),
			.attrValueType = SA_IMM_ATTR_SASTRINGT,
			.attrValuesNumber = 1,
			.attrValues = arr1
		};
		char *str2 = const_cast<char *>(stream->fileName.c_str());
		void *arr2[] = { &str2 };
		const SaImmAttrValuesT_2 attr_safLogStreamFileName = {
			.attrName = const_cast<SaImmAttrNameT>("saLogStreamFileName"),
			.attrValueType = SA_IMM_ATTR_SASTRINGT,
			.attrValuesNumber = 1,
			.attrValues = arr2
		};
		char *str3 = const_cast<char *>(stream->pathName.c_str());
		void *arr3[] = { &str3 };
		const SaImmAttrValuesT_2 attr_safLogStreamPathName = {
			.attrName = const_cast<SaImmAttrNameT>("saLogStreamPathName"),
			.attrValueType = SA_IMM_ATTR_SASTRINGT,
			.attrValuesNumber = 1,
			.attrValues = arr3
		};
		void *arr4[] = { &stream->maxLogFileSize };
		const SaImmAttrValuesT_2 attr_saLogStreamMaxLogFileSize = {
			.attrName = const_cast<SaImmAttrNameT>("saLogStreamMaxLogFileSize"),
			.attrValueType = SA_IMM_ATTR_SAUINT64T,
			.attrValuesNumber = 1,
			.attrValues = arr4
		};
		void *arr5[] = { &stream->fixedLogRecordSize };
		const SaImmAttrValuesT_2 attr_saLogStreamFixedLogRecordSize = {
			.attrName = const_cast<SaImmAttrNameT>("saLogStreamFixedLogRecordSize"),
			.attrValueType = SA_IMM_ATTR_SAUINT32T,
			.attrValuesNumber = 1,
			.attrValues = arr5
		};
		void *arr6[] = { &stream->haProperty };
		const SaImmAttrValuesT_2 attr_saLogStreamHaProperty = {
			.attrName = const_cast<SaImmAttrNameT>("saLogStreamHaProperty"),
			.attrValueType = SA_IMM_ATTR_SAUINT32T,
			.attrValuesNumber = 1,
			.attrValues = arr6
		};
		void *arr7[] = { &stream->logFullAction };
		const SaImmAttrValuesT_2 attr_saLogStreamLogFullAction = {
			.attrName = const_cast<SaImmAttrNameT>("saLogStreamLogFullAction"),
			.attrValueType = SA_IMM_ATTR_SAUINT32T,
			.attrValuesNumber = 1,
			.attrValues = arr7
		};
		void *arr8[] = { &stream->maxFilesRotated };
		const SaImmAttrValuesT_2 attr_saLogStreamMaxFilesRotated = {
			.attrName = const_cast<SaImmAttrNameT>("saLogStreamMaxFilesRotated"),
			.attrValueType = SA_IMM_ATTR_SAUINT32T,
			.attrValuesNumber = 1,
			.attrValues = arr8
		};
		char *str9 = stream->logFileFormat;
		void *arr9[] = { &str9 };
		const SaImmAttrValuesT_2 attr_saLogStreamLogFileFormat = {
			.attrName = const_cast<SaImmAttrNameT>("saLogStreamLogFileFormat"),
			.attrValueType = SA_IMM_ATTR_SASTRINGT,
			.attrValuesNumber = 1,
			.attrValues = arr9
		};
		void *arr10[] = { &stream->severityFilter };
		const SaImmAttrValuesT_2 attr_saLogStreamSeverityFilter = {
			.attrName = const_cast<SaImmAttrNameT>("saLogStreamSeverityFilter"),
			.attrValueType = SA_IMM_ATTR_SAUINT32T,
			.attrValuesNumber = 1,
			.attrValues = arr10
		};
		void *arr11[] = { &stream->creationTimeStamp };
		const SaImmAttrValuesT_2 attr_saLogStreamCreationTimestamp = {
			.attrName = const_cast<SaImmAttrNameT>("saLogStreamCreationTimestamp"),
			.attrValueType = SA_IMM_ATTR_SATIMET,
			.attrValuesNumber = 1,
			.attrValues = arr11
		};
		const SaImmAttrValuesT_2 *attrValues[] = {
			&attr_safLgStr,
			&attr_safLogStreamFileName,
			&attr_safLogStreamPathName,
			&attr_saLogStreamMaxLogFileSize,
			&attr_saLogStreamFixedLogRecordSize,
			&attr_saLogStreamHaProperty,
			&attr_saLogStreamLogFullAction,
			&attr_saLogStreamMaxFilesRotated,
			&attr_saLogStreamLogFileFormat,
			&attr_saLogStreamSeverityFilter,
			&attr_saLogStreamCreationTimestamp,
			NULL
		};

		/* parentName needs to be configurable? */
		{
			SaAisErrorT rv;

			rv = saImmOiRtObjectCreate_2(lgs_cb->immOiHandle,
						     const_cast<SaImmClassNameT>("SaLogStream"),
						     parentName, attrValues);
			free(dndup);

			if (rv != SA_AIS_OK) {
				/* no retry; avoid blocking LOG service #1886 */
				LOG_WA("saImmOiRtObjectCreate_2 returned %u for %s, parent %s",
				      rv, stream->name, parent_name);
			}
		}
	}

 done:
	TRACE_LEAVE();
	return stream;
}

/**
 * Create a new default stream. Do not create an IMM runtime object.
 * @param name
 * @param stream_id
 *
 * @return log_stream_t*
 */
log_stream_t *log_stream_new_2(SaNameT *name, int stream_id)
{
	int rc;
	log_stream_t *stream = NULL;

	osafassert(name != NULL);
	TRACE_ENTER2("%s, l: %u", name->value, (unsigned int)name->length);

	stream = new (std::nothrow) log_stream_t();
	if (stream == NULL) {
		LOG_WA("calloc FAILED");
		goto done;
	}
	memcpy(stream->name, name->value, name->length);
	stream->name[SA_MAX_NAME_LENGTH] = '\0';
	stream->streamId = stream_id;
	stream->creationTimeStamp = lgs_get_SaTime();
	stream->severityFilter = 0x7f;	/* by default all levels are allowed */
	
	/* Initiate local or shared stream file descriptor dependant on shared or
	 * split file system
	 */
	init_log_stream_fd(stream);	

	/* Add stream to tree */
	if (log_stream_add(&stream->pat_node, stream->name) != NCSCC_RC_SUCCESS) {
		log_stream_delete(&stream);
		goto done;
	}

	/* Add stream to array */
	if (stream->streamId == kInvalidId)
		rc = lgs_stream_array_insert_new(stream, &stream->streamId);
	else
		rc = lgs_stream_array_insert(stream, stream->streamId);

	if (rc < 0) {
		log_stream_delete(&stream);
		goto done;
	}

 done:
	TRACE_LEAVE();
	return stream;
}

/**
 * Open log file associated with stream
 * The file is opened in non blocking mode.
 * Note!
 *     File name has to be provided separately since it is not always
 *     logFileCurrent!
 * 
 * @param root_path[in]
 * @param stream[in]
 * @param filename[in]
 * @param errno_save - errno from open() returned here if supplied
 *
 * @return int - the file descriptor or -1 on errors
 */
int log_file_open(
	const std::string &root_path,
	log_stream_t *stream,
	const std::string &filename,
	int *errno_save)
{
	int fd;
	std::string pathname;
	int errno_ret;

	TRACE_ENTER();

	pathname = root_path + "/" + stream->pathName + "/" + filename + ".log";

	TRACE("%s - Opening file \"%s\"", __FUNCTION__, pathname.c_str());

	fd = fileopen_h(pathname, &errno_ret);
	if (errno_save != 0) {
		*errno_save = errno_ret;
	}

	TRACE_LEAVE();
	return fd;
}

/**
 * If the stream is new (number of openers are 0) the stream files are created.
 * This includes:
 *  - Remove old rotated files if there are any from an earlier closed stream
 *    with the same name.
 *  - Create relative log directory
 *  - Create configuration (.cfg) file
 *  - Create current log file
 * 
 * Note: No error if files couldn't be handled but stream file descriptor for
 *       current log file will be -1 indicating that files has to be opened
 *       when trying to write to stream.
 * 
 * @param stream
 */
void log_stream_open_fileinit(log_stream_t *stream)
{
	TRACE_ENTER2("%s, numOpeners=%u", stream->name, stream->numOpeners);
	osafassert(stream != NULL);

	/* first time open? */
	if (stream->numOpeners == 0) {
		/* Create and save current log file name */
		stream->logFileCurrent = stream->fileName + "_" + lgs_get_time(NULL);
		log_initiate_stream_files(stream);
	}
	
	/* Opening a stream will always succeed. If file system problem a new
	 * attempt to open the files is done when trying to write to the stream
	 */
	stream->numOpeners++;

	TRACE_LEAVE2("numOpeners=%u", stream->numOpeners);
}

/**
 * if ref. count allows, close the associated file, rename it and delete the
 * stream object.
 * Renaming means that a a time string based on second since Epoch is added to
 * the name of the closed log file.
 * 
 * @param stream[in]
 * @param close_time[in/out] Time in sec since Epoch (osaf_clock_gettime()). If the value of
 *                           this pointer is 0 time is fetched using osaf_clock_gettime() and
 *                           the value is updated (out) with this time.
 *                           If the value contains a time (other than 0) this
 *                           time is used
 */
void log_stream_close(log_stream_t **s, time_t *close_time_ptr)
{
	int rc = 0;
	int errno_ret;
	log_stream_t *stream = *s;
	std::string file_to_rename;
	char *timeString = NULL;
	std::string emptyStr = "";
	uint32_t msecs_waited = 0;
	const unsigned int max_waiting_time = 8 * 1000;	/* 8 secs */
	const unsigned int sleep_delay_ms = 500;
	SaUint32T trace_num_openers;
	struct timespec closetime_tspec;
	const std::string root_path = static_cast<const char *>(
		lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));

	osafassert(stream != NULL);
	TRACE_ENTER2("%s", stream->name);
	
	osafassert(stream->numOpeners > 0);
	stream->numOpeners--;
	trace_num_openers = stream->numOpeners;

	if (stream->numOpeners == 0) {
		/* Delete stream if last opener
		 * Note: standard streams can never be deleted
		 */
		osafassert(stream->streamType == STREAM_TYPE_APPLICATION);
		
		if (*stream->p_fd != -1) {
			/* Note: Stream fd is always initiated to -1
			 * If shared file system and we are standby no files are opened and
			 * fd is still -1. This means we get here only if there is an open
			 * log file on the node
			 */
			
			/* Close the log file */
			rc = fileclose_h(*stream->p_fd, &errno_ret);
			*stream->p_fd = -1;
			if (rc == -1) {
				LOG_NO("Could not close log files: %s", strerror(errno_ret));
				goto done_files;
			}

			/* Handle time for renaming */
			if (*close_time_ptr == 0) {
				osaf_clock_gettime(CLOCK_REALTIME, &closetime_tspec);
				*close_time_ptr = closetime_tspec.tv_sec;
			}
			timeString = lgs_get_time(close_time_ptr);				
	
			/* Set file to rename */
			if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE) {
				/* If active */
				file_to_rename = stream->logFileCurrent;
			} else {
				file_to_rename = stream->stb_logFileCurrent;
			}
			
			/* Rename stream log file */
			rc = lgs_file_rename_h(root_path, stream->pathName,
					file_to_rename, timeString,
					LGS_LOG_FILE_EXT, emptyStr);
			while ((rc == -1) && (msecs_waited < max_waiting_time)) {
				usleep(sleep_delay_ms * 1000);
				msecs_waited += sleep_delay_ms;
				rc = lgs_file_rename_h(root_path, stream->pathName,
					file_to_rename,	timeString,
					LGS_LOG_FILE_EXT, emptyStr);
			}

			if (rc == -1) {
				LOG_ER("Could not rename log file: %s", strerror(errno));
				goto done_files;
			}

			/* Rename stream config file */
			rc = lgs_file_rename_h(root_path, stream->pathName,
					stream->fileName, timeString,
					LGS_LOG_FILE_CONFIG_EXT, emptyStr);
			while ((rc == -1) && (msecs_waited < max_waiting_time)) {
				usleep(sleep_delay_ms * 1000);
				msecs_waited += sleep_delay_ms;
				rc = lgs_file_rename_h(root_path, stream->pathName,
						stream->fileName, timeString,
						LGS_LOG_FILE_CONFIG_EXT, emptyStr);
			}
			if (rc == -1) {
				LOG_ER("Could not rename config file: %s", strerror(errno));
				goto done_files;
			}
		}

		done_files:
			log_stream_delete(s);
			TRACE("\tApplication stream deleted");
			stream = NULL;
	}

	TRACE_LEAVE2("rc=%d, numOpeners=%u", rc, trace_num_openers);
}

/**
 * Close the stream associated file.
 * @param stream
 * 
 * @return int
 */
int log_stream_file_close(log_stream_t *stream)
{
	int rc = 0;
	int errno_ret;

	osafassert(stream != NULL);
	TRACE_ENTER2("%s", stream->name);

	osafassert(stream->numOpeners > 0);

	if (*stream->p_fd != -1) {
		if ((rc = fileclose_h(*stream->p_fd, &errno_ret)) == -1) {
			LOG_NO("log_stream_file_close FAILED: %s", strerror(errno_ret));
		}
		*stream->p_fd = -1;
	}

	TRACE_LEAVE2("%d", rc);
	return rc;
}

/**
 * Return number of log files in a dir and the name of the oldest file.
 * @param logStream[in]
 * @param oldest_file[out]
 * 
 * @return -1 if error else number of log files
 */
static int get_number_of_log_files_h(log_stream_t *logStream, char *oldest_file)
{
	lgsf_apipar_t apipar;
	lgsf_retcode_t api_rc;
	gnolfh_in_t parameters_in;
	size_t path_len;
	int rc = 0;

	TRACE_ENTER();

	std::string logsv_root_dir = static_cast<const char *>(
		lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));

	parameters_in.file_name = const_cast<char *>(logStream->fileName.c_str());
	parameters_in.logsv_root_dir = const_cast<char *>(logsv_root_dir.c_str());
	parameters_in.pathName = const_cast<char *>(logStream->pathName.c_str());

	path_len = strlen(parameters_in.file_name) +
		   strlen(parameters_in.logsv_root_dir) +
		   strlen(parameters_in.pathName);
	if (path_len > PATH_MAX) {
		LOG_WA("Path to log files > PATH_MAX");
		rc = -1;
		goto done;
	}

	/* Fill in API structure */
	apipar.req_code_in = LGSF_GET_NUM_LOGFILES;
	apipar.data_in_size = sizeof(gnolfh_in_t);
	apipar.data_in = &parameters_in;
	apipar.data_out_size = PATH_MAX;
	apipar.data_out = oldest_file;
	
	api_rc = log_file_api(&apipar);
	if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		rc = -1;
	} else {
		rc = apipar.hdl_ret_code_out;
	}

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Handle log file rotation on standby node.
 * This handler shall be used on standby only
 * 
 * @param stream
 * @param count 
 * @return 
 */
static int log_rotation_stb(log_stream_t *stream, size_t count)
{
	int rc = 0;
	int errno_save;
	int errno_ret;
	char *current_time_str;
	std::string new_current_log_filename;
	bool do_rotate = false;
	std::string root_path = static_cast<const char *>(lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));

	TRACE_ENTER();

	stream->stb_curFileSize += count;
	
	/* If active node has rotated files there is a new "logFileCurrent"
	 * (check pointed). Standby always follows active rotation.
	 * 
	 * In some cases standby may reach file size limit before active. If this
	 * happen standby must rotate. A new rotate is done when active has rotated
	 * as described above.
	 */
	
	/* XXX Should be handled...??
	 * If local rotate has been done within one second before active is rotating
	 * the file name in stb_logFileCurrent and logFileCurrent may be the same!
	 * This means that a rotation on active when standby is running is not
	 * guaranteed. This situation though is very unlikely.
	 */
	if (stream->logFileCurrent != stream->stb_prev_actlogFileCurrent) {
		TRACE("Active has rotated (follow active)");
		/* Active has rotated */
		stream->stb_prev_actlogFileCurrent = stream->logFileCurrent;
		/* Use close time from active for renaming of closed file
		 */
		
		current_time_str = lgs_get_time(&stream->act_last_close_timestamp);
		do_rotate = true;
		/* Use same name as active for new current log file */
		new_current_log_filename = stream->logFileCurrent;
	} else if (stream->stb_curFileSize > stream->maxLogFileSize) {
		TRACE("Standby has rotated (filesize)");
		/* Standby current log file has reached max limit */
		/* Use internal time since active has not yet rotated */
		current_time_str = lgs_get_time(NULL);
		do_rotate = true;
		/* Create new name for current log file based on local time */
		new_current_log_filename = stream->fileName + "_" + current_time_str;
	}
	
	if (do_rotate) {
		/* Time to use as "close time stamp" and "open time stamp" for new
		 * log file as created by active.
		 */
		
		/* Close current log file */
		rc = fileclose_h(*stream->p_fd, &errno_ret);
		*stream->p_fd = -1;
		if (rc == -1) {
			LOG_NO("close FAILED: %s", strerror(errno_ret));
			goto done;
		}

		std::string emptyStr = "";
		/* Rename file to give it the "close timestamp" */
		rc = lgs_file_rename_h(root_path,
				       stream->pathName,
				       stream->stb_logFileCurrent,
				       current_time_str,
				       LGS_LOG_FILE_EXT,
				       emptyStr);
		if (rc == -1)
			goto done;

		/* Remove oldest file if needed */
		if (rotate_if_needed(stream) == -1) {
			TRACE("Old file removal failed");
		}
		
		/* Save new name for current log file and open it */
		stream->stb_logFileCurrent = new_current_log_filename;
		if ((*stream->p_fd = log_file_open(root_path, stream,
						   stream->stb_logFileCurrent,
						   &errno_save)) == -1) {
			LOG_IN("Could not open '%s' - %s", stream->stb_logFileCurrent.c_str(),
			       strerror(errno_save));
			rc = -1;
			goto done;
		}
		
		stream->stb_curFileSize = 0;
	}
		
done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Handle log file rotation. Do rotation if needed
 * 
 * @param stream
 * @param count 
 * @return -1 on error
 */
static int log_rotation_act(log_stream_t *stream, size_t count)
{
	int rc;
	int errno_save;
	int errno_ret;
	struct timespec closetime_tspec;
	std::string root_path = static_cast<const char *>(
		lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));
	std::string emptyStr = "";

	/* If file size > max file size:
	 *  - Close the log file and create a new.
	 *  - If number of files > max number of files delete the oldest
	 */
	rc = 0;
	stream->curFileSize += count;

	if ((stream->curFileSize) > stream->maxLogFileSize) {
		osaf_clock_gettime(CLOCK_REALTIME, &closetime_tspec);
		time_t closetime = closetime_tspec.tv_sec;
		char *current_time = lgs_get_time(&closetime);

		/* Close current log file */
		rc = fileclose_h(*stream->p_fd, &errno_ret);
		*stream->p_fd = -1;
		if (rc == -1) {
			LOG_NO("close FAILED: %s", strerror(errno_ret));
			goto done;
		}

		/* Rename file to give it the "close timestamp" */
		rc = lgs_file_rename_h(root_path,
				       stream->pathName,
				       stream->logFileCurrent,
				       current_time,
				       LGS_LOG_FILE_EXT,
				       emptyStr);
		if (rc == -1)
			goto done;

		/* Save time when logFileCurrent was closed */
		stream->act_last_close_timestamp = closetime;

		/* Invalidate logFileCurrent for this stream */
		stream->logFileCurrent[0] = 0;
		
		/* Reset file size for current log file */
		stream->curFileSize = 0;
		
		/* Remove oldest file if needed */
		if (rotate_if_needed(stream) == -1) {
			TRACE("Old file removal failed");
		}

		/* Create a new file name that includes "open time stamp" and open the file */
		stream->logFileCurrent = stream->fileName + "_" + current_time;
		if ((*stream->p_fd = log_file_open(root_path,
						   stream,
						   stream->logFileCurrent,
						   &errno_save)) == -1) {
			LOG_IN("Could not open '%s' - %s", stream->logFileCurrent.c_str(),
			       strerror(errno_save));
			rc = -1;
			goto done;
		}
	}
	
done:	
	return rc;
}

/**
 * log_stream_write will write a number of bytes to the associated file. If
 * the file size gets too big, the file is closed, renamed and a new file is
 * opened. If there are too many files, the oldest file will be deleted.
 * 
 * @param stream
 * @param buf
 * @param count
 * 
 * @return int 0 No error
 *            -1 on error
 *            -2 Write failed because of write timeout or EWOULDBLOCK/EAGAIN
 */
int log_stream_write_h(log_stream_t *stream, const char *buf, size_t count)
{
	int rc = 0;
	int errno_ret;
	lgsf_apipar_t apipar;
	wlrh_t params_in;
	size_t params_in_size;
	lgsf_retcode_t api_rc;
	int write_errno=0;

	osafassert(stream != NULL && buf != NULL);
	TRACE_ENTER2("%s", stream->name);

	/* Open files on demand e.g. on new active after fail/switch-over. This
	 * enables LOG to cope with temporary file system problems. */
	if (*stream->p_fd == -1) {
		/* Create directory and log files if they were not created at
		 * stream open or reopen files if bad file descriptor.
		 */
		log_initiate_stream_files(stream);
		
		if (*stream->p_fd == -1) {
			TRACE("%s - Initiating stream files \"%s\" Failed", __FUNCTION__,
					stream->name);
			// Seems file system is busy - can not create requrested files.
			// Let inform the log client TRY_AGAIN.
			//
			// Return (-1) to inform that it is caller's responsibility
			// to free the allocated memmories.
			return -1;
		} else {
			TRACE("%s - stream files initiated", __FUNCTION__);
		}
	}
	
	TRACE("%s - *stream->p_fd = %d",__FUNCTION__,*stream->p_fd);

	/* Get size of write log record header */
	params_in_size = sizeof(wlrh_t);

	params_in.fd = *stream->p_fd;
	params_in.fixedLogRecordSize = stream->fixedLogRecordSize;
	params_in.record_size = count;

	/*
	  Not necessary to allocated memory for log record here.
	  Instead, point to log buffer allocated by the caller.

	  By this way, LOGsv will be improved performance as
	  it did not copy a large data (max could be 32 Kb) using memcpy.
	*/
	params_in.lgs_rec = buf;

	/* Fill in API structure */
	apipar.req_code_in = LGSF_WRITELOGREC;
	apipar.data_in_size = params_in_size;
	apipar.data_in = &params_in;
	apipar.data_out_size = sizeof(int);
	apipar.data_out = &write_errno;
	
	api_rc = log_file_api(&apipar);
	if (api_rc == LGSF_TIMEOUT) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		rc = -2;
	} else if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		rc = -1;
	} else {
		rc = apipar.hdl_ret_code_out;
	}

	/* End write the log record */	
	
	if ((rc == -1) || (rc == -2)) {
		/* If writing failed always invalidate the stream file descriptor.
		 */
		/* Careful with log level here to avoid syslog flooding */
		if (rc == -2) {
			LOG_IN("write '%s' failed \"Timeout\"", stream->logFileCurrent.c_str());
		} else {
			LOG_IN("write '%s' failed \"%s\"", stream->logFileCurrent.c_str(),
					strerror(write_errno));
		}
	
		if (*stream->p_fd != -1) {
			/* Close the file and invalidate the stream fd */
			if (fileclose_h(*stream->p_fd, &errno_ret) == -1) {
				LOG_NO("fileclose failed %s", strerror(errno_ret));
			}
			*stream->p_fd = -1;
		}
		
		if ((write_errno == EAGAIN) || (write_errno == EWOULDBLOCK)) {
			/* Change return code to timeout if EAGAIN (would block) */
			TRACE("Write would block");
			rc = -2;
		}
		goto done;
	}
 
	/* Handle file size and rotate if needed.
	 * Note: This is handled in different ways depending on if writing a log
	 *       record on standby or active node. Standby can write if configured
	 *       for split file system.
	 */
	if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE) {
		rc = log_rotation_act(stream, count);
	} else {
		rc = log_rotation_stb(stream, count);
	}

 done:
	TRACE_LEAVE2("rc=%d", rc);
	return rc;
}

/**
 * Get stream from array
 * @param lgs_cb
 * @param id
 * 
 * @return log_stream_t*
 */
log_stream_t *log_stream_get_by_id(uint32_t id)
{
	log_stream_t *stream = NULL;

	if (0 <= id && id < stream_array_size)
		stream = stream_array[id];

	return stream;
}

/**
 * Insert stream into array at specified position.
 * @param stream
 * @param id
 * 
 * @return int 0 if OK, <0 if failed
 */
static int lgs_stream_array_insert(log_stream_t *stream, uint32_t id)
{
	int rc = 0;

	if (numb_of_streams >= stream_array_size) {
		rc = -1;
		goto exit;
	}

	if (id >= stream_array_size) {
		rc = -2;
		goto exit;
	}

	if (stream_array[stream->streamId] != NULL) {
		rc = -3;
		goto exit;
	}

	stream_array[id] = stream;
	numb_of_streams++;

 exit:
	return rc;
}

/**
 * Add new stream to array. Update the stream ID.
 * @param lgs_cb
 * @param stream
 * 
 * @return int
 */
static int lgs_stream_array_insert_new(log_stream_t *stream, uint32_t *id)
{
	int rc = -1;
	uint32_t i = 0;

	osafassert(id != NULL);

	if (numb_of_streams >= stream_array_size)
		goto exit;

	for (i = 0; i < stream_array_size; i++) {
		if (stream_array[i] == NULL) {
			stream_array[i] = stream;
			*id = i;
			numb_of_streams++;
			rc = 0;
			break;
		}
	}

 exit:
	return rc;
}

/**
 * Remove stream from array
 * @param lgs_cb
 * @param n
 * 
 * @return int
 */
static int lgs_stream_array_remove(int id)
{
	int rc = -1;

	if (0 <= id && id < static_cast<int>(stream_array_size)) {
		osafassert(stream_array[id] != NULL);
		stream_array[id] = NULL;
		rc = 0;
		osafassert(numb_of_streams > 0);
		numb_of_streams--;
	}

	return rc;
}

/**
 * Dump the stream array
 */
void log_stream_id_print()
{
	uint32_t i = 0;

	TRACE("  Current number of streams: %u", numb_of_streams);
	for (i = 0; i < stream_array_size; i++) {
		if (stream_array[i] != NULL)
			TRACE("    Id %u - %s", i, stream_array[i]->name);
	}
}

uint32_t log_stream_init()
{
	NCS_PATRICIA_PARAMS param;
	SaUint32T value;

	/* Get configuration of how many application streams we should allow. */
	value = *static_cast<const SaUint32T *>(lgs_cfg_get(LGS_IMM_LOG_MAX_APPLICATION_STREAMS));
	stream_array_size += value;

	TRACE("Max %u application log streams", stream_array_size - 3);
	stream_array = static_cast<log_stream_t **>(calloc(1, sizeof(log_stream_t *) * stream_array_size));
	if (stream_array == NULL) {
		LOG_WA("calloc FAILED");
		return NCSCC_RC_FAILURE;
	}

	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));

	param.key_size = SA_MAX_NAME_LENGTH;

	return ncs_patricia_tree_init(&stream_dn_tree, &param);
}

/**
 * Close log file, change name of log file, optionally create new log and
 * config file. Basically the same logic as described in 3.1.6.4
 * in A.02.01.
 * 
 * @param create_files_f[in]
 *     create_files_f = true; New files are created
 *     create_files_f = false; New files are not created
 * @param root_path[in]
 * @param stream[in]
 * @param current_logfile_name[in]
 * @param cur_time_in[in]
 * 
 * @return -1 on error
 */
int log_stream_config_change(bool create_files_f,
			const std::string &root_path,
			log_stream_t *stream,
			const std::string &current_logfile_name,
			time_t *cur_time_in)
{
	int rc;
	int errno_ret;
	char *current_time = lgs_get_time(cur_time_in);
	std::string emptyStr = "";

	TRACE_ENTER2("%s", stream->name);

	/* Peer sync needed due to change in logFileCurrent */
	
	if (*stream->p_fd == -1) {
		/* lgs has not yet recieved any stream operation request after this swtchover/failover.
		 *  stream shall be opened on-request after a switchover, failover
		 */	
		TRACE("log file of the stream: %s does not exist",stream->name);
	} else {
		/* close the existing log file, and only when there is a valid fd */

		if ((rc = fileclose_h(*stream->p_fd, &errno_ret)) == -1) {
			LOG_NO("log_stream log file close  FAILED: %s", strerror(errno_ret));
			goto done;
		}
		*stream->p_fd = -1;

		rc = lgs_file_rename_h(root_path, stream->pathName, current_logfile_name,
				current_time, LGS_LOG_FILE_EXT, emptyStr);
		if (rc == -1) {
			goto done;
		}

		rc = lgs_file_rename_h(root_path, stream->pathName, stream->fileName,
				current_time, LGS_LOG_FILE_CONFIG_EXT, emptyStr);
		if (rc == -1) {
			goto done;
		}
	}

	/* Creating the new config file */
	if (create_files_f == LGS_STREAM_CREATE_FILES) {
		if ((rc = lgs_create_config_file_h(root_path, stream)) != 0)
			goto done;

		stream->logFileCurrent = stream->fileName + "_" +  current_time;

		/* Create the new log file based on updated configuration */
		*stream->p_fd = log_file_open(root_path,
					      stream,
					      stream->logFileCurrent,
					      NULL);
	}

	/* Fix bug - this function makes return (-1) when create_files_f = false */
	if (create_files_f == !LGS_STREAM_CREATE_FILES) {
		rc = 0;
	} else if (*stream->p_fd == -1) {
		rc = -1;
	} else {
		rc = 0;
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/*
 * Check if number of stream out of LOG_MAX_APPLICATION_STREAMS
 *
 * @return
 */
bool check_max_stream()
{
	TRACE("  Current number of streams: %u", numb_of_streams);
	return (numb_of_streams < stream_array_size ? false:true);
}

