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
#include "osaf/apitest/utest.h"
#include "osaf/apitest/util.h"
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <poll.h>
#include <pthread.h>
#include "tet_ntf.h"
#include "tet_ntf_common.h"
#define NTF_REST_MAX_IDS 30
#define DEFAULT_UNEXT_NAME_STRING "This is unextended SaNameT string (<256)"
#define TST_TAG_ND "\nTAG_ND\n"	/* Tag for take SC nodes down */
#define TST_TAG_NU "\nTAG_NU\n"	/* Tag for start SC nodes */

struct not_idsT {
	int length;
	SaNtfIdentifierT ids[NTF_REST_MAX_IDS];
};

struct saNtfAPIName{
	eSaNtfAPI apiNo;
	const char* apiName;
};
static struct saNtfAPIName ntf_api_name_list[SANTF_API_MAX] = {
	{ SANTF_ALL, "all API" },
	{ SANTF_INITIALIZE, "saNtfInitialize"  },
	{ SANTF_SELECTION_OBJECT_GET, "saNtfSelectionObjectGet"  },
	{ SANTF_DISPATCH, "saNtfDispatch"  },
	{ SANTF_FINALIZE, "saNtfFinalize"  },
	{ SANTF_ALARM_NOTIFICATION_ALLOCATE, "saNtfAlarmNotificationAllocate"  },
	{ SANTF_STATECHANGE_NOTIFICATION_ALLOCATE, "saNtfStateChangeNotificationAllocate"  },
	{ SANTF_OBJECTCREATEDELETE_NOTIFICATION_ALLOCATE, "saNtfObjectCreateDeleteNotificationAllocate"  },
	{ SANTF_ATTRIBUTECHANGE_NOTIFICATION_ALLOCATE, "saNtfAttributeChangeNotificationAllocate"  },
	{ SANTF_NOTIFICATION_FREE, "saNtfNotificationFree"  },
	{ SANTF_NOTIFICATION_SEND, "saNtfNotificationSend"  },
	{ SANTF_NOTIFICATION_SUBSCRIBE, "saNtfNotificationSubscribe"  },
	{ SANTF_SECURITY_ALARM_NOTIFICATION_ALLOCATE, "saNtfSecurityAlarmNotificationAllocate"  },
	{ SANTF_PTRVAL_ALLOCATE, "saNtfPtrValAllocate"  },
	{ SANTF_ARRAYVAL_ALLOCATE, "saNtfArrayValAllocate"  },
	{ SANTF_LOCALIZED_MESSAGE_GET, "saNtfLocalizedMessageGet"  },
	{ SANTF_LOCALIZED_MESSAGE_FREE, "saNtfLocalizedMessageFree"  },
	{ SANTF_PTRVAL_GET, "saNtfPtrGet"  },
	{ SANTF_ARRAYVAL_GET, "saNtfArrayGet"  },
	{ SANTF_OBJECTCREATEDELETE_NOTIFICATION_FILTER_ALLOCATE, "saNtfObjectCreateDeleteNotificationFilterAllocate"  },
	{ SANTF_ATTRIBUTECHANGE_NOTIFICATION_FILTER_ALLOCATE, "saNtfAttributeChangeNotificationFilterAllocate"  },
	{ SANTF_STATECHANGE_NOTIFICATION_FILTER_ALLOCATE, "saNtfStateChangeNotificationFilterAllocate"  },
	{ SANTF_ALARM_NOTIFICATION_FILTER_ALLOCATE, "saNtfAlarmNotificationFilterAllocate"  },
	{ SANTF_SECURITYALARM_NOTIFICATION_FILTER_ALLOCATE, "saNtfSecurityAlarmNotificationFilterAllocate"  },
	{ SANTF_NOTIFICATION_FILTER_FREE, "saNtfNotificationFilterFree"  },
	{ SANTF_NOTIFICATION_UNSUBSCRIBE, "saNtfNotificationUnsubscribe"  },
	{ SANTF_NOTIFICATION_READ_INITIALIZE, "saNtfNotificationReadInitialize"  },
	{ SANTF_NOTIFICATION_READ_FINALIZE, "saNtfNotificationReadFinalize"  },
	{ SANTF_NOTIFICATION_READ_NEXT, "saNtfNotificationReadNext"  }
};

static struct not_idsT received_ok_ids = {0,};
static struct not_idsT sent_ok_ids = {0,};

static SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles;

/* Used to keep track of which ntf we got */
static SaNtfNotificationTypeFilterHandlesT ntfRecieved;

static int glob_errors;
static SaNtfSubscriptionIdT subscriptionId;
static SaNtfAlarmNotificationT myAlarmNotification;
static SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles;
static SaNtfObjectCreateDeleteNotificationT myObjCrDelNotification;
static SaNtfAttributeChangeNotificationT myAttrChangeNotification;
static SaNtfStateChangeNotificationT myStateChangeNotification;
static SaNtfSecurityAlarmNotificationT mySecAlarmNotification;

extern int gl_tag_mode;

bool gl_suspending = true;


static void sigusr2_handler(int sig)
{
	if (gl_suspending)
		gl_suspending = false;
}

/**
 * Store all recieved notificationIds
 */
static void ntf_id_store(SaNtfIdentifierT n_id)
{
	assert(NTF_REST_MAX_IDS > received_ok_ids.length);
	received_ok_ids.ids[received_ok_ids.length++] = n_id;
}

/**
 * Post process all recived notificationIds towards the ids that
 * are expected to be received.
 */
static SaAisErrorT check_errors()
{
	int i, j, found;
	int rcv_notif_counter;
	int errors = 0;
	SaAisErrorT rc = SA_AIS_OK;
	rcv_notif_counter = 0;
	for (i = 0; i < received_ok_ids.length; i++) {
		found = 0;
		for (j= 0; j< sent_ok_ids.length; j++) {
			if (received_ok_ids.ids[i] == sent_ok_ids.ids[j]){
				found = 1;
				rcv_notif_counter++;
				break;
			}
		}
		if(!found)
			errors++;
	}
	if (rcv_notif_counter == sent_ok_ids.length)
		errors = 0;
	glob_errors += errors;
	if(glob_errors)	{
		rc = SA_AIS_ERR_FAILED_OPERATION;
		fprintf_v(stdout, "num of failed notifications: %d\n", glob_errors);
	}
	return rc;
}

void saferror(SaAisErrorT rc, SaAisErrorT exp)
{
	if (rc != exp)
		glob_errors++;
}

static void resetCounters()
{
	glob_errors = 0;
	received_ok_ids.length = 0;
	sent_ok_ids.length = 0;
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

				fprintf_v(stdout, "\n Received notificationCallback: notifType:%d, notifId:%d", (int)notification->notificationType,
						(int)*notification->notification.objectCreateDeleteNotification.notificationHeader.notificationId);

			if(myNotificationFilterHandles.objectCreateDeleteFilterHandle == 0)
				glob_errors +=1;
			else
				ntf_id_store(*notification->notification.objectCreateDeleteNotification.notificationHeader.notificationId);
			break;

		case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
			notificationHandle = notification->notification.attributeChangeNotification.notificationHandle;
			ntfRecieved.attributeChangeFilterHandle += 1;

			fprintf_v(stdout, "\n Received notificationCallback: notifType:%d, notifId:%d", (int)notification->notificationType,
						(int)*notification->notification.attributeChangeNotification.notificationHeader.notificationId);

			if(myNotificationFilterHandles.attributeChangeFilterHandle == 0)
				glob_errors += 1;
			else
				 ntf_id_store(*notification->notification.attributeChangeNotification.notificationHeader.notificationId);
			break;

		case SA_NTF_TYPE_STATE_CHANGE:
			notificationHandle = notification->notification.stateChangeNotification.notificationHandle;
			ntfRecieved.stateChangeFilterHandle += 1;

			fprintf_v(stdout, "\n Received notificationCallback: notifType:%d, notifId:%d", (int)notification->notificationType,
						(int)*notification->notification.stateChangeNotification.notificationHeader.notificationId);

			if(myNotificationFilterHandles.stateChangeFilterHandle == 0)
				glob_errors += 1;
			else
				ntf_id_store(*notification->notification.stateChangeNotification.notificationHeader.notificationId);
			break;

		case SA_NTF_TYPE_ALARM:
			notificationHandle = notification->notification.alarmNotification.notificationHandle;
			ntfRecieved.alarmFilterHandle += 1;

			fprintf_v(stdout, "\n Received notificationCallback: notifType:%d, notifId:%d", (int)notification->notificationType,
						(int)*notification->notification.alarmNotification.notificationHeader.notificationId);

			if(myNotificationFilterHandles.alarmFilterHandle == 0) {
				glob_errors +=1;
			} else
				ntf_id_store(*notification->notification.alarmNotification.notificationHeader.notificationId);
			break;

		case SA_NTF_TYPE_SECURITY_ALARM:
			notificationHandle = notification->notification.securityAlarmNotification.notificationHandle;
			ntfRecieved.securityAlarmFilterHandle += 1;

			fprintf_v(stdout, "\n Received notificationCallback: notifType:%d, notifId:%d", (int)notification->notificationType,
						(int)*notification->notification.securityAlarmNotification.notificationHeader.notificationId);

			if(myNotificationFilterHandles.securityAlarmFilterHandle == 0)
				glob_errors += 1;
			else
				ntf_id_store(*notification->notification.securityAlarmNotification.notificationHeader.notificationId);
			break;

		default:
			glob_errors +=1;
			assert(0);
			break;
	}
	last_not_id = get_ntf_id(notification);
	if (notificationHandle != 0)
		saferror(saNtfNotificationFree(notificationHandle), SA_AIS_OK);
}

/*
 * Loop of reading key press, stop if 'n+Enter'. Sleep 1 between reading key
 */
static void *nonblk_io_getchar()
{
	while (gl_suspending) {
		int c = getchar();
		if (c == 'n')
			gl_suspending = false;
		else
			sleep(1);
	}
	return NULL;
}

/*
 * Wait for both controllers UP(wished_scs_state=1) or DOWN(wished_scs_state=2)
 * Once bring up/down controllers, tester press 'n' or send USR2 to ntftest to
 * continue the test
 */
void wait_controllers(int wished_scs_state)
{
	int i = 0;
	pthread_t thread_id;
	gl_suspending = true;
	/* print slogan */
	if (wished_scs_state == 1) {
		fprintf_p(stdout, "\nNext: manually START both SCs(in UML env. ./opensaf nodestart <1|2>)");
		if (gl_tag_mode == 1) {
			fprintf_t(stdout, TST_TAG_NU);
		}
	} else if (wished_scs_state == 2) {
		fprintf_p(stdout, "\nNext: manually STOP both SCs(in UML env. ./opensaf nodestop <1|2>)");
		if (gl_tag_mode == 1) {
			fprintf_t(stdout, TST_TAG_ND);
		}
	} else {
		fprintf_p(stderr, "wrong value wished_scs_state:%d\n", wished_scs_state);
		exit(EXIT_FAILURE);
	}

	/* start non-blocking io thread */
	if (pthread_create(&thread_id, NULL, nonblk_io_getchar, NULL) != 0) {
		fprintf_p(stderr, "%d, %s; pthread_create FAILED: %s\n",__LINE__,
			__FUNCTION__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	fprintf_p(stdout, "\nThen press 'n' and 'Enter' (or 'pkill -USR2 ntftest') to continue. Waiting ...\n");
	while (gl_suspending) {
		i++;
		i = i % 20;
		i > 0 ? fprintf_p(stdout, ".") : fprintf_p(stdout, "\n");
		fflush(stdout);
		sleep(1);
	}
}

static SaNtfCallbacksT ntfCbTest = {
	saNtfNotificationCallbackT,
	NULL
};

void fillCommonNotifHeader(SaNtfNotificationHeaderT *head) {
	int i;

	head->notificationObject->length =
		strlen(DEFAULT_NOTIFICATION_OBJECT);
	(void)memcpy(head->notificationObject->value,
			DEFAULT_NOTIFICATION_OBJECT,
			head->notificationObject->length);

	head->notifyingObject->length =
		strlen(DEFAULT_NOTIFYING_OBJECT);
	(void)memcpy(head->notifyingObject->value,
			DEFAULT_NOTIFYING_OBJECT,
			head->notifyingObject->length);
	head->notificationClassId->vendorId = ERICSSON_VENDOR_ID;
	head->notificationClassId->majorId = 92;
	head->notificationClassId->minorId = 12;

	/* set additional text */
    (void)strncpy(head->additionalText,
    		DEFAULT_ADDITIONAL_TEXT,
    		(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1));

    for (i = 0; i < head->numCorrelatedNotifications; i++)
    {
	head->correlatedNotifications[i] = (SaNtfIdentifierT) (i + 400);
    }

    for (i = 0; i < head->numAdditionalInfo; i++)
    {
	head->additionalInfo[i].infoId = i;
	head->additionalInfo[i].infoType = SA_NTF_VALUE_INT32;
	head->additionalInfo[i].infoValue.int32Val = 444 + i;
    }
}

void fillCommonFilterHeader(SaNtfNotificationFilterHeaderT *head)
{
	assert(head->numEventTypes == 1);
	assert(head->numNotificationClassIds == 1);
	assert(head->numNotificationObjects == 1);
	assert(head->numNotifyingObjects == 1);

	head->notificationObjects[0].length = strlen(DEFAULT_NOTIFICATION_OBJECT);
	memcpy(head->notificationObjects[0].value, DEFAULT_NOTIFICATION_OBJECT, head->notificationObjects[0].length);

	head->notifyingObjects[0].length = strlen(DEFAULT_NOTIFYING_OBJECT);
	memcpy(head->notifyingObjects[0].value, DEFAULT_NOTIFYING_OBJECT, head->notifyingObjects[0].length);

	head->notificationClassIds[0].vendorId = ERICSSON_VENDOR_ID;
	head->notificationClassIds[0].majorId = 92;
	head->notificationClassIds[0].minorId = 12;
}

SaAisErrorT scoutage_saNtfPtrValAllocate(SaNtfNotificationHeaderT *head, SaNtfNotificationHandleT notHandle)
{
 	int i;
	SaStringT dest_ptr;
	SaNameT name;
	SaAisErrorT ret;

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

	name.length = strlen(DEFAULT_UNEXT_NAME_STRING);
	(void)memcpy(name.value, DEFAULT_UNEXT_NAME_STRING,	name.length);

	ret = ntftest_saNtfPtrValAllocate(notHandle,
				sizeof(name) + 1,
				(void**)&dest_ptr,
				&(head->additionalInfo[0].infoValue));
	if (ret == SA_AIS_OK)
		memcpy(dest_ptr, &name, sizeof(name));

	return ret;
}

SaAisErrorT scoutage_saNtfAlarmNotificationAllocate(SaNtfHandleT ntf_handle,
									SaNtfAlarmNotificationT* alarmNotification)
{
	SaNtfNotificationHeaderT *head = &alarmNotification->notificationHeader;
	SaAisErrorT rc;
	rc = ntftest_saNtfAlarmNotificationAllocate(
			ntf_handle,
			alarmNotification,
			0,
			(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1),
			1,
			0,
			0,
			0,
			SA_NTF_ALLOC_SYSTEM_LIMIT);
	if (rc == SA_AIS_OK) {
		/* Fill alarm info */
		fillCommonNotifHeader(head);
		*(head->eventType) = SA_NTF_ALARM_COMMUNICATION;
		*(head->eventTime) = SA_TIME_UNKNOWN;
		*(alarmNotification->perceivedSeverity) = SA_NTF_SEVERITY_WARNING;
		*(alarmNotification->probableCause) = SA_NTF_BANDWIDTH_REDUCED;
		*(alarmNotification->trend) = SA_NTF_TREND_MORE_SEVERE;

		alarmNotification->thresholdInformation->thresholdValueType = SA_NTF_VALUE_UINT32;
		alarmNotification->thresholdInformation->thresholdValue.uint32Val = 600;
		alarmNotification->thresholdInformation->thresholdHysteresis.uint32Val = 100;
		alarmNotification->thresholdInformation->observedValue.uint32Val = 567;
		alarmNotification->thresholdInformation->armTime = SA_TIME_UNKNOWN;
	}

	return rc;
}

SaAisErrorT scoutage_saNtfSecurityAlarmNotificationAllocate(SaNtfHandleT ntf_handle,
								SaNtfSecurityAlarmNotificationT* secAlarm_notification)
{
	SaNtfNotificationHeaderT *head = &secAlarm_notification->notificationHeader;
	SaAisErrorT rc;
	rc = ntftest_saNtfSecurityAlarmNotificationAllocate(
			ntf_handle,
			secAlarm_notification,
			1,
			(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT)+1),
			1,
			SA_NTF_ALLOC_SYSTEM_LIMIT);
	if (rc == SA_AIS_OK) {
		/* Fill sec alarm info */
		fillCommonNotifHeader(head);
		*(head->eventType) = SA_NTF_OPERATION_VIOLATION;
		*(head->eventTime) = SA_TIME_UNKNOWN;
		*(secAlarm_notification->severity) = SA_NTF_SEVERITY_MAJOR;

		*(secAlarm_notification->probableCause)  = SA_NTF_AUTHENTICATION_FAILURE;

		secAlarm_notification->serviceUser->valueType = SA_NTF_VALUE_INT32;
		secAlarm_notification->serviceUser->value.int32Val = 468;

		secAlarm_notification->serviceProvider->valueType = SA_NTF_VALUE_INT32;
		secAlarm_notification->serviceProvider->value.int32Val = 789;

		secAlarm_notification->securityAlarmDetector->valueType = SA_NTF_VALUE_UINT64;
		secAlarm_notification->securityAlarmDetector->value.uint64Val = 123412341234llu;
	}

	return rc;
}

SaAisErrorT scoutage_saNtfStateChangeNotificationAllocate(SaNtfHandleT ntf_handle,
								SaNtfStateChangeNotificationT* stateChange_notification)
{
	SaNtfNotificationHeaderT *head = &stateChange_notification->notificationHeader;
	SaAisErrorT rc;
	rc = ntftest_saNtfStateChangeNotificationAllocate(
			ntf_handle,
			stateChange_notification,
			1,
			(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT)+1),
			1,
			2,
			SA_NTF_ALLOC_SYSTEM_LIMIT);
	if (rc == SA_AIS_OK) {
		/* Fill sec alarm info */
		fillCommonNotifHeader(head);
		*(head->eventType) = SA_NTF_OBJECT_STATE_CHANGE;
		*(head->eventTime) = SA_TIME_UNKNOWN;
		*(stateChange_notification->sourceIndicator) = SA_NTF_OBJECT_OPERATION;

		stateChange_notification->changedStates[0].stateId = 2;
		stateChange_notification->changedStates[0].oldStatePresent = SA_TRUE;
		stateChange_notification->changedStates[0].newState = 3;
		stateChange_notification->changedStates[0].oldState = 5;

		stateChange_notification->changedStates[1].stateId = 77;
		stateChange_notification->changedStates[1].oldStatePresent = SA_TRUE;
		stateChange_notification->changedStates[1].oldState = 78;
		stateChange_notification->changedStates[1].newState = 79;
	}

	return rc;
}

SaAisErrorT scoutage_saNtfObjectCreateDeleteNotificationAllocate(SaNtfHandleT ntf_handle,
								SaNtfObjectCreateDeleteNotificationT* objCreateDelete_notification)
{
	SaNtfNotificationHeaderT *head = &objCreateDelete_notification->notificationHeader;
	SaAisErrorT rc;
	rc = ntftest_saNtfObjectCreateDeleteNotificationAllocate(
			ntf_handle,
			objCreateDelete_notification,
			0,
			(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) +1),
			0,
			2,
			SA_NTF_ALLOC_SYSTEM_LIMIT);
	if (rc == SA_AIS_OK) {
		/* Set objectAttibutes */
		fillCommonNotifHeader(head);
		*(head->eventType) = SA_NTF_OBJECT_CREATION;
		*(head->eventTime) = SA_TIME_UNKNOWN;

		objCreateDelete_notification->objectAttributes[0].attributeId = 0;
		objCreateDelete_notification->objectAttributes[0].attributeType = SA_NTF_VALUE_INT32;
		objCreateDelete_notification->objectAttributes[0].attributeValue.int32Val = 1;
		objCreateDelete_notification->objectAttributes[1].attributeId = 1;
		objCreateDelete_notification->objectAttributes[1].attributeType = SA_NTF_VALUE_INT32;
		objCreateDelete_notification->objectAttributes[1].attributeValue.int32Val = 2;
	}

	return rc;
}

SaAisErrorT scoutage_saNtfAttributeChangeNotificationAllocate(SaNtfHandleT ntf_handle,
								SaNtfAttributeChangeNotificationT* attrChange_notification)
{
	SaNtfNotificationHeaderT *head = &attrChange_notification->notificationHeader;
	SaAisErrorT rc;
	rc = ntftest_saNtfAttributeChangeNotificationAllocate(
			ntf_handle,
			attrChange_notification,
			1,
			(SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT)+1),
			1,
			2,
			SA_NTF_ALLOC_SYSTEM_LIMIT);
	if (rc == SA_AIS_OK) {
		/* Set objectAttibutes */
		fillCommonNotifHeader(head);
		*(head->eventType) = SA_NTF_ATTRIBUTE_CHANGED;
		*(head->eventTime) = SA_TIME_UNKNOWN;
		attrChange_notification->changedAttributes[0].attributeId = 1;
		attrChange_notification->changedAttributes[0].attributeType = SA_NTF_VALUE_INT32;
		attrChange_notification->changedAttributes[0].newAttributeValue.int32Val = 32;
		attrChange_notification->changedAttributes[0].oldAttributePresent = SA_TRUE;
		attrChange_notification->changedAttributes[0].oldAttributeValue.int32Val = 33;

		attrChange_notification->changedAttributes[1].attributeId = 2;
		attrChange_notification->changedAttributes[1].attributeType = SA_NTF_VALUE_INT16;
		attrChange_notification->changedAttributes[1].newAttributeValue.int16Val = 4;
		attrChange_notification->changedAttributes[1].oldAttributePresent = SA_TRUE;
		attrChange_notification->changedAttributes[1].oldAttributeValue.int16Val = 44;
	}
	return rc;
}

SaAisErrorT scoutage_saNtfAlarmNotificationFilterAllocate(SaNtfHandleT ntf_handle,
									SaNtfAlarmNotificationFilterT* alarm_filter)
{
	SaAisErrorT rc = SA_AIS_OK;
	rc = ntftest_saNtfAlarmNotificationFilterAllocate(ntf_handle, alarm_filter,	1, 1, 1, 1, 0, 0, 0);
	if (rc == SA_AIS_OK) {
		fillCommonFilterHeader(&alarm_filter->notificationFilterHeader);
		alarm_filter->notificationFilterHeader.eventTypes[0] = SA_NTF_ALARM_COMMUNICATION;
	}
	return rc;
}

SaAisErrorT scoutage_saNtfSecurityAlarmNotificationFilterAllocate(SaNtfHandleT ntf_handle,
									SaNtfSecurityAlarmNotificationFilterT* secAlarm_filter)
{
	SaAisErrorT rc = SA_AIS_OK;
	rc = ntftest_saNtfSecurityAlarmNotificationFilterAllocate(ntf_handle, secAlarm_filter, 0, 0, 0, 0, 1, 0, 0, 0, 0);
	if (rc == SA_AIS_OK) {
		secAlarm_filter->probableCauses[0] = SA_NTF_AUTHENTICATION_FAILURE;
	}
	return rc;
}

SaAisErrorT scoutage_saNtfStateChangeNotificationFilterAllocate(SaNtfHandleT ntf_handle,
									SaNtfStateChangeNotificationFilterT* stateChange_filter)
{
	SaAisErrorT rc = SA_AIS_OK;
	rc = ntftest_saNtfStateChangeNotificationFilterAllocate(ntf_handle, stateChange_filter, 1, 1, 1, 1, 1, 0);
	if (rc == SA_AIS_OK) {
		fillCommonFilterHeader(&stateChange_filter->notificationFilterHeader);
		stateChange_filter->sourceIndicators[0] = SA_NTF_OBJECT_OPERATION;
		stateChange_filter->notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_STATE_CHANGE;
	}
	return rc;
}

SaAisErrorT scoutage_saNtfObjectCreateDeleteNotificationFilterAllocate(SaNtfHandleT ntf_handle,
									SaNtfObjectCreateDeleteNotificationFilterT* objectCreateDelete_filter)
{
	SaAisErrorT rc = SA_AIS_OK;
	rc = ntftest_saNtfObjectCreateDeleteNotificationFilterAllocate(ntf_handle, objectCreateDelete_filter, 1, 1, 1, 1, 0);
	if (rc == SA_AIS_OK) {
		fillCommonFilterHeader(&objectCreateDelete_filter->notificationFilterHeader);
		objectCreateDelete_filter->notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_CREATION;
	}
	return rc;
}

SaAisErrorT scoutage_saNtfAttributeChangeNotificationFilterAllocate(SaNtfHandleT ntf_handle,
									SaNtfAttributeChangeNotificationFilterT* attributeChange_filter)
{
	SaAisErrorT rc = SA_AIS_OK;
	rc = ntftest_saNtfAttributeChangeNotificationFilterAllocate(ntf_handle, attributeChange_filter, 1, 1, 1, 1, 0);
	if (rc == SA_AIS_OK) {
		fillCommonFilterHeader(&attributeChange_filter->notificationFilterHeader);
		attributeChange_filter->notificationFilterHeader.eventTypes[0] = SA_NTF_ATTRIBUTE_CHANGED;
	}
	return rc;
}

SaNtfIdentifierT send_alarmNotification(SaNtfHandleT ntf_handle,
									SaNtfAlarmNotificationT* alarmNotification)
{
	SaNtfIdentifierT notifId;
	saferror(scoutage_saNtfAlarmNotificationAllocate(ntf_handle, alarmNotification), SA_AIS_OK);
	saferror(ntftest_saNtfNotificationSend(alarmNotification->notificationHandle), SA_AIS_OK);
	notifId = *alarmNotification->notificationHeader.notificationId;
	saferror(ntftest_saNtfNotificationFree(alarmNotification->notificationHandle), SA_AIS_OK);
	return notifId;
}

SaNtfIdentifierT send_secAlarmNotification(SaNtfHandleT ntf_handle,
									SaNtfSecurityAlarmNotificationT* secAlarmNotification)
{
	SaNtfIdentifierT notifId;
	saferror(scoutage_saNtfSecurityAlarmNotificationAllocate(ntf_handle, secAlarmNotification), SA_AIS_OK);
	saferror(ntftest_saNtfNotificationSend(secAlarmNotification->notificationHandle), SA_AIS_OK);
	notifId = *secAlarmNotification->notificationHeader.notificationId;
	saferror(ntftest_saNtfNotificationFree(secAlarmNotification->notificationHandle), SA_AIS_OK);
	return notifId;
}

SaNtfIdentifierT send_stateChangeNotification(SaNtfHandleT ntf_handle,
									SaNtfStateChangeNotificationT* stateChangeNotification)
{
	SaNtfIdentifierT notifId;
	saferror(scoutage_saNtfStateChangeNotificationAllocate(ntf_handle, stateChangeNotification), SA_AIS_OK);
	saferror(ntftest_saNtfNotificationSend(stateChangeNotification->notificationHandle), SA_AIS_OK);
	notifId = *stateChangeNotification->notificationHeader.notificationId;
	saferror(ntftest_saNtfNotificationFree(stateChangeNotification->notificationHandle), SA_AIS_OK);
	return notifId;
}

SaNtfIdentifierT send_objectCreateDeleteNotification(SaNtfHandleT ntf_handle,
									SaNtfObjectCreateDeleteNotificationT* objectCreateDeleteNotification)
{
	SaNtfIdentifierT notifId;
	saferror(scoutage_saNtfObjectCreateDeleteNotificationAllocate(ntf_handle, objectCreateDeleteNotification), SA_AIS_OK);
	saferror(ntftest_saNtfNotificationSend(objectCreateDeleteNotification->notificationHandle), SA_AIS_OK);
	notifId = *objectCreateDeleteNotification->notificationHeader.notificationId;
	saferror(ntftest_saNtfNotificationFree(objectCreateDeleteNotification->notificationHandle), SA_AIS_OK);
	return notifId;
}

SaNtfIdentifierT send_attrChangeNotification(SaNtfHandleT ntf_handle,
									SaNtfAttributeChangeNotificationT* attrChange_notification)
{
	SaNtfIdentifierT notifId;
	saferror(scoutage_saNtfAttributeChangeNotificationAllocate(ntf_handle, attrChange_notification), SA_AIS_OK);
	saferror(ntftest_saNtfNotificationSend(attrChange_notification->notificationHandle), SA_AIS_OK);
	notifId = *attrChange_notification->notificationHeader.notificationId;
	saferror(ntftest_saNtfNotificationFree(attrChange_notification->notificationHandle), SA_AIS_OK);
	return notifId;
}

void producer_life_cycle(int test_api)
{
	rc = SA_AIS_OK;

	fprintf_v(stdout, "\nStart test API: %s", ntf_api_name_list[test_api].apiName);
	resetCounters();

	if (test_api == SANTF_INITIALIZE) wait_controllers(2);
	rc = ntftest_saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion);
	if (test_api == SANTF_INITIALIZE) {
		saferror(rc, SA_AIS_ERR_TRY_AGAIN);
		wait_controllers(1);
		rc = ntftest_saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion);
	}
	saferror(rc, SA_AIS_OK);

	/* Create alarm notification */
	rc = scoutage_saNtfAlarmNotificationAllocate(ntfHandle, &myAlarmNotification);
	saferror(rc, SA_AIS_OK);

	rc = scoutage_saNtfPtrValAllocate(&myAlarmNotification.notificationHeader, myAlarmNotification.notificationHandle);
	saferror(rc, SA_AIS_OK);

	/* Create security alarm notification */
	rc = scoutage_saNtfSecurityAlarmNotificationAllocate(ntfHandle,	&mySecAlarmNotification);
	saferror(rc, SA_AIS_OK);

	/* Create state change notification */
	rc = scoutage_saNtfStateChangeNotificationAllocate(ntfHandle, &myStateChangeNotification);
	saferror(rc, SA_AIS_OK);

	/* Create object create delete notification */
	rc = scoutage_saNtfObjectCreateDeleteNotificationAllocate(ntfHandle, &myObjCrDelNotification);
	saferror(rc, SA_AIS_OK);

	/* Create attribute change notification */
	rc = scoutage_saNtfAttributeChangeNotificationAllocate(ntfHandle, &myAttrChangeNotification);
	saferror(rc, SA_AIS_OK);

	/* Send alarm notification*/
	if (test_api == SANTF_NOTIFICATION_SEND) wait_controllers(2);
	rc = ntftest_saNtfNotificationSend(myAlarmNotification.notificationHandle);
	if (test_api == SANTF_NOTIFICATION_SEND) {
		saferror(rc, SA_AIS_ERR_TRY_AGAIN);
		wait_controllers(1);
		rc = ntftest_saNtfNotificationSend(myAlarmNotification.notificationHandle);
	}
	saferror(rc, SA_AIS_OK);

	if ((rc = ntftest_saNtfNotificationFree(myAlarmNotification.notificationHandle)) != SA_AIS_OK ||
		(rc = ntftest_saNtfNotificationFree(mySecAlarmNotification.notificationHandle)) != SA_AIS_OK ||
		(rc = ntftest_saNtfNotificationFree(myStateChangeNotification.notificationHandle)) != SA_AIS_OK ||
		(rc = ntftest_saNtfNotificationFree(myObjCrDelNotification.notificationHandle)) != SA_AIS_OK ||
		(rc = ntftest_saNtfNotificationFree(myAttrChangeNotification.notificationHandle)) != SA_AIS_OK);
	saferror(rc, SA_AIS_OK);

	if (test_api == SANTF_FINALIZE) wait_controllers(2);
	rc = ntftest_saNtfFinalize(ntfHandle);
	saferror(rc, SA_AIS_OK);
	if (test_api == SANTF_FINALIZE) wait_controllers(1);

	fprintf_v(stdout, "\nEnd test API: %s\n", ntf_api_name_list[test_api].apiName);
}

void subscriber_life_cycle(int test_api)
{
	rc = SA_AIS_OK;
	SaNtfAlarmNotificationFilterT myAlarmFilter;
	SaNtfSecurityAlarmNotificationFilterT mySecurityAlarmFilter;
	SaNtfStateChangeNotificationFilterT myStateChangeFilter;
	SaNtfObjectCreateDeleteNotificationFilterT myObjectCreateDeleteFilter;
	SaNtfAttributeChangeNotificationFilterT myAttributeChangeFilter;

	int ret;
	struct pollfd fds[1];

	fprintf_v(stdout, "\nStart test API: %s", ntf_api_name_list[test_api].apiName);
	resetCounters();

	if (test_api == SANTF_INITIALIZE) wait_controllers(2);
	rc = ntftest_saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion);
	if (test_api == SANTF_INITIALIZE) {
		saferror(rc, SA_AIS_ERR_TRY_AGAIN);
		wait_controllers(1);
		rc = ntftest_saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion);
	}
	saferror(rc, SA_AIS_OK);

	rc = ntftest_saNtfSelectionObjectGet(ntfHandle, &selectionObject);
	saferror(rc, SA_AIS_OK);

	rc = scoutage_saNtfAlarmNotificationFilterAllocate(ntfHandle, &myAlarmFilter);
	saferror(rc, SA_AIS_OK);

	rc = scoutage_saNtfSecurityAlarmNotificationFilterAllocate(ntfHandle, &mySecurityAlarmFilter);
	saferror(rc, SA_AIS_OK);

	rc = scoutage_saNtfStateChangeNotificationFilterAllocate(ntfHandle, &myStateChangeFilter);
	saferror(rc, SA_AIS_OK);

	rc = scoutage_saNtfObjectCreateDeleteNotificationFilterAllocate(ntfHandle, &myObjectCreateDeleteFilter);
	saferror(rc, SA_AIS_OK);

	rc = scoutage_saNtfAttributeChangeNotificationFilterAllocate(ntfHandle, &myAttributeChangeFilter);
	saferror(rc, SA_AIS_OK);

	myNotificationFilterHandles.alarmFilterHandle = myAlarmFilter.notificationFilterHandle;
	myNotificationFilterHandles.attributeChangeFilterHandle = myAttributeChangeFilter.notificationFilterHandle;
	myNotificationFilterHandles.objectCreateDeleteFilterHandle = myObjectCreateDeleteFilter.notificationFilterHandle;
	myNotificationFilterHandles.securityAlarmFilterHandle = mySecurityAlarmFilter.notificationFilterHandle;
	myNotificationFilterHandles.stateChangeFilterHandle = myStateChangeFilter.notificationFilterHandle;

	if (test_api == SANTF_NOTIFICATION_SUBSCRIBE) wait_controllers(2);
	rc = ntftest_saNtfNotificationSubscribe(&myNotificationFilterHandles, subscriptionId);
	if (test_api == SANTF_NOTIFICATION_SUBSCRIBE){
		saferror(rc, SA_AIS_ERR_TRY_AGAIN);
		wait_controllers(1);
		rc = ntftest_saNtfNotificationSubscribe(&myNotificationFilterHandles, subscriptionId);
	}
	saferror(rc, SA_AIS_OK);

	// Free 2 filters here to test whether all filters are recovered
	if ((rc = ntftest_saNtfNotificationFilterFree(myObjectCreateDeleteFilter.notificationFilterHandle)) != SA_AIS_OK ||
		(rc = ntftest_saNtfNotificationFilterFree(myAttributeChangeFilter.notificationFilterHandle)) != SA_AIS_OK);
	saferror(rc, SA_AIS_OK);

	sent_ok_ids.ids[sent_ok_ids.length++] = send_alarmNotification(ntfHandle, &myAlarmNotification);
	sent_ok_ids.ids[sent_ok_ids.length++] = send_secAlarmNotification(ntfHandle, &mySecAlarmNotification);
	sent_ok_ids.ids[sent_ok_ids.length++] = send_stateChangeNotification(ntfHandle, &myStateChangeNotification);
	sent_ok_ids.ids[sent_ok_ids.length++] = send_objectCreateDeleteNotification(ntfHandle, &myObjCrDelNotification);
	sent_ok_ids.ids[sent_ok_ids.length++] = send_attrChangeNotification(ntfHandle, &myAttrChangeNotification);

	fds[0].fd = (int) selectionObject;

	while (1) {
		fds[0].events = POLLIN;
		ret = poll(fds, 1, 10000);
		if (ret <= 0) {
			saferror(true, false);
		}
		if (test_api == SANTF_DISPATCH) wait_controllers(2);

		rc = ntftest_saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (test_api == SANTF_DISPATCH) {
			saferror(rc, SA_AIS_ERR_TRY_AGAIN);
			wait_controllers(1);
			test_api = SANTF_NOTIFICATION_SEND;

		} else if (test_api == SANTF_NOTIFICATION_SEND) {
			// Resend notification by another handle to check if saNtfDispatch performs recovery and
			// subscriber can receives notification
			SaNtfHandleT subNtfHandle;
			saferror(ntftest_saNtfInitialize(&subNtfHandle, NULL, &ntfVersion), SA_AIS_OK);
			sent_ok_ids.ids[sent_ok_ids.length++] = send_alarmNotification(subNtfHandle, &myAlarmNotification);
			sent_ok_ids.ids[sent_ok_ids.length++] = send_secAlarmNotification(subNtfHandle, &mySecAlarmNotification);
			sent_ok_ids.ids[sent_ok_ids.length++] = send_stateChangeNotification(subNtfHandle, &myStateChangeNotification);
			sent_ok_ids.ids[sent_ok_ids.length++] = send_objectCreateDeleteNotification(subNtfHandle, &myObjCrDelNotification);
			sent_ok_ids.ids[sent_ok_ids.length++] = send_attrChangeNotification(subNtfHandle, &myAttrChangeNotification);
			saferror(ntftest_saNtfFinalize(subNtfHandle), SA_AIS_OK);
			test_api = SANTF_ALL;
		} else {
			saferror(rc, SA_AIS_OK);
			break;
		}
	}

	if ((rc = ntftest_saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle)) != SA_AIS_OK ||
		(rc = ntftest_saNtfNotificationFilterFree(mySecurityAlarmFilter.notificationFilterHandle)) != SA_AIS_OK ||
		(rc = ntftest_saNtfNotificationFilterFree(myStateChangeFilter.notificationFilterHandle)) != SA_AIS_OK);
	saferror(rc, SA_AIS_OK);

	if (test_api == SANTF_NOTIFICATION_UNSUBSCRIBE) wait_controllers(2);
	rc = ntftest_saNtfNotificationUnsubscribe(subscriptionId);
	if (test_api == SANTF_NOTIFICATION_UNSUBSCRIBE) {
		saferror(rc, SA_AIS_OK);
		wait_controllers(1);
	}

	if (test_api == SANTF_FINALIZE) wait_controllers(2);
	rc = ntftest_saNtfFinalize(ntfHandle);
	saferror(rc, SA_AIS_OK);
	if (test_api == SANTF_FINALIZE) wait_controllers(1);

	fprintf_v(stdout, "\nEnd test API: %s\n", ntf_api_name_list[test_api].apiName);
}

void reader_life_cycle(int test_api)
{
	rc = SA_AIS_OK;
	SaNtfAlarmNotificationFilterT myAlarmFilter;
	SaNtfSecurityAlarmNotificationFilterT mySecurityAlarmFilter;

	SaNtfReadHandleT readHandle;
	SaNtfSearchCriteriaT searchCriteria;
	SaNtfNotificationsT returnedNotification;

	fprintf_v(stdout, "\nStart test API: %s", ntf_api_name_list[test_api].apiName);
	resetCounters();

	if (test_api == SANTF_INITIALIZE) wait_controllers(2);
	rc = ntftest_saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion);
	if (test_api == SANTF_INITIALIZE) {
		saferror(rc, SA_AIS_ERR_TRY_AGAIN);
		wait_controllers(1);
		rc = ntftest_saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion);
	}
	saferror(rc, SA_AIS_OK);

	rc = ntftest_saNtfSelectionObjectGet(ntfHandle, &selectionObject);
	saferror(rc, SA_AIS_OK);

	rc = scoutage_saNtfAlarmNotificationFilterAllocate(ntfHandle, &myAlarmFilter);
	saferror(rc, SA_AIS_OK);

	rc = scoutage_saNtfSecurityAlarmNotificationFilterAllocate(ntfHandle, &mySecurityAlarmFilter);
	saferror(rc, SA_AIS_OK);

	memset(&myNotificationFilterHandles, 0, sizeof(myNotificationFilterHandles));
	myNotificationFilterHandles.alarmFilterHandle = myAlarmFilter.notificationFilterHandle;
	myNotificationFilterHandles.securityAlarmFilterHandle = mySecurityAlarmFilter.notificationFilterHandle;

	sent_ok_ids.ids[sent_ok_ids.length++] = send_alarmNotification(ntfHandle, &myAlarmNotification);
	sent_ok_ids.ids[sent_ok_ids.length++] = send_secAlarmNotification(ntfHandle, &mySecAlarmNotification);

	if (test_api == SANTF_NOTIFICATION_READ_INITIALIZE) wait_controllers(2);
	searchCriteria.searchMode = SA_NTF_SEARCH_ONLY_FILTER;
	rc = ntftest_saNtfNotificationReadInitialize(searchCriteria,
										&myNotificationFilterHandles, &readHandle);
	if (test_api == SANTF_NOTIFICATION_READ_INITIALIZE) {
		saferror(rc, SA_AIS_ERR_TRY_AGAIN);
		wait_controllers(1);
		rc = ntftest_saNtfNotificationReadInitialize(searchCriteria,
											&myNotificationFilterHandles, &readHandle);
	}
	saferror(rc, SA_AIS_OK);

	// Free one filter here to test whether all filters are recovered
	rc = ntftest_saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle);
	saferror(rc, SA_AIS_OK);

	if (test_api == SANTF_NOTIFICATION_READ_NEXT) wait_controllers(2);
	while ((rc = ntftest_saNtfNotificationReadNext(readHandle, SA_NTF_SEARCH_YOUNGER,
									&returnedNotification)) == SA_AIS_OK) {
		saNtfNotificationCallbackT(0, &returnedNotification);
	}
	if (test_api == SANTF_NOTIFICATION_READ_NEXT) {
		saferror(rc, SA_AIS_ERR_TRY_AGAIN);
		wait_controllers(1);
		// Resend notification by another handle to check if saNtfNotificationReadNext performs recovery and
		// reader can read notification
		SaNtfHandleT subNtfHandle;
		saferror(ntftest_saNtfInitialize(&subNtfHandle, NULL, &ntfVersion), SA_AIS_OK);
		sent_ok_ids.ids[sent_ok_ids.length++] = send_alarmNotification(subNtfHandle, &myAlarmNotification);
		sent_ok_ids.ids[sent_ok_ids.length++] = send_secAlarmNotification(subNtfHandle, &mySecAlarmNotification);
		saferror(ntftest_saNtfFinalize(subNtfHandle), SA_AIS_OK);

		while ((rc = ntftest_saNtfNotificationReadNext(readHandle, SA_NTF_SEARCH_YOUNGER,
										&returnedNotification)) == SA_AIS_OK) {
			saNtfNotificationCallbackT(0, &returnedNotification);
		}
	}
	saferror(rc, SA_AIS_ERR_NOT_EXIST);

	rc = ntftest_saNtfNotificationFilterFree(mySecurityAlarmFilter.notificationFilterHandle);
	saferror(rc, SA_AIS_OK);

	if (test_api == SANTF_NOTIFICATION_READ_FINALIZE) wait_controllers(2);
	rc = ntftest_saNtfNotificationReadFinalize(readHandle);
	if (test_api == SANTF_NOTIFICATION_READ_FINALIZE) {
		saferror(rc, SA_AIS_OK);
		wait_controllers(1);
	}

	if (test_api == SANTF_FINALIZE) wait_controllers(2);
	rc = ntftest_saNtfFinalize(ntfHandle);
	saferror(rc, SA_AIS_OK);
	if (test_api == SANTF_FINALIZE) wait_controllers(1);

	fprintf_v(stdout, "\nEnd test API: %s\n", ntf_api_name_list[test_api].apiName);
}

void test_sc_outage_producer_1() {
	producer_life_cycle(SANTF_INITIALIZE);
	test_validate(check_errors(), SA_AIS_OK);
}

void test_sc_outage_producer_2() {
	producer_life_cycle(SANTF_NOTIFICATION_SEND);
	test_validate(check_errors(), SA_AIS_OK);
}

void test_sc_outage_producer_3() {
	producer_life_cycle(SANTF_FINALIZE);
	test_validate(check_errors(), SA_AIS_OK);
}

void test_sc_outage_subscriber_1() {
	subscriber_life_cycle(SANTF_NOTIFICATION_SUBSCRIBE);
	test_validate(check_errors(), SA_AIS_OK);
}

void test_sc_outage_subscriber_2() {
	subscriber_life_cycle(SANTF_DISPATCH);
	test_validate(check_errors(), SA_AIS_OK);
}

void test_sc_outage_subscriber_3() {
	subscriber_life_cycle(SANTF_NOTIFICATION_UNSUBSCRIBE);
	test_validate(check_errors(), SA_AIS_OK);
}

void test_sc_outage_reader_1() {
	reader_life_cycle(SANTF_NOTIFICATION_READ_INITIALIZE);
	test_validate(check_errors(), SA_AIS_OK);
}

void test_sc_outage_reader_2() {
	reader_life_cycle(SANTF_NOTIFICATION_READ_NEXT);
	test_validate(check_errors(), SA_AIS_OK);
}

void test_sc_outage_reader_3() {
	reader_life_cycle(SANTF_NOTIFICATION_READ_FINALIZE);
	test_validate(check_errors(), SA_AIS_OK);
}

void add_scOutage_reinitializeHandle_test(void)
{
	signal(SIGUSR2, sigusr2_handler);

	test_suite_add(37, "SC outage: Test for producer");
	test_case_add(37, test_sc_outage_producer_1, "Test saNtfInitialize");
	test_case_add(37, test_sc_outage_producer_2, "Test saNtfNotificationSend");
	test_case_add(37, test_sc_outage_producer_3, "Test saNtfFinalize");
	test_suite_add(38, "SC outage: Test for consumer");
	test_case_add(38, test_sc_outage_subscriber_1, "Test saNtfNotificationSubscribe");
	test_case_add(38, test_sc_outage_subscriber_2, "Test saNtfDispatch");
	test_case_add(38, test_sc_outage_subscriber_3, "Test saNtfNotificationUnsubscribe");
	test_suite_add(39, "SC outage: Test for reader");
	test_case_add(39, test_sc_outage_reader_1, "Test saNtfNotificationReadInitialize");
	test_case_add(39, test_sc_outage_reader_2, "Test saNtfNotificationReadNext");
	test_case_add(39, test_sc_outage_reader_3, "Test saNtfNotificationReadFinalize");
}
