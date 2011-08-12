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

#include <logtrace.h>
#include <avd.h>

static char *action_name[] = {
	"invalid",
	"add",
	"rmv",
	"update"
};

/****************************************************************************\
 * Function: avd_ckpt_node
 *
 * Purpose:  Add new AVND entry if action is ADD, remove node from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        avnd - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avd_ckpt_node(AVD_CL_CB *cb, AVD_AVND *ckpt_node, NCS_MBCSV_ACT_TYPE action)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_AVND *node;

	TRACE_ENTER2("%s - '%s'", action_name[action], ckpt_node->name.value);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		if (NULL == (node = avd_node_new(&ckpt_node->name))) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		avd_node_db_add(node);
		/* fall through */
	case NCS_MBCSV_ACT_UPDATE:
		if (NULL == (node = avd_node_get(&ckpt_node->name))) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		/* Update all runtime attributes */	
		node->saAmfNodeAdminState = ckpt_node->saAmfNodeAdminState;
		node->saAmfNodeOperState = ckpt_node->saAmfNodeOperState;
		node->node_state = ckpt_node->node_state;
		node->rcv_msg_id = ckpt_node->rcv_msg_id;
		node->snd_msg_id = ckpt_node->snd_msg_id;
		node->type = ckpt_node->type;
		node->node_info.member = ckpt_node->node_info.member;
		node->node_info.bootTimestamp = ckpt_node->node_info.bootTimestamp;
		node->node_info.initialViewNumber = ckpt_node->node_info.initialViewNumber;
		node->adest = ckpt_node->adest;
		node->node_info.nodeId = ckpt_node->node_info.nodeId;
		if (NULL == avd_node_find_nodeid(ckpt_node->node_info.nodeId))
			avd_node_add_nodeid(node);
		break;
	case NCS_MBCSV_ACT_RMV:
		if (NULL == (node = avd_node_get(&ckpt_node->name))) {
			LOG_ER("%s: %s does not exist", __FUNCTION__, ckpt_node->name.value);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		avd_node_delete(node);
		break;
	default:
		assert(0);
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************\
 * Function: avd_ckpt_app
 *
 * Purpose:  Add new APP entry if action is ADD, remove APP from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        ckpt_app - Decoded structure.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avd_ckpt_app(AVD_CL_CB *cb, AVD_APP *ckpt_app, NCS_MBCSV_ACT_TYPE action)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_APP *app;

	TRACE_ENTER2("%s - '%s'", action_name[action], ckpt_app->name.value);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		if (NULL == (app = avd_app_new(&ckpt_app->name))) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		avd_app_db_add(app);
		/* fall through */
	case NCS_MBCSV_ACT_UPDATE:
		if (NULL == (app = avd_app_get(&ckpt_app->name))) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		/* Update all runtime attributes */	
		app->saAmfApplicationAdminState = ckpt_app->saAmfApplicationAdminState;
		app->saAmfApplicationCurrNumSGs = ckpt_app->saAmfApplicationCurrNumSGs;
		break;
	case NCS_MBCSV_ACT_RMV:
		if (NULL == (app = avd_app_get(&ckpt_app->name))) {
			LOG_ER("%s: %s does not exist", __FUNCTION__, ckpt_app->name.value);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		avd_app_delete(app);
		break;
	default:
		assert(0);
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************\
 * Function: avd_ckpt_sg
 *
 * Purpose:  Add new SG entry if action is ADD, remove SG from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        sg - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avd_ckpt_sg(AVD_CL_CB *cb, AVD_SG *ckpt_sg, NCS_MBCSV_ACT_TYPE action)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_SG *sg;

	TRACE_ENTER2("%s - '%s'", action_name[action], ckpt_sg->name.value);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		if (NULL == (sg = avd_sg_new(&ckpt_sg->name))) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		avd_sg_db_add(sg);
		/* fall through */
	case NCS_MBCSV_ACT_UPDATE:
		if (NULL == (sg = avd_sg_get(&ckpt_sg->name))) {
			LOG_ER("avd_sg_get FAILED for '%s'", ckpt_sg->name.value);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		/* Update all runtime attributes */	
		sg->saAmfSGAdminState = ckpt_sg->saAmfSGAdminState;
		sg->saAmfSGNumCurrAssignedSUs = ckpt_sg->saAmfSGNumCurrAssignedSUs;
		sg->saAmfSGNumCurrInstantiatedSpareSUs = ckpt_sg->saAmfSGNumCurrInstantiatedSpareSUs;
		sg->saAmfSGNumCurrNonInstantiatedSpareSUs = ckpt_sg->saAmfSGNumCurrNonInstantiatedSpareSUs;
		sg->adjust_state = ckpt_sg->adjust_state;
		sg->sg_fsm_state = ckpt_sg->sg_fsm_state;
		break;
	case NCS_MBCSV_ACT_RMV: {
		if (NULL == (sg = avd_sg_get(&ckpt_sg->name))) {
			LOG_ER("%s: %s does not exist", __FUNCTION__, ckpt_sg->name.value);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		avd_sg_delete(sg);
		break;
	}
	default:
		assert(0);
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************\
 * Function: avd_ckpt_su
 *
 * Purpose:  Add new SU entry if action is ADD, remove SU from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        su - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avd_ckpt_su(AVD_CL_CB *cb, AVD_SU *ckpt_su, NCS_MBCSV_ACT_TYPE action)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_SU *su;

	TRACE_ENTER2("%s - '%s'", action_name[action], ckpt_su->name.value);

	switch (action) {
	case NCS_MBCSV_ACT_ADD: {
		SaNameT sg_name;
		if (NULL == (su = avd_su_new(&ckpt_su->name))) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		avsv_sanamet_init(&ckpt_su->name, &sg_name, "safSg=");
		su->sg_of_su = avd_sg_get(&sg_name);
		avd_su_db_add(su);
		/* fall through */
	}
	case NCS_MBCSV_ACT_UPDATE: {
		if (NULL == (su = avd_su_get(&ckpt_su->name))) {
			LOG_ER("avd_su_get FAILED for '%s'", ckpt_su->name.value);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		/* Update all runtime attributes */	
		su->saAmfSUPreInstantiable = ckpt_su->saAmfSUPreInstantiable;
		su->saAmfSUOperState = ckpt_su->saAmfSUOperState;
		su->saAmfSUAdminState = ckpt_su->saAmfSUAdminState;
		su->saAmfSuReadinessState = ckpt_su->saAmfSuReadinessState;
		su->saAmfSUPresenceState = ckpt_su->saAmfSUPresenceState;
		su->saAmfSUNumCurrActiveSIs = ckpt_su->saAmfSUNumCurrActiveSIs;
		su->saAmfSUNumCurrStandbySIs = ckpt_su->saAmfSUNumCurrStandbySIs;
		memcpy(&su->saAmfSUHostedByNode, &ckpt_su->saAmfSUHostedByNode, sizeof(SaNameT));
		su->term_state = ckpt_su->term_state;
		su->su_switch = ckpt_su->su_switch;
		su->su_act_state = ckpt_su->su_act_state;
		su->saAmfSURestartCount = ckpt_su->saAmfSURestartCount;
		break;
	}
	case NCS_MBCSV_ACT_RMV: {
		if (NULL == (su = avd_su_get(&ckpt_su->name))) {
			LOG_ER("%s: %s does not exist", __FUNCTION__, ckpt_su->name.value);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		avd_su_delete(su);
		break;
	}
	default:
		assert(0);
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************\
 * Function: avd_ckpt_si
 *
 * Purpose:  Add new SI entry if action is ADD, remove SI from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb - CB pointer.
 *        si - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 * 
\**************************************************************************/
uns32 avd_ckpt_si(AVD_CL_CB *cb, AVD_SI *ckpt_si, NCS_MBCSV_ACT_TYPE action)
{
	uns32 rc = NCSCC_RC_FAILURE;
	AVD_SI *si;

	TRACE_ENTER2("%s - '%s'", action_name[action], ckpt_si->name.value);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		if (NULL == (si = avd_si_new(&ckpt_si->name)))
			goto done;
		avd_si_db_add(si);
		/* fall through */
	case NCS_MBCSV_ACT_UPDATE:
		if (NULL == (si = avd_si_get(&ckpt_si->name))) {
			LOG_ER("avd_si_get FAILED for '%s'", ckpt_si->name.value);
			goto done;
		}
		/* Update all runtime attributes */	
		si->saAmfSINumCurrActiveAssignments = ckpt_si->saAmfSINumCurrActiveAssignments;
		si->saAmfSINumCurrStandbyAssignments = ckpt_si->saAmfSINumCurrStandbyAssignments;
		si->max_num_csi = ckpt_si->max_num_csi;
		si->si_switch = ckpt_si->si_switch;
		si->saAmfSIAdminState = ckpt_si->saAmfSIAdminState;
		si->saAmfSIAssignmentState = ckpt_si->saAmfSIAssignmentState;
		si->saAmfSIProtectedbySG = ckpt_si->saAmfSIProtectedbySG;
		si->alarm_sent = ckpt_si->alarm_sent;
		si->sg_of_si = avd_sg_get(&si->saAmfSIProtectedbySG);
		break;
	case NCS_MBCSV_ACT_RMV: {
		if (NULL == (si = avd_si_get(&ckpt_si->name))) {
			LOG_ER("%s: %s does not exist", __FUNCTION__, ckpt_si->name.value);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		avd_si_delete(si);
		break;
	}
	default:
		assert(0);
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************\
 * Function: avd_ckpt_suoperlist
 *
 * Purpose:  Add or Remove SU from SG operation list.
 *
 * Input: cb  - CB pointer.
 *        dec - Data to be decoded
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avd_ckpt_su_oper_list(AVD_CL_CB *cb, AVD_SU *ckpt_su, NCS_MBCSV_ACT_TYPE action)
{
	AVD_SU *su;

	TRACE_ENTER2("'%s'", ckpt_su->name.value);

	su = avd_su_get(&ckpt_su->name);
	assert(su);

	if (NCS_MBCSV_ACT_ADD == action)
		avd_sg_su_oper_list_add(cb, su, TRUE);
	else if (NCS_MBCSV_ACT_RMV == action)
		avd_sg_su_oper_list_del(cb, su, TRUE);
	else
		assert(0);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avd_update_sg_admin_si
 *
 * Purpose:  Update SG Admin SI.
 *
 * Input: cb  - CB pointer.
 *        dec - Data to be decoded
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avd_ckpt_sg_admin_si(AVD_CL_CB *cb, NCS_UBAID *uba, NCS_MBCSV_ACT_TYPE action)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SI *si, *si_ptr_up;
	AVD_SI dec_si;
	EDU_ERR ederror = 0;

	TRACE_ENTER2("action %u", action);

	si = &dec_si;
	/* 
	 * Check for the action type (whether it is add, rmv or update) and act
	 * accordingly. If it is add then create new element, if it is update
	 * request then just update data structure, and if it is remove then 
	 * remove entry from the list.
	 */
	status = ncs_edu_exec(&cb->edu_hdl, avsv_edp_ckpt_msg_si, uba, EDP_OP_TYPE_DEC, (AVD_SI **)&si, &ederror, 1, 1);

	if (status != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: decode failed, ederror=%u", __FUNCTION__, ederror);
		return NCSCC_RC_FAILURE;
	}

	si_ptr_up = avd_si_get(&si->name);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		si_ptr_up->sg_of_si->admin_si = si_ptr_up;
		break;
	case NCS_MBCSV_ACT_RMV:
		si_ptr_up->sg_of_si->admin_si = NULL;
		break;
	default:
		assert(0);
	}

	return status;
}

/****************************************************************************\
 * Function: avd_ckpt_siass
 *
 * Purpose:  Add new SU_SI_REL entry if action is ADD, remove SU_SI_REL from 
 *           the tree if action is to remove and update data if request is to 
 *           update.
 *
 * Input: cb  - CB pointer.
 *        si - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avd_ckpt_siass(AVD_CL_CB *cb, AVSV_SU_SI_REL_CKPT_MSG *su_si_ckpt, NCS_MBCSV_ACT_TYPE action)
{
	uns32 status = NCSCC_RC_SUCCESS;
	AVD_SU_SI_REL *su_si_rel_ptr;
	AVD_SU *su_ptr;
	AVD_SI *si_ptr_up;

	TRACE_ENTER2("'%s' '%s'", su_si_ckpt->si_name.value, su_si_ckpt->su_name.value);

	su_si_rel_ptr = avd_susi_find(cb, &su_si_ckpt->su_name, &su_si_ckpt->si_name);

	su_ptr = avd_su_get(&su_si_ckpt->su_name);
	assert(su_ptr);
	si_ptr_up = avd_si_get(&su_si_ckpt->si_name);
	assert(si_ptr_up);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		if (NULL == su_si_rel_ptr) {
			if (NCSCC_RC_SUCCESS != avd_new_assgn_susi(cb, su_ptr, si_ptr_up,
				su_si_ckpt->state, TRUE, &su_si_rel_ptr)) {
				
				LOG_ER("%s: avd_new_assgn_susi failed", __FUNCTION__);
				return NCSCC_RC_FAILURE;
			}
		}
		/* 
		 * Don't break...continue updating data of this SU SI Relation 
		 * as done during normal update request.
		 */

	case NCS_MBCSV_ACT_UPDATE:
		if (NULL != su_si_rel_ptr) {
			su_si_rel_ptr->fsm = su_si_ckpt->fsm;
			su_si_rel_ptr->state = su_si_ckpt->state;
		} else {
			LOG_ER("%s:%u", __FUNCTION__, __LINE__);
			return NCSCC_RC_FAILURE;
		}
		break;
	case NCS_MBCSV_ACT_RMV:
		if (NULL != su_si_rel_ptr) {
			avd_compcsi_delete(cb, su_si_rel_ptr, TRUE);
			avd_susi_delete(cb, su_si_rel_ptr, TRUE);
		} else {
			LOG_ER("%s: %s %s does not exist", __FUNCTION__,
				su_si_ckpt->su_name.value, su_si_ckpt->si_name.value);
			return NCSCC_RC_FAILURE;
		}
		break;
	default:
		assert(0);
	}

	return status;
}

/****************************************************************************\
 * Function: avd_ckpt_comp
 *
 * Purpose:  Add new COMP entry if action is ADD, remove COMP from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        comp - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avd_ckpt_comp(AVD_CL_CB *cb, AVD_COMP *ckpt_comp, NCS_MBCSV_ACT_TYPE action)
{
	uns32 rc = NCSCC_RC_FAILURE;
	AVD_COMP *comp;
	const SaNameT *dn = &ckpt_comp->comp_info.name;

	TRACE_ENTER2("%s - '%s'", action_name[action], dn->value);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		if (NULL == (comp = avd_comp_new(dn)))
			goto done;
		avd_comp_db_add(comp);
		/* fall through */
	case NCS_MBCSV_ACT_UPDATE:
		if (NULL == (comp = avd_comp_get(dn))) {
			LOG_ER("avd_comp_get FAILED for '%s'", dn->value);
			goto done;
		}
		comp->saAmfCompOperState = ckpt_comp->saAmfCompOperState;
		comp->saAmfCompPresenceState = ckpt_comp->saAmfCompPresenceState;
		comp->saAmfCompRestartCount = ckpt_comp->saAmfCompRestartCount;
		comp->saAmfCompReadinessState = ckpt_comp->saAmfCompReadinessState;
		/* SaNameT struct copy */
		comp->saAmfCompCurrProxyName = ckpt_comp->saAmfCompCurrProxyName;
		break;
	case NCS_MBCSV_ACT_RMV: {
		if (NULL == (comp = avd_comp_get(dn))) {
			LOG_ER("%s: %s does not exist", __FUNCTION__, dn->value);
			goto done;
		}
		avd_comp_delete(comp);
		break;
	}
	default:
		assert(0);
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************\
 * Function: avd_ckpt_compcstype
 *
 * Purpose:  Add new COMP_CS_TYPE entry if action is ADD, remove COMP_CS_TYPE 
 *           from the tree if action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        comp_cs_type - Decoded structure.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 *
\**************************************************************************/
uns32 avd_ckpt_compcstype(AVD_CL_CB *cb, AVD_COMPCS_TYPE *ckpt_compcstype, NCS_MBCSV_ACT_TYPE action)
{
	uns32 rc = NCSCC_RC_FAILURE;
	AVD_COMPCS_TYPE *ccst;
	const SaNameT *dn = &ckpt_compcstype->name;

	TRACE_ENTER2("%s - '%s'", action_name[action], dn->value);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		if (NULL == (ccst = avd_compcstype_new(dn)))
			goto done;
		avd_compcstype_db_add(ccst);
		/* fall through */
	case NCS_MBCSV_ACT_UPDATE:
		if (NULL == (ccst = avd_compcstype_get(dn))) {
			LOG_ER("avd_compcstype_get FAILED for '%s'", dn->value);
			goto done;
		}
		ccst->saAmfCompNumCurrActiveCSIs = ckpt_compcstype->saAmfCompNumCurrActiveCSIs;
		ccst->saAmfCompNumCurrStandbyCSIs = ckpt_compcstype->saAmfCompNumCurrStandbyCSIs;
		break;
	case NCS_MBCSV_ACT_RMV: {
		if (NULL == (ccst = avd_compcstype_get(dn))) {
			LOG_ER("%s: %s does not exist", __FUNCTION__, dn->value);
			goto done;
		}
		avd_compcstype_delete(&ccst);
		break;
	}
	default:
		assert(0);
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************\
 * Function: avd_data_clean_up
 *
 * Purpose:  Function is used for cleaning the entire AVD data structure.
 *           Will be called from the standby AVD on failure of warm sync.
 *
 * Input: cb  - CB pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 avd_data_clean_up(AVD_CL_CB *cb)
{
#if 0
	AVD_HLT *hlt_chk = NULL;
	AVSV_HLT_KEY hlt_name;

	AVD_SU *su;
	SaNameT su_name;
	/*AVD_SU_SI_REL *rel; */

	AVD_CSI *csi;
	SaNameT csi_name;

	AVD_COMP *comp;
	SaNameT comp_name;

	SaNameT sg_name;
	AVD_SG *sg;

	AVD_SI *si;
	SaNameT si_name;

	SaClmNodeIdT node_id = 0;
	AVD_AVND *avnd;

	AVD_SUS_PER_SI_RANK *su_si_rank = AVD_SU_PER_SI_RANK_NULL;
	AVD_SUS_PER_SI_RANK_INDX su_si_rank_indx;

	AVD_COMPCS_TYPE *comp_cs = AVD_COMP_CS_TYPE_NULL;
	AVD_COMP_CS_TYPE_INDX comp_cs_indx;
#if 0
	AVD_CS_TYPE_PARAM *cs_param = NULL;
	AVD_CS_TYPE_PARAM_INDX cs_param_indx;
#endif
	TRACE_ENTER();

	/* 
	 * Walk through the entire HLT list.
	 * Delete all HLT entries.
	 */
	memset(&hlt_name, '\0', sizeof(AVSV_HLT_KEY));
	for (hlt_chk = avd_hlt_struc_find_next(cb, hlt_name); hlt_chk != NULL;
	     hlt_chk = avd_hlt_struc_find_next(cb, hlt_name)) {
		/*
		 * Remove this HLT from the list.
		 */
		if (NCSCC_RC_SUCCESS != avd_hlt_struc_del(cb, hlt_chk))
			return NCSCC_RC_FAILURE;
	}

	/* 
	 * Walk through the entire SU list and find SU_SI_REL.
	 * Delete all SU SI relationship entries.
	 */
	su_name.length = 0;
	for (su = avd_su_struc_find_next(cb, su_name, FALSE); su != NULL;
	     su = avd_su_struc_find_next(cb, su_name, FALSE)) {
		while (su->list_of_susi != AVD_SU_SI_REL_NULL) {
			avd_compcsi_list_del(cb, su->list_of_susi, TRUE);
			avd_susi_struc_del(cb, su->list_of_susi, TRUE);
		}
		su_name = su->name;
	}

	/* 
	 * Walk through the entire CSI list
	 * Delete all CSI entries.
	 */
	csi_name.length = 0;
	for (csi = avd_csi_struc_find_next(cb, csi_name, FALSE); csi != NULL;
	     csi = avd_csi_struc_find_next(cb, csi_name, FALSE)) {
		csi_name = csi->name;
		avd_remove_csi(cb, csi);
	}

	/* 
	 * Walk through the entire COMP list.
	 * Delete all component entries.
	 */
	comp_name.length = 0;
	for (comp = avd_comp_struc_find_next(cb, comp_name, FALSE); comp != NULL;
	     comp = avd_comp_struc_find_next(cb, comp_name, FALSE)) {
		comp_name = comp->comp_info.name;
		/* remove the COMP from SU list.
		 */

		if (comp->su)
			comp->su->curr_num_comp--;

		avd_comp_del_su_list(cb, comp);

		/*
		 * Remove this COMP from the list.
		 */
		if (NCSCC_RC_SUCCESS != avd_comp_struc_del(cb, comp))
			return NCSCC_RC_FAILURE;
	}

	/*
	 * Delete entire operation SU's.
	 */
	su_name.length = 0;
	for (su = avd_su_struc_find_next(cb, su_name, FALSE); su != NULL;
	     su = avd_su_struc_find_next(cb, su_name, FALSE)) {
		/* HACK FOR TESTING SHULD BE CHANGED LATER */
		if (su->sg_of_su == 0) {
			su_name = su->name;
			continue;
		}

		if (su->sg_of_su->su_oper_list.su != NULL)
			avd_sg_su_oper_list_del(cb, su, TRUE);

		su_name = su->name;
	}

	/* 
	 * Walk through the entire SI list
	 * Delete all SI entries.
	 */
	si_name.length = 0;
	for (si = avd_si_struc_find_next(cb, si_name, FALSE); si != NULL;
	     si = avd_si_struc_find_next(cb, si_name, FALSE)) {
		si_name = si->name;
		/* remove the SI from the SG list.
		 */
		avd_si_del_sg_list(cb, si);

		/*
		 * Remove this SI from the list.
		 */
		avd_si_struc_del(si, DB_REMOVE_FROM);
	}

	/* 
	 * Walk through the entire SU list..
	 * Delete all SU entries.
	 */
	su_name.length = 0;
	for (su = avd_su_struc_find_next(cb, su_name, FALSE); su != NULL;
	     su = avd_su_struc_find_next(cb, su_name, FALSE)) {
		su_name = su->name;
		/* remove the SU from both the SG list and the
		 * AVND list if present.
		 */
		avd_su_del_sg_list(cb, su);
		avd_su_del_avnd_list(cb, su);

		/*
		 * Remove this SU from the list.
		 */
		avd_su_delete(su);
	}

	/* 
	 * Walk through the entire SG list
	 * Delete all the SG entries.
	 */
	sg_name.length = 0;
	for (sg = avd_sg_struc_find_next(cb, sg_name, FALSE);
	     sg != NULL; sg = avd_sg_struc_find_next(cb, sg_name, FALSE)) {
		sg_name = sg->name;
		/*
		 * Remove this SG from the list.
		 */
		avd_sg_struc_del(sg, SA_TRUE);
	}

	/* 
	 * Walk through the entire AVND list.
	 * Delete all the AVND entries.
	 */
	node_id = 0;
	while (NULL != (avnd = avd_avnd_struc_find_next_nodeid(node_id))) {
		/*
		 * Remove this AVND from the list.
		 */
		if (NCSCC_RC_SUCCESS != avd_avnd_struc_rmv_nodeid(avnd))
			return NCSCC_RC_FAILURE;

		if (NCSCC_RC_SUCCESS != avd_avnd_struc_del(avnd))
			return NCSCC_RC_FAILURE;
	}

	/*
	 * Walk through the entire SUS_PER_SI_RANK list.
	 * Delete all entries.
	 */
	memset(&su_si_rank_indx, '\0', sizeof(AVD_SUS_PER_SI_RANK_INDX));
	for (su_si_rank = avd_sus_per_si_rank_struc_find_next(cb, su_si_rank_indx); su_si_rank != NULL;
	     su_si_rank = avd_sus_per_si_rank_struc_find_next(cb, su_si_rank_indx)) {
		/*
		 * Remove this entry from the list.
		 */
		if (NCSCC_RC_SUCCESS != avd_sus_per_si_rank_struc_del(cb, su_si_rank))
			return NCSCC_RC_FAILURE;
	}

	/*
	 * Walk through the entire COMP_CS_TYPE list.
	 * Delete all entries.
	 */
	memset(&comp_cs_indx, '\0', sizeof(AVD_COMP_CS_TYPE_INDX));
	for (comp_cs = avd_compcstype_struc_find_next(cb, comp_cs_indx); comp_cs != NULL;
	     comp_cs = avd_compcstype_struc_find_next(cb, comp_cs_indx)) {
		/*
		 * Remove this entry from the list.
		 */
		avd_compcstype_struc_del(comp_cs);
	}

	/*
	 * Walk through the entire CS_TYPE_PARAM list.
	 * Delete all entries.
	 */
#if 0
	/*  TODO */
	memset(&cs_param_indx, '\0', sizeof(AVD_CS_TYPE_PARAM_INDX));
	for (cs_param = avd_cs_type_param_struc_find_next(cb, cs_param_indx); cs_param != NULL;
	     cs_param = avd_cs_type_param_struc_find_next(cb, cs_param_indx)) {
		/*
		 * Remove this entry from the list.
		 */
		if (NCSCC_RC_SUCCESS != avd_cs_type_param_struc_del(cb, cs_param))
			return NCSCC_RC_FAILURE;
	}
#endif

#endif
	/* Reset the cold sync done counter */
	cb->synced_reo_type = 0;
	return NCSCC_RC_SUCCESS;
}

