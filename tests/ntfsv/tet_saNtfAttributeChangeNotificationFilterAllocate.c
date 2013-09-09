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

void saNtfAttributeChangeNotificationFilterAllocate_01(void)
{
    SaNtfHandleT ntfHandle;
    SaNtfAttributeChangeNotificationFilterT myAttributeChangeFilter;
    SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles;

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

    if((rc = saNtfAttributeChangeNotificationFilterAllocate(
        ntfHandle,
        &myAttributeChangeFilter,
        0,
        0,
        0,
        1,
        0)) == SA_AIS_OK) {

    	myAttributeChangeFilter.notificationFilterHeader.notificationClassIds[0].vendorId = 33333;
    	myAttributeChangeFilter.notificationFilterHeader.notificationClassIds[0].majorId = 999;
    	myAttributeChangeFilter.notificationFilterHeader.notificationClassIds[0].minorId = 1;

    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle =
    		myAttributeChangeFilter.notificationFilterHandle;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    	myNotificationFilterHandles.stateChangeFilterHandle = 0;

    	safassert(saNtfNotificationFilterFree(
    			myNotificationFilterHandles.attributeChangeFilterHandle), SA_AIS_OK);
    }

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_OK);

}

void saNtfAttributeChangeNotificationFilterAllocate_02(void)
{
    SaNtfAttributeChangeNotificationFilterT myAttributeChangeFilter;

    rc = saNtfAttributeChangeNotificationFilterAllocate(
        0,
        &myAttributeChangeFilter,
        0,
        0,
        0,
        1,
        0);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

}

void saNtfAttributeChangeNotificationFilterAllocate_03(void)
{
    SaNtfHandleT ntfHandle;
    SaNtfAttributeChangeNotificationFilterT myAttributeChangeFilter;

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    rc = saNtfAttributeChangeNotificationFilterAllocate(
        ntfHandle,
        &myAttributeChangeFilter,
        0,
        0,
        0,
        1,
        0);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

}

void saNtfAttributeChangeNotificationFilterAllocate_04(void)
{
    SaNtfHandleT ntfHandle;

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

    rc = saNtfAttributeChangeNotificationFilterAllocate(
        ntfHandle,
        NULL,
        0,
        0,
        0,
        1,
        0);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

__attribute__ ((constructor)) static void
saNtfAttributeChangeNotificationFilterAllocate_constructor(void)
{
    test_suite_add(7, "Consumer operations - filter allocate");
    test_case_add(7,saNtfAttributeChangeNotificationFilterAllocate_01, "saNtfAttributeChangeNotificationFilterAllocate - SA_AIS_OK");
    test_case_add(7,saNtfAttributeChangeNotificationFilterAllocate_02, "saNtfAttributeChangeNotificationFilterAllocate - handle NULL SA_AIS_ERR_BAD_HANDLE");
    test_case_add(7,saNtfAttributeChangeNotificationFilterAllocate_03, "saNtfAttributeChangeNotificationFilterAllocate - handle returned SA_AIS_ERR_BAD_HANDLE");
    test_case_add(7,saNtfAttributeChangeNotificationFilterAllocate_04, "saNtfAttributeChangeNotificationFilterAllocate - SA_AIS_ERR_INVALID_PARAM");
}


