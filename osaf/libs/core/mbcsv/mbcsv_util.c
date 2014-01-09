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
      
   This file contains supporting functions for the MBCSV.

   FUNCTIONS INCLUDED in this module:
   mbcsv_rmv_reg_inst                - Remove registration instance.
   mbcsv_remove_ckpt_inst            - Remove checkpoint instance.
   mbcsv_process_chg_role            - Process change role request received.
   mbcsv_send_ckpt_data_to_all_peers - Send checkpoint data to all the peers.
   mbcsv_send_notify_msg             - Send notify message to all the peers.
   mbcsv_send_data_req               - Send data request message to Active.
   mbcsv_send_client_msg             - Queue up the message to be sent to peer.
   ncs_mbcsv_encode_message          - Function calls encode callback.
   mbcsv_send_msg                    - Send message to the destination.

   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/
#include "mbcsv.h"
#include "ncssysf_mem.h"

/**************************************************************************\
* PROCEDURE: mbcsv_rmv_reg_inst
*
* Purpose:  This function removes registration instance from the registration
*           list. It frees all the checkpoint instances and free the all allocated
*           memory.
*
* Input:    ckpt - Checkpoint instnace to be removed.
*
* Returns:  Success or Failure.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_rmv_reg_inst(MBCSV_REG *reg_list, MBCSV_REG *mbc_reg)
{
	CKPT_INST *ckpt = NULL;
	uint32_t pwe_hdl = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	m_MBCSV_DESTROY_HANDLE(mbc_reg->mbcsv_hdl);

	/*
	 * Find and remove all the ckpt instance. After removing all the instances
	 * destroy this ckpt list.
	 */
	while (NULL != (ckpt = (CKPT_INST *)ncs_patricia_tree_getnext(&mbc_reg->ckpt_ssn_list, (const uint8_t *)&pwe_hdl))) {
		pwe_hdl = ckpt->pwe_hdl;

		if (NCSCC_RC_SUCCESS != mbcsv_remove_ckpt_inst(ckpt)) {
			rc = NCSCC_RC_FAILURE;
			goto func_return;
		}
	}

	ncs_patricia_tree_destroy(&mbc_reg->ckpt_ssn_list);

	/*
	 * Delete the entry of this service from the registration list.
	 */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_del((NCS_PATRICIA_TREE *)reg_list, (NCS_PATRICIA_NODE *)mbc_reg)) {
 		TRACE("pat del failed");
		rc = NCSCC_RC_FAILURE;
		goto func_return;
	}

	/*
	 * Destroy this service specific mailbox.
	 */
	mbcsv_client_queue_destroy(mbc_reg);

	/*
	 * We don't need service lock. So destroy it.
	 */
 func_return:
	m_NCS_LOCK_DESTROY(&mbc_reg->svc_lock);
	m_MMGR_FREE_REG_INFO(mbc_reg);
	TRACE_LEAVE();
	return rc;
}

/**************************************************************************\
* PROCEDURE: mbcsv_remove_ckpt_inst
*
* Purpose:  This function removes checkpoint instance from the MBCA 
*           service registration.
*
* Input:    ckpt - Checkpoint instnace to be removed.
*
* Returns:  Success or Failure.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_remove_ckpt_inst(CKPT_INST *ckpt)
{
	SS_SVC_ID svc_id = 0;
	TRACE_ENTER();

	m_MBCSV_DESTROY_HANDLE(ckpt->ckpt_hdl);

	/* 
	 * First remove the entry from the mailbox list so that any new message
	 * landing for this pwe handle will not be queued into mailbox.
	 */
	if (NCSCC_RC_SUCCESS != mbcsv_rmv_entry(ckpt->pwe_hdl, ckpt->my_mbcsv_inst->svc_id)) {
		return NCSCC_RC_FAILURE;
	}

	/* Queued messages are not freed. (Expected to be dropped
	 *          when it comes to processing them.)
	 */

	/*
	 * Here we are going to free all the peers
	 * of each ckpt instance. It also stops all the timers of the state machine.
	 * It completely free's all the resources curretnly being used by this MBC
	 * client registration instance.
	 */
	mbcsv_empty_peers_list(ckpt);

	/*
	 * Remove this checkpoint entry from the MBC ckpt list.
	 */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_del(&ckpt->my_mbcsv_inst->ckpt_ssn_list, (NCS_PATRICIA_NODE *)ckpt)) {
 		TRACE_LEAVE2("pat del failed");
		return  NCSCC_RC_FAILURE;
	}

	/* 
	 * Chekc whether we need to de-register from MDS.
	 */
	if (0 == mbcsv_get_next_entry_for_pwe(ckpt->pwe_hdl, &svc_id)) {
		mbcsv_mds_unreg(ckpt->pwe_hdl);
		mbcsv_rmv_ancs_for_pwe(ckpt->pwe_hdl);
	} else
		mbcsv_send_peer_disc_msg(MBCSV_PEER_DOWN_MSG, ckpt->my_mbcsv_inst, ckpt, NULL, MDS_SENDTYPE_RBCAST, 0);

	TRACE("The checkpoint instance belonging to pwe_hdl: %u is removed.", ckpt->pwe_hdl);

	m_MMGR_FREE_CKPT_INST(ckpt);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}

/**************************************************************************\
* PROCEDURE: mbcsv_process_chg_role
*
* Purpose:  This function is used for processing the change role request raised
*           by the MBCSv client.
*
* Input:    ckpt - Checkpoint instnace to be removed.
*
* Returns:  Success or Failure.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_chg_role(MBCSV_EVT *rcvd_evt, MBCSV_REG *mbc_inst)
{
	SaAmfHAStateT old_role;
	CKPT_INST *ckpt;
	PEER_INST *peer;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/*
	 * As per the new role, change the state machine.
	 * Send this new role information to all the peers.
	 */
	if (NULL == (ckpt = m_MBCSV_TAKE_HANDLE(rcvd_evt->info.peer_msg.info.chg_role.ckpt_hdl))) {
		TRACE_LEAVE2("Unable to change role since CKPT does not exist");
		return NCSCC_RC_FAILURE;
	}

	/* Don't allow setting of same role unless last role was quiescing */
	if (ckpt->my_role == rcvd_evt->info.peer_msg.info.chg_role.new_role) {
		if (!ckpt->in_quiescing) {
			TRACE_2("Trying to set same role");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		} else {
			rc = NCSCC_RC_SUCCESS;
			goto done;
		}
	}

	old_role = ckpt->my_role;

	switch (rcvd_evt->info.peer_msg.info.chg_role.new_role) {
	case SA_AMF_HA_ACTIVE:
		{
			ckpt->my_role = SA_AMF_HA_ACTIVE;
			ckpt->in_quiescing = false;
			TRACE_1("Changing role to ACTIVE");
			ckpt->fsm = (NCS_MBCSV_STATE_ACTION_FUNC_PTR *)ncsmbcsv_active_state_tbl;

			peer = ckpt->peer_list;
			/* 
			 * We are first going to check whether some other ACTIVE peer is there?
			 * If yes then set the state machine of this ACTIVE to appropriate state. 
			 */
			while (NULL != peer) {
				if (peer->incompatible)
					m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_IDLE);
				else if (SA_AMF_HA_ACTIVE == peer->peer_role) {
					TRACE_1("changing state to NCS_MBCSV_ACT_STATE_MULTIPLE_ACTIVE");
					m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_MULTIPLE_ACTIVE);
				}
				else if (peer->cold_sync_done) {
					TRACE_1("changing state to NCS_MBCSV_ACT_STATE_KEEP_STBY_IN_SYNC");
					m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_KEEP_STBY_IN_SYNC);
				}
				else if (!peer->cold_sync_done) {
					TRACE_1("changing state to NCS_MBCSV_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC");
					m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC);
				}

				peer = peer->next;
			}
		}
		break;

	case SA_AMF_HA_STANDBY:
	case SA_AMF_HA_QUIESCED:
		{
			ckpt->my_role = rcvd_evt->info.peer_msg.info.chg_role.new_role;
			TRACE_1("changing role to %s", ((ckpt->my_role == SA_AMF_HA_STANDBY) ? "STANDBY":"QUIESCED"));
			ckpt->in_quiescing = false;
			ckpt->fsm = (NCS_MBCSV_STATE_ACTION_FUNC_PTR *)ncsmbcsv_standby_state_tbl;

			/*
			 * If old state was ACTIVE then close all the old sessions.
			 */
			peer = ckpt->peer_list;
			while (NULL != peer) {
				if (old_role == SA_AMF_HA_ACTIVE) {
					TRACE_1("previous role was ACTIVE, closing old sessions with peer");
					mbcsv_close_old_session(peer);
				}
				TRACE_1("checkpoint state set to IDLE");
				m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_IDLE);

				peer = peer->next;
			}

			peer = ckpt->active_peer;
			/* 
			 * Check if ACTIVE peer exist.
			 */
			if (NULL != peer) {
				TRACE_1("ACTIVE peer exists, starting new session");
				mbcsv_set_up_new_session(ckpt, peer);
			}
		}
		break;

	case SA_AMF_HA_QUIESCING:
		{
			/*
			 * Do Nothing. Just set the varibale in_quiescing which will tell us that
			 * the HA role was set to quiescing some time.
			 */
			ckpt->in_quiescing = true;
			TRACE_1("changing state to QUIESCING, do nothing"); 
			rc = NCSCC_RC_SUCCESS;
			goto done;
		}
		break;

	default:
 		TRACE("Invalid ha role specified");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (rc == NCSCC_RC_SUCCESS)
		ckpt->role_set = true;

	/*
	 * Send this change role information to all the peers so that they will 
	 * Update their data-base with this info.
	 */
	if (ckpt->peer_up_sent)
		mbcsv_send_peer_disc_msg(MBCSV_PEER_CHG_ROLE_MSG, mbc_inst, ckpt, NULL, MDS_SENDTYPE_RBCAST, 0);
	else {
		ckpt->peer_up_sent = true;
		mbcsv_send_peer_disc_msg(MBCSV_PEER_UP_MSG, mbc_inst, ckpt, NULL, MDS_SENDTYPE_RBCAST, 0);
	}

 done:
	m_MBCSV_GIVE_HANDLE(rcvd_evt->info.peer_msg.info.chg_role.ckpt_hdl);
	TRACE_LEAVE();
	return rc;

}

/**************************************************************************\
* PROCEDURE: mbcsv_send_ckpt_data_to_all_peers
*
* Purpose:  This function sends the checkpoint data to all the standby peers.
*
* Input:    msg_to_send - Checkpoint message to be sent.
*
* Returns:  SUCCESS/FAILURE.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_send_ckpt_data_to_all_peers(NCS_MBCSV_SEND_CKPT *msg_to_send, CKPT_INST *ckpt_inst, MBCSV_REG *mbc_inst)
{
	NCS_MBCSV_CB_ARG parg;
	PEER_INST *peer_ptr = ckpt_inst->peer_list;
	PEER_INST *tmp_ptr = peer_ptr;
	NCS_UBAID *uba = NULL;
	USRBUF *dup_ub = NULL;
	MBCSV_EVT evt_msg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (NULL == ckpt_inst->peer_list) {
		if (msg_to_send->io_no_peer == true) {
			TRACE("no peer exists");	
			/* Application requested to let it know if the peer doesn't exist */
			parg.i_client_hdl = ckpt_inst->client_hdl;
			parg.i_ckpt_hdl = ckpt_inst->ckpt_hdl;
			parg.i_op = NCS_MBCSV_CBOP_ENC;
			parg.info.encode.io_msg_type = NCS_MBCSV_MSG_ASYNC_UPDATE;
			parg.info.encode.io_action = NCS_MBCSV_ACT_NO_PEER;
			parg.info.encode.io_reo_hdl = msg_to_send->i_reo_hdl;
			parg.info.encode.io_reo_type = msg_to_send->i_reo_type;

			if (NCSCC_RC_SUCCESS != mbc_inst->mbcsv_cb_func(&parg)) {
				TRACE_LEAVE2("NCS_MBCSV_MSG_ASYNC_UPDATE Encode callback returned failure");
				return NCSCC_RC_FAILURE;
			}
		}
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}

	/* 
	 * Mark all peers for message to be sent.
	 */
	while (NULL != tmp_ptr) {
		tmp_ptr->ckpt_msg_sent = false;
		tmp_ptr->okay_to_async_updt = false;
		tmp_ptr = tmp_ptr->next;
	}

	/* 
	 * Send ckpt message to all peers.
	 */
	while (NULL != peer_ptr) {
		/* 
		 * Check whether we have to send the message to this peer. If not then
		 * go to next peer.
		 */
		if (true == peer_ptr->ckpt_msg_sent) {
			peer_ptr = peer_ptr->next;
			continue;
		}
		TRACE("dispatching FSM for NCSMBCSV_SEND_ASYNC_UPDATE");
		m_NCS_MBCSV_FSM_DISPATCH(peer_ptr, NCSMBCSV_SEND_ASYNC_UPDATE, &evt_msg);

		if (false == peer_ptr->okay_to_async_updt) {
			peer_ptr->ckpt_msg_sent = true;
			peer_ptr = peer_ptr->next;
			continue;
		}

		/*
		 * Call encode callback to encode message to be sent.
		 */
		TRACE("calling encode callback");
		uba = &parg.info.encode.io_uba;
		memset(uba, '\0', sizeof(NCS_UBAID));

		if (NCSCC_RC_SUCCESS != ncs_enc_init_space(uba)) {
			TRACE_LEAVE2("encode init failed");
			return NCSCC_RC_FAILURE;
		}

		parg.i_client_hdl = ckpt_inst->client_hdl;
		parg.i_ckpt_hdl = ckpt_inst->ckpt_hdl;
		parg.i_op = NCS_MBCSV_CBOP_ENC;
		parg.info.encode.io_msg_type = NCS_MBCSV_MSG_ASYNC_UPDATE;
		parg.info.encode.io_action = msg_to_send->i_action;
		parg.info.encode.io_reo_hdl = msg_to_send->i_reo_hdl;
		parg.info.encode.io_reo_type = msg_to_send->i_reo_type;
		parg.info.encode.i_peer_version = peer_ptr->version;

		if (NCSCC_RC_SUCCESS != mbc_inst->mbcsv_cb_func(&parg)) {
			TRACE_LEAVE2("encode callback returned failure for ASYNC UPDATE");
			return NCSCC_RC_FAILURE;
		}

		/*
		 * Check whether we want to send this encoded message to any other peer
		 * with same software revision. If yes then send message to all of them.
		 * So this encoded checkpoint message will get sent to all the peers with 
		 * same software revision.
		 * Check send type before sending the message.
		 */
		tmp_ptr = peer_ptr;
		evt_msg.msg_type = MBCSV_EVT_INTERNAL;
		evt_msg.rcvr_peer_key.svc_id = mbc_inst->svc_id;
		evt_msg.info.peer_msg.type = MBCSV_EVT_INTERNAL_CLIENT;
		evt_msg.info.peer_msg.info.client_msg.type.msg_sub_type = NCS_MBCSV_MSG_ASYNC_UPDATE;
		evt_msg.info.peer_msg.info.client_msg.action = msg_to_send->i_action;
		evt_msg.info.peer_msg.info.client_msg.reo_type = msg_to_send->i_reo_type;
		evt_msg.info.peer_msg.info.client_msg.snd_type = msg_to_send->i_send_type;

		TRACE("send the encoded message to any other peer with same s/w version");

		while (NULL != tmp_ptr) {
			TRACE("dispatching FSM for NCSMBCSV_SEND_ASYNC_UPDATE");
			m_NCS_MBCSV_FSM_DISPATCH(tmp_ptr, NCSMBCSV_SEND_ASYNC_UPDATE, &evt_msg);

			if (false == tmp_ptr->okay_to_async_updt) {
				tmp_ptr->ckpt_msg_sent = true;
				tmp_ptr = tmp_ptr->next;
				continue;
			}

			if (NULL == (dup_ub = m_MMGR_DITTO_BUFR(uba->start))) {
				TRACE_LEAVE2("buffer duplication failed");
				return NCSCC_RC_FAILURE;
			}

			if (parg.info.encode.i_peer_version == tmp_ptr->version) {
				evt_msg.rcvr_peer_key.peer_inst_hdl = tmp_ptr->peer_hdl;
				evt_msg.info.peer_msg.info.client_msg.uba = *uba;
				evt_msg.info.peer_msg.info.client_msg.uba.start = dup_ub;

				switch (msg_to_send->i_send_type) {
				case NCS_MBCSV_SND_SYNC:
					{
						rc = m_NCS_MBCSV_MDS_SYNC_SEND(&evt_msg,
									  tmp_ptr->my_ckpt_inst, tmp_ptr->peer_anchor);
					}
					break;

				case NCS_MBCSV_SND_USR_ASYNC:
				case NCS_MBCSV_SND_MBC_ASYNC:
					{
						rc = m_NCS_MBCSV_MDS_ASYNC_SEND(&evt_msg,
									   tmp_ptr->my_ckpt_inst, tmp_ptr->peer_anchor);
					}
					break;
				default:
					m_MMGR_FREE_BUFR_LIST(dup_ub);
					TRACE_LEAVE2("unsupported send type");
					return NCSCC_RC_FAILURE;
				}
				tmp_ptr->ckpt_msg_sent = true;
	                       /* In failure scenario, there is a chance that MDS may  not free dup_ub */
				if (rc != NCSCC_RC_SUCCESS)
				{
					TRACE_LEAVE2("MBCSv SEND Failed");
					m_MMGR_FREE_BUFR_LIST(dup_ub);
				}
			}
			tmp_ptr = tmp_ptr->next;
		}

		m_MMGR_FREE_BUFR_LIST(uba->start);

		/*
		 * So we don't have to send this message to any other peer.
		 * Go to next peer.
		 */
		peer_ptr = peer_ptr->next;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
* PROCEDURE: mbcsv_send_notify_msg
*
* Purpose:  This function is used for sending the notify messages to the
*           destination user wants to send.
*
* Input:    uba - User buff to be sent to the destination.
*           msg_dest - Destination where message to be sent.
*
* Returns:  registration pointer.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_send_notify_msg(uint32_t msg_dest, CKPT_INST *ckpt_inst, MBCSV_REG *mbc_inst, NCSCONTEXT i_msg)
{
	NCS_MBCSV_CB_ARG parg;
	MBCSV_EVT evt_msg;
	NCS_UBAID *uba = NULL;
	TRACE_ENTER();

	memset(&parg, '\0', sizeof(NCS_MBCSV_CB_ARG));
	memset(&evt_msg, '\0', sizeof(MBCSV_EVT));

	/*
	 * Generate the message to be sent.
	 */
	evt_msg.msg_type = MBCSV_EVT_INTERNAL;
	evt_msg.rcvr_peer_key.svc_id = mbc_inst->svc_id;
	evt_msg.info.peer_msg.type = MBCSV_EVT_INTERNAL_CLIENT;
	evt_msg.info.peer_msg.info.client_msg.type.evt_type = NCSMBCSV_EVENT_NOTIFY;

	parg.i_client_hdl = ckpt_inst->client_hdl;
	parg.i_ckpt_hdl = ckpt_inst->ckpt_hdl;
	parg.i_op = NCS_MBCSV_CBOP_ENC_NOTIFY;
	parg.info.notify.i_msg = i_msg;
	
	TRACE("send NCSMBCSV_EVENT_NOTIFY to peers");
	/*
	 * Send message to the destinatin/destinations as asked by client.
	 */
	switch (msg_dest) {
	case NCS_MBCSV_ACTIVE:
		{
			if (SA_AMF_HA_ACTIVE == ckpt_inst->my_role) {
				TRACE_LEAVE2("incorrect role");
				return NCSCC_RC_FAILURE;
			}

			/* 
			 * So I am in standby role. So send message to active peer.
			 */
			if (NULL != ckpt_inst->active_peer) {
				ckpt_inst->active_peer->okay_to_send_ntfy = false;

				TRACE_1("dispatching fsm for NCSMBCSV_SEND_NOTIFY");

				m_NCS_MBCSV_FSM_DISPATCH(ckpt_inst->active_peer, NCSMBCSV_SEND_NOTIFY, &evt_msg);

				if (true == ckpt_inst->active_peer->okay_to_send_ntfy) {
					/*
					 * Call encode callback to encode message to be sent.
					 */
					TRACE_1("invoke encode callback");
					uba = &parg.info.notify.i_uba;
					memset(uba, '\0', sizeof(NCS_UBAID));

					if (NCSCC_RC_SUCCESS != ncs_enc_init_space(uba)) {
						TRACE_LEAVE2("leap encode init failed");
						return NCSCC_RC_FAILURE;
					}

					parg.info.notify.i_peer_version = ckpt_inst->active_peer->version;

					if (NCSCC_RC_SUCCESS != mbc_inst->mbcsv_cb_func(&parg)) {
						/* delete the buffer allocated */
						if ((uba) && (uba->start)) {
							m_MMGR_FREE_BUFR_LIST(uba->start);
							uba = NULL;
						}
						TRACE_LEAVE2("notify encode callback returned failure");
						return NCSCC_RC_FAILURE;
					}

					evt_msg.rcvr_peer_key.peer_inst_hdl = ckpt_inst->active_peer->peer_hdl;
					evt_msg.info.peer_msg.info.client_msg.uba = *uba;

					m_NCS_MBCSV_MDS_ASYNC_SEND(&evt_msg,
								   ckpt_inst, ckpt_inst->active_peer->peer_anchor);
				}
			}
		}
		break;

	case NCS_MBCSV_STANDBY:
	case NCS_MBCSV_ALL_PEERS:
		{
			uint16_t peer_count = 0;
			uint16_t tmp_peer_version = 0;
			uint8_t set_peer_version = true;
			USRBUF *dup_ub = NULL;
			PEER_INST *peer = ckpt_inst->peer_list;

			/*
			 * Mark all peers for message to be sent.
			 */
			while (peer) {
				peer_count++;
				peer->notify_msg_sent = false;
				peer = peer->next;
			}

			peer = ckpt_inst->peer_list;
			while (peer_count) {
				/* If done with traversing thru peer_list, again set the ptr to 
				   start of the peerlist to traverse for another version of peers */
				if (peer == NULL) {
					tmp_peer_version = 0;
					set_peer_version = true;
					peer = ckpt_inst->peer_list;

					/* Now delete the redundant ditto buffer */
					if ((uba) && (uba->start)) {
						m_MMGR_FREE_BUFR_LIST(uba->start);
						uba = NULL;
					}
				}

				/* peer already checked ?? */
				if (peer->notify_msg_sent == true) {
					peer = peer->next;
					continue;
				}

				if ((msg_dest == NCS_MBCSV_STANDBY) && (SA_AMF_HA_STANDBY != peer->peer_role)) {
					peer->notify_msg_sent = true;
					peer_count--;
					peer = peer->next;
					continue;
				}

				peer->okay_to_send_ntfy = false;

				m_NCS_MBCSV_FSM_DISPATCH(peer, NCSMBCSV_SEND_NOTIFY, &evt_msg);

				if (true == peer->okay_to_send_ntfy) {
					if (set_peer_version) {
						set_peer_version = false;
						tmp_peer_version = peer->version;

						/*
						 * Call encode callback to encode message to be sent.
						 */
						uba = &parg.info.notify.i_uba;
						memset(uba, '\0', sizeof(NCS_UBAID));

						if (NCSCC_RC_SUCCESS != ncs_enc_init_space(uba)) {
							TRACE_LEAVE2("leap enc init failed");
							return NCSCC_RC_FAILURE;
						}

						parg.info.notify.i_peer_version = peer->version;

						if (NCSCC_RC_SUCCESS != mbc_inst->mbcsv_cb_func(&parg)) {
							/* delete the buffer allocated */
							if ((uba) && (uba->start)) {
								m_MMGR_FREE_BUFR_LIST(uba->start);
								uba = NULL;
							}
							TRACE_LEAVE2("notify encode callback returned failure");
							return NCSCC_RC_FAILURE;
						}
					}

					if (tmp_peer_version == peer->version) {
						if (NULL == (dup_ub = m_MMGR_DITTO_BUFR(uba->start))) {
							TRACE_LEAVE2("buffer duplication failed");
							return NCSCC_RC_FAILURE;
						}

						evt_msg.rcvr_peer_key.peer_inst_hdl = peer->peer_hdl;
						evt_msg.info.peer_msg.info.client_msg.uba = *uba;
						evt_msg.info.peer_msg.info.client_msg.uba.start = dup_ub;

						m_NCS_MBCSV_MDS_ASYNC_SEND(&evt_msg, ckpt_inst, peer->peer_anchor);
					}
				}

				peer_count--;
				peer->notify_msg_sent = true;
				peer = peer->next;
			}

			/* Now delete the redundant ditto buffer */
			if ((uba) && (uba->start)) {
				m_MMGR_FREE_BUFR_LIST(uba->start);
				uba = NULL;
			}
		}
		break;

	default:
		TRACE_LEAVE2("Message destination type is incorrect");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
* PROCEDURE: mbcsv_send_data_req
*
* Purpose:  This function is used for sending the data request messages to the
*           active peer.
*
* Input:    uba - User buff to be sent to the destination.
*           msg_dest - Destination where message to be sent.
*
* Returns:  registration pointer.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_send_data_req(NCS_UBAID *uba, CKPT_INST *ckpt_inst, MBCSV_REG *mbc_inst)
{
	MBCSV_EVT evt_msg;
	TRACE_ENTER();

	/*
	 * Generate the message to be sent.
	 */
	evt_msg.msg_type = MBCSV_EVT_INTERNAL;
	evt_msg.rcvr_peer_key.svc_id = mbc_inst->svc_id;
	evt_msg.info.peer_msg.type = MBCSV_EVT_INTERNAL_CLIENT;
	evt_msg.info.peer_msg.info.client_msg.type.msg_sub_type = NCS_MBCSV_MSG_DATA_REQ;

	evt_msg.info.peer_msg.info.client_msg.uba = *uba;

	evt_msg.info.peer_msg.info.client_msg.uba.start = uba->start;

	m_NCS_MBCSV_FSM_DISPATCH(ckpt_inst->active_peer, NCSMBCSV_SEND_DATA_REQ, &evt_msg);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
* PROCEDURE: mbcsv_send_client_msg
*
* Purpose:  This function is used for sending the client messages. It composes
*           the client message and then queue it.
*
* Input:    uba - User buff to be sent to the destination.
*           msg_dest - Destination where message to be sent.
*
* Returns:  registration pointer.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_send_client_msg(PEER_INST *peer, uint8_t evt, uint32_t action)
{
	MBCSV_EVT *evt_msg;
	TRACE_ENTER2("send a mailbox message to the local client");

	/*
	 * Create the event message and post it to the mailbox.
	 */
	if (NULL == (evt_msg = m_MMGR_ALLOC_MBCSV_EVT)) {
		TRACE_LEAVE2("malloc failed");
		return NCSCC_RC_FAILURE;
	}

	/*
	 * Generate the message to be sent.
	 */
	evt_msg->msg_type = MBCSV_EVT_INTERNAL;
	evt_msg->rcvr_peer_key.svc_id = peer->my_ckpt_inst->my_mbcsv_inst->svc_id;
	evt_msg->rcvr_peer_key.peer_inst_hdl = peer->hdl;
	evt_msg->info.peer_msg.type = MBCSV_EVT_INTERNAL_CLIENT;
	evt_msg->info.peer_msg.info.client_msg.type.raw = evt;
	evt_msg->info.peer_msg.info.client_msg.action = action;

	if (NCSCC_RC_SUCCESS != m_MBCSV_SND_MSG(&peer->my_ckpt_inst->my_mbcsv_inst->mbx,
						evt_msg, NCS_IPC_PRIORITY_NORMAL)) {
		m_MMGR_FREE_MBCSV_EVT(evt_msg);
		TRACE_LEAVE2("ipc send failed");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
* PROCEDURE: ncs_mbcsv_encode_message
*
* Purpose:  This function calls encode callback and sets the user buff to be
*           sent along with the message.
*
* Input:    peer - Peer info.
*           evt_msg - Event message to be sent.
*           event   - Type of event.
*           uba   - Userbuff pointer.
*
* Returns:  SUCCESS/FAILURE.
*
* Notes:  
*
\**************************************************************************/
uint32_t ncs_mbcsv_encode_message(PEER_INST *peer, MBCSV_EVT *evt_msg, uint8_t *io_event, NCS_UBAID *uba)
{
	NCS_MBCSV_CB_ARG parg;
	MBCSV_REG *mbc_inst = peer->my_ckpt_inst->my_mbcsv_inst;
	TRACE_ENTER();

	/*
	 * Call encode callback to encode message to be sent.
	 */
	memset(uba, '\0', sizeof(NCS_UBAID));

	if (NCSCC_RC_SUCCESS != ncs_enc_init_space(uba)) {
		TRACE_LEAVE2("leap enc init failed");
		return NCSCC_RC_FAILURE;
	}

	parg.i_client_hdl = peer->my_ckpt_inst->client_hdl;
	parg.i_ckpt_hdl = peer->my_ckpt_inst->ckpt_hdl;
	parg.i_op = NCS_MBCSV_CBOP_ENC;
	parg.info.encode.io_msg_type = *io_event;

	switch (*io_event) {
	case NCSMBCSV_EVENT_COLD_SYNC_RESP:
	case NCSMBCSV_EVENT_WARM_SYNC_RESP:
	case NCSMBCSV_EVENT_DATA_RESP:
		TRACE("set call_again_action for coldsync_resp or warmsync_resp or data_resp");
		parg.info.encode.io_action = peer->call_again_action;
		parg.info.encode.io_reo_hdl = peer->call_again_reo_hdl;
		parg.info.encode.io_reo_type = peer->call_again_reo_type;
		parg.info.encode.io_req_context = peer->req_context;
		break;

	default:
		TRACE("do nothing");
		parg.info.encode.io_action = NCS_MBCSV_ACT_DONT_CARE;
		parg.info.encode.io_reo_hdl = 0;
		parg.info.encode.io_reo_type = 0;
		parg.info.encode.io_req_context = 0;
		break;
	}

	parg.info.encode.i_peer_version = peer->version;
	parg.info.encode.io_uba = *uba;

	if (NCSCC_RC_SUCCESS != mbc_inst->mbcsv_cb_func(&parg)) {
		TRACE_LEAVE2("encode callback returned failure");
		return NCSCC_RC_FAILURE;
	}

	*io_event = parg.info.encode.io_msg_type;

	switch (parg.info.encode.io_msg_type) {
	case NCSMBCSV_EVENT_COLD_SYNC_RESP:
	case NCSMBCSV_EVENT_WARM_SYNC_RESP:
	case NCSMBCSV_EVENT_DATA_RESP:
		TRACE("set-next call_again_action for coldsync_resp or warmsync_resp or data_resp");
		peer->call_again_action = parg.info.encode.io_action;
		peer->call_again_reo_hdl = parg.info.encode.io_reo_hdl;
		peer->call_again_reo_type = parg.info.encode.io_reo_type;
		peer->req_context = parg.info.encode.io_req_context;
		break;

	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		TRACE("setup things for NCS_MBCSV_MSG_DATA_RESP_COMPLETE");
		peer->data_resp_process = false;
		peer->call_again_action = NCS_MBCSV_ACT_DONT_CARE;
		peer->call_again_reo_hdl = 0;
		peer->call_again_reo_type = 0;
		peer->req_context = 0;
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
		TRACE("setup things for NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE");
		peer->w_syn_resp_process = false;
		peer->call_again_action = NCS_MBCSV_ACT_DONT_CARE;
		peer->call_again_reo_hdl = 0;
		peer->call_again_reo_type = 0;
		peer->req_context = 0;
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
		TRACE("setthings for NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE");
		peer->c_syn_resp_process = false;
		peer->call_again_action = NCS_MBCSV_ACT_DONT_CARE;
		peer->call_again_reo_hdl = 0;
		peer->call_again_reo_type = 0;
		peer->req_context = 0;
		break;

	default:
		TRACE("do nothing");
		peer->call_again_action = NCS_MBCSV_ACT_DONT_CARE;
		peer->call_again_reo_hdl = 0;
		peer->call_again_reo_type = 0;
		peer->req_context = 0;
		break;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
* PROCEDURE: mbcsv_send_msg
*
* Purpose:  This function is used for sending the client messages. It composes
*           the client message and then does MDS send to send this message to 
*           the MBCSv client.
*
* Input:    peer - Peer info.
*           evt_msg - Event message to be sent.
*           event   - Type of event.
*
* Returns:  SUCCESS/FAILURE.
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_send_msg(PEER_INST *peer, MBCSV_EVT *evt_msg, uint8_t event)
{
	NCS_UBAID uba;
	uint32_t ret_val;
	TRACE_ENTER2("event type: %u", event);

	/*
	 * Generate the message to be sent.
	 */
	evt_msg->msg_type = MBCSV_EVT_INTERNAL;
	evt_msg->rcvr_peer_key.svc_id = peer->my_ckpt_inst->my_mbcsv_inst->svc_id;
	evt_msg->rcvr_peer_key.peer_inst_hdl = peer->peer_hdl;
	evt_msg->info.peer_msg.type = MBCSV_EVT_INTERNAL_CLIENT;
	evt_msg->info.peer_msg.info.client_msg.type.raw = event;
	evt_msg->info.peer_msg.info.client_msg.reo_type = peer->call_again_reo_type;
	evt_msg->info.peer_msg.info.client_msg.first_rsp = peer->new_msg_seq;

	if ((event != NCS_MBCSV_MSG_SYNC_SEND_RSP) && (event != NCS_MBCSV_MSG_DATA_REQ)) {
		if ((ret_val = ncs_mbcsv_encode_message(peer, evt_msg, &event, &uba)) != NCSCC_RC_SUCCESS) {
			TRACE_LEAVE();
			return ret_val; 
		}

		evt_msg->info.peer_msg.info.client_msg.uba = uba;
		evt_msg->info.peer_msg.info.client_msg.type.raw = event;
		evt_msg->info.peer_msg.info.client_msg.action = peer->call_again_action;
	}

	if (NCS_MBCSV_MSG_SYNC_SEND_RSP == event) {
		TRACE("NCS_MBCSV_MSG_SYNC_SEND_RSP event");
		if (m_NCS_MBCSV_SYNC_MDS_RSP_SEND(evt_msg, peer->my_ckpt_inst, peer->peer_anchor) != NCSCC_RC_SUCCESS) {
			/* Failure implies communications are down; clean up segmented list */
			TRACE_LEAVE2("Unable to send response to sync send message");
			return NCSCC_RC_FAILURE;
		}
	} else {
		if (m_NCS_MBCSV_MDS_ASYNC_SEND(evt_msg, peer->my_ckpt_inst, peer->peer_anchor) != NCSCC_RC_SUCCESS) {
			/* Failure implies communications are down */
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
	}

	m_MBCSV_CHK_SUBSCRIPTION(peer, event, NCS_MBCSV_DIR_SENT);

	/* Start the transmit timer for the events that have
	 * multi-part responses and if the redundant entity has
	 * more data to send ie call_again is true
	 */
	switch (event) {
	case NCSMBCSV_EVENT_COLD_SYNC_RESP:
	case NCSMBCSV_EVENT_WARM_SYNC_RESP:
	case NCSMBCSV_EVENT_DATA_RESP:
		TRACE_1("event is multi-part response(i.e.coldsync resp, warmsync resp or dataresp), call-again");
		/* Save the event to be regenerated */
		peer->call_again_event = event;

 		TRACE_1("start the transmit timer");
		/* Start the transmit timer */
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_TRANSMIT);
		break;

	case NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE:
	case NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE:
	case NCSMBCSV_EVENT_DATA_RESP_COMPLETE:
		TRACE_1("event is of type response complete");
		peer->new_msg_seq = false;
		peer->cold_sync_done = true;
		break;

	default:
		/* The transmit timer does not apply */
		break;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
* PROCEDURE: mbcsv_subscribe_oneshot
*
* Purpose:  This function adds the message subscription filter into the message
*           subscription informatio..
*
* Input:    fltr - Filter information provided by the caller during subscribe.
*           time_10ms - .Time-out time before this subscription expires.
*
* Returns:  Success/Failure
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_subscribe_oneshot(NCS_MBCSV_FLTR *fltr, uint16_t time_10ms)
{
	TRACE("entered and returning from mbcsv_subscribe_oneshot()");
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
* PROCEDURE: mbcsv_subscribe_persist
*
* Purpose:  This function adds the message subscription filter into the message
*           subscription informatio..
*
* Input:    fltr - Filter information provided by the caller during subscribe.
*
* Returns:  Success/Failure
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_subscribe_persist(NCS_MBCSV_FLTR *fltr)
{
	TRACE("entered and returning from mbcsv_subscribe_persist()");
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
* PROCEDURE: mbcsv_subscribe_cancel
*
* Purpose:  This function removes the message subscription filter from the message
*           subscription informatio.
*
* Input:    sub_hdl - Subcription handle..
*
* Returns:  Success/Failure
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_subscribe_cancel(uint32_t sub_hdl)
{
	TRACE("entered and returning from mbcsv_subscribe_cancel()");
	return NCSCC_RC_SUCCESS;
}
