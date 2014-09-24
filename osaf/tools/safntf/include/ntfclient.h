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
 *   Client header file.
 */

#ifndef NTFCLIENT_H
#define NTFCLIENT_H

#include <saNtf.h>
#include "ntfsv_msg.h"

/* Defines */
#define DEFAULT_FLAG 0x0001
/* #define MAX_NUMBER_OF_STATE_CHANGES 5 */
#define DEFAULT_NUMBER_OF_CHANGED_STATES 4
#define DEFAULT_NUMBER_OF_OBJECT_ATTRIBUTES 2
#define DEFAULT_NUMBER_OF_CHANGED_ATTRIBUTES 2

/* TODO: decide max values ??*/
#define MAX_NUMBER_OF_STATE_CHANGES 1000
#define MAX_NUMBER_OF_CHANGED_ATTRIBUTES 1000
#define MAX_NUMBER_OF_OBJECT_ATTRIBUTES  1000

typedef enum {
	MY_APP_OPER_STATE = 1,
	MY_APP_USAGE_STATE = 2
} saNtfStateIdT;

typedef enum {
	SA_NTF_DISABLED,
	SA_NTF_ENABLED,
	SA_NTF_IDLE,
	SA_NTF_ACTIVE
} saNtfStatesT;

/* Internal structs */
typedef struct {
	SaUint16T numEventTypes;
	SaUint16T numNotificationObjects;
	SaUint16T numNotifyingObjects;
	SaUint16T numNotificationClassIds;
	SaUint32T numProbableCauses;
	SaUint32T numPerceivedSeverities;
	SaUint32T numTrends;
} saNotificationFilterAllocationParamsT;

typedef struct {
	SaUint16T numCorrelatedNotifications;
	SaUint16T lengthAdditionalText;
	SaUint16T numAdditionalInfo;
	SaUint16T numSpecificProblems;
	SaUint16T numMonitoredAttributes;
	SaUint16T numProposedRepairActions;
	SaUint16T numStateChanges;
	/* Object Create/Delete specific */
	SaUint16T numObjectAttributes;

	/* Attribute Change specific */
	SaUint16T numAttributes;

	SaInt16T variableDataSize;
} saNotificationAllocationParamsT;

typedef struct {
	/* SaNtfSeverityTrendT trend; */
	SaStringT additionalText;
	SaNtfProbableCauseT probableCause;
	SaNtfSeverityT perceivedSeverity;
	SaNtfStateChangeT changedStates[MAX_NUMBER_OF_STATE_CHANGES];
	SaNtfEventTypeT eventType;
	SaNtfEventTypeT alarmEventType;
	SaNtfEventTypeT stateChangeEventType;
	SaNtfEventTypeT objectCreateDeleteEventType;
	SaNtfEventTypeT attributeChangeEventType;
	SaNtfEventTypeT securityAlarmEventType;
	SaNtfNotificationTypeT notificationType;
	SaNameT notificationObject;
	SaNameT notifyingObject;
	SaNtfClassIdT notificationClassId;
	SaTimeT eventTime;
	SaNtfIdentifierT notificationId;
	SaNtfSubscriptionIdT subscriptionId;
	SaNtfSourceIndicatorT stateChangeSourceIndicator;

	/* Object Create Delete Specific */
	SaNtfSourceIndicatorT objectCreateDeleteSourceIndicator;
	SaNtfAttributeT objectAttributes[MAX_NUMBER_OF_OBJECT_ATTRIBUTES];
	SaNtfValueTypeT attributeType;

	/* Attribute Change Specific */
	SaNtfSourceIndicatorT attributeChangeSourceIndicator;
	SaNtfAttributeChangeT changedAttributes[
		MAX_NUMBER_OF_CHANGED_ATTRIBUTES];

	/* Security Alarm Specific */
	SaNtfProbableCauseT securityAlarmProbableCause;
	SaNtfSeverityT severity;
	SaNtfSecurityAlarmDetectorT securityAlarmDetector;
	SaNtfServiceUserT serviceProvider;
	SaNtfServiceUserT serviceUser;

	SaInt32T timeout;
	SaInt32T burstTimeout;
	unsigned int repeateSends;
} saNotificationParamsT;

typedef SaUint16T saNotificationFlagsT;

#define DEFAULT_ADDITIONAL_TEXT "default additional text"
#define DEFAULT_NOTIFICATION_OBJECT "default notification object"
#define DEFAULT_NOTIFYING_OBJECT "default notifying object"
#define ERICSSON_VENDOR_ID 193

/* used by ntfread and ntfsend */
void getVendorId(SaNtfClassIdT *notificationClassId);
int get_long_digit(char *str, long *val);
int get_long_long_digit(char *str, long long *val);

#endif
