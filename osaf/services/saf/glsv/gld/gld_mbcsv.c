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

#include "gld.h"

static uns32 glsv_gld_mbcsv_init(GLSV_GLD_CB *gld_cb);
static uns32 glsv_gld_mbcsv_open(GLSV_GLD_CB *gld_cb);
static uns32 glsv_gld_mbcsv_close(GLSV_GLD_CB *cb);
static uns32 glsv_gld_mbcsv_selobj_get(GLSV_GLD_CB *gld_cb);
static uns32 glsv_gld_mbcsv_callback(NCS_MBCSV_CB_ARG *arg);
static uns32 glsv_gld_mbcsv_finalize(GLSV_GLD_CB *gld_cb);
static uns32 glsv_gld_mbcsv_dec_sync_resp(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg);
static uns32 glsv_gld_mbcsv_encode_proc(NCS_MBCSV_CB_ARG *arg);
static uns32 glsv_gld_mbcsv_decode_proc(NCS_MBCSV_CB_ARG *arg);
static uns32 glsv_gld_mbcsv_dec_async_update(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg);
static uns32 glsv_gld_mbcsv_enc_async_update(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg);
static uns32 glsv_gld_mbcsv_enc_warm_sync_rsp(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg);
static uns32 glsv_gld_mbcsv_dec_warm_sync_resp(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg);
static uns32 glsv_gld_mbcsv_enc_msg_rsp(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg);
/*static void gld_glnd_details_tree_destroy(GLSV_GLD_CB *cb); */
/*static void gld_rsc_info_tree_destroy(GLSV_GLD_CB *cb); */
/*static uns32 gld_cb_db_destroy (GLSV_GLD_CB *cb); */
/*static uns32 gld_node_details_delete(GLSV_GLD_CB *cb, GLSV_GLD_GLND_DETAILS *node_details);*/
/*static uns32 gld_rsc_info_details_delete(GLSV_GLD_CB *cb, GLSV_GLD_RSC_INFO *rsc_info);*/

/**********************************************************************************************
 * Name                   : gld_mbcsv_async_update
 *
 * Description            : This routine sends async update events to Standby
 *
 * Arguments              : gld_ab, a2s_evt
 *
 * Return Values          :
 *
 * Notes                  : None
**********************************************************************************************/
uns32 glsv_gld_mbcsv_async_update(GLSV_GLD_CB *gld_cb, GLSV_GLD_A2S_CKPT_EVT *a2s_evt)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = SA_AIS_OK;

	if ((a2s_evt == NULL) || (gld_cb == NULL)) {
		m_LOG_GLD_MBCSV(GLD_NCS_MBCSV_SVC_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	/* populate the arg structure */
	arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
	arg.i_mbcsv_hdl = gld_cb->mbcsv_handle;
	arg.info.send_ckpt.i_ckpt_hdl = gld_cb->o_ckpt_hdl;
	arg.info.send_ckpt.i_send_type = NCS_MBCSV_SND_SYNC;
	arg.info.send_ckpt.i_reo_type = a2s_evt->evt_type;
	arg.info.send_ckpt.i_reo_hdl = NCS_PTR_TO_UNS64_CAST(a2s_evt);
	arg.info.send_ckpt.i_action = NCS_MBCSV_ACT_UPDATE;

	/*send the message using MBCSv */
	rc = ncs_mbcsv_svc(&arg);
	if (rc != SA_AIS_OK) {
		m_LOG_GLD_MBCSV(GLD_NCS_MBCSV_SVC_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}
	return rc;

}

/*****************************************************************************************
* Name         : glsv_gld_mbcsv_register 
*
* Description  : This is used by GLD to register with the MBCSv , this will call init , open , selection object
*
* Arguments    : GLSV_GLD_CB - gld_cb
*
* Return Values : Success / Error
*
* Notes   : This function first calls the mbcsv_init then does an open and does a selection object get
*****************************************************************************************/
uns32 glsv_gld_mbcsv_register(GLSV_GLD_CB *gld_cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	rc = glsv_gld_mbcsv_init(gld_cb);
	if (rc != NCSCC_RC_SUCCESS)
		return rc;

	rc = glsv_gld_mbcsv_open(gld_cb);
	if (rc != NCSCC_RC_SUCCESS) {
		goto error;
	}

	/* Selection object get */
	rc = glsv_gld_mbcsv_selobj_get(gld_cb);
	if (rc != NCSCC_RC_SUCCESS) {
		goto fail;
	}
	return rc;

 fail:
	glsv_gld_mbcsv_close(gld_cb);
 error:
	glsv_gld_mbcsv_finalize(gld_cb);
	return rc;
}

/*****************************************************************************************
* Name         : glsv_gld_mbcsv_unregister
*
* Description  : This is used by GLD to unregister with the MBCSv , this will call init , open , selection object
*
* Arguments    : GLSV_GLD_CB - gld_cb
*
* Return Values : Success / Error
*
* Notes   : This function first calls the mbcsv_init then does an open and does a selection object get
*****************************************************************************************/
uns32 glsv_gld_mbcsv_unregister(GLSV_GLD_CB *gld_cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	rc = glsv_gld_mbcsv_close(gld_cb);
	if (rc != NCSCC_RC_SUCCESS)
		return rc;

	rc = glsv_gld_mbcsv_finalize(gld_cb);
	return rc;

}

/************************************************************************************************
 * Name                   : glsv_gld_mbcsv_init
 *
 * Description            : To initialize with MBCSv
 *
 * Arguments              : GLD_CB : cb pointer
 *
 * Return Values          :
 *
 * Notes                  : None
*************************************************************************************************/
uns32 glsv_gld_mbcsv_init(GLSV_GLD_CB *gld_cb)
{
	NCS_MBCSV_ARG arg;

	if (gld_cb == NULL) {
		m_LOG_GLD_MBCSV(GLD_MBCSV_INIT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	arg.info.initialize.i_mbcsv_cb = glsv_gld_mbcsv_callback;
	arg.info.initialize.i_version = GLSV_GLD_MBCSV_VERSION;
	arg.info.initialize.i_service = NCS_SERVICE_ID_GLD;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		m_LOG_GLD_MBCSV(GLD_MBCSV_INIT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	} else {
		m_LOG_GLD_MBCSV(GLD_MBCSV_INIT_SUCCESS, NCSFL_SEV_NOTICE, __FILE__, __LINE__);

		gld_cb->mbcsv_handle = arg.info.initialize.o_mbcsv_hdl;
		return NCSCC_RC_SUCCESS;
	}

}

/*********************************************************************************************************
 * Name                   : glsv_gld_mbcsv_open
 *
 * Description            : To open a session with MBCSv
 *
 * Arguments              : GLSV_GLD_CB - gld_cb pointer
 *
 * Return Values          :
 *
 * Notes                  : Open call will set up a session between the peer entities. o_ckpt_hdl returned
                            by the OPEN call will uniquely identify the checkpoint session.
*********************************************************************************************************/
static uns32 glsv_gld_mbcsv_open(GLSV_GLD_CB *gld_cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (gld_cb == NULL) {
		m_LOG_GLD_MBCSV(GLD_MBCSV_OPEN_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_OPEN;
	arg.i_mbcsv_hdl = gld_cb->mbcsv_handle;
	arg.info.open.i_pwe_hdl = (uns32)gld_cb->mds_handle;
	arg.info.open.i_client_hdl = gld_cb->my_hdl;
	/* arg.info.open.o_ckpt_hdl  =  NULL; */

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		m_LOG_GLD_MBCSV(GLD_MBCSV_OPEN_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	} else {
		m_LOG_GLD_MBCSV(GLD_MBCSV_OPEN_SUCCESS, NCSFL_SEV_NOTICE, __FILE__, __LINE__);

		gld_cb->o_ckpt_hdl = arg.info.open.o_ckpt_hdl;
		return rc;
	}
}

/********************************************************************************************************
* Name         : glsv_gld_mbcsv_selobj_get 
*
* Description  : To get a handle from OS to process the pending callbacks
*
* Arguments  :  GLSV_GLD_CB -gld_cb 

* Return Values : Success / Error
*
* Notes  : To receive a handle from OS and can be used in further processing of pending callbacks 
********************************************************************************************************/
static uns32 glsv_gld_mbcsv_selobj_get(GLSV_GLD_CB *gld_cb)
{
	NCS_MBCSV_ARG arg;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	arg.i_mbcsv_hdl = gld_cb->mbcsv_handle;
	/*arg.info.sel_obj_get.o_select_obj  =  NULL; */
	if (ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS) {
		m_LOG_GLD_MBCSV(GLD_MBCSV_GET_SEL_OBJ_FAILURE, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	} else {
		m_LOG_GLD_MBCSV(GLD_MBCSV_GET_SEL_OBJ_SUCCESS, NCSFL_SEV_NOTICE, __FILE__, __LINE__);

		gld_cb->mbcsv_sel_obj = arg.info.sel_obj_get.o_select_obj;
		return NCSCC_RC_SUCCESS;
	}
}

/***********************************************************************
 * Name          : glsv_gld_mbcsv_close
 *
 * Description   : To close the association
 *
 * Arguments     :
 *
 * Return Values : Success / Error
****************************************************************************/
static uns32 glsv_gld_mbcsv_close(GLSV_GLD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_CLOSE;
	arg.info.close.i_ckpt_hdl = cb->o_ckpt_hdl;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		m_LOG_GLD_MBCSV(GLD_MBCSV_CLOSE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
	}
	return rc;
}

/******************************************************************************************************************
 * Name                   :glsv_gld_mbcsv_chgrole 
 *
 * Description            : To assign  role for a component

 * Arguments              : GLSV_GLD_CB - cb pointer
 *
 * Return Values          : Success / Error
 *
 * Notes                  : This API is use for setting the role. Role Standby - initiate Cold Sync Request if it finds Active
                            Role Active - send ckpt data to multiple standby peers
 *********************************************************************************************************************/
uns32 glsv_gld_mbcsv_chgrole(GLSV_GLD_CB *gld_cb)
{
	NCS_MBCSV_ARG arg;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	arg.i_mbcsv_hdl = gld_cb->mbcsv_handle;
	arg.info.chg_role.i_ckpt_hdl = gld_cb->o_ckpt_hdl;
	arg.info.chg_role.i_ha_state = gld_cb->ha_state;	/*  ha_state is assigned at the time of amf_init where csi_set_callback will assign the state */

	TRACE("SETTING THE STATE AS:%d", gld_cb->ha_state);
	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		m_LOG_GLD_MBCSV(GLD_MBCSV_CHGROLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************************
 * Name           :  glsv_gld_mbcsv_finalize
 *
 * Description    : To close the association of GLD with MBCSv

 * Arguments      : GLD_CB   - cb pointer
 *
 * Return Values  : Success / Error
 *
 * Notes          : Closes the association, represented by i_mbc_hdl, between the invok
ing process and MBCSV
***************************************************************************************/
static uns32 glsv_gld_mbcsv_finalize(GLSV_GLD_CB *gld_cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_FINALIZE;
	arg.i_mbcsv_hdl = gld_cb->mbcsv_handle;
	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		m_LOG_GLD_MBCSV(GLD_MBCSV_FINALIZE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
	}
	return rc;
}

/**************************************************************************************************
 * Name            : glsv_gld_mbcsv_callback
 *
 * Description     : To process the MBCSv Callbacks
 * Arguments       : NCS_MBCSV_CB_ARG  - arg set delivered at the single entry callback
 *
 * Return Values   :  Success / Error
 *
 * Notes           :  Callbacks from MBCSV to user
***************************************************************************************************/
static uns32 glsv_gld_mbcsv_callback(NCS_MBCSV_CB_ARG *arg)
{

	uns32 rc = NCSCC_RC_SUCCESS;

	if (arg == NULL) {
		/* log the message */
		m_LOG_GLD_MBCSV(GLD_MBCSV_CALLBACK_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		rc = glsv_gld_mbcsv_encode_proc(arg);
		break;

	case NCS_MBCSV_CBOP_DEC:
		rc = glsv_gld_mbcsv_decode_proc(arg);
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
	m_LOG_GLD_MBCSV(GLD_MBCSV_CALLBACK_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__);
	return rc;
}

/**************************************************************************************
 * Name            : glsv_gld_mbcsv_encode_proc 
 *
 * Description     : To call different callbacks ASYNC_UPDATE , COLD_SYNC , WARM_SYNC 

 * Arguments       : NCS_MBCSV_CB_ARG - Mbcsv callback argument
 *
 * Return Values   : Success / Error
 *
****************************************************************************************/
static uns32 glsv_gld_mbcsv_encode_proc(NCS_MBCSV_CB_ARG *arg)
{
	uns32 rc = NCSCC_RC_SUCCESS, msg_fmt_version;
	GLSV_GLD_CB *gld_cb;

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))
	    == NULL) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return (NCSCC_RC_FAILURE);
	}

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.encode.i_peer_version,
					      GLSV_GLD_MBCSV_VERSION, GLSV_GLD_MBCSV_VERSION_MIN);

	if (!msg_fmt_version) {
		/* Drop The Message */
		m_LOG_GLD_MBCSV(GLD_MSG_FRMT_VER_INVALID, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	switch (arg->info.encode.io_msg_type) {

	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		rc = glsv_gld_mbcsv_enc_async_update(gld_cb, arg);
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		if (gld_cb->rsc_info) {
			rc = glsv_gld_mbcsv_enc_msg_rsp(gld_cb, arg);
		} else {
			arg->info.decode.i_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
			rc = NCSCC_RC_SUCCESS;
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		rc = glsv_gld_mbcsv_enc_warm_sync_rsp(gld_cb, arg);
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
		rc = glsv_gld_mbcsv_enc_msg_rsp(gld_cb, arg);
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}
	ncshm_give_hdl(gld_cb->my_hdl);
	return rc;
}

/****************************************************************************************
 * Name          :glsv_gld_mbcsv_decode_proc 
 *
 * Description   : To invoke the various callbacks - ASYNC UPDATE , COLD SYNC , WARM SYNC at the standby

 * Arguments     : NCS_MBCSV_CB_ARG - MBCSv callback argument
 *
 * Return Values : Success / Error
*****************************************************************************************/
static uns32 glsv_gld_mbcsv_decode_proc(NCS_MBCSV_CB_ARG *arg)
{
	GLSV_GLD_CB *gld_cb;
	uns32 msg_fmt_version;

	gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);
	if (gld_cb == NULL) {
		m_LOG_GLD_HEADLINE(GLD_TAKE_HANDLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.decode.i_peer_version,
					      GLSV_GLD_MBCSV_VERSION, GLSV_GLD_MBCSV_VERSION_MIN);

	if (!msg_fmt_version) {
		/* Drop The Message */
		m_LOG_GLD_MBCSV(GLD_MSG_FRMT_VER_INVALID, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	switch (arg->info.decode.i_msg_type) {

	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		glsv_gld_mbcsv_dec_async_update(gld_cb, arg);
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		gld_cb->prev_rsc_id = 0;
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
		glsv_gld_mbcsv_dec_sync_resp(gld_cb, arg);
		if (arg->info.decode.i_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE) {
			return NCSCC_RC_SUCCESS;
		}

		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
		glsv_gld_mbcsv_dec_warm_sync_resp(gld_cb, arg);
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		glsv_gld_mbcsv_dec_sync_resp(gld_cb, arg);
		break;
	default:
		return NCSCC_RC_FAILURE;
	}
	ncshm_give_hdl(gld_cb->my_hdl);
	return NCSCC_RC_SUCCESS;
}

/***********************************************************************************
 * Name        : glsv_gld_mbcsv_dec_async_update 
 *
 * Description : To decode the async update at the Standby, so the first field is decoded which will tell the type
                 and based on the event, a corresponding action will be taken

 * Arguments   : NCS_MBCSV_CB_ARG - MBCSv callback argument
 * 
 * Return Values : Success / Error
***********************************************************************************/
static uns32 glsv_gld_mbcsv_dec_async_update(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg)
{
	uns8 *ptr, data[16];
	GLSV_GLD_A2S_CKPT_EVT *a2s_evt;
	uns32 evt_type, rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	GLSV_A2S_RSC_OPEN_INFO *rsc_info = NULL;
	GLSV_A2S_RSC_DETAILS *rsc_details = NULL;
	GLSV_A2S_GLND_MDS_INFO *node_details = NULL;

	a2s_evt = m_MMGR_ALLOC_GLSV_GLD_A2S_EVT;
	if (a2s_evt == NULL) {
		m_LOG_GLD_MEMFAIL(GLD_A2S_EVT_ALLOC_FAILED, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	memset(a2s_evt, 0, sizeof(GLSV_GLD_A2S_CKPT_EVT));

	/* To store the value of Async Update received */
	gld_cb->gld_async_cnt++;

	/* in the decode.i_uba , the 1st parameter is the Type , so first decode only the first field and based on the type then decode the entire message */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
	evt_type = ncs_decode_8bit(&ptr);
	a2s_evt->evt_type = evt_type;
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

	switch (evt_type) {
	case GLSV_GLD_EVT_RSC_OPEN:
		rsc_info = &a2s_evt->info.rsc_open_info;
		/* The contents are decoded from i_uba a2s_evt */
		rc = m_NCS_EDU_EXEC(&gld_cb->edu_hdl, glsv_edp_gld_a2s_evt_rsc_open_info, &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &rsc_info, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(EDU_EXEC_ASYNC_RSC_OPEN_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		rc = gld_process_standby_evt(gld_cb, a2s_evt);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(GLD_STANDBY_RSC_OPEN_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			goto end;
		}
		break;

	case GLSV_GLD_EVT_RSC_CLOSE:
		rsc_details = &a2s_evt->info.rsc_details;
		/* The contents are decoded from i_uba to a2s_evt */
		rc = m_NCS_EDU_EXEC(&gld_cb->edu_hdl, glsv_edp_gld_a2s_evt_rsc_details,
				    &arg->info.decode.i_uba, EDP_OP_TYPE_DEC, &rsc_details, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(EDU_EXEC_ASYNC_RSC_CLOSE_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		rc = gld_process_standby_evt(gld_cb, a2s_evt);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(GLD_STANDBY_RSC_CLOSE_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		break;

	case GLSV_GLD_EVT_SET_ORPHAN:
		rsc_details = &a2s_evt->info.rsc_details;
		/* The contents are decoded from i_uba to a2s_evt */
		rc = m_NCS_EDU_EXEC(&gld_cb->edu_hdl, glsv_edp_gld_a2s_evt_rsc_details, &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &rsc_details, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(EDU_EXEC_ASYNC_SET_ORPHAN_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		rc = gld_process_standby_evt(gld_cb, a2s_evt);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(GLD_STANDBY_RSC_SET_ORPHAN_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		break;

	case GLSV_GLD_EVT_GLND_DOWN:
	case GLSV_GLD_EVT_GLND_OPERATIONAL:
		node_details = &a2s_evt->info.glnd_mds_info;
		/* The contents are decoded from i_uba to a2s_evt */
		rc = m_NCS_EDU_EXEC(&gld_cb->edu_hdl, glsv_edp_gld_a2s_evt_glnd_mds_info, &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &node_details, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(EDU_EXEC_ASYNC_GLND_DOWN_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		rc = gld_process_standby_evt(gld_cb, a2s_evt);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(GLD_STANDBY_GLND_DOWN_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		break;
	default:
		m_MMGR_FREE_GLSV_GLD_A2S_EVT(a2s_evt);
		return NCSCC_RC_FAILURE;

	}
 end:
	return rc;

}

/*********************************************************************************
 * Name          :glsv_gld_mbcsv_dec_warm_sync_resp 
 *
 * Description   : To decode the message at the warm sync at the standby

 * Arguments     : NCS_MBCSV_CB_ARG - MBCSv callback argument
 * 
 * Return Values : Success / Error
*********************************************************************************/
static uns32 glsv_gld_mbcsv_dec_warm_sync_resp(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg)
{
	uns32 num_of_async_upd, rc = NCSCC_RC_SUCCESS;
	uns8 data[16], *ptr;
	NCS_MBCSV_ARG ncs_arg;

	/*TBD check for the validity of gld_cb arg */

	memset(&ncs_arg, '\0', sizeof(NCS_MBCSV_ARG));

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(int32));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, 4);

	if (gld_cb->gld_async_cnt == num_of_async_upd) {
		return rc;
	} else {
		gld_cb_destroy(gld_cb);
		ncs_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
		ncs_arg.i_mbcsv_hdl = gld_cb->mbcsv_handle;
		ncs_arg.info.send_data_req.i_ckpt_hdl = gld_cb->o_ckpt_hdl;
		rc = ncs_mbcsv_svc(&ncs_arg);
		if (rc != NCSCC_RC_SUCCESS) {
			/* Log */
			/* TBD */
			return rc;
		}
	}
	return rc;
}

/****************************************************************************
 * Name          : gld_cb_db_destroy
 *
 * Description   : Destoroy the databases in CB
 *
 * Arguments     : .
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************
uns32 gld_cb_db_destroy (GLSV_GLD_CB *cb)
{  
   
   gld_glnd_details_tree_destroy(cb);
   gld_rsc_info_tree_destroy(cb);
   return NCSCC_RC_SUCCESS;
}*/

/************************************************************************************
 * Name           :glsv_gld_mbcsv_dec_sync_resp 
 *
 * Description    : Decode the message at Standby for cold sync and update the database

 * Arguments      : NCS_MBCSV_CB_ARG - MBCSv callback argument
 *
 * Return Values  : Success / Error
*************************************************************************************/
static uns32 glsv_gld_mbcsv_dec_sync_resp(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg)
{
	uns8 *ptr, num_of_ckpts, data[16];
	GLSV_GLD_A2S_RSC_DETAILS *rsc_info;
	uns32 count = 0, rc = NCSCC_RC_SUCCESS, num_of_async_upd;
	EDU_ERR ederror = 0;
	GLSV_A2S_NODE_LIST *node_list, *tmp1_node_list;

	if (arg->info.decode.i_uba.ub == NULL) {	/* There is no data */
		return NCSCC_RC_SUCCESS;
	}

	/* Allocate memory */
	rsc_info = m_MMGR_ALLOC_GLSV_GLD_A2S_RSC_DETAILS;
	if (rsc_info == NULL) {
		m_LOG_GLD_MEMFAIL(GLD_RSC_INFO_ALLOC_FAILED, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	memset(rsc_info, 0, sizeof(GLSV_GLD_A2S_RSC_DETAILS));

	/* 1. Decode the 1st uns8 region ,  we will get the num of ckpts */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
	num_of_ckpts = ncs_decode_8bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

	/* Decode the data */

	while (count < num_of_ckpts) {
		rc = m_NCS_EDU_EXEC(&gld_cb->edu_hdl, glsv_edp_gld_evt_a2s_rsc_details, &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &rsc_info, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			return rc;
		}
		rc = gld_sb_proc_data_rsp(gld_cb, rsc_info);
		count++;
		memset(rsc_info, 0, sizeof(GLSV_GLD_A2S_RSC_DETAILS));
	}

	/* Get the async update count */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	gld_cb->gld_async_cnt = num_of_async_upd;

	/* New code */
	node_list = rsc_info->node_list;
	while (node_list != NULL) {
		tmp1_node_list = node_list;
		node_list = node_list->next;
		m_MMGR_FREE_GLSV_NODE_LIST(tmp1_node_list);
	}
	m_MMGR_FREE_GLSV_GLD_A2S_RSC_DETAILS(rsc_info);

	return rc;
}

/*******************************************************************************************
 * Name           : glsv_gld_mbcsv_enc_async_update
 *
 * Description    : To encode the data and to send it to Standby at the time of Async Update

 * Arguments      : NCS_MBCSV_CB_ARG - MBCSv callback Argument
 *
 * Return Values  : Success / Error

 * Notes          : from io_reo_type - the event is determined and based on the event we encode the MBCSv_MSG
                    This is called at the active side
*******************************************************************************************/
static uns32 glsv_gld_mbcsv_enc_async_update(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg)
{
	GLSV_GLD_A2S_CKPT_EVT *a2s_msg;
	uns32 rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	uns8 *gld_type_ptr = NULL;

	/*  Increment the async update count gld_cb->gld_async_cnt     */
	gld_cb->gld_async_cnt++;

	gld_type_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
	if (gld_type_ptr == NULL) {
		m_LOG_GLD_MEMFAIL(GLD_ENC_RESERVE_SPACE_FAILED, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
	ncs_encode_8bit(&gld_type_ptr, arg->info.encode.io_reo_type);

	switch (arg->info.encode.io_reo_type) {
	case GLSV_GLD_EVT_RSC_OPEN:
		a2s_msg = (GLSV_GLD_A2S_CKPT_EVT *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&gld_cb->edu_hdl, glsv_edp_gld_a2s_evt_rsc_open_info,
				    &arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &a2s_msg->info.rsc_open_info, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(EDU_EXEC_ASYNC_RSC_OPEN_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;
	case GLSV_GLD_EVT_RSC_CLOSE:
		a2s_msg = (GLSV_GLD_A2S_CKPT_EVT *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&gld_cb->edu_hdl, glsv_edp_gld_a2s_evt_rsc_details,
				    &arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &a2s_msg->info.rsc_details, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(EDU_EXEC_ASYNC_RSC_CLOSE_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case GLSV_GLD_EVT_SET_ORPHAN:
		a2s_msg = (GLSV_GLD_A2S_CKPT_EVT *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&gld_cb->edu_hdl, glsv_edp_gld_a2s_evt_rsc_details,
				    &arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &a2s_msg->info.rsc_details, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(EDU_EXEC_ASYNC_SET_ORPHAN_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case GLSV_GLD_EVT_GLND_DOWN:
	case GLSV_GLD_EVT_GLND_OPERATIONAL:
		a2s_msg = (GLSV_GLD_A2S_CKPT_EVT *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&gld_cb->edu_hdl, glsv_edp_gld_a2s_evt_glnd_mds_info,
				    &arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &a2s_msg->info.glnd_mds_info, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(EDU_EXEC_ASYNC_GLND_DOWN_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;
	default:
		return NCSCC_RC_FAILURE;

	}
	return rc;

}

/*******************************************************************************************
 * Name           : glsv_gld_mbcsv_enc_msg_rsp
 *
 * Description     : To encode the message that is to be sent to Standby for Cold Sync
 * Arguments      :
 *
 * Return Values  :
 |------------------|---------------|-----------|------|-----------|-----------|
 |No. of Ckpts      | ckpt record 1 |ckpt rec 2 |..... |ckpt rec n | async upd |
 |that will be sent |               |           |      |           | cnt ( 0 ) |
 |------------------|---------------------------|------|-----------|-----------|
*******************************************************************************************/
static uns32 glsv_gld_mbcsv_enc_msg_rsp(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg)
{
	GLSV_GLD_RSC_INFO *rsc_info = NULL;
	GLSV_GLD_A2S_RSC_DETAILS rsc_details;
	GLSV_NODE_LIST *node_list = NULL;
	GLSV_A2S_NODE_LIST *a2s_node_list = NULL;
	GLSV_A2S_NODE_LIST *tmp_a2s_node_list = NULL;
	GLSV_GLD_GLND_DETAILS *node_details = NULL;
	uns32 rc = NCSCC_RC_SUCCESS, no_of_ckpts = 0;
	EDU_ERR ederror = 0;
	uns8 *header;
	uns8 *async_cnt;
	SaLckResourceIdT prev_rsc_id;

	/* COLD_SYNC_RESP IS DONE BY THE ACTIVE */
	if (gld_cb->ha_state == SA_AMF_HA_STANDBY) {
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	memset(&rsc_details, '\0', sizeof(GLSV_GLD_A2S_RSC_DETAILS));

	/* First reserve space to store the number of checkpoints that will be sent */

	header = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
	if (header == NULL) {
		m_LOG_GLD_MEMFAIL(GLD_ENC_RESERVE_SPACE_FAILED, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));

	prev_rsc_id = gld_cb->prev_rsc_id;	/* NEED to see */

	/* Get the first node  */
	if (!gld_cb->prev_rsc_id)
		rsc_info = gld_cb->rsc_info;
	else {
		rsc_info = gld_cb->rsc_info;
		while (rsc_info != NULL) {
			if (rsc_info->rsc_id == gld_cb->prev_rsc_id)
				break;
			rsc_info = rsc_info->next;
		}
		if (rsc_info)
			rsc_info = rsc_info->next;

	}
	if (rsc_info == NULL) {
		/* LOG TBD */
		return NCSCC_RC_FAILURE;;
	} else {
		while (rsc_info != NULL) {
			node_list = rsc_info->node_list;
			no_of_ckpts++;

			memcpy(&rsc_details.resource_name, &rsc_info->lck_name, sizeof(SaNameT));
			rsc_details.rsc_id = rsc_info->rsc_id;
			rsc_details.can_orphan = rsc_info->can_orphan;
			rsc_details.orphan_lck_mode = rsc_info->orphan_lck_mode;
			rsc_details.node_list = a2s_node_list = m_MMGR_ALLOC_A2S_GLSV_NODE_LIST;
			if (rsc_details.node_list == NULL) {
				m_LOG_GLD_MEMFAIL(GLD_RSC_INFO_ALLOC_FAILED, __FILE__, __LINE__);
				return NCSCC_RC_FAILURE;
			}
			memset(rsc_details.node_list, '\0', sizeof(GLSV_A2S_NODE_LIST));
			if (node_list != NULL) {
				memcpy(&rsc_details.node_list->dest_id, &rsc_info->node_list->dest_id,
				       sizeof(MDS_DEST));
				/* Get the master node for this resource */
				node_details =
				    (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
										   (uns8 *)&node_list->node_id);
				if (node_details == NULL) {
					m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_GET_FAILED, NCSFL_SEV_ERROR, __FILE__,
							   __LINE__, node_list->node_id);
				} else
					rsc_details.node_list->status = node_details->status;
				node_list = node_list->next;
				while (node_list != NULL) {
					a2s_node_list->next = m_MMGR_ALLOC_A2S_GLSV_NODE_LIST;
					a2s_node_list = a2s_node_list->next;
					if (a2s_node_list == NULL) {
						m_LOG_GLD_MEMFAIL(GLD_RSC_INFO_ALLOC_FAILED, __FILE__, __LINE__);

						if (rsc_details.node_list) {
							a2s_node_list = rsc_details.node_list;
							while (a2s_node_list != NULL) {
								tmp_a2s_node_list = a2s_node_list;
								a2s_node_list = a2s_node_list->next;
								m_MMGR_FREE_A2S_GLSV_NODE_LIST(tmp_a2s_node_list);
							}

						}

						return NCSCC_RC_FAILURE;
					}
					memset(a2s_node_list, '\0', sizeof(GLSV_A2S_NODE_LIST));
					memcpy(&a2s_node_list->dest_id, &node_list->dest_id, sizeof(MDS_DEST));
					node_details =
					    (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_get(&gld_cb->glnd_details,
											   (uns8 *)&node_list->node_id);
					if (node_details == NULL) {
						m_LOG_GLD_HEADLINE(GLD_PATRICIA_TREE_GET_FAILED, NCSFL_SEV_ERROR,
								   __FILE__, __LINE__, node_list->node_id);
						return NCSCC_RC_FAILURE;
					} else
						a2s_node_list->status = node_details->status;
					node_list = node_list->next;
				}
			}
			/* DO THE EDU EXEC */
			rc = m_NCS_EDU_EXEC(&gld_cb->edu_hdl, glsv_edp_gld_evt_a2s_rsc_details,
					    &arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &rsc_details, &ederror);
			if (rc != NCSCC_RC_SUCCESS) {
				m_LOG_GLD_MBCSV(EDU_EXEC_COLDSYNC_EVT_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
				return NCSCC_RC_FAILURE;
			}
			if (rsc_details.node_list) {
				a2s_node_list = rsc_details.node_list;
				while (a2s_node_list != NULL) {
					tmp_a2s_node_list = a2s_node_list;
					a2s_node_list = a2s_node_list->next;
					m_MMGR_FREE_A2S_GLSV_NODE_LIST(tmp_a2s_node_list);
				}
			}

			if (no_of_ckpts == MAX_NO_OF_RSC_INFO_RECORDS)
				break;

			rsc_info = rsc_info->next;

		}		/* while */
	}
	if (rsc_info != NULL)
		gld_cb->prev_rsc_id = rsc_info->rsc_id;

	ncs_encode_8bit(&header, no_of_ckpts);

	/* This will have the count of async updates that have been sent,
	   this will be 0 initially */
	async_cnt = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
	if (async_cnt == NULL) {
		m_LOG_GLD_MEMFAIL(GLD_ENC_RESERVE_SPACE_FAILED, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
	ncs_encode_32bit(&async_cnt, gld_cb->gld_async_cnt);

	if (no_of_ckpts < MAX_NO_OF_RSC_INFO_RECORDS) {
		if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP)
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
		else {
			if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_DATA_RESP)
				arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
		}
		gld_cb->prev_rsc_id = 0;

	}
	return rc;
}

/************************************************************************************************
 * Name          : glsv_gld_mbcsv_enc_warm_sync_rsp 

 * Description   : To encode the message that is to be sent to Standby at the time of warm sync

 * Arguments     : GLSV_GLD_CB, NCS_MBCSV_CB_ARG - Mbcsv callback argument
 *
 * Return Values :  Success / Error
 *
 * Notes : This is called at the active side
************************************************************************************************/
static uns32 glsv_gld_mbcsv_enc_warm_sync_rsp(GLSV_GLD_CB *gld_cb, NCS_MBCSV_CB_ARG *arg)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	uns8 *wsync_ptr;

	/* Reserve space to send the async update counter */
	wsync_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
	if (wsync_ptr == NULL) {
		m_LOG_GLD_MEMFAIL(GLD_ENC_RESERVE_SPACE_FAILED, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	/* SEND THE ASYNC UPDATE COUNTER */
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
	ncs_encode_32bit(&wsync_ptr, gld_cb->gld_async_cnt);
	arg->info.encode.io_msg_type = NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE;
	return rc;

}
