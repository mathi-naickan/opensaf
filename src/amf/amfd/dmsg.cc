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

  This module does the encode/decode of messages between the two
  Availability Directors.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include "amf/amfd/amfd.h"
#include "base/logtrace.h"
#include "role.h"

/*****************************************************************************
 * Function: avd_mds_d_enc
 *
 * Purpose:  This is a callback routine that is invoked to encode
 *           AvD-AvD messages.
 *
 * Input: 
 *
 * Returns: None. 
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

void avd_mds_d_enc(MDS_CALLBACK_ENC_INFO *enc_info)
{
	NCS_UBAID *uba = nullptr;
	uint8_t *data;
	AVD_D2D_MSG *msg = 0;

	msg = (AVD_D2D_MSG *)enc_info->i_msg;
	uba = enc_info->io_uba;

	data = ncs_enc_reserve_space(uba, 3 * sizeof(uint32_t));
	ncs_encode_32bit(&data, msg->msg_type);
	switch (msg->msg_type) {
	case AVD_D2D_CHANGE_ROLE_REQ:
		ncs_encode_32bit(&data, msg->msg_info.d2d_chg_role_req.cause);
		ncs_encode_32bit(&data, msg->msg_info.d2d_chg_role_req.role);
		break;
	case AVD_D2D_CHANGE_ROLE_RSP:
		ncs_encode_32bit(&data, msg->msg_info.d2d_chg_role_rsp.role);
		ncs_encode_32bit(&data, msg->msg_info.d2d_chg_role_rsp.status);
		break;
	default:
		LOG_ER("%s: unknown msg %u", __FUNCTION__, msg->msg_type);
		break;
	}
	ncs_enc_claim_space(uba, 3 * sizeof(uint32_t));
}

/*****************************************************************************
 * Function: avd_mds_d_dec
 *
 * Purpose:  This is a callback routine that is invoked to decode
 *           AvD-AvD messages.
 *
 * Input: 
 *
 * Returns: 
 *
 * NOTES:
 *
 * 
 **************************************************************************/

void avd_mds_d_dec(MDS_CALLBACK_DEC_INFO *dec_info)
{
	uint8_t *data;
	uint8_t data_buff[24];
	AVD_D2D_MSG *d2d_msg = 0;
	NCS_UBAID *uba = dec_info->io_uba;

	d2d_msg = new AVD_D2D_MSG();

	data = ncs_dec_flatten_space(uba, data_buff, 3 * sizeof(uint32_t));
	d2d_msg->msg_type = static_cast<AVD_D2D_MSG_TYPE>(ncs_decode_32bit(&data));

	switch (d2d_msg->msg_type) {
	case AVD_D2D_CHANGE_ROLE_REQ:
		d2d_msg->msg_info.d2d_chg_role_req.cause = static_cast<AVD_ROLE_CHG_CAUSE_T>(ncs_decode_32bit(&data));
	d2d_msg->msg_info.d2d_chg_role_req.role = static_cast<SaAmfHAStateT>(ncs_decode_32bit(&data));
		break;
	case AVD_D2D_CHANGE_ROLE_RSP:
		d2d_msg->msg_info.d2d_chg_role_rsp.role = static_cast<SaAmfHAStateT>(ncs_decode_32bit(&data));
		d2d_msg->msg_info.d2d_chg_role_rsp.status = ncs_decode_32bit(&data);
		break;
	default:
		LOG_ER("%s: unknown msg %u", __FUNCTION__, d2d_msg->msg_type);
		break;
	}

	ncs_dec_skip_space(uba, 3 * sizeof(uint32_t));
	dec_info->o_msg = (NCSCONTEXT)d2d_msg;
}

/****************************************************************************
  Name          : avd_d2d_msg_snd

  Description   : This is a routine invoked by AVD for transmitting
  messages to the other director.
 
  Arguments     : cb     :  The control block of AvD.
                  snd_msg: the send message that needs to be sent.
                  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uint32_t avd_d2d_msg_snd(AVD_CL_CB *cb, AVD_D2D_MSG *snd_msg)
{
	NCSMDS_INFO snd_mds = {0};
	uint32_t rc;

	snd_mds.i_mds_hdl = cb->adest_hdl;
	snd_mds.i_svc_id = NCSMDS_SVC_ID_AVD;
	snd_mds.i_op = MDS_SEND;
	snd_mds.info.svc_send.i_msg = (NCSCONTEXT)snd_msg;
	snd_mds.info.svc_send.i_to_svc = NCSMDS_SVC_ID_AVD;
	snd_mds.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
	snd_mds.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	snd_mds.info.svc_send.info.snd.i_to_dest = cb->other_avd_adest;

	if ((rc = ncsmds_api(&snd_mds)) != NCSCC_RC_SUCCESS)
		LOG_ER("%s: ncsmds_api failed %u", __FUNCTION__, rc);

	return rc;
}

/****************************************************************************
  Name          : avd_d2d_msg_rcv
 
  Description   : This routine is invoked by AvD when a message arrives
  from AvD. It converts the message to the corresponding event and posts
  the message to the mailbox for further processing.
 
  Arguments     : rcv_msg    -  ptr to the received message
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uint32_t avd_d2d_msg_rcv(AVD_D2D_MSG *rcv_msg)
{
	AVD_CL_CB *cb = avd_cb;

	switch (rcv_msg->msg_type) {
	case AVD_D2D_CHANGE_ROLE_REQ:
		if (SA_AMF_HA_ACTIVE == rcv_msg->msg_info.d2d_chg_role_req.role) {
			cb->swap_switch = true;
			TRACE("amfd role change req stdby -> actv, posting to mail box");
			avd_post_amfd_switch_role_change_evt(cb, SA_AMF_HA_ACTIVE);
		} else {
			/* We will never come here, request only comes for active change role */
			osafassert(0);
		}
		break;
	case AVD_D2D_CHANGE_ROLE_RSP:
		if (SA_AMF_HA_ACTIVE == rcv_msg->msg_info.d2d_chg_role_rsp.role) {
			if (NCSCC_RC_SUCCESS == rcv_msg->msg_info.d2d_chg_role_rsp.status) {
				/* Peer AMFD went to Active state successfully, make local AVD standby */
				TRACE("amfd role change rsp stdby -> actv succ, posting to mail box for qsd -> stdby");
				avd_post_amfd_switch_role_change_evt(cb, SA_AMF_HA_STANDBY);
			} else {
				/* Other AMFD rejected the active request, move the local amfd back to Active from qsd */ 
				TRACE("amfd role change rsp stdby -> actv fail, posting to mail box for qsd -> actv");
				avd_post_amfd_switch_role_change_evt(cb, SA_AMF_HA_ACTIVE);
			}
		} else {
			/* We should never fell into this else case */
			osafassert(0);
		}
		break;
	default:
		LOG_ER("%s: unknown msg %u", __FUNCTION__, rcv_msg->msg_type);
		break;
	}

	avsv_d2d_msg_free(rcv_msg);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : avsv_d2d_msg_free 
 
  Description   : This routine is invoked by AvD when a d2d_msg is to be
                  freed.

  Arguments     : d2d_msg    -  ptr to the message which has to be freed
 
  Return Values : None. 
 
  Notes         : None.
******************************************************************************/

void avsv_d2d_msg_free(AVD_D2D_MSG *d2d_msg)
{
	delete d2d_msg;
	d2d_msg = nullptr;
}
