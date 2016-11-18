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
 */

#ifndef MSG_APITEST_TET_MQSV_H_
#define MSG_APITEST_TET_MQSV_H_

#include "tet_startup.h"
#include "ncs_lib.h"
#include "saMsg.h"

#define APP_TIMEOUT 10000000000ULL
#define MSG_SEND_RCV_TIMEOUT 10000000000ULL
#define MAX_MESSAGE_SIZE 1024
#define SA_MSG_QUEUE_NON_PERSISTENT 0
#define m_MQSV_WAIT sleep(2)

#define m_TET_MQSV_PRINTF printf

/* saMsgInitialize */

typedef enum {
   MSG_INIT_NULL_HANDLE_T=1,
   MSG_INIT_NULL_VERSION_T,
   MSG_INIT_NULL_PARAMS_T,
   MSG_INIT_NULL_CLBK_PARAM_T,
   MSG_INIT_NULL_VERSION_CBKS_T,
   MSG_INIT_BAD_VERSION_T,
   MSG_INIT_BAD_REL_CODE_T,
   MSG_INIT_BAD_MAJOR_VER_T,
   MSG_INIT_SUCCESS_T,
   MSG_INIT_SUCCESS_HDL2_T,
   MSG_INIT_NULL_CBKS_T,
   MSG_INIT_NULL_CBKS2_T,
   MSG_INIT_NULL_DEL_CBK_T,
   MSG_INIT_NULL_RCV_CBK_T,
   MSG_INIT_SUCCESS_RECV_T,
   MSG_INIT_SUCCESS_REPLY_T,
   MSG_INIT_ERR_TRY_AGAIN_T,
   MSG_INIT_MAX_T
}MSG_INIT_TC_TYPE;

struct SafMsgInitialize {
   SaMsgHandleT *msgHandle;
   SaVersionT *version;
   SaMsgCallbacksT *callbks;
   SaAisErrorT exp_output;
};

/* saMsgSelectionObjectGet */

typedef enum {
   MSG_SEL_OBJ_BAD_HANDLE_T=1,
   MSG_SEL_OBJ_NULL_SEL_OBJ_T,
   MSG_SEL_OBJ_SUCCESS_T,
   MSG_SEL_OBJ_FINALIZED_HDL_T,
   MSG_SEL_OBJ_ERR_TRY_AGAIN_T,
   MSG_SEL_OBJ_MAX_T
}MSG_SEL_OBJ_TC_TYPE;

struct SafMsgSelectionObject {
   SaMsgHandleT *msgHandle;
   SaSelectionObjectT *selobj;
   SaAisErrorT exp_output;
};

/* saMsgDispatch */

typedef enum {
   MSG_DISPATCH_ONE_BAD_HDL_T=1,
   MSG_DISPATCH_ONE_FINALIZED_HDL_T,
   MSG_DISPATCH_ALL_BAD_HDL_T,
   MSG_DISPATCH_ALL_FINALIZED_HDL_T,
   MSG_DISPATCH_BLKING_BAD_HDL_T,
   MSG_DISPATCH_BLKING_FINALIZED_HDL_T,
   MSG_DISPATCH_BAD_FLAGS_T,
   MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,
   MSG_DISPATCH_DISPATCH_ALL_SUCCESS_T,
   MSG_DISPATCH_DISPATCH_BLOCKING_SUCCESS_T,
   MSG_DISPATCH_ERR_TRY_AGAIN_T,
   MSG_DISPATCH_MAX_T
}MSG_DISPATCH_TC_TYPE;

struct SafMsgDispatch {
   SaMsgHandleT *msgHandle;
   SaDispatchFlagsT flags;
   SaAisErrorT exp_output;
};

/* saMsgFinalize */

typedef enum {
   MSG_FINALIZE_BAD_HDL_T=1,
   MSG_FINALIZE_SUCCESS_T,
   MSG_FINALIZE_SUCCESS_HDL2_T,
   MSG_FINALIZE_SUCCESS_HDL3_T,
   MSG_FINALIZE_SUCCESS_HDL4_T,
   MSG_FINALIZE_FINALIZED_HDL_T,
   MSG_FINALIZE_ERR_TRY_AGAIN_T,
   MSG_FINALIZE_MAX_T
}MSG_FINALIZE_TC_TYPE;

struct SafMsgFinalize {
   SaMsgHandleT *msgHandle;
   SaAisErrorT exp_output;
};

/* saMsgQueueOpen */

typedef enum {
   MSG_QUEUE_OPEN_BAD_HANDLE_T=1,
   MSG_QUEUE_OPEN_FINALIZED_HDL_T,
   MSG_QUEUE_OPEN_NULL_Q_HDL_T,
   MSG_QUEUE_OPEN_NULL_NAME_T,
   MSG_QUEUE_OPEN_BAD_TIMEOUT_T,
   MSG_QUEUE_OPEN_INVALID_PARAM_T,
   MSG_QUEUE_OPEN_INVALID_PARAM_EMPTY_T,
   MSG_QUEUE_OPEN_INVALID_PARAM2_EMPTY_T,
   MSG_QUEUE_OPEN_INVALID_PARAM2_ZERO_T,
   MSG_QUEUE_OPEN_INVALID_PARAM2_RC_CBK_T,
   MSG_QUEUE_OPEN_BAD_FLAGS_T,
   MSG_QUEUE_OPEN_BAD_FLAGS2_T,
   MSG_QUEUE_OPEN_PERS_ERR_EXIST_T,
   MSG_QUEUE_OPEN_NPERS_ERR_EXIST_T,
   MSG_QUEUE_OPEN_PERS_ER_NOT_EXIST_T,
   MSG_QUEUE_OPEN_PERS_ER_NOT_EXIST2_T,
   MSG_QUEUE_OPEN_NPERS_ER_NOT_EXIST_T,
   MSG_QUEUE_OPEN_ZERO_TIMEOUT_T,
   MSG_QUEUE_OPEN_EMPTY_CREATE_T,
   MSG_QUEUE_OPEN_ZERO_RET_T,
   MSG_QUEUE_OPEN_ZERO_SIZE_T,
   MSG_QUEUE_OPEN_PERS_SUCCESS_T,
   MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,
   MSG_QUEUE_OPEN_PERS_SUCCESS2_T,
   MSG_QUEUE_OPEN_NON_PERS_SUCCESS2_T,
   MSG_QUEUE_OPEN_PERS_EXIST_SUCCESS_T,
   MSG_QUEUE_OPEN_NPERS_EXIST_SUCCESS_T,
   MSG_QUEUE_OPEN_ERR_BUSY_T,
   MSG_QUEUE_OPEN_EXIST_ERR_BUSY_T,
   MSG_QUEUE_OPEN_NPERS_EMPTY_SUCCESS_T,
   MSG_QUEUE_OPEN_PERS_EMPTY_SUCCESS_T,
   MSG_QUEUE_OPEN_ERR_INIT_T,
   MSG_QUEUE_OPEN_SMALL_SIZE_ATTR_T,
   MSG_QUEUE_OPEN_SMALL_SIZE_ATTR2_T,
   MSG_QUEUE_OPEN_BIG_SIZE_ATTR_T,
   MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,
   MSG_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T,
   MSG_QUEUE_OPEN_EXIST_RECV_CLBK_SUCCESS_T,
   MSG_QUEUE_OPEN_NPERS_RECV_CLBK_T,
   MSG_QUEUE_OPEN_PERS_RECV_CLBK2_T,
   MSG_QUEUE_OPEN_NPERS_RECV_CLBK2_T,
   MSG_QUEUE_OPEN_EXISTING_PERS_T,
   MSG_QUEUE_OPEN_EXISTING_NPERS_T,
   MSG_QUEUE_OPEN_EXISTING_PERS2_T,
   MSG_QUEUE_OPEN_EXISTING_NPERS2_T,
   MSG_QUEUE_OPEN_ERR_TRY_AGAIN_T,
   MSG_QUEUE_OPEN_MAX_T
}MSG_QUEUE_OPEN_TC_TYPE;

struct SafMsgQueueOpen {
   SaMsgHandleT *msgHandle;
   SaNameT *queueName;
   SaMsgQueueCreationAttributesT *creationAttributes;
   SaMsgQueueOpenFlagsT openFlags;
   SaTimeT timeout;
   SaMsgQueueHandleT *queueHandle;
   SaAisErrorT exp_output;
};

/* saMsgQueueOpenAsync */

typedef enum {
   MSG_QUEUE_OPEN_ASYNC_BAD_HANDLE_T=1,
   MSG_QUEUE_OPEN_ASYNC_FINALIZED_HDL_T,
   MSG_QUEUE_OPEN_ASYNC_NULL_NAME_T,
   MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM_T,
   MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM_EMPTY_T,
   MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_EMPTY_T,
   MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_ZERO_T,
   MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_RC_CBK_T,
   MSG_QUEUE_OPEN_ASYNC_BAD_FLGS_T,
   MSG_QUEUE_OPEN_ASYNC_BAD_FLGS2_T,
   MSG_QUEUE_OPEN_ASYNC_ERR_INIT_T,
   MSG_QUEUE_OPEN_ASYNC_ERR_INIT2_T,
   MSG_QUEUE_OPEN_ASYNC_ERR_NOT_EXIST_T,
   MSG_QUEUE_OPEN_ASYNC_ERR_NOT_EXIST2_T,
   MSG_QUEUE_OPEN_ASYNC_ERR_BUSY_T,
   MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T,
   MSG_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T,
   MSG_QUEUE_OPEN_ASYNC_EMPTY_CREATE_T,
   MSG_QUEUE_OPEN_ASYNC_ERR_EXIST_T,
   MSG_QUEUE_OPEN_ASYNC_ERR_EXIST2_T,
   MSG_QUEUE_OPEN_ASYNC_EXIST_SUCCESS_T,
   MSG_QUEUE_OPEN_ASYNC_EXIST_SUCCESS2_T,
   MSG_QUEUE_OPEN_ASYNC_NPERS_EMPTY_SUCCESS_T,
   MSG_QUEUE_OPEN_ASYNC_SMALL_SIZE_ATTR_T,
   MSG_QUEUE_OPEN_ASYNC_PERS_RECV_CLBK_SUCCESS_T,
   MSG_QUEUE_OPEN_ASYNC_PERS_RECV_CLBK_SUCCESS2_T,
   MSG_QUEUE_OPEN_ASYNC_NPERS_RECV_CLBK_SUCCESS_T,
   MSG_QUEUE_OPEN_ASYNC_EXIST_RECV_CLBK_SUCCESS_T,
   MSG_QUEUE_OPEN_ASYNC_ZERO_RET_T,
   MSG_QUEUE_OPEN_ASYNC_ZERO_SIZE_T,
   MSG_QUEUE_OPEN_ASYNC_EXIST_ERR_BUSY_T,
   MSG_QUEUE_OPEN_ASYNC_EXIST_ERR_BUSY2_T,
   MSG_QUEUE_OPEN_ASYNC_PERS_EMPTY_SUCCESS_T,
   MSG_QUEUE_OPEN_ASYNC_ERR_TRY_AGAIN_T,
   MSG_QUEUE_OPEN_ASYNC_MAX_T
}MSG_QUEUE_OPEN_ASYNC_TC_TYPE;

struct SafMsgQueueOpenAsync {
   SaMsgHandleT *msgHandle;
   SaInvocationT invocation;
   SaNameT *queueName;
   SaMsgQueueCreationAttributesT *creationAttributes;
   SaMsgQueueOpenFlagsT openFlags;
   SaAisErrorT exp_output;
};

/* saMsgQueueClose */

typedef enum {
   MSG_QUEUE_CLOSE_INV_HANDLE_T=1,
   MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,
   MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,
   MSG_QUEUE_CLOSE_SUCCESS_HDL3_T,
   MSG_QUEUE_CLOSE_SUCCESS_HDL4_T,
   MSG_QUEUE_CLOSE_BAD_HANDLE1_T,
   MSG_QUEUE_CLOSE_BAD_HANDLE2_T,
   MSG_QUEUE_CLOSE_SUCCESS_HDL5_T,
   MSG_QUEUE_CLOSE_ERR_TRY_AGAIN_T,
   MSG_QUEUE_CLOSE_MAX_T
}MSG_QUEUE_CLOSE_TC_TYPE;

struct SafMsgQueueClose {
   SaMsgQueueHandleT *queueHandle;
   SaAisErrorT exp_output;
};

/* saMsgQueueStatusGet */

typedef enum {
   MSG_QUEUE_STATUS_BAD_HANDLE_T=1,
   MSG_QUEUE_STATUS_FINALIZED_HDL_T,
   MSG_QUEUE_STATUS_NULL_STATUS_T,
   MSG_QUEUE_STATUS_NULL_NAME_T,
   MSG_QUEUE_STATUS_GET_SUCCESS_T,
   MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,
   MSG_QUEUE_STATUS_GET_SUCCESS_Q3_T,
   MSG_QUEUE_STATUS_GET_SUCCESS_Q4_T,
   MSG_QUEUE_STATUS_GET_SUCCESS_Q5_T,
   MSG_QUEUE_STATUS_NOT_EXIST_Q1_T,
   MSG_QUEUE_STATUS_NOT_EXIST_Q2_T,
   MSG_QUEUE_STATUS_NOT_EXIST_Q3_T,
   MSG_QUEUE_STATUS_NOT_EXIST_Q4_T,
   MSG_QUEUE_STATUS_ERR_TRY_AGAIN_T,
   MSG_QUEUE_STATUS_MAX_T
}MSG_QUEUE_STATUS_TC_TYPE;

struct SafMsgQueueStatusGet {
   SaMsgHandleT *msgHandle;
   SaNameT *queueName;
   SaMsgQueueStatusT *queueStatus;
   SaAisErrorT exp_output;
};

/* saMsgQueueUnlink */

typedef enum {
   MSG_QUEUE_UNLINK_BAD_HANDLE_T=1,
   MSG_QUEUE_UNLINK_FINALIZED_HDL_T,
   MSG_QUEUE_UNLINK_NULL_NAME_T,
   MSG_QUEUE_UNLINK_ERR_NOT_EXIST_T,
   MSG_QUEUE_UNLINK_SUCCESS_Q1_T,
   MSG_QUEUE_UNLINK_SUCCESS_Q2_T,
   MSG_QUEUE_UNLINK_SUCCESS_Q3_T,
   MSG_QUEUE_UNLINK_SUCCESS_Q4_T,
   MSG_QUEUE_UNLINK_SUCCESS_Q5_T,
   MSG_QUEUE_UNLINK_ERR_TRY_AGAIN_T,
   MSG_QUEUE_UNLINK_MAX_T
}MSG_QUEUE_UNLINK_TC_TYPE;

struct SafMsgQueueUnlink {
   SaMsgHandleT *msgHandle;
   SaNameT *queueName;
   SaAisErrorT exp_output;
};

/* saMsgQueueGroupCreate */

typedef enum {
   MSG_GROUP_CREATE_BAD_HDL_T=1,
   MSG_GROUP_CREATE_FINALIZED_HDL_T,
   MSG_GROUP_CREATE_NULL_NAME_T,
   MSG_GROUP_CREATE_BAD_POL_T,
   MSG_GROUP_CREATE_LOCAL_RR_T,
   MSG_GROUP_CREATE_LCL_BSTQ_NOT_SUPP_T,
   MSG_GROUP_CREATE_BROADCAST_T,
   MSG_GROUP_CREATE_SUCCESS_T,
   MSG_GROUP_CREATE_ERR_EXIST_T,
   MSG_GROUP_CREATE_ERR_EXIST2_T,
   MSG_GROUP_CREATE_ERR_EXIST3_T,
   MSG_GROUP_CREATE_ERR_EXIST4_T,
   MSG_GROUP_CREATE_SUCCESS_GR2_T,
   MSG_GROUP_CREATE_BROADCAST2_T,
   MSG_GROUP_CREATE_ERR_TRY_AGAIN_T,
   MSG_GROUP_CREATE_MAX_T
}MSG_GROUP_CREATE_TC_TYPE;

struct SafMsgGroupCreate {
   SaMsgHandleT *msgHandle;
   SaNameT *queueGroupName;
   SaMsgQueueGroupPolicyT queueGroupPolicy;
   SaAisErrorT exp_output;   
};

/* saMsgQueueGroupInsert */

typedef enum {
   MSG_GROUP_INSERT_BAD_HDL_T=1,
   MSG_GROUP_INSERT_FINALIZED_HDL_T,
   MSG_GROUP_INSERT_NULL_GR_NAME_T,
   MSG_GROUP_INSERT_NULL_Q_NAME_T,
   MSG_GROUP_INSERT_BAD_GR_T,
   MSG_GROUP_INSERT_BAD_QUEUE_T,
   MSG_GROUP_INSERT_SUCCESS_T,
   MSG_GROUP_INSERT_SUCCESS_Q2_T,
   MSG_GROUP_INSERT_SUCCESS_Q3_T,
   MSG_GROUP_INSERT_SUCCESS_ZEROQ_T,
   MSG_GROUP_INSERT_ERR_EXIST_T,
   MSG_GROUP_INSERT_Q1_GROUP2_T,
   MSG_GROUP_INSERT_Q2_GROUP2_T,
   MSG_GROUP_INSERT_ERR_TRY_AGAIN_T,
   MSG_GROUP_INSERT_MAX_T
}MSG_GROUP_INSERT_TC_TYPE;

struct SafMsgGroupInsert {
   SaMsgHandleT *msgHandle;
   SaNameT *queueGroupName;
   SaNameT *queueName;
   SaAisErrorT exp_output;
};

/* saMsgQueueGroupRemove */

typedef enum {
   MSG_GROUP_REMOVE_BAD_HDL_T=1,
   MSG_GROUP_REMOVE_FINALIZED_HDL_T,
   MSG_GROUP_REMOVE_NULL_GR_NAME_T,
   MSG_GROUP_REMOVE_NULL_Q_NAME_T,
   MSG_GROUP_REMOVE_BAD_GROUP_T,
   MSG_GROUP_REMOVE_BAD_QUEUE_T,
   MSG_GROUP_REMOVE_SUCCESS_T,
   MSG_GROUP_REMOVE_NOT_EXIST_T,
   MSG_GROUP_REMOVE_SUCCESS_Q2_T,
   MSG_GROUP_REMOVE_SUCCESS_Q3_T,
   MSG_GROUP_REMOVE_ERR_TRY_AGAIN_T,
   MSG_GROUP_REMOVE_MAX_T
}MSG_GROUP_REMOVE_TC_TYPE;

struct SafMsgGroupRemove {
   SaMsgHandleT *msgHandle;
   SaNameT *queueGroupName;
   SaNameT *queueName;
   SaAisErrorT exp_output;
};

/* saMsgQueueGroupDelete */

typedef enum {
   MSG_GROUP_DELETE_BAD_HDL_T=1,
   MSG_GROUP_DELETE_FINALIZED_HDL_T,
   MSG_GROUP_DELETE_NULL_NAME_T,
   MSG_GROUP_DELETE_BAD_GROUP_T,
   MSG_GROUP_DELETE_WITH_MEM_T,
   MSG_GROUP_DELETE_WO_MEM_T,
   MSG_GROUP_DELETE_SUCCESS_T,
   MSG_GROUP_DELETE_SUCCESS_GR2_T,
   MSG_GROUP_DELETE_ERR_TRY_AGAIN_T,
   MSG_GROUP_DELETE_MAX_T
}MSG_GROUP_DELETE_TC_TYPE;

struct SafMsgGroupDelete {
   SaMsgHandleT *msgHandle;
   SaNameT *queueGroupName;
   SaAisErrorT exp_output;
};

/* saMsgQueueGroupTrack */

typedef enum {
   MSG_GROUP_TRACK_BAD_HDL_T=1,
   MSG_GROUP_TRACK_FINALIZED_HDL_T,
   MSG_GROUP_TRACK_NULL_NAME_T,
   MSG_GROUP_TRACK_INV_PARAM_T,
   MSG_GROUP_TRACK_BAD_GROUP_T,
   MSG_GROUP_TRACK_BAD_FLAGS_T,
   MSG_GROUP_TRACK_WRONG_FLGS_T,
   MSG_GROUP_TRACK_NULL_BUF_T,
   MSG_GROUP_TRACK_CH_BAD_BUF_T,
   MSG_GROUP_TRACK_CHONLY_BAD_BUF_T,
   MSG_GROUP_TRACK_CUR_CH_BAD_BUF_T,
   MSG_GROUP_TRACK_CUR_CHONLY_BAD_BUF_T,
   MSG_GROUP_TRACK_CURR_NULBUF_ERINIT_T,
   MSG_GROUP_TRACK_CURR_ERR_INIT_T,
   MSG_GROUP_TRACK_CHGS_ERR_INIT_T,
   MSG_GROUP_TRACK_CHONLY_ERR_INIT_T,
   MSG_GROUP_TRACK_CURRENT_T,
   MSG_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T,
   MSG_GROUP_TRACK_CUR_NO_SPACE_T,
   MSG_GROUP_TRACK_CHANGES_T,
   MSG_GROUP_TRACK_CHANGES_GR2_T,
   MSG_GROUP_TRACK_CHGS_ONLY_T,
   MSG_GROUP_TRACK_CUR_CH_T,
   MSG_GROUP_TRACK_CUR_CHLY_T,
   MSG_GROUP_TRACK_CUR_CH_NUL_BUF_T,
   MSG_GROUP_TRACK_CUR_CHLY_NUL_BUF_T,
   MSG_GROUP_TRACK_CUR_CHLY_NUL_BUF_GR2_T,
   MSG_GROUP_TRACK_ERR_TRY_AGAIN_T,
   MSG_GROUP_TRACK_MAX_T
}MSG_GROUP_TRACK_TC_TYPE;

struct SafMsgGroupTrack {
   SaMsgHandleT *msgHandle;
   SaNameT *queueGroupName;
   SaUint8T trackFlags;
   SaMsgQueueGroupNotificationBufferT *buffer;
   SaAisErrorT exp_output;
};

/* saMsgQueueGroupTrackStop */

typedef enum {
   MSG_GROUP_TRACK_STOP_BAD_HDL_T=1,
   MSG_GROUP_TRACK_STOP_FINALIZED_HDL_T,
   MSG_GROUP_TRACK_STOP_NULL_NAME_T,
   MSG_GROUP_TRACK_STOP_BAD_NAME_T,
   MSG_GROUP_TRACK_STOP_SUCCESS_T,
   MSG_GROUP_TRACK_STOP_SUCCESS_GR2_T,
   MSG_GROUP_TRACK_STOP_UNTRACKED_T,
   MSG_GROUP_TRACK_STOP_UNTRACKED2_T,
   MSG_GROUP_TRACK_STOP_ERR_TRY_AGAIN_T,
   MSG_GROUP_TRACK_STOP_MAX_T
}MSG_GROUP_TRACK_STOP_TC_TYPE;

struct SafMsgGroupTrackStop {
   SaMsgHandleT *msgHandle;
   SaNameT *queueGroupName;
   SaAisErrorT exp_output;
};

/* saMsgMessageSend */

typedef enum {
   MSG_MESSAGE_SEND_BAD_HDL_T=1,
   MSG_MESSAGE_SEND_FINALIZED_HDL_T,
   MSG_MESSAGE_SEND_NULL_NAME_T,
   MSG_MESSAGE_SEND_NULL_MSG_T,
   MSG_MESSAGE_SEND_ERR_TIMEOUT_T,
   MSG_MESSAGE_SEND_NOT_EXIST_T,
   MSG_MESSAGE_SEND_BAD_GROUP_T,
   MSG_MESSAGE_SEND_SUCCESS_NAME1_T,
   MSG_MESSAGE_SEND_SUCCESS_NAME2_T,
   MSG_MESSAGE_SEND_SUCCESS_NAME3_T,
   MSG_MESSAGE_SEND_SUCCESS_NAME4_T,
   MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME1_T,
   MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME2_T,
   MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME3_T,
   MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME4_T,
   MSG_MESSAGE_SEND_SUCCESS_MSG2_T,
   MSG_MESSAGE_SEND_SUCCESS_Q2_MSG2_T,
   MSG_MESSAGE_SEND_TO_ZERO_QUEUE_T,
   MSG_MESSAGE_SEND_QUEUE_FULL_T,
   MSG_MESSAGE_SEND_BIG_MSG_T,
   MSG_MESSAGE_SEND_ERR_NOT_EXIST_T,
   MSG_MESSAGE_SEND_ERR_NOT_EXIST2_T,
   MSG_MESSAGE_SEND_WITH_BAD_PR_T,
   MSG_MESSAGE_SEND_UNAVAILABLE_T,
   MSG_MESSAGE_SEND_GR_SUCCESS_T,
   MSG_MESSAGE_SEND_GR_SUCCESS_MSG2_T,
   MSG_MESSAGE_SEND_GR_QUEUE_FULL_T,
   MSG_MESSAGE_SEND_ZERO_SIZE_MSG_T,
   MSG_MESSAGE_SEND_MULTI_CAST_GR_FULL_T,
   MSG_MESSAGE_SEND_ERR_TRY_AGAIN_T,
   MSG_MESSAGE_SEND_MAX_T
}MSG_MESSAGE_SEND_TC_TYPE;

struct SafMsgMessageSend {
   SaMsgHandleT *msgHandle;
   SaNameT *destination;
   SaMsgMessageT *message;
   SaTimeT timeout;
   SaAisErrorT exp_output;
};

/* saMsgMessageSendAsync */

typedef enum {
   MSG_MESSAGE_SEND_ASYNC_BAD_HDL_T=1,
   MSG_MESSAGE_SEND_ASYNC_FINALIZED_HDL_T,
   MSG_MESSAGE_SEND_ASYNC_NULL_NAME_T,
   MSG_MESSAGE_SEND_ASYNC_NULL_MSG_T,
   MSG_MESSAGE_SEND_ASYNC_NOT_EXIST_T,
   MSG_MESSAGE_SEND_ASYNC_BAD_GROUP_T,
   MSG_MESSAGE_SEND_ASYNC_BAD_FLAGS_T,
   MSG_MESSAGE_SEND_ASYNC_NO_ACK_SUCCESS_T,
   MSG_MESSAGE_SEND_ASYNC_NAME1_T,
   MSG_MESSAGE_SEND_ASYNC_NAME2_T,
   MSG_MESSAGE_SEND_ASYNC_NAME3_T,
   MSG_MESSAGE_SEND_ASYNC_NAME4_T,
   MSG_MESSAGE_SEND_ASYNC_HDL2_NAME1_T,
   MSG_MESSAGE_SEND_ASYNC_HDL2_NAME2_T,
   MSG_MESSAGE_SEND_ASYNC_HDL2_NAME3_T,
   MSG_MESSAGE_SEND_ASYNC_HDL2_NAME4_T,
   MSG_MESSAGE_SEND_ASYNC_ERR_INIT_T,
   MSG_MESSAGE_SEND_ASYNC_ERR_INIT2_T,
   MSG_MESSAGE_SEND_ASYNC_ERR_NOT_EXIST_T,
   MSG_MESSAGE_SEND_ASYNC_ERR_NOT_EXIST2_T,
   MSG_MESSAGE_SEND_ASYNC_ZERO_QUEUE_T,
   MSG_MESSAGE_SEND_ASYNC_QUEUE_FULL_T,
   MSG_MESSAGE_SEND_ASYNC_WITH_BAD_PR_T,
   MSG_MESSAGE_SEND_ASYNC_BIG_MSG_T,
   MSG_MESSAGE_SEND_ASYNC_UNAVAILABLE_T,
   MSG_MESSAGE_SEND_ASYNC_SUCCESS_Q1_MSG2_T,
   MSG_MESSAGE_SEND_ASYNC_SUCCESS_Q2_MSG2_T,
   MSG_MESSAGE_SEND_ASYNC_GR_SUCCESS_T,
   MSG_MESSAGE_SEND_ASYNC_GR_SUCCESS_MSG2_T,
   MSG_MESSAGE_SEND_ASYNC_GR_QUEUE_FULL_T,
   MSG_MESSAGE_SEND_ASYNC_ZERO_SIZE_MSG_T,
   MSG_MESSAGE_SEND_ASYNC_MULTI_CAST_GR_FULL_T,
   MSG_MESSAGE_SEND_ASYNC_ERR_TRY_AGAIN_T,
   MSG_MESSAGE_SEND_ASYNC_MAX_T
}MSG_MESSAGE_SEND_ASYNC_TC_TYPE;

struct SafMsgMessageSendAsync {
   SaMsgHandleT *msgHandle;
   SaInvocationT invocation;
   SaNameT *destination;
   SaMsgMessageT *message;
   SaMsgAckFlagsT ackFlags;
   SaAisErrorT exp_output;
};

/* saMsgMessageGet */

typedef enum {
   MSG_MESSAGE_GET_BAD_HDL_T=1,
   MSG_MESSAGE_GET_CLOSED_Q_HDL_T,
   MSG_MESSAGE_GET_NULL_MSG_T,
   MSG_MESSAGE_GET_NULL_SENDER_ID_T,
   MSG_MESSAGE_GET_ZERO_TIMEOUT_T,
   MSG_MESSAGE_GET_NULL_SEND_TIME_T,
   MSG_MESSAGE_GET_SUCCESS_T,
   MSG_MESSAGE_GET_NULL_SNDR_NAME_T,
   MSG_MESSAGE_GET_SUCCESS_HDL2_T,
   MSG_MESSAGE_GET_BAD_HDL2_T,
   MSG_MESSAGE_GET_ERR_NO_SPACE_T,
   MSG_MESSAGE_GET_ERR_TIMEOUT_T,
   MSG_MESSAGE_GET_ERR_INTERRUPT_T,
   MSG_MESSAGE_GET_RECV_SUCCESS_T,
   MSG_MESSAGE_GET_ERR_TRY_AGAIN_T,
   MSG_MESSAGE_GET_MAX_T
}MSG_MESSAGE_GET_TC_TYPE;

struct SafMsgMessageGet {
   SaMsgQueueHandleT *queueHandle;
   SaMsgMessageT *message;
   SaTimeT *sendTime;
   SaMsgSenderIdT *senderId;
   SaTimeT timeout;
   SaAisErrorT exp_output;
};

/*saMsgMessageCancel */

typedef enum {
   MSG_MESSAGE_CANCEL_BAD_HDL_T=1,
   MSG_MESSAGE_CANCEL_CLOSED_Q_HDL_T,
   MSG_MESSAGE_CANCEL_NO_BLKING_T,
   MSG_MESSAGE_CANCEL_SUCCESS_T,
   MSG_MESSAGE_CANCEL_ERR_TRY_AGAIN_T,
   MSG_MESSAGE_CANCEL_MAX_T
}MSG_MESSAGE_CANCEL_TC_TYPE;

struct SafMsgMessageCancel {
   SaMsgQueueHandleT *queueHandle;
   SaAisErrorT exp_output;
};

/* saMsgMessageSendReceive */

typedef enum {
   MSG_MESSAGE_SEND_RECV_BAD_HDL_T=1,
   MSG_MESSAGE_SEND_RECV_FINALIZED_HDL_T,
   MSG_MESSAGE_SEND_RECV_NULL_NAME_T,
   MSG_MESSAGE_SEND_RECV_NULL_MSG_T,
   MSG_MESSAGE_SEND_RECV_NULL_STIME_T,
   MSG_MESSAGE_SEND_RECV_ERR_TIMEOUT_T,
   MSG_MESSAGE_SEND_RECV_NULL_RMSG_T,
   MSG_MESSAGE_SEND_RECV_ERR_NOT_EXIST_T,
   MSG_MESSAGE_SEND_RECV_NOT_EXIST_GR_T,
   MSG_MESSAGE_SEND_RECV_ERR_NO_SPACE_T,
   MSG_MESSAGE_SEND_RECV_QUEUE_FULL_T,
   MSG_MESSAGE_SEND_RECV_ZERO_Q_T,
   MSG_MESSAGE_SEND_RECV_BAD_PR_T,
   MSG_MESSAGE_SEND_RECV_SUCCESS_T,
   MSG_MESSAGE_SEND_RECV_SUCCESS_Q2_T,
   MSG_MESSAGE_SEND_RECV_SUCCESS_Q3_T,
   MSG_MESSAGE_SEND_RECV_SUCCESS_MSG2_T,
   MSG_MESSAGE_SEND_RECV_NULL_SNAME_T,
   MSG_MESSAGE_SEND_RECV_UNAVALABLE_T,
   MSG_MESSAGE_SEND_RECV_SUCCESS_GR_T,
   MSG_MESSAGE_SEND_RECV_GR_QUEUE_FULL_T,
   MSG_MESSAGE_SEND_RECV_ZERO_SIZE_MSG_T,
   MSG_MESSAGE_SEND_RECV_MULTI_CAST_GR_T,
   MSG_MESSAGE_SEND_RECV_ERR_TRY_AGAIN_T,
   MSG_MESSAGE_SEND_RECV_MAX_T
}MSG_MESSAGE_SEND_RECV_TC_TYPE;

struct SafMsgMessageSendReceive {
   SaMsgHandleT *msgHandle;
   SaNameT *destination;
   SaMsgMessageT *sendMessage;
   SaMsgMessageT *receiveMessage;
   SaTimeT *replySendTime;
   SaTimeT timeout;
   SaAisErrorT exp_output;
};

/* saMsgMessageReply */

typedef enum {
   MSG_MESSAGE_REPLY_BAD_HANDLE_T=1,
   MSG_MESSAGE_REPLY_FINALIZED_HDL_T,
   MSG_MESSAGE_REPLY_NULL_MSG_T,
   MSG_MESSAGE_REPLY_NULL_SID_T,
   MSG_MESSAGE_REPLY_ERR_NOT_EXIST_T,
   MSG_MESSAGE_REPLY_SUCCESS_T,
   MSG_MESSAGE_REPLY_ERR_NO_SPACE_T,
   MSG_MESSAGE_REPLY_NOT_EXIST_T,
   MSG_MESSAGE_REPLY_NULL_SNDR_NAME_T,
   MSG_MESSAGE_REPLY_ZERO_SIZE_MSG_T,
   MSG_MESSAGE_REPLY_INV_PARAM_T,
   MSG_MESSAGE_REPLY_ERR_TRY_AGAIN_T,
   MSG_MESSAGE_REPLY_MAX_T
}MSG_MESSAGE_REPLY_TC_TYPE;

struct SafMsgMessageReply {
   SaMsgHandleT *msgHandle;
   SaMsgMessageT *replyMessage;
   SaMsgSenderIdT *senderId;
   SaTimeT timeout;
   SaAisErrorT exp_output;
};

/* saMsgMessageReplyAsync */

typedef enum {
   MSG_MESSAGE_REPLY_ASYNC_BAD_HDL_T=1,
   MSG_MESSAGE_REPLY_ASYNC_FINALIZED_HDL_T,
   MSG_MESSAGE_REPLY_ASYNC_NULL_MSG_T,
   MSG_MESSAGE_REPLY_ASYNC_NULL_SID_T,
   MSG_MESSAGE_REPLY_ASYNC_BAD_FLAGS_T,
   MSG_MESSAGE_REPLY_ASYNC_ERR_NOT_EXIST_T,
   MSG_MESSAGE_REPLY_ASYNC_SUCCESS_T,
   MSG_MESSAGE_REPLY_ASYNC_SUCCESS2_T,
   MSG_MESSAGE_REPLY_ASYNC_ERR_INIT_T,
   MSG_MESSAGE_REPLY_ASYNC_ERR_INIT2_T,
   MSG_MESSAGE_REPLY_ASYNC_NOT_EXIST_T,
   MSG_MESSAGE_REPLY_ASYNC_ERR_NO_SPACE_T,
   MSG_MESSAGE_REPLY_ASYNC_NULL_SNDR_NAME_T,
   MSG_MESSAGE_REPLY_ASYNC_ZERO_SIZE_MSG_T,
   MSG_MESSAGE_REPLY_ASYNC_INV_PARAM_T,
   MSG_MESSAGE_REPLY_ASYNC_ERR_TRY_AGAIN_T,
   MSG_MESSAGE_REPLY_ASYNC_MAX_T
}MSG_MESSAGE_REPLY_ASYNC_TC_TYPE;

struct SafMsgMessageReplyAsync {
   SaMsgHandleT *msgHandle;
   SaInvocationT invocation;
   SaMsgMessageT *replyMessage;
   SaMsgSenderIdT *senderId;
   SaMsgAckFlagsT ackFlags;
   SaAisErrorT exp_output;
};

/* Tet list indices */

typedef enum {
   MQSV_ONE_NODE_LIST=1,
   MQSV_MANUAL_TEST_LIST=4,
}MQSV_TET_LIST_IND;


/* MQSv Instance Structure */

typedef struct tet_mqsv_inst {
   int inst_num;
   int tetlist_index;
   int test_case_num;
   int num_of_iter;
   int node_id;
   SaNameT pers_q_name1;
   SaNameT non_pers_q_name1;
   SaNameT zero_q_name;
   SaNameT pers_q_name2;
   SaNameT non_pers_q_name2;
   SaNameT q_grp_name1;
   SaNameT q_grp_name2;
   SaNameT q_grp_name3;
}TET_MQSV_INST;


extern const char *saMsgQueueOpenFlags_string[];
extern const char *saMsgQueueCreationFlags_string[];
extern const char *saMsgAckFlags_string[];
extern const char *saMsgQueueGroupChanges_string[];
extern const char *saMsgQueueGroupPolicy_string[];
extern const char *saMsgGroupTrackFlags_string[];
extern const char *mqsv_saf_error_string[];


extern void mqsv_fill_grp_notif_buffer(SaMsgQueueGroupNotificationBufferT *buffer,SaUint32T num_of_items,
                           SaMsgQueueGroupNotificationT *notif);
extern void mqsv_fill_msg_version(SaVersionT *version,SaUint8T rel_code,SaUint8T mjr_ver,SaUint8T mnr_ver);
extern void mqsv_fill_rcv_message(SaMsgMessageT *msg,void *data,SaSizeT size,SaNameT *sndr_name);

void copy_notif_buffer(SaMsgQueueGroupNotificationBufferT *buffer);
void App_saMsgQueueOpenCallback(SaInvocationT invocation,
                                SaMsgQueueHandleT queueHandle,
                                SaAisErrorT error);
void App_saMsgQueueGroupTrackCallback(const SaNameT *queueGroupName,
                                      const SaMsgQueueGroupNotificationBufferT *buffer,
                                      SaUint32T   num_mem,
                                      SaAisErrorT rc);
void App_saMsgMessageDeliveredCallback(SaInvocationT invocation, SaAisErrorT error);
void App_saMsgMessageReceivedCallback(SaMsgQueueHandleT queueHandle);
void App_saMsgMessageReceivedCallback_withMsgGet(SaMsgQueueHandleT queueHandle);
void App_saMsgMessageReceivedCallback_withMsgGet_cleanup(SaMsgQueueHandleT queueHandle);
void App_saMsgMessageReceivedCallback_withReply(SaMsgQueueHandleT queueHandle);
void App_saMsgMessageReceivedCallback_withReply_nospace(SaMsgQueueHandleT queueHandle);
void App_saMsgMessageReceivedCallback_withReply_nospace(SaMsgQueueHandleT queueHandle);
void App_saMsgMessageReceivedCallback_withReplyAsync(SaMsgQueueHandleT queueHandle);
void mqsv_fill_msg_clbks(SaMsgCallbacksT *clbk,SaMsgQueueOpenCallbackT opn_clbk,
                         SaMsgQueueGroupTrackCallbackT trk_clbk,
                         SaMsgMessageDeliveredCallbackT del_clbk,
                         SaMsgMessageReceivedCallbackT rcv_clbk);
void mqsv_fill_q_cr_attribs(SaMsgQueueCreationAttributesT *attr,SaMsgQueueCreationFlagsT cr_flgs,
                     SaSizeT size0,SaSizeT size1,SaSizeT size2,SaSizeT size3,SaTimeT ret_time);
void mqsv_fill_q_grp_names(SaNameT *name,char *string,char *inst_num_char);
void mqsv_fill_send_message(SaMsgMessageT *msg,SaUint32T type,SaUint32T version,SaSizeT size,
                       SaNameT *sndr_name,void *data,SaUint8T pri);
void init_mqsv_test_env(void);
void mqsv_print_testcase(char *string);
void mqsv_result(int result);

void mqsv_it_init_01(void);
void mqsv_it_init_02(void);
void mqsv_it_init_03(void);
void mqsv_it_init_04(void);
void mqsv_it_init_05(void);
void mqsv_it_init_06(void);
void mqsv_it_init_07(void);
void mqsv_it_init_08(void);
void mqsv_it_init_09(void);
void mqsv_it_init_10(void);
void mqsv_it_selobj_01(void);
void mqsv_it_selobj_02(void);
void mqsv_it_selobj_03(void);
void mqsv_it_selobj_04(void);
void mqsv_it_dispatch_01(void);
void mqsv_it_dispatch_02(void);
void mqsv_it_dispatch_03(void);
void mqsv_it_dispatch_04(void);
void mqsv_it_dispatch_05(void);
void mqsv_it_dispatch_06(void);
void mqsv_it_dispatch_07(void);
void mqsv_it_dispatch_08(void);
void mqsv_it_dispatch_09(void);
void mqsv_it_finalize_01(void);
void mqsv_it_finalize_02(void);
void mqsv_it_finalize_03(void);
void mqsv_it_finalize_04(void);
void mqsv_it_finalize_05(void);
void mqsv_it_finalize_06(void);
void mqsv_it_qopen_01(void);
void mqsv_it_qopen_02(void);
void mqsv_it_qopen_03(void);
void mqsv_it_qopen_04(void);
void mqsv_it_qopen_05(void);
void mqsv_it_qopen_06(void);
void mqsv_it_qopen_07(void);
void mqsv_it_qopen_08(void);
void mqsv_it_qopen_09(void);
void mqsv_it_qopen_10(void);
void mqsv_it_qopen_11(void);
void mqsv_it_qopen_12(void);
void mqsv_it_qopen_13(void);
void mqsv_it_qopen_14(void);
void mqsv_it_qopen_15(void);
void mqsv_it_qopen_16(void);
void mqsv_it_qopen_17(void);
void mqsv_it_qopen_18(void);
void mqsv_it_qopen_19(void);
void mqsv_it_qopen_20(void);
void mqsv_it_qopen_21(void);
void mqsv_it_qopen_22(void);
void mqsv_it_qopen_23(void);
void mqsv_it_qopen_24(void);
void mqsv_it_qopen_25(void);
void mqsv_it_qopen_26(void);
void mqsv_it_qopen_27(void);
void mqsv_it_qopen_28(void);
void mqsv_it_qopen_29(void);
void mqsv_it_qopen_30(void);
void mqsv_it_qopen_31(void);
void mqsv_it_qopen_32(void);
void mqsv_it_qopen_33(void);
void mqsv_it_qopen_34(void);
void mqsv_it_qopen_35(void);
void mqsv_it_qopen_36(void);
void mqsv_it_qopen_37(void);
void mqsv_it_qopen_38(void);
void mqsv_it_qopen_39(void);
void mqsv_it_qopen_40(void);
void mqsv_it_qopen_41(void);
void mqsv_it_qopen_42(void);
void mqsv_it_qopen_43(void);
void mqsv_it_qopen_44(void);
void mqsv_it_qopen_45(void);
void mqsv_it_qopen_46(void);
void mqsv_it_qopen_47(void);
void mqsv_it_qopen_48(void);
void mqsv_it_qopen_49(void);
void mqsv_it_qopen_50(void);
void mqsv_it_qopen_51(void);
void mqsv_it_close_01(void);
void mqsv_it_close_02(void);
void mqsv_it_close_03(void);
void mqsv_it_close_04(void);
void mqsv_it_close_05(void);
void mqsv_it_close_06(void);
void mqsv_it_close_07(void);
void mqsv_it_qstatus_01(void);
void mqsv_it_qstatus_02(void);
void mqsv_it_qstatus_03(void);
void mqsv_it_qstatus_04(void);
void mqsv_it_qstatus_05(void);
void mqsv_it_qstatus_06(void);
void mqsv_it_qstatus_07(void);
void mqsv_it_qunlink_01(void);
void mqsv_it_qunlink_02(void);
void mqsv_it_qunlink_03(void);
void mqsv_it_qunlink_04(void);
void mqsv_it_qunlink_05(void);
void mqsv_it_qunlink_06(void);
void mqsv_it_qunlink_07(void);
void mqsv_it_qgrp_create_01(void);
void mqsv_it_qgrp_create_02(void);
void mqsv_it_qgrp_create_03(void);
void mqsv_it_qgrp_create_04(void);
void mqsv_it_qgrp_create_05(void);
void mqsv_it_qgrp_create_06(void);
void mqsv_it_qgrp_create_07(void);
void mqsv_it_qgrp_create_08(void);
void mqsv_it_qgrp_create_09(void);
void mqsv_it_qgrp_create_10(void);
void mqsv_it_qgrp_create_11(void);
void mqsv_it_qgrp_insert_01(void);
void mqsv_it_qgrp_insert_02(void);
void mqsv_it_qgrp_insert_03(void);
void mqsv_it_qgrp_insert_04(void);
void mqsv_it_qgrp_insert_05(void);
void mqsv_it_qgrp_insert_06(void);
void mqsv_it_qgrp_insert_07(void);
void mqsv_it_qgrp_insert_08(void);
void mqsv_it_qgrp_insert_09(void);
void mqsv_it_qgrp_remove_01(void);
void mqsv_it_qgrp_remove_02(void);
void mqsv_it_qgrp_remove_03(void);
void mqsv_it_qgrp_remove_04(void);
void mqsv_it_qgrp_remove_05(void);
void mqsv_it_qgrp_remove_06(void);
void mqsv_it_qgrp_remove_07(void);
void mqsv_it_qgrp_delete_01(void);
void mqsv_it_qgrp_delete_02(void);
void mqsv_it_qgrp_delete_03(void);
void mqsv_it_qgrp_delete_04(void);
void mqsv_it_qgrp_delete_05(void);
void mqsv_it_qgrp_delete_06(void);
void mqsv_it_qgrp_delete_07(void);
void mqsv_it_qgrp_track_01(void);
void mqsv_it_qgrp_track_02(void);
void mqsv_it_qgrp_track_03(void);
void mqsv_it_qgrp_track_04(void);
void mqsv_it_qgrp_track_05(void);
void mqsv_it_qgrp_track_06(void);
void mqsv_it_qgrp_track_07(void);
void mqsv_it_qgrp_track_08(void);
void mqsv_it_qgrp_track_09(void);
void mqsv_it_qgrp_track_10(void);
void mqsv_it_qgrp_track_11(void);
void mqsv_it_qgrp_track_12(void);
void mqsv_it_qgrp_track_13(void);
void mqsv_it_qgrp_track_14(void);
void mqsv_it_qgrp_track_15(void);
void mqsv_it_qgrp_track_16(void);
void mqsv_it_qgrp_track_17(void);
void mqsv_it_qgrp_track_18(void);
void mqsv_it_qgrp_track_19(void);
void mqsv_it_qgrp_track_20(void);
void mqsv_it_qgrp_track_21(void);
void mqsv_it_qgrp_track_22(void);
void mqsv_it_qgrp_track_23(void);
void mqsv_it_qgrp_track_24(void);
void mqsv_it_qgrp_track_25(void);
void mqsv_it_qgrp_track_26(void);
void mqsv_it_qgrp_track_27(void);
void mqsv_it_qgrp_track_28(void);
void mqsv_it_qgrp_track_29(void);
void mqsv_it_qgrp_track_30(void);
void mqsv_it_qgrp_track_31(void);
void mqsv_it_qgrp_track_32(void);
void mqsv_it_qgrp_track_33(void);
void mqsv_it_qgrp_track_stop_01(void);
void mqsv_it_qgrp_track_stop_02(void);
void mqsv_it_qgrp_track_stop_03(void);
void mqsv_it_qgrp_track_stop_04(void);
void mqsv_it_qgrp_track_stop_05(void);
void mqsv_it_qgrp_track_stop_06(void);
void mqsv_it_qgrp_track_stop_07(void);
void mqsv_it_msg_send_01(void);
void mqsv_it_msg_send_02(void);
void mqsv_it_msg_send_03(void);
void mqsv_it_msg_send_04(void);
void mqsv_it_msg_send_05(void);
void mqsv_it_msg_send_06(void);
void mqsv_it_msg_send_07(void);
void mqsv_it_msg_send_08(void);
void mqsv_it_msg_send_09(void);
void mqsv_it_msg_send_10(void);
void mqsv_it_msg_send_11(void);
void mqsv_it_msg_send_12(void);
void mqsv_it_msg_send_13(void);
void mqsv_it_msg_send_14(void);
void mqsv_it_msg_send_15(void);
void mqsv_it_msg_send_16(void);
void mqsv_it_msg_send_17(void);
void mqsv_it_msg_send_18(void);
void mqsv_it_msg_send_19(void);
void mqsv_it_msg_send_20(void);
void mqsv_it_msg_send_21(void);
void mqsv_it_msg_send_22(void);
void mqsv_it_msg_send_23(void);
void mqsv_it_msg_send_24(void);
void mqsv_it_msg_send_25(void);
void mqsv_it_msg_send_26(void);
void mqsv_it_msg_send_27(void);
void mqsv_it_msg_send_28(void);
void mqsv_it_msg_send_29(void);
void mqsv_it_msg_send_30(void);
void mqsv_it_msg_send_31(void);
void mqsv_it_msg_send_32(void);
void mqsv_it_msg_send_33(void);
void mqsv_it_msg_send_34(void);
void mqsv_it_msg_send_35(void);
void mqsv_it_msg_send_36(void);
void mqsv_it_msg_send_37(void);
void mqsv_it_msg_send_38(void);
void mqsv_it_msg_send_39(void);
void mqsv_it_msg_send_40(void);
void mqsv_it_msg_send_41(void);
void mqsv_it_msg_send_42(void);
void mqsv_it_msg_send_43(void);
void mqsv_it_msg_get_01(void);
void mqsv_it_msg_get_02(void);
void mqsv_it_msg_get_03(void);
void mqsv_it_msg_get_04(void);
void mqsv_it_msg_get_05(void);
void mqsv_it_msg_get_06(void);
void mqsv_it_msg_get_07(void);
void mqsv_it_msg_get_08(void);
void mqsv_it_msg_get_09(void);
void mqsv_it_msg_get_10(void);
void mqsv_it_msg_get_11(void);
void mqsv_it_msg_get_12(void);
void mqsv_it_msg_get_13(void);
void mqsv_it_msg_get_14(void);
void mqsv_it_msg_get_15(void);
void mqsv_it_msg_get_16(void);
void mqsv_it_msg_get_17(void);
void mqsv_it_msg_get_18(void);
void mqsv_it_msg_get_19(void);
void mqsv_it_msg_get_20(void);
void mqsv_it_msg_get_21(void);
void mqsv_it_msg_get_22(void);
void mqsv_it_msg_cancel_01(void);
void mqsv_it_msg_cancel_02(void);
void mqsv_it_msg_cancel_03(void);
void mqsv_it_msg_cancel_04(void);
void mqsv_it_msg_sendrcv_01(void);
void mqsv_it_msg_sendrcv_02(void);
void mqsv_it_msg_sendrcv_03(void);
void mqsv_it_msg_sendrcv_04(void);
void mqsv_it_msg_sendrcv_05(void);
void mqsv_it_msg_sendrcv_06(void);
void mqsv_it_msg_sendrcv_07(void);
void mqsv_it_msg_sendrcv_08(void);
void mqsv_it_msg_sendrcv_09(void);
void mqsv_it_msg_sendrcv_10(void);
void mqsv_it_msg_sendrcv_11(void);
void mqsv_it_msg_sendrcv_12(void);
void mqsv_it_msg_sendrcv_13(void);
void mqsv_it_msg_sendrcv_14(void);
void mqsv_it_msg_sendrcv_15(void);
void mqsv_it_msg_sendrcv_16(void);
void mqsv_it_msg_sendrcv_17(void);
void mqsv_it_msg_sendrcv_18(void);
void mqsv_it_msg_sendrcv_19(void);
void mqsv_it_msg_sendrcv_20(void);
void mqsv_it_msg_sendrcv_21(void);
void mqsv_it_msg_sendrcv_22(void);
void mqsv_it_msg_sendrcv_23(void);
void mqsv_it_msg_sendrcv_24(void);
void mqsv_it_msg_sendrcv_25(void);
void mqsv_it_msg_sendrcv_26(void);
void mqsv_it_msg_reply_01(void);
void mqsv_it_msg_reply_02(void);
void mqsv_it_msg_reply_03(void);
void mqsv_it_msg_reply_04(void);
void mqsv_it_msg_reply_05(void);
void mqsv_it_msg_reply_06(void);
void mqsv_it_msg_reply_07(void);
void mqsv_it_msg_reply_08(void);
void mqsv_it_msg_reply_09(void);
void mqsv_it_msg_reply_10(void);
void mqsv_it_msg_reply_11(void);
void mqsv_it_msg_reply_12(void);
void mqsv_it_msg_reply_13(void);
void mqsv_it_msg_reply_14(void);
void mqsv_it_msg_reply_15(void);
void mqsv_it_msg_reply_16(void);
void mqsv_it_msg_reply_17(void);
void mqsv_it_msg_reply_18(void);
void mqsv_it_msg_reply_19(void);
void mqsv_it_msg_reply_20(void);
void mqsv_it_msg_reply_21(void);
void mqsv_it_msg_reply_22(void);
void mqsv_it_msg_reply_23(void);
void mqsv_it_msg_reply_24(void);
void mqsv_it_msgqs_01(void);
void mqsv_it_msgqs_02(void);
void mqsv_it_msgqs_03(void);
void mqsv_it_msgqs_04(void);
void mqsv_it_msgqs_05(void);
void mqsv_it_msgqs_06(void);
void mqsv_it_msgqs_07(void);
void mqsv_it_msgqs_08(void);
void mqsv_it_msgqs_09(void);
void mqsv_it_msgqs_10(void);
void mqsv_it_msgqs_11(void);
void mqsv_it_msgqs_12(void);
void mqsv_it_msgqs_13(void);
void mqsv_it_msgqs_14(void);
void mqsv_it_msgqs_15(void);
void mqsv_it_msgqs_16(void);
void mqsv_it_msgqs_17(void);
void mqsv_it_msgqs_18(void);
void mqsv_it_msgqs_19(void);
void mqsv_it_msgqs_20(void);
void mqsv_it_msgqs_21(void);
void mqsv_it_msgqs_22(void);
void mqsv_it_msgqs_23(void);
void mqsv_it_msgq_grps_01(void);
void mqsv_it_msgq_grps_02(void);
void mqsv_it_msgq_grps_03(void);
void mqsv_it_msgq_grps_04(void);
void mqsv_it_msgq_grps_05(void);
void mqsv_it_msgq_grps_06(void);
void mqsv_it_msgq_grps_07(void);
void mqsv_it_msgq_grps_08(void);
void mqsv_it_msgq_grps_09(void);
void mqsv_it_msgq_grps_10(void);
void mqsv_it_msgq_grps_11(void);
void mqsv_it_msgq_grps_12(void);
void mqsv_it_msg_delprop_01(void);
void mqsv_it_msg_delprop_02(void);
void mqsv_it_msg_delprop_03(void);
void mqsv_it_msg_delprop_04(void);
void mqsv_it_msg_delprop_05(void);
void mqsv_it_msg_delprop_06(void);
void mqsv_it_msg_delprop_07(void);
void mqsv_it_msg_delprop_08(void);
void mqsv_it_msg_delprop_09(void);
void mqsv_it_msg_delprop_10(void);
void mqsv_it_msg_delprop_11(void);
void mqsv_it_msg_delprop_12(void);
void mqsv_it_msg_delprop_13(void);
void mqsv_it_err_try_again_01(void);
void mqsv_it_err_try_again_02(void);
void mqsv_it_err_try_again_03(void);

void App_saMsgMessageReceivedCallback_withReply_nullSname(SaMsgQueueHandleT queueHandle);
void init_mqsv_test_env(void);
void tet_mqsv_get_inputs(TET_MQSV_INST *inst);
void tet_mqsv_fill_inputs(TET_MQSV_INST *inst);
void tet_mqsv_start_instance(TET_MQSV_INST *inst);
void tet_run_mqsv_app(void);
void tet_run_mqsv_dist_cases(void);
void print_qinfo(SaNameT *qname,SaMsgQueueCreationAttributesT *cr_attr,SaMsgQueueOpenFlagsT op_flgs);
void print_queueStatus(SaMsgQueueStatusT *queueStatus);
void mqsv_sendrecv(NCSCONTEXT arg);
void mqsv_cancel_msg(NCSCONTEXT arg);
void mqsv_selection_thread_blocking (NCSCONTEXT arg);
void mqsv_selection_thread_one (NCSCONTEXT arg);
void mqsv_createthread_one(SaMsgHandleT *msgHandle);
void mqsv_selection_thread_all (NCSCONTEXT arg);
void mqsv_createthread_all(SaMsgHandleT *msgHandle);
void mqsv_selection_thread_all_loop (NCSCONTEXT arg);
void mqsv_createthread_all_loop(void);
void mqsv_clean_output_params(void);
void mqsv_clean_clbk_params(void);
void mqsv_clean_q_status(void);

#endif  // MSG_APITEST_TET_MQSV_H_
