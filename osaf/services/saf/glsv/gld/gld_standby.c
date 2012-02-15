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
  FILE NAME: GLD_STANDBY.C

  DESCRIPTION: This file contains modules which process msgs recieved on standby 

  FUNCTIONS INCLUDED in this module:

******************************************************************************/
static uint32_t glsv_gld_standby_rsc_open(GLSV_GLD_A2S_CKPT_EVT *evt);
static uint32_t glsv_gld_standby_rsc_close(GLSV_GLD_A2S_CKPT_EVT *evt);
static uint32_t glsv_gld_standby_rsc_set_orphan(GLSV_GLD_A2S_CKPT_EVT *evt);
static uint32_t glsv_gld_standby_mds_glnd_down(GLSV_GLD_A2S_CKPT_EVT *evt);
static uint32_t glsv_gld_standby_glnd_operational(GLSV_GLD_A2S_CKPT_EVT *async_evt);
static void gld_a2s_evt_destroy(GLSV_GLD_A2S_CKPT_EVT *evt);
/* GLD dispatch table */
static const
GLSV_GLD_A2S_EVT_HANDLER gld_a2s_evt_dispatch_tbl[5] = {
	glsv_gld_standby_rsc_open,
	glsv_gld_standby_rsc_close,
	glsv_gld_standby_rsc_set_orphan,
	glsv_gld_standby_mds_glnd_down,
	glsv_gld_standby_glnd_operational
};

/****************************************************************************
 * Name          : gld_process_standby_evt
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
uint32_t gld_process_standby_evt(GLSV_GLD_CB *gld_cb, GLSV_GLD_A2S_CKPT_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	if (gld_a2s_evt_dispatch_tbl[evt->evt_type] (evt) != NCSCC_RC_SUCCESS) {
		LOG_ER("Active to standby event processing failed");
		gld_a2s_evt_destroy(evt);
		rc = NCSCC_RC_FAILURE;
		goto end;
	}
	gld_a2s_evt_destroy(evt);
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : gld_a2s_evt_destroy
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
static void gld_a2s_evt_destroy(GLSV_GLD_A2S_CKPT_EVT *evt)
{
	m_MMGR_FREE_GLSV_GLD_A2S_EVT(evt);
	return;
}

/****************************************************************************
 * Name          : glsv_gld_standby_rsc_open
 *
 * Description   : 
 *
 * Arguments     : async_evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_gld_standby_rsc_open(GLSV_GLD_A2S_CKPT_EVT *async_evt)
{
	GLSV_GLD_CB *gld_cb;
	GLSV_GLD_RSC_INFO *rsc_info;
	GLSV_GLD_GLND_DETAILS *node_details;
	GLSV_NODE_LIST *node_list, **tmp_node_list;
	SaAisErrorT error;
	uint32_t node_id;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (async_evt == NULL)
		goto end;

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(async_evt->info.rsc_open_info.mdest_id);

	gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);
	if (gld_cb == NULL) {
		LOG_ER("Handle take failed");
		goto end;
	}

	/* Find if the node details are available */
	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uint8_t *)&node_id)) == NULL) {
		if ((node_details = gld_add_glnd_node(gld_cb, async_evt->info.rsc_open_info.mdest_id)) == NULL) {
			LOG_ER("GLD a2s evt add node failed: rsc_id %u node_id %u ",
				      async_evt->info.rsc_open_info.rsc_id, node_id);
			goto error;
		}
	}

	rsc_info =
	    gld_find_add_rsc_name(gld_cb, &async_evt->info.rsc_open_info.rsc_name, async_evt->info.rsc_open_info.rsc_id,
				  0, &error);
	if (rsc_info == NULL) {
		LOG_ER("GLD a2s evt add rsc failed: rsc_id %u node_id %u",
			      async_evt->info.rsc_open_info.rsc_id, node_id);
		goto error;
	}
	rsc_info->saf_rsc_creation_time = async_evt->info.rsc_open_info.rsc_creation_time;
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
		node_list->node_id = node_id;
		*tmp_node_list = node_list;

	}
	TRACE_1("GLD A2S evt rsc open success: rsc_id %u node_id %u", rsc_info->rsc_id, node_id);
	ncshm_give_hdl(gld_cb->my_hdl);
	rc = NCSCC_RC_SUCCESS;
	goto end;
 error:
	LOG_ER("GLD a2s evt rsc open failed: rsc_id %u node_id %u",
		      async_evt->info.rsc_open_info.rsc_id, node_id);
	ncshm_give_hdl(gld_cb->my_hdl);
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : glsv_gld_standby_rsc_close
 *
 * Description   : This is the function is invoked when a rsc_close event is
 *                 is sent from avtive GLD to standby GLD. This function will
 *                  remove references to
 *                 to this resource from the mentioned node. If the resource
 *                 is not referred any longer then the data structures are freed
 *                 up.
 *
 * Arguments     : async_evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_gld_standby_rsc_close(GLSV_GLD_A2S_CKPT_EVT *async_evt)
{
	GLSV_GLD_CB *gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc;
	bool orphan_flag;
	uint32_t node_id;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (async_evt == NULL)
		goto end;
	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(async_evt->info.rsc_details.mdest_id);

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		LOG_ER("Handle take failed");
		goto end;
	}

	orphan_flag = async_evt->info.rsc_details.orphan;

	/* Find if the node details are available */
	if ((node_details =
	     (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details, (uint8_t *)&node_id)) == NULL) {
		LOG_ER("Patricia tree get failed: node_id %u", node_id);
		goto error;
	}

	glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_get(&node_details->rsc_info_tree,
								  (uint8_t *)&async_evt->info.rsc_details.rsc_id);
	if (glnd_rsc == NULL) {
		LOG_ER("Patricia tree get failed");
		goto error;

	}

	glnd_rsc->rsc_info->saf_rsc_no_of_users = glnd_rsc->rsc_info->saf_rsc_no_of_users - 1;

	if (async_evt->info.rsc_details.lcl_ref_cnt == 0)
		gld_rsc_rmv_node_ref(gld_cb, glnd_rsc->rsc_info, glnd_rsc, node_details, orphan_flag);

	TRACE_1("GLD a2s evt rsc close success: rsc_id %u node_id %u",
		      async_evt->info.rsc_details.rsc_id, node_id);
	ncshm_give_hdl(gld_cb->my_hdl);
	rc = NCSCC_RC_SUCCESS;
	goto end;
 error:
	LOG_ER("GLD a2s evt rsc close failed: rsc_id %u node_id %u",
		      async_evt->info.rsc_details.rsc_id, node_id);
	ncshm_give_hdl(gld_cb->my_hdl);
 end:	
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : glsv_gld_standby_rsc_set_orphan
 *
 * Description   : Instruction from the GLND to set the orphan flag
 *
 * Arguments     : async_evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_gld_standby_rsc_set_orphan(GLSV_GLD_A2S_CKPT_EVT *async_evt)
{
	GLSV_GLD_CB *gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details;
	uint32_t node_id;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (async_evt == NULL)
		goto end;

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(async_evt->info.rsc_details.mdest_id);

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		LOG_ER("Handle take failed");
		goto end;
	}

	/* Find if the node details are available */
	if ((node_details =
	     (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details, (uint8_t *)&node_id)) == NULL) {
		LOG_ER("Patricia tree get failed: node_id %u", node_id);
		goto error;
	}
	if (gld_rsc_ref_set_orphan(node_details, async_evt->info.rsc_details.rsc_id,
				   async_evt->info.rsc_details.orphan,
				   async_evt->info.rsc_details.lck_mode) == NCSCC_RC_SUCCESS) {
		TRACE_1("GLD a2s evt set orphan success: rsc_id %u node_id %u",
			      async_evt->info.rsc_details.rsc_id, node_id);
		ncshm_give_hdl(gld_cb->my_hdl);
		rc = NCSCC_RC_SUCCESS;
		goto end;
	} else
		goto error;

 error:
	LOG_ER("GLD a2s evt set orphan failed: rsc_id %u node_id %u",
		      async_evt->info.rsc_details.rsc_id, node_id);
	ncshm_give_hdl(gld_cb->my_hdl);
 end:
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
 * Name          : glsv_gld_mds_glnd_down
 *
 * Description   : MDS indicated that a glnd has gone down
 *
 * Arguments     : async_evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_gld_standby_mds_glnd_down(GLSV_GLD_A2S_CKPT_EVT *async_evt)
{
	GLSV_GLD_CB *gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc = NULL;
	bool orphan_flag;
	SaLckResourceIdT rsc_id;
	uint32_t node_id;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (async_evt == NULL) {
		rc = NCSCC_RC_FAILURE;
		goto end;
	}	
	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(async_evt->info.glnd_mds_info.mdest_id);

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		LOG_ER("Handle take failed");
		rc = NCSCC_RC_FAILURE;
		goto end;
	}

	orphan_flag = async_evt->info.rsc_details.orphan;

	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uint8_t *)&node_id)) == NULL) {
		LOG_ER("Patricia tree get failed: node_id %u", node_id);
		rc = NCSCC_RC_FAILURE;
		goto end;
	}

	/* Remove the reference to each of the resource referred by this node */
	glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uint8_t *)0);
	if (glnd_rsc) {
		rsc_id = glnd_rsc->rsc_id;
		while (glnd_rsc) {
			gld_rsc_rmv_node_ref(gld_cb, glnd_rsc->rsc_info, glnd_rsc, node_details, orphan_flag);
			glnd_rsc =
			    (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree,
									       (uint8_t *)&rsc_id);
			if (glnd_rsc)
				rsc_id = glnd_rsc->rsc_id;
		}
	}
	/* Cancel the restart timer if started */
	if (node_details->restart_timer.tmr_id != TMR_T_NULL)
		gld_stop_tmr(&node_details->restart_timer);

	/* Now delete this node details node */
	if (ncs_patricia_tree_del(&gld_cb->glnd_details, (NCS_PATRICIA_NODE *)node_details) != NCSCC_RC_SUCCESS) {
		LOG_ER("Patricia tree del failed: node_id %u",
				   node_details->node_id);
	}

	m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);

	ncshm_give_hdl(gld_cb->my_hdl);
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : glsv_gld_glnd_operational
 *
 * Description   : MDS indicated that a glnd has gone down
 *
 * Arguments     : async_evt  - Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_gld_standby_glnd_operational(GLSV_GLD_A2S_CKPT_EVT *async_evt)
{
	GLSV_GLD_CB *gld_cb = NULL;
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	GLSV_GLD_RSC_INFO *rsc_info = NULL;
	GLSV_NODE_LIST *node_list = NULL;
	uint32_t node_id;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (async_evt == NULL) {
		rc = NCSCC_RC_FAILURE;
		goto end;
	}

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(async_evt->info.glnd_mds_info.mdest_id);

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		LOG_ER("Handle take failed");
		rc = NCSCC_RC_FAILURE;
		goto end;	
	}

	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uint8_t *)&node_id)) != NULL) {
		memcpy(&node_details->dest_id, &async_evt->info.glnd_mds_info.mdest_id, sizeof(MDS_DEST));

		/* Cancel the restart timer if started */
		gld_stop_tmr(&node_details->restart_timer);

		node_details->status = GLND_OPERATIONAL_STATE;

		rsc_info = gld_cb->rsc_info;

		while (rsc_info != NULL) {
			node_list = rsc_info->node_list;
			while (node_list != NULL) {
				if (node_list->node_id == node_id) {
					memcpy(&node_list->dest_id, &async_evt->info.glnd_mds_info.mdest_id,
					       sizeof(MDS_DEST));
				}
				node_list = node_list->next;
			}
			rsc_info = rsc_info->next;
		}
	}

	ncshm_give_hdl(gld_cb->my_hdl);
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : gld_sb_proc_data_rsp(GLSV_GLD_CB *gld_cb,)
 *
 * Description   :
 *
 * Arguments     : 
 *
 * Return Values : NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t gld_sb_proc_data_rsp(GLSV_GLD_CB *gld_cb, GLSV_GLD_A2S_RSC_DETAILS *rsc_details)
{
	GLSV_A2S_NODE_LIST *node_list = NULL;
	GLSV_NODE_LIST *tmp1_node_list = NULL;
	GLSV_NODE_LIST **tmp2_node_list = NULL;
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	GLSV_GLD_RSC_INFO *rsc_info = NULL;
	SaAisErrorT ret_error;
	uint32_t node_id;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (rsc_details) {
		rsc_info =
		    (GLSV_GLD_RSC_INFO *)ncs_patricia_tree_get(&gld_cb->rsc_info_id, (uint8_t *)&rsc_details->rsc_id);
		if (rsc_info == NULL)
			rsc_info =
			    gld_add_rsc_info(gld_cb, &rsc_details->resource_name, rsc_details->rsc_id, &ret_error);
		if (rsc_info == NULL) {
			rc = NCSCC_RC_FAILURE;
			goto end;
		}	
		else
			rsc_info->can_orphan = rsc_details->can_orphan;
	} else {
		rc = NCSCC_RC_FAILURE;
		goto end;
	} 	
	if (rsc_details->node_list)
		node_list = rsc_details->node_list;
	else {
		rc = NCSCC_RC_FAILURE;
		goto end;
	}

	while (node_list != NULL) {
		node_id = m_NCS_NODE_ID_FROM_MDS_DEST(node_list->dest_id);
		/* Find if the node details are already available */
		if ((node_details =
		     (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details, (uint8_t *)&node_id)) == NULL) {
			if ((node_details = gld_add_glnd_node(gld_cb, node_list->dest_id)) == NULL) {
				rc = NCSCC_RC_FAILURE;
				goto end;
			}
			else
				node_details->status = node_list->status;
			if (node_details->status == GLND_RESTART_STATE) {
				memcpy(&node_details->restart_timer.mdest_id, &node_details->dest_id, sizeof(MDS_DEST));

				/* Start GLSV_GLD_GLND_RESTART_TIMEOUT timer */
				gld_start_tmr(gld_cb, &node_details->restart_timer,
					      GLD_TMR_NODE_RESTART_TIMEOUT, GLD_NODE_RESTART_TIMEOUT, 0);
			}
		}

		gld_rsc_add_node_ref(gld_cb, node_details, rsc_info);

		/* Now add this node to the list of nodes referring this resource */
		tmp1_node_list = rsc_info->node_list;
		tmp2_node_list = &rsc_info->node_list;
		while (tmp1_node_list != NULL) {
			if (tmp1_node_list->node_id == node_list->node_id)
				break;
			tmp2_node_list = &tmp1_node_list->next;
			tmp1_node_list = tmp1_node_list->next;
		}
		if (tmp1_node_list == NULL) {
			tmp1_node_list = m_MMGR_ALLOC_GLSV_NODE_LIST;
			memset(tmp1_node_list, 0, sizeof(GLSV_NODE_LIST));
			tmp1_node_list->dest_id = node_details->dest_id;
			/*In cold_sync,while decoding node_id value will be derived from dest_id,here updating the correct node_id in the node_list */
			tmp1_node_list->node_id = node_details->node_id;
			*tmp2_node_list = tmp1_node_list;

		}

		node_list = node_list->next;

	}
 end:
	TRACE_LEAVE();
	return rc;
}
