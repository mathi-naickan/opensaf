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

#include <string.h>
#include <saf_error.h>
#include "lga.h"

#include "lga_state.h"

#define NCS_SAF_MIN_ACCEPT_TIME 10

/* Macro to validate the dispatch flags */
#define m_DISPATCH_FLAG_IS_VALID(flag) \
   ( (SA_DISPATCH_ONE == flag) || \
     (SA_DISPATCH_ALL == flag) || \
     (SA_DISPATCH_BLOCKING == flag) )

#define LGSV_NANOSEC_TO_LEAPTM 10000000
/**
 * Temporary maximum log file name length to avoid LOG client sends
 * log file name too long (big data) to LOG service.
 *
 * The real limit check will be done by service side.
 */
#define LGA_FILE_LENGTH_TEMP_LIMIT 2048

/* The main controle block */
lga_cb_t lga_cb = {
	.cb_lock = PTHREAD_MUTEX_INITIALIZER,
	.lga_state = LGA_NORMAL,
	.lgs_state = LGS_START
};

static void populate_open_params(lgsv_stream_open_req_t *open_param,
				 const SaNameT *logStreamName,
				 lga_client_hdl_rec_t *hdl_rec,
				 SaLogFileCreateAttributesT_2 *logFileCreateAttributes,
				 SaLogStreamOpenFlagsT logStreamOpenFlags)
{
	TRACE_ENTER();
	open_param->client_id = hdl_rec->lgs_client_id;
	open_param->lstr_name = *logStreamName;

	if (logFileCreateAttributes == NULL ||
	    strcmp((const char *)logStreamName->value, SA_LOG_STREAM_NOTIFICATION) == 0 ||
	    strcmp((const char *)logStreamName->value, SA_LOG_STREAM_ALARM) == 0 ||
	    strcmp((const char *)logStreamName->value, SA_LOG_STREAM_SYSTEM) == 0) {
		open_param->logFileFmt = NULL;
		open_param->logFileFmtLength = 0;
		open_param->maxLogFileSize = 0;
		open_param->maxLogRecordSize = 0;
		open_param->haProperty = SA_FALSE;
		open_param->logFileFullAction = 0;
		open_param->maxFilesRotated = 0;
		open_param->lstr_open_flags = 0;
	} else {
		/* Server will assign a def fmt string if needed (logFileFmt==NULL) */
		open_param->logFileFmt = logFileCreateAttributes->logFileFmt;
		open_param->maxLogFileSize = logFileCreateAttributes->maxLogFileSize;
		open_param->maxLogRecordSize = logFileCreateAttributes->maxLogRecordSize;
		open_param->haProperty = logFileCreateAttributes->haProperty;
		open_param->logFileFullAction = logFileCreateAttributes->logFileFullAction;
		open_param->maxFilesRotated = logFileCreateAttributes->maxFilesRotated;
		open_param->lstr_open_flags = logStreamOpenFlags;
	}

	TRACE_LEAVE();
}

/**
 * 
 * 
 * @return SaTimeT
 */
static SaTimeT setLogTime(void)
{
	struct timeval currentTime;
	SaTimeT logTime;

	/* Fetch current system time for time stamp value */
	(void)gettimeofday(&currentTime, 0);

	logTime = ((unsigned)currentTime.tv_sec * 1000000000ULL) + ((unsigned)currentTime.tv_usec * 1000ULL);

	return logTime;
}

/***************************************************************************
 * 8.4.1
 *
 * saLogInitialize()
 *
 * This function initializes the Log Service. 
 *
 * Parameters
 *
 * logHandle - [out] A pointer to the handle for this initialization of the
 *                   Log Service.
 * callbacks - [in] A pointer to the callbacks structure that contains the
 *                  callback functions of the invoking process that the
 *                  Log Service may invoke.
 * version - [in] A pointer to the version of the Log Service that the
 *                invoking process is using.
 *
 ***************************************************************************/
SaAisErrorT saLogInitialize(SaLogHandleT *logHandle, const SaLogCallbacksT *callbacks, SaVersionT *version)
{
	lga_client_hdl_rec_t *lga_hdl_rec;
	lgsv_msg_t i_msg, *o_msg;
	SaAisErrorT ais_rc = SA_AIS_OK;
	int rc;
	uint32_t client_id = 0;

	TRACE_ENTER();

	/* Verify parameters (log handle and that version is given) */
	if ((logHandle == NULL) || (version == NULL)) {
		TRACE("version or handle FAILED");
		ais_rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* validate the version */
	if ((version->releaseCode == LOG_RELEASE_CODE) && (version->majorVersion <= LOG_MAJOR_VERSION)) {
		version->majorVersion = LOG_MAJOR_VERSION;
		version->minorVersion = LOG_MINOR_VERSION;
	} else {
		TRACE("version FAILED, required: %c.%u.%u, supported: %c.%u.%u\n",
		      version->releaseCode, version->majorVersion, version->minorVersion,
		      LOG_RELEASE_CODE, LOG_MAJOR_VERSION, LOG_MINOR_VERSION);
		version->releaseCode = LOG_RELEASE_CODE;
		version->majorVersion = LOG_MAJOR_VERSION;
		version->minorVersion = LOG_MINOR_VERSION;
		lga_shutdown_after_last_client();
		ais_rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	/***
	 * Handle states
	 * Synchronize with mds and recovery thread (mutex)
	 * NOTE: Nothing to handle if recovery state 1
	 */
	pthread_mutex_lock(&lga_cb.cb_lock);
	lgs_state_t lgs_state = lga_cb.lgs_state;
	lga_state_t lga_state = lga_cb.lga_state;
	pthread_mutex_unlock(&lga_cb.cb_lock);

	if (lgs_state == LGS_NO_ACTIVE) {
		/* We have a server but it is temporary unavailable. Client may
		 * try again
		 */
		TRACE("%s LGS no active", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	if (lga_state == LGA_NO_SERVER) {
		/* We have no server and cannot initialize.
		 * The client may try again
		 */
		TRACE("%s No server", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	if (lga_state == LGA_RECOVERY2) {
		/* Auto recovery is ongoing. We have to wait for it to finish.
		 * The client may try again
		 */
		TRACE("%s LGA auto recovery ongoing (2)", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	/***
	 * Do Initiate. It's ok to initiate in recovery state 1 since we have a
	 * server and is not conflicting with auto recovery
	 */

	/* Initiate the client in the agent and if first client also start MDS
	 * etc.
	 */
	if ((rc = lga_startup(&lga_cb)) != NCSCC_RC_SUCCESS) {
		TRACE("lga_startup FAILED: %u", rc);
		ais_rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	/* Populate the message to be sent to the LGS */
	memset(&i_msg, 0, sizeof(lgsv_msg_t));
	i_msg.type = LGSV_LGA_API_MSG;
	i_msg.info.api_info.type = LGSV_INITIALIZE_REQ;
	i_msg.info.api_info.param.init.version = *version;

	/* Send a message to LGS to obtain a client_id/server ref id which is cluster
	 * wide unique.
	 */
	rc = lga_mds_msg_sync_send(&lga_cb, &i_msg, &o_msg, LGS_WAIT_TIME, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		lga_shutdown_after_last_client();
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

    /** Make sure the LGS return status was SA_AIS_OK 
     **/
	ais_rc = o_msg->info.api_resp_info.rc;
	if (SA_AIS_OK != ais_rc) {
		TRACE("%s LGS error response %s", __FUNCTION__, saf_error(ais_rc));
		goto err;
	}

    /** Store the cient ID returned by
     *  the LGS to pass into the next
     *  routine
     **/
	client_id = o_msg->info.api_resp_info.param.init_rsp.client_id;

	/* create the hdl record & store the callbacks */
	lga_hdl_rec = lga_hdl_rec_add(&lga_cb, callbacks, client_id);
	if (lga_hdl_rec == NULL) {
		ais_rc = SA_AIS_ERR_NO_MEMORY;
		goto err;
	}

	/* pass the handle value to the appl */
	*logHandle = lga_hdl_rec->local_hdl;

 err:
	/* free up the response message */
	if (o_msg)
		lga_msg_destroy(o_msg);

	if (ais_rc != SA_AIS_OK) {
		TRACE_2("%s LGA INIT FAILED\n", __FUNCTION__);
		lga_shutdown_after_last_client();
	}

 done:
	TRACE_LEAVE2("client_id = %d", client_id);
	return ais_rc;
}

/***************************************************************************
 * 8.4.2
 *
 * saLogSelectionObjectGet()
 *
 * The saLogSelectionObjectGet() function returns the operating system handle
 * selectionObject, associated with the handle logHandle, allowing the
 * invoking process to ascertain when callbacks are pending. This function
 * allows a process to avoid repeated invoking saLogDispatch() to see if
 * there is a new event, thus, needlessly consuming CPU time. In a POSIX
 * environment the system handle could be a file descriptor that is used with
 * the poll() system call to detect incoming callbacks.
 *
 *
 * Parameters
 *
 * logHandle - [in]
 *   A pointer to the handle, obtained through the saLogInitialize() function,
 *   designating this particular initialization of the Log Service.
 *
 * selectionObject - [out]
 *   A pointer to the operating system handle that the process may use to
 *   detect pending callbacks.
 *
 ***************************************************************************/
SaAisErrorT saLogSelectionObjectGet(SaLogHandleT logHandle, SaSelectionObjectT *selectionObject)
{
	SaAisErrorT rc = SA_AIS_OK;
	lga_client_hdl_rec_t *hdl_rec;
	NCS_SEL_OBJ sel_obj;

	TRACE_ENTER();

	if (selectionObject == NULL) {
		TRACE("selectionObject = NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, logHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* Obtain the selection object from the IPC queue */
	sel_obj = m_NCS_IPC_GET_SEL_OBJ(&hdl_rec->mbx);

	/* everything's fine.. pass the sel fd to the appl */
	*selectionObject = (SaSelectionObjectT)m_GET_FD_FROM_SEL_OBJ(sel_obj);

	/* return hdl rec */
	ncshm_give_hdl(logHandle);

 done:
	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 8.4.3
 *
 * saLogDispatch()
 *
 * The saLogDispatch() function invokes, in the context of the calling thread,
 * one or all of the pending callbacks for the handle logHandle.
 *
 *
 * Parameters
 *
 * logHandle - [in]
 *   A pointer to the handle, obtained through the saLogInitialize() function,
 *   designating this particular initialization of the Log Service.
 *
 * dispatchFlags - [in]
 *   Flags that specify the callback execution behavior of the the
 *   saLogDispatch() function, which have the values SA_DISPATCH_ONE,
 *   SA_DISPATCH_ALL or SA_DISPATCH_BLOCKING, as defined in Section 3.3.8.
 *
 ***************************************************************************/
SaAisErrorT saLogDispatch(SaLogHandleT logHandle, SaDispatchFlagsT dispatchFlags)
{
	lga_client_hdl_rec_t *hdl_rec;
	SaAisErrorT rc;

	TRACE_ENTER();

	if (!m_DISPATCH_FLAG_IS_VALID(dispatchFlags)) {
		TRACE("Invalid dispatchFlags");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, logHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl logHandle ");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if ((rc = lga_hdl_cbk_dispatch(&lga_cb, hdl_rec, dispatchFlags)) != SA_AIS_OK)
		TRACE("LGA_DISPATCH_FAILURE");

	ncshm_give_hdl(logHandle);

 done:
	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
 * Finalize
 * API function and help functions
 ******************************************************************************/

/**
 * Create and send a Finalize message to the server
 *
 * @param hdl_rec
 * @return AIS return code
 */
static SaAisErrorT send_Finalize_msg(lga_client_hdl_rec_t *hdl_rec)
{
	uint32_t mds_rc;
	lgsv_msg_t msg, *o_msg = NULL;
	SaAisErrorT ais_rc = SA_AIS_OK;

	TRACE_ENTER();

	memset(&msg, 0, sizeof(lgsv_msg_t));
	msg.type = LGSV_LGA_API_MSG;
	msg.info.api_info.type = LGSV_FINALIZE_REQ;
	msg.info.api_info.param.finalize.client_id = hdl_rec->lgs_client_id;

	mds_rc = lga_mds_msg_sync_send(
		&lga_cb, &msg,
		&o_msg,
		LGS_WAIT_TIME,
		MDS_SEND_PRIORITY_MEDIUM
		);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		ais_rc = SA_AIS_ERR_TIMEOUT;
		TRACE("lga_mds_msg_sync_send FAILED: %s", saf_error(ais_rc));
		goto done;
	default:
		TRACE("lga_mds_msg_sync_send FAILED: %s", saf_error(ais_rc));
		ais_rc = SA_AIS_ERR_NO_RESOURCES;
		goto done;
	}

	if (o_msg != NULL) {
		ais_rc = o_msg->info.api_resp_info.rc;
		lga_msg_destroy(o_msg);
	} else
		ais_rc = SA_AIS_ERR_NO_RESOURCES;

	done:

	TRACE_LEAVE();
	return ais_rc;
}

/***************************************************************************
 * 8.4.4
 *
 * saLogFinalize()
 *
 * The saLogFinalize() function closes the association, represented by the
 * logHandle parameter, between the process and the Log Service.
 * It may free up resources.
 *
 * This function cannot be invoked before the process has invoked the
 * corresponding saLogInitialize() function for the Log Service. 
 * After this function is invoked, the selection object is no longer valid.
 * Moreover, the Log Service is unavailable for further use unless it is 
 * reinitialized using the saLogInitialize() function.
 *
 * Parameters
 *
 * logHandle - [in]
 *   A pointer to the handle, obtained through the saLogInitialize() function,
 *   designating this particular initialization of the Log Service.
 *
 ***************************************************************************/
SaAisErrorT saLogFinalize(SaLogHandleT logHandle)
{
	lga_client_hdl_rec_t *hdl_rec;
	SaAisErrorT ais_rc = SA_AIS_OK;
	uint32_t rc;

	TRACE_ENTER();

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, logHandle);
	if (hdl_rec == NULL) {
		TRACE("%s ncshm_take_hdl failed", __FUNCTION__);
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/***
	 * Handle states
	 * Synchronize with mds and recovery thread (mutex)
	 */
	pthread_mutex_lock(&lga_cb.cb_lock);
	lgs_state_t lgs_state = lga_cb.lgs_state;
	lga_state_t lga_state = lga_cb.lga_state;
	pthread_mutex_unlock(&lga_cb.cb_lock);

	if (lgs_state == LGS_NO_ACTIVE) {
		/* We have a server but it is temporary unavailable. Client may
		 * try again
		 */
		TRACE("%s lgs_state = LGS no active", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

	if (lga_state == LGA_NO_SERVER) {
		/* We have no server but can still finalize client.
		 * In this situation no message to server is sent
		 */
		TRACE("%s lga_state = LGS down", __FUNCTION__);
		ais_rc = SA_AIS_OK;
		goto done_give_hdl;
	}

	if (lga_state == LGA_RECOVERY2) {
		/* Auto recovery is ongoing. We have to wait for it to finish.
		 * The client may try again
		 */
		TRACE("%s lga_state = LGA auto recovery ongoing (2)", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

	if (lga_state == LGA_RECOVERY1) {
		/* We are in recovery state 1. Client may or may not have been
		 * initialized. If initialized a finalize request must be sent
		 * to the server else the client is finalized in the agent only
		 */
		TRACE("%s lga_state = Recovery state (1)", __FUNCTION__);
		if (hdl_rec->initialized_flag == false) {
			TRACE("\t Client is not initialized");
			goto done_give_hdl;
		}
		TRACE("\t Client is initialized");
	}

	/***
	 * Populate & send the finalize message
	 * and make sure the finalize from the server
	 * end returned before deleting the local records.
	 */
	ais_rc = send_Finalize_msg(hdl_rec);

	if (ais_rc == SA_AIS_OK) {
		TRACE("%s delete_one_client", __FUNCTION__);
		(void) delete_one_client(&lga_cb.client_list, hdl_rec);
	}

 done_give_hdl:
	ncshm_give_hdl(logHandle);

	if (ais_rc == SA_AIS_OK) {
		rc = lga_shutdown_after_last_client();
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("lga_shutdown ");
	}

 done:
	TRACE_LEAVE2("ais_rc = %s", saf_error(ais_rc));
	return ais_rc;
}

/******************************************************************************
 * Open a stream
 * API function and help functions
 ******************************************************************************/

/**
 * Check input parameters for opening a stream
 *
 * @param logStreamName
 * @param logFileCreateAttributes
 * @param logStreamOpenFlags
 * @param logStreamHandle
 * @param header_type
 * @return
 */
static SaAisErrorT validate_open_params(
	const SaNameT *logStreamName,
	const SaLogFileCreateAttributesT_2 *logFileCreateAttributes,
	SaLogStreamOpenFlagsT logStreamOpenFlags,
	SaLogStreamHandleT *logStreamHandle,
	uint32_t *header_type
	)
{
	size_t len;
	SaAisErrorT ais_rc = SA_AIS_OK;

	TRACE_ENTER();

	osafassert(header_type != NULL);

	if ((NULL == logStreamName) || (NULL == logStreamHandle)) {
		TRACE("SA_AIS_ERR_INVALID_PARAM => NULL pointer check");
		ais_rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}
	
	/* Note:
	 * logStreamName is of type SaNameT this means that length normally is not
	 * allowed to be > SA_MAX_NAME_LENGTH - 1 (255). SaNameT also means that
	 * value does not have to be '\0' terminated.
	 * HOWEVER:
	 * This parameter does not follow these rules. In this case value shall be
	 * '\0' terminated and length can be 256.
	 * In order to not break backwards compatibility this should not be changed.
	 */
	if (logStreamName->length >= SA_MAX_NAME_LENGTH) {
		TRACE("SA_AIS_ERR_INVALID_PARAM, logStreamName->length > SA_MAX_NAME_LENGTH");
		ais_rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* Check log stream input parameters */
	/* The well known log streams */
	if (strcmp((char *)logStreamName->value, SA_LOG_STREAM_ALARM) == 0 ||
	    strcmp((char *)logStreamName->value, SA_LOG_STREAM_NOTIFICATION) == 0 ||
	    strcmp((char *)logStreamName->value, SA_LOG_STREAM_SYSTEM) == 0) {
		/* SA_AIS_ERR_INVALID_PARAM, bullet 3 in SAI-AIS-LOG-A.02.01 
		   Section 3.6.1, Return Values */
		if ((NULL != logFileCreateAttributes) || (logStreamOpenFlags == SA_LOG_STREAM_CREATE)) {
			TRACE("SA_AIS_ERR_INVALID_PARAM, logStreamOpenFlags");
			return SA_AIS_ERR_INVALID_PARAM;
		}
		if (strcmp((const char *)logStreamName->value,	/* Well known log streams */
			   SA_LOG_STREAM_SYSTEM) == 0) {
			*header_type = (uint32_t)SA_LOG_GENERIC_HEADER;
		} else {
			*header_type = (uint32_t)SA_LOG_NTF_HEADER;
		}
	} else {		/* Application log stream */

		/* SA_AIS_ERR_INVALID_PARAM, bullet 1 in SAI-AIS-LOG-A.02.01 
		   Section 3.6.1, Return Values */

		if (logStreamOpenFlags > 1) {
			TRACE("logStreamOpenFlags invalid  when create");
			return SA_AIS_ERR_BAD_FLAGS;
		}

		if ((logStreamOpenFlags == SA_LOG_STREAM_CREATE)
		    && (logFileCreateAttributes == NULL)) {
			TRACE("logFileCreateAttributes == NULL, when create");
			return SA_AIS_ERR_INVALID_PARAM;
		}

		/* SA_AIS_ERR_INVALID_PARAM, bullet 2 in SAI-AIS-LOG-A.02.01 
		   Section 3.6.1, Return Values */
		if ((logStreamOpenFlags != SA_LOG_STREAM_CREATE)
		    && (logFileCreateAttributes != NULL)) {
			TRACE("logFileCreateAttributes defined when create");
			return SA_AIS_ERR_INVALID_PARAM;
		}

		/* SA_AIS_ERR_INVALID_PARAM, bullet 5 in SAI-AIS-LOG-A.02.01 
		   Section 3.6.1, Return Values */
		if (strncmp((const char *)logStreamName->value, "safLgStr=", 9) &&
				strncmp((const char *)logStreamName->value, "safLgStrCfg=", 12)) {
			TRACE("\"safLgStr=\" is missing in logStreamName");
			return SA_AIS_ERR_INVALID_PARAM;
		}

		/* SA_AIS_ERR_BAD_FLAGS */
		if (logStreamOpenFlags > (uint32_t)SA_LOG_STREAM_CREATE) {
			TRACE("logStreamOpenFlags");
			return SA_AIS_ERR_BAD_FLAGS;
		}

		/* Check logFileCreateAttributes */
		if (logFileCreateAttributes != NULL) {
			if (logFileCreateAttributes->logFileName == NULL) {
				TRACE("logFileCreateAttributes");
				return SA_AIS_ERR_INVALID_PARAM;
			}

			if (logFileCreateAttributes->maxLogFileSize == 0) {
				TRACE("maxLogFileSize");
				return SA_AIS_ERR_INVALID_PARAM;
			}

			if (logFileCreateAttributes->logFileFullAction < SA_LOG_FILE_FULL_ACTION_WRAP
			    || logFileCreateAttributes->logFileFullAction > SA_LOG_FILE_FULL_ACTION_ROTATE) {
				TRACE("logFileFullAction");
				return SA_AIS_ERR_INVALID_PARAM;
			}

			if (logFileCreateAttributes->haProperty > SA_TRUE) {
				TRACE("haProperty");
				return SA_AIS_ERR_INVALID_PARAM;
			}
			
			if(logFileCreateAttributes->maxLogRecordSize > logFileCreateAttributes->maxLogFileSize){
                                TRACE("maxLogRecordSize is greater than the maxLogFileSize");
                                return SA_AIS_ERR_INVALID_PARAM;
			}

			/* Verify that the fixedLogRecordSize is in valid range */
			if ((logFileCreateAttributes->maxLogRecordSize != 0) &&
				((logFileCreateAttributes->maxLogRecordSize < SA_LOG_MIN_RECORD_SIZE) ||
				(logFileCreateAttributes->maxLogRecordSize > SA_LOG_MAX_RECORD_SIZE))) {
				TRACE("maxLogRecordSize is invalid");
				return SA_AIS_ERR_INVALID_PARAM;
			}

			/* Validate maxFilesRotated just in case of SA_LOG_FILE_FULL_ACTION_ROTATE type */
			if ((logFileCreateAttributes->logFileFullAction == SA_LOG_FILE_FULL_ACTION_ROTATE) &&
				((logFileCreateAttributes->maxFilesRotated < 1) ||
				 (logFileCreateAttributes->maxFilesRotated > 127))) {
				TRACE("Invalid maxFilesRotated. Valid range = [1-127]");
				return SA_AIS_ERR_INVALID_PARAM;
			}
		}

		*header_type = (uint32_t)SA_LOG_GENERIC_HEADER;
	}

	/* Check implementation specific string length */
	if (NULL != logFileCreateAttributes) {
		len = strlen(logFileCreateAttributes->logFileName);
		if ((len == 0) || (len > LGA_FILE_LENGTH_TEMP_LIMIT)) {
			TRACE("logFileName is too long (max = %d)", LGA_FILE_LENGTH_TEMP_LIMIT);
			return SA_AIS_ERR_INVALID_PARAM;
		}
		if (logFileCreateAttributes->logFilePathName != NULL) {
			len = strlen(logFileCreateAttributes->logFilePathName);
			if ((len == 0) || (len > PATH_MAX)) {
				TRACE("logFilePathName");
				return SA_AIS_ERR_INVALID_PARAM;
			}
		}
	}

 done:
	TRACE_LEAVE();
	return ais_rc;
}

/**
 * API function for opening a stream
 * 
 * @param logHandle
 * @param logStreamName
 * @param logFileCreateAttributes
 * @param logStreamOpenFlags
 * @param timeOut
 * @param logStreamHandle
 * 
 * @return SaAisErrorT
 */
SaAisErrorT saLogStreamOpen_2(SaLogHandleT logHandle,
			      const SaNameT *logStreamName,
			      const SaLogFileCreateAttributesT_2 *logFileCreateAttributes,
			      SaLogStreamOpenFlagsT logStreamOpenFlags,
			      SaTimeT timeOut, SaLogStreamHandleT *logStreamHandle)
{
	lga_log_stream_hdl_rec_t *lstr_hdl_rec = NULL;
	lga_client_hdl_rec_t *hdl_rec;
	lgsv_msg_t msg, *o_msg = NULL;
	lgsv_stream_open_req_t *open_param;
	SaAisErrorT ais_rc;
	int rc = 0;
	uint32_t ncs_rc;
	uint32_t timeout;
	uint32_t log_stream_id;
	uint32_t log_header_type = 0;

	TRACE_ENTER();

	ais_rc = validate_open_params(logStreamName, logFileCreateAttributes,
				  logStreamOpenFlags, logStreamHandle, &log_header_type);
	if (ais_rc != SA_AIS_OK)
		goto done;

	/* retrieve log service hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, logHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/***
	 * Handle states
	 * Synchronize with mds and recovery thread (mutex)
	 */
	pthread_mutex_lock(&lga_cb.cb_lock);
	lgs_state_t lgs_state = lga_cb.lgs_state;
	lga_state_t lga_state = lga_cb.lga_state;
	pthread_mutex_unlock(&lga_cb.cb_lock);

	if (lgs_state == LGS_NO_ACTIVE) {
		/* We have a server but it is temporary unavailable. Client may
		 * try again
		 */
		TRACE("%s LGS no active", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

	if (lga_state == LGA_NO_SERVER) {
		/* We have no server and cannot open a stream.
		 * The client may try again
		 */
		TRACE("%s No server", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

	if (lga_state == LGA_RECOVERY2) {
		/* Auto recovery is ongoing. We have to wait for it to finish.
		 * The client may try again
		 */
		TRACE("%s LGA auto recovery ongoing (2)", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

	if (lga_state == LGA_RECOVERY1) {
		/* We are in recovery 1 state.
		 * Recover client and execute the request
		 */
		rc = recover_one_client(hdl_rec);
		if (rc == -1) {
			/* Client could not be recovered. Delete client and
			 * return BAD HANDLE
			 */
			TRACE("%s delete_one_client", __FUNCTION__);
			(void) delete_one_client(&lga_cb.client_list, hdl_rec);
			ais_rc = SA_AIS_ERR_BAD_HANDLE;
			/* Handles are destroyed so we shall not give handles */
			goto done;
		}
	}


    /** Populate a sync MDS message to obtain a log stream id and an
     *  instance open id.
     */
	memset(&msg, 0, sizeof(lgsv_msg_t));
	msg.type = LGSV_LGA_API_MSG;
	msg.info.api_info.type = LGSV_STREAM_OPEN_REQ;
	open_param = &msg.info.api_info.param.lstr_open_sync;

	/* Make it safe for free */
	open_param->logFileName = NULL;
	open_param->logFilePathName = NULL;

	populate_open_params(open_param,
			     logStreamName,
			     hdl_rec,
			     (SaLogFileCreateAttributesT_2 *)logFileCreateAttributes,
			     logStreamOpenFlags);

	if (logFileCreateAttributes != NULL) {
		/* Construct the logFileName */
		open_param->logFileName = (char *) malloc(strlen(logFileCreateAttributes->logFileName) + 1);
		if (open_param->logFileName == NULL) {
			ais_rc = SA_AIS_ERR_NO_MEMORY;
			goto done_give_hdl;
		}
		strcpy(open_param->logFileName, logFileCreateAttributes->logFileName);

		/* Construct the logFilePathName */
		/* A NULL pointer refers to impl defined directory */
		size_t len = (logFileCreateAttributes->logFilePathName == NULL) ? (2) :
			         (strlen(logFileCreateAttributes->logFilePathName) + 1);

		open_param->logFilePathName = (char *) malloc(len);
		if (open_param->logFilePathName == NULL) {
			ais_rc = SA_AIS_ERR_NO_MEMORY;
			goto done_give_hdl;
		}

		if (logFileCreateAttributes->logFilePathName == NULL)
			strcpy(open_param->logFilePathName, ".");
		else
			strcpy(open_param->logFilePathName, logFileCreateAttributes->logFilePathName);
	}

	/* Normalize the timeOut value */
	timeout = (uint32_t)(timeOut / LGSV_NANOSEC_TO_LEAPTM);

	if (timeout < NCS_SAF_MIN_ACCEPT_TIME) {
		TRACE("Timeout");
		ais_rc = SA_AIS_ERR_TIMEOUT;
		goto done_give_hdl;
	}

	/* Send a sync MDS message to obtain a log stream id */
	ncs_rc = lga_mds_msg_sync_send(&lga_cb, &msg, &o_msg, timeout, MDS_SEND_PRIORITY_HIGH);
	if (ncs_rc != NCSCC_RC_SUCCESS) {
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

	ais_rc = o_msg->info.api_resp_info.rc;
	if (SA_AIS_OK != ais_rc) {
		TRACE("Bad return status!!! rc = %d", ais_rc);
		lga_msg_destroy(o_msg);
		goto done_give_hdl;
	}

    /** Retrieve the log stream id and log stream open id params
     ** and pass them into the subroutine.
     **/
	log_stream_id = o_msg->info.api_resp_info.param.lstr_open_rsp.lstr_id;

    /** Lock LGA_CB
     **/
	pthread_mutex_lock(&lga_cb.cb_lock);

    /** Allocate an LGA_LOG_STREAM_HDL_REC structure and insert this
     *  into the list of channel hdl record.
     **/
	lstr_hdl_rec = lga_log_stream_hdl_rec_add(
		&hdl_rec,
		log_stream_id, logStreamOpenFlags,
		logStreamName, log_header_type
		);
	if (lstr_hdl_rec == NULL) {
		pthread_mutex_unlock(&lga_cb.cb_lock);
		lga_msg_destroy(o_msg);
		ais_rc = SA_AIS_ERR_NO_MEMORY;
		goto done_give_hdl;
	}

    /** UnLock LGA_CB
     **/
	pthread_mutex_unlock(&lga_cb.cb_lock);

	 /** Give the hdl-mgr allocated hdl to the application and free the response
	  *  message
      **/
	*logStreamHandle = (SaLogStreamHandleT)lstr_hdl_rec->log_stream_hdl;

	lga_msg_destroy(o_msg);

 done_give_hdl:
	ncshm_give_hdl(logHandle);
	free(open_param->logFileName);
	free(open_param->logFilePathName);

 done:
	TRACE_LEAVE();
	return ais_rc;
}

/**
 * 
 * @param logHandle
 * @param logStreamName
 * @param logFileCreateAttributes
 * @param logstreamOpenFlags
 * @param invocation
 * 
 * @return SaAisErrorT
 */
SaAisErrorT saLogStreamOpenAsync_2(SaLogHandleT logHandle,
				   const SaNameT *logStreamName,
				   const SaLogFileCreateAttributesT_2 *logFileCreateAttributes,
				   SaLogStreamOpenFlagsT logstreamOpenFlags, SaInvocationT invocation)
{
	TRACE_ENTER();
	TRACE_LEAVE();
	return SA_AIS_ERR_NOT_SUPPORTED;
}

/******************************************************************************
 * Write Log record
 * API function and help functions
 ******************************************************************************/

/**
 * Validate the log record and if generic header add
 * logSvcUsrName from environment variable SA_AMF_COMPONENT_NAME
 *
 * @param logRecord[in]
 * @param logSvcUsrName[out]
 * @param write_param[out]
 * @return AIS return code
 */
static SaAisErrorT handle_log_record(const SaLogRecordT *logRecord,
	SaNameT *logSvcUsrName,
	lgsv_write_log_async_req_t *write_param,
	SaTimeT *const logTimeStamp)
{
	SaAisErrorT ais_rc = SA_AIS_OK;

	TRACE_ENTER();

	if (NULL == logRecord) {
		TRACE("SA_AIS_ERR_INVALID_PARAM => NULL pointer check");
		ais_rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if (logRecord->logHdrType == SA_LOG_GENERIC_HEADER) {
		switch (logRecord->logHeader.genericHdr.logSeverity) {
		case SA_LOG_SEV_EMERGENCY:
		case SA_LOG_SEV_ALERT:
		case SA_LOG_SEV_CRITICAL:
		case SA_LOG_SEV_ERROR:
		case SA_LOG_SEV_WARNING:
		case SA_LOG_SEV_NOTICE:
		case SA_LOG_SEV_INFO:
			break;
		default:
			TRACE("Invalid severity: %x",
				logRecord->logHeader.genericHdr.logSeverity);
			ais_rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
	}

	if (logRecord->logBuffer != NULL) {
		if ((logRecord->logBuffer->logBuf == NULL) &&
			(logRecord->logBuffer->logBufSize != 0)) {
			TRACE("logBuf == NULL && logBufSize != 0");
			ais_rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
	}

	/* Set timeStamp data if not provided by application user */
	if (logRecord->logTimeStamp == SA_TIME_UNKNOWN) {
		*logTimeStamp = setLogTime();
		write_param->logTimeStamp = logTimeStamp;
	} else {
		write_param->logTimeStamp = (SaTimeT *)&logRecord->logTimeStamp;
	}

	/* SA_AIS_ERR_INVALID_PARAM, bullet 2 in SAI-AIS-LOG-A.02.01
	   Section 3.6.3, Return Values */
	if (logRecord->logHdrType == SA_LOG_GENERIC_HEADER) {
		if (logRecord->logHeader.genericHdr.logSvcUsrName == NULL) {
			char *logSvcUsrChars = NULL;
			TRACE("logSvcUsrName == NULL");
			logSvcUsrChars = getenv("SA_AMF_COMPONENT_NAME");
			if (logSvcUsrChars == NULL) {
				ais_rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}
			logSvcUsrName->length = strlen(logSvcUsrChars);
			if (logSvcUsrName->length >= SA_MAX_NAME_LENGTH) {
				TRACE("SA_AMF_COMPONENT_NAME is too long");
				ais_rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}
			strcpy((char *)logSvcUsrName->value, logSvcUsrChars);
			write_param->logSvcUsrName = logSvcUsrName;
		} else {
			if (logRecord->logHeader.genericHdr.logSvcUsrName->length >=
				SA_MAX_NAME_LENGTH) {
				TRACE("logSvcUsrName too long");
				ais_rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}
			logSvcUsrName->length =
				logRecord->logHeader.genericHdr.logSvcUsrName->length;
			write_param->logSvcUsrName =
				(SaNameT *)logRecord->logHeader.genericHdr.logSvcUsrName;
		}
	}

	if (logRecord->logHdrType == SA_LOG_NTF_HEADER) {
		if (logRecord->logHeader.ntfHdr.notificationObject == NULL) {
			TRACE("notificationObject == NULL");
			ais_rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}

		if (logRecord->logHeader.ntfHdr.notificationObject->length >=
			SA_MAX_NAME_LENGTH) {
			TRACE("notificationObject.length >= SA_MAX_NAME_LENGTH");
			ais_rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}

		if (logRecord->logHeader.ntfHdr.notifyingObject == NULL) {
			TRACE("notifyingObject == NULL");
			ais_rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}

		if (logRecord->logHeader.ntfHdr.notifyingObject->length >=
			SA_MAX_NAME_LENGTH) {
			TRACE("notifyingObject.length >= SA_MAX_NAME_LENGTH");
			ais_rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
	}

done:
	TRACE_LEAVE();
	return ais_rc;
}

/**
 * 
 * @param logStreamHandle
 * @param timeOut
 * @param logRecord
 * 
 * @return SaAisErrorT
 */
SaAisErrorT saLogWriteLog(SaLogStreamHandleT logStreamHandle, SaTimeT timeOut, const SaLogRecordT *logRecord)
{
	TRACE_ENTER();
	TRACE_LEAVE();
	return SA_AIS_ERR_NOT_SUPPORTED;
}

/**
 * 
 * @param logStreamHandle
 * @param invocation
 * @param ackFlags
 * @param logRecord
 * 
 * @return SaAisErrorT
 */
SaAisErrorT saLogWriteLogAsync(SaLogStreamHandleT logStreamHandle,
			       SaInvocationT invocation, SaLogAckFlagsT ackFlags, const SaLogRecordT *logRecord)
{
	lga_log_stream_hdl_rec_t *lstr_hdl_rec;
	lga_client_hdl_rec_t *hdl_rec;
	lgsv_msg_t msg;
	SaAisErrorT ais_rc = SA_AIS_OK;
	lgsv_write_log_async_req_t *write_param;
	SaNameT logSvcUsrName;
	int rc;

	memset(&(msg), 0, sizeof(lgsv_msg_t));
	write_param = &msg.info.api_info.param.write_log_async;
	TRACE_ENTER();

	if ((ackFlags != 0) && (ackFlags != SA_LOG_RECORD_WRITE_ACK)) {
		TRACE("SA_AIS_ERR_BAD_FLAGS=> ackFlags");
		ais_rc = SA_AIS_ERR_BAD_FLAGS;
		goto done;
	}

	/* Validate the log record and if generic header add
	 * logSvcUsrName from environment variable SA_AMF_COMPONENT_NAME
	 */
	SaTimeT logTimeStamp;
	ais_rc = handle_log_record(logRecord, &logSvcUsrName, write_param, &logTimeStamp);
	if (ais_rc != SA_AIS_OK) {
		TRACE("%s: Validate Log record Fail", __FUNCTION__);
		goto done;
	}

	/* Retrieve log stream hdl record
	 * From now on we must give stream before return
	 */
	lstr_hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, logStreamHandle);
	if (lstr_hdl_rec == NULL) {
		TRACE("%s: ncshm_take_hdl logStreamHandle FAILED!",
			__FUNCTION__);
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* SA_AIS_ERR_INVALID_PARAM, bullet 1 in SAI-AIS-LOG-A.02.01 
	   Section 3.6.3, Return Values */
	if (lstr_hdl_rec->log_header_type != logRecord->logHdrType) {
		TRACE("%s: lstr_hdl_rec->log_header_type != logRecord->logHdrType",
			__FUNCTION__);
		ais_rc = SA_AIS_ERR_INVALID_PARAM;
		goto done_give_hdl_stream;
	}

	/* retrieve the lga client hdl record
	 * From now on we must give both stream and client (parent) handle
	 * before return
	 */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, lstr_hdl_rec->parent_hdl->local_hdl);
	if (hdl_rec == NULL) {
		TRACE("%s: ncshm_take_hdl logHandle FAILED!",
			__FUNCTION__);
		ais_rc = SA_AIS_ERR_LIBRARY;
		goto done_give_hdl_stream;
	}

	if ((hdl_rec->reg_cbk.saLogWriteLogCallback == NULL) && (ackFlags == SA_LOG_RECORD_WRITE_ACK)) {
		TRACE("%s: Write Callback not registered", __FUNCTION__);
		ais_rc = SA_AIS_ERR_INIT;
		goto done_give_hdl_all;
	}

	/***
	 * Handle states
	 * Synchronize with mds and recovery thread (mutex)
	 */
	pthread_mutex_lock(&lga_cb.cb_lock);
	lgs_state_t lgs_state = lga_cb.lgs_state;
	lga_state_t lga_state = lga_cb.lga_state;
	pthread_mutex_unlock(&lga_cb.cb_lock);

	if (lgs_state == LGS_NO_ACTIVE) {
		/* We have a server but it is temporarily unavailable.
		 * Client may try again
		 */
		TRACE("%s: LGS no active", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl_all;
	}

	if (lga_state == LGA_NO_SERVER) {
		/* We have no server and cannot write. The client may try again
		 */
		TRACE("\t LGA_NO_SERVER");
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl_all;
	}

	if (lga_state == LGA_RECOVERY2) {
		/* Auto recovery is ongoing. We have to wait for it to finish.
		 * The client may try again
		 */
		TRACE("\t LGA_RECOVERY2");
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl_all;
	}

	if (lga_state == LGA_RECOVERY1) {
		/* We are in recovery 1 state.
		 * Recover client and execute the request
		 */
		TRACE("\t LGA_RECOVERY1");
		rc = recover_one_client(hdl_rec);
		if (rc == -1) {
			/* Client could not be recovered. Delete client and
			 * return BAD HANDLE
			 */
			TRACE("\t recover_one_client Fail");
			/* The log stream handle is not released in
			 * delete_one_client()  so we have to do it here.
			 * The function is used in other APIs that does not
			 * take a log stream handle
			 */
			ncshm_give_hdl(logStreamHandle);
			(void) delete_one_client(&lga_cb.client_list, hdl_rec);
			ais_rc = SA_AIS_ERR_BAD_HANDLE;
			/* Handles are destroyed so we shall not give handles */
			goto done;
		}
	}

    /** populate the mds message to send across to the LGS
    ** whence it will possibly get published
    **/
	msg.type = LGSV_LGA_API_MSG;
	msg.info.api_info.type = LGSV_WRITE_LOG_ASYNC_REQ;
	write_param->invocation = invocation;
	write_param->ack_flags = ackFlags;
	write_param->client_id = hdl_rec->lgs_client_id;
	write_param->lstr_id = lstr_hdl_rec->lgs_log_stream_id;
	write_param->logRecord = (SaLogRecordT *)logRecord;
    /** Send the message out to the LGS
     **/
	if (NCSCC_RC_SUCCESS != lga_mds_msg_async_send(&lga_cb, &msg, MDS_SEND_PRIORITY_MEDIUM))
		ais_rc = SA_AIS_ERR_TRY_AGAIN;

 done_give_hdl_all:
	ncshm_give_hdl(hdl_rec->local_hdl);
 done_give_hdl_stream:
	ncshm_give_hdl(logStreamHandle);

done:
	TRACE_LEAVE();
	return ais_rc;
}

/**
 * API function for closing stream
 * 
 * @param logStreamHandle
 * 
 * @return SaAisErrorT
 */
SaAisErrorT saLogStreamClose(SaLogStreamHandleT logStreamHandle)
{
	lga_log_stream_hdl_rec_t *lstr_hdl_rec;
	lga_client_hdl_rec_t *hdl_rec;
	lgsv_msg_t msg, *o_msg = NULL;
	SaAisErrorT ais_rc = SA_AIS_OK;
	uint32_t mds_rc;
	int rc;

	TRACE_ENTER();

	lstr_hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, logStreamHandle);
	if (lstr_hdl_rec == NULL) {
		TRACE("ncshm_take_hdl logStreamHandle ");
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/***
	 * Handle states
	 * Synchronize with mds and recovery thread (mutex)
	 */
	pthread_mutex_lock(&lga_cb.cb_lock);
	lgs_state_t lgs_state = lga_cb.lgs_state;
	lga_state_t lga_state = lga_cb.lga_state;
	pthread_mutex_unlock(&lga_cb.cb_lock);

	if (lgs_state == LGS_NO_ACTIVE) {
		/* We have a server but it is temporarily unavailable. Client may
		 * try again
		 */
		TRACE("%s LGS no active", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl_stream;
	}

	if (lga_state == LGA_RECOVERY2) {
		/* Auto recovery is ongoing. We have to wait for it to finish.
		 * The client may try again
		 */
		TRACE("%s LGA auto recovery ongoing (2)", __FUNCTION__);
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl_stream;
	}

	/* retrieve the client hdl record */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, lstr_hdl_rec->parent_hdl->local_hdl);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl logHandle ");
		ais_rc = SA_AIS_ERR_LIBRARY;
		goto done_give_hdl_stream;
	}

	if (lga_state == LGA_NO_SERVER) {
		/* No server is available. Remove the stream from client database.
		 * Server side will manage to release resources of this stream when up.
		 */
		TRACE("%s No server", __FUNCTION__);
		ais_rc = SA_AIS_OK;
		goto rmv_stream;
	}

	if (lga_state == LGA_RECOVERY1) {
		/* We are in recovery 1 state.
		 * Recover client and execute the request
		 */
		rc = recover_one_client(hdl_rec);
		if (rc == -1) {
			/* Client could not be recovered. Delete client and
			 * return BAD HANDLE
			 */
			TRACE("%s Recover Fail delete_one_client", __FUNCTION__);
			/* The log stream handle is not released in
			 * delete_one_client()  so we have to do it here.
			 * The function is used in other APIs that does not
			 * take a log stream handle
			 */
			ncshm_give_hdl(logStreamHandle);
			(void) delete_one_client(&lga_cb.client_list, hdl_rec);
			ais_rc = SA_AIS_ERR_BAD_HANDLE;
			/* Handles are destroyed so we shall not give handles */
			goto done;
		}
	}

    /** Populate a MDS message to send to the LGS for a channel
     *  close operation.
     **/
	memset(&msg, 0, sizeof(lgsv_msg_t));
	msg.type = LGSV_LGA_API_MSG;
	msg.info.api_info.type = LGSV_STREAM_CLOSE_REQ;
	msg.info.api_info.param.lstr_close.client_id = hdl_rec->lgs_client_id;
	msg.info.api_info.param.lstr_close.lstr_id = lstr_hdl_rec->lgs_log_stream_id;
	mds_rc = lga_mds_msg_sync_send(&lga_cb, &msg, &o_msg, LGS_WAIT_TIME,MDS_SEND_PRIORITY_MEDIUM);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		ais_rc = SA_AIS_ERR_TIMEOUT;
		TRACE("lga_mds_msg_sync_send FAILED: %s", saf_error(ais_rc));
		goto done_give_hdl_all;
	default:
		TRACE("lga_mds_msg_sync_send FAILED: %s", saf_error(ais_rc));
		ais_rc = SA_AIS_ERR_NO_RESOURCES;
		goto done_give_hdl_all;
	}

	if (o_msg != NULL) {
		ais_rc = o_msg->info.api_resp_info.rc;
		lga_msg_destroy(o_msg);
	} else
		ais_rc = SA_AIS_ERR_NO_RESOURCES;

rmv_stream:
	if (ais_rc == SA_AIS_OK) {
		pthread_mutex_lock(&lga_cb.cb_lock);

	/** Delete this log stream & the associated resources with this
         *  instance of log stream open.
        **/
		if (NCSCC_RC_SUCCESS != lga_log_stream_hdl_rec_del(&hdl_rec->stream_list, lstr_hdl_rec)) {
			TRACE("Unable to delete log stream");
			ais_rc = SA_AIS_ERR_LIBRARY;
		}

		pthread_mutex_unlock(&lga_cb.cb_lock);
	}

 done_give_hdl_all:
	ncshm_give_hdl(hdl_rec->local_hdl);
 done_give_hdl_stream:
	ncshm_give_hdl(logStreamHandle);

 done:
	TRACE_LEAVE();
	return ais_rc;
}

/**
 * 
 * @param logHandle
 * @param limitId
 * @param limitValue
 * 
 * @return SaAisErrorT
 */
SaAisErrorT saLogLimitGet(SaLogHandleT logHandle, SaLogLimitIdT limitId, SaLimitValueT *limitValue)
{
	TRACE_ENTER();
	TRACE_LEAVE();
	return SA_AIS_ERR_NOT_SUPPORTED;
}
