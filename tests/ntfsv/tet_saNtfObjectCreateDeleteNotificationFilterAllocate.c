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

void saNtfObjectCreateDeleteNotificationFilterAllocate_01(void)
{
    SaNtfHandleT ntfHandle;
    SaNtfObjectCreateDeleteNotificationFilterT myObjCreDelFilter;

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

    if((rc = saNtfObjectCreateDeleteNotificationFilterAllocate(
    		ntfHandle,
    		&myObjCreDelFilter,
    		0,
    		0,
    		0,
    		1,
    		0)) == SA_AIS_OK) {
    	safassert(saNtfNotificationFilterFree(myObjCreDelFilter.notificationFilterHandle), SA_AIS_OK);
    }

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_OK);
}

void saNtfObjectCreateDeleteNotificationFilterAllocate_02(void)
{
    SaNtfObjectCreateDeleteNotificationFilterT myObjCreDelFilter;

     rc = saNtfObjectCreateDeleteNotificationFilterAllocate(
    		0,
    		&myObjCreDelFilter,
    		0,
    		0,
    		0,
    		1,
    		0);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saNtfObjectCreateDeleteNotificationFilterAllocate_03(void)
{
    SaNtfHandleT ntfHandle;
    SaNtfObjectCreateDeleteNotificationFilterT myObjCreDelFilter;

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    rc = saNtfObjectCreateDeleteNotificationFilterAllocate(
    		ntfHandle,
    		&myObjCreDelFilter,
    		0,
    		0,
    		0,
    		1,
    		0);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saNtfObjectCreateDeleteNotificationFilterAllocate_04(void)
{
    SaNtfHandleT ntfHandle;

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

    rc = saNtfObjectCreateDeleteNotificationFilterAllocate(
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
saNtfObjectCreateDeleteNotificationFilterAllocate_constructor(void)
{
    test_suite_add(7, "Consumer operations - filter allocate");
    test_case_add(7,saNtfObjectCreateDeleteNotificationFilterAllocate_01,
    		"saNtfObjectCreateDeleteNotificationFilterAllocate - SA_AIS_OK");
    test_case_add(7,saNtfObjectCreateDeleteNotificationFilterAllocate_02,
    		"saNtfObjectCreateDeleteNotificationFilterAllocate - handle null SA_AIS_ERR_BAD_HANDLE");
    test_case_add(7,saNtfObjectCreateDeleteNotificationFilterAllocate_03,
    		"saNtfObjectCreateDeleteNotificationFilterAllocate - handle returned SA_AIS_ERR_BAD_HANDLE");
    test_case_add(7,saNtfObjectCreateDeleteNotificationFilterAllocate_04,
    		"saNtfObjectCreateDeleteNotificationFilterAllocate - SA_AIS_ERR_INVALID_PARAM");
}


