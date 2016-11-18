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

 FILE NAME: cpd_init.h

..............................................................................

  DESCRIPTION:
  This file consists of constats, enums and data structs used by cpd_init.c
******************************************************************************/

#ifndef CKPT_CKPTD_CPD_INIT_H_
#define CKPT_CKPTD_CPD_INIT_H_

#include <stdint.h>
#include "amf/saf/saAmf.h"
#include "cpd_cb.h"

uint32_t gl_cpd_cb_hdl;

/* Macro to get the component name for the component type */
#define m_CPD_TASKNAME "CPD"

/* Macro for MQND task priority */
#define m_CPD_TASK_PRI (5)

/* Macro for CPD task stack size */
#define m_CPD_STACKSIZE NCS_STACKSIZE_HUGE

#define m_CPD_STORE_CB_HDL(hdl) (gl_cpd_cb_hdl = hdl)
#define m_CPD_GET_CB_HDL (gl_cpd_cb_hdl)

#define m_CPD_RETRIEVE_CB(cb)                                                  \
{                                                                              \
   uint32_t hdl = m_CPD_GET_CB_HDL;                                               \
   cb = (CPD_CB *)ncshm_take_hdl(NCS_SERVICE_ID_CPD, hdl);                     \
   if(!cb) 									\
	LOG_ER("CPD CB TAKE HDL FAILED ");                                     \
}
#define m_CPD_GIVEUP_CB    ncshm_give_hdl(m_CPD_GET_CB_HDL)

/*****************************************************************************
 * structure which holds the create information.
 *****************************************************************************/
typedef struct cpd_create_info {
	uint8_t pool_id;		/* Handle manager Pool ID */
} CPD_CREATE_INFO;

/*****************************************************************************
 * structure which holds the destroy information.
 *****************************************************************************/
typedef struct cpd_destroy_info {
	uint32_t dummy;
} CPD_DESTROY_INFO;

extern uint32_t initialize_for_assignment(CPD_CB *cb,
					  SaAmfHAStateT ha_state);

#endif  // CKPT_CKPTD_CPD_INIT_H_
