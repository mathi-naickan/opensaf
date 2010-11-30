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

#ifndef MQSV_EVT_H
#define MQSV_EVT_H

/* event type enums */
typedef enum mqsv_evt_type {
	MQSV_EVT_BASE = 1,
	MQSV_EVT_ASAPI = MQSV_EVT_BASE,
	MQSV_EVT_MQP_REQ,
	MQSV_EVT_MQP_RSP,
	MQSV_EVT_MQA_CALLBACK,
	MQSV_EVT_MQD_CTRL,
	MQSV_EVT_MQND_CTRL,
	MQSV_EVT_MAX = MQSV_EVT_MQND_CTRL
} MQSV_EVT_TYPE;

/* Enums for MQP Message Types */
typedef enum mqp_req_type {
	MQP_EVT_INIT_REQ,
	MQP_EVT_FINALIZE_REQ,
	MQP_EVT_OPEN_REQ,
	MQP_EVT_OPEN_ASYNC_REQ,
	MQP_EVT_CLOSE_REQ,
	MQP_EVT_STATUS_REQ,
	MQP_EVT_UNLINK_REQ,
	MQP_EVT_MSG_GET,
	MQP_EVT_RCVD_MSG_GET,
	MQP_EVT_TRANSFER_QUEUE_REQ,
	MQP_EVT_TRANSFER_QUEUE_COMPLETE,
	MQP_EVT_STAT_UPD_REQ,
	MQP_EVT_REPLY_MSG,
	MQP_EVT_REPLY_MSG_ASYNC,
	MQP_EVT_SEND_MSG,
	MQP_EVT_SEND_MSG_ASYNC,
	MQP_EVT_CB_DUMP,	/* For CPND CB Dump */
	MQP_EVT_Q_RET_TIME_SET_REQ,
	MQP_EVT_CLM_NOTIFY
} MQP_REQ_TYPE;

/* Enums for MQP Message Types */
typedef enum mqp_rsp_type {
	MQP_EVT_INIT_RSP,
	MQP_EVT_FINALIZE_RSP,
	MQP_EVT_OPEN_RSP,
	MQP_EVT_CLOSE_RSP,
	MQP_EVT_STATUS_RSP,
	MQP_EVT_UNLINK_RSP,
	MQP_EVT_REPLY_MSG_RSP,
	MQP_EVT_SEND_MSG_RSP,
	MQP_EVT_TRACK_NOTIFY,
	MQP_EVT_MSG_GET_RSP,
	MQP_EVT_DELETE_NOTIFY,
	MQP_EVT_TRANSFER_QUEUE_RSP,
	MQP_EVT_MQND_RESTART_RSP,
	MQP_EVT_STAT_UPD_RSP,
	MQP_EVT_Q_RET_TIME_SET_RSP
} MQP_RSP_TYPE;

/* Enums for MQD messages */
typedef enum {
	MQD_MSG_USER = 1,
	MQD_MSG_COMP,
	MQD_MSG_TMR_EXPIRY,
	MQD_ND_STATUS_INFO_TYPE,
	MQD_QUISCED_STATE_INFO_TYPE,
	MQD_CB_DUMP_INFO_TYPE,
	MQD_QGRP_CNT_GET
} MQD_MSG_TYPE;

/* enums for message types in the POSIX/OS queue */
typedef enum mqsv_message_type {
	MQP_EVT_CANCEL_REQ,
	MQP_EVT_GET_REQ
} MQSV_MESSAGE_TYPE;

/* The Message format of the message that will be stored in the 
 * POSIX/OS Queue.TBD: Move it somewhere else.
 */

/* This is defined in linux in linux/msg.h. This is checked for
 * compilation in Windows
 */

#ifndef MSGMAX
#define MSGMAX    4096
#endif

typedef struct queue_message {
	SaUint32T type;
	SaUint32T version;
	SaSizeT size;
	uns32 padding;
	SaNameT senderName;
	SaUint8T priority;
	char data[1];
} QUEUE_MESSAGE;

typedef struct mqa_sender_context {
	MDS_DEST sender_dest;
	SaSizeT reply_buffer_size;
	MDS_SYNC_SND_CTXT context;
	uns8 src_dest_version;
	uns8 padding[2];	/*Req for proper alignment in 32/64 arch */
} MQA_SENDER_CONTEXT;

typedef struct mqa_senderid_info {
	NCS_QELEM qelem;
	time_t timestamp;
	MQA_SENDER_CONTEXT sender_context;

} MQA_SENDERID_INFO;

typedef struct queue_message_info {
	SaBoolT sendReceive;	/* In case of sendReceive - SA_TRUE, sender_info is used */
	SaUint32T padding;	/* Do not use */
	SaTimeT sendTime;
	union {
		SaMsgSenderIdT senderId;
		MQA_SENDER_CONTEXT sender_context;
	} sender;
} QUEUE_MESSAGE_INFO;

typedef struct saf_message {
	QUEUE_MESSAGE_INFO message_info;
	QUEUE_MESSAGE message;
} SAF_MESSAGE;

/* saMsgInitialize(): */

typedef struct mqp_init_req {
	SaVersionT version;
} MQP_INIT_REQ;

typedef struct mqp_init_rsp {
	uns32 dummy;
} MQP_INIT_RSP;

/* saMsgFinalize(): */

typedef struct mqp_finalize_req {

	SaMsgHandleT msgHandle;
} MQP_FINALIZE_REQ;

typedef struct mqp_finalize_rsp {

	SaMsgHandleT msgHandle;
} MQP_FINALIZE_RSP;

typedef struct mqp_q_ret_time_set_req {

	SaMsgQueueHandleT queueHandle;
	SaTimeT retentionTime;

} MQP_Q_RET_TIME_SET_REQ;

typedef struct mqp_clm_req {
	uns32 node_joined;
} MQP_CLM_NOTIFY;

typedef struct mqp_q_ret_time_set_rsp {

	SaMsgQueueHandleT queueHandle;
} MQP_Q_RET_TIME_SET_RSP;

/* saMsgQueueOpen(): */

typedef struct mqp_open_req {
	SaMsgHandleT msgHandle;
	SaNameT queueName;
	SaMsgQueueCreationAttributesT creationAttributes;
	SaMsgQueueOpenFlagsT openFlags;
	SaTimeT timeout;
} MQP_OPEN_REQ;

typedef struct mqp_open_rsp {
	SaMsgHandleT msgHandle;
	SaNameT queueName;
	SaMsgQueueHandleT queueHandle;
	SaMsgQueueHandleT listenerHandle;
	uns32 existing_msg_count;
} MQP_OPEN_RSP;
/* Owner transfer request: MQSV internal message */
typedef struct mqp_transferq_req {
	SaMsgQueueHandleT old_queueHandle;
	MDS_DEST new_owner;
	NCS_BOOL empty_queue;	/* TRUE if queue needs to be emptied */
	MQP_OPEN_REQ openReq;
	MQSV_SEND_INFO rcvr_mqa_sinfo;
	SaInvocationT invocation;
	MQP_REQ_TYPE openType;
} MQP_TRANSFERQ_REQ;

typedef struct mqp_transferq_rsp {
	SaMsgQueueHandleT old_queueHandle;
	MQP_OPEN_REQ openReq;
	SaInvocationT invocation;
	MQSV_SEND_INFO rcvr_mqa_sinfo;
	MDS_DEST old_owner;
	MQP_REQ_TYPE openType;
	uns32 msg_count;
	uns32 msg_bytes;
	char *mqsv_message;
	SaTimeT creationTime;
} MQP_TRANSFERQ_RSP;

typedef struct mqp_transferq_complete {
	SaMsgQueueHandleT queueHandle;
	uns32 error;
} MQP_TRANSFERQ_COMPLETE;

typedef struct mqp_openasync_req {
	MQP_OPEN_REQ mqpOpenReq;
	SaInvocationT invocation;

} MQP_OPEN_ASYNC_REQ;

/* saMsgQueueClose(): */

typedef struct mqp_close_req {
	SaMsgQueueHandleT queueHandle;
} MQP_CLOSE_REQ;

typedef struct mqp_close_rsp {
	SaMsgQueueHandleT queueHandle;
} MQP_CLOSE_RSP;

/* saMsgQueueStatusGet(): */

typedef struct mqp_queue_status_req {
	SaMsgHandleT msgHandle;
	SaMsgQueueHandleT queueHandle;
	SaNameT queueName;
} MQP_QUEUE_STATUS_REQ;

typedef struct mqp_queue_status_rsp {

	SaMsgHandleT msgHandle;
	SaMsgQueueHandleT queueHandle;
	SaNameT queueName;
	SaMsgQueueStatusT queueStatus;
} MQP_QUEUE_STATUS_RSP;

/* saMsgQueueUnlink(): */

typedef struct mqp_unlink_req {
	SaMsgHandleT msgHandle;
	SaMsgQueueHandleT queueHandle;
	SaNameT queueName;
} MQP_UNLINK_REQ;

typedef struct mqp_unlink_rsp {

	SaMsgHandleT msgHandle;
	SaMsgQueueHandleT queueHandle;
	SaNameT queueName;
} MQP_UNLINK_RSP;

typedef struct mqp_queue_reply_msg {
	SaMsgAckFlagsT ackFlags;
	uns32 padding;
	SaMsgHandleT msgHandle;
	QUEUE_MESSAGE_INFO messageInfo;
	QUEUE_MESSAGE message;
} MQP_QUEUE_REPLY_MSG;

typedef struct mqp_queue_reply_msg_async {
	SaInvocationT invocation;
	MQP_QUEUE_REPLY_MSG reply;
} MQP_QUEUE_REPLY_MSG_ASYNC;

typedef struct mqp_queue_reply_rsp {
	SaMsgHandleT msgHandle;
} MQP_QUEUE_REPLY_RSP;

/* Structure used by MQSV to ack the delivery of the sent message to MQA in Sync send */

/* Structure used by MQA to send messages to Queue sitting on MQND */
typedef struct mqp_send_msg {
	SaMsgHandleT msgHandle;	/* Application Hdl */
	SaMsgQueueHandleT queueHandle;
	SaMsgAckFlagsT ackFlags;	/* Type of Ack Required */
	SaNameT destination;	/* Queue Name */
	uns8 padding[2];
	QUEUE_MESSAGE_INFO messageInfo;
	QUEUE_MESSAGE message;
} MQP_SEND_MSG;

typedef struct mqp_send_msg_async {
	SaInvocationT invocation;
	MQP_SEND_MSG SendMsg;
} MQP_SEND_MSG_ASYNC;

/* Structure used by MQSV to ack the delivery of the sent message to MQA in Sync send */
typedef struct mqsv_send_msg_rsp {
	SaAisErrorT error;
	uns32 padding;
	SaMsgHandleT msgHandle;
} MQP_SEND_MSG_RSP;

/* Structure used to get the Message from Q */
typedef struct mqp_msg_get {
	SaMsgHandleT msgHandle;
	SaMsgQueueHandleT queueHandle;
} MQP_MSG_GET;

/* Structure used to get the Received Message from Q */
typedef struct mqp_rcvd_msg_get {
	SaMsgHandleT msgHandle;
	SaMsgQueueHandleT queueHandle;
	SaMsgHandleT messageHandle;
} MQP_RCVD_MSG_GET;

typedef struct mqp_cancel_req {
	SaMsgQueueHandleT queueHandle;
	tmr_t timerId;
} MQP_CANCEL_REQ;

typedef struct mqp_update_stats {
	SaMsgQueueHandleT qhdl;
	uns32 priority;
	uns32 size;
} MQP_UPDATE_STATS;

typedef struct mqp_stats_rsp {
	uns32 dummy;
} MQP_STATS_RSP;

/* The format of message to be placed in the queue or sent from MQA to MQND */

typedef struct mqsv_message {
	MQSV_MESSAGE_TYPE type;
	uns16 padding;		/* Do not use */
	uns16 mqsv_version;

	union {
		MQP_CANCEL_REQ cancel_req;
		SAF_MESSAGE msg;
	} info;

} MQSV_MESSAGE;

/* To tell mqa that mqnd restarted successfully */
typedef struct mqp_mqnd_restart_rsp {
	NCS_BOOL restart_done;
} MQP_MQND_RESTART_RSP;

/******************************************************************************
* MQSV_EVT_MQP Messages
******************************************************************************/
typedef struct mqp_req_msg {
	MQP_REQ_TYPE type;
	uns32 padding;		/* DO NOT USE */
	MDS_DEST agent_mds_dest;
	union {
		MQP_INIT_REQ initReq;
		MQP_FINALIZE_REQ finalReq;
		MQP_QUEUE_STATUS_REQ statusReq;
		MQP_OPEN_REQ openReq;
		MQP_OPEN_ASYNC_REQ openAsyncReq;
		MQP_CLOSE_REQ closeReq;
		MQP_UNLINK_REQ unlinkReq;
		MQP_QUEUE_REPLY_MSG replyMsg;
		MQP_QUEUE_REPLY_MSG_ASYNC replyAsyncMsg;
		MQP_SEND_MSG snd_msg;
		MQP_SEND_MSG_ASYNC sndMsgAsync;
		MQP_MSG_GET m_get;
		MQP_RCVD_MSG_GET rcvd_m_get;
		MQP_TRANSFERQ_REQ transferReq;
		MQP_TRANSFERQ_COMPLETE transferComplete;
		MQP_UPDATE_STATS statsReq;
		MQP_Q_RET_TIME_SET_REQ retTimeSetReq;
		MQP_CLM_NOTIFY clmNotify;
	} info;
} MQP_REQ_MSG;

typedef struct mqp_rsp_msg {
	MQP_RSP_TYPE type;
	SaAisErrorT error;
	union {
		MQP_INIT_RSP initRsp;
		MQP_FINALIZE_RSP finalRsp;
		MQP_QUEUE_STATUS_RSP statusRsp;
		MQP_OPEN_RSP openRsp;
		MQP_CLOSE_RSP closeRsp;
		MQP_UNLINK_RSP unlinkRsp;
		MQP_TRANSFERQ_RSP transferRsp;
		MQP_MQND_RESTART_RSP restartRsp;
		MQP_STATS_RSP statsRsp;
		MQP_QUEUE_REPLY_RSP replyRsp;
		MQP_SEND_MSG_RSP sendMsgRsp;
		MQP_Q_RET_TIME_SET_RSP retTimeSetRsp;
	} info;
} MQP_RSP_MSG;

/* MQP Messages related to the callback between MQND and MQA. */

typedef enum mqa_callback_type {
	/* enums of the callbacks. */
	MQP_ASYNC_RSP_OPEN = 1,
	MQP_ASYNC_RSP_MSGDELIVERED,
	MQP_ASYNC_RSP_MSGRECEIVED,
	MQP_ASYNC_RSP_GRP_TRACK,
	MQP_ASYNC_RSP_TYPE_MAX
} MQP_ASYNC_RSP_TYPE;

typedef struct mqsv_msgq_open_param {
	SaInvocationT invocation;
	SaMsgQueueHandleT queueHandle;
	SaAisErrorT error;
	SaMsgQueueOpenFlagsT openFlags;
	SaMsgQueueHandleT listenerHandle;
	uns32 existing_msg_count;
} MQSV_MSGQ_OPEN_PARAM;

typedef struct mqsv_msgqgrp_track_param {
	SaNameT queueGroupName;
	SaMsgQueueGroupNotificationBufferT notificationBuffer;
	SaMsgQueueGroupPolicyT queueGroupPolicy;
	SaUint32T numberOfMembers;
	SaAisErrorT error;

} MQSV_MSGQGRP_TRACK_PARAM;

typedef struct mqsv_msg_delivered_param {
	SaInvocationT invocation;
	SaAisErrorT error;

} MQSV_MSG_DELIVERED_PARAM;

typedef struct mqsv_msg_received_param {

	SaMsgQueueHandleT queueHandle;
	SaSizeT size;

} MQSV_MSG_RECEIVED_PARAM;

typedef struct mqp_async_rsp_msg {
	struct mqp_async_rsp_msg *next;
	MQP_ASYNC_RSP_TYPE callbackType;
	SaMsgHandleT messageHandle;
	union {
		MQSV_MSGQ_OPEN_PARAM qOpen;
		MQSV_MSGQGRP_TRACK_PARAM qGrpTrack;
		MQSV_MSG_DELIVERED_PARAM msgDelivered;
		MQSV_MSG_RECEIVED_PARAM msgReceived;
	} params;
} MQP_ASYNC_RSP_MSG;

/* Data struct used to inform timer expiry info to MQND */
typedef struct mqd_tmr_exp_info {
	uns32 type;		/* Timer Type */
	NODE_ID nodeid;
} MQD_TMR_EXP_INFO;

/* When ND is down is_up is FALSE else it's set to TRUE */
typedef struct mqd_nd_status_info {
	MDS_DEST dest;		/*Destination MQND which is up/ down */
	NCS_BOOL is_up;		/* TRUE if ND is up else FALSE */
	SaTimeT event_time;	/* Down time or up time */
} MQD_ND_STATUS_INFO;

 /* For handling the quisced state */
typedef struct mqd_quisced_state_info {
	SaInvocationT invocation;	/* Invocation coming from the AMF */
} MQD_QUISCED_STATE_INFO;

typedef struct qgrp_count_info {
	SaAisErrorT error;
	struct queue_info {
		SaNameT queueName;
		uns32 noOfQueueGroupMemOf;
	} info;
} MQSV_CTRL_EVT_QGRP_CNT;

typedef struct mqd_ctrl_msg {
	MQD_MSG_TYPE type;
	union {
		MDS_DEST user;
		NCS_BOOL init;
		MQD_TMR_EXP_INFO tmr_info;
		MQD_ND_STATUS_INFO nd_info;
		MQD_QUISCED_STATE_INFO quisced_info;
		MQSV_CTRL_EVT_QGRP_CNT qgrp_cnt_info;
	} info;
} MQD_CTRL_MSG;

struct asapi_msg_info;

typedef enum mqnd_ctrl_evt_type {
	MQND_CTRL_EVT_TMR_EXPIRY,
	MQND_CTRL_EVT_QATTR_GET,
	MQND_CTRL_EVT_QATTR_INFO,
	MQND_CTRL_EVT_MDS_INFO,
	MQND_CTRL_EVT_QGRP_MEMBER_INFO,
	MQND_CTRL_EVT_MDS_MQA_UP_INFO,
	MQND_CTRL_EVT_MAX,
	MQND_CTRL_EVT_QGRP_CNT_RSP,
	MQND_CTRL_EVT_DEFERRED_MQA_RSP
} MQND_CTRL_EVT_TYPE;

/* Datastructure used to get the queue attributes */
typedef struct mqsv_qattr_req {
	SaMsgQueueHandleT qhdl;
} MQND_QATTR_REQ;

/* Data Structure carries the Queue attributes */
typedef struct mqsv_qattr_info {
	SaAisErrorT error;
	SaMsgQueueCreationAttributesT qattr;
} MQND_QATTR_INFO;

/* Data struct used to inform timer expiry info to MQND */
typedef struct mqnd_tmr_exp_info {
	uns32 type;		/* Timer Type */
	SaMsgQueueHandleT qhdl;
	tmr_t tmr_id;
} MQND_TMR_EXP_INFO;

typedef struct mqsv_remove_queue {
	SaMsgQueueHandleT qhdl;
} MQND_REMOVE_QUEUE;

/* Structure for passing MDS info to components */
typedef struct mqnd_mds_info {
	NCSMDS_CHG change;	/* GONE, UP, DOWN, CHG ROLE  */
	MDS_DEST dest;
	MDS_SVC_ID svc_id;
} MQND_MDS_INFO;

/* To process the MQA up event */
typedef struct mqnd_mds_mqa_up_info {
	MDS_DEST mqa_up_dest;
} MQND_MDS_MQA_UP_INFO;

/* MQND Control Data Structure */
typedef struct mqnd_ctrl_msg {
	MQND_CTRL_EVT_TYPE type;
	union {
		MQND_QATTR_REQ qattr_get;
		MQND_QATTR_INFO qattr_info;
		MQND_TMR_EXP_INFO tmr_info;
		MQND_REMOVE_QUEUE qremove;
		MQND_MDS_INFO mds_info;
		MQND_MDS_MQA_UP_INFO mqa_up_info;
		MQSV_CTRL_EVT_QGRP_CNT qgrp_cnt_info;
	} info;
} MQND_CTRL_MSG;

typedef struct mqsv_evt {
	uns64 hook_for_ipc;
	uns8 evt_type;
	MQSV_EVT_TYPE type;
	MQSV_SEND_INFO sinfo;
	union {
		MQP_REQ_MSG mqp_req;
		MQP_RSP_MSG mqp_rsp;
		MQP_ASYNC_RSP_MSG mqp_async_rsp;
		struct asapi_msg_info *asapi;
		MQD_CTRL_MSG mqd_ctrl;
		MQND_CTRL_MSG mqnd_ctrl;
	} msg;
} MQSV_EVT;

/*Event Structure involving Direct Sends*/
typedef struct mqsv_direct_send_event {
	uns64 hook_for_ipc;
	uns8 evt_type;
	uns8 endianness;
	uns8 msg_fmt_version;
	uns8 src_dest_version;
	union {
		uns32 raw;
		MQP_RSP_TYPE rsp_type;
		MQP_REQ_TYPE req_type;
	} type;
	MQSV_DSEND_INFO sinfo;
	MDS_DEST agent_mds_dest;
	union {
		MQP_SEND_MSG snd_msg;
		MQP_SEND_MSG_ASYNC sndMsgAsync;
		MQP_QUEUE_REPLY_MSG replyMsg;
		MQP_QUEUE_REPLY_MSG_ASYNC replyAsyncMsg;
		MQP_UPDATE_STATS statsReq;
		MQP_SEND_MSG_RSP sendMsgRsp;
	} info;
} MQSV_DSEND_EVT;

#define m_MQD_EVT_SEND(mbx, msg, prio) m_NCS_IPC_SEND(mbx, (NCSCONTEXT)msg, prio)
#define MQSV_WAIT_TIME  1000	/* MDS wait time in case of syncronous call */
#define MQSV_WAIT_TIME_MQND  800
#define MQSV_MAX_SND_SIZE       4096

#endif
