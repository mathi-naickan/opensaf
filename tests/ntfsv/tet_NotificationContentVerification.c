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

extern int verbose;

static saNotificationParamsT myNotificationParams;
static saNotificationAllocationParamsT myNotificationAllocationParams;

static SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles;

/* Used to keep track of which ntf we got */
static SaNtfNotificationTypeFilterHandlesT ntfRecieved;

static int errors;
static SaNtfSubscriptionIdT subscriptionId;
static SaNtfAlarmNotificationT myAlarmNotification;
static SaNtfObjectCreateDeleteNotificationT myObjCrDelNotfification;
static SaNtfAttributeChangeNotificationT myAttrChangeNotification;
static SaNtfStateChangeNotificationT myStateChangeNotification;
static SaNtfSecurityAlarmNotificationT mySecAlarmNotification;

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
 * Verify the contents in the notification.
 * We use the myNotificationFilterHandles to know which notifications to expect.
 */
static void saNtfNotificationCallbackT(
    SaNtfSubscriptionIdT subscriptionId,
    const SaNtfNotificationsT *notification)
{
    SaNtfNotificationHandleT notificationHandle = 0;
    switch(notification->notificationType)
    {
		case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
                    notificationHandle = notification->notification.objectCreateDeleteNotification.notificationHandle;
			ntfRecieved.objectCreateDeleteFilterHandle += 1;
			if(myNotificationFilterHandles.objectCreateDeleteFilterHandle == 0)
				errors +=1;
			else errors += verifyObjectCreateDeleteNotification(&myObjCrDelNotfification,
					&(notification->notification.objectCreateDeleteNotification));
			break;

		case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
                    notificationHandle = notification->notification.attributeChangeNotification.notificationHandle;
			ntfRecieved.attributeChangeFilterHandle += 1;
			if(myNotificationFilterHandles.attributeChangeFilterHandle == 0)
				errors += 1;
			else errors += verifyAttributeChangeNotification(&myAttrChangeNotification,
					&(notification->notification.attributeChangeNotification));
			break;

		case SA_NTF_TYPE_STATE_CHANGE:
                    notificationHandle = notification->notification.stateChangeNotification.notificationHandle;
			ntfRecieved.stateChangeFilterHandle += 1;
			if(myNotificationFilterHandles.stateChangeFilterHandle == 0)
				errors += 1;
			else errors += verifyStateChangeNotification(&myStateChangeNotification,
					&(notification->notification.stateChangeNotification));
			break;

		case SA_NTF_TYPE_ALARM:
                    notificationHandle = notification->notification.alarmNotification.notificationHandle;
			ntfRecieved.alarmFilterHandle += 1;
			if(myNotificationFilterHandles.alarmFilterHandle == 0)
				errors +=1;
			else errors += verifyAlarmNotification(&myAlarmNotification,
					&(notification->notification.alarmNotification));
			break;

		case SA_NTF_TYPE_SECURITY_ALARM:
                    notificationHandle = notification->notification.securityAlarmNotification.notificationHandle;
			ntfRecieved.securityAlarmFilterHandle += 1;
			if(myNotificationFilterHandles.securityAlarmFilterHandle == 0)
				errors += 1;
			else errors += verifySecurityAlarmNotification(&mySecAlarmNotification,
					&(notification->notification.securityAlarmNotification));
			break;

		default:
			errors +=1;
			break;
    }
	 last_not_id = get_ntf_id(notification);
    if(verbose) {
        newNotification(subscriptionId, notification);
    } else if (notificationHandle != 0){
        safassert(saNtfNotificationFree(notificationHandle), SA_AIS_OK);
    }
}


static SaNtfCallbacksT ntfCbTest = {
    saNtfNotificationCallbackT,
    NULL
};


/**
 * Test the content of an alarm notification
 * Strategy: Set up a subscription and send a notification.
 *           Check it.
 */
void alarmNotificationTest(void)
{
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    subscriptionId = 1;

    rc = SA_AIS_OK;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    /* Set up the filters and subscription */
    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        0,
        0,
        0,
        0,
        0,
        2,
        0), SA_AIS_OK);
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

    safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
                                         subscriptionId),
                                         SA_AIS_OK);

    /* Create a notification and send it */
	safassert(saNtfAlarmNotificationAllocate(
			ntfHandle,
			&myAlarmNotification,
			0,
			(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
			0,
			0,
			0,
			0,
			SA_NTF_ALLOC_SYSTEM_LIMIT), SA_AIS_OK);

	fillHeader(&myAlarmNotification.notificationHeader);

    /* determine perceived severity */
    *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;

    /* set probable cause*/
    *(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
    *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
    myAlarmNotification.thresholdInformation->thresholdValueType = SA_NTF_VALUE_UINT32;
    myAlarmNotification.thresholdInformation->thresholdValue.uint32Val = 600;
    myAlarmNotification.thresholdInformation->thresholdHysteresis.uint32Val = 100;
    myAlarmNotification.thresholdInformation->observedValue.uint32Val = 567;
    myAlarmNotification.thresholdInformation->armTime = SA_TIME_UNKNOWN;

    safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);
    if(ntfRecieved.alarmFilterHandle != 1 ||
    		ntfRecieved.attributeChangeFilterHandle != 0 ||
    		ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
    		ntfRecieved.securityAlarmFilterHandle != 0 ||
    		ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);

    safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    /* Allocated in fillInDefaultValues() */
    free(myNotificationParams.additionalText);
    if(errors) rc = SA_AIS_ERR_FAILED_OPERATION;
    test_validate(rc, SA_AIS_OK);
}

/**
 * Test the content of an alarm notification
 * Strategy: Set up a subscription and send a notification.
 *           Check it.
 */
void alarmNotificationTest2(void)
{
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    subscriptionId = 1;
    int i;
    rc = SA_AIS_OK;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    /* Set up the filters and subscription */
    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        0,
        0,
        0,
        0,
        0,
        2,
        0), SA_AIS_OK);
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

    safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
                                         subscriptionId),
                                         SA_AIS_OK);

    /* Create a notification and send it */
    myAlarmNotification.notificationHeader.numCorrelatedNotifications = 2;
    myAlarmNotification.notificationHeader.numAdditionalInfo = 5;
	safassert(saNtfAlarmNotificationAllocate(
			ntfHandle,
			&myAlarmNotification,
			myAlarmNotification.notificationHeader.numCorrelatedNotifications,
			(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
			myAlarmNotification.notificationHeader.numAdditionalInfo,
			3, /*  SaUint16T numSpecificProblems,       */
			2, /*  SaUint16T numMonitoredAttributes,    */
			2, /*  SaUint16T numProposedRepairActions,  */
			SA_NTF_ALLOC_SYSTEM_LIMIT), /*  SaInt16T variableDataSize  */
              SA_AIS_OK);

	fillHeader(&myAlarmNotification.notificationHeader);

    /* determine perceived severity */
    *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;

    /* set probable cause*/
    *(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;

    myAlarmNotification.specificProblems[0].problemClassId.vendorId = 23;
    myAlarmNotification.specificProblems[0].problemClassId.majorId = 22;
    myAlarmNotification.specificProblems[0].problemClassId.minorId = 21;

    myAlarmNotification.specificProblems[1].problemClassId.vendorId = 5;
    myAlarmNotification.specificProblems[1].problemClassId.majorId = 4;
    myAlarmNotification.specificProblems[1].problemClassId.minorId = 2;

    myAlarmNotification.specificProblems[2].problemClassId.vendorId = 2;
    myAlarmNotification.specificProblems[2].problemClassId.majorId = 3;
    myAlarmNotification.specificProblems[2].problemClassId.minorId = 4;

    myAlarmNotification.monitoredAttributes[0].attributeId = 1;
    myAlarmNotification.monitoredAttributes[0].attributeType = SA_NTF_VALUE_INT64;
    myAlarmNotification.monitoredAttributes[0].attributeValue.int64Val = 987654321;

    myAlarmNotification.monitoredAttributes[1].attributeId = 2;
    myAlarmNotification.monitoredAttributes[1].attributeType = SA_NTF_VALUE_INT8;
    myAlarmNotification.monitoredAttributes[1].attributeValue.int8Val = 88;

    myAlarmNotification.proposedRepairActions[0].actionId = 3;
    myAlarmNotification.proposedRepairActions[0].actionValueType = SA_NTF_VALUE_INT16;
    myAlarmNotification.proposedRepairActions[0].actionValue.int16Val = 456;

    myAlarmNotification.proposedRepairActions[1].actionId = 4;
    myAlarmNotification.proposedRepairActions[1].actionValueType = SA_NTF_VALUE_INT32;
    myAlarmNotification.proposedRepairActions[1].actionValue.int32Val = 456;

    for (i = 0; i < myAlarmNotification.numSpecificProblems; i++)
    {
        myAlarmNotification.specificProblems[i].problemId = i;
        myAlarmNotification.specificProblems[i].problemClassId.majorId = 1;
        myAlarmNotification.specificProblems[i].problemClassId.minorId = 2;
        myAlarmNotification.specificProblems[i].problemClassId.vendorId = 33;
        myAlarmNotification.specificProblems[i].problemType = SA_NTF_VALUE_UINT32;
        myAlarmNotification.specificProblems[i].problemValue.uint32Val = (SaUint32T)(600+i);
    }

    for (i = 0; i < myAlarmNotification.numProposedRepairActions; i++)
    {
        myAlarmNotification.proposedRepairActions[i].actionId = i;
        myAlarmNotification.proposedRepairActions[i].actionValueType = SA_NTF_VALUE_UINT32;
        myAlarmNotification.proposedRepairActions[i].actionValue.uint32Val = (SaUint32T)(700+i);
    }
    *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
    myAlarmNotification.thresholdInformation->thresholdValueType = SA_NTF_VALUE_UINT32;
    myAlarmNotification.thresholdInformation->thresholdValue.uint32Val = 600;
    myAlarmNotification.thresholdInformation->thresholdHysteresis.uint32Val = 100;
    myAlarmNotification.thresholdInformation->observedValue.uint32Val = 567;
    myAlarmNotification.thresholdInformation->armTime = SA_TIME_UNKNOWN;

    myAlarmNotification.notificationHeader.correlatedNotifications[0] = 1999;
    myAlarmNotification.notificationHeader.correlatedNotifications[1] = 1984;

    safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);

	 if(ntfRecieved.alarmFilterHandle != 1 ||
    		ntfRecieved.attributeChangeFilterHandle != 0 ||
    		ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
    		ntfRecieved.securityAlarmFilterHandle != 0 ||
    		ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);

    safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    /* Allocated in fillInDefaultValues() */
    free(myNotificationParams.additionalText);
    if(errors) rc = SA_AIS_ERR_FAILED_OPERATION;
    test_validate(rc, SA_AIS_OK);
}


/**
 * Test the content of an ObjectCreateDeleteNotification
 * Strategy: Set up a subscription and send a notification.
 *           Check it.
 */
void objectCreateDeleteNotificationTest(void)
{
    SaNtfObjectCreateDeleteNotificationFilterT myFilter;

    subscriptionId = 2;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfObjectCreateDeleteNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		0, 0, 0, 0, 0)) == SA_AIS_OK) {

    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle =
    		myFilter.notificationFilterHandle;
    	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    	myNotificationFilterHandles.stateChangeFilterHandle = 0;

    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
    			subscriptionId),
    			SA_AIS_OK);

    	createObjectCreateDeleteNotification(ntfHandle,	&myObjCrDelNotfification);
    	safassert(saNtfNotificationSend(myObjCrDelNotfification.notificationHandle), SA_AIS_OK);

    	/* Delay to let the notification slip through */
		poll_until_received(ntfHandle, *myObjCrDelNotfification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle !=1 ||
    			ntfRecieved.securityAlarmFilterHandle != 0 ||
    			ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(myObjCrDelNotfification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }

	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
    if(errors && rc == SA_AIS_OK) rc = SA_AIS_ERR_FAILED_OPERATION;

    test_validate(rc, SA_AIS_OK);
}

/**
 * Test the content of an AttributeChangeNotification
 * Strategy: Set up a subscription and send a notification.
 *           Check it.
 */
void attributeChangeNotificationTest(void)
{
    SaNtfAttributeChangeNotificationFilterT myFilter;
    subscriptionId = 3;

    resetCounters();
    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfAttributeChangeNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		0, 0, 0, 0, 0)) == SA_AIS_OK) {

    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle =
    		myFilter.notificationFilterHandle;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    	myNotificationFilterHandles.stateChangeFilterHandle = 0;

    	/* subscribe */
    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
    			subscriptionId),
    			SA_AIS_OK);

    	createAttributeChangeNotification(ntfHandle, &myAttrChangeNotification);
    	safassert(saNtfNotificationSend(myAttrChangeNotification.notificationHandle), SA_AIS_OK);

    	/* Delay to let the notification slip through */
		poll_until_received(ntfHandle, *myAttrChangeNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 1 ||
    			ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
    			ntfRecieved.securityAlarmFilterHandle != 0 ||
    			ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(myAttrChangeNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);

    if(errors && rc == SA_AIS_OK) rc = SA_AIS_ERR_FAILED_OPERATION;
    test_validate(rc, SA_AIS_OK);
}

/**
 * Test the content of an StateChangeNotification
 * Strategy: Set up a subscription and send a notification.
 *           Check it.
 */
void stateChangeNotificationTest(void)
{
    SaNtfStateChangeNotificationFilterT myFilter;

    subscriptionId = 4;

    resetCounters();
    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfStateChangeNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		0, 0, 0, 0, 0, 0)) == SA_AIS_OK) {

    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    	myNotificationFilterHandles.stateChangeFilterHandle =
    		myFilter.notificationFilterHandle;

		/* subscribe */
		safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
				subscriptionId),
				SA_AIS_OK);

    	createStateChangeNotification(ntfHandle, &myStateChangeNotification);
    	safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);

    	/* Delay to let the notification slip through */
		poll_until_received(ntfHandle, *myStateChangeNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
    			ntfRecieved.securityAlarmFilterHandle != 0 ||
    			ntfRecieved.stateChangeFilterHandle != 1) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(myStateChangeNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);

    if(errors && rc == SA_AIS_OK) rc = SA_AIS_ERR_FAILED_OPERATION;
    test_validate(rc, SA_AIS_OK);
}

/**
 * Test the content of an SecurityAlarmNotification
 * Strategy: Set up a subscription and send a notification.
 *           Check it.
 */
void securityAlarmNotificationTest(void)
{
    SaNtfSecurityAlarmNotificationFilterT myFilter;

    subscriptionId = 5;

    resetCounters();
    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfSecurityAlarmNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		0, 0, 0, 0, 0,0,0,0,0)) == SA_AIS_OK) {

    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle =
    		myFilter.notificationFilterHandle;
    	myNotificationFilterHandles.stateChangeFilterHandle = 0;

    	/* subscribe */
    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
    			subscriptionId),
    			SA_AIS_OK);

    	createSecurityAlarmNotification(ntfHandle, &mySecAlarmNotification);
    	safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

    	/* Delay to let the notification slip through */
		poll_until_received(ntfHandle, *mySecAlarmNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
    			ntfRecieved.securityAlarmFilterHandle != 1 ||
    			ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }

	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);

    if(errors && rc == SA_AIS_OK) rc = SA_AIS_ERR_FAILED_OPERATION;
    test_validate(rc, SA_AIS_OK);
}

/**
 * Test the content of an MiscellaneousNotification
 * Strategy: Set up a subscription and send a notification.
 *           Check it.
 */
void miscellaneousNotificationTest(void)
{
	/* TODO: Implement test once the API exist */
	subscriptionId = 6;

	resetCounters();
    test_validate(SA_AIS_ERR_NOT_SUPPORTED, SA_AIS_ERR_NOT_SUPPORTED);
}

/**
 * Test the content of several notifications at once
 * Strategy: Set up a subscription and send a notification.
 *           Check it.
 */
void allNotificationTest(void)
{
    rc = SA_AIS_OK;
    saNotificationFilterAllocationParamsT  myNotificationFilterAllocationParams;

    SaNtfAlarmNotificationFilterT          	   myAlarmFilter;
    SaNtfObjectCreateDeleteNotificationFilterT myObjCrDeFilter;
    SaNtfAttributeChangeNotificationFilterT    myAttrChangeFilter;
    SaNtfStateChangeNotificationFilterT        myStateChangeFilter;
    SaNtfSecurityAlarmNotificationFilterT      mySecAlarmFilter;

    subscriptionId = 7;

    resetCounters();
    fillInDefaultValues(&myNotificationAllocationParams,
                        &myNotificationFilterAllocationParams,
                        &myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);

    if((rc = saNtfAlarmNotificationFilterAllocate(
    		ntfHandle,
    		&myAlarmFilter,
    		myNotificationFilterAllocationParams.numEventTypes,
    		myNotificationFilterAllocationParams.numNotificationObjects,
    		myNotificationFilterAllocationParams.numNotifyingObjects,
    		myNotificationFilterAllocationParams.numNotificationClassIds,
    		myNotificationFilterAllocationParams.numProbableCauses,
    		myNotificationFilterAllocationParams.numPerceivedSeverities,
    		myNotificationFilterAllocationParams.numTrends)) == SA_AIS_OK) {

    	/* Set perceived severities */
    	myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_WARNING;
    	myAlarmFilter.perceivedSeverities[1] = SA_NTF_SEVERITY_CLEARED;


    	if((rc = saNtfObjectCreateDeleteNotificationFilterAllocate(
    			ntfHandle,
    			&myObjCrDeFilter,
    			myNotificationFilterAllocationParams.numEventTypes,
    			myNotificationFilterAllocationParams.numNotificationObjects,
    			myNotificationFilterAllocationParams.numNotifyingObjects,
    			myNotificationFilterAllocationParams.numNotificationClassIds,
    			myNotificationFilterAllocationParams.numSourceIndicators)) == SA_AIS_OK) {

    		if((rc = saNtfAttributeChangeNotificationFilterAllocate(
    				ntfHandle,
    				&myAttrChangeFilter,
    				myNotificationFilterAllocationParams.numEventTypes,
    				myNotificationFilterAllocationParams.numNotificationObjects,
    				myNotificationFilterAllocationParams.numNotifyingObjects,
    				myNotificationFilterAllocationParams.numNotificationClassIds,
    				myNotificationFilterAllocationParams.numSourceIndicators)) == SA_AIS_OK) {

    			if((rc = saNtfSecurityAlarmNotificationFilterAllocate(
    					ntfHandle,
    					&mySecAlarmFilter,
    					myNotificationFilterAllocationParams.numEventTypes,
    					myNotificationFilterAllocationParams.numNotificationObjects,
    					myNotificationFilterAllocationParams.numNotifyingObjects,
    					myNotificationFilterAllocationParams.numNotificationClassIds,
    					myNotificationFilterAllocationParams.numProbableCauses,
    					myNotificationFilterAllocationParams.numSeverities,
    					myNotificationFilterAllocationParams.numSecurityAlarmDetectos,
    					myNotificationFilterAllocationParams.numServiceUsers,
    					myNotificationFilterAllocationParams.numServiceProviders)) == SA_AIS_OK) {

    				if((rc = saNtfStateChangeNotificationFilterAllocate(
    						ntfHandle,
    						&myStateChangeFilter,
    						myNotificationFilterAllocationParams.numEventTypes,
    						myNotificationFilterAllocationParams.numNotificationObjects,
    						myNotificationFilterAllocationParams.numNotifyingObjects,
    						myNotificationFilterAllocationParams.numNotificationClassIds,
    						myNotificationFilterAllocationParams.numSourceIndicators,
    						myNotificationFilterAllocationParams.numChangedStates)) == SA_AIS_OK) {

    					/* Initialize filter handles */
    					myNotificationFilterHandles.alarmFilterHandle =
    						myAlarmFilter.notificationFilterHandle;
    					myNotificationFilterHandles.attributeChangeFilterHandle =
    						myAttrChangeFilter.notificationFilterHandle;
    					myNotificationFilterHandles.objectCreateDeleteFilterHandle =
    						myObjCrDeFilter.notificationFilterHandle;
    					myNotificationFilterHandles.securityAlarmFilterHandle =
    						mySecAlarmFilter.notificationFilterHandle;
    					myNotificationFilterHandles.stateChangeFilterHandle =
    						myStateChangeFilter.notificationFilterHandle;

    					/* subscribe */
    					safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
    							subscriptionId),
    							SA_AIS_OK);

    					/* Create a couple of notifications and send them */
    					createAlarmNotification(ntfHandle, &myAlarmNotification);
    					safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

    					createObjectCreateDeleteNotification(ntfHandle,	&myObjCrDelNotfification);
    					safassert(saNtfNotificationSend(myObjCrDelNotfification.notificationHandle), SA_AIS_OK);

    					createAttributeChangeNotification(ntfHandle, &myAttrChangeNotification);
    					safassert(saNtfNotificationSend(myAttrChangeNotification.notificationHandle), SA_AIS_OK);

    					createStateChangeNotification(ntfHandle, &myStateChangeNotification);
    					safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);

    					createSecurityAlarmNotification(ntfHandle, &mySecAlarmNotification);
    					safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

    					/* TODO: add MiscellaneousNotification */
						poll_until_received(ntfHandle, *mySecAlarmNotification.notificationHeader.notificationId);

    					if(ntfRecieved.alarmFilterHandle != 1 ||
    							ntfRecieved.attributeChangeFilterHandle != 1 ||
    							ntfRecieved.objectCreateDeleteFilterHandle !=1 ||
    							ntfRecieved.securityAlarmFilterHandle != 1 ||
    							ntfRecieved.stateChangeFilterHandle != 1){ 
							fprintf(stderr, "ntfreceived fh: a: %llu, att: %llu, o: %llu, se: %llu, st: %llu \n",
								ntfRecieved.alarmFilterHandle,
								ntfRecieved.attributeChangeFilterHandle,
								ntfRecieved.objectCreateDeleteFilterHandle,
								ntfRecieved.securityAlarmFilterHandle,
								ntfRecieved.stateChangeFilterHandle);
								safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);
						}

    					safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    					safassert(saNtfNotificationFree(myObjCrDelNotfification.notificationHandle), SA_AIS_OK);
    					safassert(saNtfNotificationFree(myAttrChangeNotification.notificationHandle), SA_AIS_OK);
    					safassert(saNtfNotificationFree(myStateChangeNotification.notificationHandle), SA_AIS_OK);
    					safassert(saNtfNotificationFree(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

    					/* TODO: add MiscellaneousNotification free */

    					safassert(saNtfNotificationUnsubscribe(subscriptionId),
    							SA_AIS_OK);

    					safassert(saNtfNotificationFilterFree(myStateChangeFilter.notificationFilterHandle), SA_AIS_OK);
    				}
    				safassert(saNtfNotificationFilterFree(mySecAlarmFilter.notificationFilterHandle), SA_AIS_OK);
    			}
    			safassert(saNtfNotificationFilterFree(myAttrChangeFilter.notificationFilterHandle), SA_AIS_OK);
    		}
    		safassert(saNtfNotificationFilterFree(myObjCrDeFilter.notificationFilterHandle), SA_AIS_OK);
    	}
    	safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);
    }

    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);

    /* Allocated in fillInDefaultValues() */
    free(myNotificationParams.additionalText);
    if(errors && rc == SA_AIS_OK) rc = SA_AIS_ERR_FAILED_OPERATION;
    test_validate(rc, SA_AIS_OK);
}

__attribute__ ((constructor)) static void notificationContentVerification_constructor(void)
{
    test_suite_add(30, "Notification Content Verification");
    test_case_add(30, alarmNotificationTest, "AlarmNotification");
    test_case_add(30, alarmNotificationTest2, "AlarmNotification values");
    test_case_add(30, objectCreateDeleteNotificationTest, "ObjectCreateDeleteNotification");
    test_case_add(30, attributeChangeNotificationTest, "AttributeChangeNotification");
    test_case_add(30, stateChangeNotificationTest, "StateChangeNotification");
    test_case_add(30, securityAlarmNotificationTest, "SecurityAlarmNotification");
    test_case_add(30, miscellaneousNotificationTest, "MiscellaneousNotification, Not supported");
    test_case_add(30, allNotificationTest, "All Notifications at once");
}
