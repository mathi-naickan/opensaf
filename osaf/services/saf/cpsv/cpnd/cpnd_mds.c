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

  DESCRIPTION:

  This file contains routines used by CPND library for MDS interaction.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

*****************************************************************************/

#include "cpnd.h"

uint32_t cpnd_mds_callback(struct ncsmds_callback_info *info);
static uint32_t cpnd_mds_enc(CPND_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uint32_t cpnd_mds_dec(CPND_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uint32_t cpnd_mds_rcv(CPND_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info);
static uint32_t cpnd_mds_svc_evt(CPND_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt);
static uint32_t cpnd_mds_enc_flat(CPND_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uint32_t cpnd_mds_dec_flat(CPND_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uint32_t cpnd_mds_send_try_again_rsp(CPND_CB *cb, CPSV_EVT *pEvt);
static uint32_t cpsv_ckpt_access_decode(CPSV_CKPT_ACCESS *ckpt_data, NCS_UBAID *io_uba);

FUNC_DECLARATION(CPSV_EVT);

/* Message Format Verion Tables at CPND */
MDS_CLIENT_MSG_FORMAT_VER cpnd_cpa_msg_fmt_table[CPND_WRT_CPA_SUBPART_VER_RANGE] = {
	1, 2
};

MDS_CLIENT_MSG_FORMAT_VER cpnd_cpnd_msg_fmt_table[CPND_WRT_CPND_SUBPART_VER_RANGE] = {
	1, 2
};

MDS_CLIENT_MSG_FORMAT_VER cpnd_cpd_msg_fmt_table[CPND_WRT_CPD_SUBPART_VER_RANGE] = {
	1, 2, 3
};

/****************************************************************************
 * Name          : cpnd_mds_get_handle
 *
 * Description   : This function Gets the Handles of local MDS destination
 *
 * Arguments     : cb   : CPND control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t cpnd_mds_get_handle(CPND_CB *cb)
{
	NCSADA_INFO arg;
	uint32_t rc;

	memset(&arg, 0, sizeof(NCSADA_INFO));
	arg.req = NCSADA_GET_HDLS;
	rc = ncsada_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_4("cpnd mds get hdl failed");
		return rc;
	}
	cb->cpnd_mds_hdl = arg.info.adest_get_hdls.o_mds_pwe1_hdl;
	return rc;
}

/****************************************************************************
  Name          : cpnd_mds_register
 
  Description   : This routine registers the CPND Service with MDS.
 
  Arguments     : cpnd_cb - ptr to the CPND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uint32_t cpnd_mds_register(CPND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCSMDS_INFO svc_info;
	MDS_SVC_ID svc_id[1] = { NCSMDS_SVC_ID_CPD };

	TRACE_ENTER();
	/* STEP1: Get the MDS Handle */
	rc = cpnd_mds_get_handle(cb);

	if (rc != NCSCC_RC_SUCCESS)
		return rc;

	/* memset the svc_info */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));

	/* STEP 2 : Install on ADEST with MDS with service ID NCSMDS_SVC_ID_CPA. */
	svc_info.i_mds_hdl = cb->cpnd_mds_hdl;
	svc_info.i_svc_id = NCSMDS_SVC_ID_CPND;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = cb->cpnd_cb_hdl_id;
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/* node specific */
	svc_info.info.svc_install.i_svc_cb = cpnd_mds_callback;	/* callback */
	svc_info.info.svc_install.i_mds_q_ownership = false;	/* CPND owns the mds queue */
	svc_info.info.svc_install.i_mds_svc_pvt_ver = CPND_MDS_PVT_SUBPART_VERSION;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("cpnd mds install failed ");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	cb->cpnd_mdest_id = svc_info.info.svc_install.o_dest;

	/* STEP 3: Subscribe to CPD up/down events */
	svc_info.i_op = MDS_RED_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("cpnd mds subscribe cpd failed");
		goto error1;
	}

	/* STEP 4: Subscribe to CPA up/down events */
	svc_id[0] = NCSMDS_SVC_ID_CPA;
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("cpnd mds subscribe cpa failed");
		goto error1;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
 error1:

	/* Uninstall with the mds */
	cpnd_mds_unregister(cb);
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : cpnd_mds_unregister
 *
 * Description   : This function un-registers the CPND Service with MDS.
 *
 * Arguments     : cb   : CPND control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void cpnd_mds_unregister(CPND_CB *cb)
{
	NCSMDS_INFO arg;

	TRACE_ENTER();
	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->cpnd_mds_hdl;
	arg.i_svc_id = NCSMDS_SVC_ID_CPND;
	arg.i_op = MDS_UNINSTALL;

	if (ncsmds_api(&arg) != NCSCC_RC_SUCCESS) {
		TRACE_4("cpd mds unreg failed ");
	}
	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : cpnd_mds_callback
 
  Description   : This callback routine will be called by MDS on event arrival
 
  Arguments     : info - pointer to the mds callback info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t cpnd_mds_callback(struct ncsmds_callback_info *info)
{
	CPND_CB *cb = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER();
	if (info == NULL) {
		TRACE_4("cpnd mds callback called with NULL as parameter");
		return rc;
	}
	cb = (CPND_CB *)ncshm_take_hdl(NCS_SERVICE_ID_CPND, (uint32_t)info->i_yr_svc_hdl);
	if (!cb) {
		LOG_ER("cpnd cb hdl take failed");
		TRACE_LEAVE();
		return rc;
	}

	switch (info->i_op) {
	case MDS_CALLBACK_COPY:
		rc = NCSCC_RC_FAILURE;
		break;
	case MDS_CALLBACK_ENC:
		rc = cpnd_mds_enc(cb, &info->info.enc);
		break;
	case MDS_CALLBACK_DEC:
		rc = cpnd_mds_dec(cb, &info->info.dec);
		break;
	case MDS_CALLBACK_ENC_FLAT:
		rc = cpnd_mds_enc_flat(cb, &info->info.enc_flat);
		break;
	case MDS_CALLBACK_DEC_FLAT:
		rc = cpnd_mds_dec_flat(cb, &info->info.dec_flat);
		break;
	case MDS_CALLBACK_RECEIVE:
		rc = cpnd_mds_rcv(cb, &info->info.receive);
		break;

	case MDS_CALLBACK_SVC_EVENT:
		rc = cpnd_mds_svc_evt(cb, &info->info.svc_evt);
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}

	ncshm_give_hdl((uint32_t)info->i_yr_svc_hdl);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : cpnd_mds_enc
 
  Description   : This function encodes an events sent from CPND.
 
  Arguments     : cb    : CPND control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t cpnd_mds_enc(CPND_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info)
{
	CPSV_EVT *pevt = NULL;
	EDU_ERR ederror = 0;
	NCS_UBAID *io_uba = enc_info->io_uba;
	uint8_t *pstream = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_CPA) {
		enc_info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
								CPND_WRT_CPA_SUBPART_VER_MIN,
								CPND_WRT_CPA_SUBPART_VER_MAX, cpnd_cpa_msg_fmt_table);

	} else if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_CPND) {
		enc_info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
								CPND_WRT_CPND_SUBPART_VER_MIN,
								CPND_WRT_CPND_SUBPART_VER_MAX, cpnd_cpnd_msg_fmt_table);
	} else if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_CPD) {
		enc_info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
								CPND_WRT_CPD_SUBPART_VER_MIN,
								CPND_WRT_CPD_SUBPART_VER_MAX, cpnd_cpd_msg_fmt_table);
	}

	if (enc_info->o_msg_fmt_ver) {
		pevt = (CPSV_EVT *)enc_info->i_msg;
		if (pevt->type == CPSV_EVT_TYPE_CPA) {
			if (pevt->info.cpa.type == CPA_EVT_ND2A_CKPT_DATA_RSP) {
				/* Encode the CPSV_ND2A_DATA_ACCESS_RSP */
				pstream = ncs_enc_reserve_space(io_uba, 8);
				if (!pstream)
					return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
							       "Memory alloc failed in cpnd_mds_enc \n");

				ncs_encode_32bit(&pstream, pevt->type);	/* Type */
				ncs_encode_32bit(&pstream, pevt->info.cpa.type);
				ncs_enc_claim_space(io_uba, 8);

				rc = cpsv_data_access_rsp_encode(&pevt->info.cpa.info.sec_data_rsp, io_uba);
				return rc;
			}

		} else if (pevt->type == CPSV_EVT_TYPE_CPND) {
			switch (pevt->info.cpnd.type) {
			case CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC:

				pstream = ncs_enc_reserve_space(io_uba, 12);
				if (!pstream)
					return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
							       "Memory alloc failed in cpnd_mds_enc \n");
				ncs_encode_32bit(&pstream, pevt->type);	/* CPSV_EVT Type */
				ncs_encode_32bit(&pstream, pevt->info.cpnd.error);	/* cpnd_evt error This is for backword compatible purpose with EDU enc/dec with 3.0.2 */
				ncs_encode_32bit(&pstream, pevt->info.cpnd.type);	/* cpnd_evt SubType */
				ncs_enc_claim_space(io_uba, 12);

				rc = cpsv_ckpt_access_encode(&pevt->info.cpnd.info.ckpt_nd2nd_sync, io_uba);
				return rc;

			case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ:

				pstream = ncs_enc_reserve_space(io_uba, 12);
				if (!pstream)
					return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
							       "Memory alloc failed in cpnd_mds_enc \n");
				ncs_encode_32bit(&pstream, pevt->type);	/* CPSV_EVT Type */
				ncs_encode_32bit(&pstream, pevt->info.cpnd.error);	/* cpnd_evt error This is for backword compatible purpose with EDU enc/dec with 3.0.2 */
				ncs_encode_32bit(&pstream, pevt->info.cpnd.type);	/* cpnd_evt SubType */
				ncs_enc_claim_space(io_uba, 12);

				rc = cpsv_ckpt_access_encode(&pevt->info.cpnd.info.ckpt_nd2nd_data, io_uba);
				return rc;

			case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP:

				pstream = ncs_enc_reserve_space(io_uba, 12);
				if (!pstream)
					return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
							       "Memory alloc failed in cpnd_mds_enc \n");
				ncs_encode_32bit(&pstream, pevt->type);	/* CPSV_EVT Type */
				ncs_encode_32bit(&pstream, pevt->info.cpnd.error);	/* cpnd_evt error This is for backword compatible purpose with EDU enc/dec with 3.0.2 */
				ncs_encode_32bit(&pstream, pevt->info.cpnd.type);	/* cpnd_evt SubType */
				ncs_enc_claim_space(io_uba, 12);

				rc = cpsv_data_access_rsp_encode(&pevt->info.cpnd.info.ckpt_nd2nd_data_rsp, io_uba);
				return rc;
			default:
				break;
			}
		}
		/* For all other Cases Invoke EDU encode */
		return (m_NCS_EDU_EXEC(&cb->cpnd_edu_hdl, FUNC_NAME(CPSV_EVT),
				       enc_info->io_uba, EDP_OP_TYPE_ENC, pevt, &ederror));
	} else {
		TRACE_LEAVE();
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "INVALID MSG FORMAT IN ENCODE FULL\n");	/* Drop The Message - Incompatible Message Format Version */
		
	}
}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_access_decode 

 DESCRIPTION    : This routine will decode the contents of CPSV_EVT into user buf

 ARGUMENTS      :  CPSV_CKPT_DATA *ckpt_data.
                  *io_ub  - User Buff.

 RETURNS        : uint32_t 

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_ckpt_access_decode(CPSV_CKPT_ACCESS *ckpt_data, NCS_UBAID *io_uba)
{
	uint8_t *pstream = NULL;
	uint8_t local_data[150];
	uint32_t space = 4 + 8 + 8 + 8 + 4 + 4;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	pstream = ncs_dec_flatten_space(io_uba, local_data, space);
	ckpt_data->type = ncs_decode_32bit(&pstream);
	ckpt_data->ckpt_id = ncs_decode_64bit(&pstream);
	ckpt_data->lcl_ckpt_id = ncs_decode_64bit(&pstream);
	ckpt_data->agent_mdest = ncs_decode_64bit(&pstream);
	ckpt_data->num_of_elmts = ncs_decode_32bit(&pstream);
	ckpt_data->all_repl_evt_flag = (bool)ncs_decode_32bit(&pstream);
	ncs_dec_skip_space(io_uba, space);

	/* Decode The Linked List */
	rc = cpsv_ckpt_data_decode(&ckpt_data->data, io_uba);

	/* Following are Not Required But for Write/Read (Used in checkpoint sync evt) compatiblity with 3.0.2 */
	space = 4 + 1 + 8 + 8 + 8 + 8 + 4 + 8 + 4 + 4 + 1 /*+ MDS_SYNC_SND_CTXT_LEN_MAX */ ;

	pstream = ncs_dec_flatten_space(io_uba, local_data, space);
	ckpt_data->seqno = ncs_decode_32bit(&pstream);
	ckpt_data->last_seq = ncs_decode_8bit(&pstream);
	ckpt_data->ckpt_sync.ckpt_id = ncs_decode_64bit(&pstream);
	ckpt_data->ckpt_sync.invocation = ncs_decode_64bit(&pstream);
	ckpt_data->ckpt_sync.lcl_ckpt_hdl = ncs_decode_64bit(&pstream);
	ckpt_data->ckpt_sync.client_hdl = ncs_decode_64bit(&pstream);
	ckpt_data->ckpt_sync.is_ckpt_open = (bool)ncs_decode_32bit(&pstream);
	ckpt_data->ckpt_sync.cpa_sinfo.dest = ncs_decode_64bit(&pstream);
	ckpt_data->ckpt_sync.cpa_sinfo.stype = ncs_decode_32bit(&pstream);
	ckpt_data->ckpt_sync.cpa_sinfo.to_svc = ncs_decode_32bit(&pstream);
	ckpt_data->ckpt_sync.cpa_sinfo.ctxt.length = ncs_decode_8bit(&pstream);
	ncs_dec_skip_space(io_uba, space);
	ncs_decode_n_octets_from_uba(io_uba, (uint8_t *)ckpt_data->ckpt_sync.cpa_sinfo.ctxt.data,
				     (uint32_t)MDS_SYNC_SND_CTXT_LEN_MAX);
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : cpnd_mds_dec
 
  Description   : This function decodes an events sent to CPND.
 
  Arguments     : cb    : CPND control Block.
                  info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t cpnd_mds_dec(CPND_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info)
{

	CPSV_EVT *msg_ptr = NULL;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint8_t *pstream;
	uint8_t local_data[20];
	bool is_valid_msg_fmt = false;

	if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_CPA) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
							     CPND_WRT_CPA_SUBPART_VER_MIN,
							     CPND_WRT_CPA_SUBPART_VER_MAX, cpnd_cpa_msg_fmt_table);
	} else if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_CPND) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
							     CPND_WRT_CPND_SUBPART_VER_MIN,
							     CPND_WRT_CPND_SUBPART_VER_MAX, cpnd_cpnd_msg_fmt_table);
	} else if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_CPD) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
							     CPND_WRT_CPD_SUBPART_VER_MIN,
							     CPND_WRT_CPD_SUBPART_VER_MAX, cpnd_cpd_msg_fmt_table);
	}

	if (is_valid_msg_fmt) {
		msg_ptr = m_MMGR_ALLOC_CPSV_EVT(NCS_SERVICE_ID_CPND);
		if (!msg_ptr)
			return NCSCC_RC_FAILURE;

		memset(msg_ptr, 0, sizeof(CPSV_EVT));
		dec_info->o_msg = (NCSCONTEXT)msg_ptr;
		pstream = ncs_dec_flatten_space(dec_info->io_uba, local_data, 12);
		msg_ptr->type = ncs_decode_32bit(&pstream);
		if (msg_ptr->type == CPSV_EVT_TYPE_CPND) {	/* Replaced EDU with encode/decode Routines for Performance Improvement */
			msg_ptr->info.cpnd.error = ncs_decode_32bit(&pstream);
			msg_ptr->info.cpnd.type = ncs_decode_32bit(&pstream);

			switch (msg_ptr->info.cpnd.type) {
			case CPND_EVT_A2ND_CKPT_WRITE:	/* Write EVENT */
				ncs_dec_skip_space(dec_info->io_uba, 12);
				rc = cpsv_ckpt_access_decode(&msg_ptr->info.cpnd.info.ckpt_write, dec_info->io_uba);
				goto free;

			case CPND_EVT_A2ND_CKPT_READ:	/* Read Event */
				ncs_dec_skip_space(dec_info->io_uba, 12);
				rc = cpsv_ckpt_access_decode(&msg_ptr->info.cpnd.info.ckpt_read, dec_info->io_uba);
				goto free;

			case CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC:	/* Sync Event */
				ncs_dec_skip_space(dec_info->io_uba, 12);
				rc = cpsv_ckpt_access_decode(&msg_ptr->info.cpnd.info.ckpt_nd2nd_sync,
							     dec_info->io_uba);
				goto free;

			case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ:	/* Write Event ND2ND */
				ncs_dec_skip_space(dec_info->io_uba, 12);
				rc = cpsv_ckpt_access_decode(&msg_ptr->info.cpnd.info.ckpt_nd2nd_data,
							     dec_info->io_uba);
				goto free;

			case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP:
				ncs_dec_skip_space(dec_info->io_uba, 12);
				rc = cpsv_data_access_rsp_decode(&msg_ptr->info.cpnd.info.ckpt_nd2nd_data_rsp,
			 dec_info->io_uba);
				goto free;
				
				          case CPND_EVT_A2ND_CKPT_REFCNTSET:
             ncs_dec_skip_space(dec_info->io_uba, 12);
             rc = cpsv_refcnt_ckptid_decode(&msg_ptr->info.cpnd.info.refCntsetReq,dec_info->io_uba);
             goto free;
			default:
				break;
			}
		}
		/* For all other Events otherthan Write/Read Call EDU */
		rc = m_NCS_EDU_VER_EXEC(&cb->cpnd_edu_hdl, FUNC_NAME(CPSV_EVT),
					dec_info->io_uba, EDP_OP_TYPE_DEC,
					(CPSV_EVT **)&dec_info->o_msg, &ederror, dec_info->i_msg_fmt_ver);

 free:		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("cpnd mds decode failed ");
			m_MMGR_FREE_CPSV_EVT(dec_info->o_msg, NCS_SERVICE_ID_CPND);
		}
		return rc;
	} else {
		/* Drop The Message - Incompatible Message Format Version */
		LOG_ER("cpnd mds decode failed ");
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "INVALID MSG FORMAT IN DECODE FULL\n");	/* Drop The Message - Incompatible Message Format Version */
	}
}

/****************************************************************************
  Name          : cpnd_mds_enc_flat
 
  Description   : This function encodes an events sent from CPA/CPD.
 
  Arguments     : cb    : CPND control Block.
                  enc_info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t cpnd_mds_enc_flat(CPND_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info)
{
	CPSV_EVT *evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCS_UBAID *uba = info->io_uba;

	/* as all the event structures are flat */
	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	if (info->i_to_svc_id == NCSMDS_SVC_ID_CPA) {
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    CPND_WRT_CPA_SUBPART_VER_MIN,
							    CPND_WRT_CPA_SUBPART_VER_MAX, cpnd_cpa_msg_fmt_table);

	} else if (info->i_to_svc_id == NCSMDS_SVC_ID_CPND) {
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    CPND_WRT_CPND_SUBPART_VER_MIN,
							    CPND_WRT_CPND_SUBPART_VER_MAX, cpnd_cpnd_msg_fmt_table);

	} else if (info->i_to_svc_id == NCSMDS_SVC_ID_CPD) {
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    CPND_WRT_CPD_SUBPART_VER_MIN,
							    CPND_WRT_CPD_SUBPART_VER_MAX, cpnd_cpd_msg_fmt_table);

	}

	if (info->o_msg_fmt_ver) {
		evt = (CPSV_EVT *)info->i_msg;
		rc = cpsv_evt_enc_flat(&cb->cpnd_edu_hdl, evt, uba);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_CR("cpsv evt enc flat failed");
		}
		TRACE_LEAVE();
		return rc;
	} else {
		/* Drop The Message  Incompatible Message Format Version */
		LOG_CR("cpnd mds enc flat failed");
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "INVALID MSG FORMAT IN ENCODE FLAT\n");	/* Drop The Message - Incompatible Message Format Version */
	}

}

/****************************************************************************
  Name          : cpnd_mds_dec_flat
 
  Description   : This function decodes an events sent to CPND.
 
  Arguments     : cb    : CPND control Block.
                  dec_info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t cpnd_mds_dec_flat(CPND_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info)
{
	CPSV_EVT *evt = NULL;
	NCS_UBAID *uba = info->io_uba;
	uint32_t rc = NCSCC_RC_SUCCESS;
	bool is_valid_msg_fmt = false;

	if (info->i_fr_svc_id == NCSMDS_SVC_ID_CPA) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     CPND_WRT_CPA_SUBPART_VER_MIN,
							     CPND_WRT_CPA_SUBPART_VER_MAX, cpnd_cpa_msg_fmt_table);
	} else if (info->i_fr_svc_id == NCSMDS_SVC_ID_CPND) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     CPND_WRT_CPND_SUBPART_VER_MIN,
							     CPND_WRT_CPND_SUBPART_VER_MAX, cpnd_cpnd_msg_fmt_table);
	} else if (info->i_fr_svc_id == NCSMDS_SVC_ID_CPD) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     CPND_WRT_CPD_SUBPART_VER_MIN,
							     CPND_WRT_CPD_SUBPART_VER_MAX, cpnd_cpd_msg_fmt_table);
	}

	if (is_valid_msg_fmt) {
		evt = (CPSV_EVT *)m_MMGR_ALLOC_CPSV_EVT(NCS_SERVICE_ID_CPND);
		if (evt == NULL) {
			LOG_CR("cpnd evt alloc failed ");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		memset(evt, '\0', sizeof(CPSV_EVT));
		info->o_msg = evt;
		rc = cpsv_evt_dec_flat(&cb->cpnd_edu_hdl, uba, evt);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_CR("cpnd mds dec flat failed ");
		}
		return rc;
	} else {
		LOG_CR("cpnd mds dec flat failed ");
		TRACE_LEAVE();
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "INVALID MSG FORMAT IN DECODE FLAT\n");	/* Drop The Message - Incompatible Message Format Version */
	}
}

/****************************************************************************
 * Name          : cpnd_mds_rcv
 *
 * Description   : MDS will call this function on receiving CPND messages.
 *
 * Arguments     : cb - CPND Control Block
 *                 rcv_info - MDS Receive information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t cpnd_mds_rcv(CPND_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	CPSV_EVT *pEvt = (CPSV_EVT *)rcv_info->i_msg;
	CPND_SYNC_SEND_NODE *node = NULL;

	pEvt->sinfo.ctxt = rcv_info->i_msg_ctxt;
	pEvt->sinfo.dest = rcv_info->i_fr_dest;
	pEvt->sinfo.to_svc = rcv_info->i_fr_svc_id;
	if (rcv_info->i_rsp_reqd) {
		pEvt->sinfo.stype = MDS_SENDTYPE_SNDRSP;
	}

	m_NCS_LOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);

	/* Check for possible deadlock involving CPND's */
	if ((rcv_info->i_fr_svc_id == NCSMDS_SVC_ID_CPND) && (rcv_info->i_rsp_reqd)) {
		/* If incoming sync send request is from same CPND to which we have done a sync send to 
		   then, send a SA_AIS_ERR_TRY_AGAIN error back to the other CPND which sent the sync send req */
		if ((cb->cpnd_sync_send_in_progress) && (cb->target_cpnd_dest == rcv_info->i_fr_dest)) {
			cpnd_mds_send_try_again_rsp(cb, pEvt);
			cpnd_evt_destroy(pEvt);
			m_NCS_UNLOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);
			TRACE_4("cpnd mds send try again for dest:%"PRIu64,rcv_info->i_fr_dest);
			TRACE_LEAVE();
			return NCSCC_RC_SUCCESS;
		}

		node = (CPND_SYNC_SEND_NODE *)m_MMGR_ALLOC_CPND_SYNC_SEND_NODE;

		if (!node) {
			m_NCS_UNLOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);
			TRACE_4("cpnd sync send node alloc failed");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}

		node->evt = pEvt;
		node->dest = rcv_info->i_fr_dest;

		if (ncs_enqueue(&cb->cpnd_sync_send_list, (void *)node) != NCSCC_RC_SUCCESS) {
			TRACE_4("cpnd ncs enqueue evt failed ");
		}
	}

	m_NCS_UNLOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);

	/* Put it in CPND's Event Queue */
	if (pEvt->info.cpnd.type == CPND_EVT_A2ND_CKPT_INIT ||
	    pEvt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP)
		rc = m_NCS_IPC_SEND(&cb->cpnd_mbx, (NCSCONTEXT)pEvt, NCS_IPC_PRIORITY_HIGH);
	else
		rc = m_NCS_IPC_SEND(&cb->cpnd_mbx, (NCSCONTEXT)pEvt, NCS_IPC_PRIORITY_NORMAL);

	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("cpnd ncs ipc send failed ");
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : cpnd_mds_send_try_again_rsp
 *
 * Description   : This routine sends a SA_AIS_ERR_TRY_AGAIN error to the
                   CPND which sent to sync send request
 *
 * Arguments     : cb - CPND Control Block
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t cpnd_mds_send_try_again_rsp(CPND_CB *cb, CPSV_EVT *pEvt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	CPSV_EVT send_evt;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	send_evt.type = CPSV_EVT_TYPE_CPND;

	switch (pEvt->info.cpnd.type) {

	case CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ:
		send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP;
		send_evt.info.cpnd.info.active_sec_creat_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		break;

	case CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ:
		send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP;
		send_evt.info.cpnd.info.sec_delete_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		break;

	case CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ:
		send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP;
		send_evt.info.cpnd.info.sec_exp_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		break;

	case CPND_EVT_ND2ND_ACTIVE_STATUS:
		send_evt.info.cpnd.type = CPND_EVT_ND2ND_ACTIVE_STATUS_ACK;
		send_evt.info.cpnd.info.status.error = SA_AIS_ERR_TRY_AGAIN;
		break;

	case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ:
	case CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ:

		send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP;

		switch (pEvt->info.cpnd.info.ckpt_nd2nd_data.type) {

		case CPSV_CKPT_ACCESS_READ:
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.type = CPSV_DATA_ACCESS_RMT_READ_RSP;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.error = SA_AIS_ERR_TRY_AGAIN;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.num_of_elmts = -1;
			break;

		case CPSV_CKPT_ACCESS_WRITE:
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.type = CPSV_DATA_ACCESS_WRITE_RSP;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.error = SA_AIS_ERR_TRY_AGAIN;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.num_of_elmts = -1;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.size = 0;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.info.write_err_index = NULL;
			break;

		case CPSV_CKPT_ACCESS_OVWRITE:
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.type = CPSV_DATA_ACCESS_OVWRITE_RSP;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.info.ovwrite_error.error = SA_AIS_ERR_TRY_AGAIN;
			break;
		}
		break;

	case CPND_EVT_ND2ND_CKPT_SYNC_REQ:
		send_evt.info.cpnd.type = CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC;
		send_evt.info.cpnd.error = SA_AIS_ERR_TRY_AGAIN;
		break;

	case CPND_EVT_ND2ND_CKPT_ITER_NEXT_REQ:
		send_evt.info.cpnd.type = CPND_EVT_ND2ND_CKPT_ACTIVE_ITERNEXT;
		send_evt.info.cpnd.info.ckpt_nd2nd_getnext_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		break;

	default:
		return NCSCC_RC_FAILURE;
		break;
	}

	rc = cpnd_mds_send_rsp(cb, &pEvt->sinfo, &send_evt);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_CR("cpnd mds send failed");
	}

	return rc;
}

/****************************************************************************
 * Name          : cpnd_mds_bcast_send
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
uint32_t cpnd_mds_bcast_send(CPND_CB *cb, CPSV_EVT *evt, NCSMDS_SVC_ID to_svc)
{

	NCSMDS_INFO info;
	uint32_t res;

	memset(&info, 0, sizeof(info));

	info.i_mds_hdl = cb->cpnd_mds_hdl;
	info.i_op = MDS_SEND;
	info.i_svc_id = NCSMDS_SVC_ID_CPND;

	info.info.svc_send.i_msg = (NCSCONTEXT)evt;
	info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	info.info.svc_send.i_sendtype = MDS_SENDTYPE_BCAST;
	info.info.svc_send.i_to_svc = to_svc;
	info.info.svc_send.info.bcast.i_bcast_scope = NCSMDS_SCOPE_INTRANODE;

	res = ncsmds_api(&info);
	return (res);
}

/****************************************************************************
 * Name          : cpnd_mds_svc_evt
 *
 * Description   : CPND is informed when MDS events occurr that he has 
 *                 subscribed to
 *
 * Arguments     : 
 *   cb          : CPND control Block.
 *   enc_info    : Svc evt info.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t cpnd_mds_svc_evt(CPND_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{
	CPSV_EVT *evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS, priority = NCS_IPC_PRIORITY_HIGH;
	uint32_t phy_slot_sub_slot;

	TRACE_ENTER();
	if (svc_evt->i_svc_id == NCSMDS_SVC_ID_CPD) {

		m_NCS_LOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);

		switch (svc_evt->i_change) {
		case NCSMDS_DOWN:
			if (cb->is_cpd_up == true) {
				/* If CPD is already UP */
				cb->is_cpd_up = false;
				TRACE_4("cpnd cpd service went down");
				m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);
				TRACE_LEAVE();
				return NCSCC_RC_SUCCESS;
			}
			break;
		case NCSMDS_UP:
			cb->is_cpd_up = true;
			cb->cpd_mdest_id = svc_evt->i_dest;
			TRACE_4("cpnd cpd service came up ");
			break;

		case NCSMDS_NO_ACTIVE:
			cb->is_cpd_up = false;
			TRACE_4("cpnd cpd service noactive");
			break;

		case NCSMDS_NEW_ACTIVE:
			cb->is_cpd_up = true;
			TRACE_4("cpnd cpd service newactive");
			break;

		case NCSMDS_RED_UP:
			if (svc_evt->i_role == V_DEST_RL_ACTIVE) {

				phy_slot_sub_slot = cpnd_get_slot_sub_slot_id_from_node_id(svc_evt->i_node_id);
				cb->cpnd_active_id = phy_slot_sub_slot;
				cb->is_cpd_up = true;
			} else if (svc_evt->i_role == V_DEST_RL_STANDBY) {

				phy_slot_sub_slot = cpnd_get_slot_sub_slot_id_from_node_id(svc_evt->i_node_id);
				cb->cpnd_standby_id = phy_slot_sub_slot;
				m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);
				TRACE_LEAVE();
				return NCSCC_RC_SUCCESS;
			}
			break;
		case NCSMDS_RED_DOWN:
			phy_slot_sub_slot = cpnd_get_slot_sub_slot_id_from_node_id(svc_evt->i_node_id);
			if (cb->cpnd_active_id == phy_slot_sub_slot)
				cb->is_cpd_up = false;
			break;
		case NCSMDS_CHG_ROLE:
			if (svc_evt->i_role == V_DEST_RL_ACTIVE) {

				phy_slot_sub_slot = cpnd_get_slot_sub_slot_id_from_node_id(svc_evt->i_node_id);
				cb->cpnd_active_id = phy_slot_sub_slot;
				cb->is_cpd_up = true;
			} else if (svc_evt->i_role == V_DEST_RL_STANDBY) {

				phy_slot_sub_slot = cpnd_get_slot_sub_slot_id_from_node_id(svc_evt->i_node_id);
				cb->cpnd_standby_id = phy_slot_sub_slot;
			}
			break;

		default:
			break;
		}

		priority = NCS_IPC_PRIORITY_VERY_HIGH;

		m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);
	}
	if (svc_evt->i_svc_id == NCSMDS_SVC_ID_CPA) {
		if (m_NCS_NODE_ID_FROM_MDS_DEST(cb->cpnd_mdest_id) != m_NCS_NODE_ID_FROM_MDS_DEST(svc_evt->i_dest)) {
			TRACE_LEAVE();
			return rc;
		}
	}
	/* Send the CPND_EVT_MDS_INFO to CPND */
	evt = m_MMGR_ALLOC_CPSV_EVT(NCS_SERVICE_ID_CPND);
	memset(evt, 0, sizeof(CPSV_EVT));
	evt->type = CPSV_EVT_TYPE_CPND;
	evt->info.cpnd.type = CPND_EVT_MDS_INFO;
	evt->info.cpnd.info.mds_info.change = svc_evt->i_change;
	evt->info.cpnd.info.mds_info.dest = svc_evt->i_dest;
	evt->info.cpnd.info.mds_info.svc_id = svc_evt->i_svc_id;
	evt->info.cpnd.info.mds_info.role = svc_evt->i_role;

	/* Put it in CPND's Event Queue */
	rc = m_NCS_IPC_SEND(&cb->cpnd_mbx, (NCSCONTEXT)evt, priority);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("cpnd ncs ipc send failed");
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : cpnd_mds_send_rsp
 *
 * Description   : Send the Response to Sync Requests
 *
 * Arguments     : 
 *
 * Return Values : 
 *
 * Notes         :
 *****************************************************************************/
uint32_t cpnd_mds_send_rsp(CPND_CB *cb, CPSV_SEND_INFO *s_info, CPSV_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;
	TRACE_ENTER();

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->cpnd_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CPND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;

	mds_info.info.svc_send.i_to_svc = s_info->to_svc;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_RSP;
	mds_info.info.svc_send.info.rsp.i_msg_ctxt = s_info->ctxt;
	mds_info.info.svc_send.info.rsp.i_sender_dest = s_info->dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE_4("cpnd mds send fail for dest:%"PRIu64,s_info->dest);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : cpnd_mds_msg_sync_send
 
  Description   : This routine sends the Sinc requests from CPND
 
  Arguments     : cb  - ptr to the CPND CB
                  i_evt - ptr to the CPSV message
                  o_evt - ptr to the CPSV message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t cpnd_mds_msg_sync_send(CPND_CB *cb, uint32_t to_svc, MDS_DEST to_dest,
			     CPSV_EVT *i_evt, CPSV_EVT **o_evt, uint32_t timeout)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;
	CPND_SYNC_SEND_NODE *node = NULL;

	TRACE_ENTER();
	if (!i_evt) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	m_NCS_LOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);

	if ((to_svc == NCSMDS_SVC_ID_CPD) && (cb->is_cpd_up == false)) {
		m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);
		TRACE_4("cpnd cpd service is down ");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);

	if (to_svc == NCSMDS_SVC_ID_CPND) {

		m_NCS_LOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);

		node = ncs_find_item(&cb->cpnd_sync_send_list, (void *)&to_dest, cpnd_match_dest);

		if (!node) {
			/* received no sync sends from CPND to which we are going to do a sync send */
			cb->cpnd_sync_send_in_progress = true;
			cb->target_cpnd_dest = to_dest;
		} else {
			m_NCS_UNLOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);
			TRACE_4("cpnd mds send fail for dest:%"PRIu64,to_dest);
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}

		m_NCS_UNLOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);
	}

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->cpnd_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CPND;
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
		TRACE_4("cpnd mds send fail for to_dest:%"PRIu64",return value:%d",to_dest, rc);
	}

	/* Reset deadlock prevention flags in case of sync send to another CPND */
	if (to_svc == NCSMDS_SVC_ID_CPND) {
		m_NCS_LOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);
		cb->cpnd_sync_send_in_progress = false;
		cb->target_cpnd_dest = 0;
		m_NCS_UNLOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : cpnd_mds_msg_send
 
  Description   : This routine sends the Events from CPND
 
  Arguments     : cb  - ptr to the CPND CB
                  i_evt - ptr to the CPSV message
                  o_evt - ptr to the CPSV message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t cpnd_mds_msg_send(CPND_CB *cb, uint32_t to_svc, MDS_DEST to_dest, CPSV_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;

	TRACE_ENTER();
	if (!evt) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	m_NCS_LOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);

	if ((to_svc == NCSMDS_SVC_ID_CPD) && (cb->is_cpd_up == false)) {
		/* CPD is not UP */
		TRACE_4("cpnd cpd service is down");
		m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->cpnd_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CPND;
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
		TRACE_4("cpnd mds send failed for dest:%"PRIu64,to_dest);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : 
 *
 * Description   : Function to process the   
 *                 from Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t cpnd_mds_msg_sync_ack_send(CPND_CB *cb, uint32_t to_svc, MDS_DEST to_dest, CPSV_EVT *i_evt, uint32_t timeout)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;

	TRACE_ENTER();
	if (!i_evt) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	m_NCS_LOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);

	if ((to_svc == NCSMDS_SVC_ID_CPD) && (cb->is_cpd_up == false)) {
		/* CPD is not UP */
		TRACE_4("cpnd cpd service is down ");
		m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->cpnd_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CPND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = to_svc;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDACK;

	/* fill the send rsp strcuture */
	mds_info.info.svc_send.info.sndack.i_time_to_wait = timeout;	/* timeto wait in 10ms */
	mds_info.info.svc_send.info.sndack.i_to_dest = to_dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_4("cpnd mds send failed for dest:%"PRIu64,to_dest);
	}
	TRACE_LEAVE();
	return rc;
}
