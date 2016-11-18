#if (TET_A == 1)

#include "tet_mqsv.h"
#include "tet_mqa_conf.h"

#define END_OF_WHILE while((rc == SA_AIS_ERR_TRY_AGAIN) || (rc == SA_AIS_ERR_TIMEOUT))

#define MQSV_RETRY_WAIT sleep(1)

int gl_msg_thrd_cnt;
int gl_try_again_cnt;

int mqsv_test_result(SaAisErrorT rc,SaAisErrorT exp_out,char *test_case,MQSV_CONFIG_FLAG flg)
{
   int result = 0;

   if(rc == SA_AIS_ERR_TRY_AGAIN)
   {
      if(gl_try_again_cnt++ == 0)
      {
         m_TET_MQSV_PRINTF("\n RETRY           : %s\n",test_case);
         m_TET_MQSV_PRINTF(" Return Value    : %s\n\n",mqsv_saf_error_string[rc]);
      }

      if(!(gl_try_again_cnt%10))
         m_TET_MQSV_PRINTF(" Retry Count : %d \n",gl_try_again_cnt);

      MQSV_RETRY_WAIT;

      return(result);
   }

   if(rc == exp_out)
   {
      if(flg != TEST_CLEANUP_MODE)
         m_TET_MQSV_PRINTF("\n SUCCESS         : %s\n",test_case);
      result = TET_PASS;
   }
   else
   {
      m_TET_MQSV_PRINTF("\n FAILED          : %s\n",test_case);
      if(flg == TEST_CLEANUP_MODE)
         tet_printf("\n FAILED          : %s\n",test_case);
      result = TET_FAIL;
      if(flg == TEST_CONFIG_MODE)
         result = TET_UNRESOLVED;
   }

   if(flg != TEST_CLEANUP_MODE || (flg == TEST_CLEANUP_MODE && result != TET_PASS))
   {
      m_TET_MQSV_PRINTF(" Return Value    : %s\n",mqsv_saf_error_string[rc]);
      if(flg == TEST_CLEANUP_MODE)
         tet_printf(" Return Value    : %s\n",mqsv_saf_error_string[rc]);
   }

   return(result);
}

void print_qinfo(SaNameT *qname,SaMsgQueueCreationAttributesT *cr_attr,SaMsgQueueOpenFlagsT op_flgs)
{
   if(qname)
      m_TET_MQSV_PRINTF("\n Queue name      : %s\n",qname->value);
   else
      m_TET_MQSV_PRINTF("\n Queue name      : NULL\n");

   if(cr_attr)
   {
      m_TET_MQSV_PRINTF(" size            : %llu\n",cr_attr->size[0]);
      if(cr_attr->creationFlags < 2)
         m_TET_MQSV_PRINTF(" creation flags  : %s\n",
            saMsgQueueCreationFlags_string[cr_attr->creationFlags]);
      else
         m_TET_MQSV_PRINTF(" creation flags  : %u\n",cr_attr->creationFlags);
   }
   else
      m_TET_MQSV_PRINTF(" creation attr   : NULL\n");

   if(op_flgs >= 0 && op_flgs < 8)
      m_TET_MQSV_PRINTF(" open flags      : %s",saMsgQueueOpenFlags_string[op_flgs]);
   else
      m_TET_MQSV_PRINTF(" open flags      : %u",op_flgs);
}

/* ***************  Print Message Queue Status ***************** */

void print_queueStatus(SaMsgQueueStatusT *queueStatus)
{
   int i;

   m_TET_MQSV_PRINTF("\n ******* Queue Status Parameters ******\n");
   m_TET_MQSV_PRINTF(" Creation Flags  : %u\n",queueStatus->creationFlags);
   m_TET_MQSV_PRINTF(" Retention time  : %llu\n",queueStatus->retentionTime);
   m_TET_MQSV_PRINTF(" Close Time      : %llu\n",queueStatus->closeTime);

   for (i=0; i<=3; i++)
   {
      m_TET_MQSV_PRINTF("\n Priority        : %d\n",i);
      m_TET_MQSV_PRINTF(" Queue Size      : %u\n",(queueStatus->saMsgQueueUsage[i]).queueSize);
      m_TET_MQSV_PRINTF(" Queue Used      : %llu\n",(queueStatus->saMsgQueueUsage[i]).queueUsed);
      m_TET_MQSV_PRINTF(" No of messages  : %u\n",(queueStatus->saMsgQueueUsage[i]).numberOfMessages);
   }
}

/* *************** Print Message Queue Group Track Info ***************** */

void groupTrackInfo(SaMsgQueueGroupNotificationBufferT *buffer)
{
   int i;
   SaMsgQueueGroupNotificationT *notification;
   SaNameT *q_name;

   m_TET_MQSV_PRINTF("\n ******* Track Info *******\n");
   m_TET_MQSV_PRINTF(" No of Items     : %u\n",buffer->numberOfItems);
   if (!buffer->notification)
      m_TET_MQSV_PRINTF(" Notification    : NULL\n");
   else
   {
      notification = buffer->notification;
      q_name = (SaNameT *)malloc(sizeof(SaNameT));
      for (i=0; i < buffer->numberOfItems; i++,notification++)
      {
         m_TET_MQSV_PRINTF(" member          : %s\n",notification->member.queueName.value);
         m_TET_MQSV_PRINTF(" change          : %s\n",
           saMsgQueueGroupChanges_string[notification->change]);
      }
      free(q_name);
   }

   m_TET_MQSV_PRINTF(" Group Policy    : %s\n",saMsgQueueGroupPolicy_string[buffer->queueGroupPolicy]);
   m_TET_MQSV_PRINTF(" **************************\n");
}

/* *************** Print SaMsgMessageT Structure ***************** */

void msgDump(SaMsgMessageT *msg)
{
   char p[5000]={0};
   m_TET_MQSV_PRINTF("\n Message Type    : %u\n",msg->type);
   m_TET_MQSV_PRINTF(" Message Version : %u\n",msg->version);
   m_TET_MQSV_PRINTF(" Message Size    : %llu\n",msg->size);
   if(msg->senderName)
   {
      if(msg->senderName->length != 0)
         m_TET_MQSV_PRINTF(" Sender Name     : %s\n",msg->senderName->value);
      else
         m_TET_MQSV_PRINTF(" Sender Name     : ZERO length\n");
   }
   else
      m_TET_MQSV_PRINTF(" Sender Name     : NULL\n");
   if(msg->data)
   {
      strncpy(p,msg->data,msg->size);
      m_TET_MQSV_PRINTF(" Message Data    : %s\n",p);
   }
   else
      m_TET_MQSV_PRINTF(" Message Data    : NULL\n");
   m_TET_MQSV_PRINTF(" Msg Priority    : %u\n",msg->priority);
}

/* ***************  Message Initialization Test cases  ***************** */

char *API_Mqsv_Initialize_resultstring[] = {
   [MSG_INIT_NULL_HANDLE_T]         = "saMsgInitialize with null handle",
   [MSG_INIT_NULL_VERSION_T]        = "saMsgInitialize with null version and callbacks",
   [MSG_INIT_NULL_PARAMS_T]         = "saMsgInitialize with all null parameters",
   [MSG_INIT_NULL_CLBK_PARAM_T]     = "saMsgInitialize with null callback parameter",
   [MSG_INIT_NULL_VERSION_CBKS_T]   = "saMsgInitialize with null version and callbacks",
   [MSG_INIT_BAD_VERSION_T]         = "saMsgInitialize with invalid version",
   [MSG_INIT_BAD_REL_CODE_T]        = "saMsgInitialize with version with invalid release code",
   [MSG_INIT_BAD_MAJOR_VER_T]       = "saMsgInitialize with version with invalid major version",
   [MSG_INIT_SUCCESS_T]             = "saMsgInitialize with all valid parameters",
   [MSG_INIT_SUCCESS_HDL2_T]        = "saMsgInitialize with all valid parameters",
   [MSG_INIT_NULL_CBKS_T]           = "saMsgInitialize with null callbacks",
   [MSG_INIT_NULL_CBKS2_T]          = "saMsgInitialize with null callbacks",
   [MSG_INIT_NULL_DEL_CBK_T]        = "saMsgInitialize with null Delivered callback",
   [MSG_INIT_NULL_RCV_CBK_T]        = "saMsgInitialize with null Received callback",
   [MSG_INIT_SUCCESS_RECV_T]        = "saMsgInitialize with Message get in Received Clbk",
   [MSG_INIT_SUCCESS_REPLY_T]       = "saMsgInitialize with Reply in Received Clbk",
   [MSG_INIT_ERR_TRY_AGAIN_T]       = "saMsgInitialize when service is not available",
};

struct SafMsgInitialize API_Mqsv_Initialize[] = {
   [MSG_INIT_NULL_HANDLE_T]             = {NULL,&gl_mqa_env.version,&gl_mqa_env.gen_clbks,
                                           SA_AIS_ERR_INVALID_PARAM},
   [MSG_INIT_NULL_VERSION_T]            = {&gl_mqa_env.msg_hdl1,NULL,&gl_mqa_env.gen_clbks,
                                           SA_AIS_ERR_INVALID_PARAM},
   [MSG_INIT_NULL_PARAMS_T]             = {NULL,NULL,NULL,SA_AIS_ERR_INVALID_PARAM},
   [MSG_INIT_NULL_CLBK_PARAM_T]         = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.version,NULL,SA_AIS_OK},
   [MSG_INIT_NULL_VERSION_CBKS_T]       = {&gl_mqa_env.msg_hdl1,NULL,NULL,SA_AIS_ERR_INVALID_PARAM},
   [MSG_INIT_BAD_VERSION_T]             = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.inv_params.inv_version,
                                           &gl_mqa_env.gen_clbks,SA_AIS_ERR_VERSION},
   [MSG_INIT_BAD_REL_CODE_T]            = {&gl_mqa_env.msg_hdl2,
                                           &gl_mqa_env.inv_params.inv_ver_bad_rel_code,
                                           &gl_mqa_env.gen_clbks,SA_AIS_ERR_VERSION},
   [MSG_INIT_BAD_MAJOR_VER_T]           = {&gl_mqa_env.msg_hdl2,
                                           &gl_mqa_env.inv_params.inv_ver_not_supp,
                                           &gl_mqa_env.gen_clbks,SA_AIS_ERR_VERSION},
   [MSG_INIT_SUCCESS_T]                 = {&gl_mqa_env.msg_hdl1,
                                           &gl_mqa_env.version,&gl_mqa_env.gen_clbks,SA_AIS_OK},
   [MSG_INIT_SUCCESS_HDL2_T]            = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.version,
                                           &gl_mqa_env.gen_clbks,SA_AIS_OK},
   [MSG_INIT_NULL_CBKS_T]               = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.version,
                                           &gl_mqa_env.null_clbks,SA_AIS_OK},
   [MSG_INIT_NULL_CBKS2_T]              = {&gl_mqa_env.null_clbks_msg_hdl,&gl_mqa_env.version,
                                           &gl_mqa_env.null_clbks,SA_AIS_OK},
   [MSG_INIT_NULL_DEL_CBK_T]            = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.version,
                                           &gl_mqa_env.null_del_clbks,SA_AIS_OK},
   [MSG_INIT_NULL_RCV_CBK_T]            = {&gl_mqa_env.null_rcv_clbk_msg_hdl,&gl_mqa_env.version,
                                           &gl_mqa_env.null_rcv_clbks,SA_AIS_OK},
   [MSG_INIT_SUCCESS_RECV_T]            = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.version,
                                           &gl_mqa_env.gen_clbks,SA_AIS_OK},
   [MSG_INIT_SUCCESS_REPLY_T]           = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.version,
                                           &gl_mqa_env.gen_clbks,SA_AIS_OK},
   [MSG_INIT_ERR_TRY_AGAIN_T]           = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.version,
                                           &gl_mqa_env.gen_clbks,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgInitialize(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgInitialize(API_Mqsv_Initialize[i].msgHandle,API_Mqsv_Initialize[i].callbks,
                        API_Mqsv_Initialize[i].version);

   result = mqsv_test_result(rc,API_Mqsv_Initialize[i].exp_output,
                             API_Mqsv_Initialize_resultstring[i],flg);

   if(rc == SA_AIS_OK && flg != TEST_CLEANUP_MODE)
      m_TET_MQSV_PRINTF(" Message Handle  : %llu\n",*API_Mqsv_Initialize[i].msgHandle);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgInitialize(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgInitialize(API_Mqsv_Initialize[i].msgHandle,API_Mqsv_Initialize[i].callbks,
                           API_Mqsv_Initialize[i].version);

      result = mqsv_test_result(rc,API_Mqsv_Initialize[i].exp_output,
                                API_Mqsv_Initialize_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(rc == SA_AIS_OK && flg != TEST_CLEANUP_MODE)
            m_TET_MQSV_PRINTF(" Message Handle  : %llu\n",*API_Mqsv_Initialize[i].msgHandle);
      }

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Selection Object Test cases  ***************** */

char *API_Mqsv_Selection_resultstring[] = {
   [MSG_SEL_OBJ_BAD_HANDLE_T]    = "saMsgSelectionObjGet with invalid Message Handle",
   [MSG_SEL_OBJ_NULL_SEL_OBJ_T]  = "saMsgSelectionObjGet with Null selection object parameter",
   [MSG_SEL_OBJ_SUCCESS_T]       = "saMsgSelectionObjGet with all valid parameters",
   [MSG_SEL_OBJ_FINALIZED_HDL_T] = "saMsgSelectionObjGet with finalized message handle",
   [MSG_SEL_OBJ_ERR_TRY_AGAIN_T] = "saMsgSelectionObjGet when service is not available",
};

struct SafMsgSelectionObject API_Mqsv_Selection[] = {
   [MSG_SEL_OBJ_BAD_HANDLE_T]    = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.sel_obj,
                                    SA_AIS_ERR_BAD_HANDLE},
   [MSG_SEL_OBJ_NULL_SEL_OBJ_T]  = {&gl_mqa_env.msg_hdl1,NULL,SA_AIS_ERR_INVALID_PARAM},
   [MSG_SEL_OBJ_SUCCESS_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.sel_obj,SA_AIS_OK},
   [MSG_SEL_OBJ_FINALIZED_HDL_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.sel_obj,SA_AIS_ERR_BAD_HANDLE},
   [MSG_SEL_OBJ_ERR_TRY_AGAIN_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.sel_obj,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgSelectionObject(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgSelectionObjectGet(*API_Mqsv_Selection[i].msgHandle,API_Mqsv_Selection[i].selobj);

   result = mqsv_test_result(rc,API_Mqsv_Selection[i].exp_output,
                             API_Mqsv_Selection_resultstring[i],flg);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgSelectionObject(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgSelectionObjectGet(*API_Mqsv_Selection[i].msgHandle,API_Mqsv_Selection[i].selobj);

      result = mqsv_test_result(rc,API_Mqsv_Selection[i].exp_output,
                                API_Mqsv_Selection_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Dispatch Test cases  ***************** */

char *API_Mqsv_Dispatch_resultstring[] = {
   [MSG_DISPATCH_ONE_BAD_HDL_T]              = "saMsgDispatch with invalid Message Handle - DISPATCH_ONE",
   [MSG_DISPATCH_ONE_FINALIZED_HDL_T]        = "saMsgDispatch with finalized Message Handle - DISPATCH_ONE",
   [MSG_DISPATCH_ALL_BAD_HDL_T]              = "saMsgDispatch with invalid Message Handle - DISPATCH_ALL",
   [MSG_DISPATCH_ALL_FINALIZED_HDL_T]        = "saMsgDispatch with finalized Message Handle - DISPATCH_ALL",
   [MSG_DISPATCH_BLKING_BAD_HDL_T]           = "saMsgDispatch with invalid Message Handle - DISPATCH_BLOCKING",
   [MSG_DISPATCH_BLKING_FINALIZED_HDL_T]     = "saMsgDispatch with finalized Message Handle - DISPATCH_BLOCKING",
   [MSG_DISPATCH_BAD_FLAGS_T]                = "saMsgDispatch with Bad Flags",
   [MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T]     = "saMsgDispatch with valid params and DISPATCH_ONE flag",
   [MSG_DISPATCH_DISPATCH_ALL_SUCCESS_T]     = "saMsgDispatch with valid params and DISPATCH_ALL flag",
   [MSG_DISPATCH_DISPATCH_BLOCKING_SUCCESS_T] = "saMsgDispatch with valid params and DISPATCH_BLOCKING flag",
   [MSG_DISPATCH_ERR_TRY_AGAIN_T]             = "saMsgDispatch when service is not available",
};

struct SafMsgDispatch API_Mqsv_Dispatch[] = {
   [MSG_DISPATCH_ONE_BAD_HDL_T]             = {&gl_mqa_env.inv_params.inv_msg_hdl,SA_DISPATCH_ONE,
                                               SA_AIS_ERR_BAD_HANDLE},
   [MSG_DISPATCH_ONE_FINALIZED_HDL_T]       = {&gl_mqa_env.msg_hdl1,SA_DISPATCH_ONE,
                                               SA_AIS_ERR_BAD_HANDLE},
   [MSG_DISPATCH_ALL_BAD_HDL_T]             = {&gl_mqa_env.inv_params.inv_msg_hdl,SA_DISPATCH_ALL,
                                               SA_AIS_ERR_BAD_HANDLE},
   [MSG_DISPATCH_ALL_FINALIZED_HDL_T]       = {&gl_mqa_env.msg_hdl1,SA_DISPATCH_ALL,
                                               SA_AIS_ERR_BAD_HANDLE},
   [MSG_DISPATCH_BLKING_BAD_HDL_T]          = {&gl_mqa_env.inv_params.inv_msg_hdl,SA_DISPATCH_BLOCKING,
                                               SA_AIS_ERR_BAD_HANDLE},
   [MSG_DISPATCH_BLKING_FINALIZED_HDL_T]      = {&gl_mqa_env.msg_hdl1,SA_DISPATCH_BLOCKING,
                                                 SA_AIS_ERR_BAD_HANDLE},
   [MSG_DISPATCH_BAD_FLAGS_T]                 = {&gl_mqa_env.msg_hdl1,-1,SA_AIS_ERR_INVALID_PARAM},
   [MSG_DISPATCH_DISPATCH_ONE_SUCCESS_T]      = {&gl_mqa_env.msg_hdl1,SA_DISPATCH_ONE,SA_AIS_OK},
   [MSG_DISPATCH_DISPATCH_ALL_SUCCESS_T]      = {&gl_mqa_env.msg_hdl1,SA_DISPATCH_ALL,SA_AIS_OK},
   [MSG_DISPATCH_DISPATCH_BLOCKING_SUCCESS_T] = {&gl_mqa_env.msg_hdl1,SA_DISPATCH_BLOCKING,SA_AIS_OK},
   [MSG_DISPATCH_ERR_TRY_AGAIN_T]             = {&gl_mqa_env.msg_hdl1,SA_DISPATCH_ONE,
                                                 SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgDispatch(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgDispatch(*API_Mqsv_Dispatch[i].msgHandle,API_Mqsv_Dispatch[i].flags);

   result = mqsv_test_result(rc,API_Mqsv_Dispatch[i].exp_output,
                             API_Mqsv_Dispatch_resultstring[i],flg);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgDispatch(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgDispatch(*API_Mqsv_Dispatch[i].msgHandle,API_Mqsv_Dispatch[i].flags);

      result = mqsv_test_result(rc,API_Mqsv_Dispatch[i].exp_output,
                                API_Mqsv_Dispatch_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Finalize Test cases  ***************** */

char *API_Mqsv_Finalize_resultstring[] = {
   [MSG_FINALIZE_BAD_HDL_T]       = "saMsgFinalize with invalid Message Handle",
   [MSG_FINALIZE_SUCCESS_T]       = "saMsgFinalize with all valid parameters",
   [MSG_FINALIZE_SUCCESS_HDL2_T]  = "saMsgFinalize with all valid parameters",
   [MSG_FINALIZE_SUCCESS_HDL3_T]  = "saMsgFinalize with all valid parameters",
   [MSG_FINALIZE_SUCCESS_HDL4_T]  = "saMsgFinalize with all valid parameters",
   [MSG_FINALIZE_FINALIZED_HDL_T] = "saMsgFinalize with finalized Message Handle",
   [MSG_FINALIZE_ERR_TRY_AGAIN_T] = "saMsgFinalize when service is not available",
};

struct SafMsgFinalize API_Mqsv_Finalize[] = {
   [MSG_FINALIZE_BAD_HDL_T]       = {&gl_mqa_env.inv_params.inv_msg_hdl,SA_AIS_ERR_BAD_HANDLE},
   [MSG_FINALIZE_SUCCESS_T]       = {&gl_mqa_env.msg_hdl1,SA_AIS_OK},
   [MSG_FINALIZE_SUCCESS_HDL2_T]  = {&gl_mqa_env.msg_hdl2,SA_AIS_OK},
   [MSG_FINALIZE_SUCCESS_HDL3_T]  = {&gl_mqa_env.null_clbks_msg_hdl,SA_AIS_OK},
   [MSG_FINALIZE_SUCCESS_HDL4_T]  = {&gl_mqa_env.null_rcv_clbk_msg_hdl,SA_AIS_OK},
   [MSG_FINALIZE_FINALIZED_HDL_T] = {&gl_mqa_env.msg_hdl1,SA_AIS_ERR_BAD_HANDLE},
   [MSG_FINALIZE_ERR_TRY_AGAIN_T] = {&gl_mqa_env.msg_hdl1,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgFinalize(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgFinalize(*API_Mqsv_Finalize[i].msgHandle);
   result = mqsv_test_result(rc,API_Mqsv_Finalize[i].exp_output,
                             API_Mqsv_Finalize_resultstring[i],flg);

   if(rc == SA_AIS_OK && flg != TEST_CLEANUP_MODE)
      m_TET_MQSV_PRINTF(" Finalized Msg Hdl : %llu\n",*API_Mqsv_Finalize[i].msgHandle);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgFinalize(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgFinalize(*API_Mqsv_Finalize[i].msgHandle);
      result = mqsv_test_result(rc,API_Mqsv_Finalize[i].exp_output,
                                API_Mqsv_Finalize_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(rc == SA_AIS_OK && flg != TEST_CLEANUP_MODE)
            m_TET_MQSV_PRINTF(" Finalized Msg Hdl : %llu\n",*API_Mqsv_Finalize[i].msgHandle);
      }

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Open Test cases  ***************** */

char *API_Mqsv_QueueOpen_resultstring[] = {
   [MSG_QUEUE_OPEN_BAD_HANDLE_T]          = "saMsgQueueOpen with invalid Message Handle",
   [MSG_QUEUE_OPEN_FINALIZED_HDL_T]       = "saMsgQueueOpen with finalized Message Handle",
   [MSG_QUEUE_OPEN_NULL_Q_HDL_T]          = "saMsgQueueOpen with Null Queue Handle",
   [MSG_QUEUE_OPEN_NULL_NAME_T]           = "saMsgQueueOpen with Null queue name",
   [MSG_QUEUE_OPEN_BAD_TIMEOUT_T]         = "saMsgQueueOpen with bad timeout value",
   [MSG_QUEUE_OPEN_INVALID_PARAM_T]       = "saMsgQueueOpen with Null attributes and Create flag",
   [MSG_QUEUE_OPEN_INVALID_PARAM_EMPTY_T] = "saMsgQueueOpen with Null attributes and Create | Empty flag",
   [MSG_QUEUE_OPEN_INVALID_PARAM2_EMPTY_T]  = "saMsgQueueOpen with creation attributes and empty open flag",
   [MSG_QUEUE_OPEN_INVALID_PARAM2_ZERO_T]   = "saMsgQueueOpen with creation attributes and zero open flag",
   [MSG_QUEUE_OPEN_INVALID_PARAM2_RC_CBK_T] = "saMsgQueueOpen with creation attributes and rcv clbk open flag",
   [MSG_QUEUE_OPEN_BAD_FLAGS_T]           = "saMsgQueueOpen with bad open flags",
   [MSG_QUEUE_OPEN_BAD_FLAGS2_T]          = "saMsgQueueOpen with bad creation flags",
   [MSG_QUEUE_OPEN_PERS_ERR_EXIST_T]      = "Open a queue that already exists with Create flag - Persistent",
   [MSG_QUEUE_OPEN_NPERS_ERR_EXIST_T]     = "Open a queue that already exists with Create flag - Non Persistent",
   [MSG_QUEUE_OPEN_PERS_ER_NOT_EXIST_T]   = "Open a queue that does not exist - NULL attr and zero flg - Persistent",
   [MSG_QUEUE_OPEN_PERS_ER_NOT_EXIST2_T]  = "Open a queue that does not exist - non_NULL attr and empty flg",
   [MSG_QUEUE_OPEN_NPERS_ER_NOT_EXIST_T]  = "Open a queue that does not exist - Non Persistent",
   [MSG_QUEUE_OPEN_ZERO_TIMEOUT_T]        = "saMsgQueueOpen with zero timeout",
   [MSG_QUEUE_OPEN_EMPTY_CREATE_T]        = "saMsgQueueOpen with Empty and Create open flags",
   [MSG_QUEUE_OPEN_ZERO_RET_T]            = "saMsgQueueOpen with zero retention time",
   [MSG_QUEUE_OPEN_ZERO_SIZE_T]           = "saMsgQueueOpen with zero size in creation attributes",
   [MSG_QUEUE_OPEN_PERS_SUCCESS_T]        = "saMsgQueueOpen with valid params - Persistent queue", 
   [MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T]    = "saMsgQueueOpen with valid params - Non Persistent",
   [MSG_QUEUE_OPEN_PERS_SUCCESS2_T]       = "saMsgQueueOpen with valid params - Persistent queue", 
   [MSG_QUEUE_OPEN_NON_PERS_SUCCESS2_T]   = "saMsgQueueOpen with valid params - Non Persistent",
   [MSG_QUEUE_OPEN_PERS_EXIST_SUCCESS_T]  = "Open a queue that already exists - Persistent",
   [MSG_QUEUE_OPEN_NPERS_EXIST_SUCCESS_T] = "Open a queue that already exists - Non Persistent",
   [MSG_QUEUE_OPEN_ERR_BUSY_T]            = "saMsgQueueOpen ERR_BUSY case",
   [MSG_QUEUE_OPEN_EXIST_ERR_BUSY_T]      = "Open a queue that is already open",
   [MSG_QUEUE_OPEN_NPERS_EMPTY_SUCCESS_T] = "Open a closed queue with Empty flag",
   [MSG_QUEUE_OPEN_PERS_EMPTY_SUCCESS_T] = "Open a closed queue with Empty flag",
   [MSG_QUEUE_OPEN_ERR_INIT_T]            = "saMsgQueueOpen - ERR_INIT case for Received callback",
   [MSG_QUEUE_OPEN_SMALL_SIZE_ATTR_T]     = "Open a queue with small size creation attributes",
   [MSG_QUEUE_OPEN_SMALL_SIZE_ATTR2_T]    = "Open a queue with small size creation attributes",
   [MSG_QUEUE_OPEN_BIG_SIZE_ATTR_T]       = "Open a queue with big size creation attributes",
   [MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T]     = "Open a queue with Recv Clbk flag - Persistent",
   [MSG_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T] = "Open a queue with Recv Clbk flag - Non Persistent",
   [MSG_QUEUE_OPEN_EXIST_RECV_CLBK_SUCCESS_T]    = "Open an existing queue with Recv Clbk flag",
   [MSG_QUEUE_OPEN_NPERS_RECV_CLBK_T]     = "saMsgQueueOpen a queue with Recv Clbk and Create flgs - Non Persistent",
   [MSG_QUEUE_OPEN_PERS_RECV_CLBK2_T]     = "saMsgQueueOpen a queue with Recv Clbk and Create flgs - Persistent",
   [MSG_QUEUE_OPEN_NPERS_RECV_CLBK2_T]    = "saMsgQueueOpen a queue with Recv Clbk and Create flgs - Non Persistent",
   [MSG_QUEUE_OPEN_EXISTING_PERS_T]       = "Open a queue that already exists with Rcv Clbk flg - Persistent",
   [MSG_QUEUE_OPEN_EXISTING_NPERS_T]      = "Open a queue that already exists with Rcv Clbk flg - Non Persistent",
   [MSG_QUEUE_OPEN_EXISTING_PERS2_T]      = "Open a queue that already exists with Rcv Clbk flg - Persistent",
   [MSG_QUEUE_OPEN_EXISTING_NPERS2_T]     = "Open a queue that already exists with Rcv Clbk flg - Non Persistent",
   [MSG_QUEUE_OPEN_ERR_TRY_AGAIN_T]       = "saMsgQueueOpen when service is not available",
};

struct SafMsgQueueOpen API_Mqsv_QueueOpen[] = {
   [MSG_QUEUE_OPEN_BAD_HANDLE_T]          = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.non_pers_q,
                                             &gl_mqa_env.npers_cr_attribs,SA_MSG_QUEUE_CREATE,
                                             APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,
                                             SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_OPEN_FINALIZED_HDL_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,
                                             &gl_mqa_env.npers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,
                                             SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_OPEN_NULL_Q_HDL_T]          = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,
                                             &gl_mqa_env.npers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,NULL,
                                             SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_NULL_NAME_T]           = {&gl_mqa_env.msg_hdl1,NULL,&gl_mqa_env.pers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,
                                             &gl_mqa_env.npers_q_hdl,SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_BAD_TIMEOUT_T]         = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,
                                             &gl_mqa_env.npers_cr_attribs,SA_MSG_QUEUE_CREATE,-1,
                                             &gl_mqa_env.npers_q_hdl,SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_INVALID_PARAM_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,NULL,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,
                                             SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_INVALID_PARAM_EMPTY_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,NULL,
                                             SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_EMPTY,APP_TIMEOUT,
                                             &gl_mqa_env.npers_q_hdl,SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_INVALID_PARAM2_EMPTY_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.npers_cr_attribs,
                                               SA_MSG_QUEUE_EMPTY,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,
                                               SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_INVALID_PARAM2_ZERO_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.npers_cr_attribs,
                                               0,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_INVALID_PARAM2_RC_CBK_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.npers_cr_attribs,
                                               SA_MSG_QUEUE_RECEIVE_CALLBACK,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,
                                               SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_BAD_FLAGS_T]           = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.pers_cr_attribs,
                                             0x0008,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,SA_AIS_ERR_BAD_FLAGS},
   [MSG_QUEUE_OPEN_BAD_FLAGS2_T]          = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,
                                             &gl_mqa_env.inv_params.inv_cr_attribs,SA_MSG_QUEUE_CREATE,APP_TIMEOUT,
                                             &gl_mqa_env.npers_q_hdl,SA_AIS_ERR_BAD_FLAGS},
   [MSG_QUEUE_OPEN_PERS_ERR_EXIST_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.npers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_ERR_EXIST},
   [MSG_QUEUE_OPEN_NPERS_ERR_EXIST_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.pers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,SA_AIS_ERR_EXIST},
   [MSG_QUEUE_OPEN_PERS_ER_NOT_EXIST_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,NULL,0,APP_TIMEOUT,
                                             &gl_mqa_env.pers_q_hdl,SA_AIS_ERR_NOT_EXIST},
   [MSG_QUEUE_OPEN_PERS_ER_NOT_EXIST2_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,NULL,SA_MSG_QUEUE_EMPTY,
                                             APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_ERR_NOT_EXIST},
   [MSG_QUEUE_OPEN_NPERS_ER_NOT_EXIST_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,NULL,0,APP_TIMEOUT,
                                             &gl_mqa_env.npers_q_hdl,SA_AIS_ERR_NOT_EXIST},
   [MSG_QUEUE_OPEN_ZERO_TIMEOUT_T]        = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.pers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,0,&gl_mqa_env.npers_q_hdl,SA_AIS_ERR_TIMEOUT},
   [MSG_QUEUE_OPEN_EMPTY_CREATE_T]        = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_EMPTY,APP_TIMEOUT,
                                             &gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ZERO_RET_T]            = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q2,&gl_mqa_env.zero_ret_time_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl2,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ZERO_SIZE_T]           = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.zero_q,&gl_mqa_env.zero_size_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_PERS_SUCCESS_T]        = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_NON_PERS_SUCCESS_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.npers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_PERS_SUCCESS2_T]       = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_NON_PERS_SUCCESS2_T]   = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.non_pers_q,&gl_mqa_env.npers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_PERS_EXIST_SUCCESS_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,NULL,0,APP_TIMEOUT,
                                             &gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_NPERS_EXIST_SUCCESS_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,NULL,0,APP_TIMEOUT,
                                             &gl_mqa_env.npers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ERR_BUSY_T]            = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_ERR_BUSY},
   [MSG_QUEUE_OPEN_EXIST_ERR_BUSY_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,NULL,0,APP_TIMEOUT,
                                             &gl_mqa_env.npers_q_hdl,SA_AIS_ERR_BUSY},
   [MSG_QUEUE_OPEN_NPERS_EMPTY_SUCCESS_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,NULL,SA_MSG_QUEUE_EMPTY,APP_TIMEOUT,
                                             &gl_mqa_env.npers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_PERS_EMPTY_SUCCESS_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,NULL,SA_MSG_QUEUE_EMPTY,APP_TIMEOUT,
                                             &gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ERR_INIT_T]            = {&gl_mqa_env.null_rcv_clbk_msg_hdl,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,APP_TIMEOUT,
                                             &gl_mqa_env.pers_q_hdl,SA_AIS_ERR_INIT},
   [MSG_QUEUE_OPEN_SMALL_SIZE_ATTR_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.small_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_SMALL_SIZE_ATTR2_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.small_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_BIG_SIZE_ATTR_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.big_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                                    SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,APP_TIMEOUT,
                                                    &gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.npers_cr_attribs,
                                                    SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,APP_TIMEOUT,
                                                    &gl_mqa_env.npers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_EXIST_RECV_CLBK_SUCCESS_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,NULL,SA_MSG_QUEUE_RECEIVE_CALLBACK,
                                                    APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_NPERS_RECV_CLBK_T]     = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.non_pers_q,&gl_mqa_env.npers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,APP_TIMEOUT,
                                             &gl_mqa_env.npers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_PERS_RECV_CLBK2_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q2,&gl_mqa_env.pers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,APP_TIMEOUT,
                                             &gl_mqa_env.pers_q_hdl2,SA_AIS_OK},
   [MSG_QUEUE_OPEN_NPERS_RECV_CLBK2_T]    = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.non_pers_q2,&gl_mqa_env.npers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,APP_TIMEOUT,
                                             &gl_mqa_env.npers_q_hdl2,SA_AIS_OK},
   [MSG_QUEUE_OPEN_EXISTING_PERS_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,NULL,SA_MSG_QUEUE_RECEIVE_CALLBACK,
                                             APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_EXISTING_NPERS_T]      = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.non_pers_q,NULL,SA_MSG_QUEUE_RECEIVE_CALLBACK,
                                             APP_TIMEOUT,&gl_mqa_env.npers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_OPEN_EXISTING_PERS2_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q2,NULL,SA_MSG_QUEUE_RECEIVE_CALLBACK,
                                             APP_TIMEOUT,&gl_mqa_env.pers_q_hdl2,SA_AIS_OK},
   [MSG_QUEUE_OPEN_EXISTING_NPERS2_T]     = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.non_pers_q2,NULL,SA_MSG_QUEUE_RECEIVE_CALLBACK,
                                             APP_TIMEOUT,&gl_mqa_env.npers_q_hdl2,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ERR_TRY_AGAIN_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                             SA_MSG_QUEUE_CREATE,APP_TIMEOUT,&gl_mqa_env.pers_q_hdl,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgQueueOpen(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgQueueOpen(*API_Mqsv_QueueOpen[i].msgHandle,API_Mqsv_QueueOpen[i].queueName,
                       API_Mqsv_QueueOpen[i].creationAttributes,API_Mqsv_QueueOpen[i].openFlags,
                       API_Mqsv_QueueOpen[i].timeout,API_Mqsv_QueueOpen[i].queueHandle);

   print_qinfo(API_Mqsv_QueueOpen[i].queueName,API_Mqsv_QueueOpen[i].creationAttributes,
               API_Mqsv_QueueOpen[i].openFlags);

   result = mqsv_test_result(rc,API_Mqsv_QueueOpen[i].exp_output,
                             API_Mqsv_QueueOpen_resultstring[i],flg);

   if(rc == SA_AIS_OK)
      m_TET_MQSV_PRINTF(" QueueHandle     : %llu\n",*API_Mqsv_QueueOpen[i].queueHandle);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgQueueOpen(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgQueueOpen(*API_Mqsv_QueueOpen[i].msgHandle,API_Mqsv_QueueOpen[i].queueName,
                          API_Mqsv_QueueOpen[i].creationAttributes,API_Mqsv_QueueOpen[i].openFlags,
                          API_Mqsv_QueueOpen[i].timeout,API_Mqsv_QueueOpen[i].queueHandle);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         print_qinfo(API_Mqsv_QueueOpen[i].queueName,API_Mqsv_QueueOpen[i].creationAttributes,
                     API_Mqsv_QueueOpen[i].openFlags);

      result = mqsv_test_result(rc,API_Mqsv_QueueOpen[i].exp_output,
                                API_Mqsv_QueueOpen_resultstring[i],flg);

      if(rc == SA_AIS_OK)
         m_TET_MQSV_PRINTF(" QueueHandle     : %llu\n",*API_Mqsv_QueueOpen[i].queueHandle);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Open Async Test cases  ***************** */

char *API_Mqsv_QueueOpenAsync_resultstring[] = {
   [MSG_QUEUE_OPEN_ASYNC_BAD_HANDLE_T]      = "saMsgQueueOpenAsync with invalid Message Handle",
   [MSG_QUEUE_OPEN_ASYNC_FINALIZED_HDL_T]   = "saMsgQueueOpenAsync with invalid Message Handle",
   [MSG_QUEUE_OPEN_ASYNC_NULL_NAME_T]       = "saMsgQueueOpenAsync with Null queue name",
   [MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM_T]   = "saMsgQueueOpenAsync with Null attributes and Create flag",
   [MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM_EMPTY_T]   = "saMsgQueueOpenAsync with Null attributes and Create | Empty flag",
   [MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_EMPTY_T]  = "saMsgQueueOpenAsync with creation attributes and Empty flag",
   [MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_ZERO_T]   = "saMsgQueueOpenAsync with creation attributes and zero flag",
   [MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_RC_CBK_T] = "saMsgQueueOpenAsync with creation attributes and rcv clbk flag",
   [MSG_QUEUE_OPEN_ASYNC_BAD_FLGS_T]        = "saMsgQueueOpenAsync with bad open flags",
   [MSG_QUEUE_OPEN_ASYNC_BAD_FLGS2_T]       = "saMsgQueueOpenAsync with bad creation flags",
   [MSG_QUEUE_OPEN_ASYNC_ERR_INIT_T]        = "saMsgQueueOpenAsync - ERR_INIT case for Open Callback",
   [MSG_QUEUE_OPEN_ASYNC_ERR_INIT2_T]       = "saMsgQueueOpenAsync - ERR_INIT case for Receive Callback",
   [MSG_QUEUE_OPEN_ASYNC_ERR_NOT_EXIST_T]   = "Open a queue that does not exist - Null attr and zero open flag",
   [MSG_QUEUE_OPEN_ASYNC_ERR_NOT_EXIST2_T]  = "Open a queue that does not exist - non-NULL attr and Rcv clbk flag",
   [MSG_QUEUE_OPEN_ASYNC_ERR_BUSY_T]        = "saMsgQueueOpenAsync - ERR_BUSY case",
   [MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T]    = "saMsgQueueOpenAsync with valid parameters - Persistent",
   [MSG_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T]   = "saMsgQueueOpenAsync with valid parameters - Non Persistent",
   [MSG_QUEUE_OPEN_ASYNC_EMPTY_CREATE_T]    = "saMsgQueueOpenAsync with valid parameters - Empty flag",
   [MSG_QUEUE_OPEN_ASYNC_ERR_EXIST_T]       = "Open a queue that already exists with Create flag - Persistent",
   [MSG_QUEUE_OPEN_ASYNC_ERR_EXIST2_T]      = "Open a queue that already exists with Create flag - Non Persistent",
   [MSG_QUEUE_OPEN_ASYNC_EXIST_SUCCESS_T]   = "Open a queue that already exists",
   [MSG_QUEUE_OPEN_ASYNC_EXIST_SUCCESS2_T]  = "Open a queue that already exists",
   [MSG_QUEUE_OPEN_ASYNC_NPERS_EMPTY_SUCCESS_T] = "Open a queue with Empty open flag",
   [MSG_QUEUE_OPEN_ASYNC_SMALL_SIZE_ATTR_T] = "Open a queue with small size attributes",
   [MSG_QUEUE_OPEN_ASYNC_PERS_RECV_CLBK_SUCCESS_T]  = "Open a queue with Recv Clbk - Persistent",
   [MSG_QUEUE_OPEN_ASYNC_PERS_RECV_CLBK_SUCCESS2_T] = "Open a queue with Recv Clbk - Persistent",
   [MSG_QUEUE_OPEN_ASYNC_NPERS_RECV_CLBK_SUCCESS_T] = "Open a queue with Recv Clbk - Non Persistent",
   [MSG_QUEUE_OPEN_ASYNC_EXIST_RECV_CLBK_SUCCESS_T] = "Open an existing queue with Recv Clbk open flag",
   [MSG_QUEUE_OPEN_ASYNC_ZERO_RET_T]        = "saMsgQueueOpenAsync with zero retention time creation attributes",
   [MSG_QUEUE_OPEN_ASYNC_ZERO_SIZE_T]       = "saMsgQueueOpenAsync with zero size creation attributes",
   [MSG_QUEUE_OPEN_ASYNC_EXIST_ERR_BUSY_T]  = "Open a queue that is already open with NULL attr and non-create open flag",
   [MSG_QUEUE_OPEN_ASYNC_EXIST_ERR_BUSY2_T] = "Open a queue that is already open with NULL attr and non-create open flag",
   [MSG_QUEUE_OPEN_ASYNC_PERS_EMPTY_SUCCESS_T] = "Open a queue with Empty open flag",
   [MSG_QUEUE_OPEN_ASYNC_ERR_TRY_AGAIN_T]   = "saMsgQueueOpenAsync when service is not available",
};

struct SafMsgQueueOpenAsync API_Mqsv_QueueOpenAsync[] = {
   [MSG_QUEUE_OPEN_ASYNC_BAD_HANDLE_T]      = {&gl_mqa_env.inv_params.inv_msg_hdl,100,&gl_mqa_env.non_pers_q,
                                               &gl_mqa_env.npers_cr_attribs,SA_MSG_QUEUE_CREATE,SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_OPEN_ASYNC_FINALIZED_HDL_T]   = {&gl_mqa_env.msg_hdl1,101,&gl_mqa_env.non_pers_q,
                                               &gl_mqa_env.npers_cr_attribs,SA_MSG_QUEUE_CREATE,SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_OPEN_ASYNC_NULL_NAME_T]       = {&gl_mqa_env.msg_hdl1,102,NULL,&gl_mqa_env.pers_cr_attribs,
                                               SA_MSG_QUEUE_CREATE,SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM_T]   = {&gl_mqa_env.msg_hdl1,103,&gl_mqa_env.non_pers_q,NULL,SA_MSG_QUEUE_CREATE,
                                               SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM_EMPTY_T]  = {&gl_mqa_env.msg_hdl1,103,&gl_mqa_env.non_pers_q,NULL,
                                                    SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_EMPTY,SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_EMPTY_T] = {&gl_mqa_env.msg_hdl1,104,&gl_mqa_env.non_pers_q,
                                                    &gl_mqa_env.npers_cr_attribs,SA_MSG_QUEUE_EMPTY,
                                                    SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_ZERO_T]  = {&gl_mqa_env.msg_hdl1,105,&gl_mqa_env.non_pers_q,
                                                    &gl_mqa_env.npers_cr_attribs,0,SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_ASYNC_INVALID_PARAM2_RC_CBK_T]= {&gl_mqa_env.msg_hdl1,106,&gl_mqa_env.non_pers_q,
                                                    &gl_mqa_env.npers_cr_attribs,SA_MSG_QUEUE_RECEIVE_CALLBACK,
                                                    SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_OPEN_ASYNC_BAD_FLGS_T]        = {&gl_mqa_env.msg_hdl1,107,&gl_mqa_env.non_pers_q,&gl_mqa_env.npers_cr_attribs,
                                               0x0008,SA_AIS_ERR_BAD_FLAGS}, 
   [MSG_QUEUE_OPEN_ASYNC_BAD_FLGS2_T]       = {&gl_mqa_env.msg_hdl1,108,&gl_mqa_env.non_pers_q,
                                               &gl_mqa_env.inv_params.inv_cr_attribs,SA_MSG_QUEUE_CREATE,
                                               SA_AIS_ERR_BAD_FLAGS},
   [MSG_QUEUE_OPEN_ASYNC_ERR_INIT_T]        = {&gl_mqa_env.null_clbks_msg_hdl,109,&gl_mqa_env.pers_q,
                                               &gl_mqa_env.pers_cr_attribs,SA_MSG_QUEUE_CREATE,SA_AIS_ERR_INIT}, 
   [MSG_QUEUE_OPEN_ASYNC_ERR_INIT2_T]       = {&gl_mqa_env.null_rcv_clbk_msg_hdl,110,&gl_mqa_env.pers_q,
                                               &gl_mqa_env.pers_cr_attribs,
                                               SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,SA_AIS_ERR_INIT}, 
   [MSG_QUEUE_OPEN_ASYNC_ERR_NOT_EXIST_T]   = {&gl_mqa_env.msg_hdl1,111,&gl_mqa_env.non_pers_q,NULL,0,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ASYNC_ERR_NOT_EXIST2_T]  = {&gl_mqa_env.msg_hdl1,112,&gl_mqa_env.non_pers_q,NULL,
                                               SA_MSG_QUEUE_RECEIVE_CALLBACK,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ASYNC_ERR_BUSY_T]        = {&gl_mqa_env.msg_hdl1,113,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                               SA_MSG_QUEUE_CREATE,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T]    = {&gl_mqa_env.msg_hdl1,114,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                               SA_MSG_QUEUE_CREATE,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T]   = {&gl_mqa_env.msg_hdl1,115,&gl_mqa_env.non_pers_q,
                                               &gl_mqa_env.npers_cr_attribs,SA_MSG_QUEUE_CREATE,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ASYNC_EMPTY_CREATE_T]    = {&gl_mqa_env.msg_hdl1,116,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                               SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_EMPTY,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ASYNC_ERR_EXIST_T]       = {&gl_mqa_env.msg_hdl1,117,&gl_mqa_env.pers_q,&gl_mqa_env.npers_cr_attribs,
                                               SA_MSG_QUEUE_CREATE,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_ERR_EXIST2_T]      = {&gl_mqa_env.msg_hdl1,118,&gl_mqa_env.non_pers_q,&gl_mqa_env.pers_cr_attribs,
                                               SA_MSG_QUEUE_CREATE,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ASYNC_EXIST_SUCCESS_T]   = {&gl_mqa_env.msg_hdl1,119,&gl_mqa_env.pers_q,NULL,0,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_NPERS_EMPTY_SUCCESS_T] = {&gl_mqa_env.msg_hdl1,120,&gl_mqa_env.non_pers_q,NULL,
                                                   SA_MSG_QUEUE_EMPTY,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_SMALL_SIZE_ATTR_T] = {&gl_mqa_env.msg_hdl1,121,&gl_mqa_env.pers_q,&gl_mqa_env.small_cr_attribs,
                                               SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_PERS_RECV_CLBK_SUCCESS_T]  = {&gl_mqa_env.msg_hdl1,122,&gl_mqa_env.pers_q,
                                                       &gl_mqa_env.pers_cr_attribs,
                                                       SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_PERS_RECV_CLBK_SUCCESS2_T] = {&gl_mqa_env.msg_hdl1,123,&gl_mqa_env.pers_q2,
                                                       &gl_mqa_env.pers_cr_attribs,
                                                       SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_NPERS_RECV_CLBK_SUCCESS_T] = {&gl_mqa_env.msg_hdl1,124,&gl_mqa_env.non_pers_q,
                                                       &gl_mqa_env.npers_cr_attribs,
                                                       SA_MSG_QUEUE_CREATE | SA_MSG_QUEUE_RECEIVE_CALLBACK,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_EXIST_RECV_CLBK_SUCCESS_T] = {&gl_mqa_env.msg_hdl1,125,&gl_mqa_env.pers_q,NULL,
                                                       SA_MSG_QUEUE_RECEIVE_CALLBACK,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_ZERO_RET_T]        = {&gl_mqa_env.msg_hdl1,126,&gl_mqa_env.non_pers_q2,
                                               &gl_mqa_env.zero_ret_time_cr_attribs,SA_MSG_QUEUE_CREATE,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ASYNC_ZERO_SIZE_T]       = {&gl_mqa_env.msg_hdl1,127,&gl_mqa_env.zero_q,&gl_mqa_env.zero_size_cr_attribs,
                                               SA_MSG_QUEUE_CREATE,SA_AIS_OK},
   [MSG_QUEUE_OPEN_ASYNC_EXIST_ERR_BUSY_T]  = {&gl_mqa_env.msg_hdl1,128,&gl_mqa_env.non_pers_q,NULL,SA_MSG_QUEUE_EMPTY,
                                               SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_EXIST_SUCCESS2_T]  = {&gl_mqa_env.msg_hdl1,129,&gl_mqa_env.non_pers_q,NULL,0,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_PERS_EMPTY_SUCCESS_T] = {&gl_mqa_env.msg_hdl1,130,&gl_mqa_env.pers_q,NULL,
                                                  SA_MSG_QUEUE_EMPTY,SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_EXIST_ERR_BUSY2_T] = {&gl_mqa_env.msg_hdl1,131,&gl_mqa_env.pers_q,NULL,SA_MSG_QUEUE_EMPTY,
                                               SA_AIS_OK}, 
   [MSG_QUEUE_OPEN_ASYNC_ERR_TRY_AGAIN_T]   = {&gl_mqa_env.msg_hdl1,132,&gl_mqa_env.pers_q,&gl_mqa_env.pers_cr_attribs,
                                               SA_MSG_QUEUE_CREATE,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgQueueOpenAsync(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgQueueOpenAsync(*API_Mqsv_QueueOpenAsync[i].msgHandle,API_Mqsv_QueueOpenAsync[i].invocation,
                            API_Mqsv_QueueOpenAsync[i].queueName,API_Mqsv_QueueOpenAsync[i].creationAttributes,
                            API_Mqsv_QueueOpenAsync[i].openFlags);

   print_qinfo(API_Mqsv_QueueOpenAsync[i].queueName,API_Mqsv_QueueOpenAsync[i].creationAttributes,
               API_Mqsv_QueueOpenAsync[i].openFlags);

   result = mqsv_test_result(rc,API_Mqsv_QueueOpenAsync[i].exp_output,
                             API_Mqsv_QueueOpenAsync_resultstring[i],flg);

   if(rc == SA_AIS_OK)
      m_TET_MQSV_PRINTF(" Invocation      : %llu\n",API_Mqsv_QueueOpenAsync[i].invocation);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgQueueOpenAsync(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgQueueOpenAsync(*API_Mqsv_QueueOpenAsync[i].msgHandle,
                               API_Mqsv_QueueOpenAsync[i].invocation,
                               API_Mqsv_QueueOpenAsync[i].queueName,
                               API_Mqsv_QueueOpenAsync[i].creationAttributes,
                               API_Mqsv_QueueOpenAsync[i].openFlags);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         print_qinfo(API_Mqsv_QueueOpenAsync[i].queueName,API_Mqsv_QueueOpenAsync[i].creationAttributes,
                     API_Mqsv_QueueOpenAsync[i].openFlags);

      result = mqsv_test_result(rc,API_Mqsv_QueueOpenAsync[i].exp_output,
                                API_Mqsv_QueueOpenAsync_resultstring[i],flg);

      if(rc == SA_AIS_OK)
         m_TET_MQSV_PRINTF(" Invocation      : %llu\n",API_Mqsv_QueueOpenAsync[i].invocation);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Close Test cases  ***************** */

char *API_Mqsv_QueueClose_resultstring[] = {
   [MSG_QUEUE_CLOSE_INV_HANDLE_T]    = "saMsgQueueClose with invalid Queue Handle",
   [MSG_QUEUE_CLOSE_SUCCESS_HDL1_T]  = "saMsgQueueClose with valid parameters",
   [MSG_QUEUE_CLOSE_SUCCESS_HDL2_T]  = "saMsgQueueClose with valid parameters",
   [MSG_QUEUE_CLOSE_SUCCESS_HDL3_T]  = "saMsgQueueClose with valid parameters",
   [MSG_QUEUE_CLOSE_SUCCESS_HDL4_T]  = "saMsgQueueClose with valid parameters",
   [MSG_QUEUE_CLOSE_BAD_HANDLE1_T]   = "saMsgQueueClose with Bad Queue Handle",
   [MSG_QUEUE_CLOSE_BAD_HANDLE2_T]   = "saMsgQueueClose with Bad Queue Handle",
   [MSG_QUEUE_CLOSE_SUCCESS_HDL5_T]  = "saMsgQueueClose with valid parameters",
   [MSG_QUEUE_CLOSE_ERR_TRY_AGAIN_T] = "saMsgQueueClose when service is not available",
};

struct SafMsgQueueClose API_Mqsv_QueueClose[] = {
   [MSG_QUEUE_CLOSE_INV_HANDLE_T]    = {&gl_mqa_env.inv_params.inv_q_hdl,SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_CLOSE_SUCCESS_HDL1_T]  = {&gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_CLOSE_SUCCESS_HDL2_T]  = {&gl_mqa_env.npers_q_hdl,SA_AIS_OK},
   [MSG_QUEUE_CLOSE_SUCCESS_HDL3_T]  = {&gl_mqa_env.pers_q_hdl2,SA_AIS_OK},
   [MSG_QUEUE_CLOSE_SUCCESS_HDL4_T]  = {&gl_mqa_env.npers_q_hdl2,SA_AIS_OK},
   [MSG_QUEUE_CLOSE_BAD_HANDLE1_T]   = {&gl_mqa_env.pers_q_hdl,SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_CLOSE_BAD_HANDLE2_T]   = {&gl_mqa_env.npers_q_hdl,SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_CLOSE_SUCCESS_HDL5_T]  = {&gl_mqa_env.open_clbk_qhdl,SA_AIS_OK},
   [MSG_QUEUE_CLOSE_ERR_TRY_AGAIN_T] = {&gl_mqa_env.pers_q_hdl,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgQueueClose(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgQueueClose(*API_Mqsv_QueueClose[i].queueHandle);

   m_TET_MQSV_PRINTF("\n Queue Handle    : %llu",*API_Mqsv_QueueClose[i].queueHandle);

   result = mqsv_test_result(rc,API_Mqsv_QueueClose[i].exp_output,
                             API_Mqsv_QueueClose_resultstring[i],flg);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgQueueClose(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgQueueClose(*API_Mqsv_QueueClose[i].queueHandle);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n Queue Handle    : %llu",*API_Mqsv_QueueClose[i].queueHandle);

      result = mqsv_test_result(rc,API_Mqsv_QueueClose[i].exp_output,
                                API_Mqsv_QueueClose_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Status Get Test cases  ***************** */

char *API_Mqsv_QueueStatusGet_resultstring[] = {
   [MSG_QUEUE_STATUS_BAD_HANDLE_T]     = "saMsgQueueStatusGet with invalid Message Handle",
   [MSG_QUEUE_STATUS_FINALIZED_HDL_T]  = "saMsgQueueStatusGet with finalized Message Handle",
   [MSG_QUEUE_STATUS_NULL_STATUS_T]    = "saMsgQueueStatusGet with Null status parameter",
   [MSG_QUEUE_STATUS_NULL_NAME_T]      = "saMsgQueueStatusGet with Null Queue name",
   [MSG_QUEUE_STATUS_GET_SUCCESS_T]    = "saMsgQueueStatusGet with valid parameters",
   [MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T] = "saMsgQueueStatusGet with valid parameters",
   [MSG_QUEUE_STATUS_GET_SUCCESS_Q3_T] = "saMsgQueueStatusGet with valid parameters",
   [MSG_QUEUE_STATUS_GET_SUCCESS_Q4_T] = "saMsgQueueStatusGet with valid parameters",
   [MSG_QUEUE_STATUS_GET_SUCCESS_Q5_T] = "saMsgQueueStatusGet with valid parameters",
   [MSG_QUEUE_STATUS_NOT_EXIST_Q1_T]   = "saMsgQueueStatusGet for a queue that does not exist",
   [MSG_QUEUE_STATUS_NOT_EXIST_Q2_T]   = "saMsgQueueStatusGet for a queue that does not exist",
   [MSG_QUEUE_STATUS_NOT_EXIST_Q3_T]   = "saMsgQueueStatusGet for a queue that does not exist",
   [MSG_QUEUE_STATUS_NOT_EXIST_Q4_T]   = "saMsgQueueStatusGet for a queue that does not exist",
   [MSG_QUEUE_STATUS_ERR_TRY_AGAIN_T]  = "saMsgQueueStatusGet when service is not available",
};

struct SafMsgQueueStatusGet API_Mqsv_QueueStatusGet[] = {
   [MSG_QUEUE_STATUS_BAD_HANDLE_T]     = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.pers_q,
                                          &gl_mqa_env.q_status,SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_STATUS_FINALIZED_HDL_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.q_status,SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_STATUS_NULL_STATUS_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,NULL,SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_STATUS_NULL_NAME_T]      = {&gl_mqa_env.msg_hdl1,NULL,&gl_mqa_env.q_status,SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_STATUS_GET_SUCCESS_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.q_status,SA_AIS_OK},
   [MSG_QUEUE_STATUS_GET_SUCCESS_Q2_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.q_status,SA_AIS_OK},
   [MSG_QUEUE_STATUS_GET_SUCCESS_Q3_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q2,&gl_mqa_env.q_status,SA_AIS_OK},
   [MSG_QUEUE_STATUS_GET_SUCCESS_Q4_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q2,&gl_mqa_env.q_status,SA_AIS_OK},
   [MSG_QUEUE_STATUS_GET_SUCCESS_Q5_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.zero_q,&gl_mqa_env.q_status,SA_AIS_OK},
   [MSG_QUEUE_STATUS_NOT_EXIST_Q1_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.q_status,SA_AIS_ERR_NOT_EXIST},
   [MSG_QUEUE_STATUS_NOT_EXIST_Q2_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.q_status,
                                          SA_AIS_ERR_NOT_EXIST},
   [MSG_QUEUE_STATUS_NOT_EXIST_Q3_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q2,&gl_mqa_env.q_status,SA_AIS_ERR_NOT_EXIST},
   [MSG_QUEUE_STATUS_NOT_EXIST_Q4_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q2,&gl_mqa_env.q_status,
                                          SA_AIS_ERR_NOT_EXIST},
   [MSG_QUEUE_STATUS_ERR_TRY_AGAIN_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.q_status,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgQueueStatusGet(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgQueueStatusGet(*API_Mqsv_QueueStatusGet[i].msgHandle,API_Mqsv_QueueStatusGet[i].queueName,
                            API_Mqsv_QueueStatusGet[i].queueStatus);

   if(API_Mqsv_QueueStatusGet[i].queueName)
      m_TET_MQSV_PRINTF("\n Queue Name      : %s",API_Mqsv_QueueStatusGet[i].queueName->value);
   else
      m_TET_MQSV_PRINTF("\n Queue Name      : NULL");

   result = mqsv_test_result(rc,API_Mqsv_QueueStatusGet[i].exp_output,
                             API_Mqsv_QueueStatusGet_resultstring[i],flg);

   if(rc == SA_AIS_OK)
      print_queueStatus(API_Mqsv_QueueStatusGet[i].queueStatus);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgQueueStatusGet(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgQueueStatusGet(*API_Mqsv_QueueStatusGet[i].msgHandle,
                               API_Mqsv_QueueStatusGet[i].queueName,
                               API_Mqsv_QueueStatusGet[i].queueStatus);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_QueueStatusGet[i].queueName)
            m_TET_MQSV_PRINTF("\n Queue Name      : %s",API_Mqsv_QueueStatusGet[i].queueName->value);
         else
            m_TET_MQSV_PRINTF("\n Queue Name      : NULL");
      }

      result = mqsv_test_result(rc,API_Mqsv_QueueStatusGet[i].exp_output,
                                API_Mqsv_QueueStatusGet_resultstring[i],flg);

      if(rc == SA_AIS_OK)
         print_queueStatus(API_Mqsv_QueueStatusGet[i].queueStatus);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Unlink Test cases  ***************** */

char *API_Mqsv_QueueUnlink_resultstring[] = {
   [MSG_QUEUE_UNLINK_BAD_HANDLE_T]    = "saMsgQueueUnlink with invalid Message Handle",
   [MSG_QUEUE_UNLINK_FINALIZED_HDL_T] = "saMsgQueueUnlink with finalized Message Handle",
   [MSG_QUEUE_UNLINK_NULL_NAME_T]     = "saMsgQueueUnlink with Null Queue name",
   [MSG_QUEUE_UNLINK_ERR_NOT_EXIST_T] = "saMsgQueueUnlink for a queue that does not exist",
   [MSG_QUEUE_UNLINK_SUCCESS_Q1_T]    = "saMsgQueueUnlink with valid parameters",
   [MSG_QUEUE_UNLINK_SUCCESS_Q2_T]    = "saMsgQueueUnlink with valid parameters",
   [MSG_QUEUE_UNLINK_SUCCESS_Q3_T]    = "saMsgQueueUnlink with valid parameters",
   [MSG_QUEUE_UNLINK_SUCCESS_Q4_T]    = "saMsgQueueUnlink with valid parameters",
   [MSG_QUEUE_UNLINK_SUCCESS_Q5_T]    = "saMsgQueueUnlink with valid parameters",
   [MSG_QUEUE_UNLINK_ERR_TRY_AGAIN_T] = "saMsgQueueUnlink when service is not available",
}; 

struct SafMsgQueueUnlink API_Mqsv_QueueUnlink[] = {
   [MSG_QUEUE_UNLINK_BAD_HANDLE_T]    = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.pers_q,SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_UNLINK_FINALIZED_HDL_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,SA_AIS_ERR_BAD_HANDLE},
   [MSG_QUEUE_UNLINK_NULL_NAME_T]     = {&gl_mqa_env.msg_hdl1,NULL,SA_AIS_ERR_INVALID_PARAM},
   [MSG_QUEUE_UNLINK_ERR_NOT_EXIST_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,SA_AIS_ERR_NOT_EXIST},
   [MSG_QUEUE_UNLINK_SUCCESS_Q1_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,SA_AIS_OK},
   [MSG_QUEUE_UNLINK_SUCCESS_Q2_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,SA_AIS_OK},
   [MSG_QUEUE_UNLINK_SUCCESS_Q3_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q2,SA_AIS_OK},
   [MSG_QUEUE_UNLINK_SUCCESS_Q4_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q2,SA_AIS_OK},
   [MSG_QUEUE_UNLINK_SUCCESS_Q5_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.zero_q,SA_AIS_OK},
   [MSG_QUEUE_UNLINK_ERR_TRY_AGAIN_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgQueueUnlink(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgQueueUnlink(*API_Mqsv_QueueUnlink[i].msgHandle,API_Mqsv_QueueUnlink[i].queueName);

   if(flg != TEST_CLEANUP_MODE || (flg == TEST_CLEANUP_MODE && rc != SA_AIS_OK))
   {
      if(API_Mqsv_QueueUnlink[i].queueName)
         m_TET_MQSV_PRINTF("\n Queue Name      : %s",API_Mqsv_QueueUnlink[i].queueName->value);
      else
         m_TET_MQSV_PRINTF("\n Queue Name      : NULL");
   }

   result = mqsv_test_result(rc,API_Mqsv_QueueUnlink[i].exp_output,
                             API_Mqsv_QueueUnlink_resultstring[i],flg);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgQueueUnlink(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgQueueUnlink(*API_Mqsv_QueueUnlink[i].msgHandle,API_Mqsv_QueueUnlink[i].queueName);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if((flg == TEST_CLEANUP_MODE && rc != SA_AIS_OK))
         {
            if(API_Mqsv_QueueUnlink[i].queueName)
               m_TET_MQSV_PRINTF("\n Queue Name      : %s",API_Mqsv_QueueUnlink[i].queueName->value);
            else
               m_TET_MQSV_PRINTF("\n Queue Name      : NULL");
         }    
      }

      result = mqsv_test_result(rc,API_Mqsv_QueueUnlink[i].exp_output,
                                API_Mqsv_QueueUnlink_resultstring[i],flg);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Group Create Test cases  ***************** */

char *API_Mqsv_GroupCreate_resultstring[] = {
   [MSG_GROUP_CREATE_BAD_HDL_T]        = "saMsgQueueGroupCreate with invalid Message Handle",
   [MSG_GROUP_CREATE_FINALIZED_HDL_T]  = "saMsgQueueGroupCreate with finalized Message Handle",
   [MSG_GROUP_CREATE_NULL_NAME_T]      = "saMsgQueueGroupCreate with Null Group Name",
   [MSG_GROUP_CREATE_BAD_POL_T]        = "saMsgQueueGroupCreate with Bad Policy",
   [MSG_GROUP_CREATE_LOCAL_RR_T]       = "saMsgQueueGroupCreate with Policy not supported - Local Round Robin",
   [MSG_GROUP_CREATE_LCL_BSTQ_NOT_SUPP_T] = "saMsgQueueGroupCreate with Policy not supported - Local Best Queue",
   [MSG_GROUP_CREATE_BROADCAST_T]      = "saMsgQueueGroupCreate with Policy not supported - Broadcast",
   [MSG_GROUP_CREATE_SUCCESS_T]        = "saMsgQueueGroupCreate with valid parameters",
   [MSG_GROUP_CREATE_ERR_EXIST_T]      = "saMsgQueueGroupCreate for a group that already exists",
   [MSG_GROUP_CREATE_ERR_EXIST2_T]     = "saMsgQueueGroupCreate for a group that exists with different policy",
   [MSG_GROUP_CREATE_ERR_EXIST3_T]     = "saMsgQueueGroupCreate for a group that exists with different policy",
   [MSG_GROUP_CREATE_ERR_EXIST4_T]     = "saMsgQueueGroupCreate for a group that exists with different policy",
   [MSG_GROUP_CREATE_SUCCESS_GR2_T]    = "saMsgQueueGroupCreate with valid parameters",
   [MSG_GROUP_CREATE_BROADCAST2_T]     = "saMsgQueueGroupCreate with Policy not supported - Broadcast",
   [MSG_GROUP_CREATE_ERR_TRY_AGAIN_T]  = "saMsgQueueGroupCreate when service is not available",
};

struct SafMsgGroupCreate API_Mqsv_GroupCreate[] = {
   [MSG_GROUP_CREATE_BAD_HDL_T]      = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.qgroup1,
                                        SA_MSG_QUEUE_GROUP_ROUND_ROBIN,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_CREATE_FINALIZED_HDL_T]= {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,
                                        SA_MSG_QUEUE_GROUP_ROUND_ROBIN,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_CREATE_NULL_NAME_T]    = {&gl_mqa_env.msg_hdl1,NULL,SA_MSG_QUEUE_GROUP_ROUND_ROBIN,
                                        SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_CREATE_BAD_POL_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,!SA_MSG_QUEUE_GROUP_ROUND_ROBIN,
                                        SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_CREATE_LOCAL_RR_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,
                                        SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN,SA_AIS_OK},
   [MSG_GROUP_CREATE_LCL_BSTQ_NOT_SUPP_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,
                                             SA_MSG_QUEUE_GROUP_LOCAL_BEST_QUEUE,SA_AIS_ERR_NOT_SUPPORTED},
   [MSG_GROUP_CREATE_BROADCAST_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_MSG_QUEUE_GROUP_BROADCAST,
                                        SA_AIS_OK},
   [MSG_GROUP_CREATE_SUCCESS_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_MSG_QUEUE_GROUP_ROUND_ROBIN,SA_AIS_OK},
   [MSG_GROUP_CREATE_ERR_EXIST_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_MSG_QUEUE_GROUP_ROUND_ROBIN,
                                        SA_AIS_ERR_EXIST},
   [MSG_GROUP_CREATE_ERR_EXIST2_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN,
                                        SA_AIS_ERR_EXIST},
   [MSG_GROUP_CREATE_ERR_EXIST3_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_MSG_QUEUE_GROUP_LOCAL_BEST_QUEUE,
                                        SA_AIS_ERR_NOT_SUPPORTED},
   [MSG_GROUP_CREATE_ERR_EXIST4_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_MSG_QUEUE_GROUP_BROADCAST,
                                        SA_AIS_ERR_EXIST},
   [MSG_GROUP_CREATE_SUCCESS_GR2_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup2,SA_MSG_QUEUE_GROUP_ROUND_ROBIN,SA_AIS_OK},
   [MSG_GROUP_CREATE_BROADCAST2_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup2,SA_MSG_QUEUE_GROUP_BROADCAST,
                                        SA_AIS_OK},
   [MSG_GROUP_CREATE_ERR_TRY_AGAIN_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_MSG_QUEUE_GROUP_ROUND_ROBIN,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgGroupCreate(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgQueueGroupCreate(*API_Mqsv_GroupCreate[i].msgHandle,API_Mqsv_GroupCreate[i].queueGroupName,
                              API_Mqsv_GroupCreate[i].queueGroupPolicy);

   if(API_Mqsv_GroupCreate[i].queueGroupName)
      m_TET_MQSV_PRINTF("\n Group Name      : %s",API_Mqsv_GroupCreate[i].queueGroupName->value);
   else
      m_TET_MQSV_PRINTF("\n Group Name      : NULL");

   if(API_Mqsv_GroupCreate[i].queueGroupPolicy > 0 && API_Mqsv_GroupCreate[i].queueGroupPolicy < 4)
      m_TET_MQSV_PRINTF("\n Group Policy    : %s",
        saMsgQueueGroupPolicy_string[API_Mqsv_GroupCreate[i].queueGroupPolicy]);
   else
      m_TET_MQSV_PRINTF("\n Group Policy    : %d",API_Mqsv_GroupCreate[i].queueGroupPolicy);

   result = mqsv_test_result(rc,API_Mqsv_GroupCreate[i].exp_output,
                             API_Mqsv_GroupCreate_resultstring[i],flg);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgGroupCreate(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgQueueGroupCreate(*API_Mqsv_GroupCreate[i].msgHandle,
                                 API_Mqsv_GroupCreate[i].queueGroupName,
                                 API_Mqsv_GroupCreate[i].queueGroupPolicy);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_GroupCreate[i].queueGroupName)
            m_TET_MQSV_PRINTF("\n Group Name      : %s",API_Mqsv_GroupCreate[i].queueGroupName->value);
         else
            m_TET_MQSV_PRINTF("\n Group Name      : NULL");

         if(API_Mqsv_GroupCreate[i].queueGroupPolicy > 0 && API_Mqsv_GroupCreate[i].queueGroupPolicy < 4)
            m_TET_MQSV_PRINTF("\n Group Policy    : %s",
              saMsgQueueGroupPolicy_string[API_Mqsv_GroupCreate[i].queueGroupPolicy]);
         else
            m_TET_MQSV_PRINTF("\n Group Policy    : %d",API_Mqsv_GroupCreate[i].queueGroupPolicy);
      }

      result = mqsv_test_result(rc,API_Mqsv_GroupCreate[i].exp_output,
                                API_Mqsv_GroupCreate_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Group Insert Test cases  ***************** */

char *API_Mqsv_GroupInsert_resultstring[] = {
   [MSG_GROUP_INSERT_BAD_HDL_T]      = "saMsgQueueGroupInsert with invalid Message Handle",
   [MSG_GROUP_INSERT_FINALIZED_HDL_T]= "saMsgQueueGroupInsert with finalized Message Handle",
   [MSG_GROUP_INSERT_NULL_GR_NAME_T] = "saMsgQueueGroupInsert with Null Group name",
   [MSG_GROUP_INSERT_NULL_Q_NAME_T]  = "saMsgQueueGroupInsert with Null Queue name",
   [MSG_GROUP_INSERT_BAD_GR_T]       = "saMsgQueueGroupInsert with group that not exists",
   [MSG_GROUP_INSERT_BAD_QUEUE_T]    = "saMsgQueueGroupInsert with queue that not exists",
   [MSG_GROUP_INSERT_SUCCESS_T]      = "saMsgQueueGroupInsert with valid parameters",
   [MSG_GROUP_INSERT_SUCCESS_Q2_T]   = "saMsgQueueGroupInsert with valid parameters",
   [MSG_GROUP_INSERT_SUCCESS_Q3_T]   = "saMsgQueueGroupInsert with valid parameters",
   [MSG_GROUP_INSERT_SUCCESS_ZEROQ_T]= "saMsgQueueGroupInsert with valid parameters",
   [MSG_GROUP_INSERT_ERR_EXIST_T]    = "Insert a queue into a group that already contains the queue",
   [MSG_GROUP_INSERT_Q1_GROUP2_T]    = "Insert the same queue into another group",
   [MSG_GROUP_INSERT_Q2_GROUP2_T]    = "saMsgQueueGroupInsert with valid parameters",
   [MSG_GROUP_INSERT_ERR_TRY_AGAIN_T] = "saMsgQueueGroupInsert when service is not available",
};

struct SafMsgGroupInsert API_Mqsv_GroupInsert[] = {
   [MSG_GROUP_INSERT_BAD_HDL_T]      = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.qgroup1,
                                        &gl_mqa_env.pers_q,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_INSERT_FINALIZED_HDL_T]= {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_INSERT_NULL_GR_NAME_T] = {&gl_mqa_env.msg_hdl1,NULL,&gl_mqa_env.pers_q,SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_INSERT_NULL_Q_NAME_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,NULL,SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_INSERT_BAD_GR_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_ERR_NOT_EXIST},
   [MSG_GROUP_INSERT_BAD_QUEUE_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_ERR_NOT_EXIST},
   [MSG_GROUP_INSERT_SUCCESS_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_OK},
   [MSG_GROUP_INSERT_SUCCESS_Q2_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.non_pers_q,SA_AIS_OK},
   [MSG_GROUP_INSERT_SUCCESS_Q3_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q2,SA_AIS_OK},
   [MSG_GROUP_INSERT_SUCCESS_ZEROQ_T]= {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.zero_q,SA_AIS_OK},
   [MSG_GROUP_INSERT_ERR_EXIST_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_ERR_EXIST},
   [MSG_GROUP_INSERT_Q1_GROUP2_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup2,&gl_mqa_env.pers_q,SA_AIS_OK},
   [MSG_GROUP_INSERT_Q2_GROUP2_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup2,&gl_mqa_env.non_pers_q,SA_AIS_OK},
   [MSG_GROUP_INSERT_ERR_TRY_AGAIN_T]= {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgGroupInsert(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgQueueGroupInsert(*API_Mqsv_GroupInsert[i].msgHandle,API_Mqsv_GroupInsert[i].queueGroupName,
                              API_Mqsv_GroupInsert[i].queueName);

   if(API_Mqsv_GroupInsert[i].queueName)
      m_TET_MQSV_PRINTF("\n Queue Name      : %s\n",API_Mqsv_GroupInsert[i].queueName->value);
   else
      m_TET_MQSV_PRINTF("\n Queue Name      : NULL\n");

   if(API_Mqsv_GroupInsert[i].queueGroupName)
      m_TET_MQSV_PRINTF(" Group Name      : %s",API_Mqsv_GroupInsert[i].queueGroupName->value);
   else
      m_TET_MQSV_PRINTF(" Group Name      : NULL");

   result = mqsv_test_result(rc,API_Mqsv_GroupInsert[i].exp_output,
                             API_Mqsv_GroupInsert_resultstring[i],flg);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgGroupInsert(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgQueueGroupInsert(*API_Mqsv_GroupInsert[i].msgHandle,
                                 API_Mqsv_GroupInsert[i].queueGroupName,
                                 API_Mqsv_GroupInsert[i].queueName);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_GroupInsert[i].queueName)
            m_TET_MQSV_PRINTF("\n Queue Name      : %s\n",API_Mqsv_GroupInsert[i].queueName->value);
         else
            m_TET_MQSV_PRINTF("\n Queue Name      : NULL\n");

         if(API_Mqsv_GroupInsert[i].queueGroupName)
            m_TET_MQSV_PRINTF(" Group Name      : %s",API_Mqsv_GroupInsert[i].queueGroupName->value);
         else
            m_TET_MQSV_PRINTF(" Group Name      : NULL");
      }

      result = mqsv_test_result(rc,API_Mqsv_GroupInsert[i].exp_output,
                                API_Mqsv_GroupInsert_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Group Remove Test cases  ***************** */

char *API_Mqsv_GroupRemove_resultstring[] = {
   [MSG_GROUP_REMOVE_BAD_HDL_T]      = "saMsgQueueGroupRemove with invalid Message Handle",
   [MSG_GROUP_REMOVE_FINALIZED_HDL_T]= "saMsgQueueGroupRemove with finalized Message Handle",
   [MSG_GROUP_REMOVE_NULL_GR_NAME_T] = "saMsgQueueGroupRemove with Null Group name",
   [MSG_GROUP_REMOVE_NULL_Q_NAME_T]  = "saMsgQueueGroupRemove with Null Queue name",
   [MSG_GROUP_REMOVE_BAD_GROUP_T]    = "saMsgQueueGroupRemove with Bad Group",
   [MSG_GROUP_REMOVE_BAD_QUEUE_T]    = "saMsgQueueGroupRemove with Bad Queue",
   [MSG_GROUP_REMOVE_SUCCESS_T]      = "saMsgGroupQueueRemove with valid parameters",
   [MSG_GROUP_REMOVE_SUCCESS_Q2_T]   = "saMsgGroupQueueRemove with valid parameters",
   [MSG_GROUP_REMOVE_SUCCESS_Q3_T]   = "saMsgGroupQueueRemove with valid parameters",
   [MSG_GROUP_REMOVE_NOT_EXIST_T]    = "Remove a queue from a group that does not contain that queue",
   [MSG_GROUP_REMOVE_ERR_TRY_AGAIN_T] = "saMsgGroupQueueRemove when service is not available",
};

struct SafMsgGroupRemove API_Mqsv_GroupRemove[] = {
   [MSG_GROUP_REMOVE_BAD_HDL_T]      = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.qgroup1,
                                        &gl_mqa_env.pers_q,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_REMOVE_FINALIZED_HDL_T]= {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_REMOVE_NULL_GR_NAME_T] = {&gl_mqa_env.msg_hdl1,NULL,&gl_mqa_env.pers_q,SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_REMOVE_NULL_Q_NAME_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,NULL,SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_REMOVE_BAD_GROUP_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_ERR_NOT_EXIST},
   [MSG_GROUP_REMOVE_BAD_QUEUE_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_ERR_NOT_EXIST},
   [MSG_GROUP_REMOVE_SUCCESS_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_OK},
   [MSG_GROUP_REMOVE_SUCCESS_Q2_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.non_pers_q,SA_AIS_OK},
   [MSG_GROUP_REMOVE_SUCCESS_Q3_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q2,SA_AIS_OK},
   [MSG_GROUP_REMOVE_NOT_EXIST_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_ERR_NOT_EXIST},
   [MSG_GROUP_REMOVE_ERR_TRY_AGAIN_T]= {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.pers_q,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgGroupRemove(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgQueueGroupRemove(*API_Mqsv_GroupRemove[i].msgHandle,API_Mqsv_GroupRemove[i].queueGroupName,
                              API_Mqsv_GroupRemove[i].queueName);

   if(API_Mqsv_GroupRemove[i].queueName)
      m_TET_MQSV_PRINTF("\n Queue Name      : %s\n",API_Mqsv_GroupRemove[i].queueName->value);
   else
      m_TET_MQSV_PRINTF("\n Queue Name      : NULL\n");

   if(API_Mqsv_GroupRemove[i].queueGroupName)
      m_TET_MQSV_PRINTF(" Group Name      : %s",API_Mqsv_GroupRemove[i].queueGroupName->value);
   else
      m_TET_MQSV_PRINTF(" Group Name      : NULL");

   result = mqsv_test_result(rc,API_Mqsv_GroupRemove[i].exp_output,
                             API_Mqsv_GroupRemove_resultstring[i],flg);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgGroupRemove(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgQueueGroupRemove(*API_Mqsv_GroupRemove[i].msgHandle,
                                 API_Mqsv_GroupRemove[i].queueGroupName,
                                 API_Mqsv_GroupRemove[i].queueName);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_GroupRemove[i].queueName)
            m_TET_MQSV_PRINTF("\n Queue Name      : %s\n",API_Mqsv_GroupRemove[i].queueName->value);
         else
            m_TET_MQSV_PRINTF("\n Queue Name      : NULL\n");

         if(API_Mqsv_GroupRemove[i].queueGroupName)
            m_TET_MQSV_PRINTF(" Group Name      : %s",API_Mqsv_GroupRemove[i].queueGroupName->value);
         else
            m_TET_MQSV_PRINTF(" Group Name      : NULL");
      }

      result = mqsv_test_result(rc,API_Mqsv_GroupRemove[i].exp_output,
                                API_Mqsv_GroupRemove_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Group Delete Test cases  ***************** */

char *API_Mqsv_GroupDelete_resultstring[] = {
   [MSG_GROUP_DELETE_BAD_HDL_T]     = "saMsgQueueGroupDelete with invalid Message Handle", 
   [MSG_GROUP_DELETE_FINALIZED_HDL_T] = "saMsgQueueGroupDelete with finalized Message Handle", 
   [MSG_GROUP_DELETE_NULL_NAME_T]   = "saMsgQueueGroupDelete with Null Group name",
   [MSG_GROUP_DELETE_BAD_GROUP_T]   = "saMsgQueueGroupDelete with Bad Group name",
   [MSG_GROUP_DELETE_WITH_MEM_T]    = "Delete a Group with members",
   [MSG_GROUP_DELETE_WO_MEM_T]      = "Delete a Group without members",
   [MSG_GROUP_DELETE_SUCCESS_T]     = "saMsgQueueGroupDelete with valid parameters",
   [MSG_GROUP_DELETE_SUCCESS_GR2_T] = "saMsgQueueGroupDelete with valid parameters",
   [MSG_GROUP_DELETE_ERR_TRY_AGAIN_T] = "saMsgQueueGroupDelete when service is not available",
};

struct SafMsgGroupDelete API_Mqsv_GroupDelete[] = {
   [MSG_GROUP_DELETE_BAD_HDL_T]     = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.qgroup1,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_DELETE_FINALIZED_HDL_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_DELETE_NULL_NAME_T]   = {&gl_mqa_env.msg_hdl1,NULL,SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_DELETE_BAD_GROUP_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_AIS_ERR_NOT_EXIST},
   [MSG_GROUP_DELETE_WITH_MEM_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_AIS_OK},
   [MSG_GROUP_DELETE_WO_MEM_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup2,SA_AIS_OK},
   [MSG_GROUP_DELETE_SUCCESS_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_AIS_OK},
   [MSG_GROUP_DELETE_SUCCESS_GR2_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup2,SA_AIS_OK},
   [MSG_GROUP_DELETE_ERR_TRY_AGAIN_T]= {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgGroupDelete(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgQueueGroupDelete(*API_Mqsv_GroupDelete[i].msgHandle,API_Mqsv_GroupDelete[i].queueGroupName);

   if(flg != TEST_CLEANUP_MODE || (flg == TEST_CLEANUP_MODE && rc != SA_AIS_OK))
   {
      if(API_Mqsv_GroupDelete[i].queueGroupName)
         m_TET_MQSV_PRINTF(" Group Name      : %s",API_Mqsv_GroupDelete[i].queueGroupName->value);
      else
         m_TET_MQSV_PRINTF(" Group Name      : NULL");
   }

   result = mqsv_test_result(rc,API_Mqsv_GroupDelete[i].exp_output,
                           API_Mqsv_GroupDelete_resultstring[i],flg);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgGroupDelete(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgQueueGroupDelete(*API_Mqsv_GroupDelete[i].msgHandle,
                                 API_Mqsv_GroupDelete[i].queueGroupName);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(flg == TEST_CLEANUP_MODE && rc != SA_AIS_OK)
         {
            if(API_Mqsv_GroupDelete[i].queueGroupName)
               m_TET_MQSV_PRINTF(" Group Name      : %s",API_Mqsv_GroupDelete[i].queueGroupName->value);
            else
               m_TET_MQSV_PRINTF(" Group Name      : NULL");
         }
      }

      result = mqsv_test_result(rc,API_Mqsv_GroupDelete[i].exp_output,
                              API_Mqsv_GroupDelete_resultstring[i],flg);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Group Track Test cases  ***************** */

char *API_Mqsv_GroupTrack_resultstring[] = {
   [MSG_GROUP_TRACK_BAD_HDL_T]            = "saMsgQueueGroupTrack with invalid Message Handle",
   [MSG_GROUP_TRACK_FINALIZED_HDL_T]      = "saMsgQueueGroupTrack with finalized Message Handle",
   [MSG_GROUP_TRACK_NULL_NAME_T]          = "saMsgQueueGroupTrack with Null Group name",
   [MSG_GROUP_TRACK_INV_PARAM_T]          = "saMsgQueueGroupTrack with Bad buffer",
   [MSG_GROUP_TRACK_BAD_GROUP_T]          = "saMsgQueueGroupTrack with Group name that does not exist",
   [MSG_GROUP_TRACK_BAD_FLAGS_T]          = "saMsgQueueGroupTrack with Bad Track Flags",
   [MSG_GROUP_TRACK_WRONG_FLGS_T]         = "saMsgQueueGroupTrack with Wrong Track Flags",
   [MSG_GROUP_TRACK_NULL_BUF_T]           = "saMsgQueueGroupTrack with Null Notification Buffer",
   [MSG_GROUP_TRACK_CH_BAD_BUF_T]         = "saMsgQueueGroupTrack with Track changes Bad Buffer",
   [MSG_GROUP_TRACK_CHONLY_BAD_BUF_T]     = "saMsgQueueGroupTrack with Track changes only Bad Buffer",
   [MSG_GROUP_TRACK_CUR_CH_BAD_BUF_T]     = "saMsgQueueGroupTrack with Track CURRENT | CHANGES Bad Buffer",
   [MSG_GROUP_TRACK_CUR_CHONLY_BAD_BUF_T] = "saMsgQueueGroupTrack with Track CURRENT | CHANGES_ONLY Bad Buffer",
   [MSG_GROUP_TRACK_CURR_NULBUF_ERINIT_T] = "saMsgQueueGroupTrack with Track Current with NULL buffer ERR_INIT case",
   [MSG_GROUP_TRACK_CURR_ERR_INIT_T]      = "saMsgQueueGroupTrack with Track Current ERR_INIT case",
   [MSG_GROUP_TRACK_CHGS_ERR_INIT_T]      = "saMsgQueueGroupTrack with Track Changes ERR_INIT case",
   [MSG_GROUP_TRACK_CHONLY_ERR_INIT_T]    = "saMsgQueueGroupTrack with Track Changes Only ERR_INIT case", 
   [MSG_GROUP_TRACK_CURRENT_T]            = "Track with non-NULL Buffer NULL Notification - CURRENT",
   [MSG_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T] = "Track with non-NULL Buffer non-NULL Notification - CURRENT",
   [MSG_GROUP_TRACK_CUR_NO_SPACE_T]       = "saMsgQueueGroupTrack with Track Current Insufficient Buffer",
   [MSG_GROUP_TRACK_CHANGES_T]            = "saMsgQueueGroupTrack with valid parameters - Track Changes",
   [MSG_GROUP_TRACK_CHANGES_GR2_T]        = "saMsgQueueGroupTrack with valid parameters - Track Changes",
   [MSG_GROUP_TRACK_CHGS_ONLY_T]          = "saMsgQueueGroupTrack with valid parameters - Track Changes Only",
   [MSG_GROUP_TRACK_CUR_CH_T]             = "saMsgQueueGroupTrack with valid params - Current and Changes", 
   [MSG_GROUP_TRACK_CUR_CHLY_T]           = "saMsgQueueGroupTrack with valid params - Current and Changes only",
   [MSG_GROUP_TRACK_CUR_CH_NUL_BUF_T]     = "saMsgQueueGroupTrack with valid params - Current and Changes Null Buffer", 
   [MSG_GROUP_TRACK_CUR_CHLY_NUL_BUF_T]   = "saMsgQueueGroupTrack with valid params - Current and Changes Only Null Buffer", 
   [MSG_GROUP_TRACK_CUR_CHLY_NUL_BUF_GR2_T] = "saMsgQueueGroupTrack with valid params - Current and Changes Only Null Buffer", 
   [MSG_GROUP_TRACK_ERR_TRY_AGAIN_T]      = "saMsgQueueGroupTrack when service is not available",
};

struct SafMsgGroupTrack API_Mqsv_GroupTrack[] = {
   [MSG_GROUP_TRACK_BAD_HDL_T]            = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.qgroup1,
                                             SA_TRACK_CURRENT,&gl_mqa_env.buffer_null_notif,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_TRACK_FINALIZED_HDL_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_TRACK_NULL_NAME_T]          = {&gl_mqa_env.msg_hdl1,NULL,SA_TRACK_CURRENT,&gl_mqa_env.buffer_null_notif,
                                             SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_TRACK_INV_PARAM_T]          = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT,
                                             &gl_mqa_env.inv_params.inv_notif_buf,SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_TRACK_BAD_GROUP_T]          = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_ERR_NOT_EXIST},
   [MSG_GROUP_TRACK_BAD_FLAGS_T]          = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,!SA_TRACK_CURRENT,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_ERR_BAD_FLAGS},
   [MSG_GROUP_TRACK_WRONG_FLGS_T]         = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_ERR_BAD_FLAGS},
   [MSG_GROUP_TRACK_NULL_BUF_T]           = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT,NULL,SA_AIS_OK},
   [MSG_GROUP_TRACK_CH_BAD_BUF_T]         = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CHANGES,
                                             &gl_mqa_env.inv_params.inv_notif_buf,SA_AIS_OK},
   [MSG_GROUP_TRACK_CHONLY_BAD_BUF_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CHANGES_ONLY,
                                             &gl_mqa_env.inv_params.inv_notif_buf,SA_AIS_OK},
   [MSG_GROUP_TRACK_CUR_CH_BAD_BUF_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT | SA_TRACK_CHANGES,
                                             &gl_mqa_env.inv_params.inv_notif_buf,SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_TRACK_CUR_CHONLY_BAD_BUF_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT | SA_TRACK_CHANGES_ONLY,
                                             &gl_mqa_env.inv_params.inv_notif_buf,SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_TRACK_CURR_NULBUF_ERINIT_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT,
                                             NULL,SA_AIS_ERR_INIT},
   [MSG_GROUP_TRACK_CURR_ERR_INIT_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_ERR_INIT},
   [MSG_GROUP_TRACK_CHGS_ERR_INIT_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CHANGES,
                                             NULL,SA_AIS_ERR_INIT},
   [MSG_GROUP_TRACK_CHONLY_ERR_INIT_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CHANGES_ONLY,NULL,
                                             SA_AIS_ERR_INIT},
   [MSG_GROUP_TRACK_CURRENT_T]            = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_OK},
   [MSG_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT,
                                                 &gl_mqa_env.buffer_non_null_notif,SA_AIS_OK},
   [MSG_GROUP_TRACK_CUR_NO_SPACE_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT,
                                             &gl_mqa_env.no_space_notif_buf,SA_AIS_ERR_NO_SPACE},
   [MSG_GROUP_TRACK_CHANGES_T]            = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CHANGES,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_OK},
   [MSG_GROUP_TRACK_CHANGES_GR2_T]        = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup2,SA_TRACK_CHANGES,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_OK},
   [MSG_GROUP_TRACK_CHGS_ONLY_T]          = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CHANGES_ONLY,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_OK},
   [MSG_GROUP_TRACK_CUR_CH_T]             = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT | SA_TRACK_CHANGES,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_OK},
   [MSG_GROUP_TRACK_CUR_CHLY_T]           = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT | SA_TRACK_CHANGES_ONLY,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_OK},
   [MSG_GROUP_TRACK_CUR_CH_NUL_BUF_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT | SA_TRACK_CHANGES,
                                             NULL,SA_AIS_OK},
   [MSG_GROUP_TRACK_CUR_CHLY_NUL_BUF_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT | SA_TRACK_CHANGES_ONLY,
                                             NULL,SA_AIS_OK},
   [MSG_GROUP_TRACK_CUR_CHLY_NUL_BUF_GR2_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup2,SA_TRACK_CURRENT | SA_TRACK_CHANGES_ONLY,
                                             NULL,SA_AIS_OK},
   [MSG_GROUP_TRACK_ERR_TRY_AGAIN_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_TRACK_CURRENT,
                                             &gl_mqa_env.buffer_null_notif,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgGroupTrack(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaUint32T numOfItems = 0;
   SaMsgQueueGroupNotificationT *notification = NULL;
   int result;

   if(API_Mqsv_GroupTrack[i].buffer)
   {
      numOfItems = API_Mqsv_GroupTrack[i].buffer->numberOfItems;
      notification = API_Mqsv_GroupTrack[i].buffer->notification;
   }

   rc = saMsgQueueGroupTrack(*API_Mqsv_GroupTrack[i].msgHandle,API_Mqsv_GroupTrack[i].queueGroupName,
                             API_Mqsv_GroupTrack[i].trackFlags,API_Mqsv_GroupTrack[i].buffer);

   if(API_Mqsv_GroupTrack[i].queueGroupName)
      m_TET_MQSV_PRINTF("\n Tracked Group   : %s\n",API_Mqsv_GroupTrack[i].queueGroupName->value);
   else
      m_TET_MQSV_PRINTF("\n Group Name      : NULL\n");

   m_TET_MQSV_PRINTF(" Track Flag      : %s",saMsgGroupTrackFlags_string[API_Mqsv_GroupTrack[i].trackFlags]);

   if((API_Mqsv_GroupTrack[i].trackFlags & SA_TRACK_CURRENT) && (rc == SA_AIS_OK))
   {
      if(API_Mqsv_GroupTrack[i].buffer)
      {
         m_TET_MQSV_PRINTF("\n Notification Buffer \n");
         m_TET_MQSV_PRINTF(" No of Items     : %u\n",numOfItems); 
         if(notification)
            m_TET_MQSV_PRINTF(" Notification    : %p",notification);
         else
            m_TET_MQSV_PRINTF(" Notification    : NULL");
      }
      else
         m_TET_MQSV_PRINTF("\n Notif Buffer    : NULL");
   }

   result = mqsv_test_result(rc,API_Mqsv_GroupTrack[i].exp_output,
                             API_Mqsv_GroupTrack_resultstring[i],flg);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgGroupTrack(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaUint32T numOfItems = 0;
   SaMsgQueueGroupNotificationT *notification = NULL;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      if(API_Mqsv_GroupTrack[i].buffer)
      {
         numOfItems = API_Mqsv_GroupTrack[i].buffer->numberOfItems;
         notification = API_Mqsv_GroupTrack[i].buffer->notification;
      }

      rc = saMsgQueueGroupTrack(*API_Mqsv_GroupTrack[i].msgHandle,API_Mqsv_GroupTrack[i].queueGroupName,
                                API_Mqsv_GroupTrack[i].trackFlags,API_Mqsv_GroupTrack[i].buffer);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_GroupTrack[i].queueGroupName)
            m_TET_MQSV_PRINTF("\n Tracked Group   : %s\n",API_Mqsv_GroupTrack[i].queueGroupName->value);
         else
            m_TET_MQSV_PRINTF("\n Group Name      : NULL\n");

         m_TET_MQSV_PRINTF(" Track Flag      : %s",saMsgGroupTrackFlags_string[API_Mqsv_GroupTrack[i].trackFlags]);

         if((API_Mqsv_GroupTrack[i].trackFlags & SA_TRACK_CURRENT) && (rc == SA_AIS_OK))
         {
            if(API_Mqsv_GroupTrack[i].buffer)
            {
               m_TET_MQSV_PRINTF("\n Notification Buffer \n");
               m_TET_MQSV_PRINTF(" No of Items     : %u\n",numOfItems); 
               if(notification)
                  m_TET_MQSV_PRINTF(" Notification    : %p",notification);
               else
                  m_TET_MQSV_PRINTF(" Notification    : NULL");
            }
            else
               m_TET_MQSV_PRINTF("\n Notif Buffer    : NULL");
         }
      }

      result = mqsv_test_result(rc,API_Mqsv_GroupTrack[i].exp_output,
                                API_Mqsv_GroupTrack_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Queue Group Track Stop Test cases  ***************** */

char *API_Mqsv_GroupTrackStop_resultstring[] = {
   [MSG_GROUP_TRACK_STOP_BAD_HDL_T]   = "saMsgQueueGroupTrackStop with invalid Message Handle",
   [MSG_GROUP_TRACK_STOP_FINALIZED_HDL_T] = "saMsgQueueGroupTrackStop with finalized Message Handle",
   [MSG_GROUP_TRACK_STOP_NULL_NAME_T] = "saMsgQueueGroupTrackStop with Null Group name",
   [MSG_GROUP_TRACK_STOP_BAD_NAME_T]  = "saMsgQueueGroupTrackStop with Group that does not exist",
   [MSG_GROUP_TRACK_STOP_SUCCESS_T]   = "saMsgQueueGroupTrackStop with valid parameters",
   [MSG_GROUP_TRACK_STOP_SUCCESS_GR2_T] = "saMsgQueueGroupTrackStop with valid parameters",
   [MSG_GROUP_TRACK_STOP_UNTRACKED_T] = "saMsgQueueGroupTrackStop an untracked group", 
   [MSG_GROUP_TRACK_STOP_UNTRACKED2_T] = "saMsgQueueGroupTrackStop an untracked group", 
   [MSG_GROUP_TRACK_STOP_ERR_TRY_AGAIN_T] = "saMsgQueueGroupTrackStop when service is not available",
};

struct SafMsgGroupTrackStop API_Mqsv_GroupTrackStop[] = {
   [MSG_GROUP_TRACK_STOP_BAD_HDL_T]   = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.qgroup1,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_TRACK_STOP_FINALIZED_HDL_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_AIS_ERR_BAD_HANDLE},
   [MSG_GROUP_TRACK_STOP_NULL_NAME_T] = {&gl_mqa_env.msg_hdl1,NULL,SA_AIS_ERR_INVALID_PARAM},
   [MSG_GROUP_TRACK_STOP_BAD_NAME_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_AIS_ERR_NOT_EXIST},
   [MSG_GROUP_TRACK_STOP_SUCCESS_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_AIS_OK}, 
   [MSG_GROUP_TRACK_STOP_SUCCESS_GR2_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup2,SA_AIS_OK}, 
   [MSG_GROUP_TRACK_STOP_UNTRACKED_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_AIS_ERR_NOT_EXIST}, 
   [MSG_GROUP_TRACK_STOP_UNTRACKED2_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup2,SA_AIS_ERR_NOT_EXIST}, 
   [MSG_GROUP_TRACK_STOP_ERR_TRY_AGAIN_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,SA_AIS_ERR_TRY_AGAIN}, 
};

int tet_test_msgGroupTrackStop(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgQueueGroupTrackStop(*API_Mqsv_GroupTrackStop[i].msgHandle,API_Mqsv_GroupTrackStop[i].queueGroupName);

   if(flg != TEST_CLEANUP_MODE || (flg == TEST_CLEANUP_MODE && rc != SA_AIS_OK))
   {
      if(API_Mqsv_GroupTrackStop[i].queueGroupName)
         m_TET_MQSV_PRINTF(" Group Name      : %s",API_Mqsv_GroupTrackStop[i].queueGroupName->value);
      else
         m_TET_MQSV_PRINTF(" Group Name      : NULL");
   }

   result = mqsv_test_result(rc,API_Mqsv_GroupTrackStop[i].exp_output,
                             API_Mqsv_GroupTrackStop_resultstring[i],flg);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgGroupTrackStop(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgQueueGroupTrackStop(*API_Mqsv_GroupTrackStop[i].msgHandle,API_Mqsv_GroupTrackStop[i].queueGroupName);

      if(flg != TEST_CLEANUP_MODE || (flg == TEST_CLEANUP_MODE && rc != SA_AIS_OK))
      {
         if(API_Mqsv_GroupTrackStop[i].queueGroupName)
            m_TET_MQSV_PRINTF(" Group Name      : %s",API_Mqsv_GroupTrackStop[i].queueGroupName->value);
         else
            m_TET_MQSV_PRINTF(" Group Name      : NULL");
      }

      result = mqsv_test_result(rc,API_Mqsv_GroupTrackStop[i].exp_output,
                                API_Mqsv_GroupTrackStop_resultstring[i],flg);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Send Test cases  ***************** */

char *API_Mqsv_MessageSend_resultstring[] = {
   [MSG_MESSAGE_SEND_BAD_HDL_T]            = "saMsgMessageSend with invalid Message Handle",
   [MSG_MESSAGE_SEND_FINALIZED_HDL_T]      = "saMsgMessageSend with finalized Message Handle",
   [MSG_MESSAGE_SEND_NULL_NAME_T]          = "saMsgMessageSend with Null destination",
   [MSG_MESSAGE_SEND_NULL_MSG_T]           = "saMsgMessageSend with Null message",
   [MSG_MESSAGE_SEND_ERR_TIMEOUT_T]        = "saMsgMessageSend - ERR_TIMEOUT case",
   [MSG_MESSAGE_SEND_NOT_EXIST_T]          = "saMsgMessageSend to a queue that does not exist",
   [MSG_MESSAGE_SEND_BAD_GROUP_T]          = "saMsgMessageSend to a queue group that does not exist",

   [MSG_MESSAGE_SEND_SUCCESS_NAME1_T]      = "saMsgMessageSend with valid parameters - Persistent",
   [MSG_MESSAGE_SEND_SUCCESS_NAME2_T]      = "saMsgMessageSend with valid parameters - Non Persistent",
   [MSG_MESSAGE_SEND_SUCCESS_NAME3_T]      = "saMsgMessageSend with valid parameters",
   [MSG_MESSAGE_SEND_SUCCESS_NAME4_T]      = "saMsgMessageSend with valid parameters",
   [MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME1_T] = "saMsgMessageSend with valid parameters",
   [MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME2_T] = "saMsgMessageSend with valid parameters",
   [MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME3_T] = "saMsgMessageSend with valid parameters",
   [MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME4_T] = "saMsgMessageSend with valid parameters",

   [MSG_MESSAGE_SEND_SUCCESS_MSG2_T]       = "Send a message to a queue",
   [MSG_MESSAGE_SEND_SUCCESS_Q2_MSG2_T]    = "Send a message to a queue",
   [MSG_MESSAGE_SEND_TO_ZERO_QUEUE_T]      = "Message Send to zero size queue",
   [MSG_MESSAGE_SEND_QUEUE_FULL_T]         = "saMsgMessageSend - QUEUE FULL case",
   [MSG_MESSAGE_SEND_BIG_MSG_T]            = "saMsgMessageSend with big message",
   [MSG_MESSAGE_SEND_ERR_NOT_EXIST_T]      = "Message Send to an unavailable queue",
   [MSG_MESSAGE_SEND_ERR_NOT_EXIST2_T]     = "Message Send to an unavailable queue",
   [MSG_MESSAGE_SEND_WITH_BAD_PR_T]        = "Message Send with Bad priority value",
   [MSG_MESSAGE_SEND_UNAVAILABLE_T]        = "Message Send to an empty queue group",
   [MSG_MESSAGE_SEND_GR_SUCCESS_T]         = "Message Send to a group",
   [MSG_MESSAGE_SEND_GR_SUCCESS_MSG2_T]    = "Message Send to a group",
   [MSG_MESSAGE_SEND_GR_QUEUE_FULL_T]      = "Message Send to a group - QUEUE FULL case",
   [MSG_MESSAGE_SEND_ZERO_SIZE_MSG_T]      = "Send a message with zero message size",
   [MSG_MESSAGE_SEND_MULTI_CAST_GR_FULL_T] = "Message Send to a group - QUEUE FULL case",
   [MSG_MESSAGE_SEND_ERR_TRY_AGAIN_T]      = "saMsgMessageSend when service is not available",
};

struct SafMsgMessageSend API_Mqsv_MessageSend[] = {
   [MSG_MESSAGE_SEND_BAD_HDL_T]            = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.pers_q,
                                              &gl_mqa_env.send_msg,APP_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_SEND_FINALIZED_HDL_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,
                                              &gl_mqa_env.send_msg,APP_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_SEND_NULL_NAME_T]          = {&gl_mqa_env.msg_hdl1,NULL,&gl_mqa_env.send_msg,APP_TIMEOUT,
                                              SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_NULL_MSG_T]           = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,NULL,APP_TIMEOUT,
                                              SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_ERR_TIMEOUT_T]        = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,0,
                                              SA_AIS_ERR_TIMEOUT},
   [MSG_MESSAGE_SEND_NOT_EXIST_T]          = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_SEND_BAD_GROUP_T]          = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_ERR_NOT_EXIST},

   [MSG_MESSAGE_SEND_SUCCESS_NAME1_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_SUCCESS_NAME2_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_SUCCESS_NAME3_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q2,
                                              &gl_mqa_env.send_msg_null_sndr_name,APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_SUCCESS_NAME4_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q2,
                                              &gl_mqa_env.send_msg_null_sndr_name,APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME1_T] = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME2_T] = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.non_pers_q,
                                              &gl_mqa_env.send_msg_null_sndr_name,APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME3_T] = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.pers_q2,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_SUCCESS_HDL2_NAME4_T] = {&gl_mqa_env.msg_hdl2,&gl_mqa_env.non_pers_q2,
                                              &gl_mqa_env.send_msg_null_sndr_name,APP_TIMEOUT,SA_AIS_OK},

   [MSG_MESSAGE_SEND_SUCCESS_MSG2_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,
                                              &gl_mqa_env.send_msg_null_sndr_name,APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_SUCCESS_Q2_MSG2_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,
                                              &gl_mqa_env.send_msg_null_sndr_name,APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_TO_ZERO_QUEUE_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.zero_q,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_ERR_QUEUE_FULL},
   [MSG_MESSAGE_SEND_QUEUE_FULL_T]         = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_ERR_QUEUE_FULL},
   [MSG_MESSAGE_SEND_BIG_MSG_T]            = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_big_msg,
                                              APP_TIMEOUT,SA_AIS_ERR_QUEUE_FULL},
   [MSG_MESSAGE_SEND_ERR_NOT_EXIST_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_SEND_ERR_NOT_EXIST2_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_SEND_WITH_BAD_PR_T]        = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg_bad_pr,
                                              APP_TIMEOUT,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_UNAVAILABLE_T]        = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,APP_TIMEOUT,
                                              SA_AIS_ERR_QUEUE_NOT_AVAILABLE},
   [MSG_MESSAGE_SEND_GR_SUCCESS_T]         = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_GR_SUCCESS_MSG2_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,
                                              &gl_mqa_env.send_msg_null_sndr_name,APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_GR_QUEUE_FULL_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_ERR_QUEUE_FULL},
   [MSG_MESSAGE_SEND_ZERO_SIZE_MSG_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg_zero_size,
                                              APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_MULTI_CAST_GR_FULL_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.send_big_msg,
                                              APP_TIMEOUT,SA_AIS_ERR_QUEUE_FULL},
   [MSG_MESSAGE_SEND_ERR_TRY_AGAIN_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              APP_TIMEOUT,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgMessageSend(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgMessageSend(*API_Mqsv_MessageSend[i].msgHandle,API_Mqsv_MessageSend[i].destination,
                         API_Mqsv_MessageSend[i].message,API_Mqsv_MessageSend[i].timeout);

   if(API_Mqsv_MessageSend[i].destination)
      m_TET_MQSV_PRINTF("\n Destination     : %s",API_Mqsv_MessageSend[i].destination->value);
   else
      m_TET_MQSV_PRINTF("\n Destination     : NULL");

   result = mqsv_test_result(rc,API_Mqsv_MessageSend[i].exp_output,
                             API_Mqsv_MessageSend_resultstring[i],flg);

   if(rc == SA_AIS_OK)
   {
      m_TET_MQSV_PRINTF("\n ***** Message Sent ***** ");
      msgDump(API_Mqsv_MessageSend[i].message);
      m_TET_MQSV_PRINTF(" ****************************\n");
   }

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgMessageSend(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgMessageSend(*API_Mqsv_MessageSend[i].msgHandle,API_Mqsv_MessageSend[i].destination,
                            API_Mqsv_MessageSend[i].message,API_Mqsv_MessageSend[i].timeout);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_MessageSend[i].destination)
            m_TET_MQSV_PRINTF("\n Destination     : %s",API_Mqsv_MessageSend[i].destination->value);
         else
            m_TET_MQSV_PRINTF("\n Destination     : NULL");
      }

      result = mqsv_test_result(rc,API_Mqsv_MessageSend[i].exp_output,
                                API_Mqsv_MessageSend_resultstring[i],flg);

      if(rc == SA_AIS_OK)
      {
         m_TET_MQSV_PRINTF("\n ***** Message Sent ***** ");
         msgDump(API_Mqsv_MessageSend[i].message);
         m_TET_MQSV_PRINTF(" ****************************\n");
      }
      
      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Send Async Test cases  ***************** */

char *API_Mqsv_MessageSendAsync_resultstring[] = {
   [MSG_MESSAGE_SEND_ASYNC_BAD_HDL_T]         = "saMsgMessageSendAsync with invalid Message Handle",
   [MSG_MESSAGE_SEND_ASYNC_FINALIZED_HDL_T]   = "saMsgMessageSendAsync with finalized Message Handle",
   [MSG_MESSAGE_SEND_ASYNC_NULL_NAME_T]       = "saMsgMessageSendAsync with Null Queue name",
   [MSG_MESSAGE_SEND_ASYNC_NULL_MSG_T]        = "saMsgMessageSendAsync with Null Message",
   [MSG_MESSAGE_SEND_ASYNC_NOT_EXIST_T]       = "saMsgMessageSendAsync to a queue that does not exist",
   [MSG_MESSAGE_SEND_ASYNC_BAD_GROUP_T]       = "saMsgMessageSendAsync to a group that does not exist",
   [MSG_MESSAGE_SEND_ASYNC_BAD_FLAGS_T]       = "saMsgMessageSendAsync with Bad Ack Flags",
   [MSG_MESSAGE_SEND_ASYNC_NO_ACK_SUCCESS_T]  = "Message SendAsync without ack",

   [MSG_MESSAGE_SEND_ASYNC_NAME1_T]           = "saMsgMessageSendAsync with valid parameters - Persistent",
   [MSG_MESSAGE_SEND_ASYNC_NAME2_T]           = "saMsgMessageSendAsync with valid parameters - Non Persistent",
   [MSG_MESSAGE_SEND_ASYNC_NAME3_T]           = "saMsgMessageSendAsync with valid parameters",
   [MSG_MESSAGE_SEND_ASYNC_NAME4_T]           = "saMsgMessageSendAsync with valid parameters",
   [MSG_MESSAGE_SEND_ASYNC_HDL2_NAME1_T]      = "saMsgMessageSendAsync with valid parameters",
   [MSG_MESSAGE_SEND_ASYNC_HDL2_NAME2_T]      = "saMsgMessageSendAsync with valid parameters",
   [MSG_MESSAGE_SEND_ASYNC_HDL2_NAME3_T]      = "saMsgMessageSendAsync with valid parameters",
   [MSG_MESSAGE_SEND_ASYNC_HDL2_NAME4_T]      = "saMsgMessageSendAsync with valid parameters",

   [MSG_MESSAGE_SEND_ASYNC_ERR_INIT_T]        = "saMsgMessageSendAsync - ERR_INIT case",
   [MSG_MESSAGE_SEND_ASYNC_ERR_INIT2_T]       = "saMsgMessageSendAsync - ERR_INIT case2",
   [MSG_MESSAGE_SEND_ASYNC_ERR_NOT_EXIST_T]   = "saMsgMessageSendAsync to an unavailable queue",
   [MSG_MESSAGE_SEND_ASYNC_ERR_NOT_EXIST2_T]  = "saMsgMessageSendAsync to an unavailable queue",
   [MSG_MESSAGE_SEND_ASYNC_ZERO_QUEUE_T]      = "Send to a zero size queue",
   [MSG_MESSAGE_SEND_ASYNC_QUEUE_FULL_T]      = "saMsgMessageSendAsync - QUEUE FULL case",
   [MSG_MESSAGE_SEND_ASYNC_WITH_BAD_PR_T]     = "saMsgMessageSendAsync with a message with bad priority",
   [MSG_MESSAGE_SEND_ASYNC_BIG_MSG_T]         = "Send a big message to the queue",
   [MSG_MESSAGE_SEND_ASYNC_UNAVAILABLE_T]     = "saMsgMessageSendAsync to an empty Queue Group",
   [MSG_MESSAGE_SEND_ASYNC_SUCCESS_Q1_MSG2_T] = "Send a message to a queue",
   [MSG_MESSAGE_SEND_ASYNC_SUCCESS_Q2_MSG2_T] = "Send a message to a queue",
   [MSG_MESSAGE_SEND_ASYNC_GR_SUCCESS_T]      = "saMsgMessageSendAsync to a Queue Group", 
   [MSG_MESSAGE_SEND_ASYNC_GR_SUCCESS_MSG2_T] = "saMsgMessageSendAsync to a Queue Group", 
   [MSG_MESSAGE_SEND_ASYNC_GR_QUEUE_FULL_T]   = "saMsgMessageSendAsync to a Queue Group - QUEUE FULL case", 
   [MSG_MESSAGE_SEND_ASYNC_ZERO_SIZE_MSG_T]   = "Send a message with zero message size",
   [MSG_MESSAGE_SEND_ASYNC_MULTI_CAST_GR_FULL_T] = "saMsgMessageSendAsync to a Queue Group - QUEUE FULL case", 
   [MSG_MESSAGE_SEND_ASYNC_ERR_TRY_AGAIN_T]   = "saMsgMessageSendAsync when service is not available",
};

struct SafMsgMessageSendAsync API_Mqsv_MessageSendAsync[] = {
   [MSG_MESSAGE_SEND_ASYNC_BAD_HDL_T]         = {&gl_mqa_env.inv_params.inv_msg_hdl,300,&gl_mqa_env.pers_q,
                                                 &gl_mqa_env.send_msg,SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_SEND_ASYNC_FINALIZED_HDL_T]   = {&gl_mqa_env.msg_hdl1,301,&gl_mqa_env.pers_q,
                                                 &gl_mqa_env.send_msg,SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_SEND_ASYNC_NULL_NAME_T]       = {&gl_mqa_env.msg_hdl1,302,NULL,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_ASYNC_NULL_MSG_T]        = {&gl_mqa_env.msg_hdl1,303,&gl_mqa_env.pers_q,NULL,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_ASYNC_NOT_EXIST_T]       = {&gl_mqa_env.msg_hdl1,304,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_SEND_ASYNC_BAD_GROUP_T]       = {&gl_mqa_env.msg_hdl1,305,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_SEND_ASYNC_BAD_FLAGS_T]       = {&gl_mqa_env.msg_hdl1,306,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                                 2,SA_AIS_ERR_BAD_FLAGS},
   [MSG_MESSAGE_SEND_ASYNC_NO_ACK_SUCCESS_T]  = {&gl_mqa_env.msg_hdl1,307,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                                 0,SA_AIS_OK},

   [MSG_MESSAGE_SEND_ASYNC_NAME1_T]           = {&gl_mqa_env.msg_hdl1,308,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg, 
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_NAME2_T]           = {&gl_mqa_env.msg_hdl1,309,&gl_mqa_env.non_pers_q,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_NAME3_T]           = {&gl_mqa_env.msg_hdl1,310,&gl_mqa_env.pers_q2,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_NAME4_T]           = {&gl_mqa_env.msg_hdl1,311,&gl_mqa_env.non_pers_q2,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_HDL2_NAME1_T]      = {&gl_mqa_env.msg_hdl2,312,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_HDL2_NAME2_T]      = {&gl_mqa_env.msg_hdl2,313,&gl_mqa_env.non_pers_q,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_HDL2_NAME3_T]      = {&gl_mqa_env.msg_hdl2,314,&gl_mqa_env.pers_q2,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_HDL2_NAME4_T]      = {&gl_mqa_env.msg_hdl2,315,&gl_mqa_env.non_pers_q2,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},

   [MSG_MESSAGE_SEND_ASYNC_ERR_INIT_T]        = {&gl_mqa_env.msg_hdl1,316,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                                 0,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_ERR_INIT2_T]       = {&gl_mqa_env.msg_hdl1,317,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_INIT},
   [MSG_MESSAGE_SEND_ASYNC_ERR_NOT_EXIST_T]   = {&gl_mqa_env.msg_hdl1,318,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_SEND_ASYNC_ERR_NOT_EXIST2_T]  = {&gl_mqa_env.msg_hdl1,319,&gl_mqa_env.non_pers_q,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_SEND_ASYNC_ZERO_QUEUE_T]      = {&gl_mqa_env.msg_hdl1,320,&gl_mqa_env.zero_q,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_QUEUE_FULL_T]      = {&gl_mqa_env.msg_hdl1,321,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_WITH_BAD_PR_T]     = {&gl_mqa_env.msg_hdl1,322,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg_bad_pr,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_ASYNC_BIG_MSG_T]         = {&gl_mqa_env.msg_hdl1,323,&gl_mqa_env.pers_q,&gl_mqa_env.send_big_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_SUCCESS_Q1_MSG2_T] = {&gl_mqa_env.msg_hdl1,324,&gl_mqa_env.pers_q,
                                                 &gl_mqa_env.send_msg_null_sndr_name,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_SUCCESS_Q2_MSG2_T] = {&gl_mqa_env.msg_hdl1,325,&gl_mqa_env.non_pers_q,
                                                 &gl_mqa_env.send_msg_null_sndr_name,0,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_UNAVAILABLE_T]     = {&gl_mqa_env.msg_hdl1,326,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_QUEUE_NOT_AVAILABLE},
   [MSG_MESSAGE_SEND_ASYNC_GR_SUCCESS_T]      = {&gl_mqa_env.msg_hdl1,327,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_GR_SUCCESS_MSG2_T] = {&gl_mqa_env.msg_hdl1,328,&gl_mqa_env.qgroup1,
                                                 &gl_mqa_env.send_msg_null_sndr_name,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_GR_QUEUE_FULL_T]   = {&gl_mqa_env.msg_hdl1,329,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_ZERO_SIZE_MSG_T]   = {&gl_mqa_env.msg_hdl1,330,&gl_mqa_env.pers_q,
                                                 &gl_mqa_env.send_msg_zero_size, 
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_MULTI_CAST_GR_FULL_T] = {&gl_mqa_env.msg_hdl1,331,&gl_mqa_env.qgroup1,
                                                    &gl_mqa_env.send_big_msg,SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_SEND_ASYNC_ERR_TRY_AGAIN_T]   = {&gl_mqa_env.msg_hdl1,332,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg, 
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgMessageSendAsync(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgMessageSendAsync(*API_Mqsv_MessageSendAsync[i].msgHandle,API_Mqsv_MessageSendAsync[i].invocation,
                              API_Mqsv_MessageSendAsync[i].destination,API_Mqsv_MessageSendAsync[i].message,
                              API_Mqsv_MessageSendAsync[i].ackFlags);

   if(API_Mqsv_MessageSendAsync[i].destination)
      m_TET_MQSV_PRINTF("\n Destination     : %s\n",API_Mqsv_MessageSendAsync[i].destination->value);
   else
      m_TET_MQSV_PRINTF("\n Destination     : NULL\n");

   if(API_Mqsv_MessageSendAsync[i].ackFlags >= 0 && API_Mqsv_MessageSendAsync[i].ackFlags < 2)
      m_TET_MQSV_PRINTF(" AckFlags        : %s",saMsgAckFlags_string[API_Mqsv_MessageSendAsync[i].ackFlags]);
   else
      m_TET_MQSV_PRINTF(" AckFlags        : %u",API_Mqsv_MessageSendAsync[i].ackFlags);

   result = mqsv_test_result(rc,API_Mqsv_MessageSendAsync[i].exp_output,
                             API_Mqsv_MessageSendAsync_resultstring[i],flg);

   if(rc == SA_AIS_OK)
   {
      m_TET_MQSV_PRINTF(" Invocation      : %llu\n",API_Mqsv_MessageSendAsync[i].invocation);
      m_TET_MQSV_PRINTF("\n ***** Message Input ***** ");
      msgDump(API_Mqsv_MessageSendAsync[i].message);
      m_TET_MQSV_PRINTF(" ************************* \n");
   }

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgMessageSendAsync(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgMessageSendAsync(*API_Mqsv_MessageSendAsync[i].msgHandle,
                                 API_Mqsv_MessageSendAsync[i].invocation,
                                 API_Mqsv_MessageSendAsync[i].destination,
                                 API_Mqsv_MessageSendAsync[i].message,
                                 API_Mqsv_MessageSendAsync[i].ackFlags);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_MessageSendAsync[i].destination)
            m_TET_MQSV_PRINTF("\n Destination     : %s\n",API_Mqsv_MessageSendAsync[i].destination->value);
         else
            m_TET_MQSV_PRINTF("\n Destination     : NULL\n");

         if(API_Mqsv_MessageSendAsync[i].ackFlags >= 0 && API_Mqsv_MessageSendAsync[i].ackFlags < 2)
            m_TET_MQSV_PRINTF(" AckFlags        : %s",
              saMsgAckFlags_string[API_Mqsv_MessageSendAsync[i].ackFlags]);
         else
            m_TET_MQSV_PRINTF(" AckFlags        : %u",API_Mqsv_MessageSendAsync[i].ackFlags);
      }

      result = mqsv_test_result(rc,API_Mqsv_MessageSendAsync[i].exp_output,
                                API_Mqsv_MessageSendAsync_resultstring[i],flg);

      if(rc == SA_AIS_OK)
      {
         m_TET_MQSV_PRINTF(" Invocation      : %llu\n",API_Mqsv_MessageSendAsync[i].invocation);
         m_TET_MQSV_PRINTF("\n ***** Message Input ***** ");
         msgDump(API_Mqsv_MessageSendAsync[i].message);
         m_TET_MQSV_PRINTF(" ************************* \n");
      }

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Get Test cases  ***************** */

char *API_Mqsv_MessageGet_resultstring[] = {
   [MSG_MESSAGE_GET_BAD_HDL_T]        = "saMsgMessageGet with invalid Queue Handle",
   [MSG_MESSAGE_GET_CLOSED_Q_HDL_T]   = "saMsgMessageGet with closed Queue Handle",
   [MSG_MESSAGE_GET_NULL_MSG_T]       = "saMsgMessageGet with Null Message",
   [MSG_MESSAGE_GET_NULL_SENDER_ID_T] = "saMsgMessageGet with Null SenderId",
   [MSG_MESSAGE_GET_ZERO_TIMEOUT_T]   = "saMsgMessageGet with zero timeout value (message exists in the queue)", 
   [MSG_MESSAGE_GET_NULL_SEND_TIME_T] = "saMsgMessageGet with Null Sendtime",
   [MSG_MESSAGE_GET_SUCCESS_T]        = "saMsgMessageGet with valid parameters",
   [MSG_MESSAGE_GET_NULL_SNDR_NAME_T] = "saMsgMessageGet with null sender name",
   [MSG_MESSAGE_GET_SUCCESS_HDL2_T]   = "saMsgMessageGet with valid parameters",
   [MSG_MESSAGE_GET_BAD_HDL2_T]       = "Message Get with Bad Queue Handle",
   [MSG_MESSAGE_GET_ERR_NO_SPACE_T]   = "Message Get with insufficient message buffer",
   [MSG_MESSAGE_GET_ERR_TIMEOUT_T]    = "saMsgMessageGet with small timeout",
   [MSG_MESSAGE_GET_ERR_INTERRUPT_T]  = "saMsgMessageGet that is cancelled",
   [MSG_MESSAGE_GET_RECV_SUCCESS_T]   = "Receive message from the queue",
   [MSG_MESSAGE_GET_ERR_TRY_AGAIN_T]  = "saMsgMessageGet when service is not available",
};

struct SafMsgMessageGet API_Mqsv_MessageGet[] = {
   [MSG_MESSAGE_GET_BAD_HDL_T]        = {&gl_mqa_env.inv_params.inv_q_hdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,APP_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_GET_CLOSED_Q_HDL_T]   = {&gl_mqa_env.pers_q_hdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,APP_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_GET_NULL_MSG_T]       = {&gl_mqa_env.pers_q_hdl,NULL,&gl_mqa_env.send_time,&gl_mqa_env.sender_id,0,
                                         SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_GET_NULL_SENDER_ID_T] = {&gl_mqa_env.pers_q_hdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,NULL,0,
                                         SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_GET_ZERO_TIMEOUT_T]   = {&gl_mqa_env.pers_q_hdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,0,SA_AIS_OK},
   [MSG_MESSAGE_GET_NULL_SEND_TIME_T] = {&gl_mqa_env.pers_q_hdl,&gl_mqa_env.rcv_msg,NULL,&gl_mqa_env.sender_id,
                                         APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_GET_SUCCESS_T]        = {&gl_mqa_env.pers_q_hdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_GET_NULL_SNDR_NAME_T] = {&gl_mqa_env.pers_q_hdl,&gl_mqa_env.rcv_msg_null_sndr_name,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_GET_SUCCESS_HDL2_T]   = {&gl_mqa_env.npers_q_hdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_GET_BAD_HDL2_T]       = {&gl_mqa_env.npers_q_hdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,APP_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_GET_ERR_NO_SPACE_T]   = {&gl_mqa_env.pers_q_hdl,&gl_mqa_env.no_space_rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,APP_TIMEOUT,SA_AIS_ERR_NO_SPACE},
   [MSG_MESSAGE_GET_ERR_TIMEOUT_T]    = {&gl_mqa_env.pers_q_hdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,100,SA_AIS_ERR_TIMEOUT},
   [MSG_MESSAGE_GET_ERR_INTERRUPT_T]  = {&gl_mqa_env.pers_q_hdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,APP_TIMEOUT,SA_AIS_ERR_INTERRUPT},
   [MSG_MESSAGE_GET_RECV_SUCCESS_T]   = {&gl_mqa_env.rcv_clbk_qhdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_GET_ERR_TRY_AGAIN_T]  = {&gl_mqa_env.pers_q_hdl,&gl_mqa_env.rcv_msg,&gl_mqa_env.send_time,
                                         &gl_mqa_env.sender_id,APP_TIMEOUT,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgMessageGet(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgMessageGet(*API_Mqsv_MessageGet[i].queueHandle,API_Mqsv_MessageGet[i].message,
                        API_Mqsv_MessageGet[i].sendTime,API_Mqsv_MessageGet[i].senderId,
                        API_Mqsv_MessageGet[i].timeout);

   m_TET_MQSV_PRINTF(" Queue Handle    : %llu",*API_Mqsv_MessageGet[i].queueHandle);

   result = mqsv_test_result(rc,API_Mqsv_MessageGet[i].exp_output,
                             API_Mqsv_MessageGet_resultstring[i],flg);

   if(rc == SA_AIS_OK)
   {
      m_TET_MQSV_PRINTF("\n ***** Message Received ***** ");
      msgDump(API_Mqsv_MessageGet[i].message);
      m_TET_MQSV_PRINTF(" **************************** \n");
      if(API_Mqsv_MessageGet[i].sendTime)
        m_TET_MQSV_PRINTF(" SendTime        : %llu\n",*API_Mqsv_MessageGet[i].sendTime);
      else
        m_TET_MQSV_PRINTF(" SendTime        : 0\n");
      m_TET_MQSV_PRINTF(" SenderId        : %llu\n",*API_Mqsv_MessageGet[i].senderId);
   }

   if(rc == SA_AIS_ERR_NO_SPACE)
   {
      m_TET_MQSV_PRINTF("\n ***** Message Input ***** ");
      msgDump(API_Mqsv_MessageGet[i].message);
      m_TET_MQSV_PRINTF(" ************************* \n");
   }

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgMessageGet(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgMessageGet(*API_Mqsv_MessageGet[i].queueHandle,API_Mqsv_MessageGet[i].message,
                           API_Mqsv_MessageGet[i].sendTime,API_Mqsv_MessageGet[i].senderId,
                           API_Mqsv_MessageGet[i].timeout);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF(" Queue Handle    : %llu",*API_Mqsv_MessageGet[i].queueHandle);

      result = mqsv_test_result(rc,API_Mqsv_MessageGet[i].exp_output,
                                API_Mqsv_MessageGet_resultstring[i],flg);

      if(rc == SA_AIS_OK)
      {
         m_TET_MQSV_PRINTF("\n ***** Message Received ***** ");
         msgDump(API_Mqsv_MessageGet[i].message);
         m_TET_MQSV_PRINTF(" **************************** \n");
         if(API_Mqsv_MessageGet[i].sendTime)
           m_TET_MQSV_PRINTF(" SendTime        : %llu\n",*API_Mqsv_MessageGet[i].sendTime);
         else
           m_TET_MQSV_PRINTF(" SendTime        : 0\n");
         m_TET_MQSV_PRINTF(" SenderId        : %llu\n",*API_Mqsv_MessageGet[i].senderId);
      }

      if(rc == SA_AIS_ERR_NO_SPACE)
      {
         m_TET_MQSV_PRINTF("\n ***** Message Input ***** ");
         msgDump(API_Mqsv_MessageGet[i].message);
         m_TET_MQSV_PRINTF(" ************************* \n");
      }

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Cancel Test cases  ***************** */

char *API_Mqsv_MessageCancel_resultstring[] = {
   [MSG_MESSAGE_CANCEL_BAD_HDL_T]   = "Message Cancel with invalid Queue Handle",
   [MSG_MESSAGE_CANCEL_CLOSED_Q_HDL_T] = "Message Cancel with closed Queue Handle",
   [MSG_MESSAGE_CANCEL_NO_BLKING_T] = "Message Cancel without any blocking calls",
   [MSG_MESSAGE_CANCEL_SUCCESS_T]   = "Successful Message Cancel",
   [MSG_MESSAGE_CANCEL_ERR_TRY_AGAIN_T] = "saMsgMessageGet when service is not available",
};

struct SafMsgMessageCancel API_Mqsv_MessageCancel[] = {
   [MSG_MESSAGE_CANCEL_BAD_HDL_T]   = {&gl_mqa_env.inv_params.inv_q_hdl,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_CANCEL_CLOSED_Q_HDL_T] = {&gl_mqa_env.pers_q_hdl,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_CANCEL_NO_BLKING_T] = {&gl_mqa_env.npers_q_hdl,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_CANCEL_SUCCESS_T]   = {&gl_mqa_env.pers_q_hdl,SA_AIS_OK},
   [MSG_MESSAGE_CANCEL_ERR_TRY_AGAIN_T] = {&gl_mqa_env.pers_q_hdl,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgMessageCancel(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   m_TET_MQSV_PRINTF(" Queue Handle   : %llu",*API_Mqsv_MessageCancel[i].queueHandle);

   rc = saMsgMessageCancel(*API_Mqsv_MessageCancel[i].queueHandle);

   result = mqsv_test_result(rc,API_Mqsv_MessageCancel[i].exp_output,
                             API_Mqsv_MessageCancel_resultstring[i],flg);

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgMessageCancel(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF(" Queue Handle   : %llu",*API_Mqsv_MessageCancel[i].queueHandle);

      rc = saMsgMessageCancel(*API_Mqsv_MessageCancel[i].queueHandle);

      result = mqsv_test_result(rc,API_Mqsv_MessageCancel[i].exp_output,
                                API_Mqsv_MessageCancel_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Send Receive Test cases  ***************** */

char *API_Mqsv_MessageSendReceive_resultstring[] = {
   [MSG_MESSAGE_SEND_RECV_BAD_HDL_T]       = "saMsgMessageSendReceive with invalid Message Handle",
   [MSG_MESSAGE_SEND_RECV_FINALIZED_HDL_T] = "saMsgMessageSendReceive with finalized Message Handle",
   [MSG_MESSAGE_SEND_RECV_NULL_NAME_T]     = "saMsgMessageSendReceive with Null Queue Name",
   [MSG_MESSAGE_SEND_RECV_NULL_MSG_T]      = "saMsgMessageSendReceive with Null send message",
   [MSG_MESSAGE_SEND_RECV_NULL_STIME_T]    = "saMsgMessageSendReceive with Null SendTime",
   [MSG_MESSAGE_SEND_RECV_ERR_TIMEOUT_T]     = "saMsgMessageSendReceive - ERR_TIMEOUT case",
   [MSG_MESSAGE_SEND_RECV_NULL_RMSG_T]     = "saMsgMessageSendReceive with Null receive message",
   [MSG_MESSAGE_SEND_RECV_ERR_NOT_EXIST_T] = "saMsgMessageSendReceive to a queue that does not exist",
   [MSG_MESSAGE_SEND_RECV_NOT_EXIST_GR_T]  = "saMsgMessageSendReceive to a group that does not exist",
   [MSG_MESSAGE_SEND_RECV_ERR_NO_SPACE_T]  = "saMsgMessageSendReceive with insufficient buffer",
   [MSG_MESSAGE_SEND_RECV_QUEUE_FULL_T]    = "saMsgMessageSendReceive - QUEUE FULL case",
   [MSG_MESSAGE_SEND_RECV_ZERO_Q_T]        = "saMsgMessageSendReceive to a zero size queue",
   [MSG_MESSAGE_SEND_RECV_BAD_PR_T]        = "saMsgMessageSendReceive a queue with a message with invalid priority",
   [MSG_MESSAGE_SEND_RECV_SUCCESS_T]       = "saMsgMessageSendReceive with valid parameters",
   [MSG_MESSAGE_SEND_RECV_SUCCESS_Q2_T]    = "saMsgMessageSendReceive with valid parameters",
   [MSG_MESSAGE_SEND_RECV_SUCCESS_Q3_T]    = "saMsgMessageSendReceive with valid parameters",
   [MSG_MESSAGE_SEND_RECV_SUCCESS_MSG2_T]  = "saMsgMessageSendReceive with null sender name in send msg",
   [MSG_MESSAGE_SEND_RECV_NULL_SNAME_T]    = "saMsgMessageSendReceive with null sender name in recv msg",
   [MSG_MESSAGE_SEND_RECV_UNAVALABLE_T]    = "saMsgMessageSendReceive to an empty group",
   [MSG_MESSAGE_SEND_RECV_SUCCESS_GR_T]    = "saMsgMessageSendReceive to a queue group",
   [MSG_MESSAGE_SEND_RECV_GR_QUEUE_FULL_T] = "saMsgMessageSendReceive to a queue group - QUEUE FULL case",
   [MSG_MESSAGE_SEND_RECV_ZERO_SIZE_MSG_T] = "saMsgMessageSendReceive with zero size message",
   [MSG_MESSAGE_SEND_RECV_MULTI_CAST_GR_T] = "saMsgMessageSendReceive to a multicast message queue group",
   [MSG_MESSAGE_SEND_RECV_ERR_TRY_AGAIN_T] = "saMsgMessageSendReceive when service is not available",
};

struct SafMsgMessageSendReceive API_Mqsv_MessageSendReceive[] = {
   [MSG_MESSAGE_SEND_RECV_BAD_HDL_T]       = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_SEND_RECV_FINALIZED_HDL_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_SEND_RECV_NULL_NAME_T]     = {&gl_mqa_env.msg_hdl1,NULL,&gl_mqa_env.send_msg,&gl_mqa_env.reply_msg,
                                              &gl_mqa_env.reply_send_time,MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_RECV_NULL_MSG_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,NULL,&gl_mqa_env.reply_msg,
                                              &gl_mqa_env.reply_send_time,MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_RECV_NULL_STIME_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,NULL,MSG_SEND_RCV_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_RECV_ERR_TIMEOUT_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,NULL,MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_TIMEOUT},
   [MSG_MESSAGE_SEND_RECV_NULL_RMSG_T]     = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,NULL,
                                              &gl_mqa_env.reply_send_time,MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_RECV_ERR_NOT_EXIST_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_SEND_RECV_NOT_EXIST_GR_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_SEND_RECV_ERR_NO_SPACE_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.no_space_reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_NO_SPACE},
   [MSG_MESSAGE_SEND_RECV_QUEUE_FULL_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_QUEUE_FULL},
   [MSG_MESSAGE_SEND_RECV_ZERO_Q_T]        = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.zero_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_QUEUE_FULL},
   [MSG_MESSAGE_SEND_RECV_BAD_PR_T]        = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg_bad_pr,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_RECV_SUCCESS_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,MSG_SEND_RCV_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_RECV_SUCCESS_Q2_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.non_pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,MSG_SEND_RCV_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_RECV_SUCCESS_Q3_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q2,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,MSG_SEND_RCV_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_RECV_SUCCESS_MSG2_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg_null_sndr_name,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,MSG_SEND_RCV_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_RECV_NULL_SNAME_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg_null_sndr_name,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_RECV_UNAVALABLE_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,MSG_SEND_RCV_TIMEOUT,
                                              SA_AIS_ERR_QUEUE_NOT_AVAILABLE},
   [MSG_MESSAGE_SEND_RECV_SUCCESS_GR_T]    = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,MSG_SEND_RCV_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_SEND_RECV_GR_QUEUE_FULL_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_QUEUE_FULL},
   [MSG_MESSAGE_SEND_RECV_ZERO_SIZE_MSG_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg_zero_size,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_TIMEOUT},
   [MSG_MESSAGE_SEND_RECV_MULTI_CAST_GR_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.qgroup1,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,
                                              MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_SEND_RECV_ERR_TRY_AGAIN_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.pers_q,&gl_mqa_env.send_msg,
                                              &gl_mqa_env.reply_msg,&gl_mqa_env.reply_send_time,MSG_SEND_RCV_TIMEOUT,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgMessageSendReceive(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgMessageSendReceive(*API_Mqsv_MessageSendReceive[i].msgHandle,API_Mqsv_MessageSendReceive[i].destination,
                                API_Mqsv_MessageSendReceive[i].sendMessage,API_Mqsv_MessageSendReceive[i].receiveMessage,
                                API_Mqsv_MessageSendReceive[i].replySendTime,API_Mqsv_MessageSendReceive[i].timeout);

   if(API_Mqsv_MessageSendReceive[i].destination)
      m_TET_MQSV_PRINTF("\n Destination     : %s",API_Mqsv_MessageSendReceive[i].destination->value);
   else
      m_TET_MQSV_PRINTF("\n Destination     : NULL");

   result = mqsv_test_result(rc,API_Mqsv_MessageSendReceive[i].exp_output,
                             API_Mqsv_MessageSendReceive_resultstring[i],flg);

   if(API_Mqsv_MessageSendReceive[i].sendMessage)
   {
      m_TET_MQSV_PRINTF("\n ***** Message Input ***** ");
      msgDump(API_Mqsv_MessageSendReceive[i].sendMessage);
      m_TET_MQSV_PRINTF(" **************************** \n");
   }
   else
      m_TET_MQSV_PRINTF("\n Message Input   : NULL");

   if(rc == SA_AIS_OK)
   {
      m_TET_MQSV_PRINTF("\n ***** Message Received ***** ");
      msgDump(API_Mqsv_MessageSendReceive[i].receiveMessage);
      m_TET_MQSV_PRINTF(" **************************** \n");

      if(API_Mqsv_MessageSendReceive[i].replySendTime)
         m_TET_MQSV_PRINTF(" ReplySendTime   : %llu\n",*API_Mqsv_MessageSendReceive[i].replySendTime);
      else
         m_TET_MQSV_PRINTF(" ReplySendTime   : 0\n");
   }

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgMessageSendReceive(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgMessageSendReceive(*API_Mqsv_MessageSendReceive[i].msgHandle,
                                   API_Mqsv_MessageSendReceive[i].destination,
                                   API_Mqsv_MessageSendReceive[i].sendMessage,
                                   API_Mqsv_MessageSendReceive[i].receiveMessage,
                                   API_Mqsv_MessageSendReceive[i].replySendTime,
                                   API_Mqsv_MessageSendReceive[i].timeout);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_MessageSendReceive[i].destination)
            m_TET_MQSV_PRINTF("\n Destination     : %s",API_Mqsv_MessageSendReceive[i].destination->value);
         else
            m_TET_MQSV_PRINTF("\n Destination     : NULL");
      }

      result = mqsv_test_result(rc,API_Mqsv_MessageSendReceive[i].exp_output,
                                API_Mqsv_MessageSendReceive_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_MessageSendReceive[i].sendMessage)
         {
            m_TET_MQSV_PRINTF("\n ***** Message Input ***** ");
            msgDump(API_Mqsv_MessageSendReceive[i].sendMessage);
            m_TET_MQSV_PRINTF(" **************************** \n");
         }
         else
            m_TET_MQSV_PRINTF("\n Message Input   : NULL");
      }

      if(rc == SA_AIS_OK)
      {
         m_TET_MQSV_PRINTF("\n ***** Message Received ***** ");
         msgDump(API_Mqsv_MessageSendReceive[i].receiveMessage);
         m_TET_MQSV_PRINTF(" **************************** \n");

         if(API_Mqsv_MessageSendReceive[i].replySendTime)
            m_TET_MQSV_PRINTF(" ReplySendTime   : %llu\n",
              *API_Mqsv_MessageSendReceive[i].replySendTime);
         else
            m_TET_MQSV_PRINTF(" ReplySendTime   : 0\n");
      }

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Send Reply Test cases  ***************** */

char *API_Mqsv_MessageReply_resultstring[] = {
   [MSG_MESSAGE_REPLY_BAD_HANDLE_T]     = "saMsgMessageReply with invalid Message Handle",
   [MSG_MESSAGE_REPLY_FINALIZED_HDL_T]  = "saMsgMessageReply with finalized Message Handle",
   [MSG_MESSAGE_REPLY_NULL_MSG_T]       = "saMsgMessageReply with Null message",
   [MSG_MESSAGE_REPLY_NULL_SID_T]       = "saMsgMessageReply with Null SenderId",
   [MSG_MESSAGE_REPLY_ERR_NOT_EXIST_T]  = "saMsgMessageReply with SenderId that does not exist",
   [MSG_MESSAGE_REPLY_SUCCESS_T]        = "saMsgMessageReply with valid parameters",
   [MSG_MESSAGE_REPLY_ERR_NO_SPACE_T]   = "saMsgMessageReply - ERR_NO_SPACE case",
   [MSG_MESSAGE_REPLY_NOT_EXIST_T]      = "saMsgMessageReply without any thread waiting for reply",
   [MSG_MESSAGE_REPLY_NULL_SNDR_NAME_T] = "saMsgMessageReply with NULL sender name",
   [MSG_MESSAGE_REPLY_ZERO_SIZE_MSG_T]  = "Reply with zero size message",
   [MSG_MESSAGE_REPLY_INV_PARAM_T]      = "Reply to a message that was not sent using saMsgMessageSendReceive",
   [MSG_MESSAGE_REPLY_ERR_TRY_AGAIN_T]  = "saMsgMessageReply when service is not available",
};

struct SafMsgMessageReply API_Mqsv_MessageReply[] = {
   [MSG_MESSAGE_REPLY_BAD_HANDLE_T]     = {&gl_mqa_env.inv_params.inv_msg_hdl,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                           APP_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_REPLY_FINALIZED_HDL_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                           APP_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_REPLY_NULL_MSG_T]       = {&gl_mqa_env.msg_hdl1,NULL,&gl_mqa_env.sender_id,APP_TIMEOUT,
                                           SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_REPLY_NULL_SID_T]       = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.send_msg,NULL,APP_TIMEOUT,
                                           SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_REPLY_ERR_NOT_EXIST_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.send_msg,&gl_mqa_env.inv_params.inv_sender_id,
                                           APP_TIMEOUT,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_REPLY_SUCCESS_T]        = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                           APP_TIMEOUT,SA_AIS_OK}, 
   [MSG_MESSAGE_REPLY_ERR_NO_SPACE_T]   = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,APP_TIMEOUT,
                                           SA_AIS_ERR_NO_SPACE}, 
   [MSG_MESSAGE_REPLY_NOT_EXIST_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,APP_TIMEOUT,
                                           SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_REPLY_NULL_SNDR_NAME_T] = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.send_msg_null_sndr_name,&gl_mqa_env.sender_id,
                                           APP_TIMEOUT,SA_AIS_OK},
   [MSG_MESSAGE_REPLY_ZERO_SIZE_MSG_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.send_msg_zero_size,&gl_mqa_env.sender_id,
                                           APP_TIMEOUT,SA_AIS_OK}, 
   [MSG_MESSAGE_REPLY_INV_PARAM_T]      = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                           APP_TIMEOUT,SA_AIS_ERR_INVALID_PARAM}, 
   [MSG_MESSAGE_REPLY_ERR_TRY_AGAIN_T]  = {&gl_mqa_env.msg_hdl1,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                           APP_TIMEOUT,SA_AIS_ERR_TRY_AGAIN}, 
};

int tet_test_msgMessageReply(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgMessageReply(*API_Mqsv_MessageReply[i].msgHandle,API_Mqsv_MessageReply[i].replyMessage,
                          API_Mqsv_MessageReply[i].senderId,API_Mqsv_MessageReply[i].timeout);

   if(API_Mqsv_MessageReply[i].senderId)
      m_TET_MQSV_PRINTF("\n SenderId        : %llu",*API_Mqsv_MessageReply[i].senderId);
   else
      m_TET_MQSV_PRINTF("\n SenderId        : NULL");

   result = mqsv_test_result(rc,API_Mqsv_MessageReply[i].exp_output,
                             API_Mqsv_MessageReply_resultstring[i],flg);

   if(rc == SA_AIS_OK)
   {
      m_TET_MQSV_PRINTF("\n ***** Reply Sent ***** ");
      msgDump(API_Mqsv_MessageReply[i].replyMessage);
      m_TET_MQSV_PRINTF(" ************************* \n");
   }

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgMessageReply(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgMessageReply(*API_Mqsv_MessageReply[i].msgHandle,API_Mqsv_MessageReply[i].replyMessage,
                             API_Mqsv_MessageReply[i].senderId,API_Mqsv_MessageReply[i].timeout);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_MessageReply[i].senderId)
            m_TET_MQSV_PRINTF("\n SenderId        : %llu",*API_Mqsv_MessageReply[i].senderId);
         else
            m_TET_MQSV_PRINTF("\n SenderId        : NULL");
      }

      result = mqsv_test_result(rc,API_Mqsv_MessageReply[i].exp_output,
                                API_Mqsv_MessageReply_resultstring[i],flg);

      if(rc == SA_AIS_OK)
      {
         m_TET_MQSV_PRINTF("\n ***** Reply Sent ***** ");
         msgDump(API_Mqsv_MessageReply[i].replyMessage);
         m_TET_MQSV_PRINTF(" ************************* \n");
      }

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Message Send Reply Async Test cases  ***************** */

char *API_Mqsv_MessageReplyAsync_resultstring[] = {
   [MSG_MESSAGE_REPLY_ASYNC_BAD_HDL_T]        = "saMsgMessageReplyAsync with invalid Message Handle",
   [MSG_MESSAGE_REPLY_ASYNC_FINALIZED_HDL_T]  = "saMsgMessageReplyAsync with finalized Message Handle",
   [MSG_MESSAGE_REPLY_ASYNC_NULL_MSG_T]       = "saMsgMessageReplyAsync with Null message",
   [MSG_MESSAGE_REPLY_ASYNC_NULL_SID_T]       = "saMsgMessageReplyAsync with Null SenderId",
   [MSG_MESSAGE_REPLY_ASYNC_BAD_FLAGS_T]      = "saMsgMessageReplyAsync with bad ackFlags",
   [MSG_MESSAGE_REPLY_ASYNC_ERR_NOT_EXIST_T]  = "saMsgMessageReplyAsync with invalid senderId",
   [MSG_MESSAGE_REPLY_ASYNC_SUCCESS_T]        = "saMsgMessageReplyAsync with valid parameters",
   [MSG_MESSAGE_REPLY_ASYNC_SUCCESS2_T]       = "saMsgMessageReplyAsync without Deliverd callback",
   [MSG_MESSAGE_REPLY_ASYNC_ERR_INIT_T]       = "saMsgMessageReplyAsync - ERR_INIT case (zero ackFlags)",
   [MSG_MESSAGE_REPLY_ASYNC_ERR_INIT2_T]      = "saMsgMessageReplyAsync - ERR_INIT case (non-zero ackFlags)",
   [MSG_MESSAGE_REPLY_ASYNC_NOT_EXIST_T]      = "saMsgMessageReplyAsync with no thread waiting for reply",
   [MSG_MESSAGE_REPLY_ASYNC_ERR_NO_SPACE_T]   = "saMsgMessageReplyAsync - ERR_NO_SPACE case",
   [MSG_MESSAGE_REPLY_ASYNC_NULL_SNDR_NAME_T] = "saMsgMessageReplyAsync with NULL sender name",
   [MSG_MESSAGE_REPLY_ASYNC_ZERO_SIZE_MSG_T]  = "saMsgMessageReplyAsync with zero size message",
   [MSG_MESSAGE_REPLY_ASYNC_INV_PARAM_T]      = "Reply to a message that was not sent using saMsgMessageSendReceive",
   [MSG_MESSAGE_REPLY_ASYNC_ERR_TRY_AGAIN_T]  = "saMsgMessageReplyAsyncwhen service is not available",
};

struct SafMsgMessageReplyAsync API_Mqsv_MessageReplyAsync[] = {
   [MSG_MESSAGE_REPLY_ASYNC_BAD_HDL_T]        = {&gl_mqa_env.inv_params.inv_msg_hdl,500,&gl_mqa_env.send_msg,
                                                 &gl_mqa_env.sender_id,SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_REPLY_ASYNC_FINALIZED_HDL_T]  = {&gl_mqa_env.msg_hdl1,501,&gl_mqa_env.send_msg,
                                                 &gl_mqa_env.sender_id,SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_BAD_HANDLE},
   [MSG_MESSAGE_REPLY_ASYNC_NULL_MSG_T]       = {&gl_mqa_env.msg_hdl1,502,NULL,&gl_mqa_env.sender_id,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_REPLY_ASYNC_NULL_SID_T]       = {&gl_mqa_env.msg_hdl1,503,&gl_mqa_env.send_msg,NULL,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_REPLY_ASYNC_BAD_FLAGS_T]      = {&gl_mqa_env.msg_hdl1,504,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                                 2,SA_AIS_ERR_BAD_FLAGS},
   [MSG_MESSAGE_REPLY_ASYNC_ERR_NOT_EXIST_T]  = {&gl_mqa_env.msg_hdl1,505,&gl_mqa_env.send_msg,
                                                 &gl_mqa_env.inv_params.inv_sender_id,SA_MSG_MESSAGE_DELIVERED_ACK,
                                                 SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_REPLY_ASYNC_SUCCESS_T]        = {&gl_mqa_env.msg_hdl1,506,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_REPLY_ASYNC_SUCCESS2_T]       = {&gl_mqa_env.msg_hdl1,507,&gl_mqa_env.send_msg,
                                                 &gl_mqa_env.sender_id,0,SA_AIS_OK},
   [MSG_MESSAGE_REPLY_ASYNC_ERR_INIT_T]       = {&gl_mqa_env.msg_hdl1,508,&gl_mqa_env.send_msg,
                                                 &gl_mqa_env.sender_id,0,SA_AIS_OK},
   [MSG_MESSAGE_REPLY_ASYNC_ERR_INIT2_T]      = {&gl_mqa_env.msg_hdl1,509,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_INIT},
   [MSG_MESSAGE_REPLY_ASYNC_NOT_EXIST_T]      = {&gl_mqa_env.msg_hdl1,510,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_NOT_EXIST},
   [MSG_MESSAGE_REPLY_ASYNC_ERR_NO_SPACE_T]   = {&gl_mqa_env.msg_hdl1,511,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_REPLY_ASYNC_NULL_SNDR_NAME_T] = {&gl_mqa_env.msg_hdl1,512,&gl_mqa_env.send_msg_null_sndr_name,
                                                 &gl_mqa_env.sender_id,SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_REPLY_ASYNC_ZERO_SIZE_MSG_T]  = {&gl_mqa_env.msg_hdl1,513,&gl_mqa_env.send_msg_zero_size,
                                                 &gl_mqa_env.sender_id,SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_OK},
   [MSG_MESSAGE_REPLY_ASYNC_INV_PARAM_T]      = {&gl_mqa_env.msg_hdl1,514,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_INVALID_PARAM},
   [MSG_MESSAGE_REPLY_ASYNC_ERR_TRY_AGAIN_T]  = {&gl_mqa_env.msg_hdl1,515,&gl_mqa_env.send_msg,&gl_mqa_env.sender_id,
                                                 SA_MSG_MESSAGE_DELIVERED_ACK,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_msgMessageReplyAsync(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saMsgMessageReplyAsync(*API_Mqsv_MessageReplyAsync[i].msgHandle,
                               API_Mqsv_MessageReplyAsync[i].invocation,
                               API_Mqsv_MessageReplyAsync[i].replyMessage,
                               API_Mqsv_MessageReplyAsync[i].senderId,
                               API_Mqsv_MessageReplyAsync[i].ackFlags);

   if(API_Mqsv_MessageReplyAsync[i].senderId)
      m_TET_MQSV_PRINTF("\n SenderId        : %llu\n",*API_Mqsv_MessageReplyAsync[i].senderId);
   else
      m_TET_MQSV_PRINTF("\n SenderId        : NULL\n");

   if(API_Mqsv_MessageReplyAsync[i].ackFlags >= 0 && API_Mqsv_MessageReplyAsync[i].ackFlags < 2)
      m_TET_MQSV_PRINTF(" AckFlags        : %s",
                        saMsgAckFlags_string[API_Mqsv_MessageReplyAsync[i].ackFlags]);
   else
      m_TET_MQSV_PRINTF(" AckFlags        : %u",API_Mqsv_MessageReplyAsync[i].ackFlags);

   result = mqsv_test_result(rc,API_Mqsv_MessageReplyAsync[i].exp_output,
                             API_Mqsv_MessageReplyAsync_resultstring[i],flg);

   if(rc == SA_AIS_OK)
   {
      m_TET_MQSV_PRINTF(" Invocation      : %llu\n",API_Mqsv_MessageReplyAsync[i].invocation);
      m_TET_MQSV_PRINTF("\n ***** Message Input ***** ");
      msgDump(API_Mqsv_MessageReplyAsync[i].replyMessage);
      m_TET_MQSV_PRINTF(" ************************* \n");
   }

   m_TET_MQSV_PRINTF("\n");

   return(result);
}

int tet_test_red_msgMessageReplyAsync(int i,MQSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   gl_try_again_cnt = 0;

   do
   {
      rc = saMsgMessageReplyAsync(*API_Mqsv_MessageReplyAsync[i].msgHandle,
                                  API_Mqsv_MessageReplyAsync[i].invocation,
                                  API_Mqsv_MessageReplyAsync[i].replyMessage,
                                  API_Mqsv_MessageReplyAsync[i].senderId,
                                  API_Mqsv_MessageReplyAsync[i].ackFlags);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         if(API_Mqsv_MessageReplyAsync[i].senderId)
            m_TET_MQSV_PRINTF("\n SenderId        : %llu\n",*API_Mqsv_MessageReplyAsync[i].senderId);
         else
            m_TET_MQSV_PRINTF("\n SenderId        : NULL\n");

         if(API_Mqsv_MessageReplyAsync[i].ackFlags >= 0 && API_Mqsv_MessageReplyAsync[i].ackFlags < 2)
            m_TET_MQSV_PRINTF(" AckFlags        : %s",
                              saMsgAckFlags_string[API_Mqsv_MessageReplyAsync[i].ackFlags]);
         else
            m_TET_MQSV_PRINTF(" AckFlags        : %u",API_Mqsv_MessageReplyAsync[i].ackFlags);
      }

      result = mqsv_test_result(rc,API_Mqsv_MessageReplyAsync[i].exp_output,
                                API_Mqsv_MessageReplyAsync_resultstring[i],flg);

      if(rc == SA_AIS_OK)
      {
         m_TET_MQSV_PRINTF(" Invocation      : %llu\n",API_Mqsv_MessageReplyAsync[i].invocation);
         m_TET_MQSV_PRINTF("\n ***** Message Input ***** ");
         msgDump(API_Mqsv_MessageReplyAsync[i].replyMessage);
         m_TET_MQSV_PRINTF(" ************************* \n");
      }

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_MQSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_MQSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* *************** MQSV Threads ***************** */

/* Message SendReceive thread */

void mqsv_sendrecv(NCSCONTEXT arg)
{
   MSG_MESSAGE_SEND_RECV_TC_TYPE *tc = (MSG_MESSAGE_SEND_RECV_TC_TYPE *)arg;
   int result;

   result = tet_test_msgMessageSendReceive(*tc,TEST_CONFIG_MODE);
   if (result != TET_PASS)
      m_TET_MQSV_PRINTF("\n Message SendReceive Failed \n");
   else
      m_TET_MQSV_PRINTF("\n Message SendReceive Success \n");
}

void mqsv_create_sendrecvthread(MSG_MESSAGE_SEND_RECV_TC_TYPE *tc)
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;

   rc = m_NCS_TASK_CREATE((NCS_OS_CB)mqsv_sendrecv, (NCSCONTEXT)tc,"mqa_sendrcv_test",
                          5, 8000, &thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
       m_TET_MQSV_PRINTF(" Failed to create thread\n");
       return ;
   }

   rc = m_NCS_TASK_START(thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
       m_TET_MQSV_PRINTF(" Failed to start thread\n");
       return ;
   }
}

/* Message Cancel thread */

void mqsv_cancel_msg(NCSCONTEXT arg)
{
   SaMsgQueueHandleT *queueHandle = (SaMsgQueueHandleT *)arg;
   SaAisErrorT rc;

   sleep(5);
   rc = saMsgMessageCancel(*queueHandle);
   if (rc != SA_AIS_OK)
   {
      m_TET_MQSV_PRINTF("\n Return Value %s\n",mqsv_saf_error_string[rc]);
      m_TET_MQSV_PRINTF("\n Message Cancel failed \n");
   }
   else
   {
      m_TET_MQSV_PRINTF("\n Message Cancel Success \n");
      sleep(5);
   }
   return;
}

void mqsv_createcancelthread(SaMsgQueueHandleT *queueHandle)
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;

   rc = m_NCS_TASK_CREATE((NCS_OS_CB)mqsv_cancel_msg, (NCSCONTEXT)queueHandle,"mqa_cancelest",
                          5, 8000, &thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
       m_TET_MQSV_PRINTF(" Failed to create thread\n");
       return ;
   }

   rc = m_NCS_TASK_START(thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
       m_TET_MQSV_PRINTF(" Failed to start thread\n");
       return ;
   }
}

/* Dispatch Thread - DISPATCH_BLOCKING */

void mqsv_selection_thread_blocking (NCSCONTEXT arg)
{
   SaMsgHandleT *msgHandle = (SaMsgHandleT *)arg;
   uint32_t rc;

   rc = saMsgDispatch(*msgHandle, SA_DISPATCH_BLOCKING);
   if (rc != SA_AIS_OK)
   {
      m_TET_MQSV_PRINTF(" Dispatching failed %s \n", mqsv_saf_error_string[rc]);
      pthread_exit(0);
   }
}

void mqsv_createthread(SaMsgHandleT *msgHandle)
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;

   if(gl_msg_thrd_cnt >= 1)
      return;

   rc = m_NCS_TASK_CREATE((NCS_OS_CB)mqsv_selection_thread_blocking, (NCSCONTEXT)msgHandle,
                           "mqa_asynctest_blocking", 5, 8000, &thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
       m_TET_MQSV_PRINTF(" Failed to create thread\n");
       return ;
   }

   rc = m_NCS_TASK_START(thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
       m_TET_MQSV_PRINTF(" Failed to start thread\n");
       return ;
   }
}

/* Dispatch Thread - DISPATCH_ONE */

void mqsv_selection_thread_one (NCSCONTEXT arg)
{
   SaMsgHandleT *msgHandle = (SaMsgHandleT *)arg;
   uint32_t rc;

   rc = saMsgDispatch(*msgHandle, SA_DISPATCH_ONE);
   if (rc != SA_AIS_OK)
      m_TET_MQSV_PRINTF(" Dispatching failed %s \n", mqsv_saf_error_string[rc]);
   pthread_exit(0);
}

void mqsv_createthread_one(SaMsgHandleT *msgHandle)
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;
 
   if(gl_msg_thrd_cnt >= 1)
      return;

   rc = m_NCS_TASK_CREATE((NCS_OS_CB)mqsv_selection_thread_one, (NCSCONTEXT)msgHandle,
                           "mqa_asynctest_one", 5, 8000, &thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
      m_TET_MQSV_PRINTF(" Failed to create thread\n");
      return ;
   }

   rc = m_NCS_TASK_START(thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
      m_TET_MQSV_PRINTF(" Failed to start thread\n");
      return ;
   }
}

/* Dispatch Thread - DISPATCH_ALL */

void mqsv_selection_thread_all (NCSCONTEXT arg)
{
   SaMsgHandleT *msgHandle = (SaMsgHandleT *)arg;
   uint32_t rc;

   rc = saMsgDispatch(*msgHandle, SA_DISPATCH_ALL);
   if (rc != SA_AIS_OK)
      m_TET_MQSV_PRINTF(" Dispatching failed %s \n", mqsv_saf_error_string[rc]);
   pthread_exit(0);
}

void mqsv_createthread_all(SaMsgHandleT *msgHandle)
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;

   if(gl_msg_thrd_cnt >= 1)
      return;

   rc = m_NCS_TASK_CREATE((NCS_OS_CB)mqsv_selection_thread_all, (NCSCONTEXT)msgHandle,
                           "mqa_asynctest_all", 5, 8000, &thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
      m_TET_MQSV_PRINTF(" Failed to create thread\n");
      return ;
   }

   rc = m_NCS_TASK_START(thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
      m_TET_MQSV_PRINTF(" Failed to start thread\n");
      return ;
   }
}

/* Dispatch Thread - DISPATCH_ONE Infinite loop */

void mqsv_selection_thread_all_loop (NCSCONTEXT arg)
{
   while(1)
   {
      saMsgDispatch(gl_mqa_env.msg_hdl1, SA_DISPATCH_ALL);
      sleep(1);
   }
}

void mqsv_createthread_all_loop()
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;

   if(gl_msg_thrd_cnt >= 1)
      return;

   rc = m_NCS_TASK_CREATE((NCS_OS_CB)mqsv_selection_thread_all_loop, (NCSCONTEXT)NULL,
                           "mqa_asynctest_loop", 5, 8000, &thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
       m_TET_MQSV_PRINTF(" Failed to create thread\n");
       return ;
   }

   gl_msg_thrd_cnt++;
   rc = m_NCS_TASK_START(thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
       m_TET_MQSV_PRINTF(" Failed to start thread\n");
       return ;
   }
}

/* ********* MQSV RESTORE FUNCTION ********** */

void mqsv_restore_params(MQSV_RESTORE_OPT opt)
{
   switch(opt)
   {
      /* Restore - Initialization Version */

      case MSG_RESTORE_INIT_BAD_VERSION_T:
         mqsv_fill_msg_version(&gl_mqa_env.inv_params.inv_version,'C',0,1);
         break;

      case MSG_RESTORE_INIT_BAD_REL_CODE_T:
         mqsv_fill_msg_version(&gl_mqa_env.inv_params.inv_ver_bad_rel_code,'\0',1,0);
         break;

      case MSG_RESTORE_INIT_BAD_MAJOR_VER_T:
         mqsv_fill_msg_version(&gl_mqa_env.inv_params.inv_ver_not_supp,'B',3,0);
         break;

      /* Restore - Group Track Info */

      case MSG_RESTORE_GROUP_TRACK_CURRENT_T:
         mqsv_fill_grp_notif_buffer(&gl_mqa_env.buffer_null_notif,3,NULL);
         break;

      case MSG_RESTORE_GROUP_TRACK_CURRENT_NON_NULL_NOTIF_T:
         memset(&gl_mqa_env.buffer_non_null_notif,'\0',sizeof(SaMsgQueueGroupNotificationBufferT));
         break;

      case MSG_RESTORE_GROUP_TRACK_CUR_NO_SPACE_T:
         gl_mqa_env.no_space_notif_buf.numberOfItems = 1;

      /* Restore - Received Message */

      case MSG_RESTORE_MESSAGE_GET_SUCCESS_T:
      case MSG_RESTORE_MESSAGE_GET_ZERO_TIMEOUT_T:
         gl_mqa_env.sender_id = 0;
         gl_mqa_env.send_time = 0;
         memset(&gl_mqa_env.rcv_msg,'\0',sizeof(SaMsgMessageT));
         memset(&gl_mqa_env.rcv_sender_name,'\0',sizeof(SaNameT));
         mqsv_fill_rcv_message(&gl_mqa_env.rcv_msg,NULL,2,&gl_mqa_env.rcv_sender_name);

      case MSG_RESTORE_MESSAGE_GET_NULL_SNDR_NAME_T:
         gl_mqa_env.sender_id = 0;
         gl_mqa_env.send_time = 0;
         memset(&gl_mqa_env.rcv_msg_null_sndr_name,'\0',sizeof(SaMsgMessageT));
         mqsv_fill_rcv_message(&gl_mqa_env.rcv_msg_null_sndr_name,NULL,0,NULL);

      case MSG_RESTORE_MESSAGE_GET_ERR_NO_SPACE_T:
         gl_mqa_env.sender_id = 0;
         gl_mqa_env.send_time = 0;
         memset(&gl_mqa_env.rcv_sender_name,'\0',sizeof(SaNameT));
         gl_mqa_env.no_space_rcv_msg.size = 10;
         gl_mqa_env.no_space_rcv_msg.senderName = &gl_mqa_env.rcv_sender_name;

      case MSG_RESTORE_MESSAGE_SEND_RECV_NULL_STIME_T:
      case MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_T:
         gl_mqa_env.reply_send_time = 0;
         memset(&gl_mqa_env.reply_msg,'\0',sizeof(SaMsgMessageT));
         memset(&gl_mqa_env.rcv_sender_name,'\0',sizeof(SaNameT));
         mqsv_fill_rcv_message(&gl_mqa_env.reply_msg,NULL,2,&gl_mqa_env.rcv_sender_name);

      case MSG_RESTORE_MESSAGE_SEND_RECV_SUCCESS_MSG2_T:
         gl_mqa_env.reply_send_time = 0;
         memset(&gl_mqa_env.reply_msg_null_sndr_name,'\0',sizeof(SaMsgMessageT));
         mqsv_fill_rcv_message(&gl_mqa_env.reply_msg_null_sndr_name,NULL,0,NULL);

      case MSG_RESTORE_MESSAGE_SEND_RECV_ERR_NO_SPACE_T:
         gl_mqa_env.reply_send_time = 0;
         memset(&gl_mqa_env.rcv_sender_name,'\0',sizeof(SaNameT));
         gl_mqa_env.no_space_reply_msg.size = 10;
         gl_mqa_env.no_space_reply_msg.senderName = &gl_mqa_env.rcv_sender_name;
   }
}

/* ********* MQSV CLEANUP FUNCTIONS ********** */

/* Cleanupt Ouput parameters */

void mqsv_clean_output_params()
{
   gl_mqa_env.msg_hdl1 = 0;
   gl_mqa_env.msg_hdl2 = 0;
   gl_mqa_env.null_clbks_msg_hdl = 0;
   gl_mqa_env.null_rcv_clbk_msg_hdl = 0;
   gl_mqa_env.sel_obj = 0;
   gl_mqa_env.pers_q_hdl = 0;
   gl_mqa_env.npers_q_hdl = 0;
   gl_mqa_env.pers_q_hdl2 = 0;
   gl_mqa_env.npers_q_hdl2 = 0;
   gl_mqa_env.msg_hdl1 = 0;
}

/* Cleanup Callback parameters */

void mqsv_clean_clbk_params()
{
   gl_mqa_env.open_clbk_qhdl = 0;
   gl_mqa_env.open_clbk_err = 0;
   gl_mqa_env.open_clbk_invo = 0;

   gl_mqa_env.del_clbk_invo = 0;
   gl_mqa_env.del_clbk_err = 0;

   gl_mqa_env.rcv_clbk_qhdl = 0;

   gl_mqa_env.track_clbk_err = 0;
   gl_mqa_env.track_clbk_num_mem = 0;
   memset(&gl_mqa_env.track_clbk_grp_name,'\0',sizeof(SaNameT));
   if(gl_mqa_env.track_clbk_notif.notification != NULL)
      free(gl_mqa_env.track_clbk_notif.notification);
   memset(&gl_mqa_env.track_clbk_notif,'\0',sizeof(SaMsgQueueGroupNotificationBufferT));
}

/* Cleanup Queue Status Info */

void mqsv_clean_q_status()
{
   memset(&gl_mqa_env.q_status,'\0',sizeof(SaMsgQueueStatusT));
}

/* Cleanup Initialization Handles */

void mqsv_init_cleanup(MQSV_INIT_CLEANUP_OPT opt)
{
   int result;

   switch(opt)
   {
      case MSG_CLEAN_INIT_SUCCESS_T:
      case MSG_CLEAN_INIT_NULL_CBKS_T:
      case MSG_CLEAN_INIT_NULL_CLBK_PARAM_T:
         result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_INIT_SUCCESS_HDL2_T:
         result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_HDL2_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_INIT_NULL_CBKS2_T:
         result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_HDL3_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_INIT_NULL_RCV_CBK_T:
         result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_HDL4_T,TEST_CLEANUP_MODE);
         break;
   }

   if(result != TET_PASS)
      m_TET_MQSV_PRINTF("\n\n+++++++++ FAILED - Cleaning up Initialization Handles +++++++\n\n");
}

void mqsv_init_red_cleanup(MQSV_INIT_CLEANUP_OPT opt)
{
   int result;

   switch(opt)
   {
      case MSG_CLEAN_INIT_SUCCESS_T:
      case MSG_CLEAN_INIT_NULL_CBKS_T:
      case MSG_CLEAN_INIT_NULL_CLBK_PARAM_T:
         result = tet_test_red_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_INIT_SUCCESS_HDL2_T:
         result = tet_test_red_msgFinalize(MSG_FINALIZE_SUCCESS_HDL2_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_INIT_NULL_CBKS2_T:
         result = tet_test_red_msgFinalize(MSG_FINALIZE_SUCCESS_HDL3_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_INIT_NULL_RCV_CBK_T:
         result = tet_test_red_msgFinalize(MSG_FINALIZE_SUCCESS_HDL4_T,TEST_CLEANUP_MODE);
         break;
   }

   if(result != TET_PASS)
      m_TET_MQSV_PRINTF("\n\n+++++++++ FAILED - Cleaning up Initialization Handles +++++++\n\n");
}

/* Cleanup Message Queues */

void mqsv_q_cleanup(MQSV_Q_CLEANUP_OPT opt)
{
   int result;

   switch(opt)
   {
      case MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_ASYNC_EMPTY_CREATE_T:
      case MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_RECV_CLBK_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_EMPTY_CREATE_T:
      case MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T:
         result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_RECV_CLBK_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T:
         result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q2_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK2_T:
         result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q3_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_QUEUE_OPEN_ASYNC_ZERO_RET_T:
      case MSG_CLEAN_QUEUE_OPEN_ZERO_RET_T:
      case MSG_CLEAN_QUEUE_OPEN_NPERS_RECV_CLBK2_T:
         result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q4_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_QUEUE_OPEN_ASYNC_ZERO_SIZE_T:
      case MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T:
         result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q5_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_QUEUE_OPEN_AFTER_FINALIZE:
         result = tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CLEANUP_MODE);
         result = tet_test_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CLEANUP_MODE);
         result = tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CLEANUP_MODE);
   }

   if(result != TET_PASS)
      m_TET_MQSV_PRINTF("\n\n+++++++++ FAILED - Cleaning up Message Queues +++++++\n\n");
}

void mqsv_q_red_cleanup(MQSV_Q_CLEANUP_OPT opt)
{
   int result;

   switch(opt)
   {
      case MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_ASYNC_EMPTY_CREATE_T:
      case MSG_CLEAN_QUEUE_OPEN_ASYNC_PERS_RECV_CLBK_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_PERS_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_EMPTY_CREATE_T:
      case MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK_SUCCESS_T:
         result = tet_test_red_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_ASYNC_NPERS_RECV_CLBK_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_NON_PERS_SUCCESS_T:
      case MSG_CLEAN_QUEUE_OPEN_NON_PERS_RECV_CLBK_SUCCESS_T:
         result = tet_test_red_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q2_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_QUEUE_OPEN_PERS_RECV_CLBK2_T:
         result = tet_test_red_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q3_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_QUEUE_OPEN_ASYNC_ZERO_RET_T:
      case MSG_CLEAN_QUEUE_OPEN_ZERO_RET_T:
      case MSG_CLEAN_QUEUE_OPEN_NPERS_RECV_CLBK2_T:
         result = tet_test_red_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q4_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_QUEUE_OPEN_ASYNC_ZERO_SIZE_T:
      case MSG_CLEAN_QUEUE_OPEN_ZERO_SIZE_T:
         result = tet_test_red_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q5_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_QUEUE_OPEN_AFTER_FINALIZE:
         result = tet_test_red_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CLEANUP_MODE);
         result = tet_test_red_msgQueueUnlink(MSG_QUEUE_UNLINK_SUCCESS_Q1_T,TEST_CLEANUP_MODE);
         result = tet_test_red_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CLEANUP_MODE);
   }

   if(result != TET_PASS)
      m_TET_MQSV_PRINTF("\n\n+++++++++ FAILED - Cleaning up Message Queues +++++++\n\n");
}

/* Cleanup Message Queue Groups */

void mqsv_q_grp_cleanup(MQSV_Q_GRP_CLEANUP_OPT opt)
{
   int result;

   switch(opt)
   {
      case MSG_CLEAN_GROUP_CREATE_SUCCESS_T:
      case MSG_CLEAN_GROUP_CREATE_LCL_BSTQ_NOT_SUPP_T:
      case MSG_CLEAN_GROUP_CREATE_LOCAL_RR_T:
      case MSG_CLEAN_GROUP_CREATE_BROADCAST_T:
         result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_GROUP_CREATE_SUCCESS_GR2_T:
      case MSG_CLEAN_GROUP_CREATE_BROADCAST2_T:
         result = tet_test_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_GR2_T,TEST_CLEANUP_MODE);
         break;
   }

   if(result != TET_PASS)
      m_TET_MQSV_PRINTF("\n\n+++++++++ FAILED - Cleaning up Message Queue Groups +++++++\n\n");
}

void mqsv_q_grp_red_cleanup(MQSV_Q_GRP_CLEANUP_OPT opt)
{
   int result;

   switch(opt)
   {
      case MSG_CLEAN_GROUP_CREATE_SUCCESS_T:
      case MSG_CLEAN_GROUP_CREATE_LCL_BSTQ_NOT_SUPP_T:
      case MSG_CLEAN_GROUP_CREATE_LOCAL_RR_T:
      case MSG_CLEAN_GROUP_CREATE_BROADCAST_T:
         result = tet_test_red_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_T,TEST_CLEANUP_MODE);
         break;

      case MSG_CLEAN_GROUP_CREATE_SUCCESS_GR2_T:
      case MSG_CLEAN_GROUP_CREATE_BROADCAST2_T:
         result = tet_test_red_msgGroupDelete(MSG_GROUP_DELETE_SUCCESS_GR2_T,TEST_CLEANUP_MODE);
         break;
   }

   if(result != TET_PASS)
      m_TET_MQSV_PRINTF("\n\n+++++++++ FAILED - Cleaning up Message Queue Groups +++++++\n\n");
}

/* Stop Queue Group Tracking */

void mqsv_q_grp_track_stop(MQSV_Q_GRP_TRACK_STOP_OPT opt)
{
   int result;

   switch(opt)
   {
      case MSG_STOP_GROUP_TRACK_CHANGES_T:
      case MSG_STOP_GROUP_TRACK_CHGS_ONLY_T:
      case MSG_STOP_GROUP_TRACK_CUR_CHLY_NUL_BUF_T:
      case MSG_STOP_GROUP_TRACK_CUR_CH_NUL_BUF_T:
      case MSG_STOP_GROUP_TRACK_CUR_CH_T:
      case MSG_STOP_GROUP_TRACK_CUR_CHLY_T:
         result = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_SUCCESS_T,TEST_CLEANUP_MODE);
         break;

      case MSG_STOP_GROUP_TRACK_CHANGES_GR2_T:
         result = tet_test_msgGroupTrackStop(MSG_GROUP_TRACK_STOP_SUCCESS_GR2_T,TEST_CLEANUP_MODE);
         break;
   }

   if(result != TET_PASS)
      m_TET_MQSV_PRINTF("\n\n++++++++ FAILED - Stopping Queue Group Tracking +++++++\n\n");
}

#endif
