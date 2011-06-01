/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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

 /*****************************************************************************/

#include "plms_mbcsv.h"
#include "ncs_edu_pub.h"
#include "ncs_saf_edu.h"
#include "ncsencdec_pub.h"

/***********************Function Prototypes ***********************************/
SaUint32T plms_mbcsv_init();
SaUint32T plms_mbcsv_register();
SaUint32T plms_mbcsv_open();
SaUint32T plms_mbcsv_selobj_get();
SaUint32T plms_mbcsv_chgrole();
SaUint32T plms_mbcsv_close();
SaUint32T plms_mbcsv_finalize();
SaUint32T plms_mbcsv_dispatch();
SaUint32T plms_mbcsv_callback(NCS_MBCSV_CB_ARG *arg);
SaUint32T plms_mbcsv_encode_proc(NCS_MBCSV_CB_ARG *arg);
SaUint32T plms_mbcsv_decode_proc(NCS_MBCSV_CB_ARG *arg);
SaUint32T plms_mbcsv_enc_async_update(NCS_MBCSV_CB_ARG *arg);
SaUint32T plms_mbcsv_dec_async_update(NCS_MBCSV_CB_ARG *arg);
SaUint32T plms_mbcsv_decode_cold_sync_data(NCS_MBCSV_CB_ARG *cbk_arg);
SaUint32T plms_mbcsv_encode_cold_sync_data(NCS_MBCSV_CB_ARG * arg);
SaUint32T plms_mbcsv_enc_warm_sync_resp(PLMS_CB *cb, NCS_MBCSV_CB_ARG *arg);
SaUint32T plms_edu_enc_entity_grp_info_data(NCS_MBCSV_CB_ARG *arg);
SaUint32T plms_mbcsv_add_client_info(PLMS_MBCSV_MSG *msg);
SaUint32T plms_mbcsv_dec_warm_sync_resp(NCS_MBCSV_CB_ARG *arg);
SaUint32T plms_process_mbcsv_data(PLMS_MBCSV_MSG *msg,NCS_MBCSV_ACT_TYPE action);
SaUint32T plms_mbcsv_add_entity_grp_info_rec(PLMS_MBCSV_MSG *msg);
static SaUint32T plms_clean_mbcsv_database();

/*****************************************************************************
* Name         :  plms_mbcsv_register
*
* Description  : This is used by PLMS to register with the MBCSv , 
*                this will call init , open , selection object
*
* Arguments    : NONE 
*
* Return Values : Success / Error
*
* Notes   : This function first calls the mbcsv_init then does an open and 
*           does a selection object get
*****************************************************************************/
SaUint32T plms_mbcsv_register()
{
	uns32 rc = NCSCC_RC_SUCCESS;

	
	TRACE_ENTER();

	rc = plms_mbcsv_init();
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE();
		return rc;
	}

	rc = plms_mbcsv_open();
	if (rc != NCSCC_RC_SUCCESS) {
		goto error;
	}

	/* Selection object get */
	rc = plms_mbcsv_selobj_get();
	if (rc != NCSCC_RC_SUCCESS) {
		goto fail;
	}
	TRACE_LEAVE();
	return rc;

 fail:
	plms_mbcsv_close();
 error:
	plms_mbcsv_finalize();
	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
 * Name                   : plms_mbcsv_init
 *
 * Description            : To initialize with MBCSv

 * Arguments              : PLMS_CB : cb pointer
 *
 * Return Values          : Success / Error
 *
 * Notes                  : None
******************************************************************************/
SaUint32T plms_mbcsv_init()
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	PLMS_CB * cb = plms_cb;
	
	TRACE_ENTER();
	TRACE_5("NCS_SERVICE_ID_PLMS:%u", NCS_SERVICE_ID_PLMS);
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	arg.info.initialize.i_mbcsv_cb = plms_mbcsv_callback;
	arg.info.initialize.i_version = PLMS_MBCSV_VERSION;
	arg.info.initialize.i_service = NCS_SERVICE_ID_PLMS;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER(" PLMS - MBCSv INIT FAILED ");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	cb->mbcsv_hdl = arg.info.initialize.o_mbcsv_hdl;

 done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Name                   : plms_mbcsv_open
 *
 * Description            : To open a session with MBCSv
 * Arguments              : PLMS_CB - cb pointer
 *
 * Return Values          : Success / Error 
 *
 * Notes                  : Open call will set up a session between the peer
 *                          entities. o_ckpt_hdl returned by the OPEN call 
 *                          will uniquely identify the checkpoint session.
*****************************************************************************/
SaUint32T plms_mbcsv_open()
{
	NCS_MBCSV_ARG arg;
	PLMS_CB *cb = plms_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_OPEN;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	arg.info.open.i_pwe_hdl = (uns32)cb->mds_hdl;
	arg.info.open.i_client_hdl = 0;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER(" PLMS - MBCSv Open Failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
	cb->mbcsv_ckpt_hdl = arg.info.open.o_ckpt_hdl;
#if 0
	/* Disable warm sync */
	arg.i_op = NCS_MBCSV_OP_OBJ_SET;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	arg.info.obj_set.i_ckpt_hdl = cb->mbcsv_ckpt_hdl;
	arg.info.obj_set.i_obj = NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF;
	arg.info.obj_set.i_val = FALSE;
	if (ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("NCS_MBCSV_OP_OBJ_SET FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
#endif
 done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
* Name         :  plms_mbcsv_selobj_get
*olm    
* Description  : To get a handle from OS to process the pending callbacks
*
* Arguments  : PLMS_CB 

* Return Values : Success / Error
*
* Notes  : To receive a handle from OS and can be used in further processing 
*          of pending callbacks 
******************************************************************************/
SaUint32T plms_mbcsv_selobj_get()
{
	NCS_MBCSV_ARG arg;
	PLMS_CB *cb = plms_cb;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;

	if (ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMS - MBCSv Selection Object Get Failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
	cb->mbcsv_sel_obj = arg.info.sel_obj_get.o_select_obj;

 done:
	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * Name                   : plms_mbcsv_chgrole
 *
 * Description            : To assign  role for a component

 * Arguments              : PLMS_CB  - cb pointer
 *
 * Return Values          : Success / Error
 *
 * Notes                  : This API is use for setting the role. 
 *                          Role Standby - initiate Cold Sync Request if it 
 *                           finds Active
 *                          Role Active - send ckpt data to multiple standby
 *                           peers
******************************************************************************/
SaUint32T plms_mbcsv_chgrole()
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	PLMS_CB * cb = plms_cb;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	TRACE_ENTER();

	arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	arg.info.chg_role.i_ckpt_hdl = cb->mbcsv_ckpt_hdl;
	arg.info.chg_role.i_ha_state = cb->ha_state;

	/*  ha_state is assigned at the time of amf_init where csi_set_callback 
	   will assign the state */

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER(" PLMS - MBCSv Change Role Failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

/***********************************************************************
 * Name          : plms_mbcsv_close
 *
 * Description   : To close the association
 *
 * Arguments     : PLMS_CB *
 *
 * Return Values : Success / Error
****************************************************************************/
SaUint32T plms_mbcsv_close()
{
	NCS_MBCSV_ARG arg;
	PLMS_CB * cb= plms_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	TRACE_ENTER();

	arg.i_op = NCS_MBCSV_OP_CLOSE;
	arg.info.close.i_ckpt_hdl = cb->mbcsv_ckpt_hdl;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("PLMS - MBCSv Close Failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Name           :  plms_mbcsv_finalize
 *
 * Description    : To close the association of PLMS with MBCSv

 * Arguments      : PLMS_CB   - cb pointer
 *
 * Return Values  : Success / Error
 *
 * Notes          : Closes the association, represented by i_mbc_hdl, 
 *                  between the invoking process and MBCSV
*****************************************************************************/
SaUint32T plms_mbcsv_finalize()
{
	NCS_MBCSV_ARG arg;
	PLMS_CB * cb = plms_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	TRACE_ENTER();

	arg.i_op = NCS_MBCSV_OP_FINALIZE;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("PLMS - MBCSv Finalize Failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

SaUint32T plms_mbcsv_dispatch()
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	TRACE_ENTER();

	/* dispatch all the MBCSV pending callbacks */
	arg.i_op = NCS_MBCSV_OP_DISPATCH;
	arg.i_mbcsv_hdl = plms_cb->mbcsv_hdl;
	arg.info.dispatch.i_disp_flags = SA_DISPATCH_ALL;
	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("PLMS - MBCSv Dispatch Failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Name            :  plms_mbcsv_callback
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
SaUint32T plms_mbcsv_callback(NCS_MBCSV_CB_ARG *arg)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (arg == NULL) {
		rc = NCSCC_RC_FAILURE;
		LOG_ER("PLMS - MBCSv Callback Failed");
		return rc;
	}

	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		rc = plms_mbcsv_encode_proc(arg);
		break;

	case NCS_MBCSV_CBOP_DEC:
		rc = plms_mbcsv_decode_proc(arg);
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
	TRACE_5("PLMS - MBCSv Callback Success");
	TRACE_LEAVE();
	return rc;
}


/******************************************************************************
* Name            : plms_mbcsv_encode_proc
*
* Description     : To call different callbacks ASYNC_UPDATE , COLD_SYNC ,
*                   WARM_SYNC at Active
*
* Arguments       : NCS_MBCSV_CB_ARG - Mbcsv callback argument
*
* Return Values   : Success / Error
*
 ******************************************************************************/
	 
SaUint32T plms_mbcsv_encode_proc(NCS_MBCSV_CB_ARG *arg)
{
	uns32 rc = NCSCC_RC_SUCCESS;
        PLMS_CB *cb = plms_cb;
        uns16 msg_fmt_version;

        TRACE_ENTER();

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.encode.i_peer_version,
				PLMS_MBCSV_VERSION, PLMS_MBCSV_VERSION_MIN);
	if (msg_fmt_version) {
		switch (arg->info.encode.io_msg_type) {
						
			case NCS_MBCSV_MSG_ASYNC_UPDATE:
				TRACE_5("PLMS MBCSV_MSG_ASYNC_UPDATE");
				rc = plms_mbcsv_enc_async_update(arg);
				break;
				
			case NCS_MBCSV_MSG_COLD_SYNC_REQ:
				TRACE_5("PLMS MBCSV_MSG_COLD_SYNC_REQ - do nothing ?");
				break;
			case NCS_MBCSV_MSG_COLD_SYNC_RESP:
				TRACE_5("PLMS MBCSV_MSG_COLD_SYNC_RESP");
				rc = plms_mbcsv_encode_cold_sync_data(arg);
				break;
				
			case NCS_MBCSV_MSG_WARM_SYNC_REQ:
				break;
				
			case NCS_MBCSV_MSG_WARM_SYNC_RESP:
				rc = plms_mbcsv_enc_warm_sync_resp(cb,arg); 
				break;
				
			case NCS_MBCSV_MSG_DATA_REQ:
				TRACE_5("PLMS MBCSV_MSG_DATA_REQ - do nothing ?");
			        break;
				
			case NCS_MBCSV_MSG_DATA_RESP:
				TRACE_5("PLMS MBCSV_MSG_DATA_RESP");
				rc = plms_mbcsv_encode_cold_sync_data(arg);
				break;

			default:
				LOG_ER("PLMS UNRECOGNIZED REQUEST TYPE");
				rc = NCSCC_RC_FAILURE;
				break;
		}
	} else {
		LOG_ER("PLMS UNRECOGNIZED FORMAT TYPE");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
 * Name          : plms_mbcsv_decode_proc
 *
 * Description   : To invoke the various callbacks - ASYNC UPDATE , COLD SYNC,
 *                 WARM SYNC at the standby
 *
 * Arguments     : NCS_MBCSV_CB_ARG - MBCSv callback argument
 *
 * Return Values : Success / Error
******************************************************************************/
	
SaUint32T plms_mbcsv_decode_proc(NCS_MBCSV_CB_ARG *arg)
{
        uns16 msg_fmt_version;

        TRACE_ENTER();

        msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.decode.i_peer_version,
	                                  PLMS_MBCSV_VERSION,PLMS_MBCSV_VERSION_MIN);

        if (msg_fmt_version) {
		switch (arg->info.decode.i_msg_type) {

									
		case NCS_MBCSV_MSG_ASYNC_UPDATE:
			TRACE_5("PLMS MBCSV_MSG_ASYNC_UPDATE");
			plms_mbcsv_dec_async_update(arg);
			break;
								     
		case NCS_MBCSV_MSG_COLD_SYNC_REQ:
			TRACE_5("PLMS MBCSV_MSG_COLD_SYNC_REQ - do nothing");
			break;
					 
		case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
			TRACE_5("PLMS MBCSV_MSG_COLD_SYNC_RESP/COMPLETE");
			plms_mbcsv_decode_cold_sync_data(arg);
			if (arg->info.decode.i_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE) {
				TRACE_LEAVE();
				return NCSCC_RC_SUCCESS;
			}
			break;
			
		case NCS_MBCSV_MSG_WARM_SYNC_REQ:
			break;

		case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
			plms_mbcsv_dec_warm_sync_resp(arg);
			break;
													 
		case NCS_MBCSV_MSG_DATA_REQ:
			TRACE_5("PLMS MBCSV_MSG_DATA_REQ - do nothing");
			break;
			
		case NCS_MBCSV_MSG_DATA_RESP:
		case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
			TRACE_5("PLMS MBCSV_MSG_DATA_RESP/COMPLETE");
			plms_mbcsv_decode_cold_sync_data(arg);
			break;
			
		default:
			TRACE_5("PLMS UNRECOGNIZED RESPONSE TYPE");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	} else {
		/* Drop The Message */
		LOG_ER("PLMSUNRECOGNIZED FORMAT TYPE");
		return NCSCC_RC_FAILURE;
	}
									 
}
/****************************************************************************
 * Name          : plms_edp_mbcsv_msg_header
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plms message header.
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
		 
SaUint32T plms_edp_mbcsv_msg_header(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
        PLMS_MBCSV_HEADER *struct_ptr = NULL, **d_ptr;

	EDU_INST_SET plms_mbcsv_msg_header_rules[] = {
		{EDU_START, plms_edp_mbcsv_msg_header, 0, 0, 0, sizeof(PLMS_MBCSV_HEADER), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_MBCSV_HEADER *)0)->msg_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_MBCSV_HEADER *)0)->num_records, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_MBCSV_HEADER *)0)->data_len, 0, NULL},
                {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
										        };

											if(op == EDP_OP_TYPE_ENC)
	{
		struct_ptr = (PLMS_MBCSV_HEADER *)ptr;
	}
	else if(op == EDP_OP_TYPE_DEC)
	{
		d_ptr = (PLMS_MBCSV_HEADER **)ptr;
		if(*d_ptr == NULL)
		{
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(PLMS_MBCSV_HEADER));
		struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_mbcsv_msg_header_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}


/****************************************************************************
 * Name          : plms_edp_trk_step_info
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plms message header.
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

SaUint32T plms_edp_trk_step_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
        PLMS_CKPT_TRACK_STEP_INFO *struct_ptr = NULL, **d_ptr;

	EDU_INST_SET plms_trk_step_info_rules[] = {
		{EDU_START, plms_edp_trk_step_info, 0, 0, 0, sizeof(PLMS_CKPT_TRACK_STEP_INFO), 0, NULL},
                {EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((PLMS_CKPT_TRACK_STEP_INFO *)0)->dn_name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_CKPT_TRACK_STEP_INFO *)0)->opr_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_CKPT_TRACK_STEP_INFO *)0)->change_step, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_CKPT_TRACK_STEP_INFO *)0)->root_correlation_id, 0, NULL},
                {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
										        };

											if(op == EDP_OP_TYPE_ENC)
	{
		struct_ptr = (PLMS_CKPT_TRACK_STEP_INFO *)ptr;
	}
	else if(op == EDP_OP_TYPE_DEC)
	{
		d_ptr = (PLMS_CKPT_TRACK_STEP_INFO **)ptr;
		if(*d_ptr == NULL)
		{
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(PLMS_CKPT_TRACK_STEP_INFO));
		struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_trk_step_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}
/****************************************************************************
 * Name          : plms_edp_entity_list
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plms entity list.
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
SaUint32T plms_edp_entity_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
        PLMS_CKPT_ENTITY_LIST *struct_ptr = NULL, **d_ptr;

	EDU_INST_SET plms_entity_list_rules[] = {
		{EDU_START, plms_edp_entity_list, EDQ_LNKLIST, 0, 0, sizeof(PLMS_CKPT_ENTITY_LIST), 0, NULL},
		
                {EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((PLMS_CKPT_ENTITY_LIST *)0)->entity_name, 0, NULL},
		 
                {EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((PLMS_CKPT_ENTITY_LIST *)0)->root_entity_name, 0, NULL},
		
		{EDU_TEST_LL_PTR, plms_edp_entity_list, 0, 0, 0, (long)&((PLMS_CKPT_ENTITY_LIST *)0)->next, 0, NULL},
                {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
										        };

											if(op == EDP_OP_TYPE_ENC)
	{
		struct_ptr = (PLMS_CKPT_ENTITY_LIST *)ptr;
	}
	else if(op == EDP_OP_TYPE_DEC)
	{
		d_ptr = (PLMS_CKPT_ENTITY_LIST **)ptr;
		if(*d_ptr == NULL)
		{
			*d_ptr = (PLMS_CKPT_ENTITY_LIST *)malloc(sizeof(PLMS_CKPT_ENTITY_LIST));
			if (*d_ptr == NULL)
			{
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		}
		memset(*d_ptr, '\0', sizeof(PLMS_CKPT_ENTITY_LIST));
		struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_entity_list_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : plms_edp_entity_grp_info
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plms message header.
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

SaUint32T plms_edp_entity_grp_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn, NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
        PLMS_CKPT_ENTITY_GROUP_INFO *struct_ptr = NULL, **d_ptr;

	EDU_INST_SET plms_entity_grp_info_rules[] = {
		{EDU_START, plms_edp_entity_grp_info, 0, 0, 0, sizeof(PLMS_CKPT_ENTITY_GROUP_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_CKPT_ENTITY_GROUP_INFO *) 0)->entity_group_handle, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((PLMS_CKPT_ENTITY_GROUP_INFO *)0)->agent_mdest_id, 0, NULL},
		{EDU_EXEC, plms_edp_entity_list, EDQ_POINTER, 0, 0, (long)&((PLMS_CKPT_ENTITY_GROUP_INFO *)0)->entity_list, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((PLMS_CKPT_ENTITY_GROUP_INFO *)0)->track_flags, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_CKPT_ENTITY_GROUP_INFO *) 0)->track_cookie, 0, NULL},
                {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
										        };

											if(op == EDP_OP_TYPE_ENC)
	{
		struct_ptr = (PLMS_CKPT_ENTITY_GROUP_INFO *)ptr;
	}
	else if(op == EDP_OP_TYPE_DEC)
	{
		d_ptr = (PLMS_CKPT_ENTITY_GROUP_INFO **)ptr;
		if(*d_ptr == NULL)
		{
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(PLMS_CKPT_ENTITY_GROUP_INFO));
		struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_entity_grp_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}


/****************************************************************************
 * Name          : plms_edp_client_info_list
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 PLMS_CKPT_CLIENT_INFO_LIST.
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
		
SaUint32T plms_edp_client_info_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr,uns32 *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uns32 rc = NCSCC_RC_SUCCESS;
	 PLMS_CKPT_CLIENT_INFO_LIST *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_client_info_list_rules[] = {
		 {EDU_START, plms_edp_client_info_list, 0, 0, 0, sizeof(PLMS_CKPT_CLIENT_INFO_LIST), 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_CKPT_CLIENT_INFO_LIST *)0)->plm_handle, 0, NULL},
		 {EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((PLMS_CKPT_CLIENT_INFO_LIST *)0)->mdest_id, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (PLMS_CKPT_CLIENT_INFO_LIST *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (PLMS_CKPT_CLIENT_INFO_LIST **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(PLMS_CKPT_CLIENT_INFO_LIST));
                struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_client_info_list_rules,
				struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
SaUint32T plms_mbcsv_enc_async_update(NCS_MBCSV_CB_ARG *arg)
{
	PLMS_MBCSV_MSG *data;
	PLMS_CB * cb = plms_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
        TRACE_ENTER();

	data = (PLMS_MBCSV_MSG *)(long)arg->info.encode.io_reo_hdl;
        if (data == NULL) {
		LOG_ER("Received Null data Ptr in enc async");
		assert(0);
	}
	if (data->header.msg_type == PLMS_A2S_MSG_TRK_STP_INFO) {
		/* Encode header first */
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, plms_edp_mbcsv_msg_header,
&arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &data->header, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("PLMS A2S trk_stp_info header encode failed");
			return rc;
		}
		/* Encode track step info data */
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl,plms_edp_trk_step_info,&arg->info.encode.io_uba,EDP_OP_TYPE_ENC,&data->info.step_info,&ederror);
		if (!rc) {      /* Encode failed. */
			LOG_ER("PLMS A2S trk_stp_info encode failed");
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
	} else if (data->header.msg_type == PLMS_A2S_MSG_ENT_GRP_INFO) {
		/* Encode header first */
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, plms_edp_mbcsv_msg_header,
&arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &data->header, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("PLMS A2S entity grp info header encode failed");
			return rc;
		}
		/* Encode entity grp info data */
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl,plms_edp_entity_grp_info,&arg->info.encode.io_uba,EDP_OP_TYPE_ENC,&data->info.ent_grp_info,&ederror);
		if (!rc) {      /* Encode failed. */
			LOG_ER("PLMS A2S plms_edp_entity_grp_info encode failed");
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
		
	} else if (data->header.msg_type == PLMS_A2S_MSG_CLIENT_INFO || 
		     data->header.msg_type == PLMS_A2S_MSG_CLIENT_DOWN_INFO) {
		/* Encode header first */
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, plms_edp_mbcsv_msg_header,
&arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &data->header, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("PLMS A2S entity grp info header encode failed");
			return rc;
		}
		/* Encode Client info data */
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl,plms_edp_client_info_list,&arg->info.encode.io_uba,EDP_OP_TYPE_ENC,&data->info.ent_grp_info,&ederror);
		if (!rc) {      /* Encode failed. */
			LOG_ER("PLMS A2S plms_edp_client_info_list encode failed");
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
		
	}
	/* Update the Async Update Count at standby */
	plms_cb->async_update_cnt = plms_cb->async_update_cnt + 1;
	TRACE_LEAVE();
	return rc;
}       /*End () plms_mbcsv_enc_async_update*/


SaUint32T plms_mbcsv_dec_async_update(NCS_MBCSV_CB_ARG *arg)
{
	EDU_ERR ederror = 0;
	PLMS_CB * cb = plms_cb;
	PLMS_MBCSV_MSG *msg;
	PLMS_MBCSV_HEADER *hdr;
	PLMS_CKPT_ENTITY_GROUP_INFO *entity_grp_info;
	PLMS_CKPT_TRACK_STEP_INFO   *trk_step_info;
	PLMS_CKPT_CLIENT_INFO_LIST  *client_info;
	
	uns32 rc = NCSCC_RC_SUCCESS;
        TRACE_ENTER();

	msg = (PLMS_MBCSV_MSG *)malloc(sizeof(PLMS_MBCSV_MSG));
        if (msg == NULL) {
		LOG_ER("Memory Allocation Failure");
		return NCSCC_RC_FAILURE;
	}
	memset(msg,0,sizeof(PLMS_MBCSV_MSG));
	
	hdr = &msg->header;

	/* decode header */
	rc = m_NCS_EDU_EXEC(&cb->edu_hdl, plms_edp_mbcsv_msg_header,
		            &arg->info.decode.i_uba, EDP_OP_TYPE_DEC, &hdr, &ederror);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMS mbcsv dec async update header decode failed");
		return rc;
	}

	if (msg->header.msg_type == PLMS_A2S_MSG_TRK_STP_INFO) {
		/* decode track step info data */
		trk_step_info = &msg->info.step_info;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl,plms_edp_trk_step_info,&arg->info.decode.i_uba,EDP_OP_TYPE_DEC,&trk_step_info,&ederror);
		if (!rc) {      /* decode failed. */
			LOG_ER("PLMS A2S trk_stp_info decode failed");
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
	} else if (msg->header.msg_type == PLMS_A2S_MSG_ENT_GRP_INFO) {
		entity_grp_info = &msg->info.ent_grp_info;
		/* decode entity grp info data */
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl,plms_edp_entity_grp_info,&arg->info.decode.i_uba,EDP_OP_TYPE_DEC,&entity_grp_info,&ederror);
		if (!rc) {      /* Decode failed. */
			LOG_ER("PLMS A2S plms_edp_entity_grp_info decode  failed");
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
		
	}else if (msg->header.msg_type == PLMS_A2S_MSG_CLIENT_DOWN_INFO) {
		client_info = &msg->info.client_info;
		rc =  m_NCS_EDU_EXEC(&cb->edu_hdl,plms_edp_client_info_list,&arg->info.decode.i_uba,EDP_OP_TYPE_DEC,&client_info,&ederror);
		if (!rc) {      /* Decode failed. */
			LOG_ER("PLMS A2S plms_edp_client_info_list decode  failed");
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
	}else if (msg->header.msg_type == PLMS_A2S_MSG_CLIENT_INFO) {
		client_info = &msg->info.client_info;
		rc =  m_NCS_EDU_EXEC(&cb->edu_hdl,plms_edp_client_info_list,&arg->info.decode.i_uba,EDP_OP_TYPE_DEC,&client_info,&ederror);
		if (!rc) {      /* Decode failed. */
			LOG_ER("PLMS A2S plms_edp_client_info_list decode  failed");
			return NCSCC_RC_FAILURE;
		} else
			rc = NCSCC_RC_SUCCESS;
	}

	rc = plms_process_mbcsv_data(msg,arg->info.decode.i_action);

	free(msg);

	/* Update the Async Update Count at standby */
	plms_cb->async_update_cnt = plms_cb->async_update_cnt + 1;
	return rc;
}

/****************************************************************************
 * Name          : plms_enc_mbcsv_header
 *
 * Description   : This function encodes the checkpoint message header
 *                 using leap provided apis.
 *
 * Arguments     : pdata - pointer to the buffer to encode this struct in.
 *                 PLMS_MBCSV_HEADER - plmsv message header.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

void plms_enc_mbcsv_header(uns8 *pdata, PLMS_MBCSV_HEADER header)
{
	ncs_encode_32bit(&pdata, header.msg_type);
	ncs_encode_32bit(&pdata, header.num_records);
	ncs_encode_32bit(&pdata, header.data_len);
}

/****************************************************************************
 * Name          : plms_dec_mbcsv_header
 *
 * Description   : This function decodes the checkpoint message header
 *                 using leap provided apis.
 *
 * Arguments     : NCS_UBAID - pointer to the NCS_UBAID containing data.
 *                 PLMS_MBCSV_HEADER - plms mbcsv  message header.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

SaUint32T plms_dec_mbcsv_header(NCS_UBAID *uba, PLMS_MBCSV_HEADER *header)
{
	uns8 *p8;
	uns8 local_data[256];

	if ((uba == NULL) || (header == NULL)) {
		LOG_ER("Null Pointer received");
		return NCSCC_RC_FAILURE;
	}

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	header->msg_type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	header->num_records = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	header->data_len = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	return NCSCC_RC_SUCCESS;
}       /*End plms_dec_mbcsv_header */

							    
	    
SaUint32T plms_edu_enc_trk_step_info_data(NCS_MBCSV_CB_ARG *arg)
{
	PLMS_CB * cb = plms_cb;
	EDU_ERR ederror = 0;
        uns32 rc = NCSCC_RC_SUCCESS, num_rec = 0;
        uns8 *pheader = NULL;
        PLMS_MBCSV_HEADER msg_hdr;
	PLMS_CKPT_TRACK_STEP_INFO *ckpt_ptr, * ptr;
	PLMS_ENTITY_GROUP_INFO * entity_grp_ptr;
	NCS_UBAID * uba = &arg->info.encode.io_uba;


	/* First see that if there are any records to be check pointed. If there are no trk step info records and entity grp info records, then just send the complete response */
	if (!cb->step_info)
	{
		entity_grp_ptr = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_getnext(&cb->entity_group_info, (uns8 *)0);
		if (!entity_grp_ptr)
		{
			/* If there are no entity grp info recs to be checkpointed close it */ 
			if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP)
				arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
			else {
				if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_DATA_RESP)
					arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
			}
                	cb->prev_trk_step_rec = NULL;
			cb->prev_ent_grp_hdl = 0;
			return NCSCC_RC_SUCCESS;
		}
/*               	cb->prev_trk_step_rec = 0xFFFF; */
		return NCSCC_RC_SUCCESS;
        }

	ckpt_ptr = (PLMS_CKPT_TRACK_STEP_INFO *)malloc(sizeof(PLMS_CKPT_TRACK_STEP_INFO));
	if (!ckpt_ptr)
	{
		LOG_ER("Memory Allocation failed");
		return NCSCC_RC_FAILURE;
	}

	/*Reserve space for "Checkpoint Header" */
        pheader = ncs_enc_reserve_space(uba, sizeof(PLMS_MBCSV_HEADER));
        if (pheader == NULL) {
                free(ckpt_ptr);
		return (rc = EDU_ERR_MEM_FAIL);
	}
        ncs_enc_claim_space(uba, sizeof(PLMS_MBCSV_HEADER));

	
	/* Get to the last checkpointed record */
	ptr = cb->step_info;
	
	while (ptr)
	{
		if (ptr == cb->prev_trk_step_rec)
		{
			ptr = ptr->next;
			break;
		} else {
			ptr = ptr->next;
		}
	}
			

	/* Now walk through the complete list and encode */
	while (ptr)
	{
		m_PLMS_COPY_TRACK_STEP_INFO(ckpt_ptr, ptr);
		/* Encode the individual record */
		rc =  m_NCS_EDU_EXEC(&cb->edu_hdl, plms_edp_trk_step_info, uba, EDP_OP_TYPE_ENC, ckpt_ptr, &ederror);
		if (rc != NCSCC_RC_SUCCESS)
		{
			free(ckpt_ptr);
			return rc;
		}
		num_rec++;
		if (num_rec == MAX_NO_OF_TRK_STEP_INFO_RECS)
			break;
		ptr = ptr->next;
	}

	if (ptr != NULL)
		cb->prev_trk_step_rec = ptr;

	if (num_rec < MAX_NO_OF_TRK_STEP_INFO_RECS)
/*		cb->prev_trk_step_rec = 0xFFFF; */ /* This is to indicate that we are done with encoding track step info records. This will be made to zero after we are done with encoding entity group info also */

	msg_hdr.msg_type = PLMS_A2S_MSG_TRK_STP_INFO;
	msg_hdr.num_records = num_rec;

	plms_enc_mbcsv_header(pheader,msg_hdr);

	entity_grp_ptr = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_getnext(&cb->entity_group_info, (uns8 *)0);
	
	/* If there are no entity grp info recs to be checkpointed close it */ 
	if ((num_rec < MAX_NO_OF_ENTITY_GRP_INFO_RECS) && (entity_grp_ptr == NULL)) {
		if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP)
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
		else {
			if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_DATA_RESP)
				arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
		}
                cb->prev_trk_step_rec = NULL;
		cb->prev_ent_grp_hdl = 0;
        }

	free(ckpt_ptr);
	return NCSCC_RC_SUCCESS;

}

static SaUint32T plms_clean_mbcsv_database()
{
	PLMS_CB * cb = plms_cb;
	PLMS_CLIENT_INFO  *client_info = NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *tmp_ent_grp_ptr1 = NULL, *tmp_ent_grp_ptr2 = NULL;
	PLMS_CKPT_ENTITY_GROUP_INFO *ckpt_grp_info = NULL,*tmp_ckpt_grp_info = NULL ;
	PLMS_CKPT_ENTITY_LIST *list_ptr1 = NULL, *list_ptr2 = NULL;

	TRACE_ENTER();

	/* Free client_info patricia tree */
	while ((client_info = (PLMS_CLIENT_INFO *)
		ncs_patricia_tree_getnext(&cb->client_info,
		(SaUint8T *) 0)) != NULL) {
		tmp_ent_grp_ptr1 = client_info->entity_group_list;
		while (tmp_ent_grp_ptr1) 
		{
			tmp_ent_grp_ptr2 = tmp_ent_grp_ptr1->next;
			free(tmp_ent_grp_ptr1);
			tmp_ent_grp_ptr1 = tmp_ent_grp_ptr2;
		}
		client_info->entity_group_list = NULL;
		ncs_patricia_tree_del(&cb->client_info, &client_info->pat_node);
		free(client_info);
	}

	/* Client info  is freed. Now free check point entity group info */
	ckpt_grp_info = cb->ckpt_entity_grp_info;
	while (ckpt_grp_info)
	{
		/* Free the entity list before freeing the group */
		if (ckpt_grp_info->entity_list)
		{
			list_ptr1 = ckpt_grp_info->entity_list;
			while(list_ptr1)
			{
				list_ptr2 =  list_ptr1->next;
				free(list_ptr1);
				list_ptr1 = list_ptr2;
			}
		}
		tmp_ckpt_grp_info = ckpt_grp_info->next;
		free(ckpt_grp_info);
		ckpt_grp_info = tmp_ckpt_grp_info;
	}
	cb->ckpt_entity_grp_info = NULL;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}


SaUint32T plms_cpy_ent_grp_to_ckpt_ent(PLMS_ENTITY_GROUP_INFO *src_ptr,
				PLMS_CKPT_ENTITY_GROUP_INFO *dst_ptr)
{
	PLMS_CKPT_ENTITY_LIST       *tmp_ckpt_ent_list_ptr,
			     	    *prev_ckpt_list_ptr = NULL;
	PLMS_GROUP_ENTITY_ROOT_LIST *tmp_entity_ptr = NULL;
	SaUint32T                    num_entities = 0;

	/*Not necessary to check for src_ptr for null as its already checked */
	/* Memory for dst ptr is already allocated in the calling function */

	dst_ptr->entity_group_handle = src_ptr->entity_grp_hdl;
	dst_ptr->agent_mdest_id = src_ptr->agent_mdest_id;
	dst_ptr->track_flags = src_ptr->track_flags;
	dst_ptr->track_cookie = src_ptr->track_cookie;

	tmp_entity_ptr = src_ptr->plm_entity_list;
	
	while (tmp_entity_ptr)
	{
		tmp_ckpt_ent_list_ptr = (PLMS_CKPT_ENTITY_LIST *)malloc(sizeof(PLMS_CKPT_ENTITY_LIST)); 
		if (!tmp_ckpt_ent_list_ptr)
		{
			LOG_ER("Memory allocation failure"); 
			assert(0);
		}

		memset(tmp_ckpt_ent_list_ptr, 0, sizeof(PLMS_CKPT_ENTITY_LIST));

		memcpy(&tmp_ckpt_ent_list_ptr->entity_name,
		 	&tmp_entity_ptr->plm_entity->dn_name,
		 	 sizeof(SaNameT));

		if (tmp_entity_ptr->subtree_root)
		{

			memcpy(&tmp_ckpt_ent_list_ptr->root_entity_name,
				 &tmp_entity_ptr->subtree_root->dn_name,
				 sizeof(SaNameT));
		}

		if (prev_ckpt_list_ptr == NULL) 
		{ 
		 	dst_ptr->entity_list = tmp_ckpt_ent_list_ptr; 
		} else { 
		 	prev_ckpt_list_ptr->next = tmp_ckpt_ent_list_ptr; 
		} 
		prev_ckpt_list_ptr = tmp_ckpt_ent_list_ptr; 
		tmp_ckpt_ent_list_ptr->next = NULL;
		num_entities++;
		tmp_entity_ptr = tmp_entity_ptr->next;
		TRACE("Encoding Entity : %d",num_entities);
	} 
	return NCSCC_RC_SUCCESS;
}

SaUint32T plms_edu_enc_entity_grp_info_data(NCS_MBCSV_CB_ARG *arg)
{
	PLMS_CB * cb = plms_cb;
	EDU_ERR ederror = 0;
        uns32 rc = NCSCC_RC_SUCCESS, num_rec = 0;
        uns8 *pheader = NULL;
        PLMS_MBCSV_HEADER msg_hdr;
	PLMS_CKPT_ENTITY_GROUP_INFO *ckpt_ptr;
	PLMS_ENTITY_GROUP_INFO    *ptr;
	NCS_UBAID * uba = &arg->info.encode.io_uba; 
	uns8 *async_upd_cnt = NULL;

	TRACE_ENTER();

	ckpt_ptr = (PLMS_CKPT_ENTITY_GROUP_INFO *)malloc(sizeof(PLMS_CKPT_ENTITY_GROUP_INFO));
	if (!ckpt_ptr)
	{
		LOG_ER("Memory Allocation failed");
		return NCSCC_RC_FAILURE;
	}

	/*Reserve space for "Checkpoint Header" */
        pheader = ncs_enc_reserve_space(uba, sizeof(PLMS_MBCSV_HEADER));
        if (pheader == NULL) {
                free(ckpt_ptr);
		return (rc = EDU_ERR_MEM_FAIL);
	}
        ncs_enc_claim_space(uba, sizeof(PLMS_MBCSV_HEADER));

	if (cb->prev_ent_grp_hdl)
	{
		ptr = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_getnext(&cb->entity_group_info, (uns8 *)&cb->prev_ent_grp_hdl);
	} else {
		ptr = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_getnext(&cb->entity_group_info, (uns8 *)0);
	}

	/* Now walk through the complete list and encode */
	while (ptr)
	{
		memset(ckpt_ptr, 0, sizeof(PLMS_CKPT_ENTITY_GROUP_INFO));
		plms_cpy_ent_grp_to_ckpt_ent(ptr, ckpt_ptr);
		if (rc != NCSCC_RC_SUCCESS)
		{
			LOG_ER("Entity Group Info copy failed");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		/* Encode the individual record */
		rc =  m_NCS_EDU_EXEC(&cb->edu_hdl, plms_edp_entity_grp_info, uba, EDP_OP_TYPE_ENC, ckpt_ptr, &ederror);
		if (rc != NCSCC_RC_SUCCESS)
		{
			free(ckpt_ptr);
			TRACE_LEAVE();
			return rc;
		}
		num_rec++;
		if ( num_rec == MAX_NO_OF_ENTITY_GRP_INFO_RECS)
			break;
			
		ptr = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_getnext(&cb->entity_group_info, (uns8 *)&ptr->entity_grp_hdl);
	}

	if(ptr != NULL)
	{
		cb->prev_ent_grp_hdl = ptr->entity_grp_hdl;
	}
		
        /* fill header and encode the header */ 
	msg_hdr.msg_type = PLMS_A2S_MSG_ENT_GRP_INFO; 
	msg_hdr.num_records = num_rec;

	plms_enc_mbcsv_header(pheader,msg_hdr);
	free(ckpt_ptr);
	
	if (num_rec < MAX_NO_OF_ENTITY_GRP_INFO_RECS) {
		
		/* Encode the Async Update Count at standby */

		/* This will have the count of async updates that have i
		been sent, this will be 0 initially */
	        async_upd_cnt = ncs_enc_reserve_space(uba, sizeof(uns32));
		if (async_upd_cnt == NULL) 
		{
			LOG_ER("Async update count encode Failed");
			return NCSCC_RC_FAILURE;
		}
		ncs_encode_32bit(&async_upd_cnt, cb->async_update_cnt);
		ncs_enc_claim_space(uba, sizeof(uns32));

		if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP)
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
		else {
			if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_DATA_RESP)
				arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
		}
                cb->prev_trk_step_rec = NULL;
		cb->prev_ent_grp_hdl = 0;
		cb->entity_grp_cold_sync_done = FALSE;
		cb->client_info_cold_sync_done = FALSE;
        }
	TRACE_LEAVE2("Encoded %d entity grp records :",num_rec);
	return NCSCC_RC_SUCCESS;

}

SaUint32T plms_edu_enc_client_info_data(NCS_MBCSV_CB_ARG *arg)
{
	PLMS_CB * cb = plms_cb;
	EDU_ERR ederror = 0;
        uns32 rc = NCSCC_RC_SUCCESS, num_rec = 0;
        uns8 *pheader = NULL;
        PLMS_MBCSV_HEADER msg_hdr;
	PLMS_CLIENT_INFO *client_info = NULL;
	PLMS_CKPT_CLIENT_INFO_LIST *ckpt_client_info;
	NCS_UBAID * uba = &arg->info.encode.io_uba; 
	PLMS_ENTITY_GROUP_INFO * entity_grp_ptr;
	uns8 *async_upd_cnt = NULL;

	TRACE_ENTER();

	ckpt_client_info = (PLMS_CKPT_CLIENT_INFO_LIST *)malloc(sizeof(PLMS_CKPT_CLIENT_INFO_LIST));
	if (!ckpt_client_info)
	{
		LOG_ER("Memory Allocation failed");
		assert(0);
	}

	/*Reserve space for "Checkpoint Header" */
        pheader = ncs_enc_reserve_space(uba, sizeof(PLMS_MBCSV_HEADER));
        if (pheader == NULL) {
                free(ckpt_client_info);
		return (rc = EDU_ERR_MEM_FAIL);
	}
        ncs_enc_claim_space(uba, sizeof(PLMS_MBCSV_HEADER));

	if (cb->prev_client_info_hdl)
	{
		client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_getnext(&cb->client_info, (uns8 *)&cb->prev_client_info_hdl);
	} else {
		client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_getnext(&cb->client_info, (uns8 *)0);
		if (!client_info)
		{
			if (arg->info.encode.io_msg_type == 
						NCS_MBCSV_MSG_COLD_SYNC_RESP)
				arg->info.encode.io_msg_type = 
					NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
			else {
				if (arg->info.encode.io_msg_type == 
						NCS_MBCSV_MSG_DATA_RESP)
					arg->info.encode.io_msg_type = 
						NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
			}
			/* fill header and encode the header */ 
			msg_hdr.msg_type = PLMS_A2S_MSG_CLIENT_INFO; 
			msg_hdr.num_records = 0;

			plms_enc_mbcsv_header(pheader,msg_hdr);
			/* send async update count */
			async_upd_cnt = ncs_enc_reserve_space(uba, sizeof(uns32));
			if (async_upd_cnt == NULL) 
			{
				LOG_ER("Async update count encode Failed");
				return NCSCC_RC_FAILURE;
			}
			ncs_encode_32bit(&async_upd_cnt, cb->async_update_cnt);
			ncs_enc_claim_space(uba, sizeof(uns32));
			TRACE("Async update count sent to stdby : %d",cb->async_update_cnt);
			free(ckpt_client_info);
			return NCSCC_RC_SUCCESS;
		}
	}

	/* Now walk through the complete list and encode */
	while (client_info)
	{
		ckpt_client_info->plm_handle = client_info->plm_handle;
		ckpt_client_info->mdest_id = client_info->mdest_id;
		
		/* Encode the individual record */
		rc =  m_NCS_EDU_EXEC(&cb->edu_hdl, plms_edp_client_info_list, uba, EDP_OP_TYPE_ENC, ckpt_client_info, &ederror);
		if (rc != NCSCC_RC_SUCCESS)
		{
			free(ckpt_client_info);
			return rc;
		}
		num_rec++;
		if ( num_rec == MAX_NO_OF_CLIENT_INFO_RECS)
			break;
			
		client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_getnext(&cb->client_info, (uns8 *)&client_info->plm_handle);
	}
	if(client_info != NULL)
	{
		cb->prev_client_info_hdl = client_info->plm_handle;
	}
		
        /* fill header and encode the header */ 
	msg_hdr.msg_type = PLMS_A2S_MSG_CLIENT_INFO; 
	msg_hdr.num_records = num_rec;

	plms_enc_mbcsv_header(pheader,msg_hdr);
	free(ckpt_client_info);

	entity_grp_ptr = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_getnext(&cb->entity_group_info, (uns8 *)0);
	
	if (num_rec < MAX_NO_OF_CLIENT_INFO_RECS)
	{
		if( entity_grp_ptr == NULL) 
		{


			/* Encode the Async Update Count at standby */

			/* This will have the count of async updates that have i
			been sent, this will be 0 initially */
			async_upd_cnt = ncs_enc_reserve_space(uba, sizeof(uns32));
			if (async_upd_cnt == NULL) 
			{
				LOG_ER("Async update count encode Failed");
				return NCSCC_RC_FAILURE;
			}
			ncs_encode_32bit(&async_upd_cnt, cb->async_update_cnt);
			ncs_enc_claim_space(uba, sizeof(uns32));
			TRACE("Async update count sent to stdby : %d",cb->async_update_cnt);
			if (arg->info.encode.io_msg_type == 
						NCS_MBCSV_MSG_COLD_SYNC_RESP)
				arg->info.encode.io_msg_type = 
					NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
			else {
				if (arg->info.encode.io_msg_type == 
							NCS_MBCSV_MSG_DATA_RESP)
					arg->info.encode.io_msg_type = 
						NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
			}
		} else {
			cb->client_info_cold_sync_done = TRUE;
		}
                cb->prev_trk_step_rec = NULL;
		cb->prev_ent_grp_hdl = 0;
		cb->prev_client_info_hdl = 0;
        }

	TRACE_LEAVE2("Encoded %d records",num_rec);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : plms_mbcsv_decode_cold_sync
 *
 * Description   : This function decodes cold sync update data, based on the
 *                 record type contained in the header.
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with decode info
 *                 cb - pointer to  plms cb.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : COLD SYNC RECORDS are expected in an order
 *                 1. TRACK_STEP_INFO
 *                 2. GROUP_ENTITY_INFO
 *
 *                 For each record type,
 *                     a) decode header.
 *                     b) decode individual records for
 *                        header->num_records times,
 *****************************************************************************/

SaUint32T plms_mbcsv_decode_cold_sync_data(NCS_MBCSV_CB_ARG *cbk_arg)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	PLMS_CB * cb = plms_cb;
	EDU_ERR ederror = 0;
	PLMS_MBCSV_MSG *data = NULL;
	/*  NCS_UBAID *uba=NULL; */
	uns32 num_rec = 0,num_of_async_upd;
	PLMS_CKPT_TRACK_STEP_INFO *trk_step_info = NULL;
	PLMS_CKPT_ENTITY_GROUP_INFO *entity_grp_info = NULL;
	PLMS_CKPT_CLIENT_INFO_LIST *client_info;
	PLMS_CKPT_ENTITY_LIST *list_ptr1,*list_ptr2;
	uns8 * ptr;
	uns8 data_cnt[16];

	TRACE_ENTER();
	
	while (1) {
		data = (PLMS_MBCSV_MSG *)malloc(sizeof(PLMS_MBCSV_MSG));
		if (!data)
		{
			LOG_ER("MEM ALLOCATION FAILED");
			return NCSCC_RC_FAILURE;
		}

		memset(data,0,sizeof(PLMS_MBCSV_MSG));

		if ((rc = plms_dec_mbcsv_header(&cbk_arg->info.decode.i_uba, &data->header)) != NCSCC_RC_SUCCESS) {
			LOG_ER("COLD SYNC DECODE header failed");
			free(data);
			return NCSCC_RC_FAILURE;
		}

		if (data->header.msg_type == PLMS_A2S_MSG_TRK_STP_INFO)
		{

			/* Process the data */
			num_rec = data->header.num_records;
			TRACE("No of step info rec recd : %d",num_rec);
			while(num_rec)
			{
				trk_step_info = &data->info.step_info;
				rc = m_NCS_EDU_EXEC(&cb->edu_hdl, plms_edp_trk_step_info,&cbk_arg->info.decode.i_uba, EDP_OP_TYPE_DEC, &trk_step_info, &ederror);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("COLD SYNC track step info decode failed");
					free(data);
					return NCSCC_RC_FAILURE;
				}

				rc = plms_process_mbcsv_data(data,cbk_arg->info.decode.i_action);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("COLD SYNC mbcsv data update failed");
					free(data);
					return NCSCC_RC_FAILURE;
				}

				memset(trk_step_info,0,sizeof(PLMS_CKPT_TRACK_STEP_INFO));
				--num_rec;
			} /* end of while loop. Track step info record */
		}

		else if (data->header.msg_type == PLMS_A2S_MSG_ENT_GRP_INFO)
		{
			/* Process the data */
			num_rec = data->header.num_records;
			TRACE("Decoding Ent grp info recs. Recd %d recs",
								num_rec); 
			while(num_rec)
			{

				entity_grp_info = &data->info.ent_grp_info;
				rc = m_NCS_EDU_EXEC(&cb->edu_hdl, plms_edp_entity_grp_info,&cbk_arg->info.decode.i_uba, EDP_OP_TYPE_DEC, &entity_grp_info, &ederror);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("COLD SYNC entity grp info decode failed");
					free(data);
					return NCSCC_RC_FAILURE;
				}

				rc = plms_mbcsv_add_entity_grp_info_rec(data);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("COLD SYNC mbcsv data add failed");
					free(data);
					return NCSCC_RC_FAILURE;
				}

				memset(entity_grp_info,0,sizeof(PLMS_CKPT_ENTITY_GROUP_INFO));
				--num_rec;
			} /* end of while loop.  */
			/* Free the entity list before freeing the group */
			if (data->info.ent_grp_info.entity_list)
			{
				list_ptr1 = data->info.ent_grp_info.entity_list;
				while(list_ptr1)
				{
					list_ptr2 =  list_ptr1->next;
					free(list_ptr1);
					list_ptr1 = list_ptr2;
				}
			}
		} /* End of PLMS_A2S_MSG_ENT_GRP_INTO */
		else if (data->header.msg_type == PLMS_A2S_MSG_CLIENT_INFO ||
			data->header.msg_type == PLMS_A2S_MSG_CLIENT_DOWN_INFO)
		{
			/* Process the data */
			num_rec = data->header.num_records;
			TRACE("Decoding Client info recs. Recd %d recs",
								num_rec); 
			while(num_rec)
			{
				client_info = &data->info.client_info;
				rc = m_NCS_EDU_EXEC(&cb->edu_hdl, plms_edp_client_info_list,&cbk_arg->info.decode.i_uba, EDP_OP_TYPE_DEC, &client_info, &ederror);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("COLD SYNC client info decode failed");
					free(data);
					return NCSCC_RC_FAILURE;
				}

				rc = plms_mbcsv_add_client_info(data);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("COLD SYNC mbcsv data update failed");
					free(data);
					return NCSCC_RC_FAILURE;
				}

				memset(client_info,0,sizeof(PLMS_CKPT_CLIENT_INFO_LIST));
				--num_rec;
				/* FIXME :: UPDATE ASYNC COUNT */
			} /* end of while loop.  */
		} /* End of PLMS_A2S_MSG_CLIENT_INFO */

		if ((cbk_arg->info.decode.i_msg_type == 
				NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE) ||
			(cbk_arg->info.decode.i_msg_type ==
				NCS_MBCSV_MSG_DATA_RESP_COMPLETE))
		{
		
			/* Get the async update count */
			ptr = ncs_dec_flatten_space(&cbk_arg->info.decode.i_uba
						, data_cnt, sizeof(uns32));
			num_of_async_upd = ncs_decode_32bit(&ptr);
			cb->async_update_cnt = num_of_async_upd;
			ncs_dec_skip_space(&cbk_arg->info.decode.i_uba, 4);
			TRACE("Async update count received : %d",cb->async_update_cnt);
		}
		free(data);
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	
	} /* End of While(1) Loop */


} /* End of decode cold sync function */ 
				     


SaUint32T plms_mbcsv_encode_cold_sync_data(NCS_MBCSV_CB_ARG * arg)
{
	PLMS_CB * cb = plms_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	/* asynsc Update Count */

	TRACE_ENTER();
	
	TRACE("COLD SYNC ENCODE START........\n");
	
	/* Encode client info list */
	if (cb->client_info_cold_sync_done == FALSE)
	{
		rc = plms_edu_enc_client_info_data(arg);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("Client info data cold sync encode failed");
			return NCSCC_RC_FAILURE;
		}
	}
	else if (cb->entity_grp_cold_sync_done == FALSE)
	{
		/* Encode entity Group info list */
		rc = plms_edu_enc_entity_grp_info_data(arg);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("ENTITY GROUP info cold sync encode failed");
			return NCSCC_RC_FAILURE;
		}
	} /* End of if entity_grp_cold_sync_done  */
#if 0
	/* Encode track_step_info data */
	else if (cb->trk_step_info_cold_sync_done == FALSE)
	{
		rc = plms_edu_enc_trk_step_info_data(arg);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("TRACK STEP INFO cold sync encode failed");
			return NCSCC_RC_FAILURE;
		}
	} 
#endif

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
		
}
/****************************************************************************
 * Name          : plms_mbcsv_send_async_update
 *
 * Description   : This function makes a request to MBCSV to send an async
 *                 update to the STANDBY PLMS for the record held at
 *                 the address i_reo_hdl.
 *
 * Arguments     : cb - A pointer to the plms control block.
 *                 msg - pointer to the mbcsv msg to be
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

SaUint32T plms_mbcsv_send_async_update(PLMS_MBCSV_MSG *msg, uns32 action)
 {
	PLMS_CB * cb = plms_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_MBCSV_ARG mbcsv_arg;

	/* Fill mbcsv specific data */
	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
	mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.send_ckpt.i_action = action;
	mbcsv_arg.info.send_ckpt.i_ckpt_hdl = (NCS_MBCSV_CKPT_HDL)cb->mbcsv_ckpt_hdl;
	mbcsv_arg.info.send_ckpt.i_reo_hdl = (MBCSV_REO_HDL)(long)(msg);
	/*Will be used in encode callback */
	/* Just store the address of the data to be send as an
	 * async update record in reo_hdl. The same shall then be
	 *dereferenced during encode callback */

	mbcsv_arg.info.send_ckpt.i_reo_type = msg->header.msg_type;
	mbcsv_arg.info.send_ckpt.i_send_type = NCS_MBCSV_SND_SYNC;

	/* Send async update */
	if (NCSCC_RC_SUCCESS != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		LOG_ER(" MBCSV async send data operation failed !!.\\n");
		return NCSCC_RC_FAILURE;
	}
	return rc;
}       /*End plms_mbcsv_send_async_update() */
 

uns32 plms_mbcsv_add_trk_step_info_rec(PLMS_MBCSV_MSG *msg)
{

	PLMS_CB * cb = plms_cb;
	PLMS_CKPT_TRACK_STEP_INFO *ptr = NULL,
				  *prev_ptr = NULL,
				  *tmp_ptr = NULL;
	
	ptr = (PLMS_CKPT_TRACK_STEP_INFO *)malloc(sizeof(PLMS_CKPT_TRACK_STEP_INFO));
	if (!ptr)
	{
		LOG_ER("Mem Alloc failed in adding mbcsv trk step rec ");
		return NCSCC_RC_FAILURE;
	}
	memset(ptr,0,sizeof(PLMS_CKPT_TRACK_STEP_INFO));
	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE);
	
	/* update new rec with data received */
	ptr->dn_name.length = msg->info.step_info.dn_name.length;
	strcpy((SaInt8T *)ptr->dn_name.value,(SaInt8T *)msg->info.step_info.dn_name.value);
	ptr->change_step = msg->info.step_info.change_step;
	ptr->opr_type = msg->info.step_info.opr_type;
	
	/* Now add the new record to the list */
	tmp_ptr = cb->step_info;
	while(tmp_ptr)
	{
		prev_ptr = tmp_ptr;
		tmp_ptr = tmp_ptr->next;
	}
	if(!cb->step_info)	
		cb->step_info = ptr;
	else
		prev_ptr->next = ptr;
	ptr->next = NULL;

	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);
	return NCSCC_RC_SUCCESS;
}


SaUint32T plms_mbcsv_update_trk_step_info_rec(PLMS_MBCSV_MSG *msg)
{
	PLMS_CB * cb =plms_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	PLMS_CKPT_TRACK_STEP_INFO *tmp_ptr;
	
	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE);
	
	/* Now traverse and find the rec which is changed */
	tmp_ptr = cb->step_info;
	while(tmp_ptr)
	{
		if(strncmp((SaInt8T *)tmp_ptr->dn_name.value,(SaInt8T *)msg->info.step_info.dn_name.value, msg->info.step_info.dn_name.length) == 0)
		{
			/* Changed rec found ?? */
			tmp_ptr->change_step = msg->info.step_info.change_step;
			break;
		} else {
			/* Not this rec ?? go to next */
			tmp_ptr = tmp_ptr->next;
		}
	}
	
	/* If control is here, we either breaked or no rec found */
	if (!tmp_ptr)
	{
		/* Means we didn't find rec with the given name */
		LOG_ER("Unable to fetch the record at local database");
		rc = NCSCC_RC_FAILURE;
	} else 
	{
		/* Means everything went fine */
		rc = NCSCC_RC_SUCCESS;
	}

	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);
	return rc;
}

SaUint32T plms_mbcsv_rem_trk_step_info_rec(PLMS_MBCSV_MSG *msg)
{
	PLMS_CB * cb = plms_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	PLMS_CKPT_TRACK_STEP_INFO *tmp_ptr,*prev_ptr = NULL;
	
	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE);
	
	/* Now traverse and find the rec which should be removed */
	tmp_ptr = cb->step_info;
	while(tmp_ptr)
	{
		if(strncmp((SaInt8T *)tmp_ptr->dn_name.value,(SaInt8T *)msg->info.step_info.dn_name.value, msg->info.step_info.dn_name.length) == 0)
		{
			/* rec to be removed found  ?? */
			/* If this is the first rec ?? */
			if (prev_ptr == NULL)
			{
				cb->step_info = tmp_ptr->next;
				free(tmp_ptr);
			} else {
				prev_ptr->next = tmp_ptr->next;
				free(tmp_ptr);
			}
			break;
		} else {
			/* Not this rec ?? go to next */
			prev_ptr = tmp_ptr;
			tmp_ptr = tmp_ptr->next;
		}
	}
	
	/* If control is here, we either breaked or no rec found */
	if (!tmp_ptr)
	{
		/* Means we didn't find rec with the given name */
		LOG_ER("Unable to fetch the record to be removed at local database");
		rc = NCSCC_RC_FAILURE;
	} else 
	{
		/* Means everything went fine */
		rc = NCSCC_RC_SUCCESS;
	}

	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);
	return rc;
}

SaUint32T plms_mbcsv_add_entity_grp_info_rec(PLMS_MBCSV_MSG *msg)
{
	PLMS_CB * cb = plms_cb;
	PLMS_CKPT_ENTITY_GROUP_INFO *ptr = NULL,*prev = NULL,*tmp = NULL;
	PLMS_CKPT_ENTITY_LIST       *cur_list_ptr = NULL,*prev_list_ptr = NULL,
					*msg_list_ptr= NULL,*tmp_list_ptr = NULL;
	SaInt8T                         str[256] = {0};				

	TRACE_ENTER();

	/* Since the same function is used for cold sync add and async update, first we see if the entity grp info already exist. if exists, modify the existing record, if not, create a new record and add it to the list */
	
	/* Traverse the list and find out the right group */
	ptr = cb->ckpt_entity_grp_info;
	while(ptr)
	{
		if (ptr->entity_group_handle == msg->info.ent_grp_info.entity_group_handle)
		{
			/* here means we found out the group info structure. Add			   the entity names to the entity list */
			msg_list_ptr = msg->info.ent_grp_info.entity_list;
	
			while (msg_list_ptr)
			{
				/* First allocate mem for entity list and add it to group info */
				cur_list_ptr = (PLMS_CKPT_ENTITY_LIST *)malloc(sizeof(PLMS_CKPT_ENTITY_LIST));
				if (!cur_list_ptr)
				{
					/* FIXME : See if anything needs to freed*/
					LOG_ER("Mem alloc failed for entity list");
			
					return NCSCC_RC_FAILURE;
				}
				memset(cur_list_ptr,0,sizeof(PLMS_CKPT_ENTITY_LIST));
				
				strncpy(str,
				(SaInt8T *)msg_list_ptr->entity_name.value,
				msg_list_ptr->entity_name.length);
				
				TRACE("Entity %s is added",str);
				
				memcpy(&cur_list_ptr->entity_name,
					&msg_list_ptr->entity_name,
					sizeof(SaNameT));
				
				if (msg_list_ptr->root_entity_name.length)
				{
					memcpy(&cur_list_ptr->root_entity_name,
						&msg_list_ptr->root_entity_name,
						sizeof(SaNameT));
				}

				/* Now add this entity list to group info */
				tmp_list_ptr = ptr->entity_list;
				while (tmp_list_ptr)
				{
					prev_list_ptr = tmp_list_ptr;
					tmp_list_ptr = tmp_list_ptr->next;
				}
				if (prev_list_ptr == NULL)
				{
					/* it means no earlier list exists */
					ptr->entity_list = cur_list_ptr;
					ptr->entity_list->next = NULL;
					prev_list_ptr = ptr->entity_list;
				} else {
					/* Means there are records */
					prev_list_ptr->next = cur_list_ptr;
					cur_list_ptr->next = NULL;
					prev_list_ptr = cur_list_ptr;
						
				}
				msg_list_ptr = msg_list_ptr->next;
			} /* End of while (msg_list_ptr) */
			ptr->track_flags = msg->info.ent_grp_info.track_flags;
			ptr->track_cookie = msg->info.ent_grp_info.track_cookie;

			TRACE_LEAVE();
			return NCSCC_RC_SUCCESS;
		} /* end of comparing group handles */
		
		ptr = ptr->next; /* Grp info is not the one we are searching */
		
	} /* End of While ptr */

	if ( ptr == NULL) /* Means we didn't find existing entity grp info */
	{
	
		ptr = (PLMS_CKPT_ENTITY_GROUP_INFO *)malloc(sizeof(PLMS_CKPT_ENTITY_GROUP_INFO));

		if (!ptr)
		{
			LOG_ER("Mem Alloc failed for adding entity grp info rec");
			assert(0);
		}
		memset(ptr, 0 , sizeof(PLMS_CKPT_ENTITY_GROUP_INFO));

		/* copy data received into new record */
		ptr->entity_group_handle = msg->info.ent_grp_info.entity_group_handle;
		ptr->agent_mdest_id = msg->info.ent_grp_info .agent_mdest_id;
		msg_list_ptr = msg->info.ent_grp_info.entity_list;
		
		while(msg_list_ptr)
		{
			
			/* First allocate mem for entity list and add it to group info */
			cur_list_ptr = (PLMS_CKPT_ENTITY_LIST *)malloc(sizeof(PLMS_CKPT_ENTITY_LIST));
			if (!cur_list_ptr)
			{
				/* FIXME : See if anything needs to freed*/
				LOG_ER("Mem alloc failed for entity list");
				
				return NCSCC_RC_FAILURE;
			}
			memset(cur_list_ptr, 0 , sizeof(PLMS_CKPT_ENTITY_LIST));
			strncpy(str,
				(SaInt8T *)msg_list_ptr->entity_name.value,
				msg_list_ptr->entity_name.length);
			
			TRACE("Entity %s is added",str);

			memcpy(&cur_list_ptr->entity_name,
				&msg_list_ptr->entity_name,
				sizeof(SaNameT));
				
			if (msg_list_ptr->root_entity_name.length)
			{
				memcpy(&cur_list_ptr->root_entity_name,
					&msg_list_ptr->root_entity_name,
					sizeof(SaNameT));
			}

			/* Now add this entity list to group info */
			if (!prev_list_ptr)
			{
				/* Means this is the first one to be added */
				ptr->entity_list = cur_list_ptr;
				prev_list_ptr = cur_list_ptr;
			}else {
				prev_list_ptr->next = cur_list_ptr;
				prev_list_ptr = cur_list_ptr;
			}
			msg_list_ptr = msg_list_ptr->next;
		} /* end of while (msg_list_ptr) */
		ptr->track_flags = msg->info.ent_grp_info.track_flags;
		ptr->track_cookie = msg->info.ent_grp_info.track_cookie;

		/* Now add the new record to the list */
		tmp = cb->ckpt_entity_grp_info;
		while(tmp)
		{
			prev = tmp;
			tmp = tmp->next;
		}
		if (!prev)
		{
			/* Means this is the first rec to be added */
			cb->ckpt_entity_grp_info = ptr;
			prev = ptr;
			ptr->next = NULL;
		} else {
			/* Means there are other records also. So add to tail */
			prev->next = ptr;
			ptr->next = NULL;
		}

	} 
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
		 		
}

SaUint32T plms_mbcsv_rmv_entity_grp_info_rec(PLMS_MBCSV_MSG *msg)
{
	PLMS_CB * cb = plms_cb;
	PLMS_CKPT_ENTITY_GROUP_INFO *ptr,*prev_grp_node = NULL;
	PLMS_CKPT_ENTITY_LIST       *grp_ptr,*msg_ptr,*prev_ptr;
	SaInt8T                      str[256];				
	SaUint16T                    group_remove = 0;

	TRACE_ENTER();

	ptr = cb->ckpt_entity_grp_info;
	
	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE);

	/* Now find the record with group handle */
	while(ptr)
	{
		if (ptr->entity_group_handle == msg->info.ent_grp_info.entity_group_handle)
		{
			/* We found out the entity group, now remove the entities from the list */
			msg_ptr = msg->info.ent_grp_info.entity_list;
			if (msg_ptr == NULL)
			{
				/* if there are no entities to be removed means remove group */
				group_remove = 1;
			}
			while (msg_ptr)
			{
				/* For every entity begin from first */
				prev_ptr = NULL;
				grp_ptr = ptr->entity_list;
				while ( grp_ptr)
				{
					if (strncmp((SaInt8T *)grp_ptr->entity_name.value,
						    (SaInt8T *)msg_ptr->entity_name.value,
					      msg_ptr->entity_name.length)== 0)
					 {
						strncpy(str,
					(SaInt8T *)grp_ptr->entity_name.value, 
					grp_ptr->entity_name.length);
						
						TRACE("Entity %s is removed",str);


						 /* means entity needs to be removed. Here we doesn't check for subtree flag as it is already checked at the active side */
						 if (prev_ptr == NULL)
						 {
							 ptr->entity_list = grp_ptr->next;
							 /*  since entity is removed, mark prev_ptr = null for next cycle */
							 free(grp_ptr);
							 break;
						 } else {
							 prev_ptr->next = grp_ptr->next;
							 /*  since entity is removed, mark prev_ptr = null for next cycle */
							 free(grp_ptr);
							 break;
						 }
					 }
					 prev_ptr = grp_ptr;
					 grp_ptr = grp_ptr->next;
				} /* End of while (grp_ptr ) */
				msg_ptr = msg_ptr->next;
			} /* End of while(msg_ptr) */

			if (group_remove)
			{
				if (prev_grp_node)
				{
					prev_grp_node->next = ptr->next;
				}else {
					cb->ckpt_entity_grp_info = ptr->next;
				}
				TRACE("Group Deleted with Grp Hdl : %llu",
						ptr->entity_group_handle );
					 
				free(ptr);
			}
			
			m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);
			TRACE_LEAVE();
			return NCSCC_RC_SUCCESS;
			
		} /* End of if (ptr->entity_group_handle */
		prev_grp_node = ptr;
		ptr = ptr->next;
	}
	TRACE("Group with grp hdl : %llu not found", msg->info.ent_grp_info.entity_group_handle);
	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

SaUint32T plms_mbcsv_add_client_info(PLMS_MBCSV_MSG *msg) 
{
	TRACE_ENTER();
	PLMS_CLIENT_INFO *client_info;
	PLMS_CB *cb = plms_cb;

	client_info = (PLMS_CLIENT_INFO *)malloc(sizeof(PLMS_CLIENT_INFO));
	if (!client_info)
	{
		LOG_ER("Memory Allocation failed");
		assert(0);
	}
	
	if(msg->info.client_info.plm_handle > plm_handle_pool )
		plm_handle_pool = msg->info.client_info.plm_handle;

	memset(client_info,0,sizeof(PLMS_CLIENT_INFO));
	client_info->plm_handle = msg->info.client_info.plm_handle;
	client_info->mdest_id = msg->info.client_info.mdest_id; 

	client_info->pat_node.key_info = (uns8*)&client_info->plm_handle;

        if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&cb->client_info,
                                               &client_info->pat_node))
        {
                LOG_ER("PLMS: ADD NEW CLIENT NODE FAILED");
                free(client_info);
		return NCSCC_RC_FAILURE;
        }
	TRACE("Client added with plmHandle %llu,",client_info->plm_handle);
	TRACE_LEAVE();
        return NCSCC_RC_SUCCESS;
}

SaUint32T plms_mbcsv_rmv_client_info(PLMS_MBCSV_MSG *msg)
{
	TRACE_ENTER();
	PLMS_CLIENT_INFO *client_info;
	PLMS_CB *cb = plms_cb;
	SaUint32T     rc = NCSCC_RC_SUCCESS;

	client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_get(
			&cb->client_info,
			(uns8 *)&msg->info.client_info.plm_handle);
	if (!client_info)
	{
		/* LOG ERROR */
		LOG_ER("Unable to find check pointed client info record");
		return NCSCC_RC_FAILURE;
	}
	/* Delete the client info details from data base */
	rc =  ncs_patricia_tree_del(&cb->client_info,&client_info->pat_node);
	if (rc != NCSCC_RC_SUCCESS)
	{
		TRACE("Unable to delete checkpointed client info rec");
		/* FIXME Wat to do */
		return NCSCC_RC_FAILURE;
	}
	TRACE("Client deleted with plmHandle %llu,",client_info->plm_handle);
       
        free(client_info);
	TRACE_LEAVE();

        return NCSCC_RC_SUCCESS;
}

SaUint32T plms_process_mbcsv_data(PLMS_MBCSV_MSG *msg,NCS_MBCSV_ACT_TYPE action)		  
{	
	uns32 rc = NCSCC_RC_SUCCESS;
	PLMS_CKPT_ENTITY_LIST *list_ptr1,*list_ptr2;
	PLMS_CB *cb = plms_cb;

	if ((!cb) || (msg == NULL))
		return (rc = NCSCC_RC_FAILURE);

	if ((cb->ha_state == SA_AMF_HA_STANDBY) || (cb->ha_state == 
						SA_AMF_HA_QUIESCED)) {
		if (msg->header.msg_type == PLMS_A2S_MSG_TRK_STP_INFO) 
		{
			if (action == NCS_MBCSV_ACT_ADD)
			{
				rc = plms_mbcsv_add_trk_step_info_rec(msg);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("Unable to add trk step info rec to database");
					return rc;
				}
			} else if (action == NCS_MBCSV_ACT_UPDATE)
			{
				rc = plms_mbcsv_update_trk_step_info_rec(msg);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("Unable to modify trk step info record database");
					return rc;
				}
			}else if (action == NCS_MBCSV_ACT_RMV)
			{
				rc = plms_mbcsv_rem_trk_step_info_rec(msg);
				if(rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("Unable to remove trk step info rec");
					return rc;
				}
			}
			
		}
		else if (msg->header.msg_type == PLMS_A2S_MSG_ENT_GRP_INFO)
		{
			if (action == NCS_MBCSV_ACT_ADD)
			{
				rc = plms_mbcsv_add_entity_grp_info_rec(msg);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("Unable to add entity grp info rec to database");
					return rc;
				}
			}else if (action == NCS_MBCSV_ACT_UPDATE)
			{
				/* For update and add we call the same funcn */
				rc = plms_mbcsv_add_entity_grp_info_rec(msg);
				if(rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("Unable to update entity grp info rec ");
					return rc;
				}
			}else if (action == NCS_MBCSV_ACT_RMV)
			{
				rc = plms_mbcsv_rmv_entity_grp_info_rec(msg);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("Unable to remove entity grp info rec");
					return rc;
				}
			}
			/* Free the entity list in msg */
			if (msg->info.ent_grp_info.entity_list)
			{
				list_ptr1 = msg->info.ent_grp_info.entity_list;
				while(list_ptr1)
				{
					list_ptr2 =  list_ptr1->next;
					free(list_ptr1);
					list_ptr1 = list_ptr2;
				}
			}
					
			
		} /* end of else if msg->header.msg_type condition */
		else if (msg->header.msg_type == PLMS_A2S_MSG_CLIENT_INFO)
		{
			if (action == NCS_MBCSV_ACT_ADD)
			{
				rc = plms_mbcsv_add_client_info(msg);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("Unable to add client info rec to database");
					return rc;
				}
			}else if (action == NCS_MBCSV_ACT_UPDATE)
			{
				/* FIXME: This should not happen */
				LOG_ER("Got update for clientinfo");
				return NCSCC_RC_SUCCESS;
				
			}else if (action == NCS_MBCSV_ACT_RMV)
			{
				rc = plms_mbcsv_rmv_client_info(msg);
				if (rc != NCSCC_RC_SUCCESS)
				{
					LOG_ER("Unable to remove client info");
					return rc;
				}
			}
					
			
		} /* end of else if msg->header.msg_type condition */
	} /* End of if cb->ha_state */
	
	return NCSCC_RC_SUCCESS;
}


/******************************************************************************
 * Name          : plms_mbcsv_enc_warm_sync_resp

 * Description   : Sends async update count to Standby 

 * Arguments     : PLMS_CB, NCS_MBCSV_CB_ARG - Mbcsv callback argument
 *
 * Return Values :  Success / Error
 *
 * Notes : This is called at the active side
*******************************************************************************/
SaUint32T plms_mbcsv_enc_warm_sync_resp(PLMS_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	uns8 *wsync_ptr;

	/* Reserve space to send the async update counter */
	wsync_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
	if (wsync_ptr == NULL) {
		LOG_ER("PLMS mem alloc failed for encoding warm sync resp");
		return NCSCC_RC_FAILURE;
	}
	/* SEND THE ASYNC UPDATE COUNTER */
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
	ncs_encode_32bit(&wsync_ptr, cb->async_update_cnt);
	arg->info.encode.io_msg_type = NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE;
	return rc;
}
      

/************************************************************************** **
* Name          : plms_mbcsv_dec_warm_sync_resp
*
* Description   : Decodes the warm sync message resp at standby

* Arguments     : NCS_MBCSV_CB_ARG - MBCSv callback argument
*
* Return Values : Success / Error
*******************************************************************************/
SaUint32T plms_mbcsv_dec_warm_sync_resp(NCS_MBCSV_CB_ARG *arg)
{
	PLMS_CB * cb = plms_cb;
	uns32 num_of_async_upd, rc = NCSCC_RC_SUCCESS;
	uns8 data[16], *ptr;
	NCS_MBCSV_ARG ncs_arg;

	TRACE_ENTER();

	memset(&ncs_arg, '\0', sizeof(NCS_MBCSV_ARG));

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(int32));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, 4);

	if (cb->async_update_cnt == num_of_async_upd) 
	{
		return rc;
	} else {
		plms_clean_mbcsv_database();
		ncs_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
		ncs_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
		ncs_arg.info.send_data_req.i_ckpt_hdl = cb->mbcsv_ckpt_hdl;
		rc = ncs_mbcsv_svc(&ncs_arg);
		if (rc != NCSCC_RC_SUCCESS) 
		{
			LOG_ER("Unable to send warm sync data req");	
			return rc;
		}
	}
	TRACE_LEAVE();
	return rc;
}
