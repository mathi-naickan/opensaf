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
 * Author(s): Emerson  Network Power
 *
 * This file implements the AMF interface for CLMS.
 * The interface exist of one exported function: clms_amf_init().
 * The AMF callback functions except a number of exported functions from
 * other modules.
 */

#include <nid_start_util.h>
#include "clms.h"
#include "immutil.h"

/****************************************************************************
 * Name          : amf_active_state_handler
 *
 * Description   : This function is called upon receving an active state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the CLMS control block. 
 *
 * Return Values : None
 *
 * Notes         : None 
 *****************************************************************************/
static SaAisErrorT amf_active_state_handler(CLMS_CB * cb, SaInvocationT invocation)
{
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2("AMF HA ACTIVE request");

	clms_cb->mds_role = V_DEST_RL_ACTIVE;

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : amf_standby_state_handler
 *
 * Description   : This function is called upon receving an standby state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the CLMS control block. 
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/
static SaAisErrorT amf_standby_state_handler(CLMS_CB * cb, SaInvocationT invocation)
{
	TRACE_ENTER2("HA STANDBY request");

	cb->mds_role = V_DEST_RL_STANDBY;

	TRACE_LEAVE();
	return SA_AIS_OK;
}

/****************************************************************************
 * Name          : amf_quiescing_state_handler
 *
 * Description   : This function is called upon receving an Quiescing state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the CLMS control block. 
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/
static SaAisErrorT amf_quiescing_state_handler(CLMS_CB * cb, SaInvocationT invocation)
{
	TRACE_ENTER2("HA QUIESCING request");

	/* Give up our IMM OI implementer role */
	(void)immutil_saImmOiImplementerClear(cb->immOiHandle);

	TRACE_LEAVE();
	return saAmfCSIQuiescingComplete(cb->amf_hdl, invocation, SA_AIS_OK);
}

/****************************************************************************
 * Name          : amf_quiesced_state_handler
 *
 * Description   : This function is called upon receving an Quiesced state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the CLMS control block. 
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/
static SaAisErrorT amf_quiesced_state_handler(CLMS_CB * cb, SaInvocationT invocation)
{
	TRACE_ENTER2("HA AMF QUIESCED STATE request");
	SaUint32T nodeid = 0;
	CLMS_CLUSTER_NODE *node = NULL;

	/* Give up our IMM OI implementer role */
	(void)immutil_saImmOiImplementerClear(cb->immOiHandle);

	/*Stop timer if the switchover happens in middle of admin op */
	while (NULL != (node = clms_node_getnext_by_id(nodeid))) {
		nodeid = node->node_id;
		if (node->lock_timerid)
			timer_delete(node->lock_timerid);
		/*Crosscheck if the admin op and stat_change also needs to be cleared */
	}

	/*
	 ** Change the MDS VDSET role to Quiesced. Wait for MDS callback with type
	 ** MDS_CALLBACK_QUIESCED_ACK. Then change MBCSv role. Don't change
	 ** cb->ha_state now.
	 */

	cb->mds_role = V_DEST_RL_QUIESCED;
	clms_mds_change_role(cb);
	cb->amf_inv = invocation;
	cb->is_quiesced_set = TRUE;

	TRACE_LEAVE();
	return SA_AIS_OK;
}

/****************************************************************************
 * Name          : clms_amf_health_chk_callback
 *
 * Description   : This is the callback function which will be called 
 *                 when the AMF framework health checskfor the component.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 compName       - A pointer to the name of the component 
 *                                  whose readiness stae the Availability 
 *                                  Management Framework is setting.
 *                 checkType      - The type of healthcheck to be executed. 
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/
static void clms_amf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName,
					 SaAmfHealthcheckKeyT *checkType)
{
	saAmfResponse(clms_cb->amf_hdl, invocation, SA_AIS_OK);
}

/****************************************************************************
 * Name          : clms_amf_csi_set_callback
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
 *                                  whose readiness state the Availability 
 *                                  Management Framework is setting.
 *                 haState        - The new HA state to be assumed by the 
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
static void clms_amf_csi_set_callback(SaInvocationT invocation,
				      const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	SaAisErrorT error = SA_AIS_OK;
	SaAmfHAStateT prev_haState;
	NCS_BOOL role_change = TRUE;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/*
	 *  Handle Active to Active role change.
	 */
	prev_haState = clms_cb->ha_state;

	/* Invoke the appropriate state handler routine */
	switch (haState) {
	case SA_AMF_HA_ACTIVE:
		error = amf_active_state_handler(clms_cb, invocation);
		break;
	case SA_AMF_HA_STANDBY:
		error = amf_standby_state_handler(clms_cb, invocation);
		break;
	case SA_AMF_HA_QUIESCED:
		/* switch-over */
		error = amf_quiesced_state_handler(clms_cb, invocation);
		break;
	case SA_AMF_HA_QUIESCING:
		/* shutdown admin op */
		error = amf_quiescing_state_handler(clms_cb, invocation);
		break;
	default:
		LOG_WA("invalid state: %d ", haState);
		error = SA_AIS_ERR_FAILED_OPERATION;
		break;
	}

	if (error != SA_AIS_OK)
		goto response;

	if (haState == SA_AMF_HA_QUIESCED) {
		/* AMF response will be done later when MDS quiesced ack has been received */
		goto done;
	}

	/* Update control block */
	clms_cb->ha_state = haState;

	if (clms_cb->csi_assigned == FALSE) {
		clms_cb->csi_assigned = TRUE;
		/* We shall open checkpoint only once in our life time. currently doing at lib init  */
	} else if ((haState == SA_AMF_HA_ACTIVE) || (haState == SA_AMF_HA_STANDBY)) {	/* It is a switch over */
		clms_cb->ckpt_state = COLD_SYNC_IDLE;
		/* NOTE: This behaviour has to be checked later, when scxb redundancy is available 
		 * Also, change role of mds, mbcsv during quiesced has to be done after mds
		 * supports the same.  TBD
		 */
	}

	/* Handle active to active role change. */
	if ((prev_haState == SA_AMF_HA_ACTIVE) && (haState == SA_AMF_HA_ACTIVE))
		role_change = FALSE;

	/* Handle Stby to Stby role change. */
	if ((prev_haState == SA_AMF_HA_STANDBY) && (haState == SA_AMF_HA_STANDBY))
		role_change = FALSE;

	if (role_change == TRUE) {

		if(clms_cb->ha_state == SA_AMF_HA_ACTIVE)
			clms_imm_impl_set(clms_cb);

		if ((rc = clms_mds_change_role(clms_cb)) != NCSCC_RC_SUCCESS) {
			LOG_ER("clms_mds_change_role FAILED");
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		/* Inform MBCSV of HA state change */
		if (NCSCC_RC_SUCCESS != (error = clms_mbcsv_change_HA_state(clms_cb)))
			error = SA_AIS_ERR_FAILED_OPERATION;
	}

 response:
	saAmfResponse(clms_cb->amf_hdl, invocation, error);
 done:
	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : clms_amf_comp_terminate_callback
 *
 * Description   : This is the callback function which will be called 
 *                 when the AMF framework needs to terminate CLMS. This does
 *                 all required to destroy CLMS(except to unregister from AMF)
 *
 * Arguments     : invocation     - This parameter designated a particular 
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it 
 *                                  responds to the Avilability Management 
 *                                  FrameWork using the saAmfResponse() 
 *                                  function.
 *                 compName       - A pointer to the name of the component 
 *                                  whose readiness state the Availability 
 *                                  Management Framework is setting.
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/
static void clms_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	TRACE_ENTER();

	saAmfResponse(clms_cb->amf_hdl, invocation, SA_AIS_OK);

	/* Detach from IPC */
	m_NCS_IPC_DETACH(&clms_cb->mbx, NULL, clms_cb);

	/* Disconnect from MDS */
	clms_mds_finalize(clms_cb);
	sleep(1);

	TRACE_LEAVE();
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);
}

/****************************************************************************
 * Name          : clms_amf_csi_rmv_callback
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
static void clms_amf_csi_rmv_callback(SaInvocationT invocation,
				      const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags)
{
	TRACE_ENTER();

	saAmfResponse(clms_cb->amf_hdl, invocation, SA_AIS_OK);

	TRACE_LEAVE();
}

/*****************************************************************************\
 *  Name:          clms_healthcheck_start                           * 
 *                                                                            *
 *  Description:   To start the health check                                  *
 *                                                                            *
 *  Arguments:     NCSSA_CB* - Control Block                                  * 
 *                                                                            * 
 *  Returns:       SA_AIS_OK    - everything is OK                            *
 *                 SA_AIS_ERR_* -  failure                                    *
 *  NOTE:                                                                     * 
\******************************************************************************/
static SaAisErrorT clms_amf_healthcheck_start(CLMS_CB * clms_cb)
{
	SaAisErrorT error;
	SaAmfHealthcheckKeyT healthy;
	char *health_key;

	TRACE_ENTER();

    /** start the AMF health check **/
	memset(&healthy, 0, sizeof(healthy));
	health_key = getenv("CLMSV_ENV_HEALTHCHECK_KEY");

	/*TBD : adapt to new health check mechanism */
	if (health_key == NULL)
		strcpy((char *)healthy.key, "F1B2");
	else
		strcpy((char *)healthy.key, health_key);

	healthy.keyLen = strlen((char *)healthy.key);

	error = saAmfHealthcheckStart(clms_cb->amf_hdl, &clms_cb->comp_name, &healthy,
				      SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_FAILOVER);

	if (error != SA_AIS_OK)
		LOG_ER("saAmfHealthcheckStart FAILED: %u", error);

	TRACE_LEAVE();
	return error;
}

/**************************************************************************
 Function: clms_amf_init

 Purpose:  Function which registers CLMS with AMF.  

 Input:    None 

 Returns:  SA_AIS_OK    - everything is OK
           SA_AIS_ERR_* -  failure

**************************************************************************/
SaAisErrorT clms_amf_init(CLMS_CB * cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT error = SA_AIS_OK;

	TRACE_ENTER();

	if (amf_comp_name_get_set_from_file("CLMD_COMP_NAME_FILE", &cb->comp_name) != NCSCC_RC_SUCCESS)
		goto done;

	/* Initialize AMF callbacks */
	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));
	amfCallbacks.saAmfHealthcheckCallback = clms_amf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = clms_amf_csi_set_callback;
	amfCallbacks.saAmfComponentTerminateCallback = clms_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = clms_amf_csi_rmv_callback;

	amf_version.releaseCode = 'B';
	amf_version.majorVersion = 0x01;
	amf_version.minorVersion = 0x01;

	/* Initialize the AMF library */
	error = saAmfInitialize(&cb->amf_hdl, &amfCallbacks, &amf_version);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfInitialize() FAILED: %u", error);
		goto done;
	}

	/* Obtain the AMF selection object to wait for AMF events */
	error = saAmfSelectionObjectGet(cb->amf_hdl, &cb->amf_sel_obj);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfSelectionObjectGet() FAILED: %u", error);
		goto done;
	}

	/* Get the component name */
	error = saAmfComponentNameGet(cb->amf_hdl, &cb->comp_name);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentNameGet() FAILED: %u", error);
		goto done;
	}

	/* Register component with AMF */
	error = saAmfComponentRegister(cb->amf_hdl, &cb->comp_name, (SaNameT *)NULL);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentRegister() FAILED");
		goto done;
	}

	/* Start AMF healthchecks */
	if ((error = clms_amf_healthcheck_start(cb)) != SA_AIS_OK)
		goto done;

 done:
	TRACE_LEAVE();
	return error;
}
