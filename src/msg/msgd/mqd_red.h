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

  FILE NAME: mqd_red.h

..............................................................................

  DESCRIPTION:

  MQD Data Structures for Redundancy.

******************************************************************************/

#ifndef MSG_MSGD_MQD_RED_H_
#define MSG_MSGD_MQD_RED_H_

#ifndef MQD_ND_EXPIRY_TIME_STANDBY
#define MQD_ND_EXPIRY_TIME_STANDBY 20000000000LL	/* 20 * (10^9) - 20 seconds */
#endif
#ifndef MAX_NO_MQD_MSGS_A2S
#define MAX_NO_MQD_MSGS_A2S 10
#endif
/* Message type of Active to standby message */
typedef enum mqd_a2s_msg_type {
	MQD_A2S_MSG_TYPE_BASE = 0x00,	/* Base for indexing */
	MQD_A2S_MSG_TYPE_REG,	/* ASAPi Registration Message */
	MQD_A2S_MSG_TYPE_DEREG,	/* ASAPi Deregistration Message */
	MQD_A2S_MSG_TYPE_TRACK,	/* ASAPi Track Message */
	MQD_A2S_MSG_TYPE_QINFO,	/* Cold/Warm Sync Qeue Info Message */
	MQD_A2S_MSG_TYPE_USEREVT,	/* User event to delete the node from tracklist Message */
	MQD_A2S_MSG_TYPE_MQND_STATEVT,	/* MQND is down or up */
	MQD_A2S_MSG_TYPE_MQND_TIMER_EXPEVT,	/* MQND expiry timer got expired */
	MQD_A2S_MSG_TYPE_MAX	/* Max for indexing */
} MQD_A2S_MSG_TYPE;

/*****************************************************************************\
         MQD_A2S_TRACK_INFO for Track Async Update Processing
\*****************************************************************************/
typedef struct mqd_a2s_track_info {
	MDS_DEST dest;		/* Who to notify */
	MDS_SVC_ID to_svc;	/* The service at destination */
	ASAPi_TRACK_INFO track;	/* Track related name and enable flag */
} MQD_A2S_TRACK_INFO;

/*****************************************************************************\
          MQD_A2S_QUEUE_INFO for Cold Sync / Warm Sync Processing 
\*****************************************************************************/
typedef struct mqd_a2s_q_info {
	SaNameT name;		/* Queue name or Group name */
	MQSV_OBJ_TYPE type;	/*Queue or Group */
	union {
		MQD_QUEUE_PARAM q;
		MQD_QGROUP_PARAM qgrp;
	} info;
	uint32_t ilist_cnt;	/* Queue/ Group element list count */
	SaNameT *ilist_info;	/* Queue/ Group element list */
	uint32_t track_cnt;	/* Count of list of users who opted for track */
	MQD_A2S_TRACK_INFO *track_info;	/* List of the users who opted for track */
	SaTimeT creationTime;
} MQD_A2S_QUEUE_INFO;

/*****************************************************************************\
         MQD_A2S_USER_EVENT_INFO for user event processing
\*****************************************************************************/
typedef struct mqd_a2s_user_event_info {
	MDS_DEST dest;
} MQD_A2S_USER_EVENT_INFO;

/*****************************************************************************\
         MQD_A2S_ND_STAT_INFO for MQND is down or up
\*****************************************************************************/
typedef struct mqd_a2s_nd_stat_info {
	NODE_ID nodeid;
	bool is_restarting;
	SaTimeT downtime;
} MQD_A2S_ND_STAT_INFO;

/*****************************************************************************\
       MQD_A2S_ND_TIMER_EXP_INFO for MQND timer got expiredi(permanently down)
\*****************************************************************************/
typedef struct mqd_a2s_nd_timer_exp_info {
	NODE_ID nodeid;
} MQD_A2S_ND_TIMER_EXP_INFO;

/*****************************************************************************\
         MQD_ACTIVE TO STANDBY MESSAGE STRUCTURE 
\*****************************************************************************/
typedef struct mqd_a2s_msg {
	MQD_A2S_MSG_TYPE type;	/* Which message is being sent to standby */
	union {
		ASAPi_REG_INFO reg;	/* ASAPi Register Message */
		ASAPi_DEREG_INFO dereg;	/* ASAPi Deregister Message */
		MQD_A2S_TRACK_INFO track;	/* ASAPi Track Information Message */
		MQD_A2S_QUEUE_INFO qinfo;	/* Cold Sync Queue Info Message */
		MQD_A2S_USER_EVENT_INFO user_evt;	/* User event Message */
		MQD_A2S_ND_STAT_INFO nd_stat_evt;	/* User event Message */
		MQD_A2S_ND_TIMER_EXP_INFO nd_tmr_exp_evt;	/* User event Message */
	} info;
} MQD_A2S_MSG;

void mqd_a2s_async_update(MQD_CB *pMqd, MQD_A2S_MSG_TYPE type, void *pmesg);
uint32_t mqd_mbcsv_register(MQD_CB *pMqd);
uint32_t mqd_mbcsv_finalize(MQD_CB *pMqd);
uint32_t mqd_mbcsv_chgrole(MQD_CB *pMqd);
uint32_t mqd_process_a2s_event(MQD_CB *pMqd, MQD_A2S_MSG *msg);
uint32_t mqd_asapi_db_upd(MQD_CB *, ASAPi_REG_INFO *, MQD_OBJ_NODE **, ASAPi_OBJECT_OPR *);
uint32_t mqd_asapi_dereg_db_upd(MQD_CB *pMqd, ASAPi_DEREG_INFO *dereg, ASAPi_MSG_INFO *msg);
uint32_t mqd_asapi_track_db_upd(MQD_CB *, ASAPi_TRACK_INFO *, MQSV_SEND_INFO *info, MQD_OBJ_NODE **);
uint32_t mqd_user_evt_track_delete(MQD_CB *pMqd, MDS_DEST *dest);

#endif  // MSG_MSGD_MQD_RED_H_
