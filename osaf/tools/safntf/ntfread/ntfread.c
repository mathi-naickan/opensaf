/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2011 The OpenSAF Foundation
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
 *   NTF read command line program.
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

#include <saAis.h>
#include <saAmf.h>
#include <saClm.h>
#include <saNtf.h>
#include <ntfclient.h>
#include <ntfconsumer.h>

#define NTFSV_ENUM_NOT_SET 99

SaNtfHandleT ntfHandle;


/* Name of current testproxy (argv[0]) */
static char *progname;

/* Release code, major version, minor version */
static SaVersionT version = { 'A', 0x01, 0x01};

static bool filterAlarm = true;
static bool filterSecurityAlarm = true;
static SaNtfNotificationTypeFilterHandlesT fhdls = {0};
static SaNtfReadHandleT readHandle;
static SaNtfAlarmNotificationFilterT af;
static SaNtfSecurityAlarmNotificationFilterT saf;

/* reader input parameters */
static SaNtfSearchCriteriaT searchCriteria = {SA_NTF_SEARCH_ONLY_FILTER,0,0};
static SaNtfSearchDirectionT searchDirection = SA_NTF_SEARCH_YOUNGER;

/* filter header option */
static SaUint16T nNnObj = 0;
static SaUint16T nNyObj = 0;
static SaUint16T nCId = 0;

static SaNtfEventTypeT eType = 0;
static SaNameT nObj = {0};
static SaNameT nyObj = {0};
static SaNtfClassIdT cId;

/* common alarm and security alarm */
static SaUint16T nPCause = 0;
static SaUint16T nSeverities = 0;

static SaNtfSeverityT severity = NTFSV_ENUM_NOT_SET;
static SaNtfProbableCauseT probableCause = NTFSV_ENUM_NOT_SET;

/* alarm specific filter option */
static SaUint16T nETypes = 0;
static SaUint16T nTrends = 0;

static SaNtfSeverityTrendT trend = NTFSV_ENUM_NOT_SET;

/* security alarm specific filter option */
static SaUint16T nsETypes = 0;
static SaUint16T numSecurityAlarmDetectors = 0;
static SaUint16T numServiceUsers = 0;
static SaUint16T numServiceProviders = 0;

static SaNtfEventTypeT seType = 0;
static SaNtfSecurityAlarmDetectorT secAlarmDetector = {0};
static SaNtfServiceUserT serviceUser = {0};
static SaNtfServiceUserT serviceProvider = {0};

static SaNtfCallbacksT ntfCallbacks = {
	NULL,
	NULL
};

static void usage(void) {
	printf("\nNAME\n");
	printf("\t%s - read alarm and security alarm notifications\n",
		progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [OPTIONS]\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is a SAF NTF client to read notifications that match the "
	       "filter options given.\n", progname);
	printf("\nOPTIONS\n");
	printf("  -b or --searchMode=1...7                  "
	       "numeric value of alarm SaNtfSearchModeT \n");
	printf("                                            "
	       "(SA_NTF_SEARCH_BEFORE_OR_AT_TIME...SA_NTF_SEARCH_ONLY_FILTER)\n"
		);
	printf("  -c or --notificationClassId=VE,MA,MI      "
	       "vendorid, majorid, minorid\n");
	printf("  -d or --securityEventType=20480...20485   "
	       "numeric value of security alarm SaNtfEventTypeT\n");
	printf("                                            "
	       "(SA_NTF_SECURITY_ALARM_NOTIFICATIONS_START..."
	       "SA_NTF_TIME_VIOLATION)\n");
	printf("  -e or --eventType=16384...16389           "
	       "numeric value of alarm SaNtfEventTypeT\n");
	printf("                                            "
	       "(SA_NTF_ALARM_NOTIFICATIONS_START...SA_NTF_ALARM_ENVIRONMENT)\n"
		);
	printf("  -E or --eventTime=TIME                    "
	       "numeric value of SaTimeT\n");
	printf("  -i or --notificationId=<nId>              "
	       "search for a specific notification id\n");
	printf("  -k or --onlyAlarm                         "
	       "use only alarm filter\n");
	printf("  -l or --onlySecurityAlarm                 "
	       "use only securtiy alarm filter\n");
	printf("  -n or --notificationObject=NOT_OBJ        "
	       "notification object (string value)\n");
	printf("  -N or --notifyingObject=NOTIFY_OBJ        "
	       "notififying object (string value)\n");
	printf("  -o or --searchOlder                       "
	       "SA_NTF_SEARCH_OLDER\n");
	printf("  -p or --probableCause=0..74               "
	       "numeric value SaNtfProbableCauseT\n");
	printf("                                            "
	       "SA_NTF_ADAPTER_ERROR to SA_NTF_UNSPECIFIED_REASON\n");
	printf("  -s or --perceivedSeverity=0...5           "
	       "severity numeric value\n");
	printf("                                            "
	       "(clear=0,ind,warn,min,maj,crit=5)\n");
	printf("  -v or --verbose                           verbose mode\n");
	printf("  -h or --help                              this help\n");
}

static void assignAlarmFilter() {
	if (nETypes)
		*af.notificationFilterHeader.eventTypes = eType;
	if (nNnObj)
		*af.notificationFilterHeader.notificationObjects = nObj;
	if (nNyObj)
		*af.notificationFilterHeader.notifyingObjects = nyObj;
	if (nCId)
		*af.notificationFilterHeader.notificationClassIds = cId;
	if (nSeverities)
		af.perceivedSeverities[0] = severity;
	if (nPCause)
		af.probableCauses[0] = probableCause;
	if (nTrends)
		af.trends[0] = trend;
}

static void assignSecAlarmFilters() {
	if (nsETypes)
		*saf.notificationFilterHeader.eventTypes = seType;
	if (nNnObj)
		*saf.notificationFilterHeader.notificationObjects = nObj;
	if (nNyObj)
		*saf.notificationFilterHeader.notifyingObjects = nyObj;
	if (nCId)
		*saf.notificationFilterHeader.notificationClassIds = cId;
	if (nSeverities)
		saf.severities[0] = severity;
	if (nPCause)
		saf.probableCauses[0] = probableCause;
	if (numSecurityAlarmDetectors)
		saf.securityAlarmDetectors[0] = secAlarmDetector;
	if (numServiceProviders)
		saf.serviceProviders[0] = serviceProvider;
	if (numServiceUsers)
		saf.serviceUsers[0] = serviceUser;
}

static SaAisErrorT readAll() {
	SaNtfNotificationsT n;
	SaAisErrorT rc;
	if (filterAlarm) {
		rc = saNtfAlarmNotificationFilterAllocate(ntfHandle, &af,
			nETypes, nNnObj, nNyObj, nCId, nPCause, nSeverities,
			nTrends);
		if (rc != SA_AIS_OK) {
			fprintf(stderr, "saNtfAlarmNotificationFilterAllocate "
					"failed - %s\n", error_output(rc));
			goto done;
		}
		assignAlarmFilter();
	}
	if (filterSecurityAlarm) {
		rc = saNtfSecurityAlarmNotificationFilterAllocate(ntfHandle,
			&saf, nsETypes, nNnObj, nNyObj, nCId, nPCause,
			nSeverities, numSecurityAlarmDetectors, numServiceUsers,
			numServiceProviders);
		if (rc != SA_AIS_OK) {
			fprintf(stderr,
				"saNtfSecurityAlarmNotificationFilterAllocate "
				"failed - %s\n", error_output(rc));
			goto done;
		}
		assignSecAlarmFilters();
	}

	fhdls.alarmFilterHandle = af.notificationFilterHandle;
	fhdls.securityAlarmFilterHandle = saf.notificationFilterHandle; 

	rc = saNtfNotificationReadInitialize(searchCriteria, &fhdls,
		&readHandle);
	if (rc != SA_AIS_OK) {
		fprintf(stderr, "saNtfNotificationReadInitialize failed - %s\n",
			error_output(rc));
		goto done;
	}

    /* read as many notifications as exist */
	while ((rc = saNtfNotificationReadNext(readHandle, searchDirection, &n))
	       == SA_AIS_OK) {
		saNtfNotificationCallback(0, &n);                                       
	}
	if (rc == SA_AIS_ERR_NOT_EXIST) {
		rc = SA_AIS_OK;	/* no more notification exists */ 
	} else {
		fprintf(stderr, "saNtfNotificationReadNext failed - %s\n",
			error_output(rc));           
	}
	done:
	return rc;
}       

int main(int argc, char *argv[]) {
	int c;
	SaAisErrorT rc;
	struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"searchMode", required_argument, 0, 'b'},
		{"notificationClassId", required_argument, 0, 'c'},
		{"securityEventType", required_argument, 0, 'd'},
		{"eventTime", required_argument, 0, 'E'},
		{"eventType", required_argument, 0, 'e'},
		{"notificationId", required_argument, 0, 'i'},
		{"onlyAlarm", no_argument, 0, 'k'},
		{"onlySecurityAlarm", no_argument, 0, 'l'},
		{"notifyingObject", required_argument, 0, 'N'},
		{"notificationObject", required_argument, 0, 'n'},
		{"searchOlder", no_argument, 0, 'o'},
		{"probableCause", required_argument, 0, 'p'},
		{"perceivedSeverity", required_argument, 0, 's'},
		{"verbose", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	verbose = 0;
	progname = argv[0];

	/* Check options */
	while (1) {
		c = getopt_long(argc, argv, "b:c:d:hE:e:i:klN:n:op:s:v",
			long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'b':
			searchCriteria.searchMode =
				(SaNtfSearchModeT)atoi(optarg);
			break;
		case 'c':
			getVendorId(&cId);
			nCId = 1;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		case 'd':
			seType = (SaNtfEventTypeT)atoi(optarg);
			nsETypes = 1;
			break;
		case 'E':
			searchCriteria.eventTime = (SaTimeT)atoll(optarg);
			break;
		case 'e':
			eType = (SaNtfEventTypeT)atoi(optarg);
			nETypes = 1;
			break;
		case 'i':
			searchCriteria.notificationId =
				(SaNtfIdentifierT)atoll(optarg);
			break;
		case 'k':
			filterSecurityAlarm = false;
			break;
		case 'l':
			filterAlarm = false;
			break;
		case 'N':
			nyObj.length = (SaUint16T)strlen(optarg);
			if (SA_MAX_NAME_LENGTH < nyObj.length) {
				fprintf(stderr, "notifyingObject too long\n");
				exit(EXIT_FAILURE);
			}
			(void)memcpy(nyObj.value, optarg, nyObj.length);
			nNyObj = 1;
			break;
		case 'n':
			nObj.length = (SaUint16T)strlen(optarg);
			if (SA_MAX_NAME_LENGTH < nObj.length) {
				fprintf(stderr,
					"notificationObject too long\n");
				exit(EXIT_FAILURE);
			}
			(void)memcpy(nObj.value, optarg, nObj.length);
			nNnObj = 1;
			break;
		case 'o':
			searchDirection = SA_NTF_SEARCH_OLDER;
			break;
		case 'p':
			probableCause = (SaNtfProbableCauseT)atoi(optarg);
			nPCause = 1;
			break;
		case 's':
			severity = (SaNtfSeverityT)atoi(optarg);
			nSeverities = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage();
			fprintf(stderr, "invalid argument: -%c %s  -- format "
					"see above\n ", c,  optarg);  
			exit(EXIT_FAILURE);
			break;
		}
	}

	rc = saNtfInitialize(&ntfHandle, &ntfCallbacks, &version);
	if (SA_AIS_OK != rc) {
		fprintf(stderr, "saNtfInitialize failed - %s\n",
			error_output(rc));
		exit(EXIT_FAILURE);
	}
	rc = readAll();
	if (SA_AIS_OK != rc) {
		exit(EXIT_FAILURE);
	}
	if (filterSecurityAlarm) {
		rc = saNtfNotificationFilterFree(
			fhdls.securityAlarmFilterHandle);
		if (SA_AIS_OK != rc) {
			fprintf(stderr, "saNtfNotificationFilterFree failed - "
					"%s\n", error_output(rc));
			exit(EXIT_FAILURE);
		}
	}
	if (filterAlarm) {
		rc = saNtfNotificationFilterFree(fhdls.alarmFilterHandle);
		if (SA_AIS_OK != rc) {
			fprintf(stderr, "saNtfNotificationFilterFree failed - "
				"%s\n", error_output(rc));
			exit(EXIT_FAILURE);
		}
	}
	rc = saNtfNotificationReadFinalize(readHandle);
	if (SA_AIS_OK != rc) {
		fprintf(stderr, "saNtfNotificationReadFinalize failed - %s\n",
			error_output(rc));
		exit(EXIT_FAILURE);
	}
	rc = saNtfFinalize(ntfHandle);
	if (SA_AIS_OK != rc) {
		fprintf(stderr, "saNtfFinalize failed - %s\n",
			error_output(rc));
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

