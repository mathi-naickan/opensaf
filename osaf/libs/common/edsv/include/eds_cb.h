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

  This file contains EDS_CB and associated definitions.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

*******************************************************************************/
#ifndef EDS_CB_H
#define EDS_CB_H

#include <saClm.h>
#include <saImmOi.h>

#include "ncssysf_tmr.h"

/* global variables */
EXTERN_C uns32 gl_eds_hdl;

struct eda_reg_list_tag;

/*
 * Attribute bits for use in the attrib field of the workList
 */
#define CHANNEL_SYSTEM    (1<<0)	/* Is one of our pre-defined channels */
#define CHANNEL_UNLINKED  (1<<1)	/* Has been unlinked (name invalid) */
#define CHANNEL_RETENTION (1<<2)	/* Has retention timer set */

/* Channel Attribute bits */
#define CHANNEL_PUBLISHER  0x01
#define CHANNEL_SUBSCRIBER 0x02
#define CHANNEL_CREATOR    0x04
#define UNLINKED_CHANNEL   0x08

/* Default HA state assigned locally during eds initialization */
#define EDS_HA_INIT_STATE 0
/* Local limitations on patterns, filters &data */
#define EDS_MAX_NUM_PATTERNS 20
#define EDS_MAX_PATTERN_SIZE 256
#define EDS_MAX_EVENT_DATA_SIZE 1024
#define EDS_MAX_NUM_FILTERS EDS_MAX_NUM_PATTERNS
#define EDS_MAX_FILTER_SIZE EDS_MAX_PATTERN_SIZE
#define EDSV_CLM_TIMEOUT 10000000000LL

typedef enum eds_svc_state {
	RUNNING = 1,
	STOPPED,
	STARTING_UP,
	SHUTTING_DOWN,
	UNAVAILABLE
} EDS_SVC_STATE;

typedef enum eds_tmr_type_tag {
	EDS_TMR_BASE,
	EDS_RET_EVT_TMR = EDS_TMR_BASE,
	EDS_TMR_MAX
} EDS_TMR_TYPE;

/* CHECKPOINT status */
typedef enum checkpoint_status {
	COLD_SYNC_IDLE = 0,
	REG_REC_SENT,
	CHANNEL_REC_SENT,
	SUBSCRIPTION_REC_SENT,
	COLD_SYNC_COMPLETE,
	WARM_SYNC_IDLE,
	WARM_SYNC_CSUM_SENT,
	WARM_SYNC_COMPLETE,
} CHECKPOINT_STATE;

/* EDS Timer definition */
typedef struct eds_tmr_tag {
	tmr_t tmr_id;
	EDS_TMR_TYPE type;	/* timer type */
	uns32 cb_hdl;		/* cb hdl to retrieve the EDS cb ptr */
	uns32 opq_hdl;		/* hdl to retrive the timer context */
	NCS_BOOL is_active;
} EDS_TMR;

typedef struct edsv_retained_evt_list_tag {
	uns32 event_id;		/* From the EDA */

  /** hdl-mgr hdl, to be used for ret timer exp
   ** processing
   **/
	uns32 retd_evt_hdl;

   /** Event details **/
	uns8 priority;
	SaTimeT retentionTime;
	SaTimeT publishTime;
	SaNameT publisherName;
	SaEvtEventPatternArrayT *patternArray;
	SaSizeT data_len;
	uns8 *data;

  /** Fields to help delete the retained event 
   ** once the timer expires.
   **/
	uns32 retd_evt_chan_open_id;
	uns32 reg_id;
	uns32 chan_id;

	/* Retention tmr */
	struct eds_tmr_tag ret_tmr;

	struct edsv_retained_evt_list_tag *next;
} EDS_RETAINED_EVT_REC;

typedef struct subsc_rec_tag {
	uns32 subscript_id;
	uns32 chan_id;
	uns32 chan_open_id;
	SaEvtEventFilterArrayT *filters;
	struct eda_reg_list_tag *reg_list;
	struct chan_open_rec_tag *par_chan_open_inst;	/* Backpointer to the channel open instance */
	struct subsc_rec_tag *prev;
	struct subsc_rec_tag *next;
} SUBSC_REC;

typedef struct subsc_list_tag {
	SUBSC_REC *subsc_rec;
	struct subsc_list_tag *next;
} SUBSC_LIST;

/* This structure is used by regList/Hitlist */
typedef struct chan_open_list_tag {
	uns32 reg_id;
	uns32 chan_id;
	uns32 chan_open_id;	/* Channel Open ID */
	SUBSC_LIST *subsc_list_head;	/* Head of Linked list of subscriptions */
	SUBSC_LIST *subsc_list_tail;	/* Tail of Linked list of subscriptions */
	struct chan_open_list_tag *next;	/* Linked list of channel Open recs */
} CHAN_OPEN_LIST;

/* This structure is used by Worklist */
typedef struct chan_open_rec_tag {
	NCS_PATRICIA_NODE pat_node;
	uns32 reg_id;
	uns32 chan_id;
	uns32 chan_open_id;
	uns32 chan_open_flags;	/* storing the open flags */
	uns32 copen_id_Net;	/* Network order Channel Open ID */
	MDS_DEST chan_opener_dest;
	struct subsc_rec_tag *subsc_rec_head;	/* Head of  Linked list of subscriptions */
	struct subsc_rec_tag *subsc_rec_tail;	/* Tail of  Linked list of subscriptions */
} CHAN_OPEN_REC;

typedef struct eda_reg_list_tag {
	NCS_PATRICIA_NODE pat_node;
	uns32 reg_id;
	uns32 reg_id_Net;
	MDS_DEST eda_client_dest;	/* Handy when an EDA instance goes away */
	CHAN_OPEN_LIST *chan_open_list;	/* channels corresponding to this reg_id only */
} EDA_REG_REC;
typedef struct eds_mib_chan_tbl {
	SaTimeT create_time;
	uns32 num_users;
	uns32 num_subscribers;
	uns32 num_publishers;
	uns32 num_ret_evts;
	uns32 num_lost_evts;
} EDS_CHAN_TBL;


typedef struct eds_worklist_tag {
	uns32 chan_id;
	uns32 last_copen_id;	/* Last assigned chan_open_id */
	uns32 chan_attrib;	/* Attributes of this channel */
	uns32 use_cnt;
	uns16 cname_len;	/* Length of channel name */
	uns8 *cname;		/* Channel name. NULL terminated if ascii */

	/*  Channel runtime info */
	EDS_CHAN_TBL chan_row;

	NCS_PATRICIA_TREE chan_open_rec;	/* Channel Open record - mix of all opens *
						 * on this channel for all reg_ids        */
	EDS_RETAINED_EVT_REC *ret_evt_list_head[SA_EVT_LOWEST_PRIORITY + 1];	/* priority queues head */
	EDS_RETAINED_EVT_REC *ret_evt_list_tail[SA_EVT_LOWEST_PRIORITY + 1];	/* priority queues tail */
	struct eds_worklist_tag *prev;
	struct eds_worklist_tag *next;
} EDS_WORKLIST;

typedef struct eds_cname_list_tag {	/* cname list maintained by EDS for snmp mib requests */
	NCS_PATRICIA_NODE pat_node;
	SaNameT chan_name;
	EDS_WORKLIST *wp_rec;
} EDS_CNAME_REC;

typedef struct eda_down_list_tag {
	MDS_DEST mds_dest;
	struct eda_down_list_tag *next;
} EDA_DOWN_LIST;

/* List of current nodes in the cluster */
typedef struct node_info_tag {
	NCS_PATRICIA_NODE pat_node;
	NODE_ID node_id;
} NODE_INFO;

typedef struct eds_cb_tag {
	SYSF_MBX mbx;		/* EDS's mailbox                             */
	MDS_HDL mds_hdl;	/* PWE Handle for interacting with EDAs      */
	MDS_HDL mds_vdest_hdl;	/* VDEST hdl for global services             */
	V_DEST_RL mds_role;	/* Current MDS role - ACTIVE/STANDBY         */
	uns32 pool_id;		/* Handle Manager pool id                    */
	uns32 my_hdl;		/* Handle Manager hdl                        */
	NCSCONTEXT task_hdl;
	V_DEST_QA my_anc;	/* Meaningful only if this is a VDEST        */
	MDS_DEST vaddr;		/* My identification in MDS                  */
	SaVersionT eds_version;	/* The version currently supported           */
	NCS_PATRICIA_TREE eda_reg_list;	/* EDA Library instantiation list            */
	EDS_WORKLIST *eds_work_list;	/* Master publish/subscribe worklist         */
	NCS_PATRICIA_TREE eds_cname_list;	/* EDS cname tree cname/poniter to worklist node */
	EDA_DOWN_LIST *eda_down_list_head;	/* EDA down reccords - Fix for Failover missed 
						   down events Processing */
	EDA_DOWN_LIST *eda_down_list_tail;
	SaNameT comp_name;	/* Components's name EDS                     */
	SaAmfHandleT amf_hdl;	/* AMF handle, obtained thru AMF init        */
	SaInvocationT amf_invocation_id;	/* AMF InvocationID - needed to handle Quiesed state */
	NCS_BOOL is_quisced_set;
	SaSelectionObjectT amfSelectionObject;	/*Selection Object to wait for amf events */
	NCS_BOOL healthCheckStarted;	/* Flag to check Health Check started or not */
	SaAmfHAStateT ha_state;	/* present AMF HA state of the component     */
	NCS_LOCK cb_lock;	/* Lock for this control Block               */
	uns32 last_reg_id;	/* Value of last reg_id assigned             */
	uns32 async_upd_cnt;	/* Async Update Count for Warmsync */
	CHECKPOINT_STATE ckpt_state;	/* Current record that has been checkpointed */
	NCS_MBCSV_HDL mbcsv_hdl;	/* Handle obtained during mbcsv init */
	SaSelectionObjectT mbcsv_sel_obj;	/* Selection object to wait for MBCSv events */
	NCS_MBCSV_CKPT_HDL mbcsv_ckpt_hdl;	/* MBCSv handle obtained during checkpoint open */
	EDU_HDL edu_hdl;	/* Handle from EDU for encode/decode operations */
	NCS_BOOL csi_assigned;
	NODE_ID node_id;
	SaClmHandleT clm_hdl;	/* CLM handle */
	SaSelectionObjectT clm_sel_obj;	/* Selection object to wait for CLM events */
	NCS_PATRICIA_TREE eds_cluster_nodes_list;
	SaImmOiHandleT immOiHandle;	/* IMM OI Handle */
	SaSelectionObjectT imm_sel_obj;	/* Selection object to wait for IMM events */
} EDS_CB;


#define EDS_INIT_CHAN_RTINFO(wp,chan_create_time) \
      wp->chan_row.create_time=chan_create_time;  \
      wp->chan_row.num_users=0; \
      wp->chan_row.num_subscribers=0; \
      wp->chan_row.num_publishers=0; \
      wp->chan_row.num_ret_evts=0; \
      wp->chan_row.num_lost_evts=0;

EXTERN_C uns32 eds_cb_init(EDS_CB *eds_cb);

EXTERN_C void eds_cb_destroy(EDS_CB *eds_cb);

EXTERN_C void eds_main_process(SYSF_MBX *mbx);

EXTERN_C EDS_WORKLIST *eds_get_worklist_entry(EDS_WORKLIST *, uns32);

EXTERN_C EDA_REG_REC *eds_get_reglist_entry(EDS_CB *, uns32);

EXTERN_C uns32 eds_add_reglist_entry(EDS_CB *, MDS_DEST, uns32);

EXTERN_C uns32 eds_remove_reglist_entry(EDS_CB *, uns32, NCS_BOOL);

EXTERN_C uns32 eds_remove_regid_by_mds_dest(EDS_CB *, MDS_DEST);

EXTERN_C NCS_BOOL eds_eda_entry_valid(EDS_CB *, MDS_DEST);

EXTERN_C uns32 eds_remove_eda_down_rec(EDS_CB *, MDS_DEST);

EXTERN_C uns32 eds_channel_open(EDS_CB *, uns32, uns32, uns16, uns8 *, MDS_DEST, uns32 *, uns32 *, SaTimeT);

EXTERN_C uns32 eds_copen_patricia_init(EDS_WORKLIST *);

EXTERN_C uns32 eds_channel_close(EDS_CB *, uns32, uns32, uns32, NCS_BOOL);

EXTERN_C uns32 eds_channel_unlink(EDS_CB *, uns32, uns8 *);

EXTERN_C uns32 eds_add_subscription(EDS_CB *, uns32, SUBSC_REC *);

EXTERN_C uns32 eds_remove_subscription(EDS_CB *, uns32, uns32, uns32, uns32);

EXTERN_C uns32 eds_remove_worklist_entry(EDS_CB *cb, uns32 chan_id);

EXTERN_C NCS_BOOL eds_pattern_match(SaEvtEventPatternArrayT *, SaEvtEventFilterArrayT *);

EXTERN_C uns32 eds_store_retained_event(EDS_CB *, EDS_WORKLIST *, CHAN_OPEN_REC *, EDSV_EDA_PUBLISH_PARAM *, SaTimeT);

EXTERN_C uns32 eds_clear_retained_event(EDS_CB *, uns32, uns32, uns32, NCS_BOOL);

EXTERN_C void eds_remove_retained_events(EDS_RETAINED_EVT_REC **, EDS_RETAINED_EVT_REC **);

EXTERN_C void eds_dump_event_patterns(SaEvtEventPatternArrayT *);

EXTERN_C void eds_dump_pattern_filter(SaEvtEventPatternArrayT *, SaEvtEventFilterArrayT *);

EXTERN_C void eds_dump_reglist(void);

EXTERN_C void eds_dump_worklist(void);

EXTERN_C uns32 eds_start_tmr(EDS_CB *cb, EDS_TMR *tmr, EDS_TMR_TYPE type, SaTimeT period, uns32 uarg);
EXTERN_C void eds_stop_tmr(EDS_TMR *tmr);
EXTERN_C void eds_tmr_exp(void *uarg);

EXTERN_C SaBoolT update_node_db(EDS_CB *, NODE_ID, SaBoolT);

EXTERN_C void send_clm_status_change(EDS_CB *, SaClmClusterChangesT, NODE_ID);

EXTERN_C NCS_BOOL is_node_a_member(EDS_CB *, NODE_ID);

EXTERN_C EDS_WORKLIST *get_channel_from_worklist(EDS_CB *cb, SaNameT chan_name);
EXTERN_C SaAisErrorT eds_imm_init(EDS_CB *cb);
EXTERN_C void eds_imm_reinit_bg(EDS_CB * cb);
EXTERN_C void eds_imm_declare_implementer(SaImmOiHandleT *OiHandle);

#endif
