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
..............................................................................

..............................................................................

  DESCRIPTION:
  
  This file contains the MQSv SAF API definitions.
    
******************************************************************************
*/

#include "mqa.h"

#include <sched.h>

/* All MQA utility functions prototypes. */

static SaAisErrorT mqa_queue_name_to_destination(const SaNameT *queueName, SaMsgQueueHandleT *queueHandle,
						 MDS_DEST *destination, MDS_DEST *mqa);

static SaAisErrorT mqa_send_to_destination(MQA_CB *mqa_cb, MDS_DEST *mqnd_mds_dest,
					   MQSV_DSEND_EVT *qsend_evt, SaMsgAckFlagsT ackFlags,
					   SaTimeT timeout, uint32_t length);
static SaAisErrorT mqa_send_to_destination_async(MQA_CB *mqa_cb, MDS_DEST *mqnd_mds_dest,
						 MQSV_DSEND_EVT *qsend_evt, uint32_t length);
static uint32_t mqa_send_to_group(MQA_CB *mqa_cb, ASAPi_OPR_INFO *asapi_or, MQSV_DSEND_EVT *qsend_evt,
			       SaMsgAckFlagsT ackFlags, MQA_SEND_MESSAGE_PARAM *param, uint32_t length);
static SaAisErrorT mqa_send_message(SaMsgHandleT msgHandle,
				    const SaNameT *destination,
				    const SaMsgMessageT *message,
				    SaMsgAckFlagsT ackFlags, MQA_SEND_MESSAGE_PARAM *param, MQA_CB *mqa_cb);
static SaAisErrorT mqa_receive_message(SaMsgQueueHandleT queueHandle,
				       SaMsgMessageT *message,
				       SaTimeT *sendTime, SaMsgSenderIdT *senderId, SaTimeT timeout);
static SaAisErrorT mqa_send_receive(MQA_CB *mqa_cb, MDS_DEST *mqnd_mds_dest,
				    MQSV_DSEND_EVT *qsend_evt, MQSV_DSEND_EVT **qreply_evt,
				    SaTimeT timeout, uint32_t length);
static bool mqa_match_senderid(void *key, void *qelem);
static SaAisErrorT mqa_reply_to_destination(MQA_CB *mqa_cb, MDS_DEST *mqnd_mds_dest,
					    MQSV_DSEND_EVT *qsend_evt, SaMsgAckFlagsT ackFlags,
					    SaTimeT timeout, MDS_SYNC_SND_CTXT *context, uint32_t length);
static SaAisErrorT mqa_reply_to_destination_async(MQA_CB *mqa_cb, MDS_DEST *mqnd_mds_dest,
						  MQSV_DSEND_EVT *qsend_evt, MDS_SYNC_SND_CTXT *context, uint32_t length);
static SaAisErrorT mqa_reply_message(SaMsgHandleT msgHandle,
				     const SaMsgMessageT *message,
				     SaMsgSenderIdT *senderId,
				     SaMsgAckFlagsT ackFlags, MQA_SEND_MESSAGE_PARAM *param, MQA_CB *mqa_cb);
static void msgget_timer_expired(void *arg);

extern MSG_FRMT_VER mqa_mqnd_msg_fmt_table[];

MSG_FRMT_VER mqa_mqa_msg_fmt_table[MQA_WRT_MQA_SUBPART_VER_RANGE] = { 0, 2 };	/*With version 1 it is not backward compatible */

/****************************************************************************
  Name          : mqa_queue_name_to_destination
 
  Description   : This routine queries the ASAPi to get the destination MQND for
                  give queue name.
 
  Arguments     : const SaNameT *queueName 
                  SaMsgQueueHandleT *queueHandle - Queue Handle for this queue name returned.
                   MDS_DEST *destination  - destination MQND where the queue exists returned.
                   MDS_DEST *mqd  - MQD MDS address returned.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT mqa_queue_name_to_destination(const SaNameT *queueName, SaMsgQueueHandleT *queueHandle,
					  MDS_DEST *destination, MDS_DEST *mqd)
{
	ASAPi_OPR_INFO asapi_or;
	SaAisErrorT rc;
        TRACE_ENTER();

	memset(&asapi_or, 0, sizeof(asapi_or));
	asapi_or.type = ASAPi_OPR_GET_QUEUE;
	asapi_or.info.queue.i_name = *queueName;
	asapi_or.info.queue.i_sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	asapi_or.info.queue.i_sinfo.dest = *mqd;
	asapi_or.info.queue.i_sinfo.stype = MDS_SENDTYPE_SNDRSP;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != SA_AIS_OK) {
                TRACE_2("ERR_ASAPi_OPERATION: ASAPi Operation Failed:%d", rc);
		return rc;
	}

	*destination = asapi_or.info.queue.o_parm.addr;
	if (queueHandle)
		*queueHandle = asapi_or.info.queue.o_parm.hdl;

        TRACE_LEAVE();
	return SA_AIS_OK;
}

/****************************************************************************
  Name          : saMsgInitialize
 
  Description   : This routine initializes the message service library.
 
  Arguments     : SaMsgHandleT *msgHandle - The message handle for this library.
                  const SaMsgCallbacksT *msgCallbacks - The callbacks supplied for
                                                         handling async messages.
                  const SaVersionT *version - message service version of MQA.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgInitialize(SaMsgHandleT *msgHandle, const SaMsgCallbacksT *msgCallbacks, SaVersionT *version)
{
	MQA_CB *mqa_cb = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	MQSV_EVT initialize_evt;
	MQSV_EVT *out_evt = NULL;
	MQA_CLIENT_INFO *client_info = NULL;
	uint32_t mds_rc;

	TRACE_ENTER();

	/* Initialize the environment */
	if ((rc = ncs_agents_startup()) != NCSCC_RC_SUCCESS) {
                TRACE_4("ERR_LIBRARY: NCS Agents Startup Failed:%d", rc); 
		return SA_AIS_ERR_LIBRARY;
	}

	if ((rc = ncs_mqa_startup()) != NCSCC_RC_SUCCESS) {
                TRACE_4("ERR_LIBRARY: NCS Agents Startup Failed:%d", rc); 
		ncs_agents_shutdown();
		return SA_AIS_ERR_LIBRARY;
	}

	if ((!msgHandle) || (!version)) {
                TRACE_2("ERR_INVALID_PARAM: msgHandle is NULL or version is NULL"); 
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto final1;
	}

	*msgHandle = 0;

	/* Validate the version */
	if (!((version->releaseCode == MQA_RELEASE_CODE)
	      && (((version->majorVersion == MQA_MAJOR_VERSION))
		  || ((version->majorVersion == MQA_BASE_MAJOR_VERSION)
		      && (version->minorVersion == MQA_BASE_MINOR_VERSION))))) {
                TRACE_2("ERR_VERSION: version only supported for B.01.01 and B.03.01"); 

		/* Implementation is supporting the required release code */
		version->releaseCode = MQA_RELEASE_CODE;
		version->majorVersion = MQA_MAJOR_VERSION;
		version->minorVersion = MQA_MINOR_VERSION;

		rc = SA_AIS_ERR_VERSION;
		goto final1;
	}

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
                TRACE_4("ERR_LIBRARY: Control block retrieval failed"); 
		rc = SA_AIS_ERR_LIBRARY;
		goto final1;
	}

	if ((rc = m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE)) != NCSCC_RC_SUCCESS) {
                TRACE_4("ERR_LIBRARY: Lock failed for control block write"); 
		rc = SA_AIS_ERR_LIBRARY;
		goto final2;
	}

	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
                TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down"); 
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto final3;
	}
	/* populate the structure */
	memset(&initialize_evt, 0, sizeof(MQSV_EVT));
	initialize_evt.type = MQSV_EVT_MQP_REQ;
	initialize_evt.msg.mqp_req.type = MQP_EVT_INIT_REQ;
	initialize_evt.msg.mqp_req.info.initReq.version = *version;
	initialize_evt.msg.mqp_req.agent_mds_dest = mqa_cb->mqa_mds_dest;

	/* send the request to the MQND */
	mds_rc = mqa_mds_msg_sync_send(mqa_cb->mqa_mds_hdl, &mqa_cb->mqnd_mds_dest,
				       &initialize_evt, &out_evt, MQSV_WAIT_TIME);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
                TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto final3;
	case NCSCC_RC_FAILURE:
                TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest); 
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto final3;
	default:
                TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest); 
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto final3;
	}

	if (out_evt)
		rc = out_evt->msg.mqp_rsp.error;
	else {
                TRACE_4("ERR_RESOURCES: Response not received from MQND");
		rc = SA_AIS_ERR_NO_RESOURCES;
        }

	if (rc == SA_AIS_OK) {

		/* create the client node and populate it */
		client_info = mqa_client_tree_find_and_add(mqa_cb, 0, true);

		if (client_info == NULL) {
                        TRACE_4("ERR_MEMORY: Client database Registration Failed");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto final3;
		}

		/* Update version passed in initialize */
		client_info->version.releaseCode = version->releaseCode;
		client_info->version.majorVersion = version->majorVersion;
		client_info->version.minorVersion = version->minorVersion;

		if (mqsv_mqa_callback_queue_init(client_info) != NCSCC_RC_SUCCESS) {
                        TRACE_4("ERR_RESOURCES: Callback Queue Initialization Failed");
			rc = SA_AIS_ERR_NO_RESOURCES;
			mqa_client_tree_delete_node(mqa_cb, client_info);
			goto final3;
		}

		if (msgCallbacks)
			client_info->msgCallbacks = *msgCallbacks;
		else
			memset(&client_info->msgCallbacks, 0, sizeof(client_info->msgCallbacks));

		*msgHandle = client_info->msgHandle;
		client_info->finalize = 0;

		/* Implementation is supporting the required release code */
		version->releaseCode = MQA_RELEASE_CODE;
		version->majorVersion = MQA_MAJOR_VERSION;
		version->minorVersion = MQA_MINOR_VERSION;
	}

 final3:
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

 final2:
	/* clear up the out evt */
	if (out_evt)
		m_MMGR_FREE_MQA_EVT(out_evt);

	m_MQSV_MQA_GIVEUP_MQA_CB;

 final1:

	if (rc == SA_AIS_OK) {
		TRACE_LEAVE2(" Success handle - %llu ", *msgHandle);
	} else {
		ncs_mqa_shutdown();
		ncs_agents_shutdown();
		TRACE_LEAVE2(" Failed with return code - %d ", rc);
	}

	return rc;
}

/****************************************************************************
  Name          : saMsgSelectionObjectGet
 
  Description   : This routine returns the selection object to the caller. This is
                  used by application to get notfied of async messages.
 
  Arguments     : SaMsgHandleT *msgHandle - The message handle for this library.
                  SaSelectionObjectT *selectionObject - selection object returned.

  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgSelectionObjectGet(SaMsgHandleT msgHandle, SaSelectionObjectT *selectionObject)
{
	MQA_CB *mqa_cb = 0;
	MQA_CLIENT_INFO *client_info;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2(" SaMsgHandle %llu ", msgHandle);

	if (!selectionObject) {
                TRACE_2("ERR_INVALID_PARAM: selectionObject is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}
	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
                TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done1;
	}
	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
                TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto done0;
	}
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
                TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
                TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
                        TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto done;
		}
	}

	*selectionObject = (SaSelectionObjectT)m_GET_FD_FROM_SEL_OBJ(m_NCS_IPC_GET_SEL_OBJ(&client_info->callbk_mbx));
        TRACE_1("New selectionObject is %llu", *selectionObject);

 done:
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

 done0:

	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

 done1:

	if (rc != SA_AIS_OK || *selectionObject <= 0) 
                TRACE_2("MsgQ Svc selection object retrieval failed");

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;
}

/****************************************************************************
  Name          : saMsgDispatch
 
  Description   : This routine dispatches the messages in the queue to the callbacks
 
  Arguments     : SaMsgHandleT *msgHandle[OUT] - 
                  const SaMsgCallbacksT *msgCallbacks
                  const SaVersionT *version
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgDispatch(SaMsgHandleT msgHandle, SaDispatchFlagsT dispatchFlags)
{
	MQA_CB *mqa_cb = 0;
	MQA_CLIENT_INFO *client_info = 0;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2(" SaMsgHandle %llu SaDispatchFlag %d", msgHandle,dispatchFlags);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
                TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
                TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}

	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
                TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);

	if (!client_info) {
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
                        TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			goto done;
		}
	}
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	switch (dispatchFlags) {

	case SA_DISPATCH_ONE:
		rc = mqa_hdl_callbk_dispatch_one(mqa_cb, msgHandle);
		break;

	case SA_DISPATCH_ALL:
		rc = mqa_hdl_callbk_dispatch_all(mqa_cb, msgHandle);
		break;

	case SA_DISPATCH_BLOCKING:
		rc = mqa_hdl_callbk_dispatch_block(mqa_cb, msgHandle);
		break;

	default:
                TRACE_2("ERR_INVALID_PARAM: dispatchFlags is not matching");
		rc = SA_AIS_ERR_INVALID_PARAM;
		break;
	}			/* switch */

 done:
	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);

	return rc;
}

/****************************************************************************
  Name          : saMsgFinalize
 
  Description   : This routine dinalizes the message service denoted by msgHandle..
 
  Arguments     : SaMsgHandleT msgHandle[IN] - message handle to be finalized
                  
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgFinalize(SaMsgHandleT msgHandle)
{
	MQA_CB *mqa_cb = NULL;
	SaAisErrorT rc = SA_AIS_ERR_LIBRARY, rc2 = SA_AIS_OK;
	MQSV_EVT finalize_evt;
	MQSV_EVT *out_evt = NULL;
	MQA_CLIENT_INFO *client_info;
	MQA_QUEUE_INFO *queue_info;
	uint32_t mds_rc, flag = 0;
	SaMsgQueueHandleT *temp_ptr = NULL;
	SaMsgQueueHandleT temp_hdl;

	TRACE_ENTER2(" Handle %llu ", msgHandle);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	client_info->finalize = 1;

	/* scan the entire handle db & close queue opening by this client */
	while ((queue_info =
		(MQA_QUEUE_INFO *)ncs_patricia_tree_getnext(&mqa_cb->mqa_queue_tree, (uint8_t *const)temp_ptr))) {
		temp_hdl = queue_info->queueHandle;
		temp_ptr = &temp_hdl;

		if (queue_info->client_info == client_info) {
			/* Unlock before you call close */
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			flag++;
			rc2 = saMsgQueueClose(queue_info->queueHandle);
			if (rc2 != SA_AIS_OK) {
				TRACE_2("Close as part of Finalize failed with return value=%d", rc2);
				rc = rc2;
				if (flag == 1)	/* If first queue close fails */
					goto close_fail;
			}

			/* get the Lock again */
			if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
				TRACE_4("ERR_LIBRARY: Lock failed for control block write");
				rc = SA_AIS_ERR_LIBRARY;
				goto lock_fail;
			}
		}

	}

	/* populate the structure */
	memset(&finalize_evt, 0, sizeof(MQSV_EVT));

	finalize_evt.type = MQSV_EVT_MQP_REQ;
	finalize_evt.msg.mqp_req.type = MQP_EVT_FINALIZE_REQ;
	finalize_evt.msg.mqp_req.agent_mds_dest = mqa_cb->mqa_mds_dest;
	finalize_evt.msg.mqp_req.info.finalReq.msgHandle = msgHandle;

	/* send the request to the MQND */
	mds_rc = mqa_mds_msg_sync_send(mqa_cb->mqa_mds_hdl, &(mqa_cb->mqnd_mds_dest),
				       &finalize_evt, &out_evt, MQSV_WAIT_TIME);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
                TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	case NCSCC_RC_FAILURE:
                TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	default:
		TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done;
	}

	if (out_evt)
		rc = out_evt->msg.mqp_rsp.error;
	else {
                TRACE_4("ERR_RESOURCES: response not received from MQND");
		rc = SA_AIS_ERR_NO_RESOURCES;
        }

	/* cleanup all the client info */
	if (rc == SA_AIS_OK) {
		mqsv_mqa_callback_queue_destroy(client_info);
		mqa_client_tree_delete_node(mqa_cb, client_info);
	}

 done:

	if (out_evt)
		m_MMGR_FREE_MQA_EVT(out_evt);

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

 close_fail:
 lock_fail:
	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK) {
		TRACE_LEAVE2(" Success ");
	} else {
		ncs_mqa_shutdown();
		ncs_agents_shutdown();
		TRACE_LEAVE2(" Failed with return code %d", rc);
	}
	return rc;
}

/****************************************************************************
  Name          : saMsgQueueOpen
 
  Description   : This API opens/creates a queue denoted by queueName
                  
 
  Arguments     : SaMsgHandleT msgHandle,- The message handle of this library.
                  SaInvocationT invocation,
                  const SaNameT *queueName,- Queue to be opened.
                  const SaMsgQueueCreationAttributesT *creationAttributes, 
                                 - Creation attributes of the queue.
                  SaMsgQueueOpenFlagsT openFlags - Open an existing/create new.
                  SaTimeT timeout, - time to wait for response.
                  SaMsgQueueHandleT *queueHandle - Opened Queue Handle.

  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT
saMsgQueueOpen(SaMsgHandleT msgHandle,
	       const SaNameT *queueName,
	       const SaMsgQueueCreationAttributesT *creationAttributes,
	       SaMsgQueueOpenFlagsT openFlags, SaTimeT timeout, SaMsgQueueHandleT *queueHandle)
{

	MQA_CB *mqa_cb = NULL;
	MQA_CLIENT_INFO *client_info;
	MQA_QUEUE_INFO *queue_info;
	MQSV_EVT qopen_evt;
	MQSV_EVT *out_evt = NULL;
	SaAisErrorT rc;
	uint32_t mds_rc;
	NCSCONTEXT thread_handle;
	SaTimeT timeout1;

	TRACE_ENTER2(" SaMsgHandle %llu ", msgHandle);

	if ((!queueName) || (!queueHandle)) {
		TRACE_2("ERR_INVALID_PARAM: queueName is NULL or queueHandle is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (queueName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueName exceeds character 256");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	m_MQSV_SET_SANAMET(queueName);
	TRACE_1("queueName %s",queueName->value);

	if (strncmp((char *)queueName->value, "safMq=", 6)) {
                TRACE_2("ERR_INVALID_PARAM: queueName should starts with safMq=");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (m_NCS_SA_IS_VALID_TIME_DURATION(timeout) == false) {
		TRACE_2("ERR_INVALID_PARAM: SaTimeT value exceeds the limit"); 
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (openFlags != 0) {
		if (!(openFlags & (SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK | SA_MSG_QUEUE_EMPTY))) {
			TRACE_2("ERR_BAD_FLAGS: SaMsgQueueOpenFlagsT should be one of the specified flag"); 
			rc = SA_AIS_ERR_BAD_FLAGS;
			return rc;
		}
	}

	/* check for valid creationFlags */
	if (openFlags & SA_MSG_QUEUE_CREATE) {
		if (creationAttributes == NULL) {
			TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
			rc = SA_AIS_ERR_INVALID_PARAM;
			return rc;
		}
	} else {
		if (creationAttributes != NULL) {
			TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
			rc = SA_AIS_ERR_INVALID_PARAM;
			return rc;
		}
	}

	/* Check for bad creation flags and bad open flags */
	if (creationAttributes) {

		if ((creationAttributes->creationFlags != 0)
		    && (creationAttributes->creationFlags != SA_MSG_QUEUE_PERSISTENT)) {
			TRACE_2("ERR_BAD_FLAGS: Invalid Parameter as input"); 
			rc = SA_AIS_ERR_BAD_FLAGS;
			return rc;
		}

	}

	if ((timeout / SA_TIME_ONE_MILLISECOND) < NCS_SAF_MIN_ACCEPT_TIME) {
		TRACE_2("ERR_TIMEOUT: Invalid Parameter as input");
		rc = SA_AIS_ERR_TIMEOUT;
		return rc;
	}

	/* convert the timeout to 1i0 ms value and add it to the sync send timeout */
	timeout1 = timeout;
	timeout = m_MQSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}

	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto done;
		}
	}

	if ((openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK) && (!client_info->msgCallbacks.saMsgMessageReceivedCallback)) {
		TRACE_2("ERR_INIT: Receiver Call back is not given as input");
		rc = SA_AIS_ERR_INIT;
		goto done;
	}

	/* populate the evt */
	memset(&qopen_evt, 0, sizeof(MQSV_EVT));
	qopen_evt.type = MQSV_EVT_MQP_REQ;
	qopen_evt.msg.mqp_req.type = MQP_EVT_OPEN_REQ;
	if (!creationAttributes)
		memset(&qopen_evt.msg.mqp_req.info.openReq.creationAttributes, 0,
		       (sizeof(SaMsgQueueCreationAttributesT)));
	else
		qopen_evt.msg.mqp_req.info.openReq.creationAttributes = *creationAttributes;
	qopen_evt.msg.mqp_req.info.openReq.msgHandle = msgHandle;
	qopen_evt.msg.mqp_req.info.openReq.openFlags = openFlags;
	qopen_evt.msg.mqp_req.info.openReq.queueName = *queueName;
	qopen_evt.msg.mqp_req.info.openReq.timeout = timeout1;
	qopen_evt.msg.mqp_req.agent_mds_dest = mqa_cb->mqa_mds_dest;

	/* send the event */
	mds_rc = mqa_mds_msg_sync_send(mqa_cb->mqa_mds_hdl, &(mqa_cb->mqnd_mds_dest),
				       &qopen_evt, &out_evt, (uint32_t)timeout);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	case NCSCC_RC_FAILURE:
		TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	default:
		TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done;
	}

	/* got the reply... do the needful */
	if (out_evt)
		rc = out_evt->msg.mqp_rsp.error;
	else {
		TRACE_4("ERR_RESOURCES: Response not received from MQND");
		rc = SA_AIS_ERR_NO_RESOURCES;
	}

	if (rc == SA_AIS_OK) {
		*queueHandle = out_evt->msg.mqp_rsp.info.openRsp.queueHandle;

		if ((queue_info =
		     mqa_queue_tree_find_and_add(mqa_cb, *queueHandle, true, client_info, openFlags)) == NULL) {
			TRACE_2("ERR_BAD_HANDLE: Client database Registration Failed");
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}

		/* Start a thread to notify when there is a message in the queue.
		 * The thread does it by using  1 byte message buffer to read
		 * from the queue. When it fails, it assumes that there is a message
		 * in the queue.
		 */
		if (queue_info->openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK) {
			MQP_OPEN_RSP *openRsp;

			/* update queue_info data structure with listenerHandle */
			queue_info->listenerHandle = out_evt->msg.mqp_rsp.info.openRsp.listenerHandle;

			openRsp = m_MMGR_ALLOC_MQA_OPEN_RSP(sizeof(MQP_OPEN_RSP));

			if (!openRsp) {
				mqa_queue_tree_delete_node(mqa_cb, *queueHandle);
				TRACE_4("ERR_MEMORY: MQP Open Rsp Message Allocatiion Failed");
				rc = SA_AIS_ERR_NO_MEMORY;
				goto done;
			}

			*openRsp = out_evt->msg.mqp_rsp.info.openRsp;

			int policy = SCHED_OTHER; /*root defaults */
			int prio_val = sched_get_priority_min(policy);

			rc = m_NCS_TASK_CREATE((NCS_OS_CB)mqa_queue_reader,
					       (NCSCONTEXT)openRsp,
					       (char *)"OSAF_MQA", prio_val, policy, NCS_STACKSIZE_HUGE, &thread_handle);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE_4("ERR_RESOURCES: Queue Reader Thread Task Create Failed");
				rc = SA_AIS_ERR_NO_RESOURCES;
				mqa_queue_tree_delete_node(mqa_cb, *queueHandle);
				goto done;
			}

			rc = m_NCS_TASK_START(thread_handle);
			if (rc != NCSCC_RC_SUCCESS) {
				m_NCS_TASK_DETACH(thread_handle);
				TRACE_4("ERR_RESOURCES: Queue Reader Thread Task Start Failed");
				rc = SA_AIS_ERR_NO_RESOURCES;
				mqa_queue_tree_delete_node(mqa_cb, *queueHandle);
				goto done;
			}

			/* Detach this thread and allow it to have its own life. This thread is going to exit on its own 
			   This macro is going to release the refecences to this thread in the LEAP */
			m_NCS_TASK_DETACH(thread_handle);

			queue_info->task_handle = thread_handle;
		} else
			rc = SA_AIS_OK;

	}

 done:
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	if (out_evt)
		m_MMGR_FREE_MQA_EVT(out_evt);

	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success Res_id %llu ", *queueHandle);
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);

	return rc;
}

/****************************************************************************
  Name          : saMsgQueueOpenAsync
 
  Description   : This API opens/creates a queue denoted by queueName
                  
 
  Arguments     : SaMsgHandleT msgHandle,- The message handle of this library.
                    SaInvocationT invocation,
                    const SaNameT *queueName,- Queue to be opened.
                    const SaMsgQueueCreationAttributesT *creationAttributes, 
                                 - Creation attributes of the queue.
                    SaMsgQueueOpenFlagsT openFlags - Open an existing/create new.

  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT
saMsgQueueOpenAsync(SaMsgHandleT msgHandle,
		    SaInvocationT invocation,
		    const SaNameT *queueName,
		    const SaMsgQueueCreationAttributesT *creationAttributes, SaMsgQueueOpenFlagsT openFlags)
{
	MQA_CB *mqa_cb = NULL;
	MQA_CLIENT_INFO *client_info;
	MQSV_EVT qopen_evt;
	SaAisErrorT rc = SA_AIS_OK;
	MQP_ASYNC_RSP_MSG mqp_async_rsp;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	if (queueName == NULL) {
		TRACE_2("ERR_INVALID_PARAM: queueName is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (queueName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueName exceeds character 256");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	m_MQSV_SET_SANAMET(queueName);
	TRACE_1("queueName %s ", queueName->value);

	if (strncmp((char *)queueName->value, "safMq=", 6)) {
		TRACE_2("ERR_INVALID_PARAM: queueName should starts with safMq=");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (openFlags != 0) {
		if (!(openFlags & (SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK | SA_MSG_QUEUE_EMPTY))) {
			TRACE_2("ERR_BAD_FLAGS: SaMsgQueueOpenFlagsT should be one of the specified flag");
			rc = SA_AIS_ERR_BAD_FLAGS;
			return rc;
		}
	}

	/* check for valid creationFlags */
	if (openFlags & SA_MSG_QUEUE_CREATE) {
		if (creationAttributes == NULL) {
			TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
			rc = SA_AIS_ERR_INVALID_PARAM;
			return rc;
		}
	} else {
		if (creationAttributes != NULL) {
			TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
			rc = SA_AIS_ERR_INVALID_PARAM;
			return rc;
		}
	}

	/* Check for bad creation flags and bad open flags */
	if (creationAttributes) {
		if ((creationAttributes->creationFlags != 0)
		    && (creationAttributes->creationFlags != SA_MSG_QUEUE_PERSISTENT)) {
			TRACE_2("ERR_BAD_FLAGS: Invalid Parameter as input");
			rc = SA_AIS_ERR_BAD_FLAGS;
			return rc;
		}
	}

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto done1;
	}

	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto done;
		}
	}

	/* check to see if the grant callback was registered */
	if (!client_info->msgCallbacks.saMsgQueueOpenCallback) {
		TRACE_2("ERR_INIT: Receiver Call back is not given as input");
		rc = SA_AIS_ERR_INIT;
		goto done;
	}

	if ((openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK) && (!client_info->msgCallbacks.saMsgMessageReceivedCallback)) {
		TRACE_2("ERR_INIT: Receiver Call back is not given as input");
		rc = SA_AIS_ERR_INIT;
		goto done;
	}

	/* populate the evt */
	memset(&qopen_evt, 0, sizeof(MQSV_EVT));

	qopen_evt.type = MQSV_EVT_MQP_REQ;
	qopen_evt.msg.mqp_req.type = MQP_EVT_OPEN_ASYNC_REQ;
	if (!creationAttributes)
		memset(&qopen_evt.msg.mqp_req.info.openAsyncReq.mqpOpenReq.creationAttributes, 0,
		       (sizeof(SaMsgQueueCreationAttributesT)));
	else
		qopen_evt.msg.mqp_req.info.openAsyncReq.mqpOpenReq.creationAttributes = *creationAttributes;
	qopen_evt.msg.mqp_req.info.openAsyncReq.mqpOpenReq.msgHandle = msgHandle;
	qopen_evt.msg.mqp_req.info.openAsyncReq.mqpOpenReq.openFlags = openFlags;
	qopen_evt.msg.mqp_req.info.openAsyncReq.mqpOpenReq.queueName = *queueName;
	qopen_evt.msg.mqp_req.info.openAsyncReq.invocation = invocation;
	qopen_evt.msg.mqp_req.agent_mds_dest = mqa_cb->mqa_mds_dest;

	mqp_async_rsp.next = NULL;
	mqp_async_rsp.callbackType = MQP_ASYNC_RSP_OPEN;
	mqp_async_rsp.messageHandle = msgHandle;
	mqp_async_rsp.params.qOpen.error = SA_AIS_ERR_TIMEOUT;
	mqp_async_rsp.params.qOpen.invocation = invocation;
	mqp_async_rsp.params.qOpen.queueHandle = 0;

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	if (mqa_create_and_start_timer(&mqp_async_rsp, invocation) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_RESOURCES: Create and Start Tmr Failed");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done1;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto done1;
	}

	/* send the event */
	if (mqa_mds_msg_async_send((mqa_cb->mqa_mds_hdl), &(mqa_cb->mqnd_mds_dest), &qopen_evt, NCSMDS_SVC_ID_MQND) !=
	    NCSCC_RC_SUCCESS) {
		mqa_stop_and_delete_timer_by_invocation(NCS_INT64_TO_PTR_CAST(invocation));
		TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
	}

 done:

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

 done1:

	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success message handle %llu ", msgHandle);
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);

	return rc;
}

/****************************************************************************
  Name          : saMsgQueueClose
 
  Description   : This API closes the queue denoted by queueHandle. 
                  
 
  Arguments     : SaMsgHandleT msgHandle - message handle of this library.
                  const SaNameT *queueName - Queue whose status needs to be obtained.
                  SaMsgQueueStatusT *queueStatus 

  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgQueueClose(SaMsgQueueHandleT queueHandle)
{
	MQA_CB *mqa_cb = NULL;
	MQSV_EVT qclose_evt;
	MQSV_EVT *out_evt = NULL;
	SaAisErrorT rc = SA_AIS_ERR_LIBRARY;
	MQA_QUEUE_INFO *queue_node = NULL;
	int64_t mqa_timeout;
	uint32_t mds_rc;

	TRACE_ENTER2(" SaMsgQueueHandle %llu ", queueHandle);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	/* take the cb lock */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	if ((queue_node = mqa_queue_tree_find_and_add(mqa_cb, queueHandle, false, NULL, 0)) == NULL) {
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_2("ERR_BAD_HANDLE: Queue Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	if (queue_node->client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if ((!mqa_cb->clm_node_joined) && (!queue_node->client_info->finalize)) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			m_MQSV_MQA_GIVEUP_MQA_CB;
			return rc;
		}
	}

	queue_node->is_closed = true;

	/* populate the evt */
	memset(&qclose_evt, 0, sizeof(MQSV_EVT));
	qclose_evt.type = MQSV_EVT_MQP_REQ;
	qclose_evt.msg.mqp_req.type = MQP_EVT_CLOSE_REQ;
	qclose_evt.msg.mqp_req.info.closeReq.queueHandle = queueHandle;
	qclose_evt.msg.mqp_req.agent_mds_dest = mqa_cb->mqa_mds_dest;

	mqa_timeout = 2 * MQSV_WAIT_TIME;

	/* send the event */
	mds_rc = mqa_mds_msg_sync_send(mqa_cb->mqa_mds_hdl, &(mqa_cb->mqnd_mds_dest), &qclose_evt,
				       &out_evt, (uint32_t)mqa_timeout);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	case NCSCC_RC_FAILURE:
		TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	default:
		TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done;
	}

	/* got the reply... do the needful */
	if (out_evt)
		rc = out_evt->msg.mqp_rsp.error;
	else {
		TRACE_4("ERR_RESOURCES: Response not received from MQND");
		rc = SA_AIS_ERR_NO_RESOURCES;
	}

	/* Delete the queue handle node from the queue tree */

	if ((rc == SA_AIS_OK) || (rc == SA_AIS_ERR_BAD_HANDLE)) {
		if (mqa_queue_tree_delete_node(mqa_cb, queueHandle) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Queue database Deregistration Failed");
			rc = SA_AIS_ERR_LIBRARY;
		}
	}

 done:

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	if (out_evt)
		m_MMGR_FREE_MQA_EVT(out_evt);

	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success Res_id %llu ", queueHandle);
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;

}

/****************************************************************************
  Name          : saMsgQueueStatusGet
 
  Description   : This API returns the status of thequeue denoted by queueName 
                  
 
  Arguments     : SaMsgHandleT msgHandle - message handle of this library.
                  const SaNameT *queueName - Queue whose status needs to be obtained.
                  SaMsgQueueStatusT *queueStatus 

  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgQueueStatusGet(SaMsgHandleT msgHandle, const SaNameT *queueName, SaMsgQueueStatusT *queueStatus)
{
	MQA_CB *mqa_cb = 0;
	MQA_CLIENT_INFO *client_info = 0;
	SaAisErrorT rc;
	MDS_DEST mqnd_mds_dest;
	MQSV_EVT qstatus_evt;
	MQSV_EVT *out_evt = 0;
	int64_t mqa_timeout;
	SaMsgQueueHandleT queueHandle;
	uint32_t mds_rc;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	if ((queueName == NULL) || (queueStatus == NULL)) {
		TRACE_2("ERR_INVALID_PARAM: queueName is NULL queueStatus is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (queueName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueName exceeds character 256");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	m_MQSV_SET_SANAMET(queueName);
	TRACE_1("queueName %s ", queueName->value);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto done1;
	}
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);

	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto done;
		}
	}

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	if ((rc = mqa_queue_name_to_destination(queueName, &queueHandle, &mqnd_mds_dest,
						&mqa_cb->mqd_mds_dest)) != SA_AIS_OK) {
                TRACE("Destination MQND for given queue name not exist");
		goto done1;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto done1;
	}

	/* Now Send a MQP GET Status Message to destination MQND */

	memset(&qstatus_evt, 0, sizeof(MQSV_EVT));
	qstatus_evt.type = MQSV_EVT_MQP_REQ;
	qstatus_evt.msg.mqp_req.type = MQP_EVT_STATUS_REQ;
	qstatus_evt.msg.mqp_req.agent_mds_dest = mqa_cb->mqa_mds_dest;
	qstatus_evt.msg.mqp_req.info.statusReq.msgHandle = msgHandle;
	qstatus_evt.msg.mqp_req.info.statusReq.queueHandle = queueHandle;

	mqa_timeout = MQSV_WAIT_TIME;

	mds_rc = mqa_mds_msg_sync_send(mqa_cb->mqa_mds_hdl, &mqnd_mds_dest, &qstatus_evt, &out_evt, (uint32_t)mqa_timeout);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	case NCSCC_RC_FAILURE:
		TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	default:
		TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done;
	}

	/* got the reply... do the needful */
	if (out_evt)
		rc = out_evt->msg.mqp_rsp.error;
	else {
		TRACE_4("ERR_RESOURCES: Response not received from MQND");
		rc = SA_AIS_ERR_NO_RESOURCES;
	}
	if (rc == SA_AIS_OK)
		*queueStatus = out_evt->msg.mqp_rsp.info.statusRsp.queueStatus;

 done:
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

 done1:
	if (out_evt)
		m_MMGR_FREE_MQA_EVT(out_evt);

	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success Queue Name - %s ", queueName->value);
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;

}

/****************************************************************************
  Name          : saMsgQueueRetentionTimeSet
nTime

  Description   : This API changes retention time of queue.

  Arguments     : SaMsgHandleT msgHandle - message handle of this library.
                  SaTimeT *retentionTime - Retention time to be set

  Return Values : SaAisErrorT

  Notes         : None
******************************************************************************/
SaAisErrorT saMsgQueueRetentionTimeSet(SaMsgQueueHandleT queueHandle, SaTimeT *retentionTime)
{
	MQA_CB *mqa_cb = NULL;
	MQSV_EVT qret_time_evt;
	MQSV_EVT *out_evt = NULL;
	SaAisErrorT rc = SA_AIS_ERR_LIBRARY;
	MQA_QUEUE_INFO *queue_node = NULL;
	int64_t mqa_timeout;
	uint32_t mds_rc;

	TRACE_ENTER2(" SaMsgQueueHandle %llu", queueHandle);

	if ((retentionTime == NULL) || (*retentionTime <= 0)) {
		TRACE_2("ERR_INVALID_PARAM: retentionTime is NULL or Negative value");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}
	TRACE_1("retentionTime %lld", *retentionTime);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	/* take the cb lock */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	if ((queue_node = mqa_queue_tree_find_and_add(mqa_cb, queueHandle, false, NULL, 0)) == NULL) {
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_2("ERR_BAD_HANDLE: Queue Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	if (queue_node->client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			m_MQSV_MQA_GIVEUP_MQA_CB;
			return rc;
		}
	}

	/* populate the evt */
	memset(&qret_time_evt, 0, sizeof(MQSV_EVT));
	qret_time_evt.type = MQSV_EVT_MQP_REQ;
	qret_time_evt.msg.mqp_req.type = MQP_EVT_Q_RET_TIME_SET_REQ;
	qret_time_evt.msg.mqp_req.info.retTimeSetReq.queueHandle = queueHandle;
	qret_time_evt.msg.mqp_req.info.retTimeSetReq.retentionTime = *retentionTime;
	qret_time_evt.msg.mqp_req.agent_mds_dest = mqa_cb->mqa_mds_dest;

	mqa_timeout = MQSV_WAIT_TIME;

	/* send the event */
	mds_rc =
	    mqa_mds_msg_sync_send(mqa_cb->mqa_mds_hdl, &(mqa_cb->mqnd_mds_dest), &qret_time_evt, &out_evt,
				  (uint32_t)mqa_timeout);

	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	case NCSCC_RC_FAILURE:
		TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	default:
		TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done;
	}

	if (out_evt)
		rc = out_evt->msg.mqp_rsp.error;
	else {
		TRACE_4("ERR_RESOURCES: Response not received from MQND");
		rc = SA_AIS_ERR_NO_RESOURCES;
	}

 done:

	if (out_evt)
		m_MMGR_FREE_MQA_EVT(out_evt);

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success Res_id %llu ", queueHandle);
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;

}

/****************************************************************************
  Name          : saMsgQueueUnlink
 
  Description   : This API removes the queue denoted by queueName from the cluster.
 
  Arguments     : SaMsgHandleT msgHandle - message handle of this library.
                  const SaNameT *queueName - Queue to be removed from the cluster.

  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgQueueUnlink(SaMsgHandleT msgHandle, const SaNameT *queueName)
{
	MQA_CB *mqa_cb;
	MQA_CLIENT_INFO *client_info;
	SaAisErrorT rc = SA_AIS_OK;
	MDS_DEST mqnd_mds_dest;
	MQSV_EVT qunlink_evt;
	MQSV_EVT *out_evt = NULL;
	SaMsgQueueHandleT queueHandle;
	uint32_t mds_rc;
	int64_t mqa_timeout;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	if (!queueName) {
		TRACE_2("ERR_INVALID_PARAM: queueName is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (queueName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueName exceeds character 256");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	m_MQSV_SET_SANAMET(queueName);
	TRACE_1("queueName %s ", queueName->value);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	/* Check if mqd, mqnd are up */
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);

	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto done;
		}
	}

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	if ((rc = mqa_queue_name_to_destination(queueName, &queueHandle, &mqnd_mds_dest,
						&mqa_cb->mqd_mds_dest)) != SA_AIS_OK) {
		TRACE("Destination MQND for given queue name not exist");
		goto done1;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	/* Now Send a MQP UNLINK Message to destination MQND */

	memset(&qunlink_evt, 0, sizeof(MQSV_EVT));
	qunlink_evt.type = MQSV_EVT_MQP_REQ;
	qunlink_evt.msg.mqp_req.type = MQP_EVT_UNLINK_REQ;
	qunlink_evt.msg.mqp_req.agent_mds_dest = mqa_cb->mqa_mds_dest;
	qunlink_evt.msg.mqp_req.info.unlinkReq.msgHandle = msgHandle;
	qunlink_evt.msg.mqp_req.info.unlinkReq.queueHandle = queueHandle;

	mqa_timeout = MQSV_WAIT_TIME;

	mds_rc = mqa_mds_msg_sync_send(mqa_cb->mqa_mds_hdl, &mqnd_mds_dest, &qunlink_evt, &out_evt, (uint32_t)mqa_timeout);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	case NCSCC_RC_FAILURE:
		TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	default:
		TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done;
	}

	if (out_evt)
		rc = out_evt->msg.mqp_rsp.error;
	else {
		TRACE_4("ERR_RESOURCES: Response not received from MQND");
		rc = SA_AIS_ERR_NO_RESOURCES;
	}

 done:
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

 done1:
	if (out_evt)
		m_MMGR_FREE_MQA_EVT(out_evt);

	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success Queue Name %s ", queueName->value);
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;

}

/****************************************************************************
  Name          : mqa_send_to_destination
 
  Description   : This routine calls the mds util function to send the MQSV_DSEND_EVT
                  to the queue denoted by destination and waits for ack.
                  
 
  Arguments     : MQA_CB *mqa_cb - MQA control block
                  MDS_DEST *mqnd_mds_dest - mds destination of mqnd. 
                  MQSV_DSEND_EVT *qsend_evt - event structure containing the message.
                  SaTimeT timeout - time wait for ack.

  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT mqa_send_to_destination(MQA_CB *mqa_cb, MDS_DEST *mqnd_mds_dest,
				    MQSV_DSEND_EVT *qsend_evt, SaMsgAckFlagsT ackFlags, SaTimeT timeout, uint32_t length)
{
	int64_t mqa_timeout;
	SaAisErrorT rc = SA_AIS_OK;
	MQSV_DSEND_EVT *out_evt = NULL;
	uint32_t mds_rc;

	TRACE_ENTER();

	/* convert the timeout to 10 ms value and add it to the sync send timeout */
	mqa_timeout = m_MQSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout);

	mds_rc = mqa_mds_msg_sync_send_direct(mqa_cb->mqa_mds_hdl, mqnd_mds_dest, qsend_evt,
					      &out_evt, (uint32_t)mqa_timeout, length);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	case NCSCC_RC_FAILURE:
		TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	default:
		TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done;
	}

	/* got the reply... do the needful. */
	if (out_evt)
		rc = out_evt->info.sendMsgRsp.error;
	else {
		TRACE_4("ERR_RESOURCES: Response not received from MQND");
		rc = SA_AIS_ERR_NO_RESOURCES;
	}

 done:
	if (out_evt)
		mds_free_direct_buff((MDS_DIRECT_BUFF)out_evt);

	TRACE_LEAVE2("return code %d", rc);
	return rc;
}

/****************************************************************************
  Name          : mqa_send_to_destination_async
 
  Description   : This routine calls the mds util function to send the MQSV_DSEND_EVT
                  to the queue denoted by destination
                  
 
  Arguments     : MQA_CB *mqa_cb - MQA control block
                  MDS_DEST *mqnd_mds_dest - mds destination of mqnd. 
                  MQSV_DSEND_EVT *qsend_evt - event structure containing the message.

  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT mqa_send_to_destination_async(MQA_CB *mqa_cb, MDS_DEST *mqnd_mds_dest,
					  MQSV_DSEND_EVT *qsend_evt, uint32_t length)
{
	TRACE_ENTER();
	if (mqa_mds_msg_async_send_direct((mqa_cb->mqa_mds_hdl), mqnd_mds_dest, qsend_evt,
					  NCSMDS_SVC_ID_MQND, MDS_SEND_PRIORITY_MEDIUM, length) != NCSCC_RC_SUCCESS) {
		TRACE_2("ERR_TRY_AGAIN: MQA - Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		return SA_AIS_ERR_TRY_AGAIN;
	}
	TRACE_LEAVE();
	return SA_AIS_OK;
}

/****************************************************************************
  Name          : mqa_send_to_group
 
  Description   : This routine sends a message to the members of a group based 
                  on the group policy
 
  Arguments     : ASAPi_OPR_INFO    *asapi_or, -ASAPi operation structure.
                  MQSV_DSEND_EVT *qsend_evt - message to be sent
                   MQA_SEND_MESSAGE_PARAM *param - contains info abt timeout
                   incase of sync call.
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/
uint32_t mqa_send_to_group(MQA_CB *mqa_cb, ASAPi_OPR_INFO *asapi_or, MQSV_DSEND_EVT *qsend_evt,
			SaMsgAckFlagsT ackFlags, MQA_SEND_MESSAGE_PARAM *param, uint32_t length)
{

	uint32_t num_queues, status, to_dest_ver, o_msg_fmt_ver = 0;
	MDS_DEST destination_mqnd;
	uint8_t unicast = 0;
	SaAisErrorT rc = SA_AIS_ERR_NO_RESOURCES;
	MQSV_DSEND_EVT *qsend_evt_copy = NULL, *qsend_evt_buffer = NULL;
	bool is_send_success = false;

	TRACE_ENTER();

	num_queues = asapi_or->info.dest.o_cache->info.ginfo.qlist.count;

	if (num_queues == 0) {
		mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt);
		TRACE_2("ERR_QUEUE_AVAILABLE: There are no queues in the group");
		rc = SA_AIS_ERR_QUEUE_NOT_AVAILABLE;
		return rc;
	}

	if ((asapi_or->info.dest.o_cache->info.ginfo.policy == SA_MSG_QUEUE_GROUP_ROUND_ROBIN) ||
	    (asapi_or->info.dest.o_cache->info.ginfo.policy == SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN))
		unicast = 1;

	if (unicast) {
		asapi_or->info.dest.o_cache->info.ginfo.pQueue = 0;
		asapi_queue_select(&(asapi_or->info.dest.o_cache->info.ginfo));
		destination_mqnd = asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.addr;

		to_dest_ver = mqa_cb->ver_mqnd[mqsv_get_phy_slot_id(destination_mqnd)];

		/* MQND HAS GONE DOWN OR NOT YET UP */
		if (to_dest_ver == 0) {
			TRACE_2("ERR_TRY_AGAIN: MQND HAS GONE DOWN");
			mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt);
			return SA_AIS_ERR_TRY_AGAIN;
		}

		o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(to_dest_ver,
						      MQA_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT,
						      MQA_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT, mqa_mqnd_msg_fmt_table);
		if (!o_msg_fmt_ver) {
			/* Drop The Message */
			TRACE_2("ERR_VERSION: Message Format version Invalid %u", o_msg_fmt_ver);
			mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt);
			rc = SA_AIS_ERR_VERSION;
			return rc;
		}
		/*Fill the message format version before sending the message */
		qsend_evt->msg_fmt_version = o_msg_fmt_ver;

		if (!param->async_flag) {
			qsend_evt->info.snd_msg.queueHandle = asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.hdl;
			qsend_evt->info.snd_msg.destination =
			    asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.name;
			rc = mqa_send_to_destination(mqa_cb, &destination_mqnd, qsend_evt, ackFlags,
						     param->info.timeout, length);
			if (rc != NCSCC_RC_SUCCESS)
				TRACE_2("Message Send through MDS Failure");
		} else {
			qsend_evt->info.sndMsgAsync.SendMsg.queueHandle =
			    asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.hdl;
			qsend_evt->info.sndMsgAsync.SendMsg.destination =
			    asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.name;
			rc = mqa_send_to_destination_async(mqa_cb, &destination_mqnd, qsend_evt, length);
			if (rc != NCSCC_RC_SUCCESS)
				TRACE_2("Message Send through MDS Failure");
		}
	}

	else {
		asapi_or->info.dest.o_cache->info.ginfo.pQueue = 0;

		qsend_evt_buffer = (MQSV_DSEND_EVT *)mds_alloc_direct_buff(length);
		memset(qsend_evt_buffer, 0, length);
		memcpy(qsend_evt_buffer, qsend_evt, length);

		rc = SA_AIS_OK;
		do {
			asapi_queue_select(&(asapi_or->info.dest.o_cache->info.ginfo));

			if (asapi_or->info.dest.o_cache->info.ginfo.pQueue) {
				if (!param->async_flag) {
					qsend_evt->evt_type = MQSV_DSEND_EVENT;
					qsend_evt->info.snd_msg.queueHandle =
					    asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.hdl;
					qsend_evt->info.snd_msg.destination =
					    asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.name;
					destination_mqnd = asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.addr;
				} else {
					qsend_evt->info.sndMsgAsync.SendMsg.queueHandle =
					    asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.hdl;
					qsend_evt->info.sndMsgAsync.SendMsg.destination =
					    asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.name;
					destination_mqnd = asapi_or->info.dest.o_cache->info.ginfo.pQueue->param.addr;
				}
			} else {
				mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt);
				mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt_buffer);
				TRACE_2("ERR_QUEUE_AVAILABLE: There are no queues in the group");
				rc = SA_AIS_ERR_QUEUE_NOT_AVAILABLE;
				return rc;
			}
			to_dest_ver = mqa_cb->ver_mqnd[mqsv_get_phy_slot_id(destination_mqnd)];

			/* MQND HAS GONE DOWN OR NOT YET UP */
			if (to_dest_ver == 0) {
				/* Drop The Message */
				TRACE_2("MQND HAS GONE DOWN %u", to_dest_ver);
				mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt);
				goto loop;
			}

			o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(to_dest_ver,
							      MQA_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT,
							      MQA_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT,
							      mqa_mqnd_msg_fmt_table);
			if (!o_msg_fmt_ver) {
				/* Drop The Message */
				TRACE_2("ERR_VERSION: Message Format version Invalid %u", o_msg_fmt_ver);
				mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt);
				goto loop;
			}

			/*Fill the message format version before sending the message */
			qsend_evt->msg_fmt_version = o_msg_fmt_ver;

			if (!param->async_flag)
				status =
				    mqa_send_to_destination(mqa_cb, &destination_mqnd, qsend_evt, ackFlags,
							    param->info.timeout, length);
			else
				status = mqa_send_to_destination_async(mqa_cb, &destination_mqnd, qsend_evt, length);
			if (status != NCSCC_RC_SUCCESS)
				TRACE_2("Message Send through MDS Failure %d", status);

			if (status == SA_AIS_ERR_QUEUE_FULL && rc == SA_AIS_ERR_QUEUE_FULL)
				rc = status;
			else if (status == NCSCC_RC_SUCCESS) {
				rc = SA_AIS_OK;
				is_send_success = true;
			} else
				rc = status;

			if (is_send_success)
				rc = SA_AIS_OK;

 loop:
			num_queues--;

			if (num_queues > 0) {
				qsend_evt_copy = (MQSV_DSEND_EVT *)mds_alloc_direct_buff(length);
				if (!qsend_evt_copy) {
					mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt_buffer);
					TRACE_4("ERR_MEMORY: MDS_DIRECT_BUFF free failed"); 
					return SA_AIS_ERR_NO_MEMORY;
				}
				memset(qsend_evt_copy, 0, length);
				qsend_evt = qsend_evt_copy;
				memcpy(qsend_evt, qsend_evt_buffer, length);
			}

		} while (num_queues > 0);
		mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt_buffer);

		if ((is_send_success == false) && (rc != SA_AIS_ERR_QUEUE_FULL)) {
			TRACE_2("ERR_TRY_AGAIN: sending message to message queue failed");
			return SA_AIS_ERR_TRY_AGAIN;
		} else if ((is_send_success == false) && (rc == SA_AIS_ERR_QUEUE_FULL)) {
			TRACE_2("Message Queue full");
			return SA_AIS_ERR_QUEUE_FULL;
                }

	}

	TRACE_LEAVE2("return code %d", rc);
	return rc;
}

/****************************************************************************
  Name          : mqa_send_message
 
  Description   : This routine sends a message to the queue denoted by destination
                  and waits for ack.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                  const SaNameT *destination - destination queue name to send to.
                  const SaMsgMessageT *message - The message to be sent.
                  SaMsgAckFlagsT ackFlags - acknowledgement required/not
                  SaTimeT timeout - time to wait for acknowledgement.     
                  MQA_SEND_MESSAGE_PARAM *param - points to invocation for 
                                                   async call, points to timeout for
                                                   sync call.
                  MQA_CB *mqa_cb - MQA control block

  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT mqa_send_message(SaMsgHandleT msgHandle,
			     const SaNameT *destination,
			     const SaMsgMessageT *message,
			     SaMsgAckFlagsT ackFlags, MQA_SEND_MESSAGE_PARAM *param, MQA_CB *mqa_cb)
{

	MQA_CLIENT_INFO *client_info;
	SaAisErrorT rc = SA_AIS_OK;
	ASAPi_OPR_INFO asapi_or;
	SaTimeT timeout;
	MQSV_DSEND_EVT *qsend_evt = NULL;
	SaMsgQueueHandleT queueHandle = 0;
	MDS_DEST destination_mqnd;
	bool lock_taken = false;
	uint32_t length, o_msg_fmt_ver = MQA_PVT_SUBPART_VERSION, to_dest_ver;


	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	if ((destination == NULL) || (message == NULL)) {
		TRACE_2("ERR_INVALID_PARAM: destination is NULL or message is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (destination->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: destinationName exceeds character 256");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	/* To memset the bytes to zero other than the length bytes in the SaNameT Structure */
	m_MQSV_SET_SANAMET(destination);
	TRACE_1("destination queue name %s", destination->value);

	if (m_MQSV_IS_ACKFLAGS_NOT_VALID(ackFlags)) {
		TRACE_2("ERR_BAD_FLAGS: supported ackFlags SA_MSG_MESSAGE_DELIVERED_ACK");
		rc = SA_AIS_ERR_BAD_FLAGS;
		return rc;
	}

	if (message->priority > SA_MSG_MESSAGE_LOWEST_PRIORITY) {
		TRACE_2("ERR_INVALID_PARAM: priority of message should not exceed 3");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (message->size > MQSV_MAX_SND_SIZE) {
		TRACE_4("ERR_RESOURCES: Message size is greater than the system defined size");
		rc = SA_AIS_ERR_NO_RESOURCES;
		return rc;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	lock_taken = true;

	/* Check if mqd is up */
	if (!mqa_cb->is_mqd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);

	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* check to see if the grant callback was registered and ackflags are set */
	if ((param->async_flag) &&
	    (ackFlags & SA_MSG_MESSAGE_DELIVERED_ACK) && (!client_info->msgCallbacks.saMsgMessageDeliveredCallback)) {
		TRACE_2("ERR_INIT: Delivered Callback is not defined after mentioning the async send");
		rc = SA_AIS_ERR_INIT;
		goto done;
	}

	/* Get the destination MQND from ASAPi */
	memset(&asapi_or, 0, sizeof(asapi_or));
	asapi_or.type = ASAPi_OPR_GET_DEST;
	asapi_or.info.dest.i_object = *destination;
	m_ASAPi_TRACK_ENABLE_SET(asapi_or.info.dest.i_track);
	asapi_or.info.dest.i_sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	asapi_or.info.dest.i_sinfo.dest = mqa_cb->mqd_mds_dest;
	asapi_or.info.dest.i_sinfo.stype = MDS_SENDTYPE_SNDRSP;

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
	lock_taken = false;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != SA_AIS_OK) {
		TRACE_2("The ASAPi Get Dest Operation Failed");
		goto done;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	lock_taken = true;

	if (!asapi_or.info.dest.o_cache) {
		TRACE_2("ERR_EXIST: The ASAPi Get Dest Operation's result Cache does not exist");
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

	if (asapi_or.info.dest.o_cache->objtype == ASAPi_OBJ_QUEUE) {
		destination_mqnd = asapi_or.info.dest.o_cache->info.qinfo.param.addr;

		to_dest_ver = mqa_cb->ver_mqnd[mqsv_get_phy_slot_id(destination_mqnd)];

		/* MQND HAS GONE DOWN OR NOT YET UP */
		if (to_dest_ver == 0) {
			TRACE_2("ERR_TRY_AGAIN: MQND HAS GONE DOWN");
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;
		}

		o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(to_dest_ver,
						      MQA_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT,
						      MQA_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT, mqa_mqnd_msg_fmt_table);
		if (!o_msg_fmt_ver) {
			/* Drop The Message */
			TRACE_4("ERR_LIBRARY: Message Format version Invalid %u", o_msg_fmt_ver);
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
	}

	/* Allocate memory for the MQSV_DSEND_EVENT structure + data */
	length = sizeof(MQSV_DSEND_EVT) + message->size;

	qsend_evt = (MQSV_DSEND_EVT *)mds_alloc_direct_buff(length);
	if (!qsend_evt) {
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	memset(qsend_evt, 0, length);
	qsend_evt->evt_type = MQSV_DSEND_EVENT;
	qsend_evt->endianness = machineEndianness();
	qsend_evt->agent_mds_dest = mqa_cb->mqa_mds_dest;
	qsend_evt->msg_fmt_version = o_msg_fmt_ver;
	qsend_evt->src_dest_version = MQA_PVT_SUBPART_VERSION;

	/* Fill the queue handle for queues. The mqa_send_to_group()
	 * fills the queue handle for queue groups.  */
	if (asapi_or.info.dest.o_cache->objtype == ASAPi_OBJ_QUEUE)
		queueHandle = asapi_or.info.dest.o_cache->info.qinfo.param.hdl;

	if (param->async_flag == false) {
		qsend_evt->type.req_type = MQP_EVT_SEND_MSG;
		qsend_evt->info.snd_msg.ackFlags = ackFlags;
		qsend_evt->info.snd_msg.destination = *destination;

		qsend_evt->info.snd_msg.messageInfo.sender.senderId = 0;
		qsend_evt->info.snd_msg.messageInfo.sendReceive = SA_FALSE;
		qsend_evt->info.snd_msg.messageInfo.sendTime = 0;	/* to be filled by MQND */

		qsend_evt->info.snd_msg.message.type = message->type;
		qsend_evt->info.snd_msg.message.version = message->version;
		qsend_evt->info.snd_msg.message.size = message->size;
		qsend_evt->info.snd_msg.message.priority = message->priority;

		if (message->data)
			memcpy(qsend_evt->info.snd_msg.message.data, message->data, message->size);

		if (message->senderName)
			qsend_evt->info.snd_msg.message.senderName = *message->senderName;

		qsend_evt->info.snd_msg.msgHandle = msgHandle;
		qsend_evt->info.snd_msg.queueHandle = queueHandle;

	} else {
		qsend_evt->type.req_type = MQP_EVT_SEND_MSG_ASYNC;
		qsend_evt->info.sndMsgAsync.SendMsg.ackFlags = ackFlags;
		qsend_evt->info.sndMsgAsync.invocation = param->info.invocation;
		qsend_evt->info.sndMsgAsync.SendMsg.destination = *destination;

		qsend_evt->info.sndMsgAsync.SendMsg.messageInfo.sender.senderId = 0;
		qsend_evt->info.sndMsgAsync.SendMsg.messageInfo.sendReceive = SA_FALSE;
		qsend_evt->info.sndMsgAsync.SendMsg.messageInfo.sendTime = 0;	/* to be filled by MQND */

		qsend_evt->info.sndMsgAsync.SendMsg.message.type = message->type;
		qsend_evt->info.sndMsgAsync.SendMsg.message.version = message->version;
		qsend_evt->info.sndMsgAsync.SendMsg.message.size = message->size;
		qsend_evt->info.sndMsgAsync.SendMsg.message.priority = message->priority;

		if (message->data)
			memcpy(qsend_evt->info.sndMsgAsync.SendMsg.message.data, message->data, message->size);

		if (message->senderName)
			qsend_evt->info.sndMsgAsync.SendMsg.message.senderName = *message->senderName;

		qsend_evt->info.sndMsgAsync.SendMsg.msgHandle = msgHandle;
		qsend_evt->info.sndMsgAsync.SendMsg.queueHandle = queueHandle;
	}

	switch (asapi_or.info.dest.o_cache->objtype) {

	case ASAPi_OBJ_QUEUE:

		if (!param->async_flag) {
			timeout = param->info.timeout;
			rc = mqa_send_to_destination(mqa_cb, &destination_mqnd, qsend_evt, ackFlags, timeout, length);
		} else {
			rc = mqa_send_to_destination_async(mqa_cb, &destination_mqnd, qsend_evt, length);
		}
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_2("Message Send through MDS Failure");
		break;

	case ASAPi_OBJ_GROUP:
		rc = mqa_send_to_group(mqa_cb, &asapi_or, qsend_evt, ackFlags, param, length);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_2("Message Send to Queue Group Failure");
		break;

	default:
		TRACE_2("ERR_EXIST: Object type should be QUEUE or QUEUEGROUPS");
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

 done:
	if (lock_taken)
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);

	return rc;
}

/****************************************************************************
  Name          : saMsgMessageSend
 
  Description   : This routine sends a message to the queue denoted by destination
                  and waits for ack.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                  const SaNameT *destination - destination queue name to send to.
                  SaInvocationT invocation
                  const SaMsgMessageT *message - The message to be sent.
                  SaMsgAckFlagsT ackFlags - acknowledgement required/not
                  SaTimeT timeout - time to wait for acknowledgement.     
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT
saMsgMessageSend(SaMsgHandleT msgHandle, const SaNameT *destination, const SaMsgMessageT *message, SaTimeT timeout)
{

	MQA_CB *mqa_cb;
	MQA_SEND_MESSAGE_PARAM param;
	SaMsgAckFlagsT ackFlags;
	SaAisErrorT rc;
	MQA_CLIENT_INFO *client_info = NULL;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);

	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto done;
		}
	}

	if (m_NCS_SA_IS_VALID_TIME_DURATION(timeout) == false) {
		TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	param.async_flag = false;
	param.info.timeout = timeout;
	ackFlags = SA_MSG_MESSAGE_DELIVERED_ACK;

	timeout = m_MQSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout);

	if (timeout < NCS_SAF_MIN_ACCEPT_TIME) {
		TRACE_2("ERR_TIMEOUT: Invalid Parameter as input");
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	}

	rc = mqa_send_message(msgHandle, destination, message, ackFlags, &param, mqa_cb);

 done:
	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK) {
		TRACE_LEAVE2(" Success ");
	}else {
		if (rc == SA_AIS_ERR_TRY_AGAIN)
			MQA_TRY_AGAIN_WAIT;
		TRACE_LEAVE2(" Failed with return code %d", rc);
	}
	return rc;

}

/****************************************************************************
  Name          : saMsgMessageSendAsync
 
  Description   : This routine sends a message to the queue denoted by destination
                  and returns the status of the send operation.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                  const SaNameT *destination - destination queue name to send to.
                  SaInvocationT invocation
                  const SaMsgMessageT *message - The message to be sent.
                  SaMsgAckFlagsT ackFlags - acknowledgement required/not
                        
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT
saMsgMessageSendAsync(SaMsgHandleT msgHandle,
		      SaInvocationT invocation,
		      const SaNameT *destination, const SaMsgMessageT *message, SaMsgAckFlagsT ackFlags)
{

	MQA_CB *mqa_cb;
	MQA_SEND_MESSAGE_PARAM param;
	MQP_ASYNC_RSP_MSG mqp_async_rsp;
	SaAisErrorT rc;
	MQA_CLIENT_INFO *client_info = NULL;

	param.async_flag = true;
	param.info.invocation = invocation;
	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);

	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto done;
		}
	}

	if (ackFlags & SA_MSG_MESSAGE_DELIVERED_ACK) {
		mqp_async_rsp.next = NULL;
		mqp_async_rsp.callbackType = MQP_ASYNC_RSP_MSGDELIVERED;
		mqp_async_rsp.messageHandle = msgHandle;
		mqp_async_rsp.params.msgDelivered.error = SA_AIS_ERR_TIMEOUT;
		mqp_async_rsp.params.msgDelivered.invocation = invocation;

		if (mqa_create_and_start_timer(&mqp_async_rsp, mqp_async_rsp.params.msgDelivered.invocation) !=
		    NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_RESOURCES: Create and Start Tmr Failed");
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}
	}

	rc = mqa_send_message(msgHandle, destination, message, ackFlags, &param, mqa_cb);

	if ((rc != SA_AIS_OK) && (ackFlags & SA_MSG_MESSAGE_DELIVERED_ACK)) {
		/* Stop the timer for this async call */
		mqa_stop_and_delete_timer_by_invocation(NCS_INT64_TO_PTR_CAST(invocation));
	}

 done:
	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK) {
		TRACE_LEAVE2(" Success ");
	}else {
		if (rc == SA_AIS_ERR_TRY_AGAIN)
			MQA_TRY_AGAIN_WAIT;
		TRACE_LEAVE2(" Failed with return code %d", rc);
	}
	return rc;

}

/****************************************************************************
  Name          : mqa_receive_message
 
  Description   : This routine receives the message from the queue pointed by 
                  queuehandle.  This is called by messageget and messageReceivedGet
                  APIs. Timeout will be 0 for the receivedGet indicating the message
                  needs to be fetched without waiting.

  Arguments     : SaMsgQueueHandleT queueHandle - queue handle to get message from.
                  SaMsgMessageT *message - Buffer to receive the message. If this
                                                   buffer is NULL, message service lib
                                                   allocates the buffer. The caller of 
                                                   this API MUST free it.
                  SaTimeT *sendTime,
                  SaMsgSenderIdT *senderId,
                  SaTimeT timeout  - Time to wait for message in ms.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT mqa_receive_message(SaMsgQueueHandleT queueHandle,
				SaMsgMessageT *message, SaTimeT *sendTime, SaMsgSenderIdT *senderId, SaTimeT timeout)
{
	SaAisErrorT rc = SA_AIS_OK;
	MQSV_MESSAGE *mqsv_message;
	NCS_OS_POSIX_MQ_REQ_INFO mq_req;
	NCS_OS_POSIX_MQ_REQ_INFO mq_req_snd;
	NCS_OS_MQ_MSG mq_msg;
	MQA_QUEUE_INFO *queue_node;
	MQA_CB *mqa_cb;
	bool svc_allocated = false;
	MQA_SENDERID_INFO *sender_info = NULL;
	static int first = 1;
	MQSV_DSEND_EVT *stats = NULL, *statsrsp = NULL;
	uint32_t mds_rc, to_dest_ver, o_msg_fmt_ver = MQA_PVT_SUBPART_VERSION;
	tmr_t tmr_id = NULL;
	MQP_CANCEL_REQ *timer_arg = NULL;
	bool is_timer_present = false;
	bool posix_mq_get_failure;
	bool stats_update_failure = false;
	bool lock_taken = false;

	TRACE_ENTER2(" SaMsgQueueHandle %llu ", queueHandle);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	lock_taken = true;

	/* Check if mqd, mqnd are up */
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	/* Check if queueHandle is present in the tree */
	if ((queue_node = mqa_queue_tree_find_and_add(mqa_cb, queueHandle, false, NULL, 0)) == NULL) {
		TRACE_2("ERR_BAD_HANDLE: Queue Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (queue_node->client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto done;
		}
	}

	/* Increment the msg_get_count. This is used later during cancel request to 
	   send the same number of cancel messages to the queue.  */
	queue_node->msg_get_count++;
        TRACE_1("Increment the msg_get_count %u",queue_node->msg_get_count);

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
	lock_taken = false;

	if (!message || !senderId) {
		TRACE_2("ERR_INVALID_PARAM: message is NULL or senderId is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* Timer supports a granularity of not less than 10 ms, therefore timeout values less than
	   this threshold are assumed to have ZERO timeout */
	if (timeout < (SA_TIME_ONE_MILLISECOND * 10)) {
		timeout = 0;
		mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_RECV_ASYNC;
	} else {
		if (timeout != SA_TIME_MAX) {
			timeout = timeout / (SA_TIME_ONE_MILLISECOND * 10);
		}
		mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_RECV;
	}

	mq_req.info.recv.mqd = queueHandle;

	/* TBD: When POSIX is ready, pass the timeout. right now * wait indefinitely */
	memset(&mq_req.info.recv.timeout, 0, sizeof(NCS_OS_POSIX_TIMESPEC));

	/* Convert nano seconds to micro seconds. */
	mq_req.info.recv.timeout.tv_nsec = timeout;
	mq_req.info.recv.timeout.tv_sec = 0;

	mq_req.info.recv.i_msg = &mq_msg;
	mq_req.info.recv.datalen = NCS_OS_MQ_MAX_PAYLOAD;
	mq_req.info.recv.dataprio = 0;
	mq_req.info.recv.i_mtype = -7;

	/* Start timer = TimeOut value */
	if ((timeout != 0) && (timeout != SA_TIME_MAX)) {

		m_NCS_TMR_CREATE(tmr_id, timeout, msgget_timer_expired, (void *)&timer_arg);

		if (tmr_id == NULL) {
			TRACE_4("ERR_RESOURCES: Tmr Create Failed");
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}

		timer_arg = m_MMGR_ALLOC_MQA_CANCEL_REQ(sizeof(MQP_CANCEL_REQ));

		if (!timer_arg) {
			m_NCS_TMR_DESTROY(tmr_id);
			TRACE_4("ERR_MEMORY: Track Buffer Info Allocation Failed");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
		memset(timer_arg, 0, sizeof(MQP_CANCEL_REQ));
		timer_arg->queueHandle = queueHandle;
		timer_arg->timerId = tmr_id;

		m_NCS_TMR_START(tmr_id, timeout, msgget_timer_expired, (void *)&timer_arg);

		is_timer_present = true;
	}

 again:
	posix_mq_get_failure = false;

	if (ncs_os_posix_mq(&mq_req) != NCSCC_RC_SUCCESS) {
		if (timeout == 0) {
			TRACE_2("ERR_TIMEOUT: Message get failed "); 
			rc = SA_AIS_ERR_TIMEOUT;
			goto done;
		} else if (timeout == SA_TIME_MAX) {
			TRACE_4("ERR_RESOURCES: Message get failed due to native msgrcv error"); 
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		} else {
			posix_mq_get_failure = true;
		}
	}

	if (posix_mq_get_failure) {
		if (is_timer_present) {
			if (timer_arg == NULL) {
				/* If the timer expired and parallely the posix call unblocked due to error, the STOP TIMER
				   message would still remain in the queue which needs to be removed */
				memset(&mq_req, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
				mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_RECV_ASYNC;
				mq_req.info.recv.mqd = queueHandle;
				mq_req.info.recv.timeout.tv_nsec = 0;
				mq_req.info.recv.timeout.tv_sec = 0;
				mq_req.info.recv.i_msg = &mq_msg;
				mq_req.info.recv.datalen = sizeof(MQSV_MESSAGE);
				mq_req.info.recv.dataprio = 0;
				mq_req.info.recv.i_mtype = 1;
				is_timer_present = false;
				goto again;
			} else {
				m_NCS_TMR_STOP(tmr_id);
				m_NCS_TMR_DESTROY(tmr_id);
				m_MMGR_FREE_MQA_CANCEL_REQ(timer_arg);
			}
		}

		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("ERR_RESOURCES: Message get failed due to native msgrcv erro");
		goto done;
	}

	/* Fill in the message structure and return */
	mqsv_message = (MQSV_MESSAGE *)mq_msg.data;

	/* GENUINE message sent via one of the SEND APIs */
	if (mqsv_message->type == MQP_EVT_GET_REQ) {
		if (is_timer_present) {
			if (timer_arg == NULL) {
				/* If the timer expired and parallely the posix call unblocked due to a genuine message, the 
				   STOP TIMER message would still remain in the queue which needs to be removed. 
				   First put the real message back into the queue at 2nd HIGHEST PRIORITY */
				memset(&mq_req_snd, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
				mq_req_snd.req = NCS_OS_POSIX_MQ_REQ_MSG_SEND_ASYNC;
				mq_req_snd.info.send.mqd = queueHandle;
				mq_req_snd.info.send.datalen = sizeof(MQSV_MESSAGE) + message->size;
				mq_req_snd.info.send.i_msg = &mq_msg;
				mq_req_snd.info.send.i_mtype = 2;

				if (ncs_os_posix_mq(&mq_req_snd) != NCSCC_RC_SUCCESS) {
					TRACE_4("ERR_RESOURCES: Unable to put back the genuine message in msgget call");
					rc = SA_AIS_ERR_NO_RESOURCES;
					goto done;
				}

				/* the idea is to put the real message back and get back CANCEL message */
				memset(&mq_req, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
				mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_RECV_ASYNC;
				mq_req.info.recv.mqd = queueHandle;
				mq_req.info.recv.timeout.tv_nsec = 0;
				mq_req.info.recv.timeout.tv_sec = 0;
				mq_req.info.recv.i_msg = &mq_msg;
				mq_req.info.recv.datalen = sizeof(MQSV_MESSAGE);
				mq_req.info.recv.dataprio = 0;
				mq_req.info.recv.i_mtype = 1;
				is_timer_present = false;
				goto again;
			} else {
				m_NCS_TMR_STOP(tmr_id);
				m_NCS_TMR_DESTROY(tmr_id);
				m_MMGR_FREE_MQA_CANCEL_REQ(timer_arg);
			}
		}
	}

	/* STOP TIMER message sent via the expiry timer routine */
	if ((mqsv_message->type == MQP_EVT_CANCEL_REQ) && (mqsv_message->info.cancel_req.timerId != 0)) {
		if ((mqsv_message->info.cancel_req.timerId == tmr_id) && (timer_arg == NULL)) {
			TRACE_2("ERR_TIMEOUT: message sent via the expiry timer routine");
			rc = SA_AIS_ERR_TIMEOUT;
			goto done;
		} else {
			/* The timer of another saMsgMessageGet call expired due to which the posix call unblocked. The
			   STOP TIMER message meant for another call to saMsgMessageGet needs to be put back into the queue
			   and resume waiting for a genuine message until our timer expiry */
			memset(&mq_req_snd, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
			mq_req_snd.req = NCS_OS_POSIX_MQ_REQ_MSG_SEND_ASYNC;
			mq_req_snd.info.send.mqd = queueHandle;
			mq_req_snd.info.send.datalen = sizeof(MQSV_MESSAGE);
			mq_req_snd.info.send.i_msg = &mq_msg;
			mq_req_snd.info.send.i_mtype = 1;

			if (ncs_os_posix_mq(&mq_req_snd) != NCSCC_RC_SUCCESS) {
				TRACE_4("ERR_RESOURCES: Unable to put back the stop Tmr message"
					" which is meant for a different msgget");
				rc = SA_AIS_ERR_NO_RESOURCES;
				goto done;
			}

			if (is_timer_present == false) {
				memset(&mq_req, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
				mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_RECV_ASYNC;
				mq_req.info.recv.mqd = queueHandle;
				mq_req.info.recv.timeout.tv_nsec = 0;
				mq_req.info.recv.timeout.tv_sec = 0;
				mq_req.info.recv.i_msg = &mq_msg;
				mq_req.info.recv.datalen = sizeof(MQSV_MESSAGE);
				mq_req.info.recv.dataprio = 0;
				mq_req.info.recv.i_mtype = 1;
			}
			goto again;
		}
	}

	/* CANCEL message sent via saMsgMessageCancel API */
	if ((mqsv_message->type == MQP_EVT_CANCEL_REQ) && (mqsv_message->info.cancel_req.timerId == 0)) {
		if (is_timer_present) {
			if (timer_arg == NULL) {
				/* If the timer expired and parallely the posix call unblocked due to MSG_CANCEL, the STOP TIMER
				   message would still remain in the queue which needs to be removed */
				memset(&mq_req, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
				mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_RECV_ASYNC;
				mq_req.info.recv.mqd = queueHandle;
				mq_req.info.recv.timeout.tv_nsec = 0;
				mq_req.info.recv.timeout.tv_sec = 0;
				mq_req.info.recv.i_msg = &mq_msg;
				mq_req.info.recv.datalen = sizeof(MQSV_MESSAGE);
				mq_req.info.recv.dataprio = 0;
				mq_req.info.recv.i_mtype = 1;
				is_timer_present = false;
				goto again;
			} else {
				m_NCS_TMR_STOP(tmr_id);
				m_NCS_TMR_DESTROY(tmr_id);
				m_MMGR_FREE_MQA_CANCEL_REQ(timer_arg);
			}
		}
		TRACE_2("ERR_INTERRUPT: INTERRUPT occurs ");
		rc = SA_AIS_ERR_INTERRUPT;
		goto done;
	}

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}
	lock_taken = true;

	if ((queue_node = mqa_queue_tree_find_and_add(mqa_cb, queueHandle, false, NULL, 0)) == NULL) {
		TRACE_2("ERR_BAD_HANDLE: Queue Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* Decriment the msg_get_count, after msg get (This is used by Cancel Messages */
	queue_node->msg_get_count--;

	/* This memory allocated has to be freed by the application */
	if (!message->data) {
		if (mqsv_message->info.msg.message.size != 0) {
			message->data = (void *)malloc((uint32_t)mqsv_message->info.msg.message.size);
			if (!message->data) {
				TRACE_4("ERR_MEMORY: Memory allocation failed");
				rc = SA_AIS_ERR_NO_MEMORY;
				goto done;
			}
			svc_allocated = true;
		}
	} else {
		if (mqsv_message->info.msg.message.size > message->size) {
			/* return the size needed */
			message->size = mqsv_message->info.msg.message.size;
			TRACE_2("ERR_NO_SPACE: message size is NULL");
			rc = SA_AIS_ERR_NO_SPACE;

			/* Post the message back to Queue, with second highest priority 
			   Highest priority i.e. 1 is reserved for MQP_EVT_MSGGET_STOP_TIMER_REQ & 
			   MQP_EVT_CANCEL_REQ messages */
			memset(&mq_req, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
			mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_SEND_ASYNC;
			mq_req.info.send.mqd = queueHandle;
			mq_req.info.send.datalen = sizeof(MQSV_MESSAGE) + message->size;
			mq_req.info.send.i_msg = &mq_msg;
			mq_req.info.send.i_mtype = 2;

			if (ncs_os_posix_mq(&mq_req) != NCSCC_RC_SUCCESS) {
				TRACE_4("Unable to put back the genuine message in msgget call");
				/* TBD: Don't know what to do */
			}

			/* if queue opened with RCV_CALLBACK option, put back the indicator msg into the listener queue */
			if (queue_node->openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK)
				mqsv_listenerq_msg_send(queue_node->listenerHandle);

			goto done;
		}
	}

	memcpy(message->data, mqsv_message->info.msg.message.data, mqsv_message->info.msg.message.size);
	message->priority = mqsv_message->info.msg.message.priority;
	message->size = mqsv_message->info.msg.message.size;
	message->type = mqsv_message->info.msg.message.type;
	message->version = mqsv_message->info.msg.message.version;
	if (message->senderName)
		*message->senderName = mqsv_message->info.msg.message.senderName;

	to_dest_ver = mqa_cb->ver_mqnd[mqsv_get_phy_slot_id(mqa_cb->mqnd_mds_dest)];

	/* MQND HAS GONE DOWN OR NOT YET UP */
	if (to_dest_ver == 0) {
		TRACE_2("ERR_TRY_AGAIN: MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(to_dest_ver,
					      MQA_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT,
					      MQA_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT, mqa_mqnd_msg_fmt_table);
	if (!o_msg_fmt_ver) {
		/* Drop The Message */
		TRACE_2("Message Format version Invalid %u", o_msg_fmt_ver);
		goto done;
	}

	/* Send the priority and message size to MQND so that MQND update * the queue stats  */
	stats = (MQSV_DSEND_EVT *)mds_alloc_direct_buff(sizeof(MQSV_DSEND_EVT));
	if (!stats) {
		TRACE_4("ERR_MEMORY: MQSV_DSEND_EVT Memory allocation failed");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* populate the structure */
	memset(stats, 0, sizeof(MQSV_DSEND_EVT));

	stats->evt_type = MQSV_DSEND_EVENT;
	stats->endianness = machineEndianness();
	stats->msg_fmt_version = o_msg_fmt_ver;
	stats->src_dest_version = MQA_PVT_SUBPART_VERSION;
	stats->type.req_type = MQP_EVT_STAT_UPD_REQ;
	stats->agent_mds_dest = mqa_cb->mqa_mds_dest;

	stats->info.statsReq.qhdl = queueHandle;
	stats->info.statsReq.priority = message->priority;
	stats->info.statsReq.size = message->size;

	/* send the request to the MQND */
	mds_rc = mqa_mds_msg_sync_send_direct(mqa_cb->mqa_mds_hdl, &mqa_cb->mqnd_mds_dest, stats,
					      &statsrsp, (uint32_t)MQSV_WAIT_TIME, sizeof(MQSV_DSEND_EVT));

	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		rc = SA_AIS_OK;
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		stats_update_failure = true;
		goto check;
	case NCSCC_RC_FAILURE:
		TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto check;
	default:
		TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NO_RESOURCES;
		stats_update_failure = true;
		goto check;
	}

	/* In case if STATS_UPDATE fails */
	if (statsrsp) {
		if ((statsrsp->type.rsp_type == MQP_EVT_STAT_UPD_RSP)) {

			if (statsrsp->info.sendMsgRsp.error != SA_AIS_OK) {
				rc = statsrsp->info.sendMsgRsp.error;
				stats_update_failure = true;
			}

			mds_free_direct_buff((MDS_DIRECT_BUFF)statsrsp);
		}
	} else {
		TRACE_4("ERR_LIBRARY: STATS_UPDATE fails");
		rc = SA_AIS_ERR_LIBRARY;
		stats_update_failure = true;
	}

 check:

	if (stats_update_failure) {
		/* Post the message back to Queue, with second highest priority 
		   Highest priority i.e. 1 is reserved for MQP_EVT_MSGGET_STOP_TIMER_REQ & MQP_EVT_CANCEL_REQ messages */
		memset(&mq_req, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
		mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_SEND_ASYNC;
		mq_req.info.send.mqd = queueHandle;
		mq_req.info.send.datalen = sizeof(MQSV_MESSAGE) + message->size;
		mq_req.info.send.i_msg = &mq_msg;
		mq_req.info.send.i_mtype = 2;

		if (ncs_os_posix_mq(&mq_req) != NCSCC_RC_SUCCESS) {
			TRACE_4("Unable to put back the genuine message in msgget call");
			/* TBD: Don't know what to do */
		}

		/* if queue opened with RCV_CALLBACK option, put back the indicator msg into the listener queue */
		if (queue_node->openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK)
			mqsv_listenerq_msg_send(queue_node->listenerHandle);

		goto done;
	}

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
	lock_taken = false;

	/* Populate the sender MDS DEST and NCSCONTEXT into the senderId field.
	 * This will be later used in saMsgMessageReply to get info about the sender.  */
	if (mqsv_message->info.msg.message_info.sendReceive == SA_TRUE) {
		if (!senderId) {
			TRACE_2("ERR_INVALID_PARAM: senderId is NULL");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}

		sender_info = m_MMGR_ALLOC_MQA_SENDERID;
		if (!sender_info) {
			TRACE_4("ERR_MEMORY: Memory allocation failed");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}

		sender_info->sender_context.context = mqsv_message->info.msg.message_info.sender.sender_context.context;
		sender_info->sender_context.sender_dest =
		    mqsv_message->info.msg.message_info.sender.sender_context.sender_dest;
		sender_info->sender_context.src_dest_version =
		    mqsv_message->info.msg.message_info.sender.sender_context.src_dest_version;
		sender_info->sender_context.reply_buffer_size =
		    mqsv_message->info.msg.message_info.sender.sender_context.reply_buffer_size;

		m_NCS_OS_GET_TIME_STAMP(sender_info->timestamp);

		/* Enqueue the sender id info to the mqa_senderid_list */
		if (ncs_enqueue(&(mqa_cb->mqa_senderid_list), (void *)sender_info) != NCSCC_RC_SUCCESS) {
			TRACE_2("ERR_NO_SPACE: Insertion of the sender id into the senderid list failed");
			rc = SA_AIS_ERR_NO_SPACE;
			goto done;
		}

		*senderId = NCS_PTR_TO_UNS64_CAST(sender_info);
		if (sendTime != NULL)
			*sendTime = mqsv_message->info.msg.message_info.sendTime;

		if (first) {
			if (mqa_create_and_start_senderid_timer() != NCSCC_RC_SUCCESS) {
				TRACE_4("ERR_RESOURCES: Create and Start Tmr Failed");
				rc = SA_AIS_ERR_NO_RESOURCES;
				goto done;
			}
			first = 0;
		}
	} else {
		if (senderId)
			*senderId = mqsv_message->info.msg.message_info.sender.senderId;
		if (sendTime)
			*sendTime = mqsv_message->info.msg.message_info.sendTime;
	}

	rc = SA_AIS_OK;

 done:

	if (lock_taken)
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	if ((rc != SA_AIS_OK) && (svc_allocated == true))
		if (message->data) {
			free(message->data);
			message->data = NULL;
		}
	if (rc != SA_AIS_OK)
		m_MMGR_FREE_MQA_SENDERID(sender_info);

	m_MQSV_MQA_GIVEUP_MQA_CB;

	TRACE_LEAVE2(" return code %d ", rc);
	return rc;
}

/****************************************************************************
  Name          : saMsgMessageGet
 
  Description   : This routine receives the message from the queue pointed by 
                  queuehandle.  The caller MUST not call this function if there
                  is already a pending saMsgMessageReceivedGet.

  Arguments     : SaMsgQueueHandleT queueHandle - queue handle to get message from.
                  SaMsgMessageT *message - Buffer to receive the message. If this
                                                   buffer is NULL, message service lib
                                                   allocates the buffer. The caller of 
                                                   this API MUST free it.
                  SaMsgMessageInfoT *messageInfo - Buffer to hold the message info.
                                                         must be non NULL.
                  SaTimeT timeout  - Time to wait for message in ms.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT
saMsgMessageGet(SaMsgQueueHandleT queueHandle,
		SaMsgMessageT *message, SaTimeT *sendTime, SaMsgSenderIdT *senderId, SaTimeT timeout)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2(" SaMsgQueueHandle %llu ", queueHandle);

	if (m_NCS_SA_IS_VALID_TIME_DURATION(timeout) == false) {
		TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	rc = (mqa_receive_message(queueHandle, message, sendTime, senderId, timeout));

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;

}

/****************************************************************************
  Name          : saMsgMessageCancel
 
  Description   : This routine cancels all the pending saMsgMessageGet() calls 
                  that are blocking on this queueHandle. It sends cancel messages
                  to the queue pointed by queueHandle so all the blocking threads
                  pick up these messages and exit.
 
  Arguments     : SaMsgQueueHandleT queueHandle 
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgMessageCancel(SaMsgQueueHandleT queueHandle)
{
/* Send a shutdown messages = number of saMsgMessageGet() calls invoked, to the queue */

	SaAisErrorT rc = SA_AIS_OK;
	MQSV_MESSAGE *mqsv_message;
	NCS_OS_POSIX_MQ_REQ_INFO mq_req;
	NCS_OS_MQ_MSG mq_msg;
	MQA_QUEUE_INFO *queue_node;
	MQA_CB *mqa_cb;
	uint32_t cancel_message_count;
	uint8_t i;

	TRACE_ENTER2(" SaMsgQueueHandle %llu ", queueHandle);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return SA_AIS_ERR_LIBRARY;
	}

	/* Check if mqd, mqnd are up */
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* Check if queueHandle is present in the tree */
	if ((queue_node = mqa_queue_tree_find_and_add(mqa_cb, queueHandle, false, NULL, 0)) == NULL) {
		TRACE_2("ERR_BAD_HANDLE: Queue Database Find Failed");
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return SA_AIS_ERR_BAD_HANDLE;
	}
	if (queue_node->client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			m_MQSV_MQA_GIVEUP_MQA_CB;
			return rc;
		}
	}

	cancel_message_count = queue_node->msg_get_count;
        TRACE_1("Assign the same msg_get_count in message get to cancel messages %u",cancel_message_count);

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	if (cancel_message_count == 0) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_2("ERR_EXIST: cancel_message_count is 0");
		return SA_AIS_ERR_NOT_EXIST;
	}

	mqsv_message = (MQSV_MESSAGE *)mq_msg.data;
	mqsv_message->type = MQP_EVT_CANCEL_REQ;
	mqsv_message->info.cancel_req.queueHandle = queueHandle;
	mqsv_message->info.cancel_req.timerId = 0;	/* This is TO BE USED for stopping messageGet timers only */

	memset(&mq_req, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
	mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_SEND_ASYNC;
	mq_req.info.send.i_msg = &mq_msg;
	mq_req.info.send.datalen = sizeof(MQSV_MESSAGE);
	mq_req.info.send.mqd = queueHandle;
	mq_req.info.send.i_mtype = 1;
	/* Send the message from the Queue using the OS call ->ncs_os_mq() */

	for (i = 0; i < cancel_message_count; i++) {
		if (m_NCS_OS_POSIX_MQ(&mq_req) != NCSCC_RC_SUCCESS) {
			TRACE_2("ERR_TRY_AGAIN: Unable to put the cancel message in the queue");
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
	}

	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);

	return rc;
}

/****************************************************************************
  Name          : mqa_send_receive
 
  Description   : This routine is called by SaMsgMessageSendReceive to 
                  sends a MQSV_DSEND_EVT message waits for the reply and 
                  acks for the reply MQSV_DSEND_EVT message.
 
  Arguments     : MQA_CB *mqa_cb - MQA control block
                  MDS_DEST *mqnd_mds_dest - destination MQND to reply to
                  MQSV_DSEND_EVT *qsend_evt - the send event structure.
                  MQSV_DSEND_EVT *qreply_evt - the reply event structure that was received.
                  SaTimeT timeout - time to wait for an acknowledgement for this reply.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT mqa_send_receive(MQA_CB *mqa_cb, MDS_DEST *mqnd_mds_dest,
			     MQSV_DSEND_EVT *qsend_evt, MQSV_DSEND_EVT **qreply_evt, SaTimeT timeout, uint32_t length)
{

	int64_t mqa_timeout;
	SaAisErrorT rc;
	uint32_t mds_rc;

	TRACE_ENTER2(" SaTime %u ", (uint32_t)timeout);

	/* convert the timeout to 10 ms value and add it to the sync send timeout */
	mqa_timeout = m_MQSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout);

	mds_rc = mqa_mds_msg_sync_send_direct(mqa_cb->mqa_mds_hdl, mqnd_mds_dest, qsend_evt,
					      qreply_evt, (uint32_t)mqa_timeout, length);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		rc = SA_AIS_OK;
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		break;
	case NCSCC_RC_FAILURE:
		TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		break;
	default:
		TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NO_RESOURCES;
		break;
	}

	/* In case if Sending to the destination queue fails then don't wait for the reply message to come
	   Process the reply event received from the MQND */
	if (*qreply_evt) {
		if ((*qreply_evt)->type.rsp_type == MQP_EVT_SEND_MSG_RSP) {
			if ((*qreply_evt)->info.sendMsgRsp.error != SA_AIS_OK)
				rc = (*qreply_evt)->info.sendMsgRsp.error;

			mds_free_direct_buff((MDS_DIRECT_BUFF)*qreply_evt);
			*qreply_evt = NULL;
		}
	}
	TRACE_LEAVE2(" return code %d ", rc);
	return rc;
}

/****************************************************************************
  Name          : saMsgMessageSendReceive
 
  Description   : This routine sends a message waits for the reply and 
                  acks for the reply message.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                  const SaNameT *destination - destination queue name to send to.
                  const SaMsgMessageT *sendMessage - The message to be sent.
                       const SaMsgMessageT *receiveMessage - The reply message.
                       const SaMsgMessageInfoT *receiveMessageInfo - received message
                                                                     information.
                       SaMsgAckFlagsT ackFlags - acknowledgement required/not
                        SaTimeT timeout - time to wait for reply in ms.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/
SaAisErrorT
saMsgMessageSendReceive(SaMsgHandleT msgHandle,
			const SaNameT *destination,
			const SaMsgMessageT *sendMessage,
			SaMsgMessageT *receiveMessage, SaTimeT *replySendTime, SaTimeT timeout)
{
	MQA_CB *mqa_cb;
	MQA_CLIENT_INFO *client_info;
	SaAisErrorT rc = SA_AIS_OK, rc1 = SA_AIS_OK;
	ASAPi_OPR_INFO asapi_or;
	MQSV_DSEND_EVT *qreply_evt = NULL;
	MQSV_DSEND_EVT *qsend_evt = NULL;
	MDS_DEST destination_mqnd, reply_mds_dest;
	MQSV_EVT msg_dlvr_ack;
	uint8_t unicast = 0;
	uint32_t num_queues;
	uint64_t reply_msgsize;
	bool is_svc_allocated = false;
	uint32_t length, to_dest_ver, o_msg_fmt_ver;


	TRACE_ENTER2(" SaMsgHandle %llu ", msgHandle);

	if ((!destination) || (!sendMessage) || (!receiveMessage)) {
		TRACE_2("ERR_INVALID_PARAM: destination is NULL or sendMessage is NULL or receiveMessage is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (destination->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: destinationName exceeds character 256");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	/* To memset the bytes to zero other than the length bytes in the SaNameT Structure */
	m_MQSV_SET_SANAMET(destination);
	TRACE_1("Message send to queueName %s", destination->value);

	if (m_NCS_SA_IS_VALID_TIME_DURATION(timeout) == false) {
		TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (sendMessage->priority > SA_MSG_MESSAGE_LOWEST_PRIORITY) {
		TRACE_2("ERR_INVALID_PARAM: Invalid message priority of the message queue");
		rc = SA_AIS_ERR_INVALID_PARAM;
		return rc;
	}

	if (sendMessage->size > MQSV_MAX_SND_SIZE) {
		TRACE_4("ERR_RESOURCES: Message size is greater than the system defined size");
		rc = SA_AIS_ERR_NO_RESOURCES;
		return rc;
	}

	/* If the timeout is less than the minimum specified return */
	if (m_MQSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout) < NCS_SAF_MIN_ACCEPT_TIME) {
		TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
		rc = SA_AIS_ERR_TIMEOUT;
		return rc;
	}

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return SA_AIS_ERR_LIBRARY;
	}

	/* Check if mqd is up */
	if (!mqa_cb->is_mqd_up) {
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_2("ERR_TRY_AGAIN: MQD is down");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			m_MQSV_MQA_GIVEUP_MQA_CB;
			return rc;
		}
	}

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	/* Get the destination MQND from ASAPi */
	memset(&asapi_or, 0, sizeof(asapi_or));
	asapi_or.type = ASAPi_OPR_GET_DEST;
	asapi_or.info.dest.i_object = *destination;
	m_ASAPi_TRACK_ENABLE_SET(asapi_or.info.dest.i_track);
	asapi_or.info.dest.i_sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	asapi_or.info.dest.i_sinfo.dest = mqa_cb->mqd_mds_dest;
	asapi_or.info.dest.i_sinfo.stype = MDS_SENDTYPE_SNDRSP;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != SA_AIS_OK) {
		TRACE_2("The ASAPi Get Dest Operation Failed");
		goto done1;
	}

	/* Allocate memory for the MQSV_DSEND_EVT structure + data */
	length = sizeof(MQSV_DSEND_EVT) + sendMessage->size;

	qsend_evt = (MQSV_DSEND_EVT *)mds_alloc_direct_buff(length);
	if (!qsend_evt) {
		TRACE_2("ERR_MEMORY: MQSV_DSEND_EVT Memory allocation failed");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done1;
	}

	memset(qsend_evt, 0, length);

	qsend_evt->evt_type = MQSV_DSEND_EVENT;
	qsend_evt->type.req_type = MQP_EVT_SEND_MSG;
	qsend_evt->endianness = machineEndianness();
	qsend_evt->src_dest_version = MQA_PVT_SUBPART_VERSION;
	qsend_evt->agent_mds_dest = mqa_cb->mqa_mds_dest;

	qsend_evt->info.snd_msg.destination = *destination;
	qsend_evt->info.snd_msg.message.type = sendMessage->type;
	qsend_evt->info.snd_msg.message.version = sendMessage->version;
	qsend_evt->info.snd_msg.message.size = sendMessage->size;
	qsend_evt->info.snd_msg.message.priority = sendMessage->priority;

	if (sendMessage->senderName)
		qsend_evt->info.snd_msg.message.senderName = *sendMessage->senderName;

	if (sendMessage->data)
		memcpy(qsend_evt->info.snd_msg.message.data, sendMessage->data, sendMessage->size);

	qsend_evt->info.snd_msg.messageInfo.sender.sender_context.sender_dest = mqa_cb->mqa_mds_dest;
	qsend_evt->info.snd_msg.messageInfo.sender.sender_context.src_dest_version = MQA_PVT_SUBPART_VERSION;

	if (receiveMessage->data)
		qsend_evt->info.snd_msg.messageInfo.sender.sender_context.reply_buffer_size = receiveMessage->size;
	else
		qsend_evt->info.snd_msg.messageInfo.sender.sender_context.reply_buffer_size = 0;

	/* context will be filled by MQND before putting into the queue at the destination node */
	memset(&(qsend_evt->info.snd_msg.messageInfo.sender.sender_context.context), 0, sizeof(MDS_SYNC_SND_CTXT));

	qsend_evt->info.snd_msg.messageInfo.sendReceive = SA_TRUE;
	qsend_evt->info.snd_msg.messageInfo.sendTime = 0;	/* will be filled by MQND */
	qsend_evt->info.snd_msg.msgHandle = msgHandle;

	switch (asapi_or.info.dest.o_cache->objtype) {
	case ASAPi_OBJ_QUEUE:
		qsend_evt->info.snd_msg.queueHandle = asapi_or.info.dest.o_cache->info.qinfo.param.hdl;
		qsend_evt->info.snd_msg.destination = asapi_or.info.dest.o_cache->info.qinfo.param.name;
		destination_mqnd = asapi_or.info.dest.o_cache->info.qinfo.param.addr;
		break;

	case ASAPi_OBJ_GROUP:
		num_queues = asapi_or.info.dest.o_cache->info.ginfo.qlist.count;

		if (num_queues == 0) {
			mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt);
			TRACE_2("ERR_QUEUE_NOT_AVAILABLE: There are no queues in the group");
			rc = SA_AIS_ERR_QUEUE_NOT_AVAILABLE;
			goto done1;
		}

		/* Right now, group is considered multicast if the selected group policy 
		 * is not round robin. Need to define multicast group in SAF spec in future.  */
		if (asapi_or.info.dest.o_cache->info.ginfo.policy == SA_MSG_QUEUE_GROUP_ROUND_ROBIN)
			unicast = 1;

		if (unicast) {
			asapi_or.info.dest.o_cache->info.ginfo.pQueue = 0;
			asapi_queue_select(&(asapi_or.info.dest.o_cache->info.ginfo));
			qsend_evt->info.snd_msg.queueHandle = asapi_or.info.dest.o_cache->info.ginfo.pQueue->param.hdl;
			qsend_evt->info.snd_msg.destination = asapi_or.info.dest.o_cache->info.ginfo.pQueue->param.name;
			destination_mqnd = asapi_or.info.dest.o_cache->info.ginfo.pQueue->param.addr;
		} else {
			mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt);
			TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done1;
		}
		break;

	default:
		mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt);
		TRACE_2("ERR_EXIST: Invalid Parameter as input");
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done1;
	}

	to_dest_ver = mqa_cb->ver_mqnd[mqsv_get_phy_slot_id(destination_mqnd)];

	/* MQND HAS GONE DOWN OR NOT YET UP */
	if (to_dest_ver == 0) {
		TRACE_2("ERR_TRY_AGAIN: MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done1;
	}

	o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(to_dest_ver,
					      MQA_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT,
					      MQA_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT, mqa_mqnd_msg_fmt_table);

	if (!o_msg_fmt_ver) {
		/* Drop The Message */
		mds_free_direct_buff((MDS_DIRECT_BUFF)qsend_evt);
		TRACE_2("ERR_VERSION: Message Format version Invalid %u", o_msg_fmt_ver);
		rc = SA_AIS_ERR_VERSION;
		goto done1;
	}

	qsend_evt->msg_fmt_version = o_msg_fmt_ver;

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		rc = SA_AIS_ERR_LIBRARY;
		goto done1;
	}

	rc = mqa_send_receive(mqa_cb, &destination_mqnd, qsend_evt, &qreply_evt, timeout, length);

	if (rc != SA_AIS_OK) {
		TRACE_2("The send part of the message sendreceive failed ");
		goto done;
	}

	if (!qreply_evt) {
		TRACE_2("ERR_TRY_AGAIN: The send part of the message sendreceive failed");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	if (qreply_evt->type.req_type == MQP_EVT_REPLY_MSG_ASYNC)
		reply_msgsize = qreply_evt->info.replyAsyncMsg.reply.message.size;
	else
		reply_msgsize = qreply_evt->info.replyMsg.message.size;

	/* Store the message to the reply buffer */
	if (!receiveMessage->data) {
		if (reply_msgsize != 0) {
			receiveMessage->data = (void *)malloc((uint32_t)reply_msgsize);
			if (!receiveMessage->data) {
				TRACE_4("ERR_MEMORY: Memory allocation failed");
				rc = SA_AIS_ERR_NO_MEMORY;
				goto send_del_callback;
			}
			is_svc_allocated = true;
		}
	} else {
		if (receiveMessage->size < reply_msgsize) {
			TRACE_2("ERR_NO_SPACE: There is no space synchronization in reply and sendrecive calls");
			rc = SA_AIS_ERR_NO_SPACE;
			goto send_del_callback;
		}
	}

	if (qreply_evt->type.req_type == MQP_EVT_REPLY_MSG_ASYNC) {
		if (receiveMessage->senderName)
			memcpy(receiveMessage->senderName,
			       &qreply_evt->info.replyAsyncMsg.reply.message.senderName, sizeof(SaNameT));

		receiveMessage->type = qreply_evt->info.replyAsyncMsg.reply.message.type;
		receiveMessage->version = qreply_evt->info.replyAsyncMsg.reply.message.version;
		receiveMessage->size = qreply_evt->info.replyAsyncMsg.reply.message.size;
		receiveMessage->priority = qreply_evt->info.replyAsyncMsg.reply.message.priority;

		if (reply_msgsize) {
			memcpy(receiveMessage->data,
			       qreply_evt->info.replyAsyncMsg.reply.message.data, receiveMessage->size);
		}
	} else {
		if (receiveMessage->senderName)
			memcpy(receiveMessage->senderName,
			       &qreply_evt->info.replyMsg.message.senderName, sizeof(SaNameT));

		receiveMessage->type = qreply_evt->info.replyMsg.message.type;
		receiveMessage->version = qreply_evt->info.replyMsg.message.version;
		receiveMessage->size = qreply_evt->info.replyMsg.message.size;
		receiveMessage->priority = qreply_evt->info.replyMsg.message.priority;

		if (reply_msgsize) {
			memcpy(receiveMessage->data, qreply_evt->info.replyMsg.message.data, receiveMessage->size);
		}
	}

	/* Store the replyTime */
	if (replySendTime) {
		m_GET_TIME_STAMP(*replySendTime);
	}

 send_del_callback:
	/* Send message delivered indication to the replier for async calls.
	 * For sync calls, the mds receive callback */
	if (qreply_evt) {
		/* Send ack to the replyAsync. The replies to reply Sync call will be handled
		 * in the MDS receive handler. MDS sends back the error of the MDS receive 
		 * handler */
		if ((qreply_evt->type.req_type == MQP_EVT_REPLY_MSG_ASYNC) &&
		    (qreply_evt->info.replyAsyncMsg.reply.ackFlags & SA_MSG_MESSAGE_DELIVERED_ACK)) {

			reply_mds_dest = qreply_evt->agent_mds_dest;
			/* Build the message delivered callback structure */
			msg_dlvr_ack.type = MQSV_EVT_MQA_CALLBACK;
			msg_dlvr_ack.msg.mqp_async_rsp.callbackType = MQP_ASYNC_RSP_MSGDELIVERED;
			msg_dlvr_ack.msg.mqp_async_rsp.messageHandle = qreply_evt->info.replyAsyncMsg.reply.msgHandle;
			msg_dlvr_ack.msg.mqp_async_rsp.params.msgDelivered.error = rc;
			msg_dlvr_ack.msg.mqp_async_rsp.params.msgDelivered.invocation
			    = qreply_evt->info.replyAsyncMsg.invocation;

			/* Send using async send */
			rc1 = mqa_mds_msg_async_send((mqa_cb->mqa_mds_hdl), &reply_mds_dest,
						     &msg_dlvr_ack, NCSMDS_SVC_ID_MQA);

			if (rc1 != NCSCC_RC_SUCCESS) {
				TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
				rc = SA_AIS_ERR_NO_RESOURCES;
			}
		}
	}

 done:
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

 done1:
	if (qreply_evt) {
		mds_free_direct_buff((MDS_DIRECT_BUFF)qreply_evt);
	}

	if (rc != SA_AIS_OK) {
		if (rc == SA_AIS_ERR_TRY_AGAIN) {
			TRACE_2("MsgQ Svc msgsendreceive failed");
			MQA_TRY_AGAIN_WAIT;
		}

		if (replySendTime)
			*replySendTime = SA_TIME_UNKNOWN;

		if (is_svc_allocated) {
			free(receiveMessage->data);
			receiveMessage->data = NULL;
		}
	}

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);

	m_MQSV_MQA_GIVEUP_MQA_CB;

	return rc;
}

/****************************************************************************
  Name          : mqa_match_senderid
 
  Description   : This routine matches the sender id passed as opaque arguments.
 
  Arguments     : void *key, - element to match for.
                  void *qelem - a queue element.
 
  Return Values : bool
 
  Notes         : None
******************************************************************************/

bool mqa_match_senderid(void *key, void *qelem)
{

	if (key == qelem)
		return true;

	return false;
}

/****************************************************************************
  Name          : mqa_reply_to_destination
 
  Description   : This routine replies to a received message, the receive message 
                  info being passed as  argument to the function.
 
  Arguments     : MQA_CB *mqa_cb - MQA control block
                  MDS_DEST *mqnd_mds_dest - destination MQND to reply to
                  MQSV_DSEND_EVT *qsend_evt - the reply event structure.
                  SaTimeT timeout - time to wait for an acknowledgement for this reply.
                  MDS_SYNC_SND_CTXT *context - MDS context to reply to.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT mqa_reply_to_destination(MQA_CB *mqa_cb, MDS_DEST *mqnd_mds_dest,
				     MQSV_DSEND_EVT *qsend_evt, SaMsgAckFlagsT ackFlags,
				     SaTimeT timeout, MDS_SYNC_SND_CTXT *context, uint32_t length)
{
	int64_t mqa_timeout;
	SaAisErrorT rc = SA_AIS_OK;
	MQSV_DSEND_EVT *out_evt = NULL;
	uint32_t mds_rc;

	TRACE_ENTER();

	/* convert the timeout to 10 ms value and add it to the sync send timeout */
	mqa_timeout = m_MQSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout);

	mds_rc = mqa_mds_msg_sync_reply_direct(mqa_cb->mqa_mds_hdl, mqnd_mds_dest, qsend_evt,
					       (uint32_t)mqa_timeout, context, length);
	switch (mds_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_2("ERR_TIMEOUT: Message Send through MDS Timeout %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	case NCSCC_RC_FAILURE:
		TRACE_2("ERR_TRY_AGAIN: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NOT_EXIST;
		break;
	default:
		TRACE_4("ERR_RESOURCES: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

 done:
	if (out_evt)
		m_MMGR_FREE_MQA_EVT(out_evt);

	TRACE_LEAVE2(" return code %d ", rc);
	return rc;
}

/****************************************************************************
  Name          : mqa_reply_to_destination_async
 
  Description   : This routine replies to a received message, the receive message 
                  info being passed as  argument to the function.
 
  Arguments     : MQA_CB *mqa_cb - MQA control block
                  MDS_DEST *mqnd_mds_dest - destination MQND to reply to
                  MQSV_DSEND_EVT *qsend_evt - the reply event structure.
                  MDS_SYNC_SND_CTXT *context - MDS context to reply to.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT mqa_reply_to_destination_async(MQA_CB *mqa_cb, MDS_DEST *mqnd_mds_dest,
					   MQSV_DSEND_EVT *qsend_evt, MDS_SYNC_SND_CTXT *context, uint32_t length)
{
	TRACE_ENTER();
	if (mqa_mds_msg_async_reply_direct(mqa_cb->mqa_mds_hdl, mqnd_mds_dest, qsend_evt, context, length) !=
	    NCSCC_RC_SUCCESS) {
		TRACE_2("ERR_EXIST: Message Send through MDS Failure %" PRIx64, mqa_cb->mqa_mds_dest);
		return SA_AIS_ERR_NOT_EXIST;
	}

	TRACE_LEAVE();
	return SA_AIS_OK;
}

/****************************************************************************
  Name          : mqa_reply_message
 
  Description   : This routine replies to a received message, the receive message 
                  info being passed as  argument to the function.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle

                  const SaMsgMessageT *message - The reply message.
                  const SaMsgMessageInfoT *messageInfo - received message
                                                                     information.
                  SaMsgAckFlagsT ackFlags - acknowledgement required/not
                  MQA_SEND_MESSAGE_PARAM *param - points to invocation for 
                                                   async call, points to timeout for
                                                   sync call.
                  MQA_CB *mqa_cb - MQA control block

  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT mqa_reply_message(SaMsgHandleT msgHandle,
			      const SaMsgMessageT *message,
			      SaMsgSenderIdT *senderId,
			      SaMsgAckFlagsT ackFlags, MQA_SEND_MESSAGE_PARAM *param, MQA_CB *mqa_cb)
{

	MQA_CLIENT_INFO *client_info = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	MQSV_DSEND_EVT *qreply_evt = NULL;
	MDS_DEST destination_mqa;
	MQA_SENDERID_INFO *sender_info;
	MDS_SYNC_SND_CTXT destination_context;
	QUEUE_MESSAGE_INFO reply_info;
	SaSizeT dest_rcv_buff_size;
	time_t tmp_time;
	uint32_t length, destination_mqa_ver, o_msg_fmt_ver;

	TRACE_ENTER2(" SaMsgHandle %llu ", msgHandle);

	if (!message || !senderId || !(*senderId)) {
		TRACE_2("ERR_INVALID_PARAM: message is NULL or senderId is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		rc = SA_AIS_ERR_LIBRARY;
		return rc;
	}

	/* Check if mqd, mqnd are up */
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);

	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* check to see if the grant callback was registered */
	if ((param->async_flag) &&
	    (ackFlags & SA_MSG_MESSAGE_DELIVERED_ACK) && (!client_info->msgCallbacks.saMsgMessageDeliveredCallback)) {
		TRACE_2("ERR_INIT: Delivered Callback is not defined after mentioning the async send");
		rc = SA_AIS_ERR_INIT;
		goto done;
	}

	/* EXtract the sender's mds context and the mds dest */
	sender_info = (MQA_SENDERID_INFO *)NCS_INT64_TO_PTR_CAST(*senderId);

	if (!sender_info) {
		TRACE_2("ERR_EXIST: Sender_info is NULL");
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

	/* Dequeue the node from the sender id queue. */
	if (!ncs_remove_item(&(mqa_cb->mqa_senderid_list), (void *)sender_info, mqa_match_senderid)) {
		TRACE_2("ERR_EXIST: Sender info does not exist in the queue");
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

	destination_mqa = sender_info->sender_context.sender_dest;
	destination_context = sender_info->sender_context.context;
	destination_mqa_ver = sender_info->sender_context.src_dest_version;
	dest_rcv_buff_size = sender_info->sender_context.reply_buffer_size;

	m_MMGR_FREE_MQA_SENDERID(sender_info);

	/*Get the message Frmt version to reply */
	o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(destination_mqa_ver,
					      MQA_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT,
					      MQA_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT, mqa_mqa_msg_fmt_table);
	if (!o_msg_fmt_ver) {
		/* Drop The Message */
		TRACE_2("Message Format version Invalid %u", o_msg_fmt_ver);
		goto done;
	}

	memset(&reply_info, 0, sizeof(QUEUE_MESSAGE_INFO));
	reply_info.sendReceive = SA_FALSE;
	m_NCS_OS_GET_TIME_STAMP(tmp_time);
	reply_info.sendTime = (SaTimeT)tmp_time;

	reply_info.sender.senderId = 0;

	/* Allocate memory for the MQSV_DSEND_EVT structure + data */
	length = sizeof(MQSV_DSEND_EVT) + message->size;

	qreply_evt = (MQSV_DSEND_EVT *)mds_alloc_direct_buff(length);
	if (!qreply_evt) {
		TRACE_4("ERR_MEMORY: MQSV_DSEND_EVT Memory allocation failed");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	memset(qreply_evt, 0, sizeof(MQSV_DSEND_EVT));
	qreply_evt->evt_type = MQSV_DSEND_EVENT;
	qreply_evt->endianness = machineEndianness();
	qreply_evt->msg_fmt_version = o_msg_fmt_ver;
	qreply_evt->src_dest_version = MQA_PVT_SUBPART_VERSION;

	if (param->async_flag == false) {
		qreply_evt->type.req_type = MQP_EVT_REPLY_MSG;
		qreply_evt->agent_mds_dest = mqa_cb->mqa_mds_dest;

		qreply_evt->info.replyMsg.ackFlags = ackFlags;
		qreply_evt->info.replyMsg.message.type = message->type;
		qreply_evt->info.replyMsg.message.version = message->version;
		qreply_evt->info.replyMsg.message.size = message->size;
		qreply_evt->info.replyMsg.message.priority = message->priority;

		if (message->senderName)
			qreply_evt->info.replyMsg.message.senderName = *message->senderName;

		if (message->data)
			memcpy(qreply_evt->info.replyMsg.message.data, message->data, message->size);

		qreply_evt->info.replyMsg.msgHandle = msgHandle;
		memcpy(&(qreply_evt->info.replyMsg.messageInfo), &reply_info, sizeof(reply_info));

		rc = mqa_reply_to_destination(mqa_cb, &destination_mqa, qreply_evt,
					      ackFlags, param->info.timeout, &destination_context, length);

		if (rc != SA_AIS_OK)
			goto done;
	} else {
		qreply_evt->type.req_type = MQP_EVT_REPLY_MSG_ASYNC;
		qreply_evt->agent_mds_dest = mqa_cb->mqa_mds_dest;

		qreply_evt->info.replyAsyncMsg.reply.ackFlags = ackFlags;
		qreply_evt->info.replyAsyncMsg.invocation = param->info.invocation;

		qreply_evt->info.replyAsyncMsg.reply.message.type = message->type;
		qreply_evt->info.replyAsyncMsg.reply.message.version = message->version;
		qreply_evt->info.replyAsyncMsg.reply.message.size = message->size;
		qreply_evt->info.replyAsyncMsg.reply.message.priority = message->priority;

		if (message->senderName)
			qreply_evt->info.replyAsyncMsg.reply.message.senderName = *message->senderName;

		if (message->data)
			memcpy(qreply_evt->info.replyAsyncMsg.reply.message.data, message->data, message->size);

		qreply_evt->info.replyAsyncMsg.reply.msgHandle = msgHandle;
		memcpy(&(qreply_evt->info.replyAsyncMsg.reply.messageInfo), &reply_info, sizeof(reply_info));

		rc = mqa_reply_to_destination_async(mqa_cb, &destination_mqa, qreply_evt, &destination_context, length);

		if (rc != SA_AIS_OK)
			goto done;
	}

	/* This check should be done after the reply has been sent to the Sender App so that even SendReceive will
	   also be able to return SA_AIS_ERR_NO_SPACE if the allocated reply buffer size < actual size. If this check
	   had been done before sending the reply, Reply/ReplyAsync would have returned SA_AIS_ERR_NO_SPACE and
	   SendReceive would have returned SA_AIS_ERR_TIMEOUT which is not as per SAF standards */

	if ((param->async_flag == false) && (dest_rcv_buff_size != 0) && (message->size > dest_rcv_buff_size)) {
		TRACE_2("ERR_NO_SPACE: There is no space synchronization in reply and sendrecive calls");
		rc = SA_AIS_ERR_NO_SPACE;
		goto done;
	}

 done:
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE2(" return code %d ", rc);
	return rc;

}

/****************************************************************************
  Name          : saMsgMessageReply
 
  Description   : This routine replies to a received message, the receive message 
                  info being passed as  argument to the function.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle

                       const SaMsgMessageT *replyMessage - The reply message.
                       const SaMsgMessageInfoT *receiveMessageInfo - received message
                                                                     information.
                       SaMsgAckFlagsT ackFlags - acknowledgement required/not
                        SaTimeT timeout - time to wait for acknowledgement in ms.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT
saMsgMessageReply(SaMsgHandleT msgHandle,
		  const SaMsgMessageT *replyMessage, const SaMsgSenderIdT *senderId, SaTimeT timeout)
{

	MQA_CB *mqa_cb;
	MQA_SEND_MESSAGE_PARAM param;
	SaAisErrorT rc;
	SaMsgAckFlagsT ackFlags = SA_MSG_MESSAGE_DELIVERED_ACK;
	MQA_CLIENT_INFO *client_info = NULL;

	TRACE_ENTER2(" SaMsgHandle %llu ", msgHandle);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);

	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}
	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_MQSV_MQA_GIVEUP_MQA_CB;
			return rc;
		}
	}

	if (m_NCS_SA_IS_VALID_TIME_DURATION(timeout) == false) {
		TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if (m_MQSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout) < NCS_SAF_MIN_ACCEPT_TIME) {
		TRACE_2("ERR_TIMEOUT: Invalid Parameter as input");
		rc = SA_AIS_ERR_TIMEOUT;
		goto done;
	}

	param.async_flag = false;
	param.info.timeout = timeout;

	rc = mqa_reply_message(msgHandle, replyMessage, (SaMsgSenderIdT *)senderId, ackFlags, &param, mqa_cb);

 done:
	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE(" Success ");
	else
		TRACE(" Failure with return code %d", rc);
	return rc;

}

/****************************************************************************
  Name          : saMsgMessageReplyAsync
 
  Description   : This routine replies to a received message, the receive message 
                  info being passed as  argument to the function. It does not guarentee
                  the delivery of message to the receiver. A callback will be called
                  later to notify about the delivery status.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                       SaInvocationT invocation - Invocation for the async call.
                       const SaMsgMessageT *replyMessage - The reply message.
                       const SaMsgMessageInfoT *receiveMessageInfo - received message
                                                                     information.
                       SaMsgAckFlagsT ackFlags - acknowledgement required/not
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT
saMsgMessageReplyAsync(SaMsgHandleT msgHandle,
		       SaInvocationT invocation,
		       const SaMsgMessageT *replyMessage, const SaMsgSenderIdT *senderId, SaMsgAckFlagsT ackFlags)
{
	MQA_CB *mqa_cb;
	MQA_SEND_MESSAGE_PARAM param;
	SaAisErrorT rc;
	MQP_ASYNC_RSP_MSG mqp_async_rsp;
	MQA_CLIENT_INFO *client_info = NULL;

	TRACE_ENTER2(" Handle %llu ", msgHandle);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);

	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}
	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_MQSV_MQA_GIVEUP_MQA_CB;
			return rc;
		}
	}

	param.async_flag = true;
	param.info.invocation = invocation;

	if (m_MQSV_IS_ACKFLAGS_NOT_VALID(ackFlags)) {
		TRACE_2("ERR_BAD_FLAGS: Invalid Parameter as input");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return SA_AIS_ERR_BAD_FLAGS;
	}
	if (ackFlags & SA_MSG_MESSAGE_DELIVERED_ACK) {
		mqp_async_rsp.next = NULL;
		mqp_async_rsp.callbackType = MQP_ASYNC_RSP_MSGDELIVERED;
		mqp_async_rsp.messageHandle = msgHandle;
		mqp_async_rsp.params.msgDelivered.error = SA_AIS_ERR_TIMEOUT;
		mqp_async_rsp.params.msgDelivered.invocation = invocation;
		if (mqa_create_and_start_timer(&mqp_async_rsp, invocation) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_RESOURCES: Create and Start Tmr Failed");
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}
	}

	rc = mqa_reply_message(msgHandle, replyMessage, (SaMsgSenderIdT *)senderId, ackFlags, &param, mqa_cb);

	if ((rc != SA_AIS_OK) && (ackFlags & SA_MSG_MESSAGE_DELIVERED_ACK)) {
		/* Stop the timer for this async call */
		mqa_stop_and_delete_timer_by_invocation(NCS_INT64_TO_PTR_CAST(invocation));
	}

 done:
	/* return MQA CB */
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;
}

/****************************************************************************
  Name          : saMsgQueueGroupCreate
 
  Description   : This routine creates a queue group 
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                  const SaNameT *queueGroupName - Name of group to be created.
                  SaMsgQueueGroupPolicyT - policy of group. Right now
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT
saMsgQueueGroupCreate(SaMsgHandleT msgHandle, const SaNameT *queueGroupName, SaMsgQueueGroupPolicyT queueGroupPolicy)
{
	ASAPi_OPR_INFO asapi_or;
	MQA_CB *mqa_cb;
	MQA_CLIENT_INFO *client_info;
	SaAisErrorT rc;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	if (!queueGroupName) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (queueGroupName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName exceeds character 256");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* To memset the bytes to zero other than the length bytes in the SaNameT Structure */
	m_MQSV_SET_SANAMET(queueGroupName);
	TRACE_1("queueGroupName %s", queueGroupName->value);

	if (queueGroupName->length == 0) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName length is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (strncmp((char *)queueGroupName->value, "safMqg=", 7)) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName should starts with safMqg=");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if ((queueGroupPolicy < SA_MSG_QUEUE_GROUP_ROUND_ROBIN) || (queueGroupPolicy > SA_MSG_QUEUE_GROUP_BROADCAST)) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupPolicy not supported");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if ((queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN) &&
	    (queueGroupPolicy != SA_MSG_QUEUE_GROUP_BROADCAST) &&
	    (queueGroupPolicy != SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN)) {
		TRACE_2("ERR_NOT_SUPPORTED: supported queueGroupPolicy (ROUND_ROBIN,BROADCAST,LOCAL_ROUND_ROBIN)");
		return SA_AIS_ERR_NOT_SUPPORTED;
	}

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	memset(&asapi_or, 0, sizeof(asapi_or));
	/* Check if mqd, mqnd are up */
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			goto done;
		}
	}

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	asapi_or.type = ASAPi_OPR_MSG;
	asapi_or.info.msg.opr = ASAPi_MSG_SEND;

	asapi_or.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	asapi_or.info.msg.sinfo.dest = mqa_cb->mqd_mds_dest;
	asapi_or.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

	asapi_or.info.msg.req.msgtype = ASAPi_MSG_REG;
	asapi_or.info.msg.req.info.reg.objtype = ASAPi_OBJ_GROUP;
	asapi_or.info.msg.req.info.reg.group = *queueGroupName;
	asapi_or.info.msg.req.info.reg.policy = queueGroupPolicy;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != SA_AIS_OK) {
		TRACE_2("The ASAPi Registration Req Failed");

	} else {
		if (asapi_or.info.msg.resp) {
			if (asapi_or.info.msg.resp->info.rresp.err.flag) {
				rc = asapi_or.info.msg.resp->info.rresp.err.errcode;
				goto done;
			} else {
				rc = SA_AIS_OK;
			}
		} else {
			TRACE_2("ERR_TRY_AGAIN: ASAPi Response message is NULL");
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
	}

 done:
	if (asapi_or.info.msg.resp)
		asapi_msg_free(&asapi_or.info.msg.resp);

	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;
}

/****************************************************************************
  Name          : saMsgQueueGroupDelete
 
  Description   : This routine deletes a queue group  from the cluster
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                  const SaNameT *queueGroupName - Name of group to be deleted.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgQueueGroupDelete(SaMsgHandleT msgHandle, const SaNameT *queueGroupName)
{
	ASAPi_OPR_INFO asapi_or;
	MQA_CB *mqa_cb;
	MQA_CLIENT_INFO *client_info;
	SaAisErrorT rc;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	if (!queueGroupName) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (queueGroupName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName exceeds character 256");
		return SA_AIS_ERR_INVALID_PARAM;
	}
	/* To memset the bytes to zero other than the length bytes in the SaNameT Structure */
	m_MQSV_SET_SANAMET(queueGroupName);
	TRACE_1("queueGroupName %s", queueGroupName->value);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	memset(&asapi_or, 0, sizeof(asapi_or));

	/* Check if mqd, mqnd are up */
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			goto done;
		}
	}

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	asapi_or.type = ASAPi_OPR_MSG;
	asapi_or.info.msg.opr = ASAPi_MSG_SEND;

	asapi_or.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	asapi_or.info.msg.sinfo.dest = mqa_cb->mqd_mds_dest;
	asapi_or.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

	asapi_or.info.msg.req.msgtype = ASAPi_MSG_DEREG;
	asapi_or.info.msg.req.info.dereg.objtype = ASAPi_OBJ_GROUP;
	asapi_or.info.msg.req.info.dereg.group = *queueGroupName;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != SA_AIS_OK) {
		TRACE_2("The ASAPi Deregistration Req Failed");

	} else {
		if (asapi_or.info.msg.resp) {
			if (asapi_or.info.msg.resp->info.dresp.err.flag) {
				rc = asapi_or.info.msg.resp->info.dresp.err.errcode;
				goto done;
			} else {
				rc = SA_AIS_OK;
			}
		} else {
			TRACE_2("ERR_TRY_AGAIN: ASAPi Response message is NULL");
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
	}

 done:

	if (asapi_or.info.msg.resp)
		asapi_msg_free(&asapi_or.info.msg.resp);

	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;
}

/****************************************************************************
  Name          : saMsgQueueGroupInsert

  Description   : This routine inserts a queue into a queue group.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                  const SaNameT *queueGroupName - Name of group.
                  const SaNameT *queueName - Name of queue to be inserted 
                                            into the group..
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgQueueGroupInsert(SaMsgHandleT msgHandle, const SaNameT *queueGroupName, const SaNameT *queueName)
{
	ASAPi_OPR_INFO asapi_or;
	MQA_CB *mqa_cb;
	MQA_CLIENT_INFO *client_info;
	SaAisErrorT rc;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	if ((!queueGroupName) || (!queueName)) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName is NULL or queueName is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (queueName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueName exceeds character 256");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (queueGroupName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName exceeds character 256");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* To memset the bytes to zero other than the length bytes in the SaNameT Structure */
	m_MQSV_SET_SANAMET(queueGroupName);

	m_MQSV_SET_SANAMET(queueName);
	TRACE_1("queueGroupName %s queueName %s", queueGroupName->value, queueName->value);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	memset(&asapi_or, 0, sizeof(asapi_or));

	/* Check if mqd, mqnd are up */
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			goto done;
		}
	}

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	asapi_or.type = ASAPi_OPR_MSG;
	asapi_or.info.msg.opr = ASAPi_MSG_SEND;

	asapi_or.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	asapi_or.info.msg.sinfo.dest = mqa_cb->mqd_mds_dest;
	asapi_or.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

	asapi_or.info.msg.req.msgtype = ASAPi_MSG_REG;
	asapi_or.info.msg.req.info.reg.objtype = ASAPi_OBJ_BOTH;
	asapi_or.info.msg.req.info.reg.group = *queueGroupName;
	asapi_or.info.msg.req.info.reg.queue.name = *queueName;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != SA_AIS_OK) {
		TRACE_2("The ASAPi Group Insert Req Failed");
	} else {
		if (asapi_or.info.msg.resp) {
			if (asapi_or.info.msg.resp->info.rresp.err.flag) {
				rc = asapi_or.info.msg.resp->info.rresp.err.errcode;
				goto done;
			} else {
				rc = SA_AIS_OK;
			}
		} else {
			TRACE_2("ERR_TRY_AGAIN: ASAPi Response message is NULL");
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
	}
 done:
	if (asapi_or.info.msg.resp)
		asapi_msg_free(&asapi_or.info.msg.resp);

	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;
}

/****************************************************************************
  Name          : saMsgQueueGroupRemove
 
  Description   : This routine removes a queue from a queue group.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                  const SaNameT *queueGroupName - Name of group.
                  const SaNameT *queueName - Name of queue to be removed 
                                            into the group..
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgQueueGroupRemove(SaMsgHandleT msgHandle, const SaNameT *queueGroupName, const SaNameT *queueName)
{
	ASAPi_OPR_INFO asapi_or;
	MQA_CB *mqa_cb;
	MQA_CLIENT_INFO *client_info;
	SaAisErrorT rc;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	if ((!queueGroupName) || (!queueName)) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName is NULL queueName is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (queueName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueName exceeds character 256");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (queueGroupName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName exceeds character 256");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* To memset the bytes to zero other than the length bytes in the SaNameT Structure */
	m_MQSV_SET_SANAMET(queueGroupName);

	m_MQSV_SET_SANAMET(queueName);
	TRACE_1("queueGroupName %s queueName %s", queueGroupName->value, queueName->value);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	memset(&asapi_or, 0, sizeof(asapi_or));

	/* Check if mqd, mqnd are up */
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			goto done;
		}
	}

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	asapi_or.type = ASAPi_OPR_MSG;
	asapi_or.info.msg.opr = ASAPi_MSG_SEND;

	asapi_or.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	asapi_or.info.msg.sinfo.dest = mqa_cb->mqd_mds_dest;
	asapi_or.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

	asapi_or.info.msg.req.msgtype = ASAPi_MSG_DEREG;
	asapi_or.info.msg.req.info.dereg.objtype = ASAPi_OBJ_BOTH;
	asapi_or.info.msg.req.info.dereg.group = *queueGroupName;
	asapi_or.info.msg.req.info.dereg.queue = *queueName;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != SA_AIS_OK) {
		TRACE_2("The ASAPi Group Remove Req Failed");

	} else {
		if (asapi_or.info.msg.resp) {
			if (asapi_or.info.msg.resp->info.dresp.err.flag) {
				rc = asapi_or.info.msg.resp->info.dresp.err.errcode;
				goto done;
			} else {
				rc = SA_AIS_OK;
			}
		} else {
			TRACE_2("ERR_TRY_AGAIN: ASAPi Response message is NULL");
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
	}

 done:
	if (asapi_or.info.msg.resp)
		asapi_msg_free(&asapi_or.info.msg.resp);

	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;
}

/****************************************************************************
  Name          : saMsgQueueGroupTrack
 
  Description   : This routine starts tracking changes to a queue group.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                  const SaNameT *queueGroupName - Name of group.
                  SaMsgQueueGroupNotificationBufferT *notificationBuffer
                           - Notification buffer to return members in the 
                             callback. This memory must not be freed by the 
                             caller until track is stopped.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT
saMsgQueueGroupTrack(SaMsgHandleT msgHandle,
		     const SaNameT *queueGroupName,
		     SaUint8T trackFlags, SaMsgQueueGroupNotificationBufferT *notificationBuffer)
{

	ASAPi_OPR_INFO asapi_or;
	MQA_CB *mqa_cb;
	MQA_CLIENT_INFO *client_info;
	MQA_TRACK_INFO *track_info;
	SaAisErrorT rc;
	uint32_t num_queues;
	SaMsgQueueGroupNotificationBufferT temp_notificationBuffer;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);
	if (!queueGroupName) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (queueGroupName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName exceeds character 256");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* To memset the bytes to zero other than the length bytes in the SaNameT Structure */
	m_MQSV_SET_SANAMET(queueGroupName);
	TRACE_1("queueGroupName %s", queueGroupName->value);

	if (!(trackFlags & (SA_TRACK_CURRENT | SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY))) {
		TRACE_2("ERR_BAD_FLAGS: Invalid Track Flags");
		return SA_AIS_ERR_BAD_FLAGS;
	}

	if ((trackFlags & SA_TRACK_CHANGES) && (trackFlags & SA_TRACK_CHANGES_ONLY)) {
		TRACE_2("ERR_BAD_FLAGS: Invalid Track Flags");
		return SA_AIS_ERR_BAD_FLAGS;
	}

	if ((trackFlags & SA_TRACK_CURRENT) && (notificationBuffer) &&
	    (notificationBuffer->numberOfItems == 0) && (notificationBuffer->notification)) {
		TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	/* Check if mqd, mqnd are up */
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			goto done;
		}

		/* Retruning ERR_INIT if notification callback isn't registered
		   and user has given option to track */
		if (!((trackFlags & SA_TRACK_CURRENT) && (notificationBuffer)) &&
		    (!client_info->msgCallbacks.saMsgQueueGroupTrackCallback)) {
			TRACE_2("ERR_INIT: notification callback isn't registered");
			rc = SA_AIS_ERR_INIT;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			goto done;
		}
	} else {
		if (!client_info->msgCallbacks.saMsgQueueGroupTrackCallback) {
			TRACE_2("ERR_INIT: Client Database Find Failed");
			rc = SA_AIS_ERR_INIT;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			goto done;
		}
	}
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	memset(&asapi_or, 0, sizeof(asapi_or));
	asapi_or.type = ASAPi_OPR_GET_DEST;
	asapi_or.info.dest.i_object = *queueGroupName;
	m_ASAPi_TRACK_ENABLE_SET(asapi_or.info.dest.i_track);
	asapi_or.info.dest.i_sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	asapi_or.info.dest.i_sinfo.dest = mqa_cb->mqd_mds_dest;
	asapi_or.info.dest.i_sinfo.stype = MDS_SENDTYPE_SNDRSP;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != SA_AIS_OK) {
		TRACE_2("The ASAPi Get Dest Operation Failed");
		goto done;
	}

	if ((!asapi_or.info.dest.o_cache) || (asapi_or.info.dest.o_cache->objtype == ASAPi_OBJ_QUEUE)) {
		TRACE_2("ERR_EXIST: The ASAPi Get Dest Operation's result Cache does not exist");
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

	num_queues = asapi_or.info.dest.o_cache->info.ginfo.qlist.count;

	if ((trackFlags & SA_TRACK_CURRENT) && (notificationBuffer)
	    && (notificationBuffer->notification) && (notificationBuffer->numberOfItems < num_queues)) {
		notificationBuffer->numberOfItems = num_queues;
		TRACE_2("ERR_NO_SPACE: Buffer is too small to hold information about"
 					"all members in the message queue group identified");
		rc = SA_AIS_ERR_NO_SPACE;
		goto done;
	}

	memset(&temp_notificationBuffer, 0, sizeof(SaMsgQueueGroupNotificationBufferT));

	if (num_queues != 0) {
		temp_notificationBuffer.notification = m_MMGR_ALLOC_MQA_TRACK_BUFFER_INFO((uint32_t)num_queues);

		if (!temp_notificationBuffer.notification) {
			TRACE_2("ERR_MEMORY: Track Buffer Info Allocation Failed");
			return SA_AIS_ERR_NO_MEMORY;
		}

		memset(temp_notificationBuffer.notification, 0, num_queues * sizeof(SaMsgQueueGroupNotificationT));
	} else
		temp_notificationBuffer.notification = NULL;

	temp_notificationBuffer.numberOfItems = num_queues;

	memset(&asapi_or, 0, sizeof(asapi_or));
	asapi_or.type = ASAPi_OPR_TRACK;
	asapi_or.info.track.i_group = *queueGroupName;
	asapi_or.info.track.i_flags = trackFlags;
	asapi_or.info.track.i_option = ASAPi_TRACK_ENABLE;

	asapi_or.info.track.i_sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	asapi_or.info.track.i_sinfo.dest = mqa_cb->mqd_mds_dest;
	asapi_or.info.track.i_sinfo.stype = MDS_SENDTYPE_SNDRSP;

	asapi_or.info.track.o_ginfo.notification_buffer = temp_notificationBuffer;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != SA_AIS_OK) {
		TRACE_2("The ASAPi Track Req Failed");
		goto done;
	}

	if (trackFlags & SA_TRACK_CURRENT) {
		if (!notificationBuffer) {
			MQP_ASYNC_RSP_MSG *track_current_callback;

			track_current_callback = m_MMGR_ALLOC_MQP_ASYNC_RSP_MSG;
			if (!track_current_callback) {
				TRACE_4("ERR_MEMORY: MQP Async Rsp Message Allocation Failed");
				rc = SA_AIS_ERR_NO_MEMORY;
				goto done;
			}

			track_current_callback->callbackType = MQP_ASYNC_RSP_GRP_TRACK;
			track_current_callback->messageHandle = msgHandle;
			track_current_callback->params.qGrpTrack.queueGroupName = *queueGroupName;
			track_current_callback->params.qGrpTrack.notificationBuffer.numberOfItems =
			    temp_notificationBuffer.numberOfItems;
			if (num_queues != 0) {
				track_current_callback->params.qGrpTrack.notificationBuffer.notification =
				    m_MMGR_ALLOC_MQA_TRACK_BUFFER_INFO((uint32_t)num_queues);

				if (!track_current_callback->params.qGrpTrack.notificationBuffer.notification) {
					TRACE_4("ERR_MEMORY: Track Buffer Info Allocation Failed");
					m_MMGR_FREE_MQP_ASYNC_RSP_MSG(track_current_callback);
					rc = SA_AIS_ERR_NO_MEMORY;
					goto done;
				}

				memcpy(track_current_callback->params.qGrpTrack.notificationBuffer.notification,
				       temp_notificationBuffer.notification,
				       temp_notificationBuffer.numberOfItems * sizeof(SaMsgQueueGroupNotificationT));
			} else
				track_current_callback->params.qGrpTrack.notificationBuffer.notification = NULL;

			track_current_callback->params.qGrpTrack.queueGroupPolicy = asapi_or.info.track.o_ginfo.policy;
			track_current_callback->params.qGrpTrack.notificationBuffer.queueGroupPolicy =
			    asapi_or.info.track.o_ginfo.policy;
			track_current_callback->params.qGrpTrack.numberOfMembers = asapi_or.info.track.o_ginfo.qcnt;
			track_current_callback->params.qGrpTrack.error = SA_AIS_OK;

			if (mqsv_mqa_callback_queue_write(mqa_cb, msgHandle,
							  track_current_callback) != NCSCC_RC_SUCCESS) {
				TRACE_4("ERR_MEMORY: Call back Queue Write Failed");
				rc = SA_AIS_ERR_LIBRARY;
				goto done;
			}
		} /* if (!notificationBuffer) */
		else {
			/* Notification Buffer provided */
			if (!notificationBuffer->notification) {
				if (num_queues != 0) {
					/* Allocate & Copy the notification */
					notificationBuffer->notification =
					    (SaMsgQueueGroupNotificationT *)malloc((uint32_t)num_queues *
										   sizeof
										   (SaMsgQueueGroupNotificationT));

					if (!notificationBuffer->notification) {
						TRACE_4("ERR_MEMORY: SaMsgQueueGroupNotificationT Memory Allocation Failed");
						rc = SA_AIS_ERR_NO_MEMORY;
						goto done;
					}

					memset(notificationBuffer->notification, 0,
					       num_queues * sizeof(SaMsgQueueGroupNotificationT));
				} else
					notificationBuffer->notification = NULL;
			}
			notificationBuffer->queueGroupPolicy = asapi_or.info.track.o_ginfo.policy;
			notificationBuffer->numberOfItems = temp_notificationBuffer.numberOfItems;
			memcpy(notificationBuffer->notification, temp_notificationBuffer.notification,
			       temp_notificationBuffer.numberOfItems * sizeof(SaMsgQueueGroupNotificationT));
		}
	}

	if ((trackFlags & SA_TRACK_CHANGES) || (trackFlags & SA_TRACK_CHANGES_ONLY)) {
		/* get the client_info */
		if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed for control block write");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}

		track_info = mqa_track_tree_find_and_add(client_info, (SaNameT *)queueGroupName, true);
		if (!track_info) {
			TRACE_2("ERR_NO_SPACE: Track Database Registration Failed");
			rc = SA_AIS_ERR_NO_SPACE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			goto done;
		}

		if (track_info->trackFlags != 0)
			m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(track_info->notificationBuffer.notification);

		track_info->notificationBuffer = temp_notificationBuffer;
		track_info->trackFlags = trackFlags;
		track_info->track_index = num_queues;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
	} else {
		/* Free temp_notificationBuffer.notification TBD */
		if (temp_notificationBuffer.notification)
			m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(temp_notificationBuffer.notification);
	}

	rc = SA_AIS_OK;

 done:
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;
}

/****************************************************************************
  Name          : saMsgQueueGroupTrackStop
 
  Description   : This routine stops tracking changes to a queue group.
 
  Arguments     : SaMsgHandleT msgHandle - The message handle
                  const SaNameT *queueGroupName - Name of group.
 
  Return Values : SaAisErrorT
 
  Notes         : None
******************************************************************************/

SaAisErrorT saMsgQueueGroupTrackStop(SaMsgHandleT msgHandle, const SaNameT *queueGroupName)
{
	ASAPi_OPR_INFO asapi_or;
	MQA_CB *mqa_cb;
	MQA_CLIENT_INFO *client_info = NULL;
	MQA_TRACK_INFO *track_info = NULL;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	if (!queueGroupName) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (queueGroupName->length > SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: queueGroupName exceeds character 256");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* To memset the bytes to zero other than the length bytes in the SaNameT Structure */
	m_MQSV_SET_SANAMET(queueGroupName);
	TRACE_1("queueGroupName %s", queueGroupName->value);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	/* Check if mqd, mqnd are up */
	if (!mqa_cb->is_mqd_up || !mqa_cb->is_mqnd_up) {
		TRACE_2("ERR_TRY_AGAIN: MQD or MQND is down");
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done1;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done1;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			goto done1;
		}
	}

	track_info = mqa_track_tree_find_and_add(client_info, (SaNameT *)queueGroupName, false);

	if (!track_info) {
		TRACE_2("ERR_EXIST: Failed to find the track tree node");
		rc = SA_AIS_ERR_NOT_EXIST;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done1;
	}

	/* We must send track stop to MQD if this is the last track 
	 * client for that group. So search for any other clients in
	 * the track tree
	 */
	if (mqa_is_track_enabled(mqa_cb, (SaNameT *)queueGroupName) == true) {
		rc = SA_AIS_OK;
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		goto done;
	}

	memset(&asapi_or, 0, sizeof(asapi_or));
	asapi_or.info.track.i_sinfo.dest = mqa_cb->mqd_mds_dest;
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	asapi_or.type = ASAPi_OPR_TRACK;
	asapi_or.info.track.i_group = *queueGroupName;
	asapi_or.info.track.i_flags = track_info->trackFlags;
	asapi_or.info.track.i_option = ASAPi_TRACK_DISABLE;

	asapi_or.info.track.i_sinfo.to_svc = NCSMDS_SVC_ID_MQD;
	asapi_or.info.track.i_sinfo.stype = MDS_SENDTYPE_SNDRSP;

	if ((rc = asapi_opr_hdlr(&asapi_or)) != SA_AIS_OK)
		TRACE_2("The ASAPi Track Req Failed");

 done:
	if ((rc == SA_AIS_OK) || !((rc == SA_AIS_ERR_TRY_AGAIN) || (rc == SA_AIS_ERR_TIMEOUT))) {
		if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed for control block write");
			rc = SA_AIS_ERR_LIBRARY;
			goto done1;
		}
		mqa_track_tree_find_and_del(client_info, (SaNameT *)queueGroupName);
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
	}

 done1:
	m_MQSV_MQA_GIVEUP_MQA_CB;

	if (rc == SA_AIS_OK)
		TRACE_LEAVE2(" Success ");
	else
		TRACE_LEAVE2(" Failed with return code %d", rc);
	return rc;
}

/*****************************************************************************
  Name          : saMsgQueueGroupNotificationFree 

  Description   : This routine free memory pointed to notification.

  Arguments     : SaMsgHandleT msgHandle - The message handle
                  SaMsgQueueGroupNotificationT *notificatio - Pointer to notification buffer

  Return Values : SaAisErrorT

  Notes         : None
******************************************************************************/
SaAisErrorT saMsgQueueGroupNotificationFree(SaMsgHandleT msgHandle, SaMsgQueueGroupNotificationT *notification)
{

	MQA_CB *mqa_cb;
	MQA_CLIENT_INFO *client_info = NULL;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}
	/* get the client_info */
	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto done;
		}
	}

	if (notification == NULL) {
		TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	free(notification);
	rc = SA_AIS_OK;

 done:
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
	m_MQSV_MQA_GIVEUP_MQA_CB;

	TRACE_LEAVE2(" return code %d", rc);
	return rc;
}

/*****************************************************************************
  Name          : saMsgMessageDataFree 

  Description   : This routine free memory pointed to notification.

  Arguments     : SaMsgHandleT msgHandle - The message handle
                  void *data - Data pointer to be freed

  Return Values : SaAisErrorT

  Notes         : None
******************************************************************************/
SaAisErrorT saMsgMessageDataFree(SaMsgHandleT msgHandle, void *data)
{
	MQA_CB *mqa_cb;
	MQA_CLIENT_INFO *client_info = NULL;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2(" SaMsgHandle %llu", msgHandle);

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("ERR_BAD_HANDLE: Control block retrieval failed");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	/* get the client_info */
	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (client_info->version.majorVersion == MQA_MAJOR_VERSION) {
		if (!mqa_cb->clm_node_joined) {
			TRACE_2("ERR_UNAVAILABLE: MQD or MQND is down");
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto done;
		}
	}

	if (data == NULL) {
		TRACE_2("ERR_INVALID_PARAM: Invalid Parameter as input");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	free(data);
	rc = SA_AIS_OK;

 done:
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
	m_MQSV_MQA_GIVEUP_MQA_CB;

	TRACE_LEAVE2(" return code %d", rc);
	return rc;
}

/*****************************************************************************
  Name          : msgget_timer_expired 

  Description   : 

  Arguments     : 
                  

  Return Values : SaAisErrorT

  Notes         : None
******************************************************************************/

static void msgget_timer_expired(void *arg)
{
	MQSV_MESSAGE *mqsv_message;
	NCS_OS_POSIX_MQ_REQ_INFO mq_req;
	NCS_OS_MQ_MSG mq_msg;
	uint32_t rc;
	MQP_CANCEL_REQ **cancel_req = (MQP_CANCEL_REQ **)arg;
	TRACE_ENTER();

	mqsv_message = (MQSV_MESSAGE *)mq_msg.data;
	mqsv_message->type = MQP_EVT_CANCEL_REQ;
	mqsv_message->info.cancel_req.queueHandle = (*cancel_req)->queueHandle;
	mqsv_message->info.cancel_req.timerId = (*cancel_req)->timerId;

	memset(&mq_req, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
	mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_SEND_ASYNC;
	mq_req.info.send.i_msg = &mq_msg;
	mq_req.info.send.datalen = sizeof(MQSV_MESSAGE);
	mq_req.info.send.mqd = (*cancel_req)->queueHandle;
	mq_req.info.send.i_mtype = 1;

	if ((rc = m_NCS_OS_POSIX_MQ(&mq_req)) != NCSCC_RC_SUCCESS) {
		TRACE_4("Unable to put the cancel message in the queue");
	}

	m_NCS_TMR_DESTROY((*cancel_req)->timerId);
	m_MMGR_FREE_MQA_CANCEL_REQ(*cancel_req);
	*cancel_req = NULL;

	TRACE_LEAVE();
	return;
}
