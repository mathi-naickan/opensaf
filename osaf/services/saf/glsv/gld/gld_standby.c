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
static uns32 glsv_gld_standby_rsc_open(GLSV_GLD_A2S_CKPT_EVT *evt);
static uns32 glsv_gld_standby_rsc_close(GLSV_GLD_A2S_CKPT_EVT *evt);
static uns32 glsv_gld_standby_rsc_set_orphan(GLSV_GLD_A2S_CKPT_EVT *evt);
static uns32 glsv_gld_standby_mds_glnd_down(GLSV_GLD_A2S_CKPT_EVT *evt);
static uns32 glsv_gld_standby_glnd_operational(GLSV_GLD_A2S_CKPT_EVT *async_evt);
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
uns32 gld_process_standby_evt(GLSV_GLD_CB *gld_cb, GLSV_GLD_A2S_CKPT_EVT *evt)
{
	if (gld_a2s_evt_dispatch_tbl[evt->evt_type] (evt) != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_HEADLINE(GLD_A2S_EVT_PROC_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		gld_a2s_evt_destroy(evt);
		return NCSCC_RC_FAILURE;
	}
	gld_a2s_evt_destroy(evt);
	return NCSCC_RC_SUCCESS;
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
static uns32 glsv_gld_standby_rsc_open(GLSV_GLD_A2S_CKPT_EVT *async_evt)
{
	GLSV_GLD_CB *gld_cb;
	GLSV_GLD_RSC_INFO *rsc_info;
	GLSV_GLD_GLND_DETAILS *node_details;
	GLSV_NODE_LIST *node_list, **tmp_node_list;
	SaAisErrorT error;
	uns32 node_id;

	if (async_evt == NULL)
		return NCSCC_RC_FAILURE;

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(async_evt->info.rsc_open_info.mdest_id);

	gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);
	if (gld_cb == NULL) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	/* Find if the node details are available */
	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uns8 *)&node_id)) == NULL) {
		if ((node_details = gld_add_glnd_node(gld_cb, async_evt->info.rsc_open_info.mdest_id)) == NULL) {
			m_LOG_GLD_EVT(GLD_A2S_EVT_ADD_NODE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__,
				      async_evt->info.rsc_open_info.rsc_id, node_id);
			goto error;
		}
	}

	rsc_info =
	    gld_find_add_rsc_name(gld_cb, &async_evt->info.rsc_open_info.rsc_name, async_evt->info.rsc_open_info.rsc_id,
				  0, &error);
	if (rsc_info == NULL) {
		m_LOG_GLD_EVT(GLD_A2S_EVT_ADD_RSC_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__,
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
	m_LOG_GLD_EVT(GLD_A2S_EVT_RSC_OPEN_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__, rsc_info->rsc_id, node_id);
	ncshm_give_hdl(gld_cb->my_hdl);
	return NCSCC_RC_SUCCESS;
 error:
	m_LOG_GLD_EVT(GLD_A2S_EVT_RSC_OPEN_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__,
		      async_evt->info.rsc_open_info.rsc_id, node_id);
	ncshm_give_hdl(gld_cb->my_hdl);
	return NCSCC_RC_FAILURE;
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
static uns32 glsv_gld_standby_rsc_close(GLSV_GLD_A2S_CKPT_EVT *async_evt)
{
	GLSV_GLD_CB *gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc;
	NCS_BOOL orphan_flag;
	uns32 node_id;

	if (async_evt == NULL)
		return NCSCC_RC_FAILURE;
	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(async_evt->info.rsc_details.mdest_id);

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	orphan_flag = async_evt->info.rsc_details.orphan;

	/* Find if the node details are available */
	if ((node_details =
	     (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details, (uns8 *)&node_id)) == NULL) {
		m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_GET_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, node_id);
		goto error;
	}

	glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_get(&node_details->rsc_info_tree,
								  (uns8 *)&async_evt->info.rsc_details.rsc_id);
	if (glnd_rsc == NULL) {
		m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_GET_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		goto error;

	}

	glnd_rsc->rsc_info->saf_rsc_no_of_users = glnd_rsc->rsc_info->saf_rsc_no_of_users - 1;

	if (async_evt->info.rsc_details.lcl_ref_cnt == 0)
		gld_rsc_rmv_node_ref(gld_cb, glnd_rsc->rsc_info, glnd_rsc, node_details, orphan_flag);

	m_LOG_GLD_EVT(GLD_A2S_EVT_RSC_CLOSE_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__,
		      async_evt->info.rsc_details.rsc_id, node_id);
	ncshm_give_hdl(gld_cb->my_hdl);
	return NCSCC_RC_SUCCESS;
 error:
	m_LOG_GLD_EVT(GLD_A2S_EVT_RSC_CLOSE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__,
		      async_evt->info.rsc_details.rsc_id, node_id);
	ncshm_give_hdl(gld_cb->my_hdl);
	return NCSCC_RC_FAILURE;
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
static uns32 glsv_gld_standby_rsc_set_orphan(GLSV_GLD_A2S_CKPT_EVT *async_evt)
{
	GLSV_GLD_CB *gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details;
	uns32 node_id;

	if (async_evt == NULL)
		return NCSCC_RC_FAILURE;

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(async_evt->info.rsc_details.mdest_id);

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	/* Find if the node details are available */
	if ((node_details =
	     (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details, (uns8 *)&node_id)) == NULL) {
		m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_GET_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, node_id);
		goto error;
	}
	if (gld_rsc_ref_set_orphan(node_details, async_evt->info.rsc_details.rsc_id,
				   async_evt->info.rsc_details.orphan,
				   async_evt->info.rsc_details.lck_mode) == NCSCC_RC_SUCCESS) {
		m_LOG_GLD_EVT(GLD_A2S_EVT_SET_ORPHAN_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__,
			      async_evt->info.rsc_details.rsc_id, node_id);
		ncshm_give_hdl(gld_cb->my_hdl);
		return NCSCC_RC_SUCCESS;
	} else
		goto error;

 error:
	m_LOG_GLD_EVT(GLD_A2S_EVT_SET_ORPHAN_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__,
		      async_evt->info.rsc_details.rsc_id, node_id);
	ncshm_give_hdl(gld_cb->my_hdl);
	return NCSCC_RC_FAILURE;

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
static uns32 glsv_gld_standby_mds_glnd_down(GLSV_GLD_A2S_CKPT_EVT *async_evt)
{
	GLSV_GLD_CB *gld_cb;
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc = NULL;
	NCS_BOOL orphan_flag;
	SaLckResourceIdT rsc_id;
	uns32 node_id;

	if (async_evt == NULL)
		return NCSCC_RC_FAILURE;

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(async_evt->info.glnd_mds_info.mdest_id);

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return (NCSCC_RC_FAILURE);
	}

	orphan_flag = async_evt->info.rsc_details.orphan;

	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uns8 *)&node_id)) == NULL) {
		m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_GET_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, node_id);
		return NCSCC_RC_FAILURE;
	}

	/* Remove the reference to each of the resource referred by this node */
	glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uns8 *)0);
	if (glnd_rsc) {
		rsc_id = glnd_rsc->rsc_id;
		while (glnd_rsc) {
			gld_rsc_rmv_node_ref(gld_cb, glnd_rsc->rsc_info, glnd_rsc, node_details, orphan_flag);
			glnd_rsc =
			    (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree,
									       (uns8 *)&rsc_id);
			if (glnd_rsc)
				rsc_id = glnd_rsc->rsc_id;
		}
	}
	/* Cancel the restart timer if started */
	if (node_details->restart_timer.tmr_id != TMR_T_NULL)
		gld_stop_tmr(&node_details->restart_timer);

	/* Now delete this node details node */
	if (ncs_patricia_tree_del(&gld_cb->glnd_details, (NCS_PATRICIA_NODE *)node_details) != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_DEL_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__,
				   node_details->node_id);
	}

	m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);

	ncshm_give_hdl(gld_cb->my_hdl);
	return NCSCC_RC_SUCCESS;
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
static uns32 glsv_gld_standby_glnd_operational(GLSV_GLD_A2S_CKPT_EVT *async_evt)
{
	GLSV_GLD_CB *gld_cb = NULL;
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	GLSV_GLD_RSC_INFO *rsc_info = NULL;
	GLSV_NODE_LIST *node_list = NULL;
	NCS_BOOL orphan_flag;
	uns32 node_id;

	if (async_evt == NULL)
		return NCSCC_RC_FAILURE;

	node_id = m_NCS_NODE_ID_FROM_MDS_DEST(async_evt->info.glnd_mds_info.mdest_id);

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return (NCSCC_RC_FAILURE);
	}

	orphan_flag = async_evt->info.rsc_details.orphan;

	if ((node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
									   (uns8 *)&node_id)) != NULL) {
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
	return NCSCC_RC_SUCCESS;
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
uns32 gld_sb_proc_data_rsp(GLSV_GLD_CB *gld_cb, GLSV_GLD_A2S_RSC_DETAILS *rsc_details)
{
	GLSV_A2S_NODE_LIST *node_list = NULL;
	GLSV_NODE_LIST *tmp1_node_list = NULL;
	GLSV_NODE_LIST **tmp2_node_list = NULL;
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	GLSV_GLD_RSC_INFO *rsc_info = NULL;
	SaAisErrorT ret_error;
	uns32 node_id;

	if (rsc_details) {
		rsc_info =
		    (GLSV_GLD_RSC_INFO *)ncs_patricia_tree_get(&gld_cb->rsc_info_id, (uns8 *)&rsc_details->rsc_id);
		if (rsc_info == NULL)
			rsc_info =
			    gld_add_rsc_info(gld_cb, &rsc_details->resource_name, rsc_details->rsc_id, &ret_error);
		if (rsc_info == NULL)
			return NCSCC_RC_FAILURE;
		else
			rsc_info->can_orphan = rsc_details->can_orphan;
	} else
		return NCSCC_RC_FAILURE;

	if (rsc_details->node_list)
		node_list = rsc_details->node_list;
	else
		return NCSCC_RC_FAILURE;

	while (node_list != NULL) {
		node_id = m_NCS_NODE_ID_FROM_MDS_DEST(node_list->dest_id);
		/* Find if the node details are already available */
		if ((node_details =
		     (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details, (uns8 *)&node_id)) == NULL) {
			if ((node_details = gld_add_glnd_node(gld_cb, node_list->dest_id)) == NULL)
				return NCSCC_RC_FAILURE;
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
	return NCSCC_RC_SUCCESS;

}
