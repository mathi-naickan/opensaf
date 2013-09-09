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
 
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>
#include <utest.h>
#include <util.h>
#include "tet_ntf.h"
#include "tet_ntf_common.h"

extern int verbose;

void free_notif(
    SaNtfSubscriptionIdT subscriptionId,
    const SaNtfNotificationsT *notification) {
    SaNtfNotificationHandleT notificationHandle = 0;
    switch (notification->notificationType) {
        case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
            notificationHandle = notification->notification.objectCreateDeleteNotification.notificationHandle;
            break;

        case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
            notificationHandle = notification->notification.attributeChangeNotification.notificationHandle;
            break;

        case SA_NTF_TYPE_STATE_CHANGE:
            notificationHandle = notification->notification.stateChangeNotification.notificationHandle;
            break;

        case SA_NTF_TYPE_ALARM:
            notificationHandle = notification->notification.alarmNotification.notificationHandle;
            break;

        case SA_NTF_TYPE_SECURITY_ALARM:
            notificationHandle = notification->notification.securityAlarmNotification.notificationHandle;
            break;

        default:
            assert(0);
            break;
    }
    if (notificationHandle != 0) {
        safassert(saNtfNotificationFree(notificationHandle), SA_AIS_OK);
    }
}

/**
 *
 * Successful reading of (alarm)notifications
 */
void saNtfNotificationReadNext_01(void)
{
    saNotificationAllocationParamsT        myNotificationAllocationParams;
    saNotificationFilterAllocationParamsT  myNotificationFilterAllocationParams;
    saNotificationParamsT                  myNotificationParams;

    SaNtfSearchCriteriaT searchCriteria;
    SaNtfAlarmNotificationFilterT myAlarmFilter;
    SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles = {0,0,0,0,0};
    SaNtfReadHandleT readHandle;
    SaNtfHandleT ntfHandle;
    SaNtfNotificationsT returnedNotification;
    SaNtfAlarmNotificationT myNotification;
    searchCriteria.searchMode = SA_NTF_SEARCH_ONLY_FILTER;
    SaAisErrorT errorCode;
    SaUint32T readCounter = 0;

    fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams,
			&myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle,
					   &ntfCallbacks,
					   &ntfVersion), SA_AIS_OK);

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

    myNotificationFilterHandles.alarmFilterHandle =
	myAlarmFilter.notificationFilterHandle;
	 myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_WARNING;
	 myAlarmFilter.perceivedSeverities[1] = SA_NTF_SEVERITY_CLEARED;

    /* Send one alarm notification */
    safassert(saNtfAlarmNotificationAllocate(
	ntfHandle, /* handle to Notification Service instance */
        &myNotification,
        /* number of correlated notifications */
        myNotificationAllocationParams.numCorrelatedNotifications,
        /* length of additional text */
        myNotificationAllocationParams.lengthAdditionalText,
        /* number of additional info items*/
        myNotificationAllocationParams.numAdditionalInfo,
        /* number of specific problems */
        myNotificationAllocationParams.numSpecificProblems,
        /* number of monitored attributes */
        myNotificationAllocationParams.numMonitoredAttributes,
        /* number of proposed repair actions */
        myNotificationAllocationParams.numProposedRepairActions,
        /* use default allocation size */
        myNotificationAllocationParams.variableDataSize), SA_AIS_OK);

    myNotificationParams.eventType =
        myNotificationParams.alarmEventType;

    fill_header_part(
        &myNotification.notificationHeader,
        (saNotificationParamsT *)&myNotificationParams,
        myNotificationAllocationParams.lengthAdditionalText);

    /* determine perceived severity */
    *(myNotification.perceivedSeverity) =
        myNotificationParams.perceivedSeverity;

    /* set probable cause*/
    *(myNotification.probableCause) =
        myNotificationParams.probableCause;

    safassert( saNtfNotificationSend(myNotification.notificationHandle), SA_AIS_OK);
    /* Read initialize here to get the notification above */
    safassert( saNtfNotificationReadInitialize(searchCriteria,
                                              &myNotificationFilterHandles,
                                              &readHandle), SA_AIS_OK);

    /* read as many matching notifications as exist for the time period between
     the last received one and now */
   for ( ; (errorCode = saNtfNotificationReadNext(
       readHandle,
       SA_NTF_SEARCH_YOUNGER,
       &returnedNotification)) == SA_AIS_OK; )

/* errorCode = saNtfNotificationReadNext(*/
/*          readHandle,                  */
/*          SA_NTF_SEARCH_YOUNGER,       */
/*          &returnedNotification);      */
/* if (errorCode == SA_AIS_OK)           */
   {
       safassert(errorCode, SA_AIS_OK);
       readCounter++;

       if(verbose)
       {
           newNotification(69, &returnedNotification);
       } else {
           free_notif(0, &returnedNotification);
       }
   }
   if(verbose)
   {
       (void)printf("errorcode to break loop: %d\n",(int) errorCode);
   }
    if (readCounter == 0) {
	errorCode = SA_AIS_ERR_FAILED_OPERATION;
    }
    // No more...
    safassert(saNtfNotificationReadFinalize(readHandle), SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);
    free(myNotificationParams.additionalText);
    safassert(saNtfNotificationFree(myNotification.notificationHandle), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    test_validate(errorCode, SA_AIS_ERR_NOT_EXIST); /* read all notifications!! */
}

/**
 *
 * Successful reading of (alarm)notifications
 */
void saNtfNotificationReadNext_02(void)
{
    int i;
    SaNtfIdentifierT notids[3];
    saNotificationAllocationParamsT        myNotificationAllocationParams;
    saNotificationFilterAllocationParamsT  myNotificationFilterAllocationParams;
    saNotificationParamsT                  myNotificationParams;

    SaNtfSearchCriteriaT searchCriteria;
    SaNtfAlarmNotificationFilterT myAlarmFilter;
    SaNtfNotificationTypeFilterHandlesT myNotificationFilterHandles = {0,0,0,0,0};
    SaNtfReadHandleT readHandle;
    SaNtfHandleT ntfHandle;
    SaNtfNotificationsT returnedNotification;
    SaNtfAlarmNotificationT myNotification;
    searchCriteria.searchMode = SA_NTF_SEARCH_AFTER_TIME;
    SaAisErrorT errorCode;
    SaUint32T readCounter = 0;


    fillInDefaultValues(&myNotificationAllocationParams,
			&myNotificationFilterAllocationParams,
			&myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle,
					   &ntfCallbacks,
					   &ntfVersion), SA_AIS_OK);

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

	 myNotificationFilterHandles.alarmFilterHandle =
		 myAlarmFilter.notificationFilterHandle;
	 myAlarmFilter.perceivedSeverities[0] = SA_NTF_SEVERITY_WARNING;
	 myAlarmFilter.perceivedSeverities[1] = SA_NTF_SEVERITY_CLEARED;

    /* Send one alarm notification */
    safassert(saNtfAlarmNotificationAllocate(
	ntfHandle, /* handle to Notification Service instance */
        &myNotification,
        /* number of correlated notifications */
        myNotificationAllocationParams.numCorrelatedNotifications,
        /* length of additional text */
        myNotificationAllocationParams.lengthAdditionalText,
        /* number of additional info items*/
        myNotificationAllocationParams.numAdditionalInfo,
        /* number of specific problems */
        myNotificationAllocationParams.numSpecificProblems,
        /* number of monitored attributes */
        myNotificationAllocationParams.numMonitoredAttributes,
        /* number of proposed repair actions */
        myNotificationAllocationParams.numProposedRepairActions,
        /* use default allocation size */
        myNotificationAllocationParams.variableDataSize), SA_AIS_OK);

    myNotificationParams.eventType =
        myNotificationParams.alarmEventType;

    fill_header_part(
        &myNotification.notificationHeader,
        (saNotificationParamsT *)&myNotificationParams,
        myNotificationAllocationParams.lengthAdditionalText);

    /* determine perceived severity */
    *(myNotification.perceivedSeverity) =
        myNotificationParams.perceivedSeverity;

    /* set probable cause*/
    *(myNotification.probableCause) =
        myNotificationParams.probableCause;

    safassert( saNtfNotificationSend(myNotification.notificationHandle), SA_AIS_OK);
    /* Read initialize here to get the notification above */
    safassert( saNtfNotificationReadInitialize(searchCriteria,
                                              &myNotificationFilterHandles,
                                              &readHandle), SA_AIS_OK);

    /* read as many matching notifications as exist for the time period between
     the last received one and now */
   for (i=0 ; i<3 && (errorCode = saNtfNotificationReadNext(
       readHandle,
       SA_NTF_SEARCH_YOUNGER,
       &returnedNotification)) == SA_AIS_OK; i++)
   {
       safassert(errorCode, SA_AIS_OK);
       readCounter++;
       notids[i] = *returnedNotification.notification.alarmNotification.
           notificationHeader.notificationId;
       if(verbose)
       {
           newNotification(69, &returnedNotification);
       } else {
           free_notif(0, &returnedNotification);
       }
   }
   if(verbose)
   {
       (void)printf("errorcode to break loop: %d\n",(int) errorCode);
   }
   if (readCounter == 0) {
       errorCode = SA_AIS_ERR_FAILED_OPERATION;
       goto error;
   }

   readCounter =0;
   SaNtfIdentifierT last_id = 0;
   for (i=0 ; (errorCode = saNtfNotificationReadNext(
       readHandle,
       SA_NTF_SEARCH_OLDER,
       &returnedNotification)) == SA_AIS_OK;)
   {
       safassert(errorCode, SA_AIS_OK);
       readCounter++;
       last_id = *returnedNotification.notification.alarmNotification.
        notificationHeader.notificationId;
       if(verbose)
       {
           newNotification(69, &returnedNotification);
       } else {
           free_notif(0, &returnedNotification);
       }
   }

   if(verbose)
   {
       (void)printf("errorcode to break loop: %d\n",(int) errorCode);
   }
    if (readCounter == 0) {
	errorCode = SA_AIS_ERR_FAILED_OPERATION;
    }

    /* check that last is the same as the first */
    if (last_id != notids[0])
    {
        errorCode = SA_AIS_ERR_FAILED_OPERATION;
    }
    // No more...
error:
    safassert(saNtfNotificationReadFinalize(readHandle), SA_AIS_OK);
    safassert(saNtfNotificationFilterFree(myAlarmFilter.notificationFilterHandle), SA_AIS_OK);
    safassert(saNtfNotificationFree(myNotification.notificationHandle), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    free(myNotificationParams.additionalText);
    test_validate(errorCode, SA_AIS_ERR_NOT_EXIST); /* read all notifications!! */
}

/**
 *
 * Invoke test script which tests the different search criterias
 */
void saNtfNotificationReadNext_03(void)
{
	SaAisErrorT errorCode = SA_AIS_OK;
	int rc = 0;
	rc = system("/usr/local/bin/ntf_search_criteria_test.sh");
	if (rc != 0) {
		rc = WEXITSTATUS(rc);
		switch (rc) {
		case SA_NTF_SEARCH_BEFORE_OR_AT_TIME:
			(void)printf("SA_NTF_SEARCH_BEFORE_OR_AT_TIME FAILED\n");
			break;
		case SA_NTF_SEARCH_AT_TIME:
			(void)printf("SA_NTF_SEARCH_AT_TIME FAILED\n");
			break;
		case SA_NTF_SEARCH_AT_OR_AFTER_TIME:
			(void)printf("SA_NTF_SEARCH_AT_OR_AFTER_TIME FAILED\n");
			break;
		case SA_NTF_SEARCH_BEFORE_TIME:
			(void)printf("SA_NTF_SEARCH_BEFORE_TIME FAILED\n");
			break;
		case SA_NTF_SEARCH_AFTER_TIME:
			(void)printf("SA_NTF_SEARCH_AFTER_TIME FAILED\n");
			break;
		case SA_NTF_SEARCH_NOTIFICATION_ID:
			(void)printf("SA_NTF_SEARCH_NOTIFICATION_ID FAILED\n");
			break;
		case SA_NTF_SEARCH_ONLY_FILTER:
			(void)printf("SA_NTF_SEARCH_ONLY_FILTER FAILED\n");
			break;
		default:
			(void)printf("error code out of range (%d)\n", rc);
		}
		errorCode = SA_AIS_ERR_FAILED_OPERATION;
	}
	test_validate(errorCode, SA_AIS_OK);	
}

__attribute__ ((constructor)) static void saNtfNotificationReadNext_constructor(void)
{
    test_suite_add(22, "Consumer operations - Reader API 3");
    test_case_add(22, saNtfNotificationReadNext_01, "saNtfNotificationReadNext SA_AIS_OK");
    test_case_add(22, saNtfNotificationReadNext_02, "saNtfNotificationReadNext 3 search younger 3 search older");
    test_case_add(22, saNtfNotificationReadNext_03, "saNtfSearchCriteriaT test all");
}

