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

#ifndef MSG_APITEST_TET_MQA_CONF_H_
#define MSG_APITEST_TET_MQA_CONF_H_

typedef struct mqa_inv_params
{
   SaMsgHandleT inv_msg_hdl;
   SaVersionT inv_version;
   SaVersionT inv_ver_bad_rel_code;
   SaVersionT inv_ver_not_supp;
   SaMsgQueueCreationAttributesT inv_cr_attribs;
   SaMsgQueueHandleT inv_q_hdl;
   SaMsgQueueGroupNotificationBufferT inv_notif_buf;
   SaMsgSenderIdT inv_sender_id;
}MQA_INV_PARAMS;

typedef struct mqa_test_env
{
   MQA_INV_PARAMS inv_params;

   SaMsgHandleT msg_hdl1;
   SaMsgHandleT msg_hdl2;
   SaMsgHandleT null_clbks_msg_hdl;
   SaMsgHandleT null_rcv_clbk_msg_hdl;

   SaMsgCallbacksT gen_clbks;
   SaMsgCallbacksT null_clbks;
   SaMsgCallbacksT null_del_clbks;
   SaMsgCallbacksT null_rcv_clbks;

   SaVersionT version;

   SaSelectionObjectT sel_obj;

   SaNameT pers_q;
   SaNameT non_pers_q;
   SaNameT zero_q;
   SaNameT pers_q2;
   SaNameT non_pers_q2;

   SaMsgQueueCreationAttributesT pers_cr_attribs;
   SaMsgQueueCreationAttributesT npers_cr_attribs;
   SaMsgQueueCreationAttributesT small_cr_attribs;
   SaMsgQueueCreationAttributesT big_cr_attribs;
   SaMsgQueueCreationAttributesT zero_size_cr_attribs;
   SaMsgQueueCreationAttributesT zero_ret_time_cr_attribs;

   SaMsgQueueHandleT pers_q_hdl;
   SaMsgQueueHandleT npers_q_hdl;
   SaMsgQueueHandleT pers_q_hdl2;
   SaMsgQueueHandleT npers_q_hdl2;

   SaMsgQueueStatusT q_status;

   SaNameT qgroup1;
   SaNameT qgroup2;
   SaNameT qgroup3;

   SaMsgQueueGroupNotificationBufferT buffer_null_notif;
   SaMsgQueueGroupNotificationBufferT buffer_non_null_notif;
   SaMsgQueueGroupNotificationBufferT no_space_notif_buf;

   SaTimeT send_time;
   SaTimeT reply_send_time;

   SaNameT sender_name;
   SaNameT rcv_sender_name;

   char *send_data;
   char *rcv_data;

   SaMsgMessageT send_msg;
   SaMsgMessageT send_msg_null_sndr_name;
   SaMsgMessageT send_big_msg;
   SaMsgMessageT send_msg_bad_pr;
   SaMsgMessageT send_msg_zero_size;

   SaMsgMessageT rcv_msg;
   SaMsgMessageT rcv_msg_null_sndr_name;
   SaMsgMessageT no_space_rcv_msg;

   SaMsgMessageT reply_msg;
   SaMsgMessageT reply_msg_null_sndr_name;
   SaMsgMessageT no_space_reply_msg;

   SaMsgSenderIdT sender_id;

   /* Callback Info */

   SaInvocationT open_clbk_invo;
   SaAisErrorT open_clbk_err;
   SaMsgQueueHandleT open_clbk_qhdl;

   SaInvocationT del_clbk_invo;
   SaAisErrorT del_clbk_err;

   SaMsgQueueHandleT rcv_clbk_qhdl;

   SaAisErrorT track_clbk_err;
   SaNameT track_clbk_grp_name;
   SaUint32T track_clbk_num_mem;
   SaMsgQueueGroupNotificationBufferT track_clbk_notif;

}MQA_TEST_ENV;

extern MQA_TEST_ENV gl_mqa_env;

typedef enum
{
   TEST_NONCONFIG_MODE = 0,
   TEST_CONFIG_MODE,
   TEST_CLEANUP_MODE,
   TEST_USER_DEFINED_MODE
}MQSV_CONFIG_FLAG;

typedef enum
{
   MSG_RESTORE_INIT_BAD_VERSION_T = 0,
   MSG_RESTORE_INIT_BAD_REL_CODE_T,
   MSG_RESTORE_INIT_BAD_MAJOR_VER_T,
   MSG_RESTORE_GROUP_TRACK_CURRENT_T,
   MSG_RESTORE_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T,
   MSG_RESTORE_GROUP_TRACK_CUR_NO_SPACE_T,
   MSG_RESTORE_MESSAGE_GET_SUCCESS_T,
   MSG_RESTORE_MESSAGE_GET_ZERO_TIMEOUT_T,
   MSG_RESTORE_MESSAGE_GET_NULL_SNDR_NAME_T,
   MSG_RESTORE_MESSAGE_GET_ERR_NO_SPACE_T,
   MSG_RESTORE_MESSAGE_SEND_RECV_NULL_STIME_T,
   MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T,
   MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_MSG2_T,
   MSG_RESTORE_MESSAGE_SEND_RECV_ERR_NO_SPACE_T
}MQSV_RESTORE_OPT;

typedef enum
{
   MSG_CLEAN_INIT_SUCCESS_T = 0,
   MSG_CLEAN_INIT_NULL_CBKS_T,
   MSG_CLEAN_INIT_NULL_CLBK_PARAM_T,
   MSG_CLEAN_INIT_SUCCESS_HDL2_T,
   MSG_CLEAN_INIT_NULL_CBKS2_T,
   MSG_CLEAN_INIT_NULL_RCV_CBK_T
}MQSV_INIT_CLEANUP_OPT;

typedef enum
{
   MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T = 0,
   MSG_CLEAN_QUEUE_OPEN_ASYNC_EMPTY_CREATE_T,
   MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_RECV_CLBK_SUCCESS_T,
   MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T,
   MSG_CLEAN_QUEUE_OPEN_EMPTY_CREATE_T,
   MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,
   MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T,
   MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_RECV_CLBK_SUCCESS_T,
   MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T,
   MSG_CLEAN_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T,
   MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK2_T,
   MSG_CLEAN_QUEUE_OPEN_ASYNC_ZERO_RET_T,
   MSG_CLEAN_QUEUE_OPEN_ZERO_RET_T,
   MSG_CLEAN_QUEUE_OPEN_NPERS_RECV_CLBK2_T,
   MSG_CLEAN_QUEUE_OPEN_ASYNC_ZERO_SIZE_T,
   MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T,
   MSG_CLEAN_QUEUE_OPEN_AFTER_FINALIZE
}MQSV_Q_CLEANUP_OPT;

typedef enum
{
   MSG_CLEAN_GROUP_CREATE_SUCCESS_T = 0,
   MSG_CLEAN_GROUP_CREATE_BROADCAST_T,
   MSG_CLEAN_GROUP_CREATE_LCL_BSTQ_NOT_SUPP_T,
   MSG_CLEAN_GROUP_CREATE_SUCCESS_GR2_T,
   MSG_CLEAN_GROUP_CREATE_LOCAL_RR_T,
   MSG_CLEAN_GROUP_CREATE_BROADCAST2_T
}MQSV_Q_GRP_CLEANUP_OPT;

typedef enum
{
   MSG_STOP_GROUP_TRACK_CHANGES_T = 0,
   MSG_STOP_GROUP_TRACK_CHGS_ONLY_T,
   MSG_STOP_GROUP_TRACK_CUR_CHLY_NUL_BUF_T,
   MSG_STOP_GROUP_TRACK_CUR_CH_NUL_BUF_T,
   MSG_STOP_GROUP_TRACK_CUR_CH_T,
   MSG_STOP_GROUP_TRACK_CUR_CHLY_T,
   MSG_STOP_GROUP_TRACK_CHANGES_GR2_T
}MQSV_Q_GRP_TRACK_STOP_OPT;


/* MQSV thread creation functions */

extern void mqsv_createthread(SaMsgHandleT *msgHandle);
extern void mqsv_createcancelthread(SaMsgQueueHandleT *queueHandle);
extern void mqsv_create_sendrecvthread(MSG_MESSAGE_SEND_RECV_TC_TYPE *tc);
extern void mqsv_createthread_all_loop();

extern void tet_queueStatus(SaMsgQueueStatusT *queueStatus);
extern void groupTrackInfo(SaMsgQueueGroupNotificationBufferT *buffer);
extern void msgDump(SaMsgMessageT *msg);


extern int tet_test_msgInitialize(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgSelectionObject(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgDispatch(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgFinalize(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgQueueOpen(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgQueueOpenAsync(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgQueueStatusGet(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgQueueClose(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgQueueUnlink(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgGroupCreate(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgGroupInsert(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgGroupRemove(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgGroupDelete(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgGroupTrack(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgGroupTrackStop(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgMessageSend(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgMessageSendAsync(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgMessageGet(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgMessageCancel(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgMessageSendReceive(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgMessageReply(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_msgMessageReplyAsync(int i,MQSV_CONFIG_FLAG flg);

extern int tet_test_red_msgInitialize(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgSelectionObject(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgDispatch(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgFinalize(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgQueueOpen(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgQueueOpenAsync(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgQueueStatusGet(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgQueueClose(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgQueueUnlink(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgGroupCreate(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgGroupInsert(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgGroupRemove(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgGroupDelete(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgGroupTrack(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgGroupTrackStop(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgMessageSend(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgMessageSendAsync(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgMessageGet(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgMessageCancel(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgMessageSendReceive(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgMessageReply(int i,MQSV_CONFIG_FLAG flg);
extern int tet_test_red_msgMessageReplyAsync(int i,MQSV_CONFIG_FLAG flg);

/* Restore function */
extern void mqsv_restore_params(MQSV_RESTORE_OPT opt);

/* Cleanup functions */
extern void mqsv_clean_output_params();
extern void mqsv_clean_clbk_params();
extern void mqsv_clean_q_status();
extern void mqsv_init_cleanup(MQSV_INIT_CLEANUP_OPT opt);
extern void mqsv_q_cleanup(MQSV_Q_CLEANUP_OPT opt);
extern void mqsv_q_grp_cleanup(MQSV_Q_GRP_CLEANUP_OPT opt);
extern void mqsv_q_grp_track_stop(MQSV_Q_GRP_TRACK_STOP_OPT opt);
void mqsv_init_red_cleanup(MQSV_INIT_CLEANUP_OPT opt);
void mqsv_q_red_cleanup(MQSV_Q_CLEANUP_OPT opt);
void mqsv_q_grp_red_cleanup(MQSV_Q_GRP_CLEANUP_OPT opt);
int mqsv_test_result(SaAisErrorT rc,SaAisErrorT exp_out,char *test_case,MQSV_CONFIG_FLAG flg);

#endif  // MSG_APITEST_TET_MQA_CONF_H_
