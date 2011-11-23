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
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************
  DESCRIPTION:

  This file contains routines used by IMMND library for MDS interaction.
*****************************************************************************/

#include "immnd.h"
#include "ncs_util.h"

uns32 immnd_mds_callback(struct ncsmds_callback_info *info);

static uns32 immnd_mds_enc(IMMND_CB *cb, MDS_CALLBACK_ENC_INFO *info);
static uns32 immnd_mds_dec(IMMND_CB *cb, MDS_CALLBACK_DEC_INFO *info);

static uns32 immnd_mds_rcv(IMMND_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info);
static uns32 immnd_mds_svc_evt(IMMND_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt);
static uns32 immnd_mds_enc_flat(IMMND_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uns32 immnd_mds_dec_flat(IMMND_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);

/* Message Format Verion Tables at IMMND */
MDS_CLIENT_MSG_FORMAT_VER immnd_imma_msg_fmt_table[IMMND_WRT_IMMA_SUBPART_VER_RANGE] = {
	1
};

MDS_CLIENT_MSG_FORMAT_VER immnd_immnd_msg_fmt_table[IMMND_WRT_IMMND_SUBPART_VER_RANGE] = {
	1
};

MDS_CLIENT_MSG_FORMAT_VER immnd_immd_msg_fmt_table[IMMND_WRT_IMMD_SUBPART_VER_RANGE] = {
	1
};


SaAisErrorT immnd_mds_client_not_busy(IMMSV_SEND_INFO *s_info) 
{
	if(!(s_info->to_svc)) {
		/* No current syncronous reply pending. */
		assert(!(s_info->mSynReqCount));
		return SA_AIS_OK;
	}

	/* Client is currently blocked waiting on reply. 
	   This can happen if client has timed out in library
	   on previous syncronous call. The call is here still
	   being processed in the server. The server can then
	   only reply immediately to this client, on *this* call.

	   It could also happen if one handle is used concurrently
	   by several client threads, making concurrent synchronous calls.
	   This is not allowed though, see osaf/services/saf/immsv/README.
	*/

	if(s_info->mSynReqCount < 255) {
		s_info->mSynReqCount++;
		LOG_NO("ERR_TRY_AGAIN: Handle is busy with other syncronous call - timed out in client ?");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	LOG_NO("ERR_NO_RESOUCES: Handle use is blocked by pending reply on syncronous call");
	return SA_AIS_ERR_NO_RESOURCES;
}


/****************************************************************************
 * Name          : immnd_mds_get_handle
 *
 * Description   : This function Gets the Handles of local MDS destination
 *
 * Arguments     : cb   : IMMND control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 immnd_mds_get_handle(IMMND_CB *cb)
{
	NCSADA_INFO arg;
	uns32 rc;

	memset(&arg, 0, sizeof(NCSADA_INFO));
	arg.req = NCSADA_GET_HDLS;
	rc = ncsada_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_WA("MDS Handle Get Failed");
		return rc;
	}
	cb->immnd_mds_hdl = arg.info.adest_get_hdls.o_mds_pwe1_hdl;
	return rc;
}

/****************************************************************************
  Name          : immnd_mds_register
 
  Description   : This routine registers the IMMND Service with MDS.
 
  Arguments     : immnd_cb - ptr to the IMMND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uns32 immnd_mds_register(IMMND_CB *cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	NCSMDS_INFO svc_info;
	MDS_SVC_ID svc_id[1] = { NCSMDS_SVC_ID_IMMD };
	/*NCS_PHY_SLOT_ID phy_slot; */
	TRACE_ENTER();

	/* STEP1: Get the MDS Handle */
	rc = immnd_mds_get_handle(cb);

	if (rc != NCSCC_RC_SUCCESS)
		return rc;

	/* memset the svc_info */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));

	/* STEP 2 : Install on ADEST with MDS with service ID NCSMDS_SVC_ID_IMMND. */
	svc_info.i_mds_hdl = cb->immnd_mds_hdl;
	svc_info.i_svc_id = NCSMDS_SVC_ID_IMMND;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = 0;

	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_install.i_svc_cb = immnd_mds_callback;	/* callback */
	svc_info.info.svc_install.i_mds_q_ownership = FALSE;
	svc_info.info.svc_install.i_mds_svc_pvt_ver = IMMND_MDS_PVT_SUBPART_VERSION;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_WA("MDS Install Failed");
		return NCSCC_RC_FAILURE;
	}
	cb->immnd_mdest_id = svc_info.info.svc_install.o_dest;

	/* STEP 3: Subscribe to IMMD up/down events */
	svc_info.i_op = MDS_RED_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_WA("MDS Subscription Failed");
		goto error1;
	}

	/* STEP 4: Subscribe to IMMA_OM up/down events */
	svc_id[0] = NCSMDS_SVC_ID_IMMA_OM;
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_INTRANODE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_WA("MDS Subscription Failed");
		goto error1;
	}

	/* STEP 5: Subscribe to IMMA_OI up/down events */
	svc_id[0] = NCSMDS_SVC_ID_IMMA_OI;
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_INTRANODE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_WA("MDS Subscription Failed");
		goto error1;
	}

	cb->node_id = m_NCS_GET_NODE_ID;
	TRACE_2("cb->node_id:%x", cb->node_id);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
 error1:

	/* Uninstall with the mds */
	immnd_mds_unregister(cb);
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : immnd_mds_unregister
 *
 * Description   : This function un-registers the IMMND Service with MDS.
 *
 * Arguments     : cb   : IMMND control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void immnd_mds_unregister(IMMND_CB *cb)
{
	NCSMDS_INFO arg;
	TRACE_ENTER();

	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->immnd_mds_hdl;
	arg.i_svc_id = NCSMDS_SVC_ID_IMMND;
	arg.i_op = MDS_UNINSTALL;

	if (ncsmds_api(&arg) != NCSCC_RC_SUCCESS) {
		LOG_WA("MDS Unregister Failed");
	}
	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : immnd_mds_callback
 
  Description   : This callback routine will be called by MDS on event arrival
 
  Arguments     : info - pointer to the mds callback info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 immnd_mds_callback(struct ncsmds_callback_info *info)
{
	IMMND_CB *cb = immnd_cb;
	uns32 rc = NCSCC_RC_FAILURE;

	if (info == NULL)
		return rc;

	switch (info->i_op) {
	case MDS_CALLBACK_COPY:
		rc = NCSCC_RC_FAILURE;
		break;
	case MDS_CALLBACK_ENC:
		rc = immnd_mds_enc(cb, &info->info.enc);
		break;
	case MDS_CALLBACK_DEC:
		rc = immnd_mds_dec(cb, &info->info.dec);
		break;
	case MDS_CALLBACK_ENC_FLAT:
		if (1) {
			rc = immnd_mds_enc_flat(cb, &info->info.enc_flat);
		} else {
			rc = immnd_mds_enc(cb, &info->info.enc_flat);
		}
		break;
	case MDS_CALLBACK_DEC_FLAT:
		if (1) {
			rc = immnd_mds_dec_flat(cb, &info->info.dec_flat);
		} else {
			rc = immnd_mds_dec(cb, &info->info.dec_flat);
		}
		break;
	case MDS_CALLBACK_RECEIVE:
		rc = immnd_mds_rcv(cb, &info->info.receive);
		break;

	case MDS_CALLBACK_SVC_EVENT:
		rc = immnd_mds_svc_evt(cb, &info->info.svc_evt);
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}

	return rc;
}

/****************************************************************************
  Name          : immnd_mds_enc
 
  Description   : This function encodes an events sent from IMMND.
 
  Arguments     : cb    : IMMND control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 immnd_mds_enc(IMMND_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info)
{
	IMMSV_EVT *evt;

	/* Get the Msg Format version from the SERVICE_ID & 
	   RMT_SVC_PVT_SUBPART_VERSION */
	if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_IMMA_OM) {
		/*
		   enc_info->o_msg_fmt_ver = 
		   m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
		   IMMND_WRT_IMMA_OM_SUBPART_VER_MIN,
		   IMMND_WRT_IMMA_OM_SUBPART_VER_MAX,
		   immnd_imma_msg_fmt_table);
		 */
	} else if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_IMMA_OI) {
		/*
		   enc_info->o_msg_fmt_ver = 
		   m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
		   IMMND_WRT_IMMA_OI_SUBPART_VER_MIN,
		   IMMND_WRT_IMMA_OI_SUBPART_VER_MAX,
		   immnd_imma_msg_fmt_table);
		 */
	} else if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_IMMND) {
		/*
		   enc_info->o_msg_fmt_ver = 
		   m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
		   IMMND_WRT_IMMND_SUBPART_VER_MIN,
		   IMND_WRT_IMMND_SUBPART_VER_MAX,
		   immnd_immnd_msg_fmt_table);
		 */
	} else if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_IMMD) {
		enc_info->o_msg_fmt_ver =
		    m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
					  IMMND_WRT_IMMD_SUBPART_VER_MIN,
					  IMMND_WRT_IMMD_SUBPART_VER_MAX, immnd_immd_msg_fmt_table);
	}

	if (1 /*enc_info->o_msg_fmt_ver */ ) {	/*TODO: ABT Does not work */
		evt = (IMMSV_EVT *)enc_info->i_msg;

		return immsv_evt_enc( /*&cb->immnd_edu_hdl, */ evt, enc_info->io_uba);
	}

	/* Drop The Message - Incompatible Message Format Version */
	LOG_WA("INVALID MSG FORMAT IN ENCODE FULL");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : immnd_mds_dec
 
  Description   : This function decodes an events sent to IMMND.
 
  Arguments     : cb    : IMMND control Block.
                  info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

static uns32 immnd_mds_dec(IMMND_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info)
{

	IMMSV_EVT *evt;
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_BOOL is_valid_msg_fmt = FALSE;

	if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_IMMA_OM) {
		/*
		   is_valid_msg_fmt = 
		   m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
		   IMMND_WRT_IMMA_OM_SUBPART_VER_MIN,
		   IMMND_WRT_IMMA_OM_SUBPART_VER_MAX,
		   immnd_imma_msg_fmt_table);
		 */
	} else if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_IMMA_OI) {
		/*
		   is_valid_msg_fmt = 
		   m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
		   IMMND_WRT_IMMA_OI_SUBPART_VER_MIN,
		   IMMND_WRT_IMMA_OI_SUBPART_VER_MAX,
		   immnd_imma_msg_fmt_table);
		 */
	} else if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_IMMND) {
		/*
		   is_valid_msg_fmt = 
		   m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
		   IMMND_WRT_IMMND_SUBPART_VER_MIN,
		   IMMND_WRT_IMMND_SUBPART_VER_MAX,
		   immnd_immnd_msg_fmt_table);
		 */
	} else if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_IMMD) {
		is_valid_msg_fmt =
		    m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
					      IMMND_WRT_IMMD_SUBPART_VER_MIN,
					      IMMND_WRT_IMMD_SUBPART_VER_MAX, immnd_immd_msg_fmt_table);
	}

	if (1 /*is_valid_msg_fmt */ ) {	/* TODO: ABT Does not work */
		evt = calloc(1, sizeof(IMMSV_EVT));
		if (!evt) {
			LOG_WA("calloc failed");
			return NCSCC_RC_FAILURE;
		}

		dec_info->o_msg = (NCSCONTEXT)evt;

		rc = immsv_evt_dec( /*&cb->immnd_edu_hdl, */ dec_info->io_uba, evt);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_WA("MDS Decode Failed");
			free(dec_info->o_msg);
			dec_info->o_msg = NULL;
		}
		return rc;
	}
	/* Drop The Message - Incompatible Message Format Version */
	LOG_WA("INVALID MSG FORMAT");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : immnd_mds_enc_flat
 
  Description   : This function encodes an events sent from IMMA/IMMD.
 
  Arguments     : cb    : IMMND control Block.
                  enc_info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 immnd_mds_enc_flat(IMMND_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info)
{
	IMMSV_EVT *evt;
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_UBAID *uba = info->io_uba;

	/* as all the event structures are flat */
	/* Get the Msg Format version from the SERVICE_ID & 
	   RMT_SVC_PVT_SUBPART_VERSION */
	if (info->i_to_svc_id == NCSMDS_SVC_ID_IMMA_OM) {
		/*
		   info->o_msg_fmt_ver = 
		   m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
		   IMMND_WRT_IMMA_OM_SUBPART_VER_MIN,
		   IMMND_WRT_IMMA_OM_SUBPART_VER_MAX,
		   immnd_imma_msg_fmt_table);
		 */
	} else if (info->i_to_svc_id == NCSMDS_SVC_ID_IMMA_OI) {
		/*
		   info->o_msg_fmt_ver = 
		   m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
		   IMMND_WRT_IMMA_OI_SUBPART_VER_MIN,
		   IMMND_WRT_IMMA_OI_SUBPART_VER_MAX,
		   immnd_imma_msg_fmt_table);
		 */
	} else if (info->i_to_svc_id == NCSMDS_SVC_ID_IMMND) {
		/*
		   info->o_msg_fmt_ver = 
		   m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
		   IMMND_WRT_IMMND_SUBPART_VER_MIN,
		   IMMND_WRT_IMMND_SUBPART_VER_MAX,
		   immnd_immnd_msg_fmt_table);
		 */
	} else if (info->i_to_svc_id == NCSMDS_SVC_ID_IMMD) {
		info->o_msg_fmt_ver =
		    m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
					  IMMND_WRT_IMMD_SUBPART_VER_MIN,
					  IMMND_WRT_IMMD_SUBPART_VER_MAX, immnd_immd_msg_fmt_table);
	}

	if (1 /*info->o_msg_fmt_ver */ ) {	/* TODO: ABT Does not work */
		evt = (IMMSV_EVT *)info->i_msg;
		rc = immsv_evt_enc_flat( /*&cb->immnd_edu_hdl, */ evt, uba);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_WA("MDS Encode Flat Failed");
		}
		return rc;
	}
	/* Drop The Message  Incompatible Message Format Version */
	LOG_WA("INVALID MSG FORMAT IN ENCODE FLAT");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : immnd_mds_dec_flat
 
  Description   : This function decodes an events sent to IMMND.
 
  Arguments     : cb    : IMMND control Block.
                  dec_info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 immnd_mds_dec_flat(IMMND_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info)
{
	IMMSV_EVT *evt;
	NCS_UBAID *uba = info->io_uba;
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_BOOL is_valid_msg_fmt = FALSE;

	if (info->i_fr_svc_id == NCSMDS_SVC_ID_IMMA_OM) {
		/*
		   is_valid_msg_fmt = 
		   m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
		   IMMND_WRT_IMMA_OM_SUBPART_VER_MIN,
		   IMMND_WRT_IMMA_OM_SUBPART_VER_MAX,
		   immnd_imma_msg_fmt_table);
		 */
	} else if (info->i_fr_svc_id == NCSMDS_SVC_ID_IMMA_OI) {
		/*
		   is_valid_msg_fmt = 
		   m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
		   IMMND_WRT_IMMA_OI_SUBPART_VER_MIN,
		   IMMND_WRT_IMMA_OI_SUBPART_VER_MAX,
		   immnd_imma_msg_fmt_table);
		 */
	} else if (info->i_fr_svc_id == NCSMDS_SVC_ID_IMMND) {
		/*
		   is_valid_msg_fmt = 
		   m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
		   IMMND_WRT_IMMND_SUBPART_VER_MIN,
		   IMMND_WRT_IMMND_SUBPART_VER_MAX,
		   immnd_immnd_msg_fmt_table);
		 */
	} else if (info->i_fr_svc_id == NCSMDS_SVC_ID_IMMD) {
		is_valid_msg_fmt =
		    m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
					      IMMND_WRT_IMMD_SUBPART_VER_MIN,
					      IMMND_WRT_IMMD_SUBPART_VER_MAX, immnd_immd_msg_fmt_table);
	}

	if (1 /*is_valid_msg_fmt */ ) {	/* TODO: ABT Does not work */
		evt = calloc(1, sizeof(IMMSV_EVT));
		if (evt == NULL) {
			LOG_WA("calloc failed");
			return NCSCC_RC_FAILURE;
		}
		info->o_msg = evt;
		rc = immsv_evt_dec_flat(uba, evt);
		if (rc != NCSCC_RC_SUCCESS) {
			free(evt);
			info->o_msg = NULL;
			LOG_WA("MDS Decode Flat Failed");
		}
		return rc;
	}
	/* Drop The Message - Incompatible Message Format Version */
	LOG_WA("INVALID MSG FORMAT IN DECODE FLAT");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : immnd_mds_rcv
 *
 * Description   : MDS will call this function on receiving IMMND messages.
 *
 * Arguments     : cb - IMMND Control Block
 *                 rcv_info - MDS Receive information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 immnd_mds_rcv(IMMND_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT *pEvt = (IMMSV_EVT *)rcv_info->i_msg;
	/*IMMND_SYNC_SEND_NODE *node = NULL; */

	pEvt->sinfo.ctxt = rcv_info->i_msg_ctxt;
	pEvt->sinfo.dest = rcv_info->i_fr_dest;
	pEvt->sinfo.to_svc = rcv_info->i_fr_svc_id;
	if (rcv_info->i_rsp_reqd) {
		pEvt->sinfo.stype = MDS_SENDTYPE_SNDRSP;
	}

	/* Put it in IMMND's Event Queue */
	if (pEvt->info.immnd.type == IMMND_EVT_A2ND_IMM_INIT)
		rc = m_NCS_IPC_SEND(&cb->immnd_mbx, (NCSCONTEXT)pEvt, NCS_IPC_PRIORITY_HIGH);
	else
		rc = m_NCS_IPC_SEND(&cb->immnd_mbx, (NCSCONTEXT)pEvt, NCS_IPC_PRIORITY_NORMAL);

	if (NCSCC_RC_SUCCESS != rc) {
		LOG_WA("NCS IPC Send Failed");
	}

	return rc;
}

/****************************************************************************
 * Name          : immnd_mds_svc_evt
 *
 * Description   : IMMND is informed when MDS events occurr that he has 
 *                 subscribed to
 *
 * Arguments     : 
 *   cb          : IMMND control Block.
 *   enc_info    : Svc evt info.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 immnd_mds_svc_evt(IMMND_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{
	IMMSV_EVT *evt;
	uns32 rc = NCSCC_RC_SUCCESS, priority = NCS_IPC_PRIORITY_HIGH;
	TRACE_ENTER();

	if (svc_evt->i_svc_id == NCSMDS_SVC_ID_IMMD) {
		m_NCS_LOCK(&cb->immnd_immd_up_lock, NCS_LOCK_WRITE);

		switch (svc_evt->i_change) {
		case NCSMDS_DOWN:
			if (cb->fevs_replies_pending) {
				LOG_WA("Director Service is down,  fevs replies pending:%u "
				       "fevs highest processed:%llu", cb->fevs_replies_pending, cb->highestProcessed);
				if(cb->fevs_replies_pending) {
					LOG_IN("Resetting fevs replies pending to zero");
					cb->fevs_replies_pending = 0;
				}
			} else {
				LOG_NO("Director Service is down");
			}
			if (cb->is_immd_up == TRUE) {
				/* If IMMD is already UP */
				cb->is_immd_up = FALSE;
				m_NCS_UNLOCK(&cb->immnd_immd_up_lock, NCS_LOCK_WRITE);
				TRACE_LEAVE();
				return NCSCC_RC_SUCCESS;
			}
			break;

		case NCSMDS_UP:
			LOG_NO("Director Service is up");
			cb->is_immd_up = TRUE;
			cb->immd_mdest_id = svc_evt->i_dest;
			break;

		case NCSMDS_NO_ACTIVE:
			cb->is_immd_up = FALSE;
			if (cb->fevs_replies_pending) {
				LOG_WA("Director Service in NOACTIVE state - "
				       "fevs replies pending:%u fevs highest processed:%llu",
				       cb->fevs_replies_pending, cb->highestProcessed);
				LOG_IN("Resetting fevs replies pending to zero");
				cb->fevs_replies_pending = 0;
			} else {
				LOG_NO("Director Service in NOACTIVE state");
			}
			break;

		case NCSMDS_NEW_ACTIVE:
			cb->is_immd_up = TRUE;
			cb->immd_mdest_id = svc_evt->i_dest;
			LOG_NO("Director Service Is NEWACTIVE state");
			break;

		case NCSMDS_RED_UP:
			TRACE_2("NCSMDS_RED_UP (immd up) - doing nothing");
			cb->is_immd_up = TRUE;
			cb->immd_mdest_id = svc_evt->i_dest;
			break;

		case NCSMDS_RED_DOWN:
			TRACE_2("NCSMDS_RED_DOWN () - do nothing");
			break;

		case NCSMDS_CHG_ROLE:
			TRACE_2("NCSMDS_CHG_ROLE () - do nothing");
			break;

		default:
			break;
		}

		priority = NCS_IPC_PRIORITY_VERY_HIGH;

		m_NCS_UNLOCK(&cb->immnd_immd_up_lock, NCS_LOCK_WRITE);
	}

	/* IMMA events from other nodes can not happen */
	if ((svc_evt->i_svc_id == NCSMDS_SVC_ID_IMMA_OM) || (svc_evt->i_svc_id == NCSMDS_SVC_ID_IMMA_OI))
		assert(m_NCS_NODE_ID_FROM_MDS_DEST(cb->immnd_mdest_id) == m_NCS_NODE_ID_FROM_MDS_DEST(svc_evt->i_dest));

	/* Send the IMMND_EVT_MDS_INFO to IMMND */
	evt = calloc(1, sizeof(IMMSV_EVT));
	if (evt == NULL) {
		LOG_WA("calloc failed");
		return NCSCC_RC_FAILURE;
	}
	evt->type = IMMSV_EVT_TYPE_IMMND;
	evt->info.immnd.type = IMMND_EVT_MDS_INFO;
	evt->info.immnd.info.mds_info.change = svc_evt->i_change;
	evt->info.immnd.info.mds_info.dest = svc_evt->i_dest;
	evt->info.immnd.info.mds_info.svc_id = svc_evt->i_svc_id;
	evt->info.immnd.info.mds_info.role = svc_evt->i_role;

	/* Put it in IMMND's Event Queue */
	rc = m_NCS_IPC_SEND(&cb->immnd_mbx, (NCSCONTEXT)evt, priority);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_WA("NCS IPC Send Failed");
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : immnd_mds_send_rsp
 *
 * Description   : Send the Response to Sync Requests
 *
 * Arguments     : 
 *
 * Return Values : 
 *
 * Notes         :
 *****************************************************************************/
uns32 immnd_mds_send_rsp(IMMND_CB *cb, IMMSV_SEND_INFO *s_info, IMMSV_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uns32 rc;

	if(!(s_info->to_svc)) {
		LOG_WA(">>s_info->to_svc == 0<< reply context destroyed before "
			"this reply could be made");
		return NCSCC_RC_FAILURE;
	}

	assert(s_info->stype == MDS_SENDTYPE_SNDRSP);

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->immnd_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_IMMND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;

	mds_info.info.svc_send.i_to_svc = s_info->to_svc;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_RSP;
	mds_info.info.svc_send.info.rsp.i_msg_ctxt = s_info->ctxt;
	mds_info.info.svc_send.info.rsp.i_sender_dest = s_info->dest;

	immsv_msg_trace_send(s_info->dest, evt);

	/* send the message */
	rc = ncsmds_api(&mds_info);

	/* Destroy reply context after reply has been sent
	   Will also zero mSyncReqCount */
	memset(s_info, 0, sizeof(IMMSV_SEND_INFO));

	if (rc != NCSCC_RC_SUCCESS)
		LOG_WA("MDS Send Failed");

	return rc;
}

/****************************************************************************
  Name          : immnd_mds_msg_send
 
  Description   : This routine sends the Events from IMMND
 
  Arguments     : cb  - ptr to the IMMND CB
                  i_evt - ptr to the IMMSV message
                  o_evt - ptr to the IMMSV message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 immnd_mds_msg_send(IMMND_CB *cb, uns32 to_svc, MDS_DEST to_dest, IMMSV_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uns32 rc;

	if (!evt)
		return NCSCC_RC_FAILURE;

	m_NCS_LOCK(&cb->immnd_immd_up_lock, NCS_LOCK_WRITE);

	if ((to_svc == NCSMDS_SVC_ID_IMMD) && (cb->is_immd_up == FALSE)) {
		/* IMMD is not UP */
		TRACE_2("Director Service Is Down");
		m_NCS_UNLOCK(&cb->immnd_immd_up_lock, NCS_LOCK_WRITE);
		return NCSCC_RC_FAILURE;
	}

	m_NCS_UNLOCK(&cb->immnd_immd_up_lock, NCS_LOCK_WRITE);

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->immnd_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_IMMND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = to_svc;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	mds_info.info.svc_send.info.snd.i_to_dest = to_dest;

	immsv_msg_trace_send(to_dest, evt);

	/* send the message */
	rc = ncsmds_api(&mds_info);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_WA("MDS Send Failed to service:%s rc:%u",
		       (to_svc == NCSMDS_SVC_ID_IMMD) ? "IMMD" :
		       (to_svc == NCSMDS_SVC_ID_IMMA_OM) ? "IMMA OM" :
		       (to_svc == NCSMDS_SVC_ID_IMMA_OI) ? "IMMA OI" :
		       (to_svc == NCSMDS_SVC_ID_IMMND) ? "IMMND" : "NO SERVICE!", rc);
	}

	return rc;
}
