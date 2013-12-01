
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
 *   NTF subscriber command line program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <poll.h>

#include <saAis.h>
#include <saAmf.h>
#include <saClm.h>
#include <saNtf.h>
#include <ntfclient.h>
#include <ntfconsumer.h>

static SaNtfHandleT ntfHandle;
static SaSelectionObjectT selObj;

/* Name of current testproxy (argv[0]) */
static char *progname;

/* Release code, major version, minor version */
static SaVersionT version = { 'A', 0x01, 0x01 };

static struct s_filters_T {
	int all; 
	int alarm;
 	int obj_cr_del;
	int att_ch;
	int st_ch;
	int sec_al;
} used_filters = {1,0,0,0,0,0};

static void saNtfNotificationDiscardedCallback(SaNtfSubscriptionIdT subscriptionId,
					       SaNtfNotificationTypeT notificationType,
					       SaUint32T numberDiscarded,
					       const SaNtfIdentifierT *discardedNotificationIdentifiers)
{
	unsigned int i = 0;

	printf("Discarded callback function  notificationType: %d\n\
                  subscriptionId  : %u \n\
                  numberDiscarded : %u\n", (int)notificationType, (unsigned int)subscriptionId, (unsigned int)numberDiscarded);
	for (i = 0; i < numberDiscarded; i++)
		printf("[%u]", (unsigned int)discardedNotificationIdentifiers[i]);

	printf("\n");
}

static SaNtfCallbacksT ntfCallbacks = {
	saNtfNotificationCallback,
	saNtfNotificationDiscardedCallback
};

static SaAisErrorT waitForNotifications(SaNtfHandleT myHandle, int selectionObject, int timeout_ms)
{
	SaAisErrorT error;
	int rv;
	struct pollfd fds[1];

	fds[0].fd = (int)selectionObject;
	fds[0].events = POLLIN;

	for (;;) {
		rv = poll(fds, 1, timeout_ms);

		if (rv == -1) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "poll FAILED: %s\n", strerror(errno));
			return SA_AIS_ERR_BAD_OPERATION;
		}

		if (rv == 0) {
			printf("poll timeout\n");
			return SA_AIS_OK;
		}

		do {
			error = saNtfDispatch(myHandle, SA_DISPATCH_ALL);
			if (SA_AIS_ERR_TRY_AGAIN == error)
				sleep(1);
		} while (SA_AIS_ERR_TRY_AGAIN == error);

		if (error != SA_AIS_OK)
			fprintf(stderr, "saNtfDispatch Error %d\n", error);
	}

	return error;
}

static void usage(void)
{
	printf("\nNAME\n");
	printf("\t%s - subscribe for notfifications\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s subscribe [-t timeout]\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is a SAF NTF client used to subscribe for all incoming notifications.\n", progname);
	printf("\nOPTIONS\n");
	printf("  -t or --timeout=TIME                      timeout (sec) waiting for notification\n");
	printf("  -a or --alarm                             subscribe for only alarm notifications\n");
	printf("  -o or --objectCreateDelete                subscribe for only objectCreateDelete notifications\n");
	printf("  -c or --attributeChange                   subscribe for only attributeChange notifications\n");
	printf("  -s or --stateChange                       subscribe for only stateChange notifications\n");
	printf("  -y or --securityAlarm                     subscribe for only securityAlarm notifications\n");
	printf("  -h or --help                              this help\n");
	printf("  -v or --verbose                           print even more\n");
	exit((int)SA_AIS_ERR_INVALID_PARAM);
}

static void freeNtfFilter(SaNtfNotificationFilterHandleT *fh_ptr)
{
	SaAisErrorT errorCode = SA_AIS_OK;
	if (*fh_ptr) {  
		errorCode = saNtfNotificationFilterFree(*fh_ptr);
		if (SA_AIS_OK != errorCode) {
			fprintf(stderr, "saNtfNotificationFilterFree failed - %s\n", error_output(errorCode));
			exit(EXIT_FAILURE);
		}
	}
}

/* Subscribe */
static SaAisErrorT subscribeForNotifications(const saNotificationFilterAllocationParamsT
					     *notificationFilterAllocationParams, SaNtfSubscriptionIdT subscriptionId)
{
	SaAisErrorT errorCode = SA_AIS_OK;
	SaNtfAlarmNotificationFilterT myAlarmFilter;
	SaNtfAttributeChangeNotificationFilterT attChFilter;
	SaNtfStateChangeNotificationFilterT stChFilter;
	SaNtfObjectCreateDeleteNotificationFilterT objCrDelFilter;
	SaNtfSecurityAlarmNotificationFilterT secAlarmFilter;
	
	SaNtfNotificationTypeFilterHandlesT notificationFilterHandles;
	memset(&notificationFilterHandles, 0, sizeof notificationFilterHandles);

	if (used_filters.all || used_filters.alarm) {
		errorCode = saNtfAlarmNotificationFilterAllocate(ntfHandle,
								 &myAlarmFilter,
								 notificationFilterAllocationParams->numEventTypes,
								 notificationFilterAllocationParams->numNotificationObjects,
								 notificationFilterAllocationParams->numNotifyingObjects,
								 notificationFilterAllocationParams->numNotificationClassIds,
								 notificationFilterAllocationParams->numProbableCauses,
								 notificationFilterAllocationParams->numPerceivedSeverities,
								 notificationFilterAllocationParams->numTrends);
	
		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saNtfAlarmNotificationFilterAllocate failed - %s\n", error_output(errorCode));
			return errorCode;
		}
		notificationFilterHandles.alarmFilterHandle = myAlarmFilter.notificationFilterHandle;
	}
	
	if (used_filters.all || used_filters.att_ch) {
		errorCode = saNtfAttributeChangeNotificationFilterAllocate(ntfHandle,
			&attChFilter,
			notificationFilterAllocationParams->numEventTypes,
			notificationFilterAllocationParams->numNotificationObjects,
			notificationFilterAllocationParams->numNotifyingObjects,
			notificationFilterAllocationParams->numNotificationClassIds,
			0);

		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saNtfAttributeChangeNotificationFilterAllocate failed - %s\n", error_output(errorCode));
			return errorCode;
		}
		notificationFilterHandles.attributeChangeFilterHandle = attChFilter.notificationFilterHandle;
	}
	
	if (used_filters.all || used_filters.obj_cr_del) {
		errorCode = saNtfObjectCreateDeleteNotificationFilterAllocate(ntfHandle,
			&objCrDelFilter,
			notificationFilterAllocationParams->numEventTypes,
			notificationFilterAllocationParams->numNotificationObjects,
			notificationFilterAllocationParams->numNotifyingObjects,
			notificationFilterAllocationParams->numNotificationClassIds,
			0);

		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saNtfObjectCreateDeleteNotificationFilterAllocate failed - %s\n", error_output(errorCode));
			return errorCode;
		}
		notificationFilterHandles.objectCreateDeleteFilterHandle = objCrDelFilter.notificationFilterHandle;
	}

	if (used_filters.all || used_filters.st_ch) {
		errorCode = saNtfStateChangeNotificationFilterAllocate(ntfHandle,
			&stChFilter,
			notificationFilterAllocationParams->numEventTypes,
			notificationFilterAllocationParams->numNotificationObjects,
			notificationFilterAllocationParams->numNotifyingObjects,
			notificationFilterAllocationParams->numNotificationClassIds,
			0,
			0);
		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saNtfStateChangeNotificationFilterAllocate failed - %s\n", error_output(errorCode));
			return errorCode;
		}
		notificationFilterHandles.stateChangeFilterHandle = stChFilter.notificationFilterHandle;
	}
	
	if (used_filters.all || used_filters.sec_al) {
		errorCode = saNtfSecurityAlarmNotificationFilterAllocate(ntfHandle,
			&secAlarmFilter,
			notificationFilterAllocationParams->numEventTypes,
			notificationFilterAllocationParams->numNotificationObjects,
			notificationFilterAllocationParams->numNotifyingObjects,
			notificationFilterAllocationParams->numNotificationClassIds,
			0,0,0,0,0);
		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saNtfSecurityAlarmNotificationFilterAllocate failed - %s\n", error_output(errorCode));
			return errorCode;
		}
		notificationFilterHandles.securityAlarmFilterHandle = secAlarmFilter.notificationFilterHandle;
	}

	
	errorCode = saNtfNotificationSubscribe(&notificationFilterHandles, subscriptionId);
	if (SA_AIS_OK != errorCode) {
		fprintf(stderr, "saNtfNotificationSubscribe failed - %s\n", error_output(errorCode));
		return errorCode;
	}
	freeNtfFilter (&notificationFilterHandles.alarmFilterHandle);
	freeNtfFilter (&notificationFilterHandles.attributeChangeFilterHandle);
	freeNtfFilter (&notificationFilterHandles.objectCreateDeleteFilterHandle);
	freeNtfFilter (&notificationFilterHandles.stateChangeFilterHandle);
	freeNtfFilter (&notificationFilterHandles.securityAlarmFilterHandle);

	return errorCode;
}

int main(int argc, char *argv[])
{
	int c;
	SaAisErrorT error;
	int timeout = -1;	/* block indefintively in poll */
	saNotificationFilterAllocationParamsT notificationFilterAllocationParams = {0};
	SaNtfSubscriptionIdT subscriptionId = 1;
	struct option long_options[] = {
		{"alarm", no_argument, 0, 'a'},
		{"attributeChange", no_argument, 0, 'c'},
		{"objectCreateDelete", no_argument, 0, 'o'},
		{"stateChange", no_argument, 0, 's'},
		{"securityAlarm", no_argument, 0, 'y'},
		{"help", no_argument, 0, 'h'},
		{"timeout", required_argument, 0, 't'},
		{"verbose", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	verbose = 0;
	progname = argv[0];

	/* Check options */
	while (1) {
		c = getopt_long(argc, argv, "acosyht:v", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'a':
			used_filters.all = 0;
			used_filters.alarm = 1;
			break;
		case 'o':
			used_filters.all = 0;
			used_filters.obj_cr_del = 1;
			break;
		case 'c':
			used_filters.all = 0;
			used_filters.att_ch = 1;
			break;
		case 's':
			used_filters.all = 0;
			used_filters.st_ch = 1;
			break;
		case 'y':
			used_filters.all = 0;
			used_filters.sec_al = 1;
			break;
		case 't':
			timeout = atoi(optarg) * 1000;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
		case '?':
		default:
			usage();
			break;
		}
	}

	error = saNtfInitialize(&ntfHandle, &ntfCallbacks, &version);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saNtfInitialize failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	error = saNtfSelectionObjectGet(ntfHandle, &selObj);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saNtfSelectionObjectGet failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	error = subscribeForNotifications(&notificationFilterAllocationParams, subscriptionId);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "subscribeForNotifications failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	error = waitForNotifications(ntfHandle, selObj, timeout);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "subscribeForNotifications failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	error = saNtfNotificationUnsubscribe(subscriptionId);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "waitForNotifications failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	error = saNtfFinalize(ntfHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saNtfFinalize failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

