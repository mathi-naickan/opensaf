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

  This file contains routines used by GLND library for MDS interaction.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "lck/lcknd/glnd.h"
#include <string.h>
#define m_GLND_MBX_SEND(mbx,msg,prio) m_NCS_IPC_SEND(mbx,(NCSCONTEXT)msg,prio)
#define m_GLND_MBX_RECV(mbx,evt)      m_NCS_IPC_RECEIVE(mbx,evt)

uint32_t glnd_mds_callback(struct ncsmds_callback_info *info);
static uint32_t glnd_mds_cpy(GLND_CB *cb, MDS_CALLBACK_COPY_INFO *info);
static uint32_t glnd_mds_enc(GLND_CB *cb, MDS_CALLBACK_ENC_INFO *info);
static uint32_t glnd_mds_dec(GLND_CB *cb, MDS_CALLBACK_DEC_INFO *info);
static uint32_t glnd_mds_enc_flat(GLND_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uint32_t glnd_mds_dec_flat(GLND_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uint32_t glnd_mds_rcv(GLND_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info);
static uint32_t glnd_mds_svc_evt(GLND_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt);
static uint32_t glnd_send_agent_going_down_event(GLND_CB *cb, MDS_DEST dest);
static uint32_t glsv_dec_rsc_purge_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt);
static uint32_t glsv_dec_rsc_unlock_evt(NCS_UBAID *uba, GLSV_EVT_RSC_UNLOCK_INFO *evt);
static uint32_t glsv_dec_rsc_lock_evt(NCS_UBAID *uba, GLSV_EVT_RSC_LOCK_INFO *evt);
static uint32_t glsv_dec_rsc_close_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt);
static uint32_t glsv_dec_rsc_open_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt);
static uint32_t glsv_dec_finalize_evt(NCS_UBAID *uba, GLSV_EVT_FINALIZE_INFO *evt);
static uint32_t glsv_dec_initialize_evt(NCS_UBAID *uba, GLSV_EVT_CLIENT_INFO *evt);
static uint32_t glsv_dec_reg_unreg_agent_evt(NCS_UBAID *uba, GLSV_EVT_AGENT_INFO *evt);
static uint32_t glsv_dec_client_info_evt(NCS_UBAID *uba, GLSV_EVT_RESTART_CLIENT_INFO *evt);
static uint32_t glsv_gla_enc_callbk_evt(NCS_UBAID *uba, GLSV_GLA_CALLBACK_INFO *evt);
static uint32_t glsv_gla_enc_api_resp_evt(NCS_UBAID *uba, GLSV_GLA_API_RESP_INFO *evt);

uint32_t glnd_mds_get_handle(GLND_CB *cb);

/*To store the message format versions*/
static const MSG_FRMT_VER glnd_gla_msg_fmt_table[GLND_WRT_GLA_SUBPART_VER_RANGE] = { 1 };
static const MSG_FRMT_VER glnd_glnd_msg_fmt_table[GLND_WRT_GLND_SUBPART_VER_RANGE] = { 1 };
static const MSG_FRMT_VER glnd_gld_msg_fmt_table[GLND_WRT_GLD_SUBPART_VER_RANGE] = { 1 };

/****************************************************************************
 * Name          : glnd_mds_get_handle
 *
 * Description   : This function Gets the Handles of local MDS destination
 *
 * Arguments     : cb   : GLND control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t glnd_mds_get_handle(GLND_CB *cb)
{
	NCSADA_INFO arg;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	memset(&arg, 0, sizeof(NCSADA_INFO));
	arg.req = NCSADA_GET_HDLS;
	rc = ncsada_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("GLND mds get handle failed");
		goto end;
	}
	cb->glnd_mds_hdl = arg.info.adest_get_hdls.o_mds_pwe1_hdl;
end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glnd_mds_register
 
  Description   : This routine registers the GLND Service with MDS.
 
  Arguments     : glnd_cb - ptr to the GLND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uint32_t glnd_mds_register(GLND_CB *cb)
{
	NCSMDS_INFO svc_info;
	MDS_SVC_ID svc_id[] = { NCSMDS_SVC_ID_GLND, NCSMDS_SVC_ID_GLD };
	MDS_SVC_ID gla_svc_id[] = { NCSMDS_SVC_ID_GLA };
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	/* STEP1: Get the MDS Handle */
	glnd_mds_get_handle(cb);

	/* memset the svc_info */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));

	/* STEP 2 : Install on ADEST with MDS with service ID NCSMDS_SVC_ID_GLND. */
	svc_info.i_mds_hdl = cb->glnd_mds_hdl;
	svc_info.i_svc_id = NCSMDS_SVC_ID_GLND;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = cb->cb_hdl_id;
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_install.i_svc_cb = glnd_mds_callback;	/* callback */
	svc_info.info.svc_install.i_mds_q_ownership = false;
	svc_info.info.svc_install.i_mds_svc_pvt_ver = GLND_PVT_SUBPART_VERSION;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("GLND mds register failed");
		goto end;
	}
	cb->glnd_mdest_id = svc_info.info.svc_install.o_dest;
	/* o_anc : Not required because VDA installs on an ADEST */
	/* o_sel_obj : Not required, as VDA does not use mds_q_ownership option */

	/* STEP 3: Subscribe to GLD and GLND up/down events */
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 2;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("GLND mds register failed");
		/* Uninstall with the mds */
		svc_info.i_op = MDS_UNINSTALL;
		if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) 
			LOG_ER("GLND mds unregister failed");
		goto end;
	}

	/* Step 4: Subscribe to GLA up/down event */
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_INTRANODE;
	svc_info.info.svc_subscribe.i_svc_ids = gla_svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("GLND mds register failed");
		/* Uninstall with the mds */
		svc_info.i_op = MDS_UNINSTALL;
		if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
			LOG_ER("GLND mds unregister failed");
		}
		goto end;
	}
	rc = NCSCC_RC_SUCCESS;
	
end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : glnd_mds_unregesiter
 *
 * Description   : This function un-registers the GLND Service with MDS.
 *
 * Arguments     : cb   : GLND control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void glnd_mds_unregister(GLND_CB *cb)
{
	NCSMDS_INFO arg;
	TRACE_ENTER();

	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->glnd_mds_hdl;
	arg.i_svc_id = NCSMDS_SVC_ID_GLND;
	arg.i_op = MDS_UNINSTALL;

	if (ncsmds_api(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("GLND mds unregister failed");
	}
	
	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : glnd_mds_callback
 
  Description   : This callback routine will be called by MDS on event arrival
 
  Arguments     : info - pointer to the mds callback info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t glnd_mds_callback(struct ncsmds_callback_info *info)
{
	GLND_CB *glnd_cb = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (info == NULL)
		goto end;

	glnd_cb = (GLND_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLND, (uint32_t)info->i_yr_svc_hdl);
	if (!glnd_cb) {
		TRACE_2("GLND cb create failed");
		goto end;
	}

	switch (info->i_op) {
	case MDS_CALLBACK_COPY:
		rc = glnd_mds_cpy(glnd_cb, &info->info.cpy);
		break;
	case MDS_CALLBACK_ENC:
		rc = glnd_mds_enc(glnd_cb, &info->info.enc);
		break;
	case MDS_CALLBACK_DEC:
		rc = glnd_mds_dec(glnd_cb, &info->info.dec);
		break;
	case MDS_CALLBACK_ENC_FLAT:
		rc = glnd_mds_enc_flat(glnd_cb, &info->info.enc_flat);
		break;
	case MDS_CALLBACK_DEC_FLAT:
		rc = glnd_mds_dec_flat(glnd_cb, &info->info.dec_flat);;
		break;
	case MDS_CALLBACK_RECEIVE:
		rc = glnd_mds_rcv(glnd_cb, &info->info.receive);
		break;
	case MDS_CALLBACK_SVC_EVENT:
		rc = glnd_mds_svc_evt(glnd_cb, &info->info.svc_evt);
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("GLND mds callback process failed");
	}
	ncshm_give_hdl((uint32_t)info->i_yr_svc_hdl);
end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glnd_mds_cpy
 
  Description   : This function encodes an events sent from GLND.
 
  Arguments     : cb    : GLND_CB control Block.
                  info  : Info for copying
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t glnd_mds_cpy(GLND_CB *cb, MDS_CALLBACK_COPY_INFO *info)
{
	info->o_cpy = (NCSCONTEXT)NULL;
	if (info->i_to_svc_id != NCSMDS_SVC_ID_GLD) {
		/* this situation should never arise. as GLA and GLND never co-exist in the same process.
		   similarly GLND-GLND never co-exits in the same process. The only possibility is the 
		   GLND-GLD. */
		return NCSCC_RC_FAILURE;
	}

	/*
	   src = (GLSV_GLD_EVT *)info->i_msg;
	   cpy = m_MMGR_ALLOC_GLND_EVT;
	   if (cpy == NULL)
	   {
	   LOG_CR("GLND_EVT_ALLOC_FAILED");
	   return NCSCC_RC_FAILURE;
	   }
	   memset(cpy,0,sizeof(GLSV_GLND_EVT));
	   *cpy = *src;
	   info->o_cpy = (NCSCONTEXT)cpy; */
	
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : glnd_mds_enc
 
  Description   : This function encodes an events sent from GLND.
 
  Arguments     : cb    : GLND_CB control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t glnd_mds_enc(GLND_CB *cb, MDS_CALLBACK_ENC_INFO *info)
{
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_FAILURE;
	NCS_UBAID *uba = info->io_uba;
	TRACE_ENTER();
	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	switch (info->i_to_svc_id) {
	case NCSMDS_SVC_ID_GLA:
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    GLND_WRT_GLA_SUBPART_VER_AT_MIN_MSG_FMT,
							    GLND_WRT_GLA_SUBPART_VER_AT_MAX_MSG_FMT,
							    glnd_gla_msg_fmt_table);
		if (info->o_msg_fmt_ver) {
			GLSV_GLA_EVT *evt;
			uint8_t *p8;

			evt = (GLSV_GLA_EVT *)info->i_msg;
	  /** encode the type of message **/
			p8 = ncs_enc_reserve_space(uba, 16);
			if (!p8) {
				TRACE_2("GLND enc failed");
				goto end;
			}
			ncs_encode_64bit(&p8, evt->handle);
			ncs_encode_32bit(&p8, evt->error);
			ncs_encode_32bit(&p8, evt->type);
			ncs_enc_claim_space(uba, 16);
			switch (evt->type) {
			case GLSV_GLA_CALLBK_EVT:
				rc = glsv_gla_enc_callbk_evt(uba, &evt->info.gla_clbk_info);
				break;

			case GLSV_GLA_API_RESP_EVT:
				rc = glsv_gla_enc_api_resp_evt(uba, &evt->info.gla_resp_info);
				break;

			default:
				goto end;
			}
			goto end;
		}
		break;

	case NCSMDS_SVC_ID_GLND:
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    GLND_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							    GLND_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							    glnd_glnd_msg_fmt_table);
		if (info->o_msg_fmt_ver) {
			rc = m_NCS_EDU_EXEC(&cb->glnd_edu_hdl, glsv_edp_glnd_evt, info->io_uba,
					    EDP_OP_TYPE_ENC, (GLSV_GLND_EVT *)info->i_msg, &ederror);
			if (rc != NCSCC_RC_SUCCESS) {
				/* Free calls to be added here. */
				m_NCS_EDU_PRINT_ERROR_STRING(ederror);
				goto end;
			}
			goto end;
		}
		break;

	case NCSMDS_SVC_ID_GLD:
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    GLND_WRT_GLD_SUBPART_VER_AT_MIN_MSG_FMT,
							    GLND_WRT_GLD_SUBPART_VER_AT_MAX_MSG_FMT,
							    glnd_gld_msg_fmt_table);
		if (info->o_msg_fmt_ver) {
			rc = m_NCS_EDU_EXEC(&cb->glnd_edu_hdl, glsv_edp_gld_evt, info->io_uba,
					    EDP_OP_TYPE_ENC, (GLSV_GLD_EVT *)info->i_msg, &ederror);
			if (rc != NCSCC_RC_SUCCESS) {
				/* Free calls to be added here. */
				m_NCS_EDU_PRINT_ERROR_STRING(ederror);
				goto end;
			}
			goto end;
		}
		break;

	default:
		goto end;
	}

	if (!info->o_msg_fmt_ver) {	/* Drop The Message */
		TRACE_2("GLND msg format version invalid");
		goto end;
	}
end:
	TRACE_LEAVE();	
	return rc;
}

/****************************************************************************
  Name          : glnd_mds_dec
 
  Description   : This function decodes an events sent to GLND.
 
  Arguments     : cb    : GLND control Block.
                  info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t glnd_mds_dec(GLND_CB *cb, MDS_CALLBACK_DEC_INFO *info)
{
	EDU_ERR ederror = 0;
	GLSV_GLND_EVT *evt;
	uint32_t rc = NCSCC_RC_FAILURE;
	uint8_t *p8, local_data[20];
	bool is_valid_msg_fmt;
	NCS_UBAID *uba = info->io_uba;
	TRACE_ENTER();

	switch (info->i_fr_svc_id) {
	case NCSMDS_SVC_ID_GLA:
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     GLND_WRT_GLA_SUBPART_VER_AT_MIN_MSG_FMT,
							     GLND_WRT_GLA_SUBPART_VER_AT_MAX_MSG_FMT,
							     glnd_gla_msg_fmt_table);
		break;

	case NCSMDS_SVC_ID_GLND:
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     GLND_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							     GLND_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							     glnd_glnd_msg_fmt_table);
		break;

	case NCSMDS_SVC_ID_GLD:
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     GLND_WRT_GLD_SUBPART_VER_AT_MIN_MSG_FMT,
							     GLND_WRT_GLD_SUBPART_VER_AT_MAX_MSG_FMT,
							     glnd_gld_msg_fmt_table);
		break;

	default:
		goto end;
	}

	if (is_valid_msg_fmt) {
		evt = m_MMGR_ALLOC_GLND_EVT;
		if (evt == NULL) {
			LOG_CR("GLND evt alloc failed: Error %s", strerror(errno));
			assert(0);
		}
		memset(evt, 0, sizeof(GLSV_GLND_EVT));

		info->o_msg = (NCSCONTEXT)evt;
		if (NCSMDS_SVC_ID_GLA == info->i_fr_svc_id) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			evt->type = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);
			switch (evt->type) {
			case GLSV_GLND_EVT_REG_AGENT:
				rc = glsv_dec_reg_unreg_agent_evt(uba, &evt->info.agent_info);
				break;

			case GLSV_GLND_EVT_UNREG_AGENT:
				rc = glsv_dec_reg_unreg_agent_evt(uba, &evt->info.agent_info);
				break;

			case GLSV_GLND_EVT_INITIALIZE:
				rc = glsv_dec_initialize_evt(uba, &evt->info.client_info);
				break;

			case GLSV_GLND_EVT_FINALIZE:
				rc = glsv_dec_finalize_evt(uba, &evt->info.finalize_info);
				break;

			case GLSV_GLND_EVT_RSC_OPEN:
				rc = glsv_dec_rsc_open_evt(uba, &evt->info.rsc_info);
				break;

			case GLSV_GLND_EVT_RSC_CLOSE:
				rc = glsv_dec_rsc_close_evt(uba, &evt->info.rsc_info);
				break;

			case GLSV_GLND_EVT_RSC_LOCK:
				rc = glsv_dec_rsc_lock_evt(uba, &evt->info.rsc_lock_info);
				break;

			case GLSV_GLND_EVT_RSC_UNLOCK:
				rc = glsv_dec_rsc_unlock_evt(uba, &evt->info.rsc_unlock_info);
				break;

			case GLSV_GLND_EVT_RSC_PURGE:
				rc = glsv_dec_rsc_purge_evt(uba, &evt->info.rsc_info);
				break;

			case GLSV_GLND_EVT_CLIENT_INFO:
				rc = glsv_dec_client_info_evt(uba, &evt->info.restart_client_info);
				break;

			default:
				goto end;
			}

		} else {
			rc = m_NCS_EDU_EXEC(&cb->glnd_edu_hdl, glsv_edp_glnd_evt, info->io_uba,
					    EDP_OP_TYPE_DEC, (GLSV_GLND_EVT **)&info->o_msg, &ederror);
			if (rc != NCSCC_RC_SUCCESS) {
				m_MMGR_FREE_GLND_EVT(evt);
				m_NCS_EDU_PRINT_ERROR_STRING(ederror);
				goto end;
			}
		}
		evt->priority = NCS_IPC_PRIORITY_NORMAL;
		goto end;
	} else {
		/* Drop The Message */
		TRACE_2("GLND msg format version invalid");
		goto end;
	}
end: 
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glnd_mds_enc_flat
 
  Description   : This function encodes an events sent from GLND.
 
  Arguments     : cb    : GLND_CB control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t glnd_mds_enc_flat(GLND_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info)
{
	NCS_UBAID *uba = info->io_uba;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	switch (info->i_to_svc_id) {
	case NCSMDS_SVC_ID_GLA:
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    GLND_WRT_GLA_SUBPART_VER_AT_MIN_MSG_FMT,
							    GLND_WRT_GLA_SUBPART_VER_AT_MAX_MSG_FMT,
							    glnd_gla_msg_fmt_table);
		if (info->o_msg_fmt_ver) {
			GLSV_GLA_EVT *evt;
			evt = (GLSV_GLA_EVT *)info->i_msg;
			ncs_encode_n_octets_in_uba(uba, (uint8_t *)evt, sizeof(GLSV_GLA_EVT));
			goto end;
		}
		break;

	case NCSMDS_SVC_ID_GLND:
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    GLND_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							    GLND_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							    glnd_glnd_msg_fmt_table);
		if (info->o_msg_fmt_ver) {
			rc = m_NCS_EDU_EXEC(&cb->glnd_edu_hdl, glsv_edp_glnd_evt, info->io_uba,
					    EDP_OP_TYPE_ENC, (GLSV_GLND_EVT *)info->i_msg, &ederror);
			if (rc != NCSCC_RC_SUCCESS) {
				/* Free calls to be added here. */
				m_NCS_EDU_PRINT_ERROR_STRING(ederror);
				rc = NCSCC_RC_FAILURE;
				goto end;
			}
			goto end;
		}
		break;

	case NCSMDS_SVC_ID_GLD:
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    GLND_WRT_GLD_SUBPART_VER_AT_MIN_MSG_FMT,
							    GLND_WRT_GLD_SUBPART_VER_AT_MAX_MSG_FMT,
							    glnd_gld_msg_fmt_table);
		if (info->o_msg_fmt_ver) {
			GLSV_GLD_EVT *evt;
			evt = (GLSV_GLD_EVT *)info->i_msg;
			ncs_encode_n_octets(uba->ub, (uint8_t *)evt, sizeof(GLSV_GLD_EVT));
			goto end;
		}
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		goto end;
	}
	if (!info->o_msg_fmt_ver) {
		/* Drop The Message */
		TRACE_2("GLND msg format version invalid");
		rc = NCSCC_RC_FAILURE;
		goto end;
	}
end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glnd_mds_dec_flat
 
  Description   : This function decodes an events sent from GLND.
 
  Arguments     : cb    : GLND_CB control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t glnd_mds_dec_flat(GLND_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info)
{
	GLSV_GLND_EVT *evt;
	NCS_UBAID *uba = info->io_uba;
	bool is_valid_msg_fmt = false;
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	switch (info->i_fr_svc_id) {
	case NCSMDS_SVC_ID_GLA:
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     GLND_WRT_GLA_SUBPART_VER_AT_MIN_MSG_FMT,
							     GLND_WRT_GLA_SUBPART_VER_AT_MAX_MSG_FMT,
							     glnd_gla_msg_fmt_table);
		break;

	case NCSMDS_SVC_ID_GLND:
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     GLND_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							     GLND_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							     glnd_glnd_msg_fmt_table);

		if (is_valid_msg_fmt) {
			evt = m_MMGR_ALLOC_GLND_EVT;
			memset(evt, 0, sizeof(GLSV_GLND_EVT));
			info->o_msg = (NCSCONTEXT)evt;
			rc = m_NCS_EDU_EXEC(&cb->glnd_edu_hdl, glsv_edp_glnd_evt, info->io_uba,
					    EDP_OP_TYPE_DEC, (GLSV_GLND_EVT **)&info->o_msg, &ederror);
			if (rc != NCSCC_RC_SUCCESS) {
				m_MMGR_FREE_GLND_EVT(evt);
				m_NCS_EDU_PRINT_ERROR_STRING(ederror);
				return rc;
			}
			evt->priority = NCS_IPC_PRIORITY_NORMAL;
			return rc;
		}
		break;

	case NCSMDS_SVC_ID_GLD:
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     GLND_WRT_GLD_SUBPART_VER_AT_MIN_MSG_FMT,
							     GLND_WRT_GLD_SUBPART_VER_AT_MAX_MSG_FMT,
							     glnd_gld_msg_fmt_table);
		break;

	default:
		return NCSCC_RC_FAILURE;
	}

	if (is_valid_msg_fmt) {
		switch (info->i_fr_svc_id) {
		case NCSMDS_SVC_ID_GLA:
		case NCSMDS_SVC_ID_GLD:
			evt = (GLSV_GLND_EVT *)m_MMGR_ALLOC_GLND_EVT;
			if (evt == NULL) {
				LOG_CR("GLND evt alloc failed: Error %s", strerror(errno));
				assert(0);
			}
			memset(evt, 0, sizeof(GLSV_GLND_EVT));
			info->o_msg = evt;
			ncs_decode_n_octets(uba->ub, (uint8_t *)evt, sizeof(GLSV_GLND_EVT));
			if (evt->type == GLSV_GLND_EVT_RSC_MASTER_INFO) {
				if (evt->info.rsc_master_info.no_of_res > 0) {
					GLSV_GLND_RSC_MASTER_INFO_LIST *rsc_master_list = NULL;
					rsc_master_list =
					    m_MMGR_ALLOC_GLND_RES_MASTER_LIST_INFO(evt->info.rsc_master_info.no_of_res);
					if (rsc_master_list == NULL) {
						LOG_CR("GLND evt alloc failed: Error %s", strerror(errno));
						assert(0);
					}
					ncs_decode_n_octets(uba->ub, (uint8_t *)rsc_master_list,
							    sizeof(GLSV_GLND_RSC_MASTER_INFO_LIST) *
							    evt->info.rsc_master_info.no_of_res);

					evt->info.rsc_master_info.rsc_master_list = rsc_master_list;
				}

			}
			break;
		default:
			return NCSCC_RC_FAILURE;
		}
		return NCSCC_RC_SUCCESS;
	} else {
		/* Drop The Message */
		TRACE_2("GLND msg format version invalid");
		return NCSCC_RC_FAILURE;
	}
	
}

/****************************************************************************
 * Name          : glnd_mds_rcv
 *
 * Description   : MDS will call this function on receiving messages.
 *
 * Arguments     : cb - GLND Control Block
 *                 rcv_info - MDS Receive information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t glnd_mds_rcv(GLND_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	GLSV_GLND_EVT *evt = (GLSV_GLND_EVT *)rcv_info->i_msg;
	/* check the event type */
	evt->glnd_hdl = cb->cb_hdl_id;

	/* set the reponse context if set */
	if (rcv_info->i_rsp_reqd == true) {
		if (rcv_info->i_msg_ctxt.length) {
			evt->mds_context.length = rcv_info->i_msg_ctxt.length;
			memcpy(evt->mds_context.data, rcv_info->i_msg_ctxt.data, rcv_info->i_msg_ctxt.length);
		} else
			goto done;
	}

	if (evt->type > GLSV_GLND_EVT_BASE && evt->type < GLSV_GLND_EVT_MAX) {
		if (glnd_evt_local_send(cb, evt, NCS_IPC_PRIORITY_NORMAL) == NCSCC_RC_SUCCESS)
			return NCSCC_RC_SUCCESS;
	}
 done:
	glnd_evt_destroy(evt);
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : glnd_mds_svc_evt
 *
 * Description   : GLND is informed when MDS events occurr that he has 
 *                 subscribed to
 *
 * Arguments     : 
 *   cb          : GLND control Block.
 *   enc_info    : Svc evt info.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t glnd_mds_svc_evt(GLND_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{

	switch (svc_evt->i_change) {
	case NCSMDS_NO_ACTIVE:
		cb->gld_card_up = false;
		break;
	case NCSMDS_NEW_ACTIVE:
		cb->gld_card_up = true;
		break;
	case NCSMDS_DOWN:
		if (svc_evt->i_svc_id == NCSMDS_SVC_ID_GLD) {
			if (cb->gld_card_up == true) {
				memset(&cb->gld_mdest_id, 0, sizeof(MDS_DEST));
			}
			cb->gld_card_up = false;
		} else if (svc_evt->i_svc_id == NCSMDS_SVC_ID_GLA) {
			/* send an event about the agent going down */
			glnd_send_agent_going_down_event(cb, svc_evt->i_dest);
		}
		break;
	case NCSMDS_UP:
		switch (svc_evt->i_svc_id) {
		case NCSMDS_SVC_ID_GLD:
			if (cb->gld_card_up == false) {
				cb->gld_mdest_id = svc_evt->i_dest;
			}
			cb->gld_card_up = true;

			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : glnd_mds_msg_send_gla 
 
  Description   : This routine sends the GLND messages to GLA.
 
  Arguments     : cb  - ptr to the GLND CB
                  i_evt - ptr to the GLA message
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t glnd_mds_msg_send_gla(GLND_CB *cb, GLSV_GLA_EVT *i_evt, MDS_DEST to_mds_dest)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	if (!i_evt)
		goto end;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->glnd_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_GLND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLA;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the send strcuture */
	mds_info.info.svc_send.info.snd.i_to_dest = to_mds_dest;

	/* send the message */
	if (ncsmds_api(&mds_info) != NCSCC_RC_SUCCESS) {
		LOG_ER("GLND mds send failure: from mds_dest %u evt_type %u",
				     m_NCS_NODE_ID_FROM_MDS_DEST(to_mds_dest), i_evt->type);
		goto end;
	}
	rc = NCSCC_RC_SUCCESS;
end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glnd_mds_msg_send_rsp_gla 
 
  Description   : This routine sends the GLND response messages to GLA.
 
  Arguments     : cb  - ptr to the GLND CB
                  i_evt - ptr to the GLA message
                  to_mds_dest  - To address to send the message.
                  mds_ctxt - The mds context.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t glnd_mds_msg_send_rsp_gla(GLND_CB *cb, GLSV_GLA_EVT *i_evt, MDS_DEST to_mds_dest, MDS_SYNC_SND_CTXT *mds_ctxt)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (!i_evt)
		goto end;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->glnd_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_GLND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLA;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_RSP;
	mds_info.info.svc_send.info.rsp.i_sender_dest = to_mds_dest;
	memcpy(mds_info.info.svc_send.info.rsp.i_msg_ctxt.data, mds_ctxt->data, mds_ctxt->length);
	mds_info.info.svc_send.info.rsp.i_msg_ctxt.length = mds_ctxt->length;
	/* send the message */
	if (ncsmds_api(&mds_info) != NCSCC_RC_SUCCESS) {
		LOG_ER("GLND mds send failure: from mds_dest %u evt_type %u",
				     m_NCS_NODE_ID_FROM_MDS_DEST(to_mds_dest), i_evt->type);
		goto end;
	}
	rc = NCSCC_RC_SUCCESS;
end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glnd_mds_msg_send_glnd 
 
  Description   : This routine sends the GLND messages to GLND.
 
  Arguments     : cb  - ptr to the GLND CB
                  i_evt - ptr to the GLND message
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t glnd_mds_msg_send_glnd(GLND_CB *cb, GLSV_GLND_EVT *i_evt, MDS_DEST to_mds_dest)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	if (!i_evt)
		goto end;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->glnd_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_GLND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the send strcuture */
	mds_info.info.svc_send.info.snd.i_to_dest = to_mds_dest;

	if (ncsmds_api(&mds_info) != NCSCC_RC_SUCCESS) {
		LOG_ER("GLND mds send failure:from mds_dest %u evt_type %u",
				     m_NCS_NODE_ID_FROM_MDS_DEST(to_mds_dest), i_evt->type);
		goto end;
	}
	rc = NCSCC_RC_SUCCESS;
end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glnd_mds_msg_send_gld 
 
  Description   : This routine sends the GLND messages to GLD.
 
  Arguments     : cb  - ptr to the GLND CB
                  i_evt - ptr to the GLD message
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t glnd_mds_msg_send_gld(GLND_CB *cb, GLSV_GLD_EVT *i_evt, MDS_DEST to_mds_dest)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	if (!i_evt)
		goto end;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->glnd_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_GLND;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLD;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the send structure */
	if (cb->gld_card_up == true) {
		mds_info.info.svc_send.info.snd.i_to_dest = to_mds_dest;
		/* send the message */
		if (ncsmds_api(&mds_info) != NCSCC_RC_SUCCESS) {
			LOG_ER("GLND mds send failure: from mds_dest %u evt_evt_type %u",
					     m_NCS_NODE_ID_FROM_MDS_DEST(to_mds_dest), i_evt->evt_type);
			goto end;
		}
	} else {

		if (i_evt->evt_type == GLSV_GLD_EVT_RSC_CLOSE) {
			mds_info.info.svc_send.info.snd.i_to_dest = to_mds_dest;
			/* send the message */
			if (ncsmds_api(&mds_info) != NCSCC_RC_SUCCESS) {
				LOG_ER("GLND mds send failure: from mds_dest %u evt_type %d",
						     m_NCS_NODE_ID_FROM_MDS_DEST(to_mds_dest), i_evt->evt_type);
				goto end;
			}

		}
		TRACE_2("GLND mds gld down: from mds_dest %u evt_type %d",
				     m_NCS_NODE_ID_FROM_MDS_DEST(to_mds_dest), i_evt->evt_type);
		goto end;
	}
	rc = NCSCC_RC_SUCCESS;
end:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_evt_local_send

  DESCRIPTION    : Sends GLND event to the mailbox on which it waits. This 
                   routine is used to send events in the same process only.

  ARGUMENTS      : cb       - ptr to the GLND control block
                   evt       - ptr to the GLND event structure
                   priority - event priority

  RETURNS        : 

  NOTES         : None
*****************************************************************************/
uint32_t glnd_evt_local_send(GLND_CB *cb, GLSV_GLND_EVT *evt, uint32_t priority)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	rc = m_GLND_MBX_SEND(&cb->glnd_mbx, evt, priority);

	if (rc != NCSCC_RC_SUCCESS) {
		/* free the event */
		glnd_evt_destroy(evt);
	}
	return rc;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_send_agent_going_down_event

  DESCRIPTION    : Sends GLND event to the mailbox.

  ARGUMENTS      : cb       - ptr to the GLND control block
                   dest     - destination of the GLA that has gone down.

  RETURNS        : 

  NOTES         : None
*****************************************************************************/
static uint32_t glnd_send_agent_going_down_event(GLND_CB *cb, MDS_DEST dest)
{
	GLSV_GLND_EVT *evt = 0;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	evt = m_MMGR_ALLOC_GLND_EVT;
	if (!evt) {
	LOG_CR("GLND evt alloc failed: Error %s", strerror(errno));
		assert(0);
	}
	memset(evt, 0, sizeof(GLSV_GLND_EVT));
	evt->type = GLSV_GLND_EVT_UNREG_AGENT;
	evt->info.agent_info.agent_mds_dest = dest;
	evt->glnd_hdl = cb->cb_hdl_id;

	if ((rc = glnd_evt_local_send(cb, evt, NCS_IPC_PRIORITY_NORMAL)) != NCSCC_RC_SUCCESS) {
		TRACE_2("GLND mds send failure: from mds_dest %u evt_type %d",
				     m_NCS_NODE_ID_FROM_MDS_DEST(cb->glnd_mdest_id), evt->type);
	}
 	
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glsv_dec_reg_unreg_agent_evt

  Description   : This routine decodes reg/unreg evt.

  Arguments     : uba , agent info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_dec_reg_unreg_agent_evt(NCS_UBAID *uba, GLSV_EVT_AGENT_INFO *evt)
{
	uint8_t *p8, local_data[20], size;
	size = 8 + 4;
	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLND dec failed");
		return NCSCC_RC_FAILURE;
	}

	evt->agent_mds_dest = ncs_decode_64bit(&p8);
	evt->process_id = ncs_decode_32bit(&p8);

	ncs_dec_skip_space(uba, size);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : glsv_dec_initialize_evt

  Description   : This routine decodes initialize evt.

  Arguments     : uba , agent info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_dec_initialize_evt(NCS_UBAID *uba, GLSV_EVT_CLIENT_INFO *evt)
{
	uint8_t *p8, local_data[20], size;
	size = 8 + 4 + 4 + (3 * 1);
	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLND dec failed");
		return NCSCC_RC_FAILURE;
	}

	evt->agent_mds_dest = ncs_decode_64bit(&p8);
	evt->client_proc_id = ncs_decode_32bit(&p8);
	evt->cbk_reg_info = ncs_decode_32bit(&p8);
	evt->version.releaseCode = ncs_decode_8bit(&p8);
	evt->version.majorVersion = ncs_decode_8bit(&p8);
	evt->version.minorVersion = ncs_decode_8bit(&p8);

	ncs_dec_skip_space(uba, size);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : glsv_dec_finalize_evt

  Description   : This routine decodes finalize evt.

  Arguments     : uba , agent info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_dec_finalize_evt(NCS_UBAID *uba, GLSV_EVT_FINALIZE_INFO *evt)
{
	uint8_t *p8, local_data[20], size;

	size = 8 + 8;

	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLND dec failed");
		return NCSCC_RC_FAILURE;
	}

	evt->agent_mds_dest = ncs_decode_64bit(&p8);
	evt->handle_id = ncs_decode_64bit(&p8);

	ncs_dec_skip_space(uba, size);
	
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          :glsv_dec_rsc_open_evt

  Description   : This routine decodes open evt.

  Arguments     : uba , agent info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_dec_rsc_open_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt)
{
	uint8_t *p8, local_data[20], size;

	size = (5 * 8) + (5 * 4) + 2;

	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLND dec failed");
		return NCSCC_RC_FAILURE;
	}

	evt->agent_mds_dest = ncs_decode_64bit(&p8);
	evt->client_handle_id = ncs_decode_64bit(&p8);
	evt->lcl_lockid = ncs_decode_64bit(&p8);
	evt->timeout = ncs_decode_64bit(&p8);
	evt->invocation = ncs_decode_64bit(&p8);
	evt->resource_id = ncs_decode_32bit(&p8);
	evt->lcl_resource_id = ncs_decode_32bit(&p8);
	evt->flag = ncs_decode_32bit(&p8);
	evt->call_type = ncs_decode_32bit(&p8);
	evt->lcl_resource_id_count = ncs_decode_32bit(&p8);
	evt->resource_name.length = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, size);

	ncs_decode_n_octets_from_uba(uba, evt->resource_name.value, (uint32_t)evt->resource_name.length);
	
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : glsv_dec_rsc_close_evt

  Description   : This routine decodes close evt.

  Arguments     : uba , agent info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_dec_rsc_close_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt)
{
	uint8_t *p8, local_data[20], size;

	size = (5 * 8) + (5 * 4);

	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLND dec failed");
		return NCSCC_RC_FAILURE;
	}

	evt->agent_mds_dest = ncs_decode_64bit(&p8);
	evt->client_handle_id = ncs_decode_64bit(&p8);
	evt->lcl_lockid = ncs_decode_64bit(&p8);
	evt->timeout = ncs_decode_64bit(&p8);
	evt->invocation = ncs_decode_64bit(&p8);
	evt->resource_id = ncs_decode_32bit(&p8);
	evt->lcl_resource_id = ncs_decode_32bit(&p8);
	evt->flag = ncs_decode_32bit(&p8);
	evt->call_type = ncs_decode_32bit(&p8);
	evt->lcl_resource_id_count = ncs_decode_32bit(&p8);

	ncs_dec_skip_space(uba, size);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          :glsv_dec_rsc_lock_evt

  Description   : This routine decodes lock evt.

  Arguments     : uba , rsc lock info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_dec_rsc_lock_evt(NCS_UBAID *uba, GLSV_EVT_RSC_LOCK_INFO *evt)
{
	uint8_t *p8, local_data[20], size;

	size = (6 * 8) + (6 * 4);

	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLND dec failed");
		return NCSCC_RC_FAILURE;
	}

	evt->agent_mds_dest = ncs_decode_64bit(&p8);
	evt->lcl_lockid = ncs_decode_64bit(&p8);
	evt->client_handle_id = ncs_decode_64bit(&p8);
	evt->waiter_signal = ncs_decode_64bit(&p8);
	evt->invocation = ncs_decode_64bit(&p8);
	evt->timeout = ncs_decode_64bit(&p8);
	evt->resource_id = ncs_decode_32bit(&p8);
	evt->lcl_resource_id = ncs_decode_32bit(&p8);
	evt->lock_type = ncs_decode_32bit(&p8);
	evt->lockFlags = ncs_decode_32bit(&p8);
	evt->call_type = ncs_decode_32bit(&p8);
	evt->status = ncs_decode_32bit(&p8);

	ncs_dec_skip_space(uba, size);
	
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          :glsv_dec_rsc_unlock_evt

  Description   : This routine decodes unlock evt.

  Arguments     : uba , rsc unlock info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_dec_rsc_unlock_evt(NCS_UBAID *uba, GLSV_EVT_RSC_UNLOCK_INFO *evt)
{
	uint8_t *p8, local_data[20], size;
	size = (6 * 8) + (3 * 4);

	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLND dec failed");
		return NCSCC_RC_FAILURE;
	}

	evt->agent_mds_dest = ncs_decode_64bit(&p8);
	evt->client_handle_id = ncs_decode_64bit(&p8);
	evt->lockid = ncs_decode_64bit(&p8);
	evt->lcl_lockid = ncs_decode_64bit(&p8);
	evt->invocation = ncs_decode_64bit(&p8);
	evt->timeout = ncs_decode_64bit(&p8);
	evt->resource_id = ncs_decode_32bit(&p8);
	evt->lcl_resource_id = ncs_decode_32bit(&p8);
	evt->call_type = ncs_decode_32bit(&p8);

	ncs_dec_skip_space(uba, size);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : glsv_dec_rsc_purge_evt

  Description   : This routine decodes purge evt.

  Arguments     : uba , rsc info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_dec_rsc_purge_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt)
{
	uint8_t *p8, local_data[20], size;

	size = (3 * 8) + (2 * 4);
	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLND dec failed");
		return NCSCC_RC_FAILURE;
	}

	evt->agent_mds_dest = ncs_decode_64bit(&p8);
	evt->client_handle_id = ncs_decode_64bit(&p8);
	evt->invocation = ncs_decode_64bit(&p8);
	evt->resource_id = ncs_decode_32bit(&p8);
	evt->call_type = ncs_decode_32bit(&p8);

	ncs_dec_skip_space(uba, size);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : glsv_dec_client_info_evt

  Description   : This routine decodes client evt.

  Arguments     : uba , client info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_dec_client_info_evt(NCS_UBAID *uba, GLSV_EVT_RESTART_CLIENT_INFO *evt)
{
	uint8_t *p8, local_data[20], size;
	size = 33;

	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLND DEC FAILED");
		return NCSCC_RC_FAILURE;
	}

	evt->client_handle_id = ncs_decode_64bit(&p8);
	evt->app_proc_id = ncs_decode_32bit(&p8);
	evt->agent_mds_dest = ncs_decode_64bit(&p8);
	evt->version.releaseCode = ncs_decode_8bit(&p8);
	evt->version.majorVersion = ncs_decode_8bit(&p8);
	evt->version.minorVersion = ncs_decode_8bit(&p8);
	evt->cbk_reg_info = ncs_decode_16bit(&p8);
	evt->no_of_res = ncs_decode_32bit(&p8);
	evt->resource_id = ncs_decode_32bit(&p8);

	ncs_dec_skip_space(uba, size);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : glsv_gla_enc_callbk_evt

  Description   : This routine encodes callback info.

  Arguments     : uba , callback info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_gla_enc_callbk_evt(NCS_UBAID *uba, GLSV_GLA_CALLBACK_INFO *evt)
{
	uint8_t *p8, size;
	size = (2 * 4);
 /** encode the type of message **/
	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLND enc failed");
		return NCSCC_RC_FAILURE;
	}
	ncs_encode_32bit(&p8, evt->callback_type);
	ncs_encode_32bit(&p8, evt->resourceId);

	ncs_enc_claim_space(uba, size);

	switch (evt->callback_type) {
	case GLSV_LOCK_RES_OPEN_CBK:
		size = 8 + (2 * 4);
		p8 = ncs_enc_reserve_space(uba, size);
		if (!p8) {
			TRACE_2("GLND enc failed");
			return NCSCC_RC_FAILURE;
		}

		ncs_encode_64bit(&p8, evt->params.res_open.invocation);
		ncs_encode_32bit(&p8, evt->params.res_open.resourceId);
		ncs_encode_32bit(&p8, evt->params.res_open.error);

		ncs_enc_claim_space(uba, size);
		break;

	case GLSV_LOCK_GRANT_CBK:
		size = (3 * 8) + (4 * 4);
		p8 = ncs_enc_reserve_space(uba, size);
		if (!p8) {
			TRACE_2("GLND enc failed");
			return NCSCC_RC_FAILURE;
		}

		ncs_encode_64bit(&p8, evt->params.lck_grant.invocation);
		ncs_encode_64bit(&p8, evt->params.lck_grant.lockId);
		ncs_encode_64bit(&p8, evt->params.lck_grant.lcl_lockId);
		ncs_encode_32bit(&p8, evt->params.lck_grant.lockMode);
		ncs_encode_32bit(&p8, evt->params.lck_grant.resourceId);
		ncs_encode_32bit(&p8, evt->params.lck_grant.lockStatus);
		ncs_encode_32bit(&p8, evt->params.lck_grant.error);

		ncs_enc_claim_space(uba, size);
		break;

	case GLSV_LOCK_WAITER_CBK:
		size = (3 * 8) + (4 * 4);
		p8 = ncs_enc_reserve_space(uba, size);
		if (!p8) {
			TRACE_2("GLND enc failed");
			return NCSCC_RC_FAILURE;
		}

		ncs_encode_64bit(&p8, evt->params.lck_wait.invocation);
		ncs_encode_64bit(&p8, evt->params.lck_wait.lockId);
		ncs_encode_64bit(&p8, evt->params.lck_wait.lcl_lockId);
		ncs_encode_32bit(&p8, evt->params.lck_wait.wait_signal);
		ncs_encode_32bit(&p8, evt->params.lck_wait.resourceId);
		ncs_encode_32bit(&p8, evt->params.lck_wait.modeHeld);
		ncs_encode_32bit(&p8, evt->params.lck_wait.modeRequested);

		ncs_enc_claim_space(uba, size);
		break;

	case GLSV_LOCK_UNLOCK_CBK:
		size = (2 * 8) + (3 * 4);
		p8 = ncs_enc_reserve_space(uba, size);
		if (!p8) {
			TRACE_2("GLND enc failed");
			return NCSCC_RC_FAILURE;
		}

		ncs_encode_64bit(&p8, evt->params.unlock.invocation);
		ncs_encode_64bit(&p8, evt->params.unlock.lockId);
		ncs_encode_32bit(&p8, evt->params.unlock.resourceId);
		ncs_encode_32bit(&p8, evt->params.unlock.lockStatus);
		ncs_encode_32bit(&p8, evt->params.unlock.error);

		ncs_enc_claim_space(uba, size);
		break;

	default:
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : glsv_gla_enc_api_resp_evt

  Description   : This routine encodes api response info.

  Arguments     : uba , api response info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_gla_enc_api_resp_evt(NCS_UBAID *uba, GLSV_GLA_API_RESP_INFO *evt)
{
	uint8_t *p8, size;

	size = (2 * 4);
 /** encode the type of message **/
	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLND enc failed");
		return NCSCC_RC_FAILURE;
	}
	ncs_encode_32bit(&p8, evt->prc_id);
	ncs_encode_32bit(&p8, evt->type);

	ncs_enc_claim_space(uba, size);

	switch (evt->type) {
	case GLSV_GLA_LOCK_RES_OPEN:
		size = 4;
		p8 = ncs_enc_reserve_space(uba, size);
		if (!p8) {
			TRACE_2("GLND enc failed");
			return NCSCC_RC_FAILURE;
		}

		ncs_encode_32bit(&p8, evt->param.res_open.resourceId);

		ncs_enc_claim_space(uba, size);
		break;

	case GLSV_GLA_LOCK_SYNC_LOCK:
		size = 8 + 4;
		p8 = ncs_enc_reserve_space(uba, size);
		if (!p8) {
			TRACE_2("GLND enc failed");
			return NCSCC_RC_FAILURE;
		}

		ncs_encode_64bit(&p8, evt->param.sync_lock.lockId);
		ncs_encode_32bit(&p8, evt->param.sync_lock.lockStatus);

		ncs_enc_claim_space(uba, size);
		break;
	default:
		break;
	}
	
	return NCSCC_RC_SUCCESS;
}
