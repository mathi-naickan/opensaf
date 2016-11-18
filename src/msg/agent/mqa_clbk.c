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

  This file contains functions that is used for callback dispatch.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "mqa.h"

#include <sched.h>

static void mqa_process_callback(MQA_CB *cb, SaMsgHandleT msgHandle, MQP_ASYNC_RSP_MSG *callback);

static bool mqa_client_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg);
static void mqa_track_update_state(SaMsgQueueGroupNotificationBufferT *buffer,
				   SaNameT *name, SaMsgQueueSendingStateT status);
static uint32_t mqa_notify_changes(MQA_CLIENT_INFO *client_info, MQA_TRACK_INFO *track_info, ASAPi_MSG_INFO *asapi_msg);

static uint32_t mqa_notify_changes_only(MQA_CLIENT_INFO *client_info,
				     MQA_TRACK_INFO *track_info, ASAPi_MSG_INFO *asapi_msg);

static void mqa_track_remove_member(SaMsgQueueGroupNotificationBufferT *buffer,
				    SaNameT *name, SaMsgQueueSendingStateT status);

static uint32_t mqa_notify_clients(ASAPi_MSG_INFO *asapi_msg);
static MQP_ASYNC_RSP_MSG *mqsv_mqa_callback_queue_read(struct mqa_client_info *client_info);
/****************************************************************************
  Name          : mqa_track_remove_member
 
  Description   : This routine removes the queue from the queue group 
                  notification Buffer.
 
  Arguments     : SaMsgQueueGroupNotificationBufferT *buffer,  
                  SaNameT *name - Queue whose status to be updated
                   SaMsgQueueSendingStateT status  - status value
 
  Return Values : None
 
  Notes         : None
******************************************************************************/

static void mqa_track_remove_member(SaMsgQueueGroupNotificationBufferT *buffer,
				    SaNameT *name, SaMsgQueueSendingStateT status)
{

	uint32_t i;
	uint32_t num_items;

	num_items = buffer->numberOfItems;

	for (i = 0; i < num_items; i++) {

		if (buffer->notification[i].member.queueName.length != name->length)
			continue;

		if (memcmp(&buffer->notification[i].member.queueName.value, name->value, name->length) == 0) {
			buffer->notification[i].change = SA_MSG_QUEUE_GROUP_REMOVED;
			/* Status is removed in B.1.1 
			   buffer->notification[i].member.sendingState = status; */
			TRACE("mqa_track_remove_member called - Success");
			return;
		}
	}

}

/****************************************************************************
  Name          : mqa_track_update_state
 
  Description   : This routine updates the queue sending state 
 
  Arguments     : SaMsgQueueGroupNotificationBufferT *buffer,  
                  SaNameT *name - Queue whose status to be updated
                   SaMsgQueueSendingStateT status  - status value
 
  Return Values : None
 
  Notes         : None
******************************************************************************/

void mqa_track_update_state(SaMsgQueueGroupNotificationBufferT *buffer, SaNameT *name, SaMsgQueueSendingStateT status)
{

	uint32_t i;
	uint32_t num_items;

	num_items = buffer->numberOfItems;

	for (i = 0; i < num_items; i++) {

		if (buffer->notification[i].member.queueName.length != name->length)
			continue;

		if (memcmp(&buffer->notification[i].member.queueName.value, name->value, name->length) == 0) {
			/* Status is removed in B.1.1 
			   buffer->notification[i].member.sendingState = status; */
			TRACE("mqa_track_update_state called - Success");
			return;
		}
	}

}

/****************************************************************************
  Name          : mqa_process_callback
 
  Description   : This routine invokes the registered callback routine.
 
  Arguments     : cb  - ptr to the MQA control block
                  rec - ptr to the callback record
                  reg_callbk - ptr to the registered callbacks
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
static void mqa_process_callback(MQA_CB *cb, SaMsgHandleT msgHandle, MQP_ASYNC_RSP_MSG *callback)
{
	MQA_CLIENT_INFO *client_info;

	TRACE_ENTER();

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MMGR_FREE_MQP_ASYNC_RSP_MSG(callback);
		TRACE_4("Lock failed for control block write");
		return;
	}

	client_info = mqa_client_tree_find_and_add(cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("Client Database Find Failed");
		m_MMGR_FREE_MQP_ASYNC_RSP_MSG(callback);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return;
	}

	/* callback and client_info are NULL checked by caller. */

	TRACE("Function called with callbacktype %d", callback->callbackType);
	/* invoke the corresponding callback */
	switch (callback->callbackType) {
	case MQP_ASYNC_RSP_OPEN:
		{
			MQSV_MSGQ_OPEN_PARAM *param = &callback->params.qOpen;
			MQA_QUEUE_INFO *queue_info;
			NCSCONTEXT thread_handle;
			SaAisErrorT rc;

			/* We need to start the reader thread only if the Queue has been successfully opened/created by the MQND */
			if (param->error == SA_AIS_OK) {

				queue_info =
				    mqa_queue_tree_find_and_add(cb, param->queueHandle, true, client_info,
								param->openFlags);

				/* Start a thread to notify when there is a message in the queue.
				 * The thread does it by using  1 byte message buffer to read
				 * from the queue. When it fails, it assumes that there is a message
				 * in the queue.
				 */
				if (queue_info && (queue_info->openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK)) {
					MQP_OPEN_RSP *openRsp;

					/* update queue_info data structure with listenerHandle */
					queue_info->listenerHandle = param->listenerHandle;

					openRsp = m_MMGR_ALLOC_MQA_OPEN_RSP(sizeof(MQP_OPEN_RSP));

					if (!openRsp) {
						TRACE_4("MQP Open Rsp Message Allocatiion Failed");
						mqa_queue_tree_delete_node(cb, param->queueHandle);
						m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
						break;
					}

					openRsp->listenerHandle = param->listenerHandle;
					openRsp->queueHandle = param->queueHandle;
					openRsp->existing_msg_count = param->existing_msg_count;
					
					int policy = SCHED_OTHER; /*root defaults */
					int prio_val = sched_get_priority_min(policy);
	
					rc = m_NCS_TASK_CREATE((NCS_OS_CB)mqa_queue_reader,
							       (NCSCONTEXT)openRsp,
							       (char *)"OSAF_MQA_CLBK", prio_val, policy, NCS_STACKSIZE_HUGE,
							       &thread_handle);
					if (rc != NCSCC_RC_SUCCESS) {
						TRACE_4("ERR_RESOURCES: Queue Reader Thread Task Create Failed");
						param->error = SA_AIS_ERR_NO_RESOURCES;
						mqa_queue_tree_delete_node(cb, param->queueHandle);
					} else {
						rc = m_NCS_TASK_START(thread_handle);
						if (rc != NCSCC_RC_SUCCESS) {
							TRACE_4("ERR_RESOURCES: Queue Reader Thread Task Start Failed");
							m_NCS_TASK_DETACH(thread_handle);
							param->error = SA_AIS_ERR_NO_RESOURCES;
							mqa_queue_tree_delete_node(cb, param->queueHandle);
						} else {
							m_NCS_TASK_DETACH(thread_handle);
							queue_info->task_handle = thread_handle;
							param->error = SA_AIS_OK;
						}
					}
				}
			}

			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

			if (client_info->msgCallbacks.saMsgQueueOpenCallback)
				client_info->msgCallbacks.saMsgQueueOpenCallback(param->invocation,
										 param->queueHandle, param->error);
		}
		break;
	case MQP_ASYNC_RSP_GRP_TRACK:
		{
			MQSV_MSGQGRP_TRACK_PARAM *param = &callback->params.qGrpTrack;

			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

			if (client_info->msgCallbacks.saMsgQueueGroupTrackCallback)
				client_info->msgCallbacks.saMsgQueueGroupTrackCallback(&(param->queueGroupName),
										       &param->notificationBuffer,
										       param->numberOfMembers,
										       param->error);
			if (param->notificationBuffer.notification)
				m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(param->notificationBuffer.notification);

		}
		break;
	case MQP_ASYNC_RSP_MSGDELIVERED:
		{
			MQSV_MSG_DELIVERED_PARAM *param = &callback->params.msgDelivered;

			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

			if (client_info->msgCallbacks.saMsgMessageDeliveredCallback)
				client_info->msgCallbacks.saMsgMessageDeliveredCallback(param->invocation,
											param->error);
		}
		break;

	case MQP_ASYNC_RSP_MSGRECEIVED:
		{
			MQA_QUEUE_INFO *queue_info;
			MQSV_MSG_RECEIVED_PARAM *param = &callback->params.msgReceived;

			queue_info = mqa_queue_tree_find_and_add(cb, param->queueHandle, false, NULL, 0);

			if (!queue_info) {	/* The queue corresponding to the received callback has been deleted, cancel callback */
				TRACE_2("Queue Database Find Failed");
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				break;
			}

			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

			if (client_info->msgCallbacks.saMsgMessageReceivedCallback)
				client_info->msgCallbacks.saMsgMessageReceivedCallback(param->queueHandle);
		}
		break;

	default:
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		break;
	}
	/* free the callback info. This will be allocated by MDS EDU functions */
	if (callback) {
		m_MMGR_FREE_MQP_ASYNC_RSP_MSG(callback);
		callback = NULL;
	}
	TRACE_LEAVE();
}

/****************************************************************************
  Name          : mqa_hdl_callbk_dispatch_one
 
  Description   : This routine dispatches one pending callback.
 
  Arguments     : cb      - ptr to the MQA control block
                  client_info - ptr to the client info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t mqa_hdl_callbk_dispatch_one(MQA_CB *cb, SaMsgHandleT msgHandle)
{
	MQP_ASYNC_RSP_MSG *callback;
	MQA_CLIENT_INFO *client_info;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();

	/* get the client_info */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	client_info = mqa_client_tree_find_and_add(cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get it from the queue */
	callback = mqsv_mqa_callback_queue_read(client_info);
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	if (callback) {
		/* process the callback */
		mqa_process_callback(cb, msgHandle, callback);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : mqa_hdl_callbk_dispatch_all
 
  Description   : This routine dispatches all pending callback.
 
  Arguments     : cb      - ptr to the MQA control block
                  client_info - ptr to the client info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t mqa_hdl_callbk_dispatch_all(MQA_CB *cb, SaMsgHandleT msgHandle)
{
	MQP_ASYNC_RSP_MSG *callback;
	MQA_CLIENT_INFO *client_info;
	TRACE_ENTER();

	/* get the client_info */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	client_info = mqa_client_tree_find_and_add(cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	while ((callback = mqsv_mqa_callback_queue_read(client_info))) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

		mqa_process_callback(cb, msgHandle, callback);

		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed for control block write");
			return SA_AIS_ERR_LIBRARY;
		}

		if ((client_info = mqa_client_tree_find_and_add(cb, msgHandle, false)) == NULL) {
			TRACE_2("Client Database Find Failed");
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			return SA_AIS_OK;
		}
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return SA_AIS_OK;
}

/****************************************************************************
  Name          : mqa_hdl_callbk_dispatch_block
 
  Description   : This routine dispatches all pending callback.
 
  Arguments     : cb      - ptr to the MQA control  block
                  client_info - ptr to the client info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t mqa_hdl_callbk_dispatch_block(MQA_CB *mqa_cb, SaMsgHandleT msgHandle)
{
	MQP_ASYNC_RSP_MSG *callback = 0;
	SYSF_MBX *callbk_mbx;
	MQA_CLIENT_INFO *client_info;
	TRACE("mqa_hdl_callbk_dispatch_block is called");

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed for control block write");
		return SA_AIS_ERR_LIBRARY;
	}

	client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false);
	if (!client_info) {
		TRACE_2("ERR_BAD_HANDLE: Client Database Find Failed");
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	callbk_mbx = &(client_info->callbk_mbx);

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	callback = (MQP_ASYNC_RSP_MSG *)m_NCS_IPC_RECEIVE(callbk_mbx, NULL);

	while (1) {
		if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed for control block write");
			return SA_AIS_ERR_LIBRARY;
		}

		if ((client_info = mqa_client_tree_find_and_add(mqa_cb, msgHandle, false)) == NULL) {
			/* Another thread called Finalize */
			TRACE_1("Client Database Find Failed");
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			return SA_AIS_OK;
		}

		if (callback) {
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			mqa_process_callback(mqa_cb, msgHandle, callback);
		} else {
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			TRACE_1("IPC mailbox is empty");
			return SA_AIS_OK;
		}

		callback = (MQP_ASYNC_RSP_MSG *)m_NCS_IPC_RECEIVE(callbk_mbx, NULL);
		if (!callback) {
			TRACE_1("IPC mailbox is empty");
			return SA_AIS_OK;
		}
	}
}

/****************************************************************************
  Name          : mqa_notify_clients

  Description   : This routine iterates all the msg Handle nodes and notifies
                  the clients who are registered for group track.

  Arguments     : ASAPi_MSG_INFO *asapi_msg - ASAPi response message structure.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/

static uint32_t mqa_notify_clients(ASAPi_MSG_INFO *asapi_msg)
{
	MQA_TRACK_INFO *track_info;
	MQA_CLIENT_INFO *client_info;
	uint8_t *temp_name_ptr = 0;
	SaNameT temp_name;
	SaMsgHandleT *temp_hdl_ptr = 0;
	SaMsgHandleT temp_hdl;
	MQA_CB *mqa_cb;
	uint32_t len;
	TRACE_ENTER();

	/* If this track notification is not for a Queue Group then return */
	if (asapi_msg->info.tntfy.oinfo.group.length == 0) {
		return NCSCC_RC_SUCCESS;
	} else {
		/* In case of queue groups, ignore the update */
		if ((asapi_msg->info.tntfy.opr == ASAPi_QUEUE_UPD) || (asapi_msg->info.tntfy.opr == ASAPi_GROUP_ADD))
			return NCSCC_RC_SUCCESS;
	}
	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("FAILURE: Control block retrieval failed");
		return NCSCC_RC_FAILURE;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("FAILURE: Lock failed for control block write");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return NCSCC_RC_FAILURE;
	}
	while ((client_info =
		(MQA_CLIENT_INFO *)ncs_patricia_tree_getnext(&mqa_cb->mqa_client_tree, (uint8_t *const)temp_hdl_ptr))) {
		temp_hdl = client_info->msgHandle;
		temp_hdl_ptr = &temp_hdl;
		/* scan the entire group track db & delete each record */
		while ((track_info =
			(MQA_TRACK_INFO *)ncs_patricia_tree_getnext(&client_info->mqa_track_tree,
								    (uint8_t *const)temp_name_ptr))) {
			/* delete the track info */
			temp_name = track_info->queueGroupName;
			temp_name_ptr = temp_name.value;
			/* Compare group names */
			len = temp_name.length;
			if (len > asapi_msg->info.tntfy.oinfo.group.length)
				len = asapi_msg->info.tntfy.oinfo.group.length;

			if (memcmp(temp_name.value, asapi_msg->info.tntfy.oinfo.group.value, len) != 0)
				continue;

			if (track_info->trackFlags & SA_TRACK_CHANGES) {
				mqa_notify_changes(client_info, track_info, asapi_msg);
				continue;
			}

			if (track_info->trackFlags & SA_TRACK_CHANGES_ONLY) {
				mqa_notify_changes_only(client_info, track_info, asapi_msg);
				continue;
			}

		}
	}
	m_MQSV_MQA_GIVEUP_MQA_CB;
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : mqa_notify_changes

  Description   : This routine notifies the client
                  who are registered for group changes only.

  Arguments     : MQA_CLIENT_INFO *client_info - client info pointer
                  ASAPi_GROUP_TRACK_INFO * - Group track info pointer
                  ASAPi_MSG_INFO * - ASAP notify message that was received.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/
uint32_t mqa_notify_changes(MQA_CLIENT_INFO *client_info, MQA_TRACK_INFO *track_info, ASAPi_MSG_INFO *asapi_msg)
{
	MQP_ASYNC_RSP_MSG *track_current_callback = NULL;
	SaMsgQueueGroupNotificationT *buffer;
	SaMsgQueueGroupNotificationT *callback_buffer = NULL;
	MQA_CB *mqa_cb;
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint32_t num_items;
	uint32_t track_index;
	uint32_t i;
	uint32_t j = 0;
	TRACE_ENTER();

	if ((asapi_msg->info.tntfy.opr == ASAPi_QUEUE_MQND_DOWN) || (asapi_msg->info.tntfy.opr == ASAPi_QUEUE_MQND_UP)) {
		return NCSCC_RC_SUCCESS;
	}
	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("FAILURE: Control block retrieval failed");
		return NCSCC_RC_FAILURE;
	}

	track_current_callback = m_MMGR_ALLOC_MQP_ASYNC_RSP_MSG;
	if (!track_current_callback) {
		TRACE_4("FAILURE: MQP Async Rsp Message Allocation Failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	memset(track_current_callback, 0, sizeof(MQP_ASYNC_RSP_MSG));
	num_items = track_info->notificationBuffer.numberOfItems;
	track_index = track_info->track_index;

	track_current_callback->callbackType = MQP_ASYNC_RSP_GRP_TRACK;
	track_current_callback->messageHandle = client_info->msgHandle;
	track_current_callback->params.qGrpTrack.queueGroupName = track_info->queueGroupName;
	track_current_callback->params.qGrpTrack.queueGroupPolicy = asapi_msg->info.tntfy.oinfo.policy;
	track_current_callback->params.qGrpTrack.notificationBuffer.queueGroupPolicy =
	    asapi_msg->info.tntfy.oinfo.policy;
	track_current_callback->params.qGrpTrack.numberOfMembers = asapi_msg->info.tntfy.oinfo.qcnt;
	track_current_callback->params.qGrpTrack.error = SA_AIS_OK;

	if (asapi_msg->info.tntfy.opr != ASAPi_GROUP_DEL) {
		if (!(callback_buffer = m_MMGR_ALLOC_MQA_TRACK_BUFFER_INFO((track_index + 1)))) {
			TRACE_4("FAILURE: Track Buffer Info Allocation Failed");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		track_current_callback->params.qGrpTrack.notificationBuffer.notification = callback_buffer;
		if (track_info->notificationBuffer.notification) {
			buffer = track_info->notificationBuffer.notification;
			/* Copy the current queue member i.e without the new change */
			memcpy(callback_buffer, buffer, (track_index) * sizeof(SaMsgQueueGroupNotificationT));
		}
	}

	/* Now fill in the member for which we got notification */
	switch (asapi_msg->info.tntfy.opr) {
	case ASAPi_GROUP_DEL:
		track_current_callback->params.qGrpTrack.error = SA_AIS_ERR_NOT_EXIST;
		track_current_callback->params.qGrpTrack.notificationBuffer.numberOfItems = 0;
		track_current_callback->params.qGrpTrack.notificationBuffer.notification = NULL;

		if (mqa_track_tree_find_and_del(client_info, &track_info->queueGroupName) != true) {
			TRACE_2("FAILURE: Track Tree Find and Delete Failed");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;

	case ASAPi_QUEUE_ADD:
		/* Check if the QUEUE already exists notification list. */
		for (i = 0; i < track_index; i++) {
			SaMsgQueueGroupNotificationBufferT *buff =
			    &track_current_callback->params.qGrpTrack.notificationBuffer;
			SaNameT *name = &asapi_msg->info.tntfy.oinfo.qparam->name;
			if ((buff->notification[i].member.queueName.length == name->length) &&
			    memcmp(buff->notification[i].member.queueName.value, name->value, name->length) == 0) {
				TRACE_2("FAILURE: QUEUE already exists in notification list");
				rc = NCSCC_RC_FAILURE;
				goto done;
			}
		}

		callback_buffer[track_index].member.queueName = asapi_msg->info.tntfy.oinfo.qparam->name;
		/* Status is removed from B.1.1 
		   callback_buffer[track_index].member.sendingState = asapi_msg->info.tntfy.oinfo.qparam->status; */
		callback_buffer[track_index].change = SA_MSG_QUEUE_GROUP_ADDED;
		track_current_callback->params.qGrpTrack.notificationBuffer.numberOfItems = track_index + 1;

		/* Update the track_info notificationBuffer */
		track_info->notificationBuffer.numberOfItems = track_index + 1;
		track_info->track_index = track_index + 1;
		if (track_info->notificationBuffer.notification) {
			m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(track_info->notificationBuffer.notification);
			track_info->notificationBuffer.notification = NULL;
		}
		track_info->notificationBuffer.notification =
		    m_MMGR_ALLOC_MQA_TRACK_BUFFER_INFO((uint32_t)(track_index + 1));
		if (!track_info->notificationBuffer.notification) {
			TRACE_4("FAILURE: Track Buffer Info Allocation Failed");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		memcpy(track_info->notificationBuffer.notification,
		       track_current_callback->params.qGrpTrack.notificationBuffer.notification,
		       (track_index + 1) * sizeof(SaMsgQueueGroupNotificationT));
		track_info->notificationBuffer.notification[track_index].change = SA_MSG_QUEUE_GROUP_NO_CHANGE;

		break;

	case ASAPi_QUEUE_DEL:
		if (track_index == 0)
			goto done;

		track_current_callback->params.qGrpTrack.notificationBuffer.numberOfItems = track_index;
		mqa_track_remove_member(&track_current_callback->params.qGrpTrack.notificationBuffer,
					&asapi_msg->info.tntfy.oinfo.qparam->name,
					asapi_msg->info.tntfy.oinfo.qparam->status);

		/* update the track_ifo notificationBuffer */
		if (track_info->notificationBuffer.notification) {
			m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(track_info->notificationBuffer.notification);
			track_info->notificationBuffer.notification = NULL;
		}

		/* check whether there are any queues in the group or not */
		track_info->notificationBuffer.notification = m_MMGR_ALLOC_MQA_TRACK_BUFFER_INFO((uint32_t)(track_index));
		if (!track_info->notificationBuffer.notification) {
			TRACE_4("FAILURE: Track Buffer Info Allocation Failed");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		for (i = 0; i < num_items; i++) {
			SaMsgQueueGroupNotificationBufferT *buff =
			    &track_current_callback->params.qGrpTrack.notificationBuffer;
			SaNameT *name = &asapi_msg->info.tntfy.oinfo.qparam->name;
			if ((buff->notification[i].member.queueName.length == name->length) &&
			    memcmp(buff->notification[i].member.queueName.value, name->value, name->length) == 0)
				continue;
			memcpy(&track_info->notificationBuffer.notification[j],
			       &buff->notification[i], sizeof(SaMsgQueueGroupNotificationT));
			j++;
		}
		track_info->notificationBuffer.numberOfItems = track_index - 1;
		track_info->track_index = track_index - 1;
		break;

	case ASAPi_QUEUE_UPD:
		mqa_track_update_state(&track_current_callback->params.qGrpTrack.notificationBuffer,
				       &asapi_msg->info.tntfy.oinfo.qparam->name,
				       asapi_msg->info.tntfy.oinfo.qparam->status);
		track_current_callback->params.qGrpTrack.notificationBuffer.numberOfItems = track_index;
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (mqsv_mqa_callback_queue_write(mqa_cb, client_info->msgHandle, track_current_callback) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Call back Queue Write Failed");
		rc = NCSCC_RC_FAILURE;
		goto done1;
	}

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		if (track_current_callback) {
			m_MMGR_FREE_MQP_ASYNC_RSP_MSG(track_current_callback);
		}
		TRACE_2("FAILURE: Notify Changes Failed");
	}
 done1:
	if (rc != NCSCC_RC_SUCCESS) {
		if (callback_buffer) {
			m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(callback_buffer);
		}
	}

	m_MQSV_MQA_GIVEUP_MQA_CB;

	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : mqa_notify_changes_only

  Description   : This routine notifies the client
                  who are registered for group changes only.

  Arguments     : MQA_CLIENT_INFO *client_info - client info pointer
                  ASAPi_GROUP_TRACK_INFO * - Group track info pointer
                  ASAPi_MSG_INFO * - ASAP notify message that was received.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/
uint32_t mqa_notify_changes_only(MQA_CLIENT_INFO *client_info, MQA_TRACK_INFO *track_info, ASAPi_MSG_INFO *asapi_msg)
{

	MQP_ASYNC_RSP_MSG *track_current_callback = NULL;
	SaMsgQueueGroupNotificationT *callback_buffer = NULL;
	MQA_CB *mqa_cb;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if ((asapi_msg->info.tntfy.opr == ASAPi_QUEUE_MQND_DOWN) || (asapi_msg->info.tntfy.opr == ASAPi_QUEUE_MQND_UP)) {
		return NCSCC_RC_SUCCESS;
	}

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("FAILURE: Control block retrieval failed");
		return NCSCC_RC_FAILURE;
	}

	track_current_callback = m_MMGR_ALLOC_MQP_ASYNC_RSP_MSG;
	if (!track_current_callback) {
		TRACE_4("FAILURE: MQP Async Rsp Message Allocation Failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	track_current_callback->callbackType = MQP_ASYNC_RSP_GRP_TRACK;
	track_current_callback->messageHandle = client_info->msgHandle;
	track_current_callback->params.qGrpTrack.queueGroupName = asapi_msg->info.tntfy.oinfo.group;
	track_current_callback->params.qGrpTrack.queueGroupPolicy = asapi_msg->info.tntfy.oinfo.policy;
	track_current_callback->params.qGrpTrack.notificationBuffer.queueGroupPolicy =
	    asapi_msg->info.tntfy.oinfo.policy;

	track_current_callback->params.qGrpTrack.numberOfMembers = asapi_msg->info.tntfy.oinfo.qcnt;

	if (asapi_msg->info.tntfy.opr != ASAPi_GROUP_DEL) {
		callback_buffer = m_MMGR_ALLOC_MQA_TRACK_BUFFER_INFO(sizeof(SaMsgQueueGroupNotificationT));
		if (!callback_buffer) {
			TRACE_4("FAILURE: Track Buffer Info Allocation Failed");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		track_current_callback->params.qGrpTrack.notificationBuffer.notification = callback_buffer;
	}

	switch (asapi_msg->info.tntfy.opr) {
	case ASAPi_QUEUE_ADD:
	case ASAPi_QUEUE_DEL:
		track_current_callback->params.qGrpTrack.notificationBuffer.notification->member.queueName =
		    asapi_msg->info.tntfy.oinfo.qparam->name;
		track_current_callback->params.qGrpTrack.error = SA_AIS_OK;

		/* There is always 1 notification/change sent by MQD */
		track_current_callback->params.qGrpTrack.notificationBuffer.numberOfItems = 1;

		if (asapi_msg->info.tntfy.opr == ASAPi_QUEUE_ADD)
			track_current_callback->params.qGrpTrack.notificationBuffer.notification->change =
			    SA_MSG_QUEUE_GROUP_ADDED;
		else
			track_current_callback->params.qGrpTrack.notificationBuffer.notification->change =
			    SA_MSG_QUEUE_GROUP_REMOVED;

		/*  Status is removed from B.1.1
		   track_current_callback->params.qGrpTrack.notificationBuffer.notification->member.sendingState = asapi_msg->info.tntfy.oinfo.qparam->status;          */
		break;

	case ASAPi_GROUP_DEL:
		track_current_callback->params.qGrpTrack.error = SA_AIS_ERR_NOT_EXIST;
		track_current_callback->params.qGrpTrack.notificationBuffer.numberOfItems = 0;
		track_current_callback->params.qGrpTrack.notificationBuffer.notification = NULL;

		if (mqa_track_tree_find_and_del(client_info, &track_info->queueGroupName) != true) {
			TRACE_2("FAILURE: Track Tree Find and Delete Failed");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;

	case ASAPi_QUEUE_UPD:
	case ASAPi_GROUP_ADD:
		track_current_callback->params.qGrpTrack.notificationBuffer.notification->change =
		    SA_MSG_QUEUE_GROUP_STATE_CHANGED;
		goto done;
		break;

	default:
		goto done;
	}

	if (mqsv_mqa_callback_queue_write(mqa_cb, client_info->msgHandle, track_current_callback) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Call back Queue Write Failed");
		rc = NCSCC_RC_FAILURE;
		goto done1;
	}

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Notify Changes Only Failed");
		if (track_current_callback) {
			m_MMGR_FREE_MQP_ASYNC_RSP_MSG(track_current_callback);
		}

	}
 done1:
	if (rc != NCSCC_RC_SUCCESS) {
		if (callback_buffer) {
			m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(callback_buffer);
		}
	}

	m_MQSV_MQA_GIVEUP_MQA_CB;

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : mqa_asapi_msghandler
 
  Description   : This routine handles all the ASAPi callbacks.
 
  Arguments     : ASAPi_MSG_INFO *asapi_msg - ASAPi response message structure.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/

uint32_t mqa_asapi_msghandler(ASAPi_MSG_INFO *asapi_msg)
{

	TRACE_ENTER();
	if (!asapi_msg) {
		TRACE_2("FAILURE: ASAPi_MSG_INFO is NULL");
		return NCSCC_RC_FAILURE;
	}

	switch (asapi_msg->msgtype) {
	case ASAPi_MSG_TRACK_NTFY:
		return (mqa_notify_clients(asapi_msg));
	default:
		TRACE_3("FAILURE: supprted only for ASAPi_MSG_TRACK_NTFY flag");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
  Name          : mqsv_mqa_callback_queue_init
  
  Description   : This routine is used to initialize the queue for the callbacks.
 
  Arguments     : client_info - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t mqsv_mqa_callback_queue_init(MQA_CLIENT_INFO *client_info)
{
	if (m_NCS_IPC_CREATE(&client_info->callbk_mbx) == NCSCC_RC_SUCCESS) {
		if (m_NCS_IPC_ATTACH(&client_info->callbk_mbx) == NCSCC_RC_SUCCESS) {
			TRACE("mqsv_mqa_callback_queue_init called Success");
			return NCSCC_RC_SUCCESS;
		}
		m_NCS_IPC_RELEASE(&client_info->callbk_mbx, NULL);
	}
	TRACE_2("FAILURE: Callback Queue Initialization Failed");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : mqa_client_cleanup_mbx
  
  Description   : This routine is used to destroy the queue for the callbacks.
 
  Arguments     : client_info - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static bool mqa_client_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{

	TRACE_ENTER();
	MQP_ASYNC_RSP_MSG *curr = (MQP_ASYNC_RSP_MSG *)msg;

	if (curr) {
		m_MMGR_FREE_MQP_ASYNC_RSP_MSG(curr);
		TRACE_LEAVE();
		return true;
	} else
		return false;

}

/****************************************************************************
  Name          : mqsv_mqa_callback_queue_destroy
  
  Description   : This routine is used to destroy the queue for the callbacks.
 
  Arguments     : client_info - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
void mqsv_mqa_callback_queue_destroy(MQA_CLIENT_INFO *client_info)
{
	TRACE_ENTER();

	/* detach the mail box */
	m_NCS_IPC_DETACH(&client_info->callbk_mbx, mqa_client_cleanup_mbx, client_info);

	/* delete the mailbox */
	m_NCS_IPC_RELEASE(&client_info->callbk_mbx, NULL);
	TRACE_LEAVE();
}

/****************************************************************************
  Name          : mqsv_mqa_callback_queue_write
 
  Description   : This routine is used to queue the callbacks to the client 
                  by the MDS.
 
  Arguments     : 
                  mqa_cb - pointer to the mqa control block
                  handle - handle id of the client
                  clbk_info - pointer to the callback information
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t mqsv_mqa_callback_queue_write(MQA_CB *mqa_cb, SaMsgHandleT handle, MQP_ASYNC_RSP_MSG *clbk_info)
{
	MQA_CLIENT_INFO *client_info = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	/* Search for the node from the client tree */
	client_info = (MQA_CLIENT_INFO *)ncs_patricia_tree_get(&mqa_cb->mqa_client_tree, (uint8_t *)&handle);

	if (client_info == NULL) {
		/* recieved a callback for an non-existant client. so dump the callback info */
		m_MMGR_FREE_MQP_ASYNC_RSP_MSG(clbk_info);
		TRACE_2("FAILURE: Recieved a callback for an non-existant client");
		return NCSCC_RC_FAILURE;
	} else {
		if (client_info->finalize == 1) {
			TRACE_2("FAILURE: Already all pending callbacks are cancelled related to the particular handle");
			m_MMGR_FREE_MQP_ASYNC_RSP_MSG(clbk_info);
			return NCSCC_RC_FAILURE;
		}
		clbk_info->next = NULL;
		rc = m_NCS_IPC_SEND(&client_info->callbk_mbx, clbk_info, NCS_IPC_PRIORITY_NORMAL);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : mqsv_mqa_callback_queue_read
 
  Description   : This routine is used to read from the queue for the callbacks
 
  Arguments     : mqa_cb - pointer to the mqa control block
                  handle - handle id of the client
 
  Return Values : pointer to the callback
 
  Notes         : None
******************************************************************************/
static MQP_ASYNC_RSP_MSG *mqsv_mqa_callback_queue_read(MQA_CLIENT_INFO *client_info)
{
	TRACE_ENTER();
	MQP_ASYNC_RSP_MSG *return_callback = NULL;

	/* remove it to the queue */
	return_callback = (MQP_ASYNC_RSP_MSG *)m_NCS_IPC_NON_BLK_RECEIVE(&client_info->callbk_mbx, NULL);

	TRACE_LEAVE();
	return return_callback;
}

/****************************************************************************
  Name          : mqa_queue_reader
 
  Description   : This routine is used to notify mqa if there is a message
                  in the queue.
                  
 
  Arguments     : arg - Queue Handle disguised in void pointer
 
 
  Notes         : None
******************************************************************************/
void mqa_queue_reader(NCSCONTEXT arg)
{
	MQP_OPEN_RSP *openRsp = (MQP_OPEN_RSP *)arg;
	SaMsgQueueHandleT queueHandle, listenerHandle;
	NCS_OS_POSIX_MQ_REQ_INFO mq_req;
	NCS_OS_MQ_MSG mq_msg;
	MQA_QUEUE_INFO *queue_node = NULL;
	MQA_CB *mqa_cb;
	MQP_ASYNC_RSP_MSG *mqa_callbk_info = NULL;
	uint32_t existing_msg_count;
	TRACE_ENTER();

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("FAILURE: Control block retrieval failed");
		return;
	}

	queueHandle = openRsp->queueHandle;	/* Message Queue */
	listenerHandle = openRsp->listenerHandle;	/* Indicator Queue */
	existing_msg_count = openRsp->existing_msg_count;

	m_MMGR_FREE_MQA_OPEN_RSP(openRsp);

	/* get the client_info */
	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("Lock failed for control block write");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return;
	}

	/* Check if queueHandle is present in the tree */
	if ((queue_node = mqa_queue_tree_find_and_add(mqa_cb, queueHandle, false, NULL, 0)) == NULL) {
		TRACE_2("FAILURE: Queue database Registration Failed");
		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return;
	}
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	memset(&mq_req, 0, sizeof(NCS_OS_POSIX_MQ_REQ_INFO));
	mq_req.req = NCS_OS_POSIX_MQ_REQ_MSG_RECV;
	mq_req.info.recv.mqd = listenerHandle;

	/*  wait indefinitely */
	memset(&mq_req.info.recv.timeout, 0, sizeof(NCS_OS_POSIX_TIMESPEC));
	mq_req.info.recv.i_msg = &mq_msg;
	mq_req.info.recv.datalen = 1;
	mq_req.info.recv.dataprio = 0;

	while (1) {
		if ((existing_msg_count == 0) && (m_NCS_OS_POSIX_MQ(&mq_req) == NCSCC_RC_FAILURE))
			break;

		if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS)
			break;

		if ((queue_node = mqa_queue_tree_find_and_add(mqa_cb, queueHandle, false, NULL, 0)) == NULL) {
			TRACE_2("FAILURE: Queue database Registration Failed");
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			break;
		}

		if (queue_node->is_closed == true) {
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			break;
		}

		mqa_callbk_info = m_MMGR_ALLOC_MQP_ASYNC_RSP_MSG;
		if (!mqa_callbk_info) {
			TRACE_4("FAILURE: MQP Async Rsp Message Allocation Failed");
			m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
			break;
		}
		memset(mqa_callbk_info, 0, sizeof(MQP_ASYNC_RSP_MSG));
		mqa_callbk_info->callbackType = MQP_ASYNC_RSP_MSGRECEIVED;
		mqa_callbk_info->params.msgReceived.queueHandle = queueHandle;
		if (mqsv_mqa_callback_queue_write(mqa_cb, queue_node->client_info->msgHandle, mqa_callbk_info) !=
		    NCSCC_RC_SUCCESS)
			TRACE_2("FAILURE: Call back Queue Write Failed");

		if (existing_msg_count)
			existing_msg_count--;

		m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	}

	m_MQSV_MQA_GIVEUP_MQA_CB;
	TRACE_LEAVE();
	return;
}
