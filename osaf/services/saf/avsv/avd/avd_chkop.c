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
  callback functions at Availability Directors. It contains both the
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
#include <logtrace.h>
#include "avd.h"

static uint32_t avsv_mbcsv_cb(NCS_MBCSV_CB_ARG *arg);
static uint32_t avsv_mbcsv_process_enc_cb(AVD_CL_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uint32_t avsv_mbcsv_process_dec_cb(AVD_CL_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uint32_t avsv_mbcsv_process_peer_info_cb(AVD_CL_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uint32_t avsv_mbcsv_process_notify(AVD_CL_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uint32_t avsv_mbcsv_process_err_ind(AVD_CL_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uint32_t avsv_enqueue_async_update_msgs(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec);
static uint32_t avsv_mbcsv_initialize(AVD_CL_CB *cb);
static uint32_t avsv_mbcsv_open_ckpt(AVD_CL_CB *cb);
static uint32_t avsv_get_mbcsv_sel_obj(AVD_CL_CB *cb);
static uint32_t avsv_mbcsv_close_ckpt(AVD_CL_CB *cb);
static uint32_t avsv_mbcsv_finalize(AVD_CL_CB *cb);
static uint32_t avsv_validate_reo_type_in_csync(AVD_CL_CB *cb, uint32_t reo_type);

extern const AVSV_ENCODE_CKPT_DATA_FUNC_PTR avd_enc_ckpt_data_func_list[AVSV_CKPT_MSG_MAX];

extern const AVSV_DECODE_CKPT_DATA_FUNC_PTR avd_dec_data_func_list[AVSV_CKPT_MSG_MAX];

/****************************************************************************\
 * Function: avsv_mbcsv_register
 *
 * Purpose:  AVSV function to register AVD with MBCSv.
 *
 * Input: cb - AVD control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avsv_mbcsv_register(AVD_CL_CB *cb)
{
	uint32_t status = NCSCC_RC_SUCCESS;

	/*
	 * Send Async Update count to zero.
	 */
	memset(&cb->async_updt_cnt, 0, sizeof(AVSV_ASYNC_UPDT_CNT));

	/*
	 * Compile Ckpt EDP's
	 */
	if (NCSCC_RC_SUCCESS != (status = avd_compile_ckpt_edp(cb))) {
		LOG_ER("%s: avd_compile_ckpt_edp failed", __FUNCTION__);
		goto done;
	}

	/*
	 * First initialize and then call open.
	 */
	if (NCSCC_RC_SUCCESS != (status = avsv_mbcsv_initialize(cb)))
		goto done;

	if (NCSCC_RC_SUCCESS != (status = avsv_mbcsv_open_ckpt(cb)))
		goto done_final;

	/*
	 * Get MBCSv selection object.
	 */
	if (NCSCC_RC_SUCCESS != (status = avsv_get_mbcsv_sel_obj(cb)))
		goto done_close;

	return status;

done_close:
	avsv_mbcsv_close_ckpt(cb);
done_final:
	avsv_mbcsv_finalize(cb);
done:
	return status;
}

/****************************************************************************\
 * Function: avsv_mbcsv_deregister
 *
 * Purpose:  AVSV function to deregister AVD with MBCSv.
 *
 * Input: cb - AVD control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avsv_mbcsv_deregister(AVD_CL_CB *cb)
{
	uint32_t status = NCSCC_RC_SUCCESS;

	/*
	 * First close and then finalize.
	 */
	if (NCSCC_RC_SUCCESS != (status = avsv_mbcsv_close_ckpt(cb)))
		goto done;

	if (NCSCC_RC_SUCCESS != (status = avsv_mbcsv_finalize(cb)))
		goto done;

done:
	return status;
}

/****************************************************************************\
 * Function: avsv_mbcsv_callback
 *
 * Purpose:  AVSV MBCSV call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_mbcsv_cb(NCS_MBCSV_CB_ARG *arg)
{
	uint32_t status;

	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		status = avsv_mbcsv_process_enc_cb(avd_cb, arg);
		break;

	case NCS_MBCSV_CBOP_DEC:
		status = avsv_mbcsv_process_dec_cb(avd_cb, arg);
		break;

	case NCS_MBCSV_CBOP_PEER:
		status = avsv_mbcsv_process_peer_info_cb(avd_cb, arg);
		break;

	case NCS_MBCSV_CBOP_NOTIFY:
		status = avsv_mbcsv_process_notify(avd_cb, arg);
		break;

	case NCS_MBCSV_CBOP_ERR_IND:
		status = avsv_mbcsv_process_err_ind(avd_cb, arg);
		break;

	default:
		LOG_ER("avsv_mbcsv_cb: invalid op %u", arg->i_op);
		status = NCSCC_RC_FAILURE;
		break;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_mbcsv_process_enc_cb
 *
 * Purpose:  AVSV MBCSV encode call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_mbcsv_process_enc_cb(AVD_CL_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uint32_t status = NCSCC_RC_SUCCESS;

	switch (arg->info.encode.io_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		{
			/*
			 * If we are sending sync commit message then there is nothing to 
			 * encode wo return.
			 */
			if (AVSV_SYNC_COMMIT == arg->info.encode.io_reo_type)
				return status;

			/* Encode Async update message */
			if (arg->info.encode.io_reo_type >= AVSV_CKPT_MSG_MAX) {
				LOG_ER("%s: invalid type %u", __FUNCTION__, arg->info.encode.io_reo_type);
				return NCSCC_RC_FAILURE;
			}

			TRACE("Async update");

			status = avd_enc_ckpt_data_func_list[arg->info.encode.io_reo_type] (cb, &arg->info.encode);
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		{
			/* Encode Cold Sync Request message 
			 * Nothing is there to send at this point of time.
			 * But we can set cold_sync_in_progress which indicates that 
			 * cold sync in on.
			 */
			cb->stby_sync_state = AVD_STBY_OUT_OF_SYNC;
			TRACE("Cold sync req");
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		{
			/* Encode Cold Sync Response message */
			status = avd_enc_cold_sync_rsp(cb, &arg->info.encode);
			TRACE("Cold sync resp");
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
		{
			/* Encode Warm Sync Request message 
			 * Nothing is there to send at this point of time.
			 */
			TRACE("Warm sync req");
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		{
			/* Encode Warm Sync Response message */
			status = avd_enc_warm_sync_rsp(cb, &arg->info.encode);
			TRACE("Warm sync resp");
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE;
		}
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
		{
			/* Encode Data Response message */
			status = avd_enc_data_sync_rsp(cb, &arg->info.encode);
			TRACE("Data resp");
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
	case NCS_MBCSV_MSG_DATA_REQ:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
	default:
		LOG_ER("%s: invalid msg type %u", __FUNCTION__, arg->info.encode.io_msg_type);
		status = NCSCC_RC_FAILURE;
	}

	return status;
}

/****************************************************************************\
 * Function: avsv_mbcsv_process_dec_cb
 *
 * Purpose:  AVSV MBCSV decode call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_mbcsv_process_dec_cb(AVD_CL_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uint32_t status = NCSCC_RC_SUCCESS;

	switch (arg->info.decode.i_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		{
			if ((arg->info.decode.i_peer_version < AVD_MBCSV_SUB_PART_VERSION_3) && 
					(arg->info.decode.i_reo_type >= AVSV_CKPT_AVD_SI_TRANS))
				arg->info.decode.i_reo_type ++;

			/* Decode Async update message */
			if (AVD_STBY_IN_SYNC == cb->stby_sync_state) {
				if (AVSV_SYNC_COMMIT != arg->info.decode.i_reo_type) {
					/* 
					 * Enqueue async update message till sync commit message is 
					 * received from the Active.
					 */
					avsv_enqueue_async_update_msgs(cb, &arg->info.decode);
				} else {
					/*
					 * we have received sync commit message. So process all async
					 * ckpt updates received till now.
					 */
					avsv_dequeue_async_update_msgs(cb, true);
				}
			} else {
				/* Nothing is there to decode in this case */
				if (AVSV_SYNC_COMMIT == arg->info.decode.i_reo_type)
					return status;

				/* 
				 * Cold sync is in progress, then drop the async update for which
				 * cold sync response are yet to come.
				 */
				if (NCSCC_RC_SUCCESS != avsv_validate_reo_type_in_csync(cb,
											arg->info.decode.i_reo_type)) {
					/* Free userbuff and return without decoding */
					ncs_reset_uba(&arg->info.decode.i_uba);
					break;
				}

				/*
				 * During cold sync operation or in case of warm sync failure
				 * and standby require to be sync-up, process the async updates as and
				 * when they received. Dont enqueue them.
				 */
				if (arg->info.decode.i_reo_type >= AVSV_CKPT_MSG_MAX) {
					LOG_ER("%s: invalid type %u", __FUNCTION__, arg->info.encode.io_reo_type);
					status = NCSCC_RC_FAILURE;
					break;
				}
				if (cb->avd_peer_ver < AVD_MBCSV_SUB_PART_VERSION_4) {
					if ((arg->info.decode.i_action == NCS_MBCSV_ACT_ADD) || 
					    (arg->info.decode.i_action == NCS_MBCSV_ACT_RMV)) {
						/* Ignore this message because this comes as applier callbacks
							just increment the updt count */
						switch (arg->info.decode.i_reo_type) {
							case AVSV_CKPT_AVD_CB_CONFIG:
								cb->async_updt_cnt.cb_updt++;
								goto ignore_msg;
							case AVSV_CKPT_AVD_NODE_CONFIG:
								cb->async_updt_cnt.node_updt++;
								goto ignore_msg;
							case AVSV_CKPT_AVD_APP_CONFIG:
								cb->async_updt_cnt.app_updt++;
								goto ignore_msg;
							case AVSV_CKPT_AVD_SG_CONFIG:
								cb->async_updt_cnt.sg_updt++;
								goto ignore_msg;
							case AVSV_CKPT_AVD_SU_CONFIG:
								cb->async_updt_cnt.su_updt++;
								goto ignore_msg;
							case AVSV_CKPT_AVD_SI_CONFIG:
								cb->async_updt_cnt.si_updt++;
								goto ignore_msg;
							case AVSV_CKPT_AVD_COMP_CONFIG:
								cb->async_updt_cnt.comp_updt++;
								goto ignore_msg;
							case AVSV_CKPT_AVD_COMP_CS_TYPE_CONFIG:
								cb->async_updt_cnt.compcstype_updt++;
								goto ignore_msg;
							default:
								break;
						}
					}
				}
				status =
					avd_dec_data_func_list[arg->info.decode.i_reo_type] (cb,
												&arg->info.decode);
			}
		}
ignore_msg:
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		{
			/* 
			 * Decode Cold Sync request message 
			 * Nothing is there to decode.
			 */
			TRACE("COLD_SYNC_REQ");

			if (cb->init_state < AVD_INIT_DONE) {
				TRACE("invalid init state (%u) for cold sync req", cb->init_state);
				status = NCSCC_RC_FAILURE;
			}
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
		{
			if ((arg->info.decode.i_peer_version < AVD_MBCSV_SUB_PART_VERSION_3) && 
					(arg->info.decode.i_reo_type >= AVSV_CKPT_AVD_SI_TRANS))
				arg->info.decode.i_reo_type ++;

			/* Decode Cold Sync Response message */
			status = avd_dec_cold_sync_rsp(cb, &arg->info.decode);

			if (NCSCC_RC_SUCCESS != status) {
				LOG_ER("%s: cold sync decode failed %u", __FUNCTION__, status);
				(void) avd_data_clean_up(cb);
				status = NCSCC_RC_FAILURE;
				break;
			}

			/* 
			 * If we have received cold sync complete message then mark standby
			 * as in sync. 
			 */
			if ((NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE == arg->info.decode.i_msg_type) &&
			    (NCSCC_RC_SUCCESS == status)) {
				LOG_NO("Cold sync complete!");
				/* saflog something on the standby to avoid sync calls later
				** when in a more critical state */
				saflog(LOG_NOTICE, amfSvcUsrName, "Cold sync complete at %x", cb->node_id_avd);
				cb->stby_sync_state = AVD_STBY_IN_SYNC;
				cb->active_services_exist = true;
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
			TRACE("warm sync req");
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
		{
			if ((arg->info.decode.i_peer_version < AVD_MBCSV_SUB_PART_VERSION_3) && 
					(arg->info.decode.i_reo_type >= AVSV_CKPT_AVD_SI_TRANS))
				arg->info.decode.i_reo_type ++;

			/* Decode Warm Sync Response message */
			status = avd_dec_warm_sync_rsp(cb, &arg->info.decode);

			/* If we find mismatch in data or warm sync fails set in_sync to false */
			if (NCSCC_RC_FAILURE == status) {
				LOG_ER("%s: warm sync decode failed %u", __FUNCTION__, status);
				cb->stby_sync_state = AVD_STBY_OUT_OF_SYNC;
			}
		}
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		{
			/* Decode Data request message */
			status = avd_dec_data_req(cb, &arg->info.decode);
		}
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		{
			if ((arg->info.decode.i_peer_version < AVD_MBCSV_SUB_PART_VERSION_3) && 
					(arg->info.decode.i_reo_type >= AVSV_CKPT_AVD_SI_TRANS))
				arg->info.decode.i_reo_type ++;

			/* Decode Data response and data response complete message */
			status = avd_dec_data_sync_rsp(cb, &arg->info.decode);

			if (NCSCC_RC_SUCCESS != status) {
				LOG_ER("%s: data resp decode failed %u", __FUNCTION__, status);
				avd_data_clean_up(cb);

				/*
				 * Now send data request, which will sync Standby with Active.
				 */
				if (NCSCC_RC_SUCCESS != avsv_send_data_req(cb))
					break;

				break;
			}

			if (NCS_MBCSV_MSG_DATA_RESP_COMPLETE == arg->info.decode.i_msg_type) {
				cb->stby_sync_state = AVD_STBY_IN_SYNC;
				LOG_IN("Standby in sync");
			}
		}
		break;

	default:
		LOG_ER("%s: invalid msg type %u", __FUNCTION__, arg->info.decode.i_msg_type);
		status = NCSCC_RC_FAILURE;
		break;

	}
	return status;

}

/****************************************************************************\
 * Function: avsv_mbcsv_process_peer_info_cb
 *
 * Purpose:  AVSV MBCSV peer Info call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_mbcsv_process_peer_info_cb(AVD_CL_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	/* Used to refer while sending checkpoint msg to its peer AVD */
	cb->avd_peer_ver = arg->info.peer.i_peer_version;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_mbcsv_process_notify_msg
 *
 * Purpose:  AVSV MBCSV process Notify message.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_mbcsv_process_notify(AVD_CL_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	LOG_ER("%s:%u", __FUNCTION__, __LINE__);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_mbcsv_process_err_ind
 *
 * Purpose:  AVSV MBCSV process error indication.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_mbcsv_process_err_ind(AVD_CL_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	switch (arg->info.error.i_code) {
	case NCS_MBCSV_COLD_SYNC_TMR_EXP:
		/* The first cold sync request seems to be ignored so don't log */
		TRACE("mbcsv cold sync tmr exp");
		break;

	case NCS_MBCSV_WARM_SYNC_TMR_EXP:
		LOG_WA("mbcsv warm sync tmr exp");
		break;

	case NCS_MBCSV_DATA_RSP_CMPLT_TMR_EXP:
		LOG_WA("mbcsv data rsp cmplt tmr exp");
		break;

	case NCS_MBCSV_COLD_SYNC_CMPL_TMR_EXP:
		LOG_WA("mbcsv cold sync cmpl tmr exp");
		break;

	case NCS_MBCSV_WARM_SYNC_CMPL_TMR_EXP:
		LOG_WA("mbcsv warm sync cmpl tmr exp");
		break;

	case NCS_MBCSV_DATA_RESP_TERMINATED:
		LOG_WA("mbcsv data rsp term");
		break;

	case NCS_MBCSV_COLD_SYNC_RESP_TERMINATED:
		LOG_WA("mbcsv cold sync rsp term");
		break;

	case NCS_MBCSV_WARM_SYNC_RESP_TERMINATED:
		LOG_WA("mbcsv warm sync rsp term");
		break;

	default:
		LOG_IN("mbcsv unknown ecode %u", arg->info.error.i_code);
		break;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_mbcsv_initialize
 *
 * Purpose:  Initialize AVSV with MBCSV
 *
 * Input: cb       - AVD control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_mbcsv_initialize(AVD_CL_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg = {0};

	mbcsv_arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	mbcsv_arg.info.initialize.i_service = NCS_SERVICE_ID_AVD;
	mbcsv_arg.info.initialize.i_mbcsv_cb = avsv_mbcsv_cb;
	mbcsv_arg.info.initialize.i_version = AVD_MBCSV_SUB_PART_VERSION;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("%s: ncs_mbcsv_svc NCS_MBCSV_OP_INITIALIZE failed", __FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	cb->mbcsv_hdl = mbcsv_arg.info.initialize.o_mbcsv_hdl;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_mbcsv_open_ckpt
 *
 * Purpose:  Open checkpoint.
 *
 * Input: cb   - AVD control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_mbcsv_open_ckpt(AVD_CL_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg = {0};

	mbcsv_arg.i_op = NCS_MBCSV_OP_OPEN;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.open.i_pwe_hdl = cb->vaddr_pwe_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("%s: ncs_mbcsv_svc NCS_MBCSV_OP_OPEN failed", __FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	cb->ckpt_hdl = mbcsv_arg.info.open.o_ckpt_hdl;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_set_ckpt_role
 *
 * Purpose:  Set checkpoint role.
 *
 * Input: cb       - AVD control block pointer.
 *        role     - Role to be set.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avsv_set_ckpt_role(AVD_CL_CB *cb, uint32_t role)
{
	NCS_MBCSV_ARG mbcsv_arg;
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.chg_role.i_ckpt_hdl = cb->ckpt_hdl;
	mbcsv_arg.info.chg_role.i_ha_state = role;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("ncs_mbcsv_svc NCS_MBCSV_OP_CHG_ROLE %u failed", role);
		return NCSCC_RC_FAILURE;
	}

	return rc;
}

/****************************************************************************\
 * Function: avd_avnd_send_role_change
 *
 * Purpose:  Set checkpoint role.
 *
 * Input: cb       - AVD control block pointer.
 *        node_id  - Node id of the AvND.
 *        role     - Role to be set.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 *
\**************************************************************************/
uint32_t avd_avnd_send_role_change(AVD_CL_CB *cb, NODE_ID node_id, uint32_t role)
{
	AVD_DND_MSG *d2n_msg = NULL;
	AVD_AVND *node = NULL;

	TRACE_ENTER2("node_id %x role %u", node_id, role);

	/* Check wether we are sending to peer controller AvND, which is of older
	   version. Don't send role change to older version Controler AvND. */
	if ((node_id == cb->node_id_avd_other) && (cb->peer_msg_fmt_ver < AVSV_AVD_AVND_MSG_FMT_VER_2)) {
		goto done;
	}

	/* It may happen that this function has been called before AvND has come
	   up, so just return SUCCESS. Send the role change when AvND comes up. */
	if ((node = avd_node_find_nodeid(node_id)) == NULL) {
		LOG_ER("%s: node not found", __FUNCTION__);
		goto done;
	}

	d2n_msg = calloc(1, sizeof(AVSV_DND_MSG));
	if (d2n_msg == NULL) {
		LOG_ER("calloc failed");
		osafassert(0);
	}

	d2n_msg->msg_type = AVSV_D2N_ROLE_CHANGE_MSG;
	d2n_msg->msg_info.d2n_role_change_info.msg_id = ++(node->snd_msg_id);
	d2n_msg->msg_info.d2n_role_change_info.node_id = node->node_info.nodeId;
	d2n_msg->msg_info.d2n_role_change_info.role = role;

	/* send the message */
	if (avd_d2n_msg_snd(cb, node, d2n_msg) != NCSCC_RC_SUCCESS) {
		--(node->snd_msg_id);
		LOG_ER("%s: avd_d2n_msg_snd failed", __FUNCTION__);
		avsv_dnd_msg_free(d2n_msg);
		return NCSCC_RC_FAILURE;
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, node, AVSV_CKPT_AVND_SND_MSG_ID);

done:
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_get_mbcsv_sel_obj
 *
 * Purpose:  Get MBCSv Selection object.
 *
 * Input: cb        - AVD control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_get_mbcsv_sel_obj(AVD_CL_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg = {0};

	mbcsv_arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("%s: ncs_mbcsv_svc NCS_MBCSV_OP_SEL_OBJ_GET failed", __FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	cb->mbcsv_sel_obj = mbcsv_arg.info.sel_obj_get.o_select_obj;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_mbcsv_dispatch
 *
 * Purpose:  Perform dispatch operation on MBCSV selection object.
 *
 * Input: cb        - AVD control block pointer.
 *        sel_obj   - Selection object returned.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avsv_mbcsv_dispatch(AVD_CL_CB *cb, uint32_t flag)
{
	NCS_MBCSV_ARG mbcsv_arg;
	uint32_t rc;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_DISPATCH;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.dispatch.i_disp_flags = flag;

	if (NCSCC_RC_SUCCESS != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		LOG_ER("ncs_mbcsv_svc NCS_MBCSV_OP_DISPATCH %u failed", rc);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_send_ckpt_data
 *
 * Purpose:  Send checkpoint data to standby using the supplied send type.
 *
 * Input: cb        - AVD control block pointer.
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
uint32_t avsv_send_ckpt_data(AVD_CL_CB *cb, uint32_t action, MBCSV_REO_HDL reo_hdl, uint32_t reo_type, uint32_t send_type)
{
	NCS_MBCSV_ARG mbcsv_arg;

	/* 
	 * Validate HA state. If my HA state is Standby then don't send 
	 * async updates. In all other states, MBCSv will take care of sending
	 * async updates.
	 */
	if (SA_AMF_HA_STANDBY == cb->avail_state_avd)
		return NCSCC_RC_SUCCESS;

	/*
	 * Get mbcsv_handle and checkpoint handle from CB.
	 */
	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.send_ckpt.i_action = action;
	mbcsv_arg.info.send_ckpt.i_ckpt_hdl = cb->ckpt_hdl;
	mbcsv_arg.info.send_ckpt.i_reo_hdl = reo_hdl;
	mbcsv_arg.info.send_ckpt.i_reo_type = reo_type;
	mbcsv_arg.info.send_ckpt.i_send_type = send_type;

	/*
	 * Before sendig this message, update async update count.
	 */
	switch (reo_type) {
	case AVSV_CKPT_AVD_CB_CONFIG:
		cb->async_updt_cnt.cb_updt++;
		break;

	case AVSV_CKPT_AVD_NODE_CONFIG:
		if ((avd_cb->avd_peer_ver >= AVD_MBCSV_SUB_PART_VERSION_4) && 
		   ((action == NCS_MBCSV_ACT_ADD) || 
		    (action == NCS_MBCSV_ACT_RMV)))
			/* No need to send the message as standy would get the applier callback */
			return NCSCC_RC_SUCCESS;
			/* else fall through */
	case AVSV_CKPT_AVND_NODE_UP_INFO:
	case AVSV_CKPT_AVND_ADMIN_STATE:
	case AVSV_CKPT_AVND_OPER_STATE:
	case AVSV_CKPT_AVND_NODE_STATE:
	case AVSV_CKPT_AVND_RCV_MSG_ID:
	case AVSV_CKPT_AVND_SND_MSG_ID:
		cb->async_updt_cnt.node_updt++;
		break;

	case AVSV_CKPT_AVD_APP_CONFIG:
		if ((avd_cb->avd_peer_ver >= AVD_MBCSV_SUB_PART_VERSION_4) && 
		   ((action == NCS_MBCSV_ACT_ADD) || 
		    (action == NCS_MBCSV_ACT_RMV)))
			/* No need to send the message as standy would get the applier callback */
			return NCSCC_RC_SUCCESS;
			/* else fall through */
		cb->async_updt_cnt.app_updt++;
		break;

	case AVSV_CKPT_AVD_SG_CONFIG:
		if ((avd_cb->avd_peer_ver >= AVD_MBCSV_SUB_PART_VERSION_4) && 
		   ((action == NCS_MBCSV_ACT_ADD) || 
		    (action == NCS_MBCSV_ACT_RMV)))
			/* No need to send the message as standy would get the applier callback */
			return NCSCC_RC_SUCCESS;
			/* else fall through */
	case AVSV_CKPT_SG_ADMIN_STATE:
	case AVSV_CKPT_SG_ADJUST_STATE:
	case AVSV_CKPT_SG_SU_ASSIGNED_NUM:
	case AVSV_CKPT_SG_SU_SPARE_NUM:
	case AVSV_CKPT_SG_SU_UNINST_NUM:
	case AVSV_CKPT_SG_FSM_STATE:
		cb->async_updt_cnt.sg_updt++;
		break;

	case AVSV_CKPT_SU_RESTART_COUNT:
		if (avd_cb->avd_peer_ver < AVD_MBCSV_SUB_PART_VERSION_4) {
			/* No need to send the message to old std as this async is newly added. */
			return NCSCC_RC_SUCCESS;
		}
		cb->async_updt_cnt.su_updt++;
		break;
	case AVSV_CKPT_AVD_SU_CONFIG:
		if ((avd_cb->avd_peer_ver >= AVD_MBCSV_SUB_PART_VERSION_4) && 
		   ((action == NCS_MBCSV_ACT_ADD) || 
		    (action == NCS_MBCSV_ACT_RMV)))
			/* No need to send the message as standy would get the applier callback */
			return NCSCC_RC_SUCCESS;
			/* else fall through */
	case AVSV_CKPT_SU_SI_CURR_ACTIVE:
	case AVSV_CKPT_SU_SI_CURR_STBY:
	case AVSV_CKPT_SU_ADMIN_STATE:
	case AVSV_CKPT_SU_TERM_STATE:
	case AVSV_CKPT_SU_SWITCH:
	case AVSV_CKPT_SU_OPER_STATE:
	case AVSV_CKPT_SU_PRES_STATE:
	case AVSV_CKPT_SU_READINESS_STATE:
	case AVSV_CKPT_SU_ACT_STATE:
	case AVSV_CKPT_SU_PREINSTAN:
		cb->async_updt_cnt.su_updt++;
		break;

	case AVSV_CKPT_SI_DEP_STATE: {
#ifdef UPGRADE_FROM_4_2_1
		// special case for 4.2.1, si_dep_state should be check pointed using ver 4
		uint16_t ver_compare = AVD_MBCSV_SUB_PART_VERSION_4;
#else
		// default case, si_dep_state should not be check pointed for peers in ver 4 (or less)
	 	uint16_t ver_compare = AVD_MBCSV_SUB_PART_VERSION_5;
#endif
	 	if (avd_cb->avd_peer_ver < ver_compare) {
	 		/* No need to send the message to old std as this async is newly added. */
			return NCSCC_RC_SUCCESS;
		}

		/* AVD_SI_READY_TO_ASSIGN depstate is not supported in opensaf-4.2 so not checkpoint it. */ 
		if ((((AVD_SI *)(NCS_INT64_TO_PTR_CAST(reo_hdl)))->si_dep_state == AVD_SI_READY_TO_ASSIGN) &&
				(avd_cb->avd_peer_ver < AVD_MBCSV_SUB_PART_VERSION_6)) {
			return NCSCC_RC_SUCCESS;
		}

		cb->async_updt_cnt.si_updt++;
		break;
	}
	case AVSV_CKPT_AVD_SI_CONFIG:
		if ((avd_cb->avd_peer_ver >= AVD_MBCSV_SUB_PART_VERSION_4) && 
		   ((action == NCS_MBCSV_ACT_ADD) || 
		    (action == NCS_MBCSV_ACT_RMV)))
			/* No need to send the message as standy would get the applier callback */
			return NCSCC_RC_SUCCESS;
			/* else fall through */
	case AVSV_CKPT_SI_SU_CURR_ACTIVE:
	case AVSV_CKPT_SI_SU_CURR_STBY:
	case AVSV_CKPT_SI_SWITCH:
	case AVSV_CKPT_SI_ADMIN_STATE:
	case AVSV_CKPT_SI_ALARM_SENT:
	case AVSV_CKPT_SI_ASSIGNMENT_STATE:
		cb->async_updt_cnt.si_updt++;
		break;

	case AVSV_CKPT_AVD_SG_OPER_SU:
		cb->async_updt_cnt.sg_su_oprlist_updt++;
		break;

	case AVSV_CKPT_AVD_SG_ADMIN_SI:
		cb->async_updt_cnt.sg_admin_si_updt++;
		break;

	case AVSV_CKPT_AVD_COMP_CONFIG:
		if ((avd_cb->avd_peer_ver >= AVD_MBCSV_SUB_PART_VERSION_4) && 
		   ((action == NCS_MBCSV_ACT_ADD) || 
		    (action == NCS_MBCSV_ACT_RMV)))
			/* No need to send the message as standy would get the applier callback */
			return NCSCC_RC_SUCCESS;
			/* else fall through */
	case AVSV_CKPT_COMP_CURR_PROXY_NAME:
	case AVSV_CKPT_COMP_CURR_NUM_CSI_ACTV:
	case AVSV_CKPT_COMP_CURR_NUM_CSI_STBY:
	case AVSV_CKPT_COMP_OPER_STATE:
	case AVSV_CKPT_COMP_READINESS_STATE:
	case AVSV_CKPT_COMP_PRES_STATE:
	case AVSV_CKPT_COMP_RESTART_COUNT:
		cb->async_updt_cnt.comp_updt++;
		break;
	case AVSV_CKPT_AVD_SI_ASS:
		cb->async_updt_cnt.siass_updt++;
		break;
	case AVSV_CKPT_AVD_SI_TRANS:
		cb->async_updt_cnt.si_trans_updt++;
		break;

	case AVSV_SYNC_COMMIT:
		break;

	case AVSV_CKPT_AVD_COMP_CS_TYPE_CONFIG:
		cb->async_updt_cnt.compcstype_updt++;
		break;

	default:
		return NCSCC_RC_SUCCESS;
	}

	/*
	 * Now send this update.
	 */
	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("%s: ncs_mbcsv_svc NCS_MBCSV_OP_SEND_CKPT failed", __FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_mbcsv_close_ckpt
 *
 * Purpose:  Close checkpoint.
 *
 * Input: cb        - AVD control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_mbcsv_close_ckpt(AVD_CL_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg = {0};

	mbcsv_arg.i_op = NCS_MBCSV_OP_CLOSE;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.close.i_ckpt_hdl = cb->ckpt_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("%s: ncs_mbcsv_svc NCS_MBCSV_OP_CLOSE failed", __FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_mbcsv_finalize
 *
 * Purpose:  Finalize AVSV with MBCSV
 *
 * Input: cb        - AVD control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_mbcsv_finalize(AVD_CL_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg = {0};

	mbcsv_arg.i_op = NCS_MBCSV_OP_FINALIZE;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("%s: ncs_mbcsv_svc NCS_MBCSV_OP_FINALIZE failed", __FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_mbcsv_obj_set
 *
 * Purpose:  Set MBCSv objects
 *
 * Input: cb        - AVD control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avsv_mbcsv_obj_set(AVD_CL_CB *cb, uint32_t obj, uint32_t val)
{
	NCS_MBCSV_ARG mbcsv_arg = {0};

	mbcsv_arg.i_op = NCS_MBCSV_OP_OBJ_SET;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.obj_set.i_ckpt_hdl = cb->ckpt_hdl;
	mbcsv_arg.info.obj_set.i_obj = obj;
	mbcsv_arg.info.obj_set.i_val = val;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("%s: ncs_mbcsv_svc NCS_MBCSV_OP_OBJ_SET failed", __FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_enqueue_async_update_msgs
 *
 * Purpose:  Enqueue async update messages.
 *
 * Input: cb  - AVD CB pointer.
 *        dec - Decode message content pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_enqueue_async_update_msgs(AVD_CL_CB *cb, NCS_MBCSV_CB_DEC *dec)
{
	AVSV_ASYNC_UPDT_MSG_QUEUE *updt_msg;

	/*
	 * This is a FIFO queue. Add message at the tail of the queue.
	 */
	if (NULL == (updt_msg = calloc(1, sizeof(AVSV_ASYNC_UPDT_MSG_QUEUE)))) {
		LOG_ER("calloc failed");
		osafassert(0);
		return NCSCC_RC_FAILURE;
	}

	updt_msg->dec = *dec;

	/* Add this message in our FIFO queue */
	if (!(cb->async_updt_msgs.async_updt_queue))
		cb->async_updt_msgs.async_updt_queue = (updt_msg);
	else
		cb->async_updt_msgs.tail->next = (updt_msg);

	cb->async_updt_msgs.tail = (updt_msg);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_dequeue_async_update_msgs
 *
 * Purpose:  De-queue async update messages.
 *
 * Input: cb - AVD CB pointer.
 *        pr_or_fr - true - If we have to process the message.
 *                   false - If we have to FREE the message.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avsv_dequeue_async_update_msgs(AVD_CL_CB *cb, bool pr_or_fr)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVSV_ASYNC_UPDT_MSG_QUEUE *updt_msg;

	TRACE_ENTER2("%u", pr_or_fr);

	/*
	 * This is a FIFO queue. Remove first message first entered in the 
	 * queue and then process it.
	 */
	while (NULL != (updt_msg = cb->async_updt_msgs.async_updt_queue)) {
		cb->async_updt_msgs.async_updt_queue = updt_msg->next;

		/*
		 * Process de-queued message.
		 */
		if (pr_or_fr) {
			if (cb->avd_peer_ver < AVD_MBCSV_SUB_PART_VERSION_4) {
				if ((updt_msg->dec.i_action == NCS_MBCSV_ACT_ADD) || 
				    (updt_msg->dec.i_action == NCS_MBCSV_ACT_RMV)) {
					/* Ignore this message because this comes as applier callbacks
						just increment the updt count */
					switch (updt_msg->dec.i_reo_type) {
						case AVSV_CKPT_AVD_CB_CONFIG:
							cb->async_updt_cnt.cb_updt++;
							goto free_msg;
						case AVSV_CKPT_AVD_NODE_CONFIG:
							cb->async_updt_cnt.node_updt++;
							goto free_msg;
						case AVSV_CKPT_AVD_APP_CONFIG:
							cb->async_updt_cnt.app_updt++;
							goto free_msg;
						case AVSV_CKPT_AVD_SG_CONFIG:
							cb->async_updt_cnt.sg_updt++;
							goto free_msg;
						case AVSV_CKPT_AVD_SU_CONFIG:
							cb->async_updt_cnt.su_updt++;
							goto free_msg;
						case AVSV_CKPT_AVD_SI_CONFIG:
							cb->async_updt_cnt.si_updt++;
							goto free_msg;
						case AVSV_CKPT_AVD_COMP_CONFIG:
							cb->async_updt_cnt.comp_updt++;
							goto free_msg;
						case AVSV_CKPT_AVD_COMP_CS_TYPE_CONFIG:
							cb->async_updt_cnt.compcstype_updt++;
							goto free_msg;
						default:
							break;
					}
				}
			}
			status = avd_dec_data_func_list[updt_msg->dec.i_reo_type] (cb, &updt_msg->dec);
		}
free_msg:
		free(updt_msg);
	}

	/* All messages are dequeued. Set tail to NULL */
	cb->async_updt_msgs.tail = NULL;

	return status;
}

/****************************************************************************\
 * Function: avsv_send_data_req
 *
 * Purpose:  Encode data request to be sent.
 *
 * Input: cb - AVD CB pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avsv_send_data_req(AVD_CL_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg = {0};
	NCS_UBAID *uba = NULL;

	mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;

	uba = &mbcsv_arg.info.send_data_req.i_uba;

	memset(uba, '\0', sizeof(NCS_UBAID));

	mbcsv_arg.info.send_data_req.i_ckpt_hdl = cb->ckpt_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		LOG_ER("%s: ncs_mbcsv_svc NCS_MBCSV_OP_SEND_DATA_REQ failed", __FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: avsv_validate_reo_type_in_csync
 *
 * Purpose:  Vaidate reo_type received during cold sync updates. Return success,
 *           if cold sync is over for this reo_type. Return failure if standby
 *           is still to get cold sync for this reo_type.
 *
 * Input: cb - AVD CB pointer.
 *        reo_type - reo type need to be validated during cold sync.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uint32_t avsv_validate_reo_type_in_csync(AVD_CL_CB *cb, uint32_t reo_type)
{
	uint32_t status = NCSCC_RC_FAILURE;

	switch (reo_type) {
	case AVSV_CKPT_AVD_CB_CONFIG:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_CB_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

		/* AVND Async Update messages */
	case AVSV_CKPT_AVD_NODE_CONFIG:
	case AVSV_CKPT_AVND_NODE_UP_INFO:
	case AVSV_CKPT_AVND_ADMIN_STATE:
	case AVSV_CKPT_AVND_OPER_STATE:
	case AVSV_CKPT_AVND_NODE_STATE:
	case AVSV_CKPT_AVND_RCV_MSG_ID:
	case AVSV_CKPT_AVND_SND_MSG_ID:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_NODE_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVSV_CKPT_AVD_APP_CONFIG:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_APP_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

		/* SG Async Update messages */
	case AVSV_CKPT_AVD_SG_CONFIG:
	case AVSV_CKPT_SG_ADMIN_STATE:
	case AVSV_CKPT_SG_ADJUST_STATE:
	case AVSV_CKPT_SG_SU_ASSIGNED_NUM:
	case AVSV_CKPT_SG_SU_SPARE_NUM:
	case AVSV_CKPT_SG_SU_UNINST_NUM:
	case AVSV_CKPT_SG_FSM_STATE:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_SG_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

		/* SU Async Update messages */
	case AVSV_CKPT_AVD_SU_CONFIG:
	case AVSV_CKPT_SU_SI_CURR_ACTIVE:
	case AVSV_CKPT_SU_SI_CURR_STBY:
	case AVSV_CKPT_SU_ADMIN_STATE:
	case AVSV_CKPT_SU_TERM_STATE:
	case AVSV_CKPT_SU_SWITCH:
	case AVSV_CKPT_SU_OPER_STATE:
	case AVSV_CKPT_SU_PRES_STATE:
	case AVSV_CKPT_SU_READINESS_STATE:
	case AVSV_CKPT_SU_ACT_STATE:
	case AVSV_CKPT_SU_PREINSTAN:
	case AVSV_CKPT_SU_RESTART_COUNT:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_SU_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

		/* SI Async Update messages */
	case AVSV_CKPT_AVD_SI_CONFIG:
	case AVSV_CKPT_SI_SU_CURR_ACTIVE:
	case AVSV_CKPT_SI_SU_CURR_STBY:
	case AVSV_CKPT_SI_SWITCH:
	case AVSV_CKPT_SI_ASSIGNMENT_STATE:
	case AVSV_CKPT_SI_ADMIN_STATE:
	case AVSV_CKPT_SI_ALARM_SENT:
	case AVSV_CKPT_SI_DEP_STATE:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_SI_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVSV_CKPT_AVD_SG_OPER_SU:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_SG_OPER_SU)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVSV_CKPT_AVD_SG_ADMIN_SI:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_SG_ADMIN_SI)
			status = NCSCC_RC_SUCCESS;
		break;

		/* COMP Async Update messages */
	case AVSV_CKPT_AVD_COMP_CONFIG:
	case AVSV_CKPT_COMP_CURR_PROXY_NAME:
	case AVSV_CKPT_COMP_CURR_NUM_CSI_ACTV:
	case AVSV_CKPT_COMP_CURR_NUM_CSI_STBY:
	case AVSV_CKPT_COMP_OPER_STATE:
	case AVSV_CKPT_COMP_READINESS_STATE:
	case AVSV_CKPT_COMP_PRES_STATE:
	case AVSV_CKPT_COMP_RESTART_COUNT:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_COMP_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;
	case AVSV_CKPT_AVD_SI_ASS:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_SI_ASS)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVSV_CKPT_AVD_SI_TRANS:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_SI_TRANS)
			status = NCSCC_RC_SUCCESS;
		break;

	case AVSV_CKPT_AVD_COMP_CS_TYPE_CONFIG:
		if (cb->synced_reo_type >= AVSV_CKPT_AVD_COMP_CS_TYPE_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;
	default:
		LOG_WA("%s: unknown type %u", __FUNCTION__, reo_type);

	}
	return status;
}

