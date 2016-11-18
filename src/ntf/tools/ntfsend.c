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
/**
 *   Client command line program for the NTF Service
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
#include "osaf/saf/saAis.h"
#include "ntf/saf/saNtf.h"
#include "ntf/tools/ntfclient.h"
#include <ctype.h>
#include <limits.h>

/* Name of current testproxy (argv[0]) */
static char *progname;

/* Release code, major version, minor version */
SaVersionT version = { 'A', 0x01, 0x02 };

static char *error_output(SaAisErrorT result)
{
	static char error_result[256];

	(void)sprintf(error_result, "error: %u", result);
	return (error_result);
}

static SaAisErrorT fillInAdditionalInfo(SaNtfNotificationHandleT notificationHandle,
		SaNtfNotificationHeaderT *notificationHeader,
		saNotificationAdditionalInfoParamsT *additionalInfo)
{
	SaAisErrorT ret = SA_AIS_OK;
	SaUint16T infoLen = 0;
	SaStringT dest_ptr = NULL;
	int i = 0;

	for (; i < notificationHeader->numAdditionalInfo; i++) {
		notificationHeader->additionalInfo[i].infoId = additionalInfo[i].infoId;
		notificationHeader->additionalInfo[i].infoType = additionalInfo[i].infoType;

		switch (additionalInfo[i].infoType) {
		case SA_NTF_VALUE_STRING:
			infoLen = strlen(additionalInfo[i].strInfo) + 1;
			ret = saNtfPtrValAllocate(notificationHandle,
						infoLen,
						(void**)&dest_ptr,
						&(notificationHeader->additionalInfo[i].infoValue));
			if (ret == SA_AIS_OK) {
				(void)strncpy(dest_ptr, additionalInfo[i].strInfo, infoLen);
			} else {
				fprintf(stderr, "saNtfPtrValAllocate Error "
					"%d\n", ret);
				return ret;
			}
			break;

		default:
			fprintf(stderr, "additionalInfo TYPE(%d) is not supported\n",
					additionalInfo[i].infoType);
			exit(EXIT_FAILURE);
		}
	}
	return SA_AIS_OK;
}

static SaAisErrorT fill_header_part(SaNtfNotificationHandleT notificationHandle,
				SaNtfNotificationHeaderT *notificationHeader,
				saNotificationParamsT *notificationParams,
				SaUint16T lengthAdditionalText)
{
	*notificationHeader->eventType = notificationParams->eventType;
	*notificationHeader->eventTime = (SaTimeT)notificationParams->eventTime;

	*notificationHeader->notificationObject = notificationParams->notificationObject;

	*notificationHeader->notifyingObject = notificationParams->notifyingObject;

	/* vendor id 33333 is not an existing SNMP enterprise number.
	   Just an example */
	notificationHeader->notificationClassId->vendorId = notificationParams->notificationClassId.vendorId;

	/* sub id of this notification class within "name space" of vendor ID */
	notificationHeader->notificationClassId->majorId = notificationParams->notificationClassId.majorId;
	notificationHeader->notificationClassId->minorId = notificationParams->notificationClassId.minorId;

	/* set additional text */
	(void)strncpy(notificationHeader->additionalText, notificationParams->additionalText, lengthAdditionalText);

	/* Fill additional info */
	return fillInAdditionalInfo(notificationHandle, notificationHeader, notificationParams->additionalInfo);
}

static void usage(void)
{
	printf("\nNAME\n");
	printf("\t%s - send notification(s)\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [-s  perceivedSeverity]\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is a SAF NTF client used to send a notification.\n", progname);
	printf("\nOPTIONS\n");
	printf("  -T or --notificationType=0x1000...0x5000  numeric value of SaNtfNotificationTypeT\n");
	printf
	    ("                                            (obj_create_del=0x1000,attr_ch,state_ch,al,sec_al=0x5000)\n");
	printf("  -e or --eventType=4096...24589           numeric value of SaNtfEventTypeT\n");
	printf
	    ("                                            (SA_NTF_OBJECT_NOTIFICATIONS_START...SA_NTF_HPI_EVENT_OTHER)\n");
	printf("  -E or --eventTime=TIME                    numeric value of SaTimeT\n");
	printf("  -c or --notificationClassId=VE,MA,MI      vendorid, majorid, minorid\n");
	printf("  -n or --notificationObject=NOT_OBJ        notification object (string value)\n");
	printf("  -N or --notifyingObject=NOTIFY_OBJ        notififying object (string value)\n");
	printf("  -a or --additionalText=TEXT               additional text (string value)\n");
	printf("  -p or --probableCause=0..74               numeric value SaNtfProbableCauseT\n");
	printf("                                            SA_NTF_ADAPTER_ERROR to SA_NTF_UNSPECIFIED_REASON\n");
	printf("  -s or --perceivedSeverity=0...5           severity numeric value\n");
	printf("                                            (clear=0,ind,warn,min,maj,crit=5)\n");
	printf("  -r or --repeatSends=NUM                   send the same notifification NUM times\n");
	printf("  -b or --burstTimeout=TIME                 send burst of NUM repeatSends [default: 1] and sleep TIME (usec)\n"
               "                                            between each burst, will continue for ever\n");
	printf("  -i or --additionalInfo=ID,TYPE,VALUE      additional information\n"
               "                                            ID: SaNtfElementIdT integer value\n"
               "                                            TYPE: numeric value SaNtfValueTypeT, only SA_NTF_VALUE_STRING=11 is supported\n"
               "                                            VALUE: string value\n");
	printf("  -h or --help                              this help\n");
}

void getAdditionalInfo(saNotificationAdditionalInfoParamsT *ntfAdditionalInfo, char *para)
{
	long val;
	char *infoId = strtok(para, ",");
	char *infoType = strtok(NULL, ",");
	char *infoValue = strtok(NULL, "\0");
	SaUint16T infoLen = 0;

	if (NULL == infoId || NULL == infoType || NULL == infoValue) {
		fprintf(stderr, "notificationAdditionalInfo wrong format\n");
		exit(EXIT_FAILURE);
	}
	if (get_long_digit(infoId, &val)) {
		ntfAdditionalInfo->infoId = (SaNtfElementIdT)val;
	} else {
		fprintf(stderr, "additionalInfo ID wrong format\n");
		exit(EXIT_FAILURE);
	}
	if (get_long_digit(infoType, &val)) {
		if ((val > SA_NTF_VALUE_ARRAY) || (val < SA_NTF_VALUE_UINT8)) {
			fprintf(stderr, "invalid value for additionalInfo TYPE(%u)\n", (SaUint16T)val);
			usage();
			exit(EXIT_FAILURE);
		}

		ntfAdditionalInfo->infoType = (SaNtfValueTypeT)val;
		switch (ntfAdditionalInfo->infoType) {
		case SA_NTF_VALUE_STRING:
			infoLen = strlen(infoValue) + 1;
			ntfAdditionalInfo->strInfo = (SaStringT)malloc(infoLen);
			(void)strncpy(ntfAdditionalInfo->strInfo, infoValue, infoLen);
			break;
		default:
			fprintf(stderr, "additionalInfo TYPE(%d) is not supported\n",
					ntfAdditionalInfo->infoType);
			usage();
			exit(EXIT_FAILURE);
		}
	} else {
		fprintf(stderr, "additionalInfo TYPE wrong format\n");
		usage();
		exit(EXIT_FAILURE);
	}
}

static void fillInDefaultValues(saNotificationAllocationParamsT *notificationAllocationParams,
				saNotificationFilterAllocationParamsT *notificationFilterAllocationParams,
				saNotificationParamsT *notificationParams)
{
	/* Default notification allocation parameters */
	/* Common notification header */
	notificationAllocationParams->numCorrelatedNotifications = 0;
	notificationAllocationParams->lengthAdditionalText = (SaUint16T)(strlen(DEFAULT_ADDITIONAL_TEXT) + 1);
	notificationAllocationParams->numAdditionalInfo = 0;

	/* Alarm specific */
	notificationAllocationParams->numSpecificProblems = 0;
	notificationAllocationParams->numMonitoredAttributes = 0;
	notificationAllocationParams->numProposedRepairActions = 0;

	/* State change specific */
	notificationAllocationParams->numStateChanges = DEFAULT_NUMBER_OF_CHANGED_STATES;

	/* Object Create/Delete specific */
	notificationAllocationParams->numObjectAttributes = DEFAULT_NUMBER_OF_OBJECT_ATTRIBUTES;

	/* Attribute Change specific */
	notificationAllocationParams->numAttributes = DEFAULT_NUMBER_OF_CHANGED_ATTRIBUTES;

	notificationParams->changedStates[0].stateId = MY_APP_OPER_STATE;
	notificationParams->changedStates[0].oldStatePresent = SA_FALSE;
	notificationParams->changedStates[0].newState = SA_NTF_DISABLED;

	notificationParams->changedStates[1].stateId = MY_APP_USAGE_STATE;
	notificationParams->changedStates[1].oldStatePresent = SA_FALSE;
	notificationParams->changedStates[1].newState = SA_NTF_IDLE;

	notificationParams->changedStates[2].stateId = MY_APP_OPER_STATE;
	notificationParams->changedStates[2].oldStatePresent = SA_TRUE;
	notificationParams->changedStates[2].oldState = SA_NTF_DISABLED;
	notificationParams->changedStates[2].newState = SA_NTF_ENABLED;

	notificationParams->changedStates[3].stateId = MY_APP_USAGE_STATE;
	notificationParams->changedStates[3].oldStatePresent = SA_TRUE;
	notificationParams->changedStates[3].oldState = SA_NTF_IDLE;
	notificationParams->changedStates[3].newState = SA_NTF_ACTIVE;

	notificationParams->objectAttributes[0].attributeId = 58;
	notificationParams->objectAttributes[0].attributeType = SA_NTF_VALUE_INT32;
	notificationParams->objectAttributes[0].attributeValue.int32Val = 1;

	notificationParams->objectAttributes[1].attributeId = 4;
	notificationParams->objectAttributes[1].attributeType = SA_NTF_VALUE_UINT32;
	notificationParams->objectAttributes[1].attributeValue.int32Val = 16;

	notificationParams->changedAttributes[0].attributeId = 0;
	notificationParams->changedAttributes[0].attributeType = SA_NTF_VALUE_INT32;
	notificationParams->changedAttributes[0].oldAttributePresent = SA_FALSE;
	notificationParams->changedAttributes[0].newAttributeValue.int32Val = 1;

	notificationParams->changedAttributes[1].attributeId = 1;
	notificationParams->changedAttributes[1].attributeType = SA_NTF_VALUE_INT32;
	notificationParams->changedAttributes[1].oldAttributePresent = SA_TRUE;
	notificationParams->changedAttributes[1].oldAttributeValue.int32Val = 8;
	notificationParams->changedAttributes[1].newAttributeValue.int32Val = -4;

	/* Misc */
	notificationAllocationParams->variableDataSize = SA_NTF_ALLOC_SYSTEM_LIMIT;

	/* Default notification filter allocation parameters */
	notificationFilterAllocationParams->numEventTypes = 0;
	notificationFilterAllocationParams->numNotificationObjects = 0;
	notificationFilterAllocationParams->numNotifyingObjects = 0;
	notificationFilterAllocationParams->numNotificationClassIds = 0;

	/* Alarm specific */
	notificationFilterAllocationParams->numProbableCauses = 0;
	notificationFilterAllocationParams->numPerceivedSeverities = 2;
	notificationFilterAllocationParams->numTrends = 0;

	/* Default notification parameters */
	notificationParams->additionalText = (SaStringT)malloc(notificationAllocationParams->lengthAdditionalText);

	notificationParams->notificationType = SA_NTF_TYPE_ALARM;

	(void)strncpy(notificationParams->additionalText,
		      DEFAULT_ADDITIONAL_TEXT, notificationAllocationParams->lengthAdditionalText);
	saAisNameLend(DEFAULT_NOTIFICATION_OBJECT, &notificationParams->notificationObject);
	saAisNameLend(DEFAULT_NOTIFYING_OBJECT, &notificationParams->notifyingObject);
	notificationParams->notificationClassId.vendorId = ERICSSON_VENDOR_ID;
	notificationParams->notificationClassId.majorId = 0;
	notificationParams->notificationClassId.minorId = 0;
	notificationParams->eventTime = (SaTimeT)SA_TIME_UNKNOWN;

	/* Alarm specific */
	notificationParams->probableCause = SA_NTF_BANDWIDTH_REDUCED;
	notificationParams->perceivedSeverity = SA_NTF_SEVERITY_WARNING;
	notificationParams->alarmEventType = SA_NTF_ALARM_COMMUNICATION;

	/* State change specific */
	notificationParams->stateChangeSourceIndicator = SA_NTF_OBJECT_OPERATION;
	notificationParams->stateChangeEventType = SA_NTF_OBJECT_STATE_CHANGE;

	/* Object Create Delete specific */
	notificationParams->objectCreateDeleteSourceIndicator = SA_NTF_UNKNOWN_OPERATION;
	notificationParams->objectCreateDeleteEventType = SA_NTF_OBJECT_CREATION;

	/* Attribute Change params */
	notificationParams->attributeChangeSourceIndicator = SA_NTF_UNKNOWN_OPERATION;
	notificationParams->attributeChangeEventType = SA_NTF_ATTRIBUTE_ADDED;

	/* Security Alarm params */
	notificationParams->securityAlarmEventType = SA_NTF_INTEGRITY_VIOLATION;
	notificationParams->severity = SA_NTF_SEVERITY_CRITICAL;
	notificationParams->securityAlarmProbableCause = SA_NTF_UNAUTHORIZED_ACCESS_ATTEMPT;
	notificationParams->securityAlarmDetector.valueType = SA_NTF_VALUE_INT32;
	notificationParams->securityAlarmDetector.value.int32Val = 1;
	notificationParams->serviceProvider.valueType = SA_NTF_VALUE_INT32;
	notificationParams->serviceProvider.value.int32Val = 2;
	notificationParams->serviceUser.valueType = SA_NTF_VALUE_INT32;
	notificationParams->serviceUser.value.int32Val = 3;

	/* One day */
	notificationParams->timeout = (24 * 3600);
	notificationParams->burstTimeout = 0;

	/* send the same message repeatSends times */
	notificationParams->repeateSends = 1;
	notificationParams->subscriptionId = 1;
}

static SaAisErrorT
sendNotification(const saNotificationAllocationParamsT *notificationAllocationParams,
		 saNotificationParamsT *notificationParams, const saNotificationFlagsT *notificationFlags)
{
	SaInt32T i;
	SaAisErrorT errorCode;
	SaNtfHandleT ntfHandle;
	SaNtfIdentifierT ntfId;
	SaNtfNotificationHandleT myNotificationHandle;

	/* Instantiate an alarm notification struct */
	SaNtfAlarmNotificationT myAlarmNotification;

	/* Instantiate a state change notification struct */
	SaNtfStateChangeNotificationT myStateChangeNotification;

	/* Instantiate an object create/delete notification struct */
	SaNtfObjectCreateDeleteNotificationT myObjectCreateDeleteNotification;

	/* Instantiate an attribute change notification struct */
	SaNtfAttributeChangeNotificationT myAttributeChangeNotification;

	/* Instantiate a security alarm notification struct */
	SaNtfSecurityAlarmNotificationT mySecurityAlarmNotification;

	unsigned int repeat = notificationParams->repeateSends;

	do {
		errorCode = ntftool_saNtfInitialize(&ntfHandle, NULL, &version);
		if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
			(void)printf("ntftool_saNtfInitialize %s\n", error_output(errorCode));
			return errorCode;
		}
		if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
			usleep(100000);
		}
	} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

	switch (notificationParams->notificationType) {
	case SA_NTF_TYPE_ALARM:
		(void)printf("Send Alarm Notification!\n");
		if (*notificationFlags == DEFAULT_FLAG) {
			notificationParams->eventType = notificationParams->alarmEventType;
		}

		do {
			/* Allocate */
			errorCode = saNtfAlarmNotificationAllocate(ntfHandle,	/* handle to Notification Service instance */
								   &myAlarmNotification,
								   /* number of correlated notifications */
								   notificationAllocationParams->
								   numCorrelatedNotifications,
								   /* length of additional text */
								   notificationAllocationParams->lengthAdditionalText,
								   /* number of additional info items */
								   notificationAllocationParams->numAdditionalInfo,
								   /* number of specific problems */
								   notificationAllocationParams->numSpecificProblems,
								   /* number of monitored attributes */
								   notificationAllocationParams->numMonitoredAttributes,
								   /* number of proposed repair actions */
								   notificationAllocationParams->
								   numProposedRepairActions,
								   /* use default allocation size */
								   notificationAllocationParams->variableDataSize);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("saNtfAlarmNotificationAllocate %s\n", error_output(errorCode));
				return errorCode;
			} else if (SA_AIS_OK == errorCode) {
				errorCode = fill_header_part(myAlarmNotification.notificationHandle,
						&myAlarmNotification.notificationHeader,
						(saNotificationParamsT *)notificationParams,
						notificationAllocationParams->lengthAdditionalText);

				if (SA_AIS_OK != errorCode ) {
					myNotificationHandle = myAlarmNotification.notificationHandle;
					goto fillHeaderFail;
				}

				/* determine perceived severity */
				*(myAlarmNotification.perceivedSeverity) = notificationParams->perceivedSeverity;

				/* set probable cause */
				*(myAlarmNotification.probableCause) = notificationParams->probableCause;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);
		break;

	case SA_NTF_TYPE_STATE_CHANGE:
		(void)printf("Send State Change Notification!\n");

		if (*notificationFlags == (saNotificationFlagsT)DEFAULT_FLAG) {
			notificationParams->eventType = notificationParams->stateChangeEventType;
		}

		do {
			/* Allocate */
			errorCode = saNtfStateChangeNotificationAllocate(ntfHandle,	/* handle to Notification Service instance */
									 &myStateChangeNotification,
									 /* number of correlated notifications */
									 notificationAllocationParams->
									 numCorrelatedNotifications,
									 /* length of additional text */
									 notificationAllocationParams->
									 lengthAdditionalText,
									 /* number of additional info items */
									 notificationAllocationParams->
									 numAdditionalInfo,
									 /* number of state changes */
									 notificationAllocationParams->numStateChanges,
									 /* use default allocation size */
									 notificationAllocationParams->
									 variableDataSize);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("saNtfStateChangeNotificationAllocate %s\n", error_output(errorCode));
				return errorCode;
			} else if (SA_AIS_OK == errorCode) {
				errorCode = fill_header_part(myStateChangeNotification.notificationHandle,
						&myStateChangeNotification.notificationHeader,
						(saNotificationParamsT *)notificationParams,
						notificationAllocationParams->lengthAdditionalText);

				if (SA_AIS_OK != errorCode ) {
					myNotificationHandle = myStateChangeNotification.notificationHandle;
					goto fillHeaderFail;
				}

				/* set source indicator */
				*(myStateChangeNotification.sourceIndicator) =
				    notificationParams->stateChangeSourceIndicator;

				/* set states */
				for (i = 0; i < notificationAllocationParams->numStateChanges; i++) {
					myStateChangeNotification.changedStates[i].newState =
					    notificationParams->changedStates[i].newState;
					myStateChangeNotification.changedStates[i].oldState =
					    notificationParams->changedStates[i].oldState;
					myStateChangeNotification.changedStates[i].oldStatePresent =
					    notificationParams->changedStates[i].oldStatePresent;
					myStateChangeNotification.changedStates[i].stateId =
					    notificationParams->changedStates[i].stateId;
				}
				if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
					usleep(100000);
				}
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);
		break;

	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		(void)printf("Send Object Create/Delete Notification!\n");
		if (*notificationFlags == (saNotificationFlagsT)DEFAULT_FLAG) {
			notificationParams->eventType = notificationParams->objectCreateDeleteEventType;
		}

		do {

			errorCode = saNtfObjectCreateDeleteNotificationAllocate(ntfHandle,	/* handle to Notification Service instance */
										&myObjectCreateDeleteNotification,
										/* number of correlated notifications */
										notificationAllocationParams->
										numCorrelatedNotifications,
										/* length of additional text */
										notificationAllocationParams->
										lengthAdditionalText,
										/* number of additional info items */
										notificationAllocationParams->
										numAdditionalInfo,
										/* number of state changes */
										notificationAllocationParams->
										numObjectAttributes,
										/* use default allocation size */
										notificationAllocationParams->
										variableDataSize);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("saNtfObjectCreateDeleteNotificationAllocate %s\n",
					     error_output(errorCode));
				return errorCode;
			} else if (SA_AIS_OK == errorCode) {

				errorCode = fill_header_part(myObjectCreateDeleteNotification.notificationHandle,
						&myObjectCreateDeleteNotification.notificationHeader,
						(saNotificationParamsT *)notificationParams,
						notificationAllocationParams->lengthAdditionalText);

				if (SA_AIS_OK != errorCode ) {
					myNotificationHandle = myObjectCreateDeleteNotification.notificationHandle;
					goto fillHeaderFail;
				}

				/* Set source indicator */
				*(myObjectCreateDeleteNotification.sourceIndicator) =
				    notificationParams->objectCreateDeleteSourceIndicator;

				/* Set object attributes */
				for (i = 0; i < notificationAllocationParams->numObjectAttributes; i++) {
					myObjectCreateDeleteNotification.objectAttributes[i].attributeId =
					    notificationParams->objectAttributes[i].attributeId;
					myObjectCreateDeleteNotification.objectAttributes[i].attributeType =
					    notificationParams->objectAttributes[i].attributeType;
					myObjectCreateDeleteNotification.objectAttributes[i].attributeValue.int32Val =
					    notificationParams->objectAttributes[i].attributeValue.int32Val;
				}
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		break;

	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		(void)printf("Send Attribute Change Notification!\n");
		if (*notificationFlags == (saNotificationFlagsT)DEFAULT_FLAG) {
			notificationParams->eventType = notificationParams->attributeChangeEventType;
		}

		do {

			errorCode = saNtfAttributeChangeNotificationAllocate(ntfHandle,	/* handle to Notification Service instance */
									     &myAttributeChangeNotification,
									     /* number of correlated notifications */
									     notificationAllocationParams->
									     numCorrelatedNotifications,
									     /* length of additional text */
									     notificationAllocationParams->
									     lengthAdditionalText,
									     /* number of additional info items */
									     notificationAllocationParams->
									     numAdditionalInfo,
									     /* number of state changes */
									     notificationAllocationParams->
									     numAttributes,
									     /* use default allocation size */
									     notificationAllocationParams->
									     variableDataSize);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("saNtfAttributeChangeNotificationAllocate %s\n", error_output(errorCode));
				return errorCode;
			} else if (SA_AIS_OK == errorCode) {

				errorCode = fill_header_part(myAttributeChangeNotification.notificationHandle,
						&myAttributeChangeNotification.notificationHeader,
						(saNotificationParamsT *)notificationParams,
						notificationAllocationParams->lengthAdditionalText);

				if (SA_AIS_OK != errorCode ) {
					myNotificationHandle = myAttributeChangeNotification.notificationHandle;
					goto fillHeaderFail;
				}

				/* set source indicator */
				*(myAttributeChangeNotification.sourceIndicator) =
				    notificationParams->attributeChangeSourceIndicator;

				/* Set object attributes */
				for (i = 0; i < notificationAllocationParams->numAttributes; i++) {
					myAttributeChangeNotification.changedAttributes[i].attributeId =
					    notificationParams->changedAttributes[i].attributeId;
					myAttributeChangeNotification.changedAttributes[i].oldAttributePresent =
					    notificationParams->changedAttributes[i].oldAttributePresent;
					myAttributeChangeNotification.changedAttributes[i].attributeType =
					    notificationParams->changedAttributes[i].attributeType;

					if (notificationParams->changedAttributes[i].oldAttributePresent) {
						myAttributeChangeNotification.changedAttributes[i].oldAttributeValue.
						    int32Val =
						    notificationParams->changedAttributes[i].oldAttributeValue.int32Val;
					}

					myAttributeChangeNotification.changedAttributes[i].newAttributeValue.int32Val =
					    notificationParams->changedAttributes[i].newAttributeValue.int32Val;
				}
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		break;

	case SA_NTF_TYPE_SECURITY_ALARM:
		(void)printf("Send Security Alarm Notification!\n");
		if (*notificationFlags == (saNotificationFlagsT)DEFAULT_FLAG) {
			notificationParams->eventType = notificationParams->securityAlarmEventType;
		}

		do {

			errorCode = saNtfSecurityAlarmNotificationAllocate(ntfHandle,	/* handle to Notification Service instance */
									   &mySecurityAlarmNotification,
									   /* number of correlated notifications */
									   notificationAllocationParams->
									   numCorrelatedNotifications,
									   /* length of additional text */
									   notificationAllocationParams->
									   lengthAdditionalText,
									   /* number of additional info items */
									   notificationAllocationParams->
									   numAdditionalInfo,
									   /* use default allocation size */
									   notificationAllocationParams->
									   variableDataSize);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("saNtfSecurityAlarmNotificationAllocate %s\n", error_output(errorCode));
				return errorCode;
			} else if (SA_AIS_OK == errorCode) {

				errorCode = fill_header_part(mySecurityAlarmNotification.notificationHandle,
						&mySecurityAlarmNotification.notificationHeader,
						(saNotificationParamsT *)notificationParams,
						notificationAllocationParams->lengthAdditionalText);

				if (SA_AIS_OK != errorCode ) {
					myNotificationHandle = mySecurityAlarmNotification.notificationHandle;
					goto fillHeaderFail;
				}

				*(mySecurityAlarmNotification.probableCause) = notificationParams->probableCause;

				*(mySecurityAlarmNotification.severity) = notificationParams->severity;

				mySecurityAlarmNotification.serviceUser->valueType =
				    notificationParams->serviceUser.valueType;
				mySecurityAlarmNotification.serviceUser->value.int32Val =
				    notificationParams->serviceUser.value.int32Val;

				mySecurityAlarmNotification.serviceProvider->valueType =
				    notificationParams->serviceProvider.valueType;
				mySecurityAlarmNotification.serviceProvider->value.int32Val =
				    notificationParams->serviceProvider.value.int32Val;

				mySecurityAlarmNotification.securityAlarmDetector->valueType =
				    notificationParams->securityAlarmDetector.valueType;
				mySecurityAlarmNotification.securityAlarmDetector->value.int32Val =
				    notificationParams->securityAlarmDetector.value.int32Val;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		break;

	default:
		printf("Unknown notification type!!!\n");
		break;
	}

 repeatedSend:

	switch (notificationParams->notificationType) {
	case SA_NTF_TYPE_ALARM:

		/* Send the alarm notification */
		do {

			errorCode = ntftool_saNtfNotificationSend(myAlarmNotification.notificationHandle);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("ntftool_saNtfNotificationSend %s\n", error_output(errorCode));
				return errorCode;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		ntfId = *(myAlarmNotification.notificationHeader.notificationId);
		(void)printf("NotificationId: %d\n", (SaInt32T)ntfId);

		if (--repeat) {
			goto repeatedSend;
		}

		if (notificationParams->burstTimeout) {
			usleep(notificationParams->burstTimeout);
			repeat = notificationParams->repeateSends;
			goto repeatedSend;
		}

		do {
			errorCode = saNtfNotificationFree(myAlarmNotification.notificationHandle);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("saNtfNotificationFree %s\n", error_output(errorCode));
				return errorCode;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		break;
	case SA_NTF_TYPE_STATE_CHANGE:
		/* Send the state change notification */

		do {
			errorCode = ntftool_saNtfNotificationSend(myStateChangeNotification.notificationHandle);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("ntftool_saNtfNotificationSend %s\n", error_output(errorCode));
				return errorCode;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		ntfId = *(myStateChangeNotification.notificationHeader.notificationId);
		(void)printf("sent notification id: %d\n", (SaInt32T)ntfId);

		if (--repeat) {
			goto repeatedSend;
		}

		if (notificationParams->burstTimeout) {
			usleep(notificationParams->burstTimeout);
			repeat = notificationParams->repeateSends;
			goto repeatedSend;
		}

		do {
			errorCode = saNtfNotificationFree(myStateChangeNotification.notificationHandle);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("saNtfNotificationFree %s\n", error_output(errorCode));
				return errorCode;
			}
			usleep(100000);
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		break;

	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		/* Send the object create/delete notification */

		do {

			errorCode = ntftool_saNtfNotificationSend(myObjectCreateDeleteNotification.notificationHandle);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("ntftool_saNtfNotificationSend %s\n", error_output(errorCode));
				return errorCode;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		ntfId = *(myObjectCreateDeleteNotification.notificationHeader.notificationId);
		(void)printf("NotificationId: %d\n", (SaInt32T)ntfId);

		if (--repeat) {
			goto repeatedSend;
		}

		if (notificationParams->burstTimeout) {
			usleep(notificationParams->burstTimeout);
			repeat = notificationParams->repeateSends;
			goto repeatedSend;
		}

		do {

			errorCode = saNtfNotificationFree(myObjectCreateDeleteNotification.notificationHandle);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("saNtfNotificationFree %s\n", error_output(errorCode));
				return errorCode;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		break;

	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		/* Send the attribute change notification */
		do {
			errorCode = ntftool_saNtfNotificationSend(myAttributeChangeNotification.notificationHandle);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("ntftool_saNtfNotificationSend %s\n", error_output(errorCode));
				return errorCode;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		ntfId = *(myAttributeChangeNotification.notificationHeader.notificationId);
		(void)printf("NotificationId: %d\n", (SaInt32T)ntfId);

		if (--repeat) {
			goto repeatedSend;
		}
		if (notificationParams->burstTimeout) {
			usleep(notificationParams->burstTimeout);
			repeat = notificationParams->repeateSends;
			goto repeatedSend;
		}

		do {

			errorCode = saNtfNotificationFree(myAttributeChangeNotification.notificationHandle);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("saNtfNotificationFree %s\n", error_output(errorCode));
				return errorCode;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		break;

	case SA_NTF_TYPE_SECURITY_ALARM:	/* Send the state change notification */

		do {
			errorCode = ntftool_saNtfNotificationSend(mySecurityAlarmNotification.notificationHandle);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("ntftool_saNtfNotificationSend %s\n", error_output(errorCode));
				return errorCode;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		ntfId = *(mySecurityAlarmNotification.notificationHeader.notificationId);
		(void)printf("NotificationId: %d\n", (SaInt32T)ntfId);

		if (--repeat) {
			goto repeatedSend;
		}
		if (notificationParams->burstTimeout) {
			usleep(notificationParams->burstTimeout);
			repeat = notificationParams->repeateSends;
			goto repeatedSend;
		}

		do {
			errorCode = saNtfNotificationFree(mySecurityAlarmNotification.notificationHandle);
			if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
				(void)printf("saNtfNotificationFree %s\n", error_output(errorCode));
				return errorCode;
			}
			if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
				usleep(100000);
			}
		} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

		break;

	default:
		printf("Unknown notification type!");
		break;
	}

	do {
		errorCode = saNtfFinalize(ntfHandle);
		if (SA_AIS_OK != errorCode && SA_AIS_ERR_TRY_AGAIN != errorCode) {
			(void)printf("saNtfFinalize %s\n", error_output(errorCode));
			return errorCode;
		}
		if (SA_AIS_ERR_TRY_AGAIN == errorCode) {
			usleep(100000);
		}
	} while (SA_AIS_ERR_TRY_AGAIN == errorCode);

	return SA_AIS_OK;


fillHeaderFail:
	(void)printf("Fill header failed %s\n", error_output(errorCode));
	saNtfNotificationFree(myNotificationHandle);
	return errorCode;
}

int main(int argc, char *argv[])
{
	long value;
	long long valuell;
	char *endptr;
	int current_option;
	int i;
	SaBoolT optionFlag = SA_FALSE;

	/* Parameter stuct instances */
	saNotificationAllocationParamsT myNotificationAllocationParams;
	saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
	saNotificationParamsT myNotificationParams;
	saNotificationFlagsT myNotificationFlags = DEFAULT_FLAG;
	bool nType = false;

	static struct option long_options[] = {
		{"additionalText", required_argument, 0, 'a'},
		{"notificationType", required_argument, 0, 'T'},
		{"notificationClassId", required_argument, 0, 'c'},
		{"notificationObject", required_argument, 0, 'n'},
		{"notifyingObject", required_argument, 0, 'N'},
		{"perceivedSeverity", required_argument, 0, 's'},
		{"probableCause", required_argument, 0, 'p'},
		{"repeatSends", required_argument, 0, 'r'},
		{"eventType", required_argument, 0, 'e'},
		{"eventTime", required_argument, 0, 'E'},
		{"burstTimeout", required_argument, 0, 'b'},
		{"additionalInfo", required_argument, 0, 'i'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	if (setenv("SA_ENABLE_EXTENDED_NAMES", "1", 1) != 0) {
		LOG_ER("Failed to enable Extended SaNameT");
		exit(EXIT_FAILURE);
	}
	
	fillInDefaultValues(&myNotificationAllocationParams,
			    &myNotificationFilterAllocationParams, &myNotificationParams);

	progname = argv[0];

	if (argc >= 1) {
		/* Check options */
		while ((current_option = getopt_long(argc, argv, "a:c:e:E:N:n:p:r:s:b:T:i:h", long_options, NULL)) != -1) {
			optionFlag = SA_TRUE;
			switch (current_option) {
			case 'a':
				free(myNotificationParams.additionalText);

				myNotificationAllocationParams.lengthAdditionalText = (SaUint16T)(strlen(optarg) + 1);

				myNotificationParams.additionalText =
				    (SaStringT)malloc(myNotificationAllocationParams.lengthAdditionalText);

				(void)strncpy(myNotificationParams.additionalText,
					      optarg, myNotificationAllocationParams.lengthAdditionalText);

				break;
			case 'b':
				myNotificationParams.burstTimeout = (SaInt32T)atoi(optarg);
				break;
			case 'c':
				getVendorId(&myNotificationParams.notificationClassId);
				break;
			case 'n':
				if (strlen(optarg) > kOsafMaxDnLength) {
					fprintf(stderr, "notificationObject too long\n");
					exit(EXIT_FAILURE);
				}			
				saAisNameLend(optarg, &myNotificationParams.notificationObject);
				break;
			case 'N':
				if (strlen(optarg) > kOsafMaxDnLength) {
					fprintf(stderr, "notifyingObject too long\n");
					exit(EXIT_FAILURE);
				}				
				saAisNameLend(optarg, &myNotificationParams.notifyingObject);
				break;
			case 'e':
				myNotificationParams.eventType = (SaNtfEventTypeT)atoi(optarg);
				/* No default value */
				myNotificationFlags = 0x0000;
				if ((myNotificationParams.eventType < SA_NTF_OBJECT_NOTIFICATIONS_START) ||
						(myNotificationParams.eventType > SA_NTF_HPI_EVENT_OTHER)) {
					fprintf(stderr,"invalid value for eventType\n");
					exit(EXIT_FAILURE);
				}
	
				break;
			case 'E':
				if (get_long_long_digit(optarg, &valuell)) {
					myNotificationParams.eventTime = valuell;
				} else {
					fprintf(stderr,"invalid input for time\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'p':
				myNotificationParams.probableCause = (SaNtfProbableCauseT)atoi(optarg);
				if ((myNotificationParams.probableCause < SA_NTF_ADAPTER_ERROR) ||
					(myNotificationParams.probableCause > SA_NTF_UNSPECIFIED_REASON)) {
					fprintf(stderr,"invalid value for probableCause\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'r':
				if (get_long_digit(optarg, &value)) {
					myNotificationParams.repeateSends = (unsigned int) value;
					if (!myNotificationParams.repeateSends) {
						fprintf(stderr, "zero for repeateSends not allowed\n");
						exit(EXIT_FAILURE);
					}
				} else {
					fprintf(stderr, "repeatSends wrong format\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 's':
				myNotificationParams.perceivedSeverity = (SaNtfSeverityT)atoi(optarg);
				myNotificationParams.severity = (SaNtfSeverityT)atoi(optarg);
				if ((myNotificationParams.severity < SA_NTF_SEVERITY_CLEARED) ||
						(myNotificationParams.severity > SA_NTF_SEVERITY_CRITICAL)) {
					fprintf(stderr,"invalid value for perceivedSeverity/severity\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'T':
				value = (SaNtfNotificationTypeT)strtol(optarg, &endptr, 16);
				if ((value != SA_NTF_TYPE_OBJECT_CREATE_DELETE) && 
						(value != SA_NTF_TYPE_ATTRIBUTE_CHANGE) && 
						(value != SA_NTF_TYPE_STATE_CHANGE) && 
						(value != SA_NTF_TYPE_ALARM) && 
						(value != SA_NTF_TYPE_SECURITY_ALARM)) {
					fprintf(stderr,"invalid value for notificationType\n");
					exit(EXIT_FAILURE);
				}
				myNotificationParams.notificationType = value;
				nType = true;
				break;
			case 'i':
				if (myNotificationAllocationParams.numAdditionalInfo >= MAX_NUMBER_OF_ADDITIONAL_INFO) {
					fprintf(stderr, "Only one additionalInfo is supported\n");
					exit(EXIT_FAILURE);
				}
				getAdditionalInfo(&myNotificationParams.additionalInfo[myNotificationAllocationParams.numAdditionalInfo], optarg);
				myNotificationAllocationParams.numAdditionalInfo++;
				break;
			case ':':
				(void)printf("Option -%c requires an argument!!!!\n", optopt);
				usage();
				exit(EXIT_FAILURE);
				break;
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
				break;
			case '?':
			default:
				fprintf(stderr, "Try '%s -h' for more information. \n", argv[0]);
				exit(EXIT_FAILURE);
				break;
			}
		}
		if (optind < argc) {
			fprintf(stderr, "Invalid non-option: \n");
			while (optind < argc)
				fprintf(stderr, "%s \n", argv[optind++]);
			fprintf(stderr, "Try '%s -h' for more information. \n", argv[0]);
			exit(EXIT_FAILURE);
		}
		if ((optionFlag == SA_FALSE) && (argc >= 3)) {
			usage();
			exit(EXIT_FAILURE);
		} else if ((nType == true) && (myNotificationFlags != DEFAULT_FLAG)) {
			if (validate_nType_eType(myNotificationParams.notificationType, 
						myNotificationParams.eventType) == false) {
				fprintf(stderr,"invalid combination of notificationType and eventType\n");
				exit(EXIT_FAILURE);
			}
		} else if ((nType == false) && (myNotificationFlags != DEFAULT_FLAG )) {
			// Adjust the value of notificationType
			set_nType_for_eType(&myNotificationParams.notificationType,
					&myNotificationParams.eventType); 
		}
		if (sendNotification(&myNotificationAllocationParams,
					&myNotificationParams, &myNotificationFlags) != SA_AIS_OK)
			exit(EXIT_FAILURE);
	} else {
		usage();
		exit(EXIT_FAILURE);
	}
	free(myNotificationParams.additionalText);

	for (i = 0; i < myNotificationAllocationParams.numAdditionalInfo; i++) {
		if (myNotificationParams.additionalInfo[i].strInfo) {
			free(myNotificationParams.additionalInfo[i].strInfo);
			myNotificationParams.additionalInfo[i].strInfo = NULL;
		}
	}

	exit(EXIT_SUCCESS);
}
