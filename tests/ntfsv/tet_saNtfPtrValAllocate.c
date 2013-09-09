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

/**
 * Test saNtfPtrValAllocate.
 *
 */
void saNtfPtrAllocateTest_01(void)
{
    SaStringT *destPtr;
	SaNtfAlarmNotificationT myAlarmNotification;

    safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion) , SA_AIS_OK);

    safassert(saNtfAlarmNotificationAllocate(
			ntfHandle,
			&myAlarmNotification,
			0,
			0,
			0,
			0,
			0,
			2,
			SA_NTF_ALLOC_SYSTEM_LIMIT), SA_AIS_OK);

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_STRING;
    rc = saNtfPtrValAllocate(
    		myAlarmNotification.notificationHandle,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
    		(void**) &destPtr,
    		&(myAlarmNotification.proposedRepairActions[0].actionValue));


    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

/**
 * Provoke a SA_AIS_ERR_BAD_HANDLE reply
 */
void saNtfPtrAllocateTest_02(void)
{
    SaStringT *destPtr;
	SaNtfAlarmNotificationT myAlarmNotification;

    safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion) , SA_AIS_OK);

	safassert(saNtfAlarmNotificationAllocate(
			ntfHandle,
			&myAlarmNotification,
			0,
			0,
			0,
			0,
			0,
			2,
			SA_NTF_ALLOC_SYSTEM_LIMIT), SA_AIS_OK);

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_STRING;
    rc = saNtfPtrValAllocate(
    		0,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
    		(void**) &destPtr,
    		&(myAlarmNotification.proposedRepairActions[0].actionValue));

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

/**
 * Provoke a SA_AIS_ERR_BAD_HANDLE reply
 */
void saNtfPtrAllocateTest_03(void)
{
    SaStringT *destPtr;
	SaNtfAlarmNotificationT myAlarmNotification;

    safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion) , SA_AIS_OK);

	safassert(saNtfAlarmNotificationAllocate(
			ntfHandle,
			&myAlarmNotification,
			0,
			0,
			0,
			0,
			0,
			2,
			SA_NTF_ALLOC_SYSTEM_LIMIT), SA_AIS_OK);

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_STRING;

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);

    rc = saNtfPtrValAllocate(
    		myAlarmNotification.notificationHandle,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
    		(void**) &destPtr,
    		&(myAlarmNotification.proposedRepairActions[0].actionValue));

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

/**
 * Provoke a SA_AIS_ERR_INVALID_PARAM reply
 */
void saNtfPtrAllocateTest_04(void)
{
	SaNtfAlarmNotificationT myAlarmNotification;

    safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion) , SA_AIS_OK);

	safassert(saNtfAlarmNotificationAllocate(
			ntfHandle,
			&myAlarmNotification,
			0,
			0,
			0,
			0,
			0,
			2,
			SA_NTF_ALLOC_SYSTEM_LIMIT), SA_AIS_OK);

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_STRING;

    rc = saNtfPtrValAllocate(
    		myAlarmNotification.notificationHandle,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
    		NULL,
    		&(myAlarmNotification.proposedRepairActions[0].actionValue));

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

/**
 * Provoke a SA_AIS_ERR_INVALID_PARAM reply
 */
void saNtfPtrAllocateTest_05(void)
{
    SaStringT *destPtr;
	SaNtfAlarmNotificationT myAlarmNotification;

    safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion) , SA_AIS_OK);

	safassert(saNtfAlarmNotificationAllocate(
			ntfHandle,
			&myAlarmNotification,
			0,
			0,
			0,
			0,
			0,
			2,
			SA_NTF_ALLOC_SYSTEM_LIMIT), SA_AIS_OK);

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_STRING;

    rc = saNtfPtrValAllocate(
    		myAlarmNotification.notificationHandle,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
    		(void**) &destPtr,
    		NULL);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

/**
 * Test saNtfPtrValAllocate.
 *
 */
void saNtfPtrAllocateTest_06(void)
{
	SaNtfAlarmNotificationT myAlarmNotification;

	safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion) , SA_AIS_OK);
	
	rc = saNtfAlarmNotificationAllocate(
		ntfHandle,
		&myAlarmNotification,
		0,
		0,
		0,
		0,
		0,
		2,
		SA_NTF_ALLOC_SYSTEM_LIMIT -1);        
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(rc, SA_AIS_ERR_TOO_BIG);
}

__attribute__ ((constructor)) static void saNtfPtrValAllocate_constructor(void)
{
    test_suite_add(28, "Producer API ");
    test_case_add(28, saNtfPtrAllocateTest_01, "saNtfPtrValAllocate SA_AIS_OK");
    test_case_add(28, saNtfPtrAllocateTest_02, "saNtfPtrValAllocate bad handle SA_AIS_ERR_BAD_HANDLE");
    test_case_add(28, saNtfPtrAllocateTest_03, "saNtfPtrValAllocate handle freed SA_AIS_ERR_BAD_HANDLE");
    test_case_add(28, saNtfPtrAllocateTest_04, "saNtfPtrValAllocate bad dataPtr SA_AIS_ERR_INVLID_PARAM");
    test_case_add(28, saNtfPtrAllocateTest_05, "saNtfPtrValAllocate bad value pointer SA_AIS_ERR_INVLID_PARAM");
    test_case_add(28, saNtfPtrAllocateTest_06, "datasize bigger than implementation specific system limit SA_AIS_ERR_TOO_BIG");
}
