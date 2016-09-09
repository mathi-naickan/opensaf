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

  This module is the  include file for Availability Directors event
  structures.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_EVT_H
#define AVD_EVT_H

#include <util.h>

/* event type enums */
typedef enum avd_evt_type {
	/* all the message events should be in this range */
	AVD_EVT_INVALID = 0,
	AVD_EVT_NODE_UP_MSG,
	AVD_EVT_REG_SU_MSG,
	AVD_EVT_REG_COMP_MSG,
	AVD_EVT_OPERATION_STATE_MSG,
	AVD_EVT_INFO_SU_SI_ASSIGN_MSG,
	AVD_EVT_PG_TRACK_ACT_MSG,
	AVD_EVT_OPERATION_REQUEST_MSG,
	AVD_EVT_DATA_REQUEST_MSG,
	AVD_EVT_SHUTDOWN_APP_SU_MSG,
	AVD_EVT_VERIFY_ACK_NACK_MSG,
	AVD_EVT_COMP_VALIDATION_MSG,
	AVD_EVT_ND_SISU_STATE_INFO_MSG,
	AVD_EVT_ND_CSICOMP_STATE_INFO_MSG,
	AVD_EVT_MSG_MAX,
	AVD_EVT_TMR_SND_HB = AVD_EVT_MSG_MAX,
	AVD_EVT_TMR_CL_INIT,
	AVD_EVT_TMR_SI_DEP_TOL,
	AVD_EVT_TMR_NODE_SYNC,
	AVD_EVT_TMR_MAX,
	AVD_EVT_MDS_AVD_UP = AVD_EVT_TMR_MAX,
	AVD_EVT_MDS_AVD_DOWN,
	AVD_EVT_MDS_AVND_UP,
	AVD_EVT_MDS_AVND_DOWN,
	AVD_EVT_MDS_QSD_ACK,
	AVD_EVT_MDS_MAX,
	AVD_EVT_ROLE_CHANGE = AVD_EVT_MDS_MAX,
	AVD_EVT_SWITCH_NCS_SU,
	AVD_EVT_ASSIGN_SI_DEP_STATE,
	AVD_IMM_REINITIALIZED,
	AVD_EVT_UNASSIGN_SI_DEP_STATE,
	AVD_EVT_ND_MDS_VER_INFO,
	AVD_EVT_MAX
} AVD_EVT_TYPE;

struct AVD_EVT_ND_MDS_INFO {
	SaClmNodeIdT node_id;
	MDS_SVC_PVT_SUB_PART_VER mds_version;
};
union AVD_EVT_INFO {
	AVD_DND_MSG *avnd_msg;
	AVD_D2D_MSG *avd_msg;
	SaClmNodeIdT node_id;
	AVD_TMR tmr;
	AVD_EVT_ND_MDS_INFO nd_mds_info;
	AVD_EVT_INFO() {new(&tmr) AVD_TMR();}
	~AVD_EVT_INFO() {tmr.~AVD_TMR();}
};

/* AVD top-level event structure */
struct AVD_EVT {
	NCS_IPC_MSG next;
	AVD_EVT_TYPE rcv_evt;
	AVD_EVT_INFO info;
};

#define AVD_EVT_NULL ((AVD_EVT *)0)

#endif
