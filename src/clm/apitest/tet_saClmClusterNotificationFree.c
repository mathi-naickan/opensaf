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
 * Author(s): Emerson Network Power
 *
 */
#include "clmtest.h"

SaClmClusterNotificationBufferT_4 notificationBuffer_4;


void saClmClusterNotificationFree_01(void)
{
        notificationBuffer_4.numberOfItems = 1;
        notificationBuffer_4.notification = (SaClmClusterNotificationT_4*)
                malloc(sizeof(SaClmClusterNotificationT_4)*notificationBuffer_4.numberOfItems);

        safassert(saClmInitialize_4(&clmHandle, NULL, &clmVersion_4), SA_AIS_OK);
        
        rc = saClmClusterNotificationFree_4(clmHandle, notificationBuffer_4.notification);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterNotificationFree_02(void)
{
        notificationBuffer_4.numberOfItems = 1;
        notificationBuffer_4.notification = (SaClmClusterNotificationT_4*)
                malloc(sizeof(SaClmClusterNotificationT_4)*notificationBuffer_4.numberOfItems);

        safassert(saClmInitialize(&clmHandle, NULL, &clmVersion_1), SA_AIS_OK);

        rc = saClmClusterNotificationFree_4(clmHandle, notificationBuffer_4.notification);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_VERSION);
}

void saClmClusterNotificationFree_03(void)
{
        rc = saClmClusterNotificationFree_4(0, notificationBuffer_4.notification);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
        rc = saClmClusterNotificationFree_4(clmHandle, NULL);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saClmClusterNotificationFree_04(void)
{
        rc = saClmClusterNotificationFree_4(-1, notificationBuffer_4.notification);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

__attribute__ ((constructor)) static void saClmClusterNotificationFree_constructor(void)
{
        test_suite_add(10, "Test case for saClmClusterNotificationFree");
	test_case_add(10, saClmClusterNotificationFree_01, "saClmClusterNotificationFree with valid param");
	test_case_add(10, saClmClusterNotificationFree_02, "saClmClusterNotificationFree with wrong handle");
	test_case_add(10, saClmClusterNotificationFree_03, "saClmClusterNotificationFree with invalid param");
	test_case_add(10, saClmClusterNotificationFree_04, "saClmClusterNotificationFree with bad handle");
}
