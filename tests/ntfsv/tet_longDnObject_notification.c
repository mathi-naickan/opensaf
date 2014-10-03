/*	  -*- OpenSAF  -*-
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

 */
#include <utest.h>
#include <util.h>
#include "tet_ntf.h"
#include "tet_ntf_common.h"
//#include "util.h"
#define NOTIFYING_OBJECT_TEST "AVND"
#define NTF_REST_MAX_IDS 30
										
#define DEFAULT_EXT_NAME_LENGTH 300
#define DEFAULT_UNEXT_NAME_STRING "This is unextended SaNameT string (<256)"
struct not_idsT {
	int length;
	SaNtfIdentifierT ids[NTF_REST_MAX_IDS];
};

static struct not_idsT received_ids = {0,};
static struct not_idsT ok_ids = {0,};

static char default_ext_notification_object[DEFAULT_EXT_NAME_LENGTH];
static char default_ext_notifying_object[DEFAULT_EXT_NAME_LENGTH];
static char test_longdn_object_1[DEFAULT_EXT_NAME_LENGTH];
static char test_longdn_object_2[DEFAULT_EXT_NAME_LENGTH];
static char test_longdn_object_3[DEFAULT_EXT_NAME_LENGTH];

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

extern void saAisNameLend(SaConstStringT value, SaNameT* name);
extern SaConstStringT saAisNameBorrow(const SaNameT* name);

/**
 * Init default long dn objects
 */
static void init_ext_object()
{
	safassert(setenv("SA_ENABLE_EXTENDED_NAMES", "1", 1), 0);

	memset(&default_ext_notification_object, 'A', DEFAULT_EXT_NAME_LENGTH - 1);
	default_ext_notification_object[DEFAULT_EXT_NAME_LENGTH - 1] = '\0';

	memset(&default_ext_notifying_object, 'B', DEFAULT_EXT_NAME_LENGTH - 1);
	default_ext_notifying_object[DEFAULT_EXT_NAME_LENGTH - 1] = '\0';
	
	memset(&test_longdn_object_1, 'C', DEFAULT_EXT_NAME_LENGTH - 1);
	test_longdn_object_1[DEFAULT_EXT_NAME_LENGTH - 1] = '\0';

	memset(&test_longdn_object_2, 'D', DEFAULT_EXT_NAME_LENGTH - 1);
	test_longdn_object_2[DEFAULT_EXT_NAME_LENGTH - 1] = '\0';

	memset(&test_longdn_object_3, 'E', DEFAULT_EXT_NAME_LENGTH - 1);
	test_longdn_object_3[DEFAULT_EXT_NAME_LENGTH - 1] = '\0';
	
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
		printf("num of failed notifications: %d\n", errors);
	}
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
}

/**
 * Creates an ObjectCreateDeleteNotification with default values.
 *
 * @param ntfHandle
 * @param myObjCreDelNotification
 */
void extCreateObjectCreateDeleteNotification(SaNtfHandleT ntfHandle,
		SaNtfObjectCreateDeleteNotificationT *myObjCrDelNotification)
{
	SaNtfNotificationHeaderT *head;

	createObjectCreateDeleteNotification(ntfHandle, myObjCrDelNotification);
	head = &myObjCrDelNotification->notificationHeader;
	/* Overwrite with long dn object */
	saAisNameLend((SaConstStringT)&default_ext_notification_object, head->notificationObject);
	saAisNameLend((SaConstStringT)&default_ext_notifying_object, head->notifyingObject);
}

/**
 * Creates an AttributeChangeNotification with default values.
 *
 * @param ntfhandle
 * @param myAttrChangeNotification
 */
void extCreateAttributeChangeNotification(SaNtfHandleT ntfHandle,
		SaNtfAttributeChangeNotificationT *myAttrChangeNotification)
{
	SaNtfNotificationHeaderT *head;

	createAttributeChangeNotification(ntfHandle, myAttrChangeNotification);
	head = &myAttrChangeNotification->notificationHeader;
	/* Overwrite with long dn object */
	saAisNameLend((SaConstStringT)&default_ext_notification_object, head->notificationObject);
	saAisNameLend((SaConstStringT)&default_ext_notifying_object, head->notifyingObject);
}

/**
 * Create a StateChangeNotification with default values.
 *
 * @param ntfHandle
 * @param myStateChangeNotification
 */
void extCreateStateChangeNotification(SaNtfHandleT ntfHandle,
		SaNtfStateChangeNotificationT *myStateChangeNotification)
{
	SaNtfNotificationHeaderT *head;

	createStateChangeNotification(ntfHandle, myStateChangeNotification);
	head = &myStateChangeNotification->notificationHeader;
	/* Overwrite with long dn object */
	saAisNameLend((SaConstStringT)&default_ext_notification_object, head->notificationObject);
	saAisNameLend((SaConstStringT)&default_ext_notifying_object, head->notifyingObject);
}

/**
 * Create a SecurityAlarmNotification with default values.
 *
 * @param ntfHandle
 * @param mySecAlarmNotification
 */
void extCreateSecurityAlarmNotification(SaNtfHandleT ntfHandle,
		SaNtfSecurityAlarmNotificationT *mySecAlarmNotification)
{
	SaNtfNotificationHeaderT *head;

	createSecurityAlarmNotification(ntfHandle, mySecAlarmNotification);
	head = &mySecAlarmNotification->notificationHeader;
	/* Overwrite with long dn object */
	saAisNameLend((SaConstStringT)&default_ext_notification_object, head->notificationObject);
	saAisNameLend((SaConstStringT)&default_ext_notifying_object, head->notifyingObject);
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
	if (notificationHandle != 0)
		safassert(saNtfNotificationFree(notificationHandle), SA_AIS_OK);
}


static SaNtfCallbacksT ntfCbTest = {
	saNtfNotificationCallbackT,
	NULL
};

/**
 * Fill the filter header with long dn objects
 */
void extFillFilterHeader(SaNtfNotificationFilterHeaderT *head)
{
	fillFilterHeader(head);
	/* Overwrite the objects with long dn */
	saAisNameLend((SaConstStringT)&default_ext_notification_object, &head->notificationObjects[0]);
	saAisNameLend((SaConstStringT)&default_ext_notifying_object, &head->notifyingObjects[0]);
}

/**
 * Fill the header with long dn objects
 */
void extFillHeader(SaNtfNotificationHeaderT *head)
{
	fillHeader(head);
	/* Overwrite the objects with long dn */
	saAisNameLend((SaConstStringT)&default_ext_notification_object, head->notificationObject);
	saAisNameLend((SaConstStringT)&default_ext_notifying_object, head->notifyingObject);
}


/**
 * Fill header with extended SaNameT in additionalInfo
 */
 void extFillHeaderAddInfo(SaNtfNotificationHeaderT *head, SaNtfNotificationHandleT notHandle)
 {
 	int i;
	SaStringT dest_ptr;
	SaNameT name1, name2, name3, name4, name5;
	*(head->eventType) = SA_NTF_ALARM_COMMUNICATION;
	*(head->eventTime) = SA_TIME_UNKNOWN;

	saAisNameLend((SaConstStringT)&default_ext_notification_object, head->notificationObject);
	saAisNameLend((SaConstStringT)&default_ext_notifying_object, head->notifyingObject);
	
	head->notificationClassId->vendorId = ERICSSON_VENDOR_ID;
	head->notificationClassId->majorId = 92;
	head->notificationClassId->minorId = 12;

	/* set additional text */
	(void)strncpy(head->additionalText,
			DEFAULT_ADDITIONAL_TEXT,
			(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1));

	for (i = 0; i < head->numCorrelatedNotifications; i++)
		head->correlatedNotifications[i] = (SaNtfIdentifierT) (i + 400);

	// Fill first additionalInfo as extended SaNameT including NULL character
	head->additionalInfo[0].infoType = SA_NTF_VALUE_LDAP_NAME;
	head->additionalInfo[0].infoId = 1;
	saAisNameLend((SaConstStringT)&test_longdn_object_1, &name1);
	
	safassert(saNtfPtrValAllocate(notHandle,
				strlen(saAisNameBorrow(&name1)) + 3,
				(void**)&dest_ptr,
				&(head->additionalInfo[0].infoValue)), SA_AIS_OK);
				
	saAisNameLend(saAisNameBorrow(&name1), (SaNameT*) dest_ptr);
	
	// Fill sencond additionalInfo as extended SaNameT excluding NULL character 
	head->additionalInfo[1].infoType = SA_NTF_VALUE_LDAP_NAME;
	head->additionalInfo[1].infoId = 1;
	saAisNameLend((SaConstStringT)&test_longdn_object_2, &name2);
	
	safassert(saNtfPtrValAllocate(notHandle,
				strlen(saAisNameBorrow(&name2)) + 2,
				(void**)&dest_ptr,
				&(head->additionalInfo[1].infoValue)), SA_AIS_OK);
				
	saAisNameLend(saAisNameBorrow(&name2), (SaNameT*) dest_ptr);

	
	//Fill third additionalInfo as extended SaNameT as legacy code -> object is truncated
	head->additionalInfo[2].infoType = SA_NTF_VALUE_LDAP_NAME;
	head->additionalInfo[2].infoId = 1;
	saAisNameLend((SaConstStringT)&test_longdn_object_3, &name3);
	safassert(saNtfPtrValAllocate(notHandle,
				sizeof(name3) + 1,
				(void**)&dest_ptr,
				&(head->additionalInfo[2].infoValue)), SA_AIS_OK);
				
	memcpy(dest_ptr, &name3, sizeof(name3));
	
	// Fill fourth additionalInfo as unextended SaNameT as legacy code
	head->additionalInfo[3].infoType = SA_NTF_VALUE_LDAP_NAME;
	head->additionalInfo[3].infoId = 1;
	saAisNameLend(DEFAULT_UNEXT_NAME_STRING, &name4);
	
	safassert(saNtfPtrValAllocate(notHandle,
				sizeof(name4) + 1,
				(void**)&dest_ptr,
				&(head->additionalInfo[3].infoValue)), SA_AIS_OK);
				
	memcpy(dest_ptr, &name4, sizeof(name4));
	
	//Fill the fifth additionalInfo as unextended SaNameT as modern code
	head->additionalInfo[4].infoType = SA_NTF_VALUE_LDAP_NAME;
	head->additionalInfo[4].infoId = 1;
	saAisNameLend(DEFAULT_UNEXT_NAME_STRING, &name5);
	
	safassert(saNtfPtrValAllocate(notHandle,
				strlen(saAisNameBorrow(&name5)) + 3,
				(void**)&dest_ptr,
				&(head->additionalInfo[4].infoValue)), SA_AIS_OK);
				
	saAisNameLend(saAisNameBorrow(&name5), (SaNameT*) dest_ptr);
 }
 
 /**
  * Test AdditionalInfo with extended SaNameT type
  */
 void extAdditionalInfoTest(void)
 {
 	SaNtfAlarmNotificationFilterT		  myAlarmFilter;
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
	extFillFilterHeader(&myAlarmFilter.notificationFilterHeader);

	/* Initialize filter handles */
	myNotificationFilterHandles.alarmFilterHandle =
		myAlarmFilter.notificationFilterHandle;
	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
	myNotificationFilterHandles.stateChangeFilterHandle = 0;
	
	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
				subscriptionId), SA_AIS_OK);

	/* Create a notification and send it */
	safassert(saNtfAlarmNotificationAllocate(
			ntfHandle,
			&myAlarmNotification,
			0,
			(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
			5,
			0,
			0,
			0,
			SA_NTF_ALLOC_SYSTEM_LIMIT), SA_AIS_OK);

	head = &myAlarmNotification.notificationHeader;
	extFillHeaderAddInfo(head, myAlarmNotification.notificationHandle);
	/* These 3 fields is alarm filter items */
	*(myAlarmNotification.perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
	*(myAlarmNotification.probableCause) = SA_NTF_BANDWIDTH_REDUCED;
	*myAlarmNotification.trend = SA_NTF_TREND_MORE_SEVERE;
	
	myAlarmNotification.thresholdInformation->thresholdValueType = SA_NTF_VALUE_UINT32;
	myAlarmNotification.thresholdInformation->thresholdValue.uint32Val = 600;
	myAlarmNotification.thresholdInformation->thresholdHysteresis.uint32Val = 100;
	myAlarmNotification.thresholdInformation->observedValue.uint32Val = 567;
	myAlarmNotification.thresholdInformation->armTime = SA_TIME_UNKNOWN;
	
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	/* only this notification should be caught */
	ok_ids.ids[ok_ids.length++] = *myAlarmNotification.notificationHeader.notificationId;
	poll_until_received(ntfHandle, *myAlarmNotification.notificationHeader.notificationId);

	/* Resend notification with short dn object header */
	saAisNameLend(DEFAULT_NOTIFICATION_OBJECT, head->notificationObject);
	saAisNameLend(DEFAULT_NOTIFYING_OBJECT, head->notifyingObject);
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

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
 * Test all filter header fields
 */
void extFilterNotificationTest(void)
{
	SaNtfAlarmNotificationFilterT		  myAlarmFilter;
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
	extFillFilterHeader(&myAlarmFilter.notificationFilterHeader);

	/* Initialize filter handles */
	myNotificationFilterHandles.alarmFilterHandle =
		myAlarmFilter.notificationFilterHandle;
	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
	myNotificationFilterHandles.stateChangeFilterHandle = 0;
	
	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
				subscriptionId), SA_AIS_OK);

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
	extFillHeader(head);

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
	
	saAisNameLend((SaConstStringT)&test_longdn_object_1, head->notificationObject);
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	extFillHeader(head);
	saAisNameLend(NOTIFYING_OBJECT_TEST, head->notifyingObject);	
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	extFillHeader(head);
	head->notificationClassId->vendorId = 199;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);

	extFillHeader(head);
	head->notificationClassId->majorId = 89;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	extFillHeader(head);
	head->notificationClassId->minorId = 24;
	safassert(saNtfNotificationSend(myAlarmNotification.notificationHandle), SA_AIS_OK);
	
	extFillHeader(head);  	
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
 * Test Alarm notification with long dn objects
 */
void extAlarmNotificationTest(void)
{
	SaNtfAlarmNotificationFilterT		  myAlarmFilter;
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
				subscriptionId), SA_AIS_OK);

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
	extFillHeader(&myAlarmNotification.notificationHeader);
	
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
	
	extFillHeader(&myAlarmNotification.notificationHeader);
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
 * Test Object Create/Delete notification with long dn objects
 */
void extObjectCreateDeleteNotificationTest(void)
{
	SaNtfObjectCreateDeleteNotificationFilterT myFilter;

	subscriptionId = 2;

	resetCounters();

	safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

	safassert(saNtfObjectCreateDeleteNotificationFilterAllocate(
				ntfHandle,
				&myFilter,
				1,
				1,
				1,
				1,
				2), SA_AIS_OK);
	myFilter.sourceIndicators[0] = SA_NTF_OBJECT_OPERATION;
	myFilter.sourceIndicators[1] = SA_NTF_MANAGEMENT_OPERATION;
	extFillFilterHeader(&myFilter.notificationFilterHeader);
	myFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_CREATION;		
	/* Initialize filter handles */
	myNotificationFilterHandles.alarmFilterHandle = 0;
	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
	myNotificationFilterHandles.objectCreateDeleteFilterHandle =
		myFilter.notificationFilterHandle;
	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
	myNotificationFilterHandles.stateChangeFilterHandle = 0;
	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
				subscriptionId), SA_AIS_OK);
	extCreateObjectCreateDeleteNotification(ntfHandle,	&myObjCrDelNotification);
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

	safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
	safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	rc = check_errors();
	test_validate(rc, SA_AIS_OK);
}

/**
 * Test Attribute Change notification with long dn objects
 */
void extAttributeChangeNotificationTest(void)
{
	SaNtfAttributeChangeNotificationFilterT myFilter;

	subscriptionId = 2;

	resetCounters();

	safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

	safassert(saNtfAttributeChangeNotificationFilterAllocate(
				ntfHandle,
				&myFilter,
				1,
				1,
				1,
				1,
				2), SA_AIS_OK);
			
	myFilter.sourceIndicators[0] = SA_NTF_OBJECT_OPERATION;
	myFilter.sourceIndicators[1] = SA_NTF_MANAGEMENT_OPERATION;
	extFillFilterHeader(&myFilter.notificationFilterHeader);
	myFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_ATTRIBUTE_CHANGED;		
	/* Initialize filter handles */
	myNotificationFilterHandles.alarmFilterHandle = 0;
	myNotificationFilterHandles.attributeChangeFilterHandle =
		myFilter.notificationFilterHandle;
	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
	myNotificationFilterHandles.stateChangeFilterHandle = 0;
	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
				subscriptionId), SA_AIS_OK);
	extCreateAttributeChangeNotification(ntfHandle,	&myAttrChangeNotification);
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


	safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
	safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	rc = check_errors();
	test_validate(rc, SA_AIS_OK);
}

/**
 * Test State Change notification with long dn objects
 */
void extStateChangeNotificationTest(void)
{
	SaNtfStateChangeNotificationFilterT myFilter;

	subscriptionId = 2;

	resetCounters();

	safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

	safassert(saNtfStateChangeNotificationFilterAllocate(
				ntfHandle,
				&myFilter,
				1,
				1,
				1,
				1,
				2,
				0), SA_AIS_OK);
			
	myFilter.sourceIndicators[0] = SA_NTF_OBJECT_OPERATION;
	myFilter.sourceIndicators[1] = SA_NTF_MANAGEMENT_OPERATION;
	extFillFilterHeader(&myFilter.notificationFilterHeader);
	myFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_STATE_CHANGE;		
	/* Initialize filter handles */
	myNotificationFilterHandles.alarmFilterHandle = 0;
	myNotificationFilterHandles.attributeChangeFilterHandle = 0;
	myNotificationFilterHandles.objectCreateDeleteFilterHandle = 0;
	myNotificationFilterHandles.securityAlarmFilterHandle = 0;
	myNotificationFilterHandles.stateChangeFilterHandle = myFilter.notificationFilterHandle;
	safassert(saNtfNotificationSubscribe(&myNotificationFilterHandles,
				subscriptionId), SA_AIS_OK);

	extCreateStateChangeNotification(ntfHandle,	&myStateChangeNotification);
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
	
	safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	rc = check_errors();
	test_validate(rc, SA_AIS_OK);
}

/**
 * Test Security Alarm notification with long dn objects
 */
void extSecurityAlarmNotificationTest(void)
{
	SaNtfSecurityAlarmNotificationFilterT myFilter;

	subscriptionId = 5;

	resetCounters();
	safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion) , SA_AIS_OK);
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject) , SA_AIS_OK);

	safassert(saNtfSecurityAlarmNotificationFilterAllocate(
				ntfHandle,
				&myFilter,
				0,
				0,
				0,
				0,
				3,
				0,
				0,
				0,
				0), SA_AIS_OK);

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
				subscriptionId), SA_AIS_OK);
	extCreateSecurityAlarmNotification(ntfHandle, &mySecAlarmNotification);
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

	safassert(saNtfNotificationUnsubscribe(subscriptionId), SA_AIS_OK);		
	safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
	rc = check_errors();
	test_validate(rc, SA_AIS_OK);
}

__attribute__ ((constructor)) static void longDnObject_notification_constructor(void)
{
	init_ext_object();
	test_suite_add(36, "Long DNs Test");
	test_case_add(36, extAdditionalInfoTest
					, "Test additional info with extended SaNameT type");	
	test_case_add(36, extFilterNotificationTest
					, "Test filter with longDn objects");
	test_case_add(36, extAlarmNotificationTest
					, "Test Alarm notification with longDn objects");
	test_case_add(36, extObjectCreateDeleteNotificationTest
					, "Test Object Create/Delete notification with longDn objects");
	test_case_add(36, extAttributeChangeNotificationTest
					, "Test Attribute Change notification with longDn objects");
	test_case_add(36, extStateChangeNotificationTest
					, "Test State Change notification with longDn objects");
	test_case_add(36, extSecurityAlarmNotificationTest
					, "Test Security Alarm notification with longDn objects");					
}
