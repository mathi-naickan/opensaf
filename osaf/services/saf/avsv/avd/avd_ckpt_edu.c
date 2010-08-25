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

#include <avd.h>
#include <avsv.h>
#include <avd_cluster.h>

/*****************************************************************************

  PROCEDURE NAME:   avd_compile_ckpt_edp

  DESCRIPTION:      This function registers all the EDP's require 
                    for checkpointing.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avd_compile_ckpt_edp(AVD_CL_CB *cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	EDU_ERR err = EDU_NORMAL;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_cb, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_node, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_app, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_sg, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_su, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_si, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_comp, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_siass, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_async_updt_cnt, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_sus_per_si_rank, &err);
	if (rc != NCSCC_RC_SUCCESS)
		goto error;

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_ckpt_msg_comp_cs_type, &err);
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

  PROCEDURE NAME:   avsv_edp_ckpt_msg_cb

  DESCRIPTION:      EDU program handler for "AVD_CL_CB" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVD_CL_CB" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avsv_edp_ckpt_msg_cb(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			   NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_CL_CB *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_cb_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_cb, 0, 0, 0, sizeof(AVD_CL_CB), 0, NULL},

		/* AVD Control block information */
		{EDU_EXEC, ncs_edp_int, 0, 0, 0,
		 (long)&((AVD_CL_CB *)0)->init_state, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0,
		 (long)&((AVD_CL_CB *)0)->cluster_init_time, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVD_CL_CB *)0)->nodes_exit_cnt, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVD_CL_CB *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVD_CL_CB **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		/*memset(*d_ptr, '\0', sizeof(AVD_CL_CB)); */
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_cb_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_cluster

  DESCRIPTION:      EDU program handler for "AVD_CLUSTER" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVD_CLUSTER" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avsv_edp_ckpt_msg_cluster(EDU_HDL *hdl, EDU_TKN *edu_tkn,
	NCSCONTEXT ptr, uns32 *ptr_data_len,
	EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_CLUSTER *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_cluster_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_app, 0, 0, 0, sizeof(AVD_CLUSTER), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_CLUSTER *)0)->saAmfClusterAdminState, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVD_CLUSTER *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVD_CLUSTER **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVD_CLUSTER));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_cluster_rules,
		struct_ptr, ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_node

  DESCRIPTION:      EDU program handler for "AVD_AVND" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVD_AVND" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avsv_edp_ckpt_msg_node(EDU_HDL *hdl, EDU_TKN *edu_tkn,
	NCSCONTEXT ptr, uns32 *ptr_data_len,
	EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_AVND *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_node_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_node, 0, 0, 0, sizeof(AVD_AVND), 0, NULL},

		/* Fill here AVD AVND config data structure encoding rules */
		{EDU_EXEC, m_NCS_EDP_SACLMNODEIDT, 0, 0, 0, (long)&((AVD_AVND *)0)->node_info.nodeId, 0, NULL},
		{EDU_EXEC, ncs_edp_saclmnodeaddresst, 0, 0, 0, (long)&((AVD_AVND *)0)->node_info.nodeAddress, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVD_AVND *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_int, 0, 0, 0, (long)&((AVD_AVND *)0)->node_info.member, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SATIMET, 0, 0, 0, (long)&((AVD_AVND *)0)->node_info.bootTimestamp, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAUINT64T, 0, 0, 0, (long)&((AVD_AVND *)0)->node_info.initialViewNumber, 0, NULL},
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((AVD_AVND *)0)->adest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_AVND *)0)->saAmfNodeAdminState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_AVND *)0)->saAmfNodeOperState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_AVND *)0)->node_state, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_AVND *)0)->type, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_AVND *)0)->rcv_msg_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_AVND *)0)->snd_msg_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVD_AVND *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVD_AVND **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVD_AVND));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_node_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_app_config

  DESCRIPTION:      EDU program handler for "AVD_APP" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVD_AVND" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avsv_edp_ckpt_msg_app(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				    NCSCONTEXT ptr, uns32 *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_APP *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_app_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_app, 0, 0, 0, sizeof(AVD_APP), 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVD_APP *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_APP *)0)->saAmfApplicationAdminState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_APP *)0)->saAmfApplicationCurrNumSGs, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVD_APP *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVD_APP **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVD_APP));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_app_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_sg

  DESCRIPTION:      EDU program handler for "AVD_SG" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVD_SG" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avsv_edp_ckpt_msg_sg(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			   NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_SG *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_sg_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_sg, 0, 0, 0, sizeof(AVD_SG), 0, NULL},

		/* AVD SG runtime attribute encoding rules */
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVD_SG *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SG *)0)->saAmfSGAdminState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SG *)0)->saAmfSGNumCurrAssignedSUs, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SG *)0)->saAmfSGNumCurrInstantiatedSpareSUs, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SG *)0)->saAmfSGNumCurrNonInstantiatedSpareSUs, 0, NULL},
		{EDU_EXEC, ncs_edp_int, 0, 0, 0, (long)&((AVD_SG *)0)->adjust_state, 0, NULL},
		{EDU_EXEC, ncs_edp_int, 0, 0, 0, (long)&((AVD_SG *)0)->sg_fsm_state, 0, NULL},

		/*
		 * Rules for Admin SI and SU operation list to be added
		 * here.
		 */
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVD_SG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVD_SG **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVD_SG));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_sg_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_su

  DESCRIPTION:      EDU program handler for "AVD_SU" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVD_SU" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avsv_edp_ckpt_msg_su(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			   NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_SU *struct_ptr = NULL, **d_ptr = NULL;
	uns16 ver_compare = 0;

	ver_compare = AVSV_AVD_AVND_MSG_FMT_VER_2;

	EDU_INST_SET avsv_ckpt_msg_su_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_su, 0, 0, 0, sizeof(AVD_SU), 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVD_SU *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SU *)0)->saAmfSUPreInstantiable, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SU *)0)->saAmfSUOperState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SU *)0)->saAmfSUAdminState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SU *)0)->saAmfSuReadinessState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SU *)0)->saAmfSUPresenceState, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVD_SU *)0)->saAmfSUHostedByNode, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SU *)0)->saAmfSUNumCurrActiveSIs, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SU *)0)->saAmfSUNumCurrStandbySIs, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SU *)0)->saAmfSURestartCount, 0, NULL},

		{EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0, (long)&((AVD_SU *)0)->term_state, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SU *)0)->su_switch, 0, NULL},
		{EDU_EXEC, ncs_edp_int, 0, 0, 0, (long)&((AVD_SU *)0)->su_act_state, 0, NULL},

		{EDU_VER_GE, NULL, 0, 0, 2, 0, 0, (EDU_EXEC_RTINE)((uns16 *)(&(ver_compare)))},

		{EDU_EXEC, ncs_edp_ncs_bool, 0, 0, 0,
		 (long)&((AVD_SU *)0)->su_is_external, 0, NULL},

		/* Fill here AVD SU data structure encoding rules */
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVD_SU *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVD_SU **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVD_SU));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_su_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_si

  DESCRIPTION:      EDU program handler for "AVD_SI" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVD_SI" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avsv_edp_ckpt_msg_si(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			   NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_SI *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_si_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_si, 0, 0, 0, sizeof(AVD_SI), 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVD_SI *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SI *)0)->saAmfSIAdminState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SI *)0)->saAmfSIAssignmentState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SI *)0)->saAmfSINumCurrActiveAssignments, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SI *)0)->saAmfSINumCurrStandbyAssignments, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SI *)0)->si_switch, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVD_SI *)0)->saAmfSIProtectedbySG, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_SI *)0)->alarm_sent, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVD_SI *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVD_SI **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVD_SI));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_si_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_comp

  DESCRIPTION:      EDU program handler for "AVD_COMP" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVD_COMP" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avsv_edp_ckpt_msg_comp(EDU_HDL *hdl, EDU_TKN *edu_tkn,
			     NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_COMP *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_comp_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_comp, 0, 0, 0, sizeof(AVD_COMP), 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVD_COMP *)0)->comp_info.name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_COMP *)0)->saAmfCompOperState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_COMP *)0)->saAmfCompReadinessState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_COMP *)0)->saAmfCompPresenceState, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_COMP *)0)->saAmfCompRestartCount, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVD_COMP *)0)->saAmfCompCurrProxyName, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVD_COMP *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVD_COMP **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVD_COMP));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_comp_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_siass

  DESCRIPTION:      EDU program handler for "AVD_SU_SI_REL" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVD_SU_SI_REL" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avsv_edp_ckpt_msg_siass(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				  NCSCONTEXT ptr, uns32 *ptr_data_len,
				  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVSV_SU_SI_REL_CKPT_MSG *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_su_si_rel_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_siass, 0, 0, 0,
		 sizeof(AVSV_SU_SI_REL_CKPT_MSG), 0, NULL},

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVSV_SU_SI_REL_CKPT_MSG *)0)->su_name, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVSV_SU_SI_REL_CKPT_MSG *)0)->si_name, 0, NULL},
		{EDU_EXEC, m_NCS_EDP_SAAMFHASTATET, 0, 0, 0, (long)&((AVSV_SU_SI_REL_CKPT_MSG *)0)->state, 0, NULL},
		{EDU_EXEC, ncs_edp_int, 0, 0, 0, (long)&((AVSV_SU_SI_REL_CKPT_MSG *)0)->fsm, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVSV_SU_SI_REL_CKPT_MSG *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVSV_SU_SI_REL_CKPT_MSG **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVSV_SU_SI_REL_CKPT_MSG));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_su_si_rel_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_async_updt_cnt

  DESCRIPTION:      EDU program handler for "AVSV_ASYNC_UPDT_CNT" data. This 
                    function is invoked by EDU for performing encode/decode 
                    operation on "AVSV_ASYNC_UPDT_CNT" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/
uns32 avsv_edp_ckpt_msg_async_updt_cnt(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				       NCSCONTEXT ptr, uns32 *ptr_data_len,
				       EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVSV_ASYNC_UPDT_CNT *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_async_updt_cnt_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_async_updt_cnt, 0, 0, 0,
		 sizeof(AVSV_ASYNC_UPDT_CNT), 0, NULL}
		,

		/*
		 * Fill here Async Update data structure encoding rules 
		 */
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->cb_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->node_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->app_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->sg_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->su_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->si_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->sg_su_oprlist_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->sg_admin_si_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->siass_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->comp_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->csi_updt, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVSV_ASYNC_UPDT_CNT *)0)->compcstype_updt, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVSV_ASYNC_UPDT_CNT *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVSV_ASYNC_UPDT_CNT **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVSV_ASYNC_UPDT_CNT));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_async_updt_cnt_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);

	return rc;
}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_sus_per_si_rank

  DESCRIPTION:      EDU program handler for "AVD_SUS_PER_SI_RANK" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "AVD_SUS_PER_SI_RANK" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/

uns32 avsv_edp_ckpt_msg_sus_per_si_rank(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					NCSCONTEXT ptr, uns32 *ptr_data_len,
					EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_SUS_PER_SI_RANK *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_sus_per_si_rank_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_sus_per_si_rank, 0, 0, 0,
		 sizeof(AVD_SUS_PER_SI_RANK), 0, NULL},

		/* Fill here AVD SUS PER SI RANK data structure encoding rules */
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVD_SUS_PER_SI_RANK *)0)->indx.si_name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0,
		 (long)&((AVD_SUS_PER_SI_RANK *)0)->indx.su_rank, 0, NULL},
		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0,
		 (long)&((AVD_SUS_PER_SI_RANK *)0)->su_name, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVD_SUS_PER_SI_RANK *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVD_SUS_PER_SI_RANK **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVD_SUS_PER_SI_RANK));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_sus_per_si_rank_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);

	return rc;

}

/*****************************************************************************

  PROCEDURE NAME:   avsv_edp_ckpt_msg_comp_cs_type

  DESCRIPTION:      EDU program handler for "AVD_COMP_CS_TYPE" data. This
                    function is invoked by EDU for performing encode/decode
                    operation on "AVD_COMP_CS_TYPE" data.

  RETURNS:          NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

*****************************************************************************/

uns32 avsv_edp_ckpt_msg_comp_cs_type(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uns32 *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	AVD_COMPCS_TYPE *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET avsv_ckpt_msg_comp_cs_type_rules[] = {
		{EDU_START, avsv_edp_ckpt_msg_comp_cs_type, 0, 0, 0, sizeof(AVD_COMPCS_TYPE), 0, NULL}	,

		{EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((AVD_COMPCS_TYPE *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_COMPCS_TYPE *)0)->saAmfCompNumCurrActiveCSIs, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((AVD_COMPCS_TYPE *)0)->saAmfCompNumCurrStandbyCSIs, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		struct_ptr = (AVD_COMPCS_TYPE *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		d_ptr = (AVD_COMPCS_TYPE **)ptr;
		if (*d_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(AVD_COMPCS_TYPE));
		struct_ptr = *d_ptr;
	} else {
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(hdl, edu_tkn, avsv_ckpt_msg_comp_cs_type_rules, struct_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;
}

