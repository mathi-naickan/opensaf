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

  DESCRIPTION: This module does the initialisation of MBCSV and provides
  callback functions at Availability Node Directors. It contains both the
  callbacks to encode the structures at active and decode along with
  building the structures at standby availability directors. All this
  functionality happens in the context of the MBCSV.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include "avnd.h"
static uint32_t avnd_mbcsv_cb(NCS_MBCSV_CB_ARG *arg);
static uint32_t avnd_mbcsv_process_enc_cb(AVND_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uint32_t avnd_mbcsv_process_dec_cb(AVND_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uint32_t avnd_mbcsv_process_peer_info_cb(AVND_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uint32_t avnd_mbcsv_process_notify(AVND_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uint32_t avnd_mbcsv_process_err_ind(AVND_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uint32_t avnd_enqueue_async_update_msgs(AVND_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t avnd_mbcsv_initialize(AVND_CB *cb);
static uint32_t avnd_mbcsv_open_ckpt(AVND_CB *cb);
static uint32_t avnd_get_mbcsv_sel_obj(AVND_CB *cb);
static uint32_t avnd_mbcsv_close_ckpt(AVND_CB *cb);
static uint32_t avnd_mbcsv_finalize(AVND_CB *cb);
static uint32_t avnd_validate_reo_type_in_csync(AVND_CB *cb, uint32_t reo_type);
static uint32_t avnd_ha_state_act_hdlr(AVND_CB *cb);

extern const AVND_ENCODE_CKPT_DATA_FUNC_PTR avnd_enc_ckpt_data_func_list[AVND_CKPT_MSG_MAX];

extern const AVND_DECODE_CKPT_DATA_FUNC_PTR avnd_dec_ckpt_data_func_list[AVND_CKPT_MSG_MAX];
/* LOG HERE Replace all the logs */

/****************************************************************************\
 * Function: avnd_mds_mbcsv_reg
 *
 * Purpose:  AVND function to register AVND with MDS and MBCSv.
 *
 * Input: cb - AVND control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_mds_mbcsv_reg(AVND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* MDS Registration. */
	rc = avnd_mds_vdest_reg(cb);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("MDS registration failed");
		return rc;
	}

	/* MBCSV Registration */
	rc = avnd_mbcsv_register(cb);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("MBCSv registration failed");
		return rc;
	}
	return rc;
}

/****************************************************************************\
 * Function: avnd_mbcsv_register
 *
 * Purpose:  AVND function to register AVND with MBCSv.
 *
 * Input: cb - AVND control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_mbcsv_register(AVND_CB *cb)
{
	uint32_t status = NCSCC_RC_SUCCESS;

	/*  LOG HERE */
	/*
	 * Send Async Update count to zero.
	 */
	memset(&cb->avnd_async_updt_cnt, 0, sizeof(AVND_ASYNC_UPDT_CNT));

	/*
	 * First initialize and then call open.
	 */
	if (NCSCC_RC_SUCCESS != (status = avnd_mbcsv_initialize(cb))) {
		LOG_ER("MBCSV Initialization failed");
		goto done;
	}

	if (NCSCC_RC_SUCCESS != (status = avnd_mbcsv_open_ckpt(cb))) {
		LOG_ER("MBCSv Initialization failed");
		goto done_final;
	}

	/*
	 * Get MBCSv selection object.
	 */
	if (NCSCC_RC_SUCCESS != (status = avnd_get_mbcsv_sel_obj(cb))) {
		LOG_ER("MBCSv selection object get failed");
		goto done_close;
	}

	return status;

 done_close:
	avnd_mbcsv_close_ckpt(cb);
 done_final:
	avnd_mbcsv_finalize(cb);
 done:
	return status;
}

/****************************************************************************\
 * Function: avnd_mbcsv_callback
 *
 * Purpose:  AVND MBCSV call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_mbcsv_cb(NCS_MBCSV_CB_ARG *arg)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVND_CB *cb = avnd_cb;

	TRACE_ENTER();

	if (NULL == arg) {
		LOG_ER("avnd_mbcsv_cb: arg is NULL");
		return NCSCC_RC_FAILURE;
	}

	m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);

	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		status = avnd_mbcsv_process_enc_cb(cb, arg);
		break;

	case NCS_MBCSV_CBOP_DEC:
		status = avnd_mbcsv_process_dec_cb(cb, arg);
		break;

	case NCS_MBCSV_CBOP_PEER:
		status = avnd_mbcsv_process_peer_info_cb(cb, arg);
		break;

	case NCS_MBCSV_CBOP_NOTIFY:
		status = avnd_mbcsv_process_notify(cb, arg);
		break;

	case NCS_MBCSV_CBOP_ERR_IND:
		status = avnd_mbcsv_process_err_ind(cb, arg);
		break;

	default:
		LOG_ER("Invalid mbcsv callback op: %u",arg->i_op);
		status = NCSCC_RC_FAILURE;
		break;
	}

	m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);

	TRACE_LEAVE2("%u", status);
	return status;
}

/****************************************************************************\
 * Function: avnd_mbcsv_process_enc_cb
 *
 * Purpose:  AVND MBCSV encode call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_mbcsv_process_enc_cb(AVND_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Msg type: %u, Reotype: %u",arg->info.encode.io_msg_type, arg->info.encode.io_reo_type);

	switch (arg->info.encode.io_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		{
			/*
			 * If we are sending sync commit message then there is nothing to 
			 * encode wo return.
			 */
			if (AVND_SYNC_COMMIT == arg->info.encode.io_reo_type)
				return status;

			/* Encode Async update message */
			if (arg->info.encode.io_reo_type >= AVND_CKPT_MSG_MAX) {
				LOG_ER("%s Invalid reotype: %u",__FUNCTION__,arg->info.encode.io_reo_type);
				return NCSCC_RC_FAILURE;
			}

			status = avnd_enc_ckpt_data_func_list[arg->info.encode.io_reo_type] (cb, &arg->info.encode);

		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		{
			/* Encode Cold Sync Request message 
			 * Nothing is there to send at this point of time.
			 * But we can set cold_sync_in_progress which indicates that 
			 * cold sync in on.
			 */
			cb->stby_sync_state = AVND_STBY_OUT_OF_SYNC;
			TRACE("avnd_mbcsv_process_enc_cb: Cold Sync Started");
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		{
			/* Encode Cold Sync Response message */
			status = avnd_encode_cold_sync_rsp(cb, &arg->info.encode);
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
		{
			/* Encode Warm Sync Request message 
			 * Nothing is there to send at this point of time.
			 */
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		{
			/* Encode Warm Sync Response message */
			status = avnd_encode_warm_sync_rsp(cb, &arg->info.encode);
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE;
		}
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
		{
			/* Encode Data Response message */
			status = avnd_encode_data_sync_rsp(cb, &arg->info.encode);
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
	case NCS_MBCSV_MSG_DATA_REQ:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
	default:
		LOG_ER("Invalid message type: %u",arg->info.encode.io_msg_type);
		status = NCSCC_RC_FAILURE;
	}
	
	TRACE_LEAVE2("%u", status);
	return status;
}

/****************************************************************************\
 * Function: avnd_mbcsv_process_dec_cb
 *
 * Purpose:  AVND MBCSV decode call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_mbcsv_process_dec_cb(AVND_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uint32_t status = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("msg_type %u and i_reo_type %u",arg->info.decode.i_msg_type, arg->info.decode.i_reo_type);

	switch (arg->info.decode.i_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		{
			/* Decode Async update message */
			if (AVND_STBY_IN_SYNC == cb->stby_sync_state) {
				if (AVND_SYNC_COMMIT != arg->info.decode.i_reo_type) {
					/* 
					 * Enqueue async update message till sync commit message is 
					 * received from the Active.
					 */
					avnd_enqueue_async_update_msgs(cb, &arg->info.decode);
				} else {
					/*
					 * we have received sync commit message. So process all async
					 * ckpt updates received till now.
					 */
					avnd_dequeue_async_update_msgs(cb, true);
				}
			} else {
				/* Nothing is there to decode in this case */
				if (AVND_SYNC_COMMIT == arg->info.decode.i_reo_type)
					return status;

				/* 
				 * Cold sync is in progress, then drop the async update for which
				 * cold sync response are yet to come.
				 */
				if (NCSCC_RC_SUCCESS != avnd_validate_reo_type_in_csync(cb,
											arg->info.decode.i_reo_type)) {
					LOG_ER("Cold Sync on:validate_reo_type failed for %u",arg->info.decode.i_reo_type);
					/* Free userbuff and return without decoding */
					ncs_reset_uba(&arg->info.decode.i_uba);
					break;
				}

				/*
				 * During cold sync operation or in case of warm sync failure
				 * and standby require to be sync-up, process the async updates as and
				 * when they received. Dont enqueue them.
				 */
				if (arg->info.decode.i_reo_type < AVND_CKPT_MSG_MAX)
					status =
					    avnd_dec_ckpt_data_func_list[arg->info.decode.i_reo_type] (cb,
												       &arg->
												       info.decode);
				else {
					LOG_ER("%s Invalid reo type %u",__FUNCTION__,arg->info.decode.i_reo_type);
					status = NCSCC_RC_FAILURE;
				}
			}
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		{
			/* 
			 * Decode Cold Sync request message 
			 * Nothing is there to decode.
			 */
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
		{

			/* Decode Cold Sync Response message */
			status = avnd_decode_cold_sync_rsp(cb, &arg->info.decode);

			if (NCSCC_RC_SUCCESS != status) {
				if (NCSCC_RC_SUCCESS != avnd_ext_comp_data_clean_up(cb, false)) {
					/* Log Error ; FATAL; It should never happen unless there is a bug. */
					LOG_CR("%s, %u,External component data cleanup failed",__FUNCTION__,__LINE__);
				}

				LOG_ER("%s, invalid reo type %u",__FUNCTION__,arg->info.decode.i_reo_type);
				status = NCSCC_RC_FAILURE;
				break;
			}

			/* 
			 * If we have received cold sync complete message then mark standby
			 * as in sync. 
			 */
			if ((NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE == arg->info.decode.i_msg_type) &&
			    (NCSCC_RC_SUCCESS == status)) {

				cb->stby_sync_state = AVND_STBY_IN_SYNC;
				TRACE("avnd_mbcsv_process_dec_cb: Cold Sync Completed");

			}

			cb->synced_reo_type = arg->info.decode.i_reo_type;

		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
		{
			/* 
			 * Decode Warm Sync Request message 
			 * Nothing is there to decode.
			 */
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
		{
			/* Decode Warm Sync Response message */
			status = avnd_decode_warm_sync_rsp(cb, &arg->info.decode);

			/* If we find mismatch in data or warm sync fails set in_sync to false */
			if (NCSCC_RC_FAILURE == status) {
				LOG_ER("avnd_mbcsv_process_dec_cb: warm sync decode failed");
				cb->stby_sync_state = AVND_STBY_OUT_OF_SYNC;
			}

		}
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		{
			/* Decode Data request message */
			status = avnd_decode_data_req(cb, &arg->info.decode);
		}
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		{

			/* Decode Data response and data response complete message */
			status = avnd_decode_data_sync_rsp(cb, &arg->info.decode);

			if (NCSCC_RC_SUCCESS != status) {

				if (NCSCC_RC_SUCCESS != avnd_ext_comp_data_clean_up(cb, false)) {
					/* Log Error ; FATAL; It should never happen unless there is a bug. */
					LOG_CR("%s, %u,External component data cleanup failed",__FUNCTION__,__LINE__);
					break;
				}

				/*
				 * Now send data request, which will sync Standby with Active.
				 */
				if (NCSCC_RC_SUCCESS != avnd_send_data_req(cb)) {
					LOG_ER("AVND send data req: failed");
					break;
				}

				break;
			}

			if (NCS_MBCSV_MSG_DATA_RESP_COMPLETE == arg->info.decode.i_msg_type) {
				cb->stby_sync_state = AVND_STBY_IN_SYNC;
				TRACE("avnd_mbcsv_process_dec_cb: Data Sync Completed");
			}
		}
		break;

	default:
		LOG_ER("%s, Invalid message type: %u",__FUNCTION__,arg->info.decode.i_msg_type);
		status = NCSCC_RC_FAILURE;
		break;

	}
	TRACE_LEAVE2("%u", status);
	return status;

}

/****************************************************************************\
 * Function: avnd_mbcsv_process_peer_info_cb
 *
 * Purpose:  AVND MBCSV peer Info call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_mbcsv_process_peer_info_cb(AVND_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	/* Compare versions of the peer with self */
	TRACE_1("avnd_mbcsv_process_peer_info_cb. Peer Version is %d",arg->info.peer.i_peer_version);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_mbcsv_process_notify_msg
 *
 * Purpose:  AVND MBCSV process Notify message.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_mbcsv_process_notify(AVND_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	LOG_ER("avnd_mbcsv_process_notify");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_mbcsv_process_err_ind
 *
 * Purpose:  AVND MBCSV process error indication.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_mbcsv_process_err_ind(AVND_CB *cb, NCS_MBCSV_CB_ARG *arg)
{

	TRACE_ENTER2("error code is %u",arg->info.error.i_code);

	switch (arg->info.error.i_code) {
	case NCS_MBCSV_COLD_SYNC_TMR_EXP:
		break;

	case NCS_MBCSV_WARM_SYNC_TMR_EXP:
		break;

	case NCS_MBCSV_DATA_RSP_CMPLT_TMR_EXP:
		break;

	case NCS_MBCSV_COLD_SYNC_CMPL_TMR_EXP:
		break;

	case NCS_MBCSV_WARM_SYNC_CMPL_TMR_EXP:
		break;

	case NCS_MBCSV_DATA_RESP_TERMINATED:
		break;

	default:
		LOG_ER("Invalid mbcsv error indication");
		break;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_mbcsv_initialize
 *
 * Purpose:  Initialize AVND with MBCSV
 *
 * Input: cb       - AVND control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_mbcsv_initialize(AVND_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	mbcsv_arg.info.initialize.i_service = NCSMDS_SVC_ID_AVND_CNTLR;
	mbcsv_arg.info.initialize.i_mbcsv_cb = avnd_mbcsv_cb;
	mbcsv_arg.info.initialize.i_version = AVND_MBCSV_SUB_PART_VERSION;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
		return NCSCC_RC_FAILURE;

	cb->avnd_mbcsv_hdl = mbcsv_arg.info.initialize.o_mbcsv_hdl;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_mbcsv_open_ckpt
 *
 * Purpose:  Open checkpoint.
 *
 * Input: cb   - AVND control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_mbcsv_open_ckpt(AVND_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_OPEN;
	mbcsv_arg.i_mbcsv_hdl = cb->avnd_mbcsv_hdl;
	mbcsv_arg.info.open.i_pwe_hdl = cb->avnd_mbcsv_vaddr_pwe_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
		return NCSCC_RC_FAILURE;

	cb->avnd_mbcsv_ckpt_hdl = mbcsv_arg.info.open.o_ckpt_hdl;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_set_mbcsv_ckpt_role
 *
 * Purpose:  Set MBCSV checkpoint role.
 *
 * Input: cb       - AVND control block pointer.
 *        role     - Role to be set.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_set_mbcsv_ckpt_role(AVND_CB *cb, uint32_t role)
{
	NCS_MBCSV_ARG mbcsv_arg;
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	mbcsv_arg.i_mbcsv_hdl = cb->avnd_mbcsv_hdl;
	mbcsv_arg.info.chg_role.i_ckpt_hdl = cb->avnd_mbcsv_ckpt_hdl;
	mbcsv_arg.info.chg_role.i_ha_state = role;

	if (NCSCC_RC_SUCCESS != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		LOG_ER("set_ckpt_role failed");
		return rc;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_get_mbcsv_sel_obj
 *
 * Purpose:  Get MBCSv Selection object.
 *
 * Input: cb        - AVND control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_get_mbcsv_sel_obj(AVND_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;

	/* LOG HERE */

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	mbcsv_arg.i_mbcsv_hdl = cb->avnd_mbcsv_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
		return NCSCC_RC_FAILURE;

	cb->avnd_mbcsv_sel_obj = mbcsv_arg.info.sel_obj_get.o_select_obj;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_mbcsv_dispatch
 *
 * Purpose:  Perform dispatch operation on MBCSV selection object.
 *
 * Input: cb        - AVND control block pointer.
 *        sel_obj   - Selection object returned.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_mbcsv_dispatch(AVND_CB *cb, uint32_t flag)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_DISPATCH;
	mbcsv_arg.i_mbcsv_hdl = cb->avnd_mbcsv_hdl;
	mbcsv_arg.info.dispatch.i_disp_flags = flag;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("MBCSv dispatch failed");
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_send_ckpt_data
 *
 * Purpose:  Send checkpoint data to standby using the supplied send type.
 *
 * Input: cb        - AVND control block pointer.
 *        action    - Action to be perform (add, remove or update)
 *        reo_hdl   - Redudant object handle.
 *        reo_type  - Redudant object type.
 *        send_type - Send type to be used.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_send_ckpt_data(AVND_CB *cb, uint32_t action, MBCSV_REO_HDL reo_hdl, uint32_t reo_type, uint32_t send_type)
{
	NCS_MBCSV_ARG mbcsv_arg;

	/* 
	 * Validate HA state. If my HA state is Standby then don't send 
	 * async updates. In all other states, MBCSv will take care of sending
	 * async updates.
	 */
	if (SA_AMF_HA_STANDBY == cb->avail_state_avnd)
		return NCSCC_RC_SUCCESS;

	/*
	 * Get mbcsv_handle and checkpoint handle from CB.
	 */
	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
	mbcsv_arg.i_mbcsv_hdl = cb->avnd_mbcsv_hdl;
	mbcsv_arg.info.send_ckpt.i_action = action;
	mbcsv_arg.info.send_ckpt.i_ckpt_hdl = cb->avnd_mbcsv_ckpt_hdl;
	mbcsv_arg.info.send_ckpt.i_reo_hdl = reo_hdl;
	mbcsv_arg.info.send_ckpt.i_reo_type = reo_type;
	mbcsv_arg.info.send_ckpt.i_send_type = send_type;

	/*
	 * Before sendig this message, update async update count.
	 */
	switch (reo_type) {
	case AVND_CKPT_HLT_CONFIG:
	case AVND_CKPT_HC_PERIOD:
	case AVND_CKPT_HC_MAX_DUR:
		cb->avnd_async_updt_cnt.hlth_config_updt++;
		break;

	case AVND_CKPT_SU_CONFIG:
	case AVND_CKPT_SU_FLAG_CHANGE:
	case AVND_CKPT_SU_ERR_ESC_LEVEL:
	case AVND_CKPT_SU_COMP_RESTART_PROB:
	case AVND_CKPT_SU_COMP_RESTART_MAX:
	case AVND_CKPT_SU_RESTART_PROB:
	case AVND_CKPT_SU_RESTART_MAX:
	case AVND_CKPT_SU_COMP_RESTART_CNT:
	case AVND_CKPT_SU_RESTART_CNT:
	case AVND_CKPT_SU_ERR_ESC_TMR:
	case AVND_CKPT_SU_OPER_STATE:
	case AVND_CKPT_SU_PRES_STATE:
		cb->avnd_async_updt_cnt.su_updt++;
		break;

	case AVND_CKPT_COMP_CONFIG:
	case AVND_CKPT_COMP_FLAG_CHANGE:
	case AVND_CKPT_COMP_REG_HDL:
	case AVND_CKPT_COMP_REG_DEST:
	case AVND_CKPT_COMP_OPER_STATE:
	case AVND_CKPT_COMP_PRES_STATE:
	case AVND_CKPT_COMP_TERM_CBK_TIMEOUT:
	case AVND_CKPT_COMP_CSI_SET_CBK_TIMEOUT:
	case AVND_CKPT_COMP_QUIES_CMPLT_CBK_TIMEOUT:
	case AVND_CKPT_COMP_CSI_RMV_CBK_TIMEOUT:
	case AVND_CKPT_COMP_PXIED_INST_CBK_TIMEOUT:
	case AVND_CKPT_COMP_PXIED_CLEAN_CBK_TIMEOUT:
	case AVND_CKPT_COMP_ERR_INFO:
	case AVND_CKPT_COMP_DEFAULT_RECVR:
	case AVND_CKPT_COMP_PEND_EVT:
	case AVND_CKPT_COMP_ORPH_TMR:
	case AVND_CKPT_COMP_NODE_ID:
	case AVND_CKPT_COMP_TYPE:
	case AVND_CKPT_COMP_MDS_CTXT:
	case AVND_CKPT_COMP_REG_RESP_PENDING:
	case AVND_CKPT_COMP_INST_CMD:
	case AVND_CKPT_COMP_TERM_CMD:
	case AVND_CKPT_COMP_INST_TIMEOUT:
	case AVND_CKPT_COMP_TERM_TIMEOUT:
	case AVND_CKPT_COMP_INST_RETRY_MAX:
	case AVND_CKPT_COMP_INST_RETRY_CNT:
	case AVND_CKPT_COMP_EXEC_CMD:
	case AVND_CKPT_COMP_CMD_EXEC_CTXT:
	case AVND_CKPT_COMP_INST_CMD_TS:
	case AVND_CKPT_COMP_CLC_REG_TMR:
	case AVND_CKPT_COMP_INST_CODE_RCVD:
	case AVND_CKPT_COMP_PROXY_PROXIED_ADD:
	case AVND_CKPT_COMP_PROXY_PROXIED_DEL:
		cb->avnd_async_updt_cnt.comp_updt++;
		break;

	case AVND_SYNC_COMMIT:
		break;

	case AVND_CKPT_SU_SI_REC:
	case AVND_CKPT_SU_SI_REC_CURR_STATE:
	case AVND_CKPT_SU_SI_REC_PRV_STATE:
	case AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE:
	case AVND_CKPT_SU_SI_REC_PRV_ASSIGN_STATE:
		cb->avnd_async_updt_cnt.su_si_updt++;
		break;

	case AVND_CKPT_SIQ_REC:
		cb->avnd_async_updt_cnt.siq_updt++;
		break;

	case AVND_CKPT_CSI_REC:
	case AVND_CKPT_COMP_CSI_ACT_COMP_NAME:
	case AVND_CKPT_COMP_CSI_TRANS_DESC:
	case AVND_CKPT_COMP_CSI_STANDBY_RANK:
	case AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE:
	case AVND_CKPT_COMP_CSI_PRV_ASSIGN_STATE:
		cb->avnd_async_updt_cnt.csi_updt++;
		break;

	case AVND_CKPT_COMP_HLT_REC:
	case AVND_CKPT_COMP_HC_REC_STATUS:
	case AVND_CKPT_COMP_HC_REC_TMR:
		cb->avnd_async_updt_cnt.comp_hlth_rec_updt++;
		break;

	case AVND_CKPT_COMP_CBK_REC:
	case AVND_CKPT_COMP_CBK_REC_AMF_HDL:
	case AVND_CKPT_COMP_CBK_REC_MDS_DEST:
	case AVND_CKPT_COMP_CBK_REC_TMR:
	case AVND_CKPT_COMP_CBK_REC_TIMEOUT:
		cb->avnd_async_updt_cnt.comp_cbk_rec_updt++;
		break;

	default:
		return NCSCC_RC_SUCCESS;
	}

	/*
	 * Now send this update.
	 */
	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("MBCSv send data failed");
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_mbcsv_close_ckpt
 *
 * Purpose:  Close checkpoint.
 *
 * Input: cb        - AVND control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_mbcsv_close_ckpt(AVND_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;

	/* LOG HERE */

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_CLOSE;
	mbcsv_arg.i_mbcsv_hdl = cb->avnd_mbcsv_hdl;
	mbcsv_arg.info.close.i_ckpt_hdl = cb->avnd_mbcsv_ckpt_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("MBCSv checkpoint close failed");
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_mbcsv_finalize
 *
 * Purpose:  Finalize AVND with MBCSV
 *
 * Input: cb        - AVND control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_mbcsv_finalize(AVND_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;

	/* LOG HERE */

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_FINALIZE;
	mbcsv_arg.i_mbcsv_hdl = cb->avnd_mbcsv_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("MBCSV finalize failed");
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_mbcsv_obj_set
 *
 * Purpose:  Set MBCSv objects
 *
 * Input: cb        - AVND control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_mbcsv_obj_set(AVND_CB *cb, uint32_t obj, uint32_t val)
{
	NCS_MBCSV_ARG mbcsv_arg;

	/* LOG HERE */

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_OBJ_SET;
	mbcsv_arg.i_mbcsv_hdl = cb->avnd_mbcsv_hdl;
	mbcsv_arg.info.obj_set.i_ckpt_hdl = cb->avnd_mbcsv_ckpt_hdl;
	mbcsv_arg.info.obj_set.i_obj = obj;
	mbcsv_arg.info.obj_set.i_val = val;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("MBCSV object set failed");
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_enqueue_async_update_msgs
 *
 * Purpose:  Enqueue async update messages.
 *
 * Input: cb  - AVND CB pointer.
 *        dec - Decode message content pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_enqueue_async_update_msgs(AVND_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	AVND_ASYNC_UPDT_MSG_QUEUE *updt_msg;
	TRACE_ENTER();

	/*
	 * This is a FIFO queue. Add message at the tail of the queue.
	 */
	if (NULL == (updt_msg = calloc(1, sizeof(AVND_ASYNC_UPDT_MSG_QUEUE)))) {
		/* Log error */
		LOG_CR("calloc failed for AVND_ASYNC_UPDT_MSG_QUEUE");
		return NCSCC_RC_FAILURE;
	}

	updt_msg->dec = *dec;

	/* Add this message in our FIFO queue */
	if (!(cb->async_updt_msgs.async_updt_queue))
		cb->async_updt_msgs.async_updt_queue = (updt_msg);
	else
		cb->async_updt_msgs.tail->next = (updt_msg);

	cb->async_updt_msgs.tail = (updt_msg);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_dequeue_async_update_msgs
 *
 * Purpose:  De-queue async update messages.
 *
 * Input: cb - AVND CB pointer.
 *        pr_or_fr - true - If we have to process the message.
 *                   false - If we have to FREE the message.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_dequeue_async_update_msgs(AVND_CB *cb, bool pr_or_fr)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVND_ASYNC_UPDT_MSG_QUEUE *updt_msg;

	TRACE_ENTER2("%u",pr_or_fr);

	/*
	 * This is a FIFO queue. Remove first message first entered in the 
	 * queue and then process it.
	 */
	while (NULL != (updt_msg = cb->async_updt_msgs.async_updt_queue)) {
		cb->async_updt_msgs.async_updt_queue = updt_msg->next;

		/*
		 * Process de-queued message.
		 */
		if (pr_or_fr)
			status = avnd_dec_ckpt_data_func_list[updt_msg->dec.i_reo_type] (cb, &updt_msg->dec);

		free(updt_msg);
	}

	/* All messages are dequeued. Set tail to NULL */
	cb->async_updt_msgs.tail = NULL;

	TRACE_LEAVE();
	return status;
}

/****************************************************************************\
 * Function: avnd_send_data_req
 *
 * Purpose:  Encode data request to be sent.
 *
 * Input: cb - AVND CB pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_send_data_req(AVND_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;
	NCS_UBAID *uba = NULL;

	TRACE_ENTER();

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
	mbcsv_arg.i_mbcsv_hdl = cb->avnd_mbcsv_hdl;

	uba = &mbcsv_arg.info.send_data_req.i_uba;

	memset(uba, '\0', sizeof(NCS_UBAID));

	mbcsv_arg.info.send_data_req.i_ckpt_hdl = cb->avnd_mbcsv_ckpt_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("MBCSV send data req failed");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avnd_validate_reo_type_in_csync
 *
 * Purpose:  Vaidate reo_type received during cold sync updates. Return success,
 *           if cold sync is over for this reo_type. Return failure if standby
 *           is still to get cold sync for this reo_type.
 *
 * Input: cb - AVND CB pointer.
 *        reo_type - reo type need to be validated during cold sync.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avnd_validate_reo_type_in_csync(AVND_CB *cb, uint32_t reo_type)
{
	uint32_t status = NCSCC_RC_FAILURE;

	TRACE_ENTER();

	switch (reo_type) {
	case AVND_CKPT_HLT_CONFIG:
	case AVND_CKPT_HC_PERIOD:
	case AVND_CKPT_HC_MAX_DUR:
		if (cb->synced_reo_type >= AVND_CKPT_HLT_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVND_CKPT_SU_CONFIG:
	case AVND_CKPT_SU_FLAG_CHANGE:
	case AVND_CKPT_SU_ERR_ESC_LEVEL:
	case AVND_CKPT_SU_COMP_RESTART_PROB:
	case AVND_CKPT_SU_COMP_RESTART_MAX:
	case AVND_CKPT_SU_RESTART_PROB:
	case AVND_CKPT_SU_RESTART_MAX:
	case AVND_CKPT_SU_COMP_RESTART_CNT:
	case AVND_CKPT_SU_RESTART_CNT:
	case AVND_CKPT_SU_ERR_ESC_TMR:
	case AVND_CKPT_SU_OPER_STATE:
	case AVND_CKPT_SU_PRES_STATE:
		if (cb->synced_reo_type >= AVND_CKPT_SU_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVND_CKPT_COMP_CONFIG:
	case AVND_CKPT_COMP_FLAG_CHANGE:
	case AVND_CKPT_COMP_REG_HDL:
	case AVND_CKPT_COMP_REG_DEST:
	case AVND_CKPT_COMP_OPER_STATE:
	case AVND_CKPT_COMP_PRES_STATE:
	case AVND_CKPT_COMP_TERM_CBK_TIMEOUT:
	case AVND_CKPT_COMP_CSI_SET_CBK_TIMEOUT:
	case AVND_CKPT_COMP_QUIES_CMPLT_CBK_TIMEOUT:
	case AVND_CKPT_COMP_CSI_RMV_CBK_TIMEOUT:
	case AVND_CKPT_COMP_PXIED_INST_CBK_TIMEOUT:
	case AVND_CKPT_COMP_PXIED_CLEAN_CBK_TIMEOUT:
	case AVND_CKPT_COMP_ERR_INFO:
	case AVND_CKPT_COMP_DEFAULT_RECVR:
	case AVND_CKPT_COMP_PEND_EVT:
	case AVND_CKPT_COMP_ORPH_TMR:
	case AVND_CKPT_COMP_NODE_ID:
	case AVND_CKPT_COMP_TYPE:
	case AVND_CKPT_COMP_MDS_CTXT:
	case AVND_CKPT_COMP_REG_RESP_PENDING:
	case AVND_CKPT_COMP_INST_CMD:
	case AVND_CKPT_COMP_TERM_CMD:
	case AVND_CKPT_COMP_INST_TIMEOUT:
	case AVND_CKPT_COMP_TERM_TIMEOUT:
	case AVND_CKPT_COMP_INST_RETRY_MAX:
	case AVND_CKPT_COMP_INST_RETRY_CNT:
	case AVND_CKPT_COMP_EXEC_CMD:
	case AVND_CKPT_COMP_CMD_EXEC_CTXT:
	case AVND_CKPT_COMP_INST_CMD_TS:
	case AVND_CKPT_COMP_CLC_REG_TMR:
	case AVND_CKPT_COMP_INST_CODE_RCVD:
	case AVND_CKPT_COMP_PROXY_PROXIED_ADD:
	case AVND_CKPT_COMP_PROXY_PROXIED_DEL:
		if (cb->synced_reo_type >= AVND_CKPT_COMP_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVND_CKPT_SU_SI_REC:
	case AVND_CKPT_SU_SI_REC_CURR_STATE:
	case AVND_CKPT_SU_SI_REC_PRV_STATE:
	case AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE:
	case AVND_CKPT_SU_SI_REC_PRV_ASSIGN_STATE:
		if (cb->synced_reo_type >= AVND_CKPT_SU_SI_REC)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVND_CKPT_SIQ_REC:
		if (cb->synced_reo_type >= AVND_CKPT_SIQ_REC)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVND_CKPT_CSI_REC:
	case AVND_CKPT_COMP_CSI_ACT_COMP_NAME:
	case AVND_CKPT_COMP_CSI_TRANS_DESC:
	case AVND_CKPT_COMP_CSI_STANDBY_RANK:
	case AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE:
	case AVND_CKPT_COMP_CSI_PRV_ASSIGN_STATE:
		if (cb->synced_reo_type >= AVND_CKPT_CSI_REC)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVND_CKPT_COMP_HLT_REC:
	case AVND_CKPT_COMP_HC_REC_STATUS:
	case AVND_CKPT_COMP_HC_REC_TMR:
		if (cb->synced_reo_type >= AVND_CKPT_COMP_HLT_REC)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVND_CKPT_COMP_CBK_REC:
	case AVND_CKPT_COMP_CBK_REC_AMF_HDL:
	case AVND_CKPT_COMP_CBK_REC_MDS_DEST:
	case AVND_CKPT_COMP_CBK_REC_TMR:
	case AVND_CKPT_COMP_CBK_REC_TIMEOUT:
		if (cb->synced_reo_type >= AVND_CKPT_COMP_CBK_REC)
			status = NCSCC_RC_SUCCESS;
		break;

	default:
		return NCSCC_RC_SUCCESS;
	}

	TRACE_LEAVE();
	return status;
}

/******************************************************************************
  Name          : avnd_evt_avd_role_change_msg

  Description   : This routine takes cares of role change of AvND. 

  Arguments     : cb  - ptr to the AvND control block.
                  evt - ptr to the AvND event.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/
uint32_t avnd_evt_avd_role_change_evh(AVND_CB *cb, AVND_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_D2N_ROLE_CHANGE_INFO *info = NULL;
	V_DEST_RL mds_role;
	SaAmfHAStateT prev_ha_state;

	TRACE_ENTER();

	/* dont process unless AvD is up */
	if (!m_AVND_CB_IS_AVD_UP(cb)){
		LOG_IN("AVD is not up yet");
		return NCSCC_RC_FAILURE;
	}

	info = &evt->info.avd->msg_info.d2n_role_change_info;

	TRACE("MsgId: %u,NodeId:%u, role rcvd:%u role present:%u",\
			      info->msg_id, info->node_id, info->role, cb->avail_state_avnd);

	avnd_msgid_assert(info->msg_id);
	cb->rcv_msg_id = info->msg_id;

	prev_ha_state = cb->avail_state_avnd;

	/* Ignore the duplicate roles. */
	if (prev_ha_state == info->role) {
		return NCSCC_RC_SUCCESS;
	}

	if ((SA_AMF_HA_ACTIVE == cb->avail_state_avnd) && (SA_AMF_HA_QUIESCED == info->role)) {
		TRACE_1("SA_AMF_HA_QUIESCED role received");
		if (NCSCC_RC_SUCCESS != (rc = avnd_mds_set_vdest_role(cb, V_DEST_RL_QUIESCED))) {
			TRACE("avnd_mds_set_vdest_role returned failure, role:%u",info->role);
			return rc;
		}
		return rc;
	}

	cb->avail_state_avnd = info->role;

	if (cb->avail_state_avnd == SA_AMF_HA_ACTIVE) {
		mds_role = V_DEST_RL_ACTIVE;
		TRACE_1("SA_AMF_HA_ACTIVE role received");
	} else {
		mds_role = V_DEST_RL_STANDBY;
		TRACE_1("SA_AMF_HA_STANDBY role received");
	}

	if (NCSCC_RC_SUCCESS != (rc = avnd_mds_set_vdest_role(cb, mds_role))) {
		TRACE_1("avnd_mds_set_vdest_role returned failure");
		return rc;
	}

	if (NCSCC_RC_SUCCESS != (rc = avnd_set_mbcsv_ckpt_role(cb, info->role))) {
		LOG_ER("avnd_set_mbcsv_ckpt_role failed");
		return rc;
	}

	if (cb->avail_state_avnd == SA_AMF_HA_ACTIVE) {
		/* We might be having some async update messages in the
		   Queue to be processed, now drop all of them. */

		avnd_dequeue_async_update_msgs(cb, false);

		/* Go through timer list and start it. So send an event in mail box */
		/* We need to start all the timers, which were running on ACT. */
		rc = avnd_ha_state_act_hdlr(cb);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("avnd_ha_state_act_hdlr failed");
		}
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************\
 * Function: avnd_ckpt_for_ext
 *
 * Purpose:  Send checkpoint data to standby using the supplied send type.
 *
 * Input: cb        - AVND control block pointer.
 *        reo_hdl   - object handle.
 *        reo_type  - object type.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_ckpt_for_ext(AVND_CB *cb, MBCSV_REO_HDL reo_hdl, uint32_t reo_type)
{
	uint8_t *reo_hdl_ptr = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;

	reo_hdl_ptr = NCS_INT64_TO_PTR_CAST(reo_hdl);

	if (NULL == reo_hdl_ptr)
		return rc;

	switch (reo_type) {
	case AVND_CKPT_HLT_CONFIG:
	case AVND_CKPT_HC_PERIOD:
	case AVND_CKPT_HC_MAX_DUR:
		{
			if (true == ((AVND_HC *)reo_hdl_ptr)->is_ext)
				rc = NCSCC_RC_SUCCESS;
		}
		break;

	case AVND_CKPT_SU_CONFIG:
	case AVND_CKPT_SU_FLAG_CHANGE:
	case AVND_CKPT_SU_ERR_ESC_LEVEL:
	case AVND_CKPT_SU_COMP_RESTART_PROB:
	case AVND_CKPT_SU_COMP_RESTART_MAX:
	case AVND_CKPT_SU_RESTART_PROB:
	case AVND_CKPT_SU_RESTART_MAX:
	case AVND_CKPT_SU_COMP_RESTART_CNT:
	case AVND_CKPT_SU_RESTART_CNT:
	case AVND_CKPT_SU_ERR_ESC_TMR:
	case AVND_CKPT_SU_OPER_STATE:
	case AVND_CKPT_SU_PRES_STATE:
		{
			if (true == ((AVND_SU *)reo_hdl_ptr)->su_is_external)
				rc = NCSCC_RC_SUCCESS;
		}
		break;

	case AVND_CKPT_COMP_PROXY_PROXIED_ADD:
	case AVND_CKPT_COMP_PROXY_PROXIED_DEL:
	case AVND_CKPT_COMP_CONFIG:
		{
			if (m_AVND_COMP_TYPE_IS_INTER_NODE(((AVND_COMP *)reo_hdl_ptr))) {
				if (m_AVND_PROXY_IS_FOR_EXT_COMP(((AVND_COMP *)reo_hdl_ptr)))
					rc = NCSCC_RC_SUCCESS;
			} else if (true == ((AVND_COMP *)reo_hdl_ptr)->su->su_is_external)
				rc = NCSCC_RC_SUCCESS;
		}
		break;
	case AVND_CKPT_COMP_FLAG_CHANGE:
	case AVND_CKPT_COMP_REG_HDL:
	case AVND_CKPT_COMP_REG_DEST:
	case AVND_CKPT_COMP_OPER_STATE:
	case AVND_CKPT_COMP_PRES_STATE:
	case AVND_CKPT_COMP_TERM_CBK_TIMEOUT:
	case AVND_CKPT_COMP_CSI_SET_CBK_TIMEOUT:
	case AVND_CKPT_COMP_QUIES_CMPLT_CBK_TIMEOUT:
	case AVND_CKPT_COMP_CSI_RMV_CBK_TIMEOUT:
	case AVND_CKPT_COMP_PXIED_INST_CBK_TIMEOUT:
	case AVND_CKPT_COMP_PXIED_CLEAN_CBK_TIMEOUT:
	case AVND_CKPT_COMP_ERR_INFO:
	case AVND_CKPT_COMP_DEFAULT_RECVR:
	case AVND_CKPT_COMP_PEND_EVT:
	case AVND_CKPT_COMP_ORPH_TMR:
	case AVND_CKPT_COMP_NODE_ID:
	case AVND_CKPT_COMP_TYPE:
	case AVND_CKPT_COMP_MDS_CTXT:
	case AVND_CKPT_COMP_REG_RESP_PENDING:
	case AVND_CKPT_COMP_INST_CMD:
	case AVND_CKPT_COMP_TERM_CMD:
	case AVND_CKPT_COMP_INST_TIMEOUT:
	case AVND_CKPT_COMP_TERM_TIMEOUT:
	case AVND_CKPT_COMP_INST_RETRY_MAX:
	case AVND_CKPT_COMP_INST_RETRY_CNT:
	case AVND_CKPT_COMP_EXEC_CMD:
	case AVND_CKPT_COMP_CMD_EXEC_CTXT:
	case AVND_CKPT_COMP_INST_CMD_TS:
	case AVND_CKPT_COMP_CLC_REG_TMR:
	case AVND_CKPT_COMP_INST_CODE_RCVD:
		{
			if (m_AVND_COMP_TYPE_IS_INTER_NODE(((AVND_COMP *)reo_hdl_ptr))) {
				if (m_AVND_PROXY_IS_FOR_EXT_COMP(((AVND_COMP *)reo_hdl_ptr)))
					rc = NCSCC_RC_SUCCESS;
			} else if (true == ((AVND_COMP *)reo_hdl_ptr)->su->su_is_external)
				rc = NCSCC_RC_SUCCESS;
		}
		break;

	case AVND_CKPT_SU_SI_REC:
	case AVND_CKPT_SU_SI_REC_CURR_STATE:
	case AVND_CKPT_SU_SI_REC_PRV_STATE:
	case AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE:
	case AVND_CKPT_SU_SI_REC_PRV_ASSIGN_STATE:
		{
			if (true == ((AVND_SU_SI_REC *)reo_hdl_ptr)->su->su_is_external)
				rc = NCSCC_RC_SUCCESS;
		}
		break;

	case AVND_CKPT_CSI_REC:
	case AVND_CKPT_COMP_CSI_ACT_COMP_NAME:
	case AVND_CKPT_COMP_CSI_TRANS_DESC:
	case AVND_CKPT_COMP_CSI_STANDBY_RANK:
	case AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE:
	case AVND_CKPT_COMP_CSI_PRV_ASSIGN_STATE:
		{
			if (true == ((AVND_COMP_CSI_REC *)reo_hdl_ptr)->comp->su->su_is_external)
				rc = NCSCC_RC_SUCCESS;
		}
		break;

	case AVND_CKPT_COMP_HLT_REC:
	case AVND_CKPT_COMP_HC_REC_STATUS:
	case AVND_CKPT_COMP_HC_REC_TMR:
		{
			if (true == ((AVND_COMP_HC_REC *)reo_hdl_ptr)->comp->su->su_is_external)
				rc = NCSCC_RC_SUCCESS;
		}
		break;

	case AVND_CKPT_COMP_CBK_REC:
		{
			if (m_AVND_COMP_TYPE_IS_INTER_NODE(((AVND_COMP_CBK *)reo_hdl_ptr)->comp)) {
				if (m_AVND_PROXY_IS_FOR_EXT_COMP(((AVND_COMP_CBK *)reo_hdl_ptr)->comp))
					rc = NCSCC_RC_SUCCESS;
			} else if (true == ((AVND_COMP_CBK *)reo_hdl_ptr)->comp->su->su_is_external)
				rc = NCSCC_RC_SUCCESS;
		}
		break;

	case AVND_CKPT_COMP_CBK_REC_AMF_HDL:
	case AVND_CKPT_COMP_CBK_REC_MDS_DEST:
	case AVND_CKPT_COMP_CBK_REC_TMR:
	case AVND_CKPT_COMP_CBK_REC_TIMEOUT:
		{
			if (true == ((AVND_COMP_CBK *)reo_hdl_ptr)->comp->su->su_is_external)
				rc = NCSCC_RC_SUCCESS;
		}
		break;

	case AVND_CKPT_SIQ_REC:
		/* For AVND_SU_SIQ_REC data structure, we don't have anything to check
		   whether it is a part of an external or internal component. So,
		   while sending ADD/RMV async update for AVND_CKPT_SIQ_REC, validate 
		   before sending. */
		rc = NCSCC_RC_SUCCESS;
		break;

	default:
		break;

	}

	return rc;
}

/****************************************************************************\
 * Function: avnd_evt_ha_state_change
 *
 * Purpose:  Takes cares state change of HA State.
 *
 * Input: cb        - AVND control block pointer.
 *        evt - ptr to the AvND event
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_evt_ha_state_change_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_HA_STATE_CHANGE_EVT *ha_state_event = NULL;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER();

	ha_state_event = &evt->info.ha_state_change;

	if (NULL == ha_state_event)
		return rc;

	if (AVND_EVT_HA_STATE_CHANGE != evt->type)
		goto error;

	if ((SA_AMF_HA_QUIESCED == ha_state_event->ha_state) && (true == cb->is_quisced_set)) {
		cb->avail_state_avnd = SA_AMF_HA_QUIESCED;

		if (NCSCC_RC_SUCCESS != (rc = avnd_set_mbcsv_ckpt_role(cb, cb->avail_state_avnd))) {
			LOG_ER("avnd_set_mbcsv_ckpt_role failed");
			goto error;
		}
		cb->is_quisced_set = false;
		TRACE_1("mbcsv role set to SA_AMF_HA_QUIESCED");
		return rc;
	}

error:
	TRACE("evt_type:%u, ha_state:%u ,cb->is_quisced_set:%u, rc:%u are",evt->type, ha_state_event->ha_state, cb->is_quisced_set, rc);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************\
 * Function: avnd_ha_state_act_hdlr
 *
 * Purpose:  Takes cares state change of HA State ACT.
 *           It starts all the timers.
 *
 * Input: cb  - AVND control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 *
\**************************************************************************/
uint32_t avnd_ha_state_act_hdlr(AVND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_SU *su = NULL;
	AVND_COMP *comp = NULL;
	AVND_COMP_HC_REC *comp_hc = NULL;
	AVND_COMP_CBK *comp_cbk = NULL;
	TRACE_ENTER();

/******************  Starting SU Timers  *******************************/
	{

		su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)0);
		while (su != 0) {
			if (true == su->su_is_external) {
				if ((AVND_ERR_ESC_LEVEL_0 == su->su_err_esc_level) &&
				    (true == su->su_err_esc_tmr.is_active)) {
					/* This means component err esc timer is running. */
					m_AVND_TMR_COMP_ERR_ESC_START(cb, su, rc);
				} else if ((AVND_ERR_ESC_LEVEL_1 == su->su_err_esc_level) &&
					   (true == su->su_err_esc_tmr.is_active)) {
					/* This means su err esc timer is running. */
					m_AVND_TMR_SU_ERR_ESC_START(cb, su, rc);
				}

				if (rc != NCSCC_RC_SUCCESS) {
					/* Encode failed!!! */
					LOG_ER("%s,%u,encode failed,ret:%u",__FUNCTION__,__LINE__,rc);
					return rc;
				}

			}	/* if(true == su->su_is_external) */
			su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)&su->name);
		}		/* while(su != 0) */
	}
/******************  Starting SU Timers ends here *******************************/

/******************  Starting Component Timers  *******************************/
	{
		comp = (AVND_COMP *)ncs_patricia_tree_getnext(&cb->compdb, (uint8_t *)0);

		while (comp != 0) {
			if (true == comp->su->su_is_external) {
				if (true == comp->orph_tmr.is_active) {
					m_AVND_TMR_PXIED_COMP_REG_START(cb, *comp, rc);
				}

				if (rc != NCSCC_RC_SUCCESS) {
					LOG_ER("proxied comp reg start failed");
					return rc;
				}

				if ((true == comp->clc_info.clc_reg_tmr.is_active) &&
				    (AVND_TMR_CLC_COMP_REG == comp->clc_info.clc_reg_tmr.type)) {
					m_AVND_TMR_COMP_REG_START(cb, *comp, rc);
				}

				if ((true == comp->clc_info.clc_reg_tmr.is_active) &&
				    (AVND_TMR_CLC_PXIED_COMP_INST == comp->clc_info.clc_reg_tmr.type)) {
					m_AVND_TMR_PXIED_COMP_INST_START(cb, *comp, rc);
				}

				if (rc != NCSCC_RC_SUCCESS) {
					LOG_ER("proxied comp inst start failed");
					return rc;
				}

/* We can start health check and callback timers here only. */

/******************  Starting Health Check Timers ********************/
				for (comp_hc = (AVND_COMP_HC_REC *)m_NCS_DBLIST_FIND_FIRST(&comp->hc_list);
				     comp_hc;
				     comp_hc = (AVND_COMP_HC_REC *)m_NCS_DBLIST_FIND_NEXT(&comp_hc->comp_dll_node)) {
					if (true == comp_hc->tmr.is_active) {
						m_AVND_TMR_COMP_HC_START(cb, *comp_hc, rc);
					}

					if (rc != NCSCC_RC_SUCCESS) {
						LOG_ER("Healthcheck timer start failed");
						return rc;
					}
				}
/******************  Starting Health Check Timers ends here ****************/

/***********************  Starting Callback Timers ***********************/
				for (comp_cbk = comp->cbk_list; comp_cbk; comp_cbk = comp_cbk->next) {
					/* We need to overwrite opq_hdl with red_opq_hdl now and reset 
					   red_opq_hdl for every record. */

					comp_cbk->opq_hdl = comp_cbk->red_opq_hdl;
					comp_cbk->red_opq_hdl = 0;

					if (true == comp_cbk->resp_tmr.is_active) {
						m_AVND_TMR_COMP_CBK_RESP_START(cb, *comp_cbk, comp_cbk->timeout, rc);
					}

					if (rc != NCSCC_RC_SUCCESS) {
						LOG_ER("component callback response timer start failed");
						return rc;
					}
				}
/***********************  Starting Callback Timers ends here ****************/

			}	/* if(true == comp->su->su_is_external) */
			comp = (AVND_COMP *)
			    ncs_patricia_tree_getnext(&cb->compdb, (uint8_t *)&comp->name);
		}		/* while(comp != 0) */
	}
/******************  Starting Component Timers ends here ********************/

	TRACE_LEAVE();
	return rc;
}
