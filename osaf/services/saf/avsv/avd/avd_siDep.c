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
static uint32_t avd_check_si_dep_sponsors(AVD_CL_CB *cb, AVD_SI *si, bool take_action);
static void avd_update_si_dep_state_for_spons_unassign(AVD_CL_CB *cb, AVD_SI *dep_si, AVD_SI_SI_DEP *si_dep_rec);
static uint32_t avd_si_dep_si_unassigned(AVD_CL_CB *cb, AVD_SI *si);
static uint32_t avd_check_si_state_enabled(AVD_CL_CB *cb, AVD_SI *si);
static uint32_t avd_sg_red_si_process_assignment(AVD_CL_CB *cb, AVD_SI *si);
static uint32_t avd_si_dep_state_evt(AVD_CL_CB *cb, AVD_SI *si, AVD_SI_SI_DEP_INDX *si_dep_idx);
static uint32_t avd_si_si_dep_cyclic_dep_find(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx);
static void avd_si_dep_start_unassign(AVD_CL_CB *cb, AVD_EVT *evt);
static void avd_dependentsi_role_failover(AVD_SI *si);

static AVD_SI_DEP si_dep; /* SI-SI dependency data */

static const char *depstatename[] = {
	"",
	"NO_DEPENDENCY",
	"SPONSOR_UNASSIGNED",
	"ASSIGNED",
	"TOL_TIMER_RUNNING",
	"READY_TO_UNASSIGN",
	"UNASSIGNING_DUE_TO_DEP",
	"FAILOVER_UNDER_PROGRESS"
};

void si_dep_state_set(AVD_SI *si, AVD_SI_DEP_STATE state)
{
	AVD_SI_DEP_STATE old_state = si->si_dep_state;

	if ((state != AVD_SI_TOL_TIMER_RUNNING) && (state != AVD_SI_READY_TO_UNASSIGN))
		avd_si_dep_stop_tol_timer(avd_cb, si);

	si->si_dep_state = state;
	TRACE("'%s' si_dep_state %s => %s", si->name.value, depstatename[old_state], depstatename[state]);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, si, AVSV_CKPT_SI_DEP_STATE);

	if ((state == AVD_SI_SPONSOR_UNASSIGNED) && (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE))
		avd_screen_sponsor_si_state(avd_cb, si, false);
}

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
uint32_t avd_check_si_state_enabled(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *susi = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s'", si->name.value);

	susi = si->list_of_sisu;
	while (susi != AVD_SU_SI_REL_NULL) {
		if (((susi->state == SA_AMF_HA_ACTIVE) || (susi->state == SA_AMF_HA_QUIESCING)) &&
			(susi->fsm == AVD_SU_SI_STATE_ASGND) &&
			(susi->si->si_dep_state != AVD_SI_FAILOVER_UNDER_PROGRESS)) {
			rc = NCSCC_RC_SUCCESS;
			break;
		}

		susi = susi->si_next;
	}

	if (rc == NCSCC_RC_SUCCESS) {
		if (si->si_dep_state == AVD_SI_NO_DEPENDENCY) {
			if (si->tol_timer_count == 0) {
				si_dep_state_set(si, AVD_SI_ASSIGNED);
			} else {
				si_dep_state_set(si, AVD_SI_TOL_TIMER_RUNNING);
			}
		}
	} else {
		if (si->si_dep_state != AVD_SI_FAILOVER_UNDER_PROGRESS)
			si_dep_state_set(si, AVD_SI_SPONSOR_UNASSIGNED);
	}

	TRACE_LEAVE2("%u", rc);
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

	/* Dependent SI should be active, if not return error */
	if ((dep_si = avd_si_get(&si_dep_rec->indx_imm.si_name_sec))
	    == AVD_SI_NULL) {
		/* LOG the message */
		goto done;
	}

	/* SI doesn't depend on any other SIs */
	if (dep_si->spons_si_list == NULL) {
		/* LOG the message */
		goto done;
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

done:
	TRACE_LEAVE();
}

static bool rec_already_in_sponsor_list(const AVD_SI *si, const AVD_SI_SI_DEP *rec)
{
	AVD_SPONS_SI_NODE *node;

	for (node = si->spons_si_list; node; node = node->next)
		if (node->sidep_rec == rec)
			return true;

	return false;
}

/*****************************************************************************
 * Function: avd_si_dep_spons_list_add
 *
 * Purpose:  This function adds the spons-SI node in the spons-list of 
 *           dependent-SI if not already exist.
 *
 * Input:  dep_si - ptr to AVD_SI struct (dependent-SI node).
 *         spons_si - ptr to AVD_SI struct (sponsor-SI node).
 *         rec
 * 
 **************************************************************************/
void avd_si_dep_spons_list_add(AVD_SI *dep_si, AVD_SI *spons_si, AVD_SI_SI_DEP *rec)
{
	AVD_SPONS_SI_NODE *spons_si_node;

	if (rec_already_in_sponsor_list(dep_si, rec))
		return;

	spons_si_node = calloc(1, sizeof(AVD_SPONS_SI_NODE));
	osafassert(spons_si_node);

	/* increment number of dependents in sponsor SI as well */
	spons_si->num_dependents ++;

	spons_si_node->si = spons_si;
	spons_si_node->sidep_rec = rec;

	spons_si_node->next = dep_si->spons_si_list;
	dep_si->spons_si_list = spons_si_node;
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

	memset((char *)&indx, '\0', sizeof(AVD_SI_SI_DEP_INDX));
	indx.si_name_sec.length = si->name.length;
	memcpy(indx.si_name_sec.value, si->name.value, si->name.length);

	while (spons_si_node) {
		/* Need to stop the tolerance timer */
		indx.si_name_prim.length = spons_si_node->si->name.length;
		memcpy(indx.si_name_prim.value, spons_si_node->si->name.value,
		       spons_si_node->si->name.length);

		if ((rec = avd_si_si_dep_find(cb, &indx, true)) != NULL) {
			if (rec->si_dep_timer.is_active == true) {
				avd_stop_tmr(cb, &rec->si_dep_timer);
				TRACE("Tolerance timer stopped for '%s'", si->name.value);

				if (si->tol_timer_count > 0) {
					si->tol_timer_count--;
				}
			}
		}

		spons_si_node = spons_si_node->next;
	}
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
uint32_t avd_si_dep_si_unassigned(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *susi = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s'", si->name.value);

	susi = si->list_of_sisu;
	while (susi != AVD_SU_SI_REL_NULL && susi->fsm != AVD_SU_SI_STATE_UNASGN) {
		if (avd_susi_del_send(susi) != NCSCC_RC_SUCCESS)
			goto done;

		/* add the su to su-oper list */
		avd_sg_su_oper_list_add(cb, susi->su, false);

		susi = susi->si_next;
	}

	si_dep_state_set(si, AVD_SI_SPONSOR_UNASSIGNED);

	/* transition to sg-realign state */
	m_AVD_SET_SG_FSM(cb, si->sg_of_si, AVD_SG_FSM_SG_REALIGN);

	/* Check if this SI is a sponsor SI for some other SI, the take appropriate action */
	avd_si_dep_spons_state_modif(cb, si, NULL, AVD_SI_DEP_SPONSOR_UNASSIGNED);

done:
	TRACE_LEAVE2("rc:%u", rc);
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
uint32_t avd_sg_red_si_process_assignment(AVD_CL_CB *cb, AVD_SI *si)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s'", si->name.value);

	if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) {
		TRACE("unstable");
		goto done;
	}

	if ((si->max_num_csi == si->num_csi) &&
	    (si->saAmfSIAdminState == SA_AMF_ADMIN_UNLOCKED) && (cb->init_state == AVD_APP_STATE)) {
		switch (si->sg_of_si->sg_redundancy_model) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			if (avd_sg_2n_si_func(cb, si) != NCSCC_RC_SUCCESS) {
				goto done;
			}
			break;

		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
			if (avd_sg_nacvred_si_func(cb, si) != NCSCC_RC_SUCCESS) {
				goto done;
			}
			break;

		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
			if (avd_sg_nway_si_func(cb, si) != NCSCC_RC_SUCCESS) {
				goto done;
			}
			break;

		case SA_AMF_NPM_REDUNDANCY_MODEL:
			if (avd_sg_npm_si_func(cb, si) != NCSCC_RC_SUCCESS) {
				goto done;
			}
			break;

		case SA_AMF_NO_REDUNDANCY_MODEL:
		default:
			if (avd_sg_nored_si_func(cb, si) != NCSCC_RC_SUCCESS) {
				goto done;
			}
			break;
		}

		if (avd_check_si_state_enabled(cb, si) == NCSCC_RC_SUCCESS) {
			si_dep_state_set(si, AVD_SI_ASSIGNED);
			rc = NCSCC_RC_SUCCESS;
		}
	}

done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
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
uint32_t avd_si_dep_state_evt(AVD_CL_CB *cb, AVD_SI *si, AVD_SI_SI_DEP_INDX *si_dep_idx)
{
	AVD_EVT *evt = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s'", si->name.value);

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
		si_dep_state_set(si, AVD_SI_READY_TO_UNASSIGN);
	} else {		/* For ASSIGN evt, just enough to feed SI name */

		evt->info.tmr.spons_si_name.length = si->name.length;
		memcpy(evt->info.tmr.spons_si_name.value, si->name.value, si->name.length);
	}

	TRACE("%u", evt->rcv_evt);

	if (m_NCS_IPC_SEND(&cb->avd_mbx, evt, NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: ipc send %u failed", __FUNCTION__, evt->rcv_evt);
		free(evt);
		goto done;
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE2("rc:%u", rc);
	return rc;
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

	osafassert(evt->info.tmr.type == AVD_TMR_SI_DEP_TOL);

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
	avd_screen_sponsor_si_state(cb, si, false);

	switch (si->si_dep_state) {
	case AVD_SI_NO_DEPENDENCY:
		si_dep_state_set(si, AVD_SI_SPONSOR_UNASSIGNED);
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
		si_dep_state_set(si, AVD_SI_SPONSOR_UNASSIGNED);
		goto done;
	}

	/* Check if spons_si SI-Dep state is not in ASSIGNED state, then 
	 * only initiate the unassignment process. Because the SI can be 
	 * in this state due to some other spons-SI.
	 */
	if (avd_check_si_state_enabled(cb, spons_si) != NCSCC_RC_SUCCESS) {
		if (cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
			si_dep_state_set(si, AVD_SI_UNASSIGNING_DUE_TO_DEP);

			if (avd_si_dep_si_unassigned(cb, si) != NCSCC_RC_SUCCESS) {
				TRACE("'%s' Dep SI unassignment failed", si->name.value);
			}
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
 *           take_action - If true, process the impacts (SI-SI dependencies)
 *                      on dependent-SIs (of sponsor-SI being enabled/disabled)
 *
 * Returns: NCSCC_RC_SUCCESS - if sponsor-SI is in enabled state 
 *          NCSCC_RC_FAILURE - if sponsor-SI is in disabled state 
 *
 * NOTES:  
 * 
 **************************************************************************/
uint32_t avd_check_si_dep_sponsors(AVD_CL_CB *cb, AVD_SI *si, bool take_action)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVD_SPONS_SI_NODE *spons_si_node = si->spons_si_list;

	TRACE_ENTER2("'%s'", si->name.value);

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
		si_dep_state_set(si, AVD_SI_ASSIGNED);

		avd_si_dep_spons_state_modif(cb, si, NULL, AVD_SI_DEP_SPONSOR_ASSIGNED);
	}

	TRACE_LEAVE2("rc:%u", rc);
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

	TRACE_ENTER2("'%s'", sg->name.value);

	si = sg->list_of_si;
	while (si != AVD_SI_NULL) {
		if (avd_check_si_state_enabled(cb, si) == NCSCC_RC_SUCCESS) {
			/* SI was in enabled state, so check for the SI-SI dependencies
			 * conditions and update the SI accordingly.
			 */
			avd_check_si_dep_sponsors(cb, si, true);
		} else {
			avd_screen_sponsor_si_state(cb, si, true);
			if ((si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED) ||
			    (si->si_dep_state == AVD_SI_NO_DEPENDENCY)) {
				/* Check if this SI is a sponsor SI for some other SI, the take appropriate action */
				avd_si_dep_spons_state_modif(cb, si, NULL, AVD_SI_DEP_SPONSOR_UNASSIGNED);
			}
		}

		si = si->sg_list_of_si_next;
	}

	TRACE_LEAVE();
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
void avd_screen_sponsor_si_state(AVD_CL_CB *cb, AVD_SI *si, bool start_assignment)
{
	TRACE_ENTER2("%s", si->name.value);

	/* Change the SI dependency state only when all of its sponsor SIs are 
	 * in assigned state.
	 */
	if (avd_check_si_dep_sponsors(cb, si, false) != NCSCC_RC_SUCCESS) {
		/* Nothing to do here, just return */
		goto done;
	}

	switch (si->si_dep_state) {
	case AVD_SI_TOL_TIMER_RUNNING:
	case AVD_SI_READY_TO_UNASSIGN:
		if (si->tol_timer_count == 0)
			si_dep_state_set(si, AVD_SI_ASSIGNED);
		break;
	case AVD_SI_FAILOVER_UNDER_PROGRESS:
		break;

	case AVD_SI_SPONSOR_UNASSIGNED:
		/* If SI-SI dependency cfg's are removed for this SI the update the 
		 * SI dep state with AVD_SI_NO_DEPENDENCY 
		 */
		si_dep_state_set(si, AVD_SI_NO_DEPENDENCY);

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
		break;
	}

done:
	TRACE_LEAVE();
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
	bool role_failover_pending = false;

	TRACE_ENTER();

	if (evt == NULL) {
		/* Log the message */
		goto done; 
	}

	/* Check whether rcv_evt is AVD_EVT_SI_DEP_STATE event, if not LOG the 
	 * error message.
	 */
	if (evt->rcv_evt != AVD_EVT_SI_DEP_STATE) {
		/* internal error */
		osafassert(0);
		goto done;
	}

	if (evt->info.tmr.dep_si_name.length == 0) {
		/* Its an ASSIGN evt */
		si = avd_si_get(&evt->info.tmr.spons_si_name);
		if (si != NULL) {

			/* Check if we are yet to do SI role failover for the dependent SI
			 */
			if (si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS)
				role_failover_pending = true;
			if ((si->list_of_sisu) && (si->list_of_sisu->state == SA_AMF_HA_STANDBY) &&
				(si->list_of_sisu->fsm != AVD_SU_SI_STATE_UNASGN) &&
				(si->list_of_sisu->si_next == NULL)) {
					role_failover_pending = true;
			}
			if (role_failover_pending == true) {	
				avd_dependentsi_role_failover(si);
			} else {
				if ((si->si_dep_state == AVD_SI_ASSIGNED) && (si->list_of_sisu == NULL)) {
					/* Dependent SI does not have assignment but still si->si_dep_state is set to AVD_SI_ASSIGNED.
					   So reset the si_dep_state to AVD_SI_NO_DEPENDENCY, so that the SI will be considered for assignment
					 */ 
					si_dep_state_set(si, AVD_SI_NO_DEPENDENCY);
				}

				/* If dependent is already assigned and its failover is not pending then 
				  update its dep state to ASSIGNED.*/
				if ((si->si_dep_state != AVD_SI_FAILOVER_UNDER_PROGRESS) && 
						(all_sponsors_assigned_active(si)) && 
						(si_assignment_state_check(si))) {
					si_dep_state_set(si, AVD_SI_ASSIGNED);
				}

				avd_screen_sponsor_si_state(cb, si, true);
			}
		}
	} else {		/* Process UNASSIGN evt */

		avd_si_dep_start_unassign(cb, evt);
	}

done:
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
		goto done;
	}

	if (si->si_dep_state != AVD_SI_READY_TO_UNASSIGN) {
		/* Log the message */
		goto done;
	}

	/* Before starting unassignment process of SI, check again whether all its
	 * sponsors are moved back to assigned state, if so it is not required to
	 * initiate the unassignment process for the (dependent) SI.
	 */
	avd_screen_sponsor_si_state(cb, si, false);
	if (si->si_dep_state == AVD_SI_ASSIGNED)
		goto done;

	/* If the SI is already been unassigned, nothing to proceed for 
	 * unassignment 
	 */
	if (si->list_of_sisu == AVD_SU_SI_REL_NULL) {
		si_dep_state_set(si, AVD_SI_SPONSOR_UNASSIGNED);
		goto done;
	}

	/* Check if spons_si SI-Dep state is not in ASSIGNED state, then 
	 * only initiate the unassignment process. Because the SI can be 
	 * in this state due to some other spons-SI.
	 */
	if (avd_check_si_state_enabled(cb, spons_si) != NCSCC_RC_SUCCESS) {
		si_dep_state_set(si, AVD_SI_UNASSIGNING_DUE_TO_DEP);

		if (avd_si_dep_si_unassigned(cb, si) != NCSCC_RC_SUCCESS) {
			/* Log the error */
		}
	}

done:
	TRACE_LEAVE();
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
			TRACE("Starting Timer for SI:%s",dep_si->name.value);
			/* Start the tolerance timer */
			si_dep_state_set(dep_si, AVD_SI_TOL_TIMER_RUNNING);

			/* Start the tolerance timer */
			m_AVD_SI_DEP_TOL_TMR_START(cb, si_dep_rec);

			if (dep_si->tol_timer_count) {
				/* It suppose to be "0", LOG the err msg and continue. */
			}

			dep_si->tol_timer_count = 1;
		} else if (cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
			/* Send an event to start SI unassignment process */
			avd_si_dep_state_evt(cb, dep_si, &si_dep_rec->indx_imm);
		}
		break;

	case AVD_SI_TOL_TIMER_RUNNING:
	case AVD_SI_READY_TO_UNASSIGN:
		if (si_dep_rec->saAmfToleranceTime > 0) {
			if (si_dep_rec->si_dep_timer.is_active != true) {
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
				si_dep_rec->unassign_event = true;

				/* Send an event to start SI unassignment process */
				avd_si_dep_state_evt(cb, dep_si, &si_dep_rec->indx_imm);
			}
		}
		break;

	case AVD_SI_NO_DEPENDENCY:
		si_dep_state_set(dep_si, AVD_SI_SPONSOR_UNASSIGNED);
		break;

	default:
		break;
	}

	TRACE_LEAVE();
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
				goto done;
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
		si_dep_rec = avd_si_si_dep_find_next(cb, &si_indx, true);
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
				si_dep_rec = avd_si_si_dep_find_next(cb, &si_dep_rec->indx_imm, true);
				continue;
			}

			if (spons_state == AVD_SI_DEP_SPONSOR_UNASSIGNED) {
				/* If the dependent SI is under AVD_SI_FAILOVER_UNDER_PROGRESS state
				 * update its dep_state to AVD_SI_READY_TO_UNASSIGN
				 */
				if (dep_si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS) {
					TRACE("Send an event to start SI unassignment process");
					dep_si->si_dep_state = AVD_SI_READY_TO_UNASSIGN;
				}
				avd_update_si_dep_state_for_spons_unassign(cb, dep_si, si_dep_rec);
			} else {
				avd_si_dep_state_evt(cb, dep_si, NULL);
			}

			si_dep_rec = avd_si_si_dep_find_next(cb, &si_dep_rec->indx_imm, true);
		}
	} else {
		/* Just ignore and return if spons_state is AVD_SI_DEP_SPONSOR_ASSIGNED */
		if (spons_state == AVD_SI_DEP_SPONSOR_ASSIGNED)
			goto done;

		/* Frame the index completely to the associated si_dep_rec */
		si_indx.si_name_sec.length = si_dep->name.length;
		memcpy(si_indx.si_name_sec.value, si_dep->name.value, si_dep->name.length);

		si_dep_rec = avd_si_si_dep_find(cb, &si_indx, true);

		if (si_dep_rec != NULL) {
			/* si_dep_rec primary key should match with sponsor SI name */
			if (m_CMP_HORDER_SANAMET(si_dep_rec->indx_imm.si_name_prim, si_indx.si_name_prim) != 0) {
				osafassert(0);
				goto done;
			}
			avd_update_si_dep_state_for_spons_unassign(cb, si_dep, si_dep_rec);
		}
	}

done:
	TRACE_LEAVE();
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
	AVD_SI_SI_DEP *rec;
	uint32_t si_prim_len = indx->si_name_prim.length;
	uint32_t si_sec_len = indx->si_name_sec.length;

	TRACE_ENTER();

	if ((rec = avd_si_si_dep_find(cb, indx, true)) != NULL)
		goto done;

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

	rec->tree_node_imm.key_info = (uint8_t *)&(rec->indx_imm);
	rec->tree_node_imm.bit = 0;
	rec->tree_node_imm.left = NCS_PATRICIA_NODE_NULL;
	rec->tree_node_imm.right = NCS_PATRICIA_NODE_NULL;

	rec->tree_node.key_info = (uint8_t *)&(rec->indx);
	rec->tree_node.bit = 0;
	rec->tree_node.left = NCS_PATRICIA_NODE_NULL;
	rec->tree_node.right = NCS_PATRICIA_NODE_NULL;

	if (ncs_patricia_tree_add(&si_dep.spons_anchor, &rec->tree_node_imm) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: spons ncs_patricia_tree_add failed", __FUNCTION__);
		free(rec);
		rec = NULL;
		goto done;
	}

	if (ncs_patricia_tree_add(&si_dep.dep_anchor, &rec->tree_node) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: dep ncs_patricia_tree_add failed", __FUNCTION__);
		ncs_patricia_tree_del(&si_dep.spons_anchor, &rec->tree_node_imm);
		free(rec);
		rec = NULL;
		goto done;
	}

done:
	TRACE_LEAVE();
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
AVD_SI_SI_DEP *avd_si_si_dep_find(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx, bool isImmIdx)
{
	AVD_SI_SI_DEP *rec = NULL;

	if (isImmIdx) {
		rec = (AVD_SI_SI_DEP *)ncs_patricia_tree_get(&si_dep.spons_anchor, (uint8_t *)indx);
	} else {
		rec = (AVD_SI_SI_DEP *)ncs_patricia_tree_get(&si_dep.dep_anchor, (uint8_t *)indx);
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
AVD_SI_SI_DEP *avd_si_si_dep_find_next(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx, bool isImmIdx)
{
	AVD_SI_SI_DEP *rec = NULL;

	if (isImmIdx) {
		rec = (AVD_SI_SI_DEP *)ncs_patricia_tree_getnext(&si_dep.spons_anchor, (uint8_t *)indx);
	} else {
		rec = (AVD_SI_SI_DEP *)ncs_patricia_tree_getnext(&si_dep.dep_anchor, (uint8_t *)indx);
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
uint32_t avd_si_si_dep_del_row(AVD_CL_CB *cb, AVD_SI_SI_DEP *rec)
{
	AVD_SI_SI_DEP *si_dep_rec = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER();

	if (rec == NULL)
		goto done;

	if ((si_dep_rec = avd_si_si_dep_find(cb, &rec->indx, false)) != NULL) {
		if (ncs_patricia_tree_del(&si_dep.dep_anchor, &si_dep_rec->tree_node)
		    != NCSCC_RC_SUCCESS) {
			LOG_ER("Failed deleting SI Dep from Dependent Anchor");
			goto done;
		}
	}

	si_dep_rec = NULL;

	if ((si_dep_rec = avd_si_si_dep_find(cb, &rec->indx_imm, true)) != NULL) {
		if (ncs_patricia_tree_del(&si_dep.spons_anchor, &si_dep_rec->tree_node_imm)
		    != NCSCC_RC_SUCCESS) {
			LOG_ER("Failed deleting SI Dep from Sponsor Anchor");
			goto done;
		}
	}

	if (si_dep_rec)
		free(si_dep_rec);

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE();
	return rc;
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
uint32_t avd_si_si_dep_cyclic_dep_find(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx)
{
	AVD_SI_SI_DEP *rec = NULL;
	AVD_SI_DEP_NAME_LIST *start = NULL;
	AVD_SI_DEP_NAME_LIST *temp = NULL;
	AVD_SI_DEP_NAME_LIST *last = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;
	AVD_SI_SI_DEP_INDX idx;

	TRACE_ENTER();

	if (m_CMP_HORDER_SANAMET(indx->si_name_prim, indx->si_name_sec) == 0) {
		/* dependent SI and Sponsor SI can not be same 
		   Cyclic dependency found return sucess
		 */
		rc = NCSCC_RC_SUCCESS;
		goto done;
	}

	if ((start = malloc(sizeof(AVD_SI_DEP_NAME_LIST))) == NULL) {
		/*Insufficient memory, record can not be added */
		goto done;
	} else {
		start->si_name = indx->si_name_prim;
		start->next = NULL;
		last = start;
	}

	while (last) {
		memset((char *)&idx, '\0', sizeof(AVD_SI_SI_DEP_INDX));

		idx.si_name_prim.length = last->si_name.length;
		memcpy(idx.si_name_prim.value, last->si_name.value, last->si_name.length);

		rec = avd_si_si_dep_find_next(cb, &idx, false);

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

			rec = avd_si_si_dep_find_next(cb, &rec->indx, false);
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

done:
	TRACE_LEAVE();
	return rc;
}

static int avd_sidep_indx_init(const SaNameT *sidep_name, AVD_SI_SI_DEP_INDX *indx)
{
	char *p;
	SaNameT tmp;
	uint32_t i = 0;

	memset(&tmp, 0, sizeof(SaNameT));
	tmp.length = sidep_name->length;
	memcpy(&tmp.value, &sidep_name->value, tmp.length);
		
	memset(indx, 0, sizeof(AVD_SI_SI_DEP_INDX));

	/* find first occurence and step past it */
	p = strstr((char *)tmp.value, "safSi=") + 1;
	if (p == NULL) return false;

	/* find second occurence, an error if not found */
	p = strstr(p, "safSi=");
	if (p == NULL) return false;

	*(p - 1) = '\0';	/* null terminate at comma before SI */

	indx->si_name_sec.length = snprintf((char *)indx->si_name_sec.value, SA_MAX_NAME_LENGTH, "%s", p);

	/* Skip past the RDN tag */
	p = strchr((char *)tmp.value, '=') + 1;
	if (p == NULL) return false;

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

	return true;
}

static uint32_t is_config_valid(SaNameT *sidep_name, CcbUtilOperationData_t *opdata)
{
	AVD_SI_SI_DEP_INDX indx;
	AVD_SI *spons_si, *dep_si;
	CcbUtilOperationData_t *tmp;
	uint32_t dep_saAmfSIRank, spons_saAmfSIRank;
	uint32_t rc = false;
	bool dependent_si_is_assigned = false;

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
		spons_saAmfSIRank = spons_si->saAmfSIRank;
	}

	if (spons_saAmfSIRank == 0)
		spons_saAmfSIRank = ~0U; /* zero means lowest possible rank */

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
		if (dep_si->list_of_sisu)
			dependent_si_is_assigned = true;
		dep_saAmfSIRank = dep_si->saAmfSIRank;
	}

	/* don't allow to dynamically create a dependency from an assigned SI already in the model */
	if ((opdata != NULL) && dependent_si_is_assigned) {
		LOG_ER("SI dep validation: adding dependency from existing SI '%s' to SI '%s' is not allowed",
			indx.si_name_prim.value, indx.si_name_sec.value);
		goto done;
	}

	if (dep_saAmfSIRank == 0)
		dep_saAmfSIRank = ~0U; /* zero means lowest possible rank */

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
	osafassert(spons_si = avd_si_get(&indx.si_name_prim));
	osafassert(dep_si = avd_si_get(&indx.si_name_sec));
	avd_si_dep_spons_list_add(dep_si, spons_si, sidep);

	if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE)  {
		/* Move the dependent SI to appropriate state, if the configured 
		 * sponsor is not in assigned state. Not required to check with
		 * the remaining states of SI Dep states, as they are kind of 
		 * intermittent states towards SPONSOR_UNASSIGNED/ASSIGNED states. 
		 */
		if ((spons_si->si_dep_state == AVD_SI_NO_DEPENDENCY) ||
			(spons_si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED)) {
			if (dep_si->si_dep_state == AVD_SI_NO_DEPENDENCY) {
				si_dep_state_set(dep_si, AVD_SI_SPONSOR_UNASSIGNED);
			} else {
				if (dep_si->si_dep_state != AVD_SI_SPONSOR_UNASSIGNED) {
					/* Sponsor SI is not in assigned state & Dependent SI has assignments, So unassign the
					 * assignments and update the si_dep_state of Dependent SI
					 */
					avd_si_dep_spons_state_modif(avd_cb, spons_si, dep_si, AVD_SI_DEP_SPONSOR_UNASSIGNED);
				}
			}
		}
	}
done:
	TRACE_LEAVE();
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

	TRACE_ENTER();

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
	TRACE_LEAVE2("%u", error);
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
		if (NULL == avd_si_si_dep_find(avd_cb, &indx, true))
			rc = SA_AIS_ERR_BAD_OPERATION;
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();	
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
		sidep = avd_si_si_dep_find(avd_cb, &indx, true);
		osafassert(dep_si = avd_si_get(&indx.si_name_sec));
		/* If SI is in tolerance timer running state because of this
		 * SI-SI dep cfg, then stop the tolerance timer.
		 */
		if (sidep->si_dep_timer.is_active == true)
		{
			avd_stop_tmr(avd_cb, &sidep->si_dep_timer);

			if(dep_si->tol_timer_count > 0)
				dep_si->tol_timer_count--;
		}
		avd_si_dep_spons_list_del(avd_cb, sidep);
		avd_si_si_dep_del_row(avd_cb, sidep);
		if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE)  {
			/* Update the SI according to its existing sponsors state */
			avd_screen_sponsor_si_state(avd_cb, dep_si, true);
		}
		break;

	case CCBUTIL_MODIFY:
		avd_sidep_indx_init(&opdata->objectName, &indx);
		sidep = avd_si_si_dep_find(avd_cb, &indx, true);
		while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
			if (!strcmp(attr_mod->modAttr.attrName, "saAmfToleranceTime")) {
				sidep->saAmfToleranceTime = *((SaTimeT *)attr_mod->modAttr.attrValues[0]);
			}
		}
		break;

	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

void avd_sidep_constructor(void)
{
	unsigned int rc;
	NCS_PATRICIA_PARAMS patricia_params = {0};

	patricia_params.key_size = sizeof(AVD_SI_SI_DEP_INDX);
	rc = ncs_patricia_tree_init(&si_dep.spons_anchor, &patricia_params);
	osafassert(rc == NCSCC_RC_SUCCESS);
	rc = ncs_patricia_tree_init(&si_dep.dep_anchor, &patricia_params);
	osafassert(rc == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfSIDependency", NULL, NULL, sidep_ccb_completed_cb, sidep_ccb_apply_cb);
}
void avd_sidep_start_tolerance_timer_for_dependant(AVD_SI *dep_si, AVD_SI *spons_si)
{
	AVD_SI_SI_DEP_INDX si_indx;
	AVD_SI_SI_DEP *si_dep_rec;

	TRACE("dep_si:%s spons_si:%s",dep_si->name.value,spons_si->name.value);

	/* Frame the index completely to the associated si_dep_rec */
	memset((char *)&si_indx, '\0', sizeof(AVD_SI_SI_DEP_INDX));
	si_indx.si_name_prim.length = spons_si->name.length;
	memcpy(si_indx.si_name_prim.value, spons_si->name.value, si_indx.si_name_prim.length);
	si_indx.si_name_sec.length = dep_si->name.length;
	memcpy(si_indx.si_name_sec.value, dep_si->name.value, dep_si->name.length);

	si_dep_rec = avd_si_si_dep_find(avd_cb, &si_indx, true);
	if (si_dep_rec != NULL) {
		avd_update_si_dep_state_for_spons_unassign(avd_cb, dep_si, si_dep_rec);
	} else {
		TRACE("si_dep_rec is NULL");
	}

	TRACE_LEAVE();
}
/**
 * @brief      Remove all the assignments of an SI 
 *
 * @param[in]  si
 **/
void avd_si_unassign(AVD_SI *si)
{
        AVD_SU_SI_REL *sisu;

        for (sisu = si->list_of_sisu;sisu;sisu = sisu->si_next) {
                if (sisu->fsm != AVD_SU_SI_STATE_UNASGN)
                        avd_susi_del_send(sisu);
        }
}
/**
 * @brief	If role failover is not possible for the Sponsor SI, this routine iterates through all of its dependantsi and
 *		1) removes the SI assignments, if the dependant has Active assignment on the which went down
 *		2) starts tolerance timer if the dependant SI has Active assignment on some other node 
 *
 * @param[in]  si
 **/
void avd_sidep_unassign_dependents(AVD_SI *si, AVD_SU *su)
{
	AVD_SI_SI_DEP_INDX si_indx;
	AVD_SI_SI_DEP *si_dep_rec;
	AVD_SI *dep_si;
	AVD_SU_SI_REL *sisu;

	TRACE_ENTER2(": '%s'",si->name.value);

	memset(&si_indx, '\0', sizeof(si_indx));
	si_indx.si_name_prim.length = si->name.length;
	memcpy(si_indx.si_name_prim.value, si->name.value, si_indx.si_name_prim.length);
	si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_indx, true);

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
			si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
			continue;
		}
		/* Get the Active susi */
		for (sisu = dep_si->list_of_sisu;sisu;sisu = sisu->si_next) {
			if (((sisu->state == SA_AMF_HA_ACTIVE) || (sisu->state == SA_AMF_HA_QUIESCING)) &&
					(sisu->fsm != AVD_SU_SI_STATE_UNASGN)) {
				break;
			}
		}
		if (sisu == NULL) {
			si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
			continue;
		}

		/* if dependant SI has Active assignment on the node which went down, remove the assignments 
		 * else start Tolerance timer
		 */
		if (m_NCS_MDS_DEST_EQUAL(&sisu->su->su_on_node->adest,&su->su_on_node->adest)) {
			avd_si_unassign(dep_si);
		} else {
			if((dep_si->si_dep_state != AVD_SI_TOL_TIMER_RUNNING) ||
					(dep_si->si_dep_state != AVD_SI_READY_TO_UNASSIGN)) {
				avd_sidep_start_tolerance_timer_for_dependant(dep_si, si);

			}
		}
		/* If this dependent SI is sponsor too, then unassign its dependents also */
		if (dep_si->num_dependents > 0)
                        avd_sidep_unassign_dependents(dep_si, su);

		si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
	}
	TRACE_LEAVE();
}
/**
 * @brief       Checks the following assignment status of an SI during failover
 *              1) Whether SI has Active assignment
 *              2) Whether SI has valid standby assignments which can take Active role 
 *
 * @param[in]  si -- SI for which assignment state needs to be checked
 * @param[in]  su -- SU which faulted 
 * @param[out] active_sisu -- active susi assignments 
 * @param[out] valid_stdby_sisu -- valid standby assignment 
 *
 * @return  true/false    
 **/
bool si_assignment_check_during_failover(AVD_SI *si, AVD_SU *su, AVD_SU_SI_REL **active_sisu, AVD_SU_SI_REL **valid_stdby_sisu)
{
	AVD_SU_SI_REL *sisu;
	bool assignmemt_status = false;

	for (sisu = si->list_of_sisu;sisu;sisu = sisu->si_next) {
		if ((sisu->su != su) && (*valid_stdby_sisu == NULL)) {
			if (((sisu->state == SA_AMF_HA_STANDBY) || (sisu->state == SA_AMF_HA_QUIESCED)) &&
				(sisu->fsm == AVD_SU_SI_STATE_ASGND) &&
				(sisu->su->saAmfSuReadinessState != SA_AMF_READINESS_OUT_OF_SERVICE)) {
				*valid_stdby_sisu = sisu;
				if ((*active_sisu) && (*valid_stdby_sisu))
					break;
				else
					continue;
			}
		}
		if (*active_sisu == NULL) {
			if (((sisu->state == SA_AMF_HA_ACTIVE) || (sisu->state == SA_AMF_HA_QUIESCING) ||
				(sisu->state == SA_AMF_HA_QUIESCED)) &&
				(sisu->fsm != AVD_SU_SI_STATE_UNASGN)) {
				*active_sisu = sisu;
				assignmemt_status = true;

				if ((*active_sisu) && (*valid_stdby_sisu))
					break;
				else
					continue;
			}
		}
	}

        return assignmemt_status;
}
/**
 * @brief      Checks if role failover can be performed for this SI,
 *             role failover can not be done if the SI has dependency on
 *             an SI which has Active assignments on the same node
 *
 * @param[in]  si   -
 *
 * @return	true/false 
 *             
 **/
bool avd_sidep_is_si_failover_possible(AVD_SI *si, AVD_SU *su)
{
	AVD_SPONS_SI_NODE *spons_si_node;
	AVD_SU_SI_REL *sisu, *tmp_sisu;                 
	AVD_SU_SI_REL *active_sisu = NULL, *valid_stdby_sisu = NULL;
	bool failover_possible = true;
	bool assignmemt_status = false;
	bool valid_standby = false;

	TRACE_ENTER2("SI: '%s'",si->name.value);

	/* Role failover triggered because of node going down */
	if(su->su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) {
		assignmemt_status = si_assignment_check_during_failover(si, su, &active_sisu, &valid_stdby_sisu);
		if (si->spons_si_list == NULL) {
			/* Check if the susi is in unassigned state
			 * 1) Beacuse of admin operation
			 * 2) SI has Active assignments on the node which went down and there is no Standby assignment for the SI 
			 */
			if ((si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED) ||
				(si->sg_of_si->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) ||
				((si->list_of_sisu) && (si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL) &&
				(m_NCS_MDS_DEST_EQUAL(&si->list_of_sisu->su->su_on_node->adest,&su->su_on_node->adest))) ||
				(valid_stdby_sisu == NULL)) {

				avd_si_unassign(si);
				failover_possible = false;
				if (si->num_dependents > 0)
					avd_sidep_unassign_dependents(si, su);
			}
			goto done;
		}

		if (si->si_dep_state == AVD_SI_TOL_TIMER_RUNNING) {
			/* Dependant SI and Tolerance timer is running
			 * No need to to do role failover stop the Tolerance timer and delete the assignments
			 */
			avd_si_dep_stop_tol_timer(avd_cb, si);
			avd_si_unassign(si);

			failover_possible = false;
			if (si->num_dependents > 0)
				avd_sidep_unassign_dependents(si, su);
			goto done;
		}
		/* Case of dependant SI
		 * Failover is possible for the dependant only in the following case
		 * 1) None of its sponsors have Active assignment on the node which went down
		 * 2) None of its sponsors SI is undergoing admin lock operation
		 */
		for (spons_si_node = si->spons_si_list;spons_si_node;spons_si_node = spons_si_node->next) {
			assignmemt_status = false;
			active_sisu = NULL;
			valid_stdby_sisu = NULL;

			assignmemt_status = si_assignment_check_during_failover(spons_si_node->si, su, &active_sisu, &valid_stdby_sisu);

			if ((spons_si_node->si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED) ||
					(spons_si_node->si->sg_of_si->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) ||
					((spons_si_node->si->list_of_sisu) && (spons_si_node->si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL) &&
					 (m_NCS_MDS_DEST_EQUAL(&spons_si_node->si->list_of_sisu->su->su_on_node->adest,&su->su_on_node->adest))) ||
					((assignmemt_status ==  false) && (valid_stdby_sisu == NULL))) {
				avd_si_unassign(si);
				failover_possible = false;
				goto done;
			}
			if ((spons_si_node->si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS) ||
				((active_sisu) && (m_NCS_MDS_DEST_EQUAL(&active_sisu->su->su_on_node->adest,&su->su_on_node->adest)))) {
				failover_possible = false;
			}
		}
	} else {
		if (si->spons_si_list == NULL) {
			TRACE("SI doesnot have any dependencies");
			/* Check whether valid standby SU is there for performing role failover */
			for (sisu = si->list_of_sisu;sisu != NULL;sisu = sisu->si_next) {
				if (sisu->su != su) {
					if (((sisu->state == SA_AMF_HA_STANDBY) || (sisu->state == SA_AMF_HA_QUIESCED)) && 
						(sisu->fsm == AVD_SU_SI_STATE_ASGND) &&
						(sisu->su->saAmfSuReadinessState != SA_AMF_READINESS_OUT_OF_SERVICE)) {
						valid_standby = true;
					}
				}
			}

			if ((si->saAmfSIAdminState == SA_AMF_ADMIN_LOCKED) || (!valid_standby)) {
				failover_possible = false;
				avd_si_unassign(si);
			}
			goto done;

		}
		bool sponsor_in_modify_state = false;
		for (spons_si_node = si->spons_si_list;spons_si_node;spons_si_node = spons_si_node->next) {
			failover_possible = false;
			valid_standby = false;
			for (sisu = spons_si_node->si->list_of_sisu;sisu;sisu = sisu->si_next) {
				TRACE("sisu->si:%s state:%u fsm:%u",sisu->si->name.value,sisu->state,sisu->fsm);
				if (((sisu->state == SA_AMF_HA_ACTIVE) || (sisu->state == SA_AMF_HA_QUIESCING)) &&
					(sisu->fsm != AVD_SU_SI_STATE_UNASGN)) {
					failover_possible = true;
					if (sisu->fsm == AVD_SU_SI_STATE_MODIFY)
						sponsor_in_modify_state = true;
					break;
				} else if ((spons_si_node->si->sg_of_si == si->sg_of_si) && (sisu->su != su) &&
					(!valid_standby)) {
					/* Sponsor also belongs to the same SG 
					 * Check if it has valid standby susi for failover 
					 */
					if (((sisu->state == SA_AMF_HA_STANDBY) || (sisu->state == SA_AMF_HA_QUIESCED)) &&
						(sisu->fsm == AVD_SU_SI_STATE_ASGND) &&
						(sisu->su->saAmfSuReadinessState != SA_AMF_READINESS_OUT_OF_SERVICE)) {
						valid_standby = true;	
					}
				}
			}
			if ((failover_possible == false) && (valid_standby == false)) {
				/* There is no Active assignment and no valid Standby which can take Active assignment */
				if (si->si_dep_state != AVD_SI_TOL_TIMER_RUNNING) {
					for (tmp_sisu = si->list_of_sisu;tmp_sisu != NULL;tmp_sisu = tmp_sisu->si_next)
						if (tmp_sisu->fsm != AVD_SU_SI_STATE_UNASGN)
							avd_susi_del_send(tmp_sisu);
				}
				goto done;
			}
		}
		if (sponsor_in_modify_state == true) {
			si_dep_state_set(si, AVD_SI_FAILOVER_UNDER_PROGRESS);
			failover_possible = false;
			goto done;
		}
	}
done:
	TRACE_LEAVE2("return value: %d",failover_possible);
	return failover_possible;
}
/**
 * @brief      Checks if role failover can be performed for a SU. Role failover
 *             can not be done if any of the SI's having Active assignments on
 *             this SU has Active assignments on the same node
 *
 * @param[in]  su
 *
 * @return	true/flase
 *             
 **/
bool avd_sidep_is_su_failover_possible(AVD_SU *su)
{
	AVD_SU_SI_REL *susi;
	bool flag = true;

	TRACE_ENTER2("SU:'%s' node_state:%d",su->name.value,su->su_on_node->node_state);

	for (susi = su->list_of_susi;susi != NULL;susi = susi->su_next) {
		if (susi->si->spons_si_list) {
			TRACE(":susi:%s si_dep_state:%u state:%u fsm:%u",susi->si->name.value,susi->si->si_dep_state, susi->state,susi->fsm);
			if (susi->si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS) {
				TRACE("Role failover is deferred as sponsors role failover is under going");
				flag = false;
				goto done;
			}
			/* Check if susi->si has any dependency on si having Active asisgnments
			 * on the other SU on the same node
			 */
			flag = avd_sidep_is_si_failover_possible(susi->si, su);
			if (flag == false){
				TRACE("Role failover is deferred as sponsors role failover is under going");
				si_dep_state_set(susi->si, AVD_SI_FAILOVER_UNDER_PROGRESS);
				goto done;
			}
		}
	}
done:
	TRACE_LEAVE2("return value: %d",flag);
	return flag;
}

/**
 * @brief       Checks whether SI has valid standby assignments which can take Active role 
 *
 * @param[in]	susi
 *
 * @return	true/false    
 **/
bool valid_standby_susi(AVD_SU_SI_REL *sisu)
{
	AVD_SU_SI_REL *tmp_sisu;

	for (tmp_sisu = sisu->si->list_of_sisu;tmp_sisu != NULL;tmp_sisu = tmp_sisu->si_next) {
		if (tmp_sisu->su != sisu->su) {
			if (((tmp_sisu->state == SA_AMF_HA_STANDBY) || (tmp_sisu->state == SA_AMF_HA_QUIESCED)) &&
					(tmp_sisu->fsm == AVD_SU_SI_STATE_ASGND) &&
					(tmp_sisu->su->saAmfSuReadinessState != SA_AMF_READINESS_OUT_OF_SERVICE)) {
				return true;
			}
		}
	}
	return false;
}
/**
 * @brief	Updates dependents si_dep_state during SI role failover.	
 *		a) If the dependent SI has Active assignment on the the same node then
 *		   sets the si_dep_state to AVD_SI_FAILOVER_UNDER_PROGRESS
 *		b) If the dependent SI has Active assignments on some other node then
 *		   	if the tolerence timer is configured sets dep_state to AVD_SI_TOL_TIMER_RUNNING
 *		   	else sets si_dep_satte to AVD_SI_READY_TO_UNASSIGN 
 *
 *
 * @param[in]  su   -
 *
 * @return	 
 *             
 **/
void avd_update_depstate_si_failover(AVD_SI *si, AVD_SU *su)
{
	AVD_SI_SI_DEP_INDX si_indx;
	AVD_SI_SI_DEP *si_dep_rec;
	AVD_SI *dep_si;
	AVD_SU_SI_REL *sisu;
	AVD_SPONS_SI_NODE *spons_si_node;

	TRACE_ENTER2("si:%s",si->name.value);

	memset((char *)&si_indx, '\0', sizeof(si_indx));
	si_indx.si_name_prim.length = si->name.length;
	memcpy(si_indx.si_name_prim.value, si->name.value, si_indx.si_name_prim.length);

	si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_indx, true);

	while (si_dep_rec != NULL) {
		if (m_CMP_HORDER_SANAMET(si_dep_rec->indx_imm.si_name_prim, si_indx.si_name_prim) != 0) {
			/* Seems no more node exists in spons_anchor tree with
			 * "si_indx.si_name_prim" as primary key
			 */
			break;
		}
		dep_si = avd_si_get(&si_dep_rec->indx_imm.si_name_sec);
		if (dep_si == NULL) {
			TRACE("No corresponding SI node?? some thing wrong");
			si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
			continue;
		}
		for (sisu = dep_si->list_of_sisu;sisu;sisu = sisu->si_next) {
			TRACE("sisu si:%s su:%s state:%d fsm_state:%d",sisu->si->name.value,sisu->su->name.value,sisu->state,sisu->fsm);
			if (((sisu->state == SA_AMF_HA_ACTIVE) || (sisu->state == SA_AMF_HA_QUIESCING)
						|| (sisu->state == SA_AMF_HA_QUIESCED))
					&& (sisu->fsm != AVD_SU_SI_STATE_UNASGN)) {

				if(su->su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) {
					if ((m_NCS_MDS_DEST_EQUAL(&sisu->su->su_on_node->adest,&su->su_on_node->adest))) {
						if(((dep_si->si_dep_state != AVD_SI_TOL_TIMER_RUNNING) ||
							(dep_si->si_dep_state != AVD_SI_READY_TO_UNASSIGN) ||
							(dep_si->si_dep_state != AVD_SI_FAILOVER_UNDER_PROGRESS)) &&
							(all_sponsors_assigned_active(dep_si))) {

							si_dep_state_set(dep_si, AVD_SI_FAILOVER_UNDER_PROGRESS);
							if (dep_si->num_dependents > 0) {
								/* This SI also has dependents under it, update their state also */
								avd_update_depstate_si_failover(dep_si, su);
							}
						}
					}
				} else if (dep_si->sg_of_si == si->sg_of_si) {
					if((dep_si->si_dep_state != AVD_SI_TOL_TIMER_RUNNING) ||
						(dep_si->si_dep_state != AVD_SI_READY_TO_UNASSIGN) ||
						(dep_si->si_dep_state != AVD_SI_FAILOVER_UNDER_PROGRESS)) {

						si_dep_state_set(dep_si, AVD_SI_FAILOVER_UNDER_PROGRESS);
						if (dep_si->num_dependents > 0) {
							/* This SI also has dependents under it, update their state also */
							avd_update_depstate_si_failover(dep_si, su);
						}
					}
				}
			} else {
				/* In the case of su fault all the assignmemts on the Active SU will be removed
				 * So we need to consider Standby assignment only for doing role failover
				 */
				if ((su->saAmfSuReadinessState == SA_AMF_READINESS_OUT_OF_SERVICE) &&
						(su->su_on_node->saAmfNodeOperState != SA_AMF_OPERATIONAL_DISABLED)) {

					if ((((sisu->state == SA_AMF_HA_ACTIVE) || (sisu->state == SA_AMF_HA_QUIESCED)) &&
						(sisu->fsm == AVD_SU_SI_STATE_UNASGN)) && (valid_standby_susi(sisu))) {

						/* Before updating the si_dep_state, check the assignment state of all sponsors */
						bool sponsor_assignments_state = true;
						for (spons_si_node = dep_si->spons_si_list;spons_si_node && sponsor_assignments_state == true;
							spons_si_node = spons_si_node->next) {
							if (spons_si_node->si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS)
								continue;
							for (sisu = spons_si_node->si->list_of_sisu;sisu;sisu = sisu->si_next) {
								TRACE("sisu->si:%s state:%u fsm:%u",sisu->si->name.value,sisu->state,sisu->fsm);
								if (((sisu->state == SA_AMF_HA_ACTIVE) || (sisu->state == SA_AMF_HA_QUIESCING) ||
										(sisu->state == SA_AMF_HA_QUIESCED)) &&
										(sisu->fsm != AVD_SU_SI_STATE_UNASGN)){
									break;
								}
								if (sisu->si_next == NULL) {
									sponsor_assignments_state = false;
									break;
								}
							}
						}
						if (sponsor_assignments_state == true) {

							if((dep_si->si_dep_state != AVD_SI_TOL_TIMER_RUNNING) ||
							   (dep_si->si_dep_state != AVD_SI_READY_TO_UNASSIGN) ||
							   (dep_si->si_dep_state != AVD_SI_FAILOVER_UNDER_PROGRESS)) {

								si_dep_state_set(dep_si, AVD_SI_FAILOVER_UNDER_PROGRESS);
								if (dep_si->num_dependents > 0) {
									avd_update_depstate_si_failover(dep_si, su);
								}
							}
						}
					}
				}
			}
		}
		si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
	}
	TRACE_LEAVE();
}

/**
 * @brief       Update the si_dep_state for all the dependent SI's
 *              that are depending on SI's which have active assignments
 *              on this SU
 *
 * @param[in]  su   -
 *
 * @return 
 *             
 **/
void avd_update_depstate_su_rolefailover(AVD_SU *su)
{
	AVD_SU_SI_REL *susi;

	TRACE_ENTER2(" for su '%s'", su->name.value);

	for (susi = su->list_of_susi;susi != NULL;susi = susi->su_next) {
		if (susi->si->num_dependents > 0) {
			/* This is a Sponsor SI update its dependent states */
			avd_update_depstate_si_failover(susi->si, su);
			
		}
	}

	TRACE_LEAVE();
}
/**
 * @brief       Does SI role failover for the dependent SI.
 *
 * @param[in]   si - Dependent SI, for which role failover has to be done
 *
 * @return      Returns nothing
 **/
static void avd_dependentsi_role_failover(AVD_SI *si)
{
	AVD_SU *stdby_su = NULL;
	AVD_SU_SI_REL *susi;

	TRACE_ENTER2(" for SI '%s'", si->name.value);

	switch (si->sg_of_si->sg_redundancy_model) {
	case SA_AMF_2N_REDUNDANCY_MODEL:
	case SA_AMF_NPM_REDUNDANCY_MODEL:
		for (susi = si->list_of_sisu;susi != NULL;susi = susi->si_next) {
			if (susi->state == SA_AMF_HA_STANDBY) {
				stdby_su = susi->su;
				break;
			}
		}
		if (stdby_su) {
			/* Check if there are any dependent SI's on this SU, if so check if their sponsor SI's
			 * are in enable state or not
			 */
			for (susi = stdby_su->list_of_susi;susi != NULL;susi = susi->su_next) {
				if (susi->si->spons_si_list) {
					if (avd_check_si_dep_sponsors(avd_cb, si, false) != NCSCC_RC_SUCCESS) {
						TRACE("SI's sponsors are not yet assigned");
						goto done;
					}
				}
			}
			/* All of the dependent's Sponsors's are in assigned state
			 * So performing role modification for the stdby_su
			 */
			avd_sg_su_si_mod_snd(avd_cb, stdby_su, SA_AMF_HA_ACTIVE);

			si_dep_state_set(si, AVD_SI_ASSIGNED);
			if (si->sg_of_si->su_oper_list.su == NULL) {
				/* add the SU to the operation list and change the SG FSM to SG realign. */
				avd_sg_su_oper_list_add(avd_cb, stdby_su, false);
				m_AVD_SET_SG_FSM(avd_cb, stdby_su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
			}
		}
		break;
	case SA_AMF_N_WAY_REDUNDANCY_MODEL:
		if (avd_check_si_dep_sponsors(avd_cb, si, false) == NCSCC_RC_SUCCESS) {
			/* identify the most preferred standby su for this si */
			susi = avd_find_preferred_standby_susi(si);
			if (susi) {
				avd_susi_mod_send(susi, SA_AMF_HA_ACTIVE);
				m_AVD_SET_SG_FSM(avd_cb, si->sg_of_si, AVD_SG_FSM_SG_REALIGN);
				si_dep_state_set(si, AVD_SI_ASSIGNED);
			}
		}
		break;
	default:
		break;
	}
done:
	TRACE_LEAVE();
}
/**
 * @brief	While node failover is under progress, if su fault happens
 *		this routine resets the si_dep_state to AVD_SI_SPONSOR_UNASSIGNED  
 *
 * @param[in]   si 
 *
 * @return      Returns nothing
 **/
void avd_sidep_reset_dependents_depstate_in_sufault(AVD_SI *si)
{
	AVD_SI_SI_DEP_INDX si_indx;
	AVD_SI_SI_DEP *si_dep_rec;
	AVD_SI *dep_si;

	TRACE_ENTER2(" SI: '%s'",si->name.value);

	memset((char *)&si_indx, '\0', sizeof(si_indx));
	si_indx.si_name_prim.length = si->name.length;
	memcpy(si_indx.si_name_prim.value, si->name.value, si_indx.si_name_prim.length);
	si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_indx, true);

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
			si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
			continue;
		}
		if(dep_si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS) {
			si_dep_state_set(dep_si, AVD_SI_SPONSOR_UNASSIGNED);
		}
		si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
	}

	TRACE_LEAVE();
}

/**
 * @brief	Checks whether failover can be done at SU level or SI level
 *		Per SU level failover cannot be done in the following two cases
 *		1)SI-SI dependency exists across the SI's within SU
 *		2)One of the SI's of SG_1 depends on SI of SG_2 and viceversa 
 *			
 * @param[in]   su 
 *
 * @return	true/false 
 **/
bool avd_si_dependency_exists_within_su(const AVD_SU *su)
{
	AVD_SU_SI_REL *susi;
	bool spons_exist = false;
	bool dep_exist = false;
	TRACE_ENTER();

	for (susi = su->list_of_susi; susi != NULL; susi = susi->su_next) {
		if (susi->si->num_dependents) {
			spons_exist = true;
		}
		if (susi->si->spons_si_list) {
			dep_exist = true;
		}

		if (spons_exist && dep_exist)
			return true;
	}

	return false;
}
/**
 * @brief	Iterates through all the dependants and for each one if all of its
 *		sponsors are assigned sends Active role modification to it	
 *
 * @param[in]   si 
 *
 * @returns 	nothing	
 **/
void send_active_to_dependents(const AVD_SI *si)
{
	AVD_SI_SI_DEP_INDX si_indx;
	AVD_SI_SI_DEP *si_dep_rec;
	AVD_SU_SI_REL *sisu;
	AVD_SI *dep_si;
	AVD_SU *active_su  = NULL;

	TRACE_ENTER2(": '%s'",si->name.value);

	/* determine the active su */
	for (sisu = si->list_of_sisu;sisu != NULL;sisu = sisu->si_next) {
		if ((sisu->state == SA_AMF_HA_ACTIVE) && 
			(sisu->fsm != AVD_SU_SI_STATE_UNASGN)) {
			active_su = sisu->su;
			break;
		}       
	}   

	memset(&si_indx, '\0', sizeof(si_indx));
	si_indx.si_name_prim.length = si->name.length;
	memcpy(si_indx.si_name_prim.value, si->name.value, si_indx.si_name_prim.length);
	si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_indx, true);

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
			si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
			continue;
		}
		TRACE("dependent si:%s dep_si->si_dep_state:%d",dep_si->name.value,dep_si->si_dep_state);
		if(dep_si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS) {
			if (avd_check_si_dep_sponsors(avd_cb, dep_si, false) != NCSCC_RC_SUCCESS) {
				/* Some of the sponsors are not yet in Active state */
				si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
				continue;
			}
			AVD_SU_SI_REL *sisu;
			for (sisu = dep_si->list_of_sisu;sisu != NULL;sisu = sisu->si_next) {
				if (sisu->su == active_su) {
					if (((sisu->state == SA_AMF_HA_STANDBY) ||
						(sisu->state == SA_AMF_HA_QUIESCED)) &&
						(sisu->fsm != AVD_SU_SI_STATE_UNASGN)) {
						avd_susi_mod_send(sisu, SA_AMF_HA_ACTIVE);
						break;
					}
				}
			}
			si_dep_state_set(dep_si, AVD_SI_ASSIGNED);
		}
		si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
	}
	TRACE_LEAVE();
}
/**
 * @brief	Checks if quiesced response came for all of its dependents 	
 *
 * @param[in]   su 
 *
 * @return	true/false 
 **/
bool quiesced_done_for_all_dependents(const AVD_SI *si, const AVD_SU *su)
{
	AVD_SI_SI_DEP_INDX si_indx;
	AVD_SI_SI_DEP *si_dep_rec;
	AVD_SI *dep_si;
	AVD_SU_SI_REL *sisu = NULL;
	bool quiesced = true; 

	TRACE_ENTER2(": '%s'",si->name.value);

	memset(&si_indx, '\0', sizeof(si_indx));
	si_indx.si_name_prim.length = si->name.length;
	memcpy(si_indx.si_name_prim.value, si->name.value, si_indx.si_name_prim.length);
	si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_indx, true);

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
			si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
			continue;
		}
		for (sisu = dep_si->list_of_sisu; sisu ; sisu = sisu->si_next) {
			if ((sisu->su == su) && ((sisu->state != SA_AMF_HA_QUIESCED) || 
						(sisu->fsm != AVD_SU_SI_STATE_ASGND))) {
				quiesced = false;
				goto done;
			}
		}
		si_dep_rec = avd_si_si_dep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
	}

done:
	TRACE_LEAVE2(" :%u",quiesced);
	return quiesced;
}
/**
 * @brief       Checks whether all sponsors of an SI are in assigned state or not 
 *              Even if any one of the sponsor is in unassigned state, this routine
 *              returns NCSCC_RC_FAILURE else NCSCC_RC_SUCCESS
 *
 * @param[in]   SI 
 *
 * @return      NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 **/
bool all_sponsors_assigned_active(AVD_SI *si)
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

