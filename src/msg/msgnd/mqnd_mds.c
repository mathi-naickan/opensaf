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

  This file contains routines used by MQND library for MDS interaction.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "mqnd.h"

static uint32_t mqnd_mds_callback(struct ncsmds_callback_info *info);
static void mqnd_mds_cpy(MQND_CB *pMqnd, MDS_CALLBACK_COPY_INFO *cpy);
static uint32_t mqnd_mds_enc(MQND_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uint32_t mqnd_mds_dec(MQND_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uint32_t mqnd_mds_rcv(MQND_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info);
static uint32_t mqnd_mds_direct_rcv(MQND_CB *cb, MDS_CALLBACK_DIRECT_RECEIVE_INFO *direct_rcv_info);
static uint32_t mqnd_mds_svc_evt(MQND_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt);
static uint32_t mqnd_mds_get_handle(MQND_CB *cb);

/*To store the message format versions*/
MSG_FRMT_VER mqnd_mqa_msg_fmt_table[MQND_WRT_MQA_SUBPART_VER_RANGE] = { 0, 2 };	/*With version 1 it is not backward compatible */
MSG_FRMT_VER mqnd_mqnd_msg_fmt_table[MQND_WRT_MQND_SUBPART_VER_RANGE] = { 0, 2 };	/*With version 1 it is not backward compatible */
MSG_FRMT_VER mqnd_mqd_msg_fmt_table[MQND_WRT_MQD_SUBPART_VER_RANGE] = { 0, 2 };	/*With version 1 it is not backward compatible */

/****************************************************************************
 * Name          : mqnd_mds_get_handle
 *
 * Description   : This function Gets the Handles of local MDS destination
 *
 * Arguments     : cb   : MQND control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t mqnd_mds_get_handle(MQND_CB *cb)
{
	NCSADA_INFO arg;
	uint32_t rc;

	memset(&arg, 0, sizeof(NCSADA_INFO));

	arg.req = NCSADA_GET_HDLS;
	rc = ncsada_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Cannot Get MDS Handle -Failed");
		return rc;
	}
	cb->my_mds_hdl = arg.info.adest_get_hdls.o_mds_pwe1_hdl;
	TRACE_1("MDS Get Handle is Successfull");

	return rc;
}

/****************************************************************************
  Name          : mqnd_mds_register
 
  Description   : This routine registers the MQND Service with MDS.
 
  Arguments     : mqa_cb - ptr to the MQND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uint32_t mqnd_mds_register(MQND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCSMDS_INFO svc_info;
	MDS_SVC_ID svc_id[] = { NCSMDS_SVC_ID_MQD, NCSMDS_SVC_ID_MQA };
	TRACE_ENTER();

	/* STEP1: Get the MDS Handle */
	rc = mqnd_mds_get_handle(cb);

	if (rc != NCSCC_RC_SUCCESS)
		return rc;

	/* memset the svc_info */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));

	/* STEP 2 : Install on ADEST with MDS with service ID NCSMDS_SVC_ID_MQA. */
	svc_info.i_mds_hdl = cb->my_mds_hdl;
	svc_info.i_svc_id = NCSMDS_SVC_ID_MQND;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = cb->cb_hdl;
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/* node specific */
	svc_info.info.svc_install.i_svc_cb = mqnd_mds_callback;	/* callback */
	svc_info.info.svc_install.i_mds_q_ownership = false;	/* MQND owns the mds queue */
	svc_info.info.svc_install.i_mds_svc_pvt_ver = MQND_PVT_SUBPART_VERSION;

	if ((rc = ncsmds_api(&svc_info)) != NCSCC_RC_SUCCESS) {
		LOG_ER("Installing of MDS with MQND Failed");
		return NCSCC_RC_FAILURE;
	}
	TRACE_1("MDS Install is successfull");
	cb->my_dest = svc_info.info.svc_install.o_dest;

	/* STEP 3: Subscribe to MQD/MQA up/down events */
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 2;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;

	if ((rc = ncsmds_api(&svc_info)) != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS Subscribing With MQND Failed");
		/* Uninstall with the mds */
		svc_info.i_op = MDS_UNINSTALL;
		ncsmds_api(&svc_info);
		return NCSCC_RC_FAILURE;
	}
	TRACE_1("MDS Subscription to the Agent and director are successfull");
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : mqnd_mds_unregister
 *
 * Description   : This function un-registers the MQND Service with MDS.
 *
 * Arguments     : cb   : MQND control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void mqnd_mds_unregister(MQND_CB *cb)
{
	NCSMDS_INFO arg;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->my_mds_hdl;
	arg.i_svc_id = NCSMDS_SVC_ID_MQND;
	arg.i_op = MDS_UNINSTALL;

	if ((rc = ncsmds_api(&arg)) != NCSCC_RC_SUCCESS) {
		LOG_ER("Unregestreing With MDS Failed");
	}
	TRACE_1("MDS Unreging is Successfull");
	return;
}

/****************************************************************************
  Name          : mqnd_mds_callback
 
  Description   : This callback routine will be called by MDS on event arrival
 
  Arguments     : info - pointer to the mds callback info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t mqnd_mds_callback(struct ncsmds_callback_info *info)
{
	MQND_CB *cb = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (info == NULL) {
		LOG_ER("MDS event is NULL");
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	cb = (MQND_CB *)ncshm_take_hdl(NCS_SERVICE_ID_MQND, (uint32_t)info->i_yr_svc_hdl);
	if (!cb) {
		LOG_ER("%s:%u: Cb Take Failed", __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	switch (info->i_op) {
	case MDS_CALLBACK_COPY:
		mqnd_mds_cpy(cb, &info->info.cpy);
		break;

	case MDS_CALLBACK_ENC:
		rc = mqnd_mds_enc(cb, &info->info.enc);
		break;
	case MDS_CALLBACK_DEC:
		rc = mqnd_mds_dec(cb, &info->info.dec);
		break;
	case MDS_CALLBACK_ENC_FLAT:
		rc = mqnd_mds_enc(cb, &info->info.enc_flat);
		break;
	case MDS_CALLBACK_DEC_FLAT:
		rc = mqnd_mds_dec(cb, &info->info.dec_flat);
		break;
	case MDS_CALLBACK_RECEIVE:
		rc = mqnd_mds_rcv(cb, &info->info.receive);
		break;

	case MDS_CALLBACK_SVC_EVENT:
		rc = mqnd_mds_svc_evt(cb, &info->info.svc_evt);
		break;

	case MDS_CALLBACK_DIRECT_RECEIVE:
		rc = mqnd_mds_direct_rcv(cb, &info->info.direct_receive);
		break;

	default:
		TRACE_2("MDS callback does not match with clbk type");
		rc = NCSCC_RC_FAILURE;
		break;
	}
	if (rc == NCSCC_RC_SUCCESS)
		TRACE_1("MDS callback is completed with clbk type as %d",info->i_op);
	else
		LOG_ER("MDS callback is failed with clbk type as %d",info->i_op);
	ncshm_give_hdl((uint32_t)info->i_yr_svc_hdl);
	return rc;
}

/****************************************************************************\
 PROCEDURE NAME : mqnd_mds_cpy

 DESCRIPTION    : This rountine is invoked when MQND sender & receiver both on 
                  the lies in the same process space.

 ARGUMENTS      : pMqd : MQD control Block.
                  cpy  : copy info.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/

static void mqnd_mds_cpy(MQND_CB *pMqnd, MDS_CALLBACK_COPY_INFO *cpy)
{
	MQSV_EVT *pEvt = 0;

	pEvt = m_MMGR_ALLOC_MQSV_EVT(NCS_SERVICE_ID_MQND);
	if (pEvt) {
		memcpy(pEvt, cpy->i_msg, sizeof(MQSV_EVT));
		if (MQSV_EVT_ASAPI == pEvt->type) {
			pEvt->msg.asapi->usg_cnt++;	/* Increment the use count */
		}
	} else {
		LOG_CR("Event Database Creation Failed");
	}

	cpy->o_cpy = pEvt;
	return;

}

/****************************************************************************
  Name          : mqnd_mds_enc
 
  Description   : This function encodes an events sent from MQND.
 
  Arguments     : cb    : MQND control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t mqnd_mds_enc(MQND_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info)
{
	MQSV_EVT *msg_ptr;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	msg_ptr = (MQSV_EVT *)enc_info->i_msg;

	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	switch (enc_info->i_to_svc_id) {
	case NCSMDS_SVC_ID_MQA:
		enc_info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
								MQND_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT,
								MQND_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT,
								mqnd_mqa_msg_fmt_table);
		break;

	case NCSMDS_SVC_ID_MQND:
		enc_info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
								MQND_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT,
								MQND_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT,
								mqnd_mqnd_msg_fmt_table);
		break;

	case NCSMDS_SVC_ID_MQD:
		enc_info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
								MQND_WRT_MQD_SUBPART_VER_AT_MIN_MSG_FMT,
								MQND_WRT_MQD_SUBPART_VER_AT_MAX_MSG_FMT,
								mqnd_mqd_msg_fmt_table);
		break;

	default:
		LOG_ER("MDS Encode type does not match %d", enc_info->i_to_svc_id);
		return NCSCC_RC_FAILURE;
	}

	if (enc_info->o_msg_fmt_ver) {
		rc = (m_NCS_EDU_EXEC(&cb->edu_hdl, mqsv_edp_mqsv_evt,
				     enc_info->io_uba, EDP_OP_TYPE_ENC, msg_ptr, &ederror));
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("MDS Encode Failed");
		}
		return rc;
	} else {
		/* Drop The Message */
		LOG_ER("mqnd_mds_enc:INVALID MSG FORMAT %d", enc_info->o_msg_fmt_ver);
		return NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
  Name          : mqnd_mds_dec
 
  Description   : This function decodes an events sent to MQND.
 
  Arguments     : cb    : MQND control Block.
                  info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t mqnd_mds_dec(MQND_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info)
{

	MQSV_EVT *msg_ptr;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	bool is_valid_msg_fmt;
	switch (dec_info->i_fr_svc_id) {
	case NCSMDS_SVC_ID_MQA:
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
							     MQND_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT,
							     MQND_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT,
							     mqnd_mqa_msg_fmt_table);
		break;

	case NCSMDS_SVC_ID_MQND:
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
							     MQND_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT,
							     MQND_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT,
							     mqnd_mqnd_msg_fmt_table);
		break;

	case NCSMDS_SVC_ID_MQD:
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
							     MQND_WRT_MQD_SUBPART_VER_AT_MIN_MSG_FMT,
							     MQND_WRT_MQD_SUBPART_VER_AT_MAX_MSG_FMT,
							     mqnd_mqd_msg_fmt_table);
		break;

	default:
		LOG_ER("MDS Decode type does not match %d", dec_info->i_fr_svc_id);
		return NCSCC_RC_FAILURE;
	}

	if (is_valid_msg_fmt && (dec_info->i_msg_fmt_ver != 1)) {
		msg_ptr = m_MMGR_ALLOC_MQSV_EVT(NCS_SERVICE_ID_MQND);
		if (!msg_ptr) {
			LOG_CR("Event Database Creation Failed");
			return NCSCC_RC_FAILURE;
		}

		memset(msg_ptr, 0, sizeof(MQSV_EVT));
		dec_info->o_msg = (NCSCONTEXT)msg_ptr;

		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, mqsv_edp_mqsv_evt,
				    dec_info->io_uba, EDP_OP_TYPE_DEC, (MQSV_EVT **)&dec_info->o_msg, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("MDS Decode Failed");
			m_MMGR_FREE_MQSV_EVT(dec_info->o_msg, NCS_SERVICE_ID_MQND);
		}
		return rc;
	} else {
		/* Drop The Message */
		LOG_ER("mqnd_mds_dec:INVALID MSG FORMAT %d", is_valid_msg_fmt);
		return NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
 * Name          : mqnd_mds_rcv
 *
 * Description   : MDS will call this function on receiving MQND/ASAPi messages.
 *
 * Arguments     : cb - MQND Control Block
 *                 rcv_info - MDS Receive information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t mqnd_mds_rcv(MQND_CB *pMqnd, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQSV_EVT *pEvt = (MQSV_EVT *)rcv_info->i_msg;

	pEvt->sinfo.ctxt = rcv_info->i_msg_ctxt;
	pEvt->sinfo.dest = rcv_info->i_fr_dest;
	pEvt->sinfo.to_svc = rcv_info->i_fr_svc_id;
	if (rcv_info->i_rsp_reqd) {
		pEvt->sinfo.stype = MDS_SENDTYPE_RSP;
	}

	/* Put it in MQND's Event Queue */
	rc = m_NCS_IPC_SEND(&pMqnd->mbx, (NCSCONTEXT)pEvt, NCS_IPC_PRIORITY_NORMAL);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_CR("Sending the event to the MQND Mail Box failed");
	}
	return rc;
}

/****************************************************************************
 * Name          : mqnd_mds_direct_rcv
 *
 * Description   : MDS will call this function on receiving MQND/ASAPi messages.
 *
 * Arguments     : cb - MQND Control Block
 *                 direct_rcv_info - MDS Direct Receive information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqnd_mds_direct_rcv(MQND_CB *pMqnd, MDS_CALLBACK_DIRECT_RECEIVE_INFO *direct_rcv_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS, is_valid_msg_fmt;
	MQSV_DSEND_EVT *pEvt = (MQSV_DSEND_EVT *)direct_rcv_info->i_direct_buff;
	bool endianness = machineEndianness();

	is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(direct_rcv_info->i_msg_fmt_ver,
						     MQND_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT,
						     MQND_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT, mqnd_mqa_msg_fmt_table);

	if (!is_valid_msg_fmt || (direct_rcv_info->i_msg_fmt_ver == 1)) {
		/* Drop The Message */
		LOG_ER("mqnd_mds_direct_rcv:INVALID MSG FORMAT %d", is_valid_msg_fmt);
		return NCSCC_RC_FAILURE;
	}

	pEvt->sinfo.ctxt = direct_rcv_info->i_msg_ctxt;
	pEvt->sinfo.dest = direct_rcv_info->i_fr_dest;
	pEvt->sinfo.to_svc = direct_rcv_info->i_fr_svc_id;
	if (direct_rcv_info->i_rsp_reqd) {
		pEvt->sinfo.stype = MDS_SENDTYPE_RSP;
	}
	TRACE_1("Direct receive with the endianness flag set to %d", endianness);

	/* If the endianess of the source is different, decode to host order */
	if (pEvt->endianness != endianness) {

		pEvt->type.raw = m_MQSV_REVERSE_ENDIAN_L(&pEvt->type, endianness);

		switch (pEvt->type.req_type) {

		case MQP_EVT_SEND_MSG:
			{
				pEvt->agent_mds_dest = m_MQSV_REVERSE_ENDIAN_LL(&pEvt->agent_mds_dest, endianness);

				pEvt->info.snd_msg.msgHandle =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.snd_msg.msgHandle, endianness);

				pEvt->info.snd_msg.queueHandle =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.snd_msg.queueHandle, endianness);

				pEvt->info.snd_msg.destination.length =
				    m_MQSV_REVERSE_ENDIAN_S(&pEvt->info.snd_msg.destination.length, endianness);

				pEvt->info.snd_msg.ackFlags =
				    m_MQSV_REVERSE_ENDIAN_L(&pEvt->info.snd_msg.ackFlags, endianness);

				pEvt->info.snd_msg.messageInfo.sendTime =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.snd_msg.messageInfo.sendTime, endianness);

				pEvt->info.snd_msg.messageInfo.sendReceive =
				    m_MQSV_REVERSE_ENDIAN_L(&pEvt->info.snd_msg.messageInfo.sendReceive, endianness);

				if (pEvt->info.snd_msg.messageInfo.sendReceive == SA_FALSE) {
					pEvt->info.snd_msg.messageInfo.sender.senderId =
					    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.snd_msg.messageInfo.sender.senderId,
								     endianness);
				} else {
					pEvt->info.snd_msg.messageInfo.sender.sender_context.sender_dest =
					    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.snd_msg.messageInfo.sender.
								     sender_context.sender_dest, endianness);

					pEvt->info.snd_msg.messageInfo.sender.sender_context.reply_buffer_size =
					    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.snd_msg.messageInfo.sender.
								     sender_context.reply_buffer_size, endianness);
				}

				pEvt->info.snd_msg.message.type =
				    m_MQSV_REVERSE_ENDIAN_L(&pEvt->info.snd_msg.message.type, endianness);

				pEvt->info.snd_msg.message.version =
				    m_MQSV_REVERSE_ENDIAN_L(&pEvt->info.snd_msg.message.version, endianness);

				pEvt->info.snd_msg.message.size =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.snd_msg.message.size, endianness);

				pEvt->info.snd_msg.message.senderName.length =
				    m_MQSV_REVERSE_ENDIAN_S(&pEvt->info.snd_msg.message.senderName.length, endianness);
			}
			break;

		case MQP_EVT_SEND_MSG_ASYNC:
			{
				pEvt->agent_mds_dest = m_MQSV_REVERSE_ENDIAN_LL(&pEvt->agent_mds_dest, endianness);

				pEvt->info.sndMsgAsync.SendMsg.msgHandle =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.sndMsgAsync.SendMsg.msgHandle, endianness);

				pEvt->info.sndMsgAsync.SendMsg.queueHandle =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.sndMsgAsync.SendMsg.queueHandle, endianness);

				pEvt->info.sndMsgAsync.SendMsg.destination.length =
				    m_MQSV_REVERSE_ENDIAN_S(&pEvt->info.sndMsgAsync.SendMsg.destination.length,
							    endianness);

				pEvt->info.sndMsgAsync.SendMsg.ackFlags =
				    m_MQSV_REVERSE_ENDIAN_L(&pEvt->info.sndMsgAsync.SendMsg.ackFlags, endianness);

				pEvt->info.sndMsgAsync.SendMsg.messageInfo.sendTime =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.sndMsgAsync.SendMsg.messageInfo.sendTime,
							     endianness);

				pEvt->info.sndMsgAsync.SendMsg.messageInfo.sendReceive =
				    m_MQSV_REVERSE_ENDIAN_L(&pEvt->info.sndMsgAsync.SendMsg.messageInfo.sendReceive,
							    endianness);

				pEvt->info.sndMsgAsync.SendMsg.messageInfo.sender.senderId =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.sndMsgAsync.SendMsg.messageInfo.sender.
							     senderId, endianness);

				pEvt->info.sndMsgAsync.SendMsg.message.type =
				    m_MQSV_REVERSE_ENDIAN_L(&pEvt->info.sndMsgAsync.SendMsg.message.type, endianness);

				pEvt->info.sndMsgAsync.SendMsg.message.version =
				    m_MQSV_REVERSE_ENDIAN_L(&pEvt->info.sndMsgAsync.SendMsg.message.version,
							    endianness);

				pEvt->info.sndMsgAsync.SendMsg.message.size =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.sndMsgAsync.SendMsg.message.size, endianness);

				pEvt->info.sndMsgAsync.SendMsg.message.senderName.length =
				    m_MQSV_REVERSE_ENDIAN_S(&pEvt->info.sndMsgAsync.SendMsg.message.senderName.length,
							    endianness);

				pEvt->info.sndMsgAsync.invocation =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.sndMsgAsync.invocation, endianness);
			}
			break;

		case MQP_EVT_STAT_UPD_REQ:
			{
				pEvt->info.statsReq.qhdl =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.statsReq.qhdl, endianness);

				pEvt->info.statsReq.size =
				    m_MQSV_REVERSE_ENDIAN_LL(&pEvt->info.statsReq.size, endianness);
			}
			break;
		default:
			LOG_ER("MQP_EVT does not match with type %d", pEvt->type.req_type);
			return NCSCC_RC_FAILURE;
		}
	}

	/* Put it in MQND's Event Queue */
	rc = m_NCS_IPC_SEND(&pMqnd->mbx, (NCSCONTEXT)pEvt, NCS_IPC_PRIORITY_NORMAL);

	if (NCSCC_RC_SUCCESS != rc) {
		LOG_CR("Sending the event to the MQND Mail Box failed");
	}
	return rc;
}

/****************************************************************************
 * Name          : mqnd_mds_svc_evt
 *
 * Description   : MQND is informed when MDS events occurr that he has 
 *                 subscribed to
 *
 * Arguments     : 
 *   cb          : MQND control Block.
 *   enc_info    : Svc evt info.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t mqnd_mds_svc_evt(MQND_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS, to_dest_slotid, o_msg_fmt_ver;
	switch (svc_evt->i_change) {
	case NCSMDS_DOWN:
		if (svc_evt->i_svc_id == NCSMDS_SVC_ID_MQD) {
			if (cb->is_mqd_up == true) {
				/* If MQD is already UP */
				cb->is_mqd_up = false;
				TRACE_1("MQD Service Went Down");
				return NCSCC_RC_SUCCESS;
			}
		} else if (svc_evt->i_svc_id == NCSMDS_SVC_ID_MQA) {
			MQSV_EVT *evt = NULL;

			/* Post the event to Clean all the Queues opened by applications 
			   on this agent */
			evt = m_MMGR_ALLOC_MQSV_EVT(NCS_SERVICE_ID_MQND);

			if (evt == NULL) {
				LOG_CR("Event Database Creation Failed");
				return NCSCC_RC_FAILURE;
			}
			evt->evt_type = MQSV_NOT_DSEND_EVENT;
			evt->type = MQSV_EVT_MQND_CTRL;
			evt->msg.mqnd_ctrl.type = MQND_CTRL_EVT_MDS_INFO;
			evt->msg.mqnd_ctrl.info.mds_info.change = svc_evt->i_change;
			evt->msg.mqnd_ctrl.info.mds_info.dest = svc_evt->i_dest;
			evt->msg.mqnd_ctrl.info.mds_info.svc_id = svc_evt->i_svc_id;
			TRACE_1("MQA is down with the nodeid as %" PRIx64, svc_evt->i_dest);

			/* Post the event to MQND Thread */
			rc = m_NCS_IPC_SEND(&cb->mbx, evt, NCS_IPC_PRIORITY_HIGH);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_CR("Sending the event to the MQND Mail Box failed %" PRIx64, svc_evt->i_dest);
			}
			/*   mqnd_proc_mqa_down(cb, &svc_evt->i_dest); */
		} else
			return NCSCC_RC_SUCCESS;
		break;
	case NCSMDS_UP:
		switch (svc_evt->i_svc_id) {
		case NCSMDS_SVC_ID_MQD:
			{
				cb->is_mqd_up = true;
				cb->mqd_dest = svc_evt->i_dest;
				TRACE_1("MQD service is up");

				to_dest_slotid = mqsv_get_phy_slot_id(svc_evt->i_dest);

				o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(svc_evt->i_rem_svc_pvt_ver,
								      MQND_WRT_MQD_SUBPART_VER_AT_MIN_MSG_FMT,
								      MQND_WRT_MQD_SUBPART_VER_AT_MAX_MSG_FMT,
								      mqnd_mqd_msg_fmt_table);

				if (!o_msg_fmt_ver)
					/*Log informing the existence of Non compatible MQD version, Slot id being logged */
					LOG_ER("Message Format Version Invalid %u", to_dest_slotid);

			}
			break;
		case NCSMDS_SVC_ID_MQA:
			{
				MQSV_EVT *evt = NULL;

				to_dest_slotid = mqsv_get_phy_slot_id(svc_evt->i_dest);

				o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(svc_evt->i_rem_svc_pvt_ver,
								      MQND_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT,
								      MQND_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT,
								      mqnd_mqa_msg_fmt_table);

				if (!o_msg_fmt_ver)
					/*Log informing the existence of Non compatible MQA version, Slot id being logged */
					LOG_ER("Message Format Version Invalid %u", to_dest_slotid);

				/* Post the event to Update the MQA list */

				evt = m_MMGR_ALLOC_MQSV_EVT(NCS_SERVICE_ID_MQND);

				if (evt == NULL) {
					LOG_CR("Event Database Creation Failed");
					return NCSCC_RC_FAILURE;
				}
				evt->evt_type = MQSV_NOT_DSEND_EVENT;
				evt->type = MQSV_EVT_MQND_CTRL;
				evt->msg.mqnd_ctrl.type = MQND_CTRL_EVT_MDS_MQA_UP_INFO;
				evt->msg.mqnd_ctrl.info.mqa_up_info.mqa_up_dest = svc_evt->i_dest;
				TRACE_1("MQA came up %" PRIx64, svc_evt->i_dest);

				/* Post the event to MQND Thread */
				rc = m_NCS_IPC_SEND(&cb->mbx, evt, NCS_IPC_PRIORITY_HIGH);
				if (rc != NCSCC_RC_SUCCESS) {
					LOG_CR("Sending the event to the MQND Mail Box failed %" PRIx64, svc_evt->i_dest);
					m_MMGR_FREE_MQSV_EVT(evt, NCS_SERVICE_ID_MQND);
					return rc;
				}
			}
			break;
		default:
			break;
		}
		break;
	case NCSMDS_NO_ACTIVE:
		cb->is_mqd_up = false;
		break;
	case NCSMDS_NEW_ACTIVE:
		cb->is_mqd_up = true;
		{
			MQSV_EVT *evt = NULL;
			evt = m_MMGR_ALLOC_MQSV_EVT(NCS_SERVICE_ID_MQND);
			if (evt == NULL) {
				cb->is_mqd_up = true;
				LOG_CR("Event Database Creation Failed");
				return NCSCC_RC_FAILURE;
			}
			memset(evt, 0, sizeof(MQSV_EVT));
			evt->evt_type = MQSV_NOT_DSEND_EVENT;
			evt->type = MQSV_EVT_MQND_CTRL;
			evt->msg.mqnd_ctrl.type = MQND_CTRL_EVT_DEFERRED_MQA_RSP;

			/* Post the event to MQND Thread */
			rc = m_NCS_IPC_SEND(&cb->mbx, evt, NCS_IPC_PRIORITY_HIGH);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_CR("Sending the event to the MQND Mail Box failed");
			}
		}
		break;
	default:
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : mqnd_mds_send_rsp
 *
 * Description   : Send the Response to Sync Requests
 *
 * Arguments     : 
 *
 * Return Values : 
 *
 * Notes         :
 *****************************************************************************/

uint32_t mqnd_mds_send_rsp(MQND_CB *cb, MQSV_SEND_INFO *s_info, MQSV_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->my_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_MQND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;

	mds_info.info.svc_send.i_to_svc = s_info->to_svc;
	mds_info.info.svc_send.i_sendtype = s_info->stype;
	mds_info.info.svc_send.info.rsp.i_msg_ctxt = s_info->ctxt;
	mds_info.info.svc_send.info.rsp.i_sender_dest = s_info->dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Queue Attribute get :Mds Send Response Failed");
	}
	return rc;
}

/****************************************************************************
 * Name          : mqnd_mds_send_rsp_direct
 *
 * Description   : Send the Response to Sync Requests
 *
 * Arguments     :
 *
 * Return Values :
 *
 * Notes         :
 *****************************************************************************/

uint32_t mqnd_mds_send_rsp_direct(MQND_CB *cb, MQSV_DSEND_INFO *s_info, MQSV_DSEND_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->my_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_MQND;
	mds_info.i_op = MDS_DIRECT_SEND;

	/* fill the send structure */
	mds_info.info.svc_direct_send.i_direct_buff = (NCSCONTEXT)evt;
	mds_info.info.svc_direct_send.i_direct_buff_len = sizeof(MQSV_DSEND_EVT);
	mds_info.info.svc_direct_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_direct_send.i_to_svc = s_info->to_svc;
	mds_info.info.svc_direct_send.i_msg_fmt_ver = evt->msg_fmt_version;
	mds_info.info.svc_direct_send.i_sendtype = s_info->stype;

	mds_info.info.svc_direct_send.info.rsp.i_msg_ctxt = s_info->ctxt;
	mds_info.info.svc_direct_send.info.rsp.i_sender_dest = s_info->dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Mds Send Response Direct Failed");
	}

	return rc;
}

/****************************************************************************
  Name          : mqnd_mds_msg_sync_send
 
  Description   : This routine sends the Sinc requests from MQND
 
  Arguments     : cb  - ptr to the MQND CB
                  i_evt - ptr to the MQSV message
                  o_evt - ptr to the MQSV message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t mqnd_mds_msg_sync_send(MQND_CB *cb, uint32_t to_svc, MDS_DEST to_dest,
			     MQSV_EVT *i_evt, MQSV_EVT **o_evt, SaTimeT timeout)
{

	NCSMDS_INFO mds_info;
	uint32_t rc;

	if (!i_evt) {
		LOG_ER("%s:%u: Event received is NULL", __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->my_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_MQND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = to_svc;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;

	/* fill the send rsp strcuture */
	mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout;	/* timeto wait in 10ms */
	mds_info.info.svc_send.info.sndrsp.i_to_dest = to_dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc == NCSCC_RC_SUCCESS)
		*o_evt = mds_info.info.svc_send.info.sndrsp.o_rsp;
	else {
		LOG_ER("%s:%u: MDS Send Failed", __FILE__, __LINE__);
	}

	return rc;
}

/****************************************************************************
  Name          : mqnd_mds_send
 
  Description   : This routine sends the Events from MQND
 
  Arguments     : cb  - ptr to the MQND CB
                  i_evt - ptr to the MQSV message
                  o_evt - ptr to the MQSV message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t mqnd_mds_send(MQND_CB *cb, uint32_t to_svc, MDS_DEST to_dest, MQSV_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;

	if (!evt) {
		LOG_ER("%s:%u: Event received is NULL", __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->my_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_MQND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = to_svc;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	mds_info.info.svc_send.info.snd.i_to_dest = to_dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("%s:%u: MDS Send Failed", __FILE__, __LINE__);
	}

	return rc;
}

/****************************************************************************
 * Name          : mqnd_mds_bcast_send
 *
 * Description   : This is the function which is used to send the message 
 *                 using MDS broadcast.
 *
 * Arguments     : mds_hdl  - MDS handle  
 *                 from_svc - From Serivce ID.
 *                 evt      - Event to be sent. 
 *                 to_svc   - To Service ID.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t mqnd_mds_bcast_send(MQND_CB *cb, MQSV_EVT *evt, NCSMDS_SVC_ID to_svc)
{

	NCSMDS_INFO info;
	uint32_t res;

	memset(&info, 0, sizeof(info));

	info.i_mds_hdl = cb->my_mds_hdl;
	info.i_op = MDS_SEND;
	info.i_svc_id = NCSMDS_SVC_ID_MQND;

	info.info.svc_send.i_msg = (NCSCONTEXT)evt;
	info.info.svc_send.i_priority = NCS_IPC_PRIORITY_HIGH;
	info.info.svc_send.i_sendtype = MDS_SENDTYPE_BCAST;
	info.info.svc_send.i_to_svc = to_svc;
	info.info.svc_send.info.bcast.i_bcast_scope = NCSMDS_SCOPE_INTRANODE;

	res = ncsmds_api(&info);
	if (res != NCSCC_RC_SUCCESS) {
		LOG_ER("Mds Broadcast Send Failed");
	}
	return (res);
}
