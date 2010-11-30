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
..............................................................................

  FUNCTIONS INCLUDED in this module:
  ncs_mbcsv_null_func                 - Wrong event received. Log message.
  ncs_mbcsv_send_async_update         - Sends an async update.
  ncs_mbcsv_send_cold_sync_resp_cmplt - Send cold sync resp complete message.
  ncs_mbcsv_send_warm_sync_resp_cmplt - Send warm sync response complete message.
  ncs_mbcsv_send_warm_sync_resp       - Send warm sync response.
  ncs_mbcsv_send_data_resp            - Send data response message.
  ncs_mbcsv_send_data_resp_cmplt      - Send data response complete message.
  ncs_mbcsv_send_data_req             - Send data request message.
  ncs_mbcsv_send_cold_sync            - Send cold sync request message.
  ncs_mbcsv_send_cold_sync_resp       - Send cold sync response message.
  ncs_mbcsv_send_warm_sync            - Send warm sync request message.
  ncs_mbscv_rcv_decode                - Call clients decode call back.
  ncs_mbcsv_rcv_async_update          - Receive async update.
  ncs_mbcsv_rcv_cold_sync_resp        - Receive cold sync response 
  ncs_mbcsv_rcv_cold_sync_resp_cmplt  - Receive cold sync response complete.
  ncs_mbcsv_rcv_warm_sync_resp        - Receive warm sync response.
  ncs_mbcsv_rcv_warm_sync_resp_cmplt  - Receive warm sync response complete.
  ncs_mbcsv_rcv_entity_in_sync        - Receive entity in sync.
  ncs_mbcsv_rcv_data_resp             - Receive data response.
  ncs_mbcsv_rcv_data_resp_cmplt       - Receive data response complete.
  ncs_mbcsv_rcv_cold_sync             - Receive cold sync.
  ncs_mbcsv_rcv_warm_sync             - Receive wamr sync.
  ncs_mbcsv_rcv_data_req              - Receive data request.
  ncs_mbcsv_send_notify               - Send notify message.
  ncs_mbcsv_rcv_notify                - Receive notify message.
******************************************************************************/

#include "mbcsv.h"
#include "ncssysf_mem.h"

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_null_func

  DESCRIPTION:

    This function generates a protocol error message

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/

void ncs_mbcsv_null_func(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			   peer->my_ckpt_inst->pwe_hdl, "Illegal Dispatch Event:");
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_send_async_update

  DESCRIPTION:

    Sends an async update

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_send_async_update(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role, peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl, "ASYNC UPDATE to be sent");	/* added for testing */

	peer->okay_to_async_updt = TRUE;

	/* Handle the case: multiple peers for this primary PEER_INST */
}

/*****************************************************************************
*
*  PROCEDURE    ncs_mbcsv_send_cold_sync_resp_cmplt
*
*  DESCRIPTION:
*
*    Sends a cold sync response complete
*
*  ARGUMENTS:       
*        peer:   Interface to send message to.
*
*  RETURNS:          Nothing.
*
  NOTES:
*
*****************************************************************************/
void ncs_mbcsv_send_cold_sync_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			   peer->my_ckpt_inst->pwe_hdl, "Cold Sync Response complete sent.");

	mbcsv_send_msg(peer, evt, NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE);
}

/*****************************************************************************
*
*  PROCEDURE    ncs_mbcsv_send_warm_sync_resp_cmplt
*
*  DESCRIPTION:
*
*    Sends a warm sync response complete
*
*  ARGUMENTS:       
*        peer:   Interface to send message to.
*
*  RETURNS:          Nothing.
*
*  NOTES:
*
*
*****************************************************************************/
void ncs_mbcsv_send_warm_sync_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			   peer->my_ckpt_inst->pwe_hdl, "Warm Sync Response Complete sent.");

	mbcsv_send_msg(peer, evt, NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE);
}

/*****************************************************************************
*
*  PROCEDURE    ncs_mbcsv_send_warm_sync_resp
*
*  DESCRIPTION:
*
*    Sends a warm sync response 
*
*  ARGUMENTS:       
*        peer:   Interface to send message to.
*
*  RETURNS:          Nothing.
*
*  NOTES:
*
*
*****************************************************************************/
void ncs_mbcsv_send_warm_sync_resp(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			   peer->my_ckpt_inst->pwe_hdl, "Warm Sync Response sent");

	mbcsv_send_msg(peer, evt, NCSMBCSV_EVENT_WARM_SYNC_RESP);
}

/*****************************************************************************\
*
*  PROCEDURE    ncs_mbcsv_send_data_resp
*
*  DESCRIPTION:
*
*    Sends a data response 
*
*  ARGUMENTS:       
*        peer:   Interface to send message to.
*
*  RETURNS:          Nothing.
*
*  NOTES:
*
*
\*****************************************************************************/
void ncs_mbcsv_send_data_resp(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			   peer->my_ckpt_inst->pwe_hdl, "Data Response sent");

	mbcsv_send_msg(peer, evt, NCSMBCSV_EVENT_DATA_RESP);
}

/*****************************************************************************
*
*  PROCEDURE    ncs_mbcsv_send_data_resp_cmplt
*
*  DESCRIPTION:
*
*    Sends a data response complete
*
*  ARGUMENTS:       
*        peer:   Interface to send message to.
*  RETURNS:          Nothing.
*
*  NOTES:
*
*
*****************************************************************************/
void ncs_mbcsv_send_data_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			   peer->my_ckpt_inst->pwe_hdl, "Data Response Complete sent");

	mbcsv_send_msg(peer, evt, NCSMBCSV_EVENT_DATA_RESP_COMPLETE);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_send_data_req
  DESCRIPTION:

    Sends a data request
 
  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_send_data_req(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl, "Data Request sent");

	/* Change state to wait for data response */
	m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_WAIT_FOR_DATA_RESP);

	/* Start the data resp cmplt timer */
	ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_DATA_RESP_CMPLT);

	mbcsv_send_msg(peer, evt, NCSMBCSV_EVENT_DATA_REQ);

	peer->my_ckpt_inst->data_req_sent = TRUE;
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_send_cold_sync

  DESCRIPTION:

    Sends a cold sync req

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_send_cold_sync(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			   peer->my_ckpt_inst->pwe_hdl, "Cold Sync Request sent");

	mbcsv_send_msg(peer, evt, NCSMBCSV_EVENT_COLD_SYNC_REQ);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_send_cold_sync_resp

  DESCRIPTION:

    Sends a cold sync resp

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_send_cold_sync_resp(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			   peer->my_ckpt_inst->pwe_hdl, "Cold Sync Response sent");

	mbcsv_send_msg(peer, evt, NCSMBCSV_EVENT_COLD_SYNC_RESP);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_send_warm_sync

  DESCRIPTION:

    Sends a warm sync req

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_send_warm_sync(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			   peer->my_ckpt_inst->pwe_hdl, "Warm Sync Request sent");

	/* Change state to verify warm sync data - since we have it all now */
	m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_WAIT_TO_WARM_SYNC);

	mbcsv_send_msg(peer, evt, NCSMBCSV_EVENT_WARM_SYNC_REQ);
}

/*****************************************************************************

  PROCEDURE    ncs_mbscv_rcv_decode

  DESCRIPTION:

    do general MBCSV client decode callback stuff in one place.

  ARGUMENTS:       
      peer:   Peer Instance.
      evt:   MBCSV event envelope for incoming message

  RETURNS:  SUCCESS - RE says all went well
            FAILURE - RE has some problem

  NOTES:

*****************************************************************************/

uns32 ncs_mbscv_rcv_decode(PEER_INST *peer, MBCSV_EVT *evt)
{
	NCS_MBCSV_CB_ARG parg;
	uns32 status = NCSCC_RC_FAILURE;
	MBCSV_REG *mbc_inst = peer->my_ckpt_inst->my_mbcsv_inst;

	parg.i_client_hdl = peer->my_ckpt_inst->client_hdl;
	parg.i_ckpt_hdl = peer->my_ckpt_inst->ckpt_hdl;

	if (NCSMBCSV_EVENT_NOTIFY == evt->info.peer_msg.info.client_msg.type.evt_type) {
		parg.i_op = NCS_MBCSV_CBOP_NOTIFY;
		parg.info.notify.i_uba = evt->info.peer_msg.info.client_msg.uba;
		parg.info.notify.i_peer_version = peer->version;
	} else {
		parg.i_op = NCS_MBCSV_CBOP_DEC;
		parg.info.decode.i_msg_type = evt->info.peer_msg.info.client_msg.type.msg_sub_type;
		parg.info.decode.i_uba = evt->info.peer_msg.info.client_msg.uba;
		parg.info.decode.i_msg_type = evt->info.peer_msg.info.client_msg.type.msg_sub_type;
		parg.info.decode.i_peer_version = peer->version;
		parg.info.decode.i_action = evt->info.peer_msg.info.client_msg.action;
		parg.info.decode.i_reo_type = evt->info.peer_msg.info.client_msg.reo_type;
	}

	status = mbc_inst->mbcsv_cb_func(&parg);
	if (NCSMBCSV_EVENT_NOTIFY == evt->info.peer_msg.info.client_msg.type.evt_type) {
		evt->info.peer_msg.info.client_msg.uba = parg.info.notify.i_uba;
	} else {
		evt->info.peer_msg.info.client_msg.uba = parg.info.decode.i_uba;
	}

	if (status != NCSCC_RC_SUCCESS) {
		if (evt->info.peer_msg.info.client_msg.uba.ub != NULL)
			m_MMGR_FREE_BUFR_LIST(evt->info.peer_msg.info.client_msg.uba.ub);

		return m_MBCSV_DBG_SINK_SVC(NCSCC_RC_FAILURE, "Decode event failure", mbc_inst->svc_id);
	}

	/* If message is data request message then copy the data request context */
	if (NCS_MBCSV_MSG_DATA_REQ == parg.info.decode.i_msg_type) {
		peer->req_context = parg.info.decode.o_req_context;
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_async_update

  DESCRIPTION:

    Receives an async update

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_async_update(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id,
			   peer->my_ckpt_inst->pwe_hdl, "ASYNC UPDATE receivd by Standby");

	/* Now parse all the IEs */

	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS)
		return;
	else {
		if (evt->info.peer_msg.info.client_msg.snd_type == NCS_MBCSV_SND_SYNC) {
			/* Send response back to the sender */
			mbcsv_send_msg(peer, evt, NCS_MBCSV_MSG_SYNC_SEND_RSP);

		}
	}

	/* now check for subscriptions */
	m_MBCSV_CHK_SUBSCRIPTION(peer, evt->info.peer_msg.info.client_msg.msg_sub_type, NCS_MBCSV_DIR_RCVD);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_cold_sync_resp

  DESCRIPTION:

    Receives a cold sync response. 

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_cold_sync_resp(PEER_INST *peer, MBCSV_EVT *evt)
{

	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Cold Sync Response received successfully by Standby");

	/* Stop the send cold sync timer */
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_SEND_COLD_SYNC);

	/* 
	 * Check whether our decode was failed and if this is not a first
	 * message in the sequence of cold sync then return without 
	 * giving decode call-back.
	 */
	if ((peer->cold_sync_dec_fail == TRUE) && (evt->info.peer_msg.info.client_msg.first_rsp == FALSE)) {
		if (evt->info.peer_msg.info.client_msg.uba.ub != NULL)
			m_MMGR_FREE_BUFR_LIST(evt->info.peer_msg.info.client_msg.uba.ub);
		return;
	} else if (evt->info.peer_msg.info.client_msg.first_rsp == TRUE)
		peer->cold_sync_dec_fail = FALSE;

	/* Now parse all the IEs */
	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS) {
		peer->cold_sync_dec_fail = TRUE;

		/* Send Cold sync req and set FSM state to wait to cold sync */
		m_MBCSV_SEND_CLIENT_MSG(peer, NCSMBCSV_SEND_COLD_SYNC_REQ, NCS_MBCSV_ACT_DONT_CARE);

		ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_COLD_SYNC_CMPLT);

		/* When we get a cold sync response - this timer will be cancelled */
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_SEND_COLD_SYNC);

		/* This timer must be started whenever the cold sync is sent */
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_COLD_SYNC_CMPLT);

		return;
	}

	/* now check for subscriptions */
	m_MBCSV_CHK_SUBSCRIPTION(peer, evt->info.peer_msg.info.client_msg.msg_sub_type, NCS_MBCSV_DIR_RCVD);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_cold_sync_resp_cmplt

  DESCRIPTION:

    Receives a cold sync response complete. This should do the following.
      - Change state to steady_in_sync
      - Stop the send cold sync timer
      - Start the send warm sync timer

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_cold_sync_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Cold Sync Response complete recvd by Standby");
	/* 
	 * Check whether our decode was failed and if this is not a first
	 * message in the sequence of cold sync then return without 
	 * giving decode call-back.
	 */
	if ((peer->cold_sync_dec_fail == TRUE) && (evt->info.peer_msg.info.client_msg.first_rsp == FALSE)) {
		if (evt->info.peer_msg.info.client_msg.uba.ub != NULL)
			m_MMGR_FREE_BUFR_LIST(evt->info.peer_msg.info.client_msg.uba.ub);
		return;
	} else if (evt->info.peer_msg.info.client_msg.first_rsp == TRUE)
		peer->cold_sync_dec_fail = FALSE;

	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_SEND_COLD_SYNC);	/* Stop send cold sync timer */
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_COLD_SYNC_CMPLT);	/* Stop cold sync cplt timer */

	/* Now parse all the IEs */
	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS) {
		/* Send Cold sync req and set FSM state to wait to cold sync */
		m_MBCSV_SEND_CLIENT_MSG(peer, NCSMBCSV_SEND_COLD_SYNC_REQ, NCS_MBCSV_ACT_DONT_CARE);

		/* When we get a cold sync response - this timer will be cancelled */
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_SEND_COLD_SYNC);

		/* This timer must be started whenever the cold sync is sent */
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_COLD_SYNC_CMPLT);

		return;
	}

	peer->cold_sync_done = TRUE;

	m_MBCSV_CHK_SUBSCRIPTION(peer, evt->info.peer_msg.info.client_msg.msg_sub_type, NCS_MBCSV_DIR_RCVD);	/* now check for subscriptions */

	/* Change state to steady_in_sync */

	m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_STEADY_IN_SYNC);

	/* Start the send warm sync timer --- but ONLY if warm sync is enabled */

	if ((peer->my_ckpt_inst->warm_sync_on == TRUE) && (peer->my_ckpt_inst->my_role == SA_AMF_HA_STANDBY))
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_SEND_WARM_SYNC);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_warm_sync_resp

  DESCRIPTION:

    Receives a warm sync response. 

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_warm_sync_resp(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Warm Sync Response received by Standby");

	/* Stop the send warm sync timer */
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_SEND_WARM_SYNC);

	/* 
	 * Check whether our decode was failed and if this is not a first
	 * message in the sequence of warm sync then return without 
	 * giving decode call-back.
	 */
	if ((peer->warm_sync_dec_fail == TRUE) && (evt->info.peer_msg.info.client_msg.first_rsp == FALSE)) {
		if (evt->info.peer_msg.info.client_msg.uba.ub != NULL)
			m_MMGR_FREE_BUFR_LIST(evt->info.peer_msg.info.client_msg.uba.ub);
		return;
	} else if (evt->info.peer_msg.info.client_msg.first_rsp == TRUE)
		peer->warm_sync_dec_fail = FALSE;

	/* Now parse all the IEs */
	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS) {
		peer->warm_sync_dec_fail = TRUE;

		/* Stop the warm sync response complete timer */
		ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_WARM_SYNC_CMPLT);

		/* Set warm_sync_sent to FALSE since we have received the */
		/* response for the warm sync request that was sent out   */
		peer->warm_sync_sent = FALSE;

		/* Change state to verify warm sync data - since we have it all now */
		m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_VERIFY_WARM_SYNC_DATA);

		return;
	}

	/* now check for subscriptions */
	m_MBCSV_CHK_SUBSCRIPTION(peer, evt->info.peer_msg.info.client_msg.msg_sub_type, NCS_MBCSV_DIR_RCVD);

	/* Set warm_sync_sent to FALSE since we have received the
	 * response for the warm sync request that was sent out */

	peer->warm_sync_sent = FALSE;
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_warm_sync_resp_cmplt

  DESCRIPTION:

    Receives a cold sync response complete. This should do the following.
      - Change state to steady_in_sync
      - Stop the send cold sync timer
      - Start the send warm sync timer

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_warm_sync_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Warm Sync Response complete received by Standby");

	/* 
	 * Check whether our decode was failed and if this is not a first
	 * message in the sequence of warm sync then return without 
	 * giving decode call-back.
	 */
	if ((peer->warm_sync_dec_fail == TRUE) && (evt->info.peer_msg.info.client_msg.first_rsp == FALSE)) {
		if (evt->info.peer_msg.info.client_msg.uba.ub != NULL)
			m_MMGR_FREE_BUFR_LIST(evt->info.peer_msg.info.client_msg.uba.ub);
		return;
	} else if (evt->info.peer_msg.info.client_msg.first_rsp == TRUE)
		peer->warm_sync_dec_fail = FALSE;

	/* Stop send warm sync timer since a resp cmplt also serves as the response */
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_SEND_WARM_SYNC);

	/* Stop the warm sync response complete timer */
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_WARM_SYNC_CMPLT);

	/* Set warm_sync_sent to FALSE since we have received the */
	/* response for the warm sync request that was sent out   */
	peer->warm_sync_sent = FALSE;

	/* Change state to verify warm sync data - since we have it all now */
	m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_VERIFY_WARM_SYNC_DATA);

	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "The redundant entity is verifying warm sync data...");

	/* Now parse all the IEs */

	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS)
		return;

	m_MBCSV_CHK_SUBSCRIPTION(peer, evt->info.peer_msg.info.client_msg.msg_sub_type, NCS_MBCSV_DIR_RCVD);

	m_MBCSV_SEND_CLIENT_MSG(peer, NCSMBCSV_ENTITY_IN_SYNC, NCS_MBCSV_ACT_UPDATE);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_entity_in_sync

  DESCRIPTION:

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_entity_in_sync(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Entity in sync event is received. changing FSM state to study in sync.");

	/* Change state to in sync */

	m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_STEADY_IN_SYNC);

	/* Start the send warm sync timer only if warm_sync is enabled  */
	/* and the current role is Standby                               */

	if ((peer->my_ckpt_inst->warm_sync_on == TRUE) && (peer->my_ckpt_inst->my_role == SA_AMF_HA_STANDBY))
		ncs_mbcsv_start_timer(peer, NCS_MBCSV_TMR_SEND_WARM_SYNC);

}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_data_resp

  DESCRIPTION:

    Receives a cold sync response. 

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_data_resp(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Data Response received by Standby");

	/* 
	 * Check whether our decode was failed and if this is not a first
	 * message in the sequence of data rsp then return without 
	 * giving decode call-back.
	 */
	if ((peer->data_rsp_dec_fail == TRUE) && (evt->info.peer_msg.info.client_msg.first_rsp == FALSE)) {
		if (evt->info.peer_msg.info.client_msg.uba.ub != NULL)
			m_MMGR_FREE_BUFR_LIST(evt->info.peer_msg.info.client_msg.uba.ub);
		return;
	} else if (evt->info.peer_msg.info.client_msg.first_rsp == TRUE)
		peer->data_rsp_dec_fail = FALSE;

	/* Now parse all the IEs */
	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS) {
		peer->my_ckpt_inst->data_req_sent = FALSE;
		peer->data_rsp_dec_fail = TRUE;

		/* Stop the data resp cmplt timer */
		ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_DATA_RESP_CMPLT);

		/* Change state to verify warm sync data */
		m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_VERIFY_WARM_SYNC_DATA);

		return;
	}

	/* now check for subscriptions */
	m_MBCSV_CHK_SUBSCRIPTION(peer, evt->info.peer_msg.info.client_msg.msg_sub_type, NCS_MBCSV_DIR_RCVD);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_data_resp_cmplt

  DESCRIPTION:

    Receives a data response complete. This should do the following.
      - Change state to steady_in_sync
      - Stop the data resp cmplt timer
      - Start the send warm sync timer

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_data_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Data Response complete recvd by Standby");

	/* 
	 * Check whether our decode was failed and if this is not a first
	 * message in the sequence of data rsp then return without 
	 * giving decode call-back.
	 */
	if ((peer->data_rsp_dec_fail == TRUE) && (evt->info.peer_msg.info.client_msg.first_rsp == FALSE)) {
		if (evt->info.peer_msg.info.client_msg.uba.ub != NULL)
			m_MMGR_FREE_BUFR_LIST(evt->info.peer_msg.info.client_msg.uba.ub);
		return;
	} else if (evt->info.peer_msg.info.client_msg.first_rsp == TRUE)
		peer->data_rsp_dec_fail = FALSE;

	/* Stop the data resp cmplt timer */
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_DATA_RESP_CMPLT);

	/* Change state to verify warm sync data */
	m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_STBY_STATE_VERIFY_WARM_SYNC_DATA);

	/* Now parse all the IEs */

	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS) {
		peer->my_ckpt_inst->data_req_sent = FALSE;
		return;
	}

	peer->my_ckpt_inst->data_req_sent = FALSE;

	/* now check for subscriptions */
	m_MBCSV_CHK_SUBSCRIPTION(peer, evt->info.peer_msg.info.client_msg.msg_sub_type, NCS_MBCSV_DIR_RCVD);

	m_MBCSV_SEND_CLIENT_MSG(peer, NCSMBCSV_ENTITY_IN_SYNC, NCS_MBCSV_ACT_UPDATE);

}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_cold_sync

  DESCRIPTION:

    Receives a cold sync req

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_cold_sync(PEER_INST *peer, MBCSV_EVT *evt)
{
	/* COLD_SYNC_REQ tells us that Standby is starting over. */
	/* If we're in the middle of a call_again sequence, it  */
	/* should be stopped.                                   */
	ncs_mbcsv_stop_timer(peer, NCS_MBCSV_TMR_TRANSMIT);
	peer->new_msg_seq = TRUE;
	peer->call_again_reo_hdl = 0;
	peer->call_again_reo_type = 0;

	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Cold Sync Request received successfully by Active");

	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS)
		return;

	peer->c_syn_resp_process = TRUE;

	/* Change state of the ACTIVE */
	m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_KEEP_STBY_IN_SYNC);

	m_MBCSV_CHK_SUBSCRIPTION(peer, evt->info.peer_msg.info.client_msg.msg_sub_type, NCS_MBCSV_DIR_RCVD);
	m_MBCSV_SEND_CLIENT_MSG(peer, NCSMBCSV_SEND_COLD_SYNC_RESP, NCS_MBCSV_ACT_ADD);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_warm_sync

  DESCRIPTION:

    Receives a warm sync req

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_warm_sync(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Warm Sync Request received successfully by Active");

	/* Now parse all the IEs */

	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS)
		return;

	peer->new_msg_seq = TRUE;
	peer->w_syn_resp_process = TRUE;

	/* put a check that if primary receives a warm sync req. in wait_to_cold_sync */
	/* state change the state to keep_Standby_in_sync                              */

	if (peer->state == NCS_MBCSV_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC)
		m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_KEEP_STBY_IN_SYNC);

	m_MBCSV_CHK_SUBSCRIPTION(peer, evt->info.peer_msg.info.client_msg.msg_sub_type, NCS_MBCSV_DIR_RCVD);
	m_MBCSV_SEND_CLIENT_MSG(peer, NCSMBCSV_SEND_WARM_SYNC_RESP, NCS_MBCSV_ACT_UPDATE);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_data_req

  DESCRIPTION:

    Receives a data request from the Standby

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_data_req(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "Data Request received successfully by Primary");

	/* Now parse all the IEs */

	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS)
		return;

	peer->new_msg_seq = TRUE;
	peer->data_resp_process = TRUE;

	m_MBCSV_CHK_SUBSCRIPTION(peer, evt->info.peer_msg.info.client_msg.msg_sub_type, NCS_MBCSV_DIR_RCVD);

	m_MBCSV_SEND_CLIENT_MSG(peer, NCSMBCSV_SEND_DATA_RESP, NCS_MBCSV_ACT_ADD);
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_send_notify
  DESCRIPTION:

    Sends a notify

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/

void ncs_mbcsv_send_notify(PEER_INST *peer, MBCSV_EVT *evt)
{
	/* added for testing */
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "NOTIFY sent successfully");
	/*
	 ** Use exchange ID to identify sender's role.  In the case
	 ** of both primary and Standby on the same system, this is
	 ** used by receiver to determine the proper PEER_INST.
	 */

	peer->okay_to_send_ntfy = TRUE;
}

/*****************************************************************************

  PROCEDURE    ncs_mbcsv_rcv_notify

  DESCRIPTION:

    Receives a notify

  ARGUMENTS:       
        peer:   Interface to send message to.

  RETURNS:          Nothing.

  NOTES:

*****************************************************************************/
void ncs_mbcsv_rcv_notify(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "NOTIFY received successfully");

	/* Now parse all the IEs */

	if (ncs_mbscv_rcv_decode(peer, evt) != NCSCC_RC_SUCCESS)
		return;

}

/*****************************************************************************\
*
*  PROCEDURE    ncs_mbcsv_state_to_mul_act
*
*  DESCRIPTION:
*
*    Receive Multiple Active event. Transition state to Multiple Active.
*
*  ARGUMENTS:       
*        peer:   Interface to send message to.
*
*  RETURNS:          Nothing.
*
*  NOTES:
*
\*****************************************************************************/
void ncs_mbcsv_state_to_mul_act(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "State Transition to Multiple Active State.");

	m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_MULTIPLE_ACTIVE);

}

/*****************************************************************************\
*
*  PROCEDURE    ncs_mbcsv_state_to_wfcs
*
*  DESCRIPTION:
*
*    Clear Multiple Active state. Transition state to Wait for cold sync.
*
*  ARGUMENTS:       
*        peer:   Interface to send message to.
*
*  RETURNS:          Nothing.
*
*  NOTES:
*
\*****************************************************************************/
void ncs_mbcsv_state_to_wfcs(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "State Transition to Wait for Cold Sync State.");

	m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC);

}

/*****************************************************************************\
*
*  PROCEDURE    ncs_mbcsv_state_to_kstby_sync
*
*  DESCRIPTION:
*
*    Clear Multiple Active state. Transition state to Keep Standby in Sync.
*
*  ARGUMENTS:       
*        peer:   Interface to send message to.
*
*  RETURNS:          Nothing.
*
*  NOTES:
*
\*****************************************************************************/
void ncs_mbcsv_state_to_kstby_sync(PEER_INST *peer, MBCSV_EVT *evt)
{
	m_LOG_MBCSV_DBGSTR(peer->my_ckpt_inst->my_role,
			   peer->my_ckpt_inst->my_mbcsv_inst->svc_id, peer->my_ckpt_inst->pwe_hdl,
			   "State Transition to Keep Standby in Sync State.");

	m_SET_NCS_MBCSV_STATE(peer, NCS_MBCSV_ACT_STATE_KEEP_STBY_IN_SYNC);

}
