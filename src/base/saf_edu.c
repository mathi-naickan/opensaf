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

  This file defines EDPs(EDU program) for common SAF data structures.
  
******************************************************************************
*/


#include "base/ncsgl_defs.h"

#include "base/ncspatricia.h"
#include "base/ncsencdec_pub.h"

#include "osaf/saf/saAis.h"
#include "amf/saf/saAmf.h"
#include "clm/saf/saClm.h"
#include "evt/saf/saEvt.h"
#include "lck/saf/saLck.h"

#include "base/saf_mem.h"
#include "base/ncs_edu_pub.h"
#include "base/ncs_saf_edu.h"

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_sanamet

  DESCRIPTION:      EDU program handler for "SaNameT" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "SaNameT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_sanamet(EDU_HDL *hdl, EDU_TKN *edu_tkn,
		      NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	if (op == EDP_OP_TYPE_ENC) {
		const SaNameT *name = ptr;
		osaf_encode_sanamet(buf_env->info.uba, name);
	} else if (op == EDP_OP_TYPE_DEC) {
		SaNameT *name = *(SaNameT**)ptr;
		osaf_decode_sanamet(buf_env->info.uba, name);
	}
	
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_sanamet_net

  DESCRIPTION:      EDU program handler for "SaNameT" data with length field
                    in network order . This function
                    is invoked by EDU for performing encode/decode operation
                    on "SaNameT" data with network order length field.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_sanamet_net(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			  NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaNameT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET saname_rules[] = {
		{EDU_START, ncs_edp_sanamet_net, 0, 0, 0, sizeof(SaNameT), 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, 0,
		 (long)&((SaNameT *)0)->length, 2, NULL},
		{EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, 0,
		 (long)&((SaNameT *)0)->value, SA_MAX_NAME_LENGTH, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (SaNameT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (SaNameT **)ptr;
		if (*d_ptr == NULL) {
			*d_ptr = m_MMGR_ALLOC_EDP_SANAMET;
			if (*d_ptr == NULL) {
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		}
		memset(*d_ptr, '\0', sizeof(SaNameT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, saname_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_saversiont

  DESCRIPTION:      EDU program handler for "SaVersionT" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "SaVersionT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_saversiont(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			 NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaVersionT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET saversiont_rules[] = {
		{EDU_START, ncs_edp_saversiont, 0, 0, 0, sizeof(SaVersionT), 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, 0, 0, 0,
		 (long)&((SaVersionT *)0)->releaseCode, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, 0, 0, 0,
		 (long)&((SaVersionT *)0)->majorVersion, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, 0, 0, 0,
		 (long)&((SaVersionT *)0)->minorVersion, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (SaVersionT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (SaVersionT **)ptr;
		if (*d_ptr == NULL) {
			/* Malloc failed!!!! */
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(SaVersionT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, saversiont_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_saamfhealthcheckkeyt

  DESCRIPTION:      EDU program handler for "SaAmfHealthcheckKeyT" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "SaAmfHealthcheckKeyT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_saamfhealthcheckkeyt(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAmfHealthcheckKeyT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET saamfhealthcheckkeyt_rules[] = {
		{EDU_START, ncs_edp_saamfhealthcheckkeyt, 0, 0, 0,
		 sizeof(SaAmfHealthcheckKeyT), 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT16T, 0, 0, 0,
		 (long)&((SaAmfHealthcheckKeyT *)0)->keyLen, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, EDQ_ARRAY, 0, 0,
		 (long)&((SaAmfHealthcheckKeyT *)0)->key, SA_AMF_HEALTHCHECK_KEY_MAX, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (SaAmfHealthcheckKeyT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (SaAmfHealthcheckKeyT **)ptr;
		if (*d_ptr == NULL) {
			*d_ptr = m_MMGR_ALLOC_EDP_SAAMFHEALTHCHECKKEYT;
			if (*d_ptr == NULL) {
				/* Malloc failed!!!! */
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		}
		memset(*d_ptr, '\0', sizeof(SaAmfHealthcheckKeyT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, saamfhealthcheckkeyt_rules,
				 struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_saf_free_saamfhealthcheckkeyt

  DESCRIPTION:      Utility function to free "SaAmfHealthcheckKeyT" data 
                    structure, when this was malloc'ed by EDU.

  RETURNS:          void

*****************************************************************************/
void  ncs_saf_free_saamfhealthcheckkeyt(SaAmfHealthcheckKeyT *p)
{
	if (p != NULL) {
		m_NCS_MEM_FREE(p->key, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0);
		m_MMGR_FREE_EDP_SAAMFHEALTHCHECKKEYT(p);
	}
	return;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_saclmnodeaddresst

  DESCRIPTION:      EDU program handler for "SaClmNodeAddressT" data. This function
                    is invoked by EDU for performing encode/decode operation
                    on "SaClmNodeAddressT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_saclmnodeaddresst(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uint32_t *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaClmNodeAddressT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET saclmnodeaddresst_rules[] = {
		{EDU_START, ncs_edp_saclmnodeaddresst, 0, 0, 0,
		 sizeof(SaClmNodeAddressT), 0, NULL},
		{EDU_EXEC, ncs_edp_int, 0, 0, 0,
		 (long)&((SaClmNodeAddressT *)0)->family, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT16T, 0, 0, 0,
		 (long)&((SaClmNodeAddressT *)0)->length, 0, NULL},
		{EDU_EXEC, ncs_edp_uns8, EDQ_ARRAY, 0, 0,
		 (long)&((SaClmNodeAddressT *)0)->value, SA_CLM_MAX_ADDRESS_LENGTH, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (SaClmNodeAddressT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (SaClmNodeAddressT **)ptr;
		if (*d_ptr == NULL) {
			*d_ptr = m_MMGR_ALLOC_EDP_SACLMNODEADDRESST;
			if (*d_ptr == NULL) {
				/* Malloc failed!!!! */
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		}
		memset(*d_ptr, '\0', sizeof(SaClmNodeAddressT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, saclmnodeaddresst_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_saamfprotectiongroupmembert

  DESCRIPTION:      EDU program handler for "SaAmfProtectionGroupMemberT" data.
                    This function is invoked by EDU for performing encode/decode operation
                    on "SaAmfProtectionGroupMemberT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_saamfprotectiongroupmembert(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					  NCSCONTEXT ptr, uint32_t *ptr_data_len,
					  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAmfProtectionGroupMemberT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET saamfprotectiongroupmembert_rules[] = {
		{EDU_START, ncs_edp_saamfprotectiongroupmembert, 0, 0, 0,
		 sizeof(SaAmfProtectionGroupMemberT), 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((SaAmfProtectionGroupMemberT *)0)->compName, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFHASTATET, 0, 0, 0,
		 (long)&((SaAmfProtectionGroupMemberT *)0)->haState, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT32T, 0, 0, 0,
		 (long)&((SaAmfProtectionGroupMemberT *)0)->rank, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (SaAmfProtectionGroupMemberT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (SaAmfProtectionGroupMemberT **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(SaAmfProtectionGroupMemberT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, saamfprotectiongroupmembert_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   ncs_edp_saamfprotectiongroupnotificationt

  DESCRIPTION:      EDU program handler for "SaAmfProtectionGroupNotificationT" data.
                    This function is invoked by EDU for performing encode/decode operation
                    on "SaAmfProtectionGroupNotificationT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t ncs_edp_saamfprotectiongroupnotificationt(EDU_HDL *hdl, EDU_TKN *edu_tkn,
						NCSCONTEXT ptr, uint32_t *ptr_data_len,
						EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAmfProtectionGroupNotificationT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET saamfprotectiongroupnotificationt_rules[] = {
		{EDU_START, ncs_edp_saamfprotectiongroupnotificationt, 0, 0, 0,
		 sizeof(SaAmfProtectionGroupNotificationT), 0, NULL},
		{EDU_EXEC, ncs_edp_saamfprotectiongroupmembert, 0, 0, 0,
		 (long)&((SaAmfProtectionGroupNotificationT *)0)->member, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFPROTECTIONGROUPCHANGEST, 0, 0, 0,
		 (long)&((SaAmfProtectionGroupNotificationT *)0)->change, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (SaAmfProtectionGroupNotificationT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (SaAmfProtectionGroupNotificationT **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(SaAmfProtectionGroupNotificationT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, saamfprotectiongroupnotificationt_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}
