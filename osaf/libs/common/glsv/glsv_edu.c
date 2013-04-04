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

  This file contains GLSV EDU routines.
 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

/* Common Headers */
#include "ncsgl_defs.h"

/* From /base/common/inc */
#include "ncs_lib.h"
#include "ncs_ubaid.h"

#include "mds_papi.h"

/* GLSV common Include Files */
#include "glsv_defs.h"
#include "glsv_lck.h"

/* events includes */
#include "glnd_evt.h"
#include "gld_evt.h"
#include "ncs_edu_pub.h"
#include "ncs_saf_edu.h"

/* memory includes */
#include "glsv_mem.h"
#include "glnd_mem.h"

#include "glnd_edu.h"

#include "gld.h"

static uint32_t glsv_edp_lock_req_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				    NCSCONTEXT ptr, uint32_t *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

/****************************************************************************
 * Name          : glsv_glnd_evt_test_type_fnc
 *
 * Description   : This is the function which is used to test the event type
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static int glsv_glnd_evt_test_type_fnc(NCSCONTEXT arg)
{
	enum {
		LCL_TEST_JUMP_OFFSET_AGENT_INFO = 1,
		LCL_TEST_JUMP_OFFSET_CLIENT_INFO,
		LCL_TEST_JUMP_OFFSET_RESTART_CLIENT_INFO,
		LCL_TEST_JUMP_OFFSET_FINALIZE_INFO,
		LCL_TEST_JUMP_OFFSET_RSC_INFO,
		LCL_TEST_JUMP_OFFSET_RSC_LOCK_INFO,
		LCL_TEST_JUMP_OFFSET_RSC_UNLOCK_INFO,
		LCL_TEST_JUMP_OFFSET_GLND_LCK_INFO,
		LCL_TEST_JUMP_OFFSET_GLND_RSC_INFO,
		LCL_TEST_JUMP_OFFSET_DD_PROBE_INFO,
		LCL_TEST_JUMP_OFFSET_RSC_GLD_INFO,
		LCL_TEST_JUMP_OFFSET_RSC_NEW_MAST_INFO,
		LCL_TEST_JUMP_OFFSET_RSC_MASTER_INFO,
		LCL_TEST_JUMP_OFFSET_NON_MASTER_INFO,
		LCL_TEST_JUMP_OFFSET_TMR
	};
	GLSV_GLND_EVT_TYPE evt_type;

	if (arg == NULL)
		return EDU_FAIL;

	evt_type = *(GLSV_GLND_EVT_TYPE *)arg;
	switch (evt_type) {
	case GLSV_GLND_EVT_REG_AGENT:
	case GLSV_GLND_EVT_UNREG_AGENT:
		return LCL_TEST_JUMP_OFFSET_AGENT_INFO;
		break;

	case GLSV_GLND_EVT_INITIALIZE:
		return LCL_TEST_JUMP_OFFSET_CLIENT_INFO;
		break;

	case GLSV_GLND_EVT_CLIENT_INFO:
		return LCL_TEST_JUMP_OFFSET_RESTART_CLIENT_INFO;
		break;

	case GLSV_GLND_EVT_FINALIZE:
		return LCL_TEST_JUMP_OFFSET_FINALIZE_INFO;
		break;

	case GLSV_GLND_EVT_RSC_OPEN:
	case GLSV_GLND_EVT_RSC_CLOSE:
	case GLSV_GLND_EVT_RSC_PURGE:
	case GLSV_GLND_EVT_LCK_PURGE:
		return LCL_TEST_JUMP_OFFSET_RSC_INFO;
		break;

	case GLSV_GLND_EVT_RSC_LOCK:
		return LCL_TEST_JUMP_OFFSET_RSC_LOCK_INFO;
		break;

	case GLSV_GLND_EVT_RSC_UNLOCK:
		return LCL_TEST_JUMP_OFFSET_RSC_UNLOCK_INFO;
		break;

	case GLSV_GLND_EVT_LCK_REQ:
	case GLSV_GLND_EVT_UNLCK_REQ:
	case GLSV_GLND_EVT_LCK_RSP:
	case GLSV_GLND_EVT_UNLCK_RSP:
	case GLSV_GLND_EVT_LCK_WAITER_CALLBACK:
	case GLSV_GLND_EVT_LCK_REQ_CANCEL:
	case GLSV_GLND_EVT_LCK_REQ_ORPHAN:
		return LCL_TEST_JUMP_OFFSET_GLND_LCK_INFO;
		break;

	case GLSV_GLND_EVT_SND_RSC_INFO:
		return LCL_TEST_JUMP_OFFSET_GLND_RSC_INFO;
		break;

	case GLSV_GLND_EVT_FWD_DD_PROBE:
	case GLSV_GLND_EVT_DD_PROBE:
		return LCL_TEST_JUMP_OFFSET_DD_PROBE_INFO;
		break;

	case GLSV_GLND_EVT_RSC_GLD_DETAILS:
		return LCL_TEST_JUMP_OFFSET_RSC_GLD_INFO;
		break;

	case GLSV_GLND_EVT_RSC_NEW_MASTER:
		return LCL_TEST_JUMP_OFFSET_RSC_NEW_MAST_INFO;
		break;

	case GLSV_GLND_EVT_RSC_MASTER_INFO:
		return LCL_TEST_JUMP_OFFSET_RSC_MASTER_INFO;
		break;

	case GLSV_GLND_EVT_NON_MASTER_INFO:
		return LCL_TEST_JUMP_OFFSET_NON_MASTER_INFO;
		break;

	case GLSV_GLND_EVT_RSC_OPEN_TIMEOUT:
	case GLSV_GLND_EVT_RSC_LOCK_TIMEOUT:
	case GLSV_GLND_EVT_NM_RSC_LOCK_TIMEOUT:
	case GLSV_GLND_EVT_NM_RSC_UNLOCK_TIMEOUT:
		return EDU_EXIT;
		break;
	default:
		return EDU_EXIT;
		break;
	}
	return EDU_FAIL;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_agent_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_EVT_AGENT_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_agent_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					  NCSCONTEXT ptr, uint32_t *ptr_data_len,
					  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_AGENT_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_agent_info, 0, 0, 0, sizeof(GLSV_EVT_AGENT_INFO), 0, NULL},

		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_AGENT_INFO *)0)->process_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_AGENT_INFO *)0)->agent_mds_dest, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_AGENT_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_AGENT_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_AGENT_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_client_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_EVT_CLIENT_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_client_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					   NCSCONTEXT ptr, uint32_t *ptr_data_len,
					   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_CLIENT_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_client_info, 0, 0, 0, sizeof(GLSV_EVT_CLIENT_INFO), 0, NULL},

		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_CLIENT_INFO *)0)->client_proc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_CLIENT_INFO *)0)->agent_mds_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_saversiont, 0, 0, 0, (long)&((GLSV_EVT_CLIENT_INFO *)0)->version, 0, NULL},
		{EDU_EXEC, ncs_edp_uns16, 0, 0, 0, (long)&((GLSV_EVT_CLIENT_INFO *)0)->cbk_reg_info, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_CLIENT_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_CLIENT_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_CLIENT_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_restart_client_info
 *
 * Description   : This is the function which is used to encode decode
 *                 GLSV_EVT_RESTART_CLIENT_INFO structure.
 *
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_restart_client_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
						   NCSCONTEXT ptr, uint32_t *ptr_data_len,
						   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_RESTART_CLIENT_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_restart_client_info, 0, 0, 0, sizeof(GLSV_EVT_RESTART_CLIENT_INFO), 0,
		 NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RESTART_CLIENT_INFO *)0)->client_handle_id, 0,
		 NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RESTART_CLIENT_INFO *)0)->app_proc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_RESTART_CLIENT_INFO *)0)->agent_mds_dest, 0,
		 NULL},
		{EDU_EXEC, ncs_edp_saversiont, 0, 0, 0, (long)&((GLSV_EVT_RESTART_CLIENT_INFO *)0)->version, 0, NULL},
		{EDU_EXEC, ncs_edp_uns16, 0, 0, 0, (long)&((GLSV_EVT_RESTART_CLIENT_INFO *)0)->cbk_reg_info, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RESTART_CLIENT_INFO *)0)->no_of_res, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RESTART_CLIENT_INFO *)0)->resource_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_RESTART_CLIENT_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_RESTART_CLIENT_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_RESTART_CLIENT_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_finalize_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_EVT_FINALIZE_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_finalize_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					     NCSCONTEXT ptr, uint32_t *ptr_data_len,
					     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_FINALIZE_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_finalize_info, 0, 0, 0, sizeof(GLSV_EVT_FINALIZE_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_FINALIZE_INFO *)0)->agent_mds_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_FINALIZE_INFO *)0)->handle_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_FINALIZE_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_FINALIZE_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_FINALIZE_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_rsc_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_EVT_RSC_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_rsc_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					NCSCONTEXT ptr, uint32_t *ptr_data_len,
					EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_RSC_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_rsc_info, 0, 0, 0, sizeof(GLSV_EVT_RSC_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->client_handle_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->agent_mds_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->resource_name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->resource_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->lcl_resource_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->lcl_lockid, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->lcl_resource_id_count, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->invocation, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->call_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->timeout, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_INFO *)0)->flag, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_RSC_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_RSC_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_RSC_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_rsc_lock_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_EVT_RSC_LOCK_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_rsc_lock_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					     NCSCONTEXT ptr, uint32_t *ptr_data_len,
					     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_RSC_LOCK_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_rsc_lock_info, 0, 0, 0, sizeof(GLSV_EVT_RSC_LOCK_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->client_handle_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->agent_mds_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->resource_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->lcl_resource_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->lcl_lockid, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->invocation, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->lock_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->timeout, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->lockFlags, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->call_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->status, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_LOCK_INFO *)0)->waiter_signal, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_RSC_LOCK_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_RSC_LOCK_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_RSC_LOCK_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_rsc_unlock_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_EVT_RSC_UNLOCK_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_rsc_unlock_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					       NCSCONTEXT ptr, uint32_t *ptr_data_len,
					       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_RSC_UNLOCK_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_rsc_unlock_info, 0, 0, 0, sizeof(GLSV_EVT_RSC_UNLOCK_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_UNLOCK_INFO *)0)->client_handle_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_RSC_UNLOCK_INFO *)0)->agent_mds_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_UNLOCK_INFO *)0)->resource_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_UNLOCK_INFO *)0)->lockid, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_UNLOCK_INFO *)0)->lcl_lockid, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_UNLOCK_INFO *)0)->invocation, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_RSC_UNLOCK_INFO *)0)->call_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_RSC_UNLOCK_INFO *)0)->timeout, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_RSC_UNLOCK_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_RSC_UNLOCK_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_RSC_UNLOCK_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_node_lock_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_EVT_GLND_LCK_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_node_lock_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					      NCSCONTEXT ptr, uint32_t *ptr_data_len,
					      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_GLND_LCK_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_node_lock_info, 0, 0, 0, sizeof(GLSV_EVT_GLND_LCK_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->resource_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->lcl_resource_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->client_handle_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->glnd_mds_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->lockid, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->lcl_lockid, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->lock_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->lockFlags, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->lockStatus, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT64T, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->waiter_signal, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->mode_held, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->error, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_GLND_LCK_INFO *)0)->invocation, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_GLND_LCK_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_GLND_LCK_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_GLND_LCK_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_lock_list_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLND_LOCK_LIST_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_lock_list_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					  NCSCONTEXT ptr, uint32_t *ptr_data_len,
					  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLND_LOCK_LIST_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_lock_list_info, EDQ_LNKLIST, 0, 0, sizeof(GLND_LOCK_LIST_INFO), 0, NULL},
		{EDU_EXEC, glsv_edp_lock_req_info, 0, 0, 0, (long)&((GLND_LOCK_LIST_INFO *)0)->lock_info, 0, NULL},
		{EDU_TEST_LL_PTR, glsv_edp_glnd_lock_list_info, 0, 0, 0, (long)&((GLND_LOCK_LIST_INFO *)0)->next, 0,
		 NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLND_LOCK_LIST_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLND_LOCK_LIST_INFO **)ptr;
		if (*d_ptr == NULL) {
			/* malloc the memory */
			*d_ptr = (GLND_LOCK_LIST_INFO *)
			    m_MMGR_ALLOC_GLSV_GLND_LOCK_LIST_INFO(sizeof(GLND_LOCK_LIST_INFO), NCS_SERVICE_ID_GLND);
			if (*d_ptr == NULL) {
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
			memset(*d_ptr, '\0', sizeof(GLND_LOCK_LIST_INFO));
		}
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_glnd_rsc_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_EVT_GLND_RSC_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_glnd_rsc_info(EDU_HDL *edu_hdl,
					     EDU_TKN *edu_tkn,
					     NCSCONTEXT ptr, uint32_t *ptr_data_len,
					     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_GLND_RSC_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_glnd_rsc_info, 0, 0, 0, sizeof(GLSV_EVT_GLND_RSC_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_RSC_INFO *)0)->resource_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_GLND_RSC_INFO *)0)->glnd_mds_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_RSC_INFO *)0)->num_requests, 0, NULL},
		{EDU_EXEC, glsv_edp_glnd_lock_list_info, EDQ_POINTER, 0, 0,
		 (long)&((GLSV_EVT_GLND_RSC_INFO *)0)->list_of_req, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_GLND_RSC_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_GLND_RSC_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_GLND_RSC_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_res_master_list_info
 *
 * Description   : This is the function which is used to encode decode
 *                  GLSV_GLND_RSC_MASTER_INFO_LIST structure.
 *
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_res_master_list_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
						NCSCONTEXT ptr, uint32_t *ptr_data_len,
						EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_GLND_RSC_MASTER_INFO_LIST *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_res_master_list_info, 0, 0, 0, sizeof(GLSV_GLND_RSC_MASTER_INFO_LIST), 0,
		 NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLND_RSC_MASTER_INFO_LIST *)0)->rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_GLND_RSC_MASTER_INFO_LIST *)0)->master_dest_id, 0,
		 NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLND_RSC_MASTER_INFO_LIST *)0)->master_status, 0,
		 NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};
	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_GLND_RSC_MASTER_INFO_LIST *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_GLND_RSC_MASTER_INFO_LIST **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_GLND_RSC_MASTER_INFO_LIST));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;

}

 /****************************************************************************
 * Name          : glsv_edp_glnd_evt_rsc_master_info
 *
 * Description   : This is the function which is used to encode decode
 *                 GLSV_EVT_GLND_RSC_MASTER_INFO structure.
 *
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_rsc_master_info(EDU_HDL *edu_hdl,
					       EDU_TKN *edu_tkn,
					       NCSCONTEXT ptr, uint32_t *ptr_data_len,
					       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_GLND_RSC_MASTER_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_rsc_master_info, 0, 0, 0, sizeof(GLSV_EVT_GLND_RSC_MASTER_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_RSC_MASTER_INFO *)0)->no_of_res, 0, NULL},
		{EDU_EXEC, glsv_edp_glnd_res_master_list_info, EDQ_VAR_LEN_DATA, ncs_edp_uns32, 0,
		 (long)&((GLSV_EVT_GLND_RSC_MASTER_INFO *)0)->rsc_master_list,
		 (long)&((GLSV_EVT_GLND_RSC_MASTER_INFO *)0)->no_of_res, NULL},
		{EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_GLND /* Svc-ID */ , NULL, 0,
		 NCS_SERVICE_GLND_SUB_ID_GLND_RES_MASTER_LIST_INFO /* Sub-ID */ , 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_GLND_RSC_MASTER_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_GLND_RSC_MASTER_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_GLND_RSC_MASTER_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_dd_info_list
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_GLND_DD_INFO_LIST structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_dd_info_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					NCSCONTEXT ptr, uint32_t *ptr_data_len,
					EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_GLND_DD_INFO_LIST *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_dd_info_list, EDQ_LNKLIST, 0, 0, sizeof(GLSV_GLND_DD_INFO_LIST), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLND_DD_INFO_LIST *)0)->rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_GLND_DD_INFO_LIST *)0)->blck_dest_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_GLND_DD_INFO_LIST *)0)->blck_hdl_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_GLND_DD_INFO_LIST *)0)->lck_id, 0, NULL},
		{EDU_TEST_LL_PTR, glsv_edp_glnd_dd_info_list, 0, 0, 0, (long)&((GLSV_GLND_DD_INFO_LIST *)0)->next, 0,
		 NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_GLND_DD_INFO_LIST *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_GLND_DD_INFO_LIST **)ptr;
		if (*d_ptr == NULL) {
			/* malloc the memory */
			*d_ptr = (GLSV_GLND_DD_INFO_LIST *)
			    m_MMGR_ALLOC_GLSV_GLND_DD_INFO_LIST(sizeof(GLSV_GLND_DD_INFO_LIST), NCS_SERVICE_ID_GLND);
			if (*d_ptr == NULL) {
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
			memset(*d_ptr, '\0', sizeof(GLSV_GLND_DD_INFO_LIST));
		}
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_dd_probe_info
 *
 * Description   : This is the function which is used to encode decode 
 *                  structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_dd_probe_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					     NCSCONTEXT ptr, uint32_t *ptr_data_len,
					     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_GLND_DD_PROBE_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_dd_probe_info, 0, 0, 0, sizeof(GLSV_EVT_GLND_DD_PROBE_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_DD_PROBE_INFO *)0)->rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_DD_PROBE_INFO *)0)->lcl_rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_GLND_DD_PROBE_INFO *)0)->hdl_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_GLND_DD_PROBE_INFO *)0)->dest_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_EVT_GLND_DD_PROBE_INFO *)0)->lck_id, 0, NULL},
		{EDU_EXEC, glsv_edp_glnd_dd_info_list, EDQ_POINTER, 0, 0,
		 (long)&((GLSV_EVT_GLND_DD_PROBE_INFO *)0)->dd_info_list, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_GLND_DD_PROBE_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_GLND_DD_PROBE_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_GLND_DD_PROBE_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_rsc_gld_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_EVT_GLND_RSC_GLD_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_rsc_gld_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					    NCSCONTEXT ptr, uint32_t *ptr_data_len,
					    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_GLND_RSC_GLD_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_rsc_gld_info, 0, 0, 0, sizeof(GLSV_EVT_GLND_RSC_GLD_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((GLSV_EVT_GLND_RSC_GLD_INFO *)0)->rsc_name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_RSC_GLD_INFO *)0)->rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_GLND_RSC_GLD_INFO *)0)->master_dest_id, 0,
		 NULL},
		{EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((GLSV_EVT_GLND_RSC_GLD_INFO *)0)->can_orphan, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_RSC_GLD_INFO *)0)->orphan_mode, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_RSC_GLD_INFO *)0)->error, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_GLND_RSC_GLD_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_GLND_RSC_GLD_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_GLND_RSC_GLD_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_non_master_info
 *
 * Description   : This is the function which is used to encode decode
 *                 GLND_EVT_GLND_NON_MASTER_STATUS structure.
 *
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_non_master_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					       NCSCONTEXT ptr, uint32_t *ptr_data_len,
					       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLND_EVT_GLND_NON_MASTER_STATUS *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_non_master_info, 0, 0, 0, sizeof(GLND_EVT_GLND_NON_MASTER_STATUS), 0,
		 NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLND_EVT_GLND_NON_MASTER_STATUS *)0)->dest_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLND_EVT_GLND_NON_MASTER_STATUS *)0)->status, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLND_EVT_GLND_NON_MASTER_STATUS *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLND_EVT_GLND_NON_MASTER_STATUS **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLND_EVT_GLND_NON_MASTER_STATUS));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt_new_master_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_EVT_GLND_NEW_MAST_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_glnd_evt_new_master_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					       NCSCONTEXT ptr, uint32_t *ptr_data_len,
					       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_EVT_GLND_NEW_MAST_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_glnd_evt_new_master_info, 0, 0, 0, sizeof(GLSV_EVT_GLND_NEW_MAST_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_NEW_MAST_INFO *)0)->rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_EVT_GLND_NEW_MAST_INFO *)0)->master_dest_id, 0,
		 NULL},
		{EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((GLSV_EVT_GLND_NEW_MAST_INFO *)0)->orphan, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_NEW_MAST_INFO *)0)->orphan_lck_mode, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_EVT_GLND_NEW_MAST_INFO *)0)->status, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_EVT_GLND_NEW_MAST_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_EVT_GLND_NEW_MAST_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_EVT_GLND_NEW_MAST_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_lock_req_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_LOCK_REQ_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t glsv_edp_lock_req_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				    NCSCONTEXT ptr, uint32_t *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_LOCK_REQ_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_create_rules[] = {
		{EDU_START, glsv_edp_lock_req_info, 0, 0, 0, sizeof(GLSV_LOCK_REQ_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_LOCK_REQ_INFO *)0)->lockid, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_LOCK_REQ_INFO *)0)->handleId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_LOCK_REQ_INFO *)0)->invocation, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_LOCK_REQ_INFO *)0)->lock_type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_LOCK_REQ_INFO *)0)->timeout, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_LOCK_REQ_INFO *)0)->lockFlags, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_LOCK_REQ_INFO *)0)->lockStatus, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_LOCK_REQ_INFO *)0)->call_type, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_LOCK_REQ_INFO *)0)->agent_mds_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((GLSV_LOCK_REQ_INFO *)0)->waiter_signal, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_LOCK_REQ_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_LOCK_REQ_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_LOCK_REQ_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_create_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_glnd_evt
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLND event structures.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glsv_edp_glnd_evt(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_GLND_EVT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_glnd_evt_rules[] = {
		{EDU_START, glsv_edp_glnd_evt, 0, 0, 0, sizeof(GLSV_GLND_EVT), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLND_EVT *)0)->type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLND_EVT *)0)->shm_index, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLND_EVT *)0)->type, 0, glsv_glnd_evt_test_type_fnc},

		/* For GLSV_EVT_AGENT_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_agent_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.agent_info, 0, NULL},

		/* For GLSV_EVT_CLIENT_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_client_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.client_info, 0, NULL},

		/* For GLSV_EVT_RESTART_CLIENT_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_restart_client_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.restart_client_info, 0, NULL},

		/* For GLSV_EVT_FINALIZE_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_finalize_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.finalize_info, 0, NULL},

		/* For GLSV_EVT_RSC_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_rsc_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.rsc_info, 0, NULL},

		/* For GLSV_EVT_RSC_LOCK_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_rsc_lock_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.rsc_lock_info, 0, NULL},

		/* For GLSV_EVT_RSC_UNLOCK_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_rsc_unlock_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.rsc_unlock_info, 0, NULL},

		/* For GLSV_EVT_GLND_LCK_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_node_lock_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.node_lck_info, 0, NULL},

		/* For GLSV_EVT_GLND_RSC_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_glnd_rsc_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.node_rsc_info, 0, NULL},

		/* For GLSV_EVT_GLND_DD_PROBE_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_dd_probe_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.dd_probe_info, 0, NULL},

		/* For GLSV_EVT_GLND_RSC_GLD_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_rsc_gld_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.rsc_gld_info, 0, NULL},

		/* For GLSV_EVT_GLND_NEW_MAST_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_new_master_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.new_master_info, 0, NULL},

		/* For GLSV_EVT_GLND_RSC_MASTER_INFO */
		{EDU_EXEC, glsv_edp_glnd_evt_rsc_master_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.rsc_master_info, 0, NULL},

		/* For GLND_EVT_GLND_NON_MASTER_STATUS */
		{EDU_EXEC, glsv_edp_glnd_evt_non_master_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLND_EVT *)0)->info.non_master_info, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_GLND_EVT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_GLND_EVT **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_GLND_EVT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_glnd_evt_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/* GLD Encode/decode utilities begin here */
/****************************************************************************
 * Name          : glsv_gld_evt_test_type_fnc
 *
 * Description   : This is the function which is used to test the event type
 * 
 *
 * Notes         : None.
 *****************************************************************************/
static int glsv_gld_evt_test_type_fnc(NCSCONTEXT arg)
{
	enum {
		LCL_TEST_JUMP_OFFSET_RSC_OPEN_INFO = 1,
		LCL_TEST_JUMP_OFFSET_RSC_DETAILS,
		LCL_TEST_JUMP_OFFSET_GLND_DETAILS
	};
	GLSV_GLD_EVT_TYPE evt_type;

	if (arg == NULL)
		return EDU_FAIL;

	evt_type = *(GLSV_GLD_EVT_TYPE *)arg;
	switch (evt_type) {
	case GLSV_GLD_EVT_RSC_OPEN:
		return LCL_TEST_JUMP_OFFSET_RSC_OPEN_INFO;

	case GLSV_GLD_EVT_RSC_CLOSE:
	case GLSV_GLD_EVT_SET_ORPHAN:
		return LCL_TEST_JUMP_OFFSET_RSC_DETAILS;

	case GLSV_GLD_EVT_GLND_OPERATIONAL:
		return LCL_TEST_JUMP_OFFSET_GLND_DETAILS;

	default:
		return EDU_FAIL;
		break;
	}
}

/****************************************************************************
 * Name          : glsv_edp_gld_evt_rsc_open_info
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_RSC_OPEN_INFO structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glsv_edp_gld_evt_rsc_open_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uint32_t *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_RSC_OPEN_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_gld_create_rules[] = {
		{EDU_START, glsv_edp_gld_evt_rsc_open_info, 0, 0, 0, sizeof(GLSV_RSC_OPEN_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((GLSV_RSC_OPEN_INFO *)0)->rsc_name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_RSC_OPEN_INFO *)0)->rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_RSC_OPEN_INFO *)0)->flag, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_RSC_OPEN_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_RSC_OPEN_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_RSC_OPEN_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_create_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_gld_evt_rsc_details
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLSV_RSC_DETAILS structure.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glsv_edp_gld_evt_rsc_details(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_RSC_DETAILS *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_gld_create_rules[] = {
		{EDU_START, glsv_edp_gld_evt_rsc_details, 0, 0, 0, sizeof(GLSV_RSC_DETAILS), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_RSC_DETAILS *)0)->rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_RSC_DETAILS *)0)->orphan, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_RSC_DETAILS *)0)->lck_mode, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_RSC_DETAILS *)0)->lcl_ref_cnt, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_RSC_DETAILS *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_RSC_DETAILS **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_RSC_DETAILS));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_create_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_gld_evt_gld_node_list
 *
 * Description   :
 *
 *
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glsv_edp_gld_node_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			     NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	GLSV_NODE_LIST *struct_ptr = NULL, **d_ptr = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	EDU_INST_SET glsv_gld_create_rules[] = {
		{EDU_START, glsv_edp_gld_node_list, EDQ_LNKLIST, 0, 0, sizeof(GLSV_NODE_LIST), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_NODE_LIST *)0)->dest_id, 0, NULL},
		{EDU_TEST_LL_PTR, glsv_edp_gld_node_list, 0, 0, 0, (long)&((GLSV_NODE_LIST *)0)->next, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_NODE_LIST *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_NODE_LIST **)ptr;
		if (*d_ptr == NULL) {
			/* malloc the memory */
			if (*d_ptr == NULL) {
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
			memset(*d_ptr, '\0', sizeof(GLSV_NODE_LIST));
		}
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_create_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

uint32_t glsv_edp_gld_evt_node_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uint32_t *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_NODE_LIST *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_gld_create_rules[] = {
		{EDU_START, glsv_edp_gld_evt_node_list, EDQ_LNKLIST, 0, 0, sizeof(GLSV_NODE_LIST), 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_NODE_LIST *)0)->dest_id, 0, NULL},
		{EDU_TEST_LL_PTR, glsv_edp_gld_evt_node_list, 0, 0, 0, (long)&((GLSV_NODE_LIST *)0)->next, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_NODE_LIST *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_NODE_LIST **)ptr;
		if (*d_ptr == NULL) {
			*d_ptr = m_MMGR_ALLOC_GLSV_NODE_LIST;
			if (*d_ptr == NULL) {
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		}
		memset(*d_ptr, '\0', sizeof(GLSV_NODE_LIST));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_create_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;

}

/****************************************************************************
 * Name          : glsv_edp_gld_evt_a2s_node_list
 *
 * Description   : This is the function which is used to encode decode
 *                 GLSV_A2S_NODE_LIST structure.
 *
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glsv_edp_gld_evt_a2s_node_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uint32_t *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_A2S_NODE_LIST *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_gld_create_rules[] = {
		{EDU_START, glsv_edp_gld_evt_a2s_node_list, EDQ_LNKLIST, 0, 0, sizeof(GLSV_A2S_NODE_LIST), 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_A2S_NODE_LIST *)0)->dest_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_A2S_NODE_LIST *)0)->node_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_A2S_NODE_LIST *)0)->status, 0, NULL},
		{EDU_TEST_LL_PTR, glsv_edp_gld_evt_a2s_node_list, 0, 0, 0, (long)&((GLSV_A2S_NODE_LIST *)0)->next, 0,
		 NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_A2S_NODE_LIST *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_A2S_NODE_LIST **)ptr;
		if (*d_ptr == NULL) {
			*d_ptr = m_MMGR_ALLOC_A2S_GLSV_NODE_LIST;
			if (*d_ptr == NULL) {
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		}
		memset(*d_ptr, '\0', sizeof(GLSV_A2S_NODE_LIST));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_create_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;

}

/****************************************************************************
 * Name          : glsv_edp_gld_evt_a2s_rsc_details
 *
 * Description   : This is the function which is used to encode decode
 *                 GLSV_RSC_DETAILS structure.
 *
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t glsv_edp_gld_evt_a2s_rsc_details(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				       NCSCONTEXT ptr, uint32_t *ptr_data_len,
				       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_GLD_A2S_RSC_DETAILS *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_gld_create_rules[] = {
		{EDU_START, glsv_edp_gld_evt_a2s_rsc_details, 0, 0, 0, sizeof(GLSV_GLD_A2S_RSC_DETAILS), 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((GLSV_GLD_A2S_RSC_DETAILS *)0)->resource_name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLD_A2S_RSC_DETAILS *)0)->rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((GLSV_GLD_A2S_RSC_DETAILS *)0)->can_orphan, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLD_A2S_RSC_DETAILS *)0)->orphan_lck_mode, 0, NULL},
		{EDU_EXEC, glsv_edp_gld_evt_a2s_node_list, EDQ_POINTER, 0, 0,
		 (long)&((GLSV_GLD_A2S_RSC_DETAILS *)0)->node_list, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};
	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_GLD_A2S_RSC_DETAILS *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_GLD_A2S_RSC_DETAILS **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_GLD_A2S_RSC_DETAILS));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_create_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_gld_evt_glnd_mds_info
 *
 * Description   : This is the function which is used to encode decode
 *                  GLSV_GLD_GLND_MDS_INFO structure.
 *
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glsv_edp_gld_evt_glnd_mds_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uint32_t *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_GLD_GLND_MDS_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_gld_create_rules[] = {
		{EDU_START, glsv_edp_gld_evt_glnd_mds_info, 0, 0, 0, sizeof(GLSV_GLD_GLND_MDS_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLD_GLND_MDS_INFO *)0)->mds_dest_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_GLD_GLND_MDS_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_GLD_GLND_MDS_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_GLD_GLND_MDS_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_create_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_gld_evt
 *
 * Description   : This is the function which is used to encode decode 
 *                 GLD event structures.
 * 
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glsv_edp_gld_evt(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
		       NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_GLD_EVT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_gld_evt_rules[] = {
		{EDU_START, glsv_edp_gld_evt, 0, 0, 0, sizeof(GLSV_GLD_EVT), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLD_EVT *)0)->evt_type, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_GLD_EVT *)0)->evt_type, 0, glsv_gld_evt_test_type_fnc},

		/* For GLSV_RSC_OPEN_INFO */
		{EDU_EXEC, glsv_edp_gld_evt_rsc_open_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLD_EVT *)0)->info.rsc_open_info, 0, NULL},

		/* For GLSV_RSC_DETAILS */
		{EDU_EXEC, glsv_edp_gld_evt_rsc_details, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLD_EVT *)0)->info.rsc_details, 0, NULL},

		{EDU_EXEC, glsv_edp_gld_evt_glnd_mds_info, 0, 0, EDU_EXIT,
		 (long)&((GLSV_GLD_EVT *)0)->info.glnd_mds_info, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_GLD_EVT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_GLD_EVT **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_GLD_EVT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_evt_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_gld_evt_rsc_open_info
 *
 * Description   : This is the function which is used to encode decode
 *                 GLSV_RSC_OPEN_INFO structure.
 *
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glsv_edp_gld_a2s_evt_rsc_open_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					 NCSCONTEXT ptr, uint32_t *ptr_data_len,
					 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_A2S_RSC_OPEN_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_gld_create_rules[] = {
		{EDU_START, glsv_edp_gld_a2s_evt_rsc_open_info, 0, 0, 0, sizeof(GLSV_A2S_RSC_OPEN_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((GLSV_A2S_RSC_OPEN_INFO *)0)->rsc_name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_A2S_RSC_OPEN_INFO *)0)->rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_A2S_RSC_OPEN_INFO *)0)->mdest_id, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0, (long)&((GLSV_A2S_RSC_OPEN_INFO *)0)->rsc_creation_time, 0,
		 NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};
	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_A2S_RSC_OPEN_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_A2S_RSC_OPEN_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_A2S_RSC_OPEN_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_create_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : glsv_edp_gld_evt_rsc_details
 *
 * Description   : This is the function which is used to encode decode
 *                 GLSV_RSC_DETAILS structure.
 *
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glsv_edp_gld_a2s_evt_rsc_details(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				       NCSCONTEXT ptr, uint32_t *ptr_data_len,
				       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_A2S_RSC_DETAILS *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_gld_create_rules[] = {
		{EDU_START, glsv_edp_gld_a2s_evt_rsc_details, 0, 0, 0, sizeof(GLSV_A2S_RSC_DETAILS), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_A2S_RSC_DETAILS *)0)->rsc_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_A2S_RSC_DETAILS *)0)->orphan, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_A2S_RSC_DETAILS *)0)->lck_mode, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_A2S_RSC_DETAILS *)0)->mdest_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((GLSV_A2S_RSC_DETAILS *)0)->lcl_ref_cnt, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_A2S_RSC_DETAILS *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_A2S_RSC_DETAILS **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_A2S_RSC_DETAILS));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_create_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

uint32_t glsv_edp_gld_a2s_evt_glnd_mds_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					 NCSCONTEXT ptr, uint32_t *ptr_data_len,
					 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLSV_A2S_GLND_MDS_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET glsv_gld_create_rules[] = {
		{EDU_START, glsv_edp_gld_a2s_evt_glnd_mds_info, 0, 0, 0, sizeof(GLSV_A2S_GLND_MDS_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((GLSV_A2S_GLND_MDS_INFO *)0)->mdest_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL}
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (GLSV_A2S_GLND_MDS_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (GLSV_A2S_GLND_MDS_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(GLSV_A2S_GLND_MDS_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, glsv_gld_create_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}
