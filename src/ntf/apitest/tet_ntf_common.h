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
#ifndef NTF_APITEST_TET_NTF_COMMON_H_
#define NTF_APITEST_TET_NTF_COMMON_H_

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

typedef enum {
	SANTF_ALL = 0,
	SANTF_INITIALIZE,
	SANTF_SELECTION_OBJECT_GET,
	SANTF_DISPATCH,
	SANTF_FINALIZE,
	SANTF_ALARM_NOTIFICATION_ALLOCATE,
	SANTF_STATECHANGE_NOTIFICATION_ALLOCATE,
	SANTF_OBJECTCREATEDELETE_NOTIFICATION_ALLOCATE,
	SANTF_ATTRIBUTECHANGE_NOTIFICATION_ALLOCATE,
	SANTF_NOTIFICATION_FREE,
	SANTF_NOTIFICATION_SEND,
	SANTF_NOTIFICATION_SUBSCRIBE,
	SANTF_SECURITY_ALARM_NOTIFICATION_ALLOCATE,
	SANTF_PTRVAL_ALLOCATE,
	SANTF_ARRAYVAL_ALLOCATE,
	SANTF_LOCALIZED_MESSAGE_GET,
	SANTF_LOCALIZED_MESSAGE_FREE,
	SANTF_PTRVAL_GET,
	SANTF_ARRAYVAL_GET,
	SANTF_OBJECTCREATEDELETE_NOTIFICATION_FILTER_ALLOCATE,
	SANTF_ATTRIBUTECHANGE_NOTIFICATION_FILTER_ALLOCATE,
	SANTF_STATECHANGE_NOTIFICATION_FILTER_ALLOCATE,
	SANTF_ALARM_NOTIFICATION_FILTER_ALLOCATE,
	SANTF_SECURITYALARM_NOTIFICATION_FILTER_ALLOCATE,
	SANTF_NOTIFICATION_FILTER_FREE,
	SANTF_NOTIFICATION_UNSUBSCRIBE,
	SANTF_NOTIFICATION_READ_INITIALIZE,
	SANTF_NOTIFICATION_READ_FINALIZE,
	SANTF_NOTIFICATION_READ_NEXT,
	SANTF_API_MAX

} eSaNtfAPI;


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

SaAisErrorT ntftest_saNtfInitialize(SaNtfHandleT *ntfHandle, const SaNtfCallbacksT *ntfCallbacks, SaVersionT *version);
SaAisErrorT ntftest_saNtfSelectionObjectGet(SaNtfHandleT ntfHandle, SaSelectionObjectT *selectionObject);
SaAisErrorT ntftest_saNtfDispatch(SaNtfHandleT ntfHandle, SaDispatchFlagsT dispatchFlags);
SaAisErrorT ntftest_saNtfFinalize(SaNtfHandleT ntfHandle);
SaAisErrorT ntftest_saNtfNotificationFree(SaNtfNotificationHandleT notificationHandle);
SaAisErrorT ntftest_saNtfNotificationSend(SaNtfNotificationHandleT notificationHandle);
SaAisErrorT ntftest_saNtfNotificationSubscribe(const SaNtfNotificationTypeFilterHandlesT *notificationFilterHandles,
				       SaNtfSubscriptionIdT subscriptionId);
SaAisErrorT ntftest_saNtfNotificationUnsubscribe(SaNtfSubscriptionIdT subscriptionId);
SaAisErrorT ntftest_saNtfObjectCreateDeleteNotificationFilterAllocate(SaNtfHandleT ntfHandle, SaNtfObjectCreateDeleteNotificationFilterT
							      *notificationFilter, SaUint16T numEventTypes,
							      SaUint16T numNotificationObjects,
							      SaUint16T numNotifyingObjects,
							      SaUint16T numNotificationClassIds,
							      SaUint16T numSourceIndicators);
SaAisErrorT ntftest_saNtfAttributeChangeNotificationFilterAllocate(SaNtfHandleT ntfHandle,
							   SaNtfAttributeChangeNotificationFilterT *notificationFilter,
							   SaUint16T numEventTypes,
							   SaUint16T numNotificationObjects,
							   SaUint16T numNotifyingObjects,
							   SaUint16T numNotificationClassIds,
							   SaUint16T numSourceIndicators);
SaAisErrorT ntftest_saNtfStateChangeNotificationFilterAllocate(SaNtfHandleT ntfHandle,
						       SaNtfStateChangeNotificationFilterT *notificationFilter,
						       SaUint16T numEventTypes,
						       SaUint16T numNotificationObjects,
						       SaUint16T numNotifyingObjects,
						       SaUint16T numNotificationClassIds,
						       SaUint16T numSourceIndicators, SaUint16T numChangedStates);
SaAisErrorT ntftest_saNtfAlarmNotificationFilterAllocate(SaNtfHandleT ntfHandle,
						 SaNtfAlarmNotificationFilterT *notificationFilter,
						 SaUint16T numEventTypes,
						 SaUint16T numNotificationObjects,
						 SaUint16T numNotifyingObjects,
						 SaUint16T numNotificationClassIds,
						 SaUint16T numProbableCauses,
						 SaUint16T numPerceivedSeverities, SaUint16T numTrends);
SaAisErrorT ntftest_saNtfSecurityAlarmNotificationFilterAllocate(SaNtfHandleT ntfHandle,
							 SaNtfSecurityAlarmNotificationFilterT *notificationFilter,
							 SaUint16T numEventTypes,
							 SaUint16T numNotificationObjects,
							 SaUint16T numNotifyingObjects,
							 SaUint16T numNotificationClassIds,
							 SaUint16T numProbableCauses,
							 SaUint16T numSeverities,
							 SaUint16T numSecurityAlarmDetectors,
							 SaUint16T numServiceUsers, SaUint16T numServiceProviders);

SaAisErrorT ntftest_saNtfNotificationFilterFree(SaNtfNotificationFilterHandleT notificationFilterHandle);
SaAisErrorT ntftest_saNtfNotificationReadInitialize(SaNtfSearchCriteriaT searchCriteria,
					    const SaNtfNotificationTypeFilterHandlesT *notificationFilterHandles,
					    SaNtfReadHandleT *readHandle);

SaAisErrorT ntftest_saNtfNotificationReadNext(SaNtfReadHandleT readHandle,
				      SaNtfSearchDirectionT searchDirection, SaNtfNotificationsT *notification);

SaAisErrorT ntftest_saNtfNotificationReadFinalize(SaNtfReadHandleT readhandle);

SaAisErrorT ntftest_saNtfPtrValAllocate(SaNtfNotificationHandleT notificationHandle,
				SaUint16T dataSize, void **dataPtr, SaNtfValueT *value);

SaAisErrorT ntftest_saNtfAlarmNotificationAllocate(SaNtfHandleT ntfHandle,
					   SaNtfAlarmNotificationT *notification,
					   SaUint16T numCorrelatedNotifications,
					   SaUint16T lengthAdditionalText,
					   SaUint16T numAdditionalInfo,
					   SaUint16T numSpecificProblems,
					   SaUint16T numMonitoredAttributes,
					   SaUint16T numProposedRepairActions,
					   SaInt16T variableDataSize);

SaAisErrorT ntftest_saNtfSecurityAlarmNotificationAllocate(SaNtfHandleT ntfHandle,
						   SaNtfSecurityAlarmNotificationT *notification,
						   SaUint16T numCorrelatedNotifications,
						   SaUint16T lengthAdditionalText,
						   SaUint16T numAdditionalInfo,
						   SaInt16T variableDataSize);

SaAisErrorT ntftest_saNtfStateChangeNotificationAllocate(SaNtfHandleT ntfHandle,
				     SaNtfStateChangeNotificationT *notification,
				     SaUint16T numCorrelatedNotifications,
				     SaUint16T lengthAdditionalText,
				     SaUint16T numAdditionalInfo,
					 SaUint16T numStateChanges,
					 SaInt16T variableDataSize);

SaAisErrorT ntftest_saNtfObjectCreateDeleteNotificationAllocate(SaNtfHandleT ntfHandle,
					    SaNtfObjectCreateDeleteNotificationT *notification,
					    SaUint16T numCorrelatedNotifications,
					    SaUint16T lengthAdditionalText,
					    SaUint16T numAdditionalInfo,
					    SaUint16T numAttributes,
						SaInt16T variableDataSize);

SaAisErrorT ntftest_saNtfAttributeChangeNotificationAllocate(SaNtfHandleT ntfHandle,
					 SaNtfAttributeChangeNotificationT *notification,
					 SaUint16T numCorrelatedNotifications,
					 SaUint16T lengthAdditionalText,
					 SaUint16T numAdditionalInfo,
					 SaUint16T numAttributes,
					 SaInt16T variableDataSize);

SaAisErrorT ntftest_saNtfAlarmNotificationFilterAllocate(SaNtfHandleT ntfHandle,
						 SaNtfAlarmNotificationFilterT *notificationFilter,
						 SaUint16T numEventTypes,
						 SaUint16T numNotificationObjects,
						 SaUint16T numNotifyingObjects,
						 SaUint16T numNotificationClassIds,
						 SaUint16T numProbableCauses,
						 SaUint16T numPerceivedSeverities,
						 SaUint16T numTrends);

SaAisErrorT ntftest_saNtfSecurityAlarmNotificationFilterAllocate(SaNtfHandleT ntfHandle,
							 SaNtfSecurityAlarmNotificationFilterT *notificationFilter,
							 SaUint16T numEventTypes,
							 SaUint16T numNotificationObjects,
							 SaUint16T numNotifyingObjects,
							 SaUint16T numNotificationClassIds,
							 SaUint16T numProbableCauses,
							 SaUint16T numSeverities,
							 SaUint16T numSecurityAlarmDetectors,
							 SaUint16T numServiceUsers,
							 SaUint16T numServiceProviders);

SaAisErrorT ntftest_saNtfStateChangeNotificationFilterAllocate(SaNtfHandleT ntfHandle,
						       SaNtfStateChangeNotificationFilterT *notificationFilter,
						       SaUint16T numEventTypes,
						       SaUint16T numNotificationObjects,
						       SaUint16T numNotifyingObjects,
						       SaUint16T numNotificationClassIds,
						       SaUint16T numSourceIndicators,
							   SaUint16T numChangedStates);

SaAisErrorT ntftest_saNtfObjectCreateDeleteNotificationFilterAllocate(SaNtfHandleT ntfHandle,
								SaNtfObjectCreateDeleteNotificationFilterT *notificationFilter,
								SaUint16T numEventTypes,
							    SaUint16T numNotificationObjects,
							    SaUint16T numNotifyingObjects,
							    SaUint16T numNotificationClassIds,
							    SaUint16T numSourceIndicators);

SaAisErrorT ntftest_saNtfAttributeChangeNotificationFilterAllocate(SaNtfHandleT ntfHandle,
							   SaNtfAttributeChangeNotificationFilterT *notificationFilter,
							   SaUint16T numEventTypes,
							   SaUint16T numNotificationObjects,
							   SaUint16T numNotifyingObjects,
							   SaUint16T numNotificationClassIds,
							   SaUint16T numSourceIndicators);

void fprintf_v(FILE*, const char *format, ...);
void fprintf_t(FILE*, const char *format, ...);
void fprintf_p(FILE*, const char *format, ...);

extern void assertvalue_impl(__const char *__assertion, __const char *__file,
		   unsigned int __line, __const char *__function);

extern  void saNtfNotificationSend_01(void);
extern  void saNtfNotificationSend_02(void);
extern  void saNtfNotificationSend_03(void);
extern  void saNtfNotificationSend_04(void);
extern  void saNtfNotificationSend_05(void);
extern const SaVersionT lowestVersion;
#define assertvalue(expr) \
  (((expr) ? 0 : (assertvalue_impl (__STRING(expr), __FILE__, __LINE__, __FUNCTION__), 1)))

#endif  // NTF_APITEST_TET_NTF_COMMON_H_
