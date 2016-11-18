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

  DESCRIPTION:

  This file contains functions to handle the events received from the peer. On 
  receiving the PEER message MBCA will post this message to correspondig service's
  mailbox. On dispatch, following functions will get called to process the messages
  received from the peer.

  ..............................................................................

  FUNCTIONS INCLUDED in this module:

   mbcsv_search_and_return_peer         - Finds peer in the peer list.
   mbcsv_add_new_peer                   - Add new peer in the peer list.
   mbcsv_shutdown_peer                  - Close session with the peer.
   mbcsv_rmv_peer                       - Remove peer from peer list.
   mbcsv_empty_peers_list               - Empty peer list.
   mbcsv_process_peer_discovery_message -  Process peer discovery message.
   mbcsv_my_active_peer                 - Find Active peer.
   mbcsv_clear_multiple_active_state    - Clear multiple active state.
   mbcsv_close_old_session              - Close old sessions.
   mbcsv_set_up_new_session             - Setup session with the active.
   mbcsv_set_peer_state                 - Set peer state.
   mbcsv_process_peer_up_info           - Process peer info.
   mbcsv_update_peer_info               - Update peer info value.
   mbcsv_process_peer_down              - Process peer down message.
   mbcsv_process_peer_info_rsp          - Process peer info responce.
   mbcsv_process_peer_chg_role          - Process change role 
   mbcsv_send_peer_disc_msg             - Send peer discovery message.

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/
#include "mbcsv.h"
#include "base/ncssysf_mem.h"

static const char *disc_trace[] = {
			"Peer UP msg",
			"Peer DOWN msg",
			"Peer INFO msg",
			"Peer INFO resp msg",
			"Peer Role change msg"
			"Invalid peer discovery msg"
			};
				 

/**************************************************************************\
* PROCEDURE: mbcsv_search_and_return_peer
*
* Purpose:  This function searches entire MBCA peer list for the 
*           peer entity and returns peer pointer.
*
* Input:    peer_list - MBCSv peer list.
*           anchor - Anchor value of the peer to be serched in the list.
*
* Returns:  Peer instance pointer.
*
* Notes:  
*
\**************************************************************************/
PEER_INST *mbcsv_search_and_return_peer(PEER_INST *peer_list, MBCSV_ANCHOR anchor)
{
	PEER_INST *peer;

	for (peer = peer_list; peer != NULL; peer = peer->next)
		if (peer->peer_anchor == anchor)
			return peer;

	return NULL;
}

/**************************************************************************\
* PROCEDURE: mbcsv_add_new_peer
*
* Purpose:  This function adds new peer in the peer list.
*
* Input:    peer_list - MBCSv peer list.
*           new_peer - new peer to be added on the peer list.
*
* Returns:  Success or Failure.
*
* Notes:  
*
\**************************************************************************/
PEER_INST *mbcsv_add_new_peer(CKPT_INST *ckpt, MBCSV_ANCHOR anchor)
{
	PEER_INST *peer_ptr = ckpt->peer_list;
	PEER_INST *new_peer;
	TRACE_ENTER2("Peer Anchor :%" PRIu64, anchor);

	if (NULL == (new_peer = m_MMGR_ALLOC_PEER_INFO)) {
		TRACE_4("mbcsv_add_new_peer: Mem alloc failed.");
		return NULL;
	}

	memset(new_peer, '\0', sizeof(PEER_INST));

	new_peer->peer_anchor = anchor;
	new_peer->state = NCS_MBCSV_INIT_STATE_IDLE;

	TRACE("peer state : IDLE");
	if (peer_ptr == NULL) {
		ckpt->peer_list = new_peer;
		new_peer->next = NULL;
	} else {
		new_peer->next = ckpt->peer_list;
		ckpt->peer_list = new_peer;
	}

	TRACE("setting up the timers for this checkpoint");
	/* Setup timers */
	m_INIT_NCS_MBCSV_TMR(new_peer, NCS_MBCSV_TMR_SEND_COLD_SYNC, NCS_MBCSV_TMR_SEND_COLD_SYNC_PERIOD);

	m_INIT_NCS_MBCSV_TMR(new_peer, NCS_MBCSV_TMR_SEND_WARM_SYNC, ckpt->warm_sync_time);

	m_INIT_NCS_MBCSV_TMR(new_peer, NCS_MBCSV_TMR_COLD_SYNC_CMPLT, NCS_MBCSV_TMR_COLD_SYNC_CMPLT_PERIOD);

	m_INIT_NCS_MBCSV_TMR(new_peer, NCS_MBCSV_TMR_WARM_SYNC_CMPLT, NCS_MBCSV_TMR_WARM_SYNC_CMPLT_PERIOD);

	m_INIT_NCS_MBCSV_TMR(new_peer, NCS_MBCSV_TMR_DATA_RESP_CMPLT, NCS_MBCSV_TMR_DATA_RESP_CMPLT_PERIOD);

	m_INIT_NCS_MBCSV_TMR(new_peer, NCS_MBCSV_TMR_TRANSMIT, NCS_MBCSV_TMR_TRANSMIT_PERIOD);

	new_peer->warm_sync_sent = false;
	new_peer->cold_sync_done = false;
	new_peer->data_resp_process = false;
	new_peer->c_syn_resp_process = false;
	new_peer->w_syn_resp_process = false;

	TRACE_LEAVE();
	return new_peer;
}

/**************************************************************************\
* PROCEDURE: mbcsv_shutdown_peer
*
* Purpose:  This function stop the peer timers and closes its session.
*
* Input:    peer_ptr - MBCSv peer pointer
*
* Returns:  Success or Failure.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_shutdown_peer(PEER_INST *peer_ptr)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("stop peer timers, close session for peer anchor = %" PRIu64, peer_ptr->peer_anchor);
	ncs_mbcsv_stop_all_timers(peer_ptr);

	/* 
	 * Check if my role is active and I am in middle of data response
	 * with this peer then give error indication callback to clear contxt.
	 */
	if (SA_AMF_HA_ACTIVE == peer_ptr->my_ckpt_inst->my_role) {
		if (true == peer_ptr->data_resp_process) {
			TRACE("I'm ACTIVE and was in the middle of data response, \
							 hence invoking error indication calback");
			m_MBCSV_INDICATE_ERROR(peer_ptr, peer_ptr->my_ckpt_inst->client_hdl,
					       NCS_MBCSV_DATA_RESP_TERMINATED, false,
					       peer_ptr->version, peer_ptr->req_context, rc);
		} else if (true == peer_ptr->c_syn_resp_process) {
			TRACE("I'm ACTIVE and was in the middle of cold sync response, \
							hence invoking error indication calback");
			m_MBCSV_INDICATE_ERROR(peer_ptr, peer_ptr->my_ckpt_inst->client_hdl,
					       NCS_MBCSV_COLD_SYNC_RESP_TERMINATED, false,
					       peer_ptr->version, peer_ptr->req_context, rc);
		} else if (true == peer_ptr->w_syn_resp_process) {
			TRACE("I'm ACTIVE and was in the middle of warm sync response, \
							hence invoking error indication calback");
			m_MBCSV_INDICATE_ERROR(peer_ptr, peer_ptr->my_ckpt_inst->client_hdl,
					       NCS_MBCSV_WARM_SYNC_RESP_TERMINATED, false,
					       peer_ptr->version, peer_ptr->req_context, rc);
		}
	}

	if (NCSCC_RC_SUCCESS != rc)
		TRACE_2("Error Indication callback returned failure for service Id = %u", \
				     peer_ptr->my_ckpt_inst->my_mbcsv_inst->svc_id);

	m_MBCSV_DESTROY_HANDLE(peer_ptr->hdl);
	m_MMGR_FREE_PEER_INFO(peer_ptr);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
* PROCEDURE: mbcsv_rmv_peer
*
* Purpose:  This function removes peer instance from the peer list.
*
* Input:    peer_list - MBCSv peer list.
*           anchor - Anchor value of the peer to be serched in the list.
*
* Returns:  Success or Failure.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_rmv_peer(CKPT_INST *ckpt, MBCSV_ANCHOR anchor)
{
	PEER_INST *last_ptr = NULL, *this_ptr = ckpt->peer_list;
	TRACE_ENTER();

	while (this_ptr != NULL) {
		if (this_ptr->peer_anchor == anchor) {
			if (last_ptr == NULL)
				ckpt->peer_list = this_ptr->next;
			else
				last_ptr->next = this_ptr->next;

			return mbcsv_shutdown_peer(this_ptr);
		}

		last_ptr = this_ptr;
		this_ptr = last_ptr->next;
	}

	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/**************************************************************************\
* PROCEDURE: mbcsv_empty_peers_list
*
* Purpose:  This function removes all peer instance from the peer list.
*
* Input:    ckpt - Checkpoint instance of which peer list has to emptied.
*
* Returns:  Success or Failure.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_empty_peers_list(CKPT_INST *ckpt)
{
	PEER_INST *this_ptr = NULL, *next_ptr = ckpt->peer_list;
	TRACE_ENTER();

	while (NULL != next_ptr) {
		this_ptr = next_ptr;
		next_ptr = this_ptr->next;

		mbcsv_shutdown_peer(this_ptr);

	}

	ckpt->peer_list = NULL;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_process_peer_discovery_message
*
*  DESCRIPTION:       This function processes the peer discovery messages 
*                     received from its peer.
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to process PEER_UP.
*
*****************************************************************************/
uint32_t mbcsv_process_peer_discovery_message(MBCSV_EVT *msg, MBCSV_REG *mbc_reg)
{
	CKPT_INST *ckpt;
	TRACE_ENTER();

	if (NULL != (ckpt = (CKPT_INST *)ncs_patricia_tree_get(&mbc_reg->ckpt_ssn_list,
							       (const uint8_t *)&msg->rcvr_peer_key.pwe_hdl))) {
		switch (msg->info.peer_msg.info.peer_disc.msg_sub_type) {
		case MBCSV_PEER_UP_MSG:
			TRACE("peer version: %hu", msg->info.peer_msg.info.peer_disc.info.peer_up.peer_version);
			mbcsv_process_peer_up_info(msg, ckpt, true);
			break;

		case MBCSV_PEER_DOWN_MSG:
			mbcsv_process_peer_down(msg, ckpt);
			break;

		case MBCSV_PEER_INFO_MSG:
			TRACE("peer version: %hu", msg->info.peer_msg.info.peer_disc.info.peer_info.peer_version);
			mbcsv_process_peer_up_info(msg, ckpt, false);
			break;

		case MBCSV_PEER_INFO_RSP_MSG:
			TRACE("peer version: %hu", msg->info.peer_msg.info.peer_disc.info.peer_info_rsp.peer_version);
			mbcsv_process_peer_info_rsp(msg, ckpt);
			break;

		case MBCSV_PEER_CHG_ROLE_MSG:
			TRACE("eer version: %hu", msg->info.peer_msg.info.peer_disc.info.peer_chg_role.peer_version);
			mbcsv_process_peer_chg_role(msg, ckpt);
			break;

		default:
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
	TRACE_1("%s, My role: %u, My svc_id: %u, pwe handle:%u, peer role:%u, peer_anchor: %" PRIu64, \
	disc_trace[msg->info.peer_msg.info.peer_disc.msg_sub_type], ckpt->my_role, ckpt->my_mbcsv_inst->svc_id, \
				ckpt->pwe_hdl, msg->info.peer_msg.info.peer_disc.peer_role, msg->rcvr_peer_key.peer_anchor);
	} else {
			TRACE_LEAVE2("Unable to find checkpoint for svc_id = %u", mbc_reg->svc_id);
			return NCSCC_RC_FAILURE;
		}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
*
*  PROCEDURE:         mbcsv_my_active_peer
*
*  DESCRIPTION:       This function finds the ACTIVE PEER.
*
*  RETURNS:           NONE
*
\*****************************************************************************/
PEER_INST *mbcsv_my_active_peer(CKPT_INST *ckpt)
{
	PEER_INST *peer;
	TRACE_ENTER();

	/* 
	 * First check whether any other ACTIVE peer still exist.
	 */
	peer = ckpt->peer_list;

	while (peer != NULL) {
		if (peer->peer_role == SA_AMF_HA_ACTIVE)
			break;
		peer = peer->next;
	}

	TRACE_LEAVE();
	return peer;
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_clear_multiple_active_state
*
*  DESCRIPTION:       This function sets the first state of the peer session FSM.
*                     Then depending on its current role it will start FSM.
*
*  RETURNS:           NONE
*
\*****************************************************************************/
void mbcsv_clear_multiple_active_state(CKPT_INST *ckpt)
{
	PEER_INST *peer;
	MBCSV_EVT rcvd_evt;
	TRACE_ENTER();

	/* 
	 * First check whether any other ACTIVE peer still exist.
	 * If YES then, set active peer as this peer and do not clear multiple 
	 * active state. 
	 */
	if (NULL != (peer = mbcsv_my_active_peer(ckpt))) {
		ckpt->active_peer = peer;
		peer = ckpt->peer_list;
		TRACE("multiple ACTIVE peers");

		m_NCS_MBCSV_FSM_DISPATCH(peer, NCSMBCSV_EVENT_MULTIPLE_ACTIVE, &rcvd_evt);
		
		TRACE_LEAVE();
		return;
	}

	/* 
	 * There is no ACTIVE peer now so clear multiple ACTIVE state.
	 */
	peer = ckpt->peer_list;
	ckpt->active_peer = NULL;

	while (peer != NULL) {
		if (peer->incompatible)
			m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_IDLE);
		else {
			if (peer->cold_sync_done)
				m_NCS_MBCSV_FSM_DISPATCH(peer, NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC, &rcvd_evt);
			else
				m_NCS_MBCSV_FSM_DISPATCH(peer, NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC, &rcvd_evt);
		}

		peer = peer->next;
	}

	TRACE_LEAVE();
	return;
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_close_old_session
*
*  DESCRIPTION:       This function closes the session between standby and the
*                     ACTIVE peer..
*
*  RETURNS:           NONE
*
\*****************************************************************************/
void mbcsv_close_old_session(PEER_INST *active_peer)
{
	TRACE_ENTER();

	if (NULL != active_peer) {
		ncs_mbcsv_stop_all_timers(active_peer);

		m_SET_NCS_MBCSV_STATE(active_peer, NCS_MBCSV_STBY_STATE_IDLE);
	}
	TRACE_LEAVE();
	return;
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_set_up_new_session
*
*  DESCRIPTION:       This function starts new session with the new ACTIVE peer.
*
*  RETURNS:           NONE
*
\*****************************************************************************/
void mbcsv_set_up_new_session(CKPT_INST *ckpt, PEER_INST *new_act_peer)
{
	TRACE_ENTER2("peer adest : %" PRIx64, new_act_peer->peer_adest);

	if (new_act_peer->incompatible)
		m_SET_NCS_MBCSV_STATE(new_act_peer, NCS_MBCSV_STBY_STATE_IDLE);
	else if (ckpt->my_role == SA_AMF_HA_QUIESCED) {
		TRACE("my role is quiesced");
		m_SET_NCS_MBCSV_STATE(new_act_peer, NCS_MBCSV_STBY_STATE_STEADY_IN_SYNC);
	} else if (new_act_peer->cold_sync_done) {
		TRACE("cold sync done, sending warm sync request");
		/* Send Warm sync req and set FSM state to wait to warm sync */
		m_MBCSV_SEND_CLIENT_MSG(new_act_peer, NCSMBCSV_SEND_WARM_SYNC_REQ, NCS_MBCSV_ACT_DONT_CARE);

		new_act_peer->warm_sync_sent = true;

		/* When we get a warm sync response - this timer will be cancelled */
		ncs_mbcsv_start_timer(new_act_peer, NCS_MBCSV_TMR_SEND_WARM_SYNC);
		/* This timer must be started whenever the warm sync is sent */
		ncs_mbcsv_start_timer(new_act_peer, NCS_MBCSV_TMR_WARM_SYNC_CMPLT);

		m_SET_NCS_MBCSV_STATE(new_act_peer, NCS_MBCSV_STBY_STATE_WAIT_TO_WARM_SYNC);
	} else {
		TRACE("sending cold sync request");
		/* Send Cold sync req and set FSM state to wait to cold sync */
		m_MBCSV_SEND_CLIENT_MSG(new_act_peer, NCSMBCSV_SEND_COLD_SYNC_REQ, NCS_MBCSV_ACT_DONT_CARE);

		/* When we get a cold sync response - this timer will be cancelled */
		ncs_mbcsv_start_timer(new_act_peer, NCS_MBCSV_TMR_SEND_COLD_SYNC);

		/* This timer must be started whenever the cold sync is sent */
		ncs_mbcsv_start_timer(new_act_peer, NCS_MBCSV_TMR_COLD_SYNC_CMPLT);

		m_SET_NCS_MBCSV_STATE(new_act_peer, NCS_MBCSV_STBY_STATE_WAIT_TO_COLD_SYNC);
	}

	TRACE_LEAVE();
	return;
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_set_peer_state
*
*  DESCRIPTION:       This function sets the first state of the peer session FSM.
*                     Then depending on its current role it will start FSM.
*
*  INPUT              ckpt - Checkpoint instance.
*                     peer - Peer Instance.
*                     peer_up - true implies  PEER_UP processing 
*                             - false implies PEER_INFO processing
*  RETURNS:           NONE
*
\*****************************************************************************/
void mbcsv_set_peer_state(CKPT_INST *ckpt, PEER_INST *peer, bool peer_up)
{
	PEER_INST *peer_ptr = NULL;
	TRACE_ENTER2("peer adest: %" PRIx64, peer->peer_adest);

	switch (ckpt->my_role) {
	case SA_AMF_HA_ACTIVE:	/* ckpt->my_role */
		{
			TRACE("I'm ACTIVE");
			if (peer_up) {
				m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_IDLE);
				TRACE_LEAVE();
				return;
			}

			switch (peer->peer_role) {
				/* 
				 * If new peer's role is also ACTIVE  then
				 * go to MULTIPLE ACTIVE state else go to IDLE state
				 */
			case SA_AMF_HA_ACTIVE:	/*: peer->peer_role */
				{
					TRACE("peer is ACTIVE");
					ckpt->active_peer = peer;

					m_NCS_MBCSV_FSM_DISPATCH(peer, NCSMBCSV_EVENT_MULTIPLE_ACTIVE, NULL);
				}
				break;

				/* 
				 * If new peer is standby then set the state of new peer to wait for cold
				 * sync if peer is compatible else set to IDLE state.
				 */
			case SA_AMF_HA_STANDBY:	/* : peer->peer_role */
			case SA_AMF_HA_QUIESCED:
				{
					if ((NULL != ckpt->active_peer) &&
					    (peer->peer_anchor == ckpt->active_peer->peer_anchor)) {
						mbcsv_clear_multiple_active_state(ckpt);
					} else {
						if (peer->incompatible)
							m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_IDLE);
						else {
							if (peer->cold_sync_done)
								m_SET_NCS_MBCSV_STATE(peer,
										      NCS_MBCSV_ACT_STATE_KEEP_STBY_IN_SYNC);
							else
								m_SET_NCS_MBCSV_STATE(peer,
										      NCS_MBCSV_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC);
						}
					}
				}
				break;

			default:	/* peer->peer_role */
				TRACE_LEAVE2("Invalid PEER state.");
				return;
			}
		}
		break;

	case SA_AMF_HA_STANDBY:	/* ckpt->my_role */
	case SA_AMF_HA_QUIESCED:	/* ckpt->my_role */
		{
			if (peer_up) {
				m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_IDLE);
				TRACE_LEAVE();
				return;
			}

			switch (peer->peer_role) {
				/* If new peer's role is ACTIVE then establish session with this new peer
				   and close old session */
			case SA_AMF_HA_ACTIVE:	/* peer->peer_role */
				{
					TRACE("peer is ACTIVE, clear old session and start new session");
					mbcsv_close_old_session(ckpt->active_peer);

					ckpt->active_peer = peer;
					mbcsv_set_up_new_session(ckpt, peer);
				}
				break;

				/* 
				 * Peer's role is STANDBY/QUIESCED.
				 */
			case SA_AMF_HA_STANDBY:	/* peer->peer_role */
			case SA_AMF_HA_QUIESCED:	/* peer->peer_role */
				{
					if ((NULL != ckpt->active_peer) &&
					    (peer->peer_anchor == ckpt->active_peer->peer_anchor)) {
						/* Our Active peer is changing state, close its session */
						mbcsv_close_old_session(ckpt->active_peer);

						/* Check if any other peer is ACTIVE if yes then set-up session
						 * with this new peer. */
						if (NULL != (peer_ptr = mbcsv_my_active_peer(ckpt))) {
							ckpt->active_peer = peer_ptr;
							mbcsv_set_up_new_session(ckpt, peer_ptr);
						} else
							ckpt->active_peer = NULL;
					} else
						m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_IDLE);
					TRACE_LEAVE();
					return;
				}
				break;

			default:	/* peer->peer_role */
				TRACE_LEAVE2("Invalid PEER state.");
				return;
			}

		}
		break;

	default:
		TRACE_LEAVE2("Invalid HA state.");
		return;

	}
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_process_peer_up
*
*  DESCRIPTION:       This function processes the PEER_UP message received from
*                     its peer client. 
*
*  ACTION:            Actions to be taken on receiving PEER_UP are:
*                     1) Check whether this peer already exist. If YES then drop
*                        this message. Else add this peer in peer list.
*                     2) Send back PEER_INFO message to the peer from which PEER_UP
*                        is received.
*                     3) Call PEER_INFO call back which will validate the peer version.
*                     4) Initiate a role based actions to set-up a session.
*
*  INPUT              msg  - MBCSv peer message.
*                     ckpt - Checkpoint Instance.
*                     peer_up - true implies  PEER_UP processing 
*                             - false implies PEER_INFO processing
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to process PEER_UP.
*
*****************************************************************************/
uint32_t mbcsv_process_peer_up_info(MBCSV_EVT *msg, CKPT_INST *ckpt, uint8_t peer_up)
{
	PEER_INST *peer;
	MBCSV_PEER_KEY key;
	SYSF_MBX mbx;
	MBCSV_EVT *evt;

	TRACE_ENTER();

	/*
	 * Check If this peer up message is not mine. If it my peer up I don't need
	 * to process this mesage. Also check that my role is set. If no then drop 
	 * this request and wait for client to set my role.
	 */
	if ((ckpt->my_anchor == msg->rcvr_peer_key.peer_anchor) || (ckpt->role_set == false)) {
		TRACE_LEAVE2("peer up message is for self");
		return NCSCC_RC_SUCCESS;
	}

	if (NULL != (peer = mbcsv_search_and_return_peer(ckpt->peer_list, msg->rcvr_peer_key.peer_anchor))) {
		if (peer_up) {
			mbcsv_send_peer_disc_msg(MBCSV_PEER_INFO_MSG, ckpt->my_mbcsv_inst, ckpt,
						 peer, MDS_SENDTYPE_RED, msg->rcvr_peer_key.peer_anchor);
			TRACE_LEAVE();
			return NCSCC_RC_SUCCESS;
		} else {

			peer->version = msg->info.peer_msg.info.peer_disc.info.peer_info.peer_version;
			mbcsv_update_peer_info(msg, ckpt, peer);
			TRACE_LEAVE();
			return NCSCC_RC_SUCCESS;
		}
	}

	if (0 != (mbx = mbcsv_get_mbx(msg->rcvr_peer_key.pwe_hdl, msg->rcvr_peer_key.svc_id))) {

		memset(&key, '\0', sizeof(MBCSV_PEER_KEY));
		key.pwe_hdl = msg->rcvr_peer_key.pwe_hdl;
		key.anchor = msg->rcvr_peer_key.peer_anchor;

		m_NCS_LOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);

		if (NULL == ncs_patricia_tree_get(&mbcsv_cb.peer_list, (const uint8_t *)&key)) {

			if (NULL == (evt = m_MMGR_ALLOC_MBCSV_EVT)) {
				TRACE_LEAVE2("malloc failed");
				m_NCS_UNLOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);
				return NCSCC_RC_FAILURE;
			}

			memset(evt, '\0', sizeof(MBCSV_EVT));
			memcpy(evt, msg, sizeof(MBCSV_EVT));

			TRACE_4("Still RED_UP event not arrived of the peer");

			/* Again post the event, till RED_UP event arrives */
			if (NCSCC_RC_SUCCESS != m_MBCSV_SND_MSG(&mbx, evt, NCS_IPC_PRIORITY_HIGH)) {
				TRACE_LEAVE2("ipc send failed");
				m_NCS_UNLOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);
				return NCSCC_RC_FAILURE;
			}

			m_NCS_UNLOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);
			return NCSCC_RC_SUCCESS;
		}

		m_NCS_UNLOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);
	}

	if (NULL == (peer = mbcsv_add_new_peer(ckpt, msg->rcvr_peer_key.peer_anchor))) {
		TRACE_LEAVE2("failed to add new peer");
		return NCSCC_RC_FAILURE;
	}

	peer->hdl = m_MBCSV_CREATE_HANDLE(peer);
	peer->peer_role = msg->info.peer_msg.info.peer_disc.peer_role;
	peer->my_ckpt_inst = ckpt;

	if (peer_up)
		peer->version = msg->info.peer_msg.info.peer_disc.info.peer_up.peer_version;
	else
		peer->version = msg->info.peer_msg.info.peer_disc.info.peer_info.peer_version;

	/*
	 * Call peer_info callback which will tell us whether the peer is 
	 * compatible or not.
	 */
	m_MBCSV_PEER_INFO(peer, ckpt->client_hdl, ckpt->my_mbcsv_inst->svc_id, peer->version, peer->incompatible);

	if (peer_up) {
		/* Set initial state of this new peer */
		mbcsv_set_peer_state(ckpt, peer, true);

		mbcsv_send_peer_disc_msg(MBCSV_PEER_INFO_MSG, ckpt->my_mbcsv_inst, ckpt,
					 peer, MDS_SENDTYPE_RED, msg->rcvr_peer_key.peer_anchor);
	} else {
		mbcsv_update_peer_info(msg, ckpt, peer);
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_update_peer_info
*
*  DESCRIPTION:       This function Updates peer info in peer data structure in 
*                     case of message is peer info message.
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to process PEER_UP.
*
*****************************************************************************/
void mbcsv_update_peer_info(MBCSV_EVT *msg, CKPT_INST *ckpt, PEER_INST *peer)
{
	peer->peer_hdl = msg->info.peer_msg.info.peer_disc.info.peer_info.my_peer_inst_hdl;

	/* In.... */
	peer->incompatible |= msg->info.peer_msg.info.peer_disc.info.peer_info.compatible;

	mbcsv_send_peer_disc_msg(MBCSV_PEER_INFO_RSP_MSG, ckpt->my_mbcsv_inst, ckpt,
				 peer, MDS_SENDTYPE_RED, msg->rcvr_peer_key.peer_anchor);

	mbcsv_set_peer_state(ckpt, peer, false);
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_process_peer_down
*
*  DESCRIPTION:       This function processes the PEER_DOWN message received from
*                     its peer client.
*
*  ACTION:            Actions to be taken on receiving PEER_DOWN are:
*                     1) Check whether this peer already exist. If NO then drop
*                        this message. Else remove this peer from peer list.
*                     2) Close session if any.
*
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to process PEER_UP.
*
*****************************************************************************/
uint32_t mbcsv_process_peer_down(MBCSV_EVT *msg, CKPT_INST *ckpt)
{
	PEER_INST *peer, *peer_ptr;
	bool act_peer = false;
	SaAmfHAStateT role;
	TRACE_ENTER();

	if (NULL == (peer = mbcsv_search_and_return_peer(ckpt->peer_list, msg->rcvr_peer_key.peer_anchor))) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	if ((NULL != ckpt->active_peer) && (peer->peer_anchor == ckpt->active_peer->peer_anchor))
		act_peer = true;

	role = peer->peer_role;

	/* Remove this peer */
	mbcsv_rmv_peer(ckpt, msg->rcvr_peer_key.peer_anchor);

	switch (ckpt->my_role) {
	case SA_AMF_HA_ACTIVE:
		{
			if (role == SA_AMF_HA_ACTIVE)
				mbcsv_clear_multiple_active_state(ckpt);
		}
		break;

	case SA_AMF_HA_STANDBY:
	case SA_AMF_HA_QUIESCED:
		{
			if (role == SA_AMF_HA_ACTIVE) {
				/* Check if this was my ACTIVE peer? */
				if (act_peer) {
					/* Our Active peer is going down, close its session */
					ckpt->active_peer = NULL;

					/* Check if anyother peer is ACTIVE if yes then set-up session
					 * with this new peer. */
					if (NULL != (peer_ptr = mbcsv_my_active_peer(ckpt))) {
						ckpt->active_peer = peer_ptr;
						mbcsv_set_up_new_session(ckpt, peer_ptr);
					}
				}
			}

		}
		break;

	default:
		TRACE_LEAVE2("Invalid state");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_process_peer_info_rsp
*
*  DESCRIPTION:       This function processes the PEER_INFO_RSP message received from
*                     its peer client. 
*
*  ACTION:            Actions to be taken on receiving PEER_INFO_RSP are:
*                     1) We are going to receive the peer handle in this message which
*                        we will be sending along with all the client messages. so 
*                        store this handle with you for future refrences.
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to process PEER_INFO_RSP.
*
*****************************************************************************/
uint32_t mbcsv_process_peer_info_rsp(MBCSV_EVT *msg, CKPT_INST *ckpt)
{
	PEER_INST *peer;
	TRACE_ENTER();

	if (NULL == (peer = mbcsv_search_and_return_peer(ckpt->peer_list, msg->rcvr_peer_key.peer_anchor))) {
		TRACE_LEAVE2("peer does not exist, svc_id: %u", ckpt->my_mbcsv_inst->svc_id);
		return NCSCC_RC_FAILURE;
	}

	peer->peer_hdl = msg->info.peer_msg.info.peer_disc.info.peer_info_rsp.my_peer_inst_hdl;
	peer->incompatible = msg->info.peer_msg.info.peer_disc.info.peer_info_rsp.compatible;
	peer->peer_role = msg->info.peer_msg.info.peer_disc.peer_role;

	mbcsv_set_peer_state(ckpt, peer, false);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_process_peer_chg_role
*
*  DESCRIPTION:       This function processes the PEER_CHG_ROLE message received from
*                     its peer client. 
*
*  ACTION:            Actions to be taken on receiving PEER_INFO are:
*                     1) Check whether this peer already exist. If NO then drop
*                        this message. Process the role change.
*                     2) Initiate a new role based actions.
*
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to process PEER_CHG_ROLE.
*
*****************************************************************************/
uint32_t mbcsv_process_peer_chg_role(MBCSV_EVT *msg, CKPT_INST *ckpt)
{
	PEER_INST *peer;

	if (NULL == (peer = mbcsv_search_and_return_peer(ckpt->peer_list, msg->rcvr_peer_key.peer_anchor))) {
		TRACE_LEAVE2("peer does not exist, svc_id: %u", ckpt->my_mbcsv_inst->svc_id);
		return NCSCC_RC_FAILURE;
	}

	peer->peer_role = msg->info.peer_msg.info.peer_disc.peer_role;

	mbcsv_set_peer_state(ckpt, peer, false);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
*
*  PROCEDURE:    mbcsv_send_peer_disc_msg
*
*  DESCRIPTION:       This function composes and then sends the peer disco message
*                     to the intended peer.
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to process PEER_UP.
*
\*****************************************************************************/
uint32_t mbcsv_send_peer_disc_msg(uint32_t type, MBCSV_REG *mbc, CKPT_INST *ckpt,
			       PEER_INST *peer, uint32_t mds_send_type, MBCSV_ANCHOR anchor)
{
	MBCSV_EVT evt;
	TRACE_ENTER();

	memset(&evt, '\0', sizeof(MBCSV_EVT));

	evt.rcvr_peer_key.svc_id = mbc->svc_id;
	evt.info.peer_msg.type = MBCSV_EVT_INTERNAL_PEER_DISC;
	evt.info.peer_msg.info.peer_disc.peer_role = ckpt->my_role;
	evt.info.peer_msg.info.peer_disc.msg_sub_type = type;

	switch (type) {
	case MBCSV_PEER_INFO_MSG:
		TRACE("peer info msg");
		evt.info.peer_msg.info.peer_disc.info.peer_info.peer_version = mbc->version;
		evt.info.peer_msg.info.peer_disc.info.peer_info.compatible = peer->incompatible;
		evt.info.peer_msg.info.peer_disc.info.peer_info.my_peer_inst_hdl = peer->hdl;
		break;

	case MBCSV_PEER_INFO_RSP_MSG:
		TRACE("peer info response msg");
		evt.info.peer_msg.info.peer_disc.info.peer_info_rsp.peer_version = mbc->version;
		evt.info.peer_msg.info.peer_disc.info.peer_info_rsp.compatible = peer->incompatible;
		evt.info.peer_msg.info.peer_disc.info.peer_info_rsp.my_peer_inst_hdl = peer->hdl;
		break;

	case MBCSV_PEER_UP_MSG:
		TRACE("peer up msg");
		evt.info.peer_msg.info.peer_disc.info.peer_up.peer_version = mbc->version;
		break;

	case MBCSV_PEER_CHG_ROLE_MSG:
		TRACE("change role msg");
		evt.info.peer_msg.info.peer_disc.info.peer_chg_role.peer_version = mbc->version;
		break;

	case MBCSV_PEER_DOWN_MSG:
		TRACE("peer down msg");
		evt.info.peer_msg.info.peer_disc.info.peer_down.dummy = 0;
		break;

	default:
		TRACE_LEAVE2("Incorrect msg type received in peer discover message");
		return NCSCC_RC_FAILURE;
	}
	mbcsv_mds_send_msg(mds_send_type, &evt, ckpt, anchor);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
