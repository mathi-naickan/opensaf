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

#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <saImmOm.h>
#include <saImmOi.h>

#include "immutil.h"
#include "lgs.h"
#include "lgs_util.h"

/* TYPE DEFINITIONS
 * ----------------
 */
typedef struct {
	/* --- Corresponds to IMM Class SaLogConfig --- */
	char logRootDirectory[PATH_MAX];
	SaUint32T logMaxLogrecsize;
	SaUint32T logStreamSystemHighLimit;
	SaUint32T logStreamSystemLowLimit;
	SaUint32T logStreamAppHighLimit;
	SaUint32T logStreamAppLowLimit;
	SaUint32T logMaxApplicationStreams;
	/* --- end correspond to IMM Class --- */

	bool logInitiated;
	bool OpenSafLogConfig_class_exist;

	bool logRootDirectory_noteflag;
	bool logMaxLogrecsize_noteflag;
	bool logStreamSystemHighLimit_noteflag;
	bool logStreamSystemLowLimit_noteflag;
	bool logStreamAppHighLimit_noteflag;
	bool logStreamAppLowLimit_noteflag;
	bool logMaxApplicationStreams_noteflag;
} lgs_conf_t;

/* DATA DECLARATIONS
 * -----------------
 */

/* LOG configuration */
/* Default values are used if no configuration object can be found in IMM */
/* See function lgs_imm_logconf_get */
/* Note: These values should be the same as the values defined as "default"
 * values in the logConfig class definition.
 */
static lgs_conf_t _lgs_conf = {
	.logRootDirectory = "",
	.logMaxLogrecsize = 1024,
	.logStreamSystemHighLimit = 0,
	.logStreamSystemLowLimit = 0,
	.logStreamAppHighLimit = 0,
	.logStreamAppLowLimit = 0,
	.logMaxApplicationStreams = 64,

	.logInitiated = false,
	.OpenSafLogConfig_class_exist = false,
			
	.logRootDirectory_noteflag = false,
	.logMaxLogrecsize_noteflag = false,
	.logStreamSystemHighLimit_noteflag = false,
	.logStreamSystemLowLimit_noteflag = false,
	.logStreamAppHighLimit_noteflag = false,
	.logStreamAppLowLimit_noteflag = false,
	.logMaxApplicationStreams_noteflag = false
};
static lgs_conf_t *lgs_conf = &_lgs_conf;

static bool we_are_applier_flag = false;

const unsigned int sleep_delay_ms = 500;
const unsigned int max_waiting_time_ms = 60 * 1000;	/* 60 secs */

/* Must be able to index this array using streamType */
static char *log_file_format[] = {
	DEFAULT_ALM_NOT_FORMAT_EXP,
	DEFAULT_ALM_NOT_FORMAT_EXP,
	DEFAULT_APP_SYS_FORMAT_EXP,
	DEFAULT_APP_SYS_FORMAT_EXP
};

static SaVersionT immVersion = { 'A', 2, 11 };

static const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)"safLogService";
static const SaImmOiImplementerNameT applierName = (SaImmOiImplementerNameT)"@safLogService";

extern struct ImmutilWrapperProfile immutilWrapperProfile;

/* FUNCTIONS
 * ---------
 */

/**
 * Pack and send a stream checkpoint using mbcsv
 * @param cb
 * @param logStream
 * 
 * @return uns32
 */
static uint32_t ckpt_stream(log_stream_t *stream)
{
	lgsv_ckpt_msg_t ckpt;
	uint32_t rc;

	TRACE_ENTER();

	memset(&ckpt, 0, sizeof(ckpt));
	ckpt.header.ckpt_rec_type = LGS_CKPT_CFG_STREAM;
	ckpt.header.num_ckpt_records = 1;
	ckpt.header.data_len = 1;

	ckpt.ckpt_rec.stream_cfg.name = (char *)stream->name;
	ckpt.ckpt_rec.stream_cfg.fileName = stream->fileName;
	ckpt.ckpt_rec.stream_cfg.pathName = stream->pathName;
	ckpt.ckpt_rec.stream_cfg.maxLogFileSize = stream->maxLogFileSize;
	ckpt.ckpt_rec.stream_cfg.fixedLogRecordSize = stream->fixedLogRecordSize;
	ckpt.ckpt_rec.stream_cfg.logFullAction = stream->logFullAction;
	ckpt.ckpt_rec.stream_cfg.logFullHaltThreshold = stream->logFullHaltThreshold;
	ckpt.ckpt_rec.stream_cfg.maxFilesRotated = stream->maxFilesRotated;
	ckpt.ckpt_rec.stream_cfg.logFileFormat = stream->logFileFormat;
	ckpt.ckpt_rec.stream_cfg.severityFilter = stream->severityFilter;
	ckpt.ckpt_rec.stream_cfg.logFileCurrent = stream->logFileCurrent;

	rc = lgs_ckpt_send_async(lgs_cb, &ckpt, NCS_MBCSV_ACT_ADD);

	TRACE_LEAVE();
	return rc;
}

/**
 * Pack and send a open/close stream checkpoint using mbcsv
 * @param stream
 * @param recType
 *
 * @return uint32
 */
static uint32_t ckpt_stream_open_close(log_stream_t *stream, lgsv_ckpt_msg_type_t recType)
{
	lgsv_ckpt_msg_t ckpt;
	uint32_t rc;

	TRACE_ENTER();

	memset(&ckpt, 0, sizeof(ckpt));
	ckpt.header.ckpt_rec_type = recType;
	ckpt.header.num_ckpt_records = 1;
	ckpt.header.data_len = 1;

	lgs_ckpt_stream_open_set(stream, &ckpt.ckpt_rec.stream_open);

	rc = lgs_ckpt_send_async(lgs_cb, &ckpt, NCS_MBCSV_ACT_ADD);

	TRACE_LEAVE();
	return rc;
}

/**
 * Check if path is a writeable directory
 * We must have rwx access.
 * @param pathname
 * return: true  = Path is valid
 *         false = Path is invalid
 */
static bool path_is_writeable_dir(const char *pathname)
{
	bool is_writeable_dir = false;
	struct stat pathstat;

	TRACE_ENTER();

	if (lgs_relative_path_check_ts(pathname) || stat(pathname, &pathstat) != 0) {
		LOG_NO("Path %s does not exist", pathname);
		goto done;
	}

	/* Check if the path points to a directory */
	if (!S_ISDIR(pathstat.st_mode)) {
		LOG_NO("%s is not a directory", pathname);
		goto done;
	}

	/* Check is we have correct permissions. Note that we check permissions for
	 * real UID
	 */
	if (access(pathname, (R_OK | W_OK | X_OK)) != 0) {
		LOG_NO("permission denied for %s, error %s", pathname, strerror(errno));
		goto done;
	}

	is_writeable_dir = true;
done:
	TRACE_LEAVE();
	return is_writeable_dir;
}

/**
 * LOG Admin operation handling. This function is executed as an
 * IMM callback. In LOG A.02.01 only SA_LOG_ADMIN_CHANGE_FILTER
 * is supported.
 * 
 * @param immOiHandle
 * @param invocation
 * @param objectName
 * @param opId
 * @param params
 */
static void adminOperationCallback(SaImmOiHandleT immOiHandle,
		SaInvocationT invocation, const SaNameT *objectName,
		SaImmAdminOperationIdT opId, const SaImmAdminOperationParamsT_2 **params)
{
	SaUint32T severityFilter;
	const SaImmAdminOperationParamsT_2 *param = params[0];
	log_stream_t *stream;

	TRACE_ENTER2("%s", objectName->value);

	if (lgs_cb->ha_state != SA_AMF_HA_ACTIVE) {
		LOG_ER("admin op callback in applier");
		goto done;
	}

	if ((stream = log_stream_get_by_name((char *)objectName->value)) == NULL) {
		LOG_ER("Stream %s not found", objectName->value);
		(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
		goto done;
	}

	if (opId == SA_LOG_ADMIN_CHANGE_FILTER) {
		/* Only allowed to update runtime objects (application streams) */
		if (stream->streamType != STREAM_TYPE_APPLICATION) {
			LOG_ER("Admin op change filter for non app stream");
			(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto done;
		}

		if (strcmp(param->paramName, "saLogStreamSeverityFilter") != 0) {
			LOG_ER("Admin op change filter, invalid param name");
			(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto done;
		}

		if (param->paramType != SA_IMM_ATTR_SAUINT32T) {
			LOG_ER("Admin op change filter: invalid parameter type");
			(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto done;
		}

		severityFilter = *((SaUint32T *)param->paramBuffer);

		if (severityFilter > 0x7f) {	/* not a level, a bitmap */
			LOG_ER("Admin op change filter: invalid severity");
			(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
			goto done;
		}

		if (severityFilter == stream->severityFilter) {
			(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_NO_OP);
			goto done;
		}

		TRACE("Changing severity for stream %s to %u", stream->name, severityFilter);
		stream->severityFilter = severityFilter;

		(void)immutil_update_one_rattr(immOiHandle, (char *)objectName->value,
					       "saLogStreamSeverityFilter", SA_IMM_ATTR_SAUINT32T,
					       &stream->severityFilter);

		(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);

		/* Checkpoint to standby LOG server */
		ckpt_stream(stream);
	} else {
		LOG_ER("Invalid operation ID, should be %d (one) for change filter", SA_LOG_ADMIN_CHANGE_FILTER);
		(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
	}
 done:
	TRACE_LEAVE();
}

static SaAisErrorT ccbObjectDeleteCallback(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId, const SaNameT *objectName)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER2("CCB ID %llu, '%s'", ccbId, objectName->value);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB object for %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	ccbutil_ccbAddDeleteOperation(ccbUtilCcbData, objectName);

done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT ccbObjectCreateCallback(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId, const SaImmClassNameT className,
		const SaNameT *parentName, const SaImmAttrValuesT_2 **attr)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER2("CCB ID %llu, class '%s', parent '%s'",
			ccbId, className, parentName->value);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB object for %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	ccbutil_ccbAddCreateOperation(ccbUtilCcbData, className, parentName, attr);

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Validate and "memorize" a change request
 * 
 * @param immOiHandle
 * @param ccbId
 * @param objectName
 * @param attrMods
 * @return 
 */
static SaAisErrorT ccbObjectModifyCallback(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId, const SaNameT *objectName,
		const SaImmAttrModificationT_2 **attrMods)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER2("CCB ID %llu, '%s'", ccbId, objectName->value);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB objectfor %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	ccbutil_ccbAddModifyOperation(ccbUtilCcbData, objectName, attrMods);

 done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT config_ccb_completed_create(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);
	LOG_NO("Creation of OpenSafLogConfig object is not supported");
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static SaAisErrorT config_ccb_completed_modify(const CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attrMod;
	SaAisErrorT rc = SA_AIS_OK;
	int i = 0;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	attrMod = opdata->param.modify.attrMods[i++];
	while (attrMod != NULL) {
		void *value;
		const SaImmAttrValuesT_2 *attribute = &attrMod->modAttr;

		TRACE("attribute %s", attribute->attrName);

		if (attribute->attrValuesNumber == 0) {
			LOG_NO("deletion of value is not allowed for attribute %s stream %s",
					attribute->attrName, opdata->objectName.value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "logRootDirectory")) {
			char *pathName = *((char **)value);
			if (!path_is_writeable_dir(pathName)) {
				LOG_NO("pathName: %s is NOT accepted", pathName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			TRACE("pathName: %s is accepted", pathName);
		} else if (!strcmp(attribute->attrName, "logMaxLogrecsize")) {
			LOG_NO("%s cannot be changed", attribute->attrName);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		} else if (!strcmp(attribute->attrName, "logStreamSystemHighLimit")) {
			LOG_NO("%s cannot be changed", attribute->attrName);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		} else if (!strcmp(attribute->attrName, "logStreamSystemLowLimit")) {
			LOG_NO("%s cannot be changed", attribute->attrName);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		} else if (!strcmp(attribute->attrName, "logStreamAppHighLimit")) {
			LOG_NO("%s cannot be changed", attribute->attrName);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		} else if (!strcmp(attribute->attrName, "logStreamAppLowLimit")) {
			LOG_NO("%s cannot be changed", attribute->attrName);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		} else if (!strcmp(attribute->attrName, "logMaxApplicationStreams")) {
			LOG_NO("%s cannot be changed", attribute->attrName);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		} else {
			LOG_NO("attribute %s not recognized", attribute->attrName);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}

		attrMod = opdata->param.modify.attrMods[i++];
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static SaAisErrorT config_ccb_completed_delete(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);
	LOG_NO("Deletion of OpenSafLogConfig object is not supported");
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static SaAisErrorT config_ccb_completed(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		rc = config_ccb_completed_create(opdata);
		break;
	case CCBUTIL_MODIFY:
		rc = config_ccb_completed_modify(opdata);
		break;
	case CCBUTIL_DELETE:
		rc = config_ccb_completed_delete(opdata);
		break;
	default:
		assert(0);
		break;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**********************************************
 * Help functions for check_attr_validity(...)
 **********************************************/

typedef struct {
	/* Store default values for the stream config class. The values are fetched
	 * using saImmOmClassDescriptionGet(...)
	 * Note: Only values relevant for validity checks are stored.
	 */
	SaUint64T saLogStreamMaxLogFileSize;
	SaUint32T saLogStreamFixedLogRecordSize;
} lgs_stream_defval_t;

static bool lgs_stream_defval_updated_flag = false;
static lgs_stream_defval_t lgs_stream_defval;

/**
 * Return a stream config class default values. Fetched using imm om interface
 * 
 * @return  struct of type lgs_stream_defval_t
 */
static lgs_stream_defval_t *get_SaLogStreamConfig_default(void)
{
	SaImmHandleT om_handle = 0;
	SaAisErrorT rc = SA_AIS_OK;
	SaImmClassCategoryT cc;
	SaImmAttrDefinitionT_2 **attributes = NULL;
	SaImmAttrDefinitionT_2 *attribute = NULL;
	int i = 0;
	
	TRACE_ENTER2("448");
	if (lgs_stream_defval_updated_flag == false) {
		/* Get class defaults for SaLogStreamConfig class from IMM
		 * We are only interested in saLogStreamMaxLogFileSize and
		 * saLogStreamFixedLogRecordSize
		 */
		int iu_setting = immutilWrapperProfile.errorsAreFatal;
		immutilWrapperProfile.errorsAreFatal = 0;
		
		rc = immutil_saImmOmInitialize(&om_handle, NULL, &immVersion);
		if (rc != SA_AIS_OK) {
			TRACE("\t448 immutil_saImmOmInitialize fail rc=%d", rc);
		}
		if (rc == SA_AIS_OK) {
			rc = immutil_saImmOmClassDescriptionGet_2(om_handle, "SaLogStreamConfig",
					&cc, &attributes);
		}
		
		if (rc == SA_AIS_OK) {
			while ((attribute = attributes[i++]) != NULL) {
				if (!strcmp(attribute->attrName, "saLogStreamMaxLogFileSize")) {
					TRACE("\t448 Got saLogStreamMaxLogFileSize");
					lgs_stream_defval.saLogStreamMaxLogFileSize =
							*((SaUint64T *) attribute->attrDefaultValue);
					TRACE("\t448 value = %lld",
							lgs_stream_defval.saLogStreamMaxLogFileSize);
				} else if (!strcmp(attribute->attrName, "saLogStreamFixedLogRecordSize")) {
					TRACE("\t448 Got saLogStreamFixedLogRecordSize");
					lgs_stream_defval.saLogStreamFixedLogRecordSize =
							*((SaUint32T *) attribute->attrDefaultValue);
					TRACE("\t448 value = %d",
							lgs_stream_defval.saLogStreamFixedLogRecordSize);
				}
			}
			
			rc = immutil_saImmOmClassDescriptionMemoryFree_2(om_handle, attributes);
			if (rc != SA_AIS_OK) {
				LOG_ER("448 %s: Failed to free class description memory rc=%d",
					__FUNCTION__, rc);
				osafassert(0);
			}
			lgs_stream_defval_updated_flag = true;
		} else {
			/* Default values are not fetched. Temporary use hard coded values.
			 */
			TRACE("\t448 saImmOmClassDescriptionGet_2 failed rc=%d", rc);
			lgs_stream_defval.saLogStreamMaxLogFileSize = 5000000;
			lgs_stream_defval.saLogStreamFixedLogRecordSize = 150;
		}
		
		rc = immutil_saImmOmFinalize(om_handle);
		if (rc != SA_AIS_OK) {
			TRACE("\t448 immutil_saImmOmFinalize fail rc=%d", rc);
		}
				
		immutilWrapperProfile.errorsAreFatal = iu_setting;
	} else {
		TRACE("\t448 Defaults are already fetched");
		TRACE("\t448 saLogStreamMaxLogFileSize=%lld",
				lgs_stream_defval.saLogStreamMaxLogFileSize);
		TRACE("\t448 saLogStreamFixedLogRecordSize=%d",
				lgs_stream_defval.saLogStreamFixedLogRecordSize);
	}
	
	
	TRACE_LEAVE2("448");
	return &lgs_stream_defval;
}

/**
 * Check if a stream with the same file name and relative path already
 * exist
 * 
 * @param immOiHandle
 * @param fileName
 * @param pathName
 * @param stream
 * @param operationType
 * @return true if exists
*/
bool chk_filepath_stream_exist(
		char *fileName,
		char *pathName,
		log_stream_t *stream,
		enum CcbUtilOperationType operationType)
{
	log_stream_t *i_stream = NULL;
	char *i_fileName = NULL;
	char *i_pathName = NULL;
	bool rc = false;
	
	TRACE_ENTER2("448");
	TRACE("\t448 fileName \"%s\", pathName \"%s\"", fileName, pathName);
	
	/* If a stream is modified only the name may be modified. The path name
	 * must be fetched from the stream.
	 */
	if (operationType == CCBUTIL_MODIFY) {
		TRACE("\t448 MODIFY");
		if (stream == NULL) {
			/* No stream to modify. Should never happen */
			osafassert(0);
		}
		if ((fileName == NULL) && (pathName == NULL)) {
			/* Nothing has changed */
			TRACE("\t448 Nothing has changed");
			return false;
		}
		if (fileName == NULL) {
			i_fileName = stream->fileName;
			TRACE("\t448 From stream: fileName \"%s\"", i_fileName);
		} else {
			i_fileName = fileName;
		}
		if (pathName == NULL) {
			i_pathName = stream->pathName;
			TRACE("\t448 From stream: pathName \"%s\"", i_pathName);
		} else {
			i_pathName = pathName;
		}
	} else if (operationType == CCBUTIL_CREATE) {
		TRACE("\t448 CREATE");
		if ((fileName == NULL) || (pathName == NULL)) {
			/* Should never happen
			 * A valid fileName and pathName is always given at create */
			LOG_ER("448 fileName or pathName is not a string");
			osafassert(0);
		}
		
		i_fileName = fileName;
		i_pathName = pathName;
	} else {
		/* Unknown operationType. Should never happen */
			osafassert(0);
	}
	
	/* Check if any stream has given filename and path */
	TRACE("\t448 Check if any stream has given filename and path");
	i_stream = log_stream_getnext_by_name(NULL);
	while (i_stream != NULL) {
		TRACE("\t448 Check stream \"%s\"", i_stream->name);
		if ((strncmp(i_stream->fileName, i_fileName, NAME_MAX) == 0) &&
			(strncmp(i_stream->pathName, i_pathName, SA_MAX_NAME_LENGTH) == 0)) {
			rc = true;
			break;
		}
		i_stream = log_stream_getnext_by_name(i_stream->name);
	}
	
	TRACE_LEAVE2("448 rc = %d", rc);
	return rc;
}

/**
 * Verify fixedLogRecordSize and maxLogFileSize.
 * Rules:
 *  - fixedLogRecordSize must be less than maxLogFileSize
 *  - fixedLogRecordSize must be less than or equal to logMaxLogrecsize
 *  - fixedLogRecordSize can be 0. Means variable record size
 *  - maxLogFileSize must be bigger than 0. No limit is not supported
 *  - maxLogFileSize must be bigger than logMaxLogrecsize
 * 
 * The ..._flag variable == true means that the corresponding attribute is
 * changed.
 * 
 * @param immOiHandle
 * @param maxLogFileSize
 * @param maxLogFileSize_flag
 * @param fixedLogRecordSize
 * @param fixedLogRecordSize_flag
 * @param stream
 * @param operationType
 * @return false if error
 */
static bool chk_max_filesize_recordsize_compatible(SaImmOiHandleT immOiHandle,
		SaUint64T maxLogFileSize,
		bool maxLogFileSize_mod,
		SaUint32T fixedLogRecordSize,
		bool fixedLogRecordSize_mod,
		log_stream_t *stream,
		enum CcbUtilOperationType operationType,
		SaImmOiCcbIdT ccbId)
{
	SaUint64T i_maxLogFileSize = 0;
	SaUint32T i_fixedLogRecordSize = 0;
	SaUint32T i_logMaxLogrecsize = 0;
	lgs_stream_defval_t *stream_default;
	
	bool rc = true;
	
	TRACE_ENTER2("448");
	
	/** Get all parameters **/
	
	/* Get logMaxLogrecsize from configuration parameters */
	i_logMaxLogrecsize = *(SaUint32T *) lgs_imm_logconf_get(
			LGS_IMM_LOG_MAX_LOGRECSIZE, NULL);
	TRACE("\t448 i_logMaxLogrecsize = %d", i_logMaxLogrecsize);
	/* Get stream default settings */
	stream_default = get_SaLogStreamConfig_default();
	
	if (operationType == CCBUTIL_MODIFY) {
		TRACE("\t448 operationType == CCBUTIL_MODIFY");
		/* The stream exists. 
		 */
		if (stream == NULL) {
			/* Should never happen */
			LOG_ER("448 %s stream == NULL", __FUNCTION__);
			osafassert(0);
		}
		if (fixedLogRecordSize_mod == false) {
			/* fixedLogRecordSize is not given. Get from stream */
			i_fixedLogRecordSize = stream->fixedLogRecordSize;
			TRACE("\t448 Get from stream, fixedLogRecordSize = %d",
					i_fixedLogRecordSize);
		} else {
			i_fixedLogRecordSize = fixedLogRecordSize;
		}
		
		if (maxLogFileSize_mod == false) {
			/* maxLogFileSize is not given. Get from stream */
			i_maxLogFileSize = stream->maxLogFileSize;
			TRACE("\t448 Get from stream, maxLogFileSize = %lld",
					i_maxLogFileSize);
		} else {
			i_maxLogFileSize = maxLogFileSize;
			TRACE("\t448 Modified maxLogFileSize = %lld", i_maxLogFileSize);
		}
	} else if (operationType == CCBUTIL_CREATE) {
		TRACE("\t448 operationType == CCBUTIL_CREATE");
		/* The stream does not yet exist
		 */
		if (fixedLogRecordSize_mod == false) {
			/* fixedLogRecordSize is not given. Use default */
			i_fixedLogRecordSize = stream_default->saLogStreamFixedLogRecordSize;
			TRACE("\t448 Get default, fixedLogRecordSize = %d",
					i_fixedLogRecordSize);
		} else {
			i_fixedLogRecordSize = fixedLogRecordSize;
		}
		
		if (maxLogFileSize_mod == false) {
			/* maxLogFileSize is not given. Use default */
			i_maxLogFileSize = stream_default->saLogStreamMaxLogFileSize;
			TRACE("\t448 Get default, maxLogFileSize = %lld",
					i_maxLogFileSize);
		} else {
			i_maxLogFileSize = maxLogFileSize;
		}		
	} else {
		/* Unknown operationType */
		LOG_ER("%s Unknown operationType", __FUNCTION__);
		osafassert(0);
	}
	
	/** Do the verification **/
	if (i_maxLogFileSize <= i_logMaxLogrecsize) {
		/* maxLogFileSize must be bigger than logMaxLogrecsize */
		TRACE("\t448 i_maxLogFileSize (%lld) <= i_logMaxLogrecsize (%d)",
				i_maxLogFileSize, i_logMaxLogrecsize);
		rc = false;
	} else if (i_fixedLogRecordSize == 0) {
		/* fixedLogRecordSize can be 0. Means variable record size */
		TRACE("\t448 fixedLogRecordSize = 0");
		rc = true;
	} else if (i_fixedLogRecordSize >= i_maxLogFileSize) {
		/* fixedLogRecordSize must be less than maxLogFileSize */
		TRACE("\t448 i_fixedLogRecordSize >= i_maxLogFileSize");
		rc = false;
	} else if (i_fixedLogRecordSize > i_logMaxLogrecsize) {
		/* fixedLogRecordSize must be less than maxLogFileSize */
		TRACE("\t448 i_fixedLogRecordSize > i_logMaxLogrecsize");
		rc = false;
	}
	
	TRACE_LEAVE2("448 rc = %d", rc);
	return rc;
}

/**
 * Validate input parameters creation and modify of a persistent stream
 * i.e. a stream that has a configuration object.
 * 
 * @param ccbUtilOperationData
 *
 * @return SaAisErrorT
 */
static SaAisErrorT check_attr_validity(SaImmOiHandleT immOiHandle,
		const struct CcbUtilOperationData *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	void *value = NULL;
	int aindex = 0;
	const SaImmAttrValuesT_2 *attribute = NULL;
	log_stream_t *stream = NULL;
	
	/* Attribute values to be checked
	 */
	/* Mandatory if create. Can be modified */
	char *i_fileName = NULL;
	bool i_fileName_mod = false;
	/* Mandatory if create. Cannot be changed (handled in class definition) */
	char *i_pathName = NULL;
	bool i_pathName_mod = false;
	/* Modification flag -> true if modified */
	SaUint32T i_fixedLogRecordSize = 0;
	bool i_fixedLogRecordSize_mod = false;
	SaUint64T i_maxLogFileSize = 0;
	bool i_maxLogFileSize_mod = false;
	SaUint32T i_logFullAction = 0;
	bool i_logFullAction_mod = false;
	char *i_logFileFormat = NULL;
	bool i_logFileFormat_mod = false;
	SaUint32T i_logFullHaltThreshold = 0;
	bool i_logFullHaltThreshold_mod = false;
	SaUint32T i_maxFilesRotated = 0;
	bool i_maxFilesRotated_mod = false;
	SaUint32T i_severityFilter = 0;
	bool i_severityFilter_mod = false;
	
	TRACE_ENTER2("448");
	
	/* Get first attribute if any and fill in name and path if the stream
	 * exist (modify)
	 */
	if (opdata->operationType == CCBUTIL_MODIFY) {
		TRACE("\t448 Validate for MODIFY");
		stream = log_stream_get_by_name(
				(char *) opdata->param.modify.objectName->value);
		if (stream == NULL) {
			/* No stream to modify */
			TRACE("\t448 Stream does not exist");
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		} else {
			/* Get first attribute */
			if (opdata->param.modify.attrMods[aindex] != NULL) {
				attribute = &opdata->param.modify.attrMods[aindex]->modAttr;
				aindex++;
				TRACE("\t448 First attribute \"%s\" fetched",
						attribute->attrName);
			} else {
				TRACE("\t448 No attributes found");
			}
			/* Get relative path and file name from stream.
			 * Name may be modified
			 */
			i_pathName = stream->pathName;
			i_fileName = stream->fileName;
			TRACE("\t448 From stream: pathName \"%s\", fileName \"%s\"",
					i_pathName, i_fileName);
		}
	} else if (opdata->operationType == CCBUTIL_CREATE){
		TRACE("\t448 Validate for CREATE");
		/* Check if stream already exist after parameters are saved.
		 * Parameter value for fileName is needed.
		 */
		/* Get first attribute */
		attribute = opdata->param.create.attrValues[aindex];
		aindex++;
		TRACE("\t448 First attribute \"%s\" fetched", attribute->attrName);
	} else {
		/* Invalid operation type */
		LOG_ER("%s Invalid operation type", __FUNCTION__);
		osafassert(0);
	}
	

	while ((attribute != NULL) && (rc == SA_AIS_OK)){
		/* Save all changed/given attribute values
		 */
		
		/* Get the value */
		if (attribute->attrValuesNumber > 0) {
			value = attribute->attrValues[0];
		} else if (opdata->operationType == CCBUTIL_MODIFY) {
			/* An attribute without a value is never valid if modify */
			TRACE("\t448 Modify: Attribute %s has no value",attribute->attrName);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		} else {
			/* If create all attributes will be present also the ones without
			 * any value.
			 */
			TRACE("\t448 Create: Attribute %s has no value",attribute->attrName);
			goto next;
		}
		
		/* Save attributes with a value */
		if (!strcmp(attribute->attrName, "saLogStreamFileName")) {
			/* Save filename. Check later together with path name */
			i_fileName = *((char **) value);
			i_fileName_mod = true;
			TRACE("\t448 Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamPathName")) {
			/* Save path name. Check later together with filename */
			i_pathName = *((char **) value);
			i_pathName_mod = true;
			TRACE("\t448 Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamMaxLogFileSize")) {
			/* Save and compare with FixedLogRecordSize after all attributes
			 * are read. Must be bigger than FixedLogRecordSize
			 */
			i_maxLogFileSize = *((SaUint64T *) value);
			i_maxLogFileSize_mod = true;
			TRACE("\t448 Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamFixedLogRecordSize")) {
			/* Save and compare with MaxLogFileSize after all attributes
			 * are read. Must be smaller than MaxLogFileSize
			 */
			i_fixedLogRecordSize = *((SaUint64T *) value);
			i_fixedLogRecordSize_mod = true;
			TRACE("\t448 Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFullAction")) {
			i_logFullAction = *((SaUint32T *) value);
			i_logFullAction_mod = true;
			TRACE("\t448 Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFileFormat")) {
			i_logFileFormat = *((char **) value);
			i_logFileFormat_mod = true;
			TRACE("\t448 Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFullHaltThreshold")) {
			i_logFullHaltThreshold = *((SaUint32T *) value);
			i_logFullHaltThreshold_mod = true;
			TRACE("\t448 Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamMaxFilesRotated")) {
			i_maxFilesRotated = *((SaUint32T *) value);
			i_maxFilesRotated_mod = true;
			TRACE("\t448 Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamSeverityFilter")) {
			i_severityFilter = *((SaUint32T *) value);
			i_severityFilter_mod = true;
			TRACE("\t448 Saved attribute \"%s\" = %d", attribute->attrName,
					i_severityFilter);
			
		}
		
		/* Get next attribute or detect no more attributes */
	next:
		if (opdata->operationType == CCBUTIL_CREATE) {
			attribute = opdata->param.create.attrValues[aindex];
		} else {
			/* CCBUTIL_MODIFY */
			if (opdata->param.modify.attrMods[aindex] != NULL) {
				attribute = &opdata->param.modify.attrMods[aindex]->modAttr;
			} else {
				attribute = NULL;
			}
		}
		aindex++;
	}
	
	/* Check all attributes:
	 * Attributes must be within limits
	 * Note: Mandatory attributes are flagged SA_INITIALIZED meaning that
	 * IMM will reject create if these attributes are not defined. Therefore
	 * this is not checked here.
	 */
	if (rc == SA_AIS_OK) {
		/* saLogStreamPathName
		 * Must be valid and not outside of root path
		 */
		if (i_pathName_mod) {
			TRACE("\t448 Checking saLogStreamPathName");
			if (i_pathName == NULL) {
				/* Must point to a string */
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("\t448 NULL pointer to saLogStreamPathName");
				goto done;
			}
			
			if (lgs_relative_path_check_ts(i_pathName) == true) {
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("\t448 Path %s not valid", i_pathName);
				goto done;
			}
		}
		
		/* saLogStreamFileName
		 * Must be a name. A stream with this name and relative path must not
		 * already exist.
		 */
		if (i_fileName_mod) {
			TRACE("\t448 Checking saLogStreamFileName");
			if (i_fileName == NULL) {
				/* Must point to a string */
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("\t448 NULL pointer to saLogStreamFileName");
				goto done;
			}
			
			if (chk_filepath_stream_exist(i_fileName, i_pathName,
					stream, opdata->operationType)) {
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("\t448 Path/file %s/%s already exist",
						i_pathName, i_fileName);
				goto done;
			}
		}
		
		/* saLogStreamMaxLogFileSize or saLogStreamFixedLogRecordSize
		 * See chk_max_filesize_recordsize_compatible() for rules
		 */
		if (i_maxLogFileSize_mod || i_fixedLogRecordSize_mod) {
			TRACE("\t448 Check saLogStreamMaxLogFileSize,"
					" saLogStreamFixedLogRecordSize");
			if (chk_max_filesize_recordsize_compatible(immOiHandle,
					i_maxLogFileSize,
					i_maxLogFileSize_mod,
					i_fixedLogRecordSize,
					i_fixedLogRecordSize_mod,
					stream,
					opdata->operationType,
					opdata->ccbId) == false) {
				/* report_oi_error is done within check function */
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("\t448 chk_max_filesize_recordsize_compatible Fail");
				goto done;
			}
		}
		
		/* saLogStreamLogFullAction
		 * 1, 2 and 3 are valid according to AIS but only action rotate (3)
		 * is supported.
		 */
		if (i_logFullAction_mod) {
			TRACE("\t448 Check saLogStreamLogFullAction");
			/* If this attribute is changed an oi error report is written
			 * and the value is changed (backwards compatible) but this will
			 * not affect change action
			 */
			if ((i_logFullAction < SA_LOG_FILE_FULL_ACTION_WRAP) ||
					(i_logFullAction > SA_LOG_FILE_FULL_ACTION_ROTATE)) {
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("\t448 logFullAction out of range");
				goto done;
			}
			if ((i_logFullAction == SA_LOG_FILE_FULL_ACTION_WRAP) ||
					(i_logFullAction == SA_LOG_FILE_FULL_ACTION_HALT)) {
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("\t448 logFullAction: Not supported");
				goto done;
			}
		}

		/* saLogStreamLogFileFormat
		 * If create the only possible stream type is application and no stream
		 * is yet created. All streams can be modified.
		 */
		if (i_logFileFormat_mod) {
			SaBoolT dummy;
			if (opdata->operationType == CCBUTIL_CREATE) {
				if (!lgs_is_valid_format_expression(i_logFileFormat, STREAM_TYPE_APPLICATION, &dummy)) {
					rc = SA_AIS_ERR_BAD_OPERATION;
					TRACE("\t448 Create: Invalid logFileFormat: %s",
							i_logFileFormat);
					goto done;
				}
			}
			else {
				if (!lgs_is_valid_format_expression(i_logFileFormat, stream->streamType, &dummy)) {
					rc = SA_AIS_ERR_BAD_OPERATION;
					TRACE("\t448 Modify: Invalid logFileFormat: %s",
							i_logFileFormat);
					goto done;
				}
			}
		}

		/* saLogStreamLogFullHaltThreshold
		 * Not supported and not used. For backwards compatibility the
		 * value can still be set/changed between 0 and 99 (percent)
		 */
		if (i_logFullHaltThreshold_mod) {
			TRACE("\t448 Checking saLogStreamLogFullHaltThreshold");
			if (i_logFullHaltThreshold >= 100) {
				TRACE("\t448 logFullHaltThreshold out of range");
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}
		
		/* saLogStreamMaxFilesRotated
		 * < 127
		 */
		if (i_maxFilesRotated_mod) {
			TRACE("\t448 Checking saLogStreamMaxFilesRotated");
			if ((i_maxFilesRotated < 1) || (i_maxFilesRotated > 127)) {
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("\t448 maxFilesRotated out of range "
						"(min 1, max 127): %u", i_maxFilesRotated);
				goto done;
			}
		}
		
		/* saLogStreamSeverityFilter
		 *     < 0x7f
		 */
		if (i_severityFilter_mod) {
			TRACE("\t448 Checking saLogStreamSeverityFilter");
			if (i_severityFilter >= 0x7f) {
				TRACE("\t448 Invalid severity: %x", i_severityFilter);
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("\t448 Invalid severity: %x", i_severityFilter);
				goto done;
			}
		}
	}
	
	done:
		TRACE_LEAVE2("448 rc = %d", rc);
		return rc;
}

static SaAisErrorT stream_ccb_completed_create(SaImmOiHandleT immOiHandle,
		const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);
	rc = check_attr_validity(immOiHandle, opdata);
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static SaAisErrorT stream_ccb_completed_modify(SaImmOiHandleT immOiHandle,
		const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);
	rc = check_attr_validity(immOiHandle, opdata);
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static SaAisErrorT stream_ccb_completed_delete(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	const char *name = (char*) opdata->param.deleteOp.objectName->value;
	log_stream_t *stream = log_stream_get_by_name(name);

	if (stream != NULL) {
		if (stream->streamId < 3) {
			LOG_ER("Stream delete: well known stream '%s' cannot be deleted", name);
			goto done;
		}

		if (stream->numOpeners > 1) {
			LOG_ER("Stream '%s' cannot be deleted: opened by %u clients", name, stream->numOpeners);
			goto done;
		}

		rc = SA_AIS_OK;
	} else {
		LOG_ER("stream %s not found", name);
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static SaAisErrorT stream_ccb_completed(SaImmOiHandleT immOiHandle,
		const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		rc = stream_ccb_completed_create(immOiHandle, opdata);
		break;
	case CCBUTIL_MODIFY:
		rc = stream_ccb_completed_modify(immOiHandle, opdata);
		break;
	case CCBUTIL_DELETE:
		rc = stream_ccb_completed_delete(opdata);
		break;
	default:
		assert(0);
		break;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**
 * The CCB is now complete. Verify that the changes can be applied
 * 
 * @param immOiHandle
 * @param ccbId
 * @return 
 */
static SaAisErrorT ccbCompletedCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *opdata;

	TRACE_ENTER2("CCB ID %llu", ccbId);

	if (lgs_cb->ha_state != SA_AMF_HA_ACTIVE) {
		TRACE("State Not Active. Nothing to do, we are an applier");
		goto done;
	}

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_ER("%s: Failed to find CCB object for %llu", __FUNCTION__, ccbId);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done; // or exit?
	}

	/*
	 ** "check that the sequence of change requests contained in the CCB is 
	 ** valid and that no errors will be generated when these changes
	 ** are applied."
	 */
	for (opdata = ccbUtilCcbData->operationListHead; opdata; opdata = opdata->next) {
		switch (opdata->operationType) {
		case CCBUTIL_CREATE:
			if (!strcmp(opdata->param.create.className, "OpenSafLogConfig"))
				rc = config_ccb_completed(opdata);
			else if (!strcmp(opdata->param.create.className, "SaLogStreamConfig"))
				rc = stream_ccb_completed(immOiHandle, opdata);
			else
				osafassert(0);
			break;
		case CCBUTIL_DELETE:
		case CCBUTIL_MODIFY:
			if (!strncmp((char*)opdata->objectName.value, "safLgStrCfg", 11))
				rc = stream_ccb_completed(immOiHandle, opdata);
			else
				rc = config_ccb_completed(opdata);
			break;
		default:
			assert(0);
			break;
		}
	}

 done:
	TRACE_LEAVE2("rc=%u",rc);
	return rc;
}
/**
 * Set logRootDirectory to new value
 *   - Close all open logfiles
 *   - Rename all log files and .cfg files.
 *   - Update lgs_conf with new path (logRootDirectory).
 *   - Open all logfiles and .cfg files in new directory.
 *
 * @param logRootDirectory
 */
static void logRootDirectory_set(const char *logRootDirectory)
{
	log_stream_t *stream;
	char *current_time = lgs_get_time();

	/* Close and rename files at current path */
	stream = log_stream_getnext_by_name(NULL);
	while (stream != NULL) {
		TRACE("Handling file %s", stream->logFileCurrent);
		// TODO: restore/refactor log_stream_config_change back to original
		if (log_stream_config_change(!LGS_STREAM_CREATE_FILES, stream, stream->fileName) != 0) {
			LOG_ER("Old log files could not be renamed and closed, root-dir: %s",
					lgs_cb->logsv_root_dir);
		}
		stream = log_stream_getnext_by_name(stream->name);
	}

	strncpy(lgs_conf->logRootDirectory, logRootDirectory, PATH_MAX);

	/* Create new files at new path */
	stream = log_stream_getnext_by_name(NULL);
	while (stream != NULL) {
		if (lgs_create_config_file(stream) != 0) {
			LOG_ER("New config file could not be created for stream: %s",
					stream->name);
		}

		/* Create the new log file based on updated configuration */
		sprintf(stream->logFileCurrent, "%s_%s", stream->fileName, current_time);
		if ((stream->fd = log_file_open(stream, NULL)) == -1) {
			LOG_ER("New log file could not be created for stream: %s",
					stream->name);
		}
		stream = log_stream_getnext_by_name(stream->name);
	}
}

/**
 * Apply validated changes
 *
 * @param opdata
 */
static void config_ccb_apply_modify(const CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attrMod;
	int i = 0;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	attrMod = opdata->param.modify.attrMods[i++];
	while (attrMod != NULL) {
		const SaImmAttrValuesT_2 *attribute = &attrMod->modAttr;
		void *value = attribute->attrValues[0];

		TRACE("attribute %s", attribute->attrName);

		if (!strcmp(attribute->attrName, "logRootDirectory")) {
			/* Update saved configuration on both active and standby */
			const char *logRootDirectory = *((char **)value);

			if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE) {
				logRootDirectory_set(logRootDirectory);
			} else {
				strcpy(lgs_conf->logRootDirectory, logRootDirectory);
			}

			LOG_NO("log root directory changed to: %s", lgs_cb->logsv_root_dir);
		} else {
			// validation should not allow any other change
			osafassert(0);
		}

		attrMod = opdata->param.modify.attrMods[i++];
	}

	TRACE_LEAVE();
}

static void config_ccb_apply(const CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		osafassert(0);
		break;
	case CCBUTIL_MODIFY:
		config_ccb_apply_modify(opdata);
		break;
	case CCBUTIL_DELETE:
		osafassert(0);
		break;
	default:
		assert(0);
		break;
	}

	TRACE_LEAVE();
}

/**
 * Allocate and initialize new application configuration stream object.
 * @param ccb
 *
 * @return SaAisErrorT
 */
static SaAisErrorT stream_create_and_configure1(const struct CcbUtilOperationData* ccb,
		log_stream_t** stream)
{
	SaAisErrorT rc = SA_AIS_OK;
	*stream = NULL;
	int i = 0;
	SaNameT objectName;

	TRACE_ENTER();

	while (ccb->param.create.attrValues[i] != NULL) {
		if (!strcmp(ccb->param.create.attrValues[i]->attrName, "safLgStrCfg")) {
			if (ccb->param.create.parentName->length > 0) {
				objectName.length = snprintf((char*) objectName.value, sizeof(objectName.value),
						"%s,%s", *(const SaStringT*) ccb->param.create.attrValues[i]->attrValues[0],
						ccb->param.create.parentName->value);
			} else {
				objectName.length = snprintf((char*) objectName.value, sizeof(objectName.value),
						"%s", *(const SaStringT*) ccb->param.create.attrValues[i]->attrValues[0]);
			}

			if ((*stream = log_stream_new_2(&objectName, STREAM_NEW)) == NULL) {
				rc = SA_AIS_ERR_NO_MEMORY;
				goto done;
			}
		}
		i++;
	}

	if (*stream == NULL)
		goto done;

	i = 0;

	// a configurable application stream.
	(*stream)->streamType = STREAM_TYPE_APPLICATION;

	while (ccb->param.create.attrValues[i] != NULL) {
		if (ccb->param.create.attrValues[i]->attrValuesNumber > 0) {
			SaImmAttrValueT value = ccb->param.create.attrValues[i]->attrValues[0];

			if (!strcmp(ccb->param.create.attrValues[i]->attrName, "saLogStreamFileName")) {
				strcpy((*stream)->fileName, *((char **) value));
				TRACE("fileName: %s", (*stream)->fileName);
			} else if (!strcmp(ccb->param.create.attrValues[i]->attrName, "saLogStreamPathName")) {
				strcpy((*stream)->pathName, *((char **) value));
				TRACE("pathName: %s", (*stream)->pathName);
			} else if (!strcmp(ccb->param.create.attrValues[i]->attrName, "saLogStreamMaxLogFileSize")) {
				(*stream)->maxLogFileSize = *((SaUint64T *) value);
				TRACE("maxLogFileSize: %llu", (*stream)->maxLogFileSize);
			} else if (!strcmp(ccb->param.create.attrValues[i]->attrName, "saLogStreamFixedLogRecordSize")) {
				(*stream)->fixedLogRecordSize = *((SaUint32T *) value);
				TRACE("fixedLogRecordSize: %u", (*stream)->fixedLogRecordSize);
			} else if (!strcmp(ccb->param.create.attrValues[i]->attrName, "saLogStreamLogFullAction")) {
				(*stream)->logFullAction = *((SaUint32T *) value);
				TRACE("logFullAction: %u", (*stream)->logFullAction);
			} else if (!strcmp(ccb->param.create.attrValues[i]->attrName, "saLogStreamLogFullHaltThreshold")) {
				(*stream)->logFullHaltThreshold = *((SaUint32T *) value);
				TRACE("logFullHaltThreshold: %u", (*stream)->logFullHaltThreshold);
			} else if (!strcmp(ccb->param.create.attrValues[i]->attrName, "saLogStreamMaxFilesRotated")) {
				(*stream)->maxFilesRotated = *((SaUint32T *) value);
				TRACE("maxFilesRotated: %u", (*stream)->maxFilesRotated);
			} else if (!strcmp(ccb->param.create.attrValues[i]->attrName, "saLogStreamLogFileFormat")) {
				SaBoolT dummy;
				char *logFileFormat = *((char **) value);
				if (!lgs_is_valid_format_expression(logFileFormat, (*stream)->streamType, &dummy)) {
					LOG_WA("Invalid logFileFormat for stream %s, using default", (*stream)->name);
					logFileFormat = DEFAULT_APP_SYS_FORMAT_EXP;
				}

				(*stream)->logFileFormat = strdup(logFileFormat);
				TRACE("logFileFormat: %s", (*stream)->logFileFormat);
			} else if (!strcmp(ccb->param.create.attrValues[i]->attrName, "saLogStreamSeverityFilter")) {
				(*stream)->severityFilter = *((SaUint32T *) value);
				TRACE("severityFilter: %u", (*stream)->severityFilter);
			}
		}
		i++;
	} // while

	if ((*stream)->logFileFormat == NULL)
		(*stream)->logFileFormat = strdup(log_file_format[(*stream)->streamType]);
#if 0
	// TODO: fails with NOT_EXIST, post an event to ourselves?
	/* Update creation timestamp */
	(void) immutil_update_one_rattr(lgs_cb->immOiHandle, (const char*) objectName.value,
			"saLogStreamCreationTimestamp", SA_IMM_ATTR_SATIMET,
			&(*stream)->creationTimeStamp);
#endif
	done:
	TRACE_LEAVE();
	return rc;
}

static void stream_ccb_apply_create(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	log_stream_t *stream;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	if ((rc = stream_create_and_configure1(opdata, &stream)) == SA_AIS_OK) {
		if (log_stream_open(stream) == SA_AIS_OK) {
			ckpt_stream_open_close(stream, LGS_CKPT_OPEN_STREAM);
		} else {
			; // what?
		}
	} else {
		LOG_IN("Stream create and configure failed %d", rc);
	}

	TRACE_LEAVE();
}

static void stream_ccb_apply_modify(const CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attrMod;
	int i = 0;
	log_stream_t *stream;
	char current_file_name[NAME_MAX];
	bool new_cfg_file_needed = false;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	stream = log_stream_get_by_name((char*)opdata->objectName.value);
	osafassert(stream);

	strncpy(current_file_name, stream->fileName, sizeof(current_file_name));

	attrMod = opdata->param.modify.attrMods[i++];
	while (attrMod != NULL) {
		void *value;
		const SaImmAttrValuesT_2 *attribute = &attrMod->modAttr;

		TRACE("attribute %s", attribute->attrName);

		value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "saLogStreamFileName")) {
			char *fileName = *((char **)value);
			strcpy(stream->fileName, fileName);
			new_cfg_file_needed = true;
		} else if (!strcmp(attribute->attrName, "saLogStreamMaxLogFileSize")) {
			SaUint64T maxLogFileSize = *((SaUint64T *)value);
			stream->maxLogFileSize = maxLogFileSize;
			new_cfg_file_needed = true;
		} else if (!strcmp(attribute->attrName, "saLogStreamFixedLogRecordSize")) {
			SaUint32T fixedLogRecordSize = *((SaUint32T *)value);
			stream->fixedLogRecordSize = fixedLogRecordSize;
			new_cfg_file_needed = true;
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFullAction")) {
			SaLogFileFullActionT logFullAction = *((SaUint32T *)value);
			stream->logFullAction = logFullAction;
			new_cfg_file_needed = true;
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFullHaltThreshold")) {
			SaUint32T logFullHaltThreshold = *((SaUint32T *)value);
			stream->logFullHaltThreshold = logFullHaltThreshold;
		} else if (!strcmp(attribute->attrName, "saLogStreamMaxFilesRotated")) {
			SaUint32T maxFilesRotated = *((SaUint32T *)value);
			stream->maxFilesRotated = maxFilesRotated;
			new_cfg_file_needed = true;
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFileFormat")) {
			char *logFileFormat = *((char **)value);
			if (stream->logFileFormat != NULL)
				free(stream->logFileFormat);
			stream->logFileFormat = strdup(logFileFormat);
			new_cfg_file_needed = true;
		} else if (!strcmp(attribute->attrName, "saLogStreamSeverityFilter")) {
			SaUint32T severityFilter = *((SaUint32T *)value);
			stream->severityFilter = severityFilter;
		} else {
			osafassert(0);
		}

		attrMod = opdata->param.modify.attrMods[i++];
	}

	if (new_cfg_file_needed) {
		int rc;
		if ((rc = log_stream_config_change(LGS_STREAM_CREATE_FILES, stream, current_file_name))
				!= 0) {
			LOG_ER("log_stream_config_change failed: %d", rc);
		}
	}

	/* Checkpoint to standby LOG server */
	ckpt_stream(stream);

	TRACE_LEAVE();
}

static void stream_ccb_apply_delete(const CcbUtilOperationData_t *opdata)
{
	log_stream_t *stream;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	stream = log_stream_get_by_name((char *) opdata->objectName.value);

	/* Checkpoint to standby LOG server */
	ckpt_stream_open_close(stream, LGS_CKPT_CLOSE_STREAM);
	log_stream_close(&stream);
}

static void stream_ccb_apply(const CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		stream_ccb_apply_create(opdata);
		break;
	case CCBUTIL_MODIFY:
		stream_ccb_apply_modify(opdata);
		break;
	case CCBUTIL_DELETE:
		stream_ccb_apply_delete(opdata);
		break;
	default:
		assert(0);
		break;
	}

	TRACE_LEAVE();
}

/**
 * Configuration changes are done and now it's time to act on the changes
 * 
 * @param immOiHandle
 * @param ccbId
 */
static void ccbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *opdata;

	TRACE_ENTER2("CCB ID %llu", ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_ER("%s: Failed to find CCB object for %llu", __FUNCTION__, ccbId);
		goto done; // or exit?
	}

	for (opdata = ccbUtilCcbData->operationListHead; opdata; opdata = opdata->next) {
		switch (opdata->operationType) {
		case CCBUTIL_CREATE:
			if (!strcmp(opdata->param.create.className, "OpenSafLogConfig"))
				config_ccb_apply(opdata);
			else if (!strcmp(opdata->param.create.className, "SaLogStreamConfig"))
				stream_ccb_apply(opdata);
			else
				osafassert(0);
			break;
		case CCBUTIL_DELETE:
		case CCBUTIL_MODIFY:
			if (!strncmp((char*)opdata->objectName.value, "safLgStrCfg", 11))
				stream_ccb_apply(opdata);
			else
				config_ccb_apply(opdata);
			break;
		default:
			assert(0);
			break;
		}
	}

done:
	ccbutil_deleteCcbData(ccbUtilCcbData);
	TRACE_LEAVE();
}

static void ccbAbortCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER2("CCB ID %llu", ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) != NULL)
		ccbutil_deleteCcbData(ccbUtilCcbData);
	else
		LOG_ER("%s: Failed to find CCB object for %llu", __FUNCTION__, ccbId);

	TRACE_LEAVE();
}

/**
 * IMM requests us to update a non cached runtime attribute. The
 * only available attribute is saLogStreamNumOpeners.
 * @param immOiHandle
 * @param objectName
 * @param attributeNames
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT rtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
		const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	SaAisErrorT rc = SA_AIS_ERR_FAILED_OPERATION;
	SaImmAttrNameT attributeName;
	int i = 0;
	log_stream_t *stream = log_stream_get_by_name((char *)objectName->value);

	TRACE_ENTER2("%s", objectName->value);

	if (lgs_cb->ha_state != SA_AMF_HA_ACTIVE) {
		LOG_ER("admin op callback in applier");
		goto done;
	}

	if (stream == NULL) {
		LOG_ER("%s: stream %s not found", __FUNCTION__, objectName->value);
		goto done;
	}

	while ((attributeName = attributeNames[i++]) != NULL) {
		TRACE("Attribute %s", attributeName);
		if (!strcmp(attributeName, "saLogStreamNumOpeners")) {
			(void)immutil_update_one_rattr(immOiHandle, (char *)objectName->value,
						       attributeName, SA_IMM_ATTR_SAUINT32T, &stream->numOpeners);
		} else {
			LOG_ER("%s: unknown attribute %s", __FUNCTION__, attributeName);
			goto done;
		}
	}

	rc = SA_AIS_OK;

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Allocate new stream object. Get configuration from IMM and
 * initialize the stream object.
 * @param dn
 * @param in_stream
 * @param stream_id
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT stream_create_and_configure(const char *dn, log_stream_t **in_stream, int stream_id)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaImmHandleT omHandle;
	SaNameT objectName;
	SaVersionT immVersion = { 'A', 2, 1 };
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 *attribute;
	SaImmAttrValuesT_2 **attributes;
	int i = 0;
	log_stream_t *stream;
	
	int asetting = immutilWrapperProfile.errorsAreFatal;
	SaAisErrorT om_rc = SA_AIS_OK;

	TRACE_ENTER2("(%s)", dn);

	(void)immutil_saImmOmInitialize(&omHandle, NULL, &immVersion);
	(void)immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle);

	strncpy((char *)objectName.value, dn, SA_MAX_NAME_LENGTH);
	objectName.length = strlen((char *)objectName.value);

	*in_stream = stream = log_stream_new_2(&objectName, stream_id);

	if (stream == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Happens to be the same, ugly! FIX */
	stream->streamType = stream_id;

	/* Get all attributes of the object */
	if (immutil_saImmOmAccessorGet_2(accessorHandle, &objectName, NULL, &attributes) != SA_AIS_OK) {
		LOG_ER("Configuration for %s not found", objectName.value);
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

	while ((attribute = attributes[i++]) != NULL) {
		void *value;

		if (attribute->attrValuesNumber == 0)
			continue;

		value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "saLogStreamFileName")) {
			strcpy(stream->fileName, *((char **)value));
			TRACE("fileName: %s", stream->fileName);
		} else if (!strcmp(attribute->attrName, "saLogStreamPathName")) {
			strcpy(stream->pathName, *((char **)value));
			TRACE("pathName: %s", stream->pathName);
		} else if (!strcmp(attribute->attrName, "saLogStreamMaxLogFileSize")) {
			stream->maxLogFileSize = *((SaUint64T *)value);
			TRACE("maxLogFileSize: %llu", stream->maxLogFileSize);
		} else if (!strcmp(attribute->attrName, "saLogStreamFixedLogRecordSize")) {
			stream->fixedLogRecordSize = *((SaUint32T *)value);
			TRACE("fixedLogRecordSize: %u", stream->fixedLogRecordSize);
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFullAction")) {
			stream->logFullAction = *((SaUint32T *)value);
			TRACE("logFullAction: %u", stream->logFullAction);
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFullHaltThreshold")) {
			stream->logFullHaltThreshold = *((SaUint32T *)value);
			TRACE("logFullHaltThreshold: %u", stream->logFullHaltThreshold);
		} else if (!strcmp(attribute->attrName, "saLogStreamMaxFilesRotated")) {
			stream->maxFilesRotated = *((SaUint32T *)value);
			TRACE("maxFilesRotated: %u", stream->maxFilesRotated);
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFileFormat")) {
			SaBoolT dummy;
			char *logFileFormat = *((char **)value);
			if (!lgs_is_valid_format_expression(logFileFormat, stream->streamType, &dummy)) {
				LOG_WA("Invalid logFileFormat for stream %s, using default", stream->name);

				if ((stream->streamType == STREAM_TYPE_SYSTEM) ||
				    (stream->streamType == STREAM_TYPE_APPLICATION))
					logFileFormat = DEFAULT_APP_SYS_FORMAT_EXP;

				if ((stream->streamType == STREAM_TYPE_ALARM) ||
				    (stream->streamType == STREAM_TYPE_NOTIFICATION))
					logFileFormat = DEFAULT_ALM_NOT_FORMAT_EXP;
			}

			stream->logFileFormat = strdup(logFileFormat);
			TRACE("logFileFormat: %s", stream->logFileFormat);
		} else if (!strcmp(attribute->attrName, "saLogStreamSeverityFilter")) {
			stream->severityFilter = *((SaUint32T *)value);
			TRACE("severityFilter: %u", stream->severityFilter);
		}
	}

	if (stream->logFileFormat == NULL)
		stream->logFileFormat = strdup(log_file_format[stream->streamType]);

 done:
	/* Do not abort if error when finalizing */
	immutilWrapperProfile.errorsAreFatal = 0;	/* Disable immutil abort */
	om_rc = immutil_saImmOmAccessorFinalize(accessorHandle);
	if (om_rc != SA_AIS_OK) {
		LOG_NO("%s immutil_saImmOmAccessorFinalize() Fail %d",__FUNCTION__, om_rc);
	}
	om_rc = immutil_saImmOmFinalize(omHandle);
	if (om_rc != SA_AIS_OK) {
		LOG_NO("%s immutil_saImmOmFinalize() Fail %d",__FUNCTION__, om_rc);
	}
	immutilWrapperProfile.errorsAreFatal = asetting; /* Enable again */

	TRACE_LEAVE();
	return rc;
}

/**
 * Get Log configuration from IMM. See SaLogConfig class.
 * The configuration will be read from object 'dn' and
 * written to struct lgsConf.
 * Returns SA_AIS_ERR_NOT_EXIST if no object 'dn' exist.
 * 
 * @param dn
 * @param lgsConf
 * 
 * @return SaAisErrorT
 * SA_AIS_OK, SA_AIS_ERR_NOT_EXIST
 */
static SaAisErrorT read_logsv_config_obj(const char *dn, lgs_conf_t *lgsConf) {
	SaAisErrorT rc = SA_AIS_OK;
	SaImmHandleT omHandle;
	SaNameT objectName;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 *attribute;
	SaImmAttrValuesT_2 **attributes;
	int i = 0;
	int param_cnt = 0;

	int iu_setting = immutilWrapperProfile.errorsAreFatal;
	SaAisErrorT om_rc = SA_AIS_OK;
	
	TRACE_ENTER2("(%s)", dn);

	strncpy((char *) objectName.value, dn, SA_MAX_NAME_LENGTH);
	objectName.length = strlen((char *) objectName.value);

	/* NOTE: immutil init will osaf_assert if error */
	(void) immutil_saImmOmInitialize(&omHandle, NULL, &immVersion);
	(void) immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle);

	/* Get all attributes of the object */
	if (immutil_saImmOmAccessorGet_2(accessorHandle, &objectName, NULL, &attributes) != SA_AIS_OK) {
		lgsConf->OpenSafLogConfig_class_exist = false;
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}
	else {
		lgsConf->OpenSafLogConfig_class_exist = true;
	}

	while ((attribute = attributes[i++]) != NULL) {
		void *value;

		if (attribute->attrValuesNumber == 0)
			continue;

		value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "logRootDirectory")) {
			strncpy((char *) lgsConf->logRootDirectory, *((char **) value), PATH_MAX);
			param_cnt++;
			TRACE("Conf obj; logRootDirectory: %s", lgsConf->logRootDirectory);
		} else if (!strcmp(attribute->attrName, "logMaxLogrecsize")) {
			lgsConf->logMaxLogrecsize = *((SaUint32T *) value);
			param_cnt++;
			TRACE("Conf obj; logMaxLogrecsize: %u", lgsConf->logMaxLogrecsize);
		} else if (!strcmp(attribute->attrName, "logStreamSystemHighLimit")) {
			lgsConf->logStreamSystemHighLimit = *((SaUint32T *) value);
			param_cnt++;
			TRACE("Conf obj; logStreamSystemHighLimit: %u", lgsConf->logStreamSystemHighLimit);
		} else if (!strcmp(attribute->attrName, "logStreamSystemLowLimit")) {
			lgsConf->logStreamSystemLowLimit = *((SaUint32T *) value);
			param_cnt++;
			TRACE("Conf obj; logStreamSystemLowLimit: %u", lgsConf->logStreamSystemLowLimit);
		} else if (!strcmp(attribute->attrName, "logStreamAppHighLimit")) {
			lgsConf->logStreamAppHighLimit = *((SaUint32T *) value);
			param_cnt++;
			TRACE("Conf obj; logStreamAppHighLimit: %u", lgsConf->logStreamAppHighLimit);
		} else if (!strcmp(attribute->attrName, "logStreamAppLowLimit")) {
			lgsConf->logStreamAppLowLimit = *((SaUint32T *) value);
			param_cnt++;
			TRACE("Conf obj; logStreamAppLowLimit: %u", lgsConf->logStreamAppLowLimit);
		} else if (!strcmp(attribute->attrName, "logMaxApplicationStreams")) {
			lgsConf->logMaxApplicationStreams = *((SaUint32T *) value);
			param_cnt++;
			TRACE("Conf obj; logMaxApplicationStreams: %u", lgsConf->logMaxApplicationStreams);
		}
	}

	/* Check if missing parameters */
	if (param_cnt != LGS_IMM_LOG_NUMBER_OF_PARAMS) {
		lgsConf->OpenSafLogConfig_class_exist = false;
		rc = SA_AIS_ERR_NOT_EXIST;
		LOG_ER("read_logsv_configuration(), Parameter error. All parameters could not be read");
	}

done:
	/* Do not abort if error when finalizing */
	immutilWrapperProfile.errorsAreFatal = 0;	/* Disable immutil abort */
	om_rc = immutil_saImmOmAccessorFinalize(accessorHandle);
	if (om_rc != SA_AIS_OK) {
		LOG_NO("%s immutil_saImmOmAccessorFinalize() Fail %d",__FUNCTION__, om_rc);
	}
	om_rc = immutil_saImmOmFinalize(omHandle);
	if (om_rc != SA_AIS_OK) {
		LOG_NO("%s immutil_saImmOmFinalize() Fail %d",__FUNCTION__, om_rc);
	}
	immutilWrapperProfile.errorsAreFatal = iu_setting; /* Enable again */

	TRACE_LEAVE();
	return rc;
}

/**
 * Handle logsv configuration environment variables.
 * This function shall be called only if no configuration object is found in IMM.
 * The lgsConf struct contains default values but shall be updated
 * according to environment variables if there are any. See file logd.conf
 * If an environment variable is faulty the corresponding value will be set to
 * it's default value and a corresponding error flag will be set.
 * 
 * @param *lgsConf
 * 
 */
static void read_logsv_config_environ_var(lgs_conf_t *lgsConf) {
	char *val_str;
	unsigned long int val_uint;

	TRACE_ENTER2("Configured using default values and environment variables");

	/* logRootDirectory */
	if ((val_str = getenv("LOGSV_ROOT_DIRECTORY")) != NULL) {
		strncpy((char *) lgsConf->logRootDirectory, val_str, PATH_MAX);
		lgsConf->logRootDirectory_noteflag = false;
	} else {
		TRACE("LOGSV_ROOT_DIRECTORY not found");
		lgsConf->logRootDirectory_noteflag = true;
	}
	TRACE("logRootDirectory=%s, logRootDirectory_noteflag=%u",
			lgsConf->logRootDirectory, lgsConf->logRootDirectory_noteflag);

	/* logMaxLogrecsize */
	if ((val_str = getenv("LOGSV_MAX_LOGRECSIZE")) != NULL) {
/* errno = 0 is necessary as per the manpage of strtoul. Quoting here:
 * NOTES:
 * Since strtoul() can legitimately return 0 or ULONG_MAX (ULLONG_MAX for strtoull())
 * on both success and failure, the calling program should set errno to 0 before the call,
 * and then determine if an error occurred by  checking  whether  errno  has  a
 * nonzero value after the call.
 */
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			LOG_ER("Illegal value for LOGSV_MAX_LOGRECSIZE - %s, default %u",
					strerror(errno), lgsConf->logMaxLogrecsize);
			lgsConf->logMaxLogrecsize_noteflag = true;
		} else {
			lgsConf->logMaxLogrecsize = (SaUint32T) val_uint;
			lgsConf->logMaxLogrecsize_noteflag = false;
		}
	} else { /* No environment variable use default value */
		lgsConf->logMaxLogrecsize_noteflag = false;
	}
	TRACE("logMaxLogrecsize=%u, logMaxLogrecsize_noteflag=%u",
			lgsConf->logMaxLogrecsize, lgsConf->logMaxLogrecsize_noteflag);

	/* logStreamSystemHighLimit */
	if ((val_str = getenv("LOG_STREAM_SYSTEM_HIGH_LIMIT")) != NULL) {
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			LOG_ER("Illegal value for LOG_STREAM_SYSTEM_HIGH_LIMIT - %s, default %u",
					strerror(errno), lgsConf->logStreamSystemHighLimit);
			lgsConf->logStreamSystemHighLimit_noteflag = true;
		} else {
			lgsConf->logStreamSystemHighLimit = (SaUint32T) val_uint;
			lgsConf->logStreamSystemHighLimit_noteflag = false;
		}
	} else { /* No environment variable use default value */
		lgsConf->logStreamSystemHighLimit_noteflag = false;
	}
	TRACE("logStreamSystemHighLimit=%u, logStreamSystemHighLimit_noteflag=%u",
			lgsConf->logStreamSystemHighLimit, lgsConf->logStreamSystemHighLimit_noteflag);

	/* logStreamSystemLowLimit */
	if ((val_str = getenv("LOG_STREAM_SYSTEM_LOW_LIMIT")) != NULL) {
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			LOG_ER("Illegal value for LOG_STREAM_SYSTEM_LOW_LIMIT - %s, default %u",
					strerror(errno), lgsConf->logStreamSystemLowLimit);
			lgsConf->logStreamSystemLowLimit_noteflag = true;
		} else {
			lgsConf->logStreamSystemLowLimit = (SaUint32T) val_uint;
			lgsConf->logStreamSystemLowLimit_noteflag = false;
		}
	} else { /* No environment variable use default value */
		lgsConf->logStreamSystemLowLimit_noteflag = false;
	}
	TRACE("logStreamSystemLowLimit=%u, logStreamSystemLowLimit_noteflag=%u",
			lgsConf->logStreamSystemLowLimit, lgsConf->logStreamSystemLowLimit_noteflag);

	/* logStreamAppHighLimit */
	if ((val_str = getenv("LOG_STREAM_APP_HIGH_LIMIT")) != NULL) {
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			LOG_ER("Illegal value for LOG_STREAM_APP_HIGH_LIMIT - %s, default %u",
					strerror(errno), lgsConf->logStreamAppHighLimit);
			lgsConf->logStreamAppHighLimit_noteflag = true;
		} else {
			lgsConf->logStreamAppHighLimit = (SaUint32T) val_uint;
			lgsConf->logStreamAppHighLimit_noteflag = false;
		}
	} else { /* No environment variable use default value */
		lgsConf->logStreamAppHighLimit_noteflag = false;
	}
	TRACE("logStreamAppHighLimit=%u, logStreamAppHighLimit_noteflag=%u",
			lgsConf->logStreamAppHighLimit, lgsConf->logStreamAppHighLimit_noteflag);

	/* logStreamAppLowLimit */
	if ((val_str = getenv("LOG_STREAM_APP_LOW_LIMIT")) != NULL) {
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			LOG_ER("Illegal value for LOG_STREAM_APP_LOW_LIMIT - %s, default %u",
					strerror(errno), lgsConf->logStreamAppLowLimit);
			lgsConf->logStreamAppLowLimit_noteflag = true;
		} else {
			lgsConf->logStreamAppLowLimit = (SaUint32T) val_uint;
			lgsConf->logStreamAppLowLimit_noteflag = false;
		}
	} else { /* No environment variable use default value */
		lgsConf->logStreamAppLowLimit_noteflag = false;
	}
	TRACE("logStreamAppLowLimit=%u, logStreamAppLowLimit_noteflag=%u",
			lgsConf->logStreamAppLowLimit, lgsConf->logStreamAppLowLimit_noteflag);

	/* logMaxApplicationStreams */
	if ((val_str = getenv("LOG_MAX_APPLICATION_STREAMS")) != NULL) {
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			LOG_ER("Illegal value for LOG_MAX_APPLICATION_STREAMS - %s, default %u",
					strerror(errno), lgsConf->logMaxApplicationStreams);
			lgsConf->logMaxApplicationStreams_noteflag = true;
		} else {
			lgsConf->logMaxApplicationStreams = (SaUint32T) val_uint;
			lgsConf->logMaxApplicationStreams_noteflag = false;
		}
	} else { /* No environment variable use default value */
		lgsConf->logMaxApplicationStreams_noteflag = false;
	}
	TRACE("logMaxApplicationStreams=%u, logMaxApplicationStreams_noteflag=%u",
			lgsConf->logMaxApplicationStreams, lgsConf->logMaxApplicationStreams_noteflag);

	TRACE_LEAVE();
}

/* The function is called when the LOG config object exists,
 * to determine if envrionment variables also are configured.
 * If environment variables are also found, then the function
 * logs a warning message to convey that the environment variables
 * are ignored when the log config object is also configured.
 @ param none
 @ return none
 */

static void check_environs_for_configattribs(lgs_conf_t *lgsConf)
{
	char *val_str;
	unsigned long int val_uint;
	
	/* If environment variables are configured then, print a warning
	 * message to syslog.
	 */
	if (getenv("LOGSV_MAX_LOGRECSIZE") != NULL) {
		LOG_WA("Log Configuration object '%s' exists", LGS_IMM_LOG_CONFIGURATION); 
		LOG_WA("Ignoring environment variable LOGSV_MAX_LOGRECSIZE");
	}

	/* Environment variables for limits are used if limits
     * in configuration object is 0.
     */
	if ((val_str = getenv("LOG_STREAM_SYSTEM_HIGH_LIMIT")) != NULL) {
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			LOG_WA("Ignoring environment variable LOG_STREAM_SYSTEM_HIGH_LIMIT");
			LOG_WA("Illegal value");
		} else if ((lgsConf->logStreamSystemHighLimit == 0) &&
				(lgsConf->logStreamSystemLowLimit < val_uint)) {
			lgsConf->logStreamSystemHighLimit = val_uint;
		} else {
			LOG_WA("Log Configuration object '%s' exists", LGS_IMM_LOG_CONFIGURATION); 
			LOG_WA("Ignoring environment variable LOG_STREAM_SYSTEM_HIGH_LIMIT");
		}
	}	

	if ((val_str = getenv("LOG_STREAM_SYSTEM_LOW_LIMIT")) != NULL) {
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			LOG_WA("Ignoring environment variable LOG_STREAM_SYSTEM_LOW_LIMIT");
			LOG_WA("Illegal value");
		} else if ((lgsConf->logStreamSystemLowLimit == 0) &&
				(lgsConf->logStreamSystemHighLimit > val_uint)) {
				lgsConf->logStreamSystemLowLimit = val_uint;
		} else {
			LOG_WA("Log Configuration object '%s' exists", LGS_IMM_LOG_CONFIGURATION); 
			LOG_WA("Ignoring environment variable LOG_STREAM_SYSTEM_LOW_LIMIT");
		}
	}
	
	if ((val_str = getenv("LOG_STREAM_APP_HIGH_LIMIT")) != NULL) {
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			LOG_WA("Ignoring environment variable LOG_STREAM_APP_HIGH_LIMIT");
			LOG_WA("Illegal value");
		} else if ((lgsConf->logStreamAppHighLimit == 0) &&
				(lgsConf->logStreamAppLowLimit < val_uint)) {
			lgsConf->logStreamAppHighLimit = val_uint;
		} else {
			LOG_WA("Log Configuration object '%s' exists", LGS_IMM_LOG_CONFIGURATION); 
			LOG_WA("Ignoring environment variable LOG_STREAM_APP_HIGH_LIMIT");
		}
	}
	
	if ((val_str = getenv("LOG_STREAM_APP_LOW_LIMIT")) != NULL) {
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			LOG_WA("Ignoring environment variable LOG_STREAM_APP_LOW_LIMIT");
			LOG_WA("Illegal value");
		} else if ((lgsConf->logStreamAppLowLimit == 0) &&
				(lgsConf->logStreamAppHighLimit > val_uint)) {
			lgsConf->logStreamAppLowLimit = val_uint;
		} else {
			LOG_WA("Log Configuration object '%s' exists", LGS_IMM_LOG_CONFIGURATION); 
			LOG_WA("Ignoring environment variable LOG_STREAM_APP_LOW_LIMIT");
		}
	}

	if (getenv("LOG_MAX_APPLICATION_STREAMS") != NULL) {
		LOG_WA("Log Configuration object '%s' exists", LGS_IMM_LOG_CONFIGURATION); 
		LOG_WA("Ignoring environment variable LOG_MAX_APPLICATION_STREAMS");
	}
}

/**
 * Get log service configuration parameter. See SaLogConfig class.
 * The configuration will be read from IMM. If no config object exits
 * a configuration from environment variables and default will be used.
 * 
 * @param lgs_logconfGet_t param
 * Defines what configuration parameter to return
 * 
 * @param bool* noteflag
 * Is set to true if no valid configuration object exists and an invalid
 * environment variable is defined. In this case the parameter value returned
 * is the default value. 
 * It is valid to set this parameter to NULL if no error information is needed.
 * NOTE: A parameter is considered to be invalid if the corresponding environment
 *       variable cannot be converted to a value or if a mandatory environment
 *       variable is missing. This function does not check if the value is within
 *       allowed limits.
 * 
 * @return void *
 * Returns a pointer to the parameter. See struct lgs_conf
 *  
 */
const void *lgs_imm_logconf_get(lgs_logconfGet_t param, bool *noteflag)
{
	lgs_conf_t *lgs_conf_p;

	lgs_conf_p = (lgs_conf_t *) lgs_conf;
	/* Check if parameters has to be fetched from IMM */
	if (lgs_conf->logInitiated != true) {
		if (read_logsv_config_obj(LGS_IMM_LOG_CONFIGURATION, lgs_conf_p) != SA_AIS_OK) {
			LOG_NO("No or invalid log service configuration object");
			read_logsv_config_environ_var(lgs_conf_p);
		} else {
			/* LGS_IMM_LOG_CONFIGURATION object exists.
			 * If environment variables exists, then ignore them
			 * and log a message to syslog.
			 * For mailbox limits environment variables are used if the
			 * value in configuration object is 0
			 */
			check_environs_for_configattribs(lgs_conf_p);
		}
		
		/* Write configuration to syslog */
		LOG_NO("Log config system: high %d low %d, application: high %d low %d",
				lgs_conf->logStreamSystemHighLimit,
				lgs_conf->logStreamSystemLowLimit,
				lgs_conf->logStreamAppHighLimit,
				lgs_conf->logStreamAppLowLimit);
		
		lgs_conf_p->logInitiated = true;
	}

	switch (param) {
	case LGS_IMM_LOG_ROOT_DIRECTORY:
		if (noteflag != NULL) {
			*noteflag = lgs_conf->logRootDirectory_noteflag;
		}
		return (char *) lgs_conf->logRootDirectory;
	case LGS_IMM_LOG_MAX_LOGRECSIZE:
		if (noteflag != NULL) {
			*noteflag = lgs_conf->logMaxLogrecsize_noteflag;
		}
		return (SaUint32T *) &lgs_conf->logMaxLogrecsize;
	case LGS_IMM_LOG_STREAM_SYSTEM_HIGH_LIMIT:
		if (noteflag != NULL) {
			*noteflag = lgs_conf->logStreamSystemHighLimit_noteflag;
		}
		return (SaUint32T *) &lgs_conf->logStreamSystemHighLimit;
	case LGS_IMM_LOG_STREAM_SYSTEM_LOW_LIMIT:
		if (noteflag != NULL) {
			*noteflag = lgs_conf->logStreamSystemLowLimit_noteflag;
		}
		return (SaUint32T *) &lgs_conf->logStreamSystemLowLimit;
	case LGS_IMM_LOG_STREAM_APP_HIGH_LIMIT:
		if (noteflag != NULL) {
			*noteflag = lgs_conf->logStreamAppHighLimit_noteflag;
		}
		return (SaUint32T *) &lgs_conf->logStreamAppHighLimit;
	case LGS_IMM_LOG_STREAM_APP_LOW_LIMIT:
		if (noteflag != NULL) {
			*noteflag = lgs_conf->logStreamAppLowLimit_noteflag;
		}
		return (SaUint32T *) &lgs_conf->logStreamAppLowLimit;
	case LGS_IMM_LOG_MAX_APPLICATION_STREAMS:
		if (noteflag != NULL) {
			*noteflag = lgs_conf->logMaxApplicationStreams_noteflag;
		}
		return (SaUint32T *) &lgs_conf->logMaxApplicationStreams;
	case LGS_IMM_LOG_OPENSAFLOGCONFIG_CLASS_EXIST:
		if (noteflag != NULL) {
			*noteflag = false;
		}
		return (bool *) &lgs_conf->OpenSafLogConfig_class_exist;

	case LGS_IMM_LOG_NUMBER_OF_PARAMS:
	case LGS_IMM_LOG_NUMEND:
	default:
		LOG_ER("Invalid parameter %u",param);
		osafassert(0); /* Should never happen */
		break;
	}
	return NULL; /* Dummy */
}

static const SaImmOiCallbacksT_2 callbacks = {
	.saImmOiAdminOperationCallback = adminOperationCallback,
	.saImmOiCcbAbortCallback = ccbAbortCallback,
	.saImmOiCcbApplyCallback = ccbApplyCallback,
	.saImmOiCcbCompletedCallback = ccbCompletedCallback,
	.saImmOiCcbObjectCreateCallback = ccbObjectCreateCallback,
	.saImmOiCcbObjectDeleteCallback = ccbObjectDeleteCallback,
	.saImmOiCcbObjectModifyCallback = ccbObjectModifyCallback,
	.saImmOiRtAttrUpdateCallback = rtAttrUpdateCallback
};

/**
 * Give up applier role.
 */
void lgs_giveup_imm_applier(lgs_cb_t *cb)
{
	TRACE_ENTER();

	SaAisErrorT rc = SA_AIS_OK;
	if (we_are_applier_flag == true) {
		immutilWrapperProfile.nTries = 250;
		immutilWrapperProfile.errorsAreFatal = 0;

		if ((rc = immutil_saImmOiImplementerClear(cb->immOiHandle)) != SA_AIS_OK) {
			LOG_ER("immutil_saImmOiImplementerClear failed %d\n", rc);
			goto done;
		}

		we_are_applier_flag = false;
		TRACE("Applier cleared");
	} else {
		TRACE("We are not an applier");
	}
	
done:
	immutilWrapperProfile.nTries = 20; /* Reset retry time to more normal value. */
	immutilWrapperProfile.errorsAreFatal = 1;
	TRACE_LEAVE();
}

/**
 * Become object and class Applier
 */
SaAisErrorT lgs_become_imm_applier(lgs_cb_t *cb)
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;

	/* Become an applier only if an OpenSafLogConfig object exists */
	if ( false == *(bool*) lgs_imm_logconf_get(LGS_IMM_LOG_OPENSAFLOGCONFIG_CLASS_EXIST, NULL)) {
		TRACE_LEAVE2("Cannot be applier. OpenSafLogConfig object does not exist");
		return rc;
	}

	if (false == we_are_applier_flag) {
		immutilWrapperProfile.nTries = 250; /* LOG will be blocked until IMM responds */
		immutilWrapperProfile.errorsAreFatal = 0;
		if ((rc = immutil_saImmOiImplementerSet(cb->immOiHandle, applierName)) !=
				SA_AIS_OK) {
			LOG_ER("immutil_saImmOiImplementerSet(applierName) failed %d\n", rc);
			goto done;
		}
		if ((rc = immutil_saImmOiClassImplementerSet(cb->immOiHandle, "OpenSafLogConfig")) != SA_AIS_OK) {
			LOG_ER("immutil_saImmOiClassImplementerSet(OpenSafLogConfig) failed %d\n", rc);
			goto done;
		}
		we_are_applier_flag = true;
	}

done:
	immutilWrapperProfile.nTries = 20; /* Reset retry time to more normal value. */
	immutilWrapperProfile.errorsAreFatal = 1;
	TRACE_LEAVE();
	return rc;
}

/**
 * Get all dynamically added configurable application streams.
 * @param configNames
 * @param noConfObjects
 *
 * @return -
 */
static void getConfigNames(char configNames[64][128], int *noConfObjects)
{
	TRACE_ENTER();

	SaAisErrorT rc = SA_AIS_OK;
	SaImmHandleT omHandle;
	SaVersionT immVersion = {'A', 2, 1};
	SaImmSearchHandleT immSearchHandle;
	SaImmSearchParametersT_2 objectSearch;
	SaImmAttrValuesT_2 **attributes;

	(void) immutil_saImmOmInitialize(&omHandle, NULL, &immVersion);

	/* Search for all objects of class "SaLogStreamConfig" */
	objectSearch.searchOneAttr.attrName = "safLgStrCfg";
	objectSearch.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	objectSearch.searchOneAttr.attrValue = NULL;

	if ((rc = immutil_saImmOmSearchInitialize_2(omHandle, NULL,
			SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR,
			&objectSearch, NULL, /* Get no attributes */
			&immSearchHandle)) == SA_AIS_OK) {

		SaNameT objectName;
		*noConfObjects = 0;
		while (immutil_saImmOmSearchNext_2(immSearchHandle, &objectName, &attributes) == SA_AIS_OK) {
			if (strcmp((char*) objectName.value, SA_LOG_STREAM_ALARM) &&
					strcmp((char*) objectName.value, SA_LOG_STREAM_NOTIFICATION) &&
					strcmp((char*) objectName.value, SA_LOG_STREAM_SYSTEM)) {
				strcpy(configNames[*noConfObjects], (char*) objectName.value);
				*noConfObjects += 1;
			}
		}
	}
	else {
		LOG_IN("immutil_saImmOmSearchInitialize_2 %d", rc);
	}
	(void) immutil_saImmOmSearchFinalize(immSearchHandle);
	(void) immutil_saImmOmFinalize(omHandle);

	TRACE_LEAVE();
}

/**
 * Retrieve the LOG stream configuration from IMM using the
 * IMM-OM interface and initialize the corresponding information
 * in the LOG control block. Initialize the LOG IMM-OI
 * interface. Become class implementer.
 */
SaAisErrorT lgs_imm_activate(lgs_cb_t *cb)
{
	SaAisErrorT rc = SA_AIS_OK;
	log_stream_t *stream;

	TRACE_ENTER();
    
	if ((rc = stream_create_and_configure(SA_LOG_STREAM_ALARM, &cb->alarmStream, 0)) != SA_AIS_OK)
		goto done;

	if ((rc = stream_create_and_configure(SA_LOG_STREAM_NOTIFICATION, &cb->notificationStream, 1)) != SA_AIS_OK)
		goto done;

	if ((rc = stream_create_and_configure(SA_LOG_STREAM_SYSTEM, &cb->systemStream, 2)) != SA_AIS_OK)
		goto done;

	// Retrieve other configured streams
	int noConfObjects = 0;
	char configNames[64][128];
	getConfigNames(configNames, &noConfObjects);

	int i = 0;
	int streamId = 3;
	for (i = 0; i < noConfObjects; i++, streamId++) {
		if ((rc = stream_create_and_configure(configNames[i], &stream, streamId)) != SA_AIS_OK) {
			LOG_ER("stream_create_and_configure failed %d", rc);
		}
	}

	immutilWrapperProfile.nTries = 250; /* After loading,allow missed sync of large data to complete */

	(void)immutil_saImmOiImplementerSet(cb->immOiHandle, implementerName);
	(void)immutil_saImmOiClassImplementerSet(cb->immOiHandle, "SaLogStreamConfig");

	/* Do this only if the class exists */
	if ( true == *(bool*) lgs_imm_logconf_get(LGS_IMM_LOG_OPENSAFLOGCONFIG_CLASS_EXIST, NULL)) {
		(void)immutil_saImmOiClassImplementerSet(cb->immOiHandle, "OpenSafLogConfig");
	}

	immutilWrapperProfile.nTries = 20; /* Reset retry time to more normal value. */


	/* Update creation timestamp, must be object implementer first */
	(void)immutil_update_one_rattr(cb->immOiHandle, SA_LOG_STREAM_ALARM,
				       "saLogStreamCreationTimestamp", SA_IMM_ATTR_SATIMET,
				       &cb->alarmStream->creationTimeStamp);
	(void)immutil_update_one_rattr(cb->immOiHandle, SA_LOG_STREAM_NOTIFICATION,
				       "saLogStreamCreationTimestamp", SA_IMM_ATTR_SATIMET,
				       &cb->notificationStream->creationTimeStamp);
	(void)immutil_update_one_rattr(cb->immOiHandle, SA_LOG_STREAM_SYSTEM,
				       "saLogStreamCreationTimestamp", SA_IMM_ATTR_SATIMET,
				       &cb->systemStream->creationTimeStamp);

	/* Open all streams */
	stream = log_stream_getnext_by_name(NULL);
	while (stream != NULL) {
		if (log_stream_open(stream) != SA_AIS_OK)
			goto done;

		stream = log_stream_getnext_by_name(stream->name);
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Become object and class implementer/applier. Wait max
 * 'max_waiting_time_ms'.
 * @param _cb
 * 
 * @return void*
 */
static void *imm_impl_set(void *_cb)
{
	SaAisErrorT rc;
	int msecs_waited;
	lgs_cb_t *cb = (lgs_cb_t *)_cb;
	SaImmOiImplementerNameT iname;

	TRACE_ENTER();

	/* Become object implementer or applier dependent on if standby or active
	 * state
	 */
	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		iname = implementerName;
	} else {
		iname = applierName;
	}

	msecs_waited = 0;
	rc = saImmOiImplementerSet(cb->immOiHandle, iname);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiImplementerSet(cb->immOiHandle, iname);
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet failed %u", rc);
		exit(EXIT_FAILURE);
	}

	/* Become class implementer/applier for the OpenSafLogConfig class if it
	 * exists
	 */
	if ( true == *(bool*) lgs_imm_logconf_get(LGS_IMM_LOG_OPENSAFLOGCONFIG_CLASS_EXIST, NULL)) {
		(void)immutil_saImmOiClassImplementerSet(cb->immOiHandle, "OpenSafLogConfig");
	}

	/* Become class implementer for the SaLogStreamConfig class only if we are
	 * active.
	 */
	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		msecs_waited = 0;
		rc = saImmOiClassImplementerSet(cb->immOiHandle, "SaLogStreamConfig");
		while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;
			rc = saImmOiClassImplementerSet(cb->immOiHandle, "SaLogStreamConfig");
		}
	}
	
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiClassImplementerSet failed %u", rc);
		exit(EXIT_FAILURE);
	}

	TRACE_LEAVE();
	return NULL;
}

/**
 * Become object and class implementer or applier, non-blocking.
 * @param cb
 */
void lgs_imm_impl_set(lgs_cb_t *cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	/* In standby state: Become applier for configuration object if it exists.
	 * In active state: Become object implementer.
	 */
	if (cb->ha_state == SA_AMF_HA_STANDBY) {
		if (!*(bool*) lgs_imm_logconf_get(LGS_IMM_LOG_OPENSAFLOGCONFIG_CLASS_EXIST, NULL)) {
			return;
		}
	}

	TRACE_ENTER();
	if (pthread_create(&thread, &attr, imm_impl_set, cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	pthread_attr_destroy(&attr);
	
	TRACE_LEAVE();
}

/**
 * Initialize the OI interface and get a selection object. Wait
 * max 'max_waiting_time_ms'.
 * @param cb
 * 
 * @return SaAisErrorT
 */
SaAisErrorT lgs_imm_init(lgs_cb_t *cb)
{
	SaAisErrorT rc;
	int msecs_waited;

	TRACE_ENTER();

	msecs_waited = 0;
	rc = saImmOiInitialize_2(&cb->immOiHandle, &callbacks, &immVersion);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiInitialize_2(&cb->immOiHandle, &callbacks, &immVersion);
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiInitialize_2 failed %u", rc);
		return rc;
	}

	msecs_waited = 0;
	rc = saImmOiSelectionObjectGet(cb->immOiHandle, &cb->immSelectionObject);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiSelectionObjectGet(cb->immOiHandle, &cb->immSelectionObject);
	}

	if (rc != SA_AIS_OK)
		LOG_ER("saImmOiSelectionObjectGet failed %u", rc);

	TRACE_LEAVE();

	return rc;
}
