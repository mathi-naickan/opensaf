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

  This module contains routines related H&J IPC mechanisms.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  ncs_ipc_create.....create an IPC "mailbox"
  ncs_ipc_release....release an IPC "mailbox"
  ncs_ipc_attach.....attach to an IPC "mailbox"
  ncs_ipc_attach_ext.attach to an IPC "mailbox" with a task name
  ncs_ipc_detach.....detach from an IPC "mailbox"
  ipc_flush.........internal routine to flush an IPC "mailbox"
  ncs_ipc_recv.......retrieve a message from an IPC "mailbox"
  ncs_ipc_send.......send a message to an IPC "mailbox"
  ncs_ipc_config_max_msgs.....configure threshold limit of msgs
  ncs_ipc_config_usr_counters....allows a user to supply the address 
  of a 32-bit counter to track the number of messages lying in LEAP mailbox queues

 ******************************************************************************
 */

#include <ncsgl_defs.h>
#include "ncs_osprm.h"
#include "ncssysf_ipc.h"
#include "sysf_ipc.h"
#include "ncssysf_def.h"
#include "usrbuf.h"
#include "ncssysf_mem.h"

static NCS_IPC_MSG *ncs_ipc_recv_common(SYSF_MBX *mbx, NCS_BOOL block);
static uns32 ipc_enqueue_ind_processing(NCS_IPC *ncs_ipc, unsigned int queue_number);
static uns32 ipc_dequeue_ind_processing(NCS_IPC *ncs_ipc, unsigned int queue_number);

uns32 ncs_ipc_create(SYSF_MBX *mbx)
{
	NCS_IPC *ncs_ipc;
	uns32 rc;

	if (NULL == (ncs_ipc = (NCS_IPC *)m_NCS_MEM_ALLOC(sizeof(NCS_IPC),
							  NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 1)))
		return NCSCC_RC_FAILURE;

	/* create handle and give back the handle */
	*mbx = (SYSF_MBX)ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_OS_SVCS, (NCSCONTEXT)ncs_ipc);
	if (*mbx == 0) {
		/* free the memory */
		m_NCS_MEM_FREE(ncs_ipc, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 1);
		return NCSCC_RC_FAILURE;
	}

	memset(ncs_ipc, 0, sizeof(NCS_IPC));

	m_NCS_LOCK_INIT(&ncs_ipc->queue_lock);

	ncs_ipc->ref_count = 0;
	ncs_ipc->name = NULL;

	rc = m_NCS_SEL_OBJ_CREATE(&ncs_ipc->sel_obj);
	if (NCSCC_RC_SUCCESS == rc) {
	} else {
		m_NCS_LOCK_DESTROY(&ncs_ipc->queue_lock);
		m_NCS_MEM_FREE(ncs_ipc, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 1);
		return rc;
	}

	/* initialize queues... */
	ncs_ipc->queue[0].head = NULL;
	ncs_ipc->queue[0].tail = NULL;
	ncs_ipc->queue[1].head = NULL;
	ncs_ipc->queue[1].tail = NULL;
	ncs_ipc->queue[2].head = NULL;
	ncs_ipc->queue[2].tail = NULL;

	ncs_ipc->active_queue = 0;

	m_NCS_SM_IPC_ELEM_ADD(ncs_ipc);
	return rc;
}

static uns32 ipc_flush(NCS_IPC *ncs_ipc, NCS_IPC_CB remove_from_queue_cb, void *arg)
{
	NCS_IPC_MSG *msg, *p_next;
	NCS_IPC_MSG *p_prev = NULL;
	NCS_IPC_QUEUE *cur_queue;
	unsigned int ind;

	/* walk queues asking if each item should be removed... */
	for (ind = 0; ind < NCS_IPC_PRIO_LEVELS; ++ind) {
		cur_queue = &ncs_ipc->queue[ind];

		msg = cur_queue->head;
		while (NULL != msg) {
			p_next = msg->next;
			msg->next = NULL;
			if (FALSE == remove_from_queue_cb(arg, (void *)msg)) {
				msg->next = p_next;	/* restore next pointer */
				p_prev = msg;
			} else {	/* remove */

				if (msg == cur_queue->head) {
					cur_queue->head = p_next;
					if (msg == cur_queue->tail)
						cur_queue->tail = p_next;	/* should be NULL! */
				} else {
					p_prev->next = p_next;
					if (msg == cur_queue->tail)
						cur_queue->tail = p_prev;
				}

				m_NCS_SM_IPC_ELEM_CUR_DEPTH_DEC(ncs_ipc);
				ipc_dequeue_ind_processing(ncs_ipc, ind);
			}
			msg = p_next;
		}
	}

	return NCSCC_RC_SUCCESS;
}

NCS_SEL_OBJ ncs_ipc_get_sel_obj(SYSF_MBX *mbx)
{
	NCS_SEL_OBJ sel_obj;
	NCS_IPC *ncs_ipc;

	memset(&sel_obj, 0, sizeof(NCS_SEL_OBJ));
	if ((NULL == NCS_INT32_TO_PTR_CAST(mbx)) || (NULL == NCS_INT32_TO_PTR_CAST(*mbx)))
		return sel_obj;

	/* get the mailbox pointer from the handle */
	ncs_ipc = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_OS_SVCS, (uns32)*mbx);

	if (ncs_ipc == NULL)
		return sel_obj;
	sel_obj = (ncs_ipc->sel_obj);
	ncshm_give_hdl((uns32)*mbx);

	return sel_obj;
}

uns32 ncs_ipc_release(SYSF_MBX *mbx, NCS_IPC_CB remove_from_queue_cb)
{
	NCS_IPC *ncs_ipc;
	uns32 rc = NCSCC_RC_SUCCESS;

	if ((NULL == NCS_INT32_TO_PTR_CAST(mbx)) || (NULL == NCS_INT32_TO_PTR_CAST(*mbx)))
		return NCSCC_RC_FAILURE;

	/* get the mailbox pointer from the handle */
	ncs_ipc = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_OS_SVCS, (uns32)*mbx);

	if (ncs_ipc == NULL)
		return NCSCC_RC_FAILURE;

	m_NCS_LOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

	if (0 != ncs_ipc->ref_count) {
		/* all users must detach BEFORE doing a release. */
		m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
		ncshm_give_hdl((uns32)*mbx);
		return NCSCC_RC_FAILURE;
	}

	/* set the releasing flag and flush the queue... */
	if (NULL != remove_from_queue_cb)
		rc = ipc_flush(ncs_ipc, remove_from_queue_cb, NULL);

	m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

	/* destroy the handle - here */
	ncshm_give_hdl((uns32)*mbx);

	m_NCS_SEL_OBJ_RAISE_OPERATION_SHUT(&ncs_ipc->sel_obj);

	ncshm_destroy_hdl(NCS_SERVICE_ID_OS_SVCS, (uns32)*mbx);

	m_NCS_SEL_OBJ_RMV_OPERATION_SHUT(&ncs_ipc->sel_obj);

	m_NCS_LOCK_DESTROY(&ncs_ipc->queue_lock);
	m_NCS_SM_IPC_ELEM_DEL(ncs_ipc);

	if (ncs_ipc->name != NULL)
		m_NCS_MEM_FREE(ncs_ipc->name, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 1);

	/* free the memory */
	m_NCS_MEM_FREE(ncs_ipc, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 1);

	return rc;
}

uns32 ncs_ipc_attach(SYSF_MBX *mbx)
{
	NCS_IPC *ncs_ipc;

	if ((NULL == NCS_INT32_TO_PTR_CAST(mbx)) || (NULL == NCS_INT32_TO_PTR_CAST(*mbx)))
		return NCSCC_RC_FAILURE;

	/* get the mailbox pointer from the handle */
	ncs_ipc = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_OS_SVCS, NCS_PTR_TO_UNS32_CAST(*mbx));

	if (ncs_ipc == NULL)
		return NCSCC_RC_FAILURE;

	m_NCS_LOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

	/* increment the reference count. */
	m_NCS_ATOMIC_INC(&ncs_ipc->ref_count);

	m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

	ncshm_give_hdl((uns32)*mbx);
	return NCSCC_RC_SUCCESS;
}

uns32 ncs_ipc_attach_ext(SYSF_MBX *mbx, char *task_name)
{
	NCS_IPC *ncs_ipc;

	if ((NULL == task_name) || (NULL == NCS_INT32_TO_PTR_CAST(mbx)) || (NULL == NCS_INT32_TO_PTR_CAST(*mbx)))
		return NCSCC_RC_FAILURE;

	/* get the mailbox pointer from the handle */
	ncs_ipc = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_OS_SVCS, (uns32)*mbx);

	if (ncs_ipc == NULL)
		return NCSCC_RC_FAILURE;

	m_NCS_LOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

	/* increment the reference count. */
	m_NCS_ATOMIC_INC(&ncs_ipc->ref_count);

	if (NULL == (ncs_ipc->name = (char *)m_NCS_MEM_ALLOC(sizeof(char) * (strlen(task_name) + 1),
							     NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 1))) {
		m_NCS_MEM_FREE(ncs_ipc, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 1);
		ncshm_give_hdl((uns32)*mbx);
		return NCSCC_RC_FAILURE;
	}

	strcpy(ncs_ipc->name, task_name);

	m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

	ncshm_give_hdl((uns32)*mbx);
	return NCSCC_RC_SUCCESS;
}

uns32 ncs_ipc_detach(SYSF_MBX *mbx, NCS_IPC_CB remove_from_queue_cb, void *cb_arg)
{
	NCS_IPC *ncs_ipc;
	uns32 rc;

	if ((NULL == NCS_INT32_TO_PTR_CAST(mbx)) || (NULL == NCS_INT32_TO_PTR_CAST(*mbx)))
		return NCSCC_RC_FAILURE;

	/* get the mailbox pointer from the handle */
	ncs_ipc = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_OS_SVCS, (uns32)*mbx);

	if (ncs_ipc == NULL)
		return NCSCC_RC_FAILURE;

	m_NCS_LOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

	/* decrement the reference count. */
	m_NCS_ATOMIC_DEC(&ncs_ipc->ref_count);

	if (NULL == remove_from_queue_cb)
		rc = NCSCC_RC_SUCCESS;
	else
		rc = ipc_flush(ncs_ipc, remove_from_queue_cb, cb_arg);

	m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

	ncshm_give_hdl((uns32)*mbx);
	return rc;
}

NCS_IPC_MSG *ncs_ipc_recv(SYSF_MBX *mbx)
{
	return ncs_ipc_recv_common(mbx, TRUE);
}

NCS_IPC_MSG *ncs_ipc_non_blk_recv(SYSF_MBX *mbx)
{
	return ncs_ipc_recv_common(mbx, FALSE);
}

static NCS_IPC_MSG *ncs_ipc_recv_common(SYSF_MBX *mbx, NCS_BOOL block)
{
	NCS_IPC *ncs_ipc;
	NCS_IPC_MSG *msg;
	unsigned int active_queue;
	int inds_rmvd;
	NCS_SEL_OBJ mbx_obj;
	NCS_SEL_OBJ_SET obj_set;

	if ((NULL == NCS_INT32_TO_PTR_CAST(mbx)) || (NULL == NCS_INT32_TO_PTR_CAST(*mbx)))
		return NULL;

	mbx_obj = m_NCS_IPC_GET_SEL_OBJ(mbx);

	while (1) {

		/* Take the handle before proceeding */
		ncs_ipc = (NCS_IPC *)ncshm_take_hdl(NCS_SERVICE_ID_OS_SVCS, (uns32)*mbx);
		if (ncs_ipc == NULL)
			return NULL;

		if (block == TRUE) {
			m_NCS_SEL_OBJ_ZERO(&obj_set);
			m_NCS_SEL_OBJ_SET(mbx_obj, &obj_set);

			if (m_NCS_SEL_OBJ_SELECT(mbx_obj, &obj_set, NULL, NULL, NULL) != 1) {
				ncshm_give_hdl((uns32)*mbx);
				return NULL;
			}
		}

		m_NCS_LOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

		if (ncs_ipc->ref_count == 0) {
			m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
			ncshm_give_hdl((uns32)*mbx);
			return NULL;
		}
		msg = NULL;

		if (ncs_ipc->msg_count == 0) {
			/* 
			   We may reach here due to the following reasons.
			   Blocking case: Between the select() and m_NCS_LOCK() calls above, 
			   some thread detached from this mail-box, making this mailbox empty.
			   In such a case by the time we reach here, all indications
			   must have been removed.

			   Non-blocking case: Between the time that the caller chose to call
			   m_NCS_IPC_NON_BLK_RECV and the time that m_NCS_LOCK() was called,
			   some thread detached from this mail-box, making this mailbox empty.
			   In such a case by the time we reach here, all indications
			   must have been removed.
			 */
			inds_rmvd = m_NCS_SEL_OBJ_RMV_IND(ncs_ipc->sel_obj, TRUE, TRUE);
			if (inds_rmvd != 0) {
				/* Should never reach here */
				assert(0);
				m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
				ncshm_give_hdl((uns32)*mbx);
				m_LEAP_DBG_SINK(0);
				return NULL;
			} else {
				m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
				ncshm_give_hdl((uns32)*mbx);
				return NULL;
			}
		} else {
			/* queue is non-empty. Retrieve the message */
			/* get item from head of (ACTIVE) queue... */
			for (active_queue = 0; active_queue < NCS_IPC_PRIO_LEVELS; active_queue++) {
				if ((msg = ncs_ipc->queue[active_queue].head) != NULL) {
					m_NCS_SET_FN_QLAT();

					if ((ncs_ipc->queue[active_queue].head = msg->next) == NULL)
						ncs_ipc->queue[active_queue].tail = NULL;
					msg->next = NULL;
					/* ncs_ipc->active_queue = active_queue ^ 0x01; */

					m_NCS_SM_IPC_ELEM_CUR_DEPTH_DEC(ncs_ipc);

					if (ipc_dequeue_ind_processing(ncs_ipc, active_queue) != NCSCC_RC_SUCCESS) {
						m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
						ncshm_give_hdl((uns32)*mbx);
						m_LEAP_DBG_SINK(NULL);
						return NULL;
					} else {
						m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
						ncshm_give_hdl((uns32)*mbx);
						return msg;
					}
				}
			}
			/* We should never reach here. */
			assert(0);
			m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
			ncshm_give_hdl((uns32)*mbx);
			m_LEAP_DBG_SINK(NULL);
			return NULL;
		}

		m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
		ncshm_give_hdl((uns32)*mbx);
	}			/* end of while */
}


/************************************************************************\
  ipc_enqueue_ind_processing : Processing for NCS_IPC based on selection
                               objects.  This function is invoked, if a
                               message is removed from SYSF_IPC queue.

                               Allowed states of <msg-count, ind-status>
                               combination are:
                               (1)  <zero, no indication>
                               (2)  <non-zero, exactly one ind raised>
\************************************************************************/
static uns32 ipc_enqueue_ind_processing(NCS_IPC *ncs_ipc, unsigned int queue_number)
{
	if ((ncs_ipc->max_no_of_msgs[queue_number] != 0)
	    && (ncs_ipc->no_of_msgs[queue_number] >= ncs_ipc->max_no_of_msgs[queue_number]))
		return NCSCC_RC_FAILURE;

	if (ncs_ipc->msg_count == 0) {
		/* There are no messages queued, we shall raise an indication
		   on the "sel_obj".  */
		if (m_NCS_SEL_OBJ_IND(ncs_ipc->sel_obj) != NCSCC_RC_SUCCESS) {
			/* We would never reach here! */
			assert(0);
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}
	}
	ncs_ipc->msg_count++;	/* Don't think we need to check for 0xffffffff */

	ncs_ipc->no_of_msgs[queue_number]++;

	if (ncs_ipc->usr_counters[queue_number] != NULL)
		*(ncs_ipc->usr_counters[queue_number]) = ncs_ipc->no_of_msgs[queue_number];

	return NCSCC_RC_SUCCESS;
}

/************************************************************************\
  ipc_dequeue_ind_processing : Processing for NCS_IPC based on selection
                               objects.  This function is invoked, if a
                               message is removed from SYSF_IPC queue.

                               Allowed states of <msg-count, ind-status>
                               combination are:
                               (1)  <zero, no indication>
                               (2)  <non-zero, exactly one ind raised>
\************************************************************************/
static uns32 ipc_dequeue_ind_processing(NCS_IPC *ncs_ipc, unsigned int active_queue)
{
	int inds_rmvd;

	ncs_ipc->no_of_msgs[active_queue]--;

	if (ncs_ipc->usr_counters[active_queue] != NULL)
		*(ncs_ipc->usr_counters[active_queue]) = ncs_ipc->no_of_msgs[active_queue];

	ncs_ipc->msg_count--;

	if (ncs_ipc->msg_count == 0) {
		inds_rmvd = m_NCS_SEL_OBJ_RMV_IND(ncs_ipc->sel_obj, TRUE, TRUE);
		if (inds_rmvd <= 0) {
			if (inds_rmvd != -1) {
				/* The object has not been destroyed and it has no indication
				   raised on it inspite of msg_count being non-zero.
				 */
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
			/* The mbox must have been destroyed */
			return NCSCC_RC_FAILURE;
		}
	}
	return NCSCC_RC_SUCCESS;
}

uns32 ncs_ipc_send(SYSF_MBX *mbx, NCS_IPC_MSG *msg, NCS_IPC_PRIORITY prio)
{
	NCS_IPC *ncs_ipc;
	uns32 queue_number;

	if ((NULL == NCS_INT32_TO_PTR_CAST(mbx)) || (NULL == NCS_INT32_TO_PTR_CAST(*mbx)))
		return NCSCC_RC_FAILURE;

	ncs_ipc = (NCS_IPC *)ncshm_take_hdl(NCS_SERVICE_ID_OS_SVCS, (uns32)*mbx);
	if (ncs_ipc == NULL)
		return NCSCC_RC_FAILURE;

	m_NCS_LOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

	if (ncs_ipc->ref_count == 0) {
		/* 
		 * IPC queue is being released or has no "users" - don't queue
		 * messages...
		 */
		m_LEAP_DBG_SINK_VOID;
		m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
		ncshm_give_hdl((uns32)*mbx);
		return NCSCC_RC_FAILURE;
	}

	queue_number = NCS_IPC_PRIO_LEVELS - prio;

	/* Priority 4 goes into 4-4 = 0th queue, priority 3 goes into
	   4-3 = 1st queue, etc. 
	 */
	if (NULL != ncs_ipc->queue[queue_number].tail)
		ncs_ipc->queue[queue_number].tail->next = msg;
	else
		ncs_ipc->queue[queue_number].head = msg;

	ncs_ipc->queue[queue_number].tail = msg;
	msg->next = NULL;
	m_NCS_SM_IPC_ELEM_CUR_DEPTH_INC(ncs_ipc);
	m_NCS_SET_ST_QLAT();

	if (ipc_enqueue_ind_processing(ncs_ipc, queue_number) != NCSCC_RC_SUCCESS) {
		m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
		ncshm_give_hdl((uns32)*mbx);
		return NCSCC_RC_FAILURE;
	}

	m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);

	/* unblock receiver... */
	ncshm_give_hdl((uns32)*mbx);

	return NCSCC_RC_SUCCESS;
}

uns32 ncs_ipc_config_max_msgs(SYSF_MBX *mbx, NCS_IPC_PRIORITY prio, uns32 max_msgs)
{
	NCS_IPC *ncs_ipc;
	uns32 queue_number;

	if ((NULL == NCS_INT32_TO_PTR_CAST(mbx)) || ((prio >= NCS_IPC_PRIORITY_MAX) || (prio < NCS_IPC_PRIORITY_LOW)))
		return NCSCC_RC_FAILURE;

	if (NULL == NCS_INT32_TO_PTR_CAST(*mbx))
		return NCSCC_RC_FAILURE;

	ncs_ipc = (NCS_IPC *)ncshm_take_hdl(NCS_SERVICE_ID_OS_SVCS, (uns32)*mbx);
	if (ncs_ipc == NULL)
		return NCSCC_RC_FAILURE;

	m_NCS_LOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
	queue_number = NCS_IPC_PRIO_LEVELS - prio;
	ncs_ipc->max_no_of_msgs[queue_number] = max_msgs;
	m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
	ncshm_give_hdl((uns32)*mbx);

	return NCSCC_RC_SUCCESS;
}

uns32 ncs_ipc_config_usr_counters(SYSF_MBX *mbx, NCS_IPC_PRIORITY prio, uns32 *usr_counter)
{
	NCS_IPC *ncs_ipc;
	uns32 queue_number;

	if ((NULL == NCS_INT32_TO_PTR_CAST(mbx)) || ((prio >= NCS_IPC_PRIORITY_MAX) || (prio < NCS_IPC_PRIORITY_LOW))
	    || (usr_counter == NULL))
		return NCSCC_RC_FAILURE;

	if (NULL == NCS_INT32_TO_PTR_CAST(*mbx))
		return NCSCC_RC_FAILURE;

	ncs_ipc = (NCS_IPC *)ncshm_take_hdl(NCS_SERVICE_ID_OS_SVCS, (uns32)*mbx);

	if (ncs_ipc == NULL)
		return NCSCC_RC_FAILURE;

	m_NCS_LOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
	queue_number = NCS_IPC_PRIO_LEVELS - prio;
	ncs_ipc->usr_counters[queue_number] = usr_counter;
	m_NCS_UNLOCK(&ncs_ipc->queue_lock, NCS_LOCK_WRITE);
	ncshm_give_hdl((uns32)*mbx);

	return NCSCC_RC_SUCCESS;
}
