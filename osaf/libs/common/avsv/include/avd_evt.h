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

/* event type enums */
typedef enum avd_evt_type {
	/* all the message events should be in this range */
	AVD_EVT_INVALID = 0,
	AVD_EVT_NODE_UP_MSG,
	AVD_EVT_REG_HLT_MSG,
	AVD_EVT_REG_SU_MSG,
	AVD_EVT_REG_COMP_MSG,
	AVD_EVT_HEARTBEAT_MSG,
	AVD_EVT_OPERATION_STATE_MSG,
	AVD_EVT_INFO_SU_SI_ASSIGN_MSG,
	AVD_EVT_PG_TRACK_ACT_MSG,
	AVD_EVT_OPERATION_REQUEST_MSG,
	AVD_EVT_DATA_REQUEST_MSG,
	AVD_EVT_SHUTDOWN_APP_SU_MSG,
	AVD_EVT_VERIFY_ACK_NACK_MSG,
	AVD_EVT_COMP_VALIDATION_MSG,
	AVD_EVT_MSG_MAX,
	AVD_EVT_TMR_SND_HB = AVD_EVT_MSG_MAX,
	AVD_EVT_TMR_RCV_HB_D,
	AVD_EVT_TMR_RCV_HB_ND,
	AVD_EVT_TMR_RCV_HB_INIT,
	AVD_EVT_TMR_CL_INIT,
	AVD_EVT_TMR_CFG_EXP,
	AVD_EVT_TMR_SI_DEP_TOL,
	AVD_EVT_TMR_MAX,
	AVD_EVT_MIB_REQ = AVD_EVT_TMR_MAX,
	AVD_EVT_MDS_AVD_UP,
	AVD_EVT_MDS_AVD_DOWN,
	AVD_EVT_MDS_AVND_UP,
	AVD_EVT_MDS_AVND_DOWN,
	AVD_EVT_MDS_QSD_ACK,
	AVD_EVT_MDS_MAX,
	AVD_EVT_INIT_CFG_DONE_MSG = AVD_EVT_MDS_MAX,
	AVD_EVT_RESTART,
	AVD_EVT_INIT_MAX,
	AVD_EVT_ND_SHUTDOWN = AVD_EVT_INIT_MAX,
	AVD_EVT_ND_FAILOVER,
	AVD_EVT_FAULT_DMN_RSP,
	AVD_EVT_ND_RESET_RSP,
	AVD_EVT_ND_OPER_ST,
	AVD_EVT_ROLE_CHANGE,
	AVD_EVT_SWITCH_NCS_SU,
	AVD_EVT_D_HB,
	AVD_EVT_SI_DEP_STATE,
	AVD_EVT_MAX
} AVD_EVT_TYPE;

/* AVD top-level event structure */
typedef struct avd_evt_tag {
	AVSV_MBX_MSG next;
	uns32 cb_hdl;
	AVD_EVT_TYPE rcv_evt;

	union {
		AVD_DND_MSG *avnd_msg;
		AVD_D2D_MSG *avd_msg;
		AVD_TMR tmr;
		NCSMIB_ARG *mib_req;
		AVD_BAM_MSG *bam_msg;
		AVM_AVD_MSG_T *avm_msg;
	} info;

} AVD_EVT;

#define AVD_EVT_NULL ((AVD_EVT *)0)

#endif
