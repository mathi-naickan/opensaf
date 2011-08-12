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

  This include file contains the message definitions for chepointing data from 
  Active to Standby AVD.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVSV_CKPT_MSG_H
#define AVSV_CKPT_MSG_H

typedef enum avsv_ckpt_msg_reo_type {
	/* 
	 * Messages that update entire data structure and all the 
	 * config fields of that structure. All these message ID's
	 * are also used for sending cold sync updates.
	 */
	AVSV_CKPT_AVD_CB_CONFIG = 0,
	AVSV_CKPT_AVD_CLUSTER_CONFIG = 1,
	AVSV_CKPT_AVD_NODE_CONFIG = 2,
	AVSV_CKPT_AVD_APP_CONFIG = 3,
	AVSV_CKPT_AVD_SG_CONFIG = 4,
	AVSV_CKPT_AVD_SU_CONFIG = 5,
	AVSV_CKPT_AVD_SI_CONFIG = 6,
	AVSV_CKPT_AVD_SG_OPER_SU = 7,
	AVSV_CKPT_AVD_SG_ADMIN_SI = 8,
	AVSV_CKPT_AVD_COMP_CONFIG = 9,
	AVSV_CKPT_AVD_COMP_CS_TYPE_CONFIG = 10,
	AVSV_CKPT_AVD_SI_ASS = 11,
	AVSV_CKPT_AVD_SI_TRANS = 12,
	AVSV_COLD_SYNC_RSP_ASYNC_UPDT_CNT = 13,

	/* Cold sync update reo types are till here */
	AVSV_COLD_SYNC_UPDT_MAX = AVSV_COLD_SYNC_RSP_ASYNC_UPDT_CNT,

	/* 
	 * Messages to update independent fields.
	 */

	/* AVND Async Update messages */
	AVSV_CKPT_AVND_ADMIN_STATE = AVSV_COLD_SYNC_UPDT_MAX,
	AVSV_CKPT_AVND_OPER_STATE,
	AVSV_CKPT_AVND_NODE_UP_INFO,
	AVSV_CKPT_AVND_NODE_STATE,
	AVSV_CKPT_AVND_RCV_MSG_ID,
	AVSV_CKPT_AVND_SND_MSG_ID,

	/* SG Async Update messages */
	AVSV_CKPT_SG_ADMIN_STATE,
	AVSV_CKPT_SG_SU_ASSIGNED_NUM,
	AVSV_CKPT_SG_SU_SPARE_NUM,
	AVSV_CKPT_SG_SU_UNINST_NUM,
	AVSV_CKPT_SG_ADJUST_STATE,
	AVSV_CKPT_SG_FSM_STATE,

	/* SU Async Update messages */
	AVSV_CKPT_SU_PREINSTAN,
	AVSV_CKPT_SU_OPER_STATE,
	AVSV_CKPT_SU_ADMIN_STATE,
	AVSV_CKPT_SU_READINESS_STATE,
	AVSV_CKPT_SU_PRES_STATE,
	AVSV_CKPT_SU_SI_CURR_ACTIVE,
	AVSV_CKPT_SU_SI_CURR_STBY,
	AVSV_CKPT_SU_TERM_STATE,
	AVSV_CKPT_SU_SWITCH,
	AVSV_CKPT_SU_ACT_STATE,

	/* SI Async Update messages */
	AVSV_CKPT_SI_ADMIN_STATE,
	AVSV_CKPT_SI_ASSIGNMENT_STATE,
	AVSV_CKPT_SI_SU_CURR_ACTIVE,
	AVSV_CKPT_SI_SU_CURR_STBY,
	AVSV_CKPT_SI_SWITCH,
	AVSV_CKPT_SI_ALARM_SENT,

	/* COMP Async Update messages */
	AVSV_CKPT_COMP_CURR_PROXY_NAME,
	AVSV_CKPT_COMP_CURR_NUM_CSI_ACTV,
	AVSV_CKPT_COMP_CURR_NUM_CSI_STBY,
	AVSV_CKPT_COMP_OPER_STATE,
	AVSV_CKPT_COMP_READINESS_STATE,
	AVSV_CKPT_COMP_PRES_STATE,
	AVSV_CKPT_COMP_RESTART_COUNT,
	AVSV_SYNC_COMMIT,

	AVSV_CKPT_SU_RESTART_COUNT,
	AVSV_CKPT_MSG_MAX
} AVSV_CKPT_MSG_REO_TYPE;

/* Macros for sending Async updates to standby */
#define m_AVSV_SEND_CKPT_UPDT_ASYNC(cb, act, r_hdl, r_type) \
              avsv_send_ckpt_data(cb, act, NCS_PTR_TO_UNS64_CAST(r_hdl), r_type, NCS_MBCSV_SND_USR_ASYNC)

#define m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(cb, r_hdl, r_type) \
              m_AVSV_SEND_CKPT_UPDT_ASYNC(cb, NCS_MBCSV_ACT_ADD, r_hdl, r_type)

#define m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, r_hdl, r_type) \
              m_AVSV_SEND_CKPT_UPDT_ASYNC(cb, NCS_MBCSV_ACT_RMV, r_hdl, r_type)

#define m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, r_hdl, r_type) \
              m_AVSV_SEND_CKPT_UPDT_ASYNC(cb, NCS_MBCSV_ACT_UPDATE, r_hdl, r_type)

/* Macro for sending Sync update to Standby */
#define m_AVSV_SEND_CKPT_UPDT_SYNC(cb, act, r_hdl) \
              avsv_send_ckpt_data(cb, act, NCS_PTR_TO_UNS64_CAST(r_hdl), AVSV_SYNC_COMMIT, NCS_MBCSV_SND_SYNC)

#endif
