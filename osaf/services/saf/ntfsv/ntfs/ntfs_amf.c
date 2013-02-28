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
 * This file implements the AMF interface for NTFS.
 * The interface exist of one exported function: ntfs_amf_init().
 * The AMF callback functions except a number of exported functions from
 * other modules.
 */
#include <nid_start_util.h>
#include "ntfs.h"
#include "ntfs_com.h"
#include "ntfs_imcnutil.h"

/****************************************************************************
 * Name          : amf_active_state_handler
 *
 * Description   : This function is called upon receving an active state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the NTFS control block. 
 *
 * Return Values : None
 *
 * Notes         : None 
 *****************************************************************************/
static SaAisErrorT amf_active_state_handler(SaInvocationT invocation)
{
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER2("HA ACTIVE request");

	ntfs_cb->mds_role = V_DEST_RL_ACTIVE;

	TRACE_LEAVE();
	return error;
}

/****************************************************************************
 * Name          : amf_standby_state_handler
 *
 * Description   : This function is called upon receving an standby state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the NTFS control block. 
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/
static SaAisErrorT amf_standby_state_handler(SaInvocationT invocation)
{
	TRACE_ENTER2("HA STANDBY request");
	ntfs_cb->mds_role = V_DEST_RL_STANDBY;
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
 *                 cb         - A pointer to the NTFS control block. 
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/
static SaAisErrorT amf_quiescing_state_handler(SaInvocationT invocation)
{
	TRACE_ENTER2("HA QUIESCING request");
	return saAmfCSIQuiescingComplete(ntfs_cb->amf_hdl, invocation, SA_AIS_OK);
}

/****************************************************************************
 * Name          : amf_quiesced_state_handler
 *
 * Description   : This function is called upon receving an Quiesced state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the NTFS control block. 
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/
static SaAisErrorT amf_quiesced_state_handler(SaInvocationT invocation)
{
	TRACE_ENTER2("HA AMF QUIESCED STATE request");
	/*
	 ** Change the MDS VDSET role to Quiesced. Wait for MDS callback with type
	 ** MDS_CALLBACK_QUIESCED_ACK. Then change MBCSv role. Don't change
	 ** ntfs_cb->ha_state now.
	 */

	ntfs_cb->mds_role = V_DEST_RL_QUIESCED;
	ntfs_mds_change_role();
	ntfs_cb->amf_invocation_id = invocation;
	ntfs_cb->is_quisced_set = true;
	return SA_AIS_OK;
}

/****************************************************************************
 * Name          : amf_health_chk_callback
 *
 * Description   : This is the callback function which will be called 
 *                 when the AMF framework nentfs to health check for the component.
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
static void amf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	saAmfResponse(ntfs_cb->amf_hdl, invocation, SA_AIS_OK);
}

/****************************************************************************
 * Name          : amf_csi_set_callback
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
static void amf_csi_set_callback(SaInvocationT invocation,
				 const SaNameT *compName, SaAmfHAStateT new_haState, SaAmfCSIDescriptorT csiDescriptor)
{
	SaAisErrorT error = SA_AIS_OK;
	SaAmfHAStateT prev_haState;
	bool role_change = true;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/*
	 *  Handle Active to Active role change.
	 */
	prev_haState = ntfs_cb->ha_state;

	if (prev_haState == SA_AMF_HA_STANDBY &&
	    new_haState == SA_AMF_HA_ACTIVE && ntfs_cb->ckpt_state != COLD_SYNC_COMPLETE) {
		/* We are not synched and cannot take over */
		LOG_ER("NTFS cannot take over not cold synched");
		error = SA_AIS_ERR_FAILED_OPERATION;
		goto response;
	}

	/* Invoke the appropriate state handler routine */
	switch (new_haState) {
	case SA_AMF_HA_ACTIVE:
		error = amf_active_state_handler(invocation);
		break;
	case SA_AMF_HA_STANDBY:
		error = amf_standby_state_handler(invocation);
		break;
	case SA_AMF_HA_QUIESCED:
		error = amf_quiesced_state_handler(invocation);
		break;
	case SA_AMF_HA_QUIESCING:
		error = amf_quiescing_state_handler(invocation);
		break;
	default:
		LOG_WA("invalid state: %d ", new_haState);
		error = SA_AIS_ERR_BAD_OPERATION;
		break;
	}

	if (error != SA_AIS_OK)
		goto response;

	if (new_haState == SA_AMF_HA_QUIESCED)
		goto done;

	/* Update control block */
	ntfs_cb->ha_state = new_haState;
	restart_ntfimcn(ntfs_cb->ha_state);

	if (ntfs_cb->csi_assigned == false) {
		ntfs_cb->csi_assigned = true;
		/* We shall open checkpoint only once in our life time. currently doing at lib init  */
	} else if ((new_haState == SA_AMF_HA_ACTIVE) || (new_haState == SA_AMF_HA_STANDBY)) {	/* It is a switch over */
		/* NOTE: This behaviour has to be checked later, when scxb redundancy is available 
		 * Also, change role of mds, mbcsv during quiesced has to be done after mds
		 * supports the same.  TBD
		 */
	}

	/* Handle active to active role change. */
	if ((prev_haState == SA_AMF_HA_ACTIVE) && (new_haState == SA_AMF_HA_ACTIVE))
		role_change = false;

	/* Handle Stby to Stby role change. */
	if ((prev_haState == SA_AMF_HA_STANDBY) && (new_haState == SA_AMF_HA_STANDBY))
		role_change = false;

	if (role_change == true) {
		if ((rc = ntfs_mds_change_role()) != NCSCC_RC_SUCCESS) {
			LOG_ER("ntfs_mds_change_role FAILED");
			error = SA_AIS_ERR_FAILED_OPERATION;
		}

		/* Inform MBCSV of HA state change */
		if (NCSCC_RC_SUCCESS != (error = ntfs_mbcsv_change_HA_state(ntfs_cb)))
			error = SA_AIS_ERR_FAILED_OPERATION;
	}

 response:
	saAmfResponse(ntfs_cb->amf_hdl, invocation, error);
	if ((new_haState == SA_AMF_HA_ACTIVE) && (role_change == true)) {
		/* check for unsent notifictions and if notifiction is not logged */
		checkNotificationList();
	}
 done:
	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : amf_comp_terminate_callback
 *
 * Description   : This is the callback function which will be called 
 *                 when the AMF framework nentfs to terminate NTFS. This does
 *                 all required to destroy NTFS(except to unregister from AMF)
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
 * Notes         : None
 *****************************************************************************/
static void amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	SaAisErrorT rc=SA_AIS_OK;

	TRACE_ENTER();

	/* Stop imcnp (configuration notifier) */
	if (stop_ntfimcn() != 0) {
		rc = SA_AIS_ERR_FAILED_OPERATION;
		LOG_ER("stop_ntfimcn() Fail");
	}

	saAmfResponse(ntfs_cb->amf_hdl, invocation, rc);

	/* Detach from IPC */
	m_NCS_IPC_DETACH(&ntfs_cb->mbx, NULL, ntfs_cb);

	/* Disconnect from MDS */
	ntfs_mds_finalize(ntfs_cb);
	sleep(1);
	LOG_ER("Received AMF component terminate callback, exiting");
	exit(0);
}

/****************************************************************************
 * Name          : amf_csi_rmv_callback
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
static void amf_csi_rmv_callback(SaInvocationT invocation,
				 const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags)
{
	TRACE_ENTER();
	saAmfResponse(ntfs_cb->amf_hdl, invocation, SA_AIS_OK);
	TRACE_LEAVE();
}

/*****************************************************************************\
 *  Name:          ntfs_healthcheck_start                           * 
 *                                                                            *
 *  Description:   To start the health check                                  *
 *                                                                            *
 *  Arguments:     NCSSA_CB* - Control Block                                  * 
 *                                                                            * 
 *  Returns:       SA_AIS_OK    - everything is OK                            *
 *                 SA_AIS_ERR_* -  failure                                    *
 *  NOTE:                                                                     * 
\******************************************************************************/
SaAisErrorT ntfs_amf_healthcheck_start()
{
	SaAisErrorT error;
	SaAmfHealthcheckKeyT healthy;
	char *health_key;

	TRACE_ENTER();

    /** start the AMF health check **/
	memset(&healthy, 0, sizeof(healthy));
	health_key = (char *)getenv("NTFSV_ENV_HEALTHCHECK_KEY");

	if (health_key == NULL) {
		strncpy((char *)healthy.key, "F1B2", SA_AMF_HEALTHCHECK_KEY_MAX);
		healthy.keyLen = strlen((const char *)healthy.key);
	} else {
		healthy.keyLen = strlen(health_key);
		if (healthy.keyLen > sizeof(healthy.key))
			healthy.keyLen = sizeof(healthy.key);
		strncpy((char *)healthy.key, health_key, healthy.keyLen);
	}

	error = saAmfHealthcheckStart(ntfs_cb->amf_hdl, &ntfs_cb->comp_name, &healthy,
				      SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_FAILOVER);

	if (error != SA_AIS_OK)
		LOG_ER("saAmfHealthcheckStart FAILED: %u", error);

	TRACE_LEAVE();
	return error;
}

/**************************************************************************
 Function: ntfs_amf_register

 Purpose:  Function which registers NTFS with AMF.  

 Input:    None 

 Returns:  SA_AIS_OK    - everything is OK
           SA_AIS_ERR_* -  failure

**************************************************************************/
SaAisErrorT ntfs_amf_init()
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT error = SA_AIS_OK;

	TRACE_ENTER();

	if (amf_comp_name_get_set_from_file("NTFD_COMP_NAME_FILE", &ntfs_cb->comp_name) != NCSCC_RC_SUCCESS)
		goto done;

	/* Initialize AMF callbacks */
	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));
	amfCallbacks.saAmfHealthcheckCallback = amf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = amf_csi_set_callback;
	amfCallbacks.saAmfComponentTerminateCallback = amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = amf_csi_rmv_callback;

	amf_version.releaseCode = 'B';
	amf_version.majorVersion = 0x01;
	amf_version.minorVersion = 0x01;

	/* Initialize the AMF library */
	error = saAmfInitialize(&ntfs_cb->amf_hdl, &amfCallbacks, &amf_version);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfInitialize() FAILED: %u", error);
		goto done;
	}

	/* Obtain the AMF selection object to wait for AMF events */
	error = saAmfSelectionObjectGet(ntfs_cb->amf_hdl, &ntfs_cb->amfSelectionObject);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfSelectionObjectGet() FAILED: %u", error);
		goto done;
	}

	/* Get the component name */
	error = saAmfComponentNameGet(ntfs_cb->amf_hdl, &ntfs_cb->comp_name);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentNameGet() FAILED: %u", error);
		goto done;
	}

	/* Register component with AMF */
	error = saAmfComponentRegister(ntfs_cb->amf_hdl, &ntfs_cb->comp_name, (SaNameT *)NULL);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentRegister() FAILED");
		goto done;
	}

	/* Start AMF healthchecks */
	if (ntfs_amf_healthcheck_start() != SA_AIS_OK){
		LOG_ER("ntfs_amf_healthcheck_start() FAILED");
		error = NCSCC_RC_FAILURE;
		goto done;
	}

 done:
	TRACE_LEAVE();
	return error;
}
