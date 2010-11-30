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

  This file contains utility routines for managing encode/decode operations
  in AvSv components.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "mqsv.h"

#include "mqsv_edu.h"

static uns32 mqsv_edp_mqp_req(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uns32 *ptr_data_len,
			      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_mqp_rsp(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uns32 *ptr_data_len,
			      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_mqa_callback(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uns32 *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_asapi_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uns32 *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_send_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uns32 *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_mqp_open_req(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uns32 *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_message_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uns32 *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_samsgqueuecreationattributest(EDU_HDL *hdl, EDU_TKN *edu_tkn,
						    NCSCONTEXT ptr, uns32 *ptr_data_len,
						    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_queue_usage(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				  NCSCONTEXT ptr, uns32 *ptr_data_len,
				  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_queue_status(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uns32 *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_mqd_ctrl_msg(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uns32 *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_edp_mqnd_ctrl_msg(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				    NCSCONTEXT ptr, uns32 *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uns32 mqsv_qgrp_cnt_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uns32 *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static int mqsv_evt_test_type_fnc(NCSCONTEXT arg);
static int mqsv_mqp_req_test_type_fnc(NCSCONTEXT arg);
static int mqsv_mqp_rsp_test_type_fnc(NCSCONTEXT arg);
static int mqsv_mqa_callback_test_type_fnc(NCSCONTEXT arg);
static int mqsv_edp_mqnd_ctrl_msg_test_type_fnc(NCSCONTEXT arg);
static int mqsv_edp_mqd_ctrl_msg_test_type_fnc(NCSCONTEXT arg);

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_send_info

  DESCRIPTION:      EDU program handler for "MQSV_SEND_INFO" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "MQSV_SEND_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_send_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uns32 *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQSV_SEND_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET mqsv_send_info_rules[] = {
		{EDU_START, mqsv_edp_send_info, 0, 0, 0,
		 sizeof(MQSV_SEND_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQSV_SEND_INFO *)0)->to_svc, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0,
		 (long)&((MQSV_SEND_INFO *)0)->dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQSV_SEND_INFO *)0)->stype, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, 0, 0, 0,
		 (long)&((MQSV_SEND_INFO *)0)->ctxt.length, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, EDQ_ARRAY, 0, 0,
		 (long)&((MQSV_SEND_INFO *)0)->ctxt.data, MDS_SYNC_SND_CTXT_LEN_MAX, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQSV_SEND_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQSV_SEND_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;

		}
		memset(*d_ptr, '\0', sizeof(MQSV_SEND_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_send_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqp_open_req

  DESCRIPTION:      EDU program handler for "MQSV_SEND_INFO" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "MQSV_SEND_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_mqp_open_req(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uns32 *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQP_OPEN_REQ *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET mqsv_edp_mqp_open_req_rules[] = {
		{EDU_START, mqsv_edp_mqp_open_req, 0, 0, 0,
		 sizeof(MQP_OPEN_REQ), 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_OPEN_REQ *)0)->msgHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((MQP_OPEN_REQ *)0)->queueName, 0, NULL},
		{EDU_EXEC, mqsv_edp_samsgqueuecreationattributest, 0, 0, 0,
		 (long)&((MQP_OPEN_REQ *)0)->creationAttributes, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGOPENFLAGST, 0, 0, 0,
		 (long)&((MQP_OPEN_REQ *)0)->openFlags, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQP_OPEN_REQ *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQP_OPEN_REQ **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;

		}
		memset(*d_ptr, '\0', sizeof(MQP_OPEN_REQ));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqp_open_req_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_evt_test_type_fnc

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "mqsv_edp_nda_ava_msg".

  RETURNS:          uns32, denoting the offset of the next-instruction 
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "mqsv_edp_mqsv_evt").

*****************************************************************************/
static int mqsv_evt_test_type_fnc(NCSCONTEXT arg)
{
	typedef enum {
		LCL_TEST_JUMP_OFFSET_MQSV_MQP_REQ = 1,
		LCL_TEST_JUMP_OFFSET_MQSV_MQP_RSP,
		LCL_TEST_JUMP_OFFSET_MQSV_MQA_CALLBACK,
		LCL_TEST_JUMP_OFFSET_MQSV_ASAPI_MSG,
		LCL_TEST_JUMP_OFFSET_MQSV_EVT_MQD_CTRL,
		LCL_TEST_JUMP_OFFSET_MQSV_EVT_MQND_CTRL,
	} LCL_TEST_JUMP_OFFSET;
	MQSV_EVT_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(MQSV_EVT_TYPE *)arg;

	switch (type) {
	case MQSV_EVT_MQP_REQ:
		return LCL_TEST_JUMP_OFFSET_MQSV_MQP_REQ;
	case MQSV_EVT_MQP_RSP:
		return LCL_TEST_JUMP_OFFSET_MQSV_MQP_RSP;
	case MQSV_EVT_MQA_CALLBACK:
		return LCL_TEST_JUMP_OFFSET_MQSV_MQA_CALLBACK;
	case MQSV_EVT_ASAPI:
		return LCL_TEST_JUMP_OFFSET_MQSV_ASAPI_MSG;
	case MQSV_EVT_MQD_CTRL:
		return LCL_TEST_JUMP_OFFSET_MQSV_EVT_MQD_CTRL;
	case MQSV_EVT_MQND_CTRL:
		return LCL_TEST_JUMP_OFFSET_MQSV_EVT_MQND_CTRL;
	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqsv_evt

  DESCRIPTION:      EDU program handler for "MQSV_EVT" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "MQSV_EVT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_asapi_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uns32 *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (EDP_OP_TYPE_ENC == op) {
		asapi_msg_enc((ASAPi_MSG_INFO *)ptr, buf_env->info.uba);
	} else if (op == EDP_OP_TYPE_DEC) {
		rc = asapi_msg_dec(buf_env->info.uba, (ASAPi_MSG_INFO **)ptr);
	}

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqsv_evt

  DESCRIPTION:      EDU program handler for "MQSV_EVT" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "MQSV_EVT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 mqsv_edp_mqsv_evt(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQSV_EVT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET mqsv_edp_mqsv_evt_rules[] = {
		{EDU_START, mqsv_edp_mqsv_evt, 0, 0, 0,
		 sizeof(MQSV_EVT), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQSV_EVT *)0)->type, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQSV_EVT *)0)->type, 0,
		 mqsv_evt_test_type_fnc},
		{EDU_EXEC, mqsv_edp_mqp_req, 0, 0, EDU_EXIT,
		 (long)&((MQSV_EVT *)0)->msg.mqp_req, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqp_rsp, 0, 0, EDU_EXIT,
		 (long)&((MQSV_EVT *)0)->msg.mqp_rsp, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqa_callback, 0, 0, EDU_EXIT,
		 (long)&((MQSV_EVT *)0)->msg.mqp_async_rsp, 0, NULL},
		{EDU_EXEC, mqsv_edp_asapi_info, EDQ_POINTER, 0, EDU_EXIT,
		 (long)&((MQSV_EVT *)0)->msg.asapi, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_ctrl_msg, 0, 0, EDU_EXIT,
		 (long)&((MQSV_EVT *)0)->msg.mqd_ctrl, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqnd_ctrl_msg, 0, 0, EDU_EXIT,
		 (long)&((MQSV_EVT *)0)->msg.mqnd_ctrl, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQSV_EVT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQSV_EVT **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;

		}
		memset(*d_ptr, '\0', sizeof(MQSV_EVT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqsv_evt_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_test_sendreceive

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "mqsv_edp_message_info".

  RETURNS:          uns32, denoting the offset of the next-instruction 
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "mqsv_edp_message_info").

*****************************************************************************/

static int mqsv_edp_test_sendreceive(NCSCONTEXT arg)
{
	typedef enum {
		LCL_TEST_JUMP_OFFSET_MQSV_SENDERID = 1,
		LCL_TEST_JUMP_OFFSET_MQSV_SENDERCONTEXT,
	} LCL_TEST_JUMP_OFFSET;
	SaBoolT type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(SaBoolT *)arg;

	switch (type) {
	case SA_FALSE:
		return LCL_TEST_JUMP_OFFSET_MQSV_SENDERID;
	case SA_TRUE:
		return LCL_TEST_JUMP_OFFSET_MQSV_SENDERCONTEXT;
	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_message_info

  DESCRIPTION:      EDU program handler for "QUEUE_MESSAGE_INFO" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "QUEUE_MESSAGE_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/

static uns32 mqsv_edp_message_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uns32 *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	QUEUE_MESSAGE_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET mqsv_message_info_rules[] = {
		{EDU_START, mqsv_edp_message_info, 0, 0, 0,
		 sizeof(QUEUE_MESSAGE_INFO), 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SASIZET, 0, 0, 0,
		 (long)&((QUEUE_MESSAGE_INFO *)0)->sendTime, 0, NULL},
		{EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0,
		 (long)&((QUEUE_MESSAGE_INFO *)0)->sendReceive, 0, NULL},
		{EDU_TEST, ncs_edp_ncs_bool, 0, 0, 0,
		 (long)&((QUEUE_MESSAGE_INFO *)0)->sendReceive, 0, mqsv_edp_test_sendreceive},
		{EDU_EXEC, m_NCS_EDP_SAMSGSENDERIDT, 0, 0, EDU_EXIT,
		 (long)&((QUEUE_MESSAGE_INFO *)0)->sender.senderId, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, 0, 0, 0,
		 (long)&((QUEUE_MESSAGE_INFO *)0)->sender.sender_context.context.length, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, EDQ_ARRAY, 0, 0,
		 (long)&((QUEUE_MESSAGE_INFO *)0)->sender.sender_context.context.data, MDS_SYNC_SND_CTXT_LEN_MAX, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0,
		 (long)&((QUEUE_MESSAGE_INFO *)0)->sender.sender_context.sender_dest, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SASIZET, 0, 0, 0,
		 (long)&((QUEUE_MESSAGE_INFO *)0)->sender.sender_context.reply_buffer_size, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (QUEUE_MESSAGE_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (QUEUE_MESSAGE_INFO **)ptr;
		if (*d_ptr == NULL) {

			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;

		}
		memset(*d_ptr, '\0', sizeof(QUEUE_MESSAGE_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_message_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;

}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_mqp_req_test_type_fnc

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "mqsv_edp_nda_ava_msg".

  RETURNS:          uns32, denoting the offset of the next-instruction 
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "mqsv_edp_mqp_req").

*****************************************************************************/
static int mqsv_mqp_req_test_type_fnc(NCSCONTEXT arg)
{
	typedef enum {
		LCL_TEST_JUMP_OFFSET_MQP_EVT_INIT_REQ = 1,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_FINALIZE_REQ,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_OPEN_REQ,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_OPEN_ASYNC_REQ = 8,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_CLOSE_REQ = 13,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_STATUS_REQ = 14,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_UNLINK_REQ = 17,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_REPLY_MSG = 20,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_REPLY_MSG_ASYNC = 30,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_SEND_MSG = 41,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_TRANSFER_QUEUE_REQ = 53,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_TRANSFER_QUEUE_COMPLETE = 60,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_UPDATE_STATS = 62,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_RET_TIME_SET_REQ = 65,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_CLM_NOTIFY = 67
	} LCL_TEST_JUMP_OFFSET;
	MQP_REQ_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(MQP_REQ_TYPE *)arg;

	switch (type) {
	case MQP_EVT_INIT_REQ:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_INIT_REQ;
	case MQP_EVT_FINALIZE_REQ:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_FINALIZE_REQ;
	case MQP_EVT_OPEN_REQ:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_OPEN_REQ;
	case MQP_EVT_OPEN_ASYNC_REQ:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_OPEN_ASYNC_REQ;
	case MQP_EVT_CLOSE_REQ:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_CLOSE_REQ;
	case MQP_EVT_STATUS_REQ:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_STATUS_REQ;
	case MQP_EVT_UNLINK_REQ:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_UNLINK_REQ;
	case MQP_EVT_REPLY_MSG:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_REPLY_MSG;
	case MQP_EVT_REPLY_MSG_ASYNC:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_REPLY_MSG_ASYNC;
	case MQP_EVT_SEND_MSG:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_SEND_MSG;
	case MQP_EVT_TRANSFER_QUEUE_REQ:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_TRANSFER_QUEUE_REQ;
	case MQP_EVT_TRANSFER_QUEUE_COMPLETE:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_TRANSFER_QUEUE_COMPLETE;
	case MQP_EVT_STAT_UPD_REQ:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_UPDATE_STATS;
	case MQP_EVT_Q_RET_TIME_SET_REQ:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_RET_TIME_SET_REQ;
	case MQP_EVT_CLM_NOTIFY:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_CLM_NOTIFY;

	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_samsgqueuecreationflagst

  DESCRIPTION:      EDU program handler for "SaMsgQueueCreationAttributesT" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on SaMsgQueueCreationAttributesT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_samsgqueuecreationattributest(EDU_HDL *hdl, EDU_TKN *edu_tkn,
						    NCSCONTEXT ptr, uns32 *ptr_data_len,
						    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaMsgQueueCreationAttributesT *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_samsgqueuecreationattributes_rules[] = {
		{EDU_START, mqsv_edp_samsgqueuecreationattributest, 0, 0, 0,
		 sizeof(SaMsgQueueCreationAttributesT), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((SaMsgQueueCreationAttributesT *)0)->creationFlags, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SASIZET, EDQ_ARRAY, 0, 0,
		 (long)&((SaMsgQueueCreationAttributesT *)0)->size,
		 SA_MSG_MESSAGE_LOWEST_PRIORITY + 1, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0,
		 (long)&((SaMsgQueueCreationAttributesT *)0)->retentionTime, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (SaMsgQueueCreationAttributesT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (SaMsgQueueCreationAttributesT **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(SaMsgQueueCreationAttributesT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_samsgqueuecreationattributes_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqp_req

  DESCRIPTION:      EDU program handler for "MQP_REQ_MSG" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "MQP_REQ_MSG" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_mqp_req(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQP_REQ_MSG *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET mqsv_mqp_req_rules[] = {
		{EDU_START, mqsv_edp_mqp_req, 0, 0, 0,
		 sizeof(MQP_REQ_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->type, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->agent_mds_dest, 0, NULL},

		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->type, 0,
		 mqsv_mqp_req_test_type_fnc},

		/* Initialize */
		{EDU_EXEC, ncs_edp_saversiont, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.initReq.version, 0, NULL},

		/* Finalize */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.finalReq.msgHandle, 0, NULL},

		/* open request */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.openReq.msgHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.openReq.queueName, 0, NULL},
		{EDU_EXEC, mqsv_edp_samsgqueuecreationattributest, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.openReq.creationAttributes, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGOPENFLAGST, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.openReq.openFlags, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.openReq.timeout, 0, NULL},
		/* Open Async */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.openAsyncReq.mqpOpenReq.msgHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.openAsyncReq.mqpOpenReq.queueName, 0, NULL},
		{EDU_EXEC, mqsv_edp_samsgqueuecreationattributest, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.openAsyncReq.mqpOpenReq.creationAttributes, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGOPENFLAGST, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.openAsyncReq.mqpOpenReq.openFlags, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAINVOCATIONT, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.openAsyncReq.invocation, 0, NULL},

		/* close */
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.closeReq.queueHandle, 0, NULL},

		/* status */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.statusReq.msgHandle, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.statusReq.queueHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.statusReq.queueName, 0, NULL},

		/* unlink */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.unlinkReq.msgHandle, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.unlinkReq.queueHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.unlinkReq.queueName, 0, NULL},

		/*    MQP_EVT_REPLY_MSG  */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.msgHandle, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAUINT32T, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.message.type, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT32T, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.message.version, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SASIZET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.message.size, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.message.priority, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, EDQ_POINTER, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.message.senderName, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, EDQ_VAR_LEN_DATA, m_NCS_EDP_SASIZET, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.message.data,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.message.size, NULL},
		{EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_OS_SVCS /* Svc-ID */ , NULL, 0, 0 /* Sub-ID */ , 0, NULL},

		{EDU_EXEC, mqsv_edp_message_info, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.messageInfo, 0, NULL},
		{EDU_EXEC, M_NCS_EDP_SAMSGACKFLAGST, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.ackFlags, 0, NULL},

		/* MQP_EVT_REPLY_MSG_ASYNC */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyAsyncMsg.reply.msgHandle, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAUINT32T, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyAsyncMsg.reply.message.type, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT32T, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyAsyncMsg.reply.message.version, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SASIZET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyAsyncMsg.reply.message.size, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyAsyncMsg.reply.message.priority, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, EDQ_POINTER, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyAsyncMsg.reply.message.senderName, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, EDQ_VAR_LEN_DATA, m_NCS_EDP_SASIZET, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyAsyncMsg.reply.message.data,
		 (long)&((MQP_REQ_MSG *)0)->info.replyMsg.message.size, NULL},
		{EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_OS_SVCS /* Svc-ID */ , NULL, 0, 0 /* Sub-ID */ , 0, NULL},

		{EDU_EXEC, mqsv_edp_message_info, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyAsyncMsg.reply.messageInfo, 0, NULL},
		{EDU_EXEC, M_NCS_EDP_SAMSGACKFLAGST, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.replyAsyncMsg.reply.ackFlags, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAINVOCATIONT, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.replyAsyncMsg.invocation, 0, NULL},

		/* MQP_EVT_SEND_MSG */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.msgHandle, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.queueHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.destination, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT32T, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.message.type, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT32T, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.message.version, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SASIZET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.message.size, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.message.priority, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, EDQ_POINTER, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.message.senderName, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, EDQ_VAR_LEN_DATA, m_NCS_EDP_SASIZET, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.message.data,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.message.size, NULL},
		{EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_OS_SVCS /* Svc-ID */ , NULL, 0, 0 /* Sub-ID */ , 0, NULL},

		{EDU_EXEC, mqsv_edp_message_info, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.messageInfo, 0, NULL},
		{EDU_EXEC, M_NCS_EDP_SAMSGACKFLAGST, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.snd_msg.ackFlags, 0, NULL},

		/* MQP_EVT_TRANSFER_QUEUE_REQ */
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.transferReq.old_queueHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.transferReq.new_owner, 0, NULL},
		{EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.transferReq.empty_queue, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqp_open_req, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.transferReq.openReq, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAINVOCATIONT, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.transferReq.invocation, 0, NULL},
		{EDU_EXEC, mqsv_edp_send_info, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.transferReq.rcvr_mqa_sinfo, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.transferReq.openType, 0, NULL},

		/* MQP_EVT_TRANSFER_QUEUE_COMPLETE */
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.transferComplete.queueHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, EDU_EXIT,
		 (long)&((MQP_REQ_MSG *)0)->info.transferComplete.error, 0, NULL},

		/* MQP_EVT_UPDATE_STATS */
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.statsReq.qhdl, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.statsReq.priority, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.statsReq.size, 0, NULL},

		/* MQP_EVT_Q_RET_TIME_SET_REQ */
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.retTimeSetReq.queueHandle, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.retTimeSetReq.retentionTime, 0, NULL},

		/* MQP_EVT_CLM_NOTIFY */
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_REQ_MSG *)0)->info.clmNotify.node_joined, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},

	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQP_REQ_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQP_REQ_MSG **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(MQP_REQ_MSG));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_mqp_req_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_queue_usage

  DESCRIPTION:      EDU program handler for "SaMsgQueueUsageT" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "SaMsgQueueUsageT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_queue_usage(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				  NCSCONTEXT ptr, uns32 *ptr_data_len,
				  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaMsgQueueUsageT *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_queue_usage_rules[] = {
		{EDU_START, mqsv_edp_queue_usage, 0, 0, 0,
		 sizeof(SaMsgQueueUsageT), 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SASIZET, 0, 0, 0,
		 (long)&((SaMsgQueueUsageT *)0)->queueSize, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SASIZET, 0, 0, 0,
		 (long)&((SaMsgQueueUsageT *)0)->queueUsed, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((SaMsgQueueUsageT *)0)->numberOfMessages, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (SaMsgQueueUsageT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (SaMsgQueueUsageT **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(SaMsgQueueUsageT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_queue_usage_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_queue_status

  DESCRIPTION:      EDU program handler for "SaMsgQueueStatusT" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "SaMsgQueueStatusT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_queue_status(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uns32 *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaMsgQueueStatusT *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_queue_status_rules[] = {
		{EDU_START, mqsv_edp_queue_status, 0, 0, 0,
		 sizeof(SaMsgQueueStatusT), 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUECREATIONFLAGST, 0, 0, 0,
		 (long)&((SaMsgQueueStatusT *)0)->creationFlags, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0,
		 (long)&((SaMsgQueueStatusT *)0)->retentionTime, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0,
		 (long)&((SaMsgQueueStatusT *)0)->closeTime, 0, NULL},

		{EDU_EXEC, mqsv_edp_queue_usage, EDQ_ARRAY, 0, 0,
		 (long)&((SaMsgQueueStatusT *)0)->saMsgQueueUsage, SA_MSG_MESSAGE_LOWEST_PRIORITY + 1, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (SaMsgQueueStatusT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (SaMsgQueueStatusT **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(SaMsgQueueStatusT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_queue_status_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_mqp_rsp_test_type_fnc

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "mqsv_edp_mqp_rsp".

  RETURNS:          uns32, denoting the offset of the next-instruction 
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "mqsv_edp_mqp_rsp").

*****************************************************************************/
static int mqsv_mqp_rsp_test_type_fnc(NCSCONTEXT arg)
{
	typedef enum {
		LCL_TEST_JUMP_OFFSET_MQP_EVT_INIT_RSP = 1,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_FINALIZE_RSP,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_OPEN_RSP,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_CLOSE_RSP = 8,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_STATUS_RSP,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_UNLINK_RSP = 13,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_REPLY_MSG_RSP = 16,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_SEND_MSG_RSP,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_TRANSFER_QUEUE_RSP,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_ND_RESTART_RSP = 29,
		LCL_TEST_JUMP_OFFSET_MQP_EVT_RET_TIME_SET_RSP,

	} LCL_TEST_JUMP_OFFSET;
	MQP_RSP_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(MQP_RSP_TYPE *)arg;

	switch (type) {
	case MQP_EVT_INIT_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_INIT_RSP;
	case MQP_EVT_FINALIZE_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_FINALIZE_RSP;
	case MQP_EVT_OPEN_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_OPEN_RSP;
	case MQP_EVT_CLOSE_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_CLOSE_RSP;
	case MQP_EVT_STATUS_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_STATUS_RSP;
	case MQP_EVT_UNLINK_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_UNLINK_RSP;
	case MQP_EVT_REPLY_MSG_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_REPLY_MSG_RSP;
	case MQP_EVT_SEND_MSG_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_SEND_MSG_RSP;
	case MQP_EVT_TRANSFER_QUEUE_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_TRANSFER_QUEUE_RSP;
	case MQP_EVT_MQND_RESTART_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_ND_RESTART_RSP;
	case MQP_EVT_Q_RET_TIME_SET_RSP:
		return LCL_TEST_JUMP_OFFSET_MQP_EVT_RET_TIME_SET_RSP;
	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqp_rsp

  DESCRIPTION:      EDU program handler for "MQP_RSP_MSG" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "MQP_RSP_MSG" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_mqp_rsp(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQP_RSP_MSG *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_mqp_rsp_rules[] = {
		{EDU_START, mqsv_edp_mqp_rsp, 0, 0, 0,
		 sizeof(MQP_RSP_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->type, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAERRORT, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->error, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->type, 0,
		 mqsv_mqp_rsp_test_type_fnc},

		/* Initialize */
		{EDU_EXEC, ncs_edp_uns32, 0, 0, EDU_EXIT,
		 (long)&((MQP_RSP_MSG *)0)->info.initRsp.dummy, 0, NULL},

		/* Finalize */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, EDU_EXIT,
		 (long)&((MQP_RSP_MSG *)0)->info.finalRsp.msgHandle, 0, NULL},

		/* open response */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.openRsp.msgHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.openRsp.queueName, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.openRsp.queueHandle, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.openRsp.listenerHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, EDU_EXIT,
		 (long)&((MQP_RSP_MSG *)0)->info.openRsp.existing_msg_count, 0, NULL},

		/* close */
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, EDU_EXIT,
		 (long)&((MQP_RSP_MSG *)0)->info.closeRsp.queueHandle, 0, NULL},

		/* status response */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.statusRsp.msgHandle, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.statusRsp.queueHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.statusRsp.queueName, 0, NULL},
		{EDU_EXEC, mqsv_edp_queue_status, 0, 0, EDU_EXIT,
		 (long)&((MQP_RSP_MSG *)0)->info.statusRsp.queueStatus, 0, NULL},

		/* unlink */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.unlinkRsp.msgHandle, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.unlinkRsp.queueHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((MQP_RSP_MSG *)0)->info.unlinkRsp.queueName, 0, NULL},

		/*    MQP_EVT_REPLY_MSG  */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, EDU_EXIT,
		 (long)&((MQP_RSP_MSG *)0)->info.replyRsp.msgHandle, 0, NULL},

		/* MQP_EVT_SEND_MSG */
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, EDU_EXIT,
		 (long)&((MQP_RSP_MSG *)0)->info.sendMsgRsp.msgHandle, 0, NULL},

		/* MQP_EVT_TRANSFER_QUEUE_RSP */
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.old_queueHandle, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqp_open_req, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.openReq, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAINVOCATIONT, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.invocation, 0, NULL},
		{EDU_EXEC, mqsv_edp_send_info, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.rcvr_mqa_sinfo, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.old_owner, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.openType, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.msg_count, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.creationTime, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.msg_bytes, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, EDQ_VAR_LEN_DATA, ncs_edp_uns32, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.mqsv_message,
		 (long)&((MQP_RSP_MSG *)0)->info.transferRsp.msg_bytes, NULL},
		{EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_OS_SVCS /* Svc-ID */ , NULL, EDU_EXIT, 0 /* Sub-ID */ , 0, NULL},

		/* MQP_EVT_MQND_RESTART_RSP  */
		{EDU_EXEC, ncs_edp_uns32, 0, 0, EDU_EXIT,
		 (long)&((MQP_RSP_MSG *)0)->info.restartRsp.restart_done, 0, NULL},

		/* MQP_EVT_Q_RET_TIME_SET_RSP */
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_RSP_MSG *)0)->info.retTimeSetRsp.queueHandle, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQP_RSP_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQP_RSP_MSG **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(MQP_RSP_MSG));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_mqp_rsp_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_mqa_callback_test_type_fnc

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "mqsv_edp_mqa_callback".

  RETURNS:          uns32, denoting the offset of the next-instruction 
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "mqsv_edp_mqa_callback").

*****************************************************************************/

static int mqsv_mqa_callback_test_type_fnc(NCSCONTEXT arg)
{
	typedef enum {
		LCL_TEST_JUMP_OFFSET_MQSV_OPEN_CALLBACK = 1,
		LCL_TEST_JUMP_OFFSET_MQSV_GROUPTRACK_CALLBACK = 7,
		LCL_TEST_JUMP_OFFSET_MQSV_MSG_DELIVERED = 7,
		LCL_TEST_JUMP_OFFSET_MQSV_MSG_RECEIVED = 8
	} LCL_TEST_JUMP_OFFSET;
	MQP_ASYNC_RSP_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(MQP_ASYNC_RSP_TYPE *)arg;

	switch (type) {
	case MQP_ASYNC_RSP_OPEN:
		return LCL_TEST_JUMP_OFFSET_MQSV_OPEN_CALLBACK;
	case MQP_ASYNC_RSP_GRP_TRACK:
		return LCL_TEST_JUMP_OFFSET_MQSV_GROUPTRACK_CALLBACK;
	case MQP_ASYNC_RSP_MSGDELIVERED:
		return LCL_TEST_JUMP_OFFSET_MQSV_MSG_DELIVERED;
	case MQP_ASYNC_RSP_MSGRECEIVED:
		return LCL_TEST_JUMP_OFFSET_MQSV_MSG_RECEIVED;
	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqa_callback

  DESCRIPTION:      EDU program handler for "MQP_ASYNC_RSP_MSG" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "MQP_ASYNC_RSP_MSG" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_mqa_callback(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uns32 *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQP_ASYNC_RSP_MSG *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_mqa_callback_rules[] = {
		{EDU_START, mqsv_edp_mqa_callback, 0, 0, 0,
		 sizeof(MQP_ASYNC_RSP_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->callbackType, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGHANDLET, 0, 0, 0,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->messageHandle, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->callbackType, 0,
		 mqsv_mqa_callback_test_type_fnc},

		/* Open */
		{EDU_EXEC, m_NCS_EDP_SAINVOCATIONT, 0, 0, 0,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->params.qOpen.invocation, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->params.qOpen.queueHandle, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAERRORT, 0, 0, 0,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->params.qOpen.error, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGOPENFLAGST, 0, 0, 0,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->params.qOpen.openFlags, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, 0,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->params.qOpen.listenerHandle, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, EDU_EXIT,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->params.qOpen.existing_msg_count, 0, NULL},

		/* track  TBD */

		/* message delivered  */
		{EDU_EXEC, m_NCS_EDP_SAINVOCATIONT, 0, 0, 0,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->params.msgDelivered.invocation, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAERRORT, 0, 0, EDU_EXIT,
		 (long)&((MQP_ASYNC_RSP_MSG *)0)->params.msgDelivered.error, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQP_ASYNC_RSP_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQP_ASYNC_RSP_MSG **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(MQP_ASYNC_RSP_MSG));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_mqa_callback_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_ctrl_msg_test_type_fnc

  DESCRIPTION:     
                  
                

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static int mqsv_edp_mqd_ctrl_msg_test_type_fnc(NCSCONTEXT arg)
{
	typedef enum {
		LCL_TEST_JUMP_OFFSET_MQD_QGRP_CNT_GET = 1
	} LCL_TEST_JUMP_OFFSET;
	MQD_MSG_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(MQD_MSG_TYPE *)arg;

	switch (type) {
	case MQD_QGRP_CNT_GET:
		return LCL_TEST_JUMP_OFFSET_MQD_QGRP_CNT_GET;
	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:  mqsv_edp_mqnd_ctrl_msg_test_type_fnc 

  DESCRIPTION:   
              
            

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static int mqsv_edp_mqnd_ctrl_msg_test_type_fnc(NCSCONTEXT arg)
{
	typedef enum {
		LCL_TEST_JUMP_OFFSET_MQND_CTRL_EVT_QATTR_GET = 1,
		LCL_TEST_JUMP_OFFSET_MQND_CTRL_EVT_QATTR_INFO = 2,
		LCL_TEST_JUMP_OFFSET_MQND_CTRL_EVT_QGRP_MEMBER_INFO = 4,
		LCL_TEST_JUMP_OFFSET_MQND_CTRL_EVT_QGRP_CNT_RSP = 4 
	} LCL_TEST_JUMP_OFFSET;
	MQND_CTRL_EVT_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(MQND_CTRL_EVT_TYPE *)arg;

	switch (type) {
	case MQND_CTRL_EVT_QATTR_GET:
		return LCL_TEST_JUMP_OFFSET_MQND_CTRL_EVT_QATTR_GET;
	case MQND_CTRL_EVT_QATTR_INFO:
		return LCL_TEST_JUMP_OFFSET_MQND_CTRL_EVT_QATTR_INFO;
	case MQND_CTRL_EVT_QGRP_MEMBER_INFO:
		return LCL_TEST_JUMP_OFFSET_MQND_CTRL_EVT_QGRP_MEMBER_INFO;
	case MQND_CTRL_EVT_QGRP_CNT_RSP:
		return LCL_TEST_JUMP_OFFSET_MQND_CTRL_EVT_QGRP_CNT_RSP;
	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_ctrl_msg

  DESCRIPTION:      EDU program handler for MQD_CTRL_MSG data. This
                    function is invoked by EDU for performing encode/decode
                    operation on MQD_CTRL_MSG data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_mqd_ctrl_msg(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uns32 *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQD_CTRL_MSG *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_ctrl_msg_rules[] = {
		{EDU_START, mqsv_edp_mqd_ctrl_msg, 0, 0, 0,
		 sizeof(MQD_CTRL_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_CTRL_MSG *)0)->type, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_CTRL_MSG *)0)->type, 0,
		 mqsv_edp_mqd_ctrl_msg_test_type_fnc},
		{EDU_EXEC, mqsv_qgrp_cnt_info, 0, 0, 0,
		 (long)&((MQND_CTRL_MSG *)0)->info.qgrp_cnt_info, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQD_CTRL_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQD_CTRL_MSG **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(MQD_CTRL_MSG));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_ctrl_msg_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqnd_ctrl_msg

  DESCRIPTION:      EDU program handler for "MQP_ASYNC_RSP_MSG" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "MQP_ASYNC_RSP_MSG" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_edp_mqnd_ctrl_msg(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				    NCSCONTEXT ptr, uns32 *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQND_CTRL_MSG *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqnd_ctrl_msg_rules[] = {
		{EDU_START, mqsv_edp_mqnd_ctrl_msg, 0, 0, 0,
		 sizeof(MQND_CTRL_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQND_CTRL_MSG *)0)->type, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQND_CTRL_MSG *)0)->type, 0,
		 mqsv_edp_mqnd_ctrl_msg_test_type_fnc},
		{EDU_EXEC, m_NCS_EDP_SAMSGQUEUEHANDLET, 0, 0, EDU_EXIT,
		 (long)&((MQND_CTRL_MSG *)0)->info.qattr_get.qhdl, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAERRORT, 0, 0, 0,
		 (long)&((MQND_CTRL_MSG *)0)->info.qattr_info.error, 0, NULL},
		{EDU_EXEC, mqsv_edp_samsgqueuecreationattributest, 0, 0, EDU_EXIT,
		 (long)&((MQND_CTRL_MSG *)0)->info.qattr_info.qattr, 0, NULL},

		{EDU_EXEC, mqsv_qgrp_cnt_info, 0, 0, 0,
		 (long)&((MQND_CTRL_MSG *)0)->info.qgrp_cnt_info, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQND_CTRL_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQND_CTRL_MSG **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(MQND_CTRL_MSG));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqnd_ctrl_msg_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:  mqsv_qgrp_cnt_info 

  DESCRIPTION:  
                  
                  

  RETURNS:         NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uns32 mqsv_qgrp_cnt_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uns32 *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	MQSV_CTRL_EVT_QGRP_CNT *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_qgrp_cnt_info_rules[] = {
		{EDU_START, mqsv_qgrp_cnt_info, 0, 0, 0,
		 sizeof(MQSV_CTRL_EVT_QGRP_CNT), 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAERRORT, 0, 0, 0,
		 (long)&((MQSV_CTRL_EVT_QGRP_CNT *)0)->error, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((MQSV_CTRL_EVT_QGRP_CNT *)0)->info.queueName, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQSV_CTRL_EVT_QGRP_CNT *)0)->info.noOfQueueGroupMemOf, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQSV_CTRL_EVT_QGRP_CNT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQSV_CTRL_EVT_QGRP_CNT **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(MQSV_CTRL_EVT_QGRP_CNT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_qgrp_cnt_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************/
