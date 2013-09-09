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
#include <sys/time.h>
#include "tet_ntf.h"
#include "tet_ntf_common.h"

void saNtfSecurityAlarmNotificationAllocate_01(void) {
	SaNtfSecurityAlarmNotificationT  myNotification;
	saNotificationAllocationParamsT myNotificationAllocationParams;
	saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
	saNotificationParamsT myNotificationParams;

	fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams, &myNotificationParams);

	safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion), SA_AIS_OK);

	rc = saNtfSecurityAlarmNotificationAllocate(
					ntfHandle, /* handle to Notification Service instance */
					&myNotification,
					/* number of correlated notifications */
					myNotificationAllocationParams.numCorrelatedNotifications,
					/* length of additional text */
					myNotificationAllocationParams.lengthAdditionalText,
					/* number of additional info items*/
					myNotificationAllocationParams.numAdditionalInfo,
					/* use default allocation size */
					myNotificationAllocationParams.variableDataSize);

        free(myNotificationParams.additionalText);
	safassert(saNtfNotificationFree(myNotification.notificationHandle), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(rc, SA_AIS_OK);

}

/**
 * Test that return value SA_AIS_ERR_BAD_HANDLE works
 *
 * Strategy: Set the handle to zero or invalid number
 *           Create a handle and then destroy it.
 */
void saNtfSecurityAlarmNotificationAllocate_02(void) {
	int errors = 0;
	SaNtfSecurityAlarmNotificationT  myNotification;
	saNotificationAllocationParamsT myNotificationAllocationParams;
	saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
	saNotificationParamsT myNotificationParams;

	fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams, &myNotificationParams);


	rc = saNtfSecurityAlarmNotificationAllocate(
					0, /* handle to Notification Service instance */
					&myNotification,
					/* number of correlated notifications */
					myNotificationAllocationParams.numCorrelatedNotifications,
					/* length of additional text */
					myNotificationAllocationParams.lengthAdditionalText,
					/* number of additional info items*/
					myNotificationAllocationParams.numAdditionalInfo,
					/* use default allocation size */
					myNotificationAllocationParams.variableDataSize);
	if(rc != SA_AIS_ERR_BAD_HANDLE) {
		errors++;
	}
        free(myNotificationParams.additionalText);
	safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	rc = saNtfSecurityAlarmNotificationAllocate(
					ntfHandle, /* handle to Notification Service instance */
					&myNotification,
					/* number of correlated notifications */
					myNotificationAllocationParams.numCorrelatedNotifications,
					/* length of additional text */
					myNotificationAllocationParams.lengthAdditionalText,
					/* number of additional info items*/
					myNotificationAllocationParams.numAdditionalInfo,
					/* use default allocation size */
					myNotificationAllocationParams.variableDataSize);
	if(rc != SA_AIS_ERR_BAD_HANDLE) {
		errors++;
	}

	rc = (errors == 0)? SA_AIS_OK:  SA_AIS_ERR_BAD_HANDLE;

	test_validate(rc, SA_AIS_OK);

}

void saNtfSecurityAlarmNotificationAllocate_03(void) {
	saNotificationAllocationParamsT myNotificationAllocationParams;
	saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
	saNotificationParamsT myNotificationParams;

	fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams, &myNotificationParams);

	safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion), SA_AIS_OK);

	rc = saNtfSecurityAlarmNotificationAllocate(
					ntfHandle, /* handle to Notification Service instance */
					NULL,
					/* number of correlated notifications */
					myNotificationAllocationParams.numCorrelatedNotifications,
					/* length of additional text */
					myNotificationAllocationParams.lengthAdditionalText,
					/* number of additional info items*/
					myNotificationAllocationParams.numAdditionalInfo,
					/* use default allocation size */
					myNotificationAllocationParams.variableDataSize);

        free(myNotificationParams.additionalText);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

}


__attribute__ ((constructor)) static void saNtfSecurityAlarmNotificationAllocate_constructor(void) {
	test_suite_add(6, "Producer API 2 allocate");
	test_case_add(6, saNtfSecurityAlarmNotificationAllocate_01,
			"saNtfSecurityAlarmNotificationAllocate SA_AIS_OK");
	test_case_add(6, saNtfSecurityAlarmNotificationAllocate_02,
				"saNtfSecurityAlarmNotificationAllocate SA_AIS_ERR_BAD_HANDLE");
	test_case_add(6, saNtfSecurityAlarmNotificationAllocate_03,
				"saNtfSecurityAlarmNotificationAllocate SA_AIS_ERR_INVALID_PARAM");
}

