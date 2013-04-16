/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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

/*****************************************************************************

  DESCRIPTION:   This file contains routines used for sending alarms and 
  notifications.

******************************************************************************/

#include <logtrace.h>
#include <avd_util.h>
#include <avd_ntf.h>

/*****************************************************************************
  Name          :  avd_send_comp_inst_failed_alarm

  Description   :  This function generates a Component Instantiation Failed alarm.

  Arguments     :  comp_name - Pointer to the Component DN.
                   node_name - Pointer to the DN of node on which the component 
                               is hosted.

  Return Values :

  Notes         :
*****************************************************************************/
void avd_send_comp_inst_failed_alarm(const SaNameT *comp_name, const SaNameT *node_name)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "Instantiation of Component %s failed",
			comp_name->value);
	sendAlarmNotificationAvd(avd_cb,
				 *comp_name,
				 (SaUint8T*)add_text,
				 SA_SVC_AMF,
				 SA_AMF_NTFID_COMP_INSTANTIATION_FAILED,
				 SA_NTF_SOFTWARE_ERROR,
				 SA_NTF_SEVERITY_MAJOR,
				 (NCSCONTEXT)node_name,
				 1 /* add_info is node_name */); 
}

/*****************************************************************************
  Name          :  avd_send_comp_clean_failed_alarm

  Description   :  This function generates a Component Cleanup Failed alarm.

  Arguments     :  comp_name - Pointer to the Component DN
                   node_name - Pointer to the DN of node on which the component 
                               is hosted.

  Return Values :

  Notes         :
*****************************************************************************/
void avd_send_comp_clean_failed_alarm(const SaNameT *comp_name, const SaNameT *node_name)
{
	char  add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "Cleanup of Component %s failed", 
			comp_name->value);
	sendAlarmNotificationAvd(avd_cb,
				 *comp_name,
				 (SaUint8T*)add_text,
				 SA_SVC_AMF,
				 SA_AMF_NTFID_COMP_CLEANUP_FAILED,
				 SA_NTF_SOFTWARE_ERROR,
				 SA_NTF_SEVERITY_MAJOR,
				 (NCSCONTEXT)node_name,
				 1 /* add_info is node_name */); 

}
/*****************************************************************************
  Name          :  avd_send_cluster_reset_alarm

  Description   :  This function generates a Cluster Reset alarm as designated 
                   component failed and a cluster reset recovery as recommended
                   by the component is being done.

  Arguments     :  comp_name - Pointer to the Component DN
                   node_name - Pointer to the DN of node on which the component 
                               is hosted.

  Return Values :

  Notes         :
*****************************************************************************/
void avd_send_cluster_reset_alarm(const SaNameT *comp_name)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "Failure of Component %s triggered"
			" cluster reset", comp_name->value);
	sendAlarmNotificationAvd(avd_cb,
				 *comp_name,
				 (SaUint8T*)add_text,
				 SA_SVC_AMF,
				 SA_AMF_NTFID_CLUSTER_RESET,
				 SA_NTF_SOFTWARE_ERROR,
				 SA_NTF_SEVERITY_MAJOR,
				 NULL,
				 0 /* No add_info */);

}

/*****************************************************************************
  Name          :  avd_send_si_unassigned_alarm

  Description   :  This function generates a Service Instance Unassigned alarm

  Arguments     :  si_name - Pointer to the SI DN

  Return Values :

  Notes         :
*****************************************************************************/
void avd_send_si_unassigned_alarm(const SaNameT *si_name)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();
	
	snprintf(add_text, ADDITION_TEXT_LENGTH, "SI designated by %s has no current "
			"active assignments to any SU", si_name->value);
	sendAlarmNotificationAvd(avd_cb,
				 *si_name,
				 (SaUint8T*)add_text,
				 SA_SVC_AMF,
				 SA_AMF_NTFID_SI_UNASSIGNED,
				 SA_NTF_SOFTWARE_ERROR,
				 SA_NTF_SEVERITY_MAJOR,
				 NULL,
				 0 /* No add_info */);

}

/*****************************************************************************
  Name          :  avd_send_comp_proxy_status_unproxied_alarm

  Description   :  This function generates a Proxy Status of a Component 
                   Changed to Unproxied alarm.

  Arguments     :  comp_name - Pointer to the Component DN

  Return Values :

  Notes         :
*****************************************************************************/
void avd_send_comp_proxy_status_unproxied_alarm(const SaNameT *comp_name)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "Component %s become orphan",
			comp_name->value);
	sendAlarmNotificationAvd(avd_cb,
				 *comp_name,
				 (SaUint8T*)add_text,
				 SA_SVC_AMF,
				 SA_AMF_NTFID_COMP_UNPROXIED,
				 SA_NTF_SOFTWARE_ERROR,
				 SA_NTF_SEVERITY_MAJOR,
				 NULL,
				 0 /* No add_info */);

}

/*****************************************************************************
  Name          :  avd_send_admin_state_chg_ntf

  Description   :  This function generates a Admin State Change notification

  Arguments     :  name - Pointer to the specific DN
                   minor_id - Notification Class Identifier
                   old_state - Previous Administrative State
                   new_state - Present Administrative State

  Return Values :

  Notes         :
*****************************************************************************/
void avd_send_admin_state_chg_ntf(const SaNameT *name, SaAmfNotificationMinorIdT minor_id,
		SaAmfAdminStateT old_state, SaAmfAdminStateT new_state)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "Admin state of %s changed", name->value);
	sendStateChangeNotificationAvd(avd_cb,
					*name,
					(SaUint8T*)add_text,
					SA_SVC_AMF,
					minor_id,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_AMF_ADMIN_STATE,
					new_state,
					NULL,
					0);

}

/*****************************************************************************
  Name          :  avd_send_oper_chg_ntf

  Description   :  This function generates a Operational State Change notification.

  Arguments     :  name - Pointer to the specific DN
                   minor_id - Notification Class Identifier
                   old_state - Previous Operational State
                   new_state - Present Operational State

  Return Values :

  Notes         :
*****************************************************************************/
void avd_send_oper_chg_ntf(const SaNameT *name, SaAmfNotificationMinorIdT minor_id,
		SaAmfOperationalStateT old_state, SaAmfOperationalStateT new_state)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "Oper state %s changed", name->value);
	sendStateChangeNotificationAvd(avd_cb,
					*name,
					(SaUint8T*)add_text,
					SA_SVC_AMF,
					minor_id,
					SA_NTF_OBJECT_OPERATION,
					SA_AMF_OP_STATE,
					new_state,
					NULL,
					0);
}

/*****************************************************************************
  Name          :  avd_gen_su_pres_state_chg_ntf

  Description   :  This function sends a SU Presence State Change notification.

  Arguments     :  su_name - Pointer to the SU DN
                   old_state - Previous Presence State
                   new_state - Present Presence State

  Return Values :

  Notes         :
*****************************************************************************/
void avd_send_su_pres_state_chg_ntf(const SaNameT *su_name, 
		SaAmfPresenceStateT old_state, SaAmfPresenceStateT new_state)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "Presence state of SU %s changed",
			su_name->value);
	sendStateChangeNotificationAvd(avd_cb,
					*su_name,
					(SaUint8T*)add_text,
					SA_SVC_AMF,
					SA_AMF_NTFID_SU_PRESENCE_STATE,
					SA_NTF_OBJECT_OPERATION,
					SA_AMF_PRESENCE_STATE,
					new_state,
					NULL,
					0);

}

/*****************************************************************************
  Name          :  avd_send_su_ha_state_chg_ntf

  Description   :  This function sends a SU HA State Change notification

  Arguments     :  su_name - Pointer to the SU DN
                   si_name - Pointer to the SI DN 
                   old_state - Previous HA State
                   new_state - Present HA State

  Return Values :

  Notes         :  
*****************************************************************************/
void avd_send_su_ha_state_chg_ntf(const SaNameT *su_name, 
		const SaNameT *si_name, 
		SaAmfHAStateT old_state, 
		SaAmfHAStateT new_state)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "The HA state of SI %s assigned to SU %s changed",
			si_name->value, su_name->value);
	sendStateChangeNotificationAvd(avd_cb,
					*su_name,
					(SaUint8T*)add_text,
					SA_SVC_AMF,
					SA_AMF_NTFID_SU_SI_HA_STATE,
					SA_NTF_OBJECT_OPERATION,
					SA_AMF_HA_STATE,
					new_state,
					(NCSCONTEXT)si_name,
					2 /* Si_name */);

}

/*****************************************************************************
  Name          :  avd_send_si_ha_readiness_state_chg_ntf

  Description   :  This function sends a SU HA Readiness State Change notification

  Arguments     :  su_name - Pointer to the SU DN
                   si_name - Pointer to the SI DN 
                   old_state - Previous HA Readiness State
                   new_state - Present HA Readiness State

  Return Values :

  Notes         :  
*****************************************************************************/
void avd_send_su_ha_readiness_state_chg_ntf(const SaNameT *su_name, const SaNameT *si_name, 
		SaAmfHAReadinessStateT old_state, SaAmfHAReadinessStateT new_state)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "The HA readiness state of SI %s assigned"
			" to SU %s changed", si_name->value, su_name->value);
	sendStateChangeNotificationAvd(avd_cb,
					*su_name,
					(SaUint8T*)add_text,
					SA_SVC_AMF,
					SA_AMF_NTFID_SU_SI_HA_READINESS_STATE,
					SA_NTF_OBJECT_OPERATION,
					SA_AMF_HA_READINESS_STATE,
					new_state,
					(NCSCONTEXT)si_name,
					2 /* Si_name */);

}

/*****************************************************************************
  Name          :  avd_send_si_assigned_ntf

  Description   :  This function sends a SI Assignment State Change notification

  Arguments     :  si_name - Pointer to the SI DN
                   old_state - Previous Assignment State
                   new_state - Present Assignment State

  Return Values :

  Notes         :  
*****************************************************************************/
void avd_send_si_assigned_ntf(const SaNameT *si_name, SaAmfAssignmentStateT old_state,
		SaAmfAssignmentStateT new_state)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "The Assignment state of SI %s changed", si_name->value);
	sendStateChangeNotificationAvd(avd_cb,
					*si_name,
					(SaUint8T*)add_text,
					SA_SVC_AMF,
					SA_AMF_NTFID_SI_ASSIGNMENT_STATE,
					SA_NTF_OBJECT_OPERATION,
					SA_AMF_ASSIGNMENT_STATE,
					new_state,
					NULL,
					0);

}

/*****************************************************************************
  Name          :  avd_send_comp_proxy_status_proxied_ntf

  Description   :  This function sends a 'Proxy Status of a Component Changed to
                   Proxied' notification when a component once again is proxied.

  Arguments     :  comp_name - Pointer to the Component DN
                   old_status - Previous  proxy status
                   new_status - Present proxy status

  Return Values :

  Notes         :
*****************************************************************************/
void avd_send_comp_proxy_status_proxied_ntf(const SaNameT *comp_name,
		SaAmfProxyStatusT old_status, SaAmfProxyStatusT new_status)
{
	char add_text[ADDITION_TEXT_LENGTH];

	TRACE_ENTER();

	snprintf(add_text, ADDITION_TEXT_LENGTH, "Component %s is now proxied", comp_name->value);

	sendStateChangeNotificationAvd(avd_cb,
					*comp_name,
					(SaUint8T*)add_text,
					SA_SVC_AMF,
					SA_AMF_NTFID_COMP_PROXY_STATUS,
					SA_NTF_OBJECT_OPERATION,
					SA_AMF_PROXY_STATUS,
					new_status,
					NULL,
					0);

}

/*****************************************************************************
  Name          :  avd_alarm_clear

  Description   :  This function sends a alarm clear for a specific previous 
                   sent alarm.

  Arguments     :  name - Pointer to the specific DN
                   minor_id - Notification Class Identifier
                   probableCause - same as cause for original alarm

  Return Values :

  Notes         :
*****************************************************************************/
void avd_alarm_clear(const SaNameT *name, SaUint16T minorId, uint32_t probableCause)
{
       char add_text[ADDITION_TEXT_LENGTH];

       TRACE_ENTER();
       snprintf(add_text, ADDITION_TEXT_LENGTH, "Previous raised alarm of %s is now cleared", name->value);

       sendAlarmNotificationAvd(avd_cb,
	       *name,
	       (SaUint8T*)add_text,
	       SA_SVC_AMF,
	       minorId,
	       probableCause,
	       SA_NTF_SEVERITY_CLEARED,
	       NULL,
	       0);
}

SaAisErrorT fill_ntf_header_part_avd(SaNtfNotificationHeaderT *notificationHeader,
			      SaNtfEventTypeT eventType,
			      SaNameT comp_name,
			      SaUint8T *add_text,
			      SaUint16T majorId,
			      SaUint16T minorId,
			      SaInt8T *avd_name,
			      NCSCONTEXT add_info,
			      int type,
			      SaNtfNotificationHandleT notificationHandle)
{

	*notificationHeader->eventType = eventType;
	*notificationHeader->eventTime = (SaTimeT)SA_TIME_UNKNOWN;

	notificationHeader->notificationObject->length = comp_name.length;
	(void)memcpy(notificationHeader->notificationObject->value, comp_name.value, comp_name.length);

	notificationHeader->notifyingObject->length = strlen(avd_name);
	(void)memcpy(notificationHeader->notifyingObject->value, avd_name, strlen(avd_name));

	notificationHeader->notificationClassId->vendorId = SA_NTF_VENDOR_ID_SAF;
	notificationHeader->notificationClassId->majorId = majorId;
	notificationHeader->notificationClassId->minorId = minorId;

	(void)strcpy(notificationHeader->additionalText, (SaInt8T*)add_text);

	/* Fill the additional info if present */
	if (type != 0) {
		SaStringT dest_ptr;
		SaAisErrorT ret;
		SaNameT *name = (SaNameT*)(add_info);
		if (type == 1) {
			/* node_name */
			notificationHeader->additionalInfo[0].infoId = SA_AMF_NODE_NAME;
			notificationHeader->additionalInfo[0].infoType = SA_NTF_VALUE_LDAP_NAME;

		} else if (type == 2) {
			/* si_name */
			notificationHeader->additionalInfo[0].infoId = SA_AMF_SI_NAME;
			notificationHeader->additionalInfo[0].infoType = SA_NTF_VALUE_LDAP_NAME;

		}
		ret = saNtfPtrValAllocate(notificationHandle,
				sizeof (SaNameT) + 1,
				(void**)&dest_ptr,
				&(notificationHeader->additionalInfo[0].infoValue));
				
		if (ret != SA_AIS_OK) {
			LOG_ER("%s: saNtfPtrValAllocate Failed (%u)", __FUNCTION__, ret);
			return NCSCC_RC_FAILURE;
		}

		memcpy(dest_ptr, name, sizeof(SaNameT));
	}
	return SA_AIS_OK;

}

uint32_t sendAlarmNotificationAvd(AVD_CL_CB *avd_cb,
			       SaNameT ntf_object,
			       SaUint8T *add_text,
			       SaUint16T majorId,
			       SaUint16T minorId,
			       uint32_t probableCause,
			       uint32_t perceivedSeverity,
			       NCSCONTEXT add_info,
			       int type)
{
	uint32_t status = NCSCC_RC_FAILURE;
	SaNtfAlarmNotificationT myAlarmNotification;
	SaUint16T add_info_items = 0;
	SaUint64T allocation_size = 0;

	if (!avd_cb->active_services_exist) {
		// TODO #3051
		LOG_ER("Alarm lost for %s", ntf_object.value);
		return status;
	}

	if (type != 0) {
		add_info_items = 1;
		allocation_size = SA_NTF_ALLOC_SYSTEM_LIMIT;
	}

	status = saNtfAlarmNotificationAllocate(avd_cb->ntfHandle, &myAlarmNotification,
						/* numCorrelatedNotifications */
						0,
						/* lengthAdditionalText */
						strlen((char*)add_text)+1,
						/* numAdditionalInfo */
						add_info_items,
						/* numSpecificProblems */
						0,
						/* numMonitoredAttributes */
						0,
						/* numProposedRepairActions */
						0,
						/*variableDataSize */
						allocation_size);

	if (status != SA_AIS_OK) {
		LOG_ER("%s: saNtfAlarmNotificationAllocate Failed (%u)", __FUNCTION__, status);
		return NCSCC_RC_FAILURE;
	}

	status = fill_ntf_header_part_avd(&myAlarmNotification.notificationHeader,
				 SA_NTF_ALARM_PROCESSING,
				 ntf_object,
				 add_text,
				 majorId,
				 minorId,
				 AMF_NTF_SENDER,
				 add_info,
				 type,
				 myAlarmNotification.notificationHandle);
	
	if (status != SA_AIS_OK) {
		LOG_ER("%s: fill_ntf_header_part_avd Failed (%u)", __FUNCTION__, status);
		saNtfNotificationFree(myAlarmNotification.notificationHandle);
		return NCSCC_RC_FAILURE;
	}

	*(myAlarmNotification.probableCause) = probableCause;
	*(myAlarmNotification.perceivedSeverity) = perceivedSeverity;

	status = saNtfNotificationSend(myAlarmNotification.notificationHandle);

	if (status != SA_AIS_OK) {
		saNtfNotificationFree(myAlarmNotification.notificationHandle);
		LOG_ER("%s: saNtfNotificationSend Failed (%u)", __FUNCTION__, status);
		return NCSCC_RC_FAILURE;
	}

	status = saNtfNotificationFree(myAlarmNotification.notificationHandle);

	if (status != SA_AIS_OK) {
		LOG_ER("%s: saNtfNotificationFree Failed (%u)", __FUNCTION__, status);
		return NCSCC_RC_FAILURE;
	}

	return status;

}

uint32_t sendStateChangeNotificationAvd(AVD_CL_CB *avd_cb,
				     SaNameT ntf_object,
				     SaUint8T *add_text,
				     SaUint16T majorId,
				     SaUint16T minorId,
				     uint32_t sourceIndicator,
				     SaUint16T stateId,
				     SaUint16T newState,
				     NCSCONTEXT add_info,
				     int type)
{
	uint32_t status = NCSCC_RC_FAILURE;
	SaNtfStateChangeNotificationT myStateNotification;
	SaUint16T add_info_items = 0;
	SaUint64T allocation_size = 0;

	if (!avd_cb->active_services_exist) {
		// TODO #3051
		LOG_WA("State change notification lost for '%s'", ntf_object.value);
		return status;
	}

	if (type != 0) {
		add_info_items = 1;
		allocation_size = SA_NTF_ALLOC_SYSTEM_LIMIT;
	}

	status = saNtfStateChangeNotificationAllocate(avd_cb->ntfHandle,/* handle to Notification Service instance */
						      &myStateNotification,
						      /* number of correlated notifications */
						      0,
						      /* length of additional text */
						      strlen((char*)add_text)+1,
						      /* number of additional info items */
						      add_info_items,
						      /* number of state changes */
						      1,
						      /* use default allocation size */
						      allocation_size);

	if (status != SA_AIS_OK) {
		LOG_ER("%s: saNtfStateChangeNotificationAllocate Failed (%u)", __FUNCTION__, status);
		return NCSCC_RC_FAILURE;
	}

	status = fill_ntf_header_part_avd(&myStateNotification.notificationHeader,
				 SA_NTF_OBJECT_STATE_CHANGE,
				 ntf_object,
				 add_text,
				 majorId,
				 minorId,
				 AMF_NTF_SENDER,
				 add_info,
				 type,
				 myStateNotification.notificationHandle);
	
	if (status != SA_AIS_OK) {
		LOG_ER("%s: fill_ntf_header_part_avd Failed (%u)", __FUNCTION__, status);
		saNtfNotificationFree(myStateNotification.notificationHandle);
		return NCSCC_RC_FAILURE;
	}

	*(myStateNotification.sourceIndicator) = sourceIndicator;
	myStateNotification.changedStates->stateId = stateId;
	myStateNotification.changedStates->oldStatePresent = SA_FALSE;
	myStateNotification.changedStates->newState = newState;

	status = saNtfNotificationSend(myStateNotification.notificationHandle);

	if (status != SA_AIS_OK) {
		saNtfNotificationFree(myStateNotification.notificationHandle);
		LOG_ER("%s: saNtfNotificationSend Failed (%u)", __FUNCTION__, status);
		return NCSCC_RC_FAILURE;
	}

	status = saNtfNotificationFree(myStateNotification.notificationHandle);

	if (status != SA_AIS_OK) {
		LOG_ER("%s: saNtfNotificationFree Failed (%u)", __FUNCTION__, status);
		return NCSCC_RC_FAILURE;
	}

	return status;

}
