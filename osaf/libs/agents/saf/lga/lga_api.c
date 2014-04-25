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
#include "lga.h"

#define NCS_SAF_MIN_ACCEPT_TIME 10

/* Macro to validate the dispatch flags */
#define m_DISPATCH_FLAG_IS_VALID(flag) \
   ( (SA_DISPATCH_ONE == flag) || \
     (SA_DISPATCH_ALL == flag) || \
     (SA_DISPATCH_BLOCKING == flag) )

#define LGSV_NANOSEC_TO_LEAPTM 10000000

/* The main controle block */
lga_cb_t lga_cb = {
	.cb_lock = PTHREAD_MUTEX_INITIALIZER,
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

		strncpy(open_param->logFileName, logFileCreateAttributes->logFileName, NAME_MAX);

		/* A NULL pointer refers to impl defined directory */
		if (logFileCreateAttributes->logFilePathName == NULL)
			strcpy(open_param->logFilePathName, ".");
		else
			strncpy(open_param->logFilePathName, logFileCreateAttributes->logFilePathName, PATH_MAX);

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
	SaAisErrorT rc;
	uint32_t client_id;

	TRACE_ENTER();

	if ((logHandle == NULL) || (version == NULL)) {
		TRACE("version or handle FAILED");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if ((rc = lga_startup()) != NCSCC_RC_SUCCESS) {
		TRACE("lga_startup FAILED: %u", rc);
		rc = SA_AIS_ERR_LIBRARY;
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
		lga_shutdown();
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	if (!lga_cb.lgs_up) {
		lga_shutdown();
		TRACE("LGS server is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
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
	rc = lga_mds_msg_sync_send(&lga_cb, &i_msg, &o_msg, LGS_WAIT_TIME,MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		lga_shutdown();
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

    /** Make sure the LGS return status was SA_AIS_OK 
     **/
	if (SA_AIS_OK != o_msg->info.api_resp_info.rc) {
		rc = o_msg->info.api_resp_info.rc;
		TRACE("LGS return FAILED");
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
		rc = SA_AIS_ERR_NO_MEMORY;
		goto err;
	}

	/* pass the handle value to the appl */
	if (SA_AIS_OK == rc)
		*logHandle = lga_hdl_rec->local_hdl;

 err:
	/* free up the response message */
	if (o_msg)
		lga_msg_destroy(o_msg);

	if (rc != SA_AIS_OK) {
		TRACE_2("LGA INIT FAILED\n");
		lga_shutdown();
	}

 done:
	TRACE_LEAVE();
	return rc;
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
	lgsv_msg_t msg, *o_msg = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t mds_rc;

	TRACE_ENTER();

	/* retrieve hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, logHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* Check Whether LGS is up or not */
	if (!lga_cb.lgs_up) {
		TRACE("LGS down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

    /** populate & send the finalize message 
     ** and make sure the finalize from the server
     ** end returned before deleting the local records.
     **/
	memset(&msg, 0, sizeof(lgsv_msg_t));
	msg.type = LGSV_LGA_API_MSG;
	msg.info.api_info.type = LGSV_FINALIZE_REQ;
	msg.info.api_info.param.finalize.client_id = hdl_rec->lgs_client_id;

	mds_rc = lga_mds_msg_sync_send(&lga_cb, &msg, &o_msg, LGS_WAIT_TIME,MDS_SEND_PRIORITY_MEDIUM);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE("lga_mds_msg_sync_send FAILED: %u", rc);
		goto done_give_hdl;
	default:
		TRACE("lga_mds_msg_sync_send FAILED: %u", rc);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done_give_hdl;
	}

	if (o_msg != NULL) {
		rc = o_msg->info.api_resp_info.rc;
		lga_msg_destroy(o_msg);
	} else
		rc = SA_AIS_ERR_NO_RESOURCES;

	if (rc == SA_AIS_OK) {
		rc = lga_hdl_rec_del(&lga_cb.client_list, hdl_rec);
		if (rc != NCSCC_RC_SUCCESS)
			rc = SA_AIS_ERR_BAD_HANDLE;
	}

 done_give_hdl:
	ncshm_give_hdl(logHandle);

	if (rc == SA_AIS_OK) {
		rc = lga_shutdown();
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("lga_shutdown ");
	}

 done:
	TRACE_LEAVE2("rc = %u", rc);
	return rc;
}

static SaAisErrorT validate_open_params(SaLogHandleT logHandle,
					const SaNameT *logStreamName,
					const SaLogFileCreateAttributesT_2 *logFileCreateAttributes,
					SaLogStreamOpenFlagsT logStreamOpenFlags,
					SaTimeT timeOut, SaLogStreamHandleT *logStreamHandle, uint32_t *header_type)
{
	size_t len;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER();

	osafassert(header_type != NULL);

	if ((NULL == logStreamName) || (NULL == logStreamHandle)) {
		TRACE("SA_AIS_ERR_INVALID_PARAM => NULL pointer check");
		rc = SA_AIS_ERR_INVALID_PARAM;
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
		}

		*header_type = (uint32_t)SA_LOG_GENERIC_HEADER;
	}

	/* Check implementation specific string length */
	if (NULL != logFileCreateAttributes) {
		len = strlen(logFileCreateAttributes->logFileName);
		if ((len == 0) || (len > NAME_MAX)) {
			TRACE("logFileName");
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
	return rc;
}

/**
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
	SaAisErrorT rc;
	uint32_t timeout;
	uint32_t log_stream_id;
	uint32_t log_header_type = 0;

	TRACE_ENTER();

	rc = validate_open_params(logHandle, logStreamName, logFileCreateAttributes,
				  logStreamOpenFlags, timeOut, logStreamHandle, &log_header_type);
	if (rc != SA_AIS_OK)
		goto done;

	/* retrieve log service hdl rec */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, logHandle);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

    /** Populate a sync MDS message to obtain a log stream id and an
     **  instance open id.
     **/
	memset(&msg, 0, sizeof(lgsv_msg_t));
	msg.type = LGSV_LGA_API_MSG;
	msg.info.api_info.type = LGSV_STREAM_OPEN_REQ;
	open_param = &msg.info.api_info.param.lstr_open_sync;

	populate_open_params(open_param,
			     logStreamName,
			     hdl_rec, (SaLogFileCreateAttributesT_2 *)logFileCreateAttributes, logStreamOpenFlags);

	/* Normalize the timeOut value */
	timeout = (uint32_t)(timeOut / LGSV_NANOSEC_TO_LEAPTM);

	if (timeout < NCS_SAF_MIN_ACCEPT_TIME) {
		TRACE("Timeout");
		rc = SA_AIS_ERR_TIMEOUT;
		goto done_give_hdl;
	}

	/* Check whether LGS is up or not */
	if (!lga_cb.lgs_up) {
		TRACE("LGS down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

	/* Send a sync MDS message to obtain a log stream id */
	rc = lga_mds_msg_sync_send(&lga_cb, &msg, &o_msg, timeout,MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		if (o_msg)
			lga_msg_destroy(o_msg);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl;
	}

	if (SA_AIS_OK != o_msg->info.api_resp_info.rc) {
		rc = o_msg->info.api_resp_info.rc;
		TRACE("Bad return status!!! rc = %d", rc);
		if (o_msg)
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
	lstr_hdl_rec = lga_log_stream_hdl_rec_add(&hdl_rec,
						  log_stream_id, logStreamOpenFlags, logStreamName, log_header_type);
	if (lstr_hdl_rec == NULL) {
		pthread_mutex_unlock(&lga_cb.cb_lock);
		if (o_msg)
			lga_msg_destroy(o_msg);
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done_give_hdl;
	}
    /** UnLock LGA_CB
     **/
	pthread_mutex_unlock(&lga_cb.cb_lock);

    /** Give the hdl-mgr allocated hdl to the application.
     **/
	*logStreamHandle = (SaLogStreamHandleT)lstr_hdl_rec->log_stream_hdl;

	if (o_msg)
		lga_msg_destroy(o_msg);

 done_give_hdl:
	ncshm_give_hdl(logHandle);
 done:
	TRACE_LEAVE();
	return rc;
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
	SaAisErrorT rc = SA_AIS_OK;
	SaNameT logSvcUsrName;
	SaTimeT logTimeStamp;
	lgsv_write_log_async_req_t *write_param;

	memset(&(msg), 0, sizeof(lgsv_msg_t));
	write_param = &msg.info.api_info.param.write_log_async;
	TRACE_ENTER();

	if (NULL == logRecord) {
		TRACE("SA_AIS_ERR_INVALID_PARAM => NULL pointer check");
		rc = SA_AIS_ERR_INVALID_PARAM;
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
			TRACE("Invalid severity: %x", logRecord->logHeader.genericHdr.logSeverity);
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
	}

	if (logRecord->logBuffer != NULL) {
		if ((logRecord->logBuffer->logBuf == NULL) && (logRecord->logBuffer->logBufSize != 0)) {
			TRACE("logBuf == NULL && logBufSize != 0");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
	}

	if ((ackFlags != 0) && (ackFlags != SA_LOG_RECORD_WRITE_ACK)) {
		TRACE("SA_AIS_ERR_BAD_FLAGS=> ackFlags");
		rc = SA_AIS_ERR_BAD_FLAGS;
		goto done;
	}

	/* Set timeStamp data if not provided by application user */
	if (logRecord->logTimeStamp == SA_TIME_UNKNOWN) {
		logTimeStamp = setLogTime();
		write_param->logTimeStamp = &logTimeStamp;
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
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}
			logSvcUsrName.length = strlen(logSvcUsrChars);
			if (logSvcUsrName.length > SA_MAX_NAME_LENGTH) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}
			strcpy((char *)logSvcUsrName.value, logSvcUsrChars);
			write_param->logSvcUsrName = &logSvcUsrName;
		} else {
			if (logRecord->logHeader.genericHdr.logSvcUsrName->length > SA_MAX_NAME_LENGTH) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto done;
			}
			logSvcUsrName.length = logRecord->logHeader.genericHdr.logSvcUsrName->length;
			write_param->logSvcUsrName = (SaNameT *)logRecord->logHeader.genericHdr.logSvcUsrName;
		}
	}

	if (logRecord->logHdrType == SA_LOG_NTF_HEADER) {
		if (logRecord->logHeader.ntfHdr.notificationObject == NULL) {
			TRACE("notificationObject == NULL");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}

		if (logRecord->logHeader.ntfHdr.notificationObject->length > SA_MAX_NAME_LENGTH) {
			TRACE("notificationObject.length > SA_MAX_NAME_LENGTH");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}

		if (logRecord->logHeader.ntfHdr.notifyingObject == NULL) {
			TRACE("notifyingObject == NULL");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}

		if (logRecord->logHeader.ntfHdr.notifyingObject->length > SA_MAX_NAME_LENGTH) {
			TRACE("notifyingObject.length > SA_MAX_NAME_LENGTH");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
	}

	/* retrieve log stream hdl record */
	lstr_hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, logStreamHandle);
	if (lstr_hdl_rec == NULL) {
		TRACE("ncshm_take_hdl logStreamHandle FAILED!");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* SA_AIS_ERR_INVALID_PARAM, bullet 1 in SAI-AIS-LOG-A.02.01 
	   Section 3.6.3, Return Values */
	if (lstr_hdl_rec->log_header_type != logRecord->logHdrType) {
		TRACE("lstr_hdl_rec->log_header_type != logRecord->logHdrType");
		ncshm_give_hdl(logStreamHandle);
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* retrieve the lga client hdl record */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, lstr_hdl_rec->parent_hdl->local_hdl);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl logHandle FAILED!");
		ncshm_give_hdl(logStreamHandle);
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	if ((hdl_rec->reg_cbk.saLogWriteLogCallback == NULL) && (ackFlags == SA_LOG_RECORD_WRITE_ACK)) {
		TRACE("Write Callback not registered");
		ncshm_give_hdl(logStreamHandle);
		ncshm_give_hdl(hdl_rec->local_hdl);
		rc = SA_AIS_ERR_INIT;
		goto done;
	}


	/* Check Whether LGS is up or not */
	if (!lga_cb.lgs_up) {
		ncshm_give_hdl(logStreamHandle);
		ncshm_give_hdl(hdl_rec->local_hdl);
		TRACE("LGS down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
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
		rc = SA_AIS_ERR_TRY_AGAIN;

    /** Give all the handles that were taken **/
	ncshm_give_hdl(lstr_hdl_rec->log_stream_hdl);
	ncshm_give_hdl(hdl_rec->local_hdl);

 done:
	TRACE_LEAVE();
	return rc;
}

/**
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
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t mds_rc;

	TRACE_ENTER();

	lstr_hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, logStreamHandle);
	if (lstr_hdl_rec == NULL) {
		TRACE("ncshm_take_hdl logStreamHandle ");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* retrieve the client hdl record */
	hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_LGA, lstr_hdl_rec->parent_hdl->local_hdl);
	if (hdl_rec == NULL) {
		TRACE("ncshm_take_hdl logHandle ");
		rc = SA_AIS_ERR_LIBRARY;
		goto done_give_hdl_stream;
	}

	/* Check Whether LGS is up or not */
	if (!lga_cb.lgs_up) {
		TRACE("LGS is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done_give_hdl_all;
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
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE("lga_mds_msg_sync_send FAILED: %u", rc);
		goto done_give_hdl_all;
	default:
		TRACE("lga_mds_msg_sync_send FAILED: %u", rc);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done_give_hdl_all;
	}

	if (o_msg != NULL) {
		rc = o_msg->info.api_resp_info.rc;
		lga_msg_destroy(o_msg);
	} else
		rc = SA_AIS_ERR_NO_RESOURCES;

	if (rc == SA_AIS_OK) {
		pthread_mutex_lock(&lga_cb.cb_lock);

	/** Delete this log stream & the associated resources with this
         *  instance of log stream open.
        **/
		if (NCSCC_RC_SUCCESS != lga_log_stream_hdl_rec_del(&hdl_rec->stream_list, lstr_hdl_rec)) {
			TRACE("Unable to delete log stream");
			rc = SA_AIS_ERR_LIBRARY;
		}

		pthread_mutex_unlock(&lga_cb.cb_lock);
	}

 done_give_hdl_all:
	ncshm_give_hdl(hdl_rec->local_hdl);
 done_give_hdl_stream:
	ncshm_give_hdl(logStreamHandle);

 done:
	TRACE_LEAVE();
	return rc;
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
