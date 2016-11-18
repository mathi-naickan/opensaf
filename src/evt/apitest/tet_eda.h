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

#ifndef EVT_APITEST_TET_EDA_H_
#define EVT_APITEST_TET_EDA_H_

/*******Global Declarations*******/

#include <saEvt.h>
#include <ncs_lib.h>

#define gl_red 0

#define WHILE_TRY_AGAIN(rc) while((rc==SA_AIS_ERR_TRY_AGAIN)\
                                  ||(rc==SA_AIS_ERR_TIMEOUT))

#define RETRY_SLEEP sleep(3)
#define MAX_NUM_TRY_AGAINS 12
#define DISPATCH_SLEEP() sleep_for_dispatch()


int gl_act; 
int gl_tNode1, gl_tNode2, gl_tNode3, gl_node_id, gl_jCount,
  gl_allocatedNumber, gl_patternLength, gl_tCount,gl_tCase,gl_iteration,
  gl_listNumber,gl_error,subCount,gl_err,gl_cbk,iCmpCount,tempDataSize,
  gl_hide;
char *gl_saf_msg,*tempData;
char gl_eventData[20];
NCSCONTEXT gl_t_handle;
SaEvtHandleT gl_evtHandle, gl_threadEvtHandle;
SaVersionT gl_version;
SaAisErrorT gl_rc;
SaSelectionObjectT gl_selObject;
SaDispatchFlagsT gl_dispatchFlags;
SaNameT gl_channelName,gl_publisherName;
SaEvtChannelOpenFlagsT gl_channelOpenFlags;
SaTimeT gl_timeout, gl_retentionTime, gl_publishTime;
SaEvtChannelHandleT gl_channelHandle;
SaInvocationT gl_invocation;
SaEvtEventHandleT gl_eventHandle, gl_eventDeliverHandle;
SaEvtEventIdT gl_evtId;
SaEvtEventPatternArrayT gl_patternArray;
SaEvtEventPriorityT gl_priority;
SaSizeT gl_eventDataSize;
SaEvtEventFilterArrayT gl_filterArray;
SaEvtSubscriptionIdT gl_subscriptionId, gl_dupSubscriptionId;
SaEvtEventPatternT gl_pattern[2];
SaEvtEventFilterT gl_filter[1];
SaEvtCallbacksT gl_evtCallbacks;
void EvtOpenCallback(SaInvocationT invocationCallback, 
                     SaEvtChannelHandleT asyncChannelHandle, SaAisErrorT error);
void EvtDeliverCallback(SaEvtSubscriptionIdT subscriptionId, 
                        SaEvtEventHandleT deliverEventHandle, 
                        SaSizeT eventDataSize);

void b03EvtDeliverCallback(SaEvtSubscriptionIdT subscriptionId, 
                        SaEvtEventHandleT deliverEventHandle, 
                        SaSizeT eventDataSize);
struct gl_list
{
  int data;
  char *gl_eventData;
};

NCSCONTEXT eda_thread_handle,subscription_handle;
#define TET_EDSV_NODE1 3
/*******String Constants*******/
const char *gl_saf_error[32];

typedef enum 
  {
    EDSV_TEST=1,
    EDSV_API_TEST,
    EDSV_FUNC_TEST,
    EDSV_CLM_TEST,
  }EDSV_TEST_LIST;

typedef struct gl_variables
{
  int tCase;
  int iteration;
  int listNumber;
  SaNameT channelName;
  SaEvtEventPatternArrayT patternArray;
  SaNameT publisherName;
  SaSizeT eventDataSize;
  SaEvtEventFilterArrayT filterArray;
  SaEvtSubscriptionIdT subscriptionId;
}gl_edsv_defs;

void tet_saEvtInitialize(SaEvtHandleT *);
void tet_saEvtSelectionObjectGet(SaEvtHandleT *,SaSelectionObjectT *);
void tet_saEvtDispatch(SaEvtHandleT *);
void tet_saEvtFinalize(SaEvtHandleT *);

void tet_saEvtChannelOpen(SaEvtHandleT *,SaEvtChannelHandleT *);
void tet_saEvtChannelOpenAsync(SaEvtHandleT *);
void tet_saEvtChannelClose(SaEvtChannelHandleT *);
void tet_saEvtChannelUnlink(SaEvtHandleT *);

void tet_saEvtEventAllocate(SaEvtChannelHandleT *,SaEvtEventHandleT *);
void tet_saEvtEventFree(SaEvtEventHandleT *);
void tet_saEvtEventAttributesSet(SaEvtEventHandleT *);
void tet_saEvtEventAttributesGet(SaEvtEventHandleT *);
void tet_saEvtEventDataGet(SaEvtEventHandleT *);
void tet_saEvtEventPublish(SaEvtEventHandleT *, SaEvtEventIdT *);
void tet_saEvtEventSubscribe(SaEvtChannelHandleT *,SaEvtSubscriptionIdT *);
void tet_saEvtEventUnsubscribe(SaEvtChannelHandleT *);
void tet_saEvtEventRetentionTimeClear(SaEvtChannelHandleT *,SaEvtEventIdT *);

void result(char *,SaAisErrorT);
void resultSuccess(char *,SaAisErrorT);
uint32_t tet_create_task(NCS_OS_CB);

void tet_run_edsv_app(void);
void gl_defs(void);

void select_call(void);      

void var_initialize(void);
void eda_selection_thread(void);
void thread_creation(SaEvtHandleT *ptrEvtHandle);
/** Multi Node Function Declarations**/
void tet_2_node_01(void);
void tet_2_node_multiple_publish_01(void);
void tet_2_node_priority_01(void);
void tet_2_node_retentiontime_01(void);
void tet_2_node_multiple_sub_pub_01(void);
void tet_2_node_pattern_match_01(void);
void tet_2_node_null_pattern_array_01(void);
void tet_2_node_attributes_set_01(void);
#if 0
void tet_3_node(void);
void tet_3_node_channel_unlink(void);
void tet_3_node_subscriber_publisher(void);
void tet_5_node(void);
void tet_2_node_sub_pub(void);
#endif
void tet_2_node_2_com_01(void);
void tet_2_node_async_01(void);
void tet_2_node_no_retention_time_01(void);
void tet_2_node_event_order_01(void);
void tet_2_node_open_async_01(void);
void tet_2_node_sync_async_01(void);
void tet_2_node_async_sub_01(void);
void tet_2_node_sub_unsub_01(void);
void tet_2_node_pub_01(void);
void tet_2_node_pub_pf_01(void);
void tet_2_node_pub_sf_01(void);
void tet_2_node_pub_ef_01(void);
void tet_2_node_pub_apf_01(void);
void tet_2_node_pub_mf_01(void);
void tet_2_node_pub_df_01(void);
void tet_2_node_pub_less_filters_01(void);
void tet_2_node_pub_less_patterns_01(void);
void tet_2_node_pub_less_patterns_apf_01(void);
void tet_2_node_pub_less_patterns_filter0_01(void);
void tet_2_node_pub_one_one_map_01(void);
void tet_2_node_pub_pf_filter_01(void);
void tet_2_node_pub_pf_nm_01(void);
void tet_2_node_pub_sf_nm_01(void);
void tet_2_node_pub_ef_nm_01(void);

void tet_2_node_02(void);
void tet_2_node_multiple_publish_02(void);
void tet_2_node_priority_02(void);
void tet_2_node_retentiontime_02(void);
void tet_2_node_multiple_sub_pub_02(void);
void tet_2_node_pattern_match_02(void);
void tet_2_node_null_pattern_array_02(void);
void tet_2_node_attributes_set_02(void);
#if 0
void tet_3_node(void);
void tet_3_node_channel_unlink(void);
void tet_3_node_subscriber_publisher(void);
void tet_5_node(void);
void tet_2_node_sub_pub(void);
#endif
void tet_2_node_2_com_02(void);
void tet_2_node_async_02(void);
void tet_2_node_no_retention_time_02(void);
void tet_2_node_event_order_02(void);
void tet_2_node_open_async_02(void);
void tet_2_node_sync_async_02(void);
void tet_2_node_async_sub_02(void);
void tet_2_node_sub_unsub_02(void);
void tet_2_node_pub_02(void);
void tet_2_node_pub_pf_02(void);
void tet_2_node_pub_sf_02(void);
void tet_2_node_pub_ef_02(void);
void tet_2_node_pub_apf_02(void);
void tet_2_node_pub_mf_02(void);
void tet_2_node_pub_df_02(void);
void tet_2_node_pub_less_filters_02(void);
void tet_2_node_pub_less_patterns_02(void);
void tet_2_node_pub_less_patterns_apf_02(void);
void tet_2_node_pub_less_patterns_filter0_02(void);
void tet_2_node_pub_one_one_map_02(void);
void tet_2_node_pub_pf_filter_02(void);
void tet_2_node_pub_pf_nm_02(void);
void tet_2_node_pub_sf_nm_02(void);
void tet_2_node_pub_ef_nm_02(void);
/*Functionality Testcase Declarations*/
void tet_ChannelOpen_SingleEvtHandle(void);
void tet_ChannelOpen_SingleEvtHandle_SamePatterns(void);
void tet_ChannelOpen_SingleEvtHandle_DifferentPatterns(void);
void tet_ChannelOpen_MultipleEvtHandle(void);
void tet_ChannelOpen_MultipleEvtHandle_SamePatterns(void);
void tet_ChannelOpen_MultipleEvtHandle_DifferentPatterns(void);
void tet_ChannelOpen_MultipleOpen(void);
void tet_ChannelClose_Simple(void);
void tet_ChannelClose_Subscriber(void);
void tet_ChannelUnlink_Simple(void);
void tet_ChannelUnlink_OpenInstance(void);
void tet_EventFree_Simple(void);
void tet_AttributesSet_DeliverCallback(void);
void tet_AttributesGet_ReceivedEvent(void);
void tet_EventPublish_NullPatternArray(void);
void tet_EventSubscribe_PrefixFilter(void);
void tet_EventSubscribe_SuffixFilter(void);
void tet_EventSubscribe_ExactFilter(void);
void tet_EventSubscribe_AllPassFilter(void);
void tet_EventSubscribe_LessFilters(void);
void tet_EventSubscribe_LessPatterns(void);
void tet_EventSubscribe_LessPatterns_FilterSize(void);
void tet_EventSubscribe_LessFilters_DifferentOrder(void);
void tet_EventSubscribe_PatternMatch_DifferentSubscriptions(void);
void tet_ChannelOpenAsync_Simple(void);
void tet_EventSubscribe_multipleFilters(void);
void tet_EventSubscribe_PrefixFilter_NoMatch(void);
void tet_EventSubscribe_SuffixFilter_NoMatch(void);
void tet_EventSubscribe_ExactFilter_NoMatch(void);
void tet_EventSubscribe_DiffFilterTypes(void);
void tet_EventSubscribe_PrefixFilter_FilterSize(void);
void tet_EventPublish_NullPatternArray_AttributesSet(void);
void tet_Complex_Subscriber_Publisher(void);
void tet_Same_Subscriber_Publisher_DiffChannel(void);
void tet_Sync_Async_Subscriber_Publisher(void);
void tet_Event_Subscriber_Unsubscriber(void);
void tet_EventDeliverCallback_AttributesSet_RetentionTimeClear(void);
void tet_MultipleHandle_Subscribers_Publisher(void);
void tet_EventPublish_Priority(void);
void tet_EventPublish_EventOrder(void);
void tet_EventPublish_EventFree(void);
void tet_EventFree_DeliverCallback(void);
void tet_EventRetentionTimeClear_EventFree(void);
void tet_EventPublish_ChannelUnlink(void);
void tet_EventUnsubscribe_EventPublish(void);
void tet_EventAttributesGet_LessPatternsNumber(void);
void tet_Simple_Test(void);
/** Miscellenious Functions **/
void    tet_ChannelOpen_before_Initialize(void);
void    tet_ChannelOpen_after_Finalize(void);
void    tet_ChannelOpenAsync_before_Initialize(void);
void    tet_ChannelOpenAsync_after_Finalize(void);
void    tet_Subscribe_before_ChannelOpen(void);
void    tet_Subscribe_after_ChannelClose(void);
void    tet_Subscribe_after_ChannelUnlink(void);
void    tet_Subscribe_after_Finalize(void);
void    tet_Allocate_before_ChannelOpen(void);
void    tet_Allocate_after_ChannelClose(void);
void    tet_Allocate_after_ChannelUnlink(void);
void    tet_Allocate_after_Finalize(void);
void    tet_Allocate_check_DefaultValues(void);
void    tet_AttributesSet_before_Allocate(void);
void    tet_AttributesSet_after_EventFree(void);
void    tet_AttributesSet_after_ChannelClose(void);
void    tet_AttributesSet_after_ChannelUnlink(void);
void    tet_AttributesSet_after_Finalize(void);
void    tet_Publish_before_Allocate(void);
void    tet_Publish_after_EventFree(void);
void    tet_Publish_after_ChannelClose(void);
void    tet_Publish_after_ChannelUnlink(void);
void    tet_Publish_after_Finalize(void);
void    tet_SelectionObject_before_Initialize(void);
void    tet_SelectionObject_after_Finalize(void);
void    tet_Dispatch_before_Initialize(void);
void    tet_Dispatch_after_Finalize(void);
void    tet_AttributesGet_before_Allocate(void);
void    tet_AttributesGet_after_EventFree(void);
void    tet_AttributesGet_after_ChannelClose(void);
void    tet_AttributesGet_after_ChannelUnlink(void);
void    tet_AttributesGet_after_Finalize(void);
void    tet_Unsubscribe_before_ChannelOpen(void);
void    tet_Unsubscribe_after_ChannelClose(void);
void    tet_Unsubscribe_after_ChannelUnlink(void);
void    tet_Unsubscribe_after_Finalize(void);
void    tet_RetentionTimeClear_before_ChannelOpen(void);
void    tet_RetentionTimeClear_after_ChannelClose(void);
void    tet_RetentionTimeClear_after_ChannelUnlink(void);
void    tet_RetentionTimeClear_after_Finalize(void);
void    tet_EventFree_before_Allocate(void);
void    tet_EventFree_after_ChannelClose(void);
void    tet_EventFree_after_ChannelUnlink(void);
void    tet_EventFree_after_Finalize(void);
void    tet_ChannelClose_before_ChannelOpen(void);
void    tet_ChannelClose_after_ChannelClose(void);
void    tet_ChannelClose_after_ChannelUnlink(void);
void    tet_ChannelClose_after_Finalize(void);
void    tet_ChannelUnlink_before_ChannelOpen(void);
void    tet_ChannelUnlink_after_Finalize(void); 
void    tet_Finalize_after_Finalize(void);
void    tet_ChannelOpen_after_ChannelUnlink(void);
void    tet_ChannelOpen_after_ChannelClose(void);
void    tet_ChannelOpenAsync_after_ChannelClose(void);
void    tet_ChannelOpenAsync_after_ChannelUnlink(void);
void    tet_EventSubscribe_after_ChannelUnlink(void);
#if 0
void    tet_AttributesGet_NoMemory(void);
void    tet_DataGet_struct(void);
#endif
void    tet_MultiChannelOpen(void);
void    tet_DataGet_without_Memory(void);
#if 0
void    tet_EventFree_ChannelClose(void);
void    tet_ChannelOpen_ChannelUnlink(void);
#endif
struct gl_edsv
{
};
void sleep_for_dispatch(void);
void tet_saEvtInitializeCases(int iOption);
void tet_Initialize(void);
void tet_saEvtSelectionObjectGetCases(int iOption);
void tet_SelectionObjectGet(void);
void Dispatch_Blocking_Thread(void);
void tet_saEvtDispatchCases(int iOption);
void tet_Dispatch(void);
void tet_saEvtFinalizeCases(int iOption);
void tet_Finalize(void);
void tet_saEvtChannelOpenCases(int iOption);
void tet_ChannelOpen(void);
void Initialize_Thread(void);
void SelectionObjectGet_Thread(void);
void Dispatch_Thread(void);
void ChannelOpen_Thread(void);
void Allocate_Thread(void);
void AttributesSet_Thread(void);
void Publish_Thread(void);
void Free_Thread(void);
void ChannelClose_Thread(void);
void ChannelUnlink_Thread(void);
void Finalize_Thread(void);
void tet_Publish_Thread(void);

void tet_saEvtChannelOpenAsyncCases(int iOption);
void tet_ChannelOpenAsync(void);
void ChannelOpenAsync_Thread(void);
void tet_ChannelOpenAsync_Thread(void);
void tet_saEvtChannelCloseCases(int iOption);
void tet_ChannelClose(void);
void tet_saEvtChannelUnlinkCases(int iOption);
void tet_ChannelUnlink(void);
void tet_saEvtEventAllocateCases(int iOption);
void tet_Allocate(void);
void tet_saEvtEventFreeCases(int iOption);
void tet_Free(void);
void tet_saEvtEventAttributesGetCases(int iOption);
void tet_AttributesGet(void);
void tet_saEvtEventAttributesSetCases(int iOption);
void tet_AttributesSet(void);
void tet_saEvtEventDataGetCases(int iOption);
void tet_DataGet(void);
void DataGet_Thread(void);
void tet_DataGet_Thread(void);
void tet_saEvtEventPublishCases(int iOption);
void tet_Publish(void);
void tet_saEvtEventSubscribeCases(int iOption);
void tet_Subscribe(void);
void Subscribe_Thread(void);
void Unsubscribe_Thread(void);
void tet_Subscribe_Thread(void);
void tet_saEvtEventUnsubscribeCases(int iOption);
void tet_Unsubscribe(void);
void tet_saEvtEventRetentionTimeClearCases(int iOption);
void tet_RetentionTimeClear(void);
void RetentionTimeClear_Thread(void);
void tet_RetentionTimeClear_Thread(void);
void tet_stressTest(void);
void gl_defs(void);
void tet_run_edsv_app(void);
void tet_ChannelOpen_SingleEvtHandle(void);  
void tet_run_edsv_dist_cases(void);
void tet_ChannelOpen_SingleEvtHandle_SamePatterns(void);
void tet_ChannelOpen_SingleEvtHandle_DifferentPatterns(void);
void tet_ChannelOpen_MultipleEvtHandle(void);
void tet_ChannelOpen_MultipleEvtHandle_SamePatterns();
void tet_ChannelOpen_MultipleEvtHandle_DifferentPatterns();
void tet_ChannelOpen_MultipleOpen(void);
void tet_ChannelClose_Simple();
void tet_ChannelClose_Subscriber(void);
void tet_ChannelUnlink_Simple(void);
void tet_ChannelUnlink_OpenInstance(void);
void tet_EventFree_Simple(void);
void tet_AttributesSet_DeliverCallback(void);
void tet_AttributesGet_ReceivedEvent(void);
void tet_EventPublish_NullPatternArray(void);
void tet_EventSubscribe_PrefixFilter(void);
void sleep_for_dispatch(void);
void tet_EventSubscribe_SuffixFilter(void);
void tet_EventSubscribe_ExactFilter(void);
void tet_EventSubscribe_AllPassFilter();
void tet_EventSubscribe_LessFilters();
void tet_EventSubscribe_LessPatterns();
void tet_EventSubscribe_LessPatterns_FilterSize(void);
void tet_EventSubscribe_LessFilters_DifferentOrder();
void tet_EventSubscribe_PatternMatch_DifferentSubscriptions();
void tet_ChannelOpenAsync_Simple();
void tet_EventSubscribe_multipleFilters();
void tet_EventSubscribe_PrefixFilter_NoMatch();
void tet_EventSubscribe_SuffixFilter_NoMatch();
void tet_EventSubscribe_ExactFilter_NoMatch(void);
void tet_EventSubscribe_DiffFilterTypes(void);
void tet_EventSubscribe_PrefixFilter_FilterSize(void);
void tet_EventPublish_NullPatternArray_AttributesSet(void);
void tet_Complex_Subscriber_Publisher(void);
void tet_Same_Subscriber_Publisher_DiffChannel(void);
void tet_Event_Subscriber_Unsubscriber(void);
void tet_EventDeliverCallback_AttributesSet_RetentionTimeClear(void);
void tet_MultipleHandle_Subscribers_Publisher(void);
void tet_EventPublish_Priority(void);
void tet_EventPublish_EventOrder(void);
void tet_EventPublish_EventFree(void);
void tet_EventFree_DeliverCallback(void);
void tet_EventRetentionTimeClear_EventFree(void);
void tet_EventPublish_ChannelUnlink(void);
void tet_EventUnsubscribe_EventPublish(void);
void tet_EventAttributesGet_LessPatternsNumber(void);
void tet_Simple_Test(void);
void tet_ChannelOpen_before_Initialize(void);
void tet_ChannelOpen_after_Finalize(void);
void tet_ChannelOpenAsync_before_Initialize(void);
void tet_ChannelOpenAsync_after_Finalize(void);
void tet_Subscribe_before_ChannelOpen(void);
void tet_Subscribe_after_ChannelClose(void);
void tet_Subscribe_after_ChannelUnlink(void);
void tet_Subscribe_after_Finalize(void);
void tet_Allocate_before_ChannelOpen(void);
void tet_Allocate_after_ChannelClose(void);
void tet_Allocate_after_ChannelUnlink(void);
void tet_Allocate_after_Finalize(void);
void tet_Allocate_check_DefaultValues(void);
void tet_AttributesSet_before_Allocate(void);
void tet_AttributesSet_after_EventFree(void);
void tet_AttributesSet_after_ChannelClose(void);
void tet_AttributesSet_after_ChannelUnlink(void);
void tet_AttributesSet_after_Finalize(void);
void tet_Publish_before_Allocate(void);
void tet_Publish_after_EventFree(void);
void tet_Publish_after_ChannelClose(void);
void tet_Publish_after_Finalize(void);
void tet_SelectionObject_before_Initialize(void);
void tet_SelectionObject_after_Finalize(void);
void tet_Dispatch_before_Initialize(void);
void tet_Dispatch_after_Finalize(void);
void tet_AttributesGet_before_Allocate(void);
void tet_AttributesGet_after_EventFree(void);
void tet_AttributesGet_after_ChannelClose(void);
void tet_AttributesGet_after_ChannelUnlink(void);
void tet_Unsubscribe_before_ChannelOpen(void);
void tet_Unsubscribe_after_ChannelClose(void);
void tet_Unsubscribe_after_ChannelUnlink(void);
void tet_Unsubscribe_after_Finalize(void);
void tet_RetentionTimeClear_before_ChannelOpen(void);
void tet_RetentionTimeClear_after_ChannelClose(void);
void tet_RetentionTimeClear_after_ChannelUnlink(void);
void tet_RetentionTimeClear_after_Finalize(void);
void tet_EventFree_before_Allocate(void);
void tet_EventFree_after_ChannelClose(void);
void tet_EventFree_after_ChannelUnlink(void);
void tet_EventFree_after_Finalize(void);
void tet_ChannelClose_before_ChannelOpen(void);
void tet_ChannelClose_after_ChannelClose(void);
void tet_ChannelClose_after_ChannelUnlink(void);
void tet_ChannelClose_after_Finalize(void);
void tet_ChannelUnlink_before_ChannelOpen(void);
void tet_ChannelUnlink_after_Finalize(void);
void tet_Finalize_after_Finalize(void);
void tet_ChannelOpen_after_ChannelUnlink(void);
void tet_ChannelOpen_after_ChannelClose(void);
void tet_ChannelOpenAsync_after_ChannelClose(void);
void tet_ChannelOpenAsync_after_ChannelUnlink(void);
void tet_EventSubscribe_after_ChannelUnlink(void);
void tet_MultiChannelOpen(void);
void tet_DataGet_without_Memory(void);
void eda_selection_thread (void);
void var_initialize(void);
void AttributesGet_Thread(void);
void tet_PatternFree_ReceivedEvent(void);

#endif  // EVT_APITEST_TET_EDA_H_
