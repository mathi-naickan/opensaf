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

  FILE NAME: mqd_db.h

..............................................................................

  DESCRIPTION:
  
  MQD Data Base Structures & Parameters.
    
******************************************************************************/
#ifndef MQD_DB_H
#define MQD_DB_H

/******************** Service Sub part Versions ******************************/
#include <saClm.h>
#include <saImmOi.h>

#define MQSV_MQD_MBCSV_VERSION  1
#define MQSV_MQD_MBCSV_VERSION_MIN 1
#define MQD_PVT_SUBPART_VERSION 2

/* MQD - MQA */
#define MQD_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT 1
#define MQD_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT 2
#define MQD_WRT_MQA_SUBPART_VER_RANGE \
            (MQD_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT - \
        MQD_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/* MQD - MQND */
#define MQD_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT 1
#define MQD_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT 2
#define MQD_WRT_MQND_SUBPART_VER_RANGE \
            (MQD_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT - \
        MQD_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/*****************************************************************************
       Data structure used to hold the Queue Information
*****************************************************************************/

typedef struct mqd_queue_param {
	SaMsgQueueSendingStateT send_state;	/* Sending State */
	SaTimeT retentionTime;	/* Lifetime of the Queue */
	MDS_DEST dest;		/* Queue Destination */
	MQSV_QUEUE_OWN_STATE owner;	/* Orphan/Owned */
	uns32 hdl;		/* Queue Handle */
	uns8 adv;		/* Advertisement flag */
	uns8 is_mqnd_down;	/* TRUE if mqnd is down else FALSE */
	SaMsgQueueCreationFlagsT creationFlags;	/* Queue creation flags */
	SaSizeT size[SA_MSG_MESSAGE_LOWEST_PRIORITY + 1];	/* Priority queue sizes */
} MQD_QUEUE_PARAM;

typedef struct mqd_qgroup_param {
	SaMsgQueueGroupPolicyT policy;
} MQD_QGROUP_PARAM;

/* Tracking info user who subscribed for track */
typedef struct mqd_track_obj {
	NCS_QELEM trkelm;	/* Must be first in struct, Track element */
	MDS_DEST dest;		/* Who to notify */
	MDS_SVC_ID to_svc;	/* The service at the destination */
} MQD_TRACK_OBJ;

/* The queue info, which will go into queue database on MQD */
typedef struct mqd_qinfo {
	SaNameT name;		/* Queue Name or Queue Group Name */
	MQSV_OBJ_TYPE type;	/* Queue / Group */
	union {
		MQD_QUEUE_PARAM q;
		MQD_QGROUP_PARAM qgrp;
	} info;
	NCS_QUEUE ilist;	/* Queue/Group element list */
	NCS_QUEUE tlist;	/* List of the user who opted track */
	SaTimeT creationTime;
} MQD_OBJ_INFO;

typedef struct mqd_object_elem {
	NCS_QELEM qelm;		/* Must be first in struct, Queue element */
	MQD_OBJ_INFO *pObject;	/* Queue/Q-Group Information Node */
} MQD_OBJECT_ELEM;

/* Mtree Node in the Queue Database on MQD */
typedef struct mdq_obj_node {
	NCS_PATRICIA_NODE node;
	MQD_OBJ_INFO oinfo;
} MQD_OBJ_NODE;

typedef struct mqd_node_info {
	NODE_ID nodeid;		/* key for the node database */
	NCS_BOOL is_node_down;      /* TRUE when node left the cluster and FALSE when MQND is down */
	NCS_BOOL is_restarted;	/* TRUE when MQND is up and FALSE when MQND is down */
	SaNameT queue_name;
	MQD_TMR timer;		/* Retention timer for MQND down */
	/*   NCS_BOOL       is_clm_down_processed; */	/*Flag to process the CLM event */
	MDS_DEST dest;		/*Destination MQND which is up/ down */
} MQD_NODE_INFO;

typedef struct mqd_node_db_node {
	NCS_PATRICIA_NODE node;
	MQD_NODE_INFO info;
} MQD_ND_DB_NODE;
/*
typedef enum {
   NODE_DOWN = 1,
   NODE_UP,
} NODE_STATES;
*/
/* MQD Control Block */
typedef struct mqd_cb {
	V_DEST_QA my_anc;	/* Self MDS anchor */
	MDS_HDL my_mds_hdl;	/* Self MDS handle   */
	MDS_DEST my_dest;	/* Self MDS Destination ID */
	NCS_SERVICE_ID my_svc_id;	/* Service ID of the MQD */
	char my_name[MQSV_COMP_NAME_SIZE];	/* Used in AVSv Registrations */

	NCS_PATRICIA_TREE qdb;	/* Queue's Database */
	NCS_BOOL qdb_up;	/* Set to true is qdb is UP */
	uns32 mqd_sync_updt_count;	/*Number of Async Updates to the standby MQD */
	SaNameT record_qindex_name;
	NCS_BOOL cold_or_warm_sync_on;
	NCS_PATRICIA_TREE node_db;
	NCS_BOOL node_db_up;
	SaAmfHandleT amf_hdl;	/* AMF handle, which we would have got during AMF init */
	SaClmHandleT clm_hdl;
	SaAmfHAStateT ha_state;	/* Present AMF HA state of the component */
	SaNameT comp_name;

	NCS_MBCSV_HDL mbcsv_hdl;	/*MBCSV handle for Redundancy of initialize  */
	NCS_MBCSV_CKPT_HDL o_ckpt_hdl;	/*Opened Checkpoint Handle */
	SaSelectionObjectT mbcsv_sel_obj;
	NCS_LOCK mqd_cb_lock;

	SYSF_MBX mbx;		/* Mail box of this Service Part */
	uns32 hdl;		/* CB Struct Handle */
	NCS_BOOL active;	/* Component Active Flag */
	EDU_HDL edu_hdl;	/* Edu Handle */
	uns8 hmpool;		/* Handle Manager Pool ID for this Service Part */

	SaNameT safSpecVer;
	SaNameT safAgtVen;
	uns32 safAgtVenPro;
	NCS_BOOL serv_enabled;
	uns32 serv_state;

	/* For handling the Quisced state */
	SaInvocationT invocation;
	NCS_BOOL is_quisced_set;
	SaImmOiHandleT immOiHandle;	/* IMM OI Handle */
	SaSelectionObjectT imm_sel_obj;	/*Selection object to wait for 
					   IMM events */
} MQD_CB;

#define MQD_CB_NULL  ((MQD_CB *)0)
#define MQD_OBJ_INFO_NULL ((MQD_OBJ_INFO *)0)

EXTERN_C void mqd_db_node_del(MQD_CB *, MQD_OBJ_NODE *);
EXTERN_C uns32 mqd_db_node_add(MQD_CB *, MQD_OBJ_NODE *);
EXTERN_C uns32 mqd_db_node_create(MQD_CB *, MQD_OBJ_NODE **);
EXTERN_C uns32 mqd_timer_expiry_evt_process(MQD_CB *pMqd, NODE_ID *nodeid);
EXTERN_C uns32 mqd_red_db_node_add(MQD_CB *pMqd, MQD_ND_DB_NODE *pNode);
EXTERN_C uns32 mqd_red_db_node_create(MQD_CB *pMqd, MQD_ND_DB_NODE **o_pnode);
EXTERN_C void mqd_red_db_node_del(MQD_CB *pMqd, MQD_ND_DB_NODE *pNode);
EXTERN_C void mqd_qparam_upd(MQD_OBJ_NODE *, ASAPi_QUEUE_PARAM *);
EXTERN_C void mqd_qparam_fill(MQD_QUEUE_PARAM *, ASAPi_QUEUE_PARAM *);

#endif   /* MQD_DB_H */
