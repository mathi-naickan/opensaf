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

  DESCRIPTION: This module contain all the encode routines require for encoding
  AVD data structures during checkpointing. 

******************************************************************************/

#include <logtrace.h>
#include <saflog.h>
#include <avd.h>
#include <avd_cluster.h>

/* Declaration of async update functions */
static uns32 avsv_encode_ckpt_avd_cb_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_cluster_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_oper_su(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_node_up_info(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_node_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_node_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_node_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_node_rcv_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_node_snd_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_sg_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_sg_adjust_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_sg_su_assigned_num(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_sg_su_spare_num(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_sg_su_uninst_num(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_sg_fsm_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_su_si_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_su_si_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_su_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_su_term_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_su_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_su_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_su_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_su_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_su_act_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_su_preinstan(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_si_su_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_si_su_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_si_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_si_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_si_assignment_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_si_alarm_sent(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_comp_proxy_comp_name(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_comp_curr_num_csi_actv(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_comp_curr_num_csi_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_comp_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_comp_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_comp_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_comp_restart_count(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avsv_encode_ckpt_avd_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc);
static uns32 avd_entire_data_update(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, NCS_BOOL c_sync);

/* Declaration of static cold sync encode functions */
static uns32 avsv_encode_cold_sync_rsp_avd_cb_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_cluster_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_sg_su_oper_list(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_async_updt_cnt(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);
static uns32 avsv_encode_cold_sync_rsp_avd_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj);

static uns32 avsv_encode_su_oper_list(AVD_CL_CB *cb, AVD_SG *sg, NCS_MBCSV_CB_ENC *enc);
/*
 * Function list for encoding the async data.
 * We will jump into this function using the reo_type received 
 * in the encode argument.
 *
 * This array _must_ correspond to avsv_ckpt_msg_reo_type in avd_ckpt_msg.h
 */
const AVSV_ENCODE_CKPT_DATA_FUNC_PTR avsv_enc_ckpt_data_func_list[] = {
	avsv_encode_ckpt_avd_cb_config,
	avsv_encode_ckpt_avd_cluster_config,
	avsv_encode_ckpt_avd_node_config,
	avsv_encode_ckpt_avd_app_config,
	avsv_encode_ckpt_avd_sg_config,
	avsv_encode_ckpt_avd_su_config,
	avsv_encode_ckpt_avd_si_config,
	avsv_encode_ckpt_avd_oper_su,
	avsv_encode_ckpt_avd_sg_admin_si,
	avsv_encode_ckpt_avd_comp_config,
	avsv_encode_ckpt_avd_comp_cs_type_config,
	avsv_encode_ckpt_avd_siass,

	/* 
	 * Messages to update independent fields.
	 */

	/* Node Async Update messages */
	avsv_encode_ckpt_node_admin_state,
	avsv_encode_ckpt_node_oper_state,
	avsv_encode_ckpt_node_up_info,
	avsv_encode_ckpt_node_state,
	avsv_encode_ckpt_node_rcv_msg_id,
	avsv_encode_ckpt_node_snd_msg_id,

	/* SG Async Update messages */
	avsv_encode_ckpt_sg_admin_state,
	avsv_encode_ckpt_sg_su_assigned_num,
	avsv_encode_ckpt_sg_su_spare_num,
	avsv_encode_ckpt_sg_su_uninst_num,
	avsv_encode_ckpt_sg_adjust_state,
	avsv_encode_ckpt_sg_fsm_state,

	/* SU Async Update messages */
	avsv_encode_ckpt_su_preinstan,
	avsv_encode_ckpt_su_oper_state,
	avsv_encode_ckpt_su_admin_state,
	avsv_encode_ckpt_su_readiness_state,
	avsv_encode_ckpt_su_pres_state,
	avsv_encode_ckpt_su_si_curr_active,
	avsv_encode_ckpt_su_si_curr_stby,
	avsv_encode_ckpt_su_term_state,
	avsv_encode_ckpt_su_switch,
	avsv_encode_ckpt_su_act_state,

	/* SI Async Update messages */
	avsv_encode_ckpt_si_admin_state,
	avsv_encode_ckpt_si_assignment_state,
	avsv_encode_ckpt_si_su_curr_active,
	avsv_encode_ckpt_si_su_curr_stby,
	avsv_encode_ckpt_si_switch,
	avsv_encode_ckpt_si_alarm_sent,

	/* COMP Async Update messages */
	avsv_encode_ckpt_comp_proxy_comp_name,
	avsv_encode_ckpt_comp_curr_num_csi_actv,
	avsv_encode_ckpt_comp_curr_num_csi_stby,
	avsv_encode_ckpt_comp_oper_state,
	avsv_encode_ckpt_comp_readiness_state,
	avsv_encode_ckpt_comp_pres_state,
	avsv_encode_ckpt_comp_restart_count,
	NULL			/* AVSV_SYNC_COMMIT */
};

/*
 * Function list for encoding the cold sync response data
 * We will jump into this function using the reo_type received 
 * in the cold sync response argument.
 */
const AVSV_ENCODE_COLD_SYNC_RSP_DATA_FUNC_PTR avsv_enc_cold_sync_rsp_data_func_list[] = {
	avsv_encode_cold_sync_rsp_avd_cb_config,
	avsv_encode_cold_sync_rsp_avd_cluster_config,
	avsv_encode_cold_sync_rsp_avd_node_config,
	avsv_encode_cold_sync_rsp_avd_app_config,
	avsv_encode_cold_sync_rsp_avd_sg_config,
	avsv_encode_cold_sync_rsp_avd_su_config,
	avsv_encode_cold_sync_rsp_avd_si_config,
	avsv_encode_cold_sync_rsp_avd_sg_su_oper_list,
	avsv_encode_cold_sync_rsp_avd_sg_admin_si,
	avsv_encode_cold_sync_rsp_avd_comp_config,
	avsv_encode_cold_sync_rsp_avd_comp_cs_type_config,
	avsv_encode_cold_sync_rsp_avd_siass,
	avsv_encode_cold_sync_rsp_avd_async_updt_cnt
};

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_cb_config
 *
 * Purpose:  Encode entire CB data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_cb_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * For updating CB, action is always to do update. We don't have add and remove
	 * action on CB. So call EDU to encode CB data.
	 */
	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cb, &enc->io_uba,
				    EDP_OP_TYPE_ENC, cb, &ederror, enc->i_peer_version);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_cluster_config
 *
 * Purpose:  Encode entire AVD_AVND data.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_cluster_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	switch (enc->io_action) {
	case NCS_MBCSV_ACT_UPDATE:
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cluster, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_AVND *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version);
		break;
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_node_config
 *
 * Purpose:  Encode entire AVD_AVND data.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is update or add, encode entire data. If it is rmv
	 * send key information only.
	 */
	switch (enc->io_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_AVND *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version);
		break;
	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_APP*)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 1, 3);
		break;
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_app_config
 *
 * Purpose:  Encode entire AVD_AVND data.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is update or add, encode entire data. If it is rmv
	 * send key information only.
	 */
	switch (enc->io_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_app, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_APP *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version);
		break;
	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_app, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_APP*)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 1, 1);
		break;
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_sg_config
 *
 * Purpose:  Encode entire AVD_SG data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is update or add, encode entire data. If it is rmv
	 * send key information only.
	 */
	switch (enc->io_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_SG *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version);
		break;
	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_SG *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 1, 1);
		break;
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_su_config
 *
 * Purpose:  Encode entire AVD_SU data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is update or add, encode entire data. If it is rmv
	 * send key information only.
	 */
	switch (enc->io_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version);
		break;
	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 1, 1);
		break;
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_si_config
 *
 * Purpose:  Encode entire AVD_SI data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is update or add, encode entire data. If it is rmv
	 * send key information only.
	 */
	switch (enc->io_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_SI *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_SI *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 1, 1);
		break;

	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_sg_admin_si
 *
 * Purpose:  Encode entire AVD_SG_ADMIN_SI data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. 
	 */
	switch (enc->io_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_RMV:
		/*
		 * Send SI key.
		 */
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
						EDP_OP_TYPE_ENC,
						(AVD_SI
						 *)((AVD_SG *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)))->admin_si,
						&ederror, enc->i_peer_version, 1, 1);
		break;

	case NCS_MBCSV_ACT_UPDATE:
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_siass
 *
 * Purpose:  Encode entire AVD_SU_SI_REL data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVSV_SU_SI_REL_CKPT_MSG su_si_ckpt;
	EDU_ERR ederror = 0;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is update or add, encode entire data. If it is rmv
	 * send key information only. In this case key is SU and SI key.
	 */
	su_si_ckpt.su_name = ((AVD_SU_SI_REL *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)))->su->name;
	su_si_ckpt.si_name = ((AVD_SU_SI_REL *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)))->si->name;

	switch (enc->io_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		su_si_ckpt.fsm = ((AVD_SU_SI_REL *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)))->fsm;
		su_si_ckpt.state = ((AVD_SU_SI_REL *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)))->state;

		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_siass,
			&enc->io_uba, EDP_OP_TYPE_ENC, &su_si_ckpt, &ederror, enc->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_siass, &enc->io_uba,
			EDP_OP_TYPE_ENC, &su_si_ckpt, &ederror, enc->i_peer_version, 2, 1, 2);
		break;

	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_comp_config
 *
 * Purpose:  Encode entire AVD_COMP data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is update or add, encode entire data. If it is rmv
	 * send key information only.
	 */
	switch (enc->io_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_UPDATE:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_COMP *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_COMP *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 1, 1);
		break;

	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_oper_su
 *
 * Purpose:  Encode Operation SU name.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_oper_su(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * In case of both Add and remove request send the operation SU name. 
	 * We don't have update for this reo_type.
	 */
	switch (enc->io_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_RMV:
		/* Send entire data */
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
						EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
						&ederror, enc->i_peer_version, 1, 1);
		break;

	case NCS_MBCSV_ACT_UPDATE:
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	}
	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_node_up_info
 *
 * Purpose:  Encode avnd node up info.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_node_up_info(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node, &enc->io_uba,
						EDP_OP_TYPE_ENC, (AVD_AVND *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
						&ederror, enc->i_peer_version, 6, 1, 2, 4, 5, 6, 7);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);

	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_node_admin_state
 *
 * Purpose:  Encode avnd su admin state info.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_node_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node, &enc->io_uba,
						EDP_OP_TYPE_ENC, (AVD_AVND *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
						&ederror, enc->i_peer_version, 2, 3, 8);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_node_oper_state
 *
 * Purpose:  Encode avnd oper state info.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_node_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node, &enc->io_uba,
						EDP_OP_TYPE_ENC, (AVD_AVND *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
						&ederror, enc->i_peer_version, 2, 3, 9);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_node_state
 *
 * Purpose:  Encode avnd node state info.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_node_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node, &enc->io_uba,
						EDP_OP_TYPE_ENC, (AVD_AVND *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
						&ederror, enc->i_peer_version, 2, 3, 10);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_node_rcv_msg_id
 *
 * Purpose:  Encode avnd receive message ID.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_node_rcv_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node, &enc->io_uba,
						EDP_OP_TYPE_ENC, (AVD_AVND *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
						&ederror, enc->i_peer_version, 2, 1, 12);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_node_snd_msg_id
 *
 * Purpose:  Encode avnd Send message ID.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_node_snd_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node, &enc->io_uba,
						EDP_OP_TYPE_ENC, (AVD_AVND *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
						&ederror, enc->i_peer_version, 2, 1, 13);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_sg_admin_state
 *
 * Purpose:  Encode SG admin state.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_sg_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SG *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 2);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_sg_su_assigned_num
 *
 * Purpose:  Encode SG su assign number.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_sg_su_assigned_num(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);

	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SG *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 3);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_sg_su_spare_num
 *
 * Purpose:  Encode SG su spare number.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_sg_su_spare_num(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SG *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 4);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_sg_su_uninst_num
 *
 * Purpose:  Encode SG su uninst number.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_sg_su_uninst_num(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SG *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 5);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_sg_adjust_state
 *
 * Purpose:  Encode SG adjust state.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_sg_adjust_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SG *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 6);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_sg_fsm_state
 *
 * Purpose:  Encode SG FSM state.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_sg_fsm_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SG *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 7);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_su_preinstan
 *
 * Purpose:  Encode SU preinstatible object.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_su_preinstan(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 2);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_su_oper_state
 *
 * Purpose:  Encode SU Operation state.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_su_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 3);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_su_admin_state
 *
 * Purpose:  Encode SU Admin state.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_su_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 4);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_su_rediness_state
 *
 * Purpose:  Encode SU Rediness state.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_su_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 5);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_su_pres_state
 *
 * Purpose:  Encode SU Presence state.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_su_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 6);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_su_si_curr_active
 *
 * Purpose:  Encode SU Current number of Active SI.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_su_si_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 8);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_su_si_curr_stby
 *
 * Purpose:  Encode SU Current number of Standby SI.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_su_si_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 9);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_su_term_state
 *
 * Purpose:  Encode SU Admin state to terminate service.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_su_term_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 11);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_su_switch
 *
 * Purpose:  Encode SU toggle SI.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_su_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 12);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_su_act_state
 *
 * Purpose:  Encode SU action state.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_su_act_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SU *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 13);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_si_admin_state
 *
 * Purpose:  Encode SI admin state.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_si_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SI *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 2);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_si_assignment_state
 *
 * Purpose:  Encode SI assignment state.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_si_assignment_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SI *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 3);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_si_su_curr_active
 *
 * Purpose:  Encode number of active SU assignment for this SI.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_si_su_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SI *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 4);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_si_su_curr_stby
 *
 * Purpose:  Encode number of standby SU assignment for this SI.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_si_su_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SI *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 5);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_si_switch
 *
 * Purpose:  Encode SI switch.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_si_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SI *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 6);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_si_alarm_sent
 *
 * Purpose:  Encode SI alarm sent
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_si_alarm_sent(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(NCS_MBCSV_ACT_UPDATE == enc->io_action);
	status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
		EDP_OP_TYPE_ENC, (AVD_SI *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
		&ederror, enc->i_peer_version, 2, 1, 8);

	assert(status == NCSCC_RC_SUCCESS);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_comp_proxy_comp_name
 *
 * Purpose:  Encode COMP proxy compnent name.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_comp_proxy_comp_name(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &enc->io_uba,
						EDP_OP_TYPE_ENC, (AVD_COMP *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
						&ederror, enc->i_peer_version, 2, 1, 6);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_comp_curr_num_csi_actv
 *
 * Purpose:  Encode COMP Current number of CSI active.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_comp_curr_num_csi_actv(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(0);

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_COMP *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 2, 1, 32);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_comp_curr_num_csi_stby
 *
 * Purpose:  Encode COMP Current number of CSI standby.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_comp_curr_num_csi_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	assert(0);

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_COMP *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 2, 1, 33);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_comp_oper_state
 *
 * Purpose:  Encode COMP Operation State.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_comp_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_COMP *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 2, 1, 2);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_comp_readiness_state
 *
 * Purpose:  Encode COMP Presence State.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_comp_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_COMP *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 2, 1, 3);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_comp_pres_state
 *
 * Purpose:  Encode COMP Presence State.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_comp_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_COMP *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 2, 1, 4);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_comp_restart_count
 *
 * Purpose:  Encode COMP Restart count.
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_ckpt_comp_restart_count(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Action in this case is just to update. If action passed is add/rmv then log
	 * error. Call EDU encode to encode this field.
	 */
	if (NCS_MBCSV_ACT_UPDATE == enc->io_action) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_COMP *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 2, 1, 5);

		if (status != NCSCC_RC_SUCCESS)
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
	} else
		assert(0);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp
 *
 * Purpose:  Encode cold sync response message.
 *
 * Input: cb  - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avsv_encode_cold_sync_rsp(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	TRACE_ENTER();
	return avd_entire_data_update(cb, enc, TRUE);
}

/****************************************************************************\
 * Function: avd_entire_data_update
 *
 * Purpose:  Encode entire data to be sent during cold sync or warm sync.
 *
 * Input: cb  - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *        c_sync - TRUE - Called while in cold sync.
 *                 FALSE - Called while in warm sync.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avd_entire_data_update(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, NCS_BOOL c_sync)
{
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 num_of_obj = 0;
	uns8 *encoded_cnt_loc;

	TRACE_ENTER2("%u", enc->io_reo_type);

	/* 
	 * Since at decode we need to find out how many objects of particular data
	 * type are being sent, encode that information at the begining of the message.
	 */
	encoded_cnt_loc = ncs_enc_reserve_space(&enc->io_uba, sizeof(uns32));
	if (!encoded_cnt_loc) {
		/* Handle error */
	}
	ncs_enc_claim_space(&enc->io_uba, sizeof(uns32));

	/* 
	 * If reo_handle and reo_type is NULL then this the first time mbcsv is calling
	 * the cold sync response for the standby. So start from first data structure 
	 * which is CB. Next time onwards depending on the value of reo_type and reo_handle
	 * send the next data structures.
	 */
	status = avsv_enc_cold_sync_rsp_data_func_list[enc->io_reo_type] (cb, enc, &num_of_obj);

	/* Now encode the number of objects actually in the UBA. */
	if (encoded_cnt_loc != NULL) {
		ncs_encode_32bit(&encoded_cnt_loc, num_of_obj);
	}

	/*
	 * Check if we have reached to last message required to be sent in cold sync 
	 * response, if yes then send cold sync complete. Else ask MBCSv to call you 
	 * back with the next reo_type.
	 */
	if (AVSV_COLD_SYNC_RSP_ASYNC_UPDT_CNT == enc->io_reo_type) {
		if (c_sync) {
			enc->io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
			saflog(LOG_NOTICE, amfSvcUsrName, "Cold sync complete of %x", cb->node_id_avd_other);
		} else
			enc->io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
	} else
		enc->io_reo_type++;

	TRACE_LEAVE();
	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_cb_config
 *
 * Purpose:  Encode entire CB data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_cb_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Send the CB data.
	 */
	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cb, &enc->io_uba,
				    EDP_OP_TYPE_ENC, cb, &ederror, enc->i_peer_version);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
		return NCSCC_RC_FAILURE;
	}

	*num_of_obj = 1;

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_cluster_config
 *
 * Purpose:  Encode entire AVD_CLUSTER data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_cluster_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cluster, &enc->io_uba,
		EDP_OP_TYPE_ENC, avd_cluster, &ederror, enc->i_peer_version);
	
	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
		return NCSCC_RC_FAILURE;
	}

	(*num_of_obj)++;

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_node_config
 *
 * Purpose:  Encode entire AVD_AVND data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	SaClmNodeIdT node_id = 0;
	AVD_AVND *avnd;
	EDU_ERR ederror = 0;

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	while (NULL != (avnd = avd_node_getnext_nodeid(node_id))) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
					    &enc->io_uba, EDP_OP_TYPE_ENC, avnd, &ederror, enc->i_peer_version);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
			return NCSCC_RC_FAILURE;
		}

		(*num_of_obj)++;
		node_id = avnd->node_info.nodeId;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_app_config
 *
 * Purpose:  Encode entire AVD_APP data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	SaNameT app_name;
	AVD_APP *app;
	EDU_ERR ederror = 0;

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	app_name.length = 0;
	for (app = avd_app_getnext(&app_name); app != NULL; app = avd_app_getnext(&app_name)) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_app, &enc->io_uba,
					    EDP_OP_TYPE_ENC, app, &ederror, enc->i_peer_version);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
			return NCSCC_RC_FAILURE;
		}

		app_name = app->name;
		(*num_of_obj)++;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_sg_config
 *
 * Purpose:  Encode entire AVD_SG data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	SaNameT sg_name;
	AVD_SG *sg;
	EDU_ERR ederror = 0;

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	sg_name.length = 0;
	for (sg = avd_sg_getnext(&sg_name); sg != NULL; sg = avd_sg_getnext(&sg_name)) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_sg, &enc->io_uba,
					    EDP_OP_TYPE_ENC, sg, &ederror, enc->i_peer_version);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
			return NCSCC_RC_FAILURE;
		}

		sg_name = sg->name;
		(*num_of_obj)++;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_su_config
 *
 * Purpose:  Encode entire AVD_SU data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su;
	SaNameT su_name = {0};
	EDU_ERR ederror = 0;

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (su = avd_su_getnext(&su_name); su != NULL;
	     su = avd_su_getnext(&su_name)) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
					    EDP_OP_TYPE_ENC, su, &ederror, enc->i_peer_version);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
			return NCSCC_RC_FAILURE;
		}

		su_name = su->name;
		(*num_of_obj)++;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_si_config
 *
 * Purpose:  Encode entire AVD_SI data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SI *si;
	SaNameT si_name;
	EDU_ERR ederror = 0;

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	si_name.length = 0;
	for (si = avd_si_getnext(&si_name); si != NULL;
	     si = avd_si_getnext(&si_name)) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
					    EDP_OP_TYPE_ENC, si, &ederror, enc->i_peer_version);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
			return NCSCC_RC_FAILURE;
		}

		si_name = si->name;
		(*num_of_obj)++;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_sg_su_oper_list
 *
 * Purpose:  Encode entire AVD_SG_SU_OPER_LIST data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_sg_su_oper_list(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	SaNameT sg_name;
	AVD_SG *sg;

	/* 
	 * Walk through the entire SG list and send the SU operation list
	 * for that SG and then move to next SG.
	 */
	sg_name.length = 0;
	for (sg = avd_sg_getnext(&sg_name); sg != NULL; sg = avd_sg_getnext(&sg_name)) {
		status = avsv_encode_su_oper_list(cb, sg, enc);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: encode failed, %s", __FUNCTION__, sg->name.value);
			return NCSCC_RC_FAILURE;
		}

		sg_name = sg->name;
		(*num_of_obj)++;

	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_sg_admin_si
 *
 * Purpose:  Encode entire AVD_SG_ADMIN_SI data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	SaNameT sg_name;
	AVD_SG *sg;
	EDU_ERR ederror = 0;

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	sg_name.length = 0;
	for (sg = avd_sg_getnext(&sg_name); sg != NULL; sg = avd_sg_getnext(&sg_name)) {
		if (NULL == sg->admin_si) {
			sg_name = sg->name;
			continue;
		}

		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &enc->io_uba,
						EDP_OP_TYPE_ENC, sg->admin_si, &ederror, enc->i_peer_version, 1, 1);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
			return NCSCC_RC_FAILURE;
		}

		sg_name = sg->name;
		(*num_of_obj)++;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_su_si_rel
 *
 * Purpose:  Encode entire AVD_SU_SI_REL data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU *su;
	SaNameT su_name = {0};
	AVSV_SU_SI_REL_CKPT_MSG su_si_ckpt;
	AVD_SU_SI_REL *rel;
	EDU_ERR ederror = 0;

	/* 
	 * Walk through the entire list and send the entire list data.
	 * We will walk the SU tree and all the SU_SI relationship for that SU
	 * are sent.We will send the corresponding COMP_CSI relationship for that SU_SI
	 * in the same update.
	 */
	su_name.length = 0;
	for (su = avd_su_getnext(&su_name); su != NULL;
	     su = avd_su_getnext(&su_name)) {
		su_si_ckpt.su_name = su->name;

		for (rel = su->list_of_susi; rel != NULL; rel = rel->su_next) {
			su_si_ckpt.si_name = rel->si->name;
			su_si_ckpt.fsm = rel->fsm;
			su_si_ckpt.state = rel->state;

			status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_siass,
						    &enc->io_uba, EDP_OP_TYPE_ENC, &su_si_ckpt, &ederror,
						    enc->i_peer_version);

			if (status != NCSCC_RC_SUCCESS) {
				LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
				return status;
			}

			(*num_of_obj)++;
		}
		su_name = su->name;
	}
	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_comp_config
 *
 * Purpose:  Encode entire AVD_COMP data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp;
	SaNameT comp_name = {0};
	EDU_ERR ederror = 0;

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	comp_name.length = 0;
	for (comp = avd_comp_getnext(&comp_name); comp != NULL;
	     comp = avd_comp_getnext(&comp_name)) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &enc->io_uba,
					    EDP_OP_TYPE_ENC, comp, &ederror, enc->i_peer_version);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
			return NCSCC_RC_FAILURE;
		}

		comp_name = comp->comp_info.name;
		(*num_of_obj)++;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_async_updt_cnt
 *
 * Purpose:  Send the latest async update count. This count will be used 
 *           during warm sync for verifying the data at stnadby. 
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_async_updt_cnt(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Encode and send async update counts for all the data structures.
	 */
	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_async_updt_cnt,
				    &enc->io_uba, EDP_OP_TYPE_ENC, &cb->async_updt_cnt, &ederror, enc->i_peer_version);

	if (status != NCSCC_RC_SUCCESS)
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_warm_sync_rsp
 *
 * Purpose:  Encode Warm sync response message.
 *
 * Input: cb  - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avsv_encode_warm_sync_rsp(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/* 
	 * Encode and send latest async update counts. (In the same manner we sent
	 * in the last message of the cold sync response.
	 */
	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_async_updt_cnt,
				    &enc->io_uba, EDP_OP_TYPE_ENC, &cb->async_updt_cnt, &ederror, enc->i_peer_version);

	if (status != NCSCC_RC_SUCCESS)
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_data_sync_rsp
 *
 * Purpose:  Encode Data sync response message.
 *
 * Input: cb  - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avsv_encode_data_sync_rsp(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	TRACE_ENTER();
	return avd_entire_data_update(cb, enc, FALSE);
}

/****************************************************************************\
 * Function: avsv_encode_su_oper_list
 *
 * Purpose:  Encode entire AVD_SG_SU_OPER_LIST data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 avsv_encode_su_oper_list(AVD_CL_CB *cb, AVD_SG *sg, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 num_of_opr_su = 0;
	uns8 *encoded_cnt_loc;
	AVD_SG_OPER *oper_list_ptr = NULL;
	EDU_ERR ederror = 0;

	/* Reserve space for the number of operation SU to be encoded */
	encoded_cnt_loc = ncs_enc_reserve_space(&enc->io_uba, sizeof(uns32));
	assert(encoded_cnt_loc);
	ncs_enc_claim_space(&enc->io_uba, sizeof(uns32));

	/*
	 * Now walk through the entire SU operation list and encode it.
	 */
	for (oper_list_ptr = &sg->su_oper_list;
	     ((oper_list_ptr != NULL) && (oper_list_ptr->su != NULL)); oper_list_ptr = oper_list_ptr->next) {
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &enc->io_uba,
						EDP_OP_TYPE_ENC, oper_list_ptr->su, &ederror, enc->i_peer_version, 1,
						1);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
			return status;
		}
		num_of_opr_su++;
	}

	/* 
	 * Encode the number of operation SU's we are sending in this message.
	 */
	if (encoded_cnt_loc != NULL) {
		ncs_encode_32bit(&encoded_cnt_loc, num_of_opr_su);
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_ckpt_avd_comp_cs_type_config
 *
 * Purpose:  Encode entire AVD_COMP_CS_TYPE data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 *
\**************************************************************************/
static uns32 avsv_encode_ckpt_avd_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc)
{
	uns32 status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	/*
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is update or add, encode entire data. If it is rmv
	 * send key information only.
	 */
	switch (enc->io_action) {
	case NCS_MBCSV_ACT_UPDATE:
	case NCS_MBCSV_ACT_ADD:
		/* Send entire data */
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp_cs_type, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_COMPCS_TYPE *)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)), &ederror,
			enc->i_peer_version);
		break;
	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp_cs_type, &enc->io_uba,
			EDP_OP_TYPE_ENC, (AVD_COMPCS_TYPE*)(NCS_INT64_TO_PTR_CAST(enc->io_reo_hdl)),
			&ederror, enc->i_peer_version, 1, 1);
		break;
	default:
		assert(0);
	}

	if (status != NCSCC_RC_SUCCESS)
		LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);

	return status;
}

/****************************************************************************\
 * Function: avsv_encode_cold_sync_rsp_avd_comp_cs_type_config
 *
 * Purpose:  Encode entire AVD_COMP_CS_TYPE data..
 *
 * Input: cb - CB pointer.
 *        enc - Encode arguments passed by MBCSV.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 *
\**************************************************************************/
static uns32 avsv_encode_cold_sync_rsp_avd_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_ENC *enc, uns32 *num_of_obj)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_COMPCS_TYPE *compcstype;
	EDU_ERR ederror = 0;
	SaNameT dn = {0};

	/*
	 * Walk through the entire list and send the entire list data.
	 */
	dn.length = 0;
	for (compcstype = avd_compcstype_getnext(&dn); compcstype != NULL;
	     compcstype = avd_compcstype_getnext(&dn)) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp_cs_type, &enc->io_uba,
					    EDP_OP_TYPE_ENC, compcstype, &ederror, enc->i_peer_version);

		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: encode failed, ederror=%u", __FUNCTION__, ederror);
			break;
		}

		dn = compcstype->name;
		(*num_of_obj)++;
	}

	return status;
}

