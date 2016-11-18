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

#ifndef MSG_MSGD_MQD_DL_API_H_
#define MSG_MSGD_MQD_DL_API_H_

#include <stdint.h>
#include "amf/saf/saAmf.h"
#include "mqd_db.h"
#include "base/ncsgl_defs.h"
#include "base/ncs_lib.h"

typedef struct mqdlib_info {
	NCSCONTEXT task_hdl;	/* MQD Task Handle */
	uint32_t inst_hdl;		/* MQD Instance Handle */
} MQDLIB_INFO;

uint32_t mqd_lib_req(NCS_LIB_REQ_INFO *);
uint32_t initialize_for_assignment(MQD_CB *cb, SaAmfHAStateT ha_state);

#endif  // MSG_MSGD_MQD_DL_API_H_
