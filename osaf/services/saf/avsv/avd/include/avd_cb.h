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

  This module is the include file for Availability Director's control block.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_CB_H
#define AVD_CB_H

#include <stdbool.h>

#include <saImmOi.h>
#include <saClm.h>

#include <ncssysf_lck.h>
#include <ncspatricia.h>
#include <mds_papi.h>
#include <mbcsv_papi.h>
#include <ncs_edu_pub.h>

#include <avd_ckp.h>
#include <avd_tmr.h>

struct avd_avnd_tag;

typedef enum {
	AVD_INIT_BGN = 1,
	AVD_CFG_DONE,  /* Configuration successfully read from IMM */
	AVD_INIT_DONE, /* OpenSAF SUs assigned on active controller */
	AVD_APP_STATE  /* Cluster startup timer has expired */
} AVD_INIT_STATE;

/*
 * Sync state of the Standby.
 */
typedef enum {
	AVD_STBY_OUT_OF_SYNC,
	AVD_STBY_IN_SYNC,
	AVD_STBY_SYNC_STATE_MAX
} AVD_STBY_SYNC_STATE;

/*
 * Message Queue for holding the AVND messages
 * which we are going to send after processing the
 * entire event.
 */
typedef struct avsv_nd_msg_queue {
	NCSMDS_INFO snd_msg;
	struct avsv_nd_msg_queue *next;
} AVSV_ND_MSG_QUEUE;

typedef struct avsv_nd_msg_list {
	AVSV_ND_MSG_QUEUE *nd_msg_queue;
	AVSV_ND_MSG_QUEUE *tail;
} AVSV_ND_MSG_LIST;

/* For external components, we need to have a list of health check information
   as the health check is assotiated with node. But we wouldn't have health
   check assoiated with any cluster node, so we have to have this information
   somewhere else and so it would be in CB. We also want some information like
   node_state, rcv_msg_id, snd_msg_id of hosting node, so we also have a
   pointer to local AVD_AVND data structure. */
typedef struct avd_ext_comp_info {
	struct avd_avnd_tag *local_avnd_node;
	struct avd_avnd_tag *ext_comp_hlt_check;
} AVD_EXT_COMP_INFO;

/*
 * Message Queue for holding the AVD events
 * during fail over.
 */
typedef struct avd_evt_queue {
	struct avd_evt_tag *evt;
	struct avd_evt_queue *next;
} AVD_EVT_QUEUE;

typedef struct avd_evt_queue_list {
	AVD_EVT_QUEUE *evt_msg_queue;
	AVD_EVT_QUEUE *tail;
} AVD_EVT_QUEUE_LIST;

/* SI-SI dependency database */
typedef struct avd_si_dep {
	NCS_PATRICIA_TREE spons_anchor;	/* Tree of SI-SI dep, sponsor SI acts
					   as a primary key */
	NCS_PATRICIA_TREE dep_anchor;	/* Tree of SI-SI dep, dependent SI 
					   acts as a primary key */
} AVD_SI_DEP;

/* AVD IMM Admin Operation Callback */
typedef struct admin_oper_cbk {
	SaAmfAdminOperationIdT admin_oper;
	SaInvocationT invocation;
} AVD_ADMIN_OPER_CBK;

/* Cluster Control Block (Cl_CB): This data structure lives
 * in the AvD and is the anchor structure for all internal data structures.
 */
typedef struct cl_cb_tag {

	SYSF_MBX avd_mbx;	/* mailbox on which AvD waits */

	/* for HB thread */
	SYSF_MBX avd_hb_mbx;	/* mailbox on which AvD waits */

	NCSCONTEXT mds_handle;	/* The handle returned by MDS 
				 * when initialized
				 */

	AVD_INIT_STATE init_state;	/* when the AvD is comming up for
					 * the first time it will come up
					 * as down in this mode the HA
					 * state for the AvD will be
					 * determined.
					 * Checkpointing - Sent as a one time update.
					 */

	bool avd_fover_state;	/* true if avd is trying to 
					 * recover from the fail-over */

	SaAmfHAStateT avail_state_avd;	/* the redundancy state for 
					 * Availability director
					 */

	MDS_HDL vaddr_pwe_hdl;	/* The pwe handle returned when
				 * vdest address is created.
				 */
	MDS_HDL vaddr_hdl;	/* The handle returned by mds
				 * for vaddr creation
				 */
	MDS_HDL adest_hdl;	/* The handle returned by mds
				 * for adest creation
				 */
	MDS_DEST vaddr;		/* vcard address of the this AvD
				 */
	MDS_DEST other_avd_adest;	/* ADEST of other AvD */
	MDS_DEST local_avnd_adest;	/* ADEST of local AvND */

	/*
	 * Message queue to hold messages to be sent to the ND.
	 * This is a FIFO queue.
	 */
	AVSV_ND_MSG_LIST nd_msg_queue_list;

	/* Event Queue to hold the events during fail-over */
	AVD_EVT_QUEUE_LIST evt_queue;
	/* 
	 * MBCSv related variables.
	 */
	NCS_MBCSV_HDL mbcsv_hdl;
	uint32_t ckpt_hdl;
	SaSelectionObjectT mbcsv_sel_obj;
	AVD_STBY_SYNC_STATE stby_sync_state;

	uint32_t synced_reo_type;	/* Count till which sync is done */
	AVSV_ASYNC_UPDT_CNT async_updt_cnt;	/* Update counts for different async updates */
	bool sync_required;	/* if true, we need to send SYNC send to the standby 
				   after mailbox processing */

	/* Queue for keeping async update messages  on Standby */
	AVSV_ASYNC_UPDT_MSG_QUEUE_LIST async_updt_msgs;

	EDU_HDL edu_hdl;     /* EDU handle used for check pointing */
	EDU_HDL mds_edu_hdl; /* EDU handle used in MDS callbacks */
	SaTimeT cluster_init_time;	/* The time when the firstnode joined the cluster.
					 * Checkpointing - Sent as a one time update.
					 */

	SaClmNodeIdT node_id_avd;	/* node id on which this AvD is  running */
	SaClmNodeIdT node_id_avd_other;	/* node id on which the other AvD is
					 * running 
					 */
	SaClmNodeIdT node_avd_failed;	/* node id where AVD is down */

	NCS_PATRICIA_TREE node_list;	/* Tree of AvND nodes indexed by
					 * node id. used for storing the 
					 * nodes on f-over.
					 */
	AVD_TMR amf_init_tmr;	/* The timer for amf initialisation. */
	AVD_TMR heartbeat_tmr;	/* The timer for sending heart beats to nd. */
	SaTimeT heartbeat_tmr_period;

	uint32_t nodes_exit_cnt;	/* The counter to identifies the number
				   of nodes that have exited the membership
				   since the cluster boot time */

	/********** NTF related params      ***********/
	SaNtfHandleT ntfHandle;

	/********Peer AvD related*********************/
	AVD_EXT_COMP_INFO ext_comp_info;
	uint16_t peer_msg_fmt_ver;
	uint16_t avd_peer_ver;

	/******** IMM related ********/
	SaImmOiHandleT immOiHandle;
	SaImmOiHandleT immOmHandle;
	SaSelectionObjectT imm_sel_obj;
	bool is_implementer;

	/* Clm stuff */
	SaClmHandleT clmHandle;
	SaSelectionObjectT clm_sel_obj;

	// TODO swap_switch: use bool as type and change to a better name!
	SaBoolT swap_switch; /* true - In middle of role switch. */

	/** true when active services (IMM, LOG, NTF, etc.) exist
	 * Set to true when leaving the no-active state
	 * Set to false at switch-over entering the no-active state
	 * Used to skip usage of dependent services in the no-active state
	 */
	bool active_services_exist;

} AVD_CL_CB;

/* macro to push the ND msg in the queue (to the end of the list) */
#define m_AVD_DTOND_MSG_PUSH(cb, msg) \
{ \
   AVSV_ND_MSG_LIST *list = &((cb)->nd_msg_queue_list); \
   if (!(list->nd_msg_queue)) \
       list->nd_msg_queue = (msg); \
   else \
      list->tail->next = (msg); \
   list->tail = (msg); \
}

/* macro to pop the msg (from the beginning of the list) */
#define m_AVD_DTOND_MSG_POP(cb, msg) \
{ \
   AVSV_ND_MSG_LIST *list = &((cb)->nd_msg_queue_list); \
   if (list->nd_msg_queue) { \
      (msg) = list->nd_msg_queue; \
      list->nd_msg_queue = (msg)->next; \
      (msg)->next = 0; \
      if (list->tail == (msg)) list->tail = 0; \
   } else (msg) = 0; \
}

/* macro to enqueue the AVD events in the queue (to the end of the list) */
#define m_AVD_EVT_QUEUE_ENQUEUE(cb, evt) \
{ \
   AVD_EVT_QUEUE_LIST *list = &((cb)->evt_queue); \
   if (!(list->evt_msg_queue)) \
       list->evt_msg_queue = (evt); \
   else \
      list->tail->next = (evt); \
   list->tail = (evt); \
}

/* macro to dequeue the msg (from the beginning of the list) */
#define m_AVD_EVT_QUEUE_DEQUEUE(cb, evt) \
{ \
   AVD_EVT_QUEUE_LIST *list = &((cb)->evt_queue); \
   if (list->evt_msg_queue) { \
      (evt) = list->evt_msg_queue; \
      list->evt_msg_queue = (evt)->next; \
      (evt)->next = 0; \
      if (list->tail == (evt)) list->tail = 0; \
   } else (evt) = 0; \
}

extern AVD_CL_CB *avd_cb;

struct avd_evt_tag;

#endif
