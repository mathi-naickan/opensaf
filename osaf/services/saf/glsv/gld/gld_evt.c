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

#include "gld.h"

/*****************************************************************************
  FILE NAME: GLD_EVT.C

  DESCRIPTION: GLD events received and send related routines.

  FUNCTIONS INCLUDED in this module:
  
******************************************************************************/
static uint32_t gld_rsc_open(GLSV_GLD_EVT *evt);
static uint32_t gld_rsc_close(GLSV_GLD_EVT *evt);
static uint32_t gld_rsc_set_orphan(GLSV_GLD_EVT *evt);
static uint32_t gld_glnd_operational(GLSV_GLD_EVT *evt);
static uint32_t gld_send_res_master_info(GLSV_GLD_CB *gld_cb, GLSV_GLD_GLND_DETAILS *node_details, MDS_DEST dest_id);
static uint32_t gld_mds_glnd_down(GLSV_GLD_EVT *evt);
static uint32_t gld_debug_dump_cb(GLSV_GLD_EVT *evt);
GLSV_GLD_GLND_DETAILS *gld_add_glnd_node(GLSV_GLD_CB *gld_cb, MDS_DEST glnd_mds_dest);
static uint32_t gld_process_tmr_resource_reelection_wait_timeout(GLSV_GLD_EVT *evt);
static uint32_t gld_process_tmr_node_restart_wait_timeout(GLSV_GLD_EVT *evt);
static uint32_t gld_quisced_process(GLSV_GLD_EVT *evt);
static uint32_t gld_process_send_non_master_status(GLSV_GLD_CB *gld_cb, GLSV_GLD_GLND_DETAILS *node_details, uint32_t status);
static uint32_t gld_process_send_non_master_info(GLSV_GLD_CB *gld_cb, GLSV_GLD_GLND_RSC_REF *glnd_rsc,
					      GLSV_GLD_GLND_DETAILS *node_details, uint32_t status);

/* GLD dispatch table */
static const
GLSV_GLD_EVT_HANDLER gld_evt_dispatch_tbl[GLSV_GLD_EVT_MAX - GLSV_GLD_EVT_BASE] = {
	gld_rsc_open,
	gld_rsc_close,
	gld_rsc_set_orphan,
	gld_mds_glnd_down,
	gld_glnd_operational,
	gld_debug_dump_cb,
	gld_process_tmr_resource_reelection_wait_timeout,
	gld_process_tmr_node_restart_wait_timeout,
	gld_quisced_process
};

/****************************************************************************
 * Name          : gld_process_evt
 *
 * Description   : This is the function which is called when gld receives any
 *                 event
 *
 * Arguments     : evt  - Event that was posted to the GLD Mail box
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t gld_process_evt(GLSV_GLD_EVT *evt)
{
	GLSV_GLD_CB *gld_cb = evt->gld_cb;
	TRACE_ENTER2("Event type: %d", evt->evt_type);

	if (gld_cb->ha_state == SA_AMF_HA_ACTIVE) {
		if (gld_evt_dispatch_tbl[evt->evt_type] (evt) != NCSCC_RC_SUCCESS) {
			TRACE_2("Event processing failed");
		}
	}
	if (gld_cb->ha_state == SA_AMF_HA_STANDBY) {
		if (evt->evt_type == GLSV_GLD_EVT_GLND_DOWN || evt->evt_type == GLSV_GLD_EVT_RESTART_TIMEOUT) {
			if (gld_evt_dispatch_tbl[evt->evt_type] (evt) != NCSCC_RC_SUCCESS) {
				LOG_ER("Event processing failed");
			}
		}
	}

	gld_evt_destroy(evt);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : gld_evt_destroy
 *
 * Description   : This is the function which is used to free the event 
 *                 pointer which it has received.
 *
 * Arguments     : evt  - This is the pointer which holds the  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void gld_evt_destroy(GLSV_GLD_EVT *evt)
{
	switch (evt->evt_type) {
	default:
		m_MMGR_FREE_GLSV_GLD_EVT(evt);
		break;
	}
	return;
}

/****************************************************************************
 * Name          : gld_rsc_open
 *
 * Description   : This is the function is invoked when a rsc_open event is
 *                 is sent from a GLND. This function will assign a resource id
 *                 and send the information to the GLND
 *
 * Arguments     : evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t gld_rsc_open(GLSV_GLD_EVT *evt)
{
	GLSV_GLD_CB *gld_cb = evt->gld_cb;
	GLSV_GLD_RSC_INFO *rsc_info;
	GLSV_GLD_GLND_DETAILS *node_details;
	GLSV_NODE_LIST *node_list, **tmp_node_list;
	GLSV_GLND_EVT glnd_evt;
	NCSMDS_INFO snd_mds;
	uint32_t res = NCSCC_RC_FAILURE;;
	SaAisErrorT error;
	uint32_t node_id;
	bool node_first_rsc_open = false;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc = NULL;
	bool orphan_flag = false;
	TRACE_ENTER2("component name %s", gld_cb->comp_name.value);

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest_id);
	memset(&snd_mds, '\0', sizeof(NCSMDS_INFO));
	memset(&glnd_evt, '\0', sizeof(GLSV_GLND_EVT));

	if ((evt == GLSV_GLD_EVT_NULL) || (gld_cb == NULL))
		goto end;

	/* Find if the node details are available */
	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uint8_t *)&node_id)) == NULL) {
		node_first_rsc_open = true;
		if ((node_details = gld_add_glnd_node(gld_cb, evt->fr_dest_id)) == NULL)
			goto end;
	}

	rsc_info = gld_find_add_rsc_name(gld_cb, &evt->info.rsc_open_info.rsc_name,
					 evt->info.rsc_open_info.rsc_id, evt->info.rsc_open_info.flag, &error);

	if (rsc_info == NULL) {
		/* based on the error - pass on the info to the glnd */
		glnd_evt.type = GLSV_GLND_EVT_RSC_GLD_DETAILS;
		glnd_evt.info.rsc_gld_info.rsc_name = evt->info.rsc_open_info.rsc_name;
		glnd_evt.info.rsc_gld_info.error = error;

		snd_mds.i_mds_hdl = gld_cb->mds_handle;
		snd_mds.i_svc_id = NCSMDS_SVC_ID_GLD;
		snd_mds.i_op = MDS_SEND;
		snd_mds.info.svc_send.i_msg = (NCSCONTEXT)&glnd_evt;
		snd_mds.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
		snd_mds.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
		snd_mds.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
		snd_mds.info.svc_send.info.snd.i_to_dest = node_details->dest_id;

		res = ncsmds_api(&snd_mds);
		if (res != NCSCC_RC_SUCCESS) {
			LOG_ER("MDS Send failed");
		}
		goto err1;
	}
	TRACE_1("EVT Processing rsc open: rsc_id %u node_id %u", rsc_info->rsc_id, node_details->node_id);

	gld_rsc_add_node_ref(gld_cb, node_details, rsc_info);

	/* Now add this node to the list of nodes referring this resource */
	node_list = rsc_info->node_list;
	tmp_node_list = &rsc_info->node_list;
	while (node_list != NULL) {
		if (node_list->node_id == node_id)
			break;
		tmp_node_list = &node_list->next;
		node_list = node_list->next;
	}

	if (node_list == NULL) {
		node_list = m_MMGR_ALLOC_GLSV_NODE_LIST;
		memset(node_list, 0, sizeof(GLSV_NODE_LIST));
		node_list->dest_id = node_details->dest_id;
		node_list->node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest_id);
		*tmp_node_list = node_list;

		/* Send the details to the glnd */
		glnd_evt.type = GLSV_GLND_EVT_RSC_GLD_DETAILS;
		glnd_evt.info.rsc_gld_info.master_dest_id = rsc_info->node_list->dest_id;
		glnd_evt.info.rsc_gld_info.rsc_id = rsc_info->rsc_id;
		glnd_evt.info.rsc_gld_info.rsc_name = rsc_info->lck_name;
		glnd_evt.info.rsc_gld_info.can_orphan = rsc_info->can_orphan;
		glnd_evt.info.rsc_gld_info.orphan_mode = rsc_info->orphan_lck_mode;
		glnd_evt.info.rsc_gld_info.error = SA_AIS_OK;

		snd_mds.i_mds_hdl = gld_cb->mds_handle;
		snd_mds.i_svc_id = NCSMDS_SVC_ID_GLD;
		snd_mds.i_op = MDS_SEND;
		snd_mds.info.svc_send.i_msg = (NCSCONTEXT)&glnd_evt;
		snd_mds.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
		snd_mds.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
		snd_mds.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
		snd_mds.info.svc_send.info.snd.i_to_dest = node_details->dest_id;

		res = ncsmds_api(&snd_mds);
		if (res != NCSCC_RC_SUCCESS) {
			LOG_ER("MDS Send failed");
			goto err2;
		}
	}

	/* checkpoint resource */
	glsv_gld_a2s_ckpt_resource(gld_cb, &rsc_info->lck_name, rsc_info->rsc_id, evt->fr_dest_id,
				   rsc_info->saf_rsc_creation_time);

	res = NCSCC_RC_SUCCESS;
	goto end;
 err2:

	glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_get(&node_details->rsc_info_tree,
								  (uint8_t *)&rsc_info->rsc_id);
	if ((glnd_rsc == NULL) || (glnd_rsc->rsc_info == NULL)) {
		LOG_ER("Rsc operation for unopened rsc: rsc_id %u node_id %u",
				   rsc_info->rsc_id, node_details->node_id);
		goto end;
	}

	if (glnd_rsc->rsc_info->saf_rsc_no_of_users > 0)
		glnd_rsc->rsc_info->saf_rsc_no_of_users = glnd_rsc->rsc_info->saf_rsc_no_of_users - 1;

	gld_rsc_rmv_node_ref(gld_cb, glnd_rsc->rsc_info, glnd_rsc, node_details, orphan_flag);
 err1:
	if (node_first_rsc_open) {
		/* Now delete this node details node */
		if (ncs_patricia_tree_del(&gld_cb->glnd_details, (NCS_PATRICIA_NODE *)node_details) != NCSCC_RC_SUCCESS) {
			LOG_ER("Patricia tree del failed: node_id %u ", node_details->node_id);
		} else {
			m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);
			TRACE("Node getting removed on active: node_id %u", node_id);
		}
	}
	res = NCSCC_RC_FAILURE;
 end:
	TRACE_LEAVE2("return value %u", res);
	return res;

}

/****************************************************************************
 * Name          : gld_rsc_close
 *
 * Description   : This is the function is invoked when a rsc_close event is
 *                 is sent from a GLND. This function will remove references to
 *                 to this resource from the mentioned node. If the resource 
 *                 is not referred any longer then the data structures are freed
 *                 up.
 *
 * Arguments     : evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t gld_rsc_close(GLSV_GLD_EVT *evt)
{
	GLSV_GLD_CB *gld_cb = evt->gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc;
	bool orphan_flag;
	uint32_t node_id;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("component name %s", gld_cb->comp_name.value);

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest_id);

	if ((evt == GLSV_GLD_EVT_NULL) || (gld_cb == NULL)){
		rc = NCSCC_RC_FAILURE;
		goto end;
	}

	orphan_flag = evt->info.rsc_details.orphan;

	/* Find if the node details are available */
	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uint8_t *)&node_id)) == NULL) {
		LOG_ER("Event from unknown glnd: node_id %u ", node_id);
		rc = NCSCC_RC_FAILURE;
		goto end;
	}

	glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_get(&node_details->rsc_info_tree,
								  (uint8_t *)&evt->info.rsc_details.rsc_id);
	if ((glnd_rsc == NULL) || (glnd_rsc->rsc_info == NULL)) {
		LOG_ER("Rsc operation for unopened rsc: rsc_id %u node_id %u ",
				   evt->info.rsc_details.rsc_id, node_details->node_id);
		goto end;
	}

	TRACE("EVT Processing rsc close rsc_id %u node_id %u", glnd_rsc->rsc_info->rsc_id,
		      node_details->node_id);

	if (glnd_rsc->rsc_info->saf_rsc_no_of_users > 0)
		glnd_rsc->rsc_info->saf_rsc_no_of_users = glnd_rsc->rsc_info->saf_rsc_no_of_users - 1;

	/*Checkkpoint resource close event */
	glsv_gld_a2s_ckpt_rsc_details(gld_cb, evt->evt_type, evt->info.rsc_details, node_details->dest_id,
				      evt->info.rsc_details.lcl_ref_cnt);

	if (evt->info.rsc_details.lcl_ref_cnt == 0)
		gld_rsc_rmv_node_ref(gld_cb, glnd_rsc->rsc_info, glnd_rsc, node_details, orphan_flag);
 end:	
	TRACE_LEAVE2("Return value %u", rc);
	return rc;
}

/****************************************************************************
 * Name          : gld_rsc_set_orphan
 *
 * Description   : Instruction from the GLND to set the orphan flag
 *
 * Arguments     : evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t gld_rsc_set_orphan(GLSV_GLD_EVT *evt)
{
	GLSV_GLD_CB *gld_cb = evt->gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details;
	uint32_t node_id;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest_id);

	if ((evt == GLSV_GLD_EVT_NULL) || (gld_cb == NULL))
		goto end;

	/* Find if the node details are available */
	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uint8_t *)&node_id)) == NULL) {
		LOG_ER("Event from unknown glnd: node_id %u", node_id);
		goto end;
	}

	TRACE("EVT Processing set orphan: node_id %u ", node_details->node_id);

	if (gld_rsc_ref_set_orphan
	    (node_details, evt->info.rsc_details.rsc_id, evt->info.rsc_details.orphan,
	     evt->info.rsc_details.lck_mode) == NCSCC_RC_SUCCESS) {
		/* Checkpoint rsc_details */
		glsv_gld_a2s_ckpt_rsc_details(gld_cb, evt->evt_type, evt->info.rsc_details, node_details->dest_id,
					      evt->info.rsc_details.lcl_ref_cnt);
		rc = NCSCC_RC_SUCCESS;
	} 
		
 end:	
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          :gld_rsc_ref_set_orphan
 *
 * Description   :
 *
 * Arguments     :
 *
 * Return Values :
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t gld_rsc_ref_set_orphan(GLSV_GLD_GLND_DETAILS *node_details, SaLckResourceIdT rsc_id, bool orphan,
			     SaLckLockModeT lck_mode)
{
	GLSV_GLD_GLND_RSC_REF *glnd_rsc_ref;

	/* Find the rsc_info based on resource id */
	glnd_rsc_ref = (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_get(&node_details->rsc_info_tree, (uint8_t *)&rsc_id);
	if ((glnd_rsc_ref == NULL) || (glnd_rsc_ref->rsc_info == NULL)) {
		LOG_ER("Patricia tree get failed");
		return NCSCC_RC_FAILURE;
	}

	glnd_rsc_ref->rsc_info->can_orphan = orphan;
	glnd_rsc_ref->rsc_info->orphan_lck_mode = lck_mode;
	if (orphan == true)
		glnd_rsc_ref->rsc_info->saf_rsc_stripped_cnt++;

	return NCSCC_RC_SUCCESS;



}

/****************************************************************************
 * Name          : gld_glnd_operational
 *
 * Description   :
 *
 * Arguments     : evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t gld_glnd_operational(GLSV_GLD_EVT *evt)
{
	GLSV_GLD_CB *gld_cb = evt->gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	GLSV_GLD_RSC_INFO *rsc_info = NULL;
	uint32_t node_id;
	GLSV_NODE_LIST *node_list = NULL;
	TRACE_ENTER();

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest_id);
	/* Find if the node details are already  available */
	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uint8_t *)&node_id)) != NULL) {
		memcpy(&node_details->dest_id, &evt->fr_dest_id, sizeof(MDS_DEST));
		/* Cancel the restart timer if started */
		gld_stop_tmr(&node_details->restart_timer);
		node_details->status = GLND_OPERATIONAL_STATE;
		rsc_info = gld_cb->rsc_info;

		while (rsc_info != NULL) {
			node_list = rsc_info->node_list;
			while (node_list != NULL) {
				if (node_list->node_id == node_id) {
					memcpy(&node_list->dest_id, &evt->fr_dest_id, sizeof(MDS_DEST));
				}
				node_list = node_list->next;
			}
			rsc_info = rsc_info->next;
		}
		glsv_gld_a2s_ckpt_node_details(gld_cb, node_details->dest_id, GLSV_GLD_EVT_GLND_OPERATIONAL);

		rsc_info = gld_cb->rsc_info;
		while (rsc_info != NULL) {
			if (rsc_info->node_list->node_id == node_details->node_id)
				gld_snd_master_status(gld_cb, rsc_info, GLND_RESOURCE_MASTER_OPERATIONAL);
			rsc_info = rsc_info->next;
		}

		/* If this node is non master for any resource, then send node status to the master */
		gld_process_send_non_master_status(gld_cb, node_details, GLND_OPERATIONAL_STATE);

	}
	/*Send resource-master information to GLND */
	gld_send_res_master_info(gld_cb, node_details, evt->fr_dest_id);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : gld_send_res_master_info 
 *
 * Description   : This function sends resource-master details to the GLND after it restarts 
 *
 * Arguments     : GLSV_GLD_CB, GLSV_GLD_GLND_DETAILS
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t gld_send_res_master_info(GLSV_GLD_CB *gld_cb, GLSV_GLD_GLND_DETAILS *node_details, MDS_DEST dest_id)
{
	GLSV_GLD_GLND_RSC_REF *glnd_rsc = NULL;
	GLSV_GLD_GLND_DETAILS *master_node_details = NULL;
	GLSV_GLND_EVT glnd_evt;
	NCSMDS_INFO snd_mds;
	uint32_t index = 0;
	uint32_t no_of_glnd_res = 0;
	GLSV_NODE_LIST *temp_node_list = NULL;
	GLSV_GLD_GLND_DETAILS *non_master_node_details = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (gld_cb == NULL)
		goto end;

	memset(&snd_mds, '\0', sizeof(NCSMDS_INFO));
	memset(&glnd_evt, '\0', sizeof(GLSV_GLND_EVT));

	glnd_evt.type = GLSV_GLND_EVT_RSC_MASTER_INFO;

	if (node_details) {
		glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uint8_t *)0);
		while (glnd_rsc) {
			no_of_glnd_res++;
			glnd_rsc =
			    (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree,
									       (uint8_t *)&glnd_rsc->rsc_id);
		}

		glnd_evt.info.rsc_master_info.no_of_res = no_of_glnd_res;

		if (glnd_evt.info.rsc_master_info.no_of_res > 0) {
			glnd_evt.info.rsc_master_info.rsc_master_list =
			    m_MMGR_ALLOC_GLND_RES_MASTER_LIST_INFO(no_of_glnd_res);
			if (glnd_evt.info.rsc_master_info.rsc_master_list == NULL) {
				LOG_CR("Memory Alloc Failure: Error %s", strerror(errno));	/* Log the error */
				assert(0);
			}
			memset(glnd_evt.info.rsc_master_info.rsc_master_list, 0,
			       (sizeof(GLSV_GLND_RSC_MASTER_INFO_LIST) * no_of_glnd_res));

			glnd_rsc =
			    (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uint8_t *)0);

			while (glnd_rsc) {
				/* Get the master node for this resource */
				master_node_details =
				    (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
										   (uint8_t *)&glnd_rsc->
										   rsc_info->node_list->node_id);
				if (master_node_details) {
					glnd_evt.info.rsc_master_info.rsc_master_list[index].rsc_id = glnd_rsc->rsc_id;
					glnd_evt.info.rsc_master_info.rsc_master_list[index].master_dest_id =
					    glnd_rsc->rsc_info->node_list->dest_id;
					glnd_evt.info.rsc_master_info.rsc_master_list[index].master_status =
					    master_node_details->status;
					index++;

					/*If this node is master for this resource, send all the non masters info of this resource to the master */
					if (master_node_details->node_id == node_details->node_id) {
						temp_node_list = glnd_rsc->rsc_info->node_list->next;
						while (temp_node_list) {
							non_master_node_details = (GLSV_GLD_GLND_DETAILS *)
							    ncs_patricia_tree_get(&gld_cb->glnd_details, (uint8_t *)
										  &temp_node_list->node_id);
							if (non_master_node_details) {
								gld_process_send_non_master_info(gld_cb, glnd_rsc,
												 non_master_node_details,
												 non_master_node_details->status);
							} else {
								LOG_ER("Patricia tree get failed: node_id %u",
										   temp_node_list->node_id);
							}
							temp_node_list = temp_node_list->next;
						}
					}
				} else {
					LOG_ER("Patricia tree get failed: node_id %u ", glnd_rsc->rsc_info->node_list->node_id);
				}

				glnd_rsc =
				    (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree,
										       (uint8_t *)&glnd_rsc->rsc_id);
			}
		}
	}

	snd_mds.i_mds_hdl = gld_cb->mds_handle;
	snd_mds.i_svc_id = NCSMDS_SVC_ID_GLD;
	snd_mds.i_op = MDS_SEND;
	snd_mds.info.svc_send.i_msg = (NCSCONTEXT)&glnd_evt;
	snd_mds.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
	snd_mds.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
	snd_mds.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	snd_mds.info.svc_send.info.snd.i_to_dest = dest_id;

	if (ncsmds_api(&snd_mds) != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS Send failed");
		rc = NCSCC_RC_FAILURE;
		goto end;
	}
	if (no_of_glnd_res > 0)
		m_MMGR_FREE_GLND_RES_MASTER_LIST_INFO(glnd_evt.info.rsc_master_info.rsc_master_list);
 end:
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
 * Name          : gld_mds_glnd_down
 *
 * Description   : MDS indicated that a glnd has gone down
 *
 * Arguments     : evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t gld_mds_glnd_down(GLSV_GLD_EVT *evt)
{
	GLSV_GLD_CB *gld_cb = evt->gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	GLSV_GLD_RSC_INFO *rsc_info;
	uint32_t node_id;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER2("mds identification %u",gld_cb->my_dest_id );

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt->info.glnd_mds_info.mds_dest_id);

	if ((evt == GLSV_GLD_EVT_NULL) || (gld_cb == NULL))
		goto end;

	memcpy(&evt->fr_dest_id, &evt->info.glnd_mds_info.mds_dest_id, sizeof(MDS_DEST)
	    );

	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uint8_t *)&node_id)) == NULL) {
		TRACE_1("Resource details is empty for glnd on node_id %u ", node_id);
		rc = NCSCC_RC_SUCCESS;
		goto end;
	}
	node_details->status = GLND_RESTART_STATE;

	TRACE("EVT Processing MDS GLND DOWN: node_id %u", node_details->node_id);
	memcpy(&node_details->restart_timer.mdest_id, &node_details->dest_id, sizeof(MDS_DEST));

	/* Start GLSV_GLD_GLND_RESTART_TIMEOUT timer */
	gld_start_tmr(gld_cb, &node_details->restart_timer, GLD_TMR_NODE_RESTART_TIMEOUT, GLD_NODE_RESTART_TIMEOUT, 0);

	/* Check whether this node is master for any resource, if yes send the status to all
	   the
	   non master nodes */
	if (gld_cb->ha_state == SA_AMF_HA_ACTIVE) {
		/* Check whether this node is master for any resource, if yes send the status to all the non master nodes */
		rsc_info = gld_cb->rsc_info;
		while (rsc_info != NULL) {
			if (rsc_info->node_list) {
				if (rsc_info->node_list->node_id == node_details->node_id)
					gld_snd_master_status(gld_cb, rsc_info, GLND_RESOURCE_MASTER_RESTARTED);
			}
			rsc_info = rsc_info->next;
		}

		/* If this node is non master for any resource, then send node status to the master */
		gld_process_send_non_master_status(gld_cb, node_details, GLND_RESTART_STATE);

	}
 end:
	TRACE_LEAVE2("Return value: %u", rc);
	return rc;
}

/****************************************************************************
 * Name          : gld_quisced_process
 *
 * Description   : Instruction from the GLND to set the orphan flag
 *
 * Arguments     : evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t gld_quisced_process(GLSV_GLD_EVT *evt)
{
	GLSV_GLD_CB *gld_cb = evt->gld_cb;
	SaAisErrorT saErr = SA_AIS_OK;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if ((evt == GLSV_GLD_EVT_NULL) || (gld_cb == NULL)){
		rc = NCSCC_RC_FAILURE;
		goto end;
	}

	if (gld_cb->is_quiasced) {
		gld_cb->ha_state = SA_AMF_HA_QUIESCED;
		/* Give up our IMM OI implementer role */
		rc = immutil_saImmOiImplementerClear(gld_cb->immOiHandle);
		if (rc != SA_AIS_OK) {
			LOG_ER("saImmOiImplementerClear failed: err = %d", rc);
		}
		gld_cb->is_impl_set = false;
		rc = glsv_gld_mbcsv_chgrole(gld_cb);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LEAP_DBG_SINK_VOID;
			goto end;
		}
		saAmfResponse(gld_cb->amf_hdl, gld_cb->invocation, saErr);
		gld_cb->is_quiasced = false;
	}
 end:
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
 * Name          : gld_add_glnd_node
 * Description   : Adds node_details info to the gld_cb and initilizes
                   node_details->rsc_info_tree
 *
 * Arguments     :GLSV_GLD_CB, glnd_mds_dest
 *
 * Return Values :
 *
 * Notes         : None.
 *****************************************************************************/
GLSV_GLD_GLND_DETAILS *gld_add_glnd_node(GLSV_GLD_CB *gld_cb, MDS_DEST glnd_mds_dest)
{
	GLSV_GLD_GLND_DETAILS *node_details;
	NCS_PATRICIA_PARAMS params = { sizeof(uint32_t) };
	TRACE_ENTER2("glnd_mds_dest %" PRIx64, glnd_mds_dest);

	/* Need to add the node details */
	node_details = m_MMGR_ALLOC_GLSV_GLD_GLND_DETAILS;
	if (node_details == NULL) {
		LOG_CR("Node details alloc failed: Error %s", strerror(errno));
		assert(0);
	}
	memset(node_details, 0, sizeof(GLSV_GLD_GLND_DETAILS));

	memcpy(&node_details->dest_id, &glnd_mds_dest, sizeof(MDS_DEST));
	node_details->node_id = m_NCS_NODE_ID_FROM_MDS_DEST(glnd_mds_dest);
	node_details->status = GLND_OPERATIONAL_STATE;

	TRACE("EVT Processing MDS GLND UP: node_id %u", node_details->node_id);

	/* Initialize the pat tree for resource info */
	if ((ncs_patricia_tree_init(&node_details->rsc_info_tree, &params))
	    != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree init failed");
		m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);
		node_details = NULL;
		goto end;
	}
	node_details->pat_node.key_info = (uint8_t *)&node_details->node_id;
	if (ncs_patricia_tree_add(&gld_cb->glnd_details, &node_details->pat_node)
	    != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree add failed");
		m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);
		node_details = NULL;
		goto end;
	}
 end:
	TRACE_LEAVE2("Node id %u",node_details->node_id);
	return node_details;
}

/*****************************************************************************
  PROCEDURE NAME : gld_process_tmr_resource_reelection_wait_timeout

  DESCRIPTION    : Process the GLSV_GLD_EVT_RES_REELECTION_TIMEOUT event

  ARGUMENTS      :gld_cb      - ptr to the GLD control block
                  evt          - ptr to the event.

  RETURNS        :NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS

  NOTES         : None
*****************************************************************************/
static uint32_t gld_process_tmr_resource_reelection_wait_timeout(GLSV_GLD_EVT *evt)
{
	GLSV_GLD_CB *gld_cb = evt->gld_cb;
	uint32_t res_id = (uint32_t)evt->info.tmr.resource_id;
	GLSV_GLD_RSC_INFO *res_node;

	res_node = gld_find_rsc_by_id(gld_cb, res_id);
	if (res_node)
		gld_snd_master_status(gld_cb, res_node, GLND_RESOURCE_ELECTION_COMPLETED);
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : gld_process_tmr_node_restart_wait_timeout

  DESCRIPTION    :

  ARGUMENTS      :gld_cb      - ptr to the GLD control block
                  evt          - ptr to the event.

  RETURNS        :NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS

  NOTES         : None
*****************************************************************************/
static uint32_t gld_process_tmr_node_restart_wait_timeout(GLSV_GLD_EVT *evt)
{
	GLSV_GLD_CB *gld_cb = evt->gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc;
	SaLckResourceIdT rsc_id;
	uint32_t node_id;

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt->info.tmr.mdest_id);
	TRACE_ENTER2("Node restart wait timer expired: node_id %u", node_id);

	if ((node_details =
	     (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details, (uint8_t *)&node_id)) == NULL) {
		LOG_ER("Evenr from unknown glnd: node_id %u", node_id);
		return NCSCC_RC_FAILURE;
	}

	if (gld_cb->ha_state == SA_AMF_HA_ACTIVE) {

		/* checkpoint node_details */
		glsv_gld_a2s_ckpt_node_details(gld_cb, node_details->dest_id, GLSV_GLD_EVT_GLND_DOWN);

		/* If this node is non master for any resource, then send node status to the master */
		gld_process_send_non_master_status(gld_cb, node_details, GLND_DOWN_STATE);

		/* Remove the reference to each of the resource referred by this node */
		glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uint8_t *)0);
		if (glnd_rsc) {
			rsc_id = glnd_rsc->rsc_id;
			while (glnd_rsc) {
				gld_rsc_rmv_node_ref(gld_cb, glnd_rsc->rsc_info, glnd_rsc, node_details,
						     glnd_rsc->rsc_info->can_orphan);
				glnd_rsc =
				    (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree,
										       (uint8_t *)&rsc_id);
				if (glnd_rsc)
					rsc_id = glnd_rsc->rsc_id;
			}
		}

		/* Now delete this node details node */
		if (ncs_patricia_tree_del(&gld_cb->glnd_details, (NCS_PATRICIA_NODE *)node_details) != NCSCC_RC_SUCCESS) {
			LOG_ER("Patricia tree del failed: node_id %u",
					   node_details->node_id);
		} else {
			m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);
			TRACE("Node getting removed on active: node_id %u", node_id);
		}
	} else {
		node_details->status = GLND_DOWN_STATE;
	}
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : gld_process_send_non_master_status

  DESCRIPTION    : This function sends the nonmaster status to all the masters
                   corresponding to the resources opened by this node

  ARGUMENTS      :gld_cb      - ptr to the GLD control block
                  node_details
                  status

  RETURNS        :NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS

  NOTES         : None
*****************************************************************************/
static uint32_t gld_process_send_non_master_status(GLSV_GLD_CB *gld_cb, GLSV_GLD_GLND_DETAILS *node_details, uint32_t status)
{
	GLSV_GLD_GLND_RSC_REF *glnd_rsc = NULL;
	GLSV_GLND_EVT glnd_evt;
	NCSMDS_INFO snd_mds;
	uint32_t res = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uint8_t *)0);
	while (glnd_rsc) {
		if (glnd_rsc->rsc_info->node_list->node_id != node_details->node_id) {
			memset(&glnd_evt, '\0', sizeof(GLSV_GLND_EVT));

			glnd_evt.type = GLSV_GLND_EVT_NON_MASTER_INFO;
			glnd_evt.info.non_master_info.dest_id = node_details->dest_id;
			glnd_evt.info.non_master_info.status = status;

			snd_mds.i_mds_hdl = gld_cb->mds_handle;
			snd_mds.i_svc_id = NCSMDS_SVC_ID_GLD;
			snd_mds.i_op = MDS_SEND;
			snd_mds.info.svc_send.i_msg = (NCSCONTEXT)&glnd_evt;
			snd_mds.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
			snd_mds.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
			snd_mds.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
			snd_mds.info.svc_send.info.snd.i_to_dest = glnd_rsc->rsc_info->node_list->dest_id;

			res = ncsmds_api(&snd_mds);
			if (res != NCSCC_RC_SUCCESS) {
				LOG_ER("MDS Send failed");
				goto end;
			}
		}
		glnd_rsc =
		    (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree,
								       (uint8_t *)&glnd_rsc->rsc_id);
	}
 end:
	TRACE_LEAVE();
	return res;

}

/****************************************************************************
 * Name          : gld_debug_dump_cb
 *
 * Description   : Event to dump the gld control block
 *
 * Arguments     : evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t gld_debug_dump_cb(GLSV_GLD_EVT *evt)
{
	gld_dump_cb();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : gld_process_send_non_master_info
        
  DESCRIPTION    : This function sends the corresponding resource 
                   nonmaster info to the master
        
  ARGUMENTS      :gld_cb      - ptr to the GLD control block
                  glnd_rsc      
                  node_details
                  status
            
  RETURNS        :NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
        
  NOTES         : None
*****************************************************************************/
static uint32_t gld_process_send_non_master_info(GLSV_GLD_CB *gld_cb, GLSV_GLD_GLND_RSC_REF *glnd_rsc,
					      GLSV_GLD_GLND_DETAILS *node_details, uint32_t status)
{
	GLSV_GLND_EVT glnd_evt;
	NCSMDS_INFO snd_mds;
	uint32_t res;
	TRACE_ENTER();

	if (glnd_rsc->rsc_info->node_list->node_id != node_details->node_id) {
		memset(&glnd_evt, '\0', sizeof(GLSV_GLND_EVT));

		glnd_evt.type = GLSV_GLND_EVT_NON_MASTER_INFO;
		glnd_evt.info.non_master_info.dest_id = node_details->dest_id;
		glnd_evt.info.non_master_info.status = status;

		snd_mds.i_mds_hdl = gld_cb->mds_handle;
		snd_mds.i_svc_id = NCSMDS_SVC_ID_GLD;
		snd_mds.i_op = MDS_SEND;
		snd_mds.info.svc_send.i_msg = (NCSCONTEXT)&glnd_evt;
		snd_mds.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
		snd_mds.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
		snd_mds.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
		snd_mds.info.svc_send.info.snd.i_to_dest = glnd_rsc->rsc_info->node_list->dest_id;

		res = ncsmds_api(&snd_mds);
		if (res != NCSCC_RC_SUCCESS) {
			LOG_ER("MDS Send failed");
			goto end;
		}
	}
 end:	
	TRACE_LEAVE();
	return res;

}
