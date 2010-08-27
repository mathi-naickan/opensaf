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
#include <assert.h>
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

const unsigned int sleep_delay_ms = 500;
const unsigned int max_waiting_time_ms = 60 * 1000;	/* 60 secs */

/* Must be able to index this array using streamType */
static char *log_file_format[] = {
	DEFAULT_ALM_NOT_FORMAT_EXP,
	DEFAULT_ALM_NOT_FORMAT_EXP,
	DEFAULT_APP_SYS_FORMAT_EXP
};

static SaVersionT immVersion = { 'A', 2, 1 };

static const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)"safLogService";

/**
 * Pack and send a stream checkpoint using mbcsv
 * @param cb
 * @param logStream
 * 
 * @return uns32
 */
static uns32 lgs_ckpt_stream(log_stream_t *stream)
{
	lgsv_ckpt_msg_t ckpt;
	uns32 rc;

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
static void saImmOiAdminOperationCallback(SaImmOiHandleT immOiHandle,
					  SaInvocationT invocation,
					  const SaNameT *objectName,
					  SaImmAdminOperationIdT opId, const SaImmAdminOperationParamsT_2 **params)
{
	SaUint32T severityFilter;
	const SaImmAdminOperationParamsT_2 *param = params[0];
	log_stream_t *stream;

	TRACE_ENTER();

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
		lgs_ckpt_stream(stream);
	} else {
		LOG_ER("Invalid operation ID, should be %d (one) for change filter", SA_LOG_ADMIN_CHANGE_FILTER);
		(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
	}
 done:
	TRACE_LEAVE();
}

static SaAisErrorT saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle,
						  SaImmOiCcbIdT ccbId,
						  const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB objectfor %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	/* "memorize the modification request" */
	ccbutil_ccbAddModifyOperation(ccbUtilCcbData, objectName, attrMods);

 done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *ccbUtilOperationData;
	log_stream_t *stream;
	const SaImmAttrModificationT_2 *attrMod;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_ER("Failed to find CCB object for %llu", ccbId);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	/*
	 ** "check that the sequence of change requests contained in the CCB is 
	 ** valid and that no errors will be generated when these changes
	 ** are applied."
	 */
	ccbUtilOperationData = ccbUtilCcbData->operationListHead;
	while (ccbUtilOperationData != NULL) {
		int i = 0;
		TRACE("Change stream %s", ccbUtilOperationData->param.modify.objectName->value);

		if ((stream = log_stream_get_by_name((char *)ccbUtilOperationData->param.modify.objectName->value)) ==
		    NULL) {
			LOG_ER("Stream %s not found", ccbUtilOperationData->param.modify.objectName->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		attrMod = ccbUtilOperationData->param.modify.attrMods[i++];
		while (attrMod != NULL) {
			void *value;
			const SaImmAttrValuesT_2 *attribute = &attrMod->modAttr;

			TRACE("attribute %s", attribute->attrName);

			if (attribute->attrValuesNumber == 0) {
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}

			value = attribute->attrValues[0];

			if (!strcmp(attribute->attrName, "saLogStreamFileName")) {
				struct stat pathstat;
				char *fileName = *((char **)value);
				if (stat(fileName, &pathstat) == 0) {
					LOG_ER("File %s already exist", fileName);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
				TRACE("fileName: %s", fileName);
			} else if (!strcmp(attribute->attrName, "saLogStreamMaxLogFileSize")) {
				SaUint64T maxLogFileSize = *((SaUint64T *)value);
				TRACE("maxLogFileSize: %llu", maxLogFileSize);
			} else if (!strcmp(attribute->attrName, "saLogStreamFixedLogRecordSize")) {
				SaUint32T fixedLogRecordSize = *((SaUint32T *)value);
				if (fixedLogRecordSize > stream->maxLogFileSize) {
					LOG_ER("fixedLogRecordSize out of range");
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
				TRACE("fixedLogRecordSize: %u", stream->fixedLogRecordSize);
			} else if (!strcmp(attribute->attrName, "saLogStreamLogFullAction")) {
				SaLogFileFullActionT logFullAction = *((SaUint32T *)value);
				if ((logFullAction < SA_LOG_FILE_FULL_ACTION_WRAP) || 
						(logFullAction > SA_LOG_FILE_FULL_ACTION_ROTATE)){
					LOG_ER("logFullAction out of range");
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
				if ((logFullAction == SA_LOG_FILE_FULL_ACTION_WRAP) ||(logFullAction == SA_LOG_FILE_FULL_ACTION_HALT)){
					LOG_ER("logFullAction:Current Implementation doesn't support  Wrap and halt");
					rc = SA_AIS_ERR_NOT_SUPPORTED;
					goto done;
				}
				TRACE("logFullAction: %u", logFullAction);
			} else if (!strcmp(attribute->attrName, "saLogStreamLogFullHaltThreshold")) {
				SaUint32T logFullHaltThreshold = *((SaUint32T *)value);
				if (logFullHaltThreshold >= 100) {
					LOG_ER("logFullHaltThreshold out of range");
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
				TRACE("logFullHaltThreshold: %u", logFullHaltThreshold);
			} else if (!strcmp(attribute->attrName, "saLogStreamMaxFilesRotated")) {
				SaUint32T maxFilesRotated = *((SaUint32T *)value);
				if (maxFilesRotated > 128) {
					LOG_ER("Unreasonable maxFilesRotated: %x", maxFilesRotated);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
				TRACE("maxFilesRotated: %u", maxFilesRotated);
			} else if (!strcmp(attribute->attrName, "saLogStreamLogFileFormat")) {
				SaBoolT dummy;
				char *logFileFormat = *((char **)value);
				TRACE("logFileFormat: %s", logFileFormat);

				if (!lgs_is_valid_format_expression(logFileFormat, stream->streamType, &dummy)) {
					LOG_ER("Invalid logFileFormat: %s", logFileFormat);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			} else if (!strcmp(attribute->attrName, "saLogStreamSeverityFilter")) {

				if ((stream->streamType != STREAM_TYPE_ALARM)
                                    && (stream->streamType != STREAM_TYPE_NOTIFICATION)) {

					SaUint32T severityFilter = *((SaUint32T *)value);
					if (severityFilter > 0x7f) {
						LOG_ER("Invalid severity: %x", severityFilter);
						rc = SA_AIS_ERR_BAD_OPERATION;
						goto done;
					}
					TRACE("severityFilter: %u", severityFilter);
				} else {
					LOG_ER("severityFilter cannot be changed for Alarm amd Notification Stream");
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			} else {
				LOG_ER("invalid attribute");
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}

			attrMod = ccbUtilOperationData->param.modify.attrMods[i++];
		}
		ccbUtilOperationData = ccbUtilOperationData->next;
	}

 done:
	TRACE_LEAVE();
	return rc;
}

static void saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *ccbUtilOperationData;
	log_stream_t *stream = NULL;
	const SaImmAttrModificationT_2 *attrMod;
	int new_cfg_file_needed;
	char current_file_name[NAME_MAX];

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_ER("Failed to find CCB object for %llu", ccbId);
		goto done;
	}

	ccbUtilOperationData = ccbUtilCcbData->operationListHead;
	while (ccbUtilOperationData != NULL) {
		int i = 0;
		TRACE("Apply changes to stream %s", ccbUtilOperationData->param.modify.objectName->value);

		if ((stream = log_stream_get_by_name((char *)ccbUtilOperationData->param.modify.objectName->value)) ==
		    NULL) {
			LOG_ER("Stream %s not found", ccbUtilOperationData->param.modify.objectName->value);
			goto done;
		}

		strncpy(current_file_name, stream->fileName, sizeof(current_file_name));
		new_cfg_file_needed = 0;

		attrMod = ccbUtilOperationData->param.modify.attrMods[i++];
		while (attrMod != NULL) {
			void *value;
			const SaImmAttrValuesT_2 *attribute = &attrMod->modAttr;

			TRACE("attribute %s", attribute->attrName);

			value = attribute->attrValues[0];

			if (!strcmp(attribute->attrName, "saLogStreamFileName")) {
				char *fileName = *((char **)value);
				strcpy(stream->fileName, fileName);
				new_cfg_file_needed = 1;
			} else if (!strcmp(attribute->attrName, "saLogStreamMaxLogFileSize")) {
				SaUint64T maxLogFileSize = *((SaUint64T *)value);
				stream->maxLogFileSize = maxLogFileSize;
				new_cfg_file_needed = 1;
			} else if (!strcmp(attribute->attrName, "saLogStreamFixedLogRecordSize")) {
				SaUint32T fixedLogRecordSize = *((SaUint32T *)value);
				stream->fixedLogRecordSize = fixedLogRecordSize;
				new_cfg_file_needed = 1;
			} else if (!strcmp(attribute->attrName, "saLogStreamLogFullAction")) {
				SaLogFileFullActionT logFullAction = *((SaUint32T *)value);
				stream->logFullAction = logFullAction;
				new_cfg_file_needed = 1;
			} else if (!strcmp(attribute->attrName, "saLogStreamLogFullHaltThreshold")) {
				SaUint32T logFullHaltThreshold = *((SaUint32T *)value);
				stream->logFullHaltThreshold = logFullHaltThreshold;
			} else if (!strcmp(attribute->attrName, "saLogStreamMaxFilesRotated")) {
				SaUint32T maxFilesRotated = *((SaUint32T *)value);
				stream->maxFilesRotated = maxFilesRotated;
				new_cfg_file_needed = 1;
			} else if (!strcmp(attribute->attrName, "saLogStreamLogFileFormat")) {
				char *logFileFormat = *((char **)value);
				if (stream->logFileFormat != NULL)
					free(stream->logFileFormat);
				stream->logFileFormat = strdup(logFileFormat);
				new_cfg_file_needed = 1;
			} else if (!strcmp(attribute->attrName, "saLogStreamSeverityFilter")) {
				SaUint32T severityFilter = *((SaUint32T *)value);
				stream->severityFilter = severityFilter;
			} else {
				LOG_ER("invalid attribute");
				goto done;
			}

			attrMod = ccbUtilOperationData->param.modify.attrMods[i++];
		}

		if (new_cfg_file_needed) {
			if (log_stream_config_change(stream, current_file_name) != 0) {
				LOG_ER("log_stream_config_change failed");
				exit(1);
			}
		}

		/* Checkpoint to standby LOG server */
		lgs_ckpt_stream(stream);

		ccbUtilOperationData = ccbUtilOperationData->next;
	}

done:
	ccbutil_deleteCcbData(ccbUtilCcbData);
	TRACE_LEAVE();
}

static void saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) != NULL)
		ccbutil_deleteCcbData(ccbUtilCcbData);
	else
		LOG_ER("Failed to find CCB object for %llu", ccbId);

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
static SaAisErrorT saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
					       const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaImmAttrNameT attributeName;
	int i = 0;
	log_stream_t *stream = log_stream_get_by_name((char *)objectName->value);

	TRACE_ENTER();

	if (stream == NULL) {
		LOG_ER("saImmOiRtAttrUpdateCallback, stream %s not found", objectName->value);
		rc = SA_AIS_ERR_FAILED_OPERATION;	/* not really covered in spec */
		goto done;
	}

	while ((attributeName = attributeNames[i++]) != NULL) {
		TRACE("Attribute %s", attributeName);
		if (!strcmp(attributeName, "saLogStreamNumOpeners")) {
			(void)immutil_update_one_rattr(immOiHandle, (char *)objectName->value,
						       attributeName, SA_IMM_ATTR_SAUINT32T, &stream->numOpeners);
		} else {
			LOG_ER("saImmOiRtAttrUpdateCallback, unknown attribute %s", attributeName);
			rc = SA_AIS_ERR_FAILED_OPERATION;	/* not really covered in spec */
			goto done;
		}
	}

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

	(void)immutil_saImmOmAccessorFinalize(accessorHandle);
	(void)immutil_saImmOmFinalize(omHandle);

 done:
	TRACE_LEAVE();
	return rc;
}

static const SaImmOiCallbacksT_2 callbacks = {
	.saImmOiAdminOperationCallback = saImmOiAdminOperationCallback,
	.saImmOiCcbAbortCallback = saImmOiCcbAbortCallback,
	.saImmOiCcbApplyCallback = saImmOiCcbApplyCallback,
	.saImmOiCcbCompletedCallback = saImmOiCcbCompletedCallback,
	.saImmOiCcbObjectCreateCallback = NULL,
	.saImmOiCcbObjectDeleteCallback = NULL,
	.saImmOiCcbObjectModifyCallback = saImmOiCcbObjectModifyCallback,
	.saImmOiRtAttrUpdateCallback = saImmOiRtAttrUpdateCallback
};

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

	(void)immutil_saImmOiImplementerSet(cb->immOiHandle, implementerName);
	(void)immutil_saImmOiClassImplementerSet(cb->immOiHandle, "SaLogStreamConfig");

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
 * Become object and class implementer. Wait max
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

	TRACE_ENTER();

	msecs_waited = 0;
	rc = saImmOiImplementerSet(cb->immOiHandle, implementerName);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiImplementerSet(cb->immOiHandle, implementerName);
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet failed %u", rc);
		exit(EXIT_FAILURE);
	}

	msecs_waited = 0;
	rc = saImmOiClassImplementerSet(cb->immOiHandle, "SaLogStreamConfig");
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiClassImplementerSet(cb->immOiHandle, "SaLogStreamConfig");
	}

	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiClassImplementerSet failed %u", rc);
		exit(EXIT_FAILURE);
	}

	TRACE_LEAVE();
	return NULL;
}

/**
 * Become object and class implementer, non-blocking.
 * @param cb
 */
void lgs_imm_impl_set(lgs_cb_t *cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

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
