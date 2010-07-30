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

#include "gld.h"

/*****************************************************************************
  FILE NAME: GLD_MDS.C

  DESCRIPTION: File contains mds related operations

  FUNCTIONS INCLUDED in this module:
      gld_mds_callback 
      gld_mds_svc_evt
      gld_mds_rcv
      gld_mds_enc
      gld_mds_dec
      gld_mds_cpy
      gld_mds_vdest_create
      gld_mds_vdest_destroy
      gld_mds_init
      gld_mds_shut
******************************************************************************/

static uns32 gld_mds_enc_flat(GLSV_GLD_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uns32 gld_mds_dec_flat(GLSV_GLD_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uns32 gld_mds_dec(GLSV_GLD_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info);
static uns32 gld_mds_enc(GLSV_GLD_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info);

static const MSG_FRMT_VER gld_glnd_msg_fmt_table[GLD_WRT_GLND_SUBPART_VER_RANGE] = { 1 };

/****************************************************************************
 * Name          : gld_mds_callback
 *
 * Description   : Call back function provided to MDS. MDS will call this
 *                 function for enc/dec/cpy/rcv/svc_evt operations.
 *
 * Arguments     : NCSMDS_CALLBACK_INFO *info: call back info.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_mds_callback(NCSMDS_CALLBACK_INFO *info)
{
	uns32 cb_hdl;
	GLSV_GLD_CB *cb = NULL;
	uns32 rc = NCSCC_RC_FAILURE;

	if (info == NULL)
		return rc;

	cb_hdl = (uns32)info->i_yr_svc_hdl;

	if ((cb = (GLSV_GLD_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLD, cb_hdl)) == NULL) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_SUCCESS;
	}

	switch (info->i_op) {
	case MDS_CALLBACK_ENC:
		rc = gld_mds_enc(cb, &info->info.enc);
		break;

	case MDS_CALLBACK_DEC:
		rc = gld_mds_dec(cb, &info->info.dec);
		break;
	case MDS_CALLBACK_ENC_FLAT:
		rc = gld_mds_enc_flat(cb, &info->info.enc_flat);
		break;
	case MDS_CALLBACK_DEC_FLAT:
		rc = gld_mds_dec_flat(cb, &info->info.dec_flat);
		break;
	case MDS_CALLBACK_RECEIVE:
		rc = gld_mds_rcv(cb, &info->info.receive);
		break;

	case MDS_CALLBACK_COPY:
		rc = gld_mds_cpy(cb, &info->info.cpy);
		break;

	case MDS_CALLBACK_SVC_EVENT:
		rc = gld_mds_svc_evt(cb, &info->info.svc_evt);
		break;

	case MDS_CALLBACK_QUIESCED_ACK:
		rc = gld_mds_quiesced_process(cb, &info->info.svc_evt);
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_SVC_PRVDR(GLD_MDS_CALL_BACK_ERROR, NCSFL_SEV_ERROR, __FILE__, __LINE__);
	}

	ncshm_give_hdl((uns32)cb_hdl);

	return rc;

}

/****************************************************************************
 * Name          : gld_mds_quiesced_process 
 *
 * Description   : MDS will call this function on when mds events occur.
 *
 * ARGUMENTS:
 *   cb          : GLD control Block.
 *   enc_info    : Received information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_mds_quiesced_process(GLSV_GLD_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *rcv_info)
{
	GLSV_GLD_EVT *evt;
	evt = m_MMGR_ALLOC_GLSV_GLD_EVT;
	if (evt == GLSV_GLD_EVT_NULL) {
		m_LOG_GLD_MEMFAIL(GLD_EVT_ALLOC_FAILED, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	memset(evt, 0, sizeof(GLSV_GLD_EVT));
	evt->gld_cb = cb;
	evt->evt_type = GLSV_GLD_EVT_QUISCED_STATE;

	/* Push the event and we are done */
	if (m_NCS_IPC_SEND(&cb->mbx, evt, NCS_IPC_PRIORITY_NORMAL) == NCSCC_RC_FAILURE) {
		m_LOG_GLD_HEADLINE(GLD_IPC_SEND_FAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		gld_evt_destroy(evt);
		return (NCSCC_RC_FAILURE);
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : gld_mds_svc_evt
 *
 * Description   : MDS will call this function on when mds events occur.
 *
 * ARGUMENTS:
 *   cb          : GLD control Block.
 *   enc_info    : Received information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_mds_svc_evt(GLSV_GLD_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *rcv_info)
{
	/* First make sure that is event is indeed for us */
	if (rcv_info->i_your_id != NCSMDS_SVC_ID_GLD) {
		/*GLSV_ADD_LOF_HERE */
		return NCSCC_RC_SUCCESS;
	}

	switch (rcv_info->i_svc_id) {
	case NCSMDS_SVC_ID_GLND:
		{
			if (rcv_info->i_change == NCSMDS_UP)
				break;
			else if ((rcv_info->i_change == NCSMDS_DOWN)) {
				/* As of now we are only interested in GLND events */
				GLSV_GLD_EVT *evt;
				evt = m_MMGR_ALLOC_GLSV_GLD_EVT;
				if (evt == GLSV_GLD_EVT_NULL) {
					m_LOG_GLD_MEMFAIL(GLD_EVT_ALLOC_FAILED, __FILE__, __LINE__);
					return NCSCC_RC_FAILURE;
				}
				memset(evt, 0, sizeof(GLSV_GLD_EVT));
				evt->gld_cb = cb;
				evt->evt_type = GLSV_GLD_EVT_GLND_DOWN;

				evt->info.glnd_mds_info.mds_dest_id = rcv_info->i_dest;

				/* Push the event and we are done */
				if (m_NCS_IPC_SEND(&cb->mbx, evt, NCS_IPC_PRIORITY_NORMAL) == NCSCC_RC_FAILURE) {
					m_LOG_GLD_HEADLINE(GLD_IPC_SEND_FAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
					gld_evt_destroy(evt);
					return (NCSCC_RC_FAILURE);
				}
			}
		}
		break;

	default:
		m_LOG_GLD_SVC_PRVDR(GLD_MDS_UNKNOWN_SVC_EVT, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		break;

	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : gld_mds_rcv
 *
 * Description   : MDS will call this function on receiving GLD messages.
 *
 * ARGUMENTS:
 *   cb          : GLD control Block.
 *   enc_info    : Received information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

uns32 gld_mds_rcv(GLSV_GLD_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	GLSV_GLD_EVT *evt = (GLSV_GLD_EVT *)rcv_info->i_msg;

	/* Fill GLD CB handle in the received message */
	evt->gld_cb = cb;
	evt->fr_dest_id = rcv_info->i_fr_dest;

	if (m_NCS_IPC_SEND(&cb->mbx, evt, NCS_IPC_PRIORITY_NORMAL) == NCSCC_RC_FAILURE) {
		m_LOG_GLD_HEADLINE(GLD_IPC_SEND_FAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		gld_evt_destroy(evt);
		return (NCSCC_RC_FAILURE);
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME: gld_mds_enc
  DESCRIPTION   : 
  This function encodes an events sent from GLD.
  ARGUMENTS:
      cb        : GLD control Block.
      enc_info  : Info for encoding

  RETURNS       : NCSCC_RC_SUCCESS/Error Code.
      
  NOTES:
*****************************************************************************/
static uns32 gld_mds_enc(GLSV_GLD_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info)
{
	EDU_ERR ederror = 0;
	uns32 rc = NCSCC_RC_FAILURE;

	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_GLND) {
		enc_info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
								GLD_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
								GLD_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
								gld_glnd_msg_fmt_table);
	}

	if (enc_info->o_msg_fmt_ver) {
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, glsv_edp_glnd_evt, enc_info->io_uba,
				    EDP_OP_TYPE_ENC, (GLSV_GLND_EVT *)enc_info->i_msg, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			/* Free calls to be added here. */
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			return rc;
		}
		return rc;
	} else {
		/* Drop The Message */
		m_LOG_GLD_HEADLINE(GLD_MSG_FRMT_VER_INVALID, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}
}

/*****************************************************************************

  PROCEDURE NAME:   gld_mds_dec

  DESCRIPTION   :
  This function decodes a buffer containing an GLD events that was received 
  from off card.

  ARGUMENTS:        
      cb        : GLD control Block.
      dec_info  : Info for decoding

  RETURNS       : NCSCC_RC_SUCCESS/Error Code.

  NOTES:

*****************************************************************************/
static uns32 gld_mds_dec(GLSV_GLD_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info)
{
	EDU_ERR ederror = 0;
	GLSV_GLD_EVT *evt;
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_BOOL is_valid_msg_fmt = FALSE;

	if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_GLND) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
							     GLD_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							     GLD_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							     gld_glnd_msg_fmt_table);
	}

	if (is_valid_msg_fmt) {
		evt = m_MMGR_ALLOC_GLSV_GLD_EVT;
		dec_info->o_msg = (NCSCONTEXT)evt;

		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, glsv_edp_gld_evt, dec_info->io_uba,
				    EDP_OP_TYPE_DEC, (GLSV_GLD_EVT **)&dec_info->o_msg, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_GLSV_GLD_EVT(evt);
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			return rc;
		}
		return rc;
	} else {
		/* Drop The Message */
		m_LOG_GLD_HEADLINE(GLD_MSG_FRMT_VER_INVALID, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
  Name          : gld_mds_enc_flat
 
  Description   : This function encodes an events sent from GLD.
 
  Arguments     : cb    : GLD_CB control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 gld_mds_enc_flat(GLSV_GLD_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info)
{
	NCS_UBAID *uba = info->io_uba;

	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	if (info->i_to_svc_id == NCSMDS_SVC_ID_GLND) {
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    GLD_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							    GLD_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							    gld_glnd_msg_fmt_table);
	}

	if (info->o_msg_fmt_ver) {
		if (info->i_to_svc_id == NCSMDS_SVC_ID_GLND) {
			GLSV_GLND_EVT *evt;
			evt = (GLSV_GLND_EVT *)info->i_msg;
			ncs_encode_n_octets_in_uba(uba, (uns8 *)evt, sizeof(GLSV_GLND_EVT));
			if (evt->type == GLSV_GLND_EVT_RSC_MASTER_INFO) {
				if (evt->info.rsc_master_info.no_of_res > 0) {
					GLSV_GLND_RSC_MASTER_INFO_LIST *rsc_master_list =
					    evt->info.rsc_master_info.rsc_master_list;
					ncs_encode_n_octets_in_uba(uba, (uns8 *)rsc_master_list,
								   (sizeof(GLSV_GLND_RSC_MASTER_INFO_LIST) *
								    evt->info.rsc_master_info.no_of_res));
				}
			}
			return NCSCC_RC_SUCCESS;
		}
	} else {
		/* Drop The Message */
		m_LOG_GLD_HEADLINE(GLD_MSG_FRMT_VER_INVALID, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : gld_mds_dec_flat
 
  Description   : This function decodes an events sent from GLND.
 
  Arguments     : cb    : GLND_CB control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 gld_mds_dec_flat(GLSV_GLD_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info)
{
	GLSV_GLD_EVT *evt;
	NCS_UBAID *uba = info->io_uba;
	NCS_BOOL is_valid_msg_fmt = FALSE;

	if (info->i_fr_svc_id == NCSMDS_SVC_ID_GLND) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     GLD_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							     GLD_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							     gld_glnd_msg_fmt_table);
	}

	if (is_valid_msg_fmt) {
		if (info->i_fr_svc_id == NCSMDS_SVC_ID_GLND) {
			evt = (GLSV_GLD_EVT *)m_MMGR_ALLOC_GLSV_GLD_EVT;
			if (evt == NULL) {
				m_LOG_GLD_MEMFAIL(GLD_EVT_ALLOC_FAILED, __FILE__, __LINE__);
				return NCSCC_RC_FAILURE;
			}
			info->o_msg = evt;
			ncs_decode_n_octets(uba->ub, (uns8 *)evt, sizeof(GLSV_GLD_EVT));
			return NCSCC_RC_SUCCESS;
		} else
			return NCSCC_RC_FAILURE;
	} else {
		/* Drop The Message */
		m_LOG_GLD_HEADLINE(GLD_MSG_FRMT_VER_INVALID, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}
}

/*****************************************************************************

  PROCEDURE NAME:   gld_mds_cpy

  DESCRIPTION   :
  This function enable a copy of the message sent between entities belonging to
   the same thread.

  ARGUMENTS:        
      cb        : GLD control Block.
      cpy_info  : Info for decoding

  RETURNS       : NCSCC_RC_SUCCESS/Error Code.

  NOTES:

*****************************************************************************/
uns32 gld_mds_cpy(GLSV_GLD_CB *cb, MDS_CALLBACK_COPY_INFO *cpy_info)
{
	GLSV_GLND_EVT *src = (GLSV_GLND_EVT *)cpy_info->i_msg;
	GLSV_GLND_EVT *cpy = m_MMGR_ALLOC_GLND_EVT;

	if (cpy == NULL) {
		m_LOG_GLD_MEMFAIL(GLD_EVT_ALLOC_FAILED, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	memset(cpy, 0, sizeof(GLSV_GLND_EVT));

	memcpy(cpy, src, sizeof(GLSV_GLND_EVT));
	switch (src->type) {
		/* Nothing in here as of now... in future if there are any linked lists
		 * in the 'src', just make sure that they are nulled out in src as they
		 * are now owned by cpy */
	default:
		break;
	}

	/* Set the copy of the message */
	cpy_info->o_cpy = (NCSCONTEXT)cpy;
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : gld_mds_vdest_create
 *
 * Description   : This function Creates the Virtual destination for GLD
 *
 * Arguments     : cb   : GLD control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_mds_vdest_create(GLSV_GLD_CB *cb)
{
	NCSVDA_INFO arg;
	uns32 rc;
	SaClmClusterNodeT cluster_node;
	uns32 seed;

	memset(&cluster_node, 0, sizeof(SaClmClusterNodeT));

	memset(&arg, 0, sizeof(NCSVDA_INFO));
	arg.req = NCSVDA_VDEST_CREATE;
	arg.info.vdest_create.i_persistent = FALSE;
	arg.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;
	arg.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
	arg.info.vdest_create.info.specified.i_vdest = cb->my_dest_id;

	seed = random();
	cb->my_anc = seed & 0xff;

	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		cb->my_anc = V_DEST_QA_1;
	}
	if (cb->ha_state == SA_AMF_HA_STANDBY) {
		cb->my_anc = V_DEST_QA_2;
	}

	rc = ncsvda_api(&arg);
	if (rc != NCSCC_RC_SUCCESS) {
		/* RSR;TBD Log, */
		return rc;
	}
	cb->mds_handle = arg.info.vdest_create.o_mds_pwe1_hdl;

	return rc;
}

/****************************************************************************
 * Name          : gld_mds_vdest_destroy
 *
 * Description   : This function Destroys the Virtual destination of GLD
 *
 * Arguments     : cb   : GLD control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_mds_vdest_destroy(GLSV_GLD_CB *cb)
{
	NCSVDA_INFO arg;
	uns32 rc;
	SaNameT name = { 4, "GLD" };

	memset(&arg, 0, sizeof(NCSVDA_INFO));

	arg.req = NCSVDA_VDEST_DESTROY;
	arg.info.vdest_destroy.i_vdest = cb->my_dest_id;
	arg.info.vdest_destroy.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
	arg.info.vdest_destroy.i_name = name;

	rc = ncsvda_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_SVC_PRVDR(GLD_MDS_VDEST_DESTROY_FAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}

	return rc;
}

/****************************************************************************
 * Name          : gld_mds_init
 *
 * Description   : This function Calls the mds install and subscribe operations
 *
 * Arguments     : cb   : GLD control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

uns32 gld_mds_init(GLSV_GLD_CB *cb)
{
	NCSMDS_INFO arg;
	uns32 rc;
	MDS_SVC_ID subscr_svc = NCSMDS_SVC_ID_GLND;

	memset(&cb->my_dest_id, 0, sizeof(MDS_DEST));
	cb->my_dest_id = GLD_VDEST_ID;
	/* In future Anchor value will be taken from AVSV. Now we are using fix value */
	cb->my_anc = V_DEST_QA_1;

	/* Create the vertual Destination for GLD */
	rc = gld_mds_vdest_create(cb);
	if (rc != NCSCC_RC_SUCCESS)
		return rc;
	/* Set the role to active  - Needs to be removed after integration with AvSv */

	/* Install your service into MDS */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->mds_handle;
	arg.i_svc_id = NCSMDS_SVC_ID_GLD;
	arg.i_op = MDS_INSTALL;

	arg.info.svc_install.i_yr_svc_hdl = cb->my_hdl;
	arg.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	arg.info.svc_install.i_svc_cb = gld_mds_callback;
	arg.info.svc_install.i_mds_q_ownership = FALSE;
	arg.info.svc_install.i_mds_svc_pvt_ver = GLD_PVT_SUBPART_VERSION;

	rc = ncsmds_api(&arg);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_SVC_PRVDR(GLD_MDS_INSTALL_FAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}

	/* Now subscribe for GLND events in MDS */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->mds_handle;
	arg.i_svc_id = NCSMDS_SVC_ID_GLD;
	arg.i_op = MDS_SUBSCRIBE;

	arg.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	arg.info.svc_subscribe.i_num_svcs = 1;
	arg.info.svc_subscribe.i_svc_ids = &subscr_svc;

	rc = ncsmds_api(&arg);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_SVC_PRVDR(GLD_MDS_SUBSCRIBE_FAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}
	return rc;
}

/****************************************************************************
 * Name          : gld_mds_shut
 *
 * Description   : This function un-registers the GLD Service with MDS.
 *
 * Arguments     : cb   : GLD control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_mds_shut(GLSV_GLD_CB *cb)
{
	NCSMDS_INFO arg;
	uns32 rc;

	/* Un-install your service into MDS */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->mds_handle;
	arg.i_svc_id = NCSMDS_SVC_ID_GLD;
	arg.i_op = MDS_UNINSTALL;

	rc = ncsmds_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_SVC_PRVDR(GLD_MDS_UNINSTALL_FAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}

	/* Destroy the virtual Destination of GLD */
	rc = gld_mds_vdest_destroy(cb);
	return rc;
}

/****************************************************************************
  Name          : gld_mds_change_role
 
  Description   : This routine use for setting and changing role. 
 
  Arguments     : role - Role to be set.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 gld_mds_change_role(GLSV_GLD_CB *cb, V_DEST_RL role)
{
	NCSVDA_INFO arg;

	memset(&arg, 0, sizeof(NCSVDA_INFO));

	arg.req = NCSVDA_VDEST_CHG_ROLE;
	arg.info.vdest_chg_role.i_vdest = cb->my_dest_id;
	arg.info.vdest_chg_role.i_new_role = role;
	if (ncsvda_api(&arg) != NCSCC_RC_SUCCESS) {
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : gld_process_node_down_evts

  DESCRIPTION    :

  ARGUMENTS      :gld_cb      - ptr to the GLD control block
                  evt          - ptr to the event.

  RETURNS        :NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS

  NOTES         : None
*****************************************************************************/
uns32 gld_process_node_down_evts(GLSV_GLD_CB *gld_cb)
{
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc = NULL;
	SaLckResourceIdT rsc_id;
	uns32 node_id = 0;

	/* cleanup the  glnd details tree */
	node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_getnext(&gld_cb->glnd_details, (uns8 *)0);
	while (node_details) {
		node_id = node_details->node_id;
		if (node_details->status == GLND_DOWN_STATE) {
			/* Remove the reference to each of the resource referred by this node */
			glnd_rsc =
			    (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uns8 *)0);
			if (glnd_rsc) {
				rsc_id = glnd_rsc->rsc_id;
				while (glnd_rsc) {
					gld_rsc_rmv_node_ref(gld_cb, glnd_rsc->rsc_info, glnd_rsc, node_details,
							     glnd_rsc->rsc_info->can_orphan);
					glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)
					    ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uns8 *)&rsc_id);
					if (glnd_rsc)
						rsc_id = glnd_rsc->rsc_id;
				}
			}
			/* Now delete this node details node */
			if (ncs_patricia_tree_del(&gld_cb->glnd_details, (NCS_PATRICIA_NODE *)node_details) !=
			    NCSCC_RC_SUCCESS) {
				m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_DEL_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__,
						   node_details->node_id);
			} else {
				m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);
				m_LOG_GLD_HEADLINE(GLD_ACTIVE_RMV_NODE, NCSFL_SEV_NOTICE, __FILE__, __LINE__, node_id);
			}
		}
		node_details =
		    (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_getnext(&gld_cb->glnd_details, (uns8 *)&node_id);
	}
	return NCSCC_RC_SUCCESS;
}
