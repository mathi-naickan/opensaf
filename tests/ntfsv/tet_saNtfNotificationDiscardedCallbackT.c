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
#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#include "tet_ntf.h"
#include "tet_ntf_common.h"

#define NUM_NOTIF 2
static SaNtfIdentifierT cb_notId[NUM_NOTIF];
static SaNtfSubscriptionIdT my_subid = 64;
static int cb_index = 0;
extern int verbose;
static SaUint32T numDiscarded = 0;

static void resetCounters()
{
    int i;
    for(i=0; i<NUM_NOTIF;i++)
    {
        cb_notId[i]=0;
    }
    cb_index=0;
}

static void saNtfNotificationCallbackT(
    SaNtfSubscriptionIdT subscriptionId,
    const SaNtfNotificationsT *notification)
{
    SaNtfIdentifierT notId;

    notId = *notification->
        notification.alarmNotification.notificationHeader.notificationId;
/*  if(verbose)                                                         */
/*  {                                                                   */
/*      tet_printf("\nsaNtfNotificationCallbackT() subscriptionId: %u, "*/
/*                 "notificationId %llu",                               */
/*                 subscriptionId, notId);                              */
/*  }                                                              */
    safassert(my_subid, subscriptionId);

    cb_notId[cb_index++] = notId;

    SaNtfNotificationHandleT notificationHandle = \
            notification->notification.alarmNotification.notificationHandle;
    safassert(saNtfNotificationFree(notificationHandle), SA_AIS_OK);
/*  if(verbose)                                       */
/*  {                                                 */
/*      newNotification(subscriptionId, notification);*/
/*  }
 */
}

static void saNtfDiscardedCallback(
    SaNtfSubscriptionIdT subscriptionId,
    SaNtfNotificationTypeT notificationType,
    SaUint32T numberDiscarded,
    const SaNtfIdentifierT *discardedNotificationIdentifiers)
{
    numDiscarded += numberDiscarded;
}

static SaNtfCallbacksT ntfCbTest = {
    saNtfNotificationCallbackT,
    saNtfDiscardedCallback
};

static SaNtfAlarmNotificationT myAlarmNotification;


/* OBS! TODO: this test case is incomplete. Due to currently no limit in allocating */

/* incomming notifications in the MDS thread discarding makes no sense */
void saNtfNotificationDiscardedCallbackT_01(void)
{
	struct pollfd fds[1];
	int ret, i;
	saNotificationAllocationParamsT myNotificationAllocationParams;
	saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
	saNotificationParamsT myNotificationParams;

	AlarmNotificationParams myAlarmParams;
	SaNtfAlarmNotificationFilterT myAlarmFilter;
	SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles;
	SaNtfIdentifierT myNotId[NUM_NOTIF] = {0,0};

	resetCounters();
	fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams,
			&myNotificationParams);
	if (verbose) {
		printf("num not to send: %d\n", NUM_NOTIF);
	}

	//   rc=SA_AIS_ERR_NOT_SUPPORTED; /* Not fully implemented */
	//   test_validate(rc, SA_AIS_OK);       /* See above             */
	//   return;                      /*                       */

	safassert(saNtfInitialize(&ntfHandle, &ntfCbTest, &ntfVersion), SA_AIS_OK);
	safassert(saNtfSelectionObjectGet(ntfHandle, &selectionObject), SA_AIS_OK);

	safassert(saNtfAlarmNotificationFilterAllocate(
			ntfHandle,
			&myAlarmFilter,
			myNotificationFilterAllocationParams.numEventTypes,
			myNotificationFilterAllocationParams.numNotificationObjects,
			myNotificationFilterAllocationParams.numNotifyingObjects,
			myNotificationFilterAllocationParams.numNotificationClassIds,
			myNotificationFilterAllocationParams.numProbableCauses,
			myNotificationFilterAllocationParams.numPerceivedSeverities,
			myNotificationFilterAllocationParams.numTrends), SA_AIS_OK);
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
			my_subid),
			SA_AIS_OK);

	myAlarmParams.numCorrelatedNotifications =
			myNotificationAllocationParams.numCorrelatedNotifications;
	myAlarmParams.lengthAdditionalText =
			myNotificationAllocationParams.lengthAdditionalText;
	myAlarmParams.numAdditionalInfo =
			myNotificationAllocationParams.numAdditionalInfo;
	myAlarmParams.numSpecificProblems =
			myNotificationAllocationParams.numSpecificProblems;
	myAlarmParams.numMonitoredAttributes =
			myNotificationAllocationParams.numMonitoredAttributes;
	myAlarmParams.numProposedRepairActions =
			myNotificationAllocationParams.numProposedRepairActions;
	myAlarmParams.variableDataSize =
			myNotificationAllocationParams.variableDataSize;

	safassert(saNtfAlarmNotificationAllocate(
			ntfHandle,
			&myAlarmNotification,
			myAlarmParams.numCorrelatedNotifications,
			myAlarmParams.lengthAdditionalText,
			myAlarmParams.numAdditionalInfo,
			myAlarmParams.numSpecificProblems,
			myAlarmParams.numMonitoredAttributes,
			myAlarmParams.numProposedRepairActions,
			myAlarmParams.variableDataSize), SA_AIS_OK);

	myNotificationParams.eventType = myNotificationParams.alarmEventType;
	fill_header_part(
			&myAlarmNotification.notificationHeader,
			(saNotificationParamsT *) & myNotificationParams,
			myAlarmParams.lengthAdditionalText);

	/* determine perceived severity */
	*(myAlarmNotification.perceivedSeverity) =
			myNotificationParams.perceivedSeverity;

	/* set probable cause*/
	*(myAlarmNotification.probableCause) =
			myNotificationParams.probableCause;
	for (i = 0; i < NUM_NOTIF; i++) {
		if ((rc = saNtfNotificationSend(myAlarmNotification.notificationHandle)) != SA_AIS_OK) {
			if (verbose) {
				printf("send failed rc: %d\n", rc);
			}
			break;
		}
		myNotId[i] = *myAlarmNotification.notificationHeader.notificationId;
	}
	sleep(10);

	fds[0].fd = (int) selectionObject;
	fds[0].events = POLLIN;
	ret = poll(fds, 1, 10000);
	assert(ret > 0);
	safassert(saNtfDispatch(ntfHandle, SA_DISPATCH_ALL), SA_AIS_OK);

	rc = SA_AIS_ERR_FAILED_OPERATION;
	for (i = 0; i < NUM_NOTIF; i++) {
		if (cb_notId[i] != myNotId[i]) {
			break;
		}
	}

	if (numDiscarded == 0) {
		rc = SA_AIS_OK;
	} else if (numDiscarded != (NUM_NOTIF - i)) {
		if (verbose) {
			printf("failed num of discarded: %d\n", numDiscarded);
		}
		rc = SA_AIS_ERR_FAILED_OPERATION;
	}
	if (verbose) {
		printf("Number of discarded: %d\n", numDiscarded);
		printf("Number of received not: %d\n", cb_index);
	}

        free(myNotificationParams.additionalText); /* allocated in fillInDefaultValues */
        safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);
	safassert(saNtfNotificationFree(myAlarmNotification.notificationHandle), SA_AIS_OK);
	safassert(saNtfNotificationUnsubscribe(my_subid),
			SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(rc, SA_AIS_OK);
}

__attribute__ ((constructor)) static void saNtfNotificationDiscardedCallbackT_constructor(void)
{
    test_suite_add(23, "Consumer operations - discarded notification");
    test_case_add(23,saNtfNotificationDiscardedCallbackT_01, "saNtfNotificationDiscardedCallbackT Ok");
}

