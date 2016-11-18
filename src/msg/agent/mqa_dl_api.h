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

 _Public_ MQD abstractions and function prototypes

*******************************************************************************/

/*
 * Module Inclusion Control...
 */

#ifndef MSG_AGENT_MQA_DL_API_H_
#define MSG_AGENT_MQA_DL_API_H_

uint32_t mqa_lib_req(NCS_LIB_REQ_INFO *);
unsigned int ncs_mqa_startup(void);
unsigned int ncs_mqa_shutdown(void);

#endif  // MSG_AGENT_MQA_DL_API_H_
