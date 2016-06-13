/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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

#include "lgs_recov.h"
#include "lgs_file.h"
#include "lgs_filehdl.h"

/***
 * The following functions are used to handle a list of runtime stream objects
 * (application streams with runtime objects) that may be found during start of
 * the log server.
 * The list consists of a vector of strings containing runtime stream object
 * dn:s
 */

static char **rtobj_list = NULL;
static bool rtobj_list_init_f = false;
static uint32_t rtobj_cnt = 0; /* Number of object names in list */
static SaUint32T rtobj_list_len = 0; /* Also the length of the list */

/**
 * Initiate the list by allocating memory for a vector of pointers to strings
 * (char *). The number of elements will be "logMaxApplicationStreams", see log
 * configuration object.
 *
 * @return true if initiated (list exist)
 */
static bool log_rtobj_list_init()
{
	TRACE_ENTER();

	if (rtobj_list_init_f == true)
		goto done; /* Already initiated */

	rtobj_list_len = *static_cast<const SaUint32T *>(
		lgs_cfg_get(LGS_IMM_LOG_MAX_APPLICATION_STREAMS));

	rtobj_list = static_cast<char **>(calloc(rtobj_list_len, sizeof(char *)));
	if (rtobj_list != NULL) {
		rtobj_list_init_f = true;
	} else {
		TRACE("%s\tFail to alloc memory for rtobj_list", __FUNCTION__);
	}

done:
	TRACE_LEAVE2("rtobj_list_init_f = %d", rtobj_list_init_f);
	return rtobj_list_init_f;
}

/**
 * Free an initialized rtobj list
 *
 */
void log_rtobj_list_free()
{
	TRACE_ENTER();

	if (rtobj_list == NULL)
		return; /* No list to free */

	/* Free positions in the list if needed */
	uint32_t pos;
	for (pos = 0; pos < rtobj_list_len; pos++) {
		if (rtobj_list[pos] != NULL)
			free(rtobj_list[pos]);
	}

	free(rtobj_list);
	rtobj_cnt = 0;
	rtobj_list_len = 0;
	rtobj_list = NULL;

	TRACE_LEAVE();
}

/**
 * Add a dn name of an rt-object
 *
 * @param dn_str[in] '\0' terminated string containing a dn
 * @return -1 on error
 */
int log_rtobj_list_add(char *dn_str)
{
	char *str_ptr = NULL;
	size_t len = 0;
	int rc = 0;

	TRACE_ENTER();

	if (log_rtobj_list_init() == false) {
		TRACE("%s\trtobj_list not initiated!", __FUNCTION__);
		rc = -1;
		goto done;
	}

	if (rtobj_cnt >= rtobj_list_len) {
		/* Should be impossible */
		LOG_WA("%s\trtobj_cnt >= maxApplicationStreams!",
			__FUNCTION__);
		rc = -1;
		goto done;
	}

	/* Save dn string */
	len = strlen(dn_str) + 1; /* Including '\0' */
	if (len > SA_MAX_NAME_LENGTH) {
		/* Should never happen */
		LOG_WA("%s\tToo long dn string!",__FUNCTION__);
		rc = -1;
		goto done;
	}

	str_ptr = static_cast<char *>(calloc(1, len));
	if (str_ptr == NULL) {
		LOG_WA("%s\tcalloc Fail",__FUNCTION__);
		rc = -1;
		goto done;
	}

	strcpy(str_ptr, dn_str);

	/* Add dn to list */
	rtobj_list[rtobj_cnt] = str_ptr;
	rtobj_cnt++;

	done:
	TRACE_LEAVE2("rc = %d", rc);
	return rc;
}

/**
 * Return number of names in list
 *
 * @return Number of names
 */
int log_rtobj_list_no()
{
	return rtobj_cnt;
}

/**
 * Find pos of the given given name in list
 *
 * @param dn_str[in] '\0' terminated string with dn to find
 * @return Position of found name or -1 if name not found
 */
int log_rtobj_list_find(char *dn_str)
{
	uint32_t i = 0;
	int pos = -1;

	TRACE_ENTER2("dn_str \"%s\"", dn_str);

	if (rtobj_list == NULL) {
		TRACE("\t No rtobj_list exist");
		goto done;
	}

	for (i = 0; i < rtobj_list_len; i++) {
		if (rtobj_list[i] == NULL)
			continue;
		if (strcmp(rtobj_list[i], dn_str) == 0) {
			/* Found! */
			pos = (int) i;
			break;
		}
	}

done:
	TRACE_LEAVE2("pos = %d", pos);
	return pos;
}

/**
 * Get pos of first name found in list.
 *
 * @return Position of found name or -1 if no name found
 */
int log_rtobj_list_getnamepos()
{
	uint32_t i = 0;
	int pos = -1;

	for (i = 0; i < rtobj_list_len; i++) {
		if (rtobj_list[i] == NULL) {
			continue;
		} else {
			pos = static_cast<int>(i);
			break;
		}
	}

	return pos;
}

/**
 * Get name at given pos in list.
 *
 * @param pos[in] Position for name to get
 * @return A pointer to the string in given pos.
 *         If there is no string in pos or pos is outside the list
 *         NULL is returned.
 */
char *log_rtobj_list_getname(int pos)
{
	if (pos >= static_cast<int>(rtobj_list_len))
		return NULL;

	return rtobj_list[pos];
}

/**
 * "Erase" dn name at pos in list
 *
 * @param pos[in] Position in list to erase
 */
void log_rtobj_list_erase_one_pos(int pos)
{
	TRACE_ENTER2("pos = %d", pos);
	if (pos > static_cast<int>(rtobj_list_len)) {
		TRACE_LEAVE2("%s\tPosition %d outside list!", __FUNCTION__, pos);
		return;
	}

	if (rtobj_list[pos] == NULL) {
		TRACE_LEAVE2("%s\tNo string in pos %d", __FUNCTION__, pos);
		return;
	}

	free(rtobj_list[pos]);
	rtobj_list[pos] = NULL;

	if (rtobj_cnt > 0)
		rtobj_cnt--;

	TRACE_LEAVE();
}

/***
 * Restore a lost runtime stream.
 * Get attributes from given runtime object (if exists).
 * Find the last open logfile. From the file get number of log records written
 * and latest used log record id
 * If fail delete the stream IMM object (if exist)
 * Always delete the object name from the rtobj list
 */

/**
 * Local help function
 * Find the file that was current log file before server down.
 * Get file size
 * Get last written record Id by reading the first characters of the last
 * log record in the file
 *
 * NOTE:
 * All file handling must be done in the file thread
 * See lgs_get_file_params_hdl() in lgs_filehdl.c
 *
 * @param fileName[in]   The name in saLogStreamFileName e.g. "app1"
 * @param pathName[in]	 Full path to the dir where the log file can be found
 *                       root path + relative path
 * @param logFileCurrent[out]
 * @param curFileSize[out]
 * @param logRecordId[out]
 *
 * @return -1 on error
 */
//static int lgs_get_file_params_h(gfp_in_t *par_in, gfp_out_t *par_out)
static int lgs_get_file_params_h(gfp_in_t *par_in, gfp_out_t *par_out)
{
	lgsf_retcode_t api_rc;
	lgsf_apipar_t apipar;
	int rc = 0;

	TRACE_ENTER();

	/* Fill in API structure */
	apipar.req_code_in = LGSF_GET_FILE_PAR;
	apipar.data_in_size = sizeof(gfp_in_t);
	apipar.data_in = par_in;
	apipar.data_out_size = sizeof(gfp_out_t);
	apipar.data_out = par_out;

	api_rc = log_file_api(&apipar);
	if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s", __FUNCTION__,
		      lgsf_retcode_str(api_rc));
		rc = -1;
	} else {
		rc = apipar.hdl_ret_code_out;
		TRACE("\t hdl_ret_code_out = %d", rc);
	}

	TRACE_LEAVE();
	return rc;
}

static void lgs_remove_stream(uint32_t client_id, log_stream_t *log_stream)
{
	TRACE_ENTER();

	/* Remove the stream handle and
	 * remove the stream resources
	 */
       if (lgs_client_stream_rmv(client_id, log_stream->streamId) < 0) {
	       TRACE("%s lgs_client_stream_rmv Fail", __FUNCTION__);
       }

       log_free_stream_resources(log_stream);

       log_stream = NULL;

       TRACE_LEAVE();
}

/**
 * Restore parameters for a lost runtime (application) stream, create the
 * stream in the local stream list and associate with a client.
 * Remove the stream object name from the found objects list.
 *
 * @param stream_name[in]  The name of the stream
 * @param client_id[in]    Client to recover stream for
 * @param o_stream[out]    The recovered stream
 *                         stream
 * @return -1 on error
 */
int lgs_restore_one_app_stream(
	char *stream_name, uint32_t client_id,
	log_stream_t **o_stream)
{
	int int_rc = 0;
	int rc_out = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;
	SaImmHandleT immOmHandle;
	SaImmAttrValuesT_2 *attribute;
	SaImmAttrValuesT_2 **attributes;
	gfp_in_t par_in;
	gfp_out_t par_out;
	int list_pos;
	int n;
	int i = 0;

	lgsv_stream_open_req_t open_stream_param;
	log_stream_t *log_stream = NULL;
	SaTimeT restored_creationTimeStamp = 0;
	SaUint32T restored_severityFilter = 0;

	std::string fileName;
	std::string pathName, fullPathName;

	// Make it safe for free
	par_out.curFileName = NULL;

	TRACE_ENTER2("object_name \"%s\", client_id=%d", stream_name, client_id);

	memset(&open_stream_param, 0, sizeof(open_stream_param));

	/* Check and save stream file name */
	if (stream_name == NULL) {
		TRACE("%s: No object name <NULL>", __FUNCTION__);
		rc_out = -1;
		goto done;
	}
	n = snprintf(reinterpret_cast<char *>(open_stream_param.lstr_name.value),
		SA_MAX_NAME_LENGTH, "%s", stream_name);

	open_stream_param.lstr_name.length = strlen(stream_name) + 1;
	if (n >= SA_MAX_NAME_LENGTH) {
		TRACE("Log stream name \"%s\" is truncated",
			open_stream_param.lstr_name.value);
		rc_out = -1;
		goto done;
	}

	/* Check if in found objects list */
	list_pos = log_rtobj_list_find(stream_name);
	if (list_pos == -1) {
		TRACE("%s: No stream \"%s\" found to restore", __FUNCTION__, stream_name);
		rc_out = -1;
		goto done;
	}

	/* Remove from list */
	log_rtobj_list_erase_one_pos(list_pos);

	/* Get the cached attributes for the object
	 */
	int_rc = lgs_get_streamobj_attr(&attributes, stream_name, &immOmHandle);
	if (int_rc == -1) {
		TRACE("%s:\t lgs_get_streamobj_attr Fail", __FUNCTION__);
		rc_out = -1;
		goto done;
	}

	/* Fill in the "lgsv_stream_open_req_t open_stream_param" struct with
	 * recovered parameters, re-create the stream and add it to the client
	 * Note: Before restoring a stream the client to own the stream must
	 *       exist.
	 */
	i = 0;
	while ((attribute = attributes[i++]) != NULL) {
		void *value;
		char *name;
		char *str_val;

		/* Fill in parameters read from the app stream object
		 */
		name = attribute->attrName;
		if (attribute->attrValuesNumber != 0)
			value = attribute->attrValues[0];
		else {
			value = NULL;
		}

		TRACE("\t attribute name \"%s\"", name);
		if (!strcmp(name, "saLogStreamFileName")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}

			fileName = *(static_cast<char **>(value));
			TRACE("\t saLogStreamFileName \"%s\"", fileName.c_str());
		} else if (!strcmp(name, "saLogStreamPathName")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}

			pathName = *(static_cast<char **>(value));
			TRACE("\t saLogStreamPathName \"%s\"", pathName.c_str());
		} else if (!strcmp(name, "saLogStreamMaxLogFileSize")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}

			open_stream_param.maxLogFileSize = *(static_cast<SaUint64T *>(value));
			TRACE("\t saLogStreamMaxLogFileSize = %lld",
				open_stream_param.maxLogFileSize);

		} else if (!strcmp(name, "saLogStreamFixedLogRecordSize")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}
			open_stream_param.maxLogRecordSize = *(static_cast<SaUint32T *>(value));
			TRACE("\t saLogStreamFixedLogRecordSize = %d",
				open_stream_param.maxLogRecordSize);

		} else if (!strcmp(name, "saLogStreamHaProperty")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}
			open_stream_param.haProperty = *(static_cast<SaBoolT *>(value));
			TRACE("\t saLogStreamHaProperty=%d",
				open_stream_param.haProperty);

		} else if (!strcmp(name, "saLogStreamLogFullAction")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}
			open_stream_param.logFileFullAction = *(static_cast<SaLogFileFullActionT *>(value));
			TRACE("\t saLogStreamLogFullAction=%d",
				open_stream_param.logFileFullAction);

		} else if (!strcmp(name, "saLogStreamMaxFilesRotated")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}
			open_stream_param.maxFilesRotated = *(static_cast<SaUint16T *>(value));
			TRACE("\t saLogStreamMaxFilesRotated=%d",
				open_stream_param.maxFilesRotated);

		} else if (!strcmp(name, "saLogStreamLogFileFormat")) {
			/*TBD Is this correct way of handling format?
			 * - Must be checked!
			 * - Use better max length than PATH_MAX!
			 */
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}
			str_val = *(static_cast<char **>(value));
			open_stream_param.logFileFmt = NULL;
			open_stream_param.logFileFmt = static_cast<char *>(
				calloc(1, strlen(str_val) + 1));
			if (open_stream_param.logFileFmt == NULL) {
				TRACE("%s [%d] calloc Fail",
					__FUNCTION__, __LINE__);
				rc_out = -1;
				goto done_free_attr;
			}
			n = snprintf(open_stream_param.logFileFmt,
				PATH_MAX, "%s", str_val);
			open_stream_param.logFileFmtLength =
				strlen(open_stream_param.logFileFmt);
			if (n >= PATH_MAX) {
				TRACE("Format string \"%s\" too long",
					str_val);
				rc_out = -1;
				goto done_free_attr;
			}
			TRACE("\t saLogStreamLogFileFormat \"%s\"",
				open_stream_param.logFileFmt);

		} else if (!strcmp(name, "saLogStreamCreationTimestamp")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}
			restored_creationTimeStamp = *(static_cast<SaTimeT *>(value));
			TRACE("\t saLogStreamCreationTimestamp=%lld",
				restored_creationTimeStamp);

		} else if (!strcmp(name, "saLogStreamSeverityFilter")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}
			restored_severityFilter = *(static_cast<SaUint32T *>(value));
			TRACE("\t saLogStreamSeverityFilter=%d",
				restored_severityFilter);
		}
	}

	/* Fill in the rest of the stream open parameters
	 * and create the stream. Do not create an IMM object
	 */
	open_stream_param.client_id = client_id;
	open_stream_param.lstr_open_flags = 0; /* Dummy not used here */

	open_stream_param.logFileName = const_cast<char *>(fileName.c_str());
	open_stream_param.logFilePathName = const_cast<char *>(pathName.c_str());

	ais_rc = create_new_app_stream(&open_stream_param, &log_stream, 0);
	if ( ais_rc != SA_AIS_OK) {
		TRACE("%s: create_new_app_stream Fail %s",
			__FUNCTION__, saf_error(ais_rc));
		rc_out = -1;
		goto done_free_attr;
	}

	/* Create an association between this client and the stream */
	int_rc = lgs_client_stream_add(client_id, log_stream->streamId);
	if (int_rc == -1) {
		TRACE("%s: lgs_client_stream_add Fail", __FUNCTION__);
		log_free_stream_resources(log_stream); /* Undo create new stream */
		rc_out = -1;
		goto done_free_attr;
	}

	/* Set values for attributes not restored when creating a stream.
	 * The value must be set after the stream is created.
	 * Cached:
	 * - saLogStreamSeverityFilter (cached)
	 * - saLogStreamCreationTimestamp (cached)
	 *
	 * Pure runtime:
	 * - saLogStreamNumOpeners
	 *   Handled when streams are opened. No recovery needed
	 * - logStreamDiscardedCounter
	 *   Cannot be restored. Initialized with 0
	 */
	log_stream->creationTimeStamp = restored_creationTimeStamp;
	log_stream->severityFilter = restored_severityFilter;
	log_stream->filtered = 0;

	TRACE("\t Stream obj attributes handled and stream is created");

	/* Get recovery data from the log file and update the stream
	 */
	fullPathName = static_cast<const char *>(
		lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));
	fullPathName = fullPathName + "/" + open_stream_param.logFilePathName;

	par_in.file_name = open_stream_param.logFileName;
	par_in.file_path = const_cast<char *>(fullPathName.c_str());

	int_rc = lgs_get_file_params_h(&par_in, &par_out);
	if (int_rc == -1) {
		TRACE("%s:\t lgs_get_file_params_h Fail", __FUNCTION__);
		 /* Remove the stream handle and
		  * remove the stream resources
		  */
		lgs_remove_stream(client_id, log_stream);
		rc_out = -1;
		goto done_free_attr;
	}

	/* If no current log file create a file name
	 */
	if (par_out.curFileName == NULL) {
		/* There is no current log file. Create a file name */
		log_stream->logFileCurrent = log_stream->fileName + "_" + lgs_get_time(NULL);
		TRACE("\t A new file name for current log file is created");
	} else {
		log_stream->logFileCurrent = par_out.curFileName;
	}

	TRACE("\t Current log file \"%s\"", log_stream->logFileCurrent.c_str());

	log_stream->curFileSize = par_out.curFileSize;
	log_stream->logRecordId = par_out.logRecordId;

	/* The stream is open again and opened for the first time so far */
	log_stream->numOpeners = 1;

	/* Set the stream file descriptor to -1. The file will then be opened
	 * at next write.
	 */
	*log_stream->p_fd = -1;

done_free_attr:
	/* Free resources used for finding attribute values */
	int_rc = lgs_free_streamobj_attr(immOmHandle);
	if (int_rc == -1) {
		TRACE("%s:\t lgs_free_streamobj_attr Fail", __FUNCTION__);
		rc_out = -1;
	}

done:
	free(open_stream_param.logFileFmt);
	if (par_out.curFileName != NULL) {
		// This memory is allocated in lgs_get_file_params_hdl()
		free(par_out.curFileName);
	}

	/* Return recovered stream. Will be NULL if not recovered (rc_out = -1) */
	*o_stream = log_stream;
	TRACE_LEAVE2("rc_out = %d", rc_out);
	return rc_out;
}

/**
 * Restore lost files for a configuration stream.
 * Update the stream with current log file and log record Id
 *
 *
 * @param log_stream[in/out]
 * Will/May modify the following stream parameters:
 *   logFileCurrent
 *   curFileSize
 *   logRecordId
 *   creationTimeStamp
 *   *p_fd
 *   numOpeners
 *
 * @return -1 on error
 */
int log_stream_open_file_restore(log_stream_t *stream)
{
	int int_rc = 0;
	int rc_out = 0;
	gfp_in_t par_in;
	gfp_out_t par_out;
	std::string pathName, log_root_path;
	size_t name_length = lgs_max_nlength();

	TRACE_ENTER();

	pathName = static_cast<const char *>(lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));
	pathName = pathName + "/" + stream->pathName;

	par_in.file_name = const_cast<char *>(stream->fileName.c_str());
	par_in.file_path = const_cast<char *>(pathName.c_str());

	TRACE("pathName = %s, fileName = %s, name_length = %zu",
	      pathName.c_str(), stream->fileName.c_str(), name_length);

	// Initialize the output
	par_out.curFileSize = 0;
	par_out.logRecordId = 0;
	par_out.curFileName = NULL;

	int_rc = lgs_get_file_params_h(&par_in, &par_out);

	/****
	 * Rules for lgs_get_file_params_h():
	 * - If current log file not empty: Name = cur log file, Size = file size,
	 *                                  Id = fr file, rc = OK
	 * - If current log file empty no rotated: Name = cur log file, Size = 0,
	 *                                         Id = 1, rc = OK
	 * - If current log file empty is rotated: Name = cur log file, Size = 0,
	 *                                         Id = fr last rotated, rc = OK
	 * - If no log file at all:  Name = NULL, Size = 0, Id = 1, rc = OK
	 * - If only rotated log file: Name = NULL, Size = 0, Id = fr rotated file,
	 *                             rc = OK
	 */

	if (int_rc == -1) {
		/* Recovery fail. Has problem with I/O handling or memory allocation */
		TRACE("%s: lgs_get_file_params_h Fail", __FUNCTION__);
		rc_out = -1;
		goto done;
	}

	/* If no current log file create a file name
	 */
	if (par_out.curFileName == NULL) {
		/* There is no current log file. Consider as logsv starts from scratch */
		TRACE("\t Create new cfg/logfile for stream (%s)", stream->name);
		log_stream_open_fileinit(stream);
		stream->creationTimeStamp = lgs_get_SaTime();
		goto done;
	} else {
		stream->logFileCurrent = par_out.curFileName;
	}

	stream->curFileSize = par_out.curFileSize;
	stream->logRecordId = par_out.logRecordId;

	TRACE("Out: curFileSize = %u, logRecordId = %u, logFileCurrent = %s",
	      stream->curFileSize, stream->logRecordId, stream->logFileCurrent.c_str());

	if (stream->numOpeners != 0) {
		TRACE("%s: numOpeners = %u Fail", __FUNCTION__, stream->numOpeners);
		rc_out = -1;
		goto done;
	}

	int errno_save;
	log_root_path = static_cast<const char *>(lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));
	*stream->p_fd = -1;

	if ((*stream->p_fd = log_file_open(log_root_path, stream,
					   stream->logFileCurrent,
					   &errno_save)) == -1) {
		TRACE("%s - Could not open '%s' - %s", __FUNCTION__,
		      stream->logFileCurrent.c_str(), strerror(errno_save));
	}

	stream->numOpeners++;

done:
	// This memory is allocated in lgs_get_file_params_hdl()
	if (par_out.curFileName != NULL)
		free(par_out.curFileName);

	TRACE_LEAVE2("rc_out = %d", rc_out);
	return rc_out;
}


/**
 *
 * @param stream_name[in]  The name of the stream
 * @return -1 on error
 */
int log_close_rtstream_files(char *stream_name)
{
	int int_rc = 0;
	int rc_out = 0;
	SaImmHandleT immOmHandle;
	SaImmAttrValuesT_2 *attribute;
	SaImmAttrValuesT_2 **attributes;
	gfp_in_t par_in;
	gfp_out_t par_out;
	int i = 0;
	char *current_time = lgs_get_time(NULL);

	std::string fileName, rootPath;
	std::string pathName, pathName_bk;
	std::string emptyStr = "";

	// Make it safe for free
	par_out.curFileName = NULL;

	TRACE_ENTER2("object_name \"%s\"", stream_name);

	/* Check and save stream file name */
	if (stream_name == NULL) {
		TRACE("%s: No object name <NULL>", __FUNCTION__);
		rc_out = -1;
		goto done;
	}

	/* Get the cached attributes for the object
	 */
	int_rc = lgs_get_streamobj_attr(&attributes, stream_name, &immOmHandle);
	if (int_rc == -1) {
		TRACE("%s:\t lgs_get_streamobj_attr Fail", __FUNCTION__);
		rc_out = -1;
		goto done;
	}

	/* Fill in the "lgsv_stream_open_req_t open_stream_param" struct with
	 * recovered parameters, re-create the stream and add it to the client
	 * Note: Before restoring a stream the client to own the stream must
	 *       exist.
	 */
	i = 0;
	while ((attribute = attributes[i++]) != NULL) {
		void *value;
		char *name;

		/* Fill in parameters read from the app stream object
		 */
		name = attribute->attrName;
		if (attribute->attrValuesNumber != 0)
			value = attribute->attrValues[0];
		else {
			value = NULL;
		}

		TRACE("\t attribute name \"%s\"", name);
		if (!strcmp(name, "saLogStreamFileName")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}

			fileName = *(static_cast<char **>(value));
			TRACE("\t saLogStreamFileName \"%s\"", fileName.c_str());
		} else if (!strcmp(name, "saLogStreamPathName")) {
			if (value == NULL) {
				TRACE("%s: Fail, has empty value", name);
				rc_out = -1;
				goto done_free_attr;
			}

			pathName_bk = *(static_cast<char **>(value));
			TRACE("\t saLogStreamPathName \"%s\"", pathName.c_str());
		}
	}


	rootPath = static_cast<const char *>(
		lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));
	pathName = rootPath + "/" + pathName_bk;

	par_in.file_name = const_cast<char *>(fileName.c_str());
	par_in.file_path = const_cast<char *>(pathName.c_str());

	int_rc = lgs_get_file_params_h(&par_in, &par_out);
	if (int_rc == -1) {
		TRACE("%s:\t lgs_get_file_params_h Fail", __FUNCTION__);
		rc_out = -1;
		goto done_free_attr;
	}

	/* If no current log file create a file name
	 */
	if (par_out.curFileName == NULL) {
		LOG_WA("\t No log file exist. Should never happen");
		goto done_free_attr;
	}

	int_rc = lgs_file_rename_h(rootPath, pathName_bk, par_out.curFileName,
				   current_time, LGS_LOG_FILE_EXT, emptyStr);
	if (int_rc == -1) {
		LOG_WA("Failed to rename log file (%s) for stream (%s)",
		       par_out.curFileName, stream_name);
	}

	int_rc = lgs_file_rename_h(rootPath, pathName_bk, fileName,
				   current_time, LGS_LOG_FILE_CONFIG_EXT, emptyStr);

	if (int_rc == -1) {
		LOG_WA("Failed to rename configuration file (%s) for stream (%s)",
		       fileName.c_str(), stream_name);
	}

done_free_attr:
	/* Free resources used for finding attribute values */
	int_rc = lgs_free_streamobj_attr(immOmHandle);
	if (int_rc == -1) {
		TRACE("%s:\t lgs_free_streamobj_attr Fail", __FUNCTION__);
		rc_out = -1;
	}

	if (par_out.curFileName != NULL) {
		// This memory is allocated in lgs_get_file_params_hdl()
		free(par_out.curFileName);
	}
done:
	TRACE_LEAVE2("rc_out = %d", rc_out);
	return rc_out;
}
