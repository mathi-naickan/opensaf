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

  This module is the include file for Availability Node Directors control block.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVND_CB_H
#define AVND_CB_H

/*
 * Sync state of the Standby.
 */
typedef enum {
	AVND_STBY_OUT_OF_SYNC,
	AVND_STBY_IN_SYNC,
	AVND_STBY_SYNC_STATE_MAX
} AVND_STBY_SYNC_STATE;

typedef struct avnd_cb_tag {
	SYSF_MBX mbx;		/* mailbox on which AvND waits */

	/* mds params for communication */
	MDS_HDL mds_hdl;	/* mds handle */
	MDS_DEST avnd_dest;	/* AvND mds addr */
	MDS_DEST avd_dest;	/* AvD mds addr */
	bool is_avd_down;	/* Temp: Indicates if AvD went down */

	/* cb related params */
	NCS_LOCK lock;		/* cb lock */
	NCS_LOCK mon_lock;	/* PID monitor lock */

	/* external interface related params */
	uint8_t pool_id;		/* pool-id used by hdl mngr */
	EDU_HDL edu_hdl;	/* edu handle */
	EDU_HDL edu_hdl_avnd;	/* edu handle for avnd-avnd interface */
	EDU_HDL edu_hdl_ava;	/* edu handle for ava interface */

	/* node states */
	SaAmfAdminStateT admin_state;	/* node admin state */
	SaAmfOperationalStateT oper_state;	/* node oper state  */
	uint32_t flag;		/* cb flag */

	/* timeout interval values */
	SaTimeT cbk_resp_intv;	/* callback response interval */
	SaTimeT msg_resp_intv;	/* AvD message response interval */

	SaTimeT hb_duration;     /* AVD heart beat duration */
	AVND_TMR hb_duration_tmr; /* The timer for supervision of heart beats from avd. */

	/* error recovery escalation params */
	AVND_ERR_ESC_LEVEL node_err_esc_level;	/* curr escalation level of this node */
	SaTimeT su_failover_prob;	/* su failover probation period (config) */
	uint32_t su_failover_max;	/* max SU failovers (config) */
	uint32_t su_failover_cnt;	/* su failover cnt within a probation period */
	AVND_TMR node_err_esc_tmr;	/* node err esc tmr */
        AVSV_AVND_CARD type;    /* node type (scxb or payload) */
	SaNameT amf_nodeName;
        SaClmClusterNodeT_4 node_info;    /* this node's info */

	/* database declarations */
	NCS_PATRICIA_TREE sudb;	/* su db */
	NCS_PATRICIA_TREE compdb;	/* comp db */
	NCS_PATRICIA_TREE hcdb;	/* healthcheck db */
	NCS_PATRICIA_TREE pgdb;	/* pg db */
	NCS_PATRICIA_TREE nodeid_mdsdest_db;	/* pg db */
	NCS_PATRICIA_TREE internode_avail_comp_db; /* Internode components, whose node is UP */
	MDS_DEST cntlr_avnd_vdest;	/* Controller AvND Vdest addr */

	/* srmsv resource request mapping list (res mon hdl is the key) */
	NCS_DB_LINK_LIST srm_req_list;

	MDS_DEST active_avd_adest;
	uint32_t rcv_msg_id;	/* Message ID of the last message received */
	/* AvD messaging params (retransmit list etc.) */
	uint32_t snd_msg_id;	/* send msg id */
	AVND_DND_LIST dnd_list;	/* list of messages sent to AvD */

	AVND_TERM_STATE term_state;
	AVND_LED_STATE led_state;

	NCS_DB_LINK_LIST si_list; /* all application SIs sorted by their rank */

	MDS_HDL avnd_mbcsv_vaddr_pwe_hdl;	/* The pwe handle returned when
						 * vdest address is created.
						 */
	MDS_HDL avnd_mbcsv_vaddr_hdl;	/* The handle returned by mds
					 * for vaddr creation
					 */
	MDS_DEST avnd_mbcsv_vaddr;	/* vcard address of the this AvD                                                 */
	AVND_ASYNC_UPDT_CNT avnd_async_updt_cnt;
	AVND_STBY_SYNC_STATE stby_sync_state;
	uint32_t synced_reo_type;	/* Count till which sync is done */
	NCS_MBCSV_HDL avnd_mbcsv_hdl;
	uint32_t avnd_mbcsv_ckpt_hdl;
	SaSelectionObjectT avnd_mbcsv_sel_obj;
	SaAmfHAStateT avail_state_avnd;
	/* Queue for keeping async update messages  on Standby */
	AVND_ASYNC_UPDT_MSG_QUEUE_LIST async_updt_msgs;
	bool is_quisced_set;
	NCS_DB_LINK_LIST pid_mon_list;	/* PID list to monitor */

	SaClmHandleT clmHandle;
	SaSelectionObjectT clm_sel_obj;
	SaBoolT first_time_up;
	bool reboot_in_progress;
	AVND_SU *failed_su;
	bool cont_reboot_in_progress;
} AVND_CB;

#define AVND_CB_NULL ((AVND_CB *)0)

/* AvD state values */
#define AVND_CB_FLAG_AVD_UP     0x00000001

/* macros for AvD state values */
#define m_AVND_CB_IS_AVD_UP(x)    (((x)->flag) & AVND_CB_FLAG_AVD_UP)
#define m_AVND_CB_AVD_UP_SET(x)   (((x)->flag) |= AVND_CB_FLAG_AVD_UP)
#define m_AVND_CB_AVD_UP_RESET(x) (((x)->flag) &= ~AVND_CB_FLAG_AVD_UP)

/* 
 * Macros for managing the error escalation levels 
 */
#define m_AVND_NODE_ERR_ESC_LEVEL_IS_NONE(x)  \
           ((x)->node_err_esc_level == AVND_ERR_ESC_LEVEL_NONE)

#define m_AVND_NODE_ERR_ESC_LEVEL_IS_3(x)  \
           ((x)->node_err_esc_level == AVND_ERR_ESC_LEVEL_3)

#define m_AVND_NODE_ERR_ESC_LEVEL_SET(x, val)  ((x)->node_err_esc_level = (val))

extern AVND_CB *avnd_cb;

extern AVND_CB *avnd_cb;

void avnd_dnd_list_destroy(AVND_CB *);

uint32_t avnd_cb_destroy(AVND_CB *);

#endif
