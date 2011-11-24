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

  REVISION HISTORY:

..............................................................................

  DESCRIPTION:

  This module contains timer related routines for the HA FSM.

  FUNCTIONS INCLUDED in this module:
  ncs_mbcsv_start_timer          - Start timer.
  ncs_mbcsv_stop_timer           - Stop timer.
  ncs_mbcsv_stop_all_timers      - Stop all the timers.
  ncs_mbcsv_tmr_expiry           - Function handles timer expiration.
  ncs_mbcsv_send_cold_sync_tmr   - Evt handler for Send cold sync timer expire.
  ncs_mbcsv_send_warm_sync_tmr   - Evt handler for send warm sync timer expire.
  ncs_mbcsv_send_data_req_tmr    - Evt handler for data rsp complete expire.
  ncs_mbcsv_cold_sync_cmplt_tmr  - Evt hdler for cold sync cmplt timer expire.
  ncs_mbcsv_warm_sync_cmplt_tmr  - Evt hdler for warm sync cmplt timer expire.
  ncs_mbcsv_transmit_tmr         - Evt hdler for transmit timer expire.

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

#include "mbcsv.h"

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        The TIMER Database (Visibility restricted)

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

const NCS_MBCSV_TMR_DB ncs_mbcsv_tmr_db[] = {
	{"S_coldsync ", (TMR_CALLBACK)ncs_mbcsv_tmr_expiry, NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC},
	{"S_warmsync ", (TMR_CALLBACK)ncs_mbcsv_tmr_expiry, NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC},
	{"coldsync_C ", (TMR_CALLBACK)ncs_mbcsv_tmr_expiry, NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT},
	{"warmsync_C ", (TMR_CALLBACK)ncs_mbcsv_tmr_expiry, NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT},
	{"asyncdata_C", (TMR_CALLBACK)ncs_mbcsv_tmr_expiry, NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT},
	{"transmit   ", (TMR_CALLBACK)ncs_mbcsv_tmr_expiry, NCSMBCSV_EVENT_TMR_TRANSMIT}
};

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                                TIMER SERVICES

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/***************************************************************************** 

  PROCEDURE          :    ncs_mbcsv_start_timer

  DESCRIPTION:       This function is used to start a NCS_MBCSV timer 

  ARGUMENTS:

  RETURNS:           Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_start_timer(PEER_INST *peer, uns8 timer_type)
{
	NCS_MBCSV_TMR *tmr;

	if (timer_type >= NCS_MBCSV_MAX_TMRS) {
		m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "Timer out of range");
		return;
	}

	tmr = &peer->tmr[timer_type];
	tmr->xdb = (NCS_MBCSV_TMR_HDL)peer;

	/* If timer does not exist, create it. */

	if (tmr->tmr_id == TMR_T_NULL) {
		m_NCS_TMR_CREATE(tmr->tmr_id, tmr->period, ncs_mbcsv_tmr_db[timer_type].cb_func, (void *)tmr);
	}

	/*  If timer is not active, start it. */
	tmr->has_expired = FALSE;	/* if in transit, not valid now */

	if (tmr->is_active == FALSE) {
		tmr->type = timer_type;
		m_LOG_MBCSV_TMR(peer->my_ckpt_inst->my_role,
				peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
				peer->my_ckpt_inst->pwe_hdl, peer->peer_anchor,
				(timer_type + MBCSV_TMR_SND_COLD_SYNC), MBCSV_TMR_START);

		m_NCS_TMR_START(tmr->tmr_id, tmr->period, ncs_mbcsv_tmr_db[timer_type].cb_func, (void *)tmr);

		tmr->is_active = TRUE;

	}
}

/***************************************************************************** 
                                                                              
  PROCEDURE          :    ncs_mbcsv_stop_timer                                          
                                                                               
  DESCRIPTION:       This function is used to stop a NCS_MBCSV timer 
                                                                               
  ARGUMENTS:                                                                   
                                                                               
  RETURNS:           Nothing.                                                 
                                                                               
  NOTES:                                                                
                                                                               
*****************************************************************************/
void ncs_mbcsv_stop_timer(PEER_INST *peer, uns32 timer_type)
{
	NCS_MBCSV_TMR *tmr;

	if (timer_type >= NCS_MBCSV_MAX_TMRS) {
		m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "Timer out of range");;
		return;
	}
	tmr = &peer->tmr[timer_type];

	/* Stop and destroy the timer if it is active... */

	tmr->has_expired = FALSE;	/* if in transit, not valid now */

	if (tmr->is_active == TRUE) {
		m_LOG_MBCSV_TMR(peer->my_ckpt_inst->my_role,
				peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
				peer->my_ckpt_inst->pwe_hdl, peer->peer_anchor,
				(timer_type + MBCSV_TMR_SND_COLD_SYNC), MBCSV_TMR_STOP);

		m_NCS_TMR_STOP(tmr->tmr_id);
		tmr->is_active = FALSE;
		m_NCS_TMR_DESTROY(tmr->tmr_id);
		tmr->tmr_id = TMR_T_NULL;
	} else if (tmr->tmr_id != TMR_T_NULL) {
		/* Destroy the timer if it exists... */

		m_LOG_MBCSV_TMR(peer->my_ckpt_inst->my_role,
				peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
				peer->my_ckpt_inst->pwe_hdl, peer->peer_anchor,
				(timer_type + MBCSV_TMR_SND_COLD_SYNC), MBCSV_TMR_STOP);

		m_NCS_TMR_DESTROY(tmr->tmr_id);
		tmr->tmr_id = TMR_T_NULL;
	}
}

/***************************************************************************** 
                                                                              
  PROCEDURE          :    ncs_mbcsv_stop_all_timers                                          
                                                                               
  DESCRIPTION:       This function is used to stop all NCS_MBCSV timers 
                                                                               
  ARGUMENTS:                                                                   
                                                                               
  RETURNS:           Nothing.                                                 
                                                                               
  NOTES:                                                                
                                                                               
*****************************************************************************/
void ncs_mbcsv_stop_all_timers(PEER_INST *peer)
{
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_SEND_COLD_SYNC);
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_SEND_WARM_SYNC);
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_COLD_SYNC_CMPLT);
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_WARM_SYNC_CMPLT);
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_DATA_RESP_CMPLT);
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_TRANSMIT);
}

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                          TIMER EXPIRATION HANDLERS

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/***************************************************************************** 
                                                                              
  PROCEDURE          :    ncs_mbcsv_tmr_expiry                                          
                                                                               
  DESCRIPTION:       This function is the NCS_MBCSV timer expiration handler.      
                                                                               
  ARGUMENTS:                                                                   
     uarg*:          Opaque pointer to local timer structure.
                                                                               
  RETURNS:           Nothing.                                                 
                                                                               
  NOTES:                                                                
  This function must conform to the prototype for TMR_CALLBACK (sysf_tmr.h)   
                                                                               
*****************************************************************************/

void ncs_mbcsv_tmr_expiry(void *uarg)
{
	NCS_MBCSV_TMR *tmr;
	PEER_INST *peer;
	uns8 type;
	uns8 event;
	MBCSV_EVT *mbc_evt;	/* MBCSV event to be posted */

	if (NULL == (mbc_evt = m_MMGR_ALLOC_MBCSV_EVT)) {
		m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory allocation failure");;
		return;
	}

	/* Extract timer expiration info from uarg. */
	tmr = (NCS_MBCSV_TMR *)uarg;
	peer = (PEER_INST *)tmr->xdb;
	type = tmr->type;
	event = ncs_mbcsv_tmr_db[type].event;

	/* Clean up timer. */
	m_LOG_MBCSV_TMR(peer->my_ckpt_inst->my_role,
			peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			peer->my_ckpt_inst->pwe_hdl, peer->peer_anchor,
			(type + MBCSV_TMR_SND_COLD_SYNC), MBCSV_TMR_EXP);

	m_SET_NCS_MBCSV_TMR_INACTIVE(peer, type);
	m_SET_NCS_MBCSV_TMR_EXP_ON(peer, type);	/* expired and valid at action routine */

	mbc_evt->msg_type = MBCSV_EVT_TMR;
	mbc_evt->info.tmr_evt.type = type;
	mbc_evt->info.tmr_evt.peer_inst_hdl = peer->hdl;

	/* Post the timer event to the ncs_mbcsv process. */
	if (NCSCC_RC_SUCCESS != m_MBCSV_SND_MSG(&peer->my_ckpt_inst->my_mbcsv_inst->mbx,
						mbc_evt, NCS_IPC_PRIORITY_HIGH)) {
		m_MMGR_FREE_MBCSV_EVT(mbc_evt);
		m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "Failed to post message to MBCSv mailbox.");;
		return;
	}
}

/*****************************************************************************

  PROCEDURE          :    ncs_mbcsv_send_cold_sync_tmr

  DESCRIPTION:

    Sends a cold sync req on the timer expiry.

    The first response to the cold sync cancels the send cold sync
    timer since that says the other side is responding and there is
    no need to continually send cold syncs. 
    The final response to the cold sync cancels the cold sync complt
    timer. 

  ARGUMENTS:       
      peer: Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_send_cold_sync_tmr(PEER_INST *peer, MBCSV_EVT *evt)
{
	uns32 rc;

	/* Send a cold sync req  and start the send cold sync timer if the role is */
	/* Standby and the RED is enabled                                           */

	if (peer->my_ckpt_inst->my_role == SA_AMF_HA_STANDBY) {
		m_MBCSV_SEND_CLIENT_MSG(peer, NCSMBCSV_SEND_COLD_SYNC_REQ, NCS_MBCSV_ACT_DONT_CARE);

		/* Even if the message was not sent, we will try again later */
		/* When we get a cold sync response - this timer will be cancelled */
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_SEND_COLD_SYNC);

		/* This timer must be started whenever the cold sync is sent */
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_COLD_SYNC_CMPLT);
	}

	/* Tell the MBCSv clinet about the timer expiration */
	/* It can do smart things - if it cares */
	m_MBCSV_INDICATE_ERROR(peer,
			       peer->my_ckpt_inst->client_hdl,
			       NCS_MBCSV_COLD_SYNC_TMR_EXP, FALSE, peer->version, NULL, rc);

	if (NCSCC_RC_SUCCESS != rc)
		m_MBCSV_DBG_SINK_SVC(NCSCC_RC_FAILURE,
				     "mbcsv_shutdown_peer: Error callback returns fails.",
				     peer->my_ckpt_inst->my_mbcsv_inst->svc_id);

}

/*****************************************************************************

  PROCEDURE          :    ncs_mbcsv_send_warm_sync_tmr

  DESCRIPTION:

    Sends a warm sync req on the timer expiry

  ARGUMENTS:       
      peer: Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_send_warm_sync_tmr(PEER_INST *peer, MBCSV_EVT *evt)
{
	uns32 rc;

	/* Send a warm sync req  and start the send warm sync timer  */
	/* if the role is Standby and the RED is enabled              */

	if (peer->my_ckpt_inst->my_role == SA_AMF_HA_STANDBY) {

		/* Tell the redundant entity about the timer expiration */
		/* It can do smart things - if it cares */
		if (peer->warm_sync_sent == TRUE) {
			m_MBCSV_INDICATE_ERROR(peer, peer->my_ckpt_inst->client_hdl,
					       NCS_MBCSV_WARM_SYNC_TMR_EXP, FALSE, peer->version, NULL, rc);

			if (NCSCC_RC_SUCCESS != rc)
				m_MBCSV_DBG_SINK_SVC(NCSCC_RC_FAILURE,
						     "mbcsv_shutdown_peer: Error callback returns fails.",
						     peer->my_ckpt_inst->my_mbcsv_inst->svc_id);
		}

		m_MBCSV_SEND_CLIENT_MSG(peer, NCSMBCSV_SEND_WARM_SYNC_REQ, NCS_MBCSV_ACT_DONT_CARE);

		peer->warm_sync_sent = TRUE;
		/* Even if the message was not sent, we will try again later */
		/* When we get a warm sync response - this timer will be cancelled */
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_SEND_WARM_SYNC);
		/* This timer must be started whenever the warm sync is sent */
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_WARM_SYNC_CMPLT);
	}

}

/*****************************************************************************

  PROCEDURE          :    ncs_mbcsv_send_data_req_tmr

  DESCRIPTION:

    Sends a data sync req on the timer expiry

  ARGUMENTS:       
      peer: Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_send_data_req_tmr(PEER_INST *peer, MBCSV_EVT *evt)
{
	uns32 rc;
	/* Tell the redundant entity about the timer expiration */
	/* It can do smart things - if it cares */
	m_MBCSV_INDICATE_ERROR(peer, peer->my_ckpt_inst->client_hdl,
			       NCS_MBCSV_DATA_RSP_CMPLT_TMR_EXP, TRUE, peer->version, NULL, rc);

	if (NCSCC_RC_SUCCESS != rc)
		m_MBCSV_DBG_SINK_SVC(NCSCC_RC_FAILURE,
				     "mbcsv_shutdown_peer: Error callback returns fails.",
				     peer->my_ckpt_inst->my_mbcsv_inst->svc_id);

	peer->my_ckpt_inst->data_req_sent = FALSE;

}

/*****************************************************************************

  PROCEDURE          :    ncs_mbcsv_cold_sync_cmplt_tmr

  DESCRIPTION:

  ARGUMENTS:       
      peer: Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_cold_sync_cmplt_tmr(PEER_INST *peer, MBCSV_EVT *evt)
{
	uns32 rc;
	m_MBCSV_INDICATE_ERROR(peer, peer->my_ckpt_inst->client_hdl,
			       NCS_MBCSV_COLD_SYNC_CMPL_TMR_EXP, TRUE, peer->version, NULL, rc);

	if (NCSCC_RC_SUCCESS != rc)
		m_MBCSV_DBG_SINK_SVC(NCSCC_RC_FAILURE,
				     "mbcsv_shutdown_peer: Error callback returns fails.",
				     peer->my_ckpt_inst->my_mbcsv_inst->svc_id);
}

/*****************************************************************************

  PROCEDURE          :   ncs_mbcsv_warm_sync_cmplt_tmr

  DESCRIPTION:

  ARGUMENTS:       
      peer: Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_warm_sync_cmplt_tmr(PEER_INST *peer, MBCSV_EVT *evt)
{
	uns32 rc;
	m_MBCSV_INDICATE_ERROR(peer, peer->my_ckpt_inst->client_hdl,
			       NCS_MBCSV_WARM_SYNC_CMPL_TMR_EXP, TRUE, peer->version, NULL, rc);

	if (NCSCC_RC_SUCCESS != rc)
		m_MBCSV_DBG_SINK_SVC(NCSCC_RC_FAILURE,
				     "mbcsv_shutdown_peer: Error callback returns fails.",
				     peer->my_ckpt_inst->my_mbcsv_inst->svc_id);
}

/*****************************************************************************

  PROCEDURE          :   ncs_mbcsv_transmit_tmr

  DESCRIPTION:

  ARGUMENTS:       
      peer: Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_transmit_tmr(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Sending the call-again-event");

	mbcsv_send_msg(peer, evt, peer->call_again_event);

}
