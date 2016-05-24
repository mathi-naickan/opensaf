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

#define _GNU_SOURCE
#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <utmp.h>

#include <saImmOm.h>
#include <saImmOi.h>

#include "osaf_secutil.h"
#include "immutil.h"
#include "osaf_time.h"
#include "lgs.h"
#include "lgs_util.h"
#include "lgs_file.h"
#include "lgs_config.h"
#include "saf_error.h"

#include "lgs_mbcsv_v1.h"
#include "lgs_mbcsv_v2.h"
#include "lgs_mbcsv_v3.h"
#include "lgs_mbcsv_v5.h"

/* TYPE DEFINITIONS
 * ----------------
 */

/* Used for protecting global imm OI handle and selection object during
 * initialize of OI
 */
pthread_mutex_t lgs_OI_init_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Used for checkpointing time when files are closed */
static time_t chkp_file_close_time = 0;


const unsigned int sleep_delay_ms = 500;
const unsigned int max_waiting_time_60s = 60 * 1000;	/* 60 secs */
const unsigned int max_waiting_time_10s = 10 * 1000;	/* 10 secs */

/* Must be able to index this array using streamType */
static char *log_file_format[] = {
	DEFAULT_ALM_NOT_FORMAT_EXP,
	DEFAULT_ALM_NOT_FORMAT_EXP,
	DEFAULT_APP_SYS_FORMAT_EXP,
	DEFAULT_APP_SYS_FORMAT_EXP
};

static SaVersionT immVersion = { 'A', 2, 11 };

static const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)"safLogService";

extern struct ImmutilWrapperProfile immutilWrapperProfile;

/* FUNCTIONS
 * ---------
 */

static void report_oi_error(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
		const char *format, ...) __attribute__ ((format(printf, 3, 4)));

static void report_om_error(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
		const char *format, ...) __attribute__ ((format(printf, 3, 4)));

/**
 * To be used in OI callbacks to report errors by setting an error string
 * Also writes the same error string using TRACE
 * 
 * @param immOiHandle
 * @param ccbId
 * @param format
 * @param ...
 */
static void report_oi_error(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
		const char *format, ...)
{
	char err_str[256];
	va_list ap;
	
	va_start(ap, format);
	(void) vsnprintf(err_str, 256, format, ap);
	va_end(ap);
	TRACE("%s", err_str);
	(void) saImmOiCcbSetErrorString(immOiHandle, ccbId,	err_str);	
}

static void report_om_error(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
		const char *format, ...)
{
	char ao_err_string[256];
	SaStringT p_ao_err_string = ao_err_string;
	SaImmAdminOperationParamsT_2 ao_err_param = {
	SA_IMM_PARAM_ADMOP_ERROR,
	SA_IMM_ATTR_SASTRINGT,
	&p_ao_err_string };
	const SaImmAdminOperationParamsT_2 *ao_err_params[2] = {
	&ao_err_param,
	NULL };
	
	va_list ap;
	va_start(ap, format);
	(void) vsnprintf(ao_err_string, 256, format, ap);
	va_end(ap);
	TRACE("%s",ao_err_string);
	(void) saImmOiAdminOperationResult_o2(immOiHandle, invocation,
		   SA_AIS_ERR_INVALID_PARAM,
		   ao_err_params);
}

/**
 * Pack and send a log service config checkpoint using mbcsv
 * @param stream
 * 
 * @return NCSCC_RC_... error code
 */
static uint32_t ckpt_lgs_cfg(bool is_root_dir_changed, lgs_config_chg_t *v5_ckpt)
{
	void *ckpt = NULL;
	lgsv_ckpt_msg_v2_t ckpt_v2;
	lgsv_ckpt_msg_v3_t ckpt_v3;
	lgsv_ckpt_msg_v5_t ckpt_v5;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	
	if (!lgs_is_peer_v2()) {
		/* Can be called only if we are OI. This is not the case if Standby */
		TRACE("%s Called when check-pointing version 1",__FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	if (lgs_is_peer_v5()) {
		/* Checkpoint version 5 */
		memset(&ckpt_v5, 0, sizeof(ckpt_v5));
		ckpt_v5.header.ckpt_rec_type = LGS_CKPT_LGS_CFG_V5;
		ckpt_v5.header.num_ckpt_records = 1;
		ckpt_v5.header.data_len = 1;
		ckpt_v5.ckpt_rec.lgs_cfg.c_file_close_time_stamp = chkp_file_close_time;
		ckpt_v5.ckpt_rec.lgs_cfg.buffer = v5_ckpt->ckpt_buffer_ptr;
		ckpt_v5.ckpt_rec.lgs_cfg.buffer_size = v5_ckpt->ckpt_buffer_size;
		ckpt = &ckpt_v5;
		TRACE("\tCheck-pointing v5");
	} else if (lgs_is_peer_v4()) {
		/* Checkpoint version 4 */
		memset(&ckpt_v3, 0, sizeof(ckpt_v3));
		ckpt_v3.header.ckpt_rec_type = LGS_CKPT_LGS_CFG_V3;
		ckpt_v3.header.num_ckpt_records = 1;
		ckpt_v3.header.data_len = 1;
		ckpt_v3.ckpt_rec.lgs_cfg.logRootDirectory = (char *)
			lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY);
		ckpt_v3.ckpt_rec.lgs_cfg.logDataGroupname = (char *)
			lgs_cfg_get(LGS_IMM_DATA_GROUPNAME);
		ckpt_v3.ckpt_rec.lgs_cfg.c_file_close_time_stamp = chkp_file_close_time;
		ckpt = &ckpt_v3;
		TRACE("\tCheck-pointing v3 peer is v4");
	} else {
		/* Checkpoint version 3 or 2 */
		if (is_root_dir_changed) {
			memset(&ckpt_v2, 0, sizeof(ckpt_v2));
			ckpt_v2.header.ckpt_rec_type = LGS_CKPT_LGS_CFG;
			ckpt_v2.header.num_ckpt_records = 1;
			ckpt_v2.header.data_len = 1;
			ckpt_v2.ckpt_rec.lgs_cfg.logRootDirectory = (char *)
				lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY);
			ckpt_v2.ckpt_rec.lgs_cfg.c_file_close_time_stamp = chkp_file_close_time;

			ckpt = &ckpt_v2;
		}
		TRACE("\tCheck-pointing v2 peer < v4");
	}

	if (ckpt) {
		rc = lgs_ckpt_send_async(lgs_cb, ckpt, NCS_MBCSV_ACT_ADD);
	}

	TRACE_LEAVE();
	return rc;
}

/**
 * Pack and send a stream checkpoint using mbcsv
 * @param stream
 * 
 * @return NCSCC_RC_... error code
 */
static uint32_t ckpt_stream_config(log_stream_t *stream)
{
	uint32_t rc;
	lgsv_ckpt_msg_v1_t ckpt_v1;
	lgsv_ckpt_msg_v2_t ckpt_v2;
	void *ckpt_ptr;

	TRACE_ENTER();

	if (lgs_is_peer_v2()) {
		memset(&ckpt_v2, 0, sizeof(ckpt_v2));
		ckpt_v2.header.ckpt_rec_type = LGS_CKPT_CFG_STREAM;
		ckpt_v2.header.num_ckpt_records = 1;
		ckpt_v2.header.data_len = 1;

		ckpt_v2.ckpt_rec.stream_cfg.name = (char *)stream->name;
		ckpt_v2.ckpt_rec.stream_cfg.fileName = stream->fileName;
		ckpt_v2.ckpt_rec.stream_cfg.pathName = stream->pathName;
		ckpt_v2.ckpt_rec.stream_cfg.maxLogFileSize = stream->maxLogFileSize;
		ckpt_v2.ckpt_rec.stream_cfg.fixedLogRecordSize = stream->fixedLogRecordSize;
		ckpt_v2.ckpt_rec.stream_cfg.logFullAction = stream->logFullAction;
		ckpt_v2.ckpt_rec.stream_cfg.logFullHaltThreshold = stream->logFullHaltThreshold;
		ckpt_v2.ckpt_rec.stream_cfg.maxFilesRotated = stream->maxFilesRotated;
		ckpt_v2.ckpt_rec.stream_cfg.logFileFormat = stream->logFileFormat;
		ckpt_v2.ckpt_rec.stream_cfg.severityFilter = stream->severityFilter;
		ckpt_v2.ckpt_rec.stream_cfg.logFileCurrent = stream->logFileCurrent;
		ckpt_v2.ckpt_rec.stream_cfg.c_file_close_time_stamp = stream->act_last_close_timestamp;
		ckpt_ptr = &ckpt_v2;
	} else {
		memset(&ckpt_v2, 0, sizeof(ckpt_v2));
		ckpt_v1.header.ckpt_rec_type = LGS_CKPT_CFG_STREAM;
		ckpt_v1.header.num_ckpt_records = 1;
		ckpt_v1.header.data_len = 1;

		ckpt_v1.ckpt_rec.stream_cfg.name = (char *)stream->name;
		ckpt_v1.ckpt_rec.stream_cfg.fileName = stream->fileName;
		ckpt_v1.ckpt_rec.stream_cfg.pathName = stream->pathName;
		ckpt_v1.ckpt_rec.stream_cfg.maxLogFileSize = stream->maxLogFileSize;
		ckpt_v1.ckpt_rec.stream_cfg.fixedLogRecordSize = stream->fixedLogRecordSize;
		ckpt_v1.ckpt_rec.stream_cfg.logFullAction = stream->logFullAction;
		ckpt_v1.ckpt_rec.stream_cfg.logFullHaltThreshold = stream->logFullHaltThreshold;
		ckpt_v1.ckpt_rec.stream_cfg.maxFilesRotated = stream->maxFilesRotated;
		ckpt_v1.ckpt_rec.stream_cfg.logFileFormat = stream->logFileFormat;
		ckpt_v1.ckpt_rec.stream_cfg.severityFilter = stream->severityFilter;
		ckpt_v1.ckpt_rec.stream_cfg.logFileCurrent = stream->logFileCurrent;
		ckpt_ptr = &ckpt_v1;
	}

	rc = lgs_ckpt_send_async(lgs_cb, ckpt_ptr, NCS_MBCSV_ACT_ADD);

	TRACE_LEAVE();
	return rc;
}

/**
 * Pack and send an open stream checkpoint using mbcsv
 * @param stream
 * @param recType
 *
 * @return uint32
 */
static uint32_t ckpt_stream_open(log_stream_t *stream)
{
	uint32_t rc;
	lgsv_ckpt_msg_v1_t ckpt_v1;
	lgsv_ckpt_msg_v2_t ckpt_v2;
	void *ckpt_ptr;
	lgs_ckpt_stream_open_t *stream_open_ptr;
	lgsv_ckpt_header_t *header_ptr;

	TRACE_ENTER();

	if (lgs_is_peer_v2()) {
		memset(&ckpt_v2, 0, sizeof(ckpt_v2));
		header_ptr = &ckpt_v2.header;
		stream_open_ptr = &ckpt_v2.ckpt_rec.stream_open;
		ckpt_ptr = &ckpt_v2;
	} else {
		memset(&ckpt_v1, 0, sizeof(ckpt_v1));
		header_ptr = &ckpt_v1.header;
		stream_open_ptr = &ckpt_v1.ckpt_rec.stream_open;
		ckpt_ptr = &ckpt_v1;
	}
	header_ptr->ckpt_rec_type = LGS_CKPT_OPEN_STREAM;
	header_ptr->num_ckpt_records = 1;
	header_ptr->data_len = 1;

	lgs_ckpt_stream_open_set(stream, stream_open_ptr);
	rc = lgs_ckpt_send_async(lgs_cb, ckpt_ptr, NCS_MBCSV_ACT_ADD);

	TRACE_LEAVE();
	return rc;
}

/**
 * Pack and send a close stream checkpoint using mbcsv
 * @param stream
 * @param recType
 *
 * @return uint32
 */
static uint32_t ckpt_stream_close(log_stream_t *stream, time_t closetime)
{
	uint32_t rc;
	lgsv_ckpt_msg_v1_t ckpt_v1;
	lgsv_ckpt_msg_v2_t ckpt_v2;
	void *ckpt_ptr;

	TRACE_ENTER();

	if (lgs_is_peer_v2()) {
		memset(&ckpt_v2, 0, sizeof(ckpt_v2));
		ckpt_v2.header.ckpt_rec_type = LGS_CKPT_CLOSE_STREAM;
		ckpt_v2.header.num_ckpt_records = 1;
		ckpt_v2.header.data_len = 1;

		/* No client. Logservice itself has opened stream */
		ckpt_v2.ckpt_rec.stream_close.clientId = -1;
		ckpt_v2.ckpt_rec.stream_close.streamId = stream->streamId;
		ckpt_v2.ckpt_rec.stream_close.c_file_close_time_stamp = closetime;
		ckpt_ptr = &ckpt_v2;
	} else {
		memset(&ckpt_v1, 0, sizeof(ckpt_v1));
		ckpt_v1.header.ckpt_rec_type = LGS_CKPT_CLOSE_STREAM;
		ckpt_v1.header.num_ckpt_records = 1;
		ckpt_v1.header.data_len = 1;

		/* No client. Logservice itself has opened stream */
		ckpt_v1.ckpt_rec.stream_close.clientId = -1;
		ckpt_v1.ckpt_rec.stream_close.streamId = stream->streamId;
		ckpt_ptr = &ckpt_v1;
	}

	rc = lgs_ckpt_send_async(lgs_cb, ckpt_ptr, NCS_MBCSV_ACT_ADD);

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
		report_om_error(immOiHandle, invocation, "Stream %s not found",
				objectName->value);
		goto done;
	}

	if (opId == SA_LOG_ADMIN_CHANGE_FILTER) {
		/* Only allowed to update runtime objects (application streams) */
		/**
		 * className holds a pointer to an duplicated memory (strdup).
		 * Must be free after done using.
		 */
		SaImmClassNameT className = immutil_get_className(objectName);

		if (!strcmp(className, "SaLogStreamConfig")) {
			(void)immutil_saImmOiAdminOperationResult(immOiHandle,
						  invocation, SA_AIS_ERR_NOT_SUPPORTED);
			free(className);
			goto done;
		}
		free(className);

		if (stream->streamType != STREAM_TYPE_APPLICATION) {
			report_om_error(immOiHandle, invocation,
					"Admin op change filter for non app stream");
			goto done;
		}
		
		if (param == NULL) {
			/* No parameters given in admin op */
			report_om_error(immOiHandle, invocation,
					"Admin op change filter: parameters are missing");
			goto done;			
		}

		if (strcmp(param->paramName, "saLogStreamSeverityFilter") != 0) {
			report_om_error(immOiHandle, invocation,
					"Admin op change filter, invalid param name");
			goto done;
		}

		if (param->paramType != SA_IMM_ATTR_SAUINT32T) {
			report_om_error(immOiHandle, invocation,
					"Admin op change filter: invalid parameter type");
			goto done;
		}

		severityFilter = *((SaUint32T *)param->paramBuffer);

		if (severityFilter > 0x7f) {	/* not a level, a bitmap */
			report_om_error(immOiHandle, invocation,
					"Admin op change filter: invalid severity");
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
		ckpt_stream_config(stream);
	} else {
		report_om_error(immOiHandle, invocation,
				"Invalid operation ID, should be %d (one) for change filter",
				SA_LOG_ADMIN_CHANGE_FILTER);
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
			report_oi_error(immOiHandle, ccbId,
					"Failed to get CCB object for %llu", ccbId);
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
			report_oi_error(immOiHandle, ccbId,
					"Failed to get CCB object for %llu", ccbId);
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
			report_oi_error(immOiHandle, ccbId,
					"Failed to get CCB object for %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	ccbutil_ccbAddModifyOperation(ccbUtilCcbData, objectName, attrMods);

 done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Creation of log service configuration object. Not allowed
 * 
 * @param immOiHandle
 * @param opdata
 * @return 
 */
static SaAisErrorT config_ccb_completed_create(SaImmOiHandleT immOiHandle,
		const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);
	report_oi_error(immOiHandle, opdata->ccbId,
			"Creation of OpenSafLogConfig object is not allowed, ccbId = %llu",
			opdata->ccbId);
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/* =================================================
 * config_ccb_completed_modify() with help functions
 * =================================================
 */

struct vattr_t {
	/* true if any of the attributes has been changed */
	bool validate_flag;
	SaUint32T logStreamSystemHighLimit;
	/* true if this attribute has been changed */
	bool logStreamSystemHighLimit_changed;
	SaUint32T logStreamSystemLowLimit;
	bool logStreamSystemLowLimit_changed;
	SaUint32T logStreamAppHighLimit;
	bool logStreamAppHighLimit_changed;
	SaUint32T logStreamAppLowLimit;
	bool logStreamAppLowLimit_changed;
};

/**
 * Validate attributes:
 * Must be done after all attributes has been read in order to check against
 * the correct values.
 * See lgs_config for validation functions
 * 
 * @param vattr[in] struct with attributes to validate
 * @param err_str [out] char vector of 256 bytes to return a string.
 * @return error code
 */
static SaAisErrorT validate_mailbox_limits(struct vattr_t vattr, char *err_str)
{	
	SaUint32T value32_high = 0;
	SaUint32T value32_low = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;
	int rc = 0;
	
	TRACE_ENTER();
	
	/* Validate attributes that can be modified */
	if (vattr.logStreamSystemHighLimit_changed) {
		value32_high = vattr.logStreamSystemHighLimit;
		if (vattr.logStreamSystemLowLimit_changed) {
			value32_low = vattr.logStreamSystemLowLimit;
		} else {
			value32_low = *(SaUint32T *) lgs_cfg_get(
					LGS_IMM_LOG_STREAM_SYSTEM_LOW_LIMIT);
		}

		rc = lgs_cfg_verify_mbox_limit(value32_high, value32_low);
		if (rc == -1) {
			ais_rc = SA_AIS_ERR_BAD_OPERATION;
			snprintf(err_str, 256, "HIGH limit < LOW limit");
			goto done;
		}
	}
	
	if (vattr.logStreamSystemLowLimit_changed) {
		value32_low = vattr.logStreamSystemLowLimit;
		if (vattr.logStreamSystemHighLimit_changed) {
			value32_high = vattr.logStreamSystemHighLimit;
		} else {
			value32_high = *(SaUint32T *) lgs_cfg_get(
					LGS_IMM_LOG_STREAM_SYSTEM_HIGH_LIMIT);
		}
		
		rc = lgs_cfg_verify_mbox_limit(value32_high, value32_low);
		if (rc == -1) {
			ais_rc = SA_AIS_ERR_BAD_OPERATION;
			snprintf(err_str, 256, "HIGH limit < LOW limit");
			goto done;
		}
	}
	
	if (vattr.logStreamAppHighLimit_changed) {
		value32_high = vattr.logStreamAppHighLimit;
		if (vattr.logStreamAppLowLimit_changed) {
			value32_low = vattr.logStreamAppLowLimit;
		} else {
			value32_low = *(SaUint32T *) lgs_cfg_get(
					LGS_IMM_LOG_STREAM_APP_LOW_LIMIT);
		}
		
		rc = lgs_cfg_verify_mbox_limit(value32_high, value32_low);
		if (rc == -1) {
			ais_rc = SA_AIS_ERR_BAD_OPERATION;
			snprintf(err_str, 256, "HIGH limit < LOW limit");
			goto done;
		}
	}
	
	if (vattr.logStreamAppLowLimit_changed) {
		value32_low = vattr.logStreamAppLowLimit;
		if (vattr.logStreamAppHighLimit_changed) {
			value32_high = vattr.logStreamAppHighLimit;
		} else {
			value32_high = *(SaUint32T *) lgs_cfg_get(
					LGS_IMM_LOG_STREAM_APP_HIGH_LIMIT);
		}
		
		rc = lgs_cfg_verify_mbox_limit(value32_high, value32_low);
		if (rc == -1) {
			ais_rc = SA_AIS_ERR_BAD_OPERATION;
			snprintf(err_str, 256, "HIGH limit < LOW limit");
			goto done;
		}
	}
	
	done:
	TRACE_LEAVE2("rc = %d", ais_rc);
	return ais_rc;
}

/**
 * Modification of attributes in log service configuration object.
 * Validate given attribute changes and report result to IMM
 *
 * @param immOiHandle
 * @param opdata
 * @return 
 */
static SaAisErrorT config_ccb_completed_modify(SaImmOiHandleT immOiHandle,
		const CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attrMod;
	SaAisErrorT ais_rc = SA_AIS_OK;
	int rc = 0;
	int i = 0;
	
	struct vattr_t vattr= {
		.validate_flag = false,
		.logStreamSystemHighLimit = 0,
		.logStreamSystemHighLimit_changed = false,
		.logStreamSystemLowLimit = 0,
		.logStreamSystemLowLimit_changed = false,
		.logStreamAppHighLimit = 0,
		.logStreamAppHighLimit_changed = false,
		.logStreamAppLowLimit = 0,
		.logStreamAppLowLimit_changed = false,
	};
	
	char oi_err_str[256];

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	attrMod = opdata->param.modify.attrMods[i++];
	while (attrMod != NULL) {
		void *value = NULL;
		const SaImmAttrValuesT_2 *attribute = &attrMod->modAttr;

		TRACE("attribute %s", attribute->attrName);

		/**
		 *  Ignore deletion of attributes
		 *  except for logDataGroupname,
		 *  and logStreamFileFormat
		 */
		if ((strcmp(attribute->attrName, LOG_DATA_GROUPNAME) != 0) &&
			 (strcmp(attribute->attrName, LOG_STREAM_FILE_FORMAT) != 0) &&
			 (attribute->attrValuesNumber == 0)) {
				report_oi_error(immOiHandle, opdata->ccbId,
					"deletion of value is not allowed for attribute %s stream %s",
					attribute->attrName, opdata->objectName.value);
				ais_rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
		}

		if (attribute->attrValuesNumber != 0) {
			value = attribute->attrValues[0];
		}

		if (!strcmp(attribute->attrName, LOG_ROOT_DIRECTORY)) {
			if (attribute->attrValuesNumber != 0) {
				char *pathName = *((char **)value);
				rc = lgs_cfg_verify_root_dir(pathName);
				if (rc == -1) {
					report_oi_error(immOiHandle, opdata->ccbId,
							"pathName: %s is NOT accepted", pathName);
					ais_rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
				TRACE("pathName: %s is accepted", pathName);
			}
		} else if (!strcmp(attribute->attrName, LOG_DATA_GROUPNAME)) {
			if (attribute->attrValuesNumber == 0) {
				TRACE("Deleting log data group");
			} else {
				char *groupname = *((char **)value);
				rc = lgs_cfg_verify_log_data_groupname(groupname);
				if (rc == -1) {
					report_oi_error(immOiHandle, opdata->ccbId,
							"groupname: %s is NOT accepted", groupname);
					ais_rc = SA_AIS_ERR_INVALID_PARAM;
					goto done;
				}
				TRACE("groupname: %s is accepted", groupname);
			}
		} else if (!strcmp(attribute->attrName, LOG_STREAM_FILE_FORMAT)) {
			if (attribute->attrValuesNumber == 0) {
				TRACE("Delete logStreamFileFormat");
			} else {
				char *logFileFormat = *((char **) value);
				rc = lgs_cfg_verify_log_file_format(logFileFormat);
				if (rc == -1) {
					report_oi_error(immOiHandle, opdata->ccbId,
									"%s value is NOT accepted", attribute->attrName);
					ais_rc = SA_AIS_ERR_INVALID_PARAM;
					goto done;
				}
				TRACE("logFileFormat: %s value is accepted", logFileFormat);
			}
		} else if (!strcmp(attribute->attrName, LOG_MAX_LOGRECSIZE)) {
			SaUint32T maxLogRecordSize = *((SaUint32T *)value);
			rc = lgs_cfg_verify_max_logrecsize(maxLogRecordSize);
			if (rc == -1) {
				report_oi_error(immOiHandle, opdata->ccbId,
						"%s value is NOT accepted", attribute->attrName);
				ais_rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}
			TRACE("maxLogRecordSize: %s is accepted", attribute->attrName);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_SYSTEM_HIGH_LIMIT)) {
			vattr.logStreamSystemHighLimit = *((SaUint32T *)value);
			vattr.logStreamSystemHighLimit_changed = true;
			TRACE("%s %s = %d",__FUNCTION__, attribute->attrName,
					vattr.logStreamSystemHighLimit);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_SYSTEM_LOW_LIMIT)) {
			vattr.logStreamSystemLowLimit = *((SaUint32T *)value);
			vattr.logStreamSystemLowLimit_changed = true;
			TRACE("%s %s = %d",__FUNCTION__, attribute->attrName,
					vattr.logStreamSystemHighLimit);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_APP_HIGH_LIMIT)) {
			vattr.logStreamAppHighLimit = *((SaUint32T *)value);
			vattr.logStreamAppHighLimit_changed = true;
			TRACE("%s %s = %d",__FUNCTION__, attribute->attrName,
					vattr.logStreamSystemHighLimit);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_APP_LOW_LIMIT)) {
			vattr.logStreamAppLowLimit = *((SaUint32T *)value);
			vattr.logStreamAppLowLimit_changed = true;
			TRACE("%s %s = %d",__FUNCTION__, attribute->attrName,
					vattr.logStreamSystemHighLimit);
		} else if (!strcmp(attribute->attrName, LOG_MAX_APPLICATION_STREAMS)) {
			report_oi_error(immOiHandle, opdata->ccbId,
					"%s cannot be changed", attribute->attrName);
			ais_rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		} else if (!strcmp(attribute->attrName, LOG_FILE_IO_TIMEOUT)) {
			SaUint32T logFileIoTimeout = *((SaUint32T *) value);
			rc = lgs_cfg_verify_file_io_timeout(logFileIoTimeout);
			if (rc == -1) {
				report_oi_error(immOiHandle, opdata->ccbId,
					"%s value is NOT accepted", attribute->attrName);
				ais_rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}
			TRACE("logFileIoTimeout: %d value is accepted", logFileIoTimeout);
		} else if (!strcmp(attribute->attrName, LOG_FILE_SYS_CONFIG)) {
			report_oi_error(immOiHandle, opdata->ccbId,
					"%s cannot be changed", attribute->attrName);
			ais_rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		} else {
			report_oi_error(immOiHandle, opdata->ccbId,
					"attribute %s not recognized", attribute->attrName);
			ais_rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}

		attrMod = opdata->param.modify.attrMods[i++];
	}
	
	ais_rc = validate_mailbox_limits(vattr, oi_err_str);
	if (ais_rc != SA_AIS_OK) {
		TRACE("Reporting oi error \"%s\"", oi_err_str);
		report_oi_error(immOiHandle, opdata->ccbId, "%s", oi_err_str);
	}
	
done:
	TRACE_LEAVE2("rc=%u", ais_rc);
	return ais_rc;
}

/**
 * Delete log service configuration object. Not allowed
 * 
 * @param immOiHandle
 * @param opdata
 * @return 
 */
static SaAisErrorT config_ccb_completed_delete(SaImmOiHandleT immOiHandle,
		const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);
	report_oi_error(immOiHandle, opdata->ccbId,
			"Deletion of OpenSafLogConfig object is not allowed");
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static SaAisErrorT config_ccb_completed(SaImmOiHandleT immOiHandle,
		const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		rc = config_ccb_completed_create(immOiHandle, opdata);
		break;
	case CCBUTIL_MODIFY:
		rc = config_ccb_completed_modify(immOiHandle, opdata);
		break;
	case CCBUTIL_DELETE:
		rc = config_ccb_completed_delete(immOiHandle, opdata);
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
	
	TRACE_ENTER();
	if (lgs_stream_defval_updated_flag == false) {
		/* Get class defaults for SaLogStreamConfig class from IMM
		 * We are only interested in saLogStreamMaxLogFileSize and
		 * saLogStreamFixedLogRecordSize
		 */
		int iu_setting = immutilWrapperProfile.errorsAreFatal;
		immutilWrapperProfile.errorsAreFatal = 0;
		
		rc = immutil_saImmOmInitialize(&om_handle, NULL, &immVersion);
		if (rc != SA_AIS_OK) {
			TRACE("immutil_saImmOmInitialize fail rc=%d", rc);
		}
		if (rc == SA_AIS_OK) {
			rc = immutil_saImmOmClassDescriptionGet_2(om_handle, "SaLogStreamConfig",
					&cc, &attributes);
		}
		
		if (rc == SA_AIS_OK) {
			while ((attribute = attributes[i++]) != NULL) {
				if (!strcmp(attribute->attrName, "saLogStreamMaxLogFileSize")) {
					TRACE("Got saLogStreamMaxLogFileSize");
					lgs_stream_defval.saLogStreamMaxLogFileSize =
							*((SaUint64T *) attribute->attrDefaultValue);
					TRACE("value = %lld",
							lgs_stream_defval.saLogStreamMaxLogFileSize);
				} else if (!strcmp(attribute->attrName, "saLogStreamFixedLogRecordSize")) {
					TRACE("Got saLogStreamFixedLogRecordSize");
					lgs_stream_defval.saLogStreamFixedLogRecordSize =
							*((SaUint32T *) attribute->attrDefaultValue);
					TRACE("value = %d",
							lgs_stream_defval.saLogStreamFixedLogRecordSize);
				}
			}
			
			rc = immutil_saImmOmClassDescriptionMemoryFree_2(om_handle, attributes);
			if (rc != SA_AIS_OK) {
				LOG_ER("%s: Failed to free class description memory rc=%d",
					__FUNCTION__, rc);
				osafassert(0);
			}
			lgs_stream_defval_updated_flag = true;
		} else {
			/* Default values are not fetched. Temporary use hard coded values.
			 */
			TRACE("saImmOmClassDescriptionGet_2 failed rc=%d", rc);
			lgs_stream_defval.saLogStreamMaxLogFileSize = 5000000;
			lgs_stream_defval.saLogStreamFixedLogRecordSize = 150;
		}
		
		rc = immutil_saImmOmFinalize(om_handle);
		if (rc != SA_AIS_OK) {
			TRACE("immutil_saImmOmFinalize fail rc=%d", rc);
		}
				
		immutilWrapperProfile.errorsAreFatal = iu_setting;
	} else {
		TRACE("Defaults are already fetched");
		TRACE("saLogStreamMaxLogFileSize=%lld",
				lgs_stream_defval.saLogStreamMaxLogFileSize);
		TRACE("saLogStreamFixedLogRecordSize=%d",
				lgs_stream_defval.saLogStreamFixedLogRecordSize);
	}
	
	
	TRACE_LEAVE();
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
	
	TRACE_ENTER();
	TRACE("fileName \"%s\", pathName \"%s\"", fileName, pathName);
	
	/* If a stream is modified only the name may be modified. The path name
	 * must be fetched from the stream.
	 */
	if (operationType == CCBUTIL_MODIFY) {
		TRACE("MODIFY");
		if (stream == NULL) {
			/* No stream to modify. Should never happen */
			osafassert(0);
		}
		if ((fileName == NULL) && (pathName == NULL)) {
			/* Nothing has changed */
			TRACE("Nothing has changed");
			return false;
		}
		if (fileName == NULL) {
			i_fileName = stream->fileName;
			TRACE("From stream: fileName \"%s\"", i_fileName);
		} else {
			i_fileName = fileName;
		}
		if (pathName == NULL) {
			i_pathName = stream->pathName;
			TRACE("From stream: pathName \"%s\"", i_pathName);
		} else {
			i_pathName = pathName;
		}
	} else if (operationType == CCBUTIL_CREATE) {
		TRACE("CREATE");
		if ((fileName == NULL) || (pathName == NULL)) {
			/* Should never happen
			 * A valid fileName and pathName is always given at create */
			LOG_ER("fileName or pathName is not a string");
			osafassert(0);
		}
		
		i_fileName = fileName;
		i_pathName = pathName;
	} else {
		/* Unknown operationType. Should never happen */
			osafassert(0);
	}
	
	/* Check if any stream has given filename and path */
	TRACE("Check if any stream has given filename and path");
	i_stream = log_stream_getnext_by_name(NULL);
	while (i_stream != NULL) {
		TRACE("Check stream \"%s\"", i_stream->name);
		if ((strncmp(i_stream->fileName, i_fileName, NAME_MAX) == 0) &&
			(strncmp(i_stream->pathName, i_pathName, SA_MAX_NAME_LENGTH) == 0)) {
			rc = true;
			break;
		}
		i_stream = log_stream_getnext_by_name(i_stream->name);
	}
	
	TRACE_LEAVE2("rc = %d", rc);
	return rc;
}

/**
 * Verify fixedLogRecordSize and maxLogFileSize.
 * Rules:
 * 1 - saLogStreamFixedLogRecordSize must be less than saLogStreamMaxLogFileSize
 * 2 - saLogStreamFixedLogRecordSize must be less than or equal to logMaxLogrecsize
 * 3 - saLogStreamFixedLogRecordSize can be 0. Means variable record size
 * 4 - saLogStreamMaxLogFileSize must be bigger than 0. No limit is not supported
 * 5 - saLogStreamMaxLogFileSize must be bigger than logMaxLogrecsize
 * 6 - saLogStreamMaxLogFileSize must be bigger than saLogStreamFixedLogRecordSize
 * 
 * The ..._mod variable == true means that the corresponding attribute is
 * modified.
 * 
 * @param immOiHandle[in]
 * @param streamMaxLogFileSize[in]
 * @param streamMaxLogFileSize_mod[in]
 * @param streamFixedLogRecordSize[in]
 * @param streamFixedLogRecordSize_mod[in]
 * @param stream[in]
 * @param operationType[in]
 * @return false if error
 */
static bool chk_max_filesize_recordsize_compatible(SaImmOiHandleT immOiHandle,
		SaUint64T streamMaxLogFileSize,
		bool streamMaxLogFileSize_mod,
		SaUint32T streamFixedLogRecordSize,
		bool streamFixedLogRecordSize_mod,
		log_stream_t *stream,
		enum CcbUtilOperationType operationType,
		SaImmOiCcbIdT ccbId)
{
	SaUint64T i_streamMaxLogFileSize = 0;
	SaUint32T i_streamFixedLogRecordSize = 0;
	SaUint32T i_logMaxLogrecsize = 0;
	lgs_stream_defval_t *stream_default;
	
	bool rc = true;
	
	TRACE_ENTER();
	
	/** Get all parameters **/
	
	/* Get logMaxLogrecsize from configuration parameters */
	i_logMaxLogrecsize = *(SaUint32T *) lgs_cfg_get(
			LGS_IMM_LOG_MAX_LOGRECSIZE);
	TRACE("i_logMaxLogrecsize = %d", i_logMaxLogrecsize);
	/* Get stream default settings */
	stream_default = get_SaLogStreamConfig_default();
	
	if (operationType == CCBUTIL_MODIFY) {
		TRACE("operationType == CCBUTIL_MODIFY");
		/* The stream exists. 
		 */
		if (stream == NULL) {
			/* Should never happen */
			LOG_ER("%s stream == NULL", __FUNCTION__);
			osafassert(0);
		}
		if (streamFixedLogRecordSize_mod == false) {
			/* streamFixedLogRecordSize is not given. Get from stream */
			i_streamFixedLogRecordSize = stream->fixedLogRecordSize;
			TRACE("Get from stream, streamFixedLogRecordSize = %d",
					i_streamFixedLogRecordSize);
		} else {
			i_streamFixedLogRecordSize = streamFixedLogRecordSize;
		}
		
		if (streamMaxLogFileSize_mod == false) {
			/* streamMaxLogFileSize is not given. Get from stream */
			i_streamMaxLogFileSize = stream->maxLogFileSize;
			TRACE("Get from stream, streamMaxLogFileSize = %lld",
					i_streamMaxLogFileSize);
		} else {
			i_streamMaxLogFileSize = streamMaxLogFileSize;
			TRACE("Modified streamMaxLogFileSize = %lld", i_streamMaxLogFileSize);
		}
	} else if (operationType == CCBUTIL_CREATE) {
		TRACE("operationType == CCBUTIL_CREATE");
		/* The stream does not yet exist
		 */
		if (streamFixedLogRecordSize_mod == false) {
			/* streamFixedLogRecordSize is not given. Use default */
			i_streamFixedLogRecordSize = stream_default->saLogStreamFixedLogRecordSize;
			TRACE("Get default, streamFixedLogRecordSize = %d",
					i_streamFixedLogRecordSize);
		} else {
			i_streamFixedLogRecordSize = streamFixedLogRecordSize;
		}
		
		if (streamMaxLogFileSize_mod == false) {
			/* streamMaxLogFileSize is not given. Use default */
			i_streamMaxLogFileSize = stream_default->saLogStreamMaxLogFileSize;
			TRACE("Get default, streamMaxLogFileSize = %lld",
					i_streamMaxLogFileSize);
		} else {
			i_streamMaxLogFileSize = streamMaxLogFileSize;
		}		
	} else {
		/* Unknown operationType */
		LOG_ER("%s Unknown operationType", __FUNCTION__);
		osafassert(0);
	}


	/* 
	 * Do verification
	 */
	if (streamMaxLogFileSize_mod == true) {
		if (i_streamMaxLogFileSize <= i_logMaxLogrecsize) {
			/* streamMaxLogFileSize must be bigger than logMaxLogrecsize */
			report_oi_error(immOiHandle, ccbId,
							"streamMaxLogFileSize out of range");
			TRACE("i_streamMaxLogFileSize (%lld) <= i_logMaxLogrecsize (%d)",
				  i_streamMaxLogFileSize, i_logMaxLogrecsize);
			rc = false;
			goto done;
		} else if (i_streamMaxLogFileSize <= i_streamFixedLogRecordSize) {
			/* streamMaxLogFileSize must be bigger than streamFixedLogRecordSize */
			report_oi_error(immOiHandle, ccbId,
							"streamMaxLogFileSize out of range");
			TRACE("i_streamMaxLogFileSize (%lld) <= i_streamFixedLogRecordSize (%d)",
				  i_streamMaxLogFileSize, i_streamFixedLogRecordSize);
			rc = false;
			goto done;
		}
	}

	if (streamFixedLogRecordSize_mod == true) {
		if (i_streamFixedLogRecordSize == 0) {
			/* streamFixedLogRecordSize can be 0. Means variable record size */
			TRACE("streamFixedLogRecordSize = 0");
			rc = true;
		} else if (i_streamFixedLogRecordSize < SA_LOG_MIN_RECORD_SIZE) {
			/* streamFixedLogRecordSize must be larger than 150 */
			report_oi_error(immOiHandle, ccbId,
							"streamFixedLogRecordSize out of range");
			TRACE("i_streamFixedLogRecordSize (%d) < %d",
				  i_streamFixedLogRecordSize, SA_LOG_MIN_RECORD_SIZE);
			rc = false;
		} else if (i_streamFixedLogRecordSize >= i_streamMaxLogFileSize) {
			/* streamFixedLogRecordSize must be less than streamMaxLogFileSize */
			report_oi_error(immOiHandle, ccbId,
							"streamFixedLogRecordSize out of range");
			TRACE("i_streamFixedLogRecordSize (%d) >= i_streamMaxLogFileSize (%lld)",
				  i_streamFixedLogRecordSize, i_streamMaxLogFileSize);
			rc = false;
		} else if (i_streamFixedLogRecordSize > i_logMaxLogrecsize) {
			/* streamFixedLogRecordSize must be less than streamMaxLogFileSize */
			report_oi_error(immOiHandle, ccbId,
							"streamFixedLogRecordSize out of range");
			TRACE("i_streamFixedLogRecordSize (%d) > i_logMaxLogrecsize (%d)",
				  i_streamFixedLogRecordSize, i_logMaxLogrecsize);
			rc = false;
		}
	}

done:
	TRACE_LEAVE2("rc = %d", rc);
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
	SaUint32T i_streamFixedLogRecordSize = 0;
	bool i_streamFixedLogRecordSize_mod = false;
	SaUint64T i_streamMaxLogFileSize = 0;
	bool i_streamMaxLogFileSize_mod = false;
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
	
	TRACE_ENTER();
	
	/* Get first attribute if any and fill in name and path if the stream
	 * exist (modify)
	 */
	if (opdata->operationType == CCBUTIL_MODIFY) {
		TRACE("Validate for MODIFY");
		stream = log_stream_get_by_name(
				(char *) opdata->param.modify.objectName->value);
		if (stream == NULL) {
			/* No stream to modify */
			report_oi_error(immOiHandle, opdata->ccbId,
					"Stream does not exist");
			TRACE("Stream does not exist");
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		} else {
			/* Get first attribute */
			if (opdata->param.modify.attrMods[aindex] != NULL) {
				attribute = &opdata->param.modify.attrMods[aindex]->modAttr;
				aindex++;
				TRACE("First attribute \"%s\" fetched",
						attribute->attrName);
			} else {
				TRACE("No attributes found");
			}
			/* Get relative path and file name from stream.
			 * Name may be modified
			 */
			i_pathName = stream->pathName;
			i_fileName = stream->fileName;
			TRACE("From stream: pathName \"%s\", fileName \"%s\"",
					i_pathName, i_fileName);
		}
	} else if (opdata->operationType == CCBUTIL_CREATE){
		TRACE("Validate for CREATE");
		/* Check if stream already exist after parameters are saved.
		 * Parameter value for fileName is needed.
		 */
		/* Get first attribute */
		attribute = opdata->param.create.attrValues[aindex];
		aindex++;
		TRACE("First attribute \"%s\" fetched", attribute->attrName);
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
			report_oi_error(immOiHandle, opdata->ccbId,
					"Attribute %s has no value",attribute->attrName);
			TRACE("Modify: Attribute %s has no value",attribute->attrName);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		} else {
			/* If create all attributes will be present also the ones without
			 * any value.
			 */
			TRACE("Create: Attribute %s has no value",attribute->attrName);
			goto next;
		}
		
		/* Save attributes with a value */
		if (!strcmp(attribute->attrName, "saLogStreamFileName")) {
			/* Save filename. Check later together with path name */
			i_fileName = *((char **) value);
			i_fileName_mod = true;
			TRACE("Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamPathName")) {
			/* Save path name. Check later together with filename */
			i_pathName = *((char **) value);
			i_pathName_mod = true;
			TRACE("Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamMaxLogFileSize")) {
			/* Save and compare with FixedLogRecordSize after all attributes
			 * are read. Must be bigger than FixedLogRecordSize
			 */
			i_streamMaxLogFileSize = *((SaUint64T *) value);
			i_streamMaxLogFileSize_mod = true;
			TRACE("Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamFixedLogRecordSize")) {
			/* Save and compare with MaxLogFileSize after all attributes
			 * are read. Must be smaller than MaxLogFileSize
			 */
			i_streamFixedLogRecordSize = *((SaUint64T *) value);
			i_streamFixedLogRecordSize_mod = true;
			TRACE("Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFullAction")) {
			i_logFullAction = *((SaUint32T *) value);
			i_logFullAction_mod = true;
			TRACE("Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFileFormat")) {
			i_logFileFormat = *((char **) value);
			i_logFileFormat_mod = true;
			TRACE("Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamLogFullHaltThreshold")) {
			i_logFullHaltThreshold = *((SaUint32T *) value);
			i_logFullHaltThreshold_mod = true;
			TRACE("Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamMaxFilesRotated")) {
			i_maxFilesRotated = *((SaUint32T *) value);
			i_maxFilesRotated_mod = true;
			TRACE("Saved attribute \"%s\"", attribute->attrName);
			
		} else if (!strcmp(attribute->attrName, "saLogStreamSeverityFilter")) {
			i_severityFilter = *((SaUint32T *) value);
			i_severityFilter_mod = true;
			TRACE("Saved attribute \"%s\" = %d", attribute->attrName,
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
			TRACE("Checking saLogStreamPathName");
			if (i_pathName == NULL) {
				/* Must point to a string */
				rc = SA_AIS_ERR_BAD_OPERATION;
				report_oi_error(immOiHandle, opdata->ccbId,
						"NULL pointer to saLogStreamPathName");
				TRACE("NULL pointer to saLogStreamPathName");
				goto done;
			}

			if (strlen(i_pathName) > PATH_MAX) {
				/* Path name is too long */
				rc = SA_AIS_ERR_BAD_OPERATION;
				report_oi_error(immOiHandle, opdata->ccbId,
						"saLogStreamPathName is so long");
				TRACE("saLogStreamPathName is so long (max: %d)", PATH_MAX);
				goto done;
			}

			if (lgs_relative_path_check_ts(i_pathName) == true) {
				report_oi_error(immOiHandle, opdata->ccbId,
							"Path %s not valid", i_pathName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("Path %s not valid", i_pathName);
				goto done;
			}
		}
		
		/* saLogStreamFileName
		 * Must be a name. A stream with this name and relative path must not
		 * already exist.
		 */
		if (i_fileName_mod) {
			TRACE("Checking saLogStreamFileName");
			if (i_fileName == NULL) {
				/* Must point to a string */
				rc = SA_AIS_ERR_BAD_OPERATION;
				report_oi_error(immOiHandle, opdata->ccbId,
						"NULL pointer to saLogStreamFileName");
				TRACE("NULL pointer to saLogStreamFileName");
				goto done;
			}

			if (strlen(i_fileName) > LOG_NAME_MAX) {
				/* File name is too long */
				rc = SA_AIS_ERR_BAD_OPERATION;
				report_oi_error(immOiHandle, opdata->ccbId,
						"saLogStreamFileName is so long");
				TRACE("saLogStreamFileName is so long (max: %d)", LOG_NAME_MAX);
				goto done;
			}
			/* Not allow special characters exising in file name */
			if (lgs_has_special_char(i_fileName) == true) {
				/* Report failed if has special character in file name */
				rc = SA_AIS_ERR_BAD_OPERATION;
				report_oi_error(immOiHandle, opdata->ccbId,
								"Invalid saLogStreamFileName value");
				TRACE("\t448 Invalid saLogStreamFileName value");
				goto done;
			}

			if (chk_filepath_stream_exist(i_fileName, i_pathName,
					stream, opdata->operationType)) {
				report_oi_error(immOiHandle, opdata->ccbId,
						"Path/file %s/%s already exist", i_pathName, i_fileName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("Path/file %s/%s already exist",
						i_pathName, i_fileName);
				goto done;
			}
		}
		
		/* saLogStreamMaxLogFileSize or saLogStreamFixedLogRecordSize
		 * See chk_max_filesize_recordsize_compatible() for rules
		 */
		if (i_streamMaxLogFileSize_mod || i_streamFixedLogRecordSize_mod) {
			TRACE("Check saLogStreamMaxLogFileSize,"
					" saLogStreamFixedLogRecordSize");
			if (chk_max_filesize_recordsize_compatible(immOiHandle,
					i_streamMaxLogFileSize,
					i_streamMaxLogFileSize_mod,
					i_streamFixedLogRecordSize,
					i_streamFixedLogRecordSize_mod,
					stream,
					opdata->operationType,
					opdata->ccbId) == false) {
				/* report_oi_error is done within check function */
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("chk_max_filesize_recordsize_compatible Fail");
				goto done;
			}
		}
		
		/* saLogStreamLogFullAction
		 * 1, 2 and 3 are valid according to AIS but only action rotate (3)
		 * is supported.
		 */
		if (i_logFullAction_mod) {
			TRACE("Check saLogStreamLogFullAction");
			/* If this attribute is changed an oi error report is written
			 * and the value is changed (backwards compatible) but this will
			 * not affect change action
			 */
			if ((i_logFullAction < SA_LOG_FILE_FULL_ACTION_WRAP) ||
					(i_logFullAction > SA_LOG_FILE_FULL_ACTION_ROTATE)) {
				report_oi_error(immOiHandle, opdata->ccbId,
						"logFullAction out of range");
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("logFullAction out of range");
				goto done;
			}
			if ((i_logFullAction == SA_LOG_FILE_FULL_ACTION_WRAP) ||
					(i_logFullAction == SA_LOG_FILE_FULL_ACTION_HALT)) {
				report_oi_error(immOiHandle, opdata->ccbId,
						"logFullAction:Current Implementation doesn't support Wrap and halt");
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("logFullAction: Not supported");
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
					report_oi_error(immOiHandle, opdata->ccbId,
							"Invalid logFileFormat: %s", i_logFileFormat);
					rc = SA_AIS_ERR_BAD_OPERATION;
					TRACE("Create: Invalid logFileFormat: %s",
							i_logFileFormat);
					goto done;
				}
			}
			else {
				if (!lgs_is_valid_format_expression(i_logFileFormat, stream->streamType, &dummy)) {
					report_oi_error(immOiHandle, opdata->ccbId,
							"Invalid logFileFormat: %s", i_logFileFormat);
					rc = SA_AIS_ERR_BAD_OPERATION;
					TRACE("Modify: Invalid logFileFormat: %s",
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
			TRACE("Checking saLogStreamLogFullHaltThreshold");
			if (i_logFullHaltThreshold >= 100) {
				report_oi_error(immOiHandle, opdata->ccbId,
						"logFullHaltThreshold out of range");
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}
		
		/* saLogStreamMaxFilesRotated
		 * < 127
		 */
		if (i_maxFilesRotated_mod) {
			TRACE("Checking saLogStreamMaxFilesRotated");
			if ((i_maxFilesRotated < 1) || (i_maxFilesRotated > 127)) {
				report_oi_error(immOiHandle, opdata->ccbId,
						"maxFilesRotated out of range (min 1, max 127): %u",
						i_maxFilesRotated);
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("maxFilesRotated out of range "
						"(min 1, max 127): %u", i_maxFilesRotated);
				goto done;
			}
		}
		
		/* saLogStreamSeverityFilter
		 *     <= 0x7f
		 */
		if (i_severityFilter_mod) {
			TRACE("Checking saLogStreamSeverityFilter");
			if (i_severityFilter > 0x7f) {
				report_oi_error(immOiHandle, opdata->ccbId,
					"Invalid severity: %x", i_severityFilter);
				rc = SA_AIS_ERR_BAD_OPERATION;
				TRACE("Invalid severity: %x", i_severityFilter);
				goto done;
			}
		}
	}
	
	done:
		TRACE_LEAVE2("rc = %d", rc);
		return rc;
}

/**
 * Create a log stream configuration object
 * @param immOiHandle
 * @param opdata
 * @return 
 */
static SaAisErrorT stream_ccb_completed_create(SaImmOiHandleT immOiHandle, const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu", opdata->ccbId);
	if (!check_max_stream()) {
		rc = check_attr_validity(immOiHandle, opdata);
	} else {
		report_oi_error(immOiHandle, opdata->ccbId,
				"Number of stream out of limitation");
	}
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**
 * Modify attributes in log stream configuration object
 * @param opdata
 * @return 
 */
static SaAisErrorT stream_ccb_completed_modify(SaImmOiHandleT immOiHandle, const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);
	rc = check_attr_validity(immOiHandle, opdata);
	TRACE_LEAVE2("rc = %u", rc);
	return rc;
}

/**
 * Delete log stream configuration object
 * @param immOiHandle
 * @param opdata
 * @return 
 */
static SaAisErrorT stream_ccb_completed_delete(SaImmOiHandleT immOiHandle, const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	const char *name = (char*) opdata->param.deleteOp.objectName->value;
	log_stream_t *stream = log_stream_get_by_name(name);

	if (stream != NULL) {
		if (stream->streamId < 3) {
			report_oi_error(immOiHandle, opdata->ccbId,
					"Stream delete: well known stream '%s' cannot be deleted", name);
			goto done;
		}

		if (stream->numOpeners > 1) {
			report_oi_error(immOiHandle, opdata->ccbId,
					"Stream '%s' cannot be deleted: opened by %u clients",
					name, stream->numOpeners);
			goto done;
		}

		rc = SA_AIS_OK;
	} else {
		report_oi_error(immOiHandle, opdata->ccbId,
				"stream %s not found", name);
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static SaAisErrorT stream_ccb_completed(SaImmOiHandleT immOiHandle, const CcbUtilOperationData_t *opdata)
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
		rc = stream_ccb_completed_delete(immOiHandle, opdata);
		break;
	default:
		assert(0);
		break;
	}

	TRACE_LEAVE2("rc = %u", rc);
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
	SaAisErrorT cb_rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *opdata;

	TRACE_ENTER2("CCB ID %llu", ccbId);

	if (lgs_cb->ha_state != SA_AMF_HA_ACTIVE) {
		TRACE("State Not Active. Nothing to do, we are an applier");
		goto done;
	}

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_ER("%s: Failed to find CCB object for %llu", __FUNCTION__, ccbId);
		cb_rc = SA_AIS_ERR_BAD_OPERATION;
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
				rc = config_ccb_completed(immOiHandle, opdata);
			else if (!strcmp(opdata->param.create.className, "SaLogStreamConfig"))
				rc = stream_ccb_completed(immOiHandle, opdata);
			else
				osafassert(0);
			break;
		case CCBUTIL_DELETE:
		case CCBUTIL_MODIFY:
			if (!strncmp((char*)opdata->objectName.value, "safLgStrCfg", 11)) {
				rc = stream_ccb_completed(immOiHandle, opdata);
			} else {
				rc = config_ccb_completed(immOiHandle, opdata);
			}
			break;
		default:
			assert(0);
			break;
		}
	}

 done:
	/* 
	 * For now always return rc (original code)
	 * TBD: Only use relevant return codes for Completed Callback
	 */
	cb_rc = rc;
	TRACE_LEAVE2("rc = %u",cb_rc);
	return cb_rc;
}
/**
 * Set logRootDirectory to new value
 *   - Close all open logfiles
 *   - Rename all log files and .cfg files.
 *   - Open all logfiles and .cfg files in new directory.
 *
 * @param new_logRootDirectory[in]
 *             String containg path for new root directory
 * @param old_logRootDirectory[in]
 *            String containg path for old root directory
 * @param close_time[in]
 *            Time for file close time stamp
 */
/**
 *
 * @param new_logRootDirectory
 * @param old_logRootDirectory
 * @param cur_time_in
 */
void logRootDirectory_filemove(
		const char *new_logRootDirectory,
		const char *old_logRootDirectory,
		time_t *cur_time_in)
{
	TRACE_ENTER();
	log_stream_t *stream;
	int n = 0;
	char *current_logfile;
	
	/* Shall never happen */
	if (strlen(new_logRootDirectory) + 1 > PATH_MAX) {
		LOG_ER("%s Root path > PATH_MAX! Abort", __FUNCTION__);
		osafassert(0);
	}	
	
	/* Close and rename files at current path
	 */
	stream = log_stream_getnext_by_name(NULL);
	while (stream != NULL) {
		TRACE("Handling file %s", stream->logFileCurrent);

		if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE) {
			current_logfile = stream->logFileCurrent;
		} else {
			current_logfile = stream->stb_logFileCurrent;
		}

		TRACE("current_logfile \"%s\"",current_logfile);
		
		if (log_stream_config_change(!LGS_STREAM_CREATE_FILES,
				old_logRootDirectory, stream, current_logfile,
				cur_time_in) != 0) {
			LOG_ER("Old log files could not be renamed and closed for stream: %s",
					stream->name);
		}
		stream = log_stream_getnext_by_name(stream->name);
	}

	/* Create new files at new path
	 */
	stream = log_stream_getnext_by_name(NULL);
	while (stream != NULL) {
		if (lgs_create_config_file_h(new_logRootDirectory, stream) != 0) {
			LOG_ER("New config file could not be created for stream: %s",
					stream->name);
		}

		/* Create the new log file based on updated configuration */
		char *current_time = lgs_get_time(cur_time_in);
		n = snprintf(stream->logFileCurrent, NAME_MAX, "%s_%s", stream->fileName,
				current_time);
		if (n >= NAME_MAX) {
			LOG_ER("New log file could not be created for stream: %s",
					stream->name);
		} else if ((*stream->p_fd = log_file_open(new_logRootDirectory,
			stream, stream->logFileCurrent,NULL)) == -1) {
			LOG_ER("New log file could not be created for stream: %s",
					stream->name);
		}
		
		/* Also update standby current file name
		 * Used if standby and configured for split file system
		 */
		strcpy(stream->stb_logFileCurrent, stream->logFileCurrent);
		
		stream = log_stream_getnext_by_name(stream->name);
	}
	TRACE_LEAVE();
}

/**
 * Apply new group name
 *   - Update lgs_conf with new group (logDataGroupname).
 *   - Reown all log files by this new group.
 *
 * @param new_logDataGroupname[in]
 *            String contains new group.
 */
void logDataGroupname_fileown(const char *new_logDataGroupname)
{
	TRACE_ENTER();
	log_stream_t *stream;

	if (new_logDataGroupname == NULL) {
		LOG_ER("Data group is NULL");
		return;
	}

	/* For each log stream, reown all log files */
	if (strcmp(new_logDataGroupname, "")) {
		/* Not attribute values deletion
		 * Change ownership of log files to this new group
		 */
		stream = log_stream_getnext_by_name(NULL);
		while (stream != NULL) {
			lgs_own_log_files_h(stream, new_logDataGroupname);
			stream = log_stream_getnext_by_name(stream->name);
		}
	}
	TRACE_LEAVE();
}


/**
 * Change log root directory
 *
 * @param old_logRootDirectory[in]
 * @param new_logRootDirectory[in]
 */
static void apply_conf_logRootDirectory(
		const char *old_logRootDirectory,
		const char *new_logRootDirectory)
{
	struct timespec curtime_tspec;
	osaf_clock_gettime(CLOCK_REALTIME, &curtime_tspec);
	time_t cur_time = curtime_tspec.tv_sec;

	logRootDirectory_filemove(
		new_logRootDirectory,
		old_logRootDirectory,
		&cur_time);

}

static void apply_conf_logDataGroupname(const char *logDataGroupname)
{
	char *value_ptr = NULL;
	char noGroupname[] = "";
	
	if (logDataGroupname == NULL)
		value_ptr = noGroupname;
	else
		value_ptr = (char *) logDataGroupname;

	logDataGroupname_fileown(value_ptr);
}

/**
 * Apply changes. Validation is not needed here since all validation is done in
 * the complete callback
 *
 * @param opdata[in] From IMM
 */
static void config_ccb_apply_modify(const CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attrMod;
	int i = 0;
	char *value_str = NULL;
	uint32_t uint32_val = 0;
	char uint32_str[20] = {0};
	lgs_config_chg_t config_data = {NULL, 0};
	int rc = 0;
	bool root_dir_chg_flag = false; /* Needed for v3 ckpt protocol */

	/* Flag set if any of the mailbox limit values have changed */
	bool mailbox_lim_upd = false;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	attrMod = opdata->param.modify.attrMods[i++];
	while (attrMod != NULL) {
		/* Get all changed config data and save in cfg buffer */
		const SaImmAttrValuesT_2 *attribute = &attrMod->modAttr;
		void *value = NULL;
		if (attribute->attrValuesNumber != 0)
			value = attribute->attrValues[0];
		else
			value = NULL;

		TRACE("attribute %s", attribute->attrName);

		/* For each modified attribute:
		 * - Get the new value
		 * - Apply the change
		 * - Add to update buffer. Used for updating configuration data
		 *   and check-pointing
		 */
		if (!strcmp(attribute->attrName, LOG_ROOT_DIRECTORY)) {
			value_str = *((char **)value); /* New directory */
			char *old_dir =
				(char *) lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY);
			apply_conf_logRootDirectory(old_dir, value_str);
			lgs_cfgupd_list_create(LOG_ROOT_DIRECTORY,
				value_str, &config_data);
			root_dir_chg_flag = true;
		} else if (!strcmp(attribute->attrName, LOG_DATA_GROUPNAME)) {
			if (value == NULL) {
				value_str = "";
			} else {
				value_str = *((char **)value);
			}
			apply_conf_logDataGroupname(value_str);
			lgs_cfgupd_list_create(LOG_DATA_GROUPNAME,
				value_str, &config_data);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_FILE_FORMAT)) {
			if (value == NULL) {
				/* Use built-in file format as default */
				value_str = DEFAULT_APP_SYS_FORMAT_EXP;
			} else {
				value_str = *((char **)value);
			}
			/**
			 * This attribute value is used for default log file format of
			 * app stream. Changing this value don't result in cfg/log file creation.
			 */
			lgs_cfgupd_list_create(LOG_STREAM_FILE_FORMAT,
				 value_str, &config_data);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_SYSTEM_HIGH_LIMIT)) {
			uint32_val = *(uint32_t *) value;
			snprintf(uint32_str, 20, "%u", uint32_val);
			mailbox_lim_upd = true;
			lgs_cfgupd_list_create(LOG_STREAM_SYSTEM_HIGH_LIMIT,
				uint32_str, &config_data);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_SYSTEM_LOW_LIMIT)) {
			uint32_val = *(uint32_t *) value;
			snprintf(uint32_str, 20, "%u", uint32_val);
			mailbox_lim_upd = true;
			lgs_cfgupd_list_create(LOG_STREAM_SYSTEM_LOW_LIMIT,
				uint32_str, &config_data);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_APP_HIGH_LIMIT)) {
			uint32_val = *(uint32_t *) value;
			snprintf(uint32_str, 20, "%u", uint32_val);
			mailbox_lim_upd = true;
			lgs_cfgupd_list_create(LOG_STREAM_APP_HIGH_LIMIT,
				uint32_str, &config_data);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_APP_LOW_LIMIT)) {
			uint32_val = *(uint32_t *) value;
			snprintf(uint32_str, 20, "%u", uint32_val);
			mailbox_lim_upd = true;
			lgs_cfgupd_list_create(LOG_STREAM_APP_LOW_LIMIT,
				uint32_str, &config_data);
		} else if (!strcmp(attribute->attrName, LOG_MAX_LOGRECSIZE)) {
			uint32_val = *(SaUint32T *) value;
			snprintf(uint32_str, 20, "%u", uint32_val);
			lgs_cfgupd_list_create(LOG_MAX_LOGRECSIZE,
				 uint32_str, &config_data);
		} else if (!strcmp(attribute->attrName, LOG_FILE_IO_TIMEOUT)) {
			uint32_val = *(SaUint32T *) value;
			snprintf(uint32_str, 20, "%u", uint32_val);
			lgs_cfgupd_list_create(LOG_FILE_IO_TIMEOUT,
			   uint32_str, &config_data);
		}

		attrMod = opdata->param.modify.attrMods[i++];
	}

	/* Update configuration data store */
	rc = lgs_cfg_update(&config_data);
	if (rc == -1) {
		LOG_ER("%s lgs_cfg_update Fail", __FUNCTION__);
		osafassert(0);
	}

	/* Update mailbox limits if any of them has changed
	 * Note: Must be done after configuration data store is updated
	 */
	if (mailbox_lim_upd == true) {
		TRACE("\tUpdating mailbox limits");
		(void) lgs_configure_mailbox();
	}

	/* Check-point changes */
	(void) ckpt_lgs_cfg(root_dir_chg_flag, &config_data);

	lgs_trace_config();

	/* Cleanup and free cfg buffer */
	if (config_data.ckpt_buffer_ptr != NULL)
		free(config_data.ckpt_buffer_ptr);
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
		osafassert(0);
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
	int n = 0;

	TRACE_ENTER();

	while (ccb->param.create.attrValues[i] != NULL) {
		if (!strncmp(ccb->param.create.attrValues[i]->attrName, "safLgStrCfg",
				sizeof("safLgStrCfg"))) {
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
				n = snprintf((*stream)->fileName, NAME_MAX, "%s", *((char **) value));
				if (n >= NAME_MAX) {
					LOG_ER("Error: saLogStreamFileName > NAME_MAX");
					osafassert(0);
				}
				TRACE("fileName: %s", (*stream)->fileName);
			} else if (!strcmp(ccb->param.create.attrValues[i]->attrName, "saLogStreamPathName")) {
				n = snprintf((*stream)->pathName, PATH_MAX, "%s", *((char **) value));
				if (n >= PATH_MAX) {
					LOG_ER("Error: Path name size > PATH_MAX");
					osafassert(0);
				}
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
					logFileFormat = (char *) lgs_cfg_get(LGS_IMM_LOG_STREAM_FILE_FORMAT);
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

	if ((*stream)->logFileFormat == NULL) {
		/* If passing NULL to log file format, use default value */
		const char* logFileFormat = (char *) lgs_cfg_get(LGS_IMM_LOG_STREAM_FILE_FORMAT);
		(*stream)->logFileFormat = strdup(logFileFormat);
	}

	/* Update creation timestamp */
	(void) immutil_update_one_rattr(lgs_cb->immOiHandle, (const char*) objectName.value,
			"saLogStreamCreationTimestamp", SA_IMM_ATTR_SATIMET,
			&(*stream)->creationTimeStamp);

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
		log_stream_open_fileinit(stream);
		ckpt_stream_open(stream);
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
	char current_logfile_name[NAME_MAX];
	bool new_cfg_file_needed = false;
	int n = 0;
	struct timespec curtime_tspec;
	char *fileName;
	bool modify = false;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	stream = log_stream_get_by_name((char*)opdata->objectName.value);
	osafassert(stream);

	n = snprintf(current_logfile_name, NAME_MAX, "%s", stream->logFileCurrent);
	if (n >= NAME_MAX) {
		LOG_ER("Error: a. File name > NAME_MAX");
		osafassert(0);		
	}

	attrMod = opdata->param.modify.attrMods[i++];
	while (attrMod != NULL) {
		void *value;
		const SaImmAttrValuesT_2 *attribute = &attrMod->modAttr;

		TRACE("attribute %s", attribute->attrName);

		value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "saLogStreamFileName")) {
			fileName = *((char **)value);
			modify = true;
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
			LOG_ER("Error: Unknown attribute name");
			osafassert(0);
		}

		attrMod = opdata->param.modify.attrMods[i++];
	}

	osaf_clock_gettime(CLOCK_REALTIME, &curtime_tspec);
	time_t cur_time = curtime_tspec.tv_sec;
	const char *root_path = lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY);
	if (new_cfg_file_needed) {
		int rc;
		if ((rc = log_stream_config_change(LGS_STREAM_CREATE_FILES,
				root_path, stream,
				current_logfile_name, &cur_time))
				!= 0) {
			LOG_ER("log_stream_config_change failed: %d", rc);
		}
	}

	/* Fix for ticket #1346 */
	if (modify) {
		int rc;
		if ((rc = log_stream_config_change(!LGS_STREAM_CREATE_FILES,
			root_path, stream, current_logfile_name, &cur_time))
		    != 0) {
			LOG_ER("log_stream_config_change failed: %d", rc);
		}

		n = snprintf(stream->fileName, NAME_MAX, "%s", fileName);
		if (n >= NAME_MAX) {
			LOG_ER("Error: File name (%zu) > NAME_MAX (%d)", strlen(fileName), (int) NAME_MAX);
			osafassert(0);
		}
		if ((rc = lgs_create_config_file_h(root_path, stream)) != 0) {
			LOG_ER("lgs_create_config_file_h failed: %d", rc);
		}

		char *current_time = lgs_get_time(&cur_time);
		sprintf(stream->logFileCurrent, "%s_%s", stream->fileName, current_time);

		// Create the new log file based on updated configuration
		*stream->p_fd = log_file_open(root_path,
			stream, stream->logFileCurrent, NULL);
	}

	/* Checkpoint to standby LOG server */
	/* Save time change was done for standby */
	stream->act_last_close_timestamp = cur_time;
	ckpt_stream_config(stream);

	TRACE_LEAVE();
}

static void stream_ccb_apply_delete(const CcbUtilOperationData_t *opdata)
{
	log_stream_t *stream;
	struct timespec closetime_tspec;
	osaf_clock_gettime(CLOCK_REALTIME, &closetime_tspec);
	time_t file_closetime = closetime_tspec.tv_sec;
	
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	stream = log_stream_get_by_name((char *) opdata->objectName.value);

	/* Checkpoint to standby LOG server */
	ckpt_stream_close(stream, file_closetime);
	log_stream_close(&stream, &file_closetime);

	TRACE_LEAVE();
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
 * IMM requests us to update a non cached runtime attribute.
 * Can be non cached attribute in stream runtime object or
 * runtime configuration object
 *
 * @param immOiHandle[in]
 * @param objectName[in]
 * @param attributeNames[in]
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT rtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
		const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	SaAisErrorT rc = SA_AIS_ERR_FAILED_OPERATION;
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE_ENTER2("%s", objectName->value);

	if (lgs_cb->ha_state != SA_AMF_HA_ACTIVE) {
		LOG_ER("admin op callback in applier");
		goto done;
	}

	/* Handle configuration runtime object */
	if (strncmp((char *) objectName->value, LGS_CFG_RUNTIME_OBJECT,
			objectName->length) == 0) {
		/* Handle Runtome configuration object */
		conf_runtime_obj_hdl(immOiHandle, attributeNames);
	} else {

		/* Handle stream object if valid
		 */
		log_stream_t *stream = log_stream_get_by_name((char *)objectName->value);
		if (stream == NULL) {
			LOG_ER("%s: stream %s not found", __FUNCTION__, objectName->value);
			goto done;
		}

		while ((attributeName = attributeNames[i++]) != NULL) {
			TRACE("Attribute %s", attributeName);
			if (!strcmp(attributeName, "saLogStreamNumOpeners")) {
				(void)immutil_update_one_rattr(immOiHandle,
					(char *)objectName->value,
					attributeName, SA_IMM_ATTR_SAUINT32T,
					&stream->numOpeners);
			} else if (!strcmp(attributeName, "logStreamDiscardedCounter")) {
				(void) immutil_update_one_rattr(immOiHandle,
					(char *) objectName->value,
					attributeName, SA_IMM_ATTR_SAUINT64T,
					&stream->filtered);
			} else {
				LOG_ER("%s: unknown attribute %s",
					__FUNCTION__, attributeName);
				goto done;
			}
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
 *
 * @param dn
 * @param in_stream
 * @param stream_id
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT stream_create_and_configure(const char *dn,
		log_stream_t **in_stream, int stream_id, SaImmAccessorHandleT accessorHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaNameT objectName;
	SaImmAttrValuesT_2 *attribute;
	SaImmAttrValuesT_2 **attributes;
	int i = 0;
	log_stream_t *stream;
	char *attribute_names[] = {
		"saLogStreamFileName",
		"saLogStreamPathName",
		"saLogStreamMaxLogFileSize",
		"saLogStreamFixedLogRecordSize",
		"saLogStreamLogFullAction",
		"saLogStreamLogFullHaltThreshold",
		"saLogStreamMaxFilesRotated",
		"saLogStreamLogFileFormat",
		"saLogStreamSeverityFilter",
		"saLogStreamCreationTimestamp",
		NULL
	};
	
	TRACE_ENTER2("(%s)", dn);

	strncpy((char *)objectName.value, dn, SA_MAX_NAME_LENGTH);
	objectName.length = strlen((char *)objectName.value);

	*in_stream = stream = log_stream_new_2(&objectName, stream_id);

	if (stream == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	if (strcmp(dn, SA_LOG_STREAM_ALARM) == 0)
		stream->streamType = STREAM_TYPE_ALARM;
	else if (strcmp(dn , SA_LOG_STREAM_NOTIFICATION) == 0)
		stream->streamType = STREAM_TYPE_NOTIFICATION;
	else if (strcmp(dn , SA_LOG_STREAM_SYSTEM) == 0)
		stream->streamType = STREAM_TYPE_SYSTEM;
	else
		stream->streamType = STREAM_TYPE_APPLICATION;

	/* Get all attributes of the object */
	if (immutil_saImmOmAccessorGet_2(accessorHandle, &objectName,
			attribute_names, &attributes) != SA_AIS_OK) {
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

				if (stream->streamType == STREAM_TYPE_APPLICATION) {
					logFileFormat = (char *) lgs_cfg_get(LGS_IMM_LOG_STREAM_FILE_FORMAT);
				} else {
					strcpy(logFileFormat, log_file_format[stream->streamType]);
				}
			}
			stream->logFileFormat = strdup(logFileFormat);
			TRACE("logFileFormat: %s", stream->logFileFormat);
		} else if (!strcmp(attribute->attrName, "saLogStreamSeverityFilter")) {
			stream->severityFilter = *((SaUint32T *)value);
			TRACE("severityFilter: %u", stream->severityFilter);
		}
	}

	if (stream->logFileFormat == NULL) {
		if (stream->streamType == STREAM_TYPE_APPLICATION) {
			const char* logFileFormat = (char *) lgs_cfg_get(LGS_IMM_LOG_STREAM_FILE_FORMAT);
			stream->logFileFormat = strdup(logFileFormat);
		} else {
			stream->logFileFormat = strdup(log_file_format[stream->streamType]);
		}
	}

 done:
	TRACE_LEAVE();
	return rc;
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
 * Retrieve the LOG stream configuration from IMM using the
 * IMM-OM interface and initialize the corresponding information
 * in the LOG control block. Initialize the LOG IMM-OI
 * interface. Become class implementer.
 */
SaAisErrorT lgs_imm_create_configStream(lgs_cb_t *cb)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaAisErrorT om_rc;
	log_stream_t *stream;
	SaImmHandleT omHandle;
	SaImmAccessorHandleT accessorHandle;
	SaVersionT immVersion = { 'A', 2, 1 };
	SaImmSearchHandleT immSearchHandle;
	SaImmSearchParametersT_2 objectSearch;
	SaImmAttrValuesT_2 **attributes;
	int streamId = 0;
	int errorsAreFatal;
	SaNameT objectName;
	SaNameT root_name = {
		.value = "safApp=safLogService",
		.length = sizeof("safApp=safLogService")
	};


	TRACE_ENTER();

	(void)immutil_saImmOmInitialize(&omHandle, NULL, &immVersion);
	(void)immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle);

	/* Search for all objects of class "SaLogStreamConfig" */
	objectSearch.searchOneAttr.attrName = "safLgStrCfg";
	objectSearch.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	objectSearch.searchOneAttr.attrValue = NULL;

	if ((om_rc = immutil_saImmOmSearchInitialize_2(omHandle, &root_name,
			SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR,
			&objectSearch, NULL, /* Get no attributes */
			&immSearchHandle)) == SA_AIS_OK) {

		while (immutil_saImmOmSearchNext_2(immSearchHandle, &objectName, &attributes) == SA_AIS_OK) {
			if ((rc = stream_create_and_configure((char*) objectName.value,
					&stream, streamId, accessorHandle)) != SA_AIS_OK) {
				LOG_ER("stream_create_and_configure failed %d", rc);
				goto done;
			}
			streamId += 1;
		}
	}

	/* 1.Become implementer
	 * 2.Update creation timestamp for all configure object, must be object implementer first
	 * 3.Open all streams
	 *     Config file and log file will be created. If this fails we give up
	 *     without returning any error. A new attempt to create the files will
	 *     be done when trying to write a log record to the stream.
	 */
	immutilWrapperProfile.nTries = 250; /* After loading,allow missed sync of large data to complete */

	(void)immutil_saImmOiClassImplementerSet(cb->immOiHandle, "SaLogStreamConfig");

	immutilWrapperProfile.nTries = 20; /* Reset retry time to more normal value. */

	stream = log_stream_getnext_by_name(NULL);
	while (stream != NULL) {
		(void)immutil_update_one_rattr(cb->immOiHandle, stream->name,
					       "saLogStreamCreationTimestamp", SA_IMM_ATTR_SATIMET,
					       &stream->creationTimeStamp);

		log_stream_open_fileinit(stream);
		stream = log_stream_getnext_by_name(stream->name);
	}

 done:
	/* Do not abort if error when finalizing */
	errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
	immutilWrapperProfile.errorsAreFatal = 0;	/* Disable immutil abort */
	om_rc = immutil_saImmOmAccessorFinalize(accessorHandle);
	if (om_rc != SA_AIS_OK) {
		LOG_NO("%s immutil_saImmOmAccessorFinalize() Fail %d",__FUNCTION__, om_rc);
	}
	om_rc = immutil_saImmOmSearchFinalize(immSearchHandle);
	if (om_rc != SA_AIS_OK) {
		LOG_NO("%s immutil_saImmOmSearchFinalize() Fail %d",__FUNCTION__, om_rc);
	}
	om_rc = immutil_saImmOmFinalize(omHandle);
	if (om_rc != SA_AIS_OK) {
		LOG_NO("%s immutil_saImmOmFinalize() Fail %d",__FUNCTION__, om_rc);
	}
	immutilWrapperProfile.errorsAreFatal = errorsAreFatal; /* Enable again */

	TRACE_LEAVE();
	return rc;
}

/**
 * Initialize the OI interface and get a selection object.
 * Become OI for safLogService if Active
 *
 * @param immOiHandle[out]
 * @param immSelectionObject[out]
 */
void lgs_imm_init_OI_handle(SaImmOiHandleT *immOiHandle,
	SaSelectionObjectT *immSelectionObject)
{
	SaAisErrorT rc;
	int msecs_waited;

	TRACE_ENTER();

	/* Initialize IMM OI service */
	msecs_waited = 0;
	rc = saImmOiInitialize_2(immOiHandle, &callbacks, &immVersion);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_60s)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiInitialize_2(immOiHandle, &callbacks, &immVersion);
	}
	if (rc != SA_AIS_OK) {
		lgs_exit("saImmOiInitialize_2 failed", SA_AMF_COMPONENT_RESTART);
	}

	/* Get selection object for event handling */
	msecs_waited = 0;
	rc = saImmOiSelectionObjectGet(*immOiHandle, immSelectionObject);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_10s)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiSelectionObjectGet(*immOiHandle, immSelectionObject);
	}
	if (rc != SA_AIS_OK) {
		lgs_exit("saImmOiSelectionObjectGet failed", SA_AMF_COMPONENT_RESTART);
	}

	TRACE_LEAVE();
}

/**
 * Does the sequence of setting an implementer name and class implementer
 * 
 * @param immOiHandle[in]
 * @return SaAisErrorT
 */
static SaAisErrorT imm_impl_set_sequence(SaImmOiHandleT immOiHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	int msecs_waited;

	TRACE_ENTER();

	/* Become object implementer
	 */
	msecs_waited = 0;
	rc = saImmOiImplementerSet(immOiHandle, implementerName);
	while (((rc == SA_AIS_ERR_TRY_AGAIN) || (rc == SA_AIS_ERR_EXIST)) &&
			(msecs_waited < max_waiting_time_60s)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiImplementerSet(immOiHandle, implementerName);
	}
	if (rc != SA_AIS_OK) {
		TRACE("saImmOiImplementerSet failed %s", saf_error(rc));
		goto done;
	}

	/* 
	 * Become class implementer for the OpenSafLogConfig class if it exists
	 * Become class implementer for the SaLogStreamConfig class
	 */
	if ( true == *(bool*) lgs_cfg_get(LGS_IMM_LOG_OPENSAFLOGCONFIG_CLASS_EXIST)) {
		msecs_waited = 0;
		rc = saImmOiClassImplementerSet(immOiHandle, "OpenSafLogConfig");
		while (((rc == SA_AIS_ERR_TRY_AGAIN) || (rc == SA_AIS_ERR_EXIST))
				&& (msecs_waited < max_waiting_time_60s)) {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;
			rc = saImmOiClassImplementerSet(immOiHandle, "OpenSafLogConfig");
		}
		if (rc != SA_AIS_OK) {
			TRACE("saImmOiClassImplementerSet OpenSafLogConfig failed %s", saf_error(rc));
			goto done;
		}
	}

	msecs_waited = 0;
	rc = saImmOiClassImplementerSet(immOiHandle, "SaLogStreamConfig");
	while (((rc == SA_AIS_ERR_TRY_AGAIN) || (rc == SA_AIS_ERR_EXIST))
			&& (msecs_waited < max_waiting_time_60s)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saImmOiClassImplementerSet(immOiHandle, "SaLogStreamConfig");
	}
	if (rc != SA_AIS_OK) {
		TRACE("saImmOiClassImplementerSet SaLogStreamConfig failed %u", rc);
		goto done;
	}
	
done:
	return rc;
	TRACE_LEAVE();
}

/**
 * Set implementer name and become class implementer.
 * This function will block until done.
 *
 * @param cb
 */
void lgs_imm_impl_set(SaImmOiHandleT immOiHandle)
{
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER();

	rc = imm_impl_set_sequence(immOiHandle);
	if (rc != SA_AIS_OK) {
		lgs_exit("Becoming OI implementer failed", SA_AMF_COMPONENT_RESTART);
	}

	TRACE_LEAVE();
}

/**
 * Thread
 * Restore object and class implementer/applier.
 *
 * @param _cb[in]
 */
static void *imm_impl_init_thread(void *_cb)
{
	lgs_cb_t *cb = (lgs_cb_t *)_cb;
	SaSelectionObjectT immSelectionObject = 0;
	SaImmOiHandleT immOiHandle = 0;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER();

	/* Initialize handles and become implementer */
	lgs_imm_init_OI_handle(&immOiHandle, &immSelectionObject);
	rc = imm_impl_set_sequence(immOiHandle);
	if (rc != SA_AIS_OK) {
		lgs_exit("Becoming OI implementer failed", SA_AMF_COMPONENT_RESTART);
	}

	/* Store handle and selection object.
	 * Protect if the poll loop in main is released during the storage
	 * sequence.
	 */
	osaf_mutex_lock_ordie(&lgs_OI_init_mutex);
	cb->immSelectionObject = immSelectionObject;
	cb->immOiHandle = immOiHandle;
	osaf_mutex_unlock_ordie(&lgs_OI_init_mutex);

	/* Activate the poll loop in main()
	 * This will reinstall IMM poll event handling
	 */
	lgsv_lgs_evt_t *lgsv_evt;
	lgsv_evt = calloc(1, sizeof(lgsv_lgs_evt_t));
	osafassert(lgsv_evt);
	lgsv_evt->evt_type = LGSV_EVT_NO_OP;
	if (m_NCS_IPC_SEND(&lgs_mbx, lgsv_evt, LGS_IPC_PRIO_CTRL_MSGS) !=
		NCSCC_RC_SUCCESS) {
		LOG_WA("imm_reinit_thread failed to send IPC message to main thread");
		/*
		 * Se no reason why this would happen. But if it does at least there
		 * is something in the syslog. The main thread should still pick up
		 * the new imm FD when there is a healthcheck, but it could take
		 *minutes.
		 */
		free(lgsv_evt);
	}

	TRACE_LEAVE();
	return NULL;
}

/**
 * In a separate thread:
 * Initiate IMM OI handle and selection object
 * Create imm implementer for log IMM objects
 * When complete:
 * Store the new handle and selection object in the cb store.
 * Activate the poll loop in main() by sending an empty mailbox message
 *
 * @param cb[out]
 */
void lgs_imm_impl_reinit_nonblocking(lgs_cb_t *cb)
{
	pthread_t thread;
	pthread_attr_t attr;

	TRACE_ENTER();

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, imm_impl_init_thread, cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	pthread_attr_destroy(&attr);

	TRACE_LEAVE();
}
