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

#include <ncsencdec_pub.h>
#include "ntfs.h"
#include "ntfsv_enc_dec.h"
#include "ntfs_com.h"
#include "ntfsv_mem.h"
#include "ncssysf_def.h"

/*
NTFS_CKPT_DATA_HEADER
       4                4               4                 2            
-----------------------------------------------------------------
| ckpt_rec_type | num_ckpt_records | tot_data_len |  checksum   | 
-----------------------------------------------------------------

NTFSV_CKPT_COLD_SYNC_MSG
----------------------------------------------------------------------------------------------------------------------
| NTFS_CKPT_DATA_HEADER|CLIENTS|NTFS_CKPT_DATA_HEADER|SUBSCRIPTIONS|NTFS_CKPT_DATA_HEADER|NOTIFICATIONS|async_upd_cnt|
-----------------------------------------------------------------------------------------------------------------------
*/

static uint32_t edp_ed_finalize_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uint32_t *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
static uint32_t edp_ed_header_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			       NCSCONTEXT ptr, uint32_t *ptr_data_len,
			       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t ckpt_proc_reg_rec(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_finalize_rec(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_agent_down_rec(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_notification(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_subscribe(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_unsubscribe(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_not_log_confirm(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_not_send_confirm(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data);

static void enc_ckpt_header(uint8_t *pdata, ntfsv_ckpt_header_t header);
static uint32_t dec_ckpt_header(NCS_UBAID *uba, ntfsv_ckpt_header_t *header);
static uint32_t ckpt_decode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t decode_client_msg(NCS_UBAID *uba, ntfs_ckpt_reg_msg_t *param);
static uint32_t decode_subscribe_msg(NCS_UBAID *uba, ntfsv_subscribe_req_t *param);
static uint32_t decode_not_log_confirm_msg(NCS_UBAID *uba, ntfs_ckpt_not_log_confirm_t *param);
static uint32_t decode_not_send_confirm_msg(NCS_UBAID *uba, ntfs_ckpt_not_send_confirm_t *param);
static uint32_t mbcsv_callback(NCS_MBCSV_CB_ARG *arg);	/* Common Callback interface to mbcsv */
static uint32_t ckpt_decode_async_update(ntfs_cb_t *cb, NCS_MBCSV_CB_ARG *cbk_arg);

static uint32_t ckpt_encode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t ckpt_enc_cold_sync_data(ntfs_cb_t *ntfs_cb, NCS_MBCSV_CB_ARG *cbk_arg, bool data_req);
static uint32_t ckpt_encode_async_update(ntfs_cb_t *ntfs_cb, EDU_HDL edu_hdl, NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t ckpt_decode_cold_sync(ntfs_cb_t *cb, NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t ckpt_peer_info_cbk_handler(NCS_MBCSV_CB_ARG *arg);
static uint32_t ckpt_notify_cbk_handler(NCS_MBCSV_CB_ARG *arg);
static uint32_t ckpt_err_ind_cbk_handler(NCS_MBCSV_CB_ARG *arg);

static uint32_t process_ckpt_data(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data);
static void ntfs_exit(const char *msg, SaAmfRecommendedRecoveryT rec_rcvr);

static NTFS_CKPT_HDLR ckpt_data_handler[NTFS_CKPT_MSG_MAX] = {
	NULL,
	ckpt_proc_reg_rec,
	ckpt_proc_finalize_rec,
	ckpt_proc_agent_down_rec,
	ckpt_proc_notification,
	ckpt_proc_subscribe,
	ckpt_proc_unsubscribe,
	ckpt_proc_not_log_confirm,
	ckpt_proc_not_send_confirm
};

/****************************************************************************
 * Name          : ntfsv_mbcsv_init 
 *
 * Description   : This function initializes the mbcsv interface and
 *                 obtains a selection object from mbcsv.
 *                 
 * Arguments     : NTFS_CB * - A pointer to the ntfs control block.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t ntfs_mbcsv_init(ntfs_cb_t *cb)
{
	uint32_t rc;
	NCS_MBCSV_ARG arg;

	TRACE_ENTER();

	/* Initialize with MBCSv library */
	arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	arg.info.initialize.i_mbcsv_cb = mbcsv_callback;
	arg.info.initialize.i_version = NTFS_MBCSV_VERSION;
	arg.info.initialize.i_service = NCS_SERVICE_ID_NTFS;

	if ((rc = ncs_mbcsv_svc(&arg)) != NCSCC_RC_SUCCESS) {
		LOG_ER("NCS_MBCSV_OP_INITIALIZE FAILED");
		goto done;
	}

	cb->mbcsv_hdl = arg.info.initialize.o_mbcsv_hdl;

	/* Open a checkpoint */
	arg.i_op = NCS_MBCSV_OP_OPEN;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	arg.info.open.i_pwe_hdl = (uint32_t)cb->mds_hdl;
	arg.info.open.i_client_hdl = 0;

	if ((rc = ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS)) {
		LOG_ER("NCS_MBCSV_OP_OPEN FAILED");
		goto done;
	}
	cb->mbcsv_ckpt_hdl = arg.info.open.o_ckpt_hdl;

	/* Get Selection Object */
	arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	arg.info.sel_obj_get.o_select_obj = 0;
	if (NCSCC_RC_SUCCESS != (rc = ncs_mbcsv_svc(&arg))) {
		LOG_ER("NCS_MBCSV_OP_SEL_OBJ_GET FAILED");
		goto done;
	}

	cb->mbcsv_sel_obj = arg.info.sel_obj_get.o_select_obj;
	cb->ckpt_state = COLD_SYNC_IDLE;

	/* Disable warm sync */
	arg.i_op = NCS_MBCSV_OP_OBJ_SET;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	arg.info.obj_set.i_ckpt_hdl = cb->mbcsv_ckpt_hdl;
	arg.info.obj_set.i_obj = NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF;
	arg.info.obj_set.i_val = false;
	if (ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("NCS_MBCSV_OP_OBJ_SET FAILED");
		goto done;
	}

	rc = ntfs_mbcsv_change_HA_state(ntfs_cb);

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : ntfs_mbcsv_change_HA_state 
 *
 * Description   : This function inform mbcsv of our HA state. 
 *                 All checkpointing operations are triggered based on the 
 *                 state.
 *
 * Arguments     : NTFS_CB * - A pointer to the ntfs control block.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : This function should be ideally called only once, i.e.
 *                 during the first CSI assignment from AVSv  .
 *****************************************************************************/

uint32_t ntfs_mbcsv_change_HA_state(ntfs_cb_t *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;
	uint32_t rc = SA_AIS_OK;
	TRACE_ENTER();
	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	/* Set the mbcsv args */
	mbcsv_arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.chg_role.i_ckpt_hdl = cb->mbcsv_ckpt_hdl;
	mbcsv_arg.info.chg_role.i_ha_state = cb->ha_state;

	if (SA_AIS_OK != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		LOG_ER("ncs_mbcsv_svc NCS_MBCSV_OP_CHG_ROLE FAILED");
		rc = NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return rc;
}	/*End ntfs_mbcsv_change_HA_state */

/****************************************************************************
 * Name          : ntfs_mbcsv_change_HA_state 
 *
 * Description   : This function inform mbcsv of our HA state. 
 *                 All checkpointing operations are triggered based on the 
 *                 state.
 *
 * Arguments     : NCS_MBCSV_HDL - Handle provided by MBCSV during op_init. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : This function should be ideally called only once, i.e.
 *                 during the first CSI assignment from AVSv  .
 *****************************************************************************/
uint32_t ntfs_mbcsv_dispatch(NCS_MBCSV_HDL mbcsv_hdl)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, 0, sizeof(NCS_MBCSV_ARG));
	mbcsv_arg.i_op = NCS_MBCSV_OP_DISPATCH;
	mbcsv_arg.i_mbcsv_hdl = mbcsv_hdl;
	mbcsv_arg.info.dispatch.i_disp_flags = SA_DISPATCH_ALL;

	return ncs_mbcsv_svc(&mbcsv_arg);
}

/****************************************************************************
 * Name          : mbcsv_callback
 *
 * Description   : This callback is the single entry point for mbcsv to 
 *                 notify ntfs of all checkpointing operations. 
 *
 * Arguments     : NCS_MBCSV_CB_ARG - Callback Info pertaining to the mbcsv
 *                 event from ACTIVE/STANDBY NTFS peer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : Based on the mbcsv message type, the corresponding mbcsv
 *                 message handler shall be invoked.
 *****************************************************************************/
static uint32_t mbcsv_callback(NCS_MBCSV_CB_ARG *arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	osafassert(arg != NULL);

	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		/* Encode Request from MBCSv */
		rc = ckpt_encode_cbk_handler(arg);
		break;
	case NCS_MBCSV_CBOP_DEC:
		/* Decode Request from MBCSv */
		rc = ckpt_decode_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("ckpt_decode_cbk_handler FAILED");
		break;
	case NCS_MBCSV_CBOP_PEER:
		/* NTFS Peer info from MBCSv */
		rc = ckpt_peer_info_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("ckpt_peer_info_cbk_handler FAILED");
		break;
	case NCS_MBCSV_CBOP_NOTIFY:
		/* NOTIFY info from NTFS peer */
		rc = ckpt_notify_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("ckpt_notify_cbk_handler FAILED");
		break;
	case NCS_MBCSV_CBOP_ERR_IND:
		/* Peer error indication info */
		rc = ckpt_err_ind_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("ckpt_err_ind_cbk_handler FAILED");
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("default FAILED");
		break;
	}

	return rc;
}

/****************************************************************************
 * Name          : ckpt_encode_cbk_handler
 *
 * Description   : This function invokes the corresponding encode routine
 *                 based on the MBCSv encode request type.
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with encode info
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ckpt_encode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t mbcsv_version;

	osafassert(cbk_arg != NULL);

	mbcsv_version = m_NCS_MBCSV_FMT_GET(cbk_arg->info.encode.i_peer_version,
					    NTFS_MBCSV_VERSION, NTFS_MBCSV_VERSION_MIN);
	if (0 == mbcsv_version) {
		TRACE("Wrong mbcsv_version!!!\n");
		return NCSCC_RC_FAILURE;
	}

	switch (cbk_arg->info.encode.io_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		/* Encode async update */
		if ((rc = ckpt_encode_async_update(ntfs_cb, ntfs_cb->edu_hdl, cbk_arg)) != NCSCC_RC_SUCCESS)
			TRACE("  ckpt_encode_async_update FAILED");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		TRACE_2("COLD SYNC REQ ENCODE CALLED");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		/* Encode cold sync response */
		rc = ckpt_enc_cold_sync_data(ntfs_cb, cbk_arg, false);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE(" COLD SYNC ENCODE FAIL....");
		} else {
			ntfs_cb->ckpt_state = COLD_SYNC_COMPLETE;
			TRACE_2(" COLD SYNC RESPONSE SEND SUCCESS....");
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		if ((rc = ckpt_enc_cold_sync_data(ntfs_cb, cbk_arg, true)) != NCSCC_RC_SUCCESS)
			TRACE("  ckpt_enc_cold_sync_data FAILED");
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		TRACE("  default FAILED");
		break;
	}			/*End switch(io_msg_type) */

	return rc;

}	/*End ckpt_encode_cbk_handler() */

/****************************************************************************
 * Name          : ckpt_enc_cold_sync_data
 *
 * Description   : This function encodes cold sync data., viz
 *                 1.REGLIST (CLIENTS)
 *                 2.
 *                 3.Async Update Count
 *                 in that order.
 *                 Each records contain a header specifying the record type
 *                 and number of such records.
 *
 * Arguments     : ntfs_cb - pointer to the ntfs control block. 
 *                 cbk_arg - Pointer to NCS_MBCSV_CB_ARG with encode info.
 *                 data_req - Flag to specify if its for cold sync or data
 *                 request for warm sync.
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ckpt_enc_cold_sync_data(ntfs_cb_t *ntfs_cb, NCS_MBCSV_CB_ARG *cbk_arg, bool data_req)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	/* asynsc Update Count */
	uint8_t *async_upd_cnt = NULL;

	/* Currently, we shall send all data in one send.
	 * This shall avoid "delta data" problems that are associated during
	 * multiple sends
	 */
	TRACE_2("COLD SYNC ENCODE START........");
	/* TODO: Fix return value on syncRequest */
	syncRequest(&cbk_arg->info.encode.io_uba);

	/* This will have the count of async updates that have been sent,
	   this will be 0 initially */
	async_upd_cnt = ncs_enc_reserve_space(&cbk_arg->info.encode.io_uba, sizeof(uint32_t));
	if (async_upd_cnt == NULL) {
		TRACE("ncs_enc_reserve_space FAILED");
		return NCSCC_RC_FAILURE;
	}
	ncs_encode_32bit(&async_upd_cnt, ntfs_cb->async_upd_cnt);
	ncs_enc_claim_space(&cbk_arg->info.encode.io_uba, sizeof(uint32_t));

	/* Set response mbcsv msg type to complete */
	if (data_req == true)
		cbk_arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
	else
		cbk_arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
	TRACE_2("COLD SYNC ENCODE END........");
	return rc;
}	/*End  ckpt_enc_cold_sync_data() */

uint32_t enc_mbcsv_client_msg(NCS_UBAID *uba, ntfs_ckpt_reg_msg_t *param)
{
	uint8_t *p8;

	TRACE_ENTER();

	osafassert(uba != NULL);
    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 12);
	if (!p8) {
		TRACE("NULL pointer");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_64bit(&p8, param->mds_dest);
	ncs_enc_claim_space(uba, 12);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

static uint32_t enc_mbcsv_subscribe_msg(NCS_UBAID *uba, ntfsv_subscribe_req_t *param)
{
	return ntfsv_enc_subscribe_msg(uba, param);
}

static uint32_t enc_mbcsv_log_confirm_msg(NCS_UBAID *uba, ntfs_ckpt_not_log_confirm_t *param)
{
	uint8_t *p8;

	TRACE_ENTER();
	osafassert(uba != NULL);
    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_64bit(&p8, param->notificationId);
	ncs_enc_claim_space(uba, 8);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

static uint32_t enc_mbcsv_send_confirm_msg(NCS_UBAID *uba, ntfs_ckpt_not_send_confirm_t *param)
{
	uint8_t *p8;

	TRACE_ENTER();
	osafassert(uba != NULL);
    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 20);
	if (!p8) {
		TRACE("NULL pointer");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param->clientId);
	ncs_encode_32bit(&p8, param->subscriptionId);
	ncs_encode_64bit(&p8, param->notificationId);
	ncs_encode_32bit(&p8, param->discarded);
	ncs_enc_claim_space(uba, 20);
	TRACE_LEAVE();

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_encode_async_update
 *
 * Description   : This function encodes data to be sent as an async update.
 *                 The caller of this function would set the address of the
 *                 record to be encoded in the reo_hdl field(while invoking
 *                 SEND_CKPT option of ncs_mbcsv_svc. 
 *
 * Arguments     : cb - pointer to the NTFS control block.
 *                 cbk_arg - Pointer to NCS MBCSV callback argument struct.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_encode_async_update(ntfs_cb_t *ntfs_cb, EDU_HDL edu_hdl, NCS_MBCSV_CB_ARG *cbk_arg)
{
	ntfsv_ckpt_msg_t *data = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint8_t *pheader = NULL;
	ntfsv_ckpt_header_t ckpt_hdr;
	NCS_UBAID *uba = &cbk_arg->info.encode.io_uba;

	TRACE_ENTER();
	/* Set reo_hdl from callback arg to ckpt_rec */
	data = (ntfsv_ckpt_msg_t *)(long)cbk_arg->info.encode.io_reo_hdl;
	if (data == NULL) {
		TRACE("   data == NULL, FAILED");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(ntfsv_ckpt_header_t));
	if (pheader == NULL) {
		TRACE("  ncs_enc_reserve_space FAILED");
		TRACE_LEAVE();
		return (rc = EDU_ERR_MEM_FAIL);
	}
	ncs_enc_claim_space(uba, sizeof(ntfsv_ckpt_header_t));

	/* Encode async record,except publish & subscribe */
	switch (data->header.ckpt_rec_type) {
		ntfs_ckpt_reg_msg_t ckpt_reg_rec;
		ntfs_ckpt_subscribe_t subscribe_rec;
		ntfs_ckpt_unsubscribe_t unsubscribe_rec;
		EDU_ERR ederror;

	case NTFS_CKPT_INITIALIZE_REC:

		TRACE("Async update NTFS_CKPT_INITIALIZE_REC");
		/* Encode RegHeader */
		ckpt_hdr.ckpt_rec_type = NTFS_CKPT_INITIALIZE_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		ckpt_reg_rec.client_id = data->ckpt_rec.reg_rec.client_id;
		ckpt_reg_rec.mds_dest = data->ckpt_rec.reg_rec.mds_dest;
		rc = enc_mbcsv_client_msg(uba, &ckpt_reg_rec);
		break;
	case NTFS_CKPT_FINALIZE_REC:
		/* Encode RegHeader */
		ckpt_hdr.ckpt_rec_type = NTFS_CKPT_FINALIZE_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		TRACE_2("FINALIZE REC: AUPDATE");
		rc = m_NCS_EDU_EXEC(&ntfs_cb->edu_hdl,
				    edp_ed_finalize_rec,
				    &cbk_arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &data->ckpt_rec.finalize_rec, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			/* free(data); FIX ??? */
			TRACE_2("eduerr: %x", ederror);
		}
		break;
	case NTFS_CKPT_SUBSCRIBE:
		TRACE("Async update NTFS_CKPT_SUBSCRIBE");
		ckpt_hdr.ckpt_rec_type = NTFS_CKPT_SUBSCRIBE;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */
		enc_ckpt_header(pheader, ckpt_hdr);

		subscribe_rec.arg = data->ckpt_rec.subscribe.arg;
		rc = enc_mbcsv_subscribe_msg(uba, &subscribe_rec.arg);
		break;
	case NTFS_CKPT_UNSUBSCRIBE:
		TRACE("Async update NTFS_CKPT_UNSUBSCRIBE");
		ckpt_hdr.ckpt_rec_type = NTFS_CKPT_UNSUBSCRIBE;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */
		enc_ckpt_header(pheader, ckpt_hdr);

		unsubscribe_rec.arg.client_id = data->ckpt_rec.unsubscribe.arg.client_id;
		unsubscribe_rec.arg.subscriptionId = data->ckpt_rec.unsubscribe.arg.subscriptionId;
		rc = ntfsv_enc_unsubscribe_msg(uba, &unsubscribe_rec.arg);
		break;
	case NTFS_CKPT_NOTIFICATION:
		TRACE("Async update NTFS_CKPT_NOTIFICATION");
		ckpt_hdr.ckpt_rec_type = NTFS_CKPT_NOTIFICATION;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */
		enc_ckpt_header(pheader, ckpt_hdr);
		rc = ntfsv_enc_not_msg(uba, data->ckpt_rec.notification.arg);
		break;
	case NTFS_CKPT_NOT_LOG_CONFIRM:
		TRACE("Async update NTFS_CKPT_NOT_LOG_CONFIRM");
		ckpt_hdr.ckpt_rec_type = NTFS_CKPT_NOT_LOG_CONFIRM;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */
		enc_ckpt_header(pheader, ckpt_hdr);
		rc = enc_mbcsv_log_confirm_msg(uba, &data->ckpt_rec.log_confirm);
		break;

	case NTFS_CKPT_NOT_SEND_CONFIRM:
		TRACE("Async update NTFS_CKPT_NOT_SEND_CONFIRM");
		ckpt_hdr.ckpt_rec_type = NTFS_CKPT_NOT_SEND_CONFIRM;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */
		enc_ckpt_header(pheader, ckpt_hdr);
		rc = enc_mbcsv_send_confirm_msg(uba, &data->ckpt_rec.send_confirm);
		break;
	default:
		TRACE_3("FAILED no type: %d", data->header.ckpt_rec_type);
		break;
	}
	if (rc == NCSCC_RC_SUCCESS) {
		/* Update the Async Update Count at standby */
		ntfs_cb->async_upd_cnt++;
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : ckpt_decode_cbk_handler
 *
 * Description   : This function is the single entry point to all decode
 *                 requests from mbcsv. 
 *                 Invokes the corresponding decode routine based on the 
 *                 MBCSv decode request type.
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with decode info
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ckpt_decode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t msg_fmt_version;

	osafassert(cbk_arg != NULL);

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(cbk_arg->info.decode.i_peer_version,
					      NTFS_MBCSV_VERSION, NTFS_MBCSV_VERSION_MIN);
	if (0 == msg_fmt_version) {
		TRACE("wrong msg_fmt_version!!!\n");
		return NCSCC_RC_FAILURE;
	}
	TRACE_2("decode msg type: %u", (unsigned int)cbk_arg->info.decode.i_msg_type);
	switch (cbk_arg->info.decode.i_msg_type) {
	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		TRACE_2(" COLD SYNC REQ DECODE called");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
		TRACE_2(" COLD SYNC RESP DECODE called");
		if (ntfs_cb->ckpt_state != COLD_SYNC_COMPLETE) {	/*this check is needed to handle repeated requests */
			if ((rc = ckpt_decode_cold_sync(ntfs_cb, cbk_arg)) != NCSCC_RC_SUCCESS) {
				TRACE(" COLD SYNC RESPONSE DECODE ....");
			} else {
				TRACE_2(" COLD SYNC RESPONSE DECODE SUCCESS....");
				ntfs_cb->ckpt_state = COLD_SYNC_COMPLETE;
			}
		}
		break;

	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		TRACE_2(" ASYNC UPDATE DECODE called");
		if ((rc = ckpt_decode_async_update(ntfs_cb, cbk_arg)) != NCSCC_RC_SUCCESS)
			TRACE("  ckpt_decode_async_update FAILED");
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
	case NCS_MBCSV_MSG_DATA_REQ:
		TRACE_2("WARM SYNC called, not used");
		break;
	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		TRACE_2("DATA RESP COMPLETE DECODE called");
		if ((rc = ckpt_decode_cold_sync(ntfs_cb, cbk_arg)) != NCSCC_RC_SUCCESS)
			TRACE("   FAILED");
		break;

	default:
		TRACE_2(" INCORRECT DECODE called");
		rc = NCSCC_RC_FAILURE;
		TRACE("  INCORRECT DECODE called, FAILED");
		m_LEAP_DBG_SINK_VOID;
		break;
	}			/*End switch(io_msg_type) */

	return rc;

}	/*End ckpt_decode_cbk_handler() */

/****************************************************************************
 * Name          : ckpt_decode_async_update 
 *
 * Description   : This function decodes async update data, based on the 
 *                 record type contained in the header. 
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with decode info
 *                 cb - pointer to ntfs cb.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ckpt_decode_async_update(ntfs_cb_t *cb, NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	ntfsv_ckpt_msg_t *ckpt_msg;
	ntfsv_ckpt_header_t *hdr = NULL;
	ntfs_ckpt_reg_msg_t *reg_rec = NULL;
	ntfs_ckpt_subscribe_t *subscribe_rec = NULL;
	ntfs_ckpt_unsubscribe_t *unsubscribe_rec = NULL;
	ntfsv_ckpt_finalize_msg_t *finalize = NULL;
	MDS_DEST *agent_dest = NULL;

	TRACE_ENTER();

	/* Allocate memory to hold the checkpoint message */
	ckpt_msg = calloc(1, sizeof(ntfsv_ckpt_msg_t));

	/* Decode the message header */
	hdr = &ckpt_msg->header;
	rc = m_NCS_EDU_EXEC(&cb->edu_hdl, edp_ed_header_rec, &cbk_arg->info.decode.i_uba,
			    EDP_OP_TYPE_DEC, &hdr, &ederror);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("m_NCS_EDU_EXEC FAILED");
		m_NCS_EDU_PRINT_ERROR_STRING(ederror);
		goto done;
	}

	ederror = 0;
	TRACE_2("ckpt_rec_type: %d ", (int)hdr->ckpt_rec_type);
	/* Call decode routines appropriately */
	switch (hdr->ckpt_rec_type) {
	case NTFS_CKPT_INITIALIZE_REC:
		TRACE_2("INITIALIZE REC: AUPDATE");
		reg_rec = &ckpt_msg->ckpt_rec.reg_rec;

		rc = decode_client_msg(&cbk_arg->info.decode.i_uba, reg_rec);
		break;

	case NTFS_CKPT_NOTIFICATION:
		TRACE_2("NOTIFICATION: AUPDATE");
		/* freed in notificationReceivedUpdate(...) or
		   in NtfNotification destructor */
		ckpt_msg->ckpt_rec.notification.arg = calloc(1, sizeof(ntfsv_send_not_req_t));
		rc = ntfsv_dec_not_msg(&cbk_arg->info.decode.i_uba, ckpt_msg->ckpt_rec.notification.arg);
		break;

	case NTFS_CKPT_NOT_LOG_CONFIRM:
		TRACE_2("NOT_LOG_CONFIRM: AUPDATE");
		rc = decode_not_log_confirm_msg(&cbk_arg->info.decode.i_uba, &ckpt_msg->ckpt_rec.log_confirm);
		break;
	case NTFS_CKPT_NOT_SEND_CONFIRM:
		TRACE_2("NOT_SEND_CONFIRM: AUPDATE");
		rc = decode_not_send_confirm_msg(&cbk_arg->info.decode.i_uba, &ckpt_msg->ckpt_rec.send_confirm);
		break;

	case NTFS_CKPT_SUBSCRIBE:
		TRACE_2("SUBSCRIBE: AUPDATE");
		subscribe_rec = &ckpt_msg->ckpt_rec.subscribe;

		rc = decode_subscribe_msg(&cbk_arg->info.decode.i_uba, &subscribe_rec->arg);
		break;

	case NTFS_CKPT_UNSUBSCRIBE:
		TRACE_2("UNSUBSCRIBE: AUPDATE");
		unsubscribe_rec = &ckpt_msg->ckpt_rec.unsubscribe;

		rc = ntfsv_dec_unsubscribe_msg(&cbk_arg->info.decode.i_uba, &unsubscribe_rec->arg);
		break;

	case NTFS_CKPT_FINALIZE_REC:
		TRACE_2("FINALIZE REC: AUPDATE");
		reg_rec = &ckpt_msg->ckpt_rec.reg_rec;
		finalize = &ckpt_msg->ckpt_rec.finalize_rec;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl,
				    edp_ed_finalize_rec,
				    &cbk_arg->info.decode.i_uba, EDP_OP_TYPE_DEC, &finalize, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("   FAILED");
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			goto done;
		}
		break;
	case NTFS_CKPT_AGENT_DOWN:
		TRACE_2("AGENT DOWN REC: AUPDATE");
		agent_dest = &ckpt_msg->ckpt_rec.agent_dest;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, ncs_edp_mds_dest, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &agent_dest, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("   FAILED");
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			goto done;
		}
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		TRACE("   FAILED");
		goto done;
		break;
	}			/*end switch */
	if(rc == NCSCC_RC_SUCCESS) {
		rc = process_ckpt_data(cb, ckpt_msg);
		/* Update the Async Update Count at standby */
		cb->async_upd_cnt++;
	}
 done:
	free(ckpt_msg);
	TRACE_LEAVE();
	return rc;
	/* if failure, should an indication be sent to active ? */
}

static uint32_t decode_client_msg(NCS_UBAID *uba, ntfs_ckpt_reg_msg_t *param)
{
	uint8_t *p8;
	uint8_t local_data[12];

	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 12);
	param->client_id = ncs_decode_32bit(&p8);
	param->mds_dest = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 12);

	TRACE_8("decode_client_msg");
	return NCSCC_RC_SUCCESS;
}

static uint32_t decode_subscribe_msg(NCS_UBAID *uba, ntfsv_subscribe_req_t *param)
{
	return ntfsv_dec_subscribe_msg(uba, param);
}

static uint32_t decode_not_log_confirm_msg(NCS_UBAID *uba, ntfs_ckpt_not_log_confirm_t *param)
{
	uint8_t *p8;
	uint8_t local_data[8];

	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->notificationId = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	TRACE_8("decode_not_log_confirm_msg");
	return NCSCC_RC_SUCCESS;
}

static uint32_t decode_not_send_confirm_msg(NCS_UBAID *uba, ntfs_ckpt_not_send_confirm_t *param)
{
	uint8_t *p8;
	uint8_t local_data[20];

	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 20);
	param->clientId = ncs_decode_32bit(&p8);
	param->subscriptionId = ncs_decode_32bit(&p8);
	param->notificationId = ncs_decode_64bit(&p8);
	param->discarded = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 20);
	TRACE_8("decode_not_send_confirm_msg");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_decode_cold_sync 
 *
 * Description   : This function decodes async update data, based on the 
 *                 record type contained in the header. 
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with decode info
 *                 cb - pointer to ntfs cb.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : COLD SYNC RECORDS are expected in an order
 *                 1. REG RECORDS
 *                 2. 
 *                 
 *                 For each record type,
 *                     a) decode header.
 *                     b) decode individual records for 
 *                        header->num_records times, 
 *****************************************************************************/

static uint32_t ckpt_decode_cold_sync(ntfs_cb_t *cb, NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_ckpt_msg_t msg;
	ntfsv_ckpt_msg_t *data = &msg;
	/*  NCS_UBAID *uba=NULL; */
	uint32_t num_rec = 0, num_clients;
	ntfs_ckpt_reg_msg_t *reg_rec = NULL;
	uint32_t num_of_async_upd;
	uint8_t *ptr;
	uint8_t data_cnt[16];
	struct NtfGlobals ntfGlobals;
	SaNtfNotificationHeaderT *header;
	memset(&ntfGlobals, 0, sizeof(struct NtfGlobals));

	TRACE_ENTER2("COLD SYNC DECODE START........");
	/* Decode the current message header */
	if ((rc = dec_ckpt_header(&cbk_arg->info.decode.i_uba, &data->header)) != NCSCC_RC_SUCCESS) {
		goto done;
	}
	/* Check if the first in the order of records is reg record */
	if (data->header.ckpt_rec_type != NTFS_CKPT_INITIALIZE_REC) {
		TRACE("FAILED data->header.ckpt_rec_type != NTFS_CKPT_INITIALIZE_REC");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Process the reg_records */
	num_rec = data->header.num_ckpt_records;
	num_clients = data->header.num_ckpt_records;
	TRACE("client ids: num_rec = %u", num_rec);
	while (num_rec) {
		reg_rec = &data->ckpt_rec.reg_rec;
		rc = decode_client_msg(&cbk_arg->info.decode.i_uba, reg_rec);
		if (rc != NCSCC_RC_SUCCESS) {
			goto done;
		}
		/* Update our database */
		rc = process_ckpt_data(cb, data);
		if (rc != NCSCC_RC_SUCCESS) {
			goto done;
		}
		memset(&data->ckpt_rec, 0, sizeof(data->ckpt_rec));
		--num_rec;
	}			/*End while, reg records */

	while (num_clients) {
		/* Decode the current message header */
		if ((rc = dec_ckpt_header(&cbk_arg->info.decode.i_uba, &data->header))
		    != NCSCC_RC_SUCCESS) {
			goto done;
		}

		/* Check if the second in the order of records is subscription record */
		if (data->header.ckpt_rec_type != NTFS_CKPT_SUBSCRIBE) {
			TRACE("FAILED data->header.ckpt_rec_type != NTFS_CKPT_SUBSCRIBE");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		num_rec = data->header.num_ckpt_records;
		TRACE_2("subscribers num_rec: %u", num_rec);
		while (num_rec) {
			ntfsv_subscribe_req_t subscribe_rec;
			rc = decode_subscribe_msg(&cbk_arg->info.decode.i_uba, &subscribe_rec);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE("decode_subscribe_msg FAILED");
				goto done;
			}
			subscriptionAdded(subscribe_rec, NULL);
			num_rec--;
		}
		num_clients--;
	}

	/* decode highest notification id */
	ntfsv_dec_64bit_msg(&cbk_arg->info.decode.i_uba, (uint64_t *)&ntfGlobals.notificationId);
	TRACE_8("notification_id: %llu", ntfGlobals.notificationId);
	ntfsv_dec_64bit_msg(&cbk_arg->info.decode.i_uba, (uint64_t *)&ntfGlobals.clientIdCounter);
	TRACE_8("client_id: %llu", ntfGlobals.clientIdCounter);
	syncGlobals(&ntfGlobals);

	/* Decode the current message header */
	if ((rc = dec_ckpt_header(&cbk_arg->info.decode.i_uba, &data->header))
	    != NCSCC_RC_SUCCESS) {
		goto done;
	}

	TRACE_2("ckpt_rec_type: %u", data->header.ckpt_rec_type);

	/* Check if the second in the order of records is notification record */
	if (data->header.ckpt_rec_type != NTFS_CKPT_NOTIFICATION) {
		TRACE("FAILED data->header.ckpt_rec_type != NTFS_CKPT_NOTIFICATION");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	num_rec = data->header.num_ckpt_records;
	TRACE_2("Notifications num_rec: %u", num_rec);
	while (num_rec) {
		uint32_t noOfSubscriptions = 0, logged = 0;
		/* freed in NtfNotification destructor */
		ntfsv_send_not_req_t *notification_rec = calloc(1, sizeof(ntfsv_send_not_req_t));

		if (notification_rec == NULL) {
			TRACE("calloc FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		rc = ntfsv_dec_not_msg(&cbk_arg->info.decode.i_uba, notification_rec);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("decode_subscribe_msg FAILED");
			goto done;
		}
		notificationReceivedColdSync(notification_rec->client_id,
					     notification_rec->notificationType, notification_rec);

		ntfsv_dec_32bit_msg(&cbk_arg->info.decode.i_uba, &noOfSubscriptions);
		TRACE_2("noOfSubscriptions: %u", noOfSubscriptions);
		while (noOfSubscriptions != 0) {
			unsigned int subscription_id = 0, client_id;
			ntfsv_dec_32bit_msg(&cbk_arg->info.decode.i_uba, &subscription_id);
			ntfsv_dec_32bit_msg(&cbk_arg->info.decode.i_uba, &client_id);
			TRACE_2("subscription: %u", subscription_id);
			ntfsv_get_ntf_header(notification_rec, &header);
			storeMatchingSubscription(*header->notificationId, client_id, subscription_id);
			noOfSubscriptions--;
		}
		ntfsv_dec_32bit_msg(&cbk_arg->info.decode.i_uba, &logged);
		if (logged) {
			notificationLoggedConfirmed(*header->notificationId);
		}
		num_rec--;
	}

	/* Get the async update count */
	ptr = ncs_dec_flatten_space(&cbk_arg->info.decode.i_uba, data_cnt, sizeof(uint32_t));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	cb->async_upd_cnt = num_of_async_upd;
	ncs_dec_skip_space(&cbk_arg->info.decode.i_uba, 4);

	/* If we reached here, we are through. Good enough for coldsync with ACTIVE */
 done:
	if (rc != NCSCC_RC_SUCCESS) {
		/* Do not allow standby to get out of sync */
		ntfs_exit("Cold sync failed", SA_AMF_COMPONENT_RESTART);
	}
	TRACE_LEAVE2("COLD SYNC DECODE END........");
	return rc;
}

/****************************************************************************
 * Name          : process_ckpt_data
 *
 * Description   : This function updates the ntfs internal databases
 *                 based on the data type. 
 *
 * Arguments     : cb - pointer to NTFS ControlBlock. 
 *                 data - pointer to  NTFS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t process_ckpt_data(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	if ((!cb) || (data == NULL)) {
		TRACE("FAILED: (!cb) || (data == NULL)");
		return (rc = NCSCC_RC_FAILURE);
	}

	if ((cb->ha_state == SA_AMF_HA_STANDBY) || (cb->ha_state == SA_AMF_HA_QUIESCED)) {
		if (data->header.ckpt_rec_type >= NTFS_CKPT_MSG_MAX) {
			TRACE("FAILED: data->header.ckpt_rec_type >= NTFS_CKPT_MSG_MAX");
			return NCSCC_RC_FAILURE;
		}
		/* Update the internal database */
		rc = ckpt_data_handler[data->header.ckpt_rec_type] (cb, data);
		return rc;
	} else {
		return (rc = NCSCC_RC_FAILURE);
	}
}	/*End ntfs_process_ckpt_data() */

/****************************************************************************
 * Name          : ckpt_proc_reg_rec
 *
 * Description   : This function updates the ntfs reglist based on the 
 *                 info received from the ACTIVE ntfs peer.
 *
 * Arguments     : cb - pointer to NTFS  ControlBlock.
 *                 data - pointer to  NTFS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_reg_rec(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data)
{
	ntfs_ckpt_reg_msg_t *param = &data->ckpt_rec.reg_rec;

	TRACE_ENTER2("client: %d", param->client_id);
	if (!param->client_id) {
		TRACE("FAILED client_id = 0");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	clientAdded(param->client_id, param->mds_dest, NULL);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_notification
 *
 * Description   : 
 *                 
 *                 
 *
 * Arguments     : cb - pointer to NTFS  ControlBlock.
 *                 data - pointer to  NTFS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_notification(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	ntfsv_send_not_req_t *param = data->ckpt_rec.notification.arg;
	notificationReceivedUpdate(param->client_id, param->notificationType, param);
	TRACE_LEAVE();
	return rc;
}

static uint32_t ckpt_proc_not_log_confirm(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	notificationLoggedConfirmed(data->ckpt_rec.log_confirm.notificationId);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : ckpt_proc_not_send_confirm
 *
 * Description   : confirm that a notification has been sent to a subscriber,
 *                 been discarded or that a discarded message has been sent.                 
 *                 
 * Arguments     : cb - pointer to NTFS  ControlBlock.
 *                 data - pointer to  NTFS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_not_send_confirm(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data)
{
	TRACE_ENTER2("discarded %d", data->ckpt_rec.send_confirm.discarded);
	if (data->ckpt_rec.send_confirm.discarded == NTFS_NOTIFICATION_OK) {	
		notificationSentConfirmed(data->ckpt_rec.send_confirm.clientId,
			data->ckpt_rec.send_confirm.subscriptionId, data->ckpt_rec.send_confirm.notificationId,
			data->ckpt_rec.send_confirm.discarded);
	}
	else if (data->ckpt_rec.send_confirm.discarded == NTFS_NOTIFICATION_DISCARDED) {	
		discardedAdd(data->ckpt_rec.send_confirm.clientId, data->ckpt_rec.send_confirm.subscriptionId,
			data->ckpt_rec.send_confirm.notificationId);
		notificationSentConfirmed(data->ckpt_rec.send_confirm.clientId,
			data->ckpt_rec.send_confirm.subscriptionId, data->ckpt_rec.send_confirm.notificationId,
			data->ckpt_rec.send_confirm.discarded);
	}
	else if (data->ckpt_rec.send_confirm.discarded == NTFS_NOTIFICATION_DISCARDED_LIST_SENT) {	
		discardedClear(data->ckpt_rec.send_confirm.clientId, data->ckpt_rec.send_confirm.subscriptionId);
	}
	else 
		osafassert(0);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_unsubscribe
 *
 * Description   : 
 *
 * Arguments     : cb - pointer to NTFS  ControlBlock.
 *                 data - pointer to  NTFS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_unsubscribe(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data)
{
	ntfsv_unsubscribe_req_t *param = &data->ckpt_rec.unsubscribe.arg;

	TRACE_ENTER2("client: %d", param->client_id);

	if (!param->client_id) {
		TRACE("FAILED client_id = 0");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	subscriptionRemoved(param->client_id, param->subscriptionId, NULL);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_subscribe
 *
 * Description   : 
 *                 
 *                 
 *
 * Arguments     : cb - pointer to NTFS  ControlBlock.
 *                data - pointer to  NTFS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t ckpt_proc_subscribe(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data)
{
	ntfsv_subscribe_req_t *param = &data->ckpt_rec.subscribe.arg;

	TRACE_ENTER2("client: %d", param->client_id);

	if (!param->client_id) {
		TRACE("FAILED client_id = 0");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	subscriptionAdded(*param, NULL);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_finalize_rec
 *
 * Description   : This function clears the ntfs reglist and assosicated DB 
 *                 based on the info received from the ACTIVE ntfs peer.
 *
 * Arguments     : cb - pointer to NTFS ControlBlock.
 *                 data - pointer to  NTFS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_finalize_rec(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data)
{
	ntfsv_ckpt_finalize_msg_t *param = &data->ckpt_rec.finalize_rec;
	TRACE_ENTER();
	/* This insure all resources allocated by this registration are freed. */
	clientRemoved(param->client_id);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_agent_down_rec
 *
 * Description   : This function processes a agent down message 
 *                 received from the ACTIVE NTFS peer.      
 *
 * Arguments     : cb - pointer to NTFS ControlBlock.
 *                 data - pointer to  NTFS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None 
 ****************************************************************************/

static uint32_t ckpt_proc_agent_down_rec(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data)
{
	TRACE_ENTER();
	/* Remove this NTFA entry */
	clientRemoveMDS(data->ckpt_rec.agent_dest);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_send_async_update
 *
 * Description   : This function makes a request to MBCSV to send an async
 *                 update to the STANDBY NTFS for the record held at
 *                 the address i_reo_hdl.
 *
 * Arguments     : cb - A pointer to the ntfs control block.
 *                 ckpt_rec - pointer to the checkpoint record to be
 *                 sent as an async update.
 *                 action - type of async update to indiciate whether
 *                 this update is for addition, deletion or modification of
 *                 the record being sent.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : MBCSV, inturn calls our encode callback for this async
 *                 update. We use the reo_hdl in the encode callback to
 *                 retrieve the record for encoding the same.
 *****************************************************************************/

uint32_t ntfs_send_async_update(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *ckpt_rec, uint32_t action)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCS_MBCSV_ARG mbcsv_arg;
	TRACE_ENTER();
	/* Fill mbcsv specific data */
	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
	mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.send_ckpt.i_action = action;
	mbcsv_arg.info.send_ckpt.i_ckpt_hdl = (NCS_MBCSV_CKPT_HDL)cb->mbcsv_ckpt_hdl;
	mbcsv_arg.info.send_ckpt.i_reo_hdl = NCS_PTR_TO_UNS64_CAST(ckpt_rec);	/*Will be used in encode callback */

	/* Just store the address of the data to be send as an 
	 * async update record in reo_hdl. The same shall then be 
	 *dereferenced during encode callback */

	mbcsv_arg.info.send_ckpt.i_reo_type = ckpt_rec->header.ckpt_rec_type;
	mbcsv_arg.info.send_ckpt.i_send_type = NCS_MBCSV_SND_SYNC;

	/* Send async update */
	if (NCSCC_RC_SUCCESS != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		LOG_ER("MBCSV send data operation !! rc=%u.", rc);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}	/*End send_async_update() */

/****************************************************************************
 * Name          : ckpt_peer_info_cbk_handler 
 *
 * Description   : This callback is invoked by mbcsv when a peer info message
 *                 is received from NTFS STANDBY. 
 *
 * Arguments     : NCS_MBCSV_ARG containing info pertaining to the STANDBY.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/
static uint32_t ckpt_peer_info_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{
	uint16_t peer_version;
	osafassert(arg != NULL);

	peer_version = arg->info.peer.i_peer_version;
	if (peer_version < NTFS_MBCSV_VERSION_MIN) {
		TRACE("peer_version not correct!!\n");
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_notify_cbk_handler 
 *
 * Description   : This callback is invoked by mbcsv when a notify message
 *                 is received from NTFS STANDBY. 
 *
 * Arguments     : NCS_MBCSV_ARG - contains notification info from STANDBY.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/
static uint32_t ckpt_notify_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{
	/* Currently nothing to be done */
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_err_ind_cbk_handler 
 *
 * Description   : This callback is invoked by mbcsv when a notify message
 *                 is received from NTFS STANDBY. 
 *
 *
 * Arguments     : NCS_MBCSV_ARG - contains notification info from STANDBY.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/
static uint32_t ckpt_err_ind_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{
	/* Currently nothing to be done. */
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : edp_ed_finalize_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 ntfsv checkpoint finalize async updates record.
 * 
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode. 
 *                 EDU_ERR - out param to indicate errors in processing. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t edp_ed_finalize_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uint32_t *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_ckpt_finalize_msg_t *ckpt_final_msg_ptr = NULL, **ckpt_final_msg_dec_ptr;

	EDU_INST_SET ckpt_final_rec_ed_rules[] = {
		{EDU_START, edp_ed_finalize_rec, 0, 0, 0, sizeof(ntfsv_ckpt_finalize_msg_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((ntfsv_ckpt_finalize_msg_t *)0)->client_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_final_msg_ptr = (ntfsv_ckpt_finalize_msg_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_final_msg_dec_ptr = (ntfsv_ckpt_finalize_msg_t **)ptr;
		if (*ckpt_final_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_final_msg_dec_ptr, '\0', sizeof(ntfsv_ckpt_finalize_msg_t));
		ckpt_final_msg_ptr = *ckpt_final_msg_dec_ptr;
	} else {
		ckpt_final_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_final_rec_ed_rules, ckpt_final_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End edp_ed_finalize_rec() */

/****************************************************************************
 * Name          : edp_ed_header_rec 
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 ntfsv checkpoint message header record.
 * 
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode. 
 *                 EDU_ERR - out param to indicate errors in processing. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t edp_ed_header_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			       NCSCONTEXT ptr, uint32_t *ptr_data_len,
			       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_ckpt_header_t *ckpt_header_ptr = NULL, **ckpt_header_dec_ptr;

	EDU_INST_SET ckpt_header_rec_ed_rules[] = {
		{EDU_START, edp_ed_header_rec, 0, 0, 0, sizeof(ntfsv_ckpt_header_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((ntfsv_ckpt_header_t *)0)->ckpt_rec_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((ntfsv_ckpt_header_t *)0)->num_ckpt_records, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((ntfsv_ckpt_header_t *)0)->data_len, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_header_ptr = (ntfsv_ckpt_header_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_header_dec_ptr = (ntfsv_ckpt_header_t **)ptr;
		if (*ckpt_header_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_header_dec_ptr, '\0', sizeof(ntfsv_ckpt_header_t));
		ckpt_header_ptr = *ckpt_header_dec_ptr;
	} else {
		ckpt_header_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_header_rec_ed_rules, ckpt_header_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End edp_ed_header_rec() */

/* Non EDU routines */

uint32_t enc_ckpt_reserv_header(NCS_UBAID *uba, ntfsv_ckpt_msg_type_t type, uint32_t num_rec, uint32_t len)
{
	ntfsv_ckpt_header_t ckpt_hdr;
	uint8_t *pheader = NULL;
	unsigned int ckp_size = sizeof(ntfsv_ckpt_header_t);
	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(ntfsv_ckpt_header_t));
	if (pheader == NULL) {
		TRACE("  ncs_enc_reserve_space FAILED");
		return 0;
	}
	ncs_enc_claim_space(uba, sizeof(ntfsv_ckpt_header_t));
	/* Encode Header */
	ckpt_hdr.ckpt_rec_type = type;
	ckpt_hdr.num_ckpt_records = num_rec;
	ckpt_hdr.data_len = len;
	TRACE_2("ckpt_rec_type: %u", ckpt_hdr.ckpt_rec_type);
	TRACE_2("ckpt_size: %u", ckp_size);
	enc_ckpt_header(pheader, ckpt_hdr);
	return 1;
}

/****************************************************************************
 * Name          : ntfs_enc_ckpt_header
 *
 * Description   : This function encodes the checkpoint message header
 *                 using leap provided apis. 
 *
 * Arguments     : pdata - pointer to the buffer to encode this struct in. 
 *                 NTFS_CKPT_HEADER - ntfsv checkpoint message header. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static void enc_ckpt_header(uint8_t *pdata, ntfsv_ckpt_header_t header)
{
	ncs_encode_32bit(&pdata, header.ckpt_rec_type);
	ncs_encode_32bit(&pdata, header.num_ckpt_records);
	ncs_encode_32bit(&pdata, header.data_len);
}

/****************************************************************************
 * Name          : ntfs_dec_ckpt_header
 *
 * Description   : This function decodes the checkpoint message header
 *                 using leap provided apis. 
 *
 * Arguments     : NCS_UBAID - pointer to the NCS_UBAID containing data.
 *                 NTFS_CKPT_HEADER - ntfsv checkpoint message header. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t dec_ckpt_header(NCS_UBAID *uba, ntfsv_ckpt_header_t *header)
{
	uint8_t *p8;
	uint8_t local_data[4];
	TRACE_ENTER();
	if ((uba == NULL) || (header == NULL)) {
		TRACE("NULL pointer, FAILED");
		return NCSCC_RC_FAILURE;
	}
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	header->ckpt_rec_type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	header->num_ckpt_records = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	header->data_len = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}	/*End ntfs_dec_ckpt_header */

static void ntfs_exit(const char *msg, SaAmfRecommendedRecoveryT rec_rcvr)
{
	LOG_ER("Exiting with message: %s", msg);
	(void)saAmfComponentErrorReport(ntfs_cb->amf_hdl, &ntfs_cb->comp_name, 0, rec_rcvr, SA_NTF_IDENTIFIER_UNUSED);
	exit(EXIT_FAILURE);
}
