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
  in AvSv checkpoint messages.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "amf/amfd/amfd.h"
#include "amf/amf.h"
#include "amf/amfd/cluster.h"

/*****************************************************************************

  PROCEDURE NAME:   avd_compile_ckpt_edp

  DESCRIPTION:      This function registers all the EDP's require 
                    for checkpointing.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t avd_compile_ckpt_edp(AVD_CL_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR err = EDU_NORMAL;

	m_NCS_EDU_HDL_INIT(&cb->edu_hdl);

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_async_updt_cnt, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	return rc;

error:
	LOG_ER("%s:%u err=%u", __FUNCTION__, __LINE__, err);
	/* EDU cleanup */
	m_NCS_EDU_HDL_FLUSH(&cb->edu_hdl);
	return rc;
}


/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_async_updt_cnt

  DESCRIPTION:      EDU program handler for "AVSV_ASYNC_UPDT_CNT" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVSV_ASYNC_UPDT_CNT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t avsv_edp_ckpt_msg_async_updt_cnt(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				       NCSCONTEXT ptr, uint32_t *ptr_data_len,
				       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_ASYNC_UPDT_CNT *struct_ptr = nullptr, **d_ptr = nullptr;

	EDU_INST_SET avsv_ckpt_msg_async_updt_cnt_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_async_updt_cnt, 0, 0, 0,
		 sizeof(AVSV_ASYNC_UPDT_CNT), 0, nullptr}
		,

		/*
		 * Fill here Async Update data structure encoding rules 
		 */
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->cb_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->node_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->app_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->sg_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->su_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->si_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->sg_su_oprlist_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->sg_admin_si_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->siass_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->comp_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->csi_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->compcstype_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->si_trans_updt, 0, nullptr},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->ng_updt, 0, nullptr},

		{EDU_END, 0, 0, 0, 0, 0, 0, nullptr},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVSV_ASYNC_UPDT_CNT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVSV_ASYNC_UPDT_CNT **)ptr;
		if (*d_ptr == nullptr) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVSV_ASYNC_UPDT_CNT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = static_cast<AVSV_ASYNC_UPDT_CNT*>(ptr);
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_async_updt_cnt_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);

	return rc;
}
