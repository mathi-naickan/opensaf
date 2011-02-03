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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
*                                                                            *
*  MODULE NAME:  plm_notifications.c                                         *
*                                                                            *
*                                                                            *
*  DESCRIPTION                                                               *
*  This module contains the utility functions for sending Notifications and  *
*  Alarms.                                                                   *
*  PLM service uses the following wrapper functions to send Notification.    *
*  The Notifications/alarms supported are                                    *
*  StateChange Notifications, HPI resource and  Hotswap Notifications        *
*  Hardware Entity Alarm, EE ALarm, Unmapped HE Alarm                        *
*                                                                            *
*****************************************************************************/

#include "plms.h"
#include "plms_notifications.h"

/*******************************************************************************
 * Name        : plms_state_change_ntf_send 
 *
 * Description : This is the utility function to send state Change notifications
 *
 * Arguments     :
 *                 plmNtfHandle,
 *                 object,
 *                 ntfIdPtr,
 *                 sourceIndicator,
 *                 noOfStateChanges,
 *                 stateChanges,
 *                 minorId,
 *                 no_of_corr_notifications,
 *                 *corr_ids,
 *                 dnName
 *
 * Return Values :
 *
********************************************************************************/


SaAisErrorT plms_state_change_ntf_send(SaNtfHandleT      plm_ntf_hdl,
                                           SaNameT               *object,
                                           SaNtfIdentifierT      *ntf_id,
                                           SaUint16T             source_indicator,
                                           SaUint16T             no_of_state_changes,
                                           SaPlmNtfStateChangeT  *state_changes,
                                           SaUint16T             minor_id,
					   SaUint16T   no_of_corr_notifications,
					   SaNtfIdentifierT     *corr_ids,
                                           SaNameT               *dn_name /* DN of root state change PLM entity obj */
                                           )
{
	SaNtfStateChangeNotificationT   plm_notification;
	SaAisErrorT                     ret;
	SaUint16T                       ii;
	SaPlmNtfStateChangeT           *tmp_state;
	SaUint16T                       add_info_items = 0;
	SaStringT                       dest_ptr;
	SaUint16T			ntf_obj_len = 0;

	if (minor_id == SA_PLM_NTFID_STATE_CHANGE_DEP) {
		add_info_items = 1;
	}

	ret = saNtfStateChangeNotificationAllocate(
					    plm_ntf_hdl,
					    &plm_notification,
					    no_of_corr_notifications, /* No of correlated notifs */
					    0, /* Len of additional text */
					    add_info_items, 
					    no_of_state_changes,
					    SA_NTF_ALLOC_SYSTEM_LIMIT  /* variable data size*/
					    );
	if (SA_AIS_OK != ret)  {
		LOG_ER("State Change Notification allocation failed");
		/* Put code to process failure cond and return error */
		return ret;
	} 

	*(plm_notification.notificationHeader.eventType) = SA_NTF_OBJECT_STATE_CHANGE;
	*(plm_notification.notificationHeader.eventTime) = (SaTimeT)SA_TIME_UNKNOWN; /* FIXME */; /* Put current time */

	/* Copy noticationObject details give above */
	plm_notification.notificationHeader.notificationObject->length = object->length;
	memcpy(plm_notification.notificationHeader.notificationObject->value,object->value,object->length);

	/* Copy notifyingObject details give above */
	memset(plm_notification.notificationHeader.notifyingObject,0,sizeof(SaNameT));
	ntf_obj_len = strlen("safApp=safPlmService"); 
	memcpy(plm_notification.notificationHeader.notifyingObject->value,"safApp=safPlmService",ntf_obj_len);
	plm_notification.notificationHeader.notifyingObject->length = ntf_obj_len; 

	/* Fill in the class identifier details */
	plm_notification.notificationHeader.notificationClassId->vendorId = SA_NTF_VENDOR_ID_SAF;
	plm_notification.notificationHeader.notificationClassId->majorId = SA_SVC_PLM;
	plm_notification.notificationHeader.notificationClassId->minorId = minor_id;

	/* Fill in the source Indictor */
	*(plm_notification.sourceIndicator) = source_indicator;

	/* Fill in the state changes */
        plm_notification.numStateChanges = no_of_state_changes;

	tmp_state = state_changes;

	for ( ii = 0; ii < no_of_state_changes; ii++)  {

		if (NULL == tmp_state) {
			saNtfNotificationFree(plm_notification.notificationHandle);
			return SA_AIS_ERR_INVALID_PARAM; 
		}
		plm_notification.changedStates[ii].stateId = tmp_state->state.stateId;
		plm_notification.changedStates[ii].oldStatePresent = tmp_state->state.oldStatePresent;
		plm_notification.changedStates[ii].oldState = tmp_state->state.oldState;
		plm_notification.changedStates[ii].newState = tmp_state->state.newState;  

		tmp_state = tmp_state->next;
	}

	/* Fill in thd additional info */
	if (minor_id == SA_PLM_NTFID_STATE_CHANGE_DEP) {
		if ( !dn_name)
		{
			LOG_ER("Root DN not specified");
			saNtfNotificationFree(plm_notification.notificationHandle);
			return SA_AIS_ERR_INVALID_PARAM;
		}
		plm_notification.notificationHeader.additionalInfo[0].infoId = SA_PLM_AI_ROOT_OBJECT;
		plm_notification.notificationHeader.additionalInfo[0].infoType = SA_NTF_VALUE_LDAP_NAME;

		ret = saNtfPtrValAllocate(plm_notification.notificationHandle,
			                  sizeof (SaNameT) + 1,
			                  (void**)&dest_ptr,
			                  &(plm_notification.notificationHeader.additionalInfo[0].infoValue)
			                  );
		if (ret != SA_AIS_OK) {
			LOG_ER("saNtfPtrValAllocate Failed (%u)", ret);
			saNtfNotificationFree(plm_notification.notificationHandle);
			return ret;
		}
		memcpy(dest_ptr,dn_name,sizeof(SaNameT));
	}

	/* Send the notification */
	ret = saNtfNotificationSend(plm_notification.notificationHandle);

	if (ret != SA_AIS_OK)
	{
		LOG_ER("State change Notification send failed");
		saNtfNotificationFree(plm_notification.notificationHandle);
		return ret;
	}

	if (ntf_id)
	{
		*ntf_id = *(plm_notification.notificationHeader.notificationId);
	}
	/* Free the notification allocated above */
	saNtfNotificationFree(plm_notification.notificationHandle);

	return ret;
}

/*******************************************************************************
 * Name        : plms_hpi_evt_ntf_send 
 *
 * Description : This is the utility function to send HPI Event notifications.
 *
 * Arguments     :
 *                 plmNtfHandle(input)
 *                 object(input)
 *                 eventType(input)
 *                 entityPath(input)
 *                 domainId(input)
 *                 hpiEvent(input)
 *                 ntfIdPtr(output)
 *
 * Return Values :
 *
*******************************************************************************/


/* Need to discuss abt no of additional parameters */
SaAisErrorT plms_hpi_evt_ntf_send(SaNtfHandleT      plm_ntf_hdl,
                                        SaNameT           *object,
                                        SaUint32T         event_type,
                                        SaInt8T           *entity_path,
                                        SaUint32T         domain_id,
                                        SaHpiEventT       *hpi_event,
					SaUint16T      no_of_corr_notifications,
					SaNtfIdentifierT *corr_ids,
                                        SaNtfIdentifierT  *ntf_id
                                        )
{    
/*	SaNtfMiscellaneousNotificationT plm_notification; */
	SaNtfStateChangeNotificationT   plm_notification;
	SaAisErrorT                     ret = NCSCC_RC_SUCCESS; 
	SaStringT                       dest_ptr1 = NULL;
	SaUint16T			ntf_obj_len = 0;

	/* FIXME : Need to enable, when the following API is supported */
#if 0
	ret = saNtfMiscellaneousNotificationAllocate(plm_ntf_hdl,
				                     &plm_notification,
				                     no_of_corr_notifications, 
				                     0, /* FIXME Length of additional text */
				                     3, /* FIXME At present fixed it to 3*/
				                     SA_NTF_ALLOC_SYSTEM_LIMIT /* FIXME Variable data size*/ 
				                     );
#endif 
	ret = saNtfStateChangeNotificationAllocate(
					    plm_ntf_hdl,
					    &plm_notification,
					    no_of_corr_notifications, /* No of correlated notifs */
					    0, /* Len of additional text */
					    2, /* At present fixed it to 2, only mandatory fields. */ 
					    0,
					    SA_NTF_ALLOC_SYSTEM_LIMIT  /* variable data size*/
					    );
	if (SA_AIS_OK != ret)  {
		LOG_ER("HPI Event Notification allocation failed");
		return ret;
	} 

	switch ( event_type ) {
		case SAHPI_ET_RESOURCE:
		case SAHPI_ET_HOTSWAP:
			*(plm_notification.notificationHeader.eventType) = SA_NTF_HPI_EVENT_RESOURCE;
			break;
		case SAHPI_ET_SENSOR:
		case SAHPI_ET_SENSOR_ENABLE_CHANGE:
			*(plm_notification.notificationHeader.eventType) = SA_NTF_HPI_EVENT_SENSOR;
			break;
		case SAHPI_ET_WATCHDOG:
			*(plm_notification.notificationHeader.eventType) = SA_NTF_HPI_EVENT_WATCHDOG;
			break;
		case SAHPI_ET_DIMI:
			*(plm_notification.notificationHeader.eventType) = SA_NTF_HPI_EVENT_DIMI;
			break;
		case SAHPI_ET_FUMI:
			*(plm_notification.notificationHeader.eventType) = SA_NTF_HPI_EVENT_FUMI;        
			break;
		default :
			ret = NCSCC_RC_FAILURE; 
			return ret;
	}

	*(plm_notification.notificationHeader.eventTime) = (SaTimeT)SA_TIME_UNKNOWN; 
		
	/* Copy noticationObject details give above */
	plm_notification.notificationHeader.notificationObject->length = object->length;
	memcpy(plm_notification.notificationHeader.notificationObject->value,object->value,object->length);
	
	/* Initialize notifying object details. */
	ntf_obj_len = strlen("safApp=safPlmService"); 
	memset(plm_notification.notificationHeader.notifyingObject,0,sizeof(SaNameT));
	memcpy(plm_notification.notificationHeader.notifyingObject->value,"safApp=safPlmService",ntf_obj_len);
	plm_notification.notificationHeader.notifyingObject->length = ntf_obj_len; 

	/* Fill in the class identifier details */
	plm_notification.notificationHeader.notificationClassId->vendorId = SA_NTF_VENDOR_ID_SAF;
	plm_notification.notificationHeader.notificationClassId->majorId = SA_SVC_PLM;
	plm_notification.notificationHeader.notificationClassId->minorId = SA_PLM_NTFID_HPI_NORMAL_MSB; 

	/* Fill in the additional info */
	/* Domain Id first */
	plm_notification.notificationHeader.additionalInfo[0].infoId = SA_PLM_AI_HPI_DOMAIN_ID; 
	plm_notification.notificationHeader.additionalInfo[0].infoType = SA_NTF_VALUE_UINT32; 
	plm_notification.notificationHeader.additionalInfo[0].infoValue.uint32Val = domain_id;

	if (!hpi_event)
	{
		LOG_ER("HPI data not present");
		saNtfNotificationFree(plm_notification.notificationHandle);
		return NCSCC_RC_FAILURE;
	}

	/* Fill in mandatory information SaHpiEventT */
        /* For intoType = BINARY we need to allocate memory for ptrVal */
	plm_notification.notificationHeader.additionalInfo[1].infoId = SA_PLM_AI_HPI_EVENT_DATA; 
	plm_notification.notificationHeader.additionalInfo[1].infoType = SA_NTF_VALUE_BINARY; 

	ret = saNtfPtrValAllocate(plm_notification.notificationHandle,
	                          sizeof(SaHpiEventT),
	                          (void**)&dest_ptr1,
	                          &(plm_notification.notificationHeader.additionalInfo[1].infoValue)
	                          );

	if ( ret != SA_AIS_OK ) {
		saNtfNotificationFree(plm_notification.notificationHandle);
		return ret;
	}

	memcpy(dest_ptr1,hpi_event,sizeof(SaHpiEventT));
	
	/* Send the notification */
	ret = saNtfNotificationSend(plm_notification.notificationHandle);
	if (ret != SA_AIS_OK)
	{
		LOG_ER("HPI Event Notification send failed");
		saNtfNotificationFree(plm_notification.notificationHandle);
		return ret;
	}
	if(ntf_id)
	{
		*ntf_id = *(plm_notification.notificationHeader.notificationId);
	}
	/* Free the notification allocated above */
	saNtfNotificationFree(plm_notification.notificationHandle);

	return ret;
}

/*******************************************************************************
 * Name        : plms_alarm_ntf_send 
 *
 * Description : This is the utility function to send HE, EE and unmapped Alarms
 *
 * Arguments     :
 *                 plmNtfHandle(input)
 *                 object(input)
 *                 eventType(input)
 *                 entityPath(input)
 *                 severity(input)
 *                 cause(input)
 *                 minorId(input)
 *                 ntfIdPtr(output)
 *
 * Return Values :
 *
********************************************************************************/

SaAisErrorT plms_alarm_ntf_send(SaNtfHandleT  plm_ntf_hdl,
                                     SaNameT       *object,
                                     SaUint32T     event_type,
                                      SaInt8T      *entity_path,
                                     SaUint32T     severity,
                                     SaUint32T     cause,
                                     SaUint32T     minor_id,
				     SaUint16T         no_of_corr_notifications,
				     SaNtfIdentifierT *corr_ids,
                                     SaNtfIdentifierT *ntf_id
                                     )
{    
	SaNtfAlarmNotificationT   plm_notification;
	SaAisErrorT               ret;
	SaStringT                 dest_ptr = NULL;
	SaUint16T		  ntf_obj_len = 0;

	ret = saNtfAlarmNotificationAllocate(plm_ntf_hdl,
				             &plm_notification,
				             no_of_corr_notifications, /* FIXME no of corelated notifications */
			 	             0, /* FIXME Length of additional text */
				             1, /* FIXME assuming only entity path */
				             0, /* FIXME no of specific problems */
				             0, /* FIXME No of monitored attributes */
				             0, /* FIXME NO of proposed repair action */
				             SA_NTF_ALLOC_SYSTEM_LIMIT /* FIXME Variable data size */
				             );
	if (SA_AIS_OK != ret)  {
		LOG_ER("Alarm Notification allocation failed");
		return ret;	
	} 

	/* Fill the common parameters of HE, EE and unmapped HE Alarm */ 
	*(plm_notification.notificationHeader.eventType) = event_type;
	*(plm_notification.notificationHeader.eventTime) = 
			(SaTimeT)SA_TIME_UNKNOWN; /* FIXME */; /* Put current time */

	/* Copy noticationObject details give above */
	plm_notification.notificationHeader.notificationObject->length = 
								object->length;
	memcpy(plm_notification.notificationHeader.notificationObject->value,
						object->value,object->length);
	
	/* Initialize notifying object details. */
	ntf_obj_len = strlen("safApp=safPlmService"); 
	memset(plm_notification.notificationHeader.notifyingObject,0,sizeof(SaNameT));
	memcpy(plm_notification.notificationHeader.notifyingObject->value,"safApp=safPlmService",ntf_obj_len);
	plm_notification.notificationHeader.notifyingObject->length = ntf_obj_len;

	/* Fill in the class identifier details */
	plm_notification.notificationHeader.notificationClassId->vendorId = 
							SA_NTF_VENDOR_ID_SAF;
	plm_notification.notificationHeader.notificationClassId->majorId = 
							SA_SVC_PLM;
	plm_notification.notificationHeader.notificationClassId->minorId = 
							minor_id;
	/* Set the cause */
	*(plm_notification.probableCause) = cause;

	/* Set the severity */
	*(plm_notification.perceivedSeverity) = severity;

/* Fill the entity path if it is HE/unmapped HE Alarm */  
	if (event_type == SA_NTF_ALARM_EQUIPMENT){
		plm_notification.notificationHeader.additionalInfo[0].infoId = 
							SA_PLM_AI_ENTITY_PATH;
		plm_notification.notificationHeader.additionalInfo[0].infoType =
							SA_NTF_VALUE_STRING;

		ret = saNtfPtrValAllocate(plm_notification.notificationHandle,
			                  strlen(entity_path) + 1,
			                  (void**)&dest_ptr,
			                  &(plm_notification.notificationHeader.additionalInfo[0].infoValue)
			                  );

		if ( ret != SA_AIS_OK ) {
			saNtfNotificationFree(plm_notification.notificationHandle);
			return ret;
		}

		memcpy(dest_ptr,entity_path,strlen(entity_path));
	}

	ret = saNtfNotificationSend(plm_notification.notificationHandle);

	if ( ret != SA_AIS_OK ) {
		saNtfNotificationFree(plm_notification.notificationHandle);
		return ret;
	}
	
	if (ntf_id)
	{
		*ntf_id = *(plm_notification.notificationHeader.notificationId);
	}

	ret = saNtfNotificationFree(plm_notification.notificationHandle);

	return ret; 
  
}

