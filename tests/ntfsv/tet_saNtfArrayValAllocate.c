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
 * Test saNtfArrayValAllocate.
 *
 */
void saNtfArrayAllocateTest_01(void)
{
    SaStringT *arrayPtr;
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

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_ARRAY;
    rc = saNtfArrayValAllocate(
    		myAlarmNotification.notificationHandle,
    		(SaUint16T)5,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
    		(void**) &arrayPtr,
    		&(myAlarmNotification.proposedRepairActions[0].actionValue));


    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

/**
 * Provoke a SA_AIS_ERR_BAD_HANDLE reply
 */
void saNtfArrayAllocateTest_02(void)
{
    SaStringT *arrayPtr;
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

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_ARRAY;
    rc = saNtfArrayValAllocate(
    		0,
    		(SaUint16T)5,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
    		(void**) &arrayPtr,
    		&(myAlarmNotification.proposedRepairActions[0].actionValue));


    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

/**
 * Provoke a SA_AIS_ERR_BAD_HANDLE reply
 */
void saNtfArrayAllocateTest_03(void)
{
    SaStringT *arrayPtr;
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

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_ARRAY;

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);

    rc = saNtfArrayValAllocate(
    		myAlarmNotification.notificationHandle,
    		(SaUint16T)5,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
    		(void**) &arrayPtr,
    		&(myAlarmNotification.proposedRepairActions[0].actionValue));

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

/**
 * Provoke a SA_AIS_ERR_INVALID_PARAM reply
 */
void saNtfArrayAllocateTest_04(void)
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

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_ARRAY;
    rc = saNtfArrayValAllocate(
    		myAlarmNotification.notificationHandle,
    		(SaUint16T)5,
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
void saNtfArrayAllocateTest_05(void)
{
    SaStringT *arrayPtr;
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

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_ARRAY;
    rc = saNtfArrayValAllocate(
    		myAlarmNotification.notificationHandle,
    		(SaUint16T)5,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
    		(void**) &arrayPtr,
    		NULL);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);

    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

/**
 * Test saNtfArrayValAllocate.
 *
 */
void saNtfArrayAllocateTest_06(void)
{
	SaNtfAlarmNotificationT myAlarmNotification;

	safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion) , SA_AIS_OK);
	
	rc =saNtfAlarmNotificationAllocate(
		ntfHandle,
		&myAlarmNotification,
		0,
		0,
		0,
		0,
		0,
		2,
		32768); /* works for default value NTFA_VARIABLE_DATA_LIMIT = SHRT_MAX */
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(rc, SA_AIS_ERR_TOO_BIG);
}

__attribute__ ((constructor)) static void saNtfNotificationCallbackT_constructor(void)
{
    test_suite_add(28, "Producer API ");
    test_case_add(28, saNtfArrayAllocateTest_01, "saNtfArrayValAllocate SA_AIS_OK");
    test_case_add(28, saNtfArrayAllocateTest_02, "saNtfArrayValAllocate bad handle SA_AIS_ERR_BAD_HANDLE");
    test_case_add(28, saNtfArrayAllocateTest_03, "saNtfArrayValAllocate handle freed SA_AIS_ERR_BAD_HANDLE");
    test_case_add(28, saNtfArrayAllocateTest_04, "saNtfArrayValAllocate bad dataPtr SA_AIS_ERR_INVLID_PARAM");
    test_case_add(28, saNtfArrayAllocateTest_05, "saNtfArrayValAllocate bad value pointer SA_AIS_ERR_INVLID_PARAM");
    test_case_add(28, saNtfArrayAllocateTest_06, "datasize bigger than implementation specific system SA_AIS_ERR_TOO_BIG");
}
