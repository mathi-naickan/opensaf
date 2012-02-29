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

  DESCRIPTION: This file is part of the SG processing module. It contains
  the SG state machine, for processing the events related to SG for 2N
  redundancy model.

******************************************************************************/

#include <logtrace.h>
#include <immutil.h>
#include <avd.h>
#include <avd_clm.h>
#include <avd_si_dep.h>

/*****************************************************************************
 * Function: avd_sg_2n_act_susi
 *
 * Purpose:  This function searches the entire SG to identify the
 * SUSIs belonging to a SI which has active and standby asssignments. 
 *
 * Input: cb - the AVD control block
 *        sg - The pointer to the service servive group.
 * stby_susi - pointer to the pointer of standby SUSI if present.
 *
 * Returns: AVD_SU_SI_REL * Pointer to active SUSI if present.
 *
 * NOTES: This is a 2N redundancy model specific function.
 *
 * 
 **************************************************************************/

static AVD_SU_SI_REL *avd_sg_2n_act_susi(AVD_CL_CB *cb, AVD_SG *sg, AVD_SU_SI_REL **stby_susi)
{

	bool l_flag = true;
	AVD_SU_SI_REL *susi;
	AVD_SU_SI_REL *a_susi = AVD_SU_SI_REL_NULL;
	AVD_SU_SI_REL *s_susi = AVD_SU_SI_REL_NULL;
	AVD_SI *l_si;

	TRACE_ENTER2("'%s'", sg->name.value);

	l_si = sg->list_of_si;

	/* check for configured SIs and see if they have been already
	 * assigned to SUs. Find the active and standby SUSI assignments. If any.
	 */

	while ((l_si != AVD_SI_NULL) && (l_flag == true)) {

		if (l_si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS) {
			l_si = l_si->sg_list_of_si_next;
			continue;
		}
			
		/* check to see if this SI has both the assignments */
		if ((susi = l_si->list_of_sisu) == AVD_SU_SI_REL_NULL) {
			l_si = l_si->sg_list_of_si_next;
			continue;
		}

		switch (susi->state) {
		case SA_AMF_HA_ACTIVE:
		case SA_AMF_HA_QUIESCING:
			/* Check if this SI has both active and standby assignments */
			a_susi = susi;
			if (susi->si_next != AVD_SU_SI_REL_NULL) {
				s_susi = susi->si_next;
				l_flag = false;
			}
			break;
		case SA_AMF_HA_STANDBY:
			/* Check if this SI has both active and standby assignments */
			s_susi = susi;
			if (susi->si_next != AVD_SU_SI_REL_NULL) {
				a_susi = susi->si_next;
				l_flag = false;
			}
			break;
		case SA_AMF_HA_QUIESCED:
			/* Check if this SI has both active and standby assignments */
			if (susi->si_next != AVD_SU_SI_REL_NULL) {
				if (susi->si_next->state == SA_AMF_HA_ACTIVE) {
					s_susi = susi;
					a_susi = susi->si_next;
				} else {
					a_susi = susi;
					s_susi = susi->si_next;
				}

				l_flag = false;
			} else {
				a_susi = susi;
			}
			break;
		default:
			break;

		}		/* switch(susi->state) */

		l_si = l_si->sg_list_of_si_next;

	}			/* while ((l_si != AVD_SI_NULL) && (l_flag == true)) */

	*stby_susi = s_susi;

	TRACE_LEAVE2("act: '%s', stdby: '%s'", a_susi ? a_susi->su->name.value : NULL,
		s_susi ? s_susi->su->name.value : NULL);
	return a_susi;
}

/*****************************************************************************
 * Function: avd_sg_2n_su_chose_asgn
 *
 * Purpose:  This function will identify the current active SU. 
 * If not available choose the first SU in the SG that is in-service.
 * Send D2N-INFO_SU_SI_ASSIGN message for the SU with role active for the SIs in
 * the SG list that dont have active assignment. If the active assignments meet
 * all the criteria identify the standby SU, If not available identify the next
 * in-service SU and send standby assignments for the SIs that have active
 * assignments. It returns the SU on which the assignment started, if no
 * assignments returns NULL.
 *
 * Input: cb - the AVD control block
 *        sg - The pointer to the service group.
 *        
 *
 * Returns: pointer to the SU that is undergoing assignment. Null if
 *          no assignments need to happen.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

static AVD_SU *avd_sg_2n_su_chose_asgn(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SU_SI_REL *a_susi;
	AVD_SU_SI_REL *s_susi;
	AVD_SU *a_su, *s_su, *return_su = NULL;
	AVD_SI *i_si;
	bool l_flag = true;
	AVD_SU_SI_REL *tmp_susi;

	TRACE_ENTER2("'%s'", sg->name.value);

	a_susi = avd_sg_2n_act_susi(cb, sg, &s_susi);

	if (a_susi == AVD_SU_SI_REL_NULL) {
		/* No active assignment exists. Scan the ranked list of SUs in the SG
		 * to identify a in-service SU 
		 */
		a_su = sg->list_of_su;
		while ((a_su != NULL) && (l_flag == true)) {
			if (a_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
				l_flag = false;
				continue;
			}

			a_su = a_su->sg_list_su_next;
		}

		if (a_su == NULL) {
			TRACE("No in service SUs available in the SG");
			goto done;
		}
	} else {		/* if (a_susi == AVD_SU_SI_REL_NULL) */

		a_su = a_susi->su;
	}

	if (a_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) {
		TRACE("The current active SU is OOS so return");
		goto done;
	}

	/* check if any more active SIs can be assigned to this SU */
	l_flag = false;

	/* choose and assign SIs in the SG that dont have active assignment */
	for (i_si = sg->list_of_si; i_si != NULL; i_si = i_si->sg_list_of_si_next) {
		/* Screen SI sponsors state and adjust the SI-SI dep state accordingly */
		avd_screen_sponsor_si_state(cb, i_si, false);

		if ((i_si->saAmfSIAdminState == SA_AMF_ADMIN_UNLOCKED) &&
		    (i_si->max_num_csi == i_si->num_csi) && (i_si->list_of_csi != NULL) &&
		    (i_si->si_dep_state != AVD_SI_SPONSOR_UNASSIGNED) &&
		    (i_si->si_dep_state != AVD_SI_UNASSIGNING_DUE_TO_DEP) &&
		    (i_si->list_of_sisu == AVD_SU_SI_REL_NULL)) {
			/* found a SI that needs active assignment. */
			if (avd_new_assgn_susi(cb, a_su, i_si, SA_AMF_HA_ACTIVE, false, &tmp_susi) ==
			    NCSCC_RC_SUCCESS) {
				l_flag = true;
			} else {
				LOG_ER("%s:%u: %s", __FILE__, __LINE__, i_si->name.value);
			}
		}
	}

	/* if any assignments have been done return the SU */
	if (l_flag == true) {
		return_su = a_su;
		goto done;
	}

	if (s_susi == AVD_SU_SI_REL_NULL) {
		/* No standby assignment exists. Scan the ranked list of SUs in the SG
		 * to identify a in-service SU with no assignments. 
		 */
		l_flag = true;
		s_su = sg->list_of_su;
		while ((s_su != NULL) && (l_flag == true)) {
			if ((s_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
			    (s_su->list_of_susi == AVD_SU_SI_REL_NULL)) {
				l_flag = false;
				continue;
			}

			s_su = s_su->sg_list_su_next;
		}

		if (s_su == NULL) {
			TRACE("No in service SUs available in the SG, that can be made standby");
			goto done;
		}
	} else {		/* if (s_susi == AVD_SU_SI_REL_NULL) */

		s_su = s_susi->su;
	}

	if (s_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) {
		TRACE("The current standby SU is OOS so return");
		goto done;
	}

	/* check if any more standby SIs can be assigned to this SU */
	l_flag = false;

	/* choose and assign SIs in the SG that have active assignment but dont
	 * have standby assignment.
	 */
	for (i_si = sg->list_of_si; i_si != NULL; i_si = i_si->sg_list_of_si_next) {
		if (i_si->list_of_sisu != AVD_SU_SI_REL_NULL) {
			/* found a SI that has active assignment. check if it has standby
			 * assignment. If not assign standby to this SU. 
			 */
			if (i_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL) {
				if (avd_new_assgn_susi(cb, s_su, i_si, SA_AMF_HA_STANDBY, false, &tmp_susi) ==
				    NCSCC_RC_SUCCESS) {
					l_flag = true;
				} else {
					LOG_ER("%s:%u: %s", __FILE__, __LINE__, i_si->name.value);
				}
			}
		}
	}

	/* if any assignments have been done return the SU */
	if (l_flag == true) {
		return_su = s_su;
		goto done;
	}

done:
	TRACE_LEAVE2("'%s'", return_su ? return_su->name.value : NULL);
	return return_su;
}

/*****************************************************************************
 * Function: avd_sg_2n_si_func
 *
 * Purpose:  This function is called when a new SI is added to a SG. The SG is
 * of type 2N redundancy model. This function will perform the functionality
 * described in the SG FSM design. 
 *
 * Input: cb - the AVD control block
 *        si - The pointer to the service instance.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This is a 2N redundancy model specific function. If there are
 * any SIs being transitioned due to operator, this call will return
 * failure.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_2n_si_func(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU *l_su;

	TRACE_ENTER2("'%s', %u", si->name.value, si->sg_of_si->sg_fsm_state);

	/* If the SG FSM state is not stable just return success. */
	if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
		LOG_NO("SG not stable");
		goto done;
	}

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == SA_FALSE)) {
		LOG_NO("%s: state: %u", __FUNCTION__, cb->init_state);
		goto done;
	}

	if ((l_su = avd_sg_2n_su_chose_asgn(cb, si->sg_of_si)) == NULL) {
		/* all the assignments have already been done in the SG. */
		goto done;
	}

	/* Add the SU to the list and change the FSM state */
	avd_sg_su_oper_list_add(cb, l_su, false);

	m_AVD_SET_SG_FSM(cb, (si->sg_of_si), AVD_SG_FSM_SG_REALIGN);

done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_2n_suswitch_func
 *
 * Purpose:  This function is called when a operator does a SI switch on
 * a SU that belongs to 2N redundancy model SG. 
 * This will trigger a role change action as described in the SG FSM design.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU that needs to be switched.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This is a 2N redundancy model specific function.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_2n_suswitch_func(AVD_CL_CB *cb, AVD_SU *su)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s', %u", su->name.value, su->sg_of_su->sg_fsm_state);

	/* Switch operation is not allowed when the AvD is not in
	 * application state, if su has no SI assignments and If the SG FSM state is
	 * not stable.
	 */
	if ((cb->init_state != AVD_APP_STATE) ||
	    (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) || (su->list_of_susi == AVD_SU_SI_REL_NULL)) {
		TRACE("state:%u, fsm:%u, list:%p", cb->init_state, su->sg_of_su->sg_fsm_state, su->list_of_susi);
		goto done;
	}

	/* switch operation is not valid on SUs that have only standby assignment
	 * or if the SG has no standby SU.
	 */

	if ((su->sg_of_su->saAmfSGNumCurrAssignedSUs != 2) || (su->list_of_susi->state == SA_AMF_HA_STANDBY)) {
		TRACE("saAmfSGNumCurrAssignedSUs:%u, state:%u",
			su->sg_of_su->saAmfSGNumCurrAssignedSUs, su->list_of_susi->state);
		goto done;
	}

	/* Modify all the SU SIs to quiesced state. */
	if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		goto done;
	}

	/* Add the SU to the operation list and change the SG state to SU_operation. */
	avd_sg_su_oper_list_add(cb, su, false);
	m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/**
 * This function is called when a operator does a SI swap admin operation on
 * an SU that belongs to 2N redundancy model SG. The 2N redundancy model requires
 * that all SIs assigned to this SU are swapped at the same time.
 * 
 * @param si
 * @param invocation
 * 
 * @return SaAisErrorT
 */
SaAisErrorT avd_sg_2n_siswap_func(AVD_SI *si, SaInvocationT invocation)
{
	AVD_SU_SI_REL *susi;
	SaAisErrorT rc = SA_AIS_OK;
        AVD_AVND *node;

	TRACE_ENTER2("'%s' sg_fsm_state=%u", si->name.value, si->sg_of_si->sg_fsm_state);

	if (si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) {
		LOG_ER("%s SWAP failed - wrong admin state=%u", si->name.value,
			si->saAmfSIAdminState);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	if (avd_cb->init_state != AVD_APP_STATE) {
		LOG_ER("%s SWAP failed - not in app state (%u)", si->name.value,
			avd_cb->init_state);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
		LOG_ER("%s SWAP failed - SG not stable (%u)", si->name.value,
			si->sg_of_si->sg_fsm_state);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	if (si->list_of_sisu == NULL) {
		LOG_ER("%s SWAP failed - no assignments to swap", si->name.value);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}
	
	if (si->list_of_sisu->si_next == NULL) {
		LOG_ER("%s SWAP failed - only one assignment", si->name.value);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

        /* Since middleware components can still have si->list_of_sisu->si_next as not NULL, but we need to check
           whether it is unlocked. We need to reject si_swap on controllers when stdby controller is locked. */
	if (si->sg_of_si->sg_ncs_spec) {
		/* Check if the Standby is there in unlocked state. */
		node = avd_node_find_nodeid(avd_cb->node_id_avd_other);
		if (node == NULL) {
			LOG_NO("SI Swap not possible, node %x is not available", avd_cb->node_id_avd_other);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
		if (SA_FALSE == node->node_info.member) {
			LOG_NO("SI Swap not possible, node %x is locked", avd_cb->node_id_avd_other);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
	}

	/* Identify the active susi rel */
	if (si->list_of_sisu->state == SA_AMF_HA_ACTIVE) {
		susi = si->list_of_sisu;
	} else if (si->list_of_sisu->si_next->state == SA_AMF_HA_ACTIVE) {
		susi = si->list_of_sisu->si_next;
	} else {
		LOG_ER("%s SWAP failed - no active assignment", si->name.value);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}
	/* Check if there is dependency between SI's within SU */
	if (avd_si_dependency_exists_within_su(susi->su)) {
		if (avd_sg_susi_mod_snd_honouring_si_dependency(susi->su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
			LOG_NO("%s:%u: %s ", __FILE__, __LINE__, susi->su->name.value);
			goto done;
		}
	} else {
		/* change the state for all assignments to quiesced. */
		if (avd_sg_su_si_mod_snd(avd_cb, susi->su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
			LOG_NO("%s:%u: %s ", __FILE__, __LINE__, susi->su->name.value);
			goto done;
		}
	}

	/* Add the SU to the operation list and change the SG state to SU_operation. */
	avd_sg_su_oper_list_add(avd_cb, susi->su, false);
	m_AVD_SET_SG_FSM(avd_cb, susi->su->sg_of_su, AVD_SG_FSM_SU_OPER);
	m_AVD_SET_SU_SWITCH(avd_cb, susi->su, AVSV_SI_TOGGLE_SWITCH);
	si->invocation = invocation;

	LOG_NO("%s Swap initiated", susi->si->name.value);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s Swap initiated", susi->si->name.value);

	if (susi->si->sg_of_si->sg_ncs_spec) {
		LOG_NO("Controller switch over initiated");
		saflog(LOG_NOTICE, amfSvcUsrName, "Controller switch over initiated");
		avd_cb->swap_switch = SA_TRUE;
	}

done:
	TRACE_LEAVE2("sg_fsm_state=%u", si->sg_of_si->sg_fsm_state);
	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_2n_su_fault_su_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_2n_su_fault_func.
 * It is called if the SG is in AVD_SG_FSM_SU_OPER.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the service unit.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

static uint32_t avd_sg_2n_su_fault_su_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU *a_su;
	bool flag;
	AVD_AVND *su_node_ptr = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;

	/* If the SU is same as the SU in the list and if the SI relationships to
	 * the SU is quiesced or  quiescing. If this SU admin is shutdown change
	 * to LOCK and send D2N-INFO_SU_SI_ASSIGN modify quiesced all. If this
	 * SU switch state is true change to false.
	 */
	TRACE_ENTER2("'%s'", su->name.value);

	if (su->sg_of_su->su_oper_list.su == su) {
		if (su->list_of_susi->state == SA_AMF_HA_QUIESCED) {
			m_AVD_SET_SU_SWITCH(cb, su, AVSV_SI_TOGGLE_STABLE);
		} else if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
			if (avd_si_dependency_exists_within_su(su)) {
				if (avd_sg_susi_mod_snd_honouring_si_dependency(su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_NO("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
					goto done;
				}
			} else {
				/* change the state for all assignments to quiesced. */
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_NO("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
					goto done;
				}
			}

			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}
		} else {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			LOG_ER("%s:%u: %u", __FILE__, __LINE__, su->list_of_susi->state);
			goto done;
		}
	} else {		/* if(su->sg_of_su->su_oper_list.su == su) */

		/* the SU is not the same as the SU in the list */
		if (su->list_of_susi->state == SA_AMF_HA_STANDBY) {
			/* the SU has standby SI assignments Send D2N-INFO_SU_SI_ASSIGN 
			 * remove all to this SU. Add the SU to the operation list. 
			 * Change state to SG_realign.
			 */

			/* change the state for all assignments to unassign and send delete mesage. */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

			/* add the SU to the operation list and change the SG FSM to SG realign. */
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);

			/* if the other SUs switch field is true, it is in service, 
			 * having quiesced assigning state Send D2N-INFO_SU_SI_ASSIGN modify 
			 * active all to the other SU.
			 */

			a_su = su->sg_of_su->su_oper_list.su;

			if ((a_su->su_switch == AVSV_SI_TOGGLE_SWITCH) &&
			    (a_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
			    (a_su->list_of_susi->state == SA_AMF_HA_QUIESCED)) {
				/* Got quiesced respponse for all susi, so start Active role modification based on dependency */
				if (avd_si_dependency_exists_within_su(su)) {
					AVD_SU_SI_REL *susi;
					for (susi = a_su->list_of_susi;susi != NULL;susi = susi->su_next) {
						if (avd_susi_role_failover(susi, su) == NCSCC_RC_FAILURE) {
							LOG_NO(" %s: %u: Active role modification failed for  %s ",
									__FILE__, __LINE__, susi->su->name.value);
							goto done;
						}
					}
				} else {
					/* There is no dependency between SI's within SU, so trigger SU level failover */
					avd_su_role_failover(su, a_su);
				}

				m_AVD_SET_SU_SWITCH(cb, a_su, AVSV_SI_TOGGLE_STABLE);
			}

		} /* if(su->list_of_susi->state == SA_AMF_HA_STANDBY) */
		else if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
			/* The SU has active SI assignments Send D2N-INFO_SU_SI_ASSIGN 
			 * modify quiesced to this SU. Change state to SG_realign. 
			 * Add this SU to the operation list.
			 */
			if (avd_si_dependency_exists_within_su(su)) {
				if (avd_sg_susi_mod_snd_honouring_si_dependency(su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_NO("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
					goto done;
				}
			} else {
				/* change the state for all assignments to quiesced. */
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_NO("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
					goto done;
				}
			}

			/* add the SU to the operation list and change the SG FSM to SG realign. */
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);

			/* If the other SUs switch field is true, it is in service, 
			 * having quiesced assigned state Change switch field to false.
			 */

			a_su = su->sg_of_su->su_oper_list.su;

			if ((a_su->su_switch == AVSV_SI_TOGGLE_SWITCH) &&
			    (a_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
			    (a_su->list_of_susi->state == SA_AMF_HA_QUIESCED)) {
				m_AVD_SET_SU_SWITCH(cb, a_su, AVSV_SI_TOGGLE_STABLE);
			} else if (a_su->list_of_susi->state == SA_AMF_HA_QUIESCED) {
				/* the other SU has quiesced assignments meaning either it is
				 * out of service or locked. So just send a remove request
				 * to that SU.
				 */
				avd_sg_su_si_del_snd(cb, a_su);
			}

		}
		/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
	}			/* else (su->sg_of_su->su_oper_list.su == su) */

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_2n_su_fault_si_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_2n_su_fault_func.
 * It is called if the SG is in AVD_SG_FSM_SI_OPER.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the service unit.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

static uint32_t avd_sg_2n_su_fault_si_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *l_susi, *o_susi;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s'", su->name.value);

	if (su->sg_of_su->admin_si->list_of_sisu->su == su) {
		l_susi = su->sg_of_su->admin_si->list_of_sisu;
		o_susi = su->sg_of_su->admin_si->list_of_sisu->si_next;
	} else {
		l_susi = su->sg_of_su->admin_si->list_of_sisu->si_next;
		o_susi = su->sg_of_su->admin_si->list_of_sisu;
	}

	if (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) {
		/* the SI switch operation is true */

		/* If this SI relation with the SU is quiesced assigning or this SI
		 * relation with the SU is quiesced assigned and the other SU is
		 * active assigning. Change the SI switch state to false. Remove the
		 * SI from the SI admin pointer. Add the SU to the SU operation list.
		 * Change state to SU_operation.
		 */
		if (((l_susi->state == SA_AMF_HA_QUIESCED) && (l_susi->fsm == AVD_SU_SI_STATE_MODIFY))
		    || ((l_susi->state == SA_AMF_HA_QUIESCED) && (l_susi->fsm == AVD_SU_SI_STATE_ASGND)
			&& (o_susi->state == SA_AMF_HA_ACTIVE) && (o_susi->fsm == AVD_SU_SI_STATE_MODIFY))) {
			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
		} else if ((l_susi->state == SA_AMF_HA_QUIESCED) && (l_susi->fsm == AVD_SU_SI_STATE_ASGND)
			   && (o_susi->state == SA_AMF_HA_STANDBY) && (o_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
			/* If this SI relation with the SU is quiesced assigned and the 
			 * other SU is standby assigned. Change the SI switch state to false.
			 * Remove the SI from the SI admin pointer. 
			 * Send D2N-INFO_SU_SI_ASSIGN modify active all to the other SU. 
			 * Add the SU to the operation list. Change state to SU_operation state.
			 */

			if (avd_sg_su_si_mod_snd(cb, o_susi->su, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, o_susi->su->name.value, o_susi->su->name.length);
				goto done;
			}
			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			avd_sg_su_oper_list_add(cb, o_susi->su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
		} else if ((l_susi->state == SA_AMF_HA_QUIESCED) && (l_susi->fsm == AVD_SU_SI_STATE_ASGND)
			   && (o_susi->state == SA_AMF_HA_ACTIVE) && (o_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
			/* If this SI relation with the SU is quiesced assigned and the 
			 * other SU is active assigned. Change the SI switch state to false.
			 * Remove the SI from the SI admin pointer. 
			 * Send D2N-INFO_SU_SI_ASSIGN remove all to the SU. Add that SU
			 * to operation list. Change state to SG_realign.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
		} else if ((l_susi->state == SA_AMF_HA_STANDBY) && (l_susi->fsm == AVD_SU_SI_STATE_ASGND)
			   && ((o_susi->state == SA_AMF_HA_QUIESCED) || (o_susi->state == SA_AMF_HA_QUIESCING))) {
			/* If this SI relation with the SU is standby assigned and the 
			 * other SU is quiesced/quiescing assigned/assigning. Send 
			 * D2N-INFO_SU_SI_ASSIGN with modified active all to the other SU.
			 * Change the switch value of the SI to false. Remove the SI from
			 * the admin pointer. Change state to SG_realign. Add that SU to
			 * operation list. Send D2N-INFO_SU_SI_ASSIGN with remove all to
			 * this SU. Add this SU to operation list.
			 */
			if (avd_sg_su_si_mod_snd(cb, o_susi->su, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, o_susi->su->name.value, o_susi->su->name.length);
				goto done;
			}

			avd_sg_su_si_del_snd(cb, su);

			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			avd_sg_su_oper_list_add(cb, o_susi->su, false);
			avd_sg_su_oper_list_add(cb, su, false);

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		} else if ((l_susi->state == SA_AMF_HA_ACTIVE) && (l_susi->fsm == AVD_SU_SI_STATE_MODIFY)
			   && (o_susi->state == SA_AMF_HA_QUIESCED) && (o_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
			/* If this SI relation with the SU is active assigning and the
			 * other SU is quiesced assigned. Change the switch value of the
			 * SI to false. Remove the SI from the admin pointer. 
			 * Send D2N-INFO_SU_SI_ASSIGN with modified quiesced all to this
			 * SU. Add this and the other SU to operation list. 
			 * Change state to SG_realign.
			 */

			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			avd_sg_su_oper_list_add(cb, o_susi->su, false);
			avd_sg_su_oper_list_add(cb, su, false);

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		} else {
			LOG_ER("%s:%u: %u", __FILE__, __LINE__, l_susi->state);
			goto done;
		}

	} /* if (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) */
	else if (su->sg_of_su->admin_si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED) {
		/* the SI admin operation is LOCK */

		if (l_susi != AVD_SU_SI_REL_NULL) {
			if (l_susi->state == SA_AMF_HA_QUIESCED) {
				/* this SI relation with the SU is quiesced assigning. Remove
				 * the SI from the SI admin pointer. Send D2N-INFO_SU_SI_ASSIGN
				 * with modified quiesced all to this SU. Add the SU to the 
				 * SU operation list. Change state to SU_operation state.
				 */
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_oper_list_add(cb, su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
			} else {
				/* The SU has standby assignments. Send D2N-INFO_SU_SI_ASSIGN
				 * with remove all to the SU. 
				 */
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}
			}
		} else {	/* if (l_susi != AVD_SU_SI_REL_NULL) */

			/* The SU has standby assignments. Send D2N-INFO_SU_SI_ASSIGN
			 * with remove all to the SU. 
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}
		}		/* else (l_susi != AVD_SU_SI_REL_NULL) */

	} else if (su->sg_of_su->admin_si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
		/* the SI admin operation is shutdown */
		if (l_susi != AVD_SU_SI_REL_NULL) {
			if (l_susi->state == SA_AMF_HA_QUIESCING) {
				/* this SI relation with the SU is quiescing assigning/assigned
				 * Change the SI admin state to unlock. Remove the SI from the 
				 * SI admin pointer. Send D2N-INFO_SU_SI_ASSIGN with modified 
				 * quiesced all to this SU. Add the SU to the SU operation list. 
				 * Change state to SU_operation state.
				 */

				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}
				avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_UNLOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_oper_list_add(cb, su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
			} else {
				/* The SU has standby assignments. Change the SI admin state to
				 * unlock. Remove the SI from the SI admin pointer. 
				 * Send D2N-INFO_SU_SI_ASSIGN modify active for the SI to 
				 * the other SU. Change state to SG_realign state. Add both the 
				 * SUs to the operation list. Send D2N-INFO_SU_SI_ASSIGN remove 
				 * all to this SU.
				 */
				if (avd_susi_mod_send(o_susi, SA_AMF_HA_ACTIVE) != NCSCC_RC_SUCCESS)
					goto done;

				avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_UNLOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_si_del_snd(cb, su);
				avd_sg_su_oper_list_add(cb, su, false);
				avd_sg_su_oper_list_add(cb, o_susi->su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		} else {	/* if (l_susi != AVD_SU_SI_REL_NULL) */

			/* The SU has standby assignments. Change the SI admin state to
			 * unlock. Remove the SI from the SI admin pointer. 
			 * Send D2N-INFO_SU_SI_ASSIGN modify active for the SI to 
			 * the other SU. Change state to SG_realign state. Add both the 
			 * SUs to the operation list. Send D2N-INFO_SU_SI_ASSIGN remove 
			 * all to this SU.
			 */
			if (avd_susi_mod_send(o_susi, SA_AMF_HA_ACTIVE) != NCSCC_RC_SUCCESS)
				goto done;

			avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_UNLOCKED);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			avd_sg_su_si_del_snd(cb, su);
			avd_sg_su_oper_list_add(cb, su, false);
			avd_sg_su_oper_list_add(cb, o_susi->su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* else (l_susi != AVD_SU_SI_REL_NULL) */

	} else {
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->admin_si->saAmfSIAdminState);
		goto done;
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_2n_su_fault_func
 *
 * Purpose:  This function is called when a SU readiness state changes to
 * OOS due to a fault. It will do the functionality specified in
 * SG FSM.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the service unit.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_2n_su_fault_func(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU *a_su;
	AVD_SU_SI_REL *l_susi, *o_susi;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s', %u", su->name.value, su->sg_of_su->sg_fsm_state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL) {
		LOG_NO("'%s' has no assignments", su->name.value);
		rc =  NCSCC_RC_SUCCESS;
		goto done;
	}

	/* Do the functionality based on the current state. */
	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
			/* this is a active SU. */
			if (avd_si_dependency_exists_within_su(su)) {
				if (avd_sg_susi_mod_snd_honouring_si_dependency(su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_NO("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
					goto done;
				}

			} else {
				/* change the state for all assignments to quiesced. */
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_NO("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
					goto done;
				}
			}

			/* add the SU to the operation list and change the SG FSM to SU operation. */
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
		} else {	/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

			/* means standby */
			/* change the state for all assignments to unassign and send delete mesage. */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

			/* add the SU to the operation list and change the SG FSM to SG realign. */
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* else (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:
		/* If the fault happened while role failover is under progress,then for all the dependent SI's
		 * for which si_dep_state is set to AVD_SI_FAILOVER_UNDER_PROGRESS reset the dependency to
		 * AVD_SI_SPONSOR_UNASSIGNED and the default flow will remove the assignments
		 */
		for (l_susi = su->list_of_susi;l_susi != NULL;l_susi = l_susi->su_next) {
			if(l_susi->si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS)
				si_dep_state_set(l_susi->si, AVD_SI_SPONSOR_UNASSIGNED);
			if (l_susi->si->num_dependents > 0)
				avd_sidep_reset_dependents_depstate_in_sufault(l_susi->si);
		}

		l_susi = su->list_of_susi;
		while ((l_susi != AVD_SU_SI_REL_NULL) && (l_susi->fsm == AVD_SU_SI_STATE_UNASGN)) {
			l_susi = l_susi->su_next;
		}

		if (l_susi == AVD_SU_SI_REL_NULL) {
			/* Do nothing as already the su si is being removed */
		} else if (l_susi->state == SA_AMF_HA_ACTIVE) {
			/* If (the SU has active assignment Send D2N-INFO_SU_SI_ASSIGN
			 * modify quiesced all. to the SU. Add the SU to the operation list.
			 */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}
			avd_sg_su_oper_list_add(cb, su, false);
		} else {
			/* the SU has standby assignment. send D2N-INFO_SU_SI_ASSIGN remove
			 * all to the SU. Add the SU to the operation list.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}
		}

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:
		if (avd_sg_2n_su_fault_su_oper(cb, su) == NCSCC_RC_FAILURE)
			goto done;

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:
		if (avd_sg_2n_su_fault_si_oper(cb, su) == NCSCC_RC_FAILURE)
			goto done;

		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:
		if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* the SG is lock no action. */
			return NCSCC_RC_SUCCESS;
		} else if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* the SI relationships to the SU is quiescing. Change admin state to
			 * lock, make the readiness state of all the SUs OOS.  Send 
			 * D2N-INFO_SU_SI_ASSIGN modify quiesced all to the SU. Identify its
			 * standby SU assignment send D2N-INFO_SU_SI_ASSIGN remove all for the
			 * SU. 
			 */
			if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}
				o_susi = AVD_SU_SI_REL_NULL;
				a_su = su->sg_of_su->list_of_su;
				while (a_su != NULL) {
					avd_su_readiness_state_set(a_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					if ((a_su->list_of_susi != AVD_SU_SI_REL_NULL)
					    && (a_su->list_of_susi->state == SA_AMF_HA_STANDBY)) {
						o_susi = a_su->list_of_susi;
					}
					a_su = a_su->sg_list_su_next;
				}
				avd_sg_admin_state_set(su->sg_of_su, SA_AMF_ADMIN_LOCKED);
				if (o_susi != AVD_SU_SI_REL_NULL)
					avd_sg_su_si_del_snd(cb, o_susi->su);
			} /* if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) */
			else {
				/* the SI relationships to the SU is standby Send 
				 * D2N-INFO_SU_SI_ASSIGN remove all for the SU.
				 */
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}

			}	/* else (su->list_of_susi->state == SA_AMF_HA_QUIESCING) */
		} else {
			LOG_ER("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->saAmfSGAdminState);
			goto done;
		}

		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_2n_su_insvc_func
 *
 * Purpose:  This function is called when a SU readiness state changes
 * to inservice from out of service. The SG is of type 2N redundancy
 * model. It will do the functionality specified in the SG FSM.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the service unit.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_2n_su_insvc_func(AVD_CL_CB *cb, AVD_SU *su)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVD_SU *l_su;

	TRACE_ENTER2("'%s', %u", su->name.value, su->sg_of_su->sg_fsm_state);

	/* An SU will not become in service when the SG is being locked or shutdown.
	 */
	if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_ADMIN) {
		LOG_ER("Wrong sg fsm state %u", su->sg_of_su->sg_fsm_state);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* If the SG FSM state is not stable just return success. */
	if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) {
		goto done;
	}

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == SA_FALSE)) {
		goto done;
	}

	if ((l_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) == NULL) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);

		/* all the assignments have already been done in the SG. */
		goto done;
	}

	/* Add the SU to the list and change the FSM state */
	avd_sg_su_oper_list_add(cb, l_su, false);

	m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_2n_susi_sucss_sg_reln
 *
 * Purpose:  This function is a subfunctionality of avd_sg_2n_susi_sucss_func.
 * It is called if the SG is in AVD_SG_FSM_SG_REALIGN.
 *
 * Input: cb - the AVD control block
 *        su - In case of entire SU related operation the SU for
 *               which the ack is received.
 *        susi - The pointer to the service unit service instance relationship.
 *        act  - The action received in the ack message.
 *        state - The HA state in the message.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

static uint32_t avd_sg_2n_susi_sucss_sg_reln(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					  AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *i_susi, *s_susi, *o_susi, *a_susi;
	AVD_SU *o_su, *l_su;
	bool flag, as_flag;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s' act=%u, state=%u", su->name.value, act, state);
	m_AVD_CHK_OPLIST(su, flag);

	if (flag == false) {
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		goto done;
	}

	if (susi == AVD_SU_SI_REL_NULL) {
		/* assign all */
		if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)) {
			a_susi = avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);

			/* quiesced all and SU is in the operation list */
			if (su->saAmfSuReadinessState == SA_AMF_READINESS_OUT_OF_SERVICE) {
				/* this SU is out of service Send a D2N-INFO_SU_SI_ASSIGN with 
				 * remove all to the SU. 
				 */
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}

				/* if the other SU is inservice with quiesced/standby assigned
				 * send D2N-INFO_SU_SI_ASSIGN with active all to that SU. 
				 * If the another SU in the operation list is OOS and assigned 
				 * Send a D2N-INFO_SU_SI_ASSIGN with remove all to the SU.
				 */
				o_su = NULL;
				o_susi = AVD_SU_SI_REL_NULL;
				if (a_susi->su != su) {
					o_su = a_susi->su;
					o_susi = a_susi;
					m_AVD_CHK_OPLIST(o_su, flag);
				} else if ((s_susi != AVD_SU_SI_REL_NULL) && (s_susi->su != su)) {
					o_su = s_susi->su;
					o_susi = s_susi;
					m_AVD_CHK_OPLIST(o_su, flag);
				}

				if ((o_su != NULL) && (flag == true) &&
				    (o_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
				    ((o_susi->state == SA_AMF_HA_QUIESCED) ||
				     (o_susi->state == SA_AMF_HA_STANDBY)) && (o_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
					avd_sg_su_si_mod_snd(cb, o_su, SA_AMF_HA_ACTIVE);
				} else if ((o_su != NULL) && (flag == true) &&
					   (o_su->saAmfSuReadinessState == SA_AMF_READINESS_OUT_OF_SERVICE)) {
					as_flag = true;
					i_susi = o_su->list_of_susi;
					while ((i_susi != AVD_SU_SI_REL_NULL) && (as_flag == true)) {
						if (i_susi->fsm != AVD_SU_SI_STATE_UNASGN)
							as_flag = false;

						i_susi = i_susi->su_next;
					}

					if (as_flag == false)
						avd_sg_su_si_del_snd(cb, su);
				}

			}
			/* if (su->readiness_state == NCS_OUT_OF_SERVICE) */
		} /* if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)) */
		else if (act == AVSV_SUSI_ACT_DEL) {
			o_su = NULL;

			/* SU in the operation list and remove all */
			if (su->sg_of_su->su_oper_list.su != su) {
				o_su = su->sg_of_su->su_oper_list.su;
			} else if (su->sg_of_su->su_oper_list.next != NULL) {
				o_su = su->sg_of_su->su_oper_list.next->su;
			}

			if (o_su != NULL) {
				as_flag = false;
				i_susi = o_su->list_of_susi;
				while (i_susi != AVD_SU_SI_REL_NULL) {
					if (i_susi->fsm != AVD_SU_SI_STATE_ASGND) {
						as_flag = true;
						break;
					}
					i_susi = i_susi->su_next;
				}

				if (as_flag == true) {
					/* another SU in the operation list with atleast one
					 * SI assignment not in assigned state. Free all the 
					 * SI assignments for the SU. Remove the SU from the
					 * operation list.
					 */

					avd_sg_su_asgn_del_util(cb, su, true, false);
					avd_sg_su_oper_list_del(cb, su, false);
				} else if ((o_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
					   (o_su->list_of_susi->state == SA_AMF_HA_ACTIVE)) {
					/* another inservice SU has assignment all assigned active .
					 * Free all the SI assignments for the SU.  Remove all the SUs
					 * from the operation list.
					 */
					avd_sg_su_asgn_del_util(cb, su, true, false);
					while (su->sg_of_su->su_oper_list.su != NULL) {
						avd_sg_su_oper_list_del(cb, su->sg_of_su->su_oper_list.su, false);
					}

					if ((l_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) == NULL) {
						/* all the assignments have already been done in the SG. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sg_app_su_inst_func(cb, su->sg_of_su);
					} else {
						/* Add the SU to the list  */
						avd_sg_su_oper_list_add(cb, l_su, false);
					}

				}	/* if ((o_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
					   (o_su->list_of_susi->state == SA_AMF_HA_ACTIVE)) */
				else if ((o_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
					 (o_su->list_of_susi->state == SA_AMF_HA_QUIESCED) &&
					 (o_su->list_of_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
					avd_sg_su_asgn_del_util(cb, su, true, false);
					avd_sg_su_oper_list_del(cb, su, false);
					avd_sg_su_si_mod_snd(cb, o_su, SA_AMF_HA_ACTIVE);
				}
				/* if ((o_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
				   (o_su->list_of_susi->state == SA_AMF_HA_QUIESCED) &&
				   (o_su->list_of_susi->fsm == AVD_SU_SI_STATE_ASGND)) */
			} else {	/* if (o_su != AVD_SU_NULL) */

				a_susi = avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);
				if (a_susi == AVD_SU_SI_REL_NULL) {
					o_su = NULL;
					o_susi = a_susi;
				} else if (a_susi->su != su) {
					o_su = a_susi->su;
					o_susi = a_susi;
				} else if ((s_susi != AVD_SU_SI_REL_NULL) && (s_susi->su != su)) {
					o_su = s_susi->su;
					o_susi = s_susi;
				}

				if (o_su != NULL) {
					if ((o_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
					    (o_su->list_of_susi->state == SA_AMF_HA_ACTIVE)) {
						/* another inservice SU has assignment all assigned active .
						 * Free all the SI assignments for the SU.  Remove all the SUs
						 * from the operation list.
						 */
						avd_sg_su_asgn_del_util(cb, su, true, false);
						while (su->sg_of_su->su_oper_list.su != NULL) {
							avd_sg_su_oper_list_del(cb, su->sg_of_su->su_oper_list.su, false);
						}

						if ((l_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) == NULL) {
							/* all the assignments have already been done in the SG. */
							m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
							avd_sg_app_su_inst_func(cb, su->sg_of_su);
						} else {
							/* Add the SU to the list  */
							avd_sg_su_oper_list_add(cb, l_su, false);
						}

					}	/* if ((o_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
						   (o_su->list_of_susi->state == SA_AMF_HA_ACTIVE)) */
					else if ((o_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
						 (o_su->list_of_susi->state == SA_AMF_HA_STANDBY)) {
						if (avd_sg_su_si_mod_snd(cb, o_su, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, o_su->name.value,
											 o_su->name.length);
							goto done;
						}

						avd_sg_su_asgn_del_util(cb, su, true, false);
						while (su->sg_of_su->su_oper_list.su != NULL) {
							avd_sg_su_oper_list_del(cb, su->sg_of_su->su_oper_list.su, false);
						}

						avd_sg_su_oper_list_add(cb, o_su, false);
					}	/* if ((o_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
						   (o_su->list_of_susi->state == SA_AMF_HA_STANDBY)) */
				} else {	/* if (o_su != AVD_SU_NULL) */

					/* no other SU has any assignments. */
					avd_sg_su_asgn_del_util(cb, su, true, false);
					while (su->sg_of_su->su_oper_list.su != NULL) {
						avd_sg_su_oper_list_del(cb, su->sg_of_su->su_oper_list.su, false);
					}

					if ((l_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) == NULL) {
						/* all the assignments have already been done in the SG. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sg_app_su_inst_func(cb, su->sg_of_su);
					} else {
						/* Add the SU to the list  */
						avd_sg_su_oper_list_add(cb, l_su, false);
					}

				}	/* else (o_su != AVD_SU_NULL) */

			}	/* else (o_su != AVD_SU_NULL) */

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_ACTIVE) || (state == SA_AMF_HA_STANDBY))) {
			/* Update IMM and send notification */
			for (i_susi = su->list_of_susi; i_susi != NULL; i_susi = i_susi->su_next) {
				avd_susi_update(i_susi, state);
				avd_gen_su_ha_state_changed_ntf(cb, i_susi);
			}

			/* active all or standby all. Remove the SU from the operation list. */
			avd_sg_su_oper_list_del(cb, su, false);

			if ((l_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) == NULL) {
				/* all the assignments have already been done in the SG. */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
			} else {
				/* Add the SU to the list  */
				avd_sg_su_oper_list_add(cb, l_su, false);
			}

			if ((state == SA_AMF_HA_ACTIVE) && (su->su_on_node->type == AVSV_AVND_CARD_SYS_CON) &&
			    (cb->node_id_avd == su->su_on_node->node_info.nodeId)) {
				/* This is as a result of failover, start CLM tracking*/
				(void) avd_clm_track_start();
			}
		} else {
			LOG_EM("%s:%u: %u", __FILE__, __LINE__, act);
			LOG_EM("%s:%u: %u", __FILE__, __LINE__, state);
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			goto done;
		}

	} /* if (susi == AVD_SU_SI_REL_NULL) */
	else {
		/* assign single SUSI */
		if (act == AVSV_SUSI_ACT_DEL) {
			/* remove for single SU SI. Free the SUSI relationship. If all the 
			 * relationships are in the assigned state, remove the SU from the 
			 * operation list and do the realignment of the SG.
			 */
			/* free all the CSI assignments  */
			avd_compcsi_delete(cb, susi, false);
			/* Unassign the SUSI */
			m_AVD_SU_SI_TRG_DEL(cb, susi);

			as_flag = false;
			i_susi = su->list_of_susi;
			while (i_susi != AVD_SU_SI_REL_NULL) {
				if (i_susi->fsm != AVD_SU_SI_STATE_ASGND) {
					as_flag = true;
					break;
				}
				i_susi = i_susi->su_next;
			}

			if (as_flag == false) {
				/* All are assigned. Remove the SU from the operation list. */
				avd_sg_su_oper_list_del(cb, su, false);

				if (su->sg_of_su->su_oper_list.su == NULL) {
					if ((l_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) == NULL) {
						/* all the assignments have already been done in the SG. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sg_app_su_inst_func(cb, su->sg_of_su);
					} else {
						/* Add the SU to the list  */
						avd_sg_su_oper_list_add(cb, l_su, false);
					}
				}
			}
			/* if (as_flag == false) */
		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if ((state == SA_AMF_HA_ACTIVE) || (state == SA_AMF_HA_STANDBY)) {
			as_flag = false;
			i_susi = su->list_of_susi;
			while (i_susi != AVD_SU_SI_REL_NULL) {
				if ((i_susi->si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS) || 
					(i_susi->fsm != AVD_SU_SI_STATE_ASGND)) {
					as_flag = true;
					break;
				}
				i_susi = i_susi->su_next;
			}

			if (as_flag == false) {
				/* All SUSI relationship are in assigned state. check for realiging
				 * the SG.
				 */
				/* All are assigned. Remove the SU from the operation list. */
				avd_sg_su_oper_list_del(cb, su, false);

				if (su->sg_of_su->su_oper_list.su == NULL) {
					if ((l_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) == NULL) {
						/* all the assignments have already been done in the SG. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sg_app_su_inst_func(cb, su->sg_of_su);
					} else {
						/* Add the SU to the list  */
						avd_sg_su_oper_list_add(cb, l_su, false);
					}

				}

			} else {
				if ((susi->state == SA_AMF_HA_ACTIVE) && (susi->si->num_dependents > 0))
					send_active_to_dependents(susi->si);
			}
			/* if (as_flag == false) */
		} /* if ((state == SA_AMF_HA_ACTIVE) || (state == SA_AMF_HA_STANDBY)) */
		else {
			LOG_EM("%s:%u: %u", __FILE__, __LINE__, state);
			LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			goto done;
		}
	}			/* else (susi == AVD_SU_SI_REL_NULL) */

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}
/**
 * @brief	  checks whether all assignments are quiesced or not 
 *
 * @param [in]	  su
 *
 * @returns	  true/false  
 */
static bool all_quiesced(const AVD_SU *su)
{
	AVD_SU_SI_REL *susi;

	for (susi = su->list_of_susi; susi; susi = susi->su_next) {
		if ((susi->state != SA_AMF_HA_QUIESCED) || (susi->fsm != AVD_SU_SI_STATE_ASGND))
			return false;
	}

	return true;
}
/**
 * @brief        checks whether all assignments are done on this su 
 *
 * @param [in]   su
 *
 * @returns      true/false  
 */
bool all_assignments_done(const AVD_SU *su)
{
	AVD_SU_SI_REL *susi;

	for (susi = su->list_of_susi;susi != NULL;susi = susi->su_next) {
		if (AVD_SU_SI_STATE_ASGND != susi->fsm)
			return false;
	}

	return true;
}
/**
 * @brief	  Quiesced role modifications has to be done in reverse order of si-si dependency.	  
 *		  When susi response is received for quiesced modification, this routine finds 
 *		  which is the next susi to be quiesced based on si-si dependency. 
 *
 * @param [in]	  susi for which we got the response 
 *
 * @returns	  pointer to AVD_SU_SI_REL 
 */
static AVD_SU_SI_REL *next_susi_tobe_quiesced(const AVD_SU_SI_REL *susi)
{
	AVD_SU_SI_REL *a_susi;
	AVD_SPONS_SI_NODE *spons_si_node;

	TRACE_ENTER2("'%s' '%s'", susi->si->name.value, susi->su->name.value);

	for (a_susi = susi->su->list_of_susi; a_susi; a_susi = a_susi->su_next) {
		if (a_susi->state == SA_AMF_HA_ACTIVE) {
			for (spons_si_node = susi->si->spons_si_list;spons_si_node;spons_si_node = spons_si_node->next) {
				if (spons_si_node->si == a_susi->si) {
					/* Check if quiesced response came for all of its dependents */ 
					if (quiesced_done_for_all_dependents(spons_si_node->si, susi->su)) {
						goto done;
					}
				}
			}
		}
	}
done:
	TRACE_LEAVE2("next_susi: %s",a_susi ? a_susi->si->name.value : NULL);
	return a_susi;
}
/**
 * @brief       Checks if any of the assignments has quiesced state 
 *		When checking if SU is in quiesced state, we need to check if any of the susi
 *		is in quiesced state or not, because in failure case there is possibility that
 *		only some assignments will be in quiesced state
 *
 * @param[in]   su 
 *
 * @return      true/false 
 **/
static bool quiesced_susi_in_su(AVD_SU *su)
{
	AVD_SU_SI_REL *susi;

	for (susi = su->list_of_susi; susi; susi = susi->su_next) {
		if ((susi->state == SA_AMF_HA_QUIESCED))
			return true;
	}
	return false;
}
/**
 * @brief       Checks if any of the assignments has quiescing state 
 *		When checking if SU is in quiescing state, we need to check if any of the susi
 *		is in quiescing state or not, because in failure case there is possibility that
 *		only some assignments will be in quiescing state
 *
 * @param[in]   su 
 *
 * @return      true/false 
 **/
static bool quiescing_susi_in_su(AVD_SU *su)
{
	AVD_SU_SI_REL *susi;

	for (susi = su->list_of_susi; susi; susi = susi->su_next) {
		if ((susi->state == SA_AMF_HA_QUIESCING))
			return true;
	}
	return false;
}
 /*****************************************************************************
 * Function: avd_sg_2n_susi_sucss_su_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_2n_susi_sucss_func.
 * It is called if the SG is in AVD_SG_FSM_SU_OPER.
 *
 * Input: cb - the AVD control block
 *        su - In case of entire SU related operation the SU for
 *               which the ack is received.
 *        susi - The pointer to the service unit service instance relationship.
 *        act  - The action received in the ack message.
 *        state - The HA state in the message.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

static uint32_t avd_sg_2n_susi_sucss_su_oper(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					  AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *s_susi, *a_susi, *l_susi, *n_susi, *s_susi_temp;
	AVD_SU *l_su;
	bool flag;
	AVD_AVND *su_node_ptr = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;
	SaAmfHAStateT ha_state;

	TRACE_ENTER2("'%s' act=%u, state=%u", su->name.value, act, state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL) {
		TRACE("no assignments");
		rc = NCSCC_RC_SUCCESS;
		goto done;
	}

	if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED) &&
		(su->sg_of_su->su_oper_list.su == su)) {
		/* quiesced all and SU is in the operation list */

		avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);
		if (susi != NULL) {
			if (all_quiesced(susi->su)) {
				if ((s_susi != AVD_SU_SI_REL_NULL) &&
					(s_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {
					/* Got quiesced respponse for all susi, so start Active role modification based on dependency */
					for (s_susi = s_susi->su->list_of_susi;s_susi != NULL;s_susi = s_susi->su_next) {
						if (avd_susi_role_failover(s_susi, su) == NCSCC_RC_FAILURE) {
							LOG_NO(" %s: %u: Active role modification failed for  %s ",
									__FILE__, __LINE__, s_susi->su->name.value);
							goto done;
						}
					}
				} else {
					if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
						goto done;
					}
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			} else {
				n_susi = next_susi_tobe_quiesced(susi);
				if (n_susi) {
					/* determine the modify-state for active sis */
					if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN)
						ha_state = SA_AMF_HA_QUIESCING;
					else
						ha_state = SA_AMF_HA_QUIESCED;

					rc = avd_susi_mod_send(n_susi, ha_state);
					if (rc == NCSCC_RC_FAILURE) {
						LOG_ER("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
						goto done;
					}
				}
			}
		} else {
			/* There is no dependency across the SI's within SU.Send a D2N-INFO_SU_SI_ASSIGN with
			 * modified active all to the standby SU.
			 */
			if ((s_susi != AVD_SU_SI_REL_NULL) &&
				(s_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {
				if ((su->su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) ||
						(su->su_on_node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)) {
					if (avd_sidep_is_su_failover_possible(su) == true) {
						if (avd_sg_su_si_mod_snd(cb, s_susi->su, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
							LOG_NO(" %s: %u: Active role modification failed for  %s ",
								__FILE__, __LINE__, s_susi->su->name.value);
							goto done;
						} else {
							/* Update the dependent SI's dep_state */
							avd_update_depstate_su_rolefailover(su);
						}
					}
				} else {
					if (avd_sg_su_si_mod_snd(cb, s_susi->su, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
						LOG_NO(" %s: %u: Active role modification failed for  %s ",
							__FILE__, __LINE__, s_susi->su->name.value);
						goto done;
					}
				}

				/*
				 ** Update IMM and send notification. Skip if we are executing controller
				 ** switch over. We currently have no active servers.
				 */
				if (!su->sg_of_su->sg_ncs_spec) {
					for (l_susi = su->list_of_susi; l_susi != NULL; l_susi = l_susi->su_next) {
						avd_susi_update(l_susi, state);
						avd_gen_su_ha_state_changed_ntf(cb, l_susi);
					}
				}
			} else {
				/* Send a D2N-INFO_SU_SI_ASSIGN with remove all to the SU. 
				 * Change state to SG_realign
				 */
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}

				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		}

		m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
		/* the admin state of the SU is shutdown change it to lock. */
		if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
		} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
			if (flag == true) {
				node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
			}
		}
	} else if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_ACTIVE)) {

		/* The message is assign active all */
		if ((su->sg_of_su->su_oper_list.su) && (su->sg_of_su->su_oper_list.su->su_switch == AVSV_SI_TOGGLE_SWITCH)) {
			/* If we are doing a controller switch over, send the quiesced 
			 * notification now that we have a new active.
			 */
			if (su->sg_of_su->sg_ncs_spec) {
				for (l_susi = su->sg_of_su->su_oper_list.su->list_of_susi;
				      l_susi != NULL; l_susi = l_susi->su_next) {
					avd_gen_su_ha_state_changed_ntf(cb, l_susi);
				}
			}

			/* the SU in the operation list has admin operation switch Send a 
			 * D2N-INFO_SU_SI_ASSIGN with modify all standby to the SU in
			 * operation list, Change the switch state to false for the SU.
			 * Change the state to SG_realign.
			 */
			if (susi) {
				if (susi->si->num_dependents > 0)
					send_active_to_dependents(susi->si);

				if(!all_assignments_done(su))
					goto done;
			}
			if (avd_sg_su_si_mod_snd(cb, su->sg_of_su->su_oper_list.su, SA_AMF_HA_STANDBY)
					== NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

			/* Update IMM and send notification */
			for (l_susi = su->list_of_susi; l_susi != NULL; l_susi = l_susi->su_next) {
				avd_susi_update(l_susi, state);
				avd_gen_su_ha_state_changed_ntf(cb, l_susi);
			}
		} else {
			if (susi) {
				if (susi->si->num_dependents > 0)
					send_active_to_dependents(susi->si);

				if(!all_assignments_done(su))
					goto done;
			}
			/* Update IMM and send notification */
			for (l_susi = su->list_of_susi; l_susi != NULL; l_susi = l_susi->su_next) {
				avd_susi_update(l_susi, state);
				avd_gen_su_ha_state_changed_ntf(cb, l_susi);
			}
			/* Send a D2N-INFO_SU_SI_ASSIGN with remove all to the
			 * SU in operation list. Change the state to SG_realign.
			 */
			if (avd_sg_su_si_del_snd(cb, su->sg_of_su->su_oper_list.su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}
	} else if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_STANDBY) &&
		   (su->sg_of_su->su_oper_list.su == su)) {

		/* Update IMM and send notification */
		for (l_susi = su->list_of_susi; l_susi != NULL; l_susi = l_susi->su_next) {
			avd_susi_update(l_susi, state);
			avd_gen_su_ha_state_changed_ntf(cb, l_susi);
		}

		/* Finish the SI SWAP admin operation */
		m_AVD_SET_SU_SWITCH(cb, su->sg_of_su->su_oper_list.su, AVSV_SI_TOGGLE_STABLE);
		avd_sg_su_oper_list_del(cb, su->sg_of_su->su_oper_list.su, false);
		m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_STABLE);
		/* find the SI on which SWAP admin operation is pending */
		for (l_susi = su->list_of_susi; l_susi != NULL && l_susi->si->invocation == 0; l_susi = l_susi->su_next);
		osafassert(l_susi != NULL);
		immutil_saImmOiAdminOperationResult(cb->immOiHandle, l_susi->si->invocation, SA_AIS_OK);
		l_susi->si->invocation = 0;
		LOG_NO("%s Swap done", l_susi->si->name.value);
		saflog(LOG_NOTICE, amfSvcUsrName, "%s Swap done", l_susi->si->name.value);

		if (su->sg_of_su->sg_ncs_spec)
			amfd_switch(avd_cb);

	} else if ((act == AVSV_SUSI_ACT_DEL) && (su->sg_of_su->su_oper_list.su != su)) {
		/* delete all and SU is not in the operation list. Free the SUSI 
		 * relationships for the SU. 
		 */

		avd_sg_su_asgn_del_util(cb, su, true, false);

	} else if ((act == AVSV_SUSI_ACT_DEL) && (su->sg_of_su->su_oper_list.su == su)) {
		/*remove all and SU is in the operation list */

		a_susi = avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);
		TRACE("a_susi si: %s Su %s state:%d",a_susi->si->name.value, a_susi->su->name.value,a_susi->state);
		/* In failure cases, there is possibility that only some of the assignments(susi's) will be in Quiesced or 
		 * Quiescing state, so when checking whether SU is in quiesced or quiescing assignment processing, we need
		 * to check if any one of the susi is in  Quiesced or Quiescing state
		 */
		if (((a_susi->su == su) &&
			((quiesced_susi_in_su(a_susi->su)) ||
			(quiescing_susi_in_su(a_susi->su)))) ||
			((s_susi != AVD_SU_SI_REL_NULL) && (s_susi->su == su) && (s_susi->state == SA_AMF_HA_QUIESCED))) {
			/*the SI relationships to the SU is quiesced or  quiescing */
			if ((s_susi != AVD_SU_SI_REL_NULL) &&
			    (s_susi->state == SA_AMF_HA_STANDBY) &&
			    (s_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {
				/* the standby SU is in-service. Send a D2N-INFO_SU_SI_ASSIGN with
				 * modified active all to the standby SU.
				 */
				/* Check if there is dependency between SI's within SU */
				if (avd_si_dependency_exists_within_su(su)) {
					for (s_susi_temp = s_susi->su->list_of_susi;s_susi_temp != NULL;s_susi_temp = s_susi_temp->su_next) {
						if (avd_susi_role_failover(s_susi_temp, su) == NCSCC_RC_FAILURE) {
							LOG_NO(" %s: %u: Active role modification failed for  %s ",
									__FILE__, __LINE__, s_susi->su->name.value);
							goto done;
						}
					}
				} else {
					/* There is no dependency between SI's within SU, so trigger SU level failover */
					avd_su_role_failover(su, s_susi->su);
				}
				/* Add that SU to operation list and Change state to SG_realign state. */ 
				avd_sg_su_oper_list_add(cb, s_susi->su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}	/* if ((s_susi != AVD_SU_SI_REL_NULL) && 
				   (s_susi->state == SA_AMF_HA_STANDBY) &&
				   (s_susi->su->readiness_state == NCS_IN_SERVICE)) */
			else if ((a_susi->state == SA_AMF_HA_ACTIVE) && (a_susi->su != su)) {
				/* An in-service active assigning SU. Add that SU to 
				 * operation list and Change state to SG_realign state.
				 */
				avd_sg_su_oper_list_add(cb, a_susi->su, false);

				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			} /* if((a_susi->state == SA_AMF_HA_ACTIVE) && (a_susi->su != su)) */
			else {
				/* single SU that failed while taking quiesced or 
				 * quiescing assignmenents. 
				 */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);

				/* realign the SG. */
			}

			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

			/* the admin state of the SU is shutdown change it to lock. */
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			} else if (su->su_switch == AVSV_SI_TOGGLE_SWITCH) {
				/* this SU switch state is true change to false. */
				m_AVD_SET_SU_SWITCH(cb, su, AVSV_SI_TOGGLE_STABLE);
			}

			/* Remove the SU from operation list */
			avd_sg_su_oper_list_del(cb, su, false);

		}		/* if (((a_susi->su == su) && 
				   ((a_susi->state == SA_AMF_HA_QUIESCED) ||
				   (a_susi->state == SA_AMF_HA_QUIESCING))) ||
				   ((s_susi != AVD_SU_SI_REL_NULL) && (s_susi->su == su) &&
				   (s_susi->state == SA_AMF_HA_QUIESCED))) */
		else {
			/* Log error remove shouldnt happen in this state other than 
			 * for quiesced SU SI assignment.
			 */
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			goto done;
		}

		/* Free all the SI assignments to this SU. */
		avd_sg_su_asgn_del_util(cb, su, true, false);

		if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
			if ((l_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) == NULL) {
				/* all the assignments have already been done in the SG. */
				rc =  NCSCC_RC_SUCCESS;
				goto done;
			}

			/* Add the SU to the list and change the FSM state */
			avd_sg_su_oper_list_add(cb, l_su, false);

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}

	} else {
		LOG_WA("%s: Unhandled case, %s act=%u, state=%u",
			__FUNCTION__, su->name.value, act, state);
		goto done;
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_2n_susi_sucss_si_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_2n_susi_sucss_func.
 * It is called if the SG is in AVD_SG_FSM_SI_OPER.
 *
 * Input: cb - the AVD control block
 *        su - In case of entire SU related operation the SU for
 *               which the ack is received.
 *        susi - The pointer to the service unit service instance relationship.
 *        act  - The action received in the ack message.
 *        state - The HA state in the message.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

static uint32_t avd_sg_2n_susi_sucss_si_oper(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					  AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *i_susi, *s_susi, *a_susi;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s', act:%u, state:%u", su->name.value, act, state);

	if (susi != AVD_SU_SI_REL_NULL) {
		if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)
		    && (susi->si == su->sg_of_su->admin_si)
		    && (susi->si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED)) {
			/* the message with modified quiesced for a SI in the admin pointer
			 * with admin values lock. Send a D2N-INFO_SU_SI_ASSIGN with remove
			 * for this SI to this SU. Remove the SI from the admin pointer. Add
			 * the SU to operation list also add the other SU to which this SI
			 * has assignment to the operation list. Change state to SG_realign.
			 */
			if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
				goto done;

			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			avd_sg_su_oper_list_add(cb, susi->su, false);

			if (susi->si_next != AVD_SU_SI_REL_NULL) {
				avd_sg_su_oper_list_add(cb, susi->si_next->su, false);
			}

			if (susi->si->list_of_sisu != susi) {
				avd_sg_su_oper_list_add(cb, susi->si->list_of_sisu->su, false);
			}

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)
				   && (susi->si == su->sg_of_su->admin_si)
				   && (susi->si->admin_state == NCS_ADMIN_STATE_LOCK)) */
		else if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)
			 && (susi->si == su->sg_of_su->admin_si)
			 && (susi->si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN)) {
			/* message with modified quiesced for a SI in the admin pointer with 
			 * admin values shutdown Send a D2N-INFO_SU_SI_ASSIGN with remove 
			 * for this SI to this SU. Change admin state to lock for this SI. 
			 * Identify its standby SU assignment send D2N-INFO_SU_SI_ASSIGN 
			 * remove for only this SI. Remove the SI from the admin pointer. 
			 *  Add the SU to operation list also add the other SU to which this
			 * SI has assignment to the operation list. Change state to SG_realign.
			 */

			if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
				goto done;

			avd_si_admin_state_set((susi->si), SA_AMF_ADMIN_LOCKED);

			avd_sg_su_oper_list_add(cb, susi->su, false);

			if (susi->si_next != AVD_SU_SI_REL_NULL) {
				avd_susi_del_send(susi->si_next);
				avd_sg_su_oper_list_add(cb, susi->si_next->su, false);
			}

			if (susi->si->list_of_sisu != susi) {
				avd_susi_del_send(susi->si->list_of_sisu);
				avd_sg_su_oper_list_add(cb, susi->si->list_of_sisu->su, false);
			}

			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)
				   && (susi->si == su->sg_of_su->admin_si)
				   && (susi->si->admin_state == NCS_ADMIN_STATE_SHUTDOWN)) */
		else if ((act == AVSV_SUSI_ACT_DEL) && (susi->state == SA_AMF_HA_STANDBY)) {
			/* message with remove for a SU SI relationship having standby value.
			 * Delete and free the SU SI relationship.
			 */

			avd_compcsi_delete(cb, susi, false);
			m_AVD_SU_SI_TRG_DEL(cb, susi);

			if ((su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
			    (su->list_of_susi->si == su->sg_of_su->admin_si) &&
			    (su->list_of_susi->su_next == AVD_SU_SI_REL_NULL)) {
				/* the admin pointer SI switch state is true and only the SI
				 * admin list SI has standby relation to the SU.
				 * Send D2N-INFO_SU_SI_ASSIGN with modified active assignment
				 * to the SU.
				 */
				avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_ACTIVE);
			}

		} /* if ((act == AVSV_SUSI_ACT_DEL) && (susi->state == SA_AMF_HA_STANDBY)) */
		else if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_ACTIVE) &&
			 (susi->si == su->sg_of_su->admin_si) && (susi->si->si_switch == AVSV_SI_TOGGLE_SWITCH)) {
			/* message with modified active for a SI in the admin pointer with
			 * switch value true.
			 */
			s_susi = AVD_SU_SI_REL_NULL;
			if (susi->si_next != AVD_SU_SI_REL_NULL) {
				s_susi = susi->si_next;
			} else if (susi->si->list_of_sisu != susi) {
				s_susi = susi->si->list_of_sisu;
			}

			if ((s_susi != AVD_SU_SI_REL_NULL) &&
			    ((s_susi->su_next != AVD_SU_SI_REL_NULL) || (s_susi->su->list_of_susi != s_susi))) {
				/* Send D2N-INFO_SU_SI_ASSIGN with remove for each of the 
				 * SIs except this SI to the quiesced SU.
				 */
				i_susi = s_susi->su->list_of_susi;

				while (i_susi != AVD_SU_SI_REL_NULL) {
					if (i_susi != s_susi) {
						avd_susi_del_send(i_susi);
					}
					i_susi = i_susi->su_next;
				}
			}	/* if ((s_susi != AVD_SU_SI_REL_NULL) && 
				   ((s_susi->su_next != AVD_SU_SI_REL_NULL) ||
				   (s_susi->su->list_of_susi != s_susi))) */
			else if (s_susi != AVD_SU_SI_REL_NULL) {
				/* Only the SI admin list SI has quiesced relation to the other SU 
				 * Send D2N-INFO_SU_SI_ASSIGN with modified standby assignment to 
				 * the SU. Change the SI switch state to false. Remove the SI from
				 * the admin list. Add the SU to the operation list. 
				 * Change state to SG_realign.
				 */
				if (avd_susi_mod_send(s_susi, SA_AMF_HA_STANDBY) != NCSCC_RC_SUCCESS)
					goto done;

				m_AVD_SET_SI_SWITCH(cb, (susi->si), AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_oper_list_add(cb, s_susi->su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			} /* if (s_susi != AVD_SU_SI_REL_NULL) */
			else {
				/* Integrity issue the standby is not there. */
				LOG_EM("%s:%u", __FILE__, __LINE__);
			}

		}		/* if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_ACTIVE) && 
				   (susi->si == su->sg_of_su->admin_si) && 
				   (susi->si->si_switch == AVSV_SI_TOGGLE_SWITCH)) */
		else if ((act == AVSV_SUSI_ACT_DEL) && (susi->state == SA_AMF_HA_QUIESCED)) {
			/* message with remove for a SU SI relationship having quiesced value
			 * Delete and free the SU SI relationship.
			 */

			avd_compcsi_delete(cb, susi, false);
			m_AVD_SU_SI_TRG_DEL(cb, susi);

			if ((su->list_of_susi->si == su->sg_of_su->admin_si) &&
			    (su->list_of_susi->su_next == AVD_SU_SI_REL_NULL)) {
				/* only the SI admin list SI has quiesced relation to the SU
				 * Send D2N-INFO_SU_SI_ASSIGN with modified standby assignment 
				 * to the SU. Change the SI switch state to false. Remove the 
				 * SI from the admin list. Add the SU to the operation list. 
				 * Change state to SG_realign.
				 */
				avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_STANDBY);

				m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_oper_list_add(cb, su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}

		} /* if ((act == AVSV_SUSI_ACT_DEL) && (susi->state == SA_AMF_HA_QUIESCED)) */
		else {
			LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
			LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		}

	} else {		/* if (susi != AVD_SU_SI_REL_NULL) */

		if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED) &&
		    (su->sg_of_su->admin_si != AVD_SI_NULL) &&
		    (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
		    ((a_susi = avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name))
		     != AVD_SU_SI_REL_NULL)) {
			/* message with modified quiesced all and an SI is in the admin
			 * pointer with switch value true
			 */
			s_susi = AVD_SU_SI_REL_NULL;
			if (a_susi->si_next != AVD_SU_SI_REL_NULL) {
				s_susi = a_susi->si_next;
			} else if (a_susi->si->list_of_sisu != a_susi) {
				s_susi = a_susi->si->list_of_sisu;
			}

			if ((s_susi != AVD_SU_SI_REL_NULL) &&
			    ((s_susi->su_next != AVD_SU_SI_REL_NULL) || (s_susi->su->list_of_susi != s_susi))) {
				/* Send D2N-INFO_SU_SI_ASSIGN with remove for each of the 
				 * SIs except this SI to the standby SU.
				 */
				i_susi = s_susi->su->list_of_susi;

				while (i_susi != AVD_SU_SI_REL_NULL) {
					if (i_susi != s_susi) {
						avd_susi_del_send(i_susi);
					}
					i_susi = i_susi->su_next;
				}
			}	/* if ((s_susi != AVD_SU_SI_REL_NULL) && 
				   ((s_susi->su_next != AVD_SU_SI_REL_NULL) ||
				   (s_susi->su->list_of_susi != s_susi))) */
			else if (s_susi != AVD_SU_SI_REL_NULL) {
				/* Only the SI admin pointer SI has standby relation to the SU.
				 * Send D2N-INFO_SU_SI_ASSIGN with modified active assignment
				 * to the SUSI.
				 */
				if (avd_susi_mod_send(s_susi, SA_AMF_HA_ACTIVE) != NCSCC_RC_SUCCESS)
					goto done;

			} /* if (s_susi != AVD_SU_SI_REL_NULL) */
			else {
				/* Integrity issue the standby is not there. */
				LOG_EM("%s:%u", __FILE__, __LINE__);
			}

		}		/* if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED) && 
				   (su->sg_of_su->admin_si != AVD_SI_NULL) && 
				   (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
				   ((a_susi = avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
				   != AVD_SU_SI_REL_NULL)) */
		else {
			LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
			goto done;
		}
	}			/* else (susi != AVD_SU_SI_REL_NULL) */

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_2n_susi_sucss_func
 *
 * Purpose:  This function is called when a SU SI ack function is
 * received from the AVND with success value. The SG FSM for 2N redundancy
 * model will be run. The SUSI fsm state will
 * be changed to assigned or it will freed for the SU SI. 
 * 
 *
 * Input: cb - the AVD control block
 *        su - In case of entire SU related operation the SU for
 *               which the ack is received.
 *        susi - The pointer to the service unit service instance relationship.
 *        act  - The action received in the ack message.
 *        state - The HA state in the message.
 * 
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This is a 2N redundancy model specific function.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_2n_susi_sucss_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
								   AVSV_SUSI_ACT act, SaAmfHAStateT state)
{

	AVD_SU_SI_REL *s_susi;
	AVD_SU *i_su, *a_su;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s' act=%u, hastate=%u, sg_fsm_state=%u", su->name.value, act,
				 state, su->sg_of_su->sg_fsm_state);

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		/* Do the action specified in the message if delete else no action. */
		if (act == AVSV_SUSI_ACT_DEL) {
			if (susi == AVD_SU_SI_REL_NULL) {
				avd_sg_su_asgn_del_util(cb, su, true, false);
			} else {
				/* free all the CSI assignments  */
				avd_compcsi_delete(cb, susi, false);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, susi);
			}
		}
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:
		if (avd_sg_2n_susi_sucss_sg_reln(cb, su, susi, act, state) == NCSCC_RC_FAILURE)
			goto done;

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:
		if (avd_sg_2n_susi_sucss_su_oper(cb, su, susi, act, state) == NCSCC_RC_FAILURE)
			goto done;

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:
		if (avd_sg_2n_susi_sucss_si_oper(cb, su, susi, act, state) == NCSCC_RC_FAILURE)
			goto done;

		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:
		if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* SG is admin lock */
			if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)) {
				/* quiesced all Send a D2N-INFO_SU_SI_ASSIGN with remove all for this SU. */
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}
			} else if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_DEL)) {
				/* Free all the SI assignments to this SU */
				avd_sg_su_asgn_del_util(cb, su, true, false);
				i_su = su->sg_of_su->list_of_su;
				while (i_su != NULL) {
					if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
						/* found a assigned su break */
						break;
					}
					i_su = i_su->sg_list_su_next;
				}

				if (i_su == NULL) {
					/* the SG doesnt have any SI assignments.Change state to stable. */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				}
			} /* if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_DEL)) */
			else {
				LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)act));
				LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

		} /* if (su->sg_of_su->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SG is admin shutdown */
			if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)) {
				/* quiesced all. Change admin state to lock and identify its 
				 * standby SU assignment send D2N-INFO_SU_SI_ASSIGN remove all
				 * for the SU. Send a D2N-INFO_SU_SI_ASSIGN with remove all for
				 * this SU.
				 */
				avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}

				if (s_susi != AVD_SU_SI_REL_NULL) {
					avd_sg_su_si_del_snd(cb, s_susi->su);
				}
				avd_sg_admin_state_set(su->sg_of_su, SA_AMF_ADMIN_LOCKED);
				a_su = su->sg_of_su->list_of_su;
				while (a_su != NULL) {
					avd_su_readiness_state_set(a_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					a_su = a_su->sg_list_su_next;
				}
			}	/* if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   (state == SA_AMF_HA_QUIESCED)) */
			else if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_DEL)) {
				/* remove all. Free all the SI assignments to this SU */
				avd_sg_su_asgn_del_util(cb, su, true, false);
			} else {
				LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)act));
				LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

		}
		/* if (su->sg_of_su->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		goto done;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_2n_susi_fail_func
 *
 * Purpose:  This function is called when a SU SI ack function is
 * received from the AVND with some error value. The message may be an
 * ack for a particular SU SI or for the entire SU. It will log an event
 * about the failure. Since if a CSI set callback returns error it is
 * considered as failure of the component, AvND would have updated that
 * info for each of the components that failed and also for the SU an
 * operation state message would be sent the processing will be done in that
 * event context. For faulted SU this event would be considered as
 * completion of action, for healthy SU no SUSI state change will be done. 
 * 
 *
 * Input: cb - the AVD control block
 *        su - In case of entire SU related operation the SU for
 *               which the ack is received.
 *        susi - The pointer to the service unit service instance relationship.
 *        act  - The action received in the ack message.
 *        state - The HA state in the message.
 *        
 *
 * Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This is a 2N redundancy model specific function.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_2n_susi_fail_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *s_susi, *o_susi, *l_susi;
	AVD_SU *a_su;
	bool flag;
	AVD_AVND *su_node_ptr = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s' act=%u, hastate=%u, sg_fsm_state=%u", su->name.value, act, state, su->sg_of_su->sg_fsm_state);

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		/* Do the action specified in the message if delete else no action. */
		if (act == AVSV_SUSI_ACT_DEL) {
			if (susi == AVD_SU_SI_REL_NULL) {
				avd_sg_su_asgn_del_util(cb, su, true, false);
			} else {
				/* free all the CSI assignments  */
				avd_compcsi_delete(cb, susi, false);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, susi);
			}
		}

		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
		    (state == SA_AMF_HA_QUIESCED) && (su->sg_of_su->su_oper_list.su == su)
		    && (su->saAmfSuReadinessState == SA_AMF_READINESS_OUT_OF_SERVICE)) {
			/* quiesced all and SU is OOS. Send a D2N-INFO_SU_SI_ASSIGN remove
			 * all to the SU.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}
		} else {
			/* No action as other call back failure will cause operation disable 
			 * event to be sent by AvND.
			 */
			TRACE("%u", state);
		}

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:
		if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
		    (state == SA_AMF_HA_QUIESCED) && (su->su_switch == AVSV_SI_TOGGLE_SWITCH)
		    && (su->sg_of_su->su_oper_list.su == su)) {
			/* quiesced all and SU is in the operation list and the SU switch
			 * state is true. Change the SU switch state to false. Send a 
			 * D2N-INFO_SU_SI_ASSIGN with modified active all for the SU. 
			 * Change the state to SG_realign.
			 */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

			m_AVD_SET_SU_SWITCH(cb, su, AVSV_SI_TOGGLE_STABLE);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			for (l_susi = su->list_of_susi; l_susi != NULL; l_susi = l_susi->su_next) {
				if (l_susi->si->invocation != 0) {
					immutil_saImmOiAdminOperationResult(cb->immOiHandle,
							l_susi->si->invocation, SA_AIS_ERR_BAD_OPERATION);
					l_susi->si->invocation = 0;
				}
			}

		} else if ((act == AVSV_SUSI_ACT_MOD) &&
			 ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING)) &&
			 (su->sg_of_su->su_oper_list.su == su)) {
			/* quiesced/quiescing all and SU is in the operation list and the 
			 * SU admin state is lock/shutdown. Change the SU admin state to lock.
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove all for the SU. Stay in the
			 * same state. 
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

			/* the admin state of the SU is shutdown change it to lock. */
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}
		}		/* if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING)) &&
				   (su->sg_of_su->su_oper_list.su == su)) */
		else {
			/* respond failed operation to IMM */
			for (l_susi = su->list_of_susi; l_susi != NULL; l_susi = l_susi->su_next) {
				if (l_susi->si->invocation != 0) {
					immutil_saImmOiAdminOperationResult(cb->immOiHandle,
							l_susi->si->invocation, SA_AIS_ERR_BAD_OPERATION);
					l_susi->si->invocation = 0;
				}
			}

			/* Other cases log a informational message. AvND would treat that as
			 * a operation disable for the SU and cause the SU to go OOS, which
			 * will trigger the FSM.
			 */
			TRACE("%s, act=%u, state=%u", su->name.value, act, state);
		}

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		if ((susi != AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
		    (state == SA_AMF_HA_QUIESCED) && (susi->si == su->sg_of_su->admin_si)
		    && (susi->si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED)) {
			/* message with modified quiesced for a SI in the admin pointer with 
			 * admin values lock. Send a D2N-INFO_SU_SI_ASSIGN with remove for 
			 * this SI to this SU. Remove the SI from the admin pointer. Add the 
			 * SU to operation list also add the other SU to which this SI has 
			 * assignment to the operation list. Change state to SG_realign.
			 */

			if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
				goto done;

			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			avd_sg_su_oper_list_add(cb, susi->su, false);

			o_susi = AVD_SU_SI_REL_NULL;

			if (susi->si_next != AVD_SU_SI_REL_NULL) {
				o_susi = susi->si_next;
			} else if (susi->si->list_of_sisu != susi) {
				o_susi = susi->si->list_of_sisu;
			}

			if (o_susi != AVD_SU_SI_REL_NULL) {
				avd_sg_su_oper_list_add(cb, o_susi->su, false);
			}

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* if ((susi != AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   (state == SA_AMF_HA_QUIESCED) && (susi->si == su->sg_of_su->admin_si)
				   && (susi->si->admin_state == NCS_ADMIN_STATE_LOCK)) */
		else if ((susi != AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
			 ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING)) &&
			 (susi->si == su->sg_of_su->admin_si)
			 && (susi->si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN)) {
			/* message with modified quiescing/quiesced for a SI in the  
			 * admin pointer with admin values shutdown. 
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove for this SI to this SU.   
			 * Change admin state to lock for this SI. Identify its standby 
			 * SU assignment send D2N-INFO_SU_SI_ASSIGN remove for only this SI. 
			 * Remove the SI from the admin pointer. Add the SU to operation list 
			 * also add the other SU to which this SI has assignment to the 
			 * operation list. Change state to SG_realign.  
			 */

			if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
				goto done;

			avd_si_admin_state_set((susi->si), SA_AMF_ADMIN_LOCKED);

			avd_sg_su_oper_list_add(cb, susi->su, false);

			if (susi->si_next != AVD_SU_SI_REL_NULL) {
				avd_susi_del_send(susi->si_next);
				avd_sg_su_oper_list_add(cb, susi->si_next->su, false);
			}

			if (susi->si->list_of_sisu != susi) {
				avd_susi_del_send(susi->si->list_of_sisu);
				avd_sg_su_oper_list_add(cb, susi->si->list_of_sisu->su, false);
			}

			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);

		}		/* if ((susi != AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING)) && 
				   (susi->si == su->sg_of_su->admin_si)
				   && (susi->si->admin_state == NCS_ADMIN_STATE_SHUTDOWN)) */
		else if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
			 (state == SA_AMF_HA_QUIESCED) && (su->sg_of_su->admin_si != AVD_SI_NULL)
			 && (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
			 (avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name)
			  != AVD_SU_SI_REL_NULL)) {
			/* Message with modified quiesced all and an SI is in the admin 
			 * pointer with switch value true.  Send D2N-INFO_SU_SI_ASSIGN with 
			 * modified active all to the SU. Change the switch value of the SI .
			 * to false. Remove the SI from the admin pointer. Add the SU to 
			 * operation list. Change state to SG_realign.
			 */

			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

			avd_sg_su_oper_list_add(cb, su, false);

			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   (state == SA_AMF_HA_QUIESCED) && (su->sg_of_su->admin_si != AVD_SI_NULL)
				   && (su->sg_of_su->admin_si == AVSV_SI_TOGGLE_SWITCH) &&
				   (avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false)
				   != AVD_SU_SI_REL_NULL)) */
		else {
			/* message with remove for a SU SI relationship having standby value.
			 * message with modified active for a SI in the admin pointer with
			 * switch value true; message with remove for a SU SI relationship
			 * having quiesced value. Ignore this message. It will trigger the
			 * SU to go disabled and an operation state message will be received
			 * for the SU.
			 */
			TRACE("%u", state);
		}

		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:

		if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
		    (state == SA_AMF_HA_QUIESCED) && (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED)) {
			/* SG is admin lock. Message is quiesced all. 
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove all for this SU.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

		}		/* if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   (state == SA_AMF_HA_QUIESCED) && 
				   (su->sg_of_su->admin_state == NCS_ADMIN_STATE_LOCK)) */
		else if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
			 ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING)) &&
			 (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN)) {
			/* SG is admin shutdown. The message is quiescing all or quiesced all.
			 * Change admin state to lock and identify its standby SU assignment 
			 * send D2N-INFO_SU_SI_ASSIGN remove all for the SU. 
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove all for this SU.
			 */
			avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);

			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}

			if (s_susi != AVD_SU_SI_REL_NULL) {
				avd_sg_su_si_del_snd(cb, su);
			}
			avd_sg_admin_state_set(su->sg_of_su, SA_AMF_ADMIN_LOCKED);
			a_su = su->sg_of_su->list_of_su;
			while (a_su != NULL) {
				avd_su_readiness_state_set(a_su, SA_AMF_READINESS_OUT_OF_SERVICE);
				a_su = a_su->sg_list_su_next;
			}

		}		/* if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   ((state == SA_AMF_HA_QUIESCED) && 
				   (su->sg_of_su->admin_state == NCS_ADMIN_STATE_LOCK)) */
		else {
			/* Other cases log a informational message. AvND would treat that as
			 * a operation disable for the SU and cause the SU to go OOS, which
			 * will trigger the FSM.
			 */
			TRACE("%u", state);
		}

		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		goto done;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_2n_realign_func
 *
 * Purpose:  This function will call the chose assign function to check and
 * assign SIs. If any assigning is being done it adds the SU to the operation
 * list and sets the SG FSM state to SG realign. It resets the ncsSGAdjustState.
 * If everything is 
 * fine, it calls the routine to bring the preffered number of SUs to 
 * inservice state and change the SG state to stable. The functionality is
 * described in the SG FSM. The same function is used for both cluster_timer and
 * and sg_operator events as described in the SG FSM.
 *
 * Input: cb - the AVD control block
 *        sg - The pointer to the service group.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_2n_realign_func(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SU *l_su;

	TRACE_ENTER2("'%s'", sg->name.value);

	/* If the SG FSM state is not stable just return success. */
	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == SA_FALSE)) {
		goto done;
	}

	if (sg->sg_fsm_state != AVD_SG_FSM_STABLE) {
		m_AVD_SET_SG_ADJUST(cb, sg, AVSV_SG_STABLE);
		avd_sg_app_su_inst_func(cb, sg);
		goto done;
	}

	if ((l_su = avd_sg_2n_su_chose_asgn(cb, sg)) == NULL) {
		/* all the assignments have already been done in the SG. */
		m_AVD_SET_SG_ADJUST(cb, sg, AVSV_SG_STABLE);
		avd_sg_app_su_inst_func(cb, sg);
		goto done;
	}

	/* Add the SU to the list and change the FSM state */
	avd_sg_su_oper_list_add(cb, l_su, false);

	m_AVD_SET_SG_ADJUST(cb, sg, AVSV_SG_STABLE);
	m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

 done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
static bool si_assignment_state_check(AVD_SI *si)
{
        AVD_SU_SI_REL *sisu;
        bool assignmemt_status = false;

        for (sisu = si->list_of_sisu;sisu;sisu = sisu->si_next) {
                if (((sisu->state == SA_AMF_HA_ACTIVE) || (sisu->state == SA_AMF_HA_QUIESCING)) &&
                                (sisu->fsm != AVD_SU_SI_STATE_UNASGN)) {
                        assignmemt_status = true;
                        break;
                }
        }       
                
        return assignmemt_status;
}
/**
 * @brief   	Checks whether all sponsors of an SI are in assigned state or not 
 *		Even if any one of the sponsor is in unassigned state, this routine
 *		returns NCSCC_RC_FAILURE else NCSCC_RC_SUCCESS
 *
 * @param[in]	SI 
 *
 * @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 **/
static bool all_sponsors_assigned_active(AVD_SI *si)
{
	bool all_assigned_active = true;
	AVD_SPONS_SI_NODE *spons_si;

	TRACE_ENTER2("SI:%s",si->name.value);

	if (si->spons_si_list == NULL) {
		goto done;
	}

	for (spons_si = si->spons_si_list; spons_si ;spons_si = spons_si->next) {
		if (si_assignment_state_check(spons_si->si) == false) {
			all_assigned_active = false;
			goto done;
		}
	}

done:
	TRACE_LEAVE2("all_assigned_active:%u",AVSV_CKPT_SI_DEP_STATE);
	return all_assigned_active;
}

/*****************************************************************************
 * Function: avd_sg_2n_node_fail_su_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_2n_node_fail_func.
 * It is called if the SG is in AVD_SG_FSM_SU_OPER.
 *
 * Input: cb - the AVD control block
 *        su - The SU that has faulted because of the node failure.
 *        
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

static void avd_sg_2n_node_fail_su_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *a_susi, *s_susi, *s_susi_temp;
	AVD_SU *o_su;
	bool flag;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER();

	if (su->sg_of_su->su_oper_list.su == su) {
		/* the SU is same as the SU in the list */
		a_susi = avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);
		/* If node fail happens while quiesced or qioescing role modification is in progress
		 * there is possibility that all the susi wont be in quiesced or quiescing state
		 * so when checking whether SU is in quiesced or quiescing assignment processing
		 * we need to check if any one of the susi is in  Quiesced or Quiescing state
		 */
		if ((quiesced_susi_in_su(su)) || (quiescing_susi_in_su(su))) {
			/* the SI relationships to the SU is quiesced assigning. If this 
			 * SU admin is shutdown change to LOCK. If this SU switch state
			 * is true change to false. Remove the SU from operation list
			 */

			if (s_susi != AVD_SU_SI_REL_NULL) {
				if (s_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
					/* Check if there is dependency between SI's within SU */
					if (avd_si_dependency_exists_within_su(su)) {
						for (s_susi_temp = s_susi->su->list_of_susi; s_susi_temp != NULL;
							s_susi_temp = s_susi_temp->su_next) {
							if (avd_susi_role_failover(s_susi_temp, su) == NCSCC_RC_FAILURE) {
								LOG_NO(" %s: %u: Active role modification failed for  %s ",
										__FILE__, __LINE__, s_susi_temp->su->name.value);
							}
						}
						avd_sg_su_oper_list_add(cb, s_susi->su, false);
					} else {

						/* Send D2N-INFO_SU_SI_ASSIGN  modify active all. */
						avd_su_role_failover(su, s_susi->su);
					}
				}

				avd_sg_su_oper_list_del(cb, su, false);
				avd_sg_su_asgn_del_util(cb, su, true, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			} else {	/* if (s_susi != AVD_SU_SI_REL_NULL) */

				avd_sg_su_oper_list_del(cb, su, false);
				avd_sg_su_asgn_del_util(cb, su, true, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				if ((o_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) != NULL) {
					/* add the SU to the operation list and change the SG FSM to SG realign. */
					avd_sg_su_oper_list_add(cb, o_su, false);
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			}	/* else (s_susi != AVD_SU_SI_REL_NULL) */

			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
			/* the admin state of the SU is shutdown change it to lock. */
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			} else {
				m_AVD_SET_SU_SWITCH(cb, su, AVSV_SI_TOGGLE_STABLE);
			}

		}		/* if (((su->list_of_susi->state == SA_AMF_HA_QUIESCED) &&
				   (su->list_of_susi->fsm == AVD_SU_SI_STATE_MODIFY)) ||
				   (su->list_of_susi->state == SA_AMF_HA_QUIESCING)) */
		else if (a_susi->su != su) {
			/* the SI relationships to the SU is quiesced assigned and the
			 * other SU is being modified to Active. If this 
			 * SU admin is shutdown change to LOCK. If this SU switch state
			 * is true change to false. Remove the SU from operation list.
			 * Add that SU to the operation list . Change state to 
			 * SG_realign state. Free all the SI assignments to this SU.
			 */

			avd_sg_su_oper_list_del(cb, su, false);
			avd_sg_su_asgn_del_util(cb, su, true, false);

			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

			/* the admin state of the SU is shutdown change it to lock. */
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			} else {
				m_AVD_SET_SU_SWITCH(cb, su, AVSV_SI_TOGGLE_STABLE);
			}
			avd_sg_su_oper_list_add(cb, a_susi->su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		} /* if (a_susi->su != su) */
		else {
			if (s_susi != AVD_SU_SI_REL_NULL) {
				if (s_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
					/* an in-service standby SU. Send D2N-INFO_SU_SI_ASSIGN 
					 * modify active all. Add that SU to the operation list.
					 * Change state to SG_realign state. Free all the SI 
					 * assignments to this SU.
					 */
					/* Check if there is dependency between SI's within SU */
                                        if (avd_si_dependency_exists_within_su(su)) {
                                                for (s_susi_temp = s_susi->su->list_of_susi; s_susi_temp != NULL;
                                                        s_susi_temp = s_susi_temp->su_next) {
                                                        if (avd_susi_role_failover(s_susi_temp, su) == NCSCC_RC_FAILURE) {
								LOG_NO(" %s: %u: Active role modification failed for  %s ",
										__FILE__, __LINE__, s_susi_temp->su->name.value);
                                                        }
                                                }
                                                avd_sg_su_oper_list_add(cb, s_susi->su, false);
                                        } else {

                                                /* Send D2N-INFO_SU_SI_ASSIGN  modify active all. */
                                                avd_su_role_failover(su, s_susi->su);
                                        }
				}

				avd_sg_su_oper_list_del(cb, su, false);
				avd_sg_su_asgn_del_util(cb, su, true, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			} else {	/* if (s_susi != AVD_SU_SI_REL_NULL) */

				avd_sg_su_oper_list_del(cb, su, false);
				avd_sg_su_asgn_del_util(cb, su, true, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
			}
		}
	} else {		/* if(su->sg_of_su->su_oper_list.su == su) */

		/* the SU is not the same as the SU in the list */
		if (su->list_of_susi->state == SA_AMF_HA_STANDBY) {
			/* the SU has standby SI assignments. if the other SUs switch field
			 * is true, it is in service, having quiesced assigning state.
			 * Send D2N-INFO_SU_SI_ASSIGN modify active all to the other SU.
			 * Change switch field to false. Change state to SG_realign. 
			 * Free all the SI assignments to this SU.
			 */
			if ((su->sg_of_su->su_oper_list.su->su_switch == AVSV_SI_TOGGLE_SWITCH)
			    && (su->sg_of_su->su_oper_list.su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {

				/* Check if there is dependency between SI's within SU */
				if (avd_si_dependency_exists_within_su(su->sg_of_su->su_oper_list.su)) {
					for (s_susi_temp = su->sg_of_su->su_oper_list.su->list_of_susi; s_susi_temp != NULL;
							s_susi_temp = s_susi_temp->su_next) {
						if (avd_susi_role_failover(s_susi_temp, su) == NCSCC_RC_FAILURE) {
							LOG_NO(" %s: %u: Active role modification failed for  %s ",
									__FILE__, __LINE__, s_susi_temp->su->name.value);
						}
					}
					avd_sg_su_oper_list_add(cb, su->sg_of_su->su_oper_list.su, false);
				} else {

					/* Send D2N-INFO_SU_SI_ASSIGN  modify active all. */
					avd_su_role_failover(su, su->sg_of_su->su_oper_list.su);
				}

				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}

			m_AVD_SET_SU_SWITCH(cb, (su->sg_of_su->su_oper_list.su), AVSV_SI_TOGGLE_STABLE);
			avd_sg_su_asgn_del_util(cb, su, true, false);

		} /* if(su->list_of_susi->state == SA_AMF_HA_STANDBY) */
		else if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
			/* the SU has active SI assignments. The other SUs switch field 
			 * is true, it is in service, having quiesced assigned state.
			 * Send D2N-INFO_SU_SI_ASSIGN modify active all to the other SU. 
			 * Change switch field to false. Change state to SG_realign. 
			 * Free all the SI assignments to this SU.
			 */

			if ((su->sg_of_su->su_oper_list.su->su_switch == AVSV_SI_TOGGLE_SWITCH)
			    && (su->sg_of_su->su_oper_list.su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {

				/* Check if there is dependency between SI's within SU */
				if (avd_si_dependency_exists_within_su(su->sg_of_su->su_oper_list.su)) {
					for (s_susi_temp = su->sg_of_su->su_oper_list.su->list_of_susi; s_susi_temp != NULL;
							s_susi_temp = s_susi_temp->su_next) {
						if (avd_susi_role_failover(s_susi_temp, su) == NCSCC_RC_FAILURE) {
							LOG_NO(" %s: %u: Active role modification failed for  %s ",
									__FILE__, __LINE__, s_susi_temp->su->name.value);
						}
					}
					avd_sg_su_oper_list_add(cb, su->sg_of_su->su_oper_list.su, false);
				} else {

					/* Send D2N-INFO_SU_SI_ASSIGN  modify active all. */
					avd_su_role_failover(su, su->sg_of_su->su_oper_list.su);
				}

				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			} else {
				avd_sg_su_si_del_snd(cb, su->sg_of_su->su_oper_list.su);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}

			m_AVD_SET_SU_SWITCH(cb, (su->sg_of_su->su_oper_list.su), AVSV_SI_TOGGLE_STABLE);
			avd_sg_su_asgn_del_util(cb, su, true, false);

		}
		/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
	}			/* else (su->sg_of_su->su_oper_list.su == su) */

	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_sg_2n_node_fail_si_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_2n_node_fail_func.
 * It is called if the SG is in AVD_SG_FSM_SI_OPER.
 *
 * Input: cb - the AVD control block
 *        su - The SU that has faulted because of the node failure.
 *        
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

static void avd_sg_2n_node_fail_si_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *a_susi, *s_susi;
	AVD_SU *o_su;

	TRACE_ENTER();

	if (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) {
		/* the SI switch operation is true */
		a_susi = avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);

		if ((a_susi->su == su) && (a_susi->state == SA_AMF_HA_QUIESCED)) {
			/* This SI relation with the SU is quiesced assigning or this SI 
			 * relation with the SU is quiesced assigned and the other SU is
			 * standby assigned, other SIs wrt to the SUs are being removed.
			 * Change the SI switch state to false. Remove the SI from the SI
			 * admin pointer. Mark all the SI relationships to this SU as
			 * unassigning. 
			 */

			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			if (s_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
				/* in-service standby SU. Send D2N-INFO_SU_SI_ASSIGN modify
				 * active all. Add that SU to operation list. Change state 
				 * to SG_realign state. Free all the SI assignments to this SU.
				 */
				avd_su_role_failover(su, s_susi->su);

			}

			avd_sg_su_asgn_del_util(cb, su, true, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		} /* if ((a_susi->su == su) && (a_susi->state == SA_AMF_HA_QUIESCED)) */
		else if ((a_susi->su != su) && (a_susi->state == SA_AMF_HA_ACTIVE) &&
			 (a_susi->fsm != AVD_SU_SI_STATE_ASGND)) {
			/* this SI relation with the SU is quiesced assigned and the other
			 * SU is active assigning. 
			 * Change the SI switch state to false. Remove the SI from the
			 * SI admin pointer. Mark all the SI relationships to this SU as
			 * unassigning. Add the other SU to operation list. Change 
			 * state to SG_realign state. Free all the SI assignments to this SU.
			 */
			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			avd_sg_su_asgn_del_util(cb, su, true, false);
			avd_sg_su_oper_list_add(cb, a_susi->su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* if ((a_susi->su != su) && (a_susi->state == SA_AMF_HA_ACTIVE) &&
				   (a_susi->fsm != AVD_SU_SI_STATE_ASGND)) */
		else if ((a_susi->su != su) && (a_susi->state == SA_AMF_HA_ACTIVE)) {
			/* This SI relation with the SU is quiesced assigned and the other
			 * SU is active assigned. Change the SI switch state to false. 
			 * Remove the SI from the SI admin pointer. Free all the 
			 * SI assignments to this SU. Do the realignment of the SG.
			 */
			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			avd_sg_su_asgn_del_util(cb, su, true, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
			if ((o_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) != NULL) {
				/* add the SU to the operation list and change the SG FSM to SG realign. */
				avd_sg_su_oper_list_add(cb, o_su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		} /* if ((a_susi->su != su) && (a_susi->state == SA_AMF_HA_ACTIVE)) */
		else if ((s_susi->su == su) && (s_susi->state == SA_AMF_HA_STANDBY)) {
			/* This SI relation with the SU is standby assigned and the
			 * other SU is quiesced assigned/assigning. Send 
			 * D2N-INFO_SU_SI_ASSIGN with modified active all to the other SU,
			 * if inservice. Change the switch value of the SI to false. Remove
			 * the SI from the admin pointer. Add that SU to operation list. 
			 * Change state to SG_realign. Free all the SI assignments to
			 * this SU.
			 */
			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			if (a_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
				/* in-service quiesced SU. Send D2N-INFO_SU_SI_ASSIGN modify
				 * active all. Add that SU to operation list. Change state 
				 * to SG_realign state. Free all the SI assignments to this SU.
				 */
				avd_su_role_failover(su, a_susi->su);

			}

			avd_sg_su_asgn_del_util(cb, su, true, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		} /* if ((s_susi->su == su) && (s_susi->state == SA_AMF_HA_STANDBY)) */
		else if ((a_susi->su == su) && (a_susi->state == SA_AMF_HA_ACTIVE)) {
			/* This SI relation with the SU is active assigning and the other
			 * SU is quiesced assigned.  Change the switch value of the SI to
			 * false. Remove the SI from the admin pointer. 
			 * Send D2N-INFO_SU_SI_ASSIGN with modified active all to the 
			 * other SU. Add that SU to operation list. Change state to 
			 * SG_realign. Free all the SI assignments to this SU.
			 */
			m_AVD_SET_SI_SWITCH(cb, (su->sg_of_su->admin_si), AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			if (s_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
				/* in-service quiesced SU. Send D2N-INFO_SU_SI_ASSIGN modify
				 * active all. Add that SU to operation list. Change state 
				 * to SG_realign state. Free all the SI assignments to this SU.
				 */
				avd_su_role_failover(su, s_susi->su);

			}

			avd_sg_su_asgn_del_util(cb, su, true, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}
		/* if ((a_susi->su == su) && (a_susi->state == SA_AMF_HA_ACTIVE)) */
	} /* if (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) */
	else if (su->sg_of_su->admin_si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED) {
		/* the SI admin operation is LOCK */
		if ((su->list_of_susi->state == SA_AMF_HA_ACTIVE) || (su->list_of_susi->state == SA_AMF_HA_QUIESCED)) {
			/* this SI relation with the SU is quiesced assigning.
			 * Remove the SI from the SI admin pointer.  Mark all the 
			 * SI relationships to this SU as unassigning.
			 */

			a_susi = avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);

			if (s_susi != AVD_SU_SI_REL_NULL) {
				if (s_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
					/* An in-service standby SU. Send D2N-INFO_SU_SI_ASSIGN 
					 * modify active all. Add that SU to operation list. Change 
					 * state to SG_realign state. Free all the SI assignments
					 * to this SU.
					 */
					avd_su_role_failover(su, s_susi->su);

					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}

				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_asgn_del_util(cb, su, true, false);
			} /* if (s_susi != AVD_SU_SI_REL_NULL) */
			else {
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_asgn_del_util(cb, su, true, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);

				if ((o_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) != NULL) {
					/* add the SU to the operation list and change the SG FSM to SG realign. */
					avd_sg_su_oper_list_add(cb, o_su, false);
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			}	/* else (s_susi != AVD_SU_SI_REL_NULL) */

		} else {	/* if ((su->list_of_susi->state == SA_AMF_HA_ACTIVE) ||
				   (su->list_of_susi->state == SA_AMF_HA_QUIESCED))  */
			/* means standby free the assignments. */
			avd_sg_su_asgn_del_util(cb, su, true, false);

		}		/* else ((su->list_of_susi->state == SA_AMF_HA_ACTIVE) ||
				   (su->list_of_susi->state == SA_AMF_HA_QUIESCED))  */

	} /* if (su->sg_of_su->admin_si->admin_state == NCS_ADMIN_STATE_LOCK) */
	else if (su->sg_of_su->admin_si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
		/* the SI admin operation is shutdown */
		if ((su->list_of_susi->state == SA_AMF_HA_ACTIVE) || (su->list_of_susi->state == SA_AMF_HA_QUIESCING)) {
			/* this SI relation with the SU is quiescing assigning/assigned
			 * Change the SI admin state to unlock Remove the SI from the 
			 * SI admin pointer. Mark all the SI relationships to this SU as
			 * unassigning. 
			 */

			a_susi = avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);

			if (s_susi != AVD_SU_SI_REL_NULL) {
				if (s_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
					/* in-service standby SU. Send D2N-INFO_SU_SI_ASSIGN modify 
					 * active all. Change state to SG_realign state. Add that 
					 * SU to operation list. Free all the SI assignments to
					 * this SU.
					 */
					avd_su_role_failover(su, s_susi->su);

					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}

				avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_UNLOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_asgn_del_util(cb, su, true, false);
			} /* if (s_susi != AVD_SU_SI_REL_NULL) */
			else {
				avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_asgn_del_util(cb, su, true, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);

				if ((o_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) != NULL) {
					/* add the SU to the operation list and change the SG FSM to SG realign. */
					avd_sg_su_oper_list_add(cb, o_su, false);
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			}	/* else (s_susi != AVD_SU_SI_REL_NULL) */

		} else {	/* if ((su->list_of_susi->state == SA_AMF_HA_ACTIVE) ||
				   (su->list_of_susi->state == SA_AMF_HA_QUIESCING))  */
			/* means standby free the assignments. */
			avd_sg_su_asgn_del_util(cb, su, true, false);

		}		/* else ((su->list_of_susi->state == SA_AMF_HA_ACTIVE) ||
				   (su->list_of_susi->state == SA_AMF_HA_QUIESCING))  */
	}

	TRACE_LEAVE();
}
/*****************************************************************************
 * Function: avd_sg_2n_node_fail_func
 *
 * Purpose:  This function is called when the node has already failed and
 *           the SIs have to be failed over. It does the functionality 
 *           specified in the design.
 *
 * Input: cb - the AVD control block
 *        su - The SU that has faulted because of the node failure.
 *        
 *
 * Returns: None.
 *
 * NOTES: This is a 2N redundancy model specific function.
 *
 * 
 **************************************************************************/

void avd_sg_2n_node_fail_func(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *a_susi, *s_susi;
	AVD_SU *o_su, *i_su;
	bool flag;

	TRACE_ENTER2("'%s', %u", su->name.value, su->sg_of_su->sg_fsm_state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL)
		goto done;

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		if ((su->list_of_susi->state == SA_AMF_HA_ACTIVE) || (su->list_of_susi->state == SA_AMF_HA_QUIESCED)) {
			/* this is a active SU. */
			/* Identify the standby SU and send D2N-INFO_SU_SI_ASSIGN message 
			 * modify active all assignments. Remove and free all the SI
			 * assignments to this SU. Add the standby SU to operation list and 
			 * Change state to SG_realign. If standby SU doesnt exists stay in
			 * the stable state.
			 */

			avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);

			if (s_susi != AVD_SU_SI_REL_NULL) {
				if ((s_susi->fsm != AVD_SU_SI_STATE_UNASGN) &&
					(s_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {
					/* Check if there is dependency between SI's within SU */
					if (avd_si_dependency_exists_within_su(su)) {
						AVD_SU_SI_REL *susi;

						for (susi = s_susi->su->list_of_susi; susi; susi = susi->su_next) {
							if ((s_susi->si->si_dep_state == AVD_SI_READY_TO_UNASSIGN) ||
								(s_susi->si->si_dep_state == AVD_SI_UNASSIGNING_DUE_TO_DEP)) {
								/* Before starting unassignment process of SI, check once again whether 
								 * sponsor SIs are assigned back,if so move the SI state to ASSIGNED state 
								 */
								if (all_sponsors_assigned_active(s_susi->si))
									si_dep_state_set(s_susi->si, AVD_SI_ASSIGNED);
								else
									avd_susi_del_send(s_susi); 	
							}
							if (avd_susi_role_failover(susi, su) == NCSCC_RC_FAILURE) {
								TRACE(" Active role modification failed");
								goto done;
							}
							avd_sg_su_oper_list_add(cb, susi->su, false);
							m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
						}

					} else {
						/* There is no dependency between SI's within SU, so trigger SU level failover */
						avd_su_role_failover(su, s_susi->su);
					}
				}
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				avd_sg_su_asgn_del_util(cb, su, true, false);
			} /* if (s_susi != AVD_SU_SI_REL_NULL) */
			else {
				avd_sg_su_asgn_del_util(cb, su, true, false);

				if ((o_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) != NULL) {
					/* add the SU to the operation list and change the SG FSM to SG realign. */
					avd_sg_su_oper_list_add(cb, o_su, false);
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			}	/* else (s_susi != AVD_SU_SI_REL_NULL) */
		} else {	/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

			/* means standby */
			/* identify an SU in the list that is in-service and 
			 * send D2N-INFO_SU_SI_ASSIGN message new standby for all the SIs on
			 * this SU to that are in-service SU. Remove and free all the SI 
			 * assignments to this SU. Add the newly assigned SU to operation list
			 * and Change state to SG_realign. If no other in-service SU exists, 
			 * stay in the stable state.
			 */

			avd_sg_su_asgn_del_util(cb, su, true, false);
			if ((o_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) != NULL) {
				/* add the SU to the operation list and change the SG FSM to SG realign. */
				avd_sg_su_oper_list_add(cb, o_su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}

		}		/* else (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if ((su->list_of_susi->state != SA_AMF_HA_STANDBY) &&
		    !((su->list_of_susi->state == SA_AMF_HA_QUIESCED) &&
		      (su->list_of_susi->fsm == AVD_SU_SI_STATE_UNASGN)
		    )
		    ) {
			/* SU is not standby */
			a_susi = avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);

			if (a_susi->su != su) {
				o_su = a_susi->su;
			} else if (s_susi != AVD_SU_SI_REL_NULL) {
				o_su = s_susi->su;
			} else {
				o_su = NULL;
			}

			if (o_su != NULL) {
				if (o_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
					/* the other SU has quiesced or standby assignments and is in the 
					 * operation list and is in service. Send a D2N-INFO_SU_SI_ASSIGN
					 * with modified active all to the other SU. Free the 
					 * SU SI relationships of this SU.  Remove the SU from the SU 
					 * operation list.
					 */
					avd_su_role_failover(su, o_su);

					avd_sg_su_asgn_del_util(cb, su, true, false);
					avd_sg_su_oper_list_del(cb, su, false);
				} else {
					/* the other SU has quiesced or standby assigned and is in the 
					 * operation list and is out of service.
					 * Send a D2N-INFO_SU_SI_ASSIGN with remove all to that SU. 
					 * Remove this SU from operation list. Free the 
					 * SU SI relationships of this SU.
					 */
					avd_sg_su_si_del_snd(cb, o_su);
					avd_sg_su_asgn_del_util(cb, su, true, false);
					avd_sg_su_oper_list_del(cb, su, false);
					m_AVD_CHK_OPLIST(o_su, flag);
					if (flag == false) {
						avd_sg_su_oper_list_add(cb, o_su, false);
					}
				}
			} else {	/* if (o_su != AVD_SU_NULL) */

				avd_sg_su_asgn_del_util(cb, su, true, false);
				avd_sg_su_oper_list_del(cb, su, false);
				if (su->sg_of_su->su_oper_list.su == NULL) {
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
					if ((o_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) != NULL) {
						/* add the SU to the operation list and change the SG FSM to SG realign. */
						avd_sg_su_oper_list_add(cb, o_su, false);
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					}
				}
			}	/* else (o_su != AVD_SU_NULL) */

		} /* if (su->list_of_susi->state != SA_AMF_HA_STANDBY) */
		else {
			/* the SU has standby assignments. Free all the SI assignments to 
			 * this SU. Remove the SU from operation list . Verify that all the
			 * SUSIs are assigned. Do the functionality specified in table with
			 * stable state and new SI event.
			 */
			avd_sg_su_asgn_del_util(cb, su, true, false);
			avd_sg_su_oper_list_del(cb, su, false);
			if (su->sg_of_su->su_oper_list.su == NULL) {
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				if ((o_su = avd_sg_2n_su_chose_asgn(cb, su->sg_of_su)) != NULL) {
					/* add the SU to the operation list and change the SG FSM to SG realign. */
					avd_sg_su_oper_list_add(cb, o_su, false);
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			}
		}		/* else (su->list_of_susi->state != SA_AMF_HA_STANDBY) */

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:

		avd_sg_2n_node_fail_su_oper(cb, su);

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		avd_sg_2n_node_fail_si_oper(cb, su);

		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:

		if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* SG is admin lock */

			/* Free all the SI assignments to this SU */
			avd_sg_su_asgn_del_util(cb, su, true, false);
			i_su = su->sg_of_su->list_of_su;
			while (i_su != NULL) {
				if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
					/* found a assigned su break */
					break;
				}
				i_su = i_su->sg_list_su_next;
			}

			if (i_su == NULL) {
				/* the SG doesnt have any SI assignments.Change state to stable. */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
			}

		} /* if (su->sg_of_su->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SG is admin shutdown */
			if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
				/* the SI relationships to the SU is quiescing. Change admin state 
				 * to lock, make the readiness state of all the SUs OOS.  Identify 
				 * its standby SU assignment send D2N-INFO_SU_SI_ASSIGN remove all 
				 * for the SU. If no standby, change state to stable. Free all the 
				 * SI assignments to this SU.
				 */
				avd_sg_2n_act_susi(cb, su->sg_of_su, &s_susi);
				if (s_susi != AVD_SU_SI_REL_NULL) {
					avd_sg_su_si_del_snd(cb, s_susi->su);
				} else {
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				}

				avd_sg_su_asgn_del_util(cb, su, true, false);
				avd_sg_admin_state_set(su->sg_of_su, SA_AMF_ADMIN_LOCKED);

				i_su = su->sg_of_su->list_of_su;
				while (i_su != NULL) {
					avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					i_su = i_su->sg_list_su_next;
				}
			} /* if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) */
			else {
				/* It is standby SU Free all the SI assignments to this SU */
				avd_sg_su_asgn_del_util(cb, su, true, false);
			}	/* else (su->list_of_susi->state == SA_AMF_HA_QUIESCING) */

		}
		/* if (su->sg_of_su->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		goto done;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

done:
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_sg_2n_su_admin_fail
 *
 * Purpose:  This function is called when SU become OOS because of the
 * LOCK or shutdown of the SU or node.The functionality will be as described in
 * the SG design FSM. 
 *
 * Input: cb - the AVD control block
 *        su - The SU that has failed because of the admin operation.
 *        avnd - The AvND structure of the node that is being operated upon.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This is a 2N redundancy model specific function. The avnd pointer
 * value is valid only if this is a SU operation being done because of the node
 * admin change.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_2n_su_admin_fail(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s'", su->name.value);
	TRACE("SG state %u", su->sg_of_su->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == SA_FALSE)) {
		TRACE("state:%u, ncs:%u", cb->init_state, su->sg_of_su->sg_ncs_spec);
		goto done;
	}

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) ||
		    ((avnd != NULL) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED))) {
			if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
				/* Check if there is dependency between SI's within SU */
				if (avd_si_dependency_exists_within_su(su)) {
					if (avd_sg_susi_mod_snd_honouring_si_dependency(su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
						LOG_NO("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
						goto done;
					}
				} else {
					/* change the state for all assignments to quiesced. */
					if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
						LOG_NO("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
						goto done;
					}
				}

				/* add the SU to the operation list and change the SG FSM to SU operation. */
				avd_sg_su_oper_list_add(cb, su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);

			} else {	/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

				/* means standby */
				/* change the state for all assignments to unassign and send delete mesage. */
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}

				/* add the SU to the operation list and change the SG FSM to SG realign. */
				avd_sg_su_oper_list_add(cb, su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}	/* else (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
		}		/* if ((su->admin_state == NCS_ADMIN_STATE_LOCK) ||
				   ((avnd != AVD_AVND_NULL) && (avnd->su_admin_state == NCS_ADMIN_STATE_LOCK))) */
		else if ((su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) ||
			 ((avnd != NULL) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN))) {
			if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
				/* this is a active SU. */
				/* Check if there is dependency between SI's within SU */
				if (avd_si_dependency_exists_within_su(su)) {
					if (avd_sg_susi_mod_snd_honouring_si_dependency(su, SA_AMF_HA_QUIESCING) == NCSCC_RC_FAILURE) {
						LOG_NO("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
						goto done;
					}
				} else {
					/* change the state for all assignments to quiesceing. */
					if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCING) == NCSCC_RC_FAILURE) {
						LOG_NO("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
						goto done;
					}
				}

				/* add the SU to the operation list and change the SG FSM to SU operation. */
				avd_sg_su_oper_list_add(cb, su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
			} else {	/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

				/* means standby */
				/* change the state for all assignments to unassign and send delete mesage. */
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}

				/* add the SU to the operation list and change the SG FSM to SG realign. */
				avd_sg_su_oper_list_add(cb, su, false);
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN)
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}	/* else (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
		}		/* if ((su->admin_state == NCS_ADMIN_STATE_SHUTDOWN) ||
				   ((avnd != AVD_AVND_NULL) && (avnd->su_admin_state == NCS_ADMIN_STATE_SHUTDOWN))) */
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SU_OPER:
		if ((su->sg_of_su->su_oper_list.su == su) &&
		    (su->list_of_susi->state == SA_AMF_HA_QUIESCING) &&
		    ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) ||
		     ((avnd != NULL) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)))) {
			/* If the SU is in the operation list and the SU admin state is lock.
			 * send D2N-INFO_SU_SI_ASSIGN modify quiesced message to the SU. 
			 */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_NO("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}
		}
		break;		/* case AVD_SG_FSM_SU_OPER: */
	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		goto done;
		break;
	}			/* switch (su->sg_of_su->sg_fsm_state) */

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_2n_si_admin_down
 *
 * Purpose:  This function is called when SIs admin state is changed to
 * LOCK or shutdown. The functionality will be as described in
 * the SG design FSM. 
 *
 * Input: cb - the AVD control block
 *        si - The SI pointer.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This is a 2N redundancy model specific function.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_2n_si_admin_down(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *a_susi, *s_susi;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s', %u", si->name.value, si->sg_of_si->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == SA_FALSE)) {
		LOG_NO("%s: state: %u", __FUNCTION__, cb->init_state);
		goto done;
	}
	if (si->list_of_sisu == AVD_SU_SI_REL_NULL) {
		LOG_NO("'%s' has no assignments", si->name.value);
		goto done;
	}

	switch (si->sg_of_si->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if (si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED) {
			/* SI lock. Identify the SU that is active send D2N-INFO_SU_SI_ASSIGN
			 * modify quiesced for only this SI, identify its standby SU assignment
			 * send D2N-INFO_SU_SI_ASSIGN remove for only this SI. Change state to
			 * SI_operation state. Add it to admin SI pointer.
			 */
			if (si->list_of_sisu->state == SA_AMF_HA_ACTIVE) {
				a_susi = si->list_of_sisu;
				s_susi = si->list_of_sisu->si_next;
			} else {
				s_susi = si->list_of_sisu;
				a_susi = si->list_of_sisu->si_next;
			}

			/* change the state of the SI to quiesced */

			if (avd_susi_mod_send(a_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
				goto done;

			if (s_susi != AVD_SU_SI_REL_NULL)
				avd_susi_del_send(s_susi);

			/* add the SI to the admin list and change the SG FSM to SI operation. */
			m_AVD_SET_SG_ADMIN_SI(cb, si);
			m_AVD_SET_SG_FSM(cb, (si->sg_of_si), AVD_SG_FSM_SI_OPER);
		} /* if (si->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SI shutdown. Identify the SU that is active 
			 * send D2N-INFO_SU_SI_ASSIGN modify quiescing for only this SI. 
			 * Change state to SI_operation state. Add it to admin SI pointer.
			 */
			if (si->list_of_sisu->state == SA_AMF_HA_ACTIVE) {
				a_susi = si->list_of_sisu;
				s_susi = si->list_of_sisu->si_next;
			} else {
				s_susi = si->list_of_sisu;
				a_susi = si->list_of_sisu->si_next;
			}

			/* change the state of the SI to quiescing */
			if (avd_susi_mod_send(a_susi, SA_AMF_HA_QUIESCING) != NCSCC_RC_SUCCESS)
				goto done;


			/* add the SI to the admin list and change the SG FSM to SI operation. */
			m_AVD_SET_SG_ADMIN_SI(cb, si);
			m_AVD_SET_SG_FSM(cb, (si->sg_of_si), AVD_SG_FSM_SI_OPER);
		}		/* if (si->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SI_OPER:
		if ((si->sg_of_si->admin_si == si) && (si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED)) {
			/* If the SI is in the admin pointer and the SI admin state is shutdown, 
			 * change the admin state of the SI to lock and 
			 * send D2N-INFO_SU_SI_ASSIGN modify quiesced message to the SU that
			 * is being assigned quiescing state. Send a D2N-INFO_SU_SI_ASSIGN 
			 * remove message for this SI to the other SU.
			 */
			if (si->list_of_sisu->state == SA_AMF_HA_QUIESCING) {
				a_susi = si->list_of_sisu;
				s_susi = si->list_of_sisu->si_next;
			} else {
				s_susi = si->list_of_sisu;
				a_susi = si->list_of_sisu->si_next;
			}

			/* change the state of the SI to quiesced */
			if (avd_susi_mod_send(a_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
				goto done;

			if (s_susi != AVD_SU_SI_REL_NULL)
				avd_susi_del_send(s_susi);
		}
		break;		/* case AVD_SG_FSM_SI_OPER: */
	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)si->sg_of_si->sg_fsm_state));
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, si->name.value, si->name.length);
		goto done;
		break;
	}			/* switch (si->sg_of_si->sg_fsm_state) */

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_2n_sg_admin_down
 *
 * Purpose:  This function is called when SGs admin state is changed to
 * LOCK or shutdown. The functionality will be as described in
 * the SG design FSM. 
 *
 * Input: cb - the AVD control block
 *        sg - The SG pointer.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This is a 2N redundancy model specific function.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_2n_sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SU_SI_REL *a_susi, *s_susi;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s', %u", sg->name.value, sg->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == SA_FALSE)) {
		LOG_NO("%s: state: %u", __FUNCTION__, cb->init_state);
		goto done;
	}

	switch (sg->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		a_susi = avd_sg_2n_act_susi(cb, sg, &s_susi);
		if (a_susi == AVD_SU_SI_REL_NULL) {
			avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_STABLE);
			rc = NCSCC_RC_SUCCESS;
			goto done;
		}

		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* SG lock. Identify the SU that is active send D2N-INFO_SU_SI_ASSIGN 
			 * modify quiesced all for the SU; identify standby SU 
			 * send D2N-INFO_SU_SI_ASSIGN remove all for standby SU.
			 * Change state to SG admin state.
			 */
			if (avd_sg_su_si_mod_snd(cb, a_susi->su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, a_susi->su->name.value, a_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, a_susi->si->name.value, a_susi->si->name.length);
				goto done;
			}

			if (s_susi != AVD_SU_SI_REL_NULL) {
				avd_sg_su_si_del_snd(cb, s_susi->su);
			}

			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_ADMIN);
		} /* if (sg->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (sg->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SG shutdown. Identify the SU that is active 
			 * send D2N-INFO_SU_SI_ASSIGN modify quiescing all for this SU. 
			 * Change state to SG_admin. If no active SU exist, change the SG 
			 * admin state to LOCK, stay in stable state.
			 */
			if (avd_sg_su_si_mod_snd(cb, a_susi->su, SA_AMF_HA_QUIESCING) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, a_susi->su->name.value, a_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, a_susi->si->name.value, a_susi->si->name.length);
				goto done;
			}

			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_ADMIN);
		}		/* if (sg->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_ADMIN:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* If the SG admin state is shutdown, change the admin state of 
			 * the SG to lock and send D2N-INFO_SU_SI_ASSIGN modify quiesced 
			 * message to the SU with quiescing assignment. Also 
			 * send D2N-INFO_SU_SI_ASSIGN with remove all to the SU with 
			 * standby assignment.
			 */
			a_susi = avd_sg_2n_act_susi(cb, sg, &s_susi);
			if (avd_sg_su_si_mod_snd(cb, a_susi->su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, a_susi->su->name.value, a_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, a_susi->si->name.value, a_susi->si->name.length);
				goto done;
			}

			if (s_susi != AVD_SU_SI_REL_NULL) {
				avd_sg_su_si_del_snd(cb, s_susi->su);
			}

		}		/* if (sg->admin_state == NCS_ADMIN_STATE_LOCK) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)sg->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, sg->name.value, sg->name.length);
		goto done;
		break;
	}			/* switch (sg->sg_fsm_state) */

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}
