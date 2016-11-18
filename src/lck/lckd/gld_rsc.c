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

#include "lck/lckd/gld.h"
#include "gld_imm.h"
#include <string.h>
/*****************************************************************************
  FILE NAME: GLD_RSC.C

  DESCRIPTION: GLD events received and send related routines.

  FUNCTIONS INCLUDED in this module:
  
******************************************************************************/
static SaLckResourceIdT gld_gen_rsc_id(GLSV_GLD_CB *gld_cb);
GLSV_GLD_RSC_INFO *gld_find_rsc_by_id(GLSV_GLD_CB *gld_cb, SaLckResourceIdT rsc_id);

/****************************************************************************
 * Name          : gld_gen_rsc_id
 *
 * Description   : This is function is invoked generate a unique 32 bit 
 *                 resource id. This is done by simply incrementing uint32_t bit
 *                 number till we arrive at a id that has not been assinged
 *
 * Arguments     :  gld_cb -> Control block pointer
 *
 * Return Values : resource id
 *
 * Notes         : None.
 *****************************************************************************/
static SaLckResourceIdT gld_gen_rsc_id(GLSV_GLD_CB *gld_cb)
{
	SaLckResourceIdT gen_rsc_id = gld_cb->nxt_rsc_id;

	while (ncs_patricia_tree_get(&gld_cb->rsc_info_id, (uint8_t *)&gen_rsc_id) != NULL) {
		if (gen_rsc_id == 0xffffffff)
			gen_rsc_id = 210;
		else
			gen_rsc_id++;
		if (gen_rsc_id == gld_cb->nxt_rsc_id)
			return 0;
	}

	if (gen_rsc_id == 0xffffffff)
		gld_cb->nxt_rsc_id = 210;
	else
		gld_cb->nxt_rsc_id = gen_rsc_id + 1;
	return gen_rsc_id;
}

/****************************************************************************
 * Name          : gld_add_rsc_info
 *
 * Description   : This is function is invoked to create  a resource
 *                 by its name.
 *
 * Arguments     : gld_cb        - control block
                   rsc_name      - Resource that is being referred to
  *
 * Return Values : Pointer to the rsc details
 *
 * Notes         : None.
 *****************************************************************************/
GLSV_GLD_RSC_INFO *gld_add_rsc_info(GLSV_GLD_CB *gld_cb, SaNameT *rsc_name, SaLckResourceIdT rsc_id, SaAisErrorT *error)
{
	GLSV_GLD_RSC_INFO *rsc_info;
	GLSV_GLD_RSC_MAP_INFO *rsc_map_info;
	SaAisErrorT err = SA_AIS_OK;
	SaTimeT creation_time;

	rsc_info = m_MMGR_ALLOC_GLSV_GLD_RSC_INFO;
	if (rsc_info == NULL) {
		LOG_CR("Rsc info alloc failed: Error %s", strerror(errno));
		assert(0);
	}
	memset(rsc_info, '\0', sizeof(GLSV_GLD_RSC_INFO));
	memcpy(&rsc_info->lck_name, rsc_name, sizeof(SaNameT));
	memset(&creation_time, '\0', sizeof(SaTimeT));
	rsc_info->saf_rsc_no_of_users = 1;
	rsc_info->saf_rsc_creation_time = m_GET_TIME_STAMP(creation_time) * SA_TIME_ONE_SECOND;

	if (rsc_id)
		rsc_info->rsc_id = rsc_id;
	else
		rsc_info->rsc_id = gld_gen_rsc_id(gld_cb);
	if (rsc_info->rsc_id == 0) {
		LOG_ER("Unable to allocate Rsc id");
		m_MMGR_FREE_GLSV_GLD_RSC_INFO(rsc_info);
		*error = SA_AIS_ERR_NO_MEMORY;
		return NULL;
	}
	/* Add this node to the global resource id tree */
	rsc_info->pat_node.key_info = (uint8_t *)&rsc_info->rsc_id;
	if (ncs_patricia_tree_add(&gld_cb->rsc_info_id, &rsc_info->pat_node) != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree add failed");
		m_MMGR_FREE_GLSV_GLD_RSC_INFO(rsc_info);
		*error = SA_AIS_ERR_NO_MEMORY;
		return NULL;
	}

	rsc_map_info = m_MMGR_ALLOC_GLSV_GLD_RSC_MAP_INFO;
	if (rsc_map_info == NULL) {
		LOG_CR("Rsc info alloc failed: Error %s", strerror(errno));
		assert(0);
		if (ncs_patricia_tree_del(&gld_cb->rsc_info_id, (NCS_PATRICIA_NODE *)rsc_info) != NCSCC_RC_SUCCESS) {
			LOG_ER("Patricia tree del failed");
			return NULL;
		}
		m_MMGR_FREE_GLSV_GLD_RSC_INFO(rsc_info);
		*error = SA_AIS_ERR_NO_MEMORY;
		return NULL;
	}
	memset(rsc_map_info, '\0', sizeof(GLSV_GLD_RSC_MAP_INFO));
	memcpy(&rsc_map_info->rsc_name, rsc_name, sizeof(SaNameT));
	rsc_map_info->rsc_name.length = m_NCS_OS_HTONS(rsc_map_info->rsc_name.length);

	rsc_map_info->rsc_id = rsc_info->rsc_id;
	rsc_map_info->pat_node.key_info = (uint8_t *)&rsc_map_info->rsc_name;
	if (ncs_patricia_tree_add(&gld_cb->rsc_map_info, &rsc_map_info->pat_node) != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree add failed");
		if (ncs_patricia_tree_del(&gld_cb->rsc_info_id, (NCS_PATRICIA_NODE *)rsc_info) != NCSCC_RC_SUCCESS) {
			LOG_ER("Patricia tree del failed");
			m_MMGR_FREE_GLSV_GLD_RSC_MAP_INFO(rsc_map_info);
			return NULL;
		}
		m_MMGR_FREE_GLSV_GLD_RSC_INFO(rsc_info);
		m_MMGR_FREE_GLSV_GLD_RSC_MAP_INFO(rsc_map_info);
		*error = SA_AIS_ERR_LIBRARY;
		return NULL;
	}

	/*Add the imm runtime object */
	if (gld_cb->ha_state == SA_AMF_HA_ACTIVE)
		err =
		    create_runtime_object((char *)rsc_name->value, rsc_info->saf_rsc_creation_time,
					  gld_cb->immOiHandle);

	if (err != SA_AIS_OK) {
		LOG_ER("create_runtime_object failed %u\n", err);
		if (ncs_patricia_tree_del(&gld_cb->rsc_map_info, (NCS_PATRICIA_NODE *)rsc_map_info) != NCSCC_RC_SUCCESS) {
			LOG_ER("Patricia tree del failed");
			return NULL;
		}
		if (ncs_patricia_tree_del(&gld_cb->rsc_info_id, (NCS_PATRICIA_NODE *)rsc_info) != NCSCC_RC_SUCCESS) {
			LOG_ER("Patricia tree del failed");
			return NULL;
		}
		m_MMGR_FREE_GLSV_GLD_RSC_MAP_INFO(rsc_map_info);
		m_MMGR_FREE_GLSV_GLD_RSC_INFO(rsc_info);
		return NULL;
	}

	/* Add the node to the resource linked list */
	rsc_info->next = gld_cb->rsc_info;
	if (gld_cb->rsc_info)
		gld_cb->rsc_info->prev = rsc_info;
	gld_cb->rsc_info = rsc_info;

	return rsc_info;
}

/****************************************************************************
 * Name          : gld_find_rsc_by_id
 *
 * Description   : This is function is invoked to create or retrieve a resource
 *                 by its id.
 *
 * Arguments     : gld_cb        - control block
  *
 * Return Values : Pointer to the rsc details
 *
 * Notes         : None.
 *****************************************************************************/
GLSV_GLD_RSC_INFO *gld_find_rsc_by_id(GLSV_GLD_CB *gld_cb, SaLckResourceIdT rsc_id)
{
	GLSV_GLD_RSC_INFO *rsc_info;

	/* Run through the List to find if rsc is already present */
	rsc_info = gld_cb->rsc_info;

	while (rsc_info != NULL) {
		if (rsc_info->rsc_id == rsc_id)
			break;
		rsc_info = rsc_info->next;
	}
	if (rsc_info)
		return rsc_info;
	else
		return NULL;
}

/****************************************************************************
 * Name          : gld_find_add_rsc_name
 *
 * Description   : This is function is invoked to create or retrieve a resource
 *                 by its name.
 *
 * Arguments     : gld_cb        - control block
                   rsc_name      - Resource that is being referred to
  *
 * Return Values : Pointer to the rsc details
 *
 * Notes         : None.
 *****************************************************************************/
GLSV_GLD_RSC_INFO *gld_find_add_rsc_name(GLSV_GLD_CB *gld_cb,
					 SaNameT *rsc_name,
					 SaLckResourceIdT rsc_id, SaLckResourceOpenFlagsT flag, SaAisErrorT *error)
{
	GLSV_GLD_RSC_INFO *rsc_info;
	SaAisErrorT ret_error;

	/* Run through the List to find if rsc is already present */
	rsc_info = gld_cb->rsc_info;
	*error = SA_AIS_OK;

	while (rsc_info != NULL) {
		if (!memcmp(rsc_name, &rsc_info->lck_name, sizeof(SaNameT)))
			break;
		rsc_info = rsc_info->next;
	}
	if (gld_cb->ha_state == SA_AMF_HA_ACTIVE) {
		if (rsc_info == NULL && ((flag & SA_LCK_RESOURCE_CREATE) != SA_LCK_RESOURCE_CREATE)) {
			*error = SA_AIS_ERR_NOT_EXIST;
			return NULL;
		}
	}

	if (rsc_info != NULL) {
		rsc_info->saf_rsc_no_of_users = rsc_info->saf_rsc_no_of_users + 1;
		return rsc_info;
	} else {
		rsc_info = gld_add_rsc_info(gld_cb, rsc_name, rsc_id, &ret_error);
		*error = ret_error;
		return rsc_info;
	}
}

/****************************************************************************
 * Name          : gld_free_rsc_info
 *
 * Description   : This function is invoked to free a resource
 *                 by its name.
 *
 * Arguments     : gld_cb        - control block
 *                 rsc_info      - Resource that is being referred to
 *
 * Return Values : Pointer to the rsc details
 *
 * Notes         : None.
 *****************************************************************************/
void gld_free_rsc_info(GLSV_GLD_CB *gld_cb, GLSV_GLD_RSC_INFO *rsc_info)
{
	GLSV_GLD_RSC_MAP_INFO *rsc_map_info = NULL;
	SaNameT lck_name;
	SaNameT immObj_name;

	memset(&lck_name, '\0', sizeof(SaNameT));
	memset(&immObj_name, '\0', sizeof(SaNameT));

	/* Some node is still referring to this resource, so backout */
	if (rsc_info->node_list != NULL)
		return;

	/* Free the node from the resource linked list */
	if (rsc_info->prev != NULL)
		rsc_info->prev->next = rsc_info->next;
	else
		gld_cb->rsc_info = rsc_info->next;

	if (rsc_info->next != NULL)
		rsc_info->next->prev = rsc_info->prev;
	memcpy(&lck_name, &rsc_info->lck_name, sizeof(SaNameT));
	lck_name.length = m_NCS_OS_HTONS(lck_name.length);

	memcpy(&immObj_name, &rsc_info->lck_name, sizeof(SaNameT));
	/* delete imm runtime object */
	if (gld_cb->ha_state == SA_AMF_HA_ACTIVE) {
		if (immutil_saImmOiRtObjectDelete(gld_cb->immOiHandle, &immObj_name) != SA_AIS_OK) {
			LOG_ER("Deleting run time object %s FAILED", lck_name.value);
			return;
		}
	}
	rsc_map_info = (GLSV_GLD_RSC_MAP_INFO *)ncs_patricia_tree_get(&gld_cb->rsc_map_info, (uint8_t *)(uint8_t *)&lck_name);
	if (rsc_map_info) {
		if (ncs_patricia_tree_del(&gld_cb->rsc_map_info, (NCS_PATRICIA_NODE *)rsc_map_info) != NCSCC_RC_SUCCESS) {
			LOG_ER("Patricia tree del failed");
			return;
		}
		m_MMGR_FREE_GLSV_GLD_RSC_MAP_INFO(rsc_map_info);

	}
	if (rsc_info->reelection_timer.tmr_id != TMR_T_NULL)
		gld_stop_tmr(&rsc_info->reelection_timer);

	/* Delete this node from the global resource tree */
	if (ncs_patricia_tree_del(&gld_cb->rsc_info_id, (NCS_PATRICIA_NODE *)rsc_info) != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree del failed");
	}

	m_MMGR_FREE_GLSV_GLD_RSC_INFO(rsc_info);
	return;
}

/****************************************************************************
 * Name          :  gld_snd_master_status 
 *
 * Description   : This function broadcasts new mastership information for a
 *                 resource
 *
 * Arguments     : gld_cb        - control block
 *                 rsc_info      - Resource that is being referred to
 *
 * Return Values : none
 *
 * Notes         : None.
 *****************************************************************************/
void gld_snd_master_status(GLSV_GLD_CB *gld_cb, GLSV_GLD_RSC_INFO *rsc_info, uint32_t status)
{
	GLSV_GLND_EVT glnd_evt;
	NCSMDS_INFO snd_mds;
	uint32_t res;

	memset(&snd_mds, '\0', sizeof(NCSMDS_INFO));
	/*TBD need to check rsc_info */

	if (rsc_info->node_list == NULL) {
		LOG_ER("error in designating new master: rsc_id %u",
				   rsc_info->rsc_id);
		return;
	}

	/* Send the details to the glnd */
	memset(&glnd_evt, 0, sizeof(GLSV_GLND_EVT));
	glnd_evt.type = GLSV_GLND_EVT_RSC_NEW_MASTER;
	glnd_evt.info.new_master_info.rsc_id = rsc_info->rsc_id;;
	glnd_evt.info.new_master_info.master_dest_id = rsc_info->node_list->dest_id;
	glnd_evt.info.new_master_info.orphan = rsc_info->can_orphan;
	glnd_evt.info.new_master_info.orphan_lck_mode = rsc_info->orphan_lck_mode;
	glnd_evt.info.new_master_info.status = status;

	snd_mds.i_mds_hdl = gld_cb->mds_handle;
	snd_mds.i_svc_id = NCSMDS_SVC_ID_GLD;
	snd_mds.i_op = MDS_SEND;
	snd_mds.info.svc_send.i_msg = (NCSCONTEXT)&glnd_evt;
	snd_mds.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
	snd_mds.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
	snd_mds.info.svc_send.i_sendtype = MDS_SENDTYPE_BCAST;
	snd_mds.info.svc_send.info.bcast.i_bcast_scope = NCSMDS_SCOPE_NONE;

	res = ncsmds_api(&snd_mds);
	if (res != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS Send failed");
		return;
	}
	return;
}

/****************************************************************************
 * Name          : gld_rsc_rmv_node_ref
 *
 * Description   : This function is invoked when a node stops referring to a 
 *                 resource. The function will also check if the master for this
 *                 resource is getting modified, if so it will generate an event
 *                 indicating the new master
 *
 * Arguments     : gld_cb        - control block
 *                 rsc_info      - Resource that is being referred to
 *                 node_details  - node referring to this rsc
 *
 * Return Values : none
 *
 * Notes         : None.
 *****************************************************************************/
void gld_rsc_rmv_node_ref(GLSV_GLD_CB *gld_cb, GLSV_GLD_RSC_INFO *rsc_info,
			  GLSV_GLD_GLND_RSC_REF *glnd_rsc, GLSV_GLD_GLND_DETAILS *node_details, bool orphan_flag)
{
	GLSV_NODE_LIST **node_list, *free_node_list = NULL;
	bool chg_master = false;

	if (glnd_rsc == NULL || rsc_info == NULL) {
		return;
	}
	if (rsc_info->node_list->node_id == node_details->node_id)
		chg_master = true;

	/* rmv the references to this resource by the mentioned node */
	node_list = &rsc_info->node_list;
	while (*node_list != NULL) {
		if ((*node_list)->node_id == node_details->node_id) {
			free_node_list = *node_list;
			break;
		}
		node_list = &(*node_list)->next;
	}

	if (*node_list == NULL) {
		LOG_ER("Incorrect state: rsc_id %u node_id %u",
				   rsc_info->rsc_id, node_details->node_id);
	} else {
		*node_list = (*node_list)->next;
		m_MMGR_FREE_GLSV_NODE_LIST(free_node_list);
	}

	if (orphan_flag) {
		rsc_info->can_orphan = false;
	}

	if (ncs_patricia_tree_del(&node_details->rsc_info_tree, (NCS_PATRICIA_NODE *)glnd_rsc) != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree del failed");
	}

	if (rsc_info->node_list != NULL && rsc_info->can_orphan == false)
		glnd_rsc->rsc_info->saf_rsc_no_of_users = glnd_rsc->rsc_info->saf_rsc_no_of_users + 1;	/* In the purge flow we need to increment the number of users beacuse we have already decremented it in finalize flow and again decremented in purge flow which amounts to double decrement */

	m_MMGR_FREE_GLSV_GLD_GLND_RSC_REF(glnd_rsc);

	if (rsc_info->node_list == NULL && rsc_info->can_orphan == false)
		gld_free_rsc_info(gld_cb, rsc_info);
	else if (chg_master && (gld_cb->ha_state == SA_AMF_HA_ACTIVE)) {
		/*Start the timer for resource reeelection  */
		rsc_info->reelection_timer.resource_id = rsc_info->rsc_id;
		/* Start GLSV_GLD_GLND_RESTART_TIMEOUT timer */
		gld_start_tmr(gld_cb, &rsc_info->reelection_timer,
			      GLD_TMR_RES_REELECTION_WAIT, GLSV_GLND_MASTER_REELECTION_WAIT_TIME, 0);

		gld_snd_master_status(gld_cb, rsc_info, GLND_RESOURCE_ELECTION_IN_PROGESS);
	}
	return;
}

/****************************************************************************
 * Name          : gld_rsc_add_node_ref
 *
 * Description   : This function is invoked when a node refers to a resource
 *                 through a resource open request.
 *
 * Arguments     : gld_cb        - control block
 *                 rsc_info      - Resource that is being referred to
 *                 node_details  - node referring to this rsc
 *
 * Return Values : none
 *
 * Notes         : None.
 *****************************************************************************/
void gld_rsc_add_node_ref(GLSV_GLD_CB *gld_cb, GLSV_GLD_GLND_DETAILS *node_details, GLSV_GLD_RSC_INFO *rsc_info)
{
	GLSV_GLD_GLND_RSC_REF *glnd_rsc;
	/* Dont do anything if we are already referring to this resource */
	if (ncs_patricia_tree_get(&node_details->rsc_info_tree, (uint8_t *)&rsc_info->rsc_id)
	    != NULL)
		return;

	glnd_rsc = m_MMGR_ALLOC_GLSV_GLD_GLND_RSC_REF;
	if (!glnd_rsc) {
		LOG_CR("Rsc info alloc failed: Error %s", strerror(errno));
		assert(0);
	}

	memset(glnd_rsc, 0, sizeof(GLSV_GLD_GLND_RSC_REF));
	glnd_rsc->rsc_id = rsc_info->rsc_id;
	glnd_rsc->rsc_info = rsc_info;
	glnd_rsc->pat_node.key_info = (uint8_t *)&glnd_rsc->rsc_id;
	if (ncs_patricia_tree_add(&node_details->rsc_info_tree, &glnd_rsc->pat_node)
	    != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree add failed");
		m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);
		return;
	}

	return;
}
