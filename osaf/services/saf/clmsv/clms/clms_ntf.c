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

#include "clms.h"

#define ADDITION_TEXT_LENGTH 256
#define CLMS_NTF_SENDER "safApp=safClmService"

static const char *clm_adm_state_name[] = {
	"INVALID",
	"UNLOCKED",
	"LOCKED",
	"SHUTTING_DOWN"
};

const unsigned int delay_ms = 500;
const unsigned int max_wait_time_ms = 5 * 1000;     /* 5 seconds */

static void fill_ntf_header_part_clms(SaNtfNotificationHeaderT *notificationHeader,
				      SaNtfEventTypeT eventType,
				      SaNameT node_name,
				      SaUint8T *add_text, SaUint16T majorId, SaUint16T minorId, SaInt8T *clm_node)
{
	*notificationHeader->eventType = eventType;
	*notificationHeader->eventTime = (SaTimeT)SA_TIME_UNKNOWN;

	notificationHeader->notificationObject->length = node_name.length;
	(void)memcpy(notificationHeader->notificationObject->value, node_name.value, node_name.length);

	notificationHeader->notifyingObject->length = strlen(clm_node);
	(void)memcpy(notificationHeader->notifyingObject->value, clm_node, strlen(clm_node));

	notificationHeader->notificationClassId->vendorId = SA_NTF_VENDOR_ID_SAF;
	notificationHeader->notificationClassId->majorId = majorId;
	notificationHeader->notificationClassId->minorId = minorId;

	(void)strcpy(notificationHeader->additionalText, (SaInt8T *)add_text);

}

static uint32_t sendStateChangeNotificationClms(CLMS_CB * clms_cb,
					     SaNameT node_name,
					     SaUint8T *add_text,
					     SaUint16T majorId,
					     SaUint16T minorId,
					     uint32_t sourceIndicator, SaUint32T stateId, SaUint32T newState)
{
	uint32_t status = NCSCC_RC_FAILURE;
	int msecs_waited;
	SaNtfStateChangeNotificationT myStateNotification;

	status = saNtfStateChangeNotificationAllocate(clms_cb->ntf_hdl,	/* handle to Notification Service instance */
						      &myStateNotification,
						      /* number of correlated notifications */
						      0,
						      /* length of additional text */
						      ADDITION_TEXT_LENGTH,
						      /* number of additional info items */
						      0,
						      /* number of state changes */
						      1,
						      /* use default allocation size */
						      0);

	if (status != SA_AIS_OK) {
		LOG_ER("saNtfStateChangeNotificationAllocate() returned: %s", saf_error(status));
		return NCSCC_RC_FAILURE;
	}

	fill_ntf_header_part_clms(&myStateNotification.notificationHeader,
				  SA_NTF_OBJECT_STATE_CHANGE, node_name, add_text, majorId, minorId, CLMS_NTF_SENDER);

	*(myStateNotification.sourceIndicator) = sourceIndicator;
	myStateNotification.changedStates->stateId = stateId;
	myStateNotification.changedStates->oldStatePresent = SA_FALSE;
	myStateNotification.changedStates->newState = newState;

	msecs_waited = 0;
	status = saNtfNotificationSend(myStateNotification.notificationHandle);
	while ((status == SA_AIS_ERR_TRY_AGAIN) &&
						(msecs_waited < max_wait_time_ms)) {
		usleep(delay_ms * 1000);
		msecs_waited += delay_ms;
		status = saNtfNotificationSend(myStateNotification.notificationHandle);
	}

	if (status != SA_AIS_OK) {
		LOG_ER("saNtfNotificationSend() returned: %s", saf_error(status));
		status = saNtfNotificationFree(myStateNotification.notificationHandle);
		if (status != SA_AIS_OK)
			LOG_ER("saNtfNotificationFree() returned: %s", saf_error(status));
		return NCSCC_RC_FAILURE;
	}

	status = saNtfNotificationFree(myStateNotification.notificationHandle);

	if (status != SA_AIS_OK) {
		LOG_ER("saNtfNotificationFree() returned: %s", saf_error(status));
		return NCSCC_RC_FAILURE;
	}

	return status;

}

/*****************************************************************************
  Name          :  clms_node_join_ntf

  Description   :  This function generate a node join ntf

  Arguments     :  clms_cb -    Pointer to the CLMS_CB structure
                   node -      Pointer to the CLMS_CLUSTER_NODE  structure

  Return Values :  NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE

  Notes         : 
*****************************************************************************/
void clms_node_join_ntf(CLMS_CB * clms_cb, CLMS_CLUSTER_NODE * node)
{
	SaNameT dn;
	SaUint8T add_text[SA_MAX_NAME_LENGTH];

	memset(dn.value, '\0', SA_MAX_NAME_LENGTH);
	dn.length = node->node_name.length;
	(void)memcpy(dn.value, node->node_name.value, dn.length);

	TRACE("Notification for CLM node %s Join", dn.value);
	saflog(LOG_NOTICE, clmSvcUsrName, "%s JOINED, init view=%llu, cluster view=%llu",
		   node->node_name.value, node->init_view, clms_cb->cluster_view_num);

	memset(&add_text, '\0', sizeof(add_text));
	sprintf((SaInt8T *)add_text, "CLM node %s Joined", dn.value);

	sendStateChangeNotificationClms(clms_cb,
					dn,
					add_text,
					SA_SVC_CLM,
					SA_CLM_NTFID_NODE_JOIN,
					SA_NTF_OBJECT_OPERATION,
					SA_CLM_CLUSTER_CHANGE_STATUS, SA_CLM_NODE_JOINED);

}

/*****************************************************************************
  Name          :  clms_node_exit_ntf

  Description   :  This function generate a node exit ntf

  Arguments     :  clms_cb -    Pointer to the CLMS_CB structure
                   node -      Pointer to the CLMS_CLUSTER_NODE  structure

  Return Values :  NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE

  Notes         : 
*****************************************************************************/
void clms_node_exit_ntf(CLMS_CB * clms_cb, CLMS_CLUSTER_NODE * node)
{
	SaNameT dn;
	SaUint8T add_text[SA_MAX_NAME_LENGTH];

	memset(dn.value, '\0', SA_MAX_NAME_LENGTH);
	dn.length = node->node_name.length;
	(void)memcpy(dn.value, node->node_name.value, dn.length);

	TRACE("Notification for CLM node %s exit", dn.value);
	saflog(LOG_NOTICE, clmSvcUsrName, "%s LEFT, init view=%llu, cluster view=%llu",
		   node->node_name.value, node->init_view, clms_cb->cluster_view_num);

	memset(&add_text, '\0', sizeof(add_text));
	sprintf((SaInt8T *)add_text, "CLM node %s Exit", dn.value);

	sendStateChangeNotificationClms(clms_cb,
					dn,
					add_text,
					SA_SVC_CLM,
					SA_CLM_NTFID_NODE_LEAVE,
					SA_NTF_OBJECT_OPERATION,
					SA_CLM_CLUSTER_CHANGE_STATUS, SA_CLM_NODE_LEFT);

}

/*****************************************************************************
  Name          :  clms_node_reconfigured_ntf

  Description   :  This function generate a node exit ntf

  Arguments     :  clms_cb -    Pointer to the CLMS_CB structure
                   node -      Pointer to the CLMS_CLUSTER_NODE  structure

  Return Values :  NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE

  Notes         : 
*****************************************************************************/
void clms_node_reconfigured_ntf(CLMS_CB * clms_cb, CLMS_CLUSTER_NODE * node)
{
	SaNameT dn;
	SaUint8T add_text[SA_MAX_NAME_LENGTH];

	memset(dn.value, '\0', SA_MAX_NAME_LENGTH);
	saflog(LOG_NOTICE, clmSvcUsrName, "%s RECONFIGURED, init view=%llu, cluster view=%llu",
		   node->node_name.value, node->init_view, clms_cb->cluster_view_num);
	dn.length = node->node_name.length;
	(void)memcpy(dn.value, node->node_name.value, dn.length);

	memset(&add_text, '\0', sizeof(add_text));
	sprintf((SaInt8T *)add_text, "CLM node %s Reconfigured", dn.value);

	sendStateChangeNotificationClms(clms_cb,
					dn,
					add_text,
					SA_SVC_CLM,
					SA_CLM_NTFID_NODE_RECONFIG,
					SA_NTF_OBJECT_OPERATION,
					SA_CLM_CLUSTER_CHANGE_STATUS, SA_CLM_NODE_RECONFIGURED);

}

/*****************************************************************************
  Name          :  clms_node_admin_state_change_ntf

  Description   :  This function generate a node exit ntf

  Arguments     :  clms_cb -    Pointer to the CLMS_CB structure
                   node -      Pointer to the CLMS_CLUSTER_NODE  structure

  Return Values :  NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE

  Notes         : 
*****************************************************************************/
void clms_node_admin_state_change_ntf(CLMS_CB * clms_cb, CLMS_CLUSTER_NODE * node, SaUint32T newState)
{
	SaNameT dn;
	SaUint8T add_text[SA_MAX_NAME_LENGTH];

	TRACE_ENTER2("admin state change for node name %s", node->node_name.value);

	saflog(LOG_NOTICE, clmSvcUsrName, "%s Admin State Changed, new state=%s",
		   node->node_name.value, clm_adm_state_name[newState]);

	memset(dn.value, '\0', SA_MAX_NAME_LENGTH);
	dn.length = node->node_name.length;
	(void)memcpy(dn.value, node->node_name.value, dn.length);

	memset(&add_text, '\0', sizeof(add_text));
	sprintf((SaInt8T *)add_text, "CLM node %s Admin State Change", dn.value);

	sendStateChangeNotificationClms(clms_cb,
					dn,
					add_text,
					SA_SVC_CLM,
					SA_CLM_NTFID_NODE_ADMIN_STATE,
					SA_NTF_MANAGEMENT_OPERATION, SA_CLM_ADMIN_STATE, newState);

	TRACE_LEAVE();
}

SaAisErrorT clms_ntf_init(CLMS_CB * cb)
{

	SaAisErrorT rc = SA_AIS_OK;
	SaVersionT ntfVersion = { 'A', 0x01, 0x01 };

	TRACE_ENTER();

	rc = saNtfInitialize(&cb->ntf_hdl, NULL, &ntfVersion);
	TRACE("saNtfInitialize rc value %u", rc);
	if (rc != SA_AIS_OK) {
		LOG_ER("saNtfInitialize Failed (%u)", rc);
	}

	TRACE_LEAVE();
	return rc;
}
