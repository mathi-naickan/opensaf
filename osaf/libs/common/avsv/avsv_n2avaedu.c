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
  in AvSv components AvND and AvA.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  avsv_edp_nda_msg
  avsv_edp_cbq_info
  avsv_edp_api_resp_info
  avsv_edp_api_info
  avsv_nda_msg_test_type_fnc
  avsv_cbq_info_test_type_fnc
  avsv_api_resp_info_test_type_fnc
  avsv_api_info_test_type_fnc
  avsv_ha_test_type_fnc

******************************************************************************
*/
#include "avsv.h"
#include "avsv_amfparam.h"
#include "avsv_n2avamsg.h"
#include "avsv_n2avaedu.h"
#include "avsv_eduutil.h"

static uint32_t avsv_edp_api_resp_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				    NCSCONTEXT ptr, uint32_t *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static uint32_t avsv_edp_api_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			       NCSCONTEXT ptr, uint32_t *ptr_data_len,
			       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

static int avsv_nda_msg_test_type_fnc(NCSCONTEXT arg);

static int avsv_cbq_info_test_type_fnc(NCSCONTEXT arg);

static int avsv_api_resp_info_test_type_fnc(NCSCONTEXT arg);

static int avsv_api_info_test_type_fnc(NCSCONTEXT arg);

static int avsv_ha_test_type_fnc(NCSCONTEXT arg);

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_cbq_info

  DESCRIPTION:      EDU program handler for "AVSV_AMF_CBK_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "AVSV_AMF_CBK_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t avsv_edp_cbq_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_AMF_CBK_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_edp_cbq_info_rules[] = {
		{EDU_START, avsv_edp_cbq_info, 0, 0, 0,
		 sizeof(AVSV_AMF_CBK_INFO), 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->hdl, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAINVOCATIONT, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->inv, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->type, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->type, 0,
		 avsv_cbq_info_test_type_fnc},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.hc.comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_saamfhealthcheckkeyt, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.hc.hc_key, 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.comp_term.comp_name, 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_set.comp_name, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFHASTATET, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_set.ha, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFCSIFLAGST, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_set.csi_desc.csiFlags, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_set.csi_desc.csiName, 0, NULL},
		{EDU_EXEC, avsv_edp_csi_attr_info, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_set.attrs, 0, NULL},

		{EDU_TEST, m_NCS_EDP_SAAMFHASTATET, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_set.ha, 0,
		 avsv_ha_test_type_fnc},
		{EDU_EXEC, m_NCS_EDP_SAAMFCSITRANSITIONDESCRIPTORT, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_set.csi_desc.csiStateDescriptor.activeDescriptor.
		 transitionDescriptor, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_set.csi_desc.csiStateDescriptor.activeDescriptor.
		 activeCompName, 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_set.csi_desc.csiStateDescriptor.standbyDescriptor.
		 activeCompName, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT32T, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_set.csi_desc.csiStateDescriptor.standbyDescriptor.
		 standbyRank, 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_rem.comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_rem.csi_name, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFCSIFLAGST, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.csi_rem.csi_flags, 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.pg_track.csi_name, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT32T, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.pg_track.mem_num, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAERRORT, 0, 0, 0,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.pg_track.err, 0, NULL},
		{EDU_EXEC, avsv_edp_saamfprotectiongroupnotificationbuffert, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.pg_track.buf, 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.pxied_comp_inst.comp_name, 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_CBK_INFO *)0)->param.pxied_comp_clean.comp_name, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVSV_AMF_CBK_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVSV_AMF_CBK_INFO **)ptr;
		if (*d_ptr == NULL) {
			*d_ptr = malloc(sizeof(AVSV_AMF_CBK_INFO));

			if (*d_ptr == NULL) {
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		}
		memset(*d_ptr, '\0', sizeof(AVSV_AMF_CBK_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_edp_cbq_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_api_resp_info 

  DESCRIPTION:      EDU program handler for "AVSV_AMF_API_RESP_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "AVSV_AMF_API_RESP_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t avsv_edp_api_resp_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			     NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_AMF_API_RESP_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_api_resp_info_rules[] = {
		{EDU_START, avsv_edp_api_resp_info, 0, 0, 0,
		 sizeof(AVSV_AMF_API_RESP_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_AMF_API_RESP_INFO *)0)->type, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAERRORT, 0, 0, 0,
		 (long)&((AVSV_AMF_API_RESP_INFO *)0)->rc, 0, NULL},

		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_AMF_API_RESP_INFO *)0)->type, 0,
		 avsv_api_resp_info_test_type_fnc},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_RESP_INFO *)0)->param.ha_get.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_RESP_INFO *)0)->param.ha_get.comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_RESP_INFO *)0)->param.ha_get.csi_name, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFHASTATET, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_RESP_INFO *)0)->param.ha_get.ha, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVSV_AMF_API_RESP_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVSV_AMF_API_RESP_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVSV_AMF_API_RESP_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_api_resp_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_api_info 

  DESCRIPTION:      EDU program handler for "AVSV_AMF_API_INFO" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "AVSV_AMF_API_INFO" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t avsv_edp_api_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_AMF_API_INFO *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_edp_api_info_rules[] = {
		{EDU_START, avsv_edp_api_info, 0, 0, 0,
		 sizeof(AVSV_AMF_API_INFO), 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->type, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->type, 0,
		 avsv_api_info_test_type_fnc},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.finalize.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.finalize.comp_name, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.reg.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.reg.comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.reg.proxy_comp_name, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.unreg.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.unreg.comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.unreg.proxy_comp_name, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_start.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_start.comp_name, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT64T, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_start.pid, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAINT32T, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_start.desc_tree_depth, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFPMERRORST, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_start.pm_err, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFRECOMMENDEDRECOVERYT, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_start.rec_rcvr, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_stop.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_stop.comp_name, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFPMSTOPQUALIFIERT, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_stop.stop_qual, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT64T, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_stop.pid, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFPMERRORST, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pm_start.pm_err, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_start.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_start.comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_start.proxy_comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_saamfhealthcheckkeyt, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_start.hc_key, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFHEALTHCHECKINVOCATIONT, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_start.inv_type, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFRECOMMENDEDRECOVERYT, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_start.rec_rcvr, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_stop.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_stop.comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_stop.proxy_comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_saamfhealthcheckkeyt, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_stop.hc_key, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_confirm.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_confirm.comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_confirm.proxy_comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_saamfhealthcheckkeyt, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_confirm.hc_key, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAERRORT, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.hc_confirm.hc_res, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.csiq_compl.hdl, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAINVOCATIONT, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.csiq_compl.inv, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAERRORT, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.csiq_compl.err, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.csiq_compl.comp_name, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.ha_get.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.ha_get.comp_name, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.ha_get.csi_name, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFHASTATET, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.ha_get.ha, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pg_start.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pg_start.csi_name, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT8T, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pg_start.flags, 0, NULL},
		{EDU_EXEC, ncs_edp_ncs_bool, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pg_start.is_syn, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pg_stop.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.pg_stop.csi_name, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.err_rep.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.err_rep.err_comp, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.err_rep.detect_time, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFRECOMMENDEDRECOVERYT, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.err_rep.rec_rcvr, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.err_clear.hdl, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.err_clear.comp_name, 0, NULL},

		{EDU_EXEC, m_NCS_EDP_SAAMFHANDLET, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.resp.hdl, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAINVOCATIONT, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.resp.inv, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAERRORT, 0, 0, 0,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.resp.err, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, EDU_EXIT,
		 (long)&((AVSV_AMF_API_INFO *)0)->param.resp.comp_name, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVSV_AMF_API_INFO *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVSV_AMF_API_INFO **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVSV_AMF_API_INFO));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_edp_api_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_nda_msg

  DESCRIPTION:      EDU program handler for "AVSV_NDA_AVA_MSG" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "AVSV_NDA_AVA_MSG" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uint32_t avsv_edp_nda_msg(EDU_HDL *hdl, EDU_TKN *edu_tkn,
		       NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_NDA_AVA_MSG *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_nda_msg_rules[] = {
		{EDU_START, avsv_edp_nda_msg, 0, 0, 0,
		 sizeof(AVSV_NDA_AVA_MSG), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_NDA_AVA_MSG *)0)->type, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_NDA_AVA_MSG *)0)->type, 0,
		 avsv_nda_msg_test_type_fnc},

		{EDU_EXEC, avsv_edp_api_info, 0, 0, EDU_EXIT,
		 (long)&((AVSV_NDA_AVA_MSG *)0)->info.api_info, 0, NULL},

		{EDU_EXEC, avsv_edp_cbq_info, EDQ_POINTER, 0, EDU_EXIT,
		 (long)&((AVSV_NDA_AVA_MSG *)0)->info.cbk_info, 0, NULL},

		{EDU_EXEC, avsv_edp_api_resp_info, 0, 0, EDU_EXIT,
		 (long)&((AVSV_NDA_AVA_MSG *)0)->info.api_resp_info, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVSV_NDA_AVA_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVSV_NDA_AVA_MSG **)ptr;
		if (*d_ptr == NULL) {
			*d_ptr = malloc(sizeof(AVSV_NDA_AVA_MSG));
			if (*d_ptr == NULL) {
				*o_err = EDU_ERR_MEM_FAIL;
				return NCSCC_RC_FAILURE;
			}
		}
		memset(*d_ptr, '\0', sizeof(AVSV_NDA_AVA_MSG));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_nda_msg_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_nda_msg_test_type_fnc

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "avsv_edp_nda_msg".

  RETURNS:          uns32, denoting the offset of the next-instruction
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "avsv_edp_nda_msg").

*****************************************************************************/
int avsv_nda_msg_test_type_fnc(NCSCONTEXT arg)
{
	enum {
		LCL_JMP_OFFSET_AVSV_N2A_API_INFO = 1,
		LCL_JMP_OFFSET_AVSV_N2A_CBQ_INFO = 2,
		LCL_JMP_OFFSET_AVSV_N2A_API_RESP_INFO = 3,

	};
	AVSV_NDA_AVA_MSG_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(AVSV_NDA_AVA_MSG_TYPE *)arg;

	switch (type) {
	case AVSV_AVA_API_MSG:
		return LCL_JMP_OFFSET_AVSV_N2A_API_INFO;
		break;

	case AVSV_AVND_AMF_CBK_MSG:
		return LCL_JMP_OFFSET_AVSV_N2A_CBQ_INFO;
		break;

	case AVSV_AVND_AMF_API_RESP_MSG:
		return LCL_JMP_OFFSET_AVSV_N2A_API_RESP_INFO;
		break;

	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_cbq_info_test_type_fnc 

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "avsv_edp_cbq_info".

  RETURNS:          uns32, denoting the offset of the next-instruction
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "avsv_edp_cbq_info").

*****************************************************************************/
int avsv_cbq_info_test_type_fnc(NCSCONTEXT arg)
{
	enum {
		LCL_JMP_OFFSET_AVSV_N2A_HC_CBQ_INFO = 1,
		LCL_JMP_OFFSET_AVSV_N2A_COMP_TERM_INFO = 3,
		LCL_JMP_OFFSET_AVSV_N2A_CSI_SET_INFO = 4,
		LCL_JMP_OFFSET_AVSV_N2A_CSI_REM_INFO = 14,
		LCL_JMP_OFFSET_AVSV_N2A_PG_TRACK_INFO = 17,
		LCL_JMP_OFFSET_AVSV_N2A_PXIED_COMP_INST_INFO = 21,
		LCL_JMP_OFFSET_AVSV_N2A_PXIED_COMP_CLEAN_INFO = 22,

	};
	AVSV_AMF_CBK_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(AVSV_AMF_CBK_TYPE *)arg;

	switch (type) {
	case AVSV_AMF_HC:
		return LCL_JMP_OFFSET_AVSV_N2A_HC_CBQ_INFO;
		break;
	case AVSV_AMF_COMP_TERM:
		return LCL_JMP_OFFSET_AVSV_N2A_COMP_TERM_INFO;
		break;
	case AVSV_AMF_CSI_SET:
		return LCL_JMP_OFFSET_AVSV_N2A_CSI_SET_INFO;
		break;
	case AVSV_AMF_CSI_REM:
		return LCL_JMP_OFFSET_AVSV_N2A_CSI_REM_INFO;
	case AVSV_AMF_PG_TRACK:
		return LCL_JMP_OFFSET_AVSV_N2A_PG_TRACK_INFO;
		break;
	case AVSV_AMF_PXIED_COMP_INST:
		return LCL_JMP_OFFSET_AVSV_N2A_PXIED_COMP_INST_INFO;
		break;
	case AVSV_AMF_PXIED_COMP_CLEAN:
		return LCL_JMP_OFFSET_AVSV_N2A_PXIED_COMP_CLEAN_INFO;
		break;
	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_api_resp_info_test_type_fnc 

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "avsv_edp_api_resp_info".

  RETURNS:          uns32, denoting the offset of the next-instruction
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "avsv_edp_api_resp_info").

*****************************************************************************/
int avsv_api_resp_info_test_type_fnc(NCSCONTEXT arg)
{
	enum {
		LCL_JMP_OFFSET_AVSV_N2A_HA_GET_API_RESP_INFO = 1,
		LCL_JMP_OFFSET_AVSV_N2A_OTHER_API_RESP_INFO = 5,

	};
	AVSV_AMF_API_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(AVSV_AMF_API_TYPE *)arg;

	switch (type) {
	case AVSV_AMF_HA_STATE_GET:
		return LCL_JMP_OFFSET_AVSV_N2A_HA_GET_API_RESP_INFO;
		break;

	case AVSV_AMF_FINALIZE:
	case AVSV_AMF_COMP_REG:
	case AVSV_AMF_COMP_UNREG:
	case AVSV_AMF_PM_START:
	case AVSV_AMF_PM_STOP:
	case AVSV_AMF_HC_START:
	case AVSV_AMF_HC_STOP:
	case AVSV_AMF_HC_CONFIRM:
	case AVSV_AMF_CSI_QUIESCING_COMPLETE:
	case AVSV_AMF_PG_START:
	case AVSV_AMF_PG_STOP:
	case AVSV_AMF_ERR_REP:
	case AVSV_AMF_ERR_CLEAR:
	case AVSV_AMF_RESP:
	case AVSV_AMF_INITIALIZE:
	case AVSV_AMF_SEL_OBJ_GET:
	case AVSV_AMF_DISPATCH:
	case AVSV_AMF_COMP_NAME_GET:
		return LCL_JMP_OFFSET_AVSV_N2A_OTHER_API_RESP_INFO;

	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_api_info_test_type_fnc 

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "avsv_edp_api_info".

  RETURNS:          uns32, denoting the offset of the next-instruction
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "avsv_edp_api_info").

*****************************************************************************/

int avsv_api_info_test_type_fnc(NCSCONTEXT arg)
{
	enum {
		LCL_JMP_OFFSET_AVSV_N2A_FINALIZE_API_INFO = 1,
		LCL_JMP_OFFSET_AVSV_N2A_COMP_REG_API_INFO = 3,
		LCL_JMP_OFFSET_AVSV_N2A_COMP_UNREG_API_INFO = 6,
		LCL_JMP_OFFSET_AVSV_N2A_PM_START_API_INFO = 9,
		LCL_JMP_OFFSET_AVSV_N2A_PM_STOP_API_INFO = 15,
		LCL_JMP_OFFSET_AVSV_N2A_HC_START_API_INFO = 20,
		LCL_JMP_OFFSET_AVSV_N2A_HC_STOP_API_INFO = 26,
		LCL_JMP_OFFSET_AVSV_N2A_HC_CONFIRM_API_INFO = 30,
		LCL_JMP_OFFSET_AVSV_N2A_CSI_QUIESCING_COMPL_API_INFO = 35,
		LCL_JMP_OFFSET_AVSV_N2A_HA_GET_API_INFO = 39,
		LCL_JMP_OFFSET_AVSV_N2A_PG_START_API_INFO = 43,
		LCL_JMP_OFFSET_AVSV_N2A_PG_STOP_API_INFO = 47,
		LCL_JMP_OFFSET_AVSV_N2A_ERR_REP_API_INFO = 49,
		LCL_JMP_OFFSET_AVSV_N2A_ERR_CLEAR_API_INFO = 53,
		LCL_JMP_OFFSET_AVSV_N2A_RESP_API_INFO = 55,
		LCL_JMP_OFFSET_AVSV_N2A_OTHER_API_INFO = 59,

	};
	AVSV_AMF_API_TYPE type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(AVSV_AMF_API_TYPE *)arg;

	switch (type) {
	case AVSV_AMF_FINALIZE:
		return LCL_JMP_OFFSET_AVSV_N2A_FINALIZE_API_INFO;
		break;

	case AVSV_AMF_COMP_REG:
		return LCL_JMP_OFFSET_AVSV_N2A_COMP_REG_API_INFO;
		break;

	case AVSV_AMF_COMP_UNREG:
		return LCL_JMP_OFFSET_AVSV_N2A_COMP_UNREG_API_INFO;
		break;

	case AVSV_AMF_PM_START:
		return LCL_JMP_OFFSET_AVSV_N2A_PM_START_API_INFO;
		break;

	case AVSV_AMF_PM_STOP:
		return LCL_JMP_OFFSET_AVSV_N2A_PM_STOP_API_INFO;
		break;

	case AVSV_AMF_HC_START:
		return LCL_JMP_OFFSET_AVSV_N2A_HC_START_API_INFO;
		break;

	case AVSV_AMF_HC_STOP:
		return LCL_JMP_OFFSET_AVSV_N2A_HC_STOP_API_INFO;
		break;

	case AVSV_AMF_HC_CONFIRM:
		return LCL_JMP_OFFSET_AVSV_N2A_HC_CONFIRM_API_INFO;
		break;

	case AVSV_AMF_CSI_QUIESCING_COMPLETE:
		return LCL_JMP_OFFSET_AVSV_N2A_CSI_QUIESCING_COMPL_API_INFO;
		break;

	case AVSV_AMF_HA_STATE_GET:
		return LCL_JMP_OFFSET_AVSV_N2A_HA_GET_API_INFO;
		break;

	case AVSV_AMF_PG_START:
		return LCL_JMP_OFFSET_AVSV_N2A_PG_START_API_INFO;
		break;

	case AVSV_AMF_PG_STOP:
		return LCL_JMP_OFFSET_AVSV_N2A_PG_STOP_API_INFO;
		break;

	case AVSV_AMF_ERR_REP:
		return LCL_JMP_OFFSET_AVSV_N2A_ERR_REP_API_INFO;
		break;

	case AVSV_AMF_ERR_CLEAR:
		return LCL_JMP_OFFSET_AVSV_N2A_ERR_CLEAR_API_INFO;
		break;

	case AVSV_AMF_RESP:
		return LCL_JMP_OFFSET_AVSV_N2A_RESP_API_INFO;
		break;

	case AVSV_AMF_INITIALIZE:
	case AVSV_AMF_SEL_OBJ_GET:
	case AVSV_AMF_DISPATCH:
	case AVSV_AMF_COMP_NAME_GET:
		return LCL_JMP_OFFSET_AVSV_N2A_OTHER_API_INFO;

	default:
		break;
	}

	return EDU_FAIL;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_ha_test_type_fnc 

  DESCRIPTION:      Test function, that gets invoked from the EDP
                    function "avsv_edp_cbq_info".

  RETURNS:          uns32, denoting the offset of the next-instruction
                    to be executed(relative to the EDU_TEST instruction
                    defined in the EDP "avsv_edp_cbq_info").

*****************************************************************************/

int avsv_ha_test_type_fnc(NCSCONTEXT arg)
{
	enum {
		LCL_JMP_OFFSET_AVSV_HA_STATE_ACTIVE = 1,
		LCL_JMP_OFFSET_AVSV_HA_STATE_STANDBY = 3,
		LCL_JMP_OFFSET_AVSV_HA_STATE_OTHERS = 14,

	};
	SaAmfHAStateT type;

	if (arg == NULL)
		return EDU_FAIL;

	type = *(SaAmfHAStateT *)arg;

	switch (type) {
	case SA_AMF_HA_ACTIVE:
		return LCL_JMP_OFFSET_AVSV_HA_STATE_ACTIVE;
		break;

	case SA_AMF_HA_STANDBY:
		return LCL_JMP_OFFSET_AVSV_HA_STATE_STANDBY;
		break;

	case SA_AMF_HA_QUIESCED:
	case SA_AMF_HA_QUIESCING:
		return LCL_JMP_OFFSET_AVSV_HA_STATE_OTHERS;

	default:
		break;
	}

	return EDU_FAIL;
}
