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

  DESCRIPTION: This file contains the SG processing routines for the AVD.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <immutil.h>
#include <logtrace.h>
#include <set>

#include <amfd.h>
#include <imm.h>
#include <su.h>
#include <clm.h>
#include <si_dep.h>


/**
 * @brief       While creating compcsi relationship in SUSI, AMF may assign
 * 		a dependent csi to a component in the SU when its one or more 
 *              sponsor csi remains unassigned in the same SU. This function 
 *              deletes compcsi which belongs to such a dependent csi.
 *              This function should be used before sending assignment list to AMFND.
 *
 * @param[in]   ptr to AVD_SU_SI_REL.
 *
 */

static void verify_csi_deps_and_delete_invalid_compcsi(AVD_SU_SI_REL *susi)	
{
	for (AVD_CSI *csi = susi->si->list_of_csi; csi; csi = csi->si_list_of_csi_next)
	{
		if (are_sponsor_csis_assigned_in_su(csi,susi->su) == false) {
			for (AVD_COMP_CSI_REL *compcsi = susi->list_of_csicomp; compcsi;
					compcsi = compcsi->susi_csicomp_next) {
				if (compcsi->csi == csi) {
					TRACE("sponsor csi unassigned: delete compcsi "
							"between '%s' and '%s'",
							compcsi->comp->comp_info.name.value,
							compcsi->csi->name.value);
					compcsi->csi->assign_flag = false;
					compcsi->comp->assign_flag = false;
					avd_compcsi_from_csi_and_susi_delete(susi, compcsi, true);
					break;
				}
			}
		}
	}
}
/*****************************************************************************
 * Function: avd_new_assgn_susi
 *
 * Purpose:  This function creates and assigns the given role to the SUSI
 * relationship and sends the message to the AVND having the
 * SU accordingly. 
 *
 * Input: cb - the AVD control block
 *        su - The pointer to SU.
 *        si - The pointer to SI.
 *      ha_state - The HA role that needs to be assigned.
 *      ckpt - Flag indicating if called on standby.
 *      ret_ptr - Pointer to the pointer of the SUSI structure created.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

uint32_t avd_new_assgn_susi(AVD_CL_CB *cb, AVD_SU *su, AVD_SI *si,
			 SaAmfHAStateT ha_state, bool ckpt, AVD_SU_SI_REL **ret_ptr)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	AVD_SU_SI_REL *susi;
	AVD_COMP_CSI_REL *compcsi;
	AVD_COMP *l_comp;
	AVD_CSI *l_csi;
	AVD_COMPCS_TYPE *cst;

	TRACE_ENTER2("'%s' '%s' state=%u", su->name.value, si->name.value, ha_state);

	if (ckpt == false)
		/* on Active AMFD empty SI should never be tried for assignments.
		 * on Standby AMFD this may be still possible as CSI are not
		 * checkpointed dynamically */
		osafassert (si->list_of_csi != NULL);

	if ((susi = avd_susi_create(cb, si, su, ha_state, ckpt)) == NULL) {
		LOG_ER("%s: Could not create SUSI '%s' '%s'", __FUNCTION__,
			su->name.value, si->name.value);
		goto done;
	}

	susi->fsm = AVD_SU_SI_STATE_ASGN;
	susi->state = ha_state;

	/* Mark csi to be unassigned to detect duplicate assignment.*/
	l_csi = si->list_of_csi;
	while (l_csi != NULL) {
		l_csi->assign_flag = false;
		l_csi = l_csi->si_list_of_csi_next;
	}

	su->reset_all_comps_assign_flag();

	l_csi = si->list_of_csi;
	while (l_csi != NULL) {
		/* find a component that can be assigned this CSI */
		l_comp = su->find_unassigned_comp_that_provides_cstype(&l_csi->saAmfCSType);

		if (l_comp == NULL) {
			/* This means either - 1. l_csi cann't be assigned to any comp or 2. some comp got assigned 
			   and the rest cann't be assigned.*/
			l_csi = l_csi->si_list_of_csi_next;
			continue;
		}

		if ((compcsi = avd_compcsi_create(susi, l_csi, l_comp, true)) == NULL) {
			/* free all the CSI assignments and end this loop */
			avd_compcsi_delete(cb, susi, true);
			l_csi = l_csi->si_list_of_csi_next;
			continue;
		}

		l_comp->assign_flag = true;
		l_csi->assign_flag = true;
		l_csi = l_csi->si_list_of_csi_next;
	} /* while(l_csi != AVD_CSI_NULL) */

	/* For NPI SU each component will get only one CSI and that is already assigned above.*/
	if (su->saAmfSUPreInstantiable == false)
		goto npisu_done;

	/* After previous while loop(while (l_csi != NULL)) all the deserving components got assigned at least one. Some
	   components and csis may be left out. We need to ignore now all unassigned comps as they cann't be assigned 
	   any csi. Unassigned csis may include those csi, which cann't be assigned to any comp and those csi, which 
	   can be assigned to comp, which are already assigned(more than 1 csi to be assigned). 

	   Here, policy for assigning more than 1 csi to components is : Assign to max to the deserving comps and then
	   assign the rest csi to others. We are taking advantage of Specs defining implementation specific csi 
	   assigiment.*/
	TRACE("Now assiging more than one csi per comp");
	l_csi = si->list_of_csi;
	while (NULL !=  l_csi) {
		if (false == l_csi->assign_flag) {
			l_comp = su->list_of_comp;
			/* Assign to only those comps, which have assignment. Those comps, which could not have assignment 
			   before, cann't find compcsi here also.*/
			while (l_comp != NULL) { 
				AVD_COMP_TYPE *comptype = comptype_db->find(Amf::to_string(&l_comp->saAmfCompType));
				osafassert(comptype);
				if ((true == l_comp->assign_flag) && (comptype->saAmfCtCompCategory != SA_AMF_COMP_LOCAL)) {
					if (NULL != (cst = avd_compcstype_find_match(&l_csi->saAmfCSType, l_comp))) {
						if (SA_AMF_HA_ACTIVE == ha_state) {
							if (cst->saAmfCompNumCurrActiveCSIs < cst->saAmfCompNumMaxActiveCSIs) {
							} else { /* We cann't assign this csi to this comp, so check for another comp */
								l_comp = l_comp->su_comp_next;
								continue ;
							}
						} else {
							if (cst->saAmfCompNumCurrStandbyCSIs < cst->saAmfCompNumMaxStandbyCSIs) {
							} else { /* We cann't assign this csi to this comp, so check for another comp */
								l_comp = l_comp->su_comp_next;
								continue ;
							}
						}
						if ((compcsi = avd_compcsi_create(susi, l_csi, l_comp, true)) == NULL) {
							/* free all the CSI assignments and end this loop */
							avd_compcsi_delete(cb, susi, true);
							l_comp = l_comp->su_comp_next;
							continue;
						}
						l_csi->assign_flag = true;
						/* If one csi has been assigned to a comp, then look for another csi. */
						break;
					}/* if (NULL != (cst = avd_compcstype_find_match(&l_csi->saAmfCSType, l_comp))) */
				}/* if (true == l_comp->assign_flag) */
				l_comp = l_comp->su_comp_next;
			}/* while (l_comp != NULL) */
		}/* if (false == l_csi->assign_flag)*/
		l_csi = l_csi->si_list_of_csi_next;
	}/* while (l_csi != NULL) */

npisu_done:
	
	/* Check from csi deps perspective, dependent csi should be assigned only if all the sponsors
	   are assigned. Delete all those compcsi in which dependent csi is getting assigned even if 
	   anyone of the sponsors is unassigned.
	 */
	verify_csi_deps_and_delete_invalid_compcsi(susi);
	
	l_csi = si->list_of_csi;
	while (l_csi != NULL) {
		if (l_csi->assign_flag == false) {
			if (su->saAmfSUPreInstantiable == false)
				LOG_WA("Invalid configuration: More than one CSI" 
						" cannot be assigned to same"
						" component for NPI SU");
			LOG_ER("%s: Component type missing for SU '%s'", __FUNCTION__, su->name.value);
			LOG_ER("%s: Either component type or component is missing for CSI '%s'", __FUNCTION__, l_csi->name.value);
		}
		l_csi = l_csi->si_list_of_csi_next;
	}

	/* Now send the message about the SU SI assignment to
	 * the AvND. Send message only if this function is not 
	 * called while doing checkpoint update.
	 */

	if (susi->list_of_csicomp == NULL) {
		TRACE("Couldn't add any compcsi to Si'%s'", susi->si->name.value);
		avd_susi_update_assignment_counters(susi, AVSV_SUSI_ACT_DEL, static_cast<SaAmfHAStateT>(0), 
				static_cast<SaAmfHAStateT>(0));
		avd_susi_delete(cb, susi, true);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (false == ckpt) {
		if (avd_snd_susi_msg(cb, su, susi, AVSV_SUSI_ACT_ASGN, false, NULL) == NCSCC_RC_SUCCESS) {
			AVD_AVND *node = su->su_on_node;
			if ((node->admin_node_pend_cbk.invocation != 0) || ((node->admin_ng != NULL) &&
						(node->admin_ng->admin_ng_pend_cbk.invocation !=0))) {
				node->su_cnt_admin_oper++;
				TRACE("node:'%s', su_cnt_admin_oper:%u", 
						node->name.value, node->su_cnt_admin_oper);
				if (node->admin_ng != NULL) {
					node->admin_ng->node_oper_list.insert(Amf::to_string(&node->name));
					TRACE("node_oper_list size:%u",node->admin_ng->oper_list_size());
				}
			}
		} else {
			/* free all the CSI assignments and end this loop */
			avd_compcsi_delete(cb, susi, true);
			/* Unassign the SUSI */
			avd_susi_update_assignment_counters(susi, AVSV_SUSI_ACT_DEL, static_cast<SaAmfHAStateT>(0), static_cast<SaAmfHAStateT>(0));
			avd_susi_delete(cb, susi, true);

			goto done;
		}

		m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(cb, susi, AVSV_CKPT_AVD_SI_ASS);
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVSV_CKPT_AVD_SU_CONFIG);
	}

	*ret_ptr = susi;
	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE();
	return rc;
}

/**
 * @brief	Sends repair operation message to node director to perform repair of the faulted SU.
 *		This function is used to perform repiar after sufailover recovery.
 *
 * @param[in]   su
 *
 **/
static void su_try_repair(const AVD_SU *su)
{
	TRACE_ENTER2("Repair for SU:'%s'", su->name.value);

	if ((su->sg_of_su->saAmfSGAutoRepair) && (su->saAmfSUFailover) &&
			(su->saAmfSUOperState == SA_AMF_OPERATIONAL_DISABLED) &&
			(su->saAmfSUPresenceState != SA_AMF_PRESENCE_INSTANTIATION_FAILED) && 
			(su->saAmfSUPresenceState != SA_AMF_PRESENCE_TERMINATION_FAILED)) {

		saflog(LOG_NOTICE, amfSvcUsrName, "Ordering Auto repair of '%s' as sufailover repair action",
				su->sg_of_su->name.value);
		avd_admin_op_msg_snd(&su->name, AVSV_SA_AMF_SU,
				static_cast<SaAmfAdminOperationIdT>(SA_AMF_ADMIN_REPAIRED), su->su_on_node); 
	} else {
		saflog(LOG_NOTICE, amfSvcUsrName, "Autorepair not done for '%s'", su->sg_of_su->name.value);
	}

	TRACE_LEAVE();
}

/**
 * @brief	Completes admin operation on node. 
 *		If node is undergoing some admin operation, this function 
 *		will respond to imm with the provided result. Also all admin
 *		operation related parameters will be cleared in node ptr.  
 * @param 	ptr to node 
 * @param 	result
 * 
 */
static void node_complete_admin_op(AVD_AVND *node, SaAisErrorT result)
{
	if (node->admin_node_pend_cbk.invocation != 0) {
		avd_saImmOiAdminOperationResult(avd_cb->immOiHandle,
				node->admin_node_pend_cbk.invocation, result);
		node->admin_node_pend_cbk.invocation = 0;
		node->admin_node_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
	}
}
/**
 * @brief       When AMFD gets assignment response message or escalation
 *              request from AMFND, it runs SG FSM. In the context of admin
 *              operation on any AMF entity, after running SG FSM, AMFD 
 *              evaluates if this message or request marks the completion 
 *              of admin operation or atleast makes some progress in admin 
 *              operation. This functions evaluates the message or request 
 *              for nodegroup admin operation.
 * @param[in]   ptr to SU (AVD_SU). 
 * @param[in]   res(SaAisErrorT). 
 */
static void process_su_si_response_for_ng(AVD_SU *su, SaAisErrorT res)
{
	TRACE_ENTER2("'%s'",su->name.value);
	AVD_AMF_NG *ng = su->su_on_node->admin_ng;
	AVD_AVND *node = su->su_on_node;
	bool flag = false;
	/* Node may be in SHUTTING_DOWN state because of shutdown operation
	   on nodegroup. Check if node can be transitioned to LOCKED sate.*/ 
	if (node->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
		m_AVD_IS_NODE_LOCK(node, flag);
		if (flag == true)
			node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
	}
	/*In 2N model, if both active or standby SUs were part of nodegroup then
	  AMFD internally executes operation as equivalent to SG shutdown/lock.
	  So after completion of operation, clear SG admin state and give fresh
	  assignments outside the nodegroup.*/
	if ((su->sg_of_su->ng_using_saAmfSGAdminState == true) &&
			(su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) &&
			(su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE)) {
		su->sg_of_su->saAmfSGAdminState = SA_AMF_ADMIN_UNLOCKED;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, su->sg_of_su, AVSV_CKPT_SG_ADMIN_STATE);
		su->sg_of_su->ng_using_saAmfSGAdminState = false;
		TRACE("ng_using_saAmfSGAdminState:%u",su->sg_of_su->ng_using_saAmfSGAdminState);
		su->sg_of_su->realign(avd_cb, su->sg_of_su);
		if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
			/*This means no new assignments in the SG.*/
			avd_sidep_sg_take_action(su->sg_of_su);
			avd_sg_app_su_inst_func(avd_cb, su->sg_of_su);
		}
	}
	if (ng == NULL)
		goto done;
	/* During nodegroup shutdown operation, mark nodegrouoop LOCKED if 
	   all nodes transtioned to LOCKED state.*/
	if (ng->saAmfNGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
		flag = true;		
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
				iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *tmp_node = avd_node_get(*iter);
			if (tmp_node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED)
				flag = false;
		}
		if (flag == true) {
			TRACE("Move '%s' to locked state as all nodes are locked.", ng->name.value);
			avd_ng_admin_state_set(ng, SA_AMF_ADMIN_LOCKED);
		}
	}
	/*If no futher SU is undergoing assignment changes, erase node from
	   nodgroup operation tracking list.*/
	if (node->su_cnt_admin_oper == 0) {
		ng->node_oper_list.erase(Amf::to_string(&node->name));
		TRACE("node_oper_list size:%u",ng->oper_list_size());
	}
	/*If assignment changes are done on all the SUs on each node of nodegroup
	  then reply to IMM for status of admin operation.*/
	if (ng->node_oper_list.empty())
		ng_complete_admin_op(ng, res);
done:
	TRACE_LEAVE();
}
/**
 * @brief	Perform failover of SU as a single entity and free all SUSIs associated with this SU.
 * @param 	su 
 * 
 * @return SaAisErrorT
 */
static uint32_t sg_su_failover_func(AVD_SU *su)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	SaAisErrorT res = SA_AIS_OK;

	TRACE_ENTER2("'%s', %u", su->name.value, su->sg_of_su->sg_fsm_state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL) {
		TRACE("'%s' has no assignments", su->name.value);
		rc =  NCSCC_RC_SUCCESS;
		goto done;
	}

	su->set_oper_state(SA_AMF_OPERATIONAL_DISABLED);
	su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
	if (su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)
		su->complete_admin_op(SA_AIS_OK);
	else
		su->complete_admin_op(SA_AIS_ERR_TIMEOUT);
	su->disable_comps(SA_AIS_ERR_TIMEOUT);
	if ((su->su_on_node->admin_node_pend_cbk.invocation != 0) ||
			(su->su_on_node->admin_ng != NULL)) {
		/* Node or nodegroup level operation is going on the node hosting the SU for which 
		   sufailover got escalated. Sufailover event will always come after the 
		   initiation of node level operation on the list of SUs. So if this SU has 
		   list of SUSIs, AMF would have sent assignment as a part of node level operation.
		   Now SUSIs in this SU are going to be deleted so we can decrement the counter in node.
		   In case of Node unlock this counter is incremented per susi. So decrement 
		   should also be done per susi.
		 */ 
		if (su->su_on_node->saAmfNodeAdminState == SA_AMF_ADMIN_UNLOCKED) {
			for (AVD_SU_SI_REL *susi = su->list_of_susi; susi; susi = susi->su_next) {
				if (susi->fsm == AVD_SU_SI_STATE_ASGN) 
					su->su_on_node->su_cnt_admin_oper--;
			}
			res = SA_AIS_ERR_TIMEOUT;
		} else
			su->su_on_node->su_cnt_admin_oper--;

		/* If node level operation is finished on all the SUs, reply to imm.*/
		if (su->su_on_node->su_cnt_admin_oper == 0) {
			AVD_AVND *node = su->su_on_node;
			node_complete_admin_op(node, res);
		}
	}

	/*If the AvD is in AVD_APP_STATE then reassign all the SUSI assignments for this SU */
	if (avd_cb->init_state == AVD_APP_STATE) {
		/* Unlike active, quiesced and standby HA states, assignment counters
		   in quiescing HA state are updated when AMFD receives assignment 
		   response from AMFND. During sufailover amfd will not receive 
		   assignment response from AMFND. 
		   So if any SU is under going modify operation then update assignment 
		   counters for those SUSIs which are in quiescing state in the SU.
		 */ 
		for (AVD_SU_SI_REL *susi = su->list_of_susi; susi; susi = susi->su_next) {
			if ((susi->fsm == AVD_SU_SI_STATE_MODIFY) &&
					(susi->state == SA_AMF_HA_QUIESCING)) {
				avd_susi_update_assignment_counters(susi, AVSV_SUSI_ACT_MOD,
						SA_AMF_HA_QUIESCING, SA_AMF_HA_QUIESCED);
			}
			else if ((susi->fsm == AVD_SU_SI_STATE_MODIFY) &&
					(susi->state == SA_AMF_HA_ACTIVE)) {
				/* SUSI is undergoing active modification. For active state 
				   saAmfSINumCurrActiveAssignments was increased when active  
				   assignment had been sent. So decrement the count in SI before 
				   deleting the SUSI. */
				susi->si->dec_curr_act_ass();
			}
			/* Reply to IMM for admin operation on SI */
			if (susi->si->invocation != 0) {
				avd_saImmOiAdminOperationResult(avd_cb->immOiHandle,
						susi->si->invocation, SA_AIS_OK);
				susi->si->invocation = 0;
			}
		}
		su->sg_of_su->node_fail(avd_cb, su);
		su->delete_all_susis();
	}

	/* If nodegroup level operation is finished on all the nodes, reply to imm.*/
	if (su->su_on_node->admin_ng != NULL)
		process_su_si_response_for_ng(su, res);

	rc =  NCSCC_RC_SUCCESS;

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * @brief	Performs sufailover or component failover	
 *
 * @param[in]   su
 *
 * @return	NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS 
 **/
static uint32_t su_recover_from_fault(AVD_SU *su)
{
	uint32_t rc;

	if ((su->saAmfSUFailover) && (su->saAmfSUOperState == SA_AMF_OPERATIONAL_DISABLED)) {
		rc = sg_su_failover_func(su);
	} else {
		rc = su->sg_of_su->su_fault(avd_cb, su);
	}
	return rc;
}

/**
 * @brief       Try to repair node by sending reboot message to node
 *              director of the node.
 *
 * @param[in]   ptr to node. 
 *
 **/

static void node_try_repair(AVD_AVND *node)
{
	TRACE_ENTER2("'%s'", node->name.value);

	if (node->saAmfNodeAutoRepair) {
		LOG_NO("Ordering reboot of '%s' as node fail/switch-over repair action",
				node->name.value);
		saflog(LOG_NOTICE, amfSvcUsrName,
			"Ordering reboot of '%s' as node fail/switch-over repair action",
			node->name.value);
		avd_d2n_reboot_snd(node);
	} else {
		LOG_NO("NodeAutorepair disabled for '%s', no reboot ordered",
				node->name.value);
		saflog(LOG_NOTICE, amfSvcUsrName,
			"NodeAutorepair disabled for '%s', NO reboot ordered",
			node->name.value);

	}
	TRACE_LEAVE();
}

/**
 * @brief       Performs node-switchover recovery. 
 *
 * @param[in]   ptr to failed su.
 *
 **/
static void perform_nodeswitchover_recovery(AVD_AVND *node)
{
	bool node_reboot = true;
	TRACE_ENTER2("'%s'", node->name.value);

	AVD_SU *su = node->list_of_su;
	for (;su != NULL; su = su->avnd_list_su_next) {
		su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);

		if (su->list_of_susi == NULL)
			continue;

		if (su_recover_from_fault(su) == NCSCC_RC_FAILURE) {
			LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
			goto done;
		}

		if (su->list_of_susi != NULL)
			node_reboot = false;

		if (avd_sg_app_su_inst_func(avd_cb, su->sg_of_su) == NCSCC_RC_FAILURE) {
			LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
			goto done;
		}
	}

	if (node_reboot == true)
		node_try_repair(node);
done:
	TRACE_LEAVE();
}
/*****************************************************************************
 * Function: avd_su_oper_state_func
 *
 * Purpose:  This function is the handler for the operational state change event
 * indicating the arrival of the operational state change message from 
 * node director. It will process the message and call the redundancy model
 * specific event processing routine.
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

void avd_su_oper_state_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	AVD_AVND *node;
	AVD_SU *su, *i_su;
	SaAmfReadinessStateT old_state;
	bool node_reboot_req = true;

	TRACE_ENTER2("id:%u, node:%x, '%s' state:%u", n2d_msg->msg_info.n2d_opr_state.msg_id,
				 n2d_msg->msg_info.n2d_opr_state.node_id,
				 n2d_msg->msg_info.n2d_opr_state.su_name.value,
				 n2d_msg->msg_info.n2d_opr_state.su_oper_state);

	if ((node = avd_msg_sanity_chk(evt, n2d_msg->msg_info.n2d_opr_state.node_id, AVSV_N2D_OPERATION_STATE_MSG,
		n2d_msg->msg_info.n2d_opr_state.msg_id)) == NULL) {
		/* sanity failed return */
		goto done;
	}

	if ((node->node_state == AVD_AVND_STATE_ABSENT) ||(node->node_state == AVD_AVND_STATE_GO_DOWN)) {
		LOG_ER("%s: invalid node state %u", __FUNCTION__, node->node_state);
		goto done;
	}

	/* 
	 * Send the Ack message to the node, indicationg that the message with this
	 * message ID is received successfully.
	 */
	m_AVD_SET_AVND_RCV_ID(cb, node, (n2d_msg->msg_info.n2d_opr_state.msg_id));

	if (avd_snd_node_ack_msg(cb, node, node->rcv_msg_id) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: avd_snd_node_ack_msg failed", __FUNCTION__);
	}

	/* Find and validate the SU. */

	/* get the SU from the tree */

	if ((su = su_db->find(Amf::to_string(&n2d_msg->msg_info.n2d_opr_state.su_name))) == NULL) {
		LOG_ER("%s: %s not found", __FUNCTION__, n2d_msg->msg_info.n2d_opr_state.su_name.value);
		goto done;
	}

	if (n2d_msg->msg_info.n2d_opr_state.rec_rcvr.saf_amf == SA_AMF_NODE_SWITCHOVER) {
		saflog(LOG_NOTICE, amfSvcUsrName, "Node Switch-Over requested by '%s'",
			   node->name.value);
	} else if (n2d_msg->msg_info.n2d_opr_state.rec_rcvr.saf_amf == SA_AMF_NODE_FAILOVER) {
		saflog(LOG_NOTICE, amfSvcUsrName, "Node Fail-Over requested by '%s'",
			   node->name.value);
	}

	/* Verify that the SU and node oper state is diabled and rcvr is failfast */
	if ((n2d_msg->msg_info.n2d_opr_state.su_oper_state == SA_AMF_OPERATIONAL_DISABLED) &&
	    (n2d_msg->msg_info.n2d_opr_state.node_oper_state == SA_AMF_OPERATIONAL_DISABLED) &&
	    (n2d_msg->msg_info.n2d_opr_state.rec_rcvr.saf_amf == SA_AMF_NODE_FAILFAST)) {
		/* as of now do the same opearation as ncs su failure */
		su->set_oper_state(SA_AMF_OPERATIONAL_DISABLED);
		if ((node->type == AVSV_AVND_CARD_SYS_CON) && (node->node_info.nodeId == cb->node_id_avd)) {
			TRACE("Component in %s requested FAILFAST", su->name.value);
		}

		avd_nd_ncs_su_failed(cb, node);
		goto done;
	}

	/* Check if all SUs are in 'in-service' cluster-wide, if so start assignments */
	if ((cb->amf_init_tmr.is_active == true) &&
			(cluster_su_instantiation_done(cb, su) == true)) {
		avd_stop_tmr(cb, &cb->amf_init_tmr);
		cluster_startup_expiry_event_generate(cb);
	}

	/* Verify that the SU operation state is disable and do the processing. */
	if (n2d_msg->msg_info.n2d_opr_state.su_oper_state == SA_AMF_OPERATIONAL_DISABLED) {
		/* if the SU is NCS SU, call the node FSM routine to handle the failure.
		 */
		if (su->sg_of_su->sg_ncs_spec == true) {
			su->set_oper_state(SA_AMF_OPERATIONAL_DISABLED);
			avd_nd_ncs_su_failed(cb, node);
			goto done;
		}

		/* If the cluster timer hasnt expired, mark the SU operation state
		 * disabled.      
		 */

		if (cb->init_state == AVD_INIT_DONE) {
			su->set_oper_state(SA_AMF_OPERATIONAL_DISABLED);
			su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
			if (n2d_msg->msg_info.n2d_opr_state.node_oper_state == SA_AMF_OPERATIONAL_DISABLED) {
				/* Mark the node operational state as disable and make all the
				 * application SUs in the node as O.O.S.
				 */
				avd_node_oper_state_set(node, SA_AMF_OPERATIONAL_DISABLED);
				node->recvr_fail_sw = true;
				i_su = node->list_of_su;
				while (i_su != NULL) {
					su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
					i_su = i_su->avnd_list_su_next;
				}
			}	/* if (n2d_msg->msg_info.n2d_opr_state.node_oper_state == SA_AMF_OPERATIONAL_DISABLED) */
		} /* if(cb->init_state == AVD_INIT_DONE) */
		else if (cb->init_state == AVD_APP_STATE) {
			su->set_oper_state(SA_AMF_OPERATIONAL_DISABLED);
			su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
			if (n2d_msg->msg_info.n2d_opr_state.node_oper_state == SA_AMF_OPERATIONAL_DISABLED) {
				/* Mark the node operational state as disable and make all the
				 * application SUs in the node as O.O.S. Also call the SG FSM
				 * to do the reallignment of SIs for assigned SUs.
				 */
				avd_node_oper_state_set(node, SA_AMF_OPERATIONAL_DISABLED);
				node->recvr_fail_sw = true;
				switch (n2d_msg->msg_info.n2d_opr_state.rec_rcvr.raw) {
				case SA_AMF_NODE_FAILOVER:
					if ((node->node_info.nodeId == cb->node_id_avd) && 
							(node->saAmfNodeAutoRepair)) {
						/* This is a case when Act ctlr is rebooting. Don't do appl failover
						   as of now because during appl failover if Act controller reboots,
						   then there may be packet losses. Anyway, this controller is
						   rebooting and Std controller will take up its role and will do 
						   appl failover. Here Recovery and Repair is done at the same time.*/
						saflog(LOG_NOTICE, amfSvcUsrName,
								"Ordering reboot of '%s' as node fail/switch-over"
								" repair action",
								node->name.value);
						avd_d2n_reboot_snd(node);

						/* Finish as many IMM jobs as possible because active 
						   controller is rebooting.

						   TODO(Praveen): All the pending IMM related jobs should 
						   be handled by the standby when it will become
						   active after controller fail-over. 
						   TODO: There should be some flush method on the job queue.
						*/
						AvdJobDequeueResultT job_res = JOB_EXECUTED;
						while (job_res == JOB_EXECUTED)
							job_res = Fifo::execute(cb->immOiHandle);

						goto done;
					} else {
						avd_pg_node_csi_del_all(avd_cb, node);
						avd_node_down_appl_susi_failover(avd_cb, node);
						break;
					}
					break;
				case SA_AMF_NODE_SWITCHOVER:
					perform_nodeswitchover_recovery(su->su_on_node);
					goto done;
					break;
				default :
					break;
				}

				if (node_reboot_req)
					node_try_repair(node);


			} else { /* if (n2d_msg->msg_info.n2d_opr_state.node_oper_state == SA_AMF_OPERATIONAL_DISABLED) */

				if (su->list_of_susi != AVD_SU_SI_REL_NULL) {
					/* Since assignments exists call the SG FSM. */
					switch (n2d_msg->msg_info.n2d_opr_state.rec_rcvr.raw) {
					case SA_AMF_COMPONENT_FAILOVER:
						if (su->sg_of_su->su_fault(cb, su) == NCSCC_RC_FAILURE) {
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;
					case 0: /* Support for older releases. */
					case AVSV_ERR_RCVR_SU_FAILOVER:
						if (sg_su_failover_func(su) == NCSCC_RC_FAILURE) {
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;
					default :
						LOG_ER("Recovery:'%u' not supported",
								n2d_msg->msg_info.n2d_opr_state.rec_rcvr.raw);
						break;
					}
				}

				if (su->list_of_susi == NULL)
					su_try_repair(su);

				/* Verify the SG to check if any instantiations need
				 * to be done for the SG on which this SU exists.
				 */
				if (avd_sg_app_su_inst_func(cb, su->sg_of_su) == NCSCC_RC_FAILURE) {
					/* Bad situation. Free the message and return since
					 * receive id was not processed the event will again
					 * comeback which we can then process.
					 */
					LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->sg_of_su->name.value);
					goto done;
				}

			} /* else (n2d_msg->msg_info.n2d_opr_state.node_oper_state == SA_AMF_OPERATIONAL_DISABLED) */

		}
		/* else if(cb->init_state == AVD_APP_STATE) */
	} /* if (n2d_msg->msg_info.n2d_opr_state.su_oper_state == SA_AMF_OPERATIONAL_DISABLED) */
	else if (n2d_msg->msg_info.n2d_opr_state.su_oper_state == SA_AMF_OPERATIONAL_ENABLED) {
		su->set_oper_state(SA_AMF_OPERATIONAL_ENABLED);
		/* if the SU is NCS SU, mark the SU readiness state as in service and call
		 * the SG FSM.
		 */
		if (su->sg_of_su->sg_ncs_spec == true) {
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) { 
				su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
				/* Run the SG FSM */
				if (su->sg_of_su->su_insvc(cb, su) == NCSCC_RC_FAILURE) {
					/* Bad situation. Free the message and return since
					 * receive id was not processed the event will again
					 * comeback which we can then process.
					 */
					LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
					su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
					goto done;
				}
			}
		} else {	/* if(su->sg_of_su->sg_ncs_spec == true) */
			old_state = su->saAmfSuReadinessState;

			/* If oper state of Uninstantiated SU got ENABLED so try to instantiate it 
			   after evaluating SG. */
			if (su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) {
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
				goto done;
			}

			if (su->is_in_service() == true) {
				su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
				if ((cb->init_state == AVD_APP_STATE) && (old_state == SA_AMF_READINESS_OUT_OF_SERVICE)) {
					/* An application SU has become in service call SG FSM */
					if (su->sg_of_su->su_insvc(cb, su) == NCSCC_RC_FAILURE) {
						/* Bad situation. Free the message and return since
						 * receive id was not processed the event will again
						 * comeback which we can then process.
						 */
						LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
						goto done;
					}
				}
			}
		}
	}

done:
	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;
	TRACE_LEAVE();
}

static void susi_assign_msg_dump(const char *func, unsigned int line,
	AVSV_N2D_INFO_SU_SI_ASSIGN_MSG_INFO *info)
{
	LOG_ER("%s:%d %u %u %u %u %x", func, line, info->error,
		info->ha_state, info->msg_act, info->msg_id, info->node_id);
	LOG_ER("%s:%d %s", func, line, info->si_name.value);
	LOG_ER("%s:%d %s", func, line, info->su_name.value);
}


/*****************************************************************************
 * Function: avd_su_si_assign_func
 *
 * Purpose:  This function is the handler for su si assign event
 * indicating the arrival of the response to the su si assign message from
 * node director. It then call the redundancy model specific routine to process
 * this event. 
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

void avd_su_si_assign_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	AVD_AVND *node;
	AVD_SU *su, *temp_su;
	AVD_SU_SI_REL *susi;
	bool q_flag = false, qsc_flag = false, all_su_unassigned = true, all_csi_rem = true;

	TRACE_ENTER2("id:%u, node:%x, act:%u, '%s', '%s', ha:%u, err:%u, single:%u",
			n2d_msg->msg_info.n2d_su_si_assign.msg_id, n2d_msg->msg_info.n2d_su_si_assign.node_id,
			n2d_msg->msg_info.n2d_su_si_assign.msg_act,  n2d_msg->msg_info.n2d_su_si_assign.su_name.value, 
			n2d_msg->msg_info.n2d_su_si_assign.si_name.value,  n2d_msg->msg_info.n2d_su_si_assign.ha_state,
			n2d_msg->msg_info.n2d_su_si_assign.error, n2d_msg->msg_info.n2d_su_si_assign.single_csi);

	if ((node = avd_msg_sanity_chk(evt, n2d_msg->msg_info.n2d_su_si_assign.node_id, AVSV_N2D_INFO_SU_SI_ASSIGN_MSG,
	     n2d_msg->msg_info.n2d_su_si_assign.msg_id)) == NULL) {
		/* sanity failed return */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	if ((node->node_state == AVD_AVND_STATE_ABSENT) || (node->node_state == AVD_AVND_STATE_GO_DOWN)) {
		LOG_ER("%s: invalid node state %u", __FUNCTION__, node->node_state);
		goto done;
	}

	/* update the receive id count */
	m_AVD_SET_AVND_RCV_ID(cb, node, (n2d_msg->msg_info.n2d_su_si_assign.msg_id));

	/* 
	 * Send the Ack message to the node, indicationg that the message with this
	 * message ID is received successfully.
	 */
	if (avd_snd_node_ack_msg(cb, node, node->rcv_msg_id) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: avd_snd_node_ack_msg failed", __FUNCTION__);
	}

	if (n2d_msg->msg_info.n2d_su_si_assign.error != NCSCC_RC_SUCCESS) {
		switch (n2d_msg->msg_info.n2d_su_si_assign.msg_act) {
		case AVSV_SUSI_ACT_ASGN:
			LOG_NO("Failed to assign '%s' HA state to '%s'",
					avd_ha_state[n2d_msg->msg_info.n2d_su_si_assign.ha_state],
					n2d_msg->msg_info.n2d_su_si_assign.su_name.value);
			break;
		case AVSV_SUSI_ACT_MOD:
			LOG_NO("Failed to modify '%s' assignment to '%s'", 
					n2d_msg->msg_info.n2d_su_si_assign.su_name.value,
					avd_ha_state[n2d_msg->msg_info.n2d_su_si_assign.ha_state]);
			break;
		case AVSV_SUSI_ACT_DEL:
			LOG_NO("Failed to delete assignment from '%s'",
					n2d_msg->msg_info.n2d_su_si_assign.su_name.value);
			break;
		default:
			LOG_WA("%s: unknown action %u", __FUNCTION__,
					n2d_msg->msg_info.n2d_su_si_assign.msg_act);
			break;
		}
	}

	if (n2d_msg->msg_info.n2d_su_si_assign.si_name.length == 0) {

		/* get the SU from the tree since this is across the
		 * SU operation. 
		 */

		if ((su = su_db->find(Amf::to_string(&n2d_msg->msg_info.n2d_su_si_assign.su_name))) == NULL) {
			LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, n2d_msg->msg_info.n2d_su_si_assign.su_name.value);
			goto done;
		}

		if (su->list_of_susi == AVD_SU_SI_REL_NULL) {
			LOG_ER("%s: no susis", __FUNCTION__);
			goto done;
		}

		TRACE("%u", n2d_msg->msg_info.n2d_su_si_assign.msg_act);
		switch (n2d_msg->msg_info.n2d_su_si_assign.msg_act) {
		case AVSV_SUSI_ACT_DEL:
			break;

		case AVSV_SUSI_ACT_MOD:
			/* Verify that the SUSI is in the modify state for the same HA state. */
			susi = su->list_of_susi;
			while (susi != AVD_SU_SI_REL_NULL) {
				if ((susi->state != n2d_msg->msg_info.n2d_su_si_assign.ha_state)
				    && (susi->state != SA_AMF_HA_QUIESCING)
				    && (n2d_msg->msg_info.n2d_su_si_assign.ha_state != SA_AMF_HA_QUIESCED)
				    && (susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
					/* some other event has caused further state change ignore
					 * this message, by accepting the receive id and  droping the message.
					 * message id has already been accepted above.
					 */

					avsv_dnd_msg_free(n2d_msg);
					evt->info.avnd_msg = NULL;
					goto done;
				} else if ((susi->state == SA_AMF_HA_QUIESCING)
					   && (susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
					qsc_flag = true;
				}

				susi = susi->su_next;

			}	/* while (susi != AVD_SU_SI_REL_NULL) */
			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				if (n2d_msg->msg_info.n2d_su_si_assign.ha_state == SA_AMF_HA_QUIESCING) {
					q_flag = true;
					su->set_all_susis_assigned();
				} else {
					/* set the  assigned or quiesced state in the SUSIs. */
					if (qsc_flag == true)
						su->set_all_susis_assigned_quiesced();
					else
						su->set_all_susis_assigned();
				}
			}
			break;
		default:
			LOG_ER("%s: invalid act %u", __FUNCTION__, n2d_msg->msg_info.n2d_su_si_assign.msg_act);
			susi_assign_msg_dump(__FUNCTION__, __LINE__, &n2d_msg->msg_info.n2d_su_si_assign);
			goto done;
			break;
		}		/* switch (n2d_msg->msg_info.n2d_su_si_assign.msg_act) */

		/* Call the redundancy model specific procesing function. Dont call
		 * in case of acknowledgment for quiescing.
		 */

		/* Now process the acknowledge message based on
		 * Success or failure.
		 */
		if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
			if (q_flag == false) {
				su->sg_of_su->susi_success(cb, su, AVD_SU_SI_REL_NULL,
						n2d_msg->msg_info.n2d_su_si_assign.msg_act,
						n2d_msg->msg_info.n2d_su_si_assign.ha_state);
			}
		} else {
			susi_assign_msg_dump(__FUNCTION__, __LINE__,
					&n2d_msg->msg_info.n2d_su_si_assign);
			su->sg_of_su->susi_failed(cb, su, AVD_SU_SI_REL_NULL,
					n2d_msg->msg_info.n2d_su_si_assign.msg_act,
					n2d_msg->msg_info.n2d_su_si_assign.ha_state);
		}
	} else {		/* if (n2d_msg->msg_info.n2d_su_si_assign.si_name.length == 0) */

		/* Single SU SI assignment find the SU SI structure */

		if ((susi = avd_susi_find(cb, &n2d_msg->msg_info.n2d_su_si_assign.su_name,
			&n2d_msg->msg_info.n2d_su_si_assign.si_name)) == AVD_SU_SI_REL_NULL) {

			/* Acknowledgement for a deleted SU SI ignore the message */
			LOG_IN("%s: avd_susi_find failed for %s %s", __FUNCTION__,
				n2d_msg->msg_info.n2d_su_si_assign.su_name.value,
				n2d_msg->msg_info.n2d_su_si_assign.si_name.value);
			goto done;
		}

		TRACE("%u", n2d_msg->msg_info.n2d_su_si_assign.msg_act);
		switch (n2d_msg->msg_info.n2d_su_si_assign.msg_act) {
		case AVSV_SUSI_ACT_DEL:
			TRACE("Del:single_csi '%u', susi '%p'", n2d_msg->msg_info.n2d_su_si_assign.single_csi,susi);
			if (true == n2d_msg->msg_info.n2d_su_si_assign.single_csi) {
				AVD_COMP *comp;
				AVD_CSI  *csi;
				/* This is a case of single csi assignment/removal. */
				/* Don't worry abt n2d_msg->msg_info.n2d_su_si_assign.error as SUCCESS/FAILURE. We will
				   mark it as success and complete the assignment to others. In case any comp rejects 
				   csi, recovery will take care of this.*/
				AVD_COMP_CSI_REL *t_comp_csi;
				AVD_SU_SI_REL *t_sisu;
				AVD_CSI *csi_tobe_deleted = NULL;

				osafassert(susi->csi_add_rem);
				/* Checkpointing for compcsi removal */
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV (avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);

				susi->csi_add_rem = static_cast<SaBoolT>(false);
				comp = comp_db->find(Amf::to_string(&susi->comp_name));
				osafassert(comp);
				csi = csi_db->find(Amf::to_string(&susi->csi_name));
				osafassert(csi);

				for (t_comp_csi = susi->list_of_csicomp; t_comp_csi; t_comp_csi = t_comp_csi->susi_csicomp_next) { 
					if ((t_comp_csi->comp == comp) && (t_comp_csi->csi == csi))
						break;
				}
				osafassert(t_comp_csi);
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);

				/* Store csi if this is the last comp-csi to be deleted. */
				csi_tobe_deleted = t_comp_csi->csi;
				/* Delete comp-csi. */
				avd_compcsi_from_csi_and_susi_delete(susi, t_comp_csi, false);

				/* Search for the next SUSI to be added csi. */
				t_sisu = susi->si->list_of_sisu;
				while(t_sisu) {
					if (true == t_sisu->csi_add_rem) {
						all_csi_rem = false;
						comp = comp_db->find(Amf::to_string(&t_sisu->comp_name));
						osafassert(comp);
						csi = csi_db->find(Amf::to_string(&t_sisu->csi_name));
						osafassert(csi);

						for (t_comp_csi = t_sisu->list_of_csicomp; t_comp_csi; t_comp_csi = t_comp_csi->susi_csicomp_next) {
							if ((t_comp_csi->comp == comp) && (t_comp_csi->csi == csi))
								break;
						}
						osafassert(t_comp_csi);
						avd_snd_susi_msg(cb, t_sisu->su, t_sisu, AVSV_SUSI_ACT_DEL, true, t_comp_csi);
						/* Break here. We need to send one by one.  */
						break;
					}
					t_sisu = t_sisu->si_next;
				}/* while(t_sisu) */
				if (true == all_csi_rem) {
					/* All the csi removed, so now delete pg tracking and CSI. */
					csi_cmplt_delete(csi_tobe_deleted, false);
				}
				/* Comsume this message. */
				goto done;
			}
			break;

		case AVSV_SUSI_ACT_ASGN:
			TRACE("single_csi '%u', susi '%p'", n2d_msg->msg_info.n2d_su_si_assign.single_csi,susi);
			if (true == n2d_msg->msg_info.n2d_su_si_assign.single_csi) {
				AVD_COMP *comp;
				AVD_CSI  *csi;
				/* This is a case of single csi assignment/removal. */
				/* Don't worry abt n2d_msg->msg_info.n2d_su_si_assign.error as SUCCESS/FAILURE. We will
				   mark it as success and complete the assignment to others. In case any comp rejects 
				   csi, recovery will take care of this.*/
				AVD_COMP_CSI_REL *t_comp_csi;
				AVD_SU_SI_REL *t_sisu;

				osafassert(susi->csi_add_rem);
				susi->csi_add_rem = static_cast<SaBoolT>(false);
				comp = comp_db->find(Amf::to_string(&susi->comp_name));
				osafassert(comp);
				csi = csi_db->find(Amf::to_string(&susi->csi_name));
				osafassert(csi);

				for (t_comp_csi = susi->list_of_csicomp; t_comp_csi; t_comp_csi = t_comp_csi->susi_csicomp_next) {
					if ((t_comp_csi->comp == comp) && (t_comp_csi->csi == csi))
						break;
				}
				osafassert(t_comp_csi);
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);

				/* Search for the next SUSI to be added csi. */
				t_sisu = susi->si->list_of_sisu;
				while(t_sisu) {
					if (true == t_sisu->csi_add_rem) {
						/* Find the comp csi relationship. */
						comp = comp_db->find(Amf::to_string(&t_sisu->comp_name));
						osafassert(comp);
						csi = csi_db->find(Amf::to_string(&t_sisu->csi_name));
						osafassert(csi);

						for (t_comp_csi = t_sisu->list_of_csicomp; t_comp_csi; t_comp_csi = t_comp_csi->susi_csicomp_next) { 
							if ((t_comp_csi->comp == comp) && (t_comp_csi->csi == csi))
								break;
						}
						osafassert(t_comp_csi);
						avd_snd_susi_msg(cb, t_sisu->su, t_sisu, AVSV_SUSI_ACT_ASGN, true, t_comp_csi); 
						/* Break here. We need to send one by one.  */
						break;
					}
					t_sisu = t_sisu->si_next;
				}/* while(t_sisu) */
				if (t_sisu == NULL) {
					/* Since csi assignment is over, walkthrough other
					   unassigned CSIs for assignment. */
					for (csi = susi->si->list_of_csi; csi; csi = 
							csi->si_list_of_csi_next) {
						if (csi->list_compcsi == NULL) {
							/* Assign this csi and when the assignment
							   will be over for this csi, then other
							   unassigned CSIs will be taken.*/
							if (csi_assign_hdlr(csi) == SA_AIS_OK)
								goto done;
						}
					}
				}
				/* Comsume this message. */
				goto done;
			}
			/* Verify that the SUSI is in the assign state for the same HA state. */
			if ((susi->fsm != AVD_SU_SI_STATE_ASGN) ||
			    (susi->state != n2d_msg->msg_info.n2d_su_si_assign.ha_state)) {
				/* some other event has caused further state change ignore
				 * this message, by accepting the receive id and
				 * droping the message.
				 */
				LOG_IN("%s: assign susi not in proper state %u %u %u", __FUNCTION__,
					susi->fsm, susi->state, n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				LOG_IN("%s: %s %s", __FUNCTION__,
					susi->su->name.value, susi->si->name.value);
				goto done;
			}

			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				susi->fsm = AVD_SU_SI_STATE_ASGND;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);

				/* trigger pg upd */
				avd_pg_susi_chg_prc(cb, susi);
			}
			break;

		case AVSV_SUSI_ACT_MOD:
			/* Verify that the SUSI is in the modify state for the same HA state. */
			if ((susi->fsm != AVD_SU_SI_STATE_MODIFY) ||
			    ((susi->state != n2d_msg->msg_info.n2d_su_si_assign.ha_state)
			     && (susi->state != SA_AMF_HA_QUIESCING)
			     && (n2d_msg->msg_info.n2d_su_si_assign.ha_state != SA_AMF_HA_QUIESCED))) {
				/* some other event has caused further state change ignore
				 * this message, by accepting the receive id and
				 * droping the message.
				 */

				/* log Info error that the susi mentioned is not in proper state. */
				LOG_IN("%s: mod susi not in proper state %u %u %u", __FUNCTION__,
					susi->fsm, susi->state, n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				LOG_IN("%s: %s %s", __FUNCTION__,
					susi->su->name.value, susi->si->name.value);
				goto done;
			}

			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				if (n2d_msg->msg_info.n2d_su_si_assign.ha_state == SA_AMF_HA_QUIESCING) {
					q_flag = true;
					susi->fsm = AVD_SU_SI_STATE_ASGND;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				} else {
					if (susi->state == SA_AMF_HA_QUIESCING) {
						susi->state = SA_AMF_HA_QUIESCED;
						avd_gen_su_ha_state_changed_ntf(cb, susi);
						avd_susi_update_assignment_counters(susi, AVSV_SUSI_ACT_MOD,
							 SA_AMF_HA_QUIESCING, SA_AMF_HA_QUIESCED);
					}

					/* set the assigned in the SUSIs. */
					susi->fsm = AVD_SU_SI_STATE_ASGND;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				}

				/* trigger pg upd */
				avd_pg_susi_chg_prc(cb, susi);
			}
			break;

		default:
			LOG_ER("%s: invalid action %u", __FUNCTION__, n2d_msg->msg_info.n2d_su_si_assign.msg_act);
			goto done;
			break;
		}		/* switch (n2d_msg->msg_info.n2d_su_si_assign.msg_act) */

		/* Call the redundancy model specific procesing function Dont call
		 * in case of acknowledgment for quiescing.
		 */

		/* Now process the acknowledge message based on
		 * Success or failure.
		 */
		TRACE("%u", n2d_msg->msg_info.n2d_su_si_assign.error);
		if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
			if (q_flag == false) {
				susi->si->sg_of_si->susi_success(cb, susi->su, susi,
						n2d_msg->msg_info.n2d_su_si_assign.msg_act,
						n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				if ((n2d_msg->msg_info.n2d_su_si_assign.msg_act == AVSV_SUSI_ACT_ASGN) &&
						(susi->su->sg_of_su->sg_ncs_spec == true)) {
					/* Since a NCS SU has been assigned trigger the node FSM. */
					/* For (ncs_spec == SA_TRUE), su will not be external, so su
						   will have node attached. */
					avd_nd_ncs_su_assigned(cb, susi->su->su_on_node);
				}
			}
		} else {
			susi->su->sg_of_su->susi_failed(cb, susi->su, susi,
					n2d_msg->msg_info.n2d_su_si_assign.msg_act,
					n2d_msg->msg_info.n2d_su_si_assign.ha_state);
		}
	}			/* else (n2d_msg->msg_info.n2d_su_si_assign.si_name.length == 0) */

	/* If there is any admin action going on SU and it is complete then send its result to admin.
	   Lock/Shutdown is successful if all SIs have been unassigned. Unlock is successful if
	   SI could be assigned to SU successfully if there was any. The operation failed if
	   AvND encountered error while assigning/unassigning SI to the SU. */

	su = su_db->find(Amf::to_string(&n2d_msg->msg_info.n2d_su_si_assign.su_name));

	if (su != NULL) {
		if (su->pend_cbk.invocation != 0) {
			if ((su->pend_cbk.admin_oper == SA_AMF_ADMIN_LOCK)
			    || (su->pend_cbk.admin_oper == SA_AMF_ADMIN_SHUTDOWN)) {
				if ((su->saAmfSUNumCurrActiveSIs == 0) && (su->saAmfSUNumCurrStandbySIs == 0)) {
					/* For lock and shutdown, response to IMM admin operation should be
					   sent when response for DEL operation is received */
					if (AVSV_SUSI_ACT_DEL == n2d_msg->msg_info.n2d_su_si_assign.msg_act)
						su->complete_admin_op(SA_AIS_OK);
				} else if (n2d_msg->msg_info.n2d_su_si_assign.error != NCSCC_RC_SUCCESS) {
					su->complete_admin_op(SA_AIS_ERR_REPAIR_PENDING);
				}
				/* else lock is still not complete so don't send result. */
			} else if (su->pend_cbk.admin_oper == SA_AMF_ADMIN_UNLOCK) {

				/* Respond to IMM when SG is STABLE or if a fault occured */
				if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
					if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
						su->complete_admin_op(SA_AIS_OK);
					} else
						; // wait for SG to become STABLE
				} 
				else
					su->complete_admin_op(SA_AIS_ERR_TIMEOUT);
			}
		} else if (su->su_on_node->admin_node_pend_cbk.invocation != 0) {
			/* decrement the SU count on the node undergoing admin operation  
			   when all SIs have been unassigned for a SU on the node undergoing 
			   LOCK/SHUTDOWN or when successful SI assignment has happened for 
			   a SU on the node undergoing UNLOCK */
			/* For lock and shutdown,su_cnt_admin_oper should be decremented when
			   response for DEL operation is received */
			if ((((su->su_on_node->admin_node_pend_cbk.admin_oper == SA_AMF_ADMIN_LOCK) ||
				(su->su_on_node->admin_node_pend_cbk.admin_oper == SA_AMF_ADMIN_SHUTDOWN)) &&
				(su->saAmfSUNumCurrActiveSIs == 0) && (su->saAmfSUNumCurrStandbySIs == 0) &&
				(AVSV_SUSI_ACT_DEL == n2d_msg->msg_info.n2d_su_si_assign.msg_act)) ||
				((su->su_on_node->admin_node_pend_cbk.admin_oper == SA_AMF_ADMIN_UNLOCK_INSTANTIATION) &&
				 (su->saAmfSUNumCurrActiveSIs == 0) && (su->saAmfSUNumCurrStandbySIs == 0)) ||
				((su->su_on_node->admin_node_pend_cbk.admin_oper == SA_AMF_ADMIN_UNLOCK))) {
				su->su_on_node->su_cnt_admin_oper--;
				TRACE("su_cnt_admin_oper:%u", su->su_on_node->su_cnt_admin_oper);
			}

			/* if this last su to undergo admin operation then report to IMM */
			if (su->su_on_node->su_cnt_admin_oper == 0) {
				avd_saImmOiAdminOperationResult(cb->immOiHandle,
								    su->su_on_node->admin_node_pend_cbk.invocation,
								    SA_AIS_OK);
				su->su_on_node->admin_node_pend_cbk.invocation = 0;
				su->su_on_node->admin_node_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
			} else if (n2d_msg->msg_info.n2d_su_si_assign.error != NCSCC_RC_SUCCESS) {
				report_admin_op_error(cb->immOiHandle, su->su_on_node->admin_node_pend_cbk.invocation,
						SA_AIS_ERR_REPAIR_PENDING, &su->su_on_node->admin_node_pend_cbk,
						NULL);
				su->su_on_node->su_cnt_admin_oper = 0;
			}
			/* else admin oper still not complete */
		} else if ((su->sg_of_su->sg_ncs_spec == false) && ((su->su_on_node->admin_ng != NULL) ||
					(su->sg_of_su->ng_using_saAmfSGAdminState == true))) {
			AVD_AMF_NG *ng = su->su_on_node->admin_ng;
			//Got response from AMFND for assignments decrement su_cnt_admin_oper.
			if ((ng != NULL) &&
				(((((ng->admin_ng_pend_cbk.admin_oper == SA_AMF_ADMIN_SHUTDOWN) ||
				(ng->admin_ng_pend_cbk.admin_oper == SA_AMF_ADMIN_LOCK)) &&
				(su->saAmfSUNumCurrActiveSIs == 0) && (su->saAmfSUNumCurrStandbySIs == 0) &&
				(AVSV_SUSI_ACT_DEL == n2d_msg->msg_info.n2d_su_si_assign.msg_act))) ||
					(ng->admin_ng_pend_cbk.admin_oper == SA_AMF_ADMIN_UNLOCK))) {
				su->su_on_node->su_cnt_admin_oper--;
				TRACE("node:'%s', su_cnt_admin_oper:%u", 
					su->su_on_node->name.value,su->su_on_node->su_cnt_admin_oper);
			}
			process_su_si_response_for_ng(su, SA_AIS_OK);
		} else {
			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				if ((su->sg_of_su->sg_redundancy_model == SA_AMF_N_WAY_REDUNDANCY_MODEL) && 
						(su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE)) {
					for (temp_su = su->sg_of_su->list_of_su; temp_su != NULL; 
							temp_su = temp_su->sg_list_su_next) {
						temp_su->complete_admin_op(SA_AIS_OK);
					}
				} else
					; // wait for SG to become STABLE
			}
		}
		/* also check for pending clm callback operations */ 
		if (su->su_on_node->clm_pend_inv != 0) {
			if((su->saAmfSUNumCurrActiveSIs == 0) && (su->saAmfSUNumCurrStandbySIs == 0)
					&& (su->list_of_susi == NULL))
				su->su_on_node->su_cnt_admin_oper--;
			if ((su->su_on_node->su_cnt_admin_oper == 0) && (su->list_of_susi == NULL)) {
				/* since unassignment of all SIs on this node has been done
				   now go on with the terminataion */
				/* clm admin lock/shutdown operations were on, so we need to reset 
				   node admin state.*/
				su->su_on_node->saAmfNodeAdminState = SA_AMF_ADMIN_UNLOCKED;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, su->su_on_node, AVSV_CKPT_AVND_ADMIN_STATE);
				clm_node_terminate(su->su_on_node);
			} else if (n2d_msg->msg_info.n2d_su_si_assign.error != NCSCC_RC_SUCCESS) {
				/* clm admin lock/shutdown operations were on, so we need to reset 
				   node admin state.*/
				su->su_on_node->saAmfNodeAdminState = SA_AMF_ADMIN_UNLOCKED;
				/* just report error to clm let CLM take the action */
				saClmResponse_4(cb->clmHandle, su->su_on_node->clm_pend_inv, SA_CLM_CALLBACK_RESPONSE_ERROR);
				su->su_on_node->clm_pend_inv = 0;
			} /* else wait for some more time */
		}
	}

	/* Check whether the node belonging to this su is disable and susi of all su got removed.
	   This is the case of node_failover/node_switchover. */
	if ((SA_AMF_OPERATIONAL_DISABLED == node->saAmfNodeOperState) && (true == node->recvr_fail_sw)) {

		/* We are checking only application components as on payload all ncs comp are in no_red model.
		   We are doing the same thing for controller also. */
		temp_su = node->list_of_su;
		while (temp_su) {
			if (NULL != temp_su->list_of_susi) {
				all_su_unassigned = false;
			}
			temp_su = temp_su->avnd_list_su_next;
		}
		if (true == all_su_unassigned) {
			/* All app su got unassigned, Safe to reboot the blade now. */
			node_try_repair(node);
		}
	}
	/* Free the messages */
	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;

 done:
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_sg_app_node_su_inst_func
 *
 * Purpose:  This function processes the request to instantiate all the
 *           application SUs on a node. If the
 *           AvD is in AVD_INIT_DONE state i.e the AMF timer hasnt expired
 *           all the pre-instantiable SUs will be instantiated by sending presence
 *           message. The non pre-instantiable SUs operation state will be made
 *           enable. If the state is AVD_APP_STATE, then for each SU the
 *           corresponding SG will be evaluated for instantiations.
 *
 * Input: cb - the AVD control block
 *        avnd - The pointer to the node whose application SUs need to
 *               be instantiated.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

void avd_sg_app_node_su_inst_func(AVD_CL_CB *cb, AVD_AVND *avnd)
{
	AVD_SU *i_su;

	TRACE_ENTER2("'%s'", avnd->name.value);

	if (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
		TRACE("Node is in SA_AMF_ADMIN_LOCKED_INSTANTIATION state, can't instantiate");
		goto done;
	}

	if (cb->init_state == AVD_INIT_DONE) {
		i_su = avnd->list_of_su;
		while (i_su != NULL) {
			if ((i_su->term_state == false) &&
			    (i_su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) &&
			    (i_su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) && 
			    (i_su->sg_of_su->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
			    (i_su->saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) && 
			    (i_su->su_on_node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION)) { 
				if (i_su->saAmfSUPreInstantiable == true) {
					if (i_su->sg_of_su->saAmfSGNumPrefInserviceSUs >
							(sg_instantiated_su_count(i_su->sg_of_su) +
							 i_su->sg_of_su->try_inst_counter)) {
						/* instantiate all the pre-instatiable SUs */
						if (avd_snd_presence_msg(cb, i_su, false) == NCSCC_RC_SUCCESS) {
							i_su->sg_of_su->try_inst_counter++;
						}
					}

				} else {
					i_su->set_oper_state(SA_AMF_OPERATIONAL_ENABLED);

					if (i_su->is_in_service() == true) {
						i_su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
					}
				}
			}

			i_su = i_su->avnd_list_su_next;
		}
		node_reset_su_try_inst_counter(avnd);

	} else if (cb->init_state == AVD_APP_STATE) {
		i_su = avnd->list_of_su;
		while (i_su != NULL) {
			if ((i_su->term_state == false) &&
			    (i_su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED)) {
				/* Look at the SG and do the instantiations. */
				avd_sg_app_su_inst_func(cb, i_su->sg_of_su);
			}

			i_su = i_su->avnd_list_su_next;
		}
	}

done:
	TRACE_LEAVE();
}

/**
 * @brief	This function finds higher rank unlocked, uninstantiated su. 
 * @param 	ptr to sg 
 * @param 	pointer to su
 * 
 */
AVD_SU* su_to_instantiate(AVD_SG *sg)
{
	for (AVD_SU* i_su = sg->list_of_su; i_su != NULL; i_su = i_su->sg_list_su_next) {
		TRACE("%s", i_su->name.value);
		if (i_su->is_instantiable())
			return i_su;
	}
	return NULL;
}

/**
 * @brief	This function finds lower rank unassigned, locked, intantiated su. 
 * @param 	ptr to sg 
 * @param 	pointer to su
 * 
 */
AVD_SU* su_to_terminate(AVD_SG *sg)
{
	AmfDb<std::string, AVD_SU> *su_rank = NULL;
	su_rank = new  AmfDb<std::string, AVD_SU>;
	for (AVD_SU* i_su = sg->list_of_su; i_su != NULL; i_su = i_su->sg_list_su_next) {
		TRACE("In Seq %s, %u", i_su->name.value, i_su->saAmfSURank);
		su_rank->insert(Amf::to_string(&i_su->name), i_su);
	}
	for (std::map<std::string, AVD_SU*>::const_reverse_iterator rit = su_rank->rbegin();
			rit != su_rank->rend(); ++rit) {
		AVD_SU *su = rit->second;
		TRACE("Rev %s, %u, %u, %u", su->name.value, su->saAmfSURank,
				su->saAmfSuReadinessState, su->saAmfSUPresenceState);
	}
	for (std::map<std::string, AVD_SU*>::const_reverse_iterator rit = su_rank->rbegin();
			rit != su_rank->rend(); ++rit) {
		AVD_SU *su = rit->second;
		TRACE("Rev 2 %s, %u, %u, %u", su->name.value, su->saAmfSURank,
				su->saAmfSuReadinessState, su->saAmfSUPresenceState);
		if ((su->saAmfSuReadinessState == SA_AMF_READINESS_OUT_OF_SERVICE) &&
				(su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATED) &&
				(su->list_of_susi == NULL)) {
			delete su_rank;
			return su;
		}
	}
	delete su_rank;
	return NULL;
}

/**
 * @brief	This function finds higher rank unlocked, uninstantiated su. 
 * @param 	ptr to sg 
 * @param 	pointer to su
 * 
 */
uint32_t in_serv_su(AVD_SG *sg)
{
	TRACE_ENTER();
	uint32_t in_serv = 0;
	for (AVD_SU* i_su = sg->list_of_su; i_su != NULL; i_su = i_su->sg_list_su_next) {
		TRACE_ENTER2("%s", i_su->name.value);
		if (i_su->is_in_service()) {
			TRACE_ENTER2(" in_serv_su %s", i_su->name.value);
			in_serv ++;
		}
	}
	TRACE_LEAVE2("%u", in_serv);
	return in_serv;
}

/*****************************************************************************
 * Function: avd_sg_app_su_inst_func
 *
 * Purpose:  This function processes the request to evaluate the SG for
 *           Instantiations and terminations of SUs in the SG. This routine
 *           is used only when AvD is in AVD_APP_STATE state i.e the AMF 
 *           timer has expired. It Instantiates all the high ranked 
 *           pre-instantiable SUs and for all non pre-instanble SUs if
 *           they are in uninstantiated state, the operation state will
 *           be made active. Once the preffered inservice SU count is
 *           meet all the instatiated,unassigned pre-instantiated SUs will
 *           be terminated. 
 *
 * Input: cb - the AVD control block
 *        sg - The pointer to SG whose SUs need to be instantiated.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: Only if the AvD is in AVD_APP_STATE this routine should be used.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_app_su_inst_func(AVD_CL_CB *cb, AVD_SG *sg)
{
	uint32_t num_insvc_su = 0;
	uint32_t num_asgd_su = 0;
	uint32_t num_su = 0;
	uint32_t num_try_insvc_su = 0;
	AVD_SU *i_su;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("'%s'", sg->name.value);

	i_su = sg->list_of_su;
	while (i_su != NULL) {
		su_node_ptr = i_su->get_node_ptr();
		num_su++;
		/* Check if the SU is inservice */
		if (i_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
			num_insvc_su++;
			if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
				num_asgd_su++;
			}
		} else {
			/* if the SU is non preinstantiable and if the node operational state
			 * is enable. Put the SU in service.
			 */
			if ((i_su->saAmfSUPreInstantiable == false) &&
			    (i_su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) &&
			    (su_node_ptr->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED) &&
			    (i_su->saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) &&
			    (i_su->term_state == false)) {

				if (i_su->is_in_service() == true) {
					i_su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
					i_su->sg_of_su->su_insvc(cb, i_su);

					if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
						num_asgd_su++;
					}
					num_insvc_su++;
				}

			} else if ((i_su->saAmfSUPreInstantiable == true) &&
					(i_su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) &&
					((i_su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) ||
					 (i_su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)) &&
					(i_su->sg_of_su->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
					(i_su->su_on_node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
					(su_node_ptr->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED) &&
					(su_node_ptr->node_info.member == true) &&
					(i_su->saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) &&
					(i_su->term_state == false)) {
				TRACE("%u, %u", sg->saAmfSGNumPrefInserviceSUs, num_try_insvc_su);
				if (sg->saAmfSGNumPrefInserviceSUs > (sg_instantiated_su_count(i_su->sg_of_su) +
							num_try_insvc_su)){
					/* Try to Instantiate this SU */
					if (avd_snd_presence_msg(cb, i_su, false) == NCSCC_RC_SUCCESS) {
						num_try_insvc_su++;
					}
				} else {
					/* Check whether in-serv su are sufficient. */
					if (sg->saAmfSGNumPrefInserviceSUs > in_serv_su(sg)) {
						/* Find most eligible SU(Higher Rank, Unlocked) to instantiate. */
						AVD_SU* su_inst = su_to_instantiate(sg);
						/* Find lower rank unassigned, locked, intantiated su to terminate. */
						AVD_SU* su_term = su_to_terminate(sg);
						TRACE("%p, %p", su_inst, su_term);
						if (su_inst && su_term) {
							TRACE("%s, %s", su_inst->name.value, su_term->name.value);
							/* Try to Instantiate this SU */
							if (avd_snd_presence_msg(cb, su_inst, false) == NCSCC_RC_SUCCESS) {
								/* Don't increment num_try_insvc_su as we are any way
								   going to terminate one SU. */;
								if (avd_snd_presence_msg(cb, su_term, true) ==
										NCSCC_RC_SUCCESS) {
									su_term->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
									num_insvc_su --;
								}
							}
						} else {
							/* No action to take if any su can't be instantiated or
							   if any su can't be terminated. */
						}
					}
				}
			} else
				TRACE("nop for %s", i_su->name.value);
		}
		/* else if (i_su->num_of_comp == i_su->curr_num_comp) */
		i_su = i_su->sg_list_su_next;

	}			/* while (i_su != AVD_SU_NULL) */

	/* The entire SG has been scanned for reinstatiations and terminations.
	 * Fill the numbers gathered into the SG.
	 */
	sg->saAmfSGNumCurrAssignedSUs = num_asgd_su;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, sg, AVSV_CKPT_SG_SU_ASSIGNED_NUM);

	sg->saAmfSGNumCurrInstantiatedSpareSUs = num_insvc_su - num_asgd_su;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, sg, AVSV_CKPT_SG_SU_SPARE_NUM);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_app_sg_admin_func
 *
 * Purpose:  This function processes the request to do UNLOCK or LOCK or shutdown
 * of the AMF application SUs on the SG. It first verifies that the
 * SGs is stable and then it sets the readiness 
 * state of each of the SU on the node and calls the SG FSM for the
 * SG.
 *
 * Input: cb - the AVD control block
 *        sg - The pointer to the sg which needs to be administratively modified.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_app_sg_admin_func(AVD_CL_CB *cb, AVD_SG *sg)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	AVD_SU *i_su;

	TRACE_ENTER2("'%s'", sg->name.value);

	/* Based on the admin operation that is been done call the corresponding.
	 * Redundancy model specific functionality for the SG.
	 */

	switch (sg->saAmfSGAdminState) {
	case SA_AMF_ADMIN_UNLOCKED:
		/* Dont allow UNLOCK if the SG FSM is not stable. */
		if (sg->sg_fsm_state != AVD_SG_FSM_STABLE) {
			LOG_NO("%s: Unstable SG fsm state:%u", __FUNCTION__, sg->sg_fsm_state);
			goto done;
		}

		/* For each of the SUs calculate the readiness state. This routine is called
		 * only when AvD is in AVD_APP_STATE. call the SG FSM with the new readiness
		 * state.
		 */
		for (i_su = sg->list_of_su; i_su != NULL; i_su = i_su->sg_list_su_next) {
			if (i_su->is_in_service() == true) {
				i_su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
			}
		}

		if (sg->realign(cb, sg) == NCSCC_RC_FAILURE) {
			/* set all the SUs to OOS return failure */

			for (i_su = sg->list_of_su; i_su != NULL; i_su = i_su->sg_list_su_next) {
				i_su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
			}

			goto done;
		}

		break;

	case SA_AMF_ADMIN_LOCKED:
	case SA_AMF_ADMIN_SHUTTING_DOWN:
		if ((sg->sg_fsm_state != AVD_SG_FSM_STABLE) && (sg->sg_fsm_state != AVD_SG_FSM_SG_ADMIN))
			goto done;

		if (sg->sg_admin_down(cb, sg) == NCSCC_RC_FAILURE) {
			goto done;
		}

		for (i_su = sg->list_of_su; i_su != NULL; i_su = i_su->sg_list_su_next) {
			i_su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		}
		break;

	default:
		LOG_ER("%s: invalid adm state %u", __FUNCTION__, sg->saAmfSGAdminState);
		goto done;
		break;
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_node_down_mw_susi_failover
 *
 * Purpose:  This function is called to un assign all the Mw SUSIs on
 * the node after the node is found to be down. This function Makes all 
 * the Mw SUs on the node as O.O.S and failover all the SUSI assignments based 
 * on their service groups. It will then delete all the SUSI assignments 
 * corresponding to the SUs on this node if any left.
 *
 * Input: cb - the AVD control block
 *        avnd - The AVND pointer of the node whose SU SI assignments need
 *                to be deleted.
 *
 * Returns: None.
 *
 * NOTES: none.
 * 
 **************************************************************************/

void avd_node_down_mw_susi_failover(AVD_CL_CB *cb, AVD_AVND *avnd)
{
	AVD_SU *i_su;

	TRACE_ENTER2("'%s'", avnd->name.value);

	/* run through all the MW SUs, make all of them O.O.S. Set
	 * assignments for the MW SGs of which the SUs are members. Also
	 * Set the operation state and presence state for the SUs and components to
	 * disable and uninstantiated.  All the functionality for MW SUs is done in
	 * one loop as more than one MW SU per SG in one node is not supported.
	 */
	i_su = avnd->list_of_ncs_su;
	osafassert(i_su != 0);
	while (i_su != NULL) {
		i_su->set_oper_state(SA_AMF_OPERATIONAL_DISABLED);
		i_su->set_pres_state(SA_AMF_PRESENCE_UNINSTANTIATED);
		i_su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		i_su->complete_admin_op(SA_AIS_ERR_TIMEOUT);
		i_su->disable_comps(SA_AIS_ERR_TIMEOUT);

		/* Now analyze the service group for the new HA state
		 * assignments and send the SU SI assign messages
		 * accordingly.
		 */
		i_su->sg_of_su->node_fail(cb, i_su);
		/* Mw 2N SU in stable state, that means clm track will not start
		   in avd_sg_2n_susi_sucss_sg_reln, so start here.*/
		if ((i_su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) &&
				(i_su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE))
			(void) avd_clm_track_start();
		/* If Std ctlr went down in middle of Cold sync, then we need
		   to reset the sync state to IN_SYNC. */
		if ((i_su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) &&
				(cb->stby_sync_state == AVD_STBY_OUT_OF_SYNC)) {
			TRACE("Marking sync_state as in_sync");
			cb->stby_sync_state = AVD_STBY_IN_SYNC;
		}
		/* Free all the SU SI assignments*/ 
		i_su->delete_all_susis();

		i_su = i_su->avnd_list_su_next;

	}			/* while (i_su != AVD_SU_NULL) */

	/* send pending callback for this node if any */
	if (avnd->admin_node_pend_cbk.invocation != 0) {
		LOG_WA("Response to admin callback due to node fail");
		report_admin_op_error(cb->immOiHandle, avnd->admin_node_pend_cbk.invocation,
				SA_AIS_ERR_REPAIR_PENDING, &avnd->admin_node_pend_cbk, "node failure");
		avnd->su_cnt_admin_oper = 0;
	}

	TRACE_LEAVE();
}

/**
 * @brief       This function is called to un assign all the Appl SUSIs on
 *              the node after the node is found to be down. This function Makes all
 *              the Appl SUs on the node as O.O.S and failover all the SUSI assignments 
 *              based on their service groups. It will then delete all the SUSI assignments
 *              corresponding to the SUs on this node if any left.
 * @param[in]   cb - Avd control Block
 *              Avnd - The AVND pointer of the node whose SU SI assignments need to be
 *              deleted.
 * @return      Returns nothing
 **/
void avd_node_down_appl_susi_failover(AVD_CL_CB *cb, AVD_AVND *avnd)
{
	AVD_SU *i_su;

	TRACE_ENTER2("'%s'", avnd->name.value);

	/* Run through the list of application SUs make all of them O.O.S. 
	 */
	i_su = avnd->list_of_su;
	while (i_su != NULL) {
		i_su->set_oper_state(SA_AMF_OPERATIONAL_DISABLED);
		i_su->set_pres_state(SA_AMF_PRESENCE_UNINSTANTIATED);
		i_su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		i_su->complete_admin_op(SA_AIS_ERR_TIMEOUT);
		i_su->disable_comps(SA_AIS_ERR_TIMEOUT);
		i_su = i_su->avnd_list_su_next;
	}

	/* If the AvD is in AVD_APP_STATE run through all the application SUs and 
	 * reassign all the SUSI assignments for the SG of which the SU is a member
	 */

	if (cb->init_state == AVD_APP_STATE) {
		i_su = avnd->list_of_su;
		while (i_su != NULL) {

			/* Unlike active, quiesced and standby HA states, assignment counters
			   in quiescing HA state are updated when AMFD receives assignment 
			   response from AMFND. During nodefailover amfd will not receive 
			   assignment response from AMFND. 
			   So if any SU is under going modify operation then update assignment 
			   counters for those SUSIs which are in quiescing state in the SU.
			 */ 
			for (AVD_SU_SI_REL *susi = i_su->list_of_susi; susi; susi = susi->su_next) {
				if ((susi->fsm == AVD_SU_SI_STATE_MODIFY) &&
						(susi->state == SA_AMF_HA_QUIESCING)) {
					avd_susi_update_assignment_counters(susi, AVSV_SUSI_ACT_MOD,
							SA_AMF_HA_QUIESCING, SA_AMF_HA_QUIESCED);
				}
				else if ((susi->fsm == AVD_SU_SI_STATE_MODIFY) &&
						(susi->state == SA_AMF_HA_ACTIVE)) {
					/* SUSI is undergoing active modification. For active state
					   saAmfSINumCurrActiveAssignments was increased when active
					   assignment had been sent. So decrement the count in SI before
					   deleting the SUSI. */
					susi->si->dec_curr_act_ass();
				}
				else if ((susi->fsm == AVD_SU_SI_STATE_MODIFY) &&
						(susi->state == SA_AMF_HA_STANDBY)) {
					/* SUSI is undergoing standby modification. For standby state
					   saAmfSINumCurrStandbyAssignments was increased when standby 
					   assignment had been sent. So decrement the count in SI before
					   deleting the SUSI. */
					susi->si->dec_curr_stdby_ass();
				}


			}

			/* Now analyze the service group for the new HA state
			 * assignments and send the SU SI assign messages
			 * accordingly.
			 */
			i_su->sg_of_su->node_fail(cb, i_su);

			/* Free all the SU SI assignments*/ 
			i_su->delete_all_susis();

			/* Since a SU has gone out of service relook at the SG to
			 * re instatiate and terminate SUs if needed.
			 */
			avd_sg_app_su_inst_func(cb, i_su->sg_of_su);

			i_su = i_su->avnd_list_su_next;

		}		/* while (i_su != AVD_SU_NULL) */

	}

	/* If this node-failover/nodereboot occurs dueing nodegroup operation then check 
	   if this leads to completion of operation and try to reply to imm.*/
	if ((avnd->list_of_su != NULL) && (avnd->admin_ng != NULL)) {
		avnd->su_cnt_admin_oper = 0;
		process_su_si_response_for_ng(avnd->list_of_su, SA_AIS_OK);
	}
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_sg_su_oper_list_add
 *
 * Purpose:  This function adds the SU to the list of SUs undergoing
 * operation. It allocates the holding structure.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU.
 *        ckpt - true - add is called from ckpt update.
 *               false - add is called from fsm.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_su_oper_list_add(AVD_CL_CB *cb, AVD_SU *su, bool ckpt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVD_SG_OPER **i_su_opr;

	TRACE_ENTER2("'%s'", su->name.value);

	/* Check that the current pointer in the SG is empty and not same as
	 * the SU to be added.
	 */
	if (su->sg_of_su->su_oper_list.su == NULL) {
		su->sg_of_su->su_oper_list.su = su;

		if (!ckpt)
			m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(cb, su, AVSV_CKPT_AVD_SG_OPER_SU);
		goto done;
	}

	if (su->sg_of_su->su_oper_list.su == su) {
		TRACE("already added");
		goto done;
	}

	i_su_opr = &su->sg_of_su->su_oper_list.next;
	while (*i_su_opr != NULL) {
		if ((*i_su_opr)->su == su) {
			TRACE("already added");
			goto done;
		}
		i_su_opr = &((*i_su_opr)->next);
	}

	/* Allocate the holder structure for having the pointer to the SU */
	*i_su_opr = new AVD_SG_OPER;

	/* Fill the content */
	(*i_su_opr)->su = su;
	(*i_su_opr)->next = NULL;

	if (!ckpt)
		m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(cb, su, AVSV_CKPT_AVD_SG_OPER_SU);

done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_su_oper_list_del
 *
 * Purpose:  This function deletes the SU from the list of SUs undergoing
 * operation. It frees the holding structure.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU.
 *        ckpt - true - add is called from ckpt update.
 *               false - add is called from fsm.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_su_oper_list_del(AVD_CL_CB *cb, AVD_SU *su, bool ckpt)
{
	AVD_SG_OPER **i_su_opr, *temp_su_opr;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("'%s'", su->name.value);

	if (su->sg_of_su->su_oper_list.su == NULL) {
		LOG_ER("%s: su_oper_list empty", __FUNCTION__);
		goto done;
	}

	if (su->sg_of_su->su_oper_list.su == su) {
		if (su->sg_of_su->su_oper_list.next != NULL) {
			temp_su_opr = su->sg_of_su->su_oper_list.next;
			su->sg_of_su->su_oper_list.su = temp_su_opr->su;
			su->sg_of_su->su_oper_list.next = temp_su_opr->next;
			temp_su_opr->next = NULL;
			temp_su_opr->su = NULL;
			delete temp_su_opr;
		} else {
			su->sg_of_su->su_oper_list.su = NULL;
		}

		if (!ckpt)
			m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su, AVSV_CKPT_AVD_SG_OPER_SU);
		goto done;
	}

	i_su_opr = &su->sg_of_su->su_oper_list.next;
	while (*i_su_opr != NULL) {
		if ((*i_su_opr)->su == su) {
			temp_su_opr = *i_su_opr;
			*i_su_opr = temp_su_opr->next;
			temp_su_opr->next = NULL;
			temp_su_opr->su = NULL;
			delete temp_su_opr;

			if (!ckpt)
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su, AVSV_CKPT_AVD_SG_OPER_SU);

			goto done;
		}

		i_su_opr = &((*i_su_opr)->next);
	}

	rc = NCSCC_RC_FAILURE;

done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_su_si_mod_snd
 *
 * Purpose:  This function is a utility function that assigns the state specified
 * to all the SUSIs that are assigned to the SU. If a failure happens it will
 * revert back to the orginal state.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU whose SUSIs needs to be modified.
 *        state - The HA state to which the state need to be modified. 
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This utility is used by 2N and N-way actice redundancy models.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_su_si_mod_snd(AVD_CL_CB *cb, AVD_SU *su, SaAmfHAStateT state)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	AVD_SU_SI_REL *i_susi;
	SaAmfHAStateT old_ha_state = SA_AMF_HA_ACTIVE;
	AVD_SU_SI_STATE old_state = AVD_SU_SI_STATE_ASGN;

	TRACE_ENTER2("'%s', state %u", su->name.value, state);

	/* change the state for all assignments to the specified state. */
	i_susi = su->list_of_susi;
	while (i_susi != AVD_SU_SI_REL_NULL) {

		if ((i_susi->fsm == AVD_SU_SI_STATE_UNASGN) ||
			((state == SA_AMF_HA_QUIESCED) && (i_susi->state == SA_AMF_HA_QUIESCED))) {
			/* Ignore the SU SI that are getting deleted 
			 * If the operation is quiesced modification ignore the susi that
			 * is already quiesced
			 */
			i_susi = i_susi->su_next;
			continue;
		}

		old_ha_state = i_susi->state;
		old_state = i_susi->fsm;

		i_susi->state = state;
		i_susi->fsm = AVD_SU_SI_STATE_MODIFY;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
		avd_susi_update_assignment_counters(i_susi, AVSV_SUSI_ACT_MOD, old_ha_state, state);

		i_susi = i_susi->su_next;
	}

	/* Now send a single message about the SU SI assignment to
	 * the AvND for all the SIs assigned to the SU.
	 */
	if (avd_snd_susi_msg(cb, su, AVD_SU_SI_REL_NULL, AVSV_SUSI_ACT_MOD, false, NULL) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: avd_snd_susi_msg failed, %s", __FUNCTION__, su->name.value);
		i_susi = su->list_of_susi;
		while (i_susi != AVD_SU_SI_REL_NULL) {

			if (i_susi->fsm == AVD_SU_SI_STATE_UNASGN) {
				/* Ignore the SU SI that are getting deleted. */
				i_susi = i_susi->su_next;
				continue;
			}

			i_susi->state = old_ha_state;
			i_susi->fsm = old_state;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
			i_susi = i_susi->su_next;
		}

		goto done;
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE();
	return rc;
}
/**
 * @brief	Does role modification to assignments in the SU based on dependency
 *
 * @param[in]   su
 *		ha_state
 *
 * @return	NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS 
 **/
uint32_t avd_sg_susi_mod_snd_honouring_si_dependency(AVD_SU *su, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *susi;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s', state %u", su->name.value, state);

	for (susi = su->list_of_susi; susi; susi = susi->su_next) {
		if (susi->state != SA_AMF_HA_QUIESCED) {
			if (avd_susi_quiesced_canbe_given(susi)) {
				rc = avd_susi_mod_send(susi, state);
				if (rc == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s", __FILE__, __LINE__, susi->su->name.value);
					break;
				}
			}
		}
	}
	TRACE_LEAVE2(":%d",rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_su_si_del_snd
 *
 * Purpose:  This function is a utility function that makes all the SUSIs that
 * are assigned to the SU as unassign. If a failure happens it will
 * revert back to the orginal state.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU whose SUSIs needs to be modified.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This utility is used by 2N and N-way actice redundancy models.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_su_si_del_snd(AVD_CL_CB *cb, AVD_SU *su)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	AVD_SU_SI_REL *i_susi;
	AVD_SU_SI_STATE old_state = AVD_SU_SI_STATE_ASGN;

	TRACE_ENTER2("'%s'", su->name.value);

	/* change the state for all assignments to the specified state. */
	i_susi = su->list_of_susi;
	while (i_susi != AVD_SU_SI_REL_NULL) {
		old_state = i_susi->fsm;
		if (i_susi->fsm != AVD_SU_SI_STATE_UNASGN) {
			i_susi->fsm = AVD_SU_SI_STATE_UNASGN;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
			/* Update the assignment counters */
			avd_susi_update_assignment_counters(i_susi, AVSV_SUSI_ACT_DEL, static_cast<SaAmfHAStateT>(0), static_cast<SaAmfHAStateT>(0));
		}
		i_susi = i_susi->su_next;
	}

	/* Now send a single delete message about the SU SI assignment to
	 * the AvND for all the SIs assigned to the SU.
	 */
	if (avd_snd_susi_msg(cb, su, AVD_SU_SI_REL_NULL, AVSV_SUSI_ACT_DEL, false, NULL) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: avd_snd_susi_msg failed, %s", __FUNCTION__, su->name.value);
		i_susi = su->list_of_susi;
		while (i_susi != AVD_SU_SI_REL_NULL) {
			i_susi->fsm = old_state;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
			i_susi = i_susi->su_next;
		}

		goto done;
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE();
	return rc;
}

/**
 * @brief       This routine does the following functionality
 *              a. Checks the dependencies of the SI's to see whether
 *                 role failover can be performed or not
 *              b. If so sends D2N-INFO_SU_SI_ASSIGN modify active all
 *                 to  the Stdby SU
 *
 * @param[in]   su
 *              stdby_su
 *
 * @return      Returns nothing
 **/
void avd_su_role_failover(AVD_SU *su, AVD_SU *stdby_su)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	bool flag;

	TRACE_ENTER2(" from SU:'%s' to SU:'%s'",su->name.value,stdby_su->name.value);

	/* Check if role failover can be performed for this SU */
	flag = avd_sidep_is_su_failover_possible(su);
	if (flag == true) {
		/* There is no dependency to perform rolefailover for this SU,
		 * So perform role failover
		 */
		rc = avd_sg_su_si_mod_snd(avd_cb, stdby_su, SA_AMF_HA_ACTIVE);
		if (rc == NCSCC_RC_SUCCESS) {
			/* Update the dependent SI's dep_state */
			avd_sidep_update_depstate_su_rolefailover(su);
		}
	}
	TRACE_LEAVE();
}

/**
 * @brief	This function completes admin operation on component. 
 *              It responds IMM with the result of admin operation on component.  
 * @param 	ptr to comp 
 * @param 	result
 * 
 */
void comp_complete_admin_op(AVD_COMP *comp, SaAisErrorT result)
{
	if (comp->admin_pend_cbk.invocation != 0) {
		avd_saImmOiAdminOperationResult(avd_cb->immOiHandle, comp->admin_pend_cbk.invocation, result);
		comp->admin_pend_cbk.invocation = 0;
		comp->admin_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
	}
}
