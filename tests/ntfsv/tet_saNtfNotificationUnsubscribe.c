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

void saNtfNotificationUnsubscribe_01(void)
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

    safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles, 4), SA_AIS_OK);
    rc = saNtfNotificationUnsubscribe(4);
    safassert(saNtfNotificationFilterFree(
                  myNotificationFilterHandles.alarmFilterHandle), SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    free(myNotificationParams.additionalText); /* allocated in fillInDefaultValues */
    test_validate(rc, SA_AIS_OK);
}

void saNtfNotificationUnsubscribe_02(void)
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

    safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles, 4), SA_AIS_OK);
    rc = saNtfNotificationUnsubscribe(2);
    safassert(saNtfNotificationUnsubscribe(4), SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(
                  myNotificationFilterHandles.alarmFilterHandle), SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    free(myNotificationParams.additionalText); /* allocated in fillInDefaultValues */
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
}

void saNtfNotificationUnsubscribe_03(void)
{
    SaNtfHandleT ntfHandle;

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    rc = saNtfNotificationUnsubscribe(44);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
}


__attribute__ ((constructor)) static void saNtfNotificationUnsubscribe_constructor(void)
{
    test_suite_add(11, "Consumer operations - unsubscribe");
    test_case_add(11, saNtfNotificationUnsubscribe_01, "saNtfNotificationUnsubscribe first simple SA_AIS_OK");
    test_case_add(11, saNtfNotificationUnsubscribe_02, "saNtfNotificationUnsubscribe wrong id SA_AIS_ERR_NOT_EXIST");
    test_case_add(11, saNtfNotificationUnsubscribe_03, "saNtfNotificationUnsubscribe no subscription exist SA_AIS_ERR_NOT_EXIST");
}


