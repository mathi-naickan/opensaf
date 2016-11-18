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
#ifndef AMF_AMFND_AVND_CB_H_
#define AMF_AMFND_AVND_CB_H_
#include <map>


typedef struct avnd_cb_tag {
	SYSF_MBX mbx;		/* mailbox on which AvND waits */

	/* mds params for communication */
	MDS_HDL mds_hdl;	/* mds handle */
	MDS_DEST avnd_dest;	/* AvND mds addr */
	MDS_DEST avd_dest;	/* AvD mds addr */
	bool is_avd_down;	/* Temp: Indicates if AvD went down */
	bool amfd_sync_required;

	/* cb related params */
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
	std::string amf_nodeName;
	SaClmClusterNodeT_4 node_info;    /* this node's info */

	/* database declarations */
	AmfDb<std::string, AVND_SU> sudb;	/* su db */
	AmfDb<std::string, AVND_COMP> compdb;	/* comp db */
	AmfDb<std::string, AVND_HC> hcdb;	/* healthcheck db */
	AmfDb<std::string, AVND_HCTYPE> hctypedb;	/* healthcheck db */
	AmfDb<std::string, AVND_PG> pgdb;	/* pg db */
	AmfDb<NODE_ID, AVND_NODEID_TO_MDSDEST_MAP> nodeid_mdsdest_db;	/* pg db */
	AmfDb<std::string, AVND_COMP> internode_avail_comp_db; /* Internode components, whose node is UP */
	MDS_DEST cntlr_avnd_vdest;	/* Controller AvND Vdest addr */

	/* srmsv resource request mapping list (res mon hdl is the key) */
	NCS_DB_LINK_LIST srm_req_list;

	MDS_DEST active_avd_adest;
	uint32_t rcv_msg_id;	/* Message ID of the last message received */
	/* AvD messaging params (retransmit list etc.) */
	uint32_t snd_msg_id;	/* send msg id */

	/** List of messages sent to director but not yet acked.
	 * Messages are removed when acked with the ACK message.
	 * At director failover the list is scanned handling the
	 * VERIFY message from the director and possibly resent again */
	AVND_DND_LIST dnd_list;

	AVND_TERM_STATE term_state;
	AVND_LED_STATE led_state;

	NCS_DB_LINK_LIST si_list; /* all application SIs sorted by their rank */

	MDS_DEST avnd_mbcsv_vaddr;      /* vcard address of the this AvD */
	uint32_t synced_reo_type;	/* Count till which sync is done */
	SaAmfHAStateT avail_state_avnd;
	bool is_quisced_set;
	NCS_DB_LINK_LIST pid_mon_list;	/* PID list to monitor */

	SaClmHandleT clmHandle;
	SaSelectionObjectT clm_sel_obj;
	bool first_time_up;
	bool reboot_in_progress;
	AVND_SU *failed_su;
	bool cont_reboot_in_progress;

	/* the duration that amfnd should tolerate absence of any SC */
	SaTimeT scs_absence_max_duration;
	/* the timer for supervision of the absence of SC */
	AVND_TMR sc_absence_tmr;
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

void cb_increment_su_failover_count(AVND_CB& cb, const AVND_SU& su);

extern AVND_CB *avnd_cb;
extern std::map<MDS_DEST, MDS_SVC_PVT_SUB_PART_VER> agent_mds_ver_db;

#endif  // AMF_AMFND_AVND_CB_H_
