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

#include "lgs.h"
#include "ncssysf_def.h"
#include "ncssysf_mem.h"

/*
LGS_CKPT_DATA_HEADER
       4                4               4                 2            
-----------------------------------------------------------------
| ckpt_rec_type | num_ckpt_records | tot_data_len |  checksum   | 
-----------------------------------------------------------------

LGSV_CKPT_COLD_SYNC_MSG
-----------------------------------------------------------------------------------------------------------------------
| LGS_CKPT_DATA_HEADER|LGSV_CKPT_REC 1st| next |LGSV_CKPT_REC 2nd| next ..|..|..|..|LGSV_CKPT_REC "num_ckpt_records" th |
-----------------------------------------------------------------------------------------------------------------------
*/

static uint32_t edp_ed_stream_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uint32_t *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
static uint32_t edp_ed_reg_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			    NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
static uint32_t edp_ed_write_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uint32_t *ptr_data_len,
			      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
static uint32_t edp_ed_open_stream_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				    NCSCONTEXT ptr, uint32_t *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t edp_ed_finalize_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uint32_t *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t edp_ed_cfg_stream_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op,
				   EDU_ERR *o_err);

static uint32_t edp_ed_header_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			       NCSCONTEXT ptr, uint32_t *ptr_data_len,
			       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
static uint32_t edp_ed_ckpt_msg(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			     NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t ckpt_proc_initialize_client(lgs_cb_t *cb, lgsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_finalize_client(lgs_cb_t *cb, lgsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_agent_down(lgs_cb_t *cb, lgsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_log_write(lgs_cb_t *cb, lgsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_open_stream(lgs_cb_t *cb, lgsv_ckpt_msg_t *data);
static void enc_ckpt_header(uint8_t *pdata, lgsv_ckpt_header_t header);
static uint32_t dec_ckpt_header(NCS_UBAID *uba, lgsv_ckpt_header_t *header);
static uint32_t ckpt_decode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t mbcsv_callback(NCS_MBCSV_CB_ARG *arg);	/* Common Callback interface to mbcsv */
static uint32_t ckpt_decode_async_update(lgs_cb_t *cb, NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t ckpt_proc_close_stream(lgs_cb_t *cb, lgsv_ckpt_msg_t *data);
static uint32_t ckpt_proc_cfg_stream(lgs_cb_t *cb, lgsv_ckpt_msg_t *data);
static uint32_t edp_ed_close_stream_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uint32_t *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t ckpt_encode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t ckpt_enc_cold_sync_data(lgs_cb_t *lgs_cb, NCS_MBCSV_CB_ARG *cbk_arg, bool data_req);
static uint32_t ckpt_encode_async_update(lgs_cb_t *lgs_cb, EDU_HDL edu_hdl, NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t ckpt_decode_cold_sync(lgs_cb_t *cb, NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t ckpt_peer_info_cbk_handler(NCS_MBCSV_CB_ARG *arg);
static uint32_t ckpt_notify_cbk_handler(NCS_MBCSV_CB_ARG *arg);
static uint32_t ckpt_err_ind_cbk_handler(NCS_MBCSV_CB_ARG *arg);
static int32_t ckpt_msg_test_type(NCSCONTEXT arg);

static uint32_t edu_enc_reg_list(lgs_cb_t *cb, NCS_UBAID *uba);
static uint32_t edu_enc_streams(lgs_cb_t *cb, NCS_UBAID *uba);
static uint32_t process_ckpt_data(lgs_cb_t *cb, lgsv_ckpt_msg_t *data);

static LGS_CKPT_HDLR ckpt_data_handler[] = {
	ckpt_proc_initialize_client,
	ckpt_proc_finalize_client,
	ckpt_proc_agent_down,
	ckpt_proc_log_write,
	ckpt_proc_open_stream,
	ckpt_proc_close_stream,
	ckpt_proc_cfg_stream
};

static void free_edu_mem(char *ptr)
{
	m_NCS_MEM_FREE(ptr, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0);
}

/****************************************************************************
 * Name          : lgsv_mbcsv_init 
 *
 * Description   : This function initializes the mbcsv interface and
 *                 obtains a selection object from mbcsv.
 *                 
 * Arguments     : LGS_CB * - A pointer to the lgs control block.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t lgs_mbcsv_init(lgs_cb_t *cb)
{
	uint32_t rc;
	NCS_MBCSV_ARG arg;

	TRACE_ENTER();

	/* Initialize with MBCSv library */
	arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	arg.info.initialize.i_mbcsv_cb = mbcsv_callback;
	arg.info.initialize.i_version = LGS_MBCSV_VERSION;
	arg.info.initialize.i_service = NCS_SERVICE_ID_LGS;

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

	rc = lgs_mbcsv_change_HA_state(cb);

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : lgs_mbcsv_change_HA_state 
 *
 * Description   : This function inform mbcsv of our HA state. 
 *                 All checkpointing operations are triggered based on the 
 *                 state.
 *
 * Arguments     : LGS_CB * - A pointer to the lgs control block.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : This function should be ideally called only once, i.e.
 *                 during the first CSI assignment from AVSv  .
 *****************************************************************************/

uint32_t lgs_mbcsv_change_HA_state(lgs_cb_t *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;
	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	/* Set the mbcsv args */
	mbcsv_arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.chg_role.i_ckpt_hdl = cb->mbcsv_ckpt_hdl;
	mbcsv_arg.info.chg_role.i_ha_state = cb->ha_state;

	if (ncs_mbcsv_svc(&mbcsv_arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_mbcsv_svc NCS_MBCSV_OP_CHG_ROLE FAILED");
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;

}	/*End lgs_mbcsv_change_HA_state */

/****************************************************************************
 * Name          : lgs_mbcsv_change_HA_state 
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
uint32_t lgs_mbcsv_dispatch(NCS_MBCSV_HDL mbcsv_hdl)
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
 *                 notify lgs of all checkpointing operations. 
 *
 * Arguments     : NCS_MBCSV_CB_ARG - Callback Info pertaining to the mbcsv
 *                 event from ACTIVE/STANDBY LGS peer.
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
		/* LGS Peer info from MBCSv */
		rc = ckpt_peer_info_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("ckpt_peer_info_cbk_handler FAILED");
		break;
	case NCS_MBCSV_CBOP_NOTIFY:
		/* NOTIFY info from LGS peer */
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
					    LGS_MBCSV_VERSION, LGS_MBCSV_VERSION_MIN);
	if (0 == mbcsv_version) {
		TRACE("Wrong mbcsv_version!!!\n");
		return NCSCC_RC_FAILURE;
	}

	switch (cbk_arg->info.encode.io_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		/* Encode async update */
		if ((rc = ckpt_encode_async_update(lgs_cb, lgs_cb->edu_hdl, cbk_arg)) != NCSCC_RC_SUCCESS)
			TRACE("  ckpt_encode_async_update FAILED");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		TRACE_2("COLD SYNC REQ ENCODE CALLED");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		/* Encode cold sync response */
		rc = ckpt_enc_cold_sync_data(lgs_cb, cbk_arg, false);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE(" COLD SYNC ENCODE FAIL....");
		} else {
			lgs_cb->ckpt_state = COLD_SYNC_COMPLETE;
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
		if ((rc = ckpt_enc_cold_sync_data(lgs_cb, cbk_arg, true)) != NCSCC_RC_SUCCESS)
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
 *                 1.REGLIST
 *                 2.OPEN STREAMS
 *                 3.Async Update Count
 *                 in that order.
 *                 Each records contain a header specifying the record type
 *                 and number of such records.
 *
 * Arguments     : lgs_cb - pointer to the lgs control block. 
 *                 cbk_arg - Pointer to NCS_MBCSV_CB_ARG with encode info.
 *                 data_req - Flag to specify if its for cold sync or data
 *                 request for warm sync.
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ckpt_enc_cold_sync_data(lgs_cb_t *lgs_cb, NCS_MBCSV_CB_ARG *cbk_arg, bool data_req)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	/* asynsc Update Count */
	uint8_t *async_upd_cnt = NULL;

	/* Currently, we shall send all data in one send.
	 * This shall avoid "delta data" problems that are associated during
	 * multiple sends
	 */
	TRACE_2("COLD SYNC ENCODE START........");
	/* Encode Registration list */
	rc = edu_enc_reg_list(lgs_cb, &cbk_arg->info.encode.io_uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("  edu_enc_reg_list FAILED");
		return NCSCC_RC_FAILURE;
	}
	rc = edu_enc_streams(lgs_cb, &cbk_arg->info.encode.io_uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("  edu_enc_streams FAILED");
		return NCSCC_RC_FAILURE;
	}
	/* Encode the Async Update Count at standby */

	/* This will have the count of async updates that have been sent,
	   this will be 0 initially */
	async_upd_cnt = ncs_enc_reserve_space(&cbk_arg->info.encode.io_uba, sizeof(uint32_t));
	if (async_upd_cnt == NULL) {
		/* Log this error */
		TRACE("  ncs_enc_reserve_space FAILED");
		return NCSCC_RC_FAILURE;
	}

	ncs_encode_32bit(&async_upd_cnt, lgs_cb->async_upd_cnt);
	ncs_enc_claim_space(&cbk_arg->info.encode.io_uba, sizeof(uint32_t));

	/* Set response mbcsv msg type to complete */
	if (data_req == true)
		cbk_arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
	else
		cbk_arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
	TRACE_2("COLD SYNC ENCODE END........");
	return rc;
}	/*End  ckpt_enc_cold_sync_data() */

static uint32_t ckpt_stream_open_set(log_stream_t *logStream, lgs_ckpt_stream_open_t *stream_open)
{
	memset(stream_open, 0, sizeof(lgs_ckpt_stream_open_t));
	stream_open->clientId = -1;	/* not used in this message */
	stream_open->streamId = logStream->streamId;
	stream_open->logFile = logStream->fileName;
	stream_open->logPath = logStream->pathName;
	stream_open->logFileCurrent = logStream->logFileCurrent;
	stream_open->fileFmt = logStream->logFileFormat;
	stream_open->logStreamName = (char *)logStream->name;
	stream_open->maxFileSize = logStream->maxLogFileSize;
	stream_open->maxLogRecordSize = logStream->fixedLogRecordSize;
	stream_open->logFileFullAction = logStream->logFullAction;
	stream_open->maxFilesRotated = logStream->maxFilesRotated;
	stream_open->creationTimeStamp = logStream->creationTimeStamp;
	stream_open->numOpeners = logStream->numOpeners;
	stream_open->streamType = logStream->streamType;
	stream_open->logRecordId = logStream->logRecordId;

	return NCSCC_RC_SUCCESS;
}

static uint32_t edu_enc_streams(lgs_cb_t *cb, NCS_UBAID *uba)
{
	log_stream_t *log_stream_rec = NULL;
	lgs_ckpt_stream_open_t *ckpt_stream_rec;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS, num_rec = 0;
	uint8_t *pheader = NULL;
	lgsv_ckpt_header_t ckpt_hdr;

	/* Prepare reg. structure to encode */
	ckpt_stream_rec = malloc(sizeof(lgs_ckpt_stream_open_t));
	if (ckpt_stream_rec == NULL) {
		LOG_WA("malloc FAILED");
		return (NCSCC_RC_FAILURE);
	}

	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(lgsv_ckpt_header_t));
	if (pheader == NULL) {
		TRACE("ncs_enc_reserve_space FAILED");
		free(ckpt_stream_rec);
		return (rc = EDU_ERR_MEM_FAIL);
	}
	ncs_enc_claim_space(uba, sizeof(lgsv_ckpt_header_t));
	log_stream_rec = log_stream_getnext_by_name(NULL);

	/* Walk through the reg list and encode record by record */
	while (log_stream_rec != NULL) {
		ckpt_stream_open_set(log_stream_rec, ckpt_stream_rec);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl,
				    edp_ed_open_stream_rec, uba, EDP_OP_TYPE_ENC, ckpt_stream_rec, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			TRACE("m_NCS_EDU_EXEC FAILED");
			free(ckpt_stream_rec);
			return rc;
		}
		++num_rec;
		log_stream_rec = log_stream_getnext_by_name(log_stream_rec->name);
	}			/* End while RegRec */

	/* Encode RegHeader */
	ckpt_hdr.ckpt_rec_type = LGS_CKPT_OPEN_STREAM;
	ckpt_hdr.num_ckpt_records = num_rec;
	ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */

	enc_ckpt_header(pheader, ckpt_hdr);

	free(ckpt_stream_rec);
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 * Name          : edu_enc_reg_list
 *
 * Description   : This function walks through the reglist and encodes all
 *                 records in the reglist, using the edps defined for the
 *                 same.
 *
 * Arguments     : cb - Pointer to LGS control block.
 *                 uba - Pointer to ubaid provided by mbcsv 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/

static uint32_t edu_enc_reg_list(lgs_cb_t *cb, NCS_UBAID *uba)
{
	log_client_t *client = NULL;
	lgs_ckpt_initialize_msg_t *ckpt_reg_rec;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS, num_rec = 0;
	uint8_t *pheader = NULL;
	lgsv_ckpt_header_t ckpt_hdr;

	TRACE_ENTER();
	/* Prepare reg. structure to encode */
	ckpt_reg_rec = malloc(sizeof(lgs_ckpt_initialize_msg_t));
	if (ckpt_reg_rec == NULL) {
		LOG_WA("malloc FAILED");
		return (NCSCC_RC_FAILURE);
	}

	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(lgsv_ckpt_header_t));
	if (pheader == NULL) {
		TRACE("  ncs_enc_reserve_space FAILED");
		free(ckpt_reg_rec);
		return (rc = EDU_ERR_MEM_FAIL);
	}
	ncs_enc_claim_space(uba, sizeof(lgsv_ckpt_header_t));

	client = (log_client_t *)ncs_patricia_tree_getnext(&cb->client_tree, (uint8_t *)0);

	/* Walk through the reg list and encode record by record */
	while (client != NULL) {
		ckpt_reg_rec->client_id = client->client_id;
		ckpt_reg_rec->mds_dest = client->mds_dest;
		ckpt_reg_rec->stream_list = client->stream_list_root;

		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, edp_ed_reg_rec, uba, EDP_OP_TYPE_ENC, ckpt_reg_rec, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			TRACE("  m_NCS_EDU_EXEC FAILED");
			free(ckpt_reg_rec);
			return rc;
		}
		++num_rec;

		/* length+=lgs_edp_ed_reg_rec(reg_rec,o_ub); */
		client = (log_client_t *)ncs_patricia_tree_getnext(&cb->client_tree, (uint8_t *)&client->client_id_net);
	}			/* End while RegRec */

	/* Encode RegHeader */
	ckpt_hdr.ckpt_rec_type = LGS_CKPT_CLIENT_INITIALIZE;
	ckpt_hdr.num_ckpt_records = num_rec;
	ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */

	enc_ckpt_header(pheader, ckpt_hdr);

	free(ckpt_reg_rec);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}	/* End edu_enc_reg_list() */

/****************************************************************************
 * Name          : ckpt_encode_async_update
 *
 * Description   : This function encodes data to be sent as an async update.
 *                 The caller of this function would set the address of the
 *                 record to be encoded in the reo_hdl field(while invoking
 *                 SEND_CKPT option of ncs_mbcsv_svc. 
 *
 * Arguments     : cb - pointer to the LGS control block.
 *                 cbk_arg - Pointer to NCS MBCSV callback argument struct.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_encode_async_update(lgs_cb_t *lgs_cb, EDU_HDL edu_hdl, NCS_MBCSV_CB_ARG *cbk_arg)
{
	lgsv_ckpt_msg_t *data = NULL;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	/* Set reo_hdl from callback arg to ckpt_rec */
	data = (lgsv_ckpt_msg_t *)(long)cbk_arg->info.encode.io_reo_hdl;
	if (data == NULL) {
		TRACE("   data == NULL, FAILED");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	/* Encode async record,except publish & subscribe */
	rc = m_NCS_EDU_EXEC(&edu_hdl, edp_ed_ckpt_msg, &cbk_arg->info.encode.io_uba, EDP_OP_TYPE_ENC, data, &ederror);

	if (rc != NCSCC_RC_SUCCESS) {
		m_NCS_EDU_PRINT_ERROR_STRING(ederror);
		/* free(data); FIX ??? */
		TRACE_2("eduerr: %x", ederror);
		TRACE_LEAVE();
		return rc;
	}

	/* Update the Async Update Count at standby */
	lgs_cb->async_upd_cnt++;
	/* free (data); FIX??? */
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
					      LGS_MBCSV_VERSION, LGS_MBCSV_VERSION_MIN);
	if (0 == msg_fmt_version) {
		TRACE("wrong msg_fmt_version!!!\n");
		return NCSCC_RC_FAILURE;
	}

	switch (cbk_arg->info.decode.i_msg_type) {
	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		TRACE_2(" COLD SYNC REQ DECODE called");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
		TRACE_2(" COLD SYNC RESP DECODE called");
		if (lgs_cb->ckpt_state != COLD_SYNC_COMPLETE) {	/*this check is needed to handle repeated requests */
			if ((rc = ckpt_decode_cold_sync(lgs_cb, cbk_arg)) != NCSCC_RC_SUCCESS) {
				TRACE(" COLD SYNC RESPONSE DECODE ....");
			} else {
				TRACE_2(" COLD SYNC RESPONSE DECODE SUCCESS....");
				lgs_cb->ckpt_state = COLD_SYNC_COMPLETE;
			}
		}
		break;

	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		TRACE_2(" ASYNC UPDATE DECODE called");
		if ((rc = ckpt_decode_async_update(lgs_cb, cbk_arg)) != NCSCC_RC_SUCCESS)
			TRACE("ckpt_decode_async_update FAILED %u", rc);
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
		if ((rc = ckpt_decode_cold_sync(lgs_cb, cbk_arg)) != NCSCC_RC_SUCCESS)
			TRACE("   FAILED");
		break;

	default:
		TRACE_2(" INCORRECT DECODE called");
		rc = NCSCC_RC_FAILURE;
		TRACE("  INCORRECT DECODE called, FAILED");
		m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
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
 *                 cb - pointer to lgs cb.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ckpt_decode_async_update(lgs_cb_t *cb, NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror;
	lgsv_ckpt_msg_t msg;
	lgsv_ckpt_msg_t *ckpt_msg = &msg;
	lgsv_ckpt_header_t *hdr;
	lgs_ckpt_initialize_msg_t *reg_rec;
	lgs_ckpt_write_log_t *writelog;
	lgs_ckpt_stream_open_t *stream;
	lgs_ckpt_finalize_msg_t *finalize;
	lgs_ckpt_stream_cfg_t *cfg;
	MDS_DEST *agent_dest;

	TRACE_ENTER();

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
	case LGS_CKPT_CLIENT_INITIALIZE:
		TRACE_2("INITIALIZE REC: AUPDATE");
		reg_rec = &ckpt_msg->ckpt_rec.initialize_client;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, edp_ed_reg_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &reg_rec, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("   FAILED");
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			goto done;
		}
		break;
	case LGS_CKPT_LOG_WRITE:
		TRACE_2("WRITE LOG: AUPDATE");
		writelog = &ckpt_msg->ckpt_rec.write_log;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, edp_ed_write_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &writelog, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("   write log FAILED");
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			goto done;
		}
		break;

	case LGS_CKPT_OPEN_STREAM:
		TRACE_2("STREAM OPEN: AUPDATE");
		stream = &ckpt_msg->ckpt_rec.stream_open;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, edp_ed_open_stream_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &stream, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("STREAM OPEN FAILED");
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			goto done;
		}
		break;

	case LGS_CKPT_CLOSE_STREAM:
		TRACE_2("STREAM CLOSE: AUPDATE");
		stream = &ckpt_msg->ckpt_rec.stream_open;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, edp_ed_close_stream_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &stream, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("STREAM CLOSE FAILED");
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			goto done;
		}
		break;

	case LGS_CKPT_CLIENT_FINALIZE:
		TRACE_2("FINALIZE REC: AUPDATE");
		reg_rec = &ckpt_msg->ckpt_rec.initialize_client;
		finalize = &ckpt_msg->ckpt_rec.finalize_client;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, edp_ed_finalize_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &finalize, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("   FAILED");
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			goto done;
		}
		break;
	case LGS_CKPT_CLIENT_DOWN:
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
	case LGS_CKPT_CFG_STREAM:
		TRACE_2("CFG STREAM REC: AUPDATE");
		cfg = &ckpt_msg->ckpt_rec.stream_cfg;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, edp_ed_cfg_stream_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &cfg, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("CFG UPDATE FAILED");
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

	rc = process_ckpt_data(cb, ckpt_msg);
	/* Update the Async Update Count at standby */
	cb->async_upd_cnt++;
 done:
	TRACE_LEAVE();
	return rc;
	/* if failure, should an indication be sent to active ? */
}

/****************************************************************************
 * Name          : ckpt_decode_cold_sync 
 *
 * Description   : This function decodes async update data, based on the 
 *                 record type contained in the header. 
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with decode info
 *                 cb - pointer to lgs cb.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : COLD SYNC RECORDS are expected in an order
 *                 1. REG RECORDS
 *                 2. OPEN STREAMS RECORDS
 *                 
 *                 For each record type,
 *                     a) decode header.
 *                     b) decode individual records for 
 *                        header->num_records times, 
 *****************************************************************************/

static uint32_t ckpt_decode_cold_sync(lgs_cb_t *cb, NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	lgsv_ckpt_msg_t msg;
	lgsv_ckpt_msg_t *data = &msg;
	uint32_t num_rec = 0;
	lgs_ckpt_initialize_msg_t *reg_rec = NULL;
	lgs_ckpt_stream_open_t *stream_rec = NULL;
	uint32_t num_of_async_upd;
	uint8_t *ptr;
	uint8_t data_cnt[16];
	TRACE_ENTER();

	/* 
	   -------------------------------------------------
	   | Header|RegRecords1..n|Header|streamRecords1..n|
	   -------------------------------------------------
	 */

	TRACE_2("COLD SYNC DECODE START........");

	/* Decode the current message header */
	if ((rc = dec_ckpt_header(&cbk_arg->info.decode.i_uba, &data->header)) != NCSCC_RC_SUCCESS) {
		goto done;
	}
	/* Check if the first in the order of records is reg record */
	if (data->header.ckpt_rec_type != LGS_CKPT_CLIENT_INITIALIZE) {
		TRACE("FAILED data->header.ckpt_rec_type != LGS_CKPT_INITIALIZE_REC");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Process the reg_records */
	num_rec = data->header.num_ckpt_records;
	TRACE("regid: num_rec = %u", num_rec);
	while (num_rec) {
		reg_rec = &data->ckpt_rec.initialize_client;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, edp_ed_reg_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &reg_rec, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("FAILED: COLD SYNC DECODE REG REC");
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
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

	if ((rc = dec_ckpt_header(&cbk_arg->info.decode.i_uba, &data->header)) != NCSCC_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
		TRACE("lgs_dec_ckpt_header FAILED");
		goto done;
	}

	/* Check if record type is open_stream */
	if (data->header.ckpt_rec_type != LGS_CKPT_OPEN_STREAM) {
		rc = NCSCC_RC_FAILURE;
		TRACE("FAILED: LGS_CKPT_OPEN_STREAM type is expected, got %u", data->header.ckpt_rec_type);
		goto done;
	}

	/* Process the stream records */
	num_rec = data->header.num_ckpt_records;
	TRACE("opens_streams: num_rec = %u", num_rec);
	while (num_rec) {
		stream_rec = &data->ckpt_rec.stream_open;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, edp_ed_open_stream_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &stream_rec, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("FAILED: COLD SYNC DECODE STREAM REC");
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			goto done;
		}
		/* Update our database */
		rc = process_ckpt_data(cb, data);

		if (rc != NCSCC_RC_SUCCESS) {
			goto done;
		}
		memset(&data->ckpt_rec, 0, sizeof(data->ckpt_rec));
		--num_rec;
	}			/*End while, stream records */

	/* Get the async update count */
	ptr = ncs_dec_flatten_space(&cbk_arg->info.decode.i_uba, data_cnt, sizeof(uint32_t));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	cb->async_upd_cnt = num_of_async_upd;
	ncs_dec_skip_space(&cbk_arg->info.decode.i_uba, 4);

	/* If we reached here, we are through. Good enough for coldsync with ACTIVE */
	TRACE_2("COLD SYNC DECODE END........");

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		/* Do not allow standby to get out of sync */
		lgs_exit("Cold sync failed", SA_AMF_COMPONENT_RESTART);
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : process_ckpt_data
 *
 * Description   : This function updates the lgs internal databases
 *                 based on the data type. 
 *
 * Arguments     : cb - pointer to LGS ControlBlock. 
 *                 data - pointer to  LGS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t process_ckpt_data(lgs_cb_t *cb, lgsv_ckpt_msg_t *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	if ((!cb) || (data == NULL)) {
		TRACE("FAILED: (!cb) || (data == NULL)");
		return (rc = NCSCC_RC_FAILURE);
	}

	if ((cb->ha_state == SA_AMF_HA_STANDBY) || (cb->ha_state == SA_AMF_HA_QUIESCED)) {
		if (data->header.ckpt_rec_type >= LGS_CKPT_MSG_MAX) {
			TRACE("FAILED: data->header.ckpt_rec_type >= LGS_CKPT_MSG_MAX");
			return NCSCC_RC_FAILURE;
		}
		/* Update the internal database */
		rc = ckpt_data_handler[data->header.ckpt_rec_type] (cb, data);
		return rc;
	} else {
		return (rc = NCSCC_RC_FAILURE);
	}
}	/*End lgs_process_ckpt_data() */

/****************************************************************************
 * Name          : ckpt_proc_initialize_client
 *
 * Description   : This function updates the lgs reglist based on the 
 *                 info received from the ACTIVE lgs peer.
 *
 * Arguments     : cb - pointer to LGS  ControlBlock.
 *                 data - pointer to  LGS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_initialize_client(lgs_cb_t *cb, lgsv_ckpt_msg_t *data)
{
	lgs_ckpt_initialize_msg_t *param = &data->ckpt_rec.initialize_client;
	log_client_t *client;

	TRACE_ENTER2("client ID: %d", param->client_id);

	client = lgs_client_get_by_id(param->client_id);
	if (client == NULL) {
		/* Client does not exist, create new one */
		if ((client = lgs_client_new(param->mds_dest, param->client_id, param->stream_list)) == NULL) {
			/* Do not allow standby to get out of sync */
			lgs_exit("Could not create new client", SA_AMF_COMPONENT_RESTART);
		}
	} else {
		/* Client with ID already exist, check other attributes */
		if (client->mds_dest != param->mds_dest) {
			/* Do not allow standby to get out of sync */
			lgs_exit("Client attributes differ", SA_AMF_COMPONENT_RESTART);
		}
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_log_write
 *
 * Description   : This function updates the lgs alarm logRecordId
 *                 received from the ACTIVE lgs peer.
 *                 
 *
 * Arguments     : cb - pointer to LGS  ControlBlock.
 *                 data - pointer to  LGS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_log_write(lgs_cb_t *cb, lgsv_ckpt_msg_t *data)
{
	lgs_ckpt_write_log_t *param = &data->ckpt_rec.write_log;
	log_stream_t *stream;

	TRACE_ENTER();

	stream = log_stream_get_by_id(param->streamId);
	if (stream == NULL) {
		TRACE("Could not lookup stream: %u", param->streamId);
		goto done;
	}

	stream->logRecordId = param->recordId;
	stream->curFileSize = param->curFileSize;
	strcpy(stream->logFileCurrent, param->logFileCurrent);

 done:
	free_edu_mem(param->logFileCurrent);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_close_stream
 *
 * Description   : This function close a stream.
 *
 * Arguments     : cb - pointer to LGS  ControlBlock.
 *                 data - pointer to  LGS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_close_stream(lgs_cb_t *cb, lgsv_ckpt_msg_t *data)
{
	lgs_ckpt_stream_close_t *param = &data->ckpt_rec.stream_close;
	log_stream_t *stream;

	TRACE_ENTER();

	if ((stream = log_stream_get_by_id(param->streamId)) == NULL) {
		TRACE("Could not lookup stream");
		goto done;
	}

	TRACE("close stream %s, id: %u", stream->name, stream->streamId);

	(void)lgs_client_stream_rmv(param->clientId, param->streamId);

	if (log_stream_close(&stream) != 0) {
		/* Do not allow standby to get out of sync */
		lgs_exit("Client attributes differ", SA_AMF_COMPONENT_RESTART);
	}

 done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_open_stream
 *
 * Description   : This function updates an existing stream or creates a new.
 *
 * Arguments     : cb - pointer to LGS  ControlBlock.
 *                data - pointer to  LGS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t ckpt_proc_open_stream(lgs_cb_t *cb, lgsv_ckpt_msg_t *data)
{
	lgs_ckpt_stream_open_t *param = &data->ckpt_rec.stream_open;
	log_stream_t *stream;

	TRACE_ENTER();

	/* Check that client still exist */
	if ((param->clientId != -1) && (lgs_client_get_by_id(param->clientId) == NULL)) {
		LOG_WA("Client %u does not exist, failed to create stream '%s'", param->clientId, param->logStreamName);
		goto done;
	}

	stream = log_stream_get_by_name(param->logStreamName);
	if (stream != NULL) {
		TRACE("existing stream - id %u", stream->streamId);
		/*
		 ** Update stream attributes that might change when a stream is 
		 ** opened a second time.
		 */
		stream->numOpeners = param->numOpeners;
	} else {
		SaNameT name;

		TRACE("New stream %s, id %u", param->logStreamName, param->streamId);
		strcpy((char *)name.value, param->logStreamName);
		name.length = strlen(param->logStreamName);

		stream = log_stream_new(&name, param->logFile, param->logPath, param->maxFileSize, param->maxLogRecordSize, param->logFileFullAction, param->maxFilesRotated, param->fileFmt, param->streamType, param->streamId, SA_FALSE,	// FIX sync or calculate?
					param->logRecordId);

		if (stream == NULL) {
			/* Do not allow standby to get out of sync */
			LOG_ER("Failed to create stream '%s'", param->logStreamName);
			lgs_exit("Could not create new stream", SA_AMF_COMPONENT_RESTART);
		}

		stream->numOpeners = param->numOpeners;
		stream->creationTimeStamp = param->creationTimeStamp;
		strcpy(stream->logFileCurrent, param->logFileCurrent);
	}

	log_stream_print(stream);

	/*
	 ** Create an association between this client_id and the stream
	 ** A client ID of -1 indicates that no client exist, skip this step.
	 */
	if ((param->clientId != -1) && lgs_client_stream_add(param->clientId, stream->streamId) != 0) {
		/* Do not allow standby to get out of sync */
		LOG_ER("Failed to add stream '%s' to client %u", param->logStreamName, param->clientId);
		lgs_exit("Could not add stream to client", SA_AMF_COMPONENT_RESTART);
	}

 done:
	/* Free strings allocated by the EDU encoder */
	free_edu_mem(param->logFile);
	free_edu_mem(param->logPath);
	free_edu_mem(param->fileFmt);
	free_edu_mem(param->logFileCurrent);
	free_edu_mem(param->logStreamName);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_finalize_client
 *
 * Description   : This function clears the lgs reglist and assosicated DB 
 *                 based on the info received from the ACTIVE lgs peer.
 *
 * Arguments     : cb - pointer to LGS ControlBlock.
 *                 data - pointer to  LGS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_finalize_client(lgs_cb_t *cb, lgsv_ckpt_msg_t *data)
{
	lgs_ckpt_finalize_msg_t *param = &data->ckpt_rec.finalize_client;

	TRACE_ENTER();

	/* Ensure all resources allocated by this registration are freed. */
	(void)lgs_client_delete(param->client_id);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_agent_down
 *
 * Description   : This function processes a agent down message 
 *                 received from the ACTIVE LGS peer.      
 *
 * Arguments     : cb - pointer to LGS ControlBlock.
 *                 data - pointer to  LGS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None 
 ****************************************************************************/

static uint32_t ckpt_proc_agent_down(lgs_cb_t *cb, lgsv_ckpt_msg_t *data)
{
	TRACE_ENTER();

	/*Remove the  LGA DOWN REC from the LGA_DOWN_LIST */
	/* Search for matching LGA in the LGA_DOWN_LIST  */
	lgs_remove_lga_down_rec(cb, data->ckpt_rec.agent_dest);

	/* Ensure all resources allocated by this registration are freed. */
	(void)lgs_client_delete_by_mds_dest(data->ckpt_rec.agent_dest);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_cfg_streamc
 *
 * Description   : This function configures a stream.
 *
 * Arguments     : cb - pointer to LGS  ControlBlock.
 *                 data - pointer to  LGS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_cfg_stream(lgs_cb_t *cb, lgsv_ckpt_msg_t *data)
{
	lgs_ckpt_stream_cfg_t *param = &data->ckpt_rec.stream_cfg;
	log_stream_t *stream;

	TRACE_ENTER();

	if ((stream = log_stream_get_by_name(param->name)) == NULL) {
		LOG_ER("Could not lookup stream");
		goto done;
	}

	TRACE("config stream %s, id: %u", stream->name, stream->streamId);
	strcpy(stream->fileName, param->fileName);
	stream->maxLogFileSize = param->maxLogFileSize;
	stream->fixedLogRecordSize = param->fixedLogRecordSize;
	stream->logFullAction = param->logFullAction;
	stream->logFullHaltThreshold = param->logFullHaltThreshold;
	stream->maxFilesRotated = param->maxFilesRotated;
	if (stream->logFileFormat != NULL) {
		free(stream->logFileFormat);
		stream->logFileFormat = strdup(param->logFileFormat);
	}        
	strcpy(stream->logFileFormat, param->logFileFormat);
	stream->severityFilter = param->severityFilter;
	strcpy(stream->logFileCurrent, param->logFileCurrent);

 done:
	/* Free strings allocated by the EDU encoder */
	free_edu_mem(param->fileName);
	free_edu_mem(param->pathName);
	free_edu_mem(param->logFileFormat);
	free_edu_mem(param->logFileCurrent);
	free_edu_mem(param->name);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : lgs_ckpt_send_async
 *
 * Description   : This function makes a request to MBCSV to send an async
 *                 update to the STANDBY LGS for the record held at
 *                 the address i_reo_hdl.
 *
 * Arguments     : cb - A pointer to the lgs control block.
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

uint32_t lgs_ckpt_send_async(lgs_cb_t *cb, lgsv_ckpt_msg_t *ckpt_rec, uint32_t action)
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
	mbcsv_arg.info.send_ckpt.i_send_type = NCS_MBCSV_SND_USR_ASYNC;

	/* Send async update */
	if (NCSCC_RC_SUCCESS != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		LOG_ER("MBCSV send FAILED rc=%u.", rc);
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
 *                 is received from LGS STANDBY. 
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
	if (peer_version < LGS_MBCSV_VERSION_MIN) {
		TRACE("peer_version not correct!!\n");
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_notify_cbk_handler 
 *
 * Description   : This callback is invoked by mbcsv when a notify message
 *                 is received from LGS STANDBY. 
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
 *                 is received from LGS STANDBY. 
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

uint32_t edp_ed_stream_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			 NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_stream_list_t *ckpt_stream_list_msg_ptr = NULL, **ckpt_stream_list_msg_dec_ptr;

	EDU_INST_SET ckpt_stream_list_ed_rules[] = {
		{EDU_START, edp_ed_stream_list, EDQ_LNKLIST, 0, 0, sizeof(lgs_stream_list_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_stream_list_t *)0)->stream_id, 0, NULL},
		{EDU_TEST_LL_PTR, edp_ed_stream_list, 0, 0, 0, (long)&((lgs_stream_list_t *)0)->next, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_stream_list_msg_ptr = (lgs_stream_list_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_stream_list_msg_dec_ptr = (lgs_stream_list_t **)ptr;
		if (*ckpt_stream_list_msg_dec_ptr == NULL) {
			*ckpt_stream_list_msg_dec_ptr = calloc(1, sizeof(lgs_stream_list_t));
			if (*ckpt_stream_list_msg_dec_ptr == NULL) {
				LOG_WA("calloc FAILED");
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		}
		memset(*ckpt_stream_list_msg_dec_ptr, '\0', sizeof(lgs_stream_list_t));
		ckpt_stream_list_msg_ptr = *ckpt_stream_list_msg_dec_ptr;
	} else {
		ckpt_stream_list_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_stream_list_ed_rules, ckpt_stream_list_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : edp_ed_reg_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint registration rec.
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

static uint32_t edp_ed_reg_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			    NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_initialize_msg_t *ckpt_reg_msg_ptr = NULL, **ckpt_reg_msg_dec_ptr;

	EDU_INST_SET ckpt_reg_rec_ed_rules[] = {
		{EDU_START, edp_ed_reg_rec, 0, 0, 0, sizeof(lgs_ckpt_initialize_msg_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_initialize_msg_t *)0)->client_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((lgs_ckpt_initialize_msg_t *)0)->mds_dest, 0, NULL},
		{EDU_EXEC, edp_ed_stream_list, EDQ_POINTER, 0, 0, (long)&((lgs_ckpt_initialize_msg_t *)0)->stream_list,
		 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_reg_msg_ptr = (lgs_ckpt_initialize_msg_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_reg_msg_dec_ptr = (lgs_ckpt_initialize_msg_t **)ptr;

		if (*ckpt_reg_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_reg_msg_dec_ptr, '\0', sizeof(lgs_ckpt_initialize_msg_t));
		ckpt_reg_msg_ptr = *ckpt_reg_msg_dec_ptr;
	} else {
		ckpt_reg_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_reg_rec_ed_rules, ckpt_reg_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End edp_ed_reg_rec */

/****************************************************************************
 * Name          : edp_ed_write_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint write log rec.
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

static uint32_t edp_ed_write_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_write_log_t *ckpt_write_msg_ptr = NULL, **ckpt_write_msg_dec_ptr;

	EDU_INST_SET ckpt_write_rec_ed_rules[] = {
		{EDU_START, edp_ed_write_rec, 0, 0, 0, sizeof(lgs_ckpt_write_log_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_write_log_t *)0)->recordId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_write_log_t *)0)->streamId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_write_log_t *)0)->curFileSize, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_write_log_t *)0)->logFileCurrent, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_write_msg_ptr = (lgs_ckpt_write_log_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_write_msg_dec_ptr = (lgs_ckpt_write_log_t **)ptr;
		if (*ckpt_write_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_write_msg_dec_ptr, '\0', sizeof(lgs_ckpt_write_log_t));
		ckpt_write_msg_ptr = *ckpt_write_msg_dec_ptr;
	} else {
		ckpt_write_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_write_rec_ed_rules, ckpt_write_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End edp_ed_write_rec */

/****************************************************************************
 * Name          : edp_ed_open_stream_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint open_stream log rec.
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

static uint32_t edp_ed_open_stream_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				    NCSCONTEXT ptr, uint32_t *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_stream_open_t *ckpt_open_stream_msg_ptr = NULL, **ckpt_open_stream_msg_dec_ptr;

	EDU_INST_SET ckpt_open_stream_rec_ed_rules[] = {
		{EDU_START, edp_ed_open_stream_rec, 0, 0, 0, sizeof(lgs_ckpt_stream_open_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->streamId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->clientId, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logFile, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logPath, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logFileCurrent, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->maxFileSize, 0, NULL},
		{EDU_EXEC, ncs_edp_int32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->maxLogRecordSize, 0, NULL},
		{EDU_EXEC, ncs_edp_int32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logFileFullAction, 0, NULL},
		{EDU_EXEC, ncs_edp_int32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->maxFilesRotated, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->fileFmt, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logStreamName, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->creationTimeStamp, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->numOpeners, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->streamType, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logRecordId, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_open_stream_msg_ptr = (lgs_ckpt_stream_open_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_open_stream_msg_dec_ptr = (lgs_ckpt_stream_open_t **)ptr;
		if (*ckpt_open_stream_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_open_stream_msg_dec_ptr, '\0', sizeof(lgs_ckpt_stream_open_t));
		ckpt_open_stream_msg_ptr = *ckpt_open_stream_msg_dec_ptr;
	} else {
		ckpt_open_stream_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_open_stream_rec_ed_rules, ckpt_open_stream_msg_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;

}	/* End edp_ed_open_stream_rec */

/****************************************************************************
 * Name          : edp_ed_close_stream_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint close_stream log rec.
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

static uint32_t edp_ed_close_stream_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uint32_t *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_stream_close_t *ckpt_close_stream_msg_ptr = NULL, **ckpt_close_stream_msg_dec_ptr;

	EDU_INST_SET ckpt_close_stream_rec_ed_rules[] = {
		{EDU_START, edp_ed_close_stream_rec, 0, 0, 0, sizeof(lgs_ckpt_stream_close_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_close_t *)0)->streamId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_close_t *)0)->clientId, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_close_stream_msg_ptr = (lgs_ckpt_stream_close_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_close_stream_msg_dec_ptr = (lgs_ckpt_stream_close_t **)ptr;
		if (*ckpt_close_stream_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_close_stream_msg_dec_ptr, '\0', sizeof(lgs_ckpt_stream_close_t));
		ckpt_close_stream_msg_ptr = *ckpt_close_stream_msg_dec_ptr;
	} else {
		ckpt_close_stream_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_close_stream_rec_ed_rules,
				 ckpt_close_stream_msg_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;

}	/* End edp_ed_close_stream_rec */

/****************************************************************************
 * Name          : edp_ed_finalize_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint finalize async updates record.
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
	lgs_ckpt_finalize_msg_t *ckpt_final_msg_ptr = NULL, **ckpt_final_msg_dec_ptr;

	EDU_INST_SET ckpt_final_rec_ed_rules[] = {
		{EDU_START, edp_ed_finalize_rec, 0, 0, 0, sizeof(lgs_ckpt_finalize_msg_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_finalize_msg_t *)0)->client_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_final_msg_ptr = (lgs_ckpt_finalize_msg_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_final_msg_dec_ptr = (lgs_ckpt_finalize_msg_t **)ptr;
		if (*ckpt_final_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_final_msg_dec_ptr, '\0', sizeof(lgs_ckpt_finalize_msg_t));
		ckpt_final_msg_ptr = *ckpt_final_msg_dec_ptr;
	} else {
		ckpt_final_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_final_rec_ed_rules, ckpt_final_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End edp_ed_finalize_rec() */

/****************************************************************************
 * Name          : edp_ed_cfg_stream_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint cfg_update_stream log rec.
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

static uint32_t edp_ed_cfg_stream_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_stream_cfg_t *ckpt_stream_cfg_msg_ptr = NULL, **ckpt_stream_cfg_msg_dec_ptr;

	EDU_INST_SET ckpt_stream_cfg_rec_ed_rules[] = {
		{EDU_START, edp_ed_cfg_stream_rec, 0, 0, 0, sizeof(lgs_ckpt_stream_cfg_t), 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->fileName, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->pathName, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->maxLogFileSize, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->fixedLogRecordSize, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->haProperty, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->logFullAction, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->logFullHaltThreshold, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->maxFilesRotated, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->logFileFormat, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->severityFilter, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_t *)0)->logFileCurrent, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_stream_cfg_msg_ptr = (lgs_ckpt_stream_cfg_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_stream_cfg_msg_dec_ptr = (lgs_ckpt_stream_cfg_t **)ptr;
		if (*ckpt_stream_cfg_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_stream_cfg_msg_dec_ptr, '\0', sizeof(lgs_ckpt_stream_cfg_t));
		ckpt_stream_cfg_msg_ptr = *ckpt_stream_cfg_msg_dec_ptr;
	} else {
		ckpt_stream_cfg_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_stream_cfg_rec_ed_rules, ckpt_stream_cfg_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}

/****************************************************************************
 * Name          : edp_ed_header_rec 
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint message header record.
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
	lgsv_ckpt_header_t *ckpt_header_ptr = NULL, **ckpt_header_dec_ptr;

	EDU_INST_SET ckpt_header_rec_ed_rules[] = {
		{EDU_START, edp_ed_header_rec, 0, 0, 0, sizeof(lgsv_ckpt_header_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgsv_ckpt_header_t *)0)->ckpt_rec_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgsv_ckpt_header_t *)0)->num_ckpt_records, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgsv_ckpt_header_t *)0)->data_len, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_header_ptr = (lgsv_ckpt_header_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_header_dec_ptr = (lgsv_ckpt_header_t **)ptr;
		if (*ckpt_header_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_header_dec_ptr, '\0', sizeof(lgsv_ckpt_header_t));
		ckpt_header_ptr = *ckpt_header_dec_ptr;
	} else {
		ckpt_header_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_header_rec_ed_rules, ckpt_header_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End edp_ed_header_rec() */

/****************************************************************************
 * Name          : ckpt_msg_test_type
 *
 * Description   : This function is an EDU_TEST program which returns the 
 *                 offset to call the appropriate EDU programe based on
 *                 based on the checkpoint message type, for use by 
 *                 the EDU program edu_encode_ckpt_msg.
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

static int32_t ckpt_msg_test_type(NCSCONTEXT arg)
{
	enum {
		LCL_TEST_JUMP_OFFSET_LGS_CKPT_REG = 1,
		LCL_TEST_JUMP_OFFSET_LGS_CKPT_FINAL,
		LCL_TEST_JUMP_OFFSET_LGS_CKPT_WRITE_LOG,
		LCL_TEST_JUMP_OFFSET_LGS_CKPT_OPEN_STREAM,
		LCL_TEST_JUMP_OFFSET_LGS_CKPT_CLOSE_STREAM,
		LCL_TEST_JUMP_OFFSET_LGS_CKPT_AGENT_DOWN,
		LCL_TEST_JUMP_OFFSET_LGS_CKPT_CFG_STREAM
	};
	lgsv_ckpt_msg_type_t ckpt_rec_type;

	if (arg == NULL)
		return EDU_FAIL;
	ckpt_rec_type = *(lgsv_ckpt_msg_type_t *)arg;

	switch (ckpt_rec_type) {
	case LGS_CKPT_CLIENT_INITIALIZE:
		return LCL_TEST_JUMP_OFFSET_LGS_CKPT_REG;
	case LGS_CKPT_CLIENT_FINALIZE:
		return LCL_TEST_JUMP_OFFSET_LGS_CKPT_FINAL;
	case LGS_CKPT_LOG_WRITE:
		return LCL_TEST_JUMP_OFFSET_LGS_CKPT_WRITE_LOG;
	case LGS_CKPT_OPEN_STREAM:
		return LCL_TEST_JUMP_OFFSET_LGS_CKPT_OPEN_STREAM;
	case LGS_CKPT_CLOSE_STREAM:
		return LCL_TEST_JUMP_OFFSET_LGS_CKPT_CLOSE_STREAM;
	case LGS_CKPT_CLIENT_DOWN:
		return LCL_TEST_JUMP_OFFSET_LGS_CKPT_AGENT_DOWN;
	case LGS_CKPT_CFG_STREAM:
		return LCL_TEST_JUMP_OFFSET_LGS_CKPT_CFG_STREAM;
	default:
		return EDU_EXIT;
		break;
	}
	return EDU_FAIL;

}

/****************************************************************************
 * Name          : edp_ed_ckpt_msg 
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint messages. This program runs the 
 *                 edp_ed_hdr_rec program first to decide the
 *                 checkpoint message type based on which it will call the
 *                 appropriate EDU programs for the different checkpoint 
 *                 messages. 
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

static uint32_t edp_ed_ckpt_msg(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			     NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgsv_ckpt_msg_t *ckpt_msg_ptr = NULL, **ckpt_msg_dec_ptr;

	EDU_INST_SET ckpt_msg_ed_rules[] = {
		{EDU_START, edp_ed_ckpt_msg, 0, 0, 0, sizeof(lgsv_ckpt_msg_t), 0, NULL},
		{EDU_EXEC, edp_ed_header_rec, 0, 0, 0, (long)&((lgsv_ckpt_msg_t *)0)->header, 0, NULL},

		{EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((lgsv_ckpt_msg_t *)0)->header, 0,
		 (EDU_EXEC_RTINE)ckpt_msg_test_type},

		/* Reg Record */
		{EDU_EXEC, edp_ed_reg_rec, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_t *)0)->ckpt_rec.initialize_client, 0, NULL},

		/* Finalize record */
		{EDU_EXEC, edp_ed_finalize_rec, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_t *)0)->ckpt_rec.finalize_client, 0, NULL},

		/* write log Record */
		{EDU_EXEC, edp_ed_write_rec, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_t *)0)->ckpt_rec.write_log, 0, NULL},

		/* Open stream */
		{EDU_EXEC, edp_ed_open_stream_rec, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_t *)0)->ckpt_rec.stream_open, 0, NULL},

		/* Close stream */
		{EDU_EXEC, edp_ed_close_stream_rec, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_t *)0)->ckpt_rec.stream_close, 0, NULL},

		/* Agent dest */
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_t *)0)->ckpt_rec.agent_dest, 0, NULL},

		/* Cfg stream */
		{EDU_EXEC, edp_ed_cfg_stream_rec, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_t *)0)->ckpt_rec.stream_cfg, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_msg_ptr = (lgsv_ckpt_msg_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_msg_dec_ptr = (lgsv_ckpt_msg_t **)ptr;
		if (*ckpt_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_msg_dec_ptr, '\0', sizeof(lgsv_ckpt_msg_t));
		ckpt_msg_ptr = *ckpt_msg_dec_ptr;
	} else {
		ckpt_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_msg_ed_rules, ckpt_msg_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;

}	/* End edu_enc_dec_ckpt_msg() */

/* Non EDU routines */
/****************************************************************************
 * Name          : lgs_enc_ckpt_header
 *
 * Description   : This function encodes the checkpoint message header
 *                 using leap provided apis. 
 *
 * Arguments     : pdata - pointer to the buffer to encode this struct in. 
 *                 LGS_CKPT_HEADER - lgsv checkpoint message header. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static void enc_ckpt_header(uint8_t *pdata, lgsv_ckpt_header_t header)
{
	ncs_encode_32bit(&pdata, header.ckpt_rec_type);
	ncs_encode_32bit(&pdata, header.num_ckpt_records);
	ncs_encode_32bit(&pdata, header.data_len);
}

/****************************************************************************
 * Name          : lgs_dec_ckpt_header
 *
 * Description   : This function decodes the checkpoint message header
 *                 using leap provided apis. 
 *
 * Arguments     : NCS_UBAID - pointer to the NCS_UBAID containing data.
 *                 LGS_CKPT_HEADER - lgsv checkpoint message header. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t dec_ckpt_header(NCS_UBAID *uba, lgsv_ckpt_header_t *header)
{
	uint8_t *p8;
	uint8_t local_data[256];
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
}	/*End lgs_dec_ckpt_header */
