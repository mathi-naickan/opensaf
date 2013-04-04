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
        
This include file contains subroutines for EDS checkpointing operations using
MBCSv.
          
******************************************************************************
*/

#include "eds.h"
#include "logtrace.h"

/*
EDS_CKPT_DATA_HEADER
       4                4               4                 2            
-----------------------------------------------------------------
| ckpt_rec_type | num_ckpt_records | tot_data_len |  checksum   | 
-----------------------------------------------------------------

EDSV_CKPT_COLD_WARM_SYNC_MSG
-----------------------------------------------------------------------------------------------------------------------
| EDS_CKPT_DATA_HEADER|EDSV_CKPT_REC 1st| next |EDSV_CKPT_REC 2nd| next ..|..|..|..|EDSV_CKPT_REC "num_ckpt_records" th |
-----------------------------------------------------------------------------------------------------------------------
*/

/* EDS CKPT ENCODE/DECODE DISPATCHER routine definitions*/

EDS_CKPT_HDLR eds_ckpt_data_handler[EDS_CKPT_MSG_MAX - EDS_CKPT_MSG_BASE] = {
	eds_ckpt_proc_reg_rec,
	eds_ckpt_proc_finalize_rec,
	eds_ckpt_proc_chan_rec,
	eds_ckpt_proc_chan_open_rec,
	eds_ckpt_proc_chan_rec,
	eds_ckpt_proc_chan_close_rec,
	eds_ckpt_proc_chan_unlink_rec,
	eds_ckpt_proc_reten_rec,
	eds_ckpt_proc_subscribe_rec,
	eds_ckpt_proc_unsubscribe_rec,
	eds_ckpt_proc_ret_time_clr_rec,
	eds_ckpt_proc_agent_down_rec
};

/****************************************************************************
 * Name          : edsv_mbcsv_init 
 *
 * Description   : This function initializes the mbcsv interface and
 *                 obtains a selection object from mbcsv.
 *                 
 * Arguments     : EDS_CB * - A pointer to the eds control block.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t eds_mbcsv_init(EDS_CB *eds_cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCS_MBCSV_ARG init_arg, sel_obj_arg;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();

/* Initialize with MBCSv library */

/* init_arg.i_mbcsv_hdl = (NCS_MBCSV_HDL)NULL; */
	init_arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	init_arg.info.initialize.i_mbcsv_cb = eds_mbcsv_callback;
	init_arg.info.initialize.i_version = EDS_MBCSV_VERSION;
	init_arg.info.initialize.i_service = NCS_SERVICE_ID_EDS;

	if (SA_AIS_OK != (rc = ncs_mbcsv_svc(&init_arg))) {
		TRACE_LEAVE2("mbcsv init failed");
		return rc;
	}

	/* Store MBCSv handle in our CB */
	eds_cb->mbcsv_hdl = init_arg.info.initialize.o_mbcsv_hdl;

	/* We shall open checkpoint only once in our life time.  */
	if (NCSCC_RC_SUCCESS != (error = eds_mbcsv_open_ckpt(eds_cb))) {
		TRACE_LEAVE2("checkpoint open failed");
		return rc;
	}

	/* Get MBCSv Selection Object */
	sel_obj_arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	sel_obj_arg.i_mbcsv_hdl = eds_cb->mbcsv_hdl;
	sel_obj_arg.info.sel_obj_get.o_select_obj = 0;

	if (SA_AIS_OK != (rc = ncs_mbcsv_svc(&sel_obj_arg))) {
		TRACE_LEAVE2("mbcsv sel obj get failed");
		return rc;
	}

	/* Update the CB with mbcsv selectionobject */
	eds_cb->mbcsv_sel_obj = sel_obj_arg.info.sel_obj_get.o_select_obj;
	eds_cb->ckpt_state = COLD_SYNC_IDLE;

	TRACE_LEAVE();
	return rc;
}	/* End eds_mbcsv_init */

/****************************************************************************
 * Name          : eds_mbcsv_open_ckpt 
 *
 * Description   : This function opens a checkpoint with mbcsv. 
 *                 
 * Arguments     : EDS_CB * - A pointer to the eds control block.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t eds_mbcsv_open_ckpt(EDS_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	/* Set the checkpoint open arguments */
	mbcsv_arg.i_op = NCS_MBCSV_OP_OPEN;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.open.i_pwe_hdl = (uint32_t)cb->mds_hdl;
	mbcsv_arg.info.open.i_client_hdl = gl_eds_hdl;	/* Can i take like this ? */

	if (SA_AIS_OK != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		TRACE_LEAVE2("checkpoint open failed");
		return NCSCC_RC_FAILURE;
	}

	cb->mbcsv_ckpt_hdl = mbcsv_arg.info.open.o_ckpt_hdl;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}	/*End eds_mbcsv_open_ckpt */

/****************************************************************************
 * Name          : eds_mbcsv_change_HA_state 
 *
 * Description   : This function inform mbcsv of our HA state. 
 *                 All checkpointing operations are triggered based on the 
 *                 state.
 *
 * Arguments     : EDS_CB * - A pointer to the eds control block.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : This function should be ideally called only once, i.e.
 *                 during the first CSI assignment from AVSv  .
 *****************************************************************************/

uint32_t eds_mbcsv_change_HA_state(EDS_CB *cb)
{
	NCS_MBCSV_ARG mbcsv_arg;
	uint32_t rc = SA_AIS_OK;
	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
	TRACE_ENTER();

	/* Set the mbcsv args */
	mbcsv_arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.chg_role.i_ckpt_hdl = cb->mbcsv_ckpt_hdl;
	mbcsv_arg.info.chg_role.i_ha_state = cb->ha_state;

	if (SA_AIS_OK != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		TRACE_LEAVE2("HA state changed failed");
		return NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}	/*End eds_mbcsv_change_HA_state */

/****************************************************************************
 * Name          : eds_mbcsv_change_HA_state 
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
uint32_t eds_mbcsv_dispatch(NCS_MBCSV_HDL mbcsv_hdl)
{
	NCS_MBCSV_ARG mbcsv_arg;
	uint32_t rc = SA_AIS_OK;
	TRACE_ENTER();

	/* Set the mbcsv args */
	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
	mbcsv_arg.i_op = NCS_MBCSV_OP_DISPATCH;
	mbcsv_arg.i_mbcsv_hdl = mbcsv_hdl;
	mbcsv_arg.info.dispatch.i_disp_flags = SA_DISPATCH_ALL;	/*Revisit this. TBD */

	if (SA_AIS_OK != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		TRACE_LEAVE2("mbcsv callback dispatch failed");
		return NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}	/*End eds_mbcsv_dispatch */

/****************************************************************************
 * Name          : eds_mbcsv_callback
 *
 * Description   : This callback is the single entry point for mbcsv to 
 *                 notify eds of all checkpointing operations. 
 *
 * Arguments     : NCS_MBCSV_CB_ARG - Callback Info pertaining to the mbcsv
 *                 event from ACTIVE/STANDBY EDS peer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : Based on the mbcsv message type, the corresponding mbcsv
 *                 message handler shall be invoked.
 *****************************************************************************/
uint32_t eds_mbcsv_callback(NCS_MBCSV_CB_ARG *arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_LEAVE();

	if (arg == NULL) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		/* Encode Request from MBCSv */
		rc = eds_ckpt_encode_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_4("Checkpoint encode failed");
		break;
	case NCS_MBCSV_CBOP_DEC:
		/* Decode Request from MBCSv */
		rc = eds_ckpt_decode_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_4("Decode failed");
		break;
	case NCS_MBCSV_CBOP_PEER:
		/* EDS Peer info from MBCSv */
		rc = eds_ckpt_peer_info_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_4("Peer info callback handler failed");
		break;
	case NCS_MBCSV_CBOP_NOTIFY:
		/* NOTIFY info from EDS peer */
		rc = eds_ckpt_notify_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_4("Notify info callback handler failed");
		break;
	case NCS_MBCSV_CBOP_ERR_IND:
		/* Peer error indication info */
		rc = eds_ckpt_err_ind_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_4("Error Indication callback failed");
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_4("Invalid mbcsv callback type");
		/* Debug Sink here */
		break;
	}			/*End Switch(i_op) */

	TRACE_LEAVE();
	return rc;

}	/* End eds_mbcsv_callback() */

/****************************************************************************
 * Name          : eds_ckpt_encode_cbk_handler
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

uint32_t eds_ckpt_encode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CB *cb = NULL;
	uint16_t mbcsv_version;
	TRACE_ENTER();

	if (cbk_arg == NULL) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	mbcsv_version = m_NCS_MBCSV_FMT_GET(cbk_arg->info.encode.i_peer_version,
					    EDS_MBCSV_VERSION, EDS_MBCSV_VERSION_MIN);
	if (0 == mbcsv_version) {
		LOG_ER("Invalid peer version: %hu", cbk_arg->info.encode.i_peer_version);
		return NCSCC_RC_FAILURE;
	}

	/* Get EDS control block Handle. cbk_arg->arg->i_client_hdl!? */
	if (NULL == (cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		TRACE_LEAVE2("Take handle failed");
		return NCSCC_RC_FAILURE;
	}

	ncshm_give_hdl(gl_eds_hdl);

	switch (cbk_arg->info.encode.io_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		/* Encode async update */
		if ((rc = eds_ckpt_encode_async_update(cb, cb->edu_hdl, cbk_arg)) != NCSCC_RC_SUCCESS)
			TRACE_4("Encode async update failed");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		TRACE("Cold sync request success. (Nothing to do)");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		/* Encode cold sync response */
		rc = eds_ckpt_enc_cold_sync_data(cb, cbk_arg, false);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("code sync resp enc failed");
		} else {
			cb->ckpt_state = COLD_SYNC_COMPLETE;
			LOG_IN("Cold sync complete!");
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
		/* Currently, nothing to be done */
		TRACE("Warm sync req success. (Nothing to do)");
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		/* Encode warm sync 'response' data */
		if ((rc = eds_ckpt_warm_sync_csum_enc_hdlr(cb, cbk_arg)) != NCSCC_RC_SUCCESS)
			TRACE_4("warm sync response failed");
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		TRACE("Data req success. (Nothing to do)");
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		if ((rc = eds_ckpt_enc_cold_sync_data(cb, cbk_arg, true)) != NCSCC_RC_SUCCESS)
			TRACE_4("Cold sync data resp failed");
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		LOG_ER("Invalid message type recieved in ckpt encode cbk: %u", cbk_arg->info.encode.io_msg_type);
		break;
	}			/*End switch(io_msg_type) */

	TRACE_LEAVE();
	return rc;

}	/*End eds_ckpt_encode_cbk_handler() */

/****************************************************************************
 * Name          : eds_ckpt_enc_cold_sync_data
 *
 * Description   : This function encodes cold sync data., viz
 *                 1.REGLIST
 *                 2.CHANNEL RECORDS(worklist node-'channel create'records)
 *                 3.CHANNEL_OPEN RECORDS(chan open instances per channel)
 *                 4.SUBSCRIPTION RECORDS(Subscriptions on all channel)  
 *                 5.RETAINED EVENTS RECORDS.,
 *                 6.Async Update Count
 *                 in that order.
 *                 Each records contain a header specifying the record type
 *                 and number of such records.
 *
 * Arguments     : eds_cb - pointer to the eds control block. 
 *                 cbk_arg - Pointer to NCS_MBCSV_CB_ARG with encode info.
 *                 data_req - Flag to specify if its for cold sync or data
 *                 request for warm sync.
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t eds_ckpt_enc_cold_sync_data(EDS_CB *eds_cb, NCS_MBCSV_CB_ARG *cbk_arg, bool data_req)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	/* asynsc Update Count */
	uint8_t *async_upd_cnt = NULL;
	TRACE_ENTER();

/* Currently, we shall send all data in one send.
 * This shall avoid "delta data" problems that are associated during
 * multiple sends
 */
	/* Encode Registration list */
	rc = eds_edu_enc_reg_list(eds_cb, &cbk_arg->info.encode.io_uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("Reg record encode failed");
		return NCSCC_RC_FAILURE;
	}
	/* Encode Channel list */
	rc = eds_edu_enc_chan_rec(eds_cb, &cbk_arg->info.encode.io_uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("Channel list encode failed");
		return NCSCC_RC_FAILURE;
	}

	/* Encode Channel open list */
	rc = eds_edu_enc_chan_open_list(eds_cb, &cbk_arg->info.encode.io_uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("Channel open list encode failed");
		return NCSCC_RC_FAILURE;
	}

	/* Encode Subscription list */
	rc = eds_edu_enc_subsc_list(eds_cb, &cbk_arg->info.encode.io_uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("subscription list encode failed");
		return NCSCC_RC_FAILURE;
	}

	/* Encode Retained event list */
	rc = eds_edu_enc_reten_list(eds_cb, &cbk_arg->info.encode.io_uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("retained event list encode failed");
		return NCSCC_RC_FAILURE;
	}

	/* Encode the Async Update Count at standby */

	/* This will have the count of async updates that have been sent,
	   this will be 0 initially */
	async_upd_cnt = ncs_enc_reserve_space(&cbk_arg->info.encode.io_uba, sizeof(uint32_t));
	if (async_upd_cnt == NULL) {
		/* Log this error */
		LOG_ER("leap reserve space failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	ncs_encode_32bit(&async_upd_cnt, eds_cb->async_upd_cnt);
	ncs_enc_claim_space(&cbk_arg->info.encode.io_uba, sizeof(uint32_t));

	/* Set response mbcsv msg type to complete */
	if (data_req == true)
		cbk_arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
	else
		cbk_arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;

	TRACE_LEAVE();
	return rc;

}	/*End  eds_ckpt_enc_cold_sync_data() */

/****************************************************************************
 * Name          : eds_edu_enc_reg_list
 *
 * Description   : This function walks through the reglist and encodes all
 *                 records in the reglist, using the edps defined for the
 *                 same.
 *
 * Arguments     : cb - Pointer to EDS control block.
 *                 uba - Pointer to ubaid provided by mbcsv 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/

uint32_t eds_edu_enc_reg_list(EDS_CB *cb, NCS_UBAID *uba)
{
	EDA_REG_REC *reg_rec = NULL;
	EDS_CKPT_REG_MSG *ckpt_reg_rec = NULL;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS, num_rec = 0;
	uint8_t *pheader = NULL;
	EDS_CKPT_HEADER ckpt_hdr;
	TRACE_ENTER();

	/* Prepare reg. structure to encode */
	ckpt_reg_rec = m_MMGR_ALLOC_EDSV_REG_MSG;
	if (ckpt_reg_rec == NULL) {
		LOG_CR("malloc failed for checkpoint reg rec failed");
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}

	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(EDS_CKPT_HEADER));
	if (pheader == NULL) {
		LOG_ER("Reserving space for encoding failed");
		m_MMGR_FREE_EDSV_REG_MSG(ckpt_reg_rec);
		TRACE_LEAVE();
		return (rc = EDU_ERR_MEM_FAIL);
	}
	ncs_enc_claim_space(uba, sizeof(EDS_CKPT_HEADER));

	reg_rec = (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)0);

	/* Walk through the reg list and encode record by record */
	while (reg_rec != NULL) {
		m_EDS_COPY_REG_REC(ckpt_reg_rec, reg_rec);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_reg_rec, uba, EDP_OP_TYPE_ENC, ckpt_reg_rec, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			m_MMGR_FREE_EDSV_REG_MSG(ckpt_reg_rec);
			TRACE_4("Reg record encode failed");
			TRACE_LEAVE();
			return rc;
		}
		++num_rec;

		/* length+=eds_edp_ed_reg_rec(reg_rec,o_ub); */
		reg_rec = (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)&reg_rec->reg_id_Net);
	}			/* End while RegRec */

	/* Encode RegHeader */
	ckpt_hdr.ckpt_rec_type = EDS_CKPT_INITIALIZE_REC;
	ckpt_hdr.num_ckpt_records = num_rec;
	ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */

	eds_enc_ckpt_header(pheader, ckpt_hdr);

	m_MMGR_FREE_EDSV_REG_MSG(ckpt_reg_rec);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}	/* End eds_edu_enc_reg_list() */

/****************************************************************************
 * Name          : eds_edu_enc_chan_rec
 *
 * Description   : This function walks through the worklist and encodes all
 *                 channel(worklist nodes) records using the edps defined for
 *                 the same.
 *
 * Arguments     : cb - pointer to the EDS control block.  
 *                 uba - Pointer to ubaid provided by mbcsv 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : 'Channel' & 'Channel-open' records are treated separately
 *                 so as to distinguish b/w channel-creation(worklist nodes)
 *                 and channel-open instances on that channel.
 ***************************************************************************/

uint32_t eds_edu_enc_chan_rec(EDS_CB *cb, NCS_UBAID *uba)
{
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS, num_rec = 0;
	uint8_t *pheader = NULL;
	EDS_CKPT_CHAN_MSG *chan_rec = NULL;
	EDS_CKPT_HEADER ckpt_hdr;
	EDS_WORKLIST *wp = NULL;
	TRACE_ENTER();

	chan_rec = m_MMGR_ALLOC_EDSV_CHAN_MSG;
	if (chan_rec == NULL) {
		LOG_CR("malloc failed for checkpoint channel message");
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}
	/*Reserve space for "Checkpoint record header */
	pheader = ncs_enc_reserve_space(uba, sizeof(EDS_CKPT_HEADER));
	if (pheader == NULL) {
		m_MMGR_FREE_EDSV_CHAN_MSG(chan_rec);
		LOG_CR("Reserving space to encode chan rec failed");
		TRACE_LEAVE();
		return (rc = EDU_ERR_MEM_FAIL);
	}
	ncs_enc_claim_space(uba, sizeof(EDS_CKPT_HEADER));

	/* Walk through worklist and encode all channel records */
	wp = cb->eds_work_list;	/* Get root pointer to worklist */

	while (wp) {
		memset(chan_rec, 0, sizeof(EDS_CKPT_CHAN_MSG));
		m_EDS_COPY_CHAN_REC(chan_rec, wp);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_chan_rec, uba, EDP_OP_TYPE_ENC, chan_rec, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_EDSV_CHAN_MSG(chan_rec);
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			TRACE_4("checkpoint channel record encode failed");
			TRACE_LEAVE();
			return rc;
		}
		++num_rec;
		wp = wp->next;

	}			/*End while wp */

	/* Encode header */
	ckpt_hdr.ckpt_rec_type = EDS_CKPT_CHAN_REC;
	ckpt_hdr.num_ckpt_records = num_rec;
	ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */
	eds_enc_ckpt_header(pheader, ckpt_hdr);

	/* if (num_rec == 0)
	   ;;* No channels existed */

	m_MMGR_FREE_EDSV_CHAN_MSG(chan_rec);

	TRACE_LEAVE();
	return rc;
}	/* End eds_edu_enc_chan_rec */

/****************************************************************************
 * Name          : eds_edu_enc_chan_open_list
 *
 * Description   : This function walks through the worklist to encode
 *                 channel-open records in each worklist node(channel) and 
 *                 using the edps defined for the same.
 *
 * Arguments     : cb - pointer to the EDS control block.  
 *                 uba - Pointer to ubaid provided by mbcsv 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/

uint32_t eds_edu_enc_chan_open_list(EDS_CB *cb, NCS_UBAID *uba)
{
	CHAN_OPEN_REC *copen_rec = NULL;
	EDS_CKPT_CHAN_OPEN_MSG *ckpt_copen_rec = NULL;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS, num_rec = 0;
	uint8_t *pheader = NULL;
	EDS_WORKLIST *wp = NULL;
	EDS_CKPT_HEADER ckpt_hdr;
	TRACE_ENTER();

	ckpt_copen_rec = m_MMGR_ALLOC_EDSV_CHAN_OPEN_MSG;

	if (ckpt_copen_rec == NULL) {
		LOG_CR("Reserving space to encode chan open rec failed");
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}

	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(EDS_CKPT_HEADER));
	if (pheader == NULL) {
		m_MMGR_FREE_EDSV_CHAN_OPEN_MSG(ckpt_copen_rec);
		LOG_CR("Reserving space to encode chan open rec failed");
		return (rc = EDU_ERR_MEM_FAIL);
	}
	ncs_enc_claim_space(uba, sizeof(EDS_CKPT_HEADER));

	/* Walk through worklist and encode all channel records */
	wp = cb->eds_work_list;	/* Get root pointer to worklist */

	while (wp) {

		copen_rec = (CHAN_OPEN_REC *)ncs_patricia_tree_getnext(&wp->chan_open_rec, (uint8_t *)0);
		while (copen_rec != NULL) {
			m_EDS_COPY_CHAN_OPEN_REC(ckpt_copen_rec, copen_rec, wp);
			rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_chan_open_rec, uba,
					    EDP_OP_TYPE_ENC, ckpt_copen_rec, &ederror);

			if (rc != NCSCC_RC_SUCCESS) {
				m_MMGR_FREE_EDSV_CHAN_OPEN_MSG(ckpt_copen_rec);
				m_NCS_EDU_PRINT_ERROR_STRING(ederror);
				TRACE_4("Checkpoint channel open record encode failed");
				TRACE_LEAVE();
				return rc;
			}
			/* length=eds_edp_ed_chan_open_list(chan_open_rec,o_ub); */
			++num_rec;

			copen_rec = (CHAN_OPEN_REC *)ncs_patricia_tree_getnext(&wp->chan_open_rec,
									       (uint8_t *)&copen_rec->copen_id_Net);
		}		/* End while copen_rec */
		wp = wp->next;
	}			/*Enc while wp */

	/* Encode chanopen header */
	ckpt_hdr.ckpt_rec_type = EDS_CKPT_CHAN_OPEN_REC;
	ckpt_hdr.num_ckpt_records = num_rec;
	ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */
	eds_enc_ckpt_header(pheader, ckpt_hdr);
	/* rec_count == 0, then no chan open records to checkpoint */

	m_MMGR_FREE_EDSV_CHAN_OPEN_MSG(ckpt_copen_rec);

	TRACE_LEAVE();
	return rc;
}	/* End eds_edu_enc_chan_open_list() */

/****************************************************************************
 * Name          : eds_edu_enc_subsc_list
 *
 * Description   : This function walks through the worklist to encode
 *                 subscription records per channel open instance.
 *
 * Arguments     : cb - pointer to the EDS control block.
 *                 uba - Pointer to NCS_UBAID 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/

uint32_t eds_edu_enc_subsc_list(EDS_CB *cb, NCS_UBAID *uba)
{
	SUBSC_REC *sub_rec = NULL;
	CHAN_OPEN_REC *copen_rec = NULL;
	EDS_CKPT_SUBSCRIBE_MSG *ckpt_srec = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS, num_rec = 0;
	uint8_t *pheader = NULL;
	EDS_CKPT_HEADER ckpt_hdr;
	EDS_WORKLIST *wp = NULL;
	TRACE_ENTER();

	ckpt_srec = m_MMGR_ALLOC_EDSV_SUBSCRIBE_EVT_MSG;
	if (ckpt_srec == NULL) {
		LOG_CR("Reserving space to encode subscriptionrec failed");
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}
	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(EDS_CKPT_HEADER));
	if (pheader == NULL) {
		m_MMGR_FREE_EDSV_SUBSCRIBE_EVT_MSG(ckpt_srec);
		LOG_CR("Reserving space for encoding subsc rec failed");
		TRACE_LEAVE();
		return (rc = EDU_ERR_MEM_FAIL);
	}
	ncs_enc_claim_space(uba, sizeof(EDS_CKPT_HEADER));

	/* Walk through the worklist and encode subscription records */

	wp = cb->eds_work_list;	/* Get root pointer to worklist */

	while (wp) {
		copen_rec = (CHAN_OPEN_REC *)ncs_patricia_tree_getnext(&wp->chan_open_rec, (uint8_t *)0);
		while (copen_rec) {
			/* First fill the reg_id associated with this copen */
			ckpt_srec->data.reg_id = copen_rec->reg_id;

			/* From each channel entry, get the subscription record */
			sub_rec = copen_rec->subsc_rec_head;
			while (sub_rec) {
				m_EDS_COPY_SUBSC_REC(ckpt_srec, sub_rec);	/*Copy into ckpt msg */
				rc = eds_ckpt_enc_subscribe_msg(uba, ckpt_srec);
				if (!rc) {	/* Zero bytes. Encode failed. */
					TRACE_4("Checkpoint subscription record encode failed");
					m_MMGR_FREE_EDSV_SUBSCRIBE_EVT_MSG(ckpt_srec);
					TRACE_LEAVE();
					return (NCSCC_RC_FAILURE);
				}
				rc = NCSCC_RC_SUCCESS;
				++num_rec;
				sub_rec = sub_rec->next;
			}	/*End while sub_rec */
			copen_rec = (CHAN_OPEN_REC *)ncs_patricia_tree_getnext(&wp->chan_open_rec,
									       (uint8_t *)&copen_rec->copen_id_Net);
		}		/*End while co */
		wp = wp->next;
	}			/*End while wp */

	/* Encode subscription header */
	ckpt_hdr.ckpt_rec_type = EDS_CKPT_SUBSCRIBE_REC;
	ckpt_hdr.num_ckpt_records = num_rec;
	ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */
	eds_enc_ckpt_header(pheader, ckpt_hdr);

	m_MMGR_FREE_EDSV_SUBSCRIBE_EVT_MSG(ckpt_srec);

	TRACE_LEAVE();
	return rc;

}	/*End eds_edu_enc_subsc_list() */

/****************************************************************************
 * Name          : eds_edu_enc_reten_list
 *
 * Description   : This function encodes retained events records stored on
 *                 all channels.
 *
 * Arguments     : cb - pointer to the EDS control block.
 *                 uba - Pointer to NCS_UBAID 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/

uint32_t eds_edu_enc_reten_list(EDS_CB *cb, NCS_UBAID *uba)
{
	EDS_RETAINED_EVT_REC *ret_rec = NULL;
	EDS_CKPT_RETAIN_EVT_MSG *ckpt_reten_rec = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS, num_rec = 0;
	uint8_t *pheader = NULL;
	EDS_CKPT_HEADER ckpt_hdr;
	uint32_t remaining_time = 0;
	SaUint8T list_iter;
	EDS_WORKLIST *wp = NULL;
	TRACE_ENTER();

	ckpt_reten_rec = m_MMGR_ALLOC_EDSV_RETAIN_EVT_MSG;
	if (ckpt_reten_rec == NULL) {
		LOG_CR("malloc for retained event record failed");
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}

	/*Reserve space for "Checkpoint Message Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(EDS_CKPT_HEADER));
	if (pheader == NULL) {
		m_MMGR_FREE_EDSV_RETAIN_EVT_MSG(ckpt_reten_rec);
		LOG_CR("Reserving space for encoding reten list failed");
		TRACE_LEAVE();
		return (rc = EDU_ERR_MEM_FAIL);
	}
	ncs_enc_claim_space(uba, sizeof(EDS_CKPT_HEADER));

	wp = cb->eds_work_list;	/* Get the root pointer of the Worklist */
	/* Walk through the retained event list and encode retention records */
	while (wp) {
		for (list_iter = SA_EVT_HIGHEST_PRIORITY; list_iter <= SA_EVT_LOWEST_PRIORITY; list_iter++) {
			ret_rec = wp->ret_evt_list_head[list_iter];	/* calculate new time and encode */
			while (ret_rec) {
				m_EDS_COPY_RETEN_REC(ckpt_reten_rec, ret_rec);
				if (ret_rec->retentionTime == SA_TIME_MAX) {
					ckpt_reten_rec->data.retention_time = SA_TIME_MAX;
				}
				rc = eds_ckpt_enc_reten_msg(uba, ckpt_reten_rec);
				if (!rc) {	/*Encode failed.Zero bytes */
					m_MMGR_FREE_EDSV_RETAIN_EVT_MSG(ckpt_reten_rec);
					TRACE_4("Checkpoint retention event record encode failed");
					TRACE_LEAVE();
					return (NCSCC_RC_FAILURE);
				}
				rc = NCSCC_RC_SUCCESS;
				++num_rec;
				ret_rec = ret_rec->next;
			}	/*End while ret_rec */
		}
		wp = wp->next;
	}
	/* Fill header */
	ckpt_hdr.ckpt_rec_type = EDS_CKPT_RETEN_REC;	/*Same as Retention Record */
	ckpt_hdr.num_ckpt_records = num_rec;
	ckpt_hdr.data_len = 0;	/*Not in Use for Cold Sync */
	eds_enc_ckpt_header(pheader, ckpt_hdr);

	m_MMGR_FREE_EDSV_RETAIN_EVT_MSG(ckpt_reten_rec);

	TRACE_LEAVE();
	return rc;

}	/* End eds_edu_enc_reten_list */

/****************************************************************************
 * Name          : eds_ckpt_encode_async_update
 *
 * Description   : This function encodes data to be sent as an async update.
 *                 The caller of this function would set the address of the
 *                 record to be encoded in the reo_hdl field(while invoking
 *                 SEND_CKPT option of ncs_mbcsv_svc. 
 *
 * Arguments     : cb - pointer to the EDS control block.
 *                 cbk_arg - Pointer to NCS MBCSV callback argument struct.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t eds_ckpt_encode_async_update(EDS_CB *eds_cb, EDU_HDL edu_hdl, NCS_MBCSV_CB_ARG *cbk_arg)
{
	EDS_CKPT_DATA *data = NULL;
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* Set reo_hdl from callback arg to ckpt_rec */
	data = (EDS_CKPT_DATA *)(long)cbk_arg->info.encode.io_reo_hdl;
	if (data == NULL) {
		LOG_WA("Async update encode reo handle is NULL");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	if (data->header.ckpt_rec_type == EDS_CKPT_SUBSCRIBE_REC) {
		/* Encode header first */
		rc = m_NCS_EDU_EXEC(&edu_hdl, eds_edp_ed_header_rec,
				    &cbk_arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &data->header, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			/* m_MMGR_FREE_EDSV_CKPT_DATA(data); */
			LOG_WA("async update subscription header encode failed");
			TRACE_LEAVE();
			return rc;
		}
		/* Encode subscription data */
		rc = eds_ckpt_enc_subscribe_msg(&cbk_arg->info.encode.io_uba,
						(EDS_CKPT_SUBSCRIBE_MSG *)&data->ckpt_rec.subscribe_rec);
		if (!rc) {	/* Encode failed. */
			/* m_MMGR_FREE_EDSV_CKPT_DATA(data); */
			LOG_WA("async update subscription record encode failed");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
	} else if (data->header.ckpt_rec_type == EDS_CKPT_RETEN_REC) {
		/* Encode header first */
		rc = m_NCS_EDU_EXEC(&edu_hdl, eds_edp_ed_header_rec,
				    &cbk_arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &data->header, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			/* m_MMGR_FREE_EDSV_CKPT_DATA(data); */
			LOG_WA("async update retention header encode failed");
			TRACE_LEAVE();
			return rc;
		}
		/* Encode publish data */
		rc = eds_ckpt_enc_reten_msg(&cbk_arg->info.encode.io_uba,
					    (EDS_CKPT_RETAIN_EVT_MSG *)&data->ckpt_rec.retain_evt_rec);
		if (!rc) {	/* Encode failed. */
			/*m_MMGR_FREE_EDSV_CKPT_DATA(data); */
			TRACE_LEAVE();
			LOG_WA("async update retained event record encode failed");
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
	} else {
		/* Encode async record,except publish & subscribe */
		rc = m_NCS_EDU_EXEC(&edu_hdl, eds_edp_ed_ckpt_msg,
				    &cbk_arg->info.encode.io_uba, EDP_OP_TYPE_ENC, data, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			/*m_MMGR_FREE_EDSV_CKPT_DATA(data); */
			LOG_WA("Encode failed for async update");
			TRACE_LEAVE();
			return rc;
		}
	}
	/* Update the Async Update Count at standby */
	eds_cb->async_upd_cnt++;
	/*m_MMGR_FREE_EDSV_CKPT_DATA(data); */

	TRACE_LEAVE();
	return rc;
}	/*End eds_ckpt_encode_async_update() */

/****************************************************************************
 * Name          : eds_ckpt_decode_cbk_handler
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

uint32_t eds_ckpt_decode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CB *cb = NULL;
	uint16_t msg_fmt_version;
	TRACE_ENTER();

	if (cbk_arg == NULL) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	msg_fmt_version = m_NCS_MBCSV_FMT_GET(cbk_arg->info.decode.i_peer_version,
					      EDS_MBCSV_VERSION, EDS_MBCSV_VERSION_MIN);
	if (0 == msg_fmt_version) {
		LOG_ER("decode callback, unsupported version");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/* Get EDS control block Handle */
	if (NULL == (cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		TRACE_LEAVE2("handle take on global handle failed");
		return NCSCC_RC_FAILURE;
	}

	ncshm_give_hdl(gl_eds_hdl);

	switch (cbk_arg->info.decode.i_msg_type) {
	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		TRACE("cold sync req decode success (nothing to be done)");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
		if (cb->ckpt_state != COLD_SYNC_COMPLETE) {	/*this check is needed to handle repeated requests */
			if ((rc = eds_ckpt_decode_cold_sync(cb, cbk_arg)) != NCSCC_RC_SUCCESS) {
				TRACE("Cold sync response decode processing failed");
			} else {
				cb->ckpt_state = COLD_SYNC_COMPLETE;
				TRACE("Cold sync response complete");
			}
		}
		break;

	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		if ((rc = eds_ckpt_decode_async_update(cb, cbk_arg)) != NCSCC_RC_SUCCESS) {
			TRACE(" Async Update decode processing failed");
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
		/* Currently, nothing to be done */
		TRACE("warm sync decode request success (nothing to be done)");
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
		/* Decode and compare checksums */
		if ((rc = eds_ckpt_warm_sync_csum_dec_hdlr(cb, &cbk_arg->info.decode.i_uba)) != NCSCC_RC_SUCCESS) {
			TRACE("Warm sync response complete decode csum processing failed");
		}
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		TRACE("data request decode request success (nothing to be done)");
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		if ((rc = eds_ckpt_decode_cold_sync(cb, cbk_arg)) != NCSCC_RC_SUCCESS) {
			TRACE("Data response decode processing failed");
		}
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		LOG_ER("Bad message type received in ckpt decode cbk");
		break;
	}			/*End switch(io_msg_type) */

	TRACE_LEAVE();
	return rc;

}	/*End eds_ckpt_decode_cbk_handler() */

/****************************************************************************
 * Name          : eds_ckpt_decode_async_update 
 *
 * Description   : This function decodes async update data, based on the 
 *                 record type contained in the header. 
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with decode info
 *                 cb - pointer to eds cb.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t eds_ckpt_decode_async_update(EDS_CB *cb, NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	EDS_CKPT_DATA *ckpt_msg = NULL;
	EDS_CKPT_HEADER *hdr = NULL;
	EDS_CKPT_REG_MSG *reg_rec = NULL;
	EDS_CKPT_CHAN_MSG *chan_rec = NULL;
	EDS_CKPT_CHAN_CLOSE_MSG *chan_close = NULL;
	EDS_CKPT_CHAN_UNLINK_MSG *chan_unlink = NULL;
	EDS_CKPT_UNSUBSCRIBE_MSG *unsubscribe = NULL;
	EDS_CKPT_RETENTION_TIME_CLEAR_MSG *reten_clear = NULL;
	EDS_CKPT_FINALIZE_MSG *finalize = NULL;
	MDS_DEST *agent_dest = NULL;
	long msg_hdl = 0;
	TRACE_ENTER();

	/* Allocate memory to hold the checkpoint message */
	ckpt_msg = m_MMGR_ALLOC_EDSV_CKPT_DATA;
	memset(ckpt_msg, 0, sizeof(EDS_CKPT_DATA));

	/* Decode the message header */
	hdr = &ckpt_msg->header;
	rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_header_rec, &cbk_arg->info.decode.i_uba,
			    EDP_OP_TYPE_DEC, &hdr, &ederror);
	if (rc != NCSCC_RC_SUCCESS) {
		m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
		m_NCS_EDU_PRINT_ERROR_STRING(ederror);
		LOG_WA("async update header decode failed");
		TRACE_LEAVE();
		return rc;
	}
	ederror = 0;

	/* Call decode routines appropriately */
	switch (hdr->ckpt_rec_type) {
	case EDS_CKPT_INITIALIZE_REC:
		reg_rec = &ckpt_msg->ckpt_rec.reg_rec;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_reg_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &reg_rec, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			LOG_WA("async update initialize rec decode failed");
			TRACE_LEAVE();
			return rc;
		}
		break;
	case EDS_CKPT_FINALIZE_REC:
		reg_rec = &ckpt_msg->ckpt_rec.reg_rec;
		finalize = &ckpt_msg->ckpt_rec.finalize_rec;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_finalize_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &finalize, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			LOG_WA("async update finalize rec decode failed");
			TRACE_LEAVE();
			return rc;
		}
		break;
	case EDS_CKPT_CHAN_REC:
	case EDS_CKPT_CHAN_OPEN_REC:
	case EDS_CKPT_ASYNC_CHAN_OPEN_REC:
		chan_rec = &ckpt_msg->ckpt_rec.chan_rec;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_chan_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &chan_rec, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			LOG_WA("async update chan open rec decode failed");
			TRACE_LEAVE();
			return rc;
		}
		break;
	case EDS_CKPT_CHAN_CLOSE_REC:
		chan_close = &ckpt_msg->ckpt_rec.chan_close_rec;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_chan_close_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &chan_close, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			LOG_WA("async update chan close rec decode failed");
			TRACE_LEAVE();
			return rc;
		}
		break;
	case EDS_CKPT_CHAN_UNLINK_REC:
		chan_unlink = &ckpt_msg->ckpt_rec.chan_unlink_rec;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_chan_ulink_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &chan_unlink, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			LOG_WA("async update chan unlink rec decode failed");
			TRACE_LEAVE();
			return rc;
		}
		break;
	case EDS_CKPT_SUBSCRIBE_REC:
		msg_hdl = (long)ckpt_msg;
		rc = eds_dec_subscribe_msg(&cbk_arg->info.decode.i_uba, msg_hdl, true);	/* think of passing subsribe rec hdl itself */
		if (!rc) {
			m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
			LOG_WA("async update subscribe rec decode failed");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
		break;
	case EDS_CKPT_RETEN_REC:
		msg_hdl = (long)ckpt_msg;
		rc = eds_dec_publish_msg(&cbk_arg->info.decode.i_uba, msg_hdl, true);
		if (!rc) {
			m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
			LOG_WA("async update reten rec decode failed");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
		break;
	case EDS_CKPT_UNSUBSCRIBE_REC:
		unsubscribe = &ckpt_msg->ckpt_rec.unsubscribe_rec;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_usubsc_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &unsubscribe, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			LOG_WA("async update unsubscribe rec decode failed");
			TRACE_LEAVE();
			return rc;
		}
		break;
	case EDS_CKPT_RETENTION_TIME_CLR_REC:
		reten_clear = &ckpt_msg->ckpt_rec.reten_time_clr_rec;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_ret_clr_rec, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &reten_clear, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			LOG_WA("async update reten time clear rec decode failed");
			TRACE_LEAVE();
			return rc;
		}
		break;
	case EDS_CKPT_AGENT_DOWN:
		agent_dest = &ckpt_msg->ckpt_rec.agent_dest;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, ncs_edp_mds_dest, &cbk_arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &agent_dest, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
			m_NCS_EDU_PRINT_ERROR_STRING(ederror);
			LOG_WA("async update agent down rec decode failed");
			TRACE_LEAVE();
			return rc;
		}
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		LOG_ER("async update decode: unknown record type");
		m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);
		TRACE_LEAVE();
		return rc;
		break;
	}			/*end switch */

	rc = eds_process_ckpt_data(cb, ckpt_msg);
	/* Update the Async Update Count at standby */
	cb->async_upd_cnt++;

	if (rc != NCSCC_RC_SUCCESS)
		TRACE("async update decode post processing failed");

	m_MMGR_FREE_EDSV_CKPT_DATA(ckpt_msg);

	TRACE_LEAVE();
	return rc;
	/* if failure, should an indication be sent to active ? */
}	/*End eds_ckpt_decode_async_update() */

/****************************************************************************
 * Name          : eds_ckpt_decode_cold_sync 
 *
 * Description   : This function decodes async update data, based on the 
 *                 record type contained in the header. 
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with decode info
 *                 cb - pointer to eds cb.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : COLD SYNC RECORDS are expected in an order
 *                 1. REG RECORDS
 *                 2. CHANNEL RECORDS
 *                 3. CHANNEL OPEN RECORDS
 *                 4. SUBSCRIPTION RECORDS
 *                 5. RETAINED EVENT RECORDS 
 *                 
 *                 For each record type,
 *                     a) decode header.
 *                     b) decode individual records for 
 *                        header->num_records times, 
 *****************************************************************************/

uint32_t eds_ckpt_decode_cold_sync(EDS_CB *cb, NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	EDS_CKPT_DATA *data = NULL;
/*  NCS_UBAID *uba=NULL; */
	uint32_t num_rec = 0;
	long msg_hdl = 0;
	EDS_CKPT_REG_MSG *reg_rec = NULL;
	EDS_CKPT_CHAN_MSG *chan_rec = NULL;
	EDS_CKPT_CHAN_OPEN_MSG *copen_rec = NULL;
	uint32_t num_of_async_upd;
	uint8_t *ptr;
	uint8_t data_cnt[16];
	TRACE_ENTER();

	/* EDS_CKPT_RETAIN_EVT_MSG *reten_rec=NULL; */

	/* 
	 * 
	 ----------------------------------------------------------------------------------------
	 | Header|RegRecords1..n|Header|chanRecords1..n|Header|subscRecords1..n|Header|Reten1..n |
	 ----------------------------------------------------------------------------------------
	 */
	while (1) {

		TRACE("Cold sync decode start");
		/* Allocate memory to hold the checkpoint Data */
		data = m_MMGR_ALLOC_EDSV_CKPT_DATA;
		if (data == NULL) {
			LOG_CR("malloc failed for checkpoint data");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		memset(data, 0, sizeof(EDS_CKPT_DATA));
		/* Decode the current message header */

		if ((rc = eds_dec_ckpt_header(&cbk_arg->info.decode.i_uba, &data->header)) != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_EDSV_CKPT_DATA(data);
			LOG_WA("Decoding checkpoint header failed");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		/* Check if the first in the order of records is reg record */
		if (data->header.ckpt_rec_type != EDS_CKPT_INITIALIZE_REC) {
			m_MMGR_FREE_EDSV_CKPT_DATA(data);
			LOG_WA("Unexpected record. Cold sync decode failed. Expecting reg record");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}

		/* Process the reg_records */
		num_rec = data->header.num_ckpt_records;
		while (num_rec) {
			reg_rec = &data->ckpt_rec.reg_rec;
			rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_reg_rec, &cbk_arg->info.decode.i_uba,
					    EDP_OP_TYPE_DEC, &reg_rec, &ederror);

			if (rc != NCSCC_RC_SUCCESS) {
				m_MMGR_FREE_EDSV_CKPT_DATA(data);
				m_NCS_EDU_PRINT_ERROR_STRING(ederror);
				LOG_WA("Cold sync decode failed for reg record");
				TRACE_LEAVE();
				return rc;
			}
			/* Update our database */
			rc = eds_process_ckpt_data(cb, data);

			if (rc != NCSCC_RC_SUCCESS)
				TRACE("DB update failed for reg record");

			memset(&data->ckpt_rec, 0, sizeof(data->ckpt_rec));
			--num_rec;
		}		/*End while, reg records */

		/* Done with reg_records,Now decode channel records. */
		/*Decode channel records header */

		if ((rc = eds_dec_ckpt_header(&cbk_arg->info.decode.i_uba, &data->header)) != NCSCC_RC_SUCCESS) {
			rc = NCSCC_RC_FAILURE;
			LOG_WA("Decoding checkpoint header failed for channel rec");
			goto err;
		}
		/* Check if record type is channel */
		if (data->header.ckpt_rec_type != EDS_CKPT_CHAN_REC) {
			rc = NCSCC_RC_FAILURE;
			LOG_WA("Unexpected record. Cold sync decode failed. Expecting channel record");
			goto err;
		}

		/* Decode channel records. Realloc data here */
		num_rec = data->header.num_ckpt_records;
		while (num_rec) {
			chan_rec = &data->ckpt_rec.chan_rec;
			rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_chan_rec, &cbk_arg->info.decode.i_uba,
					    EDP_OP_TYPE_DEC, &chan_rec, &ederror);

			if (rc != NCSCC_RC_SUCCESS) {
				LOG_WA("cold sync decode chan rec failed");
				m_NCS_EDU_PRINT_ERROR_STRING(ederror);
				rc = NCSCC_RC_FAILURE;
				goto err;
			}
			/* Update our database */
			rc = eds_process_ckpt_data(cb, data);
			if (rc != NCSCC_RC_SUCCESS)
				TRACE("DB update failed for channel record");

			memset(&data->ckpt_rec, 0, sizeof(data->ckpt_rec));
			--num_rec;
		}		/*End while, chan records */

		/* Done with chan_records,Now decode channel open records. */
		/* Decode channel-open records header */

		if ((rc = eds_dec_ckpt_header(&cbk_arg->info.decode.i_uba, &data->header)) != NCSCC_RC_SUCCESS) {
			rc = NCSCC_RC_FAILURE;
			LOG_WA("cold sync decode failed for checkpoint header");
			goto err;
		}
		/* Check if record type is channel open */
		if (data->header.ckpt_rec_type != EDS_CKPT_CHAN_OPEN_REC) {
			rc = NCSCC_RC_FAILURE;
			LOG_WA("Unexpected record. Cold sync decode failed. Expecting channel open record");
			goto err;
		}
		/* Decode channel open records. Realloc data here */
		num_rec = data->header.num_ckpt_records;
		while (num_rec) {
			copen_rec = &data->ckpt_rec.chan_open_rec;
			rc = m_NCS_EDU_EXEC(&cb->edu_hdl, eds_edp_ed_chan_open_rec, &cbk_arg->info.decode.i_uba,
					    EDP_OP_TYPE_DEC, &copen_rec, &ederror);

			if (rc != NCSCC_RC_SUCCESS) {
				m_NCS_EDU_PRINT_ERROR_STRING(ederror);
				LOG_WA("cold sync decode failed for channel open record");
				rc = NCSCC_RC_FAILURE;
				goto err;
			}
			/* Update our database */
			rc = eds_process_ckpt_data(cb, data);
			if (rc != NCSCC_RC_SUCCESS)
				TRACE("DB update failed for channel open record");

			memset(&data->ckpt_rec, 0, sizeof(data->ckpt_rec));
			--num_rec;
		}		/*End while, chan_open records */

		/* Done with channel_records,Now decode subscription records. */
		if ((rc = eds_dec_ckpt_header(&cbk_arg->info.decode.i_uba, &data->header)) != NCSCC_RC_SUCCESS) {
			rc = NCSCC_RC_FAILURE;
			LOG_WA("cold sync decode failed for checkpoint header");
			goto err;
		}
		/* Check if record type is Subscribe */
		if (data->header.ckpt_rec_type != EDS_CKPT_SUBSCRIBE_REC) {
			rc = NCSCC_RC_FAILURE;
			LOG_WA("Unexpected record. Cold sync decode failed. Expecting subscription record");
			goto err;
		}

		/* Decode subscription records. Realloc data here */
		num_rec = data->header.num_ckpt_records;
		while (num_rec) {
			msg_hdl = (long)data;
			rc = eds_dec_subscribe_msg(&cbk_arg->info.decode.i_uba, msg_hdl, true);
			if (!rc) {
				rc = NCSCC_RC_FAILURE;
				LOG_WA("cold sync decode failed for subscription record");
				goto err;
			}
			/* Update our database */
			rc = eds_process_ckpt_data(cb, data);

			if (rc != NCSCC_RC_SUCCESS)
				TRACE("DB update failed for subscription record");
			
			memset(&data->ckpt_rec, 0, sizeof(data->ckpt_rec));
			--num_rec;
		}		/*End while, Subscription records */

		/*Decode retention records header */
		if ((rc = eds_dec_ckpt_header(&cbk_arg->info.decode.i_uba, &data->header)) != NCSCC_RC_SUCCESS) {
			rc = NCSCC_RC_FAILURE;
			LOG_WA("cold sync decode failed for checkpoint header");
			goto err;
		}
		/* Check if record type is retention */
		if (data->header.ckpt_rec_type != EDS_CKPT_RETEN_REC) {
			rc = NCSCC_RC_FAILURE;
			LOG_WA("Unexpected record. Cold sync decode failed. Expecting retention record");
			goto err;
		}
		/* Decode Retention records. Realloc data here */
		num_rec = data->header.num_ckpt_records;
		if (num_rec != 0) {
			while (num_rec) {
				msg_hdl = (long)data;
				rc = eds_dec_publish_msg(&cbk_arg->info.decode.i_uba, msg_hdl, true);
				if (!rc) {	/*Encode failed.Zero bytes */
					m_NCS_EDU_PRINT_ERROR_STRING(ederror);
					rc = NCSCC_RC_FAILURE;
					LOG_WA("cold sync decode failed for publish rec");
					goto err;
				}
				/* Update our database */
				rc = eds_process_ckpt_data(cb, data);

				if (rc != NCSCC_RC_SUCCESS)
					TRACE("DB update failed for retained events record");

				memset(&data->ckpt_rec, 0, sizeof(data->ckpt_rec));
				--num_rec;
			}	/*End while, Retention records */
		}

		/* Get the async update count */
		ptr = ncs_dec_flatten_space(&cbk_arg->info.decode.i_uba, data_cnt, sizeof(uint32_t));
		num_of_async_upd = ncs_decode_32bit(&ptr);
		cb->async_upd_cnt = num_of_async_upd;
		ncs_dec_skip_space(&cbk_arg->info.decode.i_uba, 4);

		/* If we reached here, we are through. Good enough for coldsync with ACTIVE */

 err:
		m_MMGR_FREE_EDSV_CKPT_DATA(data);	/*Commented this, coz of a sigabort once, revisit!!! */
		if (rc != NCSCC_RC_SUCCESS) {
			/* Clean up all data */
			eds_remove_reglist_entry(cb, 0, true);
			TRACE("cold sync decode failed cleaning database");
		}
		TRACE_LEAVE2("cold sync decode end");
		return rc;
	}			/*End while(1) */

}	/*End eds_ckpt_decode_cold_sync() */

/****************************************************************************
 * Name          : eds_process_ckpt_data
 *
 * Description   : This function updates the eds internal databases
 *                 based on the data type. 
 *
 * Arguments     : cb - pointer to EDS ControlBlock. 
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t eds_process_ckpt_data(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if ((!cb) || (data == NULL)) {
		TRACE_LEAVE();
		return (rc = NCSCC_RC_FAILURE);
	}

	if ((cb->ha_state == SA_AMF_HA_STANDBY) || (cb->ha_state == SA_AMF_HA_QUIESCED)) {
		if (data->header.ckpt_rec_type >= EDS_CKPT_MSG_MAX) {
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}

		/* Update the internal database */
		rc = eds_ckpt_data_handler[data->header.ckpt_rec_type] (cb, data);
		TRACE_LEAVE();
		return rc;
	} else {
		TRACE_LEAVE();
		return (rc = NCSCC_RC_FAILURE);
	}
}	/*End eds_process_ckpt_data() */

/****************************************************************************
 * Name          : eds_ckpt_proc_reg_rec
 *
 * Description   : This function updates the eds reglist based on the 
 *                 info received from the ACTIVE eds peer.
 *
 * Arguments     : cb - pointer to EDS  ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t eds_ckpt_proc_reg_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_REG_MSG *param = &data->ckpt_rec.reg_rec;
	TRACE_ENTER();

	if (!param->reg_id) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	/* Add this regid to the registration linked list. */
	if ((rc = eds_add_reglist_entry(cb, param->eda_client_dest, param->reg_id)) != NCSCC_RC_SUCCESS)
		LOG_IN("Checkpoint reg record add processing failed");

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}	/*End eds_ckpt_proc_reg_rec */

/****************************************************************************
 * Name          : eds_ckpt_proc_finalize_rec
 *
 * Description   : This function clears the eds reglist and assosicated DB 
 *                 based on the info received from the ACTIVE eds peer.
 *
 * Arguments     : cb - pointer to EDS ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t eds_ckpt_proc_finalize_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_FINALIZE_MSG *param = &data->ckpt_rec.finalize_rec;
	TRACE_ENTER();

	if (!param->reg_id) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	if ((rc = eds_remove_reglist_entry(cb, param->reg_id, false)) != NCSCC_RC_SUCCESS)
		LOG_IN("Checkpoint finalize record delete failed for reg_id: %u", param->reg_id);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}	/*end eds_ckpt_proc_finalize_rec */

/****************************************************************************
 * Name          : eds_ckpt_proc_chan_rec
 *
 * Description   : This function updates the worklist with the channel  
 *                 info received from the ACTIVE eds peer.
 *
 * Arguments     : cb - pointer to EDS ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t eds_ckpt_proc_chan_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_CHAN_MSG *param = &data->ckpt_rec.chan_rec;
	uint32_t reg_id = 0;	/* It is not used */
	MDS_DEST chan_opener_dest = 0;	/* Not used */
	TRACE_ENTER();

	if ((param->cname_len == 0) || (param->cname_len > SA_MAX_NAME_LENGTH)) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	if (data->header.ckpt_rec_type != EDS_CKPT_ASYNC_CHAN_OPEN_REC)
		reg_id = 0;
	else {
		reg_id = param->reg_id;
		chan_opener_dest = param->chan_opener_dest;
	}

	rc = eds_channel_open(cb,
			      reg_id,
			      param->chan_attrib,
			      param->cname_len,
			      param->cname,
			      chan_opener_dest, &param->chan_id, &param->last_copen_id, param->chan_create_time);

	if (rc != NCSCC_RC_SUCCESS)
		LOG_IN("Checkpoint channel record add failed for channel: %s", param->cname);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}	/*end eds_ckpt_proc_chan_rec */

/****************************************************************************
 * Name          : eds_ckpt_proc_chan_open_rec
 *
 * Description   : This function updates the worklist,channel open records, 
 *                 with the channel info received from the ACTIVE eds peer.
 *
 * Arguments     : cb - pointer to EDS ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t eds_ckpt_proc_chan_open_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaTimeT chan_create_time = 0;
	EDS_CKPT_CHAN_OPEN_MSG *param = &data->ckpt_rec.chan_open_rec;
	TRACE_ENTER();

	if ((param->cname_len == 0) || (param->cname_len > SA_MAX_NAME_LENGTH)) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	rc = eds_channel_open(cb, param->reg_id, param->chan_attrib, param->cname_len, param->cname, 
							param->chan_opener_dest, &param->chan_id, 
							&param->chan_open_id, chan_create_time);	
	/* Here the Chan Creation time is not needed in this flow -
	 * Channel got already created 
	 */
	if (rc != NCSCC_RC_SUCCESS)
		LOG_IN("Checkpoint channel open record add failed for channel: %s", param->cname);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}	/* End eds_ckpt_proc_chan_open_rec */

/****************************************************************************
 * Name          : eds_ckpt_proc_chan_close_rec
 *
 * Description   : This function closes a channel instance based on  
 *                 the channel info received from the ACTIVE eds peer.
 *
 * Arguments     : cb - pointer to EDS ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t eds_ckpt_proc_chan_close_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_CHAN_CLOSE_MSG *param = &data->ckpt_rec.chan_close_rec;
	TRACE_ENTER();

	if (!param) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	rc = eds_channel_close(cb, param->reg_id, param->chan_id, param->chan_open_id, false);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_IN("Checkpoint channel close processing failed for channelId: %u", param->chan_id);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}	/*End eds_ckpt_proc_chan_close_rec */

/****************************************************************************
 * Name          : eds_ckpt_proc_chan_unlink_rec
 *
 * Description   : This function unlinks a channel based on  
 *                 the channel info received from the ACTIVE eds peer.
 *
 * Arguments     : cb - pointer to EDS ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t eds_ckpt_proc_chan_unlink_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_CHAN_UNLINK_MSG *param = &(data->ckpt_rec).chan_unlink_rec;
	TRACE_ENTER();

	rc = eds_channel_unlink(cb, param->chan_name.length, param->chan_name.value);

	if (rc != NCSCC_RC_SUCCESS)
		LOG_IN("Checkpoint channel unlink processing failed for channel: %s", param->chan_name.value);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}	/*End eds_ckpt_proc_chan_unlink_rec */

/****************************************************************************
 * Name          : eds_ckpt_proc_chan_reten_rec
 *
 * Description   : This function stores a retained event on a given channel. 
 *                 based on the channel info received from the ACTIVE eds 
 *                 peer. 
 *
 * Arguments     : cb - pointer to EDS ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : The retention time is set to (original time - time elapsed)
 *                 since the time, this event was published .
 ****************************************************************************/

uint32_t eds_ckpt_proc_reten_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint32_t copen_id_Net;
	uint32_t retd_evt_chan_open_id = 0;
	SaTimeT publish_time = 0;
	EDS_WORKLIST *wp;
	CHAN_OPEN_REC *co;

	EDSV_EDA_PUBLISH_PARAM *param = (EDSV_EDA_PUBLISH_PARAM *)&data->ckpt_rec.retain_evt_rec.data;
	TRACE_ENTER();

	/* Get worklist ptr for this chan */
	wp = eds_get_worklist_entry(cb->eds_work_list, param->chan_id);
	if (!wp) {
		LOG_IN("Checkpoint: worklist entry not found for channelId: %u", param->chan_id);
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}
	publish_time = data->ckpt_rec.retain_evt_rec.pubtime;

	/* Grab the chan_open_rec for this id */
	copen_id_Net = m_NCS_OS_HTONL(param->chan_open_id);
	if (NULL == (co = (CHAN_OPEN_REC *)ncs_patricia_tree_get(&wp->chan_open_rec, (uint8_t *)&copen_id_Net))) {
		TRACE_3("Checkpoint: channel open entry not found for chanOpenId: %u", param->chan_open_id);
	}

	/* Store the retained event */
	rc = eds_store_retained_event(cb, wp, co, param, publish_time);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_IN("Checkpoint retained event failed. channel: %s, chan_id: %u", wp->cname, param->chan_id);
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	} else
      /** store the parent channel's event id 
       **/
	{
		if (co)
			retd_evt_chan_open_id = co->chan_open_id;
		else
			retd_evt_chan_open_id = param->chan_open_id;
	}

  /** If this event has been retained
   ** transfer memory ownership here.            
   **/
	if (0 != retd_evt_chan_open_id) {
		param->pattern_array = NULL;
		param->data_len = 0;
		param->data = NULL;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}	/*End eds_ckpt_proc_reten_rec */

/****************************************************************************
 * Name          : eds_ckpt_proc_chan_subscribe_rec
 *
 * Description   : This function creates a subscription record on a 
 *                 given channel based on the channel info received 
 *                 from the ACTIVE eds peer.
 *
 * Arguments     : cb - pointer to EDS ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None 
 ****************************************************************************/

uint32_t eds_ckpt_proc_subscribe_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDA_REG_REC *reglst;
	SUBSC_REC *subrec = NULL;
	EDSV_EDA_SUBSCRIBE_PARAM *param = &(data->ckpt_rec.subscribe_rec).data;
	TRACE_ENTER();

	/* Make sure this is a valid regID */
	reglst = eds_get_reglist_entry(cb, param->reg_id);
	if (reglst == NULL) {
		TRACE_LEAVE2("empty reg list");
		return (NCSCC_RC_FAILURE);
	}

	/* Allocate a new Subscription Record */
	subrec = m_MMGR_ALLOC_EDS_SUBREC(sizeof(SUBSC_REC));
	if (subrec == NULL) {
		LOG_ER("malloc for subscription record failed");
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}
	memset(subrec, 0, sizeof(SUBSC_REC));

	/* Fill in subscription related data */
	subrec->reg_list = reglst;
	subrec->subscript_id = param->sub_id;
	subrec->chan_id = param->chan_id;
	subrec->chan_open_id = param->chan_open_id;

	/* Take ownership of the filters memory.           */
	/* We'll free it when the subscription is removed. */
	subrec->filters = param->filter_array;

	/* Add the subscription to our internal lists */
	rc = eds_add_subscription(cb, param->reg_id, subrec);
	if (rc != NCSCC_RC_SUCCESS) {
		param->filter_array = NULL;
		LOG_IN("Checkpoint subscribe failed for reg_id: %u, chan_id: %u", param->reg_id, subrec->chan_id);
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}

	/* Transfer memory ownership to the 
	 * subscription record now 
	 */
	param->filter_array = NULL;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_ckpt_proc_chan_unsubscribe_rec
 *
 * Description   : This function removes a subscription record on a 
 *                 given channel based on the channel/subscription info 
 *                 received from the ACTIVE eds peer.
 *
 * Arguments     : cb - pointer to EDS ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None 
 ****************************************************************************/

uint32_t eds_ckpt_proc_unsubscribe_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDSV_EDA_UNSUBSCRIBE_PARAM *param = &(data->ckpt_rec.unsubscribe_rec).data;
	TRACE_ENTER();

	/* Remove subscription from our lists */
	rc = eds_remove_subscription(cb, param->reg_id, param->chan_id, param->chan_open_id, param->sub_id);

	if (rc != NCSCC_RC_SUCCESS)
		LOG_IN("Checkpoint unsubscribe failed: reg_id: %u, subscr_id: %u", param->reg_id, param->sub_id);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 * Name          : eds_ckpt_proc_chan_ret_time_clr_rec
 *
 * Description   : This function clears a retained event stored on a channel
 *                 based on the info received from the ACTIVE eds peer.  
 *
 * Arguments     : cb - pointer to EDS ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None 
 ****************************************************************************/

uint32_t eds_ckpt_proc_ret_time_clr_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDSV_EDA_RETENTION_TIME_CLR_PARAM *param = &(data->ckpt_rec.reten_time_clr_rec).data;
	TRACE_LEAVE();

	/* Lock the EDS_CB */
	m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	rc = eds_clear_retained_event(cb, param->chan_id, param->chan_open_id, param->event_id, false);

	/* Unlock the EDS_CB */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_IN("retained event clear failed for chan_id: %u, event_id:%u", param->chan_id, param->event_id);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_ckpt_proc_agent_down_rec
 *
 * Description   : This function processes a agent down message 
 *                 received from the ACTIVE EDS peer.      
 *
 * Arguments     : cb - pointer to EDS ControlBlock.
 *                 data - pointer to  EDS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None 
 ****************************************************************************/

uint32_t eds_ckpt_proc_agent_down_rec(EDS_CB *cb, EDS_CKPT_DATA *data)
{
	/*Remove the EDA DOWN REC from the EDA_DOWN_LIST */
	/* Search for matching EDA in the EDA_DOWN_LIST  */
	TRACE_ENTER();

	eds_remove_eda_down_rec(cb, data->ckpt_rec.agent_dest);

	/* Remove this EDA entry from our processing lists */
	eds_remove_regid_by_mds_dest(cb, data->ckpt_rec.agent_dest);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_ckpt_warm_sync_csum_dec_hdlr
 *
 * Description   : This function computes checksums on the: 
 *                  - reg records,
 *                  - channel & channel open records,
 *                  - subscription records and
 *                 compares them with the checksums received from the 
 *                 ACTIVE EDS peer.
 *
 * Arguments     : cb - pointer to EDS CB.
 *                 uba - pointer to  HJ_UBAID.
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None  
 ****************************************************************************/

uint32_t eds_ckpt_warm_sync_csum_dec_hdlr(EDS_CB *cb, NCS_UBAID *uba)
{

/*warm sync encode routine used before */
	uint32_t num_of_async_upd, rc = NCSCC_RC_SUCCESS;
	uint8_t data[16], *ptr;
	NCS_MBCSV_ARG mbcsv_arg;
	TRACE_ENTER();

	/*TBD check for the validity of eds_cb arg */

	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	ptr = ncs_dec_flatten_space(uba, data, sizeof(int32_t));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(uba, 4);

	if (cb->async_upd_cnt == num_of_async_upd) {
		TRACE_1("EDS Standby and ACTIVE are in warm sync");
		return rc;
	} else {
		/* Clean up all data */
		LOG_WA("WarmSync Check: Standby Event server out of synch with ACTIVE: Sending data sync request");
		eds_remove_reglist_entry(cb, 0, true);

		/* Compose and send a data request to ACTIVE */
		mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
		mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
		mbcsv_arg.info.send_data_req.i_ckpt_hdl = (NCS_MBCSV_CKPT_HDL)cb->mbcsv_ckpt_hdl;
		rc = ncs_mbcsv_svc(&mbcsv_arg);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_CR("MBCSv sync request failed");
			TRACE_LEAVE();
			return rc;
		}
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : eds_ckpt_warm_sync_csum_hdlr
 *
 * Description   : This function computes checksum on the
 *                 REG_REC,CHAN_OPNE_REC,SUBSC_REC,RETEN_REC databases
 *               :   and sends a warm_sync_resposne to the standby.
 *
 * Arguments     : cb - pointer to the eds control block.
 *                 cbk_arg - pointer to the mbcsv callback arg structure. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t eds_ckpt_warm_sync_csum_enc_hdlr(EDS_CB *cb, NCS_MBCSV_CB_ARG *cbk_arg)
{

/*warm sync encode routine used before */

	uint32_t rc = NCSCC_RC_SUCCESS;
	uint8_t *wsync_ptr = NULL;
	TRACE_ENTER();

	/* Reserve space to send the async update counter */
	wsync_ptr = ncs_enc_reserve_space(&cbk_arg->info.encode.io_uba, sizeof(uint32_t));
	if (wsync_ptr == NULL) {
		/* Log this Error */
		LOG_WA("warm sync reserve space failed");
		return NCSCC_RC_FAILURE;
	}

	/* SEND THE ASYNC UPDATE COUNTER */
	ncs_encode_32bit(&wsync_ptr, cb->async_upd_cnt);
	ncs_enc_claim_space(&cbk_arg->info.encode.io_uba, sizeof(uint32_t));
	/* Done. Return status */
	cbk_arg->info.encode.io_msg_type = NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE;

	TRACE_LEAVE();
	return rc;

}

/****************************************************************************
 * Name          : eds_ckpt_send_async_update
 *
 * Description   : This function makes a request to MBCSV to send an async
 *                 update to the STANDBY EDS for the record held at
 *                 the address i_reo_hdl.
 *
 * Arguments     : cb - A pointer to the eds control block.
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

uint32_t send_async_update(EDS_CB *cb, EDS_CKPT_DATA *ckpt_rec, uint32_t action)
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
	mbcsv_arg.info.send_ckpt.i_reo_hdl = (MBCSV_REO_HDL)(long)(ckpt_rec);	/*Will be used in encode callback */
	/* Just store the address of the data to be send as an 
	 * async update record in reo_hdl. The same shall then be 
	 *dereferenced during encode callback */

	mbcsv_arg.info.send_ckpt.i_reo_type = ckpt_rec->header.ckpt_rec_type;
	mbcsv_arg.info.send_ckpt.i_send_type = NCS_MBCSV_SND_SYNC;

	/* Send async update */
	if (NCSCC_RC_SUCCESS != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		LOG_ER("MBCSV Data Send operation failed !!");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return rc;
}	/*End send_async_update() */

/****************************************************************************
 * Name          : eds_ckpt_peer_info_cbk_handler 
 *
 * Description   : This callback is invoked by mbcsv when a peer info message
 *                 is received from EDS STANDBY. 
 *
 * Arguments     : NCS_MBCSV_ARG containing info pertaining to the STANDBY.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/
uint32_t eds_ckpt_peer_info_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{
	uint16_t peer_version;
	TRACE_ENTER();

	if (arg == NULL) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	peer_version = arg->info.peer.i_peer_version;
	if (peer_version < EDS_MBCSV_VERSION_MIN) {
		LOG_ER("peer version is less than min mbcsv version: %hu", peer_version);
		return NCSCC_RC_FAILURE;
	}
	
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_ckpt_notify_cbk_handler 
 *
 * Description   : This callback is invoked by mbcsv when a notify message
 *                 is received from EDS STANDBY. 
 *
 * Arguments     : NCS_MBCSV_ARG - contains notification info from STANDBY.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/
uint32_t eds_ckpt_notify_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{

	/* Currently nothing to be done */
	TRACE_4("Nothing to be done");
	return NCSCC_RC_SUCCESS;
}	/* End eds_ckpt_notify_cbk_handler */

/****************************************************************************
 * Name          : eds_ckpt_err_ind_cbk_handler 
 *
 * Description   : This callback is invoked by mbcsv when a notify message
 *                 is received from EDS STANDBY. 
 *
 *
 * Arguments     : NCS_MBCSV_ARG - contains notification info from STANDBY.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/
uint32_t eds_ckpt_err_ind_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{
	/* Currently nothing to be done. */
	TRACE_4("Nothing to be done");
	return NCSCC_RC_SUCCESS;
}	/* End eds_ckpt_err_ind_handler */

/****************************************************************************
 * Name          : eds_edp_ed_reg_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint registration rec.
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

uint32_t eds_edp_ed_reg_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			 NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_REG_MSG *ckpt_reg_msg_ptr = NULL, **ckpt_reg_msg_dec_ptr;

	EDU_INST_SET eds_ckpt_reg_rec_ed_rules[] = {
		{EDU_START, eds_edp_ed_reg_rec, 0, 0, 0, sizeof(EDS_CKPT_REG_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_REG_MSG *)0)->reg_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((EDS_CKPT_REG_MSG *)0)->eda_client_dest, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_reg_msg_ptr = (EDS_CKPT_REG_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_reg_msg_dec_ptr = (EDS_CKPT_REG_MSG **)ptr;
		if (*ckpt_reg_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_reg_msg_dec_ptr, '\0', sizeof(EDS_CKPT_REG_MSG));
		ckpt_reg_msg_ptr = *ckpt_reg_msg_dec_ptr;
	} else {
		ckpt_reg_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_reg_rec_ed_rules, ckpt_reg_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End eds_edp_ed_reg_rec */

/****************************************************************************
 * Name          : eds_edp_ed_chan_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint channel(WORKLIST) record.
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

uint32_t eds_edp_ed_chan_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_CHAN_MSG *ckpt_chan_msg_ptr = NULL, **ckpt_chan_msg_dec_ptr;

	EDU_INST_SET eds_ckpt_chan_rec_ed_rules[] = {
		{EDU_START, eds_edp_ed_chan_rec, 0, 0, 0, sizeof(EDS_CKPT_CHAN_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_MSG *)0)->reg_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_MSG *)0)->chan_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_MSG *)0)->last_copen_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_MSG *)0)->chan_attrib, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((EDS_CKPT_CHAN_MSG *)0)->chan_opener_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((EDS_CKPT_CHAN_MSG *)0)->chan_create_time, 0, NULL},
		{EDU_EXEC, ncs_edp_uns16, 0, 0, 0, (long)&((EDS_CKPT_CHAN_MSG *)0)->cname_len, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, 0, (long)&((EDS_CKPT_CHAN_MSG *)0)->cname, SA_MAX_NAME_LENGTH,
		 NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_chan_msg_ptr = (EDS_CKPT_CHAN_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_chan_msg_dec_ptr = (EDS_CKPT_CHAN_MSG **)ptr;
		if (*ckpt_chan_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_chan_msg_dec_ptr, '\0', sizeof(EDS_CKPT_CHAN_MSG));
		ckpt_chan_msg_ptr = *ckpt_chan_msg_dec_ptr;
	} else {
		ckpt_chan_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_chan_rec_ed_rules, ckpt_chan_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/*End eds_edp_ed_chan_rec */

/****************************************************************************
 * Name          : eds_edp_ed_chan_open_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint channel open records.
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

uint32_t eds_edp_ed_chan_open_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			       NCSCONTEXT ptr, uint32_t *ptr_data_len,
			       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_CHAN_OPEN_MSG *ckpt_copen_msg_ptr = NULL, **ckpt_copen_msg_dec_ptr;

	EDU_INST_SET eds_ckpt_copen_rec_ed_rules[] = {
		{EDU_START, eds_edp_ed_chan_open_rec, 0, 0, 0, sizeof(EDS_CKPT_CHAN_OPEN_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_OPEN_MSG *)0)->reg_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_OPEN_MSG *)0)->chan_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_OPEN_MSG *)0)->chan_open_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_OPEN_MSG *)0)->chan_attrib, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((EDS_CKPT_CHAN_OPEN_MSG *)0)->chan_opener_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns16, 0, 0, 0, (long)&((EDS_CKPT_CHAN_OPEN_MSG *)0)->cname_len, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, 0, (long)&((EDS_CKPT_CHAN_OPEN_MSG *)0)->cname,
		 SA_MAX_NAME_LENGTH, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_copen_msg_ptr = (EDS_CKPT_CHAN_OPEN_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_copen_msg_dec_ptr = (EDS_CKPT_CHAN_OPEN_MSG **)ptr;
		if (*ckpt_copen_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_copen_msg_dec_ptr, '\0', sizeof(EDS_CKPT_CHAN_OPEN_MSG));
		ckpt_copen_msg_ptr = *ckpt_copen_msg_dec_ptr;
	} else {
		ckpt_copen_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_copen_rec_ed_rules, ckpt_copen_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End eds_edp_ed_chan_open_rec() */

/****************************************************************************
 * Name          : eds_edp_ed_chan_close_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint channel close async update records.
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

uint32_t eds_edp_ed_chan_close_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uint32_t *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_CHAN_CLOSE_MSG *ckpt_cclose_msg_ptr = NULL, **ckpt_cclose_msg_dec_ptr;

	EDU_INST_SET eds_ckpt_cclose_rec_ed_rules[] = {
		{EDU_START, eds_edp_ed_chan_close_rec, 0, 0, 0, sizeof(EDS_CKPT_CHAN_CLOSE_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_CLOSE_MSG *)0)->reg_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_CLOSE_MSG *)0)->chan_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_CLOSE_MSG *)0)->chan_open_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_cclose_msg_ptr = (EDS_CKPT_CHAN_CLOSE_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_cclose_msg_dec_ptr = (EDS_CKPT_CHAN_CLOSE_MSG **)ptr;
		if (*ckpt_cclose_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_cclose_msg_dec_ptr, '\0', sizeof(EDS_CKPT_CHAN_CLOSE_MSG));
		ckpt_cclose_msg_ptr = *ckpt_cclose_msg_dec_ptr;
	} else {
		ckpt_cclose_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_cclose_rec_ed_rules, ckpt_cclose_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/*End eds_edp_ed_chan_close_rec */

/****************************************************************************
 * Name          : eds_edp_ed_chan_ulink_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint channel unlink async update records.
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

uint32_t eds_edp_ed_chan_ulink_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uint32_t *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_CHAN_UNLINK_MSG *ckpt_culink_msg_ptr = NULL, **ckpt_culink_msg_dec_ptr;

	EDU_INST_SET eds_ckpt_culink_rec_ed_rules[] = {
		{EDU_START, eds_edp_ed_chan_ulink_rec, 0, 0, 0, sizeof(EDS_CKPT_CHAN_UNLINK_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_CHAN_UNLINK_MSG *)0)->reg_id, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((EDS_CKPT_CHAN_UNLINK_MSG *)0)->chan_name, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_culink_msg_ptr = (EDS_CKPT_CHAN_UNLINK_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_culink_msg_dec_ptr = (EDS_CKPT_CHAN_UNLINK_MSG **)ptr;
		if (*ckpt_culink_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_culink_msg_dec_ptr, '\0', sizeof(EDS_CKPT_CHAN_UNLINK_MSG));
		ckpt_culink_msg_ptr = *ckpt_culink_msg_dec_ptr;
	} else {
		ckpt_culink_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_culink_rec_ed_rules, ckpt_culink_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/*End eds_edp_ed_chan_ulink_rec */

/****************************************************************************
 * Name          : eds_edp_ed_ret_clr_rec 
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint retention time clear async update records.
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

uint32_t eds_edp_ed_ret_clr_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			     NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_RETENTION_TIME_CLEAR_MSG *ckpt_ret_clr_msg_ptr = NULL, **ckpt_ret_clr_msg_dec_ptr;

	EDU_INST_SET eds_ckpt_ret_clr_rec_ed_rules[] = {
		{EDU_START, eds_edp_ed_ret_clr_rec, 0, 0, 0, sizeof(EDS_CKPT_RETENTION_TIME_CLEAR_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_RETENTION_TIME_CLEAR_MSG *)0)->data.chan_id, 0,
		 NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_RETENTION_TIME_CLEAR_MSG *)0)->data.chan_open_id,
		 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_RETENTION_TIME_CLEAR_MSG *)0)->data.event_id, 0,
		 NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_ret_clr_msg_ptr = (EDS_CKPT_RETENTION_TIME_CLEAR_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_ret_clr_msg_dec_ptr = (EDS_CKPT_RETENTION_TIME_CLEAR_MSG **)ptr;
		if (*ckpt_ret_clr_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_ret_clr_msg_dec_ptr, '\0', sizeof(EDS_CKPT_RETENTION_TIME_CLEAR_MSG));
		ckpt_ret_clr_msg_ptr = *ckpt_ret_clr_msg_dec_ptr;
	} else {
		ckpt_ret_clr_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_ret_clr_rec_ed_rules, ckpt_ret_clr_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/*End eds_edp_ed_ret_clr_rec */

/****************************************************************************
 * Name          : eds_edp_ed_csum_rec 
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint warm sync checksums record.
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

uint32_t eds_edp_ed_csum_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_DATA_CHECKSUM *ckpt_csum_msg_ptr = NULL, **ckpt_csum_msg_dec_ptr;

	EDU_INST_SET eds_ckpt_csum_rec_ed_rules[] = {
		{EDU_START, eds_edp_ed_csum_rec, 0, 0, 0, sizeof(EDS_CKPT_DATA_CHECKSUM), 0, NULL},
		{EDU_EXEC, ncs_edp_uns16, 0, 0, 0, (long)&((EDS_CKPT_DATA_CHECKSUM *)0)->reg_csum, 0, NULL},
		{EDU_EXEC, ncs_edp_uns16, 0, 0, 0, (long)&((EDS_CKPT_DATA_CHECKSUM *)0)->copen_csum, 0, NULL},
		{EDU_EXEC, ncs_edp_uns16, 0, 0, 0, (long)&((EDS_CKPT_DATA_CHECKSUM *)0)->subsc_csum, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_csum_msg_ptr = (EDS_CKPT_DATA_CHECKSUM *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_csum_msg_dec_ptr = (EDS_CKPT_DATA_CHECKSUM **)ptr;
		if (*ckpt_csum_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_csum_msg_dec_ptr, '\0', sizeof(EDS_CKPT_DATA_CHECKSUM));
		ckpt_csum_msg_ptr = *ckpt_csum_msg_dec_ptr;
	} else {
		ckpt_csum_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_csum_rec_ed_rules, ckpt_csum_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);

	return rc;
}	/*End eds_edp_ed_csum_rec */

/****************************************************************************
 * Name          : eds_edp_ed_csum_rec 
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint unsubscribe async update record.
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

uint32_t eds_edp_ed_usubsc_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			    NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_UNSUBSCRIBE_MSG *ckpt_usubsc_msg_ptr = NULL, **ckpt_usubsc_msg_dec_ptr;
	EDU_INST_SET eds_ckpt_usubsc_rec_ed_rules[] = {
		{EDU_START, eds_edp_ed_usubsc_rec, 0, 0, 0, sizeof(EDS_CKPT_UNSUBSCRIBE_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_UNSUBSCRIBE_MSG *)0)->data.reg_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_UNSUBSCRIBE_MSG *)0)->data.chan_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_UNSUBSCRIBE_MSG *)0)->data.chan_open_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_UNSUBSCRIBE_MSG *)0)->data.sub_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_usubsc_msg_ptr = (EDS_CKPT_UNSUBSCRIBE_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_usubsc_msg_dec_ptr = (EDS_CKPT_UNSUBSCRIBE_MSG **)ptr;
		if (*ckpt_usubsc_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_usubsc_msg_dec_ptr, '\0', sizeof(EDS_CKPT_UNSUBSCRIBE_MSG));
		ckpt_usubsc_msg_ptr = *ckpt_usubsc_msg_dec_ptr;
	} else {
		ckpt_usubsc_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_usubsc_rec_ed_rules, ckpt_usubsc_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);

	return rc;

}	/*End eds_edp_ed_usubsc_rec() */

/****************************************************************************
 * Name          : eds_edp_ed_finalize_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint finalize async updates record.
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

uint32_t eds_edp_ed_finalize_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_FINALIZE_MSG *ckpt_final_msg_ptr = NULL, **ckpt_final_msg_dec_ptr;

	EDU_INST_SET eds_ckpt_final_rec_ed_rules[] = {
		{EDU_START, eds_edp_ed_finalize_rec, 0, 0, 0, sizeof(EDS_CKPT_FINALIZE_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_FINALIZE_MSG *)0)->reg_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_final_msg_ptr = (EDS_CKPT_FINALIZE_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_final_msg_dec_ptr = (EDS_CKPT_FINALIZE_MSG **)ptr;
		if (*ckpt_final_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_final_msg_dec_ptr, '\0', sizeof(EDS_CKPT_FINALIZE_MSG));
		ckpt_final_msg_ptr = *ckpt_final_msg_dec_ptr;
	} else {
		ckpt_final_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_final_rec_ed_rules, ckpt_final_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End eds_edp_ed_finalize_rec() */

/****************************************************************************
 * Name          : eds_edp_ed_header_rec 
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint message header record.
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

uint32_t eds_edp_ed_header_rec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			    NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_HEADER *ckpt_header_ptr = NULL, **ckpt_header_dec_ptr;

	EDU_INST_SET eds_ckpt_header_rec_ed_rules[] = {
		{EDU_START, eds_edp_ed_header_rec, 0, 0, 0, sizeof(EDS_CKPT_HEADER), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_HEADER *)0)->ckpt_rec_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_HEADER *)0)->num_ckpt_records, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_HEADER *)0)->data_len, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_header_ptr = (EDS_CKPT_HEADER *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_header_dec_ptr = (EDS_CKPT_HEADER **)ptr;
		if (*ckpt_header_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_header_dec_ptr, '\0', sizeof(EDS_CKPT_HEADER));
		ckpt_header_ptr = *ckpt_header_dec_ptr;
	} else {
		ckpt_header_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_header_rec_ed_rules, ckpt_header_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End eds_edp_ed_header_rec() */

/****************************************************************************
 * Name          : eds_ckpt_msg_test_type
 *
 * Description   : This function is an EDU_TEST program which returns the 
 *                 offset to call the appropriate EDU programe based on
 *                 based on the checkpoint message type, for use by 
 *                 the EDU program eds_edu_encode_ckpt_msg.
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

int32_t eds_ckpt_msg_test_type(NCSCONTEXT arg)
{
	enum {
		LCL_TEST_JUMP_OFFSET_EDS_CKPT_REG = 1,
		LCL_TEST_JUMP_OFFSET_EDS_CKPT_FINAL,
		LCL_TEST_JUMP_OFFSET_EDS_CKPT_CHAN,
		LCL_TEST_JUMP_OFFSET_EDS_CKPT_CHAN_OPEN,
		LCL_TEST_JUMP_OFFSET_EDS_CKPT_CHAN_CLOSE,
		LCL_TEST_JUMP_OFFSET_EDS_CKPT_CHAN_UNLINK,
		LCL_TEST_JUMP_OFFSET_EDS_CKPT_RETEN_CLR,
		LCL_TEST_JUMP_OFFSET_EDS_CKPT_UNSUBSCRIBE,
		LCL_TEST_JUMP_OFFSET_EDS_CKPT_CSUM,
		LCL_TEST_JUMP_OFFSET_EDS_CKPT_AGENT_DOWN
	};

	EDS_CKPT_DATA_TYPE ckpt_rec_type;

	if (arg == NULL)
		return EDU_FAIL;

	ckpt_rec_type = *(EDS_CKPT_DATA_TYPE *)arg;
	switch (ckpt_rec_type) {
	case EDS_CKPT_INITIALIZE_REC:
		return LCL_TEST_JUMP_OFFSET_EDS_CKPT_REG;
	case EDS_CKPT_FINALIZE_REC:
		return LCL_TEST_JUMP_OFFSET_EDS_CKPT_FINAL;
	case EDS_CKPT_CHAN_REC:
	case EDS_CKPT_ASYNC_CHAN_OPEN_REC:
		return LCL_TEST_JUMP_OFFSET_EDS_CKPT_CHAN;
	case EDS_CKPT_CHAN_OPEN_REC:
		return LCL_TEST_JUMP_OFFSET_EDS_CKPT_CHAN_OPEN;
	case EDS_CKPT_CHAN_CLOSE_REC:
		return LCL_TEST_JUMP_OFFSET_EDS_CKPT_CHAN_CLOSE;
	case EDS_CKPT_CHAN_UNLINK_REC:
		return LCL_TEST_JUMP_OFFSET_EDS_CKPT_CHAN_UNLINK;
	case EDS_CKPT_RETENTION_TIME_CLR_REC:
		return LCL_TEST_JUMP_OFFSET_EDS_CKPT_RETEN_CLR;
	case EDS_CKPT_UNSUBSCRIBE_REC:
		return LCL_TEST_JUMP_OFFSET_EDS_CKPT_UNSUBSCRIBE;
	case EDS_CKPT_WARM_SYNC_CSUM:
		return LCL_TEST_JUMP_OFFSET_EDS_CKPT_CSUM;
	case EDS_CKPT_AGENT_DOWN:
		return LCL_TEST_JUMP_OFFSET_EDS_CKPT_AGENT_DOWN;
	default:
		return EDU_EXIT;
		break;
	}
	return EDU_FAIL;

}	/*End edp test type */

/****************************************************************************
 * Name          : eds_edp_ed_ckpt_msg 
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 edsv checkpoint messages. This program runs the 
 *                 eds_edp_ed_hdr_rec program first to decide the
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

uint32_t eds_edp_ed_ckpt_msg(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_DATA *ckpt_msg_ptr = NULL, **ckpt_msg_dec_ptr;

	EDU_INST_SET eds_ckpt_msg_ed_rules[] = {
		{EDU_START, eds_edp_ed_ckpt_msg, 0, 0, 0, sizeof(EDS_CKPT_DATA), 0, NULL},
		{EDU_EXEC, eds_edp_ed_header_rec, 0, 0, 0, (long)&((EDS_CKPT_DATA *)0)->header, 0, NULL},

		{EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((EDS_CKPT_DATA *)0)->header, 0,
		 (EDU_EXEC_RTINE)eds_ckpt_msg_test_type},

		/* Reg Record */
		{EDU_EXEC, eds_edp_ed_reg_rec, 0, 0, EDU_EXIT,
		 (long)&((EDS_CKPT_DATA *)0)->ckpt_rec.reg_rec, 0, NULL},

		/* Finalize record */
		{EDU_EXEC, eds_edp_ed_finalize_rec, 0, 0, EDU_EXIT,
		 (long)&((EDS_CKPT_DATA *)0)->ckpt_rec.finalize_rec, 0, NULL},

		/* Channel Create records */
		{EDU_EXEC, eds_edp_ed_chan_rec, 0, 0, EDU_EXIT,
		 (long)&((EDS_CKPT_DATA *)0)->ckpt_rec.chan_open_rec, 0, NULL},

		/* Channel open records */
		{EDU_EXEC, eds_edp_ed_chan_open_rec, 0, 0, EDU_EXIT,
		 (long)&((EDS_CKPT_DATA *)0)->ckpt_rec.chan_open_rec, 0, NULL},

		/* Channel close records */
		{EDU_EXEC, eds_edp_ed_chan_close_rec, 0, 0, EDU_EXIT,
		 (long)&((EDS_CKPT_DATA *)0)->ckpt_rec.chan_close_rec, 0, NULL},

		/* Channel unlink records */
		{EDU_EXEC, eds_edp_ed_chan_ulink_rec, 0, 0, EDU_EXIT,
		 (long)&((EDS_CKPT_DATA *)0)->ckpt_rec.chan_unlink_rec, 0, NULL},

		/* Retention time clear records */
		{EDU_EXEC, eds_edp_ed_ret_clr_rec, 0, 0, EDU_EXIT,
		 (long)&((EDS_CKPT_DATA *)0)->ckpt_rec.reten_time_clr_rec, 0, NULL},

		/* UnSubscribe Record */
		{EDU_EXEC, eds_edp_ed_usubsc_rec, 0, 0, EDU_EXIT,
		 (long)&((EDS_CKPT_DATA *)0)->ckpt_rec.unsubscribe_rec, 0, NULL},

		/* Warm Sync Checksum on data,record */
		{EDU_EXEC, eds_edp_ed_csum_rec, 0, 0, EDU_EXIT,
		 (long)&((EDS_CKPT_DATA *)0)->ckpt_rec.warm_sync_csum, 0, NULL},

		/* Warm Sync Checksum on data,record */
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, EDU_EXIT,
		 (long)&((EDS_CKPT_DATA *)0)->ckpt_rec.agent_dest, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_msg_ptr = (EDS_CKPT_DATA *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_msg_dec_ptr = (EDS_CKPT_DATA **)ptr;
		if (*ckpt_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_msg_dec_ptr, '\0', sizeof(EDS_CKPT_DATA));
		ckpt_msg_ptr = *ckpt_msg_dec_ptr;
	} else {
		ckpt_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, eds_ckpt_msg_ed_rules, ckpt_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End eds_edu_enc_dec_ckpt_msg() */

/* Non EDU routines */
/****************************************************************************
 * Name          : eds_enc_ckpt_header
 *
 * Description   : This function encodes the checkpoint message header
 *                 using leap provided apis. 
 *
 * Arguments     : pdata - pointer to the buffer to encode this struct in. 
 *                 EDS_CKPT_HEADER - edsv checkpoint message header. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

void eds_enc_ckpt_header(uint8_t *pdata, EDS_CKPT_HEADER header)
{
	ncs_encode_32bit(&pdata, header.ckpt_rec_type);
	ncs_encode_32bit(&pdata, header.num_ckpt_records);
	ncs_encode_32bit(&pdata, header.data_len);
}

/****************************************************************************
 * Name          : eds_dec_ckpt_header
 *
 * Description   : This function decodes the checkpoint message header
 *                 using leap provided apis. 
 *
 * Arguments     : NCS_UBAID - pointer to the NCS_UBAID containing data.
 *                 EDS_CKPT_HEADER - edsv checkpoint message header. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t eds_dec_ckpt_header(NCS_UBAID *uba, EDS_CKPT_HEADER *header)
{
	uint8_t *p8;
	uint8_t local_data[256];

	if ((uba == NULL) || (header == NULL)) {
		TRACE("uba or header pointer NULL");
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

	return NCSCC_RC_SUCCESS;
}	/*End eds_dec_ckpt_header */

/****************************************************************************
 * Name          : eds_ckpt_enc_reten_msg
 *
 * Description   : This function encodes the retained events on a channel. 
 *
 * Arguments     : NCS_UBAID - pointer to the NCS_UBAID containing data. 
 *                 EDS_CKPT_RETAIN_EVT_MSG - pointer to retained evt.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t eds_ckpt_enc_reten_msg(NCS_UBAID *uba, EDS_CKPT_RETAIN_EVT_MSG *msg)
{
	uint8_t *p8 = NULL;
	uint32_t total_bytes = 0;
	uint32_t x = 0;
	SaEvtEventPatternT *pattern_ptr;
	EDSV_EDA_PUBLISH_PARAM *param = &msg->data;

	if (uba == NULL) {
		TRACE("uba is null");
		return 0;
	}

   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 20);
	if (!p8) {
		TRACE("reserve space failed");
	}

	ncs_encode_32bit(&p8, param->reg_id);
	ncs_encode_32bit(&p8, param->chan_id);
	ncs_encode_32bit(&p8, param->chan_open_id);
	ncs_encode_64bit(&p8, param->pattern_array->patternsNumber);
	ncs_enc_claim_space(uba, 20);
	total_bytes += 20;

	/* Encode the patterns */
	pattern_ptr = param->pattern_array->patterns;
	for (x = 0; x < (uint32_t)param->pattern_array->patternsNumber; x++) {
		/* Save room for the patternSize field (8 bytes) */
		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8) {
			TRACE("reserve space failed");
		}

		ncs_encode_64bit(&p8, pattern_ptr->patternSize);
		ncs_enc_claim_space(uba, 8);
		total_bytes += 8;

		/* For zero length patterns, fake encode zero */
		if (pattern_ptr->patternSize == 0) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				TRACE("reserve space failed");
			}

			ncs_encode_32bit(&p8, 0);
			ncs_enc_claim_space(uba, 4);
			total_bytes += 4;
		} else {
			ncs_encode_n_octets_in_uba(uba, pattern_ptr->pattern, (uint32_t)pattern_ptr->patternSize);
			total_bytes += (uint32_t)pattern_ptr->patternSize;
		}
		pattern_ptr++;
	}

	p8 = ncs_enc_reserve_space(uba, 11);
	if (!p8) {
		TRACE("reserve space failed");
	}
	ncs_encode_8bit(&p8, param->priority);
	ncs_encode_64bit(&p8, param->retention_time);
	ncs_encode_16bit(&p8, param->publisher_name.length);
	ncs_enc_claim_space(uba, 11);
	total_bytes += 11;

	ncs_encode_n_octets_in_uba(uba, param->publisher_name.value, (uint32_t)param->publisher_name.length);
	total_bytes += (uint32_t)param->publisher_name.length;

	p8 = ncs_enc_reserve_space(uba, 12);
	if (!p8) {
		TRACE("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->event_id);
	ncs_encode_64bit(&p8, param->data_len);
	ncs_enc_claim_space(uba, 12);
	total_bytes += 12;

	ncs_encode_n_octets_in_uba(uba, param->data, (uint32_t)param->data_len);
	total_bytes += (uint32_t)param->data_len;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("reserve space failed");
	}
	ncs_encode_64bit(&p8, msg->pubtime);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	return total_bytes;
}	/*End eds_ckpt_enc_reten_msg */

/****************************************************************************
 * Name          : eds_ckpt_enc_subscribe_msg
 *
 * Description   : This function encodes a subscription msg to be 
 *                 checkpointed. 
 *
 * Arguments     : NCS_UBAID - pointer to the NCS_UBAID containing data. 
 *                 EDS_CKPT_RETAIN_EVT_MSG - pointer to retained evt.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t eds_ckpt_enc_subscribe_msg(NCS_UBAID *uba, EDS_CKPT_SUBSCRIBE_MSG *msg)
{
	uint8_t *p8 = NULL;
	uint32_t x = 0;
	uint32_t total_bytes = 0;
	SaEvtEventFilterT *filter_ptr;
	EDSV_EDA_SUBSCRIBE_PARAM *param = &msg->data;

	if (uba == NULL) {
		TRACE("uba is null");
		return 0;
	}
   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 24);
	if (!p8) {
		TRACE("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->reg_id);
	ncs_encode_32bit(&p8, param->chan_id);
	ncs_encode_32bit(&p8, param->chan_open_id);
	ncs_encode_32bit(&p8, param->sub_id);
	ncs_encode_64bit(&p8, param->filter_array->filtersNumber);
	ncs_enc_claim_space(uba, 24);
	total_bytes += 24;

	/* Encode the filters */
	filter_ptr = param->filter_array->filters;
	for (x = 0; x < (uint32_t)param->filter_array->filtersNumber; x++) {
		/* Save room for the filterType(4 bytes), patternSize(8 bytes)
		 */
		p8 = ncs_enc_reserve_space(uba, 12);
		if (!p8) {
			TRACE("reserve space failed");
		}
		ncs_encode_32bit(&p8, filter_ptr->filterType);
		ncs_encode_64bit(&p8, filter_ptr->filter.patternSize);
		ncs_enc_claim_space(uba, 12);
		total_bytes = 12;

		/* For zero length filters, fake encode zero */
		if ((uint32_t)filter_ptr->filter.patternSize == 0) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				TRACE("reserve space failed");
			}
			ncs_encode_32bit(&p8, 0);
			ncs_enc_claim_space(uba, 4);
			total_bytes += 4;
		} else {
			ncs_encode_n_octets_in_uba(uba, filter_ptr->filter.pattern,
						   (uint32_t)filter_ptr->filter.patternSize);
			total_bytes += (uint32_t)filter_ptr->filter.patternSize;
		}
		filter_ptr++;
	}

	return total_bytes;

}	/*End eds_ckpt_enc_subscribe_msg */
