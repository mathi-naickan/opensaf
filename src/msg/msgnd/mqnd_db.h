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
  
  MQSv event definitions.
    
******************************************************************************
*/

#ifndef MSG_MSGND_MQND_DB_H_
#define MSG_MSGND_MQND_DB_H_

#include "clm/saf/saClm.h"
#include "imm/saf/saImmOi.h"
/* Decleration for global variable */
uint32_t gl_mqnd_cb_hdl;

/* Macros for reading global database */
#define m_MQND_STORE_HDL(hdl) (gl_mqnd_cb_hdl = (hdl))
#define m_MQND_GET_HDL( ) (gl_mqnd_cb_hdl)

#define MQND_LISTENER_QUEUE_SIZE 10000

/***************************Service Sub part Versions***************************/

#define MQND_PVT_SUBPART_VERSION 2
/* MQND - MQA */
#define MQND_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT 1
#define MQND_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT 2
#define MQND_WRT_MQA_SUBPART_VER_RANGE \
            (MQND_WRT_MQA_SUBPART_VER_AT_MAX_MSG_FMT - \
        MQND_WRT_MQA_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/* MQND - MQD */
#define MQND_WRT_MQD_SUBPART_VER_AT_MIN_MSG_FMT 1
#define MQND_WRT_MQD_SUBPART_VER_AT_MAX_MSG_FMT 2
#define MQND_WRT_MQD_SUBPART_VER_RANGE \
            (MQND_WRT_MQD_SUBPART_VER_AT_MAX_MSG_FMT - \
        MQND_WRT_MQD_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/* MQND - MQND */
#define MQND_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT 1
#define MQND_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT 2
#define MQND_WRT_MQND_SUBPART_VER_RANGE \
            (MQND_WRT_MQND_SUBPART_VER_AT_MAX_MSG_FMT - \
        MQND_WRT_MQND_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/**********************************************************************************/

/* Q information stored in MQND */
typedef struct mqnd_queue_info {
	SaMsgQueueHandleT queueHandle;	/* Assigned by MQND */
	SaMsgQueueHandleT listenerHandle;	/* Listener queue handle */
	SaNameT queueName;
	SaSizeT size[SA_MSG_MESSAGE_LOWEST_PRIORITY + 1];	/* Size of the queue */
	SaMsgQueueStatusT queueStatus;	/* Status info of the queue */
	SaMsgQueueSendingStateT sendingState;	/* Sending state is removed from B.1.1, but used internally */
	uint8_t owner_flag;	/* Orphan or Owned */
	SaMsgHandleT msgHandle;	/* Message hdl of receiver application */
	MDS_DEST rcvr_mqa;	/* Agent info of Receiver */

	/* Current message queue implimentation requires the unique key
	   for creating, accessing  & destroying queue */
	NCS_OS_MQ_KEY q_key;	/* key that uniquely identify the msg queue */

	MQND_TMR tmr;		/* Retention Timer */
	MQND_TMR qtransfer_complete_tmr;	/* Q Transfer Complete Timer */
	SaTimeT creationTime;	/* Queue Creation time */
	uint32_t numberOfFullErrors[SA_MSG_MESSAGE_LOWEST_PRIORITY + 1];
	SaUint64T totalQueueSize;
	uint32_t shm_queue_index;
} MQND_QUEUE_INFO;

typedef struct mqnd_qhndl_node {
	NCS_PATRICIA_NODE pnode;
	MQND_QUEUE_INFO qinfo;
} MQND_QUEUE_NODE;

typedef struct mqnd_mqa_list_node {
	NCS_DB_LINK_LIST_NODE lnode;
	MDS_DEST mqa_dest;
} MQND_MQA_LIST_NODE;

typedef struct mqnd_qname_node {
	NCS_PATRICIA_NODE pnode;
	SaMsgQueueHandleT qhdl;
	SaNameT qname;
} MQND_QNAME_NODE;

typedef struct qtransfer_evt_node {
	NCS_PATRICIA_NODE evt;
	MQND_TMR tmr;		/* Timer to keep track of QTransfer Req */
	MDS_DEST addr;
} MQND_QTRANSFER_EVT_NODE;

typedef struct queue_stats_shm {
	SaMsgQueueUsageT saMsgQueueUsage[SA_MSG_MESSAGE_LOWEST_PRIORITY + 1];
	SaSizeT totalQueueUsed;
	SaUint32T totalNumberOfMessages;
} MQND_QUEUE_STATS_SHM;

/* Q information stored in MQND  for Checkpointing*/
typedef struct mqnd_queue_ckpt_info {
	SaMsgQueueHandleT queueHandle;	/* Assigned by MQND */
	SaMsgQueueHandleT listenerHandle;	/* Listener queue handle */
	SaNameT queueName;
	SaSizeT size[SA_MSG_MESSAGE_LOWEST_PRIORITY + 1];	/* Size of the queue */
	SaMsgQueueCreationFlagsT creationFlags;
	SaTimeT creationTime;	/* Queue Creation time */
	SaTimeT retentionTime;
	SaTimeT closeTime;
	SaMsgQueueSendingStateT sendingState;	/* Sending state is removed from B.1.1, but used internally */
	uint32_t numberOfFullErrors[SA_MSG_MESSAGE_LOWEST_PRIORITY + 1];
	uint8_t owner_flag;	/* Orphan or Owned */
	SaMsgHandleT msgHandle;	/* Message hdl of receiver application */
	MDS_DEST rcvr_mqa;	/* Agent info of Receiver */
	/* Current message queue implimentation requires the unique key
	   for creating, accessing  & destroying queue */
	NCS_OS_MQ_KEY q_key;	/* key that uniquely identify the msg queue */
	uint32_t shm_queue_index;	/*Index of queue info in shared memory */
	MQND_QUEUE_STATS_SHM QueueStatsShm;	/* Structure to store queue stats in shared memory */
	MQND_TMR qtransfer_complete_tmr;	/* Q Transfer Complete Timer */
	uint32_t valid;		/* To verify whether the checkpoint info is valid or not */
} MQND_QUEUE_CKPT_INFO;

typedef struct mqnd_shm_info {
	void *shm_start_addr;
	MQND_QUEUE_CKPT_INFO *shm_base_addr;
	uint32_t max_open_queues;
} MQND_SHM_INFO;

typedef struct mqnd_shm_version {
	uint16_t shm_version;	/* Added to provide support for SAF Inservice upgrade facilty */
	uint16_t dummy_verson1;	/* Not in use */
	uint16_t dummy_verson2;	/* Not in use */
	uint16_t dummy_verson3;	/* Not in use */
} MQND_SHM_VERSION;

typedef struct mqa_rsp_cntxt {
	MQSV_EVT evt;
	MQSV_SEND_INFO sinfo;
	struct mqa_rsp_cntxt *next;
} MQA_RSP_CNTXT;

/* Global data stored in MQND */
typedef struct mqnd_cb {
	SYSF_MBX mbx;		/* Mail box of this Service Part */
	NCSCONTEXT task_hdl;	/* Task Handle */
	uint32_t cb_hdl;		/* CB Struct Handle */
	uint8_t hm_pool;		/* Handle Manager Pool ID for this
				   Service Part */
	MDS_HDL my_mds_hdl;	/* MDS PWE handle   */
	MDS_DEST my_dest;	/* MDS Destination ID of self */

	MDS_DEST mqd_dest;	/* MDS Destination ID of MQD */
	bool is_mqd_up;	/* true/false) */

	NCS_LOCK cb_lock;

	EDU_HDL edu_hdl;	/* Handle used in EDU operations */

	NCS_PATRICIA_TREE qhndl_db;	/* Stores MQND_QUEUE_NODE */
	bool is_qhdl_db_up;	/* true/false */
	bool is_qevt_hdl_db_up;	/* true/false */

	NCS_PATRICIA_TREE qname_db;	/* Stores Mapping between name and handle */
	NCS_PATRICIA_TREE q_transfer_evt_db;	/* Stores Mapping between qtransfer_req_evt and queue_name */
	bool is_qname_db_up;	/* true/false */
	SaAmfHandleT amf_hdl;	/* AMF handle, obtained thru AMF init        */
	SaAmfHAStateT ha_state;	/* present AMF HA state of the component     */
	SaNameT comp_name;
	NCS_DB_LINK_LIST mqa_list_info;	/* List of MQAs which are up */
	MQA_RSP_CNTXT *mqa_dfrd_evt_rsp_list_head;	/* List of Deferred Event responses to mqa */
	bool is_restart_done;
	MDS_DEST up_mqa_dest;
	MQND_TMR mqa_timer;
	bool is_mqa_all_up;
	bool is_create_ckpt;
	uint32_t mqa_counter;
	MQND_SHM_INFO mqnd_shm;
	SaClmHandleT clm_hdl;
	SaClmNodeIdT nodeid;
	uint32_t clm_node_joined;
	uint32_t gl_msg_max_q_size;
	uint32_t gl_msg_max_msg_size;
	uint32_t gl_msg_max_no_of_q;
	uint32_t gl_msg_max_prio_q_size;
	SaImmOiHandleT immOiHandle;
	SaSelectionObjectT imm_sel_obj;
} MQND_CB;

#define MQND_QUEUE_INFO_NULL ((MQND_QUEUE_INFO *)0)
#define SaMsgQueueUsageT_NULL ((SaMsgQueueUsageT *)0)

/* Function Declerations */
/* Queue name to Handle mapping database related functions */
void mqnd_qname_node_get(MQND_CB *cb, SaNameT qname, MQND_QNAME_NODE **o_qnode);
uint32_t mqnd_qname_node_add(MQND_CB *cb, MQND_QNAME_NODE *qnode);
uint32_t mqnd_qname_node_del(MQND_CB *cb, MQND_QNAME_NODE *qnode);
void mqnd_qname_node_getnext(MQND_CB *cb, SaNameT qname, MQND_QNAME_NODE **o_qnode);
/* end */

void mqnd_queue_node_get(MQND_CB *cb, SaMsgQueueHandleT qhdl, MQND_QUEUE_NODE **o_qnode);
uint32_t mqnd_queue_node_add(MQND_CB *cb, MQND_QUEUE_NODE *qnode);
uint32_t mqnd_queue_node_del(MQND_CB *cb, MQND_QUEUE_NODE *qnode);
void mqnd_queue_node_getnext(MQND_CB *cb, SaMsgQueueHandleT qhdl, MQND_QUEUE_NODE **o_qnode);
void mqnd_qevt_node_get(MQND_CB *cb, SaMsgQueueHandleT qhdl, MQND_QTRANSFER_EVT_NODE **o_qnode);
void mqnd_qevt_node_getnext(MQND_CB *cb, SaMsgQueueHandleT qhdl, MQND_QTRANSFER_EVT_NODE **o_qnode);
uint32_t mqnd_qevt_node_add(MQND_CB *cb, MQND_QTRANSFER_EVT_NODE *qevt_node);
uint32_t mqnd_qevt_node_del(MQND_CB *cb, MQND_QTRANSFER_EVT_NODE *qevt_node);
uint32_t mqnd_proc_mqa_down(MQND_CB *cb, MDS_DEST *mqa);

uint32_t mqnd_proc_mqa_down(MQND_CB *cb, MDS_DEST *mqa);

/* Functions from mqnd_mq.c */
uint32_t mqnd_mq_create(MQND_QUEUE_INFO *q_info);
uint32_t mqnd_mq_open(MQND_QUEUE_INFO *q_info);
uint32_t mqnd_mq_destroy(MQND_QUEUE_INFO *q_info);
uint32_t mqnd_mq_msg_send(uint32_t qhdl, MQSV_MESSAGE *i_msg, uint32_t i_len);
uint32_t mqnd_mq_empty(SaMsgQueueHandleT handle);
uint32_t mqnd_mq_rcv(SaMsgQueueHandleT handle);

uint32_t mqnd_listenerq_create(MQND_QUEUE_INFO *q_info);
uint32_t mqnd_listenerq_destroy(MQND_QUEUE_INFO *q_info);

/* AMF Functions */
uint32_t mqnd_amf_init(MQND_CB *mqnd_cb);
void mqnd_amf_de_init(MQND_CB *mqnd_cb);
uint32_t mqnd_amf_register(MQND_CB *mqnd_cb);
uint32_t mqnd_amf_deregister(MQND_CB *mqnd_cb);

/*OTHER FUNCTIONS*/
uint32_t mqnd_evt_proc_tmr_expiry(MQND_CB *cb, MQSV_EVT *evt);

#endif  // MSG_MSGND_MQND_DB_H_
