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

static int errors = 0;
/* Indicates which test to perform in the callBack */
static int testId = 0;

/* Used to keep track of which ntf we got */
static SaNtfNotificationTypeFilterHandlesT ntfRecieved;
static SaNtfSubscriptionIdT subscriptionId;
static SaNtfAlarmNotificationT myAlarmNotification;
static SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles;

static void resetCounters()
{
    errors = 0;

    ntfRecieved.alarmFilterHandle = 0;
    ntfRecieved.attributeChangeFilterHandle = 0;
    ntfRecieved.objectCreateDeleteFilterHandle = 0;
    ntfRecieved.securityAlarmFilterHandle = 0;
    ntfRecieved.stateChangeFilterHandle = 0;
}

/**
 * Test that the value retrieved is correct.
 *
 */
void test1ArrayValGet_value_ok(SaNtfSubscriptionIdT subscriptionId,
		const SaNtfNotificationsT *notification)
{
	char *srcPtr;
	SaUint16T numElements;
	SaUint16T elementSize;

	int iCount, jCount;
	const SaNtfAlarmNotificationT *ntfAlarm;

	ntfRecieved.alarmFilterHandle += 1;
	if(myNotificationFilterHandles.alarmFilterHandle == 0)
		errors +=1;
	else {
		ntfAlarm = &notification->notification.alarmNotification;
		if(assertvalue(ntfAlarm->numProposedRepairActions ==
			myAlarmNotification.numProposedRepairActions)) {
			errors += 1;
			return;
		} else {
			for(iCount = 0; iCount < ntfAlarm->numProposedRepairActions; iCount++)
			{
				if(assertvalue(ntfAlarm->proposedRepairActions[iCount].actionValueType == SA_NTF_VALUE_ARRAY)) {
					errors += 1;
					return;
				}

				if((rc = saNtfArrayValGet(ntfAlarm->notificationHandle,
						&ntfAlarm->proposedRepairActions[iCount].actionValue,
						(void **)&srcPtr,
						&numElements,
						&elementSize)) != SA_AIS_OK)
				{
					errors += 1;
					return;
				}

				for(jCount = 0; jCount < numElements; jCount++)
				{
					if(assertvalue(strncmp(srcPtr, DEFAULT_ADDITIONAL_TEXT, elementSize) == 0)) {
						errors += 1;
					}
					srcPtr += elementSize;
				}
			}
		}
	}
}

/**
 * Provoke a bad return.
 */
void test2ArrayValGet_bad_return(SaNtfSubscriptionIdT subscriptionId,
		const SaNtfNotificationsT *notification)
{
	int iCount;
	SaStringT *srcPtr;
	SaUint16T numElements;
	SaUint16T elementSize;
	SaNtfValueT myValue;

	const SaNtfAlarmNotificationT *ntfAlarm;
	ntfAlarm = &notification->notification.alarmNotification;


	if(assertvalue(ntfAlarm->numProposedRepairActions ==
		myAlarmNotification.numProposedRepairActions))
	{
		errors += 1;
		return;
	} else {
		for(iCount = 0; iCount < ntfAlarm->numProposedRepairActions; iCount++)
		{

			/* NULL in notificationHandle */
			if((rc = saNtfArrayValGet(0,
					&ntfAlarm->proposedRepairActions[iCount].actionValue,
					(void **)&srcPtr,
					&numElements,
					&elementSize)) != SA_AIS_ERR_BAD_HANDLE) errors +=1;

			/* NULL in *value */
			if((rc = saNtfArrayValGet(ntfAlarm->notificationHandle,
					NULL,
					(void **)&srcPtr,
					&numElements,
					&elementSize)) != SA_AIS_ERR_INVALID_PARAM) errors +=1;

			/* NULL **arrayPtr */
			if((rc = saNtfArrayValGet(ntfAlarm->notificationHandle,
					&ntfAlarm->proposedRepairActions[iCount].actionValue,
					NULL,
					&numElements,
					&elementSize)) != SA_AIS_ERR_INVALID_PARAM) errors +=1;

			/* NULL *numElements */
			if((rc = saNtfArrayValGet(ntfAlarm->notificationHandle,
					&ntfAlarm->proposedRepairActions[iCount].actionValue,
					(void **)&srcPtr,
					NULL,
					&elementSize)) != SA_AIS_ERR_INVALID_PARAM) errors +=1;

			/* NULL *elementSize */
			if((rc = saNtfArrayValGet(ntfAlarm->notificationHandle,
					&ntfAlarm->proposedRepairActions[iCount].actionValue,
					(void **)&srcPtr,
					&numElements,
					NULL)) != SA_AIS_ERR_INVALID_PARAM) errors +=1;

			/* actionValue not from notification */
			if((rc = saNtfArrayValGet(ntfAlarm->notificationHandle,
					&myValue,
					(void **)&srcPtr,
					&numElements,
					&elementSize)) != SA_AIS_ERR_INVALID_PARAM) errors +=1;
		}
	}
}

/**
 * Verify the contents in the notification.
 * We use the myNotificationFilterHandles to know which notifications to expect.
 */
static void saNtfNotificationCallbackT(
    SaNtfSubscriptionIdT subscriptionId,
    const SaNtfNotificationsT *notification)
{
    switch(notification->notificationType)
    {
		case SA_NTF_TYPE_ALARM:
			ntfRecieved.alarmFilterHandle += 1;
			switch(testId)
			{
			case 1:
				test1ArrayValGet_value_ok(subscriptionId, notification);
				break;
			case 2:
				test2ArrayValGet_bad_return(subscriptionId, notification);
				break;
			default:
				break;
			}
			break;

		case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		case SA_NTF_TYPE_STATE_CHANGE:
		case SA_NTF_TYPE_SECURITY_ALARM:
		default:
	        fprintf(stderr, "error: in %s at %u: Received notification type 0x%x\n",
	                __FILE__, __LINE__, notification->notificationType);
			errors +=1;
			break;
    }
    last_not_id = get_ntf_id(notification);
    SaNtfNotificationHandleT notificationHandle = \
    notification->notification.alarmNotification.notificationHandle;
    safassert(saNtfNotificationFree(notificationHandle), SA_AIS_OK);
}


static SaNtfCallbacksT ntfCbTest = {
    saNtfNotificationCallbackT,
    NULL
};


/**
 * Sets up a subscription and sends a notification.
 * The actual test is done in the callBack.
 */
void saNtfArrayGetTest_common_prep(void)
{
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    SaStringT destPtr;

    subscriptionId = 1;

    rc = SA_AIS_OK;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    /* Set up the filters  */
    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        0,
        0,
        0,
        1,
        0,
        1,
        0), SA_AIS_OK);

    /* Set perceived severities */
    myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_WARNING;

    myAlarmFilter.notificationFilterHeader.notificationClassIds[0].vendorId = ERICSSON_VENDOR_ID;
    myAlarmFilter.notificationFilterHeader.notificationClassIds[0].majorId = 12;
    myAlarmFilter.notificationFilterHeader.notificationClassIds[0].minorId = 21;

    /* Initialize filter handles */
    myNotificationFilterHandles.alarmFilterHandle =
        myAlarmFilter.notificationFilterHandle;
    myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    myNotificationFilterHandles.stateChangeFilterHandle = 0;

    /* subscribe */
    safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
                                         subscriptionId),
                                         SA_AIS_OK);

    /* Create a notification and send it */
	safassert(saNtfAlarmNotificationAllocate(
			ntfHandle,
			&myAlarmNotification,
			0,
			sizeof(DEFAULT_ADDITIONAL_TEXT),  
			0,
			0,
			0,
			1,
			SA_NTF_ALLOC_SYSTEM_LIMIT), SA_AIS_OK);

	myAlarmNotification.notificationHeader.notificationClassId->vendorId = ERICSSON_VENDOR_ID;
	myAlarmNotification.notificationHeader.notificationClassId->majorId = 12;
	myAlarmNotification.notificationHeader.notificationClassId->minorId = 21;

	*(myAlarmNotification.notificationHeader.eventType) = SA_NTF_ALARM_COMMUNICATION;
	*(myAlarmNotification.notificationHeader.eventTime) = SA_TIME_UNKNOWN;
    *(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
    *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;

	myAlarmNotification.notificationHeader.notificationObject->length = 4;
	myAlarmNotification.notificationHeader.notifyingObject->length = 4;
	strncpy((char*)myAlarmNotification.notificationHeader.notificationObject->value,
			 "nno", 4);
	strncpy((char*)myAlarmNotification.notificationHeader.notifyingObject->value,
			"ngo", 4);
	strncpy(myAlarmNotification.notificationHeader.additionalText,
		  DEFAULT_ADDITIONAL_TEXT, sizeof(DEFAULT_ADDITIONAL_TEXT));

    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_ARRAY;
    if((rc = saNtfArrayValAllocate(
    		myAlarmNotification.notificationHandle,
    		2,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
    		(void**)&destPtr,
    		&(myAlarmNotification.proposedRepairActions[0].actionValue))) == SA_AIS_OK)
    {
    	/* Copy the actual value */
    	strncpy(destPtr, DEFAULT_ADDITIONAL_TEXT, strlen(DEFAULT_ADDITIONAL_TEXT) + 1);
		destPtr += strlen(DEFAULT_ADDITIONAL_TEXT) + 1; /* move to next string */
    	strncpy(destPtr, DEFAULT_ADDITIONAL_TEXT, strlen(DEFAULT_ADDITIONAL_TEXT) + 1);

    	if(saNtfNotificationSend(myAlarmNotification.notificationHandle) == SA_AIS_OK) {
			poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);
    	}
    }

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);
    safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
}

/**
 * Test saNtfArrayValAllocate.
 *
 */
void saNtfArrayGetTest_01(void)
{
	/* Mark it for test 1 */
    testId = 1;
    saNtfArrayGetTest_common_prep();

    if(errors != 0 && rc == SA_AIS_OK) rc = SA_AIS_ERR_FAILED_OPERATION;
    test_validate(rc, SA_AIS_OK);
}

/**
 * Provoke a the function to return SA_AIS_ERR_INVALID_PARAM and SA_AIS_ERR_BAD_HANDLE
 */
void saNtfArrayGetTest_02(void)
{
	/* Mark it for test 2 */
    testId = 2;
    saNtfArrayGetTest_common_prep();

    if(errors != 0 && rc == SA_AIS_OK) rc = SA_AIS_ERR_FAILED_OPERATION;
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

__attribute__ ((constructor)) static void saNtfArrayValGet_constructor(void)
{
    test_suite_add(29, "Consumer API ");
    test_case_add(29, saNtfArrayGetTest_01, "saNtfArrayValGet SA_AIS_OK");
    test_case_add(29, saNtfArrayGetTest_02, "saNtfArrayValGet provoke SA_AIS_ERR_BAD_HANDLE/SA_AIS_ERR_INVLID_PARAM");
}
