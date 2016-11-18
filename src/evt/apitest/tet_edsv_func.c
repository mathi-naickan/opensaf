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
int gl_b03_flag=0;
/***********************************************/
/*************Functionality Testing*************/
/***********************************************/

/*******Channel Open Functionality*******/

void tet_ChannelOpen_SingleEvtHandle()  
     /* Channel Open with same event handle single instance */ 
     /* Subscriber(3) - Publisher */
{
  char *tempData;
  SaSizeT tempDataSize;
  SaEvtChannelHandleT tempChannelHandle;
  SaEvtEventFilterT duplicate[2]=
    {
      {3,{5,5,(SaUint8T *)"Hello"}},
      {3,{4,4,(SaUint8T *)"Moto"}}
    };

  printf("\n\n*******Case 1: Channel Open with single event handle********");
  printf("\n\n******Subscriber******");
  tet_printf(" F:1: Channel Open with Single Event Handle ");
  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  printf("\n\nChannel opened for subscriber");
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
  
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  gl_filterArray.filtersNumber=2;
  gl_filterArray.filters=duplicate;
  gl_subscriptionId=5;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
  tempChannelHandle=gl_channelHandle;

  printf("\n\n******Publisher******");
  printf("\n\nChannel opened for publisher");

  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER|SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);
   
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);

   strcpy (gl_eventData,"Publish Case 1");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    
 
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();
        
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);       

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&tempChannelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);
                
  tet_saEvtFinalize(&gl_evtHandle);

  if(gl_jCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to the subscriber");
      tet_infoline("The event has been delivered to the subscriber");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to the subscriber");
          tet_infoline("The event has not been delivered to the subscriber");
          tet_result(TET_FAIL);
        }
      else   
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\n*************Case 1: End*************");
   
  
    
  
}

void tet_ChannelOpen_SingleEvtHandle_SamePatterns()  
     /* Channel Open with same event handle multiple instance, same patterns,
        same subscription */
     /* Subscriber(3) - Publisher - Subscriber(3) - Subscribe(4) */
{
  SaEvtChannelHandleT pubChannelHandle,subChannelHandle;
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;  

  printf("\n\n******Case 2: Channel Open with single event handle multiple\
 instance***");
  printf("\n\n******Subscriber******");
  tet_printf(" F:2: Channel Open with Single Event Handle and Subscriptions\
with Same Patterns");
        
  var_initialize();
  
  tet_saEvtInitialize(&gl_evtHandle); 

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  printf("\nChannel opened for subscriber");
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
                                             
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
        
  printf("\n\n******Publisher******");
  printf("\n\nChannel opened for publisher");
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&pubChannelHandle);

  tet_saEvtEventAllocate(&pubChannelHandle,&gl_eventHandle);
        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=100000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   

  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    
  strcpy (gl_eventData,"Publish Case 2");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }

  printf("\n******Subscriber******");
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&subChannelHandle);
          
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&subChannelHandle,&gl_subscriptionId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
   DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }
  printf("\nComparison Count : %d",iCmpCount);

  printf("\n NumUsers= 3 : NumSubscribers= 2 : NumPublishers= 1 \n");
  tet_saEvtEventRetentionTimeClear(&pubChannelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&subChannelHandle);

  tet_saEvtChannelClose(&pubChannelHandle);  

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==2&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscribers");
      tet_printf("The event has been delivered to subscribers");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscribers");
          tet_printf("The event has not been delivered to subscribers");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 2: End*************");
   
  
    
  
}

void tet_ChannelOpen_SingleEvtHandle_DifferentPatterns()  
     /* Channel Open with same event handle multiple instance different patterns */
     /* Subscriber(3) - Publisher - Subscribe(4) */
{
  SaEvtEventFilterT duplicate[2]=
    {
      {3,{5,5,(SaUint8T *)"Hello"}},        
      {3,{4,4,(SaUint8T *)"Moto"}}
    };
  char *tempData;
  SaEvtChannelHandleT pubChannelHandle,subChannelHandle;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  printf("\n\n*************Case 3: Channel Open with multiple instance*****");
  printf("\n\n******Subscriber******");
  tet_printf(" F:3: Channel Open with Single Event Handle and Subscriptions\
with Different Patterns");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);
        
  tet_saEvtSelectionObjectGet(&gl_evtHandle,&gl_selObject);
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER; 
  printf("\nChannel opened for subscriber");
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
  
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  printf("\n\n******Publisher******");
  printf("\n\nChannel opened for publisher");

  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&pubChannelHandle);
   

  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

  tet_saEvtEventAllocate(&pubChannelHandle,&gl_eventHandle);
                        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=100000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 3");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
        
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);

  gl_allocatedNumber=2;
  gl_patternLength=5;
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();  

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {          
      iCmpCount++;
    }

  printf("\n\n******Subscriber******");

  gl_channelOpenFlags=SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&subChannelHandle);

  gl_subscriptionId=4;
  gl_filterArray.filtersNumber=2;
  gl_filterArray.filters=duplicate;
  tet_saEvtEventSubscribe(&subChannelHandle,&gl_subscriptionId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
   DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }
  printf("Comparison Count : %d",iCmpCount);

  printf("\n NumUsers= 3 : NumSubscribers= 2 : NumPublishers= 1 \n");
  tet_saEvtEventRetentionTimeClear(&pubChannelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);
        
  tet_saEvtChannelClose(&gl_channelHandle);                

  tet_saEvtChannelClose(&pubChannelHandle);

  tet_saEvtChannelClose(&subChannelHandle); 

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==2&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscribers");
      tet_printf("The event has been delivered to subscribers");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscribers");
          tet_infoline("The event has not been delivered to subscribers");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 3: End*************");
   
  
    
  
}

void tet_ChannelOpen_MultipleEvtHandle()  
     /* Channel Open with multiple event handles */
     /* Subscriber(I1,3) - Publisher(I2)*/
{
  char *tempData;
  SaSizeT tempDataSize;

  SaEvtHandleT tempEvtHandle,temp1EvtHandle;
  SaEvtChannelHandleT tempChannelHandle;
  SaEvtEventHandleT tempEventHandle;

  printf("\n\n********Case 4: Channel Open with multiple event handles****");
  printf("\n\nFirst gl_evtHandle");
  tet_printf(" F:4: Channel Open with Multiple Event Handle ");

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);
  temp1EvtHandle=gl_evtHandle;
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;

  printf("\nSubscriber");
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  printf("\nSecond gl_evtHandle");
  
  tet_saEvtInitialize(&tempEvtHandle);
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&tempEvtHandle,&tempChannelHandle);
   

  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

  tet_saEvtEventAllocate(&tempChannelHandle,&tempEventHandle);
                           
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&tempEventHandle);
   strcpy (gl_eventData,"Publish Case 4"); 
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  printf("\nPublisher"); 
  tet_saEvtEventPublish(&tempEventHandle,&gl_evtId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
  gl_evtHandle=temp1EvtHandle;
  gl_threadEvtHandle=gl_evtHandle; 

  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  printf("\n for first gl_evtHandle");
  printf("\nchannel handle: %llu",gl_channelHandle);
  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);
 
  tet_saEvtFinalize(&gl_evtHandle);


  printf("\nfor second gl_evtHandle");

  tet_saEvtEventRetentionTimeClear(&tempChannelHandle,&gl_evtId);

  tet_saEvtEventFree(&tempEventHandle);

  tet_saEvtChannelClose(&tempChannelHandle);

  tet_saEvtFinalize(&tempEvtHandle);

  if(gl_jCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to the subscriber");
      tet_infoline("The event has been delivered to the subscriber");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1&&gl_jCount!=1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to the subscriber");
          tet_infoline("The event has not been delivered to the subscriber");
          tet_result(TET_FAIL);
        }
      else   
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 4: End*************");
   
  
    
  
}

void tet_ChannelOpen_MultipleEvtHandle_SamePatterns() 
     /* Channel Open with multiple event handles mulitple instance same patterns*/
     /* Subscriber(I1,3) - Publisher(I2) - Subscriber(I1,4) */
{
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  SaEvtHandleT tempEvtHandle,temp1EvtHandle;
  SaEvtChannelHandleT tempChannelHandle;
  SaEvtEventHandleT tempEventHandle;

  printf("\n\n*************Case 5: Channel Open with multiple handles with\
 multiple instances*************");
  printf("\nFirst gl_evtHandle");
  tet_printf(" F:5: Channel Open with Multiple Event Handles and Subscriptions\
with Same Patterns");

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);
  temp1EvtHandle=gl_evtHandle;
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
   
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  printf("\nSubscriber");
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  printf("\nSecond gl_evtHandle");
  
  tet_saEvtInitialize(&tempEvtHandle);
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&tempEvtHandle,&tempChannelHandle);
  
  tet_saEvtEventAllocate(&tempChannelHandle,&tempEventHandle);
   

  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

                           
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&tempEventHandle);
   strcpy (gl_eventData,"Publish Case 5");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
        
  printf("\nPublisher");
  tet_saEvtEventPublish(&tempEventHandle,&gl_evtId);

  gl_allocatedNumber=2;
  gl_patternLength=5;
  gl_evtHandle=temp1EvtHandle;
  gl_threadEvtHandle=gl_evtHandle;

  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  printf("\nSubscriber");
  
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  gl_subscriptionId=5;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
  

  gl_allocatedNumber=2;
  gl_patternLength=5;
   DISPATCH_SLEEP();
  
  printf("Comparison Count : %d",iCmpCount);
  
  printf("\n for first gl_evtHandle");

  tet_saEvtChannelClose(&gl_channelHandle);

  gl_evtHandle=temp1EvtHandle;
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);
  
  printf("\nfor second gl_evtHandle");

  tet_saEvtEventRetentionTimeClear(&tempChannelHandle,&gl_evtId);

  tet_saEvtEventFree(&tempEventHandle);

  tet_saEvtChannelClose(&tempChannelHandle);

  tet_saEvtFinalize(&tempEvtHandle);

  if(gl_jCount==2&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscribers");
      tet_printf("The event has been delivered to subscribers");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1&&gl_jCount!=2)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscribers");
          tet_printf("The event has not been delivered to subscribers");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 5: End*************");
   
  
    
                           
}

void tet_ChannelOpen_MultipleEvtHandle_DifferentPatterns()  
     /* Channel Open with multiple event handles mulitple instance different
        patterns*/
     /* Subscriber(I1,3) - Publisher(I2) - Subscriber(I1,4) */
{
  SaEvtEventFilterT duplicate[2]=
    {
      {3,{5,5,(SaUint8T *)"Hello"}},        
      {3,{4,4,(SaUint8T *)"Moto"}}
    };
  char *tempData;
  SaSizeT tempDataSize;

  SaEvtHandleT tempEvtHandle,localEvtHandle;
  SaEvtChannelHandleT tempChannelHandle;
  SaEvtEventHandleT tempEventHandle;

  printf("\n\n*************Case 6: Channel Open with multiple handles and\
 different patterns************");
  printf("\nFirst gl_evtHandle");
  tet_printf(" F:6: Channel Open with Multiple Event Handles and Subscriptions\
with Different Patterns");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);
  localEvtHandle=gl_evtHandle;
   
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=3;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  printf("\nSubscriber");
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  printf("\nSecond gl_evtHandle");
  
  tet_saEvtInitialize(&tempEvtHandle);
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_SUBSCRIBER|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&tempEvtHandle,&tempChannelHandle);
  
  tet_saEvtEventAllocate(&tempChannelHandle,&tempEventHandle);
                           
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=1000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&tempEventHandle);
   

  printf("\n NumUsers= 2 : NumSubscribers= 2 : NumPublishers= 1 \n");
  
    

  strcpy (gl_eventData,"Publish Case 6");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  printf("\nPublisher");
  tet_saEvtEventPublish(&tempEventHandle,&gl_evtId);

  gl_allocatedNumber=2;
  gl_patternLength=5;
  gl_evtHandle=localEvtHandle;
  gl_threadEvtHandle=gl_evtHandle;
  thread_creation(&gl_evtHandle);
  gl_tCount--;
   DISPATCH_SLEEP();

  printf("\nSubscriber");
  
  gl_subscriptionId=5;
  gl_filterArray.filtersNumber=2;
  gl_filterArray.filters=duplicate;
  tet_saEvtEventSubscribe(&tempChannelHandle,&gl_subscriptionId);
 
  gl_allocatedNumber=2;
  gl_patternLength=5; 
  gl_evtHandle=tempEvtHandle;
  gl_threadEvtHandle=gl_evtHandle;

  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  printf("\n for first gl_evtHandle");
 
  gl_evtHandle=localEvtHandle;
  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);
  
  printf("\nfor second gl_evtHandle");

  tet_saEvtEventRetentionTimeClear(&tempChannelHandle,&gl_evtId);

  tet_saEvtEventFree(&tempEventHandle);

  tet_saEvtChannelClose(&tempChannelHandle);

  tet_saEvtFinalize(&tempEvtHandle);

  if(gl_jCount<=2&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscribers");
      tet_printf("The event has been delivered to subscribers");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1&&gl_jCount!=2)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscribers");
          tet_printf("The event has not been delivered to subscribers");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
    
  printf("\n\n*************Case 6: End*************");
    
  
    

}

void tet_ChannelOpen_MultipleOpen()   
     /* Channel Open with mulitple openings for same channel name */
{
  int i=0;
  SaEvtChannelHandleT tempChannelHandle;
  printf("\n\n************Case 7: Multiple Channel Open*************");
  tet_printf(" F:7: Open Multiple Event Channels");
  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle); 
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  if(gl_rc==SA_AIS_OK)
    {
      i++;
    }

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&tempChannelHandle);

  if(gl_rc==SA_AIS_OK)
    {
      i++;
    }
    

  printf("\n NumUsers= 2 : NumSubscribers= 0 : NumPublishers= 0 \n");
  
    



  printf("Count : %d",gl_jCount);
  
  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&tempChannelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);
  
  tet_saEvtFinalize(&gl_evtHandle);

  if(i==2&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe channel has been opened multiple times");
      tet_printf("The channel has been opened multiple times");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
        
          printf("\nTest Case: FAILED");      
          printf("\nThe channel cannot be opened multiple times");
          tet_printf("The channel cannot be opened multiple times");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 7: End*************");
    
  
    

}

void tet_ChannelClose_Simple()   /* Channel Close  simple case */
{
  int iCmpCount=0,temp;
  SaEvtChannelHandleT pubChannelHandle;
  printf("\n\n*************Case 8: Channel Close*************");
  tet_printf(" F:8: Close an Opened Event Channel");

  char *tempData;
  SaSizeT tempDataSize;

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
        
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&pubChannelHandle);
        
  tet_saEvtEventAllocate(&pubChannelHandle,&gl_eventHandle);
        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 8");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
 
  gl_allocatedNumber=2;
  gl_patternLength=5; 
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }

  tet_saEvtEventRetentionTimeClear(&pubChannelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);
        
  tet_saEvtChannelClose(&gl_channelHandle);
    

  printf("\n NumUsers= 1 : NumSubscribers= 0 : NumPublishers= 1 \n");
  
    



  tet_saEvtChannelClose(&pubChannelHandle);
  temp=gl_rc;

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);        

  if(iCmpCount==1&&temp==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered and channel has been closed");
      tet_printf("The event has been delivered and channel has been closed");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          if(temp!=SA_AIS_OK)
            {
              printf("\nTest Case: FAILED");      
              printf("\nThe channel cannot be closed");
              tet_printf("The channel cannot be closed");
              tet_result(TET_FAIL);
            }
          else if(iCmpCount!=1)
            {
              printf("\nTest Case: FAILED");
              printf("\nThe event has not been delivered to the subscriber");
              tet_printf("The event has not been delivered to subscriber");
              tet_result(TET_FAIL);
            }
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 8: End*************");
    
  
    

}
        
void tet_ChannelClose_Subscriber()   
     /* Channel Close with subscriber and publisher after close */
{
  int i,j;
  
  var_initialize();
  printf("\n\n********Case 9: Channel Close with subscribe afterwards*******");
  tet_printf(" F:9: Not able to Subscribe after the Channel is Closed");

  tet_saEvtInitialize(&gl_evtHandle);
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
        
  tet_saEvtChannelClose(&gl_channelHandle);
  i=gl_rc;
    

  printf("\n NumUsers= 0 : NumSubscribers= 0 : NumPublishers= 0 \n");
  
    

  
  j=saEvtEventSubscribe(gl_channelHandle,&gl_filterArray,gl_subscriptionId);
  if(j!=SA_AIS_OK)
    {
      gl_error=SA_AIS_OK; 
    }
  

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==SA_AIS_OK&&j!=SA_AIS_OK&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe channel has been closed and channel subscribe cannot be\
 invoked");
      tet_printf("\nThe channel has been closed and channel subscribe cannot\
 be invoked");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          if(i!=SA_AIS_OK)
            {
              printf("\nTest Case: FAILED");      
              printf("\nThe channel cannot be closed");
              tet_printf("The channel cannot be closed");
              tet_result(TET_FAIL);
            }
          else if(j==SA_AIS_OK)
            {
              printf("\nTest Case: FAILED");
              printf("\nThe channel subscribe going through successfully"); 
              tet_printf("\nThe channel subscribe going through successfully");
              tet_result(TET_FAIL);
            }
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 9: End*************");
    
  
    
  
}

void tet_ChannelUnlink_Simple()   /* Channel Unlink simple case */
{
  
  var_initialize();
  printf("\n\n*************Case 10: Channel Unlink*************");
  tet_printf(" F:10: Channel Unlink Functionality");

  tet_saEvtInitialize(&gl_evtHandle);
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
        
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);
        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 10");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
 
  gl_allocatedNumber=2;
  gl_patternLength=5;
    
  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);
    

  tet_saEvtFinalize(&gl_evtHandle);

  if(gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe channel unlink invoked successfully");
      tet_printf("The channel unlink invoked successfully");
      tet_result(TET_PASS);
    }
  else 
    {
      printf("\nTest Case: UNRESOLVED");
      printf("\nAPI call has failed");
      tet_printf("\nTest Case: UNRESOLVED");
      tet_infoline("API call has failed");
      tet_result(TET_UNRESOLVED);
    }
  printf("\n\n*************Case 10: End*************");
    
  
    
  
}

void tet_ChannelUnlink_OpenInstance() 
     /* Channel Unlink being invoked with an existing open instance */
{
  SaEvtChannelHandleT pubChannelHandle,tempChannelHandle;
  
  int i;
  var_initialize();

  printf("\n\n********Case 11: Channel Unlink with an open instance********");
  tet_printf(" F:11: Channel Unlink invoked with an Open Channel instance");

  tet_saEvtInitialize(&gl_evtHandle);
          
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&tempChannelHandle);

  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 0 \n");
  tet_saEvtChannelUnlink(&gl_evtHandle);
  i=gl_rc;
    

  printf("\n No Rows Selected \n");
  
    



  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&pubChannelHandle);
                                           
  tet_saEvtChannelClose(&pubChannelHandle);

  tet_saEvtChannelClose(&tempChannelHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==SA_AIS_OK&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nChannel unlink invoked successfully");
      tet_printf("Channel unlink invoked successfully");
      tet_result(TET_PASS);
    }
  else 
    {
      if(i!=1)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event is not delivered to the subscriber");
          tet_printf("\nThe event is not delivered to the subscriber"); 
          tet_result(TET_FAIL);
        }
      else
        {     
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 11: End*************");
    
  
    
  
}

void tet_EventFree_Simple()   /* Event Free */
{
  int i;
  printf("\n\n*************Case 12: Event Free*************");
  tet_printf(" F:12: Event Free Functionality");
  var_initialize();
  
  tet_saEvtInitialize(&gl_evtHandle);
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=1000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
    

  printf("\n NumUsers= 1 : NumSubscribers= 0 : NumPublishers= 1 \n");
  
    

        
  tet_saEvtEventFree(&gl_eventHandle);
  i=gl_rc;

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==SA_AIS_OK&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe memory allocated to event channel is freed");
      tet_printf("\nThe memory allocated to event channel is freed");
      tet_result(TET_PASS);
    }
  else 
    {
      if(i!=SA_AIS_OK)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe memort allocated to event channel is not freed");
          tet_printf("\nThe memort allocated to event channel is not freed");
          tet_result(TET_FAIL);
        }
      else 
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\n*************Case 12: End*************");
    
  
    
  
}

void tet_AttributesSet_DeliverCallback()  
     /* Attributes Set in the deliver callback function */
{
  int i;
  SaEvtChannelHandleT channelPubHandle;
  printf("\n\n****Case 13: Attributes Set in deliver callback function*****");
  tet_printf(" F:13: Attributes Set in the Deliver callback function");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
        
  gl_channelOpenFlags=1;
  tet_saEvtChannelOpen(&gl_evtHandle,&channelPubHandle);
        
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
        
  tet_saEvtEventAllocate(&channelPubHandle,&gl_eventHandle);
        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
    

  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 2 \n");
  
    

   strcpy (gl_eventData,"Publish Case 13"); 
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;

  thread_creation(&gl_evtHandle); 
  DISPATCH_SLEEP();

  
  gl_allocatedNumber=2;
  gl_patternLength=5;
  
  i=saEvtEventAttributesSet(gl_eventDeliverHandle,&gl_patternArray,gl_priority,
                            gl_retentionTime,&gl_publisherName);
  

  tet_saEvtEventRetentionTimeClear(&channelPubHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&channelPubHandle);
        
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);        
  
  if(i==SA_AIS_ERR_ACCESS&&gl_error==1) 
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event attributes cannot be set in the deliver callback");
      tet_printf("\nThe event attributes cannot be set in the deliver\
 callback");
      tet_result(TET_PASS);
    }
  else
    {
      if(i!=SA_AIS_ERR_ACCESS)
        {
          printf("\nTest Case: FAILED"); 
          printf("\nThe event attributes are being set in the deliver \
callback");
          tet_printf("\nThe event attributes are being set in the deliver\
 callback");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 13: End*************");
    
  
    
  
}

void tet_AttributesGet_ReceivedEvent()   
     /* Attributes Get for a received event */
{
  int iCmpCount=0;
  char *tempData;
  SaSizeT tempDataSize;
  printf("\n\n********Case 14: Attributes Get for recevied event**********");
  tet_printf(" F:14: Attributes Get for a Received Event");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;

  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

   strcpy (gl_eventData,"Publish Case 14");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize; 
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }
  printf("\n\nComparison Count: %d",iCmpCount);

  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscriber and attributes get \
used in event deliver callback");
      tet_printf("\nThe event has been delivered to subscriber and attributes \
get used in event deliver callback");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscriber");
          tet_printf("\nThe event has not been delivered to subscriber");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 14 End*************");
    
  
    
  
}


void tet_PatternFree_ReceivedEvent()
     /* Patternfree for a received event */
{
  int iCmpCount=0;
  char *tempData;
  SaSizeT tempDataSize;
  tet_printf(" PatternsFree for a Received Event");
  gl_b03_flag=1;
  
  var_initialize();
  gl_evtCallbacks.saEvtEventDeliverCallback=b03EvtDeliverCallback; 
  
  tet_saEvtInitialize(&gl_evtHandle);
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;

  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

   strcpy (gl_eventData,"Publish Case 14");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize; 
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }
  printf("\n\nComparison Count: %d",iCmpCount);

  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscriber and attributes get \
used in event deliver callback");
      tet_printf("\nThe event has been delivered to subscriber and attributes \
get used in event deliver callback");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscriber");
          tet_printf("\nThe event has not been delivered to subscriber");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  gl_b03_flag=0;
  printf("\n\n*************Case 14 End*************");
  
}



void tet_EventPublish_NullPatternArray()  
     /* Event Publish for Null Pattern Array */
{
  int i;
  printf("\n\n*******Case 15: Event Publish for NULL pattern array********");
  tet_printf(" F:15: Publish an Event with NULL pattern array");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);        

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_retentionTime=10000000000000.0;
  gl_allocatedNumber=0;
  gl_rc=saEvtEventAttributesSet(gl_eventHandle,NULL,gl_priority,
                                gl_retentionTime,&gl_publisherName);
  resultSuccess("saEvtEventAttributesSet() invoked",gl_rc);     

    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    
  
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  i=gl_rc;

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);
  
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);
 
  if(i==SA_AIS_OK&&gl_error==1)
    { 
      printf("\nTest Case: PASSED");
      printf("\nEvent published with NULL pattern array");
      tet_printf("\nEvent published with NULL pattern array");
      tet_result(TET_PASS);
    }
  else
    {
      if(i!=SA_AIS_OK)
        {
          printf("\nTest Case: FAILED");
          printf("\nEvent cannot be published NULL pattern array");
          tet_printf("\nEvent cannot be published with NULL pattern array");
          tet_result(TET_PASS);
        }
      else if (gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 15: End*************");
    
  
    
  
}

void tet_EventSubscribe_PrefixFilter()  
     /* Event Subscribe with prefix filter */
{
  SaEvtEventFilterT prefixFilter[1]=
    {
      {1,{3,3,(SaUint8T *)"Hel"}}
    };
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  printf("\n\n*********Case 16: Event Subscribe with prefix filter*********");
  tet_printf(" F:16: Event Subscription with PREFIX Filter");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
  
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=prefixFilter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
  

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 16");      
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_dupSubscriptionId==gl_subscriptionId)
    {
      iCmpCount++;
    }
  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscriber, event subscribe\
 prefix filter");
      tet_printf("\nThe event has been delivered to subscriber, event \
subscribe prefix filter");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscriber");
          tet_printf("The event has not been delivered to subscriber");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 16: End*************");
  
  
    

}


void sleep_for_dispatch()
{
  sleep(5);
  fflush(stdout);  
}
void tet_EventSubscribe_SuffixFilter()  
     /* Event Subscribe with suffix filter */
{
  SaEvtEventFilterT suffixFilter[1]=
    {
      {2,{3,3,(SaUint8T *) "llo"}}
    };

  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  printf("\n\n*******Case 17: Event Subscribe with suffix filter**********");
  tet_printf(" F:17: Event Subscription with SUFFIX Filter");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=4;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=suffixFilter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
  

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    


        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 17");      
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  printf("\n\t"); 
  fflush(stdout);  
  
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP(); 

  if(gl_eventDataSize==tempDataSize&&gl_dupSubscriptionId==gl_subscriptionId)
    {
      iCmpCount++;
    }

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscriber, event subscribe \
suffix filter");
      tet_printf("\nThe event has been delivered to subscriber, event \
subscribe suffix filter");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscriber");
          tet_printf("The event has not been delivered to subscriber");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 17: End*************");
  
  
    

}

void tet_EventSubscribe_ExactFilter()   /* Event Subscribe with exact filter */
{

  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  printf("\n\n*************Case 18: Event Subscribe with exact filter*******");

  tet_printf(" F:18: Event Subscription with EXACT Filter");
  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=5;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
  

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    


        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 18");      
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);

  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP(); 

  if(gl_eventDataSize==tempDataSize&&gl_dupSubscriptionId==gl_subscriptionId)
    {
      iCmpCount++;
    }
  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscriber, event subscribe \
exact filter");
      tet_printf("\nThe event has been delivered to subscriber, event \
subscribe exact filter");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscriber");
          tet_printf("The event has not been delivered to subscriber");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 18: End*************");
  
  
    

}

void tet_EventSubscribe_AllPassFilter()   /* Event Subscribe with all pass \
                                             filter */
{
  SaEvtEventFilterT allPassFilter[1]=
    {
      {4,{1,1,(SaUint8T *) "A"}}
    };

  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount;

  printf("\n\n*******Case 19: Event Subscribe with all pass filter*********");
  tet_printf(" F:19: Event Subscription with ALLPASS Filter");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=6;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=allPassFilter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
  

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);        
   strcpy (gl_eventData,"Publish Case 19");      
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);

  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_dupSubscriptionId==gl_subscriptionId)
    {
      iCmpCount++;
    }
  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscriber, event subscribe \
all pass filter");
      tet_printf("\nThe event has been delivered to subscriber, event \
subscribe all pass filter");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscriber");
          tet_printf("The event has not been delivered to subscriber");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 19: End*************");
  
  
    
        
}

void tet_EventSubscribe_LessFilters()  
     /* Event Subscribe with less filters than patterns */
{
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  printf("\n\n****Case 20: Event Subscribe with less filters than patterns*");
  tet_printf(" F:20: Event Subscription with Less Filters than Patterns");
  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);             

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=7;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
  

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

        
        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 20");      
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_dupSubscriptionId==gl_subscriptionId)
    {
      iCmpCount++;
    }

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscriber, event subscribe \
with less filters than patterns");
      tet_printf("\nThe event has been delivered to subscriber, event \
subscribe with less filters than patterns");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscriber");
          tet_printf("The event has not been delivered to subscriber");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 20: End *************");
    

  
    
}

void tet_EventSubscribe_LessPatterns()  
     /* Event Subscribe with less patterns than filters */
{
  SaEvtEventFilterT moreFilters[2]=
    {
      {4,{5,5,(SaUint8T *)"Hello"}},
      {4,{4,4,(SaUint8T *)"Hell"}},
    };
  SaEvtEventPatternT patternL[1]=
    {
      {5,5,(SaUint8T *)"Hello"},
    };
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  printf("\n\n*************Case 21: Event Subscribe with less pattern than \
filter*************");
  tet_printf(" F:21: Event Subscription with Less Patterns than Filters");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=8;
  gl_filterArray.filtersNumber=2;
  gl_filterArray.filters=moreFilters;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=1;
  gl_patternArray.patterns=patternL;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=1;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
        
   strcpy (gl_eventData,"Publish Case 21");      
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_dupSubscriptionId==gl_subscriptionId)
    {
      iCmpCount++;
    }
  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscriber, event subscribe\
 with less patterns than filters");
      tet_printf("\nThe event has been delivered to subscriber, event \
subscribe with less patterns than filters");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscriber");
          tet_printf("The event has not been delivered to subscriber");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 21: End*************");
    
  
    
        
}

void tet_EventSubscribe_LessPatterns_FilterSize()   
     /* Event Subscribe with less patterns than filters with filter size 0 */
{
  SaEvtEventFilterT moreFiltersSize[2]=
    {
      {4,{5,5,(SaUint8T *)"Hello"}},
      {3,{0,0,(SaUint8T *)""}},
    };
  SaEvtEventPatternT patternL[1]=
    {
      {5,5,(SaUint8T *)"Hello"},
    };
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  printf("\n\nCase 22: Event Subscribe with less patterns than filers and \
filter size is 0*************");
  tet_printf(" F:22: Event Subscription with Less Patterns than Filters \
Filter Size being set to 0");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=8;
  gl_filterArray.filtersNumber=2;
  gl_filterArray.filters=moreFiltersSize;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

        
        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=1;
  gl_patternArray.patterns=patternL;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=1;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 22");      
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_dupSubscriptionId==gl_subscriptionId)
    {
      iCmpCount++;
    }

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscriber, event subscribe\
 with less patterns than filters, filter size 0");
      tet_printf("\nThe event has been delivered to subscriber, event \
subscribe with less patterns than filters, filter size 0");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscriber");
          tet_printf("The event has not been delivered to subscriber");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 22: End*************");
    
  
    
        
}

void tet_EventSubscribe_LessFilters_DifferentOrder()   
     /* Event Subscribe with less filters than patterns in different order */
{
  SaEvtEventFilterT filterDup[1]=
    {
      {3,{4,4,(SaUint8T *)"Hell"}}
    };
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  printf("\n\n*************Case 23: Event Subscribe with less filter than \
patterns in different order*************");
  tet_printf(" F:23: Event Subscription with Less Filters than Patterns\
and Filters doesn't match Published Patterns");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=9;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=filterDup;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

        
        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 23"); 
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_dupSubscriptionId==gl_subscriptionId)
    {
      iCmpCount++;
    }

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==0&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nNo callback received for event subscribe with different \
filters");
      tet_printf("\nNo callback received for event subscribe with different \
filters");
      tet_result(TET_PASS);
    } 
  else
    {
      if(iCmpCount!=0)
        {
          printf("\nTest Case: FAILED");
          printf("\nCallback received for event subscribe with different \
filters");
          tet_printf("\nCallback received for event subscribe with different \
filters");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 23: End*************");
    
  
    
        
}

void tet_EventSubscribe_PatternMatch_DifferentSubscriptions()  
     /* Event Subscribe with pattern match for different subscriptions*/
{
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  printf("\n\n*************Case 24: Event Subscribe with pattern match for\
 different subscription*************");
  tet_printf(" F:24: Event Subscription with Different Subscription id's\
and Filters match Published Patterns");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=10;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  gl_subscriptionId=11;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

        

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 24");      
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
    
  thread_creation(&gl_evtHandle);
  sleep(1);

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }

  gl_subscriptionId=12;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    
        
  
   DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }
  printf("Comparison Count : %d",iCmpCount);

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);
 
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(gl_jCount==2&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event delivered to the subscriber");
      tet_printf("The event delivered to the subscriber");
      tet_result(TET_PASS);
    } 
  else
    {
      if(gl_jCount!=2)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event is not delivered to the subscriber");
          tet_printf("The event is not delivered to the subscriber");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 24: End*************");
    
        
  
    
}

void tet_ChannelOpenAsync_Simple()   /* Channel Open Async Cases */
{
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  printf("\n\n*************Case 25: Channel Open Async Cases*************");
  tet_printf(" F:25: Channel Open using OpenAsync "); 

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpenAsync(&gl_evtHandle);

  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    



  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_retentionTime=10000000000.0;
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 25");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_dupSubscriptionId==gl_subscriptionId)
    {
      iCmpCount++;
    }

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event delivered to the subscriber,channel open async");
      tet_printf("The event delivered to the subscriber, channel open async");
      tet_result(TET_PASS);
    } 
  else
    {
      if(iCmpCount!=2)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event is not delivered to the subscriber");
          tet_printf("The event is not delivered to the subscriber");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 25: End*************");
    

  
    
}

void tet_EventSubscribe_multipleFilters()  
     /* Event Subscribe with mutliple filters with patternsize lessthan filtersize*/
{ 
  int iCmpCount=0;
  char *tempData;
  SaSizeT tempDataSize;


  SaEvtEventPatternT multiplePattern[5]=
    {
      {5,5,(SaUint8T *)"Hello"},
      {4,4,(SaUint8T *)"Hell"},
      {4,4,(SaUint8T *)"Test"},
      {4,4,(SaUint8T *)"News"},
      {4,4,(SaUint8T *)"Moto"},
    };
  SaEvtEventFilterT multipleFilter[5]=
    {
      {3,{5,5,(SaUint8T *)"Hello"}},
      {3,{4,4,(SaUint8T *)"Hell"}},
      {3,{4,4,(SaUint8T *)"Test"}},
      {3,{4,4,(SaUint8T *)"News"}},
      {3,{4,4,(SaUint8T *)"Moto"}},
    };
    
  printf("\n\n*************Case 26: Event Subscribe with multiple filters with\
 pattern size less than filter size*************");
  tet_printf(" F:26: Event Subscription with Mutliple Filters \
where PatternSize lessthan FilterSize "); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=5;
  gl_filterArray.filters=multipleFilter;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    



  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=5;
  gl_patternArray.patterns=multiplePattern;
  gl_retentionTime=1000000000.0;
  gl_allocatedNumber=5;
  gl_patternLength=6;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 26");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();
  /* Retained Event is showing = 0
     printf("\n RetainedEvent = 1 \n");
     if(gl_redundancy==EDA_SNMP)
     
  */
  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);
 
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event delivered to the subscriber, multiple filters");
      tet_printf("The event delivered to the subscriber, multiple filters");
      tet_result(TET_PASS);
    } 
  else
    {
      if(iCmpCount!=1)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event is not delivered to the subscriber");
          tet_printf("The event is not delivered to the subscriber");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 26: End*************");
    

  
    

}

void tet_EventSubscribe_PrefixFilter_NoMatch() 
     /* Event Subscribe with prefix filter no match*/
{
  SaEvtEventFilterT prefixFilter[1]=
    {
      {1,{3,3,(SaUint8T *)"Abc"}}
    };

  printf("\n\n*************Case 27: Event Subscribe with prefix filter no \
match*************");
  tet_printf(" F:27: Event Subscription with PREFIX Filter and\
 Filters DOESN'T match Published Event Patterns");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
  
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=prefixFilter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    


        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 27");      
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(gl_jCount<1&&gl_error==1)
    {
      printf("\nTest Case: PASSED"); 
      printf("\nThe event has not been delivered,no prefix filter match");
      tet_printf("The event has not been delivered,no prefix filter match");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_jCount>0)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event has been delivered"); 
          tet_printf("The event has been delivered"); 
          tet_result(TET_FAIL);
        }
      else if (gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 27: End*************");
    

  
    
}

void tet_EventSubscribe_SuffixFilter_NoMatch() 
     /* Event Subscribe with suffix filter no match*/
{
  SaEvtEventFilterT suffixFilter[1]=
    {
      {2,{3,3,(SaUint8T *) "Hel"}}
    };

  printf("\n\n*************Case 28: Event Subscribe with suffix filter no \
match**************");
  tet_printf(" F:28: Event Subscription with SUFFIX Filter and\
 Filters DOESN'T match Published Event Patterns");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=4;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=suffixFilter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    


        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 28");      
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(gl_jCount<1&&gl_error==1)
    {
      printf("\nTest Case: PASSED"); 
      printf("\nThe event has not been delivered,no suffix filter match");
      tet_printf("The event has not been delivered,no suffix filter match");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_jCount>0)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event has been delivered"); 
          tet_printf("The event has been delivered"); 
          tet_result(TET_FAIL);
        }
      else if (gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 28: End*************");
    

  
    
}


void tet_EventSubscribe_ExactFilter_NoMatch()  
     /* Event Subscribe with exact filter no match*/
{
  SaEvtEventFilterT exactDiffFilter[1]=
    {
      {3,{5,5,(SaUint8T *)"Hel"}}
    };

  printf("\n\n*************Case 29: Event Subscribe with exact filter no \
match*************");
  tet_printf(" F:29: Event Subscription with EXACT Filter and\
 Filters DOESN'T match Published Event Patterns");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_subscriptionId=5;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=exactDiffFilter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    


        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 29");      
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  printf("\n RetainedEvent = 0 \n");
  tet_saEvtEventFree(&gl_eventHandle);
  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(gl_jCount<1&&gl_error==1)
    {
      printf("\nTest Case: PASSED"); 
      printf("\nThe event has not been delivered,no exact filter match");
      tet_printf("The event has not been delivered,no exact filter match");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_jCount>0)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event has been delivered"); 
          tet_printf("The event has been delivered"); 
          tet_result(TET_FAIL);
        }
      else if (gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 29: End*************");
    

  
    
}

void tet_EventSubscribe_DiffFilterTypes() 
     /* Event Subscribe with different filter types */
{
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;

  SaEvtEventPatternT multiplePattern[5]=
    {
      {5,5,(SaUint8T *)"Hello"},
      {4,4,(SaUint8T *)"Hell"},
      {4,4,(SaUint8T *)"Test"},
      {4,4,(SaUint8T *)"News"},
      {4,4,(SaUint8T *)"Moto"},
    };
  SaEvtEventFilterT diffFilterType[5]=
    {
      {3,{5,5,(SaUint8T *)"Hello"}},
      {1,{2,2,(SaUint8T *)"He"}},
      {2,{4,3,(SaUint8T *)"est"}},
      {4,{4,4,(SaUint8T *)"News"}},
      {3,{4,4,(SaUint8T *)"Moto"}},
    };

  printf("\n\n*************Case 30: Event Subscribe with different filter \
types*************");
  tet_printf(" F:30: Event Subscription with Different Filters and\
 Filters Match Published Event Patterns");

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=5;
  gl_patternArray.patterns=multiplePattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=5;
  gl_patternLength=6;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 30");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);

  gl_filterArray.filtersNumber=5;
  gl_filterArray.filters=diffFilterType;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    


  
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_dupSubscriptionId==gl_subscriptionId)
    { 
      iCmpCount++;
    }        

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED"); 
      printf("\nThe event has been delivered, different filter types");
      tet_printf("The event has been delivered, different filter types");
      tet_result(TET_PASS);
    }
  else
    {
      if(iCmpCount!=1)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event has not been delivered"); 
          tet_printf("The event has not been delivered"); 
          tet_result(TET_FAIL);
        }
      else if (gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 30: End*************");
    

  
    
}

void tet_EventSubscribe_PrefixFilter_FilterSize()  
     /* Event Subscribe with prefix filter ,filter size greater than pattern size*/
{
  SaEvtEventFilterT prefixFilter[1]=
    {
      {1,{6,6,(SaUint8T *)"HelloA"}}
    };

  printf("\n\n*************Case 31: Event Subscribe with prefix filter with\
 filter size greater than pattern size*************");
  tet_printf(" F:31: Event Subscription with PREFIX Filters and\
 FilterSize is Greaterthan Published Event PatternSize");

  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
  
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=prefixFilter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    


        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 31");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  printf("\n RetainedEvent = 1 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(gl_jCount<1&&gl_error==1)
    {
      printf("\nTest Case: PASSED"); 
      printf("\nThe event has not been delivered, prefix filter, filter size \
greater than pattern size ");
      tet_printf("The event has not been delivered, prefix filter, filter size\
 greater than pattern size ");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_jCount>0)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event has been delivered"); 
          tet_printf("The event has been delivered"); 
          tet_result(TET_FAIL);
        }
      else if (gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 31: End*************");


  
    
}

void tet_EventPublish_NullPatternArray_AttributesSet()  
     /* Event Publish for Null Pattern Array with attributes set*/
{
  int i;
  SaEvtEventFilterT prefixFilter[1]=
    {
      {4,{6,6,(SaUint8T *)"HelloA"}}
    };

  printf("\n\n*************Case 32: Event Publish for NULL pattern array with\
 attributes set*************");
  tet_printf("F:32: Event Publish for Null Pattern Array with Attributes Set");
  
  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);        

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=prefixFilter;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
        
  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_retentionTime=10000000000000.0;
  gl_allocatedNumber=0;
  gl_patternArray.patternsNumber=0;
  gl_patternArray.patterns=NULL;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Null Patterns");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  i=gl_rc;
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    


  
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);
 
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);
 
  if(i==SA_AIS_OK&&gl_error==1)
    { 
      printf("\nTest Case: PASSED");
      printf("\nEvent published with NULL pattern array");
      tet_printf("\nEvent published with NULL pattern array");
      tet_result(TET_PASS);
    }
  else
    {
      if(i!=SA_AIS_OK)
        {
          printf("\nTest Case: FAILED");
          printf("\nEvent cannot be published NULL pattern array");
          tet_printf("\nEvent cannot be published with NULL pattern array");
          tet_result(TET_PASS);
        }
      else if (gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 32: End*************");

  
    

}

void tet_Complex_Subscriber_Publisher()
     /* Complex subscriptions and publisher */
{
  SaEvtEventPatternT multiplePattern[5]=
    {
      {5,5,(SaUint8T *)"Hello"},
      {3,3,(SaUint8T *)"Hai"},
      {4,4,(SaUint8T *)"Good"},
      {5,5,(SaUint8T *)"Better"},
      {4,4,(SaUint8T *)"Best"},
    };
  SaEvtEventPatternT diffPattern[3]=
    {
      {4,4,(SaUint8T *)"Moto"},
      {4,4,(SaUint8T *)"News"},
      {4,4,(SaUint8T *)"Test"},
    };
  SaEvtEventFilterT multipleFilter[5]=
    {
      {3,{5,5,(SaUint8T *)"Hello"}},
      {1,{2,2,(SaUint8T *)"Ha"}},
      {2,{3,3,(SaUint8T *)"ood"}},
    };
  SaEvtEventFilterT diffFilter[3]=
    {
      {3,{4,4,(SaUint8T *)"Moto"}},
      {3,{4,4,(SaUint8T *)"News"}},
      {3,{4,4,(SaUint8T *)"Test"}},
    };
  SaEvtEventIdT tempEvtId;

  printf("\n\n*************Case 33: Complex subscriptions and publisher******\
*******");
  tet_printf(" F:33: Event Subscription with Different Filters for Different\
 Published Event Patterns");

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=5;
  gl_patternArray.patterns=multiplePattern;
  gl_retentionTime=100000000000000.0;
  gl_allocatedNumber=5;
  gl_patternLength=6;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 33 1");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  tempEvtId=gl_evtId;
   
  gl_patternArray.patternsNumber=3;
  gl_patternArray.patterns=diffPattern;
  gl_retentionTime=100000000000000.0;
  gl_allocatedNumber=5;
  gl_patternLength=6;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 33 2");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);

  gl_filterArray.filtersNumber=3;
  gl_filterArray.filters=multipleFilter;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  gl_filterArray.filtersNumber=3;
  gl_filterArray.filters=diffFilter;
  gl_subscriptionId=9;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);


  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  printf(" RetainedEvent = 2 \n");
  
    


  gl_allocatedNumber=5;
  gl_patternLength=6;  
    
  thread_creation(&gl_evtHandle);    
  DISPATCH_SLEEP();

  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  gl_evtId=tempEvtId;

  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(gl_jCount==2&&gl_error==1)
    {
      printf("\nTest Case: PASSED"); 
      printf("\nThe event has been delivered, complex");
      tet_printf("The event has been delivered, complex");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_jCount<2)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event has not been delivered"); 
          tet_printf("The event has not been delivered"); 
          tet_result(TET_FAIL);
        }
      else if (gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 33: End*************");

  
    

}

void tet_Same_Subscriber_Publisher_DiffChannel()
     /* Same publisher and subscriber for different channels */
{
  int iCmpCount;
  char *tempData; 
  SaSizeT tempDataSize;
  SaEvtChannelHandleT tempChannelHandle;
  SaEvtEventFilterT duplicate[2]=
    {
      {3,{5,5,(SaUint8T *)"Hello"}},
      {3,{4,4,(SaUint8T *)"Hell"}},
    };
  printf("\n\n*************Case 34: Same publisher and subscriber for \
different channel*************");
  tet_printf("F:34:Multiple Open Channels have Same Publisher and Subscriber");

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
  tempChannelHandle=gl_channelHandle;

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  gl_subscriptionId=4;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  gl_filterArray.filtersNumber=2;
  gl_filterArray.filters=duplicate;
  gl_subscriptionId=3;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);


  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 0 \n");
  
    



  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=5;
  gl_patternLength=6;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
  strcpy (gl_eventData,"Publish Case 34");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
    
  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize)
    {
      iCmpCount++;
    }
 
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&tempChannelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(gl_jCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event delivered to the subscriber");
      tet_printf("The event delivered to the subscriber");
      tet_result(TET_PASS);
    } 
  else
    {
      if(gl_jCount!=1)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event is not delivered to the subscriber");
          tet_printf("The event is not delivered to the subscriber");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 34: End*************");

  
    

} 

void tet_Sync_Async_Subscriber_Publisher()
     /* Sync and Async channel for subscriber and publisher */
{
  int iCmpCount=0;
  char *tempData;
  SaSizeT tempDataSize;
  SaEvtChannelHandleT tempChannelHandle;

  printf("\n\n*************Case 35: Sync and async channel for subscriber and\
 publisher*************");
  tet_printf(" F:35: Sync Open Channel for Subscriber and Async Open Channel\
 for Publisher");

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
  tempChannelHandle=gl_channelHandle;

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  gl_rc=saEvtChannelOpenAsync(gl_evtHandle,gl_invocation,&gl_channelName,
                              gl_channelOpenFlags);

  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 35");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize; 
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();


  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    


  if(gl_eventDataSize==tempDataSize)
    {
      iCmpCount++;
    }

  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&tempChannelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event delivered to the subscriber");
      tet_printf("The event delivered to the subscriber");
      tet_result(TET_PASS);
    } 
  else
    {
      if(iCmpCount!=1)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event is not delivered to the subscriber");
          tet_printf("The event is not delivered to the subscriber");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 35: End*************");

  
    

}

void tet_Event_Subscriber_Unsubscriber() /* Event subscribe and unsubscribe */
{
  int iCmpCount=0;
  char *tempData;
  SaSizeT tempDataSize;
  SaEvtEventIdT tempId;
  printf("\n\n*********Case 36: Event subscribe and unsubscribe*************");
  tet_printf(" F:36: Simple Event Subscription and Unsubscription");

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    



  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 36");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  tempId=gl_evtId;

  gl_allocatedNumber=2;
  gl_patternLength=5;
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();
  if(gl_eventDataSize==tempDataSize)
    {
      iCmpCount++;
    }
  printf("\nComparison Count: %d",iCmpCount);
    
 
  tet_saEvtEventUnsubscribe(&gl_channelHandle);
    

  printf("\n RetainedEvent = 1 \n");
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 36 2");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  gl_allocatedNumber=2;
  gl_patternLength=5;
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();
 
  gl_evtId=tempId;
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);
 
  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event delivered to the subscriber");
      tet_printf("The event delivered to the subscriber");
      tet_result(TET_PASS);
    } 
  else
    {
      if(iCmpCount!=1)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event is not delivered to the subscriber");
          tet_printf("The event is not delivered to the subscriber");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 36: End*************");


  
    
}

void tet_EventDeliverCallback_AttributesSet_RetentionTimeClear()
     /* Event Deliver callback and attributes set after retention time clear */
{
  int i;
  SaEvtEventPatternT duplicate[1]=
    {
      {5,5,(SaUint8T *)"Test"}
    };

  printf("\n\n*************Case 37: Event Deliver Callback with attributes set\
 and after retention time clear*************");
  tet_printf(" F:37: AttributeSet and RetentionTimeClear for the Event\
 received through Deliver Callback");

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=100000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);


  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    


   strcpy (gl_eventData,"Publish Case 37");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);

  gl_allocatedNumber=2;
  gl_patternLength=5;  

  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  gl_retentionTime=0;
  gl_priority=3;
  gl_patternArray.patternsNumber=1;
  gl_patternArray.patterns=duplicate;
  gl_allocatedNumber=2;
  gl_patternLength=5;
    
  printf("\n RetainedEvent = 1 \n");
  i=saEvtEventAttributesSet(gl_eventDeliverHandle,&gl_patternArray,gl_priority,
                            gl_retentionTime,&gl_publisherName);
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
    

  printf("\n RetainedEvents = 0 \n");
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);
 
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==11&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event attributes cannot be set in callback");
      tet_printf("The event attributes cannot be set in callback");
      tet_result(TET_PASS);
    } 
  else
    {
      if(i!=11)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event attributes are being set");
          tet_printf("\nThe event attributes are being set");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 37: End*************");

  
    

}

void tet_MultipleHandle_Subscribers_Publisher()
     /* Mulitple Handle subscribers and publisher */
{
  int iCmpCount=0;
  char *tempData;
  SaSizeT tempDataSize;
  SaEvtHandleT tempEvtHandle,localEvtHandle;
  SaEvtChannelHandleT tempChannelHandle;
  SaEvtEventFilterT duplicate[1]=
    {
      {3,{5,5,(SaUint8T *)"Hello"}}
    };
  printf("\n\n*******Case 38: Multiple Handle subscribers and publisher*****");
  tet_printf(" F:38: Subscriber and Publisher with Different evtHandles"); 
  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);
  localEvtHandle=gl_evtHandle;
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  gl_subscriptionId=5;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
  gl_allocatedNumber=2;
  gl_patternLength=5;

  tet_saEvtInitialize(&tempEvtHandle);
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER
    |SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&tempEvtHandle,&tempChannelHandle);

  tet_saEvtEventAllocate(&tempChannelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 38");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  gl_evtHandle=localEvtHandle; 
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=duplicate;
  gl_subscriptionId=4;

  tet_saEvtEventSubscribe(&tempChannelHandle,&gl_subscriptionId);


  printf("\n NumUsers= 2 : NumSubscribers= 2 : NumPublishers= 2 \n");
  
    



  gl_evtHandle=tempEvtHandle;
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }

  tet_saEvtEventRetentionTimeClear(&tempChannelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&tempChannelHandle);
  gl_evtHandle=localEvtHandle;

  tet_saEvtChannelUnlink(&tempEvtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  tet_saEvtFinalize(&tempEvtHandle);

  if(iCmpCount==2&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event delivered to the subscriber");
      tet_printf("The event delivered to the subscriber");
      tet_result(TET_PASS);
    } 
  else
    {
      if(iCmpCount!=2)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event is not delivered to the subscriber");
          tet_printf("The event is not delivered to the subscriber");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
 
  printf("\n\n*************Case 38: End*************");

  
    

}

void tet_EventPublish_Priority() /* Event Publish with priority */
{
  printf("\n\n*************Case 39: Event Publish with priority*************");
  tet_printf(" F:39: Priority conformance of the Received Events"); 
  var_initialize();
  
  tet_saEvtInitialize(&gl_evtHandle);
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=1000000000000.0;
  gl_priority=0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 39");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
   DISPATCH_SLEEP();

  gl_priority=1;
  gl_retentionTime=10000000000.0;
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);


  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  printf(" RetainedEvents = 1 \n");
  
    


   strcpy (gl_eventData,"Publish Case 39 2");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);

  gl_subscriptionId=6;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();
  
  printf(" RetainedEvents = 2 \n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);
  
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(gl_priority==1&&gl_error==1&&gl_jCount==2)
    {
      printf("\nTest Case: PASSED");
      printf("\nEvents received according to priority");
      tet_printf("\nEvents received according to priority");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_priority==0)
        {
          printf("\nTest Case: FAILED");
          printf("\nEvents not received according to priority");
          tet_printf("\nEvents not received according to priority");
          tet_result(TET_FAIL);
        }
      else 
        {
          if(gl_jCount!=2)
            {
              printf("\nTest Case: FAILED");
              printf("\nEvent not received");
              tet_printf("\nEvent not received");
              tet_result(TET_FAIL); 
            }
          else if(gl_error!=1)
            {
              printf("\nTest Case: UNRESOLVED");
              printf("\nAPI call has failed");
              tet_printf("\nTest Case: UNRESOLVED");
              tet_infoline("API call has failed");
              tet_result(TET_UNRESOLVED);
            }
        }
    }

  printf("\n\n*************Case 39: End*************");


  
    
}

void tet_EventPublish_EventOrder() /* Event Publish with event order */
{ 
  int iCmpCount;
  char *tempData;
  SaSizeT tempDataSize;
  printf("\n\n*************Case 40: Event Publish with event order*********");
  tet_printf(" F:40: Sequential Delivery conformance of the Received Events"); 
  var_initialize();
  
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 40");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
   strcpy (gl_eventData,"Publish Case 40 2");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;  
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);

 
  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  printf(" RetainedEvents = 2 \n");
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  
    


  
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);
 
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(tempDataSize==gl_eventDataSize)
    {
      iCmpCount++;
    }

  if(gl_jCount==2&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event delivered to the subscriber, with event order");
      tet_printf("The event delivered to the subscriber with event order");
      tet_result(TET_PASS);
    } 
  else
    {
      if(gl_jCount!=2)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event is not delivered in order to the subscriber");
          tet_printf("The event is not delivered in order to the subscriber");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 40: End*************");


  
    
}

void tet_EventPublish_EventFree()
{
  char *tempData;
  SaSizeT tempDataSize;
  int iCmpCount=0;

  var_initialize();
  
  tet_saEvtInitialize(&gl_evtHandle);

  printf("\n\n***********Case 41: Event Free before subscriber receives event\
*************");
  tet_printf(" F:41: Free the Event Before the Subscriber Receives it"); 

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish, free");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  tet_saEvtEventFree(&gl_eventHandle);
   DISPATCH_SLEEP();
    

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  printf(" RetainedEvents = 1 \n");
  
    



  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
    

  sleep(60);
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event delivered to the subscriber, event free invoked\
 before subscriber receives the event");
      tet_printf("The event delivered to the subscriber, event free invoked\
 before subscriber receives the event");
      tet_result(TET_PASS);
    } 
  else
    {
      if(iCmpCount!=1)
        {
          printf("\nTest Case: FAILED");
          printf("\nThe event is not delivered to the subscriber");
          tet_printf("The event is not delivered to the subscriber");
          tet_result(TET_FAIL);
        }
      else if(gl_error!=1)
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\n*************Case 41: End*************");


  
    
}

void tet_EventFree_DeliverCallback()
{
  SaEvtChannelHandleT pubChannelHandle,subChannelHandle;
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;  

  printf("\n\n*************Case 42: Event free in event deliver callback****\
*********");
  tet_printf(" F:42: Free the Event in Deliver Callback"); 
  printf("\n\n******Subscriber******");
        
  var_initialize();
  
  tet_saEvtInitialize(&gl_evtHandle); 

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  printf("\nChannel opened for subscriber");
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
                                             
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);


  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 0 \n");
  
    


        
  printf("\n\n******Publisher******");
  printf("\n\nChannel opened for publisher");
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&pubChannelHandle);

  tet_saEvtEventAllocate(&pubChannelHandle,&gl_eventHandle);
        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=100000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 42");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();
  tet_saEvtEventFree(&gl_eventDeliverHandle);

  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  printf(" RetainedEvents = 1 \n");
  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }

  printf("\n******Subscriber******");
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&subChannelHandle);
          
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&subChannelHandle,&gl_subscriptionId);
 
  gl_allocatedNumber=2;
  gl_patternLength=5;
   DISPATCH_SLEEP();
  tet_saEvtEventFree(&gl_eventDeliverHandle);

  printf("\n NumUsers= 3 : NumSubscribers= 2 : NumPublishers= 1 \n");
  printf(" RetainedEvents = 1 \n");
  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }
  printf("\nComparison Count : %d",iCmpCount);

  tet_saEvtEventRetentionTimeClear(&pubChannelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&subChannelHandle);

  tet_saEvtChannelClose(&pubChannelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==2&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscribers, event free in\
 deliver callback");
      tet_printf("The event has been delivered to subscribers, event free in\
 deliver callback");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscribers");
          tet_printf("The event has not been delivered to subscribers");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\n*************Case 42: End*************");
 
  
    
}

void tet_EventRetentionTimeClear_EventFree()
{
  SaEvtChannelHandleT pubChannelHandle;
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;  

  printf("\n\n*************Case 43: Event retention time clear and event free*\
************");
  tet_printf(" F:43: Event RetentionTimeClear before Freeing the Event"); 
  printf("\n\n******Subscriber******");
        
  var_initialize();
  
  tet_saEvtInitialize(&gl_evtHandle); 

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  printf("\nChannel opened for subscriber");
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
                                             
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
        
  printf("\n\n******Publisher******");
  printf("\n\nChannel opened for publisher");
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&pubChannelHandle);

  tet_saEvtEventAllocate(&pubChannelHandle,&gl_eventHandle);
        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=100000000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);


  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  
    

  strcpy (gl_eventData,"Publish Case 43");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }

  printf("\nComparison Count : %d",iCmpCount);

  tet_saEvtEventRetentionTimeClear(&pubChannelHandle,&gl_evtId);

  printf("\n RetainedEvents = 0 \n");
  tet_saEvtEventFree(&gl_eventDeliverHandle);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&pubChannelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscribers, event retention\
 time clear and event free");
      tet_printf("The event has been delivered to subscribers, event retention\
 time clear and event free");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscribers");
          tet_printf("The event has not been delivered to subscribers");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\n*************Case 43: End*************");

  
    

}

void tet_EventPublish_ChannelUnlink()
{
  SaEvtChannelHandleT pubChannelHandle,dupHandle;
  char *tempData;
  SaSizeT tempDataSize;
  SaEvtEventIdT tempEvtId;

  int iCmpCount=0;  

  printf("\n\n********Case 44: Event publish after channel unlink**********");
  tet_printf(" F:44: Publish an Event after Unlinking the Channel"); 
  printf("\n\n******Subscriber******");
        
  var_initialize();
  
  tet_saEvtInitialize(&gl_evtHandle); 

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  printf("\nChannel opened for subscriber");
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
                                             
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
        
  printf("\n\n******Publisher******");
  printf("\n\nChannel opened for publisher");
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&pubChannelHandle);

  tet_saEvtEventAllocate(&pubChannelHandle,&gl_eventHandle);
        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=100000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 43");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  tempEvtId=gl_evtId;
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }
 
  gl_dupSubscriptionId=0;
  
  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  printf(" RetainedEvents = 1\n");

  tet_saEvtChannelUnlink(&gl_evtHandle);
  printf("\n Nothing to be Selected \n");
  
    


      
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&dupHandle);

  tet_saEvtEventAllocate(&dupHandle,&gl_eventHandle);
        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=100000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case Dup");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }

  printf("\nComparison Count : %d",iCmpCount);

  printf("\n NumUsers= 1 : NumSubscribers= 0 : NumPublishers= 1 \n");
  printf(" RetainedEvents = 1\n");
  tet_saEvtEventRetentionTimeClear(&pubChannelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&pubChannelHandle);
  tet_saEvtChannelClose(&dupHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscribers, event publish \
after channel unlink");
      tet_printf("The event has been delivered to subscribers, event publish \
after channel unlink");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscribers");
          tet_printf("The event has not been delivered to subscribers");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\n*************Case 44: End*************");


  
    
}


void tet_EventUnsubscribe_EventPublish()
{
  SaEvtChannelHandleT pubChannelHandle;
  char *tempData;
  SaSizeT tempDataSize;

  int iCmpCount=0;  

  printf("\n\n*********Case 45: Event unsubscribe before event publish******");
  tet_printf(" F:45: Unsubscribe for Event before Publishing it and Check\
 its not delivered"); 
  printf("\n\n******Subscriber******");
        
  var_initialize();
  
  tet_saEvtInitialize(&gl_evtHandle); 

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  printf("\nChannel opened for subscriber");
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
                                             
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);


  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 0 \n");
  
    


  printf("\n\n******Publisher******");
  printf("\n\nChannel opened for publisher");
        
  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&pubChannelHandle);

  tet_saEvtEventAllocate(&pubChannelHandle,&gl_eventHandle);
        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=100000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);

  tet_saEvtEventUnsubscribe(&gl_channelHandle);
    

  printf("\n NumUsers= 2 : NumSubscribers= 1 : NumPublishers= 1 \n");
  strcpy (gl_eventData,"Publish Case 45");
  tempData=gl_eventData;
  tempDataSize=gl_eventDataSize;
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  gl_allocatedNumber=2;
  gl_patternLength=5;
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  if(gl_eventDataSize==tempDataSize&&gl_subscriptionId==gl_dupSubscriptionId)
    {
      iCmpCount++;
    }
  
  printf("\nComparison Count : %d",iCmpCount);

  tet_saEvtEventRetentionTimeClear(&pubChannelHandle,&gl_evtId);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&pubChannelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(iCmpCount==0&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nThe event has been delivered to subscribers, event unsubscribe\
 before event publish");
      tet_printf("The event has been delivered to subscribers, event\
 unsubscribe before event publish");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nThe event has not been delivered to subscribers");
          tet_printf("The event has not been delivered to subscribers");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\n*************Case 45: End*************");

  
    

}

void tet_EventAttributesGet_LessPatternsNumber()
{
  SaEvtEventPatternArrayT tempPatternArray;
  printf("\n\n*************Case 46: Event attributes get with less patterns\
 number*************");
  tet_printf(" F:46: Event AttributeGet with Less Patterns"); 
 
  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);
        
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=100000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);

  tempPatternArray.allocatedNumber=1;
  tempPatternArray.patterns=
    (SaEvtEventPatternT *)malloc(sizeof(SaEvtEventPatternT));
  tempPatternArray.patterns->patternSize=0;

  gl_rc=saEvtEventAttributesGet(gl_eventHandle,&tempPatternArray,&gl_priority,
                                &gl_retentionTime,&gl_publisherName,
                                &gl_publishTime,&gl_evtId);
  result("saEvtEventAttributesGet with less patternsNumber",
         SA_AIS_ERR_NO_SPACE);
    

  printf("\n NumUsers= 1 : NumSubscribers= 0 : NumPublishers= 0 \n");
  
    


  
  tet_saEvtEventFree(&gl_eventHandle);
  
  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);
 
  printf("\n\n*************Case 46: End*************");

  
    

}

void tet_Simple_Test()
{
  printf("\n\n*************Case 47: tet_Simple_Test *************\n"); 
  tet_printf(" F:47: Simple Flow of the  Event Distribution Service"); 
  var_initialize();
  
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);
 
  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=100000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);
   strcpy (gl_eventData,"Publish Case 47"); 
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);

  gl_allocatedNumber=2;
  gl_patternLength=5;
    
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);


  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  printf(" RetainedEvents = 0\n");
  
    



  tet_saEvtEventPublish(&gl_eventDeliverHandle,&gl_evtId); 

  gl_subscriptionId=36;
  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

   DISPATCH_SLEEP();

  tet_saEvtEventUnsubscribe(&gl_channelHandle);

  printf("\n NumUsers= 1 : NumSubscribers= 1 : NumPublishers= 1 \n");
  printf(" RetainedEvents = 1\n");
  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  printf("\n\n*************Case 47: End*************");

  
    
}
/***********************************************/
/******************Miscellaneous****************/
/***********************************************/

void tet_ChannelOpen_before_Initialize()
{
  int i;
  printf("\n\n******Case 1: Channel open before initialize******");
  tet_printf(" M:1: Not able to Open a Channel before Initialize"); 

  var_initialize();

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  i=saEvtChannelOpen(gl_evtHandle,&gl_channelName,gl_channelOpenFlags,
                     1000000000.0,&gl_channelHandle);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelOpen() was not successful");
      tet_printf("saEvtChannelOpen() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelOpen() was successful");
          tet_printf("saEvtChannelOpen() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\n********** Case 1: End ***********");
}

void tet_ChannelOpen_after_Finalize()
{
  int i;
  printf("\n\n******Case 2: Channel open after finalize******");
  tet_printf(" M:2: Not able to Open a Channel After Finalize"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  
    

  tet_saEvtFinalize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  i=saEvtChannelOpen(gl_evtHandle,&gl_channelName,gl_channelOpenFlags,
                     1000000000.0,&gl_channelHandle);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelOpen() was not successful");
      tet_printf("saEvtChannelOpen() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelOpen() was successful");
          tet_printf("saEvtChannelOpen() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\n************* Case 2: End ************");

  
    
}

void tet_ChannelOpenAsync_before_Initialize()
{
  int i;
  printf("\n\n******Case 3: Channel open async before initialize******");
  tet_printf(" M:3: Not able to AsyncOpen a Channel Before Initialize"); 

  var_initialize();

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  i=saEvtChannelOpenAsync(gl_evtHandle,gl_invocation,&gl_channelName,
                          gl_channelOpenFlags);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelOpenAsync() was not successful");
      tet_printf("saEvtChannelOpenAsync() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelOpenAsync() was successful");
          tet_printf("saEvtChannelOpenAsync() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 3: End");
}

void tet_ChannelOpenAsync_after_Finalize()
{
  int i;
  printf("\n\n******Case 4: Channel open async after finalize******");
  tet_printf(" M:4: Not able to AsyncOpen a Channel After Finalize"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  
    

  tet_saEvtFinalize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  i=saEvtChannelOpen(gl_evtHandle,&gl_channelName,gl_channelOpenFlags,
                     1000000000.0,&gl_channelHandle);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelOpenAsync() was not successful");
      tet_printf("saEvtChannelOpenAsync() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelOpenAsync() was successful");
          tet_printf("saEvtChannelOpenAsync() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 4: End");

  
    
}

void tet_Subscribe_before_ChannelOpen()
{
  int i;
  printf("\n\n******Case 5: Subscribe before channel open******");
  tet_printf(" M:5: Not able to Subscribe Before Channel Opened"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  
    

  i=saEvtEventSubscribe(gl_channelHandle,&gl_filterArray,gl_subscriptionId);

  tet_saEvtFinalize(&gl_evtHandle);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventSubscribe() was not successful");
      tet_printf("saEvtEventSubscribe() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventSubscribe() was successful");
          tet_printf("saEvtEventSubscribe() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 5: End");

  
    
}

void tet_Subscribe_after_ChannelClose()
{
  int i;
  printf("\n\n******Case 6: Subscribe after channel close******");
  tet_printf(" M:6: Not able to Subscribe After Channel Closed"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);
 
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  i=saEvtEventSubscribe(gl_channelHandle,&gl_filterArray,gl_subscriptionId);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventSubscribe() was not successful");
      tet_printf("saEvtEventSubscribe() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventSubscribe() was successful");
          tet_printf("saEvtEventSubscribe() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 6: End");

  
    
}

void tet_Subscribe_after_ChannelUnlink()
{
  int i;
  printf("\n\n******Case 7: Subscribe after channel unlink******");
  tet_printf(" M:7: Not able to Subscribe After Channel Unlinked"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);
 
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  i=saEvtEventSubscribe(gl_channelHandle,&gl_filterArray,gl_subscriptionId);

  tet_saEvtFinalize(&gl_evtHandle);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventSubscribe() was not successful");
      tet_printf("saEvtEventSubscribe() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventSubscribe() was successful");
          tet_printf("saEvtEventSubscribe() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 7: End");

  
    
}

void tet_Subscribe_after_Finalize()
{
  int i;
  printf("\n\n******Case 8: Subscribe after finalize******");
  tet_printf(" M:8: Not able to Subscribe After Finalize"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);
 
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  i=saEvtEventSubscribe(gl_channelHandle,&gl_filterArray,gl_subscriptionId);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventSubscribe() was not successful");
      tet_printf("saEvtEventSubscribe() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventSubscribe() was successful");
          tet_printf("saEvtEventSubscribe() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 8: End");

  
    
}

void tet_Allocate_before_ChannelOpen()
{
  int i;
  printf("\n\n******Case 9: Allocate before channel open******");
  tet_printf(" M:9: Not able to Allocate EventHeadr Before Channel Opened"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  i=saEvtEventAllocate(gl_channelHandle,&gl_eventHandle);

  tet_saEvtFinalize(&gl_evtHandle);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAllocate() was not successful");
      tet_printf("saEvtEventAllocate() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAllocate() was successful");
          tet_printf("saEvtEventAllocate() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 9: End");
}

void tet_Allocate_after_ChannelClose()
{
  int i;
  printf("\n\n******Case 10: Allocate after channel close******");
  tet_printf(" M:10: Not able to Allocate EventHeadr After Channel Closed"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  i=saEvtEventAllocate(gl_channelHandle,&gl_eventHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAllocate() was not successful");
      tet_printf("saEvtEventAllocate() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAllocate() was successful");
          tet_printf("saEvtEventAllocate() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 10: End");

  
    
}

void tet_Allocate_after_ChannelUnlink()
{
  int i;
  printf("\n\n******Case 11: Allocate after channel unlink******");
  tet_printf(" M:11: Not able to Allocate EventHeadr After Channel Unlinked"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  i=saEvtEventAllocate(gl_channelHandle,&gl_eventHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAllocate() was not successful");
      tet_printf("saEvtEventAllocate() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAllocate() was successful");
          tet_printf("saEvtEventAllocate() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 11: End");

  
    
}

void tet_Allocate_after_Finalize()
{
  int i;
  printf("\n\n******Case 12: Allocate after finalize******");
  tet_printf(" M:12: Not able to Allocate EventHeadr After Finalize"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  i=saEvtEventAllocate(gl_channelHandle,&gl_eventHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAllocate() was not successful");
      tet_printf("saEvtEventAllocate() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAllocate() was successful");
          tet_printf("saEvtEventAllocate() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 12: End");

  
    
}

void tet_Allocate_check_DefaultValues()
{
  int i=1;
  printf("\n\n******Case 13: Allocate check default values******");
  tet_printf(" M:13: Allocate EventHeadr and Check Default Values using\
 AttributeGet"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  
    

  gl_allocatedNumber=0;
  gl_patternLength=0;
  tet_saEvtEventAttributesGet(&gl_eventHandle); 
  
  if(gl_priority!=3&&gl_retentionTime!=0&&gl_publisherName.length!=0
     &&gl_evtId!=0)
    {
      i=0;
    } 

  tet_saEvtEventFree(&gl_eventHandle);  

  tet_saEvtChannelClose(&gl_channelHandle);
  
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);


  if(i==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAllocate() was successful");
      tet_printf("saEvtEventAllocate() was successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAllocate() was successful,default values not \
assigned");
          tet_printf("saEvtEventAllocate() was successful, default values not\
 assigned");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 13: End");

  
    
}

void tet_AttributesSet_before_Allocate()
{
  int i=1;
  printf("\n\n******Case 14: Attributes set before allocate******");
  tet_printf(" M:14: Not able to Set Event Attributes Before Allocating\
 EventHeadr"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=1000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  i=saEvtEventAttributesSet(gl_eventHandle,&gl_patternArray,gl_priority,
                            gl_retentionTime,&gl_publisherName);  

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAttributesSet() was not successful");
      tet_printf("saEvtEventAttributesSet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAttributesSet() was successful");
          tet_printf("saEvtEventAttributesSet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 14: End");

  
    
}

void tet_AttributesSet_after_EventFree()
{
  int i=1;
  printf("\n\n******Case 15: Attributes set after event free******");
  tet_printf("M:15:Not able to Set Event Attributes After Freeing EventHeadr");

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  tet_saEvtEventFree(&gl_eventHandle); 

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=1000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  i=saEvtEventAttributesSet(gl_eventHandle,&gl_patternArray,gl_priority,
                            gl_retentionTime,&gl_publisherName);  

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAttributesSet() was not successful");
      tet_printf("saEvtEventAttributesSet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAttributesSet() was successful");
          tet_printf("saEvtEventAttributesSet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 15: End");

  
    
}

void tet_AttributesSet_after_ChannelClose()
{
  int i=1;
  printf("\n\n******Case 16: Attributes set after channel close******");
  tet_printf(" M:16: Not able to Set Event Attributes After Channel Closed"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  tet_saEvtEventFree(&gl_eventHandle); 

  tet_saEvtChannelClose(&gl_channelHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=1000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  i=saEvtEventAttributesSet(gl_eventHandle,&gl_patternArray,gl_priority,
                            gl_retentionTime,&gl_publisherName);  


  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAttributesSet() was not successful");
      tet_printf("saEvtEventAttributesSet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAttributesSet() was successful");
          tet_printf("saEvtEventAttributesSet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 16: End");

  
    
}

void tet_AttributesSet_after_ChannelUnlink()
{
  int i=1;
  printf("\n\n******Case 17: Attributes set after channel unlink******");
  tet_printf(" M:17: Not able to Set Event Attributes After Channel Unlinked");

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  tet_saEvtEventFree(&gl_eventHandle); 

  tet_saEvtChannelClose(&gl_channelHandle);

  
    

  tet_saEvtChannelUnlink(&gl_evtHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=1000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  i=saEvtEventAttributesSet(gl_eventHandle,&gl_patternArray,gl_priority,
                            gl_retentionTime,&gl_publisherName);  

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAttributesSet() was not successful");
      tet_printf("saEvtEventAttributesSet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAttributesSet() was successful");
          tet_printf("saEvtEventAttributesSet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 17: End");

  
    
}

void tet_AttributesSet_after_Finalize()
{
  int i=1;
  printf("\n\n******Case 18: Attributes set after finalize******");
  tet_printf(" M:18: Not able to Set Event Attributes After Finalize"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  tet_saEvtEventFree(&gl_eventHandle); 

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=1000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  i=saEvtEventAttributesSet(gl_eventHandle,&gl_patternArray,gl_priority,
                            gl_retentionTime,&gl_publisherName);  

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAttributesSet() was not successful");
      tet_printf("saEvtEventAttributesSet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAttributesSet() was successful");
          tet_printf("saEvtEventAttributesSet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 18: End");

  
    
}

void tet_Publish_before_Allocate()
{
  int i=1;
  printf("\n\n******Case 19: Publish before event allocate******");
  tet_printf("M:19:Not able to Publish an Event Before Allocating EventHeadr");

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  i=saEvtEventPublish(gl_eventHandle,gl_eventData,gl_eventDataSize,&gl_evtId);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventPublish() was not successful");
      tet_printf("saEvtEventPublish() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventPublish() was successful");
          tet_printf("saEvtEventPublish() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 19: End");

  
    
}

void tet_Publish_after_EventFree()
{
  int i=1;
  printf("\n\n******Case 20: Publish after event free******");
  tet_printf(" M:20: Not able to Publish an Event After Freeing EventHeadr"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  
    

  tet_saEvtEventFree(&gl_eventHandle);

  i=saEvtEventPublish(gl_eventHandle,gl_eventData,gl_eventDataSize,&gl_evtId);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventPublish() was not successful");
      tet_printf("saEvtEventPublish() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventPublish() was successful");
          tet_printf("saEvtEventPublish() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 20: End");

  
    
}

void tet_Publish_after_ChannelClose()
{
  int i=1;
  printf("\n\n******Case 21: Publish after channel close******");
  tet_printf(" M:21: Not able to Publish an Event After Channel Closed"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  tet_saEvtEventFree(&gl_eventHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  i=saEvtEventPublish(gl_eventHandle,gl_eventData,gl_eventDataSize,&gl_evtId);
  printf("\ntry publish");

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventPublish() was not successful");
      tet_printf("saEvtEventPublish() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventPublish() was successful");
          tet_printf("saEvtEventPublish() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 21: End");

  
    
}

void tet_Publish_after_ChannelUnlink()
{
  int i=1;
  printf("\n\n******Case 22: Publish after channel unlink******");
  tet_printf(" M:22: Not able to Publish an Event After Channel Unlinked"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  
    

  tet_saEvtChannelUnlink(&gl_evtHandle);

  i=saEvtEventPublish(gl_eventHandle,gl_eventData,gl_eventDataSize,&gl_evtId);
  printf("\ntry publish");

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventPublish() was not successful");
      tet_printf("saEvtEventPublish() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventPublish() was successful");
          tet_printf("saEvtEventPublish() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 22: End");

  
    
}

void tet_Publish_after_Finalize()
{
  int i=1;
  printf("\n\n******Case 23: Publish after finalize******");
  tet_printf(" M:23: Not able to Publish an Event After Finalize"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  
    

  tet_saEvtFinalize(&gl_evtHandle);

  i=saEvtEventPublish(gl_eventHandle,gl_eventData,gl_eventDataSize,&gl_evtId);
  printf("\ntry publish");

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventPublish() was not successful");
      tet_printf("saEvtEventPublish() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventPublish() was successful");
          tet_printf("saEvtEventPublish() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 23: End");

  
    
}

void tet_SelectionObject_before_Initialize()
{
  int i;
  printf("\n\n******Case 24: Selection object before initialize******");
  tet_printf(" M:24: Not able to Get Selection Object Before Initialize"); 

  var_initialize();

  i=saEvtSelectionObjectGet(gl_evtHandle,&gl_selObject);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtSelectionObjectGet() was not successful");
      tet_printf("saEvtSelectionObjectGet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtSelectionObjectGet() was successful");
          tet_printf("saEvtSelectionObjectGet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 24: End");
}

void tet_SelectionObject_after_Finalize()
{
  int i;
  printf("\n\n******Case 25: Selection after finalize******");
  tet_printf(" M:25: Not able to Get Selection Object After Finalize"); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  
    

  tet_saEvtFinalize(&gl_evtHandle);

  
  i=saEvtSelectionObjectGet(gl_evtHandle,&gl_selObject);
  

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtSelectionObjectGet() was not successful");
      tet_printf("saEvtSelectionObjectGet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtSelectionObjectGet() was successful");
          tet_printf("saEvtSelectionObjectGet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 25: End");

  
    
}

void tet_Dispatch_before_Initialize()
{
  int i;
  printf("\n\n*******Case 26: Dispatch before initialize******");
  tet_printf(" M:26: Not able to Dispatch Before Initialize"); 

  var_initialize();
  
  i=saEvtDispatch(gl_evtHandle,SA_DISPATCH_ONE);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtDispatch() was not successful");
      tet_printf("saEvtDispatch() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtDispatch() was successful");
          tet_printf("saEvtDispatch() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\nCase 26: End");
}
void tet_Dispatch_after_Finalize()
{
  int i;
  printf("\n\n*******Case 27: Dispatch after finalize******");
  tet_printf(" M:27: Not able to Dispatch After Finalize"); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  
    

  tet_saEvtFinalize(&gl_evtHandle);
  
  
  i=saEvtDispatch(gl_evtHandle,SA_DISPATCH_ONE);
  

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtDispatch() was not successful");
      tet_printf("saEvtDispatch() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtDispatch() was successful");
          tet_printf("saEvtDispatch() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\nCase 27: End");

  
    
}

void tet_AttributesGet_before_Allocate()
{
  int i=1;
  printf("\n\n******Case 28: Attributes get before allocate******");
  tet_printf("M:28: Not able to Get Attributes Before Allocating EventHeadr");

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  i=saEvtEventAttributesGet(gl_eventHandle,&gl_patternArray,&gl_priority,
                            &gl_retentionTime,&gl_publisherName,
                            &gl_publishTime,&gl_evtId);  
  printf("\ntry attributes get");

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAttributesGet() was not successful");
      tet_printf("saEvtEventAttributesGet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAttributesGet() was successful");
          tet_printf("saEvtEventAttributesGet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 28: End");

  
    
}

void tet_AttributesGet_after_EventFree()
{
  int i=1;
  printf("\n\n******Case 29: Attributes get after event free******");
  tet_printf(" M:29: Not able to Get Attributes After Freeing EventHeadr"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  
    

  tet_saEvtEventFree(&gl_eventHandle); 

  i=saEvtEventAttributesGet(gl_eventHandle,&gl_patternArray,&gl_priority,
                            &gl_retentionTime,&gl_publisherName,
                            &gl_publishTime,&gl_evtId);  
  printf("\ntry attributes get");

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAttributesGet() was not successful");
      tet_printf("saEvtEventAttributesGet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAttributesGet() was successful");
          tet_printf("saEvtEventAttributesGet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 29: End");

  
    
}

void tet_AttributesGet_after_ChannelClose()
{
  int i=1;
  printf("\n\n******Case 30: Attributes get after channel close******");
  tet_printf(" M:30: Not able to Get Attributes After Channel Closed"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  
    

  tet_saEvtEventFree(&gl_eventHandle); 

  tet_saEvtChannelClose(&gl_channelHandle);

  i=saEvtEventAttributesGet(gl_eventHandle,&gl_patternArray,&gl_priority,
                            &gl_retentionTime,&gl_publisherName,
                            &gl_publishTime,&gl_evtId);  
  printf("\ntry attributes get");

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAttributesGet() was not successful");
      tet_printf("saEvtEventAttributesGet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAttributesGet() was successful");
          tet_printf("saEvtEventAttributesGet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 30: End");

  
    
}

void tet_AttributesGet_after_ChannelUnlink()
{
  int i=1;
  printf("\n\n******Case 31: Attributes get after channel unlink******");
  tet_printf(" M:31: Not able to Get Attributes After Channel Unlinked"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  
    

  tet_saEvtEventFree(&gl_eventHandle); 

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  i=saEvtEventAttributesGet(gl_eventHandle,&gl_patternArray,&gl_priority,
                            &gl_retentionTime,&gl_publisherName,
                            &gl_publishTime,&gl_evtId);  
  printf("\ntry attributes get");

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAttributesGet() was not successful");
      tet_printf("saEvtEventAttributesGet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAttributesGet() was successful");
          tet_printf("saEvtEventAttributesGet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 31: End");

  
    
}

void tet_AttributesGet_after_Finalize()
{
  int i=1;
  printf("\n\n******Case 32: Attributes get after finalize******");
  tet_printf(" M:32: Not able to Get Attributes After Finalize"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  tet_saEvtEventFree(&gl_eventHandle); 

  tet_saEvtChannelClose(&gl_channelHandle);

  
    

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  i=saEvtEventAttributesGet(gl_eventHandle,&gl_patternArray,&gl_priority,
                            &gl_retentionTime,&gl_publisherName,
                            &gl_publishTime,&gl_evtId);  
  printf("\ntry attributes get");

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventAttributesGet() was not successful");
      tet_printf("saEvtEventAttributesGet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventAttributesGet() was successful");
          tet_printf("saEvtEventAttributesGet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 32: End");

  
    
}

void tet_Unsubscribe_before_ChannelOpen()
{
  int i;
  printf("\n\n******Case 33: Unsubscribe before channel open******");
  tet_printf(" M:33: Not able to Unsubscribe Before Channel Opened"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  i=saEvtEventUnsubscribe(gl_channelHandle,gl_subscriptionId);
  printf("\ntry unsubscribe");

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventUnsubscribe() was not successful");
      tet_printf("saEvtEventUnsubscribe() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventUnsubscribe() was successful");
          tet_printf("saEvtEventUnsubscribe() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 33: End");
}

void tet_Unsubscribe_after_ChannelClose()
{
  int i;
  printf("\n\n******Case 34: Unsubscribe after channel close******");
  tet_printf(" M:34: Not able to Unsubscribe After Channel Closed"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  i=saEvtEventUnsubscribe(gl_channelHandle,gl_subscriptionId);
  printf("\ntry unsubscribe");

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventUnsubscribe() was not successful");
      tet_printf("saEvtEventUnsubscribe() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventUnsubscribe() was successful");
          tet_printf("saEvtEventUnsubscribe() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 34: End");

  
    
}

void tet_Unsubscribe_after_ChannelUnlink()
{
  int i;
  printf("\n\n******Case 35: Unsubscribe after channel unlink******");
  tet_printf(" M:35: Not able to Unsubscribe After Channel Unlinked"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  
    

  tet_saEvtChannelUnlink(&gl_evtHandle);

  i=saEvtEventUnsubscribe(gl_channelHandle,gl_subscriptionId);
  printf("\ntry unsubscribe");

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventUnsubscribe() was not successful");
      tet_printf("saEvtEventUnsubscribe() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventUnsubscribe() was successful");
          tet_printf("saEvtEventUnsubscribe() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 35: End");

  
    
}

void tet_Unsubscribe_after_Finalize()
{
  int i;
  printf("\n\n******Case 36: Unsubscribe after finalize******");
  tet_printf(" M:36: Not able to Unsubscribe After Finalize"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  
    

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  i=saEvtEventUnsubscribe(gl_channelHandle,gl_subscriptionId);
  printf("\ntry unsubscribe");

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventUnsubscribe() was not successful");
      tet_printf("saEvtEventUnsubscribe() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventUnsubscribe() was successful");
          tet_printf("saEvtEventUnsubscribe() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 36: End");

  
    
}

void tet_RetentionTimeClear_before_ChannelOpen()
{
  int i;
  printf("\n\n******Case 37: Retention time clear before channel open******");
  tet_printf(" M:37: Not able to Clear RetentionTimer Before Channel Opened"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_evtId=1001;
  i=saEvtEventRetentionTimeClear(gl_channelHandle,gl_evtId);
  printf("\ntry retention time clear");

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventRetentionTime() was not successful");
      tet_printf("saEvtEventRetentionTime() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventRetentionTimeClear() was successful");
          tet_printf("saEvtEventRetentionTimeClear() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 37: End");
}

void tet_RetentionTimeClear_after_ChannelClose()
{
  int i;
  printf("\n\n******Case 38: Retention time clear after channel close******");
  tet_printf(" M:38: Not able to Clear RetentionTimer After Channel Closed"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    
 
  tet_saEvtChannelClose(&gl_channelHandle);

  gl_evtId=1001;
  i=saEvtEventRetentionTimeClear(gl_channelHandle,gl_evtId);
  printf("\ntry retention time clear");

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventRetentionTime() was not successful");
      tet_printf("saEvtEventRetentionTime() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventRetentionTimeClear() was successful");
          tet_printf("saEvtEventRetentionTimeClear() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 38: End");

  
    
}

void tet_RetentionTimeClear_after_ChannelUnlink()
{
  int i;
  printf("\n\n******Case 39: Retention time clear after channel unlink******");
  tet_printf(" M:39: Not able to Clear RetentionTimer After Channel Unlinked");

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    
 
  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  gl_evtId=1001;
  i=saEvtEventRetentionTimeClear(gl_channelHandle,gl_evtId);
  printf("\ntry retention time clear");

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventRetentionTime() was not successful");
      tet_printf("saEvtEventRetentionTime() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventRetentionTimeClear() was successful");
          tet_printf("saEvtEventRetentionTimeClear() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 39: End");

  
    
}

void tet_RetentionTimeClear_after_Finalize()
{
  int i;
  printf("\n\n******Case 40: Retention time clear after finalize******");
  tet_printf(" M:40: Not able to Clear RetentionTimer After Finalize"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    
 
  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle)
    ;
  tet_saEvtFinalize(&gl_evtHandle);

  gl_evtId=1001;
  i=saEvtEventRetentionTimeClear(gl_channelHandle,gl_evtId);
  printf("\ntry retention time clear");

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventRetentionTime() was not successful");
      tet_printf("saEvtEventRetentionTime() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventRetentionTimeClear() was successful");
          tet_printf("saEvtEventRetentionTimeClear() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 40: End");

  
    
}

void tet_EventFree_before_Allocate()
{
  int i=1;
  printf("\n\n******Case 41: Event free before allocate******");
  tet_printf(" M:41: Not able to Free EventHeadr Before Allocating it"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  i=saEvtEventFree(gl_eventHandle);
  printf("\ntry event free");

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventFree() was not successful");
      tet_printf("saEvtEventFree() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventFree() was successful");
          tet_printf("saEvtEventFree() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 41: End");

  
    
}

void tet_EventFree_after_ChannelClose()
{
  int i=1;
  printf("\n\n******Case 42: Event free after channel close******");
  tet_printf(" M:42: Not able to Free EventHeadr After Channel Closed"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  i=saEvtEventFree(gl_eventHandle);
  printf("\ntry event free");

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventFree() was not successful");
      tet_printf("saEvtEventFree() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventFree() was successful");
          tet_printf("saEvtEventFree() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 42: End");

  
    
}

void tet_EventFree_after_ChannelUnlink()
{
  int i=1;
  printf("\n\n******Case 43: Event free after channel unlink******");
  tet_printf(" M:43: Not able to Free EventHeadr After Channel Unlinked"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  i=saEvtEventFree(gl_eventHandle);
  printf("\ntry event free");

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventFree() was not successful");
      tet_printf("saEvtEventFree() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventFree() was successful");
          tet_printf("saEvtEventFree() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 43: End");

  
    
}

void tet_EventFree_after_Finalize()
{
  int i=1;
  printf("\n\n******Case 44: Event free after finalize******");
  tet_printf(" M:44: Not able to Free EventHeadr After Finalize"); 

  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  
    

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  i=saEvtEventFree(gl_eventHandle);
  printf("\ntry event free");

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventFree() was not successful");
      tet_printf("saEvtEventFree() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventFree() was successful");
          tet_printf("saEvtEventFree() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 44: End");

  
    
}

void tet_ChannelClose_before_ChannelOpen()
{
  int i;
  printf("\n\n******Case 45: Channel close before channel open******");
  tet_printf(" M:45: Not able to Close Channel Before Opening it"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  i=saEvtChannelClose(gl_channelHandle);

  tet_saEvtFinalize(&gl_evtHandle);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelClose() was not successful");
      tet_printf("saEvtChannelClose() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelClose() was successful");
          tet_printf("saEvtChannelClose() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 45: End");
}

void tet_ChannelClose_after_ChannelClose()
{
  int i;
  printf("\n\n******Case 46: Channel close after channel close******");
  tet_printf(" M:46: Not able to Close an already Closed Channel"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  
    

  i=saEvtChannelClose(gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelClose() was not successful");
      tet_printf("saEvtChannelClose() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelClose() was successful");
          tet_printf("saEvtChannelClose() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 46: End");

  
    
}

void tet_ChannelClose_after_ChannelUnlink()
{
  int i;
  printf("\n\n******Case 47: Channel close after channel unlink******");
  tet_printf(" M:47: Not able to Close an already Unlinked Channel"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  
    

  i=saEvtChannelClose(gl_channelHandle);
  WHILE_TRY_AGAIN(i)
    {
      RETRY_SLEEP; 
      i=saEvtChannelClose(gl_channelHandle);
    }

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelClose() was successful");
      tet_printf("saEvtChannelClose() was successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelClose() was not successful");
          tet_printf("saEvtChannelClose() was not successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 47: End");

  
    
}

void tet_ChannelClose_after_Finalize()
{
  int i;
  printf("\n\n******Case 48: Channel close after finalize******");
  tet_printf(" M:48: Not able to Close a Channel After Finalize"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  
    

  tet_saEvtFinalize(&gl_evtHandle);

  i=saEvtChannelClose(gl_channelHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelClose() was not successful");
      tet_printf("saEvtChannelClose() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelClose() was successful");
          tet_printf("saEvtChannelClose() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 48: End");

  
    
}

void tet_ChannelUnlink_before_ChannelOpen()
{
  int i;
  printf("\n\n******Case 49: Channel unlink before channel open******");
  tet_printf(" M:49: Not able to Unlink a Channel Before Opening it"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  
    

  i=saEvtChannelUnlink(gl_evtHandle,&gl_channelName);
  WHILE_TRY_AGAIN(i)
    {
      RETRY_SLEEP; 
      i=saEvtChannelUnlink(gl_evtHandle,&gl_channelName);
    }

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==12&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelUnlink() was not successful");
      tet_printf("saEvtChannelUnlink() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelUnlink() was successful");
          tet_printf("saEvtChannelUnlink() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 49: End");

  
    
}

void tet_ChannelUnlink_after_Finalize()
{
  int i;
  printf("\n\n******Case 50: Channel unlink after finalize******");
  tet_printf(" M:50: Not able to Unlink a Channel After Finalize"); 

  var_initialize();

  tet_saEvtInitialize(&gl_evtHandle);

  
    

  tet_saEvtFinalize(&gl_evtHandle);

  i=saEvtChannelUnlink(gl_evtHandle,&gl_channelName);


  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelUnlink() was not successful");
      tet_printf("saEvtChannelUnlink() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelUnlink() was successful");
          tet_printf("saEvtChannelUnlink() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 50: End");

  
    
}


void tet_Finalize_after_Finalize()
{
  int i;
  printf("\n\n******Case 51: Finalize after finalize******");
  tet_printf(" M:51: Not able to Finalize After Finalize"); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  
    

  tet_saEvtFinalize(&gl_evtHandle);

  
  i=saEvtFinalize(gl_evtHandle);
  

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtFinalize() was not successful");
      tet_printf("saEvtFinalize() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtFinalize() was successful");
          tet_printf("saEvtFinalize() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\nCase 51: End");

  
    
}

void tet_ChannelOpen_after_ChannelUnlink()
{
  int i;
  printf("\n\n******Case 52: Channel Open after channel Unlink*******");
  tet_printf(" M:52: Not able to Open Channel After Unlinking it"); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  printf("%llu",gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  
    

  
  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  i=saEvtChannelOpen(gl_evtHandle,&gl_channelName,gl_channelOpenFlags,
                     1000000000.0,&gl_channelHandle);
  WHILE_TRY_AGAIN(i)
    {
      RETRY_SLEEP; 
      i=saEvtChannelOpen(gl_evtHandle,&gl_channelName,gl_channelOpenFlags,
                         1000000000.0,&gl_channelHandle);
    }
  

  tet_saEvtChannelClose(&gl_channelHandle);
  
  tet_saEvtFinalize(&gl_evtHandle);

  if(i==12&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelOpen() was not successful");
      tet_printf("saEvtChannelOpen() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelOpen() was successful");
          tet_printf("saEvtChannelOpen() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\nCase 52: End");

  
    
}

void tet_ChannelOpen_after_ChannelClose()
{ 
  int i;
  printf("\n\n******Case 53: Channel Open after channel close*******");
  tet_printf(" M:53: Open Channel After Closing it"); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  printf("%llu",gl_channelHandle);

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  
    
  
    

  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  i=saEvtChannelOpen(gl_evtHandle,&gl_channelName,gl_channelOpenFlags,
                     1000000000.0,&gl_channelHandle);
  WHILE_TRY_AGAIN(i)
    {
      RETRY_SLEEP; 
      i=saEvtChannelOpen(gl_evtHandle,&gl_channelName,gl_channelOpenFlags,
                         1000000000.0,&gl_channelHandle);
    }
  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);
 
  tet_saEvtFinalize(&gl_evtHandle);

  if(i==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelOpen() was successful");
      tet_printf("saEvtChannelOpen() was successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelOpen() was not successful");
          tet_printf("saEvtChannelOpen() was not successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\nCase 53: End");

  
    
}

void tet_ChannelOpenAsync_after_ChannelClose()
{
  int i,j;
  printf("\n\n******Case 54: Channel Open Async after channel close******");
  tet_printf(" M:54: AsyncOpen Channel After Closing it"); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);  

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpenAsync(&gl_evtHandle);
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  
    

  tet_saEvtChannelClose(&gl_channelHandle);

  
    

  
    

  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpenAsync(&gl_evtHandle);
  j=gl_rc;
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();
  i=gl_rc;

  tet_saEvtChannelClose(&gl_channelHandle); 

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==1&&j==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelOpen() was successful");
      tet_printf("saEvtChannelOpen() was successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelOpen() was not successful");
          tet_printf("saEvtChannelOpen() was not successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 54: End");

  
    
}

void tet_ChannelOpenAsync_after_ChannelUnlink()
{
  SaEvtChannelHandleT tempChannelHandle;
  int i,j;
  printf("\n\n******Case 55: Channel Open Async after channel unlink******");
  tet_printf(" M:55: Not able to AsyncOpen Channel After Unlinking it"); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);  

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  gl_channelName.length=5;
  memset(gl_channelName.value,'\0',gl_channelName.length);
  memcpy(gl_channelName.value,"diff",gl_channelName.length);
  tet_saEvtChannelOpenAsync(&gl_evtHandle);
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();

  
    

  tet_saEvtChannelUnlink(&gl_channelHandle);
  tempChannelHandle=gl_channelHandle;

  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpenAsync(&gl_evtHandle);
  j=gl_rc;
  thread_creation(&gl_evtHandle);
  DISPATCH_SLEEP();
  i=gl_error;
  gl_error=1;

  tet_saEvtChannelClose(&tempChannelHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==12&&j==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelOpen() was not successful");
      tet_printf("saEvtChannelOpen() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelOpen() was successful");
          tet_printf("saEvtChannelOpen() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }
  printf("\n\nCase 55: End");

  
    
}

void tet_EventSubscribe_after_ChannelUnlink()
{
  int i;
  printf("\n\n******Case 56: Event subsribe after channel unlink******");
  tet_printf(" M:56: Event Subscribe After Channel Unlinked"); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;

  
    
  
  tet_saEvtChannelUnlink(&gl_evtHandle);
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
  i=gl_rc;

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=1000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);

  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);
  
  tet_saEvtChannelClose(&gl_channelHandle);  

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventSubscribe() was successful");
      tet_printf("saEvtEventSubscribe() was successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventSubscribe() was not successful");
          tet_printf("saEvtEventSubscribe() was not successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\nCase 56: End");             

  
    
}

#if 0

void tet_AttributesGet_NoMemory()
{
  SaEvtEventPatternArrayT *tempPatternArray=
    (SaEvtEventPatternArrayT *)malloc(sizeof(SaEvtEventPatternArrayT));
  SaNameT getPublishName;

  printf("\n\n******Case 57: Attributes Get with no memory******");

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);

  tempPatternArray->gl_allocatedNumber=(SaSizeT)malloc(sizeof(SaSizeT));
  tempPatternArray->patternsNumber=(SaSizeT)malloc(sizeof(SaSizeT));
  tempPatternArray->patterns=
    (SaEvtEventPatternT *)malloc(sizeof(SaEvtEventPatternT));
  tempPatternArray->patterns->allocatedSize=(SaSizeT)malloc(sizeof(SaSizeT));
  tempPatternArray->patterns->patternSize=(SaSizeT)malloc(sizeof(SaSizeT));
  tempPatternArray->patterns->pattern=(SaUint8T *)malloc(10);
  gl_rc=saEvtEventAttributesGet(gl_eventHandle,tempPatternArray,&gl_priority,
                                &gl_retentionTime,&getPublishName,
                                &gl_publishTime,&gl_evtId);
  resultSuccess("saEvtEventAttributesGet() invoked");

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  printf("\n\nCase 57: End");
}


void tet_DataGet_struct()
{
  struct gl_list *temp;

  printf("\n\n*******Case 58: Data Get with struct******");

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);
  
  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);


  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);

  gl_eventDataSize=sizeof(temp);
  gl_rc=saEvtEventPublish(gl_eventHandle,&temp,gl_eventDataSize,&gl_evtId);
  resultSuccess("saEvtEventPublish() invoked",gl_rc);
  sleep(3);
  thread_creation(&gl_evtHandle);
  
  tet_saEvtEventFree(&gl_eventHandle);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  printf("\n\nCase 58: End");

}
#endif

void tet_MultiChannelOpen()
{
  int j=0;
  SaEvtChannelHandleT tempChannelHandle,syncChannelHandle;

  printf("\n\n******Case 57: Multi Channel Open******");
  tet_printf(" M:57: Three Channels Opened Sync:create Sync:publisher\
 Async:create "); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);
  thread_creation(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpen(&gl_evtHandle,&syncChannelHandle);
  if(gl_rc==SA_AIS_OK)
    {
      j++;
    }
  printf("\n\nChannel Handle : %llu",syncChannelHandle);

  
    

  gl_channelOpenFlags=SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&tempChannelHandle);
  if(gl_rc==SA_AIS_OK)
    {
      j++;
    }
  printf("\n\nTemp Channel Handle : %llu",tempChannelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);
  
    

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE;
  tet_saEvtChannelOpenAsync(&gl_evtHandle);

  
    
   DISPATCH_SLEEP();
  printf("\n\nAsync Channel Handle : %llu",gl_channelHandle);
  if(gl_channelHandle)
    {
      j++;
    }

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelClose(&syncChannelHandle);

  tet_saEvtChannelClose(&tempChannelHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(j==3&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtChannelOpen() was successful");
      tet_printf("saEvtChannelOpen() was successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtChannelOpen() was not successful");
          tet_printf("saEvtChannelOpen() was not successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\nCase 57: End");

  
    
}

void tet_DataGet_without_Memory()
{
  int i;
  printf("\n\n******Case 58: Data get with no memory******");
  tet_printf(" M:58: Not able to Get Event Data with No Memory allocated "); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);
  thread_creation(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_retentionTime=10000000000.0;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);

  
    
   strcpy (gl_eventData,"Memory Test.");
  tet_saEvtEventPublish(&gl_eventHandle,&gl_evtId);

  gl_filterArray.filtersNumber=1;
  gl_filterArray.filters=gl_filter;
  
  tet_saEvtEventSubscribe(&gl_channelHandle,&gl_subscriptionId);
  sleep(1);

  
  gl_eventDataSize=0;
  i=saEvtEventDataGet(gl_eventDeliverHandle,&gl_eventData,&gl_eventDataSize);

  tet_saEvtEventRetentionTimeClear(&gl_channelHandle,&gl_evtId);

  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);
  
  tet_saEvtFinalize(&gl_evtHandle);

  if(i==15&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventDataGet() was not successful");
      tet_printf("saEvtEventDataGet() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventDataGet() was successful");
          tet_printf("saEvtEventDataGet() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\nCase 58: End");

  
    
}
#if 0
void tet_EventFree_ChannelClose() 
{
  int i;
  printf("\n\n*******Case 61: Event free after channel close******");
  tet_printf(" M:61: Not able to Free Event After Channel Closed "); 

  var_initialize();
  

  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=SA_EVT_CHANNEL_CREATE|SA_EVT_CHANNEL_SUBSCRIBER
    |SA_EVT_CHANNEL_PUBLISHER;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);

  tet_saEvtEventAllocate(&gl_channelHandle,&gl_eventHandle);

  gl_patternArray.patternsNumber=2;
  gl_patternArray.patterns=gl_pattern;
  gl_allocatedNumber=2;
  gl_patternLength=5;
  tet_saEvtEventAttributesSet(&gl_eventHandle);


  tet_saEvtChannelClose(&gl_channelHandle);

   
  i=saEvtEventFree(gl_eventHandle);
 
  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==9&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventFree() was not successful");
      tet_printf("saEvtEventFree() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventFree() was successful");
          tet_printf("saEvtEventFree() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\nCase 61: End");

}

void tet_ChannelOpen_ChannelUnlink()
{
  SaEvtChannelHandleT tempHandle;
  int i;

  printf("\n\n*******Case 62: Channel open after channel unlink******");
  var_initialize();
  tet_saEvtInitialize(&gl_evtHandle);

  gl_channelOpenFlags=4;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);
  tempHandle=gl_channelHandle;

  gl_channelOpenFlags=1;
  tet_saEvtChannelOpen(&gl_evtHandle,&gl_channelHandle);


  tet_saEvtChannelClose(&gl_channelHandle);

  tet_saEvtChannelUnlink(&gl_evtHandle);

  tet_saEvtChannelClose(&tempHandle);
  i=gl_rc;

  tet_saEvtFinalize(&gl_evtHandle);

  if(i==1&&gl_error==1)
    {
      printf("\nTest Case: PASSED");
      printf("\nsaEvtEventFree() was not successful");
      tet_printf("saEvtEventFree() was not successful");
      tet_result(TET_PASS);
    }
  else
    {
      if(gl_error==1)
        {
          printf("\nTest Case: FAILED");      
          printf("\nsaEvtEventFree() was successful");
          tet_printf("saEvtEventFree() was successful");
          tet_result(TET_FAIL);
        }
      else
        {
          printf("\nTest Case: UNRESOLVED");
          printf("\nAPI call has failed");
          tet_printf("\nTest Case: UNRESOLVED");
          tet_infoline("API call has failed");
          tet_result(TET_UNRESOLVED);
        }
    }

  printf("\n\nCase 62: End");

}
#endif
