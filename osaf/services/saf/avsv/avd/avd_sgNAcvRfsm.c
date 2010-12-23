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

  DESCRIPTION: This file is part of the SG processing module. It contains
  the SG state machine, for processing the events related to SG for N-way
  Active redundancy model.

..............................................................................

  FUNCTIONS INCLUDED in this file:

  avd_sg_nacvred_su_chose_asgn - Choose and assign SIs to SUs. 
  avd_sg_nacvred_si_func - Function to process the new complete SI in the SG.
  avd_sg_nacvred_su_fault_func - function is called when a SU failed and switchover
                               needs to be done.
  avd_sg_nacvred_su_insvc_func - function is called when a SU readiness state changes
                            to inservice from out of service
  avd_sg_nacvred_susi_sucss_func - processes successful SUSI assignment. 
  avd_sg_nacvred_susi_fail_func - processes failure of SUSI assignment.
  avd_sg_nacvred_realign_func - function called when SG operation is done or cluster
                           timer expires.
  avd_sg_nacvred_node_fail_func - function is called when the node has already failed and
                             the SIs have to be failed over.
  avd_sg_nacvred_su_admin_fail -  function is called when SU is LOCKED/SHUTDOWN.  
  avd_sg_nacvred_si_admin_down - function is called when SIs is LOCKED/SHUTDOWN.
  avd_sg_nacvred_sg_admin_down - function is called when SGs is LOCKED/SHUTDOWN.
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <logtrace.h>

#include <avd.h>
#include <avd_imm.h>

/*****************************************************************************
 * Function: avd_sg_nacvred_su_chose_asgn
 *
 * Purpose:  This function will identify SIs whose assignments is not complete, 
 * search for in-service SUs that can take assignment for this SI and assign 
 * this unassigned SIs to them by Sending D2N-INFO_SU_SI_ASSIGN message for the 
 * SUs with role active for the SIs. It then adds the Assigning SUs to 
 * the SU operation list. If no assignments happen, it returns NULL.
 *
 * Input: cb - the AVD control block
 *        sg - The pointer to the service group.
 *        
 *
 * Returns: pointer to the first SU that is undergoing assignment. Null if
 *          no assignments need to happen.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

AVD_SU *avd_sg_nacvred_su_chose_asgn(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SU *i_su;
	AVD_SI *i_si;
	NCS_BOOL l_flag;
	AVD_SUS_PER_SI_RANK_INDX i_idx;
	AVD_SUS_PER_SI_RANK *su_rank_rec;
	AVD_SU_SI_REL *tmp_rel;

	TRACE_ENTER2("'%s'", sg->name.value);

	i_si = sg->list_of_si;
	l_flag = TRUE;

	while ((i_si != AVD_SI_NULL) && (l_flag == TRUE)) {
		/* verify that the SI is ready and needs come more assignments. */
		if ((i_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) ||
		    (i_si->max_num_csi != i_si->num_csi) ||
		    (m_AVD_SI_ACTV_MAX_SU(i_si) <= m_AVD_SI_ACTV_CURR_SU(i_si))) {
			i_si = i_si->sg_list_of_si_next;
			continue;
		}

		/* Screen SI sponsors state and adjust the SI-SI dep state accordingly */
		avd_screen_sponsor_si_state(cb, i_si, FALSE);

		/* Cannot be assigned, as sponsors SIs are not in enabled state for this SI */
		if ((i_si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED) ||
		    (i_si->si_dep_state == AVD_SI_UNASSIGNING_DUE_TO_DEP)) {
			i_si = i_si->sg_list_of_si_next;
			continue;
		}

		/* identify a in-service SU which is not assigned to this SI and can
		 * take more assignments so that the SI can be assigned. 
		 */
		memset((uns8 *)&i_idx, '\0', sizeof(i_idx));
		i_idx.si_name = i_si->name;
		i_idx.su_rank = 0;
		for (su_rank_rec = avd_sirankedsu_getnext_valid(cb, i_idx, &i_su);
		     (su_rank_rec != AVD_SU_PER_SI_RANK_NULL)
		     && (m_CMP_HORDER_SANAMET(su_rank_rec->indx.si_name, i_si->name) == 0);
		     su_rank_rec = avd_sirankedsu_getnext_valid(cb, su_rank_rec->indx, &i_su)) {
			if (i_su == NULL)
				continue;

			if ((i_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
			    (i_su->si_max_active <= i_su->saAmfSUNumCurrActiveSIs) ||
			    ((i_su->sg_of_su->saAmfSGMaxActiveSIsperSU != 0)
			     && (i_su->sg_of_su->saAmfSGMaxActiveSIsperSU <= i_su->saAmfSUNumCurrActiveSIs))) {
				i_su = i_su->sg_list_su_next;
				continue;
			}

			if (avd_su_susi_find(cb, i_su, &i_si->name)
			    != AVD_SU_SI_REL_NULL) {
				/* This SU has already a assignment for this SI go to the
				 * next SU.
				 */
				continue;
			}

			/* found the SU assign the SI to the SU as active */
			if (avd_new_assgn_susi(cb, i_su, i_si, SA_AMF_HA_ACTIVE, FALSE, &tmp_rel) == NCSCC_RC_SUCCESS) {
				/* Add the SU to the operation list */
				avd_sg_su_oper_list_add(cb, i_su, FALSE);

				/* Check if the SI can take more assignments. If not exit the SU loop.                         
				 */
				if (m_AVD_SI_ACTV_MAX_SU(i_si) <= m_AVD_SI_ACTV_CURR_SU(i_si)) {
					break;
				}
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_si->name.value, i_si->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_su->name.value, i_su->name.length);
			}

		}		/* for (su_rank_rec = avd_sus_per_si_rank_struc_find_valid_next(cb,i_idx, &i_su); 
				   (su_rank_rec != AVD_SU_PER_SI_RANK_NULL) && (m_CMP_NORDER_SANAMET(su_rank_rec->si_name,i_si->name) == 0);
				   su_rank_rec = avd_sus_per_si_rank_struc_find_valid_next(cb,su_rank_rec->indx, &i_su) ) */

		/* identify a in-service SU which is not assigned to this SI and can
		 * take more assignments so that the SI can be assigned. 
		 */
		l_flag = FALSE;
		i_su = sg->list_of_su;
		while (i_su != NULL) {
			if ((i_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
			    (i_su->si_max_active <= i_su->saAmfSUNumCurrActiveSIs) ||
			    ((i_su->sg_of_su->saAmfSGMaxActiveSIsperSU != 0)
			     && (i_su->sg_of_su->saAmfSGMaxActiveSIsperSU <= i_su->saAmfSUNumCurrActiveSIs))) {
				i_su = i_su->sg_list_su_next;
				continue;
			}

			l_flag = TRUE;

			if (m_AVD_SI_ACTV_MAX_SU(i_si) <= m_AVD_SI_ACTV_CURR_SU(i_si)) {
				/* The preferred number of active assignments for SI has reached, so continue
				   to next SI */
				i_su = NULL;
				continue;
			}

			if (avd_su_susi_find(cb, i_su, &i_si->name)
			    != AVD_SU_SI_REL_NULL) {
				/* This SU has already a assignment for this SI go to the 
				 * next SU.
				 */
				i_su = i_su->sg_list_su_next;
				continue;
			}

			/* found the SU assign the SI to the SU as active */
			if (avd_new_assgn_susi(cb, i_su, i_si, SA_AMF_HA_ACTIVE, FALSE, &tmp_rel) == NCSCC_RC_SUCCESS) {
				/* Add the SU to the operation list */
				avd_sg_su_oper_list_add(cb, i_su, FALSE);

				/* Check if the SI can take more assignments. If not exit the SU loop.
				 */
				if (m_AVD_SI_ACTV_MAX_SU(i_si) <= m_AVD_SI_ACTV_CURR_SU(i_si)) {
					i_su = NULL;
					continue;
				}
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_si->name.value, i_si->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_su->name.value, i_su->name.length);
			}

			/* choose the next SU */
			i_su = i_su->sg_list_su_next;

		}		/* while (i_su != AVD_SU_NULL) */

		/* choose the next SI */
		i_si = i_si->sg_list_of_si_next;

	}			/* while ((i_si != AVD_SI_NULL) && (l_flag == TRUE)) */

	return sg->su_oper_list.su;
}

/*****************************************************************************
 * Function: avd_sg_nacvred_si_func
 *
 * Purpose:  This function is called when a new SI is added to a SG. The SG is
 * of type N-way active redundancy model. This function will perform the functionality
 * described in the SG FSM design. 
 *
 * Input: cb - the AVD control block
 *        si - The pointer to the service instance.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This is a N-way active redundancy model specific function. If there are
 * any SIs being transitioned due to operator, this call will just return
 * success.
 *
 * 
 **************************************************************************/

uns32 avd_sg_nacvred_si_func(AVD_CL_CB *cb, AVD_SI *si)
{
	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);

	/* If the SG FSM state is not stable just return success. */
	if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
		return NCSCC_RC_SUCCESS;
	}

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == SA_FALSE)) {
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, si->sg_of_si->sg_ncs_spec);
		return NCSCC_RC_SUCCESS;
	}

	if (avd_sg_nacvred_su_chose_asgn(cb, si->sg_of_si) == NULL) {
		/* all the assignments have already been done in the SG. */
		return NCSCC_RC_SUCCESS;
	}

	/* change the FSM state */
	m_AVD_SET_SG_FSM(cb, (si->sg_of_si), AVD_SG_FSM_SG_REALIGN);

	return NCSCC_RC_SUCCESS;
}

 /*****************************************************************************
 * Function: avd_sg_nacvred_su_fault_func
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

uns32 avd_sg_nacvred_su_fault_func(AVD_CL_CB *cb, AVD_SU *su)
{
	NCS_BOOL flag;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL)
		return NCSCC_RC_SUCCESS;

	/* Do the functionality based on the current state. */
	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		/* change the state for all assignments to quiesced. */
		if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			return NCSCC_RC_FAILURE;
		}

		/* add the SU to the operation list and change the SG FSM to SU operation. */
		avd_sg_su_oper_list_add(cb, su, FALSE);
		m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if (su->sg_of_su->admin_si != AVD_SI_NULL) {

			/* The SI admin pointer SI has assignment only to this SU. If 
			 * this SI admin is shutdown change to LOCK.
			 * send D2N-INFO_SU_SI_ASSIGN with quiesced all to the SU.
			 * Remove the SI from SI admin pointer. Add the SU to operation list.
			 * Stay in same state.
			 */

			if (avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name)
			    != AVD_SU_SI_REL_NULL) {
				m_AVD_SU_SI_CHK_QSD_ASGN(su, flag);
				if (flag == FALSE) {
					if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
						return NCSCC_RC_FAILURE;
					}

				}

				if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
				    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
					avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				}
			} else {	/* if ((susi = 
					   avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,FALSE))
					   != AVD_SU_SI_REL_NULL) */
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}
			}

			avd_sg_su_oper_list_add(cb, su, FALSE);
		} else {	/* if (su->sg_of_su->admin_si != AVD_SI_NULL) */

			/* Send D2N-INFO_SU_SI_ASSIGN with quiesced all to the SU, if it has 
			 * atleast one SI in quiescing or active state. If the SU is in 
			 * operation list, if the SU admin state is shutdown change to LOCK.
			 * Add the SU to the SU operation list.
			 */
			m_AVD_SU_SI_CHK_QSD_ASGN(su, flag);
			if (flag == FALSE) {
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}
			}
			m_AVD_CHK_OPLIST(su, flag);
			if (flag == TRUE) {
				m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
					if (flag == TRUE) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}

			} else {	/* if (flag == TRUE) */

				avd_sg_su_oper_list_add(cb, su, FALSE);

			}	/* else (flag == TRUE) */

		}		/* else (su->sg_of_su->admin_si != AVD_SI_NULL) */

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:

		if (su->sg_of_su->su_oper_list.su == su) {
			/* The SU is same as the SU in the list. If the SI relationships to the
			 * SU is quiescing, If this SU admin is shutdown change to LOCK and
			 * send D2N-INFO_SU_SI_ASSIGN modify quiesced all.
			 */
			if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}

				m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
					if (flag == TRUE) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}
			}	/* if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) */
		} else {	/* if(su->sg_of_su->su_oper_list.su == su) */

			/* The SU is not the same as the SU in the list.
			 * Send D2N-INFO_SU_SI_ASSIGN modify quiesced all to the SU.
			 * Add the SU to the operation list and change state to SG_realign state.
			 */

			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			avd_sg_su_oper_list_add(cb, su, FALSE);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* else (su->sg_of_su->su_oper_list.su == su) */

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		if (avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name)
		    != AVD_SU_SI_REL_NULL) {
			/* The SI admin pointer, SI has assignment only to this SU. If this 
			 * SI admin is shutdown change to LOCK and send D2N-INFO_SU_SI_ASSIGN 
			 * with quiesced all to the SU, if the SU has any other assignment
			 * send quiesced all to the SU. Remove the SI from SI admin pointer.
			 * Add the SU to operation list and change state to SU_operation.
			 */
			m_AVD_SU_SI_CHK_QSD_ASGN(su, flag);
			if (flag == FALSE) {
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}

			}

			if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
			    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
				avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
			} else {
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		} else {	/* if ((susi = 
				   avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,FALSE))
				   != AVD_SU_SI_REL_NULL) */
			/* Send D2N-INFO_SU_SI_ASSIGN with quiesced all to the SU.
			 * Add the SU to the SU operation list.
			 * Change the state to SG_realign state.
			 */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}

		avd_sg_su_oper_list_add(cb, su, FALSE);

		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:

		if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* the SG is lock no action. */
			return NCSCC_RC_SUCCESS;
		} else if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* The SG is shutdown. if the SI relationships to the SU is quiescing.
			 * Send D2N-INFO_SU_SI_ASSIGN modify quiesced to the SU. No need to
			 * change admin to LOCK here itself. We can do it just before
			 * becoming stable.
			 */
			if ((su->list_of_susi->state == SA_AMF_HA_QUIESCING) &&
			    (su->list_of_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}
			}
			/* if ((su->list_of_susi->state == SA_AMF_HA_QUIESCING) &&
			   (su->list_of_susi->fsm == AVD_SU_SI_STATE_MODIFY)) */
		} else {	/* if (su->sg_of_su->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */

			/* log fatal error */
			LOG_ER("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->saAmfSGAdminState);
			return NCSCC_RC_FAILURE;
		}

		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		/* log fatal error about the invalid value */
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

 /*****************************************************************************
 * Function: avd_sg_nacvred_su_insvc_func
 *
 * Purpose:  This function is called when a SU readiness state changes
 * to inservice from out of service. The SG is of type N-way active redundancy
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

uns32 avd_sg_nacvred_su_insvc_func(AVD_CL_CB *cb, AVD_SU *su)
{
	TRACE_ENTER2("'%s', %u", su->name.value, su->sg_of_su->sg_fsm_state);

	/* An SU will not become in service when the SG is being locked or shutdown.
	 */
	if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_ADMIN) {
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		return NCSCC_RC_FAILURE;
	}

	/* If the SG FSM state is not stable just return success. */
	if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) {
		return NCSCC_RC_SUCCESS;
	}

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == SA_FALSE)) {
		return NCSCC_RC_SUCCESS;
	}

	if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == NULL) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);

		/* all the assignments have already been done in the SG. */
		return NCSCC_RC_SUCCESS;
	}

	/* change the FSM state */
	m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
	return NCSCC_RC_SUCCESS;

}

/*****************************************************************************
 * Function: avd_sg_nacvred_susi_sucss_func
 *
 * Purpose:  This function is called when a SU SI ack function is
 * received from the AVND with success value. The SG FSM for N-way active redundancy
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
 * NOTES: This is a N-way active redundancy model specific function.
 *
 * 
 **************************************************************************/

uns32 avd_sg_nacvred_susi_sucss_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
				     AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	NCS_BOOL flag;
	AVD_SU_SI_STATE old_fsm_state;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		/* Do the action specified in the message if delete else no action. */
		if (act == AVSV_SUSI_ACT_DEL) {
			if (susi == AVD_SU_SI_REL_NULL) {
				avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);
			} else {
				/* free all the CSI assignments  */
				avd_compcsi_delete(cb, susi, FALSE);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, susi);
			}
		}
		/* log informational error. */
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if (act == AVSV_SUSI_ACT_DEL) {
			/* Action is remove. */

			if (susi != AVD_SU_SI_REL_NULL) {
				/* Remove the SI relationship to this SU. */
				/* free all the CSI assignments  */
				avd_compcsi_delete(cb, susi, FALSE);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, susi);
			} else {
				/* Remove all the SI relationships to this SU. */
				avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);
			}

			if (su->sg_of_su->admin_si != AVD_SI_NULL) {
				/* SI in the admin pointer. If the SI has only one SU assignment 
				 * and its in unassign state,  Remove the SI from the admin pointer.
				 * If  that SU is not in the operation list, Add that SU to 
				 * operation list. If the SI admin state is shutdown change 
				 * it to LOCK. 
				 */

				if (su->sg_of_su->admin_si->list_of_sisu != AVD_SU_SI_REL_NULL) {
					if ((su->sg_of_su->admin_si->list_of_sisu->fsm
					     == AVD_SU_SI_STATE_UNASGN) &&
					    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
						avd_sg_su_oper_list_add(cb, su->sg_of_su->admin_si->list_of_sisu->su,
									FALSE);
						avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
						m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
					}
				} else {
					avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				}

			}
			/* if (su->sg_of_su->admin_si != AVD_SI_NULL) */
			m_AVD_SU_SI_CHK_ASGND(su, flag);

			if (flag == TRUE) {
				/* All the assignments are assigned. Remove the SU from 
				 * the operation list. 
				 */
				avd_sg_su_oper_list_del(cb, su, FALSE);
			}

			if ((su->sg_of_su->admin_si == AVD_SI_NULL) && (su->sg_of_su->su_oper_list.su == NULL)) {
				if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == NULL) {
					/* No New assignments are been done in the SG. change the FSM state */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
					avd_sg_app_su_inst_func(cb, su->sg_of_su);
				}

			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if (state == SA_AMF_HA_QUIESCED) {
			if (susi != AVD_SU_SI_REL_NULL) {
				/* quiesced for a single SI. Send a D2N-INFO_SU_SI_ASSIGN with
				 * removal for this SU for the SI. If SI in the SI admin pointer and
				 * has only this SU assignment, Change SI admin to LOCK. Remove 
				 * the SI from the SI admin pointer. If the SU is not in 
				 * the SU operation list, Add the SU to the operation list.
				 */

				old_fsm_state = susi->fsm;
				susi->fsm = AVD_SU_SI_STATE_UNASGN;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				if (avd_snd_susi_msg(cb, su, susi, AVSV_SUSI_ACT_DEL) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
					susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
					return NCSCC_RC_FAILURE;
				}

				if ((su->sg_of_su->admin_si != AVD_SI_NULL) &&
				    (su->sg_of_su->admin_si->list_of_sisu->su == su) &&
				    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
					avd_sg_su_oper_list_add(cb, su, FALSE);
					avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				}

			} else {	/* if (susi != AVD_SU_SI_REL_NULL) */

				/* quiesced all. Send a D2N-INFO_SU_SI_ASSIGN with removal all for
				 * this SU. If SI in the SI admin pointer has assignment only to
				 * this SU, Change SI admin to LOCK. Remove the SI from 
				 * the SI admin pointer. the SU is not in the SU operation list,
				 * Add the SU to the operation list. If the SU is in 
				 * the operation list, If SU admin state is shutdown, 
				 * change SU admin to LOCK.
				 */

				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}

				m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
				if (su->sg_of_su->admin_si != AVD_SI_NULL) {
					if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
					    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
						avd_sg_su_oper_list_add(cb, su, FALSE);
						avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
						m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					}
				} else if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
					if (flag == TRUE) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}

			}	/* else (susi != AVD_SU_SI_REL_NULL) */

		} /* if (state == SA_AMF_HA_QUIESCED) */
		else if (state == SA_AMF_HA_ACTIVE) {
			/* the HA state is active and all the assignments to the SU are 
			 * assigned. Remove the SU from the SU operation list. If 
			 * the SU operation list and the SI admin pointer are empty.
			 * choose and assign SIs whose active assignment criteria is not 
			 * meet to in-service SUs, by sending D2N-INFO_SU_SI_ASSIGN message 
			 * for the SUs with role active for the SIs. Add the SUs to 
			 * operation list and stay in the same state. If no assignment can be
			 * done, change the state to stable state.
			 */

			m_AVD_SU_SI_CHK_ASGND(su, flag);

			if (flag == TRUE) {
				avd_sg_su_oper_list_del(cb, su, FALSE);
				if ((su->sg_of_su->admin_si == AVD_SI_NULL) && (su->sg_of_su->su_oper_list.su == NULL)) {
					if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == NULL) {
						/* No New assignments are been done in the SG. change the FSM state */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sg_app_su_inst_func(cb, su->sg_of_su);
					}

				}
			}
			/* if (flag == TRUE) */
		}
		/* if (state == SA_AMF_HA_ACTIVE) */
		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:
		if (act == AVSV_SUSI_ACT_DEL) {
			if (susi == AVD_SU_SI_REL_NULL) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);

				/* free all the CSI assignments  */
				avd_compcsi_delete(cb, susi, FALSE);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, susi);
			}

			avd_sg_su_oper_list_del(cb, su, FALSE);

			if (su->sg_of_su->su_oper_list.su == NULL) {
				if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) != NULL) {
					/* New assignments are been done in the SG. */
					/* change the FSM state */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					return NCSCC_RC_SUCCESS;
				}

				/* change the FSM state */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if (state == SA_AMF_HA_QUIESCED) {
			/* The HA state is quiesced all/quiesced for an SI that is the only SI 
			 * assigned and SU is in the operation list. If the admin state of the 
			 * SU is shutdown change it to lock. Send a D2N-INFO_SU_SI_ASSIGN with
			 * remove all to the SU. Change to SG_realign state.
			 */

			if ((susi == AVD_SU_SI_REL_NULL) ||
			    ((su->list_of_susi == susi) && (susi->su_next == AVD_SU_SI_REL_NULL))) {
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}

				if (su->sg_of_su->su_oper_list.su != su) {
					LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->list_of_susi->si->name.value,
									 su->list_of_susi->si->name.length);

					avd_sg_su_oper_list_add(cb, su, FALSE);
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					return NCSCC_RC_SUCCESS;
				}

				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);

				m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
					if (flag == TRUE) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}
			}
			/* if ((susi == AVD_SU_SI_REL_NULL) ||
			   ((su->list_of_susi == susi) && (susi->su_next == AVD_SU_SI_REL_NULL))) */
		}
		/* if (state == SA_AMF_HA_QUIESCED) */
		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		if (act == AVSV_SUSI_ACT_DEL) {
			/* message with remove for a SI in the admin pointer. Remove 
			 * the SI relationship to this SU. If the SI has only one SU assignment
			 * and its in unassign state,  Remove the SI from the admin pointer. 
			 * Add the SU to operation list. If the SI admin state is shutdown 
			 * change it to LOCK. Change state to SG_realign.
			 */

			if (susi != AVD_SU_SI_REL_NULL) {
				/* Remove the SI relationship to this SU. */
				/* free all the CSI assignments  */
				avd_compcsi_delete(cb, susi, FALSE);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, susi);
			} else {
				/* Remove all the SI relationships to this SU. */
				avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);
			}

			if (su->sg_of_su->admin_si->list_of_sisu != AVD_SU_SI_REL_NULL) {
				if ((su->sg_of_su->admin_si->list_of_sisu->fsm
				     == AVD_SU_SI_STATE_UNASGN) &&
				    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
					avd_sg_su_oper_list_add(cb, su->sg_of_su->admin_si->list_of_sisu->su, FALSE);
					avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			} else {
				avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if ((state == SA_AMF_HA_QUIESCED) && (susi != AVD_SU_SI_REL_NULL)) {
			/* message with modified quiesced for a SI in the admin pointer.
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove for this SI to this SU. 
			 * If the SI has only one SU assignment, Remove the SI from the 
			 * admin pointer. Add the SU to operation list. If the SI admin state 
			 * is shutdown change it to LOCK. Change state to SG_realign.
			 */

			old_fsm_state = susi->fsm;
			susi->fsm = AVD_SU_SI_STATE_UNASGN;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
			if (avd_snd_susi_msg(cb, su, susi, AVSV_SUSI_ACT_DEL) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
				susi->fsm = old_fsm_state;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				return NCSCC_RC_FAILURE;
			}

			if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
			    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
				avd_sg_su_oper_list_add(cb, su, FALSE);
				avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		}
		/* if ((state == SA_AMF_HA_QUIESCED) && (susi != AVD_SU_SI_REL_NULL)) */
		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:

		if (act == AVSV_SUSI_ACT_DEL) {
			/* The action is remove all. Remove the SI relationship to this SU.
			 * Remove the SU from the SU operation list. If the SU operation list
			 * is empty, If this SG admin is shutdown change to LOCK.
			 * Change the SG FSM state to stable.
			 */

			avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

			avd_sg_su_oper_list_del(cb, su, FALSE);

			if (su->sg_of_su->su_oper_list.su == NULL) {
				avd_sg_admin_state_set(su->sg_of_su, SA_AMF_ADMIN_LOCKED);

				/* change the FSM state */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if (state == SA_AMF_HA_QUIESCED) {
			/* If HA state is quiesced all, Send a D2N-INFO_SU_SI_ASSIGN with 
			 * removal all for this SU.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

		}
		/* if (state == SA_AMF_HA_QUIESCED) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uns32)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_nacvred_susi_fail_func
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
 * NOTES: This is a N-way active redundancy model specific function.
 *
 * 
 **************************************************************************/

uns32 avd_sg_nacvred_susi_fail_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
				    AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_STATE old_fsm_state;
	NCS_BOOL flag;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		/* log fatal error. */
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))) {

			if (susi != AVD_SU_SI_REL_NULL) {
				/* quiesced/quiescing for a single SI. Send a D2N-INFO_SU_SI_ASSIGN with
				 * removal for this SU for the SI. If SI in the SI admin pointer and
				 * has only this SU assignment, Change SI admin to LOCK. Remove 
				 * the SI from the SI admin pointer. If the SU is not in 
				 * the SU operation list, Add the SU to the operation list.
				 */

				old_fsm_state = susi->fsm;
				susi->fsm = AVD_SU_SI_STATE_UNASGN;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				if (avd_snd_susi_msg(cb, su, susi, AVSV_SUSI_ACT_DEL) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
					susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
					return NCSCC_RC_FAILURE;
				}

				if ((su->sg_of_su->admin_si != AVD_SI_NULL) &&
				    (su->sg_of_su->admin_si->list_of_sisu->su == su) &&
				    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
					avd_sg_su_oper_list_add(cb, su, FALSE);
					avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				}

			} else {	/* if (susi != AVD_SU_SI_REL_NULL) */

				/* quiesced all/quiescing all. Send a D2N-INFO_SU_SI_ASSIGN with removal all for
				 * this SU. If SI in the SI admin pointer has assignment only to
				 * this SU, Change SI admin to LOCK. Remove the SI from 
				 * the SI admin pointer. the SU is not in the SU operation list,
				 * Add the SU to the operation list. If the SU is in 
				 * the operation list, If SU admin state is shutdown, 
				 * change SU admin to LOCK.
				 */

				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}

				m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
				if (su->sg_of_su->admin_si != AVD_SI_NULL) {
					if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
					    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
						avd_sg_su_oper_list_add(cb, su, FALSE);
						avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
						m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					}
				} else if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
					if (flag == TRUE) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}

			}	/* else (susi != AVD_SU_SI_REL_NULL) */
		}		/* if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) ||
				   (state == SA_AMF_HA_QUIESCING))) */
		else {
			/* No action as other call back failure will cause operation disable 
			 * event to be sent by AvND.
			 */
			TRACE("%u", state);
		}

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:

		if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))) {

			/* The HA state is quiesced all/quiescing all or quiesced/quiescing
			 * for an SI that is the only SI 
			 * assigned and SU is in the operation list. If the admin state of the 
			 * SU is shutdown change it to lock. Send a D2N-INFO_SU_SI_ASSIGN with
			 * remove all to the SU. Change to SG_realign state.
			 */

			if ((susi == AVD_SU_SI_REL_NULL) ||
			    ((su->list_of_susi == susi) && (susi->su_next == AVD_SU_SI_REL_NULL))) {
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}

				if (su->sg_of_su->su_oper_list.su != su) {
					LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->list_of_susi->si->name.value,
									 su->list_of_susi->si->name.length);

					avd_sg_su_oper_list_add(cb, su, FALSE);
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					return NCSCC_RC_SUCCESS;
				}

				m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);

				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
					if (flag == TRUE) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}
			}
			/* if ((susi == AVD_SU_SI_REL_NULL) ||
			   ((su->list_of_susi == susi) && (susi->su_next == AVD_SU_SI_REL_NULL))) */
		}		/* if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) ||
				   (state == SA_AMF_HA_QUIESCING))) */
		else {
			/* No action as other call back failure will cause operation disable 
			 * event to be sent by AvND.
			 */
			TRACE("%u", state);
		}

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		if ((susi != AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
		    ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))) {
			/* message with modified quiesced/quiescing for a SI in the admin pointer.
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove for this SI to this SU. 
			 * If the SI has only one SU assignment, Remove the SI from the 
			 * admin pointer. Add the SU to operation list. If the SI admin state 
			 * is shutdown change it to LOCK. Change state to SG_realign.
			 */

			old_fsm_state = susi->fsm;
			susi->fsm = AVD_SU_SI_STATE_UNASGN;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
			if (avd_snd_susi_msg(cb, su, susi, AVSV_SUSI_ACT_DEL) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
				susi->fsm = old_fsm_state;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				return NCSCC_RC_FAILURE;
			}

			if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
			    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
				avd_sg_su_oper_list_add(cb, su, FALSE);
				avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}

		}
		/* if ((susi != AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) && 
		   ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))) */
		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:

		if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))) {
			/* If HA state is quiesced/quiescing all, Send a D2N-INFO_SU_SI_ASSIGN with 
			 * removal all for this SU.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

		}
		/* if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) ||
		   (state == SA_AMF_HA_QUIESCING))) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uns32)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;

}

 /*****************************************************************************
 * Function: avd_sg_nacvred_realign_func
 *
 * Purpose:  This function will call the chose assign function to check and
 * assign SIs. If any assigning is being done it adds the SUs to the operation
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

uns32 avd_sg_nacvred_realign_func(AVD_CL_CB *cb, AVD_SG *sg)
{
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

	if (avd_sg_nacvred_su_chose_asgn(cb, sg) == NULL) {
		/* all the assignments have already been done in the SG. */
		m_AVD_SET_SG_ADJUST(cb, sg, AVSV_SG_STABLE);
		avd_sg_app_su_inst_func(cb, sg);
		goto done;
	}

	/*  change the FSM state */
	m_AVD_SET_SG_ADJUST(cb, sg, AVSV_SG_STABLE);
	m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

 done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_nacvred_node_fail_func
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
 * NOTES: This is a N-way active redundancy model specific function.
 *
 * 
 **************************************************************************/

void avd_sg_nacvred_node_fail_func(AVD_CL_CB *cb, AVD_SU *su)
{
	NCS_BOOL flag;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL)
		return;

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		/* If the SU is an assigned SU, remove and free the SI assignment to 
		 * this SU. choose and assign SIs whose active assignment criteria is not 
		 * meet to in-service SUs, by sending D2N-INFO_SU_SI_ASSIGN message for 
		 * the SUs with role active for the SIs. Add the SUs to operation list and
		 * change state to SG_realign. If no assignment can be done, stay in the 
		 * stable state.
		 */

		avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

		if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) != NULL) {
			/* new assignments are been done in the SG. change the FSM state */
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}

		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		/* Remove all the SI relationships to this SU. If any SI in the 
		 * admin pointer, If the SI has only one SU assignment and its in 
		 * unassign state, Remove the SI from the admin pointer. If that SU
		 * is not in the operation list, Add that SU to operation list. 
		 * If the SI admin state is shutdown change it to LOCK. If (his SU in
		 * the operation list, Remove the SU from the operation list.
		 * If the SU operation list and the SI admin pointer are empty,
		 * choose and assign SIs whose active assignment criteria is not meet to
		 * in-service SUs, by sending D2N-INFO_SU_SI_ASSIGN message for the SUs
		 * with role active for the SIs. Add the SUs to operation list and stay 
		 * in the same state. If no assignment can be done, change the state to 
		 * stable state.
		 */

		avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

		if (su->sg_of_su->admin_si != AVD_SI_NULL) {
			if (su->sg_of_su->admin_si->list_of_sisu != AVD_SU_SI_REL_NULL) {
				if ((su->sg_of_su->admin_si->list_of_sisu->fsm
				     == AVD_SU_SI_STATE_UNASGN) &&
				    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
					avd_sg_su_oper_list_add(cb, su->sg_of_su->admin_si->list_of_sisu->su, FALSE);
					avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				}
			} else {
				avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			}

		}
		/* if (su->sg_of_su->admin_si != AVD_SI_NULL) */
		m_AVD_CHK_OPLIST(su, flag);

		if (flag == TRUE) {
			avd_sg_su_oper_list_del(cb, su, FALSE);

			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == TRUE) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}
		}

		if ((su->sg_of_su->admin_si == AVD_SI_NULL) && (su->sg_of_su->su_oper_list.su == NULL)) {
			if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == NULL) {
				/* No New assignments are been done in the SG. change the FSM state */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
			}
		}
		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:

		/* Remove the SI relationship to this SU. */
		avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

		if (su->sg_of_su->su_oper_list.su == su) {
			/*  SU is same as the SU in the list. If this SU admin is shutdown 
			 * change to LOCK. Remove the SU from operation list. choose and 
			 * assign SIs whose active assignment criteria is not meet to 
			 * in-service SUs, by sending D2N-INFO_SU_SI_ASSIGN message for the 
			 * SUs with role active for the SIs. Add the SUs to operation list and 
			 * change state to SG_realign. If no assignment can be done, change 
			 * the state to stable state. 
			 */
			avd_sg_su_oper_list_del(cb, su, FALSE);

			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == TRUE) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}

			if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == NULL) {
				/* No New assignments are been done in the SG. change the FSM state */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
			} else {
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}

		}
		/* if (su->sg_of_su->su_oper_list.su == su) */
		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		/* Remove the SI relationships to this SU. If the SI admin pointer, SI 
		 * has only one SU assignment to it and the state is un assign,
		 * If this SI admin is shutdown change to LOCK. Remove the SI from 
		 * SI admin pointer. Add the SU to operation list. Change state to SG_realign.
		 */
		avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

		if (su->sg_of_su->admin_si->list_of_sisu != AVD_SU_SI_REL_NULL) {
			if ((su->sg_of_su->admin_si->list_of_sisu->fsm
			     == AVD_SU_SI_STATE_UNASGN) &&
			    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
				avd_sg_su_oper_list_add(cb, su->sg_of_su->admin_si->list_of_sisu->su, FALSE);
				avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		} else {
			avd_si_admin_state_set((su->sg_of_su->admin_si), SA_AMF_ADMIN_LOCKED);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == NULL) {
				/* No New assignments are been done in the SG. change the FSM state */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
			} else {
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		}

		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:

		/* Remove the SI relationship to this SU. Remove the SU from the 
		 * SU operation list. If the SU operation list is empty,
		 * If this SG admin is shutdown change to LOCK. Change the SG FSM state 
		 * to stable.
		 */

		avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

		avd_sg_su_oper_list_del(cb, su, FALSE);

		if (su->sg_of_su->su_oper_list.su == NULL) {
			avd_sg_admin_state_set(su->sg_of_su, SA_AMF_ADMIN_LOCKED);

			/* change the FSM state */
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
			avd_sg_app_su_inst_func(cb, su->sg_of_su);
		}

		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uns32)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		return;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	return;
}

/*****************************************************************************
 * Function: avd_sg_nacvred_su_admin_fail
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
 * NOTES: This is a N-way active redundancy model specific function. The avnd pointer
 * value is valid only if this is a SU operation being done because of the node
 * admin change.
 *
 * 
 **************************************************************************/

uns32 avd_sg_nacvred_su_admin_fail(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd)
{
	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == SA_FALSE)) {
		return NCSCC_RC_FAILURE;
	}

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) ||
		    ((avnd != NULL) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED))) {

			/* change the state for all assignments to quiesced. */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			/* add the SU to the operation list and change the SG FSM to SU operation. */
			avd_sg_su_oper_list_add(cb, su, FALSE);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
		}		/* if ((su->admin_state == NCS_ADMIN_STATE_LOCK) ||
				   ((avnd != AVD_AVND_NULL) && (avnd->su_admin_state == NCS_ADMIN_STATE_LOCK))) */
		else if ((su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) ||
			 ((avnd != NULL) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN))) {
			/* change the state for all assignments to quiescing. */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCING) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			/* add the SU to the operation list and change the SG FSM to SU operation. */
			avd_sg_su_oper_list_add(cb, su, FALSE);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
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
			/* change the  assignment to quiesced. */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}
		}
		break;		/* case AVD_SG_FSM_SU_OPER: */
	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uns32)su->sg_of_su->sg_fsm_state));
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch (su->sg_of_su->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_nacvred_si_admin_down
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
 * NOTES: This is a N-way active redundancy model specific function.
 *
 * 
 **************************************************************************/

uns32 avd_sg_nacvred_si_admin_down(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *i_susi;

	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == SA_FALSE)) {
		return NCSCC_RC_FAILURE;
	}
	if (si->list_of_sisu == AVD_SU_SI_REL_NULL) {
		return NCSCC_RC_SUCCESS;
	}
	switch (si->sg_of_si->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if (si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED) {
			/* SI lock. Send D2N-INFO_SU_SI_ASSIGN modify quiesced for this SI to
			 * each of the SUs to which it is assigned. Change state to 
			 * SI_operation state. Add it to admin SI pointer.
			 */
			i_susi = si->list_of_sisu;
			while (i_susi != AVD_SU_SI_REL_NULL) {
				i_susi->state = SA_AMF_HA_QUIESCED;
				i_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, i_susi);
				if (avd_snd_susi_msg(cb, i_susi->su, i_susi, AVSV_SUSI_ACT_MOD) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_susi->su->name.value,
									 i_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, si->name.value, si->name.length);
				}

				i_susi = i_susi->si_next;
			}

			/* add the SI to the admin list and change the SG FSM to SI operation. */
			m_AVD_SET_SG_ADMIN_SI(cb, si);
			m_AVD_SET_SG_FSM(cb, (si->sg_of_si), AVD_SG_FSM_SI_OPER);
		} /* if (si->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SI shutdown. Send D2N-INFO_SU_SI_ASSIGN modify quiescing for this SI to
			 * each of the SUs to which it is assigned. Change state to 
			 * SI_operation state. Add it to admin SI pointer.
			 */
			i_susi = si->list_of_sisu;
			while (i_susi != AVD_SU_SI_REL_NULL) {
				i_susi->state = SA_AMF_HA_QUIESCING;
				i_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, i_susi);
				if (avd_snd_susi_msg(cb, i_susi->su, i_susi, AVSV_SUSI_ACT_MOD) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_susi->su->name.value,
									 i_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, si->name.value, si->name.length);
				}

				i_susi = i_susi->si_next;
			}

			/* add the SI to the admin list and change the SG FSM to SI operation. */
			m_AVD_SET_SG_ADMIN_SI(cb, si);
			m_AVD_SET_SG_FSM(cb, (si->sg_of_si), AVD_SG_FSM_SI_OPER);
		}		/* if (si->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SI_OPER:
		if ((si->sg_of_si->admin_si == si) && (si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED)) {
			/* If the SI is in the admin pointer and the SI admin state is shutdown,
			 * change the admin state of the SI to lock and 
			 * send D2N-INFO_SU_SI_ASSIGN modify quiesced messages to all the SUs 
			 * that is being assigned quiescing state for the SI.
			 */

			i_susi = si->list_of_sisu;
			while (i_susi != AVD_SU_SI_REL_NULL) {
				if ((i_susi->state != SA_AMF_HA_QUIESCED) && (i_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
					i_susi->state = SA_AMF_HA_QUIESCED;
					i_susi->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, i_susi);
					if (avd_snd_susi_msg(cb, i_susi->su, i_susi, AVSV_SUSI_ACT_MOD) ==
					    NCSCC_RC_FAILURE) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_susi->su->name.value,
										 i_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, si->name.value, si->name.length);
					}
				}

				i_susi = i_susi->si_next;
			}
		}
		break;		/* case AVD_SG_FSM_SI_OPER: */
	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uns32)si->sg_of_si->sg_fsm_state));
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, si->name.value, si->name.length);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch (si->sg_of_si->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_nacvred_sg_admin_down
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
 * NOTES: This is a N-way active redundancy model specific function.
 *
 * 
 **************************************************************************/

uns32 avd_sg_nacvred_sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SU *i_su;
	AVD_SG_OPER *l_suopr;

	TRACE_ENTER2("%u", sg->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == SA_FALSE)) {
		return NCSCC_RC_FAILURE;
	}

	switch (sg->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* SG lock. Identify all the assigned SUs, send D2N-INFO_SU_SI_ASSIGN
			 * modify quiesced all for each of the SU. Add them to 
			 * the SU operation list. Change state to SG_admin. 
			 * If no assigned SU exist, no action, stay in stable state.
			 */

			i_su = sg->list_of_su;
			while (i_su != NULL) {
				if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
					avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_QUIESCED);

					/* add the SU to the operation list */
					avd_sg_su_oper_list_add(cb, i_su, FALSE);
				}

				i_su = i_su->sg_list_su_next;
			}

		} /* if (sg->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (sg->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SG shutdown. Identify all the assigned SUs, send D2N-INFO_SU_SI_ASSIGN
			 * modify quiescing all for each of the SU. Add them to 
			 * the SU operation list. Change state to SG_admin. 
			 * If no assigned SU exist, no action, stay in stable state.
			 */
			i_su = sg->list_of_su;
			while (i_su != NULL) {
				if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
					avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_QUIESCING);

					/* add the SU to the operation list */
					avd_sg_su_oper_list_add(cb, i_su, FALSE);
				}

				i_su = i_su->sg_list_su_next;
			}

		} /* if (sg->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		else {
			return NCSCC_RC_FAILURE;
		}

		if (sg->su_oper_list.su != NULL) {
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_ADMIN);
		}

		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_ADMIN:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* If the SG admin state is shutdown, change the admin state of the 
			 * SG to lock and send D2N-INFO_SU_SI_ASSIGN modify quiesced message
			 * to all the SUs in the SU operation list with quiescing assignment.
			 */
			if (sg->su_oper_list.su != NULL) {
				i_su = sg->su_oper_list.su;
				if ((i_su->list_of_susi->state == SA_AMF_HA_QUIESCING) &&
				    (i_su->list_of_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {
					avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_QUIESCED);
				}

				l_suopr = i_su->sg_of_su->su_oper_list.next;
				while (l_suopr != NULL) {
					if ((l_suopr->su->list_of_susi->state == SA_AMF_HA_QUIESCING) &&
					    (l_suopr->su->list_of_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {
						avd_sg_su_si_mod_snd(cb, l_suopr->su, SA_AMF_HA_QUIESCED);
					}

					l_suopr = l_suopr->next;
				}
			}

		}		/* if (sg->admin_state == NCS_ADMIN_STATE_LOCK) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uns32)sg->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, sg->name.value, sg->name.length);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch (sg->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}
