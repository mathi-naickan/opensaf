#if (TET_A == 1)

#include "tet_mqsv.h"
#include "tet_mqa_conf.h"
#include "ncs_main_papi.h"

int gl_mqsv_inst_num;
int gl_nodeId;
int gl_get_result;
int gl_reply_result;
int gl_del_clbk_iter;
int gl_rcv_clbk_iter;
int gl_track_clbk_iter;
int gl_tetlist_index;
int gl_msg_red_flg;
int gl_mqsv_wait_time;
int gl_red_node;
SaTimeT gl_queue_ret_time = 10000000000ULL;
SaMsgSenderIdT gl_sender_id;

int TET_MQSV_NODE1;
int TET_MQSV_NODE2;
int TET_MQSV_NODE3;
MQA_TEST_ENV gl_mqa_env;

extern struct tet_testlist mqsv_twonode_testlist_node1[];
extern struct tet_testlist mqsv_twonode_testlist_node2[];
extern struct tet_testlist mqsv_threenode_testlist[];
extern struct tet_testlist mqsv_twonode_manual_testlist_node1[];
extern struct tet_testlist mqsv_twonode_manual_testlist_node2[];
extern struct tet_testlist mqsv_red_onenode_testlist[];
extern struct tet_testlist mqsv_red_twonode_testlist_node1[];
extern struct tet_testlist mqsv_red_twonode_testlist_node2[];
extern int gl_sync_pointnum;

const char *saMsgQueueOpenFlags_string[] = {
   "ZERO",
   "SA_MSG_QUEUE_CREATE",
   "SA_MSG_QUEUE_RECEIVE_CALLBACK",
   "SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK",
   "SA_MSG_QUEUE_EMPTY",
   "SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_EMPTY",
   "SA_MSG_QUEUE_EMPTY | SA_MSG_QUEUE_RECEIVE_CALLBACK",
   "SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK | SA_MSG_QUEUE_EMPTY",
};

const char *saMsgQueueCreationFlags_string[] = {
   "SA_MSG_QUEUE_NON_PERSISTENT",
   "SA_MSG_QUEUE_PERSISTENT",
};

const char *saMsgAckFlags_string[] = {
   "NO_DELIVERED_ACK",
   "SA_MSG_MESSAGE_DELIVERED_ACK",
};

const char *saMsgQueueGroupChanges_string[] = {
   "SA_MSG_QUEUE_GROUP_NOT_VALID",
   "SA_MSG_QUEUE_GROUP_NO_CHANGE",
   "SA_MSG_QUEUE_GROUP_ADDED",
   "SA_MSG_QUEUE_GROUP_REMOVED",
   "SA_MSG_QUEUE_GROUP_STATE_CHANGED",
};
 
const char *saMsgQueueGroupPolicy_string[] = {
   "SA_MSG_QUEUE_GROUP_INVALID_POLICY",
   "SA_MSG_QUEUE_GROUP_ROUND_ROBIN",
   "SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN",
   "SA_MSG_QUEUE_GROUP_LOCAL_BEST_QUEUE",
   "SA_MSG_QUEUE_GROUP_BROADCAST",
};

const char *saMsgGroupTrackFlags_string[] = {
   "ZERO",
   "SA_TRACK_CURRENT",
   "SA_TRACK_CHANGES",
   "SA_TRACK_CURRENT | SA_TRACK_CHANGES",
   "SA_TRACK_CHANGES_ONLY",
   "SA_TRACK_CURRENT | SA_TRACK_CHANGES_ONLY",
   "SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY",
   "SA_TRACK_CURRENT | SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY",
};

const char *mqsv_saf_error_string[] = {
   "SA_AIS_NOT_VALID",
   "SA_AIS_OK",
   "SA_AIS_ERR_LIBRARY",
   "SA_AIS_ERR_VERSION",
   "SA_AIS_ERR_INIT",
   "SA_AIS_ERR_TIMEOUT",
   "SA_AIS_ERR_TRY_AGAIN",
   "SA_AIS_ERR_INVALID_PARAM",
   "SA_AIS_ERR_NO_MEMORY",
   "SA_AIS_ERR_BAD_HANDLE",
   "SA_AIS_ERR_BUSY",
   "SA_AIS_ERR_ACCESS",
   "SA_AIS_ERR_NOT_EXIST",
   "SA_AIS_ERR_NAME_TOO_LONG",
   "SA_AIS_ERR_EXIST",
   "SA_AIS_ERR_NO_SPACE",
   "SA_AIS_ERR_INTERRUPT",
   "SA_AIS_ERR_NAME_NOT_FOUND",
   "SA_AIS_ERR_NO_RESOURCES",
   "SA_AIS_ERR_NOT_SUPPORTED",
   "SA_AIS_ERR_BAD_OPERATION",
   "SA_AIS_ERR_FAILED_OPERATION",
   "SA_AIS_ERR_MESSAGE_ERROR",
   "SA_AIS_ERR_QUEUE_FULL",
   "SA_AIS_ERR_QUEUE_NOT_AVAILABLE",
   "SA_AIS_ERR_BAD_FLAGS",
   "SA_AIS_ERR_TOO_BIG",
   "SA_AIS_ERR_NO_SECTIONS",
};

void copy_notif_buffer(SaMsgQueueGroupNotificationBufferT *buffer)
{
   memcpy(&gl_mqa_env.track_clbk_notif,buffer,sizeof(SaMsgQueueGroupNotificationBufferT));
   gl_mqa_env.track_clbk_notif.notification = NULL;
   if(buffer->numberOfItems)
   {
      gl_mqa_env.track_clbk_notif.notification = (SaMsgQueueGroupNotificationT *)
         calloc(buffer->numberOfItems,sizeof(SaMsgQueueGroupNotificationT));
      memcpy(gl_mqa_env.track_clbk_notif.notification,buffer->notification,
         buffer->numberOfItems * sizeof(SaMsgQueueGroupNotificationT));
   }
}

/* ********* MQSV Callback Functions ********* */

void App_saMsgQueueOpenCallback(SaInvocationT invocation,
                                SaMsgQueueHandleT queueHandle,
                                SaAisErrorT error)
{
   gl_mqa_env.open_clbk_invo = invocation;
   gl_mqa_env.open_clbk_err = error;

   if(error == SA_AIS_OK)
   {
      gl_mqa_env.open_clbk_qhdl = queueHandle;
      m_TET_MQSV_PRINTF("\n -------- Queue Open Callback Success ---------\n");
      m_TET_MQSV_PRINTF(" Queue Handle  : %llu  \n Invocation  : %llu\n",queueHandle,invocation);
   }
   else
   {
      m_TET_MQSV_PRINTF("\n ----------- Queue Open Callback -------------\n");
      m_TET_MQSV_PRINTF(" Error String  : %s  \n Invocation  : %llu\n",
                        mqsv_saf_error_string[error],invocation);
   }

   m_TET_MQSV_PRINTF(" ------------------------------------------------\n\n");
}

void App_saMsgQueueGroupTrackCallback(const SaNameT *queueGroupName,
                                      const SaMsgQueueGroupNotificationBufferT *buffer,
                                      SaUint32T   num_mem,
                                      SaAisErrorT rc)
{
   gl_mqa_env.track_clbk_err = rc;
   gl_track_clbk_iter++;

   if(rc == SA_AIS_OK )
   {
      m_TET_MQSV_PRINTF("\n -------- Queue Group Track Callback Success --------\n");
      m_TET_MQSV_PRINTF("\n Group name : %s\n",queueGroupName->value);
      m_TET_MQSV_PRINTF(" No of Members : %u\n",num_mem);
      groupTrackInfo((SaMsgQueueGroupNotificationBufferT *)buffer); 
      strcpy(gl_mqa_env.track_clbk_grp_name.value,queueGroupName->value);
      gl_mqa_env.track_clbk_grp_name.length = queueGroupName->length;
      gl_mqa_env.track_clbk_num_mem = num_mem;
      copy_notif_buffer((SaMsgQueueGroupNotificationBufferT *)buffer);
   }
   else
   {
      m_TET_MQSV_PRINTF("\n -------- Queue Group Track Callback Failed ---------\n");
      m_TET_MQSV_PRINTF(" Error String  : %s\n",mqsv_saf_error_string[rc]);
   }

   m_TET_MQSV_PRINTF(" -----------------------------------------------------\n\n");
}

void App_saMsgMessageDeliveredCallback(SaInvocationT invocation, SaAisErrorT error)
{
   gl_mqa_env.del_clbk_err = error;
   gl_mqa_env.del_clbk_invo = invocation;
   gl_del_clbk_iter++;

   if(error == SA_AIS_OK)
   {
      m_TET_MQSV_PRINTF("\n ------ Message Delivered Callback Success -----\n");
      m_TET_MQSV_PRINTF(" Invocation  : %llu\n",invocation);
   }
   else
   {
      m_TET_MQSV_PRINTF("\n ------ Message Delivered Callback Failed ------\n");
      m_TET_MQSV_PRINTF(" Error String  : %s  \n Invocation  : %llu\n",
                        mqsv_saf_error_string[error],invocation);
   }

   m_TET_MQSV_PRINTF(" -------------------------------------------------\n\n");
}

void App_saMsgMessageReceivedCallback(SaMsgQueueHandleT queueHandle)
{
   gl_mqa_env.rcv_clbk_qhdl = queueHandle;
   m_TET_MQSV_PRINTF("\n --------- Message Received Callback ------------\n");
   m_TET_MQSV_PRINTF(" Queue Handle  : %llu \n",queueHandle);
   m_TET_MQSV_PRINTF(" --------------------------------------------------\n\n");
}

void App_saMsgMessageReceivedCallback_withMsgGet(SaMsgQueueHandleT queueHandle)
{
   gl_mqa_env.rcv_clbk_qhdl = queueHandle;
   m_TET_MQSV_PRINTF("\n --------- Message Received Callback ------------\n");
   m_TET_MQSV_PRINTF(" Queue Handle  : %llu \n",queueHandle);
   m_TET_MQSV_PRINTF(" --------------------------------------------------\n\n");

   gl_get_result = tet_test_msgMessageGet(MSG_MESSAGE_GET_RECV_SUCCESS_T,TEST_CONFIG_MODE);
}

void App_saMsgMessageReceivedCallback_withMsgGet_cleanup(SaMsgQueueHandleT queueHandle)
{
   gl_mqa_env.rcv_clbk_qhdl = queueHandle;
   gl_rcv_clbk_iter++;
   m_TET_MQSV_PRINTF("\n --------- Message Received Callback ------------\n");
   m_TET_MQSV_PRINTF(" Queue Handle  : %llu \n",queueHandle);
   m_TET_MQSV_PRINTF(" --------------------------------------------------\n\n");

   tet_test_msgMessageGet(MSG_MESSAGE_GET_RECV_SUCCESS_T,TEST_CONFIG_MODE);
   gl_sender_id = gl_mqa_env.sender_id;
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);
}

void App_saMsgMessageReceivedCallback_withReply(SaMsgQueueHandleT queueHandle)
{
   gl_mqa_env.rcv_clbk_qhdl = queueHandle;
   m_TET_MQSV_PRINTF("\n --------- Message Received Callback ------------\n");
   m_TET_MQSV_PRINTF(" Queue Handle  : %llu \n",queueHandle);
   m_TET_MQSV_PRINTF(" --------------------------------------------------\n\n");

   tet_test_msgMessageGet(MSG_MESSAGE_GET_RECV_SUCCESS_T,TEST_CONFIG_MODE);
   if(gl_mqa_env.sender_id != 0)
      gl_reply_result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_SUCCESS_T,TEST_CONFIG_MODE);
}

void App_saMsgMessageReceivedCallback_withReply_nospace(SaMsgQueueHandleT queueHandle)
{
   gl_mqa_env.rcv_clbk_qhdl = queueHandle;
   m_TET_MQSV_PRINTF("\n --------- Message Received Callback ------------\n");
   m_TET_MQSV_PRINTF(" Queue Handle  : %llu \n",queueHandle);
   m_TET_MQSV_PRINTF(" --------------------------------------------------\n\n");

   tet_test_msgMessageGet(MSG_MESSAGE_GET_RECV_SUCCESS_T,TEST_CONFIG_MODE);
   if(gl_mqa_env.sender_id != 0)
      gl_reply_result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_ERR_NO_SPACE_T,TEST_CONFIG_MODE);
}

void App_saMsgMessageReceivedCallback_withReply_nullSname(SaMsgQueueHandleT queueHandle)
{
   gl_mqa_env.rcv_clbk_qhdl = queueHandle;
   m_TET_MQSV_PRINTF("\n --------- Message Received Callback ------------\n");
   m_TET_MQSV_PRINTF(" Queue Handle  : %llu \n",queueHandle);
   m_TET_MQSV_PRINTF(" --------------------------------------------------\n\n");

   tet_test_msgMessageGet(MSG_MESSAGE_GET_RECV_SUCCESS_T,TEST_CONFIG_MODE);
   if(gl_mqa_env.sender_id != 0)
      gl_reply_result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_NULL_SNDR_NAME_T,TEST_CONFIG_MODE);
}

void App_saMsgMessageReceivedCallback_withReplyAsync(SaMsgQueueHandleT queueHandle)
{
   gl_mqa_env.rcv_clbk_qhdl = queueHandle;
   m_TET_MQSV_PRINTF("\n --------- Message Received Callback ------------\n");
   m_TET_MQSV_PRINTF(" Queue Handle  : %llu \n",queueHandle);
   m_TET_MQSV_PRINTF(" --------------------------------------------------\n\n");

   tet_test_msgMessageGet(MSG_MESSAGE_GET_RECV_SUCCESS_T,TEST_CONFIG_MODE);
   if(gl_mqa_env.sender_id != 0)
      gl_reply_result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_SUCCESS_T,
                                                      TEST_CONFIG_MODE);
}

/* *********** Environment Initialization ********** */

void mqsv_fill_msg_version(SaVersionT *version,SaUint8T rel_code,SaUint8T mjr_ver,SaUint8T mnr_ver)
{
   version->releaseCode = rel_code;
   version->majorVersion = mjr_ver;
   version->minorVersion = mnr_ver;
}

void mqsv_fill_msg_clbks(SaMsgCallbacksT *clbk,SaMsgQueueOpenCallbackT opn_clbk,
                         SaMsgQueueGroupTrackCallbackT trk_clbk,
                         SaMsgMessageDeliveredCallbackT del_clbk,
                         SaMsgMessageReceivedCallbackT rcv_clbk)
{
   clbk->saMsgQueueOpenCallback = opn_clbk;
   clbk->saMsgQueueGroupTrackCallback = trk_clbk;
   clbk->saMsgMessageDeliveredCallback = del_clbk;
   clbk->saMsgMessageReceivedCallback = rcv_clbk;
}

void mqsv_fill_q_cr_attribs(SaMsgQueueCreationAttributesT *attr,SaMsgQueueCreationFlagsT cr_flgs,
                     SaSizeT size0,SaSizeT size1,SaSizeT size2,SaSizeT size3,SaTimeT ret_time)
{
   attr->creationFlags = cr_flgs;
   attr->size[0] = size0;
   attr->size[1] = size1;
   attr->size[2] = size2;
   attr->size[3] = size3;
   attr->retentionTime = ret_time;
}

void mqsv_fill_q_grp_names(SaNameT *name,char *string,char *inst_num_char)
{
   strcpy(name->value,string);
   if(inst_num_char)
      strcat(name->value,inst_num_char);
   name->length = strlen(name->value);
}

void mqsv_fill_grp_notif_buffer(SaMsgQueueGroupNotificationBufferT *buffer,SaUint32T num_of_items,
                           SaMsgQueueGroupNotificationT *notif)
{
   buffer->numberOfItems = num_of_items;
   buffer->notification = notif;
}

void mqsv_fill_send_message(SaMsgMessageT *msg,SaUint32T type,SaUint32T version,SaSizeT size,
                       SaNameT *sndr_name,void *data,SaUint8T pri)
{
   msg->type = type;
   msg->version = version;
   msg->size = size;
   msg->senderName = sndr_name;
   msg->data = data;
   msg->priority = pri;
}

void mqsv_fill_rcv_message(SaMsgMessageT *msg,void *data,SaSizeT size,SaNameT *sndr_name)
{
   msg->data = data;
   msg->size = size;
   msg->senderName = sndr_name;
}

void init_mqsv_test_env()
{
   char *data = "Message Queue Service Send Message";
   char *rcv_msg_data = NULL;
   SaMsgQueueGroupNotificationT *inv_notif=0x06; /* some  value */
   SaMsgQueueGroupNotificationT *notification;
   char inst_num_char[10] = {0};
   char *inst_char = NULL;

   memset(&gl_mqa_env,'\0',sizeof(MQA_TEST_ENV));

   if(gl_tetlist_index == MQSV_ONE_NODE_LIST)
   {
      sprintf(inst_num_char,"%d%d",gl_mqsv_inst_num,gl_nodeId);
      inst_char = inst_num_char;
   }

   /* Invalid Parameters */

   gl_mqa_env.inv_params.inv_msg_hdl = 12345;
   mqsv_fill_msg_version(&gl_mqa_env.inv_params.inv_version,'C',0,1);
   mqsv_fill_msg_version(&gl_mqa_env.inv_params.inv_ver_bad_rel_code,'\0',1,0);
   mqsv_fill_msg_version(&gl_mqa_env.inv_params.inv_ver_not_supp,'B',3,0);
   mqsv_fill_q_cr_attribs(&gl_mqa_env.inv_params.inv_cr_attribs,20,1024,1024,1024,1024,100);
   gl_mqa_env.inv_params.inv_q_hdl = 54321;
   mqsv_fill_grp_notif_buffer(&gl_mqa_env.inv_params.inv_notif_buf,0,inv_notif);
   gl_mqa_env.inv_params.inv_sender_id = 32123; 


   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,
                       App_saMsgQueueGroupTrackCallback,App_saMsgMessageDeliveredCallback,
                       App_saMsgMessageReceivedCallback);
   mqsv_fill_msg_clbks(&gl_mqa_env.null_clbks,NULL,NULL,NULL,NULL);
   mqsv_fill_msg_clbks(&gl_mqa_env.null_del_clbks,App_saMsgQueueOpenCallback,
                       App_saMsgQueueGroupTrackCallback,NULL,App_saMsgMessageReceivedCallback);
   mqsv_fill_msg_clbks(&gl_mqa_env.null_rcv_clbks,App_saMsgQueueOpenCallback,
                       App_saMsgQueueGroupTrackCallback,App_saMsgMessageDeliveredCallback,NULL);

   mqsv_fill_msg_version(&gl_mqa_env.version,'B',3,1);

   mqsv_fill_q_grp_names(&gl_mqa_env.pers_q,"safMq=pers_queue",",safApp=safMsgService");
   mqsv_fill_q_grp_names(&gl_mqa_env.non_pers_q,"safMq=non_pers_queue",",safApp=safMsgService");
   mqsv_fill_q_grp_names(&gl_mqa_env.zero_q,"safMq=zero_queue",",safApp=safMsgService");
   mqsv_fill_q_grp_names(&gl_mqa_env.pers_q2,"safMq=pers_queue2",",safApp=safMsgService");
   mqsv_fill_q_grp_names(&gl_mqa_env.non_pers_q2,"safMq=non_pers_queue2",",safApp=safMsgService");

   mqsv_fill_q_cr_attribs(&gl_mqa_env.pers_cr_attribs,SA_MSG_QUEUE_PERSISTENT,1024,1024,
                          1024,1024,gl_queue_ret_time);
   mqsv_fill_q_cr_attribs(&gl_mqa_env.npers_cr_attribs,SA_MSG_QUEUE_NON_PERSISTENT,1000,1000,
                          1000,1000,gl_queue_ret_time);
   mqsv_fill_q_cr_attribs(&gl_mqa_env.small_cr_attribs,SA_MSG_QUEUE_PERSISTENT,100,100,100,
                          100,gl_queue_ret_time);
   mqsv_fill_q_cr_attribs(&gl_mqa_env.big_cr_attribs,SA_MSG_QUEUE_PERSISTENT,10000,10000,
                          10000,10000,gl_queue_ret_time);
   mqsv_fill_q_cr_attribs(&gl_mqa_env.zero_size_cr_attribs,SA_MSG_QUEUE_PERSISTENT,0,0,0,0,
                          gl_queue_ret_time);
   mqsv_fill_q_cr_attribs(&gl_mqa_env.zero_ret_time_cr_attribs,SA_MSG_QUEUE_NON_PERSISTENT,100,
                          100,100,100,0);

   mqsv_fill_q_grp_names(&gl_mqa_env.qgroup1,"safMqg=group1",",safApp=safMsgService");
   mqsv_fill_q_grp_names(&gl_mqa_env.qgroup2,"safMqg=group2",",safApp=safMsgService");
   mqsv_fill_q_grp_names(&gl_mqa_env.qgroup3,"safMqg=group3",",safApp=safMsgService");	

   mqsv_fill_grp_notif_buffer(&gl_mqa_env.buffer_null_notif,3,NULL);
   notification = (SaMsgQueueGroupNotificationT *)calloc(1,sizeof(SaMsgQueueGroupNotificationT));
   mqsv_fill_grp_notif_buffer(&gl_mqa_env.no_space_notif_buf,1,notification);

   mqsv_fill_q_grp_names(&gl_mqa_env.sender_name,"queue1",NULL);

   gl_mqa_env.send_data = (char *)calloc(strlen(data)+1,sizeof(char));
   strcpy(gl_mqa_env.send_data,data);

   rcv_msg_data = (char *)calloc(10,sizeof(char));

   mqsv_fill_send_message(&gl_mqa_env.send_msg,0,1,30,&gl_mqa_env.sender_name,
                          (void *)gl_mqa_env.send_data,0);
   mqsv_fill_send_message(&gl_mqa_env.send_msg_null_sndr_name,1,1,12,NULL,
                          (void *)gl_mqa_env.send_data,1);

   mqsv_fill_send_message(&gl_mqa_env.send_big_msg,5,1,4096,NULL,(void *)gl_mqa_env.send_data,2);
   mqsv_fill_send_message(&gl_mqa_env.send_msg_bad_pr,2,1,12,NULL,(void *)gl_mqa_env.send_data,4);
   mqsv_fill_send_message(&gl_mqa_env.send_msg_zero_size,0,1,0,&gl_mqa_env.sender_name,NULL,0);

   mqsv_fill_rcv_message(&gl_mqa_env.rcv_msg,NULL,2,&gl_mqa_env.rcv_sender_name);
   mqsv_fill_rcv_message(&gl_mqa_env.rcv_msg_null_sndr_name,NULL,0,NULL);
   mqsv_fill_rcv_message(&gl_mqa_env.no_space_rcv_msg,rcv_msg_data,10,&gl_mqa_env.rcv_sender_name);

   mqsv_fill_rcv_message(&gl_mqa_env.reply_msg,NULL,2,&gl_mqa_env.rcv_sender_name);
   mqsv_fill_rcv_message(&gl_mqa_env.reply_msg_null_sndr_name,NULL,0,NULL);
   mqsv_fill_rcv_message(&gl_mqa_env.no_space_reply_msg,rcv_msg_data,10,&gl_mqa_env.rcv_sender_name);
}

void mqsv_print_testcase(char *string)
{
   m_TET_MQSV_PRINTF(string);
   tet_printf(string);
}

void mqsv_result(int result)
{
   mqsv_clean_q_status();
   mqsv_clean_clbk_params();
   mqsv_clean_output_params();

#if 0
   gl_sync_pointnum = 1;
#endif

   tet_result(result);

   if(result == TET_PASS)
      mqsv_print_testcase("************* TEST CASE SUCCEEDED ************\n\n");
   else
      mqsv_print_testcase("************* TEST CASE FAILED ************\n\n");
}

/*********** saMsgInitialize Api Tests ************/

void mqsv_it_init_01()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgInitialize with valid parameters *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_NONCONFIG_MODE);
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

   mqsv_result(result);
}

void mqsv_it_init_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgInitialize with NULL callback structure *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CLBK_PARAM_T,TEST_NONCONFIG_MODE);
   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CLBK_PARAM_T);

   mqsv_result(result);
}

void mqsv_it_init_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgInitialize with NULL version parameter *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_VERSION_T,TEST_NONCONFIG_MODE);
   mqsv_result(result);
}

void mqsv_it_init_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgInitialize with NULL message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_HANDLE_T,TEST_NONCONFIG_MODE);
   mqsv_result(result);
}

void mqsv_it_init_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgInitialize with NULL callback and version paramters *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_VERSION_CBKS_T,TEST_NONCONFIG_MODE);
   mqsv_result(result);
}

void mqsv_it_init_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgInitialize with release code > supported release code *****\n");

   result = tet_test_msgInitialize(MSG_INIT_BAD_VERSION_T,TEST_NONCONFIG_MODE);
   mqsv_restore_params(MSG_RESTORE_INIT_BAD_VERSION_T);
   mqsv_result(result);
}

void mqsv_it_init_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgInitialize with invalid release code in version *****\n");

   result = tet_test_msgInitialize(MSG_INIT_BAD_REL_CODE_T,TEST_NONCONFIG_MODE);
   mqsv_restore_params(MSG_RESTORE_INIT_BAD_REL_CODE_T);
   mqsv_result(result);
}

void mqsv_it_init_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgInitialize with major version > supported major version *****\n");

   result = tet_test_msgInitialize(MSG_INIT_BAD_MAJOR_VER_T,TEST_NONCONFIG_MODE);
   mqsv_restore_params(MSG_RESTORE_INIT_BAD_MAJOR_VER_T);
   mqsv_result(result);
}

void mqsv_it_init_09()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgInitialize returns supported version when called with invalid version *****\n");

   result = tet_test_msgInitialize(MSG_INIT_BAD_VERSION_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS)
   {
      if(gl_mqa_env.inv_params.inv_version.releaseCode == 'B' &&
         gl_mqa_env.inv_params.inv_version.majorVersion == 1 &&
         gl_mqa_env.inv_params.inv_version.minorVersion == 1)
      {
         mqsv_restore_params(MSG_RESTORE_INIT_BAD_VERSION_T);
         mqsv_result(TET_PASS);
         return;
      }
   }

   mqsv_result(result);
}

void mqsv_it_init_10()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgInitialize without registering any callback *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CBKS_T,TEST_NONCONFIG_MODE);
   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CBKS_T);
   mqsv_result(result);
}

/*********** saMsgSelectionObjectGet Api Tests ************/

void mqsv_it_selobj_01()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgSelectionObjectGet with valid parameters *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgSelectionObject(MSG_SEL_OBJ_SUCCESS_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_selobj_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgSelectionObjectGet with NULL selection object parameter *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgSelectionObject(MSG_SEL_OBJ_NULL_SEL_OBJ_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_selobj_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgSelectionObjectGet with uninitialized message handle  *****\n");

   result = tet_test_msgSelectionObject(MSG_SEL_OBJ_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
   mqsv_result(result);
}

void mqsv_it_selobj_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgSelectionObjectGet with finalized message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgSelectionObject(MSG_SEL_OBJ_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

final:
   mqsv_result(result);
}

/*********** saMsgDispatch Api Tests ************/

void mqsv_it_dispatch_01()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgDispatch invokes pending callbacks - SA_DISPATCH_ONE *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 114 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_dispatch_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgDispatch invokes pending callbacks - SA_DISPATCH_ALL *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   sleep(2);

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ALL_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 115 && gl_mqa_env.del_clbk_invo == 309)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_dispatch_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgDispatch invokes pending callbacks - SA_DISPATCH_BLOCKING *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   sleep(2);

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   if(gl_mqa_env.open_clbk_invo == 115 && gl_mqa_env.del_clbk_invo == 309)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_dispatch_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgDispatch with invalid dispatch flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgDispatch(MSG_DISPATCH_BAD_FLAGS_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_dispatch_05()
{
   int result,result1, result2;

   mqsv_print_testcase(" \n\n ***** saMsgDispatch with invalid message handle - SA_DISPATCH_ONE *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgDispatch(MSG_DISPATCH_ONE_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgDispatch(MSG_DISPATCH_ONE_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_dispatch_06()
{
   int result,result1, result2;

   mqsv_print_testcase(" \n\n ***** saMsgDispatch with invalid message handle - SA_DISPATCH_ALL *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgDispatch(MSG_DISPATCH_ALL_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgDispatch(MSG_DISPATCH_ALL_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_dispatch_07()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgDispatch with invalid message handle - SA_DISPATCH_BLOCKING *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgDispatch(MSG_DISPATCH_BLKING_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgDispatch(MSG_DISPATCH_BLKING_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_dispatch_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgDispatch in case of no pending callbacks - SA_DISPATCH_ONE *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_dispatch_09()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgDispatch in case of no pending callbacks - SA_DISPATCH_ALL *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ALL_SUCCESS_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgFinalize Api Tests ************/

void mqsv_it_finalize_01()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgFinalize closes association between Message Service and app process *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgDispatch(MSG_DISPATCH_ALL_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

final:
   mqsv_result(result);
}

void mqsv_it_finalize_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgFinalize with uninitialized message handle *****\n");

   result = tet_test_msgFinalize(MSG_FINALIZE_BAD_HDL_T,TEST_NONCONFIG_MODE);
   mqsv_result(result);
}

void mqsv_it_finalize_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgFinalize with finalized message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgFinalize(MSG_FINALIZE_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

final:
   mqsv_result(result);
}

void mqsv_it_finalize_04()
{
   int result;
   fd_set read_fd;
   struct timeval tv;

   mqsv_print_testcase(" \n\n ***** Selection object becomes invalid after finalizing the message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgSelectionObject(MSG_SEL_OBJ_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   tv.tv_sec = 5;
   tv.tv_usec = 0;

   FD_ZERO(&read_fd);
   FD_SET(gl_mqa_env.sel_obj, &read_fd);
   result = select(gl_mqa_env.sel_obj + 1, &read_fd, NULL, NULL, &tv);
   if(result == -1)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_finalize_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Message queues that are opened are closed after finalizing the message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_BAD_HDL2_T,TEST_NONCONFIG_MODE);

   tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CLEANUP_MODE);
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_finalize_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Group trackings are stopped when that message handle is finalized *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_UNTRACKED_T,TEST_NONCONFIG_MODE);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgQueueOpen and saMsgQueueOpenAsync Api Tests ************/

void mqsv_it_qopen_01()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpen with NULL queue handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NULL_Q_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpen with NULL queue name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpenAsync with NULL queue name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpen with uninitialized message handle *****\n");

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
   mqsv_result(result);
}

void mqsv_it_qopen_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpen with finalized message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpenAsync with uninitialized message handle *****\n");

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
   mqsv_result(result);
}

void mqsv_it_qopen_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpenAsync with finalized message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_08()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpen with NULL attributes and SA_MSG_QUEUE_CREATE in open flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result1 = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

   result2 = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_INVALID_PARAM_EMPTY_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_09()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpenAsync with NULL attributes and SA_MSG_QUEUE_CREATE in open flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result1 = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

   result2 = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM_EMPTY_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_10()
{
   int result,result1,result2,result3;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpen with non-NULL attributes and non-create open flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result1 = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_INVALID_PARAM2_EMPTY_T,TEST_NONCONFIG_MODE);

   result2 = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_INVALID_PARAM2_ZERO_T,TEST_NONCONFIG_MODE);

   result3 = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_INVALID_PARAM2_RC_CBK_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS && result3 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_11()
{
   int result,result1,result2,result3;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpenAsync with non-NULL attributes and non-create open flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result1 = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_EMPTY_T,TEST_NONCONFIG_MODE);

   result2 = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_ZERO_T,TEST_NONCONFIG_MODE);

   result3 = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_RC_CBK_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS && result3 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_12()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpen with invalid open flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_BAD_FLAGS_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_13()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpenAsync with invalid open flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_BAD_FLGS_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_14()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpen with invalid creation flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_BAD_FLAGS2_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_15()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpenAsync with invalid creation flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_BAD_FLGS2_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_16()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a non-persistent queue using saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_17()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a non-persistent queue using saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 115 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

   m_MQSV_WAIT;
   m_MQSV_WAIT;

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_18()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Invocation in open callback is same as that given in saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 115 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_19()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a persistent queue using saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_20()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a persistent queue using saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 114 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_21()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a queue with SA_MSG_QUEUE_EMPTY using saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_EMPTY_CREATE_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_EMPTY_CREATE_T);
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_22()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a queue with SA_MSG_QUEUE_EMPTY using saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_EMPTY_CREATE_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 116 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_EMPTY_CREATE_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_23()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a queue with SA_MSG_QUEUE_RECEIVE_CALLBACK using saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_24()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a queue with SA_MSG_QUEUE_RECEIVE_CALLBACK using saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_NPERS_RECV_CLBK_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 124 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_25()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a queue with zero retention time using saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_RET_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS)
   {
      result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q4_T,TEST_NONCONFIG_MODE);
      if(result == TET_PASS && gl_mqa_env.q_status.retentionTime == 0)
         result = TET_PASS;
      else
         result = TET_FAIL;
   }

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ZERO_RET_T);
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_26()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a queue with zero retention time using saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ZERO_RET_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 126 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
   {
      result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q4_T,TEST_NONCONFIG_MODE);
      if(result == TET_PASS && gl_mqa_env.q_status.retentionTime == 0)
         result = TET_PASS;
      else
         result = TET_FAIL;
   }
   else
      result = TET_FAIL;

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_ZERO_RET_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_27()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a queue with zero size using saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_SIZE_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS)
   {
      result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q5_T,TEST_NONCONFIG_MODE);
      if(result == TET_PASS && gl_mqa_env.q_status.saMsgQueueUsage[0].queueSize == 0
         && gl_mqa_env.q_status.saMsgQueueUsage[1].queueSize == 0
         && gl_mqa_env.q_status.saMsgQueueUsage[2].queueSize == 0
         && gl_mqa_env.q_status.saMsgQueueUsage[3].queueSize == 0)
         result = TET_PASS;
      else
         result = TET_FAIL;
   }

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T);
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_28()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a queue with zero size using saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ZERO_SIZE_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 127 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
   {
      result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q5_T,TEST_NONCONFIG_MODE);
      if(result == TET_PASS && gl_mqa_env.q_status.saMsgQueueUsage[0].queueSize == 0
         && gl_mqa_env.q_status.saMsgQueueUsage[1].queueSize == 0
         && gl_mqa_env.q_status.saMsgQueueUsage[2].queueSize == 0
         && gl_mqa_env.q_status.saMsgQueueUsage[3].queueSize == 0)
         result = TET_PASS;
      else
         result = TET_FAIL;
   }
   else
      result = TET_FAIL;

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_ZERO_SIZE_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_29()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpen with zero timeout value *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_TIMEOUT_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_30()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open an existing queue with different attributes - saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_ERR_EXIST_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_31()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a closed queue with different attributes - saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NPERS_ERR_EXIST_T,TEST_NONCONFIG_MODE);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_32()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open an existing queue with different attributes - saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ERR_EXIST_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 117 && gl_mqa_env.open_clbk_err == SA_AIS_ERR_EXIST)
     result = TET_PASS;
   else
     result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_33()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a closed queue with different attributes - saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ERR_EXIST2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 118 && gl_mqa_env.open_clbk_err == SA_AIS_ERR_EXIST)
     result = TET_PASS;
   else
     result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_34()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a queue that does not exist - NULL attr and zero open flag *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_ER_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_35()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open an queue that does not exist - NULL attr and non-create open flag *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_ER_NOT_EXIST2_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_36()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a queue that does not exist - NULL attr and zero open flag *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 111 && gl_mqa_env.open_clbk_err == SA_AIS_ERR_NOT_EXIST)
      result = TET_PASS;
   else
      result = TET_FAIL;

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_37()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open an queue that does not exist - NULL attr and non-create open flag *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ERR_NOT_EXIST2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 112 && gl_mqa_env.open_clbk_err == SA_AIS_ERR_NOT_EXIST)
      result = TET_PASS;
   else
      result = TET_FAIL;

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_38()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open an existing open queue with same attributes - saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ERR_BUSY_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_39()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open an existing open queue with same attributes - saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ERR_BUSY_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 113 && gl_mqa_env.open_clbk_err == SA_AIS_ERR_BUSY)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_40()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open an open queue with NULL attributes and non-create open flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_EXIST_ERR_BUSY_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_41()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open an open queue with NULL attributes and non-create open flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_EXIST_ERR_BUSY_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 128 && gl_mqa_env.open_clbk_err == SA_AIS_ERR_BUSY)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_42()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpenAsync without registering the open callback *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CBKS2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ERR_INIT_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CBKS2_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_43()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpen with recv clbk flag without registering received callback *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_RCV_CBK_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ERR_INIT_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_RCV_CBK_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_44()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueOpenAsync with recv clbk flag without registering received clbk *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_RCV_CBK_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ERR_INIT2_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_RCV_CBK_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_45()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Queue handle obtained in open clbk is valid when error is SA_AIS_OK *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ERR_EXIST_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.open_clbk_invo != 117 || gl_mqa_env.open_clbk_err != SA_AIS_ERR_EXIST)
   {
      result = TET_UNRESOLVED;
      goto final2;
   }

   gl_mqa_env.pers_q_hdl = gl_mqa_env.open_clbk_qhdl;
   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_BAD_HANDLE1_T,TEST_NONCONFIG_MODE);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_46()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a closed queue using saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_EXIST_SUCCESS_T,TEST_NONCONFIG_MODE);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_47()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a closed queue with empty flag using saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NPERS_EMPTY_SUCCESS_T,TEST_NONCONFIG_MODE);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_48()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a closed queue with recv clbk flag using saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_EXIST_RECV_CLBK_SUCCESS_T,TEST_NONCONFIG_MODE);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_49()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a closed queue using saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_EXIST_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 119 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_50()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a closed queue with empty flag using saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_NPERS_EMPTY_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 120 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qopen_51()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a closed queue with recv clbk flag using saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_EXIST_RECV_CLBK_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.open_clbk_invo == 125 && gl_mqa_env.open_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgQueueClose Api Tests ************/

void mqsv_it_close_01()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueClose with invalid queue handle *****\n");

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_INV_HANDLE_T,TEST_NONCONFIG_MODE);
   mqsv_result(result);
}

void mqsv_it_close_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueClose with a queue handle associated with finalized msg handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_BAD_HANDLE1_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_AFTER_FINALIZE);

final:
   mqsv_result(result);
}

void mqsv_it_close_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Close a message queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.q_status.closeTime != 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_close_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueClose with a closed queue handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_BAD_HANDLE1_T,TEST_NONCONFIG_MODE);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_close_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Closing a queue that is already unlinked will delete the queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_NOT_EXIST_Q1_T,TEST_NONCONFIG_MODE);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_close_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Closing a non-persistent queue with zero retention time will delete the queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ZERO_RET_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.open_clbk_invo != 126 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
   {
      result = TET_UNRESOLVED;
      goto final1;
   }

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL5_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_NOT_EXIST_Q4_T,TEST_NONCONFIG_MODE);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_ZERO_RET_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_close_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueClose cancels all pending callbacks on that queue handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.rcv_clbk_qhdl == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgQueueStatusGet Api Tests ************/

void mqsv_it_qstatus_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgQueueStatusGet with invalid message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_BAD_HANDLE_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_qstatus_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueStatusGet with NULL queue name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qstatus_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueStatusGet with NULL status parameter *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_NULL_STATUS_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qstatus_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Get the status of a message queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS)
   {
      if(gl_mqa_env.q_status.creationFlags == SA_MSG_QUEUE_NON_PERSISTENT &&
         gl_mqa_env.q_status.retentionTime == 10000000000ULL &&
         gl_mqa_env.q_status.closeTime == 0 &&
         gl_mqa_env.q_status.saMsgQueueUsage[0].queueSize == 1000 &&
         gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == 0 &&
         gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 0 &&
         gl_mqa_env.q_status.saMsgQueueUsage[1].queueSize == 1000 &&
         gl_mqa_env.q_status.saMsgQueueUsage[1].queueUsed == 0 &&
         gl_mqa_env.q_status.saMsgQueueUsage[1].numberOfMessages == 0 &&
         gl_mqa_env.q_status.saMsgQueueUsage[2].queueSize == 1000 &&
         gl_mqa_env.q_status.saMsgQueueUsage[2].queueUsed == 0 &&
         gl_mqa_env.q_status.saMsgQueueUsage[2].numberOfMessages == 0 &&
         gl_mqa_env.q_status.saMsgQueueUsage[3].queueSize == 1000 &&
         gl_mqa_env.q_status.saMsgQueueUsage[3].queueUsed == 0 &&
         gl_mqa_env.q_status.saMsgQueueUsage[3].numberOfMessages == 0)
         result = TET_PASS;
      else
         result = TET_FAIL;
   }

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qstatus_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Get the status of a message queue that does not exist *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_NOT_EXIST_Q1_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qstatus_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Get the status of a message queue that closed *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.open_clbk_invo != 114 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
   {
      result = TET_UNRESOLVED;
      goto final1;
   }

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL5_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS)
   {
      if(gl_mqa_env.q_status.closeTime != 0)
         result = TET_PASS;
      else
         result = TET_FAIL;
   }

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qstatus_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Get the status of a message queue when a message is in the queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS)
   {
      if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueSize == 1000 &&
         gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == 30 &&
         gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 1)
         result = TET_PASS;
      else
         result = TET_FAIL;
   }

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgQueueUnlink Api Tests ************/

void mqsv_it_qunlink_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgQueueUnlink with invalid message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_BAD_HANDLE_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_qunlink_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueUnlink with NULL queue name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qunlink_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Unlink a message queue that does not exist *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qunlink_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Unlink a message queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);

   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qunlink_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Unlink a message queue that is not open by any process *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_NONCONFIG_MODE);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qunlink_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Unlinking a queue that is open by any process will not delete the queue  *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);
      goto final1;
   }

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_NONCONFIG_MODE);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qunlink_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** After saMsgQueueUnlink, all apis that use this queue name return errors *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);
      goto final1;
   }

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_NOT_EXIST_Q2_T,TEST_NONCONFIG_MODE);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgQueueGroupCreate Api Tests ************/

void mqsv_it_qgrp_create_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupCreate with invalid message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgGroupCreate(MSG_GROUP_CREATE_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgGroupCreate(MSG_GROUP_CREATE_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_create_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupCreate with null group name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_create_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupCreate with bad queue group policy *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_BAD_POL_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_create_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Create a message queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_create_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupCreate with queue group that already exists *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_ERR_EXIST_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_create_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupCreate with existing group name with different group policy *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_ERR_EXIST2_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_create_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupCreate with existing group name with different group policy *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_ERR_EXIST3_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_create_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupCreate with existing group name with different group policy *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_ERR_EXIST4_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_create_09()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupCreate with a group policy not supported *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_LOCAL_RR_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_create_10()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupCreate with a group policy not supported *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_LCL_BSTQ_NOT_SUPP_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_LCL_BSTQ_NOT_SUPP_T);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_create_11()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupCreate with a group policy not supported *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_BROADCAST_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_BROADCAST_T);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgQueueGroupInsert Api Tests ************/

void mqsv_it_qgrp_insert_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupInsert with uninitilaized message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgGroupInsert(MSG_GROUP_INSERT_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgGroupInsert(MSG_GROUP_INSERT_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_insert_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupInsert with NULL queue group name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_NULL_GR_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_insert_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupInsert with NULL queue name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_NULL_Q_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_insert_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Insert a queue into a group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_insert_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Insert a non-existing queue into a group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_BAD_QUEUE_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_insert_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Insert a queue into a non-existing group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_BAD_GR_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_insert_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Insert a queue into a group more than once *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_ERR_EXIST_T,TEST_NONCONFIG_MODE);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_insert_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Insert a queue into a non-empty group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.open_clbk_invo != 114 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
   {
      result = TET_UNRESOLVED;
      goto final1;
   }

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);

final4:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_insert_09()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Insert the same queue queue into a two different queue groups *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_GR2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_Q1_GROUP2_T,TEST_NONCONFIG_MODE);

final4:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_GR2_T);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgQueueGroupRemove Api Tests ************/

void mqsv_it_qgrp_remove_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupRemove with invalid message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_remove_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupRemove with NULL queue name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_NULL_Q_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_remove_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupRemove with NULL group name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_NULL_GR_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_remove_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Remove a queue from a queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_SUCCESS_T,TEST_NONCONFIG_MODE);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_remove_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Remove a queue from a non-existing queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_BAD_GROUP_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_remove_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Remove a non-existing queue from a queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_BAD_QUEUE_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_remove_07()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** Remove a queue that is not a member of that queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result1 = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_GR2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_Q1_GROUP2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result2 = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final4:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_GR2_T);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgQueueGroupDelete Api Tests ************/

void mqsv_it_qgrp_delete_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupDelete with invalid message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgGroupDelete(MSG_GROUP_DELETE_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgGroupDelete(MSG_GROUP_DELETE_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_delete_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupDelete with NULL queue group name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_delete_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Delete a message queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_T,TEST_NONCONFIG_MODE);

   if(result != TET_PASS)
      mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_delete_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Delete a message queue group that is not existing *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_BAD_GROUP_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_delete_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Delete a queue group with member queues *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_WITH_MEM_T,TEST_NONCONFIG_MODE);

final3:
   if(result != TET_PASS)
      mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_delete_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Delete a empty queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_GR2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_WO_MEM_T,TEST_NONCONFIG_MODE);

   if(result != TET_PASS)
      mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_GR2_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_delete_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a group that is already deleted *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_GR2_T);
      goto final1;
   }

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_BAD_GROUP_T,TEST_NONCONFIG_MODE);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgQueueGroupTrack Api Tests ************/

void mqsv_it_qgrp_track_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack with invalid message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgGroupTrack(MSG_GROUP_TRACK_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgGroupTrack(MSG_GROUP_TRACK_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack with NULL queue group name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack with a group name that does not exist *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_BAD_GROUP_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack with a invalid track flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_BAD_FLAGS_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack with a wrong track flags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_WRONG_FLGS_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack with invalid notification buffer - SA_TRACK_CURRENT *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_INV_PARAM_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack without registering track callback - SA_TRACK_CURRENT *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CBKS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURR_NULBUF_ERINIT_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CBKS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack without track callback - Track Current (non-NULL buffer) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CBKS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURR_ERR_INIT_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CBKS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_09()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack without registering track callback - SA_TRACK_CHANGES *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CBKS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHGS_ERR_INIT_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CBKS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_10()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack without registering track callback - SA_TRACK_CHANGES_ONLY *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CBKS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHONLY_ERR_INIT_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CBKS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_11()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CURRENT - non-Null notif-buffer and NULL notification *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURRENT_T,TEST_NONCONFIG_MODE);

   if(result == TET_PASS &&
      gl_mqa_env.buffer_null_notif.numberOfItems == 1 &&
      gl_mqa_env.buffer_null_notif.queueGroupPolicy == SA_MSG_QUEUE_GROUP_ROUND_ROBIN &&
      !strcmp(gl_mqa_env.buffer_null_notif.notification->member.queueName.value,gl_mqa_env.non_pers_q.value) &&
      gl_mqa_env.buffer_null_notif.notification->change == SA_MSG_QUEUE_GROUP_NO_CHANGE)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_12()
{
   int result;
   SaMsgQueueGroupNotificationT *notification = NULL;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CURRENT - non-Null noif-buffer and non-NULL notification *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.open_clbk_invo != 114 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
   {
      result = TET_UNRESOLVED;
      goto final3;
   }

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   notification = (SaMsgQueueGroupNotificationT *)calloc(2,sizeof(SaMsgQueueGroupNotificationT));
   mqsv_fill_grp_notif_buffer(&gl_mqa_env.buffer_non_null_notif,2,notification);

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T,TEST_NONCONFIG_MODE);

   if(result == TET_PASS && gl_mqa_env.buffer_non_null_notif.numberOfItems == 2 &&
      gl_mqa_env.buffer_non_null_notif.queueGroupPolicy == SA_MSG_QUEUE_GROUP_ROUND_ROBIN &&
      gl_mqa_env.buffer_non_null_notif.notification &&
      !strcmp(gl_mqa_env.buffer_non_null_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) &&
      gl_mqa_env.buffer_non_null_notif.notification[0].change == SA_MSG_QUEUE_GROUP_NO_CHANGE &&
      !strcmp(gl_mqa_env.buffer_non_null_notif.notification[1].member.queueName.value,gl_mqa_env.pers_q.value) &&
      gl_mqa_env.buffer_non_null_notif.notification[1].change == SA_MSG_QUEUE_GROUP_NO_CHANGE)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_13()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack with insufficient notification buffer *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CUR_NO_SPACE_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CUR_NO_SPACE_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_14()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack updates numberOfItems when return SA_AIS_ERR_NO_SPACE *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CUR_NO_SPACE_T,TEST_NONCONFIG_MODE);

   if(result == TET_PASS && gl_mqa_env.no_space_notif_buf.numberOfItems == 2)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CUR_NO_SPACE_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_15()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack with NULL notification buffer - SA_TRACK_CURRENT *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_NULL_BUF_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.track_clbk_err != SA_AIS_OK)
   {
      result = TET_FAIL;
      goto final3;
   }

   if(gl_mqa_env.track_clbk_num_mem == 1 &&
      !strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) &&
      gl_mqa_env.track_clbk_notif.numberOfItems == 1 &&
      gl_mqa_env.track_clbk_notif.queueGroupPolicy == SA_MSG_QUEUE_GROUP_ROUND_ROBIN &&
      !strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) &&
      gl_mqa_env.track_clbk_notif.notification[0].change == SA_MSG_QUEUE_GROUP_NO_CHANGE)
      result = TET_PASS;
   else
      result = TET_FAIL;

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_16()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack with SA_TRACK_CHANGES and invalid buffer *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CH_BAD_BUF_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_T);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_17()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrack with SA_TRACK_CHANGES_ONLY and invalid buffer *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHONLY_BAD_BUF_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_T);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_18()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CURRENT | SA_TRACK_CHANGES and invalid buffer *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CUR_CH_BAD_BUF_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_19()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CURRENT | SA_TRACK_CHANGES_ONLY and invalid buffer *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CUR_CHONLY_BAD_BUF_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_20()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CHANGES and insert a queue into the group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != 0)
   {
      result = TET_FAIL;
      goto final5;
   }

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_ADDED)
   {
      result = TET_FAIL;
      goto final5;
   }

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 2 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 2 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_NO_CHANGE ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[1].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[1].change != SA_MSG_QUEUE_GROUP_ADDED)
      result = TET_FAIL;

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_21()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CHANGES_ONLY and insert a queue into the group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHGS_ONLY_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != 0)
   {
      result = TET_FAIL;
      goto final5;
   }

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_ADDED)
   {
      result = TET_FAIL;
      goto final5;
   }

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 2 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_ADDED)
      result = TET_FAIL;

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHGS_ONLY_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_22()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CHANGES and remove a queue from the group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 2 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_NO_CHANGE ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[1].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[1].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_23()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CHANGES_ONLY and remove a queue from the group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHGS_ONLY_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHGS_ONLY_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_24()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CURRENT | SA_TRACK_CHANGES with NULL notif-buffer *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CUR_CH_NUL_BUF_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 0 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 0 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      gl_mqa_env.track_clbk_notif.notification != NULL)
   {
      result = TET_FAIL;
      goto final5;
   }

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 2 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 2 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_NO_CHANGE ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[1].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[1].change != SA_MSG_QUEUE_GROUP_ADDED)
   {
      result = TET_FAIL;
      goto final5;
   }

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 2 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_NO_CHANGE ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[1].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[1].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHGS_ONLY_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_25()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CURRENT | SA_TRACK_CHANGES_ONLY with NULL notif-buffer *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CUR_CHLY_NUL_BUF_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 0 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 0 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      gl_mqa_env.track_clbk_notif.notification != NULL)
   {
      result = TET_FAIL;
      goto final5;
   }

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 2 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_ADDED)
   {
      result = TET_FAIL;
      goto final5;
   }

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHGS_ONLY_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_26()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CURRENT | SA_TRACK_CHANGES with non-NULL notif-buffer *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CUR_CH_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != 0 || gl_mqa_env.buffer_null_notif.numberOfItems != 1 ||
      gl_mqa_env.buffer_null_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.buffer_null_notif.notification->member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.buffer_null_notif.notification->change != SA_MSG_QUEUE_GROUP_NO_CHANGE)
   {
      result = TET_FAIL;
      goto final4;
   }

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 0 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;

final4:
   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T);
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CUR_CH_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_27()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track with SA_TRACK_CURRENT | SA_TRACK_CHANGES_ONLY with non-NULL notif-buffer *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CUR_CHLY_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != 0 || gl_mqa_env.buffer_null_notif.numberOfItems != 1 ||
      gl_mqa_env.buffer_null_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.buffer_null_notif.notification->member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.buffer_null_notif.notification->change != SA_MSG_QUEUE_GROUP_NO_CHANGE)
   {
      result = TET_FAIL;
      goto final4;
   }

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 0 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;

final4:
   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T);
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CUR_CHLY_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_28()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track a group with TRACK_CHANGES_ONLY while being tracked with TRACK_CHANGES *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 2 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 2 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_NO_CHANGE ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[1].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[1].change != SA_MSG_QUEUE_GROUP_ADDED)
   {
      result = TET_FAIL;
      goto final5;
   }

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHGS_ONLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;
   else
      result = TET_PASS;

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHGS_ONLY_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_29()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track a group with TRACK_CHANGES while being tracked with TRACK_CHANGES_ONLY *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHGS_ONLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 2 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_ADDED)
   {
      result = TET_FAIL;
      goto final5;
   }

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 2 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_NO_CHANGE ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[1].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[1].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;
   else
      result = TET_PASS;

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_30()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track info gives the policy of the queue group - SA_TRACK_CURRENT *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.buffer_non_null_notif.numberOfItems != 0 ||
      gl_mqa_env.buffer_non_null_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN)
   {
      result = TET_FAIL;
      goto final2;
   }

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_LOCAL_RR_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.buffer_non_null_notif.numberOfItems != 0 ||
      gl_mqa_env.buffer_non_null_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN)
   {
      result = TET_FAIL;
      goto final2;
   }

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_BROADCAST_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.buffer_non_null_notif.numberOfItems != 0 ||
      gl_mqa_env.buffer_non_null_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_BROADCAST)
      result = TET_FAIL;

final2:
   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T);
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_31()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track info gives the policy of the queue group - SA_TRACK_CHANGES *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   m_MQSV_WAIT;

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN)
   {
      result = TET_FAIL;
      goto final3;
   }

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_LOCAL_RR_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   m_MQSV_WAIT;

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN)
   {
      result = TET_FAIL;
      goto final3;
   }

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   mqsv_clean_clbk_params();

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_BROADCAST_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   m_MQSV_WAIT;

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 1 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_BROADCAST)
      result = TET_FAIL;

final3:
   mqsv_clean_clbk_params();
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_32()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Deleting a queue group while being tracked with SA_TRACK_CHANGES *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.track_clbk_err != SA_AIS_ERR_NOT_EXIST)
      result = TET_FAIL;
   else
      result = TET_PASS;

   mqsv_clean_clbk_params();

final2:
   if(result != TET_PASS)
      mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_33()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Deleting a queue group while being tracked with SA_TRACK_CHANGES_ONLY *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHGS_ONLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.track_clbk_err != SA_AIS_ERR_NOT_EXIST)
      result = TET_FAIL;
   else
      result = TET_PASS;

   mqsv_clean_clbk_params();

final2:
   if(result != TET_PASS)
      mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgQueueGroupTrackStop Api Tests ************/

void mqsv_it_qgrp_track_stop_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrackStop with invalid message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_stop_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrackStop with NULL group name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_stop_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrackStop with group name that does not exist *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_BAD_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_stop_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Stop tracking a group that is tracked with SA_TRACK_CHANGES *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_stop_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Stop tracking a group that is tracked with SA_TRACK_CHANGES_ONLY *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHGS_ONLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHGS_ONLY_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_stop_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Stop tracking a group that is tracked with SA_TRACK_CURRENT *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURRENT_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_UNTRACKED_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_qgrp_track_stop_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgQueueGroupTrackStop with an untracked group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_UNTRACKED_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgMessageSend ans saMsgMessageSendAsync Api Tests ************/

void mqsv_it_msg_send_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSend with invalid message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgMessageSend(MSG_MESSAGE_SEND_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgMessageSend(MSG_MESSAGE_SEND_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_02()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendAsync with invalid message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSend with NULL destination *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendAsync with NULL destination *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSend with NULL message parameter *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_ASYNC_NULL_MSG_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendAsync with NULL message parameter *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NULL_MSG_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSend with zero timeout *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.open_clbk_invo != 114 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
   {
      result = TET_UNRESOLVED;
      goto final1;
   }

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_ERR_TIMEOUT_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send to queue that does not exist *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_09()
{
   int result;

   mqsv_print_testcase(" \n\n ***** SendAsync to queue that does not exist *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_10()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send to queue group that does not exist *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_BAD_GROUP_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_11()
{
   int result;

   mqsv_print_testcase(" \n\n ***** SendAsync to queue group that does not exist *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_BAD_GROUP_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_12()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a message queue - saMsgMessageSend *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;


   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_13()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a message queue - saMsgMessageSendAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;


   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_14()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a message queue group - saMsgMessageSend *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_GR_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_15()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a message queue group - saMsgMessageSendAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_GR_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_16()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a member queue of a group - saMsgMessageSend *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_17()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a member queue of a group - saMsgMessageSendAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_18()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Delivered clbk in called when saMsgMessageSendAsync is called with ackFlag = 1 *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.del_clbk_invo == 309 && gl_mqa_env.del_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_19()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Invocation in delivered clbk is same as that given in saMsgMessageSendAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.del_clbk_invo == 309 && gl_mqa_env.del_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_20()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Process is not intimated of delivery when message is sent with ackFlags = 0 *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NO_ACK_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.del_clbk_invo == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_21()
{
   int result;

   mqsv_print_testcase(" \n\n ***** SendAsync without registering with delivered callback (ackFlags = 1) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CBKS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_ERR_INIT2_T,TEST_NONCONFIG_MODE);
 
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CBKS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_22()
{
   int result;

   mqsv_print_testcase(" \n\n ***** SendAsync without registering with delivered callback (ackFlags = 0) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CBKS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_ERR_INIT_T,TEST_NONCONFIG_MODE);
 
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CBKS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_23()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendAsync with invalid ackFlags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_BAD_FLAGS_T,TEST_NONCONFIG_MODE);
 
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_24()
{
   int result;
   int size;

   mqsv_print_testcase(" \n\n ***** Send to a queue that is full *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_SMALL_SIZE_ATTR_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   size = gl_mqa_env.small_cr_attribs.size[gl_mqa_env.send_msg.priority];
   size -= gl_mqa_env.send_msg.size;

   while(size > 0)
   {
      sleep(2);

      result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
      if(result != TET_PASS)
         goto final2;

      size -= gl_mqa_env.send_msg.size;
   }

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_QUEUE_FULL_T,TEST_NONCONFIG_MODE);
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_25()
{
   int result;
   int size;

   mqsv_print_testcase(" \n\n ***** SendAsync to a queue that is full *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_SMALL_SIZE_ATTR_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   size = gl_mqa_env.small_cr_attribs.size[gl_mqa_env.send_msg.priority];
   size -= gl_mqa_env.send_msg.size;

   while(size > 0)
   {
      sleep(2);

      result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
      if(result != TET_PASS)
         goto final2;

      size -= gl_mqa_env.send_msg.size;
   }

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_QUEUE_FULL_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.del_clbk_invo == 321 && gl_mqa_env.del_clbk_err == SA_AIS_ERR_QUEUE_FULL)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_26()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send to an empty queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_UNAVAILABLE_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_27()
{
   int result;

   mqsv_print_testcase(" \n\n ***** SendAsync to an empty queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_UNAVAILABLE_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_28()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message with invalid priority *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_WITH_BAD_PR_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_29()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendAsync with a message with invalid priority *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_WITH_BAD_PR_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_30()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send to a zero size queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_SIZE_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_TO_ZERO_QUEUE_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_31()
{
   int result;

   mqsv_print_testcase(" \n\n ***** SendAsync to a zero size queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_SIZE_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_ZERO_QUEUE_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_32()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send to an unavailable queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_33()
{
   int result;

   mqsv_print_testcase(" \n\n ***** SendAsync to an unavailable queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_34()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message with NULL sender name to a queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_MSG2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.senderName->length != 0)
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_35()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendAsync a message with NULL sender name to a queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_SUCCESS_Q1_MSG2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.del_clbk_invo != 324 || gl_mqa_env.del_clbk_err != SA_AIS_OK)
   {
      result = TET_FAIL;
      goto final2;
   }
 
   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.senderName->length != 0)
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_36()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a big message to a small size queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_BIG_MSG_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_37()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendAsync with a  big message to a small size queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_BIG_MSG_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.del_clbk_invo != 323 || gl_mqa_env.del_clbk_err != SA_AIS_ERR_QUEUE_FULL)
      result = TET_FAIL;
   else
      result = TET_PASS;
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_38()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send to a member queue of a group that is full *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_SIZE_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_ZEROQ_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_GR_QUEUE_FULL_T,TEST_NONCONFIG_MODE);
 
final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_39()
{
   int result;

   mqsv_print_testcase(" \n\n ***** SendAsync to a member queue of a group that is full *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_SIZE_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_ZEROQ_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_GR_QUEUE_FULL_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;
 
   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.del_clbk_invo != 329 || gl_mqa_env.del_clbk_err != SA_AIS_ERR_QUEUE_FULL)
   {
      result = TET_FAIL;
      goto final3;
   }
 
final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_40()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a message queue with zero size - saMsgMessageSend *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_ZERO_SIZE_MSG_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg_zero_size.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg_zero_size.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg_zero_size.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg_zero_size.senderName->value) &&
      gl_mqa_env.rcv_msg.data == NULL &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg_zero_size.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_41()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a message queue with zero size - saMsgMessageSendAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_ZERO_SIZE_MSG_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.del_clbk_invo != 330 || gl_mqa_env.del_clbk_err != SA_AIS_OK)
      result = TET_FAIL;
   else
      result = TET_PASS;
 
   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg_zero_size.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg_zero_size.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg_zero_size.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg_zero_size.senderName->value) &&
      gl_mqa_env.rcv_msg.data == NULL &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg_zero_size.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_42()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send to a multicast group with members queues that are full *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_SIZE_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_ZEROQ_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_MULTI_CAST_GR_FULL_T,TEST_NONCONFIG_MODE);
 
final5:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_send_43()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send Async to a multicast group with members queues that are full *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_SIZE_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_ZEROQ_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_MULTI_CAST_GR_FULL_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final5;
 
   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ALL_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.del_clbk_invo != 331 || gl_mqa_env.del_clbk_err != SA_AIS_ERR_QUEUE_FULL)
      result = TET_FAIL;
 
final5:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgMessageGet Api Tests ************/

void mqsv_it_msg_get_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet with invalid queue handle *****\n");

   mqsv_print_testcase(" \n 1. Invalid queue handle \n");

   result1 = tet_test_msgMessageGet(MSG_MESSAGE_GET_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2. Closed queue handle \n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result2 = tet_test_msgMessageGet(MSG_MESSAGE_GET_CLOSED_Q_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet with queue handle associated with finalized message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_CLOSED_Q_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_AFTER_FINALIZE);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet with NULL receive message *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_NULL_MSG_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet with NULL sender id *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_NULL_SENDER_ID_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet with zero timeout with no message in the queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_ERR_TIMEOUT_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet with zero timeout when there is a message in the queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_ZERO_TIMEOUT_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.rcv_msg.size) &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_ZERO_TIMEOUT_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive a message from the queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.rcv_msg.size) &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive a message from the queue with NULL send time *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_NULL_SEND_TIME_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.rcv_msg.size) &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_09()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Message is removed from the queue once it is received *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.q_status.saMsgQueueUsage[gl_mqa_env.send_msg.priority].queueUsed == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[gl_mqa_env.send_msg.priority].numberOfMessages == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

final3:
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_10()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Messages are received in their priority order (higher to lower) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_MSG2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.rcv_msg.priority != gl_mqa_env.send_msg.priority)
   {
      result = TET_FAIL;  
      goto final3;
   }

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.rcv_msg.priority != gl_mqa_env.send_msg_null_sndr_name.priority)
      result = TET_FAIL;  

final3:
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_11()
{
   int result;
   SaUint32T type1,type2;

   mqsv_print_testcase(" \n\n ***** Messages of same priority are receive in the order in which they are sent *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   type1 = gl_mqa_env.send_msg.type;
   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   gl_mqa_env.send_msg.type = type2 = 12;
   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.rcv_msg.type != type1)
   {
      result = TET_FAIL;  
      goto final3;
   }

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.rcv_msg.type != type2)
      result = TET_FAIL;  

final3:
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_12()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet with NULL data in message parameter *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_ZERO_TIMEOUT_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.rcv_msg.size) &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_ZERO_TIMEOUT_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_13()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet with NULL data in message parameter *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_ZERO_TIMEOUT_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.rcv_msg.size) &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_ZERO_TIMEOUT_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_14()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive a message when data in message parameter is not NULL *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   gl_mqa_env.rcv_msg.size = gl_mqa_env.send_msg.size;
   gl_mqa_env.rcv_msg.data = calloc(gl_mqa_env.send_msg.size,sizeof(char));

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.send_msg.size))
      result = TET_PASS;
   else
      result = TET_FAIL;  

   if(gl_mqa_env.rcv_msg.data)
      free(gl_mqa_env.rcv_msg.data);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_15()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet with data buffer too small to hold the received message *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_ERR_NO_SPACE_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_ERR_NO_SPACE_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_16()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet updates the size of message when return SA_AIS_ERR_NO_SPACE *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_ERR_NO_SPACE_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.no_space_rcv_msg.size == gl_mqa_env.send_msg.size)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_ERR_NO_SPACE_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_17()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageGet updates the size of message when return SA_AIS_OK *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   gl_mqa_env.rcv_msg.size = gl_mqa_env.send_msg.size + 10;
   gl_mqa_env.rcv_msg.data = calloc(gl_mqa_env.rcv_msg.size,sizeof(char));

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.send_msg.size) &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg.size)
      result = TET_PASS;
   else
      result = TET_FAIL;

   if(gl_mqa_env.rcv_msg.data)
      free(gl_mqa_env.rcv_msg.data);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_18()
{
   int result;
   SaMsgSenderIdT sender_id1,sender_id2;

   mqsv_print_testcase(" \n\n ***** When saMsgMessageSend or SendAsync is used to send, sender_id = 0 *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.del_clbk_invo != 308 || gl_mqa_env.del_clbk_err != SA_AIS_OK)
   {
      result = TET_FAIL;
      goto final2;
   }
 
   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   sender_id1 = gl_mqa_env.sender_id;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   sender_id2 = gl_mqa_env.sender_id;

   if(sender_id1 == 0 && sender_id2 == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

final3:
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_19()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive a message from the queue with non-NULL send time parameter *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.send_time != 0)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_20()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive a message from the queue with NULL senderName *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_NULL_SNDR_NAME_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg_null_sndr_name.senderName == NULL)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_NULL_SNDR_NAME_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_21()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive a message from the queue when no senderName is sent *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_MSG2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.senderName->length == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_get_22()
{
   int result;

   mqsv_print_testcase(" \n\n ***** when saMsgMessageGet returns SA_AIS_ERR_NO_SPACE the message is not consumed *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_ERR_NO_SPACE_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;

final3:
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_ERR_NO_SPACE_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgMessageCancel Api Tests ************/

void mqsv_it_msg_cancel_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgMessageCancel with invalid queue handle *****\n");

   mqsv_print_testcase(" \n 1. Invalid queue handle \n");

   result1 = tet_test_msgMessageCancel(MSG_MESSAGE_CANCEL_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2. Closed queue handle \n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result2 = tet_test_msgMessageCancel(MSG_MESSAGE_CANCEL_CLOSED_Q_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_cancel_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageCancel with queue handle associated with finalized message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result = tet_test_msgMessageCancel(MSG_MESSAGE_CANCEL_CLOSED_Q_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_AFTER_FINALIZE);

final:
   mqsv_result(result);
}

void mqsv_it_msg_cancel_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageCancel when there is no blocking call to saMsgMessageGet *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageCancel(MSG_MESSAGE_CANCEL_NO_BLKING_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_cancel_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageCancel cancels the blocking call to saMsgMessageGet *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_createcancelthread(&gl_mqa_env.pers_q_hdl);

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_ERR_INTERRUPT_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgMessageSendReceive Api Tests ************/

void mqsv_it_msg_sendrcv_01()
{
   int result,result1,result2;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive with invalid message handle *****\n");

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   result2 = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive with NULL destination *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_NULL_NAME_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive with NULL sendMessage *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_NULL_MSG_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive with NULL reply buffer *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_NULL_RMSG_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive with NULL send time *****\n");

   gl_reply_result = 0;
   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withReply;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_REPLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_NULL_STIME_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_reply_result == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_NULL_STIME_T);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to send and receive a message from a queue *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withReplyAsync;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_REPLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.reply_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.reply_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.reply_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.reply_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.reply_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.reply_msg.size) &&
      gl_mqa_env.reply_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to send and receive a message from a group *****\n");

   gl_reply_result = 0;
   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withReply;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_REPLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_SUCCESS_GR_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   if(gl_reply_result == TET_PASS && gl_mqa_env.reply_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.reply_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.reply_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.reply_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.reply_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.reply_msg.size) &&
      gl_mqa_env.reply_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive with non-NULL send time parameter *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withReply;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_REPLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.reply_send_time == 0)
       result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_09()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_ERR_TIMEOUT_T;

   mqsv_print_testcase(" \n\n ***** When message is sent using saMsgMessageSendReceive, sender_id != 0 *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.sender_id == 0)
      result = TET_FAIL;
   else
      result = TET_PASS;

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_10()
{
   int result;
   int size;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to a destination queue that is full *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_SMALL_SIZE_ATTR_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   size = gl_mqa_env.small_cr_attribs.size[gl_mqa_env.send_msg.priority];
   size -= gl_mqa_env.send_msg.size;

   while(size > 0)
   {
      sleep(2);

      result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
      if(result != TET_PASS)
         break;

      size -= gl_mqa_env.send_msg.size;
   }

   if(result != TET_PASS)
      goto final2;


   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_QUEUE_FULL_T,TEST_NONCONFIG_MODE);
 
   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_11()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to a destination queue of zero size *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_SIZE_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_ZERO_Q_T,TEST_NONCONFIG_MODE);
 
   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_12()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to a destination without reply *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_ERR_TIMEOUT_T,TEST_NONCONFIG_MODE);
 
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_13()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to a destination queue that does not exist *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);
 
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_14()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to a destination queue group that does not exist *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_NOT_EXIST_GR_T,TEST_NONCONFIG_MODE);
 
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_15()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive with insufficient reply buffer *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withReply_nospace;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_REPLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_ERR_NO_SPACE_T,TEST_NONCONFIG_MODE);

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_ERR_NO_SPACE_T);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_16()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to an unavailable queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_17()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to an empty queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_UNAVALABLE_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_18()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive with non-NULL data in reply buffer *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withReply;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_REPLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   gl_mqa_env.reply_msg.size = gl_mqa_env.send_msg.size;
   gl_mqa_env.reply_msg.data = calloc(gl_mqa_env.send_msg.size,sizeof(char));

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && !strncmp(gl_mqa_env.reply_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.reply_msg.size))
       result = TET_PASS;
   else
       result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_19()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive with invalid priority in sendMessage *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_BAD_PR_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_20()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to a member queue of a queue group *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withReplyAsync;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_REPLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_21()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to a member queue of a queue group that is full *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ZERO_SIZE_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_ZEROQ_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_GR_QUEUE_FULL_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_22()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to send a message with NULL sender name *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withReplyAsync;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_REPLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_SUCCESS_MSG2_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_MSG2_T);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_23()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive with NULL sender name in the reply buffer *****\n");

   gl_reply_result = -1;

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withReplyAsync;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_REPLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_NULL_SNAME_T,TEST_NONCONFIG_MODE);

   m_MQSV_WAIT;

   if(gl_mqa_env.rcv_clbk_qhdl != 0 && gl_reply_result == TET_PASS && gl_mqa_env.reply_msg_null_sndr_name.senderName == NULL)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_MSG2_T);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_24()
{
   int result;

   mqsv_print_testcase(" \n\n ***** SendReceive with sender name in the reply buffer and reply with NULL sender name *****\n");

   gl_reply_result = -1;

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withReply_nullSname;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_REPLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_SUCCESS_T,TEST_NONCONFIG_MODE);

   m_MQSV_WAIT;

   if(gl_mqa_env.rcv_clbk_qhdl != 0 && gl_reply_result == TET_PASS && gl_mqa_env.reply_msg.senderName->length == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_MSG2_T);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_25()
{
   int result;

   gl_get_result = -1;
   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to send a message with zero size to a queue *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withMsgGet;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_RECV_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_ZERO_SIZE_MSG_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_get_result == TET_PASS && gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg_zero_size.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg_zero_size.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg_zero_size.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg_zero_size.senderName->value) &&
      gl_mqa_env.rcv_msg.data == NULL &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg_zero_size.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_sendrcv_26()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageSendReceive to a multicast queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_RECV_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_BROADCAST_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_MULTI_CAST_GR_T,TEST_NONCONFIG_MODE);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** saMsgMessageReply and saMsgMessageReplyAsync Api Tests ************/

void mqsv_it_msg_reply_01()
{
   int result,result1,result2;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_ERR_TIMEOUT_T;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReply with invalid message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_BAD_HANDLE_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result2 = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);
   goto final;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_02()
{
   int result,result1,result2;

   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_ERR_TIMEOUT_T;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReplyAsync with invalid message handle *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   mqsv_print_testcase(" \n 1.***** Uninitialized message handle ******\n");

   result1 = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_BAD_HDL_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

   result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
   {
      mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);
      goto final;
   }

   mqsv_print_testcase(" \n 2.****** Finalized message handle ******\n");

   result2 = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_FINALIZED_HDL_T,TEST_NONCONFIG_MODE);

   if(result1 == TET_PASS && result2 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);
   goto final;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReply with NULL replyMessage *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_NULL_MSG_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReplyAsync with NULL replyMessage *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_NULL_MSG_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReply with NULL pointer to sender_id *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_NULL_SID_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReplyAsync with NULL pointer to sender_id *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_NULL_SID_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReply with invalid sender_id *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReplyAsync with invalid sender_id *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_09()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_ERR_NO_SPACE_T;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReply with a reply message size greater than the reply buffer size *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_ERR_NO_SPACE_T,TEST_NONCONFIG_MODE);

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_ERR_NO_SPACE_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_10()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_ERR_NO_SPACE_T;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReplyAsync with a message size greater than the reply buffer size *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_ERR_NO_SPACE_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ALL_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.del_clbk_invo != 511 ||
      gl_mqa_env.del_clbk_err != SA_AIS_ERR_NO_SPACE)
      result = TET_FAIL;

   mqsv_clean_clbk_params();

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_ERR_NO_SPACE_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_11()
{
   int result;

   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_SUCCESS_T;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReplyAsync without registering delivered callback (ackFlags = 0) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CBKS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_ERR_INIT_T,TEST_NONCONFIG_MODE);

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CBKS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_12()
{
   int result;

   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_ERR_TIMEOUT_T;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReplyAsync without registering delivered callback (ackFlags = 1) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_NULL_CBKS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_ERR_INIT2_T,TEST_NONCONFIG_MODE);

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_NULL_CBKS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_13()
{
   int result;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReplyAsync with invalid ackFlags *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_BAD_FLAGS_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_14()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_SUCCESS_T;

   mqsv_print_testcase(" \n\n ***** Reply a message using saMsgMessageReply *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_SUCCESS_T,TEST_NONCONFIG_MODE);

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_15()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_SUCCESS_T;

   mqsv_print_testcase(" \n\n ***** Reply a message using saMsgMessageReplyAsync with acknowledgement *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.del_clbk_invo == 506 && gl_mqa_env.del_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_16()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_SUCCESS_T;

   mqsv_print_testcase(" \n\n ***** Reply a message using saMsgMessageReplyAsync without acknowledgement *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_SUCCESS2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.del_clbk_invo == 0 && gl_mqa_env.del_clbk_err == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_17()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_SUCCESS_T;

   mqsv_print_testcase(" \n\n ***** Process cannot reply to a message more than once - saMsgMessageReply *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   sleep(10);

final3:
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_18()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_SUCCESS_T;

   mqsv_print_testcase(" \n\n ***** Process cannot reply to a message more than once - saMsgMessageReplyAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_SUCCESS2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   sleep(10);

final3:
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_19()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_SUCCESS_T;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReply with NULL sender name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_NULL_SNDR_NAME_T,TEST_NONCONFIG_MODE);

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_20()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_SUCCESS_T;

   mqsv_print_testcase(" \n\n ***** saMsgMessageReplyAsync with NULL sender name *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_NULL_SNDR_NAME_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.del_clbk_invo == 512 && gl_mqa_env.del_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_21()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Reply to a message that is not sent by saMsgMessageSendReceive *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_INV_PARAM_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_22()
{
   int result;

   mqsv_print_testcase(" \n\n ***** ReplyAsync to a message that is not sent by saMsgMessageSendReceive *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_INV_PARAM_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_23()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_SUCCESS_T;

   mqsv_print_testcase(" \n\n ***** Reply a message with zero message size *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_ZERO_SIZE_MSG_T,TEST_NONCONFIG_MODE);

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_reply_24()
{
   int result;
   MSG_MESSAGE_SEND_RECV_TC_TYPE tc = MSG_MESSAGE_SEND_RECV_SUCCESS_T;

   mqsv_print_testcase(" \n\n ***** Reply a message using saMsgMessageReplyAsync with zero size message *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   mqsv_create_sendrecvthread(&tc);

   m_MQSV_WAIT;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_ZERO_SIZE_MSG_T,TEST_NONCONFIG_MODE);

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS && gl_mqa_env.del_clbk_invo == 513 && gl_mqa_env.del_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;

   sleep(10);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/*********** SINGLE NODE Functionality Tests **************/

/* Message Queues */

void mqsv_it_msgqs_01()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Messages can be written to a message queue - saMsgMessageSend *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Messages can be written to a message queue - saMsgMessageSendAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_ZERO_TIMEOUT_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.rcv_msg.size) &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_ZERO_TIMEOUT_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Messages can be read from a message queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_ZERO_TIMEOUT_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.rcv_msg.size) &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_ZERO_TIMEOUT_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Non-Persistent queue will be removed if it is closed for retention time *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   sleep(11);

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_NOT_EXIST_Q2_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS)
      goto final1;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Non-Persistent queue will be removed if it is closed for retention time *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.open_clbk_invo != 115 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
   {
      result = TET_UNRESOLVED;
      goto final1;
   }

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL5_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   sleep(11);

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_NOT_EXIST_Q2_T,TEST_NONCONFIG_MODE);
   if(result == TET_PASS)
      goto final1;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Persistent queue will not be removed if it is closed for retention time *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   sleep(13);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_EXIST_SUCCESS_T,TEST_NONCONFIG_MODE);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Persistent queue will not be removed if it is closed for retention time *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.open_clbk_invo != 114 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
   {
      result = TET_UNRESOLVED;
      goto final1;
   }

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL5_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   sleep(13);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_EXIST_SUCCESS_T,TEST_NONCONFIG_MODE);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Opening a closed Non-Persistent queue before completion of retention time *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   sleep(5);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NPERS_EXIST_SUCCESS_T,TEST_NONCONFIG_MODE);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_09()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Opening a closed Non-Persistent queue before completion of retention time *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   sleep(5);

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_EXIST_SUCCESS2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.open_clbk_invo != 129 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
      result = TET_FAIL;
   else
      result = TET_PASS;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_10()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Message Service preserves messages that are not consumed - Non-Persistent *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   sleep(5);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NPERS_EXIST_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[gl_mqa_env.send_msg.priority].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_11()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Message Service preserves messages that are not consumed - Persistent *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   sleep(10);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_EXIST_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[gl_mqa_env.send_msg.priority].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_12()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Process should open the queue before retrieving messages from it *****\n");

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_BAD_HDL_T,TEST_NONCONFIG_MODE);
   mqsv_result(result);
}

void mqsv_it_msgqs_13()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a message queue empty - saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_MSG2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_EMPTY_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[1].queueUsed == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[1].numberOfMessages == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[2].queueUsed == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[2].numberOfMessages == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[3].queueUsed == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[3].numberOfMessages == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_14()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a message queue empty - saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_NPERS_EMPTY_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.open_clbk_invo != 120 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
   {
      result = TET_UNRESOLVED;
      goto final2;
   }

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[1].queueUsed == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[1].numberOfMessages == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[2].queueUsed == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[2].numberOfMessages == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[3].queueUsed == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[3].numberOfMessages == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_15()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a closed queue - saMsgMessageSend *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[gl_mqa_env.send_msg.priority].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_16()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Send a message to a closed queue - saMsgMessageSendAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME1_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS || gl_mqa_env.del_clbk_invo != 308 || gl_mqa_env.del_clbk_err != SA_AIS_OK)
   {
      result = TET_FAIL;
      goto final2;
   }
 
   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == gl_mqa_env.send_msg.size &&
      gl_mqa_env.q_status.saMsgQueueUsage[gl_mqa_env.send_msg.priority].numberOfMessages == 1)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_17()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Open a queue, Send a message, Close it, Open the queue and get the message *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_EXIST_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_18()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive callback is invoked when a message is in a queue - saMsgQueueOpen *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_clbk_qhdl == gl_mqa_env.pers_q_hdl)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_19()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive callback is invoked when a message is in a queue - saMsgQueueOpenAsync *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   if(gl_mqa_env.open_clbk_invo != 122 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
   {
      result = TET_UNRESOLVED;
      goto final1;
   }

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   if(gl_mqa_env.rcv_clbk_qhdl == gl_mqa_env.open_clbk_qhdl)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_20()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive callback is called when a non-empty closed "
         "queue is opened with rcv clbk flag *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_EXIST_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_clbk_qhdl != gl_mqa_env.pers_q_hdl)
      result = TET_FAIL;
   else
      result = TET_PASS;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_21()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive callback is not invoked when queue is not opened with "
      "SA_MSG_QUEUE_RECEIVE_CALLBACK open flag *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_clbk_qhdl != 0)
      result = TET_FAIL;
   else
      result = TET_PASS;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_22()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive a message from unlinked message queue - Persistent *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   m_MQSV_WAIT;

   if(gl_mqa_env.open_clbk_invo != 114 || gl_mqa_env.open_clbk_err != SA_AIS_OK)
   {
      result = TET_UNRESOLVED;
      goto final1;
   }

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   gl_mqa_env.pers_q_hdl = gl_mqa_env.open_clbk_qhdl;
   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgqs_23()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive a message from unlinked message queue - Non-Persistent *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_HDL2_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/* Message Queue Groups */

void mqsv_it_msgq_grps_01()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Message queues can be inserted into a queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURRENT_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.buffer_null_notif.numberOfItems == 1 &&
      !strcmp(gl_mqa_env.buffer_null_notif.notification->member.queueName.value,gl_mqa_env.pers_q.value) &&
      gl_mqa_env.buffer_null_notif.notification->change == SA_MSG_QUEUE_GROUP_NO_CHANGE)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_T);

   result = tet_test_msgGroupRemove(MSG_GROUP_REMOVE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURRENT_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.buffer_null_notif.numberOfItems == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_T);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgq_grps_02()
{
   int result;
   int numOfMsgs = 0;

   mqsv_print_testcase(" \n\n ***** Messages sent to a unicast group are sent to only one member queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_GR_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   numOfMsgs = gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages;

   mqsv_clean_q_status();

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   if(numOfMsgs == 1 && gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

final4:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgq_grps_03()
{
   int result,result1,result2,result3;

   mqsv_print_testcase(" \n\n ***** Messages sent to a group follow the group policy *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withMsgGet;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_RECV_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_GR_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   m_MQSV_WAIT;

   if(gl_mqa_env.rcv_clbk_qhdl == gl_mqa_env.pers_q_hdl)
      result1 = TET_PASS;
   else
      result1 = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_clean_clbk_params();

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_GR_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   m_MQSV_WAIT;

   if(gl_mqa_env.rcv_clbk_qhdl == gl_mqa_env.npers_q_hdl)
      result2 = TET_PASS;
   else
      result2 = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_clean_clbk_params();

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_GR_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   m_MQSV_WAIT;

   if(gl_mqa_env.rcv_clbk_qhdl == gl_mqa_env.pers_q_hdl)
      result3 = TET_PASS;
   else
      result3 = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   mqsv_clean_clbk_params();

   if(result1 == TET_PASS && result2 == TET_PASS && result3 == TET_PASS)
      result = TET_PASS;
   else
      result = TET_FAIL;

final4:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msgq_grps_04()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Queue groups can be created as unicast or multicast type *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_BROADCAST2_T,TEST_NONCONFIG_MODE);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_BROADCAST2_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgq_grps_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Unlinking a member queue will remove the queue from queue group *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CURRENT_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.buffer_null_notif.numberOfItems == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_restore_params(MSG_RESTORE_GROUP_TRACK_CURRENT_T);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgq_grps_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track callback is invoked when non-persistent member queue"
      " is kept closed for retention time *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   sleep(10);

   m_MQSV_WAIT;

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 0 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;
   else
      result = TET_PASS;

final4:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_T);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgq_grps_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track callback is invoked when non-persistent member queue"
      " is kept closed for retention time *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHGS_ONLY_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   sleep(10);

   m_MQSV_WAIT;

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 0 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.non_pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;
   else
      result = TET_PASS;

final4:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHGS_ONLY_T);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgq_grps_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** When a queue is deleted, it will be deleted from all groups it is member of *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_GR2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_Q1_GROUP2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_GR2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final6;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final6;

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 0 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
   {
      result = TET_FAIL;
      goto final6;
   }

   mqsv_clean_clbk_params();

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final6;

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 0 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup2.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;

final6:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_GR2_T);

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_T);

final4:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_GR2_T);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgq_grps_09()
{
   int result;

   mqsv_print_testcase(" \n\n ***** When a queue is deleted, track callback is invoked for all the "
      "groups it is member of *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_GR2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_Q1_GROUP2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_GR2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final6;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final6;

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 0 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup1.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
   {
      result = TET_FAIL;
      goto final6;
   }

   mqsv_clean_clbk_params();

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final6;

   if(gl_mqa_env.track_clbk_err != SA_AIS_OK || gl_mqa_env.track_clbk_num_mem != 0 ||
      strcmp(gl_mqa_env.track_clbk_grp_name.value,gl_mqa_env.qgroup2.value) ||
      gl_mqa_env.track_clbk_notif.numberOfItems != 1 ||
      gl_mqa_env.track_clbk_notif.queueGroupPolicy != SA_MSG_QUEUE_GROUP_ROUND_ROBIN ||
      strcmp(gl_mqa_env.track_clbk_notif.notification[0].member.queueName.value,gl_mqa_env.pers_q.value) ||
      gl_mqa_env.track_clbk_notif.notification[0].change != SA_MSG_QUEUE_GROUP_REMOVED)
      result = TET_FAIL;

final6:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_GR2_T);

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_T);

final4:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_GR2_T);

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final2:
   if(result != TET_PASS)
      mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgq_grps_10()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track callback is not invoked when a member queue is closed (TRACK CHANGES) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHANGES_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != 0)
   {
      result = TET_FAIL;
      goto final5;
   }

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   mqsv_clean_clbk_params();

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(2);

   if(gl_mqa_env.track_clbk_err != 0)
      result = TET_FAIL;

   mqsv_clean_clbk_params();

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHANGES_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgq_grps_11()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Track callback is not invoked when a member queue is closed (TRACK CHANGES ONLY) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CHGS_ONLY_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   sleep(3);

   if(gl_mqa_env.track_clbk_err != 0)
   {
      result = TET_FAIL;
      goto final5;
   }

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(3);

   mqsv_clean_clbk_params();

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(2);

   if(gl_mqa_env.track_clbk_err != 0)
      result = TET_FAIL;

   mqsv_clean_clbk_params();

final5:
   mqsv_q_grp_track_stop(MSG_STOP_GROUP_TRACK_CHGS_ONLY_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final3:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msgq_grps_12()
{
   int result;

   gl_track_clbk_iter = 0;
   mqsv_print_testcase(" \n\n ***** A scenario for group tracking with two groups and two queues *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgGroupCreate(MSG_GROUP_CREATE_SUCCESS_GR2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final4;

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_Q2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CUR_CH_NUL_BUF_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(2);
   mqsv_clean_clbk_params();

   result = tet_test_msgGroupTrack(MSG_GROUP_TRACK_CUR_CHLY_NUL_BUF_GR2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(2);
   mqsv_clean_clbk_params();

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_Q1_GROUP2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(2);
   mqsv_clean_clbk_params();

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(2);
   mqsv_clean_clbk_params();

   result = tet_test_msgGroupInsert(MSG_GROUP_INSERT_Q2_GROUP2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final5;

   sleep(2);
   mqsv_clean_clbk_params();

final5:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final4:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

   if(result == TET_PASS && gl_track_clbk_iter == 9)
      result = TET_PASS;
   else
      result = TET_FAIL;

final3:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_GR2_T);

final2:
   mqsv_q_grp_cleanup(MSG_CLEAN_GROUP_CREATE_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/* Message Delivery Properties */

void mqsv_it_msg_delprop_01()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Messages are received in their priority order (higher to lower) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_MSG2_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.rcv_msg.priority != gl_mqa_env.send_msg.priority)
   {
      result = TET_FAIL;  
      goto final3;
   }

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.rcv_msg.priority != gl_mqa_env.send_msg_null_sndr_name.priority)
      result = TET_FAIL;  

final3:
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_delprop_02()
{
   int result;
   SaUint32T type1,type2;

   mqsv_print_testcase(" \n\n ***** Messages of same priority are receive in the FIFO order *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   type1 = gl_mqa_env.send_msg.type;
   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   gl_mqa_env.send_msg.type = type2 = 12;
   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.rcv_msg.type != type1)
   {
      result = TET_FAIL;  
      goto final3;
   }

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.rcv_msg.type != type2)
      result = TET_FAIL;  

final3:
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_delprop_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Messages that are received from the queue are removed from the queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final3;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == 0 &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 0)
      result = TET_PASS;
   else
      result = TET_FAIL;

final3:
   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_delprop_04()
{
   int result;
   int size;

   mqsv_print_testcase(" \n\n ***** SendAsync to a queue that is full *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_SMALL_SIZE_ATTR_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   size = gl_mqa_env.small_cr_attribs.size[gl_mqa_env.send_msg.priority];
   size -= gl_mqa_env.send_msg.size;

   while(size > 0)
   {
      sleep(2);

      result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
      if(result != TET_PASS)
         goto final2;

      size -= gl_mqa_env.send_msg.size;
   }

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_QUEUE_FULL_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.del_clbk_invo == 321 && gl_mqa_env.del_clbk_err == SA_AIS_ERR_QUEUE_FULL)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_delprop_05()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Processes can opt for acknowledgement for message delivery *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME2_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_NONCONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.del_clbk_invo == 309 && gl_mqa_env.del_clbk_err == SA_AIS_OK)
      result = TET_PASS;
   else
      result = TET_FAIL;
 
final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_delprop_06()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Messages never expire in message queue *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   sleep(10);

   result = tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.q_status.saMsgQueueUsage[0].queueUsed == 60 &&
      gl_mqa_env.q_status.saMsgQueueUsage[0].numberOfMessages == 2)
      result = TET_PASS;
   else
      result = TET_FAIL;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_delprop_07()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive messages from received callback *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withMsgGet;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_RECV_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   result = tet_test_msgDispatch(MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   if(gl_mqa_env.rcv_clbk_qhdl == gl_mqa_env.pers_q_hdl &&
      gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.rcv_msg.size) &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_delprop_08()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Receive messages from received callback *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withMsgGet;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_RECV_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   if(gl_mqa_env.del_clbk_invo == 308 && gl_mqa_env.del_clbk_err == SA_AIS_OK &&
      gl_mqa_env.rcv_clbk_qhdl == gl_mqa_env.pers_q_hdl &&
      gl_mqa_env.rcv_msg.type == gl_mqa_env.send_msg.type &&
      gl_mqa_env.rcv_msg.version == gl_mqa_env.send_msg.version &&
      gl_mqa_env.rcv_msg.size == gl_mqa_env.send_msg.size &&
      !strcmp(gl_mqa_env.rcv_msg.senderName->value,gl_mqa_env.send_msg.senderName->value) &&
      !strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,gl_mqa_env.rcv_msg.size) &&
      gl_mqa_env.rcv_msg.priority == gl_mqa_env.send_msg.priority)
      result = TET_PASS;
   else
      result = TET_FAIL;  

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_delprop_09()
{
   int result;

   mqsv_print_testcase(" \n\n ***** Received callback is invoked when the queue is closed and opened *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withMsgGet;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_RECV_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   if(gl_mqa_env.rcv_clbk_qhdl == 0)
   {
      result = TET_FAIL;
      goto final2;
   }

   mqsv_clean_clbk_params();

   result = tet_test_msgQueueClose(MSG_QUEUE_CLOSE_SUCCESS_HDL1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
     goto final2;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_EXIST_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   m_MQSV_WAIT;

   if(gl_mqa_env.rcv_clbk_qhdl == 0)
      result = TET_FAIL;
   else
      result = TET_PASS;

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,
                       App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,
                       App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_delprop_10()
{
   int result;
   int i=1;

   mqsv_print_testcase(" \n\n ***** Sending and Receiving messages in a loop *****\n");

   gl_mqa_env.gen_clbks.saMsgMessageReceivedCallback = App_saMsgMessageReceivedCallback_withMsgGet;

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   while(i++ < 10)
   {
      result = tet_test_msgMessageSend(MSG_MESSAGE_SEND_SUCCESS_NAME1_T,TEST_CONFIG_MODE);
      if(result != TET_PASS)
         break;

      m_MQSV_WAIT;

      if(gl_mqa_env.rcv_clbk_qhdl == 0 || strncmp(gl_mqa_env.rcv_msg.data,gl_mqa_env.send_msg.data,
         gl_mqa_env.rcv_msg.size))
      {
         result = TET_FAIL;
         break;
      }

      mqsv_clean_clbk_params();
      mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);
   }

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_fill_msg_clbks(&gl_mqa_env.gen_clbks,App_saMsgQueueOpenCallback,
                       App_saMsgQueueGroupTrackCallback,
                       App_saMsgMessageDeliveredCallback,
                       App_saMsgMessageReceivedCallback);
   mqsv_result(result);
}

void mqsv_it_msg_delprop_11()
{
   int result;
   int i=1;

   gl_del_clbk_iter = 0;
   mqsv_print_testcase(" \n\n ***** Delivered callback is invoked when send async is successful *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   mqsv_createthread(&gl_mqa_env.msg_hdl1);

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   while(i++ <= 10)
   {
      result = tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_NAME1_T,TEST_CONFIG_MODE);
      if(result != TET_PASS)
         break;

      m_MQSV_WAIT;
   }

   if(gl_del_clbk_iter == 10)
      result = TET_PASS;
   else
      result = TET_FAIL;

   mqsv_clean_clbk_params();
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_delprop_12()
{
   int result;

   mqsv_print_testcase(" \n\n ***** If saMsgMessageSendReceive becomes timeout before the reply "
      "is sent, Reply returns SA_AIS_ERR_NOT_EXIST *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_ERR_TIMEOUT_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReply(MSG_MESSAGE_REPLY_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_msg_delprop_13()
{
   int result;

   mqsv_print_testcase(" \n\n ***** If saMsgMessageSendReceive becomes timeout before the reply "
      "is sent, ReplyAsync returns SA_AIS_ERR_NOT_EXIST *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   result = tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_ERR_TIMEOUT_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageGet(MSG_MESSAGE_GET_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final2;

   result = tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_NOT_EXIST_T,TEST_NONCONFIG_MODE);

   mqsv_restore_params(MSG_RESTORE_MESSAGE_GET_SUCCESS_T);

final2:
   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

/******** ERR_TRY_AGAIN Testcases *********/

void mqsv_it_err_try_again_01()
{
   int result;

   mqsv_print_testcase(" \n\n ***** API TEST for SA_AIS_ERR_TRY_AGAIN (case 1) *****\n");

   printf(" KILL MQD/MQND AND PRESS ENTER TO CONTINUE\n");
   getchar();

   result = tet_test_msgInitialize(MSG_INIT_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   mqsv_result(result);
}

void mqsv_it_err_try_again_02()
{
   int result;

   mqsv_print_testcase(" \n\n ***** API TEST for SA_AIS_ERR_TRY_AGAIN (case 2) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   printf(" KILL MQD/MQND AND PRESS ENTER TO CONTINUE\n");
   getchar();

   tet_test_msgSelectionObject(MSG_SEL_OBJ_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgDispatch(MSG_DISPATCH_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgFinalize(MSG_FINALIZE_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgQueueOpen(MSG_QUEUE_OPEN_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgQueueOpenAsync(MSG_QUEUE_OPEN_ASYNC_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgQueueStatusGet(MSG_QUEUE_STATUS_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgGroupCreate(MSG_GROUP_CREATE_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgGroupInsert(MSG_GROUP_INSERT_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgGroupRemove(MSG_GROUP_REMOVE_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgGroupDelete(MSG_GROUP_DELETE_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgGroupTrack(MSG_GROUP_TRACK_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgMessageSend(MSG_MESSAGE_SEND_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgMessageSendAsync(MSG_MESSAGE_SEND_ASYNC_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgMessageSendReceive(MSG_MESSAGE_SEND_RECV_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   gl_mqa_env.sender_id = 21;

   tet_test_msgMessageReply(MSG_MESSAGE_REPLY_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgMessageReplyAsync(MSG_MESSAGE_REPLY_ASYNC_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}

void mqsv_it_err_try_again_03()
{
   int result;

   mqsv_print_testcase(" \n\n ***** API TEST for SA_AIS_ERR_TRY_AGAIN (case 3) *****\n");

   result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final;

   result = tet_test_msgQueueOpen(MSG_QUEUE_OPEN_PERS_SUCCESS_T,TEST_CONFIG_MODE);
   if(result != TET_PASS)
      goto final1;

   printf(" KILL MQD/MQND AND PRESS ENTER TO CONTINUE\n");
   getchar();

   tet_test_msgQueueClose(MSG_QUEUE_CLOSE_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgMessageGet(MSG_MESSAGE_GET_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   tet_test_msgMessageCancel(MSG_MESSAGE_CANCEL_ERR_TRY_AGAIN_T,TEST_NONCONFIG_MODE);

   mqsv_q_cleanup(MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T);

final1:
   mqsv_init_cleanup(MSG_CLEAN_INIT_SUCCESS_T);

final:
   mqsv_result(result);
}



/********* MQSV Inputs ************/

void tet_mqsv_get_inputs(TET_MQSV_INST *inst)
{
   char *tmp_ptr=NULL;

   memset(inst,'\0',sizeof(TET_MQSV_INST));

   tmp_ptr = (char *) getenv("TET_MQSV_INST_NUM");
   if(tmp_ptr)
   {
      inst->inst_num = atoi(tmp_ptr);
      tmp_ptr = NULL;
      gl_mqsv_inst_num = inst->inst_num;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_TET_LIST_INDEX");
   if(tmp_ptr)
   {
      inst->tetlist_index = atoi(tmp_ptr);
      tmp_ptr = NULL;
      gl_tetlist_index = inst->tetlist_index;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_TEST_CASE_NUM");
   if(tmp_ptr)
   {
      inst->test_case_num = atoi(tmp_ptr);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_NUM_ITER");
   if(tmp_ptr)
   {
      inst->num_of_iter = atoi(tmp_ptr);
      tmp_ptr = NULL;
   }
   inst->node_id = m_NCS_GET_NODE_ID;
   gl_nodeId = inst->node_id;
   TET_MQSV_NODE1 = gl_nodeId;

   tmp_ptr = (char *) getenv("MQA_NODE_ID_2");
   if(tmp_ptr)
   {
      TET_MQSV_NODE2 = atoi(tmp_ptr);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("MQA_NODE_ID_3");
   if(tmp_ptr)
   {
      TET_MQSV_NODE3 = atoi(tmp_ptr);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_PERS_Q_NAME1");
   if(tmp_ptr)
   {
      strcpy(inst->pers_q_name1.value,tmp_ptr);
      inst->pers_q_name1.length = strlen(inst->pers_q_name1.value);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_NON_PERS_Q_NAME1");
   if(tmp_ptr)
   {
      strcpy(inst->non_pers_q_name1.value,tmp_ptr);
      inst->non_pers_q_name1.length = strlen(inst->non_pers_q_name1.value);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_ZERO_Q_NAME");
   if(tmp_ptr)
   {
      strcpy(inst->zero_q_name.value,tmp_ptr);
      inst->zero_q_name.length = strlen(inst->zero_q_name.value);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_PERS_Q_NAME2");
   if(tmp_ptr)
   {
      strcpy(inst->pers_q_name2.value,tmp_ptr);
      inst->pers_q_name2.length = strlen(inst->pers_q_name2.value);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_NON_PERS_Q_NAME2");
   if(tmp_ptr)
   {
      strcpy(inst->non_pers_q_name2.value,tmp_ptr);
      inst->non_pers_q_name2.length = strlen(inst->non_pers_q_name2.value);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_Q_GRP_NAME1");
   if(tmp_ptr)
   {
      strcpy(inst->q_grp_name1.value,tmp_ptr);
      inst->q_grp_name1.length = strlen(inst->q_grp_name1.value);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_Q_GRP_NAME2");
   if(tmp_ptr)
   {
      strcpy(inst->q_grp_name2.value,tmp_ptr);
      inst->q_grp_name2.length = strlen(inst->q_grp_name2.value);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_Q_GRP_NAME3");
   if(tmp_ptr)
   {
      strcpy(inst->q_grp_name3.value,tmp_ptr);
      inst->q_grp_name3.length = strlen(inst->q_grp_name3.value);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_RED_FLAG");
   if(tmp_ptr)
   {
      gl_msg_red_flg = atoi(tmp_ptr);
      tmp_ptr = NULL;
   }

   tmp_ptr = (char *) getenv("TET_MQSV_WAIT_TIME");
   if(tmp_ptr)
   {
      gl_mqsv_wait_time = atoi(tmp_ptr);
      tmp_ptr = NULL;
   }
}

void tet_mqsv_fill_inputs(TET_MQSV_INST *inst)
{
   if(inst->pers_q_name1.length)
   {
      memset(&gl_mqa_env.pers_q,'\0',sizeof(SaNameT));
      strcpy(gl_mqa_env.pers_q.value,inst->pers_q_name1.value);
      gl_mqa_env.pers_q.length = inst->pers_q_name1.length;
   }
   if(inst->non_pers_q_name1.length)
   {
      memset(&gl_mqa_env.non_pers_q,'\0',sizeof(SaNameT));
      strcpy(gl_mqa_env.non_pers_q.value,inst->non_pers_q_name1.value);
      gl_mqa_env.non_pers_q.length = inst->non_pers_q_name1.length;
   }
   if(inst->zero_q_name.length)
   {
      memset(&gl_mqa_env.zero_q,'\0',sizeof(SaNameT));
      strcpy(gl_mqa_env.zero_q.value,inst->zero_q_name.value);
      gl_mqa_env.zero_q.length = inst->zero_q_name.length;
   }
   if(inst->pers_q_name2.length)
   {
      memset(&gl_mqa_env.pers_q2,'\0',sizeof(SaNameT));
      strcpy(gl_mqa_env.pers_q2.value,inst->pers_q_name2.value);
      gl_mqa_env.pers_q2.length = inst->pers_q_name2.length;
   }
   if(inst->non_pers_q_name2.length)
   {
      memset(&gl_mqa_env.non_pers_q,'\0',sizeof(SaNameT));
      strcpy(gl_mqa_env.non_pers_q.value,inst->non_pers_q_name2.value);
      gl_mqa_env.non_pers_q.length = inst->non_pers_q_name2.length;
   }
   if(inst->q_grp_name1.length)
   {
      memset(&gl_mqa_env.qgroup1,'\0',sizeof(SaNameT));
      strcpy(gl_mqa_env.qgroup1.value,inst->q_grp_name1.value);
      gl_mqa_env.qgroup1.length = inst->q_grp_name1.length;
   }
   if(inst->q_grp_name2.length)
   {
      memset(&gl_mqa_env.qgroup2,'\0',sizeof(SaNameT));
      strcpy(gl_mqa_env.qgroup2.value,inst->q_grp_name2.value);
      gl_mqa_env.qgroup2.length = inst->q_grp_name2.length;
   }
   if(inst->q_grp_name3.length)
   {
      memset(&gl_mqa_env.qgroup3,'\0',sizeof(SaNameT));
      strcpy(gl_mqa_env.qgroup3.value,inst->q_grp_name3.value);
      gl_mqa_env.qgroup3.length = inst->q_grp_name3.length;
   }
}


#endif
