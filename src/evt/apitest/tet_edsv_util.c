/*****************************************************************************************
#
# modification:
# 2008-11   Jony <jiangnan_hw@huawei.com>  Huawei Technologies CO., LTD.
#   1.  Provide test case tet_startup and tet_cleanup functions. The tet_startup function 
#       is defined as tet_edsv_startup(), as well as the tet_cleanup function is defined 
#       as tet_edsv_end().
#   2.  If "tetware-patch" is not used, Combine edsv_test[], api_test[], func_test[] and 
#       b03_test[] arrays into a single array and named it as tet_testlist[]. 
#   3.  If "tetware-patch" is not used, delete the third parameter of each item in test 
#       cases array. And add some functions to implement the function of those parameters. 
#   4.  If "tetware-patch" is not used, delete parts of tet_run_edsv_app() function, which 
#       invoke the functions in the patch.  
#   5.  If "tetware-patch" is required, one can add a macro definition "TET_PATCH=1" in 
#       compilation process.
#
*******************************************************************************************/


#include "tet_startup.h"
#include "ncs_main_papi.h"
#include "tet_eda.h"

void edsv_smoke_test(void);
void tet_run_edsv_app(void);

void tet_edsv_startup(void);
void tet_edsv_end(void);

void (*tet_startup)()=tet_edsv_startup;
void (*tet_cleanup)()=tet_edsv_end;

extern void tet_saEvtLimitGetCases(int iOption);

#if (TET_PATCH==1)
struct tet_testlist edsv_test[]=
  {
    {tet_saEvtInitializeCases,1,1},
    {tet_saEvtInitializeCases,1,2},
    /*    {tet_saEvtInitializeCases,1,3},*/
    {tet_saEvtInitializeCases,1,4},
    {tet_saEvtInitializeCases,1,5},
    {tet_saEvtInitializeCases,1,6},
    {tet_saEvtInitializeCases,1,7},
    {tet_saEvtInitializeCases,1,8},
    {tet_saEvtInitializeCases,1,9},
    {tet_saEvtInitializeCases,1,10},
    {tet_saEvtInitializeCases,1,11},
    {tet_saEvtInitializeCases,1,12},
    {tet_saEvtInitializeCases,1,13},
    {tet_saEvtInitializeCases,1,14},

    {tet_saEvtSelectionObjectGetCases,2,1},
    {tet_saEvtSelectionObjectGetCases,2,2},
    {tet_saEvtSelectionObjectGetCases,2,3},
    {tet_saEvtSelectionObjectGetCases,2,4},
    {tet_saEvtSelectionObjectGetCases,2,5},
    {tet_saEvtSelectionObjectGetCases,2,6},
    {tet_saEvtSelectionObjectGetCases,2,7},
    {tet_saEvtSelectionObjectGetCases,2,8},
    {tet_saEvtSelectionObjectGetCases,2,9},

    {tet_saEvtDispatchCases,3,1},
    {tet_saEvtDispatchCases,3,2},
    {tet_saEvtDispatchCases,3,3},
    {tet_saEvtDispatchCases,3,4},
    {tet_saEvtDispatchCases,3,5},
    {tet_saEvtDispatchCases,3,6},
    {tet_saEvtDispatchCases,3,7},
    /*  {tet_saEvtDispatchCases,3,8},*/
    {tet_saEvtDispatchCases,3,9},

    {tet_saEvtFinalizeCases,4,1},
    {tet_saEvtFinalizeCases,4,2},
    {tet_saEvtFinalizeCases,4,3},
    {tet_saEvtFinalizeCases,4,4},

    {tet_saEvtChannelOpenCases,5,1},
    {tet_saEvtChannelOpenCases,5,2},
    {tet_saEvtChannelOpenCases,5,3},
    {tet_saEvtChannelOpenCases,5,4},
    {tet_saEvtChannelOpenCases,5,5},
    {tet_saEvtChannelOpenCases,5,6},
    {tet_saEvtChannelOpenCases,5,7},
    {tet_saEvtChannelOpenCases,5,8},
    {tet_saEvtChannelOpenCases,5,9},
    {tet_saEvtChannelOpenCases,5,10},
    {tet_saEvtChannelOpenCases,5,11},
    {tet_saEvtChannelOpenCases,5,12},
    {tet_saEvtChannelOpenCases,5,13},
    {tet_saEvtChannelOpenCases,5,14},
    {tet_saEvtChannelOpenCases,5,15},
    {tet_saEvtChannelOpenCases,5,16},

    {tet_saEvtChannelOpenAsyncCases,6,1},
    {tet_saEvtChannelOpenAsyncCases,6,2},
    {tet_saEvtChannelOpenAsyncCases,6,3},
    {tet_saEvtChannelOpenAsyncCases,6,4},
    {tet_saEvtChannelOpenAsyncCases,6,5},
    {tet_saEvtChannelOpenAsyncCases,6,6},
    {tet_saEvtChannelOpenAsyncCases,6,7},
    {tet_saEvtChannelOpenAsyncCases,6,8},
    {tet_saEvtChannelOpenAsyncCases,6,9},
    {tet_saEvtChannelOpenAsyncCases,6,10},
    {tet_saEvtChannelOpenAsyncCases,6,11},
    /*  {tet_saEvtChannelOpenAsyncCases,6,12},*/
    {tet_saEvtChannelOpenAsyncCases,6,13},
    {tet_saEvtChannelOpenAsyncCases,6,14},
    {tet_saEvtChannelOpenAsyncCases,6,15},
    {tet_saEvtChannelOpenAsyncCases,6,16},
    {tet_saEvtChannelOpenAsyncCases,6,17},
    {tet_saEvtChannelOpenAsyncCases,6,18},

    {tet_saEvtChannelCloseCases,7,1},
    {tet_saEvtChannelCloseCases,7,2},
    {tet_saEvtChannelCloseCases,7,3},
    {tet_saEvtChannelCloseCases,7,4},

    {tet_saEvtChannelUnlinkCases,8,1},
    {tet_saEvtChannelUnlinkCases,8,2},
    {tet_saEvtChannelUnlinkCases,8,3},
    {tet_saEvtChannelUnlinkCases,8,4},
    {tet_saEvtChannelUnlinkCases,8,5},
    {tet_saEvtChannelUnlinkCases,8,6},
    {tet_saEvtChannelUnlinkCases,8,7},
    {tet_saEvtChannelUnlinkCases,8,8},

    {tet_saEvtEventAllocateCases,9,1},
    {tet_saEvtEventAllocateCases,9,2},
    {tet_saEvtEventAllocateCases,9,3},
    {tet_saEvtEventAllocateCases,9,4},
    {tet_saEvtEventAllocateCases,9,5},
    {tet_saEvtEventAllocateCases,9,6},
    {tet_saEvtEventAllocateCases,9,7},

    {tet_saEvtEventFreeCases,10,1},
    {tet_saEvtEventFreeCases,10,2},
    {tet_saEvtEventFreeCases,10,3},
    {tet_saEvtEventFreeCases,10,4},
    {tet_saEvtEventFreeCases,10,5},
    {tet_saEvtEventFreeCases,10,6},

    {tet_saEvtEventAttributesSetCases,11,1},
    {tet_saEvtEventAttributesSetCases,11,2},
    {tet_saEvtEventAttributesSetCases,11,3},
    {tet_saEvtEventAttributesSetCases,11,4},
    {tet_saEvtEventAttributesSetCases,11,5},
    {tet_saEvtEventAttributesSetCases,11,6},
    {tet_saEvtEventAttributesSetCases,11,7},
    {tet_saEvtEventAttributesSetCases,11,8},
    {tet_saEvtEventAttributesSetCases,11,9},
    {tet_saEvtEventAttributesSetCases,11,10},
    {tet_saEvtEventAttributesSetCases,11,11},
    {tet_saEvtEventAttributesSetCases,11,12},
    {tet_saEvtEventAttributesSetCases,11,13},
    {tet_saEvtEventAttributesSetCases,11,14},
    {tet_saEvtEventAttributesSetCases,11,15},
    {tet_saEvtEventAttributesSetCases,11,16},
    {tet_saEvtEventAttributesSetCases,11,17},

    {tet_saEvtEventAttributesGetCases,12,1},
    {tet_saEvtEventAttributesGetCases,12,2},
    {tet_saEvtEventAttributesGetCases,12,3},
    {tet_saEvtEventAttributesGetCases,12,4},
    {tet_saEvtEventAttributesGetCases,12,5},
    {tet_saEvtEventAttributesGetCases,12,6},
    {tet_saEvtEventAttributesGetCases,12,7},
    {tet_saEvtEventAttributesGetCases,12,8},
    {tet_saEvtEventAttributesGetCases,12,9},
    {tet_saEvtEventAttributesGetCases,12,10},
    {tet_saEvtEventAttributesGetCases,12,11},
    {tet_saEvtEventAttributesGetCases,12,12},
    {tet_saEvtEventAttributesGetCases,12,13},
    {tet_saEvtEventAttributesGetCases,12,14},
    
    {tet_saEvtEventDataGetCases,13,1},
    {tet_saEvtEventDataGetCases,13,2},
    {tet_saEvtEventDataGetCases,13,3},
   /* {tet_saEvtEventDataGetCases,13,4},
      {tet_saEvtEventDataGetCases,13,5},
      {tet_saEvtEventDataGetCases,13,6},*/
    {tet_saEvtEventDataGetCases,13,7},
    {tet_saEvtEventDataGetCases,13,8},
    {tet_saEvtEventDataGetCases,13,9},
    {tet_saEvtEventDataGetCases,13,10},

    {tet_saEvtEventPublishCases,14,1},  
    {tet_saEvtEventPublishCases,14,2},
    {tet_saEvtEventPublishCases,14,3},
    {tet_saEvtEventPublishCases,14,4},
    {tet_saEvtEventPublishCases,14,5},
    {tet_saEvtEventPublishCases,14,6},
    {tet_saEvtEventPublishCases,14,7},
    {tet_saEvtEventPublishCases,14,8},
    {tet_saEvtEventPublishCases,14,9},
    {tet_saEvtEventPublishCases,14,10},
    {tet_saEvtEventPublishCases,14,11},
    {tet_saEvtEventPublishCases,14,12},
    {tet_saEvtEventPublishCases,14,13},

    {tet_saEvtEventSubscribeCases,15,1},
    {tet_saEvtEventSubscribeCases,15,2},
    {tet_saEvtEventSubscribeCases,15,3},
    {tet_saEvtEventSubscribeCases,15,4},
    {tet_saEvtEventSubscribeCases,15,5},
    {tet_saEvtEventSubscribeCases,15,6},
    {tet_saEvtEventSubscribeCases,15,7},
    {tet_saEvtEventSubscribeCases,15,8},
    {tet_saEvtEventSubscribeCases,15,9},
    {tet_saEvtEventSubscribeCases,15,10},
    {tet_saEvtEventSubscribeCases,15,11},
    {tet_saEvtEventSubscribeCases,15,12},
    {tet_saEvtEventSubscribeCases,15,13},

    {tet_saEvtEventUnsubscribeCases,16,1},
    {tet_saEvtEventUnsubscribeCases,16,2},
    {tet_saEvtEventUnsubscribeCases,16,3},
    {tet_saEvtEventUnsubscribeCases,16,4},
    {tet_saEvtEventUnsubscribeCases,16,5},
    {tet_saEvtEventUnsubscribeCases,16,6},
    {tet_saEvtEventUnsubscribeCases,16,7},
    {tet_saEvtEventUnsubscribeCases,16,8},

    {tet_saEvtEventRetentionTimeClearCases,17,1},
    {tet_saEvtEventRetentionTimeClearCases,17,2},
    {tet_saEvtEventRetentionTimeClearCases,17,3},
    {tet_saEvtEventRetentionTimeClearCases,17,4},
    {tet_saEvtEventRetentionTimeClearCases,17,5},
    {tet_saEvtEventRetentionTimeClearCases,17,6},
    {tet_saEvtEventRetentionTimeClearCases,17,7},
    {tet_saEvtEventRetentionTimeClearCases,17,8},
    {tet_saEvtEventRetentionTimeClearCases,17,9},
    {tet_saEvtEventRetentionTimeClearCases,17,10},
    {tet_saEvtEventRetentionTimeClearCases,17,11},
    {NULL,0}
  };
struct tet_testlist api_test[]=
  {
    {tet_Initialize,1},
    {tet_ChannelOpen,2},
    {tet_ChannelOpenAsync,3},
    {tet_Subscribe,4},
    {tet_Allocate,5},
    {tet_AttributesSet,6},
    {tet_Publish,7},
    {tet_SelectionObjectGet,8},
    {tet_Dispatch,9},
    {tet_AttributesGet,10},
    {tet_DataGet,11},
    {tet_Unsubscribe,12},
    {tet_RetentionTimeClear,13},
    {tet_Free,14},
    {tet_ChannelClose,15},
    {tet_ChannelUnlink,16},
    {tet_Finalize,17},
    {NULL,0}
  };
struct tet_testlist func_test[]=
  {
    {tet_ChannelOpen_SingleEvtHandle,1},
#if 1 
    {tet_ChannelOpen_SingleEvtHandle_SamePatterns,2},
    {tet_ChannelOpen_SingleEvtHandle_DifferentPatterns,3},
    {tet_ChannelOpen_MultipleEvtHandle,4},
    {tet_ChannelOpen_MultipleEvtHandle_SamePatterns,5}, /*run in a loop failing*/
    {tet_ChannelOpen_MultipleEvtHandle_DifferentPatterns,6},
    {tet_ChannelOpen_MultipleOpen,7},
    {tet_ChannelClose_Simple,8},
    {tet_ChannelClose_Subscriber,9},
    {tet_ChannelUnlink_Simple,10},
    {tet_ChannelUnlink_OpenInstance,11},
    {tet_EventFree_Simple,12},
    {tet_AttributesSet_DeliverCallback,13},
    {tet_AttributesGet_ReceivedEvent,14},
    {tet_EventPublish_NullPatternArray,15}, 
    {tet_EventSubscribe_PrefixFilter,16},
    {tet_EventSubscribe_SuffixFilter,17},
    {tet_EventSubscribe_ExactFilter,18},
    {tet_EventSubscribe_AllPassFilter,19},
    {tet_EventSubscribe_LessFilters,20},
    {tet_EventSubscribe_LessPatterns,21},
    {tet_EventSubscribe_LessPatterns_FilterSize,22},
    {tet_EventSubscribe_LessFilters_DifferentOrder,23},
    {tet_EventSubscribe_PatternMatch_DifferentSubscriptions,24}, /*snmp printfs added till here*/
    {tet_ChannelOpenAsync_Simple,25},
    {tet_EventSubscribe_multipleFilters,26},
    {tet_EventSubscribe_PrefixFilter_NoMatch,27},
    {tet_EventSubscribe_SuffixFilter_NoMatch,28},
    {tet_EventSubscribe_ExactFilter_NoMatch,29},
    {tet_EventSubscribe_DiffFilterTypes,30},
    {tet_EventSubscribe_PrefixFilter_FilterSize,31},
    {tet_EventPublish_NullPatternArray_AttributesSet,32},
    {tet_Complex_Subscriber_Publisher,33},
    {tet_Same_Subscriber_Publisher_DiffChannel,34},
    {tet_Sync_Async_Subscriber_Publisher,35},
    {tet_Event_Subscriber_Unsubscriber,36},
    {tet_EventDeliverCallback_AttributesSet_RetentionTimeClear,37},
    {tet_MultipleHandle_Subscribers_Publisher,38},
    {tet_EventPublish_Priority,39},
    {tet_EventPublish_EventOrder,40},
    {tet_EventPublish_EventFree,41},
    {tet_EventFree_DeliverCallback,42},
    {tet_EventRetentionTimeClear_EventFree,43},
    {tet_EventPublish_ChannelUnlink,44},
    {tet_EventUnsubscribe_EventPublish,45},
    {tet_EventAttributesGet_LessPatternsNumber,46},
    {tet_Simple_Test,47},
#endif
#if gl_red == 1
#endif
    {NULL,0}
  };

struct tet_testlist b03_test[]=
  {
    {tet_saEvtInitializeCases,1,15},
#if 0 
    {tet_saEvtInitializeCases,1,16}, /* Manual test for SA_AIS_ERR_UNAVAILABLE */
#endif
    {tet_saEvtLimitGetCases,2,1},
    {tet_saEvtLimitGetCases,2,2},
    {tet_saEvtLimitGetCases,2,3},
    {tet_saEvtLimitGetCases,2,4},
    {tet_saEvtLimitGetCases,2,5},
    {tet_saEvtLimitGetCases,2,6},
    {tet_saEvtLimitGetCases,2,7},
    {tet_saEvtLimitGetCases,2,8},
    {tet_saEvtLimitGetCases,2,9},
    {tet_saEvtEventAttributesGetCases,3,15},/*saEvtEventPatternFree cases by EventAlloc*/
    {tet_saEvtEventAttributesGetCases,3,16},
    {tet_saEvtEventAttributesGetCases,3,17},
    {tet_PatternFree_ReceivedEvent,18},
    {NULL,0}
  };

struct tet_testlist *edsv_testlist[]=
  {
    [EDSV_TEST]=edsv_test,
    [EDSV_API_TEST] = api_test,
    [EDSV_FUNC_TEST] = func_test,
    [EDSV_CLM_TEST] = b03_test,
  };


void tet_edsv_cleanup()
{
  tet_test_cleanup();
  return;
}


void tet_run_edsv_app()
{
  int iterCount=0,listCount=0;
  gl_defs();
#if 1 
#ifndef TET_ALL

  for(iterCount=1;iterCount<=gl_iteration;iterCount++)
    {
      printf("\n---------------- ITERATION : %d --------------\n",iterCount);
      tet_printf("\n-------------- ITERATION : %d --------------\n",iterCount);
      if(gl_listNumber==-1)
        {
            {
              for(listCount=1;listCount<=4;listCount++)
                {
                  tet_test_start(gl_tCase,edsv_testlist[listCount]);
                }
            }
        }
      else
        {
          if(gl_listNumber!=4)
            {
              tet_test_start(gl_tCase,edsv_testlist[gl_listNumber]);
            }
          
        }
      printf("\nNumber of iterations: %d",iterCount);
      tet_printf("\nNumber of iterations: %d",iterCount);
      sleep(2);
    }
  printf("\nPRESS ENTER TO GET MEM DUMP");

  getchar();

  tware_mem_dump();
  sleep(2);
  tware_mem_dump();
  tet_edsv_cleanup(); 

#else

  while(1)
    {
      tet_test_start(gl_tCase,edsv_test);

      tet_test_start(gl_tCase,api_test);

      tet_test_start(gl_tCase,func_test);

      tet_test_start(gl_tCase,b03_test);
    }
#endif
#endif
}

void tet_run_edsv_dist_cases()
{
}

#else

/********** Test Case************/
void tet_saEvtInitializeCases_01()
{
  tet_saEvtInitializeCases(1);
}

void tet_saEvtInitializeCases_02()
{
  tet_saEvtInitializeCases(2);
}

void tet_saEvtInitializeCases_03()
{
  tet_saEvtInitializeCases(3);
}

void tet_saEvtInitializeCases_04()
{
  tet_saEvtInitializeCases(4);
}

void tet_saEvtInitializeCases_05()
{
  tet_saEvtInitializeCases(5);
}

void tet_saEvtInitializeCases_06()
{
  tet_saEvtInitializeCases(6);
}

void tet_saEvtInitializeCases_07()
{
  tet_saEvtInitializeCases(7);
}

void tet_saEvtInitializeCases_08()
{
  tet_saEvtInitializeCases(8);
}

void tet_saEvtInitializeCases_09()
{
  tet_saEvtInitializeCases(9);
}

void tet_saEvtInitializeCases_10()
{
  tet_saEvtInitializeCases(10);
}

void tet_saEvtInitializeCases_11()
{
  tet_saEvtInitializeCases(11);
}

void tet_saEvtInitializeCases_12()
{
  tet_saEvtInitializeCases(12);
}

void tet_saEvtInitializeCases_13()
{
  tet_saEvtInitializeCases(13);
}

void tet_saEvtInitializeCases_14()
{
  tet_saEvtInitializeCases(14);
}

void tet_saEvtInitializeCases_15()
{
  tet_saEvtInitializeCases(15);
}

void tet_saEvtSelectionObjectGetCases_01()
{
  tet_saEvtSelectionObjectGetCases(1);
}

void tet_saEvtSelectionObjectGetCases_02()
{
  tet_saEvtSelectionObjectGetCases(2);
}

void tet_saEvtSelectionObjectGetCases_03()
{
  tet_saEvtSelectionObjectGetCases(3);
}

void tet_saEvtSelectionObjectGetCases_04()
{
  tet_saEvtSelectionObjectGetCases(4);
}

void tet_saEvtSelectionObjectGetCases_05()
{
  tet_saEvtSelectionObjectGetCases(5);
}

void tet_saEvtSelectionObjectGetCases_06()
{
  tet_saEvtSelectionObjectGetCases(6);
}

void tet_saEvtSelectionObjectGetCases_07()
{
  tet_saEvtSelectionObjectGetCases(7);
}

void tet_saEvtSelectionObjectGetCases_08()
{
  tet_saEvtSelectionObjectGetCases(8);
}

void tet_saEvtSelectionObjectGetCases_09()
{
  tet_saEvtSelectionObjectGetCases(9);
}

void tet_saEvtDispatchCases_01()
{
  tet_saEvtDispatchCases(1);
}

void tet_saEvtDispatchCases_02()
{
  tet_saEvtDispatchCases(2);
}

void tet_saEvtDispatchCases_03()
{
  tet_saEvtDispatchCases(3);
}

void tet_saEvtDispatchCases_04()
{
  tet_saEvtDispatchCases(4);
}

void tet_saEvtDispatchCases_05()
{
  tet_saEvtDispatchCases(5);
}

void tet_saEvtDispatchCases_06()
{
  tet_saEvtDispatchCases(6);
}

void tet_saEvtDispatchCases_07()
{
  tet_saEvtDispatchCases(7);
}

void tet_saEvtDispatchCases_08()
{
  tet_saEvtDispatchCases(8);
}

void tet_saEvtDispatchCases_09()
{
  tet_saEvtDispatchCases(9);
}

void tet_saEvtFinalizeCases_01()
{
  tet_saEvtFinalizeCases(1);
}

void tet_saEvtFinalizeCases_02()
{
  tet_saEvtFinalizeCases(2);
}

void tet_saEvtFinalizeCases_03()
{
  tet_saEvtFinalizeCases(3);
}

void tet_saEvtFinalizeCases_04()
{
  tet_saEvtFinalizeCases(4);
}

void tet_saEvtChannelOpenCases_01()
{
  tet_saEvtChannelOpenCases(1);
}

void tet_saEvtChannelOpenCases_02()
{
  tet_saEvtChannelOpenCases(2);
}

void tet_saEvtChannelOpenCases_03()
{
  tet_saEvtChannelOpenCases(3);
}

void tet_saEvtChannelOpenCases_04()
{
  tet_saEvtChannelOpenCases(4);
}

void tet_saEvtChannelOpenCases_05()
{
  tet_saEvtChannelOpenCases(5);
}

void tet_saEvtChannelOpenCases_06()
{
  tet_saEvtChannelOpenCases(6);
}

void tet_saEvtChannelOpenCases_07()
{
  tet_saEvtChannelOpenCases(7);
}

void tet_saEvtChannelOpenCases_08()
{
  tet_saEvtChannelOpenCases(8);
}

void tet_saEvtChannelOpenCases_09()
{
  tet_saEvtChannelOpenCases(9);
}

void tet_saEvtChannelOpenCases_10()
{
  tet_saEvtChannelOpenCases(10);
}

void tet_saEvtChannelOpenCases_11()
{
  tet_saEvtChannelOpenCases(11);
}

void tet_saEvtChannelOpenCases_12()
{
  tet_saEvtChannelOpenCases(12);
}

void tet_saEvtChannelOpenCases_13()
{
  tet_saEvtChannelOpenCases(13);
}

void tet_saEvtChannelOpenCases_14()
{
  tet_saEvtChannelOpenCases(14);
}

void tet_saEvtChannelOpenCases_15()
{
  tet_saEvtChannelOpenCases(15);
}

void tet_saEvtChannelOpenCases_16()
{
  tet_saEvtChannelOpenCases(16);
}

void tet_saEvtChannelOpenAsyncCases_01()
{
  tet_saEvtChannelOpenAsyncCases(1);
}

void tet_saEvtChannelOpenAsyncCases_02()
{
  tet_saEvtChannelOpenAsyncCases(2);
}

void tet_saEvtChannelOpenAsyncCases_03()
{
  tet_saEvtChannelOpenAsyncCases(3);
}

void tet_saEvtChannelOpenAsyncCases_04()
{
  tet_saEvtChannelOpenAsyncCases(4);
}

void tet_saEvtChannelOpenAsyncCases_05()
{
  tet_saEvtChannelOpenAsyncCases(5);
}

void tet_saEvtChannelOpenAsyncCases_06()
{
  tet_saEvtChannelOpenAsyncCases(6);
}

void tet_saEvtChannelOpenAsyncCases_07()
{
  tet_saEvtChannelOpenAsyncCases(7);
}

void tet_saEvtChannelOpenAsyncCases_08()
{
  tet_saEvtChannelOpenAsyncCases(8);
}

void tet_saEvtChannelOpenAsyncCases_09()
{
  tet_saEvtChannelOpenAsyncCases(9);
}

void tet_saEvtChannelOpenAsyncCases_10()
{
  tet_saEvtChannelOpenAsyncCases(10);
}

void tet_saEvtChannelOpenAsyncCases_11()
{
  tet_saEvtChannelOpenAsyncCases(11);
}

void tet_saEvtChannelOpenAsyncCases_12()
{
  tet_saEvtChannelOpenAsyncCases(12);
}

void tet_saEvtChannelOpenAsyncCases_13()
{
  tet_saEvtChannelOpenAsyncCases(13);
}

void tet_saEvtChannelOpenAsyncCases_14()
{
  tet_saEvtChannelOpenAsyncCases(14);
}

void tet_saEvtChannelOpenAsyncCases_15()
{
  tet_saEvtChannelOpenAsyncCases(15);
}

void tet_saEvtChannelOpenAsyncCases_16()
{
  tet_saEvtChannelOpenAsyncCases(16);
}

void tet_saEvtChannelOpenAsyncCases_17()
{
  tet_saEvtChannelOpenAsyncCases(17);
}

void tet_saEvtChannelOpenAsyncCases_18()
{
  tet_saEvtChannelOpenAsyncCases(18);
}

void tet_saEvtChannelCloseCases_01()
{
  tet_saEvtChannelCloseCases(1);
}

void tet_saEvtChannelCloseCases_02()
{
  tet_saEvtChannelCloseCases(2);
}

void tet_saEvtChannelCloseCases_03()
{
  tet_saEvtChannelCloseCases(3);
}

void tet_saEvtChannelCloseCases_04()
{
  tet_saEvtChannelCloseCases(4);
}

void tet_saEvtChannelUnlinkCases_01()
{
  tet_saEvtChannelUnlinkCases(1);
}

void tet_saEvtChannelUnlinkCases_02()
{
  tet_saEvtChannelUnlinkCases(2);
}

void tet_saEvtChannelUnlinkCases_03()
{
  tet_saEvtChannelUnlinkCases(3);
}

void tet_saEvtChannelUnlinkCases_04()
{
  tet_saEvtChannelUnlinkCases(4);
}

void tet_saEvtChannelUnlinkCases_05()
{
  tet_saEvtChannelUnlinkCases(5);
}

void tet_saEvtChannelUnlinkCases_06()
{
  tet_saEvtChannelUnlinkCases(6);
}

void tet_saEvtChannelUnlinkCases_07()
{
  tet_saEvtChannelUnlinkCases(7);
}

void tet_saEvtChannelUnlinkCases_08()
{
  tet_saEvtChannelUnlinkCases(8);
}

void tet_saEvtEventAllocateCases_01()
{
  tet_saEvtEventAllocateCases(1);
}

void tet_saEvtEventAllocateCases_02()
{
  tet_saEvtEventAllocateCases(2);
}

void tet_saEvtEventAllocateCases_03()
{
  tet_saEvtEventAllocateCases(3);
}

void tet_saEvtEventAllocateCases_04()
{
  tet_saEvtEventAllocateCases(4);
}

void tet_saEvtEventAllocateCases_05()
{
  tet_saEvtEventAllocateCases(5);
}

void tet_saEvtEventAllocateCases_06()
{
  tet_saEvtEventAllocateCases(6);
}

void tet_saEvtEventAllocateCases_07()
{
  tet_saEvtEventAllocateCases(7);
}

void tet_saEvtEventFreeCases_01()
{
  tet_saEvtEventFreeCases(1);
}

void tet_saEvtEventFreeCases_02()
{
  tet_saEvtEventFreeCases(2);
}

void tet_saEvtEventFreeCases_03()
{
  tet_saEvtEventFreeCases(3);
}

void tet_saEvtEventFreeCases_04()
{
  tet_saEvtEventFreeCases(4);
}

void tet_saEvtEventFreeCases_05()
{
  tet_saEvtEventFreeCases(5);
}

void tet_saEvtEventFreeCases_06()
{
  tet_saEvtEventFreeCases(6);
}

void tet_saEvtEventAttributesSetCases_01()
{
  tet_saEvtEventAttributesSetCases(1);
}

void tet_saEvtEventAttributesSetCases_02()
{
  tet_saEvtEventAttributesSetCases(2);
}

void tet_saEvtEventAttributesSetCases_03()
{
  tet_saEvtEventAttributesSetCases(3);
}

void tet_saEvtEventAttributesSetCases_04()
{
  tet_saEvtEventAttributesSetCases(4);
}

void tet_saEvtEventAttributesSetCases_05()
{
  tet_saEvtEventAttributesSetCases(5);
}

void tet_saEvtEventAttributesSetCases_06()
{
  tet_saEvtEventAttributesSetCases(6);
}

void tet_saEvtEventAttributesSetCases_07()
{
  tet_saEvtEventAttributesSetCases(7);
}

void tet_saEvtEventAttributesSetCases_08()
{
  tet_saEvtEventAttributesSetCases(8);
}

void tet_saEvtEventAttributesSetCases_09()
{
  tet_saEvtEventAttributesSetCases(9);
}

void tet_saEvtEventAttributesSetCases_10()
{
  tet_saEvtEventAttributesSetCases(10);
}

void tet_saEvtEventAttributesSetCases_11()
{
  tet_saEvtEventAttributesSetCases(11);
}

void tet_saEvtEventAttributesSetCases_12()
{
  tet_saEvtEventAttributesSetCases(12);
}

void tet_saEvtEventAttributesSetCases_13()
{
  tet_saEvtEventAttributesSetCases(13);
}

void tet_saEvtEventAttributesSetCases_14()
{
  tet_saEvtEventAttributesSetCases(14);
}

void tet_saEvtEventAttributesSetCases_15()
{
  tet_saEvtEventAttributesSetCases(15);
}

void tet_saEvtEventAttributesSetCases_16()
{
  tet_saEvtEventAttributesSetCases(16);
}

void tet_saEvtEventAttributesSetCases_17()
{
  tet_saEvtEventAttributesSetCases(17);
}

void tet_saEvtEventAttributesGetCases_01()
{
  tet_saEvtEventAttributesGetCases(1);
}

void tet_saEvtEventAttributesGetCases_02()
{
  tet_saEvtEventAttributesGetCases(2);
}

void tet_saEvtEventAttributesGetCases_03()
{
  tet_saEvtEventAttributesGetCases(3);
}

void tet_saEvtEventAttributesGetCases_04()
{
  tet_saEvtEventAttributesGetCases(4);
}

void tet_saEvtEventAttributesGetCases_05()
{
  tet_saEvtEventAttributesGetCases(5);
}

void tet_saEvtEventAttributesGetCases_06()
{
  tet_saEvtEventAttributesGetCases(6);
}

void tet_saEvtEventAttributesGetCases_07()
{
  tet_saEvtEventAttributesGetCases(7);
}

void tet_saEvtEventAttributesGetCases_08()
{
  tet_saEvtEventAttributesGetCases(8);
}

void tet_saEvtEventAttributesGetCases_09()
{
  tet_saEvtEventAttributesGetCases(9);
}

void tet_saEvtEventAttributesGetCases_10()
{
  tet_saEvtEventAttributesGetCases(10);
}

void tet_saEvtEventAttributesGetCases_11()
{
  tet_saEvtEventAttributesGetCases(11);
}

void tet_saEvtEventAttributesGetCases_12()
{
  tet_saEvtEventAttributesGetCases(12);
}

void tet_saEvtEventAttributesGetCases_13()
{
  tet_saEvtEventAttributesGetCases(13);
}

void tet_saEvtEventAttributesGetCases_14()
{
  tet_saEvtEventAttributesGetCases(14);
}

void tet_saEvtEventAttributesGetCases_15()
{
  tet_saEvtEventAttributesGetCases(15);
}

void tet_saEvtEventAttributesGetCases_16()
{
  tet_saEvtEventAttributesGetCases(16);
}

void tet_saEvtEventAttributesGetCases_17()
{
  tet_saEvtEventAttributesGetCases(17);
}

void tet_saEvtEventDataGetCases_01()
{
  tet_saEvtEventDataGetCases(1);
}

void tet_saEvtEventDataGetCases_02()
{
  tet_saEvtEventDataGetCases(2);
}

void tet_saEvtEventDataGetCases_03()
{
  tet_saEvtEventDataGetCases(3);
}

void tet_saEvtEventDataGetCases_04()
{
  tet_saEvtEventDataGetCases(4);
}

void tet_saEvtEventDataGetCases_05()
{
  tet_saEvtEventDataGetCases(5);
}

void tet_saEvtEventDataGetCases_06()
{
  tet_saEvtEventDataGetCases(6);
}

void tet_saEvtEventDataGetCases_07()
{
  tet_saEvtEventDataGetCases(7);
}

void tet_saEvtEventDataGetCases_08()
{
  tet_saEvtEventDataGetCases(8);
}

void tet_saEvtEventDataGetCases_09()
{
  tet_saEvtEventDataGetCases(9);
}

void tet_saEvtEventDataGetCases_10()
{
  tet_saEvtEventDataGetCases(10);
}

void tet_saEvtEventPublishCases_01()
{
  tet_saEvtEventPublishCases(1);
}

void tet_saEvtEventPublishCases_02()
{
  tet_saEvtEventPublishCases(2);
}

void tet_saEvtEventPublishCases_03()
{
  tet_saEvtEventPublishCases(3);
}

void tet_saEvtEventPublishCases_04()
{
  tet_saEvtEventPublishCases(4);
}

void tet_saEvtEventPublishCases_05()
{
  tet_saEvtEventPublishCases(5);
}

void tet_saEvtEventPublishCases_06()
{
  tet_saEvtEventPublishCases(6);
}

void tet_saEvtEventPublishCases_07()
{
  tet_saEvtEventPublishCases(7);
}

void tet_saEvtEventPublishCases_08()
{
  tet_saEvtEventPublishCases(8);
}

void tet_saEvtEventPublishCases_09()
{
  tet_saEvtEventPublishCases(9);
}

void tet_saEvtEventPublishCases_10()
{
  tet_saEvtEventPublishCases(10);
}

void tet_saEvtEventPublishCases_11()
{
  tet_saEvtEventPublishCases(11);
}

void tet_saEvtEventPublishCases_12()
{
  tet_saEvtEventPublishCases(12);
}

void tet_saEvtEventPublishCases_13()
{
  tet_saEvtEventPublishCases(13);
}

void tet_saEvtEventSubscribeCases_01()
{
  tet_saEvtEventSubscribeCases(1);
}

void tet_saEvtEventSubscribeCases_02()
{
  tet_saEvtEventSubscribeCases(2);
}

void tet_saEvtEventSubscribeCases_03()
{
  tet_saEvtEventSubscribeCases(3);
}

void tet_saEvtEventSubscribeCases_04()
{
  tet_saEvtEventSubscribeCases(4);
}

void tet_saEvtEventSubscribeCases_05()
{
  tet_saEvtEventSubscribeCases(5);
}

void tet_saEvtEventSubscribeCases_06()
{
  tet_saEvtEventSubscribeCases(6);
}

void tet_saEvtEventSubscribeCases_07()
{
  tet_saEvtEventSubscribeCases(7);
}

void tet_saEvtEventSubscribeCases_08()
{
  tet_saEvtEventSubscribeCases(8);
}

void tet_saEvtEventSubscribeCases_09()
{
  tet_saEvtEventSubscribeCases(9);
}

void tet_saEvtEventSubscribeCases_10()
{
  tet_saEvtEventSubscribeCases(10);
}

void tet_saEvtEventSubscribeCases_11()
{
  tet_saEvtEventSubscribeCases(11);
}

void tet_saEvtEventSubscribeCases_12()
{
  tet_saEvtEventSubscribeCases(12);
}

void tet_saEvtEventSubscribeCases_13()
{
  tet_saEvtEventSubscribeCases(13);
}

void tet_saEvtEventUnsubscribeCases_01()
{
  tet_saEvtEventUnsubscribeCases(1);
}

void tet_saEvtEventUnsubscribeCases_02()
{
  tet_saEvtEventUnsubscribeCases(2);
}

void tet_saEvtEventUnsubscribeCases_03()
{
  tet_saEvtEventUnsubscribeCases(3);
}

void tet_saEvtEventUnsubscribeCases_04()
{
  tet_saEvtEventUnsubscribeCases(4);
}

void tet_saEvtEventUnsubscribeCases_05()
{
  tet_saEvtEventUnsubscribeCases(5);
}

void tet_saEvtEventUnsubscribeCases_06()
{
  tet_saEvtEventUnsubscribeCases(6);
}

void tet_saEvtEventUnsubscribeCases_07()
{
  tet_saEvtEventUnsubscribeCases(7);
}

void tet_saEvtEventUnsubscribeCases_08()
{
  tet_saEvtEventUnsubscribeCases(8);
}

void tet_saEvtEventRetentionTimeClearCases_01()
{
  tet_saEvtEventRetentionTimeClearCases(1);
}

void tet_saEvtEventRetentionTimeClearCases_02()
{
  tet_saEvtEventRetentionTimeClearCases(2);
}

void tet_saEvtEventRetentionTimeClearCases_03()
{
  tet_saEvtEventRetentionTimeClearCases(3);
}

void tet_saEvtEventRetentionTimeClearCases_04()
{
  tet_saEvtEventRetentionTimeClearCases(4);
}

void tet_saEvtEventRetentionTimeClearCases_05()
{
  tet_saEvtEventRetentionTimeClearCases(5);
}

void tet_saEvtEventRetentionTimeClearCases_06()
{
  tet_saEvtEventRetentionTimeClearCases(6);
}

void tet_saEvtEventRetentionTimeClearCases_07()
{
  tet_saEvtEventRetentionTimeClearCases(7);
}

void tet_saEvtEventRetentionTimeClearCases_08()
{
  tet_saEvtEventRetentionTimeClearCases(8);
}

void tet_saEvtEventRetentionTimeClearCases_09()
{
  tet_saEvtEventRetentionTimeClearCases(9);
}

void tet_saEvtEventRetentionTimeClearCases_10()
{
  tet_saEvtEventRetentionTimeClearCases(10);
}

void tet_saEvtEventRetentionTimeClearCases_11()
{
  tet_saEvtEventRetentionTimeClearCases(11);
}

void tet_saEvtLimitGetCases_01()
{
  tet_saEvtLimitGetCases(1);
}

void tet_saEvtLimitGetCases_02()
{
  tet_saEvtLimitGetCases(2);
}

void tet_saEvtLimitGetCases_03()
{
  tet_saEvtLimitGetCases(3);
}

void tet_saEvtLimitGetCases_04()
{
  tet_saEvtLimitGetCases(4);
}

void tet_saEvtLimitGetCases_05()
{
  tet_saEvtLimitGetCases(5);
}

void tet_saEvtLimitGetCases_06()
{
  tet_saEvtLimitGetCases(6);
}

void tet_saEvtLimitGetCases_07()
{
  tet_saEvtLimitGetCases(7);
}

void tet_saEvtLimitGetCases_08()
{
  tet_saEvtLimitGetCases(8);
}

void tet_saEvtLimitGetCases_09()
{
  tet_saEvtLimitGetCases(9);
}

struct tet_testlist tet_testlist[]=
{
  /********** edsv_test************/
    {tet_saEvtInitializeCases_01,1},
    {tet_saEvtInitializeCases_02,1},
    /*{tet_saEvtInitializeCases_03,1},*/
    {tet_saEvtInitializeCases_04,1},
    {tet_saEvtInitializeCases_05,1},
    {tet_saEvtInitializeCases_06,1},
    {tet_saEvtInitializeCases_07,1},
    {tet_saEvtInitializeCases_08,1},
    {tet_saEvtInitializeCases_09,1},
    {tet_saEvtInitializeCases_10,1},
    {tet_saEvtInitializeCases_11,1},
    {tet_saEvtInitializeCases_12,1},
    {tet_saEvtInitializeCases_13,1},
    {tet_saEvtInitializeCases_14,1},

    {tet_saEvtSelectionObjectGetCases_01,2},
    {tet_saEvtSelectionObjectGetCases_02,2},
    {tet_saEvtSelectionObjectGetCases_03,2},
    {tet_saEvtSelectionObjectGetCases_04,2},
    {tet_saEvtSelectionObjectGetCases_05,2},
    {tet_saEvtSelectionObjectGetCases_06,2},
    {tet_saEvtSelectionObjectGetCases_07,2},
    {tet_saEvtSelectionObjectGetCases_08,2},
    {tet_saEvtSelectionObjectGetCases_09,2},

    {tet_saEvtDispatchCases_01,3},
    {tet_saEvtDispatchCases_02,3},
    {tet_saEvtDispatchCases_03,3},
    {tet_saEvtDispatchCases_04,3},
    {tet_saEvtDispatchCases_05,3},
    {tet_saEvtDispatchCases_06,3},
    {tet_saEvtDispatchCases_07,3},
    /*  {tet_saEvtDispatchCases_08,3},*/
    {tet_saEvtDispatchCases_09,3},

    {tet_saEvtFinalizeCases_01,4},
    {tet_saEvtFinalizeCases_02,4},
    {tet_saEvtFinalizeCases_03,4},
    {tet_saEvtFinalizeCases_04,4},

    {tet_saEvtChannelOpenCases_01,5},
    {tet_saEvtChannelOpenCases_02,5},
    {tet_saEvtChannelOpenCases_03,5},
    {tet_saEvtChannelOpenCases_04,5},
    {tet_saEvtChannelOpenCases_05,5},
    {tet_saEvtChannelOpenCases_06,5},
    {tet_saEvtChannelOpenCases_07,5},
    {tet_saEvtChannelOpenCases_08,5},
    {tet_saEvtChannelOpenCases_09,5},
    {tet_saEvtChannelOpenCases_10,5},
    {tet_saEvtChannelOpenCases_11,5},
    {tet_saEvtChannelOpenCases_12,5},
    {tet_saEvtChannelOpenCases_13,5},
    {tet_saEvtChannelOpenCases_14,5},
    {tet_saEvtChannelOpenCases_15,5},
    {tet_saEvtChannelOpenCases_16,5},

    {tet_saEvtChannelOpenAsyncCases_01,6},
    {tet_saEvtChannelOpenAsyncCases_02,6},
    {tet_saEvtChannelOpenAsyncCases_03,6},
    {tet_saEvtChannelOpenAsyncCases_04,6},
    {tet_saEvtChannelOpenAsyncCases_05,6},
    {tet_saEvtChannelOpenAsyncCases_06,6},
    {tet_saEvtChannelOpenAsyncCases_07,6},
    {tet_saEvtChannelOpenAsyncCases_08,6},
    {tet_saEvtChannelOpenAsyncCases_09,6},
    {tet_saEvtChannelOpenAsyncCases_10,6},
    {tet_saEvtChannelOpenAsyncCases_11,6},
    /*  {tet_saEvtChannelOpenAsyncCases_12,6},*/
    {tet_saEvtChannelOpenAsyncCases_13,6},
    {tet_saEvtChannelOpenAsyncCases_14,6},
    {tet_saEvtChannelOpenAsyncCases_15,6},
    {tet_saEvtChannelOpenAsyncCases_16,6},
    {tet_saEvtChannelOpenAsyncCases_17,6},
    {tet_saEvtChannelOpenAsyncCases_18,6},

    {tet_saEvtChannelCloseCases_01,7},
    {tet_saEvtChannelCloseCases_02,7},
    {tet_saEvtChannelCloseCases_03,7},
    {tet_saEvtChannelCloseCases_04,7},

    {tet_saEvtChannelUnlinkCases_01,8},
    {tet_saEvtChannelUnlinkCases_02,8},
    {tet_saEvtChannelUnlinkCases_03,8},
    {tet_saEvtChannelUnlinkCases_04,8},
    {tet_saEvtChannelUnlinkCases_05,8},
    {tet_saEvtChannelUnlinkCases_06,8},
    {tet_saEvtChannelUnlinkCases_07,8},
    {tet_saEvtChannelUnlinkCases_08,8},

    {tet_saEvtEventAllocateCases_01,9},
    {tet_saEvtEventAllocateCases_02,9},
    {tet_saEvtEventAllocateCases_03,9},
    {tet_saEvtEventAllocateCases_04,9},
    {tet_saEvtEventAllocateCases_05,9},
    {tet_saEvtEventAllocateCases_06,9},
    {tet_saEvtEventAllocateCases_07,9},

    {tet_saEvtEventFreeCases_01,10},
    {tet_saEvtEventFreeCases_02,10},
    {tet_saEvtEventFreeCases_03,10},
    {tet_saEvtEventFreeCases_04,10},
    {tet_saEvtEventFreeCases_05,10},
    {tet_saEvtEventFreeCases_06,10},

    {tet_saEvtEventAttributesSetCases_01,11},
    {tet_saEvtEventAttributesSetCases_02,11},
    {tet_saEvtEventAttributesSetCases_03,11},
    {tet_saEvtEventAttributesSetCases_04,11},
    {tet_saEvtEventAttributesSetCases_05,11},
    {tet_saEvtEventAttributesSetCases_06,11},
    {tet_saEvtEventAttributesSetCases_07,11},
    {tet_saEvtEventAttributesSetCases_08,11},
    {tet_saEvtEventAttributesSetCases_09,11},
    {tet_saEvtEventAttributesSetCases_10,11},
    {tet_saEvtEventAttributesSetCases_11,11},
    {tet_saEvtEventAttributesSetCases_12,11},
    {tet_saEvtEventAttributesSetCases_13,11},
    {tet_saEvtEventAttributesSetCases_14,11},
    {tet_saEvtEventAttributesSetCases_15,11},
    {tet_saEvtEventAttributesSetCases_16,11},
    {tet_saEvtEventAttributesSetCases_17,11},

    {tet_saEvtEventAttributesGetCases_01,12},
    {tet_saEvtEventAttributesGetCases_02,12},
    {tet_saEvtEventAttributesGetCases_03,12},
    {tet_saEvtEventAttributesGetCases_04,12},
    {tet_saEvtEventAttributesGetCases_05,12},
    {tet_saEvtEventAttributesGetCases_06,12},
    {tet_saEvtEventAttributesGetCases_07,12},
    {tet_saEvtEventAttributesGetCases_08,12},
    {tet_saEvtEventAttributesGetCases_09,12},
    {tet_saEvtEventAttributesGetCases_10,12},
    {tet_saEvtEventAttributesGetCases_11,12},
    {tet_saEvtEventAttributesGetCases_12,12},
    {tet_saEvtEventAttributesGetCases_13,12},
    {tet_saEvtEventAttributesGetCases_14,12},
    
    {tet_saEvtEventDataGetCases_01,13},
    {tet_saEvtEventDataGetCases_02,13},
    {tet_saEvtEventDataGetCases_03,13},
   /* {tet_saEvtEventDataGetCases_04,13},
      {tet_saEvtEventDataGetCases_05,13},
      {tet_saEvtEventDataGetCases_06,13},*/
    {tet_saEvtEventDataGetCases_07,13},
    {tet_saEvtEventDataGetCases_08,13},
    {tet_saEvtEventDataGetCases_09,13},
    {tet_saEvtEventDataGetCases_10,13},

    {tet_saEvtEventPublishCases_01,14},  
    {tet_saEvtEventPublishCases_02,14},
    {tet_saEvtEventPublishCases_03,14},
    {tet_saEvtEventPublishCases_04,14},
    {tet_saEvtEventPublishCases_05,14},
    {tet_saEvtEventPublishCases_06,14},
    {tet_saEvtEventPublishCases_07,14},
    {tet_saEvtEventPublishCases_08,14},
    {tet_saEvtEventPublishCases_09,14},
    {tet_saEvtEventPublishCases_10,14},
    {tet_saEvtEventPublishCases_11,14},
    {tet_saEvtEventPublishCases_12,14},
    {tet_saEvtEventPublishCases_13,14},

    {tet_saEvtEventSubscribeCases_01,15},
    {tet_saEvtEventSubscribeCases_02,15},
    {tet_saEvtEventSubscribeCases_03,15},
    {tet_saEvtEventSubscribeCases_04,15},
    {tet_saEvtEventSubscribeCases_05,15},
    {tet_saEvtEventSubscribeCases_06,15},
    {tet_saEvtEventSubscribeCases_07,15},
    {tet_saEvtEventSubscribeCases_08,15},
    {tet_saEvtEventSubscribeCases_09,15},
    {tet_saEvtEventSubscribeCases_10,15},
    {tet_saEvtEventSubscribeCases_11,15},
    {tet_saEvtEventSubscribeCases_12,15},
    {tet_saEvtEventSubscribeCases_13,15},

    {tet_saEvtEventUnsubscribeCases_01,16},
    {tet_saEvtEventUnsubscribeCases_02,16},
    {tet_saEvtEventUnsubscribeCases_03,16},
    {tet_saEvtEventUnsubscribeCases_04,16},
    {tet_saEvtEventUnsubscribeCases_05,16},
    {tet_saEvtEventUnsubscribeCases_06,16},
    {tet_saEvtEventUnsubscribeCases_07,16},
    {tet_saEvtEventUnsubscribeCases_08,16},

    {tet_saEvtEventRetentionTimeClearCases_01,17},
    {tet_saEvtEventRetentionTimeClearCases_02,17},
    {tet_saEvtEventRetentionTimeClearCases_03,17},
    {tet_saEvtEventRetentionTimeClearCases_04,17},
    {tet_saEvtEventRetentionTimeClearCases_05,17},
    {tet_saEvtEventRetentionTimeClearCases_06,17},
    {tet_saEvtEventRetentionTimeClearCases_07,17},
    {tet_saEvtEventRetentionTimeClearCases_08,17},
    {tet_saEvtEventRetentionTimeClearCases_09,17},
    {tet_saEvtEventRetentionTimeClearCases_10,17},
    {tet_saEvtEventRetentionTimeClearCases_11,17},

/********** api_test************/
    {tet_Initialize,18},
    {tet_ChannelOpen,18},
    {tet_ChannelOpenAsync,18},
    {tet_Subscribe,18},
    {tet_Allocate,18},
    {tet_AttributesSet,18},
    {tet_Publish,18},
    {tet_SelectionObjectGet,18},
    {tet_Dispatch,18},
    {tet_AttributesGet,18},
    {tet_DataGet,18},
    {tet_Unsubscribe,18},
    {tet_RetentionTimeClear,18},
    {tet_Free,18},
    {tet_ChannelClose,18},
    {tet_ChannelUnlink,18},
    {tet_Finalize,18},

/********** func_test************/
    {tet_ChannelOpen_SingleEvtHandle,19},
    {tet_ChannelOpen_SingleEvtHandle_SamePatterns,19},
    {tet_ChannelOpen_SingleEvtHandle_DifferentPatterns,19},
    {tet_ChannelOpen_MultipleEvtHandle,19},
    {tet_ChannelOpen_MultipleEvtHandle_SamePatterns,19}, /*run in a loop failing*/
    {tet_ChannelOpen_MultipleEvtHandle_DifferentPatterns,19},
    {tet_ChannelOpen_MultipleOpen,19},
    {tet_ChannelClose_Simple,19},
    {tet_ChannelClose_Subscriber,19},
    {tet_ChannelUnlink_Simple,19},
    {tet_ChannelUnlink_OpenInstance,19},
    {tet_EventFree_Simple,19},
    {tet_AttributesSet_DeliverCallback,19},
    {tet_AttributesGet_ReceivedEvent,19},
    {tet_EventPublish_NullPatternArray,19}, 
    {tet_EventSubscribe_PrefixFilter,19},
    {tet_EventSubscribe_SuffixFilter,19},
    {tet_EventSubscribe_ExactFilter,19},
    {tet_EventSubscribe_AllPassFilter,19},
    {tet_EventSubscribe_LessFilters,19},
    {tet_EventSubscribe_LessPatterns,19},
    {tet_EventSubscribe_LessPatterns_FilterSize,19},
    {tet_EventSubscribe_LessFilters_DifferentOrder,19},
    {tet_EventSubscribe_PatternMatch_DifferentSubscriptions,19}, /*snmp printfs added till here*/
    {tet_ChannelOpenAsync_Simple,19},
    {tet_EventSubscribe_multipleFilters,19},
    {tet_EventSubscribe_PrefixFilter_NoMatch,19},
    {tet_EventSubscribe_SuffixFilter_NoMatch,19},
    {tet_EventSubscribe_ExactFilter_NoMatch,19},
    {tet_EventSubscribe_DiffFilterTypes,19},
    {tet_EventSubscribe_PrefixFilter_FilterSize,19},
    {tet_EventPublish_NullPatternArray_AttributesSet,19},
    {tet_Complex_Subscriber_Publisher,19},
    {tet_Same_Subscriber_Publisher_DiffChannel,19},
    {tet_Sync_Async_Subscriber_Publisher,19},
    {tet_Event_Subscriber_Unsubscriber,19},
    {tet_EventDeliverCallback_AttributesSet_RetentionTimeClear,19},
    {tet_MultipleHandle_Subscribers_Publisher,19},
    {tet_EventPublish_Priority,19},
    {tet_EventPublish_EventOrder,19},
    {tet_EventPublish_EventFree,19},
    {tet_EventFree_DeliverCallback,19},
    {tet_EventRetentionTimeClear_EventFree,19},
    {tet_EventPublish_ChannelUnlink,19},
    {tet_EventUnsubscribe_EventPublish,19},
    {tet_EventAttributesGet_LessPatternsNumber,19},
    {tet_Simple_Test,19},


/**********B03 add************/
    {tet_saEvtInitializeCases_15,1},
#if 0 
    {tet_saEvtInitializeCases,1,16}, /* Manual test for SA_AIS_ERR_UNAVAILABLE */
#endif
    {tet_saEvtLimitGetCases_01,20},
    {tet_saEvtLimitGetCases_02,20},
    {tet_saEvtLimitGetCases_03,20},
    {tet_saEvtLimitGetCases_04,20},
    {tet_saEvtLimitGetCases_05,20},
    {tet_saEvtLimitGetCases_06,20},
    {tet_saEvtLimitGetCases_07,20},
    {tet_saEvtLimitGetCases_08,20},
    {tet_saEvtLimitGetCases_09,20},
    {tet_saEvtEventAttributesGetCases_15,12},/*saEvtEventPatternFree cases by EventAlloc*/
    {tet_saEvtEventAttributesGetCases_16,12},
    {tet_saEvtEventAttributesGetCases_17,12},
    
    {tet_PatternFree_ReceivedEvent,18},
	
    {NULL,0}
  };

void tet_run_edsv_app()
{
  gl_defs();
}

#endif

void tet_edsv_startup()
{
/* Using the default mode for startup */
     ncs_agents_startup();    

#ifdef TET_SMOKE_TEST
  edsv_smoke_test();
  return;
#endif
   
#ifdef TET_EDSV
  tet_run_edsv_app();
#endif
}

void tet_edsv_end()
{
  tet_infoline(" Ending the agony.. ");
}
