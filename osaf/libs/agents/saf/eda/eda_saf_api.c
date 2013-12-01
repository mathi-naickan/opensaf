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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
*                                                                            *
*  MODULE NAME:  eda_saf_api.c                                               *
*                                                                            *
*                                                                            *
*  DESCRIPTION                                                               *
*  This module contains the client library APIs for EDSv (a.k.a. EDA)        *
*  as defined in by Service Availability Forum (SAF) Application Interface   *
*  Specification SAI-AIS-B.03.01 section 3; SA Event Service API.            *
*                                                                            *
*  APIs                                                                      *
*                                                                            *
*  saEvtInitialize()                                                         *
*  saEvtSelectionObjectGet()                                                 *
*  saEvtDispatch()                                                           *
*  saEvtFinalize()                                                           *
*  saEvtChannelOpen()                                                        *
*  saEvtChannelOpenAsync()                                                   *
*  saEvtChannelClose()                                                       *
*  saEvtChannelUnlink()                                                      *
*  saEvtEventAllocate()                                                      *
*  saEvtEventFree()                                                          *
*  saEvtEventAttributesSet()                                                 *
*  saEvtEventAttributesGet()                                                 *
*  saEvtEventPatternFree()                                                   *
*  saEvtEventDataGet()                                                       *
*  saEvtEventDeliverCallbackT()                                              *
*  saEvtEventPublish()                                                       *
*  saEvtEventSubscribe()                                                     *
*  saEvtEventUnsubscribe()                                                   *
*  saEvtEventRetentionTimeClear()                                            *
*  saEvtLimitGet()                                                           *
*                                                                            *
* Event Service Agent library adopts the following guidelines for traces:    *
*  TRACE   debug traces                 - DEBUG                              *
*  TRACE_1 normal but important events  - INFO                               *
*  TRACE_2 user errors with return code - NOTICE                             *
*  TRACE_3 unusual or strange events    - WARNING                            *
*  TRACE_4 library errors ERR_LIBRARY   - ERROR                              *
*****************************************************************************/
/* main eda include */
#include "eda.h"

#define NCS_SAF_MIN_ACCEPT_TIME 10

/***************************************************************************
 * 3.5.1
 *
 * saEvtInitialize()
 *
 * This function initializes the Event Service for the invoking process
 * and registers the various callback functions. This function must be
 * invoked prior to the invocation of any other Event Service functionality.
 * The handle evtHandle is returned as the reference to this association
 * between the process and the Event Service. The process uses this handle
 * in subsequent communication with the Event Service.
 *
 * Parameters
 *
 * evtHandle - [out] A pointer to the handle for this initialization of the
 *                   Event Service.
 * callbacks - [in] A pointer to the callbacks structure that contains the
 *                  callback functions of the invoking process that the
 *                  Event Service may invoke.
 * version - [in] A pointer to the version of the Event Service that the
 *                invoking process is using.
 *
 ***************************************************************************/
SaAisErrorT saEvtInitialize(SaEvtHandleT *o_evtHandle, const SaEvtCallbacksT *callbacks, SaVersionT *io_version)
{
	EDA_CB *eda_cb;
	EDA_CLIENT_HDL_REC *eda_hdl_rec;
	EDSV_MSG i_msg, *o_msg;
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t reg_id = 0;
	EDSV_EDA_INITIALIZE_RSP *init_rsp = NULL;
	SaVersionT client_version;
	TRACE_ENTER();

	if ((rc = ncs_agents_startup()) != SA_AIS_OK) {
		TRACE_4("agents startup failed");
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;
	}

	if ((rc = ncs_eda_startup()) != SA_AIS_OK) {
		ncs_agents_shutdown();
		TRACE_4("eda startup failed");
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;
	}

	/* Retrieve EDA_CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("global take handle failed: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	if ((NULL == o_evtHandle) || (NULL == io_version)) {
		ncshm_give_hdl(gl_eda_hdl);
		ncs_eda_shutdown();
		ncs_agents_shutdown();
		TRACE_2("out param is NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Populate the message to be sent to the EDS */
	m_EDA_EDSV_INIT_MSG_FILL(i_msg, (*io_version));

	/* remmber the client verison to be added in the client recore */
	client_version = *io_version;

	/* Validate the version. Should be either B0101 or B0301, intermittent releases are not supported for now */

	if (m_EDA_VER_IS_VALID(io_version)) {
		m_EDA_FILL_VERSION(io_version);
		rc = SA_AIS_OK;
	} else {
		io_version->releaseCode = EDSV_RELEASE_CODE;
		m_EDA_FILL_VERSION(io_version);
		ncshm_give_hdl(gl_eda_hdl);
		ncs_eda_shutdown();
		ncs_agents_shutdown();
		rc = SA_AIS_ERR_VERSION;
		TRACE_2("unsupported version");
		TRACE_LEAVE();
		return rc;
	}

	if (!eda_cb->eds_intf.eds_up) {
		ncshm_give_hdl(gl_eda_hdl);
		ncs_eda_shutdown();
		ncs_agents_shutdown();
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("event server is not yet up");
		TRACE_LEAVE();
		return rc;
	}

	/* Send a message to EDS to obtain a reg_id/server ref id which is cluster
	 * wide unique.
	 */
	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_sync_send(eda_cb, &i_msg, &o_msg, EDSV_WAIT_TIME))) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		ncshm_give_hdl(gl_eda_hdl);
		ncs_eda_shutdown();
		ncs_agents_shutdown();
		TRACE_4("mds send to event server failed");
		TRACE_LEAVE();
		return rc;
	}

   /** Make sure the EDS return status was SA_AIS_OK 
    **/
	if (SA_AIS_OK != o_msg->info.api_resp_info.rc) {
		rc = o_msg->info.api_resp_info.rc;
		TRACE_4("event server returned failure: %u", rc);
		goto err3;
	}

	init_rsp = &(o_msg->info.api_resp_info.param.init_rsp);
	if (init_rsp) {
      /** Store the regId returned by the EDS 
       ** to pass into the next routine
       **/
		reg_id = init_rsp->reg_id;
	}

	/* create the hdl record & store the callbacks */
	if (NULL == (eda_hdl_rec = eda_hdl_rec_add(&eda_cb, callbacks, reg_id, client_version))) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("unable to add handle to db");
		goto err3;
	}

	/* pass the handle value to the appl */
	if (SA_AIS_OK == rc)
		*o_evtHandle = eda_hdl_rec->local_hdl;

 err3:
	/* free up the response message */
	if (o_msg)
		eda_msg_destroy(o_msg);

	/* return EDA CB */
	ncshm_give_hdl(gl_eda_hdl);
	if (rc != SA_AIS_OK) {
		ncs_eda_shutdown();
		ncs_agents_shutdown();
	} else 
		TRACE("user handle returned: %llx. Internal reg_id for this handle: %u", *o_evtHandle, reg_id);

	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 3.5.2
 *
 * saEvtSelectionObjectGet()
 *
 * The saEvtSelectionObjectGet() function returns the operating system handle
 * selectionObject, associated with the handle evtHandle, allowing the
 * invoking process to ascertain when callbacks are pending. This function
 * allows a process to avoid repeated invoking saEvtDispatch() to see if
 * there is a new event, thus, needlessly consuming CPU time. In a POSIX
 * environment the system handle could be a file descriptor that is used with
 * the poll() system call to detect incoming callbacks.
 *
 *
 * Parameters
 *
 * evtHandle - [in]
 *   A pointer to the handle, obtained through the saEvtInitialize() function,
 *   designating this particular initialization of the Event Service.
 *
 * selectionObject - [out]
 *   A pointer to the operating system handle that the process may use to
 *   detect pending callbacks.
 *
 ***************************************************************************/
SaAisErrorT saEvtSelectionObjectGet(SaEvtHandleT evtHandle, SaSelectionObjectT *o_sel_obj)
{
	EDA_CB *eda_cb = 0;
	EDA_CLIENT_HDL_REC *hdl_rec = 0;
	SaAisErrorT rc = SA_AIS_OK;
	NCS_SEL_OBJ sel_obj;
	TRACE_ENTER2("event handle: %llx", evtHandle);

	if (NULL == o_sel_obj) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("NULL value passed for selection object");
		TRACE_LEAVE();
		return rc;
	}
	/* retrieve EDA_CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;	/* check */
		TRACE_4("Unable to retrieve global handle: %u", gl_eda_hdl);
		goto err1;
	}

	/* retrieve hdl rec */
	if (NULL == (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evtHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;	/* check */
		TRACE_4("Unable to retrieve event handle: %llx", evtHandle);
		goto err2;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;
			TRACE_4("This node is not a member of the CLM cluster");
			goto err2;
		}
	}

	/* Obtain the selection object from the IPC queue */
	sel_obj = m_NCS_IPC_GET_SEL_OBJ(&hdl_rec->mbx);

	/* everything's fine.. pass the sel fd to the appl */
	*o_sel_obj = (SaSelectionObjectT)m_GET_FD_FROM_SEL_OBJ(sel_obj);

	/* return EDA_CB & hdl rec */
	ncshm_give_hdl(evtHandle);

 err2:
	ncshm_give_hdl(gl_eda_hdl);
 err1:
	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 3.5.3
 *
 * saEvtDispatch()
 *
 * The saEvtDispatch() function invokes, in the context of the calling thread,
 * one or all of the pending callbacks for the handle evtHandle.
 *
 *
 * Parameters
 *
 * evtHandle - [in]
 *   A pointer to the handle, obtained through the saEvtInitialize() function,
 *   designating this particular initialization of the Event Service.
 *
 * dispatchFlags - [in]
 *   Flags that specify the callback execution behavior of the the
 *   saEvtDispatch() function, which have the values SA_DISPATCH_ONE,
 *   SA_DISPATCH_ALL or SA_DISPATCH_BLOCKING, as defined in Section 3.3.8.
 *
 ***************************************************************************/
SaAisErrorT saEvtDispatch(SaEvtHandleT evtHandle, SaDispatchFlagsT dispatchFlags)
{
	EDA_CB *eda_cb = 0;
	EDA_CLIENT_HDL_REC *hdl_rec = 0;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("event handle: %llx", evtHandle);

	if (!m_EDA_DISPATCH_FLAG_IS_VALID(dispatchFlags)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("Invalid dispatchFlags");
		goto err1;
	}

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;	/* check */
		TRACE_4("Unable to retrieve global handle: %u", gl_eda_hdl);
		goto err1;
	}

	/* retrieve hdl rec */
	if (NULL == (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evtHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;	/* check */
		TRACE_4("Unable to retrieve event handle: %llx", evtHandle);
		goto err2;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;
			TRACE_1("This node is not a member of the CLM cluster");
			ncshm_give_hdl(evtHandle);
			ncshm_give_hdl(gl_eda_hdl);
		}
	}

	rc = eda_hdl_cbk_dispatch(eda_cb, hdl_rec, dispatchFlags);

	if (rc != SA_AIS_OK)
		TRACE_1("Callback dispatch returned error");

	if (hdl_rec)
		ncshm_give_hdl(evtHandle);

 err2:
	/* return EDA_CB & hdl rec */
	ncshm_give_hdl(gl_eda_hdl);
 err1:
	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 3.5.4
 *
 * saEvtFinalize()
 *
 * The saEvtFinalize() function closes the association, represented by the
 * evtHandle parameter, between the process and the Event Service.
 * It may free up resources.
 *
 * This function cannot be invoked before the process has invoked the
 * corresponding saEvtInitialize() function for the Event Service. 
 * After this function is invoked, the selection object is no longer valid.
 * Moreover, the Event Service is unavailable for further use unless it is 
 * reinitialized using the saEvtInitialize() function.
 *
 * Parameters
 *
 * evtHandle - [in]
 *   A pointer to the handle, obtained through the saEvtInitialize() function,
 *   designating this particular initialization of the Event Service.
 *
 ***************************************************************************/
SaAisErrorT saEvtFinalize(SaEvtHandleT evtHandle)
{
	EDA_CB *eda_cb = 0;
	EDA_CLIENT_HDL_REC *hdl_rec = 0;
	EDSV_MSG msg;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("event handle: %llx", evtHandle);

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve hdl rec */
	if (NULL == (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evtHandle))) {
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve event handle: %llx", evtHandle);
		TRACE_LEAVE();
		return rc;
	}

	/* Check Whether EDS is up or not */
	if (!eda_cb->eds_intf.eds_up) {
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("event server is not yet up");
		TRACE_LEAVE();
		return rc;
	}
	TRACE("reg_id: %u", hdl_rec->eds_reg_id);
   /** populate & send the finalize message 
    ** and make sure the finalize from the server
    ** end returned before deleting the local records.
    **/
	m_EDA_EDSV_FINALIZE_MSG_FILL(msg, hdl_rec->eds_reg_id);

	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_async_send(eda_cb, &msg, MDS_SEND_PRIORITY_MEDIUM))) {
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("mds send to event server failed");
		TRACE_LEAVE();
		return rc;
	}

	/*
	 * delete the hdl rec 
	 * including all its channel hdls and events
	 * if MDS send is succesful. 
	 */
	if (NCSCC_RC_SUCCESS != (rc = eda_hdl_rec_del(&eda_cb->eda_init_rec_list, hdl_rec))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		/* give handles */
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Handle record cleanup failed");
		TRACE_LEAVE();
		return rc;
	}

	/* give handles */
	ncshm_give_hdl(evtHandle);
	ncshm_give_hdl(gl_eda_hdl);

	ncs_eda_shutdown();
	ncs_agents_shutdown();

	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 3.6.1 
 *
 * saEvtChannelOpen()
 *
 * The saEvtChannelOpen() function creates a new event channel or open an
 * existing channel. The saEvtChannelOpen() function is a blocking operation
 * and returns a new event channel handle.
 *
 * An event channel may be opened multiple times by the same or different
 * processes for publishing, and subscribing to, events. If a process opens
 * an event channel multiple times, it is possible to receive the same event
 * multiple times. However, a process shall never receive an event more than
 * once on a particular event channel handle.
 *
 * If a process opens a channel twice and an event is matched on both open
 * channels, the Event Service performs two callbacks -- one for each opened
 * channel.
 *
 *
 * Parameters
 *
 * evtHandle - [in]
 *   A pointer to the handle, obtained through the saEvtInitialize() function,
 *   designating this particular initialization of the Event Service.
 *
 * channelName - [in]
 *   A pointer to the name of the event channel, which globally identifies an
 *   event channel in a cluster. If the name is already in use within the
 *   cluster, the error SA_AIS_ERR_EXIST will be returned.
 *
 * channelOpenFlags - [in]
 *   The requested access modes of the event channel. The value of this
 *   parameter is obtained by a bitwise OR of the SA_EVT_CHANNEL_PUBLISHER,
 *   SA_EVT_CHANNEL_SUBSCRIBER and SA_EVT_CHANNEL_CREATE flags defined by
 *   SaEvtChannelOpenFlagsT in Section 8.3.3. If SA_EVT_CHANNEL_PUBLISHER is
 *   set, the process may use the returned event channel handle with
 *   saEvtEventPublish(). If SA_EVT_CHANNEL_SUBSCRIBER is set, the process
 *   may use the returned event channel handle with saEvtEventSubscribe().
 *   If SA_EVT_CHANNEL_CREATE is set, the Event Service creates an event
 *   channel if one does not already exist.
 *
 * channelHandle - [in/out]
 *   A pointer to the handle of the event channel, provided by the invoking
 *   process in the address space of the process. If the event channel is
 *   opened successfully, the Event Service stores, in channelHandle, the
 *   handle that the process uses to access the channel in subsequent
 *   invocations of the functions of the Event Service API.
 *
 ***************************************************************************/
SaAisErrorT
saEvtChannelOpen(SaEvtHandleT evtHandle,
		 const SaNameT *channelName,
		 SaEvtChannelOpenFlagsT channelOpenFlags, SaTimeT timeout, SaEvtChannelHandleT *o_chanHdl)
{
	EDA_CB *eda_cb = 0;
	EDA_CLIENT_HDL_REC *hdl_rec = 0;
	EDA_CHANNEL_HDL_REC *chan_hdl_rec = 0;
	EDSV_MSG msg, *o_msg = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t chan_id, timeOut;
	uint32_t chan_open_id;
	TRACE_ENTER2("event handle: %llx", evtHandle);

	if ((NULL == channelName) || (0 == channelName->length) || (SA_MAX_NAME_LENGTH < channelName->length)
		|| (NULL == o_chanHdl) || (strncmp((char *)channelName->value, "safChnl=", 8))) {
		TRACE_2("Invalid input param(s) for channel");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	if (!m_EDA_CHAN_OPEN_FLAG_IS_VALID(channelOpenFlags)) {
		rc = SA_AIS_ERR_BAD_FLAGS;
		TRACE_2("Invalid channel open flags");
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;	/* check */
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve hdl rec */
	if (NULL == (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evtHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve event handle: %llx", evtHandle);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;
			ncshm_give_hdl(evtHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}
   /** Populate a sync MDS message to obtain a channel id and an instance 
    ** open id.
    **/
	m_EDA_EDSV_CHAN_OPEN_SYNC_MSG_FILL(msg, hdl_rec->eds_reg_id, channelOpenFlags, *channelName);

	/* Normalize the timeOut value */
	timeOut = (uint32_t)(timeout / EDSV_NANOSEC_TO_LEAPTM);

	if (timeOut < NCS_SAF_MIN_ACCEPT_TIME) {
		rc = SA_AIS_ERR_TIMEOUT;
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("The timeout is less than minimum supported value of 100 milli seconds");
		TRACE_LEAVE();
		return rc;
	}

	/* Check Whether EDS is up or not */
	if (!eda_cb->eds_intf.eds_up) {
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("event server is not yet up");
		TRACE_LEAVE();
		return rc;
	}

   /** Send a sync MDS message to obtain a channel id and an instance 
    ** open id.
    **/
	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_sync_send(eda_cb, &msg, &o_msg, (uint32_t)timeOut))) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		/* free up the response message */
		if (o_msg)
			eda_msg_destroy(o_msg);
		TRACE_4("mds send to event server failed");
		TRACE_LEAVE();
		return rc;
	}

   /** Make sure the EDS return status was SA_AIS_OK 
    **/
	if (SA_AIS_OK != o_msg->info.api_resp_info.rc) {
		rc = o_msg->info.api_resp_info.rc;
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		/* free up the response message */
		if (o_msg)
			eda_msg_destroy(o_msg);
		TRACE_2("event server returned failure: %u", rc);
		TRACE_LEAVE();
		return rc;
	}

   /** Retrieve the channel id and channel open id params
    ** and pass them into the subroutine.
    **/
	chan_id = o_msg->info.api_resp_info.param.chan_open_rsp.chan_id;
	chan_open_id = o_msg->info.api_resp_info.param.chan_open_rsp.chan_open_id;

	TRACE("reg_id: %u, chan_id: %u, chan_open_id: %u",  hdl_rec->eds_reg_id, chan_id, chan_open_id);

   /** Lock EDA_CB
    **/
	m_NCS_LOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

   /** Allocate an EDA_CHANNEL_HDL_REC structure and 
    ** insert this into the list of channel hdl record.
    **/
	if (NULL == (chan_hdl_rec = eda_channel_hdl_rec_add(&hdl_rec,
							    chan_id, chan_open_id, channelOpenFlags, channelName))) {
		rc = SA_AIS_ERR_NO_RESOURCES;
		m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		/* free up the response message */
		if (o_msg)
			eda_msg_destroy(o_msg);
		TRACE_2("unable to add channel handle to db");
		TRACE_LEAVE();
		return rc;
	}

  /** UnLock EDA_CB
   **/
	m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

   /** Give the hdl-mgr allocated hdl to the application.
    **/
	*o_chanHdl = (SaEvtChannelHandleT)chan_hdl_rec->channel_hdl;

	if (o_msg)
		eda_msg_destroy(o_msg);

	ncshm_give_hdl(evtHandle);
	ncshm_give_hdl(gl_eda_hdl);

	TRACE_LEAVE2("Channel handle: %llx", *o_chanHdl);
	return SA_AIS_OK;
}

/*********************************************************************************************
   3.6.1
   saEvtChannelOpenAsync()

   synopsis: opens an event cahnell according to the semantics of the 
   SAF AIS evt service asynchronously.

   parameters:
      evtHandle - [in] The handle, obtained through the saEvtInitialize() function, designating
      this particular initialization of the Event Service.

      invocation - [in] This parameter allows the invoking component to match this invocation
      of saEvtChannelOpenAsync() with the corresponding callback call.

      channelName - [in] A pointer to the name of the event channel that identifies an event
      channel globally in a cluster. If the event channel already exists, the error
      SA_AIS_ERR_EXIST will be returned.

      channelOpenFlags - [in] The requested access modes of the event channel. The
      value of this parameter is obtained by a bitwise OR of the
      SA_EVT_CHANNEL_PUBLISHER, SA_EVT_CHANNEL_SUBSCRIBER, and
      SA_EVT_CHANNEL_CREATE flags defined by SaEvtChannelOpenFlagsT in section
      3.2.4 on page 14. If SA_EVT_CHANNEL_PUBLISHER is set, the process may
      
   Return: AIS Return codes.

*************************************************************************************************/
SaAisErrorT
saEvtChannelOpenAsync(SaEvtHandleT evtHandle,
		      SaInvocationT invocation, const SaNameT *channelName, SaEvtChannelOpenFlagsT channelOpenFlags)
{
	EDA_CB *eda_cb = 0;
	EDA_CLIENT_HDL_REC *hdl_rec = 0;
	EDSV_MSG msg;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("event handle: %llx", evtHandle);

	if ((NULL == channelName) || (0 == channelName->length) || (SA_MAX_NAME_LENGTH < channelName->length) ||
		(strncmp((char *)channelName->value, "safChnl=", 8))) {
		TRACE_2("Invalid input param(s) for channel");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	if (!m_EDA_CHAN_OPEN_FLAG_IS_VALID(channelOpenFlags)) {
		rc = SA_AIS_ERR_BAD_FLAGS;
		TRACE_2("Invalid channel open flags");
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;	/* check */
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve hdl rec */
	if (NULL == (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evtHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve event handle: %llx", evtHandle);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;
			ncshm_give_hdl(evtHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}

	/* Check if the corresponding cbk was registered */
	if (!hdl_rec->reg_cbk.saEvtChannelOpenCallback) {
		rc = SA_AIS_ERR_INIT;
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("NULL channelOpenCallback specified by user");
		TRACE_LEAVE();
		return rc;
	}

	/* Check Whether EDS is up or not */
	if (!eda_cb->eds_intf.eds_up) {
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("event server is not yet up");
		TRACE_LEAVE();
		return rc;
	}

   /** Populate a MDS message to obtain a channel id and an instance 
    ** open id.
    **/
	m_EDA_EDSV_CHAN_OPEN_ASYNC_MSG_FILL(msg, invocation, hdl_rec->eds_reg_id, channelOpenFlags, *channelName);

   /** Send an async MDS message to obtain a channel id and a channel instance 
    ** open id. The response will return asynchronously in form of a 
    ** callback and will be handle in the callback dispatch subroutines.
    **/
	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_async_send(eda_cb, &msg, MDS_SEND_PRIORITY_MEDIUM))) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("mds send to event server failed");
		TRACE_LEAVE();
		return rc;
	}

   /** We'll create the hdl record for this channel
    ** upon receipt of the response from the EDS
    **/

	ncshm_give_hdl(evtHandle);
	ncshm_give_hdl(gl_eda_hdl);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * 3.6.3 
 *
 * saEvtChannelClose()
 *
 * The saEvtChannelClose() function closes an event channel and frees
 * resources allocated for that event channel in the invoking process.
 * If the event channel is not referenced by any process and does not hold
 * any events with non-zero retention time, the Event Service automatically
 * deletes the event channel from the cluster namespace.
 *
 *
 * Parameters
 *
 * channelHandle - [in]
 *   A pointer to the handle of the event channel to close.
 *
 ***************************************************************************/
SaAisErrorT saEvtChannelClose(SaEvtChannelHandleT channelHandle)
{
	EDA_CB *eda_cb = 0;
	EDA_CLIENT_HDL_REC *hdl_rec = 0;
	EDA_CHANNEL_HDL_REC *chan_hdl_rec;
	EDSV_MSG msg;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("channel handle: %llx", channelHandle);

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;	/* check */
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve channel hdl record */
	if (NULL == (chan_hdl_rec = (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, channelHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve channel handle: %llx", channelHandle);
		TRACE_LEAVE();
		ncshm_give_hdl(gl_eda_hdl);
		return rc;
	}

	/* retrieve the eda client hdl record */
	if (NULL ==
	    (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, chan_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(gl_eda_hdl);
		ncshm_give_hdl(channelHandle);
		TRACE_4("Unable to find the event handle associated with this channel Handle");
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;	/* For now lets return this to a pre-B03 client */
			ncshm_give_hdl(channelHandle);
			ncshm_give_hdl(hdl_rec->local_hdl);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}

	/* Check Whether EDS is up or not */
	if (!eda_cb->eds_intf.eds_up) {
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("event server is not yet up");
		TRACE_LEAVE();
		return rc;
	}


   /** Populate a MDS message to send to the EDS
    ** for a channel close operation.
    **/
	m_EDA_EDSV_CHAN_CLOSE_MSG_FILL(msg, hdl_rec->eds_reg_id,
				       chan_hdl_rec->eds_chan_id, chan_hdl_rec->eds_chan_open_id);

	TRACE("reg_id:%u, chan_id: %u, chan_open_id: %u", hdl_rec->eds_reg_id,
					chan_hdl_rec->eds_chan_id, chan_hdl_rec->eds_chan_open_id);

  /** Lock EDA_CB
   **/
	m_NCS_LOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

   /** Delete this channel & the associated resources with this
    ** instance of channel open.
    **/
	if (NCSCC_RC_SUCCESS != eda_channel_hdl_rec_del(&hdl_rec->chan_list, chan_hdl_rec)) {
		rc = SA_AIS_ERR_LIBRARY;
		m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Failed to cleanup channel records");
		TRACE_LEAVE();
		return rc;
	}

  /** UnLock EDA_CB
   **/
	m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

   /** Send an async message notification to the server
    ** that this instance of channel open is being closed
    ** and so the EDS must take the appropriate actions upon
    ** receipt of this message.
    **/
	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_async_send(eda_cb, &msg, MDS_SEND_PRIORITY_MEDIUM))) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		ncshm_give_hdl(hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("mds send to event server failed");
		TRACE_LEAVE();
		return rc;
	}

  /** Give all the handles that were
   ** taken except the channel hdl as it
   ** must have been already destroyed.
   **/
	ncshm_give_hdl(hdl_rec->local_hdl);
	ncshm_give_hdl(gl_eda_hdl);

	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 3.6.4
 *
 * saEvtChannelUnlink()
 *
 * The saEvtChannelUnlink() function  deletes an existing channel identified
 * by channelName.
 * After completion of the invocation,
 *
 * - channelName is no longer valid, that is, any invocation of a
 *   function of the Event Service API that uses the channel name
 *   saEvtChannelOpen() with channelOpenFlags not set to 
 *   SA_EVT_CHANNEL_CREATE, saEvtChannelUnlink() will return an error.
 *
 * - Any process that has the event channel open can still continue to 
 *   access it. Deletion of the channel will occur when the last 
 *   saEvtChannelClose() operation is performed.
 *
 *
 * Parameters
 *
 * evtHandle - [in] A pointer to the handle, obtained through the
 *   saEvtInitialize() function.
 *
 * channelName - [in] A pointer to the name of the event channel that is to
 *   be unlinked.
 *
 ***************************************************************************/
SaAisErrorT saEvtChannelUnlink(SaEvtHandleT evtHandle, const SaNameT *channelName)
{
	EDA_CB *eda_cb = 0;
	EDA_CLIENT_HDL_REC *hdl_rec = 0;
	EDSV_MSG msg, *o_msg = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("event handle: %llx", evtHandle);

	if ((NULL == channelName) || (0 == channelName->length) || (SA_MAX_NAME_LENGTH < channelName->length) ||
		(strncmp((char *)channelName->value, "safChnl=", 8))) {
		TRACE_2("Invalid channel Name");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
    /** Retrieve EDA CB 
     **/
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;	/* check */
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

    /** Retrieve hdl rec 
     **/
	if (NULL == (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evtHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve event handle: %llx", evtHandle);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;
			ncshm_give_hdl(evtHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}

	/* Check Whether EDS is up or not */
	if (!eda_cb->eds_intf.eds_up) {
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("event server is not yet up");
		TRACE_LEAVE();
		return rc;
	}

   /** Populate a MDS message to send to the EDS
    ** for a channel unlink operation.
    **/
	m_EDA_EDSV_CHAN_UNLINK_MSG_FILL(msg, hdl_rec->eds_reg_id, *channelName);

   /** Send the message out to the EDS
    **/
	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_sync_send(eda_cb, &msg, &o_msg, EDSV_WAIT_TIME))) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		/* free up the response message */
		if (o_msg)
			eda_msg_destroy(o_msg);
		TRACE_4("mds send to event server failed");
		TRACE_LEAVE();
		return rc;
	}

   /** Make sure the EDS return status was SA_AIS_OK 
    **/
	if (SA_AIS_OK != o_msg->info.api_resp_info.rc) {
		rc = o_msg->info.api_resp_info.rc;
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		/* free up the response message */
		if (o_msg)
			eda_msg_destroy(o_msg);
		TRACE_2("event server returned failure: %u", rc);
		TRACE_LEAVE();
		return rc;
	}

  /** Give all the handles that were
   ** taken.
   **/
	ncshm_give_hdl(evtHandle);
	ncshm_give_hdl(gl_eda_hdl);
	if (o_msg)
		eda_msg_destroy(o_msg);

	TRACE_LEAVE();
	return SA_AIS_OK;
}

/***************************************************************************
 * 3.7.1
 *
 * saEvtEventAllocate()
 *
 * The saEvtEventAllocate() function allocates memory for the event, but not
 * for the eventHandle, and initializes all event attributes to default values.
 * The event allocated by saEvtEventAllocate() is freed by invoking
 * saEvtEventFree().
 *
 * The memory associated with the eventHandle is not deallocated by the
 * saEvtEventAllocate() function or the saEvtEventFree() function. It is the
 * responsibility of the invoking process to deallocate the memory for the
 * eventHandle when the process has published the event and has freed the
 * event by invoking saEvtEventFree().
 *
 *
 * Parameters
 *
 * channelHandle - [in]
 *   A pointer to the handle of the event channel on which the event is to be
 *   published.
 *
 * eventHandle - [out]
 *   A pointer to the handle for the newly allocated event. It is the
 *   responsibility of the invoking process to allocate memory for the
 *   eventHandle before invoking this function. The Event Service will assign
 *   the value of the eventHandle when this function is invoked.
 *
 ***************************************************************************/
SaAisErrorT saEvtEventAllocate(SaEvtChannelHandleT channelHandle, SaEvtEventHandleT *o_eventHandle)
{
	EDA_CB *eda_cb = NULL;
	EDA_CLIENT_HDL_REC *hdl_rec = NULL;
	EDA_CHANNEL_HDL_REC *chan_hdl_rec = NULL;
	EDA_EVENT_HDL_REC *evt_hdl_rec = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("channel handle: %llx", channelHandle);

	if (NULL == o_eventHandle) {
		TRACE_2("out param - EventHandle is NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve channel hdl record */
	if (NULL == (chan_hdl_rec = (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, channelHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve channel handle: %llx", channelHandle);
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve the eda client hdl record */
	if (NULL ==
	    (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, chan_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Unable to retrieve client event handle");
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;	/* For now, lets return this to a pre-B03 client as well */
			ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
			ncshm_give_hdl(channelHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
		/* Check if this channel was opened with publisher access */
		if (!(chan_hdl_rec->open_flags & SA_EVT_CHANNEL_PUBLISHER)) {
			rc = SA_AIS_ERR_ACCESS;
			ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
			ncshm_give_hdl(channelHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("Channel was not opened with publish permissions");
			TRACE_LEAVE();
			return rc;
		}
	}

  /** Lock EDA_CB
   **/
	m_NCS_LOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

   /** Allocate an evt record and chain it to the opened channel hdl
    ** record. No need to exchange messages with the EDS here.
    **/
	if (NULL == (evt_hdl_rec = eda_event_hdl_rec_add(&chan_hdl_rec))) {
		rc = SA_AIS_ERR_NO_MEMORY;
		m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Failed to allocate event");
		TRACE_LEAVE();
		return rc;
	}

	/* Set the Default values. 'B' spec */
	evt_hdl_rec->priority = SA_EVT_LOWEST_PRIORITY;
	evt_hdl_rec->publish_time = SA_TIME_UNKNOWN;
	evt_hdl_rec->retention_time = (SaTimeT)0;
	evt_hdl_rec->publisher_name.length = 0;
	memset(evt_hdl_rec->publisher_name.value, '\0', SA_MAX_NAME_LENGTH);
	memcpy(evt_hdl_rec->publisher_name.value, (uint8_t *)"NULL", EDSV_DEF_NAME_LEN);
	if (NULL == (evt_hdl_rec->pattern_array = m_MMGR_ALLOC_EVENT_PATTERN_ARRAY)) {
		rc = SA_AIS_ERR_NO_MEMORY;
		m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("malloc failed for pattern array");
		TRACE_LEAVE();
		return rc;
	}
	evt_hdl_rec->pattern_array->allocatedNumber = 0;
	evt_hdl_rec->pattern_array->patternsNumber = 0;
	evt_hdl_rec->pattern_array->patterns = NULL;

  /** UnLock EDA_CB
   **/

	m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

   /** Return the hdl-mgr returned hdl back to the 
    ** invoking process
    **/
	*o_eventHandle = evt_hdl_rec->event_hdl;
  /** Give all the handles that were
   ** taken.
   **/
	ncshm_give_hdl(channelHandle);
	ncshm_give_hdl(hdl_rec->local_hdl);
	ncshm_give_hdl(gl_eda_hdl);

	TRACE_LEAVE2("Allocated event handle: %llx", *o_eventHandle);
	return SA_AIS_OK;
}

/***************************************************************************
 * 3.7.2
 *
 * saEvtEventFree()
 *
 * The saEvtEventFree() function gives the Event Service premission to
 * deallocate the memory that contains the attributes of the event that is
 * associated with eventHandle. The function is used to free events allocated
 * by saEvtEventAllocate() and by saEvtEventDeliverCallback().
 *
 *
 * Parameters
 *
 * eventHandle - [in]
 *   A pointer to the handle of the event whose memory can now be freed
 *   by the Event Service.
 *
 ***************************************************************************/
SaAisErrorT saEvtEventFree(SaEvtEventHandleT eventHandle)
{
	EDA_CB *eda_cb = NULL;
	EDA_CHANNEL_HDL_REC *chan_hdl_rec = NULL;
	EDA_EVENT_HDL_REC *evt_hdl_rec = NULL;
	EDA_CLIENT_HDL_REC *hdl_rec = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("Allocated event handle: %llx", eventHandle);

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve event hdl record */
	if (NULL == (evt_hdl_rec = (EDA_EVENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, eventHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve allocated event handle: %llx", eventHandle);
		TRACE_LEAVE();
		return rc;
	}

	if (evt_hdl_rec->parent_chan) {	/* Check if channel still exists */
		if (evt_hdl_rec->parent_chan->channel_hdl) {
			/* retrieve the eda channel hdl record */
			if (NULL ==
			    (chan_hdl_rec =
			     (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA,
								   evt_hdl_rec->parent_chan->channel_hdl))) {
				rc = SA_AIS_ERR_LIBRARY;
				ncshm_give_hdl(eventHandle);
				ncshm_give_hdl(gl_eda_hdl);
				TRACE_4("Unable to locate channel handle record");
				TRACE_LEAVE();
				return rc;
			}
		} else {
			ncshm_give_hdl(eventHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_4("Channel handle is null!");
			TRACE_LEAVE();
			return SA_AIS_ERR_LIBRARY;
		}		/* End if channel_hdl */
	} else {
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("channel info does not exist");
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;
	}			/* End if parent_chan */

	/* retrieve the eda client hdl record */
	if (NULL ==
	    (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, chan_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Unable to retrieve clienthandle associated with this event: %llx", eventHandle);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;	/* For now, lets return this to a pre-B03 client as well */
			ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
			ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
			ncshm_give_hdl(eventHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}

	ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
  /** Lock EDA_CB synchronize access with MDS thread.
   **/
	m_NCS_LOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

   /** Delete this evt record from the
    ** list of events
    **/

	if (NCSCC_RC_SUCCESS != eda_event_hdl_rec_del(&chan_hdl_rec->chan_event_anchor, evt_hdl_rec)) {
		rc = SA_AIS_ERR_LIBRARY;
		m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(chan_hdl_rec->channel_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Failed to delete the allocated event");
		TRACE_LEAVE();
		return rc;
	}

  /** UnLock EDA_CB
   **/
	m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

  /** Give all the handles that were
   ** taken back except the eventHandle as 
   ** it must have been already destroyed.
   **/

	ncshm_give_hdl(chan_hdl_rec->channel_hdl);
	ncshm_give_hdl(gl_eda_hdl);

	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 3.7.3
 *
 * saEvtEventAttributesSet()
 *
 * This function may be used to assign writeable event attributes. It takes
 * as arguments an event handle eventHandle and the attribute to be set in
 * the event structure of the event with that handle.
 * Note: The only attributes that a process can set are the patternArray,
 * priority, retentionTime and publisherName attributes.
 *
 *
 * Parameters
 *
 * eventHandle - [in]
 *   A pointer to the handle of the event whose attributes are to be set.
 *
 * patternArray - [in]
 *   A pointer to a structure that contains the array of patterns to be
 *   placed into the event pattern array and the number of such patterns.
 *
 * priority - [in]
 *   The priority of the event.
 *
 * retentionTime - [in]
 *   The duration for which the event will be retained.
 *
 * publisherName - [in]
 *   A pointer to the name of the publisher of the event.
 *
 ***************************************************************************/
SaAisErrorT
saEvtEventAttributesSet(SaEvtEventHandleT eventHandle,
			const SaEvtEventPatternArrayT *patternArray,
			SaEvtEventPriorityT priority, SaTimeT retentionTime, const SaNameT *publisherName)
{
	EDA_EVENT_HDL_REC *evt_hdl_rec = NULL;
	EDA_CHANNEL_HDL_REC *chan_hdl_rec;
	EDA_CLIENT_HDL_REC *hdl_rec = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	SaEvtEventPatternArrayT *def_pattern_array;
	EDA_CB *eda_cb = NULL;
	TRACE_ENTER2("Allocated event handle: %llx", eventHandle);

	if (publisherName != NULL) {
		if ((SA_MAX_NAME_LENGTH < publisherName->length) || (0 == publisherName->length)) {
			TRACE_2("Invalid publisher name");
			TRACE_LEAVE();
			return SA_AIS_ERR_INVALID_PARAM;
		}
	}

	if (priority > SA_EVT_LOWEST_PRIORITY) {	/*Need not check for < HIGHEST_PRIORITY, as its unsigned */
		TRACE_2("Invalid event priority");
		TRACE_LEAVE();
		return (SA_AIS_ERR_INVALID_PARAM);
	}

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve event hdl record */
	if (NULL == (evt_hdl_rec = (EDA_EVENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, eventHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve allocated event handle: %llx", eventHandle);
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve channel hdl record */
	if (NULL ==
	    (chan_hdl_rec =
	     (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evt_hdl_rec->parent_chan->channel_hdl))) {
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Failed to retreive channel handle associated with this event");
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve the eda client hdl record */
	if (NULL ==
	    (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, chan_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Failed to retrieve client handle associated with this channelHandle: %u",
								chan_hdl_rec->parent_hdl->local_hdl);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;	/* For now, lets return this to a pre B03 client as well */
			ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
			ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
			ncshm_give_hdl(eventHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}
	ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
	ncshm_give_hdl(gl_eda_hdl);
   /** if this event was received as opposed to 'to be 
    ** published'
    **/

	if (evt_hdl_rec->evt_type & EDA_EVT_RECEIVED) {
		rc = SA_AIS_ERR_ACCESS;
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(chan_hdl_rec->channel_hdl);
		TRACE_2("This is a received event and not allocated. Not permitted to change attributes");
		TRACE_LEAVE();
		return rc;
	}

   /** The channel
    ** has been opened with subscriber access only.
    **/

	if ((chan_hdl_rec->open_flags & SA_EVT_CHANNEL_SUBSCRIBER)
	    && !(chan_hdl_rec->open_flags & SA_EVT_CHANNEL_PUBLISHER)) {
		rc = SA_AIS_ERR_ACCESS;
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(chan_hdl_rec->channel_hdl);
		TRACE_2("The channel is opened without publisher access");
		TRACE_LEAVE();
		return rc;
	}

	if (retentionTime > EDSV_MAX_RETENTION_TIME) {
		rc = SA_AIS_ERR_TOO_BIG;
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(chan_hdl_rec->channel_hdl);
		TRACE_2("The retention time is too big. Supported value is 100000000000000.0 in nano seconds");
		TRACE_LEAVE();
		return rc;
	}

   /** Make a copy of the pattern array for library's use
    **/
	if (NULL != patternArray) {
		/* If event still contains default pattern,
		   free the memory allocated during saEvtEventAlloc.
		   edsv_copy_evt_pattern_array() shall alloc new memory
		 */
		def_pattern_array = evt_hdl_rec->pattern_array;

		if ((NULL == (evt_hdl_rec->pattern_array =
			      edsv_copy_evt_pattern_array(patternArray, &rc))) || (rc != SA_AIS_OK)) {
			ncshm_give_hdl(eventHandle);
			ncshm_give_hdl(chan_hdl_rec->channel_hdl);
			/* Restore the previous pattern array memory */
			evt_hdl_rec->pattern_array = def_pattern_array;
			TRACE_2("Unable to copy the pattern array locally");
			TRACE_LEAVE();
			return rc;
		}
		if (def_pattern_array) {
			edsv_free_evt_pattern_array(def_pattern_array);
			/*m_MMGR_FREE_EVENT_PATTERN_ARRAY(def_pattern_array); */
		}
	}

   /** Save attributes in this event record
    **/
	evt_hdl_rec->priority = priority;
	evt_hdl_rec->retention_time = retentionTime;
	if (NULL != publisherName)
		evt_hdl_rec->publisher_name = *publisherName;

	/* Give back the event handle */
	ncshm_give_hdl(eventHandle);
	ncshm_give_hdl(chan_hdl_rec->channel_hdl);
	TRACE_LEAVE();
	return (SA_AIS_OK);
}

/***************************************************************************
 * 3.7.4
 *
 * saEvtEventAttributesGet()
 *
 * This function takes as parameters an event handle eventHandle and the
 * attributes of the event with that handle. The function retrieves the value
 * of the attributes for the event and stores them at the address provided
 * for the out parameters.
 * 
 * It is the responsibility of the invoking process to allocate memory for
 * the out parameters before it invokes this function. The Event Service
 * assigns the out values into that memory. For each of the out, or in/out,
 * parameters, if the invoking process provides a NULL reference, the
 * Availability Management Framework does not return the out value.
 *
 * Similarly, it is the responsibility of the invoking process to allocate
 * memory for the patternArray.
 *
 *
 * Parameters
 *
 * eventHandle - [in]
 *   A pointer to the handle of the event whose attributes are to be retrieved.
 *
 * patternArray - [in/out]
 *   A pointer to a structure that contains the array of patterns to be
 *   retrieved from the event pattern array and the number of such patterns.
 *   A process that invokes this function provides the patternArray, and the
 *   Event Service inserts the patterns into the successive entries of the
 *   patternArray, starting with the first entry and continuing until the
 *   patterns are exhausted. 
 *   The number of patterns that the Event Service
 *   inserts into the patternArray is the minimum of the number of patterns
 *   for the event and the patternsNumber value of the in value of
 *   patternArray, supplied by the process. If there are more patterns than
 *   patternsNumber, the Event Service does not insert those additional
 *   patterns. The pattternsNumber value of the out value of patternArray,
 *   supplied by the Event Service, can be less than, equal to, or greater
 *   than the value of patternsNumber, supplied by the process.
 *
 * priority - [out]
 *   A pointer to the priority of the event.
 *
 * retentionTime - [out]
 *   A pointer to the duration for which the publisher will retain the event.
 *
 * publisherName - [out]
 *   A pointer to the name of the publisher of the event.
 *
 * publishTime - [out]
 *   A pointer to the time at which the publisher published the event.
 *
 * eventId - [out]
 *   A pointer to an identifier of the event.
 *
 ***************************************************************************/
SaAisErrorT
saEvtEventAttributesGet(SaEvtEventHandleT eventHandle,
			SaEvtEventPatternArrayT *o_patternArray,
			SaEvtEventPriorityT *o_priority,
			SaTimeT *o_retentionTime,
			SaNameT *o_publisherName, SaTimeT *o_publishTime, SaEvtEventIdT *o_eventId)
{
	EDA_EVENT_HDL_REC *evt_hdl_rec = NULL;
	EDA_CHANNEL_HDL_REC *chan_hdl_rec;
	EDA_CLIENT_HDL_REC *hdl_rec = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	EDA_CB *eda_cb = NULL;
	TRACE_ENTER2("Allocated event handle: %llx", eventHandle);

	if (NULL == o_patternArray) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("null pattern array");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	/* Retrieve event hdl record */
	if (NULL == (evt_hdl_rec = (EDA_EVENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, eventHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve allocated event handle: %llx", eventHandle);
		TRACE_LEAVE();
		return rc;
	}
	/* retrieve channel hdl record */
	if (NULL ==
	    (chan_hdl_rec =
	     (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evt_hdl_rec->parent_chan->channel_hdl))) {
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Failed to retreive channel handle associated with this event");
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve client handle revord */
	if (NULL ==
	    (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, chan_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Failed to retrieve client handle associated with this channelHandle: %u", 
										chan_hdl_rec->parent_hdl->local_hdl);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;
			ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
			ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
			ncshm_give_hdl(eventHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}
	/* give away handle anyway */
	ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
	ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
	ncshm_give_hdl(gl_eda_hdl);
	/* Check if user has provided memory for patterns,
	 * If not, allocate memory for patterns 
	 */
	if (NULL == o_patternArray->patterns) {
		if (evt_hdl_rec->pattern_array == NULL) {
			rc = SA_AIS_ERR_NO_MEMORY;
			ncshm_give_hdl(eventHandle);
			TRACE_2("patternArray of the original allocated event is NULL");
			TRACE_LEAVE();
			return rc;
		}
		/* NOTE: The current form of 'B' spec expects the service to allocate
		 * memory for pattern_array if user sets pattern_array->patterns to NULL,
		 * But this is not possible because o_patternArray is defined as a single
		 * pointer.
		 * SAF spec needs a change. Currently  the service will return
		 * any value only in the case when the event contains
		 * patternArray set to default values, otherwise edsv returns SA_AIS_ERR_NO_MEMORY
		 */

		/* Check if pattern array is containing default value */
		if (evt_hdl_rec->pattern_array->patternsNumber == 0) {
			o_patternArray->patternsNumber = 0;
			o_patternArray->allocatedNumber = 0;
			o_patternArray->patterns = NULL;	/* Default value */
		} else {

			if (SA_AIS_OK != (rc = eda_allocate_and_extract_pattern_from_event(evt_hdl_rec->pattern_array,
											   &o_patternArray))) {
				ncshm_give_hdl(eventHandle);
				TRACE_2("Unable to allocate patterns");
				TRACE_LEAVE();
				return rc;
			}

		}		/*end else - evt_hdl_rec->parray->patterns number == 0 */
	} /* End if o_patternArray->patterns == NULL */
	else {
		/* User has provided memory for patterns, just fill it */
		if (SA_AIS_OK != (rc = eda_extract_pattern_from_event(evt_hdl_rec->pattern_array, &o_patternArray))) {
			ncshm_give_hdl(eventHandle);
			TRACE_2("Unable to fill patterns into the user provided memory");
			TRACE_LEAVE();
			return rc;
		}
	}
	/* Retrieve the parameters that have been
	 * sought by the invoking process
	 */
	if (o_priority)
		*o_priority = evt_hdl_rec->priority;	/*Set priority */

	if (o_retentionTime)
		*o_retentionTime = evt_hdl_rec->retention_time;	/*Set reten time */

	/* check if publisher name length is invalid */
	if (evt_hdl_rec->publisher_name.length > SA_MAX_NAME_LENGTH) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(eventHandle);
		TRACE_4("Publisher name is longer than SA_MAX_NAME_LENGTH");
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;	/* corrupt length */
	}

	/* Set publisher name */
	if (o_publisherName) {
		o_publisherName->length = evt_hdl_rec->publisher_name.length;
		memcpy((void *)o_publisherName->value, (void *)evt_hdl_rec->publisher_name.value,
		       evt_hdl_rec->publisher_name.length);
	}

	/* Set publish time */
	if (o_publishTime)
		*o_publishTime = evt_hdl_rec->publish_time;

	if (o_eventId) {
		if (evt_hdl_rec->evt_type & EDA_EVT_RECEIVED) {
	   /** for now we'll use the eventHdl from hdl-mgr as the evtId **/
			*o_eventId = evt_hdl_rec->del_evt_id;
		} else {
			*o_eventId = SA_EVT_EVENTID_NONE;	/*Default value */
		}
	}
	/* Release the event handle */
	ncshm_give_hdl(eventHandle);

	TRACE_LEAVE();
	return (rc);
}

/***************************************************************************
 * 3.7.5
 *
 *  saEvtEventPatternFree()
 *
 * This function frees the patterns allocated by the Event service during
 * previous invocation of a SaEvtEventAttributesGet(). 
 *
 * Parameters
 *
 * eventHandle - [in] A handle obtained during a previous invocation of 
 *                   saEvtEventAllocate() or during saEvtEventDeliverCallback()
 * patterns - [in] A pointer to the patterns allocated by the Event service 
 *                 during previous invocation of a SaEvtEventAttributesGet().
 *
 ***************************************************************************/

extern SaAisErrorT saEvtEventPatternFree(SaEvtEventHandleT eventHandle, SaEvtEventPatternT *patterns)
{
	SaAisErrorT rc = SA_AIS_OK;
	EDA_EVENT_HDL_REC *evt_hdl_rec = NULL;
	EDA_CHANNEL_HDL_REC *chan_hdl_rec;
	EDA_CLIENT_HDL_REC *hdl_rec = NULL;
	EDA_CB *eda_cb = NULL;
	TRACE_ENTER2("Allocated event handle: %llx", eventHandle);

	if (patterns == NULL) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("patterns is NULL");
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	/* Retrieve event hdl record */
	if (NULL == (evt_hdl_rec = (EDA_EVENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, eventHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve allocated event handle: %llx", eventHandle);
		TRACE_LEAVE();
		return rc;
	}
	/* retrieve channel hdl record */
	if (NULL ==
	    (chan_hdl_rec =
	     (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evt_hdl_rec->parent_chan->channel_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Failed to retreive channel handle associated with this event");
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve client handle revord */
	if (NULL ==
	    (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, chan_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Failed to retrieve client handle associated with this channelHandle: %u", 
										chan_hdl_rec->parent_hdl->local_hdl);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;
			ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
			ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
			ncshm_give_hdl(eventHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}
	/* give away handle snyway */
	ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
	ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);

	if ((evt_hdl_rec->pattern_array == NULL)) {
		/* The pattern was already freed!? */
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("pattern array is NULL");
		TRACE_LEAVE();
		return rc;
	}

	/* Free the patterns */
	eda_free_event_patterns(patterns, evt_hdl_rec->pattern_array->patternsNumber);

	ncshm_give_hdl(eventHandle);
	ncshm_give_hdl(gl_eda_hdl);

	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 3.7.6
 *
 * saEvtEventDataGet()
 *
 * The saEvtEventDataGet() function allows a process to retrieve the data
 * associated with an event previously delivered by
 * saEvtEventDeliverCallback().
 *
 *
 * Parameters
 *
 * eventHandle - [in]
 *   A pointer to the handle to the event previously delivered by
 *   saEvtEventDeliverCallback().
 *
 * eventData - [in/out]
 *   A pointer to a buffer provided by the process in which the Event Service
 *   stores the data associated with the delivered event.
 *
 * eventDataSize - [in/out]
 *   The in value of eventDataSize is the size of the eventData buffer
 *   provided by the invoking process. The Event Service must not write more
 *   than eventDataSize bytes into the eventData buffer. The out value of
 *   eventDataSize is the size of the eventData of the event, which may be
 *   less than, equal to, or greater than the in value of eventDataSize.
 *   Note:
 *   An eventData buffer of size SA_EVENT_DATA_MAX_SIZE bytes or more
 *   will always be able to contain the largest possible event data
 *   associated with an event.
 *
 ***************************************************************************/
SaAisErrorT saEvtEventDataGet(SaEvtEventHandleT eventHandle, void *eventData, SaSizeT *eventDataSize)
{
	SaAisErrorT rc = SA_AIS_OK;
	EDA_EVENT_HDL_REC *evt_hdl_rec = NULL;
	EDA_CHANNEL_HDL_REC *chan_hdl_rec;
	EDA_CLIENT_HDL_REC *hdl_rec = NULL;
	EDA_CB *eda_cb = NULL;
	TRACE_ENTER2("Received event handle: %llx", eventHandle);

	/* Make sure the passed in arguments
	 * are valid.
	 */

	if ((eventDataSize == NULL) || (eventData == NULL)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("outparam(s) is NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

   /** Retrieve event hdl record 
    **/
	if (NULL == (evt_hdl_rec = (EDA_EVENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, eventHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve allocated event handle: %llx", eventHandle);
		TRACE_LEAVE();
		return rc;
	}
	/* retrieve channel hdl record */
	if (NULL ==
	    (chan_hdl_rec =
	     (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evt_hdl_rec->parent_chan->channel_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Failed to retreive channel handle associated with this event");
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve client handle revord */
	if (NULL ==
	    (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, chan_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Failed to retrieve client handle associated with this channelHandle: %u",
										chan_hdl_rec->parent_hdl->local_hdl);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;	/* For now, lets return this to a pre B03 client as well */
			ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
			ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
			ncshm_give_hdl(eventHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}

	ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
	ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
	ncshm_give_hdl(gl_eda_hdl);

   /** Make sure this event was received and not published
    **/
	if (!(evt_hdl_rec->evt_type & EDA_EVT_RECEIVED)) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(eventHandle);
		TRACE_2("This event is not a published event, its an allocated event");
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* Has Enough room been provided to return the data 
	 */

	/*if (eventData == NULL)
	   {
	   rc = SA_AIS_ERR_NO_SPACE;
	   *eventDataSize = evt_hdl_rec->event_data_size;
	   ncshm_give_hdl(eventHandle);
	   return rc;
	   } */

	if (*eventDataSize < evt_hdl_rec->event_data_size) {
		rc = SA_AIS_ERR_NO_SPACE;
		*eventDataSize = evt_hdl_rec->event_data_size;
		ncshm_give_hdl(eventHandle);
		TRACE_2("eventDataSize is smaller than the received event data size: %llx", evt_hdl_rec->event_data_size);
		TRACE_LEAVE();
		return rc;
	}

   /** Return the datasize
    **/
	*eventDataSize = evt_hdl_rec->event_data_size;

   /** Return the data
    **/
	if (evt_hdl_rec->event_data_size) {
		if (evt_hdl_rec->evt_data != NULL) {
			memcpy((void *)eventData, (void *)evt_hdl_rec->evt_data, (uint32_t)evt_hdl_rec->event_data_size);

		}
	}

   /** Give back the event handle 
    **/
	ncshm_give_hdl(eventHandle);

	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 3.7.8 
 *
 * saEvtEventPublish()
 *
 * The saEvtEventPublish() function publishes an event on the channel
 * designated by channelHandle. The event to be published consists of a
 * standard set of attributes (the event header) and an optional data part.
 *
 * Before an event can be published, the publisher process must invoke the
 * saEvtEventPatternArraySet() function to set the event patterns. The event
 * is delivered to subscribers whose subscription filter matches the event
 * patterns.
 *
 * When the Event Service publishes an event, it automatically sets the
 * following readonly event attributes:
 *   . Event publish time  
 *   . Event identifier
 *
 * In addition to the event attributes, a process can supply values for the
 * eventData and eventDataSize parameters for publication as part of the event.
 * The data portion of the event may be at most SA_EVT_DATA_MAX_LEN bytes in
 * length.
 *
 * The process may assume that the invocation of saEvtEventPublish() copies
 * all pertinent parameters, including the memory associated with the
 * eventHandle and eventData parameters, to its own local memory. However,
 * the invocation does not automatically deallocate memory associated with
 * the eventHandle and eventData parameters. It is the responsibility of the
 * invoking process to deallocate the memory for those parameters after
 * saEvtEventPublish() returns.
 *
 *
 * Parameters
 *
 * eventHandle - [in]
 *   A pointer to the handle of the event that is to be published. The event
 *   must have been allocated by saEvtEventAllocate() and the patterns must
 *   have been set by saEvtEvenPatternArraySet().
 *
 * eventData - [in]
 *   A pointer to a buffer that contains additional event information for the
 *   event being published. This parameter is set to NULL if no additional
 *   information is associated with the event. The process may deallocate
 *   the memory for eventData when saEvtEventPublish() returns.
 *
 * eventDataSize - [in]
 *   The number of bytes in the buffer pointed to by eventData. 
 *   eventDataSize should not be greater than SA_EVENT_DATA_MAX_SIZE. 
 *
 * eventId - [out]
 *
 ***************************************************************************/
SaAisErrorT
saEvtEventPublish(SaEvtEventHandleT eventHandle, const void *eventData, SaSizeT eventDataSize, SaEvtEventIdT *o_eventId)
{
	SaAisErrorT rc = SA_AIS_OK;
	EDA_EVENT_HDL_REC *evt_hdl_rec = NULL;
	EDA_CHANNEL_HDL_REC *chan_hdl_rec = NULL;
	EDA_CLIENT_HDL_REC *hdl_rec;
	EDSV_MSG msg;
	EDA_CB *eda_cb = NULL;
	TRACE_ENTER2("Allocated event handle: %llx", eventHandle);

	/* Event data size should not be greater than SA_EVENT_DATA_MAX_SIZE */
	if (SA_EVENT_DATA_MAX_SIZE < eventDataSize) {
		rc = SA_AIS_ERR_TOO_BIG;
		TRACE_2("eventDataSize is bigger than %u bytes", SA_EVENT_DATA_MAX_SIZE);
		TRACE_LEAVE();
		return rc;
	}
	if (o_eventId == NULL) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("eventId out param is null");
		TRACE_LEAVE();
		return rc;
	}

   /** retrieve EDA CB 
    **/
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

   /** Retrieve event hdl record 
    **/
	if (NULL == (evt_hdl_rec = (EDA_EVENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, eventHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve allocated event handle: %llx", eventHandle);
		TRACE_LEAVE();
		return rc;
	}
	/* retrieve channel hdl record */
	if (NULL ==
	    (chan_hdl_rec =
	     (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evt_hdl_rec->parent_chan->channel_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Failed to retreive channel handle associated with this event");
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve client handle revord */
	if (NULL ==
	    (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, chan_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Failed to retrieve client handle associated with this channelHandle: %u",
										chan_hdl_rec->parent_hdl->local_hdl);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;	/* For now, lets return this to a pre B03 client as well */
			ncshm_give_hdl(chan_hdl_rec->parent_hdl->local_hdl);
			ncshm_give_hdl(evt_hdl_rec->parent_chan->channel_hdl);
			ncshm_give_hdl(eventHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}

   /** Make sure that some event attributes were set properly 
    **/
	if ((NULL == evt_hdl_rec->pattern_array) || (SA_MAX_NAME_LENGTH < evt_hdl_rec->publisher_name.length)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(chan_hdl_rec->channel_hdl);
		ncshm_give_hdl(hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Ensure pattern array is not NULL and publishername length is > SA_MAX_NAME_LENGTH");
		TRACE_LEAVE();
		return rc;
	}

	/* Make sure a patternArray has been set for this event */
	if (NULL == evt_hdl_rec->pattern_array) {
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(chan_hdl_rec->channel_hdl);
		ncshm_give_hdl(hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("pattern array is not set");
		TRACE_LEAVE();
		return rc;
	}

	/* Make sure the channel was opened for publish access */
	if (!(chan_hdl_rec->open_flags & SA_EVT_CHANNEL_PUBLISHER)) {
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(chan_hdl_rec->channel_hdl);
		ncshm_give_hdl(hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_ACCESS;
		TRACE_2("The channel is not opened with publisher access");
		TRACE_LEAVE();
		return rc;
	}

	/* Check Whether EDS is up or not */
	if (!eda_cb->eds_intf.eds_up_publish) {
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(chan_hdl_rec->channel_hdl);
		ncshm_give_hdl(hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("Event server is not yet up");
		TRACE_LEAVE();
		return rc;
	}

   /** Initialize the data length & event id field in the evt 
    **/
	if (eventData != NULL)
		evt_hdl_rec->event_data_size = eventDataSize;
	else
		evt_hdl_rec->event_data_size = 0;

   /** Lock EDA_CB
    **/
	m_NCS_LOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

   /** Free data first if any
    **/
	if (NULL != evt_hdl_rec->evt_data) {
		m_MMGR_FREE_EDSV_EVENT_DATA(evt_hdl_rec->evt_data);
		evt_hdl_rec->evt_data = NULL;
	}

   /** make sure this event has some data
    **/
	if (0 != evt_hdl_rec->event_data_size) {
     /** Allocate and copy data 
      **/
		if (NULL == (evt_hdl_rec->evt_data = (uint8_t *)
			     m_MMGR_ALLOC_EDSV_EVENT_DATA((uint32_t)eventDataSize))) {
			m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);
			ncshm_give_hdl(eventHandle);
			ncshm_give_hdl(chan_hdl_rec->channel_hdl);
			ncshm_give_hdl(hdl_rec->local_hdl);
			ncshm_give_hdl(gl_eda_hdl);
			rc = SA_AIS_ERR_NO_MEMORY;
			TRACE_2("malloc failed for event data");
			TRACE_LEAVE();
			return (SA_AIS_ERR_NO_MEMORY);
		}

		if (eventData != NULL) {
			memcpy((void *)evt_hdl_rec->evt_data, (void *)eventData, (uint32_t)evt_hdl_rec->event_data_size);
		}
	}

	evt_hdl_rec->pub_evt_id = ++chan_hdl_rec->last_pub_evt_id;

   /** UnLock EDA_CB
    **/
	m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

   /** populate the mds message to send across to the EDS
    ** whence it will possibly get published
    **/
	m_EDA_EDSV_PUBLISH_MSG_FILL(msg,
				    hdl_rec->eds_reg_id,
				    chan_hdl_rec->eds_chan_id,
				    chan_hdl_rec->eds_chan_open_id,
				    evt_hdl_rec->pattern_array,
				    evt_hdl_rec->priority,
				    evt_hdl_rec->retention_time,
				    evt_hdl_rec->publisher_name,
				    evt_hdl_rec->pub_evt_id,
				    (uint32_t)evt_hdl_rec->event_data_size, evt_hdl_rec->evt_data);

  /** Send the message out to the EDS
   **/
	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_async_send(eda_cb, &msg, MDS_SEND_PRIORITY_MEDIUM))) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		ncshm_give_hdl(eventHandle);
		ncshm_give_hdl(chan_hdl_rec->channel_hdl);
		ncshm_give_hdl(hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("mds send failed for the event publish");
		TRACE_LEAVE();
		return rc;
	}

   /** mark the event as published
    **/
	evt_hdl_rec->evt_type |= EDA_EVT_PUBLISHED;

	/* Return the vent_id for this event */
	*o_eventId = evt_hdl_rec->pub_evt_id;

   /** Give all the channels that have been 
    ** taken.
    **/
	ncshm_give_hdl(eventHandle);
	ncshm_give_hdl(chan_hdl_rec->channel_hdl);
	ncshm_give_hdl(hdl_rec->local_hdl);
	ncshm_give_hdl(gl_eda_hdl);

	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 3.7.9
 *
 * saEvtEventSubscribe()
 *
 * The saEvtEventSubscribe() function enables a process to subscribe for
 * events on an event channel by registering one or more filters on that
 * event channel. The process must have opened the event channel, designated
 * by channelHandle, with the SA_EVT_CHANNEL_SUBSCRIBER flag set for an
 * invocation of this function to be successful.
 *
 * The memory associated with the filters is not deallocated by the
 * saEvtEventSubscribe() function. It is the responsibility of the invoking
 * process to deallocate the memory when the saEvtEventSubscribe() function
 * returns.
 *
 * For a given subscription, the filters parameter cannot be modified.
 * To change the filtersparameter without losing events, a process must
 * establish a new subscription with the new filters parameter. After the new
 * subscription is established, the old subscription can be removed by
 * invoking the saEvtEventUnsubscribe() function.
 *
 *
 * Parameters
 *
 * channelHandle - [in]
 *   A pointer to the handle of the event channel on which the process is
 *   subscribing to receive events. The event channel handle is returned
 *   from a previous invocation of the saEvtChannelOpen() function.
 *
 * filters - [in]
 *   A pointer to a SaEvtEventFilterArrayT structure that defines filter
 *   patterns to use to filter events received on the event channel.
 *   The process may deallocate the memory for the filters when
 *   saEvtEventSubscribe() returns.
 *
 * subscriptionId - [in]
 *   An identifier that uniquely identifies a specific subscription on an
 *   event channel and that is used as a parameter of
 *   saEvtEventDeliverCallback().
 *
 ***************************************************************************/
SaAisErrorT
saEvtEventSubscribe(SaEvtChannelHandleT channelHandle,
		    const SaEvtEventFilterArrayT *filters, SaEvtSubscriptionIdT i_subscriptionId)
{
	SaAisErrorT rc = SA_AIS_OK;
	EDA_CB *eda_cb = NULL;
	EDA_CHANNEL_HDL_REC *channel_hdl_rec = NULL;
	EDA_CLIENT_HDL_REC *eda_hdl_rec = NULL;
	EDSV_MSG msg;
	EDA_SUBSC_REC *subsc_rec = NULL;
	SaEvtEventFilterArrayT *filter_array = NULL;
	TRACE_ENTER2("channel handle: %llx", channelHandle);

   /** Make sure passed arguments were valid
    **/
	if (filters == NULL) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("filters is NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
   /** retrieve EDA_CB 
    **/
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

   /** retrieve channel_hdl_rec
    **/
	if (NULL == (channel_hdl_rec = (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, channelHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve channel handle: %llx", channelHandle);
		TRACE_LEAVE();
		return rc;
	}

   /** retrieve EDA client hdl record
    **/
	if (NULL ==
	    (eda_hdl_rec =
	     (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, channel_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Failed to retrieve client handle associated with this channelHandle: %u",
										channel_hdl_rec->parent_hdl->local_hdl);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&eda_hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;	/* For now, lets return this to a pre-B03 client as well */
			ncshm_give_hdl(channel_hdl_rec->parent_hdl->local_hdl);
			ncshm_give_hdl(channelHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}
	/* Check if the corresponding cbk was registered */
	if (!eda_hdl_rec->reg_cbk.saEvtEventDeliverCallback) {
		rc = SA_AIS_ERR_INIT;
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Event deliver callback is NULL");
		TRACE_LEAVE();
		return rc;
	}

   /** Make sure the channel was opened with subscriber access 
    **/
	if (!(channel_hdl_rec->open_flags & SA_EVT_CHANNEL_SUBSCRIBER)) {
		rc = SA_AIS_ERR_ACCESS;
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Channel was not opened with subscriber access");
		TRACE_LEAVE();
		return SA_AIS_ERR_ACCESS;
	}

	/* Check Whether EDS is up or not */
	if (!eda_cb->eds_intf.eds_up) {
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("event server is not yet up");
		TRACE_LEAVE();
		return rc;
	}

   /** allocate a new filter array **/
	if ((NULL == (filter_array = edsv_copy_evt_filter_array(filters, &rc))) || (rc != SA_AIS_OK)) {
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to copy filters into the filter array");
		TRACE_LEAVE();
		return rc;
	}

   /** make sure the subscription id is unique **/
	if (NULL != eda_find_subsc_by_subsc_id(channel_hdl_rec, i_subscriptionId)) {
		rc = SA_AIS_ERR_EXIST;
		edsv_free_evt_filter_array(filter_array);
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Subscription Id already exists, it must be unique");
		TRACE_LEAVE();
		return (SA_AIS_ERR_EXIST);
	} else {
		/* insert the subscription id into the channel rec */
		if (NULL == (subsc_rec = m_MMGR_ALLOC_EDA_SUBSC_REC)) {
			rc = SA_AIS_ERR_NO_MEMORY;
			edsv_free_evt_filter_array(filter_array);
			ncshm_give_hdl(channelHandle);
			ncshm_give_hdl(eda_hdl_rec->local_hdl);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_4("malloc failed for subscription record");
			TRACE_LEAVE();
			return SA_AIS_ERR_NO_MEMORY;
		}

		memset(subsc_rec, '\0', sizeof(EDA_SUBSC_REC));
		subsc_rec->subsc_id = i_subscriptionId;
		subsc_rec->next = channel_hdl_rec->subsc_list;
		channel_hdl_rec->subsc_list = subsc_rec;
	}

  /** populate the mds message to send across to the EDS
   ** where the subecription record will get installed.
   **/
	m_EDA_EDSV_SUBSCRIBE_MSG_FILL(msg,
				      eda_hdl_rec->eds_reg_id,
				      channel_hdl_rec->eds_chan_id,
				      channel_hdl_rec->eds_chan_open_id, i_subscriptionId, filter_array);

  /** Send the message out to the EDS to install this
   ** subscription.
   **/
	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_async_send(eda_cb, &msg, MDS_SEND_PRIORITY_MEDIUM))) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		edsv_free_evt_filter_array(filter_array);
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("mds send failed for the subscription");
		TRACE_LEAVE();
		return rc;
	}

	/* free filter array */
	edsv_free_evt_filter_array(filter_array);

	/* Give back the handle */
	ncshm_give_hdl(channelHandle);
	ncshm_give_hdl(eda_hdl_rec->local_hdl);
	ncshm_give_hdl(gl_eda_hdl);

	TRACE_LEAVE2("SubscriptionId: %u", i_subscriptionId);
	return (SA_AIS_OK);
}

/***************************************************************************
 * 3.7.10 
 *
 * saEvtEventUnsubscribe()
 *
 * The saEvtEventUnsubscribe() function allows a process to stop receiving
 * events for a particular subscription on an event channel by removing the
 * subscription. The saEvtEventUnsubscribe() operation is successful if the
 * subscriptionId parameter matches a previously registered subscription.
 * Pending events that no longer match any subscription, because the
 * saEvtEventUnsubscribe() operation had been invoked, are purged. A process
 * that wishes to modify a subscription without losing any events must
 * establish the new subscription before removing the existing subscription.
 *
 *
 * Parameters
 *
 * channelHandle - [in]
 *   A pointer to the event channel for which the subscriber is requesting
 *   the Event Service to delete the subscription.
 *
 * subscriptionId - [in]
 *   The identifier of the subscription that the subscriber is requesting
 *   the Event Service to delete.
 *
 ***************************************************************************/
SaAisErrorT saEvtEventUnsubscribe(SaEvtChannelHandleT channelHandle, SaEvtSubscriptionIdT i_subscriptionId)
{
	SaAisErrorT rc = SA_AIS_OK;
	EDA_CB *eda_cb = NULL;
	EDA_CHANNEL_HDL_REC *channel_hdl_rec = NULL;
	EDA_CLIENT_HDL_REC *eda_hdl_rec = NULL;
	EDSV_MSG msg;
	EDA_SUBSC_REC *sub_rec;
	TRACE_ENTER2("channel handle: %llx" ", subscriptionId: %u", channelHandle, i_subscriptionId);

  /** retrieve EDA_CB 
   **/
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

   /** retrieve channel_hdl_rec
    **/
	if (NULL == (channel_hdl_rec = (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, channelHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve channel handle: %llx", channelHandle);
		TRACE_LEAVE();
		return rc;
	}

   /** retrieve EDA client hdl record 
    **/
	if (NULL ==
	    (eda_hdl_rec =
	     (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, channel_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Failed to retrieve client handle associated with this channelHandle: %u",
										channel_hdl_rec->parent_hdl->local_hdl);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&eda_hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;	/* For now, lets return this to a pre B03 client as well */
			ncshm_give_hdl(gl_eda_hdl);
			ncshm_give_hdl(channelHandle);
			ncshm_give_hdl(eda_hdl_rec->local_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}
	/* Check Whether EDS is up or not */
	if (!eda_cb->eds_intf.eds_up) {
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("event server is not yet up");
		TRACE_LEAVE();
		return rc;
	}

   /** make sure the subscription id exists **/
	if (NULL == (sub_rec = eda_find_subsc_by_subsc_id(channel_hdl_rec, i_subscriptionId))) {
		rc = SA_AIS_ERR_NOT_EXIST;
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Invalid subscription id. record not found.");
		TRACE_LEAVE();
		return SA_AIS_ERR_NOT_EXIST;
	} else	/* delete this record */
		eda_del_subsc_rec(&channel_hdl_rec->subsc_list, sub_rec);

  /** populate the mds message to send across to the EDS
   ** where the subecription record will get installed.
   **/
	m_EDA_EDSV_UNSUBSCRIBE_MSG_FILL(msg,
					eda_hdl_rec->eds_reg_id,
					channel_hdl_rec->eds_chan_id,
					channel_hdl_rec->eds_chan_open_id, i_subscriptionId);

  /** Send the message out to the EDS to un-install this
   ** subscription.
   **/
	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_async_send(eda_cb, &msg, MDS_SEND_PRIORITY_MEDIUM))) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("mds send failed");
		TRACE_LEAVE();
		return rc;
	}

	/* Release the channel handle */
	ncshm_give_hdl(channelHandle);
	ncshm_give_hdl(eda_hdl_rec->local_hdl);
	ncshm_give_hdl(gl_eda_hdl);

	TRACE_LEAVE();
	return SA_AIS_OK;
}

/***************************************************************************
 * 3.7.11
 *
 * saEvtEventRetentionTimeClear()
 *
 * The saEvtEventRetentionTimeClear() function is used to clear the retention
 * time of a published event. It indicates to the Event Service that it does
 * not need to keep the event any longer for potential new subscribers.
 * Once the value of the retention time is reset to 0, the event is no longer
 * available for new subscribers. The event is held until all old subscribers
 * in the system process the event and free the event using saEvtEventFree().
 *
 *
 * Parameters
 *
 * channelHandle - [in]
 *   A pointer to the handle of the event channel on which the event has been
 *   published. This event channel handle was returned from a previous
 *   invocation to the saEvtChannelOpen() function.
 *
 * eventId - [in]
 *
 ***************************************************************************/
SaAisErrorT saEvtEventRetentionTimeClear(SaEvtChannelHandleT channelHandle, const SaEvtEventIdT eventId)
{
	SaAisErrorT rc = SA_AIS_OK;
	EDA_CB *eda_cb = NULL;
	EDA_CHANNEL_HDL_REC *channel_hdl_rec = NULL;
	EDA_CLIENT_HDL_REC *eda_hdl_rec = NULL;
	EDSV_MSG msg;
	EDSV_MSG *o_msg = NULL;
	TRACE_ENTER2("channel handle: %llx" ", eventId: %llx", channelHandle, eventId);

   /** Make sure that the eventId 
    ** passed in is valid.
    **/
	if (eventId <= MAX_RESERVED_EVENTID) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("eventId should be > 1000. EventId 0 to 1000 is reserved. ");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
  /** retrieve EDA_CB 
   **/
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

   /** retrieve channel_hdl_rec
    **/
	if (NULL == (channel_hdl_rec = (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, channelHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve channel handle: %llx", channelHandle);
		TRACE_LEAVE();
		return rc;
	}

   /** retrieve eda client hdl rec
    **/
	if (NULL == (eda_hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA,
									channel_hdl_rec->parent_hdl->local_hdl))) {
		rc = SA_AIS_ERR_LIBRARY;
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_4("Failed to retrieve client handle associated with this channelHandle: %u",
										channel_hdl_rec->parent_hdl->local_hdl);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&eda_hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;	/* For now, lets return this to a pre-B03 client as well */
			ncshm_give_hdl(channelHandle);
			ncshm_give_hdl(eda_hdl_rec->local_hdl);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	}
   /** Make sue this event belongs to the channel 
    ** that has been supplied.
    **/

	if (eventId > channel_hdl_rec->last_pub_evt_id) {
		rc = SA_AIS_ERR_NOT_EXIST;
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("The eventId was not published on this channnel");
		TRACE_LEAVE();
		return rc;
	}
	/* Check Whether EDS is up or not */
	if (!eda_cb->eds_intf.eds_up) {
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("event server is not yet up");
		TRACE_LEAVE();
		return rc;
	}

   /** populate a message to be sent to the EDS
    ** that will do the needful
    **/
	m_EDA_EDSV_CHAN_RETENTION_TIME_CLR_MSG_FILL(msg,
						    eda_hdl_rec->eds_reg_id,
						    channel_hdl_rec->eds_chan_id,
						    channel_hdl_rec->eds_chan_open_id, (uint32_t)eventId);

  /** Send the message out to the EDS to 
   ** clear the retention timer.
   **/

   /** Send the message out to the EDS
    **/
	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_sync_send(eda_cb, &msg, &o_msg, EDSV_WAIT_TIME))) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		ncshm_give_hdl(channelHandle);
		ncshm_give_hdl(eda_hdl_rec->local_hdl);
		ncshm_give_hdl(gl_eda_hdl);

		/* free up the response message */
		if (o_msg)
			eda_msg_destroy(o_msg);
		TRACE_4("mds send failed");
		TRACE_LEAVE();
		return rc;
	}

    /** Make sure the EDS return status was SA_AIS_OK 
    **/
	if (o_msg) {
		if (SA_AIS_OK != o_msg->info.api_resp_info.rc) {
			rc = o_msg->info.api_resp_info.rc;
			TRACE_1("event server returned error: %u", rc);
		}
	}

	/* Release all the handles that were taken */
	ncshm_give_hdl(channelHandle);
	ncshm_give_hdl(eda_hdl_rec->local_hdl);
	ncshm_give_hdl(gl_eda_hdl);

	if (o_msg)
		eda_msg_destroy(o_msg);

	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * 3.8.1
 *
 *  saEvtLimitGet()
 *
 * This function obtains the current implementation specific limit value of
 * the Event Service for the following:
 *              - Max Number of Channels allowed by this implementation 
 *              - Max size of an Event
 *              - Max size of an Event Pattern
 *              - Max number of patterns in an event
 *              - Max retention time duration for an event
 * Parameters
 *
 * evtHandle - [in] A handle obtained during a previous invocation of
 *                  saEvtInitialized().
 * limitValue - [out] A pointer to the saLimitValueT structure. The limit
 *                  values are filled based on the limitId specified by the 
 *                  user. 
 *
 ***************************************************************************/

SaAisErrorT saEvtLimitGet(SaEvtHandleT evtHandle, SaEvtLimitIdT limitId, SaLimitValueT *limitValue)
{
	EDA_CB *eda_cb = 0;
	EDA_CLIENT_HDL_REC *hdl_rec = 0;
	EDSV_MSG i_msg, *o_msg;
	SaAisErrorT rc = SA_AIS_OK;
	EDSV_EDA_LIMIT_GET_RSP *limit_get_rsp = NULL;
	TRACE_ENTER2("Event handle: %llx", evtHandle);

	if (limitValue == NULL) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("out param limitValue is NULL");
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, gl_eda_hdl))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("Unable to retrieve global handle: %u", gl_eda_hdl);
		TRACE_LEAVE();
		return rc;
	}

	/* retrieve hdl rec */
	if (NULL == (hdl_rec = (EDA_CLIENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, evtHandle))) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("Unable to retrieve client event handle: %llx", evtHandle);
		TRACE_LEAVE();
		return rc;
	}

	if (m_IS_B03_CLIENT((&hdl_rec->version))) {
		if (eda_cb->node_status != SA_CLM_NODE_JOINED) {
			rc = SA_AIS_ERR_UNAVAILABLE;
			ncshm_give_hdl(evtHandle);
			ncshm_give_hdl(gl_eda_hdl);
			TRACE_2("This node is not a member of the CLM cluster");
			TRACE_LEAVE();
			return rc;
		}
	} else {
		rc = SA_AIS_ERR_VERSION;
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		TRACE_2("This API is supported only in the B.03.01 version");
		TRACE_LEAVE();
		return rc;
	}

	/* Populate the message to be sent to the EDS */

	m_EDA_EDSV_LIMIT_GET_MSG_FILL(i_msg);

	/* Send a message to the EDS (ACTIVE) to obtain our limits.
	 * After an Upgrade, the old and new EDS may have different
	 * limits, but that's okay. That's what we need.
	 */
	if (NCSCC_RC_SUCCESS != (rc = eda_mds_msg_sync_send(eda_cb, &i_msg, &o_msg, EDSV_WAIT_TIME))) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		ncs_eda_shutdown();
		ncs_agents_shutdown();
		TRACE_4("mds send failed");
		TRACE_LEAVE();
		return rc;
	}

	/* Make sure the EDS return status was SA_AIS_OK */
	if (SA_AIS_OK != o_msg->info.api_resp_info.rc) {
		rc = o_msg->info.api_resp_info.rc;
		ncshm_give_hdl(evtHandle);
		ncshm_give_hdl(gl_eda_hdl);
		/* free up the response message */
		if (o_msg)
			eda_msg_destroy(o_msg);
		TRACE_4("server returned error: %u", rc);
		TRACE_LEAVE();
		return rc;
	}

	limit_get_rsp = &(o_msg->info.api_resp_info.param.limit_get_rsp);
	if (limit_get_rsp) {

		if (limitId == SA_EVT_MAX_NUM_CHANNELS_ID)
			limitValue->uint64Value = limit_get_rsp->max_chan;
		else if (limitId == SA_EVT_MAX_EVT_SIZE_ID)
			limitValue->uint64Value = limit_get_rsp->max_evt_size;
		else if (limitId == SA_EVT_MAX_PATTERN_SIZE_ID)
			limitValue->uint64Value = limit_get_rsp->max_ptrn_size;
		else if (limitId == SA_EVT_MAX_NUM_PATTERNS_ID)
			limitValue->uint64Value = limit_get_rsp->max_num_ptrns;
		else if (limitId == SA_EVT_MAX_RETENTION_DURATION_ID)
			limitValue->timeValue = limit_get_rsp->max_ret_time;
		else
			rc = SA_AIS_ERR_INVALID_PARAM;

	} else
		rc = SA_AIS_ERR_TRY_AGAIN;

	ncshm_give_hdl(evtHandle);
	ncshm_give_hdl(gl_eda_hdl);

	TRACE_LEAVE();
	return rc;
}
