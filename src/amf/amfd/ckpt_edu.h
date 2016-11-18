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

  This module is the include file for Availability Directors checkpointing.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AMF_AMFD_CKPT_EDU_H_
#define AMF_AMFD_CKPT_EDU_H_

/* Function Definations of avd_ckpt_edu.c */
uint32_t avd_compile_ckpt_edp(AVD_CL_CB *cb);

uint32_t avsv_edp_ckpt_msg_node(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					     NCSCONTEXT ptr, uint32_t *ptr_data_len,
					     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t avsv_edp_ckpt_msg_async_updt_cnt(EDU_HDL *hdl, EDU_TKN *edu_tkn,
						NCSCONTEXT ptr, uint32_t *ptr_data_len,
						EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

#endif  // AMF_AMFD_CKPT_EDU_H_
