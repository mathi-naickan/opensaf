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

  This file contains routines used by GLA library for MDS interaction.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "gla.h"

uint32_t gla_mds_callback(struct ncsmds_callback_info *info);
static uint32_t gla_mds_enc_flat(GLA_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uint32_t gla_mds_enc(GLA_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uint32_t gla_mds_dec_flat(GLA_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uint32_t gla_mds_dec(GLA_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uint32_t gla_mds_rcv(GLA_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info);
static uint32_t gla_mds_svc_evt(GLA_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt);
static uint32_t glsv_enc_client_info_evt(NCS_UBAID *uba, GLSV_EVT_RESTART_CLIENT_INFO *evt);
static uint32_t glsv_enc_rsc_purge_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt);
static uint32_t glsv_enc_rsc_unlock_evt(NCS_UBAID *uba, GLSV_EVT_RSC_UNLOCK_INFO *evt);
static uint32_t glsv_enc_rsc_lock_evt(NCS_UBAID *uba, GLSV_EVT_RSC_LOCK_INFO *evt);
static uint32_t glsv_enc_rsc_close_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt);
static uint32_t glsv_enc_rsc_open_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt);
static uint32_t glsv_enc_finalize_evt(NCS_UBAID *uba, GLSV_EVT_FINALIZE_INFO *evt);
static uint32_t glsv_enc_initialize_evt(NCS_UBAID *uba, GLSV_EVT_CLIENT_INFO *evt);
static uint32_t glsv_enc_reg_unreg_agent_evt(NCS_UBAID *uba, GLSV_EVT_AGENT_INFO *evt);
static uint32_t glsv_gla_dec_callbk_evt(NCS_UBAID *uba, GLSV_GLA_CALLBACK_INFO *evt);
static uint32_t glsv_gla_dec_api_resp_evt(NCS_UBAID *uba, GLSV_GLA_API_RESP_INFO *evt);

uint32_t gla_mds_get_handle(GLA_CB *cb);

MSG_FRMT_VER gla_glnd_msg_fmt_table[GLA_WRT_GLND_SUBPART_VER_RANGE] = { 1 };

/****************************************************************************
 * Name          : gla_mds_get_handle
 *
 * Description   : This function Gets the Handles of local MDS destination
 *
 * Arguments     : cb   : GLA control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t gla_mds_get_handle(GLA_CB *cb)
{
	NCSADA_INFO arg;
	uint32_t rc;
	TRACE_ENTER();

	memset(&arg, 0, sizeof(NCSADA_INFO));
	arg.req = NCSADA_GET_HDLS;
	rc = ncsada_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("GLA mds get handle failed");
		goto end;
	}
	cb->gla_mds_hdl = arg.info.adest_get_hdls.o_mds_pwe1_hdl;
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : gla_mds_register
 
  Description   : This routine registers the GLA Service with MDS.
 
  Arguments     : gla_cb - ptr to the GLA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uint32_t gla_mds_register(GLA_CB *cb)
{
	NCSMDS_INFO svc_info;
	MDS_SVC_ID svc_id = NCSMDS_SVC_ID_GLND;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* STEP1: Get the MDS Handle */
	gla_mds_get_handle(cb);

	/* memset the svc_info */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));

	/* STEP 2 : Install on ADEST with MDS with service ID NCSMDS_SVC_ID_GLA. */
	svc_info.i_mds_hdl = cb->gla_mds_hdl;
	svc_info.i_svc_id = NCSMDS_SVC_ID_GLA;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = cb->agent_handle_id;
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_INTRANODE;	/* node specific */
	svc_info.info.svc_install.i_svc_cb = gla_mds_callback;	/* callback */
	svc_info.info.svc_install.i_mds_q_ownership = false;	/* GLA owns the mds queue */
	svc_info.info.svc_install.i_mds_svc_pvt_ver = GLA_PVT_SUBPART_VERSION;	/* Private Subpart Version of GLA */

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		rc = NCSCC_RC_FAILURE;
		goto end;
	}
	cb->gla_mds_dest = svc_info.info.svc_install.o_dest;

	/* STEP 3: Subscribe to GLND up/down events */
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_INTRANODE;
	svc_info.info.svc_subscribe.i_svc_ids = &svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		/* Uninstall with the mds */
		svc_info.i_op = MDS_UNINSTALL;
		if (ncsmds_api(&svc_info) != NCSCC_RC_SUCCESS) {
			TRACE_2("GLA mds register failed");
		}
		rc = NCSCC_RC_FAILURE;
		goto end;
	}

 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : gla_mds_unreg
 *
 * Description   : This function un-registers the GLA Service with MDS.
 *
 * Arguments     : cb   : GLA control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void gla_mds_unregister(GLA_CB *cb)
{
	NCSMDS_INFO arg;
	TRACE_ENTER();
	
	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->gla_mds_hdl;
	arg.i_svc_id = NCSMDS_SVC_ID_GLA;
	arg.i_op = MDS_UNINSTALL;

	if (ncsmds_api(&arg) != NCSCC_RC_SUCCESS) {
		TRACE_2("GLA mds register failed");
	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : gla_mds_callback
 
  Description   : This callback routine will be called by MDS on event arrival
 
  Arguments     : info - pointer to the mds callback info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t gla_mds_callback(struct ncsmds_callback_info *info)
{
	GLA_CB *gla_cb = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (info == NULL)
		goto end;

	gla_cb = (GLA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLA, (uint32_t)info->i_yr_svc_hdl);
	if (!gla_cb) {
		TRACE_2("GLA cb retrieval failed");
		goto end;
	}

	switch (info->i_op) {
	case MDS_CALLBACK_COPY:
		rc = NCSCC_RC_FAILURE;
		break;
	case MDS_CALLBACK_ENC:
		rc = gla_mds_enc(gla_cb, &info->info.enc);
		break;
	case MDS_CALLBACK_DEC:
		rc = gla_mds_dec(gla_cb, &info->info.dec);
		break;
	case MDS_CALLBACK_ENC_FLAT:
		rc = gla_mds_enc_flat(gla_cb, &info->info.enc_flat);
		break;
	case MDS_CALLBACK_DEC_FLAT:
		rc = gla_mds_dec_flat(gla_cb, &info->info.dec_flat);
		break;
	case MDS_CALLBACK_RECEIVE:
		rc = gla_mds_rcv(gla_cb, &info->info.receive);
		break;

	case MDS_CALLBACK_SVC_EVENT:
		rc = gla_mds_svc_evt(gla_cb, &info->info.svc_evt);
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("GLA mds callback call failure");
	}
	ncshm_give_hdl((uint32_t)info->i_yr_svc_hdl);
 
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : gla_mds_enc_flat
 
  Description   : This function encodes an events sent from GLA.
 
  Arguments     : cb    : GLA control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t gla_mds_enc_flat(GLA_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info)
{
	GLSV_GLND_EVT *evt;
	NCS_UBAID *uba = info->io_uba;
	uint32_t size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	if (info->i_to_svc_id == NCSMDS_SVC_ID_GLND) {
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    GLA_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							    GLA_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							    gla_glnd_msg_fmt_table);
	}

	if (info->i_to_svc_id == NCSMDS_SVC_ID_GLND && info->o_msg_fmt_ver) {
		/* as all the event structures are flat */
		evt = (GLSV_GLND_EVT *)info->i_msg;
		size = sizeof(GLSV_GLND_EVT);
		ncs_encode_n_octets_in_uba(uba, (uint8_t *)evt, size);
		rc = NCSCC_RC_SUCCESS;
	} else {
		/* Drop The Message */
		TRACE_2("GLA message format version invalid");
	}
	
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : gla_mds_enc

  Description   : This function encodes an events sent from GLA.

  Arguments     : cb    : GLA control Block.
                  info  : Info for encoding

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t gla_mds_enc(GLA_CB *cb, MDS_CALLBACK_ENC_INFO *info)
{
	GLSV_GLND_EVT *evt;
	NCS_UBAID *uba = info->io_uba;
	uint8_t *p8;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (uba == NULL) {
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}

	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	if (info->i_to_svc_id == NCSMDS_SVC_ID_GLND) {
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    GLA_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							    GLA_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							    gla_glnd_msg_fmt_table);
	}

	if (info->i_to_svc_id == NCSMDS_SVC_ID_GLND && info->o_msg_fmt_ver) {
		/* as all the event structures are flat */
		evt = (GLSV_GLND_EVT *)info->i_msg;
    /** encode the type of message **/
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE_2("GLA mds enc flat failure");
			goto end;
		}
		ncs_encode_32bit(&p8, evt->type);
		ncs_enc_claim_space(uba, 4);
		switch (evt->type) {
		case GLSV_GLND_EVT_REG_AGENT:
			glsv_enc_reg_unreg_agent_evt(uba, &evt->info.agent_info);
			break;

		case GLSV_GLND_EVT_UNREG_AGENT:
			glsv_enc_reg_unreg_agent_evt(uba, &evt->info.agent_info);
			break;

		case GLSV_GLND_EVT_INITIALIZE:
			glsv_enc_initialize_evt(uba, &evt->info.client_info);
			break;

		case GLSV_GLND_EVT_FINALIZE:
			glsv_enc_finalize_evt(uba, &evt->info.finalize_info);
			break;

		case GLSV_GLND_EVT_RSC_OPEN:
			glsv_enc_rsc_open_evt(uba, &evt->info.rsc_info);
			break;

		case GLSV_GLND_EVT_RSC_CLOSE:
			glsv_enc_rsc_close_evt(uba, &evt->info.rsc_info);
			break;

		case GLSV_GLND_EVT_RSC_LOCK:
			glsv_enc_rsc_lock_evt(uba, &evt->info.rsc_lock_info);
			break;

		case GLSV_GLND_EVT_RSC_UNLOCK:
			glsv_enc_rsc_unlock_evt(uba, &evt->info.rsc_unlock_info);
			break;

		case GLSV_GLND_EVT_RSC_PURGE:
			glsv_enc_rsc_purge_evt(uba, &evt->info.rsc_info);
			break;

		case GLSV_GLND_EVT_CLIENT_INFO:
			glsv_enc_client_info_evt(uba, &evt->info.restart_client_info);
			break;

		default:
			goto end;
		}
		rc = NCSCC_RC_SUCCESS;
		goto end;
	} else {
		/* Drop The Message */
		TRACE_2("GLA message format version invalid");
		goto end;
	}
 end:	
	TRACE_LEAVE();	
	return rc;
}

/****************************************************************************
  Name          : gla_mds_dec_flat
 
  Description   : This function decodes an events sent to GLA.
 
  Arguments     : cb    : GLA control Block.
                  info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t gla_mds_dec_flat(GLA_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info)
{

	GLSV_GLA_EVT *evt;
	NCS_UBAID *uba = info->io_uba;
	bool is_valid_msg_fmt = false;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (info->i_fr_svc_id == NCSMDS_SVC_ID_GLND) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     GLA_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							     GLA_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							     gla_glnd_msg_fmt_table);
	}

	if (is_valid_msg_fmt && (info->i_fr_svc_id == NCSMDS_SVC_ID_GLND)) {
		evt = (GLSV_GLA_EVT *)m_MMGR_ALLOC_GLA_EVT;
		if (evt == NULL) {
			TRACE_4("GLA evt alloc failed");
			goto end;
		}
		info->o_msg = evt;
		uba->ub = ncs_decode_n_octets(uba->ub, (uint8_t *)evt, sizeof(GLSV_GLA_EVT));
		rc = NCSCC_RC_SUCCESS;
		goto end;
	} else {
		if (!is_valid_msg_fmt) {
			TRACE_2("GLA message format version invalid");
		}
		TRACE_2("GLA mds dec flat failure");
		goto end;
	}
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : gla_mds_dect
 
  Description   : This function decodes an events sent to GLA.
 
  Arguments     : cb    : GLA control Block.
                  info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t gla_mds_dec(GLA_CB *cb, MDS_CALLBACK_DEC_INFO *info)
{

	GLSV_GLA_EVT *evt;
	NCS_UBAID *uba = info->io_uba;
	bool is_valid_msg_fmt = false;
	uint8_t *p8, local_data[20];
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	
	if (info->i_fr_svc_id == NCSMDS_SVC_ID_GLND) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     GLA_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT,
							     GLA_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT,
							     gla_glnd_msg_fmt_table);
	}

	if (is_valid_msg_fmt && (info->i_fr_svc_id == NCSMDS_SVC_ID_GLND)) {
		evt = (GLSV_GLA_EVT *)m_MMGR_ALLOC_GLA_EVT;
		if (evt == NULL) {
			TRACE_4("GLA evt alloc failed");
			goto end;
		}
		memset(evt, 0, sizeof(GLSV_GLA_EVT));
		info->o_msg = (NCSCONTEXT)evt;

		p8 = ncs_dec_flatten_space(uba, local_data, 16);
		evt->handle = ncs_decode_64bit(&p8);
		evt->error = ncs_decode_32bit(&p8);
		evt->type = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 16);

		switch (evt->type) {
		case GLSV_GLA_CALLBK_EVT:
			glsv_gla_dec_callbk_evt(uba, &evt->info.gla_clbk_info);
			break;

		case GLSV_GLA_API_RESP_EVT:
			glsv_gla_dec_api_resp_evt(uba, &evt->info.gla_resp_info);
			break;

		default:
			goto end;
		}
		rc = NCSCC_RC_SUCCESS;
		goto end;
	} else {
		if (!is_valid_msg_fmt) {
			 TRACE_2("GLA message format version invalid");
		}
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}
 end:
	TRACE_LEAVE();	
	return rc;
}

/****************************************************************************
 * Name          : gla_mds_rcv
 *
 * Description   : MDS will call this function on receiving GLND messages.
 *
 * Arguments     : cb - GLA Control Block
 *                 rcv_info - MDS Receive information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t gla_mds_rcv(GLA_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	GLSV_GLA_EVT *evt = (GLSV_GLA_EVT *)rcv_info->i_msg;
	GLSV_GLA_CALLBACK_INFO *gla_callbk_info;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();	

	if (evt == NULL)
		goto end;
	/* check the event type */
	if (evt->type == GLSV_GLA_CALLBK_EVT) {
		/*allocate the memory */
		gla_callbk_info = m_MMGR_ALLOC_GLA_CALLBACK_INFO;
		memcpy(gla_callbk_info, &evt->info.gla_clbk_info, sizeof(GLSV_GLA_CALLBACK_INFO));
		/* Stop & Destroy the timer */
		switch (gla_callbk_info->callback_type) {
		case GLSV_LOCK_RES_OPEN_CBK:
			{
				GLA_RESOURCE_ID_INFO *res_id_node = NULL;
				res_id_node =
				    (GLA_RESOURCE_ID_INFO *)ncshm_take_hdl(NCS_SERVICE_ID_GLA,
									   gla_callbk_info->resourceId);
				if (res_id_node) {
					gla_stop_tmr(&res_id_node->res_async_tmr);
					ncshm_give_hdl(gla_callbk_info->resourceId);
				}
			}
			break;
		case GLSV_LOCK_GRANT_CBK:
			{
				GLSV_LOCK_GRANT_PARAM *param = &gla_callbk_info->params.lck_grant;
				GLA_LOCK_ID_INFO *lock_id_node = NULL;
				lock_id_node =
				    (GLA_LOCK_ID_INFO *)ncshm_take_hdl(NCS_SERVICE_ID_GLA, param->lcl_lockId);

				if (lock_id_node) {
					gla_stop_tmr(&lock_id_node->lock_async_tmr);
					ncshm_give_hdl(param->lcl_lockId);
				}
			}
			break;
		case GLSV_LOCK_UNLOCK_CBK:
			{
				GLSV_LOCK_UNLOCK_PARAM *param = &gla_callbk_info->params.unlock;
				GLA_LOCK_ID_INFO *lock_id_node = NULL;
				lock_id_node = (GLA_LOCK_ID_INFO *)ncshm_take_hdl(NCS_SERVICE_ID_GLA, param->lockId);

				if (lock_id_node) {
					gla_stop_tmr(&lock_id_node->unlock_async_tmr);
					ncshm_give_hdl(param->lockId);
				}
			}
			break;
		case GLSV_LOCK_WAITER_CBK:
			break;
		}

		/* Put it in place it in the Queue */
		rc = glsv_gla_callback_queue_write(cb, evt->handle, gla_callbk_info);

		if (gla_callbk_info->callback_type == GLSV_LOCK_RES_OPEN_CBK) {
			GLSV_LOCK_RES_OPEN_PARAM *param = &gla_callbk_info->params.res_open;
			GLA_CLIENT_INFO *client_info;
			GLA_RESOURCE_ID_INFO *res_id_node;

			/* get the client_info */
			client_info = gla_client_tree_find_and_add(cb, evt->handle, false);
			if (client_info) {
				if (client_info->lckCallbk.saLckResourceOpenCallback) {
					if (param->error == SA_AIS_OK) {
						/* add the resource id to the local tree */
						if ((res_id_node =
						     (GLA_RESOURCE_ID_INFO *)ncshm_take_hdl(NCS_SERVICE_ID_GLA,
											    gla_callbk_info->
											    resourceId))) {
							res_id_node->gbl_res_id = param->resourceId;
							res_id_node->lock_handle_id = client_info->lock_handle_id;
							ncshm_give_hdl(gla_callbk_info->resourceId);

						}
					}
				}
			}
		}
		m_MMGR_FREE_GLA_EVT(evt);
		if (rc == NCSCC_RC_FAILURE) {
			m_MMGR_FREE_GLA_CALLBACK_INFO(gla_callbk_info);
		}
		goto end;
	} else {
		if (evt)
			m_MMGR_FREE_GLA_EVT(evt);
		goto end;
	}
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : gla_mds_svc_evt
 *
 * Description   : GLA is informed when MDS events occurr that he has 
 *                 subscribed to
 *
 * Arguments     : 
 *   cb          : GLA control Block.
 *   enc_info    : Svc evt info.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t gla_mds_svc_evt(GLA_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{
	
	switch (svc_evt->i_change) {
	case NCSMDS_DOWN:
		if (svc_evt->i_svc_id == NCSMDS_SVC_ID_GLND) {
			if (cb->glnd_svc_up != false) {
				memset(&cb->glnd_mds_dest, 0, sizeof(MDS_DEST));
				/* clean up the client tree */
				m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				TRACE_2("GLA recieved glnd service down");
			}
			cb->glnd_svc_up = false;
			cb->glnd_crashed = true;
		}
		break;
	case NCSMDS_UP:
		switch (svc_evt->i_svc_id) {
		case NCSMDS_SVC_ID_GLND:
			cb->glnd_mds_dest = svc_evt->i_dest;
			if (cb->glnd_svc_up == false) {
				cb->glnd_svc_up = true;
				/* send the resigteration information to the GLND */
				TRACE_1("GLA recieved glnd service up");
				if (gla_agent_register(cb) != NCSCC_RC_SUCCESS) {
					TRACE_2("GLA agent register failure");
				}

				if (cb->glnd_crashed) {
					if (gla_client_info_send(cb) == NCSCC_RC_SUCCESS) {
						/*TBD LOG */

					}

				}
			}
			m_NCS_LOCK(&cb->glnd_sync_lock, NCS_LOCK_WRITE);

			if (cb->glnd_sync_awaited == true) {
				m_NCS_SEL_OBJ_IND(&cb->glnd_sync_sel);
			}

			m_NCS_UNLOCK(&cb->glnd_sync_lock, NCS_LOCK_WRITE);
			break;

		default:
			break;

		}

	default:
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : gla_mds_msg_sync_send
 
  Description   : This routine sends the GLA message to GLND.
 
  Arguments     : cb  - ptr to the GLA CB
                  i_evt - ptr to the GLA message
                  o_evt - ptr to the GLA message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t gla_mds_msg_sync_send(GLA_CB *cb, GLSV_GLND_EVT *i_evt, GLSV_GLA_EVT **o_evt, uint32_t timeout)
{
	
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (!i_evt || cb->glnd_svc_up == false)
		goto end;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->gla_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_GLA;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;

	/* fill the send rsp strcuture */
	mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout;	/* timeto wait in 10ms */
	mds_info.info.svc_send.info.sndrsp.i_to_dest = cb->glnd_mds_dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc == NCSCC_RC_SUCCESS)
		*o_evt = mds_info.info.svc_send.info.sndrsp.o_rsp;
	
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : gla_mds_msg_async_send
 
  Description   : This routine sends the GLA message to GLND.
 
  Arguments     : cb  - ptr to the GLA CB
                  i_evt - ptr to the GLA message
                  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t gla_mds_msg_async_send(GLA_CB *cb, GLSV_GLND_EVT *i_evt)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (!i_evt || cb->glnd_svc_up == false)
		goto end;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->gla_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_GLA;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the send rsp strcuture */
	mds_info.info.svc_send.info.snd.i_to_dest = cb->glnd_mds_dest;
		
	/* send the message */
	rc = ncsmds_api(&mds_info);
 end:
	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
  Name          : gla_agent_register
 
  Description   : This routine sends the GLA registeration message to GLND.
 
  Arguments     : cb  - ptr to the GLA CB
                  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t gla_agent_register(GLA_CB *cb)
{
	GLSV_GLND_EVT evt;
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();	

	if (cb->glnd_svc_up == false)
		goto end;

	memset(&evt, 0, sizeof(GLSV_GLND_EVT));
	evt.type = GLSV_GLND_EVT_REG_AGENT;
	evt.info.agent_info.agent_mds_dest = cb->gla_mds_dest;
	evt.info.agent_info.process_id = cb->process_id;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->gla_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_GLA;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)&evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the send rsp strcuture */
	mds_info.info.svc_send.info.snd.i_to_dest = cb->glnd_mds_dest;
	
	/* send the message */
	rc = ncsmds_api(&mds_info);
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : gla_agent_unregister
 
  Description   : This routine sends the GLA unregisteration message to GLND.
 
  Arguments     : cb  - ptr to the GLA CB
                  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t gla_agent_unregister(GLA_CB *cb)
{
	GLSV_GLND_EVT evt;
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	if (cb->glnd_svc_up == false)
		goto end;

	memset(&evt, 0, sizeof(GLSV_GLND_EVT));
	evt.type = GLSV_GLND_EVT_UNREG_AGENT;
	evt.info.agent_info.agent_mds_dest = cb->gla_mds_dest;
	evt.info.agent_info.process_id = cb->process_id;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->gla_mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_GLA;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)&evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_GLND;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the send rsp strcuture */
	mds_info.info.svc_send.info.snd.i_to_dest = cb->glnd_mds_dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);

 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glsv_enc_reg_unreg_agent_evt 

  Description   : This routine encodes reg/unreg evt.

  Arguments     : uba , agent info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_enc_reg_unreg_agent_evt(NCS_UBAID *uba, GLSV_EVT_AGENT_INFO *evt)
{
	uint8_t *p8, size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	size = 8 + 4;

	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}

	ncs_encode_64bit(&p8, evt->agent_mds_dest);
	ncs_encode_32bit(&p8, evt->process_id);

	ncs_enc_claim_space(uba, size);
	rc = NCSCC_RC_SUCCESS;

 end:	
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glsv_enc_initialize_evt

  Description   : This routine encodes initialize evt.

  Arguments     : uba , agent info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_enc_initialize_evt(NCS_UBAID *uba, GLSV_EVT_CLIENT_INFO *evt)
{
	uint8_t *p8, size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	size = 8 + 4 + 4 + (3 * 1);

	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}

	ncs_encode_64bit(&p8, evt->agent_mds_dest);
	ncs_encode_32bit(&p8, evt->client_proc_id);
	ncs_encode_32bit(&p8, evt->cbk_reg_info);
	ncs_encode_8bit(&p8, evt->version.releaseCode);
	ncs_encode_8bit(&p8, evt->version.majorVersion);
	ncs_encode_8bit(&p8, evt->version.minorVersion);

	ncs_enc_claim_space(uba, size);
	rc = NCSCC_RC_SUCCESS;
 
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glsv_enc_finalize_evt

  Description   : This routine encodes finalize evt.

  Arguments     : uba , agent info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_enc_finalize_evt(NCS_UBAID *uba, GLSV_EVT_FINALIZE_INFO *evt)
{
	uint8_t *p8, size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	size = 8 + 8;

	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}

	ncs_encode_64bit(&p8, evt->agent_mds_dest);
	ncs_encode_64bit(&p8, evt->handle_id);

	ncs_enc_claim_space(uba, size);
	rc = NCSCC_RC_SUCCESS;
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :glsv_enc_rsc_open_evt 

  Description   : This routine encodes open evt.

  Arguments     : uba , agent info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_enc_rsc_open_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt)
{
	uint8_t *p8, size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	size = (5 * 8) + (5 * 4) + 2;

	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}

	ncs_encode_64bit(&p8, evt->agent_mds_dest);
	ncs_encode_64bit(&p8, evt->client_handle_id);
	ncs_encode_64bit(&p8, evt->lcl_lockid);
	ncs_encode_64bit(&p8, evt->timeout);
	ncs_encode_64bit(&p8, evt->invocation);
	ncs_encode_32bit(&p8, evt->resource_id);
	ncs_encode_32bit(&p8, evt->lcl_resource_id);
	ncs_encode_32bit(&p8, evt->flag);
	ncs_encode_32bit(&p8, evt->call_type);
	ncs_encode_32bit(&p8, evt->lcl_resource_id_count);
	ncs_encode_16bit(&p8, evt->resource_name.length);
	ncs_enc_claim_space(uba, size);

	ncs_encode_n_octets_in_uba(uba, evt->resource_name.value, (uint32_t)evt->resource_name.length);
	rc = NCSCC_RC_SUCCESS;
 end:	
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glsv_enc_rsc_close_evt

  Description   : This routine encodes close evt.

  Arguments     : uba , agent info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_enc_rsc_close_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt)
{
	uint8_t *p8, size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();
	size = (5 * 8) + (5 * 4);

	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}

	ncs_encode_64bit(&p8, evt->agent_mds_dest);
	ncs_encode_64bit(&p8, evt->client_handle_id);
	ncs_encode_64bit(&p8, evt->lcl_lockid);
	ncs_encode_64bit(&p8, evt->timeout);
	ncs_encode_64bit(&p8, evt->invocation);
	ncs_encode_32bit(&p8, evt->resource_id);
	ncs_encode_32bit(&p8, evt->lcl_resource_id);
	ncs_encode_32bit(&p8, evt->flag);
	ncs_encode_32bit(&p8, evt->call_type);
	ncs_encode_32bit(&p8, evt->lcl_resource_id_count);

	ncs_enc_claim_space(uba, size);
	rc = NCSCC_RC_SUCCESS;
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :glsv_enc_rsc_lock_evt 

  Description   : This routine encodes lock evt.

  Arguments     : uba , rsc lock info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_enc_rsc_lock_evt(NCS_UBAID *uba, GLSV_EVT_RSC_LOCK_INFO *evt)
{
	uint8_t *p8, size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	size = (6 * 8) + (6 * 4);

	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}

	ncs_encode_64bit(&p8, evt->agent_mds_dest);
	ncs_encode_64bit(&p8, evt->lcl_lockid);
	ncs_encode_64bit(&p8, evt->client_handle_id);
	ncs_encode_64bit(&p8, evt->waiter_signal);
	ncs_encode_64bit(&p8, evt->invocation);
	ncs_encode_64bit(&p8, evt->timeout);
	ncs_encode_32bit(&p8, evt->resource_id);
	ncs_encode_32bit(&p8, evt->lcl_resource_id);
	ncs_encode_32bit(&p8, evt->lock_type);
	ncs_encode_32bit(&p8, evt->lockFlags);
	ncs_encode_32bit(&p8, evt->call_type);
	ncs_encode_32bit(&p8, evt->status);

	ncs_enc_claim_space(uba, size);
	rc = NCSCC_RC_SUCCESS;
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :glsv_enc_rsc_unlock_evt

  Description   : This routine encodes unlock evt.

  Arguments     : uba , rsc unlock info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_enc_rsc_unlock_evt(NCS_UBAID *uba, GLSV_EVT_RSC_UNLOCK_INFO *evt)
{
	uint8_t *p8, size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	size = (6 * 8) + (3 * 4);

	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}

	ncs_encode_64bit(&p8, evt->agent_mds_dest);
	ncs_encode_64bit(&p8, evt->client_handle_id);
	ncs_encode_64bit(&p8, evt->lockid);
	ncs_encode_64bit(&p8, evt->lcl_lockid);
	ncs_encode_64bit(&p8, evt->invocation);
	ncs_encode_64bit(&p8, evt->timeout);
	ncs_encode_32bit(&p8, evt->resource_id);
	ncs_encode_32bit(&p8, evt->lcl_resource_id);
	ncs_encode_32bit(&p8, evt->call_type);

	ncs_enc_claim_space(uba, size);
	rc = NCSCC_RC_SUCCESS;
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glsv_enc_rsc_purge_evt 

  Description   : This routine encodes purge evt.

  Arguments     : uba , rsc info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_enc_rsc_purge_evt(NCS_UBAID *uba, GLSV_EVT_RSC_INFO *evt)
{
	uint8_t *p8, size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	size = (3 * 8) + (2 * 4);

	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}

	ncs_encode_64bit(&p8, evt->agent_mds_dest);
	ncs_encode_64bit(&p8, evt->client_handle_id);
	ncs_encode_64bit(&p8, evt->invocation);
	ncs_encode_32bit(&p8, evt->resource_id);
	ncs_encode_32bit(&p8, evt->call_type);

	ncs_enc_claim_space(uba, size);
	rc = NCSCC_RC_SUCCESS;

 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glsv_enc_client_info_evt 

  Description   : This routine encodes client evt.

  Arguments     : uba , client info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_enc_client_info_evt(NCS_UBAID *uba, GLSV_EVT_RESTART_CLIENT_INFO *evt)
{
	uint8_t *p8, size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

	size = (2 * 8) + 4 + 2;

	p8 = ncs_enc_reserve_space(uba, size);
	if (!p8) {
		TRACE_2("GLA mds enc flat failure");
		goto end;
	}

	ncs_encode_64bit(&p8, evt->agent_mds_dest);
	ncs_encode_64bit(&p8, evt->client_handle_id);
	ncs_encode_32bit(&p8, evt->app_proc_id);
	ncs_encode_16bit(&p8, evt->cbk_reg_info);

	ncs_enc_claim_space(uba, size);
	rc = NCSCC_RC_SUCCESS;
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glsv_gla_dec_callbk_evt

  Description   : This routine decodes callback info.

  Arguments     : uba , callback evt.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_gla_dec_callbk_evt(NCS_UBAID *uba, GLSV_GLA_CALLBACK_INFO *evt)
{
	uint8_t *p8, local_data[20], size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();	

	size = (2 * 4);

	/*decode the type of message */
	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLA mds dec failure");
		goto end;
	}
	evt->callback_type = ncs_decode_32bit(&p8);
	evt->resourceId = ncs_decode_32bit(&p8);

	ncs_dec_skip_space(uba, size);

	switch (evt->callback_type) {
	case GLSV_LOCK_RES_OPEN_CBK:
		size = 8 + (2 * 4);
		p8 = ncs_dec_flatten_space(uba, local_data, size);
		if (!p8) {
			TRACE_2("GLA mds dec failure");
			goto end;
		}

		evt->params.res_open.invocation = ncs_decode_64bit(&p8);
		evt->params.res_open.resourceId = ncs_decode_32bit(&p8);
		evt->params.res_open.error = ncs_decode_32bit(&p8);

		ncs_dec_skip_space(uba, size);
		break;

	case GLSV_LOCK_GRANT_CBK:
		size = (3 * 8) + (4 * 4);
		p8 = ncs_dec_flatten_space(uba, local_data, size);
		if (!p8) {
			TRACE_2("GLA mds dec failure");
			goto end;
		}

		evt->params.lck_grant.invocation = ncs_decode_64bit(&p8);
		evt->params.lck_grant.lockId = ncs_decode_64bit(&p8);
		evt->params.lck_grant.lcl_lockId = ncs_decode_64bit(&p8);
		evt->params.lck_grant.lockMode = ncs_decode_32bit(&p8);
		evt->params.lck_grant.resourceId = ncs_decode_32bit(&p8);
		evt->params.lck_grant.lockStatus = ncs_decode_32bit(&p8);
		evt->params.lck_grant.error = ncs_decode_32bit(&p8);

		ncs_dec_skip_space(uba, size);
		break;

	case GLSV_LOCK_WAITER_CBK:
		size = (3 * 8) + (4 * 4);
		p8 = ncs_dec_flatten_space(uba, local_data, size);
		if (!p8) {
			TRACE_2("GLA mds dec failure");
			goto end;
		}

		evt->params.lck_wait.invocation = ncs_decode_64bit(&p8);
		evt->params.lck_wait.lockId = ncs_decode_64bit(&p8);
		evt->params.lck_wait.lcl_lockId = ncs_decode_64bit(&p8);
		evt->params.lck_wait.wait_signal = ncs_decode_32bit(&p8);
		evt->params.lck_wait.resourceId = ncs_decode_32bit(&p8);
		evt->params.lck_wait.modeHeld = ncs_decode_32bit(&p8);
		evt->params.lck_wait.modeRequested = ncs_decode_32bit(&p8);

		ncs_dec_skip_space(uba, size);
		break;
	case GLSV_LOCK_UNLOCK_CBK:
		size = (2 * 8) + (3 * 4);
		p8 = ncs_dec_flatten_space(uba, local_data, size);
		if (!p8) {
			TRACE_2("GLA mds dec failure");
			goto end;
		}

		evt->params.unlock.invocation = ncs_decode_64bit(&p8);
		evt->params.unlock.lockId = ncs_decode_64bit(&p8);
		evt->params.unlock.resourceId = ncs_decode_32bit(&p8);
		evt->params.unlock.lockStatus = ncs_decode_32bit(&p8);
		evt->params.unlock.error = ncs_decode_32bit(&p8);

		ncs_dec_skip_space(uba, size);
		break;

	default:
		goto end;
	}
	rc = NCSCC_RC_SUCCESS;
 
 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : glsv_gla_DEC_api_resp_evt

  Description   : This routine decodes api response info.

  Arguments     : uba , api response info.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uint32_t glsv_gla_dec_api_resp_evt(NCS_UBAID *uba, GLSV_GLA_API_RESP_INFO *evt)
{
	uint8_t *p8, local_data[20], size;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER();

 /** decode the type of message **/
	size = (2 * 4);
	p8 = ncs_dec_flatten_space(uba, local_data, size);
	if (!p8) {
		TRACE_2("GLA mds dec failure");
		goto end;
	}
	evt->prc_id = ncs_decode_32bit(&p8);
	evt->type = ncs_decode_32bit(&p8);

	ncs_dec_skip_space(uba, size);

	switch (evt->type) {
	case GLSV_GLA_LOCK_RES_OPEN:
		size = 4;
		p8 = ncs_dec_flatten_space(uba, local_data, size);
		if (!p8) {
			TRACE_2("GLA mds dec failure");
			goto end;
		}

		evt->param.res_open.resourceId = ncs_decode_32bit(&p8);

		ncs_dec_skip_space(uba, size);
		break;

	case GLSV_GLA_LOCK_SYNC_LOCK:
		size = 8 + 4;
		p8 = ncs_dec_flatten_space(uba, local_data, size);
		if (!p8) {
			TRACE_2("GLA mds dec failure");
			goto end;
		}

		evt->param.sync_lock.lockId = ncs_decode_64bit(&p8);
		evt->param.sync_lock.lockStatus = ncs_decode_32bit(&p8);

		ncs_dec_skip_space(uba, size);
		break;

	default:
		break;
	}
	rc = NCSCC_RC_SUCCESS;
	
 end:
	TRACE_LEAVE();
	return rc;
}
