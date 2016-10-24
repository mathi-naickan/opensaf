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
extern uint32_t cpsv_encode_extended_name(NCS_UBAID *uba, SaNameT *name);
extern uint32_t cpsv_decode_extended_name(NCS_UBAID *uba, SaNameT *name);

static void cpd_mbcsv_enc_a2s_usr_info_2(NCS_UBAID *io_uba, CPD_A2S_CKPT_USR_INFO_2 *usr_info);
static void cpd_mbcsv_dec_a2s_usr_info_2(NCS_UBAID *io_uba, CPD_A2S_CKPT_USR_INFO_2 *usr_info, NCS_MBCSV_CB_ARG *arg);


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
uint32_t cpd_mbcsv_async_update(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = SA_AIS_OK;

	TRACE_ENTER();
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
		LOG_ER("cpd mbcsv async update send failed");
		TRACE_LEAVE();
		return rc;
	}
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_register(CPD_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
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

	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_init(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER();
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	arg.info.initialize.i_mbcsv_cb = cpd_mbcsv_callback;
	arg.info.initialize.i_version = CPSV_CPD_MBCSV_VERSION;
	arg.info.initialize.i_service = NCS_SERVICE_ID_CPD;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("cpd mbcsv init failed ");
		rc = NCSCC_RC_FAILURE;
		TRACE_LEAVE();
		return rc;
	}

	cb->mbcsv_handle = arg.info.initialize.o_mbcsv_hdl;
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_open(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	
	TRACE_ENTER();
	arg.i_op = NCS_MBCSV_OP_OPEN;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	arg.info.open.i_pwe_hdl = (uint32_t)cb->mds_handle;
	arg.info.open.i_client_hdl = cb->cpd_hdl;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("cpd mbcsv open failed");
		rc = NCSCC_RC_FAILURE;
		TRACE_LEAVE();
		return rc;
	}
	cb->o_ckpt_hdl = arg.info.open.o_ckpt_hdl;
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_selobj_get(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;

	if (ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpd mbcsv get sel obj failure ");
		rc = NCSCC_RC_FAILURE;
		TRACE_LEAVE();
		return rc;
	}
	cb->mbcsv_sel_obj = arg.info.sel_obj_get.o_select_obj;
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_chgrole(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	arg.info.chg_role.i_ckpt_hdl = cb->o_ckpt_hdl;
	arg.info.chg_role.i_ha_state = cb->ha_state;	/*  ha_state is assigned at the time of amf_init where csi_set_callback will assign the state */

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("cpd mbcsv chgrole failed ");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_close(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	TRACE_ENTER();
	arg.i_op = NCS_MBCSV_OP_CLOSE;
	arg.info.close.i_ckpt_hdl = cb->o_ckpt_hdl;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		TRACE_4("cpd mbcsv close failed"); 
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_finalize(CPD_CB *cb)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	arg.i_op = NCS_MBCSV_OP_FINALIZE;
	arg.i_mbcsv_hdl = cb->mbcsv_handle;
	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		TRACE_4("cpd mbcsv finalize failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_callback(NCS_MBCSV_CB_ARG *arg)
{

	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	if (arg == NULL) {
		rc = NCSCC_RC_FAILURE;
		LOG_ER("cpd mbcsv callback failed");
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
	TRACE_1("cpd mbcsv callback success ");
	TRACE_ENTER();
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
uint32_t cpd_mbcsv_enc_async_update(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	CPD_MBCSV_MSG *cpd_msg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	uint8_t *cpd_type_ptr;

	TRACE_ENTER();
	/*  Increment the async update count cb->cpd_sync_cnt     */
	cb->cpd_sync_cnt++;

	  TRACE("ENC ASYNC COUNT %d",cb->cpd_sync_cnt);  

	cpd_type_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uint8_t));
	if (cpd_type_ptr == NULL) {
		LOG_ER("ncs enc reserve space failed ");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uint8_t));
	ncs_encode_8bit(&cpd_type_ptr, arg->info.encode.io_reo_type);

	switch (arg->info.encode.io_reo_type) {

	case CPD_A2S_MSG_CKPT_CREATE:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_CREATE), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.ckpt_create, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async create failed");
			rc = NCSCC_RC_FAILURE;
		}

		cpsv_encode_extended_name(&arg->info.encode.io_uba, &cpd_msg->info.ckpt_create.ckpt_name); 

		break;

	case CPD_A2S_MSG_CKPT_UNLINK:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_UNLINK), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.ckpt_ulink, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async unlink failed");
			rc = NCSCC_RC_FAILURE;
		}

		cpsv_encode_extended_name(&arg->info.encode.io_uba, &cpd_msg->info.ckpt_ulink.ckpt_name); 

		break;

	case CPD_A2S_MSG_CKPT_RDSET:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_RDSET), &arg->info.encode.io_uba, EDP_OP_TYPE_ENC,
				    &cpd_msg->info.rd_set, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async rdset failed");
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_AREP_SET:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.arep_set, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async arep failed");
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_ADD:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.dest_add, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async destadd failed ");
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_DEL:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.dest_del, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec dest del encode failed ");
			rc = NCSCC_RC_FAILURE;
		}
		break;

	case CPD_A2S_MSG_CKPT_USR_INFO:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		cpd_mbcsv_enc_a2s_usr_info_2(&arg->info.encode.io_uba, &cpd_msg->info.usr_info_2);
		break;

	case CPD_A2S_MSG_CKPT_DEST_DOWN:
		cpd_msg = (CPD_MBCSV_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.encode.io_uba,
				    EDP_OP_TYPE_ENC, &cpd_msg->info.dest_down, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd A2S msg ckpt dest down encode failed");
			rc = NCSCC_RC_FAILURE;
		}
		break;

	default:
		rc= NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
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

uint32_t cpd_mbcsv_enc_msg_resp(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	CPD_CKPT_INFO_NODE *ckpt_node;
	CPD_A2S_CKPT_CREATE ckpt_create;
	SaCkptCheckpointHandleT prev_ckpt_id = 0;
	uint32_t rc = NCSCC_RC_SUCCESS, i;
	CPD_NODE_REF_INFO *nref_info;
	EDU_ERR ederror = 0;
	uint8_t *header, num_ckpt = 0, *sync_cnt_ptr;
	bool flag = false;

	TRACE_ENTER();
	/* COLD_SYNC_RESP IS DONE BY THE ACTIVE */
	if (cb->ha_state == SA_AMF_HA_STANDBY) {
		rc = NCSCC_RC_FAILURE;	
		TRACE_LEAVE();
		return rc;
	}
	/*  cb->prev_ckpt_id = 0; */
	prev_ckpt_id = cb->prev_ckpt_id;

	ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(&cb->ckpt_tree, (uint8_t *)&prev_ckpt_id);
	if (ckpt_node == NULL) {
		rc = NCSCC_RC_SUCCESS;
		arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
		cb->cold_or_warm_sync_on = false;
	}

	/* First reserve space to store the number of checkpoints that will be sent */

	header = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uint8_t));
	if (header == NULL) {
		TRACE("ncs_enc reserve space failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uint8_t));

	while (ckpt_node) {

		if (ckpt_node->is_unlink_set == true ) {
			TRACE_1("cpd mbcsv enc continue  for ckpt_id %llx",ckpt_node->ckpt_id);
			prev_ckpt_id = ckpt_node->ckpt_id;
			ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(&cb->ckpt_tree, (uint8_t *)&prev_ckpt_id);
			continue;
		}
		
		nref_info = ckpt_node->node_list;

		/* Populate the A2S_CKPT_CREATE structure */
		osaf_extended_name_lend(ckpt_node->ckpt_name, &ckpt_create.ckpt_name);
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
				LOG_ER("cpd cpnd dest info memory allocation failed");
				rc = NCSCC_RC_OUT_OF_MEM;
				TRACE_LEAVE();
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
			TRACE_4("edu exec async create failed");
			m_MMGR_FREE_CPSV_CPND_DEST_INFO(ckpt_create.dest_list);
			TRACE_LEAVE();
			return rc;
		}
		
		cpsv_encode_extended_name(&arg->info.encode.io_uba, &ckpt_create.ckpt_name);

		if (ckpt_create.dest_list)
			m_MMGR_FREE_CPSV_CPND_DEST_INFO(ckpt_create.dest_list);

		prev_ckpt_id = ckpt_node->ckpt_id;
		if (num_ckpt == CPD_MBCSV_MAX_MSG_CNT)
			break;

		ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(&cb->ckpt_tree, (uint8_t *)&prev_ckpt_id);

	}			/* End of While */

	cb->prev_ckpt_id = prev_ckpt_id;	/* To preserve the next ckpt_id for cold sync (after 50 to hold 51st) */
	ncs_encode_8bit(&header, num_ckpt);

	/* This will have the count of async updates that have been sent , this will be 0 initially */
	sync_cnt_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uint32_t));
	if (sync_cnt_ptr == NULL) {
		LOG_ER("ncs_enc_reserve space_failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uint32_t));
	ncs_encode_32bit(&sync_cnt_ptr, cb->cpd_sync_cnt);

	ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(&cb->ckpt_tree, (uint8_t *)&prev_ckpt_id);
	if (ckpt_node == NULL) {
		flag = true;
	}
	if ((num_ckpt < CPD_MBCSV_MAX_MSG_CNT) || (flag == true)) {
		if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP) {
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
			cb->prev_ckpt_id = 0;
			cb->cold_or_warm_sync_on = false;

		} else {
			if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_DATA_RESP) {
				arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
				cb->prev_ckpt_id = 0;
				cb->cold_or_warm_sync_on = false;

			}
		}

	}
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_enc_warm_sync_resp(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint8_t *wsync_ptr;

	/* Reserve space to send the async update counter */
	wsync_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uint32_t));
	if (wsync_ptr == NULL) {
		LOG_ER("cpd enc reserve space failed for mbcsv_enc_warm_sync_resp");
		return NCSCC_RC_FAILURE;
	}

	/* SEND THE ASYNC UPDATE COUNTER */
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uint32_t));
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
uint32_t cpd_mbcsv_encode_proc(NCS_MBCSV_CB_ARG *arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	CPD_CB *cb;
	uint16_t msg_fmt_version;

	TRACE_ENTER();
	m_CPD_RETRIEVE_CB(cb);	/* finally give up the handle */
	if (cb == NULL) {
		LOG_ER("cb retrieve failed in cpd mbcsv encode proc");
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
	TRACE_LEAVE();
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

uint32_t cpd_mbcsv_dec_async_update(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uint8_t *ptr;
	uint8_t data[4];
	CPD_MBCSV_MSG *cpd_msg;
	CPD_A2S_CKPT_CREATE *ckpt_create = NULL;
	CPD_A2S_CKPT_UNLINK *ckpt_unlink = NULL;
	CPSV_CKPT_RDSET *ckpt_rdset = NULL;
	CPSV_CKPT_DEST_INFO *ckpt_arep = NULL;
	CPSV_CKPT_DEST_INFO *ckpt_dest_add = NULL;
	CPSV_CKPT_DEST_INFO *ckpt_dest_del = NULL;
	CPSV_CKPT_DEST_INFO *ckpt_dest_down = NULL;
	uint32_t evt_type, rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;

	cpd_msg = m_MMGR_ALLOC_CPD_MBCSV_MSG;
	if (cpd_msg == NULL) {
		LOG_ER("cpd mbcsv msg memory alloc failed ");
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	memset(cpd_msg, 0, sizeof(CPD_MBCSV_MSG));

	/* To store the value of Async Update received */
	cb->cpd_sync_cnt++;

	  TRACE("DEC ASYNC UPDATE %d",cb->sync_upd_cnt); 

	/* in the decode.i_uba , the 1st parameter is the Type , so first decode only the first field and based on the type then decode the entire message */

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uint8_t));
	evt_type = ncs_decode_8bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uint8_t));


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
			TRACE_4("edu exec async create failed");

			rc = NCSCC_RC_FAILURE;
			goto end;
		}

		cpsv_decode_extended_name(&arg->info.decode.i_uba, &ckpt_create->ckpt_name);

		cpd_msg->type = evt_type;
		cpd_msg->info.ckpt_create = *ckpt_create;
		rc = cpd_process_sb_msg(cb, cpd_msg);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd standby create evt failed");
			goto end;
		}

		if (osaf_is_an_extended_name(&ckpt_create->ckpt_name))
			free((void *)osaf_extended_name_borrow(&ckpt_create->ckpt_name));

		if (ckpt_create->dest_list)
			m_MMGR_FREE_CPSV_SYS_MEMORY(ckpt_create->dest_list);
		m_MMGR_FREE_CPD_A2S_CKPT_CREATE(ckpt_create);
		break;

	case CPD_A2S_MSG_CKPT_UNLINK:
		ckpt_unlink = &cpd_msg->info.ckpt_ulink;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_UNLINK), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_unlink, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async unlink failed");
			rc = NCSCC_RC_FAILURE;
			goto end;
		}

		cpsv_decode_extended_name(&arg->info.decode.i_uba, &ckpt_unlink->ckpt_name);

		cpd_msg->type = evt_type;
		cpd_msg->info.ckpt_ulink = *ckpt_unlink;
		rc = cpd_process_sb_msg(cb, cpd_msg);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd standby unlink evt failed");
			goto end;
		}

		if (osaf_is_an_extended_name(&ckpt_unlink->ckpt_name))
			free((void *)osaf_extended_name_borrow(&ckpt_unlink->ckpt_name));

		break;

	case CPD_A2S_MSG_CKPT_RDSET:
		ckpt_rdset = &cpd_msg->info.rd_set;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_RDSET), &arg->info.decode.i_uba, EDP_OP_TYPE_DEC,
				    &ckpt_rdset, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async rdset failed"); 
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.rd_set = *ckpt_rdset;

		rc = cpd_process_sb_msg(cb, cpd_msg);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd standby rdset evt failed");
			goto end;
		}
		break;

	case CPD_A2S_MSG_CKPT_AREP_SET:
		ckpt_arep = &cpd_msg->info.arep_set;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_arep, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async arep failed ");
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.arep_set = *ckpt_arep;
		rc = cpd_process_sb_msg(cb, cpd_msg);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd standby arep evt failed ");
			goto end;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_ADD:
		ckpt_dest_add = &cpd_msg->info.dest_add;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_dest_add, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async destadd failed");
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.dest_add = *ckpt_dest_add;
		rc = cpd_process_sb_msg(cb, cpd_msg);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd standby dest add evt failed ");
			goto end;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_DEL:
		ckpt_dest_del = &cpd_msg->info.dest_del;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_dest_del, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async dest del failed ");
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.dest_del = *ckpt_dest_del;
		rc = cpd_process_sb_msg(cb, cpd_msg);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd standby destdel evt failed");
			goto end;
		}
		break;

	case CPD_A2S_MSG_CKPT_USR_INFO:
		cpd_msg->type = evt_type;
		cpd_mbcsv_dec_a2s_usr_info_2(&arg->info.decode.i_uba, &cpd_msg->info.usr_info_2, arg);
		rc = cpd_process_sb_msg(cb, cpd_msg);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd standby dest del evt failed");
			goto end;
		}
		break;

	case CPD_A2S_MSG_CKPT_DEST_DOWN:
		ckpt_dest_down = &cpd_msg->info.dest_down;
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPSV_CKPT_DEST_INFO), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_dest_down, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("edu exec async destdel failed");
			rc = NCSCC_RC_FAILURE;
			goto end;
		}
		cpd_msg->type = evt_type;
		cpd_msg->info.dest_down = *ckpt_dest_down;
		rc = cpd_process_sb_msg(cb, cpd_msg);

		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd process sb msg");
			goto end;
		}
		break;

	default:
		
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
		break;
	}

 end:
	m_MMGR_FREE_CPD_MBCSV_MSG(cpd_msg);
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_dec_sync_resp(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uint8_t *ptr, num_of_ckpts, data[16];
	CPD_MBCSV_MSG mbcsv_msg;
	uint32_t count = 0, rc = NCSCC_RC_SUCCESS, num_of_async_upd;
	CPD_A2S_CKPT_CREATE *ckpt_data;
	EDU_ERR ederror = 0;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (arg->info.decode.i_uba.ub == NULL) {	/* There is no data */
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}

	ckpt_data = m_MMGR_ALLOC_CPD_A2S_CKPT_CREATE;
	if (ckpt_data == NULL) {
		rc = NCSCC_RC_FAILURE;
		TRACE_LEAVE();
		return rc;
	}
	memset(ckpt_data, 0, sizeof(CPD_A2S_CKPT_CREATE));
	memset(&mbcsv_msg, 0, sizeof(CPD_MBCSV_MSG));

	/* 1. Decode the 1st uint8_t region ,  we will get the num of ckpts */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uint8_t));
	num_of_ckpts = ncs_decode_8bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uint8_t));

	/* Decode the data */

	while (count < num_of_ckpts) {
		rc = m_NCS_EDU_EXEC(&cb->edu_hdl, FUNC_NAME(CPD_A2S_CKPT_CREATE), &arg->info.decode.i_uba,
				    EDP_OP_TYPE_DEC, &ckpt_data, &ederror);
		if (rc != NCSCC_RC_SUCCESS) {
			if (ckpt_data->dest_list)
				m_MMGR_FREE_CPSV_SYS_MEMORY(ckpt_data->dest_list);
			m_MMGR_FREE_CPD_A2S_CKPT_CREATE(ckpt_data);
			TRACE_4("edu exec async create failed");
			TRACE_LEAVE();
			return rc;
		}

		cpsv_decode_extended_name(&arg->info.decode.i_uba, &ckpt_data->ckpt_name);

		mbcsv_msg.info.ckpt_create = *ckpt_data;
		proc_rc = cpd_sb_proc_ckpt_create(cb, &mbcsv_msg);
		if (proc_rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd standby create evt failed");
		}
		count++;
		if (ckpt_data->dest_list)
			m_MMGR_FREE_CPSV_SYS_MEMORY(ckpt_data->dest_list);

		if (osaf_is_an_extended_name(&ckpt_data->ckpt_name))
			free((void *)osaf_extended_name_borrow(&ckpt_data->ckpt_name));

		memset(ckpt_data, 0, sizeof(CPD_A2S_CKPT_CREATE));
		memset(&mbcsv_msg, 0, sizeof(CPD_MBCSV_MSG));
	}

	/* Get the async update count */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(uint32_t));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uint32_t));
	cb->cpd_sync_cnt = num_of_async_upd;

	/*  if(ckpt_data->dest_list)
	   m_MMGR_FREE_CPSV_SYS_MEMORY(ckpt_data->dest_list); */

	m_MMGR_FREE_CPD_A2S_CKPT_CREATE(ckpt_data);
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_dec_warm_sync_resp(CPD_CB *cb, NCS_MBCSV_CB_ARG *arg)
{
	uint32_t num_of_async_upd, rc = NCSCC_RC_SUCCESS;
	uint8_t data[16], *ptr;
	NCS_MBCSV_ARG ncs_arg;

	TRACE_ENTER();
	memset(&ncs_arg, '\0', sizeof(NCS_MBCSV_ARG));

	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(int32_t));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(int32_t));

	if (cb->cpd_sync_cnt == num_of_async_upd) {
		TRACE_LEAVE();
		return rc;
	} else {
		cb->cold_or_warm_sync_on = true;

		TRACE_4("cpd mbcsv warm sync count mismatch cpd_sync_cnt: %u num_of_async_upd: %u", cb->cpd_sync_cnt, num_of_async_upd);
		/* cpd_cb_db_destroy(cb); */
		cpd_ckpt_tree_node_destroy(cb);
		ncs_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
		ncs_arg.i_mbcsv_hdl = cb->mbcsv_handle;
		ncs_arg.info.send_data_req.i_ckpt_hdl = cb->o_ckpt_hdl;
		rc = ncs_mbcsv_svc(&ncs_arg);
		if (rc != NCSCC_RC_SUCCESS) {
			/* Log */
			TRACE_LEAVE();
			return rc;
		}
	}
	TRACE_LEAVE();
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
uint32_t cpd_mbcsv_decode_proc(NCS_MBCSV_CB_ARG *arg)
{
	CPD_CB *cb;
	uint16_t msg_fmt_version;
	uint32_t status;

	TRACE_ENTER();
	m_CPD_RETRIEVE_CB(cb);	/* finally give up the handle */
	if (cb == NULL) {
		LOG_ER("cpd cb retrieve failed ");
		TRACE_LEAVE();
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
			cb->cold_or_warm_sync_on = true;
			TRACE("Cold sync started");
			break;

		case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
			cb->cold_or_warm_sync_on = true;
			status = cpd_mbcsv_dec_sync_resp(cb, arg);
			if (arg->info.decode.i_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE) {
				if (status == NCSCC_RC_SUCCESS) {
					cb->cold_or_warm_sync_on = false;
					TRACE("Cold sync completed");
				}

				ncshm_give_hdl(cb->cpd_hdl);
				TRACE_LEAVE();
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
					cb->cold_or_warm_sync_on = false;
					TRACE("Warm sync completed");
				}
			}
			break;
		default:
			ncshm_give_hdl(cb->cpd_hdl);
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		ncshm_give_hdl(cb->cpd_hdl);
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	} else {
		/* Drop The Message */
		ncshm_give_hdl(cb->cpd_hdl);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

}

/**********************************************************************************************
 * Name                   : cpd_mbcsv_enc_a2s_usr_info_2
 *
 * Description            : This function encodes the message CPD_A2S_MSG_CKPT_USR_INFO
 *
 * Return Values          : None
 *
 * Notes                  : None
**********************************************************************************************/
void cpd_mbcsv_enc_a2s_usr_info_2(NCS_UBAID *io_uba, CPD_A2S_CKPT_USR_INFO_2 *usr_info)
{
	TRACE_ENTER();
	osaf_encode_uint64(io_uba, usr_info->ckpt_id);
	osaf_encode_uint32(io_uba, usr_info->num_user);
	osaf_encode_uint32(io_uba, usr_info->num_writer);
	osaf_encode_uint32(io_uba, usr_info->num_reader);
	osaf_encode_uint32(io_uba, usr_info->num_sections);
	osaf_encode_uint32(io_uba, usr_info->ckpt_on_scxb1);
	osaf_encode_uint32(io_uba, usr_info->ckpt_on_scxb2);
	osaf_encode_uint32(io_uba, usr_info->node_users_cnt);

	int i = 0;
	for (i = 0; i < usr_info->node_users_cnt; i++) {
		osaf_encode_uint64(io_uba, usr_info->node_list[i].dest);
		osaf_encode_uint32(io_uba, usr_info->node_list[i].num_users);
		osaf_encode_uint32(io_uba, usr_info->node_list[i].num_writers);
		osaf_encode_uint32(io_uba, usr_info->node_list[i].num_readers);
	}

	TRACE_LEAVE();
}

/**********************************************************************************************
 * Name                   : cpd_mbcsv_dec_a2s_usr_info_2
 *
 * Description            : This function decodes the message CPD_A2S_MSG_CKPT_USR_INFO
 *
 *
 * Return Values          : None
 *
 * Notes                  : None
**********************************************************************************************/
void cpd_mbcsv_dec_a2s_usr_info_2(NCS_UBAID *io_uba, CPD_A2S_CKPT_USR_INFO_2 *usr_info, NCS_MBCSV_CB_ARG *arg)
{
	TRACE_ENTER();
	osaf_decode_uint64(io_uba, (uint64_t *) &usr_info->ckpt_id);
	osaf_decode_uint32(io_uba, &usr_info->num_user);
	osaf_decode_uint32(io_uba, &usr_info->num_writer);
	osaf_decode_uint32(io_uba, &usr_info->num_reader);
	osaf_decode_uint32(io_uba, &usr_info->num_sections);
	osaf_decode_uint32(io_uba, &usr_info->ckpt_on_scxb1);
	osaf_decode_uint32(io_uba, &usr_info->ckpt_on_scxb2);
	// Handle old message
	uint16_t msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.decode.i_peer_version,
					      CPSV_CPD_MBCSV_VERSION, CPSV_CPD_MBCSV_VERSION_MIN);
	if (msg_fmt_version < CPSV_CPD_MBCSV_VERSION_USR_INFO) {
		TRACE_LEAVE();
		return;
	}
	osaf_decode_uint32(io_uba, &usr_info->node_users_cnt);

	int i = 0;
	CPD_NODE_USER_INFO *node_list = malloc(usr_info->node_users_cnt * sizeof(CPD_NODE_USER_INFO));
	for (i = 0; i < usr_info->node_users_cnt; i++) {
		osaf_decode_uint64(io_uba, &node_list[i].dest);
		osaf_decode_uint32(io_uba, &node_list[i].num_users);
		osaf_decode_uint32(io_uba, &node_list[i].num_writers);
		osaf_decode_uint32(io_uba, &node_list[i].num_readers);
	}

	usr_info->node_list = node_list;

	TRACE_LEAVE();
}
