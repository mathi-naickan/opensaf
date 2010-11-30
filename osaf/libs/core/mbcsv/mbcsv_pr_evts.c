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

  This file contains functions that process the events from the MBCSv mailbox
  when MBCA clients calls the dispatch call.

..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "mbcsv.h"

/****************************************************************************
  PROCEDURE:    : mbcsv_process_events
 
  Description   : It is called when the MBCSv client chooses
                  to *Dispatch* callbacks.
 
  Arguments     : rcvd_evt - Event to be processed.
                  mbcsv_hdl - MBCSv Handle
 
  Return Values : Success or failure code.
 
  Notes         : None
******************************************************************************/
uns32 mbcsv_process_events(MBCSV_EVT *rcvd_evt, uns32 mbcsv_hdl)
{
	uns32 rc = SA_AIS_OK;
	MBCSV_REG *mbc_reg = NULL;
	PEER_INST *peer = NULL;
	CKPT_INST *ckpt = NULL;
	uns32 hdl_to_give;

	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(mbcsv_hdl))) {
		return m_MBCSV_DBG_SINK(SA_AIS_ERR_BAD_HANDLE, "mbcsv_process_events: Bad handle received.");
	}

	/* 
	 * Before starting processing of the event take service lock 
	 */
	m_NCS_LOCK(&mbc_reg->svc_lock, NCS_LOCK_WRITE);
	m_LOG_MBCSV_SVC_LOCK(MBCSV_LK_LOCKED, mbc_reg->svc_id, &mbc_reg->svc_lock);

	/* invoke the corresponding callback */
	switch (rcvd_evt->msg_type) {
	case MBCSV_EVT_MDS_SUBSCR:
		{
			switch (rcvd_evt->info.mds_sub_evt.evt_type) {
			case NCSMDS_RED_UP:
				{
					/*
					 * Send PEER_UP message to the peer anchor from which we have
					 * received UP event.
					 */
					if (NULL != (ckpt = (CKPT_INST *)ncs_patricia_tree_get(&mbc_reg->ckpt_ssn_list,
											       (const uns8 *)&rcvd_evt->
											       rcvr_peer_key.
											       pwe_hdl))) {
						mbcsv_send_peer_disc_msg(MBCSV_PEER_UP_MSG, mbc_reg, ckpt, peer,
									 MDS_SENDTYPE_RED,
									 rcvd_evt->rcvr_peer_key.peer_anchor);
					}
				}
				break;

			case NCSMDS_RED_DOWN:
				{
					if (NULL != (ckpt = (CKPT_INST *)ncs_patricia_tree_get(&mbc_reg->ckpt_ssn_list,
											       (const uns8 *)&rcvd_evt->
											       rcvr_peer_key.
											       pwe_hdl))) {
						/*
						 * Check if any of the peers of my ckpts lies on this vdest.
						 * If yes then remove that peer and close a session.
						 */
						mbcsv_process_peer_down(rcvd_evt, ckpt);
					}

				}
				break;

			default:
				goto pr_done;
			}
		}
		break;

	case MBCSV_EVT_TMR:
		{
			/* 
			 * Send all these timer events to the state machine which will handle 
			 * this event for us.
			 */
			if (NULL != (peer = (PEER_INST *)m_MBCSV_TAKE_HANDLE(rcvd_evt->info.tmr_evt.peer_inst_hdl))) {
				hdl_to_give = rcvd_evt->info.tmr_evt.peer_inst_hdl;
				ckpt = peer->my_ckpt_inst;

				m_LOG_MBCSV_FSM_EVT(ckpt->my_role, mbc_reg->svc_id, ckpt->pwe_hdl,
						    peer->peer_anchor, peer->state,
						    (MBCSV_FSM_EV_DBG_NONE + NCSMBCSV_EVENT_TMR_MIN +
						     rcvd_evt->info.tmr_evt.type));

				m_NCS_MBCSV_FSM_DISPATCH(peer, (NCSMBCSV_EVENT_TMR_MIN +
								rcvd_evt->info.tmr_evt.type), rcvd_evt);

				m_MBCSV_GIVE_HANDLE(hdl_to_give);
			} else
				goto pr_done;
		}
		break;
	case MBCSV_EVT_INTERNAL:
		{
			/*
			 * Process all the received events.
			 */
			switch (rcvd_evt->info.peer_msg.type) {
			case MBCSV_EVT_INTERNAL_CLIENT:
				{

					if (NULL !=
					    (peer =
					     (PEER_INST *)m_MBCSV_TAKE_HANDLE((uns32)rcvd_evt->rcvr_peer_key.
									      peer_inst_hdl))) {
						hdl_to_give = (uns32)rcvd_evt->rcvr_peer_key.peer_inst_hdl;
						ckpt = peer->my_ckpt_inst;

						m_LOG_MBCSV_FSM_EVT(ckpt->my_role, mbc_reg->svc_id, ckpt->pwe_hdl,
								    peer->peer_anchor, peer->state,
								    (MBCSV_FSM_EV_DBG_NONE +
								     rcvd_evt->info.peer_msg.info.client_msg.
								     type.evt_type));

						m_NCS_MBCSV_FSM_DISPATCH(peer,
									 rcvd_evt->info.peer_msg.info.client_msg.
									 type.evt_type, rcvd_evt);

						m_MBCSV_GIVE_HANDLE(hdl_to_give);
					} else
						goto pr_done;;
				}
				break;
			case MBCSV_EVT_INTERNAL_PEER_DISC:
				mbcsv_process_peer_discovery_message(rcvd_evt, mbc_reg);
				break;

			case MBCSV_EVT_CHG_ROLE:
				mbcsv_process_chg_role(rcvd_evt, mbc_reg);
				break;

			case MBCSV_EVT_MBC_ASYNC_SEND:
				{
					if (NULL == (ckpt =
						     (CKPT_INST *)m_MBCSV_TAKE_HANDLE(rcvd_evt->info.peer_msg.info.
										      usr_msg_info.i_ckpt_hdl))) {
						m_MBCSV_DBG_SINK_SVC(SA_AIS_ERR_BAD_HANDLE,
								     "NCS_MBCSV_OP_SEND_CKPT: CKPT instance does exist.",
								     0);
						goto pr_done;
					}

					mbcsv_send_ckpt_data_to_all_peers(&rcvd_evt->info.peer_msg.info.usr_msg_info,
									  ckpt, mbc_reg);

					m_MBCSV_GIVE_HANDLE(rcvd_evt->info.peer_msg.info.usr_msg_info.i_ckpt_hdl);
				}
				break;

			default:
				goto pr_done;
			}
		}
		break;

	default:
		break;
	}

 pr_done:
	m_NCS_UNLOCK(&mbc_reg->svc_lock, NCS_LOCK_WRITE);
	m_LOG_MBCSV_SVC_LOCK(MBCSV_LK_UNLOCKED, mbc_reg->svc_id, &mbc_reg->svc_lock);
	m_MBCSV_GIVE_HANDLE(mbcsv_hdl);

	/* free the event info */
	if (rcvd_evt)
		m_MMGR_FREE_MBCSV_EVT(rcvd_evt);

	return rc;
}

/****************************************************************************
  PROCEDURE:  : mbcsv_hdl_dispatch_one
 
  Description   : This routine dispatches one pending event and processe it.
 
  Arguments     : mbc_reg - Service instance for which dispatch is called.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 mbcsv_hdl_dispatch_one(uns32 mbcsv_hdl, SYSF_MBX mbx)
{
	MBCSV_EVT *rcvd_evt;
	uns32 rc = SA_AIS_OK;

	/* get it from the queue */
	rcvd_evt = m_MBCSV_RCV_MSG(&mbx);
	if (rcvd_evt) {
		/* process the callback */
		rc = mbcsv_process_events(rcvd_evt, mbcsv_hdl);
	}

	return rc;
}

/****************************************************************************
  PROCEDURE:  : mbcsv_hdl_dispatch_all
 
  Description   : This routine dispatches all pending events and process them.
 
  Arguments     : mbc_reg - Service instance for which dispatch is called.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 mbcsv_hdl_dispatch_all(uns32 mbcsv_hdl, SYSF_MBX mbx)
{
	MBCSV_EVT *rcvd_evt;
	uns32 rc = SA_AIS_OK;

	while ((rcvd_evt = m_MBCSV_RCV_MSG(&mbx))) {
		if (SA_AIS_OK != (rc = mbcsv_process_events(rcvd_evt, mbcsv_hdl))) {
			return rc;
		}
	}

	return rc;
}

/****************************************************************************
  PROCEDURE:  : mbcsv_hdl_dispatch_block
 
  Description   : This routine dispatches all pending events and process them.
 
  Arguments     : mbc_reg - Service instance for which dispatch is called.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 mbcsv_hdl_dispatch_block(uns32 mbcsv_hdl, SYSF_MBX mbx)
{
	MBCSV_EVT *rcvd_evt;
	uns32 rc = SA_AIS_OK;

	while ((rcvd_evt = m_MBCSV_BLK_RCV_MSG(&mbx))) {
		if (SA_AIS_OK != (rc = mbcsv_process_events(rcvd_evt, mbcsv_hdl))) {
			return rc;
		}
	}

	return SA_AIS_OK;
}
