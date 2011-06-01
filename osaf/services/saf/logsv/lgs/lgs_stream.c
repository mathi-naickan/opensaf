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

#include "lgs.h"

#define DEFAULT_NUM_APP_LOG_STREAMS 64
#define LGS_LOG_FILE_EXT ".log"
#define LGS_LOG_FILE_CONFIG_EXT ".cfg"

static NCS_PATRICIA_TREE stream_dn_tree;

static log_stream_t **stream_array;
/* We have at least the 3 well known streams. */
static unsigned int stream_array_size = 3;
/* Current number of streams */
static unsigned int numb_of_streams;

static int lgs_stream_array_insert(log_stream_t *stream, uint32_t id);
static int lgs_stream_array_insert_new(log_stream_t *stream, uint32_t *id);
static int lgs_stream_array_remove(int id);
static int get_number_of_log_files(log_stream_t *logStream, char *oldest_file);

/**
 * Close with retry at EINTR
 * @param fd
 *
 * @return int
 */
static int fileclose(int fd)
{
	int rc;

	TRACE_ENTER2("fd=%d", fd);

close_retry:
	rc = close(fd);

	if (rc == -1 && errno == EINTR)
		goto close_retry;

	TRACE_LEAVE2("rc=%d", rc);
	return rc;
}

/**
 * Delete config file.
 * @param stream
 * 
 * @return int
 */
static int delete_config_file(log_stream_t *stream)
{
	int rc, n;
	char pathname[PATH_MAX + NAME_MAX];

	TRACE_ENTER();

	/* create absolute path for config file */
	n = snprintf(pathname, PATH_MAX, "%s/%s/%s.cfg", lgs_cb->logsv_root_dir, stream->pathName, stream->fileName);

	assert(n < sizeof(pathname));

	if ((rc = unlink(pathname)) == -1) {
		if (errno == ENOENT)
			rc = 0;
		else
			LOG_NO("could not unlink: %s - %s", pathname, strerror(errno));
	}

	TRACE_LEAVE2("rc=%d", rc);
	return rc;
}

static int rotate_if_needed(log_stream_t *stream)
{
	char oldest_file[PATH_MAX + NAME_MAX];
	int rc = 0;
	int file_cnt;

	TRACE_ENTER();

	/* Rotate out files from previous lifes */
	if ((file_cnt = get_number_of_log_files(stream, oldest_file)) == -1) {
		rc = -1;
		goto done;
	}

	/*
	 ** Remove until we have one less than allowed, we are just about to
	 ** create a new one again.
	 */
	while (file_cnt >= stream->maxFilesRotated) {
		TRACE_1("remove oldest file: %s", oldest_file);
		if ((rc = unlink(oldest_file)) == -1) {
			LOG_NO("could not unlink: %s - %s", oldest_file, strerror(errno));
			goto done;
		}

		if ((file_cnt = get_number_of_log_files(stream, oldest_file)) == -1) {
			rc = -1;
			goto done;
		}
	}

 done:
	TRACE_LEAVE2("rc=%d", rc);
	return rc;
}

static uns32 log_stream_add(NCS_PATRICIA_NODE *node, const char *key)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	node->key_info = (uns8 *)key;

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

static uns32 log_stream_remove(const char *key)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	log_stream_t *stream;

	stream = (log_stream_t *)ncs_patricia_tree_get(&stream_dn_tree, (uns8 *)key);
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

log_stream_t *log_stream_get_by_name(const char *name)
{
	char nname[SA_MAX_NAME_LENGTH + 1];

	/* Create null-terminated stream name */
	strcpy(nname, name);
	memset(&nname[strlen(name)], 0, SA_MAX_NAME_LENGTH + 1 - strlen(name));

	return (log_stream_t *)ncs_patricia_tree_get(&stream_dn_tree, (uns8 *)nname);
}

log_stream_t *log_stream_getnext_by_name(const char *name)
{
	char nname[SA_MAX_NAME_LENGTH + 1];

	if (name != NULL) {
		/* Create SA_MAX_NAME_LENGTH stream name */
		strcpy(nname, name);
		memset(&nname[strlen(name)], 0, SA_MAX_NAME_LENGTH + 1 - strlen(name));
		return (log_stream_t *)ncs_patricia_tree_getnext(&stream_dn_tree, (uns8 *)nname);
	} else
		return (log_stream_t *)ncs_patricia_tree_getnext(&stream_dn_tree, NULL);
}

void log_stream_print(log_stream_t *stream)
{
	assert(stream != NULL);

	TRACE_2("******** Stream %s ********", stream->name);
	TRACE_2("  fileName:             %s", stream->fileName);
	TRACE_2("  pathName:             %s", stream->pathName);
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
	TRACE_2("  fd:                   %d", stream->fd);
	TRACE_2("  logFileCurrent:       %s", stream->logFileCurrent);
	TRACE_2("  curFileSize:          %u", stream->curFileSize);
	TRACE_2("  logRecordId:          %u", stream->logRecordId);
	TRACE_2("  streamType:           %u", stream->streamType);
	TRACE_2("  filtered:             %llu", stream->filtered);
}

void log_stream_delete(log_stream_t **s)
{
	log_stream_t *stream;

	assert(s != NULL && *s != NULL);
	stream = *s;

	TRACE_ENTER2("%s", stream->name);

	if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE) 
		if(stream->streamType == STREAM_TYPE_APPLICATION) {
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
	if (stream->streamId != 0)
		lgs_stream_array_remove(stream->streamId);

	if (stream->pat_node.key_info != NULL)
		log_stream_remove(stream->name);

	if (stream->logFileFormat != NULL)
		free(stream->logFileFormat);

	free(stream);
	*s = NULL;
	TRACE_LEAVE();
}

/**
 * Create a new stream object. If HA state active, create the
 * correspronding IMM runtime object.
 * 
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
log_stream_t *log_stream_new(SaNameT *dn,
			     const char *filename,
			     const char *pathname,
			     SaUint64T maxLogFileSize,
			     SaUint32T fixedLogRecordSize,
			     SaLogFileFullActionT logFullAction,
			     SaUint32T maxFilesRotated,
			     const char *logFileFormat,
			     logStreamTypeT streamType, int stream_id, SaBoolT twelveHourModeFlag, uint32_t logRecordId)
{
	int rc;
	log_stream_t *stream = NULL;

	assert(dn != NULL);
	TRACE_ENTER2("%s, l: %u", dn->value, dn->length);

	stream = calloc(1, sizeof(log_stream_t));
	if (stream == NULL) {
		LOG_WA("log_stream_new calloc FAILED");
		goto done;
	}
	memcpy(stream->name, dn->value, dn->length);
	stream->name[SA_MAX_NAME_LENGTH] = '\0';
	strncpy(stream->fileName, filename, sizeof(stream->fileName));
	strncpy(stream->pathName, pathname, sizeof(stream->pathName));
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
	stream->fd = -1;
	stream->severityFilter = 0x7f;	/* by default all levels are allowed */
	stream->streamType = streamType;
	stream->twelveHourModeFlag = twelveHourModeFlag;
	stream->logRecordId = logRecordId;

	/* Add stream to tree */
	if (log_stream_add(&stream->pat_node, stream->name) != NCSCC_RC_SUCCESS) {
		log_stream_delete(&stream);
		goto done;
	}

	/* Add stream to array */
	if (stream->streamId == -1)
		rc = lgs_stream_array_insert_new(stream, &stream->streamId);
	else
		rc = lgs_stream_array_insert(stream, stream->streamId);

	if (rc < 0) {
		LOG_WA("Add stream to array FAILED");
		log_stream_delete(&stream);
		goto done;
	}

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
			.attrName = "safLgStr",
			.attrValueType = SA_IMM_ATTR_SASTRINGT,
			.attrValuesNumber = 1,
			.attrValues = arr1
		};
		char *str2 = stream->fileName;
		void *arr2[] = { &str2 };
		const SaImmAttrValuesT_2 attr_safLogStreamFileName = {
			.attrName = "saLogStreamFileName",
			.attrValueType = SA_IMM_ATTR_SASTRINGT,
			.attrValuesNumber = 1,
			.attrValues = arr2
		};
		char *str3 = stream->pathName;
		void *arr3[] = { &str3 };
		const SaImmAttrValuesT_2 attr_safLogStreamPathName = {
			.attrName = "saLogStreamPathName",
			.attrValueType = SA_IMM_ATTR_SASTRINGT,
			.attrValuesNumber = 1,
			.attrValues = arr3
		};
		void *arr4[] = { &stream->maxLogFileSize };
		const SaImmAttrValuesT_2 attr_saLogStreamMaxLogFileSize = {
			.attrName = "saLogStreamMaxLogFileSize",
			.attrValueType = SA_IMM_ATTR_SAUINT64T,
			.attrValuesNumber = 1,
			.attrValues = arr4
		};
		void *arr5[] = { &stream->fixedLogRecordSize };
		const SaImmAttrValuesT_2 attr_saLogStreamFixedLogRecordSize = {
			.attrName = "saLogStreamFixedLogRecordSize",
			.attrValueType = SA_IMM_ATTR_SAUINT32T,
			.attrValuesNumber = 1,
			.attrValues = arr5
		};
		void *arr6[] = { &stream->haProperty };
		const SaImmAttrValuesT_2 attr_saLogStreamHaProperty = {
			.attrName = "saLogStreamHaProperty",
			.attrValueType = SA_IMM_ATTR_SAUINT32T,
			.attrValuesNumber = 1,
			.attrValues = arr6
		};
		void *arr7[] = { &stream->logFullAction };
		const SaImmAttrValuesT_2 attr_saLogStreamLogFullAction = {
			.attrName = "saLogStreamLogFullAction",
			.attrValueType = SA_IMM_ATTR_SAUINT32T,
			.attrValuesNumber = 1,
			.attrValues = arr7
		};
		void *arr8[] = { &stream->maxFilesRotated };
		const SaImmAttrValuesT_2 attr_saLogStreamMaxFilesRotated = {
			.attrName = "saLogStreamMaxFilesRotated",
			.attrValueType = SA_IMM_ATTR_SAUINT32T,
			.attrValuesNumber = 1,
			.attrValues = arr8
		};
		char *str9 = stream->logFileFormat;
		void *arr9[] = { &str9 };
		const SaImmAttrValuesT_2 attr_saLogStreamLogFileFormat = {
			.attrName = "saLogStreamLogFileFormat",
			.attrValueType = SA_IMM_ATTR_SASTRINGT,
			.attrValuesNumber = 1,
			.attrValues = arr9
		};
		void *arr10[] = { &stream->severityFilter };
		const SaImmAttrValuesT_2 attr_saLogStreamSeverityFilter = {
			.attrName = "saLogStreamSeverityFilter",
			.attrValueType = SA_IMM_ATTR_SAUINT32T,
			.attrValuesNumber = 1,
			.attrValues = arr10
		};
		void *arr11[] = { &stream->creationTimeStamp };
		const SaImmAttrValuesT_2 attr_saLogStreamCreationTimestamp = {
			.attrName = "saLogStreamCreationTimestamp",
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
							     "SaLogStream", parentName, attrValues);
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
 * Create a new stream object. Do not create an IMM object.
 * @param name
 * @param stream_id
 * 
 * @return log_stream_t*
 */
log_stream_t *log_stream_new_2(SaNameT *name, int stream_id)
{
	int rc;
	log_stream_t *stream = NULL;

	assert(name != NULL);
	TRACE_ENTER2("%s, l: %u", name->value, (unsigned int)name->length);

	stream = calloc(1, sizeof(log_stream_t));
	if (stream == NULL) {
		LOG_WA("calloc FAILED");
		goto done;
	}
	memcpy(stream->name, name->value, name->length);
	stream->name[SA_MAX_NAME_LENGTH] = '\0';
	stream->streamId = stream_id;
	stream->creationTimeStamp = lgs_get_SaTime();
	stream->fd = -1;
	stream->severityFilter = 0x7f;	/* by default all levels are allowed */

	/* Add stream to tree */
	if (log_stream_add(&stream->pat_node, stream->name) != NCSCC_RC_SUCCESS) {
		log_stream_delete(&stream);
		goto done;
	}

	/* Add stream to array */
	if (stream->streamId == -1)
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
 * Open file associated with stream
 * @param stream
 * @param errno_save - errno from open() returned here if supplied
 *
 * @return int - the file descriptor or -1 on errors
 */
static int log_file_open(log_stream_t *stream, int *errno_save)
{
	int fd;
	char pathname[PATH_MAX + NAME_MAX + 1];

	TRACE_LEAVE2("%s", stream->logFileCurrent);

	sprintf(pathname, "%s/%s/%s.log", lgs_cb->logsv_root_dir, stream->pathName, stream->logFileCurrent);

open_retry:
	fd = open(pathname, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP);

	if (fd == -1) {
		if (errno == EINTR)
			goto open_retry;
		/* save errno for caller logging */
		if (errno_save)
			*errno_save = errno;
		/* Do not log with higher severity here to avoid flooding the log.
		 * Can be called in context of log_stream_write */
		LOG_IN("%s: Could not open: %s - %s", __FUNCTION__, pathname, strerror(errno));
	}

	TRACE_LEAVE2("%d", fd);
	return fd;
}

SaAisErrorT log_stream_open(log_stream_t *stream)
{
	SaAisErrorT rc = SA_AIS_OK;
	int errno_save;
	
	assert(stream != NULL);
	TRACE_ENTER2("%s, numOpeners=%u", stream->name, stream->numOpeners);

	/* first time open? */
	if (stream->numOpeners == 0) {
		
		char command[PATH_MAX + 16];
		
		/* Delete to get counting right. It might not exist. */
		(void)delete_config_file(stream);

		/* Remove files from a previous life if needed */
		if (rotate_if_needed(stream) == -1) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;
		}
		
		sprintf(command, "mkdir -p %s/%s", lgs_cb->logsv_root_dir, stream->pathName);  
		if (system(command) != 0){
			LOG_NO("Create directory '%s/%s' failed", lgs_cb->logsv_root_dir, stream->pathName);
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;   
		}

		if (lgs_create_config_file(stream) != 0) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;
		}
		
		sprintf(stream->logFileCurrent, "%s_%s", stream->fileName, lgs_get_time());
		if ((stream->fd = log_file_open(stream, &errno_save)) == -1) {
			LOG_NO("Could not open '%s' - %s", stream->logFileCurrent, strerror(errno_save));
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;
		}
		stream->numOpeners++;
	} else {
		/* Second or more open on a stream */

		/* All file system problems as treated as temporary. It is OK to open
		an existing stream therefore the use count must be increased unconditionally. */
		stream->numOpeners++;

		if (stream->fd != -1) {
			/* already opened, we're done */
			goto done;
		}

		/* be nice to those without a shared file system, see #1138 */
		if (lgs_create_config_file(stream) != 0) {
			LOG_ER("%s: Creating config file failed", __FUNCTION__);
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;
		}

		if ((stream->fd = log_file_open(stream, &errno_save)) == -1) {
			LOG_NO("Could not open '%s' - %s", stream->logFileCurrent, strerror(errno_save));
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;
		}
	}
 done:
	TRACE_LEAVE2("rc=%u, numOpeners=%u", rc, stream->numOpeners);
	return rc;
}

/**
 * if ref. count allows, close the associated file, rename it and delete the
 * stream object.
 * @param stream
 * 
 * @return int
 */
int log_stream_close(log_stream_t **s)
{
	int rc = 0;
	log_stream_t *stream = *s;

	assert(stream != NULL);
	TRACE_ENTER2("%s, numOpeners=%u", stream->name, stream->numOpeners);

	assert(stream->numOpeners > 0);
	stream->numOpeners--;

	if (stream->numOpeners == 0) {
		/* standard streams can never be deleted */
		assert(stream->streamType == STREAM_TYPE_APPLICATION);
		if (stream->fd != -1) {
			char *timeString = lgs_get_time();
			if ((rc = fileclose(stream->fd)) == -1) {
				LOG_ER("log_stream_close FAILED: %s", strerror(errno));
				goto done;
			}

			rc = lgs_file_rename(stream->pathName, stream->logFileCurrent, timeString, LGS_LOG_FILE_EXT);
			if (rc == -1)
				goto done;

			rc = lgs_file_rename(stream->pathName, stream->fileName, timeString, LGS_LOG_FILE_CONFIG_EXT);
			if (rc == -1)
				goto done;
		}


		log_stream_delete(s);
	}

 done:
	TRACE_LEAVE2("rc=%d, numOpeners=%u", rc, stream->numOpeners);
	return rc;
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

	assert(stream != NULL);
	TRACE_ENTER2("%s", stream->name);

	assert(stream->numOpeners > 0);

	if (stream->fd != -1) {
		if ((rc = fileclose(stream->fd)) == -1) {
			LOG_ER("log_stream_file_close FAILED: %s", strerror(errno));
			stream->fd = -1; /* Force reset the fd, otherwise fd will be stale. */
		} else
			stream->fd = -1;
	}

	TRACE_LEAVE2("%d", rc);
	return rc;
}

static int check_oldest(char *line, char *fname_prefix, int fname_prefix_size, int *old_date, int *old_time)
{
	int date, time, c, d;
	date = time = c = d = 0;
	int len = 0;
	char name_format[NAME_MAX+1];
	char time_stamps[] = "_%d_%d_%d_%d.log";

	len = strlen(time_stamps);
	len += fname_prefix_size;

	strncpy(name_format, fname_prefix, fname_prefix_size);
	name_format[fname_prefix_size] = '\0';
	TRACE_3("fname: %s", name_format);
	strncat(name_format, time_stamps, NAME_MAX);
	if (sscanf(line, name_format, &date, &time, &c, &d) == 4) {
		TRACE_3("line: arg1: %d 2: %d 3: %d 4: %d ok", date, time, c, d);
		if (date < *old_date || *old_date == -1) {
			*old_date = date;
			*old_time = time;
			return 1;
		} else if ((date == *old_date) && (time < *old_time)) {
			*old_date = date;
			*old_time = time;
			return 1;
		}
	} else if (sscanf(line, name_format, &date, &time) == 2) {
		TRACE_3("line: arg1: %d 2: %d ok", date, time);
		if (date < *old_date || *old_date == -1) {
			*old_date = date;
			*old_time = time;
			return 1;
		} else if ((date == *old_date) && (time < *old_time)) {
			*old_date = date;
			*old_time = time;
			return 1;
		}
	} else {
		TRACE_3("no match");
	}
	return 0;
}

/* Filter function used by scandir. */
static char file_prefix[NAME_MAX];
static int filter_func(const struct dirent *finfo)
{
	int ret;
	ret = strncmp(file_prefix, finfo->d_name, strlen(file_prefix));
	return !ret;
}

/**
 * Return number of log files in a dir.
 * @param logStream
 * @param oldest_file
 * @param filter
 * 
 * @return int
 */
static int get_number_of_log_files(log_stream_t *logStream, char *oldest_file)
{
	struct dirent **namelist;
	int n, old_date = -1, old_time = -1, old_ind = -1, files, i, failed = 0;
	char path[PATH_MAX];

	assert(oldest_file != NULL);

	/* Initialize the filter */
	strcpy(file_prefix, logStream->fileName);

	sprintf(path, "%s/%s", lgs_cb->logsv_root_dir, logStream->pathName);
	files = n = scandir(path, &namelist, filter_func, alphasort);
	if (n == -1 && errno == ENOENT)
		return 0;

	if (n < 0) {
		LOG_ER("scandir:%s %s", strerror(errno), path);
		return -1;
	}

	TRACE_3("There are %d files", n);
	if (n == 0)
		return files;

	while (n--) {
		TRACE_3("%s", namelist[n]->d_name);
		if (check_oldest(namelist[n]->d_name, logStream->fileName,
				 strlen(logStream->fileName), &old_date, &old_time)) {
			old_ind = n;
		} else {
			failed++;	/* wrong format */
		}
	}
	if (old_ind != -1) {
		TRACE_1(" oldest: %s", namelist[old_ind]->d_name);
		sprintf(oldest_file, "%s/%s", path, namelist[old_ind]->d_name);
	} else {
		TRACE("Only file/files with wrong format found");
	}

	/* Free scandir allocated memory */
	for (i = 0; i < files; i++)
		free(namelist[i]);
	free(namelist);

	return (files - failed);
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
 * @return int -1 on error, 0 otherwise
 */
int log_stream_write(log_stream_t *stream, const char *buf, size_t count)
{
	int rc, bytes_written = 0;

	assert(stream != NULL && buf != NULL);
	TRACE_ENTER2("%s", stream->name);

	/* Open files on demand e.g. on new active after fail/switch-over. This
	 * enables LOG to cope with temporary file system problems. */
	
	/* Creating config file on new ACTIVE */
	if (stream->fd == -1) {
		if (lgs_create_config_file(stream) != 0) {
			TRACE("Creating config file failed");
			rc = -1;
			goto done;
		}
		TRACE("stream: %s not opened, opening it now", stream->name);
		stream->fd = log_file_open(stream, NULL);
		if (stream->fd == -1) {
			rc = -1;
			goto done;
		}
		TRACE("stream %s now opened", stream->name);
	}

 retry:
	rc = write(stream->fd, &buf[bytes_written], count - bytes_written);
	if (rc == -1) {
		if (errno == EINTR)
			goto retry;

		LOG_ER("write FAILED: %s", strerror(errno));
		/* Careful with log level here to avoid syslog flooding */
		LOG_IN("write '%s' failed - %s", stream->logFileCurrent, strerror(errno));
		goto done;
	} else {
		/* Handle partial writes */
		bytes_written += rc;
		if (bytes_written < stream->fixedLogRecordSize)
			goto retry;
	}

	rc = 0;
	stream->curFileSize += count;

	if ((stream->curFileSize + count) > stream->maxLogFileSize) {
		int errno_save;
		char *current_time = lgs_get_time();

		if ((rc = fileclose(stream->fd)) == -1) {
			LOG_ER("close FAILED: %s", strerror(errno));
			goto done;
		}
		stream->fd = -1;

		rc = lgs_file_rename(stream->pathName, stream->logFileCurrent, current_time, LGS_LOG_FILE_EXT);
		if (rc == -1)
			goto done;

		/* Invalidate logFileCurrent for this stream */
		stream->logFileCurrent[0] = 0;

		/* Remove oldest file if needed */
		if ((rc = rotate_if_needed(stream)) == -1)
			goto done;

		stream->creationTimeStamp = lgs_get_SaTime();
		sprintf(stream->logFileCurrent, "%s_%s", stream->fileName, current_time);
		if ((stream->fd = log_file_open(stream, &errno_save)) == -1) {
			LOG_NO("Could not open '%s' - %s", stream->logFileCurrent, strerror(errno_save));
			rc = -1;
			goto done;
		}

		stream->curFileSize = 0;
	}

 done:
	TRACE_LEAVE2("rc=%u", rc);
	return rc;
}

/**
 * Get stream from array
 * @param lgs_cb
 * @param id
 * 
 * @return log_stream_t*
 */
log_stream_t *log_stream_get_by_id(uns32 id)
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
	int i;

	assert(id != NULL);

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

	if (0 <= id && id < stream_array_size) {
		assert(stream_array[id] != NULL);
		stream_array[id] = NULL;
		rc = 0;
		assert(numb_of_streams > 0);
		numb_of_streams--;
	}

	return rc;
}

/**
 * Dump the stream array
 */
void log_stream_id_print(void)
{
	int i;

	TRACE("  Current number of streams: %u", numb_of_streams);
	for (i = 0; i < stream_array_size; i++) {
		if (stream_array[i] != NULL)
			TRACE("    Id %u - %s", i, stream_array[i]->name);
	}
}

uns32 log_stream_init(void)
{
	NCS_PATRICIA_PARAMS param;
	char *stream_array_size_str;

	/* Get configuration of how many application streams we should allow. */
	stream_array_size_str = getenv("LOG_MAX_APPLICATION_STREAMS");
	if (stream_array_size_str != NULL) {
		long int value = strtol(stream_array_size_str, NULL, 10);
		/* Check for correct conversion and a reasonable amount */
		if (value == LONG_MIN || value == LONG_MAX) {
			LOG_WA("Env var LOG_MAX_APPLICATION_STREAMS has wrong value, using default");
			stream_array_size += DEFAULT_NUM_APP_LOG_STREAMS;
		}
		stream_array_size += value;
	} else
		stream_array_size += DEFAULT_NUM_APP_LOG_STREAMS;

	TRACE("Max %u application log streams", stream_array_size - 3);
	stream_array = calloc(1, sizeof(log_stream_t *) * stream_array_size);
	if (stream_array == NULL) {
		LOG_WA("calloc FAILED");
		return NCSCC_RC_FAILURE;
	}

	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));

	param.key_size = SA_MAX_NAME_LENGTH;

	return ncs_patricia_tree_init(&stream_dn_tree, &param);
}

/**
 * Close log file, change name of log file, create new log and
 * config file. Basically the same logic as described in 3.1.6.4
 * in A.02.01.
 * @param stream
 * @param current_file_name
 * 
 * @return int
 */
int log_stream_config_change(log_stream_t *stream, const char *current_file_name)
{
	int rc;
	char *current_time;

	TRACE_ENTER2("%s", stream->name);

	/* Peer sync needed due to change in logFileCurrent */
	current_time = lgs_get_time();
	
	if (stream->fd == -1) {
		/* lgs has not yet recieved any stream operation request after this swtchover/failover.
		 *  stream shall be opened on-request after a switchover, failover
		 */	
		TRACE("log file of the stream: %s does not exist",stream->name);
	} else {
		/* close the existing log file, and only when there is a valid fd */

		if ((rc = fileclose(stream->fd)) == -1) {
			LOG_ER("log_stream_config_change file close  FAILED: %s", strerror(errno));
			goto done;
		}

		if ((rc = lgs_file_rename(stream->pathName, stream->logFileCurrent, current_time, LGS_LOG_FILE_EXT)) == -1) {
			goto done;
		}

		if ((rc = lgs_file_rename(stream->pathName, current_file_name, current_time, LGS_LOG_FILE_CONFIG_EXT)) == -1) {
			goto done;
		}
	}

	/* Creating the new config file */
	if ((rc = lgs_create_config_file(stream)) != 0)
		goto done;

	sprintf(stream->logFileCurrent, "%s_%s", stream->fileName, current_time);
	
	/* Create the new log file based on updated configuration */
	stream->fd = log_file_open(stream, NULL);

	if (stream->fd == -1)
		rc = -1;
	else
		rc = 0;

 done:
	TRACE_LEAVE();
	return rc;
}
