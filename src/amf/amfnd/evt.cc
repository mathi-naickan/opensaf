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

  This file contains AvND event routines. 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "amf/amfnd/avnd.h"

/****************************************************************************
  Name          : avnd_evt_create
 
  Description   : This routine allocates & populates the AvND event structure.
 
  Arguments     : cb          - ptr to the AvND control block
                  type        - event type
                  mds_ctxt    - ptr to the mds-context (used for response 
                                match by MDS)
                  mds_dest    - ptr to the MDS dest
                  info        - ptr to evt-info
                  clc         - ptr to the CLC event
                  comp_fsm    - ptr to comp-fsm event
 
  Return Values : ptr to the AvND event
 
  Notes         : None.
******************************************************************************/

AVND_EVT *avnd_evt_create(AVND_CB *cb,
			  AVND_EVT_TYPE type,
			  MDS_SYNC_SND_CTXT *mds_ctxt,
			  MDS_DEST *mds_dest, void *info, AVND_CLC_EVT *clc, AVND_COMP_FSM_EVT *comp_fsm)
{
	AVND_EVT *evt = 0;

	/* alloc avnd event */
	evt = new AVND_EVT();

	/* fill the common fields */
	if (mds_ctxt)
		evt->mds_ctxt = *mds_ctxt;
	evt->type = type;
	evt->priority = NCS_IPC_PRIORITY_NORMAL;	/* default priority */

	/* fill the event specific fields */
	switch (type) {
		/* AvD event types */
	case AVND_EVT_AVD_NODE_UP_MSG:
	case AVND_EVT_AVD_REG_SU_MSG:
	case AVND_EVT_AVD_REG_COMP_MSG:
	case AVND_EVT_AVD_INFO_SU_SI_ASSIGN_MSG:
	case AVND_EVT_AVD_PG_TRACK_ACT_RSP_MSG:
	case AVND_EVT_AVD_PG_UPD_MSG:
	case AVND_EVT_AVD_OPERATION_REQUEST_MSG:
	case AVND_EVT_AVD_SU_PRES_MSG:
	case AVND_EVT_AVD_VERIFY_MSG:
	case AVND_EVT_AVD_ACK_MSG:
	case AVND_EVT_AVD_SHUTDOWN_APP_SU_MSG:
	case AVND_EVT_AVD_SET_LEDS_MSG:
	case AVND_EVT_AVD_COMP_VALIDATION_RESP_MSG:
	case AVND_EVT_AVD_ROLE_CHANGE_MSG:
	case AVND_EVT_AVD_ADMIN_OP_REQ_MSG:
	case AVND_EVT_AVD_REBOOT_MSG:
	case AVND_EVT_AVD_COMPCSI_ASSIGN_MSG:
		evt->info.avd = (AVSV_DND_MSG *)info;
		break;

	case AVND_EVT_AVD_HEARTBEAT_MSG:
		evt->priority = NCS_IPC_PRIORITY_HIGH;
		evt->info.avd = (AVSV_DND_MSG *)info;
		break;
		/* AvA event types */
	case AVND_EVT_AVA_FINALIZE:
	case AVND_EVT_AVA_COMP_REG:
	case AVND_EVT_AVA_COMP_UNREG:
	case AVND_EVT_AVA_PM_START:
	case AVND_EVT_AVA_PM_STOP:
	case AVND_EVT_AVA_HC_START:
	case AVND_EVT_AVA_HC_STOP:
	case AVND_EVT_AVA_HC_CONFIRM:
	case AVND_EVT_AVA_CSI_QUIESCING_COMPL:
	case AVND_EVT_AVA_HA_GET:
	case AVND_EVT_AVA_PG_START:
	case AVND_EVT_AVA_PG_STOP:
	case AVND_EVT_AVA_ERR_REP:
	case AVND_EVT_AVA_ERR_CLEAR:
	case AVND_EVT_AVA_RESP:
		evt->info.ava.mds_dest = *mds_dest;
		evt->info.ava.msg = (AVSV_NDA_AVA_MSG *)info;
		break;

		/* timer event types */
	case AVND_EVT_TMR_HC:
	case AVND_EVT_TMR_CBK_RESP:
	case AVND_EVT_TMR_RCV_MSG_RSP:
	case AVND_EVT_TMR_CLC_COMP_REG:
	case AVND_EVT_TMR_SU_ERR_ESC:
	case AVND_EVT_TMR_NODE_ERR_ESC:
	case AVND_EVT_TMR_CLC_PXIED_COMP_INST:
	case AVND_EVT_TMR_CLC_PXIED_COMP_REG:
	case AVND_EVT_TMR_HB_DURATION:
	case AVND_EVT_TMR_SC_ABSENCE:
	case AVND_EVT_TMR_QSCING_CMPL:
		evt->priority = NCS_IPC_PRIORITY_HIGH;	/* bump up the priority */
		evt->info.tmr.opq_hdl = *(uint32_t *)info;
		break;

		/* mds event types */
	case AVND_EVT_MDS_AVD_DN:
		evt->info.mds.node_id = *(uint32_t *)info;
		/* Don't break, continue */
	case AVND_EVT_MDS_AVD_UP:
	case AVND_EVT_MDS_AVND_DN:
	case AVND_EVT_MDS_AVND_UP:
		evt->priority = NCS_IPC_PRIORITY_HIGH;	/* bump up the priority */
		evt->info.mds.mds_dest = *mds_dest;
		break;

		/* HA state event types */
	case AVND_EVT_HA_STATE_CHANGE:
		break;

	case AVND_EVT_MDS_AVA_DN:
		evt->priority = NCS_IPC_PRIORITY_NORMAL;	/* keep priority as normal so that it doesn't */
		evt->info.mds.mds_dest = *mds_dest;	/* supercede 'response' */
		break;

		/* clc event types */
	case AVND_EVT_CLC_RESP:
		memcpy(&evt->info.clc, clc, sizeof(AVND_CLC_EVT));
		osaf_extended_name_alloc(osaf_extended_name_borrow(&clc->comp_name), &evt->info.clc.comp_name);
		break;

		/* AvND-AvND event types */
	case AVND_EVT_AVND_AVND_MSG:
		evt->info.avnd = (AVSV_ND2ND_AVND_MSG *)info;
		break;

		/* internal event types */
	case AVND_EVT_COMP_PRES_FSM_EV:
		memcpy(&evt->info.comp_fsm, comp_fsm, sizeof(AVND_COMP_FSM_EVT));
		osaf_extended_name_alloc(osaf_extended_name_borrow(&comp_fsm->comp_name), &evt->info.comp_fsm.comp_name);
		break;

	case AVND_EVT_LAST_STEP_TERM:
		/* nothing to be copied, evt type should do. */
		break;

	case AVND_EVT_IR:
		/* Only SU name to be copied. */
		osaf_extended_name_alloc(osaf_extended_name_borrow((SaNameT *)info), &evt->info.ir_evt.su_name);
		break;

	case AVND_EVT_PID_EXIT:
		osaf_extended_name_alloc(((AVND_COMP_PM_REC *)info)->comp->name.c_str(), &evt->info.pm_evt.comp_name);
		evt->info.pm_evt.pid = ((AVND_COMP_PM_REC *)info)->pid;
		evt->info.pm_evt.pm_rec = (AVND_COMP_PM_REC *)info;
		break;

	case AVND_EVT_AMFA_MDS_VER_INFO:
		break;
	default:
		delete evt;
		evt = nullptr;
		break;
	}

	if (!evt)
		LOG_ER("AvND event creation failed for evt type %u",type);

	return evt;
}

/****************************************************************************
  Name          : avnd_evt_destroy
 
  Description   : This routine frees AvND event.
 
  Arguments     : evt - ptr to the AvND event
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_evt_destroy(AVND_EVT *evt)
{
	uint32_t type = 0;

	if (!evt)
		return;

	type = evt->type;

	switch (type) {
		/* AvD event types */
	case AVND_EVT_AVD_NODE_UP_MSG:
	case AVND_EVT_AVD_REG_SU_MSG:
	case AVND_EVT_AVD_REG_COMP_MSG:
	case AVND_EVT_AVD_INFO_SU_SI_ASSIGN_MSG:
	case AVND_EVT_AVD_PG_TRACK_ACT_RSP_MSG:
	case AVND_EVT_AVD_PG_UPD_MSG:
	case AVND_EVT_AVD_OPERATION_REQUEST_MSG:
	case AVND_EVT_AVD_SU_PRES_MSG:
	case AVND_EVT_AVD_VERIFY_MSG:
	case AVND_EVT_AVD_ACK_MSG:
	case AVND_EVT_AVD_SHUTDOWN_APP_SU_MSG:
	case AVND_EVT_AVD_SET_LEDS_MSG:
	case AVND_EVT_AVD_COMP_VALIDATION_RESP_MSG:
	case AVND_EVT_AVD_ROLE_CHANGE_MSG:
	case AVND_EVT_AVD_ADMIN_OP_REQ_MSG:
	case AVND_EVT_AVD_HEARTBEAT_MSG:
	case AVND_EVT_AVD_REBOOT_MSG:
	case AVND_EVT_AVD_COMPCSI_ASSIGN_MSG:
		if (evt->info.avd)
			avsv_dnd_msg_free(evt->info.avd);
		break;

	case AVND_EVT_AVND_AVND_MSG:
		if (evt->info.avnd)
			avsv_nd2nd_avnd_msg_free(evt->info.avnd);
		break;
		/* AvA event types */
	case AVND_EVT_AVA_FINALIZE:
	case AVND_EVT_AVA_COMP_REG:
	case AVND_EVT_AVA_COMP_UNREG:
	case AVND_EVT_AVA_PM_START:
	case AVND_EVT_AVA_PM_STOP:
	case AVND_EVT_AVA_HC_START:
	case AVND_EVT_AVA_HC_STOP:
	case AVND_EVT_AVA_HC_CONFIRM:
	case AVND_EVT_AVA_CSI_QUIESCING_COMPL:
	case AVND_EVT_AVA_HA_GET:
	case AVND_EVT_AVA_PG_START:
	case AVND_EVT_AVA_PG_STOP:
	case AVND_EVT_AVA_ERR_REP:
	case AVND_EVT_AVA_ERR_CLEAR:
	case AVND_EVT_AVA_RESP:
		if (evt->info.ava.msg)
			avsv_nda_ava_msg_free(evt->info.ava.msg);
		break;

		/* timer event types */
	case AVND_EVT_TMR_HC:
	case AVND_EVT_TMR_CBK_RESP:
	case AVND_EVT_TMR_RCV_MSG_RSP:
	case AVND_EVT_TMR_CLC_COMP_REG:
	case AVND_EVT_TMR_SU_ERR_ESC:
	case AVND_EVT_TMR_NODE_ERR_ESC:
	case AVND_EVT_TMR_CLC_PXIED_COMP_INST:
	case AVND_EVT_TMR_CLC_PXIED_COMP_REG:
	case AVND_EVT_TMR_HB_DURATION:
	case AVND_EVT_TMR_SC_ABSENCE:
	case AVND_EVT_TMR_QSCING_CMPL:
		break;

		/* mds event types */
	case AVND_EVT_MDS_AVD_UP:
	case AVND_EVT_MDS_AVD_DN:
	case AVND_EVT_MDS_AVA_DN:
	case AVND_EVT_MDS_AVND_DN:
	case AVND_EVT_MDS_AVND_UP:
		break;

		/* HA state event types */
	case AVND_EVT_HA_STATE_CHANGE:
		break;

		/* clc event types */
	case AVND_EVT_CLC_RESP:
		osaf_extended_name_free(&evt->info.clc.comp_name);
		break;

		/* internal event types */
	case AVND_EVT_COMP_PRES_FSM_EV:
		osaf_extended_name_free(&evt->info.comp_fsm.comp_name);
		break;

		/* last step of termination */
	case AVND_EVT_LAST_STEP_TERM:
		break;

		/* PID exist event */
	case AVND_EVT_PID_EXIT:
		osaf_extended_name_free(&evt->info.pm_evt.comp_name);
		break;

		/* Imm Reader event */
	case AVND_EVT_IR:
		osaf_extended_name_free(&evt->info.ir_evt.su_name);
		break;
	case AVND_EVT_AMFA_MDS_VER_INFO:
		break;

	default:
		LOG_NO("%s: unknown event type %u", __FUNCTION__, type);
		break;
	}

	/* free the avnd event */
	delete evt;

	return;
}

/****************************************************************************
  Name          : avnd_evt_send
 
  Description   : This routine enqueues the AvND event to the AvND mailbox.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_send(AVND_CB *cb, AVND_EVT *evt)
{
	uint32_t rc = m_NCS_IPC_SEND(&cb->mbx, evt, evt->priority);
	if (NCSCC_RC_SUCCESS != rc)
		LOG_CR("AvND send event to mailbox failed, type = %u", evt->type);

	return rc;
}

/****************************************************************************\
 * Function: avnd_evt_ha_state_change
 *
 * Purpose:  Takes cares state change of HA State.
 *
 * Input: cb        - AVND control block pointer.
 *        evt - ptr to the AvND event
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 *
\**************************************************************************/
uint32_t avnd_evt_ha_state_change_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_HA_STATE_CHANGE_EVT *ha_state_event = nullptr;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER();

	ha_state_event = &evt->info.ha_state_change;

	if (nullptr == ha_state_event)
		return rc;

	if (AVND_EVT_HA_STATE_CHANGE != evt->type)
		goto error;

	if ((SA_AMF_HA_QUIESCED == ha_state_event->ha_state) && (true == cb->is_quisced_set)) {
		cb->avail_state_avnd = SA_AMF_HA_QUIESCED;
		cb->is_quisced_set = false;
		return rc;
	}

error:
	TRACE("evt_type:%u, ha_state:%u ,cb->is_quisced_set:%u, rc:%u are",evt->type, ha_state_event->ha_state, cb->is_quisced_set, rc);

	TRACE_LEAVE();
	return rc;
}
