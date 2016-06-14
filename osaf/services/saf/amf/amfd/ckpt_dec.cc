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
#include <amfd.h>
#include <cluster.h>
#include <si_dep.h>
#include <sg.h>

extern "C" const AVSV_DECODE_CKPT_DATA_FUNC_PTR avd_dec_data_func_list[AVSV_CKPT_MSG_MAX];

/* Declaration of async update functions */
static uint32_t dec_cb_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_cluster_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_si_trans(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_oper_su(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_node_up_info(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_node_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_node_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_node_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_node_rcv_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_node_snd_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_sg_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_sg_adjust_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_sg_su_assigned_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_sg_su_spare_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_sg_su_uninst_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_sg_fsm_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_si_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_si_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_term_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_act_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_preinstan(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_su_restart_count(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_si_su_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_si_su_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_si_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_si_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_si_assignment_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_si_dep_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_si_alarm_sent(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_comp_proxy_comp_name(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_comp_curr_num_csi_actv(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_comp_curr_num_csi_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_comp_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_comp_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_comp_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_comp_restart_count(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_cs_oper_su(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t dec_ng_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);

/* Declaration of static cold sync decode functions */
static uint32_t dec_cs_cb_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_cluster_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_sg_su_oper_list(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_si_trans(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_async_updt_cnt(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);
static uint32_t dec_cs_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj);

/*
 * Function list for decoding the async data.
 * We will jump into this function using the reo_type received 
 * in the decode argument.
 *
 * This array _must_ correspond to avsv_ckpt_msg_reo_type in ckpt_msg.h
 */

const AVSV_DECODE_CKPT_DATA_FUNC_PTR avd_dec_data_func_list[] = {
	dec_cb_config,
	dec_cluster_config,
	dec_node_config,
	dec_app_config,
	dec_sg_config,
	dec_su_config,
	dec_si_config,
	dec_oper_su,
	dec_sg_admin_si,
	dec_comp_config,
	dec_comp_cs_type_config,
	dec_siass,
	dec_si_trans,

	/* 
	 * Messages to update independent fields.
	 */

	/* AVND Async Update messages */
	dec_node_admin_state,
	dec_node_oper_state,
	dec_node_up_info,
	dec_node_state,
	dec_node_rcv_msg_id,
	dec_node_snd_msg_id,

	/* SG Async Update messages */
	dec_sg_admin_state,
	dec_sg_su_assigned_num,
	dec_sg_su_spare_num,
	dec_sg_su_uninst_num,
	dec_sg_adjust_state,
	dec_sg_fsm_state,

	/* SU Async Update messages */
	dec_su_preinstan,
	dec_su_oper_state,
	dec_su_admin_state,
	dec_su_readiness_state,
	dec_su_pres_state,
	dec_su_si_curr_active,
	dec_su_si_curr_stby,
	dec_su_term_state,
	dec_su_switch,
	dec_su_act_state,

	/* SI Async Update messages */
	dec_si_admin_state,
	dec_si_assignment_state,
	dec_si_su_curr_active,
	dec_si_su_curr_stby,
	dec_si_switch,
	dec_si_alarm_sent,

	/* COMP Async Update messages */
	dec_comp_proxy_comp_name,
	dec_comp_curr_num_csi_actv,
	dec_comp_curr_num_csi_stby,
	dec_comp_oper_state,
	dec_comp_readiness_state,
	dec_comp_pres_state,
	dec_comp_restart_count,
	NULL,			/* AVSV_SYNC_COMMIT */
	dec_su_restart_count,
	dec_si_dep_state,
	dec_ng_admin_state
};

/*
 * Function list for decoding the cold sync response data
 * We will jump into this function using the reo_type received 
 * in the cold sync response argument.
 */
const AVSV_DECODE_COLD_SYNC_RSP_DATA_FUNC_PTR dec_cs_data_func_list[] = {
	dec_cs_cb_config,
	dec_cs_cluster_config,
	dec_cs_node_config,
	dec_cs_app_config,
	dec_cs_sg_config,
	dec_cs_su_config,
	dec_cs_si_config,
	dec_cs_sg_su_oper_list,
	dec_cs_sg_admin_si,
	dec_cs_comp_config,
	dec_cs_comp_cs_type_config,
	dec_cs_siass,
	dec_cs_si_trans,
	dec_cs_async_updt_cnt
};

/****************************************************************************\
 * Function: dec_cb_config
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
static uint32_t dec_cb_config(AVD_CL_CB *cb_ptr, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
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

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_avnd_config
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
static uint32_t dec_cluster_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_CLUSTER dec_cluster;
	AVD_CLUSTER *cluster = &dec_cluster;

	TRACE_ENTER();

	osafassert(dec->i_action == NCS_MBCSV_ACT_UPDATE);

	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cluster,
		&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_CLUSTER **)&cluster, &ederror,
		dec->i_peer_version);
	osafassert(status == NCSCC_RC_SUCCESS);

	avd_cluster->saAmfClusterAdminState = cluster->saAmfClusterAdminState;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dec_node_config
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
static uint32_t dec_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);

	TRACE_ENTER2("i_action '%u'", dec->i_action);

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
		osafassert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	status = avd_ckpt_node(cb, avnd_ptr, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.node_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

static void decode_app(NCS_UBAID *ub, AVD_APP *app)
{
	osaf_decode_sanamet(ub, &app->name);
	osaf_decode_uint32(ub, (uint32_t*)&app->saAmfApplicationAdminState);
	osaf_decode_uint32(ub, &app->saAmfApplicationCurrNumSGs);
}

/****************************************************************************\
 * Function: dec_app_config
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
static uint32_t dec_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_APP app;
	
	osafassert(dec->i_action == NCS_MBCSV_ACT_UPDATE);
	decode_app(&dec->i_uba, &app);
	
	status = avd_ckpt_app(cb, &app, dec->i_action);

	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.app_updt++;

	TRACE_LEAVE2("status:%u, su_updt:%d", status, cb->async_updt_cnt.app_updt);
	return status;
}

static void decode_sg(NCS_UBAID *ub, AVD_SG *sg)
{
	osaf_decode_sanamet(ub, &sg->name);
	osaf_decode_uint32(ub, (uint32_t*)&sg->saAmfSGAdminState);
	osaf_decode_uint32(ub, &sg->saAmfSGNumCurrAssignedSUs);
	osaf_decode_uint32(ub, &sg->saAmfSGNumCurrInstantiatedSpareSUs);
	osaf_decode_uint32(ub, &sg->saAmfSGNumCurrNonInstantiatedSpareSUs);
	osaf_decode_uint32(ub, (uint32_t*)&sg->adjust_state);
	osaf_decode_uint32(ub, (uint32_t*)&sg->sg_fsm_state);
}

/****************************************************************************\
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
static uint32_t dec_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SG_2N sg;

	TRACE_ENTER2("i_action '%u'", dec->i_action);

	osafassert(dec->i_action == NCS_MBCSV_ACT_UPDATE);
	decode_sg(&dec->i_uba, &sg);
	uint32_t status = avd_ckpt_sg(cb, &sg, dec->i_action);

	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.sg_updt++;

	TRACE_LEAVE2("status '%u', sg_updt:%d", status, cb->async_updt_cnt.sg_updt);
	return status;
}

static void decode_su(NCS_UBAID *ub, AVD_SU *su, uint16_t peer_version)
{
	osaf_decode_sanamet(ub, &su->name);
	osaf_decode_bool(ub, (bool*)&su->saAmfSUPreInstantiable);
	osaf_decode_uint32(ub, (uint32_t*)&su->saAmfSUOperState);
	osaf_decode_uint32(ub, (uint32_t*)&su->saAmfSUAdminState);
	osaf_decode_uint32(ub, (uint32_t*)&su->saAmfSuReadinessState);
	osaf_decode_uint32(ub, (uint32_t*)&su->saAmfSUPresenceState);
	osaf_decode_sanamet(ub, &su->saAmfSUHostedByNode);
	osaf_decode_uint32(ub, &su->saAmfSUNumCurrActiveSIs);
	osaf_decode_uint32(ub, &su->saAmfSUNumCurrStandbySIs);
	osaf_decode_uint32(ub, &su->saAmfSURestartCount);
	osaf_decode_bool(ub, &su->term_state);
	osaf_decode_uint32(ub, (uint32_t*)&su->su_switch);
	osaf_decode_uint32(ub, (uint32_t*)&su->su_act_state);

	if (peer_version >= AVD_MBCSV_SUB_PART_VERSION_2)
		osaf_decode_bool(ub, &su->su_is_external);
}

/****************************************************************************\
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
static uint32_t dec_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	AVD_SU su;

	TRACE_ENTER2("i_action '%u'", dec->i_action);

	osafassert(dec->i_action == NCS_MBCSV_ACT_UPDATE);
	decode_su(&dec->i_uba, &su, dec->i_peer_version);
	uint32_t status = avd_ckpt_su(cb, &su, dec->i_action);

	if (status == NCSCC_RC_SUCCESS)
		cb->async_updt_cnt.su_updt++;

	TRACE_LEAVE2("status:%u, su_updt:%d", status, cb->async_updt_cnt.su_updt);
	return status;
}

static void decode_si(NCS_UBAID *ub,
	AVD_SI *si,
	const uint16_t peer_version)
{
#ifdef UPGRADE_FROM_4_2_1
	// special case for 4.2.1, si_dep_state should be check pointed using ver 4
	uint16_t ver_compare = AVD_MBCSV_SUB_PART_VERSION_4;
#else
	// default case, si_dep_stateavd_peer_ver should not be check pointed for peers in ver 4 (or less)
	uint16_t ver_compare = AVD_MBCSV_SUB_PART_VERSION_5;
#endif
	TRACE_ENTER2("my_version: %u, to_version: %u", ver_compare, peer_version);

	osaf_decode_sanamet(ub, &si->name);
	osaf_decode_uint32(ub, (uint32_t*)&si->saAmfSIAdminState);
	osaf_decode_uint32(ub, (uint32_t*)&si->saAmfSIAssignmentState);
	osaf_decode_uint32(ub, (uint32_t*)&si->saAmfSINumCurrActiveAssignments);
	osaf_decode_uint32(ub, (uint32_t*)&si->saAmfSINumCurrStandbyAssignments);
	osaf_decode_uint32(ub, (uint32_t*)&si->si_switch);
	osaf_decode_sanamet(ub, &si->saAmfSIProtectedbySG);
	osaf_decode_bool(ub, &si->alarm_sent);

	if (peer_version >= ver_compare) {
		osaf_decode_uint32(ub, (uint32_t*)&si->si_dep_state);
	}
}

/****************************************************************************\
 * Function: dec_si_config
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
static uint32_t dec_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	AVD_SI si;
	
	TRACE_ENTER2("i_action '%u'", dec->i_action);
	osafassert(dec->i_action == NCS_MBCSV_ACT_UPDATE);
	decode_si(&dec->i_uba, &si, dec->i_peer_version);
	uint32_t status = avd_ckpt_si(cb, &si, dec->i_action);

	if (status == NCSCC_RC_SUCCESS)
		cb->async_updt_cnt.si_updt++;

	TRACE_LEAVE2("status:%u, si_updt:%d", status, cb->async_updt_cnt.si_updt);
	return status;
}

/****************************************************************************\
 * Function: dec_sg_admin_si
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
static uint32_t dec_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("i_action '%u'", dec->i_action);

	status = avd_ckpt_sg_admin_si(cb, &dec->i_uba, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.sg_admin_si_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/**************************************************************************
 * @brief  decodes the si transfer parameters 
 * @param[in] cb
 * @param[in] dec
 *************************************************************************/
static uint32_t dec_si_trans(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVSV_SI_TRANS_CKPT_MSG *si_trans_ckpt;
	AVSV_SI_TRANS_CKPT_MSG dec_si_trans_ckpt;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);

	TRACE_ENTER2("i_action '%u'", dec->i_action);

	si_trans_ckpt = &dec_si_trans_ckpt;

	switch (dec->i_action) {
	case NCS_MBCSV_ACT_ADD:
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si_trans,
			&dec->i_uba, EDP_OP_TYPE_DEC, (AVSV_SI_TRANS_CKPT_MSG **)&si_trans_ckpt,
			&ederror, dec->i_peer_version);
		break;

	case NCS_MBCSV_ACT_RMV:
		/* Send only key information */
		status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_si_trans, &dec->i_uba,
			EDP_OP_TYPE_DEC, (AVSV_SI_TRANS_CKPT_MSG **)&si_trans_ckpt, &ederror, 1, 1);
		break;

	default:
		osafassert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	avd_ckpt_si_trans(cb, si_trans_ckpt, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.si_trans_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_siass
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
static uint32_t dec_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVSV_SU_SI_REL_CKPT_MSG *su_si_ckpt;
	AVSV_SU_SI_REL_CKPT_MSG dec_su_si_ckpt;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);

	TRACE_ENTER2("i_action '%u'", dec->i_action);

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
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_siass,
				&dec->i_uba, EDP_OP_TYPE_DEC, (AVSV_SU_SI_REL_CKPT_MSG **)&su_si_ckpt,
				&ederror, dec->i_peer_version);
		break;

	default:
		osafassert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	avd_ckpt_siass(cb, su_si_ckpt, dec);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.siass_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_comp_config
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
static uint32_t dec_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);

	TRACE_ENTER2("i_action '%u'", dec->i_action);
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
		osafassert(0);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return status;
	}

	status = avd_ckpt_comp(cb, comp_ptr, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.comp_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
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
static uint32_t dec_oper_su(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	SaNameT name;

	TRACE_ENTER2("i_action '%u'", dec->i_action);

	switch (dec->i_action) {
	case NCS_MBCSV_ACT_ADD:
	case NCS_MBCSV_ACT_RMV:
		osaf_decode_sanamet(&dec->i_uba, &name);
		break;
	case NCS_MBCSV_ACT_UPDATE:
	default:
		osafassert(0);
	}

	status = avd_ckpt_su_oper_list(&name, dec->i_action);

	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.sg_su_oprlist_updt++;

	TRACE_LEAVE2("'%s', status '%u', updt %d",
		name.value, status, cb->async_updt_cnt.sg_su_oprlist_updt);
	return status;
}

/****************************************************************************\
 * Function: dec_node_up_info
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
static uint32_t dec_node_up_info(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
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
	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_node_admin_state
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
static uint32_t dec_node_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 2, 3, 8);

	osafassert(status == NCSCC_RC_SUCCESS);

	if (NULL == (avnd_struct = avd_node_get(&avnd_ptr->name))) {
		LOG_ER("%s: node not found, nodeid=%s", __FUNCTION__, avnd_ptr->name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->saAmfNodeAdminState = avnd_ptr->saAmfNodeAdminState;

	cb->async_updt_cnt.node_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_node_oper_state
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
static uint32_t dec_node_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 2, 3, 9);

	osafassert(status == NCSCC_RC_SUCCESS);

	if (NULL == (avnd_struct = avd_node_get(&avnd_ptr->name))) {
		LOG_ER("%s: node not found, nodeid=%s", __FUNCTION__, avnd_ptr->name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->saAmfNodeOperState = avnd_ptr->saAmfNodeOperState;

	cb->async_updt_cnt.node_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_node_state
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
static uint32_t dec_node_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 2, 3, 10);

	osafassert(status == NCSCC_RC_SUCCESS);

	if (NULL == (avnd_struct = avd_node_get(&avnd_ptr->name))) {
		LOG_ER("%s: node not found, nodeid=%s", __FUNCTION__, avnd_ptr->name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->node_state = avnd_ptr->node_state;

	cb->async_updt_cnt.node_updt++;
	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_node_rcv_msg_id
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
static uint32_t dec_node_rcv_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 2, 1, 12);

	osafassert(status == NCSCC_RC_SUCCESS);

	if (NULL == (avnd_struct = avd_node_find_nodeid(avnd_ptr->node_info.nodeId))) {
		LOG_ER("%s: node not found, nodeid=%x", __FUNCTION__, avnd_ptr->node_info.nodeId);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->rcv_msg_id = avnd_ptr->rcv_msg_id;

	cb->async_updt_cnt.node_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_avnd_snd_msg_id
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
static uint32_t dec_node_snd_msg_id(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_AVND *avnd_ptr;
	AVD_AVND dec_avnd;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_AVND *avnd_struct;

	TRACE_ENTER();

	avnd_ptr = &dec_avnd;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_node,
			      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_AVND **)&avnd_ptr, &ederror, 2, 1, 13);

	osafassert(status == NCSCC_RC_SUCCESS);

	if (NULL == (avnd_struct = avd_node_find_nodeid(avnd_ptr->node_info.nodeId))) {
		LOG_ER("%s: node not found, nodeid=%x", __FUNCTION__, avnd_ptr->node_info.nodeId);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	avnd_struct->snd_msg_id = avnd_ptr->snd_msg_id;

	cb->async_updt_cnt.node_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
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
static uint32_t dec_sg_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SG *sg = sg_db->find(Amf::to_string(&name));
	osafassert(sg != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&sg->saAmfSGAdminState);

	cb->async_updt_cnt.sg_updt++;

	//For 2N model check if this checkpointing being done in the context of nodegroup operation.
	if ((sg->sg_ncs_spec == false) && (sg->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL)) {
		if (((sg->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) ||
			 (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED)) &&
			 (sg->ng_using_saAmfSGAdminState == false)) { 
			for (std::map<std::string, AVD_AMF_NG*>::const_iterator it = nodegroup_db->begin();
					it != nodegroup_db->end(); it++) {
				AVD_AMF_NG *ng = it->second;
				if ((sg->is_sg_assigned_only_in_ng(ng) == true) &&
						((ng->saAmfNGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) ||
						 (ng->saAmfNGAdminState == SA_AMF_ADMIN_LOCKED)) &&
						(sg->sg_fsm_state == AVD_SG_FSM_STABLE)) {
					sg->ng_using_saAmfSGAdminState = true;
					break;
				}
			}
		} else if ((sg->saAmfSGAdminState == SA_AMF_ADMIN_UNLOCKED) &&
			(sg->ng_using_saAmfSGAdminState == true))
			sg->ng_using_saAmfSGAdminState = false;
	}
	TRACE("ng_using_saAmfSGAdminState:%u",sg->ng_using_saAmfSGAdminState);

	TRACE_LEAVE2("'%s', saAmfSGAdminState=%u, sg_updt:%d",
		sg->name.value, sg->saAmfSGAdminState, cb->async_updt_cnt.sg_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_sg_su_assigned_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SG *sg = sg_db->find(Amf::to_string(&name));
	osafassert(sg != NULL);
	osaf_decode_uint32(&dec->i_uba, &sg->saAmfSGNumCurrAssignedSUs);

	cb->async_updt_cnt.sg_updt++;

	TRACE_LEAVE2("'%s', saAmfSGNumCurrAssignedSUs=%u, sg_updt:%d",
		sg->name.value, sg->saAmfSGNumCurrAssignedSUs,
		cb->async_updt_cnt.sg_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_sg_su_spare_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SG *sg = sg_db->find(Amf::to_string(&name));
	osafassert(sg != NULL);
	osaf_decode_uint32(&dec->i_uba, &sg->saAmfSGNumCurrInstantiatedSpareSUs);

	cb->async_updt_cnt.sg_updt++;

	TRACE_LEAVE2("'%s', saAmfSGNumCurrInstantiatedSpareSUs=%u, sg_updt:%d",
		sg->name.value, sg->saAmfSGNumCurrInstantiatedSpareSUs,
		cb->async_updt_cnt.sg_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_sg_su_uninst_num(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SG *sg = sg_db->find(Amf::to_string(&name));
	osafassert(sg != NULL);
	osaf_decode_uint32(&dec->i_uba, &sg->saAmfSGNumCurrNonInstantiatedSpareSUs);

	cb->async_updt_cnt.sg_updt++;

	TRACE_LEAVE2("'%s', saAmfSGNumCurrNonInstantiatedSpareSUs=%u, sg_updt:%d",
		sg->name.value, sg->saAmfSGNumCurrNonInstantiatedSpareSUs,
		cb->async_updt_cnt.sg_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_sg_adjust_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SG *sg = sg_db->find(Amf::to_string(&name));
	osafassert(sg != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&sg->adjust_state);

	cb->async_updt_cnt.sg_updt++;

	TRACE_LEAVE2("'%s', adjust_state=%u, sg_updt:%d",
		sg->name.value, sg->adjust_state, cb->async_updt_cnt.sg_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_sg_fsm_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SG *sg = sg_db->find(Amf::to_string(&name));
	osafassert(sg != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&sg->sg_fsm_state);

	cb->async_updt_cnt.sg_updt++;

	TRACE_LEAVE2("'%s', sg_fsm_state=%u, sg_updt:%d",
		sg->name.value, sg->sg_fsm_state, cb->async_updt_cnt.sg_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_su_preinstan(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SU *su = su_db->find(Amf::to_string(&name));
	osafassert(su != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&su->saAmfSUPreInstantiable);

	cb->async_updt_cnt.su_updt++;

	TRACE_LEAVE2("'%s', saAmfSUPreInstantiable=%u, su_updt:%d",
		name.value, su->saAmfSUPreInstantiable, cb->async_updt_cnt.su_updt);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_su_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	
	AVD_SU *su = su_db->find(Amf::to_string(&name));
	if (su == NULL) {
		TRACE("'%s' does not exist, creating it", name.value);
		su = new AVD_SU(&name);
		unsigned int rc = su_db->insert(Amf::to_string(&su->name), su);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
	
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&su->saAmfSUOperState);

	cb->async_updt_cnt.su_updt++;
	avd_saImmOiRtObjectUpdate(&su->name, "saAmfSUOperState",
		SA_IMM_ATTR_SAUINT32T, &su->saAmfSUOperState);

	TRACE_LEAVE2("'%s', saAmfSUOperState=%u, su_updt:%d",
		name.value, su->saAmfSUOperState, cb->async_updt_cnt.su_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_su_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SU *su = su_db->find(Amf::to_string(&name));
	osafassert(su != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&su->saAmfSUAdminState);

	cb->async_updt_cnt.su_updt++;

	avd_saImmOiRtObjectUpdate(&su->name, "saAmfSUAdminState",
		SA_IMM_ATTR_SAUINT32T, &su->saAmfSUAdminState);
	TRACE_LEAVE2("'%s', saAmfSUAdminState=%u, su_updt:%d",
		name.value, su->saAmfSUAdminState, cb->async_updt_cnt.su_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_su_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SU *su = su_db->find(Amf::to_string(&name));
	osafassert(su != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&su->saAmfSuReadinessState);

	cb->async_updt_cnt.su_updt++;

	avd_saImmOiRtObjectUpdate(&su->name, "saAmfSUReadinessState",
		SA_IMM_ATTR_SAUINT32T, &su->saAmfSuReadinessState);
	TRACE_LEAVE2("'%s', saAmfSuReadinessState=%u, su_updt:%d",
		name.value, su->saAmfSuReadinessState, cb->async_updt_cnt.su_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 *
 * Purpose:  Decode SU Presence state.
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
static uint32_t dec_su_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SU *su = su_db->find(Amf::to_string(&name));
	osafassert(su != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&su->saAmfSUPresenceState);

	cb->async_updt_cnt.su_updt++;

	avd_saImmOiRtObjectUpdate(&su->name, "saAmfSUPresenceState",
		SA_IMM_ATTR_SAUINT32T, &su->saAmfSUPresenceState);
	TRACE_LEAVE2("'%s', saAmfSUPresenceState=%u, su_updt:%d",
		name.value, su->saAmfSUPresenceState, cb->async_updt_cnt.su_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_su_si_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SU *su = su_db->find(Amf::to_string(&name));
	osafassert(su != NULL);
	osaf_decode_uint32(&dec->i_uba, &su->saAmfSUNumCurrActiveSIs);

	cb->async_updt_cnt.su_updt++;

	TRACE_LEAVE2("'%s', saAmfSUNumCurrActiveSIs=%u, su_updt:%d",
		name.value, su->saAmfSUNumCurrActiveSIs, cb->async_updt_cnt.su_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_su_si_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SU *su = su_db->find(Amf::to_string(&name));
	osafassert(su != NULL);
	osaf_decode_uint32(&dec->i_uba, &su->saAmfSUNumCurrStandbySIs);

	cb->async_updt_cnt.su_updt++;

	TRACE_LEAVE2("'%s', saAmfSUNumCurrStandbySIs=%u, su_updt:%d",
		name.value, su->saAmfSUNumCurrStandbySIs, cb->async_updt_cnt.su_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 *
 * Purpose:  Decode SU term state
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
static uint32_t dec_su_term_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SU *su = su_db->find(Amf::to_string(&name));
	osafassert(su != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&su->term_state);

	cb->async_updt_cnt.su_updt++;

	TRACE_LEAVE2("'%s', term_state=%u, su_updt:%d",
		name.value, su->term_state, cb->async_updt_cnt.su_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_su_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SU *su = su_db->find(Amf::to_string(&name));
	osafassert(su != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&su->su_switch);

	cb->async_updt_cnt.su_updt++;

	TRACE_LEAVE2("'%s', su_switch=%u, su_updt:%d",
		name.value, su->su_switch, cb->async_updt_cnt.su_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
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
static uint32_t dec_su_act_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	TRACE_ENTER();
	cb->async_updt_cnt.su_updt++;
	TRACE_LEAVE2("su_updt=%u", cb->async_updt_cnt.su_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 *
 * Purpose:  Decode SU Restart count.
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
static uint32_t dec_su_restart_count(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SU *su = su_db->find(Amf::to_string(&name));
	osafassert(su != NULL);
	osaf_decode_uint32(&dec->i_uba, &su->saAmfSURestartCount);

	cb->async_updt_cnt.su_updt++;

	TRACE_LEAVE2("'%s', saAmfSURestartCount=%u, su_updt:%d",
		name.value, su->saAmfSURestartCount, cb->async_updt_cnt.su_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dec_si_admin_state
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
static uint32_t dec_si_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SI *si = si_db->find(Amf::to_string(&name));
	osafassert(si != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&si->saAmfSIAdminState);

	cb->async_updt_cnt.si_updt++;

	TRACE_LEAVE2("'%s', saAmfSIAdminState=%u, si_updt:%d",
		name.value, si->saAmfSIAdminState, cb->async_updt_cnt.si_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dec_si_assignment_state
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
static uint32_t dec_si_assignment_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SI *si = si_db->find(Amf::to_string(&name));
	osafassert(si != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&si->saAmfSIAssignmentState);

	cb->async_updt_cnt.si_updt++;

	TRACE_LEAVE2("'%s', saAmfSIAssignmentState=%u, si_updt:%d",
		name.value, si->saAmfSIAssignmentState, cb->async_updt_cnt.si_updt);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************
 * @brief    decodes si_dep_state during async update
 *
 * @param[in] cb
 *
 * @param[in] dec
 *
 *****************************************************************/
static uint32_t dec_si_dep_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;
	uint32_t si_dep_state;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SI *si = si_db->find(Amf::to_string(&name));
	if (si == NULL) {
		si = avd_si_new(&name);
		osafassert(si != NULL);
		avd_si_db_add(si);		
	}

	osaf_decode_uint32(&dec->i_uba, &si_dep_state);

	/* Update the fields received in this checkpoint message */
	avd_sidep_si_dep_state_set(si, (AVD_SI_DEP_STATE)si_dep_state);

	cb->async_updt_cnt.si_updt++;

	TRACE_LEAVE2("'%s', si_dep_state=%u, si_updt:%d",
		name.value, si->si_dep_state, cb->async_updt_cnt.si_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dec_si_su_curr_active
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
static uint32_t dec_si_su_curr_active(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SI *si = si_db->find(Amf::to_string(&name));
	osafassert(si != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&si->saAmfSINumCurrActiveAssignments);

	cb->async_updt_cnt.si_updt++;

	TRACE_LEAVE2("'%s', saAmfSINumCurrActiveAssignments=%u, si_updt:%d",
		name.value, si->saAmfSINumCurrActiveAssignments, cb->async_updt_cnt.si_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dec_si_su_curr_stby
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
static uint32_t dec_si_su_curr_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SI *si = si_db->find(Amf::to_string(&name));
	osafassert(si != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&si->saAmfSINumCurrStandbyAssignments);

	cb->async_updt_cnt.si_updt++;

	TRACE_LEAVE2("'%s', saAmfSINumCurrStandbyAssignments=%u, si_updt:%d",
		name.value, si->saAmfSINumCurrStandbyAssignments, cb->async_updt_cnt.si_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dec_si_switch
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
static uint32_t dec_si_switch(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SI *si = si_db->find(Amf::to_string(&name));
	osafassert(si != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&si->si_switch);

	cb->async_updt_cnt.si_updt++;

	TRACE_LEAVE2("'%s', si_switch=%u, si_updt:%d",
		name.value, si->si_switch, cb->async_updt_cnt.si_updt);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dec_si_alarm_sent
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
static uint32_t dec_si_alarm_sent(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_SI *si = si_db->find(Amf::to_string(&name));
	osafassert(si != NULL);
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&si->alarm_sent);

	cb->async_updt_cnt.si_updt++;

	TRACE_LEAVE2("'%s', alarm_sent=%u, si_updt:%d",
		name.value, si->alarm_sent, cb->async_updt_cnt.si_updt);
	return NCSCC_RC_SUCCESS;
}
/****************************************************************************\
 * Function: dec_comp_proxy_comp_name
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
static uint32_t dec_comp_proxy_comp_name(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_COMP *comp_struct;

	TRACE_ENTER();

	comp_ptr = &dec_comp;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror, 2, 1, 6);

	if (status != NCSCC_RC_SUCCESS)
		osafassert(0);

	if (NULL == (comp_struct = comp_db->find(Amf::to_string(&comp_ptr->comp_info.name))))
		osafassert(0);

	/* Update the fields received in this checkpoint message */
	comp_struct->saAmfCompCurrProxyName = comp_ptr->saAmfCompCurrProxyName;

	cb->async_updt_cnt.comp_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_comp_curr_num_csi_actv
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
static uint32_t dec_comp_curr_num_csi_actv(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_COMP *comp_struct;

	TRACE_ENTER();

	comp_ptr = &dec_comp;

	/* 
	 * Action in this case is just to update.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_comp,
	      &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMP **)&comp_ptr, &ederror, 2, 1, 32);

	if (status != NCSCC_RC_SUCCESS)
		osafassert(0);

	if (NULL == (comp_struct = comp_db->find(Amf::to_string(&comp_ptr->comp_info.name)))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->curr_num_csi_actv = comp_ptr->curr_num_csi_actv;

	cb->async_updt_cnt.comp_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_comp_curr_num_csi_stby
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
static uint32_t dec_comp_curr_num_csi_stby(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
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

	if (NULL == (comp_struct = comp_db->find(Amf::to_string(&comp_ptr->comp_info.name)))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->curr_num_csi_stdby = comp_ptr->curr_num_csi_stdby;

	cb->async_updt_cnt.comp_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_comp_oper_state
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
static uint32_t dec_comp_oper_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
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

	comp_struct = avd_comp_get_or_create(&comp_ptr->comp_info.name);

	/* Update the fields received in this checkpoint message */
	comp_struct->saAmfCompOperState = comp_ptr->saAmfCompOperState;
	avd_saImmOiRtObjectUpdate(&comp_ptr->comp_info.name, "saAmfCompOperState",
		SA_IMM_ATTR_SAUINT32T, &comp_ptr->saAmfCompOperState);

	cb->async_updt_cnt.comp_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_comp_readiness_state
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
static uint32_t dec_comp_readiness_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
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

	if (NULL == (comp_struct = comp_db->find(Amf::to_string(&comp_ptr->comp_info.name)))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->saAmfCompReadinessState = comp_ptr->saAmfCompReadinessState;

	cb->async_updt_cnt.comp_updt++;

	avd_saImmOiRtObjectUpdate(&comp_struct->comp_info.name, "saAmfCompReadinessState",
		SA_IMM_ATTR_SAUINT32T, &comp_struct->saAmfCompReadinessState);
	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_comp_pres_state
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
static uint32_t dec_comp_pres_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
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

	if (NULL == (comp_struct = comp_db->find(Amf::to_string(&comp_ptr->comp_info.name)))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->saAmfCompPresenceState = comp_ptr->saAmfCompPresenceState;

	cb->async_updt_cnt.comp_updt++;
	avd_saImmOiRtObjectUpdate(&comp_struct->comp_info.name, "saAmfCompPresenceState",
		SA_IMM_ATTR_SAUINT32T, &comp_struct->saAmfCompPresenceState);

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_comp_restart_count
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
static uint32_t dec_comp_restart_count(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
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

	if (NULL == (comp_struct = comp_db->find(Amf::to_string(&comp_ptr->comp_info.name)))) {
		LOG_ER("%s: comp not found, %s", __FUNCTION__, comp_ptr->comp_info.name.value);
		return NCSCC_RC_FAILURE;
	}

	/* Update the fields received in this checkpoint message */
	comp_struct->saAmfCompRestartCount = comp_ptr->saAmfCompRestartCount;

	cb->async_updt_cnt.comp_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: avd_dec_cold_sync_rsp
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
uint32_t avd_dec_cold_sync_rsp(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t num_of_obj;
	uint8_t *ptr;
	char logbuff[SA_MAX_NAME_LENGTH];

	TRACE_ENTER2("dec->i_reo_type '%u'", dec->i_reo_type);

	/* 
	 * Since at decode we need to find out how many objects of particular data
	 * type are sent, decode that information at the begining of the message.
	 */
	ptr = ncs_dec_flatten_space(&dec->i_uba, (uint8_t *)&num_of_obj, sizeof(uint32_t));
	num_of_obj = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&dec->i_uba, sizeof(uint32_t));

	/* 
	 * Decode data received in the message.
	 */
	dec->i_action = NCS_MBCSV_ACT_ADD;
	memset(logbuff, '\0', SA_MAX_NAME_LENGTH);
	snprintf(logbuff, SA_MAX_NAME_LENGTH - 1, "\n\nReceived reotype = %d num obj = %d --------------------\n",
		 dec->i_reo_type, num_of_obj);

	if (((dec->i_reo_type >= AVSV_CKPT_AVD_CB_CONFIG) && 
	     (dec->i_reo_type <= AVSV_CKPT_AVD_SI_CONFIG)) || 
	     (dec->i_reo_type == AVSV_CKPT_AVD_COMP_CONFIG) || 
	     (dec->i_reo_type == AVSV_CKPT_AVD_COMP_CS_TYPE_CONFIG)) 
	{
		/* 4.2 release onwards, no need to decode and process ADD/RMV messages 
		   for the types above, since they are received as applier callbacks. 
		   but processing this message by setting the action to UPDATE, because 
		   there might be some runtime attributes in the cold sync response */
		dec->i_action = NCS_MBCSV_ACT_UPDATE;
	}
	uint32_t status = dec_cs_data_func_list[dec->i_reo_type] (cb, dec, num_of_obj);
	TRACE_LEAVE2("status:%d",status);
	return status; 
}

/****************************************************************************\
 * Function: dec_cs_cb_config
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
static uint32_t dec_cs_cb_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_CL_CB *cb_ptr;

	TRACE_ENTER();

	cb_ptr = cb;
	/* 
	 * Send the CB data.
	 */
	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cb, &dec->i_uba,
				    EDP_OP_TYPE_DEC, (AVD_CL_CB **)&cb_ptr, &ederror, dec->i_peer_version);
	osafassert(status == NCSCC_RC_SUCCESS);

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_cs_cluster_config
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
static uint32_t dec_cs_cluster_config(AVD_CL_CB *cb,
	NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_CLUSTER dec_cluster;
	AVD_CLUSTER *cluster = &dec_cluster;

	TRACE_ENTER();

	status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_cluster,
		&dec->i_uba, EDP_OP_TYPE_DEC, (AVD_CLUSTER **)&cluster, &ederror,
		dec->i_peer_version);
	osafassert(status == NCSCC_RC_SUCCESS);
	avd_cluster->saAmfClusterAdminState = cluster->saAmfClusterAdminState;

	TRACE_LEAVE2("status '%u'", status);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dec_cs_avnd_config
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
static uint32_t dec_cs_node_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	uint32_t count = 0;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
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
		osafassert(status == NCSCC_RC_SUCCESS);
		status = avd_ckpt_node(cb, avnd_ptr, dec->i_action);
		osafassert(status == NCSCC_RC_SUCCESS);
	}

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_cs_app_config
 *
 * Purpose:  Decode entire AVD_APP data.
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
static uint32_t dec_cs_app_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	uint32_t count;
	AVD_APP dec_app;

	TRACE_ENTER();

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (count = 0; count < num_of_obj; count++) {
		decode_app(&dec->i_uba, &dec_app);
		status = avd_ckpt_app(cb, &dec_app, dec->i_action);
		if (status != NCSCC_RC_SUCCESS) {
			// perhaps this ckpt came late, after app was deleted
			LOG_WA("'%s' update failed", dec_app.name.value);
		}
	}

	TRACE_LEAVE2("status %u, count %u", status, count);
	return status;
}

/****************************************************************************\
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
static uint32_t dec_cs_sg_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	uint32_t count;
	SG_2N dec_sg;

	TRACE_ENTER();

	for (count = 0; count < num_of_obj; count++) {
		decode_sg(&dec->i_uba, &dec_sg);
		status = avd_ckpt_sg(cb, &dec_sg, dec->i_action);
		if (status != NCSCC_RC_SUCCESS) {
			// perhaps this ckpt came late, after sg was deleted
			LOG_WA("'%s' update failed", dec_sg.name.value);
		}
	}

	TRACE_LEAVE2("status %u, count %u", status, count);
	return status;
}

/****************************************************************************\
 * Function: dec_cs_su_config
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
static uint32_t dec_cs_su_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_SU su;

	TRACE_ENTER();

	for (unsigned i = 0; i < num_of_obj; i++) {
		decode_su(&dec->i_uba, &su, dec->i_peer_version);
		status = avd_ckpt_su(cb, &su, dec->i_action);
		if (status != NCSCC_RC_SUCCESS) {
			// perhaps this ckpt came late, after a su delete
			LOG_WA("'%s' update failed", su.name.value);
		}
	}

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_cs_si_config
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
static uint32_t dec_cs_si_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_SI si;

	TRACE_ENTER();

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (unsigned i = 0; i < num_of_obj; i++) {
		decode_si(&dec->i_uba, &si, dec->i_peer_version);
		status = avd_ckpt_si(cb, &si, dec->i_action);
		if (status != NCSCC_RC_SUCCESS) {
			// perhaps this ckpt came late, after a si delete
			LOG_WA("'%s' update failed", si.name.value);
		}
	}

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_cs_sg_su_oper_list
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
static uint32_t dec_cs_sg_su_oper_list(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	uint32_t cs_count = 0;

	TRACE_ENTER();

	/* 
	 * Walk through the entire list and send the entire list data.
	 */
	for (cs_count = 0; cs_count < num_of_obj; cs_count++) {
		if (NCSCC_RC_SUCCESS != dec_cs_oper_su(cb, dec))
			break;
	}

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_cs_sg_admin_si
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
static uint32_t dec_cs_sg_admin_si(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	uint32_t count = 0;

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

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/******************************************************************
 * @brief    decodes si transfer parameters during cold sync
 * @param[in] cb
 * @param[in] dec
 * @param[in] num_of_obj
 *****************************************************************/
static uint32_t dec_cs_si_trans(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVSV_SI_TRANS_CKPT_MSG *si_trans_ckpt;
	AVSV_SI_TRANS_CKPT_MSG dec_si_trans_ckpt;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	uint32_t count = 0;

	TRACE_ENTER();

	si_trans_ckpt = &dec_si_trans_ckpt;

	for (count = 0; count < num_of_obj; count++) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_si_trans,
				&dec->i_uba, EDP_OP_TYPE_DEC, (AVSV_SI_TRANS_CKPT_MSG **)&si_trans_ckpt,
				&ederror, dec->i_peer_version);
		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		}

		status = avd_ckpt_si_trans(cb, si_trans_ckpt, dec->i_action);

		if (status != NCSCC_RC_SUCCESS) {
			return NCSCC_RC_FAILURE;
		}
	}

	TRACE_LEAVE2("status '%u'", status);
	return status;

}

/****************************************************************************\
 * Function: dec_cs_siass
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
static uint32_t dec_cs_siass(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVSV_SU_SI_REL_CKPT_MSG *su_si_ckpt;
	AVSV_SU_SI_REL_CKPT_MSG dec_su_si_ckpt;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	uint32_t count = 0;

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

		status = avd_ckpt_siass(cb, su_si_ckpt, dec);

		if (status != NCSCC_RC_SUCCESS) {
			return NCSCC_RC_FAILURE;
		}
	}

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_cs_comp_config
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
static uint32_t dec_cs_comp_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVD_COMP *comp_ptr;
	AVD_COMP dec_comp;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	uint32_t count = 0;

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

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_cs_async_updt_cnt
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
static uint32_t dec_cs_async_updt_cnt(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVSV_ASYNC_UPDT_CNT *updt_cnt;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);

	TRACE_ENTER();

	updt_cnt = &cb->async_updt_cnt;
	/* 
	 * Decode and send async update counts for all the data structures.
	 */
	if (dec->i_peer_version >= AVD_MBCSV_SUB_PART_VERSION_7) {
		TRACE("Peer AMFD version is >= AVD_MBCSV_SUB_PART_VERSION_7,"
				"peer ver:%d",avd_cb->avd_peer_ver);
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_async_updt_cnt,
				&dec->i_uba, EDP_OP_TYPE_DEC, &updt_cnt, &ederror, dec->i_peer_version);
	} else {
		TRACE("Peer AMFD version is <AVD_MBCSV_SUB_PART_VERSION_7,"
				"peer version:%d", avd_cb->avd_peer_ver);
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_async_updt_cnt,
				&dec->i_uba, EDP_OP_TYPE_DEC, &updt_cnt, &ederror, dec->i_peer_version,
				13,1,2,3,4,5,6,7,8,9,10,11,12,13);
	}

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
	}

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: avd_dec_warm_sync_rsp
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
uint32_t avd_dec_warm_sync_rsp(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVSV_ASYNC_UPDT_CNT *updt_cnt;
	AVSV_ASYNC_UPDT_CNT dec_updt_cnt;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);

	TRACE_ENTER();

	updt_cnt = &dec_updt_cnt;

	/* 
	 * Decode latest async update counts. (In the same manner we received
	 * in the last message of the cold sync response.
	 */
	if (dec->i_peer_version >= AVD_MBCSV_SUB_PART_VERSION_7)
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_async_updt_cnt,
				&dec->i_uba, EDP_OP_TYPE_DEC, &updt_cnt, &ederror, dec->i_peer_version);
	else
		status = m_NCS_EDU_SEL_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_async_updt_cnt,
				&dec->i_uba, EDP_OP_TYPE_DEC, &updt_cnt, &ederror, dec->i_peer_version,
				13,1,2,3,4,5,6,7,8,9,10,11,12,13);

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
		LOG_ER("Out of sync detected in warm sync response, exiting");
		osafassert(0);

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

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: avd_dec_data_sync_rsp
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
uint32_t avd_dec_data_sync_rsp(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t num_of_obj;
	uint8_t *ptr;

	TRACE_ENTER2("%u", dec->i_reo_type);

	/* 
	 * Since at decode we need to find out how many objects of particular data
	 * type are sent, decode that information at the begining of the message.
	 */
	ptr = ncs_dec_flatten_space(&dec->i_uba, (uint8_t *)&num_of_obj, sizeof(uint32_t));
	num_of_obj = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&dec->i_uba, sizeof(uint32_t));

	/* 
	 * Decode data received in the message.
	 */
	dec->i_action = NCS_MBCSV_ACT_ADD;

	if (((dec->i_reo_type >= AVSV_CKPT_AVD_CB_CONFIG) && 
	     (dec->i_reo_type <= AVSV_CKPT_AVD_SI_CONFIG)) || 
	     (dec->i_reo_type == AVSV_CKPT_AVD_COMP_CONFIG) || 
	     (dec->i_reo_type == AVSV_CKPT_AVD_COMP_CS_TYPE_CONFIG)) 
	{
		/* 4.2 release onwards, no need to decode and process ADD/RMV messages 
		   for the types above, since they are received as applier callbacks. */
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
	return dec_cs_data_func_list[dec->i_reo_type] (cb, dec, num_of_obj);
}

/****************************************************************************\
 * Function: avd_dec_data_req
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
uint32_t avd_dec_data_req(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	TRACE_ENTER();

	/*
	 * Don't decode anything...just return success.
	 */
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dec_cs_oper_su
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
static uint32_t dec_cs_oper_su(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	uint32_t num_of_oper_su, count;
	SaNameT name;

	TRACE_ENTER();

	osaf_decode_uint32(&dec->i_uba, &num_of_oper_su);

	for (count = 0; count < num_of_oper_su; count++) {
		osaf_decode_sanamet(&dec->i_uba, &name);

		status = avd_ckpt_su_oper_list(&name, dec->i_action);
		if (status != NCSCC_RC_SUCCESS) {
			LOG_ER("%s: avd_ckpt_su_oper_list failed", __FUNCTION__);
			return status;
		}
	}

	TRACE_LEAVE2("%d", status);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dec_comp_cs_type_config
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
static uint32_t dec_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
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
		osafassert(0);
	}

	osafassert(status == NCSCC_RC_SUCCESS);
	status = avd_ckpt_compcstype(cb, comp_cs, dec->i_action);

	/* If update is successful, update async update count */
	if (NCSCC_RC_SUCCESS == status)
		cb->async_updt_cnt.compcstype_updt++;

	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/****************************************************************************\
 * Function: dec_cs_comp_cs_type_config
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
static uint32_t dec_cs_comp_cs_type_config(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec, uint32_t num_of_obj)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = static_cast<EDU_ERR>(0);
	AVD_COMPCS_TYPE dec_comp_cs;
	AVD_COMPCS_TYPE *comp_cs_ptr = &dec_comp_cs;
	uint32_t count;

	TRACE_ENTER();

	for (count = 0; count < num_of_obj; count++) {
		status = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_ckpt_msg_comp_cs_type,
					    &dec->i_uba, EDP_OP_TYPE_DEC, (AVD_COMPCS_TYPE **)&comp_cs_ptr, &ederror,
					    dec->i_peer_version);

		osafassert(status == NCSCC_RC_SUCCESS);
		status = avd_ckpt_compcstype(cb, comp_cs_ptr, dec->i_action);
		if (status != NCSCC_RC_SUCCESS) {
			// perhaps this ckpt came late, after a delete
			LOG_WA("'%s' update failed", comp_cs_ptr->name.value);
		}
	}
	TRACE_LEAVE2("status '%u'", status);
	return status;
}

/**
 * @brief   decodes saAmfNGAdminState and updates it in corresponding ng. 
 *
 * @param   ptr to AVD_CL_CB
 * @param   ptr to decode structure NCS_MBCSV_CB_DEC. 
 *
 * @return NCSCC_RC_SUCCESS 
 */
static uint32_t dec_ng_admin_state(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	SaNameT name;
	TRACE_ENTER();
	osaf_decode_sanamet(&dec->i_uba, &name);
	AVD_AMF_NG *ng = nodegroup_db->find(Amf::to_string(&name));
	if (ng == nullptr) {
		LOG_WA("Could not find %s in nodegroup_db", name.value);
		cb->async_updt_cnt.ng_updt++;
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
	osaf_decode_uint32(&dec->i_uba, (uint32_t*)&ng->saAmfNGAdminState);
	cb->async_updt_cnt.ng_updt++;

	TRACE("'%s',saAmfNGAdminState:%d",ng->name.value, ng->saAmfNGAdminState);
	ng->admin_ng_pend_cbk.invocation = 0;
	ng->admin_ng_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		AVD_SU *su = NULL;
		//If this node has any susi on it.
		for (const auto& tmp : node->list_of_su) {
			su = tmp;
			if (su->list_of_susi != NULL)
				break;
		}
		if ((ng->saAmfNGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) && (su != NULL))
			/* Still some assignments are not removed on the node.
			   if ng in SHUTTING_DOWN state and contoller role changes then
			   after all nodes of nodegroup transitions to LOCKED state, new active 
			   controller will will mark ng LOCKED.
			 */
			node->admin_ng = ng;
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_LOCKED)
			/* If controller does not change role during shutdown operation,
			   then new active controller will have to clear its admin_ng pointer.*/
			node->admin_ng = NULL;
	}
	TRACE_LEAVE2("'%s', saAmfNGAdminState=%u, ng_updt:%d",
			name.value, ng->saAmfNGAdminState, cb->async_updt_cnt.ng_updt);
	return NCSCC_RC_SUCCESS;
}

