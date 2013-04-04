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
  in MBCSv related components of MQSV.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/
#include "mqsv.h"

#include "mqd.h"

static uint32_t mqsv_edp_mqd_asapi_reg_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					 NCSCONTEXT ptr, uint32_t *ptr_data_len,
					 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t mqsv_edp_mqd_asapi_dereg_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					   NCSCONTEXT ptr, uint32_t *ptr_data_len,
					   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t mqsv_edp_mqd_asapi_que_param(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					  NCSCONTEXT ptr, uint32_t *ptr_data_len,
					  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t mqsv_edp_mqd_asapi_track_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					   NCSCONTEXT ptr, uint32_t *ptr_data_len,
					   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t mqsv_edp_mqd_a2s_user_evt_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					    NCSCONTEXT ptr, uint32_t *ptr_data_len,
					    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t mqsv_edp_mqd_a2s_nd_stat_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					   NCSCONTEXT ptr, uint32_t *ptr_data_len,
					   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t mqsv_edp_mqd_a2s_nd_timer_exp_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
						NCSCONTEXT ptr, uint32_t *ptr_data_len,
						EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t mqsv_edp_mqd_track_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uint32_t *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t mqsv_edp_mqd_queue_param(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				      NCSCONTEXT ptr, uint32_t *ptr_data_len,
				      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t mqsv_edp_mqd_qgroup_param(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				       NCSCONTEXT ptr, uint32_t *ptr_data_len,
				       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static int mqd_a2s_msg_test_type_fnc(NCSCONTEXT arg);
static int mqd_a2s_que_info_test_type_func(NCSCONTEXT arg);

/*****************************************************************************

  PROCEDURE NAME:   mqd_a2s_msg_test_type_fnc

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "mqsv_edp_mqd_a2s_msg".

  RETURNS:          uns32, denoting the offset of the next-instruction
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "mqsv_edp_mqd_a2s_msg").

*****************************************************************************/
static int mqd_a2s_msg_test_type_fnc(NCSCONTEXT arg)
{
	enum {
		LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_REG = 1,
		LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_DEREG,
		LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_TRACK,
		LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_QINFO,
		LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_USEREVT,
		LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_ND_STATEVT,
		LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_ND_TIMER_EXPEVT,
	};
	MQD_A2S_MSG_TYPE type;

	if (arg == NULL) {
		LOG_ER("%s:%u: Invalid EDP argument type", __FILE__, __LINE__);
		return EDU_FAIL;
	}

	type = *(MQD_A2S_MSG_TYPE *)arg;

	switch (type) {
	case MQD_A2S_MSG_TYPE_REG:
		return LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_REG;
	case MQD_A2S_MSG_TYPE_DEREG:
		return LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_DEREG;
	case MQD_A2S_MSG_TYPE_TRACK:
		return LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_TRACK;
	case MQD_A2S_MSG_TYPE_QINFO:
		return LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_QINFO;
	case MQD_A2S_MSG_TYPE_USEREVT:
		return LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_USEREVT;
	case MQD_A2S_MSG_TYPE_MQND_STATEVT:
		return LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_ND_STATEVT;
	case MQD_A2S_MSG_TYPE_MQND_TIMER_EXPEVT:
		return LCL_TEST_JUMP_OFFSET_MQSV_MQD_A2S_MSG_ND_TIMER_EXPEVT;
	default:
		break;
	}

	LOG_ER("%s:%u: Invalid EDP argument type", __FILE__, __LINE__);
	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_a2s_msg

  DESCRIPTION:      EDU program handler for "MQD_A2S_MSG" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "MQD_A2S_MSG" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t mqsv_edp_mqd_a2s_msg(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			   NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_A2S_MSG *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_a2s_msg_rules[] = {
		{EDU_START, mqsv_edp_mqd_a2s_msg, 0, 0, 0,
		 sizeof(MQD_A2S_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_A2S_MSG *)0)->type, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_A2S_MSG *)0)->type, 0,
		 mqd_a2s_msg_test_type_fnc},
		{EDU_EXEC, mqsv_edp_mqd_asapi_reg_info, 0, 0, EDU_EXIT,
		 (long)&((MQD_A2S_MSG *)0)->info.reg, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_asapi_dereg_info, 0, 0, EDU_EXIT,
		 (long)&((MQD_A2S_MSG *)0)->info.dereg, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_track_info, 0, 0, EDU_EXIT,
		 (long)&((MQD_A2S_MSG *)0)->info.track, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_a2s_queue_info, 0, 0, EDU_EXIT,
		 (long)&((MQD_A2S_MSG *)0)->info.qinfo, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_a2s_user_evt_info, 0, 0, EDU_EXIT,
		 (long)&((MQD_A2S_MSG *)0)->info.user_evt, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_a2s_nd_stat_info, 0, 0, EDU_EXIT,
		 (long)&((MQD_A2S_MSG *)0)->info.nd_stat_evt, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_a2s_nd_timer_exp_info, 0, 0, EDU_EXIT,
		 (long)&((MQD_A2S_MSG *)0)->info.nd_tmr_exp_evt, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQD_A2S_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQD_A2S_MSG **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(MQD_A2S_MSG));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_a2s_msg_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_asapi_reg_info

  DESCRIPTION:      EDU program handler for "ASAPi_REG_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "ASAPi_REG_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t mqsv_edp_mqd_asapi_reg_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					 NCSCONTEXT ptr, uint32_t *ptr_data_len,
					 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_REG_INFO *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_asapi_reg_info_rules[] = {
		{EDU_START, mqsv_edp_mqd_asapi_reg_info, 0, 0, 0,
		 sizeof(ASAPi_REG_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((ASAPi_REG_INFO *)0)->objtype, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_asapi_que_param, 0, 0, 0,
		 (long)&((ASAPi_REG_INFO *)0)->queue, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((ASAPi_REG_INFO *)0)->group, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((ASAPi_REG_INFO *)0)->policy, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (ASAPi_REG_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (ASAPi_REG_INFO **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(ASAPi_REG_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_asapi_reg_info_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_asapi_que_param

  DESCRIPTION:      EDU program handler for "ASAPi_QUEUE_PARAM" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "ASAPi_QUEUE_PARAM" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t mqsv_edp_mqd_asapi_que_param(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					  NCSCONTEXT ptr, uint32_t *ptr_data_len,
					  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_QUEUE_PARAM *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_asapi_queue_param_rules[] = {
		{EDU_START, mqsv_edp_mqd_asapi_que_param, 0, 0, 0,
		 sizeof(ASAPi_QUEUE_PARAM), 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0,
		 (long)&((ASAPi_QUEUE_PARAM *)0)->addr, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0,
		 (long)&((ASAPi_QUEUE_PARAM *)0)->retentionTime, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((ASAPi_QUEUE_PARAM *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((ASAPi_QUEUE_PARAM *)0)->status, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((ASAPi_QUEUE_PARAM *)0)->hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((ASAPi_QUEUE_PARAM *)0)->owner, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, 0, 0, 0,
		 (long)&((ASAPi_QUEUE_PARAM *)0)->is_mqnd_down, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((ASAPi_QUEUE_PARAM *)0)->creationFlags, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, EDQ_ARRAY, 0, 0,
		 (long)&((ASAPi_QUEUE_PARAM *)0)->size, SA_MSG_MESSAGE_LOWEST_PRIORITY + 1, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (ASAPi_QUEUE_PARAM *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (ASAPi_QUEUE_PARAM **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(ASAPi_QUEUE_PARAM));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_asapi_queue_param_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_asapi_dereg_info

  DESCRIPTION:      EDU program handler for "ASAPi_DEREG_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "ASAPi_DEREG_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t mqsv_edp_mqd_asapi_dereg_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					   NCSCONTEXT ptr, uint32_t *ptr_data_len,
					   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_DEREG_INFO *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_asapi_dereg_info_rules[] = {
		{EDU_START, mqsv_edp_mqd_asapi_dereg_info, 0, 0, 0,
		 sizeof(ASAPi_DEREG_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((ASAPi_DEREG_INFO *)0)->objtype, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((ASAPi_DEREG_INFO *)0)->group, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((ASAPi_DEREG_INFO *)0)->queue, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (ASAPi_DEREG_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (ASAPi_DEREG_INFO **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(ASAPi_DEREG_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_asapi_dereg_info_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_asapi_track_info

  DESCRIPTION:      EDU program handler for "ASAPi_TRACK_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "ASAPi_TRACK_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t mqsv_edp_mqd_asapi_track_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					   NCSCONTEXT ptr, uint32_t *ptr_data_len,
					   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ASAPi_TRACK_INFO *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_asapi_track_info_rules[] = {
		{EDU_START, mqsv_edp_mqd_asapi_track_info, 0, 0, 0,
		 sizeof(ASAPi_TRACK_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((ASAPi_TRACK_INFO *)0)->object, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, 0, 0, 0,
		 (long)&((ASAPi_TRACK_INFO *)0)->val, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (ASAPi_TRACK_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (ASAPi_TRACK_INFO **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(ASAPi_TRACK_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_asapi_track_info_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_a2s_user_evt_info

  DESCRIPTION:      EDU program handler for "MQD_A2S_USER_EVENT_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "MQD_A2S_USER_EVENT_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t mqsv_edp_mqd_a2s_user_evt_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					    NCSCONTEXT ptr, uint32_t *ptr_data_len,
					    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_A2S_USER_EVENT_INFO *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_a2s_user_evt_info_rules[] = {
		{EDU_START, mqsv_edp_mqd_a2s_user_evt_info, 0, 0, 0,
		 sizeof(MQD_A2S_USER_EVENT_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0,
		 (long)&((MQD_A2S_USER_EVENT_INFO *)0)->dest, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQD_A2S_USER_EVENT_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQD_A2S_USER_EVENT_INFO **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			*o_err = EDU_ERR_MEM_FAIL;
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(MQD_A2S_USER_EVENT_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_a2s_user_evt_info_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_a2s_nd_stat_info

  DESCRIPTION:      EDU program handler for "MQD_A2S_ND_STAT_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "MQD_A2S_ND_STAT_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t mqsv_edp_mqd_a2s_nd_stat_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					   NCSCONTEXT ptr, uint32_t *ptr_data_len,
					   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_A2S_ND_STAT_INFO *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_a2s_nd_stat_info_rules[] = {
		{EDU_START, mqsv_edp_mqd_a2s_nd_stat_info, 0, 0, 0,
		 sizeof(MQD_A2S_ND_STAT_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_A2S_ND_STAT_INFO *)0)->nodeid, 0, NULL},
		{EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0,
		 (long)&((MQD_A2S_ND_STAT_INFO *)0)->is_restarting, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0,
		 (long)&((MQD_A2S_ND_STAT_INFO *)0)->downtime, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQD_A2S_ND_STAT_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQD_A2S_ND_STAT_INFO **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(MQD_A2S_ND_STAT_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_a2s_nd_stat_info_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_a2s_nd_timer_exp_info

  DESCRIPTION:      EDU program handler for "MQD_A2S_ND_TIMER_EXP_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "MQD_A2S_ND_TIMER_EXP_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t mqsv_edp_mqd_a2s_nd_timer_exp_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
						NCSCONTEXT ptr, uint32_t *ptr_data_len,
						EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_A2S_ND_TIMER_EXP_INFO *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_a2s_nd_timer_exp_info_rules[] = {
		{EDU_START, mqsv_edp_mqd_a2s_nd_timer_exp_info, 0, 0, 0,
		 sizeof(MQD_A2S_ND_TIMER_EXP_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_A2S_ND_TIMER_EXP_INFO *)0)->nodeid, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQD_A2S_ND_TIMER_EXP_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQD_A2S_ND_TIMER_EXP_INFO **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(MQD_A2S_ND_TIMER_EXP_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_a2s_nd_timer_exp_info_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_a2s_queue_info

  DESCRIPTION:      EDU program handler for "MQD_A2S_QUEUE_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "MQD_A2S_QUEUE_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t mqsv_edp_mqd_a2s_queue_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				  NCSCONTEXT ptr, uint32_t *ptr_data_len,
				  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_A2S_QUEUE_INFO *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_a2s_queue_info_rules[] = {
		{EDU_START, mqsv_edp_mqd_a2s_queue_info, 0, 0, 0,
		 sizeof(MQD_A2S_QUEUE_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((MQD_A2S_QUEUE_INFO *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_A2S_QUEUE_INFO *)0)->type, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_A2S_QUEUE_INFO *)0)->type, 0,
		 mqd_a2s_que_info_test_type_func},
		{EDU_EXEC, mqsv_edp_mqd_queue_param, 0, 0, 6,
		 (long)&((MQD_A2S_QUEUE_INFO *)0)->info.q, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_qgroup_param, 0, 0, 6,
		 (long)&((MQD_A2S_QUEUE_INFO *)0)->info.qgrp, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_A2S_QUEUE_INFO *)0)->ilist_cnt, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, EDQ_VAR_LEN_DATA, ncs_edp_uns32, 0,
		 (long)&((MQD_A2S_QUEUE_INFO *)0)->ilist_info, (long)&((MQD_A2S_QUEUE_INFO *)0)->ilist_cnt, NULL},
		{EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_OS_SVCS /* Svc-ID */ , NULL, 0, 0 /* Sub-ID */ , 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_A2S_QUEUE_INFO *)0)->track_cnt, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_track_info, EDQ_VAR_LEN_DATA, ncs_edp_uns32, 0,
		 (long)&((MQD_A2S_QUEUE_INFO *)0)->track_info, (long)&((MQD_A2S_QUEUE_INFO *)0)->track_cnt, NULL},
		{EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_OS_SVCS /* Svc-ID */ , NULL, 0, 0 /* Sub-ID */ , 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQD_A2S_QUEUE_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQD_A2S_QUEUE_INFO **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(MQD_A2S_QUEUE_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_a2s_queue_info_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_queue_param

  DESCRIPTION:      EDU program handler for "MQD_QUEUE_PARAM" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "MQD_QUEUE_PARAM" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t mqsv_edp_mqd_queue_param(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				      NCSCONTEXT ptr, uint32_t *ptr_data_len,
				      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_QUEUE_PARAM *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_queue_param_rules[] = {
		{EDU_START, mqsv_edp_mqd_queue_param, 0, 0, 0,
		 sizeof(MQD_QUEUE_PARAM), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_QUEUE_PARAM *)0)->send_state, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0,
		 (long)&((MQD_QUEUE_PARAM *)0)->retentionTime, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0,
		 (long)&((MQD_QUEUE_PARAM *)0)->dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_QUEUE_PARAM *)0)->owner, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_QUEUE_PARAM *)0)->hdl, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, 0, 0, 0,
		 (long)&((MQD_QUEUE_PARAM *)0)->adv, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, 0, 0, 0,
		 (long)&((MQD_QUEUE_PARAM *)0)->is_mqnd_down, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_QUEUE_PARAM *)0)->creationFlags, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, EDQ_ARRAY, 0, 0,
		 (long)&((MQD_QUEUE_PARAM *)0)->size, SA_MSG_MESSAGE_LOWEST_PRIORITY + 1, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQD_QUEUE_PARAM *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQD_QUEUE_PARAM **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(MQD_QUEUE_PARAM));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_queue_param_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_qgroup_param

  DESCRIPTION:      EDU program handler for "MQD_QGROUP_PARAM" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "MQD_QGROUP_PARAM" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t mqsv_edp_mqd_qgroup_param(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				       NCSCONTEXT ptr, uint32_t *ptr_data_len,
				       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_QGROUP_PARAM *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_qgroup_param_rules[] = {
		{EDU_START, mqsv_edp_mqd_qgroup_param, 0, 0, 0,
		 sizeof(MQD_QGROUP_PARAM), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_QGROUP_PARAM *)0)->policy, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQD_QGROUP_PARAM *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQD_QGROUP_PARAM **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(MQD_QGROUP_PARAM));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_qgroup_param_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   mqd_a2s_que_info_test_type_func

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "mqsv_edp_mqd_a2s_queue_info".

  RETURNS:          uns32, denoting the offset of the next-instruction
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "mqsv_edp_mqd_a2s_queue_info").

*****************************************************************************/
static int mqd_a2s_que_info_test_type_func(NCSCONTEXT arg)
{
	enum {
		LCL_TEST_JUMP_OFFSET_MQSV_OBJ_QUEUE = 1,
		LCL_TEST_JUMP_OFFSET_MQSV_OBJ_QGROUP
	};
	MQSV_OBJ_TYPE type;

	if (arg == NULL) {
		LOG_ER("%s:%u: Invalid EDP argument type", __FILE__, __LINE__);
		return EDU_FAIL;
	}

	type = *(MQSV_OBJ_TYPE *)arg;

	switch (type) {
	case MQSV_OBJ_QUEUE:
		return LCL_TEST_JUMP_OFFSET_MQSV_OBJ_QUEUE;
	case MQSV_OBJ_QGROUP:
		return LCL_TEST_JUMP_OFFSET_MQSV_OBJ_QGROUP;
	default:
		break;
	}

	LOG_ER("%s:%u: Invalid EDP argument type", __FILE__, __LINE__);
	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   mqsv_edp_mqd_track_info

  DESCRIPTION:      EDU program handler for "MQD_A2S_TRACK_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "MQD_A2S_TRACK_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
static uint32_t mqsv_edp_mqd_track_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uint32_t *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_A2S_TRACK_INFO *struct_ptr = NULL, **d_ptr = NULL;
	EDU_INST_SET mqsv_edp_mqd_track_info_rules[] = {
		{EDU_START, mqsv_edp_mqd_track_info, 0, 0, 0,
		 sizeof(MQD_A2S_TRACK_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0,
		 (long)&((MQD_A2S_TRACK_INFO *)0)->dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((MQD_A2S_TRACK_INFO *)0)->to_svc, 0, NULL},
		{EDU_EXEC, mqsv_edp_mqd_asapi_track_info, 0, 0, 0,
		 (long)&((MQD_A2S_TRACK_INFO *)0)->track, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (MQD_A2S_TRACK_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (MQD_A2S_TRACK_INFO **)ptr;
		if (*d_ptr == NULL) {
			/* This should have already been a valid pointer. */
			LOG_ER("%s:%u: Event is NULL", __FILE__, __LINE__);
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}

		memset(*d_ptr, '\0', sizeof(MQD_A2S_TRACK_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, mqsv_edp_mqd_track_info_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}
