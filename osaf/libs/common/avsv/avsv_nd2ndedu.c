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

#include "avsv.h"
#include "avsv_amfparam.h"
#include "avsv_d2nmsg.h"

/* AVD-AVND common EDPs */
#include "avsv_d2nedu.h"
#include "avsv_eduutil.h"
#include "avsv_n2avaedu.h"
#include "avsv_nd2ndmsg.h"

int avsv_nd_nd_test_type_fnc(NCSCONTEXT arg);

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ndnd_msg

  DESCRIPTION:      EDU program handler for "AVSV_ND2ND_AVND_MSG" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVSV_DND_MSG" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t avsv_edp_ndnd_msg(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_ND2ND_AVND_MSG *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ndnd_msg_rules[] = {
		{EDU_START, avsv_edp_ndnd_msg, 0, 0, 0,
		 sizeof(AVSV_ND2ND_AVND_MSG), 0, NULL},

		{EDU_EXEC, ncs_edp_uns8, 0, 0, 0,
		 (long)&((AVSV_ND2ND_AVND_MSG *)0)->mds_ctxt.length, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, 0,
		 (long)&((AVSV_ND2ND_AVND_MSG *)0)->mds_ctxt.data,
		 MDS_SYNC_SND_CTXT_LEN_MAX, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_ND2ND_AVND_MSG *)0)->comp_name, 0, NULL},

		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ND2ND_AVND_MSG *)0)->type, 0, NULL},

		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ND2ND_AVND_MSG *)0)->type, 0,
		 avsv_nd_nd_test_type_fnc},

		{EDU_EXEC, avsv_edp_nda_msg, EDQ_POINTER, 0, EDU_EXIT,
		 (long)&((AVSV_ND2ND_AVND_MSG *)0)->info.msg, 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_ND2ND_AVND_MSG *)0)->info.cbk_del.comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, EDU_EXIT,
		 (long)&((AVSV_ND2ND_AVND_MSG *)0)->info.cbk_del.opq_hdl, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVSV_ND2ND_AVND_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVSV_ND2ND_AVND_MSG **)ptr;
		if (*d_ptr == NULL) {
			*d_ptr = malloc(sizeof(AVSV_ND2ND_AVND_MSG));
			if (*d_ptr == NULL) {
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		}
		memset(*d_ptr, '\0', sizeof(AVSV_ND2ND_AVND_MSG));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ndnd_msg_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_nd_nd_test_type_fnc

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "avsv_edp_ndnd_msg".

  RETURNS:          uns32, denoting the offset of the next-instruction
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "avsv_edp_ndnd_msg").

*****************************************************************************/
int avsv_nd_nd_test_type_fnc(NCSCONTEXT arg)
{
	enum {
		LCL_JMP_OFFSET_AVSV_N2N_AVA_INFO = 1,
		LCL_JMP_OFFSET_AVSV_N2N_CBK_DEL_INFO = 2,
	};

	AVND_AVND_EVT_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(AVND_AVND_EVT_TYPE *)arg;

	switch (type) {
	case AVND_AVND_AVA_MSG:
		return LCL_JMP_OFFSET_AVSV_N2N_AVA_INFO;
		break;
	case AVND_AVND_CBK_DEL:
		return LCL_JMP_OFFSET_AVSV_N2N_CBK_DEL_INFO;
		break;
	default:
		break;
	}
	return EDU_FAIL;
}
