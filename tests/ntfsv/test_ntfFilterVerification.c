/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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
/** 
 * This file contains filter tests for NTF 
 * Strategy: Set up a subscription with wanted filter and send several 
 * notifications. Compare received notification id(s) with the expected id(s). 
 * If a non expected notification id is recived an error has occured and the 
 * test will fail. 
 * Tests are run in a singel thread and poll is used to recive notifications 
 * after they have been sent. A test if the last sent id is received 
 * (which has to be expected) before stoping the polling.
 */
#include <utest.h>
#include <util.h>
#include "tet_ntf.h"
#include "tet_ntf_common.h"

#define NOTIF_OBJECT_TEST "safComp=CompT_SC_FAKE,safSu=SuT_SC_FAKE,safNode=SC_2_1"
#define NOTIF_OBJECT_TEST2 "safComp=CompT_SC_FAKE,safSu=SuT_SC_FAKE,safNode=SC_2_2"
#define NOTIF_OBJECT_TEST3 "safComp=CompT_SC_ANOTHER_FAKE,safSu=SuT_SC_FAKE,safNode=SC_2_1"
#define NOTIF_OBJECT_TEST4 "safComp=CompT_SC_FAKE,safSu=SuT_SC_JOKE,safNode=SC_2_1"
#define NOTIF_OBJECT_NO_EXIST "safComp=CompT_SC_FAKE,safSu=SuT_SC_NOTEXIST,safNode=SC_2_1"
#define NOTIF_OBJECT_TEST_SU "safSu=SuT_SC_FAKE"
#define NOTIF_OBJECT_TEST_SU2 "safSu=SuT_SC_JOKE"
#define NOTIFYING_OBJECT_TEST "AVND"
#define NTF_REST_MAX_IDS 30
struct not_idsT {
	int length;
	SaNtfIdentifierT ids[NTF_REST_MAX_IDS];
};

static struct not_idsT received_ids = {0,}; 
static struct not_idsT ok_ids = {0,};

static int cb_index = 0;

extern int verbose;
static int id_verbose = 0; /* set to 1 if debuging ids */

static SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles;

/* Used to keep track of which ntf we got */
static SaNtfNotificationTypeFilterHandlesT ntfRecieved;

static int errors;
static SaNtfSubscriptionIdT subscriptionId;
static SaNtfAlarmNotificationT myAlarmNotification;
static SaNtfObjectCreateDeleteNotificationT myObjCrDelNotification;
static SaNtfAttributeChangeNotificationT myAttrChangeNotification;
static SaNtfStateChangeNotificationT myStateChangeNotification;
static SaNtfSecurityAlarmNotificationT mySecAlarmNotification;

static void saNameSet(SaNameT *sn, char* s)
{
	sn->length = strlen (s);
	(void)memcpy(sn->value, s, sn->length);
}

/**
 * Store all recieved notificationIds
 */
static void ntf_id_store(SaNtfIdentifierT n_id)
{
	assert(NTF_REST_MAX_IDS > received_ids.length); 
	received_ids.ids[received_ids.length++] = n_id;
}

/**
 * Print all notificationIds that is expected to be received. 
 * Used for debuging, 'id_verbose' must be set. 
 */
static void printOkIds()
{
	int j;
	if (!id_verbose)
		return;
	for (j= 0; j< ok_ids.length; j++)
		printf("ok_id: %llu\n", ok_ids.ids[j]);
}

/**
 * Post process all recived notificationIds towards the ids that
 * are expected to be received. 
 */
static SaAisErrorT check_errors()
{
	int i, j, found;
	SaAisErrorT rc = SA_AIS_OK;
	
	for (i = 0; i< received_ids.length; i++) {
		found=0;
		for (j= 0; j< ok_ids.length; j++) {
			if (received_ids.ids[i] == ok_ids.ids[j]){
				found = 1;
				break;
			}
		}
		if(!found)
			errors++;
	}
	if(errors)
	{ 
		rc = SA_AIS_ERR_FAILED_OPERATION;
		printf("num of failed notifications: %d(%d)\n", errors, cb_index);
	}
	printOkIds();
	return rc; 
}

static void resetCounters()
{
    errors = 0;
	 received_ids.length = 0;
	 ok_ids.length = 0;
	 ntfRecieved.alarmFilterHandle = 0;
    ntfRecieved.attributeChangeFilterHandle = 0;
    ntfRecieved.objectCreateDeleteFilterHandle = 0;
    ntfRecieved.securityAlarmFilterHandle = 0;
    ntfRecieved.stateChangeFilterHandle = 0;
	 cb_index=0;
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
	cb_index++;
/* printf("received Type %#x\n", notification->notificationType);*/
	switch(notification->notificationType)
	{
		case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
                    notificationHandle = notification->notification.objectCreateDeleteNotification.notificationHandle;
			ntfRecieved.objectCreateDeleteFilterHandle += 1;
			if(myNotificationFilterHandles.objectCreateDeleteFilterHandle == 0)
				errors +=1;
			else 
				ntf_id_store(*notification->notification.objectCreateDeleteNotification.notificationHeader.notificationId);
			break;

		case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
                    notificationHandle = notification->notification.attributeChangeNotification.notificationHandle;
			ntfRecieved.attributeChangeFilterHandle += 1;
			if(myNotificationFilterHandles.attributeChangeFilterHandle == 0)
				errors += 1;
			else 
				 ntf_id_store(*notification->notification.attributeChangeNotification.notificationHeader.notificationId);
			break;

		case SA_NTF_TYPE_STATE_CHANGE:
                    notificationHandle = notification->notification.stateChangeNotification.notificationHandle;
			ntfRecieved.stateChangeFilterHandle += 1;
			if(myNotificationFilterHandles.stateChangeFilterHandle == 0)
				errors += 1;
			else 
				ntf_id_store(*notification->notification.stateChangeNotification.notificationHeader.notificationId);
			break;

		case SA_NTF_TYPE_ALARM:
                    notificationHandle = notification->notification.alarmNotification.notificationHandle;
			ntfRecieved.alarmFilterHandle += 1;
			if(myNotificationFilterHandles.alarmFilterHandle == 0) {
				printf("alarmFilterHandle == 0\n");				
				errors +=1;
			} else 
				ntf_id_store(*notification->notification.alarmNotification.notificationHeader.notificationId);
			break;

		case SA_NTF_TYPE_SECURITY_ALARM:
                    notificationHandle = notification->notification.securityAlarmNotification.notificationHandle;
			ntfRecieved.securityAlarmFilterHandle += 1;
			if(myNotificationFilterHandles.securityAlarmFilterHandle == 0)
				errors += 1;
			else 
				ntf_id_store(*notification->notification.securityAlarmNotification.notificationHeader.notificationId);
			break;

		default:
			errors +=1;
			assert(0);
			break;
    }
	last_not_id = get_ntf_id(notification);
	if (id_verbose)
		printf("received id %llu\n", last_not_id);
    if(verbose) {
        newNotification(subscriptionId, notification);
    } else if (notificationHandle != 0) {
        safassert(saNtfNotificationFree(notificationHandle), SA_AIS_OK);
    }
}


static SaNtfCallbacksT ntfCbTest = {
    saNtfNotificationCallbackT,
    NULL
};

/**  
 * Test all filter header fields 
 */
void headerTest(void)
{
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    subscriptionId = 1;
	 SaNtfNotificationHeaderT *head;
	 
    rc = SA_AIS_OK;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    /* specify complex filter */
    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        1,
        1,
        1,
        1,
        0,
        0,
        0), SA_AIS_OK);
	 /* set header filter specific fields */
	 fillFilterHeader(&myAlarmFilter.notificationFilterHeader);

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

	head = &myAlarmNotification.notificationHeader;
	fillHeader(head); 

	/* These 3 fields is alarm filter items */
    *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
    *(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
    *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
	  
    myAlarmNotification.thresholdInformation->thresholdValueType = SA_NTF_VALUE_UINT32;
    myAlarmNotification.thresholdInformation->thresholdValue.uint32Val = 600;
    myAlarmNotification.thresholdInformation->thresholdHysteresis.uint32Val = 100;
    myAlarmNotification.thresholdInformation->observedValue.uint32Val = 567;
    myAlarmNotification.thresholdInformation->armTime = SA_TIME_UNKNOWN;

	 *(head->eventType) = SA_NTF_ALARM_EQUIPMENT;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 fillHeader(head);  
	 head->notificationObject->length = strlen(NOTIF_OBJECT_TEST);
	 (void)memcpy(head->notificationObject->value, NOTIF_OBJECT_TEST, head->notificationObject->length);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 
	 fillHeader(head);  
	 head->notifyingObject->length = strlen(NOTIFYING_OBJECT_TEST);
	 (void)memcpy(head->notifyingObject->value, NOTIFYING_OBJECT_TEST, head->notifyingObject->length);   
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 fillHeader(head);  
	 head->notificationClassId->vendorId = 199;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 fillHeader(head);  

	 head->notificationClassId->majorId = 89;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 fillHeader(head);  
	 head->notificationClassId->minorId = 24;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 fillHeader(head);  	 
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 
	 /* only this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;
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
	 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/**
 * Test HeaderFilter eventType 
 */
void headerTest2(void)
{
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    subscriptionId = 1;
	 SaNtfNotificationHeaderT *head;
	 
    rc = SA_AIS_OK;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    /* specify complex filter */
	 
    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        2,
        0,
        0,
        0,
        0,
        0,
        0), SA_AIS_OK);
	 /* only this eventType should be received */
	 myAlarmFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_ALARM_EQUIPMENT;
	 myAlarmFilter.notificationFilterHeader.eventTypes[1] = SA_NTF_ALARM_PROCESSING;

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
	/* These 3 fields is alarm filter items */
    *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
    *(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
    *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;

	head = &myAlarmNotification.notificationHeader;
	fillHeader(head); 
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 	 
	 *(head->eventType) = SA_NTF_ALARM_EQUIPMENT;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;
	 
	 fillHeader(head);  
	 *(head->eventType) = SA_NTF_TYPE_ALARM;
	 head->notifyingObject->length = strlen(NOTIFYING_OBJECT_TEST);
	 (void)memcpy(head->notifyingObject->value, NOTIFYING_OBJECT_TEST, head->notifyingObject->length);   
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 fillHeader(head);  
	 *(head->eventType) = SA_NTF_ALARM_PROCESSING;
	 head->notificationClassId->vendorId = 199;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;

	 fillHeader(head);  
	 *(head->eventType) = SA_NTF_ALARM_ENVIRONMENT;
	 head->notificationClassId->majorId = 89;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 fillHeader(head);  
	 head->notificationClassId->minorId = 24;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 

	 fillHeader(head);  
	 head->notificationObject->length = strlen(NOTIF_OBJECT_TEST);
	 (void)memcpy(head->notificationObject->value, NOTIF_OBJECT_TEST, head->notificationObject->length);
	 *(head->eventType) = SA_NTF_ALARM_EQUIPMENT;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;
	 poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);
	 if(ntfRecieved.alarmFilterHandle != 3 ||
		 ntfRecieved.attributeChangeFilterHandle != 0 ||
		 ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
		 ntfRecieved.securityAlarmFilterHandle != 0 ||
		 ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);

    safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/**
 * Test HeaderFilter notificationObjects 
 */
void headerTest3(void)
{
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    subscriptionId = 1;
	 SaNtfNotificationHeaderT *head;
	 
    rc = SA_AIS_OK;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    /* specify complex filter */
	 
    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        0,
        2,
        0,
        0,
        0,
        0,
        0), SA_AIS_OK);
	 /* only these su objects should be received */
	 saNameSet(&myAlarmFilter.notificationFilterHeader.notificationObjects[0], NOTIF_OBJECT_TEST_SU);
	 saNameSet(&myAlarmFilter.notificationFilterHeader.notificationObjects[1], NOTIF_OBJECT_TEST_SU2);

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
	 /* These 3 fields is alarm filter items */
	 *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	 *(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
	 *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
	 
	 head = &myAlarmNotification.notificationHeader;
	 fillHeader(head); 
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	 saNameSet(head->notificationObject, NOTIF_OBJECT_TEST2);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;
	 
	 fillHeader(head);  
	 head->notifyingObject->length = strlen(NOTIFYING_OBJECT_TEST);
	 (void)memcpy(head->notifyingObject->value, NOTIFYING_OBJECT_TEST, head->notifyingObject->length);   
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 fillHeader(head);  
	 head->notificationClassId->vendorId = 199;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 saNameSet(head->notificationObject, NOTIF_OBJECT_TEST3);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;

	 fillHeader(head);  
	 head->notificationClassId->majorId = 89;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 saNameSet(head->notificationObject, NOTIF_OBJECT_TEST4);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;

	 fillHeader(head);  
	 head->notificationClassId->minorId = 24;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 

	 fillHeader(head);  
	 saNameSet(head->notificationObject, NOTIF_OBJECT_NO_EXIST);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 
	 saNameSet(head->notificationObject, NOTIF_OBJECT_TEST3);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;

	 poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);
	 if(ntfRecieved.alarmFilterHandle != ok_ids.length||
		 ntfRecieved.attributeChangeFilterHandle != 0 ||
		 ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
		 ntfRecieved.securityAlarmFilterHandle != 0 ||
		 ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);

    safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/**
 * Test HeaderFilter notifyingObject 
 */
void headerTest4(void)
{
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    subscriptionId = 1;
	 SaNtfNotificationHeaderT *head;
	 
    rc = SA_AIS_OK;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    /* specify complex filter */
	 
    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        0,
        0,
        2,
        0,
        0,
        0,
        0), SA_AIS_OK);
	 /* only these su objects should be received */
	 saNameSet(&myAlarmFilter.notificationFilterHeader.notifyingObjects[0], NOTIF_OBJECT_TEST_SU);
	 saNameSet(&myAlarmFilter.notificationFilterHeader.notifyingObjects[1], NOTIF_OBJECT_TEST_SU2);

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
	 /* These 3 fields is alarm filter items */
	 *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	 *(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
	 *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
	 
	 head = &myAlarmNotification.notificationHeader;
	 fillHeader(head); 
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	 saNameSet(head->notifyingObject, NOTIF_OBJECT_TEST2);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;
	 
	 fillHeader(head);  
	 head->notifyingObject->length = strlen(NOTIFYING_OBJECT_TEST);
	 (void)memcpy(head->notifyingObject->value, NOTIFYING_OBJECT_TEST, head->notifyingObject->length);   
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 fillHeader(head);  
	 head->notificationClassId->vendorId = 199;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 saNameSet(head->notifyingObject, NOTIF_OBJECT_TEST3);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;

	 fillHeader(head);  
	 head->notificationClassId->majorId = 89;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	 saNameSet(head->notifyingObject, NOTIF_OBJECT_TEST4);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;

	 fillHeader(head);  
	 head->notificationClassId->minorId = 24;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 

	 fillHeader(head);  
	 saNameSet(head->notifyingObject, NOTIF_OBJECT_NO_EXIST);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 
	 saNameSet(head->notifyingObject, NOTIF_OBJECT_TEST3);
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;

	 poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);
	 if(ntfRecieved.alarmFilterHandle != ok_ids.length||
		 ntfRecieved.attributeChangeFilterHandle != 0 ||
		 ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
		 ntfRecieved.securityAlarmFilterHandle != 0 ||
		 ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);

    safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/**
 * Test HeaderFilter notificationClassIds 
 */
void headerTest5(void)
{
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    subscriptionId = 1;
	 SaNtfNotificationHeaderT *head;
	 
    rc = SA_AIS_OK;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    /* specify complex filter */
	 
    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        0,
        0,
        0,
        2,
        0,
        0,
        0), SA_AIS_OK);
	 myAlarmFilter.notificationFilterHeader.notificationClassIds[0].vendorId = ERICSSON_VENDOR_ID;
	 myAlarmFilter.notificationFilterHeader.notificationClassIds[0].majorId = 32;
	 myAlarmFilter.notificationFilterHeader.notificationClassIds[0].minorId = 55;

	 myAlarmFilter.notificationFilterHeader.notificationClassIds[1].vendorId = 144;
	 myAlarmFilter.notificationFilterHeader.notificationClassIds[1].majorId = 44;
	 myAlarmFilter.notificationFilterHeader.notificationClassIds[1].minorId = 22;

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
	/* These 3 fields is alarm filter items */
    *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
    *(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
    *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;

	 head = &myAlarmNotification.notificationHeader;
	 fillHeader(head); 
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 
	 head->notificationClassId->vendorId = ERICSSON_VENDOR_ID;
	 head->notificationClassId->majorId = 32;
	 head->notificationClassId->minorId = 55;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;
	 
	 head->notificationClassId->majorId = 89;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 
	 fillHeader(head);  
	 head->notifyingObject->length = strlen(NOTIFYING_OBJECT_TEST);
	 (void)memcpy(head->notifyingObject->value, NOTIFYING_OBJECT_TEST, head->notifyingObject->length);   
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 
	 fillHeader(head);  
	 /* this notification should also be caught */
	 head->notificationClassId->vendorId = 144;
	 head->notificationClassId->majorId = 44;
	 head->notificationClassId->minorId = 22;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;

	 head->notificationClassId->vendorId = 199;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 
	 
	 head->notificationClassId->vendorId = 144;
	 head->notificationClassId->minorId = 24;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 

	 fillHeader(head);  
	 head->notificationObject->length = strlen(NOTIF_OBJECT_TEST);
	 (void)memcpy(head->notificationObject->value, NOTIF_OBJECT_TEST, head->notificationObject->length);
	 head->notificationClassId->vendorId = ERICSSON_VENDOR_ID;
	 head->notificationClassId->majorId = 32;
	 head->notificationClassId->minorId = 55;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught */
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;
	 poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);
	 if(ntfRecieved.alarmFilterHandle != 3 ||
		 ntfRecieved.attributeChangeFilterHandle != 0 ||
		 ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
		 ntfRecieved.securityAlarmFilterHandle != 0 ||
		 ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);

    safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/* 
 * Set all alarm notification fields
*/
void alarmNotificationFilterTest(void)
{
    SaNtfAlarmNotificationFilterT          myAlarmFilter;
    subscriptionId = 1;

    rc = SA_AIS_OK;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    safassert(saNtfAlarmNotificationFilterAllocate(
        ntfHandle,
        &myAlarmFilter,
        0,
        0,
        0,
        0,
        1,
        1,
        1), SA_AIS_OK);
    /* Set perceived severities */
	 myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_WARNING;
	 myAlarmFilter.probableCauses[0] = SA_NTF_BANDWIDTH_REDUCED;
	 myAlarmFilter.trends[0] = SA_NTF_TREND_MORE_SEVERE;

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
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_CRITICAL;
	*(myAlarmNotification.probableCause) = SA_NTF_CALL_ESTABLISHMENT_ERROR;
	*myAlarmNotification.trend = SA_NTF_TREND_NO_CHANGE;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 
	 *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	 *(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 
	 *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	 *(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
	 *myAlarmNotification.trend = SA_NTF_TREND_NO_CHANGE;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 
	 
	 fillHeader(&myAlarmNotification.notificationHeader);
	  /* These 3 fields is filter items */
	  *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	  *(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
	  *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
	  myAlarmNotification.thresholdInformation->thresholdValueType = SA_NTF_VALUE_UINT32;
	  myAlarmNotification.thresholdInformation->thresholdValue.uint32Val = 600;
	  myAlarmNotification.thresholdInformation->thresholdHysteresis.uint32Val = 100;
	  myAlarmNotification.thresholdInformation->observedValue.uint32Val = 567;
	  myAlarmNotification.thresholdInformation->armTime = SA_TIME_UNKNOWN;
	  safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	  /* only this notification should be caught*/
	  ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;	 
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
    rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}


/**
 * Test AlarmNotification probableCause 
 */
void alarmNotificationFilterTest2(void)
{
	SaNtfAlarmNotificationFilterT          myAlarmFilter;
	subscriptionId = 1;
	
	rc = SA_AIS_OK;
	
	resetCounters();
	
	safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);
	
	safassert(saNtfAlarmNotificationFilterAllocate(
		ntfHandle,
		&myAlarmFilter,
		0,
		0,
		0,
		0,
		3,
		0,
		0), SA_AIS_OK);
    /* Set perceived severities */
	 myAlarmFilter.probableCauses[0] = SA_NTF_BANDWIDTH_REDUCED;
	 myAlarmFilter.probableCauses[1] = SA_NTF_CORRUPT_DATA;
	 myAlarmFilter.probableCauses[2] = SA_NTF_UNSPECIFIED_REASON;

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
	*(myAlarmNotification.probableCause) = SA_NTF_UNSPECIFIED_REASON;
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_CRITICAL;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	/* this notification should be caught*/
	ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;	 
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_ADAPTER_ERROR;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_APPLICATION_SUBSYSTEM_FAILURE;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	*(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
	*myAlarmNotification.trend = SA_NTF_TREND_NO_CHANGE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 
	/* this notification should be caught*/
	ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;	 
	
	*(myAlarmNotification.probableCause) = SA_NTF_POWER_PROBLEM;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_INDETERMINATE;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	*(myAlarmNotification.probableCause) = SA_NTF_BREACH_OF_CONFIDENTIALITY;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 
	
	 fillHeader(&myAlarmNotification.notificationHeader);
	 /* These 3 fields is filter items */
	 *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	 *(myAlarmNotification.probableCause) = SA_NTF_CORRUPT_DATA;
	 *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
	 myAlarmNotification.thresholdInformation->thresholdValueType = SA_NTF_VALUE_UINT32;
	 myAlarmNotification.thresholdInformation->thresholdValue.uint32Val = 600;
	 myAlarmNotification.thresholdInformation->thresholdHysteresis.uint32Val = 100;
	 myAlarmNotification.thresholdInformation->observedValue.uint32Val = 567;
	 myAlarmNotification.thresholdInformation->armTime = SA_TIME_UNKNOWN;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught*/
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;	 
	 poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);
	 if(ntfRecieved.alarmFilterHandle != ok_ids.length ||
		 ntfRecieved.attributeChangeFilterHandle != 0 ||
		 ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
		 ntfRecieved.securityAlarmFilterHandle != 0 ||
		 ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);

    safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/** 
 * AlarmNotification perceivedSeverities 
 */
void alarmNotificationFilterTest3(void)
{
	SaNtfAlarmNotificationFilterT          myAlarmFilter;
	subscriptionId = 1;
	
	rc = SA_AIS_OK;
	
	resetCounters();
	
	safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);
	
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
	myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_CLEARED;
	myAlarmFilter.perceivedSeverities[1] = SA_NTF_SEVERITY_CRITICAL;


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
	*(myAlarmNotification.probableCause) = SA_NTF_UNSPECIFIED_REASON;
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_CRITICAL;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	/* this notification should be caught*/
	ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;	 
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_INDETERMINATE;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_MAJOR;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_CLEARED;
	*(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
	*myAlarmNotification.trend = SA_NTF_TREND_NO_CHANGE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 
	/* this notification should be caught*/
	ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;	 
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_MINOR;
	*(myAlarmNotification.probableCause) = SA_NTF_POWER_PROBLEM;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_INDETERMINATE;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	*(myAlarmNotification.probableCause) = SA_NTF_BREACH_OF_CONFIDENTIALITY;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 
	
	 fillHeader(&myAlarmNotification.notificationHeader);
	 /* These 3 fields is filter items */
	 *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_CRITICAL;
	 *(myAlarmNotification.probableCause) = SA_NTF_CORRUPT_DATA;
	 *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
	 myAlarmNotification.thresholdInformation->thresholdValueType = SA_NTF_VALUE_UINT32;
	 myAlarmNotification.thresholdInformation->thresholdValue.uint32Val = 600;
	 myAlarmNotification.thresholdInformation->thresholdHysteresis.uint32Val = 100;
	 myAlarmNotification.thresholdInformation->observedValue.uint32Val = 567;
	 myAlarmNotification.thresholdInformation->armTime = SA_TIME_UNKNOWN;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught*/
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;	 
	 poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);
	 if(ntfRecieved.alarmFilterHandle != ok_ids.length ||
		 ntfRecieved.attributeChangeFilterHandle != 0 ||
		 ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
		 ntfRecieved.securityAlarmFilterHandle != 0 ||
		 ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);

    safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/** 
 * AlarmNotification trends 
 */
void alarmNotificationFilterTest4(void)
{
	SaNtfAlarmNotificationFilterT          myAlarmFilter;
	subscriptionId = 1;
	
	rc = SA_AIS_OK;
	
	resetCounters();
	
	safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);
	
	safassert(saNtfAlarmNotificationFilterAllocate(
		ntfHandle,
		&myAlarmFilter,
		0,
		0,
		0,
		0,
		0,
		0,
		2), SA_AIS_OK);
	/* Set perceived severities */
	myAlarmFilter.trends[0] = SA_NTF_TREND_MORE_SEVERE;
	myAlarmFilter.trends[1] = SA_NTF_TREND_LESS_SEVERE;


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
	*myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
	*(myAlarmNotification.probableCause) = SA_NTF_UNSPECIFIED_REASON;
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_CRITICAL;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	/* this notification should be caught*/
	ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;	 
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	*myAlarmNotification.trend = SA_NTF_TREND_NO_CHANGE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_INDETERMINATE;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	*myAlarmNotification.trend = SA_NTF_TREND_NO_CHANGE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_CLEARED;
	*(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
	*myAlarmNotification.trend = SA_NTF_TREND_LESS_SEVERE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 
	/* this notification should be caught*/
	ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;	 
	
	*(myAlarmNotification.probableCause) = SA_NTF_POWER_PROBLEM;
	*myAlarmNotification.trend = SA_NTF_TREND_NO_CHANGE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_INDETERMINATE;
	*(myAlarmNotification.probableCause) = SA_NTF_PRESSURE_UNACCEPTABLE;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	*(myAlarmNotification.probableCause) = SA_NTF_BREACH_OF_CONFIDENTIALITY;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK); 
	
	 fillHeader(&myAlarmNotification.notificationHeader);
	 /* These 3 fields is filter items */
	 *(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_CRITICAL;
	 *(myAlarmNotification.probableCause) = SA_NTF_CORRUPT_DATA;
	 *myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
	 myAlarmNotification.thresholdInformation->thresholdValueType = SA_NTF_VALUE_UINT32;
	 myAlarmNotification.thresholdInformation->thresholdValue.uint32Val = 600;
	 myAlarmNotification.thresholdInformation->thresholdHysteresis.uint32Val = 100;
	 myAlarmNotification.thresholdInformation->observedValue.uint32Val = 567;
	 myAlarmNotification.thresholdInformation->armTime = SA_TIME_UNKNOWN;
	 safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	 /* this notification should be caught*/
	 ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;	 
	 poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);
	 if(ntfRecieved.alarmFilterHandle != ok_ids.length ||
		 ntfRecieved.attributeChangeFilterHandle != 0 ||
		 ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
		 ntfRecieved.securityAlarmFilterHandle != 0 ||
		 ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);

    safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/** 
 * ObjectCreateDeleteNotificationFilter sourceIndicators 
 */
void objectCreateDeleteNotificationFilterTest(void)
{
    SaNtfObjectCreateDeleteNotificationFilterT myFilter;

    subscriptionId = 2;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfObjectCreateDeleteNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		1, 1, 1, 1, 2)) == SA_AIS_OK) {
		 myFilter.sourceIndicators[0] = SA_NTF_OBJECT_OPERATION;
		 myFilter.sourceIndicators[1] = SA_NTF_MANAGEMENT_OPERATION;
		 fillFilterHeader(&myFilter.notificationFilterHeader);
		 myFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_CREATION;		 
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

    	createObjectCreateDeleteNotification(ntfHandle,	&myObjCrDelNotification);
		*(myObjCrDelNotification.sourceIndicator) = SA_NTF_OBJECT_OPERATION;				
		safassert(saNtfNotificationSend(myObjCrDelNotification.notificationHandle), SA_AIS_OK);
		/* this notification should be caught*/
		ok_ids.ids[ok_ids.length++] = *myObjCrDelNotification.notificationHeader.notificationId;	   
		
		*(myObjCrDelNotification.sourceIndicator) = SA_NTF_UNKNOWN_OPERATION;
		safassert(saNtfNotificationSend(myObjCrDelNotification.notificationHandle), SA_AIS_OK);
				
		*(myObjCrDelNotification.sourceIndicator) = SA_NTF_MANAGEMENT_OPERATION;				
    	safassert(saNtfNotificationSend(myObjCrDelNotification.notificationHandle), SA_AIS_OK);
		/* this notification should be caught*/
		ok_ids.ids[ok_ids.length++] = *myObjCrDelNotification.notificationHeader.notificationId;	   
		poll_until_received(ntfHandle, *myObjCrDelNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle != ok_ids.length||
    			ntfRecieved.securityAlarmFilterHandle != 0 ||
    			ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(myObjCrDelNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/** 
 * AttributeChangeNotificationFilter sourceIndicators 
 */
void attributeChangeNotificationFilterTest(void)
{
    SaNtfAttributeChangeNotificationFilterT myFilter;

    subscriptionId = 2;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfAttributeChangeNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		1, 1, 1, 1, 2)) == SA_AIS_OK) {
		 myFilter.sourceIndicators[0] = SA_NTF_OBJECT_OPERATION;
		 myFilter.sourceIndicators[1] = SA_NTF_MANAGEMENT_OPERATION;
		 fillFilterHeader(&myFilter.notificationFilterHeader);
		 myFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_ATTRIBUTE_CHANGED;		 
    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle =
			myFilter.notificationFilterHandle;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    	myNotificationFilterHandles.stateChangeFilterHandle = 0;

    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
    			subscriptionId),
    			SA_AIS_OK);

    	createAttributeChangeNotification(ntfHandle,	&myAttrChangeNotification);
		*(myAttrChangeNotification.sourceIndicator) = SA_NTF_OBJECT_OPERATION;				
		safassert(saNtfNotificationSend(myAttrChangeNotification.notificationHandle), SA_AIS_OK);
		/* this notification should be caught*/
		ok_ids.ids[ok_ids.length++] = *myAttrChangeNotification.notificationHeader.notificationId;	   
		
		*(myAttrChangeNotification.sourceIndicator) = SA_NTF_UNKNOWN_OPERATION;
		safassert(saNtfNotificationSend(myAttrChangeNotification.notificationHandle), SA_AIS_OK);
				
		*(myAttrChangeNotification.sourceIndicator) = SA_NTF_MANAGEMENT_OPERATION;				
    	safassert(saNtfNotificationSend(myAttrChangeNotification.notificationHandle), SA_AIS_OK);
		/* this notification should be caught*/
		ok_ids.ids[ok_ids.length++] = *myAttrChangeNotification.notificationHeader.notificationId;	   
		poll_until_received(ntfHandle, *myAttrChangeNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != ok_ids.length ||
    			ntfRecieved.objectCreateDeleteFilterHandle != 0||
    			ntfRecieved.securityAlarmFilterHandle != 0 ||
    			ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(myAttrChangeNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}
/**  
 * StateChangeNotificationFilter sourceIndicators
 */
void stateChangeNotificationFilterTest(void)
{
    SaNtfStateChangeNotificationFilterT myFilter;

    subscriptionId = 2;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfStateChangeNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		1, 1, 1, 1, 2, 0)) == SA_AIS_OK) {
		 myFilter.sourceIndicators[0] = SA_NTF_OBJECT_OPERATION;
		 myFilter.sourceIndicators[1] = SA_NTF_MANAGEMENT_OPERATION;
		 fillFilterHeader(&myFilter.notificationFilterHeader);
		 myFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_STATE_CHANGE;		 
    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    	myNotificationFilterHandles.stateChangeFilterHandle =
			myFilter.notificationFilterHandle;

    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
    			subscriptionId),
    			SA_AIS_OK);

    	createStateChangeNotification(ntfHandle,	&myStateChangeNotification);
		*(myStateChangeNotification.sourceIndicator) = SA_NTF_OBJECT_OPERATION;				
		safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);
		/* this notification should be caught*/
		ok_ids.ids[ok_ids.length++] = *myStateChangeNotification.notificationHeader.notificationId;	   
		
		*(myStateChangeNotification.sourceIndicator) = SA_NTF_UNKNOWN_OPERATION;
		safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);
				
		*(myStateChangeNotification.sourceIndicator) = SA_NTF_MANAGEMENT_OPERATION;				
    	safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);
		/* this notification should be caught*/
		ok_ids.ids[ok_ids.length++] = *myStateChangeNotification.notificationHeader.notificationId;	   
		poll_until_received(ntfHandle, *myStateChangeNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle != 0||
    			ntfRecieved.securityAlarmFilterHandle != 0 ||
    			ntfRecieved.stateChangeFilterHandle != ok_ids.length) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(myStateChangeNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}
/** 
 * StateChangeNotificationFilter stateIds 
 */
void stateChangeNotificationFilterTest2(void)
{
    SaNtfStateChangeNotificationFilterT myFilter;

    subscriptionId = 2;

    resetCounters();

    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfStateChangeNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		1, 1, 1, 1, 2, 2)) == SA_AIS_OK) {
		 myFilter.sourceIndicators[0] = SA_NTF_OBJECT_OPERATION;
		 myFilter.sourceIndicators[1] = SA_NTF_MANAGEMENT_OPERATION;
		 fillFilterHeader(&myFilter.notificationFilterHeader);
		 myFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_STATE_CHANGE;		 
/* only filtering on stateId is used when filtering, SaNtfStateChangeNotificationFilterT is changed in NTF-A.02.01*/
		 myFilter.changedStates[0].stateId = 44;
		 myFilter.changedStates[0].oldStatePresent = SA_FALSE;
/* 	 myFilter->changedStates[0].newState = 3;*/
/* 	 myFilter->changedStates[0].oldState = 5;*/
		 myFilter.changedStates[1].stateId = 87;
		 myFilter.changedStates[1].oldStatePresent = SA_FALSE;
/* 	 myFilter->changedStates[1].oldState = 78;*/
/* 	 myFilter->changedStates[1].newState = 79;*/

		 /* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
    	myNotificationFilterHandles.stateChangeFilterHandle =
			myFilter.notificationFilterHandle;

    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
    			subscriptionId),
    			SA_AIS_OK);

    	createStateChangeNotification(ntfHandle,	&myStateChangeNotification);
		*(myStateChangeNotification.sourceIndicator) = SA_NTF_OBJECT_OPERATION;		  

		myStateChangeNotification.changedStates[0].stateId = 44;				  
		safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);
		/* this notification should be caught*/
		ok_ids.ids[ok_ids.length++] = *myStateChangeNotification.notificationHeader.notificationId;	   
		
		*(myStateChangeNotification.sourceIndicator) = SA_NTF_OBJECT_OPERATION;
		myStateChangeNotification.changedStates[0].stateId = 4;				  
		safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);
				
		*(myStateChangeNotification.sourceIndicator) = SA_NTF_MANAGEMENT_OPERATION;				
		myStateChangeNotification.changedStates[0].stateId = 87;  
    	safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);
		/* this notification should be caught*/
		ok_ids.ids[ok_ids.length++] = *myStateChangeNotification.notificationHeader.notificationId;	   

		*(myStateChangeNotification.sourceIndicator) = SA_NTF_UNKNOWN_OPERATION;				
		myStateChangeNotification.changedStates[0].stateId = 87;				  
		myStateChangeNotification.changedStates[1].stateId = 44;				  
		safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);

		*(myStateChangeNotification.sourceIndicator) = SA_NTF_MANAGEMENT_OPERATION;				
		myStateChangeNotification.changedStates[0].stateId = 55;				  
		myStateChangeNotification.changedStates[1].stateId = 44;				  
		safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);
		/* this notification should be caught*/
		ok_ids.ids[ok_ids.length++] = *myStateChangeNotification.notificationHeader.notificationId;	   

		poll_until_received(ntfHandle, *myStateChangeNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle != 0||
    			ntfRecieved.securityAlarmFilterHandle != 0 ||
    			ntfRecieved.stateChangeFilterHandle != ok_ids.length) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(myStateChangeNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/** 
 * SecurityAlarmNotificationFilter probableCauses
 */
void securityAlarmNotificationFilterTest(void)
{
    SaNtfSecurityAlarmNotificationFilterT myFilter;

    subscriptionId = 5;

    resetCounters();
    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfSecurityAlarmNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		0, 0, 0, 0, 3,0,0,0,0)) == SA_AIS_OK) {

    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle =
    		myFilter.notificationFilterHandle;
    	myNotificationFilterHandles.stateChangeFilterHandle = 0;
		myFilter.probableCauses[0] = SA_NTF_ADAPTER_ERROR;
		myFilter.probableCauses[1] = SA_NTF_EQUIPMENT_MALFUNCTION;
		myFilter.probableCauses[2] = SA_NTF_VERSION_MISMATCH;
		
    	/* subscribe */
    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
    			subscriptionId),
    			SA_AIS_OK);

    	createSecurityAlarmNotification(ntfHandle, &mySecAlarmNotification);
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		
		*mySecAlarmNotification.probableCause = SA_NTF_VERSION_MISMATCH;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;
		
		*mySecAlarmNotification.probableCause = SA_NTF_RESPONSE_TIME_EXCESSIVE;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
				
		*mySecAlarmNotification.probableCause = SA_NTF_NON_REPUDIATION_FAILURE;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		
		
				
		*mySecAlarmNotification.probableCause = SA_NTF_EQUIPMENT_MALFUNCTION;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;

		*mySecAlarmNotification.probableCause = SA_NTF_STORAGE_CAPACITY_PROBLEM;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

		*mySecAlarmNotification.probableCause = SA_NTF_DENIAL_OF_SERVICE;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

		*mySecAlarmNotification.probableCause = SA_NTF_ADAPTER_ERROR;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;
		poll_until_received(ntfHandle, *mySecAlarmNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
    			ntfRecieved.securityAlarmFilterHandle != ok_ids.length ||
    			ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/** 
 * SecurityAlarmNotificationFilter severities
 */
void securityAlarmNotificationFilterTest2(void)
{
    SaNtfSecurityAlarmNotificationFilterT myFilter;

    subscriptionId = 5;

    resetCounters();
    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfSecurityAlarmNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		0, 0, 0, 0, 0,2,0,0,0)) == SA_AIS_OK) {

    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle =
    		myFilter.notificationFilterHandle;
    	myNotificationFilterHandles.stateChangeFilterHandle = 0;
		myFilter.severities[0] = SA_NTF_SEVERITY_WARNING;
		myFilter.severities[1] = SA_NTF_SEVERITY_CRITICAL; 
		
    	/* subscribe */
    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
    			subscriptionId),
    			SA_AIS_OK);

    	createSecurityAlarmNotification(ntfHandle, &mySecAlarmNotification);
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
				
		*mySecAlarmNotification.severity = SA_NTF_SEVERITY_MAJOR;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		
				
		*mySecAlarmNotification.severity = SA_NTF_SEVERITY_WARNING;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;

		*mySecAlarmNotification.severity = SA_NTF_SEVERITY_INDETERMINATE;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

		*mySecAlarmNotification.severity = SA_NTF_SEVERITY_MINOR;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		

		*mySecAlarmNotification.severity = SA_NTF_SEVERITY_CRITICAL;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;
		poll_until_received(ntfHandle, *mySecAlarmNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
    			ntfRecieved.securityAlarmFilterHandle != ok_ids.length ||
    			ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}
/** 
 * SecurityAlarmNotificationFilter securityAlarmDetectors
 */
void securityAlarmNotificationFilterTest3(void)
{
    SaNtfSecurityAlarmNotificationFilterT myFilter;

    subscriptionId = 5;

    resetCounters();
    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfSecurityAlarmNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		0, 0, 0, 0, 0,0,3,0,0)) == SA_AIS_OK) {

    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle =
    		myFilter.notificationFilterHandle;
    	myNotificationFilterHandles.stateChangeFilterHandle = 0;
		myFilter.securityAlarmDetectors[0].valueType = SA_NTF_VALUE_UINT32;
		myFilter.securityAlarmDetectors[0].value.uint32Val = 4008640511u; /* 0xeeeeffff */
		myFilter.securityAlarmDetectors[1].valueType = SA_NTF_VALUE_UINT64;
		myFilter.securityAlarmDetectors[1].value.uint64Val = 1311879807313575935llu; /* 0x1234bbbbeeeeffff */
		myFilter.securityAlarmDetectors[2].valueType = SA_NTF_VALUE_UINT8;
		myFilter.securityAlarmDetectors[2].value.uint32Val = 248; 
		
    	/* subscribe */
    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles, subscriptionId), SA_AIS_OK);

    	createSecurityAlarmNotification(ntfHandle, &mySecAlarmNotification);
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
				
		mySecAlarmNotification.securityAlarmDetector->valueType = SA_NTF_VALUE_UINT32;		
		mySecAlarmNotification.securityAlarmDetector->value.int32Val = 124134134;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		
				
		mySecAlarmNotification.securityAlarmDetector->valueType = SA_NTF_VALUE_UINT32;		
		mySecAlarmNotification.securityAlarmDetector->value.int32Val = 4008640511u;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;

		mySecAlarmNotification.securityAlarmDetector->valueType = SA_NTF_VALUE_UINT64;		
		mySecAlarmNotification.securityAlarmDetector->value.uint64Val = 1311879807313575936llu;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

		mySecAlarmNotification.securityAlarmDetector->valueType = SA_NTF_VALUE_UINT8;		
		mySecAlarmNotification.securityAlarmDetector->value.int32Val = 148;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		

		mySecAlarmNotification.securityAlarmDetector->valueType = SA_NTF_VALUE_UINT8;		
		mySecAlarmNotification.securityAlarmDetector->value.int32Val = 248;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;

		mySecAlarmNotification.securityAlarmDetector->valueType = SA_NTF_VALUE_UINT64;		
		mySecAlarmNotification.securityAlarmDetector->value.uint64Val = 1311879807313575935llu;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;
		poll_until_received(ntfHandle, *mySecAlarmNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
    			ntfRecieved.securityAlarmFilterHandle != ok_ids.length ||
    			ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/** 
 * SecurityAlarmNotificationFilter serviceUsers
 */
void securityAlarmNotificationFilterTest4(void)
{
    SaNtfSecurityAlarmNotificationFilterT myFilter;

    subscriptionId = 5;

    resetCounters();
    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfSecurityAlarmNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		0, 0, 0, 0, 0,0,0,3,0)) == SA_AIS_OK) {

    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle =
    		myFilter.notificationFilterHandle;
    	myNotificationFilterHandles.stateChangeFilterHandle = 0;
		myFilter.serviceUsers[0].valueType = SA_NTF_VALUE_UINT32;
		myFilter.serviceUsers[0].value.uint32Val = 4008640511u; /* 0xeeeeffff */
		myFilter.serviceUsers[1].valueType = SA_NTF_VALUE_UINT64;
		myFilter.serviceUsers[1].value.uint64Val = 1311879807313575935llu; /* 0x1234bbbbeeeeffff */
		myFilter.serviceUsers[2].valueType = SA_NTF_VALUE_UINT8;
		myFilter.serviceUsers[2].value.uint32Val = 248; 
		
    	/* subscribe */
    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles, subscriptionId), SA_AIS_OK);

    	createSecurityAlarmNotification(ntfHandle, &mySecAlarmNotification);
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
				
		mySecAlarmNotification.serviceUser->valueType = SA_NTF_VALUE_UINT32;		
		mySecAlarmNotification.serviceUser->value.int32Val = 124134134;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		
				
		mySecAlarmNotification.serviceUser->valueType = SA_NTF_VALUE_UINT32;		
		mySecAlarmNotification.serviceUser->value.int32Val = 4008640511u;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;

		mySecAlarmNotification.serviceUser->valueType = SA_NTF_VALUE_UINT64;		
		mySecAlarmNotification.serviceUser->value.uint64Val = 1311879807313575936llu;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

		mySecAlarmNotification.serviceUser->valueType = SA_NTF_VALUE_UINT8;		
		mySecAlarmNotification.serviceUser->value.int32Val = 148;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		

		mySecAlarmNotification.serviceUser->valueType = SA_NTF_VALUE_UINT8;		
		mySecAlarmNotification.serviceUser->value.int32Val = 248;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;

		mySecAlarmNotification.serviceUser->valueType = SA_NTF_VALUE_UINT64;		
		mySecAlarmNotification.serviceUser->value.uint64Val = 1311879807313575935llu;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;
		poll_until_received(ntfHandle, *mySecAlarmNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
    			ntfRecieved.securityAlarmFilterHandle != ok_ids.length ||
    			ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/** 
 * SecurityAlarmNotificationFilter serviceProviders
 */
void securityAlarmNotificationFilterTest5(void)
{
    SaNtfSecurityAlarmNotificationFilterT myFilter;

    subscriptionId = 5;

    resetCounters();
    safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
    safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

    if((rc = saNtfSecurityAlarmNotificationFilterAllocate(
    		ntfHandle,
    		&myFilter,
    		0, 0, 0, 0, 0,0,0,0,3)) == SA_AIS_OK) {

    	/* Initialize filter handles */
    	myNotificationFilterHandles.alarmFilterHandle = 0;
    	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
    	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
    	myNotificationFilterHandles.securityAlarmFilterHandle =
    		myFilter.notificationFilterHandle;
    	myNotificationFilterHandles.stateChangeFilterHandle = 0;
		myFilter.serviceProviders[0].valueType = SA_NTF_VALUE_UINT32;
		myFilter.serviceProviders[0].value.uint32Val = 4008640511u; /* 0xeeeeffff */
		myFilter.serviceProviders[1].valueType = SA_NTF_VALUE_UINT64;
		myFilter.serviceProviders[1].value.uint64Val = 1311879807313575935llu; /* 0x1234bbbbeeeeffff */
		myFilter.serviceProviders[2].valueType = SA_NTF_VALUE_UINT8;
		myFilter.serviceProviders[2].value.uint32Val = 248; 
		
    	/* subscribe */
    	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles, subscriptionId), SA_AIS_OK);

    	createSecurityAlarmNotification(ntfHandle, &mySecAlarmNotification);
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
				
		mySecAlarmNotification.serviceProvider->valueType = SA_NTF_VALUE_UINT32;		
		mySecAlarmNotification.serviceProvider->value.int32Val = 124134134;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		
				
		mySecAlarmNotification.serviceProvider->valueType = SA_NTF_VALUE_UINT32;		
		mySecAlarmNotification.serviceProvider->value.int32Val = 4008640511u;		
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;

		mySecAlarmNotification.serviceProvider->valueType = SA_NTF_VALUE_UINT64;		
		mySecAlarmNotification.serviceProvider->value.uint64Val = 1311879807313575936llu;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

		mySecAlarmNotification.serviceProvider->valueType = SA_NTF_VALUE_UINT8;		
		mySecAlarmNotification.serviceProvider->value.int32Val = 148;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		

		mySecAlarmNotification.serviceProvider->valueType = SA_NTF_VALUE_UINT8;		
		mySecAlarmNotification.serviceProvider->value.int32Val = 248;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);		
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;

		mySecAlarmNotification.serviceProvider->valueType = SA_NTF_VALUE_UINT64;		
		mySecAlarmNotification.serviceProvider->value.uint64Val = 1311879807313575935llu;
		safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
		ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;
		poll_until_received(ntfHandle, *mySecAlarmNotification.notificationHeader.notificationId);
    	if(ntfRecieved.alarmFilterHandle != 0 ||
    			ntfRecieved.attributeChangeFilterHandle != 0 ||
    			ntfRecieved.objectCreateDeleteFilterHandle !=0 ||
    			ntfRecieved.securityAlarmFilterHandle != ok_ids.length ||
    			ntfRecieved.stateChangeFilterHandle != 0) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);

    	safassert(saNtfNotificationFree(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
    	safassert(saNtfNotificationFilterFree(myFilter.notificationFilterHandle), SA_AIS_OK);
    }
	 
	 safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

/**
 * Test all types of filters. 
 */
void combinedNotificationFilterTest(void)
{
	rc = SA_AIS_OK;
	SaNtfNotificationHeaderT *head = NULL;
	saNotificationFilterAllocationParamsT  myNotificationFilterAllocationParams;
	saNotificationAllocationParamsT myNotificationAllocationParams;
   saNotificationParamsT myNotificationParams;

	
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
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);
	
	if((rc =saNtfAlarmNotificationFilterAllocate(
		ntfHandle,
		&myAlarmFilter,
		1,
		1,
		1,
		1,
		1,
		1,
		1)) == SA_AIS_OK){
		fillFilterHeader(&myAlarmFilter.notificationFilterHeader);
		myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_WARNING;
		myAlarmFilter.probableCauses[0] = SA_NTF_BANDWIDTH_REDUCED;
		myAlarmFilter.trends[0] = SA_NTF_TREND_MORE_SEVERE;
		
		if((rc = saNtfObjectCreateDeleteNotificationFilterAllocate(
			ntfHandle,
			&myObjCrDeFilter,
			1,
			1,
			1,
			1,
			1)) == SA_AIS_OK) {
			fillFilterHeader(&myObjCrDeFilter.notificationFilterHeader);
			myObjCrDeFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_CREATION;
			myObjCrDeFilter.sourceIndicators[0] = SA_NTF_MANAGEMENT_OPERATION;
			
			
			if((rc = saNtfAttributeChangeNotificationFilterAllocate(
				ntfHandle,
				&myAttrChangeFilter,
				1, 1, 1, 1, 1)) == SA_AIS_OK) {
				fillFilterHeader(&myAttrChangeFilter.notificationFilterHeader);
				myAttrChangeFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_ATTRIBUTE_CHANGED;
				myAttrChangeFilter.sourceIndicators[0] = SA_NTF_OBJECT_OPERATION;
				  
				if((rc = saNtfSecurityAlarmNotificationFilterAllocate(
					ntfHandle, &mySecAlarmFilter, 1, 1, 1, 1, 1, 1, 1, 1,1)) == SA_AIS_OK) {
					fillFilterHeader(&mySecAlarmFilter.notificationFilterHeader);
					mySecAlarmFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_OPERATION_VIOLATION;
					mySecAlarmFilter.serviceUsers[0].valueType = SA_NTF_VALUE_INT32;
					mySecAlarmFilter.serviceUsers[0].value.int32Val = 468;
					mySecAlarmFilter.serviceProviders[0].valueType = SA_NTF_VALUE_INT32;
					mySecAlarmFilter.serviceProviders[0].value.int32Val = 789;
					mySecAlarmFilter.securityAlarmDetectors[0].valueType = SA_NTF_VALUE_UINT64;
					mySecAlarmFilter.securityAlarmDetectors[0].value.uint64Val = 123412341234llu;
					mySecAlarmFilter.severities[0] = SA_NTF_SEVERITY_MAJOR;
					mySecAlarmFilter.probableCauses[0] = SA_NTF_AUTHENTICATION_FAILURE;
					
					if((rc = saNtfStateChangeNotificationFilterAllocate(
						  ntfHandle,
						  &myStateChangeFilter,
						  1, 1, 1, 1, 1, 1)) == SA_AIS_OK) {
						myStateChangeFilter.sourceIndicators[0] = SA_NTF_OBJECT_OPERATION;
						fillFilterHeader(&myStateChangeFilter.notificationFilterHeader);
						myStateChangeFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_STATE_CHANGE;		 
						/* only filtering on stateId is used when filtering, SaNtfStateChangeNotificationFilterT is changed in NTF-A.02.01*/
						myStateChangeFilter.changedStates[0].stateId = 77;
						myStateChangeFilter.changedStates[0].oldStatePresent = SA_FALSE;
						/* 	 myStateChangeFilter->changedStates[0].newState = 3;*/
						/* 	 myStateChangeFilter->changedStates[0].oldState = 5;*/

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
						/* === Alarms  send 3 */
						/* should be caught */
						createAlarmNotification(ntfHandle, &myAlarmNotification);  
    					safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
						ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;
						/* header changed 1 item wrong*/
						*myAlarmNotification.notificationHeader.eventType = SA_NTF_ALARM_EQUIPMENT;
						safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
						/* alarm part changed 1 items wrong */
						*myAlarmNotification.notificationHeader.eventType = SA_NTF_ALARM_COMMUNICATION;
						*myAlarmNotification.probableCause = SA_NTF_CORRUPT_DATA;
						safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

						/* === Oject create delete send 5  */
						/* should be caught */
    					createObjectCreateDeleteNotification(ntfHandle,	&myObjCrDelNotification);
						*myObjCrDelNotification.sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
						*myObjCrDelNotification.notificationHeader.eventType = SA_NTF_OBJECT_CREATION;
    					safassert(saNtfNotificationSend(myObjCrDelNotification.notificationHandle), SA_AIS_OK);
						ok_ids.ids[ok_ids.length++] = *myObjCrDelNotification.notificationHeader.notificationId;

						*myObjCrDelNotification.sourceIndicator = SA_NTF_UNKNOWN_OPERATION;
						*myObjCrDelNotification.notificationHeader.eventType = SA_NTF_OBJECT_CREATION;
						safassert(saNtfNotificationSend(myObjCrDelNotification.notificationHandle), SA_AIS_OK);
						
						*myObjCrDelNotification.sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
						myObjCrDelNotification.notificationHeader.notificationClassId->minorId =10;
						safassert(saNtfNotificationSend(myObjCrDelNotification.notificationHandle), SA_AIS_OK);
						
						*myObjCrDelNotification.sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
						myObjCrDelNotification.notificationHeader.notificationClassId->minorId =12;
						myObjCrDelNotification.notificationHeader.notificationClassId->majorId = 88;
						safassert(saNtfNotificationSend(myObjCrDelNotification.notificationHandle), SA_AIS_OK);

						*myObjCrDelNotification.sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
						myObjCrDelNotification.notificationHeader.notificationClassId->vendorId =195;
						myObjCrDelNotification.notificationHeader.notificationClassId->majorId = 92;
						safassert(saNtfNotificationSend(myObjCrDelNotification.notificationHandle), SA_AIS_OK);
												
						/* === Attribute change send   */
						/* to be caught */
    					createAttributeChangeNotification(ntfHandle, &myAttrChangeNotification);
    					safassert(saNtfNotificationSend(myAttrChangeNotification.notificationHandle), SA_AIS_OK);
						ok_ids.ids[ok_ids.length++] = *myAttrChangeNotification.notificationHeader.notificationId;
						
						*myAttrChangeNotification.sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
						safassert(saNtfNotificationSend(myAttrChangeNotification.notificationHandle), SA_AIS_OK);

						*myAttrChangeNotification.sourceIndicator = SA_NTF_OBJECT_OPERATION;
						head = &myAttrChangeNotification.notificationHeader;
						head->notificationObject->length = strlen(NOTIF_OBJECT_TEST);
						(void)memcpy(head->notificationObject->value, NOTIF_OBJECT_TEST, head->notificationObject->length);  
						safassert(saNtfNotificationSend(myAttrChangeNotification.notificationHandle), SA_AIS_OK);
						
						/* === State change send   */
    					createStateChangeNotification(ntfHandle, &myStateChangeNotification);
    					safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);
						ok_ids.ids[ok_ids.length++] = *myStateChangeNotification.notificationHeader.notificationId;

						*myStateChangeNotification.sourceIndicator = SA_NTF_UNKNOWN_OPERATION;
						safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);

						*myStateChangeNotification.sourceIndicator = SA_NTF_OBJECT_OPERATION;
						myStateChangeNotification.changedStates[1].stateId = 67;
						safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);

						head = &myStateChangeNotification.notificationHeader;
						head->notifyingObject->length = strlen(NOTIFYING_OBJECT_TEST);
						(void)memcpy(head->notifyingObject->value, NOTIFYING_OBJECT_TEST, head->notifyingObject->length);   						
						safassert(saNtfNotificationSend(myStateChangeNotification.notificationHandle), SA_AIS_OK);
						
						/* === Security alarm  send   */
    					createSecurityAlarmNotification(ntfHandle, &mySecAlarmNotification);

						*mySecAlarmNotification.probableCause = SA_NTF_COMMUNICATIONS_SUBSYSTEM_FAILURE;
						safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
						
						*mySecAlarmNotification.probableCause =SA_NTF_AUTHENTICATION_FAILURE;
						*mySecAlarmNotification.severity = SA_NTF_SEVERITY_CRITICAL;
						safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
						
						*mySecAlarmNotification.severity = SA_NTF_SEVERITY_MAJOR;
						mySecAlarmNotification.serviceUser->value.int32Val = 400;
						safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

						mySecAlarmNotification.serviceUser->value.int32Val = 468;
						mySecAlarmNotification.serviceProvider->valueType = SA_NTF_VALUE_UINT32;
						safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
												
						mySecAlarmNotification.serviceProvider->valueType = SA_NTF_VALUE_INT32;
						mySecAlarmNotification.securityAlarmDetector->value.uint64Val = 223412341234llu;
						safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
						
						mySecAlarmNotification.securityAlarmDetector->value.uint64Val = 123412341234llu;
						safassert(saNtfNotificationSend(mySecAlarmNotification.notificationHandle), SA_AIS_OK);
						ok_ids.ids[ok_ids.length++] = *mySecAlarmNotification.notificationHeader.notificationId;
    					/* TODO: add MiscellaneousNotification */
						poll_until_received(ntfHandle, *mySecAlarmNotification.notificationHeader.notificationId);						
						if(ntfRecieved.alarmFilterHandle != 1 ||
							ntfRecieved.attributeChangeFilterHandle != 1 ||
							ntfRecieved.objectCreateDeleteFilterHandle !=1 ||
							ntfRecieved.securityAlarmFilterHandle != 1 ||
							ntfRecieved.stateChangeFilterHandle != 1) safassert(SA_AIS_ERR_BAD_FLAGS, SA_AIS_OK);
						
						safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle) , SA_AIS_OK);
    					safassert(saNtfNotificationFree(myObjCrDelNotification.notificationHandle), SA_AIS_OK);
    					safassert(saNtfNotificationFree(myAttrChangeNotification.notificationHandle), SA_AIS_OK);
						safassert(saNtfNotificationFree(myStateChangeNotification.notificationHandle), SA_AIS_OK);
    					safassert(saNtfNotificationFree(mySecAlarmNotification.notificationHandle), SA_AIS_OK);

						/* TODO: add MiscellaneousNotification free */
						
						safassert(saNtfNotificationUnsubscribe(subscriptionId),SA_AIS_OK);
						
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

    free(myNotificationParams.additionalText);
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	 if(rc == SA_AIS_OK)
		 rc = check_errors();
    test_validate(rc, SA_AIS_OK);
}

__attribute__ ((constructor)) static void notificationFilterVerification_constructor(void)
{
    test_suite_add(31, "Filter Verification");
	 test_case_add(31, headerTest, "HeaderFilter all set");
	 test_case_add(31, headerTest2, "HeaderFilter eventType");
	 test_case_add(31, headerTest3, "HeaderFilter notificationObjects");
	 test_case_add(31, headerTest4, "HeaderFilter notifyingObject");
	 test_case_add(31, headerTest5, "HeaderFilter notificationClassIds"); 
	 test_case_add(31, alarmNotificationFilterTest, "AlarmNotificationFilter all set");
	 test_case_add(31, alarmNotificationFilterTest2, "AlarmNotification probableCause");
	 test_case_add(31, alarmNotificationFilterTest3, "AlarmNotification perceivedSeverities"); 
	 test_case_add(31, alarmNotificationFilterTest4, "AlarmNotification trends");
	 test_case_add(31, objectCreateDeleteNotificationFilterTest, "ObjectCreateDeleteNotificationFilter sourceIndicators");
	 test_case_add(31, attributeChangeNotificationFilterTest, "AttributeChangeNotificationFilter sourceIndicators");
	 test_case_add(31, stateChangeNotificationFilterTest, "StateChangeNotificationFilter sourceIndicators");
	 test_case_add(31, stateChangeNotificationFilterTest2, "StateChangeNotificationFilter stateIds");
	 test_case_add(31, securityAlarmNotificationFilterTest, "SecurityAlarmNotificationFilter probableCauses");
	 test_case_add(31, securityAlarmNotificationFilterTest2, "SecurityAlarmNotificationFilter severities");
	 test_case_add(31, securityAlarmNotificationFilterTest3, "SecurityAlarmNotificationFilter securityAlarmDetectors");
	 test_case_add(31, securityAlarmNotificationFilterTest4, "SecurityAlarmNotificationFilter serviceUsers");
	 test_case_add(31, securityAlarmNotificationFilterTest5, "SecurityAlarmNotificationFilter serviceProviders");
	 test_case_add(31, combinedNotificationFilterTest, "Combined filter test");
}
