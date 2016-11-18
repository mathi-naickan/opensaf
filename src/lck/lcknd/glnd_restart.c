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

  This file contains functions related to the Event Handling
..............................................................................

..............................................................................

  FUNCTIONS INCLUDED in this module:

******************************************************************************/

#include "lck/lcknd/glnd.h"
#include<string.h>
/*
 * Function Prototypes
 */
static uint32_t glnd_restart_build_resource_tree(GLND_CB *glnd_cb);

static uint32_t glnd_restart_build_res_lock_list(GLND_CB *glnd_cb);

static uint32_t glnd_restart_build_backup_event_tree(GLND_CB *glnd_cb);

static uint32_t glnd_restart_add_res_lock_to_resource_tree(GLND_CB *glnd_cb,
							GLND_RESTART_RES_LOCK_LIST_INFO *restart_res_lock_list_info);

static uint32_t glnd_restart_resource_node_add(GLND_CB *glnd_cb, GLND_RESTART_RES_INFO *restart_res_info);

static uint32_t glnd_restart_event_add(GLND_CB *glnd_cb, GLSV_RESTART_BACKUP_EVT_INFO *evt_info);

/*****************************************************************************
  PROCEDURE NAME : glnd_restart_build_database()

  DESCRIPTION    : Build the database after restart.

  ARGUMENTS      : glnd_cb      - ptr to the GLND control block

  RETURNS        :

  NOTES         : None
*****************************************************************************/
uint32_t glnd_restart_build_database(GLND_CB *glnd_cb)
{
	uint32_t ret_val = NCSCC_RC_FAILURE;

	TRACE_ENTER();

	/* Build resource tree of glnd_cb */
	ret_val = glnd_restart_build_resource_tree(glnd_cb);
	if (ret_val == NCSCC_RC_SUCCESS) {
		/* Build resource_lock_list  of client_tree and resource_tree  */
		ret_val = glnd_restart_build_res_lock_list(glnd_cb);
		if (ret_val == NCSCC_RC_SUCCESS) {
			/* Build backup event  of glnd_cb */
			ret_val = glnd_restart_build_backup_event_tree(glnd_cb);
			if (ret_val == NCSCC_RC_SUCCESS) {
				TRACE_1("GLND restart build database success: ret_value %u ", ret_val);
				goto done;
			} else {
				TRACE_2("GLND restart build database failure: ret_value %u", ret_val);
				ret_val = NCSCC_RC_FAILURE;
				goto done;
			}

		}
		ret_val = NCSCC_RC_FAILURE;
		goto done;
	} 
done:
	TRACE_LEAVE2("Return valu: %d",ret_val);
	return ret_val;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_restart_build_resource_tree()

  DESCRIPTION    : Build the client tree.

  ARGUMENTS      :glnd_cb                  - ptr to the GLND control block

  RETURNS        :
                  

  NOTES         : None
*****************************************************************************/
static uint32_t glnd_restart_build_resource_tree(GLND_CB *glnd_cb)
{
	SaAisErrorT rc = NCSCC_RC_SUCCESS;
	GLND_RESTART_RES_INFO *shm_base_addr = NULL;
	GLND_RESTART_RES_INFO restart_res_info;
	uint32_t i;

	TRACE_ENTER();

	shm_base_addr = glnd_cb->glnd_res_shm_base_addr;
	for (i = 0; i < GLND_RESOURCE_INFO_CKPT_MAX_SECTIONS; i++) {
		if (shm_base_addr[i].valid == GLND_SHM_INFO_VALID) {
			/* Read the res_info from shared memory and build the client tree */
			memset(&restart_res_info, 0, sizeof(GLND_RESTART_RES_INFO));
			rc = glnd_restart_resource_ckpt_read(glnd_cb, &restart_res_info, i);
			if (rc == NCSCC_RC_SUCCESS)
				glnd_restart_resource_node_add(glnd_cb, &restart_res_info);
			else {
				TRACE_2("GLND restart resource tree build failure: rc %d", rc);
				goto done;
			}
		}
	}
	TRACE("GLND restart resource tree build success");
done:
	TRACE_LEAVE2("Return val:%d",rc);
	return rc;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_restart_build_res_lock_list()

  DESCRIPTION    : Build the client tree.

  ARGUMENTS      : glnd_cb  - ptr to the GLND control block

  RETURNS        :

  NOTES         : None
*****************************************************************************/
static uint32_t glnd_restart_build_res_lock_list(GLND_CB *glnd_cb)
{
	GLND_RESTART_RES_LOCK_LIST_INFO restart_res_lock_list_info;
	GLND_RESTART_RES_LOCK_LIST_INFO *shm_base_addr = NULL;
	SaAisErrorT rc;
	uint32_t i;

	shm_base_addr = glnd_cb->glnd_lck_shm_base_addr;
	for (i = 0; i < GLND_RES_LOCK_INFO_CKPT_MAX_SECTIONS; i++) {
		if (shm_base_addr[i].valid == GLND_SHM_INFO_VALID) {
			/* Read the res_info from shared memory and build the client tree */
			memset(&restart_res_lock_list_info, 0, sizeof(GLND_RESTART_RES_LOCK_LIST_INFO));
			rc = glnd_restart_res_lock_ckpt_read(glnd_cb, &restart_res_lock_list_info, i);
			if (rc == NCSCC_RC_SUCCESS) {
				if (restart_res_lock_list_info.resource_id) {
					glnd_restart_add_res_lock_to_resource_tree(glnd_cb,
										   &restart_res_lock_list_info);
				}

			} else {
				TRACE_2("GLND restart lck list build failure: rc %d", rc);
				return NCSCC_RC_FAILURE;
			}

		}
	}
	TRACE_1("GLND restart lck list build success");
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_restart_add_res_lock_to_resource_tree()

  DESCRIPTION    : Build the client tree.

  ARGUMENTS      :glnd_cb                  - ptr to the GLND control block

  RETURNS        :

  NOTES         : None
*****************************************************************************/
static uint32_t glnd_restart_add_res_lock_to_resource_tree(GLND_CB *glnd_cb,
							GLND_RESTART_RES_LOCK_LIST_INFO *restart_res_lock_list_info)
{
	GLND_RESOURCE_INFO *res_info = NULL;
	GLND_RES_LOCK_LIST_INFO *lck_list_info = NULL;
	GLND_CLIENT_INFO *client_info = NULL;
	uint32_t node_id;

	if (restart_res_lock_list_info == NULL)
		return NCSCC_RC_FAILURE;

	lck_list_info = (GLND_RES_LOCK_LIST_INFO *)m_MMGR_ALLOC_GLND_RES_LOCK_LIST_INFO;
	if (lck_list_info == NULL) {
		LOG_CR("GLND Rsc lock list alloc failed: Error %s", strerror(errno));
		assert(0);
	}
	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(restart_res_lock_list_info->req_mdest_id);

	client_info =
	    (GLND_CLIENT_INFO *)ncs_patricia_tree_get(&glnd_cb->glnd_client_tree,
						      (uint8_t *)&(restart_res_lock_list_info->lock_info.handleId));

	res_info =
	    (GLND_RESOURCE_INFO *)ncs_patricia_tree_get(&glnd_cb->glnd_res_tree,
							(uint8_t *)&(restart_res_lock_list_info->resource_id));
	if (res_info) {
		memset(lck_list_info, 0, sizeof(GLND_RES_LOCK_LIST_INFO));

		lck_list_info->lck_info_hdl_id =
		    ncshm_create_hdl((uint8_t)glnd_cb->pool_id, NCS_SERVICE_ID_GLND, (NCSCONTEXT)lck_list_info);

		lck_list_info->lock_info = restart_res_lock_list_info->lock_info;
		if (node_id == m_NCS_NODE_ID_FROM_MDS_DEST(glnd_cb->glnd_mdest_id))
			lck_list_info->req_mdest_id = glnd_cb->glnd_mdest_id;
		else
			lck_list_info->req_mdest_id = restart_res_lock_list_info->req_mdest_id;
		lck_list_info->res_info = res_info;
		lck_list_info->lcl_resource_id = restart_res_lock_list_info->lcl_resource_id;
		lck_list_info->unlock_call_type = restart_res_lock_list_info->unlock_call_type;
		lck_list_info->unlock_req_sent = restart_res_lock_list_info->unlock_req_sent;
		lck_list_info->non_master_status = restart_res_lock_list_info->non_master_status;
		lck_list_info->shm_index = restart_res_lock_list_info->shm_index;
		/* based on which_list add the restart_res_lock_list_info->which_list 
		   add res_lock_list_info to either lock_master_info or lcl_lck_req_info of resource_info  */

		if (restart_res_lock_list_info->to_which_list == LCL_LOCK_REQ_LIST) {
			/* ADD TO LCL_LOCK_REQ_INFO */
			lck_list_info->next = res_info->lcl_lck_req_info;
			if (res_info->lcl_lck_req_info)
				res_info->lcl_lck_req_info->prev = lck_list_info;
			res_info->lcl_lck_req_info = lck_list_info;
		} else if (restart_res_lock_list_info->to_which_list == LOCK_MASTER_LIST) {
			/* ADD TO LOCK_MASTER_INFO */
			if (lck_list_info->lock_info.lock_type == SA_LCK_EX_LOCK_MODE) {
				if (lck_list_info->lock_info.lockStatus == SA_LCK_LOCK_GRANTED) {
					if (res_info->lck_master_info.grant_list == NULL) {
						/* add it to the grant list */
						res_info->lck_master_info.grant_list = lck_list_info;
					}
				} else {
					/*Add to the wait_list */
					lck_list_info->next = res_info->lck_master_info.wait_exclusive_list;
					if (res_info->lck_master_info.wait_exclusive_list)
						res_info->lck_master_info.wait_exclusive_list->prev = lck_list_info;
					res_info->lck_master_info.wait_exclusive_list = lck_list_info;

					if (lck_list_info->req_mdest_id == glnd_cb->glnd_mdest_id) {
						glnd_start_tmr(glnd_cb, &lck_list_info->timeout_tmr,
							       GLND_TMR_RES_LOCK_REQ_TIMEOUT,
							       lck_list_info->lock_info.timeout,
							       (uint32_t)lck_list_info->lck_info_hdl_id);
					}

				}
			}
			if (lck_list_info->lock_info.lock_type == SA_LCK_PR_LOCK_MODE) {
				if (lck_list_info->lock_info.lockStatus == SA_LCK_LOCK_GRANTED) {
					lck_list_info->next = res_info->lck_master_info.grant_list;
					if (res_info->lck_master_info.grant_list)
						res_info->lck_master_info.grant_list->prev = lck_list_info;
					res_info->lck_master_info.grant_list = lck_list_info;
				} else {
					/*Add to the wait_list */
					/* add it to the read wait list */
					lck_list_info->next = res_info->lck_master_info.wait_read_list;
					if (res_info->lck_master_info.wait_read_list)
						res_info->lck_master_info.wait_read_list->prev = lck_list_info;
					res_info->lck_master_info.wait_read_list = lck_list_info;

					if (lck_list_info->req_mdest_id == glnd_cb->glnd_mdest_id) {
						glnd_start_tmr(glnd_cb, &lck_list_info->timeout_tmr,
							       GLND_TMR_RES_LOCK_REQ_TIMEOUT,
							       lck_list_info->lock_info.timeout,
							       (uint32_t)lck_list_info->lck_info_hdl_id);
					}

				}
			}
		}
		/* Add lock_list_info to client_tree */
		if (client_info != NULL) {
			glnd_client_node_resource_lock_req_add(client_info, res_info, lck_list_info);
		}
	} else
		m_MMGR_FREE_GLND_RES_LOCK_LIST_INFO(lck_list_info);

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_restart_resource_node_add

  DESCRIPTION    : Adds the resource node to the tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block
                  client_info  - client info to be added

  RETURNS        :The pointer to the client info node on success.
                  else returns NULL.

  NOTES         : None
*****************************************************************************/
static uint32_t glnd_restart_resource_node_add(GLND_CB *glnd_cb, GLND_RESTART_RES_INFO *restart_res_info)
{
	GLND_RESOURCE_INFO *res_info = NULL;
	int new_node = 0;
	uint32_t node_id;
	TRACE_ENTER();

	if (restart_res_info == NULL)
		return NCSCC_RC_FAILURE;
	/* TBD NEED TO remove adding new node info */
	/* check to see if already present */
	res_info = glnd_resource_node_find(glnd_cb, restart_res_info->resource_id);
	if (res_info == NULL) {
		new_node = 1;
		/* allocate the memory */
		res_info = (GLND_RESOURCE_INFO *)m_MMGR_ALLOC_GLND_RESOURCE_INFO;
		if (!res_info) {
			LOG_CR("GLND Rsc node alloc failed: Error %s", strerror(errno));
			assert(0);
		}
		memset(res_info, 0, sizeof(GLND_RESOURCE_INFO));
	}

	/* assign the values */
	res_info->resource_id = restart_res_info->resource_id;
	res_info->status = restart_res_info->status;
	res_info->master_status = restart_res_info->master_status;
	res_info->lcl_ref_cnt = restart_res_info->lcl_ref_cnt;
	res_info->lck_master_info.pr_orphan_req_count = restart_res_info->pr_orphan_req_count;
	res_info->lck_master_info.ex_orphan_req_count = restart_res_info->ex_orphan_req_count;
	res_info->lck_master_info.pr_orphaned = restart_res_info->pr_orphaned;
	res_info->lck_master_info.ex_orphaned = restart_res_info->ex_orphaned;
	res_info->shm_index = restart_res_info->shm_index;

	memcpy(&res_info->resource_name, &restart_res_info->resource_name, sizeof(SaNameT));

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(restart_res_info->master_mds_dest);

	if (node_id == m_NCS_NODE_ID_FROM_MDS_DEST(glnd_cb->glnd_mdest_id))
		res_info->master_mds_dest = glnd_cb->glnd_mdest_id;
	else
		res_info->master_mds_dest = restart_res_info->master_mds_dest;

	if (new_node) {
		/* add it to the tree */
		res_info->patnode.key_info = (uint8_t *)&res_info->resource_id;
		if (ncs_patricia_tree_add(&glnd_cb->glnd_res_tree, &res_info->patnode) != NCSCC_RC_SUCCESS) {
			LOG_ER("GLND Rsc node add failed");
			m_MMGR_FREE_GLND_RESOURCE_INFO(res_info);
			return NCSCC_RC_FAILURE;
		}
		TRACE("GLND Rsc node add success: resource_id %u", (uint32_t)res_info->resource_id);
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME :glnd_restart_client_resource_node_add 

  DESCRIPTION    : Adds the resource node to the tree.

  ARGUMENTS      :glnd_cb      - ptr to the GLND control block

  RETURNS        :

  NOTES         : None
*****************************************************************************/
GLND_RESOURCE_INFO *glnd_restart_client_resource_node_add(GLND_CB *glnd_cb, SaLckResourceIdT resource_id)
{
	GLND_RESOURCE_INFO *res_info = NULL;

	/* check to see if already present */
	if ((res_info = glnd_resource_node_find(glnd_cb, resource_id)) != NULL) {
		return res_info;
	}

	/* allocate the memory */
	res_info = (GLND_RESOURCE_INFO *)m_MMGR_ALLOC_GLND_RESOURCE_INFO;

	if (!res_info) {
		LOG_CR("GLND Rsc node alloc failed: Error %s", strerror(errno));
		assert(0);
	}
	memset(res_info, 0, sizeof(GLND_RESOURCE_INFO));

	/* assign the values */
	res_info->resource_id = resource_id;

	/* add it to the tree */
	res_info->patnode.key_info = (uint8_t *)&res_info->resource_id;
	if (ncs_patricia_tree_add(&glnd_cb->glnd_res_tree, &res_info->patnode) != NCSCC_RC_SUCCESS) {
		LOG_ER("GLND Rsc node add failed");
		m_MMGR_FREE_GLND_RESOURCE_INFO(res_info);
		return NULL;
	}
	/* log the Resource Add */
	TRACE("GLND Rsc node add success: resource_id %u", (uint32_t)res_info->resource_id);
	return res_info;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_restart_event_add()

  DESCRIPTION    : Build the client tree.

  ARGUMENTS      :glnd_cb                  - ptr to the GLND control block
                  resource_info_checkpoint_handle - resourc_info checkpoint handle

  RETURNS        :

  NOTES         : None
*****************************************************************************/
static uint32_t glnd_restart_event_add(GLND_CB *glnd_cb, GLSV_RESTART_BACKUP_EVT_INFO *evt_info)
{
	GLSV_GLND_EVT glnd_evt;
	GLND_RES_LOCK_LIST_INFO *lck_list_info = NULL;
	GLND_RESOURCE_INFO *res_node = NULL;

	/* check for the resource node */
	res_node = glnd_resource_node_find(glnd_cb, evt_info->resource_id);
	if (!res_node) {
		LOG_ER("GLND Rsc node find failed");
		return NCSCC_RC_FAILURE;
	}

	/* get the lock node */
	if (res_node->lcl_lck_req_info) {
		for (lck_list_info = res_node->lcl_lck_req_info; lck_list_info != NULL;
		     lck_list_info = lck_list_info->next) {

			if (lck_list_info->lock_info.handleId == evt_info->client_handle_id &&
			    lck_list_info->lock_info.lockid == evt_info->lockid)
				break;
		}
	}

	memset(&glnd_evt, 0, sizeof(GLSV_GLND_EVT));
	if (lck_list_info) {
		if (evt_info->type == GLSV_GLND_EVT_LCK_PURGE) {
			glnd_evt.type = GLSV_GLND_EVT_LCK_PURGE;
			glnd_evt.info.rsc_info.resource_id = evt_info->resource_id;
		} else {
			glnd_evt.type = evt_info->type;
			glnd_evt.info.node_lck_info.glnd_mds_dest = evt_info->mds_dest;
			glnd_evt.info.node_lck_info.client_handle_id = evt_info->client_handle_id;
			glnd_evt.info.node_lck_info.lock_type = evt_info->lock_type;
			glnd_evt.info.node_lck_info.lockFlags = evt_info->lockFlags;
			glnd_evt.info.node_lck_info.resource_id = evt_info->resource_id;
			glnd_evt.info.node_lck_info.lockid = evt_info->lockid;
			glnd_evt.shm_index = evt_info->shm_index;

		}

		glnd_evt_backup_queue_add(glnd_cb, &glnd_evt);
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_restart_build_backup_event_tree()

  DESCRIPTION    : Build the client tree.

  ARGUMENTS      :glnd_cb                  - ptr to the GLND control block

  RETURNS        :

  NOTES         : None
*****************************************************************************/
static uint32_t glnd_restart_build_backup_event_tree(GLND_CB *glnd_cb)
{
	GLSV_RESTART_BACKUP_EVT_INFO glnd_restart_backup_evt;
	GLSV_RESTART_BACKUP_EVT_INFO *shm_base_address = NULL;;
	SaAisErrorT rc;
	uint32_t i;

	shm_base_address = glnd_cb->glnd_evt_shm_base_addr;
	for (i = 0; i < GLND_BACKUP_EVT_CKPT_MAX_SECTIONS; i++) {
		if (shm_base_address[i].valid == GLND_SHM_INFO_VALID) {
			/* Read the res_info from shared memory and build the client tree */
			memset(&glnd_restart_backup_evt, 0, sizeof(GLSV_RESTART_BACKUP_EVT_INFO));
			rc = glnd_restart_backup_event_read(glnd_cb, &glnd_restart_backup_evt, i);
			if (rc == NCSCC_RC_SUCCESS) {
				glnd_restart_event_add(glnd_cb, &glnd_restart_backup_evt);
			} else {
				TRACE_2("GLND restart evt list build failure: rc %d", rc);
				return NCSCC_RC_FAILURE;
			}

		}
	}
	TRACE_1("GLND restart evt list build success");
	return NCSCC_RC_SUCCESS;
}
