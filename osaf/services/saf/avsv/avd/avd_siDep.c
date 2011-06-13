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
 *            Ericsson
 *
 */

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION: 
  
  This module deals with the creation, accessing and deletion of the SI-SI
  dependency database.

..............................................................................

  FUNCTIONS INCLUDED in this module:

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <saImmOm.h>
#include <immutil.h>
#include <logtrace.h>

#include <avd_imm.h>
#include <avd_si_dep.h>
#include <avd_susi.h>
#include <avd_proc.h>

/* static function prototypes */
static uns32 avd_check_si_dep_sponsors(AVD_CL_CB *cb, AVD_SI *si, NCS_BOOL take_action);
static void avd_update_si_dep_state_for_spons_unassign(AVD_CL_CB *cb, AVD_SI *dep_si, AVD_SI_SI_DEP *si_dep_rec);
static uns32 avd_si_dep_si_unassigned(AVD_CL_CB *cb, AVD_SI *si);
static uns32 avd_check_si_state_enabled(AVD_CL_CB *cb, AVD_SI *si);
static uns32 avd_sg_red_si_process_assignment(AVD_CL_CB *cb, AVD_SI *si);
static uns32 avd_si_dep_state_evt(AVD_CL_CB *cb, AVD_SI *si, AVD_SI_SI_DEP_INDX *si_dep_idx);
static void avd_si_dep_spons_state_modif(AVD_CL_CB *cb, AVD_SI *si, AVD_SI *si_dep,
					 AVD_SI_DEP_SPONSOR_SI_STATE spons_state);
static uns32 avd_si_si_dep_cyclic_dep_find(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx);
static void avd_si_dep_start_unassign(AVD_CL_CB *cb, AVD_EVT *evt);

static AVD_SI_DEP si_dep; /* SI-SI dependency data */

/*****************************************************************************
 * Function: avd_check_si_state_enabled 
 *
 * Purpose:  This function check the active and the quiescing HA states for
 *           SI of all service units.
 *
 * Input:   cb - Pointer to AVD ctrl-block
 *          si - Pointer to AVD_SI struct
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 **************************************************************************/
uns32 avd_check_si_state_enabled(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *susi = NULL;
	uns32 rc = NCSCC_RC_FAILURE;
	TRACE_ENTER2("%s",si->name.value);
	susi = si->list_of_sisu;
	while (susi != AVD_SU_SI_REL_NULL) {
		if ((susi->state == SA_AMF_HA_ACTIVE) || (susi->state == SA_AMF_HA_QUIESCING)) {
			rc = NCSCC_RC_SUCCESS;
			break;
		}

		susi = susi->si_next;
	}

	if (rc == NCSCC_RC_SUCCESS) {
		if (si->si_dep_state == AVD_SI_NO_DEPENDENCY) {
			if (si->tol_timer_count == 0) {
				m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_ASSIGNED);
			} else {
				m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_TOL_TIMER_RUNNING);
			}
		}
	} else
		m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_SPONSOR_UNASSIGNED);

	return rc;
}

/*****************************************************************************
 * Function: avd_si_dep_spons_list_del
 *
 * Purpose:  This function deletes the spons-SI node from the spons-list of 
 *           dependent-SI.
 *
 * Input:  cb - ptr to AVD control block
 *         si_dep_rec - ptr to AVD_SI_SI_DEP struct 
 *
 * Returns:
 *
 * NOTES:
 * 
 **************************************************************************/
void avd_si_dep_spons_list_del(AVD_CL_CB *cb, AVD_SI_SI_DEP *si_dep_rec)
{
	AVD_SI *dep_si = NULL;
	AVD_SPONS_SI_NODE *spons_si_node = NULL;
	AVD_SPONS_SI_NODE *del_spons_si_node = NULL;

	TRACE_ENTER();

	/* Dependent SI row Status should be active, if not return error */
	if ((dep_si = avd_si_get(&si_dep_rec->indx_imm.si_name_sec))
	    == AVD_SI_NULL) {
		/* LOG the message */
		return;
	}

	/* SI doesn't depend on any other SIs */
	if (dep_si->spons_si_list == NULL) {
		/* LOG the message */
		return;
	}

	spons_si_node = dep_si->spons_si_list;

	/* Check if the sponsor is the first node of the list */
	if (m_CMP_HORDER_SANAMET(dep_si->spons_si_list->si->name, si_dep_rec->indx_imm.si_name_prim) == 0) {
		dep_si->spons_si_list = spons_si_node->next;
		/* decrement the dependent SI count in sponsor SI */
		spons_si_node->si->num_dependents --;
		free(spons_si_node);
	} else {
		while (spons_si_node->next != NULL) {
			if (m_CMP_HORDER_SANAMET(spons_si_node->next->si->name,
						 si_dep_rec->indx_imm.si_name_prim) != 0) {
				spons_si_node = spons_si_node->next;
				continue;
			}

			/* Matched spons si node found, then delete */
			del_spons_si_node = spons_si_node->next;
			spons_si_node->next = spons_si_node->next->next;
			del_spons_si_node->si->num_dependents --;
			free(del_spons_si_node);
			break;
		}
	}

	return;
}

/*****************************************************************************
 * Function: avd_si_dep_spons_list_add
 *
 * Purpose:  This function adds the spons-SI node in the spons-list of 
 *           dependent-SI.
 *
 * Input:  cb - ptr to AVD control block
 *         dep_si - ptr to AVD_SI struct (dependent-SI node).
 *         spons_si - ptr to AVD_SI struct (sponsor-SI node).
 *
 * Returns:
 *
 * NOTES: 
 * 
 **************************************************************************/
uns32 avd_si_dep_spons_list_add(AVD_CL_CB *avd_cb, AVD_SI *dep_si, AVD_SI *spons_si,
		AVD_SI_SI_DEP *rec)
{
	AVD_SPONS_SI_NODE *spons_si_node;

	TRACE_ENTER();

	spons_si_node = calloc(1, sizeof(AVD_SPONS_SI_NODE));
	if (spons_si_node == NULL) {
		TRACE("calloc failed");
		return NCSCC_RC_FAILURE;
	}

	/* increment number of dependents in sponsor SI as well */
	spons_si->num_dependents ++;

	spons_si_node->si = spons_si;
	spons_si_node->sidep_rec = rec;

	spons_si_node->next = dep_si->spons_si_list;
	dep_si->spons_si_list = spons_si_node;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_si_dep_stop_tol_timer
 *
 * Purpose:  This function is been called when SI was unassigned, checks 
 *           whether tolerance timers are running for this SI (because
 *           of its sponsor-SI unassignment), if yes just stop them.  
 *
 * Input:  cb - ptr to AVD control block
 *         si - ptr to AVD_SI struct.
 *
 * Returns:
 *
 * NOTES: 
 * 
 **************************************************************************/
void avd_si_dep_stop_tol_timer(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SI_SI_DEP_INDX indx;
	AVD_SI_SI_DEP *rec = NULL;
	AVD_SPONS_SI_NODE *spons_si_node = si->spons_si_list;

	TRACE_ENTER();

	memset((char *)&indx, '\0', sizeof(AVD_SI_SI_DEP_INDX));
	indx.si_name_sec.length = si->name.length;
	memcpy(indx.si_name_sec.value, si->name.value, si->name.length);

	while (spons_si_node) {
		/* Need to stop the tolerance timer */
		indx.si_name_prim.length = spons_si_node->si->name.length;
		memcpy(indx.si_name_prim.value, spons_si_node->si->name.value,
		       spons_si_node->si->name.length);

		if ((rec = avd_si_si_dep_find(cb, &indx, TRUE)) != NULL) {
			if (rec->si_dep_timer.is_active == TRUE) {
				avd_stop_tmr(cb, &rec->si_dep_timer);

				if (si->tol_timer_count > 0) {
					si->tol_timer_count--;
				}
			}
		}

		spons_si_node = spons_si_node->next;
	}

	return;
}

/*****************************************************************************
 * Function: avd_si_dep_si_unassigned
 *
 * Purpose:  This function removes the active and the quiescing HA states for
 *           SI from all service units and moves SI dependency state to
 *           SPONSOR_UNASSIGNED state.  
 *
 * Input: cb - The AVD control block
 *        si - Pointer to AVD_SI struct
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 **************************************************************************/
uns32 avd_si_dep_si_unassigned(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *susi = NULL;
	uns32 rc = NCSCC_RC_FAILURE;
	AVD_SU_SI_STATE old_fsm_state;

	susi = si->list_of_sisu;
	while (susi != AVD_SU_SI_REL_NULL) {
		old_fsm_state = susi->fsm;
		susi->fsm = AVD_SU_SI_STATE_UNASGN;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);

		rc = avd_snd_susi_msg(cb, susi->su, susi, AVSV_SUSI_ACT_DEL);
		if (NCSCC_RC_SUCCESS != rc) {
			/* LOG the erro */
			susi->fsm = old_fsm_state;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
			return rc;
		}

		/* add the su to su-oper list */
		avd_sg_su_oper_list_add(cb, susi->su, FALSE);

		susi = susi->si_next;
	}

	m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_SPONSOR_UNASSIGNED);

	/* transition to sg-realign state */
	m_AVD_SET_SG_FSM(cb, si->sg_of_si, AVD_SG_FSM_SG_REALIGN);

	/* Check if this SI is a sponsor SI for some other SI, the take appropriate action */
	avd_si_dep_spons_state_modif(cb, si, NULL, AVD_SI_DEP_SPONSOR_UNASSIGNED);

	return rc;
}

/*****************************************************************************
 * Function: avd_sg_red_si_process_assignment 
 *
 * Purpose:  This function process SI assignment process on a redundancy model
 *           that was associated with the SI.
 *
 * Input: cb - The AVD control block
 *        si - Pointer to AVD_SI struct
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_INV_VAL
 *
 * NOTES:
 *
 **************************************************************************/
uns32 avd_sg_red_si_process_assignment(AVD_CL_CB *cb, AVD_SI *si)
{
	if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
		return NCSCC_RC_FAILURE;
	}

	if ((si->max_num_csi == si->num_csi) &&
	    (si->saAmfSIAdminState == SA_AMF_ADMIN_UNLOCKED) && (cb->init_state == AVD_APP_STATE)) {
		switch (si->sg_of_si->sg_redundancy_model) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			if (avd_sg_2n_si_func(cb, si) != NCSCC_RC_SUCCESS) {
				return NCSCC_RC_FAILURE;
			}
			break;

		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
			if (avd_sg_nacvred_si_func(cb, si) != NCSCC_RC_SUCCESS) {
				return NCSCC_RC_FAILURE;
			}
			break;

		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
			if (avd_sg_nway_si_func(cb, si) != NCSCC_RC_SUCCESS) {
				return NCSCC_RC_FAILURE;
			}
			break;

		case SA_AMF_NPM_REDUNDANCY_MODEL:
			if (avd_sg_npm_si_func(cb, si) != NCSCC_RC_SUCCESS) {
				return NCSCC_RC_FAILURE;
			}
			break;

		case SA_AMF_NO_REDUNDANCY_MODEL:
		default:
			if (avd_sg_nored_si_func(cb, si) != NCSCC_RC_SUCCESS) {
				return NCSCC_RC_FAILURE;
			}
			break;
		}

		if (avd_check_si_state_enabled(cb, si) == NCSCC_RC_SUCCESS) {
			m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_ASSIGNED);
			return NCSCC_RC_SUCCESS;
		}
	}

	return NCSCC_RC_FAILURE;
}

/*****************************************************************************
 * Function: avd_si_dep_state_evt
 *
 * Purpose:  This function prepares the event to send AVD_EVT_SI_DEP_STATE
 *           event. This event is sent on expiry of tolerance timer or the 
 *           sponsor SI was unassigned and tolerance timer is "0".
 *
 * Input:  cb - ptr to AVD control block
 *         si - ptr to AVD_SI struct.
 *
 * Returns: 
 *
 * NOTES:
 * 
 **************************************************************************/
uns32 avd_si_dep_state_evt(AVD_CL_CB *cb, AVD_SI *si, AVD_SI_SI_DEP_INDX *si_dep_idx)
{
	AVD_EVT *evt = NULL;

	TRACE_ENTER();

	evt = calloc(1, sizeof(AVD_EVT));
	if (evt == NULL) {
		TRACE("calloc failed");
		return NCSCC_RC_FAILURE;
	}

	/* Update evt struct, using tmr field even though this field is not
	 * relevant for this event, but it accommodates the required data.
	 */
	evt->rcv_evt = AVD_EVT_SI_DEP_STATE;

	/* si_dep_idx is NULL for ASSIGN event non-NULL for UNASSIGN event */
	if (si_dep_idx != NULL) {
		evt->info.tmr.spons_si_name = si_dep_idx->si_name_prim;
		evt->info.tmr.dep_si_name = si_dep_idx->si_name_sec;
		m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_READY_TO_UNASSIGN);
	} else {		/* For ASSIGN evt, just enough to feed SI name */

		evt->info.tmr.spons_si_name.length = si->name.length;
		memcpy(evt->info.tmr.spons_si_name.value, si->name.value, si->name.length);
	}

	TRACE("%u", evt->rcv_evt);

	if (m_NCS_IPC_SEND(&cb->avd_mbx, evt, NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: ipc send %u failed", __FUNCTION__, evt->rcv_evt);
		free(evt);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_tmr_si_dep_tol_evh
 *
 * Purpose:  On expiry of tolerance timer in SI-SI dependency context, this
 *           function initiates the process of SI unassignment due to its 
 *           sponsor moved to unassigned state.
 *
 * Input:  cb - ptr to AVD control block
 *         evt - ptr to AVD_EVT struct.
 *
 * Returns: 
 *
 * NOTES:
 * 
 **************************************************************************/
void avd_tmr_si_dep_tol_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_SI *si = NULL;
	AVD_SI *spons_si = NULL;

	TRACE_ENTER();

	assert(evt->info.tmr.type == AVD_TMR_SI_DEP_TOL);

	si = avd_si_get(&evt->info.tmr.dep_si_name);
	spons_si = avd_si_get(&evt->info.tmr.spons_si_name);

	if ((si == NULL) || (spons_si == NULL)) {
		/* Nothing to do here as SI/spons-SI itself lost their existence */
		TRACE("Sponsor SI or Dependent SI not exists");
		goto done;
	}

	TRACE("expiry of tolerance timer '%s'", si->name.value);

	/* Since the tol-timer is been expired, can decrement tol_timer_count of 
	 * the SI. 
	 */
	if (si->tol_timer_count > 0)
		si->tol_timer_count--;

	/* Before starting unassignment process of SI, check once again whether 
	 * sponsor SIs are assigned back,if so move the SI state to ASSIGNED state 
	 */
	avd_screen_sponsor_si_state(cb, si, FALSE);

	switch (si->si_dep_state) {
	case AVD_SI_NO_DEPENDENCY:
		m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_SPONSOR_UNASSIGNED);
	case AVD_SI_SPONSOR_UNASSIGNED:
		break;
	case AVD_SI_ASSIGNED:
	case AVD_SI_UNASSIGNING_DUE_TO_DEP:
		/* LOG the ERROR message. Before moving to this state, need to ensure 
		 * all the tolerance timers of this SI are in stopped state.
		 */
		goto done;

	default:
		break;
	}

	/* If the SI is already been unassigned, nothing to proceed for 
	 * unassignment 
	 */
	if (si->list_of_sisu == AVD_SU_SI_REL_NULL) {
		m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_SPONSOR_UNASSIGNED);
		goto done;
	}

	/* Check if spons_si SI-Dep state is not in ASSIGNED state, then 
	 * only initiate the unassignment process. Because the SI can be 
	 * in this state due to some other spons-SI.
	 */
	if (avd_check_si_state_enabled(cb, spons_si) != NCSCC_RC_SUCCESS) {
		m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_UNASSIGNING_DUE_TO_DEP);

		if (avd_si_dep_si_unassigned(cb, si) != NCSCC_RC_SUCCESS) {
			/* Log the error */
			TRACE("'%s' Dep SI unassignment failed", si->name.value);
		}
	}
done:
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_check_si_dep_sponsors
 *
 * Purpose:  This function checks whether sponsor SIs of an SI are in enabled /
 *           disabled state.  
 *
 * Input:    cb - Pointer to AVD control block
 *           si - Pointer to AVD_SI struct 
 *           take_action - If TRUE, process the impacts (SI-SI dependencies)
 *                      on dependent-SIs (of sponsor-SI being enabled/disabled)
 *
 * Returns: NCSCC_RC_SUCCESS - if sponsor-SI is in enabled state 
 *          NCSCC_RC_FAILURE - if sponsor-SI is in disabled state 
 *
 * NOTES:  
 * 
 **************************************************************************/
uns32 avd_check_si_dep_sponsors(AVD_CL_CB *cb, AVD_SI *si, NCS_BOOL take_action)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_SPONS_SI_NODE *spons_si_node = si->spons_si_list;
	TRACE_ENTER2("%s", si->name.value);
	while (spons_si_node) {
		if (avd_check_si_state_enabled(cb, spons_si_node->si) != NCSCC_RC_SUCCESS) {
			rc = NCSCC_RC_FAILURE;
			if (take_action) {
				avd_si_dep_spons_state_modif(cb, spons_si_node->si, si, AVD_SI_DEP_SPONSOR_UNASSIGNED);
			} else {
				/* Sponsors are not in enabled state */
				break;
			}
		}

		spons_si_node = spons_si_node->next;
	}

	/* All of the sponsors are in enabled state */
	if ((rc == NCSCC_RC_SUCCESS) && (take_action)) {
		m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_ASSIGNED);

		avd_si_dep_spons_state_modif(cb, si, NULL, AVD_SI_DEP_SPONSOR_ASSIGNED);
	}

	return rc;
}

/*****************************************************************************
 * Function: avd_sg_screen_si_si_dependencies
 *
 * Purpose:  This function screens SI-SI dependencies of the SG SIs. 
 *
 * Input: cb - The AVD control block
 *        sg - Pointer to AVD_SG struct 
 *
 * Returns: 
 *
 * NOTES:  
 * 
 **************************************************************************/
void avd_sg_screen_si_si_dependencies(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SI *si = NULL;

	si = sg->list_of_si;
	while (si != AVD_SI_NULL) {
		if (avd_check_si_state_enabled(cb, si) == NCSCC_RC_SUCCESS) {
			/* SI was in enabled state, so check for the SI-SI dependencies
			 * conditions and update the SI accordingly.
			 */
			avd_check_si_dep_sponsors(cb, si, TRUE);
		} else {
			avd_screen_sponsor_si_state(cb, si, TRUE);
			if ((si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED) ||
			    (si->si_dep_state == AVD_SI_NO_DEPENDENCY)) {
				/* Check if this SI is a sponsor SI for some other SI, the take appropriate action */
				avd_si_dep_spons_state_modif(cb, si, NULL, AVD_SI_DEP_SPONSOR_UNASSIGNED);
			}
		}

		si = si->sg_list_of_si_next;
	}

	return;
}

/*****************************************************************************
 * Function: avd_screen_sponsor_si_state 
 *
 * Purpose:  This function checks whether the sponsor SIs are in ASSIGNED 
 *           state. If they are in assigned state, dependent SI changes its
 *           si_dep state accordingly.
 *
 * Input: cb - The AVD control block
 *        si - Pointer to AVD_SI struct 
 *
 * Returns: 
 *
 * NOTES:  
 * 
 **************************************************************************/
void avd_screen_sponsor_si_state(AVD_CL_CB *cb, AVD_SI *si, NCS_BOOL start_assignment)
{
	TRACE_ENTER2("%s", si->name.value);

	/* Change the SI dependency state only when all of its sponsor SIs are 
	 * in assigned state.
	 */
	if (avd_check_si_dep_sponsors(cb, si, FALSE) != NCSCC_RC_SUCCESS) {
		/* Nothing to do here, just return */
		return;
	}

	switch (si->si_dep_state) {
	case AVD_SI_TOL_TIMER_RUNNING:
	case AVD_SI_READY_TO_UNASSIGN:
		if (si->tol_timer_count == 0)
			m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_ASSIGNED);
		break;

	case AVD_SI_SPONSOR_UNASSIGNED:
		/* If SI-SI dependency cfg's are removed for this SI the update the 
		 * SI dep state with AVD_SI_NO_DEPENDENCY 
		 */
		m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_NO_DEPENDENCY);

	case AVD_SI_NO_DEPENDENCY:
		/* Initiate the process of ASSIGNMENT state, as all of its sponsor SIs
		 * are in ASSIGNED state, should be done only when SI is been unassigned
		 * due to SI-SI dependency (sponsor unassigned).
		 */
		if (start_assignment) {
			if (avd_sg_red_si_process_assignment(cb, si) == NCSCC_RC_SUCCESS) {
				/* Check if this SI is a sponsor SI for some other SI, the take appropriate action */
				avd_si_dep_spons_state_modif(cb, si, NULL, AVD_SI_DEP_SPONSOR_ASSIGNED);
			} else {
				/* LOG the error message */
				TRACE("SI assignment failed");
			}
		}
		break;

	default:
		return;
	}

	return;
}

/*****************************************************************************
 * Function: avd_process_si_dep_state_evh
 *
 * Purpose:  This function starts SI unassignment process due to sponsor was  
 *           unassigned (SI-SI dependency logics).
 *
 * Input: cb - The AVD control block
 *        evt - Pointer to AVD_EVT struct 
 *
 * Returns: 
 *
 * NOTES:
 * 
 **************************************************************************/
void avd_process_si_dep_state_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_SI *si = NULL;

	TRACE_ENTER();

	if (evt == NULL) {
		/* Log the message */
		return;
	}

	/* Check whether rcv_evt is AVD_EVT_SI_DEP_STATE event, if not LOG the 
	 * error message.
	 */
	if (evt->rcv_evt != AVD_EVT_SI_DEP_STATE) {
		/* internal error */
		assert(0);
		return;
	}

	if (evt->info.tmr.dep_si_name.length == 0) {
		/* Its an ASSIGN evt */
		si = avd_si_get(&evt->info.tmr.spons_si_name);
		if (si != NULL) {
			avd_screen_sponsor_si_state(cb, si, TRUE);
		}
	} else {		/* Process UNASSIGN evt */

		avd_si_dep_start_unassign(cb, evt);
	}

	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_si_dep_start_unassign
 *
 * Purpose:  This function starts SI unassignment process due to sponsor was  
 *           unassigned (SI-SI dependency logics).
 *
 * Input: cb - The AVD control block
 *        evt - Pointer to AVD_EVT struct 
 *
 * Returns: 
 *
 * NOTES:
 * 
 **************************************************************************/
void avd_si_dep_start_unassign(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_SI *si = NULL;
	AVD_SI *spons_si = NULL;

	TRACE_ENTER();

	si = avd_si_get(&evt->info.tmr.dep_si_name);
	spons_si = avd_si_get(&evt->info.tmr.spons_si_name);

	if ((!si) || (!spons_si)) {
		/* Log the ERROR message */
		return;
	}

	if (si->si_dep_state != AVD_SI_READY_TO_UNASSIGN) {
		/* Log the message */
		return;
	}

	/* Before starting unassignment process of SI, check again whether all its
	 * sponsors are moved back to assigned state, if so it is not required to
	 * initiate the unassignment process for the (dependent) SI.
	 */
	avd_screen_sponsor_si_state(cb, si, FALSE);
	if (si->si_dep_state == AVD_SI_ASSIGNED)
		return;

	/* If the SI is already been unassigned, nothing to proceed for 
	 * unassignment 
	 */
	if (si->list_of_sisu == AVD_SU_SI_REL_NULL) {
		m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_SPONSOR_UNASSIGNED);
		return;
	}

	/* Check if spons_si SI-Dep state is not in ASSIGNED state, then 
	 * only initiate the unassignment process. Because the SI can be 
	 * in this state due to some other spons-SI.
	 */
	if (avd_check_si_state_enabled(cb, spons_si) != NCSCC_RC_SUCCESS) {
		m_AVD_SET_SI_DEP_STATE(cb, si, AVD_SI_UNASSIGNING_DUE_TO_DEP);

		if (avd_si_dep_si_unassigned(cb, si) != NCSCC_RC_SUCCESS) {
			/* Log the error */
		}
	}

	return;
}

/*****************************************************************************
 * Function: avd_update_si_dep_state_for_spons_unassign 
 *
 * Purpose:  Upon sponsor SI is unassigned, this function updates the SI  
 *           dependent states either to AVD_SI_READY_TO_UNASSIGN / 
 *           AVD_SI_TOL_TIMER_RUNNING states.
 *
 * Input:  cb - ptr to AVD control block
 *         dep_si - ptr to AVD_SI struct (dependent SI).
 *         si_dep_rec - ptr to AVD_SI_SI_DEP struct 
 *
 * Returns: 
 *
 * NOTES:
 * 
 **************************************************************************/
void avd_update_si_dep_state_for_spons_unassign(AVD_CL_CB *cb, AVD_SI *dep_si, AVD_SI_SI_DEP *si_dep_rec)
{
	TRACE_ENTER();

	switch (dep_si->si_dep_state) {
	case AVD_SI_ASSIGNED:
		if (si_dep_rec->saAmfToleranceTime > 0) {
			/* Start the tolerance timer */
			m_AVD_SET_SI_DEP_STATE(cb, dep_si, AVD_SI_TOL_TIMER_RUNNING);

			/* Start the tolerance timer */
			m_AVD_SI_DEP_TOL_TMR_START(cb, si_dep_rec);

			if (dep_si->tol_timer_count) {
				/* It suppose to be "0", LOG the err msg and continue. */
			}

			dep_si->tol_timer_count = 1;
		} else {
			/* Send an event to start SI unassignment process */
			avd_si_dep_state_evt(cb, dep_si, &si_dep_rec->indx_imm);
		}
		break;

	case AVD_SI_TOL_TIMER_RUNNING:
	case AVD_SI_READY_TO_UNASSIGN:
		if (si_dep_rec->saAmfToleranceTime > 0) {
			if (si_dep_rec->si_dep_timer.is_active != TRUE) {
				/* Start the tolerance timer */
				m_AVD_SI_DEP_TOL_TMR_START(cb, si_dep_rec);

				/* SI is already in AVD_SI_TOL_TIMER_RUNNING state, so just 
				 * increment tol_timer_count indicates that >1 sponsors are 
				 * in unassigned state for this SI.
				 */
				dep_si->tol_timer_count++;
			} else {
				/* Should not happen, LOG the error message & go ahead */
			}
		} else {
			if (!si_dep_rec->unassign_event) {
				si_dep_rec->unassign_event = TRUE;

				/* Send an event to start SI unassignment process */
				avd_si_dep_state_evt(cb, dep_si, &si_dep_rec->indx_imm);
			}
		}
		break;

	case AVD_SI_NO_DEPENDENCY:
		m_AVD_SET_SI_DEP_STATE(cb, dep_si, AVD_SI_SPONSOR_UNASSIGNED);
		break;

	default:
		break;
	}

	return;
}

/*****************************************************************************
 * Function: avd_si_dep_spons_state_modif 
 *
 * Purpose:  Upon SI is assigned/unassigned and if this SI turns out to be 
 *           sponsor SI for some of the SIs (dependents),then update the states
 *           of dependent SIs accordingly (either to AVD_SI_READY_TO_UNASSIGN / 
 *           AVD_SI_TOL_TIMER_RUNNING states).
 *
 * Input:  cb - ptr to AVD control block
 *         si - ptr to AVD_SI struct (sponsor SI).
 *         si_dep - ptr to AVD_SI struct (dependent SI), NULL for all
 *                  dependent SIs of the above sponsor SI.
 *
 * Returns: 
 *
 * NOTES:
 * 
 **************************************************************************/
void avd_si_dep_spons_state_modif(AVD_CL_CB *cb, AVD_SI *si, AVD_SI *si_dep, AVD_SI_DEP_SPONSOR_SI_STATE spons_state)
{
	AVD_SI *dep_si = NULL;
	AVD_SI_SI_DEP_INDX si_indx;
	AVD_SI_SI_DEP *si_dep_rec = NULL;

	TRACE_ENTER();

	if (si_dep != NULL)
	{
		/* Check spons-SI & dep-SI belongs to the same SG */
		if (m_CMP_NORDER_SANAMET(si->sg_of_si->name,
					si_dep->sg_of_si->name) == 0)
		{
			/* If the SG is not in STABLE state, then not required to do state 
			 * change for the dep-SI as it can be taken care as part of SG 
			 * screening when SG becomes STABLE.
			 */
			if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
				TRACE("SG not in STABLE state");
				return;
			}
		}
	}

	memset((char *)&si_indx, '\0', sizeof(AVD_SI_SI_DEP_INDX));
	si_indx.si_name_prim.length = si->name.length;
	memcpy(si_indx.si_name_prim.value, si->name.value, si_indx.si_name_prim.length);

	/* If si_dep is NULL, means adjust the SI dep states for all depended 
	 * SIs of the sponsor SI.
	 */
	if (si_dep == NULL) {
		si_dep_rec = avd_si_si_dep_find_next(cb, &si_indx, TRUE);
		while (si_dep_rec != NULL) {
			if (m_CMP_HORDER_SANAMET(si_dep_rec->indx_imm.si_name_prim, si_indx.si_name_prim) != 0) {
				/* Seems no more node exists in spons_anchor tree with 
				 * "si_indx.si_name_prim" as primary key 
				 */
				break;
			}

			dep_si = avd_si_get(&si_dep_rec->indx_imm.si_name_sec);
			if (dep_si == NULL) {
				/* No corresponding SI node?? some thing wrong */
				si_dep_rec = avd_si_si_dep_find_next(cb, &si_dep_rec->indx_imm, TRUE);
				continue;
			}

			if (spons_state == AVD_SI_DEP_SPONSOR_UNASSIGNED) {
				avd_update_si_dep_state_for_spons_unassign(cb, dep_si, si_dep_rec);
			} else {
				avd_si_dep_state_evt(cb, dep_si, NULL);
			}

			si_dep_rec = avd_si_si_dep_find_next(cb, &si_dep_rec->indx_imm, TRUE);
		}
	} else {
		/* Just ignore and return if spons_state is AVD_SI_DEP_SPONSOR_ASSIGNED */
		if (spons_state == AVD_SI_DEP_SPONSOR_ASSIGNED)
			return;

		/* Frame the index completely to the associated si_dep_rec */
		si_indx.si_name_sec.length = si_dep->name.length;
		memcpy(si_indx.si_name_sec.value, si_dep->name.value, si_dep->name.length);

		si_dep_rec = avd_si_si_dep_find(cb, &si_indx, TRUE);

		if (si_dep_rec != NULL) {
			/* si_dep_rec primary key should match with sponsor SI name */
			if (m_CMP_HORDER_SANAMET(si_dep_rec->indx_imm.si_name_prim, si_indx.si_name_prim) != 0) {
				assert(0);
				return;
			}
			avd_update_si_dep_state_for_spons_unassign(cb, si_dep, si_dep_rec);
		}
	}

	return;
}

/*****************************************************************************
 * Function: avd_si_si_dep_struc_crt
 *
 * Purpose: This function will create and add a AVD_SI_SI_DEP structure to the
 *          trees if an element with the same key value doesn't exist in the
 *          tree.
 *
 * Input: cb - the AVD control block
 *        indx - Index for the row to be added. 
 *
 * Returns: The pointer to AVD_SI_SI_DEP structure allocated and added.
 *
 * NOTES:
 *
 **************************************************************************/
AVD_SI_SI_DEP *avd_si_si_dep_struc_crt(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx)
{
	AVD_SI_SI_DEP *rec = NULL;
	uns32 si_prim_len = indx->si_name_prim.length;
	uns32 si_sec_len = indx->si_name_sec.length;

	TRACE_ENTER();

	/* Allocate a new block structure for imm rec now */
	if ((rec = calloc(1, sizeof(AVD_SI_SI_DEP))) == NULL) {
		LOG_ER("%s: calloc failed", __FUNCTION__);
		return NULL;
	}

	rec->indx_imm.si_name_prim.length = indx->si_name_prim.length;
	memcpy(rec->indx_imm.si_name_prim.value, indx->si_name_prim.value, si_prim_len);

	rec->indx_imm.si_name_sec.length = indx->si_name_sec.length;
	memcpy(rec->indx_imm.si_name_sec.value, indx->si_name_sec.value, si_sec_len);

	rec->indx.si_name_prim.length = indx->si_name_sec.length;
	memcpy(rec->indx.si_name_prim.value, indx->si_name_sec.value, si_sec_len);

	rec->indx.si_name_sec.length = indx->si_name_prim.length;
	memcpy(rec->indx.si_name_sec.value, indx->si_name_prim.value, si_prim_len);

	rec->tree_node_imm.key_info = (uns8 *)&(rec->indx_imm);
	rec->tree_node_imm.bit = 0;
	rec->tree_node_imm.left = NCS_PATRICIA_NODE_NULL;
	rec->tree_node_imm.right = NCS_PATRICIA_NODE_NULL;

	rec->tree_node.key_info = (uns8 *)&(rec->indx);
	rec->tree_node.bit = 0;
	rec->tree_node.left = NCS_PATRICIA_NODE_NULL;
	rec->tree_node.right = NCS_PATRICIA_NODE_NULL;

	if (ncs_patricia_tree_add(&si_dep.spons_anchor, &rec->tree_node_imm) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: spons ncs_patricia_tree_add failed", __FUNCTION__);
		free(rec);
		return NULL;
	}

	if (ncs_patricia_tree_add(&si_dep.dep_anchor, &rec->tree_node) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: dep ncs_patricia_tree_add failed", __FUNCTION__);
		ncs_patricia_tree_del(&si_dep.spons_anchor, &rec->tree_node_imm);
		free(rec);
		return NULL;
	}

	return rec;
}

/*****************************************************************************
 * Function: avd_si_si_dep_find 
 *
 * Purpose:  This function will find a AVD_SI_SI_DEP structure in the tree 
 *           with indx value as key. Indices can be provided as per the order
 *           mention in the imm or in the reverse of that. 
 *
 * Input: cb - The AVD control block
 *        indx - The key.
 *
 * Returns: The pointer to AVD_SG_SI_RANK structure found in the tree.
 *
 * NOTES: Set the isImmIdx flag value to 1 if indices are as defined by imm and 0
 *        if it is in reverse order
 * 
 **************************************************************************/
AVD_SI_SI_DEP *avd_si_si_dep_find(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx, NCS_BOOL isImmIdx)
{
	AVD_SI_SI_DEP *rec = NULL;

	TRACE_ENTER();

	if (isImmIdx) {
		rec = (AVD_SI_SI_DEP *)ncs_patricia_tree_get(&si_dep.spons_anchor, (uns8 *)indx);
	} else {
		rec = (AVD_SI_SI_DEP *)ncs_patricia_tree_get(&si_dep.dep_anchor, (uns8 *)indx);
		if (rec != NULL) {
			/* Adjust the pointer */
			rec = (AVD_SI_SI_DEP *)(((char *)rec)
						- (((char *)&(AVD_SI_SI_DEP_NULL->tree_node))
						   - ((char *)AVD_SI_SI_DEP_NULL)));
		}
	}

	return rec;
}

/*****************************************************************************
 * Function: avd_si_si_dep_find_next 
 *
 * Purpose:  This function will find next AVD_SI_SI_DEP structure in the tree
 *           with indx value as key. Indices can be provided as per the order
 *           mention in the imm or in the reverse of that. 
 *
 * Input: cb - the AVD control block
 *        indx - The key.
 *
 * Returns: The next pointer to AVD_SG_SI_RANK structure found in the tree.
 *
 * NOTES: Set the isImmIdx flag value to 1 if indices are as defined by imm and 0
 *        if it is in reverse order
 * 
 **************************************************************************/
AVD_SI_SI_DEP *avd_si_si_dep_find_next(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx, NCS_BOOL isImmIdx)
{
	AVD_SI_SI_DEP *rec = NULL;

	TRACE_ENTER();

	if (isImmIdx) {
		rec = (AVD_SI_SI_DEP *)ncs_patricia_tree_getnext(&si_dep.spons_anchor, (uns8 *)indx);
	} else {
		rec = (AVD_SI_SI_DEP *)ncs_patricia_tree_getnext(&si_dep.dep_anchor, (uns8 *)indx);
		if (rec != NULL) {
			/* Adjust the pointer */
			rec = (AVD_SI_SI_DEP *)(((char *)rec)
						- (((char *)&(AVD_SI_SI_DEP_NULL->tree_node))
						   - ((char *)AVD_SI_SI_DEP_NULL)));
		}
	}

	return rec;
}

/*****************************************************************************
 * Function: avd_si_si_dep_del_row 
 *
 * Purpose:  This function will delete and free AVD_SI_SI_DEP structure from 
 *           the tree. It will delete the record from both patricia trees
 *
 * Input: cb - The AVD control block
 *        si - Pointer to service instance row 
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE  
 *
 * NOTES:
 * 
 **************************************************************************/
uns32 avd_si_si_dep_del_row(AVD_CL_CB *cb, AVD_SI_SI_DEP *rec)
{
	AVD_SI_SI_DEP *si_dep_rec = NULL;

	TRACE_ENTER();

	if (rec == NULL)
		return NCSCC_RC_FAILURE;

	if ((si_dep_rec = avd_si_si_dep_find(cb, &rec->indx, FALSE)) != NULL) {
		if (ncs_patricia_tree_del(&si_dep.dep_anchor, &si_dep_rec->tree_node)
		    != NCSCC_RC_SUCCESS) {
			LOG_ER("Failed deleting SI Dep from Dependent Anchor");
			return NCSCC_RC_FAILURE;
		}
	}

	si_dep_rec = NULL;

	if ((si_dep_rec = avd_si_si_dep_find(cb, &rec->indx_imm, TRUE)) != NULL) {
		if (ncs_patricia_tree_del(&si_dep.spons_anchor, &si_dep_rec->tree_node_imm)
		    != NCSCC_RC_SUCCESS) {
			LOG_ER("Failed deleting SI Dep from Sponsor Anchor");
			return NCSCC_RC_FAILURE;
		}
	}

	if (si_dep_rec)
		free(si_dep_rec);

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_si_si_dep_cyclic_dep_find 
 *
 * Purpose:  This function will use to evalute that is new record will because
 *           of cyclic dependency exist in SIs or not.
 *
 * Input: cb   - The AVD control block
 *        indx - Index of the new rec needs to be evaluated.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: Return value NCSCC_RC_SUCCESS means it found a cyclic dependency or
 *        buffer is not sufficient to process this request.
 *
 **************************************************************************/
uns32 avd_si_si_dep_cyclic_dep_find(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx)
{
	AVD_SI_SI_DEP *rec = NULL;
	AVD_SI_DEP_NAME_LIST *start = NULL;
	AVD_SI_DEP_NAME_LIST *temp = NULL;
	AVD_SI_DEP_NAME_LIST *last = NULL;
	uns32 rc = NCSCC_RC_FAILURE;
	AVD_SI_SI_DEP_INDX idx;

	TRACE_ENTER();

	if (m_CMP_HORDER_SANAMET(indx->si_name_prim, indx->si_name_sec) == 0) {
		/* dependent SI and Sponsor SI can not be same 
		   Cyclic dependency found return sucess
		 */
		return NCSCC_RC_SUCCESS;
	}

	if ((start = malloc(sizeof(AVD_SI_DEP_NAME_LIST))) == NULL) {
		/*Insufficient memory, record can not be added */
		return NCSCC_RC_SUCCESS;
	} else {
		start->si_name = indx->si_name_prim;
		start->next = NULL;
		last = start;
	}

	while (last) {
		memset((char *)&idx, '\0', sizeof(AVD_SI_SI_DEP_INDX));

		idx.si_name_prim.length = last->si_name.length;
		memcpy(idx.si_name_prim.value, last->si_name.value, last->si_name.length);

		rec = avd_si_si_dep_find_next(cb, &idx, FALSE);

		while ((rec != NULL) && (m_CMP_HORDER_SANAMET(rec->indx.si_name_prim, idx.si_name_prim) == 0)) {
			if (m_CMP_HORDER_SANAMET(indx->si_name_sec, rec->indx.si_name_sec) == 0) {
				/* Cyclic dependency found */
				rc = NCSCC_RC_SUCCESS;
				break;
			}

			/* Search if this SI name already exist in the list */
			temp = start;
			if (m_CMP_HORDER_SANAMET(temp->si_name, rec->indx.si_name_sec) != 0) {
				while ((temp->next != NULL) &&
				       (m_CMP_HORDER_SANAMET(temp->next->si_name, rec->indx.si_name_sec) != 0)) {
					temp = temp->next;
				}

				/* SI Name not found in the list, add it */
				if (temp->next == NULL) {
					if ((temp->next = malloc(sizeof(AVD_SI_DEP_NAME_LIST))) == NULL) {
						/* Insufficient memory space */
						rc = NCSCC_RC_SUCCESS;
						break;
					}

					temp->next->si_name = rec->indx.si_name_sec;
					temp->next->next = NULL;
				}
			}

			rec = avd_si_si_dep_find_next(cb, &rec->indx, FALSE);
		}

		if (rc == NCSCC_RC_SUCCESS) {
			break;
		} else {
			last = last->next;
		}
	}

	/* Free the allocated SI name list */
	while (start) {
		temp = start->next;
		free(start);
		start = temp;
	}

	return rc;
}

static int avd_sidep_indx_init(const SaNameT *sidep_name, AVD_SI_SI_DEP_INDX *indx)
{
	char *p;
	SaNameT tmp;
	uns32 i = 0;

	memset(&tmp, 0, sizeof(SaNameT));
	tmp.length = sidep_name->length;
	memcpy(&tmp.value, &sidep_name->value, tmp.length);
		
	memset(indx, 0, sizeof(AVD_SI_SI_DEP_INDX));

	/* find first occurence and step past it */
	p = strstr((char *)tmp.value, "safSi=") + 1;
	if (p == NULL) return FALSE;

	/* find second occurence, an error if not found */
	p = strstr(p, "safSi=");
	if (p == NULL) return FALSE;

	*(p - 1) = '\0';	/* null terminate at comma before SI */

	indx->si_name_sec.length = snprintf((char *)indx->si_name_sec.value, SA_MAX_NAME_LENGTH, "%s", p);

	/* Skip past the RDN tag */
	p = strchr((char *)tmp.value, '=') + 1;
	if (p == NULL) return FALSE;

	/*
	 ** Example DN, need to copy to get rid of back slash escaped commas.
	 ** 'safDepend=safSi=SC2-NoRed\,safApp=OpenSAF,safSi=SC-2N,safApp=OpenSAF'
	 */

	/* Copy the RDN value which is a DN with escaped commas */
	while (*p) {
		if (*p != '\\')
			indx->si_name_prim.value[i++] = *p;
		p++;
	}
	indx->si_name_prim.length = strlen((char *)indx->si_name_prim.value);

	return TRUE;
}

static uint32_t is_config_valid(SaNameT *sidep_name, CcbUtilOperationData_t *opdata)
{
	AVD_SI_SI_DEP_INDX indx;
	AVD_SI *spons_si, *dep_si;
	CcbUtilOperationData_t *tmp;
	uint32_t dep_saAmfSIRank, spons_saAmfSIRank;
	uint32_t rc = false;
	bool dependent_si_exist_in_curr_model = false;
	bool spons_si_exist_in_curr_model = false;

	TRACE_ENTER();
	
	if( !avd_sidep_indx_init(sidep_name, &indx)) {
		LOG_ER("SI dep validation: Bad DN for SI Dependency");
		goto done;
	}

	/* Sponsor SI need to exist */
	if ((spons_si = avd_si_get(&indx.si_name_prim)) == NULL) {
		if (opdata == NULL) {
			LOG_ER("SI dep validation: '%s' does not exist in model and '%s' depends on it",
				indx.si_name_prim.value, indx.si_name_sec.value);
			goto done;
		}

		/* SI does not exist in current model, check CCB */
		if ((tmp = ccbutil_getCcbOpDataByDN(opdata->ccbId, &indx.si_name_prim)) == NULL) {
			LOG_ER("SI dep validation: '%s' does not exist in existing model or in CCB and '%s' depends on it",
				indx.si_name_prim.value, indx.si_name_sec.value);
			goto done;
		}

		if (immutil_getAttr("saAmfSIRank", tmp->param.create.attrValues, 0, &spons_saAmfSIRank) != SA_AIS_OK)
			spons_saAmfSIRank = 0; /* default value is zero (lowest rank) if empty */
	} else {
		/* sponsor SI exist in current model, get rank */
		spons_si_exist_in_curr_model = true;
		spons_saAmfSIRank = spons_si->saAmfSIRank;
	}

	if (spons_saAmfSIRank == 0)
		spons_saAmfSIRank = -1; /* zero means lowest possible rank */

	if ((dep_si = avd_si_get(&indx.si_name_sec)) == NULL) {
		if (opdata == NULL) {
			LOG_ER("SI dep validation: '%s' does not exist in model", indx.si_name_sec.value);
			goto done;
		}

		/* SI does not exist in current model, check CCB */
		if ((tmp = ccbutil_getCcbOpDataByDN(opdata->ccbId, &indx.si_name_sec)) == NULL) {
			LOG_ER("SI dep validation: '%s' does not exist in existing model or in CCB",
				indx.si_name_sec.value);
			goto done;
		}

		if (immutil_getAttr("saAmfSIRank", tmp->param.create.attrValues, 0, &dep_saAmfSIRank) != SA_AIS_OK)
			dep_saAmfSIRank = 0; /* default value is zero (lowest rank) if empty */
	} else {
		/* dependent SI exist in current model, get rank */
		dependent_si_exist_in_curr_model = true;
		dep_saAmfSIRank = dep_si->saAmfSIRank;
	}

	/* don't allow to dynamically create a dependency from an SI already in the model */
	if ((opdata != NULL) && dependent_si_exist_in_curr_model) {
		LOG_ER("SI dep validation: adding dependency from existing SI '%s' to SI '%s' is not allowed",
			indx.si_name_prim.value, indx.si_name_sec.value);
		goto done;
	}

	if (dep_saAmfSIRank == 0)
		dep_saAmfSIRank = -1; /* zero means lowest possible rank */

	/* higher number => lower rank, see 3.8.1.1 */
	if (spons_saAmfSIRank > dep_saAmfSIRank) {
		LOG_ER("SI dep validation: Sponsor SI '%s' has lower rank than dependent SI '%s'",
			indx.si_name_prim.value, indx.si_name_sec.value);
		goto done;
	}

	if (avd_si_si_dep_cyclic_dep_find(avd_cb, &indx) == NCSCC_RC_SUCCESS) {
		/* Return value that record cannot be added due to cyclic dependency */
		LOG_ER("SI dep validation: cyclic dependency for '%s'", indx.si_name_sec.value);
		goto done;
	}

	rc = true;

done:
	TRACE_LEAVE();
	return rc;
} 
 
static AVD_SI_SI_DEP *sidep_new(SaNameT *sidep_name, const SaImmAttrValuesT_2 **attributes)
{
	AVD_SI_SI_DEP *sidep = NULL;
	AVD_SI_SI_DEP_INDX indx;
	AVD_SI *dep_si = NULL;
	AVD_SI *spons_si = NULL;

	TRACE_ENTER2("%s", sidep_name->value);

	avd_sidep_indx_init(sidep_name, &indx);

	if ((sidep = avd_si_si_dep_struc_crt(avd_cb, &indx)) == NULL) {
		LOG_ER("Unable create SI-SI dependency record");
		goto done;
	}
	
	if (immutil_getAttr("saAmfToleranceTime", attributes, 0, &sidep->saAmfToleranceTime) != SA_AIS_OK) {
		/* Empty, assign default value */
		sidep->saAmfToleranceTime = 0;
	}

	/* Add to dependent's sponsors list */
	assert(spons_si = avd_si_get(&indx.si_name_prim));
	assert(dep_si = avd_si_get(&indx.si_name_sec));
	assert(avd_si_dep_spons_list_add(avd_cb, dep_si, spons_si, sidep) == NCSCC_RC_SUCCESS);

	/* Move the dependent SI to appropriate state, if the configured 
	 * sponsor is not in assigned state. Not required to check with
	 * the remaining states of SI Dep states, as they are kind of 
	 * intermittent states towards SPONSOR_UNASSIGNED/ASSIGNED states. 
	 */
	if ((avd_cb->init_state  == AVD_APP_STATE) &&
		((spons_si->si_dep_state == AVD_SI_NO_DEPENDENCY) ||
		 (spons_si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED)))
	{
		avd_si_dep_spons_state_modif(avd_cb, spons_si, dep_si,
				AVD_SI_DEP_SPONSOR_UNASSIGNED);
	}
done:
	return sidep;
}

/**
 * Get configuration for all AMF SI dependency objects from IMM
 * and create AVD internal objects.
 * 
 * @return int
 */
SaAisErrorT avd_sidep_config_get(void)
{
	SaAisErrorT error;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT sidep_name;
	SaNameT sidep_validate;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSIDependency";

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, NULL, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		NULL, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("%s: saImmOmSearchInitialize failed: %u", __FUNCTION__, error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &sidep_name, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		/* validate the configuration before adding the object */
		memcpy(&sidep_validate, &sidep_name, sizeof(SaNameT));
		if (!is_config_valid(&sidep_name, NULL)) {
			error = SA_AIS_ERR_BAD_OPERATION;
			goto done2;
		}
		if(sidep_new(&sidep_name, attributes) == NULL) {
			error = SA_AIS_ERR_BAD_OPERATION;
			goto done2;
		}
	}

	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:

	return error;
}

static SaAisErrorT sidep_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	AVD_SI_SI_DEP_INDX indx;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (!is_config_valid(&opdata->objectName, opdata))
			rc = SA_AIS_ERR_BAD_OPERATION;
		break;
	case CCBUTIL_DELETE:
	case CCBUTIL_MODIFY:
		avd_sidep_indx_init(&opdata->objectName, &indx);
		if (NULL == avd_si_si_dep_find(avd_cb, &indx, TRUE))
			rc = SA_AIS_ERR_BAD_OPERATION;
		break;
	default:
		assert(0);
		break;
	}

	return rc;
}

static void sidep_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_SI_SI_DEP *sidep;
	AVD_SI_SI_DEP_INDX indx;
	AVD_SI *dep_si = NULL;

	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		sidep = sidep_new(&opdata->objectName, opdata->param.create.attrValues);
		break;

	case CCBUTIL_DELETE:
		avd_sidep_indx_init(&opdata->objectName, &indx);
		sidep = avd_si_si_dep_find(avd_cb, &indx, TRUE);
		assert(dep_si = avd_si_get(&indx.si_name_sec));
		/* If SI is in tolerance timer running state because of this
		 * SI-SI dep cfg, then stop the tolerance timer.
		 */
		if (sidep->si_dep_timer.is_active == TRUE)
		{
			avd_stop_tmr(avd_cb, &sidep->si_dep_timer);

			if(dep_si->tol_timer_count > 0)
				dep_si->tol_timer_count--;
		}
		avd_si_dep_spons_list_del(avd_cb, sidep);
		avd_si_si_dep_del_row(avd_cb, sidep);
		/* Update the SI according to its existing sponsors state */
		avd_screen_sponsor_si_state(avd_cb, dep_si, TRUE);
		break;

	case CCBUTIL_MODIFY:
		avd_sidep_indx_init(&opdata->objectName, &indx);
		sidep = avd_si_si_dep_find(avd_cb, &indx, TRUE);
		while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
			if (!strcmp(attr_mod->modAttr.attrName, "saAmfToleranceTime")) {
				sidep->saAmfToleranceTime = *((SaTimeT *)attr_mod->modAttr.attrValues[0]);
			}
		}
		break;

	default:
		assert(0);
		break;
	}
}

void avd_sidep_constructor(void)
{
	unsigned int rc;
	NCS_PATRICIA_PARAMS patricia_params = {0};

	patricia_params.key_size = sizeof(AVD_SI_SI_DEP_INDX);
	rc = ncs_patricia_tree_init(&si_dep.spons_anchor, &patricia_params);
	assert(rc == NCSCC_RC_SUCCESS);
	rc = ncs_patricia_tree_init(&si_dep.dep_anchor, &patricia_params);
	assert(rc == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfSIDependency", NULL, NULL, sidep_ccb_completed_cb, sidep_ccb_apply_cb);
}

