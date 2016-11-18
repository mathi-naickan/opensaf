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
  the SG state machine, for processing the events related to SG for No
  redundancy model.

******************************************************************************
*/

#include "base/logtrace.h"

#include "amf/amfd/amfd.h"
#include "amf/amfd/si_dep.h"

/**
 * Identifies unassigned SIs, searches for unassigned in-service SUs and
 * assigns the unassigned SIs to the SUs.
 *
 * Returns: pointer to the first SU that is undergoing assignment. Null if
 *          no assignments need to happen.
 */
AVD_SU *SG_NORED::assign_sis_to_sus() {
	AVD_SU_SI_REL *tmp;

	TRACE_ENTER();

	avd_sidep_update_si_dep_state_for_all_sis(this);
	for (const auto& i_si : list_of_si) {

		/* verify that the SI is unassigned and ready */
		if ((i_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) ||
		    (i_si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED) ||
		    (i_si->si_dep_state == AVD_SI_READY_TO_UNASSIGN) ||
		    (i_si->si_dep_state == AVD_SI_UNASSIGNING_DUE_TO_DEP) ||
			(i_si->list_of_csi == nullptr) ||
		    (i_si->list_of_sisu != nullptr)) {
			continue;
		}

		auto iter = list_of_su.begin();

		/* identify a in-service unassigned SU so that the SI can be assigned. */
		for (; iter != list_of_su.end(); ++iter) {
			const auto& su = *iter;
			if ((su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
			    (su->list_of_susi == nullptr)) {
				/* if the SU is not null assign active to that SU for the SI. */
				if (avd_new_assgn_susi(avd_cb, su, i_si, SA_AMF_HA_ACTIVE, false, &tmp) == NCSCC_RC_SUCCESS) {
					su_oper_list_add(su);
					break;
				} else {
					LOG_ER("%s:%u: %s", __FILE__, __LINE__, i_si->name.c_str());
					// try the next SU
				}
			}
		}

		if (iter == list_of_su.end()) {
			break;
		}
	}

	TRACE_LEAVE();
	if (su_oper_list.empty() == false)
		return su_oper_list.front();
	else
		return nullptr;
}

uint32_t SG_NORED::si_assign(AVD_CL_CB *cb, AVD_SI *si) {
	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);

	/* If the SG FSM state is not stable just return success. */
	if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
		return NCSCC_RC_SUCCESS;
	}

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == false)) {
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, si->sg_of_si->sg_ncs_spec);
		return NCSCC_RC_SUCCESS;
	}

	if (assign_sis_to_sus() == nullptr) {
		/* all the assignments have already been done in the SG. */
		return NCSCC_RC_SUCCESS;
	}

	set_fsm_state(AVD_SG_FSM_SG_REALIGN);

	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NORED::su_fault(AVD_CL_CB *cb, AVD_SU *su) {
	bool flag;
	AVD_AVND *su_node_ptr = nullptr;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL)
		return NCSCC_RC_SUCCESS;

	/* Do the functionality based on the current state. */
	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		/* change the state of the SI to quiesced */
		if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
			return NCSCC_RC_FAILURE;

		su_oper_list_add(su);
		set_fsm_state(AVD_SG_FSM_SU_OPER);
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if (su->list_of_susi->si == su->sg_of_su->admin_si) {
			/* The SUs SI is in the SI admin pointer. If this SI admin is
			 * shutdown change to LOCK and send D2N-INFO_SU_SI_ASSIGN with
			 * quiesced to the SU. Remove the SI from SI admin pointer.
			 * Add the SU to operation list and stay in the same state.
			 */
			if (su->list_of_susi->si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
					return NCSCC_RC_FAILURE;

				su->list_of_susi->si->set_admin_state(SA_AMF_ADMIN_LOCKED);
			}

			su_oper_list_add(su);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
		} else {	/* if (su->list_of_susi->si == su->sg_of_su->admin_si) */

			m_AVD_CHK_OPLIST(su, flag);
			if (flag == true) {
				/* The SU is same as the SU in the list. If the HA state assignment
				 * for the SU is active, Send D2N-INFO_SU_SI_ASSIGN with quiesced
				 * to the SU. Stay in the same state. If the HA state assignment
				 * for the SU is quiescing. Send D2N-INFO_SU_SI_ASSIGN with
				 * quiesced to the SU. Stay in the same state. Change admin state
				 * of SU to LOCK. For other HA states, Stay in same state. 
				 */

				if ((su->list_of_susi->state == SA_AMF_HA_ACTIVE) ||
				    (su->list_of_susi->state == SA_AMF_HA_QUIESCING)) {
					if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
						return NCSCC_RC_FAILURE;

					su_node_ptr = su->get_node_ptr();
					if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
						su->set_admin_state(SA_AMF_ADMIN_LOCKED);
					} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
						if (su_node_ptr->is_node_lock() == true) {
							node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
						}
					}
				}

			} else {	/* if (flag == true) */

				/* The SU is not part of the SU operation list and the SUs SI is
				 * not part of the SI admin pointer. Send D2N-INFO_SU_SI_ASSIGN with
				 * quiesced to the SU.  Add the SU to the SU operation list. 
				 * Stay in the same state.
				 */
				if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
					return NCSCC_RC_FAILURE;

				su_oper_list_add(su);

			}	/* else (flag == true) */

		}		/* else (su->list_of_susi->si == su->sg_of_su->admin_si) */

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:
		if (su_oper_list_front() == su) {
			/* The SU is same as the SU in the list. If the SI relationships to
			 * the SU is quiescing change the SU admin to LOCK. 
			 * Send D2N-INFO_SU_SI_ASSIGN modify quiesced.
			 */
			if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
				if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
					return NCSCC_RC_FAILURE;

				su_node_ptr = su->get_node_ptr();
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					su->set_admin_state(SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					if (su_node_ptr->is_node_lock() == true) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}
			}	/* if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) */
		} else {	/* if(su->sg_of_su->su_oper_list.su == su) */

			/* The SU is not the same as the SU in the list.
			 * Send D2N-INFO_SU_SI_ASSIGN modify quiesced to the SU. Add the SU to
			 * the operation list and change state to SG_realign state.the SU is
			 * not the same as the SU in the list.
			 */
			if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;

			su_oper_list_add(su);
			set_fsm_state(AVD_SG_FSM_SG_REALIGN);
		}		/* else (su->sg_of_su->su_oper_list.su == su) */

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		if (su->sg_of_su->admin_si == su->list_of_susi->si) {
			/* The SI on this nodes SU is in the SI admin pointer. If this 
			 * SI admin is shutdown change to LOCK and send D2N-INFO_SU_SI_ASSIGN
			 * with quiesced to the SU. Remove the SI from SI admin pointer.
			 * Add the SU to operation list and change state to SU_operation. 
			 */
			if (su->sg_of_su->admin_si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
					return NCSCC_RC_FAILURE;
			}

			su_oper_list_add(su);
			su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			set_fsm_state(AVD_SG_FSM_SU_OPER);
		} else {	/* if (su->sg_of_su->admin_si == su->list_of_susi->si) */

			/* Send D2N-INFO_SU_SI_ASSIGN with quiesced to the SU. Add the SU to
			 * the SU operation list. Change the state to SG_realign state.
			 */
			if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;

			su_oper_list_add(su);
			set_fsm_state(AVD_SG_FSM_SG_REALIGN);
		}		/* else (su->sg_of_su->admin_si == su->list_of_susi->si) */

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
				if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
					return NCSCC_RC_FAILURE;
			}
			/* if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) */
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

uint32_t SG_NORED::su_insvc(AVD_CL_CB *cb, AVD_SU *su) {
	TRACE_ENTER2("'%s', %u", su->name.c_str(), su->sg_of_su->sg_fsm_state);

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

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == false)) {
		return NCSCC_RC_SUCCESS;
	}

	if (assign_sis_to_sus() == nullptr) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);

		/* all the assignments have already been done in the SG. */
		return NCSCC_RC_SUCCESS;
	}

	set_fsm_state(AVD_SG_FSM_SG_REALIGN);
	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NORED::susi_success(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
				   AVSV_SUSI_ACT act, SaAmfHAStateT state) {
	AVD_SI *l_si;
	AVD_AVND *su_node_ptr = nullptr;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);
	osafassert(su->sg_of_su == this);	

	switch (sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		/* Do the action specified in the message if delete else no action. */
		if (act == AVSV_SUSI_ACT_DEL) {
			/* free all the CSI assignments  */
			avd_compcsi_delete(cb, su->list_of_susi, false);
			/* Unassign the SUSI */
			m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);
		}
		/* log informational error. */
		LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if (act == AVSV_SUSI_ACT_DEL) {
			/* Action is remove. Remove the SI relationship to this SU. If this SI  
			 * is in the SI admin pointer. Change SI admin to LOCK. Remove the SI
			 * from the SI admin pointer. If this SU is in the operation list,
			 * Remove the SU from the SU operation list. If the SU operation list 
			 * and the SI admin pointer are empty. Choose and assign 
			 * unassigned SIs to unassigned in-service SUs .
			 */

			l_si = su->list_of_susi->si;

			/* free all the CSI assignments  */
			avd_compcsi_delete(cb, su->list_of_susi, false);
			/* Unassign the SUSI */
			m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);

			if (admin_si == l_si) {
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			} else {
				su_oper_list_del(su);
			}

			if ((admin_si == AVD_SI_NULL) && (su_oper_list.empty() == true)) {
				if (assign_sis_to_sus() == nullptr) {
					/* No New assignments are been done in the SG */
					set_fsm_state(AVD_SG_FSM_STABLE);
					avd_sidep_sg_take_action(this); 
					avd_sg_app_su_inst_func(cb, this);
				}

			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if (state == SA_AMF_HA_QUIESCED) {
			/* The state is quiesced. Send a D2N-INFO_SU_SI_ASSIGN with removal
			 * for this SU. If the SI is in the SI admin pointer.
			 * Change SI admin to LOCK. Remove the SI from the SI admin pointer. 
			 * Add the SU to the operation list. If the SU is in the operation list
			 * and SU admin state is shutdown, change SU admin to LOCK.
			 */

			if (avd_susi_del_send(su->list_of_susi) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;

			su_node_ptr = su->get_node_ptr();

			if (su->sg_of_su->admin_si == su->list_of_susi->si) {
				su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				su_oper_list_add(su);
			} else if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				su->set_admin_state(SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				if (su_node_ptr->is_node_lock() == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}
		} /* if (state == SA_AMF_HA_QUIESCED) */
		else if (state == SA_AMF_HA_ACTIVE) {
			/* the HA state is active. Remove the SU from the SU operation list.
			 * If the SU operation list and the SI admin pointer are empty,
			 * Choose and assign unassigned SIs to unassigned in-service SUs.
			 */

			su_oper_list_del(su);

			if (su_oper_list.empty() == true) {
				if (assign_sis_to_sus() == nullptr) {
					/* No New assignments are been done in the SG */
					set_fsm_state(AVD_SG_FSM_STABLE);
					avd_sidep_sg_take_action(this); 
					avd_sg_app_su_inst_func(cb, this);
				}
			}
		}

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:
		if (act == AVSV_SUSI_ACT_DEL) {
			/* log error. Action is remove. Remove the SI relationship to this SU. 
			 * If this SU is in the operation list,
			 * Remove the SU from the SU operation list. If the SU operation list 
			 * is empty. Choose and assign unassigned SIs to unassigned in-service SUs .
			 */

			LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
			LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->list_of_susi->si->name.c_str(),
							 su->list_of_susi->si->name.length());

			/* free all the CSI assignments  */
			avd_compcsi_delete(cb, su->list_of_susi, false);
			/* Unassign the SUSI */
			m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);

			su_oper_list_del(su);

			if (su->sg_of_su->su_oper_list.empty() == true) {
				if (assign_sis_to_sus() != nullptr) {
					/* New assignments are been done in the SG. */
					set_fsm_state(AVD_SG_FSM_SG_REALIGN);
					return NCSCC_RC_SUCCESS;
				}

				set_fsm_state(AVD_SG_FSM_STABLE);
				avd_sidep_sg_take_action(su->sg_of_su); 
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if (state == SA_AMF_HA_QUIESCED) {
			/* The HA state is quiesced and SU is in the operation list.
			 * If the admin state of the SU is shutdown change it to lock.
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove to the SU. Change to 
			 * SG_realign state.
			 */
			if (avd_susi_del_send(su->list_of_susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			if (su_oper_list_front() != su) {
				LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->list_of_susi->si->name.c_str(),
								 su->list_of_susi->si->name.length());

				su_oper_list_add(su);
				set_fsm_state(AVD_SG_FSM_SG_REALIGN);
				return NCSCC_RC_SUCCESS;
			}

			set_fsm_state(AVD_SG_FSM_SG_REALIGN);

			su_node_ptr = su->get_node_ptr();

			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				su->set_admin_state(SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				if (su_node_ptr->is_node_lock() == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}
		}
		/* if (state == SA_AMF_HA_QUIESCED) */
		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		if (act == AVSV_SUSI_ACT_DEL) {
			/* log error. Action is remove. Remove the SI relationship to this SU. 
			 * If this SI is in the admin pointer,
			 * Remove the SI from the SI admin pointer. If the SI admin pointer 
			 * is empty. Choose and assign unassigned SIs to unassigned in-service SUs .
			 */

			LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
			LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->list_of_susi->si->name.c_str(),
							 su->list_of_susi->si->name.length());

			l_si = su->list_of_susi->si;

			/* free all the CSI assignments  */
			avd_compcsi_delete(cb, su->list_of_susi, false);
			/* Unassign the SUSI */
			m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);

			if (su->sg_of_su->admin_si == l_si) {
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			}

			if (su->sg_of_su->admin_si == AVD_SI_NULL) {
				if (assign_sis_to_sus() != nullptr) {
					/* New assignments are been done in the SG. */
					set_fsm_state(AVD_SG_FSM_SG_REALIGN);
					return NCSCC_RC_SUCCESS;
				}

				set_fsm_state(AVD_SG_FSM_STABLE);
				avd_sidep_sg_take_action(su->sg_of_su); 
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if (state == SA_AMF_HA_QUIESCED) {
			/* message with modified quiesced for a SI in the admin pointer.
			 * If the SI admin state is shutdown change it to LOCK.
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove for this SI to this SU. 
			 * Remove the SI from the admin pointer. Add the SU to operation list. 
			 * Change state to SG_realign.
			 */

			if (avd_susi_del_send(su->list_of_susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			if (su->sg_of_su->admin_si != su->list_of_susi->si) {
				LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->list_of_susi->si->name.c_str(),
								 su->list_of_susi->si->name.length());

				su_oper_list_add(su);
				set_fsm_state(AVD_SG_FSM_SG_REALIGN);
				return NCSCC_RC_SUCCESS;
			}

			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			su_oper_list_add(su);
			su->list_of_susi->si->set_admin_state(SA_AMF_ADMIN_LOCKED);
			set_fsm_state(AVD_SG_FSM_SG_REALIGN);

		}
		/* if (state == SA_AMF_HA_QUIESCED) */
		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:

		if (act == AVSV_SUSI_ACT_DEL) {
			/* The action is remove. Remove the SI relationship to this SU.
			 * Remove the SU from the SU operation list. If the SU operation list
			 * is empty. If this SG admin is shutdown change to LOCK. Change 
			 * the SG FSM state to stable.
			 */

			/* free all the CSI assignments  */
			avd_compcsi_delete(cb, su->list_of_susi, false);
			/* Unassign the SUSI */
			m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);

			su_oper_list_del(su);

			if (su_oper_list.empty() == true) {
				avd_sg_admin_state_set(this, SA_AMF_ADMIN_LOCKED);

				set_fsm_state(AVD_SG_FSM_STABLE);
				/*As sg is stable, screen for si dependencies and take action on whole sg*/
				avd_sidep_update_si_dep_state_for_all_sis(this);
				avd_sidep_sg_take_action(this); 
				avd_sg_app_su_inst_func(cb, this);
			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if (state == SA_AMF_HA_QUIESCED) {
			/* If HA state is quiesced. Send a D2N-INFO_SU_SI_ASSIGN with removal
			 * for this SU. Since when the removal is done for all the SUs
			 * we change the state to LOCK no need to scan the list to make the SG
			 * state to LOCK here.
			 */
			if (avd_susi_del_send(su->list_of_susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;
		}
		/* if (state == SA_AMF_HA_QUIESCED) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NORED::susi_failed(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
				  AVSV_SUSI_ACT act, SaAmfHAStateT state) {
	AVD_AVND *su_node_ptr = nullptr;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		/* log fatal error. */
		LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))) {
			/* The HA state is quiesced/quiescing. Send a 
			 * D2N-INFO_SU_SI_ASSIGN with removal for this SU. If SI in the 
			 * SI admin pointer, Change SI admin to LOCK. Remove the SI from
			 * the SI admin pointer. Add the SU to the operation list. 
			 * If the SU is in the operation list then. If SU admin state is 
			 * shutdown, change SU admin to LOCK.
			 */
			if (avd_susi_del_send(su->list_of_susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			su_node_ptr = su->get_node_ptr();

			if (su->sg_of_su->admin_si == su->list_of_susi->si) {
				su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				su_oper_list_add(su);
			} else if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				su->set_admin_state(SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				su->list_of_susi->state = SA_AMF_HA_QUIESCED;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, (su->list_of_susi), AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, su->list_of_susi);
				if (su_node_ptr->is_node_lock() == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}
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
			/* If HA state is quiesced/quiescing and SU is in the operation list.
			 * If the admin state of the SU is shutdown change it to lock.
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove to the SU. 
			 * Change to SG_realign state.
			 */
			if (avd_susi_del_send(su->list_of_susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			su_node_ptr = su->get_node_ptr();
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				su->set_admin_state(SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				su->list_of_susi->state = SA_AMF_HA_QUIESCED;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, (su->list_of_susi), AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, su->list_of_susi);
				if (su_node_ptr->is_node_lock() == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}
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

		if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))) {
			/* If message with modified quiesced/quiescing for a SI in the
			 * admin pointer. If the SI admin state is shutdown change it to LOCK.
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove for this SI to this SU. 
			 * Remove the SI from the admin pointer. Add the SU to operation list. 
			 * Change state to SG_realign.
			 */
			if (avd_susi_del_send(su->list_of_susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			if (su->sg_of_su->admin_si != su->list_of_susi->si) {
				LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->list_of_susi->si->name.c_str(),
								 su->list_of_susi->si->name.length());

				su_oper_list_add(su);
				set_fsm_state(AVD_SG_FSM_SG_REALIGN);
				return NCSCC_RC_SUCCESS;
			}

			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			su_oper_list_add(su);
			su->list_of_susi->si->set_admin_state(SA_AMF_ADMIN_LOCKED);
			set_fsm_state(AVD_SG_FSM_SG_REALIGN);
		}
		/* if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) ||
		   (state == SA_AMF_HA_QUIESCING))) */
		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:

		if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))) {
			/* If HA state is quiesced/quiescing. Send a D2N-INFO_SU_SI_ASSIGN 
			 * with removal for this SU. The SG admin state can be changed to LOCK
			 * when all the SUs are unassigned.
			 */
			if (avd_susi_del_send(su->list_of_susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;
		}
		/* if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) ||
		   (state == SA_AMF_HA_QUIESCING))) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;

}

uint32_t SG_NORED::realign(AVD_CL_CB *cb, AVD_SG *sg) {
	TRACE_ENTER2("'%s'", sg->name.c_str());

	/* If the SG FSM state is not stable just return success. */
	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == false)) {
		goto done;
	}

	if (sg->sg_fsm_state != AVD_SG_FSM_STABLE) {
		set_adjust_state(AVSV_SG_STABLE);
		avd_sg_app_su_inst_func(cb, sg);
		goto done;
	}

	if (assign_sis_to_sus() == nullptr) {
		/* all the assignments have already been done in the SG. */
		set_adjust_state(AVSV_SG_STABLE);
		avd_sg_app_su_inst_func(cb, sg);
		goto done;
	}

	set_adjust_state(AVSV_SG_STABLE);
	set_fsm_state(AVD_SG_FSM_SG_REALIGN);

 done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

void SG_NORED::node_fail(AVD_CL_CB *cb, AVD_SU *su) {

	AVD_SI *l_si;
	SaAmfHAStateT old_state;
	AVD_AVND *su_node_ptr = nullptr;

	TRACE_ENTER2("%s, sg_fsm_state=%u", su->name.c_str(), su->sg_of_su->sg_fsm_state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL)
		return;

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		/* free all the CSI assignments  */
		avd_compcsi_delete(cb, su->list_of_susi, false);
		/* Unassign the SUSI */
		m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);

		if (assign_sis_to_sus() != nullptr) {
			/* new assignments are been done in the SG. */
			set_fsm_state(AVD_SG_FSM_SG_REALIGN);
		}
		else
			avd_sidep_sg_take_action(su->sg_of_su); 

		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		/* If the SU is same as the SU in the list,Remove the SI relationship
		 * to this SU. Remove the SU from operation list. If 
		 * the SUs SI assignment is the same as that in the SI admin pointer,
		 * Remove the SI relationship to this SU. Remove the SI from
		 * SI admin pointer. If the SI admin state is shutdown change to LOCK.
		 * If SU operation list and SI admin list is empty, Choose and assign
		 * unassigned SIs to unassigned in-service SUs. If the SU is not part of
		 * the SU operation list or the SUs SI is not part of the SI admin pointer.
		 * Remove the SI relationship to this SU.
		 */
		l_si = su->list_of_susi->si;
		old_state = su->list_of_susi->state;

		/* free all the CSI assignments  */
		avd_compcsi_delete(cb, su->list_of_susi, false);
		/* Unassign the SUSI */
		m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);

		if (su->sg_of_su->admin_si == l_si) {
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			l_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
		} else {
			if (in_su_oper_list(su) == true) {
				su_oper_list_del(su);

				su_node_ptr = su->get_node_ptr();

				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					su->set_admin_state(SA_AMF_ADMIN_LOCKED);
				} else if ((su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) &&
					   (old_state == SA_AMF_HA_QUIESCING)) {
					if (su_node_ptr->is_node_lock() == true) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}
			}
		}

		if ((admin_si == AVD_SI_NULL) && (su_oper_list.empty() == true)) {
			if (assign_sis_to_sus() == nullptr) {
				/* No New assignments are been done in the SG */
				set_fsm_state(AVD_SG_FSM_STABLE);
				avd_sidep_sg_take_action(this); 
				avd_sg_app_su_inst_func(cb, this);
			}

		}
		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:

		/* If the SU is same as the SU in the list, Remove the SI relationship
		 * to this SU. If this SU admin is shutdown change to LOCK.
		 * Remove the SU from operation list. Choose and 
		 * assign unassigned SIs to unassigned in-service SUs. If the SU is not
		 * the same as the SU in the list, Remove the SI relationship to this SU.
		 * Stay in the same state.
		 */
		old_state = su->list_of_susi->state;

		/* free all the CSI assignments  */
		avd_compcsi_delete(cb, su->list_of_susi, false);
		/* Unassign the SUSI */
		m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);

		if (su_oper_list_front() == su) {
			su_oper_list_del(su);

			su_node_ptr = su->get_node_ptr();

			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				su->set_admin_state(SA_AMF_ADMIN_LOCKED);
			} else if ((su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) &&
				   (old_state == SA_AMF_HA_QUIESCING)) {
				if (su_node_ptr->is_node_lock() == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}

			if (assign_sis_to_sus() != nullptr) {
				/* New assignments are been done in the SG */
				set_fsm_state(AVD_SG_FSM_SG_REALIGN);
				return;
			}

			set_fsm_state(AVD_SG_FSM_STABLE);
			/*As sg is stable, screen for si dependencies and take action on whole sg*/
			avd_sidep_update_si_dep_state_for_all_sis(su->sg_of_su);
			avd_sidep_sg_take_action(su->sg_of_su); 
			avd_sg_app_su_inst_func(cb, su->sg_of_su);
		}

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		/* If the SI on this nodes SU is in the SI admin pointer.  Remove the
		 * SI relationship to this SU. If this SI admin is shutdown change
		 * to LOCK. Remove the SI from SI admin pointer. Choose and assign
		 * unassigned SIs to unassigned in-service SUs. If not the same SI,
		 * Remove the SI relationship to this SU. Stay in the same state.
		 */

		l_si = su->list_of_susi->si;
		old_state = su->list_of_susi->state;

		/* free all the CSI assignments  */
		avd_compcsi_delete(cb, su->list_of_susi, false);
		/* Unassign the SUSI */
		m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);

		if (su->sg_of_su->admin_si == l_si) {
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			l_si->set_admin_state(SA_AMF_ADMIN_LOCKED);

			if (assign_sis_to_sus() != nullptr) {
				/* New assignments are been done in the SG */
				set_fsm_state(AVD_SG_FSM_SG_REALIGN);
				return;
			}

			set_fsm_state(AVD_SG_FSM_STABLE);
			avd_sidep_sg_take_action(su->sg_of_su); 
			avd_sg_app_su_inst_func(cb, su->sg_of_su);
		}

		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:

		/* Remove the SI relationship to this SU. Remove the SU from
		 * the SU operation list. If the SU operation list is empty,
		 * If this SG admin is shutdown change to LOCK. Change the
		 * SG FSM state to stable.
		 */

		/* free all the CSI assignments  */
		avd_compcsi_delete(cb, su->list_of_susi, false);
		/* Unassign the SUSI */
		m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);

		su_oper_list_del(su);

		if (su_oper_list.empty() == true) {
			avd_sg_admin_state_set(this, SA_AMF_ADMIN_LOCKED);
			set_fsm_state(AVD_SG_FSM_STABLE);
			/*As sg is stable, screen for si dependencies and take action on whole sg*/
			avd_sidep_update_si_dep_state_for_all_sis(this);
			avd_sidep_sg_take_action(this); 
			avd_sg_app_su_inst_func(cb, this);
		}

		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
		return;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	TRACE_LEAVE();
}

uint32_t SG_NORED::su_admin_down(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd) {
	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == false)) {
		return NCSCC_RC_FAILURE;
	}

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) ||
		    ((avnd != nullptr) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED))) {

			/* change the  assignment to quiesced. */
			if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			su_oper_list_add(su);
			set_fsm_state(AVD_SG_FSM_SU_OPER);
		}		/* if ((su->admin_state == NCS_ADMIN_STATE_LOCK) ||
				   ((avnd != AVD_AVND_NULL) && (avnd->su_admin_state == NCS_ADMIN_STATE_LOCK))) */
		else if ((su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) ||
			 ((avnd != nullptr) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN))) {
			/* change the  assignment to quiesced. */
			if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCING) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			su_oper_list_add(su);
			set_fsm_state(AVD_SG_FSM_SU_OPER);
		}		/* if ((su->admin_state == NCS_ADMIN_STATE_SHUTDOWN) ||
				   ((avnd != AVD_AVND_NULL) && (avnd->su_admin_state == NCS_ADMIN_STATE_SHUTDOWN))) */
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SU_OPER:
		if ((su_oper_list_front() == su) &&
		    (su->list_of_susi->state == SA_AMF_HA_QUIESCING) &&
		    ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) ||
		     ((avnd != nullptr) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)))) {
			/* If the SU is in the operation list and the SU admin state is lock.
			 * send D2N-INFO_SU_SI_ASSIGN modify quiesced message to the SU. 
			 */
			/* change the  assignment to quiesced. */
			if (avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;
		}
		break;		/* case AVD_SG_FSM_SU_OPER: */
	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch (su->sg_of_su->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NORED::si_admin_down(AVD_CL_CB *cb, AVD_SI *si) {
	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == false)) {
		return NCSCC_RC_FAILURE;
	}
	if (si->list_of_sisu == AVD_SU_SI_REL_NULL) {
		return NCSCC_RC_SUCCESS;
	}
	switch (si->sg_of_si->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if (si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED) {
			/* SI lock. Send D2N-INFO_SU_SI_ASSIGN modify quiesced for this SI. 
			 * Change state to SI_operation state. Add it to admin SI pointer.
			 */
			/* change the  assignment to quiesced. */
			if (avd_susi_mod_send(si->list_of_sisu, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			m_AVD_SET_SG_ADMIN_SI(cb, si);
			set_fsm_state(AVD_SG_FSM_SI_OPER);
		} /* if (si->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SI shutdown. Send D2N-INFO_SU_SI_ASSIGN modify quiescing for this SI.
			 * Change state to SI_operation state. Add it to admin SI pointer.
			 */

			/* change the state of the SI to quiescing */
			if (avd_susi_mod_send(si->list_of_sisu, SA_AMF_HA_QUIESCING) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			m_AVD_SET_SG_ADMIN_SI(cb, si);
			set_fsm_state(AVD_SG_FSM_SI_OPER);
		}		/* if (si->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SI_OPER:
		if ((si->sg_of_si->admin_si == si) && (si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED)) {
			/* If the SI is in the admin pointer and the SI admin state is shutdown,
			 * change the admin state of the SI to lock and 
			 * send D2N-INFO_SU_SI_ASSIGN modify quiesced message to the SU that
			 * is being assigned quiescing state.
			 */
			/* change the state of the SI to quiesced */
			if (avd_susi_mod_send(si->list_of_sisu, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;
		}
		break;		/* case AVD_SG_FSM_SI_OPER: */
	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)si->sg_of_si->sg_fsm_state));
		LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, si->name.c_str(), si->name.length());
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch (si->sg_of_si->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NORED::sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg) {
	TRACE_ENTER2("%u", sg->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == false)) {
		return NCSCC_RC_FAILURE;
	}

	switch (sg->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* SG lock. Identify all the assigned SUs, send D2N-INFO_SU_SI_ASSIGN 
			 * modify quiesced for each of the SU w.r.t the corresponding SI. 
			 * Add them to the SU operation list. Change state to SG_admin. 
			 * If no assigned SU exist, no action, stay in stable state.
			 */

			for (const auto& su : sg->list_of_su) {
				if (su->list_of_susi != AVD_SU_SI_REL_NULL) {
					avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCED);
					su_oper_list_add(su);
				}
			}
		} /* if (sg->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (sg->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SG shutdown. Identify all the assigned SUs, 
			 * send D2N-INFO_SU_SI_ASSIGN modify quiescing for each of the SU w.r.t
			 * the corresponding SI. Add the SUs to the SU operation list. 
			 * Change state to SG_admin. If no assigned SU exist, change the 
			 * SG admin state to LOCK, stay in stable state.
			 */
			for (const auto& su : sg->list_of_su) {
				if (su->list_of_susi != AVD_SU_SI_REL_NULL) {
					avd_susi_mod_send(su->list_of_susi, SA_AMF_HA_QUIESCING);
					su_oper_list_add(su);
				}
			}
		} /* if (sg->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		else {
			return NCSCC_RC_FAILURE;
		}

		if (su_oper_list.empty() == false) {
			set_fsm_state(AVD_SG_FSM_SG_ADMIN);
		}

		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_ADMIN:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* If the SG admin state is shutdown, change the admin state of the 
			 * SG to lock and send D2N-INFO_SU_SI_ASSIGN modify quiesced message
			 * to all the SUs in the SU operation list with quiescing assignment.
			 */
			
			for (const auto& i_su : su_oper_list) {
				if ((i_su->list_of_susi->state == SA_AMF_HA_QUIESCING) &&
				    (i_su->list_of_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {
					avd_susi_mod_send(i_su->list_of_susi, SA_AMF_HA_QUIESCED);
				}
			}
		}		/* if (sg->admin_state == NCS_ADMIN_STATE_LOCK) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)sg->sg_fsm_state));
		LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, sg->name.c_str(), sg->name.length());
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch (sg->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}
/*
 * @brief      Handles modification of assignments in SU of NoRed SG
 *             because of lock or shutdown operation on Node group.
 *             If SU does not have any SIs assigned to it, AMF will try
 *             to instantiate new SUs in the SG. If SU has assignments,
 *             then depending upon lock or shutdown operation, quiesced
 *             or quiescing state will be sent for the SU.
 *
 * @param[in]  ptr to SU
 * @param[in]  ptr to nodegroup AVD_AMF_NG.
 */
void SG_NORED::ng_admin(AVD_SU *su, AVD_AMF_NG *ng) 
{
	TRACE_ENTER2("'%s', sg_fsm_state:%u",su->name.c_str(),
			su->sg_of_su->sg_fsm_state);
	if (su->list_of_susi == nullptr) {
		avd_sg_app_su_inst_func(avd_cb, su->sg_of_su);
		return;
	}
	SaAmfHAStateT ha_state;
	if (ng->saAmfNGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN)
		ha_state = SA_AMF_HA_QUIESCING;
	else 
		ha_state = SA_AMF_HA_QUIESCED;
	//change the state for all assignments to quiescing/quiesced.
	if (avd_susi_mod_send(su->list_of_susi, ha_state) == NCSCC_RC_FAILURE) {
		LOG_ER("quiescing state transtion failed for '%s'",su->name.c_str());
		return;
	}
	avd_sg_su_oper_list_add(avd_cb, su, false);
	su->sg_of_su->set_fsm_state(AVD_SG_FSM_SG_REALIGN);
	//Increment node counter for tracking status of ng operation.
	if (su->list_of_susi->fsm == AVD_SU_SI_STATE_MODIFY) {
		su->su_on_node->su_cnt_admin_oper++;
		TRACE("node:%s, su_cnt_admin_oper:%u", su->su_on_node->name.c_str(),
				su->su_on_node->su_cnt_admin_oper);
	}
	TRACE_LEAVE();
	return;
}

SG_NORED::~SG_NORED() {
}

