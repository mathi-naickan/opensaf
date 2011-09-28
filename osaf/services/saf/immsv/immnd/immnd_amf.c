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

#include "immnd.h"
#include <nid_start_util.h>

#define NCS_SAF_ACCEPT_TIME 1000

/****************************************************************************
 * Name          : immnd_saf_health_chk_callback
 *
 * Description   : This function SAF callback function which will be called 
 *                 when the AMF framework needs to health for the component.
 *
 * Arguments     : invocation     - This parameter designated a particular 
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it 
 *                                  responds to the Availability Management 
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
static void immnd_saf_health_chk_callback(SaInvocationT invocation,
					  const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	TRACE_ENTER();

	if (saAmfResponse(immnd_cb->amf_hdl, invocation, SA_AIS_OK) != SA_AIS_OK)
		LOG_ER("Amf Response Failed");
}

/****************************************************************************
 * Name          : immnd_amf_comp_terminate_callback
 *
 * Description   : This function SAF callback function which will be called 
 *                 when the AMF framework needs to terminate GLSV. This does
 *                 all required to destroy GLSV(except to unregister from AMF)
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
static void immnd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	TRACE_ENTER();
	saAmfResponse(immnd_cb->amf_hdl, invocation, SA_AIS_OK);
	sleep(1);
	TRACE_LEAVE();
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);
}

/****************************************************************************
 * Name          : immnd_amf_csi_rmv_callback
 *
 * Description   : TBD
 *
 *
 * Return Values : None 
 *****************************************************************************/
static void immnd_amf_csi_rmv_callback(SaInvocationT invocation,
				       const SaNameT *compName, const SaNameT *csiName, SaAmfCSIFlagsT csiFlags)
{
	TRACE_ENTER();
	saAmfResponse(immnd_cb->amf_hdl, invocation, SA_AIS_OK);
	TRACE_LEAVE();
	return;
}

/****************************************************************************\
 PROCEDURE NAME : immnd_saf_csi_set_cb
 
 DESCRIPTION    : This function SAF callback function which will be called 
                  when there is any change in the HA state.
 
 ARGUMENTS      : invocation     - This parameter designated a particular 
                                  invocation of this callback function. The 
                                  invoke process return invocation when it 
                                  responds to the Avilability Management 
                                  FrameWork using the saAmfResponse() 
                                  function.
                 compName       - A pointer to the name of the component 
                                  whose readiness stae the Availability 
                                  Management Framework is setting.
                 haState        - The new HA state to be assumeb by the 
                                  component service instance identified by 
                                  csiName.
\****************************************************************************/
static void immnd_saf_csi_set_cb(SaInvocationT invocation,
				 const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	SaAisErrorT saErr = SA_AIS_OK;

	TRACE_ENTER();
	immnd_cb->ha_state = haState;	/* Set the HA State */
	/*TODO: this ha_state is never accessed by any other part of immnd.
	   Perhaps it should be? Perhaps we should wait in responding until
	   we have synced? Perhaps we should check this state as being active
	   before allowing any service request to be completed? In particular
	   mutating requests such as create a new CCB. */

	saAmfResponse(immnd_cb->amf_hdl, invocation, saErr);
	TRACE_2("CSI Set Callback Invoked ha-state:%u", haState);
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : immnd_amf_init
 *
 * Description   : IMMND initializes AMF for involking process and registers 
 *                 the various callback functions.
 *
 * Arguments     : immnd_cb - control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 immnd_amf_init(IMMND_CB *cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT version;
	SaAisErrorT error;
	uns32 res = NCSCC_RC_FAILURE;
	SaClmClusterNodeT cluster_node;
	const char *health_key;
	SaAmfHealthcheckKeyT healthy;

	TRACE_ENTER();

	memset(&cluster_node, 0, sizeof(SaClmClusterNodeT));

	if (cb->nid_started &&
	    (amf_comp_name_get_set_from_file("IMMND_COMP_NAME_FILE", &cb->comp_name) != NCSCC_RC_SUCCESS))
		goto done;

	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));
	amfCallbacks.saAmfHealthcheckCallback = immnd_saf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = immnd_saf_csi_set_cb;
	amfCallbacks.saAmfComponentTerminateCallback = immnd_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = immnd_amf_csi_rmv_callback;

	m_IMMSV_GET_AMF_VER(version);

	error = saAmfInitialize(&cb->amf_hdl, &amfCallbacks, &version);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfInitialize failed");
		goto done;
	}

	error = saAmfComponentNameGet(cb->amf_hdl, &cb->comp_name);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentNameGet failed");
		goto done;
	}

	error = saAmfSelectionObjectGet(cb->amf_hdl, &cb->amf_sel_obj);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfSelectionObjectGet failed");
		goto done;
	}

	error = saAmfComponentRegister(cb->amf_hdl, &cb->comp_name, NULL);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentRegister failed: %u", error);
		goto done;
	}

	/*   start the AMF Health Check  */
	memset(&healthy, 0, sizeof(healthy));
	health_key = getenv("IMMSV_ENV_HEALTHCHECK_KEY");

	if (health_key == NULL) {
		strcpy((char *)healthy.key, "A1B2");
		healthy.keyLen = strlen((char *)healthy.key);
	} else {
		healthy.keyLen = strlen((char *)health_key);
		if (healthy.keyLen <= SA_MAX_NAME_LENGTH) {
			strcpy((char *)healthy.key, health_key);
		} else {
			LOG_ER("Health check key too long:%u", healthy.keyLen);
			/* SA_MAX_NAME_LENGTH is an arbitrary length delimiter in this 
			   case. On the other hand, it should be long enough for all
			   reasonable health check keys */
			goto done;
		}
	}

	error = saAmfHealthcheckStart(cb->amf_hdl, &cb->comp_name, &healthy,
				      SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_RESTART);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfHealthcheckStart failed: %u", error);
		goto done;
	}

	res = NCSCC_RC_SUCCESS;

 done:
	TRACE_LEAVE2("%u, %s", res, cb->comp_name.value);
	return (res);
}
