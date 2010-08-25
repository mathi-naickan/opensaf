/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARANTY; without even the implied warranty of MERCHANTABILITY
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

/*****************************************************************************
  FILE NAME: immd_mbcsv.c

  DESCRIPTION: IMMD MBCSv Interface Processing Routines

******************************************************************************/

#include "immd.h"
#include "ncs_util.h"

static uns32 immd_mbcsv_decode_proc(NCS_MBCSV_CB_ARG *arg);
static int immd_silence_count = 0;

/*************************************************************************
 * Name                   : immd_mbcsv_sync_update
 *
 * Description            : Populates the MBCSv structure and sends the 
 *                          message using MBCSv
 *
 * Arguments              : IMMD_MBCSV_MSG - MBCSv message 
 *
 * Return Values          : Success / Error
 *************************************************************************/
uns32 immd_mbcsv_sync_update(IMMD_CB *cb, IMMD_MBCSV_MSG *msg)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = SA_AIS_OK;
	TRACE_ENTER();

	/* populate the arg structure */
	arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	arg.info.send_ckpt.i_ckpt_hdl = cb->o_ckpt_hdl;
	arg.info.send_ckpt.i_send_type = NCS_MBCSV_SND_SYNC;
	arg.info.send_ckpt.i_reo_type = msg->type;
	/* Address of IMMD_MBCSV_MSG, which can be used later,
	   in encode of async update */
	arg.info.send_ckpt.i_reo_hdl = NCS_PTR_TO_UNS64_CAST(msg);
	arg.info.send_ckpt.i_action = NCS_MBCSV_ACT_UPDATE;

	/*send the message using MBCSv */
	rc = ncs_mbcsv_svc(&arg);
	if (rc != SA_AIS_OK) {
		LOG_WA("IMMD - MBCSv Sync Update Send Failed");
	}
	TRACE_LEAVE();
	return rc;
}

/*************************************************************************
 * Name                   : immd_mbcsv_async_update
 *
 * Description            : Populates the MBCSv structure and sends the 
 *                          message using MBCSv
 *
 * Arguments              : IMMD_MBCSV_MSG - MBCSv message 
 *
 * Return Values          : Success / Error
 *
 * Notes                  : None
*************************************************************************/
uns32 immd_mbcsv_async_update(IMMD_CB *cb, IMMD_MBCSV_MSG *msg)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = SA_AIS_OK;
	TRACE_ENTER();

	/* populate the arg structure */
	arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	arg.info.send_ckpt.i_ckpt_hdl = cb->o_ckpt_hdl;
	arg.info.send_ckpt.i_send_type = NCS_MBCSV_SND_USR_ASYNC;
	arg.info.send_ckpt.i_reo_type = msg->type;

	/* Address of IMMD_MBCSV_MSG, which can be used later,
	   in encode of async update */
	arg.info.send_ckpt.i_reo_hdl = NCS_PTR_TO_UNS64_CAST(msg);
	arg.info.send_ckpt.i_action = NCS_MBCSV_ACT_UPDATE;

	/*send the message using MBCSv */
	rc = ncs_mbcsv_svc(&arg);
	if (rc != SA_AIS_OK) {
		LOG_WA("IMMD - MBCSv Async Update Send Failed");
	}
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
* Name         :  immd_mbcsv_register
*
* Description  : This is used by IMMD to register with the MBCSv , 
*                this will call init , open , selection object
*
* Arguments    : IMMD_CB - cb
*
* Return Values : Success / Error
*
* Notes   : This function first calls the mbcsv_init then does an open and 
*           does a selection object get
*****************************************************************************/
uns32 immd_mbcsv_register(IMMD_CB *cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	rc = immd_mbcsv_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE();
		return rc;
	}

	rc = immd_mbcsv_open(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		goto error;
	}

	/* Selection object get */
	rc = immd_mbcsv_selobj_get(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		goto fail;
	}
	TRACE_LEAVE();
	return rc;

 fail:
	immd_mbcsv_close(cb);
 error:
	immd_mbcsv_finalize(cb);
	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
 * Name                   : immd_mbcsv_init
 *
 * Description            : To initialize with MBCSv

 * Arguments              : IMMD_CB : cb pointer
 *
 * Return Values          : Success / Error
 *
 * Notes                  : None
******************************************************************************/
uns32 immd_mbcsv_init(IMMD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	TRACE_5("NCS_SERVICE_ID_IMMD:%u", NCS_SERVICE_ID_IMMD);
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	arg.info.initialize.i_mbcsv_cb = immd_mbcsv_callback;
	arg.info.initialize.i_version = IMMSV_IMMD_MBCSV_VERSION;
	arg.info.initialize.i_service = NCS_SERVICE_ID_IMMD;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER(" IMMD - MBCSv INIT FAILED ");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	cb->mbcsv_handle = arg.info.initialize.o_mbcsv_hdl;

 done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Name                   : immd_mbcsv_open
 *
 * Description            : To open a session with MBCSv
 * Arguments              : IMMD_CB - cb pointer
 *
 * Return Values          : Success / Error 
 *
 * Notes                  : Open call will set up a session between the peer
 *                          entities. o_ckpt_hdl returned by the OPEN call 
 *                          will uniquely identify the checkpoint session.
*****************************************************************************/
uns32 immd_mbcsv_open(IMMD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_OPEN;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	arg.info.open.i_pwe_hdl = (uns32)cb->mds_handle;
	arg.info.open.i_client_hdl = 0;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER(" IMMD - MBCSv Open Failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
	cb->o_ckpt_hdl = arg.info.open.o_ckpt_hdl;

	/* Disable warm sync */
	arg.i_op = NCS_MBCSV_OP_OBJ_SET;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	arg.info.obj_set.i_ckpt_hdl = cb->o_ckpt_hdl;
	arg.info.obj_set.i_obj = NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF;
	arg.info.obj_set.i_val = FALSE;
	if (ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("NCS_MBCSV_OP_OBJ_SET FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
* Name         :  immd_mbcsv_selobj_get
*
* Description  : To get a handle from OS to process the pending callbacks
*
* Arguments  : IMMD_CB 

* Return Values : Success / Error
*
* Notes  : To receive a handle from OS and can be used in further processing 
*          of pending callbacks 
******************************************************************************/
uns32 immd_mbcsv_selobj_get(IMMD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;

	if (ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("IMMD - MBCSv Selection Object Get Failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
	cb->mbcsv_sel_obj = arg.info.sel_obj_get.o_select_obj;

 done:
	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * Name                   : immd_mbcsv_chgrole
 *
 * Description            : To assign  role for a component

 * Arguments              : IMMD_CB  - cb pointer
 *
 * Return Values          : Success / Error
 *
 * Notes                  : This API is use for setting the role. 
 *                          Role Standby - initiate Cold Sync Request if it 
 *                           finds Active
 *                          Role Active - send ckpt data to multiple standby
 *                           peers
******************************************************************************/
uns32 immd_mbcsv_chgrole(IMMD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	TRACE_ENTER();

	arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	arg.info.chg_role.i_ckpt_hdl = cb->o_ckpt_hdl;
	arg.info.chg_role.i_ha_state = cb->ha_state;

	/*  ha_state is assigned at the time of amf_init where csi_set_callback 
	   will assign the state */

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_WA(" IMMD - MBCSv Change Role Failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

/***********************************************************************
 * Name          : immd_mbcsv_close
 *
 * Description   : To close the association
 *
 * Arguments     :
 *
 * Return Values : Success / Error
****************************************************************************/
uns32 immd_mbcsv_close(IMMD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	TRACE_ENTER();

	arg.i_op = NCS_MBCSV_OP_CLOSE;
	arg.info.close.i_ckpt_hdl = cb->o_ckpt_hdl;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_WA("IMMD - MBCSv Close Failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Name           :  immd_mbcsv_finalize
 *
 * Description    : To close the association of IMMD with MBCSv

 * Arguments      : IMMD_CB   - cb pointer
 *
 * Return Values  : Success / Error
 *
 * Notes          : Closes the association, represented by i_mbc_hdl, 
 *                  between the invoking process and MBCSV
*****************************************************************************/
uns32 immd_mbcsv_finalize(IMMD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	TRACE_ENTER();

	arg.i_op = NCS_MBCSV_OP_FINALIZE;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_WA("IMMD - MBCSv Finalize Failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

uns32 immd_mbcsv_dispatch(IMMD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	TRACE_ENTER();

	/* dispatch all the MBCSV pending callbacks */
	arg.i_op = NCS_MBCSV_OP_DISPATCH;
	arg.i_mbcsv_hdl = immd_cb->mbcsv_handle;
	arg.info.dispatch.i_disp_flags = SA_DISPATCH_ALL;
	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_WA("IMMD - MBCSv Dispatch Failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Name            :  immd_mbcsv_callback
 *
 * Description     : To process the MBCSv Callbacks
 *
 * Arguments       :  NCS_MBCSV_CB_ARG  - arg set delivered at the single entry
 *                                        callback
 *
 * Return Values   : Success / Error
 *
 * Notes           : Callbacks from MBCSV to user
*****************************************************************************/
uns32 immd_mbcsv_callback(NCS_MBCSV_CB_ARG *arg)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (arg == NULL) {
		rc = NCSCC_RC_FAILURE;
		LOG_WA("IMMD - MBCSv Callback NULL arg received");
		return rc;
	}

	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		rc = immd_mbcsv_encode_proc(arg);
		break;

	case NCS_MBCSV_CBOP_DEC:
		rc = immd_mbcsv_decode_proc(arg);
		break;

	case NCS_MBCSV_CBOP_PEER:
		break;

	case NCS_MBCSV_CBOP_NOTIFY:
		break;

	case NCS_MBCSV_CBOP_ERR_IND:
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}
	TRACE_5("IMMD - MBCSv Callback Success");
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name           : ncs_mbcsv_enc_async_update
 *
 * Description    : To encode the data and to send it to Standby at the time 
 *                  of Async Update
 *
 * Arguments      : NCS_MBCSV_CB_ARG - MBCSv callback Argument
 *
 * Return Values  : Success / Error
 *
 * Notes          : from io_reo_type - the event is determined and based on 
 *                   the event we encode the MBCSv_MSG
 *                  This is called at the active side
************************************************************************/
static uns32 mbcsv_enc_async_update(IMMD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	IMMD_MBCSV_MSG *immd_msg;
	uns32 rc = NCSCC_RC_SUCCESS;
	uns8 *immd_type_ptr;
	uns8 *uns32_ptr;
	uns8 *uns64_ptr;
	uns8 *uns8_ptr;
	TRACE_ENTER();

	/*  Increment the update count cb->immd_sync_cnt
	   This increments on the primary/active side (encode)
	 */

	cb->immd_sync_cnt++;

	TRACE_5("************ENC SYNC COUNT %u", cb->immd_sync_cnt);

	immd_type_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
	if (immd_type_ptr == NULL) {
		LOG_ER("IMMD - Encode Reserve Space for sync count Failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
	ncs_encode_8bit(&immd_type_ptr, arg->info.encode.io_reo_type);

	switch (arg->info.encode.io_reo_type) {

	case IMMD_A2S_MSG_FEVS:
		immd_msg = (IMMD_MBCSV_MSG *)
		    NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);

		TRACE_5("ENCODE IMMD_A2S_MSG_FEVS: send count: %llu handle: %llu",
			immd_msg->info.fevsReq.sender_count, immd_msg->info.fevsReq.client_hdl);
		uns64_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns64));
		assert(uns64_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns64));
		ncs_encode_64bit(&uns64_ptr, immd_msg->info.fevsReq.sender_count);

		uns64_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns64));
		assert(uns64_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns64));
		ncs_encode_64bit(&uns64_ptr, immd_msg->info.fevsReq.reply_dest);

		uns64_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns64));
		assert(uns64_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns64));
		ncs_encode_64bit(&uns64_ptr, immd_msg->info.fevsReq.client_hdl);

		uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
		assert(uns32_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
		ncs_encode_32bit(&uns32_ptr, immd_msg->info.fevsReq.msg.size);

		IMMSV_OCTET_STRING *os = &(immd_msg->info.fevsReq.msg);
		immsv_evt_enc_inline_string(&arg->info.encode.io_uba, os);
		break;

	case IMMD_A2S_MSG_ADMINIT:
	case IMMD_A2S_MSG_IMPLSET:
	case IMMD_A2S_MSG_CCBINIT:
		immd_msg = (IMMD_MBCSV_MSG *)
		    NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);

		/*TRACE_5("ENCODE COUNT: %u", immd_msg->info.count); */

		uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
		assert(uns32_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
		ncs_encode_32bit(&uns32_ptr, immd_msg->info.count);
		break;

	case IMMD_A2S_MSG_INTRO_RSP:
	case IMMD_A2S_MSG_SYNC_START:
	case IMMD_A2S_MSG_SYNC_ABORT:
	case IMMD_A2S_MSG_DUMP_OK:
		immd_msg = (IMMD_MBCSV_MSG *)
		    NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);

		uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
		assert(uns32_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
		ncs_encode_32bit(&uns32_ptr, immd_msg->info.ctrl.nodeId);

		uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
		assert(uns32_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
		ncs_encode_32bit(&uns32_ptr, immd_msg->info.ctrl.rulingEpoch);

		uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
		assert(uns32_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
		ncs_encode_32bit(&uns32_ptr, immd_msg->info.ctrl.ndExecPid);

		uns8_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
		assert(uns8_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
		ncs_encode_8bit(&uns8_ptr, immd_msg->info.ctrl.canBeCoord);

		uns8_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
		assert(uns8_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
		ncs_encode_8bit(&uns8_ptr, immd_msg->info.ctrl.isCoord);

		uns8_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
		assert(uns8_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
		ncs_encode_8bit(&uns8_ptr, immd_msg->info.ctrl.syncStarted);

		uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
		assert(uns32_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
		ncs_encode_32bit(&uns32_ptr, immd_msg->info.ctrl.nodeEpoch);

		uns8_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
		assert(uns8_ptr);
		ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
		ncs_encode_8bit(&uns8_ptr, immd_msg->info.ctrl.pbeEnabled);

		break;

	case IMMD_A2S_MSG_RESET:
		break;

	default:
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
 * Name            : ncs_mbcsv_enc_msg_resp
 *
 * Description     : To encode the message that is to be sent to Standby for
 *                   Cold Sync
 *
 * Arguments       : NCS_MBCSV_CB_ARG - Mbcsv callback argument
 *
 * Return Values   : Success / Error
 *
 * Notes : This is called at the active side
 |-----------------|---------------|-----------|------|-----------|-----------|
 |No. of Fevs msgs | fevs msg 1    |fevs msg 2 |..... |fevs msg n | Counters  |
 |that will be sent|               |           |      |           |           |
 |-----------------|---------------------------|------|-----------|-----------|
 *****************************************************************************/

static uns32 mbcsv_enc_msg_resp(IMMD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	TRACE_ENTER();
	uns32 rc = NCSCC_RC_SUCCESS;
	uns8 *header, num_fevs = 0, *sync_cnt_ptr;
	uns8 *uns32_ptr, *uns64_ptr, *uns8_ptr;

	/* COLD_SYNC_RESP IS DONE BY THE ACTIVE */
	if (cb->ha_state == SA_AMF_HA_STANDBY) {
		rc = NCSCC_RC_FAILURE;
		TRACE_LEAVE();
		return rc;
	}

	if ((cb->immd_sync_cnt == 0) && (cb->fevsSendCount == 0)) {	/*initial start */
		TRACE_5("COLD SYNC after initial start => empty.");
		rc = NCSCC_RC_SUCCESS;
		arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
		return rc;
	}

	/* First reserve space to store the number of X that will be sent */

	header = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
	assert(header);

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));

	/* Count fevs messages in local buffer and send count to sby.
	   Then send the fevs messages. Or ? is that really practical ?
	 */

	ncs_encode_8bit(&header, num_fevs);

	/* This will have the count of async updates that have been sent,
	   this will be 0 initially */
	sync_cnt_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
	assert(sync_cnt_ptr);
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
	ncs_encode_32bit(&sync_cnt_ptr, cb->immd_sync_cnt);

	uns64_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns64));
	assert(uns64_ptr);
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns64));
	ncs_encode_64bit(&uns64_ptr, cb->fevsSendCount);

	uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
	assert(uns32_ptr);
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
	ncs_encode_32bit(&uns32_ptr, cb->admo_id_count);

	uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
	assert(uns32_ptr);
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
	ncs_encode_32bit(&uns32_ptr, cb->impl_count);

	uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
	assert(uns32_ptr);
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
	ncs_encode_32bit(&uns32_ptr, cb->ccb_id_count);

	uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
	assert(uns32_ptr);
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
	ncs_encode_32bit(&uns32_ptr, cb->mRulingEpoch);

	if (cb->is_immnd_tree_up && cb->immnd_tree.n_nodes > 0) {
		MDS_DEST key;
		IMMD_IMMND_INFO_NODE *immnd_info_node;
		memset(&key, 0, sizeof(MDS_DEST));

		immd_immnd_info_node_getnext(&cb->immnd_tree, &key, &immnd_info_node);

		while (immnd_info_node) {
			/* Encode continue marker. */
			uns8_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
			assert(uns8_ptr);
			ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
			ncs_encode_8bit(&uns8_ptr, 0x1);

			uns64_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns64));
			assert(uns64_ptr);
			ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns64));
			ncs_encode_64bit(&uns64_ptr, immnd_info_node->immnd_dest);

			uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
			assert(uns32_ptr);
			ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
			ncs_encode_32bit(&uns32_ptr, immnd_info_node->immnd_execPid);

			uns32_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
			assert(uns32_ptr);
			ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
			ncs_encode_32bit(&uns32_ptr, immnd_info_node->epoch);

			uns8_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
			assert(uns8_ptr);
			ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
			ncs_encode_8bit(&uns8_ptr, immnd_info_node->isOnController);

			uns8_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
			assert(uns8_ptr);
			ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
			ncs_encode_8bit(&uns8_ptr, immnd_info_node->isCoord);

			uns8_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
			assert(uns8_ptr);
			ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
			ncs_encode_8bit(&uns8_ptr, immnd_info_node->syncRequested);

			uns8_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
			assert(uns8_ptr);
			ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
			ncs_encode_8bit(&uns8_ptr, immnd_info_node->syncStarted);

			TRACE_5("Cold sync encoded node:%x Pid:%u Epoch:%u syncR:%u "
				"syncS:%u onCtrlr:%u isCoord:%u",
				immnd_info_node->immnd_key,
				immnd_info_node->immnd_execPid,
				immnd_info_node->epoch,
				immnd_info_node->syncRequested,
				immnd_info_node->syncStarted,
				immnd_info_node->isOnController, immnd_info_node->isCoord);

			key = immnd_info_node->immnd_dest;

			immd_immnd_info_node_getnext(&cb->immnd_tree, &key, &immnd_info_node);
		}
	}

	/* Encode termination marker. */
	uns8_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
	assert(uns8_ptr);
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
	ncs_encode_8bit(&uns8_ptr, 0x0);

	/* Alter this to follow same pattern as logsv */
	if (num_fevs < IMMD_MBCSV_MAX_MSG_CNT) {
		if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP) {
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
		} else if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_DATA_RESP) {
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
		}
	}
	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
 * Name            : immd_mbcsv_encode_proc
 *
 * Description     : To call different callbacks ASYNC_UPDATE , COLD_SYNC ,
 *                   WARM_SYNC 
 *
 * Arguments       : NCS_MBCSV_CB_ARG - Mbcsv callback argument
 *
 * Return Values   : Success / Error
 *
******************************************************************************/
uns32 immd_mbcsv_encode_proc(NCS_MBCSV_CB_ARG *arg)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	IMMD_CB *cb = immd_cb;
	uns16 msg_fmt_version;
	TRACE_ENTER();

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.encode.i_peer_version,
					      IMMSV_IMMD_MBCSV_VERSION, IMMSV_IMMD_MBCSV_VERSION_MIN);
	if (msg_fmt_version) {
		switch (arg->info.encode.io_msg_type) {

		case NCS_MBCSV_MSG_ASYNC_UPDATE:
			TRACE_5("MBCSV_MSG_ASYNC_UPDATE");
			rc = mbcsv_enc_async_update(cb, arg);
			break;

		case NCS_MBCSV_MSG_COLD_SYNC_REQ:
			TRACE_5("MBCSV_MSG_COLD_SYNC_REQ - do nothing ?");
			break;

		case NCS_MBCSV_MSG_COLD_SYNC_RESP:
			TRACE_5("MBCSV_MSG_COLD_SYNC_RESP");
			rc = mbcsv_enc_msg_resp(cb, arg);
			break;

		case NCS_MBCSV_MSG_WARM_SYNC_REQ:
			if (++immd_silence_count < 10) {
				TRACE_5("MBCSV_MSG_WARM_SYNC_REQ - problem with MBCSV - ignoring");
			}
			break;

		case NCS_MBCSV_MSG_WARM_SYNC_RESP:
			if (++immd_silence_count < 10) {
				LOG_WA("MBCSV_MSG_WARM_SYNC_RESP - problem with MBCSV - ignoring");
			}
			/*rc = mbcsv_enc_warm_sync_resp(cb,arg); */
			break;

		case NCS_MBCSV_MSG_DATA_REQ:
			TRACE_5("MBCSV_MSG_DATA_REQ - do nothing ?");
			break;

		case NCS_MBCSV_MSG_DATA_RESP:
			TRACE_5("MBCSV_MSG_DATA_RESP");
			rc = mbcsv_enc_msg_resp(cb, arg);
			break;

		default:
			LOG_ER("UNRECOGNIZED REQUEST TYPE");
			rc = NCSCC_RC_FAILURE;
			break;
		}
	} else {
		LOG_ER("UNHANDLED FORMAT Version %u. Incompatible peer.", msg_fmt_version);
		rc = NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
 * Name        : ncs_mbcsv_dec_async_update
 *
 * Description : To decode the async update at the Standby, so the first 
 *               field is decoded which will tell the type and based on 
 *	             event, a corresponding action will be taken.
 *
 * Arguments   : NCS_MBCSV_CB_ARG - MBCSv callback argument
 * 
 * Return Values : Success / Error
******************************************************************************/

static uns32 mbcsv_dec_async_update(IMMD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uns8 *ptr;
	uns8 data[4];
	IMMD_MBCSV_MSG *immd_msg;
	uns32 evt_type, rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	immd_msg = (IMMD_MBCSV_MSG *)calloc(1, sizeof(IMMD_MBCSV_MSG));
	if (immd_msg == NULL) {
		LOG_ER("IMMD - IMMD Mbcsv calloc Failed");
		return NCSCC_RC_FAILURE;
	}
	memset(immd_msg, 0, sizeof(IMMD_MBCSV_MSG));

	/* To store the value of Async Update received */
	cb->immd_sync_cnt++;

	/* in the decode.i_uba , the 1st parameter is the Type , so first decode 
	   only the first field and based on the type then decode the entire 
	   message */

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
	evt_type = ncs_decode_8bit(&ptr);
	immd_msg->type = evt_type;
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

	switch (evt_type) {
	case IMMD_A2S_MSG_FEVS:
		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns64));
		immd_msg->info.fevsReq.sender_count = ncs_decode_64bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns64));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns64));
		immd_msg->info.fevsReq.reply_dest = ncs_decode_64bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns64));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns64));
		immd_msg->info.fevsReq.client_hdl = ncs_decode_64bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns64));

		IMMSV_OCTET_STRING *os = &(immd_msg->info.fevsReq.msg);
		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
		os->size = ncs_decode_32bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

		TRACE_5("DECODE IMMD_A2S_MSG_FEVS count:%llu handle:%llu",
			immd_msg->info.fevsReq.sender_count, immd_msg->info.fevsReq.client_hdl);
		TRACE_5("size:%u", os->size);

		immsv_evt_dec_inline_string(&arg->info.decode.i_uba, os);

		rc = immd_process_sb_fevs(cb, &(immd_msg->info.fevsReq));

		if (rc != NCSCC_RC_SUCCESS) {
			LOG_WA("Processing of mbcsv msg at standby failed");
			goto end;
		}
		if (immd_msg->info.fevsReq.msg.buf) {
			free(immd_msg->info.fevsReq.msg.buf);
			immd_msg->info.fevsReq.msg.buf = NULL;
			immd_msg->info.fevsReq.msg.size = 0;
		}
		break;

	case IMMD_A2S_MSG_ADMINIT:
	case IMMD_A2S_MSG_IMPLSET:
	case IMMD_A2S_MSG_CCBINIT:
		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
		immd_msg->info.count = ncs_decode_32bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

		rc = immd_process_sb_count(cb, immd_msg->info.count, evt_type);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_WA("Processing of mbcsv msg at standby failed");
			goto end;
		}
		break;

	case IMMD_A2S_MSG_INTRO_RSP:
	case IMMD_A2S_MSG_SYNC_START:
	case IMMD_A2S_MSG_SYNC_ABORT:
	case IMMD_A2S_MSG_DUMP_OK:
		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
		immd_msg->info.ctrl.nodeId = ncs_decode_32bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
		immd_msg->info.ctrl.rulingEpoch = ncs_decode_32bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
		immd_msg->info.ctrl.ndExecPid = ncs_decode_32bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
		immd_msg->info.ctrl.canBeCoord = ncs_decode_8bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
		immd_msg->info.ctrl.isCoord = ncs_decode_8bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
		immd_msg->info.ctrl.syncStarted = ncs_decode_8bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
		immd_msg->info.ctrl.nodeEpoch = ncs_decode_32bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
		immd_msg->info.ctrl.pbeEnabled = ncs_decode_8bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

		rc = immd_process_node_accept(cb, &immd_msg->info.ctrl);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_WA("Processing of mbcsv msg at standby failed");
			goto end;
		}
		break;

	case IMMD_A2S_MSG_RESET:
		immd_proc_immd_reset(cb, FALSE);
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}

 end:
	free(immd_msg);
	TRACE_LEAVE();
	return rc;

}

/******************************************************************************
 * Name           : ncs_mbcsv_dec_sync_resp 
 *
 * Description    : Decode the message at Standby for cold sync and update the
 *                  database
 *
 * Arguments      : NCS_MBCSV_CB_ARG - MBCSv callback argument
 *
 * Return Values  : Success / Error
******************************************************************************/
static uns32 mbcsv_dec_sync_resp(IMMD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uns8 *ptr, num_fevs, continue_marker, data[16];
	uns32 count = 0, rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	TRACE_5("RECEIVED COLD SYNC MESSAGE");
	if (arg->info.decode.i_uba.ub == NULL) {	/* There is no data */
		TRACE_5("NO DATA IN CLD SYNC");
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}

	/* 1. Decode the 1st uns8 region ,  to get the num of fevs msgs to sync */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
	num_fevs = ncs_decode_8bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

	/* Decode the data */

	while (count < num_fevs) {
		/*The idea is here that we should be syncing some fevs messages 
		   that are still in the "period of grace", i.e. available for 
		   resends. */
		assert(0);	/* Saving of fevs messages in immd A/S not yet implemented */
	}

	/* Get the counters */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
	cb->immd_sync_cnt = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns64));
	cb->fevsSendCount = ncs_decode_64bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns64));

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
	cb->admo_id_count = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
	cb->impl_count = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
	cb->ccb_id_count = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
	cb->mRulingEpoch = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

	/* OBTAIN THE IMMND TREE. */
	/* Get first continue marker. */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
	continue_marker = ncs_decode_8bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

	while (continue_marker) {
		MDS_DEST dest;
		IMMD_IMMND_INFO_NODE *node_info = NULL;
		NCS_BOOL add_flag = TRUE;

		/* Unpack the IMMND node. */
		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns64));
		dest = ncs_decode_64bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns64));

		immd_immnd_info_node_find_add(&cb->immnd_tree, &dest, &node_info, &add_flag);
		assert(node_info);

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
		node_info->immnd_execPid = ncs_decode_32bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
		node_info->epoch = ncs_decode_32bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
		node_info->isOnController = ncs_decode_8bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
		node_info->isCoord = ncs_decode_8bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

		if (node_info->isCoord) {
			cb->immnd_coord = node_info->immnd_key;
		}

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
		node_info->syncRequested = ncs_decode_8bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
		node_info->syncStarted = ncs_decode_8bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

		TRACE_5("Cold sync decoded Pid:%u Epoch:%u syncR:%u syncS:%u "
			"onCtrlr:%u isCoord:%u", node_info->immnd_execPid,
			node_info->epoch,
			node_info->syncRequested,
			node_info->syncStarted, node_info->isOnController, node_info->isCoord);

		/* Obtain next continue marker. */
		ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
		continue_marker = ncs_decode_8bit(&ptr);
		ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));
	}

	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
 * Name          : immd_mbcsv_decode_proc
 *
 * Description   : To invoke the various callbacks - ASYNC UPDATE , COLD SYNC,
 *                 WARM SYNC at the standby
 *
 * Arguments     : NCS_MBCSV_CB_ARG - MBCSv callback argument
 *
 * Return Values : Success / Error
******************************************************************************/
static uns32 immd_mbcsv_decode_proc(NCS_MBCSV_CB_ARG *arg)
{
	IMMD_CB *cb = immd_cb;
	uns16 msg_fmt_version;
	TRACE_ENTER();

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.decode.i_peer_version,
					      IMMSV_IMMD_MBCSV_VERSION, IMMSV_IMMD_MBCSV_VERSION_MIN);

	if (msg_fmt_version) {
		switch (arg->info.decode.i_msg_type) {

		case NCS_MBCSV_MSG_ASYNC_UPDATE:
			TRACE_5("MBCSV_MSG_ASYNC_UPDATE");
			mbcsv_dec_async_update(cb, arg);
			break;

		case NCS_MBCSV_MSG_COLD_SYNC_REQ:
			TRACE_5("MBCSV_MSG_COLD_SYNC_REQ - do nothing");
			break;

		case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
			TRACE_5("MBCSV_MSG_COLD_SYNC_RESP/COMPLETE");
			mbcsv_dec_sync_resp(cb, arg);
			if (arg->info.decode.i_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE) {
				TRACE_LEAVE();
				return NCSCC_RC_SUCCESS;
			}
			break;

		case NCS_MBCSV_MSG_WARM_SYNC_REQ:
			if (++immd_silence_count < 10) {
				LOG_WA("MBCSV_MSG_WARM_SYNC_REQ - should be turned off");
			}
			break;

		case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
			if (++immd_silence_count < 10) {
				LOG_WA("MBCSV_MSG_WARM_SYNC_RESP/COMPLETE - should be off");
			}
			break;

		case NCS_MBCSV_MSG_DATA_REQ:
			TRACE_5("MBCSV_MSG_DATA_REQ - do nothing");
			break;

		case NCS_MBCSV_MSG_DATA_RESP:
		case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
			TRACE_5("MBCSV_MSG_DATA_RESP/COMPLETE");
			mbcsv_dec_sync_resp(cb, arg);
			break;
		default:
			TRACE_5("UNRECOGNIZED RESPONSE TYPE");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	} else {
		/* Drop The Message */
		LOG_ER("UNRECOGNIZED FORMAT Version %u. Incompatible peer.", 
			msg_fmt_version);
		return NCSCC_RC_FAILURE;
	}
}
