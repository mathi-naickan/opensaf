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
 * Author(s): Ericsson AB
 *
 */
#include <utest.h>
#include <util.h>
#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#include "tet_ntf.h"
#include "tet_ntf_common.h"

static SaNtfIdentifierT cb_notId[8];
static SaNtfSubscriptionIdT cb_subId[8];
static int cb_index = 0;

static SaNtfIdentifierT cb2_notId[8];
static SaNtfSubscriptionIdT cb2_subId[8];
static int cb2_index = 0;
extern int verbose;
static saNotificationParamsT myNotificationParams;
static saNotificationAllocationParamsT myNotificationAllocationParams;

static void resetCounters()
{
    int i;
    for(i=0; i<8;i++)
    {
        cb_notId[i]=0;
        cb_subId[0]=0;
    }
    cb_index=0;
}

static void resetCounters2()
{
    int i;
    for(i=0; i<8;i++)
    {
        cb2_notId[i]=0;
        cb2_subId[0]=0;
    }
    cb2_index=0;
}

static void saNtfNotificationCallbackT(
    SaNtfSubscriptionIdT subscriptionId,
    const SaNtfNotificationsT *notification)
{
    SaNtfIdentifierT notId;

    notId = *notification->
        notification.alarmNotification.notificationHeader.notificationId;

    if(verbose)
    {
        tet_printf("\nsaNtfNotificationCallbackT() subscriptionId: %u, "
                   "notificationId %llu",
                   subscriptionId, notId);
    }
    cb_subId[cb_index] = subscriptionId;
    cb_notId[cb_index++] = notId;

    if(verbose)
    {
        newNotification(subscriptionId, notification);
    }
    assert(saNtfNotificationFree(notification->notification.
                                 alarmNotification.
                                 notificationHandle) == SA_AIS_OK);
}

static void saNtfNotificationCallbackT2(
    SaNtfSubscriptionIdT subscriptionId,
    const SaNtfNotificationsT *notification)
{
    SaNtfIdentifierT notId;

    notId = *notification->
        notification.alarmNotification.notificationHeader.notificationId;
/*  tet_printf("\nsaNtfNotificationCallbackT() subscriptionId: %u, "*/
/*             "notificationId %llu cb2_index: %d",                 */
/*             subscriptionId, notId, cb2_index);                   */
    cb2_subId[cb2_index] = subscriptionId;
    cb2_notId[cb2_index++] = notId;
    assert(saNtfNotificationFree(notification->notification.
                                 alarmNotification.
                                 notificationHandle) == SA_AIS_OK);
}

static SaNtfCallbacksT ntfCbTest = {
    saNtfNotificationCallbackT,
    NULL
};

static SaNtfCallbacksT ntfCbTest2 = {
    saNtfNotificationCallbackT2,
    NULL
};


static void *dispatchTread(void * arg)
{
    /* TODO: stop this thread */
    safassert(saNtfDispatch(ntfHandle, SA_DISPATCH_BLOCKING), SA_AIS_OK);
    exit(0);
    return 0;
}


static SaNtfAlarmNotificationT myAlarmNotification;


void saNtfNotificationCallbackT_01(void)
{
    struct pollfd fds[1];
    int ret;
    saNotificationFilterAllocationParamsT  myNotificationFilterAllocationParams;
    AlarmNotificationParams                myAlarmParams;
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    SaNtfNotificationTypeFilterHandlesT    myNotificationFilterHandles;

    fillInDefaultValues(&myNotificationAllocationParams,
                        &myNotificationFilterAllocationParams,
                        &myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        myNotificationFilterAllocationParams.numEventTypes,
        myNotificationFilterAllocationParams.numNotificationObjects,
        myNotificationFilterAllocationParams.numNotifyingObjects,
        myNotificationFilterAllocationParams.numNotificationClassIds,
        myNotificationFilterAllocationParams.numProbableCauses,
        myNotificationFilterAllocationParams.numPerceivedSeverities,
        myNotificationFilterAllocationParams.numTrends), SA_AIS_OK);
    /* Set perceived severities */
    myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_WARNING;
    myAlarmFilter.perceivedSeverities[1] = SA_NTF_SEVERITY_CLEARED;

    /* Initialize filter handles */
    myNotificationFilterHandles.alarmFilterHandle =
        myAlarmFilter.notificationFilterHandle;
    myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    myNotificationFilterHandles.stateChangeFilterHandle = 0;

    safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
                                         myNotificationParams.subscriptionId[0]),
                                         SA_AIS_OK);

    myAlarmParams.numCorrelatedNotifications =
        myNotificationAllocationParams.numCorrelatedNotifications;
    myAlarmParams.lengthAdditionalText =
        myNotificationAllocationParams.lengthAdditionalText;
    myAlarmParams.numAdditionalInfo =
        myNotificationAllocationParams.numAdditionalInfo;
    myAlarmParams.numSpecificProblems =
        myNotificationAllocationParams.numSpecificProblems;
    myAlarmParams.numMonitoredAttributes =
        myNotificationAllocationParams.numMonitoredAttributes;
    myAlarmParams.numProposedRepairActions =
        myNotificationAllocationParams.numProposedRepairActions;
    myAlarmParams.variableDataSize =
        myNotificationAllocationParams.variableDataSize;

    safassert(saNtfAlarmNotificationAllocate(
        ntfHandle,
        &myAlarmNotification,
        myAlarmParams.numCorrelatedNotifications,
        myAlarmParams.lengthAdditionalText,
        myAlarmParams.numAdditionalInfo,
        myAlarmParams.numSpecificProblems,
        myAlarmParams.numMonitoredAttributes,
        myAlarmParams.numProposedRepairActions,
        myAlarmParams.variableDataSize), SA_AIS_OK);

    myNotificationParams.eventType = myNotificationParams.alarmEventType;
    fill_header_part(
        &myAlarmNotification.notificationHeader,
        (saNotificationParamsT *)&myNotificationParams,
        myAlarmParams.lengthAdditionalText);

    /* determine perceived severity */
    *(myAlarmNotification.perceivedSeverity) =
        myNotificationParams.perceivedSeverity;

    /* set probable cause*/
    *(myAlarmNotification.probableCause) =
        myNotificationParams.probableCause;

    safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
    fds[0].fd = (int) selectionObject;
    fds[0].events = POLLIN;
    ret = poll(fds, 1, 10000);
    assert(ret > 0);
    safassert(saNtfDispatch(ntfHandle, SA_DISPATCH_ONE) , SA_AIS_OK);

    rc =SA_AIS_ERR_FAILED_OPERATION;
    if(verbose)
    {
        tet_printf("%-30s cb_notId[0]: %llu, notid: %llu",
                   __FUNCTION__,
                   cb_notId[0],
                   *myAlarmNotification.notificationHeader.notificationId);
    }
    if(cb_notId[0] == *myAlarmNotification.notificationHeader.notificationId)
    {
        if(cb_subId[0] == myNotificationParams.subscriptionId[0])
        {
            rc = SA_AIS_OK;
        }
    }
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);
    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationUnsubscribe(myNotificationParams.subscriptionId[0]),
                                           SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);

    /* Allocated in fillInDefaultValues() */
    free(myNotificationParams.additionalText);
    test_validate(rc, SA_AIS_OK);
}


void saNtfNotificationCallbackT_02(void)
{
    struct pollfd fds[1];
    int ret,i;
    saNotificationFilterAllocationParamsT  myNotificationFilterAllocationParams;

    AlarmNotificationParams                myAlarmParams;
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    SaNtfNotificationTypeFilterHandlesT    myNotificationFilterHandles;
    SaNtfIdentifierT                       myNotId[3];

    resetCounters();
    fillInDefaultValues(&myNotificationAllocationParams,
                        &myNotificationFilterAllocationParams,
                        &myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        myNotificationFilterAllocationParams.numEventTypes,
        myNotificationFilterAllocationParams.numNotificationObjects,
        myNotificationFilterAllocationParams.numNotifyingObjects,
        myNotificationFilterAllocationParams.numNotificationClassIds,
        myNotificationFilterAllocationParams.numProbableCauses,
        myNotificationFilterAllocationParams.numPerceivedSeverities,
        myNotificationFilterAllocationParams.numTrends), SA_AIS_OK);
    /* Set perceived severities */
    myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_WARNING;
    myAlarmFilter.perceivedSeverities[1] = SA_NTF_SEVERITY_CLEARED;

    /* Initialize filter handles */
    myNotificationFilterHandles.alarmFilterHandle =
        myAlarmFilter.notificationFilterHandle;
    myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    myNotificationFilterHandles.stateChangeFilterHandle = 0;

    safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
                                         myNotificationParams.subscriptionId[0]),
                                         SA_AIS_OK);

    myAlarmParams.numCorrelatedNotifications =
        myNotificationAllocationParams.numCorrelatedNotifications;
    myAlarmParams.lengthAdditionalText =
        myNotificationAllocationParams.lengthAdditionalText;
    myAlarmParams.numAdditionalInfo =
        myNotificationAllocationParams.numAdditionalInfo;
    myAlarmParams.numSpecificProblems =
        myNotificationAllocationParams.numSpecificProblems;
    myAlarmParams.numMonitoredAttributes =
        myNotificationAllocationParams.numMonitoredAttributes;
    myAlarmParams.numProposedRepairActions =
        myNotificationAllocationParams.numProposedRepairActions;
    myAlarmParams.variableDataSize =
        myNotificationAllocationParams.variableDataSize;

    safassert(saNtfAlarmNotificationAllocate(
        ntfHandle,
        &myAlarmNotification,
        myAlarmParams.numCorrelatedNotifications,
        myAlarmParams.lengthAdditionalText,
        myAlarmParams.numAdditionalInfo,
        myAlarmParams.numSpecificProblems,
        myAlarmParams.numMonitoredAttributes,
        myAlarmParams.numProposedRepairActions,
        myAlarmParams.variableDataSize), SA_AIS_OK);

    myNotificationParams.eventType = myNotificationParams.alarmEventType;
    fill_header_part(
        &myAlarmNotification.notificationHeader,
        (saNotificationParamsT *)&myNotificationParams,
        myAlarmParams.lengthAdditionalText);

    /* determine perceived severity */
    *(myAlarmNotification.perceivedSeverity) =
        myNotificationParams.perceivedSeverity;

    /* set probable cause*/
    *(myAlarmNotification.probableCause) =
        myNotificationParams.probableCause;

    safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
    myNotId[0] = *myAlarmNotification.notificationHeader.notificationId;
    safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
    myNotId[1] = *myAlarmNotification.notificationHeader.notificationId;
    safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
    myNotId[2] = *myAlarmNotification.notificationHeader.notificationId;
    sleep(1);
    fds[0].fd = (int) selectionObject;
    fds[0].events = POLLIN;
    ret = poll(fds, 1, 10000);
    assert(ret > 0);
    safassert(saNtfDispatch(ntfHandle, SA_DISPATCH_ALL) , SA_AIS_OK);

    rc =SA_AIS_OK;
    for(i = 0; i < 3; i++)
    {
        if(cb_notId[i] == myNotId[i]){
            if(cb_subId[i] == myNotificationParams.subscriptionId[0]){
                rc = SA_AIS_OK;
            }
            else {
                rc =SA_AIS_ERR_FAILED_OPERATION;
                break;
            }
        } else {
            rc =SA_AIS_ERR_FAILED_OPERATION;
            break;
        }
    }
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);
    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationUnsubscribe(myNotificationParams.subscriptionId[0]),
                                           SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);

    /* Allocated in fillInDefaultValues() */
    free(myNotificationParams.additionalText);

    test_validate(rc, SA_AIS_OK);
}

void saNtfNotificationCallbackT_03(void)
{
    int i;
    pthread_t tid;

    saNotificationFilterAllocationParamsT  myNotificationFilterAllocationParams;

    AlarmNotificationParams                myAlarmParams;
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    SaNtfNotificationTypeFilterHandlesT    myNotificationFilterHandles;
    SaNtfIdentifierT                       myNotId[3];

    resetCounters2();
    fillInDefaultValues(&myNotificationAllocationParams,
                        &myNotificationFilterAllocationParams,
                        &myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest2, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        myNotificationFilterAllocationParams.numEventTypes,
        myNotificationFilterAllocationParams.numNotificationObjects,
        myNotificationFilterAllocationParams.numNotifyingObjects,
        myNotificationFilterAllocationParams.numNotificationClassIds,
        myNotificationFilterAllocationParams.numProbableCauses,
        myNotificationFilterAllocationParams.numPerceivedSeverities,
        myNotificationFilterAllocationParams.numTrends), SA_AIS_OK);
    /* Set perceived severities */
    myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_WARNING;
    myAlarmFilter.perceivedSeverities[1] = SA_NTF_SEVERITY_CLEARED;

    /* Initialize filter handles */
    myNotificationFilterHandles.alarmFilterHandle =
        myAlarmFilter.notificationFilterHandle;
    myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    myNotificationFilterHandles.stateChangeFilterHandle = 0;

    safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
                                         myNotificationParams.subscriptionId[0]),
                                         SA_AIS_OK);

    myAlarmParams.numCorrelatedNotifications =
        myNotificationAllocationParams.numCorrelatedNotifications;
    myAlarmParams.lengthAdditionalText =
        myNotificationAllocationParams.lengthAdditionalText;
    myAlarmParams.numAdditionalInfo =
        myNotificationAllocationParams.numAdditionalInfo;
    myAlarmParams.numSpecificProblems =
        myNotificationAllocationParams.numSpecificProblems;
    myAlarmParams.numMonitoredAttributes =
        myNotificationAllocationParams.numMonitoredAttributes;
    myAlarmParams.numProposedRepairActions =
        myNotificationAllocationParams.numProposedRepairActions;
    myAlarmParams.variableDataSize =
        myNotificationAllocationParams.variableDataSize;

    safassert(saNtfAlarmNotificationAllocate(
        ntfHandle,
        &myAlarmNotification,
        myAlarmParams.numCorrelatedNotifications,
        myAlarmParams.lengthAdditionalText,
        myAlarmParams.numAdditionalInfo,
        myAlarmParams.numSpecificProblems,
        myAlarmParams.numMonitoredAttributes,
        myAlarmParams.numProposedRepairActions,
        myAlarmParams.variableDataSize), SA_AIS_OK);

    myNotificationParams.eventType = myNotificationParams.alarmEventType;
    fill_header_part(
        &myAlarmNotification.notificationHeader,
        (saNotificationParamsT *)&myNotificationParams,
        myAlarmParams.lengthAdditionalText);

    /* determine perceived severity */
    *(myAlarmNotification.perceivedSeverity) =
        myNotificationParams.perceivedSeverity;

    /* set probable cause*/
    *(myAlarmNotification.probableCause) =
        myNotificationParams.probableCause;

    safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
    myNotId[0] = *myAlarmNotification.notificationHeader.notificationId;
    safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
    myNotId[1] = *myAlarmNotification.notificationHeader.notificationId;
    safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
    myNotId[2] = *myAlarmNotification.notificationHeader.notificationId;

    safassert(pthread_create(&tid,NULL,dispatchTread, NULL), 0);
    safassert(pthread_detach(tid),0);
    /* TODO: fix sync */
    for(;cb2_index < 3;)
    {
        usleep(100000);
    }
    rc =SA_AIS_OK;
    for(i = 0; i < 3; i++)
    {
        if(cb2_notId[i] == myNotId[i]){
            if(cb2_subId[i] == myNotificationParams.subscriptionId[0]){
                rc = SA_AIS_OK;
            }
            else {
                rc =SA_AIS_ERR_FAILED_OPERATION;
                break;
            }
        } else {
            rc =SA_AIS_ERR_FAILED_OPERATION;
            break;
        }
    }
    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationUnsubscribe(myNotificationParams.subscriptionId[0]),
                                           SA_AIS_OK);
    assert(saNtfFinalize(ntfHandle)== SA_AIS_OK);
    /* Allocated in fillInDefaultValues() */
    free(myNotificationParams.additionalText);
    test_validate(rc, SA_AIS_OK);
}

__attribute__ ((constructor)) static void saNtfNotificationCallbackT_constructor(void)
{
    test_suite_add(14, "Consumer operations - callback");
    test_case_add(14, saNtfNotificationCallbackT_01, "saNtfNotificationCallbackT SA_DISPATCH_ONE");
    test_case_add(14, saNtfNotificationCallbackT_02, "saNtfNotificationCallbackT SA_DISPATCH_ALL");
/*    test_case_add(14, saNtfNotificationCallbackT_03, "saNtfNotificationCallbackT SA_DISPATCH_BLOCKING");
 */
}


