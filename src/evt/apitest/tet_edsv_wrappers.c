#include <string.h>
#include "tet_api.h"
#include "tet_startup.h"
#include "tet_eda.h"


extern int gl_sync_pointnum;
extern int fill_syncparameters(int);
extern int gl_jCount;
extern int gl_err;
extern SaTimeT gl_timeout;
extern SaInvocationT gl_invocation;
extern SaEvtEventPatternT gl_pattern[2];
extern SaEvtEventFilterT gl_filter[1];
extern const char *gl_saf_error[32];
extern int gl_major_version;
extern int gl_minor_version;
extern int gl_b03_flag;

/****************************************************************/
/***************** EDSV CALLBACK FUNCTIONS **********************/
/****************************************************************/
void EvtOpenCallback(SaInvocationT invocationCallback, 
                     SaEvtChannelHandleT asyncChannelHandle, SaAisErrorT error)
{
  gl_cbk=1;
  gl_invocation=invocationCallback;
  gl_channelHandle=asyncChannelHandle;
  if ( error != SA_AIS_OK)
    {
      printf("\nError %s \n",gl_saf_error[error]);
      gl_error=error;
    }
  else 
    {
      printf("\nOpen Callback function called with invocation %llu and \
channel hdl %llu\n",gl_invocation,gl_channelHandle);
    }
  if(gl_listNumber==4)
    {
      gl_node_id=error;
    }
}

void b03EvtDeliverCallback(SaEvtSubscriptionIdT subscriptionId, 
                        SaEvtEventHandleT deliverEventHandle, 
                        SaSizeT eventDataSize)
{
  gl_cbk=1;
  printf("\nEvent Deliver Handle (in deliver callback): %llu",
         deliverEventHandle);
  gl_eventDeliverHandle=deliverEventHandle;
  gl_dupSubscriptionId=subscriptionId;
  printf("\n\nSubscription id : %u",subscriptionId);

  if(subscriptionId!=37)
    {
      tet_saEvtEventDataGet(&gl_eventDeliverHandle);
    }
  printf("\nDeliver Callback function");

  gl_jCount++;
  printf("\nInvoked these many times : %d",gl_jCount);

  tet_saEvtEventAttributesGet(&gl_eventDeliverHandle);
  subCount++;
}

void EvtDeliverCallback(SaEvtSubscriptionIdT subscriptionId, 
                        SaEvtEventHandleT deliverEventHandle, 
                        SaSizeT eventDataSize)
{
  gl_cbk=1;
  printf("\nEvent Deliver Handle (in deliver callback): %llu",
         deliverEventHandle);
  gl_eventDeliverHandle=deliverEventHandle;
  gl_dupSubscriptionId=subscriptionId;
  printf("\n\nSubscription id : %u",subscriptionId);

  if(subscriptionId!=37)
    {
      tet_saEvtEventDataGet(&gl_eventDeliverHandle);
    }
  printf("\nDeliver Callback function");

  gl_jCount++;
  printf("\nInvoked these many times : %d",gl_jCount);

  tet_saEvtEventAttributesGet(&gl_eventDeliverHandle);
  subCount++;
}

SaEvtCallbacksT gl_evtCallbacks=
  {
    saEvtChannelOpenCallback:EvtOpenCallback,
    saEvtEventDeliverCallback:EvtDeliverCallback
  };

/****************************************************************/
/***************** EDSV API WRAPPERS ****************************/
/****************************************************************/
void tet_saEvtInitialize(SaEvtHandleT *ptrEvtHandle)
{
  static int try_again_count;
  gl_rc=saEvtInitialize(ptrEvtHandle,&gl_evtCallbacks,&gl_version);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Initialize API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtInitialize(ptrEvtHandle,&gl_evtCallbacks,&gl_version);

      if(gl_rc==SA_AIS_OK)
        printf("\n Initialize Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtInitialize() invoked",gl_rc);

  gl_evtHandle=*ptrEvtHandle;

  printf("\nEvent Handle (when initialized): %llu",*ptrEvtHandle);
}
void tet_saEvtSelectionObjectGet(SaEvtHandleT *ptrEvtHandle,
                                 SaSelectionObjectT *ptrSelectionObject)
{
  static int try_again_count;
  gl_rc=saEvtSelectionObjectGet(*ptrEvtHandle,ptrSelectionObject);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      RETRY_SLEEP; 
      gl_rc=saEvtSelectionObjectGet(*ptrEvtHandle,ptrSelectionObject);
      if(gl_rc==SA_AIS_OK)
        printf("\n Selection Object Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtSelectionObjectGet() invoked",gl_rc);
 
  printf("\nSelection Object: %llu",*ptrSelectionObject);
}
void tet_saEvtDispatch(SaEvtHandleT *ptrEvtHandle)
{
  static int try_again_count;
  gl_rc=saEvtDispatch(*ptrEvtHandle,SA_DISPATCH_ONE);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      RETRY_SLEEP; 
      gl_rc=saEvtDispatch(*ptrEvtHandle,SA_DISPATCH_ONE);
      if(gl_rc==SA_AIS_OK)
        printf("\n Dispatch Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtDispatch() invoked with dispatch flags set to \
SA_DISPATCH_ONE",gl_rc);
}
void tet_saEvtFinalize(SaEvtHandleT *ptrEvtHandle)
{
  static int try_again_count;
  gl_rc=saEvtFinalize(*ptrEvtHandle);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Finalize API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtFinalize(*ptrEvtHandle);
      if(gl_rc==SA_AIS_OK)
        printf("\n Finalize Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtFinalize() invoked",gl_rc);
}
void tet_saEvtChannelOpen(SaEvtHandleT *ptrEvtHandle,
                          SaEvtChannelHandleT *ptrChannelHandle)
{
  static int try_again_count;
  gl_rc=saEvtChannelOpen(*ptrEvtHandle,&gl_channelName,gl_channelOpenFlags,
                         gl_timeout,ptrChannelHandle);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Open API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtChannelOpen(*ptrEvtHandle,&gl_channelName,gl_channelOpenFlags,
                             gl_timeout,ptrChannelHandle);
      if(gl_rc==SA_AIS_OK)
        printf("\n ChannelOpen Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtChannelOpen() invoked",gl_rc);

  printf("\nChannel Handle: %llu",*ptrChannelHandle);
}
void tet_saEvtChannelOpenAsync(SaEvtHandleT *ptrEvtHandle)
{
  static int try_again_count;
  gl_rc=saEvtChannelOpenAsync(*ptrEvtHandle,gl_invocation,&gl_channelName,
                              gl_channelOpenFlags);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Open Async API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtChannelOpenAsync(*ptrEvtHandle,gl_invocation,&gl_channelName,
                                  gl_channelOpenFlags);
      if(gl_rc==SA_AIS_OK)
        printf("\n ChannelOpenAsync Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtChannelOpenAsync() invoked",gl_rc);
}
void tet_saEvtChannelClose(SaEvtChannelHandleT *ptrChannelHandle)
{
  static int try_again_count;
  gl_rc=saEvtChannelClose(*ptrChannelHandle);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Close API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtChannelClose(*ptrChannelHandle);
      if(gl_rc==SA_AIS_OK)
        printf("\n ChannelClose Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtChannelClose() invoked for saEvtChannelOpen",gl_rc);
}
void tet_saEvtChannelUnlink(SaEvtHandleT *ptrEvtHandle)
{
  static int try_again_count;
  gl_rc=saEvtChannelUnlink(*ptrEvtHandle,&gl_channelName);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Unlink API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtChannelUnlink(*ptrEvtHandle,&gl_channelName);
      if(gl_rc==SA_AIS_OK)
        printf("\n ChannelUnlink Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtChannelUnlink() invoked with channel name",gl_rc);
}
void tet_saEvtEventAllocate(SaEvtChannelHandleT *ptrChannelHandle,
                            SaEvtEventHandleT *ptrEventHandle)
{
  static int try_again_count;
  gl_rc=saEvtEventAllocate(*ptrChannelHandle,ptrEventHandle);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Event Allocate API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtEventAllocate(*ptrChannelHandle,ptrEventHandle);
      if(gl_rc==SA_AIS_OK)
        printf("\n Event Allocate Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtEventAllocate() invoked",gl_rc);
}
void tet_saEvtEventFree(SaEvtEventHandleT *ptrEventHandle)
{
  static int try_again_count;
  gl_rc=saEvtEventFree(*ptrEventHandle);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Event Free API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtEventFree(*ptrEventHandle);
      if(gl_rc==SA_AIS_OK)
        printf("\n Event Free Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtEventFree() invoked",gl_rc);
}
void tet_saEvtEventAttributesGet(SaEvtEventHandleT *ptrEventHandle)
{
  static int try_again_count;
  int character,length=0,pCount=0;

  SaEvtEventPatternArrayT *ptrPatternArray=
    (SaEvtEventPatternArrayT *)malloc(sizeof(SaEvtEventPatternArrayT));

  memset(ptrPatternArray,'\0',(sizeof(SaEvtEventPatternArrayT)));

  SaNameT *getPublishName=(SaNameT *)malloc(sizeof(SaNameT));

  memset(getPublishName,'\0',(sizeof(SaNameT)));

  ptrPatternArray->allocatedNumber=gl_allocatedNumber;
  ptrPatternArray->patterns=
    (SaEvtEventPatternT *)malloc((sizeof(SaEvtEventPatternT))*gl_allocatedNumber);

  memset(ptrPatternArray->patterns,'\0',
               sizeof(SaEvtEventPatternT)*gl_allocatedNumber);

  printf("\n\n******************************\n");

  printf("\nEvent Patterns\n");

  while(pCount<gl_allocatedNumber)
    {
      ptrPatternArray->patterns[pCount].allocatedSize=gl_patternLength;
      ptrPatternArray->patterns[pCount].patternSize=gl_patternLength;
      ptrPatternArray->patterns[pCount].pattern=
        (SaUint8T *)malloc(gl_patternLength);

      memset(ptrPatternArray->patterns[pCount].pattern,'\0',
                   sizeof(gl_patternLength));
      pCount++;
    }

  gl_rc=saEvtEventAttributesGet(*ptrEventHandle,ptrPatternArray,&gl_priority,
                                &gl_retentionTime,getPublishName,
                                &gl_publishTime,&gl_evtId);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Attributes Get API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtEventAttributesGet(*ptrEventHandle,ptrPatternArray,
                                    &gl_priority,
                                    &gl_retentionTime,getPublishName,
                                    &gl_publishTime,&gl_evtId);
      if(gl_rc==SA_AIS_OK)
        printf("\n EventAttibute Get Try Again Count = %d\n",try_again_count);
    }
  /*resultSuccess("saEvtEventAttributesGet() invoked",gl_rc);*/

  if(gl_rc==1)
    {
      printf("\nNumber of patterns retrieved : %llu\n\n",
             ptrPatternArray->allocatedNumber);
      pCount=0;
      while(pCount<ptrPatternArray->allocatedNumber)
        {
          printf("Pattern Size: %llu\n",
                 ptrPatternArray->patterns[pCount].patternSize);
          character=ptrPatternArray->patterns[pCount].pattern[length];
          printf("Pattern: ");

          while(((character>=65&&character<=91)
                 ||(character>=97&&character<=123))
                &&length<ptrPatternArray->patterns[pCount].patternSize)
            {
              printf("%c",ptrPatternArray->patterns[pCount].pattern[length]);
              length++;
              if(length<ptrPatternArray->patterns[pCount].patternSize)
                {
                  character=ptrPatternArray->patterns[pCount].pattern[length];
                }
            }

          length=0;
          pCount++;
          printf("\n\n");
        }
    }

  printf("\nEvent Attributes retrieved : \n");
  printf("\nPriority: %d",gl_priority);
  printf("\nRetention Time: %llu",gl_retentionTime);
  printf("\nPublisher Name: %s",getPublishName->value);
  printf("\nPublish Time: %llu",gl_publishTime);
  printf("\nEvent Id: %llu\n",gl_evtId);
  printf("\n******************************\n"); 
  if(gl_b03_flag) 
  {
    gl_rc = saEvtEventPatternFree(*ptrEventHandle,ptrPatternArray->patterns);
    result("saEvtEventPatternFree() from a DeliverCallback()",
             SA_AIS_OK);
  }
}

void tet_saEvtEventAttributesSet(SaEvtEventHandleT *ptrEventHandle)
{
  static int try_again_count;
  gl_rc=saEvtEventAttributesSet(*ptrEventHandle,&gl_patternArray,gl_priority,
                                gl_retentionTime,&gl_publisherName);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Attributes Set API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtEventAttributesSet(*ptrEventHandle,&gl_patternArray,
                                    gl_priority,
                                    gl_retentionTime,&gl_publisherName);
      if(gl_rc==SA_AIS_OK)
        printf("\n EventAttibute Set Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtEventAttributesSet() invoked",gl_rc);

#if 0 
  tet_saEvtEventAttributesGet(ptrEventHandle);
#endif
}
void tet_saEvtEventDataGet(SaEvtEventHandleT *ptrEventHandle)
{
  static int try_again_count;
  SaSizeT tempDataSize;
 
  char gl_eventData[20];
 
  tempDataSize=gl_eventDataSize;
  gl_rc=saEvtEventDataGet(*ptrEventHandle,&gl_eventData,&tempDataSize);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Data Get API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtEventDataGet(*ptrEventHandle,&gl_eventData,&tempDataSize);
      if(gl_rc==SA_AIS_OK)
        printf("\n Event Data Get Try Again Count = %d\n",try_again_count);
    }
  printf("\nThe size of the data : %llu",tempDataSize);
  gl_eventDataSize=tempDataSize;
  resultSuccess("saEvtEventDataGet() invoked",gl_rc);
}
void tet_saEvtEventPublish(SaEvtEventHandleT *ptrEventHandle,
                           SaEvtEventIdT *ptrEvtId)
{
  static int try_again_count;
  gl_rc=saEvtEventPublish(*ptrEventHandle,gl_eventData,gl_eventDataSize,
                          ptrEvtId);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Publish API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtEventPublish(*ptrEventHandle,gl_eventData,gl_eventDataSize,
                              ptrEvtId);
      if(gl_rc==SA_AIS_OK)
        printf("\n Event Publish Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtEventPublish() invoked",gl_rc);

  printf("\nEvent Id (when published): %llu",*ptrEvtId);
   strcpy (gl_eventData,"");
}
void tet_saEvtEventSubscribe(SaEvtChannelHandleT *ptrChannelHandle,
                             SaEvtSubscriptionIdT *ptrSubId)
{
  static int try_again_count;
  int length=0;  
  gl_rc=saEvtEventSubscribe(*ptrChannelHandle,&gl_filterArray,*ptrSubId);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Subscribe API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtEventSubscribe(*ptrChannelHandle,&gl_filterArray,*ptrSubId);
      if(gl_rc==SA_AIS_OK)
        printf("\n Event Subscribe Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtEventSubscribe() invoked",gl_rc);

  printf("\n******************************\n");
  printf("\nEvent Filters\n");
  printf("\nNumber of filters: %llu\n",gl_filterArray.filtersNumber);

  while(length<gl_filterArray.filtersNumber)
    {
      printf("\nFilter Type: %d",gl_filterArray.filters[length].filterType);
      printf("\nFilter Size: %llu",
             gl_filterArray.filters[length].filter.patternSize);
      printf("\nFilter Pattern: %s",
             gl_filterArray.filters[length].filter.pattern);
      length++;
      printf("\n");
    }

  printf("\nSubscription Id: %u\n",*ptrSubId);
  printf("\n******************************\n");
}
void tet_saEvtEventUnsubscribe(SaEvtChannelHandleT *ptrChannelHandle)
{
  static int try_again_count;
  gl_rc=saEvtEventUnsubscribe(*ptrChannelHandle,gl_subscriptionId);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Unsubscribe API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtEventUnsubscribe(*ptrChannelHandle,gl_subscriptionId);
      if(gl_rc==SA_AIS_OK)
        printf("\n Event UnSubscribe Try Again Count = %d\n",try_again_count);
    }
  resultSuccess("saEvtEventUnsubscribe() invoked",gl_rc);
}
void tet_saEvtEventRetentionTimeClear(SaEvtChannelHandleT *ptrChannelHandle,
                                      SaEvtEventIdT *ptrEvtId)
{
  static int try_again_count;
  gl_rc=saEvtEventRetentionTimeClear(*ptrChannelHandle,*ptrEvtId);
  WHILE_TRY_AGAIN(gl_rc)
    {
      ++try_again_count;
      if(try_again_count==MAX_NUM_TRY_AGAINS)
      {
        printf("\n Retention Time Clear API Crossed Max Try Agains: exiting \n");
        break;
      }
      RETRY_SLEEP; 
      gl_rc=saEvtEventRetentionTimeClear(*ptrChannelHandle,*ptrEvtId);
      if(gl_rc==SA_AIS_OK)
        printf("\n Event Retention Time Try Again Count = %d\n",try_again_count);
    }
/*This condition can be removed*/
  if(gl_rc==SA_AIS_ERR_NOT_EXIST)
  {
    gl_rc=SA_AIS_OK;
  }
  resultSuccess("saEvtEventRetentionTimeClear() invoked",gl_rc);
}


/**************************************************************/
/******************** UTILITY FUNCTIONS **********************/
/**************************************************************/

void result(char *gl_saf_msg,SaAisErrorT ref)
{
  printf("\n\n\n%s",&gl_saf_msg[0]);
  tet_printf("%s",&gl_saf_msg[0]);
  if(gl_rc==ref)
    {
      printf("\n\nSUCCESS : ");
      tet_printf("Success : %s",gl_saf_error[gl_rc]);
      tet_result(TET_PASS);
    }
  else             
    {
      printf("\n\nFAILURE : ");
      tet_printf("Failure : %s",gl_saf_error[gl_rc]);
      tet_result(TET_FAIL); 
      gl_error=gl_rc;
    }
  printf("%s\n",gl_saf_error[gl_rc]);
}

void resultSuccess(char *gl_saf_msg,SaAisErrorT rc)
{
  if(iCmpCount!=8)
    {
      printf("\n----------------------------------------");
      printf("\n\n\n%s",&gl_saf_msg[0]);
      if(gl_cbk!=1)
        {
          tet_printf("%s",&gl_saf_msg[0]);
        }
      if(rc!=SA_AIS_OK)
        {
          printf("\n\nFAILURE : ");
          printf("%s\n",gl_saf_error[rc]);
          if(gl_cbk!=1)
            {
              tet_printf("Failure : %s",gl_saf_error[rc]);
              tet_result(TET_FAIL);
            }
          gl_err=rc; 
        }       
      else
        {
          printf("\n\nSUCCESS : ");
          printf("%s\n",gl_saf_error[rc]);
          if(gl_cbk!=1)
            {
              tet_printf("Success : %s",gl_saf_error[rc]);
              tet_result(TET_PASS);
            }
        }
      if(gl_error==1)
        {
          gl_error=rc;
        }
      printf("\n----------------------------------------");
    }
  gl_cbk=0;
}

void eda_selection_thread ()
{
  gl_jCount=0;

  gl_rc = saEvtSelectionObjectGet(gl_threadEvtHandle, &gl_selObject);
  WHILE_TRY_AGAIN(gl_rc)
    {
      RETRY_SLEEP; 
      gl_rc = saEvtSelectionObjectGet(gl_threadEvtHandle, &gl_selObject);
    }
  if (gl_rc != SA_AIS_OK)
    {
      printf("\nsaEvtSelectionObjectGet() failed");
      return;
    }

  while(1)
    {
      gl_threadEvtHandle=gl_evtHandle;
      gl_rc = saEvtDispatch(gl_threadEvtHandle,SA_DISPATCH_ALL);
      WHILE_TRY_AGAIN(gl_rc)
        {
          RETRY_SLEEP; 
          gl_rc = saEvtDispatch(gl_threadEvtHandle,SA_DISPATCH_ALL);
        }
      sleep(1);
    }
}

void thread_creation(SaEvtHandleT *ptrEvtHandle)
{
  gl_threadEvtHandle=*ptrEvtHandle;
  gl_tCount++;

  if(gl_tCount<2)
    {
      gl_rc = m_NCS_TASK_CREATE((NCS_OS_CB)eda_selection_thread,NULL,
                                "edsv_test", 8, 8000, &eda_thread_handle);
      if (gl_rc != NCSCC_RC_SUCCESS)
        {
          printf("\nthread cannot be created");
          return ;
        }
      printf("\nThread is created");
      gl_rc = m_NCS_TASK_START(eda_thread_handle);
      if (gl_rc != NCSCC_RC_SUCCESS)
        {
          printf("\nthread cannot be started");
          return ;
        }
    }
}
uint32_t tet_create_task(NCS_OS_CB task_startup)
{
  char taskname[]="My Task";
  if ( m_NCS_TASK_CREATE(task_startup,
                         NULL,
                         taskname,
                         NCS_TASK_PRIORITY_6,
                         NCS_STACKSIZE_MEDIUM,
                         &gl_t_handle)==NCSCC_RC_SUCCESS )
    return NCSCC_RC_SUCCESS;
  else
    return NCSCC_RC_FAILURE;
}

void var_initialize()
{
  gl_version.releaseCode='B';
  gl_version.majorVersion=1; 
  gl_version.minorVersion=1;
  gl_evtCallbacks.saEvtChannelOpenCallback=EvtOpenCallback;
  gl_evtCallbacks.saEvtEventDeliverCallback=EvtDeliverCallback; 
  gl_timeout=100000000000.0; 
  gl_error=1;
  gl_err=1;
  gl_cbk=0;
  iCmpCount=0;
  if(gl_listNumber>4) 
    {
      sleep(1);
    }
  gl_hide=0;  
  gl_publisherName.length=4;
  gl_channelName.length=7;
  memset(&gl_channelName.value,'\0',gl_channelName.length);
  memcpy(&gl_channelName.value,"channel",gl_channelName.length);
  gl_jCount=0;
}
