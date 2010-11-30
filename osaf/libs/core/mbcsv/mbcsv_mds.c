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
  This file includes the functions require for MDS interface.

  ..............................................................................

  FUNCTIONS INCLUDED in this module:
  mbcsv_mds_reg        - Register with MDS.
  mbcsv_mds_rcv        - rcv something from MDS lower layer.
  mbcsv_mds_evt        - subscription event entry point
  mbcsv_mds_enc        - MDS encode routine.
  mbcsv_mds_dec        - MDS decode routine.
  mbcsv_mds_cpy        - MDS copy routine.  
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

#include "mbcsv.h"

MDS_CLIENT_MSG_FORMAT_VER
 MBCSV_wrt_PEER_msg_fmt_array[MBCSV_WRT_PEER_SUBPART_VER_RANGE] = {
	1 /* msg format version for subpart version */
};

/****************************************************************************
  PROCEDURE     : mbcsv_mds_reg
 
  Description   : This routine registers the MBCSV Service with MDS with the 
                  pwe handle passed by MBCSv client.
 
  Arguments     : pwe_hdl - Handle passed by MBCSv client in OPEN API.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 mbcsv_mds_reg(uns32 pwe_hdl, uns32 svc_hdl, MBCSV_ANCHOR *anchor, MDS_DEST *vdest)
{
	NCSMDS_INFO svc_to_mds_info;
	MDS_SVC_ID svc_ids_array[1];

	if (NCSCC_RC_SUCCESS != mbcsv_query_mds(pwe_hdl, anchor, vdest)) {
		m_LOG_MBCSV_SVC_PRVDR(pwe_hdl, *anchor, MBCSV_SP_MDS_SUBSCR_FAILED);
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_reg: Invalid handle passed.");
	}

	/* Install mds */
	memset(&svc_to_mds_info, 0, sizeof(NCSMDS_INFO));
	svc_to_mds_info.i_mds_hdl = pwe_hdl;
	svc_to_mds_info.i_svc_id = NCSMDS_SVC_ID_MBCSV;
	svc_to_mds_info.i_op = MDS_INSTALL;
	svc_to_mds_info.info.svc_install.i_yr_svc_hdl = pwe_hdl;
	svc_to_mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	svc_to_mds_info.info.svc_install.i_svc_cb = mbcsv_mds_callback;
	svc_to_mds_info.info.svc_install.i_mds_q_ownership = FALSE;
	svc_to_mds_info.info.svc_install.i_mds_svc_pvt_ver = MBCSV_MDS_SUB_PART_VERSION;

	if (ncsmds_api(&svc_to_mds_info) != NCSCC_RC_SUCCESS) {
		m_LOG_MBCSV_SVC_PRVDR(pwe_hdl, 0, MBCSV_SP_MDS_INSTALL_FAILED);
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_reg:  MDS install failed");
	}

	/* MBCSV is subscribing for MBCSv events */
	memset(&svc_to_mds_info, 0, sizeof(NCSMDS_INFO));
	svc_to_mds_info.i_mds_hdl = pwe_hdl;
	svc_to_mds_info.i_svc_id = NCSMDS_SVC_ID_MBCSV;
	svc_to_mds_info.i_op = MDS_RED_SUBSCRIBE;
	svc_to_mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_to_mds_info.info.svc_subscribe.i_num_svcs = 1;
	svc_ids_array[0] = NCSMDS_SVC_ID_MBCSV;
	svc_to_mds_info.info.svc_subscribe.i_svc_ids = svc_ids_array;

	if (ncsmds_api(&svc_to_mds_info) != NCSCC_RC_SUCCESS) {
		mbcsv_mds_unreg(pwe_hdl);
		m_LOG_MBCSV_SVC_PRVDR(pwe_hdl, *anchor, MBCSV_SP_MDS_SUBSCR_FAILED);
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_reg: Event subscription failed!!");
	}

	m_LOG_MBCSV_SVC_PRVDR(pwe_hdl, *anchor, MBCSV_SP_MDS_INSTALL_SUCCESS);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  PROCEDURE     : mbcsv_query_mds
  
  Description   : This routine sends MDS query for getting anchor and VDEST;.
 
  Arguments     : pwe_hdl - Handle passed by MBCSv client in OPEN API.
                  anchor  - anchor value returned by MDS.
                  vdest   - VDEST value returned by MDS.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 mbcsv_query_mds(uns32 pwe_hdl, MBCSV_ANCHOR *anchor, MDS_DEST *vdest)
{
	NCSMDS_INFO mds_info;

	memset(&mds_info, 0, sizeof(mds_info));

	mds_info.i_mds_hdl = pwe_hdl;
	mds_info.i_op = MDS_QUERY_PWE;
	mds_info.i_svc_id = NCSMDS_SVC_ID_MBCSV;

	if (ncsmds_api(&mds_info) != NCSCC_RC_SUCCESS) {
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_query_mds: MDS query failed");
	}

	if ((mds_info.info.query_pwe.o_pwe_id == 0) || (mds_info.info.query_pwe.o_absolute)) {
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_query_mds: Bad handle received.");
	} else {
		*anchor = mds_info.info.query_pwe.info.virt_info.o_anc;
		memcpy(vdest, &mds_info.info.query_pwe.info.virt_info.o_vdest, sizeof(MDS_DEST));
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  PROCEDURE     : mbcsv_mds_unreg
 
  Description   : This routine unregisters the MBCSV Service from MDS on the
                  MBCSVs Vaddress 
 
  Arguments     :  pwe_hdl - Handle with whic chackpoint instance registered with MBCSv.
 
  Return Values : NONE
 
  Notes         : None.
******************************************************************************/
void mbcsv_mds_unreg(uns32 pwe_hdl)
{
	NCSMDS_INFO svc_to_mds_info;

	svc_to_mds_info.i_mds_hdl = pwe_hdl;
	svc_to_mds_info.i_svc_id = NCSMDS_SVC_ID_MBCSV;
	svc_to_mds_info.i_op = MDS_UNINSTALL;

	if (NCSCC_RC_SUCCESS != ncsmds_api(&svc_to_mds_info)) {
		m_LOG_MBCSV_SVC_PRVDR(pwe_hdl, 0, MBCSV_SP_MDS_UNINSTALL_FAILED);
	}

	m_LOG_MBCSV_SVC_PRVDR(pwe_hdl, 0, MBCSV_SP_MDS_UNINSTALL_SUCCESS);
	return;
}

/****************************************************************************
  PROCEDURE     : mbcsv_mds_send_msg
 
  Description   : This routine is use for sending the message to the MBCSv peer.
 
  Arguments     : cb - ptr to the MBCSVcontrol block
 
  Return Values : NONE
 
  Notes         : None.
******************************************************************************/
uns32 mbcsv_mds_send_msg(uns32 send_type, MBCSV_EVT *msg, CKPT_INST *ckpt, MBCSV_ANCHOR anchor)
{
	NCSMDS_INFO mds_info;

	memset(&mds_info, 0, sizeof(mds_info));

	mds_info.i_mds_hdl = ckpt->pwe_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_MBCSV;
	mds_info.i_op = MDS_SEND;

	mds_info.info.svc_send.i_msg = msg;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_MBCSV;
	mds_info.info.svc_send.i_sendtype = send_type;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;

	switch (send_type) {
	case MDS_SENDTYPE_RED:
		{
			mds_info.info.svc_send.info.red.i_to_anc = anchor;
			mds_info.info.svc_send.info.red.i_to_vdest = ckpt->my_vdest;
		}
		break;

	case MDS_SENDTYPE_REDRSP:
		{
			mds_info.info.svc_send.info.redrsp.i_time_to_wait = MBCSV_SYNC_TIMEOUT;
			mds_info.info.svc_send.info.redrsp.i_to_anc = anchor;
			mds_info.info.svc_send.info.redrsp.i_to_vdest = ckpt->my_vdest;
		}
		break;

	case MDS_SENDTYPE_RRSP:
		{
			mds_info.info.svc_send.info.rrsp.i_msg_ctxt = msg->msg_ctxt;
			mds_info.info.svc_send.info.rrsp.i_to_anc = anchor;
			mds_info.info.svc_send.info.rrsp.i_to_dest = ckpt->my_vdest;
		}
		break;

	case MDS_SENDTYPE_RBCAST:
		{
			m_NCS_MBCSV_MDS_BCAST_SEND(ckpt->pwe_hdl, msg, ckpt);
			return NCSCC_RC_SUCCESS;
		}
		break;

	default:
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_send_msg : Send type not supported by MBCSv");
	}

	if (ncsmds_api(&mds_info) == NCSCC_RC_SUCCESS) {
		/* If message is send resp  then free the message received in response  */
		if ((MDS_SENDTYPE_REDRSP == send_type) && (NULL != mds_info.info.svc_send.info.redrsp.o_rsp)) {
			m_MMGR_FREE_MBCSV_EVT(mds_info.info.svc_send.info.redrsp.o_rsp);
		}

		return NCSCC_RC_SUCCESS;
	} else {
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_send_msg : MDS send failed");
	}
}

/****************************************************************************
 * PROCEDURE     : mbcsv_mds_callback
 *
 * Description   : Call back function provided to MDS. MDS will call this
 *                 function for enc/dec/cpy/rcv/svc_evt operations.
 *
 * Arguments     : NCSMDS_CALLBACK_INFO *info: call back info.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mbcsv_mds_callback(NCSMDS_CALLBACK_INFO *cbinfo)
{
	uns32 status;

	switch (cbinfo->i_op) {
	case MDS_CALLBACK_COPY:
		status = mbcsv_mds_cpy(cbinfo->i_yr_svc_hdl, cbinfo->info.cpy.i_msg,
				       cbinfo->info.cpy.i_to_svc_id, &cbinfo->info.cpy.o_cpy,
				       cbinfo->info.cpy.i_last,
				       cbinfo->info.cpy.i_rem_svc_pvt_ver, &cbinfo->info.cpy.o_msg_fmt_ver);
		break;

	case MDS_CALLBACK_ENC:
		/* Treating both encode types as same */
		status = mbcsv_mds_enc(cbinfo->i_yr_svc_hdl, cbinfo->info.enc.i_msg,
				       cbinfo->info.enc.i_to_svc_id,
				       cbinfo->info.enc.io_uba,
				       cbinfo->info.enc.i_rem_svc_pvt_ver, &cbinfo->info.enc.o_msg_fmt_ver);
		break;

	case MDS_CALLBACK_ENC_FLAT:
		/* Treating both encode types as same */
		status = mbcsv_mds_enc(cbinfo->i_yr_svc_hdl, cbinfo->info.enc_flat.i_msg,
				       cbinfo->info.enc_flat.i_to_svc_id,
				       cbinfo->info.enc_flat.io_uba,
				       cbinfo->info.enc_flat.i_rem_svc_pvt_ver, &cbinfo->info.enc_flat.o_msg_fmt_ver);
		break;

	case MDS_CALLBACK_DEC:
		/* Treating both decode types as same */
		status = mbcsv_mds_dec(cbinfo->i_yr_svc_hdl, &cbinfo->info.dec.o_msg,
				       cbinfo->info.dec.i_fr_svc_id,
				       cbinfo->info.dec.io_uba, cbinfo->info.dec.i_msg_fmt_ver);
		break;

	case MDS_CALLBACK_DEC_FLAT:
		/* Treating both decode types as same */
		status = mbcsv_mds_dec(cbinfo->i_yr_svc_hdl, &cbinfo->info.dec_flat.o_msg,
				       cbinfo->info.dec_flat.i_fr_svc_id,
				       cbinfo->info.dec_flat.io_uba, cbinfo->info.dec_flat.i_msg_fmt_ver);
		break;

	case MDS_CALLBACK_RECEIVE:
		status = mbcsv_mds_rcv(cbinfo);
		break;

	case MDS_CALLBACK_SVC_EVENT:
		status = mbcsv_mds_evt(cbinfo->info.svc_evt, cbinfo->i_yr_svc_hdl);
		break;

		/* Fix for  MBCSv doesn't handle MDS Quiesced Ack callback */
	case MDS_CALLBACK_DIRECT_RECEIVE:
	case MDS_CALLBACK_QUIESCED_ACK:
		status = NCSCC_RC_SUCCESS;
		break;

	default:
		status = m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
					  "mbcsv_mds_callback: MBCSV callback is called with wrong operation type");
		break;
	}

	return status;
}

/****************************************************************************
 * Function Name: mbcsv_mds_rcv
 *
 * Description   : MBCSV receive callback function. On receiving message from 
 *                 peer, MBCSv will post this message to the MBCSv mailbox for
 *                 further processing.
 *
 * Arguments     : NCSMDS_CALLBACK_INFO *info: call back info.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/

uns32 mbcsv_mds_rcv(NCSMDS_CALLBACK_INFO *cbinfo)
{
	MBCSV_EVT *msg = (MBCSV_EVT *)cbinfo->info.receive.i_msg;
	SYSF_MBX mbx;
	uns32 send_pri;

	if (NULL == msg)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_rcv: NULL message received.");

	msg->rcvr_peer_key.pwe_hdl = (uns32)cbinfo->i_yr_svc_hdl;
	msg->rcvr_peer_key.peer_anchor = cbinfo->info.receive.i_fr_anc;

	/* Is used during sync send only but will be copied always. */
	msg->msg_ctxt = cbinfo->info.receive.i_msg_ctxt;

	if (0 != (mbx = mbcsv_get_mbx(msg->rcvr_peer_key.pwe_hdl, msg->rcvr_peer_key.svc_id))) {
		/* 
		 * We found out the mailbox to which we can post a message. Now construct
		 * and send this message to the mailbox.
		 */
		msg->msg_type = MBCSV_EVT_INTERNAL;

		if (msg->info.peer_msg.type == MBCSV_EVT_INTERNAL_PEER_DISC) {
			send_pri = NCS_IPC_PRIORITY_VERY_HIGH;
		} else
			send_pri = NCS_IPC_PRIORITY_NORMAL;

		if (NCSCC_RC_SUCCESS != m_MBCSV_SND_MSG(&mbx, msg, send_pri)) {
			m_MMGR_FREE_MBCSV_EVT(msg);
			return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_rcv: Message Send failed.");
		}

	}

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 * Function Name: mbcsv_mds_evt
 *
 * Description  : MBCSV is informed when MDS events occurr that he has 
 *                subscribed to. First MBCSv will post this evet to all the
 *                the services which are currently registered on this VDEST.
 *                Later on this event will get processed in the dispatch.
 *
 * Arguments     : NCSMDS_CALLBACK_INFO *info: call back info.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mbcsv_mds_evt(MDS_CALLBACK_SVC_EVENT_INFO svc_info, MDS_CLIENT_HDL yr_svc_hdl)
{
	MBCSV_EVT *evt;
	SYSF_MBX mbx;
	SS_SVC_ID svc_id = 0;
	MDS_DEST vdest;
	MBCSV_ANCHOR anchor;

	if ((svc_info.i_change != NCSMDS_RED_UP) && (svc_info.i_change != NCSMDS_RED_DOWN))
		return NCSCC_RC_SUCCESS;
	/*
	 * First find out whether MBCSv is registered on this VDEST 
	 * If no then return success.
	 */
	if (NCSCC_RC_SUCCESS != mbcsv_query_mds((uns32)svc_info.svc_pwe_hdl, &anchor, &vdest))
		return NCSCC_RC_FAILURE;

	/* If VDEST not equal, then discard message.(message from other VDESTs) */
	if (0 != memcmp(&vdest, &svc_info.i_dest, sizeof(MDS_DEST)))
		return NCSCC_RC_SUCCESS;

	/* VDEST is my VDEST. So next, check for self events */
	if (anchor == svc_info.i_anc)
		return NCSCC_RC_SUCCESS;

	if (svc_info.i_change == NCSMDS_RED_UP) {
		m_LOG_MBCSV_SVC_PRVDR(svc_info.svc_pwe_hdl, svc_info.i_anc, MBCSV_SP_MDS_RCV_EVT_UP);

		mbcsv_add_new_pwe_anc((uns32)svc_info.svc_pwe_hdl, svc_info.i_anc);
	} else {
		m_LOG_MBCSV_SVC_PRVDR(svc_info.svc_pwe_hdl, svc_info.i_anc, MBCSV_SP_MDS_RCV_EVT_DN);

		mbcsv_rmv_pwe_anc_entry((uns32)svc_info.svc_pwe_hdl, svc_info.i_anc);
	}

	while (0 != (mbx = mbcsv_get_next_entry_for_pwe((uns32)svc_info.svc_pwe_hdl, &svc_id))) {
		if (NULL == (evt = m_MMGR_ALLOC_MBCSV_EVT)) {
			return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
						"Memory allocation failure for m_MMGR_ALLOC_MBCSV_EVT");
		}

		memset(evt, '\0', sizeof(MBCSV_EVT));

		evt->msg_type = MBCSV_EVT_MDS_SUBSCR;

		evt->rcvr_peer_key.svc_id = svc_id;
		evt->rcvr_peer_key.pwe_hdl = (uns32)svc_info.svc_pwe_hdl;
		evt->rcvr_peer_key.peer_anchor = svc_info.i_anc;

		evt->info.mds_sub_evt.evt_type = svc_info.i_change;

		if (NCSCC_RC_SUCCESS != m_MBCSV_SND_MSG(&mbx, evt, NCS_IPC_PRIORITY_HIGH)) {
			m_MMGR_FREE_MBCSV_EVT(evt);
			return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "Message send failed.");
		}
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Function Name: mbcsv_mds_enc
 *
 * Description  : Encode message to tbe sent to the MBCSv peer. We are just 
 *                the internal messages which we exchange between the peers.
 *
 * Arguments     : NCSMDS_CALLBACK_INFO *info: call back info.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mbcsv_mds_enc(MDS_CLIENT_HDL yr_svc_hdl, NCSCONTEXT msg,
		    SS_SVC_ID to_svc, NCS_UBAID *uba,
		    MDS_SVC_PVT_SUB_PART_VER rem_svc_pvt_ver, MDS_CLIENT_MSG_FORMAT_VER *msg_fmt_ver)
{
	uns8 *data;
	MBCSV_EVT *mm;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	if (uba == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_enc: User buff is NULL");

	/* Set the current message format version */
	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(rem_svc_pvt_ver,
						MBCSV_WRT_PEER_SUBPART_VER_MIN,
						MBCSV_WRT_PEER_SUBPART_VER_MAX, MBCSV_wrt_PEER_msg_fmt_array);
	if (0 == msg_fmt_version) {
		char str[200];
		snprintf(str, sizeof(str), "Peer MDS Subpart version:%d not supported, message to svc-id:%d dropped", rem_svc_pvt_ver,
			to_svc);
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, str);
	}
	*msg_fmt_ver = msg_fmt_version;

	mm = (MBCSV_EVT *)msg;

	if (mm == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_enc: Message to be encoded is NULL");

	data = ncs_enc_reserve_space(uba, MBCSV_MSG_TYPE_SIZE);
	if (data == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_enc: ncs_enc_reserve_space returns NULL");

	ncs_encode_8bit(&data, mm->info.peer_msg.type);
	ncs_encode_32bit(&data, mm->rcvr_peer_key.svc_id);

	ncs_enc_claim_space(uba, MBCSV_MSG_TYPE_SIZE);

	switch (mm->info.peer_msg.type) {
	case MBCSV_EVT_INTERNAL_PEER_DISC:
		{
			data = ncs_enc_reserve_space(uba, MBCSV_MSG_SUB_TYPE);
			if (data == NULL)
				return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
							"mbcsv_mds_enc: ncs_enc_reserve_space returns NULL");

			ncs_encode_8bit(&data, mm->info.peer_msg.info.peer_disc.msg_sub_type);
			ncs_enc_claim_space(uba, MBCSV_MSG_SUB_TYPE);

			switch (mm->info.peer_msg.info.peer_disc.msg_sub_type) {
			case MBCSV_PEER_UP_MSG:
				{
					data = ncs_enc_reserve_space(uba, MBCSV_PEER_UP_MSG_SIZE);
					if (data == NULL)
						return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
									"mbcsv_mds_enc: ncs_enc_reserve_space returns NULL");
					ncs_encode_32bit(&data, mm->info.peer_msg.info.peer_disc.peer_role);
					ncs_enc_claim_space(uba, MBCSV_PEER_UP_MSG_SIZE);

					mbcsv_encode_version(uba,
							     mm->info.peer_msg.info.peer_disc.info.peer_up.
							     peer_version);

					break;
				}

			case MBCSV_PEER_DOWN_MSG:
				{
					data = ncs_enc_reserve_space(uba, MBCSV_PEER_DOWN_MSG_SIZE);
					if (data == NULL)
						return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
									"mbcsv_mds_enc: ncs_enc_reserve_space returns NULL");

					ncs_encode_32bit(&data, (uns32)mm->rcvr_peer_key.peer_inst_hdl);
					ncs_encode_32bit(&data, mm->info.peer_msg.info.peer_disc.peer_role);
					ncs_enc_claim_space(uba, MBCSV_PEER_DOWN_MSG_SIZE);

					break;
				}

			case MBCSV_PEER_INFO_MSG:
				{
					data = ncs_enc_reserve_space(uba, MBCSV_PEER_INFO_MSG_SIZE);
					if (data == NULL)
						return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
									"mbcsv_mds_enc: ncs_enc_reserve_space returns NULL");
					ncs_encode_32bit(&data,
							 mm->info.peer_msg.info.peer_disc.info.peer_info.
							 my_peer_inst_hdl);
					ncs_encode_32bit(&data, mm->info.peer_msg.info.peer_disc.peer_role);
					ncs_encode_8bit(&data,
							mm->info.peer_msg.info.peer_disc.info.peer_info.compatible);
					ncs_enc_claim_space(uba, MBCSV_PEER_INFO_MSG_SIZE);

					mbcsv_encode_version(uba,
							     mm->info.peer_msg.info.peer_disc.info.peer_up.
							     peer_version);

					break;
				}

			case MBCSV_PEER_INFO_RSP_MSG:
				{
					data = ncs_enc_reserve_space(uba, MBCSV_PEER_INFO_RSP_MSG_SIZE);
					if (data == NULL)
						return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
									"mbcsv_mds_enc: ncs_enc_reserve_space returns NULL");

					ncs_encode_32bit(&data, (uns32)mm->rcvr_peer_key.peer_inst_hdl);
					ncs_encode_32bit(&data,
							 mm->info.peer_msg.info.peer_disc.info.peer_info_rsp.
							 my_peer_inst_hdl);
					ncs_encode_32bit(&data, mm->info.peer_msg.info.peer_disc.peer_role);
					ncs_encode_8bit(&data,
							mm->info.peer_msg.info.peer_disc.info.peer_info_rsp.compatible);
					ncs_enc_claim_space(uba, MBCSV_PEER_INFO_RSP_MSG_SIZE);

					mbcsv_encode_version(uba,
							     mm->info.peer_msg.info.peer_disc.info.peer_info_rsp.
							     peer_version);

					break;
				}

			case MBCSV_PEER_CHG_ROLE_MSG:
				{
					data = ncs_enc_reserve_space(uba, MBCSV_PEER_CHG_ROLE_MSG_SIZE);
					if (data == NULL)
						return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
									"mbcsv_mds_enc: ncs_enc_reserve_space returns NULL");

					ncs_encode_32bit(&data, (uns32)mm->rcvr_peer_key.peer_inst_hdl);
					ncs_encode_32bit(&data, mm->info.peer_msg.info.peer_disc.peer_role);
					ncs_enc_claim_space(uba, MBCSV_PEER_CHG_ROLE_MSG_SIZE);

					break;
				}

			default:
				return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
							"mbcsv_mds_enc: Incorrect peer sub message type.");
			}

			break;
		}
	case MBCSV_EVT_INTERNAL_CLIENT:
		{
			data = ncs_enc_reserve_space(uba, MBCSV_INT_CLIENT_MSG_SIZE);
			if (data == NULL)
				return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
							"mbcsv_mds_enc: ncs_enc_reserve_space returns NULL");

			ncs_encode_8bit(&data, mm->info.peer_msg.info.client_msg.type.evt_type);
			ncs_encode_8bit(&data, mm->info.peer_msg.info.client_msg.action);
			ncs_encode_32bit(&data, mm->info.peer_msg.info.client_msg.reo_type);
			ncs_encode_32bit(&data, mm->info.peer_msg.info.client_msg.first_rsp);
			ncs_encode_8bit(&data, mm->info.peer_msg.info.client_msg.snd_type);
			ncs_encode_32bit(&data, (uns32)mm->rcvr_peer_key.peer_inst_hdl);
			ncs_enc_claim_space(uba, MBCSV_INT_CLIENT_MSG_SIZE);

			/* Append user buffer */
			if (mm->info.peer_msg.info.client_msg.type.evt_type != NCS_MBCSV_MSG_SYNC_SEND_RSP)
				ncs_enc_append_usrbuf(uba, mm->info.peer_msg.info.client_msg.uba.start);

			break;
		}
	default:
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_enc: Incorrect message type.");
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Function Name: mbcsv_mds_dec
 *
 * Description  : Decode message received from the MBCSv peer.
 *
 * Arguments     : NCSMDS_CALLBACK_INFO *info: call back info.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mbcsv_mds_dec(MDS_CLIENT_HDL yr_svc_hdl, NCSCONTEXT *msg,
		    SS_SVC_ID to_svc, NCS_UBAID *uba, MDS_CLIENT_MSG_FORMAT_VER msg_fmat_ver)
{
	uns8 *data;
	MBCSV_EVT *mm;
	uns8 data_buff[MBCSV_MAX_SIZE_DATA];

	if (0 == m_NCS_MSG_FORMAT_IS_VALID(msg_fmat_ver,
					   MBCSV_WRT_PEER_SUBPART_VER_MIN,
					   MBCSV_WRT_PEER_SUBPART_VER_MAX, MBCSV_wrt_PEER_msg_fmt_array)) {
		char str[200];
		snprintf(str, sizeof(str), "Msg format version:%d not supported, message from svc-id:%d dropped", msg_fmat_ver,
			to_svc);
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, str);
	}

	if (uba == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_dec : User buffer is NULL");

	if (msg == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_dec : Message is NULL");

	mm = m_MMGR_ALLOC_MBCSV_EVT;
	if (mm == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_dec : Memory allocation failed.");

	memset(mm, '\0', sizeof(MBCSV_EVT));

	*msg = mm;

	data = ncs_dec_flatten_space(uba, data_buff, MBCSV_MSG_TYPE_SIZE);
	if (data == NULL) {
		m_MMGR_FREE_MBCSV_EVT(mm);
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_dec :ncs_dec_flatten_space returns NULL");
	}

	mm->info.peer_msg.type = ncs_decode_8bit(&data);
	mm->rcvr_peer_key.svc_id = ncs_decode_32bit(&data);

	ncs_dec_skip_space(uba, MBCSV_MSG_TYPE_SIZE);

	switch (mm->info.peer_msg.type) {
	case MBCSV_EVT_INTERNAL_PEER_DISC:
		{
			data = ncs_dec_flatten_space(uba, data_buff, MBCSV_MSG_SUB_TYPE);
			if (data == NULL) {
				m_MMGR_FREE_MBCSV_EVT(mm);
				return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
							"mbcsv_mds_dec: ncs_dec_flatten_space returns NULL");
			}

			mm->info.peer_msg.info.peer_disc.msg_sub_type = ncs_decode_8bit(&data);
			ncs_dec_skip_space(uba, MBCSV_MSG_SUB_TYPE);

			switch (mm->info.peer_msg.info.peer_disc.msg_sub_type) {
			case MBCSV_PEER_UP_MSG:
				{
					data = ncs_dec_flatten_space(uba, data_buff, MBCSV_PEER_UP_MSG_SIZE);
					if (data == NULL) {
						m_MMGR_FREE_MBCSV_EVT(mm);
						return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
									"mbcsv_mds_dec: ncs_dec_flatten_space returns NULL");
					}
					mm->info.peer_msg.info.peer_disc.peer_role = ncs_decode_32bit(&data);
					ncs_dec_skip_space(uba, MBCSV_PEER_UP_MSG_SIZE);

					mbcsv_decode_version(uba,
							     &mm->info.peer_msg.info.peer_disc.info.peer_up.
							     peer_version);

					break;
				}

			case MBCSV_PEER_DOWN_MSG:
				{
					data = ncs_dec_flatten_space(uba, data_buff, MBCSV_PEER_DOWN_MSG_SIZE);
					if (data == NULL) {
						m_MMGR_FREE_MBCSV_EVT(mm);
						return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
									"mbcsv_mds_dec: ncs_dec_flatten_space returns NULL");
					}

					mm->rcvr_peer_key.peer_inst_hdl = ncs_decode_32bit(&data);
					mm->info.peer_msg.info.peer_disc.peer_role = ncs_decode_32bit(&data);
					ncs_dec_skip_space(uba, MBCSV_PEER_DOWN_MSG_SIZE);

					break;
				}

			case MBCSV_PEER_INFO_MSG:
				{
					data = ncs_dec_flatten_space(uba, data_buff, MBCSV_PEER_INFO_MSG_SIZE);
					if (data == NULL) {
						m_MMGR_FREE_MBCSV_EVT(mm);
						return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
									"mbcsv_mds_dec: ncs_dec_flatten_space returns NULL");
					}
					mm->info.peer_msg.info.peer_disc.info.peer_info.my_peer_inst_hdl =
					    ncs_decode_32bit(&data);
					mm->info.peer_msg.info.peer_disc.peer_role = ncs_decode_32bit(&data);
					mm->info.peer_msg.info.peer_disc.info.peer_info.compatible =
					    ncs_decode_8bit(&data);
					ncs_dec_skip_space(uba, MBCSV_PEER_INFO_MSG_SIZE);

					mbcsv_decode_version(uba,
							     &mm->info.peer_msg.info.peer_disc.info.peer_up.
							     peer_version);

					break;
				}

			case MBCSV_PEER_INFO_RSP_MSG:
				{
					data = ncs_dec_flatten_space(uba, data_buff, MBCSV_PEER_INFO_RSP_MSG_SIZE);
					if (data == NULL) {
						m_MMGR_FREE_MBCSV_EVT(mm);
						return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
									"mbcsv_mds_dec: ncs_dec_flatten_space returns NULL");
					}

					mm->rcvr_peer_key.peer_inst_hdl = ncs_decode_32bit(&data);
					mm->info.peer_msg.info.peer_disc.info.peer_info_rsp.my_peer_inst_hdl =
					    ncs_decode_32bit(&data);
					mm->info.peer_msg.info.peer_disc.peer_role = ncs_decode_32bit(&data);
					mm->info.peer_msg.info.peer_disc.info.peer_info_rsp.compatible =
					    ncs_decode_8bit(&data);
					ncs_dec_skip_space(uba, MBCSV_PEER_INFO_RSP_MSG_SIZE);

					mbcsv_decode_version(uba,
							     &mm->info.peer_msg.info.peer_disc.info.peer_info_rsp.
							     peer_version);

					break;
				}

			case MBCSV_PEER_CHG_ROLE_MSG:
				{
					data = ncs_dec_flatten_space(uba, data_buff, MBCSV_PEER_CHG_ROLE_MSG_SIZE);
					if (data == NULL) {
						m_MMGR_FREE_MBCSV_EVT(mm);
						return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
									"mbcsv_mds_dec: ncs_dec_flatten_space returns NULL");
					}

					mm->rcvr_peer_key.peer_inst_hdl = ncs_decode_32bit(&data);
					mm->info.peer_msg.info.peer_disc.peer_role = ncs_decode_32bit(&data);
					ncs_dec_skip_space(uba, MBCSV_PEER_CHG_ROLE_MSG_SIZE);

					break;
				}

			default:
				m_MMGR_FREE_MBCSV_EVT(mm);
				return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
							"mbcsv_mds_dec: Incorrect peer sub message type");
			}

			break;
		}
	case MBCSV_EVT_INTERNAL_CLIENT:
		{
			data = ncs_dec_flatten_space(uba, data_buff, MBCSV_INT_CLIENT_MSG_SIZE);
			if (data == NULL) {
				m_MMGR_FREE_MBCSV_EVT(mm);
				return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE,
							"mbcsv_mds_dec: ncs_dec_flatten_space returns NULL");
			}

			mm->info.peer_msg.info.client_msg.type.evt_type = ncs_decode_8bit(&data);
			mm->info.peer_msg.info.client_msg.action = ncs_decode_8bit(&data);
			mm->info.peer_msg.info.client_msg.reo_type = ncs_decode_32bit(&data);
			mm->info.peer_msg.info.client_msg.first_rsp = ncs_decode_32bit(&data);
			mm->info.peer_msg.info.client_msg.snd_type = ncs_decode_8bit(&data);
			mm->rcvr_peer_key.peer_inst_hdl = ncs_decode_32bit(&data);
			ncs_dec_skip_space(uba, MBCSV_INT_CLIENT_MSG_SIZE);

			/* Copy user buffer */
			memcpy(&mm->info.peer_msg.info.client_msg.uba, uba, sizeof(NCS_UBAID));

			break;
		}
	default:
		m_MMGR_FREE_MBCSV_EVT(mm);
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_dec: Incorrect message type");
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Function Name: mbcsv_mds_cpy
 *
 * Description  : Copy message to be sent to the peer in the same process.
 *
 * Arguments     : NCSMDS_CALLBACK_INFO *info: call back info.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
 * Notes         : None.
 *****************************************************************************/

uns32 mbcsv_mds_cpy(MDS_CLIENT_HDL yr_svc_hdl, NCSCONTEXT msg,
		    SS_SVC_ID to_svc, NCSCONTEXT *cpy,
		    NCS_BOOL last, MDS_SVC_PVT_SUB_PART_VER rem_svc_pvt_ver, MDS_CLIENT_MSG_FORMAT_VER *msg_fmt_ver)
{
	MBCSV_EVT *mm;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	if (msg == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_cpy: msg received is null");

	/* Set the current message format version */
	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(rem_svc_pvt_ver,
						MBCSV_WRT_PEER_SUBPART_VER_MIN,
						MBCSV_WRT_PEER_SUBPART_VER_MAX, MBCSV_wrt_PEER_msg_fmt_array);
	if (0 == msg_fmt_version) {
		char str[200];
		snprintf(str, sizeof(str), "Peer MDS Subpart version:%d not supported, message to svc-id:%d dropped", rem_svc_pvt_ver,
			to_svc);
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, str);
	}
	*msg_fmt_ver = msg_fmt_version;

	mm = m_MMGR_ALLOC_MBCSV_EVT;

	if (mm == NULL) {
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_mds_cpy: message allocation failed");
	}

	memset(mm, '\0', sizeof(MBCSV_EVT));
	*cpy = mm;

	/*No mem set is require here since we are copying the message */
	memcpy(mm, msg, sizeof(MBCSV_EVT));

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Function Name: mbcsv_encode_version
 *
 * Description  : Encode version information in the message to be sent to the
 *                peer.
 *
 * Arguments     : uba  - User Buffer.
 *                 version - Software version.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/

uns32 mbcsv_encode_version(NCS_UBAID *uba, uns16 version)
{
	uns8 *data;

	if (uba == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_encode_version: User buff is NULL");

	data = ncs_enc_reserve_space(uba, MBCSV_MSG_VER_SIZE);

	if (data == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_encode_version: ncs_enc_reserve_space returns NULL");

	ncs_encode_16bit(&data, version);

	ncs_enc_claim_space(uba, MBCSV_MSG_VER_SIZE);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Function Name: mbcsv_decode_version
 *
 * Description  : Decode version information from the message received from peer.
 *
 * Arguments     : uba  - User Buffer.
 *                 version - Software version.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/

uns32 mbcsv_decode_version(NCS_UBAID *uba, uns16 *version)
{
	uns8 *data;
	uns8 data_buff[MBCSV_MAX_SIZE_DATA];

	if (uba == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_decode_version : User buffer is NULL");

	data = ncs_dec_flatten_space(uba, data_buff, MBCSV_MSG_VER_SIZE);

	if (data == NULL)
		return m_MBCSV_DBG_SINK(NCSCC_RC_FAILURE, "mbcsv_decode_version :ncs_dec_flatten_space returns NULL");

	*version = ncs_decode_16bit(&data);

	ncs_dec_skip_space(uba, MBCSV_MSG_VER_SIZE);

	return NCSCC_RC_SUCCESS;
}
