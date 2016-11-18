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
  
  MQA database defintion.
    
******************************************************************************
*/

#ifndef MSG_AGENT_MQA_DB_H_
#define MSG_AGENT_MQA_DB_H_

#include "base/ncsgl_defs.h"

extern  uint32_t gl_mqa_hdl;

/*Maximum Nodes in the cluster */
#define MQA_MAX_NODES kMaxSlotsSubslots
typedef unsigned short SVC_SUBPART_VER;

/********************Service Sub part Versions*********************************/

#define MQA_PVT_SUBPART_VERSION 2

/* MQA - MQA */
#define MQA_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT 1
#define MQA_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT 2
#define MQA_WRT_MQA_SUBPART_VER_RANGE \
            (MQA_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT - \
        MQA_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/* MQA - MQND */
#define MQA_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT 1
#define MQA_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT 2
#define MQA_WRT_MQND_SUBPART_VER_RANGE \
            (MQA_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT - \
        MQA_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/* MQA - MQD */
#define MQA_WRT_MQD_SUBPART_VER_AT_MIN_MSG_FMT 1
#define MQA_WRT_MQD_SUBPART_VER_AT_MAX_MSG_FMT 2
#define MQA_WRT_MQD_SUBPART_VER_RANGE \
           (MQA_WRT_MQD_SUBPART_VER_AT_MAX_MSG_FMT - \
        MQA_WRT_MQD_SUBPART_VER_AT_MIN_MSG_FMT + 1)
/********************************************************************************/
typedef struct mqa_send_message_param {
	bool async_flag;
	union {
		SaTimeT timeout;
		SaInvocationT invocation;
	} info;

} MQA_SEND_MESSAGE_PARAM;

typedef struct mqa_tmr_info MQA_TMR_INFO;

typedef struct mqa_track_info {
	NCS_PATRICIA_NODE patnode;
	SaNameT queueGroupName;	/* Index for the tree */
	SaUint8T trackFlags;
	SaMsgQueueGroupNotificationBufferT notificationBuffer;
	uint32_t track_index;
	uint32_t first;
} MQA_TRACK_INFO;

/* Patricia trie of MQA client info structures. */
typedef struct mqa_client_info {
	NCS_PATRICIA_NODE patnode;
	SaMsgHandleT msgHandle;	/* index for the tree */
	SaMsgCallbacksT msgCallbacks;
	SaDispatchFlagsT dispatchFlags;

	/* Mailbox Queue to store the messages for the clients */
	SYSF_MBX callbk_mbx;
	NCS_PATRICIA_TREE mqa_track_tree;
	uint8_t finalize;
	SaVersionT version;

} MQA_CLIENT_INFO;

typedef struct mqa_queue_info {
	NCS_PATRICIA_NODE patnode;
	SaMsgQueueHandleT queueHandle;
	SaMsgQueueHandleT listenerHandle;
	SaMsgQueueOpenFlagsT openFlags;
	uint8_t msg_get_count;
	MQA_CLIENT_INFO *client_info;
	NCSCONTEXT task_handle;
	bool is_closed;	/* Set to true after invoking the close */
} MQA_QUEUE_INFO;

/* Timer node */
typedef struct mqa_tmr_node {
	NCS_QELEM qelem;
	NCS_RP_TMR_HDL tmr_id;
	SaInvocationT invoc;
	MQP_ASYNC_RSP_MSG *callback;
} MQA_TMR_NODE;

/* MQA control block */

typedef struct mqa_cb {

	/* Identification Information about the MQA */
	uint32_t process_id;
	uint32_t agent_handle_id;
	uint8_t pool_id;
	uint32_t mqa_mds_hdl;
	MDS_DEST mqa_mds_dest;
	NCS_LOCK cb_lock;
	EDU_HDL edu_hdl;
	/* To check wheather mqnd and mqds up or not */
	bool is_mqd_up;
	bool is_mqnd_up;
	/* Sender Id information i.e sender context and MDS dest is 
	 * malloced after saMsgMessageGet() if the message needs ack(i.e Reply)
	 * The pointer to this mallcoed structure is passed as senderId in
	 * the messageInfo field of saMsgMessageReply(). The Reply() will
	 * free this senderId. But if the caller does not call Reply() then this
	 * memory is freed by having a timeout mechanism. 
	 * So the mqa_senderid_list keeps all the malloced info to be freed
	 * if timeout occurs.
	 */
	NCS_RP_TMR_HDL mqa_senderid_tmr;
	NCS_QUEUE mqa_senderid_list;

	/* This timer list will be used to lookup running timer and stop it
	 * when an Async response is received.
	 */
	NCS_QUEUE mqa_timer_list;

	NCS_RP_TMR_CB *mqa_tmr_cb;

	/* Information about MQND */
	MDS_DEST mqnd_mds_dest;

	/* Information about MQD */
	MDS_DEST mqd_mds_dest;

	/* MQA data */
	NCS_PATRICIA_TREE mqa_client_tree;	/* MQA_CLIENT_INFO - node */
	NCS_PATRICIA_TREE mqa_queue_tree;	/* MQA_QUEUE_INFO - node */

	/* Sync up with MQND ( MDS ) */
	NCS_LOCK mqnd_sync_lock;
	bool mqnd_sync_awaited;
	NCS_SEL_OBJ mqnd_sync_sel;
	/* Sync up with MQD ( MDS ) */
	NCS_LOCK mqd_sync_lock;
	bool mqd_sync_awaited;
	NCS_SEL_OBJ mqd_sync_sel;

	/*To store versions of MQND across cluster */
	SVC_SUBPART_VER ver_mqnd[MQA_MAX_NODES];
	uint32_t clm_node_joined;
} MQA_CB;

bool mqa_track_tree_find_and_del(MQA_CLIENT_INFO *client_info, SaNameT *group);
bool mqa_is_track_enabled(MQA_CB *mqa_cb, SaNameT *queueGroupName);

#endif  // MSG_AGENT_MQA_DB_H_
