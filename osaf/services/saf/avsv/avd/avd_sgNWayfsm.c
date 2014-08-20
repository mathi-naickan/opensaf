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
  the SG state machine, for processing the events related to SG for N-Way
  redundancy model.

******************************************************************************
*/

#include <logtrace.h>

#include <avd.h>
#include <avd_si_dep.h>

/* static function declarations */

static uint32_t avd_sg_nway_su_fault_stable(AVD_CL_CB *, AVD_SU *);
static uint32_t avd_sg_nway_su_fault_sg_realign(AVD_CL_CB *, AVD_SU *);
static uint32_t avd_sg_nway_su_fault_su_oper(AVD_CL_CB *, AVD_SU *);
static uint32_t avd_sg_nway_su_fault_si_oper(AVD_CL_CB *, AVD_SU *);
static uint32_t avd_sg_nway_su_fault_sg_admin(AVD_CL_CB *, AVD_SU *);

static uint32_t avd_sg_nway_susi_succ_sg_realign(AVD_CL_CB *, AVD_SU *, AVD_SU_SI_REL *, AVSV_SUSI_ACT, SaAmfHAStateT);
static uint32_t avd_sg_nway_susi_succ_su_oper(AVD_CL_CB *, AVD_SU *, AVD_SU_SI_REL *, AVSV_SUSI_ACT, SaAmfHAStateT);
static uint32_t avd_sg_nway_susi_succ_si_oper(AVD_CL_CB *, AVD_SU *, AVD_SU_SI_REL *, AVSV_SUSI_ACT, SaAmfHAStateT);
static uint32_t avd_sg_nway_susi_succ_sg_admin(AVD_CL_CB *, AVD_SU *, AVD_SU_SI_REL *, AVSV_SUSI_ACT, SaAmfHAStateT);

static void avd_sg_nway_node_fail_stable(AVD_CL_CB *, AVD_SU *, AVD_SU_SI_REL *);
static void avd_sg_nway_node_fail_su_oper(AVD_CL_CB *, AVD_SU *);
static void avd_sg_nway_node_fail_si_oper(AVD_CL_CB *, AVD_SU *);
static void avd_sg_nway_node_fail_sg_admin(AVD_CL_CB *, AVD_SU *);
static void avd_sg_nway_node_fail_sg_realign(AVD_CL_CB *, AVD_SU *);
static AVD_SU_SI_REL * find_pref_standby_susi(AVD_SU_SI_REL *sisu);

/* macro to determine if all the active sis assigned to a 
   particular su have been successfully engaged by the standby */
#define m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(su, susi, is) \
{ \
   AVD_SU_SI_REL *csusi = 0, *csisu = 0; \
   bool is_si_eng; \
   (is) = true; \
   for (csusi = (su)->list_of_susi; csusi; csusi = csusi->su_next) { \
      if ( (csusi == (susi)) || (SA_AMF_HA_STANDBY == csusi->state) ) \
         continue; \
      if ( (SA_AMF_HA_ACTIVE == csusi->state) || \
           (SA_AMF_HA_QUIESCING == csusi->state) || \
           ((SA_AMF_HA_QUIESCED == csusi->state) && \
            (AVD_SU_SI_STATE_MODIFY == csusi->fsm)) ) { \
         (is) = false; \
         break; \
      } \
      is_si_eng = false; \
      for (csisu = csusi->si->list_of_sisu; csisu; csisu = csisu->si_next) { \
         if (SA_AMF_HA_ACTIVE == csisu->state) break; \
      } \
      if (!csisu || (csisu && (AVD_SU_SI_STATE_ASGND == csisu->fsm))) \
         is_si_eng = true; \
      if (false == is_si_eng) { \
         (is) = false; \
         break; \
      } \
   } \
};

/*****************************************************************************
 * Function: avd_sg_nway_si_func
 *
 * Purpose:  This function is called when a new SI is added to a SG. The SG is
 * of type N-Way redundancy model. This function will perform the functionality
 * described in the SG FSM design. 
 *
 * Input: cb - the AVD control block
 *        si - The pointer to the service instance.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function. If there are
 * any SIs being transitioned due to operator, this call will return
 * failure.
 *
 **************************************************************************/
uint32_t avd_sg_nway_si_func(AVD_CL_CB *cb, AVD_SI *si)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER2(" SI '%s'",si->name.value);

	/* If the SG FSM state is not stable just return success. */
	if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
		TRACE("SG fsm is not stable");
		goto done;
	}

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == SA_FALSE)) {
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, si->sg_of_si->sg_ncs_spec);
		goto done;
	}

	/* assign the si to the appropriate su */
	rc = avd_sg_nway_si_assign(cb, si->sg_of_si);

done:	
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_nway_siswitch_func
 *
 * Purpose:  This function is called when a operator does a SI switch on
 * a SI that belongs to N-Way redundancy model SG. 
 * This will trigger a role change action as described in the SG FSM design.
 *
 * Input: cb - the AVD control block
 *        si - The pointer to the SI that needs to be switched.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function.
 *
 **************************************************************************/
uint32_t avd_sg_nway_siswitch_func(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *curr_susi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2(" SI '%s' sg_fsm_state:%d",si->name.value,si->sg_of_si->sg_fsm_state);

	/* Switch operation is not allowed on NCS SGs, when the AvD is not in
	 * application state, if si has no SU assignments and If the SG FSM state is
	 * not stable.
	 */
	if ((cb->init_state != AVD_APP_STATE) ||
	    (si->sg_of_si->sg_ncs_spec == SA_TRUE) ||
	    (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) || (si->list_of_sisu == AVD_SU_SI_REL_NULL)) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* switch operation is not valid on SIs that have only one assignment.
	 */
	if (si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* identify the active susi rel */
	for (curr_susi = si->list_of_sisu;
	     curr_susi && (curr_susi->state != SA_AMF_HA_ACTIVE); curr_susi = curr_susi->si_next) ;

	if (!curr_susi) {
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* send quiesced assignment */
	rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* set the toggle switch flag */
	m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_SWITCH);

	/* Add the SI to the SG admin pointer and change the SG state to SI_operation. */
	m_AVD_SET_SG_ADMIN_SI(cb, si);
	m_AVD_SET_SG_FSM(cb, si->sg_of_si, AVD_SG_FSM_SI_OPER);

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_nway_su_fault_func
 *
 * Purpose:  This function is called when a SU readiness state changes to
 * OOS due to a fault. It will do the functionality specified in
 * SG FSM.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the service unit.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: None.
 *
 **************************************************************************/
uint32_t avd_sg_nway_su_fault_func(AVD_CL_CB *cb, AVD_SU *su)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2(" SU '%s' sg_fsm_state %u", su->name.value, su->sg_of_su->sg_fsm_state);

	if (!su->list_of_susi) {
		TRACE("No assignments on this SU");
		goto done;
	}

	/* Do the functionality based on the current state. */
	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		rc = avd_sg_nway_su_fault_stable(cb, su);
		break;

	case AVD_SG_FSM_SG_REALIGN:
		rc = avd_sg_nway_su_fault_sg_realign(cb, su);
		break;

	case AVD_SG_FSM_SU_OPER:
		rc = avd_sg_nway_su_fault_su_oper(cb, su);
		break;

	case AVD_SG_FSM_SI_OPER:
		rc = avd_sg_nway_su_fault_si_oper(cb, su);
		break;

	case AVD_SG_FSM_SG_ADMIN:
		rc = avd_sg_nway_su_fault_sg_admin(cb, su);
		break;

	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		rc  = NCSCC_RC_FAILURE;
	}			/* switch */

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_nway_su_insvc_func
 *
 * Purpose:  This function is called when a SU readiness state changes
 * to inservice from out of service. The SG is of type N-Way redundancy
 * model. It will do the functionality specified in the SG FSM.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the service unit.
 *        
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: none.
 *
 **************************************************************************/
uint32_t avd_sg_nway_su_insvc_func(AVD_CL_CB *cb, AVD_SU *su)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("'%s', %u", su->name.value, su->sg_of_su->sg_fsm_state);

	/* An SU will not become in service when the SG is being locked or shutdown.
	 */
	if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_ADMIN) {
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		rc = NCSCC_RC_FAILURE;
        	goto done;
	}

	/* If the SG FSM state is not stable just return success. */
	if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE)
        	goto done;

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == SA_FALSE))
        	goto done;

	/* a new su is available for assignments.. start assigning */
	rc = avd_sg_nway_si_assign(cb, su->sg_of_su);

	if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);
		/* if equal ranked su then screen for redistribution
		 * of the SIs if the SG fsm state is still STABLE */
		if ((su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				(su->sg_of_su->equal_ranked_su == true) &&
				(su->sg_of_su->saAmfSGAutoAdjust == SA_TRUE))
			avd_sg_nway_screen_si_distr_equal(su->sg_of_su);
	}

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_nway_susi_sucss_func
 *
 * Purpose:  This function is called when a SU SI ack function is
 * received from the AVND with success value. The SG FSM for N-Way redundancy
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
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_susi_sucss_func(AVD_CL_CB *cb,
				  AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("sg_fsm_state :%u action:%u state:%u", su->sg_of_su->sg_fsm_state, act, state);

	/* Do the functionality based on the current state. */
	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_SG_REALIGN:
		rc = avd_sg_nway_susi_succ_sg_realign(cb, su, susi, act, state);
		break;

	case AVD_SG_FSM_SU_OPER:
		rc = avd_sg_nway_susi_succ_su_oper(cb, su, susi, act, state);
		break;

	case AVD_SG_FSM_SI_OPER:
		rc = avd_sg_nway_susi_succ_si_oper(cb, su, susi, act, state);
		break;

	case AVD_SG_FSM_SG_ADMIN:
		rc = avd_sg_nway_susi_succ_sg_admin(cb, su, susi, act, state);
		break;

	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}			/* switch */

	if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);
		/* if equal ranked su then screen for redistribution
		 * of the SIs if the SG fsm state is still STABLE */
		if ((su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				(su->sg_of_su->equal_ranked_su == true) &&
				(su->sg_of_su->saAmfSGAutoAdjust == SA_TRUE))
			avd_sg_nway_screen_si_distr_equal(su->sg_of_su);
		if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
			avd_sidep_update_si_dep_state_for_all_sis(su->sg_of_su);
			avd_sidep_sg_take_action(su->sg_of_su);
		}
	}
done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_nway_susi_fail_func
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
 * Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_susi_fail_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SG *sg = su->sg_of_su;
	bool is_eng = false;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("sg_fsm_state :%u action:%u state:%u", su->sg_of_su->sg_fsm_state, act, state);

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_SU_OPER:
		if (susi && (SA_AMF_HA_QUIESCED == susi->state)) {
			/* send remove all msg for all sis for this su */
			rc = avd_sg_su_si_del_snd(cb, susi->su);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
				goto done;
			}
		}
		break;

	case AVD_SG_FSM_SI_OPER:
		if (susi && (SA_AMF_HA_QUIESCED == susi->state) &&
		    (sg->admin_si == susi->si) &&
		    (AVSV_SI_TOGGLE_SWITCH == susi->si->si_switch) &&
		    (SA_AMF_READINESS_IN_SERVICE == susi->su->saAmfSuReadinessState)) {
			/* send active assignment */
			rc = avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;

			/* reset the switch operation */
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
			m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

			/* add the su to su-oper list */
			avd_sg_su_oper_list_add(cb, susi->su, false);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else if (susi && sg->si_tobe_redistributed == susi->si) {
			/* if si redistribution is going on */
			if ((susi->state == SA_AMF_HA_QUIESCED) && (susi->su == sg->max_assigned_su)) {
				/* if the si is being quiesced on max su in SI transfer */
				/* send active assignment */
				rc = avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);
				if (rc == NCSCC_RC_SUCCESS) {
					avd_sg_su_oper_list_add(cb, susi->su, false);
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
				}
			}
		       	else if ((susi->state == SA_AMF_HA_ACTIVE) && (susi->su == sg->min_assigned_su)) {
				AVD_SU_SI_REL *t_susi = NULL;
				/* if the si is assigned active on min su in SI transfer */
				/* delete the quiesced assignment on the max su  and let su failure
				 * handling take care of assigning the standby to active */
				/* find the susi with max assigned su which should be by now quiesced */
				t_susi = avd_su_susi_find(avd_cb, susi->si->sg_of_si->max_assigned_su, &susi->si->name);
				osafassert(t_susi->state == SA_AMF_HA_QUIESCED);

				if (avd_susi_del_send(t_susi) == NCSCC_RC_SUCCESS) {
					/* add the su to su-oper list and move the SG to Realign */
					avd_sg_su_oper_list_add(avd_cb, t_susi->su, false);
					m_AVD_SET_SG_FSM(avd_cb, t_susi->su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
				}
			}
			/* reset all the pointers marked for si transfer */
			sg->si_tobe_redistributed = NULL;
			sg->max_assigned_su = NULL;
			sg->min_assigned_su = NULL;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, sg, AVSV_CKPT_AVD_SI_TRANS);
			TRACE ("SI transfer FAILED '%s'", susi->si->name.value);
		}
		break;

	case AVD_SG_FSM_SG_ADMIN:
		if (susi && (SA_AMF_HA_QUIESCED == state) && (AVSV_SUSI_ACT_DEL != act)) {
			/* => single quiesced failure */
			if (SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) {
				/* determine if the in-svc su has any quiesced assigning assignment */
				if (SA_AMF_READINESS_IN_SERVICE == susi->su->saAmfSuReadinessState) {
					for (curr_susi = susi->su->list_of_susi; curr_susi;
					     curr_susi = curr_susi->su_next) {
						if (curr_susi == susi)
							continue;

						if ((SA_AMF_HA_QUIESCED == curr_susi->state) &&
						    (curr_susi->fsm == AVD_SU_SI_STATE_MODIFY))
							break;
					}
				}

				if (!curr_susi) {
					/* => now remove all can be sent */
					rc = avd_sg_su_si_del_snd(cb, su);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
						goto done;
					}
				}
			}
		}
		break;

	case AVD_SG_FSM_SG_REALIGN:
		if (susi && (SA_AMF_HA_QUIESCING == susi->state)) {
			/* send quiesced assignment */
			rc = avd_susi_mod_send(susi, SA_AMF_HA_QUIESCED);
			if (rc != NCSCC_RC_SUCCESS)
				goto done;
		} else if (susi && (SA_AMF_HA_QUIESCED == susi->state)) {
			if (sg->admin_si != susi->si) {
				/* determine if all the standby sus are engaged */
				m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(susi->su, susi, is_eng);
				if (true == is_eng) {
					/* send remove all msg for all sis for this su */
					rc = avd_sg_su_si_del_snd(cb, susi->su);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value,
										 susi->su->name.length);
						goto done;
					}
				}
			}

			if (false == is_eng) {
				/* send remove one msg */
				rc = avd_susi_del_send(susi);
				if (rc != NCSCC_RC_SUCCESS)
					goto done;
			}
		}
		break;

	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		rc = NCSCC_RC_FAILURE; 
	}			/* switch */
done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_nway_realign_func
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
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: none.
 *
 **************************************************************************/
uint32_t avd_sg_nway_realign_func(AVD_CL_CB *cb, AVD_SG *sg)
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

	/* initiate si assignments */
	avd_sg_nway_si_assign(cb, sg);

	if (sg->sg_fsm_state == AVD_SG_FSM_STABLE) {
		avd_sg_app_su_inst_func(cb, sg);
		/* if equal ranked su then screen for redistribution
		 * of the SIs if the SG fsm state is still STABLE */
		if ((sg->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				(sg->equal_ranked_su == true) &&
				(sg->saAmfSGAutoAdjust == SA_TRUE))
			avd_sg_nway_screen_si_distr_equal(sg);
	}
 done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_nway_node_fail_func
 *
 * Purpose:  This function is called when the node has already failed and
 *           the SIs have to be failed over. It does the functionality 
 *           specified in the design.
 *
 * Input: cb - the AVD control block
 *        su - The SU that has faulted because of the node failure.
 *        
 * Returns: None.
 *
 * Notes: This is a N-Way redundancy model specific function.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_func(AVD_CL_CB *cb, AVD_SU *su)
{
	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if (!su->list_of_susi)
		goto done;

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		avd_sg_nway_node_fail_stable(cb, su, 0);
		break;

	case AVD_SG_FSM_SU_OPER:
		avd_sg_nway_node_fail_su_oper(cb, su);
		break;

	case AVD_SG_FSM_SI_OPER:
		avd_sg_nway_node_fail_si_oper(cb, su);
		break;

	case AVD_SG_FSM_SG_ADMIN:
		avd_sg_nway_node_fail_sg_admin(cb, su);
		break;

	case AVD_SG_FSM_SG_REALIGN:
		avd_sg_nway_node_fail_sg_realign(cb, su);
		break;

	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		goto done;
	}			/* switch */

	if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);
		/* if equal ranked su then screen for redistribution
		 * of the SIs if the SG fsm state is still STABLE */
		if ((su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				(su->sg_of_su->equal_ranked_su == true) &&
				(su->sg_of_su->saAmfSGAutoAdjust == SA_TRUE))
			avd_sg_nway_screen_si_distr_equal(su->sg_of_su);
		if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
			avd_sidep_update_si_dep_state_for_all_sis(su->sg_of_su);
			avd_sidep_sg_take_action(su->sg_of_su);
		}
	}

done:	
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_sg_nway_su_admin_fail
 *
 * Purpose:  This function is called when SU become OOS because of the
 * LOCK or shutdown of the SU or node.The functionality will be as described in
 * the SG design FSM. 
 *
 * Input: cb - the AVD control block
 *        su - The SU that has failed because of the admin operation.
 *        avnd - The AvND structure of the node that is being operated upon.
 *        
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function. The avnd pointer
 * value is valid only if this is a SU operation being done because of the node
 * admin change.
 *
 **************************************************************************/
uint32_t avd_sg_nway_su_admin_fail(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd)
{
	AVD_SU_SI_REL *curr_susi = 0;
	SaAmfHAStateT state;
	bool is_all_stdby = true;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("su '%s' sg_fsm_state:%u",su->name.value, su->sg_of_su->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == SA_FALSE)) {
		goto done;
	}

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if (((SA_AMF_ADMIN_LOCKED == su->saAmfSUAdminState)
		     || (SA_AMF_ADMIN_SHUTTING_DOWN == su->saAmfSUAdminState)) || (avnd
										   &&
										   ((SA_AMF_ADMIN_LOCKED ==
										     avnd->saAmfNodeAdminState)
										    || (SA_AMF_ADMIN_SHUTTING_DOWN ==
											avnd->saAmfNodeAdminState)))) {
			/* determine the modify-state for active sis */
			state = ((SA_AMF_ADMIN_LOCKED == su->saAmfSUAdminState) ||
				 (avnd && (SA_AMF_ADMIN_LOCKED == avnd->saAmfNodeAdminState))) ?
			    SA_AMF_HA_QUIESCED : SA_AMF_HA_QUIESCING;

			/* identify all the active assignments & send quiesced / quiescing assignment */
			for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
				if (SA_AMF_HA_ACTIVE != curr_susi->state)
					continue;

				/* send quiesced / quiescing assignment */
				rc  = avd_susi_mod_send(curr_susi, state);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;

				/* transition to su-oper state */
				m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SU_OPER);

				is_all_stdby = false;
			}	/* for */

			/* if all standby assignments.. remove them */
			if (true == is_all_stdby) {
				rc = avd_sg_su_si_del_snd(cb, su);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}

				if (SA_AMF_ADMIN_SHUTTING_DOWN == su->saAmfSUAdminState) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				}

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
			}

			/* add su to the su-oper list */
			avd_sg_su_oper_list_add(cb, su, false);
		}
		break;

	case AVD_SG_FSM_SU_OPER:
		if ((su->sg_of_su->su_oper_list.su == su) &&
		    ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) ||
		     (avnd && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)))) {
			/* identify all the quiescing assignments & send quiesced assignment */
			for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
				if (SA_AMF_HA_QUIESCING != curr_susi->state)
					continue;

				/* send quiesced assignment */
				rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
				if (rc != NCSCC_RC_SUCCESS)
					goto done;
			}	/* for */
		}
		break;

	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)su->sg_of_su->sg_fsm_state));
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
	}			/* switch */

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_nway_si_admin_down
 *
 * Purpose:  This function is called when SIs admin state is changed to
 * LOCK or shutdown. The functionality will be as described in
 * the SG design FSM. 
 *
 * Input: cb - the AVD control block
 *        si - The SI pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function.
 *
 **************************************************************************/
uint32_t avd_sg_nway_si_admin_down(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *curr_susi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == SA_FALSE)) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* if nothing's assigned, do nothing */
	if (si->list_of_sisu == AVD_SU_SI_REL_NULL)
		goto done;

	switch (si->sg_of_si->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if ((SA_AMF_ADMIN_LOCKED == si->saAmfSIAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == si->saAmfSIAdminState)) {
			/* identify the active susi rel */
			for (curr_susi = si->list_of_sisu;
			     curr_susi && (curr_susi->state != SA_AMF_HA_ACTIVE); curr_susi = curr_susi->si_next) ;

			if (!curr_susi) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
				rc = NCSCC_RC_FAILURE;
				goto done;
			}

			/* send quiesced / quiescing assignment */
			rc  = avd_susi_mod_send(curr_susi, ((SA_AMF_ADMIN_LOCKED == si->saAmfSIAdminState) ?
				SA_AMF_HA_QUIESCED : SA_AMF_HA_QUIESCING));
			if (rc != NCSCC_RC_SUCCESS)
				goto done;

			/* Add the SI to the SG admin pointer and change the SG state to SI_operation. */
			m_AVD_SET_SG_ADMIN_SI(cb, si);
			m_AVD_SET_SG_FSM(cb, si->sg_of_si, AVD_SG_FSM_SI_OPER);
		}
		break;

	case AVD_SG_FSM_SI_OPER:
		if ((si->sg_of_si->admin_si == si) && (SA_AMF_ADMIN_LOCKED == si->saAmfSIAdminState)) {
			/* the si that was being shutdown is now being locked.. 
			   identify the quiescing susi rel */
			for (curr_susi = si->list_of_sisu;
			     curr_susi && (curr_susi->state != SA_AMF_HA_QUIESCING); curr_susi = curr_susi->si_next) ;

			if (!curr_susi) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
				rc = NCSCC_RC_FAILURE;
				goto done;
			}

			/* send quiesced assignment */
			rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
			if (rc != NCSCC_RC_SUCCESS)
				goto done;
		}
		break;

	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)si->sg_of_si->sg_fsm_state));
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, si->name.value, si->name.length);
		rc = NCSCC_RC_FAILURE;
	}			/* switch */

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_nway_sg_admin_down
 *
 * Purpose:  This function is called when SGs admin state is changed to
 * LOCK or shutdown. The functionality will be as described in
 * the SG design FSM. 
 *
 * Input: cb - the AVD control block
 *        sg - The SG pointer.
 *        
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SU *curr_su = 0;
	AVD_SI *curr_si = 0;
	bool is_act_asgn;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%u", sg->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == SA_FALSE)) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	switch (sg->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if ((SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == sg->saAmfSGAdminState)) {
			/* identify & send quiesced / quiescing assignment to all the 
			   active susi assignments in this sg */
			for (curr_su = sg->list_of_su; curr_su; curr_su = curr_su->sg_list_su_next) {
				/* skip the su if there are no assignments */
				if (!curr_su->list_of_susi)
					continue;

				is_act_asgn = false;
				for (curr_susi = curr_su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
					if (SA_AMF_HA_ACTIVE != curr_susi->state)
						continue;

					is_act_asgn = true;
					rc  = avd_susi_mod_send(curr_susi, ((SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) ?
						 SA_AMF_HA_QUIESCED : SA_AMF_HA_QUIESCING));
					if (rc != NCSCC_RC_SUCCESS)
						goto done;
				}	/* for */

				/* if there are no active assignments to this su, send remove all message */
				if (false == is_act_asgn) {
					/* send remove all msg for all sis for this su */
					rc = avd_sg_su_si_del_snd(cb, curr_su);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_su->name.value,
										 curr_su->name.length);
						goto done;
					}
				}

				/* add the su to su-oper list & transition to sg-admin state */
				avd_sg_su_oper_list_add(cb, curr_su, false);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_ADMIN);
			}	/* for */

			/* if sg is still in stable state, lock it */
			if (AVD_SG_FSM_STABLE == sg->sg_fsm_state) {
				avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);
			}
		}
		break;

	case AVD_SG_FSM_SG_ADMIN:
		if (SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) {
			/* pick up all the quiescing susi assignments & 
			   send quiesced assignments to them */
			for (curr_si = sg->list_of_si; curr_si; curr_si = curr_si->sg_list_of_si_next) {
				for (curr_susi = curr_si->list_of_sisu;
				     curr_susi && (SA_AMF_HA_QUIESCING != curr_susi->state);
				     curr_susi = curr_susi->si_next) ;

				if (curr_susi) {
					rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
					if (rc != NCSCC_RC_SUCCESS)
						goto done;
				}
			}	/* for */
		}
		break;

	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uint32_t)sg->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, sg->name.value, sg->name.length);
		rc = NCSCC_RC_FAILURE;
	}			/* switch */
done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/**
 *
 * @brief    This function transfers the si_tobe_redistributed in this sg 
 * 	     from a Maximum assigned SU to a Minimum assinged SU, that are 
 * 	     populated in the AVD_SG fields max_assigned_su, min_assigned_su
 * 	     prior to calling this function. 
 * 	     
 * @param[in] sg
 *
 * @return    none 
 *
 */
static void avd_sg_nway_swap_si_redistr(AVD_SG *sg)
{
	AVD_SU_SI_REL *susi = NULL;

	osafassert(sg->si_tobe_redistributed != NULL);
	osafassert(sg->max_assigned_su != NULL);
	osafassert(sg->min_assigned_su != NULL);

	TRACE_ENTER2("'%s' from '%s' to '%s'", sg->si_tobe_redistributed->name.value,
                sg->max_assigned_su->name.value, sg->min_assigned_su->name.value);
	/* check point the SI transfer parameters with STANDBY*/
	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, sg, AVSV_CKPT_AVD_SI_TRANS);

	/* get the susi that is to be transferred */
	susi = avd_su_susi_find(avd_cb, sg->max_assigned_su, &sg->si_tobe_redistributed->name); 
	osafassert(susi != NULL);


	if (susi->state == SA_AMF_HA_ACTIVE) {
		/* if active assignment of an SI is to be transferred then first make the SU 
		 * which is having the current active assignment to quiesced and then
		 * once the susi response comes make the assignment on the min su
		 */
		if (avd_susi_mod_send(susi, SA_AMF_HA_QUIESCED) == NCSCC_RC_FAILURE) {
			sg->max_assigned_su = NULL;
			sg->min_assigned_su = NULL;
			sg->si_tobe_redistributed = NULL;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, sg, AVSV_CKPT_AVD_SI_TRANS);
		} else {
			m_AVD_SET_SG_FSM(avd_cb, susi->su->sg_of_su, AVD_SG_FSM_SI_OPER);

		}

	} else if (susi->state == SA_AMF_HA_STANDBY) {
		/* if standby assignment is to be transferred simply remove the standby assignment
		 * and let the SI assignment flow with in the SG take care of assigning the removed SI 
		 * to lowest assigned SU
		 */
		if (avd_susi_del_send(susi) == NCSCC_RC_SUCCESS) {
			avd_sg_su_oper_list_add(avd_cb, susi->su, false);
			m_AVD_SET_SG_FSM(avd_cb, susi->su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
		}
		/* reset the min max Sus and si to be redistributed 
		 * as they are not needed further, SG si assignment 
		 * flow will take care of assigning the removed SI 
		 * to the possible minimum assigned SU
		 */
		sg->max_assigned_su = NULL;
		sg->min_assigned_su = NULL;
		sg->si_tobe_redistributed = NULL;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, sg, AVSV_CKPT_AVD_SI_TRANS);
	} else {
		/* susi to be redistributed cannot be in any other HA state */
		osafassert(0);
	}
	TRACE_LEAVE();
}
/**
 * @brief Finds SiRankedSU node corresponding to the given su and si
 *
 * @param [in] si
 * @param [in] su
 * @param suname
 *
 * @returns pointer to avd_sirankedsu_t
 */
avd_sirankedsu_t *avd_si_find_sirankedsu(AVD_SI *si, AVD_SU *su)
{
        avd_sirankedsu_t *temp_su;

        for (temp_su = si->rankedsu_list_head; temp_su != NULL; temp_su = temp_su->next) {
                if (memcmp(&temp_su->suname, &su->name, sizeof(SaNameT)) == 0)
                        break;
        }
        return temp_su;
}
/**
 * @brief check if this SU has any non ranked assignments or not 
 *
 * @param [in] su
 * @param [in] ha_state 
 *
 * @returns true/false
 */
static uint32_t su_has_any_non_ranked_assignment_with_state(AVD_SU *su, SaAmfHAStateT ha_state)
{
	AVD_SU_SI_REL *susi;
	bool found = false;

	TRACE_ENTER2("'%s' for state '%u'",su->name.value,ha_state);

	for (susi = su->list_of_susi;susi != NULL;susi = susi->su_next) {
		if (susi->state == ha_state) {
                        if (NULL == susi->si->rankedsu_list_head) {
                                TRACE("No sirankedsu_assignment ");
                                found = true;
                                break;
                        } else {
                                /* Check whether the susi assignment is based on SIRankedSU */
                                if (avd_si_find_sirankedsu(susi->si, susi->su) == NULL) {
                                        TRACE("NON sirankedsu_assignment");
                                        found = true;
                                        break;
                                }

                        }
                }
	}
	TRACE_LEAVE2(" '%s'",found ? "found":"not found");
	return found;
}
/**
 * @brief  Finds the SI whose assignment has to be transferred from max_assigned_su to min_assigned_su. 
 *	   Interates through the list of assignments on the mas_assigned_su, figure out less ranked SI
 *	   that does not have assignment on the min_assigned_su. Also check that the SI to be transferred
 *	   does not have SIRankedSU assignment on the max_assigned_su
 *
 * @param[in] sg
 * @param[in] ha_state
 *
 * @return si to be transferred
 */
static AVD_SI * find_the_si_to_transfer(AVD_SG *sg, SaAmfHAStateT ha_state)
{
	AVD_SI *si_to_transfer = NULL;
	AVD_SU_SI_REL *susi = sg->max_assigned_su->list_of_susi;
	TRACE_ENTER();

	if (sg->max_assigned_su == NULL) {
		/* should not hit this point */
		goto done;
	}
	for (susi = sg->max_assigned_su->list_of_susi;susi != NULL;susi = susi->su_next) {
		if (susi->state == ha_state) {
			if (SA_AMF_HA_STANDBY == ha_state) {
				/* Find if this is assigned to avd_sg->min_assigned_su. */
				if (avd_su_susi_find(avd_cb, sg->min_assigned_su, &susi->si->name) != AVD_SU_SI_REL_NULL) {
					/* min_assigned_su already has a assignment for this SI go to the
					 * next SI.
					 */
					continue;
				}
			}
			/* Check whether this assignment is based on SIRankedSU, if so dont consider this SI for transfer */
			if (avd_si_find_sirankedsu(susi->si, susi->su) != NULL) {
				continue;
			}
			if (si_to_transfer == NULL)
				si_to_transfer = susi->si;
			else if (si_to_transfer->saAmfSIRank < susi->si->saAmfSIRank)
				si_to_transfer = susi->si;
		}
	}
done:
        TRACE_LEAVE2(" '%s'",si_to_transfer ? (char *)si_to_transfer->name.value:"Not found");
        return si_to_transfer;
}
/**
 *
 * @brief   This routine scans the SU list for the specified SG & picks up 
 *          an SU with minimum number of assignments and SU maximum number 
 *          of assignments and initiates a transfer of SI from maximum 
 *          assigned SU to minimum assigned SU. 
 *          While choosing max_assigned_su(from_su) SU which has atleast one
 *          non SIRankedSU assignments is considered
 *
 * @param[in] sg 
 *
 * @return none
 */
void avd_sg_nway_screen_si_distr_equal(AVD_SG *sg)
{
	AVD_SU *curr_su = NULL;
	AVD_SI *si_to_transfer = NULL;

	TRACE_ENTER();

	sg->max_assigned_su = NULL; 
	sg->min_assigned_su = NULL;
	sg->si_tobe_redistributed = NULL;

	/* first screen for equal number of active assignments 
	 * among the SUs if active assignments are already 
	 * equally distributed among the in-service SUs then
	 * check for equal distribution of standby assignments 
	 */

	/* do screening for active assignements */
	for (curr_su = sg->list_of_su;curr_su != NULL;curr_su = curr_su->sg_list_su_next) {
		if (curr_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {

			/* set the  min su ptr to the first inservice su */
                        if (sg->min_assigned_su == NULL) {
                                sg->min_assigned_su = curr_su;
                        }
                        if (curr_su->saAmfSUNumCurrActiveSIs < sg->min_assigned_su->saAmfSUNumCurrActiveSIs)
                                sg->min_assigned_su = curr_su;

                        /* Check whether the curr_su has atleast one non SIrankedSU assignment */
                        if (su_has_any_non_ranked_assignment_with_state(curr_su, SA_AMF_HA_ACTIVE) == true) {
                                if (sg->max_assigned_su == NULL)
                                        sg->max_assigned_su = curr_su;

                                if (curr_su->saAmfSUNumCurrActiveSIs > sg->max_assigned_su->saAmfSUNumCurrActiveSIs)
                                        sg->max_assigned_su = curr_su;
                        }
                }
	}

	if (sg->min_assigned_su == NULL) {
		/* this means there are no inservice sus nothing to be done */
		TRACE("No redistribution of SI assignments needed");
		goto done;
	}
	if (sg->max_assigned_su == NULL) {
		/* There is no SU with non sirankedsu assignments */
		TRACE("No redistribution of Active assignments needed");
		goto screen_standby_assignments;
	}
	if ((sg->max_assigned_su->saAmfSUNumCurrActiveSIs - sg->min_assigned_su->saAmfSUNumCurrActiveSIs) > 1) {
		TRACE("Redistribution of active assignments initiated");
		TRACE("max_assigned_su '%s' no of act assignments '%u', min_assigned_su '%s' no of act assignments '%u'", 
				sg->max_assigned_su->name.value, sg->max_assigned_su->saAmfSUNumCurrActiveSIs, 
				sg->min_assigned_su->name.value, sg->min_assigned_su->saAmfSUNumCurrActiveSIs);
		
		/* Now find the SI whose active assignment needs to be transferred, find the SI whose  assignment
		 * has to be changed, figure out less ranked SI if SU has more than one assignments.
		 * Also check that the SI to be transferred does not have SIRankedSU assignment on the max_assigned_su
		 */
                si_to_transfer = find_the_si_to_transfer(sg, SA_AMF_HA_ACTIVE);

                if (si_to_transfer != NULL) {
                        sg->si_tobe_redistributed = si_to_transfer;
                        avd_sg_nway_swap_si_redistr(sg);
                        goto done;
                }
                /* if (si_to_transfer == NULL) means that there is no SI that is assigned on max SU
                 * and not assigned on min SU or thre is no non SIRankedSU assignment on the max su.
                 * Here we cannot redistribute the Active SI as of now
                 */

                TRACE("Not possible to transfer SI from max_su");
	}

	TRACE("No redistribution of active assignments needed");
	TRACE("max_assigned_su '%s' no of act assignments '%u', min_assigned_su '%s' no of act assignments '%u'", 
			sg->max_assigned_su->name.value, sg->max_assigned_su->saAmfSUNumCurrActiveSIs, 
			sg->min_assigned_su->name.value, sg->min_assigned_su->saAmfSUNumCurrActiveSIs);

screen_standby_assignments:
	/* do screening for standby assignments */
	sg->max_assigned_su = sg->min_assigned_su = NULL;

	for (curr_su = sg->list_of_su;curr_su != NULL;curr_su = curr_su->sg_list_su_next) {
		if (curr_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
			/* set the  min su ptr to the first inservice su */
                        if (sg->min_assigned_su == NULL) {
                                sg->min_assigned_su = curr_su;
                        }
                        if (curr_su->saAmfSUNumCurrStandbySIs < sg->min_assigned_su->saAmfSUNumCurrStandbySIs)
                                sg->min_assigned_su = curr_su;

                        /* Check whether the curr_su has atleast one non SIrankedSU assignment */
                        if (su_has_any_non_ranked_assignment_with_state(curr_su, SA_AMF_HA_STANDBY) == true) {
                                if (sg->max_assigned_su == NULL)
                                        sg->max_assigned_su = curr_su;

                                if (curr_su->saAmfSUNumCurrStandbySIs > sg->max_assigned_su->saAmfSUNumCurrStandbySIs)
                                        sg->max_assigned_su = curr_su;
                        }
                }
	}
	if (sg->max_assigned_su == NULL) {
                /* There is no SU with non sirankedsu assignments */
                TRACE("No redistribution of Standby assignments needed");
                goto done;
	}
	if ((sg->max_assigned_su->saAmfSUNumCurrStandbySIs - sg->min_assigned_su->saAmfSUNumCurrStandbySIs) > 1) {
		TRACE("Redistribution of standby assignments initiated");
		TRACE("max_assigned_su '%s' no of std assignments '%u', min_assigned_su '%s' no of std assignments '%u'", 
				sg->max_assigned_su->name.value, sg->max_assigned_su->saAmfSUNumCurrStandbySIs, 
				sg->min_assigned_su->name.value, sg->min_assigned_su->saAmfSUNumCurrStandbySIs);

		/* Now find the SI whose standby assignment needs to be transferred, find the SI whose  assignment
		 * has to be changed, figure out less ranked SI if SU has more than one assignments. Do check that
		 * max_assigned_su and min_assigned_su don't share the same SI assignment, Also check that the SI
		 * to be transferred does not have SIRankedSU assignment on the max_assigned_su
		 */
                si_to_transfer = find_the_si_to_transfer(sg, SA_AMF_HA_STANDBY);

                if (si_to_transfer != NULL ) {
                        sg->si_tobe_redistributed = si_to_transfer;
                        avd_sg_nway_swap_si_redistr(sg);
                        goto done;
                }
                /* if (si_to_transfer == NULL) means that there is no SI that is assigned on max SU
                 * and not assigned on min SU or thre is no non SIRankedSU assignment on the max su.
                 * Here we cannot redistribute the SI as of now
                 */
		TRACE("Standby assignments redistribution not possible at this time");
	}

	/* Here means that the SG is load balanced for both 
	 * active and standby SI assignments */
	TRACE("No redistribution of standby assignments needed");
	TRACE("max_assigned_su '%s' no of std assignments '%u', min_assigned_su '%s' no of std assignments '%u'", 
			sg->max_assigned_su->name.value, sg->max_assigned_su->saAmfSUNumCurrStandbySIs, 
			sg->min_assigned_su->name.value, sg->min_assigned_su->saAmfSUNumCurrStandbySIs);
done:
	TRACE_LEAVE();
}
/**
 *
 * @brief   This routine scans the SU list for the specified SG & picks up 
 *          an SU for preferred number of standby assignment for given SI. 
 *
 * @param[in] curr_si 
 *        
 * @param[in] sg 
 *
 * @return AVD_SU - pointer to an qualified service unit
 */
AVD_SU *avd_sg_nway_get_su_std_equal(AVD_SG *sg, AVD_SI *curr_si)
{
	AVD_SU *curr_su = NULL;
	AVD_SU *pref_su = NULL;
	bool l_flag = false;
	int curr_su_stdby_cnt = 0;
	int pref_su_stdby_cnt = 0;

	TRACE_ENTER2("SI to be assigned : %s", curr_si->name.value);

	for (curr_su = sg->list_of_su; curr_su; curr_su = curr_su->sg_list_su_next) {
		/* verify if this su can take the standby assignment */
		if ((curr_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
				((curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU != 0) &&
				 (curr_su->saAmfSUNumCurrStandbySIs >= curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU)))
			continue;

		l_flag = true;

		/* Get the current no of Standby assignments on the su */
		curr_su_stdby_cnt = avd_su_get_current_no_of_assignments(curr_su, SA_AMF_HA_STANDBY);

		/* first try to select an SU which has no assignments */
		if ((curr_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
				(curr_su->saAmfSUNumCurrActiveSIs == 0) &&
				(curr_su->saAmfSUNumCurrStandbySIs == 0)) {
			/* got an SU without any assignments select 
			 * it for this SI's standby assignment
			 */
			pref_su = curr_su;
			break;
		}
		/* else try to select an SU with Standby assignments */
		else if ((curr_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
                                ((curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU == 0) ||
                                 (curr_su_stdby_cnt < curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU)) &&
                                (!pref_su || pref_su_stdby_cnt > curr_su_stdby_cnt) &&
                                (avd_su_susi_find(avd_cb, curr_su, &curr_si->name) == AVD_SU_SI_REL_NULL)) {
                        /* mark this as the preferred SU */
                        pref_su = curr_su;
                        pref_su_stdby_cnt = curr_su_stdby_cnt;
                }
	}
	/* no more SIs can be assigned */
	if (l_flag == false)
		curr_si = NULL;

	return pref_su;
}
/**
 * @brief finds the highest siranked SU for a particular SI
 *        Also finds the su for which Active role is assigned
 *
 * @param [in] si
 * @param [in] su
 * @param [out] assigned_su
 *
 * @returns pointer to the highest ranked SU
 */
AVD_SU *avd_sg_nway_si_find_highest_sirankedsu(AVD_CL_CB *cb, AVD_SI *si, AVD_SU **assigned_su)
{
	AVD_SU_SI_REL *sisu;
	avd_sirankedsu_t *assigned_sirankedsu = NULL;
	AVD_SU *pref_sirankedsu = NULL;
	AVD_SU *ranked_su;

	TRACE_ENTER2("for SI '%s'",si->name.value);

	/* Find the SU for which Active role is assigned for this SI */
	for (sisu = si->list_of_sisu; sisu ; sisu = sisu->si_next) {
		if (SA_AMF_HA_ACTIVE == sisu->state) {
			*assigned_su = sisu->su;
			break;
		}
	}
	/* Find the corresponding sirankedsu */
	if (sisu)
		assigned_sirankedsu = avd_si_find_sirankedsu(si, sisu->su);
	else {
		/* Atleast one Active assignment should be there */
		LOG_ER("Not able to find the Active assignment");
		goto done;
	}
	
	/* Iterate through the si->rankedsu_list_head to find the highest sirankedsu */
	avd_sirankedsu_t *sirankedsu = si->rankedsu_list_head;
	for (; sirankedsu ; sirankedsu = sirankedsu->next) {
		if ((ranked_su = avd_su_get(&sirankedsu->suname)) != NULL) {
			if (ranked_su == *assigned_su) {
                                TRACE("SI is assigned to highest SIRankedSU for this SI");
                                break;
			} else {
				if ((ranked_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
					(ranked_su->saAmfSUNumCurrActiveSIs < si->sg_of_si->saAmfSGMaxActiveSIsperSU)) {
					if (assigned_sirankedsu) {
						if (assigned_sirankedsu->saAmfRank > sirankedsu->saAmfRank) {
							pref_sirankedsu = ranked_su;
							break;
						}
					} else {
						pref_sirankedsu = ranked_su;
						break;
					}
				}
			}
		}
	}
done:
	TRACE_LEAVE2(" '%s'",pref_sirankedsu ? (char *)pref_sirankedsu->name.value:"assigned to highest SIRankedSU");
	return pref_sirankedsu;
}

/*****************************************************************************
 * Function : avd_sg_nway_si_assign
 *
 * Purpose  : This routine scans the SI list for the specified SG & picks up 
 *            an unassigned SI for active assignment. If no unassigned SI is 
 *            found, it picks up the SI that has only active assignment & 
 *            assigns standbys to it (as many as possible).
 *            This routine is invoked when a new-si assignment is received or 
 *            the SG FSM upheaval is over.
 *
 * Input    : cb - the AVD control block
 *            sg - ptr to SG
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : This routine update the su-oper list & moves to sg-realign 
 *            state if any su-si assignment message is sent to AvND.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_si_assign(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SI *curr_si = 0;
	AVD_SU *curr_su = NULL;
	AVD_SU *pref_su = NULL;
	AVD_SUS_PER_SI_RANK_INDX i_idx;
	AVD_SUS_PER_SI_RANK *su_rank_rec = 0;
	bool is_act_ass_sent = false, is_all_su_oos = true, is_all_si_ok = false, su_found = true;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVD_SU_SI_REL *tmp_susi;

	TRACE_ENTER2("%s", sg->name.value);

	m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_STABLE);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, sg, AVSV_CKPT_SG_FSM_STATE);

	avd_sidep_update_si_dep_state_for_all_sis(sg);
	/* assign active assignments to unassigned sis */
	for (curr_si = sg->list_of_si; curr_si; curr_si = curr_si->sg_list_of_si_next) {

		/* verify if si is ready and needs an assignment */
		if ((curr_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) ||
		    (curr_si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED) ||
		    (curr_si->si_dep_state == AVD_SI_UNASSIGNING_DUE_TO_DEP) ||
		    (curr_si->si_dep_state == AVD_SI_READY_TO_UNASSIGN) ||
		    (curr_si->list_of_csi == NULL))
			continue;

		is_all_si_ok = true;

		if (curr_si->list_of_sisu && sg->saAmfSGAutoAdjust && (curr_si->rankedsu_list_head)) {
			/* The SI has sirankedsu configured and auto adjust enabled, make
			 *sure the highest ranked SU has the active assignment
			 */
			AVD_SU *preferred_su = NULL;
			AVD_SU *assigned_su = NULL;
			preferred_su  = avd_sg_nway_si_find_highest_sirankedsu(cb, curr_si, &assigned_su);
			if ((preferred_su && assigned_su) && preferred_su != assigned_su) {
				TRACE("Move SI '%s' to su '%s'",curr_si->name.value, preferred_su->name.value);
				sg->si_tobe_redistributed = curr_si;
				sg->max_assigned_su = assigned_su;
				sg->min_assigned_su = preferred_su;
				avd_sg_nway_swap_si_redistr(sg);
				goto done;
			} else {
				// leave SI assigned as is
				continue;
			}
		}

		/* verify if si is unassigned */
		if (curr_si->list_of_sisu)
			continue;

		if (curr_si->saAmfSIAssignmentState == SA_AMF_ASSIGNMENT_FULLY_ASSIGNED) {
			TRACE_1("SI fully assigned, next SI");
			continue;
		}
		/* we've an unassigned si.. find su for active assignment */

		/* first, scan based on su rank for this si */
		memset((uint8_t *)&i_idx, '\0', sizeof(i_idx));
		i_idx.si_name = curr_si->name;
		i_idx.su_rank = 0;
		curr_su = NULL;
		for (su_rank_rec = avd_sirankedsu_getnext_valid(cb, i_idx, &curr_su);
		     su_rank_rec; su_rank_rec = avd_sirankedsu_getnext_valid(cb, su_rank_rec->indx, &curr_su)) {
			if (m_CMP_HORDER_SANAMET(su_rank_rec->indx.si_name, curr_si->name) != 0) {
				curr_su = 0;
				break;
			}

			if (!curr_su)
				continue;

			if ((curr_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
			    ((curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU == 0) ||
			     (curr_su->saAmfSUNumCurrActiveSIs < curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU)))
				break;

			/* reset the curr-su ptr */
			curr_su = 0;
		}

		/* if not found, scan based on su rank for the sg */
		if (!curr_su) {
			/* Reset pref_su for every SI */	
			pref_su = NULL;

			for (curr_su = sg->list_of_su; curr_su; curr_su = curr_su->sg_list_su_next) {
				if (SA_AMF_READINESS_IN_SERVICE == curr_su->saAmfSuReadinessState) {
					is_all_su_oos = false;
					/* if equal distribution is needed when all SUs are of
					 * equal rank or no rank configured for any of the SU
					 */
					if (sg->equal_ranked_su == true) {
						/* first try to select an SU which has no assignments */
						if ((curr_su->saAmfSUNumCurrActiveSIs == 0) &&
							(curr_su->saAmfSUNumCurrStandbySIs == 0)) { 
							/* got an SU without any assignments select 
							 * it for this SI's active assignment
							 */
							pref_su = curr_su;
							break;
						}
						/* else try to select an SU with least active assignments */
						else {
							if ((curr_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
								((curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU == 0) ||
								 (curr_su->saAmfSUNumCurrActiveSIs < curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU)) &&
								(!pref_su || pref_su->saAmfSUNumCurrActiveSIs > curr_su->saAmfSUNumCurrActiveSIs)) 
								/* mark this as the preferred SU */
								pref_su = curr_su;
						}
					}
					else if (((curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU == 0) ||
						  (curr_su->saAmfSUNumCurrActiveSIs <
						   curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU)))
						break;
				}
			}

			if (true == is_all_su_oos) {
				rc = NCSCC_RC_SUCCESS;
				goto done;
			}

			/* pref_su is used only for equal_ranked_su, so adding the check */
			if (true == sg->equal_ranked_su)
				curr_su = pref_su;
		}

		/* if found, send active assignment */
		if (curr_su) {
			/* set the flag */
			is_act_ass_sent = true;

			rc = avd_new_assgn_susi(cb, curr_su, curr_si, SA_AMF_HA_ACTIVE, false, &tmp_susi);
			if (NCSCC_RC_SUCCESS == rc) {
				/* add su to the su-oper list & change the fsm state to sg-realign */
				avd_sg_su_oper_list_add(cb, curr_su, false);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_si->name.value, curr_si->name.length);
			}
		} else
			break;
	}			/* for */

	if ((true == is_act_ass_sent) || (false == is_all_si_ok))
		goto done;

	/* assign standby assignments to the sis */
	for (curr_si = sg->list_of_si; curr_si && (true == su_found); curr_si = curr_si->sg_list_of_si_next) {
		/* verify if si is ready */
		if (curr_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED)
			continue;

		/* this si has to have one active assignment */
		if (!curr_si->list_of_sisu)
			continue;

		/* verify if si needs more assigments */
		if (m_AVD_SI_STDBY_CURR_SU(curr_si) == m_AVD_SI_STDBY_MAX_SU(curr_si))
			continue;

		/* we've a not-so-fully-assigned si.. find sus for standby assignment */

		/* first, scan based on su rank for this si */
		memset((uint8_t *)&i_idx, '\0', sizeof(i_idx));
		i_idx.si_name = curr_si->name;
		i_idx.su_rank = 0;
		for (su_rank_rec = avd_sirankedsu_getnext_valid(cb, i_idx, &curr_su);
		     su_rank_rec && (m_CMP_HORDER_SANAMET(su_rank_rec->indx.si_name, curr_si->name) == 0);
		     su_rank_rec = avd_sirankedsu_getnext_valid(cb, su_rank_rec->indx, &curr_su)) {
			/* verify if this su can take the standby assignment */
			if (!curr_su || (curr_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
			    ((curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU != 0) &&
			     (curr_su->saAmfSUNumCurrStandbySIs >= curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU)))
				continue;

			/* verify if this su does not have this assignment */
			if (avd_su_susi_find(cb, curr_su, &curr_si->name) != AVD_SU_SI_REL_NULL)
				continue;

			/* send the standby assignment */
			rc = avd_new_assgn_susi(cb, curr_su, curr_si, SA_AMF_HA_STANDBY, false, &tmp_susi);
			if (NCSCC_RC_SUCCESS == rc) {
				/* add su to the su-oper list & change the fsm state to sg-realign */
				avd_sg_su_oper_list_add(cb, curr_su, false);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

				/* verify if si needs more assigments */
				if (m_AVD_SI_STDBY_CURR_SU(curr_si) == m_AVD_SI_STDBY_MAX_SU(curr_si))
					break;
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_si->name.value, curr_si->name.length);
			}
		}

		/* verify if si needs more assigments */
		if (m_AVD_SI_STDBY_CURR_SU(curr_si) == m_AVD_SI_STDBY_MAX_SU(curr_si))
			continue;

		if (sg->equal_ranked_su == true) {

			while (true) {
				/* do the preferred number of standby assignments in equal way */
				curr_su = avd_sg_nway_get_su_std_equal(sg, curr_si);

				if (curr_su == NULL)
					break;

				/* send the standby assignment */
				rc = avd_new_assgn_susi(cb, curr_su, curr_si, SA_AMF_HA_STANDBY, false, &tmp_susi);
				if (NCSCC_RC_SUCCESS == rc) {
					/* add su to the su-oper list & change the fsm state to sg-realign */
					avd_sg_su_oper_list_add(cb, curr_su, false);
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

					/* verify if si needs more assigments */
					if (m_AVD_SI_STDBY_CURR_SU(curr_si) == m_AVD_SI_STDBY_MAX_SU(curr_si))
						break;
				} else {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_si->name.value, curr_si->name.length);
				}
			}
			/* skip to the next SI to be assigned */
			if (curr_si == NULL) /* no more si to be assigned */
				break;
			else /* continue with the next SI assignment */
				continue;
		}

		su_found = false;

		/* next, scan based on su rank for the sg */
		for (curr_su = sg->list_of_su; curr_su; curr_su = curr_su->sg_list_su_next) {
			/* verify if this su can take the standby assignment */
			if (!curr_su || (curr_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
					((curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU != 0) &&
					 (curr_su->saAmfSUNumCurrStandbySIs >= curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU)))
				continue;

			su_found = true;

			/* verify if this su does not have this assignment */
			if (avd_su_susi_find(cb, curr_su, &curr_si->name) != AVD_SU_SI_REL_NULL) 
				continue;

			/* send the standby assignment */
			rc = avd_new_assgn_susi(cb, curr_su, curr_si, SA_AMF_HA_STANDBY, false, &tmp_susi);
			if (NCSCC_RC_SUCCESS == rc) {
				/* add su to the su-oper list & change the fsm state to sg-realign */
				avd_sg_su_oper_list_add(cb, curr_su, false);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

				/* verify if si needs more assigments */
				if (m_AVD_SI_STDBY_CURR_SU(curr_si) == m_AVD_SI_STDBY_MAX_SU(curr_si))
					break;
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_si->name.value, curr_si->name.length);
			}
		}		/* for */
	}			/* for */

done:
	TRACE_LEAVE2("rc '%u' sg_fsm_state '%u'",rc, sg->sg_fsm_state);
	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_su_fault_stable
 *
 * Purpose  : This routine handles the su-fault event in the stable state.
 *
 * Input    : cb - the AVD control block
 *            su - ptr to SU
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_su_fault_stable(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0;
	bool is_all_stdby = true;
	uint32_t rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER2("SU '%s'",su->name.value);

	if (!su->list_of_susi) {
		TRACE("No assignments on this SU");
		goto done;
	}

	/* identify all the active assignments & send quiesced assignment */
	for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
		if (SA_AMF_HA_ACTIVE != curr_susi->state)
			continue;

		/* send quiesced assignment */
		rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
		if (rc != NCSCC_RC_SUCCESS)
			goto done;

		/* transition to su-oper state */
		m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SU_OPER);

		is_all_stdby = false;
	}			/* for */

	/* if all standby assignments.. remove them */
	if (true == is_all_stdby) {
		rc = avd_sg_su_si_del_snd(cb, su);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			goto done;
		}

		/* transition to sg-realign state */
		m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
	}

	/* add su to the su-oper list */
	avd_sg_su_oper_list_add(cb, su, false);

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_su_fault_sg_realign
 *
 * Purpose  : This routine handles the su-fault event in the sg-realign state.
 *
 * Input    : cb - the AVD control block
 *            su - ptr to SU
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_su_fault_sg_realign(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SG *sg = su->sg_of_su;
	AVD_SI *si = sg->admin_si;
	bool is_su_present, flag;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("SU '%s'",su->name.value);

	/* check if su is present in the su-oper list */
	m_AVD_CHK_OPLIST(su, is_su_present);

	if (!si) {
		/* => no si operation in progress */

		if (true == is_su_present) {
			/* If the fault happened while role failover is under progress,then for all the dependent SI's
			 * for which si_dep_state is set to AVD_SI_FAILOVER_UNDER_PROGRESS reset the dependency to
			 * AVD_SI_SPONSOR_UNASSIGNED and the default flow will remove the assignments
			 */
			for (curr_susi = su->list_of_susi;curr_susi != NULL;curr_susi = curr_susi->su_next) {
				if(curr_susi->si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS)
					avd_sidep_si_dep_state_set(curr_susi->si, AVD_SI_SPONSOR_UNASSIGNED);
				if (curr_susi->si->num_dependents > 0)
					avd_sidep_reset_dependents_depstate_in_sufault(curr_susi->si);
			}	

			/* => su is present in the su-oper list */
			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

			if ((SA_AMF_ADMIN_SHUTTING_DOWN == su->saAmfSUAdminState) ||
			    (SA_AMF_ADMIN_SHUTTING_DOWN == su_node_ptr->saAmfNodeAdminState)) {
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
					avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
					if (flag == true) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}

				/* identify all the quiescing assignments & send quiesced assignment */
				for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
					if (SA_AMF_HA_QUIESCING != curr_susi->state)
						continue;

					/* send quiesced assignment */
					rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
					if (rc != NCSCC_RC_SUCCESS)
						goto done;
				}	/* for */
			} else {
				/* process as su-oos evt is processed in stable state */
				rc = avd_sg_nway_su_fault_stable(cb, su);

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		} else {
			/* => su is not present in the su-oper list */

			/* process as su-oos evt is processed in stable state */
			rc = avd_sg_nway_su_fault_stable(cb, su);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		}
	} else {
		/* => si operation in progress */

		/* determine if this si has a rel with the su */
		for (curr_susi = si->list_of_sisu; curr_susi && (curr_susi->su != su); curr_susi = curr_susi->si_next) ;

		if (curr_susi) {
			/* => relationship exists */

			if (si->si_switch == AVSV_SI_TOGGLE_SWITCH) {
				if (SA_AMF_HA_QUIESCED == curr_susi->state) {
					/* si switch operation aborted */
					m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_STABLE);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

					/* identify all the active assigned assignments & send quiesced assignment */
					for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
						if (!((SA_AMF_HA_ACTIVE == curr_susi->state) &&
						      (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)))
							continue;

						/* send quiesced assignment */
						rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
						if (rc != NCSCC_RC_SUCCESS)
							goto done;
					}	/* for */

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, su, false);
				} else if ((SA_AMF_HA_ACTIVE == curr_susi->state) &&
					   (AVD_SU_SI_STATE_ASGN == curr_susi->fsm)) {
					/* identify all the active assignments & send quiesced assignment */
					for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
						if (SA_AMF_HA_ACTIVE != curr_susi->state)
							continue;

						/* send quiesced assignment */
						rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED); 
						if (rc != NCSCC_RC_SUCCESS)
							goto done;
					}	/* for */

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, su, false);
				} else if ((SA_AMF_HA_STANDBY == curr_susi->state) &&
					   (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
					/* TBD */
				}
			} else {
				if ((SA_AMF_HA_QUIESCING == curr_susi->state) ||
				    ((SA_AMF_HA_QUIESCED == curr_susi->state) &&
				     (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm))) {
					/* identify all the active assigned assignments & send quiesced assignment */
					for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
						if (!((SA_AMF_HA_ACTIVE == curr_susi->state) &&
						      (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)))
							continue;

						/* send quiesced assignment */
						rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
						if (rc != NCSCC_RC_SUCCESS)
							goto done;
					}	/* for */

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, su, false);
				} else if ((SA_AMF_HA_STANDBY == curr_susi->state) &&
					   (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
					/* process as su-oos evt is processed in stable state */
					rc = avd_sg_nway_su_fault_stable(cb, su);

					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
				}
			}
		} else {
			/* => relationship doesnt exist */

			/* process as su-oos evt is processed in stable state */
			rc = avd_sg_nway_su_fault_stable(cb, su);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		}
	}
done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_su_fault_su_oper
 *
 * Purpose  : This routine handles the su-fault event in the su-oper state.
 *
 * Input    : cb - the AVD control block
 *            su - ptr to SU
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_su_fault_su_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0;
	bool is_all_stdby = true, flag;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("SU '%s'",su->name.value);

	if (su->sg_of_su->su_oper_list.su == su) {

		m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

		/* => su-lock/shutdown is followed by su-disable event */
		if ((SA_AMF_ADMIN_SHUTTING_DOWN == su->saAmfSUAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == su_node_ptr->saAmfNodeAdminState)) {
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}

			/* identify all the quiescing assignments & send quiesced assignment */
			for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
				if (SA_AMF_HA_QUIESCING != curr_susi->state)
					continue;

				/* send quiesced assignment */
				rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
				if (rc != NCSCC_RC_SUCCESS)
					goto done;
			}	/* for */
		}
	} else {
		/* => multiple sus undergoing sg semantics */
		/* identify all the active assignments & send quiesced assignment */
		for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			if (SA_AMF_HA_ACTIVE != curr_susi->state)
				continue;

			/* send quiesced assignment */
			rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
			if (rc != NCSCC_RC_SUCCESS)
				goto done;
			is_all_stdby = false;
		}		/* for */

		/* if all standby assignments.. remove them */
		if (true == is_all_stdby) {
			rc = avd_sg_su_si_del_snd(cb, su);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				goto done;
			}
		}

		/* transition to sg-realign state */
		m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SG_REALIGN);

		/* add su to the su-oper list */
		avd_sg_su_oper_list_add(cb, su, false);
	}

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_su_fault_si_oper
 *
 * Purpose  : This routine handles the su-fault event in the si-oper state.
 *
 * Input    : cb - the AVD control block
 *            su - ptr to SU
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_su_fault_si_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SG *sg = su->sg_of_su;
	AVD_SI *si = sg->admin_si;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("SU '%s'",su->name.value);

	/* process the failure of su while there is an si transfer 
	 * going in this SG */

	if (su->sg_of_su->si_tobe_redistributed) {
		if (su->sg_of_su->min_assigned_su == su) {
			/* determine if this si has a rel with the su */
			for (curr_susi = su->sg_of_su->si_tobe_redistributed->list_of_sisu; curr_susi && 
					(curr_susi->su != su); curr_susi = curr_susi->si_next);
			if (curr_susi) {
				if ((curr_susi->state == SA_AMF_HA_ACTIVE) &&
						(curr_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {
					/* send quiesced assignment */
					rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
					if (rc != NCSCC_RC_SUCCESS)
						goto done;

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, su, false);
				}

				/* delete the quiesced assignment to max assigned su */
				curr_susi = avd_su_susi_find(cb, su->sg_of_su->max_assigned_su,
						&su->sg_of_su->si_tobe_redistributed->name);
				osafassert(curr_susi->state == SA_AMF_HA_QUIESCED);

				if (avd_susi_del_send(curr_susi) == NCSCC_RC_SUCCESS) {
					/* add the su to su-oper list and move the SG to Realign */
					avd_sg_su_oper_list_add(cb, curr_susi->su, false);
				}
				m_AVD_SET_SG_FSM(cb, curr_susi->su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
			}

		} else if (su->sg_of_su->max_assigned_su == su) {
			/* determine if this si has a rel with the max su */
			for (curr_susi = su->sg_of_su->si_tobe_redistributed->list_of_sisu; curr_susi && 
					(curr_susi->su != su); curr_susi = curr_susi->si_next);
			if (curr_susi) {
				if ((curr_susi->state == SA_AMF_HA_QUIESCED) &&
						(curr_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {

					/* add su to the su-oper list & transition to su-oper state */
					avd_sg_su_oper_list_add(cb, su, false);
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SU_OPER);
				} else {
					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
					avd_sg_su_oper_list_add(cb, su, false);
				}
			}

		} else {
			/* any other su has faulted then add to su oper and move to oper state */
			avd_sg_su_oper_list_add(cb, su, false);
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SU_OPER);
		}
		/* reset all pointers for SI transfer */
		su->sg_of_su->si_tobe_redistributed = NULL;
		su->sg_of_su->max_assigned_su = NULL;
		su->sg_of_su->min_assigned_su = NULL;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
		TRACE("SI transfer aborted '%s'", su->name.value);
		goto process_remaining;
	}

	if (!si) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (si->si_switch == AVSV_SI_TOGGLE_SWITCH) {
		/* => si-switch operation semantics in progress */

		/* determine if this si has a rel with the su */
		for (curr_susi = si->list_of_sisu; curr_susi && (curr_susi->su != su); curr_susi = curr_susi->si_next) ;

		if (curr_susi) {
			/* => relationship exists */

			if ((SA_AMF_HA_QUIESCED == curr_susi->state) && (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm)) {
				/* si switch operation aborted */
				m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* transition to su-oper state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SU_OPER);

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, su, false);
			} else if ((SA_AMF_HA_ACTIVE == curr_susi->state) && (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm)) {
				/* send quiesced assignment */
				rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
				if (rc != NCSCC_RC_SUCCESS)
					goto done;

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, su, false);
			} else if ((SA_AMF_HA_QUIESCED == curr_susi->state) &&
				   (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
				/* si switch operation aborted */
				m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, su, false);
			} else if ((SA_AMF_HA_STANDBY == curr_susi->state) && (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
				AVD_SU_SI_REL *susi = 0;

				/* si switch operation aborted */
				m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* identify the quiesced assigning susi */
				for (susi = si->list_of_sisu;
				     susi && !((SA_AMF_READINESS_IN_SERVICE == susi->su->saAmfSuReadinessState) &&
					       (SA_AMF_HA_QUIESCED == susi->state) &&
					       (AVD_SU_SI_STATE_MODIFY == susi->fsm)); susi = susi->si_next) ;

				/* send active assignment */
				if (susi) {
					rc  = avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);
					if (rc != NCSCC_RC_SUCCESS)
						goto done;

					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, susi->su, false);
				}
			}
		} else {
			/* => relationship doesnt exist */

			/* si switch operation aborted */
			m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

			/* identify the quiesced assigning susi */
			for (curr_susi = si->list_of_sisu;
			     curr_susi && !((SA_AMF_READINESS_IN_SERVICE == curr_susi->su->saAmfSuReadinessState) &&
					    (SA_AMF_HA_QUIESCED == curr_susi->state) &&
					    (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm));
			     curr_susi = curr_susi->si_next) ;

			/* send active assignment */
			if (curr_susi) {
				rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_ACTIVE);
				if (rc != NCSCC_RC_SUCCESS)
					goto done;

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, curr_susi->su, false);
			}
		}
	} else {
		/* => non si-switch operation semantics in progress */

		/* determine if this si has a rel with the su */
		for (curr_susi = si->list_of_sisu; curr_susi && (curr_susi->su != su); curr_susi = curr_susi->si_next) ;

		if (curr_susi) {
			/* => relationship exists */

			if ((SA_AMF_HA_QUIESCING == curr_susi->state) ||
			    ((SA_AMF_HA_QUIESCED == curr_susi->state) && (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm))) {
				/* si operation aborted */
				avd_si_admin_state_set(si, SA_AMF_ADMIN_UNLOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* if quiescing, send quiesced assignment */
				if (SA_AMF_HA_QUIESCING == curr_susi->state) {
					rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
					if (rc != NCSCC_RC_SUCCESS)
						goto done;
				}

				/* transition to su-oper state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SU_OPER);

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, su, false);
			} else if ((SA_AMF_HA_STANDBY == curr_susi->state) && (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
				AVD_SU_SI_REL *susi = 0;

				/* si operation aborted */
				avd_si_admin_state_set(si, SA_AMF_ADMIN_UNLOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* identify the quiesced assigning susi */
				for (susi = si->list_of_sisu;
				     susi && !((SA_AMF_READINESS_IN_SERVICE == curr_susi->su->saAmfSuReadinessState) &&
					       (SA_AMF_HA_QUIESCED == susi->state) &&
					       (AVD_SU_SI_STATE_MODIFY == susi->fsm)); susi = susi->si_next) ;

				/* send active assignment */
				if (susi) {
					rc = avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);
					if (rc != NCSCC_RC_SUCCESS)
						goto done;

					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, susi->su, false);
				}
			}
		} else {
			/* => relationship doesnt exist */
			AVD_SU_SI_REL *susi = 0;

			/* si operation aborted */
			avd_si_admin_state_set(si, SA_AMF_ADMIN_UNLOCKED);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

			/* identify the quiesced assigning susi */
			for (susi = si->list_of_sisu;
			     susi && !((SA_AMF_READINESS_IN_SERVICE == curr_susi->su->saAmfSuReadinessState) &&
				       (SA_AMF_HA_QUIESCED == susi->state) &&
				       (AVD_SU_SI_STATE_MODIFY == susi->fsm)); susi = susi->si_next) ;

			/* send active assignment */
			if (susi) {
				rc  = avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);
				if (rc != NCSCC_RC_SUCCESS)
					goto done;

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, susi->su, false);
			}
		}
	}

process_remaining:

	/* process the remaining sis as is done in stable state */
	for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
		if ((curr_susi->si == si) || (SA_AMF_HA_ACTIVE != curr_susi->state))
			continue;

		/* send quiesced assignment */
		rc  = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
	}			/* for */

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_su_fault_sg_admin
 *
 * Purpose  : This routine handles the su-fault event in the sg-admin state.
 *
 * Input    : cb - the AVD control block
 *            su - ptr to SU
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_su_fault_sg_admin(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("SU '%s'",su->name.value);

	/* if already locked, do nothing */
	if (SA_AMF_ADMIN_LOCKED == su->sg_of_su->saAmfSGAdminState)
		goto done;

	if (SA_AMF_ADMIN_SHUTTING_DOWN == su->sg_of_su->saAmfSGAdminState) {
		/* identify all the quiescing assignments & send quiesced assignment */
		for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			if (SA_AMF_HA_QUIESCING != curr_susi->state)
				continue;

			/* send quiesced assignment */
			rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_QUIESCED);
		}		/* for */
	}

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_susi_succ_sg_realign
 *
 * Purpose  : This routine handles the susi-success event in the sg-realign 
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *            susi  - ptr to susi rel
 *            act   - susi operation
 *            state - ha state
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_susi_succ_sg_realign(AVD_CL_CB *cb,
				       AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *curr_susi = 0, *curr_sisu = 0, tmp_susi;
	AVD_SG *sg = su->sg_of_su;
	bool is_su_present, is_eng, flag;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2(" action:%u state:%u",act, state);

	if (susi && (SA_AMF_HA_ACTIVE == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single active assignment success */

		if (susi->si == sg->admin_si) {
			/* si switch semantics in progress */

			/* identify the quiesced assigned assignment */
			for (curr_susi = susi->si->list_of_sisu;
			     curr_susi && !((SA_AMF_HA_QUIESCED == curr_susi->state) &&
					    (AVD_SU_SI_STATE_ASGND == curr_susi->fsm));
			     curr_susi = curr_susi->si_next) ;

			if (curr_susi) {
				/* send standby assignment */
				rc  = avd_susi_mod_send(curr_susi, SA_AMF_HA_STANDBY);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;

				/* reset the switch operation */
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
				m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

				/* add the su to su-oper list */
				avd_sg_su_oper_list_add(cb, curr_susi->su, false);
			}
		} else {
			/* check if su is present in the su-oper list */
			m_AVD_CHK_OPLIST(su, is_su_present);
			if (is_su_present) {
				/* if other susis of this SU are assigned, remove it from the su-oper list */
				for (curr_susi = su->list_of_susi;
				     curr_susi && (AVD_SU_SI_STATE_ASGND == curr_susi->fsm);
				     curr_susi = curr_susi->su_next) ;

				if (!curr_susi)
					avd_sg_su_oper_list_del(cb, su, false);
			} else {
				/* identify the quiesced susi assignment */
				for (curr_susi = susi->si->list_of_sisu;
				     curr_susi && (SA_AMF_HA_QUIESCED != curr_susi->state);
				     curr_susi = curr_susi->si_next) ;

				if (curr_susi) {
					/* the corresponding su should be in the su-oper list */
					m_AVD_CHK_OPLIST(curr_susi->su, is_su_present);
					if (is_su_present) {
						/* determine if all the standby sus are engaged */
						m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(curr_susi->su, 0, is_eng);
						if (true == is_eng) {
							/* send remove all msg for all sis for this su */
							rc = avd_sg_su_si_del_snd(cb, curr_susi->su);
							if (NCSCC_RC_SUCCESS != rc) {
								LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.
												 value,
												 curr_susi->su->name.
												 length);
								goto done;
							}

							m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

							/* if su-shutdown semantics in progress, mark it locked */
							if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
								avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
							} else if (su_node_ptr->saAmfNodeAdminState ==
								   SA_AMF_ADMIN_SHUTTING_DOWN) {
								m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
								if (flag == true) {
									node_admin_state_set(su_node_ptr,
											     SA_AMF_ADMIN_LOCKED);
								}
							}
						}
					}
				}
			}
		}

		/* if we are done with the upheaval, try assigning new sis to the sg */
		if (!sg->admin_si && !sg->su_oper_list.su)
			rc = avd_sg_nway_si_assign(cb, sg);

	} else if (susi && (SA_AMF_HA_STANDBY == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single standby assignment success */

		/* if all susis are assigned, remove the su from the su-oper list */
		for (curr_susi = su->list_of_susi;
		     curr_susi && (AVD_SU_SI_STATE_ASGND == curr_susi->fsm); curr_susi = curr_susi->su_next) ;

		if (!curr_susi)
			avd_sg_su_oper_list_del(cb, su, false);

		/* if we are done with the upheaval, try assigning new sis to the sg */
		if (!sg->admin_si && !sg->su_oper_list.su)
			rc = avd_sg_nway_si_assign(cb, sg);

	} else if (susi && (SA_AMF_HA_QUIESCED == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single quiesced assignment success */

		if (susi->si == sg->admin_si) {
			/* si switch semantics in progress */

			/* identify the other quiesced assignment */
			for (curr_susi = susi->si->list_of_sisu;
			     curr_susi && !((SA_AMF_HA_QUIESCED == curr_susi->state) &&
					    (AVD_SU_SI_STATE_ASGND == curr_susi->fsm) &&
					    (curr_susi != susi)); curr_susi = curr_susi->si_next) ;

			if (curr_susi) {
				/* send active assignment to curr_susi */
				rc  = avd_susi_mod_send(curr_susi, SA_AMF_HA_ACTIVE);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;

				/* also send remove susi msg to the susi for which the response is rcvd */
				rc = avd_susi_del_send(susi);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;

				/* reset the switch operation */
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
				m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

				/* add both sus to su-oper list */
				avd_sg_su_oper_list_add(cb, curr_susi->su, false);
				avd_sg_su_oper_list_add(cb, susi->su, false);
			}
		} else {
			/* check if su is present in the su-oper list */
			m_AVD_CHK_OPLIST(su, is_su_present);
			if (is_su_present) {
				/* identify the most preferred standby su for this si */
				curr_susi = find_pref_standby_susi(susi);
				if (curr_susi) {
					/* send active assignment */
					rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_ACTIVE);
					if (NCSCC_RC_SUCCESS != rc)
						goto done;
				} else {
					/* determine if all the standby sus are engaged */
					m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(su, 0, is_eng);
					if (true == is_eng) {
						/* send remove all msg for all sis for this su */
						rc = avd_sg_su_si_del_snd(cb, su);
						if (NCSCC_RC_SUCCESS != rc) {
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value,
											 su->name.length);
							goto done;
						}

						m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

						/* if su-shutdown semantics in progress, mark it locked */
						if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
							avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
						} else if (su_node_ptr->saAmfNodeAdminState ==
							   SA_AMF_ADMIN_SHUTTING_DOWN) {
							m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
							if (flag == true) {
								node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
							}
						}

					}
					/* As susi failover is not possible, delete all the assignments corresponding to
					 * curr_susi->si
					 */
					avd_si_assignments_delete(cb, susi->si);
					
				}
			}
		}

	} else if (susi && (AVSV_SUSI_ACT_DEL == act)) {
		/* => single remove success */

		/* store the susi info before freeing */
		tmp_susi = *susi;

		/* free all the CSI assignments  */
		avd_compcsi_delete(cb, susi, false);
		/* free susi assignment */
		m_AVD_SU_SI_TRG_DEL(cb, susi);

		if (sg->admin_si == tmp_susi.si) {
			if (SA_AMF_HA_QUIESCED == tmp_susi.state) {
				/* identify the other quiesced assigned in service susi */
				for (curr_susi = tmp_susi.si->list_of_sisu;
				     curr_susi && !((SA_AMF_HA_QUIESCED == curr_susi->state) &&
						    (AVD_SU_SI_STATE_ASGND == curr_susi->fsm) &&
						    (SA_AMF_READINESS_IN_SERVICE ==
						     curr_susi->su->saAmfSuReadinessState));
				     curr_susi = curr_susi->si_next) ;

				if (curr_susi) {
					/* send active assignment to curr_susi */
					rc  = avd_susi_mod_send(curr_susi, SA_AMF_HA_ACTIVE);
					if (NCSCC_RC_SUCCESS != rc)
						goto done;

					/* reset the switch operation */
					m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
					m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

					/* add the su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_susi->su, false);
				}
			}
		}

		/* identify if there's any active for this si */
		for (curr_susi = tmp_susi.si->list_of_sisu;
		     curr_susi && (SA_AMF_HA_ACTIVE != curr_susi->state); curr_susi = curr_susi->si_next) ;

		if (!curr_susi && (SA_AMF_ADMIN_UNLOCKED == tmp_susi.si->saAmfSIAdminState)
                               && ((tmp_susi.si->si_dep_state == AVD_SI_ASSIGNED) ||
                                (tmp_susi.si->si_dep_state == AVD_SI_NO_DEPENDENCY))) {
			/* no active present.. identify the preferred in-svc standby & 
			   send active assignment to it */
			curr_susi = find_pref_standby_susi(&tmp_susi);
			if (curr_susi) {
				/* send active assignment to curr_susi */
				rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_ACTIVE);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}

		/* if all the other assignments are assigned for this SU and their 
		   corresponding actives are assigned, remove the su from the su-oper list */
		for (curr_susi = tmp_susi.su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			if (AVD_SU_SI_STATE_ASGND != curr_susi->fsm)
				break;

			if (SA_AMF_HA_STANDBY == curr_susi->state) {
				for (curr_sisu = curr_susi->si->list_of_sisu;
				     curr_sisu && !((SA_AMF_HA_ACTIVE == curr_sisu->state) &&
						    (AVD_SU_SI_STATE_ASGND == curr_sisu->fsm));
				     curr_sisu = curr_sisu->si_next) ;
				if (!curr_sisu)
					break;
			}
		}

		if (!curr_susi)
			avd_sg_su_oper_list_del(cb, su, false);

		/* if we are done with the upheaval, try assigning new sis to the sg */
		if (!sg->admin_si && !sg->su_oper_list.su)
			rc = avd_sg_nway_si_assign(cb, sg);

	} else if (!susi && (AVSV_SUSI_ACT_DEL == act)) {
		/* => remove all success */
		/* if su-shutdown semantics in progress, mark it locked */
		m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

		if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
		} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
			if (flag == true) {
				node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
			}
		}

		/* delete the si assignments to this su */
		avd_sg_su_asgn_del_util(cb, su, true, false);

		/* remove the su from the su-oper list */
		avd_sg_su_oper_list_del(cb, su, false);

		/* if we are done with the upheaval, try assigning new sis to the sg */
		if (!sg->admin_si && !sg->su_oper_list.su)
			rc = avd_sg_nway_si_assign(cb, sg);
	}

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/**
 * @brief     Finds the most preferred Standby SISU for a particular Si to do SI failover 
 * 	     
 * @param[in] sisu 
 *
 * @return   AVD_SU_SI_REL - pointer to the preferred sisu
 *
 */
static AVD_SU_SI_REL * find_pref_standby_susi(AVD_SU_SI_REL *sisu)
{
	AVD_SU_SI_REL *curr_sisu = 0,  *curr_susi = 0;
	int curr_su_act_cnt = 0;

	TRACE_ENTER();	

	curr_sisu = sisu->si->list_of_sisu;
	while (curr_sisu) {
		if ((SA_AMF_READINESS_IN_SERVICE == curr_sisu->su->saAmfSuReadinessState) &&
			(SA_AMF_HA_STANDBY == curr_sisu->state) && (curr_sisu->su != sisu->su)) {
			/* Find the Current Active assignments on the curr_sisu->su. 
			 * We cannot depend on su->saAmfSUNumCurrActiveSIs, because for an Active assignment 
			 * saAmfSUNumCurrActiveSIs will be  updated only after completion of the assignment 
			 * process(getting response). So if there are any  assignments in the middle  of 
			 * assignment process saAmfSUNumCurrActiveSIs wont give the currect value 
			 */
			curr_susi = curr_sisu->su->list_of_susi;
			curr_su_act_cnt = 0;
			while (curr_susi) {
				if (SA_AMF_HA_ACTIVE == curr_susi->state)
					curr_su_act_cnt++;
				curr_susi = curr_susi->su_next;
			}
			if (curr_su_act_cnt < sisu->su->sg_of_su->saAmfSGMaxActiveSIsperSU) {
				TRACE("Found preferred sisu SU: '%s' SI: '%s'",curr_sisu->su->name.value,
										curr_sisu->si->name.value);
				break;
			}
		}
		curr_sisu = curr_sisu->si_next;
	}

	TRACE_LEAVE();
	return curr_sisu;
}
/*****************************************************************************
 * Function : avd_sg_nway_susi_succ_su_oper
 *
 * Purpose  : This routine handles the susi-success event in the su-oper
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *            susi  - ptr to susi rel
 *            act   - susi operation
 *            state - ha state
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_susi_succ_su_oper(AVD_CL_CB *cb,
				    AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *curr_susi = 0, *curr_sisu = 0;
	AVD_SG *sg = su->sg_of_su;
	bool is_eng = false, flag;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVD_AVND *su_node_ptr = NULL;
	TRACE_ENTER2("SU '%s'  ",su->name.value);

	if (susi && (SA_AMF_HA_QUIESCED == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single quiesced assignment success */

		/* identify the most preferred standby su for this si */
		curr_sisu = find_pref_standby_susi(susi);
		if (curr_sisu) {
			if ((su->su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED)
				|| (su->su_on_node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)) {
				rc = avd_susi_role_failover(curr_sisu, susi->su);
				if (rc == NCSCC_RC_FAILURE) {
					TRACE("Active role modification failed");
					goto done;
				}
			} else {
				rc = avd_susi_mod_send(curr_sisu, SA_AMF_HA_ACTIVE);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		} else {
			/* determine if all the standby sus are engaged */
			m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(su, 0, is_eng);
			if (true == is_eng) {
				/* send remove all msg for all sis for this su */
				rc = avd_sg_su_si_del_snd(cb, su);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s ", __FILE__, __LINE__, su->name.value);
					goto done;
				}

				m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

				/* if su-shutdown semantics in progress, mark it locked */
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
					if (flag == true) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
			/* As susi failover is not possible, delete all the assignments corresponding to
			 * curr_susi->si
			 */
			avd_si_assignments_delete(cb, susi->si);
		}
	} else if (susi && (SA_AMF_HA_ACTIVE == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single active assignment success */

		avd_susi_update(susi, state);
		/* determine if all the standby sus are engaged */
		m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(sg->su_oper_list.su, 0, is_eng);
		if (true == is_eng) {
			/* send remove all msg for all sis for this su */
			rc = avd_sg_su_si_del_snd(cb, sg->su_oper_list.su);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, sg->su_oper_list.su->name.value,
								 sg->su_oper_list.su->name.length);
				goto done;
			}

			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

			/* if su-shutdown semantics in progress, mark it locked */
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		}
	} else if (!susi && (AVSV_SUSI_ACT_DEL == act)) {
		/* => remove all success */

		/* delete the su from the oper list */
		avd_sg_su_oper_list_del(cb, su, false);

		/* scan the su-si list & send active assignments to the standbys (if any) */
		for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			/* skip standby assignments */
			if (SA_AMF_HA_STANDBY == curr_susi->state)
				continue;

			/* identify the most preferred standby su for this si */
			for (curr_sisu = curr_susi->si->list_of_sisu;
			     curr_sisu && !((SA_AMF_HA_STANDBY == curr_sisu->state) &&
					    (SA_AMF_READINESS_IN_SERVICE == curr_sisu->su->saAmfSuReadinessState));
			     curr_sisu = curr_sisu->si_next) ;

			if (curr_sisu) {
				/* send active assignment */
				rc = avd_susi_mod_send(curr_sisu, SA_AMF_HA_ACTIVE);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;

				/* add the prv standby to the su-oper list */
				avd_sg_su_oper_list_add(cb, curr_sisu->su, false);
			}
		}		/* for */

		/* free all the SI assignments to this SU */
		avd_sg_su_asgn_del_util(cb, su, true, false);

		/* transition to sg-realign state or initiate si assignments */
		if (sg->su_oper_list.su) {
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else
			avd_sg_nway_si_assign(cb, sg);
	} else if (susi && (AVSV_SUSI_ACT_DEL == act)) {
		/* Single susi delete success processing */

		/* delete the su from the oper list */
		avd_sg_su_oper_list_del(cb, su, false);

		/* free all the CSI assignments  */
                avd_compcsi_delete(cb, susi, false);

                /* free susi assignment */
                m_AVD_SU_SI_TRG_DEL(cb, susi);

		/* transition to sg-realign state or initiate si assignments */
		if (sg->su_oper_list.su) {
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else
			avd_sg_nway_si_assign(cb, sg);
	}
done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_susi_succ_si_oper
 *
 * Purpose  : This routine handles the susi-success event in the si-oper
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *            susi  - ptr to susi rel
 *            act   - susi operation
 *            state - ha state
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_susi_succ_si_oper(AVD_CL_CB *cb,
				    AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SG *sg = su->sg_of_su;
	uint32_t rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER();

	if (susi && (SA_AMF_HA_QUIESCED == state) && (AVSV_SUSI_ACT_DEL != act)) {
		if (sg->admin_si != susi->si && susi->si->sg_of_si->si_tobe_redistributed != susi->si ) {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		if ((SA_AMF_ADMIN_LOCKED == susi->si->saAmfSIAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == susi->si->saAmfSIAdminState)) {
			/* lock / shutdown semantics in progress.. 
			   remove the assignment of this si from all the sus */
			for (curr_susi = susi->si->list_of_sisu; curr_susi; curr_susi = curr_susi->si_next) {
				rc = avd_susi_del_send(curr_susi);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;

				/* add the su to su-oper list */
				avd_sg_su_oper_list_add(cb, curr_susi->su, false);
			}	/* for */

			/* reset si-admin ptr & transition to sg-realign state */
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

			/* transition to locked admin state (if shutdown) */
			if (SA_AMF_ADMIN_SHUTTING_DOWN == susi->si->saAmfSIAdminState) {
				avd_si_admin_state_set(susi->si, SA_AMF_ADMIN_LOCKED);
			}
		} else if (AVSV_SI_TOGGLE_SWITCH == susi->si->si_switch) {
			/* si switch semantics in progress.. 
			   identify the most preferred standby su & assign it active */
			for (curr_susi = susi->si->list_of_sisu;
			     curr_susi && (SA_AMF_HA_STANDBY != curr_susi->state); curr_susi = curr_susi->si_next) ;

			if (curr_susi) {
				/* send active assignment */
				rc  = avd_susi_mod_send(curr_susi, SA_AMF_HA_ACTIVE);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			} else {
				/* reset the switch operation */
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
				m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

				/* no standby.. send the prv active (now quiesced) active */
				rc  = avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;

				/* add the su to su-oper list & transition to sg-realign state */
				avd_sg_su_oper_list_add(cb, susi->su, false);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		} else if (susi->si->sg_of_si->si_tobe_redistributed == susi->si) {
			TRACE("SI transfer '%s'", susi->si->name.value);

			osafassert(susi->su == susi->si->sg_of_si->max_assigned_su);

			/* check if the min SU is still in-service */
			if (susi->si->sg_of_si->min_assigned_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
				AVD_SU_SI_REL *t_susi = NULL;
				t_susi = avd_su_susi_find(avd_cb, susi->si->sg_of_si->min_assigned_su, &susi->si->name);
				if (t_susi == NULL) {
					/* initiate new active assignment for this SI */
					rc = avd_new_assgn_susi(avd_cb, susi->si->sg_of_si->min_assigned_su,
							susi->si->sg_of_si->si_tobe_redistributed, 
							SA_AMF_HA_ACTIVE, false, &t_susi);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, t_susi->su->name.value,
								t_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, t_susi->si->name.value,
								t_susi->si->name.length);
						susi->si->sg_of_si->max_assigned_su = NULL;
						susi->si->sg_of_si->min_assigned_su = NULL;
						susi->si->sg_of_si->si_tobe_redistributed = NULL;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, susi->si->sg_of_si, AVSV_CKPT_AVD_SI_TRANS);
					}
				} else {
					/* check whether an standby assignment is existing then 
					 * send modify for this SI from standby to active 
					 */
					/* send active assignment */
					rc = avd_susi_mod_send(t_susi, SA_AMF_HA_ACTIVE);
					if (NCSCC_RC_SUCCESS != rc) {
						susi->si->sg_of_si->max_assigned_su = NULL;
						susi->si->sg_of_si->min_assigned_su = NULL;
						susi->si->sg_of_si->si_tobe_redistributed = NULL;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, susi->si->sg_of_si, AVSV_CKPT_AVD_SI_TRANS);
					}
		
				}
			} else {
				TRACE("Min assigned SU out of service '%s'",
						susi->si->sg_of_si->min_assigned_su->name.value);

				/* reset the min, max SUs and si_tobe_redistributed of the sg to NULL*/
				susi->si->sg_of_si->max_assigned_su = NULL;
				susi->si->sg_of_si->min_assigned_su = NULL;
				susi->si->sg_of_si->si_tobe_redistributed = NULL;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, susi->si->sg_of_si, AVSV_CKPT_AVD_SI_TRANS);

				rc = avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);
				if (NCSCC_RC_SUCCESS == rc) {
					/* add the su to su-oper list & transition to sg-realign state */
					avd_sg_su_oper_list_add(avd_cb, susi->su, false);
					m_AVD_SET_SG_FSM(avd_cb, susi->si->sg_of_si, AVD_SG_FSM_SG_REALIGN);
				}
			}
		}
	} else if (susi && (SA_AMF_HA_ACTIVE == state) && (AVSV_SUSI_ACT_DEL != act)) {
		if ((sg->admin_si == susi->si) && (AVSV_SI_TOGGLE_SWITCH == susi->si->si_switch)) {
			/* reset the switch operation */
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
			m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

			/* identify the prv active su (now quiesced) */
			for (curr_susi = susi->si->list_of_sisu;
					curr_susi && (SA_AMF_HA_QUIESCED != curr_susi->state); curr_susi = curr_susi->si_next) ;

			if (curr_susi) {
				/* send standby assignment */
				rc = avd_susi_mod_send(curr_susi, SA_AMF_HA_STANDBY);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;

				/* add the su to su-oper list & transition to sg-realign state */
				avd_sg_su_oper_list_add(cb, curr_susi->su, false);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
			}
		} else if (susi->si->sg_of_si->si_tobe_redistributed == susi->si) { 
			AVD_SU_SI_REL *t_susi = NULL;
			/* si transfer is in progress for equal distribution 
			 * now get the susi between si tobe redistributed and max SU 
			 * should be quiesced state, now send a remove for that susi
			 */
			osafassert(susi->su == susi->si->sg_of_si->min_assigned_su);

			/* find the susi with max assigned su which should be by now quiesced */
			t_susi = avd_su_susi_find(avd_cb, susi->si->sg_of_si->max_assigned_su, &susi->si->name);
			osafassert(t_susi->state == SA_AMF_HA_QUIESCED);

			rc = avd_susi_del_send(t_susi);
			if (NCSCC_RC_SUCCESS == rc) {
				avd_sg_su_oper_list_add(avd_cb, t_susi->su, false);
				m_AVD_SET_SG_FSM(avd_cb, t_susi->su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
			}
			t_susi->si->sg_of_si->max_assigned_su = NULL;
			t_susi->si->sg_of_si->min_assigned_su = NULL;
			t_susi->si->sg_of_si->si_tobe_redistributed = NULL;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, t_susi->si->sg_of_si, AVSV_CKPT_AVD_SI_TRANS);
			TRACE ("SI transfer completed '%s'", t_susi->si->name.value);
		}
	       	else {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
			rc = NCSCC_RC_FAILURE;
		}
	}

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_susi_succ_sg_admin
 *
 * Purpose  : This routine handles the susi-success event in the sg-admin
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *            susi  - ptr to susi rel
 *            act   - susi operation
 *            state - ha state
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uint32_t avd_sg_nway_susi_succ_sg_admin(AVD_CL_CB *cb,
				     AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SG *sg = su->sg_of_su;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	if (susi && (SA_AMF_HA_QUIESCED == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single quiesced success */
		if ((SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == sg->saAmfSGAdminState)) {
			/* determine if the su has any quiesced assigning assignment */
			for (curr_susi = su->list_of_susi;
			     curr_susi
			     &&
			     !(((SA_AMF_HA_QUIESCED == curr_susi->state) || (SA_AMF_HA_QUIESCING == curr_susi->state))
			       && (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm)); curr_susi = curr_susi->su_next) ;

			if (!curr_susi) {
				/* => now remove all can be sent */
				rc = avd_sg_su_si_del_snd(cb, su);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					goto done;
				}
			}
		}
	} else if (!susi && (AVSV_SUSI_ACT_DEL == act)) {
		/* => delete all success */
		if ((SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == sg->saAmfSGAdminState)) {
			/* free all the SI assignments to this SU */
			avd_sg_su_asgn_del_util(cb, su, true, false);

			/* delete the su from the oper list */
			avd_sg_su_oper_list_del(cb, su, false);

			/* if oper list is empty, transition the sg back to stable state */
			if (!sg->su_oper_list.su) {
				avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_STABLE);
				/*As sg is stable, screen for si dependencies and take action on whole sg*/
				avd_sidep_update_si_dep_state_for_all_sis(sg);
				avd_sidep_sg_take_action(sg); 
			}
		}
	}

done:
	TRACE_LEAVE2(" return value: %d",rc);
	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_node_fail_stable
 *
 * Purpose  : This routine handles the node failure event in the stable
 *            state.
 *
 * Input    : cb   - the AvD control block
 *            su   - ptr to su
 *            susi - ptr to susi rel that is to be skipped
 *        
 * Returns  : None.
 *
 * Notes    : None.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_stable(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi)
{
	AVD_SU_SI_REL *curr_susi = 0, *curr_sisu = 0;
	AVD_SG *sg = su->sg_of_su;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s', %u", su->name.value, su->sg_of_su->sg_fsm_state);

	if (!su->list_of_susi) {
		TRACE("No assignments on this su");
		goto done;
	}

	/* engage the active susis with their standbys */
	for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
		if (curr_susi == susi)
			continue;

		if ((SA_AMF_HA_ACTIVE == curr_susi->state) ||
		    (SA_AMF_HA_QUIESCED == curr_susi->state) || (SA_AMF_HA_QUIESCING == curr_susi->state)) {
			/* identify the most preferred standby su for this si */
			curr_sisu = find_pref_standby_susi(curr_susi);
			if (curr_sisu) {
				rc = avd_susi_role_failover(curr_sisu, curr_susi->su);
				if (rc == NCSCC_RC_FAILURE) {
					TRACE("Active role modification failed");
					goto done;
				}

				/* add su to su-oper list */
				avd_sg_su_oper_list_add(cb, curr_sisu->su, false);
			} else {
				/* As susi failover is not possible, delete all the assignments 
				   corresponding to curr_susi->si except on failed node.
				 */
				for (curr_sisu = curr_susi->si->list_of_sisu ;
					curr_sisu != NULL; curr_sisu = curr_sisu->si_next) {
					if (curr_sisu != curr_susi) {
						rc = avd_susi_del_send(curr_sisu);
						if (NCSCC_RC_SUCCESS == rc)
							avd_sg_su_oper_list_add(cb, curr_sisu->su, false);
					}
				}
                        }
		}

		/* transition to sg-realign state */
		if (sg->su_oper_list.su) {
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		}
	}			/* for */

	/* delete the si assignments to this su */
	avd_sg_su_asgn_del_util(cb, su, true, false);

	/* if sg is still in stable state, initiate si assignments */
	if (AVD_SG_FSM_STABLE == sg->sg_fsm_state) {
		avd_sg_nway_si_assign(cb, sg);
		/* if equal ranked su then screen for redistribution
		 * of the SIs if the SG fsm state is still STABLE */
		if ((su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				(su->sg_of_su->equal_ranked_su == true) &&
				(su->sg_of_su->saAmfSGAutoAdjust == SA_TRUE))
			avd_sg_nway_screen_si_distr_equal(su->sg_of_su);
		if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE)
			avd_sidep_sg_take_action(su->sg_of_su);
	}

done:
	TRACE_LEAVE();
	return;
}

/*****************************************************************************
 * Function : avd_sg_nway_node_fail_su_oper
 *
 * Purpose  : This routine handles the node failure event in the su-oper
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *        
 * Returns  : None.
 *
 * Notes    : None.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_su_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0, *curr_sisu = 0;
	AVD_SG *sg = su->sg_of_su;
	bool is_su_present, flag;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("SU '%s'",su->name.value);

	/* check if su is present in the su-oper list */
	m_AVD_CHK_OPLIST(su, is_su_present);

	if (is_su_present) {
		m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

		if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
		} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
			if (flag == true) {
				node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
			}
		}

		for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			if (((SA_AMF_HA_QUIESCING == curr_susi->state) ||
			     ((SA_AMF_HA_QUIESCED == curr_susi->state) &&
			      (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm))) ||
			    (AVD_SU_SI_STATE_UNASGN == curr_susi->fsm)) {

				curr_sisu = find_pref_standby_susi(curr_susi);
				/* send active assignment */
				if (curr_sisu) {
					rc = avd_susi_role_failover(curr_sisu, curr_susi->su);
					if (rc == NCSCC_RC_FAILURE) {
						TRACE("Active role modification failed");
						goto done;
					}
					
					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, false);

					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
				}
			} else if ((SA_AMF_HA_QUIESCED == curr_susi->state) &&
				   (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
				/* already quiesced assigned.. idenify the active assigning & 
				   add it to the su-oper list */
				for (curr_sisu = curr_susi->si->list_of_sisu;
				     curr_sisu && !((SA_AMF_HA_ACTIVE == curr_sisu->state) &&
						    (AVD_SU_SI_STATE_ASGN == curr_sisu->fsm) &&
						    (SA_AMF_READINESS_IN_SERVICE ==
						     curr_sisu->su->saAmfSuReadinessState));
				     curr_sisu = curr_sisu->si_next) ;

				/* add su to su-oper list */
				if (curr_sisu)
					avd_sg_su_oper_list_add(cb, curr_sisu->su, false);

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		}		/* for */

		/* remove this su from su-oper list */
		avd_sg_su_oper_list_del(cb, su, false);

		/* if sg hasnt transitioned to sg-realign, initiate new si assignments */
		if (AVD_SG_FSM_SG_REALIGN != sg->sg_fsm_state)
			avd_sg_nway_si_assign(cb, sg);
	} else {
		/* engage the active susis with their standbys */
		for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			if (SA_AMF_HA_ACTIVE == curr_susi->state) {
				curr_sisu = find_pref_standby_susi(curr_susi);
				/* send active assignment */
				if (curr_sisu) {
					rc = avd_susi_role_failover(curr_sisu, curr_susi->su);
					if (rc == NCSCC_RC_FAILURE) {
						TRACE("Active role modification failed");
						goto done;
					}

					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, false);

					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
				}	/* if (curr_sisu) */
			}
		}		/* for */
	}

	/* delete the si assignments to this su */
	avd_sg_su_asgn_del_util(cb, su, true, false);

done:
	TRACE_LEAVE();
	return;
}

/*****************************************************************************
 * Function : avd_sg_nway_node_fail_si_oper
 *
 * Purpose  : This routine handles the node failure event in the si-oper
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *        
 * Returns  : None.
 *
 * Notes    : None.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_si_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_sisu = 0, *susi = 0;
	AVD_SG *sg = su->sg_of_su;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("SU '%s'",su->name.value);

	/* process the failure of su while there is an si transfer 
	 * going in this SG */

	if (su->sg_of_su->si_tobe_redistributed) {
		if (su->sg_of_su->min_assigned_su == su) {
			/* find whether the min_assigned su is already
			 * in the process of taking active assignment */	
			susi = avd_su_susi_find(cb, sg->min_assigned_su,
					&sg->si_tobe_redistributed->name);

			if (susi != NULL && susi->state == SA_AMF_HA_ACTIVE) {
				/* identify the quiesced assigning assignment
				 * for the si_tobe_redistributed with max_su */

				curr_sisu = avd_su_susi_find(cb, sg->max_assigned_su,
						&sg->si_tobe_redistributed->name);
				/* if already ACTIVE assignment under process then
				 * remove the quiesced assignment */
				if (curr_sisu) {
					if (avd_susi_del_send(curr_sisu) == NCSCC_RC_FAILURE)
						goto done;
					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, false);
				}
			} else {
				/* identify the quiesced assignment of si_tobe_redistributed
				 * to max su and assign back the active assignment */
				 
				curr_sisu = avd_su_susi_find(cb, sg->max_assigned_su,
						&sg->si_tobe_redistributed->name);
				if (curr_sisu) {
					rc = avd_susi_mod_send(curr_sisu, SA_AMF_HA_ACTIVE);
					if (NCSCC_RC_SUCCESS != rc)
						goto done;
					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, false);
				}
			}

			/* process the susis assigned to this su as in stable state */
			avd_sg_nway_node_fail_stable(cb, su, 0);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else {
			/* process the susis assigned to this su as in stable state */
			avd_sg_nway_node_fail_stable(cb, su, 0);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		}
		/* reset the pointer for SI transfer */
		su->sg_of_su->si_tobe_redistributed = NULL;
		su->sg_of_su->max_assigned_su = NULL;
		su->sg_of_su->min_assigned_su = NULL;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
		TRACE("SI transfer aborted '%s'", su->name.value);
	}

	if (!sg->admin_si) {
		goto done;
	}

	/* check if admin-si has a relationship with this su */
	for (susi = sg->admin_si->list_of_sisu; susi && (susi->su != su); susi = susi->si_next) ;

	if ((SA_AMF_ADMIN_LOCKED == sg->admin_si->saAmfSIAdminState) ||
	    (SA_AMF_ADMIN_SHUTTING_DOWN == sg->admin_si->saAmfSIAdminState)) {
		if (!susi) {
			/* process as in stable state */
			avd_sg_nway_node_fail_stable(cb, su, 0);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else {
			/* abort the si lock / shutdown operation */
			avd_si_admin_state_set(susi->si, SA_AMF_ADMIN_UNLOCKED);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

			if (((SA_AMF_HA_QUIESCED == susi->state) ||
			     (SA_AMF_HA_QUIESCING == susi->state)) && (AVD_SU_SI_STATE_MODIFY == susi->fsm)) {
				/* process as in stable state */
				avd_sg_nway_node_fail_stable(cb, su, 0);

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			} else if ((SA_AMF_HA_STANDBY == susi->state) && (AVD_SU_SI_STATE_ASGND == susi->fsm)) {
				/* identify in-svc quiesced/quiescing assignment & 
				   send an active assignment */
				for (curr_sisu = susi->si->list_of_sisu;
				     curr_sisu
				     && !((SA_AMF_READINESS_IN_SERVICE == curr_sisu->su->saAmfSuReadinessState)
					  && ((SA_AMF_HA_QUIESCED == curr_sisu->state)
					      || (SA_AMF_HA_QUIESCING == curr_sisu->state)));
				     curr_sisu = curr_sisu->si_next) ;

				/* send active assignment */
				if (curr_sisu) {
					rc = avd_susi_role_failover(curr_sisu, susi->su);
					if (rc == NCSCC_RC_FAILURE) {
						TRACE("Active role modification failed");
						goto done;
					}

					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, false);
				}

				/* process the rest susis as in stable state */
				avd_sg_nway_node_fail_stable(cb, su, susi);

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		}
	} else if (AVSV_SI_TOGGLE_SWITCH == sg->admin_si->si_switch) {
		if (!susi) {
			/* identify the quiesced assigning assignment for the admin si ptr */
			for (curr_sisu = sg->admin_si->list_of_sisu;
			     curr_sisu && !((SA_AMF_HA_QUIESCED == curr_sisu->state) &&
					    (AVD_SU_SI_STATE_MODIFY == curr_sisu->fsm));
			     curr_sisu = curr_sisu->si_next) ;

			/* send active assignment */
			if (curr_sisu) {
				rc = avd_susi_mod_send(curr_sisu, SA_AMF_HA_ACTIVE);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;

				/* add su to su-oper list */
				avd_sg_su_oper_list_add(cb, curr_sisu->su, false);

				/* si switch operation aborted */
				m_AVD_SET_SI_SWITCH(cb, sg->admin_si, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
			}

			/* process the susis assigned to this su as in stable state */
			avd_sg_nway_node_fail_stable(cb, su, 0);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else {
			if (((SA_AMF_HA_QUIESCED == susi->state) && (AVD_SU_SI_STATE_MODIFY == susi->fsm)) ||
			    ((SA_AMF_HA_ACTIVE == susi->state) && (AVD_SU_SI_STATE_ASGN == susi->fsm)) ||
			    ((SA_AMF_HA_QUIESCED == susi->state) && (AVD_SU_SI_STATE_ASGND == susi->fsm)) ||
			    ((SA_AMF_HA_STANDBY == susi->state) && (AVD_SU_SI_STATE_ASGND == susi->fsm))
			    ) {
				/* si switch operation aborted */
				m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* identify the susi that has to be assigned active */
				for (curr_sisu = susi->si->list_of_sisu; curr_sisu; curr_sisu = curr_sisu->si_next) {
					if ((SA_AMF_HA_QUIESCED == susi->state)
					    && (AVD_SU_SI_STATE_MODIFY == susi->fsm)) {
						if ((curr_sisu != susi)
						    && ((SA_AMF_HA_QUIESCED == curr_sisu->state)
							|| (SA_AMF_HA_STANDBY == curr_sisu->state)))
							break;
					}

					if ((SA_AMF_HA_ACTIVE == susi->state) && (AVD_SU_SI_STATE_ASGN == susi->fsm))
						if (SA_AMF_HA_QUIESCED == curr_sisu->state)
							break;

					if ((SA_AMF_HA_QUIESCED == susi->state) && (AVD_SU_SI_STATE_ASGND == susi->fsm))
						if (SA_AMF_HA_ACTIVE == curr_sisu->state)
							break;

					if ((SA_AMF_HA_STANDBY == susi->state) && (AVD_SU_SI_STATE_ASGND == susi->fsm))
						if (SA_AMF_HA_QUIESCED == curr_sisu->state)
							break;
				}	/* for */

				/* send active assignment */
				if (curr_sisu) {
					if (!((SA_AMF_HA_QUIESCED == susi->state)
					      && (AVD_SU_SI_STATE_ASGND == susi->fsm))) {
						rc = avd_susi_mod_send(curr_sisu, SA_AMF_HA_ACTIVE);
						if (NCSCC_RC_SUCCESS != rc)
							goto done;
					}

					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, false);
				}

				/* process the rest susis as in stable state */
				avd_sg_nway_node_fail_stable(cb, su, susi);

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		}
	}

done:
	TRACE_LEAVE();
	return;
}

/*****************************************************************************
 * Function : avd_sg_nway_node_fail_sg_admin
 *
 * Purpose  : This routine handles the node failure event in the sg-admin
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *        
 * Returns  : None.
 *
 * Notes    : None.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_sg_admin(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SG *sg = su->sg_of_su;

	TRACE_ENTER2("SU '%s'",su->name.value);

	/* delete the si assignments to this su */
	avd_sg_su_asgn_del_util(cb, su, true, false);

	/* delete the su from the su-oper list */
	avd_sg_su_oper_list_del(cb, su, false);

	if (!sg->su_oper_list.su) {
		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);
		m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_STABLE);
		/*As sg is stable, screen for si dependencies and take action on whole sg*/
		avd_sidep_update_si_dep_state_for_all_sis(sg);
		avd_sidep_sg_take_action(sg); 
	}
	
	TRACE_LEAVE();
	return;
}

/*****************************************************************************
 * Function : avd_sg_nway_node_fail_sg_realign
 *
 * Purpose  : This routine handles the node failure event in the sg-realign
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *        
 * Returns  : None.
 *
 * Notes    : None.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_sg_realign(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SG *sg = su->sg_of_su;
	bool is_su_present, flag;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("SU '%s'",su->name.value);

	if (sg->admin_si) {
		/* process as in si-oper state */
		avd_sg_nway_node_fail_si_oper(cb, su);
	} else {
		/* => si operation isnt in progress */

		/* check if su is present in the su-oper list */
		m_AVD_CHK_OPLIST(su, is_su_present);

		if (true == is_su_present) {
			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == true) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}
		}

		/* process as in stable state */
		avd_sg_nway_node_fail_stable(cb, su, 0);

		/* delete the su from the su-oper list */
		avd_sg_su_oper_list_del(cb, su, false);

		/* if we are done with the upheaval, try assigning new sis to the sg */
		if (!sg->su_oper_list.su)
			avd_sg_nway_si_assign(cb, sg);
	}

	TRACE_LEAVE();
	return;
}

/**
 * Initialize redundancy model specific handlers
 * @param sg
 */
void avd_sg_nway_init(AVD_SG *sg)
{
	sg->node_fail = avd_sg_nway_node_fail_func;
	sg->realign = avd_sg_nway_realign_func;
	sg->si_func = avd_sg_nway_si_func;
	sg->si_admin_down = avd_sg_nway_si_admin_down;
	sg->sg_admin_down = avd_sg_nway_sg_admin_down;
	sg->su_insvc = avd_sg_nway_su_insvc_func;
	sg->su_fault = avd_sg_nway_su_fault_func;
	sg->su_admin_down = avd_sg_nway_su_admin_fail;
	sg->susi_success = avd_sg_nway_susi_sucss_func;
	sg->susi_failed = avd_sg_nway_susi_fail_func;
}
