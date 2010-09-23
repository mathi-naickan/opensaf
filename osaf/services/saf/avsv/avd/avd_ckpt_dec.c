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

  DESCRIPTION: This module contain all the decode routines require for decoding
  AVD data structures during checkpointing. 

******************************************************************************/

#include <logtrace.h>
#include <avd.h>
#include <avd_cluster.h>

/* Declaration of async update functions */
static uns32 avsv_decode_ckpt_avd_cb_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_cluster_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_oper_su(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_node_up_info(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_node_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_node_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_node_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_node_rcv_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_node_snd_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_sg_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_sg_adjust_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_sg_su_assigned_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_sg_su_spare_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_sg_su_uninst_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_sg_fsm_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_su_si_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_su_si_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_su_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_su_term_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_su_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_su_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_su_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_su_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_su_act_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_su_preinstan(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_si_su_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_si_su_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_si_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_si_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_si_assignment_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_si_alarm_sent(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_comp_proxy_comp_name(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_comp_curr_num_csi_actv(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_comp_curr_num_csi_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_comp_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_comp_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_comp_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_comp_restart_count(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_oper_su(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uns32 avsv_decode_ckpt_avd_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);

/* Declaration of static cold sync decode functions */
static uns32 avsv_decode_cold_sync_rsp_avd_cb_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_cluster_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_sg_su_oper_list(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_async_updt_cnt(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);
static uns32 avsv_decode_cold_sync_rsp_avd_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj);

/*
 * Function list for decoding the async data.
 * We will jump into this function using the reo_type received 
 * in the decode argument.
 *
 * This array _must_ correspond to avsv_ckpt_msg_reo_type in avd_ckpt_msg.h
 */

const AVSV_DECODE_CKPT_DATA_FUNC_PTR avsv_dec_ckpt_data_func_list[] = {
	avsv_decode_ckpt_avd_cb_config,
	avsv_decode_ckpt_avd_cluster_config,
	avsv_decode_ckpt_avd_node_config,
	avsv_decode_ckpt_avd_app_config,
	avsv_decode_ckpt_avd_sg_config,
	avsv_decode_ckpt_avd_su_config,
	avsv_decode_ckpt_avd_si_config,
	avsv_decode_ckpt_avd_oper_su,
	avsv_decode_ckpt_avd_sg_admin_si,
	avsv_decode_ckpt_avd_comp_config,
	avsv_decode_ckpt_avd_comp_cs_type_config,
	avsv_decode_ckpt_avd_siass,

	/* 
	 * Messages to update independent fields.
	 */

	/* AVND Async Update messages */
	avsv_decode_ckpt_node_admin_state,
	avsv_decode_ckpt_node_oper_state,
	avsv_decode_ckpt_node_up_info,
	avsv_decode_ckpt_node_state,
	avsv_decode_ckpt_node_rcv_msg_id,
	avsv_decode_ckpt_node_snd_msg_id,

	/* SG Async Update messages */
	avsv_decode_ckpt_sg_admin_state,
	avsv_decode_ckpt_sg_su_assigned_num,
	avsv_decode_ckpt_sg_su_spare_num,
	avsv_decode_ckpt_sg_su_uninst_num,
	avsv_decode_ckpt_sg_adjust_state,
	avsv_decode_ckpt_sg_fsm_state,

	/* SU Async Update messages */
	avsv_decode_ckpt_su_preinstan,
	avsv_decode_ckpt_su_oper_state,
	avsv_decode_ckpt_su_admin_state,
	avsv_decode_ckpt_su_readiness_state,
	avsv_decode_ckpt_su_pres_state,
	avsv_decode_ckpt_su_si_curr_active,
	avsv_decode_ckpt_su_si_curr_stby,
	avsv_decode_ckpt_su_term_state,
	avsv_decode_ckpt_su_switch,
	avsv_decode_ckpt_su_act_state,

	/* SI Async Update messages */
	avsv_decode_ckpt_si_admin_state,
	avsv_decode_ckpt_si_assignment_state,
	avsv_decode_ckpt_si_su_curr_active,
	avsv_decode_ckpt_si_su_curr_stby,
	avsv_decode_ckpt_si_switch,
	avsv_decode_ckpt_si_alarm_sent,

	/* COMP Async Update messages */
	avsv_decode_ckpt_comp_proxy_comp_name,
	avsv_decode_ckpt_comp_curr_num_csi_actv,
	avsv_decode_ckpt_comp_curr_num_csi_stby,
	avsv_decode_ckpt_comp_oper_state,
	avsv_decode_ckpt_comp_readiness_state,
	avsv_decode_ckpt_comp_pres_state,
	avsv_decode_ckpt_comp_restart_count,
	NULL,			/* AVSV_SYNC_COMMIT */
};

/*
 * Function list for decoding the cold sync response data
 * We will jump into this function using the reo_type received 
 * in the cold sync response argument.
 */
const AVSV_DECODE_COLD_SYNC_RSP_DATA_FUNC_PTR avsv_dec_cold_sync_rsp_data_func_list[] = {
	avsv_decode_cold_sync_rsp_avd_cb_config,
	avsv_decode_cold_sync_rsp_avd_cluster_config,
	avsv_decode_cold_sync_rsp_avd_node_config,
	avsv_decode_cold_sync_rsp_avd_app_config,
	avsv_decode_cold_sync_rsp_avd_sg_config,
	avsv_decode_cold_sync_rsp_avd_su_config,
	avsv_decode_cold_sync_rsp_avd_si_config,
	avsv_decode_cold_sync_rsp_avd_sg_su_oper_list,
	avsv_decode_cold_sync_rsp_avd_sg_admin_si,
	avsv_decode_cold_sync_rsp_avd_comp_config,
	avsv_decode_cold_sync_rsp_avd_comp_cs_type_config,
	avsv_decode_cold_sync_rsp_avd_siass,
	avsv_decode_cold_sync_rsp_avd_async_updt_cnt
};

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_cb_config
 *
 * Purpose:  Decode entire CB data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_cb_config(AVD_CL_CB *cb_ptr, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	AVD_CL_CB *cb;

	cb = cb_ptr;

	TRACE_ENTER();
	/* 
	 * For updating CB, action is always to do update. We don't have add and remove
	 * action on CB. So call EDU to decode CB data.
	 */
	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cb, &dec->i_uba,
				    EDP_OP_TYPE_DEC, (AVD_CL_CB **)&cb, &ederror, dec->i_peer_version);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	/* Since update is successful, update async update count */
	cb->async_updt_cnt.cb_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_avnd_config
 *
 * Purpose:  Decode entire AVD_AVND data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_cluster_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status;
	EDU_ERR ederror = 0;
	AVD_CLUSTER dec_cluster;
	AVD_CLUSTER *cluster = &dec_cluster;

	TRACE_ENTER();

	assert(dec->i_action == NCS_MBCSV_ACT_UPDATE);

	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cluster,
		&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_CLUSTER **)&cluster, &ederror,
		dec->i_peer_version);
	assert(status == NCSCC_RC_SUCCESS);

	avd_cluster->saAmfClusterAdminState = cluster->saAmfClusterAdminState;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_node_config
 *
 * Purpose:  Decode entire AVD_AVND data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is add then create new element, if it is update
	 * request then just update data structure, and if it is remove then 
	 * remove entry from the list.
	 */
	switch (dec->i_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror,
			dec->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 1, 3);
		break;

	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	status = avd_ckpt_node(cb, avnd_ptr, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.node_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_app_config
 *
 * Purpose:  Decode entire AVD_APP data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_APP _app;
	AVD_APP *app = &_app;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is add then create new element, if it is update
	 * request then just update data structure, and if it is remove then 
	 * remove entry from the list.
	 */
	switch (dec->i_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_app,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_APP **)&app, &ederror,
			dec->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_app,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_APP **)&app, &ederror, 1, 1);
		break;
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	status = avd_ckpt_app(cb, app, dec->i_action);

	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.app_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_sg_config
 *
 * Purpose:  Decode entire AVD_SG data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SG *sg_ptr;
	AVD_SG dec_sg;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	sg_ptr = &dec_sg;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is add then create new element, if it is update
	 * request then just update data structure, and if it is remove then 
	 * remove entry from the list.
	 */
	switch (dec->i_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SG **)&sg_ptr, &ederror,
			dec->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_sg,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SG **)&sg_ptr, &ederror, 1, 1);
		break;
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	status = avd_ckpt_sg(cb, sg_ptr, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.sg_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_su_config
 *
 * Purpose:  Decode entire AVD_SU data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is add then create new element, if it is update
	 * request then just update data structure, and if it is remove then 
	 * remove entry from the list.
	 */
	switch (dec->i_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror,
			dec->i_peer_version);
		break;
	case NCS_MBCSV_ACT_RMV:
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 1, 1);
		break;
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	status = avd_ckpt_su(cb, su_ptr, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.su_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_si_config
 *
 * Purpose:  Decode entire AVD_SI data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SI *si_ptr_dec;
	AVD_SI dec_si;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	si_ptr_dec = &dec_si;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is add then create new element, if it is update
	 * request then just update data structure, and if it is remove then 
	 * remove entry from the list.
	 */
	switch (dec->i_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SI **)&si_ptr_dec, &ederror,
			dec->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_si,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SI **)&si_ptr_dec, &ederror, 1, 1);
		break;

	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	status = avd_ckpt_si(cb, si_ptr_dec, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.si_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_sg_admin_si
 *
 * Purpose:  Decode entire AVD_SG_ADMIN_SI data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	status = avd_ckpt_sg_admin_si(cb, &dec->i_uba, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.sg_admin_si_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_siass
 *
 * Purpose:  Decode entire AVD_SU_SI_REL data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVSV_SU_SI_REL_CKPT_MSG *su_si_ckpt;
	AVSV_SU_SI_REL_CKPT_MSG dec_su_si_ckpt;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	su_si_ckpt = &dec_su_si_ckpt;
	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is add then create new element, if it is update
	 * request then just update data structure, and if it is remove then 
	 * remove entry from the list.
	 */
	switch (dec->i_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_siass,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVSV_SU_SI_REL_CKPT_MSG **)&su_si_ckpt,
			&ederror, dec->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_siass, &dec->i_uba,
			EDP_OP_TYPE_DEC, (AVSV_SU_SI_REL_CKPT_MSG **)&su_si_ckpt, &ederror, 2, 1, 2);
		break;

	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	avd_ckpt_siass(cb, su_si_ckpt, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.siass_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_comp_config
 *
 * Purpose:  Decode entire AVD_COMP data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = 0;

	TRACE_ENTER();
	comp_ptr = &dec_comp;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is add then create new element, if it is update
	 * request then just update data structure, and if it is remove then 
	 * remove entry from the list.
	 */
	switch (dec->i_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror,
			dec->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror, 1, 1);
		break;

	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	status = avd_ckpt_comp(cb, comp_ptr, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.comp_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_oper_su
 *
 * Purpose:  Decode Operation SU name.
 *
 * Input: cb - CB pointer.
 *        dec - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_oper_su(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * In case of both Add and remove request send the operation SU name. 
	 * We don't have update for this reo_type.
	 */
	switch (dec->i_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_RMV:
		/* Send entire data */
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &dec->i_uba,
				      EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 1, 1);
		break;

	case NCS_MBCSV_ACT_UPDATE:
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	status = avd_ckpt_su_oper_list(cb, su_ptr, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.sg_su_oprlist_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_node_up_info
 *
 * Purpose:  Decode avnd node up info.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_node_up_info(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = 0;
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 6, 1, 2, 4, 5, 6, 7);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	if (NULL == (avnd_struct = avd_node_find_nodeid(avnd_ptr->node_info.nodeId))) {
		LOG_ER("%s: node not found, nodeid=%x", __FUNCTION__, avnd_ptr->node_info.nodeId);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->node_info.nodeAddress = avnd_ptr->node_info.nodeAddress;
	avnd_struct->node_info.member = avnd_ptr->node_info.member;
	avnd_struct->node_info.bootTimestamp = avnd_ptr->node_info.bootTimestamp;
	avnd_struct->node_info.initialViewNumber = avnd_ptr->node_info.initialViewNumber;
	avnd_struct->adest = avnd_ptr->adest;

	cb->async_updt_cnt.node_updt++;
	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_node_admin_state
 *
 * Purpose:  Decode avnd su admin state info.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_node_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = 0;
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 2, 3, 8);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (avnd_struct = avd_node_get(&avnd_ptr->name))) {
		LOG_ER("%s: node not found, nodeid=%s", __FUNCTION__, avnd_ptr->name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->saAmfNodeAdminState = avnd_ptr->saAmfNodeAdminState;

	cb->async_updt_cnt.node_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_node_oper_state
 *
 * Purpose:  Decode avnd oper state info.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_node_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = 0;
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 2, 3, 9);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (avnd_struct = avd_node_get(&avnd_ptr->name))) {
		LOG_ER("%s: node not found, nodeid=%s", __FUNCTION__, avnd_ptr->name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->saAmfNodeOperState = avnd_ptr->saAmfNodeOperState;

	cb->async_updt_cnt.node_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_node_state
 *
 * Purpose:  Decode node state info.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_node_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = 0;
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 2, 3, 10);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (avnd_struct = avd_node_get(&avnd_ptr->name))) {
		LOG_ER("%s: node not found, nodeid=%s", __FUNCTION__, avnd_ptr->name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->node_state = avnd_ptr->node_state;

	cb->async_updt_cnt.node_updt++;
	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_node_rcv_msg_id
 *
 * Purpose:  Decode avnd receive message ID.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_node_rcv_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = 0;
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 2, 1, 12);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (avnd_struct = avd_node_find_nodeid(avnd_ptr->node_info.nodeId))) {
		LOG_ER("%s: node not found, nodeid=%x", __FUNCTION__, avnd_ptr->node_info.nodeId);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->rcv_msg_id = avnd_ptr->rcv_msg_id;

	cb->async_updt_cnt.node_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avnd_snd_msg_id
 *
 * Purpose:  Decode avnd Send message ID.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_node_snd_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = 0;
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 2, 1, 13);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (avnd_struct = avd_node_find_nodeid(avnd_ptr->node_info.nodeId))) {
		LOG_ER("%s: node not found, nodeid=%x", __FUNCTION__, avnd_ptr->node_info.nodeId);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->snd_msg_id = avnd_ptr->snd_msg_id;

	cb->async_updt_cnt.node_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_sg_admin_state
 *
 * Purpose:  Decode SG admin state.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_sg_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SG *sg_ptr;
	AVD_SG dec_sg;
	EDU_ERR ederror = 0;
	AVD_SG *sg_struct;

	TRACE_ENTER();

	sg_ptr = &dec_sg;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_sg,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SG **)&sg_ptr, &ederror, 2, 1, 2);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (sg_struct = avd_sg_get(&sg_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	sg_struct->saAmfSGAdminState = sg_ptr->saAmfSGAdminState;

	cb->async_updt_cnt.sg_updt++;
	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_sg_su_assigned_num
 *
 * Purpose:  Decode SG su assign number.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_sg_su_assigned_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SG *sg_ptr;
	AVD_SG dec_sg;
	EDU_ERR ederror = 0;
	AVD_SG *sg_struct;

	TRACE_ENTER();

	sg_ptr = &dec_sg;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_sg,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SG **)&sg_ptr, &ederror, 2, 1, 3);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (sg_struct = avd_sg_get(&sg_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	sg_struct->saAmfSGNumCurrAssignedSUs = sg_ptr->saAmfSGNumCurrAssignedSUs;

	cb->async_updt_cnt.sg_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_sg_su_spare_num
 *
 * Purpose:  Decode SG su spare number.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_sg_su_spare_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SG *sg_ptr;
	AVD_SG dec_sg;
	EDU_ERR ederror = 0;
	AVD_SG *sg_struct;

	TRACE_ENTER();

	sg_ptr = &dec_sg;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_sg,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SG **)&sg_ptr, &ederror, 2, 1, 4);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (sg_struct = avd_sg_get(&sg_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	sg_struct->saAmfSGNumCurrInstantiatedSpareSUs = sg_ptr->saAmfSGNumCurrInstantiatedSpareSUs;

	cb->async_updt_cnt.sg_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_sg_su_uninst_num
 *
 * Purpose:  Decode SG su uninst number.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_sg_su_uninst_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SG *sg_ptr;
	AVD_SG dec_sg;
	EDU_ERR ederror = 0;
	AVD_SG *sg_struct;

	TRACE_ENTER();

	sg_ptr = &dec_sg;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_sg,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SG **)&sg_ptr, &ederror, 2, 1, 5);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (sg_struct = avd_sg_get(&sg_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	sg_struct->saAmfSGNumCurrNonInstantiatedSpareSUs = sg_ptr->saAmfSGNumCurrNonInstantiatedSpareSUs;

	cb->async_updt_cnt.sg_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_sg_adjust_state
 *
 * Purpose:  Decode SG adjust state.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_sg_adjust_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SG *sg_ptr;
	AVD_SG dec_sg;
	EDU_ERR ederror = 0;
	AVD_SG *sg_struct;

	TRACE_ENTER();

	sg_ptr = &dec_sg;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_sg,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SG **)&sg_ptr, &ederror, 2, 1, 6);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (sg_struct = avd_sg_get(&sg_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	sg_struct->adjust_state = sg_ptr->adjust_state;

	cb->async_updt_cnt.sg_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_sg_fsm_state
 *
 * Purpose:  Decode SG FSM state.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_sg_fsm_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SG *sg_ptr;
	AVD_SG dec_sg;
	EDU_ERR ederror = 0;
	AVD_SG *sg_struct;

	TRACE_ENTER();

	sg_ptr = &dec_sg;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_sg,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SG **)&sg_ptr, &ederror, 2, 1, 7);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (sg_struct = avd_sg_get(&sg_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	sg_struct->sg_fsm_state = sg_ptr->sg_fsm_state;

	cb->async_updt_cnt.sg_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_su_preinstan
 *
 * Purpose:  Decode SU preinstatible object.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_su_preinstan(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;
	AVD_SU *su_struct;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 2, 1, 2);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (su_struct = avd_su_get(&su_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	su_struct->saAmfSUPreInstantiable = su_ptr->saAmfSUPreInstantiable;

	cb->async_updt_cnt.su_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_su_oper_state
 *
 * Purpose:  Decode SU Operation state.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_su_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;
	AVD_SU *su_struct;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 2, 1, 3);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (su_struct = avd_su_get(&su_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	su_struct->saAmfSUOperState = su_ptr->saAmfSUOperState;

	cb->async_updt_cnt.su_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_su_admin_state
 *
 * Purpose:  Decode SU Admin state.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_su_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;
	AVD_SU *su_struct;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 2, 1, 4);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (su_struct = avd_su_get(&su_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	su_struct->saAmfSUAdminState = su_ptr->saAmfSUAdminState;

	cb->async_updt_cnt.su_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_su_rediness_state
 *
 * Purpose:  Decode SU Rediness state.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_su_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;
	AVD_SU *su_struct;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 2, 1, 5);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (su_struct = avd_su_get(&su_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	su_struct->saAmfSuReadinessState = su_ptr->saAmfSuReadinessState;

	cb->async_updt_cnt.su_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_su_pres_state
 *
 * Purpose:  Decode SU Presdece state.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_su_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;
	AVD_SU *su_struct;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 2, 1, 6);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (su_struct = avd_su_get(&su_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	su_struct->saAmfSUPresenceState = su_ptr->saAmfSUPresenceState;

	cb->async_updt_cnt.su_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_su_si_curr_active
 *
 * Purpose:  Decode SU Current number of Active SI.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_su_si_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;
	AVD_SU *su_struct;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 2, 1, 8);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (su_struct = avd_su_get(&su_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	su_struct->saAmfSUNumCurrActiveSIs = su_ptr->saAmfSUNumCurrActiveSIs;

	cb->async_updt_cnt.su_updt++;
	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_su_si_curr_stby
 *
 * Purpose:  Decode SU Current number of Standby SI.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_su_si_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;
	AVD_SU *su_struct;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 2, 1, 9);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (su_struct = avd_su_get(&su_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	su_struct->saAmfSUNumCurrStandbySIs = su_ptr->saAmfSUNumCurrStandbySIs;

	cb->async_updt_cnt.su_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_su_term_state
 *
 * Purpose:  Decode SU Admin state to terminate service.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_su_term_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;
	AVD_SU *su_struct;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
	   &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 2, 1, 11);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (su_struct = avd_su_get(&su_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	su_struct->term_state = su_ptr->term_state;

	cb->async_updt_cnt.su_updt++;
	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_su_switch
 *
 * Purpose:  Decode SU toggle SI.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_su_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;
	AVD_SU *su_struct;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 2, 1, 12);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (su_struct = avd_su_get(&su_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	su_struct->su_switch = su_ptr->su_switch;

	cb->async_updt_cnt.su_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_su_act_state
 *
 * Purpose:  Decode SU action state.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_su_act_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;
	AVD_SU *su_struct;

	TRACE_ENTER();

	su_ptr = &dec_su;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 2, 1, 13);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (su_struct = avd_su_get(&su_ptr->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	su_struct->su_act_state = su_ptr->su_act_state;

	cb->async_updt_cnt.su_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_si_admin_state
 *
 * Purpose:  Decode SI admin state.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_si_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SI *si_ptr_dec;
	AVD_SI dec_si;
	EDU_ERR ederror = 0;
	AVD_SI *si_struct;

	TRACE_ENTER();

	si_ptr_dec = &dec_si;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_si,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SI **)&si_ptr_dec, &ederror, 2, 1, 2);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (si_struct = avd_si_get(&si_ptr_dec->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	si_struct->saAmfSIAdminState = si_ptr_dec->saAmfSIAdminState;

	cb->async_updt_cnt.si_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_si_assignment_state
 *
 * Purpose:  Decode SI admin state.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_si_assignment_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SI *si_ptr_dec;
	AVD_SI dec_si;
	EDU_ERR ederror = 0;
	AVD_SI *si_struct;

	TRACE_ENTER();

	si_ptr_dec = &dec_si;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_si,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SI **)&si_ptr_dec, &ederror, 2, 1, 3);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (si_struct = avd_si_get(&si_ptr_dec->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	si_struct->saAmfSIAssignmentState = si_ptr_dec->saAmfSIAssignmentState;

	cb->async_updt_cnt.si_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_si_su_curr_active
 *
 * Purpose:  Decode number of active SU assignment for this SI.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_si_su_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SI *si_ptr_dec;
	AVD_SI dec_si;
	EDU_ERR ederror = 0;
	AVD_SI *si;

	si_ptr_dec = &dec_si;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_si,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SI **)&si_ptr_dec, &ederror, 2, 1, 4);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (si = avd_si_get(&si_ptr_dec->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	si->saAmfSINumCurrActiveAssignments = si_ptr_dec->saAmfSINumCurrActiveAssignments;
	TRACE("%s saAmfSINumCurrActiveAssignments=%u", si->name.value, si->saAmfSINumCurrActiveAssignments);

	cb->async_updt_cnt.si_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_si_su_curr_stby
 *
 * Purpose:  Decode number of standby SU assignment for this SI.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_si_su_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SI *si_ptr_dec;
	AVD_SI dec_si;
	EDU_ERR ederror = 0;
	AVD_SI *si;

	si_ptr_dec = &dec_si;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_si,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SI **)&si_ptr_dec, &ederror, 2, 1, 5);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (si = avd_si_get(&si_ptr_dec->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	si->saAmfSINumCurrStandbyAssignments = si_ptr_dec->saAmfSINumCurrStandbyAssignments;
	TRACE("%s saAmfSINumCurrStandbyAssignments=%u", si->name.value, si->saAmfSINumCurrStandbyAssignments);

	cb->async_updt_cnt.si_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_si_switch
 *
 * Purpose:  Decode SI switch.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_si_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SI *si_ptr_dec;
	AVD_SI dec_si;
	EDU_ERR ederror = 0;
	AVD_SI *si_struct;

	TRACE_ENTER();

	si_ptr_dec = &dec_si;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_si,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SI **)&si_ptr_dec, &ederror, 2, 1, 6);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (si_struct = avd_si_get(&si_ptr_dec->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	si_struct->si_switch = si_ptr_dec->si_switch;

	cb->async_updt_cnt.si_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_si_alarm_sent
 *
 * Purpose:  Decode SI alarm sent.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_si_alarm_sent(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SI *si_ptr_dec;
	AVD_SI dec_si;
	EDU_ERR ederror = 0;
	AVD_SI *si_struct;

	si_ptr_dec = &dec_si;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_si,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SI **)&si_ptr_dec, &ederror, 2, 1, 8);

	assert(status == NCSCC_RC_SUCCESS);

	if (NULL == (si_struct = avd_si_get(&si_ptr_dec->name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	si_struct->alarm_sent = si_ptr_dec->alarm_sent;
	TRACE("%s alarm_sent=%d", si_ptr_dec->name.value, si_struct->alarm_sent);

	cb->async_updt_cnt.si_updt++;

	return status;
}
/****************************************************************************\
 * Function: avsv_decode_ckpt_comp_proxy_comp_name
 *
 * Purpose:  Decode COMP proxy compnent name.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_comp_proxy_comp_name(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = 0;
	AVD_COMP *comp_struct;

	TRACE_ENTER();

	comp_ptr = &dec_comp;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror, 2, 1, 6);

	if (status != NCSCC_RC_SUCCESS)
		assert(0);

	if (NULL == (comp_struct = avd_comp_get(&comp_ptr->comp_info.name)))
		assert(0);

	/* Update the fields received in this checkpoint message */
	comp_struct->saAmfCompCurrProxyName = comp_ptr->saAmfCompCurrProxyName;

	cb->async_updt_cnt.comp_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_comp_curr_num_csi_actv
 *
 * Purpose:  Decode COMP Current number of CSI active.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_comp_curr_num_csi_actv(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = 0;
	AVD_COMP *comp_struct;

	TRACE_ENTER();

	comp_ptr = &dec_comp;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror, 2, 1, 32);

	if (status != NCSCC_RC_SUCCESS)
		assert(0);

	if (NULL == (comp_struct = avd_comp_get(&comp_ptr->comp_info.name))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->curr_num_csi_actv = comp_ptr->curr_num_csi_actv;

	cb->async_updt_cnt.comp_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_comp_curr_num_csi_stby
 *
 * Purpose:  Decode COMP Current number of CSI standby.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_comp_curr_num_csi_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = 0;
	AVD_COMP *comp_struct;

	TRACE_ENTER();

	comp_ptr = &dec_comp;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror, 2, 1, 33);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	if (NULL == (comp_struct = avd_comp_get(&comp_ptr->comp_info.name))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->curr_num_csi_stdby = comp_ptr->curr_num_csi_stdby;

	cb->async_updt_cnt.comp_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_comp_oper_state
 *
 * Purpose:  Decode COMP Operation State.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_comp_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = 0;
	AVD_COMP *comp_struct;

	TRACE_ENTER();

	comp_ptr = &dec_comp;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
		&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror, 2, 1, 2);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	if (NULL == (comp_struct = avd_comp_get(&comp_ptr->comp_info.name))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->saAmfCompOperState = comp_ptr->saAmfCompOperState;

	cb->async_updt_cnt.comp_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_comp_readiness_state
 *
 * Purpose:  Decode COMP Readiness State.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_comp_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = 0;
	AVD_COMP *comp_struct;

	TRACE_ENTER();

	comp_ptr = &dec_comp;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
		&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror, 2, 1, 3);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	if (NULL == (comp_struct = avd_comp_get(&comp_ptr->comp_info.name))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->saAmfCompReadinessState = comp_ptr->saAmfCompReadinessState;

	cb->async_updt_cnt.comp_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_comp_pres_state
 *
 * Purpose:  Decode COMP Presdece State.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_comp_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = 0;
	AVD_COMP *comp_struct;

	TRACE_ENTER();

	comp_ptr = &dec_comp;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
		&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror, 2, 1, 4);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	if (NULL == (comp_struct = avd_comp_get(&comp_ptr->comp_info.name))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->saAmfCompPresenceState = comp_ptr->saAmfCompPresenceState;

	cb->async_updt_cnt.comp_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_comp_restart_count
 *
 * Purpose:  Decode COMP Restart count.
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_ckpt_comp_restart_count(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = 0;
	AVD_COMP *comp_struct;

	TRACE_ENTER();

	comp_ptr = &dec_comp;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror, 2, 1, 5);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	if (NULL == (comp_struct = avd_comp_get(&comp_ptr->comp_info.name))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->saAmfCompRestartCount = comp_ptr->saAmfCompRestartCount;

	cb->async_updt_cnt.comp_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp
 *
 * Purpose:  Decode cold sync response message.
 *
 * Input: cb  - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avsv_decode_cold_sync_rsp(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 num_of_obj;
	uns8 *ptr;
	char logbuff[SA_MAX_NAME_LENGTH];

	TRACE_ENTER();

	/* 
	 * Since at decode we need to find out how many objects of particular data
	 * type are sent, decode that information at the begining of the message.
	 */
	ptr = ncs_dec_flatten_space(&dec->i_uba, (uns8 *)&num_of_obj, sizeof(uns32));
	num_of_obj = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&dec->i_uba, sizeof(uns32));

	/* 
	 * Decode data received in the message.
	 */
	dec->i_action = NCS_MBCSV_ACT_ADD;
	memset(logbuff, '\0', SA_MAX_NAME_LENGTH);
	snprintf(logbuff, SA_MAX_NAME_LENGTH - 1, "\n\nReceived reotype = %d num obj = %d --------------------\n",
		 dec->i_reo_type, num_of_obj);
	TRACE_ENTER();

	return avsv_dec_cold_sync_rsp_data_func_list[dec->i_reo_type] (cb, dec, num_of_obj);
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_cb_config
 *
 * Purpose:  Decode entire CB data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_cb_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	AVD_CL_CB *cb_ptr;

	TRACE_ENTER();

	cb_ptr = cb;
	/* 
	 * Send the CB data.
	 */
	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cb, &dec->i_uba,
				    EDP_OP_TYPE_DEC, (AVD_CL_CB **)&cb_ptr, &ederror, dec->i_peer_version);
	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_avnd_config
 *
 * Purpose:  Decode entire AVD_AVND data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_cluster_config(AVD_CL_CB *cb,
	NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status;
	EDU_ERR ederror = 0;
	AVD_CLUSTER dec_cluster;
	AVD_CLUSTER *cluster = &dec_cluster;

	TRACE_ENTER();

	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cluster,
		&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_CLUSTER **)&cluster, &ederror,
		dec->i_peer_version);
	assert(status == NCSCC_RC_SUCCESS);
	avd_cluster->saAmfClusterAdminState = cluster->saAmfClusterAdminState;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_avnd_config
 *
 * Purpose:  Decode entire AVD_AVND data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 count = 0;
	EDU_ERR ederror = 0;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Walk through the entire list and decode the entire AVND data received.
	 */
	for (count = 0; count < num_of_obj; count++) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
					    &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror,
					    dec->i_peer_version);
		assert(status == NCSCC_RC_SUCCESS);
		status = avd_ckpt_node(cb, avnd_ptr, dec->i_action);
		assert(status == NCSCC_RC_SUCCESS);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_app_config
 *
 * Purpose:  Decode entire AVD_SG data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 count = 0;
	EDU_ERR ederror = 0;
	AVD_APP _app;
	AVD_APP *app = &_app;

	TRACE_ENTER();

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (count = 0; count < num_of_obj; count++) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_app,
					    &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_APP **)&app, &ederror,
					    dec->i_peer_version);
		assert(status == NCSCC_RC_SUCCESS);
		status = avd_ckpt_app(cb, app, dec->i_action);
		assert(status == NCSCC_RC_SUCCESS);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_sg_config
 *
 * Purpose:  Decode entire AVD_SG data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 count = 0;
	EDU_ERR ederror = 0;
	AVD_SG dec_sg;
	AVD_SG *sg_ptr;

	sg_ptr = &dec_sg;

	TRACE_ENTER();

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (count = 0; count < num_of_obj; count++) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg,
					    &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SG **)&sg_ptr, &ederror,
					    dec->i_peer_version);
		assert(status == NCSCC_RC_SUCCESS);
		status = avd_ckpt_sg(cb, sg_ptr, dec->i_action);
		assert(status == NCSCC_RC_SUCCESS);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_su_config
 *
 * Purpose:  Decode entire AVD_SU data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 count = 0;
	EDU_ERR ederror = 0;
	AVD_SU dec_su;
	AVD_SU *su_ptr;

	su_ptr = &dec_su;

	TRACE_ENTER();

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (count = 0; count < num_of_obj; count++) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su,
					    &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror,
					    dec->i_peer_version);
		assert(status == NCSCC_RC_SUCCESS);
		status = avd_ckpt_su(cb, su_ptr, dec->i_action);
		assert(status == NCSCC_RC_SUCCESS);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_si_config
 *
 * Purpose:  Decode entire AVD_SI data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 count = 0;
	AVD_SI *si_ptr_dec;
	AVD_SI dec_si;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	si_ptr_dec = &dec_si;

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (count = 0; count < num_of_obj; count++) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si,
					    &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_SI **)&si_ptr_dec, &ederror,
					    dec->i_peer_version);

		assert(status == NCSCC_RC_SUCCESS);
		status = avd_ckpt_si(cb, si_ptr_dec, dec->i_action);
		assert(status == NCSCC_RC_SUCCESS);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_sg_su_oper_list
 *
 * Purpose:  Decode entire AVD_SG_SU_OPER_LIST data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_sg_su_oper_list(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 cs_count = 0;

	TRACE_ENTER();

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (cs_count = 0; cs_count < num_of_obj; cs_count++) {
		if (NCSCC_RC_SUCCESS != avsv_decode_oper_su(cb, dec))
			break;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_sg_admin_si
 *
 * Purpose:  Decode entire AVD_SG_ADMIN_SI data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 count = 0;

	TRACE_ENTER();

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (count = 0; count < num_of_obj; count++) {
		status = avd_ckpt_sg_admin_si(cb, &dec->i_uba, dec->i_action);

		if (status != NCSCC_RC_SUCCESS) {
			return NCSCC_RC_FAILURE;
		}

	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_su_si_rel
 *
 * Purpose:  Decode entire AVD_SU_SI_REL data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVSV_SU_SI_REL_CKPT_MSG *su_si_ckpt;
	AVSV_SU_SI_REL_CKPT_MSG dec_su_si_ckpt;
	EDU_ERR ederror = 0;
	uns32 count = 0;

	TRACE_ENTER();

	su_si_ckpt = &dec_su_si_ckpt;

	/* 
	 * Walk through the entire list and send the entire list data.
	 * We will walk the SU tree and all the SU_SI relationship for that SU
	 * are sent.We will send the corresponding COMP_CSI relationship for that SU_SI
	 * in the same update.
	 */
	for (count = 0; count < num_of_obj; count++) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_siass,
					    &dec->i_uba, EDP_OP_TYPE_DEC, (AVSV_SU_SI_REL_CKPT_MSG **)&su_si_ckpt,
					    &ederror, dec->i_peer_version);
		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		}

		status = avd_ckpt_siass(cb, su_si_ckpt, dec->i_action);

		if (status != NCSCC_RC_SUCCESS) {
			return NCSCC_RC_FAILURE;
		}
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_comp_config
 *
 * Purpose:  Decode entire AVD_COMP data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = 0;
	uns32 count = 0;

	TRACE_ENTER();

	comp_ptr = &dec_comp;
	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (count = 0; count < num_of_obj; count++) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
					    &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror,
					    dec->i_peer_version);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
			return status;
		}

		status = avd_ckpt_comp(cb, comp_ptr, dec->i_action);

		if (status != NCSCC_RC_SUCCESS) {
			return NCSCC_RC_FAILURE;
		}

	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_async_updt_cnt
 *
 * Purpose:  Send the latest async update count. This count will be used 
 *           during warm sync for verifying the data at stnadby. 
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_async_updt_cnt(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVSV_ASYNC_UPDT_CNT *updt_cnt;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	updt_cnt = &cb->async_updt_cnt;
	/* 
	 * Decode and send async update counts for all the data structures.
	 */
	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_async_updt_cnt,
				    &dec->i_uba, EDP_OP_TYPE_DEC, &updt_cnt, &ederror, dec->i_peer_version);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_warm_sync_rsp
 *
 * Purpose:  Decode Warm sync response message.
 *
 * Input: cb  - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avsv_decode_warm_sync_rsp(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVSV_ASYNC_UPDT_CNT *updt_cnt;
	AVSV_ASYNC_UPDT_CNT dec_updt_cnt;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	updt_cnt = &dec_updt_cnt;

	/* 
	 * Decode latest async update counts. (In the same manner we received
	 * in the last message of the cold sync response.
	 */
	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_async_updt_cnt,
				    &dec->i_uba, EDP_OP_TYPE_DEC, &updt_cnt, &ederror, dec->i_peer_version);

	if (status != NCSCC_RC_SUCCESS)
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);

	/*
	 * Compare the update counts of the Standby with Active.
	 * if they matches return success. If it fails then 
	 * send a data request.
	 */
	if (0 != memcmp(updt_cnt, &cb->async_updt_cnt, sizeof(AVSV_ASYNC_UPDT_CNT))) {
		/* Log this as a Notify kind of message....this means we 
		 * have mismatch in warm sync and we need to resync. 
		 * Stanby AVD is unavailable for failure */

		cb->stby_sync_state = AVD_STBY_OUT_OF_SYNC;
                /* We need to figure out when there is out of sync, later on we have to remove it. */
		assert(0);

		/*
		 * Remove All data structures from the standby. We will get them again
		 * during our data responce sync-up.
		 */
		avd_data_clean_up(cb);

		/*
		 * Now send data request, which will sync Standby with Active.
		 */
		(void) avsv_send_data_req(cb);
		status = NCSCC_RC_FAILURE;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_data_sync_rsp
 *
 * Purpose:  Decode Data sync response message.
 *
 * Input: cb  - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avsv_decode_data_sync_rsp(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 num_of_obj;
	uns8 *ptr;

	TRACE_ENTER2("%u", dec->i_reo_type);

	/* 
	 * Since at decode we need to find out how many objects of particular data
	 * type are sent, decode that information at the begining of the message.
	 */
	ptr = ncs_dec_flatten_space(&dec->i_uba, (uns8 *)&num_of_obj, sizeof(uns32));
	num_of_obj = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&dec->i_uba, sizeof(uns32));

	/* 
	 * Decode data received in the message.
	 */
	dec->i_action = NCS_MBCSV_ACT_ADD;

	return avsv_dec_cold_sync_rsp_data_func_list[dec->i_reo_type] (cb, dec, num_of_obj);
}

/****************************************************************************\
 * Function: avsv_decode_data_req
 *
 * Purpose:  Decode Data request message.
 *
 * Input: cb  - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avsv_decode_data_req(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	TRACE_ENTER();

	/*
	 * Don't decode anything...just return success.
	 */
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_decode_oper_su
 *
 * Purpose:  Decode Operation SU's received.
 *
 * Input: cb  - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_decode_oper_su(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 num_of_oper_su, count;
	uns8 *ptr;
	AVD_SU *su_ptr;
	AVD_SU dec_su;
	EDU_ERR ederror = 0;

	TRACE_ENTER();

	ptr = ncs_dec_flatten_space(&dec->i_uba, (uns8 *)&num_of_oper_su, sizeof(uns32));
	num_of_oper_su = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&dec->i_uba, sizeof(uns32));

	su_ptr = &dec_su;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is add then create new element, if it is update
	 * request then just update data structure, and if it is remove then 
	 * remove entry from the list.
	 */
	for (count = 0; count < num_of_oper_su; count++) {
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &dec->i_uba,
				      EDP_OP_TYPE_DEC, (AVD_SU **)&su_ptr, &ederror, 1, 1);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
			return status;
		}

		status = avd_ckpt_su_oper_list(cb, su_ptr, dec->i_action);
		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: avd_ckpt_su_oper_list failed", __FUNCTION__);
			return status;
		}
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_ckpt_avd_comp_cs_type_config
 *
 * Purpose:  Decode entire AVD_COMP_CS_TYPE data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 *
\**************************************************************************/
static uns32 avsv_decode_ckpt_avd_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	AVD_COMPCS_TYPE *comp_cs;
	AVD_COMPCS_TYPE comp_cs_type;

	TRACE_ENTER();

	comp_cs = &comp_cs_type;

	/*
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is add then create new element, if it is update
	 * request then just update data structure, and if it is remove then
	 * remove entry from the list.
	 */
	switch (dec->i_action) {
	case NCS_MBCSV_ACT_UPDATE:
	case NCS_MBCSV_ACT_ADD:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp_cs_type,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMPCS_TYPE **)&comp_cs, &ederror,
			dec->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp_cs_type,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMPCS_TYPE **)&comp_cs, &ederror, 1, 1);
		break;
	default:
		assert(0);
	}

	assert(status == NCSCC_RC_SUCCESS);
	status = avd_ckpt_compcstype(cb, comp_cs, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.compcstype_updt++;

	return status;
}

/****************************************************************************\
 * Function: avsv_decode_cold_sync_rsp_avd_comp_cs_type_config
 *
 * Purpose:  Decode entire AVD_COMP_CS_TYPE data..
 *
 * Input: cb - CB pointer.
 *        dec - Decode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 *
\**************************************************************************/
static uns32 avsv_decode_cold_sync_rsp_avd_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uns32 num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	AVD_COMPCS_TYPE dec_comp_cs;
	AVD_COMPCS_TYPE *comp_cs_ptr = &dec_comp_cs;
	uns32 count;

	TRACE_ENTER();

	for (count = 0; count < num_of_obj; count++) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp_cs_type,
					    &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMPCS_TYPE **)&comp_cs_ptr, &ederror,
					    dec->i_peer_version);

		assert(status == NCSCC_RC_SUCCESS);
		status = avd_ckpt_compcstype(cb, comp_cs_ptr, dec->i_action);
		assert(status == NCSCC_RC_SUCCESS);
	}

	return status;
}

