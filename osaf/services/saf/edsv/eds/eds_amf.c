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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................
  
    
..............................................................................
      
DESCRIPTION:
        
This include file contains AMF interaction logic for health-check and other
stuff.
*******************************************************************************/
#include "eds.h"

/* HA AMF statemachine & State handler definitions */

/****************************************************************************
 * Name          : eds_quiescing_state_handler
 *
 * Description   : This function is called upon receving an Quiescing state
 *                 assignment from AMF.
 *
 * Arguments     : invocation     - This parameter designated a particular
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it
 *                                  responds to the Avilability Management
 *                                  FrameWork using the saAmfResponse()
 *                                  function.
 *                 cb              -A pointer to the EDS control block.
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/

uint32_t eds_quiescing_state_handler(EDS_CB *cb, SaInvocationT invocation)
{
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();

	error = saAmfCSIQuiescingComplete(cb->amf_hdl, invocation, error);

	saAmfResponse(cb->amf_hdl, invocation, error);
	TRACE_LEAVE2("HA AMF QUIESCING state");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_quiesced_state_handler
 *
 * Description   : This function is called upon receving an Quiesced state
 *                 assignment from AMF.
 *
 * Arguments     : invocation     - This parameter designated a particular
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it
 *                                  responds to the Avilability Management
 *                                  FrameWork using the saAmfResponse()
 *                                  function.
 *                 cb              -A pointer to the EDS control block.
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/

uint32_t eds_quiesced_state_handler(EDS_CB *cb, SaInvocationT invocation)
{
	V_DEST_RL mds_role;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();

	mds_role = V_DEST_RL_QUIESCED;

	/** set the CB's anchor value & mds role */

	cb->mds_role = mds_role;
	eds_mds_change_role(cb);
	cb->amf_invocation_id = invocation;

	cb->is_quisced_set = true;
	TRACE("HA AMF QUIESCED state");

	/* Give up our implementer role */
	error = immutil_saImmOiImplementerClear(cb->immOiHandle);
	if (error != SA_AIS_OK)
		LOG_ER("saImmOiImplementerClear failed with rc = %d", error);
	else
		cb->is_impl_set = false;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 * Name          : eds_invalid_state_handler
 *
 * Description   : This function is called upon receving an invalid state(as
 *                 in the FSM) assignment from AMF.
 *
 * Arguments     : invocation     - This parameter designated a particular
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it
 *                                  responds to the Avilability Management
 *                                  FrameWork using the saAmfResponse()
 *                                  function.
 *                 cb              -A pointer to the EDS control block.
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/

uint32_t eds_invalid_state_handler(EDS_CB *cb, SaInvocationT invocation)
{
	saAmfResponse(cb->amf_hdl, invocation, SA_AIS_ERR_BAD_OPERATION);
	TRACE_LEAVE2("Invalid AMF state");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : eds_amf_health_chk_callback
 *
 * Description   : This is the callback function which will be called 
 *                 when the AMF framework needs to health check for the component.
 *
 * Arguments     : invocation     - This parameter designated a particular 
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it 
 *                                  responds to the Avilability Management 
 *                                  FrameWork using the saAmfResponse() 
 *                                  function.
 *                 compName       - A pointer to the name of the component 
 *                                  whose readiness stae the Availability 
 *                                  Management Framework is setting.
 *                 checkType      - The type of healthcheck to be executed. 
 *
 * Return Values : None
 *
 * Notes         : At present we are just support a simple liveness check.
 *****************************************************************************/
void eds_amf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	EDS_CB *eds_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();

	if (NULL == (eds_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		LOG_ER("Failed to retrieve eds handle: %u", gl_eds_hdl);
		TRACE_LEAVE();
		return;
	} else {
		saAmfResponse(eds_cb->amf_hdl, invocation, error);
		ncshm_give_hdl(gl_eds_hdl);
	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : eds_amf_CSI_set_callback
 *
 * Description   : AMF callback function called 
 *                 when there is any change in the HA state.
 *
 * Arguments     : invocation     - This parameter designated a particular 
 *                                  invocation of this callback function. The 
 *                                  invoke process return invocation when it 
 *                                  responds to the Avilability Management 
 *                                  FrameWork using the saAmfResponse() 
 *                                  function.
 *                 compName       - A pointer to the name of the component 
 *                                  whose readiness stae the Availability 
 *                                  Management Framework is setting.
 *                 haState        - The new HA state to be assumeb by the 
 *                                  component service instance identified by 
 *                                  csiName.
 *                 csiDescriptor - This will indicate whether or not the 
 *                                  component service instance for 
 *                                  ativeCompName went through quiescing.
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void
eds_amf_CSI_set_callback(SaInvocationT invocation,
			 const SaNameT *compName, SaAmfHAStateT new_haState, SaAmfCSIDescriptorT csiDescriptor)
{
	EDS_CB *eds_cb;
	SaAisErrorT error = SA_AIS_OK;
	SaAmfHAStateT prev_haState;
	bool role_change = true;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (NULL == (eds_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		LOG_ER("Failed to retrieve eds handle: %u", gl_eds_hdl);
		TRACE_LEAVE();
		return;
	} else {
		/*
		 *  Handle Active to Active role change.
		 */
		prev_haState = eds_cb->ha_state;
		TRACE_1("Previous HA State: %d", prev_haState);

		/* Invoke the appropriate state handler routine */
		switch (new_haState) {
		case SA_AMF_HA_ACTIVE:
			eds_cb->mds_role = V_DEST_RL_ACTIVE;
			TRACE_1("New HA state: ACTIVE");
			break;
		case SA_AMF_HA_STANDBY:
			eds_cb->mds_role = V_DEST_RL_STANDBY;
			TRACE_1("New HA state: STANDBY");
			break;
		case SA_AMF_HA_QUIESCED:
			eds_quiesced_state_handler(eds_cb, invocation);
			TRACE_1("New HA state: QUIESCED");
			break;
		case SA_AMF_HA_QUIESCING:
			eds_quiescing_state_handler(eds_cb, invocation);
			TRACE_1("New HA state: QUIESCING");
			break;
		default:
			TRACE_2("Invalid HA state: %d", new_haState);
			eds_invalid_state_handler(eds_cb, invocation);
		}

		if (new_haState == SA_AMF_HA_QUIESCED) {
			TRACE_LEAVE2("New state is Quiesced, returning immediately");
			return;
		}

		/* Update control block */
		eds_cb->ha_state = new_haState;

		if (eds_cb->csi_assigned == false) {
			eds_cb->csi_assigned = true;
			/* We shall open checkpoint only once in our life time. currently doing at lib init  */
		} else if ((new_haState == SA_AMF_HA_ACTIVE) || (new_haState == SA_AMF_HA_STANDBY)) {	/* It is a switch over */
			eds_cb->ckpt_state = COLD_SYNC_IDLE;
		}
		if ((prev_haState == SA_AMF_HA_ACTIVE) && (new_haState == SA_AMF_HA_ACTIVE)) {
			role_change = false;
		}

		/*
		 * Handle Stby to Stby role change.
		 */
		if ((prev_haState == SA_AMF_HA_STANDBY) && (new_haState == SA_AMF_HA_STANDBY)) {
			role_change = false;
		}

		if (role_change == true) {
			if ((rc = eds_mds_change_role(eds_cb)) != NCSCC_RC_SUCCESS) {
				LOG_ER("MDS role change failed");
			} else {
				/* Declare as Implementer if state changed only from STANDBY to ACTIVE, 
				 * Need to check for transition from QUEISCED back to ACTIVE 
				 */
				if (eds_cb->ha_state == SA_AMF_HA_ACTIVE) {
					eds_imm_declare_implementer(eds_cb);
				}
			}
		}
		/* Inform MBCSV of HA state change */
		if (NCSCC_RC_SUCCESS != (error = eds_mbcsv_change_HA_state(eds_cb))) {
			LOG_ER("MBCSv state change failed");
		}

		TRACE_1("Responding to AMF CSI set callback");
		/* Send the response to AMF */
		saAmfResponse(eds_cb->amf_hdl, invocation, SA_AIS_OK);

		/* Do some OverHead Processing At New Active For EDA Down Events Cleanup if those reccords are present */
		if (eds_cb->ha_state == SA_AMF_HA_ACTIVE) {
			EDA_DOWN_LIST *eda_down_rec = NULL;
			EDA_DOWN_LIST *temp_eda_down_rec = NULL;

			eda_down_rec = eds_cb->eda_down_list_head;
			while (eda_down_rec) {
				/*Remove the EDA DOWN REC from the EDA_DOWN_LIST */
				/* Free the EDA_DOWN_REC */
				/* Remove this EDA entry from our processing lists */
				TRACE_1("Processing EDA down for:%" PRIx64 "at new Active", eda_down_rec->mds_dest);
				temp_eda_down_rec = eda_down_rec;
				eds_remove_regid_by_mds_dest(eds_cb, eda_down_rec->mds_dest);
				eda_down_rec = eda_down_rec->next;
				m_MMGR_FREE_EDA_DOWN_LIST(temp_eda_down_rec);
			}
			eds_cb->eda_down_list_head = NULL;
			eds_cb->eda_down_list_tail = NULL;
		}
		/*Give handles */
		ncshm_give_hdl(gl_eds_hdl);
	}
	TRACE_LEAVE();
}	/* End CSI set callback */

/****************************************************************************
 * Name          : eds_amf_comp_terminate_callback
 *
 * Description   : This is the callback function which will be called 
 *                 when the AMF framework needs to terminate EDS. This does
 *                 all required to destroy EDS(except to unregister from AMF)
 *
 * Arguments     : invocation     - This parameter designated a particular 
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it 
 *                                  responds to the Avilability Management 
 *                                  FrameWork using the saAmfResponse() 
 *                                  function.
 *                 compName       - A pointer to the name of the component 
 *                                  whose readiness stae the Availability 
 *                                  Management Framework is setting.
 *
 * Return Values : None
 *
 * Notes         : At present we are just support a simple liveness check.
 *****************************************************************************/
void eds_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	EDS_CB *eds_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();

	if (NULL == (eds_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		LOG_ER("Failed to retrieve eds handle: %u", gl_eds_hdl);
		TRACE_LEAVE();
		return;
	} else {
		TRACE_1("Responding to AMF CSI terminate callback");
		saAmfResponse(eds_cb->amf_hdl, invocation, error);
		/* Clean up all internal structures */
		eds_remove_reglist_entry(eds_cb, 0, true);

		/* Destroy the cb */
		eds_cb_destroy(eds_cb);
		/* Give back the handle */
		ncshm_give_hdl(gl_eds_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl);

		/* Detach from IPC */
		m_NCS_IPC_DETACH(&eds_cb->mbx, NULL, eds_cb);

		/* Disconnect from MDS */
		eds_mds_finalize(eds_cb);
		m_MMGR_FREE_EDS_CB(eds_cb);
		sleep(1);
		LOG_ER("Received AMF component terminate callback, exiting");
		exit(0);

	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : eds_amf_csi_rmv_callback
 *
 * Description   : This callback routine is invoked by AMF during a
 *                 CSI set removal operation. 
 *
 * Arguments     : invocation     - This parameter designated a particular
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it
 *                                  responds to the Avilability Management
 *                                  FrameWork using the saAmfResponse()
 *                                  function.
 *                 compName       - A pointer to the name of the component
 *                                  whose readiness stae the Availability
 *                                  Management Framework is setting.
 *                 csiName        - A const pointer to csiName
 *                 csiFlags       - csi Flags
 * Return Values : None 
 *****************************************************************************/
void
eds_amf_csi_rmv_callback(SaInvocationT invocation,
			 const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags)
{
	EDS_CB *eds_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();

	if (NULL == (eds_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		LOG_ER("Failed to retrieve eds handle: %u", gl_eds_hdl);
		TRACE_LEAVE();
		return;
	} else {
		TRACE_1("Responding to AMF CSI remove callback");
		saAmfResponse(eds_cb->amf_hdl, invocation, error);
		ncshm_give_hdl(gl_eds_hdl);
	}
	TRACE_LEAVE();
	return;
}

/*****************************************************************************\
 *  Name:          eds_healthcheck_start                           * 
 *                                                                            *
 *  Description:   To start the health check                                  *
 *                                                                            *
 *  Arguments:     NCSSA_CB* - Control Block                                  * 
 *                                                                            * 
 *  Returns:       NCSCC_RC_SUCCESS   - everything is OK                      *
 *                 NCSCC_RC_FAILURE   -  failure                              *
 *  NOTE:                                                                     * 
\******************************************************************************/
static uint32_t eds_healthcheck_start(EDS_CB *eds_cb)
{
	SaAisErrorT error;
	SaAmfHealthcheckKeyT Healthy;
	char *health_key = 0;
	TRACE_ENTER();
	
	if (eds_cb->healthCheckStarted == true) {
		TRACE_LEAVE2("Health check already started");
		return NCSCC_RC_SUCCESS;
	}

   /** start the AMF health check **/
	memset(&Healthy, 0, sizeof(Healthy));
	health_key = getenv("EDSV_ENV_HEALTHCHECK_KEY");
	if (health_key == NULL || strlen(health_key) > SA_AMF_HEALTHCHECK_KEY_MAX) {
		strcpy((char *)Healthy.key, "E5F6");
		/* Log it */
	} else {
		strncpy((char *)Healthy.key, health_key, SA_AMF_HEALTHCHECK_KEY_MAX);
	}
	Healthy.keyLen = strlen((char *)Healthy.key);

	error = saAmfHealthcheckStart(eds_cb->amf_hdl, &eds_cb->comp_name, &Healthy,
				      SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_FAILOVER);

	if (error != SA_AIS_OK) {
		TRACE_LEAVE2("saAmfHealthcheckStart failed");
		return NCSCC_RC_FAILURE;
	} else {
		eds_cb->healthCheckStarted = true;
		TRACE_LEAVE2("saAmfHealthcheckStart success");
		return NCSCC_RC_SUCCESS;
	}
}

/****************************************************************************
 * Name          : eds_amf_init
 *
 * Description   : EDS initializes AMF for invoking process and registers 
 *                 the various callback functions.
 *
 * Arguments     : EDS_CB - EDS control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t eds_amf_init(EDS_CB *eds_cb)
{

	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* Initialize amf callbacks */
	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));

	amfCallbacks.saAmfHealthcheckCallback = eds_amf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = eds_amf_CSI_set_callback;
	amfCallbacks.saAmfComponentTerminateCallback = eds_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = eds_amf_csi_rmv_callback;

	m_EDSV_GET_AMF_VER(amf_version);

	/*Initialize the amf library */

	rc = saAmfInitialize(&eds_cb->amf_hdl, &amfCallbacks, &amf_version);

	if (rc != SA_AIS_OK) {
		LOG_ER("saAmfInitialize failed with Error: %u", rc);
		return NCSCC_RC_FAILURE;
	}

	/* Obtain the amf selection object to wait for amf events */
	if (SA_AIS_OK != (rc = saAmfSelectionObjectGet(eds_cb->amf_hdl, &eds_cb->amfSelectionObject))) {
		LOG_ER("saAmfSelectionObjectGet failed with Error: %u", rc);
		return rc;
	}

	/* get the component name */
	rc = saAmfComponentNameGet(eds_cb->amf_hdl, &eds_cb->comp_name);
	if (rc != SA_AIS_OK) {
		LOG_ER("saAmfComponentNameGet failed with Error: %u", rc);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return rc;

}	/*End eds_amf_init */

/**************************************************************************
 Function: eds_amf_register

 Purpose:  Function which registers EDS with AMF.  

 Input:    Pointer to the EDS control block. 

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  Here we call eds_amf_init after reading the component name file and
         setting the environment varaiable in our own context.
         Proceed to register with AMF, since it has come up. 
**************************************************************************/
uint32_t eds_amf_register(EDS_CB *eds_cb)
{

	SaAisErrorT error;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	m_NCS_LOCK(&eds_cb->cb_lock, NCS_LOCK_WRITE);

	/* Initialize amf framework for hc interface */
	if ((rc = eds_amf_init(eds_cb)) != NCSCC_RC_SUCCESS) {
		m_NCS_UNLOCK(&eds_cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE2("Amf init failed");
		return NCSCC_RC_FAILURE;
	}

	/* register EDS component with AvSv */
	error = saAmfComponentRegister(eds_cb->amf_hdl, &eds_cb->comp_name, (SaNameT *)NULL);
	if (error != SA_AIS_OK) {
		m_NCS_UNLOCK(&eds_cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE2("Amf component register failed");
		return NCSCC_RC_FAILURE;
	}

	if (NCSCC_RC_SUCCESS != (rc = eds_healthcheck_start(eds_cb))) {
		m_NCS_UNLOCK(&eds_cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE2("Amf healthcheck start failed");
		return NCSCC_RC_FAILURE;
	}
	m_NCS_UNLOCK(&eds_cb->cb_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************
 * Function: eds_clm_init
 *
 * Purpose:  Function to register with the CLM service (Currently B01.01). 
 * Input  :    None
 *
 * Returns: SaAisErrorT  type
 *
 * Notes  : The CLM registration is done only after AMF registration is
 * done and not during the pre-amf period.
 **************************************************************************/

SaAisErrorT eds_clm_init(EDS_CB *cb)
{
	SaVersionT clm_version;
	SaClmCallbacksT clm_cbk;
	SaClmClusterNodeT cluster_node;
	SaClmClusterNotificationBufferT notify_buff;
	SaAisErrorT rc = SA_AIS_OK;
	SaTimeT timeout = EDSV_CLM_TIMEOUT;
	TRACE_ENTER();

	memset(&clm_version, 0, sizeof(SaVersionT));
	memset(&cluster_node, 0, sizeof(SaClmClusterNodeT));
	memset(&notify_buff, 0, sizeof(SaClmClusterNotificationBufferT));

	/* Fill version */
	m_EDSV_GET_CLM_VER(clm_version);

	clm_cbk.saClmClusterNodeGetCallback = NULL;
	clm_cbk.saClmClusterTrackCallback = eds_clm_cluster_track_cbk;

	/* Say Hello */
	rc = saClmInitialize(&cb->clm_hdl, &clm_cbk, &clm_version);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmInitialize failed with error: %d", rc);
		TRACE_LEAVE();
		return rc;
	}
	/* Get the FD */
	if (SA_AIS_OK != (rc = saClmSelectionObjectGet(cb->clm_hdl, &cb->clm_sel_obj))) {
		LOG_ER("saClmSelectionObjectGet failed with error: %d", rc);
		TRACE_LEAVE();
		return rc;
	}
	/* Get your own node id */
	rc = saClmClusterNodeGet(cb->clm_hdl, SA_CLM_LOCAL_NODE_ID, timeout, &cluster_node);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmClusterNodeGet failed with error: %d", rc);
		TRACE_LEAVE();
		return rc;
	}
	/* Get my node_id. Not sure if i need this at all */
	cb->node_id = cluster_node.nodeId;

	notify_buff.notification = NULL;
	rc = saClmClusterTrack(cb->clm_hdl, (SA_TRACK_CURRENT | SA_TRACK_CHANGES), NULL);
	if (rc != SA_AIS_OK)
		LOG_ER("saClmClusterTrack failed with error: %d", rc);

	TRACE_LEAVE();
	return rc;

}

/**************************************************************************
 * Function: eds_clm_cluster_track_cbk
 *
 * Purpose: The cluster track callback invoked by CLM whenever thereis a 
 *          change in the status of a node in the CLUSTER. 
 *          This callback is invoked whenever any of the following events
 *          occur on a cluster node:
 *          - SA_CLM_NODE_JOINED
 *          - SA_CLM_NODE_EXITED
 *          - SA_CLM_NODE_NO_CHANGE
 *          - SA_CLM_NODE_RECONFIGURED
 * Input  : none 
 * Output : NotificationBuffer that contains information of all nodes in the 
 *          cluster and the corresponding change that occured on that node.
 * Returns:   None 
 *
 * Notes  : Upon invocation of this callback, if a node joined or left the cluster,
 * EDS shall send an update to all the EDAs on that particular node.
 * This function updates the nodes list (in the cluster) accordingly
 * This list is checked while processing init messages from EDAs. All current 
 * member nodes of the cluster are maintained in this list. SA_AIS_ERR_UNAVAILABLE 
 * is retured for an saEvtInitialize() originating from a non member node.
 **************************************************************************/

void eds_clm_cluster_track_cbk(const SaClmClusterNotificationBufferT *notificationBuffer,
			       SaUint32T numberOfMembers, SaAisErrorT error)
{
	EDS_CB *cb = NULL;
	NODE_ID node_id;
	SaClmClusterChangesT cluster_change;
	SaBoolT is_member;
	uint32_t counter = 0;
	TRACE_ENTER();

	if (error == SA_AIS_OK) {
		/* Get EDS CB Handle. */
		if (NULL == (cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
			LOG_ER("Failed to retrieve eds handle: %u", gl_eds_hdl);
			TRACE_LEAVE();
			return;
		}

		if (notificationBuffer != NULL) {
			for (counter = 0; counter < notificationBuffer->numberOfItems; counter++) {
				is_member = notificationBuffer->notification[counter].clusterNode.member;
				node_id = notificationBuffer->notification[counter].clusterNode.nodeId;
				/* Send an update 
				 * - if there is 'change' in cluster membership to an existing node
				 * - if its a new node node joining the cluster
				 */ 
				if ((cluster_change = update_node_db(cb, node_id, is_member))) {
					/* There is a change to a cluster node.
					 * Send updates to all EDAs on that node
					 */
					if (is_member) {
						cluster_change = SA_CLM_NODE_JOINED;
						TRACE("CLM Node : %u Joined the cluster", node_id);
					}
					else {
						cluster_change = SA_CLM_NODE_LEFT;
						TRACE("CLM Node : %u Left the cluster", node_id);
					}

					/* Send to all EDAs on node_id */
					send_clm_status_change(cb, cluster_change, node_id);
				}
			}
		}
		else
			TRACE_1("Notification buffer is NULL");

		TRACE("After processing, clust list contains: %d nodes",ncs_patricia_tree_size(&cb->eds_cluster_nodes_list));
		TRACE("=======================================");
		NODE_INFO *tmp=NULL;
		tmp = (NODE_INFO *)ncs_patricia_tree_getnext(&cb->eds_cluster_nodes_list, (uint8_t *)0);
		while (tmp != NULL)
		{
			TRACE("Cluster Node Id : %u\n", tmp->node_id);
			tmp=(NODE_INFO *)ncs_patricia_tree_getnext(&cb->eds_cluster_nodes_list, (uint8_t *)&tmp->node_id);
		}
		TRACE("=======================================");
		ncshm_give_hdl(gl_eds_hdl);
	} else
		TRACE_1("Cluster track callback processing failed");

	TRACE_LEAVE();
	return;

}

/**************************************************************************
 * Function: update_node_db 
 *
 * Purpose: This function determines whether an update has to be sent
 * 	    to agents on the node for which CLM track callback has been
 * 	    received.
 * Input  : Control block, node_id  and is_member - both as received in CLM
 * 	    track callback. 
 * Output : SA_TRUE - If an update has to be sent to agents on that node.
 * 	    SA_FALSE - If no update has to be sent.  
 * 	    TBD - Either the node DB or this function(not so good),
 *	    can be eliminated if we can conclude from the CLM callback.
 *	    
 **************************************************************************/

SaBoolT update_node_db(EDS_CB *cb, NODE_ID node_id, SaBoolT is_member)
{
	NODE_INFO *cn = NULL;
	TRACE_ENTER();

	if (is_member) {
		/* Node has joined. Add it to our list if not present already */
		if ((cn = (NODE_INFO *)ncs_patricia_tree_get(&cb->eds_cluster_nodes_list,(uint8_t *)&node_id)) == NULL) {
			/* This is a new node joining the cluster. Add it to our DB */
			TRACE("Node = %u not existing in the DB, adding to DB", node_id);
			cn = (NODE_INFO *)malloc(sizeof(NODE_INFO));
			if (cn == NULL) {
				LOG_CR("Malloc failed for cluster node record");
				TRACE_LEAVE();
				return SA_FALSE;
			}
			cn->node_id = node_id;
			cn->pat_node.key_info = (uint8_t *)&cn->node_id;
			if (ncs_patricia_tree_add(&cb->eds_cluster_nodes_list,&cn->pat_node) != NCSCC_RC_SUCCESS) {
				free(cn);
				LOG_ER("Patricia add failed for cluster node %u", cn->node_id);
				TRACE_LEAVE();
				return SA_FALSE;
			} else {
				TRACE_LEAVE2("Node:  %u is a member, num nodes = %d", node_id, \
								ncs_patricia_tree_size(&cb->eds_cluster_nodes_list));
				return SA_TRUE;
			}
		} else {
			TRACE_LEAVE2("Node = %u, already a member", node_id);
			return SA_FALSE; /* Dont send an update */
		}
	} else {
		/* Node left the cluster. Remove from our list if this node is in our node DB*/
		if ((cn = (NODE_INFO *)ncs_patricia_tree_get(&cb->eds_cluster_nodes_list,(uint8_t *)&node_id)) == NULL) {
			TRACE_LEAVE2("Node = %d not in the DB, nothing to be done", node_id);
			return SA_FALSE;
		}
		if (ncs_patricia_tree_del(&cb->eds_cluster_nodes_list, &cn->pat_node) != NCSCC_RC_SUCCESS) {
			LOG_ER("Patricia tree delete failed for node_id = %d", node_id);
			TRACE_LEAVE();
			return SA_FALSE;
		} else {
			if (cn)
				free(cn);

			TRACE_LEAVE2("Node: %d not a member, deleted from DB", node_id);
			return SA_TRUE;
		}
	}/* Else node is not a member */
}

/**************************************************************************
 * Function: send_clm_status_change 
 *
 * Purpose: The cluster track callback invoked by CLM whenever thereis a
 *          change in the status of a node in the CLUSTER.
 * Input  : pointer to eds CB, cluster change and the node_id for this change.
 * Returns:   None
 * Notes  : Sends an asynch update message to all EDAs residing on the nodes
 * that have had a change in their cluster membership ((Re)joined/left).
 **************************************************************************/

void send_clm_status_change(EDS_CB *cb, SaClmClusterChangesT cluster_change, NODE_ID node_id)
{
	EDSV_MSG msg;
	EDA_REG_REC *reg_rec = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	m_EDS_EDSV_CLM_STATUS_CB_MSG_FILL(msg, cluster_change);
	reg_rec = (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)0);
	while (reg_rec != NULL) {
		if (node_id == (m_EDS_GET_NODE_ID_FROM_ADEST(reg_rec->eda_client_dest))) {
			rc = eds_mds_msg_send(cb, &msg, &reg_rec->eda_client_dest, NULL, MDS_SEND_PRIORITY_MEDIUM);
			if (rc != NCSCC_RC_SUCCESS)
				LOG_ER("Failed to send cluster change update to EDA. dest: %" PRIx64, reg_rec->eda_client_dest);
			else
				TRACE_1("Sent ClusterNodeUpdate = %u for %u", cluster_change, node_id);
		}
		reg_rec = (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)&reg_rec->reg_id_Net);
	}

	TRACE_LEAVE();
}

/**************************************************************************
 * Function: is_node_a_member
 *
 * Purpose : This function walks through the cluster_node_list to see if
 * the input node_id is present in the list or not.
 *
 * Input   : pointer to eds CB, the node_id.
 * Returns : true/false. 
 **************************************************************************/

bool is_node_a_member(EDS_CB *cb, NODE_ID node_id)
{
	NODE_INFO *cn = NULL;
	TRACE_ENTER();

	if ((cn = (NODE_INFO *)ncs_patricia_tree_get(&cb->eds_cluster_nodes_list,(uint8_t *)&node_id)) == NULL)
		return false;
	else
		return true;
	TRACE_LEAVE();
}

