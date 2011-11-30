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
  FILE NAME: cpd_mbcsv.c

  DESCRIPTION: CPD MBCSv Interface Processing Routines

******************************************************************************/

#include "cpd.h"

/**********************************************************************************************
 * Name                   : cpd_mbcsv_async_update
 *
 * Description            : Populates the MBCSv structure and sends the message using MBCSv

 * Arguments              : CPD_MBCSV_MSG - MBCSv message 
 *
 * Return Values          : Success / Error
 *
 * Notes                  : None
**********************************************************************************************/
uns32 cpd_mbcsv_async_update(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = SA_AIS_OK;

	/* populate the arg structure */
	arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	arg.info.send_ckpt.i_ckpt_hdl = cb->o_ckpt_hdl;
	arg.info.send_ckpt.i_send_type = NCS_MBCSV_SND_SYNC;
	arg.info.send_ckpt.i_reo_type = msg->type;
	arg.info.send_ckpt.i_reo_hdl = NCS_PTR_TO_UNS64_CAST(msg);	/* Address of CPD_MBCSV_MSG ,-- which can be used later in encode of async update */
	arg.info.send_ckpt.i_action = NCS_MBCSV_ACT_UPDATE;

	/*send the message using MBCSv */
	rc = ncs_mbcsv_svc(&arg);
	if (rc != SA_AIS_OK) {
		m_LOG_CPD_CL(CPD_MBCSV_ASYNC_UPDATE_SEND_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}
	return rc;
}

/*****************************************************************************************
* Name         :  cpd_mbcsv_register
*
* Description  : This is used by CPD to register with the MBCSv , this will call init , open , selection object
*
* Arguments    : CPD_CB - cb
*
* Return Values : Success / Error
*
* Notes   : This function first calls the mbcsv_init then does an open and does a selection object get
*****************************************************************************************/
uns32 cpd_mbcsv_register(CPD_CB *cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	rc = cpd_mbcsv_init(cb);
	if (rc != NCSCC_RC_SUCCESS)
		return rc;

	rc = cpd_mbcsv_open(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		goto error;
	}

	/* Selection object get */
	rc = cpd_mbcsv_selobj_get(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		goto fail;
	}
	return rc;

 fail:
	cpd_mbcsv_close(cb);
 error:
	cpd_mbcsv_finalize(cb);
	return rc;
}

/************************************************************************************************
 * Name                   : cpd_mbcsv_init
 *
 * Description            : To initialize with MBCSv

 * Arguments              : CPD_CB : cb pointer
 *
 * Return Values          : Success / Error
 *
 * Notes                  : None
*************************************************************************************************/
uns32 cpd_mbcsv_init(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	arg.info.initialize.i_mbcsv_cb = cpd_mbcsv_callback;
	arg.info.initialize.i_version = CPSV_CPD_MBCSV_VERSION;
	arg.info.initialize.i_service = NCS_SERVICE_ID_CPD;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		m_LOG_CPD_CL(CPD_MBCSV_INIT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	cb->mbcsv_handle = arg.info.initialize.o_mbcsv_hdl;
	return rc;
}

/*********************************************************************************************************
 * Name                   : cpd_mbcsv_open
 *
 * Description            : To open a session with MBCSv
 * Arguments              : CPD_CB - cb pointer
 *
 * Return Values          : Success / Error 
 *
 * Notes                  : Open call will set up a session between the peer entities. o_ckpt_hdl returned
                            by the OPEN call will uniquely identify the checkpoint session.
*********************************************************************************************************/
uns32 cpd_mbcsv_open(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_OPEN;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	arg.info.open.i_pwe_hdl = (uns32)cb->mds_handle;
	arg.info.open.i_client_hdl = cb->cpd_hdl;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		m_LOG_CPD_CL(CPD_MBCSV_OPEN_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	cb->o_ckpt_hdl = arg.info.open.o_ckpt_hdl;
	return rc;
}

/********************************************************************************************************
* Name         :  cpd_mbcsv_selobj_get
*
* Description  : To get a handle from OS to process the pending callbacks
*
* Arguments  : CPD_CB 

* Return Values : Success / Error
*
* Notes  : To receive a handle from OS and can be used in further processing of pending callbacks 
********************************************************************************************************/
uns32 cpd_mbcsv_selobj_get(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	uns32 rc = NCSCC_RC_SUCCESS;

	arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;

	if (ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_CL(CPD_MBCSV_GET_SEL_OBJ_FAILURE, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	cb->mbcsv_sel_obj = arg.info.sel_obj_get.o_select_obj;
	return rc;
}

/******************************************************************************************************************
 * Name                   : cpd_mbcsv_chgrole
 *
 * Description            : To assign  role for a component

 * Arguments              : CPD_CB  - cb pointer
 *
 * Return Values          : Success / Error
 *
 * Notes                  : This API is use for setting the role. Role Standby - initiate Cold Sync Request if it finds Active
                            Role Active - send ckpt data to multiple standby peers
 *********************************************************************************************************************/
uns32 cpd_mbcsv_chgrole(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	arg.info.chg_role.i_ckpt_hdl = cb->o_ckpt_hdl;
	arg.info.chg_role.i_ha_state = cb->ha_state;	/*  ha_state is assigned at the time of amf_init where csi_set_callback will assign the state */

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		m_LOG_CPD_CL(CPD_MBCSV_CHGROLE_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
	}
	return rc;
}

/***********************************************************************
 * Name          : cpd_mbcsv_close
 *
 * Description   : To close the association
 *
 * Arguments     :
 *
 * Return Values : Success / Error
****************************************************************************/
uns32 cpd_mbcsv_close(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_CLOSE;
	arg.info.close.i_ckpt_hdl = cb->o_ckpt_hdl;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		m_LOG_CPD_CL(CPD_MBCSV_CLOSE_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
	}
	return rc;
}

/*********************************************************************************************************
 * Name           :  cpd_mbcsv_finalize
 *
 * Description    : To close the association of CPD with MBCSv

 * Arguments      : CPD_CB   - cb pointer
 *
 * Return Values  : Success / Error
 *
 * Notes          : Closes the association, represented by i_mbc_hdl, between the invoking process and MBCSV
***********************************************************************************************************/
uns32 cpd_mbcsv_finalize(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_FINALIZE;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		m_LOG_CPD_CL(CPD_MBCSV_FINALIZE_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
	}
	return rc;
}

/**************************************************************************************************
 * Name            :  cpd_mbcsv_callback
 *
 * Description     : To process the MBCSv Callbacks

 * Arguments       :  NCS_MBCSV_CB_ARG  - arg set delivered at the single entry callback
 *
 * Return Values   : Success / Error
 *
 * Notes           : Callbacks from MBCSV to user
***************************************************************************************************/
uns32 cpd_mbcsv_callback(NCS_MBCSV_CB_ARG *arg)
{

	uns32 rc = NCSCC_RC_SUCCESS;

	if (arg == NULL) {
		rc = NCSCC_RC_FAILURE;
		m_LOG_CPD_CL(CPD_MBCSV_CALLBACK_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}

	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		rc = cpd_mbcsv_encode_proc(arg);
		break;

	case NCS_MBCSV_CBOP_DEC:
		rc = cpd_mbcsv_decode_proc(arg);
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
	m_LOG_CPD_CL(CPD_MBCSV_CALLBACK_SUCCESS, CPD_FC_MBCSV, NCSFL_SEV_INFO, __FILE__, __LINE__);
	return rc;
}

/*******************************************************************************************
 * Name           : cpd_mbcsv_enc_async_update
 *
 * Description    : To encode the data and to send it to Standby at the time of Async Update

 * Arguments      : NCS_MBCSV_CB_ARG - MBCSv callback Argument
 *
 * Return Values  : Success / Error

 * Notes          : from io_reo_type - the event is determined and based on the event we encode the MBCSv_MSG
                    This is called at the active side
*******************************************************************************************/
uns32 cpd_mbcsv_enc_async_update(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	CPD_MBCSV_MSG *cpd_msg;
	uns32 rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	uns8 *cpd_type_ptr;

	/*  Increment the async update count cb->cpd_sync_cnt     */
	cb->cpd_sync_cnt++;

	  TRACE("ENC ASYNC COUNT %d",cb->cpd_sync_cnt);  

	cpd_type_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
	if (cpd_type_ptr == NULL) {
		m_LOG_CPD_MEMFAIL(NCS_ENC_RESERVE_SPACE_FAILED);
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));
	ncs_encode_8bit(&cpd_type_ptr, arg->info.encode.io_reo_type);

	switch (arg->info.encode.io_reo_type) {

	case CPD_A2S_MSG_CKPT_CREATE:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_CREATE), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.ckpt_create, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_CREATE_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_UNLINK:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_UNLINK), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.ckpt_ulink, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_UNLINK_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_RDSET:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_RDSET), &arg->info.encode.io_uba, EDP_OP_TYPE_ENC,
				    &cpd_msg->info.rd_set, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_RDSET_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_AREP_SET:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.arep_set, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_AREP_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_ADD:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.dest_add, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_DESTADD_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_DEL:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.dest_del, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_DESTDEL_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_USR_INFO:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_USR_INFO), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.usr_info, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_USRINFO_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_DOWN:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.dest_down, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			/* LOG */
			rc = NCSCC_RC_FAILURE;
		}
		break;

	default:
		return NCSCC_RC_FAILURE;
	}
	return rc;
}

/*****************************************************************************************************
 * Name            : cpd_mbcsv_enc_msg_resp
 *
 * Description     : To encode the message that is to be sent to Standby for Cold Sync

 * Arguments       : NCS_MBCSV_CB_ARG - Mbcsv callback argument
 *
 * Return Values   : Success / Error
 *
 * Notes : This is called at the active side
 |------------------|---------------|-----------|------|-----------|-----------| 
 |No. of Ckpts      | ckpt record 1 |ckpt rec 2 |..... |ckpt rec n | async upd |
 |that will be sent |               |           |      |           | cnt ( 0 ) |
 |------------------|---------------------------|------|-----------|-----------|
 *****************************************************************************************************/

uns32 cpd_mbcsv_enc_msg_resp(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	CPD_CKPT_INFO_NODE *ckpt_node;
	CPD_A2S_CKPT_CREATE ckpt_create;
	SaCkptCheckpointHandleT prev_ckpt_id = 0;
	uns32 rc = NCSCC_RC_SUCCESS, i;
	CPD_NODE_REF_INFO *nref_info;
	EDU_ERR ederror = 0;
	uns8 *header, num_ckpt = 0, *sync_cnt_ptr;
	NCS_BOOL flag = FALSE;

	/* COLD_SYNC_RESP IS DONE BY THE ACTIVE */
	if (cb->ha_state == SA_AMF_HA_STANDBY) {
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	/*  cb->prev_ckpt_id = 0; */
	prev_ckpt_id = cb->prev_ckpt_id;

	ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(&cb->ckpt_tree, (uns8 *)&prev_ckpt_id);
	if (ckpt_node == NULL) {
		rc = NCSCC_RC_SUCCESS;
		arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
		cb->cold_or_warm_sync_on = FALSE;
	}

	/* First reserve space to store the number of checkpoints that will be sent */

	header = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns8));
	if (header == NULL) {
		m_LOG_CPD_MEMFAIL(NCS_ENC_RESERVE_SPACE_FAILED);
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns8));

	while (ckpt_node) {

		if (ckpt_node->is_unlink_set == TRUE ) {
			TRACE_1("cpd mbcsv enc continue  for ckpt_id %llx",ckpt_node->ckpt_id);
			prev_ckpt_id = ckpt_node->ckpt_id;
			ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(&cb->ckpt_tree, (uint8_t *)&prev_ckpt_id);
			continue;
		}
		
		nref_info = ckpt_node->node_list;

		/* Populate the A2S_CKPT_CREATE structure */
		ckpt_create.ckpt_name = ckpt_node->ckpt_name;
		ckpt_create.ckpt_id = ckpt_node->ckpt_id;
		ckpt_create.ckpt_attrib = ckpt_node->attributes;
		ckpt_create.is_unlink_set = ckpt_node->is_unlink_set;
		ckpt_create.is_active_exists = ckpt_node->is_active_exists;
		ckpt_create.create_time = ckpt_node->create_time;
		ckpt_create.num_users = ckpt_node->num_users;
		ckpt_create.num_writers = ckpt_node->num_writers;
		ckpt_create.num_readers = ckpt_node->num_readers;
		ckpt_create.num_sections = ckpt_node->num_sections;
		ckpt_create.active_dest = ckpt_node->active_dest;
		ckpt_create.dest_cnt = ckpt_node->dest_cnt;
		ckpt_create.dest_list = NULL;

		if (ckpt_node->dest_cnt) {
			ckpt_create.dest_list = m_MMGR_ALLOC_CPSV_CPND_DEST_INFO(ckpt_node->dest_cnt);
			if (ckpt_create.dest_list == NULL) {
				m_LOG_CPD_CL(CPD_CPND_DEST_INFO_ALLOC_FAILED, CPD_FC_MEMFAIL, NCSFL_SEV_ERROR, __FILE__,
					     __LINE__);
				rc = NCSCC_RC_OUT_OF_MEM;
				return rc;
			} else {
				memset(ckpt_create.dest_list, 0, (sizeof(CPSV_CPND_DEST_INFO) * ckpt_node->dest_cnt));
				for (i = 0; i < ckpt_node->dest_cnt; i++) {
					ckpt_create.dest_list[i].dest = nref_info->dest;
					nref_info = nref_info->next;
				}
			}
		}

		num_ckpt++;

		/* DO THE EDU EXEC */
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_CREATE), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &ckpt_create, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_MBCSV(EDU_EXEC_ASYNC_CREATE_FAILED, NCSFL_SEV_ERROR);
			m_MMGR_FREE_CPSV_CPND_DEST_INFO(ckpt_create.dest_list);
			return rc;
		}

		if (ckpt_create.dest_list)
			m_MMGR_FREE_CPSV_CPND_DEST_INFO(ckpt_create.dest_list);

		prev_ckpt_id = ckpt_node->ckpt_id;
		if (num_ckpt == CPD_MBCSV_MAX_MSG_CNT)
			break;

		ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(&cb->ckpt_tree, (uns8 *)&prev_ckpt_id);

	}			/* End of While */

	cb->prev_ckpt_id = prev_ckpt_id;	/* To preserve the next ckpt_id for cold sync (after 50 to hold 51st) */
	ncs_encode_8bit(&header, num_ckpt);

	/* This will have the count of async updates that have been sent , this will be 0 initially */
	sync_cnt_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
	if (sync_cnt_ptr == NULL) {
		m_LOG_CPD_MEMFAIL(NCS_ENC_RESERVE_SPACE_FAILED);
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
	ncs_encode_32bit(&sync_cnt_ptr, cb->cpd_sync_cnt);

	ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(&cb->ckpt_tree, (uns8 *)&prev_ckpt_id);
	if (ckpt_node == NULL) {
		flag = TRUE;
	}
	if ((num_ckpt < CPD_MBCSV_MAX_MSG_CNT) || (flag == TRUE)) {
		if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP) {
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
			cb->prev_ckpt_id = 0;
			cb->cold_or_warm_sync_on = FALSE;

		} else {
			if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_DATA_RESP) {
				arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
				cb->prev_ckpt_id = 0;
				cb->cold_or_warm_sync_on = FALSE;

			}
		}

	}
	return rc;
}

/************************************************************************************************
 * Name          :  cpd_mbcsv_enc_warm_sync_resp

 * Description   : To encode the message that is to be sent to Standby at the time of warm sync

 * Arguments     : NCS_MBCSV_CB_ARG - Mbcsv callback argument
 *
 * Return Values :  Success / Error
 *
 * Notes : This is called at the active side
************************************************************************************************/
uns32 cpd_mbcsv_enc_warm_sync_resp(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	uns8 *wsync_ptr;

	/* Reserve space to send the async update counter */
	wsync_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uns32));
	if (wsync_ptr == NULL) {
		m_LOG_CPD_MEMFAIL(NCS_ENC_RESERVE_SPACE_FAILED);
		return NCSCC_RC_FAILURE;
	}

	/* SEND THE ASYNC UPDATE COUNTER */
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uns32));
	ncs_encode_32bit(&wsync_ptr, cb->cpd_sync_cnt);
	arg->info.encode.io_msg_type = NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE;
	return rc;
}

/**************************************************************************************
 * Name            : cpd_mbcsv_encode_proc
 *
 * Description     : To call different callbacks ASYNC_UPDATE , COLD_SYNC , WARM_SYNC 

 * Arguments       : NCS_MBCSV_CB_ARG - Mbcsv callback argument
 *
 * Return Values   : Success / Error
 *
****************************************************************************************/
uns32 cpd_mbcsv_encode_proc(NCS_MBCSV_CB_ARG *arg)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPD_CB *cb;
	uns16 msg_fmt_version;

	m_CPD_RETRIEVE_CB(cb);	/* finally give up the handle */
	if (cb == NULL) {
		m_LOG_CPD_HEADLINE(CPD_DESTROY_FAIL, NCSFL_SEV_ERROR);
		return (NCSCC_RC_FAILURE);
	}

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.encode.i_peer_version,
					      CPSV_CPD_MBCSV_VERSION, CPSV_CPD_MBCSV_VERSION_MIN);

	if (msg_fmt_version) {
		switch (arg->info.encode.io_msg_type) {

		case NCS_MBCSV_MSG_ASYNC_UPDATE:
			rc = cpd_mbcsv_enc_async_update(cb, arg);
			break;

		case NCS_MBCSV_MSG_COLD_SYNC_REQ:
			break;

		case NCS_MBCSV_MSG_COLD_SYNC_RESP:
			rc = cpd_mbcsv_enc_msg_resp(cb, arg);
			return rc;
			break;

		case NCS_MBCSV_MSG_WARM_SYNC_REQ:
			break;

		case NCS_MBCSV_MSG_WARM_SYNC_RESP:
			rc = cpd_mbcsv_enc_warm_sync_resp(cb, arg);
			break;

		case NCS_MBCSV_MSG_DATA_REQ:
			break;

		case NCS_MBCSV_MSG_DATA_RESP:
			rc = cpd_mbcsv_enc_msg_resp(cb, arg);
			break;

		default:
			rc = NCSCC_RC_FAILURE;
			break;
		}
	} else {
		rc = NCSCC_RC_FAILURE;
	}

	ncshm_give_hdl(cb->cpd_hdl);
	return rc;
}

/***********************************************************************************
 * Name        : cpd_mbcsv_dec_async_update
 *
 * Description : To decode the async update at the Standby, so the first field is decoded which will tell the type
                 and based on the event, a corresponding action will be taken

 * Arguments   : NCS_MBCSV_CB_ARG - MBCSv callback argument
 * 
 * Return Values : Success / Error
***********************************************************************************/

uns32 cpd_mbcsv_dec_async_update(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uns8 *ptr;
	uns8 data[4];
	CPD_MBCSV_MSG *cpd_msg;
	CPD_A2S_CKPT_CREATE *ckpt_create = NULL;
	CPD_A2S_CKPT_UNLINK *ckpt_unlink = NULL;
	CPSV_CKPT_RDSET *ckpt_rdset = NULL;
	CPSV_CKPT_DEST_INFO *ckpt_arep = NULL;
	CPSV_CKPT_DEST_INFO *ckpt_dest_add = NULL;
	CPSV_CKPT_DEST_INFO *ckpt_dest_del = NULL;
	CPSV_CKPT_DEST_INFO *ckpt_dest_down = NULL;
	CPD_A2S_CKPT_USR_INFO *ckpt_usr_info = NULL;
	uns32 evt_type, rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	cpd_msg = m_MMGR_ALLOC_CPD_MBCSV_MSG;
	if (cpd_msg == NULL) {
		m_LOG_CPD_MEMFAIL(CPD_MBCSV_MSG_ALLOC_FAILED);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	memset(cpd_msg, 0, sizeof(CPD_MBCSV_MSG));

	/* To store the value of Async Update received */
	cb->cpd_sync_cnt++;

	  TRACE("DEC ASYNC UPDATE %d",cb->sync_upd_cnt); 

	/* in the decode.i_uba , the 1st parameter is the Type , so first decode only the first field and based on the type then decode the entire message */

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
	evt_type = ncs_decode_8bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

	switch (evt_type) {
	case CPD_A2S_MSG_CKPT_CREATE:
		ckpt_create = m_MMGR_ALLOC_CPD_A2S_CKPT_CREATE;
		/* The contents are decoded from i_uba to cpd_msg */
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_CREATE), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_create, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			if (ckpt_create->dest_list)
				m_MMGR_FREE_CPSV_SYS_MEMORY(ckpt_create->dest_list);
			m_MMGR_FREE_CPD_A2S_CKPT_CREATE(ckpt_create);
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_CREATE_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);

			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.ckpt_create = *ckpt_create;
		rc = cpd_process_sb_msg(cb, cpd_msg);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(CPD_STANDBY_CREATE_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			goto end;
		}
		if (ckpt_create->dest_list)
			m_MMGR_FREE_CPSV_SYS_MEMORY(ckpt_create->dest_list);
		m_MMGR_FREE_CPD_A2S_CKPT_CREATE(ckpt_create);
		break;

	case CPD_A2S_MSG_CKPT_UNLINK:
		ckpt_unlink = &cpd_msg->info.ckpt_ulink;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_UNLINK), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_unlink, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_UNLINK_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.ckpt_ulink = *ckpt_unlink;
		rc = cpd_process_sb_msg(cb, cpd_msg);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(CPD_STANDBY_UNLINK_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			goto end;
		}

		break;

	case CPD_A2S_MSG_CKPT_RDSET:
		ckpt_rdset = &cpd_msg->info.rd_set;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_RDSET), &arg->info.decode.i_uba, EDP_OP_TYPE_DEC,
				    &ckpt_rdset, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_RDSET_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.rd_set = *ckpt_rdset;

		rc = cpd_process_sb_msg(cb, cpd_msg);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(CPD_STANDBY_RDSET_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			goto end;
		}
		break;

	case CPD_A2S_MSG_CKPT_AREP_SET:
		ckpt_arep = &cpd_msg->info.arep_set;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_arep, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_AREP_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.arep_set = *ckpt_arep;
		rc = cpd_process_sb_msg(cb, cpd_msg);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(CPD_STANDBY_AREP_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			goto end;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_ADD:
		ckpt_dest_add = &cpd_msg->info.dest_add;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_dest_add, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_DESTADD_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.dest_add = *ckpt_dest_add;
		rc = cpd_process_sb_msg(cb, cpd_msg);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(CPD_STANDBY_DESTADD_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			goto end;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_DEL:
		ckpt_dest_del = &cpd_msg->info.dest_del;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_dest_del, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_DESTDEL_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.dest_del = *ckpt_dest_del;
		rc = cpd_process_sb_msg(cb, cpd_msg);

		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(CPD_STANDBY_DESTDEL_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			goto end;
		}
		break;

	case CPD_A2S_MSG_CKPT_USR_INFO:
		ckpt_usr_info = &cpd_msg->info.usr_info;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_USR_INFO), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_usr_info, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_DESTDEL_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.usr_info = *ckpt_usr_info;
		rc = cpd_process_sb_msg(cb, cpd_msg);

		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(CPD_STANDBY_DESTDEL_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			goto end;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_DOWN:
		ckpt_dest_down = &cpd_msg->info.dest_down;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_dest_down, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_DESTDEL_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.dest_down = *ckpt_dest_down;
		rc = cpd_process_sb_msg(cb, cpd_msg);

		if (rc != NCSCC_RC_SUCCESS) {
			/* LOG */
			goto end;
		}
		break;

	default:
		return NCSCC_RC_FAILURE;
		break;
	}

 end:
	m_MMGR_FREE_CPD_MBCSV_MSG(cpd_msg);
	return rc;

}

/************************************************************************************
 * Name           : cpd_mbcsv_dec_sync_resp 
 *
 * Description    : Decode the message at Standby for cold sync and update the database

 * Arguments      : NCS_MBCSV_CB_ARG - MBCSv callback argument
 *
 * Return Values  : Success / Error
*************************************************************************************/
uns32 cpd_mbcsv_dec_sync_resp(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uns8 *ptr, num_of_ckpts, data[16];
	CPD_MBCSV_MSG mbcsv_msg;
	uns32 count = 0, rc = NCSCC_RC_SUCCESS, num_of_async_upd;
	CPD_A2S_CKPT_CREATE *ckpt_data;
	EDU_ERR ederror = 0;
	uns32 proc_rc = NCSCC_RC_SUCCESS;

	if (arg->info.decode.i_uba.ub == NULL) {	/* There is no data */
		return NCSCC_RC_SUCCESS;
	}

	ckpt_data = m_MMGR_ALLOC_CPD_A2S_CKPT_CREATE;
	if (ckpt_data == NULL) {
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	memset(ckpt_data, 0, sizeof(CPD_A2S_CKPT_CREATE));
	memset(&mbcsv_msg, 0, sizeof(CPD_MBCSV_MSG));

	/* 1. Decode the 1st uns8 region ,  we will get the num of ckpts */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns8));
	num_of_ckpts = ncs_decode_8bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns8));

	/* Decode the data */

	while (count < num_of_ckpts) {
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_CREATE), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_data, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			if (ckpt_data->dest_list)
				m_MMGR_FREE_CPSV_SYS_MEMORY(ckpt_data->dest_list);
			m_MMGR_FREE_CPD_A2S_CKPT_CREATE(ckpt_data);
			m_LOG_CPD_CL(EDU_EXEC_ASYNC_CREATE_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			return rc;
		}
		mbcsv_msg.info.ckpt_create = *ckpt_data;
		proc_rc = cpd_sb_proc_ckpt_create(cb, &mbcsv_msg);
		if (proc_rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(CPD_STANDBY_CREATE_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		}
		count++;
		if (ckpt_data->dest_list)
			m_MMGR_FREE_CPSV_SYS_MEMORY(ckpt_data->dest_list);

		memset(ckpt_data, 0, sizeof(CPD_A2S_CKPT_CREATE));
		memset(&mbcsv_msg, 0, sizeof(CPD_MBCSV_MSG));
	}

	/* Get the async update count */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uns32));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uns32));
	cb->cpd_sync_cnt = num_of_async_upd;

	/*  if(ckpt_data->dest_list)
	   m_MMGR_FREE_CPSV_SYS_MEMORY(ckpt_data->dest_list); */

	m_MMGR_FREE_CPD_A2S_CKPT_CREATE(ckpt_data);

	return rc;
}

/*********************************************************************************
 * Name          : cpd_mbcsv_dec_warm_sync_resp
 *
 * Description   : To decode the message at the warm sync at the standby

 * Arguments     : NCS_MBCSV_CB_ARG - MBCSv callback argument
 * 
 * Return Values : Success / Error
*********************************************************************************/
uns32 cpd_mbcsv_dec_warm_sync_resp(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uns32 num_of_async_upd, rc = NCSCC_RC_SUCCESS;
	uns8 data[16], *ptr;
	NCS_MBCSV_ARG ncs_arg;

	memset(&ncs_arg, '\0', sizeof(NCS_MBCSV_ARG));

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(int32));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(int32));

	if (cb->cpd_sync_cnt == num_of_async_upd) {
		return rc;
	} else {
		cb->cold_or_warm_sync_on = TRUE;

		m_LOG_CPD_LLCL(CPD_MBCSV_WARM_SYNC_COUNT_MISMATCH, CPD_FC_MBCSV, NCSFL_SEV_ERROR, cb->cpd_sync_cnt,
			       num_of_async_upd, __FILE__, __LINE__);
		/* cpd_cb_db_destroy(cb); */
		cpd_ckpt_tree_node_destroy(cb);
		ncs_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
		ncs_arg.i_mbcsv_hdl = cb->mbcsv_handle;
		ncs_arg.info.send_data_req.i_ckpt_hdl = cb->o_ckpt_hdl;
		rc = ncs_mbcsv_svc(&ncs_arg);
		if (rc != NCSCC_RC_SUCCESS) {
			/* Log */
			return rc;
		}
	}
	return rc;
}

/****************************************************************************************
 * Name          : cpd_mbcsv_decode_proc
 *
 * Description   : To invoke the various callbacks - ASYNC UPDATE , COLD SYNC , WARM SYNC at the standby

 * Arguments     : NCS_MBCSV_CB_ARG - MBCSv callback argument
 *
 * Return Values : Success / Error
*****************************************************************************************/
uns32 cpd_mbcsv_decode_proc(NCS_MBCSV_CB_ARG *arg)
{
	CPD_CB *cb;
	uns16 msg_fmt_version;
	uns32 status;
	m_CPD_RETRIEVE_CB(cb);	/* finally give up the handle */
	if (cb == NULL) {
		m_LOG_CPD_HEADLINE(CPD_DESTROY_FAIL, NCSFL_SEV_ERROR);
		return (NCSCC_RC_FAILURE);
	}

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.decode.i_peer_version,
					      CPSV_CPD_MBCSV_VERSION, CPSV_CPD_MBCSV_VERSION_MIN);

	if (msg_fmt_version) {
		switch (arg->info.decode.i_msg_type) {

		case NCS_MBCSV_MSG_ASYNC_UPDATE:
			cpd_mbcsv_dec_async_update(cb, arg);
			break;

		case NCS_MBCSV_MSG_COLD_SYNC_REQ:
			cb->cold_or_warm_sync_on = TRUE;
			TRACE("Cold sync started");
			break;

		case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
			cb->cold_or_warm_sync_on = TRUE;
			status = cpd_mbcsv_dec_sync_resp(cb, arg);
			if (arg->info.decode.i_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE) {
				if (status == NCSCC_RC_SUCCESS) {
					cb->cold_or_warm_sync_on = FALSE;
					TRACE("Cold sync completed");
				}

				ncshm_give_hdl(cb->cpd_hdl);
				return NCSCC_RC_SUCCESS;
			}
			break;

		case NCS_MBCSV_MSG_WARM_SYNC_REQ:
			break;

		case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
			status = cpd_mbcsv_dec_warm_sync_resp(cb, arg);
			break;

		case NCS_MBCSV_MSG_DATA_REQ:
			break;

		case NCS_MBCSV_MSG_DATA_RESP:
		case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
			status = cpd_mbcsv_dec_sync_resp(cb, arg);
			if (NCS_MBCSV_MSG_DATA_RESP_COMPLETE == arg->info.decode.i_msg_type) {
				if (status == NCSCC_RC_SUCCESS) {
					cb->cold_or_warm_sync_on = FALSE;
					TRACE("Warm sync completed");
				}
			}
			break;
		default:
			ncshm_give_hdl(cb->cpd_hdl);
			return NCSCC_RC_FAILURE;
		}
		ncshm_give_hdl(cb->cpd_hdl);
		return NCSCC_RC_SUCCESS;
	} else {
		/* Drop The Message */
		ncshm_give_hdl(cb->cpd_hdl);
		return NCSCC_RC_FAILURE;
	}

}
