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

#include <saNtf.h>
#include <stdlib.h>
#include "ntfsv_mem.h"
#include "osaf_extended_name.h"
#include "saAis.h"
#include <logtrace.h>

void ntfsv_free_header(const SaNtfNotificationHeaderT *notificationHeader, bool deallocate_longdn)
{
	/* Data */
	if (notificationHeader->eventType != NULL)
		free(notificationHeader->eventType);
	if (notificationHeader->eventTime != NULL)
		free(notificationHeader->eventTime);
	if (notificationHeader->notificationObject != NULL) {
		if (deallocate_longdn)
			osaf_extended_name_free(notificationHeader->notificationObject); 
		free(notificationHeader->notificationObject);
	}
	if (notificationHeader->notifyingObject != NULL) {
		if (deallocate_longdn)
			osaf_extended_name_free(notificationHeader->notifyingObject); 
		free(notificationHeader->notifyingObject);
	}
	if (notificationHeader->notificationClassId != NULL)
		free(notificationHeader->notificationClassId);
	if (notificationHeader->notificationId != NULL)
		free(notificationHeader->notificationId);

	/* Array Data */
	if (notificationHeader->correlatedNotifications != NULL) {
		free(notificationHeader->correlatedNotifications);
	}
	if (notificationHeader->additionalText != NULL) {
		free(notificationHeader->additionalText);
	}
	if (notificationHeader->additionalInfo != NULL) {
		free(notificationHeader->additionalInfo);
	}
}

void ntfsv_free_alarm(SaNtfAlarmNotificationT *alarm, bool deallocate_longdn)
{
	TRACE_ENTER();
	ntfsv_free_header(&alarm->notificationHeader, deallocate_longdn);
	if (alarm->trend != NULL)
		free(alarm->trend);
	if (alarm->thresholdInformation != NULL)
		free(alarm->thresholdInformation);
	if (alarm->probableCause != NULL)
		free(alarm->probableCause);
	if (alarm->perceivedSeverity != NULL)
		free(alarm->perceivedSeverity);
	if (alarm->specificProblems != NULL) {
		free(alarm->specificProblems);
	}
	if (alarm->monitoredAttributes != NULL) {
		free(alarm->monitoredAttributes);
	}
	if (alarm->proposedRepairActions != NULL) {
		free(alarm->proposedRepairActions);
	}
	TRACE_LEAVE();
}

void ntfsv_free_state_change(SaNtfStateChangeNotificationT *stateChange, bool deallocate_longdn)
{
	ntfsv_free_header(&stateChange->notificationHeader, deallocate_longdn);
	if (stateChange->sourceIndicator != NULL)
		free(stateChange->sourceIndicator);
	if (stateChange->changedStates != NULL) {
		free(stateChange->changedStates);
	}
}

void ntfsv_free_attribute_change(SaNtfAttributeChangeNotificationT *attrChange, bool deallocate_longdn)
{
	ntfsv_free_header(&attrChange->notificationHeader, deallocate_longdn);
	if (attrChange->sourceIndicator != NULL)
		free(attrChange->sourceIndicator);
	if (attrChange->changedAttributes != NULL) {
		free(attrChange->changedAttributes);
	}
}

void ntfsv_free_obj_create_del(SaNtfObjectCreateDeleteNotificationT *objCrDel, bool deallocate_longdn)
{
	ntfsv_free_header(&objCrDel->notificationHeader, deallocate_longdn);

	if (objCrDel->sourceIndicator != NULL)
		free(objCrDel->sourceIndicator);
	if (objCrDel->objectAttributes != NULL) {
		free(objCrDel->objectAttributes);
	}
}

void ntfsv_free_security_alarm(SaNtfSecurityAlarmNotificationT *secAlarm, bool deallocate_longdn)
{
	ntfsv_free_header(&secAlarm->notificationHeader, deallocate_longdn);
	if (secAlarm->probableCause != NULL)
		free(secAlarm->probableCause);
	if (secAlarm->severity != NULL)
		free(secAlarm->severity);
	if (secAlarm->securityAlarmDetector != NULL)
		free(secAlarm->securityAlarmDetector);
	if (secAlarm->serviceProvider != NULL)
		free(secAlarm->serviceProvider);
	if (secAlarm->serviceUser != NULL)
		free(secAlarm->serviceUser);
}

SaAisErrorT ntfsv_alloc_ntf_header(SaNtfNotificationHeaderT *notificationHeader,
				   SaUint16T numCorrelatedNotifications,
				   SaUint16T lengthAdditionalText, SaUint16T numAdditionalInfo)
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;
	if (notificationHeader == NULL) {
		TRACE("NULL pointer in *notificationHeader!");
		return SA_AIS_ERR_INVALID_PARAM;
	}
	notificationHeader->numCorrelatedNotifications = numCorrelatedNotifications;
	notificationHeader->lengthAdditionalText = lengthAdditionalText;
	notificationHeader->numAdditionalInfo = numAdditionalInfo;

	/* freed by ntfsv_free_header() */
	notificationHeader->notificationObject = NULL;
	notificationHeader->notifyingObject = NULL;
	notificationHeader->eventTime = NULL;
	notificationHeader->eventType = NULL;
	notificationHeader->correlatedNotifications = NULL;
	notificationHeader->notificationClassId = NULL;
	notificationHeader->additionalInfo = NULL;
	notificationHeader->additionalText = NULL;
	notificationHeader->notificationId = NULL;

	/* Event type */
	notificationHeader->eventType = malloc(sizeof(SaNtfEventTypeT));
	if (notificationHeader->eventType == NULL) {
		TRACE("Out of memory in eventType field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Notification object */
	notificationHeader->notificationObject = calloc(1,sizeof(SaNameT));
	if (notificationHeader->notificationObject == NULL) {
		TRACE("Out of memory in notificationObject field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Notifying object */
	notificationHeader->notifyingObject = calloc(1,sizeof(SaNameT));
	if (notificationHeader->notifyingObject == NULL) {
		TRACE("Out of memory in notifyingObject field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Notification class ID */
	notificationHeader->notificationClassId = malloc(sizeof(SaNtfClassIdT));
	if (notificationHeader->notificationClassId == NULL) {
		TRACE("Out of memory in notificationClassId field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Event time */
	notificationHeader->eventTime = malloc(sizeof(SaTimeT));
	if (notificationHeader->eventTime == NULL) {
		TRACE("Out of memory in eventTime field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Notification ID */
	notificationHeader->notificationId = malloc(sizeof(SaNtfIdentifierT));
	if (notificationHeader->notificationId == NULL) {
		TRACE("Out of memory in notificationId field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}
	/* Correlated notifications */
	if (numCorrelatedNotifications != 0) {
		notificationHeader->correlatedNotifications = (SaNtfIdentifierT *)
		    malloc(numCorrelatedNotifications * sizeof(SaNtfIdentifierT));
		if (notificationHeader->correlatedNotifications == NULL) {
			TRACE("Out of memory in correlatedNotifications field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	/* Additional info */
	if (numAdditionalInfo != 0) {
		notificationHeader->additionalInfo = (SaNtfAdditionalInfoT *)
		    calloc(1, numAdditionalInfo * sizeof(SaNtfAdditionalInfoT));
		if (notificationHeader->additionalInfo == NULL) {
			TRACE("Out of memory in additionalInfo field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	/* Additional text */
	if (lengthAdditionalText != 0) {
		notificationHeader->additionalText = (SaStringT)malloc(lengthAdditionalText * sizeof(char));
		if (notificationHeader->additionalText == NULL) {
			TRACE("Out of memory in additionalText field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

 done:
	if (rc != SA_AIS_OK) {
		ntfsv_free_header(notificationHeader, false);
	}
	TRACE_LEAVE();
	return rc;
}

SaAisErrorT ntfsv_alloc_ntf_alarm(SaNtfAlarmNotificationT *alarmNotification,
				  SaUint16T numSpecificProblems,
				  SaUint16T numMonitoredAttributes, SaUint16T numProposedRepairActions)
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;
	/* Perceived severity */
	alarmNotification->numSpecificProblems = numSpecificProblems;
	alarmNotification->numMonitoredAttributes = numMonitoredAttributes;
	alarmNotification->numProposedRepairActions = numProposedRepairActions;

	/* freed in */
	alarmNotification->probableCause = NULL;
	alarmNotification->specificProblems = NULL;
	alarmNotification->perceivedSeverity = NULL;
	alarmNotification->trend = NULL;
	alarmNotification->thresholdInformation = NULL;
	alarmNotification->monitoredAttributes = NULL;
	alarmNotification->proposedRepairActions = NULL;

	alarmNotification->perceivedSeverity = malloc(sizeof(SaNtfSeverityT));
	if (alarmNotification->perceivedSeverity == NULL) {
		TRACE("Out of memory in perceivedSeverity field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}
	/* Trend */
	alarmNotification->trend = malloc(sizeof(SaNtfSeverityTrendT));
	if (alarmNotification->trend == NULL) {
		TRACE("Out of memory in trend field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	*(alarmNotification->trend) = SA_NTF_TREND_NO_CHANGE;

	/* ThresholdInformation */
	alarmNotification->thresholdInformation = calloc(1, sizeof(SaNtfThresholdInformationT));
	if (alarmNotification->thresholdInformation == NULL) {
		TRACE("Out of memory in thresholdInformation field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}
	/* Probable cause */
	alarmNotification->probableCause = malloc(sizeof(SaNtfProbableCauseT));
	if (alarmNotification->probableCause == NULL) {
		TRACE("Out of memory in probableCause field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}
	/* Specific problems */
	if (numSpecificProblems != 0) {
		alarmNotification->specificProblems = (SaNtfSpecificProblemT *)
		    calloc(1, numSpecificProblems * sizeof(SaNtfSpecificProblemT));
		if (alarmNotification->specificProblems == NULL) {
			TRACE("Out of memory in specificProblems field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}
	/* Monitored attributes */
	if (numMonitoredAttributes != 0) {
		alarmNotification->monitoredAttributes = (SaNtfAttributeT *)
		    calloc(1, numMonitoredAttributes * sizeof(SaNtfAttributeT));
		if (alarmNotification->monitoredAttributes == NULL) {
			TRACE("Out of memory in monitoredAttributes field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}
	/* Proposed repair actions */
	if (numProposedRepairActions != 0) {
		alarmNotification->proposedRepairActions = (SaNtfProposedRepairActionT *)
		    calloc(1, numProposedRepairActions * sizeof(SaNtfProposedRepairActionT));
		if (alarmNotification->proposedRepairActions == NULL) {
			TRACE("Out of memory in proposedRepairActions field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}
 done:
	if (rc != SA_AIS_OK) {
		ntfsv_free_alarm(alarmNotification, false);
	}
	TRACE_LEAVE();
	return rc;
}

SaAisErrorT ntfsv_alloc_ntf_obj_create_del(SaNtfObjectCreateDeleteNotificationT *objCrDelNotification,
					   SaUint16T numAttributes)
{
	SaAisErrorT rc = SA_AIS_OK;
	objCrDelNotification->sourceIndicator = NULL;

	objCrDelNotification->objectAttributes = NULL;
	/* Source indicator */
	objCrDelNotification->numAttributes = numAttributes;
	objCrDelNotification->sourceIndicator = malloc(sizeof(SaNtfSourceIndicatorT));
	if (objCrDelNotification->sourceIndicator == NULL) {
		TRACE_1("Out of memory in sourceIndicator field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	*(objCrDelNotification->sourceIndicator) = SA_NTF_UNKNOWN_OPERATION;

	/* Object attributes */
	if (numAttributes != 0) {
		objCrDelNotification->objectAttributes = (SaNtfAttributeT *)
		    malloc(numAttributes * sizeof(SaNtfAttributeT));
		if (objCrDelNotification->objectAttributes == NULL) {
			TRACE_1("Out of memory in objectAttributes field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

 done:
	if (rc != SA_AIS_OK) {
		ntfsv_free_obj_create_del(objCrDelNotification, false);
	}
	return rc;
}

SaAisErrorT ntfsv_alloc_ntf_attr_change(SaNtfAttributeChangeNotificationT *attrChangeNotification,
					SaUint16T numAttributes)
{
	SaAisErrorT rc = SA_AIS_OK;
	attrChangeNotification->numAttributes = numAttributes;

	attrChangeNotification->sourceIndicator = NULL;
	attrChangeNotification->changedAttributes = NULL;
	/* Source indicator */
	attrChangeNotification->sourceIndicator = malloc(sizeof(SaNtfSourceIndicatorT));
	if (attrChangeNotification->sourceIndicator == NULL) {
		TRACE_1("Out of memory in sourceIndicator field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	*(attrChangeNotification->sourceIndicator) = SA_NTF_UNKNOWN_OPERATION;

	/* Changed attributes */
	if (numAttributes != 0) {
		attrChangeNotification->changedAttributes = (SaNtfAttributeChangeT *)
		    malloc(numAttributes * sizeof(SaNtfAttributeChangeT));
		if (attrChangeNotification->changedAttributes == NULL) {
			TRACE_1("Out of memory in changedAttributes field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}
 done:
	if (rc != SA_AIS_OK) {
		ntfsv_free_attribute_change(attrChangeNotification, false);
	}
	return rc;
}

SaAisErrorT ntfsv_alloc_ntf_state_change(SaNtfStateChangeNotificationT *stateChangeNotification,
					 SaUint16T numStateChanges)
{
	SaAisErrorT rc = SA_AIS_OK;

	stateChangeNotification->numStateChanges = numStateChanges;
	stateChangeNotification->changedStates = NULL;
	stateChangeNotification->sourceIndicator = NULL;
	/* Source indicator */
	stateChangeNotification->sourceIndicator = malloc(sizeof(SaNtfSourceIndicatorT));
	if (stateChangeNotification->sourceIndicator == NULL) {
		TRACE_1("Out of memory in sourceIndicator field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	*(stateChangeNotification->sourceIndicator) = SA_NTF_UNKNOWN_OPERATION;

	/* Changed states */
	if (numStateChanges != 0) {
		stateChangeNotification->changedStates = (SaNtfStateChangeT *)
		    malloc(numStateChanges * sizeof(SaNtfStateChangeT));
		if (stateChangeNotification->changedStates == NULL) {
			TRACE_1("Out of memory in changedStates field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}
 done:
	if (rc != SA_AIS_OK) {
		ntfsv_free_state_change(stateChangeNotification, false);
	}
	return rc;
}

SaAisErrorT ntfsv_alloc_ntf_security_alarm(SaNtfSecurityAlarmNotificationT *securityAlarm)
{
	SaAisErrorT rc = SA_AIS_OK;

	securityAlarm->probableCause = NULL;
	securityAlarm->severity = NULL;
	securityAlarm->securityAlarmDetector = NULL;
	securityAlarm->serviceUser = NULL;
	securityAlarm->serviceProvider = NULL;
	/* Probable cause */
	securityAlarm->probableCause = calloc(1, sizeof(SaNtfProbableCauseT));
	if (securityAlarm->probableCause == NULL) {
		TRACE_1("Out of memory in probableCause field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}
	/* Severity */
	securityAlarm->severity = calloc(1, sizeof(SaNtfSeverityT));
	if (securityAlarm->severity == NULL) {
		TRACE_1("Out of memory in severity field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}
	/* Security alarm detector */
	securityAlarm->securityAlarmDetector = calloc(1, sizeof(SaNtfSecurityAlarmDetectorT));
	if (securityAlarm->securityAlarmDetector == NULL) {
		TRACE_1("Out of memory in securityAlarmDetector field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}
	/* Service user */
	securityAlarm->serviceUser = calloc(1, sizeof(SaNtfServiceUserT));
	if (securityAlarm->serviceUser == NULL) {
		TRACE_1("Out of memory in serviceUser field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}
	/* Service provider */
	securityAlarm->serviceProvider = calloc(1, sizeof(SaNtfServiceUserT));
	if (securityAlarm->serviceProvider == NULL) {
		TRACE_1("Out of memory in serviceProvider field");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}
 done:
	if (rc != SA_AIS_OK) {
		ntfsv_free_security_alarm(securityAlarm, false);
	}
	return SA_AIS_OK;
}

void ntfsv_copy_ntf_header(SaNtfNotificationHeaderT *dest, const SaNtfNotificationHeaderT *src)
{
	int i;
	TRACE_ENTER();
	*(dest->eventType) = *(src->eventType);

	/* event time to be set automatically to current
	   time by saNtfNotificationSend */
	*(dest->eventTime) = *(src->eventTime);

	/* Set Notification Object */
	ntfsv_sanamet_copy(dest->notificationObject, src->notificationObject);

	/* Set Notifying Object */
	ntfsv_sanamet_copy(dest->notifyingObject, src->notifyingObject);

	/* set Notification Class Identifier */
	dest->notificationClassId->vendorId = src->notificationClassId->vendorId;

	/* sub id of this notification class within "name space" of vendor ID */
	dest->notificationClassId->majorId = src->notificationClassId->majorId;
	dest->notificationClassId->minorId = src->notificationClassId->minorId;

	*(dest->notificationId) = *src->notificationId;

	dest->lengthAdditionalText = src->lengthAdditionalText;
	/* Set additional text and additional info */
	(void)strncpy(dest->additionalText, src->additionalText, src->lengthAdditionalText);

	dest->numCorrelatedNotifications = src->numCorrelatedNotifications;
	TRACE("src.>numCorrelatedNotifications: %d", src->numCorrelatedNotifications);
	for (i = 0; i < dest->numCorrelatedNotifications; i++) {
		dest->correlatedNotifications[i] = src->correlatedNotifications[i];
	}
	TRACE("src.>numAdditionalInfo: %d", src->numAdditionalInfo);
	dest->numAdditionalInfo = src->numAdditionalInfo;
	if (0 < dest->numAdditionalInfo) {
		memcpy(dest->additionalInfo,
		       src->additionalInfo, dest->numAdditionalInfo * sizeof(SaNtfAdditionalInfoT));
	}
	TRACE_LEAVE();
}

/**
 * Copy a SaNtfAlarmNotificationT* from src to dest. 
 *  Note dest needs to be allocated to the same size as src.
 *  
 *  @param dest 
 *  @param src
 *  
 */
void ntfsv_copy_ntf_alarm(SaNtfAlarmNotificationT *dest, const SaNtfAlarmNotificationT *src)
{
	int i;
	TRACE_ENTER();
	ntfsv_copy_ntf_header(&dest->notificationHeader, &src->notificationHeader);
	/* Set perceived severity */
	*(dest->perceivedSeverity) = *(src->perceivedSeverity);
	/* Set probable cause */
	*(dest->probableCause) = *(src->probableCause);
	*(dest->trend) = *(src->trend);
	dest->numMonitoredAttributes = src->numMonitoredAttributes;
	dest->numProposedRepairActions = src->numProposedRepairActions;
	dest->numSpecificProblems = src->numSpecificProblems;
	/* TODO: fix SaNtfValueT copy for SA_NTF_VALUE_ARRAY SA_NTF_VALUE_STRING etc */
	*(dest->thresholdInformation) = *(src->thresholdInformation);
	for (i = 0; i < src->numMonitoredAttributes; i++) {
		dest->monitoredAttributes[i] = src->monitoredAttributes[i];
	}
	for (i = 0; i < src->numProposedRepairActions; i++) {
		dest->proposedRepairActions[i] = src->proposedRepairActions[i];
	}
	for (i = 0; i < src->numSpecificProblems; i++) {
		dest->specificProblems[i] = src->specificProblems[i];
	}
	TRACE_LEAVE();
}

/**
 * Copy a SaNtfxxNotificationT* from src to dest. 
 *  Note dest needs to be allocated to the same size as src.
 *  
 *  @param dest 
 *  @param src
 *  
 */
void ntfsv_copy_ntf_security_alarm(SaNtfSecurityAlarmNotificationT *dest, const SaNtfSecurityAlarmNotificationT *src)
{
	ntfsv_copy_ntf_header(&dest->notificationHeader, &src->notificationHeader);
	*(dest->probableCause) = *(src->probableCause);

	*(dest->severity) = *(src->severity);
	memcpy(dest->securityAlarmDetector, src->securityAlarmDetector, sizeof(SaNtfSecurityAlarmDetectorT));
	memcpy(dest->serviceUser, src->serviceUser, sizeof(SaNtfServiceUserT));
	memcpy(dest->serviceProvider, src->serviceProvider, sizeof(SaNtfServiceUserT));
}

/**
 * Copy a SaNtfxxNotificationT* from src to dest. 
 *  Note dest needs to be allocated to the same size as src.
 *  
 *  @param dest 
 *  @param src
 *  
 */
void ntfsv_copy_ntf_obj_cr_del(SaNtfObjectCreateDeleteNotificationT *dest,
			       const SaNtfObjectCreateDeleteNotificationT *src)
{
	ntfsv_copy_ntf_header(&dest->notificationHeader, &src->notificationHeader);
	dest->numAttributes = src->numAttributes;
	*dest->sourceIndicator = *src->sourceIndicator;

	if (0 < dest->numAttributes) {
		memcpy(dest->objectAttributes, src->objectAttributes, dest->numAttributes * sizeof(SaNtfAttributeT));
	}
}

/**
 * Copy a SaNtfxxNotificationT* from src to dest. 
 *  Note dest needs to be allocated to the same size as src.
 *  
 *  @param dest 
 *  @param src
 *  
 */
void ntfsv_copy_ntf_attr_change(SaNtfAttributeChangeNotificationT *dest, const SaNtfAttributeChangeNotificationT *src)
{
	ntfsv_copy_ntf_header(&dest->notificationHeader, &src->notificationHeader);
	dest->numAttributes = src->numAttributes;

	*dest->sourceIndicator = *src->sourceIndicator;
	if (0 < dest->numAttributes) {
		memcpy(dest->changedAttributes,
		       src->changedAttributes, dest->numAttributes * sizeof(SaNtfAttributeChangeT));
	}
}

/**
 * Copy a SaNtfxxNotificationT* from src to dest. 
 *  Note dest needs to be allocated to the same size as src.
 *  
 *  @param dest 
 *  @param src
 *  
 */
void ntfsv_copy_ntf_state_change(SaNtfStateChangeNotificationT *dest, const SaNtfStateChangeNotificationT *src)
{
	ntfsv_copy_ntf_header(&dest->notificationHeader, &src->notificationHeader);
	dest->numStateChanges = src->numStateChanges;

	*dest->sourceIndicator = *src->sourceIndicator;

	if (0 < dest->numStateChanges) {
		memcpy(dest->changedStates, src->changedStates, dest->numStateChanges * sizeof(SaNtfStateChangeT));
	}
}

void ntfsv_dealloc_notification(ntfsv_send_not_req_t *param)
{
	TRACE_ENTER2("ntfsv_send_not_req_t ptr = %p " "notificationType = %#x", param, (int)param->notificationType);
	switch (param->notificationType) {
	case SA_NTF_TYPE_ALARM:
		ntfsv_free_alarm(&param->notification.alarm, true);
		break;
	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		ntfsv_free_obj_create_del(&param->notification.objectCreateDelete, true);
		break;
	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		ntfsv_free_attribute_change(&param->notification.attributeChange, true);
		break;
	case SA_NTF_TYPE_STATE_CHANGE:
		ntfsv_free_state_change(&param->notification.stateChange, true);
		break;
	case SA_NTF_TYPE_SECURITY_ALARM:
		ntfsv_free_security_alarm(&param->notification.securityAlarm, true);
		break;
	default:
		TRACE("notificationType not valid");
	}
	TRACE_1("free v_data.p_base %p", param->variable_data.p_base);
	free(param->variable_data.p_base);
	param->variable_data.p_base = NULL;
	param->variable_data.size = 0;
	TRACE_LEAVE();
}

SaAisErrorT ntfsv_alloc_and_copy_not(ntfsv_send_not_req_t *dest, const ntfsv_send_not_req_t *src)
{
	SaAisErrorT rc;
	TRACE_ENTER2("ntfsv_send_not_req_t* src = %p", src);
	TRACE("notificationType = %x", (unsigned int)src->notificationType);
	switch (src->notificationType) {
	case SA_NTF_TYPE_ALARM:
		rc = ntfsv_alloc_ntf_alarm(&dest->notification.alarm,
					   src->notification.alarm.numSpecificProblems,
					   src->notification.alarm.numMonitoredAttributes,
					   src->notification.alarm.numProposedRepairActions);
		if (rc != SA_AIS_OK)
			goto done;
		rc = ntfsv_alloc_ntf_header(&dest->notification.alarm.notificationHeader,
					    src->notification.alarm.notificationHeader.numCorrelatedNotifications,
					    src->notification.alarm.notificationHeader.lengthAdditionalText,
					    src->notification.alarm.notificationHeader.numAdditionalInfo);
		if (rc != SA_AIS_OK)
			goto done;
		ntfsv_copy_ntf_alarm(&dest->notification.alarm, &src->notification.alarm);
		break;
	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		rc = ntfsv_alloc_ntf_obj_create_del(&dest->notification.objectCreateDelete,
						    src->notification.objectCreateDelete.numAttributes);
		if (rc != SA_AIS_OK)
			goto done;
		rc = ntfsv_alloc_ntf_header(&dest->notification.objectCreateDelete.notificationHeader,
					    src->notification.objectCreateDelete.
					    notificationHeader.numCorrelatedNotifications,
					    src->notification.objectCreateDelete.
					    notificationHeader.lengthAdditionalText,
					    src->notification.objectCreateDelete.notificationHeader.numAdditionalInfo);
		if (rc != SA_AIS_OK)
			goto done;
		ntfsv_copy_ntf_obj_cr_del(&dest->notification.objectCreateDelete,
					  &src->notification.objectCreateDelete);
		break;
	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		rc = ntfsv_alloc_ntf_attr_change(&dest->notification.attributeChange,
						 src->notification.attributeChange.numAttributes);
		if (rc != SA_AIS_OK)
			goto done;
		rc = ntfsv_alloc_ntf_header(&dest->notification.attributeChange.notificationHeader,
					    src->notification.attributeChange.
					    notificationHeader.numCorrelatedNotifications,
					    src->notification.attributeChange.notificationHeader.lengthAdditionalText,
					    src->notification.attributeChange.notificationHeader.numAdditionalInfo);
		if (rc != SA_AIS_OK)
			goto done;
		ntfsv_copy_ntf_attr_change(&dest->notification.attributeChange, &src->notification.attributeChange);
		break;
	case SA_NTF_TYPE_STATE_CHANGE:
		rc = ntfsv_alloc_ntf_state_change(&dest->notification.stateChange,
						  src->notification.stateChange.numStateChanges);
		if (rc != SA_AIS_OK)
			goto done;
		rc = ntfsv_alloc_ntf_header(&dest->notification.stateChange.notificationHeader,
					    src->notification.stateChange.notificationHeader.numCorrelatedNotifications,
					    src->notification.stateChange.notificationHeader.lengthAdditionalText,
					    src->notification.stateChange.notificationHeader.numAdditionalInfo);
		if (rc != SA_AIS_OK)
			goto done;
		ntfsv_copy_ntf_state_change(&dest->notification.stateChange, &src->notification.stateChange);
		break;
	case SA_NTF_TYPE_SECURITY_ALARM:
		rc = ntfsv_alloc_ntf_security_alarm(&dest->notification.securityAlarm);
		if (rc != SA_AIS_OK)
			goto done;
		rc = ntfsv_alloc_ntf_header(&dest->notification.securityAlarm.notificationHeader,
					    src->notification.securityAlarm.
					    notificationHeader.numCorrelatedNotifications,
					    src->notification.securityAlarm.notificationHeader.lengthAdditionalText,
					    src->notification.securityAlarm.notificationHeader.numAdditionalInfo);
		if (rc != SA_AIS_OK)
			goto done;
		ntfsv_copy_ntf_security_alarm(&dest->notification.securityAlarm, &src->notification.securityAlarm);
		break;
	default:
		TRACE("notificationType not valid");
		rc = SA_AIS_ERR_FAILED_OPERATION;
	}
	rc = ntfsv_v_data_cp(&dest->variable_data, &src->variable_data);
	TRACE_LEAVE();
 done:
	return rc;
}

void ntfsv_get_ntf_header(ntfsv_send_not_req_t *notif, SaNtfNotificationHeaderT **ntfHeader)
{
	TRACE_ENTER();
	switch (notif->notificationType) {
	case SA_NTF_TYPE_ALARM:
		*ntfHeader = &notif->notification.alarm.notificationHeader;
		break;
	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		*ntfHeader = &notif->notification.objectCreateDelete.notificationHeader;
		break;
	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		*ntfHeader = &notif->notification.attributeChange.notificationHeader;
		break;
	case SA_NTF_TYPE_STATE_CHANGE:
		*ntfHeader = &notif->notification.stateChange.notificationHeader;
		break;
	case SA_NTF_TYPE_SECURITY_ALARM:
		*ntfHeader = &notif->notification.securityAlarm.notificationHeader;
		break;
	default:
		TRACE("notificationType not valid");
		osafassert(0);
	}
	TRACE_LEAVE();
}

SaAisErrorT ntfsv_variable_data_init(v_data * vd, SaInt32T max_data_size, SaUint32T ntfsv_var_data_limit)
{
	/* Preallocated memory is not supported */
	SaAisErrorT rc = SA_AIS_OK;
	vd->p_base = NULL;
	vd->size = 0;
	vd->max_data_size = max_data_size;

	if (max_data_size == SA_NTF_ALLOC_SYSTEM_LIMIT) {
		vd->max_data_size = ntfsv_var_data_limit;
	} else if (vd->max_data_size > ntfsv_var_data_limit) {
		TRACE("SA_AIS_ERR_TOO_BIG\n");
		rc = SA_AIS_ERR_TOO_BIG;
	}
	return rc;
}

SaAisErrorT ntfsv_ptr_val_alloc(v_data * vd, SaNtfValueT *nv, SaUint16T data_size, void **data_ptr)
{
	SaAisErrorT rc = SA_AIS_OK;
	if (data_size == 0) {
		TRACE("SA_AIS_ERR_INVALID_PARAM\n");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if ((vd->size + data_size) <= vd->max_data_size) {
		void *p;
		/* realloc */
		TRACE("realloc base=%p, size=%zd, data_size=%hu\n", vd->p_base, vd->size, data_size);
		p = realloc(vd->p_base, vd->size + data_size);
		if (p == NULL) {
			rc = SA_AIS_ERR_NO_MEMORY;
			TRACE("SA_AIS_ERR_NO_MEMORY\n");
			goto done;
		}
		vd->p_base = p;
		nv->ptrVal.dataOffset = vd->size;
		nv->ptrVal.dataSize = data_size;
		*data_ptr = vd->p_base + vd->size;
		memset(*data_ptr, 0, data_size);
		vd->size += data_size;
	} else {
		TRACE("SA_AIS_ERR_NO_SPACE\n");
		return SA_AIS_ERR_NO_SPACE;
	}
 done:
	return rc;
}

SaAisErrorT ntfsv_ptr_val_get(v_data * vd, const SaNtfValueT *nv, void **data_ptr, SaUint16T *data_size)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE("vd->size=%zd, nv:dataOffset = %u, nv:dataSize = %u, p_base= %p\n",
	      vd->size, nv->ptrVal.dataOffset, nv->ptrVal.dataSize, vd->p_base);
	if (vd->size < nv->ptrVal.dataOffset || vd->p_base == NULL) {
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	*data_ptr = vd->p_base + nv->ptrVal.dataOffset;
	TRACE("*dataPtr: %p\n", *data_ptr);
	*data_size = nv->ptrVal.dataSize;
 done:
	return rc;
}

SaAisErrorT ntfsv_array_val_alloc(v_data * vd,
				  SaNtfValueT *nv, SaUint16T numElements, SaUint16T elementSize, void **array_ptr)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaUint64T addedSize = 0;

	if (numElements == 0 || elementSize == 0) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}
	addedSize = (SaUint64T)numElements * (SaUint64T)elementSize;

	TRACE("ptrAlloc base=%p, size=%zd, numElements=%u, elementSize=%hu,"
	      "addedSize=%llu\n", vd->p_base, vd->size, numElements, elementSize, addedSize);

	if ((vd->size + addedSize) <= vd->max_data_size) {
		void *p;
		/* realloc */
		p = realloc(vd->p_base, vd->size + (size_t)addedSize);
		if (p == NULL) {
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
		vd->p_base = p;
		nv->arrayVal.arrayOffset = vd->size;
		nv->arrayVal.numElements = numElements;
		nv->arrayVal.elementSize = elementSize;
		*array_ptr = vd->p_base + vd->size;
		vd->size += addedSize;
	} else {
		TRACE("SA_AIS_ERR_NO_SPACE\n");
		return SA_AIS_ERR_NO_SPACE;
	}
 done:
	return rc;
}

SaAisErrorT ntfsv_array_val_get(v_data * vd,
				const SaNtfValueT *nv, void **arrayPtr, SaUint16T *numElements, SaUint16T *elementSize)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaUint64T maxOffset = (SaUint64T)nv->arrayVal.arrayOffset + (SaUint64T)nv->arrayVal.numElements * (SaUint64T)nv->arrayVal.elementSize;
	SaUint64T llsize = 0;
	llsize = (SaUint64T)vd->size;
	TRACE("vd->base: %p vd->size: %zd ntfvalue: offset: %hu numE: %hu esize: %hu, maxOffset: %llu, llsize: %llu",
	      vd->p_base,
	      vd->size,
	      nv->arrayVal.arrayOffset, nv->arrayVal.numElements, nv->arrayVal.elementSize, maxOffset, llsize);

	if (llsize < maxOffset) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}
	if (vd->p_base == NULL) {
		rc = SA_AIS_ERR_LIBRARY;
		goto done;
	}
	*numElements = nv->arrayVal.numElements;
	*elementSize = nv->arrayVal.elementSize;
	*arrayPtr = vd->p_base + nv->arrayVal.arrayOffset;
 done:
	return rc;
}

SaAisErrorT ntfsv_v_data_cp(v_data * dest, const v_data * src)
{
	TRACE_ENTER2("src alloc v_data max_size %u, size %zd, p_base %p", src->max_data_size, src->size, src->p_base);
	dest->max_data_size = src->max_data_size;
	dest->size = src->size;
	if (src->size) {
		dest->p_base = calloc(1, src->size);
		TRACE_1("alloc v_data.p_base %p", dest->p_base);
		if (dest->p_base == NULL) {
			TRACE_LEAVE();
			return SA_AIS_ERR_NO_MEMORY;
		}
		memcpy(dest->p_base, src->p_base, src->size);
	} else {
		dest->p_base = NULL;
	} 
	TRACE_LEAVE2("dest alloc v_data max_size %u, size %zd, p_base %p",
		     dest->max_data_size, dest->size, dest->p_base);
	return SA_AIS_OK;
}

SaAisErrorT ntfsv_filter_header_alloc(SaNtfNotificationFilterHeaderT *header,
													SaUint16T numEventTypes,
													SaUint16T numNotificationObjects,
													SaUint16T numNotifyingObjects,
													SaUint16T numNotificationClassIds)
{
	SaAisErrorT rc = SA_AIS_OK;
	header->eventTypes = NULL;
	header->notificationObjects = NULL;
	header->notifyingObjects = NULL;
	header->notificationClassIds = NULL;

	header->numNotificationClassIds = numNotificationClassIds;
	header->numEventTypes = numEventTypes;
	header->numNotificationObjects = numNotificationObjects;
	header->numNotifyingObjects = numNotifyingObjects;

	/* Event types */
	if (numEventTypes != 0) {
		header->eventTypes = (SaNtfEventTypeT *) malloc(numEventTypes * sizeof(SaNtfEventTypeT));
		if (header->eventTypes == NULL) {
			TRACE_1("Out of memory eventTypes");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	/* Notification objects */
	if (numNotificationObjects != 0) {

		header->notificationObjects = (SaNameT *)calloc(numNotificationObjects, sizeof(SaNameT));
		if (header->notificationObjects == NULL) {
			TRACE_1("Out of memory notificationObjects");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}

	/* Notifying objects */
	if (numNotifyingObjects != 0) {
		header->notifyingObjects = (SaNameT *)calloc(numNotifyingObjects, sizeof(SaNameT));
		if (header->notifyingObjects == NULL) {
			TRACE_1("Out of memory notifyingObjects");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}

	/* Notification class Id:s */
	if (numNotificationClassIds != 0) {
		header->notificationClassIds = (SaNtfClassIdT *) malloc(numNotificationClassIds * sizeof(SaNtfClassIdT));
		if (header->notificationClassIds == NULL) {
			TRACE_1("Out of memory notificationClassIds");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}
done:
	 return rc;

error_free:
	ntfsv_filter_header_free(header, false);
	goto done;
}

SaAisErrorT ntfsv_filter_obj_cr_del_alloc(SaNtfObjectCreateDeleteNotificationFilterT *filter,
														SaUint16T numSourceIndicators)
{
	SaAisErrorT rc = SA_AIS_OK;

	filter->sourceIndicators = NULL;
	filter->numSourceIndicators = numSourceIndicators;
	if (numSourceIndicators != 0) {
	filter->sourceIndicators = (SaNtfSourceIndicatorT *) malloc(numSourceIndicators * sizeof(SaNtfSourceIndicatorT));
	if (filter->sourceIndicators == NULL) {
		TRACE_1("Out of memory in SourceIndicators field");
		rc = SA_AIS_ERR_NO_MEMORY;
		}
	}
	return rc;
}

SaAisErrorT ntfsv_filter_attr_change_alloc(SaNtfAttributeChangeNotificationFilterT *filter,
														SaUint16T numSourceIndicators)
{
	SaAisErrorT rc = SA_AIS_OK;

	filter->sourceIndicators = NULL;
	filter->numSourceIndicators = numSourceIndicators;
	if (numSourceIndicators != 0) {
	filter->sourceIndicators = (SaNtfSourceIndicatorT *) malloc(numSourceIndicators * sizeof(SaNtfSourceIndicatorT));
	if (filter->sourceIndicators == NULL) {
		TRACE_1("Out of memory in SourceIndicators field");
		rc = SA_AIS_ERR_NO_MEMORY;
		}
	}
	return rc;
}

SaAisErrorT ntfsv_filter_state_ch_alloc(SaNtfStateChangeNotificationFilterT *f, SaUint32T numSourceIndicators,
	SaUint32T numChangedStates)
{
	SaAisErrorT rc = SA_AIS_OK;

	f->sourceIndicators = NULL;
	f->changedStates = NULL;

	f->numSourceIndicators = numSourceIndicators;
	if (numSourceIndicators != 0) {
		f->sourceIndicators = (SaNtfSourceIndicatorT *) malloc(numSourceIndicators * sizeof(SaNtfSourceIndicatorT));
		if (f->sourceIndicators == NULL) {
			TRACE_1("Out of memory in SourceIndicators field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	f->numStateChanges = numChangedStates;
	  if (numChangedStates != 0) {
		  f->changedStates = (SaNtfStateChangeT*) malloc(numChangedStates * sizeof(SaNtfStateChangeT));
		  if (f->changedStates == NULL) {
			  TRACE_1("Out of memory in StateChanges field");
			  rc = SA_AIS_ERR_NO_MEMORY;  
		  }
	  }
done: 
	return rc;
}

SaAisErrorT ntfsv_filter_alarm_alloc(SaNtfAlarmNotificationFilterT *filter,
	SaUint32T numProbableCauses,
	SaUint32T numPerceivedSeverities,
	SaUint32T numTrends)
{
	SaAisErrorT rc = SA_AIS_OK;

	filter->probableCauses = NULL;
	filter->perceivedSeverities = NULL;
	filter->trends = NULL;

	filter->numProbableCauses = numProbableCauses;
	filter->numPerceivedSeverities = numPerceivedSeverities;
	filter->numTrends = numTrends;

	/* Probable causes */
	if (numProbableCauses != 0) {
		filter->probableCauses = (SaNtfProbableCauseT *) malloc(numProbableCauses * sizeof(SaNtfProbableCauseT));
		if (filter->probableCauses == NULL) {
			TRACE_1("Out of memory in probableCauses field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}
	/* Percieved severities */
	if (numPerceivedSeverities != 0) {
		filter->perceivedSeverities = (SaNtfSeverityT *) malloc(numPerceivedSeverities * sizeof(SaNtfSeverityT));
		if (filter->perceivedSeverities == NULL) {
			TRACE_1("Out of memory in perceivedSeverities field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}
	/* Severity trends */
	if (numTrends != 0) {
		filter->trends = (SaNtfSeverityTrendT *) malloc(numTrends * sizeof(SaNtfSeverityTrendT));
		if (filter->trends == NULL) {
			TRACE_1("Out of memory in trends field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}
 done:
	return rc;
 error_free:
	ntfsv_filter_alarm_free(filter, false);
	goto done;
}

SaAisErrorT ntfsv_filter_sec_alarm_alloc(SaNtfSecurityAlarmNotificationFilterT *filter,
													  SaUint32T numProbableCauses,
													  SaUint32T numSeverities,
													  SaUint32T numSecurityAlarmDetectors,
													  SaUint32T numServiceUsers,
													  SaUint32T numServiceProviders)
{
	SaAisErrorT rc = SA_AIS_OK;

	filter->probableCauses = NULL;
	filter->severities = NULL;
	filter->securityAlarmDetectors = NULL;
	filter->serviceUsers = NULL;
	filter->serviceProviders = NULL;

	filter->numProbableCauses = numProbableCauses;
	filter->numSeverities = numSeverities;
	filter->numSecurityAlarmDetectors = numSecurityAlarmDetectors;
	filter->numServiceUsers = numServiceUsers;
	filter->numServiceProviders = numServiceProviders;

	if (numProbableCauses != 0) {
		filter->probableCauses = (SaNtfProbableCauseT *) malloc(numProbableCauses * sizeof(SaNtfProbableCauseT));
		if (filter->probableCauses == NULL) {
			TRACE_1("Out of memory in probableCauses field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}
	if (numSeverities != 0) {
		filter->severities = (SaNtfSeverityT *) malloc(numSeverities * sizeof(SaNtfSeverityT));
		if (filter->severities == NULL) {
			TRACE_1("Out of memory in perceivedSeverities field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}
	if (numSecurityAlarmDetectors != 0) {
		filter->securityAlarmDetectors = (SaNtfSecurityAlarmDetectorT *) malloc(numSecurityAlarmDetectors * sizeof(SaNtfSecurityAlarmDetectorT));
		if (filter->securityAlarmDetectors == NULL) {
			TRACE_1("Out of memory in securityAlarmDetector field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}
	if (numServiceUsers != 0) {
		filter->serviceUsers = (SaNtfServiceUserT *) malloc(numServiceUsers * sizeof(SaNtfServiceUserT));
		if (filter->serviceUsers == NULL) {
			TRACE_1("Out of memory in ServiceUsers field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}
	if (numServiceProviders != 0) {
		filter->serviceProviders = (SaNtfServiceUserT *) malloc(numServiceProviders * sizeof(SaNtfServiceUserT));
		if (filter->serviceProviders == NULL) {
			TRACE_1("Out of memory in ServiceProviders field");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto error_free;
		}
	}
done:
	return rc;  
error_free:
	ntfsv_filter_sec_alarm_free(filter, false);
	goto done;
}



void ntfsv_filter_header_free(SaNtfNotificationFilterHeaderT *header, bool deallocate_longdn)
{
	free(header->eventTypes);
	if (deallocate_longdn) {
		size_t i;
		for (i = 0; i != header->numNotificationObjects; ++i)
			osaf_extended_name_free(&header->notificationObjects[i]);
		for (i = 0; i != header->numNotifyingObjects; ++i)
			osaf_extended_name_free(&header->notifyingObjects[i]);
	}
 	free(header->notificationObjects);
 	free(header->notifyingObjects);
 	free(header->notificationClassIds);
}

void ntfsv_filter_sec_alarm_free(SaNtfSecurityAlarmNotificationFilterT *s_filter, bool deallocate_longdn)
{
	if (s_filter) {
		free(s_filter->probableCauses);
		free(s_filter->securityAlarmDetectors);
		free(s_filter->serviceProviders);
		free(s_filter->serviceUsers);
		free(s_filter->severities);
		ntfsv_filter_header_free(&s_filter->notificationFilterHeader, deallocate_longdn);
	}
}

void ntfsv_filter_alarm_free(SaNtfAlarmNotificationFilterT *a_filter, bool deallocate_longdn)
{
	if (a_filter) {
		free(a_filter->probableCauses);
		free(a_filter->perceivedSeverities);
		free(a_filter->trends);
		ntfsv_filter_header_free(&a_filter->notificationFilterHeader, deallocate_longdn);
	}
}

void ntfsv_filter_state_ch_free(SaNtfStateChangeNotificationFilterT *f, bool deallocate_longdn)
{
	if (f) {
		free(f->changedStates);
		free(f->sourceIndicators);
		ntfsv_filter_header_free(&f->notificationFilterHeader, deallocate_longdn);
	}
}
void ntfsv_filter_obj_cr_del_free(SaNtfObjectCreateDeleteNotificationFilterT *f, bool deallocate_longdn)
{
	if (f) {
		free(f->sourceIndicators);
		ntfsv_filter_header_free(&f->notificationFilterHeader, deallocate_longdn);
	}

}
void ntfsv_filter_attr_ch_free(SaNtfAttributeChangeNotificationFilterT *f, bool deallocate_longdn)
{
	if (f) {
		free(f->sourceIndicators);
		ntfsv_filter_header_free(&f->notificationFilterHeader, deallocate_longdn);
	}
}

/**
 *  @Brief: Copy SaNameT pointed by pSrc to pDes
 *
 */
SaAisErrorT ntfsv_sanamet_copy(SaNameT* pDes, SaNameT* pSrc)
{
	SaAisErrorT rc = SA_AIS_OK;
	if (osaf_is_an_extended_name(pDes)) {
		TRACE("Can not modify the existing longDn object");
		return SA_AIS_ERR_EXIST;
	}
	if (osaf_is_an_extended_name(pSrc)) {
		osaf_extended_name_alloc(osaf_extended_name_borrow(pSrc), pDes);
	} else {
		/* pSrc is old SaNameT format, @.length could be counted with or
		 * without null-termination, we need to preserve the @.length,
 		 * just a memcpy here
		 */
		 memcpy(pDes, pSrc, sizeof(SaNameT));
	}
	return rc;
}

/**
 *  @Brief: Check SaNameT is a valid formation
 *
 */
bool ntfsv_sanamet_is_valid(const SaNameT* pName)
{
	if (!osaf_is_extended_name_valid(pName)) {
		LOG_ER("Environment variable SA_ENABLE_EXTENDED_NAMES "
			"is not set, or not using extended name api");
		return false;
	}
	if (osaf_extended_name_length(pName) > kOsafMaxDnLength) {
		LOG_ER("Exceeding maximum of extended name length(%u)"
			,kOsafMaxDnLength);
		return false;
	}
	return true;
}

/**
 *  @Brief: Return the length of string specified in SaNameT type.
 *
 */
size_t ntfs_sanamet_length(const SaNameT* pName)
{
	if (osaf_is_an_extended_name(pName))
		return osaf_extended_name_length(pName);
	
	/* In case of unextended name, sometimes @.length includes the count on
	 * null-termination, and osaf_extended_name_length returns the length
	 * excluding the null-termination which less 1 than the @.length. Here
	 * we need the length specified in @.length. This case mainly applies
	 * in encodeSaNameT/decodeSaNameT in order to preserve the original
	 * @.length value
	 */
	size_t length = *((SaUint16T*)pName);
	osafassert(length < SA_MAX_UNEXTENDED_NAME_LENGTH);
	return length;
}

/**
 *  @Brief: Create SaNameT input by string @value and @length,
 *			@value is not freed then
 */
void ntfs_sanamet_alloc(SaConstStringT value, size_t length, SaNameT* pName)
{
	osaf_extended_name_alloc(value, pName);
	/* Accept the old SaNameT which's @.length counting the null termination */
	if (!osaf_is_an_extended_name(pName) 
		&& ((ntfs_sanamet_length(pName)+1) == length)) {
		*((SaUint16T*)pName) += 1;
	}	
}

/**
 *  @Brief: Create SaNameT input by string @value and @length,
 *			@value is freed then
 */
void ntfs_sanamet_steal(SaStringT value, size_t length, SaNameT* pName)
{
	/* create pName by string @value */
	osaf_extended_name_free(pName);
	osaf_extended_name_steal(value, pName);
	
	/* Accept the old SaNameT which's @.length counting the null termination */
	if (!osaf_is_an_extended_name(pName) 
		&& ((ntfs_sanamet_length(pName)+1) == length)) {
		*((SaUint16T*)pName) += 1;
	}
}
