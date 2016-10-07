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

  This file contains routines for SU operation.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include <logtrace.h>
#include <avnd.h>
#include <immutil.h>

static uint32_t avnd_avd_su_update_on_fover(AVND_CB *cb, AVSV_D2N_REG_SU_MSG_INFO *info);

/**
 * Return SU failover read from IMM
 *
 * @param name
 * @return
 */
static bool get_su_failover(const SaNameT *name)
{
	SaImmAccessorHandleT accessorHandle;
	const SaImmAttrValuesT_2 **attributes;
	SaImmHandleT immOmHandle;
	SaVersionT immVersion = { 'A', 2, 1 };
	SaNameT sutype;
	SaBoolT sufailover = SA_FALSE;
	SaImmAttrNameT attributeNames[] = {
		const_cast<SaImmAttrNameT>("saAmfSUFailover"),
		const_cast<SaImmAttrNameT>("saAmfSUType"),
		nullptr
	};
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", name->value);

	// TODO remove, just for test
	LOG_NO("get_su_failover '%s'", name->value);

	error = saImmOmInitialize_cond(&immOmHandle, nullptr, &immVersion);
	if (error != SA_AIS_OK ) {
		LOG_CR("saImmOmInitialize failed: %u", error);
		goto done1;
	}
	amf_saImmOmAccessorInitialize(immOmHandle, accessorHandle);

	/* Use an attribute name list to avoid reading runtime attributes which
	 * causes callbacks executed in AMF director. */
	if (amf_saImmOmAccessorGet_2(immOmHandle, accessorHandle, name, attributeNames,
		(SaImmAttrValuesT_2 ***)&attributes) != SA_AIS_OK) {
		LOG_ER("amf_saImmOmAccessorGet_2 FAILED for '%s'", name->value);
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUFailover"),
			attributes, 0, &sufailover) != SA_AIS_OK) {
		/* nothing specified in SU, read type */
		if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUType"),
				attributes, 0, &sutype) == SA_AIS_OK) {

			attributeNames[0] = const_cast<SaImmAttrNameT>("saAmfSutDefSUFailover");
			attributeNames[1] = nullptr;
			if (amf_saImmOmAccessorGet_2(immOmHandle, accessorHandle, &sutype, nullptr,
					(SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
				immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutDefSUFailover"),
					attributes, 0, &sufailover);
			}
		}
	}

done:
	immutil_saImmOmAccessorFinalize(accessorHandle);
	immutil_saImmOmFinalize(immOmHandle);
done1:
	TRACE_LEAVE2();
	return (sufailover == SA_TRUE) ? true : false;
}

/****************************************************************************
  Name          : avnd_evt_avd_reg_su_msg
 
  Description   : This routine processes the SU addition message from AvD. SU
                  deletion is handled as a part of operation request message 
                  processing.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_avd_reg_su_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_REG_SU_MSG_INFO *info = 0;
	AVSV_SU_INFO_MSG *su_info = 0;
	AVND_SU *su = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* dont process unless AvD is up */
	if (!m_AVND_CB_IS_AVD_UP(cb))
		goto done;

	info = &evt->info.avd->msg_info.d2n_reg_su;

	avnd_msgid_assert(info->msg_id);
	cb->rcv_msg_id = info->msg_id;

	/* 
	 * Check whether SU updates are received after fail-over then
	 * call a separate processing function.
	 */
	if (info->msg_on_fover) {
		rc = avnd_avd_su_update_on_fover(cb, info);
		goto done;
	}

	/* scan the su list & add each su to su-db */
	for (su_info = info->su_list; su_info; su = 0, su_info = su_info->next) {
		su = m_AVND_SUDB_REC_GET(cb->sudb, su_info->name);
		/* This function is common
		   1. for adding new SU in the data base
		   2. for adding a new component in the existing su.
		   So, check whether the SU exists or not. */
		if (su == nullptr)
			su = avnd_sudb_rec_add(cb, su_info, &rc);

		/* su_failover included in message version 5 and higher */
		if (evt->msg_fmt_ver < 5) {
			su->sufailover = get_su_failover(&su->name);
		}

		/* add components belonging to this SU */
		if (avnd_comp_config_get_su(su) != NCSCC_RC_SUCCESS) {
			m_AVND_SU_REG_FAILED_SET(su);
			/* Will transition to instantiation-failed when instantiated */
			LOG_ER("%s: FAILED", __FUNCTION__);
			rc = NCSCC_RC_FAILURE;
			break;
		}

		/* When NPI comp is added into PI SU(that is UNINSTANTIATED),
		   we don't have to run SU FSM as anyway, we are not
		   instantiating NPI component. In this case, anyway, SU will
		   remain in instantiated state. NPI comp will get instantiated
		   when corresponding csi is added. */
		bool su_is_instantiated;
		m_AVND_SU_IS_INSTANTIATED(su, su_is_instantiated);

		if ((su->pres == SA_AMF_PRESENCE_INSTANTIATED) &&
				(su_is_instantiated == false)) {
			avnd_su_pres_state_set(cb, su, SA_AMF_PRESENCE_UNINSTANTIATED);
			rc = avnd_su_pres_fsm_run(cb, su, 0, AVND_SU_PRES_FSM_EV_INST);
		}
	}

	/*** send the response to AvD ***/
	rc = avnd_di_reg_su_rsp_snd(cb, &info->su_list->name, rc);


done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_avd_su_update_on_fover
 
  Description   : This routine processes the SU update message sent by AVD 
                  on fail-over.
 
  Arguments     : cb  - ptr to the AvND control block
                  info - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t avnd_avd_su_update_on_fover(AVND_CB *cb, AVSV_D2N_REG_SU_MSG_INFO *info)
{
	AVSV_SU_INFO_MSG *su_info = 0;
	AVND_SU *su = 0;
	AVND_COMP *comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaNameT su_name;

	TRACE_ENTER();

	/* scan the su list & add each su to su-db */
	for (su_info = info->su_list; su_info; su = 0, su_info = su_info->next) {
		if (nullptr == (su = m_AVND_SUDB_REC_GET(cb->sudb, su_info->name))) {
			/* SU is not present so add it */
			su = avnd_sudb_rec_add(cb, su_info, &rc);
			if (!su) {
				avnd_di_reg_su_rsp_snd(cb, &su_info->name, rc);
				/* Log Error, we are not able to update at this time */
				LOG_EM("%s, %u, SU update failed",__FUNCTION__,__LINE__);
				return rc;
			}

			avnd_di_reg_su_rsp_snd(cb, &su_info->name, rc);
		} else {
			/* SU present, so update its contents */
			/* update error recovery escalation parameters */
			su->comp_restart_prob = su_info->comp_restart_prob;
			su->comp_restart_max = su_info->comp_restart_max;
			su->su_restart_prob = su_info->su_restart_prob;
			su->su_restart_max = su_info->su_restart_max;
			su->is_ncs = su_info->is_ncs;
		}

		su->avd_updt_flag = true;
	}

	/*
	 * Walk through the entire SU table, and remove SU for which 
	 * updates are not received in the message.
	 */
	memset(&su_name, 0, sizeof(SaNameT));
	while (nullptr != (su = (AVND_SU *)sudb_rec_get_next(&cb->sudb, (uint8_t *)&su_name))) {
		su_name = su->name;

		if (false == su->avd_updt_flag) {
			/* First walk entire comp list of this SU and delete all the
			 * component records which are there in the list.
			 */
			while ((comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list)))) {
				/* delete the record */
				rc = avnd_compdb_rec_del(cb, &comp->name);
				if (NCSCC_RC_SUCCESS != rc) {
					/* Log error */
					LOG_EM("%s, %u, SU update failed",__FUNCTION__,__LINE__);
					goto err;
				}
			}

			/* Delete SU from the list */
			/* delete the record */
			rc = avnd_sudb_rec_del(cb, &su->name);
			if (NCSCC_RC_SUCCESS != rc) {
				/* Log error */
				LOG_EM("%s, %u, SU update failed",__FUNCTION__,__LINE__);
				goto err;
			}

		} else
			su->avd_updt_flag = false;
	}

 err:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static void handle_su_si_assign_in_term_state(AVND_CB *cb,
		const AVND_SU *su,
		const AVSV_D2N_INFO_SU_SI_ASSIGN_MSG_INFO *info)
{
	AVND_MSG msg;

	assert(cb->term_state == AVND_TERM_STATE_NODE_FAILOVER_TERMINATED);

	msg.info.avd = new AVSV_DND_MSG();

	msg.type = AVND_MSG_AVD;
	msg.info.avd->msg_type = AVSV_N2D_INFO_SU_SI_ASSIGN_MSG;
	msg.info.avd->msg_info.n2d_su_si_assign.msg_id = ++(cb->snd_msg_id);
	msg.info.avd->msg_info.n2d_su_si_assign.node_id = cb->node_info.nodeId;
	msg.info.avd->msg_info.n2d_su_si_assign.msg_act = info->msg_act;
	msg.info.avd->msg_info.n2d_su_si_assign.su_name = info->su_name;
	msg.info.avd->msg_info.n2d_su_si_assign.si_name = info->si_name;
	msg.info.avd->msg_info.n2d_su_si_assign.ha_state = info->ha_state;
	/* Fake a SUCCESS response in fail-over state */
	msg.info.avd->msg_info.n2d_su_si_assign.error = NCSCC_RC_SUCCESS;
	msg.info.avd->msg_info.n2d_su_si_assign.single_csi = info->single_csi;

	if (avnd_di_msg_send(cb, &msg) != NCSCC_RC_SUCCESS) {
		LOG_NO("Failed to send SU_SI_ASSIGN response");
	}
}

/**
 * Return SI rank read from IMM
 *
 * @param dn DN of SI
 *
 * @return      rank of SI or -1 if not configured for SI
 */
static uint32_t get_sirank(const SaNameT *dn)
{
	SaAisErrorT error;
	SaImmAccessorHandleT accessorHandle;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAttrNameT attributeNames[2] = {const_cast<SaImmAttrNameT>("saAmfSIRank"), nullptr};
	SaImmHandleT immOmHandle;
	SaVersionT immVersion = {'A', 2, 1};
	uint32_t rank = -1; // lowest possible rank if uninitialized

	// TODO remove, just for test
	LOG_NO("get_sirank %s", dn->value);

	error = saImmOmInitialize_cond(&immOmHandle, nullptr, &immVersion);
	if (error != SA_AIS_OK ) {
		LOG_CR("saImmOmInitialize failed: %u", error);
		goto done;
	}
	amf_saImmOmAccessorInitialize(immOmHandle, accessorHandle);

	osafassert((error = amf_saImmOmAccessorGet_2(immOmHandle, accessorHandle, dn,
		attributeNames, (SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK);

	osafassert((error = immutil_getAttr(attributeNames[0], attributes, 0, &rank)) == SA_AIS_OK);

	// saAmfSIRank attribute has a default value of zero (returned by IMM)
	if (rank == 0) {
		// Unconfigured ranks are treated as lowest possible rank
		rank = -1;
	}

	immutil_saImmOmAccessorFinalize(accessorHandle);
	immutil_saImmOmFinalize(immOmHandle);

done:
	return rank;
}

/****************************************************************************
  Name          : avnd_evt_avd_info_su_si_assign_msg
 
  Description   : This routine processes the SU-SI assignment message from 
                  AvD. It buffers the message if already some assignment is on.
                  Else it initiates SI addition, deletion or removal.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_avd_info_su_si_assign_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_INFO_SU_SI_ASSIGN_MSG_INFO *info = &evt->info.avd->msg_info.d2n_su_si_assign;
	AVND_SU_SIQ_REC *siq = 0;
	AVND_SU *su = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("'%s'", info->su_name.value);

	su = m_AVND_SUDB_REC_GET(cb->sudb, info->su_name);
	if (!su) {
		LOG_ER("susi_assign_evh: '%s' not found, action:%u",
				info->su_name.value, info->msg_act);
		goto done;
	}

	if ((cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN_INITIATED) ||
			(cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED)) {
		if ((su->is_ncs == true) &&
			(info->msg_act == AVSV_SUSI_ACT_MOD) && 
				(info->ha_state == SA_AMF_HA_ACTIVE)) {
			LOG_NO("shutdown started, failover requested, escalate to forced shutdown");
			avnd_last_step_clean(cb);
		} else {
			LOG_NO("Shutting started : Ignoring assignment for SU'%s'",
						info->su_name.value);
			goto done;
		}
	}

	avnd_msgid_assert(info->msg_id);
	cb->rcv_msg_id = info->msg_id;

	if (info->msg_act == AVSV_SUSI_ACT_ASGN) {
		/* SI rank and CSI capability (originally from SaAmfCtCsType)
		 * was introduced in version 5 of the node director supported protocol.
		 * If the protocol is older, take action */
		if (evt->msg_fmt_ver < 5) {
			AVSV_SUSI_ASGN *csi;

			/* indicate that capability is invalid for later use when
			 * creating CSI_REC */
			for (csi = info->list; csi != nullptr; csi = csi->next) {
				csi->capability = (SaAmfCompCapabilityModelT) ~0;
			}

			/* SI rank is uninitialized, read it from IMM */
 			info->si_rank = get_sirank(&info->si_name);
		}
	} else {
		if (info->si_name.length > 0) {
			if (avnd_su_si_rec_get(cb, &info->su_name, &info->si_name) == nullptr)
				LOG_ER("susi_assign_evh: '%s' is not assigned to '%s'",
						info->si_name.value, su->name.value);
		} else {
			if (m_NCS_DBLIST_FIND_FIRST(&su->si_list) == nullptr) {
				LOG_ER("susi_assign_evh: '%s' has no assignments", su->name.value);
				/* Some times AMFD sends redundant message for removal of assignments.
				   If removal of assignments is already done for the SU then complete
				   the assignment process here.
				 */
				goto done;
			}
		}
		/*
		   SU failover and Node-switchover (with sufailover true) is in progress
		   and AMFND gets deletion of assignment for failed SU. Since AMFND launches 
		   cleanup of all the components failed SU, it must discard deletion of assignment 
		   in it. After successful cleanup of all the components, AMFND sends recovery
		   request to AMFD and it will take care of failover of this failed SU. Also AMFD 
		   will be able to respond to any pending admin op while processing recovery request.
		   For Node-failover, assignment can be discarded for any SU as AMFND launches clean up 
		   of all the components.
		 */
		if ((sufailover_in_progress(su) || sufailover_during_nodeswitchover(su) ||
			(cb->term_state == AVND_TERM_STATE_NODE_FAILOVER_TERMINATING)) &&
				(info->msg_act == AVSV_SUSI_ACT_DEL)) {
			TRACE_2("Discarding assignment deletion for '%s'", su->name.value);
			goto done;
		}
	}

	if (cb->term_state == AVND_TERM_STATE_NODE_FAILOVER_TERMINATED) {
		handle_su_si_assign_in_term_state(cb, su, info);
	} else {
		/* buffer the msg (if no assignment / removal is on) */
		siq = avnd_su_siq_rec_buf(cb, su, info);
		if (siq == nullptr) {
			/* the msg isn't buffered, process it */
			rc = avnd_su_si_msg_prc(cb, su, info);
		}
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_tmr_su_err_esc
 
  Description   : This routine handles the the expiry of the 'su error 
                  escalation' timer. It indicates the end of the comp/su 
                  restart probation period for the SU.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_tmr_su_err_esc_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_SU *su;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* retrieve avnd cb */
	if (0 == (su = (AVND_SU *)ncshm_take_hdl(NCS_SERVICE_ID_AVND, (uint32_t)evt->info.tmr.opq_hdl))) {
		LOG_CR("Unable to retrieve handle");
		goto done;
	}

	TRACE("'%s'", su->name.value);
	
	LOG_NO("'%s' Component or SU restart probation timer expired", su->name.value);

	if (NCSCC_RC_SUCCESS == m_AVND_CHECK_FOR_STDBY_FOR_EXT_COMP(cb, su->su_is_external))
		goto done;

	switch (su->su_err_esc_level) {
	case AVND_ERR_ESC_LEVEL_0:
		su->comp_restart_cnt = 0;
		su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		su_reset_restart_count_in_comps(cb, su);
		break;
	case AVND_ERR_ESC_LEVEL_1:
		su->su_restart_cnt = 0;
		su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		cb->node_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		su_reset_restart_count_in_comps(cb, su);
		avnd_di_uns32_upd_send(AVSV_SA_AMF_SU, saAmfSURestartCount_ID, &su->name, su->su_restart_cnt);
		break;
	case AVND_ERR_ESC_LEVEL_2:
		cb->su_failover_cnt = 0;
		su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		cb->node_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		break;
	default:
		osafassert(0);
	}

done:
	if (su)
		ncshm_give_hdl((uint32_t)evt->info.tmr.opq_hdl);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_reassign
 
  Description   : This routine reassigns all the SIs in the su-si list. It is
                  invoked when the SU reinstantiates as a part of SU restart
                  recovery.
 
  Arguments     : cb - ptr to the AvND control block
                  su - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_si_reassign(AVND_CB *cb, AVND_SU *su)
{
	AVND_SU_SI_REC *si = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su->name.value);

	/* scan the su-si list & reassign the sis */
	for (si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	     si; si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&si->su_dll_node)) {
		rc = avnd_su_si_assign(cb, su, si);
		if (NCSCC_RC_SUCCESS != rc)
			break;
	}			/* for */

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_curr_info_del
 
  Description   : This routine deletes the dynamic info associated with this 
                  SU. This includes deleting the dynamic info for all it's 
                  components. If the SU is marked failed, the error 
                  escalation parameters are retained.
 
  Arguments     : cb - ptr to the AvND control block
                  su - ptr to the su
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : SIs associated with this SU are not deleted.
******************************************************************************/
uint32_t avnd_su_curr_info_del(AVND_CB *cb, AVND_SU *su)
{
	AVND_COMP *comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su->name.value);

	/* reset err-esc param & oper state (if su is healthy) */
	if (!m_AVND_SU_IS_FAILED(su)) {
		su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		su->comp_restart_cnt = 0;
		su_reset_restart_count_in_comps(cb, su);
		su->su_restart_cnt = 0;
		avnd_di_uns32_upd_send(AVSV_SA_AMF_SU, saAmfSURestartCount_ID, &su->name, su->su_restart_cnt);
		/* stop su_err_esc_tmr TBD Later */
	}

	/* scan & delete the current info store in each component */
	for (comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
	     comp; comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node))) {
		rc = avnd_comp_curr_info_del(cb, comp);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static bool comp_in_term_failed_state(void)
{
	AVND_COMP *comp =
		(AVND_COMP *)compdb_rec_get_next(&avnd_cb->compdb, (uint8_t *)0);

	while (comp != nullptr) {
		if (comp->pres == SA_AMF_PRESENCE_TERMINATION_FAILED)
			return true;

		comp = (AVND_COMP *)
		    compdb_rec_get_next(&avnd_cb->compdb, (uint8_t *)&comp->name);
	}

	return false;
}

/**
 * Process SU admin operation request from director
 *
 * @param cb
 * @param evt
 */
uint32_t avnd_evt_su_admin_op_req(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_ADMIN_OP_REQ_MSG_INFO *info = &evt->info.avd->msg_info.d2n_admin_op_req_info;
	AVND_SU *su;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s op=%u", info->dn.value, info->oper_id);

	avnd_msgid_assert(info->msg_id);
	cb->rcv_msg_id = info->msg_id;

	su = m_AVND_SUDB_REC_GET(cb->sudb, info->dn);
	osafassert(su != nullptr);

	switch(info->oper_id) {
	case SA_AMF_ADMIN_REPAIRED: {
		AVND_COMP *comp;

		/* SU has been repaired. Reset states and update AMF director accordingly. */
		LOG_NO("Repair request for '%s'", su->name.value);

		for (comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		      comp;
		      comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node))) {

			comp->admin_oper = false;
			m_AVND_COMP_STATE_RESET(comp);
			avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_UNINSTANTIATED);
			
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
			avnd_di_uns32_upd_send(AVSV_SA_AMF_COMP, saAmfCompOperState_ID, &comp->name, comp->oper);
		}

		if ((su->pres == SA_AMF_PRESENCE_TERMINATION_FAILED) &&
				(comp_in_term_failed_state() == false))
			avnd_failed_state_file_delete();

		su->admin_op_Id = static_cast<SaAmfAdminOperationIdT>(0);
		reset_suRestart_flag(su);
		m_AVND_SU_STATE_RESET(su);
		m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_ENABLED);
		avnd_di_uns32_upd_send(AVSV_SA_AMF_SU, saAmfSUOperState_ID, &su->name, su->oper);
		avnd_su_pres_state_set(cb, su, SA_AMF_PRESENCE_UNINSTANTIATED);
		rc = avnd_di_oper_send(cb, su, 0);

		break;
	}
	case SA_AMF_ADMIN_RESTART: {
		LOG_NO("Admin Restart request for '%s'", su->name.value);
		su->admin_op_Id = SA_AMF_ADMIN_RESTART;
		set_suRestart_flag(su);
		if ((su_all_comps_restartable(*su) == true) || 
				(is_any_non_restartable_comp_assigned(*su) == false)) { 
			rc = avnd_su_curr_info_del(cb, su);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			rc = avnd_su_si_unmark(cb, su);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
		rc = avnd_su_pres_fsm_run(cb, su, 0, AVND_SU_PRES_FSM_EV_RESTART);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
		break;
	}
	default:
		LOG_NO("%s: unsupported adm op %u", __FUNCTION__, info->oper_id);
		rc = NCSCC_RC_FAILURE;
		break;
	}
done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Set the new SU presence state to 'newState'. Update AMF director. Checkpoint.
 * Syslog at level NOTICE.
 * @param su
 * @param newstate
 */
void avnd_su_pres_state_set(const AVND_CB *cb, AVND_SU *su, SaAmfPresenceStateT newstate)
{
	osafassert(newstate <= SA_AMF_PRESENCE_TERMINATION_FAILED);
	LOG_NO("'%s' Presence State %s => %s", su->name.value,
		presence_state[su->pres], presence_state[newstate]);
	su->pres = newstate;
	if (cb->is_avd_down == false) {
		avnd_di_uns32_upd_send(AVSV_SA_AMF_SU, saAmfSUPresenceState_ID, &su->name, su->pres);
	}
}

/**
 * @brief Resets component restart count for each component of SU. 
 * @param su
 */
void su_reset_restart_count_in_comps(const AVND_CB *cb, const AVND_SU *su)
{
	AVND_COMP *comp;
	for (comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		comp;
		comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node))) {
		comp_reset_restart_count(cb, comp);
	}

}

void su_increment_su_restart_count(AVND_SU& su)
{
	su.su_restart_cnt++;
	LOG_NO("Restarting '%s' (SU restart count: %u)",
		su.name.value, su.su_restart_cnt);	
}

void su_increment_comp_restart_count(AVND_SU& su)
{
	su.comp_restart_cnt++;
	LOG_NO("Restarting a component of '%s' (comp restart count: %u)",
		su.name.value, su.comp_restart_cnt);	
}

void cb_increment_su_failover_count(AVND_CB& cb, const AVND_SU& su)
{
	cb.su_failover_cnt++;
	LOG_NO("Performing failover of '%s' (SU failover count: %u)",
		su.name.value, cb.su_failover_cnt);	
}
/**
 * @brief A wrapper function on top of macro to 
 *	  set suRestart flag in SU. 
 * @param ptr to su
 */
void set_suRestart_flag(AVND_SU *su) 
{
	TRACE("suRestart flag set for '%s'",su->name.value);
	m_AVND_SU_RESTART_SET(su);
	
}

/**
 * @brief A wrapper function on top of macro to 
 *	  reset suRestart flag in SU. 
 * @param ptr to su
 */
void reset_suRestart_flag(AVND_SU *su) 
{
	TRACE("suRestart flag reset for '%s'",su->name.value);
	m_AVND_SU_RESTART_RESET(su);
}

/**
 * @brief Checks if SU consists of only restartable component. 
 *        For a restartable component saAmfCompDisableRestart=0.
 * @param su reference
 * @return true/false.
 */
bool su_all_comps_restartable(const AVND_SU& su)
{
	for (AVND_COMP *comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su.comp_list));
		comp;
		comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node))) {
		if (m_AVND_COMP_IS_RESTART_DIS(comp))
			return false;
	}
	return true;
}

/**
 * @brief Request AMFD to perform surestart recovery.
 *        It will send a su_oper_state enabled evh to AMFD with
 *	  recovery set to AVSV_ERR_RCVR_SU_RESTART.
 * @param ptr to su
 */
void su_send_suRestart_recovery_msg(AVND_SU *su)
{
	su->oper = SA_AMF_OPERATIONAL_ENABLED; 	
	//Keep the su enabled for sending the message.
	avnd_di_oper_send(avnd_cb, su, AVSV_ERR_RCVR_SU_RESTART);
	su->oper = SA_AMF_OPERATIONAL_DISABLED;	
}

/**
 * @brief For a PI SU it checks if all the comps are in uninstantiated state.
 *        For a PI SU, presence state is deduced from PI comps only. 
 * @param su reference
 * @return true/false.
 */
bool pi_su_all_comps_uninstantiated (const AVND_SU& su)
{
	for (AVND_COMP *comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su.comp_list));
			comp;
			comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node))) {
		if ((comp->pres != SA_AMF_PRESENCE_UNINSTANTIATED) &&
				(m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)))
			return false;
	}
	return true;
}

/**
 * @brief Checks if any non-restartable comp ins SU is assigned..
 *        For a non restartable comp  saAmfCompDisableRestart=1. 
 * @param su reference
 * @return true/false.
 */
bool is_any_non_restartable_comp_assigned(const AVND_SU& su)
{
	for (AVND_COMP *comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su.comp_list));
			comp;
			comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node))) {
                if ((m_AVND_COMP_IS_RESTART_DIS(comp)) &&
                                (comp->csi_list.n_nodes > 0))
                        return true;
        }
        return false;
}

/**
 * @brief  Checks if all PI comps of SU are in INSTANTIATED state.
 * @return true/false
 */
bool su_all_pi_comps_instantiated(const AVND_SU *su) 
{
	bool su_is_instantiated;
	m_AVND_SU_IS_INSTANTIATED(su, su_is_instantiated);
	TRACE("All PI comps instantiated :'%u'",su_is_instantiated);
	return su_is_instantiated;
}
/**
 * @brief Checks if RESTART admin op is going on SU.
 * @return true/false
 */
bool isAdminRestarted(const AVND_SU *su) 
{ 
	return (su->admin_op_Id == SA_AMF_ADMIN_RESTART);
}

/**
 * @brief  Checks if SU is marked failed.
 * @return true/false
 */
bool isFailed(const AVND_SU *su)
{
	return (m_AVND_SU_IS_FAILED(su));
}

/**
 * @brief  Checks if SU is marked restarting because of 
 *		su restart recovery or RESTART admin op.
 * @return true/false
 */
bool isRestartSet(const AVND_SU *su)
{
	return (m_AVND_SU_IS_RESTART(su));
}
/*
 * @brief  Checks if all SIs of SU have a given prev_assign_state.
 * @return true/false
 */
bool AVND_SU::avnd_su_check_sis_previous_assign_state(const AVND_SU_SI_ASSIGN_STATE prv_assign_state) const {
  for (AVND_SU_SI_REC *si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&this->si_list);
    si; si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&si->su_dll_node)) {
    if ((prv_assign_state == AVND_SU_SI_ASSIGN_STATE_UNASSIGNED) &&
      (!m_AVND_SU_SI_PRV_ASSIGN_STATE_IS_UNASSIGNED(si)))
      return false;
    else if ((prv_assign_state == AVND_SU_SI_ASSIGN_STATE_ASSIGNED) &&
      (!m_AVND_SU_SI_PRV_ASSIGN_STATE_IS_ASSIGNED(si)))
      return false;
  }
  return true;
}

