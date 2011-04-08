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
  callback functions at FlexLog Servers. It contains both the
  callbacks to encode the structures at active and decode along with
  building the structures at standby FlexLog Servers. All this
  functionality happens in the context of the MBCSV.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include "dts.h"
#include "dts_imm.h"

static uns32 dtsv_mbcsv_callback(NCS_MBCSV_CB_ARG *arg);
static uns32 dtsv_mbcsv_process_enc_cb(DTS_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uns32 dtsv_mbcsv_process_dec_cb(DTS_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uns32 dtsv_mbcsv_process_peer_info_cb(DTS_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uns32 dtsv_mbcsv_process_notify(DTS_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uns32 dtsv_mbcsv_process_err_ind(DTS_CB *cb, NCS_MBCSV_CB_ARG *arg);
static uns32 dtsv_mbcsv_initialize(DTS_CB *cb);
static uns32 dtsv_mbcsv_open_ckpt(DTS_CB *cb);
static uns32 dtsv_get_mbcsv_sel_obj(DTS_CB *cb);
static uns32 dtsv_mbcsv_close_ckpt(DTS_CB *cb);
static uns32 dtsv_mbcsv_finalize(DTS_CB *cb);
static uns32 dtsv_validate_reo_type_in_csync(DTS_CB *cb, uns32 reo_type);

extern const DTSV_ENCODE_CKPT_DATA_FUNC_PTR dtsv_enc_ckpt_data_func_list[DTSV_CKPT_MSG_MAX];

extern const DTSV_DECODE_CKPT_DATA_FUNC_PTR dtsv_dec_ckpt_data_func_list[DTSV_CKPT_MSG_MAX];

/****************************************************************************\
 * Function: dts_role_change
 *
 * Purpose:  DTSV function to handle DTS role change event received.
 *
 * Input: cb - DTS control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: 
 *
 * 
\**************************************************************************/
uns32 dts_role_change(DTS_CB *cb, SaAmfHAStateT haState)
{
	SaAmfHAStateT prev_haState = cb->ha_state;
	SaAisErrorT error = SA_AIS_OK;
	SVC_KEY nt_key;
	DTS_SVC_REG_TBL *service = NULL;
	OP_DEVICE *device = NULL;

	/*
	 * Validate the role. In case of illegal role setting raise an error.
	 */
	if (((prev_haState == SA_AMF_HA_ACTIVE) && (haState == SA_AMF_HA_STANDBY))
	    || ((prev_haState == SA_AMF_HA_STANDBY) && (haState == SA_AMF_HA_QUIESCED))) {
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_role_change:Invalid role change");
	}

	/*
	 * Handle Active to Active role change.
	 */
	if ((prev_haState == SA_AMF_HA_ACTIVE) && (haState == SA_AMF_HA_ACTIVE)) {
		/* Once it becomes active, it IS in-sync with itself */
		cb->in_sync = TRUE;
		return NCSCC_RC_SUCCESS;
	}

	/*
	 * Handle Stby to Stby role change.
	 */
	if ((prev_haState == SA_AMF_HA_STANDBY) && (haState == SA_AMF_HA_STANDBY)) {
		return NCSCC_RC_SUCCESS;
	}

	/*
	 * Handle Active to Quiesced role change.
	 * Change in the way DTS was handling this case. Now, change the MDS VDSET
	 * role to Quiesced. Don't change MBCSv role. Then wait for MDS callback
	 * with type MDS_CALLBACK_QUIESCED_ACK. Don't change cb->ha_state now.
	 */
	if ((prev_haState == SA_AMF_HA_ACTIVE) && (haState == SA_AMF_HA_QUIESCED)) {
		m_DTS_DBG_SINK(NCSCC_RC_SUCCESS, "dts_role_change: DTS going ACTIVE --> QUIESCED");

		/* Give up our IMM OI implementer role */
		error = dts_saImmOiImplementerClear(cb->immOiHandle);
		if (error != SA_AIS_OK) {
			m_DTS_DBG_SINK(NCSFL_SEV_ERROR, "saImmOiImplementerClear failed");
		}

		/* Change MDS VDEST */
		if (dts_mds_change_role(cb, haState) != NCSCC_RC_SUCCESS) {
			m_LOG_DTS_API(DTS_MDS_ROLE_CHG_FAIL);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_role_change: MDS role change failed");
		} else
			ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_MDS_EVT, DTS_FC_EVT, NCSFL_LC_EVENT, NCSFL_SEV_NOTICE,
				   "TIL", DTS_MDS_VDEST_CHG, haState);

		/* return now */
		return NCSCC_RC_SUCCESS;
	}

	/*
	 * Handle Quiesced to Standby role change.
	 */
	if ((prev_haState == SA_AMF_HA_QUIESCED) && (haState == SA_AMF_HA_STANDBY)) {
		m_DTS_DBG_SINK(NCSCC_RC_SUCCESS, "dts_role_change: DTS going QUIESCED --> STANDBY");
		goto valid_role_change;
	}
	/*
	 * Handle Quiesced to Active role change.
	 */
	/*if((prev_haState == SA_AMF_HA_QUIESCED) && (haState == SA_AMF_HA_ACTIVE))
	   {
	   goto valid_role_change; 
	   } */
	/*
	 * Handle Standby/Quiesced to Active role change. 
	 * Before moving to Active role,
	 * verify that this DTS is sync-ed up with the Active. If yes then only 
	 * set it to Active immidiately. Else restart this node as a new Active.
	 */
	if (((prev_haState == SA_AMF_HA_STANDBY) ||
	     (prev_haState == SA_AMF_HA_QUIESCED)) && (haState == SA_AMF_HA_ACTIVE)) {
		m_DTS_DBG_SINK(NCSCC_RC_SUCCESS, "dts_role_change: DTS got ACTIVE role assignment");
		/* Check whether standby is in-sync or not.
		 * If not, then request AMF to restart this instance 
		 */
		if (cb->in_sync != TRUE) {
			m_LOG_DTS_CHKOP(DTS_ACT_ASSIGN_NOT_INSYNC);
			if (saAmfComponentErrorReport(cb->amf_hdl, &cb->comp_name, 0, SA_AMF_COMPONENT_RESTART, 0) !=
			    SA_AIS_OK)
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
						      "dts_role_change: Failed to send Error report to AMF for Active role assignment during Cold-Sync");
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dts_role_change: Active role assignment during Cold-Sync of Standby received");
		}

		if (dts_handle_fail_over() != NCSCC_RC_SUCCESS) {
			m_LOG_DTS_API(DTS_FAIL_OVER_FAILED);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_role_change: DTS role change to ACTIVE failed");
		}

		/* Change MDS VDEST */
		if (dts_mds_change_role(cb, haState) != NCSCC_RC_SUCCESS) {
			m_LOG_DTS_API(DTS_MDS_ROLE_CHG_FAIL);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_role_change: MDS role change failed");
		} else
			ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_MDS_EVT, DTS_FC_EVT, NCSFL_LC_EVENT, NCSFL_SEV_NOTICE,
				   "TIL", DTS_MDS_VDEST_CHG, haState);

		/* Change MBCSv ckpt role */
		if (dtsv_set_ckpt_role(cb, haState) != NCSCC_RC_SUCCESS) {
			m_LOG_DTS_API(DTS_CKPT_CHG_FAIL);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_role_change:MBCSv ckpt role change failed");
		}

		if (NCSCC_RC_SUCCESS != dtsv_mbcsv_dispatch(cb, SA_DISPATCH_ALL))
			m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_role_change: Error received");

		/*if( dts_handle_fail_over() != NCSCC_RC_SUCCESS )
		   {
		   m_LOG_DTS_API(DTS_FAIL_OVER_FAILED);
		   return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_role_change: DTS fail-over failed");
		   } */
		if (prev_haState == SA_AMF_HA_QUIESCED)
			dts_imm_declare_implementer(cb);

		if (prev_haState == SA_AMF_HA_STANDBY) {
			dts_imm_declare_implementer(cb);

			/* get default global configuration from global policy object */
			if (dts_configure_global_policy() == NCSCC_RC_FAILURE) {
				LOG_ER("Failed to load global log policy object from IMMSv, exiting");
				exit(1);
			}

			/* Loads all NodeLogPolicy objects from IMMSv */
			dts_read_log_policies("OpenSAFDtsvNodeLogPolicy");

			/* Loads all ServiceLogPolicy objects from IMMSv */
			dts_read_log_policies("OpenSAFDtsvServiceLogPolicy");
		}

		/* Once it becomes active, it IS in-sync with itself */
		cb->in_sync = TRUE;

		m_LOG_DTS_API(DTS_FAIL_OVER_SUCCESS);
		/* Change ha_state of dts_cb now */
		cb->ha_state = haState;

		ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_AMF_EVT, DTS_FC_API, NCSFL_LC_STATE, NCSFL_SEV_NOTICE, "TILL",
			   DTS_AMF_ROLE_CHG, prev_haState, haState);

		return NCSCC_RC_SUCCESS;
	}
	/* end of prev_state=stby && hastate=active */
 valid_role_change:
	/* Change MDS VDEST */
	if (dts_mds_change_role(cb, haState) != NCSCC_RC_SUCCESS) {
		m_LOG_DTS_API(DTS_MDS_ROLE_CHG_FAIL);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_role_change: MDS role change failed");
	} else
		ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_MDS_EVT, DTS_FC_EVT, NCSFL_LC_EVENT, NCSFL_SEV_NOTICE, "TIL",
			   DTS_MDS_VDEST_CHG, haState);

	/* Change MBCSv ckpt role */
	if (dtsv_set_ckpt_role(cb, haState) != NCSCC_RC_SUCCESS) {
		m_LOG_DTS_API(DTS_CKPT_CHG_FAIL);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_role_change:MBCSv ckpt role change failed");
	}

	/* Change ha_state of dts_cb now */
	cb->ha_state = haState;

	/* Now the files are opened on demand on active. The file list is 
	freed on the standby & log list in svc reg tbl set to null so that
	a fresh cycle of opening files starts on when the role changes
	*/
	service = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&cb->svc_tbl, NULL);
	while (service != NULL) {
		nt_key.node = service->ntwk_key.node;
		nt_key.ss_svc_id = service->ntwk_key.ss_svc_id;

		device = &service->device;
		device->new_file = TRUE;
		device->cur_file_size = 0;
		device->file_open = FALSE;
		m_DTS_FREE_FILE_LIST(device);
		memset(&device->log_file_list, '\0', sizeof(DTS_FILE_LIST));
		service = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&cb->svc_tbl, (const uns8 *)&nt_key);
	}
	fflush(stdout);

	ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_AMF_EVT, DTS_FC_API, NCSFL_LC_STATE, NCSFL_SEV_NOTICE, "TILL",
		   DTS_AMF_ROLE_CHG, prev_haState, haState);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_register
 *
 * Purpose:  DTSV function to register DTS with MBCSv.
 *
 * Input: cb - DTS control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 dtsv_mbcsv_register(DTS_CB *cb)
{
	uns32 status = NCSCC_RC_SUCCESS;

	/*
	 * Send Async Update count to zero.
	 */
	memset(&cb->async_updt_cnt, 0, sizeof(DTSV_ASYNC_UPDT_CNT));

	/*
	 * Compile Ckpt EDP's
	 */
	if (NCSCC_RC_SUCCESS != (status = dts_compile_ckpt_edp(cb))) {
		/* Log error */ ;
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_register: EDP compilation failed");
		goto done;
	}
	/*
	 * First initialize and then call open.
	 */
	if (NCSCC_RC_SUCCESS != (status = dtsv_mbcsv_initialize(cb))) {
		/* Log error */ ;
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_register: MBCSv initialize failed");
		goto done;
	}

	if (NCSCC_RC_SUCCESS != (status = dtsv_mbcsv_open_ckpt(cb))) {
		/* Log error */ ;
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_register: Checkpoint open failed");
		dtsv_mbcsv_finalize(cb);
		goto done;
	}

	/*
	 * Get MBCSv selection object.
	 */
	if (NCSCC_RC_SUCCESS != (status = dtsv_get_mbcsv_sel_obj(cb))) {
		/* Log error */ ;
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_register: Failed to get MBCSv selection object");
		dtsv_mbcsv_close_ckpt(cb);
		dtsv_mbcsv_finalize(cb);
		goto done;
	}

	/*
	 * Set DTS initial role. ?? Here or somewhere else.
	 */
	if (NCSCC_RC_SUCCESS != (status = dtsv_set_ckpt_role(cb, cb->ha_state))) {
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_register: Failed to set intial role for DTS");
		dtsv_mbcsv_close_ckpt(cb);
		dtsv_mbcsv_finalize(cb);
		goto done;
	}

 done:
	return status;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_deregister
 *
 * Purpose:  DTSV function to deregister DTS with MBCSv.
 *
 * Input: cb - DTS control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 dtsv_mbcsv_deregister(DTS_CB *cb)
{
	uns32 status = NCSCC_RC_SUCCESS;

	/*
	 * First close and then finalize.
	 */
	if (NCSCC_RC_SUCCESS != (status = dtsv_mbcsv_close_ckpt(cb))) {
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_deregister: Close checkpoint failed");
	} else
		m_LOG_DTS_API(DTS_CLOSE_CKPT);

	if (NCSCC_RC_SUCCESS != (status = dtsv_mbcsv_finalize(cb))) {
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_deregister: MBCSv finalize failed");
	} else
		m_LOG_DTS_API(DTS_MBCSV_FIN);

	return status;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_callback
 *
 * Purpose:  DTSV MBCSV call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_mbcsv_callback(NCS_MBCSV_CB_ARG *arg)
{
	uns32 status = NCSCC_RC_SUCCESS;
	DTS_CB *cb;

	/* Smik - get the CB from the global variable for dts_cb in this context */
	cb = &dts_cb;
	if (cb->created != TRUE) {
		/* Log Error msg and free received UBA */
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				      "dtsv_mbcsv_cb: Failed to get cb handle. cb->created is FALSE ");
	}

	m_DTS_LK(&cb->lock);
	m_LOG_DTS_LOCK(DTS_LK_LOCKED, &cb->lock);

	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		status = dtsv_mbcsv_process_enc_cb(cb, arg);
		break;

	case NCS_MBCSV_CBOP_DEC:
		status = dtsv_mbcsv_process_dec_cb(cb, arg);
		break;

	case NCS_MBCSV_CBOP_PEER:
		status = dtsv_mbcsv_process_peer_info_cb(cb, arg);
		break;

	case NCS_MBCSV_CBOP_NOTIFY:
		status = dtsv_mbcsv_process_notify(cb, arg);
		break;

	case NCS_MBCSV_CBOP_ERR_IND:
		status = dtsv_mbcsv_process_err_ind(cb, arg);
		break;

	default:
		status = NCSCC_RC_FAILURE;
		break;
	}

	m_DTS_UNLK(&cb->lock);
	m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &cb->lock);

	return status;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_process_enc_cb
 *
 * Purpose:  DTSV MBCSV encode call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_mbcsv_process_enc_cb(DTS_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uns32 status = NCSCC_RC_SUCCESS, msg_fmt_version;

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.encode.i_peer_version,
					      DTS_MBCSV_VERSION, DTS_MBCSV_VERSION_MIN);

	if (!msg_fmt_version)
		/* Drop The Message */
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_process_enc_cb: Invalid message format version");

	switch (arg->info.encode.io_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		{
			/* Encode Async update message */
			status = dtsv_enc_ckpt_data_func_list[arg->info.encode.io_reo_type] (cb, &arg->info.encode);
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		{
			/* Encode Cold Sync Request message 
			 * Nothing is there to send at this point of time.
			 * But we can set cold_sync_in_progress which indicates that 
			 * cold sync in on.
			 */
			m_LOG_DTS_CHKOP(DTS_CSYNC_REQ_ENC);
			/* Clear the svc_reg datastructures before cold_sync */
			dtsv_clear_registration_table(cb);
			cb->in_sync = FALSE;
			cb->cold_sync_in_progress = TRUE;
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		{
			/* Encode Cold Sync Response message */
			status = dtsv_encode_cold_sync_rsp(cb, &arg->info.encode);
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
		{
			/* Encode Warm Sync Request message 
			 * Nothing is there to send at this point of time.
			 */
			cb->cold_sync_in_progress = TRUE;
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		{
			/* Encode Warm Sync Response message */
			status = dtsv_encode_warm_sync_rsp(cb, &arg->info.encode);
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE;
		}
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
		{
			/* Encode Data Response message */
			status = dtsv_encode_data_sync_rsp(cb, &arg->info.encode);
		}
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
	case NCS_MBCSV_MSG_DATA_REQ:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
	default:
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_process_enc_cb: Invalid message type received");
	}
	return status;

}

/****************************************************************************\
 * Function: dtsv_mbcsv_process_dec_cb
 *
 * Purpose:  DTSV MBCSV decode call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_mbcsv_process_dec_cb(DTS_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uns32 status = NCSCC_RC_SUCCESS, msg_fmt_version;

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.decode.i_peer_version,
					      DTS_MBCSV_VERSION, DTS_MBCSV_VERSION_MIN);

	if (!msg_fmt_version)
		/* Drop The Message */
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_process_dec_cb: Invalid message format version");

	switch (arg->info.decode.i_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		{
			/* Decode Async update message */
			if (cb->in_sync) {
				/* Smik - No need to queue Async messages, process each msg */
				status =
				    dtsv_dec_ckpt_data_func_list[arg->info.decode.i_reo_type] (cb, &arg->info.decode);
			} else {
				/* 
				 * If Cold sync is in progress, then drop the async update for which
				 * cold sync response are yet to come.
				 */
				if (TRUE == cb->cold_sync_in_progress) {
					if (NCSCC_RC_SUCCESS != dtsv_validate_reo_type_in_csync(cb,
												arg->info.decode.
												i_reo_type)) {
						/* Free userbuff and return without decoding */
						ncs_reset_uba(&arg->info.decode.i_uba);
						break;
					}
					/* Decode the message here */
					dtsv_dec_ckpt_data_func_list[arg->info.decode.i_reo_type] (cb,
												   &arg->info.decode);
				}

			}	/*end of else */
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
		{
			/* If cold_sync is not in progress then do a cleanup before 
			   receiving cold_sync response */
			if (cb->cold_sync_in_progress == FALSE)
				dtsv_clear_registration_table(cb);
		}
	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
		{
			/*set cold_sync_in_progress to TRUE to indicate start of cold_sync */
			cb->cold_sync_in_progress = TRUE;
			/* Decode Cold Sync Response message */
			status = dtsv_decode_cold_sync_rsp(cb, &arg->info.decode);

			if (NCSCC_RC_SUCCESS != status) {
				cb->in_sync = FALSE;
				status = NCSCC_RC_FAILURE;
				m_LOG_DTS_CHKOP(DTS_CSYNC_DEC_FAILED);
				/* Smik - Cleanup of svc reg datastruct at standby */
				dtsv_clear_registration_table(cb);
				break;
			}

			if ((NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE == arg->info.decode.i_msg_type) &&
			    (NCSCC_RC_SUCCESS == status)) {
				cb->in_sync = TRUE;
				cb->cold_sync_in_progress = FALSE;
				m_LOG_DTS_CHKOP(DTS_CSYNC_DEC_COMPLETE);
			}

			cb->cold_sync_done = arg->info.decode.i_reo_type;
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
			/* Decode Cold Sync Response message */
			status = dtsv_decode_warm_sync_rsp(cb, &arg->info.decode);

			/* If we find mismatch in data or warm sync fails set in_sync */
			if (NCSCC_RC_FAILURE == status) {
				cb->in_sync = FALSE;
				m_LOG_DTS_CHKOP(DTS_WSYNC_FAILED);
			}
		}
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		{
			/* Decode Data request message */
			status = dtsv_decode_data_req(cb, &arg->info.decode);
		}
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		{
			/* Decode Data response and data response complete message */
			status = dtsv_decode_data_sync_rsp(cb, &arg->info.decode);

			if (NCS_MBCSV_MSG_DATA_RESP_COMPLETE == arg->info.decode.i_msg_type) {
				cb->cold_sync_in_progress = FALSE;
				cb->in_sync = TRUE;
				m_LOG_DTS_CHKOP(DTS_DATA_SYNC_CMPLT);
			}
			cb->cold_sync_done = arg->info.decode.i_reo_type;
		}
		break;

	}
	return status;

}

/****************************************************************************\
 * Function: dtsv_mbcsv_process_peer_info_cb
 *
 * Purpose:  DTSV MBCSV peer Info call back function.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_mbcsv_process_peer_info_cb(DTS_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	/* Compare versions of the peer with self */
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_process_notify_msg
 *
 * Purpose:  DTSV MBCSV process Notify message.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_mbcsv_process_notify(DTS_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_process_err_ind
 *
 * Purpose:  DTSV MBCSV process error indication.
 *
 * Input: arg - MBCSV callback argument pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_mbcsv_process_err_ind(DTS_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	switch (arg->info.error.i_code) {
	case NCS_MBCSV_COLD_SYNC_TMR_EXP:
		m_LOG_DTS_CHKOP(DTS_COLD_SYNC_TIMER_EXP);
		goto send_error;

	case NCS_MBCSV_WARM_SYNC_CMPL_TMR_EXP:
		m_LOG_DTS_CHKOP(DTS_WARM_SYNC_CMPLT_EXP);
		/* Don't send saAmfErrorReport, let the next warm sync take
		 * care of any inconsistencies
		 */
		break;

	case NCS_MBCSV_WARM_SYNC_TMR_EXP:
		m_LOG_DTS_CHKOP(DTS_WARM_SYNC_TIMER_EXP);
		/* Don't send saAmfErrorReport, let the next warm sync take
		 * care of any inconsistencies
		 */
		break;

	case NCS_MBCSV_DATA_RSP_CMPLT_TMR_EXP:
		m_LOG_DTS_CHKOP(DTS_DATA_RESP_CMPLT_EXP);
		goto send_error;

	case NCS_MBCSV_COLD_SYNC_CMPL_TMR_EXP:
		m_LOG_DTS_CHKOP(DTS_COLD_SYNC_CMPLT_EXP);

 send_error:
		/* Send Error report to AMF with RestartRecovery as option */
		if (saAmfComponentErrorReport(cb->amf_hdl, &cb->comp_name, 0, SA_AMF_COMPONENT_RESTART, 0) != SA_AIS_OK)
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dtsv_mbcsv_process_err_ind: Failed to send Error report to AMF");
		break;

	case NCS_MBCSV_DATA_RESP_TERMINATED:
		m_LOG_DTS_CHKOP(DTS_DATA_RESP_TERM);
		break;

	default:
		break;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_initialize
 *
 * Purpose:  Initialize DTSV with MBCSV
 *
 * Input: cb       - DTS control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_mbcsv_initialize(DTS_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	mbcsv_arg.info.initialize.i_service = NCS_SERVICE_ID_DTSV;
	mbcsv_arg.info.initialize.i_mbcsv_cb = dtsv_mbcsv_callback;
	mbcsv_arg.info.initialize.i_version = cb->dts_mbcsv_version;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_initialize: MBCSV Initialize operation failed");
	}

	cb->mbcsv_hdl = mbcsv_arg.info.initialize.o_mbcsv_hdl;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_open_ckpt
 *
 * Purpose:  Open checkpoint.
 *
 * Input: cb   - DTS control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_mbcsv_open_ckpt(DTS_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_OPEN;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.open.i_pwe_hdl = (uns32)cb->mds_hdl;
	    if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_open_ckpt: MBCSV Open operation failed");
	}

	cb->ckpt_hdl = mbcsv_arg.info.open.o_ckpt_hdl;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_set_ckpt_role
 *
 * Purpose:  Set checkpoint role.
 *
 * Input: cb       - DTS control block pointer.
 *        role     - Role to be set.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 dtsv_set_ckpt_role(DTS_CB *cb, uns32 role)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.chg_role.i_ckpt_hdl = cb->ckpt_hdl;
	mbcsv_arg.info.chg_role.i_ha_state = role;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		m_LOG_DTS_API(DTS_AMF_ROLE_FAIL);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_set_ckpt_role: MBCSV Role set operation failed");
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_get_mbcsv_sel_obj
 *
 * Purpose:  Get MBCSv Selection object.
 *
 * Input: cb        - DTS control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_get_mbcsv_sel_obj(DTS_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				      "dtsv_get_mbcsv_sel_obj: MBCSV get selection object operation failed");
	}

	cb->mbcsv_sel_obj = mbcsv_arg.info.sel_obj_get.o_select_obj;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_dispatch
 *
 * Purpose:  Perform dispatch operation on MBCSV selection object.
 *
 * Input: cb        - DTS control block pointer.
 *        sel_obj   - Selection object returned.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 dtsv_mbcsv_dispatch(DTS_CB *cb, uns32 flag)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_DISPATCH;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.dispatch.i_disp_flags = flag;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_dispatch: MBCSV dispatch operation failed");
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_send_ckpt_data
 *
 * Purpose:  Send checkpoint data to standby using the supplied send type.
 *
 * Input: cb        - DTS control block pointer.
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
uns32 dtsv_send_ckpt_data(DTS_CB *cb, uns32 action, MBCSV_REO_HDL reo_hdl, uns32 reo_type, uns32 send_type)
{
	NCS_MBCSV_ARG mbcsv_arg;

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
	/* Smik - List the checkpointed datastructures here */
	switch (reo_type) {
	case DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG:
		cb->async_updt_cnt.dts_svc_reg_tbl_updt++;
		break;

	case DTSV_CKPT_DTA_DEST_LIST_CONFIG:
		cb->async_updt_cnt.dta_dest_list_updt++;
		break;

	case DTSV_CKPT_GLOBAL_POLICY_CONFIG:
		cb->async_updt_cnt.global_policy_updt++;
		break;

	case DTSV_CKPT_DTS_LOG_FILE_CONFIG:
		cb->async_updt_cnt.dts_log_updt++;
		break;

	default:
		return NCSCC_RC_SUCCESS;
	}

	/*
	 * Now send this update.
	 */
	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_send_ckpt_data: MBCSV send data operation failed");
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_close_ckpt
 *
 * Purpose:  Close checkpoint.
 *
 * Input: cb        - DTS control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_mbcsv_close_ckpt(DTS_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_CLOSE;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.close.i_ckpt_hdl = cb->ckpt_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_close_ckpt: MBCSV Close operation failed");
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_finalize
 *
 * Purpose:  Finalize DTSV with MBCSV
 *
 * Input: cb        - DTS control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_mbcsv_finalize(DTS_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_FINALIZE;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_finalize: MBCSV Finalize operation failed");
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_mbcsv_obj_set
 *
 * Purpose:  Set MBCSv objects
 *
 * Input: cb        - DTS control block pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 dtsv_mbcsv_obj_set(DTS_CB *cb, uns32 obj, uns32 val)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_OBJ_SET;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.obj_set.i_ckpt_hdl = cb->ckpt_hdl;
	mbcsv_arg.info.obj_set.i_obj = obj;
	mbcsv_arg.info.obj_set.i_val = val;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_mbcsv_obj_set: MBCSV Set operation failed");
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_send_data_req
 *
 * Purpose:  Encode data request to be sent.
 *
 * Input: cb - DTS CB pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uns32 dtsv_send_data_req(DTS_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;
	NCS_UBAID *uba = NULL;

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;

	uba = &mbcsv_arg.info.send_data_req.i_uba;

	memset(uba, '\0', sizeof(NCS_UBAID));

	mbcsv_arg.info.send_data_req.i_ckpt_hdl = cb->ckpt_hdl;

	if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg)) {
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_send_data_req: Send data failed");
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: dtsv_validate_reo_type_in_csync
 *
 * Purpose:  Vaidate reo_type received during cold sync updates. Return success, *           if cold sync is over for this reo_type. Return failure if standby
 *           is still to get cold sync for this reo_type.
 *
 * Input: cb - DTS CB pointer.
 *        reo_type - reo type need to be validated during cold sync.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
static uns32 dtsv_validate_reo_type_in_csync(DTS_CB *cb, uns32 reo_type)
{
	uns32 status = NCSCC_RC_FAILURE;

	switch (reo_type) {
	case DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG:
		if (cb->cold_sync_done >= DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

	case DTSV_CKPT_DTA_DEST_LIST_CONFIG:
		if (cb->cold_sync_done >= DTSV_CKPT_DTA_DEST_LIST_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

	case DTSV_CKPT_GLOBAL_POLICY_CONFIG:
		if (cb->cold_sync_done >= DTSV_CKPT_GLOBAL_POLICY_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

	case DTSV_CKPT_DTS_LOG_FILE_CONFIG:
		if (cb->cold_sync_done >= DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG)
			status = NCSCC_RC_SUCCESS;
		break;

	}
	return status;
}
