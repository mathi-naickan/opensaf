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

  GLND EDU related definitions.
  
******************************************************************************
*/

#ifndef LCK_LCKD_GLD_EDU_H_
#define LCK_LCKD_GLD_EDU_H_

/*** Extern function declarations ***/

uint32_t glsv_edp_gld_evt(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uint32_t *ptr_data_len,
				EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_glnd_evt(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uint32_t *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_gld_evt_rsc_open_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					      NCSCONTEXT ptr, uint32_t *ptr_data_len,
					      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_gld_a2s_evt_rsc_open_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
						  NCSCONTEXT ptr, uint32_t *ptr_data_len,
						  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_gld_evt_rsc_details(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					    NCSCONTEXT ptr, uint32_t *ptr_data_len,
					    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_gld_a2s_evt_rsc_details(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
						NCSCONTEXT ptr, uint32_t *ptr_data_len,
						EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_gld_node_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					  NCSCONTEXT ptr, uint32_t *ptr_data_len,
					  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_gld_evt_node_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					  NCSCONTEXT ptr, uint32_t *ptr_data_len,
					  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_gld_evt_a2s_node_list(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					      NCSCONTEXT ptr, uint32_t *ptr_data_len,
					      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_gld_evt_mds_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					 NCSCONTEXT ptr, uint32_t *ptr_data_len,
					 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_gld_evt_glnd_mds_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
					      NCSCONTEXT ptr, uint32_t *ptr_data_len,
					      EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t glsv_edp_gld_a2s_evt_glnd_mds_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
						  NCSCONTEXT ptr, uint32_t *ptr_data_len,
						  EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t glsv_edp_gld_evt_a2s_rsc_details(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
						NCSCONTEXT ptr, uint32_t *ptr_data_len,
						EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

#endif  // LCK_LCKD_GLD_EDU_H_
