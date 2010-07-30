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

  This file contains functions related to the Client structure handling.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************/

#include "glnd.h"

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_find

  DESCRIPTION    : Finds the Client info node from the tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  mds_handle_id  - vcard id of the agent.

  RETURNS        :The pointer to the client info node

  NOTES         : None
*****************************************************************************/
GLND_CLIENT_INFO *glnd_client_node_find(GLND_CB *glnd_cb, SaLckHandleT handle_id)
{
	/* search for the agent id */
	return (GLND_CLIENT_INFO *)ncs_patricia_tree_get(&glnd_cb->glnd_client_tree, (uns8 *)&handle_id);
}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_find_next

  DESCRIPTION    : Finds the Client info node from the tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  handle_id - the handle id of the client.

  RETURNS        :The pointer to the client info node

  NOTES         : None
*****************************************************************************/
GLND_CLIENT_INFO *glnd_client_node_find_next(GLND_CB *glnd_cb, SaLckHandleT handle_id, MDS_DEST agent_mds_dest)
{
	GLND_CLIENT_INFO *client_info;

	/* search for the agent id */
	client_info = (GLND_CLIENT_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_client_tree, (uns8 *)&handle_id);

	while (client_info) {
		if (memcmp(&client_info->agent_mds_dest, &agent_mds_dest, sizeof(MDS_DEST)) == 0)
			return client_info;
		else
			client_info = (GLND_CLIENT_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_client_tree, (uns8 *)
										    &client_info->app_handle_id);
	}
	return NULL;

}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_add

  DESCRIPTION    : Adds the client node to the tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  agent_mds_dest   - mds dest id for the agent.
                 

  RETURNS        :The pointer to the client info node on success.
                  else returns NULL.

  NOTES         : None
*****************************************************************************/
GLND_CLIENT_INFO *glnd_client_node_add(GLND_CB *glnd_cb, MDS_DEST agent_mds_dest, SaLckHandleT app_handle_id)
{
	GLND_CLIENT_INFO *client_info = NULL;

	/* create new clientt info and put it into the tree */
	if ((client_info = m_MMGR_ALLOC_GLND_CLIENT_INFO) == NULL) {
		m_LOG_GLND_MEMFAIL(GLND_CLIENT_ALLOC_FAILED, __FILE__, __LINE__);
		return NULL;
	}
	memset(client_info, 0, sizeof(GLND_CLIENT_INFO));
	if (app_handle_id) {
		client_info->app_handle_id = app_handle_id;
	} else {
		/* assign the memory space as the client's handle id */
		client_info->app_handle_id = m_ASSIGN_LCK_HANDLE_ID((long)client_info);
	}
	client_info->agent_mds_dest = agent_mds_dest;

	client_info->patnode.key_info = (uns8 *)&client_info->app_handle_id;
	if (ncs_patricia_tree_add(&glnd_cb->glnd_client_tree, &client_info->patnode) != NCSCC_RC_SUCCESS) {
		m_LOG_GLND_API(GLND_CLIENT_TREE_ADD_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		/* free and return */
		m_MMGR_FREE_GLND_CLIENT_INFO(client_info);
		return NULL;
	}
	return client_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_del

  DESCRIPTION    : Deletes the client node from the tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  agent_mds_dest   - mds dest id for the agent.
                 

  RETURNS        :NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
uns32 glnd_client_node_del(GLND_CB *glnd_cb, GLND_CLIENT_INFO *client_info)
{
	GLND_CLIENT_LIST_RESOURCE *res_list, *tmp_res_list;
	GLND_RESOURCE_INFO *res_info;
	SaLckLockModeT mode;
	GLSV_GLD_EVT gld_evt;
	NCS_BOOL orphan = FALSE;

	/* delete from the tree */
	if (ncs_patricia_tree_del(&glnd_cb->glnd_client_tree, &client_info->patnode) != NCSCC_RC_SUCCESS) {
		m_LOG_GLND_API(GLND_CLIENT_TREE_DEL_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	/* free up all the resource requests */
	for (res_list = client_info->res_list; res_list != NULL;) {
		res_info = res_list->rsc_info;
		tmp_res_list = res_list;
		res_list = res_list->next;
		if (res_info) {
			glnd_set_orphan_state(glnd_cb, res_info);
			glnd_client_node_resource_del(glnd_cb, client_info, res_info);
			if (!res_info->lck_master_info.grant_list) {
				/* do the re sync of the grant list */
				glnd_resource_master_lock_resync_grant_list(glnd_cb, res_info);
			}
			if (res_info->lcl_ref_cnt > 0) {

				memset(&gld_evt, 0, sizeof(GLSV_GLD_EVT));

				gld_evt.evt_type = GLSV_GLD_EVT_RSC_CLOSE;
				gld_evt.info.rsc_details.rsc_id = res_info->resource_id;
				gld_evt.info.rsc_details.lcl_ref_cnt = res_info->lcl_ref_cnt;
				glnd_mds_msg_send_gld(glnd_cb, &gld_evt, glnd_cb->gld_mdest_id);

				res_info->lcl_ref_cnt--;
			}

			glnd_restart_resource_info_ckpt_overwrite(glnd_cb, res_info);

			if (tmp_res_list->open_ref_cnt > 1) {
				memset(&gld_evt, 0, sizeof(GLSV_GLD_EVT));

				gld_evt.evt_type = GLSV_GLD_EVT_RSC_CLOSE;
				gld_evt.info.rsc_details.rsc_id = res_info->resource_id;
				gld_evt.info.rsc_details.lcl_ref_cnt = tmp_res_list->open_ref_cnt;
				glnd_mds_msg_send_gld(glnd_cb, &gld_evt, glnd_cb->gld_mdest_id);

			}
			if (res_info->lcl_ref_cnt == 0
			    && glnd_resource_grant_list_orphan_locks(res_info, &mode) == FALSE) {
				glnd_resource_node_destroy(glnd_cb, res_info, orphan);
			}
		}
	}

	/* free up any stale res_requests from this finalized client ... */
	while (glnd_cb->res_req_list != NULL) {
		if (client_info->app_handle_id == glnd_cb->res_req_list->client_handle_id)
			glnd_resource_req_node_del(glnd_cb, glnd_cb->res_req_list->res_req_hdl_id);
	}

	/* free the memory */
	m_MMGR_FREE_GLND_CLIENT_INFO(client_info);

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_resource_add

  DESCRIPTION    : Adds the resource node to the client resource list

  ARGUMENTS      :client_info - ptr to the client info
                  res_info - ptr to the resource node
                 

  RETURNS        :NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
uns32 glnd_client_node_resource_add(GLND_CLIENT_INFO *client_info, GLND_RESOURCE_INFO *res_info)
{
	GLND_CLIENT_LIST_RESOURCE *resource_list;

	if (!client_info)
		return NCSCC_RC_FAILURE;

	for (resource_list = client_info->res_list; resource_list != NULL &&
	     resource_list->rsc_info != res_info; resource_list = resource_list->next) ;

	if (resource_list == NULL) {
		resource_list = m_MMGR_ALLOC_GLND_CLIENT_RES_LIST;
		if (resource_list == NULL) {
			m_LOG_GLND_MEMFAIL(GLND_CLIENT_RSC_LIST_ALLOC_FAILED, __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}
		memset(resource_list, 0, sizeof(GLND_CLIENT_LIST_RESOURCE));
		resource_list->rsc_info = res_info;
		resource_list->open_ref_cnt = 1;
		resource_list->next = client_info->res_list;
		if (client_info->res_list)
			client_info->res_list->prev = resource_list;
		client_info->res_list = resource_list;
		return NCSCC_RC_SUCCESS;
	} else if (resource_list->rsc_info == res_info) {
		resource_list->open_ref_cnt++;
	}
	return NCSCC_RC_FAILURE;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_resource_del

  DESCRIPTION    : deletes the resource node to the client resource list

  ARGUMENTS      :client_info - ptr to the client info
                  res_info - ptr to the resource node

  RETURNS        :NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
uns32 glnd_client_node_resource_del(GLND_CB *glnd_cb, GLND_CLIENT_INFO *client_info, GLND_RESOURCE_INFO *res_info)
{
	GLND_CLIENT_LIST_RESOURCE *resource_list;
	GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *lock_req_list, *del_req_list;
	GLSV_GLND_EVT glnd_evt;
	SaLckLockModeT lock_type = SA_LCK_PR_LOCK_MODE;
	NCS_BOOL local_orphan_lock = FALSE;

	if (!client_info) {
		return NCSCC_RC_FAILURE;
	}

	for (resource_list = client_info->res_list; resource_list != NULL &&
	     resource_list->rsc_info != res_info; resource_list = resource_list->next) ;

	if (resource_list) {
		/* remove it from the list */
		/* delete all the lock requests */
		for (lock_req_list = resource_list->lck_list; lock_req_list != NULL;) {
			if (resource_list->rsc_info->status == GLND_RESOURCE_ACTIVE_NON_MASTER) {
				/* send request to orphan the lock */
				m_GLND_RESOURCE_NODE_LCK_INFO_FILL(glnd_evt, GLSV_GLND_EVT_LCK_REQ_ORPHAN,
								   resource_list->rsc_info->resource_id,
								   lock_req_list->lck_req->lcl_resource_id,
								   lock_req_list->lck_req->lock_info.handleId,
								   lock_req_list->lck_req->lock_info.lockid,
								   lock_req_list->lck_req->lock_info.lock_type,
								   lock_req_list->lck_req->lock_info.lockFlags,
								   0, 0, 0, 0,
								   lock_req_list->lck_req->lock_info.lcl_lockid, 0);
				glnd_evt.info.node_lck_info.glnd_mds_dest = glnd_cb->glnd_mdest_id;
				glnd_mds_msg_send_glnd(glnd_cb, &glnd_evt, res_info->master_mds_dest);
			} else {
				/* unset any orphan count */
				lock_type = lock_req_list->lck_req->lock_info.lock_type;
				if ((lock_req_list->lck_req->lock_info.lockFlags & SA_LCK_LOCK_ORPHAN) ==
				    SA_LCK_LOCK_ORPHAN)
					local_orphan_lock = TRUE;
			}
			del_req_list = lock_req_list;
			lock_req_list = lock_req_list->next;

			if (del_req_list->lck_req) {
				if (del_req_list->lck_req->res_info == res_info)
					glnd_resource_lock_req_delete(res_info, del_req_list->lck_req);
			}

			if (local_orphan_lock == TRUE)
				glnd_resource_lock_req_unset_orphan(glnd_cb, res_info, lock_type);
			glnd_client_node_resource_lock_req_del(client_info, resource_list, del_req_list);
		}

		if (resource_list == client_info->res_list)
			client_info->res_list = resource_list->next;
		if (resource_list->next)
			resource_list->next->prev = resource_list->prev;
		if (resource_list->prev)
			resource_list->prev->next = resource_list->next;

		m_MMGR_FREE_GLND_CLIENT_RES_LIST(resource_list);

	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_lcl_resource_del

  DESCRIPTION    : deletes the resource node to the client resource list

  ARGUMENTS      :client_info - ptr to the client info
                  res_info - ptr to the resource node
                 

  RETURNS        :NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
uns32 glnd_client_node_lcl_resource_del(GLND_CB *glnd_cb,
					GLND_CLIENT_INFO *client_info,
					GLND_RESOURCE_INFO *res_info,
					SaLckResourceIdT lcl_resource_id,
					uns32 lcl_res_id_count, NCS_BOOL *resource_del_flag)
{
	GLND_CLIENT_LIST_RESOURCE *resource_list;
	GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *lock_req_list, *del_req_list;
	GLSV_GLND_EVT glnd_evt;
	SaLckLockModeT lock_type = SA_LCK_PR_LOCK_MODE;
	NCS_BOOL local_orphan_lock = FALSE;
	NCS_BOOL resource_del = TRUE;

	if (!client_info) {
		return NCSCC_RC_FAILURE;
	}

	for (resource_list = client_info->res_list; resource_list != NULL &&
	     resource_list->rsc_info != res_info; resource_list = resource_list->next) ;

	if (resource_list) {
		/* delete all the lock requests */
		for (lock_req_list = resource_list->lck_list; lock_req_list != NULL;) {
			if (lock_req_list->lck_req) {
				if (lock_req_list->lck_req->lcl_resource_id == lcl_resource_id) {
					if (lock_req_list->lck_req->res_info->status == GLND_RESOURCE_ACTIVE_NON_MASTER) {

						/* send request to orphan the lock */
						m_GLND_RESOURCE_NODE_LCK_INFO_FILL(glnd_evt,
										   GLSV_GLND_EVT_LCK_REQ_ORPHAN,
										   lock_req_list->lck_req->res_info->
										   resource_id,
										   lock_req_list->lck_req->
										   lcl_resource_id,
										   lock_req_list->lck_req->lock_info.
										   handleId,
										   lock_req_list->lck_req->lock_info.
										   lockid,
										   lock_req_list->lck_req->lock_info.
										   lock_type,
										   lock_req_list->lck_req->lock_info.
										   lockFlags, 0, 0, 0, 0,
										   lock_req_list->lck_req->lock_info.
										   lcl_lockid, 0);

						glnd_evt.info.node_lck_info.glnd_mds_dest = glnd_cb->glnd_mdest_id;
						glnd_mds_msg_send_glnd(glnd_cb, &glnd_evt, res_info->master_mds_dest);
					} else {
						/* unset any orphan count */
						lock_type = lock_req_list->lck_req->lock_info.lock_type;
						if ((lock_req_list->lck_req->lock_info.
						     lockFlags & SA_LCK_LOCK_ORPHAN) == SA_LCK_LOCK_ORPHAN)
							local_orphan_lock = TRUE;
					}
					del_req_list = lock_req_list;
					lock_req_list = lock_req_list->next;
					glnd_resource_lock_req_delete(res_info, del_req_list->lck_req);

					if (local_orphan_lock == TRUE)
						glnd_resource_lock_req_unset_orphan(glnd_cb, res_info, lock_type);
					glnd_client_node_resource_lock_req_del(client_info, resource_list,
									       del_req_list);
				} else {
					lock_req_list = lock_req_list->next;
				}

			}
		}
		if (resource_del == TRUE && lcl_res_id_count == 1) {
			/* remove it from the list */
			if (resource_list == client_info->res_list)
				client_info->res_list = resource_list->next;
			if (resource_list->next)
				resource_list->next->prev = resource_list->prev;
			if (resource_list->prev)
				resource_list->prev->next = resource_list->next;

			*resource_del_flag = resource_del;

			m_MMGR_FREE_GLND_CLIENT_RES_LIST(resource_list);
		} else
			*resource_del_flag = resource_del;
	}
	*resource_del_flag = resource_del;

	return NCSCC_RC_SUCCESS;

}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_resource_lock_req_add

  DESCRIPTION    : Adds the lock request to the client resource lock req list

  ARGUMENTS      :client_info - ptr to the client info
                  res_info - ptr to the resource node

                 

  RETURNS        :NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
uns32 glnd_client_node_resource_lock_req_add(GLND_CLIENT_INFO *client_info,
					     GLND_RESOURCE_INFO *res_info, GLND_RES_LOCK_LIST_INFO *lock_req_info)
{
	GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *lock_req_list;
	GLND_CLIENT_LIST_RESOURCE *resource_list;

	if (!client_info)
		return NCSCC_RC_FAILURE;

	for (resource_list = client_info->res_list; resource_list != NULL &&
	     resource_list->rsc_info != res_info; resource_list = resource_list->next) ;

	if (resource_list == NULL)
		return NCSCC_RC_FAILURE;

	lock_req_list = m_MMGR_ALLOC_GLND_CLIENT_RES_LOCK_LIST_REQ;
	if (!lock_req_list) {
		m_LOG_GLND_MEMFAIL(GLND_CLIENT_RSC_LOCK_LIST_ALLOC_FAILED, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	memset(lock_req_list, 0, sizeof(GLND_CLIENT_LIST_RESOURCE_LOCK_REQ));
	lock_req_list->lck_req = lock_req_info;
	lock_req_list->next = resource_list->lck_list;
	if (resource_list->lck_list)
		resource_list->lck_list->prev = lock_req_list;
	resource_list->lck_list = lock_req_list;
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_resource_lock_req_del

  DESCRIPTION    : deletes the lock request to the client resource lock req list

  ARGUMENTS      :client_info - ptr to the client info
                  res_info - ptr to the resource node

                 

  RETURNS        :NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
uns32 glnd_client_node_resource_lock_req_del(GLND_CLIENT_INFO *client_info,
					     GLND_CLIENT_LIST_RESOURCE *res_list,
					     GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *lock_req_list)
{
	if (client_info == NULL || lock_req_list == NULL)
		return NCSCC_RC_FAILURE;

	if (res_list->lck_list == lock_req_list)
		res_list->lck_list = lock_req_list->next;
	if (lock_req_list->prev)
		lock_req_list->prev->next = lock_req_list->next;
	if (lock_req_list->next)
		lock_req_list->next->prev = lock_req_list->prev;
	m_MMGR_FREE_GLND_CLIENT_RES_LOCK_LIST_REQ(lock_req_list);
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_resource_lock_req_find

  DESCRIPTION    : Finds the lock request from the client tree/
  ARGUMENTS      :client_info - ptr to the client info
                  res_info - ptr to the resource node

                 

  RETURNS        :NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *glnd_client_node_resource_lock_req_find(GLND_CLIENT_INFO *client_info,
									    SaLckResourceIdT res_id,
									    SaLckLockIdT lockid)
{
	GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *lck_req_list;
	GLND_CLIENT_LIST_RESOURCE *resource_list;

	if (!client_info)
		return NULL;

	for (resource_list = client_info->res_list; resource_list != NULL &&
	     resource_list->rsc_info->resource_id != res_id; resource_list = resource_list->next) ;

	if (resource_list) {
		for (lck_req_list = resource_list->lck_list; lck_req_list != NULL &&
		     lck_req_list->lck_req->lock_info.lockid != lockid; lck_req_list = lck_req_list->next) ;

		return lck_req_list;
	}
	return NULL;

}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_resource_lock_req_find_and_del

  DESCRIPTION    : Finds the lock request from the client tree/
  ARGUMENTS      :client_info - ptr to the client info
                  res_info - ptr to the resource node

                 

  RETURNS        :NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
uns32 glnd_client_node_resource_lock_req_find_and_del(GLND_CLIENT_INFO *client_info,
						      SaLckResourceIdT res_id,
						      SaLckLockIdT lockid, SaLckResourceIdT lcl_resource_id)
{
	GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *lck_req_list;
	GLND_CLIENT_LIST_RESOURCE *resource_list;

	if (!client_info)
		return NCSCC_RC_FAILURE;

	for (resource_list = client_info->res_list; resource_list != NULL &&
	     resource_list->rsc_info->resource_id != res_id; resource_list = resource_list->next) ;

	if (resource_list) {
		for (lck_req_list = resource_list->lck_list; lck_req_list != NULL &&
		     lck_req_list->lck_req->lcl_resource_id == lcl_resource_id &&
		     lck_req_list->lck_req->lock_info.lockid != lockid; lck_req_list = lck_req_list->next) ;

		if (lck_req_list) {
			if (lck_req_list == resource_list->lck_list) {
				resource_list->lck_list = lck_req_list->next;
			}
			glnd_client_node_resource_lock_req_del(client_info, resource_list, lck_req_list);
			return NCSCC_RC_SUCCESS;
		}
	}
	return NCSCC_RC_FAILURE;

}

/*****************************************************************************
  PROCEDURE NAME : glnd_client_node_resource_lock_find_duplicate_ex

  DESCRIPTION    : Finds the lock request from the client tree/
  ARGUMENTS      :client_info - ptr to the client info
                  res_info - ptr to the resource node

                 

  RETURNS        :NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES         : None
*****************************************************************************/
uns32 glnd_client_node_resource_lock_find_duplicate_ex(GLND_CLIENT_INFO *client_info,
						       SaLckResourceIdT res_id, SaLckResourceIdT lcl_res_id)
{
	GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *lck_req_list;
	GLND_CLIENT_LIST_RESOURCE *resource_list;

	if (!client_info)
		return NCSCC_RC_FAILURE;

	for (resource_list = client_info->res_list; resource_list != NULL &&
	     resource_list->rsc_info->resource_id != res_id; resource_list = resource_list->next) ;

	if (resource_list) {
		for (lck_req_list = resource_list->lck_list; lck_req_list != NULL; lck_req_list = lck_req_list->next) {
			if (lck_req_list->lck_req->lock_info.lock_type == SA_LCK_EX_LOCK_MODE
			    && lck_req_list->lck_req->lcl_resource_id == lcl_res_id
			    && lck_req_list->lck_req->lock_info.lockStatus == SA_LCK_LOCK_GRANTED)
				return NCSCC_RC_FAILURE;
		}
	}
	return NCSCC_RC_SUCCESS;

}
