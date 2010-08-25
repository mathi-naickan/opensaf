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

  This file contains routines used by IMMD library for MDS interaction.

  FUNCTIONS INCLUDED in this module:
  

*****************************************************************************/

#include "immd.h"
#include "ncs_util.h"

uns32 immd_mds_callback(struct ncsmds_callback_info *info);
static uns32 immd_mds_enc(IMMD_CB *cb, MDS_CALLBACK_ENC_INFO *info);
static uns32 immd_mds_dec(IMMD_CB *cb, MDS_CALLBACK_DEC_INFO *info);
static uns32 immd_mds_enc_flat(IMMD_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uns32 immd_mds_dec_flat(IMMD_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uns32 immd_mds_rcv(IMMD_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info);
static uns32 immd_mds_svc_evt(IMMD_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt);
static uns32 immd_mds_quiesced_ack_process(IMMD_CB *cb);

/* Message Format Verion Tables at IMMD */

MDS_CLIENT_MSG_FORMAT_VER immd_immnd_msg_fmt_table[IMMD_WRT_IMMND_SUBPART_VER_RANGE] = {
	1
};

/****************************************************************************\
 PROCEDURE NAME : immd_mds_vdest_create

 DESCRIPTION    : This function Creates the Virtual destination for IMMD

 ARGUMENTS      : cb : IMMD control Block pointer.

 RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
\****************************************************************************/
uns32 immd_mds_vdest_create(IMMD_CB *cb)
{
	NCSVDA_INFO arg;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&arg, 0, sizeof(arg));

	cb->immd_dest_id = IMMD_VDEST_ID;

	arg.req = NCSVDA_VDEST_CREATE;
	arg.info.vdest_create.i_persistent = FALSE;
	arg.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;
	arg.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
	arg.info.vdest_create.info.specified.i_vdest = cb->immd_dest_id;

	/* Create VDEST */
	rc = ncsvda_api(&arg);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_WA("NCSVDA_VDEST_CREATE failed");
		return rc;
	}

	cb->mds_handle = arg.info.vdest_create.o_mds_pwe1_hdl;

	return rc;
}

/****************************************************************************
  Name          : immd_mds_register
 
  Description   : This routine registers the IMMD Service with MDS.
 
  Arguments     : mqa_cb - ptr to the IMMD control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
****************************************************************************/
uns32 immd_mds_register(IMMD_CB *cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	NCSMDS_INFO svc_info;
	MDS_SVC_ID svc_id[1] = { NCSMDS_SVC_ID_IMMND };
	MDS_SVC_ID immd_id[1] = { NCSMDS_SVC_ID_IMMD };

	/* Create the virtual Destination for  IMMD */
	rc = immd_mds_vdest_create(cb);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("IMMD - VDEST CREATE FAILED");
		return rc;
	}

	/* Set the role of MDS  ABT added new code see lgs_mds.c lgs_mds_init */
	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		TRACE_5("Set MDS role to ACTIVE");
		cb->mds_role = V_DEST_RL_ACTIVE;
	} else if (cb->ha_state == SA_AMF_HA_STANDBY) {
		TRACE_5("Set MDS role to STANDBY");
		cb->mds_role = V_DEST_RL_STANDBY;
	} else {
		TRACE_5("Could not set MDS role");
	}

	if (NCSCC_RC_SUCCESS != (rc = immd_mds_change_role(cb))) {
		LOG_ER("MDS role change to %d FAILED", cb->mds_role);
		return rc;
	}

	memset(&svc_info, 0, sizeof(NCSMDS_INFO));

	/* STEP 2 : Install with MDS with service ID NCSMDS_SVC_ID_IMMD. */
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_IMMD;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = 0;
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/*node specific */
	svc_info.info.svc_install.i_svc_cb = immd_mds_callback;	/* callback */
	svc_info.info.svc_install.i_mds_q_ownership = FALSE;
	svc_info.info.svc_install.i_mds_svc_pvt_ver = IMMD_MDS_PVT_SUBPART_VERSION;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("IMMD - MDS Install Failed");
		return NCSCC_RC_FAILURE;
	}

	/* STEP 3: Subscribe to IMMD for redundancy events */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_IMMD;
	svc_info.i_op = MDS_RED_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = immd_id;
	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("IMMD - MDS Subscribe for redundancy Failed");
		immd_mds_unregister(cb);
		return NCSCC_RC_FAILURE;
	}

	/* STEP 4: Subscribe to IMMND up/down events */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_IMMD;
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;
	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("IMMD - MDS Subscribe for IMMND up/down Failed");
		immd_mds_unregister(cb);
		return NCSCC_RC_FAILURE;
	}

	/* Note IMMD is not concerned with IMMA. */

	/* Get the node id of local IMMD */
	cb->node_id = m_NCS_GET_NODE_ID;

	cb->immd_self_id = immd_get_slot_and_subslot_id_from_node_id(cb->node_id);
	TRACE_5("NodeId:%x SelfId:%x", cb->node_id, cb->immd_self_id);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : immd_mds_change_role
 *
 * Description   : This function informs mds of our vdest role change 
 *
 * Arguments     : cb   : IMMD control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *****************************************************************************/
uns32 immd_mds_change_role(IMMD_CB *cb)
{
	NCSVDA_INFO arg;
	TRACE_ENTER();

	memset(&arg, 0, sizeof(NCSVDA_INFO));

	arg.req = NCSVDA_VDEST_CHG_ROLE;
	arg.info.vdest_chg_role.i_vdest = cb->immd_dest_id;
	arg.info.vdest_chg_role.i_new_role = cb->mds_role;

	TRACE_LEAVE();
	return ncsvda_api(&arg);
}

/****************************************************************************
 * Name          : immd_mds_unregister
 *
 * Description   : This function un-registers the IMMD Service with MDS.
 *
 * Arguments     : cb   : IMMD control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void immd_mds_unregister(IMMD_CB *cb)
{
	NCSMDS_INFO arg;
	uns32 rc;
	/* NCSVDA_INFO    vda_info; */

	TRACE_ENTER();

	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->mds_handle;
	arg.i_svc_id = NCSMDS_SVC_ID_IMMD;
	arg.i_op = MDS_UNINSTALL;

	if ((rc = ncsmds_api(&arg)) != NCSCC_RC_SUCCESS) {
		LOG_ER("IMMD - MDS Unregister Failed rc:%u", rc);
		goto done;
	}
#if 0				/*ABT This was tentatively added in IMMsv by me.
				   But seeing as the CPSv does not have it and that is apparently
				   not a bug, then IMMSv should also not have it. But why then
				   does LOGSv have it ??! */

	/* Destroy the virtual Destination of IMMD */
	memset(&vda_info, 0, sizeof(NCSVDA_INFO));
	vda_info.req = NCSVDA_VDEST_DESTROY;
	vda_info.info.vdest_destroy.i_vdest = cb->immd_dest_id;

	if (NCSCC_RC_SUCCESS != (rc = ncsvda_api(&vda_info))) {
		LOG_ER("NCSVDA_VDEST_DESTROY failed rc:%u", rc);
	}
#endif

 done:
	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : immd_mds_callback
 
  Description   : This callback routine will be called by MDS on event arrival
 
  Arguments     : info - pointer to the mds callback info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
*****************************************************************************/
uns32 immd_mds_callback(struct ncsmds_callback_info *info)
{
	IMMD_CB *cb = immd_cb;
	uns32 rc = NCSCC_RC_SUCCESS;

	assert(info != NULL);

	switch (info->i_op) {
	case MDS_CALLBACK_COPY:
		rc = NCSCC_RC_FAILURE;
		break;
	case MDS_CALLBACK_ENC:
		rc = immd_mds_enc(cb, &info->info.enc);
		break;
	case MDS_CALLBACK_DEC:
		rc = immd_mds_dec(cb, &info->info.dec);
		break;
	case MDS_CALLBACK_ENC_FLAT:
		if (1) {	/*Set to zero for righorous testing of byteorder enc/dec. */
			rc = immd_mds_enc_flat(cb, &info->info.enc_flat);
		} else {
			rc = immd_mds_enc(cb, &info->info.enc_flat);
		}
		break;
	case MDS_CALLBACK_DEC_FLAT:
		if (1) {	/*Set to zero for righorous testing of byteorder enc/dec. */
			rc = immd_mds_dec_flat(cb, &info->info.dec_flat);
		} else {
			rc = immd_mds_dec(cb, &info->info.dec_flat);
		}
		break;
	case MDS_CALLBACK_RECEIVE:
		rc = immd_mds_rcv(cb, &info->info.receive);
		break;
	case MDS_CALLBACK_SVC_EVENT:
		rc = immd_mds_svc_evt(cb, &info->info.svc_evt);
		break;
	case MDS_CALLBACK_QUIESCED_ACK:
		rc = immd_mds_quiesced_ack_process(cb);
		break;
	case MDS_CALLBACK_DIRECT_RECEIVE:
		rc = NCSCC_RC_FAILURE;
		TRACE_1("MDS_CALLBACK_DIRECT_RECEIVE - do nothing");
		break;
	default:
		LOG_ER("Illegal type of MDS message");
		rc = NCSCC_RC_FAILURE;
		break;
	}

	return rc;
}

/****************************************************************************
  Name          : immd_mds_enc
 
  Description   : This function encodes an events sent from IMMD.
 
  Arguments     : cb    : IMMD control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 immd_mds_enc(IMMD_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info)
{
	IMMSV_EVT *evt;

	/* Get the Msg Format version from the SERVICE_ID & 
	   RMT_SVC_PVT_SUBPART_VERSION */
	if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_IMMND) {
		enc_info->o_msg_fmt_ver =
		    m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
					  IMMD_WRT_IMMND_SUBPART_VER_MIN,
					  IMMD_WRT_IMMND_SUBPART_VER_MAX, immd_immnd_msg_fmt_table);
	} else if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_IMMA_OM) {
		LOG_WA("IMMD can not communicate directly with IMMA OM");
	} else if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_IMMA_OI) {
		LOG_WA("IMMD can not communicate directly with IMMA OI");
	}

	if (1 /*enc_info->o_msg_fmt_ver */ ) {	/*Does not work. */

		evt = (IMMSV_EVT *)enc_info->i_msg;

		return immsv_evt_enc( /*&cb->edu_hdl, */ evt, enc_info->io_uba);

	}

	/* Drop The Message,Format Version Invalid */
	LOG_ER("INVALID MSG FORMAT IN ENC");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : immd_mds_dec
 
  Description   : This function decodes an events sent to IMMD.
 
  Arguments     : cb    : IMMD control Block.
                  info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 immd_mds_dec(IMMD_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info)
{
	IMMSV_EVT *evt;
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_BOOL is_valid_msg_fmt = FALSE;

	if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_IMMND) {
		is_valid_msg_fmt =
		    m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
					      IMMD_WRT_IMMND_SUBPART_VER_MIN,
					      IMMD_WRT_IMMND_SUBPART_VER_MAX, immd_immnd_msg_fmt_table);
	}

	if (1 /*is_valid_msg_fmt */ ) {	/*Does not work. */
		evt = calloc(1, sizeof(IMMSV_EVT));
		if (!evt)
			return NCSCC_RC_FAILURE;

		dec_info->o_msg = (NCSCONTEXT)evt;

		rc = immsv_evt_dec( /*&cb->edu_hdl, */ dec_info->io_uba, evt);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("IMMD - MDS Decode Failed");
			free(dec_info->o_msg);
			dec_info->o_msg = NULL;
		}
		return rc;
	}

	LOG_ER("INVALID MSG FORMAT VERSION IN DECODE FULL, "
	       "VER %u SVC_ID  %u", dec_info->i_msg_fmt_ver, dec_info->i_fr_svc_id);

	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : immd_mds_enc_flat
 
  Description   : This function encodes an events sent from IMMD.
 
  Arguments     : cb    : IMMD control Block.
                  enc_info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
*****************************************************************************/
static uns32 immd_mds_enc_flat(IMMD_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info)
{
	IMMSV_EVT *evt = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_UBAID *uba = info->io_uba;

	/* Get the Msg Format version from the SERVICE_ID & 
	   RMT_SVC_PVT_SUBPART_VERSION */

	if (info->i_to_svc_id == NCSMDS_SVC_ID_IMMND) {
		info->o_msg_fmt_ver =
		    m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
					  IMMD_WRT_IMMND_SUBPART_VER_MIN,
					  IMMD_WRT_IMMND_SUBPART_VER_MAX, immd_immnd_msg_fmt_table);
	} else if (info->i_to_svc_id == NCSMDS_SVC_ID_IMMA_OM) {
		LOG_WA("IMMD can not communicate directly with IMMA OM");
	} else if (info->i_to_svc_id == NCSMDS_SVC_ID_IMMA_OI) {
		LOG_WA("IMMD can not communicate directly with IMMA OI");
	}

	if (1 /*info->o_msg_fmt_ver */ ) {	/* Does not work */

		/* as all the event structures are flat */
		evt = (IMMSV_EVT *)info->i_msg;

		rc = immsv_evt_enc_flat( /*&cb->edu_hdl, */ evt, uba);
		if (rc == NCSCC_RC_FAILURE) {
			LOG_ER("IMMD - MDS ENCODE FLAT FAILED");
		}
		return rc;
	}

	LOG_ER("INVALID MSG FORMAT VERSION IN ENC FLAT, "
	       "VER %u SVC_ID  %u", info->i_rem_svc_pvt_ver, info->i_to_svc_id);
	return NCSCC_RC_FAILURE;	/* Drop The Message */
}

/****************************************************************************
  Name          : immd_mds_dec_flat
 
  Description   : This function decodes an events sent to IMMD.
 
  Arguments     : cb    : IMMD control Block.
                  dec_info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 immd_mds_dec_flat(IMMD_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info)
{
	IMMSV_EVT *evt;
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_UBAID *uba = info->io_uba;
	NCS_BOOL is_valid_msg_fmt = FALSE;

	if (info->i_fr_svc_id == NCSMDS_SVC_ID_IMMND) {
		is_valid_msg_fmt =
		    m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
					      IMMD_WRT_IMMND_SUBPART_VER_MIN,
					      IMMD_WRT_IMMND_SUBPART_VER_MAX, immd_immnd_msg_fmt_table);
	}

	if (1 /*is_valid_msg_fmt */ ) {	/* ABT Does not work */
		evt = (IMMSV_EVT *)calloc(1, sizeof(IMMSV_EVT));
		if (evt == NULL) {
			LOG_ER("IMMD - Evt calloc Failed");
			return NCSCC_RC_FAILURE;
		}
		info->o_msg = evt;
		rc = immsv_evt_dec_flat( /*&cb->edu_hdl, */ uba, evt);
		if (rc == NCSCC_RC_FAILURE) {
			LOG_ER("IMMD - MDS DECODE FLAT FAILED");
			free(evt);
			info->o_msg = NULL;
		}

		return rc;
	}
	/* Drop The message */
	LOG_ER("INVALID MSG FORMAT VERSION IN DECODE FLAT, "
	       "VER %u SVC_ID  %u", info->i_msg_fmt_ver, info->i_fr_svc_id);
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : immd_mds_rcv
 *
 * Description   : MDS will call this function on receiving IMMD messages.
 *
 * Arguments     : cb - IMMD Control Block
 *                 rcv_info - MDS Receive information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 ****************************************************************************/
static uns32 immd_mds_rcv(IMMD_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT *pEvt = (IMMSV_EVT *)rcv_info->i_msg;

	pEvt->sinfo.ctxt = rcv_info->i_msg_ctxt;
	pEvt->sinfo.dest = rcv_info->i_fr_dest;
	pEvt->sinfo.to_svc = rcv_info->i_fr_svc_id;
	if (rcv_info->i_rsp_reqd) {
		pEvt->sinfo.stype = MDS_SENDTYPE_RSP;
	}

	/* Put it in IMMD's Event Queue */
	rc = m_NCS_IPC_SEND(&cb->mbx, (NCSCONTEXT)pEvt, NCS_IPC_PRIORITY_NORMAL);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_WA("IMMD - IPC SEND FAILED");
	}

	return rc;
}

/****************************************************************************
 * Name          : immd_mds_svc_evt
 *
 * Description   : IMMD is informed when MDS events occur that he has 
 *                 subscribed to
 *
 * Arguments     : 
 *   cb          : IMMD control Block.
 *   enc_info    : Svc evt info.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 immd_mds_svc_evt(IMMD_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{
	IMMSV_EVT *evt = NULL;
	uns32 rc;

	evt = calloc(1, sizeof(IMMSV_EVT));

	if (!evt) {
		LOG_ER("IMMD - Evt calloc Failed");
		return NCSCC_RC_OUT_OF_MEM;
	}

	/* Send the IMMD_EVT_MDS_INFO to IMMD */
	memset(evt, 0, sizeof(IMMSV_EVT));
	evt->type = IMMSV_EVT_TYPE_IMMD;
	evt->info.immd.type = IMMD_EVT_MDS_INFO;
	evt->info.immd.info.mds_info.change = svc_evt->i_change;
	evt->info.immd.info.mds_info.dest = svc_evt->i_dest;
	evt->info.immd.info.mds_info.svc_id = svc_evt->i_svc_id;
	evt->info.immd.info.mds_info.node_id = svc_evt->i_node_id;

	/* Put it in IMMD's Event Queue */
	rc = m_NCS_IPC_SEND(&cb->mbx, (NCSCONTEXT)evt, NCS_IPC_PRIORITY_HIGH);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_WA("IMMD - IPC SEND FAILED");
		free(evt);
	}

	return rc;
}

/******************************************************************************
 * Name   : immd_mds_quiesced_ack_process
 *
 * Description   : This callback is received when immd goes from active to 
 *                 quiesced, we change the mds role in csi set callback and
 *                 from mds we get the quiesced_ack callback , post an event
 *                 to your immd thread to process the events in mail box
 *
*****************************************************************************/
uns32 immd_mds_quiesced_ack_process(IMMD_CB *cb)
{
	IMMSV_EVT *evt = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (cb->is_quiesced_set) {
		cb->ha_state = SA_AMF_HA_QUIESCED;	/* Set the HA State */
		LOG_IN("HA STATE CHANGED TO QUIESCED");
		evt = calloc(1, sizeof(IMMSV_EVT));
		if (!evt) {
			LOG_ER("IMMD - Evt calloc Failed");
			return NCSCC_RC_OUT_OF_MEM;
		}

		memset(evt, 0, sizeof(IMMSV_EVT));
		evt->type = IMMSV_EVT_TYPE_IMMD;
		evt->info.immd.type = IMMD_EVT_MDS_QUIESCED_ACK_RSP;

		rc = m_NCS_IPC_SEND(&cb->mbx, evt, NCS_IPC_PRIORITY_NORMAL);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_WA("IMMD - IPC SEND FAILED");
			free(evt);	/*ABT leaks ?? */
			return rc;
		}
	} else {
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : immd_mds_send_rsp
 *
 * Description   : Send the Response to Sync Requests
 *
 * Arguments     : 
 *
 * Return Values : 
 *
 * Notes         :
 *****************************************************************************/
uns32 immd_mds_send_rsp(IMMD_CB *cb, IMMSV_SEND_INFO *s_info, IMMSV_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uns32 rc;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_handle;
	mds_info.i_svc_id = NCSMDS_SVC_ID_IMMD;
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
		LOG_WA("IMMD - IPC SEND FAILED");
	}

	return rc;
}

/****************************************************************************
  Name          : immd_mds_msg_sync_send
 
  Description   : This routine sends the Sync requests from IMMD
 
  Arguments     : cb  - ptr to the IMMD CB
                  i_evt - ptr to the IMMSV message
                  o_evt - ptr to the IMMSV message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 immd_mds_msg_sync_send(IMMD_CB *cb, uns32 to_svc, MDS_DEST to_dest,
			     IMMSV_EVT *i_evt, IMMSV_EVT **o_evt, uns32 timeout)
{

	NCSMDS_INFO mds_info;
	uns32 rc;

	if (!i_evt)
		return NCSCC_RC_FAILURE;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_handle;
	mds_info.i_svc_id = NCSMDS_SVC_ID_IMMD;
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
		LOG_WA("IMMD - IPC SEND FAILED");
	}

	return rc;
}

/****************************************************************************
  Name          : immd_mds_msg_send
 
  Description   : This routine sends the Events from IMMD
 
  Arguments     : cb  - ptr to the IMMD CB
                  i_evt - ptr to the IMMSV message
                  o_evt - ptr to the IMMSV message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 immd_mds_msg_send(IMMD_CB *cb, uns32 to_svc, MDS_DEST to_dest, IMMSV_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uns32 rc;

	if (!evt) {
		return NCSCC_RC_FAILURE;
	}

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_handle;
	mds_info.i_svc_id = NCSMDS_SVC_ID_IMMD;
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
		LOG_WA("IMMD - MDS Send Failed");
	}
	return rc;
}

/****************************************************************************
 * Name          : immd_mds_bcast_send
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
uns32 immd_mds_bcast_send(IMMD_CB *cb, IMMSV_EVT *evt, NCSMDS_SVC_ID to_svc)
{
	NCSMDS_INFO info;
	uns32 res;

	TRACE_ENTER();
	memset(&info, 0, sizeof(info));

	info.i_mds_hdl = cb->mds_handle;
	info.i_op = MDS_SEND;
	info.i_svc_id = NCSMDS_SVC_ID_IMMD;

	info.info.svc_send.i_msg = (NCSCONTEXT)evt;
	info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	info.info.svc_send.i_sendtype = MDS_SENDTYPE_BCAST;
	info.info.svc_send.i_to_svc = to_svc;
	info.info.svc_send.info.bcast.i_bcast_scope = NCSMDS_SCOPE_NONE;

	immsv_msg_trace_send(0, evt);

	res = ncsmds_api(&info);
	if (res != NCSCC_RC_SUCCESS) {
		LOG_WA("IMMD - MDS Send Failed");
	}

	TRACE_LEAVE();
	return (res);
}
