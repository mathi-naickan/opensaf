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

  This file lists EDP(EDU program) definitions for Active-Standby MQD  data structures.
  
******************************************************************************
*/

#ifndef MSG_MQSV_MBEDU_H_
#define MSG_MQSV_MBEDU_H_
uint32_t mqsv_edp_mqd_a2s_msg(EDU_HDL *hdl, EDU_TKN *edu_tkn,
				    NCSCONTEXT ptr, uint32_t *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t mqsv_edp_mqd_a2s_queue_info(EDU_HDL *hdl, EDU_TKN *edu_tkn,
					   NCSCONTEXT ptr, uint32_t *ptr_data_len,
					   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

#endif  // MSG_MQSV_MBEDU_H_
