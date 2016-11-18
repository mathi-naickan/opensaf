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

  This file contains common utility routines for managing encode/decode operations
  in AvSv components.
..............................................................................

  FUNCTIONS INCLUDED in this module:
 
  avsv_edp_attr_val
  avsv_edp_csi_attr_info
  avsv_edp_saamfprotectiongroupnotificationbuffert  

******************************************************************************
*/

#include "amf/amf.h"
#include "base/ncs_saf_edu.h"
#include "amf/amf_eduutil.h"
#include "amf/amf_d2nmsg.h"

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_attr_val

  DESCRIPTION:      EDU program handler for "NCS_AVSV_ATTR_NAME_VAL" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "NCS_AVSV_ATTR_NAME_VAL" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t avsv_edp_attr_val(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_ATTR_NAME_VAL *struct_ptr = NULL, **d_ptr = NULL;
	uint16_t base_ver = AVSV_AVD_AVND_MSG_FMT_VER_4;

	EDU_INST_SET avsv_attr_val_rules[] = {
		{EDU_START, avsv_edp_attr_val, 0, 0, 0,
		 sizeof(AVSV_ATTR_NAME_VAL), 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_ATTR_NAME_VAL *)0)->name, 0, NULL},
		{EDU_VER_GE, NULL,   0, 0, 2, 0, 0, (EDU_EXEC_RTINE)((uint16_t *)(&(base_ver)))},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0,
		(long)&((AVSV_ATTR_NAME_VAL *)0)->string_ptr, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_ATTR_NAME_VAL *)0)->value, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVSV_ATTR_NAME_VAL *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVSV_ATTR_NAME_VAL **)ptr;
		if (*d_ptr == NULL) {
			/* Malloc failed!! */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVSV_ATTR_NAME_VAL));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_attr_val_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_csi_attr_info

  DESCRIPTION:      EDU program handler for "NCS_AVSV_CSI_ATTRS" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "NCS_AVSV_CSI_ATTRS" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t avsv_edp_csi_attr_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			     NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_CSI_ATTRS *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_csi_attr_info_rules[] = {
		{EDU_START, avsv_edp_csi_attr_info, 0, 0, 0,
		 sizeof(AVSV_CSI_ATTRS), 0, NULL},

		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_CSI_ATTRS *)0)->number, 0, NULL},
		{EDU_EXEC, avsv_edp_attr_val, EDQ_VAR_LEN_DATA, ncs_edp_uns32, 0,
		 (long)&((AVSV_CSI_ATTRS *)0)->list, (long)&((AVSV_CSI_ATTRS *)0)->number, NULL},
		{EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_AVSV, NULL, 0, AVSV_COMMON_SUB_ID_DEFAULT_VAL, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVSV_CSI_ATTRS *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVSV_CSI_ATTRS **)ptr;
		if (*d_ptr == NULL) {
			/* Malloc failed!! */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVSV_CSI_ATTRS));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_csi_attr_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_saamfprotectiongroupnotificationbuffert

  DESCRIPTION:      EDU program handler for "SaAmfProtectionGroupNotificationBufferT" data.
                    This function is invoked by EDU for performing encode/decode operation
                    on "SaAmfProtectionGroupNotificationBufferT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t avsv_edp_saamfprotectiongroupnotificationbuffert(EDU_HDL *hdl, EDU_TKN *edu_tkn,
						       NCSCONTEXT ptr, uint32_t *ptr_data_len,
						       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAmfProtectionGroupNotificationBufferT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET saamfprotectiongroupnotificationbuffert_rules[] = {
		{EDU_START, avsv_edp_saamfprotectiongroupnotificationbuffert, 0, 0, 0,
		 sizeof(SaAmfProtectionGroupNotificationBufferT), 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT32T, 0, 0, 0,
		 (long)&((SaAmfProtectionGroupNotificationBufferT *)0)->numberOfItems, 0, NULL},
		{EDU_EXEC, ncs_edp_saamfprotectiongroupnotificationt, EDQ_VAR_LEN_DATA, m_NCS_EDP_SAUINT32T, 0,
		 (long)&((SaAmfProtectionGroupNotificationBufferT *)0)->notification,
		 (long)&((SaAmfProtectionGroupNotificationBufferT *)0)->numberOfItems, NULL},
		{EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_AVSV, NULL, 0, AVSV_COMMON_SUB_ID_DEFAULT_VAL, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (SaAmfProtectionGroupNotificationBufferT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (SaAmfProtectionGroupNotificationBufferT **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(SaAmfProtectionGroupNotificationBufferT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, saamfprotectiongroupnotificationbuffert_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}
