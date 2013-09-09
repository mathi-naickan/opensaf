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


void saNtfMiscellaneousNotificationAllocate_01(void) {
	test_validate(SA_AIS_ERR_NOT_SUPPORTED, SA_AIS_ERR_NOT_SUPPORTED);
#if 0
	/*  struct pollfd fds[1];*/
	/*  int ret;             */
	static SaNtfAlarmNotificationT myAlarmNotification;
	saNotificationAllocationParamsT myNotificationAllocationParams;
	saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
	saNotificationParamsT myNotificationParams;

	fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams, &myNotificationParams);

	safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion), SA_AIS_OK);
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject), SA_AIS_OK);

	AlarmNotificationParams myAlarmParams;

	myAlarmParams.numCorrelatedNotifications
			= myNotificationAllocationParams.numCorrelatedNotifications;
	myAlarmParams.lengthAdditionalText
			= myNotificationAllocationParams.lengthAdditionalText;
	myAlarmParams.numAdditionalInfo
			= myNotificationAllocationParams.numAdditionalInfo;
	myAlarmParams.numSpecificProblems
			= myNotificationAllocationParams.numSpecificProblems;
	myAlarmParams.numMonitoredAttributes
			= myNotificationAllocationParams.numMonitoredAttributes;
	myAlarmParams.numProposedRepairActions
			= myNotificationAllocationParams.numProposedRepairActions;
	myAlarmParams.variableDataSize
			= myNotificationAllocationParams.variableDataSize;

	rc = saNtfAlarmNotificationAllocate(
					ntfHandle,
					&myAlarmNotification,
					myAlarmParams.numCorrelatedNotifications,
					myAlarmParams.lengthAdditionalText,
					myAlarmParams.numAdditionalInfo,
					myAlarmParams.numSpecificProblems,
					myAlarmParams.numMonitoredAttributes,
					myAlarmParams.numProposedRepairActions,
					myAlarmParams.variableDataSize);

	safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(rc, SA_AIS_OK);
#endif
}

/**
 * Test that return value SA_AIS_ERR_BAD_HANDLE works
 *
 * Strategy: Set the handle to zero or invalid number
 *           Create a handle and then destroy it.
 */
void saNtfMiscellaneousNotificationAllocate_02(void) {
	test_validate(SA_AIS_ERR_NOT_SUPPORTED, SA_AIS_ERR_NOT_SUPPORTED);
#if 0
	/* Variable to count the number of errors */
	int errors = 0;
	static SaNtfAlarmNotificationT myAlarmNotification;
	saNotificationAllocationParamsT myNotificationAllocationParams;
	saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
	saNotificationParamsT myNotificationParams;
	AlarmNotificationParams myAlarmParams;

	fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams, &myNotificationParams);

	myAlarmParams.numCorrelatedNotifications
			= myNotificationAllocationParams.numCorrelatedNotifications;
	myAlarmParams.lengthAdditionalText
			= myNotificationAllocationParams.lengthAdditionalText;
	myAlarmParams.numAdditionalInfo
			= myNotificationAllocationParams.numAdditionalInfo;
	myAlarmParams.numSpecificProblems
			= myNotificationAllocationParams.numSpecificProblems;
	myAlarmParams.numMonitoredAttributes
			= myNotificationAllocationParams.numMonitoredAttributes;
	myAlarmParams.numProposedRepairActions
			= myNotificationAllocationParams.numProposedRepairActions;
	myAlarmParams.variableDataSize
			= myNotificationAllocationParams.variableDataSize;

	/* Set the handle to invalid number */
	rc = saNtfAlarmNotificationAllocate(
					0,
					&myAlarmNotification,
					myAlarmParams.numCorrelatedNotifications,
					myAlarmParams.lengthAdditionalText,
					myAlarmParams.numAdditionalInfo,
					myAlarmParams.numSpecificProblems,
					myAlarmParams.numMonitoredAttributes,
					myAlarmParams.numProposedRepairActions,
					myAlarmParams.variableDataSize);

	if(rc != SA_AIS_ERR_BAD_HANDLE) {
		errors++;
	}

	/* Create a handle and then destroy it */
	safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

	rc = saNtfAlarmNotificationAllocate(
					ntfHandle,
					&myAlarmNotification,
					myAlarmParams.numCorrelatedNotifications,
					myAlarmParams.lengthAdditionalText,
					myAlarmParams.numAdditionalInfo,
					myAlarmParams.numSpecificProblems,
					myAlarmParams.numMonitoredAttributes,
					myAlarmParams.numProposedRepairActions,
					myAlarmParams.variableDataSize);

	if(rc != SA_AIS_ERR_BAD_HANDLE) {
		errors++;
	}

	rc = (errors == 0)? SA_AIS_OK:  SA_AIS_ERR_BAD_HANDLE;

	test_validate(rc, SA_AIS_OK);
#endif
}

/**
 * Test that the return value SA_AIS_ERR_INVALID_PARAM works
 *
 * Strategy:
 */
void saNtfMiscellaneousNotificationAllocate_03(void) {
	test_validate(SA_AIS_ERR_NOT_SUPPORTED, SA_AIS_ERR_NOT_SUPPORTED);
#if 0
	int errors = 0;

	static SaNtfAlarmNotificationT myAlarmNotification;
	saNotificationAllocationParamsT myNotificationAllocationParams;
	saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
	saNotificationParamsT myNotificationParams;
	AlarmNotificationParams myAlarmParams;

	fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams, &myNotificationParams);

	myAlarmParams.numCorrelatedNotifications
			= myNotificationAllocationParams.numCorrelatedNotifications;
	myAlarmParams.lengthAdditionalText
			= myNotificationAllocationParams.lengthAdditionalText;
	myAlarmParams.numAdditionalInfo
			= myNotificationAllocationParams.numAdditionalInfo;
	myAlarmParams.numSpecificProblems
			= myNotificationAllocationParams.numSpecificProblems;
	myAlarmParams.numMonitoredAttributes
			= myNotificationAllocationParams.numMonitoredAttributes;
	myAlarmParams.numProposedRepairActions
			= myNotificationAllocationParams.numProposedRepairActions;
	myAlarmParams.variableDataSize
			= myNotificationAllocationParams.variableDataSize;

	safassert(saNtfInitialize(&ntfHandle, &ntfSendCallbacks, &ntfVersion), SA_AIS_OK);
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject), SA_AIS_OK);

	rc = saNtfMiscellaneousNotificationAllocate(
					ntfHandle,
					NULL,
					myAlarmParams.numCorrelatedNotifications,
					myAlarmParams.lengthAdditionalText,
					myAlarmParams.numAdditionalInfo,
					myAlarmParams.numSpecificProblems,
					myAlarmParams.numMonitoredAttributes,
					myAlarmParams.numProposedRepairActions,
					myAlarmParams.variableDataSize);
	if(rc != SA_AIS_ERR_INVALID_PARAM) {
		errors++;
	}

	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

	rc = (errors == 0)? SA_AIS_OK:  SA_AIS_ERR_INVALID_PARAM;

	test_validate(rc, SA_AIS_OK);

#endif
}

__attribute__ ((constructor)) static void saNtfMiscellaneousNotificationAllocate_constructor(
		void) {
	test_suite_add(6, "Producer API 2 allocate");
	test_case_add(6, saNtfMiscellaneousNotificationAllocate_01, "saNtfMiscellaneousNotificationAllocate, Not supported");
	test_case_add(6, saNtfMiscellaneousNotificationAllocate_02, "saNtfMiscellaneousNotificationAllocate SA_AIS_ERR_BAD_HANDLE");
	test_case_add(6, saNtfMiscellaneousNotificationAllocate_03, "saNtfMiscellaneousNotificationAllocate SA_AIS_ERR_INVALID_PARAM");
//	test_case_add(30, saNtfAlarmNotificationAllocate_04, "saNtfAlarmNotificationAllocate SA_AIS_ERR_TOO_BIG");
//	test_case_add(30, saNtfAlarmNotificationAllocate_05, "saNtfAlarmNotificationAllocate SA_AIS_ERR_UNAVAILABLE");
}

