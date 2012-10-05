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

  This file includes the routines to handle the last step of terminating the 
  node.
 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/
#include "avnd.h"
#include "nid_api.h"

extern const AVND_EVT_HDLR g_avnd_func_list[AVND_EVT_MAX];

/****************************************************************************
  Name          : avnd_evt_last_step_clean
 
  Description   : The last step term is done or failed. Now we use brute force
                  to clean.
  
                  This routine processes clean up of all SUs (NCS/APP if exist).
                  Even if the clean up of all components is not successful, 
                  We still go ahead and free the DB records coz this is 
                  last step anyway.
 
  Arguments     : cb  - ptr to the AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : All the errors are ignored and brute force is employed.

******************************************************************************/
void avnd_last_step_clean(AVND_CB *cb)
{
	AVND_COMP *comp;
	int cleanup_call_cnt = 0;

	TRACE_ENTER();

	LOG_NO("Terminating all AMF components");

	comp = (AVND_COMP *)ncs_patricia_tree_getnext(&cb->compdb, (uint8_t *)0);
	while (comp != NULL) {
		if (false == comp->su->su_is_external) {
			/*
			** If there is a single comp in failed termination or instantiation state
			** stopping OpenSAF has failed.
			*/
			if (comp->pres == SA_AMF_PRESENCE_TERMINATION_FAILED) {
				LOG_ER("%s in termination failed state", comp->name.value);
				opensaf_reboot(avnd_cb->node_info.nodeId, (char *)avnd_cb->node_info.executionEnvironment.value,
						"Stopping OpenSAF failed");
				LOG_ER("Amfnd is exiting (due to comp term failed) to aid fast reboot");
				exit(0);
			}

			if (comp->pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED) {
				LOG_ER("%s in instantiation failed state", comp->name.value);
				opensaf_reboot(avnd_cb->node_info.nodeId, (char *)avnd_cb->node_info.executionEnvironment.value,
						"Stopping OpenSAF failed");
				LOG_ER("Amfnd is exiting (due to comp int failed) to aid fast reboot");
				exit(0);
			}

			/* Don't call cleanup script for PI/NPI components in UNINSTANTIATED state.*/
			if (comp->pres != SA_AMF_PRESENCE_UNINSTANTIATED) {
				avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
				cleanup_call_cnt++;
			}

			/* make avnd_err_process() a nop, will be called due to ava mds down */
			comp->admin_oper = SA_TRUE;
		}

		comp = (AVND_COMP *)
		    ncs_patricia_tree_getnext(&cb->compdb, (uint8_t *)&comp->name);
	}

	/* Stop was called early or some other problem */
	if (cleanup_call_cnt == 0) {
		LOG_NO("No component to terminate, exiting");
		exit(0);
	}

	TRACE_LEAVE();
}

/****************************************************************************
  Name          : avnd_evt_last_step_term
 
  Description   : 1. Send a message to AVD about the signal.
                  2. Do the cleanup of the nodes
                  3. Call the NID API informing the completion of cleanup.
  
                  This routine processes clean up of all SUs (NCS/APP if exist).
                  Even if the clean up of all components is not successful, We still
                  go ahead and free the DB records coz this is last step 
                  anyway.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : All the errors are ignored and brute force deletes are
                  employed. There might be an issue with ASSERT in SU_REC_DEL
                  However, it doesnt matter coz this is during the last step
                  and NID script will timeout and kill anyway.
******************************************************************************/
uint32_t avnd_evt_last_step_term_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_SU_SI_REC *si;
	uint32_t sirank;
	bool si_removed = false;

	TRACE_ENTER();

	cb->term_state = AVND_TERM_STATE_OPENSAF_SHUTDOWN_INITIATED;

	/* Ensure all assignments are in stable state */
	for (si = avnd_silist_getfirst(); si; si = avnd_silist_getnext(si)) {
		if (si->su->is_ncs || si->su->su_is_external)
			continue;

		if ((si->curr_assign_state == AVND_SU_SI_ASSIGN_STATE_ASSIGNING) ||
		    (si->curr_assign_state == AVND_SU_SI_ASSIGN_STATE_REMOVING)) {
			LOG_NO("Waiting for '%s' (state %u)", si->name.value, si->curr_assign_state);
			goto done;
		}
	}

	/* The SI list sorted by SI rank. Rank correspond to SI dependencies */
	si = avnd_silist_getlast();
	if (si)
		sirank = si->rank;
	else
		goto cleanup_components;

	cb->term_state = AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED;
	LOG_NO("Removing assignments from AMF components");

	/* Remove all assignments of equal rank */
	for (; (si != NULL) && (si->rank == sirank); ) {
		AVND_SU_SI_REC *tmp;

		/* Remove assignments only for local application SUs */
		if (si->su->is_ncs || si->su->su_is_external)
			continue;

		if (si->curr_assign_state == AVND_SU_SI_ASSIGN_STATE_REMOVED)
			continue;

		si_removed = true;
		tmp = avnd_silist_getprev(si);
		uint32_t rc = avnd_su_si_remove(cb, si->su, si);
		osafassert(rc == NCSCC_RC_SUCCESS);
		si = tmp;
	}

cleanup_components:
	if (!si_removed)
		avnd_last_step_clean(cb);
done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : avnd_check_su_shutdown_done
 
  Description   : Call the NID API informing the completion of init.
  
 
  Arguments     : cb  - ptr to the AvND control block
                  is_ncs - boolean to indicate if the SU terminated is
                           an NCS SU.
 
  Return Values : None.
 
  Notes         : 
******************************************************************************/
void avnd_check_su_shutdown_done(AVND_CB *cb, bool is_ncs)
{
	AVND_SU *su = 0;
	TRACE_ENTER();

	su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)0);

	/* scan SU term by PRES_STATE FSM on each su */
	while (su != 0) {
		if (su->is_ncs != is_ncs) {
			su = (AVND_SU *)
			    ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)&su->name);
			continue;
		}

		/* Check the state of the SU if they are in final state */
		if ((su->pres != SA_AMF_PRESENCE_UNINSTANTIATED) &&
		    (su->pres != SA_AMF_PRESENCE_INSTANTIATION_FAILED)
		    && (su->pres != SA_AMF_PRESENCE_TERMINATION_FAILED)) {
			/* There are still some SUs to be terminated. We are not done 
			 ** so just return */
			return;
		}
		su = (AVND_SU *)
		    ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)&su->name);
	}

	if (is_ncs == true) {
		/* All NCS SUs have finished their termination. Now call the 
		 ** cleanup of CB
		 */
		LOG_NO("%s: exiting", __FUNCTION__);
		exit(0);
	} else {
		/* No SUs to be processed for termination.
		 ** send the response message to AVD informing DONE. 
		 */
		cb->term_state = AVND_TERM_STATE_SHUTTING_APP_DONE;
		avnd_snd_shutdown_app_su_msg(cb);
	}
	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : avnd_evt_avd_set_leds_msg
 
  Description   : Call the NID API informing the completion of init.
  
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : 
******************************************************************************/
uint32_t avnd_evt_avd_set_leds_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_SET_LEDS_MSG_INFO *info = &evt->info.avd->msg_info.d2n_set_leds;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	avnd_msgid_assert(info->msg_id);
	cb->rcv_msg_id = info->msg_id;

	if (cb->led_state == AVND_LED_STATE_GREEN) {
		/* Nothing to be done we have already got this msg */
		goto done;
	}

	cb->led_state = AVND_LED_STATE_GREEN;

	/* Notify the NIS script/deamon that we have fully come up */
	nid_notify("AMFND", NCSCC_RC_SUCCESS, NULL);

done:
	TRACE_LEAVE();
	return rc;
}
