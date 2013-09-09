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


void saNtfAlarmNotificationFilterAllocate_01(void)
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

    rc = saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        myNotificationFilterAllocationParams.numEventTypes,
        myNotificationFilterAllocationParams.numNotificationObjects,
        myNotificationFilterAllocationParams.numNotifyingObjects,
        myNotificationFilterAllocationParams.numNotificationClassIds,
        myNotificationFilterAllocationParams.numProbableCauses,
        myNotificationFilterAllocationParams.numPerceivedSeverities,
        myNotificationFilterAllocationParams.numTrends);
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

    free(myNotificationParams.additionalText); /* allocated in fillInDefaultValues */
    safassert(saNtfNotificationFilterFree(
                  myNotificationFilterHandles.alarmFilterHandle), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_OK);

}

void saNtfAlarmNotificationFilterAllocate_02(void)
{
    SaNtfAlarmNotificationFilterT myAlarmFilter;

    rc = saNtfAlarmNotificationFilterAllocate(
        0,
        &myAlarmFilter,
        0,
        0,
        0,
        1,
        0,
        1,
        0);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saNtfAlarmNotificationFilterAllocate_03(void)
{
    SaNtfHandleT ntfHandle;
    SaNtfAlarmNotificationFilterT myAlarmFilter;

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    rc = saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        0,
        0,
        0,
        1,
        0,
        1,
        0);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saNtfAlarmNotificationFilterAllocate_04(void)
{
    SaNtfHandleT ntfHandle;

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

    rc = saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        NULL,
        0,
        0,
        0,
        1,
        0,
        1,
        0);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

__attribute__ ((constructor)) static void
saNtfAlarmNotificationFilterAllocate_constructor(void)
{
    test_suite_add(7, "Consumer operations - filter allocate");
    test_case_add(7,saNtfAlarmNotificationFilterAllocate_01, "saNtfAlarmNotificationFilterAllocate - SA_AIS_OK");
    test_case_add(7,saNtfAlarmNotificationFilterAllocate_02, "saNtfAlarmNotificationFilterAllocate - handle null SA_AIS_ERR_BAD_HANDLE");
    test_case_add(7,saNtfAlarmNotificationFilterAllocate_03, "saNtfAlarmNotificationFilterAllocate - handle returned SA_AIS_ERR_BAD_HANDLE");
    test_case_add(7,saNtfAlarmNotificationFilterAllocate_04, "saNtfAlarmNotificationFilterAllocate - SA_AIS_ERR_INVALID_PARAM");
}


