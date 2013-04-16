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

	if (lgs_relative_path_check(pathname) || stat(pathname, &pathstat) != 0) {
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

/**
 * Verify that attribute values are reasonable.
 * @param ccbUtilOperationData
 *
 * @return SaAisErrorT
 */
static SaAisErrorT check_attr_validity(const struct CcbUtilOperationData *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	void *value;
	const SaImmAttrValuesT_2 *attribute;
	log_stream_t *stream = (opdata->operationType == CCBUTIL_CREATE) ? NULL
			: log_stream_get_by_name((char *) opdata->param.modify.objectName->value);

	TRACE_ENTER();

	int i = 0;
	while (rc == SA_AIS_OK) {
		if (opdata->operationType == CCBUTIL_CREATE) {
			attribute = opdata->param.create.attrValues[i];
			value = (attribute != NULL && attribute->attrValuesNumber > 0) ?
					attribute->attrValues[0] : NULL;
		} else {
			// CCBUTIL_MODIFY
			attribute = (opdata->param.modify.attrMods[i] != NULL) ?
					&opdata->param.modify.attrMods[i]->modAttr : NULL;
			value = (attribute != NULL && attribute->attrValuesNumber > 0) ?
					attribute->attrValues[0] : NULL;
		}

		if (attribute != NULL && value != NULL) {
			TRACE("attribute %s", attribute->attrName);

			if (!strcmp(attribute->attrName, "saLogStreamFileName")) {
				struct stat pathstat;
				char *fileName = *((char **) value);
				if (stat(fileName, &pathstat) == 0) {

					LOG_ER("File %s already exist", fileName);
					rc = SA_AIS_ERR_EXIST;
				}
				TRACE("fileName: %s", fileName);
			} else if (!strcmp(attribute->attrName, "saLogStreamPathName")) {
				struct stat pathstat;
				char fileName[PATH_MAX];
				strcpy(fileName, lgs_cb->logsv_root_dir);
				strcat(fileName, "//");
				strcat(fileName, *((char **) value));
				strcat(fileName, "//.");
				if (lgs_relative_path_check(fileName)) {
					LOG_ER("Path %s not valid", fileName);
					rc = SA_AIS_ERR_INVALID_PARAM;
				} else if (stat(lgs_cb->logsv_root_dir, &pathstat) != 0) {
					LOG_ER("Path %s does not exist", fileName);
					rc = SA_AIS_ERR_BAD_OPERATION;
				}
				TRACE("fileName: %s", fileName);
			} else if (!strcmp(attribute->attrName, "saLogStreamMaxLogFileSize")) {
				SaUint64T maxLogFileSize = *((SaUint64T *) value);
				// maxLogFileSize == 0 is interpreted as "infinite" size.
				if (maxLogFileSize > 0 &&
						stream != NULL &&
						maxLogFileSize < stream->fixedLogRecordSize) {
					LOG_ER("maxLogFileSize out of range");
					rc = SA_AIS_ERR_BAD_OPERATION;
				}
				TRACE("maxLogFileSize: %llu", maxLogFileSize);
			} else if (!strcmp(attribute->attrName, "saLogStreamFixedLogRecordSize")) {
				SaUint32T fixedLogRecordSize = *((SaUint32T *) value);
				if (stream != NULL &&
						stream->maxLogFileSize > 0 &&
						fixedLogRecordSize > stream->maxLogFileSize) {
					LOG_ER("fixedLogRecordSize out of range");
					rc = SA_AIS_ERR_BAD_OPERATION;
				}
				TRACE("fixedLogRecordSize: %u", fixedLogRecordSize);
			} else if (!strcmp(attribute->attrName, "saLogStreamLogFullAction")) {
				SaLogFileFullActionT logFullAction = *((SaUint32T *) value);
				if ((logFullAction < SA_LOG_FILE_FULL_ACTION_WRAP) ||
						(logFullAction > SA_LOG_FILE_FULL_ACTION_ROTATE)) {
					LOG_ER("logFullAction out of range");
					rc = SA_AIS_ERR_BAD_OPERATION;
				}
				if ((logFullAction == SA_LOG_FILE_FULL_ACTION_WRAP) ||
						(logFullAction == SA_LOG_FILE_FULL_ACTION_HALT)) {
					LOG_ER("logFullAction:Current Implementation doesn't support  Wrap and halt");
					rc = SA_AIS_ERR_NOT_SUPPORTED;
				}
				TRACE("logFullAction: %u", logFullAction);
			} else if (!strcmp(attribute->attrName, "saLogStreamLogFullHaltThreshold")) {
				SaUint32T logFullHaltThreshold = *((SaUint32T *) value);
				if (logFullHaltThreshold >= 100) {
					LOG_ER("logFullHaltThreshold out of range");
					rc = SA_AIS_ERR_BAD_OPERATION;
				}
				TRACE("logFullHaltThreshold: %u", logFullHaltThreshold);
			} else if (!strcmp(attribute->attrName, "saLogStreamMaxFilesRotated")) {
				SaUint32T maxFilesRotated = *((SaUint32T *) value);
				if (maxFilesRotated < 1 || maxFilesRotated > 128) {
					LOG_ER("Unreasonable maxFilesRotated: %x", maxFilesRotated);
					rc = SA_AIS_ERR_BAD_OPERATION;
				}
				TRACE("maxFilesRotated: %u", maxFilesRotated);
			} else if (!strcmp(attribute->attrName, "saLogStreamLogFileFormat")) {
				SaBoolT dummy;
				char *logFileFormat = *((char **) value);
				TRACE("logFileFormat: %s", logFileFormat);

				if (opdata->operationType == CCBUTIL_CREATE) {
					if (!lgs_is_valid_format_expression(logFileFormat, STREAM_TYPE_APPLICATION, &dummy)) {
						LOG_ER("Invalid logFileFormat: %s", logFileFormat);
						rc = SA_AIS_ERR_BAD_OPERATION;
					}
				}
				else {
					if (!lgs_is_valid_format_expression(logFileFormat, stream->streamType, &dummy)) {
						LOG_ER("Invalid logFileFormat: %s", logFileFormat);
						rc = SA_AIS_ERR_BAD_OPERATION;
					}
				}
			} else if (!strcmp(attribute->attrName, "saLogStreamSeverityFilter")) {
				SaUint32T severityFilter = *((SaUint32T *) value);
				if (severityFilter > 0x7f) {
					LOG_ER("Invalid severity: %x", severityFilter);
					rc = SA_AIS_ERR_BAD_OPERATION;
				}
				TRACE("severityFilter: %u", severityFilter);
			} else if (!strncmp(attribute->attrName, "SaImm", 5) ||
					!strncmp(attribute->attrName, "safLg", 5)) {
				;
			} else {
				LOG_ER("invalid attribute %s", attribute->attrName);
				rc = SA_AIS_ERR_BAD_OPERATION;
			}
		} else {
			TRACE("i: %d, attribute: %d, value: %d", i, attribute == NULL ? 0 : 1
					, value == NULL ? 0 : 1);
			if (attribute == NULL) {
				break;
			}
		}
		i++;
	}
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT stream_ccb_completed_create(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);
	rc = check_attr_validity(opdata);
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static SaAisErrorT stream_ccb_completed_modify(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);
	rc = check_attr_validity(opdata);
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

static SaAisErrorT stream_ccb_completed(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		rc = stream_ccb_completed_create(opdata);
		break;
	case CCBUTIL_MODIFY:
		rc = stream_ccb_completed_modify(opdata);
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
				rc = stream_ccb_completed(opdata);
			else
				osafassert(0);
			break;
		case CCBUTIL_DELETE:
		case CCBUTIL_MODIFY:
			if (!strncmp((char*)opdata->objectName.value, "safLgStrCfg", 11))
				rc = stream_ccb_completed(opdata);
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

	TRACE_ENTER2("(%s)", dn);

	strncpy((char *)objectName.value, dn, SA_MAX_NAME_LENGTH);
	objectName.length = strlen((char *)objectName.value);

	*in_stream = stream = log_stream_new_2(&objectName, stream_id);

	if (stream == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Happens to be the same, ugly! FIX */
	stream->streamType = stream_id;

	(void)immutil_saImmOmInitialize(&omHandle, NULL, &immVersion);
	(void)immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle);

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
	(void)immutil_saImmOmAccessorFinalize(accessorHandle);
	(void)immutil_saImmOmFinalize(omHandle);

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

	TRACE_ENTER2("(%s)", dn);

	strncpy((char *) objectName.value, dn, SA_MAX_NAME_LENGTH);
	objectName.length = strlen((char *) objectName.value);

	/* NOTE: immutil init osaf_assert if error */
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
			TRACE("logRootDirectory: %s", lgsConf->logRootDirectory);
		} else if (!strcmp(attribute->attrName, "logMaxLogrecsize")) {
			lgsConf->logMaxLogrecsize = *((SaUint32T *) value);
			param_cnt++;
			TRACE("logMaxLogrecsize: %u", lgsConf->logMaxLogrecsize);
		} else if (!strcmp(attribute->attrName, "logStreamSystemHighLimit")) {
			lgsConf->logStreamSystemHighLimit = *((SaUint32T *) value);
			param_cnt++;
			TRACE("logStreamSystemHighLimit: %u", lgsConf->logStreamSystemHighLimit);
		} else if (!strcmp(attribute->attrName, "logStreamSystemLowLimit")) {
			lgsConf->logStreamSystemLowLimit = *((SaUint32T *) value);
			param_cnt++;
			TRACE("logStreamSystemLowLimit: %u", lgsConf->logStreamSystemLowLimit);
		} else if (!strcmp(attribute->attrName, "logStreamAppHighLimit")) {
			lgsConf->logStreamAppHighLimit = *((SaUint32T *) value);
			param_cnt++;
			TRACE("logStreamAppHighLimit: %u", lgsConf->logStreamAppHighLimit);
		} else if (!strcmp(attribute->attrName, "logStreamAppLowLimit")) {
			lgsConf->logStreamAppLowLimit = *((SaUint32T *) value);
			param_cnt++;
			TRACE("logStreamAppLowLimit: %u", lgsConf->logStreamAppLowLimit);
		} else if (!strcmp(attribute->attrName, "logMaxApplicationStreams")) {
			lgsConf->logMaxApplicationStreams = *((SaUint32T *) value);
			param_cnt++;
			TRACE("logMaxApplicationStreams: %u", lgsConf->logMaxApplicationStreams);
		}
	}

	/* Check if missing parameters */
	if (param_cnt != LGS_IMM_LOG_NUMBER_OF_PARAMS) {
		lgsConf->OpenSafLogConfig_class_exist = false;
		rc = SA_AIS_ERR_NOT_EXIST;
		LOG_ER("read_logsv_configuration(), Parameter error. All parameters could not be read");
	}

done:
	(void) immutil_saImmOmAccessorFinalize(accessorHandle);
	(void) immutil_saImmOmFinalize(omHandle);

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
			read_logsv_config_environ_var( lgs_conf_p);
		}
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
