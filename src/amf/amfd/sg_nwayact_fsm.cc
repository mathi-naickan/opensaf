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
  the SG state machine, for processing the events related to SG for N-way
  Active redundancy model.

******************************************************************************
*/

#include "base/logtrace.h"

#include "amf/amfd/amfd.h"
#include "amf/amfd/imm.h"
#include "amf/amfd/si_dep.h"
#include <algorithm>

static AVD_SU *avd_get_qualified_su(AVD_SG *avd_sg, AVD_SI *avd_si, 
		bool *next_si_tobe_assigned);
/*****************************************************************************
 * Function: avd_sg_nacvred_su_chose_asgn
 *
 * Purpose:  This function will identify SIs whose assignments is not complete, 
 * search for in-service SUs that can take assignment for this SI and assign 
 * this unassigned SIs to them by Sending D2N-INFO_SU_SI_ASSIGN message for the 
 * SUs with role active for the SIs. It then adds the Assigning SUs to 
 * the SU operation list. If no assignments happen, it returns nullptr.
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
	AVD_SU *i_su, *qualified_su;
	bool l_flag, next_si_tobe_assigned = true;
	AVD_SU_SI_REL *tmp_rel;

	TRACE_ENTER2("'%s'", sg->name.c_str());

	// @todo remove this l_flag??
	l_flag = true;

	avd_sidep_update_si_dep_state_for_all_sis(sg);
	for (const auto& i_si : sg->list_of_si) {
		if (l_flag == false) {
			break;
		}

		/* verify that the SI is ready and needs come more assignments. */
		if ((i_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) ||
			(i_si->list_of_csi == nullptr) ||
		    (i_si->pref_active_assignments() <= i_si->curr_active_assignments() )) {
			continue;
		}

		/* Cannot be assigned, as sponsors SIs are not in enabled state for this SI */
		if ((i_si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED) ||
				(i_si->si_dep_state == AVD_SI_READY_TO_UNASSIGN) ||
				(i_si->si_dep_state == AVD_SI_UNASSIGNING_DUE_TO_DEP)) {
			continue;
		}

		/* identify a in-service SU which is not assigned to this SI and can
		 * take more assignments so that the SI can be assigned. 
		 */
		for (std::map<std::pair<std::string, uint32_t>, AVD_SUS_PER_SI_RANK*>::const_iterator
				it = sirankedsu_db->begin(); it != sirankedsu_db->end(); it++) {
			AVD_SUS_PER_SI_RANK *su_rank_rec = it->second;
			{
				if (su_rank_rec->indx.si_name.compare(i_si->name) != 0)
					continue;

				/* get the su & si */
				i_su = su_db->find(su_rank_rec->su_name);
				AVD_SI *si = avd_si_get(su_rank_rec->indx.si_name);

				/* validate this entry */
				if ((si == nullptr) || (i_su == nullptr) || (si->sg_of_si != i_su->sg_of_su))
					continue;
			}

			if ((i_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
			    ((i_su->sg_of_su->saAmfSGMaxActiveSIsperSU != 0)
			     && (i_su->sg_of_su->saAmfSGMaxActiveSIsperSU <= i_su->saAmfSUNumCurrActiveSIs))) {
				// set i_su to the next su in the list
				std::vector<AVD_SU*>::iterator iter = std::find(i_su->sg_of_su->list_of_su.begin(),
					i_su->sg_of_su->list_of_su.end(), i_su);
				if (iter != i_su->sg_of_su->list_of_su.end()) {
					++iter;
					if (iter != i_su->sg_of_su->list_of_su.end()) {
						i_su = *iter;
					} else {
						i_su = nullptr;
					}
				}
				continue;
			}

			if (avd_su_susi_find(cb, i_su, i_si->name)
			    != AVD_SU_SI_REL_NULL) {
				/* This SU has already a assignment for this SI go to the
				 * next SU.
				 */
				continue;
			}

			/* found the SU assign the SI to the SU as active */
			if (avd_new_assgn_susi(cb, i_su, i_si, SA_AMF_HA_ACTIVE, false, &tmp_rel) == NCSCC_RC_SUCCESS) {
				/* Add the SU to the operation list */
				avd_sg_su_oper_list_add(cb, i_su, false);

				/* Check if the SI can take more assignments. If not exit the SU loop.                         
				 */
				if (i_si->pref_active_assignments() <= i_si->curr_active_assignments() ) {
					break;
				}
			} else {
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, i_si->name.c_str(), i_si->name.length());
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, i_su->name.c_str(), i_su->name.length());
			}

		}

		/* identify a in-service SU which is not assigned to this SI and can
		 * take more assignments so that the SI can be assigned. 
		 */
		l_flag = false;
		for (const auto& iter : sg->list_of_su) {
			if (sg->equal_ranked_su == true) {
				break;
			}

			i_su = iter;
			if ((i_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
			    ((i_su->sg_of_su->saAmfSGMaxActiveSIsperSU != 0)
			     && (i_su->sg_of_su->saAmfSGMaxActiveSIsperSU <= i_su->saAmfSUNumCurrActiveSIs))) {
				continue;
			}

			l_flag = true;

			if (i_si->pref_active_assignments() <= i_si->curr_active_assignments() ) {
				/* The preferred number of active assignments for SI has reached, so continue
				   to next SI */
				i_su = nullptr;
				continue;
			}

			if (avd_su_susi_find(cb, i_su, i_si->name)
			    != AVD_SU_SI_REL_NULL) {
				/* This SU has already a assignment for this SI go to the 
				 * next SU.
				 */
				continue;
			}

			/* found the SU assign the SI to the SU as active */
			if (avd_new_assgn_susi(cb, i_su, i_si, SA_AMF_HA_ACTIVE, false, &tmp_rel) == NCSCC_RC_SUCCESS) {
				/* Add the SU to the operation list */
				avd_sg_su_oper_list_add(cb, i_su, false);

				/* Check if the SI can take more assignments. If not exit the SU loop.
				 */
				if (i_si->pref_active_assignments() <= i_si->curr_active_assignments() ) {
					i_su = nullptr;
					continue;
				}
			} else {
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, i_si->name.c_str(), i_si->name.length());
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, i_su->name.c_str(), i_su->name.length());
			}

			/* choose the next SU */
		}		/* while (i_su != AVD_SU_NULL) */

		if (true == sg->equal_ranked_su) {
			l_flag = false;
			while (true) {
				qualified_su = avd_get_qualified_su(sg, i_si, &next_si_tobe_assigned);
				if ((nullptr == qualified_su) && (false == next_si_tobe_assigned)) {
					l_flag = false; /* All SU are unqualified, no need to next si*/
					break;
				}
				if ((nullptr == qualified_su) && (true == next_si_tobe_assigned)) {
					l_flag = true; /* Go for next assignment */
					break;
				}
				if ((nullptr != qualified_su) && (true == next_si_tobe_assigned)){
					l_flag = true; 
					/* found the SU assign the SI to the SU as active */
					if (avd_new_assgn_susi(cb, qualified_su, i_si, SA_AMF_HA_ACTIVE, false, &tmp_rel) 
							== NCSCC_RC_SUCCESS) {
						/* Add the SU to the operation list */
						avd_sg_su_oper_list_add(cb, qualified_su, false);
					} else {
						LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, i_si->name.c_str(), 
								i_si->name.length());
						LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, qualified_su->name.c_str(), 
								qualified_su->name.length());
						l_flag = true; /* Go for next assignment */
						break;
					}
				}
			}/* while */
		}/* if (true == sg->equal_ranked_su) */

		/* choose the next SI */
	}

	TRACE_LEAVE();

	if (sg->su_oper_list.empty() == true) {
		return nullptr;
	} else {
		return sg->su_oper_list.front();
	}
}

uint32_t SG_NACV::si_assign(AVD_CL_CB *cb, AVD_SI *si) {
	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);

	/* If the SG FSM state is not stable just return success. */
	if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
		return NCSCC_RC_SUCCESS;
	}

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == false)) {
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, si->sg_of_si->sg_ncs_spec);
		return NCSCC_RC_SUCCESS;
	}

	if (avd_sg_nacvred_su_chose_asgn(cb, si->sg_of_si) == nullptr) {
		/* all the assignments have already been done in the SG. */
		return NCSCC_RC_SUCCESS;
	}

	/* change the FSM state */
	m_AVD_SET_SG_FSM(cb, (si->sg_of_si), AVD_SG_FSM_SG_REALIGN);

	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NACV::su_fault(AVD_CL_CB *cb, AVD_SU *su) {
	bool flag;
	AVD_AVND *su_node_ptr = nullptr;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL)
		return NCSCC_RC_SUCCESS;

	/* Do the functionality based on the current state. */
	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		/* change the state for all assignments to quiesced. */
		if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
			LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
			return NCSCC_RC_FAILURE;
		}

		/* add the SU to the operation list and change the SG FSM to SU operation. */
		avd_sg_su_oper_list_add(cb, su, false);
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

			if (avd_su_susi_find(cb, su, su->sg_of_su->admin_si->name)
			    != AVD_SU_SI_REL_NULL) {
				m_AVD_SU_SI_CHK_QSD_ASGN(su, flag);
				if (flag == false) {
					if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
						LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
						return NCSCC_RC_FAILURE;
					}

				}

				if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
				    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
					su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				}
			} else {	/* if ((susi = 
					   avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
					   != AVD_SU_SI_REL_NULL) */
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
					return NCSCC_RC_FAILURE;
				}
			}

			avd_sg_su_oper_list_add(cb, su, false);
		} else {	/* if (su->sg_of_su->admin_si != AVD_SI_NULL) */

			/* Send D2N-INFO_SU_SI_ASSIGN with quiesced all to the SU, if it has 
			 * atleast one SI in quiescing or active state. If the SU is in 
			 * operation list, if the SU admin state is shutdown change to LOCK.
			 * Add the SU to the SU operation list.
			 */
			if (nullptr != su->sg_of_su->max_assigned_su) {
				if (su == su->sg_of_su->min_assigned_su) {
					/* Only care for min_assigned_su as it has been given Act assgnmt and SU 
					   reported OOS, max_assigned_su would be in Quisced state, so send Act
					   to max_assigned_su if it is Act.*/
					/* This is the case of max_assigned_su has refused to go to Quisced, send Act one to
					   it and reset the pointers as SI Transfer has failed.*/
					if (su->sg_of_su->max_assigned_su->saAmfSuReadinessState ==
							SA_AMF_READINESS_IN_SERVICE) {
						AVD_SU_SI_REL *i_susi;
						i_susi = avd_su_susi_find(avd_cb, su->sg_of_su->max_assigned_su,
								su->sg_of_su->si_tobe_redistributed->name);
						osafassert(i_susi);

						if (avd_susi_mod_send(i_susi, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
							su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
							su->sg_of_su->si_tobe_redistributed = nullptr;
							m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su,AVSV_CKPT_AVD_SI_TRANS);
							/* Don't return from here as we need to send Quisced for su*/
						}
					}
					su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
					su->sg_of_su->si_tobe_redistributed = nullptr;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
				} else if (su == su->sg_of_su->max_assigned_su) {
					/* If su is max_assigned_su then let the default path
					   be hitted as max_assigned_su would be assigned Quisced, any way when
					   min_assigned_su get Act resp then it will check if max_assigned_su is OOS
					   and then it wouldn't do anything just reset pointers. So, here don't reset 
					   pointers.*/
				}
			}
			m_AVD_SU_SI_CHK_QSD_ASGN(su, flag);
			if (flag == false) {
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
					return NCSCC_RC_FAILURE;
				}
			}
			m_AVD_CHK_OPLIST(su, flag);
			if (flag == true) {
				su_node_ptr = su->get_node_ptr();
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					su->set_admin_state(SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					if (su_node_ptr->is_node_lock() == true) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}

			} else {	/* if (flag == true) */

				avd_sg_su_oper_list_add(cb, su, false);

			}	/* else (flag == true) */

		}		/* else (su->sg_of_su->admin_si != AVD_SI_NULL) */

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:

		if (su_oper_list_front() == su) {
			/* The SU is same as the SU in the list. If the SI relationships to the
			 * SU is quiescing, If this SU admin is shutdown change to LOCK and
			 * send D2N-INFO_SU_SI_ASSIGN modify quiesced all.
			 */
			if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
					return NCSCC_RC_FAILURE;
				}

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
			 * Send D2N-INFO_SU_SI_ASSIGN modify quiesced all to the SU.
			 * Add the SU to the operation list and change state to SG_realign state.
			 */

			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				return NCSCC_RC_FAILURE;
			}

			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* else (su->sg_of_su->su_oper_list.su == su) */

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		if (avd_su_susi_find(cb, su, su->sg_of_su->admin_si->name)
		    != AVD_SU_SI_REL_NULL) {
			/* The SI admin pointer, SI has assignment only to this SU. If this 
			 * SI admin is shutdown change to LOCK and send D2N-INFO_SU_SI_ASSIGN 
			 * with quiesced all to the SU, if the SU has any other assignment
			 * send quiesced all to the SU. Remove the SI from SI admin pointer.
			 * Add the SU to operation list and change state to SU_operation.
			 */
			m_AVD_SU_SI_CHK_QSD_ASGN(su, flag);
			if (flag == false) {
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
					return NCSCC_RC_FAILURE;
				}

			}

			if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
			    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
				su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
			} else {
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		} else {	/* if ((susi = 
				   avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
				   != AVD_SU_SI_REL_NULL) */
			/* Send D2N-INFO_SU_SI_ASSIGN with quiesced all to the SU.
			 * Add the SU to the SU operation list.
			 * Change the state to SG_realign state.
			 */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				return NCSCC_RC_FAILURE;
			}

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}

		avd_sg_su_oper_list_add(cb, su, false);

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
					LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
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

uint32_t SG_NACV::su_insvc(AVD_CL_CB *cb, AVD_SU *su) {
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

	if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);
		if (AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) {
			/* If after avd_sg_app_su_inst_func call, 
			   still fsm is stable, then do screening 
			   for SI Distribution. */
			if (true == su->sg_of_su->equal_ranked_su)
				avd_sg_nwayact_screening_for_si_distr(su->sg_of_su);
		}

		/* all the assignments have already been done in the SG. */
		return NCSCC_RC_SUCCESS;
	}

	/* change the FSM state */
	m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
	return NCSCC_RC_SUCCESS;

}

uint32_t SG_NACV::susi_success(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
				     AVSV_SUSI_ACT act, SaAmfHAStateT state) {
	bool flag;
	AVD_AVND *su_node_ptr = nullptr;
	AVD_SU_SI_REL *tmp_susi;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		/* Do the action specified in the message if delete else no action. */
		if (act == AVSV_SUSI_ACT_DEL) {
			if (susi == AVD_SU_SI_REL_NULL) {
				su->delete_all_susis();
			} else {
				/* free all the CSI assignments  */
				avd_compcsi_delete(cb, susi, false);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, susi);
			}
		}
		/* log informational error. */
		LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if (act == AVSV_SUSI_ACT_DEL) {
			/* Action is remove. */

			if (susi != AVD_SU_SI_REL_NULL) {
				/* Remove the SI relationship to this SU. */
				/* free all the CSI assignments  */
				avd_compcsi_delete(cb, susi, false);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, susi);
			} else {
				/* Remove all the SI relationships to this SU. */
				su->delete_all_susis();
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
						avd_sg_su_oper_list_add(cb, su->sg_of_su->admin_si->list_of_sisu->su, false);
						su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
						m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
					}
				} else {
					su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				}

			}
			/* if (su->sg_of_su->admin_si != AVD_SI_NULL) */
			m_AVD_SU_SI_CHK_ASGND(su, flag);

			if (flag == true) {
				/* All the assignments are assigned. Remove the SU from 
				 * the operation list.
				 */
				avd_sg_su_oper_list_del(cb, su, false);
			}

			if ((su->sg_of_su->admin_si == AVD_SI_NULL) && (su->sg_of_su->su_oper_list.empty() == true)) {
				if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
					/* No New assignments are been done in the SG. change the FSM state */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
					avd_sidep_sg_take_action(su->sg_of_su); 
					avd_sg_app_su_inst_func(cb, su->sg_of_su);
					if (AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) {
						/* If after avd_sg_app_su_inst_func call, 
						   still fsm is stable, then do screening 
						   for SI Distribution. */
						if (true == su->sg_of_su->equal_ranked_su)
							avd_sg_nwayact_screening_for_si_distr(su->sg_of_su);
					}
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

				if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
					return NCSCC_RC_FAILURE;

				if ((su->sg_of_su->admin_si != AVD_SI_NULL) &&
				    (su->sg_of_su->admin_si->list_of_sisu->su == su) &&
				    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
					avd_sg_su_oper_list_add(cb, su, false);
					su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
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
					LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
					return NCSCC_RC_FAILURE;
				}

				su_node_ptr = su->get_node_ptr();
				if (su->sg_of_su->admin_si != AVD_SI_NULL) {
					if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
					    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
						avd_sg_su_oper_list_add(cb, su, false);
						su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
						m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					}
				} else if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					su->set_admin_state(SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					if (su_node_ptr->is_node_lock() == true) {
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

			if (flag == true) {
				avd_sg_su_oper_list_del(cb, su, false);
				if (nullptr != su->sg_of_su->max_assigned_su) {
					/* SI Transfer is undergoing, this is the case when min_su has been assigned
					   Act, so check still max_su is still IN-SERV, then send DEL to it and reset
					   the pointers, the DEL resp would be handled in the default scenario as the
					   pointers has been reset to indicate that the desired SI transfer has been 
					   successful. If max_su is OOS, no problem just reset the pointer.*/
					if (su->sg_of_su->max_assigned_su->saAmfSuReadinessState ==
							SA_AMF_READINESS_IN_SERVICE) {
						AVD_SU_SI_REL *i_susi;
						avd_sg_su_oper_list_del(cb, su, false);
						/* Find the SUSI, which is in Quisced state. */
						i_susi = avd_su_susi_find(avd_cb, su->sg_of_su->max_assigned_su,
								su->sg_of_su->si_tobe_redistributed->name);
						osafassert(i_susi);
						if (avd_susi_del_send(i_susi) == NCSCC_RC_FAILURE) {
							su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
							su->sg_of_su->si_tobe_redistributed = nullptr;
							m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
							return NCSCC_RC_FAILURE;
						}
					}
					su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
					su->sg_of_su->si_tobe_redistributed = nullptr;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
				}

				if ((su->sg_of_su->admin_si == AVD_SI_NULL) && (su->sg_of_su->su_oper_list.empty() == true)) {
					if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
						/* No New assignments are been done in the SG. change the FSM state */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sidep_sg_take_action(su->sg_of_su); 
						avd_sg_app_su_inst_func(cb, su->sg_of_su);
						if (AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) {
							/* If after avd_sg_app_su_inst_func call, 
							   still fsm is stable, then do screening 
							   for SI Distribution. */
							if (true == su->sg_of_su->equal_ranked_su)
								avd_sg_nwayact_screening_for_si_distr(su->sg_of_su);
						}

					}

				}
			}
			/* if (flag == true) */
		}
		/* if (state == SA_AMF_HA_ACTIVE) */
		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:
		if (act == AVSV_SUSI_ACT_DEL) {
			if (susi == AVD_SU_SI_REL_NULL) {
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				su->delete_all_susis();

			} else {
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, susi->si->name.c_str(), susi->si->name.length());

				/* free all the CSI assignments  */
				avd_compcsi_delete(cb, susi, false);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, susi);
			}

			avd_sg_su_oper_list_del(cb, su, false);

			if (su->sg_of_su->su_oper_list.empty() == true) {
				if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) != nullptr) {
					/* New assignments are been done in the SG. */
					/* change the FSM state */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					return NCSCC_RC_SUCCESS;
				}

				/* change the FSM state */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sidep_sg_take_action(su->sg_of_su); 
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
				if (AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) {
					/* If after avd_sg_app_su_inst_func call, 
					   still fsm is stable, then do screening 
					   for SI Distribution. */
					if (true == su->sg_of_su->equal_ranked_su)
						avd_sg_nwayact_screening_for_si_distr(su->sg_of_su);
				}

			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if (state == SA_AMF_HA_QUIESCED) {
			/* The HA state is quiesced all/quiesced for an SI that is the only SI 
			 * assigned and SU is in the operation list. If the admin state of the 
			 * SU is shutdown change it to lock. Send a D2N-INFO_SU_SI_ASSIGN with
			 * remove all to the SU. Change to SG_realign state.
			 */
			if (nullptr != su->sg_of_su->max_assigned_su) {
				/* SI Transfer is in progress. */
				/* Check whether su->sg_of_su->min_assigned_su readiness state is 
				   SA_AMF_READINESS_IN_SERVICE */
				osafassert(su == su->sg_of_su->max_assigned_su);
				if (su->sg_of_su->min_assigned_su->saAmfSuReadinessState == 
						SA_AMF_READINESS_IN_SERVICE) {
					if (avd_new_assgn_susi(cb, su->sg_of_su->min_assigned_su, su->sg_of_su->
								si_tobe_redistributed, SA_AMF_HA_ACTIVE, false, 
								&tmp_susi) == NCSCC_RC_SUCCESS) {
					} else {
						/* log a fatal error */
						LOG_ER("%s:%u: SU '%s' (%zu), SI '%s' (%zu)", __FILE__, __LINE__,
								su->sg_of_su->min_assigned_su->name.c_str(), 
								su->sg_of_su->min_assigned_su->name.length(), 
								su->sg_of_su->si_tobe_redistributed->name.c_str(), 
								su->sg_of_su->si_tobe_redistributed->name.length());
						/* Act assignment has failed, so let us retrieve back to old state and
						   send Act assignment to the quisced su and reset the ponters as SI 
						   transfer has failed.*/
						if (avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
							su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
							su->sg_of_su->si_tobe_redistributed = nullptr;
							m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
							return NCSCC_RC_FAILURE;
						}
						su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
						su->sg_of_su->si_tobe_redistributed = nullptr;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
						m_AVD_SET_SG_FSM(cb, (susi->su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
						return NCSCC_RC_FAILURE;
					}
					/* Add the SU to the list and change the FSM state */
					avd_sg_su_oper_list_add(cb, su->sg_of_su->min_assigned_su, false);
					m_AVD_SET_SG_FSM(cb, (susi->su->sg_of_su), AVD_SG_FSM_SG_REALIGN);

				} else { /* Target SU is Out of Service. Return Assignment to max_assigned_su and reset
					    SI Transfer pointers. */
					if (avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
						su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
						su->sg_of_su->si_tobe_redistributed = nullptr;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
						return NCSCC_RC_FAILURE;
					}
					su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
					su->sg_of_su->si_tobe_redistributed = nullptr;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
					m_AVD_SET_SG_FSM(cb, (susi->su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}

			} /* if (nullptr != su->sg_of_su->max_assigned_su) */ else {

				if ((susi == AVD_SU_SI_REL_NULL) ||
						((su->list_of_susi == susi) && (susi->su_next == AVD_SU_SI_REL_NULL))) {
					if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
						LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
						return NCSCC_RC_FAILURE;
					}

					if (su_oper_list_front() != su) {
						LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
						LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->list_of_susi->si->name.c_str(),
								su->list_of_susi->si->name.length());

						avd_sg_su_oper_list_add(cb, su, false);
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
						return NCSCC_RC_SUCCESS;
					}

					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);

					su_node_ptr = su->get_node_ptr();
					if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
						su->set_admin_state(SA_AMF_ADMIN_LOCKED);
					} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
						if (su_node_ptr->is_node_lock() == true) {
							node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
						}
					}
				} /* if ((susi == AVD_SU_SI_REL_NULL) ||
				   ((su->list_of_susi == susi) && (susi->su_next == AVD_SU_SI_REL_NULL))) */
			} /* else */
		} /* if (state == SA_AMF_HA_QUIESCED) */
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
				avd_compcsi_delete(cb, susi, false);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, susi);
			} else {
				/* Remove all the SI relationships to this SU. */
				su->delete_all_susis();
			}

			if (su->sg_of_su->admin_si->list_of_sisu != AVD_SU_SI_REL_NULL) {
				if ((su->sg_of_su->admin_si->list_of_sisu->fsm
				     == AVD_SU_SI_STATE_UNASGN) &&
				    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
					avd_sg_su_oper_list_add(cb, su->sg_of_su->admin_si->list_of_sisu->su, false);
					su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			} else {
				su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				/*As sg is stable, screen for si dependencies and take action on whole sg*/
				avd_sidep_update_si_dep_state_for_all_sis(su->sg_of_su);
				avd_sidep_sg_take_action(su->sg_of_su); 
				if (AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) {
					/* If after avd_sg_app_su_inst_func call, 
					   still fsm is stable, then do screening 
					   for SI Distribution. */
					if (true == su->sg_of_su->equal_ranked_su)
						avd_sg_nwayact_screening_for_si_distr(su->sg_of_su);
				}

			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if ((state == SA_AMF_HA_QUIESCED) && (susi != AVD_SU_SI_REL_NULL)) {
			/* message with modified quiesced for a SI in the admin pointer.
			 * Send a D2N-INFO_SU_SI_ASSIGN with remove for this SI to this SU. 
			 * If the SI has only one SU assignment, Remove the SI from the 
			 * admin pointer. Add the SU to operation list. If the SI admin state 
			 * is shutdown change it to LOCK. Change state to SG_realign.
			 */

			if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
			    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
				avd_sg_su_oper_list_add(cb, su, false);
				su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
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

			su->delete_all_susis();

			avd_sg_su_oper_list_del(cb, su, false);

			if (su->sg_of_su->su_oper_list.empty() == true) {
				avd_sg_admin_state_set(su->sg_of_su, SA_AMF_ADMIN_LOCKED);

				/* change the FSM state */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				/*As sg is stable, screen for si dependencies and take action on whole sg*/
				avd_sidep_update_si_dep_state_for_all_sis(su->sg_of_su);
				avd_sidep_sg_take_action(su->sg_of_su); 
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
				if (AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) {
					/* If after avd_sg_app_su_inst_func call, 
					   still fsm is stable, then do screening 
					   for SI Distribution. */
					if (true == su->sg_of_su->equal_ranked_su)
						avd_sg_nwayact_screening_for_si_distr(su->sg_of_su);
				}

			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if (state == SA_AMF_HA_QUIESCED) {
			/* If HA state is quiesced all, Send a D2N-INFO_SU_SI_ASSIGN with 
			 * removal all for this SU.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				return NCSCC_RC_FAILURE;
			}

		}
		/* if (state == SA_AMF_HA_QUIESCED) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NACV::susi_failed(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
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

			if (susi != AVD_SU_SI_REL_NULL) {
				/* quiesced/quiescing for a single SI. Send a D2N-INFO_SU_SI_ASSIGN with
				 * removal for this SU for the SI. If SI in the SI admin pointer and
				 * has only this SU assignment, Change SI admin to LOCK. Remove 
				 * the SI from the SI admin pointer. If the SU is not in 
				 * the SU operation list, Add the SU to the operation list.
				 */

				if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
					return NCSCC_RC_FAILURE;

				if ((su->sg_of_su->admin_si != AVD_SI_NULL) &&
				    (su->sg_of_su->admin_si->list_of_sisu->su == su) &&
				    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
					avd_sg_su_oper_list_add(cb, su, false);
					su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
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
					LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
					return NCSCC_RC_FAILURE;
				}

				su_node_ptr = su->get_node_ptr();
				if (su->sg_of_su->admin_si != AVD_SI_NULL) {
					if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
					    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
						avd_sg_su_oper_list_add(cb, su, false);
						su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
						m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					}
				} else if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					su->set_admin_state(SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					if (su_node_ptr->is_node_lock() == true) {
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
			/* Check whether SI Transfer is going on, if yes then handle it differently. Else follow 
			   default path*/
			if ((nullptr != su->sg_of_su->max_assigned_su) && (su->sg_of_su->max_assigned_su->saAmfSuReadinessState ==
						SA_AMF_READINESS_IN_SERVICE)) {
				/* This is the case of max_assigned_su has refused to go to Quisced, send Act one to 
				   it and reset the pointers as SI Transfer has failed.*/
				osafassert(su == su->sg_of_su->max_assigned_su);
				if (avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
					su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
					su->sg_of_su->si_tobe_redistributed = nullptr;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
					return NCSCC_RC_FAILURE;
				}
				su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
				su->sg_of_su->si_tobe_redistributed = nullptr;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
				m_AVD_SET_SG_FSM(cb, (susi->su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			} else {
				if ((susi == AVD_SU_SI_REL_NULL) || (nullptr != su->sg_of_su->max_assigned_su) ||
						((su->list_of_susi == susi) && (susi->su_next == AVD_SU_SI_REL_NULL))) {
					su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
					su->sg_of_su->si_tobe_redistributed = nullptr;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
					if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
						LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
						return NCSCC_RC_FAILURE;
					}

					if (su_oper_list_front() != su) {
						LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
						LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->list_of_susi->si->name.c_str(),
								su->list_of_susi->si->name.length());

						avd_sg_su_oper_list_add(cb, su, false);
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
						return NCSCC_RC_SUCCESS;
					}

					su_node_ptr = su->get_node_ptr();
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);

					if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
						su->set_admin_state(SA_AMF_ADMIN_LOCKED);
					} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
						if (su_node_ptr->is_node_lock() == true) {
							node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
						}
					}
				} /* if ((susi == AVD_SU_SI_REL_NULL) ||
				     ((su->list_of_susi == susi) && (susi->su_next == AVD_SU_SI_REL_NULL))) */
			}
		} /* if ((act == AVSV_SUSI_ACT_MOD) && ((state == SA_AMF_HA_QUIESCED) ||
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

			if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			if ((su->sg_of_su->admin_si->list_of_sisu->su == su) &&
			    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
				avd_sg_su_oper_list_add(cb, su, false);
				su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
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
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				return NCSCC_RC_FAILURE;
			}

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

uint32_t SG_NACV::realign(AVD_CL_CB *cb, AVD_SG *sg) {
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

	if (avd_sg_nacvred_su_chose_asgn(cb, sg) == nullptr) {
		/* all the assignments have already been done in the SG. */
		set_adjust_state(AVSV_SG_STABLE);
		avd_sg_app_su_inst_func(cb, sg);
		goto done;
	}

	set_adjust_state(AVSV_SG_STABLE);
	m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

 done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

void SG_NACV::node_fail(AVD_CL_CB *cb, AVD_SU *su) {
	bool flag;
	AVD_AVND *su_node_ptr = nullptr;

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

		su->delete_all_susis();

		if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) != nullptr) {
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

		if (nullptr != su->sg_of_su->max_assigned_su) {
			if (su == su->sg_of_su->min_assigned_su) {
				/* Only care for min_assigned_su as it has been given Act assgnmt and SU
				   reported OOS, max_assigned_su would be in Quisced state, so send Act
				   to max_assigned_su if it is IN-SERV.*/
				if (su->sg_of_su->max_assigned_su->saAmfSuReadinessState ==
						SA_AMF_READINESS_IN_SERVICE) {
					AVD_SU_SI_REL *i_susi;
					i_susi = avd_su_susi_find(avd_cb, su->sg_of_su->max_assigned_su,
							su->sg_of_su->si_tobe_redistributed->name);
					osafassert(i_susi);

					if (avd_susi_mod_send(i_susi, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
						su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
						su->sg_of_su->si_tobe_redistributed = nullptr;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
						/* Don't return from here as we need to delete su from oper list*/
					}
				}
				su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
				su->sg_of_su->si_tobe_redistributed = nullptr;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
			} else if (su == su->sg_of_su->max_assigned_su) {
				/* If su is max_assigned_su then let the default path
				   be hitted as max_assigned_su would be assigned Quisced, any way when
				   min_assigned_su get Act resp then it will check if max_assigned_su is OOS
				   and then it wouldn't do anything just reset pointers. So, here don't reset
				   pointers.*/
			}
		}
		su->delete_all_susis();

		if (su->sg_of_su->admin_si != AVD_SI_NULL) {
			if (su->sg_of_su->admin_si->list_of_sisu != AVD_SU_SI_REL_NULL) {
				if ((su->sg_of_su->admin_si->list_of_sisu->fsm
				     == AVD_SU_SI_STATE_UNASGN) &&
				    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
					avd_sg_su_oper_list_add(cb, su->sg_of_su->admin_si->list_of_sisu->su, false);
					su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				}
			} else {
				su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			}

		}
		/* if (su->sg_of_su->admin_si != AVD_SI_NULL) */
		m_AVD_CHK_OPLIST(su, flag);

		if (flag == true) {
			avd_sg_su_oper_list_del(cb, su, false);

			su_node_ptr = su->get_node_ptr();
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				su->set_admin_state(SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				if (su_node_ptr->is_node_lock() == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}
		}

		if ((su->sg_of_su->admin_si == AVD_SI_NULL) && (su->sg_of_su->su_oper_list.empty() == true)) {
			if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
				/* No New assignments are been done in the SG. change the FSM state */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sidep_sg_take_action(su->sg_of_su); 
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
				if (AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) {
					/* If after avd_sg_app_su_inst_func call, 
					   still fsm is stable, then do screening 
					   for SI Distribution. */
					if (true == su->sg_of_su->equal_ranked_su)
						avd_sg_nwayact_screening_for_si_distr(su->sg_of_su);
				}

			}
		}
		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:

		/* Remove the SI relationship to this SU. */
		su->delete_all_susis();

		if (su_oper_list_front() == su) {
			/*  SU is same as the SU in the list. If this SU admin is shutdown 
			 * change to LOCK. Remove the SU from operation list. choose and 
			 * assign SIs whose active assignment criteria is not meet to 
			 * in-service SUs, by sending D2N-INFO_SU_SI_ASSIGN message for the 
			 * SUs with role active for the SIs. Add the SUs to operation list and 
			 * change state to SG_realign. If no assignment can be done, change 
			 * the state to stable state. 
			 */
			avd_sg_su_oper_list_del(cb, su, false);
			if (nullptr != su->sg_of_su->max_assigned_su) {
				/* This is the case when max_assigned_su has been given Quisced during SI Transfer and
				   the corresponding node has gone down*/
				/* min_assigned_su is not assgned SI so if (su->sg_of_su->su_oper_list.su == su) will
				   fail and it will not come till here.  */
				osafassert(su == su->sg_of_su->max_assigned_su);
				su->sg_of_su->max_assigned_su = su->sg_of_su->min_assigned_su = nullptr;
				su->sg_of_su->si_tobe_redistributed = nullptr;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
			}
			su_node_ptr = su->get_node_ptr();
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				su->set_admin_state(SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				if (su_node_ptr->is_node_lock() == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}

			if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
				/* No New assignments are been done in the SG. change the FSM state */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sidep_sg_take_action(su->sg_of_su); 
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
				if (AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) {
					/* If after avd_sg_app_su_inst_func call, 
					   still fsm is stable, then do screening 
					   for SI Distribution. */
					if (true == su->sg_of_su->equal_ranked_su)
						avd_sg_nwayact_screening_for_si_distr(su->sg_of_su);
				}

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
		su->delete_all_susis();

		if (su->sg_of_su->admin_si->list_of_sisu != AVD_SU_SI_REL_NULL) {
			if ((su->sg_of_su->admin_si->list_of_sisu->fsm
			     == AVD_SU_SI_STATE_UNASGN) &&
			    (su->sg_of_su->admin_si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)) {
				avd_sg_su_oper_list_add(cb, su->sg_of_su->admin_si->list_of_sisu->su, false);
				su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		} else {
			su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			if (avd_sg_nacvred_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
				/* No New assignments are been done in the SG. change the FSM state */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sidep_sg_take_action(su->sg_of_su); 
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
				if (AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) {
					/* If after avd_sg_app_su_inst_func call, 
					   still fsm is stable, then do screening 
					   for SI Distribution. */
					if (true == su->sg_of_su->equal_ranked_su)
						avd_sg_nwayact_screening_for_si_distr(su->sg_of_su);
				}

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

		su->delete_all_susis();

		avd_sg_su_oper_list_del(cb, su, false);

		if (su->sg_of_su->su_oper_list.empty() == true) {
			avd_sg_admin_state_set(su->sg_of_su, SA_AMF_ADMIN_LOCKED);

			/* change the FSM state */
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
			/*As sg is stable, screen for si dependencies and take action on whole sg*/
			avd_sidep_update_si_dep_state_for_all_sis(su->sg_of_su);
			avd_sidep_sg_take_action(su->sg_of_su); 
			avd_sg_app_su_inst_func(cb, su->sg_of_su);
			if (AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) {
				/* If after avd_sg_app_su_inst_func call, 
				   still fsm is stable, then do screening 
				   for SI Distribution. */
				if (true == su->sg_of_su->equal_ranked_su)
					avd_sg_nwayact_screening_for_si_distr(su->sg_of_su);
			}

		}

		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
		return;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	return;
}

uint32_t SG_NACV::su_admin_down(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd) {
	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == false)) {
		return NCSCC_RC_FAILURE;
	}

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) ||
		    ((avnd != nullptr) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED))) {

			/* change the state for all assignments to quiesced. */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				return NCSCC_RC_FAILURE;
			}

			/* add the SU to the operation list and change the SG FSM to SU operation. */
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
		}		/* if ((su->admin_state == NCS_ADMIN_STATE_LOCK) ||
				   ((avnd != AVD_AVND_NULL) && (avnd->su_admin_state == NCS_ADMIN_STATE_LOCK))) */
		else if ((su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) ||
			 ((avnd != nullptr) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN))) {
			/* change the state for all assignments to quiescing. */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCING) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				return NCSCC_RC_FAILURE;
			}

			/* add the SU to the operation list and change the SG FSM to SU operation. */
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
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
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
				return NCSCC_RC_FAILURE;
			}
		} else if ((avnd != nullptr) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)) {
			/* Use case: node lock with two (or more) SUs of the same SG hosted
			 * by the same node */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s", __FILE__, __LINE__, su->name.c_str());
				return NCSCC_RC_FAILURE;
			}

			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
		}
		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SG_REALIGN:
		if ((avnd != nullptr) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)) {
			/* Use case: node lock with two (or more) SUs of the same SG hosted
			 * by the same node */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s", __FILE__, __LINE__, su->name.c_str());
				return NCSCC_RC_FAILURE;
			}

			avd_sg_su_oper_list_add(cb, su, false);
		}
		break;
	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_ER("%s:%u: %s (%zu)", __FILE__, __LINE__, su->name.c_str(), su->name.length());
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch (su->sg_of_su->sg_fsm_state) */

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NACV::si_admin_down(AVD_CL_CB *cb, AVD_SI *si) {
	AVD_SU_SI_REL *i_susi;

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
			/* SI lock. Send D2N-INFO_SU_SI_ASSIGN modify quiesced for this SI to
			 * each of the SUs to which it is assigned. Change state to 
			 * SI_operation state. Add it to admin SI pointer.
			 */
			for (i_susi = si->list_of_sisu; i_susi != AVD_SU_SI_REL_NULL; i_susi = i_susi->si_next)
				avd_susi_mod_send(i_susi, SA_AMF_HA_QUIESCED);

			/* add the SI to the admin list and change the SG FSM to SI operation. */
			m_AVD_SET_SG_ADMIN_SI(cb, si);
			m_AVD_SET_SG_FSM(cb, (si->sg_of_si), AVD_SG_FSM_SI_OPER);
		} /* if (si->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SI shutdown. Send D2N-INFO_SU_SI_ASSIGN modify quiescing for this SI to
			 * each of the SUs to which it is assigned. Change state to 
			 * SI_operation state. Add it to admin SI pointer.
			 */
			for (i_susi = si->list_of_sisu; i_susi != AVD_SU_SI_REL_NULL; i_susi = i_susi->si_next)
				avd_susi_mod_send(i_susi, SA_AMF_HA_QUIESCING);

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
					avd_susi_mod_send(i_susi, SA_AMF_HA_QUIESCED);
				}

				i_susi = i_susi->si_next;
			}
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

uint32_t SG_NACV::sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg) {
	TRACE_ENTER2("%u", sg->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == false)) {
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

			for (const auto& i_su : sg->list_of_su) {
				if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
					avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_QUIESCED);

					/* add the SU to the operation list */
					avd_sg_su_oper_list_add(cb, i_su, false);
				}
			}

		} /* if (sg->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (sg->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SG shutdown. Identify all the assigned SUs, send D2N-INFO_SU_SI_ASSIGN
			 * modify quiescing all for each of the SU. Add them to 
			 * the SU operation list. Change state to SG_admin. 
			 * If no assigned SU exist, no action, stay in stable state.
			 */
			for (const auto& i_su : sg->list_of_su) {
				if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
					avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_QUIESCING);

					/* add the SU to the operation list */
					avd_sg_su_oper_list_add(cb, i_su, false);
				}
			}

		} /* if (sg->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		else {
			return NCSCC_RC_FAILURE;
		}

		if (sg->su_oper_list.empty() == false) {
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_ADMIN);
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
					avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_QUIESCED);
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

/*****************************************************************************
 * Function: avd_get_qualified_su
 *
 * Purpose:  This function is called only when sg->equal_ranked_su is true.
 *           This finds qualified SU to get assigned with SI.
 * Input: avd_sg - The SG pointer. 
 *        avd_si - The SI pointer.
 * Output: si_tobe_assigned : If none SU qualifies then return FAILURE else 
 *                            Success.
 * Returns: Qualified SU/nullptr.
 **************************************************************************/
static AVD_SU *avd_get_qualified_su(AVD_SG *sg, AVD_SI *i_si, 
		bool *next_si_tobe_assigned)
{
	AVD_SU *i_su, *pref_su = nullptr, *pre_temp_su = nullptr;
	bool l_flag = false, l_flag_1 = false;
	TRACE_ENTER();
	if (i_si->pref_active_assignments() <= i_si->curr_active_assignments() ) {
		/* The preferred number of active assignments for SI has reached, so return
		   and try assigning next SI*/
		*next_si_tobe_assigned = true; 
		return nullptr;
	}

	for (const auto& iter : sg->list_of_su) {
		i_su = iter;

		if ((i_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
				((i_su->sg_of_su->saAmfSGMaxActiveSIsperSU != 0)
				 && (i_su->sg_of_su->saAmfSGMaxActiveSIsperSU <=
					 i_su->saAmfSUNumCurrActiveSIs))) {
			continue;
		}
		l_flag = true;
		if (avd_su_susi_find(avd_cb, i_su, i_si->name) != AVD_SU_SI_REL_NULL) {
			/* This SU has already a assignment for this SI go to the
			 * next SU.
			 */
			continue;
		}

		l_flag_1 = true;
		if (nullptr == pre_temp_su) {
			/* First SU, so mark it as prefered.*/
			pre_temp_su = i_su;
			pref_su = i_su;
		} else {
			/* Check whether the next su has more/less assignments.*/
			if (pre_temp_su->saAmfSUNumCurrActiveSIs > i_su->saAmfSUNumCurrActiveSIs)
				pref_su = i_su;
		}
		pre_temp_su = i_su;
		/* choose the next SU */
	}/* while (i_su != nullptr)*/

	if (false == l_flag){
		/* This means no SU is qualified, so no need to assign SI, so return failure*/
		*next_si_tobe_assigned = false; 
		TRACE_LEAVE();
		return nullptr;
	}

	if (false == l_flag_1){
		/* This means all SU is assigned, so try assigning next SI, return nullptr but
		   in next_si_tobe_assigned SUCCESS*/
		*next_si_tobe_assigned = true; 
		TRACE_LEAVE();
		return nullptr;
	}

	/* Got the Qualified SU return Success.*/
	*next_si_tobe_assigned = true; 
	osafassert(pref_su);
	TRACE_LEAVE2("'%s'", pref_su->name.c_str());
	return pref_su;
}

/**
 * @brief        This routine checks whether the SIs can be redistributed among SUs
 *               to achieve equal  distribution, if so finds  the  max assigned SU,
 *               min assigned SU and the SI that needs to be transferred
 *
 * @param[in]    avd_sg - pointer to service group
 *
 * @return       Returns nothing.
 */
void avd_sg_nwayact_screening_for_si_distr(AVD_SG *avd_sg)
{
	AVD_SU *i_su;
	AVD_SI *i_si = nullptr;
	AVD_SU_SI_REL *su_si, *i_susi = nullptr;

	TRACE_ENTER();
	osafassert(true == avd_sg->equal_ranked_su);
	osafassert(AVD_SG_FSM_STABLE == avd_sg->sg_fsm_state);
	/* Reset Max and Min ptrs. */
	avd_sg->max_assigned_su = avd_sg->min_assigned_su = nullptr;
	avd_sg->si_tobe_redistributed = nullptr;

	for (const auto& iter : avd_sg->list_of_su) {
		i_su = iter;
		if (i_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) {
			continue;
		}
		if (nullptr == avd_sg->max_assigned_su) {
			/* First SU. Assign it to max_assigned_su and min_assigned_su. */
			avd_sg->max_assigned_su = avd_sg->min_assigned_su = i_su;
		} else {
			if (i_su->saAmfSUNumCurrActiveSIs > avd_sg->max_assigned_su->saAmfSUNumCurrActiveSIs)
				avd_sg->max_assigned_su = i_su;

			if (i_su->saAmfSUNumCurrActiveSIs < avd_sg->min_assigned_su->saAmfSUNumCurrActiveSIs)
				avd_sg->min_assigned_su = i_su;
		}
	} /*  while (i_su != nullptr)  */

	if ((nullptr == avd_sg->max_assigned_su) || (nullptr == avd_sg->min_assigned_su)) {
		TRACE("No need for reassignments");
		goto done; /* No need to change assignments. */
	}

	if ((avd_sg->max_assigned_su->saAmfSUNumCurrActiveSIs - avd_sg->min_assigned_su->saAmfSUNumCurrActiveSIs) <= 1) {
		TRACE("max_assigned_su '%s' no of assignments '%u', min_assigned_su '%s' no of assignments '%u'",
				avd_sg->max_assigned_su->name.c_str(), avd_sg->max_assigned_su->saAmfSUNumCurrActiveSIs,
				avd_sg->min_assigned_su->name.c_str(), avd_sg->min_assigned_su->saAmfSUNumCurrActiveSIs);
		avd_sg->max_assigned_su = avd_sg->min_assigned_su = nullptr;
		TRACE("No need for reassignments");
		goto done; /* No need to change assignments. */
	}
	else {
		/* Need to change assignments. Find the SI whose assignment has to be changed, figure out less
		   ranked SI if SU has more than one assignments. Do check that max_assigned_su and min_assigned_su
		   don't share the same SI assignment(Say SU1 has SI1, SI2, SI3, SI4 and SU2 has SI4, so don't select
		   SI4 at least.)*/
		su_si = avd_sg->max_assigned_su->list_of_susi;
		while (nullptr != su_si) {
			/* Find if this is assigned to avd_sg->min_assigned_su. */
			if (avd_su_susi_find(avd_cb, avd_sg->min_assigned_su, su_si->si->name) != AVD_SU_SI_REL_NULL) {
				/* This SU has already a assignment for this SI go to the
				 * next SI.
				 */
				su_si = su_si->su_next;
				continue;
			}
			if (nullptr == i_si) { /* first si. */
				i_si = su_si->si;
				i_susi = su_si;
			}
			else {
				if (i_si->saAmfSIRank < su_si->si->saAmfSIRank) {
					i_si = su_si->si;
					i_susi = su_si;
				}
			}
			su_si = su_si->su_next;
		}/* while (nullptr != su_si) */
		osafassert(i_si);
		osafassert(i_susi);
		avd_sg->si_tobe_redistributed = i_si;
	}
	/* check point the SI transfer parameters with STANDBY*/
	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, avd_sg, AVSV_CKPT_AVD_SI_TRANS);

	TRACE("max_assigned_su '%s' no of assignments '%u', min_assigned_su '%s' no of assignments '%u',"
			"si_tobe_redistributed '%s'", avd_sg->max_assigned_su->name.c_str(), avd_sg->max_assigned_su->
			saAmfSUNumCurrActiveSIs, avd_sg->min_assigned_su->name.c_str(), avd_sg->min_assigned_su->
			saAmfSUNumCurrActiveSIs, avd_sg->si_tobe_redistributed->name.c_str());
	/* Trigger SI redistribution/transfer here. Only one SI getting shifted from one SU to another SU */

	if (avd_susi_mod_send(i_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS) {
		avd_sg->max_assigned_su = nullptr;
		avd_sg->min_assigned_su = nullptr;
		avd_sg->si_tobe_redistributed = nullptr;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, avd_sg, AVSV_CKPT_AVD_SI_TRANS);
		goto done;
	}
	/* add the SU to the operation list and change the SG FSM to SU operation. */
	avd_sg_su_oper_list_add(avd_cb, i_susi->su, false);
	m_AVD_SET_SG_FSM(avd_cb, (i_susi->su->sg_of_su), AVD_SG_FSM_SU_OPER);

done:
	TRACE_LEAVE();

	return;
}
/*
 * @brief      Handles modification of assignments in SU of NWay_Active SG
 *             because of lock or shutdown operation on Node group.
 *             If SU does not have any SIs assigned to it, AMF will try
 *             to instantiate new SUs in the SG. If SU has assignments,
 *             then depending upon lock or shutdown operation, quiesced
 *             or quiescing state will be sent for the SU.
 *
 * @param[in]  ptr to SU
 * @param[in]  ptr to nodegroup AVD_AMF_NG.
 */
void SG_NACV::ng_admin(AVD_SU *su, AVD_AMF_NG *ng) 
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
	if (avd_sg_su_si_mod_snd(avd_cb, su, ha_state) == NCSCC_RC_FAILURE) {
		LOG_ER("quiescing state transtion failed for '%s'",su->name.c_str());
		return ;
	}

	avd_sg_su_oper_list_add(avd_cb, su, false);
	su->sg_of_su->set_fsm_state(AVD_SG_FSM_SG_REALIGN);
	//Increment node counter for tracking status of ng operation.
	if (su->any_susi_fsm_in(AVD_SU_SI_STATE_MODIFY) == true) {
		su->su_on_node->su_cnt_admin_oper++;
		TRACE("node:%s, su_cnt_admin_oper:%u", su->su_on_node->name.c_str(),
				su->su_on_node->su_cnt_admin_oper);
	}
	TRACE_LEAVE();
	return;
}

SG_NACV::~SG_NACV() {
}

