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
  the SG state machine, for processing the events related to SG for N+M
  redundancy model.
  
******************************************************************************
*/

#include <logtrace.h>

#include <amfd.h>
#include <si_dep.h>
#include <algorithm>

extern uint32_t avd_sg_get_curr_act_cnt(AVD_SG *sg);

static void avd_sg_npm_screening_for_si_redistr(AVD_SG *avd_sg);
static void avd_sg_npm_si_transfer_for_redistr(AVD_SG *avd_sg);

/*****************************************************************************
 * Function: avd_sg_npm_su_next_asgn
 *
 * Purpose:  This function will identify the next SU that has active/standby 
 * assignments based on the state field. If the current SU is null the SG field 
 * will be used to identify the highest ranked active/standby SU.
 *
 * Input: cb - the AVD control block.
 *        sg - The pointer to the service group.
 *        su - The pointer to the current service unit.
 *        state - The HA state of the SU.
 *        
 *
 * Returns: pointer to the SU that is active/standby assigning/assigned.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

static AVD_SU *avd_sg_npm_su_next_asgn(AVD_CL_CB *cb, AVD_SG *sg, AVD_SU *su, SaAmfHAStateT state)
{
	std::vector<AVD_SU*>::iterator iter;
	AVD_SU* next_su = nullptr;
	
	TRACE_ENTER();

	if (su == nullptr) {
		if (sg->list_of_su.empty() == false) {
			iter = sg->list_of_su.begin();
		} else {
			return next_su;
		}
	} else {
		iter = std::find(sg->list_of_su.begin(),
                       sg->list_of_su.end(),
                       su);
		if (iter != sg->list_of_su.end()) {
			// get the SU after 'su'
			++iter;
		}
	}
	
	while (iter != sg->list_of_su.end()) {
		const AVD_SU* su = *iter;
		if ((su->list_of_susi != AVD_SU_SI_REL_NULL) && (su->list_of_susi->state == state)) {
			next_su = *iter;
			break;
		}

		++iter;
	}

	return next_su;
}

/*****************************************************************************
 * Function: avd_sg_npm_su_othr
 *
 * Purpose:  This function will identify the other SU through an susi for a 
 *           given SU.
 *
 * Input: cb - the AVD control block.
 *        su - The pointer to the service unit.
 *        
 *
 * Returns: pointer to the SUSI pointing to the other SU .
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

static AVD_SU_SI_REL *avd_sg_npm_su_othr(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *i_susi, *o_susi;
	TRACE_ENTER();

	i_susi = su->list_of_susi;
	o_susi = AVD_SU_SI_REL_NULL;

	while (i_susi != AVD_SU_SI_REL_NULL) {
		if (i_susi->si->list_of_sisu != i_susi) {
			o_susi = i_susi->si->list_of_sisu;
			if (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)
				return o_susi;
		} else if (i_susi->si->list_of_sisu->si_next != AVD_SU_SI_REL_NULL) {
			o_susi = i_susi->si->list_of_sisu->si_next;
			if (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)
				return o_susi;
		}

		i_susi = i_susi->su_next;
	}

	return o_susi;
}

/*****************************************************************************
 * Function: avd_sg_npm_su_chk_snd
 *
 * Purpose:  This function will verify that for a given standby SU, if there are 
 * any SUSI relationships whose SI relationship is not with the quiesced SU specified.
 * If yes a remove message will be sent to each of those SUSIs. It then sends a
 * active all message to the standby SU.
 *
 * Input: cb - the AVD control block.
 *        s_su - The pointer to the standby service unit.
 *        q_su - pointer to the quiesced SU.
 *        
 *
 * Returns: none.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

static uint32_t avd_sg_npm_su_chk_snd(AVD_CL_CB *cb, AVD_SU *s_su, AVD_SU *q_su)
{
	AVD_SU_SI_REL *i_susi;
	uint32_t rc =  NCSCC_RC_FAILURE;

	TRACE_ENTER();

	/* Check if node failover is undergoing or admin opr is going on the node*/
	if ((q_su->su_on_node->node_state == AVD_AVND_STATE_ABSENT) 
		|| (q_su->su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED)
		|| (q_su->su_on_node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)) {
		if (avd_sidep_is_su_failover_possible(q_su) == false) {
			TRACE("role modification cannot be done now as Sponsor SI's are not yet assigned");
			goto done;
		}
	}

	for (i_susi = s_su->list_of_susi; i_susi != AVD_SU_SI_REL_NULL; i_susi = i_susi->su_next) {
		if ((i_susi->si->list_of_sisu != i_susi) && (i_susi->si->list_of_sisu->su == q_su))
			continue;

		if ((i_susi->si->list_of_sisu->si_next != AVD_SU_SI_REL_NULL) &&
		    (i_susi->si->list_of_sisu->si_next->su == q_su))
			continue;

		avd_susi_del_send(i_susi);

	}			/* for (i_susi = su->list_of_susi;i_susi != AVD_SU_SI_REL_NULL; i_susi = i_susi->su_next) */

	/* Now send active for all the remaining SUSIs. */
	rc = avd_sg_su_si_mod_snd(cb, s_su, SA_AMF_HA_ACTIVE);
	if (rc == NCSCC_RC_SUCCESS) {
		if ((q_su->su_on_node->node_state == AVD_AVND_STATE_ABSENT) 
			|| (q_su->su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED)
			|| (q_su->su_on_node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)) {
			/* Update the dependent SI's dep_state */
			for (i_susi = s_su->list_of_susi;i_susi != nullptr;i_susi = i_susi->su_next) {
				if (i_susi->si->num_dependents > 0) {
					/* This is a Sponsor SI update its dependent states */
					avd_sidep_update_depstate_si_failover(i_susi->si, q_su);
				}
			}
		}
	} else {
		LOG_NO(" %s: %u: Active role modification failed for %s ", __FILE__, __LINE__, s_su->name.value);
	}

done:
	TRACE_LEAVE2("return value :%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_npm_get_least_su
 *
 * Purpose:  This function will identify SU which is IN-SERVICE having least
 * SI Assignments for the given ha state, this function will only be called when
 * equal_ranked_su flag is set.
 *
 * Input: 	sg - sg in which least assigned su to be found
 * 		ha_state - the SU required for this HA state assignment 
 *        
 * Return value: A pointer to SU which has least SI assignments or 
 * 		 nullptr if not found any SU
 *          
 *****************************************************************************/
static AVD_SU* avd_sg_npm_get_least_su(AVD_SG *sg, SaAmfHAStateT ha_state)
{
	AVD_SU *pref_su = nullptr;
	uint32_t curr_act_sus = 0;
	uint32_t curr_std_sus = 0;

	TRACE_ENTER2("HA state = %d", ha_state);

	if (SA_AMF_HA_ACTIVE == ha_state) {
		/* find an SU from the active group of SUs to give an
		 * active SI assignment 
		 * if number of SUs that are active have reached the maximum 
		 * as configured in SG, 
		 * return the SU with the least active SI assignments
		 * otherwise, return an in-service SU which has no assignments
		 */

		/* Get the count of current Active SUs in this SG */
		curr_act_sus = avd_sg_get_curr_act_cnt(sg);
		TRACE("curr_act_sus = %u", curr_act_sus);

		if (curr_act_sus < sg->saAmfSGNumPrefActiveSUs) {
			for (const auto& curr_su : sg->list_of_su) {
		                if ((curr_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
						(curr_su->list_of_susi == AVD_SU_SI_REL_NULL)) {
					/* got an in-service SU with no SI assignments */
					pref_su = curr_su;
					goto done;
				}
			}
		}

		/* we are here, means found no empty su that can be assigned 
		 * active SI assignment so now loop through the su list of the sg 
		 * to find an SU with least active SI assignments
		 */
		for (const auto& curr_su : sg->list_of_su) {
			if (curr_act_sus == 0)
				break;

			if ((curr_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
				(curr_su->list_of_susi->state == ha_state) &&
				((curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU == 0) ||
				 (curr_su->saAmfSUNumCurrActiveSIs < curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU)) && 
				((!pref_su || pref_su->saAmfSUNumCurrActiveSIs > curr_su->saAmfSUNumCurrActiveSIs))) {
					pref_su = curr_su;
			}
		}
	}
	else if (SA_AMF_HA_STANDBY == ha_state) {
		/* find SU for standby SI assignment
		 * if number of SUs that are standby have reached the maximum 
		 * as configured in SG, 
		 * return the SU with the least standby SI assignments
		 * otherwise, return an in-service SU which has no assignments
		 */
		for (const auto& curr_su : sg->list_of_su) {
			if (curr_std_sus >= sg->saAmfSGNumPrefStandbySUs) {
				break;
			}

			if ((curr_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
				(curr_su->list_of_susi == AVD_SU_SI_REL_NULL)) {
				/* got an in-service SU with no SI assignments */
				pref_su = curr_su;
				goto done;
			}
			else if ((curr_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
					(curr_su->list_of_susi->state == ha_state)) {
				/* increment the current active SUs count as you found 
				 * a SU with atleast one active SI assignment
				 */
				curr_std_sus ++;
			}
		}

		TRACE("curr_std_sus = %u", curr_std_sus);

		/* we are here, means found no empty su 
		 * so now loop through the su list of the sg 
		 * to find an SU with least standby SI assignments
		 */

		for (const auto& curr_su : sg->list_of_su) {
			if (curr_std_sus == 0) {
				break;
			}
	
			if ((curr_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
				(curr_su->list_of_susi->state == ha_state) &&
				((curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU == 0) ||
			          (curr_su->saAmfSUNumCurrStandbySIs < curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU)) && 
				((!pref_su || pref_su->saAmfSUNumCurrStandbySIs > curr_su->saAmfSUNumCurrStandbySIs))) {
					pref_su = curr_su;
			}
		}

	}
	else
		osafassert(0);

done:
	return pref_su;
}

/*****************************************************************************
 * Function: avd_sg_npm_distribute_si_equal
 *
 * Purpose:  This function will distribute the SIs equally among the IN-SERVICE 
 * SUs in the given SG where all the SUs of equal rank, this function will be
 * called only when equal_ranked_su flag is set to true.
 *
 * Input: 	sg - sg in which equal distribution of SIs is to be done
 *        
 * Return value: nullptr
 *          
 *****************************************************************************/
static void avd_sg_npm_distribute_si_equal(AVD_SG *sg)
{
	AVD_SU *curr_su = nullptr;
	AVD_SU_SI_REL *susi;

	TRACE_ENTER();

	/* Run through the SI list of the SG to identify
	 * if any SIs need an active SI assignment then choose a 
	 * qualified SU to take the active assignment for that SI
	 */

	for (const auto& curr_si : sg->list_of_si) {

		/* verify that the SI is ready and needs active assignments. */
		if ((curr_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) ||
				(curr_si->saAmfSIAssignmentState == SA_AMF_ASSIGNMENT_FULLY_ASSIGNED) ||
				(curr_si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED) ||
				(curr_si->si_dep_state == AVD_SI_UNASSIGNING_DUE_TO_DEP) ||
				(curr_si->list_of_csi == nullptr)) {
			continue;
		}

		if (curr_si->list_of_sisu == AVD_SU_SI_REL_NULL) {
			/* this SI needs an active assignment now go through 
			 * SU list if we can find an SU to assign active HA
			 * assignment for this SI
			 */
			curr_su = avd_sg_npm_get_least_su(sg, SA_AMF_HA_ACTIVE);

			if (curr_su == nullptr) {
				/* there are no more SUs that can take an active assignment
				 * so break from the loop as no more SIs can be assigned active
				 */
				TRACE("No more SUs to assign Active HA state: '%s'", curr_si->name.value);
				break;
			}

			TRACE("Assigning active for SI(%s) to SU(%s)", curr_si->name.value, curr_su->name.value);

			if (avd_new_assgn_susi(avd_cb, curr_su, curr_si, SA_AMF_HA_ACTIVE, false, &susi) 
					== NCSCC_RC_SUCCESS) {
				/* Add the SU to the operation list */
				avd_sg_su_oper_list_add(avd_cb, curr_su, false);
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_si->name.value, curr_si->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_su->name.value, curr_su->name.length);
			}
		}
	}

	/* Now run through SI list of the SG to identify if there is
	 * an SI which needs a STANDBY assignment and find an SU to 
	 * make a Standby HA assignment accordingly
	 */

	for (const auto& curr_si : sg->list_of_si) {
		if ((curr_si->list_of_sisu ==  AVD_SU_SI_REL_NULL) || 
			(curr_si->saAmfSIAssignmentState == SA_AMF_ASSIGNMENT_FULLY_ASSIGNED)) { 
			/* No standby assignment needs to be done when there is 
			 * either no active assignment exists or it is fully assigned already
			 */
			continue;
		}
		/* an assignment exists for this SI check for 
		 * now check whether its active and needs a 
		 * standby assignment now to make it fully assigned
		 */

		if (curr_si->list_of_sisu->state == SA_AMF_HA_ACTIVE) {

			/* now get an SU for the standby HA assignment for this SI */
			curr_su = avd_sg_npm_get_least_su(sg, SA_AMF_HA_STANDBY);

			if (curr_su == nullptr) {
				/* there are no more SUs that can take an standby assignment
				 * so break from the loop as no more SIs can be assigned standby
				 */
				TRACE("No more SUs to assign Standby HA state: '%s'", curr_si->name.value);
				break;
			}

			TRACE("Assigning standby for SI(%s) to SU(%s)", curr_si->name.value, curr_su->name.value);

			if (avd_new_assgn_susi(avd_cb, curr_su, curr_si, SA_AMF_HA_STANDBY, false, &susi) 
					== NCSCC_RC_SUCCESS) {
				/* Add the SU to the operation list */
				avd_sg_su_oper_list_add(avd_cb, curr_su, false);
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_si->name.value, curr_si->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_su->name.value, curr_su->name.length);
			}
		}
	}

	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_sg_npm_su_chose_asgn
 *
 * Purpose:  This function will identify SIs whose active assignments is not  
 * complete, search for in-service SUs that can take assignment for this SI and assign 
 * this unassigned SIs to them by Sending D2N-INFO_SU_SI_ASSIGN message for the 
 * SUs with role active for the SIs. It then adds the Assigning SUs to 
 * the SU operation list. If all the SIs are assigned active or no more SUs can
 * take active assignments. For each SI that has active assignment it verifies if
 * standby assignment exists, if no it identifies the standby SU corresponding to
 * this SU and assigns this SI to the SU in standby role. It then adds the Assigning SUs to 
 * the SU operation list. If no assignments happen, it returns nullptr.
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

static AVD_SU *avd_sg_npm_su_chose_asgn(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SU *i_su = nullptr, *l_su = nullptr, *n_su;
	uint32_t cnt = 0;
	AVD_SI *i_si = nullptr;
	bool new_su = false;
	bool su_found, load_su_found;
	bool actv_su_found = false;
	AVD_SU_SI_REL *o_susi, *tmp_susi;
	std::vector<AVD_SI*>::iterator si_iter;

	TRACE_ENTER();

	avd_sidep_update_si_dep_state_for_all_sis(sg);
	if (sg->equal_ranked_su == true ) {
		/* In case of equal ranked SUs in the SG distribute the SIs equally among
		 * the IN-SERVICE SUs, 
		 */
		avd_sg_npm_distribute_si_equal(sg);
		goto done;
	}

	/* Identifying and assigning active assignments. */
	si_iter = sg->list_of_si.begin();
	su_found = true;
	actv_su_found = load_su_found = false;

	while ((si_iter != sg->list_of_si.end()) && (su_found == true)) {
		i_si = *si_iter;

		/* verify that the SI is ready and needs active assignments. */
		if ((i_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) ||
		    (i_si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED) ||
		    (i_si->si_dep_state == AVD_SI_READY_TO_UNASSIGN) ||
			(i_si->list_of_csi == nullptr) ||
		    (i_si->si_dep_state == AVD_SI_UNASSIGNING_DUE_TO_DEP)) {
			++si_iter;
			continue;
		}

		if (i_si->list_of_sisu != AVD_SU_SI_REL_NULL) {
			++si_iter;
			actv_su_found = true;
			continue;
		}

		su_found = false;

		/* check if we already have a SU in hand that can take active assignments. */
		if (i_su != nullptr) {
			if ((i_su->sg_of_su->saAmfSGMaxActiveSIsperSU == 0) ||
			    (i_su->sg_of_su->saAmfSGMaxActiveSIsperSU > i_su->saAmfSUNumCurrActiveSIs)) {
				/* found the SU assign the SI to the SU as active */
				su_found = true;

			} else {	/* if ((i_su->si_max_active > i_su->si_curr_active) &&
					   ((i_su->sg_of_su->su_max_active == 0) ||
					   (i_su->sg_of_su->su_max_active > i_su->si_curr_active))) */

				if (sg->saAmfSGMaxActiveSIsperSU > i_su->saAmfSUNumCurrActiveSIs) {
					load_su_found = true;
				}

				if (new_su == false) {
					/* we have last assigning SU identify the next SU that has
					 * active assignments.
					 */
					while ((i_su != nullptr) &&
					       (cnt < sg->saAmfSGNumPrefActiveSUs)) {
						i_su = avd_sg_npm_su_next_asgn(cb, sg, i_su, SA_AMF_HA_ACTIVE);
						if (i_su != nullptr) {
							cnt++;
							if ((i_su->sg_of_su->saAmfSGMaxActiveSIsperSU == 0) ||
							    (i_su->sg_of_su->saAmfSGMaxActiveSIsperSU >
							     i_su->saAmfSUNumCurrActiveSIs)) {
								/* Found the next assigning SU that can take SI. */
								su_found = true;
								break;
							} else if (sg->saAmfSGMaxActiveSIsperSU > i_su->saAmfSUNumCurrActiveSIs) {
								load_su_found = true;
							}

						}
					}

					if ((i_su == nullptr) && (cnt < sg->saAmfSGNumPrefActiveSUs)) {
						/* All the current active SUs are full. The SG can have
						 * more active assignments. identify the highest ranked
						 * in service unassigned SU in the SG.
						 */
						new_su = true;

						if (su_found == false) {
							for (const auto& iter : sg->list_of_su) {
								i_su = iter;
								if ((i_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)
								    && (i_su->list_of_susi == AVD_SU_SI_REL_NULL)) {
									su_found = true;
									cnt++;
									break;
								}
							}
						}

						if (su_found == false) {
							// if su_found is false, then i_su is invalid, so set nullptr
							i_su = nullptr;
						}
					}
					/* if ((i_su == AVD_SU_NULL) && 
					   (cnt < sg->pref_num_active_su)) */
				} /* if (new_su == false) */
				else if (cnt < sg->saAmfSGNumPrefActiveSUs) {
					/* we have last assigning new SU, New inservice unassigned SUs need 
					 * to be used for the SI. So identify the next SU.
					 */

					for (const auto& iter : sg->list_of_su) {
						i_su = iter;
						if ((i_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
						    (i_su->list_of_susi == AVD_SU_SI_REL_NULL)) {
							su_found = true;
							cnt++;
							break;
						}
					}

					if (su_found == false) {
						// if su_found is false, then i_su is invalid, so set nullptr
						i_su = nullptr;
					}
				}
				/* else if (cnt < sg->pref_num_active_su) */
			}	/* else ((i_su->si_max_active > i_su->si_curr_active) &&
				   ((i_su->sg_of_su->su_max_active == 0) ||
				   (i_su->sg_of_su->su_max_active > i_su->si_curr_active))) */

		} else {	/* if (i_su != AVD_SU_NULL) */

			/* Identify the highest ranked active assigning SU.  */
			i_su = avd_sg_npm_su_next_asgn(cb, sg, nullptr, SA_AMF_HA_ACTIVE);
			while (i_su != nullptr) {
				actv_su_found = true;
				cnt++;

				if ((sg->saAmfSGMaxActiveSIsperSU == 0) ||
				     (i_su->sg_of_su->saAmfSGMaxActiveSIsperSU > i_su->saAmfSUNumCurrActiveSIs)) {
					/* Found the next assigning SU that can take SI. */
					su_found = true;
					break;
				} else if (cnt < sg->saAmfSGNumPrefActiveSUs) {
					if (sg->saAmfSGMaxActiveSIsperSU > i_su->saAmfSUNumCurrActiveSIs)
						load_su_found = true;

					i_su = avd_sg_npm_su_next_asgn(cb, sg, i_su, SA_AMF_HA_ACTIVE);
				} else {
					if (sg->saAmfSGMaxActiveSIsperSU > i_su->saAmfSUNumCurrActiveSIs)
						load_su_found = true;
					i_su = nullptr;
				}
			}

			if ((su_found == false) && (cnt < sg->saAmfSGNumPrefActiveSUs)) {
				/* All the current active SUs are full. The SG can have
				 * more active assignments. identify the highest ranked
				 * in service unassigned SU in the SG.
				 */
				new_su = true;
				for (const auto& iter : sg->list_of_su) {
					i_su = iter;
					if ((i_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
					    (i_su->list_of_susi == AVD_SU_SI_REL_NULL)) {
						su_found = true;
						actv_su_found = true;
						cnt++;
						break;
					}
				}

				if (su_found == false) {
					// if su_found is false, then i_su is invalid, so set nullptr
					i_su = nullptr;
				}
			}
			/* if ((su_found == false) && 
			   (cnt < sg->pref_num_active_su)) */
		}		/* else (i_su != AVD_SU_NULL) */

		if (su_found == true) {
			if (avd_new_assgn_susi(cb, i_su, i_si, SA_AMF_HA_ACTIVE, false, &tmp_susi) == NCSCC_RC_SUCCESS) {
				/* Add the SU to the operation list */
				avd_sg_su_oper_list_add(cb, i_su, false);
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_si->name.value, i_si->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_su->name.value, i_su->name.length);
			}

			/* choose the next SI */
			++si_iter;
		}

	}

	/* The SUs were loaded only upto the preffered level. If still SIs
	 * are left load them upto capability level.
	 */
	if ((load_su_found == true) && (si_iter != sg->list_of_si.end()) && (cnt == sg->saAmfSGNumPrefActiveSUs)) {
		/* Identify the highest ranked active assigning SU.  */
		i_su = avd_sg_npm_su_next_asgn(cb, sg, nullptr, SA_AMF_HA_ACTIVE);
		while ((si_iter != sg->list_of_si.end()) && (i_su != nullptr)) {
			i_si = *si_iter;

			if ((i_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) ||
			    (i_si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED) ||
			    (i_si->si_dep_state == AVD_SI_UNASSIGNING_DUE_TO_DEP) ||
				(i_si->list_of_csi == nullptr) ||
			    (i_si->list_of_sisu != AVD_SU_SI_REL_NULL)) {
				++si_iter;
				continue;
			}

			if (sg->saAmfSGMaxActiveSIsperSU <= i_su->saAmfSUNumCurrActiveSIs) {
				i_su = avd_sg_npm_su_next_asgn(cb, sg, i_su, SA_AMF_HA_ACTIVE);
				continue;
			}
			if (avd_new_assgn_susi(cb, i_su, i_si, SA_AMF_HA_ACTIVE, false, &tmp_susi) == NCSCC_RC_SUCCESS) {
				/* Add the SU to the operation list */
				avd_sg_su_oper_list_add(cb, i_su, false);
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_si->name.value, i_si->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_su->name.value, i_su->name.length);
			}

			/* choose the next SI */
			++si_iter;
		}

	}
	/* if ((load_su_found == true) && (i_si != AVD_SI_NULL)) */
	if ((actv_su_found == false) || (sg->su_oper_list.empty() == false)) {
		/* Either there are no active SUs or there are some SUs being assigned 
		 * active SI assignment.
		 */
		if (sg->su_oper_list.empty() == true) {
			TRACE_LEAVE();
			return nullptr;
		} else {
			TRACE_LEAVE();
			return sg->su_oper_list.front();
		}
	}

	/* Identifying and assigning standby assignments. */
	si_iter = sg->list_of_si.begin();
	new_su = false;
	cnt = 0;
	l_su = nullptr;
	/* su with last inservice un assigned SUs. */
	n_su = nullptr;
	while (si_iter != sg->list_of_si.end()) {
		i_si = *si_iter;

		/* verify that the SI has active assignments and needs standby assignments.
                   Also check if active assignment is valid or not.*/
		if ((i_si->list_of_sisu == AVD_SU_SI_REL_NULL) ||
				(i_si->list_of_sisu->si_next != AVD_SU_SI_REL_NULL) ||
				(i_si->is_active() == false)) {
			++si_iter;
			continue;
		}

		su_found = false;

		/* Identify the standby SU if it already exists */
		o_susi = avd_sg_npm_su_othr(cb, i_si->list_of_sisu->su);

		if ((o_susi != AVD_SU_SI_REL_NULL) &&
		    (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
		    (o_susi->state == SA_AMF_HA_STANDBY) &&
		    (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
			i_su = o_susi->su;
			if ((i_su->sg_of_su->saAmfSGMaxStandbySIsperSU == 0) ||
			    (i_su->sg_of_su->saAmfSGMaxStandbySIsperSU > i_su->saAmfSUNumCurrStandbySIs)) {
				/* found the SU assign the SI to the SU as standby */
				su_found = true;

			}
		}/* if (o_susi != AVD_SU_SI_REL_NULL) */
		if(false == su_found) {
			/* su_found will be false in the following cases
			* Didnot found the Standby SU corresponding to i_si->list_of_sisu->su
			* Standby SU is found but current Standby assignments are equal to saAmfSGMaxStandbySIsperSU
			*/
			if (l_su != nullptr) {
				if ((i_su->sg_of_su->saAmfSGMaxStandbySIsperSU == 0) ||
				    (l_su->sg_of_su->saAmfSGMaxStandbySIsperSU > l_su->saAmfSUNumCurrStandbySIs)) {
					/* found the SU assign the SI to the SU as standby */
					su_found = true;
					i_su = l_su;

				} else {	/* if ((l_su->si_max_standby > l_su->si_curr_standby) &&
						   ((i_su->sg_of_su->su_max_standby == 0) ||
						   (l_su->sg_of_su->su_max_standby > l_su->si_curr_standby))) */
					if ((new_su == true) && (cnt < sg->saAmfSGNumPrefStandbySUs)) {
						std::vector<AVD_SU*>::iterator iter = std::find(sg->list_of_su.begin(),
									sg->list_of_su.end(),
									l_su);
						
						while (iter != sg->list_of_su.end()) {
							i_su = *iter;
							if ((i_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)
							    && (i_su->list_of_susi == AVD_SU_SI_REL_NULL)) {
								su_found = true;
								cnt++;
								break;
							}

							/* choose the next SU */
							++iter;
						}	/* while (i_su != AVD_SU_NULL) */

						l_su = i_su;

					} /* if ((new_su == true) && (cnt < sg->pre_num_standby_su)) */
					else if (cnt < sg->saAmfSGNumPrefStandbySUs) {
						i_su = avd_sg_npm_su_next_asgn(cb, sg, l_su, SA_AMF_HA_STANDBY);
						while (i_su != nullptr) {
							cnt++;
							if ((i_su->sg_of_su->saAmfSGMaxStandbySIsperSU == 0) ||
							     (i_su->sg_of_su->saAmfSGMaxStandbySIsperSU >
							      i_su->saAmfSUNumCurrStandbySIs)) {
								/* Found the next assigning SU that can take SI. */
								su_found = true;
								break;
							} else if (cnt < sg->saAmfSGNumPrefStandbySUs) {
								i_su =
								    avd_sg_npm_su_next_asgn(cb, sg, i_su,
											    SA_AMF_HA_STANDBY);
							} else {
								i_su = nullptr;
							}
						}

						if ((su_found == false) && (cnt < sg->saAmfSGNumPrefStandbySUs)) {
							std::vector<AVD_SU*>::iterator iter;

							/* All the current standby SUs are full. The SG can have
							 * more standby assignments. identify the highest ranked
							 * in service unassigned SU in the SG.
							 */
							new_su = true;
							if (n_su == nullptr) {
								iter = sg->list_of_su.begin();
							} else {
								iter = std::find(sg->list_of_su.begin(),
									sg->list_of_su.end(),
									n_su);
							}

							while (iter != sg->list_of_su.end()) {
								i_su = *iter;
								if ((i_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)
								    && (i_su->list_of_susi == AVD_SU_SI_REL_NULL)) {
									su_found = true;
									cnt++;
									break;
								}

								/* choose the next SU */
								++iter;
							}	
							if (su_found == false) {
								i_su = nullptr;
							}
							n_su = i_su;
						}
						/* if ((su_found == false) && 
						   (cnt < sg->pre_num_standby_su)) */
						l_su = i_su;
					}
					/* else if (cnt < sg->pre_num_standby_su) */
				}	/* else ((l_su->si_max_standby > l_su->si_curr_standby) &&
					   ((i_su->sg_of_su->su_max_standby == 0) ||
					   (l_su->sg_of_su->su_max_standby > l_su->si_curr_standby))) */

			} /* if (l_su != AVD_SU_NULL) */
			else if (cnt == 0) {
				/* Since we havent yet tried assigning any new standby SUs.
				 * Identify the highest ranked standby assigning SU.  
				 */

				i_su = avd_sg_npm_su_next_asgn(cb, sg, nullptr, SA_AMF_HA_STANDBY);
				while (i_su != nullptr) {
					cnt++;
					if ((i_su->sg_of_su->saAmfSGMaxStandbySIsperSU == 0) ||
					    (i_su->sg_of_su->saAmfSGMaxStandbySIsperSU >
					     i_su->saAmfSUNumCurrStandbySIs)) {
						/* Found the next assigning SU that can take SI. */
						su_found = true;
						break;
					} else if (cnt < sg->saAmfSGNumPrefStandbySUs) {
						i_su = avd_sg_npm_su_next_asgn(cb, sg, i_su, SA_AMF_HA_STANDBY);
					} else {
						i_su = nullptr;
					}
				}

				if ((su_found == false) && (cnt < sg->saAmfSGNumPrefStandbySUs)) {
					std::vector<AVD_SU*>::iterator iter;

					/* All the current standby SUs are full. The SG can have
					 * more standby assignments. identify the highest ranked
					 * in service unassigned SU in the SG.
					 */
					new_su = true;
					if (n_su == nullptr) {
						iter = sg->list_of_su.begin();
					} else {
						iter = std::find(sg->list_of_su.begin(),
									sg->list_of_su.end(),
									n_su);
					}
					while (iter != sg->list_of_su.end()) {
						i_su = *iter;
						if ((i_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
						    (i_su->list_of_susi == AVD_SU_SI_REL_NULL)) {
							su_found = true;
							cnt++;
							break;
						}

						/* choose the next SU */
						++iter;
					}	/* while (i_su != AVD_SU_NULL) */

					if (su_found == false) {
						// if su_found is false, then i_su is invalid, so set nullptr
						i_su = nullptr;
					}
					n_su = i_su;

				}
				/* if ((su_found == false) && 
				   (cnt < sg->pref_num_active_su)) */
				l_su = i_su;

			}
			/* else if (cnt == 0) */
			if (su_found == false) {
				/* since no su has enough space to take this SI exit the loop here. */
				break;
			}

		}		/* else (o_susi != AVD_SU_SI_REL_NULL) */
		if (su_found == true) {
			if (avd_new_assgn_susi(cb, i_su, i_si, SA_AMF_HA_STANDBY, false, &tmp_susi) == NCSCC_RC_SUCCESS) {
				/* Add the SU to the operation list */
				avd_sg_su_oper_list_add(cb, i_su, false);
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_si->name.value, i_si->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, i_su->name.value, i_su->name.length);
			}

		}

		/* choose the next SI */
		++si_iter;
	}

done:
	if (sg->su_oper_list.empty() == true) {
		TRACE("%s su_oper_list is empty", sg->name.value);
		TRACE_LEAVE();
		return nullptr;
	} else {
		TRACE("%s su_oper_list.front", sg->name.value);
		TRACE_LEAVE();
		return sg->su_oper_list.front();
	}
}

uint32_t SG_NPM::si_assign(AVD_CL_CB *cb, AVD_SI *si) {
	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);

	/* If the SG FSM state is not stable just return success. */
	if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
		return NCSCC_RC_SUCCESS;
	}

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == false)) {
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, si->sg_of_si->sg_ncs_spec);
		return NCSCC_RC_SUCCESS;
	}

	if (avd_sg_npm_su_chose_asgn(cb, si->sg_of_si) == nullptr) {
		/* all the assignments have already been done in the SG. */
		return NCSCC_RC_SUCCESS;
	}

	/* change the FSM state */
	m_AVD_SET_SG_FSM(cb, (si->sg_of_si), AVD_SG_FSM_SG_REALIGN);

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_npm_siswitch_func
 *
 * Purpose:  This function is called when a operator does a SI switch on
 * a SI that belongs to N+M redundancy model SG. 
 * This will trigger a role change action as described in the SG FSM design.
 *
 * Input: cb - the AVD control block
 *        si - The pointer to the SI that needs to be switched.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This is a N+M redundancy model specific function. The initial 
 * functionality of N+M SI switch is same as 2N switch so it just calls that
 * function.
 *
 * 
 **************************************************************************/

uint32_t avd_sg_npm_siswitch_func(AVD_CL_CB *cb, AVD_SI *si)
{
	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);
	osafassert(0);
//	return avd_sg_2n_siswitch_func(cb, si);
	return 0;
}

 /*****************************************************************************
 * Function: su_fault_su_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_npm_su_fault_func.
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

uint32_t SG_NPM::su_fault_su_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_AVND *su_node_ptr = nullptr;

	if (su_oper_list_front() == su) {
		/* SU is in the SU oper list. If the admin state of the SU is 
		 * shutdown change it to lock. Send a D2N-INFO_SU_SI_ASSIGN message
		 * modify quiesced all message to the SU.
		 */
		if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
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
		}

	} else {		/* if (su->sg_of_su->su_oper_list.su == su) */

		/* SU not in the SU oper list. */

		if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
			/* The SU has active SI assignments. send quiesced all message to 
			 * the SU. Add the SU to the oper list. Change the state to SG realign.
			 */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}
			avd_sg_su_oper_list_add(cb, su, false);

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);

		} else {	/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

			/* The SU has standby SI assignments. send D2N-INFO_SU_SI_ASSIGN
			 * message remove all to the SU. Add this SU to the oper list.
			 * Change the state to SG realign.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* else (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

	}			/* else (su->sg_of_su->su_oper_list.su == su) */

	return NCSCC_RC_SUCCESS;
}

 /*****************************************************************************
 * Function: su_fault_si_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_npm_su_fault_func.
 * It is called if the SG is in AVD_SG_FSM_SI_OPER or 
 * SG is in AVD_SG_FSM_SG_REALIGN state with a SI in the SI admin pointer.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the service unit.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This function is called in AVD_SG_FSM_SG_REALIGN also.
 *
 * 
 **************************************************************************/

uint32_t SG_NPM::su_fault_si_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *l_susi;
	bool l_flag;
	AVD_SU *o_su;

	if ((l_susi = avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name))
	    == AVD_SU_SI_REL_NULL) {
		/* SI doesn't have any assignment with the SU */
		m_AVD_CHK_OPLIST(su, l_flag);
		if (l_flag == true) {
			/* the SU is in the SU oper list. So no action return success. */
			return NCSCC_RC_SUCCESS;
		}

		if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
			/* The SU has active SI assignments. send D2N-INFO_SU_SI_ASSIGN 
			 * message modify quiesced all message to the SU. Add the SU to 
			 * the oper list.
			 */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}
			avd_sg_su_oper_list_add(cb, su, false);
			if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SI_OPER) {
				/*It is in SI oper state change state to sg_realign */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}

		} else {	/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

			/* The SU has standby SI assignments. send D2N-INFO_SU_SI_ASSIGN
			 * message remove all to the SU. Add this SU to the oper list.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}
			avd_sg_su_oper_list_add(cb, su, false);
			if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SI_OPER) {
				/*It is in SI oper state change state to sg_realign */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}

		}		/* else (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
	}			/* if ((l_susi = avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
				   == AVD_SU_SI_REL_NULL) */
	else if (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) {
		/* the admin pointer SI has switch operation as true */

		if (l_susi->state == SA_AMF_HA_QUIESCED) {
			if (l_susi->fsm == AVD_SU_SI_STATE_ASGND) {
				/* This SI relation with the SU is quiesced. 
				 * Change the SI switch state to false. Remove the SI from 
				 * the SI admin pointer. send a D2N -INFO_SU_SI_ASSIGN remove
				 * all to the quiesced SU.  Add the SU to the SU 
				 * operation list.  Identify the other SU assigned to this SI,
				 * add it to the SU oper list.
				 */
				su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}
				avd_sg_su_oper_list_add(cb, su, false);
				if (su->sg_of_su->admin_si->list_of_sisu == l_susi) {
					o_su = su->sg_of_su->admin_si->list_of_sisu->si_next->su;
				} else {
					o_su = su->sg_of_su->admin_si->list_of_sisu->su;
				}
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_oper_list_add(cb, o_su, false);

				if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SI_OPER) {
					/*It is in SI oper state change state to sg_realign */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}

			} /* if (l_susi->fsm == AVD_SU_SI_STATE_ASGND) */
			else if (l_susi->fsm == AVD_SU_SI_STATE_MODIFY) {
				/* This SI relation with the SU is quiesced assigning.
				 * Change the SI switch state to false. Remove the SI from 
				 * the SI admin pointer. Add the SU to the SU operation list.
				 */
				su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				avd_sg_su_oper_list_add(cb, su, false);
				if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SI_OPER) {
					/*It is in SI oper state change state to su_oper */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
				}
			}	/* else if (l_susi->fsm == AVD_SU_SI_STATE_MODIFY) */
		} /* if (l_susi->state == SA_AMF_HA_QUIESCED) */
		else if ((l_susi->state == SA_AMF_HA_STANDBY) && (l_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
			/* This SI relation with the SU is standby assigned.
			 * Send D2N-INFO_SU_SI_ASSIGN with remove all to this SU. 
			 * Add this SU to operation list.
			 */

			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}
			avd_sg_su_oper_list_add(cb, su, false);
			if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SI_OPER) {
				/*It is in SI oper state change state to sg_realign */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		}		/*else if ((l_susi->state == SA_AMF_HA_STANDBY) && 
				   (l_susi->fsm == AVD_SU_SI_STATE_ASGND)) */
		else if (l_susi->state == SA_AMF_HA_ACTIVE) {
			/* This SI relation with the SU is active assigning and the 
			 * other SU is quiesced assigned. Send D2N-INFO_SU_SI_ASSIGN 
			 * with modified quiesced all to this SU.
			 */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SI_OPER) {
				/*It is in SI oper state change state to sg_realign */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}

		}
		/* else if (l_susi->state == SA_AMF_HA_ACTIVE) */
	} /* else if (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) */
	else if (su->sg_of_su->admin_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) {
		/* The admin pointer SI admin operation is LOCK/shutdown */
		if ((l_susi->state == SA_AMF_HA_QUIESCED) || (l_susi->state == SA_AMF_HA_QUIESCING)) {
			/* This SI relation with the SU is quiesced/quiescing assigning.
			 * Send D2N-INFO_SU_SI_ASSIGN with modified quiesced all to this
			 * SU. Add the SU to the SU operation list. Mark the SI admin 
			 * state UNLOCK. Remove the SI from the admin pointer.
			 */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}
			avd_sg_su_oper_list_add(cb, su, false);
			su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_UNLOCKED);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SI_OPER) {
				/*It is in SI oper state change state to su_oper */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
			}

		}		/* if ((l_susi->state == SA_AMF_HA_QUIESCED) || 
				   (l_susi->state == SA_AMF_HA_QUIESCING)) */
		else if ((l_susi->state == SA_AMF_HA_STANDBY) && (l_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
			/* The SI has standby HA state w.r.t the SU. 
			 * Send D2N-INFO_SU_SI_ASSIGN with remove all to the SU. 
			 * Add the SU to the SU oper list.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}
			avd_sg_su_oper_list_add(cb, su, false);
			if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SI_OPER) {
				/*It is in SI oper state change state to sg_realign */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		}
		/* if ((l_susi->state == SA_AMF_HA_STANDBY) && 
		   (l_susi->fsm == AVD_SU_SI_STATE_ASGND)) */
	}

	/* else if (su->sg_of_su->admin_si->admin_state != NCS_ADMIN_STATE_UNLOCK) */
	return NCSCC_RC_SUCCESS;

}

 /*****************************************************************************
 * Function: su_fault_sg_relgn
 *
 * Purpose:  This function is a subfunctionality of avd_sg_npm_su_fault_func.
 * It is called if the SG is in AVD_SG_FSM_SG_REALIGN.
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

uint32_t SG_NPM::su_fault_sg_relgn(AVD_CL_CB *cb, AVD_SU *su)
{
	bool l_flag = false;
	AVD_AVND *su_node_ptr = nullptr;

	if (su->sg_of_su->admin_si != AVD_SI_NULL) {
		return su_fault_si_oper(cb, su);

	} /* if ((su->sg_of_su->admin_si != AVD_SI_NULL) */
	else {
		/* SI is not undergoing any admin semantics. */
		/* Check if SI transfer operation is going on */
		if (nullptr != su->sg_of_su->si_tobe_redistributed){
			if (su == su->sg_of_su->min_assigned_su) {
				/* min_assigned_su which is assigned Active role is OOS, max_assigned_su
				   will be in quiesced state. Send Active role to max_assigned_su */
				if (su->sg_of_su->max_assigned_su->saAmfSuReadinessState ==
                                                SA_AMF_READINESS_IN_SERVICE) {
                                        AVD_SU_SI_REL *susi;

                                        susi = avd_su_susi_find(avd_cb, su->sg_of_su->max_assigned_su,
                                                                &su->sg_of_su->si_tobe_redistributed->name);
                                        osafassert(susi);
					avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);

				}
				su->sg_of_su->max_assigned_su = nullptr;
                        	su->sg_of_su->min_assigned_su = nullptr;
                        	su->sg_of_su->si_tobe_redistributed = nullptr;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
			}
		}

		m_AVD_CHK_OPLIST(su, l_flag);
		if (l_flag == true) {
			/* If the fault happened while role failover is under progress,then for all the dependent SI's
			 * for which si_dep_state is set to AVD_SI_FAILOVER_UNDER_PROGRESS reset the dependency to
			 * AVD_SI_SPONSOR_UNASSIGNED and the default flow will remove the assignments
			 */
			AVD_SU_SI_REL *l_susi;
			for (l_susi = su->list_of_susi;l_susi != nullptr;l_susi = l_susi->su_next) {
				if(l_susi->si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS)
					avd_sidep_si_dep_state_set(l_susi->si, AVD_SI_SPONSOR_UNASSIGNED);
				if (l_susi->si->num_dependents > 0)
					avd_sidep_reset_dependents_depstate_in_sufault(l_susi->si);
			}

			/* SU is in the SU oper list. If the admin state of the SU is 
			 * shutdown change it to lock. Send a D2N-INFO_SU_SI_ASSIGN message
			 * modify quiesced all message to the SU. if the SU has standby 
			 * SI assignments. send D2N -INFO_SU_SI_ASSIGN message remove all to
			 * the SU.  if the SU has active SI assignments 
			 * send D2N-INFO_SU_SI_ASSIGN message modify quiesced all message to
			 * the SU.
			 */
			if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
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
			} /* if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) */
			else if ((su->list_of_susi->state == SA_AMF_HA_ACTIVE) ||
				 ((su->list_of_susi->su_next != AVD_SU_SI_REL_NULL) &&
				  (su->list_of_susi->su_next->state == SA_AMF_HA_ACTIVE))) {
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}
			}	/* else if ((su->list_of_susi->state == SA_AMF_HA_ACTIVE) || 
				   ((su->list_of_susi->su_next != AVD_SU_SI_REL_NULL) && 
				   (su->list_of_susi->su_next->state == SA_AMF_HA_ACTIVE))) */
			else if (((su->list_of_susi->state == SA_AMF_HA_STANDBY) &&
				  (su->list_of_susi->fsm != AVD_SU_SI_STATE_UNASGN)) ||
				 ((su->list_of_susi->su_next != AVD_SU_SI_REL_NULL) &&
				  (su->list_of_susi->su_next->state == SA_AMF_HA_STANDBY))) {
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}
			}
			/* else if (((su->list_of_susi->state == SA_AMF_HA_STANDBY) && 
			   (su->list_of_susi->fsm == AVD_SU_SI_STATE_ASGND)) || 
			   ((su->list_of_susi->su_next != AVD_SU_SI_REL_NULL) && 
			   (su->list_of_susi->su_next->state == SA_AMF_HA_STANDBY))) */
		} else {	/* if (l_flag == true) */

			/* SU not in the SU oper list. */

			if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
				/* The SU has active SI assignments. send D2N-INFO_SU_SI_ASSIGN 
				 * message modify quiesced all message to the SU. Add the SU to 
				 * the oper list.
				 */
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}
				avd_sg_su_oper_list_add(cb, su, false);

			} else {	/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

				/* The SU has standby SI assignments. send D2N-INFO_SU_SI_ASSIGN
				 * message remove all to the SU. Add this SU to the oper list.
				 */
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}
				avd_sg_su_oper_list_add(cb, su, false);

			}	/* else (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

		}		/* else (l_flag == true) */

	}			/* else ((su->sg_of_su->admin_si != AVD_SI_NULL) */

	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NPM::su_fault(AVD_CL_CB *cb, AVD_SU *su) {
	AVD_SU_SI_REL *a_susi;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL)
		return NCSCC_RC_SUCCESS;

	/* Do the functionality based on the current state. */
	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
			/* this is a active SU. */
			/* change the state for all assignments to quiesced. */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			/* add the SU to the operation list and change the SG FSM to SU operation. */
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
		} else {	/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

			/* means standby */
			/* change the state for all assignments to unassign and send delete mesage. */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			/* add the SU to the operation list and change the SG FSM to SG realign. */
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* else (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if (su_fault_sg_relgn(cb, su) == NCSCC_RC_FAILURE)
			return NCSCC_RC_FAILURE;

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:

		if (su_fault_su_oper(cb, su) == NCSCC_RC_FAILURE)
			return NCSCC_RC_FAILURE;

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		if (su_fault_si_oper(cb, su) == NCSCC_RC_FAILURE)
			return NCSCC_RC_FAILURE;

		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:

		if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* the SG is lock no action. */
			return NCSCC_RC_SUCCESS;
		} else if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* the SI relationships to the SU is quiescing. Send   
			 * D2N-INFO_SU_SI_ASSIGN modify quiesced all to the SU. Identify its
			 * standby SU assignment send D2N-INFO_SU_SI_ASSIGN remove all for the
			 * SU and add it to the SU operlist. 
			 */
			if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
				a_susi = avd_sg_npm_su_othr(cb, su);
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}

				if ((a_susi != AVD_SU_SI_REL_NULL) && (a_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
					avd_sg_su_si_del_snd(cb, a_susi->su);
					avd_sg_su_oper_list_add(cb, a_susi->su, false);
				}
			} /* if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) */
			else if (su->list_of_susi->fsm == AVD_SU_SI_STATE_ASGND) {
				/* the SI relationships to the SU is standby Send 
				 * D2N-INFO_SU_SI_ASSIGN remove all for the SU and 
				 * add it to the SU operlist.
				 */
				if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}
				avd_sg_su_oper_list_add(cb, su, false);

			}	/* else (su->list_of_susi->state == SA_AMF_HA_QUIESCING) */
		} else {
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

uint32_t SG_NPM::su_insvc(AVD_CL_CB *cb, AVD_SU *su) {
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

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == false)) {
		return NCSCC_RC_SUCCESS;
	}

	if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);
		if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) && 
				(true == su->sg_of_su->equal_ranked_su) &&
				(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
			/* SG fsm is stable, screen for possibility of redistributing SI
			   to achieve equal distribution */
                        avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                }
		/* all the assignments have already been done in the SG. */
		return NCSCC_RC_SUCCESS;
	}

	/* change the FSM state */
	m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
	return NCSCC_RC_SUCCESS;
}

 /*****************************************************************************
 * Function: susi_sucss_sg_reln
 *
 * Purpose:  This function is a subfunctionality of avd_sg_npm_susi_sucss_func.
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

uint32_t SG_NPM::susi_sucss_sg_reln(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					   AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *i_susi, *o_susi;
	bool flag, fover_progress = false;
	AVD_AVND *su_node_ptr = nullptr;

	TRACE_ENTER();

	if (susi == AVD_SU_SI_REL_NULL) {
		/* assign all */
		if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)) {
			/* quiesced all */

			if (su->sg_of_su->admin_si != AVD_SI_NULL) {
				/* SI in admin pointer */
				if ((su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH)
				    && ((i_susi = avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name))
					!= AVD_SU_SI_REL_NULL)) {
					/* SI swith state is true and the SI has relation to this SU. */
					if (i_susi->si->list_of_sisu != i_susi) {
						o_susi = i_susi->si->list_of_sisu;
					} else {
						o_susi = i_susi->si_next;
					}
					if (o_susi == AVD_SU_SI_REL_NULL) {
						/* No standby SU, send D2N-INFO_SU_SI_ASSIGN message 
						 * modify active all assignment to this quiesced SU, remove 
						 * the SI from the SI admin pointer, change the switch value 
						 * to false. Add the SU to SU operlist. 
						 */
						if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value,
											 su->name.length);
							return NCSCC_RC_FAILURE;
						}
						su->sg_of_su->admin_si->set_si_switch(cb,
							AVSV_SI_TOGGLE_STABLE);
						avd_sg_su_oper_list_add(cb, su, false);
						m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					} /* if(o_susi == AVD_SU_SI_REL_NULL) */
					else if ((o_susi->state == SA_AMF_HA_STANDBY) &&
						 (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {
						/* The standby SU is inservice has standby SI assignment from
						 * another SU, send remove message for each of those SI 
						 * assignment to the standby SU and send D2N-INFO_SU_SI_ASSIGN
						 * message modify active all assignments to that SU.
						 */
						avd_sg_npm_su_chk_snd(cb, o_susi->su, su);
					} else if ((o_susi->state == SA_AMF_HA_QUIESCED) &&
						   (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {
						/* This SU's SIs have quiesced assignment with another SU.
						 * which is inservice send D2N -INFO_SU_SI_ASSIGN message 
						 * modify active all assignments to that SU Add the SU to 
						 * oper list. Send D2N -INFO_SU_SI_ASSIGN message remove all 
						 * to this SU. Add this SU to the SU oper list. Remove the 
						 * SI from admin pointer. Set the SI switch state to false.
						 */
						if (avd_sg_su_si_mod_snd(cb, o_susi->su, SA_AMF_HA_ACTIVE) ==
						    NCSCC_RC_FAILURE) {
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value,
											 su->name.length);
							return NCSCC_RC_FAILURE;
						}
						avd_sg_su_oper_list_add(cb, o_susi->su, false);
						avd_sg_su_si_del_snd(cb, su);
						avd_sg_su_oper_list_add(cb, su, false);
						su->sg_of_su->admin_si->set_si_switch(cb,
							AVSV_SI_TOGGLE_STABLE);
						m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					}
					/* if ((o_susi->state == SA_AMF_HA_QUIESCED) &&
					   (o_susi->su->readiness_state == NCS_IN_SERVICE)) */
				} else {	/* if ((su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH)
						   && ((i_susi = avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
						   != AVD_SU_SI_REL_NULL)) */
					m_AVD_CHK_OPLIST(su, flag);

					if (flag == false) {
						/* Log fatal error  */
						LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
						return NCSCC_RC_FAILURE;
					}

					/* SU in the oper list */
					o_susi = avd_sg_npm_su_othr(cb, su);
					if ((o_susi != AVD_SU_SI_REL_NULL) &&
					    (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
					    (o_susi->state == SA_AMF_HA_STANDBY) &&
					    (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
						/* The standby SU w.r.t this SU is inservice has standby 
						 * SI assignment from another SU, send remove message for 
						 * each of those SI assignment to the standby SU and 
						 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
						 * assignments to that SU. Add the SU to the oper list.                   
						 */
						if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
							avd_sg_su_oper_list_add(cb, o_susi->su, false);
					}

					/* Send D2N -INFO_SU_SI_ASSIGN message remove all to this SU. */
					avd_sg_su_si_del_snd(cb, su);

				}	/* else  ((su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH)
					   && ((i_susi = avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
					   != AVD_SU_SI_REL_NULL)) */

			} else {	/* if (su->sg_of_su->admin_si != AVD_SI_NULL) */

				m_AVD_CHK_OPLIST(su, flag);

				if (flag == false) {
					/* Log fatal error  */
					LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}

				/* SU in the oper list */
				o_susi = avd_sg_npm_su_othr(cb, su);
				if ((o_susi != AVD_SU_SI_REL_NULL) &&
				    (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
				    (o_susi->state == SA_AMF_HA_STANDBY) && (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
					/* The standby SU w.r.t this SU is inservice has standby 
					 * SI assignment from another SU, send remove message for 
					 * each of those SI assignment to the standby SU and 
					 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
					 * assignments to that SU. Add the SU to the oper list.                   
					 */
					if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
						avd_sg_su_oper_list_add(cb, o_susi->su, false);
				}

				/* Send D2N-INFO_SU_SI_ASSIGN message remove all to this SU. If
				 * the SUs admin state is shutdown change it to LOCK.
				 */
				su_node_ptr = su->get_node_ptr();

				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					su->set_admin_state(SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					if (su_node_ptr->is_node_lock() == true) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}
				avd_sg_su_si_del_snd(cb, su);

			}	/* else (su->sg_of_su->admin_si != AVD_SI_NULL) */

		} /* if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)) */
		else if (act == AVSV_SUSI_ACT_DEL) {
			/* remove all */
			if (su->list_of_susi->state == SA_AMF_HA_QUIESCED) {
				o_susi = avd_sg_npm_su_othr(cb, su);

				if ((o_susi != AVD_SU_SI_REL_NULL) &&
				    (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
				    (o_susi->state == SA_AMF_HA_STANDBY) && (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
					/* The standby SU w.r.t this SU is inservice has standby 
					 * SI assignment from another SU, send remove message for 
					 * each of those SI assignment to the standby SU and 
					 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
					 * assignments to that SU. Add the SU to the oper list.                   
					 */
					if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
						avd_sg_su_oper_list_add(cb, o_susi->su, false);
					else {
						/* Primarily because of Si Dep. */
						fover_progress = true;
					}
				}
			}

			/* free all the SUSI relationships to this SU. remove this SU from the
			 * SU oper list.SU in the operation list 
			 */
			su->delete_all_susis();
			avd_sg_su_oper_list_del(cb, su, false);

			if ((su_oper_list.empty() == true) && (admin_si == AVD_SI_NULL) &&
					(fover_progress == false)) {
				/* Both the SI admin pointer and SU oper list are empty. Do the
				 * functionality as in stable state to verify if new assignments 
				 * can be done. If yes stay in the same state. If no 
				 * new assignments change state to stable.
				 */
				if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
					/* all the assignments have already been done in the SG. */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
					avd_sidep_sg_take_action(su->sg_of_su); 
					avd_sg_app_su_inst_func(cb, su->sg_of_su);
					if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) && 
							(true == su->sg_of_su->equal_ranked_su) &&
							(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
						/* SG fsm is stable, screen for possibility of redistributing SI
			   			to achieve equal distribution */
                        			avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                			}
				}
			}

		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_ACTIVE)) {
			for (i_susi = su->list_of_susi;
			     (i_susi != AVD_SU_SI_REL_NULL) && (i_susi->fsm == AVD_SU_SI_STATE_ASGND);
			     i_susi = i_susi->su_next) {
				/* Update the IMM */
				avd_susi_update(i_susi, state);
			}

			if (i_susi == AVD_SU_SI_REL_NULL) {
				/* active all and the entire SU is assigned. */
				if ((su->sg_of_su->admin_si != AVD_SI_NULL) &&
				    (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
				    ((i_susi = avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name))
				     != AVD_SU_SI_REL_NULL)) {
					/* SI in admin pointer with switch state true and SI has 
					 * relationship with this SU. Send D2N-INFO_SU_SI_ASSIGN with 
					 * modify standby all to the Quiesced SU. Remove the SI from 
					 * admin pointer and add the quiesced SU to the SU oper list.
					 */
					if (su->sg_of_su->admin_si->list_of_sisu == i_susi) {
						o_susi = i_susi->si_next;
					} else {
						o_susi = su->sg_of_su->admin_si->list_of_sisu;
					}

					if (avd_sg_su_si_mod_snd(cb, o_susi->su, SA_AMF_HA_STANDBY) == NCSCC_RC_FAILURE) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
						return NCSCC_RC_FAILURE;
					}
					avd_sg_su_oper_list_add(cb, o_susi->su, false);
					su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

				}	/* if ((su->sg_of_su->admin_si != AVD_SI_NULL) && 
					   (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
					   ((i_susi = avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
					   != AVD_SU_SI_REL_NULL)) */
				else {
					/* Remove the SU from the SU oper list. */
					avd_sg_su_oper_list_del(cb, su, false);
					if ((su_oper_list.empty() == true) &&
					    (admin_si == AVD_SI_NULL)) {
						/* Both the SI admin pointer and SU oper list are empty. Do the
						 * functionality as in stable state to verify if new assignments 
						 * can be done. If yes stay in the same state. If no 
						 * new assignments change state to stable.
						 */
						if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
							/* all the assignments have already been done in the SG. */
							m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
							avd_sidep_sg_take_action(su->sg_of_su); 
							avd_sg_app_su_inst_func(cb, su->sg_of_su);
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of 
								redistributing SI to achieve equal distribution */
                        					avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
						}
					}

				}	/* ((su->sg_of_su->admin_si != AVD_SI_NULL) && 
					   (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
					   ((i_susi = avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
					   != AVD_SU_SI_REL_NULL)) */
			}
			/* if (i_susi == AVD_SU_SI_REL_NULL) */
		} /* if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_ACTIVE)) */
		else if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_STANDBY)) {
			/* standby all for a SU. remove the SU from the SU oper list. */
			avd_sg_su_oper_list_del(cb, su, false);

			if ((su_oper_list.empty() == true) && (admin_si == AVD_SI_NULL)) {
				/* Both the SI admin pointer and SU oper list are empty. Do the
				 * functionality as in stable state to verify if new assignments 
				 * can be done. If yes stay in the same state. If no 
				 * new assignments change state to stable.
				 */
				if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
					/* all the assignments have already been done in the SG. */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
					avd_sidep_sg_take_action(su->sg_of_su); 
					avd_sg_app_su_inst_func(cb, su->sg_of_su);
					if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
							(true == su->sg_of_su->equal_ranked_su) &&
							(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
						/* SG fsm is stable, screen for possibility of redistributing SI
			   			   to achieve equal distribution */
                        			avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                			}
				}
			}

		}
		/* else if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_STANDBY)) */
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

			for (i_susi = su->list_of_susi;
			     (i_susi != AVD_SU_SI_REL_NULL) && (i_susi->fsm == AVD_SU_SI_STATE_ASGND);
			     i_susi = i_susi->su_next) ;

			if (i_susi == AVD_SU_SI_REL_NULL) {
				if ((su->list_of_susi != AVD_SU_SI_REL_NULL) &&
				    (su->list_of_susi->state == SA_AMF_HA_ACTIVE)) {
					/* active and the entire SU is assigned. */
					if ((su->sg_of_su->admin_si != AVD_SI_NULL) &&
					    (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
					    ((i_susi = avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name))
					     != AVD_SU_SI_REL_NULL)) {
						/* SI in admin pointer with switch state true and SI has 
						 * relationship with this SU. Send D2N-INFO_SU_SI_ASSIGN with 
						 * modify standby all to the Quiesced SU. Remove the SI from 
						 * admin pointer and add the quiesced SU to the SU oper list.
						 */
						if (su->sg_of_su->admin_si->list_of_sisu == i_susi) {
							o_susi = i_susi->si_next;
						} else {
							o_susi = su->sg_of_su->admin_si->list_of_sisu;
						}

						if (avd_sg_su_si_mod_snd(cb, o_susi->su, SA_AMF_HA_STANDBY) ==
						    NCSCC_RC_FAILURE) {
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value,
											 su->name.length);
							return NCSCC_RC_FAILURE;
						}
						avd_sg_su_oper_list_add(cb, o_susi->su, false);
						su->sg_of_su->admin_si->set_si_switch(cb,
							AVSV_SI_TOGGLE_STABLE);
						m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					}	/* if ((su->sg_of_su->admin_si != AVD_SI_NULL) && 
						   (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
						   ((i_susi = avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
						   != AVD_SU_SI_REL_NULL)) */
					else {
						/* Remove the SU from the SU oper list. */
						avd_sg_su_oper_list_del(cb, su, false);
						if ((su_oper_list.empty() == true) &&
						    (admin_si == AVD_SI_NULL)) {
							/* Both the SI admin pointer and SU oper list are empty. Do the
							 * functionality as in stable state to verify if new assignments 
							 * can be done. If yes stay in the same state. If no 
							 * new assignments change state to stable.
							 */
							if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
								/* all the assignments have already been done in the SG. */
								m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
								avd_sidep_sg_take_action(su->sg_of_su); 
								avd_sg_app_su_inst_func(cb, su->sg_of_su);
								if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
									/* SG fsm is stable, screen for possibility of 
									redistributing SI to achieve equal distribution */
                        						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                						}
							}
						}

					}	/* ((su->sg_of_su->admin_si != AVD_SI_NULL) && 
						   (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
						   ((i_susi = avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
						   != AVD_SU_SI_REL_NULL)) */

				}	/* if ((su->list_of_susi != AVD_SU_SI_REL_NULL) && 
					   (su->list_of_susi->state == SA_AMF_HA_ACTIVE)) */
				else {
					/* Remove the SU from the SU oper list. */
					avd_sg_su_oper_list_del(cb, su, false);
					if ((su_oper_list.empty() == true) &&
					    (admin_si == AVD_SI_NULL)) {
						/* Both the SI admin pointer and SU oper list are empty. Do the
						 * functionality as in stable state to verify if new assignments 
						 * can be done. If yes stay in the same state. If no 
						 * new assignments change state to stable.
						 */
						if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
							/* all the assignments have already been done in the SG. */
							m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
							avd_sidep_sg_take_action(su->sg_of_su); 
							avd_sg_app_su_inst_func(cb, su->sg_of_su);
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of 
								redistributing SI to achieve equal distribution */
                        					avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
						}
					}
				}	/* else ((su->list_of_susi != AVD_SU_SI_REL_NULL) && 
					   (su->list_of_susi->state == SA_AMF_HA_ACTIVE)) */

			}
			/* if (i_susi == AVD_SU_SI_REL_NULL) */
		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if ((state == SA_AMF_HA_ACTIVE) || (state == SA_AMF_HA_STANDBY)) {
			for (i_susi = su->list_of_susi;
			     (i_susi != AVD_SU_SI_REL_NULL) && (i_susi->fsm == AVD_SU_SI_STATE_ASGND);
			     i_susi = i_susi->su_next) ;

			if (i_susi == AVD_SU_SI_REL_NULL) {
				/* active or standby for a SUSI and the entire SU is assigned. 
				 * remove the SU from the SU oper list.
				 */
				avd_sg_su_oper_list_del(cb, su, false);

				if((nullptr != su->sg_of_su->si_tobe_redistributed) && 
					(su == su->sg_of_su->min_assigned_su)) {
					AVD_SU_SI_REL *max_su_susi = nullptr;
					/* SI transfe flow, Active assignment to min_assigned_su is suucess. Check if 
				  	   max_assigned_su is still In-service if so send delete request to it and  
				   	   reset the pointers	*/

					if (su->sg_of_su->max_assigned_su->saAmfSuReadinessState == 
									SA_AMF_READINESS_IN_SERVICE) {
                				/* Find the SUSI, which is in Quisced state */
                				max_su_susi = avd_su_susi_find(avd_cb, su->sg_of_su->max_assigned_su,
                        	        	        	&su->sg_of_su->si_tobe_redistributed->name);
						if( max_su_susi )
							avd_susi_del_send(max_su_susi);
					}
			   		/* Reset the SI transfer pointers */ 
					su->sg_of_su->max_assigned_su = nullptr;
                        		su->sg_of_su->min_assigned_su = nullptr;
                        		su->sg_of_su->si_tobe_redistributed = nullptr;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);

				} /* if( ( nullptr != su->sg_of_su->si_tobe_redistributed ) &&
				     (su == su->sg_of_su->min_assigned_su ) ) */

				if ((su_oper_list.empty() == true) && (admin_si== AVD_SI_NULL)) {
					/* Both the SI admin pointer and SU oper list are empty. Do the
					 * functionality as in stable state to verify if new assignments 
					 * can be done. If yes stay in the same state. If no 
					 * new assignments change state to stable.
					 */
					if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
						/* all the assignments have already been done in the SG. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sidep_sg_take_action(su->sg_of_su); 
						avd_sg_app_su_inst_func(cb, su->sg_of_su);
						if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
								(true == su->sg_of_su->equal_ranked_su) &&
								(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
							/* SG fsm is stable, screen for possibility of redistributing SI
			   			   	to achieve equal distribution */
                        				avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                				}
					}
				}
			}
			/* if (i_susi == AVD_SU_SI_REL_NULL) */
		} /* if ((state == SA_AMF_HA_ACTIVE) || (state == SA_AMF_HA_STANDBY)) */
		else if ((state == SA_AMF_HA_QUIESCED) &&
			 (susi->si == su->sg_of_su->admin_si) &&
			 (SA_AMF_ADMIN_UNLOCKED != su->sg_of_su->admin_si->saAmfSIAdminState)) {
			/* Message with modified quiesced for a SI in the admin pointer with.
			 * admin values lock/shutdown. Send a D2N-INFO_SU_SI_ASSIGN with remove
			 * for this SI to this SU, Add this SU to the SU oper list. 
			 * Change admin state to lock for this SI. Identify its standby SU 
			 * assignment, if the standby SU is not in the SU oper list. 
			 * send D2N-INFO_SU_SI_ASSIGN remove for only this SI and Add the 
			 * standby SU to the SU oper list. Remove the SI from the 
			 * SI admin pointer.
			 */
			if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;
			avd_sg_su_oper_list_add(cb, susi->su, false);
			su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
			if (su->sg_of_su->admin_si->list_of_sisu == susi)
				i_susi = susi->si_next;
			else
				i_susi = su->sg_of_su->admin_si->list_of_sisu;

			if ((i_susi != AVD_SU_SI_REL_NULL) && (i_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
				i_susi->fsm = AVD_SU_SI_STATE_UNASGN;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_susi_del_send(i_susi);
				avd_sg_su_oper_list_add(cb, i_susi->su, false);
			}

			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

		}
		/* if ((state == SA_AMF_HA_QUIESCED) && 
		   (susi->si == su->sg_of_su->admin_si) &&
		   (NCS_ADMIN_STATE_UNLOCK != su->sg_of_su->admin_si->admin_state)) */
	}			/* else (susi == AVD_SU_SI_REL_NULL) */

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}
/**
 * @brief        This routine checks whether the SIs can be redistributed among SUs
 *               to achieve equal  distribution, if so finds  the  max assigned SU,
 *               min assigned SU and the SI that needs to be transferred
 *
 * @param[in]    avd_sg -  Pointer to Service Group  that is screened for possibility
 *               of redistribution
 *
 * @return       Returns nothing.
 */
static void avd_sg_npm_screening_for_si_redistr(AVD_SG *avd_sg)
{
	AVD_SU *i_su;
        AVD_SI *i_si = nullptr;
        AVD_SU_SI_REL *su_si;
	uint32_t   assigned_prefferd_SUs = 0;

	TRACE_ENTER2("SG name:%s", avd_sg->name.value);

	avd_sg->max_assigned_su = nullptr;
	avd_sg->min_assigned_su = nullptr;
	avd_sg->si_tobe_redistributed = nullptr;

	/* Screen Active SUs */
	for (const auto& iter : avd_sg->list_of_su) {
		i_su = iter;
                if ((i_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
					(nullptr == i_su->list_of_susi)) {
                        continue;
                }
		if(i_su->list_of_susi->state != SA_AMF_HA_ACTIVE ) {
                        	continue;
                }
		assigned_prefferd_SUs++;
                if (nullptr == avd_sg->max_assigned_su) { 
                        /* First SU. Assign it to max_assigned_su & min_assigned_su */
                        avd_sg->max_assigned_su  = i_su;
                        avd_sg->min_assigned_su = i_su;
		}
                if (i_su->saAmfSUNumCurrActiveSIs > avd_sg->max_assigned_su->saAmfSUNumCurrActiveSIs)
                                avd_sg->max_assigned_su = i_su;

                if (i_su->saAmfSUNumCurrActiveSIs < avd_sg->min_assigned_su->saAmfSUNumCurrActiveSIs)
                                avd_sg->min_assigned_su = i_su;
        } /*  while (i_su != nullptr)  */

        if (nullptr != avd_sg->max_assigned_su) { 
		if(avd_sg->saAmfSGNumPrefActiveSUs > assigned_prefferd_SUs) {
			for (const auto& iter : avd_sg->list_of_su) {
				i_su = iter;
				if ((i_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) && 
						(nullptr == i_su->list_of_susi)) {
					avd_sg->min_assigned_su = i_su;
					break;
				}
			}
		}
	} else {
		TRACE("No max assigned SU, so no redistribution of SIs for SG:'%s'",
				avd_sg->name.value);
		goto done;
	}	

        if ((avd_sg->max_assigned_su->saAmfSUNumCurrActiveSIs -
		avd_sg->min_assigned_su->saAmfSUNumCurrActiveSIs) <= 1) {
                TRACE("SIs are assigned equally among Active SUs, Screen Standby SUs");
	
        	avd_sg->max_assigned_su = nullptr;
		avd_sg->min_assigned_su = nullptr;
        	avd_sg->si_tobe_redistributed = nullptr;
		assigned_prefferd_SUs = 0;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, avd_sg, AVSV_CKPT_AVD_SI_TRANS);

		for (const auto& iter : avd_sg->list_of_su) {
			i_su = iter;
                	if ((i_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
							(nullptr == i_su->list_of_susi)) {
                        	continue;
                	}
			if(i_su->list_of_susi->state != SA_AMF_HA_STANDBY ) {
                        	continue;
                	}
			assigned_prefferd_SUs++;

                	if (nullptr == avd_sg->max_assigned_su) { 
                        	/* First SU. Assign it to max_assigned_su & min_assigned_su */
                        	avd_sg->max_assigned_su  = i_su;
                        	avd_sg->min_assigned_su = i_su;
			}
                        if (i_su->saAmfSUNumCurrStandbySIs > avd_sg->max_assigned_su->saAmfSUNumCurrStandbySIs)
                        	avd_sg->max_assigned_su = i_su;

                        if (i_su->saAmfSUNumCurrStandbySIs < avd_sg->min_assigned_su->saAmfSUNumCurrStandbySIs)
                                avd_sg->min_assigned_su = i_su;
        	}
        	if (nullptr != avd_sg->max_assigned_su) { 
			if(avd_sg->saAmfSGNumPrefStandbySUs > assigned_prefferd_SUs) {
				for (const auto& iter : avd_sg->list_of_su) {
					i_su = iter;
                			if ((i_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) && 
							(nullptr == i_su->list_of_susi)) {
                                		avd_sg->min_assigned_su = i_su;
                        			break;
                			}
				}
			}
		} else {
			TRACE("No max assigned SU, so no redistribution of SIs for SG:'%s'",
					avd_sg->name.value);
			goto done;
		}	

        	if ((avd_sg->max_assigned_su->saAmfSUNumCurrStandbySIs - 
			avd_sg->min_assigned_su->saAmfSUNumCurrStandbySIs) <= 1) {
                	avd_sg->max_assigned_su = avd_sg->min_assigned_su = nullptr;
                	TRACE("SIs are assigned equally among Standby SUs, so no need of SI transfer ");
			goto done;
		}
		TRACE("max_assigned_su '%s' no of assignments '%u', min_assigned_su '%s' no of assignments '%u'",
                        avd_sg->max_assigned_su->name.value, avd_sg->max_assigned_su->saAmfSUNumCurrStandbySIs,
                        avd_sg->min_assigned_su->name.value, avd_sg->min_assigned_su->saAmfSUNumCurrStandbySIs);

	} else {
                TRACE("max_assigned_su '%s' no of assignments '%u', min_assigned_su '%s' no of assignments '%u'",
                        avd_sg->max_assigned_su->name.value, avd_sg->max_assigned_su->saAmfSUNumCurrActiveSIs,
                        avd_sg->min_assigned_su->name.value, avd_sg->min_assigned_su->saAmfSUNumCurrActiveSIs);
        }

        /* Find the SI whose assignment has to be changed,figure out less ranked SI if SU has more than one assignments
           Do check that max_assigned_su and min_assigned_su, don't share the same SI assignment */
        su_si = avd_sg->max_assigned_su->list_of_susi;
        while (su_si) {
                       if (nullptr == i_si) /* first si */
                                i_si = su_si->si;
                        else {
                                if (i_si->saAmfSIRank > su_si->si->saAmfSIRank)
                                        i_si = su_si->si;
                        }
                        su_si = su_si->su_next;
	}/* while (su_si) */

        avd_sg->si_tobe_redistributed = i_si;

        /* Transfer SI assignment from max_assigned_su to min_assigned_su */
	if(avd_sg->si_tobe_redistributed && avd_sg->max_assigned_su)
		avd_sg_npm_si_transfer_for_redistr(avd_sg);

	done:
        	TRACE_LEAVE();
        	return;
}
/**
 * @brief        This routine transfers the SI assignment from the SU which has maximum
 *               assignments to  the SU which has minimum assignments
 *
 * @param[in]    avd_sg -  Pointer to Service Group
 *
 * @return       Returns nothing.
 */
static void avd_sg_npm_si_transfer_for_redistr(AVD_SG *avd_sg)
{
        AVD_SU_SI_REL *susi = nullptr;

	TRACE_ENTER2("SG name:%s SI name:%s", avd_sg->name.value,avd_sg->si_tobe_redistributed->name.value);

	/* checkpoint the SI transfer parameters with STANDBY*/
	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, avd_sg, AVSV_CKPT_AVD_SI_TRANS);

        /* Find the su_si corresponding to max_assign_su & si_tobe_redistributed */
        susi = avd_su_susi_find(avd_cb, avd_sg->max_assigned_su,&avd_sg->si_tobe_redistributed->name);
        if( susi == AVD_SU_SI_REL_NULL ) {
                /* No assignments for this SU corresponding to the SI to be swapped*/
                LOG_ER("Failed to find the susi %s:%u: SU:%s SI:%s", __FILE__,__LINE__,
			avd_sg->max_assigned_su->name.value,avd_sg->si_tobe_redistributed->name.value);
                avd_sg->max_assigned_su = nullptr;
		avd_sg->min_assigned_su = nullptr;
                avd_sg->si_tobe_redistributed = nullptr;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, avd_sg, AVSV_CKPT_AVD_SI_TRANS);
		goto done;
        }

        /* If susi state is Active then swap SI from max_assign_su to min_assign_su */
        if(susi->state == SA_AMF_HA_ACTIVE) {

                /* change the state for Active SU to quiesced */
		if (avd_susi_mod_send(susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS) {
			/* Reset the SI transfer fields */
        		avd_sg->max_assigned_su = nullptr;
			avd_sg->min_assigned_su = nullptr;
        		avd_sg->si_tobe_redistributed = nullptr;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, avd_sg, AVSV_CKPT_AVD_SI_TRANS);
			goto done;
		}
		/* Checkpoint the susi state changes and send notification */
                m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);
                avd_gen_su_ha_state_changed_ntf(avd_cb, susi);

                /* Add the SU to the operation list and change the SG FSM to SU operation. */
                avd_sg_su_oper_list_add(avd_cb, susi->su, false);
                m_AVD_SET_SG_FSM(avd_cb, avd_sg, AVD_SG_FSM_SU_OPER);
        }

        /* If susi state is Standby then swap SI from max_assign_su to min_assign_su */
        if(susi->state == SA_AMF_HA_STANDBY) {
                /* send a delete message about the SU SI assignment to the AvND */
		/* Checkpoint the susi state changes and send notification */
		if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE) {
			/* Reset the SI transfer fields */
        		avd_sg->max_assigned_su = nullptr;
			avd_sg->min_assigned_su = nullptr;
        		avd_sg->si_tobe_redistributed = nullptr;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, avd_sg, AVSV_CKPT_AVD_SI_TRANS);
			goto done;
                }

                /* Change the SG FSM to AVD_SG_FSM_SG_REALIGN */
                m_AVD_SET_SG_FSM(avd_cb, avd_sg, AVD_SG_FSM_SG_REALIGN);

		/* Reset the SI transfer fields, SG si assignment flow will take care of assigning the removed 
		   SI to the possible minimum assigned SU*/
        	avd_sg->max_assigned_su = nullptr;
		avd_sg->min_assigned_su = nullptr;
        	avd_sg->si_tobe_redistributed = nullptr;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, avd_sg, AVSV_CKPT_AVD_SI_TRANS);
        }

done:
        	TRACE_LEAVE();
        	return;
}
/*
 * @brief        Get the count of cureent Active SUs in this SG 
 *
 * @param[in]    sg -  Pointer to Active Service Group 
 *
 * @return       curr_pref_active_sus - No of cureent Active SUs.
 */
uint32_t avd_sg_get_curr_act_cnt(AVD_SG *sg)
{
	uint32_t curr_pref_active_sus = 0;
	TRACE_ENTER2("SG name:%s ", sg->name.value);

	for (const auto& i_su : sg->list_of_su) {
		if ((i_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) || (nullptr == i_su->list_of_susi)) {
        		continue;
        	}

        	if(SA_AMF_HA_ACTIVE == i_su->list_of_susi->state)
			curr_pref_active_sus++;
	}

        TRACE_LEAVE2("current Active SUs :%u",curr_pref_active_sus);
	return curr_pref_active_sus;
}
/*
 * @brief        This routine finds the Standby SU for each of the active assignments on this SU, checks
 *		 the possibility  of assigning Active role to this SU, if so assigns the  Active role to
 *		 the corresponding susi and remove the other assignments.If it is not possible to assign
 *		 Active role sends susi  del to all the asignments on the Standby SU.For the assignments
 *		 that are removed  the default flow takes care of finding and assigning the qualified SUs 
 *
 * @param[in]    su -  Pointer to Active Service unit
 *
 * @return       Returns nothing.
 */
void avd_sg_npm_stdbysu_role_change(AVD_SU *su)
{
	AVD_SU_SI_REL *std_susi, *act_susi = su->list_of_susi;
	uint32_t curr_pref_active_sus = 0;
	bool  flag = true;

	TRACE_ENTER2("SU name:%s",su->name.value);

	while (act_susi != AVD_SU_SI_REL_NULL) {
		std_susi = AVD_SU_SI_REL_NULL;
                if (act_susi->si->list_of_sisu != act_susi) {
                        if ((act_susi->si->list_of_sisu->fsm != AVD_SU_SI_STATE_UNASGN) &&
				(act_susi->si->list_of_sisu->state == SA_AMF_HA_STANDBY)) {
                        	std_susi = act_susi->si->list_of_sisu;
                	}
		} else if (act_susi->si->list_of_sisu->si_next != AVD_SU_SI_REL_NULL) {
                        if ((act_susi->si->list_of_sisu->si_next->fsm != AVD_SU_SI_STATE_UNASGN) &&
				(act_susi->si->list_of_sisu->si_next->state == SA_AMF_HA_STANDBY)) {
                        	std_susi = act_susi->si->list_of_sisu->si_next;
			}
		}
		if (nullptr != std_susi) {
			if (std_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
				if (!curr_pref_active_sus) {
					/* Get the count of current Active SUs in this SG */
					curr_pref_active_sus = avd_sg_get_curr_act_cnt(std_susi->su->sg_of_su);
				}
				/* Check the possibility of assigning Active role to std_susi->su  */
				if(curr_pref_active_sus >= std_susi->su->sg_of_su->saAmfSGNumPrefActiveSUs ) {
					 /* Send a D2N-INFO_SU_SI_ASSIGN with remove all to the standby SU */ 
					if (avd_sg_su_si_del_snd(avd_cb, std_susi->su) == NCSCC_RC_FAILURE) {
                                		LOG_ER("SU del failed :%s :%u :%s", __FILE__, __LINE__,
											std_susi->su->name.value);
						goto done;
                        		}
					/* Add Standby SU to SU operlist */
					avd_sg_su_oper_list_add(avd_cb, std_susi->su, false);

					/*Change state to SG_realign. */
					m_AVD_SET_SG_FSM(avd_cb, su->sg_of_su, AVD_SG_FSM_SG_REALIGN);

				} else {
					/* Check if node failover is undergoing or admin opr is going on the node*/
					if ((su->su_on_node->node_state == AVD_AVND_STATE_ABSENT)
						|| (su->su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED)
						|| (su->su_on_node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)) {
						/* Check if there is any dependency for performing failover for this act_susi */
						flag = avd_sidep_is_si_failover_possible(act_susi->si, su);
					}
					/* Check si_dep_state as well because si dep underprogress SI should
					   not be deleted. */
					if ((flag == true) && (act_susi->si->si_dep_state != AVD_SI_FAILOVER_UNDER_PROGRESS)) {

						/* Check if standby SU has standby SI assignment from another SU, if so
					 	* send remove message for each of those SI assignments 
					 	*/
						AVD_SU_SI_REL *susi;
						for (susi = std_susi->su->list_of_susi; susi != AVD_SU_SI_REL_NULL;
											susi = susi->su_next) {
							if ((susi->si->list_of_sisu != susi) && 
							(	susi->si->list_of_sisu->su == su)) {
								continue;
							}
							if ((susi->si->list_of_sisu->si_next != AVD_SU_SI_REL_NULL) &&
		    							(susi->si->list_of_sisu->si_next->su == su)) {
								continue;
							}
							avd_susi_del_send(susi);
						}/* for (susi = std_susi->su->list_of_susi; susi != AVD_SU_SI_REL_NULL; 
							susi = susi->su_next) */

						/* Now send active for all the remaining SUSIs */
						avd_sg_su_si_mod_snd(avd_cb, std_susi->su, SA_AMF_HA_ACTIVE);

						if ((su->su_on_node->node_state == AVD_AVND_STATE_ABSENT)
							|| (su->su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED)
							|| (su->su_on_node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)) {

							/* update dependents si_dep_state */
							if (act_susi->si->num_dependents > 0) {
								avd_sidep_update_depstate_si_failover(act_susi->si, su);
							}
						}

						/* std_susi->su role changed to Active so increment the curr_pref_active_sus */
						curr_pref_active_sus++;
					
						/* Add Standby SU to SU operlist */
						avd_sg_su_oper_list_add(avd_cb, std_susi->su, false);

						/*Change state to SG_realign. */
						m_AVD_SET_SG_FSM(avd_cb, su->sg_of_su, AVD_SG_FSM_SG_REALIGN);

					} else {
						TRACE("Role failover is deferred as sponsors role failover is under going");
						avd_sidep_si_dep_state_set(act_susi->si, AVD_SI_FAILOVER_UNDER_PROGRESS);
					}
				}
			}
		}
                act_susi = act_susi->su_next;
	}
done:
	TRACE_LEAVE();
}
 /*****************************************************************************
 * Function: susi_success_su_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_npm_susi_sucss_func.
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

uint32_t SG_NPM::susi_success_su_oper(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					   AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *i_susi;
	AVD_SU_SI_REL *o_susi;
	AVD_SU_SI_REL *tmp_susi;
	bool susi_assgn_failed = false;
	
	AVD_AVND *su_node_ptr = nullptr;

        TRACE_ENTER();

	if (su->list_of_susi == AVD_SU_SI_REL_NULL)
		return NCSCC_RC_SUCCESS;

	
	if ( act == AVSV_SUSI_ACT_MOD ) {
		if ((state == SA_AMF_HA_QUIESCED) && (su_oper_list_front() == su)) {
			if ( (su->sg_of_su->si_tobe_redistributed) && (su->sg_of_su->max_assigned_su == su)) {
				/* SI Transfer flow, got response for max_assigned_su quiesced opr, if min_assigned_su
				   is in in_service send Active assignment to it else revert back Active assignment to
				   max_assigned_su */
				if (su->sg_of_su->min_assigned_su->saAmfSuReadinessState == 
								SA_AMF_READINESS_IN_SERVICE) {
					TRACE("Assigning active for SI:%s to SU:%s",
						su->sg_of_su->si_tobe_redistributed->name.value,
						su->sg_of_su->min_assigned_su->name.value);
					if (avd_new_assgn_susi(avd_cb, su->sg_of_su->min_assigned_su,
							su->sg_of_su->si_tobe_redistributed,
					 		SA_AMF_HA_ACTIVE, false,&tmp_susi) == NCSCC_RC_SUCCESS) { 

						/* Add the SU to the operation list and set SG FSM to AVD_SG_FSM_SG_REALIGN */
						avd_sg_su_oper_list_add(cb, su->sg_of_su->min_assigned_su, false);
                                		m_AVD_SET_SG_FSM(cb, susi->su->sg_of_su, AVD_SG_FSM_SG_REALIGN);

					} else {
                        			LOG_ER("susi assgn failed %s:%u: SU:%s SI:%s", __FILE__,__LINE__,
							su->sg_of_su->min_assigned_su->name.value,
							su->sg_of_su->si_tobe_redistributed->name.value);

						/* Assign back Active role to max_asssigned_su */
						susi_assgn_failed = true;
					}
				} 
				if ((su->sg_of_su->min_assigned_su->saAmfSuReadinessState == 
								SA_AMF_READINESS_OUT_OF_SERVICE) ||
								( susi_assgn_failed  == true )) {
					/* min_assigne_su is Out of Service or susi assgn failed for min_assigned_su
					   So return Active assignment to max_assigned_su */

					avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);

					su->sg_of_su->max_assigned_su = nullptr;
                        		su->sg_of_su->min_assigned_su = nullptr;
                        		su->sg_of_su->si_tobe_redistributed = nullptr;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);

                                	/* max_assigned_su is already in the su_oper_list, move SG to AVD_SG_FSM_SG_REALIGN */
                                	m_AVD_SET_SG_FSM(cb, (susi->su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}

			}
			else{
				if (susi != AVD_SU_SI_REL_NULL) {
					/* Quiesced for a SUSI. change the SUSI state to quiesced assigned.
			 		* If all the SUSIs of the SU are assigned quiesced do the 
			 		* functionality of quiesced all. If not stay in the same state.
			 		*/
					for (i_susi = su->list_of_susi;
			     			(i_susi != AVD_SU_SI_REL_NULL) && (i_susi->fsm ==AVD_SU_SI_STATE_ASGND);
			     			i_susi = i_susi->su_next) ;

					if (i_susi != AVD_SU_SI_REL_NULL)
						return NCSCC_RC_SUCCESS;

				}
				
				/* Find Standby SU corresponding to susi on this SU and do role change */
				avd_sg_npm_stdbysu_role_change(su);

				/* Send D2N-INFO_SU_SI_ASSIGN message remove all to this SU. If
		 		* the SUs admin state is shutdown change it to LOCK.
		 		*/
				su_node_ptr = su->get_node_ptr();

				/* If the admin state of the SU is shutdown change it to lock*/
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					su->set_admin_state(SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					if (su_node_ptr->is_node_lock() == true) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}

				avd_sg_su_si_del_snd(cb, su);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		}
	}			/* if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED) &&
				   (su->sg_of_su->su_oper_list.su == su)) */
	else if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_DEL) && (su_oper_list_front() != su)) {
		/* delete all and SU is not in the operation list. Free the SUSI 
		 * relationships for the SU. 
		 */

		su->delete_all_susis();

	}			/* if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_DEL) && 
				   (su->sg_of_su->su_oper_list.su != su)) */
	else if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_DEL) && (su_oper_list_front() == su)) {
		/*remove all and SU is in the operation list */
		if (su->list_of_susi->state != SA_AMF_HA_QUIESCED) {
			/* log error */
			LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		}

		o_susi = avd_sg_npm_su_othr(cb, su);

		if ((o_susi != AVD_SU_SI_REL_NULL) &&
		    (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
		    (o_susi->state == SA_AMF_HA_STANDBY) && (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
			/* The standby SU w.r.t this SU is inservice has standby 
			 * SI assignment from another SU, send remove message for 
			 * each of those SI assignment to the standby SU and 
			 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
			 * assignments to that SU. Add the SU to the oper list.                   
			 */
			if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
				avd_sg_su_oper_list_add(cb, o_susi->su, false);
		} else if ((o_susi != AVD_SU_SI_REL_NULL) &&
			   (o_susi->su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE)) {
			/* Send a D2N-INFO_SU_SI_ASSIGN with remove all to the standby SU 
			 * add it to SU oper list.
			 */
			avd_sg_su_si_del_snd(cb, o_susi->su);
			avd_sg_su_oper_list_add(cb, o_susi->su, false);
		}

		/* free all the SUSI relationships to this SU. remove this SU from the
		 * SU oper list.SU in the operation list 
		 */
		su->delete_all_susis();
		avd_sg_su_oper_list_del(cb, su, false);

		if (su_oper_list.empty() == true) {
			/* SU oper list is empty. Do the
			 * functionality as in stable state to verify if new assignments 
			 * can be done. If yes stay in the same state. If no 
			 * new assignments change state to stable.
			 */
			if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
				/* all the assignments have already been done in the SG. */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sidep_sg_take_action(su->sg_of_su); 
				avd_sg_app_su_inst_func(cb, su->sg_of_su);
				if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
						(true == su->sg_of_su->equal_ranked_su) &&
						(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
					/* SG fsm is stable, screen for possibility of redistributing SI
			   		to achieve equal distribution */
                        		avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                		}
			} else {
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		} else {
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}

	}

	/* if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_DEL) && 
	   (su->sg_of_su->su_oper_list.su == su)) */
	return NCSCC_RC_SUCCESS;
}

 /*****************************************************************************
 * Function: avd_sg_npm_susi_sucss_si_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_npm_susi_sucss_func.
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

static uint32_t avd_sg_npm_susi_sucss_si_oper(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					   AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *i_susi, *o_susi;

	if (susi != AVD_SU_SI_REL_NULL) {
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

			if ((su->sg_of_su->admin_si != AVD_SI_NULL) &&
			    (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH)) {
				/* SI in admin pointer with switch state true  */

				for (i_susi = su->list_of_susi;
				     (i_susi != AVD_SU_SI_REL_NULL) && (i_susi->fsm == AVD_SU_SI_STATE_ASGND);
				     i_susi = i_susi->su_next) ;

				if (i_susi == AVD_SU_SI_REL_NULL) {
					/* entire SU is assigned. Send D2N-INFO_SU_SI_ASSIGN with 
					 * modify standby all to the Quiesced SU. Remove the SI from 
					 * admin pointer and add the quiesced SU to the SU oper list.
					 */
					if (su->sg_of_su->admin_si->list_of_sisu == i_susi) {
						o_susi = i_susi->si_next;
					} else {
						o_susi = su->sg_of_su->admin_si->list_of_sisu;
					}

					if (avd_sg_su_si_mod_snd(cb, o_susi->su, SA_AMF_HA_STANDBY) == NCSCC_RC_FAILURE) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
						return NCSCC_RC_FAILURE;
					}

					avd_sg_su_oper_list_add(cb, o_susi->su, false);
					su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}

				/* if (i_susi == AVD_SU_SI_REL_NULL) */
				/* Some more SIs need to be removed.  So dont do anything. */
			}
			/* if ((su->sg_of_su->admin_si != AVD_SI_NULL) && 
			   (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH)) */
		} /* if (act == AVSV_SUSI_ACT_DEL) */
		else if ((state == SA_AMF_HA_QUIESCED) &&
			 (susi->si == su->sg_of_su->admin_si) &&
			 (SA_AMF_ADMIN_UNLOCKED != su->sg_of_su->admin_si->saAmfSIAdminState)) {
			/* Message with modified quiesced for a SI in the admin pointer with.
			 * admin values lock/shutdown. Send a D2N-INFO_SU_SI_ASSIGN with remove
			 * for this SI to this SU, Add this SU to the SU oper list. 
			 * Change admin state to lock for this SI. Identify its standby SU assignment. 
			 * send D2N-INFO_SU_SI_ASSIGN remove for only this SI and Add the 
			 * standby SU to the SU oper list. Remove the SI from the 
			 * SI admin pointer.
			 */
			if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			avd_sg_su_oper_list_add(cb, susi->su, false);
			su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_LOCKED);
			if (su->sg_of_su->admin_si->list_of_sisu == susi)
				i_susi = susi->si_next;
			else
				i_susi = su->sg_of_su->admin_si->list_of_sisu;

			if (i_susi != AVD_SU_SI_REL_NULL) {
				avd_susi_del_send(i_susi);
				avd_sg_su_oper_list_add(cb, i_susi->su, false);
			}

			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}
		/* if ((state == SA_AMF_HA_QUIESCED) && 
		   (susi->si == su->sg_of_su->admin_si) &&
		   (NCS_ADMIN_STATE_UNLOCK != su->sg_of_su->admin_si->admin_state)) */
			
	} else {		/* if (susi != AVD_SU_SI_REL_NULL) */

		if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED) &&
		    (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
		    ((i_susi = avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name))
		     != AVD_SU_SI_REL_NULL)) {
			/* message with modified quiesced all and an SI is in the admin
			 * pointer with switch value true
			 */
			if (i_susi->si->list_of_sisu != i_susi) {
				o_susi = i_susi->si->list_of_sisu;
			} else {
				o_susi = i_susi->si_next;
			}
			if (o_susi == AVD_SU_SI_REL_NULL) {
				/* No standby SU, send D2N-INFO_SU_SI_ASSIGN message 
				 * modify active all assignment to this quiesced SU, remove 
				 * the SI from the SI admin pointer, change the switch value 
				 * to false. Add the SU to SU operlist. 
				 */
				if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_ACTIVE) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return NCSCC_RC_FAILURE;
				}
				su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
				avd_sg_su_oper_list_add(cb, su, false);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			} /* if(o_susi == AVD_SU_SI_REL_NULL) */
			else if ((o_susi->state == SA_AMF_HA_STANDBY) &&
				 (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {
				/* The standby SU is inservice has standby SI assignment from
				 * another SU, send remove message for each of those SI 
				 * assignment to the standby SU and send D2N-INFO_SU_SI_ASSIGN
				 * message modify active all assignments to that SU.
				 */
				avd_sg_npm_su_chk_snd(cb, o_susi->su, su);
			}

		}		/* if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED) && 
				   (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
				   ((i_susi = avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
				   != AVD_SU_SI_REL_NULL)) */
		else if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_ACTIVE)) {
			for (i_susi = su->list_of_susi;
			     (i_susi != AVD_SU_SI_REL_NULL) && (i_susi->fsm == AVD_SU_SI_STATE_ASGND);
			     i_susi = i_susi->su_next) ;

			if (i_susi == AVD_SU_SI_REL_NULL) {
				/* active all and the entire SU is assigned. */
				if ((su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
				    ((i_susi = avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name))
				     != AVD_SU_SI_REL_NULL)) {
					/* SI in admin pointer with switch state true and SI has 
					 * relationship with this SU. Send D2N-INFO_SU_SI_ASSIGN with 
					 * modify standby all to the Quiesced SU. Remove the SI from 
					 * admin pointer and add the quiesced SU to the SU oper list.
					 */
					if (su->sg_of_su->admin_si->list_of_sisu == i_susi) {
						o_susi = i_susi->si_next;
					} else {
						o_susi = su->sg_of_su->admin_si->list_of_sisu;
					}

					if (avd_sg_su_si_mod_snd(cb, o_susi->su, SA_AMF_HA_STANDBY) == NCSCC_RC_FAILURE) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
						return NCSCC_RC_FAILURE;
					}
					avd_sg_su_oper_list_add(cb, o_susi->su, false);
					su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
				/* if ((su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
				   ((i_susi = avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false))
				   != AVD_SU_SI_REL_NULL)) */
			}
			/* if (i_susi == AVD_SU_SI_REL_NULL) */
		} /* if ((act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_ACTIVE)) */
		else {
			LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		}
	}			/* else (susi != AVD_SU_SI_REL_NULL) */

	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NPM::susi_success(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
		AVSV_SUSI_ACT act, SaAmfHAStateT state) {
	AVD_SU_SI_REL *o_susi;

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
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		if (susi_sucss_sg_reln(cb, su, susi, act, state) == NCSCC_RC_FAILURE)
			return NCSCC_RC_FAILURE;

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:

		if (susi_success_su_oper(cb, su, susi, act, state) == NCSCC_RC_FAILURE)
			return NCSCC_RC_FAILURE;

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		if (avd_sg_npm_susi_sucss_si_oper(cb, su, susi, act, state) == NCSCC_RC_FAILURE)
			return NCSCC_RC_FAILURE;

		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:
		if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) && (state == SA_AMF_HA_QUIESCED)) {
			/* quiesced all Send a D2N-INFO_SU_SI_ASSIGN with remove all for this SU. */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				/* If the admin state is shutdown, identify the corresponding 
				 * standby and send a D2N-INFO_SU_SI_ASSIGN with remove all for 
				 * that SU and add it to the SU oper list.
				 */
				o_susi = avd_sg_npm_su_othr(cb, su);
				if ((o_susi != AVD_SU_SI_REL_NULL) && (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
					avd_sg_su_si_del_snd(cb, o_susi->su);
					avd_sg_su_oper_list_add(cb, o_susi->su, false);
				}

			}
			/* if (su->sg_of_su->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		}		/* if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   (state == SA_AMF_HA_QUIESCED)) */
		else if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_DEL)) {
			/* remove all. Free all the SI assignments to this SU. If the 
			 * SU oper list is empty,  change the SG admin state to LOCK, 
			 * Change state to stable.
			 */
			su->delete_all_susis();
			avd_sg_su_oper_list_del(cb, su, false);
			if (su_oper_list.empty() == true) {
				avd_sg_admin_state_set(su->sg_of_su, SA_AMF_ADMIN_LOCKED);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				/*As sg is stable, screen for si dependencies and take action on whole sg*/
				avd_sidep_update_si_dep_state_for_all_sis(su->sg_of_su);
				avd_sidep_sg_take_action(su->sg_of_su); 
				if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
						(true == su->sg_of_su->equal_ranked_su) &&
						(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
					/* SG fsm is stable, screen for possibility of redistributing SI
			   		to achieve equal distribution */
                        		avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                		}
			}

		} /* else if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_DEL)) */
		else {
			LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)act));
			LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			return NCSCC_RC_FAILURE;
		}

		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NPM::susi_failed(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
		AVSV_SUSI_ACT act, SaAmfHAStateT state) {
	AVD_SU_SI_REL *o_susi;
	AVD_AVND *su_node_ptr = nullptr;

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

		/* log fatal error. */
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:
	case AVD_SG_FSM_SU_OPER:

		if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
		    ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))
		    && (su_oper_list_front() == su)) {
			/* quiesced all and SU is OOS. Send a D2N-INFO_SU_SI_ASSIGN remove
			 * all to the SU.
			 */
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
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

		} else {
			if((susi != AVD_SU_SI_REL_NULL) && (state == SA_AMF_HA_QUIESCED) && (act == AVSV_SUSI_ACT_MOD)){
				/* Check if SI transfer is going on */
				if(su->sg_of_su->si_tobe_redistributed) {
					if((su->sg_of_su->max_assigned_su == su) && (SA_AMF_READINESS_IN_SERVICE ==
							su->sg_of_su->max_assigned_su->saAmfSuReadinessState)) {
						
						/* Failure returned for max_assigned_su quiesced operation,
					   	   Reset the Active state to it */ 
						avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);
						su->sg_of_su->max_assigned_su = nullptr;
                        			su->sg_of_su->min_assigned_su = nullptr;
                        			su->sg_of_su->si_tobe_redistributed = nullptr;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, su->sg_of_su,
										AVSV_CKPT_AVD_SI_TRANS);

                                        	/* max_assigned_su is already in the su_oper_list, move SG to 
						AVD_SG_FSM_SG_REALIGN */
                                        	m_AVD_SET_SG_FSM(avd_cb, (susi->su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					}

				}
			}else {
					/* No action as other call back failure will cause operation disable 
			 		* event to be sent by AvND.
			 		*/
					TRACE("%u", state);
			}
		}

		break;		/* case AVD_SG_FSM_SG_REALIGN: 
				 * case AVD_SG_FSM_SU_OPER:
				 */

	case AVD_SG_FSM_SI_OPER:

		if ((susi != AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
		    ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))
		    && (susi->si == su->sg_of_su->admin_si)) {
			/* message with modified quiesced for a SI in the admin pointer with 
			 * admin values lock/shutdown. Send a D2N-INFO_SU_SI_ASSIGN with remove for 
			 * this SI to this SU. Remove the SI from the admin pointer. Add the 
			 * SU to operation list also add the other SU to which this SI has 
			 * assignment to the operation list. Change state to SG_realign.
			 */

			if (avd_susi_del_send(susi) == NCSCC_RC_FAILURE)
				return NCSCC_RC_FAILURE;

			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			avd_sg_su_oper_list_add(cb, susi->su, false);

			if (susi->si->list_of_sisu != susi) {
				o_susi = susi->si->list_of_sisu;
			} else {
				o_susi = susi->si_next;
			}

			if ((o_susi != AVD_SU_SI_REL_NULL) && (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
				avd_susi_del_send(o_susi);
				avd_sg_su_oper_list_add(cb, o_susi->su, false);
			}

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* if ((susi != AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING)) 
				   && (susi->si == su->sg_of_su->admin_si)) */
		else if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
			 (state == SA_AMF_HA_QUIESCED) && (su->sg_of_su->admin_si != AVD_SI_NULL)
			 && (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
			 (avd_su_susi_find(cb, su, &su->sg_of_su->admin_si->name)
			  != AVD_SU_SI_REL_NULL) && (su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {
			/* Message with modified quiesced all and an SI is in the admin 
			 * pointer with switch value true.  Send D2N-INFO_SU_SI_ASSIGN with 
			 * modified active all to the SU. Change the switch value of the SI .
			 * to false. Remove the SI from the admin pointer. Add the SU to 
			 * operation list. Change state to SG_realign.
			 */

			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_ACTIVE)
			    == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			avd_sg_su_oper_list_add(cb, su, false);

			su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);

			m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   (state == SA_AMF_HA_QUIESCED) && (su->sg_of_su->admin_si != AVD_SI_NULL)
				   && (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) &&
				   (avd_su_susi_struc_find(cb,su,su->sg_of_su->admin_si->name,false)
				   != AVD_SU_SI_REL_NULL) && (su->readiness_state == NCS_IN_SERVICE)) */
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
				return NCSCC_RC_FAILURE;
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
			if (avd_sg_su_si_del_snd(cb, su) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			o_susi = avd_sg_npm_su_othr(cb, su);
			if ((o_susi != AVD_SU_SI_REL_NULL) && (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
				avd_sg_su_si_del_snd(cb, o_susi->su);
				avd_sg_su_oper_list_add(cb, o_susi->su, false);
			}

		}		/* else if ((susi == AVD_SU_SI_REL_NULL) && (act == AVSV_SUSI_ACT_MOD) &&
				   ((state == SA_AMF_HA_QUIESCED) || (state == SA_AMF_HA_QUIESCING))&& 
				   (su->sg_of_su->admin_state == NCS_ADMIN_STATE_SHUTDOWN)) */
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
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;

}

uint32_t SG_NPM::realign(AVD_CL_CB *cb, AVD_SG *sg) {
	TRACE_ENTER2("'%s'", sg->name.value);

	/* If the SG FSM state is not stable just return success. */
	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == false)) {
		goto done;
	}

	if (sg->sg_fsm_state != AVD_SG_FSM_STABLE) {
		set_adjust_state(AVSV_SG_STABLE);
		avd_sg_app_su_inst_func(cb, sg);
		if ((AVD_SG_FSM_STABLE == sg->sg_fsm_state) &&
				(true == sg->equal_ranked_su) &&
				(SA_TRUE == sg->saAmfSGAutoAdjust)) {
			/* SG fsm is stable, screen for possibility of redistributing SI
			to achieve equal distribution */
                	avd_sg_npm_screening_for_si_redistr(sg);
                }
		goto done;
	}

	if (avd_sg_npm_su_chose_asgn(cb, sg) == nullptr) {
		/* all the assignments have already been done in the SG. */
		set_adjust_state(AVSV_SG_STABLE);
		avd_sg_app_su_inst_func(cb, sg);
		goto done;
	}

	set_adjust_state(AVSV_SG_STABLE);
	m_AVD_SET_SG_FSM(cb, (sg), AVD_SG_FSM_SG_REALIGN);

 done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_npm_node_fail_sg_relgn
 *
 * Purpose:  This function is a subfunctionality of avd_sg_npm_node_fail_func.
 * It is called if the SG is in AVD_SG_FSM_SG_REALIGN.
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

static void avd_sg_npm_node_fail_sg_relgn(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *l_susi, *o_susi, *ot_susi;
	bool l_flag = false;
	AVD_AVND *su_node_ptr = nullptr;

	if (su->sg_of_su->admin_si != AVD_SI_NULL) {
		l_susi = o_susi = AVD_SU_SI_REL_NULL;
		if (su->sg_of_su->admin_si->list_of_sisu->su == su) {
			l_susi = su->sg_of_su->admin_si->list_of_sisu;
			o_susi = l_susi->si_next;
		} else if ((su->sg_of_su->admin_si->list_of_sisu->si_next != AVD_SU_SI_REL_NULL) &&
			   (su->sg_of_su->admin_si->list_of_sisu->si_next->su == su)) {
			o_susi = su->sg_of_su->admin_si->list_of_sisu;
			l_susi = o_susi->si_next;
		}
		if (l_susi == AVD_SU_SI_REL_NULL) {
			/* SI doesn't have any assignment with the SU. */
			m_AVD_CHK_OPLIST(su, l_flag);
			if (l_flag == true) {
				/* SU is in the SU oper list. */
				if ((su->list_of_susi->state == SA_AMF_HA_QUIESCED) &&
				    (su->list_of_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {
					/* the SI relationships to the SU state is quiesced assigning. */
					ot_susi = avd_sg_npm_su_othr(cb, su);

					if ((ot_susi != AVD_SU_SI_REL_NULL) &&
					    (ot_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
					    (ot_susi->state == SA_AMF_HA_STANDBY) &&
					    (ot_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
						/* The standby SU w.r.t this SU is inservice has standby 
						 * SI assignment from another SU, send remove message for 
						 * each of those SI assignment to the standby SU and 
						 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
						 * assignments to that SU. Add the SU to the oper list.                   
						 */
						if (avd_sg_npm_su_chk_snd(cb, ot_susi->su, su) == NCSCC_RC_SUCCESS)
							avd_sg_su_oper_list_add(cb, ot_susi->su, false);
					}

				}

				/* if ((su->list_of_susi->state == SA_AMF_HA_QUIESCED) && 
				   (su->list_of_susi->fsm = AVD_SU_SI_STATE_MODIFY)) */
				/* remove the SU from the operation list. */
				avd_sg_su_oper_list_del(cb, su, false);

			} else {	/*if (l_flag == true) */

				/* SU not in the SU oper list */
				if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
					/* The SU is active SU, identify the inservice standby SU for this SU.
					 */

					ot_susi = avd_sg_npm_su_othr(cb, su);

					if ((ot_susi != AVD_SU_SI_REL_NULL) &&
					    (ot_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
					    (ot_susi->state == SA_AMF_HA_STANDBY) &&
					    (ot_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
						/* The standby SU w.r.t this SU is inservice has standby 
						 * SI assignment from another SU, send remove message for 
						 * each of those SI assignment to the standby SU and 
						 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
						 * assignments to that SU. Add the SU to the oper list.                   
						 */
						if (avd_sg_npm_su_chk_snd(cb, ot_susi->su, su) == NCSCC_RC_SUCCESS)
							avd_sg_su_oper_list_add(cb, ot_susi->su, false);
					}

				}
				/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
			}	/* else (l_flag == true) */

			/* Free all the SI assignments to this SU. */
			su->delete_all_susis();

		} else {	/* if (l_susi == AVD_SU_SI_REL_NULL) */

			/* SI has assignment with the SU. */
			if (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) {
				/* the admin pointer SI has switch operation as true. */
				if ((l_susi->state == SA_AMF_HA_QUIESCED) && (l_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {
					/* this SI relation with the SU is quiesced assigning. */
					if (o_susi != AVD_SU_SI_REL_NULL) {
						if ((o_susi->fsm == AVD_SU_SI_STATE_ASGND) &&
						    (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE))
						{
							/* The SUs corresponding standby/quiesced SU is in-service.
							 * If the standby SU has standby SI assignment from 
							 * another SU, send remove message for each of those SI 
							 * assignment to the standby SU and send D2N-INFO_SU_SI_ASSIGN
							 * message modify active all assignments to that SU.
							 * Add the standby SU to operation list.
							 */
							if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
								avd_sg_su_oper_list_add(cb, o_susi->su, false);
						} else if (o_susi->su->saAmfSuReadinessState !=
							   SA_AMF_READINESS_IN_SERVICE) {
							/* Add the standby SU to operation list. */
							avd_sg_su_oper_list_add(cb, o_susi->su, false);
						}
					}

					/* remove the SU from the operation list. */
					avd_sg_su_oper_list_del(cb, su, false);

					/* Free all the SI assignments to this SU. */
					su->delete_all_susis();

					/* Change the SI switch state to false. Remove the SI from the 
					 * SI admin pointer.
					 */
					su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					if (su->sg_of_su->su_oper_list.empty() == true) {
						/* both the SI admin pointer and SU oper list are empty.
						 * Do the functionality as in stable state to verify if 
						 * new assignments can be done. If yes stay in the same state.
						 * If no new assignments change state to stable.
						 */
						if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
							/* all the assignments have already been done in the SG. */
							m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
							avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
						}
					}

				}	/* if ((l_susi->state == SA_AMF_HA_QUIESCED) &&
					   (l_susi->fsm == AVD_SU_SI_STATE_MODIFY)) */
				else if ((l_susi->state == SA_AMF_HA_QUIESCED) &&
					 (l_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
					/* this SI relation with the SU is quiesced assigned. */
					if (o_susi != AVD_SU_SI_REL_NULL) {
						/* Add the other SU to operation list. */
						avd_sg_su_oper_list_add(cb, o_susi->su, false);
					}
					/* Change the SI switch state to false. Remove the SI from the 
					 * SI admin pointer.
					 */
					su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					/* Free all the SI assignments to this SU. */
					su->delete_all_susis();

					if (su->sg_of_su->su_oper_list.empty() == true) {
						/* both the SI admin pointer and SU oper list are empty.
						 * Do the functionality as in stable state to verify if 
						 * new assignments can be done. If yes stay in the same state.
						 * If no new assignments change state to stable.
						 */
						if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
							/* all the assignments have already been done in the SG. */
							m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
							avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
						}
					}
				}	/* else if ((l_susi->state == SA_AMF_HA_QUIESCED) &&
					   (l_susi->fsm == AVD_SU_SI_STATE_ASGND)) */
				else if (l_susi->state == SA_AMF_HA_STANDBY) {
					/* this SI relation with the SU is standby. */
					if (l_susi->fsm != AVD_SU_SI_STATE_ASGND)
						/* remove the SU from the operation list. */
						avd_sg_su_oper_list_del(cb, su, false);

					/* Free all the SI assignments to this SU. */
					su->delete_all_susis();
				} /* else if (l_susi->state == SA_AMF_HA_STANDBY) */
				else if (l_susi->state == SA_AMF_HA_ACTIVE) {
					/* this SI relation with the SU is standby. */
					if (o_susi != AVD_SU_SI_REL_NULL) {
						if ((o_susi->fsm == AVD_SU_SI_STATE_ASGND) &&
						    (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE))
						{
							/* The SUs corresponding quiesced SU is in-service.
							 * send D2N-INFO_SU_SI_ASSIGN
							 * message modify active all assignments to that SU.
							 * Add the quiesced SU to operation list.
							 */
							avd_sg_su_si_mod_snd(cb, o_susi->su, SA_AMF_HA_ACTIVE);
							/* Add the quiesced SU to operation list. */
							avd_sg_su_oper_list_add(cb, o_susi->su, false);
						} else if (o_susi->su->saAmfSuReadinessState !=
							   SA_AMF_READINESS_IN_SERVICE) {
							/* Add the quiesced SU to operation list. */
							avd_sg_su_oper_list_add(cb, o_susi->su, false);
						}
					}

					/* Change the SI switch state to false. Remove the SI from the 
					 * SI admin pointer.
					 */
					su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					/* Free all the SI assignments to this SU. */
					su->delete_all_susis();

					if (su->sg_of_su->su_oper_list.empty() == true) {
						/* both the SI admin pointer and SU oper list are empty.
						 * Do the functionality as in stable state to verify if 
						 * new assignments can be done. If yes stay in the same state.
						 * If no new assignments change state to stable.
						 */
						if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
							/* all the assignments have already been done in the SG. */
							m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
							avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
						}
					}
				}
				/* else if (l_susi->state == SA_AMF_HA_ACTIVE) */
			} else {	/* if (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) */

				/* admin pointer SI admin operation is LOCK/shutdown. */
				if ((l_susi->state == SA_AMF_HA_QUIESCING) || (l_susi->state == SA_AMF_HA_QUIESCED)) {
					/* Change the admin operation to UNLOCK. */
					su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_UNLOCKED);

					if (o_susi != AVD_SU_SI_REL_NULL) {
						if ((o_susi->fsm == AVD_SU_SI_STATE_ASGND) &&
						    (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE))
						{
							/* The SUs corresponding standby/quiesced SU is in-service.
							 * If the standby SU has standby SI assignment from 
							 * another SU, send remove message for each of those SI 
							 * assignment to the standby SU and send D2N-INFO_SU_SI_ASSIGN
							 * message modify active all assignments to that SU.
							 * Add the standby SU to operation list.
							 */
							if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
								avd_sg_su_oper_list_add(cb, o_susi->su, false);
						} else if (o_susi->su->saAmfSuReadinessState !=
							   SA_AMF_READINESS_IN_SERVICE) {
							/* Add the standby SU to operation list. */
							avd_sg_su_oper_list_add(cb, o_susi->su, false);
						}
					}

					/* Free all the SI assignments to this SU. */
					su->delete_all_susis();

					/*  Remove the SI from the SI admin pointer.  */
					m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

					if (su->sg_of_su->su_oper_list.empty() == true) {
						/* both the SI admin pointer and SU oper list are empty.
						 * Do the functionality as in stable state to verify if 
						 * new assignments can be done. If yes stay in the same state.
						 * If no new assignments change state to stable.
						 */
						if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
							/* all the assignments have already been done in the SG. */
							m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
							avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
						}
					}

				}	/* if ((l_susi->state == SA_AMF_HA_QUIESCING) ||
					   (l_susi->state == SA_AMF_HA_QUIESCED)) */
				else if (l_susi->state == SA_AMF_HA_STANDBY) {
					/* this SI relation with the SU is standby. */
					if (l_susi->fsm != AVD_SU_SI_STATE_ASGND)
						/* remove the SU from the operation list. */
						avd_sg_su_oper_list_del(cb, su, false);

					/* Free all the SI assignments to this SU. */
					su->delete_all_susis();

				}	/* else if (l_susi->state == SA_AMF_HA_STANDBY) */
			}	/* else (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) */

		}		/* else (l_susi == AVD_SU_SI_REL_NULL) */

	} else {		/* if (su->sg_of_su->admin_si != AVD_SI_NULL) */

		/* Check if SI transfer operation is going on */
		if (nullptr != su->sg_of_su->si_tobe_redistributed) {
			if (su == su->sg_of_su->min_assigned_su) {
				/* min_assigned_su which is assigned Active role is OOS, max_assigned_su
				   will be in quiesced state. Send Active role to max_assigned_su */
				if (su->sg_of_su->max_assigned_su->saAmfSuReadinessState ==
                                                SA_AMF_READINESS_IN_SERVICE) {
					AVD_SU_SI_REL *susi;

                                        susi = avd_su_susi_find(avd_cb, su->sg_of_su->max_assigned_su,
                                                                &su->sg_of_su->si_tobe_redistributed->name);
                                        osafassert(susi);
					avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);

				}
				su->sg_of_su->max_assigned_su = nullptr;
                        	su->sg_of_su->min_assigned_su = nullptr;
                        	su->sg_of_su->si_tobe_redistributed = nullptr;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
			}
		}

		m_AVD_CHK_OPLIST(su, l_flag);
		if (l_flag == true) {
			/* SU is in the SU oper list. */
			if ((su->list_of_susi->state == SA_AMF_HA_QUIESCED) ||
			    (su->list_of_susi->state == SA_AMF_HA_QUIESCING) ||
			    (su->list_of_susi->state == SA_AMF_HA_ACTIVE)) {
				/* the SI relationships to the SU state is active/quiesced/quiescing assigning. */
				ot_susi = avd_sg_npm_su_othr(cb, su);

				if (ot_susi != AVD_SU_SI_REL_NULL) {
					if ((ot_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
					    (ot_susi->state == SA_AMF_HA_STANDBY) &&
					    (ot_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
						/* The standby SU w.r.t this SU is inservice has standby 
						 * SI assignment from another SU, send remove message for 
						 * each of those SI assignment to the standby SU and 
						 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
						 * assignments to that SU. Add the SU to the oper list.                   
						 */
						if (avd_sg_npm_su_chk_snd(cb, ot_susi->su, su) == NCSCC_RC_SUCCESS)
							avd_sg_su_oper_list_add(cb, ot_susi->su, false);
					} else if (ot_susi->su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) {
						avd_sg_su_oper_list_add(cb, ot_susi->su, false);
					}
				}

				/* Free all the SI assignments to this SU. */
				su->delete_all_susis();

				/* remove the SU from the operation list. */
				avd_sg_su_oper_list_del(cb, su, false);

				su_node_ptr = su->get_node_ptr();

				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					su->set_admin_state(SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					if (su_node_ptr->is_node_lock() == true) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}

			} else {	/* if (su->list_of_susi->state == SA_AMF_HA_QUIESCED) || 
					   (su->list_of_susi->state == SA_AMF_HA_QUIESCING) ||
					   (su->list_of_susi->state == SA_AMF_HA_ACTIVE)) */
				/* Free all the SI assignments to this SU. */
				su->delete_all_susis();

				/* remove the SU from the operation list. */
				avd_sg_su_oper_list_del(cb, su, false);
			}	/* else (su->list_of_susi->state == SA_AMF_HA_QUIESCED) || 
				   (su->list_of_susi->state == SA_AMF_HA_QUIESCING) ||
				   (su->list_of_susi->state == SA_AMF_HA_ACTIVE)) */

			if (su->sg_of_su->su_oper_list.empty() == true) {
				/* both the SI admin pointer and SU oper list are empty.
				 * Do the functionality as in stable state to verify if 
				 * new assignments can be done. If yes stay in the same state.
				 * If no new assignments change state to stable.
				 */
				if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
					/* all the assignments have already been done in the SG. */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
					avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
				}
			}

		} else {	/*if (l_flag == true) */

			/* SU not in the SU oper list */
			if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
				/* The SU is active SU, identify the inservice standby SU for this SU.
				 */

				o_susi = avd_sg_npm_su_othr(cb, su);

				if ((o_susi != AVD_SU_SI_REL_NULL) &&
				    (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
				    (o_susi->state == SA_AMF_HA_STANDBY) && (o_susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
					/* The standby SU w.r.t this SU is inservice has standby 
					 * SI assignment from another SU, send remove message for 
					 * each of those SI assignment to the standby SU and 
					 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
					 * assignments to that SU. Add the SU to the oper list.                   
					 */
					if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
						avd_sg_su_oper_list_add(cb, o_susi->su, false);
				}

			}

			/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
			/* Free all the SI assignments to this SU. */
			su->delete_all_susis();

		}		/* else (l_flag == true) */

	}			/* else (su->sg_of_su->admin_si != AVD_SI_NULL) */
}

/*****************************************************************************
 * Function: node_fail_su_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_npm_node_fail_func.
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

void SG_NPM::node_fail_su_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *o_susi;
	AVD_AVND *su_node_ptr = nullptr;

	TRACE_ENTER2("'%s' ", su->name.value);

	if (su_oper_list_front() == su) {
		/* SU is in the SU oper list. */

		/* Check if we are in SI Transfer flow */
		if(nullptr != su->sg_of_su->si_tobe_redistributed) {
			if(su == su->sg_of_su->max_assigned_su) {
				/* Max_assigned_su is assigned  quiesced state as part of SI transfer and the
			   	node went down */
				su->sg_of_su->max_assigned_su = nullptr;
                        	su->sg_of_su->min_assigned_su = nullptr;
                        	su->sg_of_su->si_tobe_redistributed = nullptr;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
				TRACE("SI transfer failed for SI '%s' as max_assigned_su went down",
				su->sg_of_su->si_tobe_redistributed->name.value);
			}
		}

		/* the SI relationships to the SU state is quiesced/quiescing assigning. */
		o_susi = avd_sg_npm_su_othr(cb, su);

		if (o_susi != AVD_SU_SI_REL_NULL) {
			if (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
				/* The standby SU w.r.t this SU is inservice has standby 
				 * SI assignment from another SU, send remove message for 
				 * each of those SI assignment to the standby SU and 
				 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
				 * assignments to that SU. Add the SU to the oper list.                   
				 */
				if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
					avd_sg_su_oper_list_add(cb, o_susi->su, false);
			} else {
				avd_sg_su_oper_list_add(cb, o_susi->su, false);
			}
		}

		/* Free all the SI assignments to this SU. */
		su->delete_all_susis();

		/* remove the SU from the operation list. */
		avd_sg_su_oper_list_del(cb, su, false);

		su_node_ptr = su->get_node_ptr();
		if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			su->set_admin_state(SA_AMF_ADMIN_LOCKED);
		} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			if (su_node_ptr->is_node_lock() == true) {
				node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
			}
		}

		if (su_oper_list.empty() == true) {
			/* SU oper list are empty.
			 * Do the functionality as in stable state to verify if 
			 * new assignments can be done. If yes change state to SG realign.
			 * If no new assignments change state to stable.
			 */
			if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
				/* all the assignments have already been done in the SG. */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
				avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
			} else {
				/* new assignments need to be done in the SG. */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		} else {
			/* Change state to SG realign. */
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}
	} else {		/* if(su->sg_of_su->su_oper_list.su == su) */

		/* the SU is not the same as the SU in the list */
		if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
			/* The SU is active SU, identify the inservice standby SU for this SU.
			 */
			o_susi = avd_sg_npm_su_othr(cb, su);

			if ((o_susi != AVD_SU_SI_REL_NULL) &&
			    (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE)) {
				/* The standby SU w.r.t this SU is inservice has standby 
				 * SI assignment from another SU, send remove message for 
				 * each of those SI assignment to the standby SU and 
				 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
				 * assignments to that SU. Add the SU to the oper list.                   
				 */
				if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
					avd_sg_su_oper_list_add(cb, o_susi->su, false);
				/* Change state to SG realign. */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		}

		/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
		/* Free all the SI assignments to this SU. */
		su->delete_all_susis();

	}			/* else (su->sg_of_su->su_oper_list.su == su) */

	return;

}

/*****************************************************************************
 * Function: node_fail_si_oper
 *
 * Purpose:  This function is a subfunctionality of avd_sg_npm_node_fail_func.
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

void SG_NPM::node_fail_si_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *l_susi, *o_susi, *ot_susi;
	
	osafassert(this == su->sg_of_su);

	l_susi = o_susi = AVD_SU_SI_REL_NULL;

	if (su->sg_of_su->admin_si->list_of_sisu->su == su) {
		l_susi = su->sg_of_su->admin_si->list_of_sisu;
		o_susi = l_susi->si_next;
	} else if ((su->sg_of_su->admin_si->list_of_sisu->si_next != AVD_SU_SI_REL_NULL) &&
		   (su->sg_of_su->admin_si->list_of_sisu->si_next->su == su)) {
		o_susi = su->sg_of_su->admin_si->list_of_sisu;
		l_susi = o_susi->si_next;
	}

	if (l_susi == AVD_SU_SI_REL_NULL) {
		/* SI doesn't have any assignment with the SU. */

		if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
			/* The SU is active SU, identify the inservice standby SU for this SU.
			 */

			ot_susi = avd_sg_npm_su_othr(cb, su);

			if ((ot_susi != AVD_SU_SI_REL_NULL) &&
			    (ot_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
			    (ot_susi->state == SA_AMF_HA_STANDBY) && (ot_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
				/* The standby SU w.r.t this SU is inservice has standby 
				 * SI assignment from another SU, send remove message for 
				 * each of those SI assignment to the standby SU and 
				 * send D2N -INFO_SU_SI_ASSIGN message modify active all 
				 * assignments to that SU. Add the SU to the oper list.                   
				 */
				if (avd_sg_npm_su_chk_snd(cb, ot_susi->su, su) == NCSCC_RC_SUCCESS)
					avd_sg_su_oper_list_add(cb, ot_susi->su, false);
				/* Change state to SG realign. */
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}
		}

		/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
		/* Free all the SI assignments to this SU. */
		su->delete_all_susis();

	} else {		/* if (l_susi == AVD_SU_SI_REL_NULL) */

		/* SI has assignment with the SU. */
		if (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) {
			/* the admin pointer SI has switch operation as true. */
			if ((l_susi->state == SA_AMF_HA_QUIESCED) && (l_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {
				/* this SI relation with the SU is quiesced assigning. */
				if (o_susi != AVD_SU_SI_REL_NULL) {
					if (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
						/* The SUs corresponding standby/quiesced SU is in-service.
						 * If the standby SU has standby SI assignment from 
						 * another SU, send remove message for each of those SI 
						 * assignment to the standby SU and send D2N-INFO_SU_SI_ASSIGN
						 * message modify active all assignments to that SU.
						 * Add the standby SU to operation list.
						 */
						if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
							avd_sg_su_oper_list_add(cb, o_susi->su, false);
					} else {
						/* Add the standby SU to operation list. */
						avd_sg_su_oper_list_add(cb, o_susi->su, false);
					}
				}

				/* Free all the SI assignments to this SU. */
				su->delete_all_susis();

				/* Change the SI switch state to false. Remove the SI from the 
				 * SI admin pointer.
				 */
				su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

				if (su_oper_list.empty() == true) {
					/* both the SI admin pointer and SU oper list are empty.
					 * Do the functionality as in stable state to verify if 
					 * new assignments can be done. If yes stay in the same state.
					 * If no new assignments change state to stable.
					 */
					if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
						/* all the assignments have already been done in the SG. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
					} else {
						/* Change state to SG realign. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					}
				} else {
					/* Change state to SG realign. */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}

			}	/* if ((l_susi->state == SA_AMF_HA_QUIESCED) &&
				   (l_susi->fsm == AVD_SU_SI_STATE_MODIFY)) */
			else if ((l_susi->state == SA_AMF_HA_QUIESCED) && (l_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
				/* this SI relation with the SU is quiesced assigned. */
				if (o_susi != AVD_SU_SI_REL_NULL) {
					/* Add the other SU to operation list. */
					avd_sg_su_oper_list_add(cb, o_susi->su, false);
				}
				/* Change the SI switch state to false. Remove the SI from the 
				 * SI admin pointer.
				 */
				su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

				/* Free all the SI assignments to this SU. */
				su->delete_all_susis();

				if (su_oper_list.empty() == true) {
					/* both the SI admin pointer and SU oper list are empty.
					 * Do the functionality as in stable state to verify if 
					 * new assignments can be done. If yes stay in the same state.
					 * If no new assignments change state to stable.
					 */
					if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
						/* all the assignments have already been done in the SG. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
					} else {
						/* Change state to SG realign. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					}
				} else {
					/* Change state to SG realign. */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			}	/* else if ((l_susi->state == SA_AMF_HA_QUIESCED) &&
				   (l_susi->fsm == AVD_SU_SI_STATE_ASGND)) */
			else if (l_susi->state == SA_AMF_HA_STANDBY) {
				/* Free all the SI assignments to this SU. */
				su->delete_all_susis();
			} /* else if (l_susi->state == SA_AMF_HA_STANDBY) */
			else if (l_susi->state == SA_AMF_HA_ACTIVE) {
				/* this SI relation with the SU is standby. */
				if (o_susi != AVD_SU_SI_REL_NULL) {
					if (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
						/* The SUs corresponding quiesced SU is in-service.
						 * send D2N-INFO_SU_SI_ASSIGN
						 * message modify active all assignments to that SU.
						 * Add the quiesced SU to operation list.
						 */
						avd_sg_su_si_mod_snd(cb, o_susi->su, SA_AMF_HA_ACTIVE);
						/* Add the quiesced SU to operation list. */
						avd_sg_su_oper_list_add(cb, o_susi->su, false);
					} else {
						/* Add the quiesced SU to operation list. */
						avd_sg_su_oper_list_add(cb, o_susi->su, false);
					}
				}

				/* Change the SI switch state to false. Remove the SI from the 
				 * SI admin pointer.
				 */
				su->sg_of_su->admin_si->set_si_switch(cb, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

				/* Free all the SI assignments to this SU. */
				su->delete_all_susis();

				if (su_oper_list.empty() == true) {
					/* both the SI admin pointer and SU oper list are empty.
					 * Do the functionality as in stable state to verify if 
					 * new assignments can be done. If yes stay in the same state.
					 * If no new assignments change state to stable.
					 */
					if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
						/* all the assignments have already been done in the SG. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
					} else {
						/* Change state to SG realign. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					}
				} else {
					/* Change state to SG realign. */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			}
			/* else if (l_susi->state == SA_AMF_HA_ACTIVE) */
		} else {	/* if (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) */

			/* admin pointer SI admin operation is LOCK/shutdown. */
			if ((l_susi->state == SA_AMF_HA_QUIESCING) || (l_susi->state == SA_AMF_HA_QUIESCED)) {
				/* Change the admin operation to UNLOCK. */
				su->sg_of_su->admin_si->set_admin_state(SA_AMF_ADMIN_UNLOCKED);

				if (o_susi != AVD_SU_SI_REL_NULL) {
					if (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
						/* The SUs corresponding standby/quiesced SU is in-service.
						 * If the standby SU has standby SI assignment from 
						 * another SU, send remove message for each of those SI 
						 * assignment to the standby SU and send D2N-INFO_SU_SI_ASSIGN
						 * message modify active all assignments to that SU.
						 * Add the standby SU to operation list.
						 */
						if (avd_sg_npm_su_chk_snd(cb, o_susi->su, su) == NCSCC_RC_SUCCESS)
							avd_sg_su_oper_list_add(cb, o_susi->su, false);
					} else {
						/* Add the standby SU to operation list. */
						avd_sg_su_oper_list_add(cb, o_susi->su, false);
					}
				}

				/* Free all the SI assignments to this SU. */
				su->delete_all_susis();

				/*  Remove the SI from the SI admin pointer.  */
				m_AVD_CLEAR_SG_ADMIN_SI(cb, (su->sg_of_su));

				if (su_oper_list.empty() == true) {
					/* both the SI admin pointer and SU oper list are empty.
					 * Do the functionality as in stable state to verify if 
					 * new assignments can be done. If yes stay in the same state.
					 * If no new assignments change state to stable.
					 */
					if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) == nullptr) {
						/* all the assignments have already been done in the SG. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
						avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
					} else {
						/* Change state to SG realign. */
						m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					}
				} else {
					/* Change state to SG realign. */
					m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}

			}	/* if ((l_susi->state == SA_AMF_HA_QUIESCING) ||
				   (l_susi->state == SA_AMF_HA_QUIESCED)) */
			else if (l_susi->state == SA_AMF_HA_STANDBY) {
				/* Free all the SI assignments to this SU. */
				su->delete_all_susis();

			}	/* else if (l_susi->state == SA_AMF_HA_STANDBY) */
		}		/* else (su->sg_of_su->admin_si->si_switch == AVSV_SI_TOGGLE_SWITCH) */

	}			/* else (l_susi == AVD_SU_SI_REL_NULL) */

	return;

}

void SG_NPM::node_fail(AVD_CL_CB *cb, AVD_SU *su) {
	AVD_SU *o_su;
	AVD_SU_SI_REL *o_susi;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if (su->list_of_susi == AVD_SU_SI_REL_NULL)
		return;

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
			/* Find Standby SU corresponding to each susi on this SU and do role change */
			avd_sg_npm_stdbysu_role_change(su);

			/* Free all the SI assignments to this SU. */
			su->delete_all_susis();
			/* sg_fsm_state is still STABLE if no standby SU is found to take role change. */
			if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
				/* Try to engage susi assignment if the other su may still have enough
				 * capability (total ActiveSI not greater than saAmfSGMaxActiveSIsperSU)
				 */
				if (avd_sg_npm_su_chose_asgn(cb, su->sg_of_su) != nullptr) {
					su->sg_of_su->set_fsm_state(AVD_SG_FSM_SG_REALIGN);
				} else {
					/* No need to redistribute si here, since at this point in time
					 * it lacks the IN-SERVICE SU to take the assignment, so there
					 * will be no SU that can share the assignment from highloading
					 * SU during SI redistribution. Only take action on sidep state.
					 */
					avd_sidep_sg_take_action(su->sg_of_su);
				}
			}
		} else {	/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

			/* means standby */
			/* identify an SU in the list that is in-service and 
			 * send D2N-INFO_SU_SI_ASSIGN message new standby for all the SIs on
			 * this SU to that are in-service SU. Remove and free all the SI 
			 * assignments to this SU. Add the newly assigned SU to operation list
			 * and Change state to SG_realign. If no other in-service SU exists, 
			 * stay in the stable state.
			 */

			su->delete_all_susis();
			if ((o_su = avd_sg_npm_su_chose_asgn(cb, su->sg_of_su)) != nullptr) {
				/* add the SU to the operation list and change the SG FSM to SG realign. */
				avd_sg_su_oper_list_add(cb, o_su, false);
				m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
			}

		}		/* else (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_REALIGN:

		avd_sg_npm_node_fail_sg_relgn(cb, su);

		break;		/* case AVD_SG_FSM_SG_REALIGN: */
	case AVD_SG_FSM_SU_OPER:

		node_fail_su_oper(cb, su);

		break;		/* case AVD_SG_FSM_SU_OPER: */
	case AVD_SG_FSM_SI_OPER:

		node_fail_si_oper(cb, su);

		break;		/* case AVD_SG_FSM_SI_OPER: */
	case AVD_SG_FSM_SG_ADMIN:
		if (su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
			/* The SU is quiescing.  */
			o_susi = avd_sg_npm_su_othr(cb, su);
			if (o_susi != AVD_SU_SI_REL_NULL) {
				/* if the other SU w.r.t to SIs of this SU is not unassigning, 
				 * send a D2N-INFO_SU_SI_ASSIGN with a remove all for that SU if it
				 * is in-service. Add that SU to the SU oper list.
				 */
				if (o_susi->fsm != AVD_SU_SI_STATE_UNASGN) {
					avd_sg_su_oper_list_add(cb, o_susi->su, false);
					if (o_susi->su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
						avd_sg_su_si_del_snd(cb, o_susi->su);
					}
				}
			}

		}

		/* if (su->sg_of_su->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		/* Free all the SI assignments to this SU. */
		su->delete_all_susis();

		/* remove the SU from the operation list. */
		avd_sg_su_oper_list_del(cb, su, false);

		if (su_oper_list.empty() == true) {
			if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_sg_admin_state_set(su->sg_of_su, SA_AMF_ADMIN_LOCKED);
			}

			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_STABLE);
			/*As sg is stable, screen for si dependencies and take action on whole sg*/
			avd_sidep_update_si_dep_state_for_all_sis(su->sg_of_su);
			avd_sidep_sg_take_action(su->sg_of_su); 
							if ((AVD_SG_FSM_STABLE == su->sg_of_su->sg_fsm_state) &&
									(true == su->sg_of_su->equal_ranked_su) &&
									(SA_TRUE == su->sg_of_su->saAmfSGAutoAdjust)) {
								/* SG fsm is stable, screen for possibility of
								   redistributing SI to achieve equal distribution */
                						avd_sg_npm_screening_for_si_redistr(su->sg_of_su);
                					}
		}

		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		return;
		break;
	}			/* switch(su->sg_of_su->sg_fsm_state) */

	return;
}

uint32_t SG_NPM::su_admin_down(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd) {
	uint32_t rc;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == false)) {
		return NCSCC_RC_FAILURE;
	}

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
			rc = NCSCC_RC_FAILURE;

			/* this is a active SU. */
			if ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) ||
			    ((avnd != nullptr) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED))) {
				/* change the state for all assignments to quiesced. */
				rc = avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED);
			} else if ((su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) || ((avnd != nullptr)
											     &&
											     (avnd->saAmfNodeAdminState
											      ==
											      SA_AMF_ADMIN_SHUTTING_DOWN)))
			{
				/* change the state for all assignments to quiesceing. */
				rc = avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCING);
			}

			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			/* add the SU to the operation list and change the SG FSM to SU operation. */
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SU_OPER);
		} else {	/* if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */

			/* means standby */
			/* change the state for all assignments to unassign and send delete mesage. */
			if (avd_sg_su_si_del_snd(cb, su) != NCSCC_RC_SUCCESS) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}

			/* add the SU to the operation list and change the SG FSM to SG realign. */
			avd_sg_su_oper_list_add(cb, su, false);
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN)
				su->set_admin_state(SA_AMF_ADMIN_LOCKED);
			m_AVD_SET_SG_FSM(cb, (su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
		}		/* else (su->list_of_susi->state == SA_AMF_HA_ACTIVE) */
		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SU_OPER:
		if ((su_oper_list_front() == su) &&
		    (su->list_of_susi->state == SA_AMF_HA_QUIESCING) &&
		    ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) ||
		     ((avnd != nullptr) && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)))) {
			/* If the SU is in the operation list and the SU admin state is lock.
			 * send D2N-INFO_SU_SI_ASSIGN modify quiesced message to the SU. 
			 */
			if (avd_sg_su_si_mod_snd(cb, su, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return NCSCC_RC_FAILURE;
			}
		}
		break;		/* case AVD_SG_FSM_SU_OPER: */
	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch (su->sg_of_su->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NPM::si_admin_down(AVD_CL_CB *cb, AVD_SI *si) {
	AVD_SU_SI_REL *a_susi;

	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == false)) {
		return NCSCC_RC_FAILURE;
	}
	if (si->list_of_sisu == AVD_SU_SI_REL_NULL) {
		return NCSCC_RC_SUCCESS;
	}
	switch (si->sg_of_si->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:

		if (si->list_of_sisu->state == SA_AMF_HA_ACTIVE) {
			a_susi = si->list_of_sisu;
		} else {
			a_susi = si->list_of_sisu->si_next;
		}
		if (si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED) {
			/* SI lock. Identify the SU that is active send D2N-INFO_SU_SI_ASSIGN
			 * modify quiesced for only this SI. Change state to
			 * SI_operation state. Add it to admin SI pointer.
			 */

			/* change the state of the SI to quiesced */

			if (avd_susi_mod_send(a_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;

			/* add the SI to the admin list and change the SG FSM to SI operation. */
			m_AVD_SET_SG_ADMIN_SI(cb, si);
			m_AVD_SET_SG_FSM(cb, (si->sg_of_si), AVD_SG_FSM_SI_OPER);
		} /* if (si->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (si->saAmfSIAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SI shutdown. Identify the SU that is active 
			 * send D2N-INFO_SU_SI_ASSIGN modify quiescing for only this SI. 
			 * Change state to SI_operation state. Add it to admin SI pointer.
			 */

			/* change the state of the SI to quiescing */

			if (avd_susi_mod_send(a_susi, SA_AMF_HA_QUIESCING) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;

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
			} else {
				a_susi = si->list_of_sisu->si_next;
			}

			/* change the state of the SI to quiesced */
			if (avd_susi_mod_send(a_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;

		}
		break;		/* case AVD_SG_FSM_SI_OPER: */
	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)si->sg_of_si->sg_fsm_state));
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, si->name.value, si->name.length);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch (si->sg_of_si->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

uint32_t SG_NPM::sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg) {
	TRACE_ENTER2("%u", sg->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == false)) {
		return NCSCC_RC_FAILURE;
	}

	switch (sg->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* SG lock. Identify the SUs that are active send D2N-INFO_SU_SI_ASSIGN
			 * modify quiesced all for the SUs. identify standby SUs send 
			 * D2N-INFO_SU_SI_ASSIGN remove all for standby SU, add each of these 
			 * SUs to the SU oper list. Change state to SG_admin. If no active SU 
			 * exist, no action, stay in stable state.
			 */

			for (const auto& i_su : sg->list_of_su) {
				if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
					if (i_su->list_of_susi->state == SA_AMF_HA_ACTIVE)
						avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_QUIESCED);
					else
						avd_sg_su_si_del_snd(cb, i_su);

					/* add the SU to the operation list */
					avd_sg_su_oper_list_add(cb, i_su, false);
				}
			}

		} /* if (sg->admin_state == NCS_ADMIN_STATE_LOCK) */
		else if (sg->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			/* SG shutdown. Identify the SU that are  active send 
			 * D2N-INFO_SU_SI_ASSIGN modify quiescing all for this SUs, add 
			 * each of these SUs to the SU oper list.Change state to SG_admin. 
			 * If no active SU exist, change the SG admin state to LOCK, stay 
			 * in stable state.
			 */
			for (const auto& i_su : sg->list_of_su) {
				if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
					if (i_su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
						avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_QUIESCING);

						/* add the SU to the operation list */
						avd_sg_su_oper_list_add(cb, i_su, false);
					}
				}
			}

		} /* if (sg->admin_state == NCS_ADMIN_STATE_SHUTDOWN) */
		else {
			return NCSCC_RC_FAILURE;
		}

		if (su_oper_list.empty() == false) {
			m_AVD_SET_SG_FSM(cb, (sg), AVD_SG_FSM_SG_ADMIN);
		}

		break;		/* case AVD_SG_FSM_STABLE: */
	case AVD_SG_FSM_SG_ADMIN:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			/* If the SG admin state is shutdown, change the admin state of the SG
			 * to lock and send D2N-INFO_SU_SI_ASSIGN modify quiesced messages to
			 * the SUs with quiescing assignment. Also send D2N-INFO_SU_SI_ASSIGNs
			 * with remove all to the SUs with standby assignment and add the 
			 * standby SUs to the SU oper list.
			 */
			for (const auto& i_su : sg->list_of_su) {
				if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
					if (i_su->list_of_susi->state == SA_AMF_HA_QUIESCING) {
						avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_QUIESCED);
					} else if ((i_su->list_of_susi->state == SA_AMF_HA_STANDBY) &&
						   (i_su->list_of_susi->fsm == AVD_SU_SI_STATE_ASGND)) {
						avd_sg_su_si_del_snd(cb, i_su);
						/* add the SU to the operation list */
						avd_sg_su_oper_list_add(cb, i_su, false);
					}

				}
			}

		}		/* if (sg->admin_state == NCS_ADMIN_STATE_LOCK) */
		break;		/* case AVD_SG_FSM_SG_ADMIN: */
	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)sg->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, sg->name.value, sg->name.length);
		return NCSCC_RC_FAILURE;
		break;
	}			/* switch (sg->sg_fsm_state) */

	return NCSCC_RC_SUCCESS;
}

SG_NPM::~SG_NPM() {
}

