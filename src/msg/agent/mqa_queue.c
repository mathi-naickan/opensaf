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

  This file contains the routines to create/destroy/timeout handling of timers.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "mqa.h"

static void mqa_main_timeout_handler(void *arg);
static void mqa_node_timeout_handler(void *arg);
static bool match_invocation(void *key, void *qelem);
static bool match_expiry(void *key, void *qelem);
static void mqa_cleanup_senderid(void *arg);
static bool match_node(void *key, void *qelem);
static bool match_all(void *key, void *qelem);

/****************************************************************************
  Name          : mqa_timer_table_init
  
  Description   : This routine is used to initialize the timer table.
 
  Arguments     : cb - pointer to the MQA Control Block
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/

uint32_t mqa_timer_table_init(MQA_CB *mqa_cb)
{

	NCS_RP_TMR_INIT tmr_init_info;

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("FAILURE: Control block retrieval failed");
		return NCSCC_RC_FAILURE;

	}

	tmr_init_info.callback_arg = NULL;
	tmr_init_info.svc_id = NCSMDS_SVC_ID_MQA;
	tmr_init_info.svc_sub_id = 0;
	tmr_init_info.tmr_callback = mqa_main_timeout_handler;
	tmr_init_info.tmr_ganularity = 1;	/* in secs */

	if ((mqa_cb->mqa_tmr_cb = m_NCS_RP_TMR_INIT(&tmr_init_info)) == NULL) {
		TRACE_2("FAILURE: Tmr initialization Failed");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return NCSCC_RC_FAILURE;
	}

	ncs_create_queue(&(mqa_cb->mqa_timer_list));

	m_MQSV_MQA_GIVEUP_MQA_CB;
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
  Name          : mqa_timer_table_destroy
  
  Description   : This routine is used to destroy the timer table.
 
  Arguments     : cb - pointer to the MQA Control Block
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/

void mqa_timer_table_destroy(MQA_CB *mqa_cb)
{

	void *temp;
	MQA_TMR_NODE *tmr_node;

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("Control block retrieval failed");
		return;

	}

	while ((temp = (void *)ncs_dequeue(&(mqa_cb->mqa_timer_list))) != NCS_QELEM_NULL) {
		tmr_node = (MQA_TMR_NODE *)temp;

		/* we don't check the return value, as we have to cleanup anyways. */
		if (mqa_cb->mqa_tmr_cb) {
			ncs_rp_tmr_stop(mqa_cb->mqa_tmr_cb, tmr_node->tmr_id);

			ncs_rp_tmr_delete(mqa_cb->mqa_tmr_cb, tmr_node->tmr_id);

		}
	}

	if (mqa_cb->mqa_tmr_cb) {
		mqa_destroy_senderid_timers(mqa_cb);
		m_NCS_RP_TMR_DESTROY(&(mqa_cb->mqa_tmr_cb));
	}

	m_MQSV_MQA_GIVEUP_MQA_CB;

	return;
}

/****************************************************************************
  Name          : mqa_main_timeout_handler
 
  Description   : This routine will be called on expiry of the OS timer. This
                  inturn calls the timeout handler for all the bucket timers.
 
  Arguments     : void *arg - opaque argument passed when starting the timer.
 
  Return Values : None
 
  Notes         : None
******************************************************************************/

static void mqa_main_timeout_handler(void *arg)
{

	MQA_CB *mqa_cb;

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("Control block retrieval failed");
		return;
	}

	m_NCS_RP_TMR_EXP(mqa_cb->mqa_tmr_cb);

	m_MQSV_MQA_GIVEUP_MQA_CB;

	return;

}

/****************************************************************************
  Name          : mqa_node_timeout_handler
 
  Description   : This routine will be called on expiry of the timer in the 
                  timer node.
 
  Arguments     : void *arg - opaque argument passed when starting the timer.
 
  Return Values : None
 
  Notes         : None
******************************************************************************/

static void mqa_node_timeout_handler(void *arg)
{

	MQA_CB *mqa_cb;
	SaInvocationT key;
	MQA_TMR_NODE *tmr_node;

	MQP_ASYNC_RSP_MSG *mqa_callback = (MQP_ASYNC_RSP_MSG *)arg;

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("Control block retrieval failed");
		return;
	}

	/* delete the tmr_node from the timer_list */
	switch (mqa_callback->callbackType) {
	case MQP_ASYNC_RSP_OPEN:
		key = mqa_callback->params.qOpen.invocation;
		break;
	case MQP_ASYNC_RSP_MSGDELIVERED:
		key = mqa_callback->params.msgDelivered.invocation;
		break;
	default:
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return;
	}

	tmr_node = ncs_find_item(&(mqa_cb->mqa_timer_list), NCS_INT64_TO_PTR_CAST(key), match_invocation);

	if (tmr_node == NULL) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return;
	}

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return;
	}

	/*  Dequeue and free the node */
	ncs_remove_item(&(mqa_cb->mqa_timer_list), tmr_node, match_node);

	m_MMGR_FREE_MQA_TMR_NODE(tmr_node);

	mqsv_mqa_callback_queue_write(mqa_cb, mqa_callback->messageHandle, mqa_callback);

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);
	m_MQSV_MQA_GIVEUP_MQA_CB;

	return;

}

/****************************************************************************
  Name          : mqa_create_and_start_timer
 
  Description   : This routine creates a timer node and starts the timer adn
                  enqueues to the timer list under MQA control block.
 
  Arguments     : MQP_ASYNC_RSP_MSG *mqa_callback - Timer callback data.
                  SaInvocationT invocation 
 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 
  Notes         : None
******************************************************************************/

uint32_t mqa_create_and_start_timer(MQP_ASYNC_RSP_MSG *mqa_callback, SaInvocationT invocation)
{

	MQA_TMR_NODE *node;
	NCS_RP_TMR_HDL tmr_id;
	MQP_ASYNC_RSP_MSG *callback;
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQA_CB *mqa_cb;

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("FAILURE: Control block retrieval failed");
		return NCSCC_RC_FAILURE;
	}

	if ((tmr_id = ncs_rp_tmr_create(mqa_cb->mqa_tmr_cb)) == 0) {
		TRACE_2("FAILURE: Tmr Create Failed");
		goto err1;
	}

	callback = m_MMGR_ALLOC_MQP_ASYNC_RSP_MSG;
	if (!callback) {
		TRACE_4("FAILURE: MQP Async Rsp Message Allocation Failed");
		goto err2;
	}
	memset(callback, 0, sizeof(MQP_ASYNC_RSP_MSG));

	memcpy(callback, mqa_callback, sizeof(MQP_ASYNC_RSP_MSG));

	if ((rc = ncs_rp_tmr_start(mqa_cb->mqa_tmr_cb, tmr_id,
				   MQA_ASYNC_TIMEOUT_DEFAULT, mqa_node_timeout_handler,
				   (void *)callback)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Tmr Start Failed");
		goto err3;
	}

	/* Enqueue the timer information and invocation to the timer queue */

	node = (MQA_TMR_NODE *)m_MMGR_ALLOC_MQA_TMR_NODE;
	if (!node) {
		TRACE_4("FAILURE: Tmr database creation failed");
		goto err4;
	}

	node->tmr_id = tmr_id;
	node->invoc = invocation;
	node->callback = callback;

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		goto err5;
	}

	if (ncs_enqueue(&(mqa_cb->mqa_timer_list), (void *)node) != NCSCC_RC_SUCCESS) {
		goto err6;
	}

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	m_MQSV_MQA_GIVEUP_MQA_CB;

	return NCSCC_RC_SUCCESS;

 err6:
	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

 err5:
	m_MMGR_FREE_MQA_TMR_NODE(node);

 err4:
	ncs_rp_tmr_stop(mqa_cb->mqa_tmr_cb, tmr_id);

 err3:
	m_MMGR_FREE_MQP_ASYNC_RSP_MSG(callback);

 err2:
	ncs_rp_tmr_delete(mqa_cb->mqa_tmr_cb, tmr_id);

 err1:
	m_MQSV_MQA_GIVEUP_MQA_CB;

	return NCSCC_RC_FAILURE;

}

static bool match_invocation(void *key, void *qelem)
{

	MQA_TMR_NODE *node = (MQA_TMR_NODE *)qelem;
	SaInvocationT invocation = NCS_PTR_TO_UNS64_CAST(key);

	if (!qelem)
		return false;

	if (invocation == node->invoc)
		return true;

	return false;
}

static bool match_node(void *key, void *qelem)
{

	if (key == qelem)
		return true;

	return false;

}

/****************************************************************************
  Name          : mqa_stop_and_delete_timer
 
  Description   : This routine stops the timer and deletes the timer node
                  enqueues to the timer list under MQA control block.
 
  Arguments     : MQP_ASYNC_RSP_MSG *mqa_callback - Timer callback data.
                  SaInvocationT invocation 
 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 
  Notes         : None
******************************************************************************/

uint32_t mqa_stop_and_delete_timer(MQP_ASYNC_RSP_MSG *mqa_callbk_info)
{
	MQA_CB *mqa_cb;
	SaInvocationT key;
	MQA_TMR_NODE *tmr_node;
	uint32_t rc = NCSCC_RC_FAILURE;

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("FAILURE: Control block retrieval failed");
		return rc;
	}

	switch (mqa_callbk_info->callbackType) {
	case MQP_ASYNC_RSP_OPEN:
		key = mqa_callbk_info->params.qOpen.invocation;
		break;
	case MQP_ASYNC_RSP_MSGDELIVERED:
		key = mqa_callbk_info->params.msgDelivered.invocation;
		break;
	default:
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	tmr_node = ncs_find_item(&(mqa_cb->mqa_timer_list), NCS_INT64_TO_PTR_CAST(key), match_invocation);
	if (tmr_node == NULL) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	if ((rc = ncs_rp_tmr_stop(mqa_cb->mqa_tmr_cb, tmr_node->tmr_id)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Tmr Stop Failed");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	if ((rc = ncs_rp_tmr_delete(mqa_cb->mqa_tmr_cb, tmr_node->tmr_id)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Tmr Deletion Failed");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	/*  Dequeue and free the node */
	ncs_remove_item(&(mqa_cb->mqa_timer_list), tmr_node, match_node);

	if (tmr_node->callback)
		m_MMGR_FREE_MQP_ASYNC_RSP_MSG(tmr_node->callback);

	m_MMGR_FREE_MQA_TMR_NODE(tmr_node);

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	m_MQSV_MQA_GIVEUP_MQA_CB;
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : mqa_stop_and_delete_timer_by_invocation
 
  Description   : This routine stops the timer and deletes the timer node
                  enqueues to the timer list under MQA control block.
 
  Arguments     : key - Pass SaInvocationT invocation as key
 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 
  Notes         : None
******************************************************************************/
uint32_t mqa_stop_and_delete_timer_by_invocation(void *key)
{
	MQA_CB *mqa_cb;
	MQA_TMR_NODE *tmr_node;
	uint32_t rc = NCSCC_RC_FAILURE;

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("FAILURE: Control block retrieval failed");
		return rc;
	}

	tmr_node = ncs_find_item(&(mqa_cb->mqa_timer_list), key, match_invocation);
	if (tmr_node == NULL) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	if ((rc = ncs_rp_tmr_stop(mqa_cb->mqa_tmr_cb, tmr_node->tmr_id)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Tmr Stop Failed");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	if ((rc = ncs_rp_tmr_delete(mqa_cb->mqa_tmr_cb, tmr_node->tmr_id)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Tmr Deletion Failed");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return rc;
	}

	/*  Dequeue and free the node */
	ncs_remove_item(&(mqa_cb->mqa_timer_list), tmr_node, match_node);

	if (tmr_node->callback)
		m_MMGR_FREE_MQP_ASYNC_RSP_MSG(tmr_node->callback);

	m_MMGR_FREE_MQA_TMR_NODE(tmr_node);

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	m_MQSV_MQA_GIVEUP_MQA_CB;
	return NCSCC_RC_SUCCESS;
}

static bool match_expiry(void *key, void *qelem)
{

	MQA_SENDERID_INFO *node = (MQA_SENDERID_INFO *)qelem;
	time_t now;

	if (!node)
		return false;

	m_NCS_OS_GET_TIME_STAMP(now);

	if ((now - node->timestamp) > MQSV_SENDERID_CLEANUP_INTERVAL)
		return true;

	return false;
}

static void mqa_cleanup_senderid(void *arg)
{

	MQA_CB *mqa_cb;
	MQA_SENDERID_INFO *senderid_node;

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("Control block retrieval failed");
		return;
	}

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return;
	}

	while ((senderid_node = ncs_remove_item(&(mqa_cb->mqa_senderid_list), NULL, match_expiry)) != NULL) {
		m_MMGR_FREE_MQA_SENDERID(senderid_node);
	}

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	m_MQSV_MQA_GIVEUP_MQA_CB;
	return;

}

/****************************************************************************
  Name          : mqa_create_and_start_senderid_timer
 
  Description   : This routine creates a sender id timer node and starts the timer adn
                  enqueues  the senderid info  under MQA control block.
 
  Arguments     : None.
 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 
  Notes         : None
******************************************************************************/

uint32_t mqa_create_and_start_senderid_timer()
{
	MQA_CB *mqa_cb;
	NCS_RP_TMR_HDL tmr_id;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* retrieve MQA CB */
	mqa_cb = (MQA_CB *)m_MQSV_MQA_RETRIEVE_MQA_CB;
	if (!mqa_cb) {
		TRACE_2("FAILURE: Control block retrieval failed");
		return NCSCC_RC_FAILURE;
	}

	if ((tmr_id = ncs_rp_tmr_create(mqa_cb->mqa_tmr_cb)) == 0) {
		TRACE_2("FAILURE: Tmr Create Failed");
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return NCSCC_RC_FAILURE;
	}

	if ((rc = ncs_rp_tmr_start(mqa_cb->mqa_tmr_cb, tmr_id,
				   MQSV_SENDERID_CLEANUP_INTERVAL, mqa_cleanup_senderid,
				   (void *)NULL)) == NCSCC_RC_FAILURE) {
		TRACE_2("FAILURE: Tmr Start Failed");
		ncs_rp_tmr_delete(mqa_cb->mqa_tmr_cb, tmr_id);
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return NCSCC_RC_FAILURE;
	}

	if (m_NCS_LOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_MQSV_MQA_GIVEUP_MQA_CB;
		return NCSCC_RC_FAILURE;
	}

	mqa_cb->mqa_senderid_tmr = tmr_id;

	m_NCS_UNLOCK(&mqa_cb->cb_lock, NCS_LOCK_WRITE);

	m_MQSV_MQA_GIVEUP_MQA_CB;
	return NCSCC_RC_SUCCESS;
}

static bool match_all(void *key, void *qelem)
{

	return true;
}

/****************************************************************************
  Name          : mmqa_destroy_senderid_timers
 
  Description   : This routine stops the timer and deletes the senderid timer node
                  enqueued to the timer list under MQA control block.
 
  Arguments     : MQA_CB    *mqa_cb MQA Control block.
 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 
  Notes         : None
******************************************************************************/

uint32_t mqa_destroy_senderid_timers(MQA_CB *mqa_cb)
{

	MQA_SENDERID_INFO *senderid_node = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	while ((senderid_node = ncs_remove_item(&(mqa_cb->mqa_senderid_list), NULL, match_all)) != NULL) {
		m_MMGR_FREE_MQA_SENDERID(senderid_node);

	}

	ncs_destroy_queue(&(mqa_cb->mqa_senderid_list));

	if ((rc = ncs_rp_tmr_stop(mqa_cb->mqa_tmr_cb, mqa_cb->mqa_senderid_tmr)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Tmr Stop Failed");
		return NCSCC_RC_FAILURE;
	}

	if ((rc = ncs_rp_tmr_delete(mqa_cb->mqa_tmr_cb, mqa_cb->mqa_senderid_tmr)) != NCSCC_RC_SUCCESS) {
		TRACE_2("FAILURE: Tmr Deletion Failed");
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;

}
