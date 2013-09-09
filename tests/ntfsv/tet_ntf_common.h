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
#ifndef tet_ntf_common_h
#define tet_ntf_common_h

#include <saNtf.h>
/* Defines */
#define DEFAULT_FLAG 0x0001
//#define MAX_NUMBER_OF_STATE_CHANGES 5
#define DEFAULT_NUMBER_OF_CHANGED_STATES 4
#define DEFAULT_NUMBER_OF_OBJECT_ATTRIBUTES 1
#define DEFAULT_NUMBER_OF_CHANGED_ATTRIBUTES 1

#define READER_USED 0
#define CALLBACK_USED 1


#define DEFAULT_NOTIFICATION_OBJECT "AMF BLABLA Component"
#define DEFAULT_NOTIFYING_OBJECT "AMF BLABLA Entity"
#define ERICSSON_VENDOR_ID 193
#define MAX_NUMBER_OF_STATE_CHANGES 5
#define MAX_NUMBER_OF_OBJECT_ATTRIBUTES  1
#define MAX_NUMBER_OF_CHANGED_ATTRIBUTES 1

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
    SaUint16T numSourceIndicators;
    SaUint16T numSeverities;
    SaUint16T numSecurityAlarmDetectos;
    SaUint16T numServiceUsers;
    SaUint16T numServiceProviders;
    SaUint16T numChangedStates;
    SaUint32T numProbableCauses;
    SaUint32T numPerceivedSeverities;
    SaUint32T numTrends;
} saNotificationFilterAllocationParamsT;

typedef struct {
    SaUint16T   numCorrelatedNotifications;
    SaUint16T   lengthAdditionalText;
    SaUint16T   numAdditionalInfo;
    SaUint16T   numSpecificProblems;
    SaUint16T   numMonitoredAttributes;
    SaUint16T   numProposedRepairActions;
    SaUint16T   numStateChanges;
    /* Object Create/Delete specific */
    SaUint16T numObjectAttributes;

    /* Attribute Change specific */
    SaUint16T numAttributes;

    SaInt16T    variableDataSize;
} saNotificationAllocationParamsT;

typedef struct {
    /* SaNtfSeverityTrendT trend; */
    SaVersionT              testVersion;
    unsigned int            testHandle;
    SaStringT               additionalText;
    SaNtfProbableCauseT     probableCause;
    SaNtfSeverityT          perceivedSeverity;
    SaNtfStateChangeT       changedStates[MAX_NUMBER_OF_STATE_CHANGES];
    SaNtfEventTypeT         eventType;
    SaNtfEventTypeT         alarmEventType;
    SaNtfEventTypeT         stateChangeEventType;
    SaNtfEventTypeT         objectCreateDeleteEventType;
    SaNtfEventTypeT         attributeChangeEventType;
    SaNtfEventTypeT         securityAlarmEventType;
    SaNtfNotificationTypeT  notificationType;
    SaNameT                 notificationObject;
    SaNameT                 notifyingObject;
    SaNtfClassIdT           notificationClassId;
    SaTimeT                 eventTime;
    SaNtfIdentifierT        notificationId;
    SaNtfSubscriptionIdT    subscriptionId[20];
    SaNtfSourceIndicatorT   stateChangeSourceIndicator;

    /* Object Create Delete Specific */
    SaNtfSourceIndicatorT objectCreateDeleteSourceIndicator;
    SaNtfAttributeT objectAttributes[MAX_NUMBER_OF_OBJECT_ATTRIBUTES];
    SaNtfValueTypeT attributeType;

    /* Attribute Change Specific*/
    SaNtfSourceIndicatorT attributeChangeSourceIndicator;
    SaNtfAttributeChangeT changedAttributes[MAX_NUMBER_OF_CHANGED_ATTRIBUTES];

    /* Security Alarm Specific */
    SaNtfProbableCauseT   securityAlarmProbableCause;
    SaNtfSeverityT severity;
    SaNtfSecurityAlarmDetectorT securityAlarmDetector;
    SaNtfServiceUserT serviceProvider;
    SaNtfServiceUserT serviceUser;

    SaNtfSearchCriteriaT    searchCriteria;
    SaNtfSearchDirectionT   searchDirection;
    SaInt32T                noSubscriptionIds;
    SaInt32T                timeout;
    unsigned int            hangTimeout;
    unsigned int            repeateSends;
} saNotificationParamsT;

typedef SaUint16T       saNotificationFlagsT;
extern SaNtfIdentifierT last_not_id;

extern SaNtfCallbacksT ntfSendCallbacks;

extern void fillInDefaultValues(
    saNotificationAllocationParamsT *notificationAllocationParams,
    saNotificationFilterAllocationParamsT *notificationFilterAllocationParams,
    saNotificationParamsT *notificationParams);

extern void fill_header_part(SaNtfNotificationHeaderT *notificationHeader,
                             saNotificationParamsT *notificationParams,
                             SaUint16T lengthAdditionalText);

extern int cmpSaNtfValueT(const SaNtfValueTypeT type,
		const SaNtfValueT *val1,	const SaNtfValueT *val2);


extern void fillHeader(SaNtfNotificationHeaderT *head);
extern void fillFilterHeader(SaNtfNotificationFilterHeaderT *head);

extern int verifyNotificationHeader( const SaNtfNotificationHeaderT *head1,
		const SaNtfNotificationHeaderT *head2);

extern SaAisErrorT createAlarmNotification(SaNtfHandleT ntfHandle,
		SaNtfAlarmNotificationT *myAlarmNotification);

extern int verifyAlarmNotification( const SaNtfAlarmNotificationT *aNtf1,
		const SaNtfAlarmNotificationT *aNtf2);

extern void createObjectCreateDeleteNotification(SaNtfHandleT ntfHandle,
		SaNtfObjectCreateDeleteNotificationT *myObjCrDeNotification);
extern int verifyObjectCreateDeleteNotification( const SaNtfObjectCreateDeleteNotificationT *aNtf1,
		const SaNtfObjectCreateDeleteNotificationT *aNtf2);

extern void createAttributeChangeNotification(SaNtfHandleT ntfHandle,
		SaNtfAttributeChangeNotificationT *myAttrChangeNotification);
extern int verifyAttributeChangeNotification( const SaNtfAttributeChangeNotificationT *aNtf1,
		const SaNtfAttributeChangeNotificationT *aNtf2);

extern void createStateChangeNotification(SaNtfHandleT ntfHandle,
		SaNtfStateChangeNotificationT *myStateChangeNotification);
extern int verifyStateChangeNotification(const SaNtfStateChangeNotificationT *aNtf1,
		const SaNtfStateChangeNotificationT *aNtf2);


extern void createSecurityAlarmNotification(SaNtfHandleT ntfHandle,
		SaNtfSecurityAlarmNotificationT *mySecAlarmNotification);
extern int verifySecurityAlarmNotification(const SaNtfSecurityAlarmNotificationT *aNtf1,
		const SaNtfSecurityAlarmNotificationT *aNtf2);

void newNotification(
    SaNtfSubscriptionIdT subscriptionId,
    const SaNtfNotificationsT *notification);
extern SaNtfIdentifierT get_ntf_id(const SaNtfNotificationsT *notif);
void poll_until_received(SaNtfHandleT ntfHandle, SaNtfIdentifierT wanted_id);


extern void assertvalue_impl(__const char *__assertion, __const char *__file,
		   unsigned int __line, __const char *__function);
#define assertvalue(expr) \
  (((expr) ? 0 : (assertvalue_impl (__STRING(expr), __FILE__, __LINE__, __FUNCTION__), 1)))

#endif
