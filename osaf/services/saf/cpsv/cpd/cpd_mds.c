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

  This file contains routines used by CPD library for MDS interaction.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

*****************************************************************************/

#include "cpd.h"
#define FUNC_NAME(DS) cpsv_edp_##DS##_info

uint32_t FUNC_NAME(CPSV_EVT) ();

uint32_t cpd_mds_callback(struct ncsmds_callback_info *info);
static uint32_t cpd_mds_enc(CPD_CB *cb, MDS_CALLBACK_ENC_INFO *info);
static uint32_t cpd_mds_dec(CPD_CB *cb, MDS_CALLBACK_DEC_INFO *info);
static uint32_t cpd_mds_enc_flat(CPD_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uint32_t cpd_mds_dec_flat(CPD_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uint32_t cpd_mds_rcv(CPD_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info);
static uint32_t cpd_mds_svc_evt(CPD_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt);
static uint32_t cpd_mds_quiesced_ack_process(CPD_CB *cb);

/* Message Format Verion Tables at CPND */

MDS_CLIENT_MSG_FORMAT_VER cpd_cpnd_msg_fmt_table[CPD_WRT_CPND_SUBPART_VER_RANGE] = {
	1, 2, 3
};

MDS_CLIENT_MSG_FORMAT_VER cpd_cpa_msg_fmt_table[CPD_WRT_CPA_SUBPART_VER_RANGE] = {
	1, 2
};

/****************************************************************************\
 PROCEDURE NAME : cpd_mds_vdest_create

 DESCRIPTION    : This function Creates the Virtual destination for CPD

 ARGUMENTS      : cb : CPD control Block pointer.

 RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
\*****************************************************************************/

uint32_t cpd_mds_vdest_create(CPD_CB *cb)
{
	NCSVDA_INFO arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
/*   SaNameT     name = {4,"CPD"}; */

	TRACE_ENTER();
	memset(&arg, 0, sizeof(arg));

	cb->cpd_dest_id = CPD_VDEST_ID;

	arg.req = NCSVDA_VDEST_CREATE;
/*   arg.info.vdest_create.info.named.i_name = name;
   arg.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_NAMED; */
	arg.info.vdest_create.i_persistent = false;
	arg.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;
	arg.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;

	arg.info.vdest_create.info.specified.i_vdest = cb->cpd_dest_id;

	/* Create VDEST */
	rc = ncsvda_api(&arg);
	if (NCSCC_RC_SUCCESS != rc) {
		m_LEAP_DBG_SINK_VOID;
		return rc;
	}

/*   cb->cpd_dest_id = arg.info.vdest_create.info.named.o_vdest;  */
	/*  cb->cpd_anc     = arg.info.vdest_create.info.named.o_anc; */
	cb->mds_handle = arg.info.vdest_create.o_mds_pwe1_hdl;
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : cpd_mds_register
 
  Description   : This routine registers the CPD Service with MDS.
 
  Arguments     : mqa_cb - ptr to the CPD control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uint32_t cpd_mds_register(CPD_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCSMDS_INFO svc_info;
	MDS_SVC_ID svc_id[1] = { NCSMDS_SVC_ID_CPND };
	MDS_SVC_ID cpd_id[1] = { NCSMDS_SVC_ID_CPD };
	uint32_t phy_slot_sub_slot;

	TRACE_ENTER();
	/* Create the virtual Destination for  CPD */
	rc = cpd_mds_vdest_create(cb);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("cpd mds vdest create failed"); 
		return rc;
	}

	/* memset the svc_info */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));

	/* STEP 2 : Install with MDS with service ID NCSMDS_SVC_ID_CPD. */
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_CPD;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = cb->cpd_hdl;
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/* node specific */
	svc_info.info.svc_install.i_svc_cb = cpd_mds_callback;	/* callback */
	svc_info.info.svc_install.i_mds_q_ownership = false;
	svc_info.info.svc_install.i_mds_svc_pvt_ver = CPD_MDS_PVT_SUBPART_VERSION;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("cpd mds install failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	/*  cb->cpd_dest_id = svc_info.info.svc_install.o_dest;  */

	/* STEP 3: Subscribe to CPD for redundancy events */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_CPD;
	svc_info.i_op = MDS_RED_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = cpd_id;
	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		TRACE_4("cpd mds subscribe cpd redundancy events failed ");
		cpd_mds_unregister(cb);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/* STEP 4: Subscribe to CPND up/down events */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_CPD;
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		TRACE_4("cpd mds subscribe to CPND up/down events failed ");
		cpd_mds_unregister(cb);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	/* STEP 5: Subscribe to CPA up/down events */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_id[0] = NCSMDS_SVC_ID_CPA;
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_CPD;
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		TRACE_4("cpd mds subscribe to CPA up/down events failed");
		cpd_mds_unregister(cb);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/* Get the node id of local CPD */
	cb->node_id = m_NCS_GET_NODE_ID;
	phy_slot_sub_slot = cpd_get_slot_sub_slot_id_from_node_id(cb->node_id);
	cb->cpd_self_id = phy_slot_sub_slot;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpd_mds_unregister
 *
 * Description   : This function un-registers the CPD Service with MDS.
 *
 * Arguments     : cb   : CPD control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void cpd_mds_unregister(CPD_CB *cb)
{
	NCSMDS_INFO arg;
	TRACE_ENTER();
	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->mds_handle;
	arg.i_svc_id = NCSMDS_SVC_ID_CPD;
	arg.i_op = MDS_UNINSTALL;

	if (ncsmds_api(&arg) != NCSCC_RC_SUCCESS) {
		TRACE_4("cpd mds unreg failed");
	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : cpd_mds_callback
 
  Description   : This callback routine will be called by MDS on event arrival
 
  Arguments     : info - pointer to the mds callback info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t cpd_mds_callback(struct ncsmds_callback_info *info)
{
	CPD_CB *cb = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;
	
	TRACE_ENTER();
	if (info == NULL)
		return rc;

	cb = (CPD_CB *)ncshm_take_hdl(NCS_SERVICE_ID_CPD, (uint32_t)info->i_yr_svc_hdl);
	if (!cb) {
		TRACE_4("cpd cb take hdl failed");
		return rc;
	}

	switch (info->i_op) {
	case MDS_CALLBACK_COPY:
		rc = NCSCC_RC_FAILURE;
		break;
	case MDS_CALLBACK_ENC:
		rc = cpd_mds_enc(cb, &info->info.enc);
		break;
	case MDS_CALLBACK_DEC:
		rc = cpd_mds_dec(cb, &info->info.dec);
		break;
	case MDS_CALLBACK_ENC_FLAT:
		rc = cpd_mds_enc_flat(cb, &info->info.enc_flat);
		break;
	case MDS_CALLBACK_DEC_FLAT:
		rc = cpd_mds_dec_flat(cb, &info->info.dec_flat);
		break;
	case MDS_CALLBACK_RECEIVE:
		rc = cpd_mds_rcv(cb, &info->info.receive);
		break;

	case MDS_CALLBACK_SVC_EVENT:
		rc = cpd_mds_svc_evt(cb, &info->info.svc_evt);
		break;

	case MDS_CALLBACK_QUIESCED_ACK:
		rc = cpd_mds_quiesced_ack_process(cb);
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
  Name          : cpd_mds_enc
 
  Description   : This function encodes an events sent from CPD.
 
  Arguments     : cb    : CPD control Block.
                  info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t cpd_mds_enc(CPD_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info)
{
	CPSV_EVT *msg_ptr = NULL;
	EDU_ERR ederror = 0;

	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_CPA) {
		enc_info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
								CPD_WRT_CPA_SUBPART_VER_MIN,
								CPD_WRT_CPA_SUBPART_VER_MAX, cpd_cpa_msg_fmt_table);

	} else if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_CPND) {
		enc_info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
								CPD_WRT_CPND_SUBPART_VER_MIN,
								CPD_WRT_CPND_SUBPART_VER_MAX, cpd_cpnd_msg_fmt_table);

	}

	if (enc_info->o_msg_fmt_ver) {

		msg_ptr = (CPSV_EVT *)enc_info->i_msg;

		return (m_NCS_EDU_VER_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_EVT),
					   enc_info->io_uba, EDP_OP_TYPE_ENC, msg_ptr, &ederror,
					   enc_info->i_rem_svc_pvt_ver));
	} else {
		TRACE_4("INVALID MSG FORMAT IN ENC FULL");	/* Drop The Message,Format Version Invalid */
		return NCSCC_RC_FAILURE;
	}

}

/****************************************************************************
  Name          : cpd_mds_dec
 
  Description   : This function decodes an events sent to CPD.
 
  Arguments     : cb    : CPD control Block.
                  info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t cpd_mds_dec(CPD_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info)
{

	CPSV_EVT *msg_ptr;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	bool is_valid_msg_fmt = false;

	if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_CPND) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
							     CPD_WRT_CPND_SUBPART_VER_MIN,
							     CPD_WRT_CPND_SUBPART_VER_MAX, cpd_cpnd_msg_fmt_table);
	}
	if (is_valid_msg_fmt) {
		msg_ptr = m_MMGR_ALLOC_CPSV_EVT(NCS_SERVICE_ID_CPD);
		if (!msg_ptr)
			return NCSCC_RC_FAILURE;

		memset(msg_ptr, 0, sizeof(CPSV_EVT));
		dec_info->o_msg = (NCSCONTEXT)msg_ptr;

		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_EVT),
				    dec_info->io_uba, EDP_OP_TYPE_DEC, (CPSV_EVT **)&dec_info->o_msg, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd mds decode failed ");
			m_MMGR_FREE_CPSV_EVT(dec_info->o_msg, NCS_SERVICE_ID_CPD);
		}
		return rc;
	} else {
		TRACE_4("cpd mds dec failed");
		return NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
  Name          : cpd_mds_enc_flat
 
  Description   : This function encodes an events sent from CPD.
 
  Arguments     : cb    : CPD control Block.
                  enc_info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t cpd_mds_enc_flat(CPD_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info)
{
	CPSV_EVT *evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCS_UBAID *uba = info->io_uba;

	TRACE_ENTER();
	/* Get the Msg Format version from the SERVICE_ID & RMT_SVC_PVT_SUBPART_VERSION */
	if (info->i_to_svc_id == NCSMDS_SVC_ID_CPA) {
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    CPD_WRT_CPA_SUBPART_VER_MIN,
							    CPD_WRT_CPA_SUBPART_VER_MAX, cpd_cpa_msg_fmt_table);

	} else if (info->i_to_svc_id == NCSMDS_SVC_ID_CPND) {
		info->o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
							    CPD_WRT_CPND_SUBPART_VER_MIN,
							    CPD_WRT_CPND_SUBPART_VER_MAX, cpd_cpnd_msg_fmt_table);

	}

	if (info->o_msg_fmt_ver) {

		/* as all the event structures are flat */
		evt = (CPSV_EVT *)info->i_msg;

		rc = cpsv_evt_enc_flat(&cb->edu_hdl, evt, uba);
		if (rc == NCSCC_RC_FAILURE) {
			TRACE_4("cpd mds enc flat failed");
			TRACE_LEAVE();
			return rc;
		}
	} else {

		TRACE_4("cpd mds enc flat failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;	/* Drop The Message */

	}

	/*   ncs_encode_n_octets_in_uba(uba,(uint8_t*)evt,size);   */

	/* Based on the event type copy the internal pointers TBD */
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : cpd_mds_dec_flat
 
  Description   : This function decodes an events sent to CPD.
 
  Arguments     : cb    : CPD control Block.
                  dec_info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t cpd_mds_dec_flat(CPD_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info)
{
	CPSV_EVT *evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCS_UBAID *uba = info->io_uba;
	bool is_valid_msg_fmt = false;
	TRACE_ENTER();
	if (info->i_fr_svc_id == NCSMDS_SVC_ID_CPND) {
		is_valid_msg_fmt = m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
							     CPD_WRT_CPND_SUBPART_VER_MIN,
							     CPD_WRT_CPND_SUBPART_VER_MAX, cpd_cpnd_msg_fmt_table);
	}
	if (is_valid_msg_fmt) {

		evt = (CPSV_EVT *)m_MMGR_ALLOC_CPSV_EVT(NCS_SERVICE_ID_CPD);
		if (evt == NULL) {
			LOG_ER("cpd evt memory allocation failed ");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		info->o_msg = evt;
		rc = cpsv_evt_dec_flat(&cb->edu_hdl, uba, evt);
		if (rc == NCSCC_RC_FAILURE) {
			TRACE_4("CPD MDS Decode Flat Failed");
			m_MMGR_FREE_CPSV_EVT(evt, NCS_SERVICE_ID_CPD);
			TRACE_LEAVE();
			return rc;
		}

/*   ncs_decode_n_octets(uba->ub,(uint8_t*)evt,sizeof(CPSV_EVT)); */
		/* Based on the event type copy the internal pointers TBD */
		TRACE_LEAVE();
		return rc;
	} else {
		/* Drop The message */
		TRACE_4("CPD MDS Decode Flat Failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;

	}

}

/****************************************************************************
 * Name          : cpd_mds_rcv
 *
 * Description   : MDS will call this function on receiving CPD messages.
 *
 * Arguments     : cb - CPD Control Block
 *                 rcv_info - MDS Receive information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t cpd_mds_rcv(CPD_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	CPSV_EVT *pEvt = (CPSV_EVT *)rcv_info->i_msg;

	pEvt->sinfo.ctxt = rcv_info->i_msg_ctxt;
	pEvt->sinfo.dest = rcv_info->i_fr_dest;
	pEvt->sinfo.to_svc = rcv_info->i_fr_svc_id;
	if (rcv_info->i_rsp_reqd) {
		pEvt->sinfo.stype = MDS_SENDTYPE_RSP;
	}

	/* Put it in CPD's Event Queue */
	rc = m_NCS_IPC_SEND(&cb->cpd_mbx, (NCSCONTEXT)pEvt, NCS_IPC_PRIORITY_NORMAL);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("cpd ipc send failed for mds receive");
	}
	return rc;
}

/****************************************************************************
 * Name          : cpd_mds_svc_evt
 *
 * Description   : CPD is informed when MDS events occurr that he has 
 *                 subscribed to
 *
 * Arguments     : 
 *   cb          : CPD control Block.
 *   enc_info    : Svc evt info.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t cpd_mds_svc_evt(CPD_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{
	CPSV_EVT *evt = NULL;
	uint32_t rc;

	if (svc_evt->i_svc_id == NCSMDS_SVC_ID_CPA)
		return NCSCC_RC_SUCCESS;

	evt = m_MMGR_ALLOC_CPSV_EVT(NCS_SERVICE_ID_CPD);

	if (!evt) {
		LOG_ER("cpd evt memory allocation failed ");
		return NCSCC_RC_OUT_OF_MEM;
	}

	/* Send the CPD_EVT_MDS_INFO to CPD */
	memset(evt, 0, sizeof(CPSV_EVT));
	evt->type = CPSV_EVT_TYPE_CPD;
	evt->info.cpd.type = CPD_EVT_MDS_INFO;
	evt->info.cpd.info.mds_info.change = svc_evt->i_change;
	evt->info.cpd.info.mds_info.dest = svc_evt->i_dest;
	evt->info.cpd.info.mds_info.svc_id = svc_evt->i_svc_id;
	evt->info.cpd.info.mds_info.node_id = svc_evt->i_node_id;

	/* Put it in CPD's Event Queue */
	rc = m_NCS_IPC_SEND(&cb->cpd_mbx, (NCSCONTEXT)evt, NCS_IPC_PRIORITY_HIGH);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("cpd evt mds info ipc send failed");
		m_MMGR_FREE_CPSV_EVT(evt, NCS_SERVICE_ID_CPD);
		TRACE_LEAVE();
		return rc;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/***********************************************************************************
 * Name   : cpd_mds_quiesced_ack_process
 *
 * Description   : This callback is received when cpd goes from active to quiesced,
                   we change the mds role in csi set callback and from mds we get the 
                  quiesced_ack callback , post an event to your cpd thread to process
                  the events in mail box
 *
*************************************************************************************/

uint32_t cpd_mds_quiesced_ack_process(CPD_CB *cb)
{
	CPSV_EVT *evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	if (cb->is_quiesced_set) {
		evt = m_MMGR_ALLOC_CPSV_EVT(NCS_SERVICE_ID_CPD);
		if (!evt) {
			LOG_ER("cpd evt memory allocation failed");
			TRACE_LEAVE();
			return NCSCC_RC_OUT_OF_MEM;
		}

		memset(evt, 0, sizeof(CPSV_EVT));
		evt->type = CPSV_EVT_TYPE_CPD;
		evt->info.cpd.type = CPD_EVT_MDS_QUIESCED_ACK_RSP;

		rc = m_NCS_IPC_SEND(&cb->cpd_mbx, evt, NCS_IPC_PRIORITY_NORMAL);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("cpd ipc send failed");
			m_MMGR_FREE_CPSV_EVT(evt, NCS_SERVICE_ID_CPD);
			TRACE_LEAVE();
			return rc;
		}

		TRACE_LEAVE();
		return rc;
	} else {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
 * Name          : cpd_mds_send_rsp
 *
 * Description   : Send the Response to Sync Requests
 *
 * Arguments     : 
 *
 * Return Values : 
 *
 * Notes         :
 *****************************************************************************/
uint32_t cpd_mds_send_rsp(CPD_CB *cb, CPSV_SEND_INFO *s_info, CPSV_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;

	TRACE_ENTER();
	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_handle;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CPD;
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
		LOG_ER("cpd mds send failed for dest %"PRIu64,s_info->dest);
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : cpd_mds_msg_sync_send
 
  Description   : This routine sends the Sinc requests from CPD
 
  Arguments     : cb  - ptr to the CPD CB
                  i_evt - ptr to the CPSV message
                  o_evt - ptr to the CPSV message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t cpd_mds_msg_sync_send(CPD_CB *cb, uint32_t to_svc, MDS_DEST to_dest,
			    CPSV_EVT *i_evt, CPSV_EVT **o_evt, uint32_t timeout)
{

	NCSMDS_INFO mds_info;
	uint32_t rc;

	TRACE_ENTER();
	if (!i_evt)
		return NCSCC_RC_FAILURE;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_handle;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CPD;
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
		LOG_ER("cpd mds send failed for dest %"PRIu64,to_dest);
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : cpd_mds_msg_send
 
  Description   : This routine sends the Events from CPD
 
  Arguments     : cb  - ptr to the CPD CB
                  i_evt - ptr to the CPSV message
                  o_evt - ptr to the CPSV message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t cpd_mds_msg_send(CPD_CB *cb, uint32_t to_svc, MDS_DEST to_dest, CPSV_EVT *evt)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;

	TRACE_ENTER();
	if (!evt)
		return NCSCC_RC_FAILURE;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_handle;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CPD;
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
		LOG_ER("cpd mds send failed for dest: %"PRIu64,to_dest);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : cpd_mds_bcast_send
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
uint32_t cpd_mds_bcast_send(CPD_CB *cb, CPSV_EVT *evt, NCSMDS_SVC_ID to_svc)
{

	NCSMDS_INFO info;
	uint32_t res;

	TRACE_ENTER();
	memset(&info, 0, sizeof(info));

	info.i_mds_hdl = cb->mds_handle;
	info.i_op = MDS_SEND;
	info.i_svc_id = NCSMDS_SVC_ID_CPD;

	info.info.svc_send.i_msg = (NCSCONTEXT)evt;
	info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	info.info.svc_send.i_sendtype = MDS_SENDTYPE_BCAST;
	info.info.svc_send.i_to_svc = to_svc;
	info.info.svc_send.info.bcast.i_bcast_scope = NCSMDS_SCOPE_NONE;

	res = ncsmds_api(&info);
	if (res != NCSCC_RC_SUCCESS) {
		LOG_ER("cpd mds bcast send failed");
	}

	TRACE_LEAVE();
	return (res);
}
