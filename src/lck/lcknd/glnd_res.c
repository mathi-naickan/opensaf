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

  This file contains functions related to the resource structure handling.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************/

#include "lck/lcknd/glnd.h"
#include <string.h>

static void glnd_master_process_lock_initiate_waitercallbk(GLND_CB *cb,
							   GLND_RESOURCE_INFO *res_info,
							   GLSV_LOCK_REQ_INFO lock_info, SaLckLockIdT lockid);
static bool glnd_resource_grant_list_exclusive_locks(GLND_RESOURCE_INFO *res_info);

static uint32_t glnd_initiate_deadlock_algorithm(GLND_CB *cb,
					      GLND_RESOURCE_INFO *res_info,
					      GLSV_LOCK_REQ_INFO lock_info, GLND_RES_LOCK_LIST_INFO *lck_list_info);

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_node_find

  DESCRIPTION    : Finds the Resource node from the resource tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  res_id       - resource id index.

  RETURNS        :The pointer to the resource info node

  NOTES         : None
*****************************************************************************/

GLND_RESOURCE_INFO *glnd_resource_node_find(GLND_CB *glnd_cb, SaLckResourceIdT res_id)
{
	GLND_RESOURCE_INFO *res_info = NULL;

	res_info = (GLND_RESOURCE_INFO *)ncs_patricia_tree_get(&glnd_cb->glnd_res_tree, (uint8_t *)&res_id);

	return res_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_node_find_by_name

  DESCRIPTION    : Finds the Resource node from the resource tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  res_id       - resource id index.

  RETURNS        :The pointer to the resource info node

  NOTES         : None
*****************************************************************************/

GLND_RESOURCE_INFO *glnd_resource_node_find_by_name(GLND_CB *glnd_cb, SaNameT *res_name)
{
	GLND_RESOURCE_INFO *res_info = NULL;
	SaLckResourceIdT prev_rsc_info;

	res_info = (GLND_RESOURCE_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_res_tree, (uint8_t *)0);
	while (res_info) {
		prev_rsc_info = res_info->resource_id;
		if (memcmp(res_name, &res_info->resource_name, sizeof(SaNameT)) == 0) {
			return res_info;
		}
		res_info =
		    (GLND_RESOURCE_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_res_tree, (uint8_t *)&prev_rsc_info);
	}
	return NULL;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_node_add

  DESCRIPTION    : Adds the Resource node from the resource tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  res_id       - resource id index.
                  Resource_name - The resource name
                  is_master  - indicates the mastership
                  master_vcard_id - the Mds handle for the master node.

  RETURNS        :The pointer to the resource info node on success.
                  else returns NULL.

  NOTES         : None
*****************************************************************************/
GLND_RESOURCE_INFO *glnd_resource_node_add(GLND_CB *glnd_cb,
					   SaLckResourceIdT res_id,
					   SaNameT *resource_name, bool is_master, MDS_DEST master_mds_dest)
{
	GLND_RESOURCE_INFO *res_info = NULL;

	/* check to see if already present */
	if ((res_info = glnd_resource_node_find(glnd_cb, res_id)) != NULL) {
		return res_info;
	}

	/* allocate the memory */
	res_info = (GLND_RESOURCE_INFO *)m_MMGR_ALLOC_GLND_RESOURCE_INFO;

	if (!res_info) {
		LOG_CR("GLND Rsc node alloc failed: resource_id :%u resource_name: %s Error: %s", 
				res_id, resource_name->value, strerror(errno) );
		assert(0);
	}
	memset(res_info, 0, sizeof(GLND_RESOURCE_INFO));

	/* assign the values */
	res_info->resource_id = res_id;
	if (is_master)
		res_info->status = GLND_RESOURCE_ACTIVE_MASTER;
	else
		res_info->status = GLND_RESOURCE_ACTIVE_NON_MASTER;

	memcpy(&res_info->resource_name, resource_name, sizeof(SaNameT));
	res_info->master_mds_dest = master_mds_dest;
	res_info->master_status = GLND_OPERATIONAL_STATE;

	/* add it to the tree */
	res_info->patnode.key_info = (uint8_t *)&res_info->resource_id;
	if (ncs_patricia_tree_add(&glnd_cb->glnd_res_tree, &res_info->patnode) != NCSCC_RC_SUCCESS) {
		LOG_ER("GLND Rsc node add failed");
		m_MMGR_FREE_GLND_RESOURCE_INFO(res_info);
		return NULL;
	}
	TRACE("GLND Resource node add - %d", (uint32_t)res_id);
	/* log the Resource Add */
	TRACE("GLND Rsc node add success: res_id %u ", (uint32_t)res_id);
	return res_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_set_orphan_state

  DESCRIPTION    : Deletes the Resource node from the resource tree.

  ARGUMENTS      : 
                  glnd_cb      - ptr to the GLND control block
                  res_info     - resource node.

  RETURNS        : NCSCC_RC_SUCCESS/NCS_RC_FAILURE

  NOTES         : Decrements the reference count and deletes the node when it reaches
                  zero.
*****************************************************************************/
uint32_t glnd_set_orphan_state(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info)
{
	GLND_RES_LOCK_LIST_INFO *grant_list = NULL;

	if (res_info->lck_master_info.grant_list == NULL) {
		return NCSCC_RC_FAILURE;
	}
	grant_list = res_info->lck_master_info.grant_list;

	/* decrement the local reference */
	while (grant_list != NULL) {
		if ((grant_list->lock_info.lockFlags & SA_LCK_LOCK_ORPHAN) == SA_LCK_LOCK_ORPHAN) {
			if (grant_list->lock_info.lock_type == SA_LCK_EX_LOCK_MODE)
				res_info->lck_master_info.ex_orphaned = true;
			if (grant_list->lock_info.lock_type == SA_LCK_PR_LOCK_MODE)
				res_info->lck_master_info.pr_orphaned = true;

			glnd_restart_resource_info_ckpt_overwrite(glnd_cb, res_info);
			/* send notification to the GLD about the orphan locks */
			GLSV_GLD_EVT gld_evt;

			memset(&gld_evt, 0, sizeof(GLSV_GLD_EVT));
			m_GLND_RESOURCE_LCK_FILL(gld_evt, GLSV_GLD_EVT_SET_ORPHAN,
						 res_info->resource_id, true, grant_list->lock_info.lock_type);
			glnd_mds_msg_send_gld(glnd_cb, &gld_evt, glnd_cb->gld_mdest_id);

		}
		grant_list = grant_list->next;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_lock_req_set_orphan

  DESCRIPTION    : Maintains the orphan lock count
  
  ARGUMENTS      : 
                  glnd_cb      - ptr to the GLND control block
                  res_info     - resource node.

  RETURNS        : NCSCC_RC_SUCCESS/NCS_RC_FAILURE

  NOTES         : maintains track of local orphan lock requests
*****************************************************************************/
void glnd_resource_lock_req_set_orphan(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info, SaLckLockModeT type)
{

	/* increment the local reference */
	if (SA_LCK_PR_LOCK_MODE == type) {
		res_info->lck_master_info.pr_orphan_req_count++;
	} else {
		res_info->lck_master_info.ex_orphan_req_count++;
	}
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_lock_req_unset_orphan

  DESCRIPTION    : Maintains the orphan lock count
  
  ARGUMENTS      : 
                  glnd_cb      - ptr to the GLND control block
                  res_info     - resource node.

  RETURNS        : NCSCC_RC_SUCCESS/NCS_RC_FAILURE

  NOTES         : 
*****************************************************************************/
void glnd_resource_lock_req_unset_orphan(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info, SaLckLockModeT type)
{

	/* increment the local reference */
	if (SA_LCK_PR_LOCK_MODE == type) {
		if (res_info->lck_master_info.pr_orphan_req_count != 0)
			res_info->lck_master_info.pr_orphan_req_count--;
	} else {
		if (res_info->lck_master_info.ex_orphan_req_count != 0)
			res_info->lck_master_info.ex_orphan_req_count--;
	}
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_node_destroy

  DESCRIPTION    : Deletes the Resource node from the resource tree.

  ARGUMENTS      : 
                  glnd_cb      - ptr to the GLND control block
                  res_info     - resource node.

  RETURNS        : NCSCC_RC_SUCCESS/NCS_RC_FAILURE

  NOTES         : Decrements the reference count and deletes the node when it reaches
                  zero.
*****************************************************************************/
uint32_t glnd_resource_node_destroy(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info, bool orphan)
{
	GLSV_GLD_EVT evt;
	GLND_CLIENT_INFO *client_info = NULL;
	SaLckHandleT prev_app_handle_id;
	GLND_CLIENT_LIST_RESOURCE *res_list = NULL;
	GLND_RES_LOCK_LIST_INFO *lock_info = NULL;
	GLND_RES_LOCK_LIST_INFO *prev_lock_info = NULL;

	if (res_info == NULL)
		return NCSCC_RC_FAILURE;
	memset(&evt, 0, sizeof(GLSV_GLD_EVT));
	evt.evt_type = GLSV_GLD_EVT_RSC_CLOSE;
	evt.info.rsc_details.rsc_id = res_info->resource_id;
	evt.info.rsc_details.lcl_ref_cnt = res_info->lcl_ref_cnt;
	evt.info.rsc_details.orphan = orphan;
	glnd_mds_msg_send_gld(glnd_cb, &evt, glnd_cb->gld_mdest_id);

	if (ncs_patricia_tree_del(&glnd_cb->glnd_res_tree, &res_info->patnode) != NCSCC_RC_SUCCESS) {
		LOG_ER("GLND Rsc node destroy failed");
		return NCSCC_RC_FAILURE;
	}

	TRACE("GLND Resource node destroy - %d", (uint32_t)res_info->resource_id);
	TRACE("GLND Rsc node destroy success: resource_id  %u", (uint32_t)res_info->resource_id);

	for (lock_info = res_info->lck_master_info.grant_list; lock_info != NULL;) {
		prev_lock_info = lock_info;
		lock_info = lock_info->next;

		glnd_resource_lock_req_delete(res_info, prev_lock_info);
	}

	for (lock_info = res_info->lck_master_info.wait_exclusive_list; lock_info != NULL;) {
		prev_lock_info = lock_info;
		lock_info = lock_info->next;

		glnd_resource_lock_req_delete(res_info, prev_lock_info);
	}

	client_info = (GLND_CLIENT_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_client_tree, (uint8_t *)0);
	while (client_info) {
		prev_app_handle_id = client_info->app_handle_id;

		for (res_list = client_info->res_list; res_list != NULL;) {
			if (res_list->rsc_info == res_info)
				res_list->rsc_info = NULL;
			res_list = res_list->next;

		}

		client_info =
		    (GLND_CLIENT_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_client_tree,
								  (uint8_t *)&prev_app_handle_id);
	}

	glnd_res_shm_section_invalidate(glnd_cb, res_info);
	/* free the memory */
	m_MMGR_FREE_GLND_RESOURCE_INFO(res_info);
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_grant_lock_req_find

  DESCRIPTION    : Find the lock reuqest from the grant list

  ARGUMENTS      : 
                  res_info      - ptr to the Resource Node.
                  lock_info     - pointer to the lock info
                  req_mds_dest  - The requesting node director mds dest.

  RETURNS        : returns pointer to the master info node.

  NOTES         : None
*****************************************************************************/
GLND_RES_LOCK_LIST_INFO *glnd_resource_grant_lock_req_find(GLND_RESOURCE_INFO *res_info,
							   GLSV_LOCK_REQ_INFO res_lock_info,
							   MDS_DEST req_mds_dest, SaLckResourceIdT lcl_resource_id)
{
	GLND_RES_LOCK_LIST_INFO *lock_info;
	TRACE_ENTER();
	for (lock_info = res_info->lck_master_info.grant_list; lock_info != NULL; lock_info = lock_info->next) {

		TRACE("handleId: %llu lcl_lockid: %llu lcl_resource_id: %u res handleId: %llu res lcl_lockid: %llu res lcl_resource_id: %u req_mdest_id: %d req_mds_dest: %d is local node: %i",
				lock_info->lock_info.handleId, lock_info->lock_info.lcl_lockid, lock_info->lcl_resource_id, res_lock_info.handleId, res_lock_info.lcl_lockid, lcl_resource_id, m_NCS_NODE_ID_FROM_MDS_DEST(lock_info->req_mdest_id), m_NCS_NODE_ID_FROM_MDS_DEST(req_mds_dest), m_GLND_IS_LOCAL_NODE(&lock_info->req_mdest_id, &req_mds_dest));
		if (lock_info->lock_info.handleId == res_lock_info.handleId &&
		    lock_info->lock_info.lcl_lockid == res_lock_info.lcl_lockid &&
		    lock_info->lcl_resource_id == lcl_resource_id &&
		    m_GLND_IS_LOCAL_NODE(&lock_info->req_mdest_id, &req_mds_dest) == 0)
			return lock_info;
	}
	TRACE_LEAVE();
	return NULL;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_pending_lock_req_find

  DESCRIPTION    : Find the pending lock request from the pending list

  ARGUMENTS      : 
                  res_info      - ptr to the Resource Node.
                  lock_info     - pointer to the lock info
                  req_mds_dest  - The requesting node director mds dest.

  RETURNS        : returns pointer to the master info node.

  NOTES         : None
*****************************************************************************/
GLND_RES_LOCK_LIST_INFO *glnd_resource_pending_lock_req_find(GLND_RESOURCE_INFO *res_info,
							     GLSV_LOCK_REQ_INFO res_lock_info,
							     MDS_DEST req_mds_dest, SaLckResourceIdT lcl_resource_id)
{
	GLND_RES_LOCK_LIST_INFO *lock_info;
	for (lock_info = res_info->lck_master_info.wait_exclusive_list; lock_info != NULL; lock_info = lock_info->next) {
		if (lock_info->lock_info.handleId == res_lock_info.handleId &&
		    lock_info->lock_info.lcl_lockid == res_lock_info.lcl_lockid &&
		    lock_info->lcl_resource_id == lcl_resource_id &&
		    m_GLND_IS_LOCAL_NODE(&lock_info->req_mdest_id, &req_mds_dest) == 0)
			return lock_info;
	}
	for (lock_info = res_info->lck_master_info.wait_read_list; lock_info != NULL; lock_info = lock_info->next) {
		if (lock_info->lock_info.handleId == res_lock_info.handleId &&
		    lock_info->lock_info.lcl_lockid == res_lock_info.lockid &&
		    lock_info->lcl_resource_id == lcl_resource_id &&
		    m_GLND_IS_LOCAL_NODE(&lock_info->req_mdest_id, &req_mds_dest) == 0)
			return lock_info;
	}

	return NULL;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_remote_lock_req_find

  DESCRIPTION    : Find the lock request from the master list

  ARGUMENTS      :res_info      - ptr to the Resource Node.
                  lockid - lock id of the lock.
                  handleId - handle id of the client
                  req_mds_dest  - The requesting node director mds dest.

  RETURNS        : returns pointer to the master info node.

  NOTES         : None
*****************************************************************************/
GLND_RES_LOCK_LIST_INFO *glnd_resource_remote_lock_req_find(GLND_RESOURCE_INFO *res_info,
							    SaLckLockIdT lockid,
							    SaLckHandleT handleId,
							    MDS_DEST req_mds_dest, SaLckResourceIdT lcl_resource_id)
{
	GLND_RES_LOCK_LIST_INFO *lock_info = NULL;

	for (lock_info = res_info->lck_master_info.grant_list; lock_info != NULL; lock_info = lock_info->next) {
		if (lock_info->lock_info.handleId == handleId &&
		    lock_info->lock_info.lockid == lockid &&
		    lock_info->lcl_resource_id == lcl_resource_id &&
		    m_GLND_IS_LOCAL_NODE(&lock_info->req_mdest_id, &req_mds_dest) == 0)
			return lock_info;
	}
	for (lock_info = res_info->lck_master_info.wait_exclusive_list; lock_info != NULL; lock_info = lock_info->next) {
		if (lock_info->lock_info.handleId == handleId &&
		    lock_info->lock_info.lcl_lockid == lockid &&
		    lock_info->lcl_resource_id == lcl_resource_id &&
		    m_GLND_IS_LOCAL_NODE(&lock_info->req_mdest_id, &req_mds_dest) == 0)
			return lock_info;

	}
	for (lock_info = res_info->lck_master_info.wait_read_list; lock_info != NULL; lock_info = lock_info->next) {
		if (lock_info->lock_info.handleId == handleId &&
		    lock_info->lock_info.lcl_lockid == lockid &&
		    lock_info->lcl_resource_id == lcl_resource_id &&
		    m_GLND_IS_LOCAL_NODE(&lock_info->req_mdest_id, &req_mds_dest) == 0)
			return lock_info;

	}

	return NULL;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_local_lock_req_find

  DESCRIPTION    : Finds the local lock resquest from the local lock req queue
  ARGUMENTS      : 
                  res_info      - ptr to the Resource Node.
                  lock_info     - pointer to the lock info
                  req_mds_dest  - The requesting node director mds dest.

  RETURNS        : returns pointer to the master info node.

  NOTES         : None
*****************************************************************************/
GLND_RES_LOCK_LIST_INFO *glnd_resource_local_lock_req_find(GLND_RESOURCE_INFO *res_info,
							   SaLckLockIdT lockid,
							   SaLckHandleT handleId, SaLckResourceIdT lcl_resource_id)
{
	GLND_RES_LOCK_LIST_INFO *lock_info;
	TRACE_ENTER();
	for (lock_info = res_info->lcl_lck_req_info; lock_info != NULL; lock_info = lock_info->next) {

		TRACE("handleId: %llu lcl_lockid: %llu lcl_resource_id: %u res handleId: %llu res lcl_lockid: %llu res lcl_resource_id: %u",
				lock_info->lock_info.handleId, lock_info->lock_info.lcl_lockid, lock_info->lcl_resource_id, handleId, lockid, lcl_resource_id);
		if (lock_info->lock_info.handleId == handleId &&
		    lock_info->lock_info.lcl_lockid == lockid && lock_info->lcl_resource_id == lcl_resource_id)
			return lock_info;
	}
	TRACE_LEAVE();
	return NULL;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_lock_req_delete

  DESCRIPTION    : Deletes the lock request 

  ARGUMENTS      :res_info      - ptr to the Resource Node.
                  lock_info     - pointer to the lock info

  RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
void glnd_resource_lock_req_delete(GLND_RESOURCE_INFO *res_info, GLND_RES_LOCK_LIST_INFO *lck_list_info)
{
	GLND_CB *glnd_cb;
	/* take the handle */
	glnd_cb = (GLND_CB *)m_GLND_TAKE_GLND_CB;
	if (!glnd_cb) {
		LOG_ER("GLND cb take handle failed");
		return;
	}

	if (lck_list_info->lock_info.lockStatus != SA_LCK_LOCK_NOT_QUEUED
	    && lck_list_info->lock_info.lockStatus != SA_LCK_LOCK_ORPHANED)
		glnd_lck_shm_section_invalidate(glnd_cb, lck_list_info);

	glnd_resource_lock_req_destroy(res_info, lck_list_info);

	/* Giveup the handle */
	m_GLND_GIVEUP_GLND_CB;
	return;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_lock_req_destroy

  DESCRIPTION    : Deletes the lock request 

  ARGUMENTS      :res_info      - ptr to the Resource Node.
                  lock_info     - pointer to the lock info

  RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
void glnd_resource_lock_req_destroy(GLND_RESOURCE_INFO *res_info, GLND_RES_LOCK_LIST_INFO *lck_list_info)
{
	if (res_info->lcl_lck_req_info == lck_list_info) {
		res_info->lcl_lck_req_info = lck_list_info->next;
	} else if (res_info->lck_master_info.grant_list == lck_list_info) {
		res_info->lck_master_info.grant_list = lck_list_info->next;
	} else if (res_info->lck_master_info.wait_read_list == lck_list_info) {
		res_info->lck_master_info.wait_read_list = lck_list_info->next;
	} else if (res_info->lck_master_info.wait_exclusive_list == lck_list_info) {
		res_info->lck_master_info.wait_exclusive_list = lck_list_info->next;
	}

	if (lck_list_info->next)
		lck_list_info->next->prev = lck_list_info->prev;
	if (lck_list_info->prev)
		lck_list_info->prev->next = lck_list_info->next;
	/* stop the timer if started */
	if (lck_list_info->timeout_tmr.tmr_id != TMR_T_NULL)
		glnd_stop_tmr(&lck_list_info->timeout_tmr);
	ncshm_destroy_hdl(NCS_SERVICE_ID_GLND, lck_list_info->lck_info_hdl_id);

	TRACE("GLND Resource lock req destroy res - %d lockid- %d",
	      (uint32_t)res_info->resource_id, (uint32_t)lck_list_info->lock_info.lockid);

	TRACE("GLND Rsc lock req destroy: resource_id %u, lockid %u", (uint32_t)res_info->resource_id,
				(uint32_t)lck_list_info->lock_info.lockid);
	m_MMGR_FREE_GLND_RES_LOCK_LIST_INFO(lck_list_info);
	return;
}

/*****************************************************************************
    PROCEDURE NAME :glnd_initiate_deadlock_algorithm 

    DESCRIPTION    : 

    ARGUMENTS    :
                   res_info      - ptr to the Resource Node.
                   lock_info     - pointer to the lock info
                   req_node_mds_dest - mds dest for the requesting node sirector

    RETURNS        : pointer to the lock info

    NOTES         : None
*****************************************************************************/
uint32_t glnd_initiate_deadlock_algorithm(GLND_CB *cb,
				       GLND_RESOURCE_INFO *res_info,
				       GLSV_LOCK_REQ_INFO lock_info, GLND_RES_LOCK_LIST_INFO *lck_list_info)
{
	GLSV_GLND_EVT glnd_evt;
	GLSV_GLND_DD_INFO_LIST *dd_info_list = NULL;

	/* triggering deadlock detection .. */
	memset(&glnd_evt, 0, sizeof(GLSV_GLND_EVT));

	/* Add this as the first element on the list... */
	dd_info_list = glnd_evt.info.dd_probe_info.dd_info_list =
	    m_MMGR_ALLOC_GLSV_GLND_DD_INFO_LIST(sizeof(GLSV_GLND_DD_INFO_LIST), NCS_SERVICE_ID_GLND);
	if (dd_info_list == NULL) {
		LOG_CR("GLSV GLND DD INFO LIST memory failure: Error %s", strerror(errno)); 	
		assert(0);	/* GLSV_ADD_LOG_HERE - memory failure */
	} else {
		memset(dd_info_list, 0, sizeof(GLSV_GLND_DD_INFO_LIST));

		/* Fill in the details of the blocked resource... */
		glnd_evt.type = GLSV_GLND_EVT_FWD_DD_PROBE;
		dd_info_list->blck_dest_id = glnd_evt.info.dd_probe_info.dest_id = lck_list_info->req_mdest_id;
		dd_info_list->blck_hdl_id = glnd_evt.info.dd_probe_info.hdl_id = lck_list_info->lock_info.handleId;
		dd_info_list->lck_id = glnd_evt.info.dd_probe_info.lck_id = lck_list_info->lock_info.lockid;
		dd_info_list->rsc_id = glnd_evt.info.dd_probe_info.rsc_id = res_info->resource_id;

		glnd_evt.info.dd_probe_info.lcl_rsc_id = lck_list_info->lcl_resource_id;
		/* Send the probe to the master... that is us... deadlock detection
		   continues in fwd_dd_probe processing ... */
		if (!memcmp(&cb->glnd_mdest_id, &res_info->master_mds_dest, sizeof(MDS_DEST))) {
			GLSV_GLND_EVT *tmp_glnd_evt;
			tmp_glnd_evt = m_MMGR_ALLOC_GLND_EVT;
			if (tmp_glnd_evt == NULL) {
				LOG_CR("GLND evt alloc failed: Error %s", strerror(errno));
				m_MMGR_FREE_GLSV_GLND_DD_INFO_LIST(dd_info_list, NCS_SERVICE_ID_GLND);
				assert(0);
			}
			memset(tmp_glnd_evt, 0, sizeof(GLSV_GLND_EVT));
			*tmp_glnd_evt = glnd_evt;
			glnd_evt_local_send(cb, tmp_glnd_evt, MDS_SEND_PRIORITY_MEDIUM);
		} else if (glnd_mds_msg_send_glnd(cb, &glnd_evt, res_info->master_mds_dest)
			   != NCSCC_RC_SUCCESS) {
			TRACE_2("GLND mds send failure");/* GLSV_ADD_LOG_HERE MDS send failed */
			return NCSCC_RC_FAILURE;
		}
		return NCSCC_RC_SUCCESS;
	}
}

/*****************************************************************************
PROCEDURE NAME : glnd_resource_master_process_lock_req

    DESCRIPTION    : queues the lock request 
  
    ARGUMENTS    : 
                   res_info      - ptr to the Resource Node.
                   lock_info     - pointer to the lock info
                   req_node_mds_dest - mds dest for the requesting node sirector
    
    RETURNS        : pointer to the lock info
      
    NOTES         : None
*****************************************************************************/
GLND_RES_LOCK_LIST_INFO *glnd_resource_master_process_lock_req(GLND_CB *cb,
							       GLND_RESOURCE_INFO *res_info,
							       GLSV_LOCK_REQ_INFO lock_info,
							       MDS_DEST req_node_mds_dest,
							       SaLckResourceIdT lcl_resource_id,
							       SaLckLockIdT lcl_lock_id)
{
	GLND_RES_LOCK_LIST_INFO *lck_list_info = NULL;
	SaLckLockModeT mode;
	GLND_CLIENT_INFO *client_info = NULL;
	GLND_CLIENT_LIST_RESOURCE *client_res_list = NULL;
	
	TRACE_ENTER2("resource id %u lockid %llx", lcl_resource_id, lcl_lock_id);

	client_info = glnd_client_node_find(cb, lock_info.handleId);

	/* Add it to the master info */
	lck_list_info = (GLND_RES_LOCK_LIST_INFO *)m_MMGR_ALLOC_GLND_RES_LOCK_LIST_INFO;
	if (lck_list_info == NULL) {
		LOG_CR("GLND Rsc lock list alloc failed lcl_resource_id: %u lcl_lock_id: %llx Error %s", 
							lcl_resource_id, lcl_lock_id, strerror(errno));
		assert(0);
	}
	memset(lck_list_info, 0, sizeof(GLND_RES_LOCK_LIST_INFO));
	lck_list_info->lck_info_hdl_id =
	    ncshm_create_hdl((uint8_t)cb->pool_id, NCS_SERVICE_ID_GLND, (NCSCONTEXT)lck_list_info);

	lck_list_info->lock_info = lock_info;
	lck_list_info->req_mdest_id = req_node_mds_dest;
	lck_list_info->lcl_resource_id = lcl_resource_id;
	lck_list_info->res_info = res_info;
	if (m_GLND_IS_LOCAL_NODE(&req_node_mds_dest, &cb->glnd_mdest_id) == 0) {
		lck_list_info->lock_info.lockid = m_ASSIGN_LCK_HANDLE_ID(NCS_PTR_TO_UNS64_CAST(lck_list_info));
		glnd_client_node_resource_lock_req_add(client_info, res_info, lck_list_info);
	} else
		lck_list_info->lock_info.lockid = lock_info.lockid;

	/* add it to the list */
	switch (lock_info.lock_type) {
	case SA_LCK_EX_LOCK_MODE:
		/* check if we can add it to the grant list */
		if (res_info->lck_master_info.grant_list == NULL &&
		    glnd_resource_grant_list_orphan_locks(res_info, &mode) == false) {
			/* add it to the grant list */
			TRACE("Lock granted handle - %d res - %d lockid- %d",
			      (uint32_t)lock_info.handleId, (uint32_t)res_info->resource_id,
			      (uint32_t)lck_list_info->lock_info.lockid);

			TRACE("GLND Rsc lock granted: handleId %u, resource_id %u, lockid %u ", 
					(uint32_t)lock_info.handleId, (uint32_t)res_info->resource_id, 
					(uint32_t)lck_list_info->lock_info.lockid);

			lck_list_info->lock_info.lockStatus = SA_LCK_LOCK_GRANTED;
			res_info->lck_master_info.grant_list = lck_list_info;
		} else if ((lock_info.lockFlags & SA_LCK_LOCK_NO_QUEUE) == SA_LCK_LOCK_NO_QUEUE) {
			/* send back the request as it can't be queued */
			lck_list_info->lock_info.lockStatus = SA_LCK_LOCK_NOT_QUEUED;
		} else if (((lock_info.lockFlags & SA_LCK_LOCK_ORPHAN) == SA_LCK_LOCK_ORPHAN) &&
			   glnd_resource_grant_list_orphan_locks(res_info, &mode) == true) {
			/* check if there are any orphan locks in the grant queue */
			/* send back the request as it can't be queued as it is orphaned */
			lck_list_info->lock_info.lockStatus = SA_LCK_LOCK_ORPHANED;
		} else {
			/* add it to the exclusive wait list */
			TRACE("LOCK_QUEUED in EX handle - %d res - %d lockid- %d",
			      (uint32_t)lock_info.handleId, (uint32_t)res_info->resource_id,
			      (uint32_t)lck_list_info->lock_info.lockid);

			TRACE("GLND Rsc lock queued: handleId %u, resource_id %u, lockid %u ", 
					(uint32_t)lock_info.handleId, (uint32_t)res_info->resource_id, 
					(uint32_t)lck_list_info->lock_info.lockid);

			lck_list_info->next = res_info->lck_master_info.wait_exclusive_list;
			if (res_info->lck_master_info.wait_exclusive_list)
				res_info->lck_master_info.wait_exclusive_list->prev = lck_list_info;
			res_info->lck_master_info.wait_exclusive_list = lck_list_info;
			if (m_GLND_IS_LOCAL_NODE(&req_node_mds_dest, &cb->glnd_mdest_id) == 0) {

				glnd_start_tmr(cb, &lck_list_info->timeout_tmr,
					       GLND_TMR_RES_LOCK_REQ_TIMEOUT,
					       lock_info.timeout, (uint32_t)lck_list_info->lck_info_hdl_id);
			}

			/* send back waitercallback requests to the lock holders in grant list */

			glnd_master_process_lock_initiate_waitercallbk(cb, res_info, lock_info, lcl_lock_id);
			if (m_GLND_IS_LOCAL_NODE(&req_node_mds_dest, &cb->glnd_mdest_id) == 0) {
				if (res_info->lck_master_info.grant_list) {
					if (res_info->lck_master_info.grant_list->lock_info.lock_type ==
					    SA_LCK_EX_LOCK_MODE) {
						if (!memcmp(&cb->glnd_mdest_id, &req_node_mds_dest, sizeof(MDS_DEST))) {
							if (lock_info.handleId ==
							    res_info->lck_master_info.grant_list->lock_info.handleId) {
								glnd_initiate_deadlock_algorithm(cb, res_info,
												 lock_info,
												 lck_list_info);
								break;
							}
						}
					}
				}
				if (client_info) {
					if (client_info->res_list) {
						client_res_list = client_info->res_list;
						while (client_res_list != NULL) {
							if (client_res_list->rsc_info->resource_id !=
							    res_info->resource_id) {
								if (!memcmp
								    (&cb->glnd_mdest_id,
								     &client_res_list->rsc_info->master_mds_dest,
								     sizeof(MDS_DEST))) {
									if (client_res_list->rsc_info->
									    lck_master_info.wait_exclusive_list !=
									    NULL) {
										glnd_initiate_deadlock_algorithm(cb,
														 res_info,
														 lock_info,
														 lck_list_info);
										break;
									}
								}
							}
							client_res_list = client_res_list->next;
						}
					}
				}
				if (res_info->lck_master_info.grant_list) {
					if (res_info->lck_master_info.grant_list->lock_info.lock_type ==
					    SA_LCK_PR_LOCK_MODE) {
						if (!memcmp(&cb->glnd_mdest_id, &req_node_mds_dest, sizeof(MDS_DEST))) {
							if (lock_info.handleId ==
							    res_info->lck_master_info.grant_list->lock_info.handleId) {
								glnd_initiate_deadlock_algorithm(cb, res_info,
												 lock_info,
												 lck_list_info);
								break;
							}
						}
					}
				}

			} else
				glnd_initiate_deadlock_algorithm(cb, res_info, lock_info, lck_list_info);
		}
		break;
	case SA_LCK_PR_LOCK_MODE:
		/* check if we can add it to the grant list */
		if (glnd_resource_grant_list_exclusive_locks(res_info) != true &&
		    res_info->lck_master_info.ex_orphaned != true) {
			/* add it in the grant list */
			lck_list_info->lock_info.lockStatus = SA_LCK_LOCK_GRANTED;
			lck_list_info->next = res_info->lck_master_info.grant_list;
			if (res_info->lck_master_info.grant_list)
				res_info->lck_master_info.grant_list->prev = lck_list_info;
			res_info->lck_master_info.grant_list = lck_list_info;

			TRACE("LOCK_GRANTED handle - %d res - %d lockid- %d",
			      (uint32_t)lock_info.handleId, (uint32_t)res_info->resource_id,
			      (uint32_t)lck_list_info->lock_info.lockid);

			TRACE("GLND Rsc lock granted: handleId %u, resource_id %u lockid %u", 
					(uint32_t)lock_info.handleId, (uint32_t)res_info->resource_id, 
					(uint32_t)lck_list_info->lock_info.lockid);
		} else if ((lock_info.lockFlags & SA_LCK_LOCK_NO_QUEUE) == SA_LCK_LOCK_NO_QUEUE) {
			/* send back the request as it can't be queued */
			lck_list_info->lock_info.lockStatus = SA_LCK_LOCK_NOT_QUEUED;
		} else if (((lock_info.lockFlags & SA_LCK_LOCK_ORPHAN) == SA_LCK_LOCK_ORPHAN) &&
			   glnd_resource_grant_list_orphan_locks(res_info, &mode) == true) {
			/* check if there are any orphan locks in the grant queue */
			/* send back the request as it can't be queued as it is orphaned */
			lck_list_info->lock_info.lockStatus = SA_LCK_LOCK_ORPHANED;
		} else {

			TRACE("LOCK_QUEUED handle - %d res - %d lockid- %d",
			      (uint32_t)lock_info.handleId, (uint32_t)res_info->resource_id,
			      (uint32_t)lck_list_info->lock_info.lockid);

			TRACE("GLND Rsc lock queued: handleId %u, resource_id %u, lockid %u", 
					(uint32_t)lock_info.handleId, (uint32_t)res_info->resource_id, 
					(uint32_t)lck_list_info->lock_info.lockid);

			/* add it to the read wait list */
			lck_list_info->next = res_info->lck_master_info.wait_read_list;
			if (res_info->lck_master_info.wait_read_list)
				res_info->lck_master_info.wait_read_list->prev = lck_list_info;
			res_info->lck_master_info.wait_read_list = lck_list_info;
			/* send back waitercallback requests to the lock holders in grant list */
			glnd_master_process_lock_initiate_waitercallbk(cb, res_info, lock_info, lcl_lock_id);

			/* start the timer */
			if (!memcmp(&cb->glnd_mdest_id, &req_node_mds_dest, sizeof(MDS_DEST))) {
				glnd_start_tmr(cb, &lck_list_info->timeout_tmr,
					       GLND_TMR_RES_LOCK_REQ_TIMEOUT,
					       lock_info.timeout, (uint32_t)lck_list_info->lck_info_hdl_id);

				if (client_info) {
					if (client_info->res_list) {

						client_res_list = client_info->res_list;
						while (client_res_list != NULL) {
							if (client_res_list->rsc_info->resource_id !=
							    res_info->resource_id) {
								if (!memcmp
								    (&cb->glnd_mdest_id,
								     &client_res_list->rsc_info->master_mds_dest,
								     sizeof(MDS_DEST))) {
									if (client_res_list->rsc_info->
									    lck_master_info.wait_read_list != NULL)
										glnd_initiate_deadlock_algorithm(cb,
														 res_info,
														 lock_info,
														 lck_list_info);
								}
							}
							client_res_list = client_res_list->next;
						}
					}
				}

				if (res_info->lck_master_info.grant_list) {
					if (res_info->lck_master_info.grant_list->lock_info.lock_type ==
					    SA_LCK_EX_LOCK_MODE) {
						if (!memcmp(&cb->glnd_mdest_id, &req_node_mds_dest, sizeof(MDS_DEST))) {
							if (lock_info.handleId ==
							    res_info->lck_master_info.grant_list->lock_info.handleId) {
								glnd_initiate_deadlock_algorithm(cb, res_info,
												 lock_info,
												 lck_list_info);
								break;
							}
						}
					}
				}

			} else
				glnd_initiate_deadlock_algorithm(cb, res_info, lock_info, lck_list_info);
		}
		break;
	}
	TRACE_LEAVE2("Resource lock id: %u", lck_list_info->lcl_resource_id);
	return lck_list_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_non_master_lock_req

  DESCRIPTION    : queues the lock request and sends it to the master for further
                  processing.

  ARGUMENTS      : cb - pointer to the glnd control block
                  res_info      - ptr to the Resource Node.
                  lock_info     - pointer to the lock info

  RETURNS        : Pointer to the lock node

  NOTES         : None
*****************************************************************************/
GLND_RES_LOCK_LIST_INFO *glnd_resource_non_master_lock_req(GLND_CB *cb,
							   GLND_RESOURCE_INFO *res_info,
							   GLSV_LOCK_REQ_INFO lock_info,
							   SaLckResourceIdT lcl_resource_id, SaLckLockIdT lcl_lock_id)
{
	GLSV_GLND_EVT evt;
	GLND_RES_LOCK_LIST_INFO *lck_list_info;
	GLSV_RESTART_BACKUP_EVT_INFO restart_backup_evt;
	TRACE_ENTER2("lock resource id %u lockid %llx", lcl_resource_id, lcl_lock_id); 

	/* Add it to the non-master info and send the info to master */
	lck_list_info = m_MMGR_ALLOC_GLND_RES_LOCK_LIST_INFO;
	if (lck_list_info == NULL) {
		LOG_CR("GLND Rsc lock list alloc failed: lcl_resource_id %u lcl_lock_id %llxError %s", 
					lcl_resource_id, lcl_lock_id, strerror(errno));
		assert(0);
	}
	memset(lck_list_info, 0, sizeof(GLND_RES_LOCK_LIST_INFO));
	lck_list_info->lock_info = lock_info;
	lck_list_info->lcl_resource_id = lcl_resource_id;
	lck_list_info->res_info = res_info;
	lck_list_info->req_mdest_id = cb->glnd_mdest_id;
	lck_list_info->lck_info_hdl_id =
	    ncshm_create_hdl((uint8_t)cb->pool_id, NCS_SERVICE_ID_GLND, (NCSCONTEXT)lck_list_info);
	lck_list_info->lock_info.lockid = m_ASSIGN_LCK_HANDLE_ID(NCS_PTR_TO_UNS64_CAST(lck_list_info));

	/* add it to the list */
	lck_list_info->next = res_info->lcl_lck_req_info;
	if (res_info->lcl_lck_req_info)
		res_info->lcl_lck_req_info->prev = lck_list_info;
	res_info->lcl_lck_req_info = lck_list_info;

	/* start the timer */
	glnd_start_tmr(cb, &lck_list_info->timeout_tmr,
		       GLND_TMR_RES_NM_LOCK_REQ_TIMEOUT, lock_info.timeout, (uint32_t)lck_list_info->lck_info_hdl_id);

	/*  send the request to the master node director */
	memset(&evt, 0, sizeof(GLSV_GLND_EVT));
	evt.type = GLSV_GLND_EVT_LCK_REQ;

	evt.info.node_lck_info.glnd_mds_dest = cb->glnd_mdest_id;
	evt.info.node_lck_info.client_handle_id = lock_info.handleId;
	evt.info.node_lck_info.waiter_signal = lock_info.waiter_signal;
	evt.info.node_lck_info.lock_type = lock_info.lock_type;
	evt.info.node_lck_info.lockFlags = lock_info.lockFlags;
	evt.info.node_lck_info.resource_id = res_info->resource_id;
	evt.info.node_lck_info.lcl_resource_id = lcl_resource_id;
	evt.info.node_lck_info.lockid = lck_list_info->lock_info.lockid;
	evt.info.node_lck_info.lcl_lockid = lcl_lock_id;
	evt.info.node_lck_info.waiter_signal = lock_info.waiter_signal;

	if (res_info->status != GLND_RESOURCE_ELECTION_IN_PROGESS) {
		glnd_mds_msg_send_glnd(cb, &evt, res_info->master_mds_dest);
		/* Checkpoint */
		glnd_restart_res_lock_list_ckpt_write(cb, lck_list_info, lck_list_info->res_info->resource_id, 0, 1);

	} else {
		glnd_evt_backup_queue_add(cb, &evt);

		memset(&restart_backup_evt, 0, sizeof(GLSV_RESTART_BACKUP_EVT_INFO));

		/* Need to fill all the remaining fields including timer also, and start the timer once it retsrts */
		restart_backup_evt.type = evt.type;
		restart_backup_evt.resource_id = res_info->resource_id;
		restart_backup_evt.lockid = lck_list_info->lock_info.lockid;
		restart_backup_evt.timeout = lck_list_info->lock_info.timeout;
		restart_backup_evt.client_handle_id = lock_info.handleId;
		restart_backup_evt.lock_type = lock_info.lock_type;
		restart_backup_evt.lockFlags = lock_info.lockFlags;

		m_GET_TIME_STAMP(restart_backup_evt.time_stamp);

		glnd_restart_lock_event_info_ckpt_write(cb, restart_backup_evt);
	}

	TRACE_LEAVE2("lock resource id %u", lck_list_info->lcl_resource_id); 	
	return lck_list_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_master_unlock_req

  DESCRIPTION    : queues the unlock request and sends it to the master for further
                  processing.

  ARGUMENTS      :cb - pointer to the glnd control block
                  res_info      - ptr to the Resource Node.
                  lock_info     - pointer to the unlock info
                  req_mds_dest  - The requesting node director mds dest.

  RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
GLND_RES_LOCK_LIST_INFO *glnd_resource_master_unlock_req(GLND_CB *cb,
							 GLND_RESOURCE_INFO *res_info,
							 GLSV_LOCK_REQ_INFO lock_info,
							 MDS_DEST req_mds_dest, SaLckResourceIdT lcl_resource_id)
{
	GLND_RES_LOCK_LIST_INFO *lock_list_info=NULL;
	TRACE_ENTER2("lcl_resource_id %u", lcl_resource_id);

	if (res_info == NULL) {
		lock_list_info = NULL;
		goto done;
	}

	if (res_info->status == GLND_RESOURCE_ACTIVE_MASTER) {
		/* search for the lock info in the queues */
		lock_list_info = glnd_resource_grant_lock_req_find(res_info, lock_info, req_mds_dest, lcl_resource_id);
		if (!lock_list_info) {
			LOG_ER("GLND Rsc grant lock req find failed: lcl_resource_id %u", lcl_resource_id);
			lock_list_info = NULL;
			goto done;
		} else {
			TRACE("UNLOCK_DONE handle - %d res - %d lockid- %d",
			      (uint32_t)lock_info.handleId, (uint32_t)res_info->resource_id, (uint32_t)lock_info.lockid);

			TRACE_1("GLND Rsc unlock success: handleId %u, resource_id %u, lockid %u", 
					(uint32_t)lock_info.handleId, (uint32_t)res_info->resource_id, 
					(uint32_t)lock_info.lockid);

			/* set the value of the lock status */
			lock_list_info->lock_info.lockStatus = GLSV_LOCK_STATUS_RELEASED;
		}
	}
 done:
	TRACE_LEAVE();
	return lock_list_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_non_master_unlock_req

  DESCRIPTION    : queues the unlock request and sends it to the master for further
                  processing.

  ARGUMENTS      :cb - pointer to the glnd control block
                  res_info      - ptr to the Resource Node.
                  lock_info     - pointer to the unlock info
                  req_mds_dest  - The requesting node director mds dest.

  RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
GLND_RES_LOCK_LIST_INFO *glnd_resource_non_master_unlock_req(GLND_CB *cb,
							     GLND_RESOURCE_INFO *res_info,
							     GLSV_LOCK_REQ_INFO lock_info,
							     SaLckResourceIdT lcl_resource_id, SaLckLockIdT lcl_lock_id)
{
	GLSV_GLND_EVT evt;
	GLND_RES_LOCK_LIST_INFO *lck_list_info;
	GLSV_RESTART_BACKUP_EVT_INFO restart_backup_evt;
	uint32_t shm_index;
	TRACE_ENTER2("lcl_resource_id %u", lcl_resource_id);

	/* search for the lock info in the queues */
	lck_list_info =
	    glnd_resource_local_lock_req_find(res_info, lock_info.lcl_lockid, lock_info.handleId, lcl_resource_id);
	if (lck_list_info == NULL) {
		LOG_ER("GLND Rsc local lock req find failed");
		lck_list_info = NULL;
		goto done;
	}

	/* start the timer */
	glnd_start_tmr(cb, &lck_list_info->timeout_tmr,
		       GLND_TMR_RES_NM_UNLOCK_REQ_TIMEOUT, lock_info.timeout, (uint32_t)lck_list_info->lck_info_hdl_id);

	/*  send the request to the master node director */
	memset(&evt, 0, sizeof(GLSV_GLND_EVT));
	evt.type = GLSV_GLND_EVT_UNLCK_REQ;

	evt.info.node_lck_info.glnd_mds_dest = cb->glnd_mdest_id;
	evt.info.node_lck_info.client_handle_id = lock_info.handleId;
	evt.info.node_lck_info.lockid = lock_info.lockid;
	evt.info.node_lck_info.lcl_lockid = lcl_lock_id;
	evt.info.node_lck_info.resource_id = res_info->resource_id;
	evt.info.node_lck_info.lcl_resource_id = lcl_resource_id;
	evt.info.node_lck_info.invocation = lock_info.invocation;

	if (res_info->status != GLND_RESOURCE_ELECTION_IN_PROGESS)
		glnd_mds_msg_send_glnd(cb, &evt, res_info->master_mds_dest);
	else {
		memset(&restart_backup_evt, 0, sizeof(GLSV_RESTART_BACKUP_EVT_INFO));

		/* Find valid sections to write res info in the shared memory  */
		glnd_find_res_shm_ckpt_empty_section(cb, &shm_index);
		restart_backup_evt.shm_index = evt.shm_index = shm_index;

		glnd_evt_backup_queue_add(cb, &evt);

		/* Need to fill all the remaining fields including timer also, and start the timer once it retsrts */
		restart_backup_evt.type = evt.type;
		restart_backup_evt.resource_id = res_info->resource_id;
		restart_backup_evt.lockid = lck_list_info->lock_info.lockid;
		restart_backup_evt.timeout = lck_list_info->lock_info.timeout;
		restart_backup_evt.client_handle_id = lock_info.handleId;
		restart_backup_evt.lock_type = lock_info.lock_type;
		restart_backup_evt.lockFlags = lock_info.lockFlags;

		m_GET_TIME_STAMP(restart_backup_evt.time_stamp);

		glnd_restart_lock_event_info_ckpt_write(cb, restart_backup_evt);

	}
 done:
	TRACE_LEAVE();
	return lck_list_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_grant_list_orphan_locks

  DESCRIPTION    : Checks to see if any orphan locks are present in the grant list
  
  ARGUMENTS      : res_info      - ptr to the Resource Node.
                   mode          - the mode of the orphan lock.  

  RETURNS        : true/false

  NOTES         : None
*****************************************************************************/
bool glnd_resource_grant_list_orphan_locks(GLND_RESOURCE_INFO *res_info, SaLckLockModeT *mode)
{
	if (res_info->lck_master_info.ex_orphaned == true) {
		*mode = SA_LCK_EX_LOCK_MODE;
		return true;
	}
	if (res_info->lck_master_info.pr_orphaned == true) {
		*mode = SA_LCK_PR_LOCK_MODE;
		return true;
	}
	return false;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_grant_list_exclusive_locks

  DESCRIPTION    : Checks to see if any exclusive locks are present in the grant list
  ARGUMENTS      : 
                  res_info      - ptr to the Resource Node.
                 

  RETURNS        : true/false

  NOTES         : None
*****************************************************************************/
static bool glnd_resource_grant_list_exclusive_locks(GLND_RESOURCE_INFO *res_info)
{
	GLND_RES_LOCK_LIST_INFO *lock_list_info;

	for (lock_list_info = res_info->lck_master_info.grant_list; lock_list_info != NULL;
	     lock_list_info = lock_list_info->next) {
		if (lock_list_info->lock_info.lock_type == SA_LCK_EX_LOCK_MODE)
			return true;
	}
	return false;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_master_process_lock_initiate_waitercallbk

  DESCRIPTION    : initiates the waitercallback mechanism

  ARGUMENTS      :cb - ptr to the GLND control block 
                  res_info      - ptr to the Resource Node.
                 
  RETURNS        : true/false

  NOTES         : None
*****************************************************************************/
static void glnd_master_process_lock_initiate_waitercallbk(GLND_CB *cb,
							   GLND_RESOURCE_INFO *res_info,
							   GLSV_LOCK_REQ_INFO lock_info, SaLckLockIdT lcl_lock_id)
{
	GLND_RES_LOCK_LIST_INFO *lock_list_info;

	for (lock_list_info = res_info->lck_master_info.grant_list; lock_list_info != NULL;
	     lock_list_info = lock_list_info->next) {

		if (m_GLND_IS_LOCAL_NODE(&lock_list_info->req_mdest_id, &cb->glnd_mdest_id)) {
			/* send to the corresponding GLND the waitercallback evt */
			GLSV_GLND_EVT glnd_evt;
			m_GLND_RESOURCE_NODE_LCK_INFO_FILL(glnd_evt, GLSV_GLND_EVT_LCK_WAITER_CALLBACK,
							   res_info->resource_id,
							   lock_list_info->lcl_resource_id,
							   lock_list_info->lock_info.handleId,
							   lock_list_info->lock_info.lockid,
							   lock_info.lock_type,
							   lock_list_info->lock_info.lockFlags,
							   lock_list_info->lock_info.lockStatus,
							   lock_info.waiter_signal,
							   0, 0, lock_list_info->lock_info.lcl_lockid, 0);
			glnd_mds_msg_send_glnd(cb, &glnd_evt, lock_list_info->req_mdest_id);

		} else {
			GLSV_GLA_EVT gla_evt;
			GLND_CLIENT_INFO *client_info;
			/* send it to the local GLA component */
			client_info = glnd_client_node_find(cb, lock_list_info->lock_info.handleId);
			if (client_info) {
				if ((client_info->cbk_reg_info & GLSV_LOCK_WAITER_CBK_REG) == GLSV_LOCK_WAITER_CBK_REG) {
					m_GLND_RESOURCE_ASYNC_LCK_WAITER_FILL(gla_evt,
									      lock_list_info->lock_info.lockid,
									      lock_list_info->lcl_resource_id,
									      lock_list_info->lock_info.invocation,
									      lock_list_info->lock_info.lock_type,
									      lock_info.lock_type,
									      client_info->app_handle_id,
									      lock_info.waiter_signal,
									      lock_list_info->lock_info.lcl_lockid);

					/* send it to GLA */
					glnd_mds_msg_send_gla(cb, &gla_evt, client_info->agent_mds_dest);
				}
			}
		}
	}
	return;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_master_lock_purge_req

  DESCRIPTION    : Purges all the orphan granted locks.

  ARGUMENTS      : glnd_cb      - ptr to the Control Block
                   res_info     - pointer to the resource info
                   is_local  - true/false

  RETURNS        : None.

  NOTES         : None
*****************************************************************************/
void glnd_resource_master_lock_purge_req(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info, bool is_local)
{
	bool orphan = true;

	if (res_info->lck_master_info.ex_orphaned == true) {
		res_info->lck_master_info.ex_orphaned = false;

		if (is_local == true && res_info->lck_master_info.ex_orphan_req_count == 0) {
			/* send notification to the GLD about the shared locks */
			GLSV_GLD_EVT gld_evt;
			memset(&gld_evt, 0, sizeof(GLSV_GLD_EVT));
			m_GLND_RESOURCE_LCK_FILL(gld_evt, GLSV_GLD_EVT_SET_ORPHAN,
						 res_info->resource_id, false, SA_LCK_EX_LOCK_MODE);
			glnd_mds_msg_send_gld(glnd_cb, &gld_evt, glnd_cb->gld_mdest_id);
		}
	}
	if (res_info->lck_master_info.pr_orphaned == true) {
		res_info->lck_master_info.pr_orphaned = false;

		if (is_local == true && res_info->lck_master_info.pr_orphan_req_count == 0) {
			/* send notification to the GLD about the shared locks */
			GLSV_GLD_EVT gld_evt;
			memset(&gld_evt, 0, sizeof(GLSV_GLD_EVT));
			m_GLND_RESOURCE_LCK_FILL(gld_evt, GLSV_GLD_EVT_SET_ORPHAN,
						 res_info->resource_id, false, SA_LCK_PR_LOCK_MODE);
			glnd_mds_msg_send_gld(glnd_cb, &gld_evt, glnd_cb->gld_mdest_id);
		}
	}

	glnd_restart_resource_info_ckpt_overwrite(glnd_cb, res_info);
	if (res_info->lcl_ref_cnt == 0) {
		glnd_resource_node_destroy(glnd_cb, res_info, orphan);
	} else {
		if (res_info->status == GLND_RESOURCE_ACTIVE_MASTER) {
			/* do the re sync of the grant list */
			glnd_resource_master_lock_resync_grant_list(glnd_cb, res_info);
		}
	}
	return;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_master_grant_lock_send_notification

  DESCRIPTION    : sends notifications to the lock requester

  ARGUMENTS      : glnd_cb - ptr to the control block
                   res_info      - ptr to the Resource Node.
                   m_node - ptr to the lock node.  

  RETURNS        : none

  NOTES         : None
*****************************************************************************/
static void glnd_resource_master_grant_lock_send_notification(GLND_CB *glnd_cb,
							      GLND_RESOURCE_INFO *res_info,
							      GLND_RES_LOCK_LIST_INFO *lock_list_node)
{
	GLSV_GLND_EVT glnd_evt;
	GLSV_GLA_EVT gla_evt;

	TRACE("LOCK_GRANTED handle - %d res - %d lockid- %d",
	      (uint32_t)lock_list_node->lock_info.handleId,
	      (uint32_t)res_info->resource_id, (uint32_t)lock_list_node->lock_info.lockid);

	TRACE("GLND Rsc lock granted: handleId %u, resource_id %u, lockid %u", 
			(uint32_t)lock_list_node->lock_info.handleId, (uint32_t)res_info->resource_id, 
			(uint32_t)lock_list_node->lock_info.lockid);

	if (m_GLND_IS_LOCAL_NODE(&lock_list_node->req_mdest_id, &glnd_cb->glnd_mdest_id) == 0) {
		/* local master */
		if ((lock_list_node->lock_info.lockFlags & SA_LCK_LOCK_ORPHAN) == SA_LCK_LOCK_ORPHAN) {
			glnd_resource_lock_req_set_orphan(glnd_cb, res_info, lock_list_node->lock_info.lock_type);
		}
		if (lock_list_node->lock_info.call_type == GLSV_SYNC_CALL) {
			m_GLND_RESOURCE_SYNC_LCK_GRANT_FILL(gla_evt, SA_AIS_OK,
							    lock_list_node->lock_info.lockid,
							    lock_list_node->lock_info.lockStatus,
							    lock_list_node->lock_info.handleId);
			/* send the evt to GLA */
			glnd_mds_msg_send_rsp_gla(glnd_cb, &gla_evt,
						  lock_list_node->lock_info.agent_mds_dest,
						  &lock_list_node->glnd_res_lock_mds_ctxt);
		} else {
			m_GLND_RESOURCE_ASYNC_LCK_GRANT_FILL(gla_evt, SA_AIS_OK,
							     lock_list_node->lock_info.lockid,
							     lock_list_node->lock_info.lcl_lockid,
							     lock_list_node->lock_info.lock_type,
							     lock_list_node->lcl_resource_id,
							     lock_list_node->lock_info.invocation,
							     lock_list_node->lock_info.lockStatus,
							     lock_list_node->lock_info.handleId);
			glnd_mds_msg_send_gla(glnd_cb, &gla_evt, lock_list_node->lock_info.agent_mds_dest);
		}
	} else {
		/* send notification back to non-master glnd */
		m_GLND_RESOURCE_NODE_LCK_INFO_FILL(glnd_evt, GLSV_GLND_EVT_LCK_RSP,
						   res_info->resource_id,
						   lock_list_node->lcl_resource_id,
						   lock_list_node->lock_info.handleId,
						   lock_list_node->lock_info.lockid,
						   lock_list_node->lock_info.lock_type,
						   lock_list_node->lock_info.lockFlags,
						   lock_list_node->lock_info.lockStatus,
						   0, 0, SA_AIS_OK, lock_list_node->lock_info.lcl_lockid, 0);
		glnd_evt.info.node_lck_info.glnd_mds_dest = glnd_cb->glnd_mdest_id;

		/* send the response evt to GLND */
		glnd_mds_msg_send_glnd(glnd_cb, &glnd_evt, lock_list_node->req_mdest_id);
	}
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_master_move_pr_locks_to_grant_list

  DESCRIPTION    : Moves the lock request from wait queue to grant queue

  ARGUMENTS      : glnd_cb - ptr to the control block
                   res_info      - ptr to the Resource Node.

  RETURNS        : returns pointer to the master info node.

  NOTES         : None
*****************************************************************************/
static void glnd_resource_master_move_pr_locks_to_grant_list(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info)
{
	GLND_RES_LOCK_LIST_INFO *m_node, *pr_node;

	for (pr_node = res_info->lck_master_info.wait_read_list; pr_node != NULL;) {
		m_node = pr_node;
		pr_node = pr_node->next;

		/* change the status */
		m_node->lock_info.lockStatus = SA_LCK_LOCK_GRANTED;

		/* add it to the grant list */
		m_node->prev = NULL;
		m_node->next = res_info->lck_master_info.grant_list;
		if (res_info->lck_master_info.grant_list)
			res_info->lck_master_info.grant_list->prev = m_node;
		res_info->lck_master_info.grant_list = m_node;

		/* stop the local lock timer */
		glnd_stop_tmr(&m_node->timeout_tmr);

		/* send notification to the lock requesters */
		glnd_resource_master_grant_lock_send_notification(glnd_cb, res_info, m_node);
	}

	/* 
	   if ( res_info->lck_master_info.wait_exclusive_list )
	   m_MMGR_FREE_GLND_RES_LOCK_LIST_INFO(res_info->lck_master_info.wait_read_list);
	 */
	res_info->lck_master_info.wait_read_list = NULL;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_master_move_ex_locks_to_grant_list

  DESCRIPTION    : Moves the lock request from wait queue to grant queue

  ARGUMENTS      : glnd_cb - ptr to the control block
                   res_info      - ptr to the Resource Node.

  RETURNS        : returns pointer to the master info node.

  NOTES         : None
*****************************************************************************/
static void glnd_resource_master_move_ex_locks_to_grant_list(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info)
{
	GLND_RES_LOCK_LIST_INFO *ex_node;
	for (ex_node = res_info->lck_master_info.wait_exclusive_list;
	     ex_node != NULL && ex_node->next != NULL; ex_node = ex_node->next) ;

	if (ex_node) {
		/* remove it from the list */
		if (res_info->lck_master_info.wait_exclusive_list == ex_node)
			res_info->lck_master_info.wait_exclusive_list = ex_node->next;
		if (ex_node->next)
			ex_node->next->prev = ex_node->prev;
		if (ex_node->prev)
			ex_node->prev->next = ex_node->next;
		/* stop the local lock timer */
		glnd_stop_tmr(&ex_node->timeout_tmr);

		/* add it to the grant list */
		ex_node->prev = NULL;
		ex_node->next = res_info->lck_master_info.grant_list;
		if (res_info->lck_master_info.grant_list)
			res_info->lck_master_info.grant_list->prev = ex_node;
		res_info->lck_master_info.grant_list = ex_node;

		/* change the status and send notification to the lock requester */
		ex_node->lock_info.lockStatus = SA_LCK_LOCK_GRANTED;

		/* send the notification */
		glnd_resource_master_grant_lock_send_notification(glnd_cb, res_info, ex_node);
	}
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_master_lock_resync_grant_list

  DESCRIPTION    : Moves the lock request from wait queue to grant queue

  ARGUMENTS      : res_info      - ptr to the Resource Node.

  RETURNS        : returns pointer to the master info node.

  NOTES         : None
*****************************************************************************/
void glnd_resource_master_lock_resync_grant_list(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info)
{
	if (!res_info)
		return;

	/* check and see if any orphan locks are present */
	if (res_info->lck_master_info.ex_orphaned == true)
		return;

	if (res_info->lck_master_info.grant_list == NULL && res_info->lck_master_info.pr_orphaned == false) {
		/* can move the exclusive lock list into the grant list */
		if (res_info->lck_master_info.wait_exclusive_list) {
			if (res_info->lck_master_info.wait_exclusive_list->res_info == res_info)
				glnd_resource_master_move_ex_locks_to_grant_list(glnd_cb, res_info);
		}
	}
	/* check to see if we move pr locks */
	if (glnd_resource_grant_list_exclusive_locks(res_info) != true) {
		if (res_info->lck_master_info.wait_read_list) {
			if (res_info->lck_master_info.wait_read_list->res_info == res_info)
				glnd_resource_master_move_pr_locks_to_grant_list(glnd_cb, res_info);
		}
	}
	return;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_convert_nonmaster_to_master

  DESCRIPTION    : Converts the non-master info to the master info as part of 
                   relection of master

  ARGUMENTS      :cb - pointer to the glnd control block
                  res_info      - ptr to the Resource Node.

  RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
void glnd_resource_convert_nonmaster_to_master(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_node)
{
	GLND_RES_LOCK_LIST_INFO *lck_list_nm_info;
	GLND_RES_LOCK_LIST_INFO *lck_list_m_info;
	int64_t remaining_time = 0;
	SaTimeT time;

	TRACE_ENTER();

	for (lck_list_nm_info = res_node->lcl_lck_req_info; lck_list_nm_info != NULL;) {
		TRACE("lockStatus: %d  Lock Id : %d   Node Id : %d  App Handle : %d  Lcl Resource Id: %u",
		     lck_list_nm_info->lock_info.lockStatus,
                     (uint32_t)lck_list_nm_info->lock_info.lockid, m_NCS_NODE_ID_FROM_MDS_DEST(lck_list_nm_info->req_mdest_id), (uint32_t)lck_list_nm_info->lock_info.handleId, lck_list_nm_info->lcl_resource_id);

		lck_list_m_info = lck_list_nm_info;
		lck_list_nm_info = lck_list_nm_info->next;

		lck_list_m_info->prev = NULL;
		lck_list_m_info->next = NULL;
		lck_list_m_info->req_mdest_id = glnd_cb->glnd_mdest_id;

		if (lck_list_m_info->timeout_tmr.tmr_id != TMR_T_NULL) {
			m_NCS_TMR_MSEC_REMAINING(&lck_list_m_info->timeout_tmr.tmr_id, &remaining_time);
			if (remaining_time > 0) {
				glnd_stop_tmr(&lck_list_m_info->timeout_tmr);
				/* start the timer in master mode */
				if (lck_list_m_info->timeout_tmr.type == GLND_TMR_RES_NM_LOCK_REQ_TIMEOUT) {
					time = (SaTimeT)remaining_time;
					time = time * 100000;
					glnd_start_tmr(glnd_cb, &lck_list_m_info->timeout_tmr,
						       GLND_TMR_RES_LOCK_REQ_TIMEOUT,
						       time, (uint32_t)lck_list_m_info->lck_info_hdl_id);
					/* convert the remaining time to satimeT millsec to nanosec */
				} else if (lck_list_m_info->timeout_tmr.type == GLND_TMR_RES_NM_UNLOCK_REQ_TIMEOUT) {
					/* TBD: start the master unlock timer */
				}
			}
		}
		/* check to see the status of the lock */
		if (lck_list_m_info->lock_info.lockStatus == SA_LCK_LOCK_GRANTED) {
			/* move it to the grant queue */
			lck_list_m_info->next = res_node->lck_master_info.grant_list;
			if (res_node->lck_master_info.grant_list)
				res_node->lck_master_info.grant_list->prev = lck_list_m_info;
			res_node->lck_master_info.grant_list = lck_list_m_info;

			/* take care of the orphan locks */
			if ((lck_list_m_info->lock_info.lockFlags & SA_LCK_LOCK_ORPHAN) == SA_LCK_LOCK_ORPHAN) {
				glnd_resource_lock_req_set_orphan(glnd_cb, res_node,
								  lck_list_m_info->lock_info.lock_type);
			}
		} else if (lck_list_m_info->lock_info.lock_type == SA_LCK_EX_LOCK_MODE) {
			/* move it to the exclusive wait list */
			lck_list_m_info->next = res_node->lck_master_info.wait_exclusive_list;
			if (res_node->lck_master_info.wait_exclusive_list)
				res_node->lck_master_info.wait_exclusive_list->prev = lck_list_m_info;
			res_node->lck_master_info.wait_exclusive_list = lck_list_m_info;
		} else {
			/* move it to the read wait list */
			lck_list_m_info->next = res_node->lck_master_info.wait_read_list;
			if (res_node->lck_master_info.wait_read_list)
				res_node->lck_master_info.wait_read_list->prev = lck_list_m_info;
			res_node->lck_master_info.wait_read_list = lck_list_m_info;
		}

	}
	res_node->lcl_lck_req_info = NULL;

	TRACE_LEAVE();
	return;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_resend_nonmaster_info_to_newmaster

  DESCRIPTION    : resend the local info to the master node director

  ARGUMENTS      :cb - pointer to the glnd control block
                  res_info      - ptr to the Resource Node.

  RETURNS        : none

  NOTES         : None
*****************************************************************************/
void glnd_resource_resend_nonmaster_info_to_newmaster(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_node)
{
	GLND_RES_LOCK_LIST_INFO *lck_list_nm_info;
	GLSV_GLND_EVT glnd_evt;
	GLND_LOCK_LIST_INFO *lck_list_info = NULL, *new_list_info = NULL, *tmp_list = NULL;
	SaLckResourceIdT lcl_resource_id = 0;
	bool unlock_req_sent = false;
	uint32_t count = 0;

	TRACE_ENTER();

	for (lck_list_nm_info = res_node->lcl_lck_req_info; lck_list_nm_info != NULL;
	     lck_list_nm_info = lck_list_nm_info->next) {
		/* prepare the lock list */
		new_list_info = m_MMGR_ALLOC_GLSV_GLND_LOCK_LIST_INFO(sizeof(GLND_LOCK_LIST_INFO), NCS_SERVICE_ID_GLND);
		if (!new_list_info) {
			LOG_CR("GLND lock list alloc failed: Error %s", strerror(errno));
			goto err;
		}
		lcl_resource_id = lck_list_nm_info->lcl_resource_id;
		unlock_req_sent = lck_list_nm_info->unlock_req_sent;
		TRACE("setting lcl_resource_id: %u unlock_req_sent: %i", lcl_resource_id, unlock_req_sent);
		new_list_info->lock_info = lck_list_nm_info->lock_info;
		TRACE("lcl_lckid: %llu", lck_list_nm_info->lock_info.lcl_lockid);
		/* add it to the list */
		new_list_info->next = lck_list_info;
		lck_list_info = new_list_info;
		count++;
	}
	/* send the info to the new master glnd  */
	memset(&glnd_evt, 0, sizeof(GLSV_GLND_EVT));
	glnd_evt.type = GLSV_GLND_EVT_SND_RSC_INFO;
	glnd_evt.info.node_rsc_info.resource_id = res_node->resource_id;
	glnd_evt.info.node_rsc_info.lcl_resource_id = lcl_resource_id;
	glnd_evt.info.node_rsc_info.unlock_req_sent = unlock_req_sent;
	TRACE("sending lcl_resource_id: %u unlock_req_sent: %i", lcl_resource_id, unlock_req_sent);
	glnd_evt.info.node_rsc_info.num_requests = count;
	glnd_evt.info.node_rsc_info.list_of_req = lck_list_info;
	glnd_evt.info.node_rsc_info.glnd_mds_dest = glnd_cb->glnd_mdest_id;

	glnd_mds_msg_send_glnd(glnd_cb, &glnd_evt, res_node->master_mds_dest);

	/* free the memory */
	while (lck_list_info) {
		tmp_list = lck_list_info;
		lck_list_info = lck_list_info->next;
		m_MMGR_FREE_GLSV_GLND_LOCK_LIST_INFO(tmp_list, NCS_SERVICE_ID_GLND);
	}
	TRACE_LEAVE();
	return;
 err:
	while (lck_list_info) {
		tmp_list = lck_list_info;
		lck_list_info = lck_list_info->next;
		m_MMGR_FREE_GLND_LOCK_LIST_INFO(tmp_list);
	}
	assert(0);
	TRACE_LEAVE();
	return;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_master_process_resend_lock_req

  DESCRIPTION    : Processes the lock requests that were resend due to new master election

  ARGUMENTS      :cb - pointer to the glnd control block
                  res_info      - ptr to the Resource Node.
                  lock_info  - lock info

  RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
void glnd_resource_master_process_resend_lock_req(GLND_CB *glnd_cb,
						  GLND_RESOURCE_INFO *res_node,
						  GLSV_LOCK_REQ_INFO lock_info,
						  SaLckResourceIdT lcl_resource_id,
						  bool unlock_req_sent,
						  MDS_DEST req_node_mds_id)
{
	GLND_RES_LOCK_LIST_INFO *lck_list_info;

	lck_list_info = (GLND_RES_LOCK_LIST_INFO *)m_MMGR_ALLOC_GLND_RES_LOCK_LIST_INFO;
	if (lck_list_info == NULL) {
		LOG_CR("GLND Rsc lock list alloc failed: Error %s", strerror(errno));
		assert(0);
	}
	TRACE_ENTER();

	memset(lck_list_info, 0, sizeof(GLND_RES_LOCK_LIST_INFO));

	lck_list_info->lck_info_hdl_id =
	    ncshm_create_hdl((uint8_t)glnd_cb->pool_id, NCS_SERVICE_ID_GLND, (NCSCONTEXT)lck_list_info);
	lck_list_info->lock_info = lock_info;
	lck_list_info->req_mdest_id = req_node_mds_id;
	lck_list_info->res_info = res_node;
	lck_list_info->lcl_resource_id = lcl_resource_id;
	lck_list_info->unlock_req_sent = unlock_req_sent;
	TRACE("lck_list_info->lcl_resource_id: %u lck_list_info->unlock_req_sent: %i",
		lcl_resource_id,
		unlock_req_sent);

	/* check to see the status of the lock */
	if (lock_info.lockStatus == SA_LCK_LOCK_GRANTED) {
		/* move it to the grant queue */
		lck_list_info->next = res_node->lck_master_info.grant_list;
		if (res_node->lck_master_info.grant_list)
			res_node->lck_master_info.grant_list->prev = lck_list_info;
		res_node->lck_master_info.grant_list = lck_list_info;

	} else if (lock_info.lock_type == SA_LCK_EX_LOCK_MODE) {
		/* move it to the exclusive wait list */
		lck_list_info->next = res_node->lck_master_info.wait_exclusive_list;
		if (res_node->lck_master_info.wait_exclusive_list)
			res_node->lck_master_info.wait_exclusive_list->prev = lck_list_info;
		res_node->lck_master_info.wait_exclusive_list = lck_list_info;
	} else {
		/* move it to the read wait list */
		lck_list_info->next = res_node->lck_master_info.wait_read_list;
		if (res_node->lck_master_info.wait_read_list)
			res_node->lck_master_info.wait_read_list->prev = lck_list_info;
		res_node->lck_master_info.wait_read_list = lck_list_info;
	}
	TRACE_LEAVE();
	return;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_deadlock_detect

  DESCRIPTION    : This function will run through the list of resource in the
                   Deadlock probe list and matches it against the blocked rsc
                   specified, if the resource is present in the list then a
                   deadlock has happened. The function will also take the 
                   responsibilty of calling of the lock request

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  client_info  - Client to which the probe was directed
                  dd_probe     - The DD probe that has been received

  RETURNS        :TRUE - Deadlock has been detected

  NOTES         : 
*****************************************************************************/
bool glnd_deadlock_detect(GLND_CB *glnd_cb, GLND_CLIENT_INFO *client_info, GLSV_EVT_GLND_DD_PROBE_INFO *dd_probe)
{
	GLSV_GLND_DD_INFO_LIST *dd_info_list = NULL;
	GLND_CLIENT_LIST_RESOURCE *client_res_list = NULL;
	GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *lck_req_info = NULL;
	bool deadlock_present = false, ignore_probe = false;
	GLSV_GLA_EVT gla_evt;
	GLSV_GLND_EVT glnd_evt;

	dd_info_list = dd_probe->dd_info_list;
	while (dd_info_list != NULL) {
		if ((client_info->app_handle_id == dd_info_list->blck_hdl_id) &&
		    m_NCS_NODE_ID_FROM_MDS_DEST(glnd_cb->glnd_mdest_id) ==
		    m_NCS_NODE_ID_FROM_MDS_DEST(dd_info_list->blck_dest_id)) {
			client_res_list = client_info->res_list;
			while (client_res_list != NULL) {
				if (client_res_list->rsc_info->resource_id == dd_info_list->rsc_id) {
					lck_req_info = client_res_list->lck_list;
					while (lck_req_info != NULL) {
						if (lck_req_info->lck_req->lock_info.lockid == dd_info_list->lck_id) {
							/* Now we have out request queued up in the probe, Logically this request
							   should still be blocked */
							if (lck_req_info->lck_req->lock_info.lockStatus !=
							    SA_LCK_LOCK_GRANTED) {
								lck_req_info->lck_req->lock_info.lockStatus =
								    SA_LCK_LOCK_DEADLOCK;
								deadlock_present = true;
							} else
								ignore_probe = true;
							break;
						}
						lck_req_info = lck_req_info->next;
					}
				}
				if (!deadlock_present && !ignore_probe)
					client_res_list = client_res_list->next;
				else
					break;
			}
		}

		if (!deadlock_present && !ignore_probe)
			dd_info_list = dd_info_list->next;
		else
			break;
	}

	if (deadlock_present) {
		glnd_stop_tmr(&lck_req_info->lck_req->timeout_tmr);

		/* Step 1, send an event to GLA informing him of the deadlock */
		if (lck_req_info->lck_req->lock_info.call_type == GLSV_SYNC_CALL) {
			m_GLND_RESOURCE_SYNC_LCK_GRANT_FILL(gla_evt, SA_AIS_OK,
							    lck_req_info->lck_req->lock_info.lcl_lockid,
							    lck_req_info->lck_req->lock_info.lockStatus,
							    lck_req_info->lck_req->lock_info.handleId);
			glnd_mds_msg_send_rsp_gla(glnd_cb, &gla_evt,
						  client_info->agent_mds_dest,
						  &lck_req_info->lck_req->glnd_res_lock_mds_ctxt);
		} else {
			m_GLND_RESOURCE_ASYNC_LCK_GRANT_FILL(gla_evt, SA_AIS_OK,
							     lck_req_info->lck_req->lock_info.lockid,
							     lck_req_info->lck_req->lock_info.lcl_lockid,
							     lck_req_info->lck_req->lock_info.lock_type,
							     lck_req_info->lck_req->lcl_resource_id,
							     lck_req_info->lck_req->lock_info.invocation,
							     lck_req_info->lck_req->lock_info.lockStatus,
							     lck_req_info->lck_req->lock_info.handleId);
			glnd_mds_msg_send_gla(glnd_cb, &gla_evt, lck_req_info->lck_req->lock_info.agent_mds_dest);
		}

		/*  step 2: send the request to the master node director */
		memset(&glnd_evt, 0, sizeof(GLSV_GLND_EVT));
		m_GLND_RESOURCE_NODE_LCK_INFO_FILL(glnd_evt, GLSV_GLND_EVT_LCK_REQ_CANCEL,
						   lck_req_info->lck_req->res_info->resource_id,
						   lck_req_info->lck_req->lcl_resource_id,
						   lck_req_info->lck_req->lock_info.handleId,
						   lck_req_info->lck_req->lock_info.lockid, 0, 0, 0, 0, 0, 0,
						   lck_req_info->lck_req->lock_info.lcl_lockid, 0);
		glnd_evt.info.node_lck_info.glnd_mds_dest = lck_req_info->lck_req->req_mdest_id;
		if (memcmp(&glnd_cb->glnd_mdest_id, &lck_req_info->lck_req->res_info->master_mds_dest,
			   sizeof(MDS_DEST))) {
			if (glnd_mds_msg_send_glnd(glnd_cb, &glnd_evt, lck_req_info->lck_req->res_info->master_mds_dest)
			    != NCSCC_RC_SUCCESS) {
				TRACE_2("GLND mds send failure");/* GLSV_ADD_LOG_HERE MDS send failed */
			}

		}

		/* Step 3, The information relating to this lock id needs to be cleaned up */
		glnd_resource_lock_req_delete(lck_req_info->lck_req->res_info, lck_req_info->lck_req);
		/* Step 4, we are done with the all necessary clean up, Now go clean up the lck_req
		   in the client info */
		glnd_client_node_resource_lock_req_del(client_info, client_res_list, lck_req_info);
	}

	if (deadlock_present || ignore_probe)
		return true;
	else
		return false;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_resource_check_lost_unlock_requests

  DESCRIPTION    : resend the local info to the master node director

  ARGUMENTS      :cb - pointer to the glnd control block
                  res_info      - ptr to the Resource Node.

  RETURNS        : none

  NOTES         : check for the lost unlock requests and generate 
                  proper unlock response to cleanup those unlock 
                  requests.
*****************************************************************************/
void glnd_resource_check_lost_unlock_requests(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_node)
{
	GLND_RES_LOCK_LIST_INFO *lck_list_nm_info;
	GLSV_GLND_EVT *glnd_evt;

	TRACE_ENTER();

	for (lck_list_nm_info = res_node->lck_master_info.grant_list;
	     lck_list_nm_info != NULL; lck_list_nm_info = lck_list_nm_info->next) {
		/* check for unlock sent flag */
		if (lck_list_nm_info->unlock_req_sent == true) {
			/* generate the unlck rsp event */
			glnd_evt = m_MMGR_ALLOC_GLND_EVT;
			if (glnd_evt == NULL) {
				LOG_CR("GLND evt alloc failed: Error %s", strerror(errno));
				return;
			}
			memset(glnd_evt, 0, sizeof(GLSV_GLND_EVT));
			glnd_evt->glnd_hdl = glnd_cb->cb_hdl_id;
			glnd_evt->type = GLSV_GLND_EVT_UNLCK_REQ;
			glnd_evt->info.node_lck_info.glnd_mds_dest = lck_list_nm_info->req_mdest_id;
			glnd_evt->info.node_lck_info.client_handle_id = lck_list_nm_info->lock_info.handleId;
			glnd_evt->info.node_lck_info.lockid = lck_list_nm_info->lock_info.lockid;
			glnd_evt->info.node_lck_info.lcl_lockid = lck_list_nm_info->lock_info.lcl_lockid;
			glnd_evt->info.node_lck_info.resource_id = res_node->resource_id;
			glnd_evt->info.node_lck_info.lcl_resource_id = lck_list_nm_info->lcl_resource_id;
			glnd_evt->info.node_lck_info.invocation = lck_list_nm_info->lock_info.invocation;

			TRACE("answering lost unlock request");
			glnd_evt_local_send(glnd_cb, glnd_evt, MDS_SEND_PRIORITY_MEDIUM);
		}
	}

	TRACE_LEAVE();
}
