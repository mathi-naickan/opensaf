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

#include "lck/lckd/gld.h"
#include <string.h>
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

static uint32_t gld_mds_enc_flat(GLSV_GLD_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uint32_t gld_mds_dec_flat(GLSV_GLD_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uint32_t gld_mds_dec(GLSV_GLD_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info);
static uint32_t gld_mds_enc(GLSV_GLD_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info);

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
uint32_t gld_mds_callback(NCSMDS_CALLBACK_INFO *info)
{
	uint32_t cb_hdl;
	GLSV_GLD_CB *cb = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;
	TRACE_ENTER2("mds service id %u", info->i_yr_svc_id);

	if (info == NULL)
		goto end;

	cb_hdl = (uint32_t)info->i_yr_svc_hdl;

	if ((cb = (GLSV_GLD_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLD, cb_hdl)) == NULL) {
		LOG_ER("Handle take failed");
		rc = NCSCC_RC_SUCCESS;
		goto end;
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
		LOG_ER("MDS Call back error");
	}

	ncshm_give_hdl((uint32_t)cb_hdl);
 end:
	TRACE_LEAVE2("return value: %u", rc);
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
uint32_t gld_mds_quiesced_process(GLSV_GLD_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *rcv_info)
{
	GLSV_GLD_EVT *evt;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("mds_dest %" PRIx64, rcv_info->i_dest);

	evt = m_MMGR_ALLOC_GLSV_GLD_EVT;
	if (evt == GLSV_GLD_EVT_NULL) {
		LOG_CR("Event alloc failed: Error %s", strerror(errno));
		assert(0);
	}
	memset(evt, 0, sizeof(GLSV_GLD_EVT));
	evt->gld_cb = cb;
	evt->evt_type = GLSV_GLD_EVT_QUISCED_STATE;

	/* Push the event and we are done */
	if (m_NCS_IPC_SEND(&cb->mbx, evt, NCS_IPC_PRIORITY_NORMAL) == NCSCC_RC_FAILURE) {
		LOG_ER("IPC send failed");
		gld_evt_destroy(evt);
		rc = NCSCC_RC_FAILURE;
	}
 
	TRACE_LEAVE2("Return value: %u", rc);
	return rc;
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
uint32_t gld_mds_svc_evt(GLSV_GLD_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *rcv_info)
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
					LOG_CR("Event alloc failed: Error %s", strerror(errno));
					assert(0);
				}
				memset(evt, 0, sizeof(GLSV_GLD_EVT));
				evt->gld_cb = cb;
				evt->evt_type = GLSV_GLD_EVT_GLND_DOWN;

				evt->info.glnd_mds_info.mds_dest_id = rcv_info->i_dest;

				/* Push the event and we are done */
				if (m_NCS_IPC_SEND(&cb->mbx, evt, NCS_IPC_PRIORITY_NORMAL) == NCSCC_RC_FAILURE) {
					LOG_ER("IPC send failed");
					gld_evt_destroy(evt);
					return (NCSCC_RC_FAILURE);
				}
			}
		}
		break;


	default:
		LOG_ER("MDS unsubscribed svc evt");
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

uint32_t gld_mds_rcv(GLSV_GLD_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	GLSV_GLD_EVT *evt = (GLSV_GLD_EVT *)rcv_info->i_msg;

	/* Fill GLD CB handle in the received message */
	evt->gld_cb = cb;
	evt->fr_dest_id = rcv_info->i_fr_dest;

	if (m_NCS_IPC_SEND(&cb->mbx, evt, NCS_IPC_PRIORITY_NORMAL) == NCSCC_RC_FAILURE) {
		LOG_ER("IPC send failed");
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
static uint32_t gld_mds_enc(GLSV_GLD_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info)
{
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_FAILURE;

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
		LOG_ER("GLD msg format version invalid");
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
static uint32_t gld_mds_dec(GLSV_GLD_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info)
{
	EDU_ERR ederror = 0;
	GLSV_GLD_EVT *evt;
	uint32_t rc = NCSCC_RC_SUCCESS;
	bool is_valid_msg_fmt = false;

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
		LOG_ER("GLD msg format version invalid");
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
static uint32_t gld_mds_enc_flat(GLSV_GLD_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info)
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
			ncs_encode_n_octets_in_uba(uba, (uint8_t *)evt, sizeof(GLSV_GLND_EVT));
			if (evt->type == GLSV_GLND_EVT_RSC_MASTER_INFO) {
				if (evt->info.rsc_master_info.no_of_res > 0) {
					GLSV_GLND_RSC_MASTER_INFO_LIST *rsc_master_list =
					    evt->info.rsc_master_info.rsc_master_list;
					ncs_encode_n_octets_in_uba(uba, (uint8_t *)rsc_master_list,
								   (sizeof(GLSV_GLND_RSC_MASTER_INFO_LIST) *
								    evt->info.rsc_master_info.no_of_res));
				}
			}
			return NCSCC_RC_SUCCESS;
		}
	} else {
		/* Drop The Message */
		LOG_ER("GLD msg format version invalid");
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
static uint32_t gld_mds_dec_flat(GLSV_GLD_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info)
{
	GLSV_GLD_EVT *evt;
	NCS_UBAID *uba = info->io_uba;
	bool is_valid_msg_fmt = false;

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
				LOG_CR("Event alloc failed: Error %s", strerror(errno));
				assert(0);
			}
			info->o_msg = evt;
			ncs_decode_n_octets(uba->ub, (uint8_t *)evt, sizeof(GLSV_GLD_EVT));
			return NCSCC_RC_SUCCESS;
		} else
			return NCSCC_RC_FAILURE;
	} else {
		/* Drop The Message */
		LOG_ER("GLD msg format version invalid");
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
uint32_t gld_mds_cpy(GLSV_GLD_CB *cb, MDS_CALLBACK_COPY_INFO *cpy_info)
{
	GLSV_GLND_EVT *src = (GLSV_GLND_EVT *)cpy_info->i_msg;
	GLSV_GLND_EVT *cpy = m_MMGR_ALLOC_GLND_EVT;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (cpy == NULL) {
		LOG_CR("Event alloc failed:Error %s", strerror(errno));
		assert(0);
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
 
	TRACE_LEAVE();
	return rc;
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
uint32_t gld_mds_vdest_create(GLSV_GLD_CB *cb)
{
	NCSVDA_INFO arg;
	uint32_t rc;
	SaClmClusterNodeT cluster_node;
	uint32_t seed;
	TRACE_ENTER();

	memset(&cluster_node, 0, sizeof(SaClmClusterNodeT));

	memset(&arg, 0, sizeof(NCSVDA_INFO));
	arg.req = NCSVDA_VDEST_CREATE;
	arg.info.vdest_create.i_persistent = false;
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
		goto end;
	}
	cb->mds_handle = arg.info.vdest_create.o_mds_pwe1_hdl;
 end:
	TRACE_LEAVE2("Return value %u", rc);
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
uint32_t gld_mds_vdest_destroy(GLSV_GLD_CB *cb)
{
	NCSVDA_INFO arg;
	uint32_t rc;
	SaNameT name = { 4, "GLD" };
	TRACE_ENTER();

	memset(&arg, 0, sizeof(NCSVDA_INFO));

	arg.req = NCSVDA_VDEST_DESTROY;
	arg.info.vdest_destroy.i_vdest = cb->my_dest_id;
	arg.info.vdest_destroy.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
	arg.info.vdest_destroy.i_name = name;

	rc = ncsvda_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) 
		LOG_ER("MDS vdest destroy failed");
	
	TRACE_LEAVE2("Return value: %u", rc);
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

uint32_t gld_mds_init(GLSV_GLD_CB *cb)
{
	NCSMDS_INFO arg;
	uint32_t rc;
	MDS_SVC_ID subscr_svc = NCSMDS_SVC_ID_GLND;
	TRACE_ENTER();

	cb->my_dest_id = GLD_VDEST_ID;
	/* In future Anchor value will be taken from AVSV. Now we are using fix value */
	cb->my_anc = V_DEST_QA_1;

	/* Create the vertual Destination for GLD */
	rc = gld_mds_vdest_create(cb);
	if (rc != NCSCC_RC_SUCCESS)
		goto end;
	/* Set the role to active  - Needs to be removed after integration with AvSv */

	/* Install your service into MDS */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->mds_handle;
	arg.i_svc_id = NCSMDS_SVC_ID_GLD;
	arg.i_op = MDS_INSTALL;

	arg.info.svc_install.i_yr_svc_hdl = cb->my_hdl;
	arg.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	arg.info.svc_install.i_svc_cb = gld_mds_callback;
	arg.info.svc_install.i_mds_q_ownership = false;
	arg.info.svc_install.i_mds_svc_pvt_ver = GLD_PVT_SUBPART_VERSION;

	rc = ncsmds_api(&arg);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS Install failed");
		goto end;
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
		LOG_ER("MDS Subscription failed");
		goto end;
	}
 end:
	TRACE_LEAVE2("Return value %u", rc);
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
uint32_t gld_mds_shut(GLSV_GLD_CB *cb)
{
	NCSMDS_INFO arg;
	uint32_t rc;
	TRACE_ENTER();

	/* Un-install your service into MDS */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->mds_handle;
	arg.i_svc_id = NCSMDS_SVC_ID_GLD;
	arg.i_op = MDS_UNINSTALL;

	rc = ncsmds_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS Uninstall failed");
		goto end;
	}

	/* Destroy the virtual Destination of GLD */
	rc = gld_mds_vdest_destroy(cb);
 end:
	TRACE_LEAVE2("Return value %u", rc);
	return rc;
}

/****************************************************************************
  Name          : gld_mds_change_role
 
  Description   : This routine use for setting and changing role. 
 
  Arguments     : role - Role to be set.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t gld_mds_change_role(GLSV_GLD_CB *cb, V_DEST_RL role)
{
	NCSVDA_INFO arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	memset(&arg, 0, sizeof(NCSVDA_INFO));

	arg.req = NCSVDA_VDEST_CHG_ROLE;
	arg.info.vdest_chg_role.i_vdest = cb->my_dest_id;
	arg.info.vdest_chg_role.i_new_role = role;
	if (ncsvda_api(&arg) != NCSCC_RC_SUCCESS) {
		rc = m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		LOG_ER("ncsvda_api failed");
	}
	TRACE_LEAVE2("Return value %u", rc);
	return rc;
}

/*****************************************************************************
  PROCEDURE NAME : gld_process_node_down_evts

  DESCRIPTION    :

  ARGUMENTS      :gld_cb      - ptr to the GLD control block
                  evt          - ptr to the event.

  RETURNS        :NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS

  NOTES         : None
*****************************************************************************/
uint32_t gld_process_node_down_evts(GLSV_GLD_CB *gld_cb)
{
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	GLSV_GLD_GLND_RSC_REF *glnd_rsc = NULL;
	SaLckResourceIdT rsc_id;
	uint32_t node_id = 0;
	TRACE_ENTER();
	
	/* cleanup the  glnd details tree */
	node_details = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_getnext(&gld_cb->glnd_details, (uint8_t *)0);
	while (node_details) {
		node_id = node_details->node_id;
		if (node_details->status == GLND_DOWN_STATE) {
			/* Remove the reference to each of the resource referred by this node */
			glnd_rsc =
			    (GLSV_GLD_GLND_RSC_REF *)ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uint8_t *)0);
			if (glnd_rsc) {
				rsc_id = glnd_rsc->rsc_id;
				while (glnd_rsc) {
					gld_rsc_rmv_node_ref(gld_cb, glnd_rsc->rsc_info, glnd_rsc, node_details,
							     glnd_rsc->rsc_info->can_orphan);
					glnd_rsc = (GLSV_GLD_GLND_RSC_REF *)
					    ncs_patricia_tree_getnext(&node_details->rsc_info_tree, (uint8_t *)&rsc_id);
					if (glnd_rsc)
						rsc_id = glnd_rsc->rsc_id;
				}
			}
			/* Now delete this node details node */
			if (ncs_patricia_tree_del(&gld_cb->glnd_details, (NCS_PATRICIA_NODE *)node_details) !=
			    NCSCC_RC_SUCCESS) {
				LOG_ER("Patricia tree del failed: node_id %u ", node_details->node_id);
			} else {
				m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(node_details);
				TRACE_2("Node getting removed on active: node_id %u", node_id);
			}
		}
		node_details =
		    (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_getnext(&gld_cb->glnd_details, (uint8_t *)&node_id);
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
