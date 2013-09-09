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
#include "tet_ntf.h"
#include "tet_ntf_common.h"

void saNtfNotificationSubscribe_01(void)
{
    SaNtfHandleT ntfHandle;
    SaNtfAlarmNotificationFilterT myAlarmFilter;
    SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles;

    saNotificationAllocationParamsT        myNotificationAllocationParams;
    saNotificationFilterAllocationParamsT  myNotificationFilterAllocationParams;
    saNotificationParamsT                  myNotificationParams;

    fillInDefaultValues(&myNotificationAllocationParams,
                        &myNotificationFilterAllocationParams,
                        &myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

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

    rc = saNtfNotificationSubscribe(&myNotificationFilterHandles, 4);
    safassert(saNtfNotificationUnsubscribe(4), SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(
                  myNotificationFilterHandles.alarmFilterHandle), SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    free(myNotificationParams.additionalText); /* allocated in fillInDefaultValues */
    test_validate(rc, SA_AIS_OK);
}

/* Test NULL handle parameter */
void saNtfNotificationSubscribe_02(void)
{
    rc = saNtfNotificationSubscribe(NULL, 4);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

/* Test all filter handles set to NULL */
void saNtfNotificationSubscribe_03(void)
{
    SaNtfHandleT ntfHandle;
    SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles;

    saNotificationAllocationParamsT        myNotificationAllocationParams;
    saNotificationFilterAllocationParamsT  myNotificationFilterAllocationParams;
    saNotificationParamsT                  myNotificationParams;

    fillInDefaultValues(&myNotificationAllocationParams,
                        &myNotificationFilterAllocationParams,
                        &myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

    /* Initialize filter handles */
    myNotificationFilterHandles.alarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    myNotificationFilterHandles.attributeChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    myNotificationFilterHandles.objectCreateDeleteFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    myNotificationFilterHandles.securityAlarmFilterHandle = SA_NTF_FILTER_HANDLE_NULL;
    myNotificationFilterHandles.stateChangeFilterHandle = SA_NTF_FILTER_HANDLE_NULL;

    rc = saNtfNotificationSubscribe(&myNotificationFilterHandles, 4);
    safassert(saNtfNotificationUnsubscribe(4), SA_AIS_ERR_NOT_EXIST);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    free(myNotificationParams.additionalText); /* allocated in fillInDefaultValues */
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

/* Test already existing subscriptionId */
void saNtfNotificationSubscribe_04(void)
{
    SaNtfHandleT ntfHandle;
    SaNtfAlarmNotificationFilterT myAlarmFilter;
    SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles;
    SaNtfSubscriptionIdT subscId = 0;
    saNotificationAllocationParamsT        myNotificationAllocationParams;
    saNotificationFilterAllocationParamsT  myNotificationFilterAllocationParams;
    saNotificationParamsT                  myNotificationParams;

    fillInDefaultValues(&myNotificationAllocationParams,
                        &myNotificationFilterAllocationParams,
                        &myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

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

    safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles, subscId), SA_AIS_OK);
    rc = saNtfNotificationSubscribe(&myNotificationFilterHandles, subscId);
    /* No need to saNtfNotificationUnsubscribe since the subscribe failed */

    safassert(saNtfNotificationUnsubscribe(subscId), SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(
                  myNotificationFilterHandles.alarmFilterHandle), SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    free(myNotificationParams.additionalText); /* allocated in fillInDefaultValues */
    test_validate(rc, SA_AIS_ERR_EXIST);
}

__attribute__ ((constructor)) static void saNtfNotificationSubscribe_constructor(void)
{
    test_suite_add(10, "Consumer operations - subscribe");
    test_case_add(10, saNtfNotificationSubscribe_01, "saNtfNotificationSubscribe first simple SA_AIS_OK");
    test_case_add(10, saNtfNotificationSubscribe_02, "saNtfNotificationSubscribe null handle SA_AIS_ERR_INVALID_PARAM");
    test_case_add(10, saNtfNotificationSubscribe_03, "saNtfNotificationSubscribe All filter handles null  SA_AIS_ERR_INVALID_PARAM");
    test_case_add(10, saNtfNotificationSubscribe_04, "saNtfNotificationSubscribe subscriptionId exist SA_AIS_ERR_EXIST");
}


