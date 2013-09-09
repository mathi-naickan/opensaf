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

/* Parameter struct instances */
static saNotificationAllocationParamsT myNotificationAllocationParams;
static saNotificationFilterAllocationParamsT
		myNotificationFilterAllocationParams;
static saNotificationParamsT myNotificationParams;

void saNtfNotificationReadInitialize_01(SaNtfSearchModeT sMode, SaAisErrorT expectedRC) {
	SaNtfHandleT ntfHandle = 0;
	SaNtfSearchCriteriaT searchCriteria;
	SaNtfAlarmNotificationFilterT myAlarmFilter;
	SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles = {0,0,0,0,0};
	SaNtfReadHandleT readHandle = 0;

	searchCriteria.searchMode = sMode;

	fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams, &myNotificationParams);

	safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

	safassert(saNtfAlarmNotificationFilterAllocate(
					ntfHandle, /* handle to Notification Service instance */
					&myAlarmFilter, /* put filter here */
					/* number of event types */
					myNotificationFilterAllocationParams.numEventTypes,
					/* number of notification objects */
					myNotificationFilterAllocationParams.numNotificationObjects,
					/* number of notifying objects */
					myNotificationFilterAllocationParams.numNotifyingObjects,
					/* number of notification class ids */
					myNotificationFilterAllocationParams.numNotificationClassIds,
					/* number of probable causes */
					myNotificationFilterAllocationParams.numProbableCauses,
					/* number of perceived severities */
					myNotificationFilterAllocationParams.numPerceivedSeverities,
					/* number of trend indications */
					myNotificationFilterAllocationParams.numTrends), SA_AIS_OK);

	myNotificationFilterHandles.alarmFilterHandle
			= myAlarmFilter.notificationFilterHandle;
	myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_WARNING;
	myAlarmFilter.perceivedSeverities[1] = SA_NTF_SEVERITY_CLEARED;

	rc = saNtfNotificationReadInitialize(searchCriteria,
			&myNotificationFilterHandles, &readHandle);

        if (rc == SA_AIS_OK) safassert(saNtfNotificationReadFinalize(readHandle), SA_AIS_OK);
        safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	free(myNotificationParams.additionalText);
	test_validate(rc, expectedRC);
}

void saNtfNotificationReadInitialize_01_1(void) {
	saNtfNotificationReadInitialize_01(SA_NTF_SEARCH_AT_OR_AFTER_TIME, SA_AIS_OK);
}
void saNtfNotificationReadInitialize_01_2(void) {
	saNtfNotificationReadInitialize_01(SA_NTF_SEARCH_AT_TIME, SA_AIS_OK);
}

void saNtfNotificationReadInitialize_01_3(void) {
	saNtfNotificationReadInitialize_01(SA_NTF_SEARCH_AT_OR_AFTER_TIME, SA_AIS_OK);
}

void saNtfNotificationReadInitialize_01_4(void) {
	saNtfNotificationReadInitialize_01(SA_NTF_SEARCH_BEFORE_TIME, SA_AIS_OK);
}

void saNtfNotificationReadInitialize_01_5(void) {
	saNtfNotificationReadInitialize_01(SA_NTF_SEARCH_AFTER_TIME, SA_AIS_OK);
}

void saNtfNotificationReadInitialize_01_6(void) {
	saNtfNotificationReadInitialize_01(SA_NTF_SEARCH_NOTIFICATION_ID, SA_AIS_OK);
}

void saNtfNotificationReadInitialize_01_7(void) {
	saNtfNotificationReadInitialize_01(SA_NTF_SEARCH_ONLY_FILTER, SA_AIS_OK);
}

void saNtfNotificationReadInitialize_01_8(void) {
	saNtfNotificationReadInitialize_01(SA_NTF_SEARCH_ONLY_FILTER + 10, SA_AIS_ERR_INVALID_PARAM);
}

void saNtfNotificationReadInitialize_01_9(void) {
	saNtfNotificationReadInitialize_01(-1, SA_AIS_ERR_INVALID_PARAM);
}

/**
 * Test of NULL pointer exception.
 */
void saNtfNotificationReadInitialize_02(void) {
	SaNtfHandleT ntfHandle;
	SaNtfSearchCriteriaT searchCriteria;
	SaNtfNotificationTypeFilterHandlesT *myNotificationFilterHandles = NULL;
	SaNtfReadHandleT readHandle;

	searchCriteria.searchMode = SA_NTF_SEARCH_AT_OR_AFTER_TIME;
	safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

	rc = saNtfNotificationReadInitialize(searchCriteria,
			myNotificationFilterHandles, &readHandle);

	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

/**
 * Test of freed handler.
 */
void saNtfNotificationReadInitialize_03(void) {
	SaNtfHandleT ntfHandle;
	SaNtfSearchCriteriaT searchCriteria;
	SaNtfAlarmNotificationFilterT myAlarmFilter;
	SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles = {0,0,0,0,0};
	SaNtfReadHandleT readHandle;

	searchCriteria.searchMode = SA_NTF_SEARCH_AT_OR_AFTER_TIME;

	fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams, &myNotificationParams);

	safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);

	safassert(saNtfAlarmNotificationFilterAllocate(
					ntfHandle, /* handle to Notification Service instance */
					&myAlarmFilter, /* put filter here */
					/* number of event types */
					myNotificationFilterAllocationParams.numEventTypes,
					/* number of notification objects */
					myNotificationFilterAllocationParams.numNotificationObjects,
					/* number of notifying objects */
					myNotificationFilterAllocationParams.numNotifyingObjects,
					/* number of notification class ids */
					myNotificationFilterAllocationParams.numNotificationClassIds,
					/* number of probable causes */
					myNotificationFilterAllocationParams.numProbableCauses,
					/* number of perceived severities */
					myNotificationFilterAllocationParams.numPerceivedSeverities,
					/* number of trend indications */
					myNotificationFilterAllocationParams.numTrends), SA_AIS_OK);

	myNotificationFilterHandles.alarmFilterHandle
			= myAlarmFilter.notificationFilterHandle;

	safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);

	rc = saNtfNotificationReadInitialize(searchCriteria,
			&myNotificationFilterHandles, &readHandle);

	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	free(myNotificationParams.additionalText);
	test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}




void saNtfNotificationReadInitialize_not_supported(int filter) {
	SaNtfHandleT ntfHandle;
	SaNtfSearchCriteriaT searchCriteria;
	SaNtfAlarmNotificationFilterT myAlarmFilter;
	SaNtfObjectCreateDeleteNotificationFilterT myObjCrDeFilter;
	SaNtfAttributeChangeNotificationFilterT myAttributeChangeFilter;
	SaNtfStateChangeNotificationFilterT myStateChangefilter;
	SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles = {0,0,0,0,0};
	SaNtfReadHandleT readHandle;

	searchCriteria.searchMode = SA_NTF_SEARCH_AT_OR_AFTER_TIME;

	fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams, &myNotificationParams);

	safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion),SA_AIS_OK);

	safassert(saNtfAlarmNotificationFilterAllocate(
					ntfHandle, /* handle to Notification Service instance */
					&myAlarmFilter, /* put filter here */
					/* number of event types */
					myNotificationFilterAllocationParams.numEventTypes,
					/* number of notification objects */
					myNotificationFilterAllocationParams.numNotificationObjects,
					/* number of notifying objects */
					myNotificationFilterAllocationParams.numNotifyingObjects,
					/* number of notification class ids */
					myNotificationFilterAllocationParams.numNotificationClassIds,
					/* number of probable causes */
					myNotificationFilterAllocationParams.numProbableCauses,
					/* number of perceived severities */
					myNotificationFilterAllocationParams.numPerceivedSeverities,
					/* number of trend indications */
					myNotificationFilterAllocationParams.numTrends), SA_AIS_OK);

	if (filter == 1)
		safassert(saNtfObjectCreateDeleteNotificationFilterAllocate(
					ntfHandle, /* handle to Notification Service instance */
					&myObjCrDeFilter, /* put filter here */
					/* number of event types */
					myNotificationFilterAllocationParams.numEventTypes,
					/* number of notification objects */
					myNotificationFilterAllocationParams.numNotificationObjects,
					/* number of notifying objects */
					myNotificationFilterAllocationParams.numNotifyingObjects,
					/* number of notification class ids */
					myNotificationFilterAllocationParams.numNotificationClassIds,
					/* number of source indicators */
					0), SA_AIS_OK);
	if (filter == 2)
		safassert(rc = saNtfStateChangeNotificationFilterAllocate(
			ntfHandle,
			&myStateChangefilter,
			0,
			0,
			0,
			1,
			0,
			0), SA_AIS_OK);
	if (filter == 3)
		safassert(rc = saNtfAttributeChangeNotificationFilterAllocate(
			ntfHandle,
			&myAttributeChangeFilter,
			0,
			0,
			0,
			1,
			0), SA_AIS_OK);


	myNotificationFilterHandles.alarmFilterHandle
			= myAlarmFilter.notificationFilterHandle;
	if (filter == 1)
		myNotificationFilterHandles.objectCreateDeleteFilterHandle
		= myObjCrDeFilter.notificationFilterHandle;
	if (filter == 2)
		myNotificationFilterHandles.stateChangeFilterHandle
		= myStateChangefilter.notificationFilterHandle;
	if (filter == 3)
		myNotificationFilterHandles.attributeChangeFilterHandle
		= myAttributeChangeFilter.notificationFilterHandle;
	
	rc = saNtfNotificationReadInitialize(searchCriteria,
			&myNotificationFilterHandles, &readHandle);

        safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);
        if (filter == 1)
            safassert(saNtfNotificationFilterFree(myObjCrDeFilter.notificationFilterHandle), SA_AIS_OK);
        if (filter == 2)
            safassert(saNtfNotificationFilterFree(myStateChangefilter.notificationFilterHandle), SA_AIS_OK);
        if (filter == 3)
            safassert(saNtfNotificationFilterFree(myAttributeChangeFilter.notificationFilterHandle), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	free(myNotificationParams.additionalText);
	test_validate(rc, SA_AIS_ERR_NOT_SUPPORTED);
}

void saNtfNotificationReadInitialize_04() {
	/* try objectCreateDeleteFilter */
	saNtfNotificationReadInitialize_not_supported(1);
}

void saNtfNotificationReadInitialize_05() {
	/* try saNtfStateChangeNotificationFilter */
	saNtfNotificationReadInitialize_not_supported(2);
}

void saNtfNotificationReadInitialize_06() {
	/* try  saNtfAttributeChangeNotificationFilter */
	saNtfNotificationReadInitialize_not_supported(3);
}

__attribute__ ((constructor)) static void saNtfNotificationReadInitialize_constructor(
		void) {
	test_suite_add(20, "Consumer operations - Reader API 1");
	test_case_add(20, saNtfNotificationReadInitialize_01_1,
			"saNtfNotificationReadInitialize searchCriteria SA_NTF_SEARCH_BEFORE_OR_AT_TIME");
	test_case_add(20, saNtfNotificationReadInitialize_01_2,
			"saNtfNotificationReadInitialize searchCriteria SA_NTF_SEARCH_AT_TIME");
	test_case_add(20, saNtfNotificationReadInitialize_01_3,
			"saNtfNotificationReadInitialize searchCriteria SA_NTF_SEARCH_AT_OR_AFTER_TIME");
	test_case_add(20, saNtfNotificationReadInitialize_01_4,
			"saNtfNotificationReadInitialize searchCriteria SA_NTF_SEARCH_BEFORE_TIME");
	test_case_add(20, saNtfNotificationReadInitialize_01_5,
			"saNtfNotificationReadInitialize searchCriteria SA_NTF_SEARCH_AFTER_TIME");
	test_case_add(20, saNtfNotificationReadInitialize_01_6,
			"saNtfNotificationReadInitialize searchCriteria SA_NTF_SEARCH_NOTIFICATION_ID");
	test_case_add(20, saNtfNotificationReadInitialize_01_7,
			"saNtfNotificationReadInitialize searchCriteria SA_NTF_SEARCH_ONLY_FILTER");
	test_case_add(20, saNtfNotificationReadInitialize_01_8,
			"saNtfNotificationReadInitialize searchCriteria invalid SaNtfSearchModeT too big");
	test_case_add(20, saNtfNotificationReadInitialize_01_9,
			"saNtfNotificationReadInitialize searchCriteria invalid SaNtfSearchModeT -1");
	
	test_case_add(20, saNtfNotificationReadInitialize_02,
			"saNtfNotificationReadInitialize filter NULL pointer SA_AIS_ERR_INVALID_PARAM");
	test_case_add(20, saNtfNotificationReadInitialize_03,
			"saNtfNotificationReadInitialize filterHandle freed SA_AIS_ERR_BAD_HANDLE");
	test_case_add(20, saNtfNotificationReadInitialize_04,
			"saNtfNotificationReadInitialize multiple filters with objCreateDelete SA_AIS_ERR_NOT_SUPPORTED");
	test_case_add(20, saNtfNotificationReadInitialize_05,
			"saNtfNotificationReadInitialize multiple filters with stateChangeFilter SA_AIS_ERR_NOT_SUPPORTED");
	test_case_add(20, saNtfNotificationReadInitialize_06,
			"saNtfNotificationReadInitialize multiple filters with attributeChange SA_AIS_ERR_NOT_SUPPORTED");
}
